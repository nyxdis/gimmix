/*
 * interface.c
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
 * Author: Priyank Gosalia <priyankmgg@gmail.com>
 */

#include <glib.h>
#include <libnotify/notify.h>
#include <glade/glade.h>
#include "gimmix-interface.h"
#include "gimmix-playlist.h"
#include "gimmix-tagedit.h"
#include "gimmix.h"

enum {	PLAY,
	PAUSE,
	STOP
};

static int 			status;
static GtkWidget 		*progress;
static GtkWidget 		*progressbox;
static GtkStatusIcon 		*icon;
static NotifyNotification 	*notify;

static gboolean 	gimmix_timer (void);
static void 		gimmix_about_show (void);
static void 		gimmix_show_ver_info (void);
static void 		gimmix_systray_popup_menu (GtkStatusIcon *sicon, guint button);
static void		gimmix_update_volume (void);
static void		gimmix_update_repeat (void);
static void		gimmix_update_shuffle (void);
static void 		gimmix_window_visible (void);
static GtkWidget* 	get_image (const gchar *, GtkIconSize);

/* Callbacks */
static int		cb_gimmix_main_window_delete_event (GtkWidget *widget, gpointer data);
static void		cb_play_button_clicked 	(GtkWidget *widget, gpointer data);
static void		cb_stop_button_clicked 	(GtkWidget *widget, gpointer data);
static void		cb_next_button_clicked 	(GtkWidget *widget, gpointer data);
static void		cb_prev_button_clicked 	(GtkWidget *widget, gpointer data);
static void 		cb_info_button_clicked 	(GtkWidget *widget, gpointer data);
static void 		cb_pref_button_clicked 	(GtkWidget *widget, gpointer data);
static void 		cb_repeat_button_toggled (GtkToggleButton *button, gpointer data);
static void 		cb_shuffle_button_toggled (GtkToggleButton *button, gpointer data);

static void 		cb_gimmix_progress_seek (GtkWidget *widget, GdkEvent *event);

static void 		cb_volume_scale_changed (GtkWidget *widget, gpointer data);
static void		cb_volume_slider_scroll (GtkWidget *widget, GdkEventScroll *event);

static void 		cb_pref_apply_clicked (GtkWidget *widget, gpointer data);
static void		cb_pref_systray_checkbox_toggled (GtkToggleButton *button, gpointer data);
static void		cb_pref_notify_checkbox_toggled (GtkToggleButton *button, gpointer data);
static void		cb_systray_popup_play_clicked (GtkMenuItem *menuitem, gpointer data);
static void		gimmix_update_and_display_notification (NotifyNotification *notify, SongInfo *s, gboolean display);

static void		gimmix_create_systray_icon (gboolean notify_enable);
static void		gimmix_disable_systray_icon (void);
static void		gimmix_enable_systray_icon (void);
static NotifyNotification *	gimmix_create_notification (void);

