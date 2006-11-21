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

#endif
