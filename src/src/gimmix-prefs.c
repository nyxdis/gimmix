/*
 * gimmix-prefs.c
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
 
#include "gimmix-prefs.h"
#include "gimmix-systray.h"
#include "gimmix-interface.h"
#include "gimmix.h"

extern GM 			*pub;
extern GladeXML 	*xml;

static void 	cb_pref_apply_clicked (GtkWidget *widget, gpointer data);
static void		cb_pref_systray_checkbox_toggled (GtkToggleButton *button, gpointer data);
static void		cb_pref_notify_checkbox_toggled (GtkToggleButton *button, gpointer data);
static void		cb_pref_notify_timeout_spin_change (GtkSpinButton *button, gpointer data);

void
gimmix_prefs_dialog_show (void)
{
	gchar 		*port;
	gint 		systray_enable;
	gint		play_immediate;
	gint		stop_on_exit;
	GtkWidget	*entry;
	GtkWidget	*widget;
	GtkWidget	*pref_window;
	
	pref_window = glade_xml_get_widget (xml, "prefs_window");
	port = g_strdup_printf ("%d", pub->conf->port);
	systray_enable = pub->conf->systray_enable;
	play_immediate = pub->conf->play_immediate;
	stop_on_exit = pub->conf->stop_on_exit;

	gtk_entry_set_text (GTK_ENTRY(glade_xml_get_widget (xml,"host_entry")), pub->conf->hostname);

	gtk_entry_set_text (GTK_ENTRY(glade_xml_get_widget (xml,"port_entry")), port);
	g_free (port);
	
	entry = glade_xml_get_widget (xml,"password_entry");
	gtk_entry_set_visibility (GTK_ENTRY(entry), FALSE);
	gtk_entry_set_invisible_char (GTK_ENTRY(entry), g_utf8_get_char("*"));
		
	if (strlen(pub->conf->password)>1)
	{	
		gtk_entry_set_text (GTK_ENTRY(entry), pub->conf->password);
	}

	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(glade_xml_get_widget (xml, "conf_dir_chooser")), pub->conf->musicdir);
	
	entry = glade_xml_get_widget (xml, "systray_checkbutton");
	if (systray_enable == 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(entry), TRUE);
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(entry), FALSE);
	}
	g_signal_connect (G_OBJECT(entry), "toggled", G_CALLBACK(cb_pref_systray_checkbox_toggled), NULL);
	
	widget = glade_xml_get_widget (xml, "pref_play_immediate");
	if (play_immediate == 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(widget), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(widget), FALSE);
		
	widget = glade_xml_get_widget (xml, "pref_stop_on_exit");
	if (stop_on_exit == 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(widget), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(widget), FALSE);
	
	g_signal_connect (G_OBJECT(glade_xml_get_widget (xml, "button_apply")), "clicked", G_CALLBACK(cb_pref_apply_clicked), NULL);
	gtk_widget_show (GTK_WIDGET(pref_window));
	
	return;
}

static void
cb_pref_apply_clicked (GtkWidget *widget, gpointer data)
{
	const gchar *host;
	const gchar *port;
	const gchar *password;
	const gchar *dir;
	gint		val;
	GtkWidget 	*pref_widget;

	pref_widget = glade_xml_get_widget (xml,"host_entry");
	host = gtk_entry_get_text (GTK_ENTRY(pref_widget));
	
	pref_widget = glade_xml_get_widget (xml,"port_entry");
	port = gtk_entry_get_text (GTK_ENTRY(pref_widget));
	
	pref_widget = glade_xml_get_widget (xml,"password_entry");
	password = gtk_entry_get_text (GTK_ENTRY(pref_widget));

	pref_widget = glade_xml_get_widget (xml, "conf_dir_chooser");
	dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER(pref_widget));
	
	pref_widget = glade_xml_get_widget (xml, "pref_play_immediate");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_widget)))
		pub->conf->play_immediate = 1;
	else
		pub->conf->play_immediate = 0;
		
	pref_widget = glade_xml_get_widget (xml, "pref_stop_on_exit");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_widget)))
		pub->conf->stop_on_exit = 1;
	else
		pub->conf->stop_on_exit = 0;
	
	strncpy (pub->conf->musicdir, dir, 255);
	strncpy (pub->conf->hostname, host, 255);
	strncpy (pub->conf->password, password, 255);
	pub->conf->port = atoi (port);

	gimmix_config_save (pub->conf);
	
	return;
}

static void
cb_pref_systray_checkbox_toggled (GtkToggleButton *button, gpointer data)
{
	if (gtk_toggle_button_get_active(button) == TRUE)
	{
		gimmix_enable_systray_icon ();
		pub->conf->systray_enable = 1;
	}
	else
	if (gtk_toggle_button_get_active(button) == FALSE)
	{
			gimmix_disable_systray_icon ();
			pub->conf->systray_enable = 0;
	}
	
	gimmix_config_save (pub->conf);

	return;
}

