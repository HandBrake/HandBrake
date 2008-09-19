/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.c
 * Copyright (C) John Stebbins 2008 <stebbins@stebbins>
 * 
 * callbacks.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libhal-storage.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include "callbacks.h"
#include "resources.h"
#include "settings.h"
#include "presets.h"
#include "values.h"
#include "plist.h"
#include "hb-backend.h"
#include "ghb-dvd.h"
#include "ghbcellrenderertext.h"
#include "hb.h"

static void update_chapter_list(signal_user_data_t *ud);
static void clear_audio_list(signal_user_data_t *ud);
static GList* dvd_device_list();
static gboolean cancel_encode();
static void audio_list_refresh_selected(signal_user_data_t *ud);
static GValue* get_selected_asettings(signal_user_data_t *ud);
static gint find_last_finished(GValue *queue);

// This is a dependency map used for greying widgets
// that are dependent on the state of another widget.
// The enable_value comes from the values that are
// obtained from ghb_widget_value().  For combo boxes
// you will have to look further to combo box options
// maps in hb-backend.c

GValue *dep_map;
GValue *rev_map;

void
ghb_init_dep_map()
{
	dep_map = ghb_resource_get("widget-deps");
	rev_map = ghb_resource_get("widget-reverse-deps");
}

static gboolean
dep_check(signal_user_data_t *ud, const gchar *name)
{
	GtkWidget *widget;
	GObject *dep_object;
	gint ii;
	gint count;
	gboolean result = TRUE;
	GValue *array, *data;
	gchar *widget_name;
	
	g_debug("dep_check () %s", name);

	array = ghb_dict_lookup(rev_map, name);
	count = ghb_array_len(array);
	for (ii = 0; ii < count; ii++)
	{
		data = ghb_array_get_nth(array, ii);
		widget_name = ghb_value_string(ghb_array_get_nth(data, 0));
		widget = GHB_WIDGET(ud->builder, widget_name);
		dep_object = gtk_builder_get_object(ud->builder, name);
		g_free(widget_name);
		if (dep_object == NULL)
		{
			g_message("Failed to find widget");
		}
		else
		{
			gchar *value;
			gint jj = 0;
			gchar **values;
			gboolean sensitive = FALSE;
			gboolean die;

			die = ghb_value_boolean(ghb_array_get_nth(data, 2));
			value = ghb_value_string(ghb_array_get_nth(data, 1));
			values = g_strsplit(value, "|", 10);
			g_free(value);

			if (widget)
				value = ghb_widget_string(widget);
			else
				value = ghb_settings_get_string(ud->settings, name);
			while (values && values[jj])
			{
				if (values[jj][0] == '>')
				{
					gdouble dbl = g_strtod (&values[jj][1], NULL);
					gdouble dvalue = ghb_widget_double(widget);
					if (dvalue > dbl)
					{
						sensitive = TRUE;
						break;
					}
				}
				else if (values[jj][0] == '<')
				{
					gdouble dbl = g_strtod (&values[jj][1], NULL);
					gdouble dvalue = ghb_widget_double(widget);
					if (dvalue < dbl)
					{
						sensitive = TRUE;
						break;
					}
				}
				if (strcmp(values[jj], value) == 0)
				{
					sensitive = TRUE;
					break;
				}
				jj++;
			}
			sensitive = die ^ sensitive;
			if (!sensitive) result = FALSE;
			g_strfreev (values);
			g_free(value);
		}
	}
	return result;
}

static void
check_dependency(signal_user_data_t *ud, GtkWidget *widget)
{
	GObject *dep_object;
	const gchar *name;
	GValue *array, *data;
	gint count, ii;
	gchar *dep_name;
	GType type;

	type = GTK_WIDGET_TYPE(widget);
	if (type == GTK_TYPE_COMBO_BOX || type == GTK_TYPE_COMBO_BOX_ENTRY)
		if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) < 0) return;

	name = gtk_widget_get_name(widget);
	g_debug("check_dependency () %s", name);

	array = ghb_dict_lookup(dep_map, name);
	count = ghb_array_len(array);
	for (ii = 0; ii < count; ii++)
	{
		gboolean sensitive;

		data = ghb_array_get_nth(array, ii);
		dep_name = ghb_value_string(data);
		dep_object = gtk_builder_get_object(ud->builder, dep_name);
		if (dep_object == NULL)
		{
			g_message("Failed to find dependent widget %s", dep_name);
			g_free(dep_name);
			continue;
		}
		sensitive = dep_check(ud, dep_name);
		g_free(dep_name);
		if (GTK_IS_ACTION(dep_object))
			gtk_action_set_sensitive(GTK_ACTION(dep_object), sensitive);
		else
			gtk_widget_set_sensitive(GTK_WIDGET(dep_object), sensitive);
	}
}

void
ghb_check_all_depencencies(signal_user_data_t *ud)
{
	GHashTableIter iter;
	gchar *dep_name;
	GValue *value;
	GObject *dep_object;

	g_debug("ghb_check_all_depencencies ()");
	ghb_dict_iter_init(&iter, rev_map);
	// middle (void*) cast prevents gcc warning "defreferencing type-punned
	// pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&dep_name, (gpointer*)(void*)&value))
	{
		gboolean sensitive;
		dep_object = gtk_builder_get_object (ud->builder, dep_name);
		if (dep_object == NULL)
		{
			g_message("Failed to find dependent widget %s", dep_name);
			continue;
		}
		sensitive = dep_check(ud, dep_name);
		if (GTK_IS_ACTION(dep_object))
			gtk_action_set_sensitive(GTK_ACTION(dep_object), sensitive);
		else
			gtk_widget_set_sensitive(GTK_WIDGET(dep_object), sensitive);
	}
}

static void
clear_presets_selection(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	
	if (ud->dont_clear_presets) return;
	g_debug("clear_presets_selection()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	gtk_tree_selection_unselect_all (selection);
	ghb_settings_set_boolean(ud->settings, "preset_modified", TRUE);
}

static gchar*
expand_tilde(const gchar *path)
{
	const gchar *user_home;
	gchar *home;
	const gchar *suffix;
	gchar *expanded_path = NULL;
	
	g_debug("expand_tilde ()");
	if (path[0] == '~')
	{
		user_home = g_get_home_dir();
		home = NULL; // squash warning about home uninitialized
		if (path[1] == 0)
		{
			home = g_strdup(user_home);
			suffix = "";
		}
		else if (path[1] == '/')
		{
			home = g_strdup(user_home);
			suffix = &path[2];
		}
		else
		{
			home = g_path_get_dirname(user_home);
			suffix = &path[1];
		}
		expanded_path = g_strdup_printf("%s/%s", home, suffix);
		g_free(home);
	}
	return expanded_path;
}

void
on_quit1_activate(GtkMenuItem *quit, signal_user_data_t *ud)
{
	gint state = ghb_get_queue_state();
	g_debug("on_quit1_activate ()");
	if (state & GHB_STATE_WORKING)
   	{
		if (cancel_encode("Closing HandBrake will terminate encoding.\n"))
		{
			ghb_hb_cleanup(FALSE);
			gtk_main_quit();
			return;
		}
		return;
	}
	ghb_hb_cleanup(FALSE);
	gtk_main_quit();
}

static void
set_destination(signal_user_data_t *ud)
{
	g_debug("set_destination");
	if (ghb_settings_get_boolean(ud->settings, "use_source_name"))
	{
		gchar *vol_name, *filename, *extension;
		gchar *dir, *new_name;
		
		filename = ghb_settings_get_string(ud->settings, "destination");
		extension = ghb_settings_get_string(ud->settings, "container");
		dir = g_path_get_dirname (filename);
		vol_name = ghb_settings_get_string(ud->settings, "volume_label");
		if (ghb_settings_get_boolean(ud->settings, "chapters_in_destination"))
		{
			gint start, end;

			start = ghb_settings_get_int(ud->settings, "start_chapter");
			end = ghb_settings_get_int(ud->settings, "end_chapter");
			if (start == end)
			{
				new_name = g_strdup_printf("%s/%s-%d.%s", 
					dir, vol_name, start, extension);
			}
			else
			{
				new_name = g_strdup_printf("%s/%s-%d-%d.%s", 
					dir, vol_name, start, end, extension);
			}
		}
		else
		{
			new_name = g_strdup_printf("%s/%s.%s", dir, vol_name, extension);
		}
		ghb_ui_update(ud, "destination", ghb_string_value(new_name));
		g_free(filename);
		g_free(extension);
		g_free(vol_name);
		g_free(dir);
		g_free(new_name);
	}
}

gboolean
uppers_and_unders(const gchar *str)
{
	if (str == NULL) return FALSE;
	while (*str)
	{
		if (*str == ' ')
		{
			return FALSE;
		}
		if (*str >= 'a' && *str <= 'z')
		{
			return FALSE;
		}
		str++;
	}
	return TRUE;
}

enum
{
	CAMEL_FIRST_UPPER,
	CAMEL_OTHER
};

void
camel_convert(gchar *str)
{
	gint state = CAMEL_OTHER;
	
	if (str == NULL) return;
	while (*str)
	{
		if (*str == '_') *str = ' ';
		switch (state)
		{
			case CAMEL_OTHER:
			{
				if (*str >= 'A' && *str <= 'Z')
					state = CAMEL_FIRST_UPPER;
				else
					state = CAMEL_OTHER;
				
			} break;
			case CAMEL_FIRST_UPPER:
			{
				if (*str >= 'A' && *str <= 'Z')
					*str = *str - 'A' + 'a';
				else
					state = CAMEL_OTHER;
			} break;
		}
		str++;
	}
}

static gboolean
update_source_label(signal_user_data_t *ud, const gchar *source)
{
	gchar *label = NULL;
	gint len;
	gchar **path;
	gchar *filename = g_strdup(source);
	
	len = strlen(filename);
	if (filename[len-1] == '/') filename[len-1] = 0;
	if (g_file_test(filename, G_FILE_TEST_IS_DIR))
	{
		path = g_strsplit(filename, "/", -1);
		len = g_strv_length (path);
		if ((len > 1) && (strcmp("VIDEO_TS", path[len-1]) == 0))
		{
			label = g_strdup(path[len-2]);
		}
		else
		{
			label = g_strdup(path[len-1]);
		}
		g_strfreev (path);
	}
	else
	{
		// Is regular file or block dev.
		// Check to see if it is a dvd image
		label = ghb_dvd_volname (filename);
		if (label == NULL)
		{
			path = g_strsplit(filename, "/", -1);
			len = g_strv_length (path);
			// Just use the last combonent of the path
			label = g_strdup(path[len-1]);
			g_strfreev (path);
		}
		else
		{
			if (uppers_and_unders(label))
			{
				camel_convert(label);
			}
		}
	}
	g_free(filename);
	GtkWidget *widget = GHB_WIDGET (ud->builder, "source_title");
	if (label != NULL)
	{
		gtk_label_set_text (GTK_LABEL(widget), label);
		ghb_settings_set_string(ud->settings, "volume_label", label);
		g_free(label);
		set_destination(ud);
	}
	else
	{
		label = "No Title Found";
		gtk_label_set_text (GTK_LABEL(widget), label);
		ghb_settings_set_string(ud->settings, "volume_label", label);
		return FALSE;
	}
	return TRUE;
}

static GtkWidget *dvd_device_combo = NULL;

void
chooser_file_selected_cb(GtkFileChooser *dialog, GtkComboBox *combo)
{
	const gchar *name = gtk_file_chooser_get_filename (dialog);
	GtkTreeModel *store;
	GtkTreeIter iter;
	const gchar *device;
	gboolean foundit = FALSE;
	
	if (name == NULL) return;
	store = gtk_combo_box_get_model(combo);
	if (gtk_tree_model_get_iter_first(store, &iter))
	{
		do
		{
			gtk_tree_model_get(store, &iter, 0, &device, -1);
			if (strcmp(name, device) == 0)
			{
				foundit = TRUE;
				break;
			}
		} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
	}
	if (foundit)
		gtk_combo_box_set_active_iter (combo, &iter);
	else
		gtk_combo_box_set_active (combo, 0);
}

void
dvd_device_changed_cb(GtkComboBox *combo, GtkWidget *dialog)
{
	gint ii = gtk_combo_box_get_active (combo);
	if (ii != 0)
	{
		const gchar *device = gtk_combo_box_get_active_text (combo);
		const gchar *name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(dialog));
		if (name == NULL || strcmp(name, device) != 0)
			gtk_file_chooser_select_filename (GTK_FILE_CHOOSER(dialog), device);
	}
}

void
source_type_changed_cb(GtkToggleButton *toggle, GtkFileChooser *chooser)
{
	gchar *folder;
	
	g_debug("source_type_changed_cb ()");
	folder = gtk_file_chooser_get_current_folder (chooser);
	if (gtk_toggle_button_get_active (toggle))
	{
		gtk_file_chooser_set_action (chooser, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
		gtk_widget_set_sensitive (dvd_device_combo, FALSE);
		gtk_combo_box_set_active (GTK_COMBO_BOX(dvd_device_combo), 0);
	}
	else
	{
		gtk_file_chooser_set_action (chooser, GTK_FILE_CHOOSER_ACTION_OPEN);
		gtk_widget_set_sensitive (dvd_device_combo, TRUE);
	}
	if (folder != NULL)
	{
		gtk_file_chooser_set_current_folder(chooser, folder);
		g_free(folder);
	}
}

static GtkWidget*
source_dialog_extra_widgets(GtkWidget *dialog, gboolean checkbutton_active)
{
	GtkBox *vbox;
	GtkWidget *checkbutton;
	
	vbox = GTK_BOX(gtk_vbox_new (FALSE, 2));
	checkbutton = gtk_check_button_new_with_label ("Open VIDEO_TS folder");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkbutton), checkbutton_active);
	gtk_box_pack_start (vbox, checkbutton, FALSE, FALSE, 1);
	gtk_widget_show(checkbutton);

	GtkWidget *combo;
	GtkBox *hbox;
	GList *drives, *link;
	GtkWidget *label, *blank;

	hbox = GTK_BOX(gtk_hbox_new (FALSE, 2));
	combo = gtk_combo_box_new_text();
	label = gtk_label_new("Detected DVD devices:");
	blank = gtk_label_new("");
	link = drives = dvd_device_list();
	gtk_combo_box_append_text (GTK_COMBO_BOX(combo), "Not Selected");
	while (link != NULL)
	{
		gchar *name = (gchar*)link->data;
		gtk_combo_box_append_text (GTK_COMBO_BOX(combo), name);
		g_free(name);
		link = link->next;
	}
	g_list_free(drives);
	gtk_combo_box_set_active (GTK_COMBO_BOX(combo), 0);
	gtk_box_pack_start (vbox, GTK_WIDGET(hbox), FALSE, FALSE, 1);
	gtk_widget_show(GTK_WIDGET(hbox));
	gtk_box_pack_start (hbox, label, FALSE, FALSE, 1);
	gtk_widget_show(label);
	gtk_box_pack_start (hbox, combo, FALSE, FALSE, 2);
	gtk_widget_show(combo);
	gtk_box_pack_start (hbox, blank, TRUE, TRUE, 1);
	gtk_widget_show(blank);
 
	// Ugly hackish global alert
	dvd_device_combo = combo;
	g_signal_connect(combo, "changed", (GCallback)dvd_device_changed_cb, dialog);
	g_signal_connect(dialog, "selection-changed", (GCallback)chooser_file_selected_cb, combo);

	g_signal_connect(checkbutton, "toggled", (GCallback)source_type_changed_cb, dialog);
	return GTK_WIDGET(vbox);
}

static void
do_scan(signal_user_data_t *ud, const gchar *filename)
{
	if (filename != NULL)
	{
		ghb_settings_set_string(ud->settings, "source", filename);
		if (update_source_label(ud, filename))
		{
			GtkProgressBar *progress;
			progress = GTK_PROGRESS_BAR(GHB_WIDGET(ud->builder, "progressbar"));
			gchar *path;
			path = ghb_settings_get_string( ud->settings, "source");
			gtk_progress_bar_set_fraction (progress, 0);
			gtk_progress_bar_set_text (progress, "Scanning ...");
			ghb_hb_cleanup(TRUE);
			ghb_backend_scan (path, 0);
			g_free(path);
		}
		else
		{
			// TODO: error dialog
		}
	}
}

