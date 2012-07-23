/*
 * gimmix-covers.c
 *
 * Copyright (C) 2008-2009 Priyank Gosalia
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

#ifdef HAVE_COVER_PLUGIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <nxml.h>
#include <libxml/xmlreader.h>
#include "wejpconfig.h"
#include "gimmix-metadata.h"
#include "gimmix-covers.h"
#include "gimmix-tagedit.h"

#define DEFAULT_COVER	"gimmix-album.png"
#define COVERS_DIR	".gimmix/covers"
#define COVERS_DBF	".gimmix/covers/covers.db"

#define FREECOVERS_URL	"http://www.freecovers.net/api/search/"

static guint h3_size = 0;

extern GladeXML		*xml;
extern ConfigFile	conf;
extern MpdObj		*gmo;
extern mpd_Song		*glob_song_info;
extern GimmixTooltip 	*tooltip;
extern GtkWidget	*main_window;

static ConfigFile	cover_db;
static CURL		*curl;
static char		*cover_image_path;
static GtkWidget	*gimmix_metadata_image;
static GtkWidget	*gimmix_plcbox_image;
static GtkWidget	*gimmix_plcbox_eventbox;

GtkWidget		*gimmix_plcbox_frame;
static GMutex		*mutex = NULL;

/* Get metadata for the specified arguments */
static CoverNode* gimmix_covers_plugin_get_metadata (char *artist, char *album);

/* Get the fallback cover image of specified size */
static GdkPixbuf* gimmix_covers_plugin_get_default_cover (guint width, guint height);

static gboolean gimmix_covers_plugin_download (const char *url, const char *file);
static CoverNode* gimmix_cover_node_new (void);
static gchar *gimmix_url_encode (const char *string);
static void gimmix_covers_plugin_cover_db_init (void);
static void gimmix_covers_plugin_cover_db_save (void);
static void gimmix_covers_plugin_find_cover (mpd_Song *s);
static void gimmix_cover_plugin_save_cover (char *artist, char *album);

static void
cb_gimmix_covers_plugin_plcbox_size_allocated (GtkWidget *widget, GtkAllocation *a, gpointer data)
{
	if (!h3_size)
	{
		h3_size = a->height;
	}
	
	return;
}

static void
cb_gimmix_covers_plugin_cover_file_preview (GtkFileChooser *file_chooser, gpointer data)
{
	GtkWidget	*preview = NULL;
	char		*filename = NULL;
	GdkPixbuf	*pixbuf = NULL;
	gboolean	have_preview;
	
	preview = GTK_WIDGET (data);
	filename = gtk_file_chooser_get_preview_filename (file_chooser);
	if (filename == NULL)
		return;
	pixbuf = gdk_pixbuf_new_from_file_at_size (filename, 128, 128, NULL);
	have_preview = (pixbuf != NULL);
	g_free (filename);
	if (pixbuf != NULL)
	{
		gtk_image_set_from_pixbuf (GTK_IMAGE (preview), pixbuf);
		g_object_unref (pixbuf);
	}
	gtk_file_chooser_set_preview_widget_active (file_chooser, have_preview);

	return;
}

static void
cb_gimmix_covers_plugin_refetch_cover (void)
{
	g_thread_create ((GThreadFunc)gimmix_covers_plugin_update_cover,
				FALSE,
				FALSE,
				NULL);

	return;
}

