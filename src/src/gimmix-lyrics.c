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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <libxml/xmlreader.h>

#define TEMP_XML	"/home/priyank/ly.xml"
#define LYRC_XML	"/home/priyank/lyc.xml"
#define SEARCH_URL	"http://api.leoslyrics.com/api_search.php?auth=Gimmix"
#define LYRICS_URL	"http://api.leoslyrics.com/api_lyrics.php?auth=Gimmix&hid="
#define SEARCH		1
#define FETCHL		0

typedef enum _lyrics_status
{
	LYRICS_STATUS_ERR_CURL = 0,
	LYRICS_STATUS_ERR_UNK,
	LYRICS_STATUS_OK
} LYRICS_STATUS;

typedef struct _lnode
{
	char artist[80];
	char title[80];
	char hid[16];
	char writer[80];
	gboolean match;
	char *lyrics;
} LYRICS_NODE;

static gboolean fetched = FALSE;

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
	char		*dir = NULL;

	curl = curl_easy_init ();
	if (curl)
	{
		if (action == SEARCH)
			path = g_strdup (TEMP_XML);
		else
			path = g_strdup (LYRC_XML);
//		dir = g_build_path (G_DIR_SEPARATOR_S, g_get_home_dir(), NEWS_ITEM_DIR, NULL);
//		g_mkdir_with_parents (dir, 0755);
//		g_free (dir);
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
		for (i=0;i<36;i++) xmlTextReaderRead ((*reader));
		value = xmlTextReaderConstValue((*reader));
		
		if (value == NULL)
			return;
		else
		{
			temp = g_strdup (value);
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
	reader = xmlReaderForFile (filename, NULL, 0);
	if (reader != NULL)
	{
        	ret = xmlTextReaderRead (reader);
		printf ("NAME=%s\n", name);
		/* Process response code */
		for (i=0;i<3;i++) xmlTextReaderRead ((reader));
		/* read response code */
		value = xmlTextReaderConstValue((reader));

		if (value == NULL)
			return;
		else
		{
			temp = g_strdup (value);
   		 	g_strstrip (temp);
			if (strlen(temp)>0)
			{
				printf ("FOOBAR: %s\n", temp);
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
		fprintf (stderr, "Unable to open %s\n", filename);
		return;
	}
}

static gboolean
lyrics_process_lyrics_node (LYRICS_NODE *ptr)
{
	LYRICS_NODE *node = ptr;
	
	if (node == NULL)
		return FALSE;
	
	if (node->match)
	{
		char *url = NULL;
		gint status = -1;
		
		url = g_strdup_printf ("%s%s", LYRICS_URL, node->hid);
		printf ("%s\n", url);
		status = lyrics_perform_curl (url, FETCHL);
		if (status == LYRICS_STATUS_OK)
		{
			g_print ("fetching ok\n");
			lyrics_parse_fetch_result_xml (LYRC_XML, node);
			return TRUE;
		}
	}
}

static void
lyrics_process_search_result (xmlTextReaderPtr *reader)
{
	const		xmlChar *name, *value;
	char		*temp = NULL;
	char		*hid = NULL;
	char		*match = NULL;
	static int	found = 0;
	int		i;
	LYRICS_NODE	*lnode;

    	printf ("i entered processNode()\n");
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
		hid = xmlTextReaderGetAttribute ((*reader), "hid");
		if (hid != NULL)
		{
			strncpy (lnode->hid, hid, strlen(hid));
			printf ("HID: %s\n", lnode->hid);
		}
		match = xmlTextReaderGetAttribute ((*reader), "exactMatch");
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
			printf ("MATCH: %s\n", match);
		}

		for (i=0;i<3;i++) xmlTextReaderRead ((*reader)); /* <title> */
		value = xmlTextReaderConstValue((*reader));
		if (value != NULL)
		{
			temp = g_strdup (value);
   		 	g_strstrip (temp);
			if (strlen(temp)>0)
			{
				strncpy (lnode->title, temp, strlen(temp));
				printf ("TITLE: %s\n", lnode->title);
				g_free (temp);
			}
		}
		
		for (i=0;i<8;i++) xmlTextReaderRead ((*reader)); /* <title> */
		value = xmlTextReaderConstValue((*reader));
		if (value != NULL)
		{
			temp = g_strdup (value);
   		 	g_strstrip (temp);
			if (strlen(temp)>0)
			{
				strncpy (lnode->artist, temp, strlen(temp));
				printf ("ARTIST: %s\n", lnode->artist);
				g_free (temp);
			}
		}
		if (lnode->match)
		{
			lyrics_process_lyrics_node (lnode);
		}
		xmlTextReaderRead ((*reader)); /* padding */
		free (lnode);
	}
	
	printf ("=================================\n");

	return;
}

static void
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
	reader = xmlReaderForFile (filename, NULL, 0);
	if (reader != NULL)
	{
        	ret = xmlTextReaderRead (reader);
		printf ("NAME=%s\n", name);
		/* Process response code */
		for (i=0;i<3;i++) xmlTextReaderRead ((reader));
		/* read response code */
		value = xmlTextReaderConstValue((reader));

		if (value == NULL)
			return;
		else
		{
			temp = g_strdup (value);
   		 	g_strstrip (temp);
			if (strlen(temp)>0)
			{
				printf ("RESPONSE CODE: %s\n", temp);
				g_free (temp);
			}
		}
		while (ret == 1)
		{
			lyrics_process_search_result (&reader);
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
		fprintf (stderr, "Unable to open %s\n", filename);
		return;
	}
}

static LYRICS_NODE *
lyrics_search (const char *artist, const char *title)
{
	gchar	*url = NULL;
	gint	state = -1;

	if (artist != NULL && title != NULL)
	{
		url = g_strdup_printf ("%s&artist=%s&songtitle=%s", SEARCH_URL, artist, title);
		state = lyrics_perform_curl (url, SEARCH);
		if (state == LYRICS_STATUS_OK)
		{
			g_print ("everything ok\n");
			lyrics_parse_search_result_xml (TEMP_XML);
			return TRUE;
		}
	}
}

/*
int
main (int argc, char *argv[])
{
	lyrics_search ("Pink+Floyd", "Coming+Back+To+Life");

	return 0;
}
*/
