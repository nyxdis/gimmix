/*
 * gimmix-interface.c
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

#include <glib.h>
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>
#include "gimmix-interface.h"
#include "gimmix-tooltip.h"
#include "gimmix-playlist.h"
#include "gimmix-tagedit.h"
#include "gimmix-metadata.h"
#include "gimmix-prefs.h"
#include "gimmix.h"

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#ifdef HAVE_LYRICS
#	include "gimmix-lyrics.h"
#endif

#ifdef HAVE_COVER_PLUGIN
#	include "gimmix-covers.h"
#endif

#define GIMMIX_APP_ICON  	"gimmix.png"

gint 			status;
GtkWidget		*main_window;
GtkWidget 		*progress = NULL;
GtkWidget		*shuffle_toggle_button;
GtkWidget		*repeat_toggle_button;
GtkWidget		*volume_window;
GtkWidget		*volume_scale;
GtkWidget		*volume_button;
GtkWidget		*volume_hscalebox;
GtkWidget		*volume_hscale;
GtkWidget		*song_label;
GtkWidget		*artist_label;
GtkWidget		*search_entry;
GtkWidget		*playlist_button;
GtkWidget		*playlist_box;
GtkWidget		*image_play;
GtkWidget		*play_button;
GtkWidget		*prev_button;
GtkWidget		*info_button;
GtkWidget		*next_button;
GtkWidget		*pref_button;
GtkWidget		*stop_button;
GtkWidget		*plcontrolshbox;
GtkTooltips 		*play_button_tooltip = NULL;

extern MpdObj 		*gmo;
extern GladeXML 	*xml;
extern ConfigFile	conf;
extern GimmixTooltip 	*tooltip;
extern GtkWidget	*current_playlist_treeview;

mpd_Song		*glob_song_info;

static void		gimmix_update_volume (void);
static void		gimmix_update_repeat (void);
static void		gimmix_update_shuffle (void);
static gboolean		inited = FALSE;

static gboolean		is_user_searching (void);
static gboolean 	gimmix_timer (void);

/* Callbacks */
static gboolean cb_gimmix_main_window_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean cb_gimmix_main_window_configure_event (GtkWidget *widget, GdkEventConfigure *event, gpointer data);
static void	cb_play_button_clicked 	(GtkWidget *widget, gpointer data);
static void	cb_stop_button_clicked 	(GtkWidget *widget, gpointer data);
static void	cb_next_button_clicked 	(GtkWidget *widget, gpointer data);
static void	cb_prev_button_clicked 	(GtkWidget *widget, gpointer data);
static gboolean cb_info_button_press 	(GtkWidget *widget, GdkEventButton *event, gpointer data);
static void 	cb_pref_button_clicked 	(GtkWidget *widget, gpointer data);
static void 	cb_repeat_button_toggled (GtkToggleButton *button, gpointer data);
static void 	cb_shuffle_button_toggled (GtkToggleButton *button, gpointer data);
static gboolean	cb_playlist_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data);

static void 	cb_gimmix_progress_seek (GtkWidget *widget, GdkEvent *event);
static void 	cb_volume_scale_changed (GtkWidget *widget, gpointer data);
static void	cb_volume_slider_scroll (GtkWidget *widget, GdkEventScroll *event);
static void 	cb_volume_button_clicked (GtkWidget *widget, gpointer data);
static void	gimmix_reposition_volume_window (GtkWidget *volwindow);
static gboolean cb_gimmix_key_press(GtkWidget *widget, GdkEventKey *event, gpointer userdata);

/* mpd callbacks */
static void 	gimmix_status_changed (MpdObj *mo, ChangedStatusType id);

static void
gimmix_update_global_song_info (void)
{
	mpd_Song *tsi = NULL;

	tsi = mpd_playlist_get_current_song (gmo);
	if (tsi!=NULL)
	{
		glob_song_info = tsi;
	}
	
	return;
}

