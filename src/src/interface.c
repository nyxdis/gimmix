/*
 * interface.c
 *
 * Copyright (C) 2006 Priyank Gosalia
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Priyank Gosalia <priyankmgg@gmail.com>
 */

#include <glib.h>
#include <libnotify/notify.h>
#include <glade/glade.h>
#include "interface.h"
#include "playlist.h"
#include "gimmix.h"

enum {	PLAY,
		PAUSE,
		STOP
	};

static int status;
GtkWidget *progress;
GtkWidget *progressbox;

static gboolean 	gimmix_timer (void);
static void 		gimmix_about_show (void);
static void 		gimmix_show_ver_info (void);
static void 		gimmix_systray_popup_menu (void);
static void 		gimmix_update_volume (void);
static void			gimmix_systray_icon_create (void);
static void 		gimmix_window_visible (void);
static GtkWidget* 	get_image (const gchar *, GtkIconSize);

/* Callbacks */
static int			cb_gimmix_main_window_delete_event (GtkWidget *widget, gpointer data);
static void			cb_play_button_clicked 	(GtkWidget *widget, gpointer data);
static void			cb_stop_button_clicked 	(GtkWidget *widget, gpointer data);
static void			cb_next_button_clicked 	(GtkWidget *widget, gpointer data);
static void			cb_prev_button_clicked 	(GtkWidget *widget, gpointer data);
static void 		cb_info_button_clicked 	(GtkWidget *widget, gpointer data);
static void 		cb_pref_button_clicked 	(GtkWidget *widget, gpointer data);
static void 		cb_repeat_button_toggled 	(GtkToggleButton *button, gpointer data);
static void 		cb_shuffle_button_toggled 	(GtkToggleButton *button, gpointer data);

static void 		cb_gimmix_progress_seek (GtkWidget *widget, GdkEvent *event);

static void 		cb_volume_scale_changed (GtkWidget *widget, gpointer data);
static void			cb_volume_slider_scroll (GtkWidget *widget, GdkEventScroll *event);

static void 		cb_pref_apply_clicked (GtkWidget *widget, gpointer data);
static void			cb_pref_systray_checkbox_toggled (GtkWidget *widget, gpointer data);

/*
static void			cb_window_expanded (GObject *object, GParamSpec *pspec, gpointer data);
*/

void
gimmix_init (void)
{
	GtkWidget 		*widget;
	GtkWidget		*image;
	GtkAdjustment	*vol_adj;
	GdkPixbuf		*icon;
	gchar			*path;
	gint			state;
	
	status = gimmix_get_status (pub->gmo);

	/* Set the application icon */
	widget = glade_xml_get_widget (xml, "main_window");
	g_signal_connect (G_OBJECT(widget), "delete-event", G_CALLBACK(cb_gimmix_main_window_delete_event), NULL);
	path = g_strdup_printf ("%s%s", PREFIX, "/share/pixmaps/gimmix.png");
    icon = gdk_pixbuf_new_from_file (path, NULL);
    gtk_window_set_icon (GTK_WINDOW(widget), icon);
    g_object_unref (icon);
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
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(cb_repeat_button_toggled), NULL);
	
	widget = glade_xml_get_widget (xml, "shuffle_toggle");
	if (is_gimmix_shuffle (pub->gmo))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(widget), TRUE);
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(cb_shuffle_button_toggled), NULL);

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
	
	/*widget = glade_xml_get_widget (xml, "playlist_expander");
	g_signal_connect (G_OBJECT(widget), "notify::expanded", G_CALLBACK(cb_window_expanded), NULL);*/
	
	widget = glade_xml_get_widget (xml, "play_button");
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(cb_play_button_clicked), NULL);

	gimmix_systray_icon_create ();
	
    if (pub->conf->systray_enable != 1)
	{	
		//notify = gimmix_notify_init (tray_icon);
        gtk_status_icon_set_visible (tray_icon, FALSE);
	}

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
	
	song_is_changed = false;
	g_timeout_add (300, (GSourceFunc)gimmix_timer, NULL);
	
	gimmix_playlist_init ();
	gimmix_update_current_playlist ();
}

