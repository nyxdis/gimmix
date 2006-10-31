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
#define VERSION "0.1 beta"

typedef struct Gimmix
{
	MpdObj 	*gmo;
	Conf 	*conf;
} GM;

GM *pub;
GladeXML *xml;

bool gimmix_connect (void);

/* Connection error dialog */
void gimmix_connect_error (void);
void error_dialog_response ();
void on_fr_close_clicked (GtkWidget *widget, gpointer data);
void on_fr_apply_clicked (GtkWidget *widget, gpointer data);

/* Finalize */
void exit_cleanup (void);

#endif
