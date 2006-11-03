#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "gimmix-core.h"
#include "gimmix.h"
#include <string.h>
#include <stdbool.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include <gtk/gtk.h>

void gimmix_playlist_init (void);
void gimmix_update_current_playlist (void);

/* Callbacks */
void on_dir_activated (GtkTreeView *);
void gimmix_current_playlist_play (GtkTreeView *);
void gimmix_current_playlist_right_click (GtkTreeView *, GdkEventButton *);

#endif
