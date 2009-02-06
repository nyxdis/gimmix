/*
 * gimmix-tagedit.c
 *
 * Copyright (C) 2006-2009 Priyank Gosalia
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

#ifdef HAVE_TAGEDITOR
TagLib_File 	*file = NULL;
TagLib_Tag 	*tag = NULL;
#endif

static GtkWidget	*tag_title;
static GtkWidget	*tag_file;
static GtkWidget	*tag_artist;
static GtkWidget	*tag_album;
static GtkWidget	*tag_comment;
static GtkWidget	*tag_track_spin;
static GtkWidget	*tag_year_spin;
static GtkWidget	*tag_genre;
static GtkWidget	*tag_info_bitrate;
static GtkWidget	*tag_info_channels;
static GtkWidget	*tag_info_length;
GtkWidget	*tag_editor_window;
static GtkWidget	*tag_editor_cover_image;

#ifdef HAVE_TAGEDITOR
/* Save action */
static void	gimmix_tag_editor_save (GtkWidget *button, gpointer data);
#endif

/* Close action */
static void gimmix_tag_editor_close (GtkWidget *widget, gpointer data);

/* error dialog callback */
static void cb_gimmix_tag_editor_error_response (GtkDialog *dialog, gint arg1, gpointer data);

#ifdef HAVE_TAGEDITOR
const gchar *dir_error = "You have specified an invalid music directory. Do you want to specify the correct music directory now ?";
#endif

void
gimmix_tag_editor_widgets_init (void)
{
	GtkWidget *widget;
	
	tag_editor_window = glade_xml_get_widget (xml, "tag_editor_window");
	tag_title = glade_xml_get_widget (xml,"entry_title");
	tag_file = glade_xml_get_widget (xml,"entry_filename");
	tag_artist = glade_xml_get_widget (xml,"entry_artist");
	tag_album = glade_xml_get_widget (xml,"entry_album");
	tag_comment = glade_xml_get_widget (xml,"entry_comment");
	tag_year_spin = glade_xml_get_widget (xml, "tag_year");
	tag_track_spin = glade_xml_get_widget (xml, "tag_track");
	tag_genre = glade_xml_get_widget (xml,"combo_genre");
	tag_info_length = glade_xml_get_widget (xml, "info_length");
	tag_info_channels = glade_xml_get_widget (xml, "info_channels");
	tag_info_bitrate = glade_xml_get_widget (xml, "info_bitrate");
	tag_editor_cover_image = glade_xml_get_widget (xml, "gimmix_tagedit_cover_image");
	
	widget = glade_xml_get_widget (xml, "tag_editor_save");
	#ifdef HAVE_TAGEDITOR
	g_signal_connect (G_OBJECT(widget), "clicked", G_CALLBACK(gimmix_tag_editor_save), NULL);
	#else
	gtk_widget_set_sensitive (widget, FALSE);
	#endif

	widget = glade_xml_get_widget (xml, "tag_editor_close");
	g_signal_connect (G_OBJECT(widget),
				"clicked",
				G_CALLBACK(gimmix_tag_editor_close),
				NULL);
	g_signal_connect (G_OBJECT(tag_editor_window),
				"delete-event",
				G_CALLBACK(gtk_widget_hide_on_delete),
				NULL);
	
	return;
}

/* Load the tag editor */
gboolean
gimmix_tag_editor_populate (const void *song)
{
	GtkTreeModel 	*genre_model;
	gchar 		*info;
	gint 		n;
	guint		year = 0;
	guint		track = 0;
	gchar		*title = NULL;
	gchar		*artist = NULL;
	gchar		*album = NULL;
	gchar		*genre = NULL;
	gchar		*comment = NULL;
	
	if (!song)
		return FALSE;
	
	#ifdef HAVE_TAGEDITOR
	const TagLib_AudioProperties *properties;
	if (!g_file_test (song, G_FILE_TEST_EXISTS))
		return FALSE;
		
	file = taglib_file_new (song);
	if (file == NULL)
		return FALSE;

	taglib_set_strings_unicode (FALSE);
	
	tag = taglib_file_tag (file);
	properties = taglib_file_audioproperties (file);
	#else
	mpd_Song *foo = (mpd_Song*) g_malloc0 (sizeof(mpd_Song));
	memcpy (foo, song, sizeof(mpd_Song));
	#endif
	
	#ifdef HAVE_TAGEDITOR
	year = taglib_tag_year (tag);
	track = taglib_tag_track (tag);
	title = g_strstrip (taglib_tag_title(tag));
	artist = g_strstrip (taglib_tag_artist(tag));
	album = g_strstrip (taglib_tag_album(tag));
	comment = g_strstrip (taglib_tag_comment(tag));
	genre = taglib_tag_genre (tag);
	#else
	if (foo->date)
	{
		year = atoi (foo->date);
	}
	if (foo->track)
	{
		track = atoi (foo->track);
	}
	title = foo->title;
	artist = foo->artist;
	album = foo->album;
	comment = foo->comment;
	genre = foo->genre;
	#endif
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(tag_year_spin), year);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(tag_track_spin), track);

	gtk_entry_set_text (GTK_ENTRY(tag_file),
				#ifdef HAVE_TAGEDITOR
				song
				#else
				foo->file
				#endif
				);
	if (title)
	{
		gtk_entry_set_text (GTK_ENTRY(tag_title), title);
	}
	if (artist)
	{
		gtk_entry_set_text (GTK_ENTRY(tag_artist), artist);
	}
	if (album)
	{
		gtk_entry_set_text (GTK_ENTRY(tag_album), album);
	}
	if (comment)
	{
		gtk_entry_set_text (GTK_ENTRY(tag_comment), comment);
	}
	if (genre)
	{
		gtk_combo_box_append_text (GTK_COMBO_BOX(tag_genre), genre);
	}

	genre_model = gtk_combo_box_get_model (GTK_COMBO_BOX(tag_genre));
	n = gtk_tree_model_iter_n_children (genre_model, NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX(tag_genre), n-1);

	/* Audio Information */
	#ifdef HAVE_TAGEDITOR
	guint sec = taglib_audioproperties_length(properties) % 60;
    	guint min = (taglib_audioproperties_length(properties) - sec) / 60;
	info = g_strdup_printf ("%02i:%02i", min, sec);
	gtk_label_set_text (GTK_LABEL(tag_info_length), info);
	#else
	char *tok = NULL;
	info = (char*) g_malloc0 (sizeof(char)*32);
	gimmix_get_progress_status (gmo, NULL, info);
	tok = strtok (info, "/");
	tok = strtok (NULL, "/");
	g_strstrip (tok);
	gtk_label_set_text (GTK_LABEL(tag_info_length), tok);
	#endif
	g_free (info);

	#ifdef HAVE_TAGEDITOR
	info = g_strdup_printf ("%i Kbps", taglib_audioproperties_bitrate(properties));
	#else
	info = g_strdup_printf ("%i Kbps", mpd_status_get_bitrate(gmo));
	#endif
	gtk_label_set_text (GTK_LABEL(tag_info_bitrate), info);
	g_free (info);
	
	#ifdef HAVE_TAGEDITOR
	info = g_strdup_printf ("%i", taglib_audioproperties_channels(properties));
	#else
	info = g_strdup_printf ("%i", mpd_status_get_channels(gmo));
	#endif
	gtk_label_set_text (GTK_LABEL(tag_info_channels), info);
	g_free (info);
	
	#ifdef HAVE_TAGEDITOR
	taglib_tag_free_strings ();
	#else
	free (foo);
	#endif
	
	return TRUE;
}

