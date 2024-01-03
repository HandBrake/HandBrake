/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * title-add.c
 * Copyright (C) John Stebbins 2008-2024 <stebbins@stebbins>
 *
 * title-add.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * title-add.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with title-add.c.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#include "compat.h"
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "handbrake/handbrake.h"
#include "settings.h"
#include "jobdict.h"
#include "titledict.h"
#include "hb-backend.h"
#include "values.h"
#include "callbacks.h"
#include "presets.h"
#include "audiohandler.h"
#include "hb-dvd.h"
#include "queuehandler.h"
#include "title-add.h"
#include "ghb-file-button.h"

G_MODULE_EXPORT void
title_selected_cb (GtkWidget *widget, signal_user_data_t *ud);
G_MODULE_EXPORT void
title_dest_file_cb (GtkWidget *widget, signal_user_data_t *ud);
G_MODULE_EXPORT void
title_dest_dir_cb (GtkWidget *widget, GParamSpec *spec, signal_user_data_t *ud);


static GtkWidget *find_widget (GtkWidget *widget, gchar *name)
{
    const char *wname;
    GtkWidget *result = NULL;

    if (widget == NULL || name == NULL)
        return NULL;

    wname = gtk_widget_get_name(widget);
    if (wname != NULL && !strncmp(wname, name, 80))
    {
        return widget;
    }
    if (GTK_IS_CONTAINER(widget))
    {
        GList *list, *link;
        link = list = gtk_container_get_children(GTK_CONTAINER(widget));
        while (link)
        {
            result = find_widget(GTK_WIDGET(link->data), name);
            if (result != NULL)
                break;
            link = link->next;
        }
        g_list_free(list);
    }
    return result;
}

static gboolean
validate_settings (signal_user_data_t *ud, GhbValue *settings, gint batch)
{
    // Check to see if the dest file exists or is
    // already in the queue
    const gchar *dest;
    gint count, ii;
    gint title_id, titleindex;
    const hb_title_t *title;
    GtkWindow *hb_window;

    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));

    title_id = ghb_dict_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL) return FALSE;
    dest = ghb_dict_get_string(settings, "destination");
    count = ghb_array_len(ud->queue);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *queueDict, *uiDict;
        const gchar *filename;

        queueDict = ghb_array_get(ud->queue, ii);
        uiDict = ghb_dict_get(queueDict, "uiSettings");
        filename = ghb_dict_get_string(uiDict, "destination");
        if (g_strcmp0(dest, filename) == 0)
        {
            if (!ghb_question_dialog_run(hb_window, GHB_ACTION_NORMAL,
                _("Overwrite"), _("Cancel"), _("Overwrite File?"),
                _("Destination: %s\n\n"
                  "Another queued job has specified the same destination.\n"
                  "Do you want to overwrite?"), dest))
            {
                return FALSE;
            }
            break;
        }
    }
    gchar *destdir = g_path_get_dirname(dest);
    if (!g_file_test(destdir, G_FILE_TEST_IS_DIR))
    {
        ghb_alert_dialog_show(GTK_MESSAGE_ERROR, _("Invalid Destination"),
                              _("“%s” is is not a valid directory."), destdir);
        g_free(destdir);
        return FALSE;
    }
#if !defined(_WIN32)
    // This doesn't work properly on windows
    if (g_access(destdir, R_OK|W_OK) != 0)
    {
        ghb_alert_dialog_show(GTK_MESSAGE_ERROR, _("Invalid Destination"),
                              _("“%s” is not a writable directory."), destdir);
        g_free(destdir);
        return FALSE;
    }
#endif
    g_free(destdir);
    if (g_file_test(dest, G_FILE_TEST_EXISTS))
    {
        if (!ghb_question_dialog_run(hb_window, GHB_ACTION_DESTRUCTIVE,
            _("Overwrite"), _("Cancel"), _("Overwrite File?"),
            _("The file “%s” already exists.\n"
              "Do you want to overwrite it?"), dest))
        {
            return FALSE;
        }
    }
    // Validate audio settings
    if (!ghb_validate_audio(settings, hb_window))
    {
        return FALSE;
    }
    // Validate audio settings
    if (!ghb_validate_subtitles(settings, hb_window))
    {
        return FALSE;
    }
    // Validate video settings
    if (!ghb_validate_video(settings, hb_window))
    {
        return FALSE;
    }
    // Validate filter settings
    if (!ghb_validate_filters(settings, hb_window))
    {
        return FALSE;
    }
    return TRUE;
}

