#ifndef GIMMIX_CORE_H
#define GIMMIX_CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include "gimmix-config.h"

/* the SongInfo structure */
typedef struct songinfo
{
	char *title;
	char *artist;
	char *album;
	char *genre;
	char *file;
} SongInfo;

typedef enum { 	PLAY = 1,
				PAUSE,
				STOP
} GimmixStatus;
	
/* status flags */
/* set when song is changed */
bool song_is_changed;

/* set when playlist is changed */
bool playlist_is_changed;

/* set when voulume is changed */
bool volume_is_changed;

/* set when shuffle/repeat is toggled */
bool shuffle_is_changed;
bool repeat_is_changed;

/* create a mpd object and connect to mpd using the conf */
MpdObj * gimmix_mpd_connect (void);
void gimmix_disconnect (MpdObj *);

/* playback control */
bool gimmix_play (MpdObj *);
bool gimmix_stop (MpdObj *);
bool gimmix_prev (MpdObj *);
bool gimmix_next (MpdObj *);
bool gimmix_seek (MpdObj *, int);

/* get full image path */
char *gimmix_get_full_image_path (const char *);

/* Gets the information of currently playing song (artist, title, genre.etc)*/
SongInfo * gimmix_get_song_info (MpdObj *);

/* Free memory allocated by gimmix_get_song_info() */
void gimmix_free_song_info (SongInfo *);

void gimmix_get_progress_status (MpdObj *, float *, char *);

/* Check mpd status for PLAY/PAUSE/STOP */
GimmixStatus gimmix_get_status (MpdObj *);

/* repeat / shuffle status functions */
bool is_gimmix_repeat (MpdObj *);
bool is_gimmix_shuffle (MpdObj *);

/* Status changed callback */
/* Monitors song, volume, and playlist changes */
void gimmix_status_changed (MpdObj *, ChangedStatusType);

#endif
