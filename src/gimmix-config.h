#ifndef GIMMIX_CONFIG_H
#define GIMMIX_CONFIG_H

#include <stdio.h>
#include <stdbool.h>
#include "wejpconfig.h"

typedef struct _conf {
	/* connection info */
	char		mpd_hostname[256];
	char		mpd_password[256];
	unsigned int	mpd_port;
	
	/* proxy server stuff */
	bool		proxy_enable;
	char		proxy_host[256];
	unsigned int	proxy_port;
	
	/* tag editor */
	char		music_directory[256];
	
	/* systray & notification */
	bool		enable_systray;
	bool		enable_notification;
	
	/* cover art plugin */
	bool		coverart_enable;
	char		*coverart_location;
	
	/* other stuff */
	bool		play_on_add;
	bool		stop_on_exit;
	bool		full_view_mode;
	bool		enable_search;
	bool		update_on_startup;
	
} GimmixConfig;

/* Returns true if file exists otherwise false */
bool gimmix_config_exists (void);

/* Parse config file and set the initial config values */
bool gimmix_config_init (void);

/* Returns true/false depending on config key value */
bool gimmix_config_get_bool (const char *);

/* Save current settings back to gimmixrc */
void gimmix_config_save (void);

/* Returns proxy server url as host:port */
char* gimmix_config_get_proxy_string (void);

/* Free conf */
void gimmix_config_free (void);

#endif
