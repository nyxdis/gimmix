/*
 * gimmix-covers.c
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

#define DEFAULT_COVER	"gimmix-album.png"
#define COVERS_DIR	".gimmix/covers"
#define COVERS_DBF	".gimmix/covers/covers.db"
#define AMAZON_KEY	"14TBPBEBTPCVM7BY0C02"
#define AMAZON_URL1	"http://ecs.amazonaws.%s/onca/xml?Service=AWSECommerceService&Operation=ItemSearch&SearchIndex=Music&ResponseGroup=Images,EditorialReview&AWSAccessKeyId=%s&%s=%s"
#define AMAZON_URL2	"http://ecs.amazonaws.%s/onca/xml?Service=AWSECommerceService&Operation=ItemSearch&SearchIndex=Music&ResponseGroup=Images,EditorialReview&AWSAccessKeyId=%s&%s=%s&%s=%s"

char *cover_locations[6][2] = 
{ 
	{"com", "United States"},
	{"co.uk", "United Kingdom"},
	{"jp", "Japan"},
	{"fr", "France"},
	{"ca", "Canada"},
	{"de", "Germany"}
};

static guint h3_size = 0;

extern GladeXML		*xml;
extern ConfigFile	conf;
extern MpdObj		*gmo;
extern mpd_Song		*glob_song_info;
extern GimmixTooltip 	*tooltip;

static ConfigFile	cover_db;
static CURL		*curl;
static char		*cover_image_path;
static GtkWidget	*gimmix_metadata_image;
static GtkWidget	*gimmix_plcbox_image;

GtkWidget		*gimmix_plcbox_frame;

/* Get metadata for the specified arguments */
static CoverNode* gimmix_covers_plugin_get_metadata (char *arg1, char *arg1d, char *arg2, char *arg2d);

/* Get the fallback cover image of specified size */
static GdkPixbuf* gimmix_covers_plugin_get_default_cover (guint width, guint height);

/* Set the cover image for metadata section */
static void gimmix_covers_plugin_set_metadata_image (GdkPixbuf *pixbuf);

static gboolean gimmix_covers_plugin_download (const char *url, const char *file);
static CoverNode* gimmix_cover_node_new (void);
static gchar *gimmix_url_encode (const char *string);
static void gimmix_covers_plugin_cover_db_init (void);
static void gimmix_covers_plugin_cover_db_save (void);
static void gimmix_covers_plugin_find_cover (mpd_Song *s);