static gboolean
gimmix_timer (void)
{
	gchar 	time[15];
	int 	new_status;
	float 	fraction;

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
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.0);
			gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), "Stopped");
			gimmix_show_ver_info ();
		}
		return TRUE;
	}
	else
	{
		GtkWidget *button;
		GtkWidget *image;

		status = new_status;
		if (status == PLAY)
		{
			image = get_image ("gtk-media-pause", GTK_ICON_SIZE_BUTTON);
			button = glade_xml_get_widget (xml, "play_button");
			gtk_button_set_image (GTK_BUTTON(button), image);
		}
		else if (status == PAUSE || status == STOP)
		{
			image = get_image ("gtk-media-play", GTK_ICON_SIZE_BUTTON);
			button = glade_xml_get_widget (xml, "play_button");
			gtk_button_set_image (GTK_BUTTON(button), image);
		}
		return TRUE;
	}
}

/*
static void
cb_window_expanded (GObject *object, GParamSpec *pspec, gpointer data)
{
	GtkWidget 		*window;
	GtkExpander 	*expander;
	GtkWidget		*notebook;
	static gint 	new_width;
	static gint		new_height;

	window = glade_xml_get_widget (xml, "main_window");
	notebook = glade_xml_get_widget (xml, "playlist_notebook");
	expander = GTK_EXPANDER (object);
	
	if (!width || !height)
		gtk_window_get_size (window, &width, &height);
	
	if ((width != new_width) || (height != new_height))
	{
		gtk_window_get_size (window, &new_width, &new_height);
		width = new_width;
		height = new_height;
	}
		
	if (gtk_expander_get_expanded (expander))
	{	
		gtk_widget_show (notebook);
		gtk_window_resize (GTK_WINDOW(window), width, height);
	}
	else
	{
		gtk_widget_hide (notebook);
		gtk_window_resize (GTK_WINDOW(window), width, 1);
	}
	
	return;
}*/

static void
cb_prev_button_clicked (GtkWidget *widget, gpointer data)
{
	if (gimmix_prev(pub->gmo))
		gimmix_set_song_info();
}

static void
cb_next_button_clicked (GtkWidget *widget, gpointer data)
{
	if (gimmix_next(pub->gmo))
		gimmix_set_song_info ();
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
	gimmix_toggle_repeat (pub->gmo);
	return;
}

static void
cb_shuffle_button_toggled (GtkToggleButton *button, gpointer data)
{
	gimmix_toggle_shuffle (pub->gmo);
	return;
}
		
static void
cb_pref_button_clicked (GtkWidget *widget, gpointer data)
{
	gchar 		port[8];
	gint 		systray_enable;
	GtkWidget	*entry;
	GtkWidget	*pref_window;
	
	pref_window = glade_xml_get_widget (xml, "prefs_window");

	sprintf (port, "%d", pub->conf->port);
	systray_enable = pub->conf->systray_enable;

	entry = glade_xml_get_widget (xml,"host_entry");
	gtk_entry_set_text (GTK_ENTRY(entry), pub->conf->hostname);
	
	entry = glade_xml_get_widget (xml,"port_entry");
	gtk_entry_set_text (GTK_ENTRY(entry), port);
	
	if (pub->conf->password)
	{
		entry = glade_xml_get_widget (xml,"password_entry");
		gtk_entry_set_text (GTK_ENTRY(entry), pub->conf->password);
	}
	
	entry = glade_xml_get_widget (xml, "systray_checkbutton");
	if (systray_enable == 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(entry), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(entry), FALSE);
	g_signal_connect (G_OBJECT(entry), "toggled", G_CALLBACK(cb_pref_systray_checkbox_toggled), NULL);
	
	widget = glade_xml_get_widget (xml, "button_apply");
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(cb_pref_apply_clicked), NULL);
	gtk_widget_show (GTK_WIDGET(pref_window));
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
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(pref_widget)))
		pub->conf->systray_enable = 1;
	else
		pub->conf->systray_enable = 0;

	gimmix_config_save (pub->conf);
}

static void 
cb_pref_systray_checkbox_toggled (GtkWidget *widget, gpointer data)
{
	if (pub->conf->systray_enable == 1)
	{	
		pub->conf->systray_enable = 0;
		//g_object_unref (G_OBJECT(notify));
		gtk_status_icon_set_visible (tray_icon, FALSE);
		return;
	}
	else
	{
		pub->conf->systray_enable = 1;
		//gimmix_systray_icon_create ();
		gtk_status_icon_set_visible (tray_icon, TRUE);
		notify = gimmix_notify_init (tray_icon);
	}
	return;
}

