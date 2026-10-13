/*
 * gimmix-tooltip.h
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

#ifndef _GIMMIX_TOOLTIP_H
#define _GIMMIX_TOOLTIP_H

#include <gtk/gtk.h>

/* The widget shown as the tray icon's tooltip: cover, title, artist and
 * a progress bar */
typedef struct {
	GtkWidget *hbox;
	GtkWidget *icon;
	GtkWidget *label1;
	GtkWidget *label2;
	GtkWidget *progressbar;
} GimmixTooltip;

/* Create a new tooltip */
GimmixTooltip *gimmix_tooltip_new (void);

/* Sets the tooltip text (label1) */
void gimmix_tooltip_set_text1 (GimmixTooltip *tooltip, const gchar *text, gboolean formatting);

/* Sets the tooltip text (label2) */
void gimmix_tooltip_set_text2 (GimmixTooltip *tooltip, const gchar *text, gboolean formatting);

/* Sets the icon for the tooltip */
void gimmix_tooltip_set_icon (GimmixTooltip *tooltip, GdkPixbuf *pixbuf);

/* Destroy the tooltip object and free the memory */
void gimmix_tooltip_destroy (GimmixTooltip *tooltip);

#endif
