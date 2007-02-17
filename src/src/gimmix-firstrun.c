/*
 * gimmix-firstrun.c
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
#include "gimmix-firstrun.h"
#include "gimmix.h"

#define CONFIG_FILE ".gimmixrc"

extern GladeXML 	*xml;

/* config file object */
ConfigFile	cf;

static void on_fr_close_clicked (GtkWidget *widget, gpointer data);
static void on_fr_apply_clicked (GtkWidget *widget, gpointer data);

void
gimmix_show_firstrun_dialog (void)
{
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *check;
	GtkWidget *entry;

	cfg_init_config_file_struct (&cf);
	
	cfg_add_key (&cf, "mpd_hostname",		"localhost");
	cfg_add_key (&cf, "mpd_port", 			"6600");
	cfg_add_key (&cf, "mpd_password",		"");
	cfg_add_key (&cf, "music_directory",	"");
	cfg_add_key (&cf, "enable_systray",		"true");
	cfg_add_key (&cf, "play_on_add",		"false");
	cfg_add_key (&cf, "stop_on_exit",		"false");
	cfg_add_key (&cf, "window_width",		"334");
	cfg_add_key (&cf, "window_height",		"120");
	cfg_add_key (&cf, "full_view_mode",		"false");
	cfg_add_key (&cf, "enable_search",		"true");
	cfg_add_key (&cf, "update_on_startup",	"false");
			
	window = glade_xml_get_widget (xml, "first_run_dialog");
	button = glade_xml_get_widget (xml, "fr_apply");
	g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK(on_fr_apply_clicked), NULL);
	g_signal_connect (G_OBJECT(window), "delete_event", G_CALLBACK(gtk_main_quit), NULL);
	button = glade_xml_get_widget (xml, "fr_close");
	g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK(on_fr_close_clicked), window);
	
	gtk_entry_set_text (GTK_ENTRY(glade_xml_get_widget(xml, "fr_hostname")), "localhost");
	gtk_entry_set_text (GTK_ENTRY(glade_xml_get_widget(xml, "fr_port")), "6600");
	
	entry = glade_xml_get_widget (xml, "fr_password");
	gtk_entry_set_visibility (GTK_ENTRY(entry), FALSE);
	gtk_entry_set_invisible_char (GTK_ENTRY(entry), g_utf8_get_char("*"));

	check = glade_xml_get_widget (xml, "fr_systray_toggle");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check), TRUE);
		
	gtk_widget_show (window);
		
	return;
}

static void
on_fr_close_clicked (GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy (data);
	
	cfg_free_config_file_struct (&cf);
	if (gimmix_config_exists())
	{	
		gimmix_config_init ();
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
	exit (0);
	return;
}

static void
on_fr_apply_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget 	*entry;
	GtkWidget	*s_checkbox;
	const gchar *host;
	const gchar *port;
	const gchar *password;
	const gchar *dir;
	gchar 		*rcfile;
	
	entry = glade_xml_get_widget (xml,"fr_hostname");
	host = gtk_entry_get_text (GTK_ENTRY(entry));
	cfg_add_key (&cf, "mpd_hostname", (char *)host);
	
	entry = glade_xml_get_widget (xml,"fr_port");
	port = gtk_entry_get_text (GTK_ENTRY(entry));
	cfg_add_key (&cf, "mpd_port", (char *)port);
	
	entry = glade_xml_get_widget (xml,"fr_password");
	password = gtk_entry_get_text (GTK_ENTRY(entry));
	cfg_add_key (&cf, "mpd_password", (char *)password);
	
	entry = glade_xml_get_widget (xml, "fst_music_dir");
	dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER(entry));
	cfg_add_key (&cf, "music_directory", (char *)dir);
	
	s_checkbox = glade_xml_get_widget (xml, "fr_systray_toggle");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(s_checkbox)))
	cfg_add_key (&cf, "enable_systray", "true");
	else
	cfg_add_key (&cf, "enable_systray", "false");
	
	rcfile = cfg_get_path_to_config_file (CONFIG_FILE);
	cfg_write_config_file (&cf, rcfile);
	g_free (rcfile);
	
	return;
}
