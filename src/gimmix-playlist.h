#ifndef GIMMIX_PLAYLIST_H
#define GIMMIX_PLAYLIST_H

#include "gimmix-core.h"

#include <string.h>
#include <stdbool.h>
#include <gtk/gtk.h>

/* Initialize the playlist widgets and setup signals */
void gimmix_playlist_widgets_init (void);

/* Disable widgets */
void gimmix_playlist_disable_controls (void);

void gimmix_playlist_enable_controls (void);

/* Initialize and populate playlists */
void gimmix_playlist_init (void);

/* update current playlist depending on the mpd playlist */
void gimmix_update_current_playlist (MpdObj *mo, MpdData *pdata);

/* update library */
void gimmix_library_update (void);

#endif
