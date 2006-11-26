#ifndef GIMMIX_INTERFACE_H
#define GIMMIX_INTERFACE_H

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdbool.h>
#include <libnotify/notify.h>
#include "gimmix-core.h"

/* Initializes default interface signals */
void gimmix_init (void);

/* Sets the song info labels to reflect the song status */
void gimmix_set_song_info (void);

/* interface cleanup */
void gimmix_interface_cleanup (void);

NotifyNotification 	*notify;

/* create a new notification */
NotifyNotification *gimmix_create_notification (void);

/* Disable system tray icon */
void gimmix_disable_systray_icon (void);

/* Enable system tray icon */
void gimmix_enable_systray_icon (void);

/* Returns an image from stock id */
GtkWidget	*get_image (const gchar *, GtkIconSize);

#endif
