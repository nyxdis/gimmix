/*
 * gimmix-playlist.c
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

#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "gimmix-playlist.h"
#include "gimmix-tagedit.h"

#define GIMMIX_MEDIA_ICON 	"gimmix_logo_small.png"
#define GIMMIX_PLAYLIST_ICON 	"gimmix_playlist.png"

typedef enum {
	SONG = 1,
	DIR,
	PLAYLIST
} GimmixFileType;

extern MpdObj		*gmo;
extern GladeXML 	*xml;
extern ConfigFile	conf;

static gchar *dir_error = "You have specified an invalid music directory. Please specify the correct music directory in the preferences.";

GtkWidget 	*search_combo;
GtkWidget	*search_entry;
GtkWidget	*search_box;
	
GtkWidget		*gimmix_statusbar;
GtkWidget		*current_playlist_treeview;
GtkWidget		*library_treeview;
GtkWidget		*playlists_treeview;
GtkTreeSelection	*current_playlist_selection;
GtkTreeSelection	*library_selection;

extern GtkWidget	*tag_editor_window;

gchar			*loaded_playlist;

static void		gimmix_search_init (void);
static void		gimmix_library_search (gint, gchar *);
static void		gimmix_library_and_playlists_populate (void);
static void		gimmix_update_library_with_dir (gchar *);
static void		gimmix_current_playlist_popup_menu (void);
static void		gimmix_library_popup_menu (void);
static void		gimmix_playlists_popup_menu (void);
static gchar*		gimmix_path_get_parent_dir (gchar *);
static void 		gimmix_load_playlist (gchar *);
static void		gimmix_display_total_playlist_time (void);

/* Callbacks */
/* Current playlist callbacks */
static void		cb_current_playlist_double_click (GtkTreeView *);
static void		cb_current_playlist_right_click (GtkTreeView *treeview, GdkEventButton *event);
static void		cb_repeat_menu_toggled (GtkCheckMenuItem *item, gpointer data);
static void		cb_shuffle_menu_toggled (GtkCheckMenuItem *item, gpointer data);
static void		cb_current_playlist_delete_press (GtkWidget *widget, GdkEventKey *event, gpointer data);
static void		gimmix_current_playlist_remove_song (void);
static void		gimmix_current_playlist_song_info (void);
static void		gimmix_current_playlist_clear (void);
static void		gimmix_library_update (GtkWidget *widget, gpointer data);
static gboolean		gimmix_update_player_status (gpointer data);

/* Library browser callbacks */
static void		cb_library_dir_activated (gpointer data);
static void		gimmix_library_song_info (void);
static void 		cb_playlist_activated (GtkTreeView *);
static void		cb_library_right_click (GtkTreeView *treeview, GdkEventButton *event);
static void		cb_search_keypress (GtkWidget *widget, GdkEventKey *event, gpointer data);

/* Playlist browser callbacks */
static void		gimmix_update_playlists_treeview (void);
static void		gimmix_playlist_save_dialog_show (void);
static void 		cb_gimmix_playlist_save_response (GtkDialog *dlg, gint arg1, gpointer dialog);
static void		cb_playlists_right_click (GtkTreeView *treeview, GdkEventButton *event);
static bool		cb_all_playlist_button_press (GtkTreeView *treeview, GdkEventButton *event);
static void		cb_gimmix_playlist_remove ();
static void		cb_gimmix_playlist_load ();
static void		cb_playlists_delete_press (GtkWidget *widget, GdkEventKey *event, gpointer data);
static void		cb_library_popup_add_clicked (GtkWidget *widget, gpointer data);

void
gimmix_playlist_init (void)
{
	GtkTreeModel		*current_playlist_model;
	GtkListStore		*current_playlist_store;
	GtkCellRenderer		*current_playlist_renderer;
	
	current_playlist_treeview = glade_xml_get_widget (xml, "current_playlist_treeview");
	playlists_treeview = glade_xml_get_widget (xml, "playlists_treeview");
	library_treeview = glade_xml_get_widget (xml, "album");
	
	current_playlist_renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (current_playlist_treeview),
							-1,
							_("Song"),
							current_playlist_renderer,
							"markup", 0,
							NULL);
	current_playlist_store = gtk_list_store_new (3,
						G_TYPE_STRING, 	/* name (0) */
						G_TYPE_STRING, 	/* path (1) */
						G_TYPE_INT); 	/* id (2) */
	current_playlist_model	= GTK_TREE_MODEL (current_playlist_store);
	current_playlist_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(current_playlist_treeview));
	gtk_tree_selection_set_mode (current_playlist_selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_model (GTK_TREE_VIEW (current_playlist_treeview), current_playlist_model);
	gimmix_statusbar = glade_xml_get_widget (xml, "gimmix_status");
	
	g_signal_connect (current_playlist_treeview, "row-activated", G_CALLBACK(cb_current_playlist_double_click), NULL);
	g_signal_connect (current_playlist_treeview, "button-press-event", G_CALLBACK(cb_all_playlist_button_press), NULL);
	g_signal_connect (current_playlist_treeview, "button-release-event", G_CALLBACK (cb_current_playlist_right_click), NULL);
	g_signal_connect (current_playlist_treeview, "key_release_event", G_CALLBACK (cb_current_playlist_delete_press), NULL);
	g_object_unref (current_playlist_model);
	g_signal_connect (playlists_treeview, "button-release-event", G_CALLBACK (cb_playlists_right_click), NULL);
	g_signal_connect (playlists_treeview, "key_release_event", G_CALLBACK (cb_playlists_delete_press), NULL);
	
	/* populate the file browser */
	gimmix_library_and_playlists_populate ();
	
	/* Initialize playlist search */
	gimmix_search_init ();
	
	loaded_playlist = NULL;
	return;
}

