/*
 * config.c
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

#include <stdlib.h>
#include <string.h>
#include <confuse.h>
#include "config.h"

Conf *
gimmix_config_init (void)
{
	cfg_t 	*cfg = NULL;
	char	*rcfile;
	Conf 	*conf;
	int 	ret; 
	
	conf = (Conf*)malloc(sizeof(Conf));
	char	*host = NULL;
	char	*pass = NULL;

	cfg_opt_t opts[] = {
		CFG_SIMPLE_STR ("mpd_hostname", &host),
		CFG_SIMPLE_INT ("mpd_port", &conf->port),
		CFG_SIMPLE_STR ("mpd_password", &pass),
		CFG_SIMPLE_INT ("enable_systray", &conf->systray_enable),
		CFG_END()
	};
	
	cfg = cfg_init (opts, 0);
	rcfile = cfg_tilde_expand ("~/.gimmixrc");
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
	
	if (!conf->port)
		conf->port = 0;
	
	if ((conf->systray_enable != 0) && (conf->systray_enable != 1))
		conf->systray_enable = -1;
	
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
	
	cfg_opt_t opts[] = {
		CFG_SIMPLE_STR ("mpd_hostname", NULL),
		CFG_SIMPLE_INT ("mpd_port", 0),
		CFG_SIMPLE_STR ("mpd_password", NULL),
		CFG_SIMPLE_INT ("enable_systray", 0),
		CFG_END()
	};

	cfg = cfg_init(opts, 0);
    char *rcfile = cfg_tilde_expand ("~/.gimmixrc");
	
	if((fp = fopen(rcfile, "w")))
	{	
		if (conf->hostname)
			cfg_setstr(cfg, "mpd_hostname", conf->hostname);

		if (conf->port > 0)
			cfg_setint(cfg, "mpd_port", conf->port);
		else
			cfg_setint(cfg, "mpd_port", 0);

		if (conf->systray_enable == 1 || conf->systray_enable == 0)
			cfg_setint(cfg, "enable_systray", conf->systray_enable);
		else
			cfg_setint(cfg, "enable_systray", -1);

		if (conf->password)
			cfg_setstr(cfg, "mpd_password", conf->password);

		cfg_print (cfg, fp);
        free (rcfile);
		fclose (fp);
	}
	else
		fprintf (stderr, "Error while saving config.\n");

	cfg_free_value (opts);
	cfg_free (cfg);
	
	return;
}

bool
gimmix_config_exists ()
{
	FILE *fp;
	char *rcfile = cfg_tilde_expand ("~/.gimmixrc");
	
	if (fp = fopen(rcfile, "r"))
	{
		fclose (fp);
		return true;
	}
	
	return false;
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
