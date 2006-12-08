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
#include <gtk/gtk.h>
#include "gimmix.h"
#include "gimmix-config.h"

#define CONFIG_FILE ".gimmixrc"

ConfigFile conf;

bool
gimmix_config_init (void)
{
	char		*rcfile;

	cfg_init_config_file_struct (&conf);
	
	cfg_add_key (&conf, "mpd_hostname",		"localhost");
	cfg_add_key (&conf, "mpd_port", 		"6600");
	cfg_add_key (&conf, "mpd_password",		"");
	cfg_add_key (&conf, "music_directory",	"");
	cfg_add_key (&conf, "enable_systray",	"true");
	cfg_add_key (&conf, "play_on_add",		"false");
	cfg_add_key (&conf, "stop_on_exit",		"false");
	cfg_add_key (&conf, "window_xpos",		"200");
	cfg_add_key (&conf, "window_ypos",		"300");
	cfg_add_key (&conf, "full_view_mode",	"false");
	
	rcfile = cfg_get_path_to_config_file (CONFIG_FILE);
	
	if (cfg_read_config_file (&conf, rcfile) != 0)
	{	
		g_free (rcfile);
		return false;
	}
	else
	{	
		g_free (rcfile);
		return true;
	}

	return false;
}

void
gimmix_config_save (void)
{
	char *rcfile;
	
	rcfile = cfg_get_path_to_config_file (CONFIG_FILE);
	cfg_write_config_file (&conf, rcfile);
	g_free (rcfile);
	
	return;
}

bool
gimmix_config_exists (void)
{
	char *config_file;
	bool status;
	
	config_file = cfg_get_path_to_config_file (CONFIG_FILE);
	if (g_file_test(config_file, G_FILE_TEST_EXISTS))
		status = true;
	else
		status = false;

	free (config_file);
	return status;
}

void
gimmix_config_free (void)
{
	cfg_free_config_file_struct (&conf);
	
	return;
}