static void
gimmix_search_init (void)
{
	search_box = glade_xml_get_widget (xml, "search_box");
	search_combo = glade_xml_get_widget (xml, "search_combo");
	search_entry = glade_xml_get_widget (xml, "search_entry");
	
	gtk_combo_box_set_active (GTK_COMBO_BOX(search_combo), 0);
	g_signal_connect (G_OBJECT(search_entry), "key_release_event", G_CALLBACK(cb_search_keypress), NULL);
	
	if (strncasecmp(cfg_get_key_value(conf, "enable_search"), "true", 4) != 0)
		gtk_widget_hide (search_box);
	
	return;
}

void
gimmix_update_current_playlist (void)
{
	GtkTreeModel	*current_playlist_model;
	GtkListStore	*current_playlist_store;
	GtkTreeIter		current_playlist_iter;
	gint 			new;
	MpdData 		*data;
	gint			current_song_id = -1;

	new = mpd_playlist_get_playlist_id (gmo);
	data = mpd_playlist_get_changes (gmo, 0);
	current_song_id = mpd_player_get_current_song_id (gmo);
	
	current_playlist_model = gtk_tree_view_get_model (GTK_TREE_VIEW(current_playlist_treeview));
	current_playlist_store = GTK_LIST_STORE (current_playlist_model);
	gtk_list_store_clear (current_playlist_store);
	while (data != NULL)
	{
		gchar 	*title;
		
		if (data->song->id == current_song_id)
		{
			if (data->song->title != NULL)
			{
				if (data->song->artist)
				title = g_markup_printf_escaped ("<span size=\"medium\"weight=\"bold\">%s - %s</span>", data->song->artist, data->song->title);
				else
					title = g_markup_printf_escaped ("<span size=\"medium\"weight=\"bold\">%s</span>", data->song->title);
			}
			else
			{
				title = g_markup_printf_escaped ("<span size=\"medium\"weight=\"bold\">%s</span>", data->song->file);
			}
		}
		else
		{	
			if (data->song->title != NULL)
			{
				if (data->song->artist)
				title = g_markup_printf_escaped ("%s - %s", data->song->artist, data->song->title);
				else
					title = g_markup_printf_escaped ("%s", data->song->title);
			}
			else
			{
				title = g_markup_printf_escaped (data->song->file);
			}
		}
		gtk_list_store_append (current_playlist_store, &current_playlist_iter);
		gtk_list_store_set (current_playlist_store, 
							&current_playlist_iter,
							0, title,
							1, data->song->file,
							2, data->song->id,
							-1);
		data = mpd_data_get_next (data);
		g_free (title);
	}
	gtk_tree_view_set_model (GTK_TREE_VIEW (current_playlist_treeview), current_playlist_model);
	gimmix_display_total_playlist_time ();
	
	mpd_data_free (data);
	
	return;
}

static void
gimmix_display_total_playlist_time (void)
{
	MpdData 	*data = NULL;
	gchar		*time_string;
	gint		time = 0;
	
	data = mpd_playlist_get_changes (gmo, 0);
	if (mpd_playlist_get_playlist_length(gmo) == 0)
	{
		gtk_widget_hide (gimmix_statusbar);
		return;
	}
	
	while (data != NULL)
	{
		time += data->song->time;
		data = mpd_data_get_next (data);
	}
	
	if (time > 0)
	{
		time_string = g_strdup_printf ("%d %s, %s%d %s", mpd_playlist_get_playlist_length(gmo), _("Items"), _("Total Playlist Time: "), time/60, _("minutes"));
		gtk_label_set_text (GTK_LABEL(gimmix_statusbar), time_string);
		gtk_widget_show (gimmix_statusbar);
		g_free (time_string);
	}
	else
	{
		gtk_widget_hide (gimmix_statusbar);
	}
	
	return;
}