static void
cb_gimmix_covers_plugin_set_cover_from_file (void)
{
	GtkWidget	*dialog;
	GtkFileFilter	*filter = NULL;
	GtkImage	*preview;
	mpd_Song	*song = NULL;
	gchar		*artist = NULL;
	gchar		*album = NULL;
	
	if (!(song=mpd_playlist_get_current_song(gmo)))
		return;
	
	artist = (song->artist != NULL) ? g_strdup (song->artist) : NULL;
	album = (song->album != NULL) ? g_strdup (song->album) : NULL;
	dialog = gtk_file_chooser_dialog_new ("Open File",
					GTK_WINDOW(main_window),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL);
	preview = (GtkImage*)gtk_image_new ();
	gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER(dialog), GTK_WIDGET(preview));
	g_signal_connect (GTK_FILE_CHOOSER(dialog), "update-preview", G_CALLBACK (cb_gimmix_covers_plugin_cover_file_preview), preview);
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, "Images (.jpg)");
	gtk_file_filter_add_pattern (filter, "*.jpg");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(dialog), filter);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char	*filename = NULL;
		gchar	*contents = NULL;
		gsize	len = 0;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		if (g_file_get_contents(filename, &contents, &len, NULL))
		{
			gchar	*temp = NULL;
			char	*p = cfg_get_path_to_config_file (COVERS_DIR);
			temp = g_strdup_printf ("%s/temp.jpg", p);
			g_free (p);
			if (g_file_set_contents (temp, contents, len, NULL))
			{
				if (artist!=NULL && album!=NULL)
				{
					gimmix_cover_plugin_save_cover (artist, album);
					g_thread_create ((GThreadFunc)gimmix_covers_plugin_update_cover,
							FALSE,
							FALSE,
							NULL);
					g_free (artist);
					g_free (album);
				}
			}
			else
			{
				gimmix_error ("There was an error while setting the album cover. Please try using a different image.");
			}
			g_free (temp);
		}
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
	
	return;
}

static void
cb_gimmix_covers_plugin_plc_popup (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkWidget 	*menu = NULL;
	GtkWidget 	*menu_item = NULL;
	GtkWidget	*image = NULL;

	if (event->button == 3) /* If right click */
	{
		menu = gtk_menu_new ();

		image = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_MENU);
		menu_item = gtk_image_menu_item_new_with_label (_("Set cover from file"));
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menu_item), image);
		g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (cb_gimmix_covers_plugin_set_cover_from_file), NULL);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
		gtk_widget_show (menu_item);
		
		image = gtk_image_new_from_stock ("gtk-refresh", GTK_ICON_SIZE_MENU);
		menu_item = gtk_image_menu_item_new_with_label (_("Re-fetch cover"));
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menu_item), image);
		g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK(cb_gimmix_covers_plugin_refetch_cover), NULL);
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
	}
}

static void
gimmix_covers_plugin_proxy_init (nxml_t *n)
{
	char	*proxy = NULL;
	
	if (!strncasecmp(cfg_get_key_value(conf,"proxy_enable"),"true",4))
	{
		proxy = gimmix_config_get_proxy_string ();
		nxml_set_proxy (n, proxy, NULL);
		g_free (proxy);
	}
	
	return;
}

void
gimmix_covers_plugin_init (void)
{
	gchar		*cpath = NULL;
	GtkWidget	*widget = NULL;

	/* check if .gimmix/covers exists */
	cpath = cfg_get_path_to_config_file (COVERS_DIR);
	g_mkdir_with_parents (cpath, 00755);
	g_free (cpath);
	
	/* check if .gimmix/covers/covers.db exists */
	cpath = cfg_get_path_to_config_file (COVERS_DBF);
	if (!g_file_test(cpath,G_FILE_TEST_EXISTS))
	{
		FILE *fp = fopen (cpath, "w");
		fclose (fp);
	}
	g_free (cpath);
	
	/* initialize curl */
	curl = curl_easy_init ();
	
	/* initialize mutex */
	mutex = g_mutex_new ();
	
	/* initialize cover database */
	gimmix_covers_plugin_cover_db_init ();
	
	/* initialize metadata widgets */
	gimmix_plcbox_eventbox = glade_xml_get_widget (xml, "cover_event_box");
	gimmix_plcbox_image = glade_xml_get_widget (xml, "gimmix_plcbox_image");
	gimmix_metadata_image = glade_xml_get_widget (xml, "gimmix_metadata_image");
	gimmix_plcbox_frame = glade_xml_get_widget (xml, "gimmix_plc_image_frame");
	
	/* some signals */
	g_signal_connect (gimmix_plcbox_eventbox, "button_press_event", G_CALLBACK(cb_gimmix_covers_plugin_plc_popup), NULL);
	
	/* an ugly way to calculate size of the album picture placeholder */
	widget = glade_xml_get_widget (xml,"plcvbox");
	g_signal_connect (widget, "size-allocate", G_CALLBACK(cb_gimmix_covers_plugin_plcbox_size_allocated), NULL);
	
	/* configuration init */
	if (!strncasecmp(cfg_get_key_value(conf,"coverart_enable"),"false",4))
		gtk_widget_hide (gimmix_plcbox_frame);
	
	return;
}