static void
gimmix_status_changed (MpdObj *mo, ChangedStatusType id)
{
	gimmix_update_global_song_info ();
	
	if (!(id&MPD_CST_STATE) && (id&MPD_CST_SONGID || id&MPD_CST_DATABASE))
	{
		gimmix_update_current_playlist (mo, mpd_playlist_get_changes(mo,0));
		#ifdef HAVE_COVER_PLUGIN
		g_thread_create ((GThreadFunc)gimmix_covers_plugin_update_cover,
				FALSE,
				FALSE,
				NULL);
		
		#endif
		gimmix_set_song_info ();
		#ifdef HAVE_LYRICS
		g_thread_create ((GThreadFunc)gimmix_lyrics_plugin_update_lyrics,
				NULL,
				FALSE,
				NULL);
		#endif
		
		return;
	}

	if (id&MPD_CST_STATE)
	{
		int state = mpd_player_get_state (gmo);
		if (state == MPD_PLAYER_PLAY)
		{
			gtk_image_set_from_stock (GTK_IMAGE(image_play), "gtk-media-pause", GTK_ICON_SIZE_MENU);
			gtk_tooltips_set_tip (play_button_tooltip, play_button, _("Pause <x or c>"), NULL);
			
			#ifdef HAVE_COVER_PLUGIN
			if (!strncasecmp(cfg_get_key_value(conf,"coverart_enable"),"true",4))
			{
				g_thread_create ((GThreadFunc)gimmix_covers_plugin_update_cover,
						FALSE,
						FALSE,
						NULL);
			}
			#endif
			gimmix_set_song_info ();
			#ifdef HAVE_LYRICS
			g_thread_create ((GThreadFunc)gimmix_lyrics_plugin_update_lyrics ,
				NULL,
				FALSE,
				NULL);
			#endif
			
		}
		else
		if (state == MPD_PLAYER_PAUSE)
		{
			gtk_image_set_from_stock (GTK_IMAGE(image_play), "gtk-media-play", GTK_ICON_SIZE_MENU);
			gtk_tooltips_set_tip (play_button_tooltip, play_button, _("Play <x or c>"), NULL);
		}
		else
		if (state == MPD_PLAYER_STOP)
		{
			//g_print ("stopped\n");
			gimmix_show_ver_info ();
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.0);
			gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), _("Stopped"));
			if (strncasecmp(cfg_get_key_value(conf, "enable_systray"), "true", 4) == 0)
			{
				gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(tooltip->progressbar), 0.0);
				gtk_progress_bar_set_text (GTK_PROGRESS_BAR(tooltip->progressbar), _("Stopped"));
			}
			
			#ifdef HAVE_COVER_PLUGIN
			if (!strncasecmp(cfg_get_key_value(conf,"coverart_enable"),"true",4))
			{
				g_thread_create ((GThreadFunc)gimmix_covers_plugin_update_cover,
						(gpointer)TRUE,
						FALSE,
						NULL);
			}
			#endif
			gtk_image_set_from_stock (GTK_IMAGE(image_play), "gtk-media-play", GTK_ICON_SIZE_MENU);
			gtk_tooltips_set_tip (play_button_tooltip, play_button, _("Play <x or c>"), NULL);
			gimmix_update_current_playlist (mo, mpd_playlist_get_changes(mo,0));
			return;
		}
		g_object_ref_sink (play_button_tooltip);
		gimmix_update_current_playlist (mo, mpd_playlist_get_changes(mo,0));
	}
	
	if (id&MPD_CST_PLAYLIST)
		gimmix_update_current_playlist (mo, mpd_playlist_get_changes(mo,0));

	if (id&MPD_CST_VOLUME)
		gimmix_update_volume ();

	if (id&MPD_CST_RANDOM)
		gimmix_update_shuffle ();

	if (id&MPD_CST_REPEAT)
		gimmix_update_repeat ();

	return;
}

static void
gimmix_toggle_playlist_show (gboolean show)
{
	static int height;
	static int width;

	if (show)
	{
		gtk_widget_show (volume_hscalebox);
		gtk_widget_hide (volume_button);
		gtk_widget_show (GTK_WIDGET(playlist_box));
		if (height>0 && width>0)
			gtk_window_resize (GTK_WINDOW(main_window), width, height);
		gtk_window_get_size (GTK_WINDOW(main_window), &width, &height);
		gtk_window_set_resizable (GTK_WINDOW(main_window), TRUE);
	}
	else
	{
		gint w;
		gtk_widget_hide (volume_hscalebox);
		gtk_widget_show (volume_button);
		w = main_window->allocation.width;
		gtk_widget_hide (GTK_WIDGET(playlist_box));
		gtk_window_get_size (GTK_WINDOW(main_window), &width, &height);
		gtk_window_resize (GTK_WINDOW(main_window), 1, 1);
		gtk_window_set_resizable (GTK_WINDOW(main_window), FALSE);
	}
	
	return;
}

