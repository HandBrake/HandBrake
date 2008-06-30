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
#include <glib/gstdio.h>
#include <gio/gio.h>

#include "callbacks.h"
#include "settings.h"
#include "hb-backend.h"
#include "ghb-dvd.h"

extern gboolean ghb_autostart;
static void update_chapter_list(signal_user_data_t *ud);
static void clear_audio_list(signal_user_data_t *ud);
static GList* dvd_device_list();
static gboolean cancel_encode();
static void audio_list_refresh_selected(signal_user_data_t *ud);
static GHashTable* get_selected_asettings(signal_user_data_t *ud);

// This is a dependency map used for greying widgets
// that are dependent on the state of another widget.
// The enable_value comes from the values that are
// obtained from ghb_widget_value().  For combo boxes
// you will have to look further to combo box options
// maps in hb-backend.c
typedef struct
{
	const gchar *widget_name;
	const gchar *dep_name;
	const gchar *enable_value;
	const gboolean disable_if_equal;
} dependency_t;

static dependency_t dep_map[] =
{
	{"title", "queue_add", "none", TRUE},
	{"title", "queue_add_menu", "none", TRUE},
	{"title", "preview_button", "none", TRUE},
	{"title", "show_preview_menu", "none", TRUE},
	{"title", "preview_frame", "none", TRUE},
	{"title", "picture_label", "none", TRUE},
	{"title", "picture_tab", "none", TRUE},
	{"title", "audio_label", "none", TRUE},
	{"title", "audio_tab", "none", TRUE},
	{"title", "chapters_label", "none", TRUE},
	{"title", "chapters_tab", "none", TRUE},
	{"title", "title", "none", TRUE},
	{"title", "start_chapter", "none", TRUE},
	{"title", "end_chapter", "none", TRUE},
	{"vquality_type_bitrate", "video_bitrate", "enable", FALSE},
	{"vquality_type_target", "video_target_size", "enable", FALSE},
	{"vquality_type_constant", "video_quality", "enable", FALSE},
	{"vquality_type_constant", "constant_rate_factor", "enable", FALSE},
	{"two_pass", "turbo", "enable", FALSE},
	{"container", "large_mp4", "mp4|m4v", FALSE},
	{"container", "http_optimize_mp4", "mp4|m4v", FALSE},
	{"container", "ipod_file", "mp4|m4v", FALSE},
	{"container", "variable_frame_rate", "avi", TRUE},
	{"variable_frame_rate", "framerate", "enable", TRUE},
	{"variable_frame_rate", "detelecine", "enable", TRUE},
	{"decomb", "deinterlace", "enable", TRUE},
	{"autocrop", "crop_top", "disable", FALSE},
	{"autocrop", "crop_bottom", "disable", FALSE},
	{"autocrop", "crop_left", "disable", FALSE},
	{"autocrop", "crop_right", "disable", FALSE},
	{"autoscale", "scale_width", "disable", FALSE},
	{"autoscale", "scale_height", "disable", FALSE},
	{"anamorphic", "keep_aspect", "disable", FALSE},
	{"anamorphic", "scale_height", "disable", FALSE},
	{"keep_aspect", "scale_height", "disable", FALSE},
	{"video_codec", "x264_tab", "x264", FALSE},
	{"video_codec", "x264_tab_label", "x264", FALSE},
	{"video_codec", "ipod_file", "x264", FALSE},
	{"audio_track", "audio_add", "none", TRUE},
	{"audio_track", "audio_codec", "none", TRUE},
	{"audio_track", "audio_bitrate", "none", TRUE},
	{"audio_track", "audio_sample_rate", "none", TRUE},
	{"audio_track", "audio_mix", "none", TRUE},
	{"audio_track", "audio_drc", "none", TRUE},
	{"audio_codec", "audio_bitrate", "ac3", TRUE},
	{"audio_codec", "audio_sample_rate", "ac3", TRUE},
	{"audio_codec", "audio_mix", "ac3", TRUE},
	{"audio_codec", "audio_drc", "ac3", TRUE},
	{"x264_bframes", "x264_weighted_bframes", "0", TRUE},
	{"x264_bframes", "x264_brdo", "0", TRUE},
	{"x264_bframes", "x264_bime", "0", TRUE},
	{"x264_bframes", "x264_bpyramid", "<2", TRUE},
	{"x264_bframes", "x264_direct", "0", TRUE},
	{"x264_refs", "x264_mixed_refs", "<2", TRUE},
	{"x264_cabac", "x264_trellis", "enable", FALSE},
	{"x264_subme", "x264_brdo", "<6", TRUE},
	{"x264_analyse", "x264_direct", "none", TRUE},
	{"x264_me", "x264_merange", "umh|esa", FALSE},
	{"pref_audio_codec1", "pref_audio_bitrate1", "none", TRUE},
	{"pref_audio_codec1", "pref_audio_rate1", "none", TRUE},
	{"pref_audio_codec1", "pref_audio_mix1", "none", TRUE},
	{"pref_audio_codec1", "pref_audio_codec2", "none", TRUE},
	{"pref_audio_codec1", "pref_audio_bitrate2", "none", TRUE},
	{"pref_audio_codec1", "pref_audio_rate2", "none", TRUE},
	{"pref_audio_codec1", "pref_audio_mix2", "none", TRUE},
	{"pref_audio_codec2", "pref_audio_bitrate2", "none", TRUE},
	{"pref_audio_codec2", "pref_audio_rate2", "none", TRUE},
	{"pref_audio_codec2", "pref_audio_mix2", "none", TRUE},
	{"chapter_markers", "chapters_list", "enable", FALSE},
};