void ghb_finalize_job (GhbValue *settings)
{
    GhbValue *preset, *job;

    preset = ghb_settings_to_preset(settings);
    job    = ghb_dict_get(settings, "Job");

    // Add scale filter since the above does not
    GhbValue *filter_settings, *filter_list, *filter_dict;
    gint width, height, crop[4];

    filter_settings = ghb_get_job_filter_settings(settings);
    filter_list = ghb_array_new();
    ghb_dict_set(filter_settings, "FilterList", filter_list);

    width = ghb_dict_get_int(settings, "scale_width");
    height = ghb_dict_get_int(settings, "scale_height");

    crop[0] = ghb_dict_get_int(settings, "PictureTopCrop");
    crop[1] = ghb_dict_get_int(settings, "PictureBottomCrop");
    crop[2] = ghb_dict_get_int(settings, "PictureLeftCrop");
    crop[3] = ghb_dict_get_int(settings, "PictureRightCrop");

    hb_dict_t * dict = ghb_dict_new();
    ghb_dict_set_int(dict, "width", width);
    ghb_dict_set_int(dict, "height", height);
    ghb_dict_set_int(dict, "crop-top", crop[0]);
    ghb_dict_set_int(dict, "crop-bottom", crop[1]);
    ghb_dict_set_int(dict, "crop-left", crop[2]);
    ghb_dict_set_int(dict, "crop-right", crop[3]);

    filter_dict = ghb_dict_new();
    ghb_dict_set_int(filter_dict, "ID", HB_FILTER_CROP_SCALE);
    ghb_dict_set(filter_dict, "Settings", dict);
    hb_add_filter2(filter_list, filter_dict);

    // Apply selected preset settings
    hb_preset_apply_mux(preset, job);
    hb_preset_apply_video(preset, job);
    hb_preset_apply_filters(preset, job);

    ghb_value_free(&preset);
}

gboolean
ghb_add_title_to_queue (signal_user_data_t *ud, GhbValue *settings, gint batch)
{
    // Add settings to the queue
    if (!validate_settings(ud, settings, batch))
    {
        return FALSE;
    }

    if (ud->queue == NULL)
        ud->queue = ghb_array_new();

    ghb_finalize_job(settings);

    GhbValue *titleDict  = ghb_get_title_settings(settings);
    GhbValue *jobDict    = ghb_get_job_settings(settings);
    GhbValue *destDict   = ghb_get_job_dest_settings(settings);
    GhbValue *uiDict     = ghb_value_dup(settings);

    if (ghb_dict_get_string(destDict, "File") == NULL)
    {
        const char *dest = ghb_dict_get_string(uiDict, "destination");
        ghb_dict_set_string(destDict, "File", dest);
    }

    ghb_dict_remove(uiDict, "Job");
    ghb_dict_remove(uiDict, "Title");

    GhbValue *queueDict  = ghb_dict_new();
    ghb_dict_set(queueDict, "uiSettings", uiDict);
    ghb_dict_set(queueDict, "Job", ghb_value_dup(jobDict));
    ghb_dict_set(queueDict, "Title", ghb_value_dup(titleDict));

    // Copy current prefs into settings
    // The job should run with the preferences that existed
    // when the job was added to the queue.
    ghb_dict_set(uiDict, "Preferences", ghb_value_dup(ud->prefs));

    // Make a copy of current settings to be used for the new job
    ghb_dict_set_int(uiDict, "job_status", GHB_QUEUE_PENDING);
    ghb_dict_set_int(uiDict, "job_unique_id", 0);

    ghb_array_append(ud->queue, queueDict);
    ghb_add_to_queue_list(ud, queueDict);
    ghb_save_queue(ud->queue);
    ghb_update_pending(ud);
    ghb_queue_buttons_grey(ud);

    return TRUE;
}

