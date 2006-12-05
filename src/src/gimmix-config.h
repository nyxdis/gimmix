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

/* Free conf */
void gimmix_config_free (void);

#endif