static gboolean
cb_playlist_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data)
{	
	if (event != NULL)
	if (event->button != 1)
		return FALSE;
	
	if (!GTK_WIDGET_VISIBLE(playlist_box))
		gimmix_toggle_playlist_show (TRUE);
	else
		gimmix_toggle_playlist_show (FALSE);

	return TRUE;
}

void
gimmix_interface_widgets_init (void)
{
	GtkWidget 		*widget;
	GtkWidget		*progressbox;
	GtkAdjustment		*vol_adj;
	GdkPixbuf		*app_icon;
	gchar			*path;
	
	if (inited)
	{
		return;
	}
	/* Set the application icon */
	main_window = glade_xml_get_widget (xml, "main_window");
	g_signal_connect (G_OBJECT(main_window), "delete-event", G_CALLBACK(cb_gimmix_main_window_delete_event), NULL);
	gtk_window_set_default_size (GTK_WINDOW(main_window), -1, 80);
	gtk_window_resize (GTK_WINDOW(main_window), atoi(cfg_get_key_value(conf, "window_width")), atoi(cfg_get_key_value(conf, "window_height")));
	gtk_window_move (GTK_WINDOW(main_window), atoi(cfg_get_key_value(conf, "window_xpos")), atoi(cfg_get_key_value(conf, "window_ypos")));
	path = gimmix_get_full_image_path (GIMMIX_APP_ICON);
	app_icon = gdk_pixbuf_new_from_file_at_size (path, 48, 48, NULL);
	gtk_window_set_icon (GTK_WINDOW(main_window), app_icon);
	g_object_unref (app_icon);
	g_free (path);
	
	/* connect the key press signal */
	g_signal_connect (G_OBJECT(main_window), "key-press-event", G_CALLBACK(cb_gimmix_key_press), NULL);
	
	g_signal_connect (G_OBJECT(main_window), "configure-event", G_CALLBACK(cb_gimmix_main_window_configure_event), (gpointer)glade_xml_get_widget(xml, "volume_window"));
	
	/* connect the destroy signal */
	g_signal_connect(G_OBJECT(main_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	
	gtk_image_set_from_icon_name (GTK_IMAGE(glade_xml_get_widget(xml, "image_volume")), "stock_volume", GTK_ICON_SIZE_MENU);
	gtk_image_set_from_icon_name (GTK_IMAGE(glade_xml_get_widget(xml, "image_fmodevolume")), "stock_volume", GTK_ICON_SIZE_MENU);
	
	g_signal_connect (G_OBJECT(glade_xml_get_widget (xml, "prev_button")), "clicked", G_CALLBACK(cb_prev_button_clicked), NULL);

	g_signal_connect (G_OBJECT(glade_xml_get_widget (xml, "play_button")), "clicked", G_CALLBACK(cb_play_button_clicked), NULL);
	
	g_signal_connect (G_OBJECT(glade_xml_get_widget (xml, "next_button")), "clicked", G_CALLBACK(cb_next_button_clicked), NULL);
	
	g_signal_connect (G_OBJECT(glade_xml_get_widget (xml, "stop_button")), "clicked", G_CALLBACK(cb_stop_button_clicked), NULL);
	
	g_signal_connect (G_OBJECT(glade_xml_get_widget (xml, "pref_button")), "clicked", G_CALLBACK(cb_pref_button_clicked), NULL);
	
	shuffle_toggle_button = glade_xml_get_widget (xml, "shuffle_toggle");
	repeat_toggle_button = glade_xml_get_widget (xml, "repeat_toggle");
	volume_window = glade_xml_get_widget (xml, "volume_window");
	volume_scale = glade_xml_get_widget (xml, "volume_scale");
	volume_hscalebox = glade_xml_get_widget (xml, "volume_hscalebox");
	volume_hscale = glade_xml_get_widget (xml, "volume_hscale");
	volume_button = glade_xml_get_widget (xml, "volume_button");
	playlist_button = glade_xml_get_widget (xml, "playlist_button");
	playlist_box = glade_xml_get_widget (xml, "playlistbox");
	song_label = glade_xml_get_widget (xml, "song_label");
	artist_label = glade_xml_get_widget (xml, "artist_label");
	search_entry = glade_xml_get_widget (xml, "search_label");
	play_button = glade_xml_get_widget (xml, "play_button");
	image_play = glade_xml_get_widget (xml, "image_play");
	plcontrolshbox = glade_xml_get_widget (xml, "plcontrolshbox");
	next_button = glade_xml_get_widget (xml, "next_button");
	prev_button = glade_xml_get_widget (xml, "prev_button");
	pref_button = glade_xml_get_widget (xml, "pref_button");
	info_button = glade_xml_get_widget (xml, "info_button");
	stop_button = glade_xml_get_widget (xml, "stop_button");
	
	/* set icons for buttons */
	gtk_image_set_from_stock (GTK_IMAGE(gtk_bin_get_child(GTK_BIN(prev_button))), "gtk-media-previous", GTK_ICON_SIZE_MENU);
	gtk_image_set_from_stock (GTK_IMAGE(gtk_bin_get_child(GTK_BIN(play_button))), "gtk-media-play", GTK_ICON_SIZE_MENU);
	gtk_image_set_from_stock (GTK_IMAGE(gtk_bin_get_child(GTK_BIN(next_button))), "gtk-media-next", GTK_ICON_SIZE_MENU);
	gtk_image_set_from_stock (GTK_IMAGE(gtk_bin_get_child(GTK_BIN(stop_button))), "gtk-media-stop", GTK_ICON_SIZE_MENU);
	gtk_image_set_from_stock (GTK_IMAGE(gtk_bin_get_child(GTK_BIN(info_button))), "gtk-info", GTK_ICON_SIZE_MENU);
	gtk_image_set_from_stock (GTK_IMAGE(glade_xml_get_widget(xml,"image_prefs")), "gtk-preferences", GTK_ICON_SIZE_MENU);
	gtk_image_set_from_stock (GTK_IMAGE(gtk_bin_get_child(GTK_BIN(playlist_button))), "gtk-justify-fill", GTK_ICON_SIZE_MENU);
	gtk_image_set_from_icon_name (GTK_IMAGE(gtk_bin_get_child(GTK_BIN(shuffle_toggle_button))), "stock_shuffle", GTK_ICON_SIZE_MENU);
	gtk_image_set_from_icon_name (GTK_IMAGE(gtk_bin_get_child(GTK_BIN(repeat_toggle_button))), "stock_repeat", GTK_ICON_SIZE_MENU);

	g_signal_connect (G_OBJECT(playlist_button), "button-press-event", G_CALLBACK(cb_playlist_button_press), NULL);

	if (is_gimmix_repeat (gmo))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(repeat_toggle_button), TRUE);
	g_signal_connect (G_OBJECT(repeat_toggle_button), "toggled", G_CALLBACK(cb_repeat_button_toggled), NULL);
	
	if (is_gimmix_shuffle (gmo))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(shuffle_toggle_button), TRUE);
	g_signal_connect (G_OBJECT(shuffle_toggle_button), "toggled", G_CALLBACK(cb_shuffle_button_toggled), NULL);

	widget = glade_xml_get_widget (xml, "info_button");
	g_signal_connect (G_OBJECT(widget), "button-release-event", G_CALLBACK(cb_info_button_press), NULL);

	g_signal_connect (G_OBJECT(volume_scale), "value_changed", G_CALLBACK(cb_volume_scale_changed), NULL);
	g_signal_connect (G_OBJECT(volume_scale), "scroll_event", G_CALLBACK(cb_volume_slider_scroll), NULL);
	g_signal_connect (G_OBJECT(volume_hscale), "scroll_event", G_CALLBACK(cb_volume_slider_scroll), NULL);
	vol_adj = gtk_range_get_adjustment (GTK_RANGE(volume_scale));
	gtk_range_set_adjustment (GTK_RANGE(volume_hscale), vol_adj);
	gtk_adjustment_set_value (GTK_ADJUSTMENT(vol_adj), mpd_status_get_volume (gmo));
	g_signal_connect (G_OBJECT(volume_button), "clicked", G_CALLBACK(cb_volume_button_clicked), volume_window);
	g_signal_connect (G_OBJECT(volume_button), "scroll_event", G_CALLBACK(cb_volume_slider_scroll), NULL);

	progress = glade_xml_get_widget (xml,"progress");
	progressbox = glade_xml_get_widget (xml,"progress_event_box");
	g_signal_connect (G_OBJECT(progressbox), "button_press_event", G_CALLBACK(cb_gimmix_progress_seek), NULL);
	
	play_button_tooltip = gtk_tooltips_new ();
	
	if (strncasecmp(cfg_get_key_value(conf, "enable_systray"), "true", 4) == 0)
	{
		gimmix_create_systray_icon ();
	}
	if (strncasecmp(cfg_get_key_value(conf, "full_view_mode"), "true", 4) == 0)
	{
		gtk_widget_show (playlist_box);
		gimmix_toggle_playlist_show (TRUE);
	}
	else
	{
		gtk_widget_hide (playlist_box);
		gimmix_toggle_playlist_show (FALSE);
	}
	
	/* initialize metadata */
	gimmix_metadata_init ();
	
	#ifdef HAVE_COVER_PLUGIN
	gimmix_covers_plugin_init ();
	#else
	widget = glade_xml_get_widget (xml, "gimmix_plc_image_frame");
	gtk_widget_hide (widget);
	#endif
	
	#ifdef HAVE_LYRICS
	gimmix_lyrics_plugin_init ();
	#else
	widget = glade_xml_get_widget (xml, "lyrics_container");
	gtk_widget_hide (widget);
	#endif
	
	widget = glade_xml_get_widget (xml, "metadata_container");
	#ifdef HAVE_COVER_PLUGIN
	gtk_widget_show (widget);
	#elif defined(HAVE_LYRICS)
	gtk_widget_show (widget);
	#else
	gtk_widget_hide (widget);
	#endif
	
	/* initialize preferences dialog */
	gimmix_prefs_init ();
	
	/* playlist widgets */
	gimmix_playlist_widgets_init ();
	
	/* tag editor widgets */
	gimmix_tag_editor_widgets_init ();
	
	/* show version info */
	gimmix_show_ver_info ();
	
	/* show the main window */
	gtk_widget_show (main_window);
	
	#ifdef HAVE_COVER_PLUGIN
	if (!strncasecmp(cfg_get_key_value(conf,"coverart_enable"),"true",4))
	{
		g_thread_create ((GThreadFunc)gimmix_covers_plugin_update_cover,
				(gpointer)TRUE,
				FALSE,
				NULL);
	}
	#endif
}