void
gimmix_covers_plugin_cleanup (void)
{
	curl_easy_cleanup (curl);
	if (mutex)
	{
		g_mutex_free (mutex);
	}
	
	return;
}

static void
gimmix_covers_plugin_cover_db_init (void)
{
	char *rcfile = NULL;
	
	cfg_init_config_file_struct (&cover_db);
	rcfile = cfg_get_path_to_config_file (COVERS_DBF);
	if (cfg_read_config_file(&cover_db,rcfile))
	{
		g_error ("cover db init failed\n");
	}
	g_free (rcfile);
	
	return;
}

static void
gimmix_covers_plugin_cover_db_save (void)
{
	char *rcfile = NULL;

	rcfile = cfg_get_path_to_config_file (COVERS_DBF);
	cfg_write_config_file (&cover_db, rcfile);
	cfg_read_config_file (&cover_db, rcfile);
	cfg_free_config_file_struct (&cover_db);
	gimmix_covers_plugin_cover_db_init ();

	g_free (rcfile);
	
	return;
}

static CoverNode*
gimmix_cover_node_new (void)
{
	CoverNode *ret = NULL;
	
	ret = (CoverNode*) malloc (sizeof(CoverNode));
	if (ret)
	{
		memset (ret, 0, sizeof(CoverNode));
	}

	return ret;
}

/* URL encodes a string */
static gchar *
gimmix_url_encode (const char *string)
{
	CURL	*curl = NULL;
	gchar	*ret = NULL;
	
	if (string)
	{
		ret = curl_easy_escape (curl, string, 0);
	}

	return ret;
}

static size_t
__curl_write_func (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return fwrite (ptr, size, nmemb, stream);
}

static size_t
__curl_read_func (void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return fread (ptr, size, nmemb, stream);
}

static gboolean
gimmix_covers_plugin_download (const char *url, const char *file)
{
	CURLcode	res;
	gboolean	ret = FALSE;
	
	if (url!=NULL && file!=NULL)
	{
		if (curl)
		{
			FILE *outfile = NULL;
			char *path = NULL;
			char *proxy = NULL;

			//path = g_strdup_printf ("%s/%s", cfg_get_path_to_config_file(COVERS_DIR), file);
			//g_print (path);
			outfile = fopen (file, "w");
			/* use a proxy if enabled */
			if (gimmix_config_get_bool("proxy_enable"))
			{
				proxy = gimmix_config_get_proxy_string ();
				curl_easy_setopt (curl, CURLOPT_PROXY, proxy);
				g_free (proxy);
			}
			curl_easy_setopt (curl, CURLOPT_URL, url);
			curl_easy_setopt (curl, CURLOPT_WRITEDATA, outfile);
			curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, __curl_write_func);
			curl_easy_setopt (curl, CURLOPT_READFUNCTION, __curl_read_func);
			res = curl_easy_perform (curl);
			if (!res)
			{
				ret = TRUE;
			}
			fclose (outfile);
			g_free (path);
		}
	}
	
	return ret;
}

