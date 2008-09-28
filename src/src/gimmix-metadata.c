/*
 * gimmix-metadata.c
 *
 * Copyright (C) 2008 Priyank Gosalia
 *
 * Gimmix is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * Gimmix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with Gimmix; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Priyank Gosalia <priyankmg@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "gimmix-metadata.h"

extern GladeXML		*xml;
extern MpdObj		*gmo;
extern ConfigFile	conf;

static GtkWidget *metadata_song_title;
static GtkWidget *metadata_song_artistbox;
static GtkWidget *metadata_song_artist;
static GtkWidget *metadata_song_albumbox;
static GtkWidget *metadata_song_album;
static GtkWidget *metadata_song_genrebox;
static GtkWidget *metadata_song_genre;
static GtkWidget *metadata_song_albumreview;
static GtkWidget *metadata_song_cover;
static GtkWidget *metadata_container;

void
gimmix_metadata_init (void)
{
	metadata_song_title = glade_xml_get_widget (xml, "metadata_song_label");
	metadata_song_artist = glade_xml_get_widget (xml, "metadata_artist_label");
	metadata_song_album = glade_xml_get_widget (xml, "metadata_album_label");
	metadata_song_genre = glade_xml_get_widget (xml, "metadata_genre_label");
	metadata_song_albumreview = glade_xml_get_widget (xml, "metadata_albuminfo");
	metadata_song_albumbox = glade_xml_get_widget (xml, "metadata_album_box");
	metadata_song_artistbox = glade_xml_get_widget (xml, "metadata_artist_box");
	metadata_song_genrebox = glade_xml_get_widget (xml, "metadata_genre_box");
	metadata_song_cover = glade_xml_get_widget (xml, "metadata_albumart");
	metadata_container = glade_xml_get_widget (xml, "metadata_container");
	
	#ifndef HAVE_COVER_PLUGIN
	gimmix_metadata_show_song_cover (FALSE);
	#else
	if (!strncasecmp(cfg_get_key_value(conf,"coverart_enable"),"true",4))
	{
		gimmix_metadata_show_song_cover (TRUE);
	}
	else
	{
		gimmix_metadata_show_song_cover (FALSE);
	}
	#endif

	return;
}

void
gimmix_metadata_disable_controls (void)
{
	gtk_widget_set_sensitive (metadata_container, FALSE);

	return;
}

void
gimmix_metadata_enable_controls (void)
{
	gtk_widget_set_sensitive (metadata_container, TRUE);

	return;
}


void
gimmix_metadata_show_song_cover (gboolean show)
{
	if (show)
		gtk_widget_show (metadata_song_cover);
	else
		gtk_widget_hide (metadata_song_cover);
		
	return;
}

void
gimmix_metadata_set_song_details (mpd_Song *song, char* albumreview)
{
	gchar *markup = NULL;
	
	if (song == NULL)
		return;

	if (song->title)
	{
		markup = g_markup_printf_escaped ("<span size=\"x-large\"weight=\"bold\">%s</span>", song->title);
		gtk_label_set_markup (GTK_LABEL(metadata_song_title), markup);
		g_free (markup);
	}
	
	if (song->artist)
	{
		gtk_widget_show (metadata_song_artistbox);
		gtk_label_set_text (GTK_LABEL(metadata_song_artist), song->artist);
	}
	else
	{
		gtk_widget_hide (metadata_song_artistbox);
	}
	
	if (song->album)
	{
		gtk_widget_show (metadata_song_albumbox);
		gtk_label_set_text (GTK_LABEL(metadata_song_album), song->album);
	}
	else
	{
		gtk_widget_hide (metadata_song_albumbox);
	}
		
	if (song->genre)
	{
		gtk_widget_show (metadata_song_genrebox);
		gtk_label_set_text (GTK_LABEL(metadata_song_genre), song->genre);
	}
	else
	{
		gtk_widget_hide (metadata_song_genrebox);
	}
	
	gtk_label_set_text (GTK_LABEL(metadata_song_albumreview), "");
	if (albumreview!=NULL)
	{
		markup = g_markup_escape_text (albumreview, strlen(albumreview));
		gtk_label_set_markup (GTK_LABEL(metadata_song_albumreview), markup);
		return;
	}
	
	return;
}