void
source_button_clicked_cb(GtkButton *button, signal_user_data_t *ud)
{
	GtkWidget *dialog;
	GtkWidget *widget;
	gchar *sourcename;
	gint	response;
	GtkFileChooserAction action;
	gboolean checkbutton_active;

	g_debug("source_browse_clicked_cb ()");
	sourcename = ghb_settings_get_string(ud->settings, "source");
	checkbutton_active = FALSE;
	if (g_file_test(sourcename, G_FILE_TEST_IS_DIR))
	{
		action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
		checkbutton_active = TRUE;
	}
	else
	{
		action = GTK_FILE_CHOOSER_ACTION_OPEN;
	}
	dialog = gtk_file_chooser_dialog_new ("Select Source",
								NULL,
								action,
								GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
								GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
								NULL);
	widget = source_dialog_extra_widgets(dialog, checkbutton_active);
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER(dialog), widget);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), sourcename);
	response = gtk_dialog_run(GTK_DIALOG (dialog));
	gtk_widget_hide(dialog);
	if (response == GTK_RESPONSE_ACCEPT)
	{
		char *filename;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		if (filename != NULL)
		{
			do_scan(ud, filename);
			if (strcmp(sourcename, filename) != 0)
			{
				ghb_settings_set_string (ud->settings, "default_source", filename);
				ghb_pref_save (ud->settings, "default_source");
				ghb_dvd_set_current (filename, ud);
			}
			g_free(filename);
		}
	}
	g_free(sourcename);
	gtk_widget_destroy(dialog);
}

void
dvd_source_activate_cb(GtkAction *action, signal_user_data_t *ud)
{
	const gchar *filename;
	gchar *sourcename;

	sourcename = ghb_settings_get_string(ud->settings, "source");
	filename = gtk_action_get_name(action);
	do_scan(ud, filename);
	if (strcmp(sourcename, filename) != 0)
	{
		ghb_settings_set_string (ud->settings, "default_source", filename);
		ghb_pref_save (ud->settings, "default_source");
		ghb_dvd_set_current (filename, ud);
	}
	g_free(sourcename);
}

static void
update_destination_extension(signal_user_data_t *ud)
{
	static gchar *containers[] = {".mkv", ".mp4", ".m4v", ".avi", ".ogm", NULL};
	gchar *filename;
	gchar *extension;
	gint ii;
	GtkEntry *entry;

	g_debug("update_destination_extension ()");
	extension = ghb_settings_get_string(ud->settings, "container");
	entry = GTK_ENTRY(GHB_WIDGET(ud->builder, "destination"));
	filename = g_strdup(gtk_entry_get_text(entry));
	for (ii = 0; containers[ii] != NULL; ii++)
	{
		if (g_str_has_suffix(filename, containers[ii]))
		{
			gchar *pos;
			gchar *new_name;
			
			pos = g_strrstr( filename, "." );
			if (pos == NULL)
			{
				// No period? shouldn't happen
				break;
			}
			*pos = 0;
			if (strcmp(extension, &pos[1]) == 0)
			{
				// Extension is already correct
				break;
			}
			new_name = g_strjoin(".", filename, extension, NULL); 
			ghb_ui_update(ud, "destination", ghb_string_value(new_name));
			g_free(new_name);
			break;
		}
	}
	g_free(extension);
	g_free(filename);
}

static void
destination_select_title(GtkEntry *entry)
{
	const gchar *dest;
	gint start, end;

	dest = gtk_entry_get_text(entry);
	for (end = strlen(dest)-1; end > 0; end--)
	{
		if (dest[end] == '.')
		{
			break;
		}
	}
	for (start = end; start >= 0; start--)
	{
		if (dest[start] == '/')
		{
			start++;
			break;
		}
	}
	if (start < end)
	{
		gtk_editable_select_region(GTK_EDITABLE(entry), start, end);
	}
}

gboolean
destination_grab_cb(
	GtkEntry *entry, 
	signal_user_data_t *ud)
{
	destination_select_title(entry);
	return FALSE;
}

static gboolean update_default_destination = FALSE;

void
destination_entry_changed_cb(GtkEntry *entry, signal_user_data_t *ud)
{
	gchar *dest;
	
	g_debug("destination_entry_changed_cb ()");
	if ((dest = expand_tilde(gtk_entry_get_text(entry))) != NULL)
	{
		gtk_entry_set_text(entry, dest);
		g_free(dest);
	}
	update_destination_extension(ud);
	ghb_widget_to_setting(ud->settings, (GtkWidget*)entry);
	// This signal goes off with ever keystroke, so I'm putting this
	// update on the timer.
	update_default_destination = TRUE;
}

void
destination_browse_clicked_cb(GtkButton *button, signal_user_data_t *ud)
{
	GtkWidget *dialog;
	GtkEntry *entry;
	gchar *destname;
	gchar *basename;

	g_debug("destination_browse_clicked_cb ()");
	destname = ghb_settings_get_string(ud->settings, "destination");
	dialog = gtk_file_chooser_dialog_new ("Choose Destination",
					  NULL,
					  GTK_FILE_CHOOSER_ACTION_SAVE,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					  NULL);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), destname);
	basename = g_path_get_basename(destname);
	g_free(destname);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), basename);
	g_free(basename);
	if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		entry = (GtkEntry*)GHB_WIDGET(ud->builder, "destination");
		if (entry == NULL)
		{
			g_debug("Failed to find widget: %s", "destination");
		}
		else
		{
			gtk_entry_set_text(entry, filename);
		}
		g_free (filename);
	}
	gtk_widget_destroy(dialog);
}

gboolean
window_destroy_event_cb(GtkWidget *widget, GdkEvent *event, signal_user_data_t *ud)
{
	g_debug("window_destroy_event_cb ()");
	ghb_hb_cleanup(FALSE);
	gtk_main_quit();
	return FALSE;
}

gboolean
window_delete_event_cb(GtkWidget *widget, GdkEvent *event, signal_user_data_t *ud)
{
	gint state = ghb_get_queue_state();
	g_debug("window_delete_event_cb ()");
	if (state & GHB_STATE_WORKING)
	{
		if (cancel_encode("Closing HandBrake will terminate encoding.\n"))
		{
			ghb_hb_cleanup(FALSE);
			gtk_main_quit();
			return FALSE;
		}
		return TRUE;
	}
	ghb_hb_cleanup(FALSE);
	gtk_main_quit();
	return FALSE;
}

static void
update_acodec_combo(signal_user_data_t *ud)
{
	ghb_grey_combo_options (ud->builder);
}

void
container_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	const GValue *audio_list;
	g_debug("container_changed_cb ()");
	ghb_widget_to_setting(ud->settings, widget);
	update_destination_extension(ud);
	check_dependency(ud, widget);
	update_acodec_combo(ud);
	clear_presets_selection(ud);

	audio_list = ghb_settings_get_value(ud->settings, "audio_list");
	if (ghb_ac3_in_audio_list (audio_list))
	{
		gchar *container;

		container = ghb_settings_get_string(ud->settings, "container");
		if (strcmp(container, "mp4") == 0)
		{
			ghb_ui_update(ud, "container", ghb_string_value("m4v"));
		}
		g_free(container);
	}
}

static gchar*
get_aspect_string(gint aspect_n, gint aspect_d)
{
	gchar *aspect;

	if (aspect_d < 10)
	{
		aspect = g_strdup_printf("%d:%d", aspect_n, aspect_d);
	}
	else
	{
		gdouble aspect_nf = (gdouble)aspect_n / aspect_d;
		aspect = g_strdup_printf("%.2f:1", aspect_nf);
	}
	return aspect;
}

static gchar*
get_rate_string(gint rate_base, gint rate)
{
	gdouble rate_f = (gdouble)rate / rate_base;
	gchar *rate_s;

	rate_s = g_strdup_printf("%.6g", rate_f);
	return rate_s;
}
static void
show_title_info(signal_user_data_t *ud, ghb_title_info_t *tinfo)
{
	GtkWidget *widget;
	gchar *text;

	widget = GHB_WIDGET (ud->builder, "title_duration");
	if (tinfo->duration != 0)
	{
		text = g_strdup_printf ("%02d:%02d:%02d", tinfo->hours, 
				tinfo->minutes, tinfo->seconds);
	}
	else
	{
		text = g_strdup_printf ("Unknown");
	}
	gtk_label_set_text (GTK_LABEL(widget), text);
	g_free(text);
	widget = GHB_WIDGET (ud->builder, "source_dimensions");
	text = g_strdup_printf ("%d x %d", tinfo->width, tinfo->height);
	gtk_label_set_text (GTK_LABEL(widget), text);
	ghb_settings_set_int(ud->settings, "source_width", tinfo->width);
	ghb_settings_set_int(ud->settings, "source_height", tinfo->height);
	g_free(text);
	widget = GHB_WIDGET (ud->builder, "source_aspect");
	text = get_aspect_string(tinfo->aspect_n, tinfo->aspect_d);
	gtk_label_set_text (GTK_LABEL(widget), text);
	g_free(text);

	widget = GHB_WIDGET (ud->builder, "source_frame_rate");
	text = (gchar*)get_rate_string(tinfo->rate_base, tinfo->rate);
	gtk_label_set_text (GTK_LABEL(widget), text);
	g_free(text);

	ghb_ui_update(ud, "scale_width", 
		ghb_int64_value(tinfo->width - tinfo->crop[2] - tinfo->crop[3]));
	// If anamorphic or keep_aspect, the hight will be automatically calculated
	gboolean keep_aspect, anamorphic;
	keep_aspect = ghb_settings_get_boolean(ud->settings, "keep_aspect");
	anamorphic = ghb_settings_get_boolean(ud->settings, "anamorphic");
	if (!(keep_aspect || anamorphic))
	{
		ghb_ui_update(ud, "scale_height", 
			ghb_int64_value(tinfo->height - tinfo->crop[0] - tinfo->crop[1]));
	}

	// Set the limits of cropping.  hb_set_anamorphic_size crashes if
	// you pass it a cropped width or height == 0.
	gint bound;
	bound = tinfo->height / 2 - 2;
	widget = GHB_WIDGET (ud->builder, "crop_top");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	widget = GHB_WIDGET (ud->builder, "crop_bottom");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	bound = tinfo->width / 2 - 2;
	widget = GHB_WIDGET (ud->builder, "crop_left");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	widget = GHB_WIDGET (ud->builder, "crop_right");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	if (ghb_settings_get_boolean(ud->settings, "autocrop"))
	{
		ghb_ui_update(ud, "crop_top", ghb_int64_value(tinfo->crop[0]));
		ghb_ui_update(ud, "crop_bottom", ghb_int64_value(tinfo->crop[1]));
		ghb_ui_update(ud, "crop_left", ghb_int64_value(tinfo->crop[2]));
		ghb_ui_update(ud, "crop_right", ghb_int64_value(tinfo->crop[3]));
	}
	g_debug("setting max end chapter %d", tinfo->num_chapters);
	widget = GHB_WIDGET (ud->builder, "end_chapter");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 1, tinfo->num_chapters);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(widget), tinfo->num_chapters);
	widget = GHB_WIDGET (ud->builder, "start_chapter");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(widget), 1);
}

static void
adjust_audio_rate_combos(signal_user_data_t *ud)
{
	gint titleindex, audioindex, acodec;
	ghb_audio_info_t ainfo;
	GtkWidget *widget;
	
	g_debug("adjust_audio_rate_combos ()");
	titleindex = ghb_settings_combo_int(ud->settings, "title");

	widget = GHB_WIDGET(ud->builder, "audio_track");
	audioindex = ghb_lookup_combo_int("audio_track", ghb_widget_value(widget));

	widget = GHB_WIDGET(ud->builder, "audio_codec");
	acodec = ghb_lookup_combo_int("audio_codec", ghb_widget_value(widget));

	if (ghb_audio_is_passthru (acodec))
	{
		ghb_set_default_bitrate_opts (ud->builder, -1);
		if (ghb_get_audio_info (&ainfo, titleindex, audioindex))
		{
			gint br = ainfo.bitrate / 1000;
			// Set the values for bitrate and samplerate to the input rates
			ghb_set_passthru_bitrate_opts (ud->builder, br);
			ghb_ui_update(ud, "audio_bitrate", ghb_int64_value(br));
			ghb_ui_update(ud, "audio_rate", ghb_int64_value(0));
			ghb_ui_update(ud, "audio_mix", ghb_int64_value(0));
		}
		else
		{
			ghb_ui_update(ud, "audio_bitrate", ghb_int64_value(384));
			ghb_ui_update(ud, "audio_rate", ghb_int64_value(0));
			ghb_ui_update(ud, "audio_mix", ghb_int64_value(0));
		}
	}
	else if (acodec == HB_ACODEC_FAAC)
	{
		gint br;

		widget = GHB_WIDGET(ud->builder, "audio_bitrate");
		br = ghb_lookup_combo_int("audio_bitrate", ghb_widget_value(widget));
		if (br > 160)
			ghb_ui_update(ud, "audio_bitrate", ghb_int64_value(160));
		ghb_set_default_bitrate_opts (ud->builder, 160);
	}
	else
	{
		ghb_set_default_bitrate_opts (ud->builder, -1);
	}
}

static void
set_pref_audio(gint titleindex, signal_user_data_t *ud)
{
	gint acodec_code, mix_code, track;
	gchar *source_lang;
	GtkWidget *button;
	ghb_audio_info_t ainfo;
	gint index;
	GHashTable *track_indicies;
	gint *iptr;

	GValue *pref_audio;
	GValue *audio, *acodec, *bitrate, *rate, *mix, *drc;
	gint count, ii, list_count;
	
	g_debug("set_pref_audio");
	track_indicies = g_hash_table_new(g_int_hash, g_int_equal);
	// Clear the audio list
	clear_audio_list(ud);
	// Find "best" audio based on audio preferences
	button = GHB_WIDGET (ud->builder, "audio_add");
	source_lang = ghb_settings_get_string(ud->settings, "source_audio_lang");

	pref_audio = ghb_settings_get_value(ud->settings, "pref_audio_list");

	list_count = 0;
	count = ghb_array_len(pref_audio);
	for (ii = 0; ii < count; ii++)
	{
		audio = ghb_array_get_nth(pref_audio, ii);
		acodec = ghb_settings_get_value(audio, "audio_codec");
		bitrate = ghb_settings_get_value(audio, "audio_bitrate");
		rate = ghb_settings_get_value(audio, "audio_rate");
		mix = ghb_settings_get_value(audio, "audio_mix");
		drc = ghb_settings_get_value(audio, "audio_drc");
		acodec_code = ghb_lookup_combo_int("audio_codec", acodec);
		// If there are multiple audios using the same codec, then
		// select sequential tracks for each.  This hash keeps track 
		// of the last used track for each codec.
		iptr = g_hash_table_lookup(track_indicies, &acodec_code);
		if (iptr == NULL)
			index = 0;
		else
			index = *(gint*)iptr;

		track = ghb_find_audio_track(titleindex, source_lang, index);
		// Check to see if:
		// 1. pref codec is ac3
		// 2. source codec is not ac3
		// 3. next pref is enabled
		if (ghb_get_audio_info (&ainfo, titleindex, track) && 
			ghb_audio_is_passthru (acodec_code))
		{
			if (!ghb_audio_is_passthru(ainfo.codec))
			{
				acodec_code = ghb_get_default_acodec();
				// If there's more audio to process, or we've already
				// placed one in the list, then we can skip this one
				if ((ii + 1 < count) || (list_count != 0))
				{
					// Skip this audio
					acodec_code = 0;
				}
			}
		}
		if (titleindex >= 0 && track < 0)
			acodec_code = 0;
		if (acodec_code != 0)
		{
			// Add to audio list
			g_signal_emit_by_name(button, "clicked", ud);
			list_count++;
			ghb_ui_update(ud, "audio_track", ghb_int64_value(track));
			ghb_ui_update(ud, "audio_codec", acodec);
			if (!ghb_audio_is_passthru (acodec_code))
			{
				// This gets set autimatically if the codec is passthru
				ghb_ui_update(ud, "audio_bitrate", bitrate);
				ghb_ui_update(ud, "audio_rate", rate);
				mix_code = ghb_lookup_combo_int("audio_mix", mix);
				mix_code = ghb_get_best_mix(
					titleindex, track, acodec_code, mix_code);
				ghb_ui_update(ud, "audio_mix", ghb_int64_value(mix_code));
			}
			ghb_ui_update(ud, "audio_drc", drc);
			index++;
			g_hash_table_insert(track_indicies, &acodec_code, &index);
		}
	}
	g_free(source_lang);
	g_hash_table_destroy(track_indicies);
}

static gint preview_button_width;
static gint preview_button_height;
static gboolean update_preview = FALSE;

static void
set_preview_image(signal_user_data_t *ud)
{
	GtkWidget *widget;
	gint preview_width, preview_height, target_height, width, height;

	g_debug("set_preview_button_image ()");
	gint titleindex;

	titleindex = ghb_settings_combo_int(ud->settings, "title");
	if (titleindex < 0) return;
	widget = GHB_WIDGET (ud->builder, "preview_frame");
	gint frame = ghb_widget_int(widget) - 1;
	GdkPixbuf *preview = ghb_get_preview_image (titleindex, frame, ud->settings, TRUE);
	if (preview == NULL) return;
	widget = GHB_WIDGET (ud->builder, "preview_image");
	gtk_image_set_from_pixbuf(GTK_IMAGE(widget), preview);

	preview_width = gdk_pixbuf_get_width(preview);
	preview_height = gdk_pixbuf_get_height(preview);
	gchar *text = g_strdup_printf("%d x %d", preview_width, preview_height);
	widget = GHB_WIDGET (ud->builder, "preview_dims");
	gtk_label_set_text(GTK_LABEL(widget), text);
	g_free(text);
	
	g_debug("preview %d x %d", preview_width, preview_height);
	target_height = MIN(preview_button_height - 12, 128);
	height = target_height;
	width = preview_width * height / preview_height;

	if ((height >= 16) && (width >= 16))
	{
		GdkPixbuf *scaled_preview;
		scaled_preview = gdk_pixbuf_scale_simple (preview, width, height, GDK_INTERP_NEAREST);
		if (scaled_preview != NULL)
		{
			g_object_unref (preview);
			
			widget = GHB_WIDGET (ud->builder, "preview_button_image");
			gtk_image_set_from_pixbuf(GTK_IMAGE(widget), scaled_preview);
			g_object_unref (scaled_preview);
		}
	}
}

