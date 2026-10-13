/*
 * gimmix-http.c: small libcurl wrapper shared by the cover and lyrics plugins
 *
 * Copyright (C) 2026 Freydis Mende
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
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <curl/curl.h>

#include "gimmix-http.h"

#define USER_AGENT	"gimmix/" VERSION " ( https://github.com/nyxdis/gimmix )"

static gboolean initialized = FALSE;

void
gimmix_http_init (void)
{
	if (!initialized)
	{
		curl_global_init (CURL_GLOBAL_DEFAULT);
		initialized = TRUE;
	}
}

void
gimmix_http_cleanup (void)
{
	if (initialized)
	{
		curl_global_cleanup ();
		initialized = FALSE;
	}
}

static size_t
write_to_string (void *ptr, size_t size, size_t nmemb, void *data)
{
	g_string_append_len ((GString *) data, ptr, size * nmemb);
	return size * nmemb;
}

static size_t
write_to_file (void *ptr, size_t size, size_t nmemb, void *data)
{
	return fwrite (ptr, size, nmemb, (FILE *) data);
}

static CURL *
request_new (const gchar *url, const gchar *proxy, struct curl_slist **list,
	     const gchar *const *headers)
{
	CURL *curl = curl_easy_init ();

	if (curl == NULL)
		return NULL;

	*list = NULL;
	for (; headers != NULL && *headers != NULL; headers++)
		*list = curl_slist_append (*list, *headers);

	curl_easy_setopt (curl, CURLOPT_URL, url);
	curl_easy_setopt (curl, CURLOPT_USERAGENT, USER_AGENT);
	curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt (curl, CURLOPT_MAXREDIRS, 5L);
	curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt (curl, CURLOPT_TIMEOUT, 30L);
	curl_easy_setopt (curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt (curl, CURLOPT_ACCEPT_ENCODING, "");
	if (*list != NULL)
		curl_easy_setopt (curl, CURLOPT_HTTPHEADER, *list);
	if (proxy != NULL && *proxy != '\0')
		curl_easy_setopt (curl, CURLOPT_PROXY, proxy);

	return curl;
}

static gboolean
request_perform (CURL *curl)
{
	long code = 0;
	CURLcode res;

	res = curl_easy_perform (curl);
	if (res != CURLE_OK)
	{
		g_debug ("http: %s", curl_easy_strerror (res));
		return FALSE;
	}
	curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &code);
	g_debug ("http: %ld", code);

	return code >= 200 && code < 300;
}

gboolean
gimmix_http_get (const gchar *url, const gchar *proxy,
		 const gchar *const *headers, gchar **body, gsize *length)
{
	struct curl_slist	*list = NULL;
	GString			*str = NULL;
	CURL			*curl = NULL;
	gboolean		ret;

	g_return_val_if_fail (url != NULL && body != NULL, FALSE);

	g_debug ("http: GET %s", url);
	curl = request_new (url, proxy, &list, headers);
	if (curl == NULL)
		return FALSE;

	str = g_string_new ("");
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, write_to_string);
	curl_easy_setopt (curl, CURLOPT_WRITEDATA, str);
	ret = request_perform (curl);
	curl_slist_free_all (list);
	curl_easy_cleanup (curl);

	if (length != NULL)
		*length = str->len;
	*body = g_string_free (str, FALSE);

	return ret;
}

gboolean
gimmix_http_download (const gchar *url, const gchar *proxy, const gchar *path)
{
	struct curl_slist	*list = NULL;
	CURL			*curl = NULL;
	FILE			*file = NULL;
	gboolean		ret;

	g_return_val_if_fail (url != NULL && path != NULL, FALSE);

	file = g_fopen (path, "wb");
	if (file == NULL)
		return FALSE;

	g_debug ("http: GET %s -> %s", url, path);
	curl = request_new (url, proxy, &list, NULL);
	if (curl == NULL)
	{
		fclose (file);
		g_unlink (path);
		return FALSE;
	}

	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, write_to_file);
	curl_easy_setopt (curl, CURLOPT_WRITEDATA, file);
	ret = request_perform (curl);
	curl_slist_free_all (list);
	curl_easy_cleanup (curl);

	if (ret)
		ret = ftell (file) > 0;
	fclose (file);
	if (!ret)
		g_unlink (path);

	return ret;
}

gchar *
gimmix_http_escape (const gchar *string)
{
	return g_uri_escape_string (string != NULL ? string : "", NULL, FALSE);
}