void
gimmix_init (void)
{
	GtkWidget 		*widget;
	GtkWidget		*image;
	GtkAdjustment	*vol_adj;
	GdkPixbuf		*app_icon;
	gchar			*path;
	gint			state;
	
	status = gimmix_get_status (pub->gmo);

	/* Set the application icon */
	widget = glade_xml_get_widget (xml, "main_window");
	g_signal_connect (G_OBJECT(widget), "delete-event", G_CALLBACK(cb_gimmix_main_window_delete_event), NULL);
	path = g_strdup_printf ("%s%s", PREFIX, "/share/pixmaps/gimmix.png");
	app_icon = gdk_pixbuf_new_from_file (path, NULL);
	gtk_window_set_icon (GTK_WINDOW(widget), app_icon);
	g_object_unref (app_icon);
	g_free (path);
	
	widget = glade_xml_get_widget (xml, "prev_button");
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(cb_prev_button_clicked), NULL);
	
	widget = glade_xml_get_widget (xml, "next_button");
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(cb_next_button_clicked), NULL);
	
	widget = glade_xml_get_widget (xml, "stop_button");
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(cb_stop_button_clicked), NULL);
	
	widget = glade_xml_get_widget (xml, "pref_button");
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(cb_pref_button_clicked), NULL);
	
	widget = glade_xml_get_widget (xml, "repeat_toggle");
	if (is_gimmix_repeat (pub->gmo))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(widget), TRUE);
	g_signal_connect (G_OBJECT(widget), "toggled", G_CALLBACK(cb_repeat_button_toggled), NULL);
	
	widget = glade_xml_get_widget (xml, "shuffle_toggle");
	if (is_gimmix_shuffle (pub->gmo))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(widget), TRUE);
	g_signal_connect (G_OBJECT(widget), "toggled", G_CALLBACK(cb_shuffle_button_toggled), NULL);

	widget = glade_xml_get_widget (xml, "info_button");
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(cb_info_button_clicked), NULL);

	widget = glade_xml_get_widget (xml, "volume_scale");
	g_signal_connect(G_OBJECT(widget), "value_changed", G_CALLBACK(cb_volume_scale_changed), NULL);
	g_signal_connect (G_OBJECT(widget), "scroll_event", G_CALLBACK(cb_volume_slider_scroll), NULL);
	vol_adj = gtk_range_get_adjustment (GTK_RANGE(widget));
	gtk_adjustment_set_value (GTK_ADJUSTMENT(vol_adj), gimmix_get_volume(pub->gmo));

	progress = glade_xml_get_widget (xml,"progress");
	progressbox = glade_xml_get_widget (xml,"progress_event_box");
	g_signal_connect (G_OBJECT(progressbox), "button_press_event", G_CALLBACK(cb_gimmix_progress_seek), NULL);
	
	widget = glade_xml_get_widget (xml, "play_button");
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(cb_play_button_clicked), NULL);

	if (status == PLAY)
	{
		gchar time[15];
		float fraction;
		image = get_image ("gtk-media-pause", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image (GTK_BUTTON(widget), image);
		gimmix_get_progress_status (pub->gmo, &fraction, time);
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), fraction);
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), time);
		gimmix_set_song_info ();
	}
	else if (status == PAUSE)
	{
		image = get_image ("gtk-media-play", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image (GTK_BUTTON(widget), image);
		gimmix_set_song_info ();
	}
	else if (status == STOP)
	{
		gimmix_show_ver_info();
	}
	
	if (pub->conf->notify_enable == 1)
		gimmix_create_systray_icon (TRUE);
	else
		gimmix_create_systray_icon (FALSE);

	if (pub->conf->systray_enable == 0)
	{
		gtk_status_icon_set_visible (icon, FALSE);
	}
	
	song_is_changed = false;
	g_timeout_add (300, (GSourceFunc)gimmix_timer, NULL);

	gimmix_playlist_init ();
	gimmix_tag_editor_init ();
	gimmix_update_current_playlist ();

	return;
}

static gboolean
gimmix_timer (void)
{
	gchar 	time[15];
	int 	new_status;
	float 	fraction;
	static	gboolean stop;

	new_status = gimmix_get_status (pub->gmo);

	if (song_is_changed && new_status == PLAY)
	{
		gimmix_set_song_info ();
		song_is_changed = false;
	}
	
	if (playlist_is_changed)
	{
		gimmix_update_current_playlist ();
		playlist_is_changed = false;
	}
	
	if (volume_is_changed)
	{
		gimmix_update_volume ();
		volume_is_changed = false;
	}
	
	if (repeat_is_changed)
	{
		gimmix_update_repeat ();
		repeat_is_changed = false;
	}
	
	if (shuffle_is_changed)
	{
		gimmix_update_shuffle ();
		shuffle_is_changed = false;
	}
	
	if (status == new_status)
	{
		if (status == PLAY || status == PAUSE)
		{
			gimmix_get_progress_status (pub->gmo, &fraction, time);
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), fraction);
			gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), time);
		}
		else if (status == STOP)
		{
			if (stop == true)
			{
				gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.0);
				gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), "Stopped");
				gimmix_show_ver_info ();
				stop = false;
			}
		}
		return TRUE;
	}
	else
	{
		GtkWidget *button;
		GtkWidget *image;
		GtkTooltips *tooltip;
		
		button = glade_xml_get_widget (xml, "play_button");
		tooltip = gtk_tooltips_new ();
		status = new_status;
		if (status == PLAY)
		{
			image = get_image ("gtk-media-pause", GTK_ICON_SIZE_BUTTON);
			gtk_button_set_image (GTK_BUTTON(button), image);
			gtk_tooltips_set_tip (tooltip, button, "Pause", NULL);
			stop = true;
		}
		else if (status == PAUSE || status == STOP)
		{
			image = get_image ("gtk-media-play", GTK_ICON_SIZE_BUTTON);
			gtk_button_set_image (GTK_BUTTON(button), image);
			gtk_tooltips_set_tip (tooltip, button, "Play", NULL);
		}
		return TRUE;
	}
}