void
title_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	ghb_title_info_t tinfo;
	gint titleindex;
	gchar *preset;
	
	g_debug("title_changed_cb ()");
	ghb_widget_to_setting(ud->settings, widget);
	check_dependency(ud, widget);

	titleindex = ghb_settings_combo_int(ud->settings, "title");
	ghb_update_ui_combo_box (ud->builder, "audio_track", titleindex, FALSE);
	ghb_update_ui_combo_box (ud->builder, "subtitle_lang", titleindex, FALSE);

	preset = ghb_settings_get_string (ud->settings, "preset");
	ghb_update_from_preset(ud, preset, "subtitle_lang");
	g_free(preset);
	if (ghb_get_title_info (&tinfo, titleindex))
	{
		show_title_info(ud, &tinfo);
	}
	update_chapter_list (ud);
	adjust_audio_rate_combos(ud);
	set_pref_audio(titleindex, ud);
	if (ghb_settings_get_boolean(ud->settings, "vquality_type_target"))
	{
		gint bitrate = ghb_calculate_target_bitrate (ud->settings, titleindex);
		ghb_ui_update(ud, "video_bitrate", ghb_int64_value(bitrate));
	}

	// Unfortunately, there is no way to query how many frames were
	// actually generated during the scan.  It attempts to make 10.
	// If I knew how many were generated, I would adjust the spin
	// control range here.
	ghb_ui_update(ud, "preview_frame", ghb_int64_value(1));

	set_preview_image (ud);
}

void
audio_codec_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	static gint prev_acodec = 0;
	gint acodec_code, mix_code;
	GValue *asettings;
	
	g_debug("audio_codec_changed_cb ()");
	acodec_code = ghb_lookup_combo_int("audio_codec", ghb_widget_value(widget));
	if (ghb_audio_is_passthru (prev_acodec) && 
		!ghb_audio_is_passthru (acodec_code))
	{
		// Transition from passthru to not, put some audio settings back to 
		// pref settings
		gint titleindex;
		gint track;

		titleindex = ghb_settings_combo_int(ud->settings, "title");
		track = ghb_settings_combo_int(ud->settings, "audio_track");

		ghb_ui_update(ud, "audio_bitrate", ghb_string_value("160"));
		ghb_ui_update(ud, "audio_rate", ghb_string_value("source"));
		mix_code = ghb_lookup_combo_int("audio_mix", ghb_string_value("dpl2"));
		mix_code = ghb_get_best_mix( titleindex, track, acodec_code, mix_code);
		ghb_ui_update(ud, "audio_mix", ghb_int64_value(mix_code));
		ghb_ui_update(ud, "audio_drc", ghb_double_value(1.0));
	}
	adjust_audio_rate_combos(ud);
	ghb_grey_combo_options (ud->builder);
	check_dependency(ud, widget);
	prev_acodec = acodec_code;
	asettings = get_selected_asettings(ud);
	if (asettings != NULL)
	{
		ghb_widget_to_setting(asettings, widget);
		audio_list_refresh_selected(ud);
	}

	const GValue *audio_list;
	audio_list = ghb_settings_get_value(ud->settings, "audio_list");
	if (ghb_ac3_in_audio_list (audio_list))
	{
		gchar *container;

		container = ghb_settings_get_string(ud->settings, "container");
		if (strcmp(container, "mp4") == 0)
		{
			ghb_ui_update(ud, "container", ghb_string_value("m4v"));
		}
		g_free(container);
	}
}

static void audio_list_refresh_selected(signal_user_data_t *ud);

void
audio_track_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	GValue *asettings;

	g_debug("audio_track_changed_cb ()");
	adjust_audio_rate_combos(ud);
	check_dependency(ud, widget);
	ghb_grey_combo_options(ud->builder);
	asettings = get_selected_asettings(ud);
	if (asettings != NULL)
	{
		const gchar *track;

		ghb_widget_to_setting(asettings, widget);
		audio_list_refresh_selected(ud);
		track = ghb_settings_combo_option(asettings, "audio_track");
		ghb_settings_set_string(asettings, "audio_track_long", track);
	}
}

void
audio_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	GValue *asettings;

	g_debug("audio_widget_changed_cb ()");
	check_dependency(ud, widget);
	asettings = get_selected_asettings(ud);
	if (asettings != NULL)
	{
		ghb_widget_to_setting(asettings, widget);
		audio_list_refresh_selected(ud);
	}
}

void
generic_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("generic_widget_changed_cb ()");
	check_dependency(ud, widget);
}

void
setting_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	ghb_widget_to_setting(ud->settings, widget);
	check_dependency(ud, widget);
	clear_presets_selection(ud);
}

static void
validate_filter_widget(signal_user_data_t *ud, const gchar *name)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	const gchar *str;
	gboolean foundit = FALSE;
	GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
	if (gtk_combo_box_get_active(combo) < 0)
	{ // Validate user input
		gchar *val = ghb_settings_get_string(ud->settings, name);
		store = gtk_combo_box_get_model(combo);
		// Check to see if user manually entered one of the combo options
		if (gtk_tree_model_get_iter_first(store, &iter))
		{
			do
			{
				gtk_tree_model_get(store, &iter, 0, &str, -1);
				if (strcasecmp(val, str) == 0)
				{
					gtk_combo_box_set_active_iter(combo, &iter);
					foundit = TRUE;
					break;
				}
			} while (gtk_tree_model_iter_next(store, &iter));
		}
		if (!foundit)
		{ // validate format of filter string
			if (!ghb_validate_filter_string(val, -1))
				gtk_combo_box_set_active(combo, 0);
		}
		g_free(val);
	}
}

gboolean
deint_tweak_focus_out_cb(GtkWidget *widget, GdkEventFocus *event, 
	signal_user_data_t *ud)
{
	g_debug("deint_tweak_focus_out_cb ()");
	validate_filter_widget(ud, "tweak_deinterlace");
	return FALSE;
}

gboolean
denoise_tweak_focus_out_cb(GtkWidget *widget, GdkEventFocus *event, 
	signal_user_data_t *ud)
{
	g_debug("denoise_tweak_focus_out_cb ()");
	validate_filter_widget(ud, "tweak_noise");
	return FALSE;
}

void
http_opt_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	ghb_widget_to_setting(ud->settings, widget);
	check_dependency(ud, widget);
	clear_presets_selection(ud);
	ghb_grey_combo_options (ud->builder);
}

void
vcodec_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	gint vqmin, vqmax;

	ghb_widget_to_setting(ud->settings, widget);
	check_dependency(ud, widget);
	clear_presets_selection(ud);
	ghb_vquality_range(ud, &vqmin, &vqmax);
	GtkWidget *qp = GHB_WIDGET(ud->builder, "video_quality");
	gtk_range_set_range (GTK_RANGE(qp), vqmin, vqmax);
}

void
vfr_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	//const gchar *name = gtk_widget_get_name(widget);
	//g_debug("setting_widget_changed_cb () %s", name);
	ghb_widget_to_setting(ud->settings, widget);
	check_dependency(ud, widget);
	clear_presets_selection(ud);
	if (ghb_settings_get_boolean(ud->settings, "variable_frame_rate"))
	{
		ghb_ui_update(ud, "framerate", ghb_int64_value(0));
	}
}

// subtitles have their differ from other settings in that
// the selection is updated automaitcally when the title
// changes.  I don't want the preset selection changed as
// would happen for regular settings.
void
subtitle_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	const gchar *name = gtk_widget_get_name(widget);
	g_debug("subtitle_changed_cb () %s", name);
	ghb_widget_to_setting(ud->settings, widget);
	check_dependency(ud, widget);
}

void
target_size_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	const gchar *name = gtk_widget_get_name(widget);
	g_debug("setting_widget_changed_cb () %s", name);
	ghb_widget_to_setting(ud->settings, widget);
	check_dependency(ud, widget);
	clear_presets_selection(ud);
	if (ghb_settings_get_boolean(ud->settings, "vquality_type_target"))
	{
		gint titleindex;
		titleindex = ghb_settings_combo_int(ud->settings, "title");
		gint bitrate = ghb_calculate_target_bitrate (ud->settings, titleindex);
		ghb_ui_update(ud, "video_bitrate", ghb_int64_value(bitrate));
	}
}

void
start_chapter_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	gint start, end;
	const gchar *name = gtk_widget_get_name(widget);

	g_debug("start_chapter_changed_cb () %s", name);
	ghb_widget_to_setting(ud->settings, widget);
	start = ghb_settings_get_int(ud->settings, "start_chapter");
	end = ghb_settings_get_int(ud->settings, "end_chapter");
	if (start > end)
		ghb_ui_update(ud, "end_chapter", ghb_int_value(start));
	check_dependency(ud, widget);
	if (ghb_settings_get_boolean(ud->settings, "chapters_in_destination"))
	{
		set_destination(ud);
	}
}

void
end_chapter_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	gint start, end;
	const gchar *name = gtk_widget_get_name(widget);

	g_debug("end_chapter_changed_cb () %s", name);
	ghb_widget_to_setting(ud->settings, widget);
	start = ghb_settings_get_int(ud->settings, "start_chapter");
	end = ghb_settings_get_int(ud->settings, "end_chapter");
	if (start > end)
		ghb_ui_update(ud, "start_chapter", ghb_int_value(end));
	check_dependency(ud, widget);
	if (ghb_settings_get_boolean(ud->settings, "chapters_in_destination"))
	{
		set_destination(ud);
	}
}

void
scale_width_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("scale_width_changed_cb ()");
	ghb_widget_to_setting(ud->settings, widget);
	check_dependency(ud, widget);
	ghb_set_scale (ud, GHB_SCALE_KEEP_WIDTH);
	update_preview = TRUE;
	gchar *text;
	gint width = ghb_settings_get_int(ud->settings, "scale_width");
	gint height = ghb_settings_get_int(ud->settings, "scale_height");
	widget = GHB_WIDGET (ud->builder, "scale_dimensions");
	text = g_strdup_printf ("%d x %d", width, height);
	gtk_label_set_text (GTK_LABEL(widget), text);
	g_free(text);
}

void
scale_height_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("scale_height_changed_cb ()");
	ghb_widget_to_setting(ud->settings, widget);
	check_dependency(ud, widget);
	ghb_set_scale (ud, GHB_SCALE_KEEP_HEIGHT);
	update_preview = TRUE;
	gchar *text;
	gint width = ghb_settings_get_int(ud->settings, "scale_width");
	gint height = ghb_settings_get_int(ud->settings, "scale_height");
	widget = GHB_WIDGET (ud->builder, "scale_dimensions");
	text = g_strdup_printf ("%d x %d", width, height);
	gtk_label_set_text (GTK_LABEL(widget), text);
	g_free(text);
}

void
crop_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	gint titleindex, crop[4];
	ghb_title_info_t tinfo;
	
	g_debug("crop_changed_cb ()");
	ghb_widget_to_setting(ud->settings, widget);
	check_dependency(ud, widget);
	ghb_set_scale (ud, GHB_SCALE_KEEP_NONE);

	crop[0] = ghb_settings_get_int(ud->settings, "crop_top");
	crop[1] = ghb_settings_get_int(ud->settings, "crop_bottom");
	crop[2] = ghb_settings_get_int(ud->settings, "crop_left");
	crop[3] = ghb_settings_get_int(ud->settings, "crop_right");
	titleindex = ghb_settings_combo_int(ud->settings, "title");
	if (ghb_get_title_info (&tinfo, titleindex))
	{
		gint width, height;
		gchar *text;
		
		width = tinfo.width - crop[2] - crop[3];
		height = tinfo.height - crop[0] - crop[1];
		widget = GHB_WIDGET (ud->builder, "crop_dimensions");
		text = g_strdup_printf ("%d x %d", width, height);
		gtk_label_set_text (GTK_LABEL(widget), text);
		g_free(text);
	}
	gchar *text;
	widget = GHB_WIDGET (ud->builder, "crop_values");
	text = g_strdup_printf ("%d:%d:%d:%d", crop[0], crop[1], crop[2], crop[3]);
	gtk_label_set_text (GTK_LABEL(widget), text);
	g_free(text);
	update_preview = TRUE;
}

void
scale_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("scale_changed_cb ()");
	ghb_widget_to_setting(ud->settings, widget);
	check_dependency(ud, widget);
	clear_presets_selection(ud);
	ghb_set_scale (ud, GHB_SCALE_KEEP_NONE);
	update_preview = TRUE;
	
	gchar *text;
	
	text = ghb_settings_get_boolean(ud->settings, "autocrop") ? "On" : "Off";
	widget = GHB_WIDGET (ud->builder, "crop_auto");
	gtk_label_set_text (GTK_LABEL(widget), text);
	text = ghb_settings_get_boolean(ud->settings, "autoscale") ? "On" : "Off";
	widget = GHB_WIDGET (ud->builder, "scale_auto");
	gtk_label_set_text (GTK_LABEL(widget), text);
	text = ghb_settings_get_boolean(ud->settings, "anamorphic") ? "On" : "Off";
	widget = GHB_WIDGET (ud->builder, "scale_anamorphic");
	gtk_label_set_text (GTK_LABEL(widget), text);
}

void
generic_entry_changed_cb(GtkEntry *entry, signal_user_data_t *ud)
{
	// Normally (due to user input) I only want to process the entry
	// when editing is done and the focus-out signal is sent.
	// But... there's always a but.
	// If the entry is changed by software, the focus-out signal is not sent.
	// The changed signal is sent ... so here we are.
	// I don't want to process upon every keystroke, so I prevent processing
	// while the widget has focus.
	g_debug("generic_entry_changed_cb ()");
	if (!GTK_WIDGET_HAS_FOCUS((GtkWidget*)entry))
	{
		ghb_widget_to_setting(ud->settings, (GtkWidget*)entry);
	}
}

gboolean
generic_focus_out_cb(GtkWidget *widget, GdkEventFocus *event, 
	signal_user_data_t *ud)
{
	g_debug("generic_focus_out_cb ()");
	ghb_widget_to_setting(ud->settings, widget);
	return FALSE;
}

// Flag needed to prevent x264 options processing from chasing its tail
static gboolean ignore_options_update = FALSE;

void
x264_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	ghb_widget_to_setting(ud->settings, widget);
	if (!ignore_options_update)
	{
		ignore_options_update = TRUE;
		ghb_x264_opt_update(ud, widget);
		ignore_options_update = FALSE;
	}
	check_dependency(ud, widget);
	clear_presets_selection(ud);
}

void
x264_entry_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("x264_entry_changed_cb ()");
	if (!ignore_options_update)
	{
		GtkWidget *textview;
		gchar *options;

		textview = GTK_WIDGET(GHB_WIDGET(ud->builder, "x264_options"));
		ghb_widget_to_setting(ud->settings, textview);
		options = ghb_settings_get_string(ud->settings, "x264_options");
		ignore_options_update = TRUE;
		ghb_x264_parse_options(ud, options);
		if (!GTK_WIDGET_HAS_FOCUS(textview))
		{
			gchar *sopts;

			sopts = ghb_sanitize_x264opts(ud, options);
			ghb_ui_update(ud, "x264_options", ghb_string_value(sopts));
			ghb_x264_parse_options(ud, sopts);
			g_free(sopts);
		}
		g_free(options);
		ignore_options_update = FALSE;
	}
}

gboolean
x264_focus_out_cb(GtkWidget *widget, GdkEventFocus *event, 
	signal_user_data_t *ud)
{
	gchar *options, *sopts;

	ghb_widget_to_setting(ud->settings, widget);
	options = ghb_settings_get_string(ud->settings, "x264_options");
	sopts = ghb_sanitize_x264opts(ud, options);
	ignore_options_update = TRUE;
	if (sopts != NULL)
	{
		ghb_ui_update(ud, "x264_options", ghb_string_value(sopts));
		ghb_x264_parse_options(ud, sopts);
	}
	g_free(options);
	g_free(sopts);
	ignore_options_update = FALSE;
	return FALSE;
}

