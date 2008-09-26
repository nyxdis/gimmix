/*
 * gimmix-systray.c
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
 
#include "gimmix-core.h"
#include "gimmix-systray.h"
#include "gimmix-tooltip.h"
#include "sexy-tooltip.h"
#include <glib.h>

#ifdef HAVE_COVER_PLUGIN
#	include	"gimmix-covers.h"
#endif

#define GIMMIX_ICON  		"gimmix_logo_small.png"
#define GIMMIX_TOOLTIP_ICON 	"gimmix.png"

EggTrayIcon		*icon = NULL;
GimmixTooltip		*tooltip = NULL;
GtkWidget		*stooltip;
extern GtkWidget 	*progress;

extern MpdObj		*gmo;
extern GladeXML 	*xml;
extern ConfigFile	conf;

extern GtkWidget	*volume_scale;

static void 	gimmix_systray_display_popup_menu (void);

/* system tray popup menu callbacks */
static void		cb_systray_popup_play_clicked (GtkMenuItem *menuitem, gpointer data);
static void		cb_systray_popup_stop_clicked (GtkMenuItem *menuitem, gpointer data);
static void		cb_systray_popup_next_clicked (GtkMenuItem *menuitem, gpointer data);
static void		cb_systray_popup_prev_clicked (GtkMenuItem *menuitem, gpointer data);
static void		cb_systray_popup_quit_clicked (GtkMenuItem *menuitem, gpointer data);
static void		cb_systray_volume_scroll (GtkWidget *widget, GdkEventScroll *event);

static gboolean	cb_gimmix_systray_icon_clicked (GtkWidget *widget, GdkEventButton *event, gpointer data);

static gboolean cb_systray_enter_notify (GtkWidget *widget, GdkEventCrossing *event, gpointer data);

static gboolean cb_systray_leave_notify (GtkWidget *widget, GdkEventCrossing *event, gpointer data);

void
gimmix_create_systray_icon (void)
{
	gchar 		*icon_file;
	GdkPixbuf	*icon_image;
	GdkPixbuf	*icon_tooltip;
	GtkWidget	*systray_icon;
	GdkColor 	color;
	
	/* create the tray icon */
	icon = egg_tray_icon_new (APPNAME);
	icon_file = gimmix_get_full_image_path (GIMMIX_ICON);
	icon_image = gdk_pixbuf_new_from_file_at_size (icon_file, 20, 20, NULL);
	g_free (icon_file);
	systray_icon = gtk_image_new_from_pixbuf (icon_image);
	gtk_container_add (GTK_CONTAINER (icon), systray_icon);
	g_object_unref (icon_image);
	
	stooltip = sexy_tooltip_new ();
	gdk_color_parse ("white", &color);
	gtk_widget_modify_bg (GTK_WIDGET(stooltip), GTK_STATE_NORMAL, &color);
	
	/* set the default tooltip */
	tooltip = gimmix_tooltip_new ();
	gimmix_tooltip_set_text1 (tooltip, APPNAME, TRUE);
	icon_file = gimmix_get_full_image_path (GIMMIX_TOOLTIP_ICON);
	icon_tooltip = gdk_pixbuf_new_from_file_at_size (icon_file, 32, 32, NULL);
	g_free (icon_file);
	gimmix_tooltip_set_icon (tooltip, icon_tooltip);
	g_object_unref (icon_tooltip);
	
	g_signal_connect (icon, "button-press-event", G_CALLBACK (cb_gimmix_systray_icon_clicked), NULL);
	g_signal_connect (icon, "scroll_event", G_CALLBACK(cb_systray_volume_scroll), NULL);
	g_signal_connect (icon, "enter-notify-event", G_CALLBACK(cb_systray_enter_notify), NULL);
	g_signal_connect (icon, "leave-notify-event", G_CALLBACK(cb_systray_leave_notify), NULL);
	gtk_widget_show (GTK_WIDGET(systray_icon));
	gtk_widget_show (GTK_WIDGET(icon));
	
	gtk_widget_ref (tooltip->hbox);
	gtk_container_remove (GTK_CONTAINER(tooltip->window), tooltip->hbox);
	gtk_container_add (GTK_CONTAINER(stooltip), tooltip->hbox);
	gtk_widget_unref (tooltip->hbox);
	
	return;
}