static CoverNode*
gimmix_covers_plugin_get_metadata (char *artist, char *album)
{
	char		*url = NULL;
	CoverNode	*node = NULL;
	char		*u_artist = NULL;
	char		*u_album = NULL;
	nxml_t		*nxml = NULL;
	nxml_data_t	*nroot = NULL;
	nxml_data_t	*ndata = NULL;
	nxml_data_t	*nndata = NULL;
	nxml_attr_t *nstatus = NULL;
	
	(artist) && (u_artist = g_utf8_strdown (artist,-1));
	(album) && (u_album = g_utf8_strdown (album,-1));

	/* Initialize libnxml */
	nxml_new (&nxml);
	
	/* Construct search url based on artist and/or album */
	if (artist==NULL && album!=NULL)
		url = g_strdup_printf ("%s%s", FREECOVERS_URL, gimmix_url_encode(u_album));
	else
	if (album==NULL && artist!=NULL)
		url = g_strdup_printf ("%s%s", FREECOVERS_URL, gimmix_url_encode(u_artist));
	else
	{
		url = g_strdup_printf ("%s%s-%s",FREECOVERS_URL,
								gimmix_url_encode(u_artist),
								gimmix_url_encode(u_album));	
	}
	g_print ("%s\n",url);

	nxml_set_timeout (nxml, 20);
	gimmix_covers_plugin_proxy_init (nxml);
	/* Parse the results returned */
	printf ("Returned: %d\n", nxml_parse_url (nxml, url));
	g_free (url);

	/* Get the root rsp element to check the status of the result */
	nxml_find_element (nxml, NULL, "rsp", &ndata);
	if (ndata)
	{
		/* check status of the result, if ok, continue */
		nxml_find_attribute (ndata, "stat", &nstatus);
		if (nstatus)
		{
			if (strcmp(nstatus->value,"ok"))
			{
				goto cleanup;
			}
		}
	}
	else
	{
		goto cleanup;
	}
	
	/* Good, we've got something... let's see if we get our desired cover */
	nxml_find_element (nxml, ndata, "title", &nndata);
	while (nndata)
	{
		nxml_data_t *n_name = NULL;
		nxml_data_t *n_category = NULL;
		char *name = NULL;
		char *category = NULL;
		
		nxml_find_element (nxml, nndata, "name", &n_name);
		nxml_find_element (nxml, nndata, "category", &n_category);
		nxml_get_string (n_name, &name);
		nxml_get_string (n_category, &category);
		
		gboolean found = FALSE;
		if (name)
		{
			name = g_utf8_strdown (name,-1);
			printf ("name: %s\n", name);			
			if (artist && !album)
			{
				if (g_strrstr(name,u_artist)) found = TRUE;
			}
			else
			if (album && !artist)
			{
				if (g_strrstr(name,u_album)) found = TRUE;
			}
			else
			{
				if (g_strrstr(name,u_artist) || g_strrstr(name,u_album)) found = TRUE;
			}
			free (name);
		}
		
		if (found)
		{
			nxml_data_t *n_covers = NULL;
			nxml_find_element (nxml, nndata, "covers", &n_covers);
			gboolean foundcover = FALSE;
			
			if (n_covers)
			{
				nxml_data_t *d;
				nxml_find_element (nxml, n_covers, "cover", &d);
				while (d)
				{
					char *cover_type;
					char *cover_url;
					char *cover_width;
					char *cover_height;
					
					nxml_data_t *n_type;
					nxml_data_t *n_url;
					nxml_data_t *n_width;
					nxml_data_t *n_height;
					
					nxml_find_element (nxml, d, "type", &n_type);
					nxml_find_element (nxml, d, "preview", &n_url);
					nxml_find_element (nxml, d, "width", &n_width);
					nxml_find_element (nxml, d, "height", &n_height);
					
					nxml_get_string (n_url, &cover_url);
					nxml_get_string (n_type, &cover_type);
					nxml_get_string (n_width, &cover_width);
					nxml_get_string (n_height, &cover_height);
					
					guint width = atoi (cover_width);
					guint height = atoi (cover_height);

					g_free (cover_width);
					g_free (cover_height);
					
					guint diff = width-height;

					if (cover_url && !strcmp(cover_type,"front") && diff<=100)
					{
						printf ("==== FOUND COVER =====\n");
						printf ("downloading cover from: %s\n", cover_url);
						node = gimmix_cover_node_new ();
						node->img_large = g_strdup (cover_url);
						g_free (cover_type);
						g_free (cover_url);
						foundcover = TRUE;
						break;
					}
					
					d = d->next;
				}
				if (foundcover)
				break;
			}
		}
		else
		{
			g_print ("cover not found\n");
		}
		nndata = nndata->next;
	}
	cleanup:
	nxml_free (nxml);

	return node;
}

