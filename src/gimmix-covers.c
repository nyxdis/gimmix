/*
 * gimmix-covers.c: album cover plugin
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
 * Covers are looked up in this order: the local cover database, image
 * files next to the song in the music directory, and the Cover Art Archive
 * (via a MusicBrainz release search). Network and file work happens in a
 * worker thread; everything that touches GTK or MPD runs in the main loop.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#ifdef HAVE_COVER_PLUGIN

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
#include "gimmix-covers.h"
#include "gimmix-tagedit.h"
#include "gimmix-tooltip.h"
#include "wejpconfig.h"

#define DEFAULT_COVER	"gimmix-album.png"
#define COVERS_DIR	".gimmix/covers"
#define COVERS_DBF	".gimmix/covers/covers.db"

#define MUSICBRAINZ_URL	"https://musicbrainz.org/ws/2/release/?fmt=json&limit=5&query="
#define COVERART_URL	"https://coverartarchive.org/release/"

/* a cover lookup running in a worker thread */
typedef struct {
	guint	generation;
	gchar	*artist;
	gchar	*album;
	gchar	*file;		/* song path relative to the music directory */
	gchar	*music_dir;
	gchar	*proxy;
	gchar	*download;	/* where a fetched cover is stored */
	gchar	*result;	/* path of the cover found, or NULL */
} CoverJob;

extern GtkBuilder	*xml;
extern ConfigFile	conf;
extern MpdObj		*gmo;
extern GimmixTooltip 	*tooltip;
extern GtkWidget	*main_window;

static ConfigFile	cover_db;
static gchar		*cover_image_path = NULL;
static GHashTable	*misses = NULL;		/* albums without a cover, this session */
static guint		generation = 0;		/* bumped on every lookup request */
static gchar		*pending = NULL;	/* key of the album being looked up */
static guint		h3_size = 0;

static GtkWidget	*gimmix_metadata_image;
static GtkWidget	*gimmix_plcbox_image;
static GtkWidget	*gimmix_plcbox_eventbox;
GtkWidget		*gimmix_plcbox_frame;

static void		gimmix_covers_plugin_apply (void);
static gchar*		gimmix_covers_db_key (const gchar *artist, const gchar *album);
static gchar*		gimmix_covers_file_for (const gchar *artist, const gchar *album);
static void		gimmix_covers_db_save (void);
static void		gimmix_covers_remember (const gchar *artist, const gchar *album, const gchar *path);

static void
cb_gimmix_covers_plugin_plcbox_size_allocated (G_GNUC_UNUSED GtkWidget *widget,
					       GtkAllocation	       *a,
					       G_GNUC_UNUSED gpointer   data)
{
	if (!h3_size)
	{
		h3_size = a->height;
	}
}

static void
cb_gimmix_covers_plugin_cover_file_preview (GtkFileChooser *file_chooser, gpointer data)
{
	GtkWidget	*preview = GTK_WIDGET (data);
	char		*filename = NULL;
	GdkPixbuf	*pixbuf = NULL;

	filename = gtk_file_chooser_get_preview_filename (file_chooser);
	if (filename == NULL)
		return;
	pixbuf = gdk_pixbuf_new_from_file_at_size (filename, 128, 128, NULL);
	g_free (filename);
	if (pixbuf != NULL)
	{
		gtk_image_set_from_pixbuf (GTK_IMAGE (preview), pixbuf);
		g_object_unref (pixbuf);
	}
	gtk_file_chooser_set_preview_widget_active (file_chooser, pixbuf != NULL);
}

static void
cb_gimmix_covers_plugin_refetch_cover (void)
{
	mpd_Song	*song = mpd_playlist_get_current_song (gmo);
	gchar		*key, *cached;

	if (song == NULL || song->artist == NULL || song->album == NULL)
		return;

	/* forget what we know about this album and look again */
	key = gimmix_covers_db_key (song->artist, song->album);
	g_hash_table_remove (misses, key);
	cached = cfg_get_key_value (&cover_db, key);
	if (cached != NULL)
	{
		gchar *own = gimmix_covers_file_for (song->artist, song->album);
		if (strcmp (cached, own) == 0)
			g_unlink (own);
		g_free (own);
		cfg_add_key (&cover_db, key, "");
		gimmix_covers_db_save ();
	}
	g_free (key);

	gimmix_covers_plugin_update_cover ();
}