static void
clear_audio_list(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkListStore *store;
	GValue *audio_list;
	
	g_debug("clear_audio_list ()");
	audio_list = ghb_settings_get_value(ud->settings, "audio_list");
	if (audio_list == NULL)
	{
		audio_list = ghb_array_value_new(8);
		ghb_settings_set_value(ud->settings, "audio_list", audio_list);
	}
	else
		ghb_array_value_reset(audio_list, 8);
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	gtk_list_store_clear (store);
}

static void
add_to_audio_list(signal_user_data_t *ud, GValue *settings)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	GtkTreeSelection *selection;
	const gchar *track, *codec, *br, *sr, *mix;
	gchar *drc, *s_track, *s_codec, *s_br, *s_sr, *s_mix;
	gdouble s_drc;
	
	g_debug("add_to_audio_list ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	selection = gtk_tree_view_get_selection (treeview);
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

	track = ghb_settings_combo_option(settings, "audio_track");
	codec = ghb_settings_combo_option(settings, "audio_codec");
	br = ghb_settings_combo_option(settings, "audio_bitrate");
	sr = ghb_settings_combo_option(settings, "audio_rate");
	mix = ghb_settings_combo_option(settings, "audio_mix");
	drc = ghb_settings_get_string(settings, "audio_drc");

	s_track = ghb_settings_get_string(settings, "audio_track");
	s_codec = ghb_settings_get_string(settings, "audio_codec");
	s_br = ghb_settings_get_string(settings, "audio_bitrate");
	s_sr = ghb_settings_get_string(settings, "audio_rate");
	s_mix = ghb_settings_get_string(settings, "audio_mix");
	s_drc = ghb_settings_get_double(settings, "audio_drc");

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
		// These are displayed in list
		0, track,
		1, codec,
		2, br,
		3, sr,
		4, mix,
		// These are used to set combo box values when a list item is selected
		5, drc,
		6, s_track,
		7, s_codec,
		8, s_br,
		9, s_sr,
		10, s_mix,
		11, s_drc,
		-1);
	gtk_tree_selection_select_iter(selection, &iter);
	g_free(drc);
	g_free(s_track);
	g_free(s_codec);
	g_free(s_br);
	g_free(s_sr);
	g_free(s_mix);
}

static void
audio_list_refresh_selected(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gint *indices;
	gint row;
	GValue *asettings = NULL;
	const GValue *audio_list;
	
	g_debug("audio_list_refresh_selected ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		const gchar *track, *codec, *br, *sr, *mix;
		gchar *drc, *s_track, *s_codec, *s_br, *s_sr, *s_mix;
		gdouble s_drc;
		// Get the row number
		treepath = gtk_tree_model_get_path (store, &iter);
		indices = gtk_tree_path_get_indices (treepath);
		row = indices[0];
		gtk_tree_path_free(treepath);
		// find audio settings
		if (row < 0) return;
		audio_list = ghb_settings_get_value(ud->settings, "audio_list");
		if (row >= ghb_array_len(audio_list))
			return;
		asettings = ghb_array_get_nth(audio_list, row);

		track = ghb_settings_combo_option(asettings, "audio_track");
		codec = ghb_settings_combo_option(asettings, "audio_codec");
		br = ghb_settings_combo_option(asettings, "audio_bitrate");
		sr = ghb_settings_combo_option(asettings, "audio_rate");
		mix = ghb_settings_combo_option(asettings, "audio_mix");
		drc = ghb_settings_get_string(asettings, "audio_drc");

		s_track = ghb_settings_get_string(asettings, "audio_track");
		s_codec = ghb_settings_get_string(asettings, "audio_codec");
		s_br = ghb_settings_get_string(asettings, "audio_bitrate");
		s_sr = ghb_settings_get_string(asettings, "audio_rate");
		s_mix = ghb_settings_get_string(asettings, "audio_mix");
		s_drc = ghb_settings_get_double(asettings, "audio_drc");

		gtk_list_store_set(GTK_LIST_STORE(store), &iter, 
			// These are displayed in list
			0, track,
			1, codec,
			2, br,
			3, sr,
			4, mix,
			// These are used to set combo values when a list item is selected
			5, drc,
			6, s_track,
			7, s_codec,
			8, s_br,
			9, s_sr,
			10, s_mix,
			11, s_drc,
			-1);
		g_free(drc);
		g_free(s_track);
		g_free(s_codec);
		g_free(s_br);
		g_free(s_sr);
		g_free(s_mix);
	}
}

static GValue*
get_selected_asettings(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gint *indices;
	gint row;
	GValue *asettings = NULL;
	const GValue *audio_list;
	
	g_debug("get_selected_asettings ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		// Get the row number
		treepath = gtk_tree_model_get_path (store, &iter);
		indices = gtk_tree_path_get_indices (treepath);
		row = indices[0];
		gtk_tree_path_free(treepath);
		// find audio settings
		if (row < 0) return NULL;
		audio_list = ghb_settings_get_value(ud->settings, "audio_list");
		if (row >= ghb_array_len(audio_list))
			return NULL;
		asettings = ghb_array_get_nth(audio_list, row);
	}
	return asettings;
}

void
audio_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkWidget *widget;
	
	g_debug("audio_list_selection_changed_cb ()");
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		const gchar *track, *codec, *bitrate, *sample_rate, *mix;
		gdouble drc;

		gtk_tree_model_get(store, &iter,
						   6, &track,
						   7, &codec,
						   8, &bitrate,
						   9, &sample_rate,
						   10, &mix,
						   11, &drc,
						   -1);
		ghb_ui_update(ud, "audio_track", ghb_string_value(track));
		ghb_ui_update(ud, "audio_codec", ghb_string_value(codec));
		ghb_ui_update(ud, "audio_bitrate", ghb_string_value(bitrate));
		ghb_ui_update(ud, "audio_rate", ghb_string_value(sample_rate));
		ghb_ui_update(ud, "audio_mix", ghb_string_value(mix));
		ghb_ui_update(ud, "audio_drc", ghb_double_value(drc));
		widget = GHB_WIDGET (ud->builder, "audio_remove");
		gtk_widget_set_sensitive(widget, TRUE);
		//widget = GHB_WIDGET (ud->builder, "audio_update");
		//gtk_widget_set_sensitive(widget, TRUE);
	}
	else
	{
		widget = GHB_WIDGET (ud->builder, "audio_remove");
		gtk_widget_set_sensitive(widget, FALSE);
		//widget = GHB_WIDGET (ud->builder, "audio_update");
		//gtk_widget_set_sensitive(widget, FALSE);
	}
}

void
audio_add_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	// Add the current audio settings to the list.
	GValue *asettings;
	GtkWidget *widget;
	gint count;
	GValue *audio_list;
	const gchar *track;
	
	g_debug("audio_add_clicked_cb ()");
	asettings = ghb_dict_value_new();
	// Only allow up to 8 audio entries
	widget = GHB_WIDGET(ud->builder, "audio_track");
	ghb_settings_take_value(asettings, "audio_track", ghb_widget_value(widget));
	widget = GHB_WIDGET(ud->builder, "audio_codec");
	ghb_settings_take_value(asettings, "audio_codec", ghb_widget_value(widget));
	widget = GHB_WIDGET(ud->builder, "audio_bitrate");
	ghb_settings_take_value(asettings, "audio_bitrate", ghb_widget_value(widget));
	widget = GHB_WIDGET(ud->builder, "audio_rate");
	ghb_settings_take_value(asettings, "audio_rate", ghb_widget_value(widget));
	widget = GHB_WIDGET(ud->builder, "audio_mix");
	ghb_settings_take_value(asettings, "audio_mix", ghb_widget_value(widget));
	widget = GHB_WIDGET(ud->builder, "audio_drc");
	ghb_settings_take_value(asettings, "audio_drc", ghb_widget_value(widget));
	track = ghb_settings_combo_option(asettings, "audio_track");
	ghb_settings_set_string(asettings, "audio_track_long", track);

	audio_list = ghb_settings_get_value(ud->settings, "audio_list");
	if (audio_list == NULL)
	{
		audio_list = ghb_array_value_new(8);
		ghb_settings_set_value(ud->settings, "audio_list", audio_list);
	}
	ghb_array_append(audio_list, asettings);
	add_to_audio_list(ud, asettings);
	count = ghb_array_len(audio_list);
	if (count >= 8)
	{
		gtk_widget_set_sensitive(xwidget, FALSE);
	}
}

void
audio_remove_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter, nextIter;
	gint *indices;
	gint row;
	GValue *audio_list;

	g_debug("audio_remove_clicked_cb ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		nextIter = iter;
		if (!gtk_tree_model_iter_next(store, &nextIter))
		{
			nextIter = iter;
			if (gtk_tree_model_get_iter_first(store, &nextIter))
			{
				gtk_tree_selection_select_iter (selection, &nextIter);
			}
		}
		else
		{
			gtk_tree_selection_select_iter (selection, &nextIter);
		}
		// Get the row number
		treepath = gtk_tree_model_get_path (store, &iter);
		indices = gtk_tree_path_get_indices (treepath);
		row = indices[0];
		gtk_tree_path_free(treepath);
		// Remove the selected item
		gtk_list_store_remove (GTK_LIST_STORE(store), &iter);
		// remove from audio settings list
		if (row < 0) return;
		widget = GHB_WIDGET (ud->builder, "audio_add");
		gtk_widget_set_sensitive(widget, TRUE);
		audio_list = ghb_settings_get_value(ud->settings, "audio_list");
		if (row >= ghb_array_len(audio_list))
			return;
		GValue *old = ghb_array_get_nth(audio_list, row);
		ghb_value_free(old);
		ghb_array_remove(audio_list, row);
	}
}

static void
audio_list_refresh(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean done;
	gint row = 0;
	GValue *audio_list;

	g_debug("audio_list_refresh ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
	{
		do
		{
			const gchar *track, *codec, *br, *sr, *mix;
			gchar *drc, *s_track, *s_codec, *s_br, *s_sr, *s_mix;
			gdouble s_drc;
			GValue *asettings;

			audio_list = ghb_settings_get_value(ud->settings, "audio_list");
			if (row >= ghb_array_len(audio_list))
				return;
			asettings = ghb_array_get_nth(audio_list, row);

			track = ghb_settings_combo_option(asettings, "audio_track");
			codec = ghb_settings_combo_option(asettings, "audio_codec");
			br = ghb_settings_combo_option(asettings, "audio_bitrate");
			sr = ghb_settings_combo_option(asettings, "audio_rate");
			mix = ghb_settings_combo_option(asettings, "audio_mix");
			drc = ghb_settings_get_string(asettings, "audio_drc");

			s_track = ghb_settings_get_string(asettings, "audio_track");
			s_codec = ghb_settings_get_string(asettings, "audio_codec");
			s_br = ghb_settings_get_string(asettings, "audio_bitrate");
			s_sr = ghb_settings_get_string(asettings, "audio_rate");
			s_mix = ghb_settings_get_string(asettings, "audio_mix");
			s_drc = ghb_settings_get_double(asettings, "audio_drc");

			gtk_list_store_set(GTK_LIST_STORE(store), &iter, 
				// These are displayed in list
				0, track,
				1, codec,
				2, br,
				3, sr,
				4, mix,
				// These are used to set combo values when an item is selected
				5, drc,
				6, s_track,
				7, s_codec,
				8, s_br,
				9, s_sr,
				10, s_mix,
				11, s_drc,
				-1);
			g_free(drc);
			g_free(s_track);
			g_free(s_codec);
			g_free(s_br);
			g_free(s_sr);
			g_free(s_mix);
			done = !gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
			row++;
		} while (!done);
	}
}

void
ghb_presets_list_update(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean done;
	GList *presets, *plink;
	gchar *preset, *def_preset;
	gchar *description;
	gint flags, custom, def;
	
	g_debug("ghb_presets_list_update ()");
	def_preset = ghb_settings_get_string(ud->settings, "default_preset");
	plink = presets = ghb_presets_get_names();
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
	{
		do
		{
			if (plink)
			{
				// Update row with settings data
				g_debug("Updating row");
				preset = (gchar*)plink->data;
				def = 0;
				if (strcmp(preset, def_preset) == 0)
					def = PRESET_DEFAULT;
				
				description = ghb_presets_get_description(preset);
				flags = ghb_preset_flags(preset);
				custom = flags & PRESET_CUSTOM;
				gtk_list_store_set(store, &iter, 
							0, preset, 
							1, def ? 800 : 400, 
							2, def ? 2 : 0,
						   	3, custom ? "black" : "blue", 
							4, description,
							-1);
				plink = plink->next;
				g_free(description);
				done = !gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
			}
			else
			{
				// No more settings data, remove row
				g_debug("Removing row");
				done = !gtk_list_store_remove(store, &iter);
			}
		} while (!done);
	}
	while (plink)
	{
		// Additional settings, add row
		g_debug("Adding rows");
		preset = (gchar*)plink->data;
		def = 0;
		if (strcmp(preset, def_preset) == 0)
			def = PRESET_DEFAULT;

		description = ghb_presets_get_description(preset);
		gtk_list_store_append(store, &iter);
		flags = ghb_preset_flags(preset);
		custom = flags & PRESET_CUSTOM;
		gtk_list_store_set(store, &iter, 0, preset, 
						   	1, def ? 800 : 400, 
						   	2, def ? 2 : 0,
						   	3, custom ? "black" : "blue", 
							4, description,
						   	-1);
		plink = plink->next;
		g_free(description);
	}
	g_free(def_preset);
	g_list_free (presets);
}

void
ghb_select_preset(GtkBuilder *builder, const gchar *preset)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gchar *tpreset;
	gboolean done;
	gboolean foundit = FALSE;
	
	g_debug("select_preset()");
	if (preset == NULL) return;
	treeview = GTK_TREE_VIEW(GHB_WIDGET(builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	store = gtk_tree_view_get_model (treeview);
	if (gtk_tree_model_get_iter_first(store, &iter))
	{
		do
		{
			gtk_tree_model_get(store, &iter, 0, &tpreset, -1);
			if (strcmp(preset, tpreset) == 0)
			{
				gtk_tree_selection_select_iter (selection, &iter);
				foundit = TRUE;
				g_free(tpreset);
				break;
			}
			g_free(tpreset);
			done = !gtk_tree_model_iter_next(store, &iter);
		} while (!done);
	}
	if (!foundit)
	{
		gtk_tree_model_get_iter_first(store, &iter);
		gtk_tree_selection_select_iter (selection, &iter);
	}
}

static void
update_audio_presets(signal_user_data_t *ud)
{
	g_debug("update_audio_presets");
	const GValue *audio_list;

	audio_list = ghb_settings_get_value(ud->settings, "audio_list");
	ghb_settings_set_value(ud->settings, "pref_audio_list", audio_list);
}

void
presets_save_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkWidget *dialog;
	GtkEntry *entry;
	GtkTextView *desc;
	GtkResponseType response;
	gchar *preset;

	g_debug("presets_save_clicked_cb ()");
	preset = ghb_settings_get_string (ud->settings, "preset");
	// Clear the description
	desc = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "preset_description"));
	dialog = GHB_WIDGET(ud->builder, "preset_save_dialog");
	entry = GTK_ENTRY(GHB_WIDGET(ud->builder, "preset_name"));
	gtk_entry_set_text(entry, preset);
	g_free(preset);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
	if (response == GTK_RESPONSE_OK)
	{
		// save the preset
		const gchar *name = gtk_entry_get_text(entry);
		g_debug("description to settings");
		ghb_widget_to_setting(ud->settings, GTK_WIDGET(desc));
		// Construct the audio settings presets from the current audio list
		update_audio_presets(ud);
		ghb_settings_save(ud, name);
		ghb_presets_list_update(ud);
		// Make the new preset the selected item
		ghb_select_preset(ud->builder, name);
	}
}

void
presets_restore_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	g_debug("presets_restore_clicked_cb ()");
	// Reload only the standard presets
	ghb_presets_reload(ud);
	ghb_presets_list_update(ud);
	// Updating the presets list shuffles things around
	// need to make sure the proper preset is selected
	gchar *preset = ghb_settings_get_string (ud->settings, "preset");
	ghb_select_preset(ud->builder, preset);
	g_free(preset);
}

void
prefs_dialog_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkWidget *dialog;
	GtkResponseType response;

	g_debug("prefs_dialog_cb ()");
	dialog = GHB_WIDGET(ud->builder, "prefs_dialog");
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
}

void
presets_remove_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gchar *preset;
	GtkResponseType response;

	g_debug("presets_remove_clicked_cb ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		GtkWidget *dialog;

		gtk_tree_model_get(store, &iter, 0, &preset, -1);
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
								GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
								"Confirm deletion of preset %s.", preset);
		response = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy (dialog);
		if (response == GTK_RESPONSE_YES)
		{
			GtkTreeIter nextIter = iter;
			gchar *nextPreset = NULL;
			if (!gtk_tree_model_iter_next(store, &nextIter))
			{
				if (gtk_tree_model_get_iter_first(store, &nextIter))
				{
					gtk_tree_model_get(store, &nextIter, 0, &nextPreset, -1);
				}
			}
			else
			{
				gtk_tree_model_get(store, &nextIter, 0, &nextPreset, -1);
			}
			// Remove the selected item
			// First unselect it so that selecting the new item works properly
			gtk_tree_selection_unselect_iter (selection, &iter);
			ghb_presets_remove(preset);
			ghb_presets_list_update(ud);
			ghb_select_preset(ud->builder, nextPreset);
		}
	}
}

