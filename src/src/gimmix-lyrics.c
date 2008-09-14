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
#include <glib/gstdio.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <libxml/xmlreader.h>
#include "gimmix-lyrics.h"

#define LYRICS_DIR	".gimmix/lyrics/"
#define TEMP_XML	"lyt.xml"
#define LYRC_XML	"lyc.xml"
#define SEARCH_URL	"http://api.leoslyrics.com/api_search.php?auth=Gimmix"
#define LYRICS_URL	"http://api.leoslyrics.com/api_lyrics.php?auth=Gimmix&hid="
#define SEARCH		1
#define FETCHL		0

extern GladeXML *xml;

static GtkWidget *lyrics_textview = NULL;

typedef enum _lyrics_status
{
	LYRICS_STATUS_ERR_CURL = 0,
	LYRICS_STATUS_ERR_UNK,
	LYRICS_STATUS_OK
} LYRICS_STATUS;


static GtkWidget	*lyrics_song_label = NULL;
static GtkWidget	*lyrics_artist_label = NULL;
static GtkWidget	*lyrics_song_box = NULL;
static GtkWidget	*lyrics_artist_box = NULL;

static gboolean fetched = FALSE;
static gchar*	search_artist = NULL;
static gchar*	search_title = NULL;
static LYRICS_NODE* lyrics_node = NULL;

static gchar *lyrics_url_encode (const char *string);
static LYRICS_STATUS lyrics_perform_curl (const char *url, gint action);
static void lyrics_process_fetch_result (xmlTextReaderPtr *reader, LYRICS_NODE *lnode);
static gboolean lyrics_process_lyrics_node (LYRICS_NODE *ptr);
static gboolean lyrics_process_search_result (xmlTextReaderPtr *reader);

static void cb_gimmix_lyrics_get_btn_clicked (GtkWidget *widget, gpointer data);

void
gimmix_lyrics_plugin_init (void)
{
	char		*cpath = NULL;
	
	lyrics_textview = glade_xml_get_widget (xml, "lyrics_textview");
	lyrics_song_label = glade_xml_get_widget (xml, "lyrics_song_label");
	lyrics_artist_label = glade_xml_get_widget (xml, "lyrics_artist_label");
	lyrics_artist_box = glade_xml_get_widget (xml, "lyrics_artistbox");
	lyrics_song_box = glade_xml_get_widget (xml, "lyrics_songbox");
	
	g_signal_connect (G_OBJECT(glade_xml_get_widget(xml,"lyrics_get_btn")),
				"clicked",
				G_CALLBACK(cb_gimmix_lyrics_get_btn_clicked),
				NULL);

	/* check if .gimmix/lyrics exists */
	cpath = cfg_get_path_to_config_file (LYRICS_DIR);
	g_mkdir_with_parents (cpath, 00755);

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

LYRICS_NODE*
lyrics_get_lyrics (void)
{
	return lyrics_node;
}

void
lyrics_set_artist (const char *artist)
{
	if (search_artist != NULL)
		g_free (search_artist);
	search_artist = g_strdup (artist);
	
	return;
}

void
lyrics_set_songtitle (const char *title)
{
	if (search_title != NULL)
		g_free (search_title);
	search_title = g_strdup (title);
	
	return;
}

static size_t
lyrics_xml_write_func (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return fwrite (ptr, size, nmemb, stream);
}

static size_t
lyrics_xml_read_func (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return fread (ptr, size, nmemb, stream);
}

static LYRICS_STATUS
lyrics_perform_curl (const char *url, gint action)
{
	CURL		*curl;
	CURLcode	res;
	FILE		*outfile;
	char		*path = NULL;
	char		*file = NULL;

	curl = curl_easy_init ();
	if (curl)
	{
		if (action == SEARCH)
			file = g_strdup (TEMP_XML);
		else
			file = g_strdup (LYRC_XML);
		path = g_strdup_printf ("%s/%s", cfg_get_path_to_config_file(LYRICS_DIR), file);
		g_free (file);
		if (g_file_test(path,G_FILE_TEST_EXISTS))
			g_remove (path);
		outfile = fopen (path, "w");

		curl_easy_setopt (curl, CURLOPT_URL, url);
		curl_easy_setopt (curl, CURLOPT_WRITEDATA, outfile);
		curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, lyrics_xml_write_func);
		curl_easy_setopt (curl, CURLOPT_READFUNCTION, lyrics_xml_read_func);

		res = curl_easy_perform (curl);
		if (res != 0)
		{
			fetched = FALSE;
		}
		else
		{
			fetched = TRUE;
		}
		fclose (outfile);
		curl_easy_cleanup (curl);
		g_free (path);

		if (fetched)
			return LYRICS_STATUS_OK;
		else
			return LYRICS_STATUS_ERR_UNK;
	}

	return LYRICS_STATUS_ERR_CURL;
}

