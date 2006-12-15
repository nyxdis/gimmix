#ifndef GIMMIX_TAGEDIT_H
#define GIMMIX_TAGEDIT_H

#include <gtk/gtk.h>
#include <tag_c.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include "gimmix-prefs.h"

/* Initialize the tag editor signals */
void gimmix_tag_editor_init (void);

/* populate the tag editor window */
gboolean gimmix_tag_editor_populate (const gchar *);

/* Display the tag editor window */
void gimmix_tag_editor_show (void);

/* Display an error */
void gimmix_tag_editor_error (const gchar *);

#endif
