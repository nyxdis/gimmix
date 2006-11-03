#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "gimmix-core.h"
#include "gimmix.h"
#include <string.h>
#include <stdbool.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include <gtk/gtk.h>

/* Initialize the playlist interface, file browser and setup signals */
void gimmix_playlist_init (void);

/* update current playlist depending on the mpd playlist */
void gimmix_update_current_playlist (void);

#endif
