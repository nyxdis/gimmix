/*
 * gimmix.c
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

#include <gtk/gtk.h>
#include <locale.h>

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "gimmix.h"
#include "gimmix-interface.h"
#include "gimmix-playlist.h"

#define GLADE_FILE	"/share/gimmix/gimmix.glade"
#define GIMMIX_ICON	"gimmix.png"

MpdObj 		*gmo = NULL;
GladeXML 	*xml = NULL;

static void error_dialog_response (GtkDialog *err_dialog, gint arg1, gpointer dialog);

bool
gimmix_connect (void)
{
	MpdObj *mo;

	mo = gimmix_mpd_connect ();
	
	if (mo != NULL)
	{
		gmo = mo;
		return true;
	}
	
	gmo = NULL;
	return false;
}

void
gimmix_connect_error (void)
{
	GtkWidget	*error_dialog;
	gchar		*error;
	
	error = _("Gimmix couldn't connect to mpd. \n\nCheck whether mpd is running.\nAlso check that you have specified the proper hostname, port and password in ~/.gimmixrc");
	
	error_dialog = gtk_message_dialog_new_with_markup (NULL,
							GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_ERROR,
							GTK_BUTTONS_CLOSE,
							"<b>%s: </b><span size=\"large\">%s</span>",
							_("ERROR"),
							error);
	g_signal_connect (error_dialog,
			"response",
			G_CALLBACK (error_dialog_response),
			(gpointer)error_dialog);
	
	gtk_widget_show_all (error_dialog);
    
	return;
}

static void
error_dialog_response (GtkDialog *err_dialog, gint arg1, gpointer dialog)
{
	gtk_widget_destroy (GTK_WIDGET(dialog));
	gtk_main_quit ();
	
	return;
}

void
gimmix_about_show (void)
{
	GdkPixbuf		*about_pixbuf;
	gchar			*path;
	static gchar		*license = 
	("Gimmix is free software; you can redistribute it and/or "
	"modify it under the terms of the GNU General Public Licence as "
	"published by the Free Software Foundation; either version 2 of the "
	"Licence, or (at your option) any later version.\n"
	"\n"
	"Gimmix is distributed in the hope that it will be useful, "
	"but WITHOUT ANY WARRANTY; without even the implied warranty of "
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU "
	"General Public Licence for more details.\n"
	"\n"
	"You should have received a copy of the GNU General Public Licence "
	"along with Gimmix; if not, write to the Free Software "
	"Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, "
	"MA  02110-1301  USA");
	
	path = gimmix_get_full_image_path (GIMMIX_ICON);
	about_pixbuf = gdk_pixbuf_new_from_file (path, NULL);
	g_free (path);

	gchar *authors[] = { "Priyank M. Gosalia <priyankmg@gmail.com>",
					"Gimmix uses wejpconfig written by Johannes Heimansberg.",
					_("A part of the song seek code borrowed from Pygmy."),
					"Alex Smith: Autoconf 2.5 support",
				 	"Rohan Dhruva: Extensive testing and bug reports",
					 NULL
					};

	gtk_show_about_dialog (NULL,
                           "name", APPNAME,
                           "version", VERSION,
                           "copyright", _("\xC2\xA9 2006 Priyank Gosalia  (GPL)"),
                           "comments", _("Gimmix is a graphical music player daemon (MPD) client written in C."),
                           "license", license,
                           "authors", authors,
					  "translator-credits", _("Traditional Chinese (zh_TW) - Cheng-Wei Chien <e.cwchien@gmail.com>"),
                           "website", APPURL,
                           "website-label", APPURL,
                           "logo", about_pixbuf,
                           "wrap-license", true,
                           NULL);
	g_object_unref (about_pixbuf);

	return;
}

int
main (int argc, char *argv[])
{
	gchar 		*path;
	char		*lang;
	
	lang = getenv ("LC_ALL");
	if (lang==NULL || lang[0]=='\0')
		lang = getenv ("LANG");
	
	setlocale (LC_ALL, lang);
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	textdomain (GETTEXT_PACKAGE);

	gtk_init (&argc, &argv);
	
	path = g_strdup_printf ("%s%s", PREFIX, GLADE_FILE);
	xml = glade_xml_new (path, NULL, NULL);
	g_free (path);
	
	if (xml == NULL)
	{	
		g_error (_("Failed to initialize interface."));
		exit_cleanup ();
	}
	
	glade_xml_signal_autoconnect (xml);
	
	if (gimmix_config_exists())
	{
		gimmix_config_init (); /* initialize configuration */
		if (gimmix_connect())
		{
			gimmix_init ();
		}
		else
		{
			gimmix_connect_error ();
		}
	}
	else
	{
		gimmix_show_firstrun_dialog (); /* display the first run dialog */
	}
	
	gtk_main ();
	exit_cleanup ();
	
	return 0;
}

void
exit_cleanup ()
{
	gimmix_interface_cleanup ();
	if (gmo != NULL)
		gimmix_disconnect (gmo);
	gimmix_config_free ();
}