static void
gimmix_library_and_playlists_populate (void)
{
	GtkTreeModel 		*dir_model;
	GtkTreeModel		*pls_model;
	GtkTreeIter  		dir_iter;
	GtkTreeIter			pls_iter;
	GtkCellRenderer 	*dir_renderer;
	GtkCellRenderer		*pls_renderer;
	GtkListStore 		*dir_store;
	GtkListStore		*pls_store;
	MpdData 			*data = NULL;
	GdkPixbuf 			*dir_pixbuf;
	GdkPixbuf			*song_pixbuf;
	GdkPixbuf			*pls_pixbuf;
	gchar				*path;

	library_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(library_treeview));
	gtk_tree_selection_set_mode (library_selection, GTK_SELECTION_MULTIPLE);
	dir_renderer		= gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (library_treeview),
							-1,
							"Icon",
							dir_renderer,
							"pixbuf", 0,
							NULL);
	
	dir_renderer 		= gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (library_treeview),
							-1,
							"Albums",
							dir_renderer,
							"text", 1,
							NULL);
	
	pls_renderer 		= gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (playlists_treeview), 
							-1,
							"PlsIcon",
							pls_renderer,
							"pixbuf", 0,
							NULL);			
							
	pls_renderer 		= gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (playlists_treeview), 
							-1,
							"Playlists",
							pls_renderer,
							"text", 1,
							NULL);
	
	dir_store 	= gtk_list_store_new (4, 
								GDK_TYPE_PIXBUF, 	/* icon (0) */
								G_TYPE_STRING, 		/* name (1) */
								G_TYPE_STRING,		/* path (2) */
								G_TYPE_INT);		/* type DIR/SONG (3) */
	
	pls_store 	= gtk_list_store_new (2, 
								GDK_TYPE_PIXBUF, 	/* icon */
								G_TYPE_STRING);		/* name */
	
	path = gimmix_get_full_image_path (GIMMIX_MEDIA_ICON);
	song_pixbuf = gdk_pixbuf_new_from_file_at_size (path, 12, 12, NULL);
	g_free (path);
	dir_pixbuf	= gtk_widget_render_icon (GTK_WIDGET(library_treeview), GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_MENU, NULL);
	path = gimmix_get_full_image_path (GIMMIX_PLAYLIST_ICON);
	pls_pixbuf	= gdk_pixbuf_new_from_file_at_size (path, 16, 16, NULL);
	g_free (path);
	
	for (data = mpd_database_get_directory(gmo, NULL); data != NULL; data = mpd_data_get_next(data))
	{	
		if (data->type == MPD_DATA_TYPE_DIRECTORY)
		{
			gtk_list_store_append (dir_store, &dir_iter);
			path = g_path_get_basename (data->directory);
			gtk_list_store_set (dir_store, &dir_iter,
						0, dir_pixbuf,
						1, path,
						2, data->directory,
						3, DIR,
						-1);
			g_free (path);
		}
		else if (data->type == MPD_DATA_TYPE_SONG)
		{
			gchar *title;

			gtk_list_store_append (dir_store, &dir_iter);
			title = data->song->title ? g_strdup (data->song->title) : g_strdup (data->song->file);
			gtk_list_store_set (dir_store, &dir_iter,
								0, song_pixbuf,
								1, title,
								2, data->song->file,
								3, SONG,
								-1);
			free (title);
		}
		else if (data->type == MPD_DATA_TYPE_PLAYLIST)
		{
			gchar *name;
			
			name = data->playlist;
			gtk_list_store_append (pls_store, &pls_iter);
			gtk_list_store_set (pls_store, &pls_iter,
								0, pls_pixbuf,
								1, name,
								-1);
		}
	}

	mpd_data_free (data);

	dir_model	= GTK_TREE_MODEL (dir_store);
	gtk_tree_view_set_model (GTK_TREE_VIEW (library_treeview), dir_model);
	
	pls_model 	= GTK_TREE_MODEL (pls_store);
	gtk_tree_view_set_model (GTK_TREE_VIEW (playlists_treeview), pls_model);
	
	g_signal_connect (library_treeview, "row-activated", G_CALLBACK(cb_library_dir_activated), NULL);
	g_signal_connect (playlists_treeview, "row-activated", G_CALLBACK(cb_playlist_activated), NULL);
	g_signal_connect (library_treeview, "button-press-event", G_CALLBACK(cb_all_playlist_button_press), NULL);
	g_signal_connect (library_treeview, "button-release-event", G_CALLBACK(cb_library_right_click), NULL);
	g_object_unref (dir_model);
	g_object_unref (pls_model);
	g_object_unref (dir_pixbuf);
	g_object_unref (song_pixbuf);
	g_object_unref (pls_pixbuf);
	
	return;
}

