/*
 * gimmix-tooltip.c
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

#include <gtk/gtk.h>
#include "gimmix-tooltip.h"

GimmixTooltip *
gimmix_tooltip_new (void)
{
	GimmixTooltip	*tooltip;
	GtkWidget	*vbox;

	tooltip = g_malloc0 (sizeof(GimmixTooltip));

	/* the box is handed to GtkTooltip whenever the tooltip is shown and
	 * taken back when it hides, so it is kept alive here */
	tooltip->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
	g_object_ref_sink (tooltip->hbox);
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_end (GTK_BOX(tooltip->hbox), vbox, FALSE, FALSE, 2);

	/* tooltip icon */
	tooltip->icon = gtk_image_new_from_pixbuf (NULL);
	g_object_set (tooltip->icon, "margin", 4, NULL);
	gtk_box_pack_start (GTK_BOX(tooltip->hbox), tooltip->icon, TRUE, TRUE, 0);

	/* labels */
	tooltip->label1 = gtk_label_new (NULL);
	gtk_label_set_use_markup (GTK_LABEL(tooltip->label1), TRUE);
	gtk_label_set_ellipsize (GTK_LABEL(tooltip->label1), PANGO_ELLIPSIZE_END);
	gtk_label_set_max_width_chars (GTK_LABEL(tooltip->label1), 25);
	gtk_widget_set_halign (tooltip->label1, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX(vbox), tooltip->label1, TRUE, FALSE, 1);

	tooltip->label2 = gtk_label_new (NULL);
	gtk_label_set_use_markup (GTK_LABEL(tooltip->label2), TRUE);
	gtk_label_set_ellipsize (GTK_LABEL(tooltip->label2), PANGO_ELLIPSIZE_END);
	gtk_label_set_max_width_chars (GTK_LABEL(tooltip->label2), 25);
	gtk_widget_set_halign (tooltip->label2, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX(vbox), tooltip->label2, TRUE, FALSE, 1);

	/* And finally, the progress meter */
	tooltip->progressbar = gtk_progress_bar_new ();
	gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR(tooltip->progressbar), TRUE);
	gtk_widget_set_size_request (tooltip->progressbar, -1, 16);
	gtk_box_pack_start (GTK_BOX(vbox), tooltip->progressbar, TRUE, FALSE, 2);

	gtk_widget_show_all (tooltip->hbox);

	return tooltip;
}

static void
gimmix_tooltip_set_label (GtkWidget *label, const gchar *text, const gchar *format)
{
	if (text == NULL)
	{
		gtk_label_set_text (GTK_LABEL(label), "");
		gtk_widget_hide (label);
		return;
	}

	if (format != NULL)
	{
		gchar *markup = g_markup_printf_escaped (format, text);
		gtk_label_set_markup (GTK_LABEL(label), markup);
		g_free (markup);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(label), text);
	}
	gtk_widget_show (label);
}

void
gimmix_tooltip_set_text1 (GimmixTooltip *tooltip, const gchar *text, gboolean formatting)
{
	gimmix_tooltip_set_label (tooltip->label1, text,
				  formatting ? "<span size=\"large\" weight=\"bold\">%s</span>" : NULL);
}

void
gimmix_tooltip_set_text2 (GimmixTooltip *tooltip, const gchar *text, gboolean formatting)
{
	gimmix_tooltip_set_label (tooltip->label2, text,
				  formatting ? "<span size=\"medium\"><i>%s</i></span>" : NULL);
}

void
gimmix_tooltip_set_icon (GimmixTooltip *tooltip, GdkPixbuf *pixbuf)
{
	gtk_image_set_from_pixbuf (GTK_IMAGE(tooltip->icon), pixbuf);
}

void
gimmix_tooltip_destroy (GimmixTooltip *tooltip)
{
	if (tooltip == NULL)
		return;

	gtk_widget_destroy (tooltip->hbox);
	g_object_unref (tooltip->hbox);
	g_free (tooltip);
}
