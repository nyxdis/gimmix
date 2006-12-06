/*
 * gimmix-interface.c
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

#include <glib.h>
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>
#include "gimmix-interface.h"
#include "gimmix-playlist.h"
#include "gimmix-tagedit.h"
#include "gimmix-prefs.h"
#include "gimmix.h"

#define GIMMIX_APP_ICON  	"/share/pixmaps/gimmix_logo_small.png"

GimmixStatus 		status;
GtkWidget 			*progress = NULL;
GtkTooltips 		*play_button_tooltip = NULL;

extern GM 			*pub;
extern GladeXML 	*xml;
extern ConfigFile	conf;

static gboolean 	gimmix_timer (void);
static void			gimmix_update_volume (void);
static void			gimmix_update_repeat (void);
static void			gimmix_update_shuffle (void);
static void			gimmix_save_window_pos (void);

/* Callbacks */
static int		cb_gimmix_main_window_delete_event (GtkWidget *widget, gpointer data);
static void		cb_play_button_clicked 	(GtkWidget *widget, gpointer data);
static void		cb_stop_button_clicked 	(GtkWidget *widget, gpointer data);
static void		cb_next_button_clicked 	(GtkWidget *widget, gpointer data);
static void		cb_prev_button_clicked 	(GtkWidget *widget, gpointer data);
static void 	cb_info_button_clicked 	(GtkWidget *widget, gpointer data);
static void 	cb_pref_button_clicked 	(GtkWidget *widget, gpointer data);
static void 	cb_repeat_button_toggled (GtkToggleButton *button, gpointer data);
static void 	cb_shuffle_button_toggled (GtkToggleButton *button, gpointer data);

static void 	cb_gimmix_progress_seek (GtkWidget *widget, GdkEvent *event);
static void 	cb_volume_scale_changed (GtkWidget *widget, gpointer data);
static void		cb_volume_slider_scroll (GtkWidget *widget, GdkEventScroll *event);
static gboolean cb_gimmix_key_press(GtkWidget *widget, GdkEventKey *event, gpointer userdata);