static void
gimmix_library_search (gint type, gchar *text)
{
	if (!text)
		return;

	MpdData 		*data;
	GtkTreeModel	*directory_model;
	GtkListStore	*dir_store;
	GdkPixbuf 		*song_pixbuf;
	GtkTreeIter 	dir_iter;
	gchar 			*path;

	switch (type)
	{
		/* Search by Artist */
		case 0: data = mpd_database_find (gmo, MPD_TABLE_ARTIST, text, FALSE);
				break;
		
		/* Search by Album */
		case 1: data = mpd_database_find (gmo, MPD_TABLE_ALBUM, text, FALSE);
				break;
		
		/* Search by Title */
		case 2: data = mpd_database_find (gmo, MPD_TABLE_TITLE, text, FALSE);
				break;
		
		/* Search by File name */
		case 3: data = mpd_database_find (gmo, MPD_TABLE_FILENAME, text, FALSE);
				break;
				
		default: return;
	}
	
	directory_model = gtk_tree_view_get_model (GTK_TREE_VIEW (library_treeview));
	dir_store 	= GTK_LIST_STORE (directory_model);

	/* Clear the stores */
	gtk_list_store_clear (dir_store);
	
	if (!data)
	{
		GdkPixbuf *icon	= gtk_widget_render_icon (GTK_WIDGET(library_treeview), "gtk-dialog-error",
						GTK_ICON_SIZE_MENU,
						NULL);
		gtk_list_store_append (dir_store, &dir_iter);
		gtk_list_store_set (dir_store, &dir_iter,
								0, icon,
								1, _("No Result"),
								2, NULL,
								3, 0,
								-1);
		directory_model = GTK_TREE_MODEL (dir_store);
		gtk_tree_view_set_model (GTK_TREE_VIEW(library_treeview), directory_model);
		g_object_unref (icon);
		return;
	}

	path = gimmix_get_full_image_path (GIMMIX_MEDIA_ICON);
	song_pixbuf = gdk_pixbuf_new_from_file_at_size (path, 12, 12, NULL);
	g_free (path);

	for (data; data!=NULL; data = mpd_data_get_next (data))
	{
		if (data->type == MPD_DATA_TYPE_SONG)
		{
			gchar *title;
			
			title = (data->song->title!=NULL) ? g_strdup (data->song->title) : g_path_get_basename(data->song->file);
			gtk_list_store_append (dir_store, &dir_iter);
			gtk_list_store_set (dir_store, &dir_iter,
								0, song_pixbuf,
								1, title,
								2, data->song->file,
								3, SONG,
								-1);
			g_free (title);
		}
	}
	
	mpd_data_free (data);

	directory_model = GTK_TREE_MODEL (dir_store);
	gtk_tree_view_set_model (GTK_TREE_VIEW(library_treeview), directory_model);
	g_object_unref (song_pixbuf);
	
	return;
}

static void
cb_search_keypress (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	gint		index;
	gchar		text[20];
	
	strcpy (text, gtk_entry_get_text (GTK_ENTRY(search_entry)));
	if ( (strlen (text)) <= 1 )
	{
		gimmix_update_library_with_dir ("/");
		return;
	}
	index = gtk_combo_box_get_active (GTK_COMBO_BOX(search_combo));
	gimmix_library_search (index, text);
	
	return;
}

static void
cb_current_playlist_delete_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (event->keyval != GDK_Delete)
		return;
	
	gimmix_current_playlist_remove_song ();
	
	return;
}

static void
cb_current_playlist_double_click (GtkTreeView *treeview)
{
	GtkTreeModel 		*model;
	GList				*list;
	GtkTreeIter			iter;
	gint 				id;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	list = gtk_tree_selection_get_selected_rows (current_playlist_selection, &model);
	gtk_tree_model_get_iter (model, &iter, list->data);
	gtk_tree_model_get (model, &iter, 2, &id, -1);
	mpd_player_play_id (gmo, id);
	mpd_status_update (gmo);
	
	/* free the list */
	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);
	
	return;
}

/* This keeps multiple items selected in case of a right click */
static bool
cb_all_playlist_button_press (GtkTreeView *treeview, GdkEventButton *event)
{
	GtkTreeSelection *selection;
	
	if (event->button == 3)
	{
		GtkTreePath *path;
		
		if (gtk_tree_view_get_path_at_pos(treeview, event->x, event->y, &path, NULL, NULL, NULL))
		{
			selection = gtk_tree_view_get_selection (treeview);
			bool sel = gtk_tree_selection_path_is_selected (selection, path);
			gtk_tree_path_free (path);
			
			if (sel)
				return TRUE;
		}
	}
	
	return FALSE;
}

static void
cb_library_dir_activated (gpointer data)
{
	GtkTreeModel 		*model;
	GtkTreeIter 		iter;
	GList				*list;
	gchar				*path;
	GimmixFileType		type;
	MpdData				*mpddata;
	gint				id;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (library_treeview));
	list = gtk_tree_selection_get_selected_rows (library_selection, &model);
	
	if (gtk_tree_selection_count_selected_rows (library_selection) == 1)
	{
		gtk_tree_model_get_iter (model, &iter, list->data);
		gtk_tree_model_get (model, &iter, 2, &path, 3, &type, -1);
		
		if (type == DIR)
		{	
			gimmix_update_library_with_dir (path);
		}
		else if (type == SONG)
		{
			mpd_playlist_add (gmo, path);
		}
		
		g_free (path);
	}
	
	mpddata = mpd_playlist_get_changes (gmo, mpd_playlist_get_playlist_id(gmo));
	if (strncasecmp(cfg_get_key_value(conf, "play_on_add"), "true", 4) == 0)
	{
		id = mpddata->song->id;
		mpd_player_play_id (gmo, mpddata->song->id);
		gimmix_set_song_info ();
	}
	mpd_status_update (gmo);
	mpd_data_free (mpddata);
		
	/* free the list */
	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);
	
	return;
}