static void
lyrics_process_fetch_result (xmlTextReaderPtr *reader, LYRICS_NODE *lnode)
{
	const		xmlChar *name, *value;
	char		*temp = NULL;
	int		i;

    	printf ("i entered processNode() for fetch\n");
	name = xmlTextReaderConstName ((*reader));

	if (name == NULL)
	{
		name = BAD_CAST "--";
	}
	else
	{
		do {
			value = xmlTextReaderConstName ((*reader));
			/* see if we have the writer name available for the lyric */
			if (!strcmp((char*)value,"writer"))
			{
				xmlTextReaderRead ((*reader));
				value = xmlTextReaderConstValue ((*reader));
				if (value != NULL)
				{
					strcpy (lnode->writer, (char*)value);
				}
				continue;
			}
			if (!strcmp((char*)value,"text"))
			{
				break;
			}
		} while (xmlTextReaderRead(*reader));
		xmlTextReaderRead ((*reader));
		value = xmlTextReaderConstValue ((*reader));
		if (value == NULL)
			return;
		else
		{
			temp = g_strdup ((gchar*)value);
   		 	g_strstrip (temp);
			if (strlen(temp)>0)
			{
				printf ("RESULT: %s\n", temp);
				lnode->lyrics = g_strdup (temp);
				g_free (temp);
			}
		}
		
	}
	for (i=0;i<5;i++) xmlTextReaderRead ((*reader));

	printf ("=================================\n");
	
}

static void
lyrics_parse_fetch_result_xml (const char *filename, LYRICS_NODE *ptr)
{
	xmlTextReaderPtr reader;
	int ret;
	gchar *path = NULL;
	int i;
	char *value = NULL;
	char *name = NULL;
	char *temp = NULL;

	/* Initialize the XML library */
	LIBXML_TEST_VERSION
	path = g_strdup_printf ("%s/%s", cfg_get_path_to_config_file(LYRICS_DIR), filename);
	reader = xmlReaderForFile (path, NULL, 0);
	g_free (path);
	if (reader != NULL)
	{
        	ret = xmlTextReaderRead (reader);
		printf ("NAME=%s\n", name);
		/* Process response code */
		for (i=0;i<3;i++) xmlTextReaderRead ((reader));
		/* read response code */
		value = (char*)xmlTextReaderConstValue((reader));

		if (value == NULL)
			return;
		else
		{
			temp = g_strdup (value);
   		 	g_strstrip (temp);
			if (strlen(temp)>0)
			{
				printf ("FOOBAR: %s\n", temp);
				if (!strcmp(temp,"NOT_FOUND"))
				{
					ptr->lyrics = NULL;
					g_free (temp);
					return;
				}
				g_free (temp);
			}
		}
		while (ret == 1)
		{
			lyrics_process_fetch_result (&reader, ptr);
			ret = xmlTextReaderRead (reader);
		}
		xmlFreeTextReader (reader);
		/*
		if (ret != 0)
		{
			fprintf (stderr, "%s : failed to parse\n", filename);
			return;
		}
		*/
	}
	else
	{
		fprintf (stderr, "Unable to open %s\n", path);
		return;
	}
}

