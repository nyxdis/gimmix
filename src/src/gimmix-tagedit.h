#ifndef GIMMIX_TAGEDIT_H
#define GIMMIX_TAGEDIT_H

#include <gtk/gtk.h>
#include <tag_c.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>

/* Initialize the tag editor signals */
void gimmix_tag_editor_init (void);

/* Populate the tag editor window */
void gimmix_tag_editor_populate (const char *);

#endif