static void
cb_library_popup_add_clicked (GtkWidget *widget, gpointer data)
{
	GtkTreeModel 		*model;
	GtkTreeIter 		iter;
	GList				*list;
	gchar				*path;
	GimmixFileType		type;
	MpdData				*mpddata;
	gint				id;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (library_treeview));
	list = gtk_tree_selection_get_selected_rows (library_selection, &model);
	
	if (gtk_tree_selection_count_selected_rows (library_selection) == 1)
	{
		gtk_tree_model_get_iter (model, &iter, list->data);
		gtk_tree_model_get (model, &iter, 2, &path, 3, &type, -1);
		
		if (type == DIR)
		{
			mpd_playlist_queue_add (gmo, path);
		}
		else if (type == SONG)
		{
			mpd_playlist_add (gmo, path);
		}
		
		g_free (path);
	}
	else
	while (list != NULL)
	{
		gtk_tree_model_get_iter (model, &iter, list->data);
		gtk_tree_model_get (model, &iter, 2, &path, 3, &type, -1);
	
		
		if (type == DIR)
		{	
			mpd_playlist_queue_add (gmo, path);
			g_free (path);
		}
		
		if (type == SONG)
		{
			mpd_playlist_queue_add (gmo, path);
			g_free (path);
		}
		
		list = g_list_next (list);
	}
	
	mpd_playlist_queue_commit (gmo);
	mpddata = mpd_playlist_get_changes (gmo, mpd_playlist_get_playlist_id(gmo));
	if (strncasecmp(cfg_get_key_value(conf, "play_on_add"), "true", 4) == 0)
	{
		id = mpddata->song->id;
		mpd_player_play_id (gmo, mpddata->song->id);
		gimmix_set_song_info ();
	}
	mpd_status_update (gmo);
	mpd_data_free (mpddata);
		
	/* free the list */
	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);
	
	return;
}

static void
cb_playlist_activated (GtkTreeView *treeview)
{
	GtkTreeModel 		*model;
	GtkTreeSelection 	*selection;
	GtkTreeIter 		iter;
	gchar 				*pl_name;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

	if ( gtk_tree_selection_get_selected (selection, &model, &iter) )
		gtk_tree_model_get (model, &iter, 1, &pl_name, -1);
	
	mpd_playlist_queue_load (gmo, pl_name);
	gimmix_current_playlist_clear ();
	mpd_playlist_queue_commit (gmo);
	
	gimmix_load_playlist (pl_name);
	g_free (pl_name);
}

static void
gimmix_load_playlist (gchar *pls)
{
	/* unload the old playlist */
	if (loaded_playlist != NULL)
		g_free (loaded_playlist);
	
	/* load the new playlist*/
	loaded_playlist = g_strdup (pls);
	
	return;
}

static void
gimmix_update_playlists_treeview (void)
{
	GtkTreeModel	*pls_treemodel;
	GtkTreeIter		pls_treeiter;
	GtkListStore	*pls_liststore;
	GdkPixbuf		*pls_pixbuf;
	MpdData			*data;
	gchar			*name;
	gchar			*path;
	
	pls_treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW(playlists_treeview));
	pls_liststore = GTK_LIST_STORE (pls_treemodel);
	
	gtk_list_store_clear (pls_liststore);
	
	path = gimmix_get_full_image_path (GIMMIX_PLAYLIST_ICON);
	pls_pixbuf	= gdk_pixbuf_new_from_file_at_size (path, 16, 16, NULL);
	g_free (path);
	
	for (data = mpd_database_get_directory(gmo, NULL); data != NULL; data = mpd_data_get_next(data))
	{
		if (data->type == MPD_DATA_TYPE_PLAYLIST)
		{
			gtk_list_store_append (pls_liststore, &pls_treeiter);
			name = data->playlist;
			gtk_list_store_set (pls_liststore, &pls_treeiter,
								0, pls_pixbuf,
								1, name,
								-1);
		}
	}
	g_object_unref (pls_pixbuf);
	mpd_data_free (data);

	pls_treemodel = GTK_TREE_MODEL (pls_liststore);
	gtk_tree_view_set_model (GTK_TREE_VIEW(playlists_treeview), pls_treemodel);

	return;
}

