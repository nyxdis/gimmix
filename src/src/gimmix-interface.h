#ifndef GIMMIX_INTERFACE_H
#define GIMMIX_INTERFACE_H

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdbool.h>
#include "gimmix-core.h"
#include "gimmix-systray.h"

/* Initializes default interface signals */
void gimmix_init (void);

/* Sets the song info labels to reflect the song status */
void gimmix_set_song_info (void);

/* Toggles visibility of main window */
void gimmix_window_visible_toggle (void);

/* interface cleanup */
void gimmix_interface_cleanup (void);

/* Returns an image from stock id */
GtkWidget *get_image (const gchar *, GtkIconSize);

/* Display version info */
void gimmix_show_ver_info (void);

/* Save window position */
void gimmix_save_window_pos (void);

#endif