static void
cb_prev_button_clicked (GtkWidget *widget, gpointer data)
{
	if (gimmix_prev(pub->gmo))
		gimmix_set_song_info();

	return;
}

static void
cb_next_button_clicked (GtkWidget *widget, gpointer data)
{
	if (gimmix_next(pub->gmo))
		gimmix_set_song_info ();
	
	return;
}

static void
cb_play_button_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget	*image;
	
	if (gimmix_play(pub->gmo))
	{
		image = get_image ("gtk-media-pause", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image (GTK_BUTTON(widget), image);
		gimmix_set_song_info ();
	}
	else
	{
		image = get_image ("gtk-media-play", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image (GTK_BUTTON(widget), image);
	}
	
	return;
}

static void
cb_stop_button_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget *image;
	GtkWidget *play_button;

	if (gimmix_stop(pub->gmo))
	{
		play_button = glade_xml_get_widget (xml, "play_button");
		image = get_image ("gtk-media-play", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image (GTK_BUTTON(play_button), image);
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.0);
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), "Stopped");
		gimmix_show_ver_info ();
	}

	return;
}

static void
cb_repeat_button_toggled (GtkToggleButton *button, gpointer data)
{
	gboolean state;
	
	state = gtk_toggle_button_get_active (button);
	if (state == TRUE)
	{
		gimmix_repeat (pub->gmo, true);
	}
	else if (state == FALSE)
	{
		gimmix_repeat (pub->gmo, false);
	}
	
	return;
}

static void
cb_shuffle_button_toggled (GtkToggleButton *button, gpointer data)
{
	gboolean state;
	
	state = gtk_toggle_button_get_active (button);
	if (state == TRUE)
	{
		gimmix_shuffle (pub->gmo, true);
	}
	else if (state == FALSE)
	{
		gimmix_shuffle (pub->gmo, false);
	}
	
	return;
}
		
static void
cb_pref_button_clicked (GtkWidget *widget, gpointer data)
{
	gchar 		*port;
	gint 		systray_enable;
	gint		notify_enable;
	gint		disable_notify;
	GtkWidget	*entry;
	GtkWidget	*pref_window;
	
	pref_window = glade_xml_get_widget (xml, "prefs_window");
	port = g_strdup_printf ("%d", pub->conf->port);
	systray_enable = pub->conf->systray_enable;
	notify_enable = pub->conf->notify_enable;

	entry = glade_xml_get_widget (xml,"host_entry");
	gtk_entry_set_text (GTK_ENTRY(entry), pub->conf->hostname);
	
	entry = glade_xml_get_widget (xml,"port_entry");
	gtk_entry_set_text (GTK_ENTRY(entry), port);
	g_free (port);

	if (pub->conf->password)
	{
		entry = glade_xml_get_widget (xml,"password_entry");
		gtk_entry_set_visibility (GTK_ENTRY(entry), FALSE);
		gtk_entry_set_invisible_char (GTK_ENTRY(entry), g_utf8_get_char("*"));
		gtk_entry_set_text (GTK_ENTRY(entry), pub->conf->password);
	}

	entry = glade_xml_get_widget (xml, "systray_checkbutton");
	if (systray_enable == 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(entry), TRUE);
	else
	{
		disable_notify = 1;
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(entry), FALSE);
	}
	g_signal_connect (G_OBJECT(entry), "toggled", G_CALLBACK(cb_pref_systray_checkbox_toggled), NULL);
	
	entry = glade_xml_get_widget (xml, "notify_checkbutton");
	if (notify_enable == 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(entry), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(entry), FALSE);
	if (disable_notify == 1)
		gtk_widget_set_sensitive (entry, FALSE);
	g_signal_connect (G_OBJECT(entry), "toggled", G_CALLBACK(cb_pref_notify_checkbox_toggled), NULL);
	
	widget = glade_xml_get_widget (xml, "button_apply");
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(cb_pref_apply_clicked), NULL);
	gtk_widget_show (GTK_WIDGET(pref_window));
	
	return;
}

