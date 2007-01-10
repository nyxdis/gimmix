/*
 * gimmix-tooltip.c
 *
 * Copyright (C) 2006 Priyank Gosalia
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

#include <gtk/gtk.h>
#include <stdio.h>
#include "gimmix-tooltip.h"

GimmixTooltip *gimmix_tooltip_new (void)
{
	GimmixTooltip	*tooltip;
	GtkWidget		*label1;
	GtkWidget		*label2;
	GtkWidget		*label3;
	GtkWidget		*event_box;
	GdkColor		color;
	
	/* main tooltip window */
	tooltip = g_malloc (sizeof(GimmixTooltip));
	tooltip->window = gtk_window_new (GTK_WINDOW_POPUP);
	
	gtk_window_set_resizable (GTK_WINDOW(tooltip->window), FALSE);
	gtk_window_set_decorated (GTK_WINDOW(tooltip->window), FALSE);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW(tooltip->window), TRUE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW(tooltip->window), TRUE);

	/* the two main layout boxes */
	tooltip->hbox = gtk_hbox_new (FALSE, 4);
	tooltip->vbox = gtk_vbox_new (FALSE, 0);
	
	/* pack the boxes */
	gtk_container_add (GTK_CONTAINER(tooltip->window), tooltip->hbox);
	gtk_box_pack_end (GTK_BOX(tooltip->hbox), tooltip->vbox, FALSE, FALSE, 2);
	
	/* tooltip icon */
	tooltip->icon = gtk_image_new_from_pixbuf (NULL);
	gtk_misc_set_padding (GTK_MISC(tooltip->icon), 5, 4);
	gdk_color_parse ("grey", &color);
	event_box = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER(event_box), tooltip->icon);
	gtk_widget_modify_bg (GTK_WIDGET(event_box), GTK_STATE_NORMAL, &color);
	gtk_box_pack_start (GTK_BOX(tooltip->hbox), event_box, TRUE, TRUE, 0);

	/* labels */
	label1 = gtk_label_new (NULL);
	g_object_set (G_OBJECT(label1), "use-markup", TRUE, NULL);
	gtk_box_pack_start (GTK_BOX(tooltip->vbox), label1, TRUE, FALSE, 1);
	gtk_misc_set_alignment (GTK_MISC(label1), 0, 0);
	label2 = gtk_label_new (NULL);
	g_object_set (G_OBJECT(label2), "use-markup", TRUE, NULL);
	gtk_box_pack_start (GTK_BOX(tooltip->vbox), label2, TRUE, FALSE, 1);
	gtk_misc_set_alignment (GTK_MISC(label2), 0, 0);
	label3 = gtk_label_new (NULL);
	g_object_set (G_OBJECT(label3), "use-markup", TRUE, NULL);
	gtk_box_pack_start (GTK_BOX(tooltip->vbox), label3, TRUE, FALSE, 1);
	gtk_misc_set_alignment (GTK_MISC(label3), 0, 0);

	return tooltip;
}

void gimmix_tooltip_set_text1 (GimmixTooltip *tooltip, const gchar *text)
{
	GList *list;
	gchar *markup;

	if ( (list = gtk_container_get_children (GTK_CONTAINER(tooltip->vbox))) != NULL )
	{
		if (text == NULL)
		{
			gtk_label_set_text (GTK_LABEL(list->data), NULL);
			return;
		}
		markup = g_markup_printf_escaped ("<span size=\"large\" weight=\"bold\">%s</span>", text);
		gtk_label_set_markup (GTK_LABEL(list->data), markup);
		g_free (markup);
		g_list_free (list);
	}

	return;
}

void gimmix_tooltip_set_text2 (GimmixTooltip *tooltip, const gchar *text)
{
	GList *list;
	gchar *markup;

	if ( (list = gtk_container_get_children (GTK_CONTAINER(tooltip->vbox))) != NULL )
	{
		if ((list = g_list_nth (list, 1)) == NULL)
			return;

		if (text == NULL)
		{
			gtk_label_set_text (GTK_LABEL(list->data), NULL);
			return;
		}
		markup = g_markup_printf_escaped ("<span size=\"medium\">%s</span>", text);
		gtk_label_set_markup (GTK_LABEL(list->data), markup);
		g_free (markup);
		g_list_free (list);
	}

	return;
}

void gimmix_tooltip_set_text3 (GimmixTooltip *tooltip, const gchar *text)
{
	GList *list;
	gchar *markup;

	if ( (list = gtk_container_get_children (GTK_CONTAINER(tooltip->vbox))) != NULL )
	{
		if ((list = g_list_nth (list, 2)) == NULL)
			return;

		if (text == NULL)
		{
			gtk_label_set_text (GTK_LABEL(list->data), NULL);
			return;
		}
		markup = g_markup_printf_escaped ("%s", text);
		gtk_label_set_markup (GTK_LABEL(list->data), markup);
		g_free (markup);
		g_list_free (list);
	}

	return;
}

void gimmix_tooltip_set_icon (GimmixTooltip *tooltip, GdkPixbuf *pixbuf)
{
	gtk_image_set_from_pixbuf (GTK_IMAGE(tooltip->icon), pixbuf);

	return;
}

void gimmix_tooltip_attach_to_widget (GimmixTooltip *tooltip, GtkWidget *widget)
{
	if (widget == NULL)
		return;
	
	gint x, y;
	gdk_window_get_origin (widget->window, &x, &y);
	gtk_window_move (GTK_WINDOW(tooltip->window), x-150, y-75);
}

void gimmix_tooltip_show (GimmixTooltip *tooltip)
{
	if (tooltip != NULL)
		gtk_widget_show_all (GTK_WIDGET(tooltip->window));
	
	return;
}

void gimmix_tooltip_hide (GimmixTooltip *tooltip)
{
	if (tooltip != NULL)
		gtk_widget_hide (GTK_WIDGET(tooltip->window));
	
	return;
}

void gimmix_tooltip_destroy (GimmixTooltip *tooltip)
{
	gtk_widget_destroy (GTK_WIDGET(tooltip->vbox));
	gtk_widget_destroy (GTK_WIDGET(tooltip->hbox));
	gtk_widget_destroy (GTK_WIDGET(tooltip->window));
	g_free (tooltip);

	return;
}

