/*
 * gimmix-lyrics.c: lyrics plugin
 *
 * Copyright (C) 2008-2009 Priyank Gosalia
 * Copyright (C) 2012-2026 Freydis Mende
 *
 * Gimmix is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Gimmix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gimmix; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Priyank Gosalia <priyankmg@gmail.com>
 *
 * Lyrics come from LRCLIB (https://lrclib.net) and are cached in
 * ~/.gimmix/lyrics. Network and file work happens in a worker thread;
 * everything that touches GTK or MPD runs in the main loop.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#ifdef HAVE_LYRICS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <json-glib/json-glib.h>

#include "gimmix.h"
#include "gimmix-core.h"
#include "gimmix-config.h"
#include "gimmix-http.h"
#include "gimmix-metadata.h"
#include "gimmix-lyrics.h"
#include "wejpconfig.h"

#define LYRICS_DIR	".gimmix/lyrics"
#define LRCLIB_GET_URL	"https://lrclib.net/api/get?"
#define LRCLIB_SEARCH_URL "https://lrclib.net/api/search?"

/* a lyrics lookup running in a worker thread */
typedef struct {
	guint	generation;
	gchar	*artist;
	gchar	*title;
	gchar	*album;
	guint	duration;
	gchar	*proxy;
	gchar	*cache;		/* file the lyrics are stored in */
	gchar	*lyrics;	/* the result, or NULL */
} LyricsJob;

extern GtkBuilder	*xml;
extern MpdObj		*gmo;
extern ConfigFile	conf;

static GtkWidget	*lyrics_textview = NULL;
static gchar		*lyrics_dir = NULL;
static guint		generation = 0;

static void cb_gimmix_lyrics_get_btn_clicked (GtkWidget *widget, gpointer data);

void
gimmix_lyrics_plugin_init (void)
{
	gimmix_http_init ();

	lyrics_textview = GTK_WIDGET (gtk_builder_get_object (xml, "lyrics_textview"));
	g_signal_connect (gtk_builder_get_object (xml,"lyrics_get_btn"),
				"clicked",
				G_CALLBACK(cb_gimmix_lyrics_get_btn_clicked),
				NULL);

	lyrics_dir = cfg_get_path_to_config_file (LYRICS_DIR);
	g_mkdir_with_parents (lyrics_dir, 0755);
}

void
gimmix_lyrics_plugin_cleanup (void)
{
	generation++; /* results of running lookups are dropped */
	g_free (lyrics_dir);
	lyrics_dir = NULL;
}

static void
gimmix_lyrics_set_text (const gchar *text)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(lyrics_textview));

	if (buffer != NULL)
		gtk_text_buffer_set_text (buffer, text != NULL ? text : "", -1);
}

/* the cache file of a song; tags may contain anything, so keep it to one name */
static gchar *
gimmix_lyrics_cache_file (const gchar *artist, const gchar *title)
{
	gchar *name = g_strdup_printf ("%s-%s.txt", artist, title);
	gchar *path;

	for (gchar *p = name; *p != '\0'; p++)
		if (*p == '/' || *p == G_DIR_SEPARATOR)
			*p = '_';
	if (name[0] == '.')
		name[0] = '_';
	path = g_build_filename (lyrics_dir, name, NULL);
	g_free (name);

	return path;
}

static void
lyrics_job_free (LyricsJob *job)
{
	g_free (job->artist);
	g_free (job->title);
	g_free (job->album);
	g_free (job->proxy);
	g_free (job->cache);
	g_free (job->lyrics);
	g_free (job);
}

/* the plain lyrics of a LRCLIB record, or NULL */
static gchar *
lrclib_plain_lyrics (JsonObject *record)
{
	const gchar *text;

	if (record == NULL || !json_object_has_member (record, "plainLyrics"))
		return NULL;
	if (json_node_is_null (json_object_get_member (record, "plainLyrics")))
		return NULL;
	text = json_object_get_string_member (record, "plainLyrics");

	return (text != NULL && *text != '\0') ? g_strdup (text) : NULL;
}