static gboolean
dep_check(signal_user_data_t *ud, const gchar *name)
{
	GtkWidget *widget;
	GObject *dep_object;
	gchar *value;
	int ii;
	int count = sizeof(dep_map) / sizeof(dependency_t);
	gboolean result = TRUE;
	
	g_debug("dep_check () %s\n", name);
	for (ii = 0; ii < count; ii++)
	{
		if (strcmp(dep_map[ii].dep_name, name) == 0)
		{
			widget = GHB_WIDGET(ud->builder, dep_map[ii].widget_name);
			dep_object = gtk_builder_get_object(ud->builder, dep_map[ii].dep_name);
			value = ghb_widget_short_opt(widget);
			if (dep_object == NULL || widget == NULL)
			{
				g_message("Failed to find widget\n");
			}
			else
			{
				gint jj = 0;
				gchar **values = g_strsplit(dep_map[ii].enable_value, "|", 10);
				gboolean sensitive = FALSE;

				while (values && values[jj])
				{
					if (values[jj][0] == '>')
					{
						gdouble dbl = g_strtod (&values[jj][1], NULL);
						gdouble dvalue = ghb_widget_dbl (widget);
						if (dvalue > dbl)
						{
							sensitive = TRUE;
							break;
						}
					}
					else if (values[jj][0] == '<')
					{
						gdouble dbl = g_strtod (&values[jj][1], NULL);
						gdouble dvalue = ghb_widget_dbl (widget);
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
				sensitive = dep_map[ii].disable_if_equal ^ sensitive;
				if (!sensitive) result = FALSE;
				g_strfreev (values);
			}
			g_free(value);
		}
	}
	return result;
}

static void
check_depencency(signal_user_data_t *ud, GtkWidget *widget)
{
	GObject *dep_object;
	const gchar *name;
	int ii;
	int count = sizeof(dep_map) / sizeof(dependency_t);

	if (ghb_widget_index(widget) < 0) return;
	name = gtk_widget_get_name(widget);
	g_debug("check_depencency () %s\n", name);
	for (ii = 0; ii < count; ii++)
	{
		if (strcmp(dep_map[ii].widget_name, name) == 0)
		{
			gboolean sensitive;

			dep_object = gtk_builder_get_object (ud->builder, dep_map[ii].dep_name);
			if (dep_object == NULL)
			{
				g_message("Failed to find dependent widget %s\n", dep_map[ii].dep_name);
				continue;
			}
			sensitive = dep_check(ud, dep_map[ii].dep_name);
			if (GTK_IS_ACTION(dep_object))
				gtk_action_set_sensitive(GTK_ACTION(dep_object), sensitive);
			else
				gtk_widget_set_sensitive(GTK_WIDGET(dep_object), sensitive);
		}
	}
}

void
ghb_check_all_depencencies(signal_user_data_t *ud)
{
	GObject *dep_object;
	int ii;
	int count = sizeof(dep_map) / sizeof(dependency_t);

	g_debug("ghb_check_all_depencencies ()\n");
	for (ii = 0; ii < count; ii++)
	{
		gboolean sensitive;
		dep_object = gtk_builder_get_object (ud->builder, dep_map[ii].dep_name);
		if (dep_object == NULL)
		{
			g_message("Failed to find dependent widget %s\n", dep_map[ii].dep_name);
			continue;
		}
		sensitive = dep_check(ud, dep_map[ii].dep_name);
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
	g_debug("clear_presets_selection()\n");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	gtk_tree_selection_unselect_all (selection);
}

static gchar*
expand_tilde(const gchar *path)
{
	const gchar *user_home;
	gchar *home;
	const gchar *suffix;
	gchar *expanded_path = NULL;
	
	g_debug("expand_tilde ()\n");
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
	g_debug("on_quit1_activate ()\n");
    if (ud->state & GHB_STATE_WORKING)
    {
        if (cancel_encode("Closing HandBrake will terminate encoding.\n"))
        {
			ghb_hb_cleanup();
	        gtk_main_quit();
            return;
        }
        return;
    }
	ghb_hb_cleanup();
	gtk_main_quit();
}

static void
set_destination(signal_user_data_t *ud)
{
	if (ghb_settings_get_bool(ud->settings, "use_source_name"))
	{
		const gchar *vol_name, *filename, *extension;
		gchar *dir, *new_name;
		
		filename = ghb_settings_get_string(ud->settings, "destination");
		extension = ghb_settings_get_string(ud->settings, "container");
		dir = g_path_get_dirname (filename);
		vol_name = ghb_settings_get_string(ud->settings, "volume_label");
		g_debug("volume_label (%s)\n", vol_name);
		if (vol_name == NULL)
		{
			vol_name = "new_video";
		}
		new_name = g_strdup_printf("%s/%s.%s", dir, vol_name, extension);
		ghb_ui_update(ud, "destination", new_name);
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
	gchar *filename;
	
	g_debug("source_type_changed_cb ()\n");
	if (gtk_toggle_button_get_active (toggle))
	{
		filename = gtk_file_chooser_get_filename (chooser);
		gtk_file_chooser_set_action (chooser, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
		if (filename != NULL)
		{
			gtk_file_chooser_set_filename(chooser, filename);
			g_free(filename);
		}
		gtk_widget_set_sensitive (dvd_device_combo, FALSE);
		gtk_combo_box_set_active (GTK_COMBO_BOX(dvd_device_combo), 0);
	}
	else
	{
		filename = gtk_file_chooser_get_filename (chooser);
		gtk_file_chooser_set_action (chooser, GTK_FILE_CHOOSER_ACTION_OPEN);
		if (filename != NULL)
		{
			gtk_file_chooser_set_filename(chooser, filename);
			g_free(filename);
		}
		gtk_widget_set_sensitive (dvd_device_combo, TRUE);
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
			const gchar *path;
			path = ghb_settings_get_string( ud->settings, "source");
			gtk_progress_bar_set_fraction (progress, 0);
			gtk_progress_bar_set_text (progress, "Scanning ...");
			ud->state |= GHB_STATE_SCANNING;
			ghb_backend_scan (path, 0);
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
	const gchar *sourcename;
	gint	response;
	GtkFileChooserAction action;
	gboolean checkbutton_active;

	g_debug("source_browse_clicked_cb ()\n");
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
				ghb_prefs_save (ud->settings);
				ghb_dvd_set_current (filename, ud);
			}
			g_free(filename);
		}
	}
	gtk_widget_destroy(dialog);
}

void
dvd_source_activate_cb(GtkAction *action, signal_user_data_t *ud)
{
	const gchar *filename;
	const gchar *sourcename;

	sourcename = ghb_settings_get_string(ud->settings, "source");
	filename = gtk_action_get_name(action);
	do_scan(ud, filename);
	if (strcmp(sourcename, filename) != 0)
	{
		ghb_settings_set_string (ud->settings, "default_source", filename);
		ghb_prefs_save (ud->settings);
		ghb_dvd_set_current (filename, ud);
	}
}

static void
update_destination_extension(signal_user_data_t *ud)
{
	static gchar *containers[] = {"mkv", "mp4", "m4v", "avi", "ogm", NULL};
	gchar *filename;
	const gchar *extension;
	gint ii;
	GtkEntry *entry;

	g_debug("update_destination_extension ()\n");
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
			*pos = 0;
			if (strcmp(extension, &pos[1]) == 0)
			{
				// Extension is already correct
				break;
			}
			new_name = g_strjoin(".", filename, extension, NULL); 
			ghb_ui_update(ud, "destination", new_name);
			g_free(new_name);
			break;
		}
	}
	g_free(filename);
}

static gboolean update_default_destination = FALSE;

void
destination_entry_changed_cb(GtkEntry *entry, signal_user_data_t *ud)
{
	gchar *dest;
	
	g_debug("destination_entry_changed_cb ()\n");
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
	const char *destname;
	gchar *basename;

	g_debug("destination_browse_clicked_cb ()\n");
	destname = ghb_settings_get_string(ud->settings, "destination");
	dialog = gtk_file_chooser_dialog_new ("Choose Destination",
                      NULL,
                      GTK_FILE_CHOOSER_ACTION_SAVE,
                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                      NULL);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), destname);
	basename = g_path_get_basename(destname);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), basename);
	g_free(basename);
	if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		entry = (GtkEntry*)GHB_WIDGET(ud->builder, "destination");
		if (entry == NULL)
		{
			g_debug("Failed to find widget: %s\n", "destination");
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
	g_debug("window_destroy_event_cb ()\n");
	ghb_hb_cleanup();
	gtk_main_quit();
    return FALSE;
}

gboolean
window_delete_event_cb(GtkWidget *widget, GdkEvent *event, signal_user_data_t *ud)
{
	g_debug("window_delete_event_cb ()\n");
    if (ud->state & GHB_STATE_WORKING)
    {
        if (cancel_encode("Closing HandBrake will terminate encoding.\n"))
        {
			ghb_hb_cleanup();
	        gtk_main_quit();
            return FALSE;
        }
        return TRUE;
    }
	ghb_hb_cleanup();
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
	g_debug("container_changed_cb ()\n");
	ghb_widget_to_setting(ud->settings, widget);
	update_destination_extension(ud);
	check_depencency(ud, widget);
	update_acodec_combo(ud);
	clear_presets_selection(ud);
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
	text = g_strdup_printf ("%02d:%02d:%02d", tinfo->hours, tinfo->minutes, tinfo->seconds);
	gtk_label_set_text (GTK_LABEL(widget), text);
	g_free(text);
	widget = GHB_WIDGET (ud->builder, "source_dimensions");
	text = g_strdup_printf ("%d x %d", tinfo->width, tinfo->height);
	gtk_label_set_text (GTK_LABEL(widget), text);
	g_free(text);
	widget = GHB_WIDGET (ud->builder, "source_aspect");
	text = get_aspect_string(tinfo->aspect_n, tinfo->aspect_d);
	gtk_label_set_text (GTK_LABEL(widget), text);
	g_free(text);

	widget = GHB_WIDGET (ud->builder, "source_frame_rate");
	text = (gchar*)get_rate_string(tinfo->rate_base, tinfo->rate);
	gtk_label_set_text (GTK_LABEL(widget), text);
	g_free(text);

	ghb_ui_update_int (ud, "scale_width", tinfo->width - tinfo->crop[2] - tinfo->crop[3]);
	// If anamorphic or keep_aspect, the hight will be automatically calculated
	gboolean keep_aspect = ghb_settings_get_bool(ud->settings, "keep_aspect");
	gboolean anamorphic = ghb_settings_get_bool(ud->settings, "anamorphic");
	if (!(keep_aspect || anamorphic))
		ghb_ui_update_int (ud, "scale_height", tinfo->height - tinfo->crop[0] - tinfo->crop[1]);

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
	if (ghb_settings_get_bool (ud->settings, "autocrop"))
	{
		ghb_ui_update_int (ud, "crop_top", tinfo->crop[0]);
		ghb_ui_update_int (ud, "crop_bottom", tinfo->crop[1]);
		ghb_ui_update_int (ud, "crop_left", tinfo->crop[2]);
		ghb_ui_update_int (ud, "crop_right", tinfo->crop[3]);
	}
	g_debug("setting max end chapter %d\n", tinfo->num_chapters);
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
	
	g_debug("adjust_audio_rate_combos ()\n");
	titleindex = ghb_settings_get_index(ud->settings, "title");

	widget = GHB_WIDGET(ud->builder, "audio_track");
	audioindex = ghb_widget_int(widget);

	widget = GHB_WIDGET(ud->builder, "audio_codec");
	acodec = ghb_widget_int(widget);

	if (ghb_get_audio_info (&ainfo, titleindex, audioindex) && ghb_audio_is_passthru (acodec))
	{
		// Set the values for bitrate and samplerate to the input rates
		ghb_set_passthru_rate_opts (ud->builder, ainfo.bitrate);
		ghb_ui_update_int (ud, "audio_bitrate", ainfo.bitrate);
		ghb_ui_update_int (ud, "audio_sample_rate", 0);
		ghb_ui_update_int (ud, "audio_mix", 0);
	}
	else
	{
		ghb_set_default_rate_opts (ud->builder);
	}
}

