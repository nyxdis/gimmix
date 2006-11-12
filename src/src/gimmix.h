#ifndef GIMMIX_H
#define GIMMIX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include <glade/glade.h>
#include "config.h"

#define APPNAME "Gimmix"
#define VERSION "0.2RC1"

typedef struct Gimmix
{
	MpdObj 	*gmo;
	Conf 	*conf;
} GM;

GM *pub;

GladeXML *xml;

/* Finalize */
void exit_cleanup (void);

#endif
