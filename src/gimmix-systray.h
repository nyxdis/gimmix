/*
 * gimmix-systray.h
 *
 * Copyright (C) 2006-2009 Priyank Gosalia
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

#ifndef _GIMMIX_SYSTRAY_H
#define _GIMMIX_SYSTRAY_H

#include <gtk/gtk.h>
#include "gimmix.h"
#include "gimmix-tooltip.h"

/* create system tray icon */
void gimmix_create_systray_icon (void);

/* Disable system tray icon */
void gimmix_disable_systray_icon (void);

/* Enable system tray icon */
void gimmix_enable_systray_icon (void);

/* update notification tooltip */
void gimmix_update_systray_tooltip (mpd_Song *s);

/* destroy system tray icon */
void gimmix_destroy_systray_icon (void);

/* set default image of systray tooltip */
void gimmix_systray_tooltip_set_default_image (void);

#endif