static void
gimmix_update_library_with_dir (gchar *dir)
{
	GtkTreeModel		*directory_model;
	GtkListStore		*dir_store;
	GtkTreeIter			dir_iter;
	GdkPixbuf			*dir_pixbuf;
	GdkPixbuf			*song_pixbuf;
	MpdData				*data;
	gchar				*parent;
	gchar				*path;
	gchar				*directory;
	
	directory_model = gtk_tree_view_get_model (GTK_TREE_VIEW (library_treeview));
	dir_store 	= GTK_LIST_STORE (directory_model);

	if (!strlen(dir))
		dir = "/";

	/* Clear the stores */
	gtk_list_store_clear (dir_store);

	dir_pixbuf 	= gtk_widget_render_icon (GTK_WIDGET(library_treeview),
										GTK_STOCK_DIRECTORY,
										GTK_ICON_SIZE_SMALL_TOOLBAR,
										NULL);
	path = gimmix_get_full_image_path (GIMMIX_MEDIA_ICON);
	song_pixbuf = gdk_pixbuf_new_from_file_at_size (path, 12, 12, NULL);
	g_free (path);
	
	if (dir != "/")
	{	
		parent = gimmix_path_get_parent_dir (dir);
		gtk_list_store_append (dir_store, &dir_iter);
		gtk_list_store_set (dir_store, &dir_iter,
								0, dir_pixbuf,
								1, "..",
								2, parent,
								3, DIR,
								-1);
		g_free (parent);
	}
	
	for (data = mpd_database_get_directory(gmo, dir); data != NULL; data = mpd_data_get_next(data))
	{	
		if (data->type == MPD_DATA_TYPE_DIRECTORY)
		{
			directory = g_path_get_basename(data->directory);
			gtk_list_store_append (dir_store, &dir_iter);
			gtk_list_store_set (dir_store, &dir_iter,
								0, dir_pixbuf,
								1, directory,
								2, data->directory,
								3, DIR,
								-1);
			g_free (directory);
		}
		else if (data->type == MPD_DATA_TYPE_SONG)
		{
			gchar *title;
			
			gtk_list_store_append (dir_store, &dir_iter);
			title = data->song->title ? g_strdup (data->song->title) : g_path_get_basename(data->song->file);
			gtk_list_store_set (dir_store, &dir_iter,
								0, song_pixbuf,
								1, title,
								2, data->song->file,
								3, SONG,
								-1);
			g_free (title);
		}
	}
	
	mpd_data_free (data);

	directory_model = GTK_TREE_MODEL (dir_store);
	gtk_tree_view_set_model (GTK_TREE_VIEW(library_treeview), directory_model);
	
	g_object_unref (dir_pixbuf);
	g_object_unref (song_pixbuf);
	
	return;
}

static void
cb_current_playlist_right_click (GtkTreeView *treeview, GdkEventButton *event)
{	
	if (event->button == 3) /* If right click */
	{
		gimmix_current_playlist_popup_menu ();
	}
	
	return;
}

static void
cb_library_right_click (GtkTreeView *treeview, GdkEventButton *event)
{	
	if (event->button == 3) /* If right click */
	{
		gimmix_library_popup_menu ();
	}
	
	return;
}

static void
cb_playlists_right_click (GtkTreeView *treeview, GdkEventButton *event)
{
	if (event->button == 3) /* If right click */
	{
		gimmix_playlists_popup_menu ();
	}
	
	return;
}

static void
gimmix_library_song_info (void)
{
	GtkTreeModel		*model;
	GList				*list;
	GtkTreeIter			iter;
	gchar				*path;
	gchar				*song_path;
	gint				type = -1;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW(library_treeview));
	
	list = gtk_tree_selection_get_selected_rows (library_selection, &model);
	gtk_tree_model_get_iter (model, &iter, list->data);
	gtk_tree_model_get (model, &iter, 2, &path, 3, &type, -1);

	if (type == DIR)
	{
		g_free (path);
		return;
	}
	
	song_path = g_strdup_printf ("%s/%s", cfg_get_key_value(conf, "music_directory"), path);
	if (gimmix_tag_editor_populate (song_path))
	{	
		gtk_widget_show (tag_editor_window);
	}
	else
		gimmix_tag_editor_error (dir_error);
		
	g_free (path);
	g_free (song_path);
	
	/* free the list */
	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);
	
	return;
}

static void
gimmix_current_playlist_song_info (void)
{
	GtkTreeModel		*model;
	GList				*list;
	GtkTreeIter			iter;
	gchar				*path;
	gchar				*song_path;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW(current_playlist_treeview));
	if (gtk_tree_selection_count_selected_rows(current_playlist_selection) != 1)
	return;
	
	list = gtk_tree_selection_get_selected_rows (current_playlist_selection, &model);
	gtk_tree_model_get_iter (model, &iter, list->data);
	gtk_tree_model_get (model, &iter, 1, &path, -1);
	
	song_path = g_strdup_printf ("%s/%s", cfg_get_key_value(conf, "music_directory"), path);
	if (gimmix_tag_editor_populate (song_path))
	{	
		gtk_widget_show (tag_editor_window);
	}
	else
		gimmix_tag_editor_error (dir_error);
		
	g_free (path);
	g_free (song_path);
	
	/* free the list */
	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);
	
	return;
}