static void
cb_gimmix_covers_plugin_plcbox_size_allocated (GtkWidget *widget, GtkAllocation *a, gpointer data)
{
	if (!h3_size)
	{
		h3_size = a->height;
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
	
	/* initialize curl */
	curl = curl_easy_init ();
	
	/* initialize cover database */
	gimmix_covers_plugin_cover_db_init ();
	
	/* initialize metadata widgets */
	gimmix_plcbox_image = glade_xml_get_widget (xml, "gimmix_plcbox_image");
	gimmix_metadata_image = glade_xml_get_widget (xml, "gimmix_metadata_image");
	gimmix_plcbox_frame = glade_xml_get_widget (xml, "gimmix_plc_image_frame");
	
	/* some signals */
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

			//path = g_strdup_printf ("%s/%s", cfg_get_path_to_config_file(COVERS_DIR), file);
			//g_print (path);
			outfile = fopen (file, "w");
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
gimmix_covers_plugin_get_metadata (char *arg1, char *arg1d, char *arg2, char *arg2d)
{
	char		*url = NULL;
	CoverNode	*node = NULL;
	char		*u_artist = NULL;
	char		*u_title = NULL;
	nxml_t		*nxml = NULL;
	nxml_data_t	*nroot = NULL;
	nxml_data_t	*ndata = NULL;
	nxml_data_t	*nndata = NULL;
	char		*str = NULL;
	char		*location = NULL;
	nxml_error_t	e;
	
	u_artist = gimmix_url_encode (arg1d);
	u_title = gimmix_url_encode (arg2d);
	location = cfg_get_key_value(conf,"coverart_location");
	if (!arg1 && !arg1d)
	{
		url = g_strdup_printf (AMAZON_URL1, location, AMAZON_KEY, arg2, u_title);
	}
	else
	{
		url = g_strdup_printf (AMAZON_URL2, location, AMAZON_KEY, arg1, u_artist, arg2, u_title);
	}
	//g_print ("%s\n", url);

	e = nxml_new (&nxml);
	nxml_parse_url (nxml, url);
	nxml_root_element (nxml, &nroot);
	nxml_find_element (nxml, nroot, "Items", &ndata);
	nxml_find_element (nxml, ndata, "Item", &nndata);
	if (nndata)
	{
		nxml_data_t *child = NULL;
		nxml_data_t *d = NULL;
		nxml_data_t *t = NULL;
		child = nndata;
		node = gimmix_cover_node_new ();
		
		/* large image */
		nxml_find_element (nxml, child, "LargeImage", &d);
		nxml_find_element (nxml, d, "URL", &t);
		nxml_get_string (t, &str);
		if (str!=NULL)
		{
			node->img_large = g_strdup (str);
			free (str);
		}
		
		/* medium image */
		nxml_find_element (nxml, child, "MediumImage", &d);
		nxml_find_element (nxml, d, "URL", &t);
		nxml_get_string (t, &str);
		if (str!=NULL)
		{
			node->img_medium = g_strdup (str);
			free (str);
			str = NULL;
		}
				
		/* small image */
		nxml_find_element (nxml, child, "SmallImage", &d);
		nxml_find_element (nxml, d, "URL", &t);
		nxml_get_string (t, &str);
		if (str!=NULL)
		{
			node->img_small = g_strdup (str);
			free (str);
			str = NULL;
		}
		
		/* editorial reviews */
		nxml_find_element (nxml, child, "EditorialReviews", &d);
		nxml_find_element (nxml, d, "EditorialReview", &t);
		nxml_find_element (nxml, t, "Content", &d);
		nxml_get_string (d, &str);
		if (str!=NULL)
		{
			g_print ("%s\n", str);
			node->album_info = g_strdup (str);
			free (str);
		}
		
	}
	nxml_free (nxml);
	g_free (url);

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
	
	artist_e = gimmix_url_encode (artist);
	album_e = gimmix_url_encode (album);
	
	/* save cover art */
	old_path = g_strdup_printf ("%s/temp.jpg", cfg_get_path_to_config_file(COVERS_DIR));
	new_path = g_strdup_printf ("%s/%s-%s.jpg", cfg_get_path_to_config_file(COVERS_DIR), artist_e, album_e);
	g_rename (old_path, new_path);
	
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
	g_free (key);

	return;
}

static void
gimmix_covers_plugin_save_albuminfo (char *artist, char *album, char *info)
{
	FILE	*fp = NULL;
	char	*path = NULL;
	char	*artist_e = NULL;
	char	*album_e = NULL;
	
	if (info == NULL || !strlen(info))
		return;
	
	/* save album info */
	artist_e = gimmix_url_encode (artist);
	album_e = gimmix_url_encode (album);
	path = g_strdup_printf ("%s/%s-%s.albuminfo", cfg_get_path_to_config_file(COVERS_DIR), artist_e, album_e);
	fp = fopen (path, "w");
	fprintf (fp, info);
	fclose (fp);
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
	path = g_strdup_printf ("%s/%s-%s.albuminfo", cfg_get_path_to_config_file(COVERS_DIR), artist_e, album_e);
	g_free (artist_e);
	g_free (album_e);
	
	if ((fp=fopen(path,"r")))
	{
		GString *str = g_string_new ("");
		while (fgets(line,255,fp))
		{
			str = g_string_append (str, line);
		}
		ret = g_strdup (str->str);
		return ret;
	}
	
	artist_e = gimmix_url_encode (s->performer);
	album_e = gimmix_url_encode (s->album);
	path = g_strdup_printf ("%s/%s-%s.albuminfo", cfg_get_path_to_config_file(COVERS_DIR), artist_e, album_e);
	g_free (artist_e);
	g_free (album_e);
	
	if ((fp=fopen(path,"r")))
	{
		GString *str = g_string_new ("");
		while (fgets(line,255,fp))
		{
			str = g_string_append (str, line);
		}
		ret = g_strdup (str->str);
		return ret;
	}
	
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
		mpd_Song *s = NULL;
		/*do {
			s = mpd_playlist_get_current_song (gmo);
		} while (s==NULL);*/
		sleep (2);
		s = mpd_playlist_get_current_song (gmo);
		//if (s == NULL)
		//g_print ("s = NULL\n");
		gimmix_covers_plugin_find_cover (s);
		
		if (s == NULL || cover_image_path == NULL)
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
	
	if (s != NULL)
	{
		char *result = NULL;
		
		/* first look into the local cover database */
		if (!s->artist || !s->album)
		{
			gimmix_covers_plugin_set_cover_image_path (result);
			return;
		}
		temp = g_strdup_printf ("%s-%s", s->artist, s->album);
		gimmix_strcrep (temp, ' ', '_');
		result = cfg_get_key_value (cover_db, temp);
		if (result!=NULL)
		{
			gimmix_covers_plugin_set_cover_image_path (result);
			//g_print ("found on localdisk\n");
			return;
		}
		/* if not found locally, fetch it from amazon */
		else
		{
			//g_print ("beginning to fetch \n");	
			temp = g_strdup_printf ("%s/temp.jpg", cfg_get_path_to_config_file(COVERS_DIR));
			node = gimmix_covers_plugin_get_metadata ("Artist", s->artist, "Title", s->album);
			if (node!=NULL)
			{
				if (gimmix_covers_plugin_download(node->img_large,temp) ||
					gimmix_covers_plugin_download(node->img_medium,temp) ||
					gimmix_covers_plugin_download(node->img_small,temp))
				{
					gimmix_cover_plugin_save_cover (s->artist, s->album);
					gimmix_covers_plugin_save_albuminfo (s->artist, s->album, node->album_info);
					gimmix_covers_plugin_find_cover (s);
				}
				g_free (node);
				return;
			}
			node = gimmix_covers_plugin_get_metadata (NULL, NULL, "Title", s->album);
			if (node!=NULL)
			{
				if (gimmix_covers_plugin_download(node->img_large,temp) ||
					gimmix_covers_plugin_download(node->img_medium,temp) ||
					gimmix_covers_plugin_download(node->img_small,temp))
				{
					gimmix_cover_plugin_save_cover (s->artist, s->album);
					gimmix_covers_plugin_save_albuminfo (s->artist, s->album, node->album_info);
					gimmix_covers_plugin_find_cover (s);
				}
				g_free (node);
				return;
			}
			else
			{
				node = gimmix_covers_plugin_get_metadata ("Performer", s->performer, "Title", s->album);
				if (node!=NULL)
				{
					if (gimmix_covers_plugin_download(node->img_large,temp) ||
					gimmix_covers_plugin_download(node->img_medium,temp) ||
					gimmix_covers_plugin_download(node->img_small,temp))
					{
						gimmix_cover_plugin_save_cover (s->artist, s->album);
						gimmix_covers_plugin_save_albuminfo (s->artist, s->album, node->album_info);
						gimmix_covers_plugin_find_cover (s);
					}
					g_free (node);
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

static void
gimmix_covers_plugin_set_metadata_image (GdkPixbuf *pixbuf)
{
	gtk_image_set_from_pixbuf (GTK_IMAGE(gimmix_metadata_image), pixbuf);
	
	return;
}

/* if default = TRUE, set the default cover */
void
gimmix_covers_plugin_update_cover (gboolean defaultc)
{
	guint		height;
	GdkPixbuf	*pixbuf = NULL;
	mpd_Song	*s = NULL;

	height = h3_size;
	if (defaultc)
	{
		pixbuf = gimmix_covers_plugin_get_default_cover (96, height);
		gtk_image_set_from_pixbuf (GTK_IMAGE(gimmix_plcbox_image), pixbuf);
		g_object_unref (pixbuf);
		pixbuf = gimmix_covers_plugin_get_default_cover (64, 64);
		gimmix_covers_plugin_set_metadata_image (pixbuf);
		g_object_unref (pixbuf);
		if (!strncasecmp(cfg_get_key_value(conf,"enable_systray"),"true",4))
		{
			if (!strncasecmp(cfg_get_key_value(conf,"enable_notification"),"true",4))
			{
				pixbuf = gimmix_covers_plugin_get_default_cover (48, 48);
				gimmix_tooltip_set_icon (tooltip, pixbuf);
				g_object_unref (pixbuf);
			}
		}
		return;
	}
	else
	{
		pixbuf = gimmix_covers_plugin_get_cover_image_of_size (96, height);
	}
	
	int i = 0;
	sleep (2);
	if (mpd_player_get_state(gmo)!=MPD_PLAYER_STOP)
	{
		if (mpd_playlist_get_playlist_length(gmo))
			s = mpd_playlist_get_current_song (gmo);
		else
			s = NULL;
	}
	/*do {
		s = mpd_playlist_get_current_song (gmo);
		i++;
	} while (s==NULL);*/
	
	
	//printf ("looped: %d\n", i);
	if (pixbuf != NULL)
	{
		char *areview = NULL;
		
		/* main window cover art */
		gtk_image_set_from_pixbuf (GTK_IMAGE(gimmix_plcbox_image), pixbuf);
		g_object_unref (pixbuf);
		
		/* metadata cover art */
		if (s!=NULL)
			pixbuf = gimmix_covers_plugin_get_cover_image_of_size (64, 64);
		else
			pixbuf = gimmix_covers_plugin_get_default_cover (64, 64);
		gimmix_covers_plugin_set_metadata_image (pixbuf);
		g_object_unref (pixbuf);
		
		/* metadata albuminfo */
		areview = gimmix_covers_plugin_get_albuminfo (s);
		gimmix_metadata_set_song_details (s, areview);
		if (areview)
			g_free (areview);
		
		/* also system tray tooltip image */
		if (!strncasecmp(cfg_get_key_value(conf,"enable_systray"),"true",4))
		{
			if (!strncasecmp(cfg_get_key_value(conf,"enable_notification"),"true",4))
			{
				pixbuf = gimmix_covers_plugin_get_cover_image_of_size (48, 48);
				gimmix_tooltip_set_icon (tooltip, pixbuf);
				g_object_unref (pixbuf);
			}
		}
	}
	
	return;
}

#endif