static void
preset_update_title_deps(signal_user_data_t *ud, ghb_title_info_t *tinfo)
{
	GtkWidget *widget;

	ghb_ui_update(ud, "scale_width", 
			ghb_int64_value(tinfo->width - tinfo->crop[2] - tinfo->crop[3]));
	// If anamorphic or keep_aspect, the hight will be automatically calculated
	gboolean keep_aspect, anamorphic;
	keep_aspect = ghb_settings_get_boolean(ud->settings, "keep_aspect");
	anamorphic = ghb_settings_get_boolean(ud->settings, "anamorphic");
	if (!(keep_aspect || anamorphic))
	{
		ghb_ui_update(ud, "scale_height", 
			ghb_int64_value(tinfo->height - tinfo->crop[0] - tinfo->crop[1]));
	}

	// Set the limits of cropping.  hb_set_anamorphic_size crashes if
	// you pass it a cropped width or height == 0.
	gint bound;
	bound = tinfo->height / 2 - 2;
	widget = GHB_WIDGET (ud->builder, "crop_top");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	widget = GHB_WIDGET (ud->builder, "crop_bottom");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	bound = tinfo->width / 2 - 2;
	widget = GHB_WIDGET (ud->builder, "crop_left");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	widget = GHB_WIDGET (ud->builder, "crop_right");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(widget), 0, bound);
	if (ghb_settings_get_boolean(ud->settings, "autocrop"))
	{
		ghb_ui_update(ud, "crop_top", ghb_int64_value(tinfo->crop[0]));
		ghb_ui_update(ud, "crop_bottom", ghb_int64_value(tinfo->crop[1]));
		ghb_ui_update(ud, "crop_left", ghb_int64_value(tinfo->crop[2]));
		ghb_ui_update(ud, "crop_right", ghb_int64_value(tinfo->crop[3]));
	}
}

void
presets_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	gchar *preset;
	ghb_title_info_t tinfo;
	GtkWidget *widget;
	
	g_debug("presets_list_selection_changed_cb ()");
	widget = GHB_WIDGET (ud->builder, "presets_remove");
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		gtk_tree_model_get(store, &iter, 0, &preset, -1);
		ud->dont_clear_presets = TRUE;
		// Temporarily set the video_quality range to (0,100)
		// This is needed so the video_quality value does not get
		// truncated when set.  The range will be readjusted below
		GtkWidget *qp = GHB_WIDGET(ud->builder, "video_quality");
		gtk_range_set_range (GTK_RANGE(qp), 0, 100);
		// Clear the audio list prior to changing the preset.  Existing audio
		// can cause the container extension to be automatically changed when
		// it shouldn't be
		clear_audio_list(ud);
		ghb_set_preset(ud, preset);
		gint titleindex;
		titleindex = ghb_settings_combo_int(ud->settings, "title");
		set_pref_audio(titleindex, ud);
		ghb_settings_set_boolean(ud->settings, "preset_modified", FALSE);
		ud->dont_clear_presets = FALSE;
		if (ghb_get_title_info (&tinfo, titleindex))
		{
			preset_update_title_deps(ud, &tinfo);
		}
		ghb_set_scale (ud, GHB_SCALE_KEEP_NONE);

		gint vqmin, vqmax;
		ghb_vquality_range(ud, &vqmin, &vqmax);
		gtk_range_set_range (GTK_RANGE(qp), vqmin, vqmax);
		gtk_widget_set_sensitive(widget, TRUE);
	}
	else
	{
		g_debug("No selection???  Perhaps unselected.");
		gtk_widget_set_sensitive(widget, FALSE);
	}
}

void
queue_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
	GtkTreeModel *store;
	GtkTreeIter iter, piter;
	
	g_debug("queue_list_selection_changed_cb ()");
	// A queue entry is made up of a parent and multiple
	// children that are visible when expanded.  When and entry
	// is selected, I want the parent to be selected.
	// This is purely cosmetic.
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		if (gtk_tree_model_iter_parent (store, &piter, &iter))
		{
			GtkTreePath *path;
			GtkTreeView *treeview;
			
			gtk_tree_selection_select_iter (selection, &piter);
			path = gtk_tree_model_get_path (store, &piter);
			treeview = gtk_tree_selection_get_tree_view (selection);
			// Make the parent visible in scroll window if it is not.
			gtk_tree_view_scroll_to_cell (treeview, path, NULL, FALSE, 0, 0);
			gtk_tree_path_free(path);
		}
	}
}

static void
add_to_queue_list(signal_user_data_t *ud, GValue *settings, GtkTreeIter *piter)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkTreeStore *store;
	gchar *info;
	gint status;
	GtkTreeIter citer;
	gchar *dest, *preset, *vol_name;
	const gchar *vcodec, *container;
	gchar *fps, *vcodec_abbr;
	gint title, start_chapter, end_chapter, width, height, vqvalue;
	gint source_width, source_height;
	gboolean pass2, anamorphic, round_dim, keep_aspect, vqtype, turbo;
	
	g_debug("update_queue_list ()");
	if (settings == NULL) return;
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
		
	title = ghb_settings_combo_int(settings, "title");
	start_chapter = ghb_settings_get_int(settings, "start_chapter");
	end_chapter = ghb_settings_get_int(settings, "end_chapter");
	pass2 = ghb_settings_get_boolean(settings, "two_pass");
	vol_name = ghb_settings_get_string(settings, "volume_label");
	info = g_strdup_printf 
	(
		 "<big><b>%s</b></big> (Title %d, Chapters %d through %d, %d Video %s)",
		 vol_name, title+1, start_chapter, end_chapter, 
		 pass2 ? 2:1, pass2 ? "Passes":"Pass"
	);

	if (piter)
		iter = *piter;
	else
		gtk_tree_store_append(store, &iter, NULL);

	gtk_tree_store_set(store, &iter, 1, info, 2, "hb-queue-delete", -1);
	g_free(info);
	status = ghb_settings_get_int(settings, "job_status");
	switch (status)
	{
		case GHB_QUEUE_PENDING:
			gtk_tree_store_set(store, &iter, 0, "hb-queue-job", -1);
			break;
		case GHB_QUEUE_CANCELED:
			gtk_tree_store_set(store, &iter, 0, "hb-canceled", -1);
			break;
		case GHB_QUEUE_RUNNING:
			gtk_tree_store_set(store, &iter, 0, "hb-working0", -1);
			break;
		case GHB_QUEUE_DONE:
			gtk_tree_store_set(store, &iter, 0, "hb-complete", -1);
			break;
		default:
			gtk_tree_store_set(store, &iter, 0, "hb-queue-job", -1);
			break;
	}

	GString *str = g_string_new("");
	gboolean markers;
	gboolean preset_modified;
	gint mux;

	container = ghb_settings_combo_option(settings, "container");
	mux = ghb_settings_combo_int(settings, "container");
	dest = ghb_settings_get_string(settings, "destination");
	preset_modified = ghb_settings_get_boolean(settings, "preset_modified");
	preset = ghb_settings_get_string(settings, "preset");
	markers = ghb_settings_get_boolean(settings, "chapter_markers");

	if (preset_modified)
		g_string_append_printf(str, "<b>Customized Preset Based On:</b> %s\n", 
								preset);
	else
		g_string_append_printf(str, "<b>Preset:</b> %s\n", preset);

	if (markers)
	{
		g_string_append_printf(str, 
			"<b>Format:</b> %s Container, Chapter Markers\n", container);
	}
	else
	{
		g_string_append_printf(str, 
			"<b>Format:</b> %s Container\n", container);
	}
	if (mux == HB_MUX_MP4)
	{
		gboolean ipod, http, large;

		ipod = ghb_settings_get_boolean(settings, "ipod_file");
		http = ghb_settings_get_boolean(settings, "http_optimize_mp4");
		large = ghb_settings_get_boolean(settings, "large_mp4");
		if (http || ipod || large)
		{
			g_string_append_printf(str, "<b>MP4 Options:</b>");
			if (ipod)
				g_string_append_printf(str, " - iPod Atom");
			if (http)
				g_string_append_printf(str, " - Http Optimized");
			if (large)
				g_string_append_printf(str, " - 64 Bit");
			g_string_append_printf(str, "\n");
		}
	}
	g_string_append_printf(str, "<b>Destination:</b> %s\n", dest);

	width = ghb_settings_get_int(settings, "scale_width");
	height = ghb_settings_get_int(settings, "scale_height");
	anamorphic = ghb_settings_get_boolean(settings, "anamorphic");
	round_dim = ghb_settings_get_boolean(settings, "round_dimensions");
	keep_aspect = ghb_settings_get_boolean(settings, "keep_aspect");

	gchar *aspect_desc;
	if (anamorphic)
	{
		if (round_dim)
		{
			aspect_desc = "(Anamorphic)";
		}
		else
		{
			aspect_desc = "(Strict Anamorphic)";
		}
	}
	else
	{
		if (keep_aspect)
		{
			aspect_desc = "(Aspect Preserved)";
		}
		else
		{
			aspect_desc = "(Aspect Lost)";
		}
	}
	vqtype = ghb_settings_get_boolean(settings, "vquality_type_constant");
	vqvalue = 0;

	gchar *vq_desc = "Error";
	gchar *vq_units = "";
	if (!vqtype)
	{
		vqtype = ghb_settings_get_boolean(settings, "vquality_type_target");
		if (!vqtype)
		{
			// Has to be bitrate
			vqvalue = ghb_settings_get_int(settings, "video_bitrate");
			vq_desc = "Bitrate:";
			vq_units = "kbps";
		}
		else
		{
			// Target file size
			vqvalue = ghb_settings_get_int(settings, "video_target");
			vq_desc = "Target Size:";
			vq_units = "MB";
		}
	}
	else
	{
		// Constant quality
		vqvalue = ghb_settings_get_int(settings, "video_quality");
		vq_desc = "Constant Quality:";
	}
	fps = ghb_settings_get_string(settings, "framerate");
	if (strcmp("source", fps) == 0)
	{
		g_free(fps);
		fps = g_strdup("Same As Source");
	}
	vcodec = ghb_settings_combo_option(settings, "video_codec");
	vcodec_abbr = ghb_settings_get_string(settings, "video_codec");
	source_width = ghb_settings_get_int(settings, "source_width");
	source_height = ghb_settings_get_int(settings, "source_height");
	g_string_append_printf(str,
		"<b>Picture:</b> Source: %d x %d, Output %d x %d %s\n"
		"<b>Video:</b> %s, Framerate: %s, %s %d%s\n",
			 source_width, source_height, width, height, aspect_desc,
			 vcodec, fps, vq_desc, vqvalue, vq_units);

	turbo = ghb_settings_get_boolean(settings, "turbo");
	if (turbo)
	{
		g_string_append_printf(str, "<b>Turbo:</b> On\n");
	}
	if (strcmp(vcodec_abbr, "x264") == 0)
	{
		gchar *x264opts = ghb_build_x264opts_string(settings);
		g_string_append_printf(str, "<b>x264 Options:</b> %s\n", x264opts);
		g_free(x264opts);
	}
	// Add the audios
	gint count, ii;
	const GValue *audio_list;

	audio_list = ghb_settings_get_value(settings, "audio_list");
	count = ghb_array_len(audio_list);
	for (ii = 0; ii < count; ii++)
	{
		gchar *bitrate, *samplerate, *track;
		const gchar *acodec, *mix;
		GValue *asettings;

		asettings = ghb_array_get_nth(audio_list, ii);

		acodec = ghb_settings_combo_option(asettings, "audio_codec");
		bitrate = ghb_settings_get_string(asettings, "audio_bitrate");
		samplerate = ghb_settings_get_string(asettings, "audio_rate");
		if (strcmp("source", samplerate) == 0)
		{
			g_free(samplerate);
			samplerate = g_strdup("Same As Source");
		}
		track = ghb_settings_get_string(asettings, "audio_track_long");
		mix = ghb_settings_combo_option(asettings, "audio_mix");
		g_string_append_printf(str,
			"<b>Audio:</b> %s, Encoder: %s, Mixdown: %s, SampleRate: %s, Bitrate: %s",
			 track, acodec, mix, samplerate, bitrate);
		if (ii < count-1)
			g_string_append_printf(str, "\n");
		g_free(track);
		g_free(bitrate);
		g_free(samplerate);
	}
	info = g_string_free(str, FALSE);
	gtk_tree_store_append(store, &citer, &iter);
	gtk_tree_store_set(store, &citer, 1, info, -1);
	g_free(info);
	g_free(fps);
	g_free(vcodec_abbr);
	g_free(vol_name);
	g_free(dest);
	g_free(preset);
}

gboolean
ghb_message_dialog(GtkMessageType type, const gchar *message, const gchar *no, const gchar *yes)
{
	GtkWidget *dialog;
	GtkResponseType response;
			
	// Toss up a warning dialog
	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
							type, GTK_BUTTONS_NONE,
							message);
	gtk_dialog_add_buttons( GTK_DIALOG(dialog), 
						   no, GTK_RESPONSE_NO,
						   yes, GTK_RESPONSE_YES, NULL);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy (dialog);
	if (response == GTK_RESPONSE_NO)
	{
		return FALSE;
	}
	return TRUE;
}

static gint64
estimate_file_size(signal_user_data_t *ud)
{
	ghb_title_info_t tinfo;
	gint duration;
	gint bitrate;
	gint64 size;
	gint titleindex;

	titleindex = ghb_settings_combo_int(ud->settings, "title");
	if (titleindex < 0) return 0;
			
	if (!ghb_get_title_info(&tinfo, titleindex)) return 0;
	duration = ((tinfo.hours*60)+tinfo.minutes)*60+tinfo.seconds;
	bitrate = ghb_guess_bitrate(ud->settings);
	size = (gint64)duration * (gint64)bitrate/8;
	return size;
}

#define DISK_FREE_THRESH	(1024L*1024L*1024L*3)

static gboolean
validate_settings(signal_user_data_t *ud)
{
	// Check to see if the dest file exists or is
	// already in the queue
	gchar *message, *dest;
	gint count, ii;
	gint titleindex;

	titleindex = ghb_settings_combo_int(ud->settings, "title");
	if (titleindex < 0) return FALSE;
	dest = ghb_settings_get_string(ud->settings, "destination");
	count = ghb_array_len(ud->queue);
	for (ii = 0; ii < count; ii++)
	{
		GValue *js;
		gchar *filename;

		js = ghb_array_get_nth(ud->queue, ii);
		filename = ghb_settings_get_string(js, "destination");
		if (strcmp(dest, filename) == 0)
		{
			message = g_strdup_printf(
						"Destination: %s\n\n"
						"Another queued job has specified the same destination.\n"
						"Do you want to overwrite?",
						dest);
			if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message, "Cancel", "Overwrite"))
			{
				g_free(filename);
				g_free(dest);
				g_free(message);
				return FALSE;
			}
			g_free(message);
			break;
		}
		g_free(filename);
	}
	gchar *destdir = g_path_get_dirname(dest);
	if (!g_file_test(destdir, G_FILE_TEST_IS_DIR))
	{
		message = g_strdup_printf(
					"Destination: %s\n\n"
					"This is not a valid directory.",
					destdir);
		ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
		g_free(dest);
		g_free(message);
		g_free(destdir);
		return FALSE;
	}
	if (g_access(destdir, R_OK|W_OK) != 0)
	{
		message = g_strdup_printf(
					"Destination: %s\n\n"
					"Can not read or write the directory.",
					destdir);
		ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
		g_free(dest);
		g_free(message);
		g_free(destdir);
		return FALSE;
	}
	GFile *gfile;
	GFileInfo *info;
	guint64 size;
	gchar *resolved = ghb_resolve_symlink(destdir);

	gfile = g_file_new_for_path(resolved);
	info = g_file_query_filesystem_info(gfile, 
						G_FILE_ATTRIBUTE_FILESYSTEM_FREE, NULL, NULL);
	if (info != NULL)
	{
		if (g_file_info_has_attribute(info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE))
		{
			size = g_file_info_get_attribute_uint64(info, 
									G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
			
			gint64 fsize = estimate_file_size(ud);
			if (size < fsize)
			{
				message = g_strdup_printf(
							"Destination filesystem is almost full: %uM free\n\n"
							"Encode may be incomplete if you proceed.\n",
							(guint)(size / (1024L*1024L)));
				if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message, "Cancel", "Proceed"))
				{
					g_free(dest);
					g_free(message);
					return FALSE;
				}
				g_free(message);
			}
		}
		g_object_unref(info);
	}
	g_object_unref(gfile);
	g_free(resolved);
	g_free(destdir);
	if (g_file_test(dest, G_FILE_TEST_EXISTS))
	{
		message = g_strdup_printf(
					"Destination: %s\n\n"
					"File already exhists.\n"
					"Do you want to overwrite?",
					dest);
		if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message, "Cancel", "Overwrite"))
		{
			g_free(dest);
			g_free(message);
			return FALSE;
		}
		g_free(message);
		g_unlink(dest);
	}
	g_free(dest);
	// Validate video quality is in a reasonable range
	if (!ghb_validate_vquality(ud->settings))
	{
		return FALSE;
	}
	// Validate audio settings
	if (!ghb_validate_audio(ud))
	{
		return FALSE;
	}
	// Validate video settings
	if (!ghb_validate_video(ud))
	{
		return FALSE;
	}
	// Validate container settings
	if (!ghb_validate_container(ud))
	{
		return FALSE;
	}
	// Validate filter settings
	if (!ghb_validate_filters(ud))
	{
		return FALSE;
	}
	audio_list_refresh(ud);
	return TRUE;
}

