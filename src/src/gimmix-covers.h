#ifdef HAVE_COVER_PLUGIN

#ifndef GIMMIX_COVERS_H
#define GIMMIX_COVERS_H

#include "gimmix-core.h"
#include "gimmix.h"
#include "wejpconfig.h"
#include <gtk/gtk.h>

typedef struct _covers_node {
	char	*img_large;
	char	*img_medium;
	char	*img_small;
	char	*album_info;
} CoverNode;

/* Initialize the covers plugin */
void gimmix_covers_plugin_init (void);

/* De-initialize the covers plugin */
void gimmix_covers_plugin_cleanup (void);

/* Get metadata for the specified arguments */
CoverNode* gimmix_covers_plugin_get_metadata (char *arg1, char *arg1d, char *arg2, char *arg2d);

/* Get the fallback cover image of specified size */
GdkPixbuf* gimmix_covers_plugin_get_default_cover (guint width, guint height);

/* Get a cover image of specified size */
GdkPixbuf* gimmix_covers_plugin_get_cover_image_of_size (guint width, guint height);

/* Set the cover image for metadata section */
void gimmix_covers_plugin_set_metadata_image (GdkPixbuf *pixbuf);

/* Update cover */
void gimmix_covers_plugin_update_cover (SongInfo *s);

#endif

#endif
