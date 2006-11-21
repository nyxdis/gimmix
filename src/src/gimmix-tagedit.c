/*
 * gimmix-tagedit.c
 *
 * Copyright (C) 2006 Priyank Gosalia
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
 * Author: Priyank Gosalia <priyankmgg@gmail.com>
 */

#include <tag_c.h>
#include "gimmix.h"
#include "gimmix-tagedit.h"

/* Globals */
TagLib_File *file;
TagLib_Tag 	*tag;

/* Functions */

void
gimmix_populate_tag_editor (const char *song)
{
	GtkWidget *widget;
	char length[6];
	char bitrate[6];
	int min;
	int sec;
	const TagLib_AudioProperties *properties;
	
	if(!song)
		return;
	
	file = taglib_file_new (song);
	if(file == NULL)
		return;

	taglib_set_strings_unicode(FALSE);
	tag = taglib_file_tag (file);
	properties = taglib_file_audioproperties(file);
	
	widget = glade_xml_get_widget (xml, "info_file");
	gtk_entry_set_text (GTK_ENTRY(widget), song);

	widget = glade_xml_get_widget (xml,"info_title");
	gtk_label_set_text (GTK_LABEL(widget), taglib_tag_title(tag));
		
	widget = glade_xml_get_widget (xml,"info_artist");
	gtk_label_set_text (GTK_LABEL(widget), taglib_tag_artist(tag));
		
	widget = glade_xml_get_widget (xml,"info_album");
	gtk_label_set_text (GTK_LABEL(widget), taglib_tag_album(tag));
		
	widget = glade_xml_get_widget (xml,"info_genre");
	gtk_label_set_text (GTK_LABEL(widget), taglib_tag_genre(tag));

	widget = glade_xml_get_widget (xml, "info_length");
	sec = taglib_audioproperties_length(properties) % 60;
    min = (taglib_audioproperties_length(properties) - sec) / 60;
	snprintf (length, 6, "%02i:%02i", min, sec);
	gtk_label_set_text (GTK_LABEL(widget), length);

	widget = glade_xml_get_widget (xml, "info_bitrate");
	snprintf (bitrate, 10, "%i Kbps", taglib_audioproperties_bitrate(properties));
	gtk_label_set_text (GTK_LABEL(widget), bitrate);
	
	taglib_tag_free_strings();
    taglib_file_free(file);
	
	return;
}