static gboolean
cb_systray_enter_notify (GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	GdkScreen *screen = NULL;
	GdkScreen *def_screen = NULL;
	GdkRectangle rectangle;
	gint x, y;
	gint w, h;
	gint top;
	
	if (strncasecmp(cfg_get_key_value(conf, "enable_notification"), "true", 4) != 0)
	return FALSE;
	
	/* Check where to place our tooltip */
	def_screen = gdk_screen_get_default ();
	w = gdk_screen_get_width (def_screen);
	h = gdk_screen_get_height (def_screen);
	/* Get the location of the system tray icon */
	gdk_window_get_origin ((GTK_WIDGET(icon)->window), &x, &y);
	if (h-y >= 100)
		top = 1; /* tooltip should be placed on top */
	else
		top = 0; /* tooltip should be placed on bottom */
	w = h = 0;
	
	/* Move the tooltip off-screen to calculate the exact co-ordinates */
	rectangle.x = 2500;
	rectangle.y = 2500;
	rectangle.width = 100;
	rectangle.height = 50;
	screen = gtk_widget_get_screen (GTK_WIDGET(icon));
	sexy_tooltip_position_to_rect (SEXY_TOOLTIP(stooltip), &rectangle, screen);
	gtk_widget_show_all (stooltip);
	gtk_window_get_size (GTK_WINDOW(stooltip), &w, &h);
	
	/* Good, now lets move it back to where it should be */
	if (top == 1)
	{	
		rectangle.x = x-(w/4);
		rectangle.y = y-25;
	}
	else
	{
		rectangle.x = x-(w/4);
		rectangle.y = y-120;
	}

	sexy_tooltip_position_to_rect (SEXY_TOOLTIP(stooltip), &rectangle, screen);

	return TRUE;
}

static gboolean
cb_systray_leave_notify (GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	gimmix_tooltip_hide (tooltip);
	gtk_widget_hide (stooltip);
	
	return TRUE;
}

static gboolean
cb_gimmix_systray_icon_clicked (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	switch (event->button)
	{
		case 1: gimmix_window_visible_toggle ();
				break;
		case 3: gimmix_systray_display_popup_menu ();
				break;
		default: break;
	}
	
	return TRUE;
}

static void
cb_systray_volume_scroll (GtkWidget *widget, GdkEventScroll *event)
{
	gint volume;
	GtkAdjustment *volume_adj;
	if (event->type != GDK_SCROLL)
		return;
	
	volume_adj = gtk_range_get_adjustment (GTK_RANGE(volume_scale));
	switch (event->direction)
	{
		case GDK_SCROLL_UP:
			volume = gtk_adjustment_get_value (GTK_ADJUSTMENT(volume_adj)) + 2;
			gtk_adjustment_set_value (GTK_ADJUSTMENT (volume_adj), volume);
			break;
		case GDK_SCROLL_DOWN:
			volume = gtk_adjustment_get_value (GTK_ADJUSTMENT(volume_adj)) - 2;
			gtk_adjustment_set_value (GTK_ADJUSTMENT(volume_adj), volume);
			break;
		default:
			return;
	}
	
	return;
}

static void
gimmix_systray_display_popup_menu (void)
{
	GtkWidget *menu, *menu_item;

	menu = gtk_menu_new();

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (gimmix_about_show), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	if (gimmix_get_status(gmo) == PLAY)
	{
		menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PAUSE, NULL);
	}
	else
	{
		menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PLAY, NULL);
	}
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_systray_popup_play_clicked), NULL);
	
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_STOP, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_systray_popup_stop_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_systray_popup_prev_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_NEXT, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_systray_popup_next_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_systray_popup_quit_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	gtk_widget_show (menu);
	gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
	
	return;
}