static void
cb_pref_apply_clicked (GtkWidget *widget, gpointer data)
{
	const gchar *host;
	const gchar *port;
	const gchar *password;
	GtkWidget *pref_widget;

	pref_widget = glade_xml_get_widget (xml,"host_entry");
	host = gtk_entry_get_text (GTK_ENTRY(pref_widget));
	
	pref_widget = glade_xml_get_widget (xml,"port_entry");
	port = gtk_entry_get_text (GTK_ENTRY(pref_widget));
	
	pref_widget = glade_xml_get_widget (xml,"password_entry");
	password = gtk_entry_get_text (GTK_ENTRY(pref_widget));

	pref_widget = glade_xml_get_widget (xml, "systray_checkbutton");
	
	strncpy (pub->conf->hostname, host, 255);
	strncpy (pub->conf->password, password, 255);
	pub->conf->port = atoi (port);

	gimmix_config_save (pub->conf);
	
	return;
}

static void
cb_pref_systray_checkbox_toggled (GtkToggleButton *button, gpointer data)
{
	GtkWidget *notify_checkbutton;
	notify_checkbutton = glade_xml_get_widget (xml, "notify_checkbutton");
	
	if (gtk_toggle_button_get_active(button) == TRUE)
	{
		gtk_widget_set_sensitive (notify_checkbutton, TRUE);
		gimmix_enable_systray_icon ();
		pub->conf->systray_enable = 1;
	}
	else
	if (gtk_toggle_button_get_active(button) == FALSE)
	{
			gtk_widget_set_sensitive (notify_checkbutton, FALSE);
			gimmix_disable_systray_icon ();
			pub->conf->systray_enable = 0;
			pub->conf->notify_enable = 0;
	}
	
	gimmix_config_save (pub->conf);

	return;
}

static void
cb_pref_notify_checkbox_toggled (GtkToggleButton *button, gpointer data)
{
	if (gtk_toggle_button_get_active(button) == TRUE)
	{	
		notify = gimmix_create_notification ();
		pub->conf->notify_enable = 1;
	}
	else
	if (gtk_toggle_button_get_active(button) == FALSE)
	{
		pub->conf->notify_enable = 0;
	}
	
	return;
}

static void
cb_info_button_clicked (GtkWidget *widget, gpointer data)
{
	gint 		state;
	GtkWidget	*window;
	SongInfo	*info;
	gchar		song[255];
	
	state = gimmix_get_status (pub->gmo);
	window = glade_xml_get_widget (xml, "tag_editor_window");
	
	if (state == PLAY || state == PAUSE)
	{
		info = gimmix_get_song_info (pub->gmo);
		snprintf (song, 255, "%s/%s", "/mnt/music", info->file);
		gimmix_tag_editor_populate (song);
		gimmix_free_song_info (info);
		gtk_widget_show (GTK_WIDGET(window));
	}
	
	return;
}

static void
cb_volume_scale_changed (GtkWidget *widget, gpointer data)
{
	GtkAdjustment *volume_adj;
	gint value;
	
	volume_adj = gtk_range_get_adjustment (GTK_RANGE(widget));

	value = gtk_adjustment_get_value (GTK_ADJUSTMENT(volume_adj));
	gimmix_set_volume (pub->gmo, value);
	
	return;
}

