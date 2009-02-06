#ifndef GIMMIX_TAGEDIT_H
#define GIMMIX_TAGEDIT_H

#include <gtk/gtk.h>

#ifdef HAVE_TAGEDITOR
#include <tag_c.h>
#endif

#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include "gimmix-prefs.h"

/* Initialize the tag editor signals */
void gimmix_tag_editor_widgets_init (void);

/* populate the tag editor window */
gboolean gimmix_tag_editor_populate (const void *);

/* Display the tag editor window */
void gimmix_tag_editor_show (void);

/* Display an error */
void gimmix_tag_editor_error (const gchar *);

/* Set cover image */
void gimmix_tag_editor_set_cover_image (GdkPixbuf *);

#endif