static void
set_pref_audio(gint titleindex, signal_user_data_t *ud)
{
	gint acodec, track, ivalue;
	const gchar *svalue;
	GtkWidget *button;
	ghb_audio_info_t ainfo;
	gboolean skipped_1st = FALSE;
	
	// Clear the audio list
	clear_audio_list(ud);

	// Find "best" audio based on audio preferences
	button = GHB_WIDGET (ud->builder, "audio_add");
	svalue = ghb_settings_get_short_opt(ud->settings, "pref_source_audio_lang");
	acodec = ghb_settings_get_int(ud->settings, "pref_source_audio_codec");
	track = ghb_find_audio_track(titleindex, svalue, acodec);
	ghb_ui_update_int(ud, "audio_track", track);
	// Get the resulting track, it may not be what was asked for.
	track = ghb_settings_get_int(ud->settings, "audio_track");
	if (track == -1)
	{
		// No audio tracks. Perhaps no source dvd yet
		// Just initialize the audio controls and do not add anything to
		// the audio list
		acodec = ghb_settings_get_int(ud->settings, "pref_audio_codec1");
		ghb_ui_update_int(ud, "audio_codec", acodec);
		ivalue = ghb_settings_get_int(ud->settings, "pref_audio_bitrate1");
		ghb_ui_update_int(ud, "audio_bitrate", ivalue);
		ivalue = ghb_settings_get_int(ud->settings, "pref_audio_rate1");
		ghb_ui_update_int(ud, "audio_sample_rate", ivalue);
		ivalue = ghb_settings_get_int(ud->settings, "pref_audio_mix1");
		ghb_ui_update_int(ud, "audio_mix", ivalue);
		svalue = ghb_settings_get_string(ud->settings, "pref_audio_drc");
		ghb_ui_update(ud, "audio_drc", svalue);
		return;
	}
	acodec = ghb_settings_get_int(ud->settings, "pref_audio_codec1");
	// Check to see if:
	// 1. pref codec is ac3
	// 2. source codec is not ac3
	// 3. 2nd pref is enabled
	if (ghb_get_audio_info (&ainfo, titleindex, track) && ghb_audio_is_passthru (acodec))
	{
		if (!ghb_audio_is_passthru(ainfo.codec))
		{
			acodec = ghb_get_default_acodec();
			if (ghb_settings_get_int(ud->settings, "pref_audio_codec2") != 0)
			{
				// Skip first pref audio
				acodec = 0;
				skipped_1st = TRUE;
			}
		}
	}
	ghb_ui_update_int(ud, "audio_codec", acodec);
	if (!ghb_audio_is_passthru (acodec))
	{
		// This gets set autimatically if the codec is passthru
		ivalue = ghb_settings_get_int(ud->settings, "pref_audio_bitrate1");
		ghb_ui_update_int(ud, "audio_bitrate", ivalue);
		ivalue = ghb_settings_get_int(ud->settings, "pref_audio_rate1");
		ghb_ui_update_int(ud, "audio_sample_rate", ivalue);
		ivalue = ghb_settings_get_int(ud->settings, "pref_audio_mix1");
		ivalue = ghb_get_best_mix(titleindex, track, acodec, ivalue);
		ghb_ui_update_int(ud, "audio_mix", ivalue);
	}
	svalue = ghb_settings_get_string(ud->settings, "pref_audio_drc");
	ghb_ui_update(ud, "audio_drc", svalue);
	if (acodec != 0) // 0 is none
	{
		// Add to audio list
		g_signal_emit_by_name(button, "clicked", ud);
	}
	acodec = ghb_settings_get_int(ud->settings, "pref_audio_codec2");
	// Check to see if:
	// 1. pref codec is ac3
	// 2. source codec is not ac3
	if (ghb_audio_is_passthru (acodec))
	{
		if (!ghb_audio_is_passthru(ainfo.codec) && skipped_1st)
		{
			acodec = ghb_get_default_acodec();
		}
		else
		{
			acodec = 0;
		}
	}
	if (acodec != 0)
	{
		ghb_ui_update_int(ud, "audio_codec", acodec);
		// Do the second prefs track
		if (!ghb_audio_is_passthru (acodec))
		{
			ivalue = ghb_settings_get_int(ud->settings, "pref_audio_bitrate2");
			ghb_ui_update_int(ud, "audio_bitrate", ivalue);
			ivalue = ghb_settings_get_int(ud->settings, "pref_audio_rate2");
			ghb_ui_update_int(ud, "audio_sample_rate", ivalue);
			ivalue = ghb_settings_get_int(ud->settings, "pref_audio_mix2");
			ivalue = ghb_get_best_mix(titleindex, track, acodec, ivalue);
			ghb_ui_update_int(ud, "audio_mix", ivalue);
		}
		g_signal_emit_by_name(button, "clicked", ud);
	}
}

static gint preview_button_width;
static gint preview_button_height;
static gboolean update_preview = FALSE;

static void
set_preview_image(signal_user_data_t *ud)
{
	GtkWidget *widget;
	gint preview_width, preview_height, target_height, width, height;

	g_debug("set_preview_button_image ()\n");
	gint titleindex = ghb_settings_get_int(ud->settings, "title");
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
	
	g_debug("preview %d x %d\n", preview_width, preview_height);
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
	const gchar *preset;
	
	g_debug("title_changed_cb ()\n");
	ghb_widget_to_setting(ud->settings, widget);
	check_depencency(ud, widget);

	titleindex = ghb_settings_get_int(ud->settings, "title");
	ghb_update_ui_combo_box (ud->builder, "audio_track", titleindex, FALSE);
	ghb_update_ui_combo_box (ud->builder, "subtitle_lang", titleindex, FALSE);
	preset = ghb_settings_get_string (ud->settings, "preset");
	ghb_update_from_preset(ud, preset, "subtitle_lang");
	if (ghb_get_title_info (&tinfo, titleindex))
	{
		show_title_info(ud, &tinfo);
	}
	update_chapter_list (ud);
	adjust_audio_rate_combos(ud);
	set_pref_audio(titleindex, ud);
	if (ghb_settings_get_bool (ud->settings, "vquality_type_target"))
	{
		gint bitrate = ghb_calculate_target_bitrate (ud->settings, titleindex);
		ghb_ui_update_int (ud, "video_bitrate", bitrate);
	}

	// Unfortunately, there is no way to query how many frames were
	// actually generated during the scan.  It attempts to make 10.
	// If I knew how many were generated, I would adjust the spin
	// control range here.
	ghb_ui_update_int (ud, "preview_frame", 1);

	set_preview_image (ud);
}

void
audio_codec_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	static gint prev_acodec = 0;
	gint acodec, ivalue;
	GHashTable *asettings;
	
	g_debug("audio_codec_changed_cb ()\n");
	
	acodec = ghb_widget_int(widget);
	if (ghb_audio_is_passthru (prev_acodec) && !ghb_audio_is_passthru (acodec))
	{
		// Transition from passthru to not, put some audio settings back to 
		// pref settings
		gint titleindex = ghb_settings_get_int(ud->settings, "title");
		gint track = ghb_settings_get_int(ud->settings, "audio_track");

		ivalue = ghb_settings_get_int(ud->settings, "pref_audio_bitrate1");
		ghb_ui_update_int (ud, "audio_bitrate", ivalue);
		ivalue = ghb_settings_get_int(ud->settings, "pref_audio_rate1");
		ghb_ui_update_int (ud, "audio_sample_rate", ivalue);
		ivalue = ghb_settings_get_int(ud->settings, "pref_audio_mix1");
		ivalue = ghb_get_best_mix(titleindex, track, acodec, ivalue);
		ghb_ui_update_int (ud, "audio_mix", ivalue);
	}
	adjust_audio_rate_combos(ud);
	ghb_grey_combo_options (ud->builder);
	check_depencency(ud, widget);
	prev_acodec = acodec;
	asettings = get_selected_asettings(ud);
	if (asettings != NULL)
	{
		ghb_widget_to_setting(asettings, widget);
		audio_list_refresh_selected(ud);
	}
}

static void audio_list_refresh_selected(signal_user_data_t *ud);
static GHashTable* get_selected_asettings(signal_user_data_t *ud);

void
audio_track_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	GHashTable *asettings;

	g_debug("audio_track_changed_cb ()\n");
	adjust_audio_rate_combos(ud);
	check_depencency(ud, widget);
	ghb_grey_combo_options(ud->builder);
	asettings = get_selected_asettings(ud);
	if (asettings != NULL)
	{
		ghb_widget_to_setting(asettings, widget);
		audio_list_refresh_selected(ud);
	}
}

void
audio_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	GHashTable *asettings;

	g_debug("audio_widget_changed_cb ()\n");
	check_depencency(ud, widget);
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
	g_debug("generic_widget_changed_cb ()\n");
	check_depencency(ud, widget);
}

void
setting_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	ghb_widget_to_setting(ud->settings, widget);
	check_depencency(ud, widget);
	clear_presets_selection(ud);
}

void
vfr_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	//const gchar *name = gtk_widget_get_name(widget);
	//g_debug("setting_widget_changed_cb () %s\n", name);
	ghb_widget_to_setting(ud->settings, widget);
	check_depencency(ud, widget);
	clear_presets_selection(ud);
	if (ghb_settings_get_bool(ud->settings, "variable_frame_rate"))
	{
		ghb_ui_update_int(ud, "framerate", 0);
	}
}