static void
cb_gimmix_covers_plugin_set_cover_from_file (void)
{
	GtkWidget	*dialog;
	GtkFileFilter	*filter = NULL;
	GtkWidget	*preview;
	mpd_Song	*song = NULL;
	gchar		*artist = NULL;
	gchar		*album = NULL;

	if (!(song = mpd_playlist_get_current_song (gmo)))
		return;
	if (song->artist == NULL || song->album == NULL)
	{
		gimmix_error (_("The current song has no artist or album tag, so the cover cannot be stored."));
		return;
	}
	artist = g_strdup (song->artist);
	album = g_strdup (song->album);

	dialog = gtk_file_chooser_dialog_new (_("Set cover from file"),
					GTK_WINDOW(main_window),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL);
	preview = gtk_image_new ();
	gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER(dialog), preview);
	g_signal_connect (GTK_FILE_CHOOSER(dialog), "update-preview", G_CALLBACK (cb_gimmix_covers_plugin_cover_file_preview), preview);
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Images"));
	gtk_file_filter_add_pixbuf_formats (filter);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(dialog), filter);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar	*filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		gchar	*contents = NULL;
		gsize	len = 0;

		if (filename != NULL && g_file_get_contents (filename, &contents, &len, NULL))
		{
			gchar *target = gimmix_covers_file_for (artist, album);
			if (g_file_set_contents (target, contents, len, NULL))
			{
				gimmix_covers_remember (artist, album, target);
				gimmix_covers_plugin_apply ();
			}
			else
			{
				gimmix_error (_("There was an error while setting the album cover. Please try using a different image."));
			}
			g_free (target);
			g_free (contents);
		}
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
	g_free (artist);
	g_free (album);
}

static gboolean
cb_gimmix_covers_plugin_plc_popup (G_GNUC_UNUSED GtkWidget *widget,
				   GdkEventButton	   *event,
				   G_GNUC_UNUSED gpointer   data)
{
	GtkWidget 	*menu = NULL;
	GtkWidget 	*menu_item = NULL;
	GtkWidget	*image = NULL;

	if (event->button != 3) /* only on right click */
		return FALSE;

	menu = gtk_menu_new ();

	image = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_MENU);
	menu_item = gtk_image_menu_item_new_with_label (_("Set cover from file"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menu_item), image);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_gimmix_covers_plugin_set_cover_from_file), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	image = gtk_image_new_from_stock ("gtk-refresh", GTK_ICON_SIZE_MENU);
	menu_item = gtk_image_menu_item_new_with_label (_("Re-fetch cover"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menu_item), image);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK(cb_gimmix_covers_plugin_refetch_cover), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	gtk_widget_show (menu);
	gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());

	return TRUE;
}

void
gimmix_covers_plugin_init (void)
{
	gchar		*cpath = NULL;
	GtkWidget	*widget = NULL;

	gimmix_http_init ();

	/* ~/.gimmix/covers holds the fetched images and the database */
	cpath = cfg_get_path_to_config_file (COVERS_DIR);
	g_mkdir_with_parents (cpath, 0755);
	g_free (cpath);

	/* downloads that were still running when gimmix quit */
	cpath = cfg_get_path_to_config_file (COVERS_DIR);
	{
		GDir *dir = g_dir_open (cpath, 0, NULL);
		const gchar *name;
		while (dir != NULL && (name = g_dir_read_name (dir)) != NULL)
		{
			if (g_str_has_prefix (name, ".download-"))
			{
				gchar *stale = g_build_filename (cpath, name, NULL);
				g_unlink (stale);
				g_free (stale);
			}
		}
		if (dir != NULL)
			g_dir_close (dir);
	}
	g_free (cpath);

	cfg_init_config_file_struct (&cover_db);
	cpath = cfg_get_path_to_config_file (COVERS_DBF);
	if (g_file_test (cpath, G_FILE_TEST_EXISTS))
		cfg_read_config_file (&cover_db, cpath);
	g_free (cpath);

	misses = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	/* metadata widgets */
	gimmix_plcbox_eventbox = GTK_WIDGET (gtk_builder_get_object (xml, "cover_event_box"));
	gimmix_plcbox_image = GTK_WIDGET (gtk_builder_get_object (xml, "gimmix_plcbox_image"));
	gimmix_metadata_image = GTK_WIDGET (gtk_builder_get_object (xml, "gimmix_metadata_image"));
	gimmix_plcbox_frame = GTK_WIDGET (gtk_builder_get_object (xml, "gimmix_plc_image_frame"));

	g_signal_connect (gimmix_plcbox_eventbox, "button_press_event", G_CALLBACK(cb_gimmix_covers_plugin_plc_popup), NULL);

	/* the album picture placeholder is as high as the song info box next to it */
	widget = GTK_WIDGET (gtk_builder_get_object (xml,"plcvbox"));
	g_signal_connect (widget, "size-allocate", G_CALLBACK(cb_gimmix_covers_plugin_plcbox_size_allocated), NULL);

	if (!gimmix_config_get_bool ("coverart_enable"))
		gtk_widget_hide (gimmix_plcbox_frame);
}