static gboolean
queue_add(signal_user_data_t *ud)
{
	// Add settings to the queue
	GValue *settings;
	gint titleindex;
	gint titlenum;
	
	g_debug("queue_add ()");
	if (!validate_settings(ud))
	{
		return FALSE;
	}
	if (ud->queue == NULL)
		ud->queue = ghb_array_value_new(32);
	// Make a copy of current settings to be used for the new job
	settings = ghb_value_dup(ud->settings);
	ghb_settings_set_int(settings, "job_status", GHB_QUEUE_PENDING);
	ghb_settings_set_int(settings, "job_unique_id", 0);
	titleindex = ghb_settings_combo_int(settings, "title");
	titlenum = ghb_get_title_number(titleindex);
	ghb_settings_set_int(settings, "titlenum", titlenum);
	ghb_array_append(ud->queue, settings);
	add_to_queue_list(ud, settings, NULL);
	ghb_save_queue(ud->queue);

	return TRUE;
}

void
queue_add_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("queue_add_clicked_cb ()");
	queue_add(ud);
}

static gboolean
cancel_encode(const gchar *extra_msg)
{
	GtkWidget *dialog;
	GtkResponseType response;
	
	if (extra_msg == NULL) extra_msg = "";
	// Toss up a warning dialog
	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
				GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
				"%sYour movie will be lost if you don't continue encoding.",
				extra_msg);
	gtk_dialog_add_buttons( GTK_DIALOG(dialog), 
						   "Continue Encoding", GTK_RESPONSE_NO,
						   "Stop Encoding", GTK_RESPONSE_YES, NULL);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy (dialog);
	if (response == GTK_RESPONSE_NO) return FALSE;
	ghb_stop_queue();
	return TRUE;
}

void
queue_remove_clicked_cb(GtkWidget *widget, gchar *path, signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gint row;
	gint *indices;
	gint unique_id;
	GValue *settings;
	gint status;

	g_debug("queue_remove_clicked_cb ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
	store = gtk_tree_view_get_model(treeview);
	treepath = gtk_tree_path_new_from_string (path);
	if (gtk_tree_path_get_depth(treepath) > 1) return;
	if (gtk_tree_model_get_iter(store, &iter, treepath))
	{
		// Find the entry in the queue
		indices = gtk_tree_path_get_indices (treepath);
		row = indices[0];
		// Can only free the treepath After getting what I need from
		// indices since this points into treepath somewhere.
		gtk_tree_path_free (treepath);
		if (row < 0) return;
		if (row >= ghb_array_len(ud->queue))
			return;
		settings = ghb_array_get_nth(ud->queue, row);
		status = ghb_settings_get_int(settings, "job_status");
		if (status == GHB_QUEUE_RUNNING)
		{
			// Ask if wants to stop encode.
			if (!cancel_encode(NULL))
			{
				return;
			}
			unique_id = ghb_settings_get_int(settings, "job_unique_id");
			ghb_remove_job(unique_id);
		}
		// Remove the selected item
		gtk_tree_store_remove(GTK_TREE_STORE(store), &iter);
		// Remove the corresponding item from the queue list
		GValue *old = ghb_array_get_nth(ud->queue, row);
		ghb_value_free(old);
		ghb_array_remove(ud->queue, row);
		ghb_save_queue(ud->queue);
	}
	else
	{	
		gtk_tree_path_free (treepath);
	}
}

// This little bit is needed to prevent the default drag motion
// handler from expanding rows if you hover over them while
// dragging.
// Also controls where valid drop locations are
gboolean
queue_drag_motion_cb(
	GtkTreeView *tv,
	GdkDragContext *ctx,
	gint x,
	gint y,
	guint time,
	signal_user_data_t *ud)
{
	GtkTreePath *path = NULL;
	GtkTreeViewDropPosition pos;
	gint *indices, row, status, finished;
	GValue *js;
	GtkTreeIter iter;
	GtkTreeView *srctv;
	GtkTreeModel *model;
	GtkTreeSelection *select;

	// This bit checks to see if the source is allowed to be
	// moved.  Only pending and canceled items may be moved.
	srctv = GTK_TREE_VIEW(gtk_drag_get_source_widget(ctx));
	select = gtk_tree_view_get_selection (srctv);
	gtk_tree_selection_get_selected (select, &model, &iter);
	path = gtk_tree_model_get_path (model, &iter);
	indices = gtk_tree_path_get_indices(path);
	row = indices[0];
	gtk_tree_path_free(path);
	js = ghb_array_get_nth(ud->queue, row);
	status = ghb_settings_get_int(js, "job_status");
	if (status != GHB_QUEUE_PENDING && status != GHB_QUEUE_CANCELED)
	{
		gdk_drag_status(ctx, 0, time);
		return TRUE;
	}

	// The reset checks that the destination is a valid position
	// in the list.  Can not move above any finished or running items
	gtk_tree_view_get_dest_row_at_pos (tv, x, y, &path, &pos);
	if (path == NULL)
	{
		gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
		return TRUE;
	}
	// Don't allow *drop into*
	if (pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
		pos = GTK_TREE_VIEW_DROP_BEFORE;
	if (pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
		pos = GTK_TREE_VIEW_DROP_AFTER;
	// Don't allow droping int child items
	if (gtk_tree_path_get_depth(path) > 1)
	{
		gtk_tree_path_up(path);
		pos = GTK_TREE_VIEW_DROP_AFTER;
	}
	indices = gtk_tree_path_get_indices(path);
	row = indices[0];
	js = ghb_array_get_nth(ud->queue, row);

	finished = find_last_finished(ud->queue);
	if (row < finished)
	{
		gtk_tree_path_free(path);
		gdk_drag_status(ctx, 0, time);
		return TRUE;
	}
	if (pos != GTK_TREE_VIEW_DROP_AFTER && 
		row == finished)
	{
		gtk_tree_path_free(path);
		gdk_drag_status(ctx, 0, time);
		return TRUE;
	}
	gtk_tree_view_set_drag_dest_row(tv, path, pos);
	gtk_tree_path_free(path);
	gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
	return TRUE;
}

void 
queue_drag_cb(
	GtkTreeView *dstwidget, 
	GdkDragContext *dc, 
	gint x, gint y, 
	GtkSelectionData *selection_data, 
	guint info, guint t, 
	signal_user_data_t *ud)
{
	GtkTreePath *path = NULL;
	//GtkTreeModel *model;
	GtkTreeViewDropPosition pos;
	GtkTreeIter dstiter, srciter;
	gint *indices, row;
	GValue *js;
	
	GtkTreeModel *dstmodel = gtk_tree_view_get_model(dstwidget);
			
	g_debug("queue_drag_cb ()");
	// This doesn't work here for some reason...
	// gtk_tree_view_get_drag_dest_row(dstwidget, &path, &pos);
	gtk_tree_view_get_dest_row_at_pos (dstwidget, x, y, &path, &pos);
	// This little hack is needed because attempting to drop after
	// the last item gives us no path or pos.
	if (path == NULL)
	{
		gint n_children;

		n_children = gtk_tree_model_iter_n_children(dstmodel, NULL);
		if (n_children)
		{
			pos = GTK_TREE_VIEW_DROP_AFTER;
			path = gtk_tree_path_new_from_indices(n_children-1, -1);
		}
		else
		{
			pos = GTK_TREE_VIEW_DROP_BEFORE;
			path = gtk_tree_path_new_from_indices(0, -1);
		}
	}
	if (path)
	{
		if (gtk_tree_path_get_depth(path) > 1)
			gtk_tree_path_up(path);
		if (gtk_tree_model_get_iter (dstmodel, &dstiter, path))
		{
			GtkTreeIter iter;
			GtkTreeView *srcwidget;
			GtkTreeModel *srcmodel;
			GtkTreeSelection *select;
			GtkTreePath *srcpath = NULL;
			GtkTreePath *dstpath = NULL;

			srcwidget = GTK_TREE_VIEW(gtk_drag_get_source_widget(dc));
			//srcmodel = gtk_tree_view_get_model(srcwidget);
			select = gtk_tree_view_get_selection (srcwidget);
			gtk_tree_selection_get_selected (select, &srcmodel, &srciter);

			srcpath = gtk_tree_model_get_path (srcmodel, &srciter);
			indices = gtk_tree_path_get_indices(srcpath);
			row = indices[0];
			gtk_tree_path_free(srcpath);
			js = ghb_array_get_nth(ud->queue, row);

			switch (pos)
			{
				case GTK_TREE_VIEW_DROP_BEFORE:
				case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
					gtk_tree_store_insert_before (GTK_TREE_STORE (dstmodel), 
													&iter, NULL, &dstiter);
					break;

				case GTK_TREE_VIEW_DROP_AFTER:
				case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
					gtk_tree_store_insert_after (GTK_TREE_STORE (dstmodel), 
													&iter, NULL, &dstiter);
					break;

				default:
					break;
			}
			// Reset job to pending
			ghb_settings_set_int(js, "job_status", GHB_QUEUE_PENDING);
			add_to_queue_list(ud, js, &iter);

			dstpath = gtk_tree_model_get_path (dstmodel, &iter);
			indices = gtk_tree_path_get_indices(dstpath);
			row = indices[0];
			gtk_tree_path_free(dstpath);
			ghb_array_insert(ud->queue, row, js);

			srcpath = gtk_tree_model_get_path (srcmodel, &srciter);
			indices = gtk_tree_path_get_indices(srcpath);
			row = indices[0];
			gtk_tree_path_free(srcpath);
			ghb_array_remove(ud->queue, row);
			gtk_tree_store_remove (GTK_TREE_STORE (srcmodel), &srciter);
			ghb_save_queue(ud->queue);
		}
		gtk_tree_path_free(path);
	}
}


static gint
find_last_finished(GValue *queue)
{
	GValue *js;
	gint ii, count;
	gint status;
	
	g_debug("find_last_finished");
	count = ghb_array_len(queue);
	for (ii = 0; ii < count; ii++)
	{
		js = ghb_array_get_nth(queue, ii);
		status = ghb_settings_get_int(js, "job_status");
		if (status != GHB_QUEUE_DONE && status != GHB_QUEUE_RUNNING)
		{
			return ii-1;
		}
	}
	return -1;
}

static gint
find_queue_job(GValue *queue, gint unique_id, GValue **job)
{
	GValue *js;
	gint ii, count;
	gint job_unique_id;
	
	*job = NULL;
	g_debug("find_queue_job");
	count = ghb_array_len(queue);
	for (ii = 0; ii < count; ii++)
	{
		js = ghb_array_get_nth(queue, ii);
		job_unique_id = ghb_settings_get_int(js, "job_unique_id");
		if (job_unique_id == unique_id)
		{
			*job = js;
			return ii;
		}
	}
	return -1;
}

static void
queue_buttons_grey(signal_user_data_t *ud, gboolean working)
{
	GtkWidget *widget;
	GtkAction *action;
	gint queue_count;
	gint titleindex;
	gboolean title_ok;

	queue_count = ghb_array_len(ud->queue);
	titleindex = ghb_settings_combo_int(ud->settings, "title");
	title_ok = (titleindex >= 0);

	widget = GHB_WIDGET (ud->builder, "queue_start1");
	gtk_widget_set_sensitive (widget, !working && (title_ok || queue_count));
	widget = GHB_WIDGET (ud->builder, "queue_start2");
	gtk_widget_set_sensitive (widget, !working && (title_ok || queue_count));
	action = GHB_ACTION (ud->builder, "queue_start_menu");
	gtk_action_set_sensitive (action, !working && (title_ok || queue_count));
	widget = GHB_WIDGET (ud->builder, "queue_pause1");
	gtk_widget_set_sensitive (widget, working);
	widget = GHB_WIDGET (ud->builder, "queue_pause2");
	gtk_widget_set_sensitive (widget, working);
	action = GHB_ACTION (ud->builder, "queue_pause_menu");
	gtk_action_set_sensitive (action, working);
	widget = GHB_WIDGET (ud->builder, "queue_stop");
	gtk_widget_set_sensitive (widget, working);
	action = GHB_ACTION (ud->builder, "queue_stop_menu");
	gtk_action_set_sensitive (action, working);
}

void queue_start_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud);

static void
submit_job(GValue *settings)
{
	static gint unique_id = 1;

	g_debug("submit_job");
	if (settings == NULL) return;
	ghb_settings_set_int(settings, "job_unique_id", unique_id);
	ghb_settings_set_int(settings, "job_status", GHB_QUEUE_RUNNING);
	ghb_add_job (settings, unique_id);
	ghb_start_queue();
	unique_id++;
}

static void
queue_scan(GValue *js)
{
	gchar *path;
	gint titlenum;

	path = ghb_settings_get_string( js, "source");
	titlenum = ghb_settings_get_int(js, "titlenum");
	ghb_backend_queue_scan(path, titlenum);
	g_free(path);
}

static GValue* 
start_next_job(signal_user_data_t *ud, gboolean find_first)
{
	static gint current = 0;
	gint count, ii, jj;
	GValue *js;
	gint status;

	g_debug("start_next_job");
	count = ghb_array_len(ud->queue);
	if (find_first)
	{	// Start the first pending item in the queue
		current = 0;
		for (ii = 0; ii < count; ii++)
		{

			js = ghb_array_get_nth(ud->queue, ii);
			status = ghb_settings_get_int(js, "job_status");
			if (status == GHB_QUEUE_PENDING)
			{
				current = ii;
				queue_scan(js);
				return js;
			}
		}
		// Nothing pending
		return NULL;
	}
	// Find the next pending item after the current running item
	for (ii = 0; ii < count-1; ii++)
	{
		js = ghb_array_get_nth(ud->queue, ii);
		status = ghb_settings_get_int(js, "job_status");
		if (status == GHB_QUEUE_RUNNING)
		{
			for (jj = ii+1; jj < count; jj++)
			{
				js = ghb_array_get_nth(ud->queue, jj);
				status = ghb_settings_get_int(js, "job_status");
				if (status == GHB_QUEUE_PENDING)
				{
					current = jj;
					queue_scan(js);
					return js;
				}
			}
		}
	}
	// No running item found? Maybe it was deleted
	// Look for a pending item starting from the last index we started
	for (ii = current; ii < count; ii++)
	{
		js = ghb_array_get_nth(ud->queue, ii);
		status = ghb_settings_get_int(js, "job_status");
		if (status == GHB_QUEUE_PENDING)
		{
			current = ii;
			queue_scan(js);
			return js;
		}
	}
	// Nothing found
	return NULL;
}

static void
ghb_backend_events(signal_user_data_t *ud)
{
	ghb_status_t status;
	gchar *status_str;
	GtkProgressBar *progress;
	gint titleindex;
	GValue *js;
	gint index;
	GtkTreeView *treeview;
	GtkTreeStore *store;
	GtkTreeIter iter;
	static gint working = 0;
	static gboolean work_started = FALSE;
	
	ghb_track_status();
	ghb_get_status(&status);
	progress = GTK_PROGRESS_BAR(GHB_WIDGET (ud->builder, "progressbar"));
	// First handle the status of title scans
	// Then handle the status of the queue
	if (status.state & GHB_STATE_SCANNING)
	{
		status_str = g_strdup_printf ("Scanning title %d of %d...", 
								  status.title_cur, status.title_count );
		gtk_progress_bar_set_text (progress, status_str);
		g_free(status_str);
		if (status.title_count > 0)
		{
			gtk_progress_bar_set_fraction (progress, 
				(gdouble)status.title_cur / status.title_count);
		}
	}
	else if (status.state & GHB_STATE_SCANDONE)
	{
		status_str = g_strdup_printf ("Scan done"); 
		gtk_progress_bar_set_text (progress, status_str);
		g_free(status_str);
		gtk_progress_bar_set_fraction (progress, 1.0);

		ghb_title_info_t tinfo;
			
		ghb_update_ui_combo_box(ud->builder, "title", 0, FALSE);
		titleindex = ghb_longest_title();
		ghb_ui_update(ud, "title", ghb_int64_value(titleindex));

		// Are there really any titles.
		if (!ghb_get_title_info(&tinfo, titleindex))
		{
			GtkProgressBar *progress;
			progress = GTK_PROGRESS_BAR(GHB_WIDGET (ud->builder, "progressbar"));
			gtk_progress_bar_set_fraction (progress, 0);
			gtk_progress_bar_set_text (progress, "No Source");
		}
		ghb_clear_state(GHB_STATE_SCANDONE);
		queue_buttons_grey(ud, work_started);
	}
	else if (status.queue_state & GHB_STATE_SCANNING)
	{
		status_str = g_strdup_printf ("Scanning ...");
		gtk_progress_bar_set_text (progress, status_str);
		g_free(status_str);
		gtk_progress_bar_set_fraction (progress, 0);
	}
	else if (status.queue_state & GHB_STATE_SCANDONE)
	{
		ghb_clear_queue_state(GHB_STATE_SCANDONE);
		submit_job(ud->current_job);
	}
	else if (status.queue_state & GHB_STATE_PAUSED)
	{
		status_str = g_strdup_printf ("Paused"); 
		gtk_progress_bar_set_text (progress, status_str);
		g_free(status_str);
	}
	else if (status.queue_state & GHB_STATE_WORKING)
	{
		gchar *task_str, *job_str;
		gint qcount;

		if (status.job_count > 1)
		{
			task_str = g_strdup_printf("pass %d of %d, ", 
				status.job_cur, status.job_count);
		}
		else
		{
			task_str = g_strdup("");
		}
		qcount = ghb_array_len(ud->queue);
		if (qcount > 1)
		{
			index = find_queue_job(ud->queue, status.unique_id, &js);
			job_str = g_strdup_printf("job %d of %d, ", index+1, qcount);
		}
		else
		{
			job_str = g_strdup("");
		}
		if(status.seconds > -1)
		{
			status_str= g_strdup_printf(
				"Encoding: %s%s%.2f %%"
				" (%.2f fps, avg %.2f fps, ETA %02dh%02dm%02ds)",
				job_str, task_str,
				100.0 * status.progress,
				status.rate_cur, status.rate_avg, status.hours, 
				status.minutes, status.seconds );
		}
		else
		{
			status_str= g_strdup_printf(
				"Encoding: %s%s%.2f %%",
				job_str, task_str,
				100.0 * status.progress );
		}
		g_free(job_str);
		g_free(task_str);
		gtk_progress_bar_set_text (progress, status_str);
		gtk_progress_bar_set_fraction (progress, status.progress);
		g_free(status_str);
	}
	else if (status.queue_state & GHB_STATE_WORKDONE)
	{
		gint qstatus;

		work_started = FALSE;
		queue_buttons_grey(ud, FALSE);
		index = find_queue_job(ud->queue, status.unique_id, &js);
		treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
		store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
		if (ud->cancel_encode)
			status.error = GHB_ERROR_CANCELED;
		switch( status.error )
		{
			case GHB_ERROR_NONE:
				gtk_progress_bar_set_text( progress, "Rip done!" );
				qstatus = GHB_QUEUE_DONE;
				if (js != NULL)
				{
					gchar *path = g_strdup_printf ("%d", index);
					if (gtk_tree_model_get_iter_from_string(
							GTK_TREE_MODEL(store), &iter, path))
					{
						gtk_tree_store_set(store, &iter, 0, "hb-complete", -1);
					}
					g_free(path);
				}
				break;
			case GHB_ERROR_CANCELED:
				gtk_progress_bar_set_text( progress, "Rip canceled." );
				qstatus = GHB_QUEUE_CANCELED;
				if (js != NULL)
				{
					gchar *path = g_strdup_printf ("%d", index);
					if (gtk_tree_model_get_iter_from_string(
							GTK_TREE_MODEL(store), &iter, path))
					{
						gtk_tree_store_set(store, &iter, 0, "hb-canceled", -1);
					}
					g_free(path);
				}
				break;
			default:
				gtk_progress_bar_set_text( progress, "Rip failed.");
				qstatus = GHB_QUEUE_CANCELED;
				if (js != NULL)
				{
					gchar *path = g_strdup_printf ("%d", index);
					if (gtk_tree_model_get_iter_from_string(
							GTK_TREE_MODEL(store), &iter, path))
					{
						gtk_tree_store_set(store, &iter, 0, "hb-canceled", -1);
					}
					g_free(path);
				}
		}
		gtk_progress_bar_set_fraction (progress, 1.0);
		ghb_clear_queue_state(GHB_STATE_WORKDONE);
		if (!ud->cancel_encode)
			ud->current_job = start_next_job(ud, FALSE);
		else
			ud->current_job = NULL;
		if (js)
			ghb_settings_set_int(js, "job_status", qstatus);
		ghb_save_queue(ud->queue);
		ud->cancel_encode = FALSE;
	}
	else if (status.queue_state & GHB_STATE_MUXING)
	{
		gtk_progress_bar_set_text(progress, "Muxing: this may take awhile...");
	}
	if (status.queue_state & GHB_STATE_SCANNING)
	{
		// This needs to be in scanning and working since scanning
		// happens fast enough that it can be missed
		if (!work_started)
		{
			work_started = TRUE;
			queue_buttons_grey(ud, TRUE);
		}
	}
	if (status.queue_state & GHB_STATE_WORKING)
	{
		// This needs to be in scanning and working since scanning
		// happens fast enough that it can be missed
		if (!work_started)
		{
			work_started = TRUE;
			queue_buttons_grey(ud, TRUE);
		}
		index = find_queue_job(ud->queue, status.unique_id, &js);
		if (status.unique_id != 0 && index >= 0)
		{
			gchar working_icon[] = "hb-working0";
			working_icon[10] = '0' + working;
			working = (working+1) % 6;
			treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
			store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
			gchar *path = g_strdup_printf ("%d", index);
			if (gtk_tree_model_get_iter_from_string(
					GTK_TREE_MODEL(store), &iter, path))
			{
				gtk_tree_store_set(store, &iter, 0, working_icon, -1);
			}
			g_free(path);
		}
	}
}

gboolean
ghb_timer_cb(gpointer data)
{
	signal_user_data_t *ud = (signal_user_data_t*)data;

	ghb_backend_events(ud);
	if (update_default_destination)
	{
		gchar *dest, *dest_dir, *def_dest;
		dest = ghb_settings_get_string(ud->settings, "destination");
		dest_dir = g_path_get_dirname (dest);
		def_dest = ghb_settings_get_string(ud->settings, "destination_dir");
		if (strcmp(dest_dir, def_dest) != 0)
		{
			ghb_settings_set_string (ud->settings, "destination_dir", dest_dir);
			ghb_pref_save (ud->settings, "destination_dir");
		}
		g_free(dest);
		g_free(dest_dir);
		g_free(def_dest);
		update_default_destination = FALSE;
	}
	if (update_preview)
	{
		set_preview_image (ud);
		update_preview = FALSE;
	}
	return TRUE;
}

gboolean
ghb_log_cb(GIOChannel *source, GIOCondition cond, gpointer data)
{
	gchar *text = NULL;
	gsize length;
	GtkTextView *textview;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	GtkTextMark *mark;
	GError *gerror = NULL;
	GIOStatus status;
	
	signal_user_data_t *ud = (signal_user_data_t*)data;

	status = g_io_channel_read_line (source, &text, &length, NULL, &gerror);
	if (text != NULL)
	{
		GdkWindow *window;
		gint width, height;
		gint x, y;
		gboolean bottom = FALSE;

		textview = GTK_TEXT_VIEW(GHB_WIDGET (ud->builder, "activity_view"));
		buffer = gtk_text_view_get_buffer (textview);
		// I would like to auto-scroll the window when the scrollbar
		// is at the bottom, 
		// must determining whether the insert point is at
		// the bottom of the window 
		window = gtk_text_view_get_window(textview, GTK_TEXT_WINDOW_TEXT);
		if (window != NULL)
		{
			gdk_drawable_get_size(GDK_DRAWABLE(window), &width, &height);
			gtk_text_view_window_to_buffer_coords(textview, 
				GTK_TEXT_WINDOW_TEXT, width, height, &x, &y);
			gtk_text_view_get_iter_at_location(textview, &iter, x, y);
			if (gtk_text_iter_is_end(&iter))
			{
				bottom = TRUE;
			}
		}
		else
		{
			// If the window isn't available, assume bottom
			bottom = TRUE;
		}
		gtk_text_buffer_get_end_iter(buffer, &iter);
		gtk_text_buffer_insert(buffer, &iter, text, -1);
		if (bottom)
		{
			//gtk_text_view_scroll_to_iter(textview, &iter, 0, FALSE, 0, 0);
			mark = gtk_text_buffer_get_insert (buffer);
			gtk_text_view_scroll_mark_onscreen(textview, mark);
		}
		g_io_channel_write_chars (ud->activity_log, text, length, &length, NULL);
		g_free(text);
	}
	if (status != G_IO_STATUS_NORMAL)
	{
		// This should never happen, but if it does I would get into an
		// infinite loop.  Returning false removes this callback.
		g_warning("Error while reading activity from pipe");
		if (gerror != NULL)
		{
			g_warning("%s", gerror->message);
			g_error_free (gerror);
		}
		return FALSE;
	}
	if (gerror != NULL)
		g_error_free (gerror);
	return TRUE;
}

void
about_activate_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkWidget *widget = GHB_WIDGET (ud->builder, "hb_about");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(widget), ghb_version());
	gtk_widget_show (widget);
}