static gboolean
title_destination_is_unique (GhbValue *settings_array, gint index)
{
    gint count, ii;
    GhbValue *settings, *gdest;
    const char *dest;

    settings = ghb_array_get(settings_array, index);
    gdest = ghb_dict_get_value(settings, "destination");
    dest = ghb_value_get_string(gdest);
    if (dest == NULL)
        return FALSE;

    count = ghb_array_len(settings_array);
    count = count < index ? count : index;
    for (ii = 0; ii < count; ii++)
    {
        const char *tmp;

        settings = ghb_array_get(settings_array, ii);
        gdest = ghb_dict_get_value(settings, "destination");
        tmp = ghb_value_get_string(gdest);
        if (tmp != NULL && !strncmp(dest, tmp, PATH_MAX))
            return FALSE;
    }
    return TRUE;
}

static PangoAttrList *default_title_attrs;

static void
title_add_set_sensitive (GtkWidget *row, gboolean sensitive)
{
    GtkWidget *widget;
    widget = find_widget(row, "title_selected");
    gtk_widget_set_sensitive(widget, sensitive);

    widget = find_widget(row, "title_label");
    if (!sensitive)
    {
        PangoAttrList *pal;
        PangoAttribute *bg;
        bg = pango_attr_background_new(0xFFFF, 0xFFFF, 0xA000);
        pal = pango_attr_list_new();
        pango_attr_list_insert(pal, bg);
        gtk_label_set_attributes(GTK_LABEL(widget), pal);
        gtk_widget_set_has_tooltip(widget, TRUE);
    }
    else
    {
        gtk_label_set_attributes(GTK_LABEL(widget), default_title_attrs);
        gtk_widget_set_has_tooltip(widget, FALSE);
    }
}

static gboolean
title_add_are_conflicts (signal_user_data_t *ud)
{
    GtkListBox *list;
    GtkWidget *row;
    gint count, ii;

    list = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "title_add_multiple_list"));
    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        row = GTK_WIDGET(gtk_list_box_get_row_at_index(list, ii));
        if (!title_destination_is_unique(ud->settings_array, ii))
        {
            title_add_set_sensitive(GTK_WIDGET(row), FALSE);
            return TRUE;
        }
        title_add_set_sensitive(GTK_WIDGET(row), TRUE);
    }
    return FALSE;
}

static void
title_add_set_error_text (signal_user_data_t *ud, gboolean are_conflicts)
{
    const gchar *msg;
    static gboolean conflict_showing = FALSE;
    GtkMessageType msg_type;

    if (are_conflicts)
    {
        msg =
            "<span foreground='black' weight='bold'>"
            "Duplicate destination files detected.\n"
            "Duplicates will not be added to the queue."
            "</span>";
            msg_type = GTK_MESSAGE_WARNING;
    }
    else
    {
        msg =
            "Destination files OK.  No duplicates detected.";
            msg_type = GTK_MESSAGE_INFO;
    }
    if (are_conflicts ^ conflict_showing)
    {
        GtkInfoBar *info;
        GtkContainer *content_area;
        GList *list;
        GtkLabel *label;

        info = GTK_INFO_BAR(GHB_WIDGET(ud->builder,
                                       "title_add_multiple_infobar"));
        content_area = GTK_CONTAINER(gtk_info_bar_get_content_area(info));
        list = gtk_container_get_children(content_area);
        // Label is first in list
        label = GTK_LABEL(list->data);
        gtk_label_set_markup(label, msg);
        gtk_info_bar_set_message_type(info, msg_type);
        conflict_showing = are_conflicts;
    }
}

