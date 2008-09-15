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

void gimmix_covers_plugin_init (void);

CoverNode* gimmix_covers_plugin_get_metadata (char *arg1, char *arg1d, char *arg2, char *arg2d);

GdkPixbuf* gimmix_covers_plugin_get_cover_image_of_size (guint width, guint height);

#endif

#endif