static void
gimmix_cover_plugin_save_cover (char *artist, char *album)
{
	char	*artist_e = NULL;
	char	*album_e = NULL;
	char	*old_path = NULL;
	char	*new_path = NULL;
	char	*key = NULL;
	char	*temp = NULL;
	
	if (artist == NULL || album == NULL)
		return;
	artist_e = gimmix_url_encode (artist);
	album_e = gimmix_url_encode (album);
	
	/* save cover art */
	temp = cfg_get_path_to_config_file (COVERS_DIR);
	old_path = g_strdup_printf ("%s/temp.jpg", temp);
	new_path = g_strdup_printf ("%s/%s-%s.jpg", temp, artist_e, album_e);
	g_rename (old_path, new_path);
	g_free (temp);
	
	/* okay, add an entry to covers.db */
	key = g_strdup_printf ("%s-%s", artist, album);
	gimmix_strcrep (key, ' ', '_');
	//g_print ("%s\n\n%s\n",key, new_path);
	cfg_add_key (&cover_db, key, new_path);
	gimmix_covers_plugin_cover_db_save ();
	
	g_free (old_path);
	g_free (new_path);
	g_free (artist_e);
	g_free (album_e);
	//g_free (key);

	return;
}

static void
gimmix_covers_plugin_save_albuminfo (char *artist, char *album, char *info)
{
	FILE	*fp = NULL;
	char	*path = NULL;
	char	*artist_e = NULL;
	char	*album_e = NULL;
	char	*temp = NULL;
	
	if (info == NULL || !strlen(info))
		return;
	
	/* save album info */
	artist_e = gimmix_url_encode (artist);
	album_e = gimmix_url_encode (album);
	temp = cfg_get_path_to_config_file (COVERS_DIR);
	path = g_strdup_printf ("%s/%s-%s.albuminfo", temp, artist_e, album_e);
	if ((fp=fopen(path, "w")))
	{
		fprintf (fp, info);
		fclose (fp);
	}
	g_free (temp);
	g_free (path);
	g_free (artist_e);
	g_free (album_e);
	
	return;
}

gchar*
gimmix_covers_plugin_get_albuminfo (mpd_Song *s)
{
	FILE	*fp = NULL;
	char	*artist_e = NULL;
	char	*album_e = NULL;
	char	*path = NULL;
	char	line[256] = "";
	char	*ret = NULL;
	char	*temp = NULL;
	char	*p = NULL;
	
	if (s == NULL)
	{
		return NULL;
	}
	if (s->artist == NULL || s->album == NULL)
	{
		return NULL;
	}
	artist_e = gimmix_url_encode (s->artist);
	album_e = gimmix_url_encode (s->album);
	temp = cfg_get_path_to_config_file (COVERS_DIR);
	path = g_strdup_printf ("%s/%s-%s.albuminfo", temp, artist_e, album_e);
	g_free (artist_e);
	g_free (album_e);
	
	if ((fp=fopen(path,"r")))
	{
		GString *str = g_string_new ("");
		while (fgets(line,255,fp))
		{
			str = g_string_append (str, line);
		}
		fclose (fp);
		ret = g_strdup (str->str);
		g_free (path);
		g_free (temp);
		g_string_free (str, TRUE);
		return ret;
	}
	
	artist_e = gimmix_url_encode (s->performer);
	album_e = gimmix_url_encode (s->album);
	g_free (path);
	p = cfg_get_path_to_config_file (COVERS_DIR);
	path = g_strdup_printf ("%s/%s-%s.albuminfo", p, artist_e, album_e);
	g_free (p);
	g_free (temp);
	g_free (artist_e);
	g_free (album_e);
	
	if ((fp=fopen(path,"r")))
	{
		GString *str = g_string_new ("");
		while (fgets(line,255,fp))
		{
			str = g_string_append (str, line);
		}
		fclose (fp);
		ret = g_strdup (str->str);
		g_free (path);
		g_string_free (str, TRUE);
		return ret;
	}
	g_free (path);
	
	return ret;
}

