#ifndef INTERFACE_H
#define INTERFACE_H

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdbool.h>
#include <libnotify/notify.h>
#include "gimmix-core.h"

/* Main window widgets */
GtkStatusIcon *tray_icon;
NotifyNotification *notify;

/* Initializes default interface signals */
void gimmix_init (void);

/* Timer */
gboolean gimmix_timer (void);

/* Utility functions */
GtkWidget * get_image (const gchar *, GtkIconSize);
NotifyNotification * gimmix_notify_init (GtkStatusIcon *);
gchar * strip_filename (gchar *);
void gimmix_set_song_info (void);
void gimmix_show_ver_info (void);

/* Systray and Popup menu */
void gimmix_systray_icon_create (void);
void gimmix_systray_popup_menu (void);

/* Callbacks */
void gimmix_systray_popup_menu (void);
void on_prev_button_clicked (GtkWidget *, gpointer);
void on_next_button_clicked (GtkWidget *, gpointer);
void on_play_button_clicked (GtkWidget *, gpointer);
void on_stop_button_clicked (GtkWidget *, gpointer);
void on_info_button_clicked (GtkWidget *, gpointer);
void on_repeat_button_toggled (GtkToggleButton *, gpointer);
void on_shuffle_button_toggled (GtkToggleButton *, gpointer);
void on_prefs_button_clicked (GtkWidget *, gpointer);
void on_volume_scale_changed (GtkWidget *, gpointer data);
void on_preferences_apply (GtkWidget *, gpointer);
void gimmix_scroll_volume_slider (GtkWidget *, GdkEventScroll *);
void gimmix_update_volume (void);
void gimmix_progress_seek (GtkWidget *, GdkEvent *);
void gimmix_about_show (void);
void gimmix_window_visible (void);

/* Preferences dialog callbacks */
void on_systray_checkbox_toggled (GtkWidget *, gpointer);

#endif
