/*
 * gimmix-lyrics.c
 *
 * Copyright (C) 2008 Priyank Gosalia
 *
 * Gimmix is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * Gimmix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with Gimmix; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Priyank Gosalia <priyankmg@gmail.com>
 */
 
#ifdef HAVE_LYRICS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <nxml.h>
#include <glib/gstdio.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <libxml/xmlreader.h>
#include "gimmix-lyrics.h"

#define LYRICS_DIR	".gimmix/lyrics/"
#define SEARCH_URL	"http://api.leoslyrics.com/api_search.php?auth=Gimmix"
#define LYRICS_URL	"http://api.leoslyrics.com/api_lyrics.php?auth=Gimmix&hid="
#define SEARCH		1
#define FETCHL		0

extern GladeXML		*xml;
extern MpdObj		*gmo;
extern ConfigFile	conf;

static GtkWidget	*lyrics_textview = NULL;

static gchar*		search_artist = NULL;
static gchar*		search_title = NULL;

static gchar *lyrics_url_encode (const char *string);
static gboolean lyrics_process_lyrics_node (LYRICS_NODE *ptr);

static void cb_gimmix_lyrics_get_btn_clicked (GtkWidget *widget, gpointer data);

void
gimmix_lyrics_plugin_init (void)
{
	char		*cpath = NULL;
	
	lyrics_textview = glade_xml_get_widget (xml, "lyrics_textview");
	
	g_signal_connect (G_OBJECT(glade_xml_get_widget(xml,"lyrics_get_btn")),
				"clicked",
				G_CALLBACK(cb_gimmix_lyrics_get_btn_clicked),
				NULL);

	/* check if .gimmix/lyrics exists */
	cpath = cfg_get_path_to_config_file (LYRICS_DIR);
	g_mkdir_with_parents (cpath, 00755);
	g_free (cpath);

	return;
}

static void
gimmix_lyrics_plugin_proxy_init (nxml_t *n)
{
	char	*proxy = NULL;
	
	if (!strncasecmp(cfg_get_key_value(conf,"proxy_enable"),"true",4))
	{
		proxy = gimmix_config_get_proxy_string ();
		g_print ("proxyis : %s\n", proxy);
		nxml_set_proxy (n, proxy, NULL);
		g_free (proxy);
	}
	else
	{
		g_print ("prox is disabled\n");
	}
	
	return;
}

/* URL encodes a string */
static gchar *
lyrics_url_encode (const char *string)
{
	CURL	*curl = NULL;
	gchar	*ret = NULL;
	
	curl = curl_easy_init ();
	ret = curl_easy_escape (curl, string, 0);
	curl_easy_cleanup (curl);
	
	return ret;
}

static void
gimmix_covers_plugin_set_artist (const char *artist)
{
	if (search_artist != NULL)
		g_free (search_artist);
	search_artist = g_strdup (artist);
	
	return;
}

static void
gimmix_covers_plugin_set_songtitle (const char *title)
{
	if (search_title != NULL)
		g_free (search_title);
	search_title = g_strdup (title);
	
	return;
}

static gboolean
lyrics_process_lyrics_node (LYRICS_NODE *ptr)
{
	LYRICS_NODE	*node = ptr;
	char		*url = NULL;
	char		*str = NULL;
	gboolean	ret = FALSE;
	nxml_t		*nxml = NULL;
	nxml_data_t	*nroot = NULL;
	nxml_data_t	*ndata = NULL;
	nxml_data_t	*nndata = NULL;
	nxml_error_t	e;

	if (node == NULL)
		return ret;
	
	url = g_strdup_printf ("%s%s", LYRICS_URL, node->hid);
	//printf ("%s\n", url);
	
	e = nxml_new (&nxml);
	nxml_set_timeout (nxml, 20);
	gimmix_lyrics_plugin_proxy_init (nxml);
	nxml_parse_url (nxml, url);
	nxml_root_element (nxml, &nroot);
	nxml_find_element (nxml, nroot, "lyric", &ndata);
	nxml_find_element (nxml, ndata, "text", &nndata);
	nxml_get_string (nndata, &str);
	if (str)
	{
		node->lyrics = g_strdup_printf (str);
		g_free (str);
		ret = TRUE;
	}
	nxml_free (nxml);
	g_free (url);
	
	return ret;
}