void
gimmix_init (void)
{
	GtkWidget 		*widget;
	GtkWidget		*image;
	GtkWidget		*progressbox;
	GtkWidget		*main_window;
	GtkAdjustment	*vol_adj;
	GdkPixbuf		*app_icon;
	gchar			*path;
	gint			state;
	
	/* Set the application icon */
	main_window = glade_xml_get_widget (xml, "main_window");
	g_signal_connect (G_OBJECT(main_window), "delete-event", G_CALLBACK(cb_gimmix_main_window_delete_event), NULL);
	gtk_window_move (GTK_WINDOW(main_window), atoi(cfg_get_key_value(conf, "window_xpos")), atoi(cfg_get_key_value(conf, "window_ypos")));
	path = g_strdup_printf ("%s%s", PREFIX, GIMMIX_APP_ICON);
	app_icon = gdk_pixbuf_new_from_file_at_size (path, 12, 12, NULL);
	gtk_window_set_icon (GTK_WINDOW(main_window), app_icon);
	g_object_unref (app_icon);
	g_free (path);
	
	/* connect the key press signal */
	g_signal_connect(G_OBJECT(main_window), "key-press-event", G_CALLBACK(cb_gimmix_key_press), NULL);
	
	/* set icons for buttons */
	gtk_image_set_from_stock (GTK_IMAGE(glade_xml_get_widget(xml, "image_prev")), "gtk-media-previous", GTK_ICON_SIZE_BUTTON);
	gtk_image_set_from_stock (GTK_IMAGE(glade_xml_get_widget(xml, "image_play")), "gtk-media-play", GTK_ICON_SIZE_BUTTON);
	gtk_image_set_from_stock (GTK_IMAGE(glade_xml_get_widget(xml, "image_next")), "gtk-media-next", GTK_ICON_SIZE_BUTTON);
	gtk_image_set_from_stock (GTK_IMAGE(glade_xml_get_widget(xml, "image_stop")), "gtk-media-stop", GTK_ICON_SIZE_BUTTON);
	gtk_image_set_from_stock (GTK_IMAGE(glade_xml_get_widget(xml, "image_info")), "gtk-info", GTK_ICON_SIZE_BUTTON);
	gtk_image_set_from_stock (GTK_IMAGE(glade_xml_get_widget(xml, "image_pref")), "gtk-preferences", GTK_ICON_SIZE_BUTTON);
	
	g_signal_connect (G_OBJECT(glade_xml_get_widget (xml, "prev_button")), "clicked", G_CALLBACK(cb_prev_button_clicked), NULL);
	
	g_signal_connect (G_OBJECT(glade_xml_get_widget (xml, "play_button")), "clicked", G_CALLBACK(cb_play_button_clicked), NULL);
	
	g_signal_connect (G_OBJECT(glade_xml_get_widget (xml, "next_button")), "clicked", G_CALLBACK(cb_next_button_clicked), NULL);
	
	g_signal_connect (G_OBJECT(glade_xml_get_widget (xml, "stop_button")), "clicked", G_CALLBACK(cb_stop_button_clicked), NULL);
	
	g_signal_connect (G_OBJECT(glade_xml_get_widget (xml, "pref_button")), "clicked", G_CALLBACK(cb_pref_button_clicked), NULL);
	
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
	gtk_adjustment_set_value (GTK_ADJUSTMENT(vol_adj), mpd_status_get_volume (pub->gmo));

	progress = glade_xml_get_widget (xml,"progress");
	progressbox = glade_xml_get_widget (xml,"progress_event_box");
	g_signal_connect (G_OBJECT(progressbox), "button_press_event", G_CALLBACK(cb_gimmix_progress_seek), NULL);
	
	play_button_tooltip = gtk_tooltips_new ();
	
	if (strncasecmp(cfg_get_key_value(conf, "enable_systray"), "true", 4) == 0)
	{
		gimmix_create_systray_icon ();
	}

	status = gimmix_get_status (pub->gmo);

	if (status == PLAY)
	{
		gimmix_set_song_info ();
		status = -1;
		song_is_changed = true;
	}
	else if (status == PAUSE)
	{
		gimmix_set_song_info ();
	}
	else if (status == STOP)
	{
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), _("Stopped"));
		gimmix_show_ver_info ();
	}

	g_timeout_add (300, (GSourceFunc)gimmix_timer, NULL);

	/* initialize playlist and tag editor */
	gimmix_playlist_init ();
	gimmix_tag_editor_init ();
	gimmix_update_current_playlist ();
	
	/* show the main window */
	gtk_widget_show (main_window);
	
	return;
}

static gboolean
cb_gimmix_key_press (GtkWidget   *widget,
					GdkEventKey *event,
					gpointer     userdata)
{
	gboolean result = FALSE;
	GtkWidget *button;
	gint state;

	if (event->type == GDK_KEY_PRESS) {
		switch (event->keyval) {
			case GDK_b: /* NEXT */
				cb_next_button_clicked (NULL, NULL);
				result = TRUE;
				break;
			case GDK_v: /* STOP */
				cb_stop_button_clicked (NULL, NULL);
				result = TRUE;
				break;
			case GDK_c: /* PLAY/PAUSE */
				cb_play_button_clicked (NULL, NULL);
				result = TRUE;
				break;
			case GDK_x: /* PLAY/PAUSE */
				cb_play_button_clicked (NULL, NULL);
				result = TRUE;
				break;
			case GDK_z: /* PREV */
				cb_prev_button_clicked (NULL, NULL);
				result = TRUE;
				break;
			case GDK_i: /* INFO */
				cb_info_button_clicked (NULL, NULL);
				result = TRUE;
				break;
			case GDK_r: /* REPEAT */
				button = glade_xml_get_widget(xml, "repeat_toggle");
				state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(button));
				if (state == TRUE)
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), FALSE);
				else
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), TRUE);
				result = TRUE;
				break;
			case GDK_s: /* SHUFFLE */
				button = glade_xml_get_widget(xml, "shuffle_toggle");
				state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(button));
				if (state == TRUE)
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), FALSE);
				else
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), TRUE);
				result = TRUE;
				break;
		}
	}
	return result;
}

