/*
 * gimmix-prefs.c
 *
 * Copyright (C) 2006-2008 Priyank Gosalia
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
#include "gimmix-covers.h"
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
GtkWidget *pref_outputdev_tvw;
GtkWidget *pref_coverart_vbox;
GtkWidget *pref_use_proxy_check;
GtkWidget *pref_proxy_host_entry;
GtkWidget *pref_proxy_port_spin;

#ifdef HAVE_COVER_PLUGIN
static GtkWidget *pref_coverart_check;
extern GtkWidget *gimmix_plcbox_frame;
extern char *cover_locations[6][2];
static GtkWidget *pref_coverart_loc_combo;
#endif

extern GtkWidget *search_box;

static void 	cb_pref_apply_clicked (GtkWidget *widget, gpointer data);
static void	cb_pref_systray_toggled (GtkToggleButton *button, gpointer data);
static void	cb_pref_notification_toggled (GtkToggleButton *button, gpointer data);
static void	cb_pref_search_toggled (GtkToggleButton *button, gpointer data);
static void	cb_pref_crossfade_toggled (GtkToggleButton *button, gpointer data);
static void	cb_pref_outputdev_enable_toggled (GtkCellRendererToggle *toggle, gchar *path_str, gpointer data);
static void	cb_pref_use_proxy_toggled (GtkToggleButton *button, gpointer data);

#ifdef HAVE_COVER_PLUGIN
static void	gimmix_prefs_setup_covers_location_combo (void);
static void	cb_pref_coverart_disp_toggled (GtkToggleButton *button, gpointer data);
#endif

void
gimmix_prefs_init (void)
{
	GtkListStore		*store = NULL;
	GtkCellRenderer		*renderer = NULL;
	GtkTreeViewColumn	*column = NULL;

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
	pref_outputdev_tvw = glade_xml_get_widget (xml, "pref_outputdev_tvw");
	pref_use_proxy_check = glade_xml_get_widget (xml, "use_proxy_checkbtn");
	pref_proxy_host_entry = glade_xml_get_widget (xml, "proxy_host_entry");
	pref_proxy_port_spin = glade_xml_get_widget (xml, "proxy_port_spin");
	#ifdef HAVE_COVER_PLUGIN
	pref_coverart_check = glade_xml_get_widget (xml, "coverart_checkbutton");
	pref_coverart_loc_combo = glade_xml_get_widget (xml, "pref_coverart_location");
	pref_coverart_vbox = glade_xml_get_widget(xml,"pref_coverart_vbox");
	gimmix_prefs_setup_covers_location_combo ();
	#else
	gtk_widget_hide (glade_xml_get_widget(xml,"pref_interface_ifacebox"));
	gtk_widget_hide (glade_xml_get_widget(xml,"pref_coverart_vbox"));
	#endif
	
	g_signal_connect (G_OBJECT(pref_use_proxy_check), "toggled", G_CALLBACK(cb_pref_use_proxy_toggled), NULL);
	g_signal_connect (G_OBJECT(pref_systray_check), "toggled", G_CALLBACK(cb_pref_systray_toggled), (gpointer)pref_notification_check);
	g_signal_connect (G_OBJECT(pref_notification_check), "toggled", G_CALLBACK(cb_pref_notification_toggled), NULL);
	g_signal_connect (G_OBJECT(pref_crossfade_check), "toggled", G_CALLBACK(cb_pref_crossfade_toggled), pref_crossfade_spin);
	g_signal_connect (G_OBJECT(pref_button_apply), "clicked", G_CALLBACK(cb_pref_apply_clicked), NULL);
	g_signal_connect (G_OBJECT(pref_search_check), "toggled", G_CALLBACK(cb_pref_search_toggled), NULL);
	g_signal_connect (G_OBJECT(pref_window), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
	#ifdef HAVE_COVER_PLUGIN
	g_signal_connect (G_OBJECT(pref_coverart_check), "toggled", G_CALLBACK(cb_pref_coverart_disp_toggled), NULL);
	#endif
	
	/* setup output devices treeview */
	store = gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_INT);
	renderer = gtk_cell_renderer_toggle_new ();
	g_object_set (G_OBJECT(renderer), "activatable", TRUE, NULL);
	g_signal_connect (renderer, "toggled", G_CALLBACK(cb_pref_outputdev_enable_toggled), store);
	column = gtk_tree_view_column_new_with_attributes (_("Enabled"),
							renderer,
							"active", 0,
							NULL);
	gtk_tree_view_column_set_resizable (column, FALSE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(pref_outputdev_tvw), column);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Device"),
							renderer,
							"text", 1,
							NULL);
	gtk_tree_view_column_set_resizable (column, FALSE);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 140);
	gtk_tree_view_append_column (GTK_TREE_VIEW(pref_outputdev_tvw), column);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW(pref_outputdev_tvw), GTK_TREE_MODEL(store));
	
	return;
}

