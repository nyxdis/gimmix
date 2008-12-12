#ifndef GIMMIX_CONFIG_H
#define GIMMIX_CONFIG_H

#include <stdio.h>
#include <stdbool.h>
#include "wejpconfig.h"

/* Returns true if file exists otherwise false */
bool gimmix_config_exists (void);

/* Parse config file and set the initial config values */
bool gimmix_config_init (void);

/* Save current settings back to gimmixrc */
void gimmix_config_save (void);

/* Returns proxy server url as host:port */
char* gimmix_config_get_proxy_string (void);

/* Free conf */
void gimmix_config_free (void);

#endif