static void
gimmix_covers_plugin_set_cover_image_path (const char *path)
{
	if (cover_image_path)
	{
		g_free (cover_image_path);
		cover_image_path = NULL;
	}
	if (path)
	{
		cover_image_path = g_strdup (path);
	}

	return;
}

static GdkPixbuf*
gimmix_covers_plugin_get_default_cover (guint width, guint height)
{
	GdkPixbuf	*ret = NULL;
	gchar		*path = NULL;
	
	path = gimmix_get_full_image_path (DEFAULT_COVER);
	ret = gdk_pixbuf_new_from_file_at_size (path, width, height, NULL);
	g_free (path);
	
	return ret;
}

GdkPixbuf*
gimmix_covers_plugin_get_cover_image_of_size (guint width, guint height)
{
	GdkPixbuf	*pixbuf = NULL;
	int		status;
	//g_print ("gimmix_covers_plugin_get_cover_image_of_size() called\n");
	status = gimmix_get_status (gmo);
	if (status == STOP || status == ERROR)
	{
		/* set default image */
		pixbuf = gimmix_covers_plugin_get_default_cover (width, height);
	}
	else
	{
		if (!cover_image_path)
		{
			/* set default image */
			//g_print ("cover_image_path is NULL\n");
			pixbuf = gimmix_covers_plugin_get_default_cover (width, height);
		}
		else
		{
			pixbuf = gdk_pixbuf_new_from_file_at_size (cover_image_path, width, height, NULL);
		}
	}

	return pixbuf;
}