static gboolean
lyrics_process_lyrics_node (LYRICS_NODE *ptr)
{
	LYRICS_NODE *node = ptr;
	char *url = NULL;
	gint status = -1;
	
	if (node == NULL)
		return FALSE;
	
	url = g_strdup_printf ("%s%s", LYRICS_URL, node->hid);
	printf ("%s\n", url);
	status = lyrics_perform_curl (url, FETCHL);
	g_free (url);
	if (status == LYRICS_STATUS_OK)
	{
		g_print ("fetching ok\n");
		lyrics_parse_fetch_result_xml (LYRC_XML, node);
		if (node->lyrics!=NULL)
			return TRUE;
		else
			return FALSE;
	}
	
	return FALSE;
}

static gboolean
lyrics_process_search_result (xmlTextReaderPtr *reader)
{
	const		xmlChar *name, *value;
	char		*temp = NULL;
	xmlChar		*hid = NULL;
	char		*match = NULL;
	int		i;
	LYRICS_NODE	*lnode;

    	//printf ("i entered processNode()\n");
	name = xmlTextReaderConstName ((*reader));

	if (name == NULL)
	{
		name = BAD_CAST "--";
	}
	else
	{
		lnode = (LYRICS_NODE*) malloc (sizeof(LYRICS_NODE));
		memset (lnode, 0, sizeof(LYRICS_NODE));

		for (i=0;i<4;i++) xmlTextReaderRead ((*reader)); /* <searchResults> */
		xmlTextReaderRead ((*reader));	/* <result> */
		hid = xmlTextReaderGetAttribute ((*reader), (xmlChar*)"hid");
		if (hid != NULL)
		{
			strncpy (lnode->hid, (char*)hid, strlen((char*)hid));
			//printf ("HID: %s\n", lnode->hid);
		}
		match = (char*)xmlTextReaderGetAttribute ((*reader), (xmlChar*)"exactMatch");
		if (match != NULL)
		{
			if (!strcmp(match,"true"))
			{
				lnode->match = TRUE;
			}
			else
			{
				lnode->match = FALSE;
			}
			//printf ("MATCH: %s\n", match);
		}

		for (i=0;i<3;i++) xmlTextReaderRead ((*reader)); /* <title> */
		value = xmlTextReaderConstValue((*reader));
		if (value != NULL)
		{
			temp = g_strdup ((char*)value);
   		 	g_strstrip (temp);
			if (strlen(temp)>0)
			{
				strncpy (lnode->title, temp, strlen(temp));
				//printf ("TITLE: %s\n", lnode->title);
				g_free (temp);
			}
		}
		
		for (i=0;i<8;i++) xmlTextReaderRead ((*reader)); /* <title> */
		value = xmlTextReaderConstValue((*reader));
		if (value != NULL)
		{
			temp = g_strdup ((char*)value);
   		 	g_strstrip (temp);
			if (strlen(temp)>0)
			{
				strncpy (lnode->artist, temp, strlen(temp));
				//printf ("ARTIST: %s\n", lnode->artist);
				g_free (temp);
			}
		}
		if (lnode->match)
		{
			lyrics_process_lyrics_node (lnode);
			lyrics_node = lnode;
			return FALSE;
		}
		else
		{
			/* compare artist */
			if (!g_ascii_strcasecmp(lnode->artist, search_artist) &&
				!g_ascii_strcasecmp(lnode->title, search_title))
			{
				lyrics_process_lyrics_node (lnode);
				lyrics_node = lnode;
				return FALSE;
			}
			else
			if (!g_ascii_strcasecmp(lnode->artist, search_artist))
			{
				/* try to match a part of song */
				if (!g_ascii_strncasecmp(lnode->title, search_title, 5))
				{
					lyrics_process_lyrics_node (lnode);
					lyrics_node = lnode;
					//printf ("yes it did \n");
					return FALSE;
				}
			}
		}
		xmlTextReaderRead ((*reader)); /* padding */
		free (lnode);
	}
	
	printf ("=================================\n");

	return TRUE;
}