static void
cb_systray_popup_play_clicked (GtkMenuItem *menuitem, gpointer data)
{
	gimmix_play (gmo);
	
	return;
}

static void
cb_systray_popup_stop_clicked (GtkMenuItem *menuitem, gpointer data)
{
	gimmix_stop (gmo);
	
	return;
}

static void
cb_systray_popup_prev_clicked (GtkMenuItem *menuitem, gpointer data)
{
	gimmix_prev (gmo);

	return;
}

static void
cb_systray_popup_next_clicked (GtkMenuItem *menuitem, gpointer data)
{
	gimmix_next (gmo);

	return;
}

static void
cb_systray_popup_quit_clicked (GtkMenuItem *menuitem, gpointer data)
{
	gimmix_save_window_pos ();
	if (strncasecmp(cfg_get_key_value(conf, "stop_on_exit"), "true", 4) == 0)
		gimmix_stop (gmo);
	
	gtk_main_quit ();
	
	return;
}

void
gimmix_disable_systray_icon (void)
{
	if (icon == NULL)
		return;

	gtk_widget_hide (GTK_WIDGET(icon));
	cfg_add_key (&conf, "enable_systray", "false");
	
	return;
}

void
gimmix_enable_systray_icon (void)
{
	if (icon == NULL)
	{	
		gimmix_create_systray_icon ();
		cfg_add_key (&conf, "enable_systray", "true");
	}
	else
	{
		gtk_widget_show (GTK_WIDGET(icon));
	}
	return;
}

void
gimmix_destroy_systray_icon (void)
{
	if (icon == NULL)
		return;
	gtk_widget_destroy (GTK_WIDGET(icon));
	gimmix_tooltip_destroy (tooltip);
	
	icon = NULL;
	tooltip = NULL;
	
	return;
}	

#ifdef HAVE_COVER_PLUGIN
static void
gimmix_systray_update_tooltip_image (void)
{
	GdkPixbuf	*pixbuf = NULL;
	
	pixbuf = gimmix_covers_plugin_get_cover_image_of_size (48, 48);
	gimmix_tooltip_set_icon (tooltip, pixbuf);
	
	return;
}
#endif

void
gimmix_update_systray_tooltip (mpd_Song *s)
{
	gchar		*text = NULL;
	gchar		*artist_str = NULL;
	
	if (icon == NULL)
		return;
		
	if (s == NULL)
	{
		gimmix_tooltip_set_text1 (tooltip, APPNAME, TRUE);
		gimmix_tooltip_set_text2 (tooltip, APPURL, FALSE);
		return;
	}
	
	if (s->title != NULL)
	{
		gimmix_tooltip_set_text1 (tooltip, s->title, TRUE);
	}
	else
	{
		text = g_path_get_basename (s->file);
		gimmix_strip_file_ext (text);
		gimmix_tooltip_set_text1 (tooltip, text, TRUE);
		g_free (text);
	}
	
	if (s->artist != NULL)
	{	
		artist_str = g_strdup_printf ("%s %s", _("by"), s->artist);
		gimmix_tooltip_set_text2 (tooltip, artist_str, TRUE);
		g_free (artist_str);
	}
	else
		gimmix_tooltip_set_text2 (tooltip, NULL, FALSE);

	return;
}

void
gimmix_systray_tooltip_set_default_image (void)
{
	GdkPixbuf	*pixbuf = NULL;
	gchar		*icon_file = NULL;
	
	icon_file = gimmix_get_full_image_path (GIMMIX_TOOLTIP_ICON);
	pixbuf = gdk_pixbuf_new_from_file_at_size (icon_file, 32, 32, NULL);
	g_free (icon_file);
	gimmix_tooltip_set_icon (tooltip, pixbuf);
	g_object_unref (pixbuf);
	
	return;
}