static void title_add_check_conflicts (signal_user_data_t *ud)
{
    gint count, ii;
    GhbValue *settings;
    GtkWidget *row;
    GtkListBox *list;
    GtkToggleButton *selected;
    gboolean can_select;
    gboolean are_conflicts = FALSE;

    list = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "title_add_multiple_list"));
    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        row = GTK_WIDGET(gtk_list_box_get_row_at_index(list, ii));
        selected = GTK_TOGGLE_BUTTON(find_widget(row, "title_selected"));

        settings = ghb_array_get(ud->settings_array, ii);
        can_select = title_destination_is_unique(ud->settings_array, ii);
        ghb_dict_set_bool(settings, "title_selected", FALSE);
        gtk_toggle_button_set_active(selected, FALSE);
        title_add_set_sensitive(GTK_WIDGET(row), can_select);
        are_conflicts |= !can_select;
    }
    title_add_set_error_text(ud, are_conflicts);
}

static void
add_multiple_titles (signal_user_data_t *ud)
{
    gint count, ii;

    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *settings;

        settings = ghb_array_get(ud->settings_array, ii);
        if (ghb_dict_get_bool(settings, "title_selected") &&
            !ghb_add_title_to_queue(ud, settings, 1))
        {
            ghb_log("Validation failed. Could not add all titles to queue.");
            break;
        }
    }
    ghb_queue_selection_init(ud);
}

static GtkListBoxRow *list_box_get_row (GtkWidget *widget)
{
    while (widget != NULL && G_OBJECT_TYPE(widget) != GTK_TYPE_LIST_BOX_ROW)
    {
        widget = gtk_widget_get_parent(widget);
    }
    return GTK_LIST_BOX_ROW(widget);
}

static GtkWidget *title_create_row (signal_user_data_t *ud)
{
    GtkBox *hbox, *vbox_dest;
    GtkCheckButton *selected;
    GtkLabel *title;
    GtkEntry *dest_file;
    GhbFileButton *dest_dir;

    hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_box_set_spacing(hbox, 6);
    gtk_widget_show(GTK_WIDGET(hbox));

    // Select checkbox
    selected = GTK_CHECK_BUTTON(gtk_check_button_new());
    gtk_widget_set_tooltip_markup(GTK_WIDGET(selected),
      _("Select this title for adding to the queue.\n"));
    gtk_widget_set_valign(GTK_WIDGET(selected), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(selected), "title_selected");
    gtk_widget_show(GTK_WIDGET(selected));
    g_signal_connect(selected, "toggled", (GCallback)title_selected_cb, ud);
    ghb_box_append_child(hbox, GTK_WIDGET(selected));

    // Title label
    title = GTK_LABEL(gtk_label_new(_("No Title")));
    gtk_label_set_width_chars(title, 12);
    gtk_widget_set_halign(GTK_WIDGET(title), GTK_ALIGN_START);
    gtk_widget_set_valign(GTK_WIDGET(title), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(title), "title_label");
    gtk_widget_show(GTK_WIDGET(title));
    ghb_box_append_child(hbox, GTK_WIDGET(title));

    default_title_attrs = gtk_label_get_attributes(title);
    gtk_widget_set_tooltip_text(GTK_WIDGET(title),
        _("There is another title with the same destination file name.\n"
        "This title will not be added to the queue unless you change\n"
        "the output file name.\n"));
    gtk_widget_set_has_tooltip(GTK_WIDGET(title), FALSE);

    // Destination entry and file button
    vbox_dest = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_hexpand(GTK_WIDGET(vbox_dest), TRUE);
    gtk_widget_set_halign(GTK_WIDGET(vbox_dest), GTK_ALIGN_FILL);
    //gtk_widget_set_hexpand(GTK_WIDGET(vbox_dest), TRUE);
    dest_file = GTK_ENTRY(gtk_entry_new());
    ghb_entry_set_width_chars(dest_file, 40);
    gtk_widget_set_name(GTK_WIDGET(dest_file), "title_file");
    //gtk_widget_set_hexpand(GTK_WIDGET(dest_file), TRUE);
    gtk_widget_show(GTK_WIDGET(dest_file));
    g_signal_connect(dest_file, "changed", (GCallback)title_dest_file_cb, ud);
    ghb_box_append_child(vbox_dest, GTK_WIDGET(dest_file));
    dest_dir = ghb_file_button_new(_("Destination Directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    ghb_file_button_set_accept_label(dest_dir, _("Select"));
    g_signal_connect(dest_dir, "notify::file",
                     G_CALLBACK(title_dest_dir_cb), ud);
    gtk_widget_set_name(GTK_WIDGET(dest_dir), "title_dir");
    gtk_widget_set_hexpand(GTK_WIDGET(dest_dir), TRUE);
    gtk_widget_show(GTK_WIDGET(dest_dir));
    ghb_box_append_child(vbox_dest, GTK_WIDGET(dest_dir));
    gtk_widget_show(GTK_WIDGET(vbox_dest));
    ghb_box_append_child(hbox, GTK_WIDGET(vbox_dest));

    return GTK_WIDGET(hbox);
}

static gboolean clear_select_all_busy = FALSE;

G_MODULE_EXPORT void
title_add_select_all_cb (GtkWidget *widget, signal_user_data_t *ud)
{
    gint count, ii;
    GhbValue *settings;
    GtkWidget *row;
    GtkListBox *list;
    GtkToggleButton *selected;
    gboolean can_select;
    GtkToggleButton *clear_all;
    GtkToggleButton *select_all;

    if (!ghb_widget_boolean(widget))
        return;

    clear_select_all_busy = TRUE;

    clear_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_clear_all"));
    select_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_select_all"));
    gtk_toggle_button_set_active(clear_all, FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(select_all), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(clear_all), TRUE);

    list = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "title_add_multiple_list"));
    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        row = GTK_WIDGET(gtk_list_box_get_row_at_index(list, ii));
        selected = GTK_TOGGLE_BUTTON(find_widget(row, "title_selected"));
        settings = ghb_array_get(ud->settings_array, ii);
        can_select = title_destination_is_unique(ud->settings_array, ii);
        ghb_dict_set_bool(settings, "title_selected", can_select);
        gtk_toggle_button_set_active(selected, TRUE);
        title_add_set_sensitive(GTK_WIDGET(row), can_select);
    }
    clear_select_all_busy = FALSE;
}

