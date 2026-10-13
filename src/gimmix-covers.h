/*
 * gimmix-covers.h
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
 */

#ifdef HAVE_COVER_PLUGIN

#ifndef GIMMIX_COVERS_H
#define GIMMIX_COVERS_H

#include <gtk/gtk.h>

/* Initialize the covers plugin */
void gimmix_covers_plugin_init (void);

/* De-initialize the covers plugin */
void gimmix_covers_plugin_cleanup (void);

/* Look up the cover of the current song and show it. The lookup runs in
 * the background, the widgets are updated from the main loop. */
void gimmix_covers_plugin_update_cover (void);

/* Show the default cover */
void gimmix_covers_plugin_set_default_cover (void);

/* get cover image of specified size (returned pixbuf should be unref'd) */
GdkPixbuf* gimmix_covers_plugin_get_cover_image_of_size (guint width, guint height);

#endif

#endif
