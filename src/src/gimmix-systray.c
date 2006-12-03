/*
 * gimmix-systray.c
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
 
#include "gimmix-core.h"
#include "gimmix-systray.h"

#define GIMMIX_ICON  	"/share/pixmaps/gimmix_logo_small.png"

GtkStatusIcon	*icon = NULL;

static void 	cb_gimmix_systray_icon_popup (GtkStatusIcon *sicon, guint button, guint time, gpointer data);
static void		cb_gimmix_systray_icon_activate (GtkStatusIcon *sicon, gpointer data);
static void 	gimmix_systray_display_popup_menu (void);
static void		cb_systray_popup_play_clicked (GtkMenuItem *menuitem, gpointer data);

void
gimmix_create_systray_icon (void)
{
	gchar 		*icon_file;
	GtkWidget	*icon_image;
	
	icon_file = g_strdup_printf ("%s%s", PREFIX, GIMMIX_ICON);
	
	/* create the tray icon */
	icon = gtk_status_icon_new_from_file (icon_file);
	g_free (icon_file);
	gtk_status_icon_set_tooltip (icon, APPNAME);
	
	g_signal_connect (G_OBJECT(icon), "popup-menu", G_CALLBACK (cb_gimmix_systray_icon_popup), NULL);
	g_signal_connect (G_OBJECT(icon), "activate", G_CALLBACK(cb_gimmix_systray_icon_activate), NULL);

	return;
}

static void
cb_gimmix_systray_icon_activate (GtkStatusIcon *sicon, gpointer data)
{
	gimmix_window_visible_toggle ();
	return;
}

static void
cb_gimmix_systray_icon_popup (GtkStatusIcon *sicon, guint button, guint time, gpointer data)
{
	gimmix_systray_display_popup_menu ();
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
	
	if (gimmix_get_status(pub->gmo) == PLAY)
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
	//g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_stop_button_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS, NULL);
	//g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_prev_button_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_NEXT, NULL);
//	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_next_button_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (gtk_main_quit), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	gtk_widget_show (menu);
	gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
	
	return;
}


static void
cb_systray_popup_play_clicked (GtkMenuItem *menuitem, gpointer data)
{
	GtkWidget	*image = NULL;
	
	if (gimmix_play(pub->gmo))
	{
		image = get_image ("gtk-media-pause", GTK_ICON_SIZE_BUTTON);
		gimmix_set_song_info ();
	}
	else
	{
		image = get_image ("gtk-media-play", GTK_ICON_SIZE_BUTTON);
	}
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menuitem), image);
			
	return;
}

void
gimmix_disable_systray_icon (void)
{
	if (icon == NULL)
		return;
	
	gtk_status_icon_set_visible (icon, FALSE);
	pub->conf->systray_enable = 0;
	
	return;
}

void
gimmix_enable_systray_icon (void)
{
	if (icon == NULL)
	{	
		gimmix_create_systray_icon ();
		pub->conf->systray_enable = 1;
	}
	else
	{
		gtk_status_icon_set_visible (icon, TRUE);
	}
	return;
}

void
gimmix_destroy_systray_icon (void)
{
	if (icon == NULL)
		return;
	g_object_unref (icon);
	icon = NULL;
	
	return;
}	


void
gimmix_update_systray_tooltip (SongInfo *s)
{
	gchar 	*summary = NULL;
	
	if (icon == NULL)
		return;
		
	if (s == NULL)
	{
		gtk_status_icon_set_tooltip (icon, APPNAME);
		return;
	}
	
	if (s->title != NULL)
	{
		if (s->artist != NULL)
			summary = g_strdup_printf ("%s\n%s - %s", APPNAME, s->title, s->artist);
		else
			summary = g_strdup_printf ("%s\n%s", APPNAME, s->title);
	}
	else
	{	
		gchar *file;
		file = g_path_get_basename (s->file);
		summary = g_strdup_printf ("%s", file);
		g_free (file);
	}
	
	gtk_status_icon_set_tooltip (icon, summary);
	g_free (summary);
	
	return;
}