void
mirror_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	const gchar *name = gtk_widget_get_name(widget);
	if (name == NULL) return;
	
	g_debug("mirror_cb () %s\n", name);
	gchar *mirror = g_strdup(name);
	gchar *pos = g_strrstr(mirror, "_mirror");
	if (pos == NULL) return;
	*pos = 0;
	gchar *value = ghb_widget_short_opt (widget);
	ghb_ui_update (ud, mirror, value);
	g_free(mirror);
}

// subtitles have their differ from other settings in that
// the selection is updated automaitcally when the title
// changes.  I don't want the preset selection changed as
// would happen for regular settings.
void
subtitle_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	const gchar *name = gtk_widget_get_name(widget);
	g_debug("subtitle_changed_cb () %s\n", name);
	ghb_widget_to_setting(ud->settings, widget);
	check_depencency(ud, widget);
}

void
target_size_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	const gchar *name = gtk_widget_get_name(widget);
	g_debug("setting_widget_changed_cb () %s\n", name);
	ghb_widget_to_setting(ud->settings, widget);
	check_depencency(ud, widget);
	clear_presets_selection(ud);
	if (ghb_settings_get_bool (ud->settings, "vquality_type_target"))
	{
		gint titleindex = ghb_settings_get_int(ud->settings, "title");
		gint bitrate = ghb_calculate_target_bitrate (ud->settings, titleindex);
		ghb_ui_update_int (ud, "video_bitrate", bitrate);
	}
}

void
start_chapter_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	const gchar *name = gtk_widget_get_name(widget);
	g_debug("start_chapter_changed_cb () %s\n", name);
	ghb_widget_to_setting(ud->settings, widget);
	GtkWidget *end_ch = GHB_WIDGET (ud->builder, "end_chapter");
	gdouble start, end;
	gtk_spin_button_get_range (GTK_SPIN_BUTTON(end_ch), &start, &end);
	start = ghb_settings_get_int(ud->settings, "start_chapter");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(end_ch), start, end);
	check_depencency(ud, widget);
}

void
end_chapter_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	const gchar *name = gtk_widget_get_name(widget);
	g_debug("end_chapter_changed_cb () %s\n", name);
	ghb_widget_to_setting(ud->settings, widget);
	GtkWidget *start_ch = GHB_WIDGET (ud->builder, "start_chapter");
	gdouble start, end;
	gtk_spin_button_get_range (GTK_SPIN_BUTTON(start_ch), &start, &end);
	end = ghb_settings_get_int(ud->settings, "end_chapter");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON(start_ch), start, end);
	check_depencency(ud, widget);
}

void
scale_width_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("scale_width_changed_cb ()\n");
	ghb_widget_to_setting(ud->settings, widget);
	check_depencency(ud, widget);
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
	g_debug("scale_height_changed_cb ()\n");
	ghb_widget_to_setting(ud->settings, widget);
	check_depencency(ud, widget);
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
	
	g_debug("crop_changed_cb ()\n");
	ghb_widget_to_setting(ud->settings, widget);
	check_depencency(ud, widget);
	ghb_set_scale (ud, GHB_SCALE_KEEP_NONE);

	crop[0] = ghb_settings_get_int(ud->settings, "crop_top");
	crop[1] = ghb_settings_get_int(ud->settings, "crop_bottom");
	crop[2] = ghb_settings_get_int(ud->settings, "crop_left");
	crop[3] = ghb_settings_get_int(ud->settings, "crop_right");
	titleindex = ghb_settings_get_index(ud->settings, "title");
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
	g_debug("scale_changed_cb ()\n");
	ghb_widget_to_setting(ud->settings, widget);
	check_depencency(ud, widget);
	clear_presets_selection(ud);
	ghb_set_scale (ud, GHB_SCALE_KEEP_NONE);
	update_preview = TRUE;
	
	gchar *text;
	
	text = ghb_settings_get_bool(ud->settings, "autocrop") ? "On" : "Off";
	widget = GHB_WIDGET (ud->builder, "crop_auto");
	gtk_label_set_text (GTK_LABEL(widget), text);
	text = ghb_settings_get_bool(ud->settings, "autoscale") ? "On" : "Off";
	widget = GHB_WIDGET (ud->builder, "scale_auto");
	gtk_label_set_text (GTK_LABEL(widget), text);
	text = ghb_settings_get_bool(ud->settings, "anamorphic") ? "On" : "Off";
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
	g_debug("generic_entry_changed_cb ()\n");
	if (!GTK_WIDGET_HAS_FOCUS((GtkWidget*)entry))
	{
		ghb_widget_to_setting(ud->settings, (GtkWidget*)entry);
	}
}

gboolean
generic_focus_out_cb(GtkWidget *widget, GdkEventFocus *event, 
    signal_user_data_t *ud)
{
	g_debug("generic_focus_out_cb ()\n");
	ghb_widget_to_setting(ud->settings, widget);
	return FALSE;
}

static void
clear_audio_list(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkListStore *store;
	GSList *link;
	
	g_debug("clear_audio_list ()\n");
	while (ud->audio_settings != NULL)
	{
		link = ud->audio_settings;
		ud->audio_settings = g_slist_remove_link(ud->audio_settings, link);
		g_hash_table_destroy((GHashTable*)link->data);
		g_slist_free_1(link);
	}
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	gtk_list_store_clear (store);
}

static void
add_to_audio_list(signal_user_data_t *ud, GHashTable *settings)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	GtkTreeSelection *selection;
	
	g_debug("add_to_audio_list ()\n");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	selection = gtk_tree_view_get_selection (treeview);
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
		// These are displayed in list
		0, ghb_settings_get_option(settings, "audio_track"),
		1, ghb_settings_get_option(settings, "audio_codec"),
		2, ghb_settings_get_option(settings, "audio_bitrate"),
		3, ghb_settings_get_option(settings, "audio_sample_rate"),
		4, ghb_settings_get_option(settings, "audio_mix"),
		// These are used to set combo box values when a list item is selected
		5, ghb_settings_get_string(settings, "audio_drc"),
		6, ghb_settings_get_short_opt(settings, "audio_track"),
		7, ghb_settings_get_short_opt(settings, "audio_codec"),
		8, ghb_settings_get_short_opt(settings, "audio_bitrate"),
		9, ghb_settings_get_short_opt(settings, "audio_sample_rate"),
		10, ghb_settings_get_short_opt(settings, "audio_mix"),
		-1);
	gtk_tree_selection_select_iter(selection, &iter);
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
	GSList *link;
	GHashTable *asettings = NULL;
	
	g_debug("get_selected_asettings ()\n");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
        // Get the row number
		treepath = gtk_tree_model_get_path (store, &iter);
		indices = gtk_tree_path_get_indices (treepath);
		g_free(treepath);
		row = indices[0];
		// find audio settings
		if (row < 0) return;
		link = g_slist_nth(ud->audio_settings, row);
		if (link == NULL) return;
		asettings = (GHashTable*)link->data;
		gtk_list_store_set(GTK_LIST_STORE(store), &iter, 
			// These are displayed in list
			0, ghb_settings_get_option(asettings, "audio_track"),
			1, ghb_settings_get_option(asettings, "audio_codec"),
			2, ghb_settings_get_option(asettings, "audio_bitrate"),
			3, ghb_settings_get_option(asettings, "audio_sample_rate"),
			4, ghb_settings_get_option(asettings, "audio_mix"),
			// These are used to set combo box values when a list item is selected
			5, ghb_settings_get_string(asettings, "audio_drc"),
			6, ghb_settings_get_short_opt(asettings, "audio_track"),
			7, ghb_settings_get_short_opt(asettings, "audio_codec"),
			8, ghb_settings_get_short_opt(asettings, "audio_bitrate"),
			9, ghb_settings_get_short_opt(asettings, "audio_sample_rate"),
			10, ghb_settings_get_short_opt(asettings, "audio_mix"),
			-1);
	}
}

static GHashTable*
get_selected_asettings(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gint *indices;
	gint row;
	GSList *link;
	GHashTable *asettings = NULL;
	
	g_debug("get_selected_asettings ()\n");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
        // Get the row number
		treepath = gtk_tree_model_get_path (store, &iter);
		indices = gtk_tree_path_get_indices (treepath);
		g_free(treepath);
		row = indices[0];
		// find audio settings
		if (row < 0) return NULL;
		link = g_slist_nth(ud->audio_settings, row);
		if (link == NULL) return NULL;
		asettings = (GHashTable*)link->data;
	}
	return asettings;
}