void
gimmix_interface_disable_controls (void)
{
	gtk_widget_set_sensitive (repeat_toggle_button, FALSE);
	gtk_widget_set_sensitive (shuffle_toggle_button, FALSE);
	gtk_widget_set_sensitive (play_button, FALSE);
	gtk_widget_set_sensitive (stop_button, FALSE);
	gtk_widget_set_sensitive (next_button, FALSE);
	gtk_widget_set_sensitive (prev_button, FALSE);
	gtk_widget_set_sensitive (info_button, FALSE);
	gtk_widget_set_sensitive (pref_button, FALSE);
	gtk_widget_set_sensitive (volume_button, FALSE);
	gtk_widget_set_sensitive (volume_hscalebox, FALSE);
	
	gimmix_playlist_disable_controls ();
	gimmix_metadata_disable_controls ();
}

void
gimmix_interface_enable_controls (void)
{
	gtk_widget_set_sensitive (repeat_toggle_button, TRUE);
	gtk_widget_set_sensitive (shuffle_toggle_button, TRUE);
	gtk_widget_set_sensitive (play_button, TRUE);
	gtk_widget_set_sensitive (stop_button, TRUE);
	gtk_widget_set_sensitive (next_button, TRUE);
	gtk_widget_set_sensitive (prev_button, TRUE);
	gtk_widget_set_sensitive (info_button, TRUE);
	gtk_widget_set_sensitive (pref_button, TRUE);
	gtk_widget_set_sensitive (volume_button, TRUE);
	gtk_widget_set_sensitive (volume_hscalebox, TRUE);
	
	gimmix_playlist_enable_controls ();
	gimmix_metadata_enable_controls ();
}

