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
	char title[80];
	char artist[80];
	char album[80];
	char genre[80];
	char file[255];
	int length;
	int bitrate;
	int id;
	int pos;
} SongInfo;

typedef enum { 	PLAY,
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
MpdObj * gimmix_mpd_connect (Conf *);
void gimmix_disconnect (MpdObj *);

/* playback control */
bool gimmix_play (MpdObj *);
bool gimmix_stop (MpdObj *);
bool gimmix_prev (MpdObj *);
bool gimmix_next (MpdObj *);
bool gimmix_seek (MpdObj *, int);
void gimmix_repeat (MpdObj *, bool);
void gimmix_shuffle (MpdObj *, bool);

/* voulme control */
int gimmix_get_volume (MpdObj *);
void gimmix_set_volume (MpdObj *, int);

/* Gets the information of currently playing song (artist, title, genre.etc)*/
SongInfo * gimmix_get_song_info (MpdObj *);

/* Free memory allocated by gimmix_get_song_info() */
void gimmix_free_song_info (SongInfo *);

void gimmix_get_progress_status (MpdObj *, float *, char *);
int gimmix_get_total_song_time (MpdObj *);
char * gimmix_get_song_length (SongInfo *);
char * gimmix_get_song_bitrate (SongInfo *);

/* Check mpd status for PLAY/PAUSE/STOP */
GimmixStatus gimmix_get_status (MpdObj *);

/* repeat / shuffle status functions */
bool is_gimmix_repeat (MpdObj *);
bool is_gimmix_shuffle (MpdObj *);

/* Status changed callback */
/* Monitors song, volume, and playlist changes */
void song_changed (MpdObj *, ChangedStatusType);

#endif