void
guide_activate_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	gboolean result;
	char *argv[] = 
		{"xdg-open","http://trac.handbrake.fr/wiki/HandBrakeGuide",NULL,NULL};
	result = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL,
				NULL, NULL, NULL);
	if (result) return;

	argv[0] = "gnome-open";
	result = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL,
				NULL, NULL, NULL);
	if (result) return;

	argv[0] = "kfmclient";
	argv[1] = "exec";
	argv[2] = "http://trac.handbrake.fr/wiki/HandBrakeGuide";
	result = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL,
				NULL, NULL, NULL);
	if (result) return;

	argv[0] = "firefox";
	argv[1] = "http://trac.handbrake.fr/wiki/HandBrakeGuide";
	argv[2] = NULL;
	result = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL,
				NULL, NULL, NULL);
	if (result) return;
}

void
hb_about_response_cb(GtkWidget *widget, gint response, signal_user_data_t *ud)
{
	gtk_widget_hide (widget);
}

void
show_activity_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkWidget *widget = GHB_WIDGET (ud->builder, "activity_window");
	gtk_widget_show (widget);
}

void
show_queue_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkWidget *widget = GHB_WIDGET (ud->builder, "queue_window");
	gtk_widget_show (widget);
}

void
show_presets_toggled_cb(GtkToggleButton *button, signal_user_data_t *ud)
{
	GtkWidget *widget;
	GtkWindow *hb_window;
	
	g_debug("show_presets_clicked_cb ()");
	widget = GHB_WIDGET (ud->builder, "presets_frame");
	if (gtk_toggle_button_get_active(button))
	{
		gtk_widget_show_now(widget);
	}
	else
	{
		gtk_widget_hide(widget);
		hb_window = GTK_WINDOW(GHB_WIDGET (ud->builder, "hb_window"));
		gtk_window_resize(hb_window, 16, 16);
	}
	ghb_widget_to_setting(ud->settings, GTK_WIDGET(button));
	ghb_pref_save(ud->settings, "show_presets");
}

void
presets_frame_size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation, signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		GtkTreePath *path;
		path = gtk_tree_model_get_path (store, &iter);
		// Make the parent visible in scroll window if it is not.
		gtk_tree_view_scroll_to_cell (treeview, path, NULL, FALSE, 0, 0);
		gtk_tree_path_free(path);
	}
}

static void
update_chapter_list(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean done;
	GValue *chapters;
	gint titleindex, ii;
	gint count;
	
	g_debug("update_chapter_list ()");
	titleindex = ghb_settings_combo_int(ud->settings, "title");
	chapters = ghb_get_chapters(titleindex);
	count = ghb_array_len(chapters);
	if (chapters)
		ghb_settings_set_value(ud->settings, "chapter_list", chapters);
	
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "chapters_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	ii = 0;
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
	{
		do
		{

			if (ii < count)
			{
				gchar *chapter;

				// Update row with settings data
				g_debug("Updating row");
				chapter = ghb_value_string(ghb_array_get_nth(chapters, ii));
				gtk_list_store_set(store, &iter, 
					0, ii+1,
					1, chapter,
					2, TRUE,
					-1);
				g_free(chapter);
				ii++;
				done = !gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
			}
			else
			{
				// No more settings data, remove row
				g_debug("Removing row");
				done = !gtk_list_store_remove(store, &iter);
			}
		} while (!done);
	}
	while (ii < count)
	{
		gchar *chapter;

		// Additional settings, add row
		g_debug("Adding row");
		chapter = ghb_value_string(ghb_array_get_nth(chapters, ii));
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
			0, ii+1,
			1, chapter,
			2, TRUE,
			-1);
		g_free(chapter);
		ii++;
	}
}

static gint chapter_edit_key = 0;

gboolean
chapter_keypress_cb(
	GhbCellRendererText *cell,
	GdkEventKey *event,
	signal_user_data_t *ud)
{
	chapter_edit_key = event->keyval;
	return FALSE;
}

void
chapter_edited_cb(
	GhbCellRendererText *cell, 
	gchar *path, 
	gchar *text, 
	signal_user_data_t *ud)
{
	GtkTreePath *treepath;
	GtkListStore *store;
	GtkTreeView *treeview;
	GtkTreeIter iter;
	gint index;
	gint *pi;
	gint row;
	
	g_debug("chapter_edited_cb ()");
	g_debug("path (%s)", path);
	g_debug("text (%s)", text);
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "chapters_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	treepath = gtk_tree_path_new_from_string (path);
	pi = gtk_tree_path_get_indices(treepath);
	row = pi[0];
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath);
	gtk_list_store_set(store, &iter, 
		1, text,
		2, TRUE,
		-1);
	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &index, -1);

	GValue *chapters;
	GValue *chapter;

	chapters = ghb_settings_get_value(ud->settings, "chapter_list");
	chapter = ghb_array_get_nth(chapters, index-1);
	g_value_set_string(chapter, text);
	if ((chapter_edit_key == GDK_Return || chapter_edit_key == GDK_Down) &&
		gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter))
	{
		GtkTreeViewColumn *column;

		gtk_tree_path_next(treepath);
		// When a cell has been edited, I want to advance to the
		// next cell and start editing it automaitcally.
		// Unfortunately, we may not be in a state here where
		// editing is allowed.  This happens when the user selects
		// a new cell with the mouse instead of just hitting enter.
		// Some kind of Gtk quirk.  widget_editable==NULL assertion.
		// Editing is enabled again once the selection event has been
		// processed.  So I'm queueing up a callback to be called
		// when things go idle.  There, I will advance to the next
		// cell and initiate editing.
		//
		// Now, you might be asking why I don't catch the keypress
		// event and determine what action to take based on that.
		// The Gtk developers in their infinite wisdom have made the 
		// actual GtkEdit widget being used a private member of
		// GtkCellRendererText, so it can not be accessed to hang a
		// signal handler off of.  And they also do not propagate the
		// keypress signals in any other way.  So that information is lost.
		//g_idle_add((GSourceFunc)next_cell, ud);
		//
		// Keeping the above comment for posterity.
		// I got industrious and made my own CellTextRendererText that
		// passes on the key-press-event. So now I have much better
		// control of this.
		column = gtk_tree_view_get_column(treeview, 1);
		gtk_tree_view_set_cursor(treeview, treepath, column, TRUE);
	}
	else if (chapter_edit_key == GDK_Up && row > 0)
	{
		GtkTreeViewColumn *column;
		gtk_tree_path_prev(treepath);
		column = gtk_tree_view_get_column(treeview, 1);
		gtk_tree_view_set_cursor(treeview, treepath, column, TRUE);
	}
	gtk_tree_path_free (treepath);
}

void
chapter_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
	g_debug("chapter_list_selection_changed_cb ()");
	//chapter_selection_changed = TRUE;
}

void
queue_list_size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation, GtkCellRenderer *cell)
{
	GtkTreeViewColumn *column;
	gint width;
	
	column = gtk_tree_view_get_column (GTK_TREE_VIEW(widget), 0);
	width = gtk_tree_view_column_get_width(column);
	g_debug("col width %d alloc width %d", width, allocation->width);
	// Set new wrap-width.  Shave a little off to accomidate the icons
	// that share this column.
	if (width >= 564) // Don't allow below a certain size
		g_object_set(cell, "wrap-width", width-70, NULL);
}

void
preview_button_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	gint titleindex;

	titleindex = ghb_settings_combo_int(ud->settings, "title");
	if (titleindex < 0) return;
	g_debug("titleindex %d", titleindex);

	GtkWidget *widget = GHB_WIDGET (ud->builder, "preview_window");
	gtk_widget_show (widget);
}

