/*
 * gimmix-lyrics.h
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

#ifdef HAVE_LYRICS

#ifndef GIMMIX_LYRICS_H
#define GIMMIX_LYRICS_H

#include <gtk/gtk.h>

/* Initialize the gimmix lyrics plugin */
void gimmix_lyrics_plugin_init (void);

/* Free memory used by lyrics plugin */
void gimmix_lyrics_plugin_cleanup (void);

/* Show the lyrics of the current song. Lyrics that are not cached are
 * fetched in the background, the text view is updated from the main loop. */
void gimmix_lyrics_plugin_update_lyrics (void);

#endif

#endif