static void
cb_info_button_clicked (GtkWidget *widget, gpointer data)
{
	gint state;

	state = gimmix_get_status (pub->gmo);
	
	if (state == PLAY || state == PAUSE)
	{
		SongInfo 	*info = NULL;
		GtkWidget 	*widget;
		GtkWidget	*window;
		gchar 		*length;
		gchar 		*bitrate;
		
		info = gimmix_get_song_info (pub->gmo);
		window = glade_xml_get_widget (xml, "info_window");

		widget = glade_xml_get_widget (xml, "info_file");
		gtk_entry_set_text (GTK_ENTRY(widget), info->file ? info->file : NULL);

		widget = glade_xml_get_widget (xml,"info_title");
		gtk_label_set_text (GTK_LABEL(widget), info->title ? info->title : NULL);
		
		widget = glade_xml_get_widget (xml,"info_artist");
		gtk_label_set_text (GTK_LABEL(widget), info->artist ? info->artist : NULL);
		
		widget = glade_xml_get_widget (xml,"info_album");
		gtk_label_set_text (GTK_LABEL(widget), info->album ? info->album : NULL);
		
		widget = glade_xml_get_widget (xml,"info_genre");
		gtk_label_set_text (GTK_LABEL(widget), info->genre ? info->genre : NULL);
		
		widget = glade_xml_get_widget (xml, "info_length");
		length = gimmix_get_song_length (info);
		if (length)
		{
			gtk_label_set_text (GTK_LABEL(widget), length);
			g_free (length);
		}

		widget = glade_xml_get_widget (xml, "info_bitrate");
		bitrate = gimmix_get_song_bitrate (info);
		if (bitrate)
		{
			gtk_label_set_text (GTK_LABEL(widget), bitrate);
			g_free (bitrate);
		}
		else
			gtk_label_set_text (GTK_LABEL(widget), "(only available while playing)");

		gimmix_free_song_info (info);

		gtk_widget_show (GTK_WIDGET(window));
	}
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
gimmix_update_volume ()
{
	gint 			volume;
	GtkWidget		*widget;
	GtkAdjustment	*volume_adj;
	
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
	
	song = gimmix_get_song_info (pub->gmo);
	window = glade_xml_get_widget (xml, "main_window");
	song_label = glade_xml_get_widget (xml,"song_label");
	artist_label = glade_xml_get_widget (xml,"artist_label");
	album_label = glade_xml_get_widget (xml,"album_label");
		
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
	gimmix_free_song_info (song);
	
	return;
}

static void
gimmix_systray_icon_create (void)
{
	gchar *icon;
	
	icon = g_strdup_printf ("%s%s", PREFIX, "/share/pixmaps/gimmix.png");
	tray_icon = gtk_status_icon_new_from_file(icon);
	g_free (icon);
	gtk_status_icon_set_tooltip(tray_icon, "Gimmix");
	g_signal_connect (tray_icon, "popup-menu", G_CALLBACK (gimmix_systray_popup_menu), NULL);
	g_signal_connect (tray_icon, "activate", G_CALLBACK(gimmix_window_visible), NULL);
	
	return;
}

static void
gimmix_systray_popup_menu (void)
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
		g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_play_button_clicked), NULL);
	}
	else
	{
		menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PLAY, NULL);
		g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_play_button_clicked), NULL);
	}
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

NotifyNotification *
gimmix_notify_init (GtkStatusIcon *status_icon)
{
	NotifyNotification *notify;
	
	/* Initialize notify */
	if(!notify_is_initted())
		notify_init("Gimmix");
	
	notify = notify_notification_new_with_status_icon (APPNAME, APPNAME, "gtk-cdrom", status_icon);
	
	notify_notification_set_timeout (notify, 1000);
	//notify_notification_show(notify, NULL);
	
	return notify;
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

	gchar *website = "http://priyank.one09.net/gimmix";
	gchar *website_label = "http://priyank.one09.net/gimmix";
	gchar *authors[] = 	{ "Priyank M. Gosalia <priyankmg@gmail.com>",
						 "Part of the song seek code borrowed from Pygmy.",
						 NULL
						};
	
	gtk_show_about_dialog (NULL,
                           "name", APPNAME,
                           "version", VERSION,
                           "copyright", "\xC2\xA9 2006 Priyank Gosalia  (GPL)",
                           "comments", "Gimmix is a graphical Music player daemon (MPD) client written in C",
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
	gtk_label_set_text (GTK_LABEL(artist_label), "http://priyank.one09.net/gimmix");
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

		
