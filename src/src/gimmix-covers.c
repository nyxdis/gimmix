/*
 * gimmix-covers.c
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
#include <glib/gstdio.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <libxml/xmlreader.h>
#include "wejpconfig.h"
#include "gimmix-covers.h"

#define COVERS_DIR	".gimmix/covers"
#define COVERS_DBF	".gimmix/covers/covers.db"
#define RESULT_XML	"cvr.xml"
#define AMAZON_KEY	"14TBPBEBTPCVM7BY0C02"
#define AMAZON_URL	"http://ecs.amazonaws.com/onca/xml?Service=AWSECommerceService&Operation=ItemSearch&SearchIndex=Music&ResponseGroup=Images,EditorialReview&AWSAccessKeyId=%s&%s=%s&%s=%s"

static ConfigFile cover_db;

static gboolean gimmix_covers_plugin_download (const char *url, const char *file);
static CoverNode* gimmix_cover_node_new (void);
static gchar *gimmix_url_encode (const char *string);
static void gimmix_covers_plugin_cover_db_init (void);
static void gimmix_covers_plugin_cover_db_save (void);

void
gimmix_covers_plugin_init (void)
{
	gchar	*cpath = NULL;

	/* check if .gimmix/covers exists */
	cpath = cfg_get_path_to_config_file (COVERS_DIR);
	g_mkdir_with_parents (cpath, 00755);
	g_free (cpath);
	
	/* check if .gimmix/covers/covers.db exists */
	cpath = cfg_get_path_to_config_file (COVERS_DBF);
	if (!g_file_test(cpath,G_FILE_TEST_EXISTS))
	{
		FILE *fp = fopen (cpath, "w");
		fclose (fp);
	}
	
	gimmix_covers_plugin_cover_db_init ();
	
	return;
}

static void
gimmix_covers_plugin_cover_db_init (void)
{
	char *rcfile = NULL;
	
	cfg_init_config_file_struct (&cover_db);
	rcfile = cfg_get_path_to_config_file (COVERS_DBF);
	if (cfg_read_config_file(&cover_db,rcfile))
	{
		g_error ("cover db init failed\n");
	}
	g_free (rcfile);
	
	return;
}

static void
gimmix_covers_plugin_cover_db_save (void)
{
	char *rcfile = NULL;

	rcfile = cfg_get_path_to_config_file (COVERS_DBF);
	cfg_write_config_file (&cover_db, rcfile);
	cfg_free_config_file_struct (&cover_db);
	gimmix_covers_plugin_cover_db_init ();
	g_free (rcfile);
	
	return;
}

static CoverNode*
gimmix_cover_node_new (void)
{
	CoverNode *ret = NULL;
	
	ret = (CoverNode*) malloc (sizeof(CoverNode));
	if (ret)
	{
		memset (ret, 0, sizeof(CoverNode));
	}

	return ret;
}

/* URL encodes a string */
static gchar *
gimmix_url_encode (const char *string)
{
	CURL	*curl = NULL;
	gchar	*ret = NULL;
	
	curl = curl_easy_init ();
	ret = curl_easy_escape (curl, string, 0);
	curl_easy_cleanup (curl);
	
	return ret;
}

static size_t
__curl_write_func (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return fwrite (ptr, size, nmemb, stream);
}

static size_t
__curl_read_func (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return fread (ptr, size, nmemb, stream);
}

static gboolean
gimmix_covers_plugin_download (const char *url, const char *file)
{
	CURL		*curl;
	CURLcode	res;
	gboolean	ret = FALSE;
	
	if (url!=NULL && file!=NULL)
	{
		curl = curl_easy_init ();
		if (curl)
		{
			FILE *outfile = NULL;
			char *path = NULL;

			//path = g_strdup_printf ("%s/%s", cfg_get_path_to_config_file(COVERS_DIR), file);
			//g_print (path);
			outfile = fopen (file, "w");
			curl_easy_setopt (curl, CURLOPT_URL, url);
			curl_easy_setopt (curl, CURLOPT_WRITEDATA, outfile);
			curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, __curl_write_func);
			curl_easy_setopt (curl, CURLOPT_READFUNCTION, __curl_read_func);
			res = curl_easy_perform (curl);
			if (!res)
			{
				ret = TRUE;
			}
			fclose (outfile);
			curl_easy_cleanup (curl);
			g_free (path);
		}
	}
	
	return ret;
}

/* Copyright Qball */
static xmlNodePtr get_first_node_by_name(xmlNodePtr xml, gchar *name) {
	if(xml) {
		xmlNodePtr c = xml->xmlChildrenNode;
		for(;c;c=c->next) {
			if(xmlStrEqual(c->name, (xmlChar *) name))
				return c;
		}
	}
	return NULL;
}
/* end of snippet */