void
audio_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkWidget *widget;
	
	g_debug("audio_list_selection_changed_cb ()\n");
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		const gchar *track, *codec, *bitrate, *sample_rate, *mix, *drc;
		gtk_tree_model_get(store, &iter,
						   6, &track,
						   7, &codec,
						   8, &bitrate,
						   9, &sample_rate,
						   10, &mix,
						   5, &drc,
						   -1);
		ghb_ui_update(ud, "audio_track", track);
		ghb_ui_update(ud, "audio_codec", codec);
		ghb_ui_update(ud, "audio_bitrate", bitrate);
		ghb_ui_update(ud, "audio_sample_rate", sample_rate);
		ghb_ui_update(ud, "audio_mix", mix);
		ghb_ui_update(ud, "audio_drc", drc);
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
	GHashTable *asettings;
	GtkWidget *widget;
	gint count;
	
	g_debug("audio_add_clicked_cb ()\n");
	// Only allow up to 8 audio entries
	asettings = ghb_settings_new();
	widget = GHB_WIDGET(ud->builder, "audio_track");
	ghb_settings_set(asettings,	"audio_track", ghb_widget_value(widget));
	widget = GHB_WIDGET(ud->builder, "audio_codec");
	ghb_settings_set(asettings,	"audio_codec", ghb_widget_value(widget));
	widget = GHB_WIDGET(ud->builder, "audio_bitrate");
	ghb_settings_set(asettings,	"audio_bitrate", ghb_widget_value(widget));
	widget = GHB_WIDGET(ud->builder, "audio_sample_rate");
	ghb_settings_set(asettings,	"audio_sample_rate", ghb_widget_value(widget));
	widget = GHB_WIDGET(ud->builder, "audio_mix");
	ghb_settings_set(asettings,	"audio_mix", ghb_widget_value(widget));
	widget = GHB_WIDGET(ud->builder, "audio_drc");
	ghb_settings_set(asettings,	"audio_drc", ghb_widget_value(widget));

	ud->audio_settings = g_slist_append(ud->audio_settings, asettings);
	add_to_audio_list(ud, asettings);
	count = g_slist_length(ud->audio_settings);
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
	GSList *link;

	g_debug("audio_remove_clicked_cb ()\n");
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
		g_free(treepath);
		row = indices[0];
		// Remove the selected item
		gtk_list_store_remove (GTK_LIST_STORE(store), &iter);
		// remove from audio settings list
		if (row < 0) return;
		link = g_slist_nth(ud->audio_settings, row);
		if (link == NULL) return;
		ud->audio_settings = g_slist_remove_link(ud->audio_settings, link);
		g_hash_table_destroy((GHashTable*)link->data);
		g_slist_free_1(link);
		widget = GHB_WIDGET (ud->builder, "audio_add");
		gtk_widget_set_sensitive(widget, TRUE);
	}
}

static void
audio_list_refresh(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean done;
	
	g_debug("audio_list_refresh ()\n");
	GSList *link = ud->audio_settings;
	if (link == NULL) return;
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
	{
		do
		{
			GHashTable *asettings;

			asettings = (GHashTable*)link->data;
			gtk_list_store_set(GTK_LIST_STORE(store), &iter, 
				// These are displayed in list
				0, ghb_settings_get_option(asettings, "audio_track"),
				1, ghb_settings_get_option(asettings, "audio_codec"),
				2, ghb_settings_get_option(asettings, "audio_bitrate"),
				3, ghb_settings_get_option(asettings, "audio_sample_rate"),
				4, ghb_settings_get_option(asettings, "audio_mix"),
				// These are used to set combo box values when a list item is selected
				5, ghb_settings_get_string(asettings, "audio_drc"),
				6, ghb_settings_get_short_opt(asettings, "audio_track"),
				7, ghb_settings_get_short_opt(asettings, "audio_codec"),
				8, ghb_settings_get_short_opt(asettings, "audio_bitrate"),
				9, ghb_settings_get_short_opt(asettings, "audio_sample_rate"),
				10, ghb_settings_get_short_opt(asettings, "audio_mix"),
				-1);
			done = !gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
			link = link->next;
		} while (!done && link);
	}
}

void
ghb_presets_list_update(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean done;
	gint ii = 0;
	gint index;
	gchar **presets;
	gchar **descriptions;
	gint flags, custom, def;
	
	g_debug("ghb_presets_list_update ()\n");
	presets = ghb_presets_get_names();
	descriptions = ghb_presets_get_descriptions();
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
	{
		do
		{
			if ((presets != NULL) && (presets[ii] != NULL))
			{
				// Update row with settings data
				g_debug("Updating row\n");
				flags = ghb_preset_flags(presets[ii], &index);
				def = flags & PRESET_DEFAULT;
				custom = flags & PRESET_CUSTOM;
				gtk_list_store_set(store, &iter, 
							0, presets[ii], 
							1, def ? 800 : 400, 
							2, def ? 2 : 0,
						   	3, custom ? "black" : "blue", 
							4, descriptions[ii],
							-1);
				ii++;
				done = !gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
			}
			else
			{
				// No more settings data, remove row
				g_debug("Removing row\n");
				done = !gtk_list_store_remove(store, &iter);
			}
		} while (!done);
	}
	while ((presets != NULL) && (presets[ii] != NULL))
	{
		// Additional settings, add row
		g_debug("Adding row %s\n", presets[ii]);
		gtk_list_store_append(store, &iter);
		flags = ghb_preset_flags(presets[ii], &index);
		def = flags & PRESET_DEFAULT;
		custom = flags & PRESET_CUSTOM;
		gtk_list_store_set(store, &iter, 0, presets[ii], 
						   	1, def ? 800 : 400, 
						   	2, def ? 2 : 0,
						   	3, custom ? "black" : "blue", 
							4, descriptions[ii],
						   	-1);
		ii++;
	}
	g_strfreev (presets);
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
	
	g_debug("select_preset()\n");
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
				break;
			}
			done = !gtk_tree_model_iter_next(store, &iter);
		} while (!done);
	}
}

void
presets_save_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GtkWidget *dialog;
	GtkEntry *entry;
	GtkTextView *desc;
	GtkResponseType response;
	const gchar *preset = "";

	g_debug("presets_save_clicked_cb ()\n");
	preset = ghb_settings_get_string (ud->settings, "preset");
	// Clear the description
	desc = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "preset_description"));
	//gtk_entry_set_text(desc, "");
	dialog = GHB_WIDGET(ud->builder, "preset_save_dialog");
	entry = GTK_ENTRY(GHB_WIDGET(ud->builder, "preset_name"));
	gtk_entry_set_text(entry, preset);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
	if (response == GTK_RESPONSE_OK)
	{
		// save the preset
		const gchar *name = gtk_entry_get_text(entry);
		g_debug("description to settings\n");
		ghb_widget_to_setting(ud->settings, GTK_WIDGET(desc));
		ghb_settings_save(ud, name);
		ghb_presets_list_update(ud);
		// Make the new preset the selected item
		ghb_select_preset(ud->builder, name);
	}
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

	g_debug("presets_remove_clicked_cb ()\n");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		GtkWidget *dialog;

		gtk_tree_model_get(store, &iter, 0, &preset, -1);
		if (!ghb_presets_is_standard(preset))
		{
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
				ghb_presets_remove(ud->settings, preset);
				ghb_presets_list_update(ud);
				ghb_select_preset(ud->builder, nextPreset);
			}
		}
		else
		{
			dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
									GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
									"Can not delete standard preset %s.", preset);
			response = gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy (dialog);
		}
	}
}

static void
preset_update_title_deps(signal_user_data_t *ud, ghb_title_info_t *tinfo)
{
	GtkWidget *widget;

	ghb_ui_update_int (ud, "scale_width", tinfo->width - tinfo->crop[2] - tinfo->crop[3]);
	// If anamorphic or keep_aspect, the hight will be automatically calculated
	gboolean keep_aspect = ghb_settings_get_bool(ud->settings, "keep_aspect");
	gboolean anamorphic = ghb_settings_get_bool(ud->settings, "anamorphic");
	if (!(keep_aspect || anamorphic))
		ghb_ui_update_int (ud, "scale_height", tinfo->height - tinfo->crop[0] - tinfo->crop[1]);

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
	if (ghb_settings_get_bool (ud->settings, "autocrop"))
	{
		ghb_ui_update_int (ud, "crop_top", tinfo->crop[0]);
		ghb_ui_update_int (ud, "crop_bottom", tinfo->crop[1]);
		ghb_ui_update_int (ud, "crop_left", tinfo->crop[2]);
		ghb_ui_update_int (ud, "crop_right", tinfo->crop[3]);
	}
}

void
presets_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	gchar *preset;
	GtkWidget *widget;
	gboolean sensitive = FALSE;
	ghb_title_info_t tinfo;
	
	g_debug("presets_list_selection_changed_cb ()\n");
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		gtk_tree_model_get(store, &iter, 0, &preset, -1);
		if (!ghb_presets_is_standard(preset))
		{
			sensitive = TRUE;
		}
		ud->dont_clear_presets = TRUE;
		ghb_set_preset(ud, preset);
		gint titleindex = ghb_settings_get_int(ud->settings, "title");
		set_pref_audio(titleindex, ud);
		ud->dont_clear_presets = FALSE;
		if (ghb_get_title_info (&tinfo, titleindex))
		{
			preset_update_title_deps(ud, &tinfo);
		}
		ghb_set_scale (ud, GHB_SCALE_KEEP_NONE);
	}
	else
	{
		g_debug("No selection???  Perhaps unselected.\n");
	}
	widget = GHB_WIDGET (ud->builder, "presets_remove");
	gtk_widget_set_sensitive(widget, sensitive);
}

