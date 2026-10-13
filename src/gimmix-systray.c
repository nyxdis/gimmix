/*
 * gimmix-systray.c: the tray icon
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

#include <glib.h>
#include "gimmix-core.h"
#include "gimmix-config.h"
#include "gimmix-interface.h"
#include "gimmix-systray.h"
#include "gimmix-tooltip.h"

#define GIMMIX_ICON  		"gimmix_logo_small.png"
#define GIMMIX_TOOLTIP_ICON 	"gimmix.png"

GtkStatusIcon		*icon = NULL;
GimmixTooltip		*tooltip = NULL;

extern MpdObj		*gmo;
extern ConfigFile	conf;
extern GtkWidget	*volume_scale;

/* system tray popup menu callbacks */
static void	cb_systray_popup_play_clicked (GtkMenuItem *menuitem, gpointer data);
static void	cb_systray_popup_stop_clicked (GtkMenuItem *menuitem, gpointer data);
static void	cb_systray_popup_next_clicked (GtkMenuItem *menuitem, gpointer data);
static void	cb_systray_popup_prev_clicked (GtkMenuItem *menuitem, gpointer data);
static void	cb_systray_popup_quit_clicked (GtkMenuItem *menuitem, gpointer data);

static void	cb_systray_activate (GtkStatusIcon *status_icon, gpointer data);
static void	cb_systray_popup_menu (GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer data);
static gboolean	cb_systray_scroll (GtkStatusIcon *status_icon, GdkEventScroll *event, gpointer data);
static gboolean	cb_systray_query_tooltip (GtkStatusIcon *status_icon, gint x, gint y, gboolean keyboard_mode, GtkTooltip *gtk_tooltip, gpointer data);

void
gimmix_create_systray_icon (void)
{
	gchar 		*icon_file;
	GdkPixbuf	*icon_image;
	GdkPixbuf	*icon_tooltip;

	/* create the tray icon */
	icon_file = gimmix_get_full_image_path (GIMMIX_ICON);
	icon_image = gdk_pixbuf_new_from_file (icon_file, NULL);
	g_free (icon_file);
	if (icon_image != NULL)
	{
		icon = gtk_status_icon_new_from_pixbuf (icon_image);
		g_object_unref (icon_image);
	}
	else
	{
		icon = gtk_status_icon_new_from_icon_name ("audio-x-generic");
	}
	gtk_status_icon_set_title (icon, APPNAME);

	/* the notification tooltip */
	tooltip = gimmix_tooltip_new ();
	gimmix_tooltip_set_text1 (tooltip, APPNAME, TRUE);
	icon_file = gimmix_get_full_image_path (GIMMIX_TOOLTIP_ICON);
	icon_tooltip = gdk_pixbuf_new_from_file_at_size (icon_file, 32, 32, NULL);
	g_free (icon_file);
	if (icon_tooltip != NULL)
	{
		gimmix_tooltip_set_icon (tooltip, icon_tooltip);
		g_object_unref (icon_tooltip);
	}

	g_signal_connect (icon, "activate", G_CALLBACK (cb_systray_activate), NULL);
	g_signal_connect (icon, "popup-menu", G_CALLBACK (cb_systray_popup_menu), NULL);
	g_signal_connect (icon, "scroll-event", G_CALLBACK (cb_systray_scroll), NULL);
	g_signal_connect (icon, "query-tooltip", G_CALLBACK (cb_systray_query_tooltip), NULL);
	gtk_status_icon_set_tooltip_text (icon, APPNAME);
	gtk_status_icon_set_has_tooltip (icon, TRUE);
	gtk_status_icon_set_visible (icon, TRUE);
}

/* the tray icon's tooltip is the notification widget, if enabled */
static gboolean
cb_systray_query_tooltip (G_GNUC_UNUSED GtkStatusIcon *status_icon,
			  G_GNUC_UNUSED gint	      x,
			  G_GNUC_UNUSED gint	      y,
			  G_GNUC_UNUSED gboolean      keyboard_mode,
			  GtkTooltip		     *gtk_tooltip,
			  G_GNUC_UNUSED gpointer      data)
{
	if (tooltip == NULL || !gimmix_config_get_bool ("enable_notification"))
		return FALSE;

	gtk_tooltip_set_custom (gtk_tooltip, tooltip->hbox);

	return TRUE;
}

static void
cb_systray_activate (G_GNUC_UNUSED GtkStatusIcon *status_icon,
		     G_GNUC_UNUSED gpointer	  data)
{
	gimmix_window_visible_toggle ();
}