void
gimmix_prefs_dialog_show (void)
{
	gchar 		*port = NULL;
	gchar		*phost = NULL;
	gint		crossfade_time;
	gboolean	syst = FALSE;
	MpdData		*d = NULL;
	GtkListStore	*store = NULL;
	GtkTreeIter	iter;
	GtkTreeModel	*model = NULL;
	gint		proxy_port;
	
	port = g_strdup_printf ("%s", cfg_get_key_value (conf, "mpd_port"));

	gtk_entry_set_text (GTK_ENTRY(pref_host_entry), cfg_get_key_value (conf, "mpd_hostname"));

	gtk_entry_set_text (GTK_ENTRY(pref_port_entry), port);
	g_free (port);
	port = NULL;
	
	gtk_entry_set_visibility (GTK_ENTRY(pref_pass_entry), FALSE);
	gtk_entry_set_invisible_char (GTK_ENTRY(pref_pass_entry), g_utf8_get_char("*"));
		
	gtk_entry_set_text (GTK_ENTRY(pref_pass_entry), cfg_get_key_value (conf, "mpd_password"));

	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(pref_dir_chooser), cfg_get_key_value(conf, "music_directory"));
	
	if (strncasecmp(cfg_get_key_value(conf, "enable_systray"), "true", 4) == 0)
	{	
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_systray_check), TRUE);
		syst = TRUE;
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_systray_check), FALSE);
		syst = FALSE;
	}
	
	if (syst)
	{
		if (strncasecmp(cfg_get_key_value(conf, "enable_notification"), "true", 4) == 0)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_notification_check), TRUE);
		else
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_notification_check), FALSE);
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_notification_check), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET(pref_notification_check), FALSE);
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
	
	/* crossfade */
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
	
	/* output devices */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(pref_outputdev_tvw));
	store = GTK_LIST_STORE (model);
	gtk_list_store_clear (store);
	d = mpd_server_get_output_devices (gmo);
	while (d!=NULL)
	{
		gboolean enabled = d->output_dev->enabled;
		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
						0, enabled,
						1, g_strdup(d->output_dev->name),
						2, d->output_dev->id,
						-1);
		d = mpd_data_get_next (d);
	}
	
	if (strncasecmp(cfg_get_key_value(conf, "enable_search"), "true", 4) == 0)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_search_check), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_search_check), FALSE);
	
	/* proxy stuff */
	if (!strncasecmp(cfg_get_key_value(conf,"proxy_enable"),"true",4))
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_use_proxy_check), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET(pref_proxy_host_entry), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET(pref_proxy_port_spin), TRUE);
		
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_use_proxy_check), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET(pref_proxy_host_entry), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET(pref_proxy_port_spin), FALSE);
	}
	phost = cfg_get_key_value (conf, "proxy_host");
	if (phost != NULL)
	{
		gtk_entry_set_text (GTK_ENTRY(pref_proxy_host_entry), phost);
	}
	else
	{
		gtk_entry_set_text (GTK_ENTRY(pref_proxy_host_entry), "");
	}
	port = 	cfg_get_key_value (conf, "proxy_port");
	if (port != NULL && strlen(port))
	{
		proxy_port = atoi (port);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON(pref_proxy_port_spin), (gdouble)proxy_port);
	}
	else
	{
		proxy_port = 8080;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON(pref_proxy_port_spin), (gdouble)proxy_port);
	}
	
	#ifdef HAVE_COVER_PLUGIN
	/* cover art enable check */
	if (!strncasecmp(cfg_get_key_value(conf,"coverart_enable"),"true",4))
	{
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_coverart_check), TRUE);
	}
	else
	{
		
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pref_coverart_check), FALSE);
	}
	/* cover art location */
	char *loc = cfg_get_key_value (conf, "coverart_location");
	if (loc!=NULL)
	{
		int i;
		for (i=0;i<5;i++)
		{
			if (!strcmp(cover_locations[i][0],loc))
				break;
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX(pref_coverart_loc_combo), i);
	}
	#else
	gtk_widget_hide (GTK_WIDGET(pref_coverart_vbox));
	#endif
	gtk_widget_show (GTK_WIDGET(pref_window));
	
	return;
}

