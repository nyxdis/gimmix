/*
 * gimmixcore.c
 *
 * Copyright (C) 2006 Priyank Gosalia
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Priyank Gosalia <priyankmg@gmail.com>
 */

#include "gimmix-core.h"

enum { 	PLAY,
		PAUSE,
		STOP
	};

MpdObj *
gimmix_mpd_connect (Conf *conf)
{
	MpdObj *mo;

	if (conf)
	{
		if ((conf->hostname!="") && (conf->port!=-1))
		{
			mo = mpd_new (conf->hostname, conf->port, conf->password);
			mpd_connect (mo);
			if (mpd_check_connected (mo))
			{
				mpd_signal_connect_status_changed (mo, (StatusChangedCallback)song_changed, NULL);
			return mo;
			}
		}
		else
		return NULL;
	}
	return NULL;
}

int
gimmix_get_status (MpdObj *mo)
{
	int status;
	mpd_status_update (mo);
	status = mpd_player_get_state (mo);
	
	if (status == MPD_PLAYER_PAUSE)
		return PAUSE;
	else if (status == MPD_PLAYER_PLAY)
		return PLAY;
	else if (status == MPD_PLAYER_STOP)
		return STOP;
	return -1;
}

bool
gimmix_play (MpdObj *mo)
{
	mpd_status_update (mo);
	if (mpd_playlist_get_playlist_length (mo))
	{
		int state;

		state = mpd_player_get_state (mo);

		if (state == MPD_PLAYER_PAUSE || state == MPD_PLAYER_STOP)
		{	
			mpd_player_play (mo);
			return true;
		}

		else if (state == MPD_PLAYER_PLAY)
		{
			mpd_player_pause (mo);
			return false;
		}
	}
	return false;
}

bool
gimmix_stop (MpdObj *mo)
{
	int state;
	state = gimmix_get_status (mo);

	if (state == PLAY || state == PAUSE)
	{
		mpd_player_stop (mo);
		return true;
	}
	return false;
}

bool
gimmix_prev (MpdObj *mo)
{
	int state;
	
	mpd_player_prev (mo);
	state = gimmix_get_status (mo);
	if (state == PLAY || state == PAUSE)
	{
		return true;
	}
	return false;
}

bool
gimmix_next (MpdObj *mo)
{
	int state;
	
	mpd_player_next (mo);
	state = gimmix_get_status (mo);
	if (state == PLAY || state == PAUSE)
	{
		return true;
	}
	return false;
}

void
gimmix_repeat (MpdObj *mo, bool set)
{
	if (!mo)
		return;

	mpd_player_set_repeat (mo, set);
	
	return;
}

void
gimmix_shuffle (MpdObj *mo, bool set)
{
	if (!mo)
		return;

	mpd_player_set_random (mo, set);
	
	return;
}

bool
is_gimmix_repeat (MpdObj *mo)
{
	int state;
	
	state = mpd_player_get_repeat (mo);
	if (state == 1)
		return TRUE;
	
	return FALSE;
}

bool
is_gimmix_shuffle (MpdObj *mo)
{
	int state;
	
	state = mpd_player_get_random (mo);
	if (state == 1)
		return TRUE;
	
	return FALSE;
}

bool
gimmix_seek (MpdObj *mo, int seektime)
{
	int state;
	state = gimmix_get_status (mo);

	if(state == PLAY || state == PAUSE)
	{
		mpd_player_seek(mo, seektime);
		return true;
	}

	return false;
}

void
gimmix_set_volume (MpdObj *mo, int vol)
{
	mpd_status_set_volume(mo, vol);
}

int
gimmix_get_volume (MpdObj *mo)
{
	int volume;

	volume = mpd_status_get_volume (mo);
	return volume;
}

int
gimmix_get_total_song_time (MpdObj *mo)
{
	int time;

	time = mpd_status_get_total_song_time (mo);
	return time;
}

SongInfo *
gimmix_get_song_info (MpdObj *mo)
{
	mpd_Song *ms;
	SongInfo *s = (SongInfo *)malloc(sizeof(SongInfo));
	
	mpd_status_update(mo);
	ms = mpd_playlist_get_current_song(mo);
	s->file 	= (ms->file) 	? strdup (ms->file) : NULL;
	s->title 	= (ms->title) 	? strdup (ms->title) : NULL;
	s->artist 	= (ms->artist) 	? strdup (ms->artist) : NULL;
	s->album 	= (ms->album) 	? strdup (ms->album) : NULL;
	s->genre 	= (ms->genre) 	? strdup (ms->genre) : NULL;
	s->length 	= ms->time;
	if (gimmix_get_status(mo) == PLAY)
		s->bitrate 	= mpd_status_get_bitrate (mo);
	else
		s->bitrate 	= -1;
	
	return s;
}

char *
gimmix_get_song_length (SongInfo *s)
{
	int time;
	char *length = malloc (10);
	
	time = s->length;
	snprintf (length, 10, "%02i:%02i", time/60, time%60);
	return length;
}

char *
gimmix_get_song_bitrate (SongInfo *s)
{
	int bitr;
	char *bitrate = malloc (10);
	
	bitr = s->bitrate;
	if (bitr != -1)
		snprintf (bitrate, 10, "%i Kbps", bitr);
	else
		return NULL;
	return bitrate;
};

void
gimmix_free_song_info (SongInfo *si)
{
	if (si != NULL)
	{
		if (si->title)
			free (si->title);
		if (si->artist)
			free (si->artist);
		if (si->album)
			free (si->album);
		if (si->file)
			free (si->file);
		if (si->genre)
			free (si->genre);
		si->length = -1;
		si->pos = -1;
		si->id = -1;
		free (si);
	}
	return;
}

void
gimmix_get_progress_status (MpdObj *mo, float *fraction, char *time)
{
	int state;
	int total, elapsed;
		
	state = mpd_player_get_state (mo);
	
	switch (state)
	{
		case MPD_PLAYER_PLAY:
		case MPD_PLAYER_PAUSE:
			mpd_status_update(mo);
			total = mpd_status_get_total_song_time (mo);
			elapsed = mpd_status_get_elapsed_song_time (mo);
			snprintf (time, 20, "%02i:%02i / %02i:%02i", elapsed/60,
					elapsed%60,
					total/60,
					total%60);
			*fraction = (float)((float)elapsed/(float)total);
			break;

		case MPD_PLAYER_STOP:
		case MPD_PLAYER_UNKNOWN:
			time = NULL;
			return;
	}
	return;
}

void
song_changed (MpdObj *mo, ChangedStatusType id)
{
	if (id&MPD_CST_SONGID)
		song_is_changed = true;
	else
		song_is_changed = false;
		
	if (id&MPD_CST_PLAYLIST)
		playlist_is_changed = true;
	else
		playlist_is_changed = false;
		
	if (id&MPD_CST_VOLUME)
		volume_is_changed = true;
	else
		volume_is_changed = false;
	
	if (id&MPD_CST_RANDOM)
		shuffle_is_changed = true;
	else
		shuffle_is_changed = false;
	
	if (id&MPD_CST_REPEAT)
		repeat_is_changed = true;
	else
		repeat_is_changed = false;
	

	return;
}

void
gimmix_disconnect (MpdObj *mo)
{
	if (mo != NULL || mpd_check_connected(mo))
	{
		mpd_free (mo);
	}
	
	return;
}