static gboolean
cb_systray_scroll (G_GNUC_UNUSED GtkStatusIcon *status_icon,
		   GdkEventScroll		*event,
		   G_GNUC_UNUSED gpointer	 data)
{
	gint volume;
	GtkAdjustment *volume_adj;

	volume_adj = gtk_range_get_adjustment (GTK_RANGE(volume_scale));
	switch (event->direction)
	{
		case GDK_SCROLL_UP:
			volume = gtk_adjustment_get_value (volume_adj) + 2;
			gtk_adjustment_set_value (volume_adj, volume);
			break;
		case GDK_SCROLL_DOWN:
			volume = gtk_adjustment_get_value (volume_adj) - 2;
			gtk_adjustment_set_value (volume_adj, volume);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

static void
cb_systray_popup_menu (GtkStatusIcon *status_icon,
		       guint	      button,
		       guint	      activate_time,
		       G_GNUC_UNUSED gpointer data)
{
	GtkWidget *menu, *menu_item;

	menu = gtk_menu_new();

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (gimmix_about_show), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	if (gimmix_get_status(gmo) == PLAY)
		menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PAUSE, NULL);
	else
		menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PLAY, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_systray_popup_play_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_STOP, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_systray_popup_stop_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_systray_popup_prev_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_NEXT, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_systray_popup_next_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_systray_popup_quit_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	gtk_widget_show_all (menu);
	gtk_menu_popup (GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, status_icon, button, activate_time);
}

static void
cb_systray_popup_play_clicked (G_GNUC_UNUSED GtkMenuItem *menuitem,
			       G_GNUC_UNUSED gpointer	  data)
{
	gimmix_play (gmo);
}

static void
cb_systray_popup_stop_clicked (G_GNUC_UNUSED GtkMenuItem *menuitem,
			       G_GNUC_UNUSED gpointer	  data)
{
	gimmix_stop (gmo);
}

static void
cb_systray_popup_prev_clicked (G_GNUC_UNUSED GtkMenuItem *menuitem,
			       G_GNUC_UNUSED gpointer	  data)
{
	gimmix_prev (gmo);
}

static void
cb_systray_popup_next_clicked (G_GNUC_UNUSED GtkMenuItem *menuitem,
			       G_GNUC_UNUSED gpointer	  data)
{
	gimmix_next (gmo);
}

static void
cb_systray_popup_quit_clicked (G_GNUC_UNUSED GtkMenuItem *menuitem,
			       G_GNUC_UNUSED gpointer	  data)
{
	gimmix_save_window_pos ();
	if (gimmix_config_get_bool ("stop_on_exit"))
		gimmix_stop (gmo);

	gtk_main_quit ();
}

void
gimmix_disable_systray_icon (void)
{
	if (icon == NULL)
		return;

	gtk_status_icon_set_visible (icon, FALSE);
	cfg_add_key (&conf, "enable_systray", "false");
}

void
gimmix_enable_systray_icon (void)
{
	if (icon == NULL)
		gimmix_create_systray_icon ();
	else
		gtk_status_icon_set_visible (icon, TRUE);
	cfg_add_key (&conf, "enable_systray", "true");
}

void
gimmix_destroy_systray_icon (void)
{
	if (icon == NULL)
		return;

	g_object_unref (icon);
	gimmix_tooltip_destroy (tooltip);

	icon = NULL;
	tooltip = NULL;
}

void
gimmix_update_systray_tooltip (mpd_Song *s)
{
	gchar		*title = NULL;
	gchar		*artist_str = NULL;
	gchar		*plain = NULL;

	if (icon == NULL)
		return;

	if (s == NULL)
	{
		gimmix_tooltip_set_text1 (tooltip, APPNAME, TRUE);
		gimmix_tooltip_set_text2 (tooltip, APPURL, FALSE);
		gtk_status_icon_set_tooltip_text (icon, APPNAME);
		return;
	}

	if (s->title != NULL)
	{
		title = g_strdup (s->title);
	}
	else
	{
		title = g_path_get_basename (s->file);
		gimmix_strip_file_ext (title);
	}
	gimmix_tooltip_set_text1 (tooltip, title, TRUE);

	if (s->artist != NULL)
	{
		artist_str = g_strdup_printf ("%s %s", _("by"), s->artist);
		gimmix_tooltip_set_text2 (tooltip, artist_str, TRUE);
		plain = g_strdup_printf ("%s\n%s", title, artist_str);
		g_free (artist_str);
	}
	else
	{
		gimmix_tooltip_set_text2 (tooltip, NULL, FALSE);
		plain = g_strdup (title);
	}

	/* the plain text is what platforms without custom tooltips show */
	gtk_status_icon_set_tooltip_text (icon, plain);
	g_free (plain);
	g_free (title);
}

void
gimmix_systray_tooltip_set_default_image (void)
{
	GdkPixbuf	*pixbuf = NULL;
	gchar		*icon_file = NULL;

	if (tooltip == NULL)
		return;

	icon_file = gimmix_get_full_image_path (GIMMIX_TOOLTIP_ICON);
	pixbuf = gdk_pixbuf_new_from_file_at_size (icon_file, 32, 32, NULL);
	g_free (icon_file);
	if (pixbuf == NULL)
		return;
	gimmix_tooltip_set_icon (tooltip, pixbuf);
	g_object_unref (pixbuf);
}