static gboolean
gimmix_timer (void)
{
	gchar 	time[15];
	int 	new_status;
	float 	fraction;
	static	gboolean stop;

	new_status = gimmix_get_status (pub->gmo);
	
	if (song_is_changed == true && new_status == PLAY)
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
				gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), _("Stopped"));
				gimmix_show_ver_info ();
				stop = false;
			}
		}
		return TRUE;
	}
	else
	{
		GtkWidget 	*button;
		
		button = glade_xml_get_widget (xml, "play_button");
		status = new_status;
		if (status == PLAY)
		{
			gtk_image_set_from_stock (GTK_IMAGE(glade_xml_get_widget(xml, "image_play")), "gtk-media-pause", GTK_ICON_SIZE_BUTTON);
			gtk_tooltips_set_tip (play_button_tooltip, button, _("Pause <x or c>"), NULL);
			stop = true;
		}
		else if (status == PAUSE || status == STOP)
		{
			gtk_image_set_from_stock (GTK_IMAGE(glade_xml_get_widget(xml, "image_play")), "gtk-media-play", GTK_ICON_SIZE_BUTTON);
			gtk_tooltips_set_tip (play_button_tooltip, button, _("Play <x or c>"), NULL);
		}
		
		g_object_ref_sink (play_button_tooltip);
		return TRUE;
	}
}

static void
cb_prev_button_clicked (GtkWidget *widget, gpointer data)
{
	if (gimmix_prev(pub->gmo))
		gimmix_set_song_info ();

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
	if (gimmix_play(pub->gmo))
	{
		gtk_image_set_from_stock (GTK_IMAGE(glade_xml_get_widget(xml, "image_play")), "gtk-media-pause", GTK_ICON_SIZE_BUTTON);
		gimmix_set_song_info ();
	}
	else
	{
		gtk_image_set_from_stock (GTK_IMAGE(glade_xml_get_widget(xml, "image_play")), "gtk-media-play", GTK_ICON_SIZE_BUTTON);
	}
	
	return;
}

static void
cb_stop_button_clicked (GtkWidget *widget, gpointer data)
{
	if (gimmix_stop(pub->gmo))
	{
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.0);
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), _("Stopped"));
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
		mpd_player_set_repeat (pub->gmo, true);
	}
	else if (state == FALSE)
	{
		mpd_player_set_repeat (pub->gmo, false);
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
		mpd_player_set_random (pub->gmo, true);
	}
	else if (state == FALSE)
	{
		mpd_player_set_random (pub->gmo, false);
	}
	
	return;
}
		
static void
cb_pref_button_clicked (GtkWidget *widget, gpointer data)
{
	gimmix_prefs_dialog_show ();
	
	return;
}

static void
cb_info_button_clicked (GtkWidget *widget, gpointer data)
{
	gimmix_tag_editor_show ();
	
	return;
}

static void
cb_volume_scale_changed (GtkWidget *widget, gpointer data)
{
	GtkAdjustment *volume_adj;
	gint value;
	
	volume_adj = gtk_range_get_adjustment (GTK_RANGE(widget));

	value = gtk_adjustment_get_value (GTK_ADJUSTMENT(volume_adj));
	mpd_status_set_volume (pub->gmo, value);
	
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
	GtkAdjustment	*volume_adj;
	
	widget = glade_xml_get_widget (xml, "volume_scale");
	volume_adj = gtk_range_get_adjustment (GTK_RANGE(widget));
	volume = mpd_status_get_volume (pub->gmo);
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
	totaltime = mpd_status_get_total_song_time (pub->gmo);
	seektime = (gdouble)x/allocation.width;
	newtime = seektime * totaltime;
	if (gimmix_seek(pub->gmo, newtime))
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), seektime);

	return;
}

