/*
 * config.c
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
 * Author: Priyank Gosalia <priyankmgg@gmail.com>
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
	int		port;
	int		systray_enable;
	int		notify_enable;
	
	conf = (Conf*)malloc(sizeof(Conf));
	char	*host = NULL;
	char	*pass = NULL;

	cfg_opt_t opts[] = {
		CFG_SIMPLE_STR ("mpd_hostname", &host),
		CFG_SIMPLE_INT ("mpd_port", &port),
		CFG_SIMPLE_STR ("mpd_password", &pass),
		CFG_SIMPLE_INT ("enable_systray", &systray_enable),
		CFG_SIMPLE_INT ("enable_notify", &notify_enable),
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
	
	if (!port)
		conf->port = 0;
	else
		conf->port = port;
	
	if ((systray_enable != 0) && (systray_enable != 1))
		conf->systray_enable = 1;
	else
		conf->systray_enable = systray_enable;
		
	if ((notify_enable != 0) && (notify_enable != 1))
		conf->notify_enable = 1;
	else
		conf->notify_enable = notify_enable;
	
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
		CFG_SIMPLE_INT ("enable_systray", 0),
		CFG_SIMPLE_INT ("enable_notify", 0),
		CFG_END()
	};

	cfg = cfg_init(opts, 0);
	char *rcfile = cfg_tilde_expand ("~/.gimmixrc");
	
	if((fp = fopen(rcfile, "w")))
	{	
		fprintf (fp, "# Gimmix configuration\n");
		fprintf (fp, "\n# MPD hostname (default: localhost)\n");
		if (conf->hostname)
			cfg_setstr(cfg, "mpd_hostname", conf->hostname);
		sopts = cfg_getopt (cfg, "mpd_hostname");
		cfg_opt_print (sopts, fp);

		fprintf (fp, "\n# MPD port (default: 6600)\n");
		if (conf->port > 0)
			cfg_setint(cfg, "mpd_port", conf->port);
		else
			cfg_setint(cfg, "mpd_port", 0);
		sopts = cfg_getopt (cfg, "mpd_port");
		cfg_opt_print (sopts, fp);
		
		fprintf (fp, "\n# MPD password (leave blank for no password) \n");
		if (conf->password)
			cfg_setstr(cfg, "mpd_password", conf->password);
		sopts = cfg_getopt (cfg, "mpd_password");
		cfg_opt_print (sopts, fp);

		fprintf (fp, "\n# Enable/Disable systray icon (1 = Enable, 0 = Disable) \n");
		if (conf->systray_enable == 1 || conf->systray_enable == 0)
			cfg_setint(cfg, "enable_systray", conf->systray_enable);
		else
			cfg_setint(cfg, "enable_systray", 1);
		sopts = cfg_getopt (cfg, "enable_systray");
		cfg_opt_print (sopts, fp);
		
		fprintf (fp, "\n# Enable/Disable system tray notifications (1 = Enable, 0 = Disable) \n");
		if (conf->notify_enable == 1 || conf->notify_enable == 0)
			cfg_setint(cfg, "enable_notify", conf->notify_enable);
		else
			cfg_setint(cfg, "enable_notify", 1);
		sopts = cfg_getopt (cfg, "enable_notify");
		cfg_opt_print (sopts, fp);

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
