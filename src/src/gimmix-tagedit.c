/*
 * gimmix-tagedit.c
 *
 * Copyright (C) 2006-2007 Priyank Gosalia
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

extern MpdObj 		*gmo;
extern GladeXML 	*xml;
extern ConfigFile	conf;

TagLib_File 	*file = NULL;
TagLib_Tag 		*tag = NULL;

GtkWidget	*tag_title;
GtkWidget	*tag_file;
GtkWidget	*tag_artist;
GtkWidget	*tag_album;
GtkWidget	*tag_comment;
GtkWidget	*tag_track_spin;
GtkWidget	*tag_year_spin;
GtkWidget	*tag_genre;
GtkWidget	*tag_info_bitrate;
GtkWidget	*tag_info_channels;
GtkWidget	*tag_info_length;
GtkWidget	*tag_editor_window;

/* Save action */
static void	gimmix_tag_editor_save (GtkWidget *button, gpointer data);

/* Close action */
static void gimmix_tag_editor_close (GtkWidget *widget, gpointer data);

/* Update action */
static gboolean	gimmix_update_song_info (gpointer data);

/* error dialog callback */
static void cb_gimmix_tag_editor_error_response (GtkDialog *dialog, gint arg1, gpointer data);

const gchar *dir_error = "You have specified an invalid music directory. Do you want to specify the correct music directory now ?";
	
void
gimmix_tag_editor_init (void)
{
	GtkWidget *widget;
	
	tag_editor_window = glade_xml_get_widget (xml, "tag_editor_window");
	tag_title = glade_xml_get_widget (xml,"entry_title");
	tag_file = glade_xml_get_widget (xml,"label_filename");
	tag_artist = glade_xml_get_widget (xml,"entry_artist");
	tag_album = glade_xml_get_widget (xml,"entry_album");
	tag_comment = glade_xml_get_widget (xml,"entry_comment");
	tag_year_spin = glade_xml_get_widget (xml, "tag_year");
	tag_track_spin = glade_xml_get_widget (xml, "tag_track");
	tag_genre = glade_xml_get_widget (xml,"combo_genre");
	tag_info_length = glade_xml_get_widget (xml, "info_length");
	tag_info_channels = glade_xml_get_widget (xml, "info_channels");
	tag_info_bitrate = glade_xml_get_widget (xml, "info_bitrate");
	
	widget = glade_xml_get_widget (xml, "tag_editor_save");
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(gimmix_tag_editor_save), NULL);
	
	widget = glade_xml_get_widget (xml, "tag_editor_close");
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(gimmix_tag_editor_close), NULL);
	g_signal_connect (G_OBJECT(tag_editor_window), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
	
	return;
}

/* Load the tag editor */
gboolean
gimmix_tag_editor_populate (const gchar *song)
{
	GtkTreeModel 	*genre_model;
	gchar 			*info;
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
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(tag_year_spin), taglib_tag_year(tag));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(tag_track_spin), taglib_tag_track(tag));
	gtk_label_set_text (GTK_LABEL(tag_file), song);
	gtk_entry_set_text (GTK_ENTRY(tag_title), taglib_tag_title(tag));
	gtk_entry_set_text (GTK_ENTRY(tag_artist), taglib_tag_artist(tag));
	gtk_entry_set_text (GTK_ENTRY(tag_album), taglib_tag_album(tag));
	gtk_entry_set_text (GTK_ENTRY(tag_comment), taglib_tag_comment(tag));
	
	gtk_combo_box_append_text (GTK_COMBO_BOX(tag_genre), taglib_tag_genre(tag));
	genre_model = gtk_combo_box_get_model (GTK_COMBO_BOX(tag_genre));
	n = gtk_tree_model_iter_n_children (genre_model, NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX(tag_genre), n-1);

	/* Audio Information */
	sec = taglib_audioproperties_length(properties) % 60;
    min = (taglib_audioproperties_length(properties) - sec) / 60;
	info = g_strdup_printf ("%02i:%02i", min, sec);
	gtk_label_set_text (GTK_LABEL(tag_info_length), info);
	g_free (info);

	info = g_strdup_printf ("%i Kbps", taglib_audioproperties_bitrate(properties));
	gtk_label_set_text (GTK_LABEL(tag_info_bitrate), info);
	g_free (info);
	
	info = g_strdup_printf ("%i", taglib_audioproperties_channels(properties));
	gtk_label_set_text (GTK_LABEL(tag_info_channels), info);
	g_free (info);
	
	taglib_tag_free_strings ();
	
	return TRUE;
}