void
gimmix_init (void)
{
	if (!inited)
	{
		inited = TRUE;
		mpd_status_update (gmo);
		status = mpd_player_get_state (gmo);
		gimmix_update_global_song_info ();
		
		if (status == MPD_PLAYER_PLAY)
		{
			gimmix_set_song_info ();
			status = -1;
			gtk_image_set_from_stock (GTK_IMAGE(image_play), "gtk-media-pause", GTK_ICON_SIZE_MENU);
			gtk_tooltips_set_tip (play_button_tooltip, play_button, _("Pause <x or c>"), NULL);
		}
		else if (status == MPD_PLAYER_PAUSE)
		{
			gimmix_set_song_info ();
		}
		else if (status == MPD_PLAYER_STOP)
		{
			gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), _("Stopped"));
			if (gimmix_config_get_bool("enable_systray"))
				gtk_progress_bar_set_text (GTK_PROGRESS_BAR(tooltip->progressbar), _("Stopped"));
			gimmix_show_ver_info ();
		}
		gtk_adjustment_set_value (GTK_ADJUSTMENT(gtk_range_get_adjustment(GTK_RANGE(volume_scale))), mpd_status_get_volume (gmo));
		gimmix_playlist_init ();
		/* check if library needs to be updated on startup */
		if (gimmix_config_get_bool("update_on_startup"))
			gimmix_library_update ();
		
		g_object_unref (xml);
	}
	
	g_timeout_add (300, (GSourceFunc)gimmix_timer, NULL);
	
	/* update current playlist */
	gimmix_update_current_playlist (gmo, mpd_playlist_get_changes(gmo,0));

	/* set song info */
	status = mpd_player_get_state (gmo);
	if (status == MPD_PLAYER_PLAY || status == MPD_PLAYER_PAUSE)
	{
		gimmix_set_song_info ();
	}
	
	#ifdef HAVE_COVER_PLUGIN
	if (gimmix_config_get_bool("coverart_enable"))
	{
		g_thread_create ((GThreadFunc)gimmix_covers_plugin_update_cover,
				FALSE,
				FALSE,
				NULL);
	}
	
	#endif
	#ifdef HAVE_LYRICS
	if (status == MPD_PLAYER_PLAY || status == MPD_PLAYER_PAUSE)
	{
		g_thread_create ((GThreadFunc)gimmix_lyrics_plugin_update_lyrics,
				NULL,
				FALSE,
				NULL);
	}
	#endif

	/* connect the main mpd callbacks */
	mpd_signal_connect_status_changed (gmo, (StatusChangedCallback)gimmix_status_changed, NULL);
	//mpd_signal_connect_error (gmo, (ErrorCallback)gimmix_mpd_error, NULL);
	
	return;
}

