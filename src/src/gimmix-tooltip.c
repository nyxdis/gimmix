/*
 * gimmix-tooltip.c
 *
 * Copyright (C) 2006-2008 Priyank Gosalia
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
	gtk_misc_set_padding (GTK_MISC(tooltip->icon), 4, 4);
	gtk_box_pack_start (GTK_BOX(tooltip->hbox), tooltip->icon, TRUE, TRUE, 0);

	/* labels */
	label1 = gtk_label_new (NULL);
	g_object_set (G_OBJECT(label1), "use-markup", TRUE, NULL);
	gtk_box_pack_start (GTK_BOX(tooltip->vbox), label1, TRUE, FALSE, 1);
	gtk_label_set_ellipsize (label1, PANGO_ELLIPSIZE_END);
	gtk_label_set_max_width_chars (label1, 25);
	gtk_misc_set_alignment (GTK_MISC(label1), 0, 0);
	label2 = gtk_label_new (NULL);
	g_object_set (G_OBJECT(label2), "use-markup", TRUE, NULL);
	gtk_box_pack_start (GTK_BOX(tooltip->vbox), label2, TRUE, FALSE, 1);
	gtk_misc_set_alignment (GTK_MISC(label2), 0, 0);
	
	/* And finally, the progress meter */
	tooltip->progressbar = gtk_progress_bar_new ();
	gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR(tooltip->progressbar), GTK_PROGRESS_LEFT_TO_RIGHT);
	gtk_widget_set_size_request (tooltip->progressbar, -1, 16);
	gtk_box_pack_start (GTK_BOX(tooltip->vbox), tooltip->progressbar, TRUE, FALSE, 2);

	return tooltip;
}

void gimmix_tooltip_set_text1 (GimmixTooltip *tooltip, const gchar *text, gboolean formatting)
{
	GList *list;
	gchar *markup;

	if ( (list = gtk_container_get_children (GTK_CONTAINER(tooltip->vbox))) != NULL )
	{
		if (text == NULL)
		{
			gtk_label_set_text (GTK_LABEL(list->data), NULL);
			gtk_widget_hide (GTK_WIDGET(list->data));
			return;
		}
		
		if (formatting == TRUE)
		{
			markup = g_markup_printf_escaped ("<span size=\"large\" weight=\"bold\">%s</span>", text);
			gtk_label_set_markup (GTK_LABEL(list->data), markup);
			g_free (markup);
		}
		else
		{
			gtk_label_set_text (GTK_LABEL(list->data), text);
		}

		g_list_free (list);
	}

	return;
}

void gimmix_tooltip_set_text2 (GimmixTooltip *tooltip, const gchar *text, gboolean formatting)
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
			gtk_widget_hide (GTK_WIDGET(list->data));
			return;
		}
		if (formatting == TRUE)
		{
			markup = g_markup_printf_escaped ("<span size=\"medium\"><i>%s</i></span>", text);
			gtk_label_set_markup (GTK_LABEL(list->data), markup);
			g_free (markup);
		}
		else
		{
			gtk_label_set_text (GTK_LABEL(list->data), text);
		}
		gtk_widget_show (GTK_WIDGET(list->data));
		g_list_free (list);
	}

	return;
}

void gimmix_tooltip_set_icon (GimmixTooltip *tooltip, GdkPixbuf *pixbuf)
{
	gdk_threads_enter ();
	gtk_image_set_from_pixbuf (GTK_IMAGE(tooltip->icon), pixbuf);
	gdk_threads_leave ();

	return;
}
/*
void gimmix_tooltip_attach_to_widget (GimmixTooltip *tooltip, GtkWidget *widget)
{
	if (widget == NULL)
		return;
	
	gint x, y;
	gint wheight, wwidth;
	GtkRequisition req;
	gint height, width;
	GdkScreen *screen;
	
	screen = gdk_screen_get_default ();
	width = gdk_screen_get_width (screen);
	height = gdk_screen_get_height (screen);
	gdk_window_get_geometry (tooltip->window, NULL, NULL, &wwidth, &wheight, NULL);
	printf ("tooltip size: %d x %d\n", wwidth, wheight);
	printf ("screen size: %d x %d\n", width, height);
	
	gdk_window_get_origin (widget->window, &x, &y);
	gtk_window_move (GTK_WINDOW(tooltip->window), x-150, y-75);
}
*/
void gimmix_tooltip_show (GimmixTooltip *tooltip)
{
	if (tooltip != NULL)
	{
		gtk_widget_show (GTK_WIDGET(tooltip->hbox));
		gtk_widget_show (GTK_WIDGET(tooltip->window));
	}
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