void
preview_frame_value_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	set_preview_image(ud);
}

void
preview_button_size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation, signal_user_data_t *ud)
{
	g_debug("-------------------------------allocate %d x %d", allocation->width, allocation->height);
	if (preview_button_width == allocation->width &&
		preview_button_height == allocation->height)
	{
		// Nothing to do. Bug out.
		g_debug("nothing to do");
		return;
	}
	g_debug("-------------------------------prev allocate %d x %d", preview_button_width, preview_button_height);
	preview_button_width = allocation->width;
	preview_button_height = allocation->height;
	set_preview_image(ud);
}

void
queue_start_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GValue *js;
	gboolean running = FALSE;
	gint count, ii;
	gint status;
	gint state;

	count = ghb_array_len(ud->queue);
	for (ii = 0; ii < count; ii++)
	{
		js = ghb_array_get_nth(ud->queue, ii);
		status = ghb_settings_get_int(js, "job_status");
		if ((status == GHB_QUEUE_RUNNING) || 
			(status == GHB_QUEUE_PENDING))
		{
			running = TRUE;
			break;
		}
	}
	if (!running)
	{
		// The queue has no running or pending jobs.
		// Add current settings to the queue, then run.
		if (!queue_add(ud))
			return;
	}
	state = ghb_get_queue_state();
	if (state == GHB_STATE_IDLE)
	{
		// Add the first pending queue item and start
		ud->current_job = start_next_job(ud, TRUE);
	}
}

void
queue_stop_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	ud->cancel_encode = TRUE;
	cancel_encode(NULL);
}

void
queue_pause_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	ghb_pause_queue();
}

void
presets_default_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	ghb_set_preset_default(ud->settings);
	ghb_presets_list_update(ud);
}

void
debug_log_handler(const gchar *domain, GLogLevelFlags flags, const gchar *msg, gpointer data)
{
	signal_user_data_t *ud = (signal_user_data_t*)data;
	
	if (ud->debug)
	{
		printf("%s: %s\n", domain, msg);
	}
}

static void
set_visible(GtkWidget *widget, gboolean visible)
{
	if (visible)
	{
		gtk_widget_show_now(widget);
	}
	else
	{
		gtk_widget_hide(widget);
	}
}

void
ghb_hbfd(signal_user_data_t *ud, gboolean hbfd)
{
	GtkWidget *widget;
	g_debug("ghb_hbfd");
	widget = GHB_WIDGET(ud->builder, "queue_pause1");
	set_visible(widget, !hbfd);
	widget = GHB_WIDGET(ud->builder, "queue_add");
	set_visible(widget, !hbfd);
	widget = GHB_WIDGET(ud->builder, "show_queue");
	set_visible(widget, !hbfd);
	widget = GHB_WIDGET(ud->builder, "show_activity");
	set_visible(widget, !hbfd);

	widget = GHB_WIDGET(ud->builder, "chapter_box");
	set_visible(widget, !hbfd);
	widget = GHB_WIDGET(ud->builder, "container_box");
	set_visible(widget, !hbfd);
	widget = GHB_WIDGET(ud->builder, "settings_box");
	set_visible(widget, !hbfd);
	widget = GHB_WIDGET(ud->builder, "presets_save");
	set_visible(widget, !hbfd);
	widget = GHB_WIDGET(ud->builder, "presets_remove");
	set_visible(widget, !hbfd);
	widget = GHB_WIDGET(ud->builder, "presets_default");
	set_visible(widget, !hbfd);
	widget = GHB_WIDGET (ud->builder, "hb_window");
	gtk_window_resize(GTK_WINDOW(widget), 16, 16);

}

void
hbfd_toggled_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("hbfd_toggled_cb");
	ghb_widget_to_setting (ud->settings, widget);
	gboolean hbfd = ghb_settings_get_boolean(ud->settings, "hbfd");
	ghb_hbfd(ud, hbfd);
	ghb_pref_save(ud->settings, "hbfd");
}

void
pref_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("pref_changed_cb");
	ghb_widget_to_setting (ud->settings, widget);
	check_dependency(ud, widget);
	const gchar *name = gtk_widget_get_name(widget);
	ghb_pref_save(ud->settings, name);
}

void
tweaks_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("tweaks_changed_cb");
	ghb_widget_to_setting (ud->settings, widget);
	const gchar *name = gtk_widget_get_name(widget);
	ghb_pref_save(ud->settings, name);

	gboolean tweaks = ghb_settings_get_boolean(ud->settings, "allow_tweaks");
	widget = GHB_WIDGET(ud->builder, "deinterlace");
	tweaks ? gtk_widget_hide(widget) : gtk_widget_show(widget);
	widget = GHB_WIDGET(ud->builder, "tweak_deinterlace");
	!tweaks ? gtk_widget_hide(widget) : gtk_widget_show(widget);

	widget = GHB_WIDGET(ud->builder, "denoise");
	tweaks ? gtk_widget_hide(widget) : gtk_widget_show(widget);
	widget = GHB_WIDGET(ud->builder, "tweak_denoise");
	!tweaks ? gtk_widget_hide(widget) : gtk_widget_show(widget);
	if (tweaks)
	{
		const GValue *value;
		value = ghb_settings_get_value(ud->settings, "deinterlace");
		ghb_ui_update(ud, "tweak_deinterlace", value);
		value = ghb_settings_get_value(ud->settings, "denoise");
		ghb_ui_update(ud, "tweak_denoise", value);
	}
	else
	{
		const GValue *value;
		value = ghb_settings_get_value(ud->settings, "tweak_deinterlace");
		ghb_ui_update(ud, "deinterlace", value);
		value = ghb_settings_get_value(ud->settings, "tweak_denoise");
		ghb_ui_update(ud, "denoise", value);
	}
}

void
hbfd_feature_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("hbfd_feature_changed_cb");
	ghb_widget_to_setting (ud->settings, widget);
	const gchar *name = gtk_widget_get_name(widget);
	ghb_pref_save(ud->settings, name);

	gboolean hbfd = ghb_settings_get_boolean(ud->settings, "hbfd_feature");
	GtkAction *action;
	if (hbfd)
	{
		const GValue *val;
		val = ghb_settings_get_value(ud->settings, "hbfd");
		ghb_ui_update(ud, "hbfd", val);
	}
	action = GHB_ACTION (ud->builder, "hbfd");
	gtk_action_set_visible(action, hbfd);
}

void
ghb_file_menu_add_dvd(signal_user_data_t *ud)
{
	GList *link, *drives;

	GtkActionGroup *agroup = GTK_ACTION_GROUP(
		gtk_builder_get_object(ud->builder, "actiongroup1"));
	GtkUIManager *ui = GTK_UI_MANAGER(
		gtk_builder_get_object(ud->builder, "uimanager1"));
	guint merge_id = gtk_ui_manager_new_merge_id(ui);

	link = drives = dvd_device_list();
	while (link != NULL)
	{
		gchar *name = (gchar*)link->data;
		// Create action for this drive
		GtkAction *action = gtk_action_new(name, name,
			"Scan this DVD source", "gtk-cdrom");
		// Add action to action group
		gtk_action_group_add_action_with_accel(agroup, action, "");
		// Add to ui manager
		gtk_ui_manager_add_ui(ui, merge_id, 
			"ui/menubar1/menuitem1/quit1", name, name,
			GTK_UI_MANAGER_AUTO, TRUE);
		// Connect signal to action (menu item)
		g_signal_connect(action, "activate", 
			(GCallback)dvd_source_activate_cb, ud);
		g_free(name);
		link = link->next;
	}
	g_list_free(drives);

	// Add separator
	gtk_ui_manager_add_ui(ui, merge_id, 
		"ui/menubar1/menuitem1/quit1", "", NULL,
		GTK_UI_MANAGER_AUTO, TRUE);
}

gboolean ghb_is_cd(GDrive *gd);

static GList*
dvd_device_list()
{
	GVolumeMonitor *gvm;
	GList *drives, *link;
	GList *dvd_devices = NULL;
	
	gvm = g_volume_monitor_get ();
	drives = g_volume_monitor_get_connected_drives (gvm);
	link = drives;
	while (link != NULL)
	{
		GDrive *gd;
		
		gd = (GDrive*)link->data;
		if (ghb_is_cd(gd))
		{
			gchar *device;
			device = g_drive_get_identifier(gd, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
			dvd_devices = g_list_append(dvd_devices, (gpointer)device);
		}
		g_object_unref (gd);
		link = link->next;
	}
	g_list_free(drives);
	return dvd_devices;
}


static DBusConnection *dbus_connection = NULL;
static LibHalContext *hal_ctx;

gboolean
ghb_is_cd(GDrive *gd)
{
	gchar *device;
	LibHalDrive *halDrive;
	LibHalDriveType dtype;

	device = g_drive_get_identifier(gd, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
	halDrive = libhal_drive_from_device_file (hal_ctx, device);
	dtype = libhal_drive_get_type(halDrive);
	libhal_drive_free(halDrive);
	g_free(device);
	return (dtype == LIBHAL_DRIVE_TYPE_CDROM);
}

void
drive_changed_cb(GVolumeMonitor *gvm, GDrive *gd, signal_user_data_t *ud)
{
	gchar *device;
	gint state = ghb_get_state();
	static gboolean first_time = TRUE;

	if (ud->current_dvd_device == NULL) return;
	// A drive change event happens when the program initially starts
	// and I don't want to automatically scan at that time.
	if (first_time)
	{
		first_time = FALSE;
		return;
	}
	if (state != GHB_STATE_IDLE) return;
	device = g_drive_get_identifier(gd, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
	
	// DVD insertion detected.  Scan it.
	if (strcmp(device, ud->current_dvd_device) == 0)
	{
		if (g_drive_has_media (gd))
		{
			GtkProgressBar *progress;
			progress = GTK_PROGRESS_BAR(GHB_WIDGET (ud->builder, "progressbar"));
			gtk_progress_bar_set_text (progress, "Scanning ...");
			gtk_progress_bar_set_fraction (progress, 0);
 			update_source_label(ud, device);
			ghb_hb_cleanup(TRUE);
			ghb_backend_scan(device, 0);
		}
		else
		{
			ghb_hb_cleanup(TRUE);
			ghb_backend_scan("/dev/null", 0);
		}
	}
	g_free(device);
}


static gboolean
dbus_init (void)
{
	DBusError error;

	if (dbus_connection != NULL)
		return TRUE;

	dbus_error_init (&error);
	if (!(dbus_connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error))) {
		g_debug ("could not get system bus: %s", error.message);
		dbus_error_free (&error);
		return FALSE;
	}

	//dbus_connection_setup_with_g_main (dbus_connection, NULL);
	//dbus_connection_set_exit_on_disconnect (dbus_connection, FALSE);
	//dbus_connection_add_filter (dbus_connection, gvm_dbus_filter_function, NULL, NULL);

	return TRUE;
}

void
ghb_hal_init()
{
	DBusError error;
	char **devices;
	int nr;

	if (!dbus_init ())
		return;

	if (!(hal_ctx = libhal_ctx_new ())) {
		g_warning ("failed to create a HAL context!");
		return;
	}

	libhal_ctx_set_dbus_connection (hal_ctx, dbus_connection);
	dbus_error_init (&error);
	if (!libhal_ctx_init (hal_ctx, &error)) {
		g_warning ("libhal_ctx_init failed: %s", error.message ? error.message : "unknown");
		dbus_error_free (&error);
		libhal_ctx_free (hal_ctx);
		return;
	}

	/*
	 * Do something to ping the HAL daemon - the above functions will
	 * succeed even if hald is not running, so long as DBUS is.  But we
	 * want to exit silently if hald is not running, to behave on
	 * pre-2.6 systems.
	 */
	if (!(devices = libhal_get_all_devices (hal_ctx, &nr, &error))) {
		g_warning ("seems that HAL is not running: %s", error.message ? error.message : "unknown");
		dbus_error_free (&error);

		libhal_ctx_shutdown (hal_ctx, NULL);
		libhal_ctx_free (hal_ctx);
		return;
	}

	libhal_free_string_array (devices);

	//gvm_hal_claim_branch ("/org/freedesktop/Hal/devices/local");
}

gboolean 
tweak_setting_cb(
	GtkWidget *widget, 
	GdkEventButton *event, 
	signal_user_data_t *ud)
{
	const gchar *name;
	gchar *tweak_name;
	gboolean ret = FALSE;
	gboolean allow_tweaks;

	g_debug("press %d %d", event->type, event->button);
	allow_tweaks = ghb_settings_get_boolean(ud->settings, "allow_tweaks");
	if (allow_tweaks && event->type == GDK_BUTTON_PRESS && event->button == 3)
	{ // Its a right mouse click
		GtkWidget *dialog;
		GtkEntry *entry;
		GtkResponseType response;
		gchar *tweak = NULL;

		name = gtk_widget_get_name(widget);
		if (g_str_has_prefix(name, "tweak_"))
		{
			tweak_name = g_strdup(name);
		}
		else
		{
			tweak_name = g_strdup_printf("tweak_%s", name);
		}

		tweak = ghb_settings_get_string (ud->settings, tweak_name);
		dialog = GHB_WIDGET(ud->builder, "tweak_dialog");
		gtk_window_set_title(GTK_WINDOW(dialog), tweak_name);
		entry = GTK_ENTRY(GHB_WIDGET(ud->builder, "tweak_setting"));
		if (tweak)
		{
			gtk_entry_set_text(entry, tweak);
			g_free(tweak);
		}
		response = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_hide(dialog);
		if (response == GTK_RESPONSE_OK)
		{
			tweak = (gchar*)gtk_entry_get_text(entry);
			if (ghb_validate_filter_string(tweak, -1))
				ghb_settings_set_string(ud->settings, tweak_name, tweak);
			else
			{
				gchar *message;
				message = g_strdup_printf(
							"Invalid Settings:\n%s",
							tweak);
				ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
				g_free(message);
			}
		}
		g_free(tweak_name);
		ret = TRUE;
	}
	return ret;
}

gboolean 
easter_egg_cb(
	GtkWidget *widget, 
	GdkEventButton *event, 
	signal_user_data_t *ud)
{
	g_debug("press %d %d", event->type, event->button);
	if (event->type == GDK_3BUTTON_PRESS && event->button == 1)
	{ // Its a tripple left mouse button click
		GtkWidget *widget;
		widget = GHB_WIDGET(ud->builder, "allow_tweaks");
		gtk_widget_show(widget);
		widget = GHB_WIDGET(ud->builder, "hbfd_feature");
		gtk_widget_show(widget);
	}
	else if (event->type == GDK_BUTTON_PRESS && event->button == 1)
	{
		GtkWidget *widget;
		widget = GHB_WIDGET(ud->builder, "allow_tweaks");
		gtk_widget_hide(widget);
		widget = GHB_WIDGET(ud->builder, "hbfd_feature");
		gtk_widget_hide(widget);
	}
	return FALSE;
}

gboolean
ghb_reload_queue(signal_user_data_t *ud)
{
	GValue *queue;
	gint unfinished = 0;
	gint count, ii;
	gint status;
	GValue *settings;
	gchar *message;

	g_debug("ghb_reload_queue");
	queue = ghb_load_queue();
	// Look for unfinished entries
	count = ghb_array_len(queue);
	for (ii = 0; ii < count; ii++)
	{
		settings = ghb_array_get_nth(queue, ii);
		status = ghb_settings_get_int(settings, "job_status");
		if (status != GHB_QUEUE_DONE && status != GHB_QUEUE_CANCELED)
		{
			unfinished++;
		}
	}
	if (unfinished)
	{
		message = g_strdup_printf(
					"You have %d unfinished jobs in a saved queue.\n\n"
					"Would you like to reload them?",
					unfinished);
		if (ghb_message_dialog(GTK_MESSAGE_QUESTION, message, "No", "Yes"))
		{
			GtkWidget *widget = GHB_WIDGET (ud->builder, "queue_window");
			gtk_widget_show (widget);

			ud->queue = queue;
			// First get rid of any old items we don't want
			for (ii = count-1; ii >= 0; ii--)
			{
				settings = ghb_array_get_nth(queue, ii);
				status = ghb_settings_get_int(settings, "job_status");
				if (status == GHB_QUEUE_DONE || status == GHB_QUEUE_CANCELED)
				{
					GValue *old = ghb_array_get_nth(queue, ii);
					ghb_value_free(old);
					ghb_array_remove(queue, ii);
				}
			}
			count = ghb_array_len(queue);
			for (ii = 0; ii < count; ii++)
			{
				settings = ghb_array_get_nth(queue, ii);
				ghb_settings_set_int(settings, "job_unique_id", 0);
				ghb_settings_set_int(settings, "job_status", GHB_QUEUE_PENDING);
				add_to_queue_list(ud, settings, NULL);
			}
			queue_buttons_grey(ud, FALSE);
		}
		else
		{
			ghb_value_free(queue);
			ghb_remove_queue_file();
		}
		g_free(message);
	}
	return FALSE;
}