static gboolean
cb_gimmix_key_press (GtkWidget   *widget,
		GdkEventKey *event,
		gpointer     userdata)
{
	gboolean result = FALSE;
	gint state;
	
	if (is_user_searching())
		return result;

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
				cb_info_button_press (NULL, NULL, NULL);
				result = TRUE;
				break;
			case GDK_r: /* REPEAT */
				state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(repeat_toggle_button));
				if (state == TRUE)
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(repeat_toggle_button), FALSE);
				else
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(repeat_toggle_button), TRUE);
				result = TRUE;
				break;
			case GDK_s: /* SHUFFLE */
				state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(shuffle_toggle_button));
				if (state == TRUE)
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(shuffle_toggle_button), FALSE);
				else
					gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(shuffle_toggle_button), TRUE);
				result = TRUE;
				break;
			case GDK_l: /* TOGGLE DISPLAY PLAYLIST */
				cb_playlist_button_press (NULL, NULL, NULL);
				break;
		}
	}
	return result;
}
	
static gboolean
gimmix_timer (void)
{
	gchar 	time[32] = "";
	int 	new_status;
	float 	fraction;
	if (mpd_check_connected(gmo)==FALSE)
	{
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.0);
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), "");
		gimmix_show_ver_info ();
		#ifdef HAVE_COVER_PLUGIN
		if (gimmix_config_get_bool("coverart_enable"))
		{
			g_thread_create ((GThreadFunc)gimmix_covers_plugin_update_cover,
					(gpointer)TRUE, /* set default cover */
					FALSE,
					NULL);
		}
		#endif
		return FALSE;
	}
	if (mpd_check_connected(gmo))
	{
		mpd_status_update (gmo);
		new_status = mpd_player_get_state (gmo);
	}
	else
	{
		return FALSE;
	}
	
	if (status == new_status)
	{
		if (status == MPD_PLAYER_PLAY || status == MPD_PLAYER_PAUSE)
		{
			gimmix_get_progress_status (gmo, &fraction, time);
			if (fraction >= 0.0 && fraction <= 1.0)
				gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), fraction);
			gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), time);
			
			/* Update the system tray tooltip progress bar */
			if (gimmix_config_get_bool("enable_systray"))
				if (gimmix_config_get_bool("enable_notification"))
				{
					if (fraction >= 0.0 && fraction <= 1.0)
						gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(tooltip->progressbar), fraction);
					gtk_progress_bar_set_text (GTK_PROGRESS_BAR(tooltip->progressbar), time);
				}
		}
		return TRUE;
	}
	else
	{
		status = new_status;
		return TRUE;
	}
}

