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
 * Author: Priyank Gosalia <priyankmg@gmail.com>
 */

#include "gimmix.h"
#include "gimmix-core.h"
#include "gimmix-tagedit.h"

TagLib_File 	*file;
TagLib_Tag 		*tag;

/* Save action */
static void	gimmix_tag_editor_save (GtkWidget *button, gpointer data);

/* Close action */
static void gimmix_tag_editor_close (GtkWidget *widget, gpointer data);

/* Update action */
static gboolean	gimmix_update_song_info (gpointer data);

static gchar *dir_error = "You have specified an invalid music directory. Please specify the correct music directory in the preferences.";
	
void
gimmix_tag_editor_init (void)
{
	GtkWidget *widget;
	
	widget = glade_xml_get_widget (xml, "tag_editor_save");
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(gimmix_tag_editor_save), NULL);
	
	widget = glade_xml_get_widget (xml, "tag_editor_close");
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(gimmix_tag_editor_close), NULL);
	
	return;
}

/* Load the tag editor */
gboolean
gimmix_tag_editor_populate (const gchar *song)
{
	GtkWidget 		*widget;
	GtkTreeModel 	*genre_model;
	gchar 			info[10];
	gint 			min;
	gint			sec;
	gint 			n;
	const TagLib_AudioProperties *properties;
	
	if (!song)
		return FALSE;
	
	if (!g_file_test (song, G_FILE_TEST_EXISTS))
		return FALSE;
		
	file = taglib_file_new (song);
	if (file == NULL)
		return FALSE;

	taglib_set_strings_unicode (FALSE);
	
	tag = taglib_file_tag (file);
	properties = taglib_file_audioproperties (file);
	
	widget = glade_xml_get_widget (xml, "tag_year");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(widget), taglib_tag_year(tag));
	
	widget = glade_xml_get_widget (xml, "tag_track");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(widget), taglib_tag_track(tag));

	widget = glade_xml_get_widget (xml,"entry_title");
	gtk_entry_set_text (GTK_ENTRY(widget), taglib_tag_title(tag));

	widget = glade_xml_get_widget (xml,"entry_artist");
	gtk_entry_set_text (GTK_ENTRY(widget), taglib_tag_artist(tag));

	widget = glade_xml_get_widget (xml,"entry_album");
	gtk_entry_set_text (GTK_ENTRY(widget), taglib_tag_album(tag));

	widget = glade_xml_get_widget (xml,"combo_genre");
	gtk_combo_box_append_text (GTK_COMBO_BOX(widget), taglib_tag_genre(tag));
	genre_model = gtk_combo_box_get_model (GTK_COMBO_BOX(widget));
	n = gtk_tree_model_iter_n_children (genre_model, NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX(widget), n-1);
    
    widget = glade_xml_get_widget (xml, "entry_comment");
	gtk_entry_set_text (GTK_ENTRY(widget), taglib_tag_comment(tag));
	
	/* Audio Information */
	widget = glade_xml_get_widget (xml, "info_length");
	sec = taglib_audioproperties_length(properties) % 60;
    min = (taglib_audioproperties_length(properties) - sec) / 60;
	snprintf (info, 10, "%02i:%02i", min, sec);
	gtk_label_set_text (GTK_LABEL(widget), info);

	widget = glade_xml_get_widget (xml, "info_bitrate");
	snprintf (info, 10, "%i Kbps", taglib_audioproperties_bitrate(properties));
	gtk_label_set_text (GTK_LABEL(widget), info);
	
	widget = glade_xml_get_widget (xml, "info_channels");
	snprintf (info, 10, "%i", taglib_audioproperties_channels(properties));
	gtk_label_set_text (GTK_LABEL(widget), info);
	
	taglib_tag_free_strings ();
	
	return TRUE;
}

static void
gimmix_tag_editor_close (GtkWidget *widget, gpointer data)
{
	GtkWidget *window;
    window = glade_xml_get_widget (xml, "tag_editor_window");
	taglib_tag_free_strings ();
    taglib_file_free (file);
    
    gtk_widget_hide (window);
    
    return;
}