void
gimmix_window_visible_toggle (void)
{
	static int x;
	static int y;
	GtkWidget *window;

	window = glade_xml_get_widget (xml, "main_window");
	if( !GTK_WIDGET_VISIBLE (window) )
	{	
		gtk_window_move (GTK_WINDOW(window), x, y);
		gtk_widget_show (GTK_WIDGET(window));
		gtk_window_present (GTK_WINDOW(window));
	}
	else
	{	
		gtk_window_get_position (GTK_WINDOW(window), &x, &y);
		gtk_widget_hide (GTK_WIDGET(window));
	}

	return;
}

GtkWidget *
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
	
	song 			= gimmix_get_song_info (pub->gmo);
	window 			= glade_xml_get_widget (xml, "main_window");
	song_label 		= glade_xml_get_widget (xml,"song_label");
	artist_label 	= glade_xml_get_widget (xml,"artist_label");
	album_label 	= glade_xml_get_widget (xml,"album_label");
		
	if (song->title != NULL)
	{
		title = g_strdup_printf ("%s - %s", APPNAME, song->title);
		gtk_window_set_title (GTK_WINDOW(window), title);
		g_free (title);
		markup = g_markup_printf_escaped ("<span size=\"large\"weight=\"bold\"><i>%s</i></span>", song->title);
	}
	else
	{
		title = g_path_get_basename (song->file);
		markup = g_markup_printf_escaped ("<span size=\"large\"weight=\"bold\"><i>%s</i></span>", title);
		g_free (title);
		gtk_window_set_title (GTK_WINDOW(window), APPNAME);
	}
	gtk_label_set_markup (GTK_LABEL(song_label), markup);
	gtk_label_set_text (GTK_LABEL(artist_label), song->artist);
	gtk_label_set_text (GTK_LABEL(album_label), song->album);
	g_free (markup);

	if (strncasecmp(cfg_get_key_value(conf, "enable_systray"), "true", 4) == 0)
		gimmix_update_systray_tooltip (song);
	
	gimmix_free_song_info (song);
	
	return;
}

void
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
	gtk_label_set_text (GTK_LABEL(artist_label), APPURL);
	gtk_label_set_text (GTK_LABEL(album_label), NULL);
	gtk_window_set_title (GTK_WINDOW(window), APPNAME);
	gimmix_update_systray_tooltip (NULL);
	g_free (markup);
	g_free (appver);
	
	return;
}

static int
cb_gimmix_main_window_delete_event (GtkWidget *widget, gpointer data)
{
	if (strncasecmp(cfg_get_key_value(conf, "enable_systray"), "true", 4) == 0)
	{	
		gimmix_window_visible_toggle ();
		return 1;
	}
	
	/* stop playback on exit */
	if (strncasecmp(cfg_get_key_value(conf, "stop_on_exit"), "true", 4) == 0)
		gimmix_stop (pub->gmo);
		
	/* save window position */
	gimmix_save_window_pos ();
	
	return 0;
}

static void
gimmix_save_window_pos (void)
{
	GtkWidget *window;
	gint x,y;
	gchar xpos[4];
	gchar ypos[4];
	
	window = glade_xml_get_widget (xml, "main_window");
	gtk_window_get_position (GTK_WINDOW(window), &x, &y);
	sprintf (xpos, "%d", x);
	sprintf (ypos, "%d", y);
	cfg_add_key (&conf, "window_xpos", xpos);
	cfg_add_key (&conf, "window_ypos", ypos);
	gimmix_config_save ();
	
	return;
}

void
gimmix_interface_cleanup (void)
{	
	/* destroy the main window */
	GtkWidget *w = glade_xml_get_widget (xml, "main_window");
	if (w)
	gtk_widget_destroy (w);
	
	/* destroy system tray icon */
	gimmix_destroy_systray_icon ();
	
	return;
}