G_MODULE_EXPORT void
title_add_clear_all_cb (GtkWidget *widget, signal_user_data_t *ud)
{
    gint count, ii;
    GhbValue *settings;
    GtkWidget *row;
    GtkListBox *list;
    GtkToggleButton *selected;
    GtkToggleButton *clear_all;
    GtkToggleButton *select_all;

    if (!ghb_widget_boolean(widget))
        return;

    clear_select_all_busy = TRUE;

    clear_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_clear_all"));
    select_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_select_all"));
    gtk_toggle_button_set_active(select_all, FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(select_all), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(clear_all), FALSE);

    list = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "title_add_multiple_list"));
    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        row = GTK_WIDGET(gtk_list_box_get_row_at_index(list, ii));
        selected = GTK_TOGGLE_BUTTON(find_widget(row, "title_selected"));
        settings = ghb_array_get(ud->settings_array, ii);
        ghb_dict_set_bool(settings, "title_selected", FALSE);
        gtk_toggle_button_set_active(selected, FALSE);
    }
    clear_select_all_busy = FALSE;
}

G_MODULE_EXPORT void
title_selected_cb (GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *settings;
    gboolean selected;
    GtkToggleButton *select_all;
    GtkToggleButton *clear_all;
    gboolean can_select;

    if (clear_select_all_busy)
        return;

    GtkListBoxRow * row = list_box_get_row(widget);
    if (row == NULL)
        return;

    clear_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_clear_all"));
    select_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_select_all"));
    gtk_toggle_button_set_active(select_all, FALSE);
    gtk_toggle_button_set_active(clear_all, FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(clear_all), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(select_all), TRUE);

    gint index = gtk_list_box_row_get_index(row);
    selected = ghb_widget_boolean(widget);
    settings = ghb_array_get(ud->settings_array, index);
    can_select = title_destination_is_unique(ud->settings_array, index);
    ghb_dict_set_bool(settings, "title_selected",
                             selected && can_select);
}