static void
gimmix_tag_editor_close (GtkWidget *widget, gpointer data)
{
	taglib_tag_free_strings ();
    if (file)
    	taglib_file_free (file);
    gtk_widget_hide (tag_editor_window);
    
    return;
}

static void
gimmix_tag_editor_save (GtkWidget *button, gpointer data)
{
	gint		year;
	gint		track;
	gchar		*genre;
	const gchar *title;
	const gchar *artist;
	const gchar *album;
	const gchar *comment;

	year = gtk_spin_button_get_value (GTK_SPIN_BUTTON(tag_year_spin));
	taglib_tag_set_year (tag, year);

	track = gtk_spin_button_get_value (GTK_SPIN_BUTTON(tag_track_spin));
	taglib_tag_set_track (tag, track);

	title = gtk_entry_get_text (GTK_ENTRY(tag_title));
	artist = gtk_entry_get_text (GTK_ENTRY(tag_artist));
	album = gtk_entry_get_text (GTK_ENTRY(tag_album));
	comment = gtk_entry_get_text (GTK_ENTRY(tag_comment));
	genre = gtk_combo_box_get_active_text (GTK_COMBO_BOX(tag_genre));

	taglib_tag_set_title (tag, title);
	taglib_tag_set_artist (tag, artist);
	taglib_tag_set_album (tag, album);
	taglib_tag_set_comment (tag, comment);
	taglib_tag_set_genre (tag, genre);
	
	/* update the mpd database */
	mpd_database_update_dir (gmo, "/");
	
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
	
	if (mpd_status_db_is_updating (gmo))
		return TRUE;
	else
		return FALSE;
	
	/* Set the new song info */
	status = gimmix_get_status (gmo);
	if (status == PLAY || status == PAUSE)
		gimmix_set_song_info ();

	return FALSE;
}

void
gimmix_tag_editor_show (void)
{
	GimmixStatus 	status;
	SongInfo		*info;
	gchar			*song;
	
	status = gimmix_get_status (gmo);
	
	if (status == PLAY || status == PAUSE)
	{
		info = gimmix_get_song_info (gmo);
		song = g_strdup_printf ("%s/%s", cfg_get_key_value(conf, "music_directory"), info->file);
		if (gimmix_tag_editor_populate (song))
			gtk_widget_show (GTK_WIDGET(tag_editor_window));
		else
		{	
			g_warning (_("Invalid music directory."));
			gimmix_tag_editor_error (dir_error);
		}	
		gimmix_free_song_info (info);
		g_free (song);
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
												GTK_BUTTONS_YES_NO,
												"<b>%s: </b><span size=\"large\">%s</span>",
												_("ERROR"),
												error_text);
	gtk_window_set_resizable (GTK_WINDOW(error_dialog), FALSE);
    g_signal_connect (error_dialog,
					"response",
					G_CALLBACK (cb_gimmix_tag_editor_error_response),
					NULL);
	
    gtk_widget_show_all (error_dialog);
    
    return;
}

static void
cb_gimmix_tag_editor_error_response (GtkDialog *dialog, gint arg1, gpointer data)
{
	if (arg1 == GTK_RESPONSE_YES)
	{
		extern GtkWidget *pref_notebook;
		gtk_notebook_set_current_page (GTK_NOTEBOOK(pref_notebook), 2);
		gimmix_prefs_dialog_show ();
	}
	
	gtk_widget_destroy (GTK_WIDGET(dialog));
	
	return;
}