void
queue_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
	GtkTreeModel *store;
	GtkTreeIter iter, piter;
	
	g_debug("queue_list_selection_changed_cb ()\n");
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
			g_free(path);
		}
	}
}

static void
add_to_queue_list(signal_user_data_t *ud, job_settings_t *item)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkTreeStore *store;
	gchar *info;
	gint num_pass = 1;
	gint ii;
	GtkTreeIter citer;
	
	g_debug("update_queue_list ()\n");
	if (item == NULL) return;
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
	store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
		
	gint title = ghb_settings_get_int(item->settings, "title");
	gint start_chapter = ghb_settings_get_int(item->settings, "start_chapter");
	gint end_chapter = ghb_settings_get_int(item->settings, "end_chapter");
	gboolean pass2 = ghb_settings_get_bool(item->settings, "two_pass");
	const gchar *vol_name = ghb_settings_get_string(item->settings, "volume_label");
	if (vol_name == NULL)
		vol_name = "No Title";
	info = g_strdup_printf 
		(
		 "<big><b>%s</b></big> (Title %d, Chapters %d through %d, %d Video %s)",
		 vol_name, title+1, start_chapter, end_chapter, 
		 pass2 ? 2:1, pass2 ? "Passes":"Pass");

	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, "hb-queue-job", 1, info, 2, "hb-queue-delete", -1);
	g_free(info);
	
	const gchar *vcodec = ghb_settings_get_option(item->settings, "video_codec");
	const gchar *container = ghb_settings_get_option(item->settings, "container");
	const gchar *acodec = ghb_settings_get_option(item->settings, "audio_codec");
	const gchar *dest = ghb_settings_get_string(item->settings, "destination");
	const gchar *preset = ghb_settings_get_string(item->settings, "preset");
	info = g_strdup_printf 
		(
		 "<b>Preset:</b> %s\n"
		 "<b>Format:</b> %s Container, %s Video + %s Audio\n"
		 "<b>Destination:</b> %s",
		 preset, container, vcodec, acodec, dest);

	gtk_tree_store_append(store, &citer, &iter);
	gtk_tree_store_set(store, &citer, 1, info, -1);
	g_free(info);

	gint width = ghb_settings_get_int(item->settings, "scale_width");
	gint height = ghb_settings_get_int(item->settings, "scale_height");
	gboolean anamorphic = ghb_settings_get_bool(item->settings, "anamorphic");
	gboolean round_dim = ghb_settings_get_bool(item->settings, "round_dimensions");
	gboolean keep_aspect = ghb_settings_get_bool(item->settings, "keep_aspect");
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
	gboolean vqtype = ghb_settings_get_bool(item->settings, "vquality_type_constant");
	gint vqvalue = 0;
	gchar *vq_desc = "Error";
	if (!vqtype)
	{
		vqtype = ghb_settings_get_bool(item->settings, "vquality_type_target");
		if (!vqtype)
		{
			// Has to be bitrate
			vqvalue = ghb_settings_get_int(item->settings, "video_bitrate");
			vq_desc = "kbps";
		}
		else
		{
			// Target file size
			vqvalue = ghb_settings_get_int(item->settings, "video_target");
			vq_desc = "MB";
		}
	}
	else
	{
		// Constant quality
		vqvalue = ghb_settings_get_int(item->settings, "video_quality");
		vq_desc = "% Constant Quality";
	}
	const gchar *fps = ghb_settings_get_string(item->settings, "framerate");
	const gchar *vcodec_abbr = ghb_settings_get_short_opt(item->settings, "video_codec");
	gchar *extra_opts;
	if (strcmp(vcodec_abbr, "x264") == 0)
	{
		gchar *x264opts = ghb_build_x264opts_string(item->settings);
		g_debug("xopts (%s)\n", x264opts);
		extra_opts = g_strdup_printf ("\n<b>x264 Options:</b> %s", x264opts);
		g_free(x264opts);
	}
	else
	{
		extra_opts = g_strdup("");
	}
	gboolean turbo = ghb_settings_get_bool (item->settings, "turbo");
	gchar *turbo_desc = "\n<b>Turbo:</b> Off";;
	if (turbo)
	{
		turbo_desc = "\n<b>Turbo:</b> On";
	}
	num_pass = pass2 ? 2 : 1;
	for (ii = 0; ii < num_pass; ii++)
	{
		gboolean final = (ii == (num_pass - 1));
		GString *pass = g_string_new("");
		g_string_append_printf( pass,
			"<b>%s Pass</b>\n"
			"<b>Picture:</b> %d x %d %s\n"
			"<b>Video:</b> %s, %d %s, %s fps"
			"%s",
			 ii ? "2nd":"1st", width, height, aspect_desc,
			 vcodec, vqvalue, vq_desc, fps, 
			 final ? extra_opts : turbo_desc);

		if (final)
		{
			// Add the audios
			GSList *link = item->audio_settings;
			while (link)
			{
				GHashTable *asettings = (GHashTable*)link->data;
				const gchar *acodec = ghb_settings_get_option(asettings, "audio_codec");
				const gchar *bitrate = ghb_settings_get_string(asettings, "audio_bitrate");
				const gchar *samplerate = ghb_settings_get_string(asettings, "audio_sample_rate");
				gint track = ghb_settings_get_int(asettings, "audio_track");
				const gchar *mix = ghb_settings_get_option(asettings, "audio_mix");
				g_string_append_printf(pass,
					"\n<b>Audio:</b> %s, %s kbps, %s kHz, Track %d: %s",
					 acodec, bitrate, samplerate, track+1, mix);
				link = link->next;
			}
		}
		info = g_string_free(pass, FALSE);
		gtk_tree_store_append(store, &citer, &iter);
		gtk_tree_store_set(store, &citer, 0, ii ? "hb-queue-pass2" : "hb-queue-pass1", 1, info,	-1);
		g_free(info);
	}
	g_free(extra_opts);
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
	gint titleindex = ghb_settings_get_int(ud->settings, "title");
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
	gchar *message;
	gint titleindex = ghb_settings_get_int(ud->settings, "title");
	if (titleindex < 0) return FALSE;
	const gchar *dest = ghb_settings_get_string(ud->settings, "destination");
	GSList *link = ud->queue;
	while (link != NULL)
	{
		job_settings_t *item;
		const gchar *filename;
		item = (job_settings_t*)link->data;
		filename = ghb_settings_get_string(item->settings, "destination");
		if (strcmp(dest, filename) == 0)
		{
			message = g_strdup_printf(
						"Destination: %s\n\n"
						"Another queued job has specified the same destination.\n"
						"Do you want to overwrite?",
						dest);
			if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message, "Cancel", "Overwrite"))
			{
				g_free(message);
				return FALSE;
			}
			g_free(message);
			break;
		}
		link = link->next;
	}
	gchar *destdir = g_path_get_dirname(dest);
	if (!g_file_test(destdir, G_FILE_TEST_IS_DIR))
	{
		message = g_strdup_printf(
					"Destination: %s\n\n"
					"This is not a valid directory.",
					destdir);
		ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
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
			g_free(message);
			return FALSE;
		}
		g_free(message);
		g_unlink(dest);
	}
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
	audio_list_refresh(ud);
	return TRUE;
}

static gboolean
queue_add(signal_user_data_t *ud)
{
	// Add settings to the queue
	job_settings_t *queue_item;
	GSList *link;
	static gint unique_id = 0;
	
	g_debug("queue_add ()\n");
	if (!validate_settings(ud))
	{
		return FALSE;
	}
	// Make a copy of current settings to be used for the new job
	queue_item = g_malloc(sizeof(job_settings_t));
	queue_item->settings = ghb_settings_dup(ud->settings);
	queue_item->audio_settings = NULL;
	link = ud->audio_settings;
	while (link != NULL)
	{
		GHashTable *asettings;
		asettings = ghb_settings_dup((GHashTable*)link->data);
		queue_item->audio_settings =
			g_slist_append(queue_item->audio_settings, asettings);
		link = g_slist_next(link);
	}
	queue_item->chapter_list = g_strdupv(ud->chapter_list);
	ud->queue = g_slist_append(ud->queue, queue_item);
	add_to_queue_list(ud, queue_item);
	ghb_add_job (queue_item, unique_id);
	queue_item->unique_id = unique_id;
	queue_item->status = GHB_QUEUE_PENDING;
	unique_id++;
	return TRUE;
}