static void
gimmix_current_playlist_remove_song (void)
{
	GtkTreeModel		*current_playlist_model;
	GtkTreeIter			iter;
	GList				*list;
	gint				id;

	current_playlist_model = gtk_tree_view_get_model (GTK_TREE_VIEW(current_playlist_treeview));

	list = gtk_tree_selection_get_selected_rows (current_playlist_selection, &current_playlist_model);
	
	while (list != NULL)
	{
		gtk_tree_model_get_iter (current_playlist_model, &iter, list->data);
		gtk_tree_model_get (current_playlist_model, &iter, 2, &id, -1);
		mpd_playlist_delete_id (gmo, id);
		list = list->next;
	}
	
	mpd_status_update (gmo);
	
	/* free the list */
	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);
	
	return;
}	

static void
gimmix_current_playlist_clear (void)
{
	GtkListStore	*current_playlist_store;
	
	current_playlist_store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(current_playlist_treeview)));
	gtk_list_store_clear (GTK_LIST_STORE(current_playlist_store));
	if (mpd_playlist_get_playlist_length (gmo) != 0)
	{
		mpd_playlist_clear (gmo);
		mpd_status_update (gmo);
	}
	if (loaded_playlist != NULL)
	{
		g_free (loaded_playlist);
		loaded_playlist = NULL;
	}
	return;
}

static void
gimmix_current_playlist_popup_menu (void)
{
	GtkWidget *menu, *menu_item;

	menu = gtk_menu_new ();

	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_REMOVE, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (gimmix_current_playlist_remove_song), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_INFO, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (gimmix_current_playlist_song_info), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CLEAR, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (gimmix_current_playlist_clear), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	menu_item = gtk_check_menu_item_new_with_label (_("Repeat"));
	g_signal_connect (G_OBJECT(menu_item), "toggled", G_CALLBACK(cb_repeat_menu_toggled), NULL);
	if (is_gimmix_repeat(gmo))
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menu_item), TRUE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	menu_item = gtk_check_menu_item_new_with_label (_("Shuffle"));
	g_signal_connect (G_OBJECT(menu_item), "toggled", G_CALLBACK(cb_shuffle_menu_toggled), NULL);
	if (is_gimmix_shuffle(gmo))
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menu_item), TRUE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	menu_item = gtk_image_menu_item_new_with_label (_("Save Playlist"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menu_item), GTK_WIDGET(get_image ("gtk-save", GTK_ICON_SIZE_MENU)));
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (gimmix_playlist_save_dialog_show), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	gtk_widget_show (menu);
	gtk_menu_popup (GTK_MENU(menu),
					NULL,
					NULL,
					NULL,
					NULL,
					3,
					gtk_get_current_event_time());
	
	return;
}

static void
gimmix_library_popup_menu (void)
{
	GtkWidget 			*menu;
	GtkWidget 			*menu_item;
	GtkWidget			*image;

	menu = gtk_menu_new ();
	image = gtk_image_new_from_stock ("gtk-refresh", GTK_ICON_SIZE_MENU);
	
	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_ADD, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_library_popup_add_clicked), (gpointer)1);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	if (gtk_tree_selection_count_selected_rows(library_selection) == 1)
	{
		menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_INFO, NULL);
		g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (gimmix_library_song_info), NULL);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
		gtk_widget_show (menu_item);
	}
	
	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	menu_item = gtk_image_menu_item_new_with_label (_("Update Library"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menu_item), image);
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (gimmix_library_update), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	gtk_widget_show (menu);
	gtk_menu_popup (GTK_MENU(menu),
					NULL,
					NULL,
					NULL,
					NULL,
					3,
					gtk_get_current_event_time());
					
	return;
}

static void
gimmix_playlists_popup_menu (void)
{
	GtkWidget *menu, *menu_item;

	menu = gtk_menu_new ();
	
	menu_item = gtk_image_menu_item_new_with_label (_("Load"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menu_item), GTK_WIDGET(get_image ("gtk-open", GTK_ICON_SIZE_MENU)));
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_gimmix_playlist_load), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	menu_item = gtk_image_menu_item_new_with_label (_("Delete"));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menu_item), GTK_WIDGET(get_image ("gtk-delete", GTK_ICON_SIZE_MENU)));
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_gimmix_playlist_remove), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
	
	gtk_widget_show (menu);
	gtk_menu_popup (GTK_MENU(menu),
					NULL,
					NULL,
					NULL,
					NULL,
					3,
					gtk_get_current_event_time());
					
	return;
}

static void
cb_repeat_menu_toggled (GtkCheckMenuItem *item, gpointer data)
{
	gboolean state;
	
	state = gtk_check_menu_item_get_active (item);
	if (state == TRUE)
	{
		mpd_player_set_repeat (gmo, true);
	}
	else if (state == FALSE)
	{
		mpd_player_set_repeat (gmo, false);
	}
	
	return;
}

static void
cb_shuffle_menu_toggled (GtkCheckMenuItem *item, gpointer data)
{
	gboolean state;
	
	state = gtk_check_menu_item_get_active (item);
	if (state == TRUE)
	{
		mpd_player_set_random (gmo, true);
	}
	else if (state == FALSE)
	{
		mpd_player_set_random (gmo, false);
	}
	
	return;
}