static void
cb_volume_slider_scroll (GtkWidget *widget, GdkEventScroll *event)
{
	gint volume;
	GtkAdjustment *volume_adj;
	if (event->type != GDK_SCROLL)
		return;
	
	volume_adj = gtk_range_get_adjustment (GTK_RANGE(widget));
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
gimmix_update_repeat (void)
{
	GtkWidget *button;
	
	button = glade_xml_get_widget (xml, "repeat_toggle");
	if (is_gimmix_repeat (pub->gmo))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), FALSE);
	
	return;
}

static void
gimmix_update_shuffle (void)
{
	GtkWidget *button;
	
	button = glade_xml_get_widget (xml, "shuffle_toggle");
	if (is_gimmix_shuffle (pub->gmo))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), FALSE);
	
	return;
}

static void
gimmix_update_volume ()
{
	gint 			volume;
	GtkWidget		*widget;
	GtkAdjustment		*volume_adj;
	
	widget = glade_xml_get_widget (xml, "volume_scale");
	volume_adj = gtk_range_get_adjustment (GTK_RANGE(widget));
	volume = gimmix_get_volume (pub->gmo);
	gtk_adjustment_set_value (GTK_ADJUSTMENT(volume_adj), volume);
	
	return;
}

void
cb_gimmix_progress_seek (GtkWidget *progressbox, GdkEvent *event)
{
	GtkAllocation allocation;
	gint x, newtime, totaltime;
	gdouble seektime;

	x = event->button.x;
	allocation = GTK_WIDGET (progressbox)->allocation;
	totaltime = gimmix_get_total_song_time (pub->gmo);
	seektime = (gdouble)x/allocation.width;
	newtime = seektime * totaltime;
	if (gimmix_seek(pub->gmo, newtime))
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), seektime);

	return;
}

static void
gimmix_window_visible (void)
{
	static int x;
	static int y;
	GtkWidget *window;

	window = glade_xml_get_widget (xml, "main_window");
	if( !GTK_WIDGET_VISIBLE (window) )
	{	
		gtk_window_move (GTK_WINDOW(window), x, y);
		gtk_widget_show (GTK_WIDGET(window));
	}
	else
	{	
		gtk_window_get_position (GTK_WINDOW(window), &x, &y);
		gtk_widget_hide (GTK_WIDGET(window));
	}

	return;
}

static GtkWidget *
get_image (const gchar *id, GtkIconSize size)
{
	GtkWidget *image;

	image = gtk_image_new_from_stock (id, size);

	return image;
}

void
gimmix_set_song_info (void)
{
	gchar 		*markup;
	gchar 		*title;
	SongInfo 	*song = NULL;
	GtkWidget 	*window;
	GtkWidget	*artist_label;
	GtkWidget	*album_label;
	GtkWidget	*song_label;
	
	song 		= gimmix_get_song_info (pub->gmo);
	window 		= glade_xml_get_widget (xml, "main_window");
	song_label 	= glade_xml_get_widget (xml,"song_label");
	artist_label 	= glade_xml_get_widget (xml,"artist_label");
	album_label 	= glade_xml_get_widget (xml,"album_label");
		
	if (song->title)
	{
		title = g_strdup_printf ("Gimmix - %s", song->title);
		gtk_window_set_title (GTK_WINDOW(window), title);
		g_free (title);
		markup = g_markup_printf_escaped ("<span size=\"medium\"weight=\"bold\"><i>%s</i></span>", song->title);
		gtk_label_set_markup (GTK_LABEL(song_label), markup);
	}
	else
	{
		markup = g_markup_printf_escaped ("<span size=\"medium\"weight=\"bold\"><i>%s</i></span>", g_path_get_basename(song->file));
		gtk_label_set_markup (GTK_LABEL(song_label), markup);
		gtk_window_set_title (GTK_WINDOW(window), "Gimmix");
	}

	if (song->artist)
		gtk_label_set_text (GTK_LABEL(artist_label), song->artist);
	else
		gtk_label_set_text (GTK_LABEL(artist_label), NULL);
	if (song->album)
		gtk_label_set_text (GTK_LABEL(album_label), song->album);
	else
		gtk_label_set_text (GTK_LABEL(album_label), NULL);

	g_free (markup);
	if ((pub->conf->notify_enable == 1) && (notify!=NULL))
		gimmix_update_and_display_notification (notify, song, TRUE);
	
	gimmix_free_song_info (song);
	
	return;
}

