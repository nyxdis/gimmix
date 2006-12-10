#ifndef GIMMIX_H
#define GIMMIX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <locale.h>
#include <libintl.h>
#define _(String) gettext (String)
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include <glade/glade.h>
#include "wejpconfig.h"
#include "gimmix-config.h"
#include "gimmix-core.h"
#include "gimmix-interface.h"

#define APPNAME 		"Gimmix"
#define VERSION 		"0.3"
#define APPURL			"http://gimmix.berlios.de/"

/* main connect function */
bool gimmix_connect (void);

/* display connection error */
void gimmix_connect_error (void);

/* displays the about dialog */
void gimmix_about_show (void);

/* Finalize */
void exit_cleanup (void);

#endif
