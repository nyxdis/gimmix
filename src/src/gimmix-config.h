#ifndef GIMMIX_CONFIG_H
#define GIMMIX_CONFIG_H

#include <stdio.h>
#include <stdbool.h>
#include <confuse.h>

typedef struct
{
	int port;
	int systray_enable;
	int	notify_enable;
	int play_immediate;
	int stop_on_exit;
	int notify_timeout;
	char hostname[255];
	char password[255];
	char musicdir[255];
} Conf;

/* Returns true if file exists otherwise false */
bool gimmix_config_exists (void);

/* Parse config file and set the initial config values */
Conf * gimmix_config_init (void);

/* Save current settings back to gimmixrc */
void gimmix_config_save (Conf *);

/* Free conf */
void gimmix_config_free (Conf *);

#endif