void
queue_add_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	g_debug("queue_add_clicked_cb ()\n");
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
	GSList *link;
	gint *indices;
	job_settings_t *queue_item;
	gint unique_id;

	g_debug("queue_remove_clicked_cb ()\n");
	g_debug("ud %p\n", ud);
	g_debug("ud->builder %p\n", ud->builder);

	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
	store = gtk_tree_view_get_model(treeview);
	treepath = gtk_tree_path_new_from_string (path);
	if (gtk_tree_model_get_iter(store, &iter, treepath))
	{
		// Find the entry in the queue
		indices = gtk_tree_path_get_indices (treepath);
		row = indices[0];
		// Can only free the treepath After getting what I need from
		// indices since this points into treepath somewhere.
		gtk_tree_path_free (treepath);
		if (row < 0) return;
		link = g_slist_nth(ud->queue, row);
		if (link == NULL) return;
		queue_item = (job_settings_t*)link->data;
		if (queue_item->status == GHB_QUEUE_RUNNING)
		{
			// Ask if wants to stop encode.
			if (!cancel_encode(NULL))
			{
				return;
			}
		}
		// Remove the selected item
		g_debug(" should be removing from treestore\n");
		gtk_tree_store_remove(GTK_TREE_STORE(store), &iter);
		// Remove the corresponding item from the queue list
		ud->queue = g_slist_remove_link(ud->queue, link);
		g_slist_free_1(link);
		g_hash_table_destroy(queue_item->settings);
		link = queue_item->audio_settings;
		while (link != NULL)
		{
			GSList *nextlink;
			g_hash_table_destroy((GHashTable*)link->data);
			nextlink = g_slist_next(link);
			g_slist_free_1(link);
			link = nextlink;
		}
		g_strfreev (queue_item->chapter_list);
		unique_id = queue_item->unique_id;
		g_free(queue_item);
		ghb_remove_job(unique_id);
	}
	else
	{	
		gtk_tree_path_free (treepath);
	}
}

static gint
find_queue_item(GSList *queue, gint unique_id, job_settings_t **job)
{
	job_settings_t *js;
	gint index = -1;
	
	while (queue != NULL)
	{
		index++;
		js = (job_settings_t*)queue->data;
		if (js->unique_id == unique_id)
		{
			*job = js;
			return index;
		}
		queue = queue->next;
	}
	*job = NULL;
	return index;
}

static void
queue_buttons_grey(signal_user_data_t *ud, gboolean working)
{
	GtkWidget *widget;
	GtkAction *action;
	gint titleindex = ghb_settings_get_int(ud->settings, "title");
	gboolean title_ok = (titleindex >= 0);
	widget = GHB_WIDGET (ud->builder, "queue_start1");
	gtk_widget_set_sensitive (widget, !working && title_ok);
	widget = GHB_WIDGET (ud->builder, "queue_start2");
	gtk_widget_set_sensitive (widget, !working && title_ok);
	action = GHB_ACTION (ud->builder, "queue_start_menu");
	gtk_action_set_sensitive (action, !working && title_ok);
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

static gint autostart_timeout = -1;

gboolean
autostart_timer_cb(gpointer data)
{
	GtkWidget *widget;
	GtkProgressBar *progress;
	signal_user_data_t *ud = (signal_user_data_t*)data;
	
	if (autostart_timeout < 0) return FALSE;
	gchar *remaining = g_strdup_printf("Encoding will start in %d second%c",
									   (autostart_timeout-1) / 40 + 1, autostart_timeout <= 40 ? ' ':'s');
	progress = GTK_PROGRESS_BAR(GHB_WIDGET (ud->builder, "autostart_countdown"));
	gtk_progress_bar_set_fraction (progress, (gdouble)autostart_timeout / 800);
	gtk_progress_bar_set_text(progress, remaining);
	g_free(remaining);
	autostart_timeout--;
	if (autostart_timeout == 0)
	{
		widget = GHB_WIDGET(ud->builder, "autostart_dialog");
		gtk_widget_hide(widget);
		queue_start_clicked_cb(NULL, ud);
		return FALSE;
	}
	return TRUE;
}

gboolean
ghb_timer_cb(gpointer data)
{
	static gint ticks = 0;
	gint titleindex;
	gint unique_id;
	job_settings_t *js;
	static gint current_id = -1;
	gint index;
	GtkTreeView *treeview;
	GtkTreeStore *store;
	GtkTreeIter iter;
	static gint working = 0;
	static gboolean work_started = FALSE;

	signal_user_data_t *ud = (signal_user_data_t*)data;
	switch (ghb_backend_events (ud, &unique_id))
	{
        case GHB_EVENT_WORKING:
        {
			if (!work_started)
			{
				work_started = TRUE;
				queue_buttons_grey(ud, TRUE);
			}
			if (unique_id != current_id)
			{
				index = find_queue_item(ud->queue, current_id, &js);
				if (js != NULL)
				{
					js->status = GHB_QUEUE_DONE;
					treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
					store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
					gchar *path = g_strdup_printf ("%d", index);
					if (gtk_tree_model_get_iter_from_string ( GTK_TREE_MODEL(store), &iter, path))
					{
						gtk_tree_store_set(store, &iter, 0, "hb-complete", -1);
					}
					g_free(path);
				}

				index = find_queue_item(ud->queue, unique_id, &js);
				if (js != NULL)
				{
					js->status = GHB_QUEUE_RUNNING;
					current_id = unique_id;
				}
			}
			index = find_queue_item(ud->queue, unique_id, &js);
			if (index >= 0)
			{
				gchar working_icon[] = "hb-working0";
				working_icon[10] = '0' + working;
				working = (working+1) % 6;
				treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
				store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
				gchar *path = g_strdup_printf ("%d", index);
				if (gtk_tree_model_get_iter_from_string ( GTK_TREE_MODEL(store), &iter, path))
				{
					gtk_tree_store_set(store, &iter, 0, working_icon, -1);
				}
				g_free(path);
			}
        } break;
        case GHB_EVENT_PAUSED:
        {
		} break;
        case GHB_EVENT_WORK_DONE:
        {
			ud->state &= ~GHB_STATE_WORKING;
			work_started = FALSE;
			queue_buttons_grey(ud, FALSE);
			index = find_queue_item(ud->queue, current_id, &js);
			if (js != NULL)
			{
				js->status = GHB_QUEUE_DONE;
				treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
				store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
				gchar *path = g_strdup_printf ("%d", index);
				if (gtk_tree_model_get_iter_from_string ( GTK_TREE_MODEL(store), &iter, path))
				{
					gtk_tree_store_set(store, &iter, 0, "hb-complete", -1);
				}
				g_free(path);
			}
			current_id = -1;
			if (ghb_autostart)
			{
				ghb_hb_cleanup();
				gtk_main_quit();
			}
        } break;
        case GHB_EVENT_WORK_CANCELED:
        {
			work_started = FALSE;
			queue_buttons_grey(ud, FALSE);
			index = find_queue_item(ud->queue, current_id, &js);
			if (js != NULL)
			{
				js->status = GHB_QUEUE_CANCELED;
				treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
				store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
				gchar *path = g_strdup_printf ("%d", index);
				if (gtk_tree_model_get_iter_from_string ( GTK_TREE_MODEL(store), &iter, path))
				{
					gtk_tree_store_set(store, &iter, 0, "hb-canceled", -1);
				}
				g_free(path);
			}
			current_id = -1;
        } break;
		case GHB_EVENT_SCAN_DONE:
		{
			ghb_title_info_t tinfo;
			
			ud->state &= ~GHB_STATE_SCANNING;
			ghb_update_ui_combo_box(ud->builder, "title", 0, FALSE);
			titleindex = ghb_longest_title();
			ghb_ui_update_int(ud, "title", titleindex);
			queue_buttons_grey(ud, FALSE);

			// Are there really any titles.
			if (ghb_get_title_info(&tinfo, titleindex))
			{
				if (ghb_autostart)
				{
					GtkWidget *widget;
					
					gint title = ghb_settings_get_int(ud->settings, "title");
					gint start_chapter = ghb_settings_get_int(ud->settings, "start_chapter");
					gint end_chapter = ghb_settings_get_int(ud->settings, "end_chapter");
					gboolean pass2 = ghb_settings_get_bool(ud->settings, "two_pass");
					const gchar *vol_name = ghb_settings_get_string(ud->settings, "volume_label");
					if (vol_name == NULL)
						vol_name = "No Title";
					const gchar *vcodec = ghb_settings_get_option(ud->settings, "video_codec");
					const gchar *container = ghb_settings_get_option(ud->settings, "container");
					const gchar *acodec = ghb_settings_get_option(ud->settings, "audio_codec");
					const gchar *dest = ghb_settings_get_string(ud->settings, "destination");
					const gchar *preset = ghb_settings_get_string(ud->settings, "preset");
					gchar *info = g_strdup_printf 
						(
						 "<big><b>%s</b></big> (Title %d, Chapters %d through %d, %d Video %s)"
						 "\n<b>Preset:</b> %s"
						 "\n<b>Format:</b> %s Container, %s Video + %s Audio"
						 "\n<b>Destination:</b> %s",
						 vol_name, title+1, start_chapter, end_chapter, 
						 pass2 ? 2:1, pass2 ? "Passes":"Pass",
						 preset, container, vcodec, acodec, dest);

					widget = GHB_WIDGET (ud->builder, "autostart_summary");
					gtk_label_set_markup (GTK_LABEL(widget), info);
					g_free(info);
					widget = GHB_WIDGET(ud->builder, "autostart_dialog");
					gtk_widget_show_now(widget);
					g_timeout_add (25, autostart_timer_cb, (gpointer)ud);
					autostart_timeout = 800;
				}
			}
			else
			{
				GtkProgressBar *progress;
				progress = GTK_PROGRESS_BAR(GHB_WIDGET (ud->builder, "progressbar"));
				gtk_progress_bar_set_fraction (progress, 0);
				gtk_progress_bar_set_text (progress, "No Source");
			}
		} break;
		default:
		{
			if (work_started)
			{
				work_started = FALSE;
				queue_buttons_grey(ud, FALSE);
			}
		} break;
	}
	if (update_default_destination)
	{
		const gchar *dest = ghb_settings_get_string(ud->settings, "destination");
		gchar *dest_dir = g_path_get_dirname (dest);
		const gchar *def_dest = ghb_settings_get_string(ud->settings, "destination_dir");
		if (strcmp(dest_dir, def_dest) != 0)
		{
			ghb_settings_set_string (ud->settings, "destination_dir", dest_dir);
			ghb_prefs_save (ud->settings);
		}
		update_default_destination = FALSE;
	}
	if (update_preview)
	{
		set_preview_image (ud);
		update_preview = FALSE;
	}
	if (ticks == 3 && ghb_autostart)
	{
		// Make sure this doesn't happen twice
		const gchar *source = ghb_settings_get_string(ud->settings, "source");
		if (update_source_label(ud, source))
		{
			GtkProgressBar *progress;
			progress = GTK_PROGRESS_BAR(GHB_WIDGET (ud->builder, "progressbar"));
			const gchar *path = ghb_settings_get_string( ud->settings, "source");
			gtk_progress_bar_set_fraction (progress, 0);
			gtk_progress_bar_set_text (progress, "Scanning ...");
			ud->state |= GHB_STATE_SCANNING;
			ghb_backend_scan (path, 0);
		}
	}
	ticks++;
	return TRUE;
}

void
autostart_ok_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	widget = GHB_WIDGET(ud->builder, "autostart_dialog");
	gtk_widget_hide(widget);
	queue_start_clicked_cb(NULL, ud);
	autostart_timeout = -1;
}