G_MODULE_EXPORT void
title_dest_file_cb (GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *settings;
    const gchar *dest_dir;
    gchar *dest_file, *dest;
    GtkListBoxRow * row = list_box_get_row(widget);
    if (row == NULL)
        return;
    gint index = gtk_list_box_row_get_index(row);

    dest_file = ghb_widget_string(widget);
    settings = ghb_array_get(ud->settings_array, index);

    ghb_dict_set_string(settings, "dest_file", dest_file);
    dest_dir = ghb_dict_get_string(settings, "dest_dir");
    dest = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", dest_dir, dest_file);
    ghb_dict_set_string(settings, "destination", dest);
    GhbValue *dest_dict = ghb_get_job_dest_settings(settings);
    ghb_dict_set_string(dest_dict, "File", dest);

    // Check if changing the destination file name resolves
    // a file name conflict.  Enable selection if so.
    // Disable selection if it creates a conflict!!!
    gboolean selected, can_select;

    widget     = find_widget(GTK_WIDGET(row), "title_selected");
    selected   = ghb_widget_boolean(widget);
    can_select = title_destination_is_unique(ud->settings_array, index);

    ghb_dict_set_bool(settings, "title_selected", selected && can_select);
    title_add_set_sensitive(GTK_WIDGET(row), can_select);

    g_free(dest_file);
    g_free(dest);

    title_add_set_error_text(ud,
        title_add_are_conflicts(ud));
}

G_MODULE_EXPORT void
title_dest_dir_cb (GtkWidget *widget, GParamSpec *spec, signal_user_data_t *ud)
{
    GhbValue *settings;
    const gchar *dest_file;
    gchar *dest_dir, *dest;
    GtkListBoxRow * row = list_box_get_row(widget);
    if (row == NULL)
        return;
    gint index = gtk_list_box_row_get_index(row);

    dest_dir = ghb_widget_string(widget);
    settings = ghb_array_get(ud->settings_array, index);

    ghb_dict_set_string(settings, "dest_dir", dest_dir);
    dest_file = ghb_dict_get_string(settings, "dest_file");
    dest = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", dest_dir, dest_file);
    ghb_dict_set_string(settings, "destination", dest);
    GhbValue *dest_dict = ghb_get_job_dest_settings(settings);
    ghb_dict_set_string(dest_dict, "File", dest);

    // Check if changing the destination file name resolves
    // a file name conflict.  Enable selection if so.
    // Disable selection if it creates a conflict!!!
    gboolean selected, can_select;

    widget     = find_widget(GTK_WIDGET(row), "title_selected");
    selected   = ghb_widget_boolean(widget);
    can_select = title_destination_is_unique(ud->settings_array, index);

    ghb_dict_set_bool(settings, "title_selected", selected && can_select);
    title_add_set_sensitive(GTK_WIDGET(row), can_select);

    g_free(dest_dir);
    g_free(dest);

    title_add_set_error_text(ud,
        title_add_are_conflicts(ud));
}

G_MODULE_EXPORT void
title_add_action_cb (GSimpleAction *action, GVariant *param,
                     signal_user_data_t *ud)
{
    ghb_add_title_to_queue(ud, ud->settings, 0);
    ghb_queue_selection_init(ud);
    // Validation of settings may have changed audio list
    ghb_audio_list_refresh_all(ud);
}

static void
title_add_multiple_response_cb (GtkDialog *dialog, int response,
                                signal_user_data_t *ud)
{
    g_signal_handlers_disconnect_by_data(dialog, ud);
    gtk_widget_set_visible(GTK_WIDGET(dialog), FALSE);
    if (response == GTK_RESPONSE_OK)
    {
        add_multiple_titles(ud);
    }
}