static void
cb_prev_button_clicked (GtkWidget *widget, gpointer data)
{
	gimmix_prev (gmo);

	return;
}

static void
cb_next_button_clicked (GtkWidget *widget, gpointer data)
{
	gimmix_next (gmo);
	
	return;
}

static void
cb_play_button_clicked (GtkWidget *widget, gpointer data)
{
	gimmix_play (gmo);
	return;
}

static void
cb_stop_button_clicked (GtkWidget *widget, gpointer data)
{
	gimmix_stop (gmo);
	
	return;
}

static void
cb_volume_button_clicked (GtkWidget *widget, gpointer data)
{
	if (GTK_WIDGET_VISIBLE (data))
		gtk_widget_hide (data);
	else
		gtk_widget_show (data);
		gimmix_reposition_volume_window (data);
	return;
}

static void
gimmix_reposition_volume_window (GtkWidget *volwindow)
{
		if (volwindow == NULL || !GTK_WIDGET_VISIBLE(volwindow))
		return;
		
		gint x, y;
		gdk_window_get_origin (volume_button->window, &x, &y);
		x += (volume_button->allocation.x);
		y += (volume_button->allocation.y + volume_button->allocation.height);
		gtk_window_resize (GTK_WINDOW(volwindow), volume_button->allocation.width, 1);
		gtk_window_move (GTK_WINDOW(volwindow), x, y);
		
		return;
}

static void
cb_repeat_button_toggled (GtkToggleButton *button, gpointer data)
{
	gboolean state;
	
	state = gtk_toggle_button_get_active (button);
	if (state == TRUE)
	{
		mpd_player_set_repeat (gmo, true);
	}
	else if (state == FALSE)
	{
		mpd_player_set_repeat (gmo, false);
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
		mpd_player_set_random (gmo, true);
	}
	else if (state == FALSE)
	{
		mpd_player_set_random (gmo, false);
	}
	
	return;
}
		
static void
cb_pref_button_clicked (GtkWidget *widget, gpointer data)
{
	gimmix_prefs_dialog_show ();
	
	return;
}

static gboolean
cb_info_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	if (event != NULL)
	if (event->button != 1)
		return FALSE;
	
	gimmix_tag_editor_show ();
	
	return TRUE;
}

static void
cb_volume_scale_changed (GtkWidget *widget, gpointer data)
{
	GtkAdjustment *volume_adj;
	gint value;
	
	volume_adj = gtk_range_get_adjustment (GTK_RANGE(widget));

	value = gtk_adjustment_get_value (GTK_ADJUSTMENT(volume_adj));
	mpd_status_set_volume (gmo, value);
	
	return;
}

static void
cb_volume_slider_scroll (GtkWidget *widget, GdkEventScroll *event)
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
gimmix_update_repeat (void)
{	
	if (is_gimmix_repeat (gmo))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(repeat_toggle_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(repeat_toggle_button), FALSE);
	
	return;
}

static void
gimmix_update_shuffle (void)
{
	if (is_gimmix_shuffle (gmo))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(shuffle_toggle_button), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(shuffle_toggle_button), FALSE);
	
	return;
}

static void
gimmix_update_volume ()
{
	gint 		volume;
	GtkAdjustment	*volume_adj;
	
	volume_adj = gtk_range_get_adjustment (GTK_RANGE(volume_scale));
	volume = mpd_status_get_volume (gmo);
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
	totaltime = mpd_status_get_total_song_time (gmo);
	seektime = (gdouble)x/allocation.width;
	newtime = seektime * totaltime;

	gimmix_seek (gmo, newtime);

	return;
}

