/*
 * gimmix.c
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

#include <gtk/gtk.h>
#include "gimmix.h"
#include "gimmix-firstrun.h"
#include "gimmix-interface.h"
#include "gimmix-playlist.h"

#define GLADE_FILE "/share/gimmix/gimmix.glade"

static void error_dialog_response (GtkDialog *err_dialog, gint arg1, gpointer dialog);

bool
gimmix_connect (void)
{
	MpdObj *mo;

	mo = gimmix_mpd_connect (pub->conf);
	
	if (mo != NULL)
	{	
		pub->gmo = mo;
		return true;
	}
	else
	{
		pub->gmo = NULL;
		return false;
	}
}

void
gimmix_connect_error (void)
{
	GtkWidget 	*error_dialog;
	static gchar *error = "Gimmix couldn't connect to mpd. \n\nCheck whether mpd is running.\nAlso check that you have specified the proper hostname, port and password in ~/.gimmixrc";
	
	error_dialog = gtk_message_dialog_new_with_markup (NULL,
												GTK_DIALOG_DESTROY_WITH_PARENT,
												GTK_MESSAGE_ERROR,
												GTK_BUTTONS_OK,
												"<b>%s: </b><span size=\"large\">%s</span>",
												"ERROR",
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

int
main (int argc, char *argv[])
{
	gchar 		*path;
	Conf		*conf;
	GtkWidget	*main_window;

	pub = (GM *) malloc(sizeof(GM));
	pub->conf = NULL;
	pub->gmo = NULL;

	gtk_init (&argc, &argv);
	
	path = g_strdup_printf ("%s%s", PREFIX, GLADE_FILE);
	xml = glade_xml_new (path, NULL, NULL);
	g_free (path);
	
	if (!xml)
		exit_cleanup ();
	
	glade_xml_signal_autoconnect (xml);
	
	if (gimmix_config_exists())
	{
		pub->conf = gimmix_config_init ();
		if (!pub->conf)
		{
			gimmix_connect_error ();
		}
		main_window = glade_xml_get_widget (xml, "main_window");
		if (gimmix_connect())
		{
			gtk_widget_show (main_window);
			gimmix_init ();
		}
		else
		{
			gimmix_connect_error ();
		}
	}
	else
	{
		gimmix_show_firstrun_dialog ();
	}
	
	gtk_main ();
	exit_cleanup ();
}

void
exit_cleanup ()
{
	gimmix_interface_cleanup ();
	
	if (pub->gmo != NULL)
		gimmix_disconnect (pub->gmo);
	if (pub->conf != NULL)
		gimmix_config_free (pub->conf);
	g_free (pub);

	exit (0);
}