static void
gimmix_tag_editor_close (GtkWidget *widget, gpointer data)
{
	#ifdef HAVE_TAGEDITOR
	taglib_tag_free_strings ();
	if (file)
    	{
    		taglib_file_free (file);
    	}
	#endif
	gtk_widget_hide (tag_editor_window);

	return;
}

#ifdef HAVE_TAGEDITOR
static void
gimmix_tag_editor_save (GtkWidget *button, gpointer data)
{
	gint		year;
	gint		track;
	gchar		*genre = NULL;
	gchar		*title = NULL;
	gchar		*artist = NULL;
	gchar		*album = NULL;
	gchar		*comment = NULL;

	year = gtk_spin_button_get_value (GTK_SPIN_BUTTON(tag_year_spin));
	taglib_tag_set_year (tag, year);

	track = gtk_spin_button_get_value (GTK_SPIN_BUTTON(tag_track_spin));
	taglib_tag_set_track (tag, track);

	title = g_strdup (gtk_entry_get_text (GTK_ENTRY(tag_title)));
	artist = g_strdup (gtk_entry_get_text (GTK_ENTRY(tag_artist)));
	album = g_strdup (gtk_entry_get_text (GTK_ENTRY(tag_album)));
	comment = g_strdup (gtk_entry_get_text (GTK_ENTRY(tag_comment)));
	genre = gtk_combo_box_get_active_text (GTK_COMBO_BOX(tag_genre));

	taglib_tag_set_title (tag, g_strchomp(title));
	taglib_tag_set_artist (tag, g_strchomp(artist));
	taglib_tag_set_album (tag, g_strchomp(album));
	taglib_tag_set_comment (tag, g_strchomp(comment));
	taglib_tag_set_genre (tag, genre);
	
	/* update the mpd database */
	mpd_database_update_dir (gmo, "/");
	
	/* set the song info a few seconds after update */
	mpd_status_update (gmo);
	
	/* free the strings */
	taglib_tag_free_strings ();
	taglib_file_save (file);
	g_free (title);
	g_free (artist);
	g_free (album);
	g_free (comment);
	
	return;
}
#endif

void
gimmix_tag_editor_show (void)
{
	GimmixStatus 	status;
	mpd_Song	*info;
	gchar		*song = NULL;
	
	status = gimmix_get_status (gmo);
	
	if (status == PLAY || status == PAUSE)
	{
		info = mpd_playlist_get_current_song (gmo);
		#if HAVE_TAGEDITOR
		song = g_strdup_printf ("%s/%s", cfg_get_key_value(conf, "music_directory"), info->file);
		if (gimmix_tag_editor_populate (song))
			gtk_widget_show (GTK_WIDGET(tag_editor_window));
		#else
		if (gimmix_tag_editor_populate (info))
			gtk_widget_show (GTK_WIDGET(tag_editor_window));
		#endif
		else
		{	
			#ifdef HAVE_TAGEDITOR
			g_warning (_("Invalid music directory."));
			gimmix_tag_editor_error (dir_error);
			#else
			gimmix_tag_editor_error (_("An error occurred while trying to get song information. Please try again."));
			#endif
		}	
		g_free (song);
	}
	
	return;
}

void
gimmix_tag_editor_set_cover_image (GdkPixbuf *image)
{
	gtk_image_set_from_pixbuf (GTK_IMAGE(tag_editor_cover_image), image);
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