void
gimmix_library_update (GtkWidget *widget, gpointer data)
{
	mpd_database_update_dir (gmo, "/");
	gtk_label_set_text (GTK_LABEL(gimmix_statusbar), _("Updating Library..."));
	gtk_widget_show (gimmix_statusbar);
	g_timeout_add (300, (GSourceFunc)gimmix_update_player_status, NULL);

	return;
}

static gboolean
gimmix_update_player_status (gpointer data)
{
	if (mpd_status_db_is_updating (gmo))
		return TRUE;
	else
	{
		gimmix_display_total_playlist_time ();		
		gimmix_update_library_with_dir ("/");
		return FALSE;
	}
	
	return FALSE;
}

static gchar *
gimmix_path_get_parent_dir (gchar *path)
{
	gchar *p, buf[128];
	gchar *ret;

	strcpy (buf, path);
	p = strrchr (buf, '/');
	if (p)
	{	
		*p = 0;
		ret = g_strdup (buf);
 	}
 	else
 	ret = g_strdup ("/");
 	
 	return ret;
}

static void
gimmix_playlist_save_dialog_show (void)
{
	GtkWidget 	*dialog;
	GtkWidget 	*label;
	GtkWidget 	*entry;
	static gchar *message = "Enter the name of the playlist: ";
   
	dialog = gtk_dialog_new_with_buttons (_("Save playlist"),
                                         NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_ACCEPT,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_REJECT,
                                         NULL);
	gtk_window_set_resizable (GTK_WINDOW(dialog), FALSE);
	label = gtk_label_new (message);
	entry = gtk_entry_new ();
	if (loaded_playlist != NULL)
		gtk_entry_set_text (GTK_ENTRY(entry), loaded_playlist);
	
	g_signal_connect_swapped (dialog,
                             "response", 
                             G_CALLBACK (cb_gimmix_playlist_save_response),
                             dialog);
	gtk_misc_set_padding (GTK_MISC(label), 5, 5);
	gtk_dialog_set_has_separator (GTK_DIALOG(dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER((GTK_DIALOG(dialog))->vbox), 10);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), entry);
   
   gtk_widget_show_all (dialog);
   
   return;
}

static void
cb_gimmix_playlist_save_response (GtkDialog *dlg, gint arg1, gpointer dialog)
{
	if (arg1 == GTK_RESPONSE_ACCEPT)
	{	
		if (loaded_playlist != NULL)
		{
			mpd_database_delete_playlist (gmo, loaded_playlist);
			if ((mpd_database_save_playlist (gmo, loaded_playlist)) == MPD_DATABASE_PLAYLIST_EXIST)
				g_print (_("playlist already exists.\n"));
		}
		else
		{
			GList *widget_list;
			const gchar *text;
			
			widget_list = gtk_container_get_children (GTK_CONTAINER(GTK_DIALOG(dialog)->vbox));
			widget_list = widget_list->next;
			text = gtk_entry_get_text (GTK_ENTRY(widget_list->data));
			if ((mpd_database_save_playlist (gmo, (char*)text)) == MPD_DATABASE_PLAYLIST_EXIST)
				g_print (_("playlist already exists.\n"));
			
			g_list_free (widget_list);
			gimmix_load_playlist ((char*)text);
		}
		gimmix_update_playlists_treeview ();
	}	

	gtk_widget_destroy (GTK_WIDGET(dlg));
	
	return;
}

static void
cb_gimmix_playlist_remove ()
{
	GtkTreeModel		*pls_treemodel;
	GtkTreeSelection	*selection;
	GtkTreeIter			iter;
	gchar				*path;

	pls_treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW(playlists_treeview));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(playlists_treeview));
		
	if ( gtk_tree_selection_get_selected (selection, &pls_treemodel, &iter) )
	{
		gtk_tree_model_get (pls_treemodel, &iter,
							1, &path,
							-1);
		mpd_database_delete_playlist (gmo, path);
	}
	
	if (loaded_playlist != NULL)
		if (strcmp (path, loaded_playlist) == 0)
		{
			gimmix_current_playlist_clear ();
		}
	
	gimmix_update_playlists_treeview ();
	g_free (path);
	
	return;
}

static void
cb_gimmix_playlist_load ()
{	
	GtkTreeModel		*pls_treemodel;
	GtkTreeSelection	*selection;
	GtkTreeIter			iter;
	gchar				*path;

	pls_treemodel = gtk_tree_view_get_model (GTK_TREE_VIEW(playlists_treeview));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(playlists_treeview));
	
	if ( gtk_tree_selection_get_selected (selection, &pls_treemodel, &iter) )
	{
		gtk_tree_model_get (pls_treemodel, &iter,
							1, &path,
							-1);
	}
	
	mpd_playlist_queue_load (gmo, path);
	gimmix_current_playlist_clear ();
	mpd_playlist_queue_commit (gmo);
	
	gimmix_load_playlist (path);
	g_free (path);
	
	return;
}

static void
cb_playlists_delete_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (event->keyval != GDK_Delete)
		return;
	
	cb_gimmix_playlist_remove ();
	
	return;
}