static gboolean
lyrics_parse_search_result_xml (const char *filename)
{
	xmlTextReaderPtr reader;
	int ret;
	gchar *path = NULL;
	int i;
	char *value = NULL;
	char *name = NULL;
	char *temp = NULL;

	/* Initialize the XML library */
	LIBXML_TEST_VERSION
	path = g_strdup_printf ("%s/%s", cfg_get_path_to_config_file(LYRICS_DIR), filename);
	reader = xmlReaderForFile (path, NULL, 0);
	if (reader != NULL)
	{
        	ret = xmlTextReaderRead (reader);
		//printf ("NAME=%s\n", name);
		/* Process response code */
		for (i=0;i<3;i++) xmlTextReaderRead ((reader));
		/* read response code */
		value = (char*)xmlTextReaderConstValue((reader));

		if (value == NULL)
			return FALSE;
		else
		{
			temp = g_strdup (value);
   		 	g_strstrip (temp);
			if (strlen(temp)>0)
			{
				//printf ("RESPONSE CODE: %s\n", temp);
				g_free (temp);
			}
		}
		while (ret && lyrics_process_search_result(&reader))
			ret = xmlTextReaderRead (reader);
		
		xmlFreeTextReader (reader);
		/*
		if (ret != 0)
		{
			fprintf (stderr, "%s : failed to parse\n", filename);
			return;
		}
		*/
		if (!ret)
			return FALSE;
	}
	else
	{
		fprintf (stderr, "Unable to open %s\n", path);
		return FALSE;
	}
	
	return TRUE;
}

gboolean
lyrics_search (void)
{
	gchar		*url = NULL;
	gint		state = -1;
	gboolean	result = FALSE;
	char		*path = NULL;

	if (search_artist != NULL && search_title != NULL)
	{
		/* first check if the lyrics exist in ~/.lyrics/ */
		path = g_strdup_printf ("%s/%s-%s.txt", cfg_get_path_to_config_file(LYRICS_DIR), search_artist, search_title);
		if (g_file_test(path,G_FILE_TEST_EXISTS))
		{
			GString	*str = g_string_new ("");
			FILE *fp = NULL;
			char line[PATH_MAX+1] = "";
			lyrics_node = (LYRICS_NODE*) malloc(sizeof(LYRICS_NODE));
			memset (lyrics_node, 0, sizeof(LYRICS_NODE));
			strncpy (lyrics_node->artist, search_artist, strlen(search_artist));
			strncpy (lyrics_node->title, search_title, strlen(search_title));
			fp = fopen (path, "r");
			if (fp != NULL)
			{
				while (fgets(line, PATH_MAX, fp))
				{
					str = g_string_append (str, line);
				}
				lyrics_node->lyrics = g_strdup (str->str);
				g_string_free (str, TRUE);
				fclose (fp);
			}
			g_free (path);
			return TRUE;
		}
		char *artist_e = lyrics_url_encode (search_artist);
		char *title_e = lyrics_url_encode (search_title);
		url = g_strdup_printf ("%s&artist=%s&songtitle=%s", SEARCH_URL, artist_e, title_e);
		g_free (artist_e);
		g_free (title_e);
		g_print ("%s\n", url);
		state = lyrics_perform_curl (url, SEARCH);
		g_free (url);
		if (state == LYRICS_STATUS_OK)
		{
			//g_print ("everything ok\n");
			result = lyrics_parse_search_result_xml (TEMP_XML);
			if (result)
			{
				/* save the lyrics to a file in ~/.gimmix/lyrics */
				if (lyrics_node->lyrics != NULL)
				{
					FILE *fp = fopen (path, "w");
					fprintf (fp, "%s", lyrics_node->lyrics);
					fclose (fp);
				}
				return TRUE;
			}
		}
		return FALSE;
	}
	
	return FALSE;
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
		gtk_widget_show (lyrics_song_box);
		gtk_widget_show (lyrics_artist_box);
		gtk_label_set_text (GTK_LABEL(lyrics_song_label), node->title);
		gtk_label_set_text (GTK_LABEL(lyrics_artist_label), node->artist);
		gtk_text_buffer_insert (buffer, &iter, node->lyrics, -1);
	}
	else
	{
		gtk_widget_hide (lyrics_song_box);
		gtk_widget_hide (lyrics_artist_box);
		gtk_text_buffer_insert (buffer, &iter, _("Lyrics not found"), -1);
	}

	return;
}

static void
cb_gimmix_lyrics_get_btn_clicked (GtkWidget *widget, gpointer data)
{
	gimmix_update_lyrics ();

	return;
}

#endif
