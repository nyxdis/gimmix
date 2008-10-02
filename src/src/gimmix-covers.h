#ifdef HAVE_COVER_PLUGIN

#ifndef GIMMIX_COVERS_H
#define GIMMIX_COVERS_H

#include "gimmix-core.h"
#include "gimmix-metadata.h"
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

/* Update cover */
void gimmix_covers_plugin_update_cover (gboolean defaultc);

/* get cover image of specified size */
GdkPixbuf* gimmix_covers_plugin_get_cover_image_of_size (guint width, guint height);

#endif

#endif