static gchar *
lyrics_job_fetch (LyricsJob *job)
{
	static const gchar *headers[] = {
		"Accept: application/json",
		"Lrclib-Client: gimmix/" VERSION " (https://github.com/nyxdis/gimmix)",
		NULL
	};
	JsonParser	*parser;
	JsonNode	*root;
	gchar		*artist_e, *title_e, *album_e, *url, *body = NULL;
	gchar		*lyrics = NULL;
	gsize		length = 0;

	artist_e = gimmix_http_escape (job->artist);
	title_e = gimmix_http_escape (job->title);
	album_e = gimmix_http_escape (job->album);
	parser = json_parser_new ();

	/* exact lookup first, it needs the album and the duration */
	if (job->album != NULL && job->duration > 0)
	{
		url = g_strdup_printf ("%sartist_name=%s&track_name=%s&album_name=%s&duration=%u",
				       LRCLIB_GET_URL, artist_e, title_e, album_e, job->duration);
		if (gimmix_http_get (url, job->proxy, headers, &body, &length) &&
		    json_parser_load_from_data (parser, body, length, NULL) &&
		    (root = json_parser_get_root (parser)) != NULL && JSON_NODE_HOLDS_OBJECT (root))
		{
			lyrics = lrclib_plain_lyrics (json_node_get_object (root));
		}
		g_free (url);
		g_free (body);
		body = NULL;
	}

	/* otherwise search and take the first record that has lyrics */
	if (lyrics == NULL)
	{
		url = g_strdup_printf ("%sartist_name=%s&track_name=%s", LRCLIB_SEARCH_URL, artist_e, title_e);
		if (gimmix_http_get (url, job->proxy, headers, &body, &length) &&
		    json_parser_load_from_data (parser, body, length, NULL) &&
		    (root = json_parser_get_root (parser)) != NULL && JSON_NODE_HOLDS_ARRAY (root))
		{
			JsonArray *results = json_node_get_array (root);
			for (guint i = 0; i < json_array_get_length (results) && lyrics == NULL; i++)
				lyrics = lrclib_plain_lyrics (json_array_get_object_element (results, i));
		}
		g_free (url);
		g_free (body);
	}

	g_object_unref (parser);
	g_free (artist_e);
	g_free (title_e);
	g_free (album_e);

	return lyrics;
}

static gboolean
lyrics_job_done (gpointer data)
{
	LyricsJob *job = data;

	if (job->generation == generation)
	{
		if (job->lyrics != NULL)
			gimmix_lyrics_set_text (job->lyrics);
		else
			gimmix_lyrics_set_text (_("Lyrics not found"));
	}
	lyrics_job_free (job);

	return FALSE;
}

static gpointer
lyrics_job_run (gpointer data)
{
	LyricsJob *job = data;

	job->lyrics = lyrics_job_fetch (job);
	if (job->lyrics != NULL)
		g_file_set_contents (job->cache, job->lyrics, -1, NULL);
	g_idle_add (lyrics_job_done, job);

	return NULL;
}

static void
gimmix_lyrics_plugin_fetch (gboolean refetch)
{
	mpd_Song	*song = NULL;
	LyricsJob	*job;
	GThread		*thread;
	gchar		*cache, *cached = NULL;

	generation++;

	if (mpd_player_get_state (gmo) != MPD_PLAYER_STOP && mpd_playlist_get_playlist_length (gmo))
		song = mpd_playlist_get_current_song (gmo);
	if (song == NULL)
	{
		gimmix_lyrics_set_text ("");
		return;
	}

	gimmix_metadata_set_song_details (song, NULL);

	if (song->artist == NULL || song->title == NULL)
	{
		gimmix_lyrics_set_text (_("Lyrics not found"));
		return;
	}

	cache = gimmix_lyrics_cache_file (song->artist, song->title);
	if (refetch)
		g_unlink (cache);
	if (g_file_get_contents (cache, &cached, NULL, NULL))
	{
		gimmix_lyrics_set_text (cached);
		g_free (cached);
		g_free (cache);
		return;
	}

	gimmix_lyrics_set_text (_("Fetching lyrics..."));

	job = g_new0 (LyricsJob, 1);
	job->generation = generation;
	job->artist = g_strdup (song->artist);
	job->title = g_strdup (song->title);
	job->album = g_strdup (song->album);
	job->duration = song->time > 0 ? (guint) song->time : 0;
	job->cache = cache;
	if (gimmix_config_get_bool ("proxy_enable"))
		job->proxy = gimmix_config_get_proxy_string ();

	thread = g_thread_new ("gimmix-lyrics", lyrics_job_run, job);
	g_thread_unref (thread);
}

void
gimmix_lyrics_plugin_update_lyrics (void)
{
	gimmix_lyrics_plugin_fetch (FALSE);
}

static void
cb_gimmix_lyrics_get_btn_clicked (G_GNUC_UNUSED GtkWidget *widget,
				  G_GNUC_UNUSED gpointer   data)
{
	gimmix_lyrics_plugin_fetch (TRUE);
}

#endif