void
gimmix_covers_plugin_cleanup (void)
{
	generation++; /* results of running lookups are dropped */
	if (misses != NULL)
	{
		g_hash_table_destroy (misses);
		misses = NULL;
	}
	cfg_free_config_file_struct (&cover_db);
	g_free (cover_image_path);
	cover_image_path = NULL;
	g_free (pending);
	pending = NULL;
}

/* the database key, compatible with the files written by older versions */
static gchar *
gimmix_covers_db_key (const gchar *artist, const gchar *album)
{
	gchar *key = g_strdup_printf ("%s-%s", artist, album);
	gimmix_strcrep (key, ' ', '_');
	return key;
}

/* where a cover for this album is stored */
static gchar *
gimmix_covers_file_for (const gchar *artist, const gchar *album)
{
	gchar *dir = cfg_get_path_to_config_file (COVERS_DIR);
	gchar *artist_e = gimmix_http_escape (artist);
	gchar *album_e = gimmix_http_escape (album);
	gchar *path = g_strdup_printf ("%s/%s-%s.jpg", dir, artist_e, album_e);

	g_free (dir);
	g_free (artist_e);
	g_free (album_e);
	return path;
}

static void
gimmix_covers_db_save (void)
{
	gchar *rcfile = cfg_get_path_to_config_file (COVERS_DBF);
	cfg_write_config_file (&cover_db, rcfile);
	g_free (rcfile);
}

/* record path as the cover of an album and make it the current cover */
static void
gimmix_covers_remember (const gchar *artist, const gchar *album, const gchar *path)
{
	gchar *key = gimmix_covers_db_key (artist, album);

	cfg_add_key (&cover_db, key, (gchar *) path);
	gimmix_covers_db_save ();
	g_hash_table_remove (misses, key);
	g_free (key);

	g_free (cover_image_path);
	cover_image_path = g_strdup (path);
}

static GdkPixbuf*
gimmix_covers_plugin_get_default_cover (guint width, guint height)
{
	GdkPixbuf	*ret = NULL;
	gchar		*path = gimmix_get_full_image_path (DEFAULT_COVER);

	ret = gdk_pixbuf_new_from_file_at_size (path, width, height, NULL);
	g_free (path);

	return ret;
}

GdkPixbuf*
gimmix_covers_plugin_get_cover_image_of_size (guint width, guint height)
{
	GdkPixbuf	*pixbuf = NULL;

	if (cover_image_path != NULL)
		pixbuf = gdk_pixbuf_new_from_file_at_size (cover_image_path, width, height, NULL);
	if (pixbuf == NULL)
		pixbuf = gimmix_covers_plugin_get_default_cover (width, height);

	return pixbuf;
}

