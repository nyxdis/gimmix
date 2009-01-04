/*
 * gimmix-core.c
 *
 * Copyright (C) 2006-2009 Priyank Gosalia
 *
 * Gimmix is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * Gimmix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with Gimmix; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Priyank Gosalia <priyankmg@gmail.com>
 */

#include "gimmix-core.h"
#include "gimmix.h"

extern ConfigFile 	conf;

GimmixStatus
gimmix_get_status (MpdObj *mo)
{
	int status;
	if (!mo || !mpd_check_connected(mo))
		return ERROR;
	
	mpd_status_queue_update (mo);
	mpd_status_update (mo);
	status = mpd_player_get_state (mo);
	
	if (status == MPD_PLAYER_PAUSE)
		return PAUSE;
	else if (status == MPD_PLAYER_PLAY)
		return PLAY;
	else if (status == MPD_PLAYER_STOP)
		return STOP;
	return UNKNOWN;
}

bool
gimmix_play (MpdObj *mo)
{
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
	bool ret;
	state = gimmix_get_status (mo);

	if (state == PLAY || state == PAUSE)
	{
		int i;
		i = mpd_player_seek (mo, seektime);
		if (i == MPD_OK)
			ret = true;
		else
		{
			printf ("%s %d: %s\n", _("Error"), i, _("Status Failed."));
			ret = false;
		}
	}
	else
	ret = false;

	return ret;
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
			total = mpd_status_get_total_song_time (mo);
			elapsed = mpd_status_get_elapsed_song_time (mo);
			snprintf (time, 20, "%02i:%02i / %02i:%02i", elapsed/60,
					elapsed%60,
					total/60,
					total%60);
			if (fraction!=NULL)
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
gimmix_get_total_time_for_song (MpdObj *mo, mpd_Song *song, char *time)
{
	if (!song)
	{
		time = NULL;
		return;
	}
	snprintf (time, 15, "%02i:%02i",
				song->time/60,
				song->time%60);
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

char *
gimmix_get_full_image_path (const char *image_name)
{
	char *full_path;
	
	full_path = (char*)malloc((strlen(PREFIX) + strlen(image_name) + 16) * sizeof(char));
	sprintf (full_path, "%s%s%s", PREFIX, "/share/pixmaps/", image_name);
	
	return full_path;
}

void
gimmix_strip_file_ext (char *string)
{
	int len;
	int i;
	
	if (string == NULL || (len=strlen(string))==0 )
	return;
	
	for (i=len-1; i>0; i--)
	{
		if (string[i] == '.')
		{
			string[i] = '\0';
			break;
		}
	}

	return;
}

void
gimmix_strcrep (char *string, char c1, char c2)
{
	char *ptr = NULL;
	
	ptr = string;
	while (*ptr!=0)
	{
		if (*ptr == c1)
			*ptr = c2;
		ptr++;
	}
	
	return;
}

double
gimmix_get_libmpd_version (void)
{
	double version = 0;

	version = atof (libmpd_version);

	return version;
}