static void
gimmix_systray_popup_menu (GtkStatusIcon *sicon, guint button)
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
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_stop_button_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_prev_button_clicked), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_NEXT, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_next_button_clicked), NULL);
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
	gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 1,gtk_get_current_event_time());
	
	return;
}


static void
cb_systray_popup_play_clicked (GtkMenuItem *menuitem, gpointer data)
{
	GtkWidget	*image;
	
	if (gimmix_play(pub->gmo))
	{
		image = get_image ("gtk-media-pause", GTK_ICON_SIZE_BUTTON);
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menuitem), image);
		gimmix_set_song_info ();
	}
	else
	{
		image = get_image ("gtk-media-play", GTK_ICON_SIZE_BUTTON);
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menuitem), image);
	}
	
	return;
}

static void
gimmix_update_and_display_notification (NotifyNotification *notify,
					SongInfo *s,
					gboolean display)
{
	gchar 			*summary;
	GdkScreen 		*screen;
	GdkRectangle 		area;
	
	if (pub->conf->notify_enable != 1)
	return;
	
	if (s->title != NULL)
	{
		if (s->artist != NULL)
			summary = g_strdup_printf ("%s\n  %s", s->title, s->artist);
		else
			summary = g_strdup_printf ("%s\n", s->title);
	}
	else
	{	
		summary = g_strdup_printf ("%s", g_path_get_basename(s->file));
	}
	
	notify_notification_update (notify, summary, NULL, NULL);
	g_free (summary);
	gtk_status_icon_get_geometry (icon, &screen, &area, NULL);
	notify_notification_set_geometry_hints (notify, screen, area.x, area.y);
	
	if (display)
	{
		notify_notification_close (notify, NULL);
		notify_notification_show (notify, NULL);
	}
	
	return;
}

static void
gimmix_about_show (void)
{
 	GdkPixbuf 	*about_pixbuf;
	gchar		*path;
	gchar 		*license = 
	("Gimmix is free software; you can redistribute it and/or "
	"modify it under the terms of the GNU General Public Licence as "
	"published by the Free Software Foundation; either version 2 of the "
	"Licence, or (at your option) any later version.\n"
	"\n"
	"Gimmix is distributed in the hope that it will be useful, "
	"but WITHOUT ANY WARRANTY; without even the implied warranty of "
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU "
	"General Public Licence for more details.\n"
	"\n"
	"You should have received a copy of the GNU General Public Licence "
	"along with Gimmix; if not, write to the Free Software "
	"Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, "
	"MA  02110-1301  USA");
	
	path = g_strdup_printf ("%s%s", PREFIX, "/share/pixmaps/gimmix.png");
	about_pixbuf = gdk_pixbuf_new_from_file (path, NULL);
	g_free (path);

	gchar *website = "http://gimmix.berlios.de/";
	gchar *website_label = "http://priyank.one09.net/gimmix";
	gchar *authors[] = 	{ "Priyank M. Gosalia <priyankmg@gmail.com>",
				 "Part of the song seek code borrowed from Pygmy.",
				 NULL
				};
	
	gtk_show_about_dialog (NULL,
                           "name", APPNAME,
                           "version", VERSION,
                           "copyright", "\xC2\xA9 2006 Priyank Gosalia  (GPL)",
                           "comments", "Gimmix is a graphical music player daemon (MPD) client written in C.",
                           "license", license,
                           "authors", authors,
                           "website", website,
                           "website-label", website_label,
                           "logo", about_pixbuf,
                           "wrap-license", true,
                           NULL);
	g_object_unref (about_pixbuf);

	return;
}

