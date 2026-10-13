/*
 * gimmix-http.h: small libcurl wrapper shared by the cover and lyrics plugins
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

#ifndef GIMMIX_HTTP_H
#define GIMMIX_HTTP_H

#include <glib.h>

/* Initialize libcurl, call once from the main thread before any request */
void gimmix_http_init (void);

/* Release libcurl */
void gimmix_http_cleanup (void);

/* Fetch url into a newly allocated, NUL terminated buffer.
 * headers is a NULL terminated list of extra "Name: value" headers or NULL.
 * Returns TRUE on a 2xx response. All functions here may be called from any thread. */
gboolean gimmix_http_get (const gchar *url, const gchar *proxy,
			  const gchar *const *headers, gchar **body, gsize *length);

/* Fetch url into the file at path. Returns TRUE on a 2xx response with a
 * non-empty body, otherwise the file is removed. */
gboolean gimmix_http_download (const gchar *url, const gchar *proxy, const gchar *path);

/* Percent encode a query string value (returned string should be g_free'd) */
gchar *gimmix_http_escape (const gchar *string);

#endif