void
autostart_cancel_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	widget = GHB_WIDGET(ud->builder, "autostart_dialog");
	gtk_widget_hide(widget);
	autostart_timeout = -1;
}

gboolean
ghb_log_cb(GIOChannel *source, GIOCondition cond, gpointer data)
{
	gchar *text = NULL;
	gsize length;
	GtkTextView *textview;
	GtkTextBuffer *buffer;
	GtkTextMark *mark;
	GError *gerror = NULL;
	GIOStatus status;
	
	signal_user_data_t *ud = (signal_user_data_t*)data;

	status = g_io_channel_read_line (source, &text, &length, NULL, &gerror);
	if (text != NULL)
	{
		textview = GTK_TEXT_VIEW(GHB_WIDGET (ud->builder, "activity_view"));
		buffer = gtk_text_view_get_buffer (textview);
		mark = gtk_text_buffer_get_insert (buffer);
		gtk_text_view_scroll_mark_onscreen(textview, mark);
		gtk_text_buffer_insert_at_cursor (buffer, text, -1);
		g_io_channel_write_chars (ud->activity_log, text, length, &length, NULL);
		g_free(text);
	}
	if (status != G_IO_STATUS_NORMAL)
	{
		// This should never happen, but if it does I would get into an
		// infinite loop.  Returning false removes this callback.
		g_warning("Error while reading activity from pipe\n");
		if (gerror != NULL)
		{
			g_warning("%s\n", gerror->message);
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
	
	g_debug("show_presets_clicked_cb ()\n");
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
	ghb_prefs_save(ud->settings);
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
		g_free(path);
	}
}

static void
update_chapter_list(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean done;
	gchar **chapters;
	gint titleindex, ii;
	
	g_debug("update_chapter_list ()\n");
	titleindex = ghb_settings_get_index(ud->settings, "title");
	chapters = ghb_get_chapters(titleindex);
	if (ud->chapter_list != NULL)
		g_strfreev (ud->chapter_list);
	ud->chapter_list = chapters;
	
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "chapters_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	ii = 0;
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
	{
		do
		{
			if (chapters != NULL && chapters[ii])
			{
				// Update row with settings data
				g_debug("Updating row\n");
				gtk_list_store_set(store, &iter, 
					0, ii+1,
					1, chapters[ii],
					2, TRUE,
					-1);
				ii++;
				done = !gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
			}
			else
			{
				// No more settings data, remove row
				g_debug("Removing row\n");
				done = !gtk_list_store_remove(store, &iter);
			}
		} while (!done);
	}
	while (chapters != NULL && chapters[ii])
	{
		// Additional settings, add row
		g_debug("Adding row\n");
		g_debug("%d -- %s\n", ii, chapters[ii]);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
			0, ii+1,
			1, chapters[ii],
			2, TRUE,
			-1);
		ii++;
	}
}

void
chapter_edited_cb(GtkCellRendererText *cell, gchar *path, gchar *text, signal_user_data_t *ud)
{
	GtkTreePath *treepath;
	GtkListStore *store;
	GtkTreeView *treeview;
	GtkTreeViewColumn *column;
	GtkTreeIter iter;
	gint index;
	
	g_debug("chapter_edited_cb ()\n");
	g_debug("path (%s)\n", path);
	g_debug("text (%s)\n", text);
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "chapters_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	treepath = gtk_tree_path_new_from_string (path);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath);
	gtk_tree_path_free (treepath);
	gtk_list_store_set(store, &iter, 
		1, text,
		2, TRUE,
		-1);
	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &index, -1);
	g_free(ud->chapter_list[index-1]);
	ud->chapter_list[index-1] = g_strdup(text);
	if (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter))
	{
		column = gtk_tree_view_get_column(treeview, 1);
		treepath = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
		gtk_tree_view_set_cursor(treeview, treepath, column, TRUE);
		gtk_tree_path_free (treepath);
	}
}

void
chapter_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	
	g_debug("chapter_list_selection_changed_cb ()\n");
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		// What to do??
	}
}

void
queue_list_size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation, GtkCellRenderer *cell)
{
	GtkTreeViewColumn *column;
	gint width;
	
	column = gtk_tree_view_get_column (GTK_TREE_VIEW(widget), 0);
	width = gtk_tree_view_column_get_width(column);
	g_debug("col width %d alloc width %d\n", width, allocation->width);
	// Set new wrap-width.  Shave a little off to accomidate the icons
	// that share this column.
	if (width >= 564) // Don't allow below a certain size
		g_object_set(cell, "wrap-width", width-70, NULL);
}

void
preview_button_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	gint titleindex = ghb_settings_get_int(ud->settings, "title");
	if (titleindex < 0) return;
	g_debug("titleindex %d\n", titleindex);

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
	g_debug("-------------------------------allocate %d x %d\n", allocation->width, allocation->height);
	if (preview_button_width == allocation->width &&
		preview_button_height == allocation->height)
	{
		// Nothing to do. Bug out.
		g_debug("nothing to do\n");
		return;
	}
	g_debug("-------------------------------prev allocate %d x %d\n", preview_button_width, preview_button_height);
	preview_button_width = allocation->width;
	preview_button_height = allocation->height;
	set_preview_image(ud);
}

void
queue_start_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	GSList *link = ud->queue;
	job_settings_t *job;
	gboolean running = FALSE;
	while (link != NULL)
	{
		job = (job_settings_t*)link->data;
		if ((job->status == GHB_QUEUE_RUNNING) || 
			(job->status == GHB_QUEUE_PENDING))
		{
			running = TRUE;
			break;
		}
		link = link->next;
	}
	if (!running)
	{
		// The queue has no running or pending jobs.
		// Add current settings to the queue, then run.
		if (!queue_add(ud))
			return;
	}
	ud->state |= GHB_STATE_WORKING;
	ghb_start_queue();
}

void
queue_stop_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
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
	g_debug("ghb_hbfd\n");
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
	g_debug("hbfd_toggled_cb\n");
	ghb_widget_to_setting (ud->settings, widget);
	gboolean hbfd = ghb_settings_get_bool(ud->settings, "hbfd");
	ghb_hbfd(ud, hbfd);
	ghb_prefs_save(ud->settings);
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
	g_free(device);
	return (dtype == LIBHAL_DRIVE_TYPE_CDROM);
}

void
drive_changed_cb(GVolumeMonitor *gvm, GDrive *gd, signal_user_data_t *ud)
{
	gchar *device;

	if (ud->current_dvd_device == NULL) return;
	if (ud->state != GHB_STATE_IDLE) return;
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
			ud->state |= GHB_STATE_SCANNING;
			ghb_backend_scan(device, 0);
		}
		else
		{
			ud->state |= GHB_STATE_SCANNING;
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
        g_debug ("could not get system bus: %s\n", error.message);
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