static void
cb_pref_outputdev_enable_toggled (GtkCellRendererToggle *toggle, gchar *path_str, gpointer data)
{
	GtkTreeModel	*model = NULL;
	GtkTreeIter	iter;
	GtkTreePath	*path;
	gboolean	enabled;
	guint		id;
	guint		state = 0;

	model = (GtkTreeModel *)data;
	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &enabled, 2, &id, -1);
	enabled ^= 1;
	gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, enabled, -1);
	state = (enabled)?1:0;
	mpd_server_set_output_device (gmo, id, state);
	
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
	
	/* proxy server stuff */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pref_use_proxy_check)))
	{
		host = gtk_entry_get_text (GTK_ENTRY(pref_proxy_host_entry));
		if (host == NULL || !strlen(host))
		{
			gimmix_error ("Please specify a valid proxy server hostname / ip address");
			return;
		}
		else
		{
			port = NULL;
			port = g_strdup_printf ("%d", (gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(pref_proxy_port_spin)));
			cfg_add_key (&conf, "proxy_host", (char*) host);
			cfg_add_key (&conf, "proxy_port", (char*) port);
			g_free (port);
		}
	}

	#ifdef HAVE_COVER_PLUGIN
	guint locid = gtk_combo_box_get_active (GTK_COMBO_BOX(pref_coverart_loc_combo));
	cfg_add_key (&conf, "coverart_location", cover_locations[locid][0]);
	#endif
	
	gimmix_config_save ();

	return;
}

static void
cb_pref_use_proxy_toggled (GtkToggleButton *button, gpointer data)
{
	if (gtk_toggle_button_get_active(button) == TRUE)
	{
		cfg_add_key (&conf, "proxy_enable", "true");
		gtk_widget_set_sensitive (GTK_WIDGET(pref_proxy_host_entry), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET(pref_proxy_port_spin), TRUE);
	}
	else
	{
		cfg_add_key (&conf, "proxy_enable", "false");
		gtk_widget_set_sensitive (GTK_WIDGET(pref_proxy_host_entry), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET(pref_proxy_port_spin), FALSE);
	}
	
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
		gtk_widget_set_sensitive (GTK_WIDGET(pref_notification_check), TRUE);
	}
	else
	if (gtk_toggle_button_get_active(button) == FALSE)
	{
		gimmix_disable_systray_icon ();
		cfg_add_key (&conf, "enable_systray", "false");
		/* disable notificaiton tooltips too */
		gtk_toggle_button_set_active (data, FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET(pref_notification_check), FALSE);
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

#ifdef HAVE_COVER_PLUGIN
static void
cb_pref_coverart_disp_toggled (GtkToggleButton *button, gpointer data)
{
	if (gtk_toggle_button_get_active(button) == TRUE)
	{
		g_thread_create ((GThreadFunc)gimmix_covers_plugin_update_cover,
						NULL,
						FALSE,
						NULL);
		gtk_widget_show (gimmix_plcbox_frame);
		gimmix_metadata_show_song_cover (TRUE);
		cfg_add_key (&conf, "coverart_enable", "true");
	}
	else
	if (gtk_toggle_button_get_active(button) == FALSE)
	{
		gtk_widget_hide (gimmix_plcbox_frame);
		gimmix_metadata_show_song_cover (FALSE);
		gimmix_systray_tooltip_set_default_image ();
		cfg_add_key (&conf, "coverart_enable", "false");
	}
	
	gimmix_config_save ();

	return;
}

static void
gimmix_prefs_setup_covers_location_combo (void)
{
	GtkListStore		*store = NULL;
	GtkTreeIter		iter;
	guint			i = 0;
	
	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING); /* LOCID, LOCATION */
	gtk_combo_box_set_model (GTK_COMBO_BOX(pref_coverart_loc_combo), GTK_TREE_MODEL(store));
	
	/* populate */
	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX(pref_coverart_loc_combo)));
	gtk_list_store_clear (store);
	for (i=0;i<=5;i++)
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, cover_locations[i][1], 1, cover_locations[i][0], -1);
	}
	gtk_combo_box_set_model (GTK_COMBO_BOX(pref_coverart_loc_combo), GTK_TREE_MODEL(store));
	
	return;
}

#endif