static void
gimmix_covers_plugin_find_cover (mpd_Song *s)
{
	CoverNode	*node = NULL;
	char		*temp = NULL;
	char		salbum[256] = "";
	char		sartist[256] = "";
	char		sperformer[256] = "";
	
	if (s != NULL)
	{
		char *result = NULL;
		guint len = 0;
		
		/* first look into the local cover database */
		if (!s->artist || !s->album)
		{
			gimmix_covers_plugin_set_cover_image_path (result);
			return;
		}
		if (s->artist)
			len = strlen (s->artist);
		strncpy (sartist, s->artist, len);
		g_strstrip (sartist);
		strncpy (salbum, s->album, strlen(s->album));
		g_strstrip (salbum);
		if (s->performer)
		{
			strncpy (sperformer, s->performer, strlen(s->performer));
			g_strstrip (sperformer);
		}
		temp = g_strdup_printf ("%s-%s", sartist, salbum);
		gimmix_strcrep (temp, ' ', '_');
		result = cfg_get_key_value (cover_db, temp);
		g_free (temp);
		//g_print ("result: %s\n", result);
		if (result!=NULL)
		{
			gimmix_covers_plugin_set_cover_image_path (result);
			g_print ("cover found on disk\n");
			return;
		}
		/* if not found locally, fetch it from amazon */
		else
		{
			//g_print ("beginning to fetch \n");
			char *ptr = cfg_get_path_to_config_file (COVERS_DIR);
			temp = g_strdup_printf ("%s/temp.jpg", ptr);
			g_free (ptr);
			node = gimmix_covers_plugin_get_metadata (sartist, salbum);
			if (node!=NULL)
			{
				if (gimmix_covers_plugin_download(node->img_large,temp) ||
					gimmix_covers_plugin_download(node->img_medium,temp) ||
					gimmix_covers_plugin_download(node->img_small,temp))
				{
					gimmix_cover_plugin_save_cover (sartist, salbum);
					gimmix_covers_plugin_save_albuminfo (sartist, salbum, node->album_info);
					gimmix_covers_plugin_find_cover (s);
				}
				g_free (node);
				g_free (temp);
				return;
			}
			node = gimmix_covers_plugin_get_metadata (NULL, salbum);
			if (node!=NULL)
			{
				if (gimmix_covers_plugin_download(node->img_large,temp) ||
					gimmix_covers_plugin_download(node->img_medium,temp) ||
					gimmix_covers_plugin_download(node->img_small,temp))
				{
					gimmix_cover_plugin_save_cover (sartist, salbum);
					gimmix_covers_plugin_save_albuminfo (sartist, salbum, node->album_info);
					gimmix_covers_plugin_find_cover (s);
				}
				g_free (node);
				g_free (temp);
				return;
			}
			else
			{
				node = gimmix_covers_plugin_get_metadata (sperformer, salbum);
				if (node!=NULL)
				{
					if (gimmix_covers_plugin_download(node->img_large,temp) ||
					gimmix_covers_plugin_download(node->img_medium,temp) ||
					gimmix_covers_plugin_download(node->img_small,temp))
					{
						gimmix_cover_plugin_save_cover (sartist, salbum);
						gimmix_covers_plugin_save_albuminfo (sartist, salbum, node->album_info);
						gimmix_covers_plugin_find_cover (s);
					}
					g_free (node);
					g_free (temp);
					return;
				}
				else
				{
					/* set default icon */
					gimmix_covers_plugin_set_cover_image_path (NULL);
				}
			}
		}
	}
	gimmix_covers_plugin_set_cover_image_path (NULL);

	return;
}

/* if default = TRUE, set the default cover */
void
gimmix_covers_plugin_update_cover (gboolean defaultc)
{
	guint		height;
	GdkPixbuf	*pixbuf = NULL;

	g_mutex_lock (mutex);
	height = h3_size;
	if (!defaultc)
	{
		mpd_Song *s = NULL;
		mpd_Song *sng = mpd_newSong ();
		while (!(s=mpd_playlist_get_current_song(gmo)))
			sleep (1);
		sng = mpd_songDup (s);
		gimmix_covers_plugin_find_cover (sng);
		pixbuf = gimmix_covers_plugin_get_cover_image_of_size (96, height);
		mpd_freeSong (sng);
	}
	else
	{
		pixbuf = gimmix_covers_plugin_get_default_cover (96, height);
	}
	
	if (pixbuf != NULL)
	{
		/* main window cover art */
		gdk_threads_enter ();
		gtk_image_set_from_pixbuf (GTK_IMAGE(gimmix_plcbox_image), pixbuf);
		gdk_flush ();
		gdk_threads_leave ();
		g_object_unref (pixbuf);
		
		/* metadata albuminfo */
		// gimmix_metadata_set_song_details (s, areview);

		/* tag editor cover image */
		pixbuf = gimmix_covers_plugin_get_cover_image_of_size (96, 96);
		gdk_threads_enter ();
		gimmix_tag_editor_set_cover_image (pixbuf);
		gdk_flush ();
		gdk_threads_leave ();
		g_object_unref (pixbuf);

		/* also system tray tooltip image */
		if (gimmix_config_get_bool("enable_systray"))
		{
			if (gimmix_config_get_bool("enable_notification"))
			{
				pixbuf = gimmix_covers_plugin_get_cover_image_of_size (48, 48);
				gimmix_tooltip_set_icon (tooltip, pixbuf);
				g_object_unref (pixbuf);
			}
		}
	}
	g_mutex_unlock (mutex);

	return;
}

#endif