/* show the current cover in the main window, the tag editor and the tooltip */
static void
gimmix_covers_plugin_apply (void)
{
	GdkPixbuf	*pixbuf;
	guint		height = h3_size > 0 ? h3_size : 96;

	pixbuf = gimmix_covers_plugin_get_cover_image_of_size (96, height);
	if (pixbuf != NULL)
	{
		gtk_image_set_from_pixbuf (GTK_IMAGE(gimmix_plcbox_image), pixbuf);
		g_object_unref (pixbuf);
	}

	pixbuf = gimmix_covers_plugin_get_cover_image_of_size (96, 96);
	if (pixbuf != NULL)
	{
		gimmix_tag_editor_set_cover_image (pixbuf);
		g_object_unref (pixbuf);
	}

	if (tooltip != NULL && gimmix_config_get_bool ("enable_notification"))
	{
		pixbuf = gimmix_covers_plugin_get_cover_image_of_size (48, 48);
		if (pixbuf != NULL)
		{
			gimmix_tooltip_set_icon (tooltip, pixbuf);
			g_object_unref (pixbuf);
		}
	}
}

void
gimmix_covers_plugin_set_default_cover (void)
{
	generation++;
	g_free (pending);
	pending = NULL;
	g_free (cover_image_path);
	cover_image_path = NULL;
	gimmix_covers_plugin_apply ();
}

static void
cover_job_free (CoverJob *job)
{
	g_free (job->artist);
	g_free (job->album);
	g_free (job->file);
	g_free (job->music_dir);
	g_free (job->proxy);
	g_free (job->download);
	g_free (job->result);
	g_free (job);
}

/* image files that commonly sit next to the music */
static gchar *
cover_job_find_local (CoverJob *job)
{
	static const gchar *names[] = { "cover", "folder", "front", "album", "Cover", "Folder", "Front", "Album", NULL };
	static const gchar *exts[] = { "jpg", "jpeg", "png", "JPG", "PNG", NULL };
	gchar *dir, *found = NULL;

	if (job->music_dir == NULL || *job->music_dir == '\0' || job->file == NULL)
		return NULL;

	dir = g_path_get_dirname (job->file);
	for (gint n = 0; names[n] != NULL && found == NULL; n++)
	{
		for (gint e = 0; exts[e] != NULL; e++)
		{
			gchar *name = g_strdup_printf ("%s.%s", names[n], exts[e]);
			gchar *path = g_build_filename (job->music_dir, dir, name, NULL);
			g_free (name);
			if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
			{
				found = path;
				break;
			}
			g_free (path);
		}
	}
	g_free (dir);

	return found;
}

/* a Lucene query term: quoted, with quotes and backslashes dropped */
static gchar *
lucene_term (const gchar *field, const gchar *value)
{
	GString *s = g_string_new (field);

	g_string_append (s, ":\"");
	for (; *value != '\0'; value++)
		if (*value != '"' && *value != '\\')
			g_string_append_c (s, *value);
	g_string_append_c (s, '"');

	return g_string_free (s, FALSE);
}

/* ask MusicBrainz for releases of the album, then the Cover Art Archive for
 * their front covers; the first one that exists is kept */
static gchar *
cover_job_fetch (CoverJob *job)
{
	static const gchar *headers[] = { "Accept: application/json", NULL };
	JsonParser	*parser = NULL;
	JsonNode	*root;
	JsonArray	*releases;
	gchar		*artist_q, *album_q, *query, *query_e, *url, *body = NULL;
	gchar		*found = NULL;
	gsize		length = 0;

	artist_q = lucene_term ("artist", job->artist);
	album_q = lucene_term ("release", job->album);
	query = g_strdup_printf ("%s AND %s", album_q, artist_q);
	query_e = gimmix_http_escape (query);
	url = g_strconcat (MUSICBRAINZ_URL, query_e, NULL);
	g_free (artist_q);
	g_free (album_q);
	g_free (query);
	g_free (query_e);

	if (!gimmix_http_get (url, job->proxy, headers, &body, &length))
	{
		g_free (url);
		g_free (body);
		return NULL;
	}
	g_free (url);

	parser = json_parser_new ();
	if (json_parser_load_from_data (parser, body, length, NULL) &&
	    (root = json_parser_get_root (parser)) != NULL &&
	    JSON_NODE_HOLDS_OBJECT (root) &&
	    json_object_has_member (json_node_get_object (root), "releases"))
	{
		releases = json_object_get_array_member (json_node_get_object (root), "releases");
		for (guint i = 0; releases != NULL && i < json_array_get_length (releases); i++)
		{
			JsonObject	*release = json_array_get_object_element (releases, i);
			const gchar	*id;

			if (release == NULL || !json_object_has_member (release, "id"))
				continue;
			id = json_object_get_string_member (release, "id");
			if (id == NULL)
				continue;

			url = g_strconcat (COVERART_URL, id, "/front-500", NULL);
			if (gimmix_http_download (url, job->proxy, job->download))
			{
				found = g_strdup (job->download);
				g_free (url);
				break;
			}
			g_free (url);
		}
	}
	g_object_unref (parser);
	g_free (body);

	return found;
}

