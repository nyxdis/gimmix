#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdbool.h>
#include <confuse.h>

typedef struct
{
	int systray_enable;
	int port;
	char hostname[256];
	char password[256];
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