CoverNode*
gimmix_covers_plugin_get_metadata (char *arg1, char *arg1d, char *arg2, char *arg2d)
{
	char		*url = NULL;
	char		*rxml = NULL;
	CoverNode	*node = NULL;
	char		*u_artist = NULL;
	char		*u_title = NULL;
	
	u_artist = gimmix_url_encode (arg1d);
	u_title = gimmix_url_encode (arg2d);
	url = g_strdup_printf (AMAZON_URL, AMAZON_KEY, arg1, u_artist, arg2, u_title);
	g_print ("%s\n", url);
	rxml = g_strdup_printf ("%s/%s", cfg_get_path_to_config_file(COVERS_DIR), RESULT_XML);
	if (gimmix_covers_plugin_download(url,rxml))
	{
		xmlDocPtr dptr = xmlParseFile (rxml);
		if (dptr!=NULL)
		{
			xmlNodePtr rnode = xmlDocGetRootElement (dptr);
			xmlNodePtr cnode = get_first_node_by_name(rnode, "Items");
			if (cnode!=NULL)
			{
				cnode = get_first_node_by_name(cnode, "Item");
				if (cnode!=NULL)
				{
					xmlNodePtr child = NULL;
					node = gimmix_cover_node_new ();
					if((child = get_first_node_by_name(cnode, "LargeImage")))
					{
						xmlChar *temp = xmlNodeGetContent(get_first_node_by_name(child, "URL"));
						node->img_large = g_strdup((char *)temp);
						g_print ("b: %s\n", temp);
						xmlFree(temp);
					}
					if ((child = get_first_node_by_name(cnode, "MediumImage")))
					{
						xmlChar *temp = xmlNodeGetContent(get_first_node_by_name(child, "URL"));
						node->img_medium = g_strdup((char *)temp);
						g_print ("m: %s\n", temp);
						xmlFree(temp);
					}
					if ((child = get_first_node_by_name(cnode, "SmallImage")))
					{
						xmlChar *temp = xmlNodeGetContent(get_first_node_by_name(child, "URL"));
						node->img_small = g_strdup((char *)temp);
						g_print ("s: %s\n", temp);
						xmlFree(temp);
					}	

					if((child = get_first_node_by_name(cnode, "EditorialReviews")))
					{
						child = get_first_node_by_name (child, "EditorialReview");
						if(child)
						{
							xmlChar *temp = xmlNodeGetContent(get_first_node_by_name(child, "Content"));
							node->album_info = g_strdup((char *)temp);
							xmlFree(temp);
						}
					}
				}
			}
			xmlFreeDoc (dptr);
		}
	}

	g_free (url);
	g_free (rxml);

	return node;
}

static void
gimmix_cover_plugin_save_cover (char *artist, char *album)
{
	char	*artist_e = NULL;
	char	*album_e = NULL;
	char	*old_path = NULL;
	char	*new_path = NULL;
	char	*key = NULL;
	
	artist_e = gimmix_url_encode (artist);
	album_e = gimmix_url_encode (album);
	old_path = g_strdup_printf ("%s/temp.jpg", cfg_get_path_to_config_file(COVERS_DIR));
	new_path = g_strdup_printf ("%s/%s-%s.jpg", cfg_get_path_to_config_file(COVERS_DIR), artist_e, album_e);
	g_rename (old_path, new_path);
	
	/* okay, add an entry to covers.db */
	key = g_strdup_printf ("%s-%s", artist, album);
	cfg_add_key (&cover_db, key, new_path);
	gimmix_covers_plugin_cover_db_save ();
	
	g_free (artist_e);
	g_free (album_e);
	g_free (old_path);
	g_free (new_path);
	g_free (key);

	return;
}

void
gimmix_covers_plugin_set_cover (SongInfo *s)
{
	CoverNode	*node = NULL;
	char		*temp = NULL;
	
	if (s != NULL)
	{
		char *result = NULL;
		
		/* first look into the local cover database */
		temp = g_strdup_printf ("%s-%s", s->artist, s->album);
		result = cfg_get_key_value (cover_db, temp);
		if (result!=NULL)
		{
			g_print ("found: %s\n", result);
		}
		else
		{	temp = g_strdup_printf ("%s/temp.jpg", cfg_get_path_to_config_file(COVERS_DIR));
			node = gimmix_covers_plugin_get_metadata ("Artist", s->artist, "Title", s->album);
			if (node!=NULL)
			{
				g_print ("%s\n%s\n%s\n",
					node->img_large,
					node->img_medium,
					node->img_small);
			
				if (gimmix_covers_plugin_download(node->img_large,temp) ||
					gimmix_covers_plugin_download(node->img_medium,temp) ||
					gimmix_covers_plugin_download(node->img_small,temp))
				{
					gimmix_cover_plugin_save_cover (s->artist, s->album);
				}
			
				g_free (node);
			}
			else
			{
				node = gimmix_covers_plugin_get_metadata ("Performer", s->performer, "Title", s->album);
			}
		}
	}
	
	return;
}
