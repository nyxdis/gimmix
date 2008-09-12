#ifndef GIMMIX_LYRICS_H
#define GIMMIX_LYRICS_H

#include "gimmix-core.h"
#include "gimmix.h"
#include "wejpconfig.h"
#include <gtk/gtk.h>

typedef struct lnode
{
	char artist[80];
	char title[80];
	char hid[16];
	char writer[80];
	gboolean match;
	char *lyrics;
} LYRICS_NODE;

/* Initialize the gimmix lyrics plugin */
void gimmix_lyrics_plugin_init (void);

void lyrics_set_artist (const char *artist);
void lyrics_set_songtitle (const char *title);
LYRICS_NODE* lyrics_get_lyrics (void);
gboolean lyrics_search (void);
void gimmix_lyrics_populate_textview (const char *text);

#endif