static gboolean
cover_job_done (gpointer data)
{
	CoverJob *job = data;

	if (job->generation == generation)
	{
		g_free (pending);
		pending = NULL;
		if (job->result != NULL)
		{
			gchar *final = job->result;

			/* a downloaded image moves to its permanent name */
			if (strcmp (job->result, job->download) == 0)
			{
				final = gimmix_covers_file_for (job->artist, job->album);
				if (g_rename (job->download, final) != 0)
				{
					g_free (final);
					final = job->result;
				}
			}
			gimmix_covers_remember (job->artist, job->album, final);
			if (final != job->result)
				g_free (final);
		}
		else
		{
			g_hash_table_add (misses, gimmix_covers_db_key (job->artist, job->album));
			g_free (cover_image_path);
			cover_image_path = NULL;
		}
		gimmix_covers_plugin_apply ();
	}
	else if (job->result != NULL && strcmp (job->result, job->download) == 0)
	{
		g_unlink (job->download);
	}
	cover_job_free (job);

	return FALSE;
}

static gpointer
cover_job_run (gpointer data)
{
	CoverJob *job = data;

	job->result = cover_job_find_local (job);
	if (job->result == NULL)
		job->result = cover_job_fetch (job);
	g_idle_add (cover_job_done, job);

	return NULL;
}

void
gimmix_covers_plugin_update_cover (void)
{
	mpd_Song	*song;
	CoverJob	*job;
	GThread		*thread;
	gchar		*key, *known, *dir;

	generation++;
	g_free (cover_image_path);
	cover_image_path = NULL;

	if (!gimmix_config_get_bool ("coverart_enable"))
	{
		gimmix_covers_plugin_apply ();
		return;
	}

	song = mpd_playlist_get_current_song (gmo);
	if (song == NULL || song->artist == NULL || song->album == NULL)
	{
		gimmix_covers_plugin_apply ();
		return;
	}

	key = gimmix_covers_db_key (song->artist, song->album);
	known = cfg_get_key_value (&cover_db, key);
	if (known != NULL && *known != '\0' && g_file_test (known, G_FILE_TEST_IS_REGULAR))
	{
		cover_image_path = g_strdup (known);
		g_free (key);
		gimmix_covers_plugin_apply ();
		return;
	}
	if (g_hash_table_contains (misses, key))
	{
		g_free (key);
		gimmix_covers_plugin_apply ();
		return;
	}

	/* the same album is already being looked up: let that finish */
	if (pending != NULL && strcmp (pending, key) == 0)
	{
		generation--;
		g_free (key);
		gimmix_covers_plugin_apply ();
		return;
	}
	g_free (pending);
	pending = key;

	/* show the default cover while the lookup runs */
	gimmix_covers_plugin_apply ();

	job = g_new0 (CoverJob, 1);
	job->generation = generation;
	job->artist = g_strdup (song->artist);
	job->album = g_strdup (song->album);
	job->file = g_strdup (song->file);
	job->music_dir = g_strdup (cfg_get_key_value (&conf, "music_directory"));
	if (gimmix_config_get_bool ("proxy_enable"))
		job->proxy = gimmix_config_get_proxy_string ();
	dir = cfg_get_path_to_config_file (COVERS_DIR);
	job->download = g_strdup_printf ("%s/.download-%u.jpg", dir, generation);
	g_free (dir);

	thread = g_thread_new ("gimmix-cover", cover_job_run, job);
	g_thread_unref (thread);
}

#endif
