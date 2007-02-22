/*
 * gimmix-prefs.c
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
 
#include "gimmix-prefs.h"
#include "gimmix-systray.h"
#include "gimmix-interface.h"
#include "gimmix.h"

extern MpdObj		*gmo;
extern GladeXML 	*xml;
extern ConfigFile	conf;

GtkWidget *pref_window;
GtkWidget *pref_notebook;
GtkWidget *pref_host_entry;
GtkWidget *pref_pass_entry;
GtkWidget *pref_port_entry;
GtkWidget *pref_systray_check;
GtkWidget *pref_notification_check;
GtkWidget *pref_play_immediate_check;
GtkWidget *pref_stop_exit_check;
GtkWidget *pref_upd_startup_check;
GtkWidget *pref_crossfade_check;
GtkWidget *pref_crossfade_spin;
GtkWidget *pref_button_apply;
GtkWidget *pref_button_close;
GtkWidget *pref_dir_chooser;
GtkWidget *pref_search_check;

extern GtkWidget *search_box;

static void 	cb_pref_apply_clicked (GtkWidget *widget, gpointer data);
static void		cb_pref_systray_toggled (GtkToggleButton *button, gpointer data);
static void		cb_pref_notification_toggled (GtkToggleButton *button, gpointer data);
static void		cb_pref_search_toggled (GtkToggleButton *button, gpointer data);
static void		cb_pref_crossfade_toggled (GtkToggleButton *button, gpointer data);

void
gimmix_prefs_init (void)
{
	pref_window = glade_xml_get_widget (xml, "prefs_window");
	pref_host_entry = glade_xml_get_widget (xml, "host_entry");
	pref_pass_entry = glade_xml_get_widget (xml, "password_entry");
	pref_port_entry = glade_xml_get_widget (xml, "port_entry");
	pref_systray_check = glade_xml_get_widget (xml, "systray_checkbutton");
	pref_notification_check = glade_xml_get_widget (xml, "tooltip_checkbutton");
	pref_play_immediate_check = glade_xml_get_widget (xml, "pref_play_immediate");
	pref_stop_exit_check = glade_xml_get_widget (xml, "pref_stop_on_exit");
	pref_crossfade_check = glade_xml_get_widget (xml, "pref_crossfade");
	pref_upd_startup_check = glade_xml_get_widget (xml, "pref_upd_on_startup");
	pref_crossfade_spin = glade_xml_get_widget (xml, "crossfade_spin");
	pref_button_apply = glade_xml_get_widget (xml, "button_apply");
	pref_button_close = glade_xml_get_widget (xml, "prefs_window");
	pref_dir_chooser = glade_xml_get_widget (xml, "conf_dir_chooser");
	pref_search_check = glade_xml_get_widget (xml, "search_checkbutton");
	pref_notebook = glade_xml_get_widget (xml, "pref_notebook");
	
	g_signal_connect (G_OBJECT(pref_systray_check), "toggled", G_CALLBACK(cb_pref_systray_toggled), (gpointer)pref_notification_check);
	g_signal_connect (G_OBJECT(pref_notification_check), "toggled", G_CALLBACK(cb_pref_notification_toggled), NULL);
	g_signal_connect (G_OBJECT(pref_crossfade_check), "toggled", G_CALLBACK(cb_pref_crossfade_toggled), pref_crossfade_spin);
	g_signal_connect (G_OBJECT(pref_button_apply), "clicked", G_CALLBACK(cb_pref_apply_clicked), NULL);
	g_signal_connect (G_OBJECT(pref_search_check), "toggled", G_CALLBACK(cb_pref_search_toggled), NULL);
	g_signal_connect (G_OBJECT(pref_window), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
	
	return;
}

void
gimmix_prefs_dialog_show (void)
{
	gchar 		*port;
	gint		crossfade_time;
	
	port = g_strdup_printf ("%s", cfg_get_key_value (conf, "mpd_port"));

	gtk_entry_set_text (GTK_ENTRY(pref_host_entry), cfg_get_key_value (conf, "mpd_hostname"));

	gtk_entry_set_text (GTK_ENTRY(pref_port_entry), port);
	g_free (port);
	
	gtk_entry_set_visibility (GTK_ENTRY(pref_pass_entry), FALSE);
	gtk_entry_set_invisible_char (GTK_ENTRY(pref_pass_entry), g_utf8_get_char("*"));
		
	gtk_entry_set_text (GTK_ENTRY(pref_pass_entry), cfg_get_key_value (conf, "mpd_password"));

	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(pref_dir_chooser), cfg_get_key_value(conf, "music_directory"));
	
	if (strncasecmp(cfg_get_key_value(conf, "enable_systray"), "true", 4) == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_systray_check), TRUE);
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_systray_check), FALSE);
	}
	
	if (strncasecmp(cfg_get_key_value(conf, "enable_notification"), "true", 4) == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_notification_check), TRUE);
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_notification_check), FALSE);
	}
	
	if (strncasecmp(cfg_get_key_value(conf, "play_on_add"), "true", 4) == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_play_immediate_check), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_play_immediate_check), FALSE);
	
	if (strncasecmp(cfg_get_key_value(conf, "stop_on_exit"), "true", 4) == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_stop_exit_check), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_stop_exit_check), FALSE);
		
	if (strncasecmp(cfg_get_key_value(conf, "update_on_startup"), "true", 4) == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_upd_startup_check), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_upd_startup_check), FALSE);
	
	crossfade_time = mpd_status_get_crossfade (gmo);
	if (crossfade_time != 0)
	{
		gtk_widget_set_sensitive (GTK_WIDGET(pref_crossfade_check), TRUE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_crossfade_check), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET(pref_crossfade_spin), TRUE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON(pref_crossfade_spin), (gdouble)crossfade_time);
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_crossfade_check), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET(pref_crossfade_spin), FALSE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON(pref_crossfade_spin), (gdouble)crossfade_time);
	}
	
	if (strncasecmp(cfg_get_key_value(conf, "enable_search"), "true", 4) == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_search_check), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_search_check), FALSE);
	
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

	host = gtk_entry_get_text (GTK_ENTRY(pref_host_entry));
	cfg_add_key (&conf, "mpd_hostname", (char *)host);
	
	port = gtk_entry_get_text (GTK_ENTRY(pref_port_entry));
	cfg_add_key (&conf, "mpd_port", (char *)port);
	
	password = gtk_entry_get_text (GTK_ENTRY(pref_pass_entry));
	cfg_add_key (&conf, "mpd_password", (char *)password);
	
	dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER(pref_dir_chooser));
	cfg_add_key (&conf, "music_directory", (char *)dir);
		
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_play_immediate_check)))
		cfg_add_key (&conf, "play_on_add", "true");
	else
		cfg_add_key (&conf, "play_on_add", "false");
		
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_stop_exit_check)))
		cfg_add_key (&conf, "stop_on_exit", "true");
	else
		cfg_add_key (&conf, "stop_on_exit", "false");
	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_upd_startup_check)))
		cfg_add_key (&conf, "update_on_startup", "true");
	else
		cfg_add_key (&conf, "update_on_startup", "false");
		
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(pref_crossfade_check)))
	{
		gint val = gtk_spin_button_get_value (GTK_SPIN_BUTTON(pref_crossfade_spin));
		mpd_status_set_crossfade (gmo, val);
	}
	else
	{
		mpd_status_set_crossfade (gmo, 0);
	}
	
	gimmix_config_save ();

	return;
}

static void
cb_pref_crossfade_toggled (GtkToggleButton *button, gpointer data)
{
	gtk_widget_set_sensitive (GTK_WIDGET(data), gtk_toggle_button_get_active (button));
	
	return;
}

static void
cb_pref_systray_toggled (GtkToggleButton *button, gpointer data)
{
	if (gtk_toggle_button_get_active(button) == TRUE)
	{
		gimmix_enable_systray_icon ();
		cfg_add_key (&conf, "enable_systray", "true");
	}
	else
	if (gtk_toggle_button_get_active(button) == FALSE)
	{
		gimmix_disable_systray_icon ();
		cfg_add_key (&conf, "enable_systray", "false");
		/* disable notificaiton tooltips too */
		gtk_toggle_button_set_active (data, FALSE);
	}
	
	gimmix_config_save ();

	return;
}

static void
cb_pref_notification_toggled (GtkToggleButton *button, gpointer data)
{
	if (gtk_toggle_button_get_active(button) == TRUE)
	{
		cfg_add_key (&conf, "enable_notification", "true");
	}
	else
	if (gtk_toggle_button_get_active(button) == FALSE)
	{
		cfg_add_key (&conf, "enable_notification", "false");
	}
	
	gimmix_config_save ();
	
	return;
}

static void
cb_pref_search_toggled (GtkToggleButton *button, gpointer data)
{
	if (gtk_toggle_button_get_active(button) == TRUE)
	{
		gtk_widget_show (search_box);
		cfg_add_key (&conf, "enable_search", "true");
	}
	else
	if (gtk_toggle_button_get_active(button) == FALSE)
	{
		gtk_widget_hide (search_box);
		cfg_add_key (&conf, "enable_search", "false");
	}

	return;
}