static void
gimmix_show_ver_info (void)
{
	gchar 		*markup;
	gchar 		*appver;
	GtkWidget	*artist_label;
	GtkWidget	*album_label;
	GtkWidget	*song_label;
	GtkWidget	*window;

	song_label = glade_xml_get_widget (xml, "song_label");
	artist_label = glade_xml_get_widget (xml, "artist_label");
	album_label = glade_xml_get_widget (xml, "album_label");
	window = glade_xml_get_widget (xml, "main_window");

	appver = g_strdup_printf ("%s %s", APPNAME, VERSION);
	markup = g_markup_printf_escaped ("<span size=\"large\"weight=\"bold\">%s</span>", appver);
	gtk_label_set_markup (GTK_LABEL(song_label), markup);
	gtk_label_set_text (GTK_LABEL(artist_label), "http://gimmix.berlios.de");
	gtk_label_set_text (GTK_LABEL(album_label), NULL);
	gtk_window_set_title (GTK_WINDOW(window), APPNAME);
	g_free (markup);
	g_free (appver);
	
	return;
}

static int
cb_gimmix_main_window_delete_event (GtkWidget *widget, gpointer data)
{
	if (pub->conf->systray_enable == 1)
	{	
		gimmix_window_visible ();
		return 1;
	}

	return 0;
}

static void
gimmix_create_systray_icon (gboolean notify_enable)
{
	gchar 		*icon_file;
	GtkWidget	*systray_eventbox;
	
	icon_file = g_strdup_printf ("%s%s", PREFIX, "/share/pixmaps/gimmix.png");
	icon = gtk_status_icon_new_from_file (icon_file);
	g_free (icon_file);
	//systray_eventbox = gtk_event_box_new ();
	//gtk_container_add (GTK_CONTAINER(systray_eventbox), GTK_WIDGET(icon));
	//gtk_event_box_set_above_child (GTK_EVENT_BOX(systray_eventbox), TRUE);
	//g_object_set_data (G_OBJECT(systray_eventbox), "data", icon);
	//gtk_widget_show (GTK_WIDGET(systray_eventbox));
	gtk_status_icon_set_visible (icon, TRUE);
	gtk_status_icon_set_tooltip (icon, APPNAME);
	g_signal_connect (icon, "popup-menu", G_CALLBACK (gimmix_systray_popup_menu), NULL);
	g_signal_connect (icon, "activate", G_CALLBACK(gimmix_window_visible), NULL);
	//g_signal_connect (systray_eventbox, "scroll_event", G_CALLBACK(cb_volume_slider_scroll), NULL);

	if (notify_enable == TRUE)
	{
		notify = gimmix_create_notification ();
	}
	
	return;
}

static void
gimmix_disable_systray_icon (void)
{
	gtk_status_icon_set_visible (icon, FALSE);
	if (notify)
	{	
		g_object_unref (notify);
		notify = NULL;
	}
	
	pub->conf->notify_enable = 0;
	pub->conf->systray_enable = 0;
	
	return;
}

static void
gimmix_enable_systray_icon (void)
{
	gtk_status_icon_set_visible (icon, TRUE);
	if (pub->conf->notify_enable == 1)
	{	
		notify = gimmix_create_notification ();
	}
	else
	notify = NULL;
	
	return;
}

static NotifyNotification *
gimmix_create_notification (void)
{
	GdkRectangle 		area;
	GdkScreen		*screen;
	gchar			*path;
	GdkPixbuf		*pixbuf;
	NotifyNotification 	*notif;

	if (!icon)
		return NULL;

	/* Initialize notify */
	if(!notify_is_initted())
		notify_init(APPNAME);

	path = g_strdup_printf ("%s%s", PREFIX, "/share/pixmaps/gimmix.png");
	notif = notify_notification_new ("Gimmix version 0.2", "http://gimmix.berlios.de", NULL, NULL);
	notify_notification_set_category (notif, "information");
	g_free (path);

	notify_notification_set_timeout (notif, 1800);
	gtk_status_icon_get_geometry (icon, &screen, &area, NULL);
	notify_notification_set_geometry_hints (notif, screen, area.x, area.y);
	
	return notif;
}

void
gimmix_interface_cleanup (void)
{
	if (notify_is_initted())
		notify_uninit ();
	
	if (icon != NULL)
		g_object_unref (icon);
	
	return;
}