static void
gimmix_tag_editor_save (GtkWidget *button, gpointer data)
{
	GtkWidget 	*widget;
	gint		year;
	gint		track;
	gchar		*genre;
	const gchar *title;
	const gchar *artist;
	const gchar *album;
	const gchar *comment;

	widget = glade_xml_get_widget (xml, "tag_year");
	year = gtk_spin_button_get_value (GTK_SPIN_BUTTON(widget));
	taglib_tag_set_year (tag, year);

	widget = glade_xml_get_widget (xml, "tag_track");
	track = gtk_spin_button_get_value (GTK_SPIN_BUTTON(widget));
	taglib_tag_set_track (tag, track);

	widget = glade_xml_get_widget (xml, "entry_title");
	title = gtk_entry_get_text (GTK_ENTRY(widget));

	widget = glade_xml_get_widget (xml, "entry_artist");
	artist = gtk_entry_get_text (GTK_ENTRY(widget));

	widget = glade_xml_get_widget (xml, "entry_album");
	album = gtk_entry_get_text (GTK_ENTRY(widget));

	widget = glade_xml_get_widget (xml, "entry_comment");
	comment = gtk_entry_get_text (GTK_ENTRY(widget));

	widget = glade_xml_get_widget (xml,"combo_genre");
	genre = gtk_combo_box_get_active_text (GTK_COMBO_BOX(widget));

	taglib_tag_set_title (tag, title);
	taglib_tag_set_artist (tag, artist);
	taglib_tag_set_album (tag, album);
	taglib_tag_set_comment (tag, comment);
	taglib_tag_set_genre (tag, genre);
	
	/* update the mpd database */
	mpd_database_update_dir (pub->gmo, "/");
	
	/* set the song info a few seconds after update */
	g_timeout_add (300, (GSourceFunc)gimmix_update_song_info, NULL);
	
	/* free the strings */
	taglib_tag_free_strings ();
	taglib_file_save (file);
	
	return;
}

static gboolean
gimmix_update_song_info (gpointer data)
{
	GimmixStatus status;
	
	if (mpd_status_db_is_updating (pub->gmo))
		return TRUE;
	
	/* Set the new song info */
	status = gimmix_get_status (pub->gmo);
	if (status == PLAY || status == PAUSE)
		gimmix_set_song_info ();

	return FALSE;
}

void
gimmix_tag_editor_show (void)
{
	GimmixStatus 	status;
	GtkWidget		*window;
	SongInfo		*info;
	gchar			song[255];
	
	status = gimmix_get_status (pub->gmo);
	window = glade_xml_get_widget (xml, "tag_editor_window");
	
	if (status == PLAY || status == PAUSE)
	{
		info = gimmix_get_song_info (pub->gmo);
		snprintf (song, 255, "%s/%s", pub->conf->musicdir, info->file);
		if (gimmix_tag_editor_populate (song))
			gtk_widget_show (GTK_WIDGET(window));
		else
		{	
			g_print ("Error: Invalid music directory.\n");
			gimmix_tag_editor_error (dir_error);
		}	
		gimmix_free_song_info (info);
	}
	
	return;
}

void
gimmix_tag_editor_error (const gchar *error_text)
{
	GtkWidget 	*error_dialog;

	error_dialog = gtk_message_dialog_new_with_markup (NULL,
												GTK_DIALOG_DESTROY_WITH_PARENT,
												GTK_MESSAGE_ERROR,
												GTK_BUTTONS_OK,
												"<b>%s: </b><span size=\"large\">%s</span>",
												"ERROR",
												error_text);
    g_signal_connect (error_dialog,
					"response",
					G_CALLBACK (gtk_widget_destroy),
					(gpointer)error_dialog);
	
    gtk_widget_show_all (error_dialog);
    
    return;
}

