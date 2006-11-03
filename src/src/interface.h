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

/* Sets the song info labels to reflect the song status */
void gimmix_set_song_info (void);

/* notification init */
NotifyNotification * gimmix_notify_init (GtkStatusIcon *);

#endif
