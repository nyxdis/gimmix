/*
 * gimmix-firstrun.c
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
#include "gimmix-firstrun.h"
#include "gimmix.h"

static void on_fr_close_clicked (GtkWidget *widget, gpointer data);
static void on_fr_apply_clicked (GtkWidget *widget, gpointer data);
static void on_fr_systray_checkbox_toggled (GtkToggleButton *button, gpointer data);

void
gimmix_show_firstrun_dialog (void)
{
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *check;
	GtkWidget *entry;

	window = glade_xml_get_widget (xml, "first_run_dialog");
	button = glade_xml_get_widget (xml, "fr_apply");
	g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK(on_fr_apply_clicked), NULL);
	g_signal_connect (G_OBJECT(window), "delete_event", G_CALLBACK(gtk_main_quit), NULL);
	button = glade_xml_get_widget (xml, "fr_close");
	g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK(on_fr_close_clicked), window);
	
	entry = glade_xml_get_widget (xml, "fr_password");
	gtk_entry_set_visibility (GTK_ENTRY(entry), FALSE);
	gtk_entry_set_invisible_char (GTK_ENTRY(entry), g_utf8_get_char("*"));

	check = glade_xml_get_widget (xml, "fr_systray_toggle");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check), TRUE);
	g_signal_connect (G_OBJECT(check), "toggled", G_CALLBACK(on_fr_systray_checkbox_toggled), NULL);
		
	gtk_widget_show (window);
		
	return;
}

static void
on_fr_close_clicked (GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy (data);
	
	GtkWidget *main_window = glade_xml_get_widget (xml, "main_window");
	
	if (!pub->conf)
	{
		gtk_main_quit ();
	}
	else
	{
		if (gimmix_connect())
		{
			gtk_widget_show (main_window);
			gimmix_init ();
			return;
		}
		else
		{
			gimmix_connect_error ();
		}
	}
	return;
}

static void
on_fr_apply_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget 	*entry;
	GtkWidget	*s_checkbox;
	GtkWidget	*n_checkbox;
	const gchar *host;
	const gchar *port;
	const gchar *password;
	const gchar *dir;

	pub->conf = (Conf*) malloc(sizeof(Conf));

	entry = glade_xml_get_widget (xml,"fr_hostname");
	host = gtk_entry_get_text (GTK_ENTRY(entry));
	
	entry = glade_xml_get_widget (xml,"fr_port");
	port = gtk_entry_get_text (GTK_ENTRY(entry));
	
	entry = glade_xml_get_widget (xml,"fr_password");
	password = gtk_entry_get_text (GTK_ENTRY(entry));
	
	entry = glade_xml_get_widget (xml, "fst_music_dir");
	dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER(entry));
	
	s_checkbox = glade_xml_get_widget (xml, "fr_systray_toggle");
	n_checkbox = glade_xml_get_widget (xml, "fr_notify_toggle");

	strncpy (pub->conf->hostname, host, 255);
	strncpy (pub->conf->password, password, 255);
	strncpy (pub->conf->musicdir, dir, 255);
	pub->conf->port = atoi(port);
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(s_checkbox)))
		pub->conf->systray_enable = 1;
	else
		pub->conf->systray_enable = 0;
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(n_checkbox)))
		pub->conf->notify_enable = 1;
	else
		pub->conf->notify_enable = 0;

	pub->conf->play_immediate = 0;
	gimmix_config_save (pub->conf);
}

static void
on_fr_systray_checkbox_toggled (GtkToggleButton *button, gpointer data)
{
	GtkWidget *notify_checkbox;
	
	notify_checkbox = glade_xml_get_widget (xml, "fr_notify_toggle");
	if (gtk_toggle_button_get_active(button) == TRUE)
		gtk_widget_set_sensitive (notify_checkbox, TRUE);
	else
		gtk_widget_set_sensitive (notify_checkbox, FALSE);
	
	return;
}