G_MODULE_EXPORT void
title_add_multiple_action_cb (GSimpleAction *action, GVariant *param,
                              signal_user_data_t *ud)
{
    GtkListBox *list;
    GtkWidget *row;
    gint count, ii;
    gint max_title_len = 0;
    GhbValue * preset = NULL;

    list = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "title_add_multiple_list"));
    // Clear title list
    ghb_container_empty(GTK_CONTAINER(list));

    if (ghb_dict_get_bool(ud->prefs, "SyncTitleSettings"))
    {
        preset = ghb_settings_to_preset(ud->settings);
    }

    // Set up the list of titles
    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *settings;
        GtkLabel *label;
        GtkEntry *entry;
        GhbFileButton *button;
        gchar *title_label;
        const gchar *dest_dir, *dest_file;
        gint title_id, titleindex;
        const hb_title_t *title;

        row = title_create_row(ud);
        label = GTK_LABEL(find_widget(row, "title_label"));
        entry = GTK_ENTRY(find_widget(row, "title_file"));
        button = GHB_FILE_BUTTON(find_widget(row, "title_dir"));

        settings = ghb_array_get(ud->settings_array, ii);
        if (preset != NULL)
        {
            ghb_preset_to_settings(settings, preset);
            ghb_set_title_settings(ud, settings);
        }
        title_id = ghb_dict_get_int(settings, "title");
        title = ghb_lookup_title(title_id, &titleindex);
        if (title != NULL)
        {
            gint len;

            title_label = ghb_create_title_label(title);
            len = strnlen(title_label, PATH_MAX);
            if (len > max_title_len)
                max_title_len = len;

            dest_file = ghb_dict_get_string(settings, "dest_file");
            dest_dir = ghb_dict_get_string(settings, "dest_dir");

            gtk_label_set_markup(label, title_label);
            ghb_editable_set_text(entry, dest_file);
            ghb_file_button_set_filename(button, dest_dir);

            g_free(title_label);
        }

        gtk_list_box_insert(list, row, -1);
    }

    if (preset != NULL)
    {
        ghb_value_free(&preset);
    }

    // Now we need to set the width of the title label since it
    // can vary on each row
    if (max_title_len > 60)
        max_title_len = 60;
    for (ii = 0; ii < count; ii++)
    {
        GtkLabel *label;

        row = GTK_WIDGET(gtk_list_box_get_row_at_index(list, ii));
        label = GTK_LABEL(find_widget(row, "title_label"));
        gtk_label_set_max_width_chars(label, max_title_len);
        gtk_label_set_width_chars(label, max_title_len);
        gtk_label_set_ellipsize(label, PANGO_ELLIPSIZE_END);
    }

    // Clear "select all" and "clear all" options
    GtkToggleButton *select_all;
    GtkToggleButton *clear_all;

    clear_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_clear_all"));
    select_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_select_all"));
    gtk_toggle_button_set_active(clear_all, FALSE);
    gtk_toggle_button_set_active(select_all, FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(select_all), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(clear_all), TRUE);

    // Disable selection of files with duplicate file names.
    title_add_check_conflicts(ud);

    // Pop up the title multiple selections dialog
    GtkWidget *dialog = GHB_WIDGET(ud->builder, "title_add_multiple_dialog");
    g_signal_connect(dialog, "response",
                     G_CALLBACK(title_add_multiple_response_cb), ud);
    gtk_widget_set_visible(dialog, TRUE);
}

G_MODULE_EXPORT void
title_add_all_action_cb (GSimpleAction *action, GVariant *param,
                         signal_user_data_t *ud)
{
    gint count, ii;
    GhbValue * preset = NULL;

    if (ghb_dict_get_bool(ud->prefs, "SyncTitleSettings"))
    {
        preset = ghb_settings_to_preset(ud->settings);
    }

    // Set up the list of titles
    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *settings;

        settings = ghb_array_get(ud->settings_array, ii);
        if (preset != NULL)
        {
            ghb_preset_to_settings(settings, preset);
            ghb_set_title_settings(ud, settings);
        }
        ghb_dict_set_bool(settings, "title_selected", TRUE);
    }

    if (preset != NULL)
    {
        ghb_value_free(&preset);
    }

    for (ii = 0; ii < count; ii++)
    {
        if (!title_destination_is_unique(ud->settings_array, ii))
        {
            ghb_question_dialog_run(GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window")),
                GHB_ACTION_NORMAL, _("OK"), NULL, _("Cannot Add Titles"),
                _("The filenames are not unique. Please choose\n"
                  "a unique destination filename for each title."));
            title_add_multiple_action_cb(action, param, ud);
            return;
        }
    }
    add_multiple_titles(ud);
}