static LYRICS_NODE*
lyrics_perform_search (const char *url)
{
	char		*str = NULL;
	nxml_t		*nxml = NULL;
	nxml_data_t	*nroot = NULL;
	nxml_data_t	*child = NULL;
	nxml_error_t	e;
	LYRICS_NODE	*lnode = NULL;
	
	e = nxml_new (&nxml);
	nxml_set_timeout (nxml, 20);
	gimmix_lyrics_plugin_proxy_init (nxml);
	nxml_parse_url (nxml, (char*)url);
	nxml_root_element (nxml, &nroot);
	nxml_find_element (nxml, nroot, "response", &child);
	nxml_get_string (child, &str);
	
	if (str!=NULL)
	{
		lnode = (LYRICS_NODE*) malloc (sizeof(LYRICS_NODE));
		memset (lnode, 0, sizeof(LYRICS_NODE));
		if (!strcmp(str,"SUCCESS"))
		{
			nxml_data_t *temp = NULL;
			nxml_attr_t *attr = NULL;
			nxml_find_element (nxml, nroot, "searchResults", &child);
			nxml_find_element (nxml, child, "result", &temp);
			nxml_find_attribute (temp, "exactMatch", &attr);
			if (attr)
			{
				if (!strcmp(attr->value,"true"))
					lnode->match = TRUE;
			}
			nxml_find_attribute (temp, "hid", &attr);
			if (attr)
			{
				strncpy (lnode->hid, attr->value, strlen(attr->value));
			}
			nxml_find_element (nxml, child, "title", &temp);
			if (temp)
			{
				strncpy (lnode->title, temp->value, strlen(temp->value));
			}
			nxml_find_element (nxml, child, "artist", &temp);
			if (temp)
			{
				nxml_data_t *ar = NULL;
				nxml_find_element (nxml, temp, "name", &ar);
				if (ar)
				strncpy (lnode->title, ar->value, strlen(ar->value));
			}
			/* if match not found */
			if (!lnode->match)
			{
				/* compare artist */
				if (!g_ascii_strcasecmp(lnode->artist, search_artist) &&
					!g_ascii_strcasecmp(lnode->title, search_title))
				{
					lnode->match = TRUE;
				}
				else
				if (!g_ascii_strcasecmp(lnode->artist, search_artist))
				{
					/* try to match a part of song */
					if (!g_ascii_strncasecmp(lnode->title, search_title, 5))
					{
						lnode->match = TRUE;
					}
				}
			}
			if (lnode->match)
			{
				lyrics_process_lyrics_node (lnode);
			}
			else
			{
				g_free (lnode);
				lnode = NULL;
			}
		}
		g_free (str);
	}
	nxml_free (nxml);
	
	return lnode;
}

LYRICS_NODE*
lyrics_search (void)
{
	gchar		*url = NULL;
	char		*path = NULL;
	LYRICS_NODE	*ret = NULL;

	if (search_artist != NULL && search_title != NULL)
	{
		/* first check if the lyrics exist in ~/.lyrics/ */
		path = g_strdup_printf ("%s/%s-%s.txt", cfg_get_path_to_config_file(LYRICS_DIR), search_artist, search_title);
		if (g_file_test(path,G_FILE_TEST_EXISTS))
		{
			GString	*str = g_string_new ("");
			FILE *fp = NULL;
			char line[PATH_MAX+1] = "";
			ret = (LYRICS_NODE*) malloc(sizeof(LYRICS_NODE));
			memset (ret, 0, sizeof(LYRICS_NODE));
			strncpy (ret->artist, search_artist, strlen(search_artist));
			strncpy (ret->title, search_title, strlen(search_title));
			fp = fopen (path, "r");
			if (fp != NULL)
			{
				while (fgets(line, PATH_MAX, fp))
				{
					str = g_string_append (str, line);
				}
				ret->lyrics = g_strdup (str->str);
				g_string_free (str, TRUE);
				fclose (fp);
			}
			g_free (path);
			return ret;
		}
		char *artist_e = lyrics_url_encode (search_artist);
		char *title_e = lyrics_url_encode (search_title);
		url = g_strdup_printf ("%s&artist=%s&songtitle=%s", SEARCH_URL, artist_e, title_e);
		g_free (artist_e);
		g_free (title_e);
		//g_print ("%s\n", url);
		ret = lyrics_perform_search (url);
		g_free (url);
		if (ret)
		{
			//g_print ("everything ok\n");
			if (ret->lyrics != NULL)
			{
				FILE *fp = fopen (path, "w");
				fprintf (fp, "%s", ret->lyrics);
				fclose (fp);
			}
		}
	}
	
	return ret;
}

void
gimmix_lyrics_populate_textview (LYRICS_NODE *node)
{
	GtkTextBuffer	*buffer;
	GtkTextIter	iter;
	
	/* clear the textview */
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(lyrics_textview));
	gtk_text_buffer_set_text (buffer, "", 0);
	gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
	
	/* display the lyrics */
	if (node && node->lyrics)
	{
		gtk_text_buffer_insert (buffer, &iter, node->lyrics, -1);
	}
	else
	{
		gtk_text_buffer_insert (buffer, &iter, _("Lyrics not found"), -1);
	}

	return;
}

void
gimmix_lyrics_plugin_update_lyrics (void)
{
	LYRICS_NODE	*node = NULL;
	mpd_Song	*s = NULL;
	
	sleep (2);
	if (mpd_player_get_state(gmo)!=MPD_PLAYER_STOP)
	{
		if (mpd_playlist_get_playlist_length(gmo))
			s = mpd_playlist_get_current_song (gmo);
		else
			s = NULL;
	}
	
	if (s)
	{
		if (s->artist)
			gimmix_covers_plugin_set_artist (s->artist);
		if (s->title)
			gimmix_covers_plugin_set_songtitle (s->title);
		#ifndef HAVE_COVER_PLUGIN
		gimmix_metadata_set_song_details (s, NULL);
		#else
		if (!strncasecmp(cfg_get_key_value(conf,"coverart_enable"),"false",5))
		{
			gimmix_metadata_set_song_details (s, NULL);
		}
		#endif
	}

	node = lyrics_search ();
	gimmix_lyrics_populate_textview (node);
	if (node)
	{
		if (node->lyrics)
		g_free (node->lyrics);
		g_free (node);
	}
	
	return;
}

static void
cb_gimmix_lyrics_get_btn_clicked (GtkWidget *widget, gpointer data)
{
	g_thread_create ((GThreadFunc)gimmix_lyrics_plugin_update_lyrics ,
				NULL,
				FALSE,
				NULL);

	return;
}

#endif
