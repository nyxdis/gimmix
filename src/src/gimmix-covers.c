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
#include "gimmix-covers.h"

#define COVERS_DIR	".gimmix/covers"
#define COVERS_DBF	".gimmix/covers/covers.db"
#define RESULT_XML	"cvr.xml"
#define AMAZON_KEY	"14TBPBEBTPCVM7BY0C02"
#define AMAZON_URL	"http://ecs.amazonaws.com/onca/xml?Service=AWSECommerceService&Operation=ItemSearch&SearchIndex=Music&ResponseGroup=Images,EditorialReview&AWSAccessKeyId=%s&%s=%s&%s=%s"

static gboolean gimmix_covers_plugin_download (const char *url, const char *file);

static void
gimmix_covers_plugin_init (void)
{
	gchar	*cpath = NULL;

	/* check if .gimmix/lyrics exists */
	cpath = cfg_get_path_to_config_file (COVERS_DIR);
	g_mkdir_with_parents (cpath, 00755);
	
	return;
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
	
	curl = curl_easy_init ();
	if (curl)
	{
		FILE *outfile = NULL;
		char *path = NULL;

		path = g_strdup_printf ("%s/%s", cfg_get_path_to_config_file(COVERS_DIR), file);
		outfile = fopen (path, "w");
		curl_easy_setopt (curl, CURLOPT_URL, url);
		curl_easy_setopt (curl, CURLOPT_WRITEDATA, outfile);
		curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, __curl_write_func);
		curl_easy_setopt (curl, CURLOPT_READFUNCTION, __curl_read_func);
		res = curl_easy_perform (curl);
		if (!res != 0)
		{
			ret = TRUE;
		}
		fclose (outfile);
		curl_easy_cleanup (curl);
		g_free (path);
	}
	
	return ret;
}