void
gimmix_window_visible_toggle (void)
{
	static int x;
	static int y;

	if( !GTK_WIDGET_VISIBLE (main_window) )
	{	
		gtk_window_move (GTK_WINDOW(main_window), x, y);
		gtk_widget_show (GTK_WIDGET(main_window));
		gtk_window_present (GTK_WINDOW(main_window));
	}
	else
	{	
		gtk_window_get_position (GTK_WINDOW(main_window), &x, &y);
		/* hide the volume window if not hidden */
		gtk_widget_hide (GTK_WIDGET(volume_window));
		gtk_widget_hide (GTK_WIDGET(main_window));
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
	mpd_Song	*song = NULL;
	
	song = mpd_playlist_get_current_song (gmo);
	
	if (song == NULL)
	{
		g_print ("song = NULL\n");
		return;
	}
	
	if (song->title != NULL)
	{
		title = g_strdup_printf ("%s - %s", "Gimmix", song->title);
		gtk_window_set_title (GTK_WINDOW(main_window), title);
		g_free (title);
		markup = g_markup_printf_escaped ("<span size=\"x-large\"weight=\"bold\">%s</span>", song->title);
	}
	else
	{
		title = g_path_get_basename (song->file);
		gimmix_strip_file_ext (title);
		markup = g_markup_printf_escaped ("<span size=\"x-large\"weight=\"bold\">%s</span>", title);
		g_free (title);
		gtk_window_set_title (GTK_WINDOW(main_window), "Gimmix");
	}
	gtk_label_set_markup (GTK_LABEL(song_label), markup);
	if (song->artist != NULL)
	{
		gchar *string;
		string = g_markup_printf_escaped ("<i>by %s</i>", song->artist);
		gtk_label_set_markup (GTK_LABEL(artist_label), string);
		g_free (string);
	}
	else
	gtk_label_set_text (GTK_LABEL(artist_label), NULL);
	
	g_free (markup);

	if (strncasecmp(cfg_get_key_value(conf, "enable_systray"), "true", 4) == 0)
		gimmix_update_systray_tooltip (song);
	
	return;
}

void
gimmix_show_ver_info (void)
{
	gchar 		*markup;
	gchar 		*appver;

	appver = g_strdup_printf ("%s %s", APPNAME, VERSION);
	markup = g_markup_printf_escaped ("<span size=\"x-large\"weight=\"bold\">%s</span>", appver);
	gtk_label_set_markup (GTK_LABEL(song_label), markup);
	gtk_label_set_text (GTK_LABEL(artist_label), "");
	gtk_window_set_title (GTK_WINDOW(main_window), APPNAME);
	gimmix_update_systray_tooltip (NULL);
	g_free (markup);
	g_free (appver);

	return;
}

static gboolean
cb_gimmix_main_window_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	if (gimmix_config_get_bool("enable_systray"))
	{
		gimmix_window_visible_toggle ();
		return TRUE;
	}
	
	/* stop playback on exit */
	if (gimmix_config_get_bool("stop_on_exit"))
		gimmix_stop (gmo);
		
	/* save window position and mode */
	gimmix_save_window_pos ();
	
	return FALSE;
}

static gboolean cb_gimmix_main_window_configure_event (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	if (!GTK_WIDGET_VISIBLE(data))
		return FALSE;
	
	gimmix_reposition_volume_window (data);
	return FALSE;
}


void
gimmix_save_window_pos (void)
{
	gint x,y;
	gchar xpos[4];
	gchar ypos[4];
	gchar width[4];
	gchar height[4];
	
	/* save position and geometry */
	gtk_window_get_position (GTK_WINDOW(main_window), &x, &y);
	sprintf (xpos, "%d", x);
	sprintf (ypos, "%d", y);
	gtk_window_get_size (GTK_WINDOW(main_window), &x, &y);
	sprintf (width, "%d", x);
	sprintf (height, "%d", y);
	
	cfg_add_key (&conf, "window_xpos", xpos);
	cfg_add_key (&conf, "window_ypos", ypos);
	cfg_add_key (&conf, "window_width", width);
	cfg_add_key (&conf, "window_height", height);
	
	/* save mode */
	if (GTK_WIDGET_VISIBLE (GTK_WIDGET(playlist_box)))
		cfg_add_key (&conf, "full_view_mode", "true");
	else
		cfg_add_key (&conf, "full_view_mode", "false");
		
	gimmix_config_save ();
	
	return;
}

void
gimmix_interface_cleanup (void)
{	
	/* destroy system tray icon */
	gimmix_destroy_systray_icon ();
	
	#if HAVE_COVERS_PLUGIN
	gimmix_covers_plugin_cleanup ();
	#endif

	return;
}

static gboolean
is_user_searching (void)
{
	if (GTK_WIDGET_HAS_FOCUS(search_entry))
		return TRUE;
	
	return FALSE;
}


