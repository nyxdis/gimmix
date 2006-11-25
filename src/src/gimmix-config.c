/*
 * gimmix-config.c
 *
 * Copyright (C) 2006 Priyank Gosalia
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

#include <stdlib.h>
#include <string.h>
#include <confuse.h>
#include <gtk/gtk.h>
#include "gimmix-config.h"

#define CONFIG_FILE "~/.gimmixrc"

Conf *
gimmix_config_init (void)
{
	cfg_t 		*cfg = NULL;
	char		*rcfile;
	Conf 		*conf;
	int 		ret;
	int			port = 1;
	cfg_bool_t	systray_enable = true;
	cfg_bool_t	notify_enable = true;
	cfg_bool_t	play_song_on_add = false;
	cfg_bool_t 	stop_on_exit = false;
	
	conf = (Conf*)malloc(sizeof(Conf));
	char	*host = NULL;
	char	*pass = NULL;
	char	*musicdir = NULL;

	cfg_opt_t opts[] = {
		CFG_SIMPLE_STR ("mpd_hostname", &host),
		CFG_SIMPLE_INT ("mpd_port", &port),
		CFG_SIMPLE_STR ("mpd_password", &pass),
		CFG_SIMPLE_BOOL ("enable_systray", &systray_enable),
		CFG_SIMPLE_BOOL ("enable_notify", &notify_enable),
		CFG_SIMPLE_STR ("music_directory", &musicdir),
		CFG_SIMPLE_BOOL ("play_immediately_on_add", &play_song_on_add),
		CFG_SIMPLE_BOOL ("stop_playback_on_exit", &stop_on_exit),
		CFG_END()
	};
	
	cfg = cfg_init (opts, 0);
	rcfile = cfg_tilde_expand (CONFIG_FILE);
	ret = cfg_parse (cfg, rcfile);
	free (rcfile);
	
	if (ret == CFG_PARSE_ERROR)
	{
		free (conf);
		return NULL;
	}
	
	if (host != NULL)
		strncpy (conf->hostname, host, 255);
	
	if (pass != NULL)
		strncpy (conf->password, pass, 255);
		
	if (musicdir != NULL)
		strncpy (conf->musicdir, musicdir, 255);
	
	conf->port = port;
	
	if (systray_enable == true)
	{	
		conf->systray_enable = 1;
	}
	else
	{	
		conf->systray_enable = 0;
	}
		
	if (notify_enable == true)
	{	
		conf->notify_enable = 1;
	}
	else
	{
		conf->notify_enable = 0;
	}

	if (play_song_on_add == true)
		conf->play_immediate = 1;
	else
		conf->play_immediate = 0;
	
	if (stop_on_exit == true)
		conf->stop_on_exit = 1;
	else
		conf->stop_on_exit = 0;
		
	/* Free the memory */
	cfg_free_value (opts);
	
	if (cfg)
		cfg_free (cfg);
	
	return conf;
}

void
gimmix_config_save (Conf *conf)
{
	FILE *fp;
	cfg_t *cfg;
	cfg_opt_t *sopts;
	
	cfg_opt_t opts[] = {
		CFG_SIMPLE_STR ("mpd_hostname", NULL),
		CFG_SIMPLE_INT ("mpd_port", 0),
		CFG_SIMPLE_STR ("mpd_password", NULL),
		CFG_SIMPLE_BOOL ("enable_systray", false),
		CFG_SIMPLE_BOOL ("enable_notify", false),
		CFG_SIMPLE_STR ("music_directory", NULL),
		CFG_SIMPLE_BOOL ("play_immediately_on_add", false),
		CFG_SIMPLE_BOOL ("stop_playback_on_exit", false),
		CFG_END()
	};

	cfg = cfg_init(opts, 0);
	char *rcfile = cfg_tilde_expand (CONFIG_FILE);
	
	if((fp = fopen(rcfile, "w")))
	{	
		fprintf (fp, "# Gimmix configuration\n");
		fprintf (fp, "\n# MPD hostname (default: localhost)\n");
		if (conf->hostname)
			cfg_setstr (cfg, "mpd_hostname", conf->hostname);
		sopts = cfg_getopt (cfg, "mpd_hostname");
		cfg_opt_print (sopts, fp);

		fprintf (fp, "\n# MPD port (default: 6600)\n");
		if (conf->port > 0)
			cfg_setint (cfg, "mpd_port", conf->port);
		else
			cfg_setint (cfg, "mpd_port", 0);
		sopts = cfg_getopt (cfg, "mpd_port");
		cfg_opt_print (sopts, fp);
		
		fprintf (fp, "\n# MPD password (leave blank for no password) \n");
		if (conf->password)
			cfg_setstr (cfg, "mpd_password", conf->password);
		sopts = cfg_getopt (cfg, "mpd_password");
		cfg_opt_print (sopts, fp);

		fprintf (fp, "\n# Enable/Disable systray icon (Enable = true, Disable = false)\n");
		if (conf->systray_enable == 1)
			cfg_setbool (cfg, "enable_systray", true);
		else
			cfg_setbool (cfg, "enable_systray", false);
		sopts = cfg_getopt (cfg, "enable_systray");
		cfg_opt_print (sopts, fp);
		
		fprintf (fp, "\n# Enable/Disable system tray notifications (Enable = true, Disable = false) \n");
		if (conf->notify_enable == 1)
			cfg_setbool (cfg, "enable_notify", true);
		else
			cfg_setbool (cfg, "enable_notify", false);
		sopts = cfg_getopt (cfg, "enable_notify");
		cfg_opt_print (sopts, fp);
		
		fprintf (fp, "\n# Music directory (should be same as mpd's music_directory) \n# This is rquired for editing ID3 tags\n");
		if (conf->musicdir)
			cfg_setstr (cfg, "music_directory", conf->musicdir);
		sopts = cfg_getopt (cfg, "music_directory");
		cfg_opt_print (sopts, fp);
		
		fprintf (fp, "\n# Play the song immediately when added to playlist\n");
		if (conf->play_immediate == 1)
			cfg_setbool (cfg, "play_immediately_on_add", true);
		else
			cfg_setbool (cfg, "play_immediately_on_add", false);
		sopts = cfg_getopt (cfg, "play_immediately_on_add");
		cfg_opt_print (sopts, fp);
		
		fprintf (fp, "\n# Stop playback when gimmix exits\n");
		if (conf->stop_on_exit == 1)
			cfg_setbool (cfg, "stop_playback_on_exit", true);
		else
			cfg_setbool (cfg, "stop_playback_on_exit", false);
		sopts = cfg_getopt (cfg, "stop_playback_on_exit");
		cfg_opt_print (sopts, fp);
		
		free (rcfile);
		fclose (fp);
	}
	else
	{	
		fprintf (stderr, "Error while saving config.\n");
	}

	cfg_free_value (opts);
	cfg_free (cfg);
	
	return;
}

bool
gimmix_config_exists (void)
{
	char *config_file;
	bool status;
	
	config_file = cfg_tilde_expand (CONFIG_FILE);
	if (g_file_test(config_file, G_FILE_TEST_EXISTS))
		status = true;
	else
		status = false;

	free (config_file);
	return status;
}

void
gimmix_config_free (Conf *conf)
{
	if (conf != NULL)
	{
		free (conf);
	}

	return;
}
