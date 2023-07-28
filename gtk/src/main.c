/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * main.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * main.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <locale.h>

#ifndef _WIN32
#include <sys/utsname.h>
#endif

#include <config.h>

#include "ghbcompat.h"

#if defined(_ENABLE_GST)
#include <gst/gst.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#include <io.h>
#define pipe(phandles)  _pipe (phandles, 4096, _O_BINARY)
#endif

#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "handbrake/handbrake.h"
#include "renderer_button.h"
#include "hb-backend.h"
#include "ghb-dvd.h"
#include "values.h"
#include "icons.h"
#include "callbacks.h"
#include "queuehandler.h"
#include "audiohandler.h"
#include "subtitlehandler.h"
#include "settings.h"
#include "resources.h"
#include "presets.h"
#include "preview.h"
#include "ui_res.h"
#include "color-scheme.h"
#include "power-manager.h"


/*
 * Standard gettext macros.
 */
#ifndef ENABLE_NLS
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif

#define BUILDER_NAME "ghb"

static GtkBuilder*
create_builder_or_die(const gchar * name)
{
    GtkWidget *dialog;
    GtkBuilder *xml;
    GError *error = NULL;

    ghb_log_func();
    xml = gtk_builder_new();
    gtk_builder_add_from_resource(xml, "/fr/handbrake/ghb/ui/ghb.ui", &error);
    if (!error)
        gtk_builder_add_from_resource(xml, "/fr/handbrake/ghb/ui/menu.ui", &error);

    const gchar *markup =
        N_("<b><big>Unable to create %s.</big></b>\n"
        "\n"
        "Internal error. Could not parse UI description.\n"
        "%s");

    if (error)
    {
        dialog = gtk_message_dialog_new_with_markup(NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            gettext(markup),
            name, error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        exit(EXIT_FAILURE);
    }
    return xml;
}

static GCallback
self_symbol_lookup(const gchar * symbol_name)
{
    static GModule *module = NULL;
    gpointer symbol = NULL;

    if (!module)
        module = g_module_open(NULL, 0);

    g_module_symbol(module, symbol_name, &symbol);
    return (GCallback) symbol;
}


static void
MyConnect(
    GtkBuilder *builder,
    GObject *object,
    const gchar *signal_name,
    const gchar *handler_name,
    GObject *connect_object,
    GConnectFlags flags,
    gpointer user_data)
{
    GCallback callback;

    g_return_if_fail(object != NULL);
    g_return_if_fail(handler_name != NULL);
    g_return_if_fail(signal_name != NULL);

    //const gchar *name = ghb_get_setting_key((GtkWidget*)object);
    //g_message("\n\nname %s", name);
    g_debug("Handler: %s", handler_name);
    g_debug("Signal: %s", signal_name);
    callback = self_symbol_lookup(handler_name);
    if (!callback)
    {
        g_warning("Signal handler (%s) not found", handler_name);
        return;
    }
    if (connect_object)
    {
        g_signal_connect_object(object, signal_name, callback, connect_object, flags);
    }
    else
    {
        if (flags & G_CONNECT_AFTER)
        {
            g_signal_connect_after( object, signal_name, callback, user_data);
        }
        else
        {
            g_signal_connect(object, signal_name, callback, user_data);
        }
    }
}

#if 0
// If you should ever need to change the font for the running application..
// Ugly but effective.
void
change_font(GtkWidget *widget, gpointer data)
{
    PangoFontDescription *font_desc;
    gchar *font = (gchar*)data;
    const gchar *name;

    font_desc = pango_font_description_from_string(font);
    if (font_desc == NULL) exit(1);
    gtk_widget_modify_font(widget, font_desc);
    name = ghb_get_setting_key(widget);
    g_debug("changing font for widget %s", name);
    if (GTK_IS_CONTAINER(widget))
    {
        gtk_container_foreach((GtkContainer*)widget, change_font, data);
    }
}
    //gtk_container_foreach((GtkContainer*)window, change_font, "sans 20");
#endif

// Create and bind the tree model to the tree view for the audio track list
// Also, connect up the signal that lets us know the selection has changed
static void
bind_audio_tree_model(signal_user_data_t *ud)
{
    GtkCellRenderer *source_cell;
    GtkCellRenderer *arrow_cell;
    GtkCellRenderer *output_cell;
    GtkCellRenderer *edit_cell;
    GtkCellRenderer *delete_cell;
    GtkTreeViewColumn *column;
    GtkTreeStore *treestore;
    GtkTreeView  *treeview;
    GtkTreeSelection *selection;

    ghb_log_func();
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list_view"));
    selection = gtk_tree_view_get_selection(treeview);
    treestore = gtk_tree_store_new(6, G_TYPE_STRING, G_TYPE_STRING,
                                      G_TYPE_STRING, G_TYPE_STRING,
                                      G_TYPE_STRING, G_TYPE_FLOAT);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    source_cell = gtk_cell_renderer_text_new();
    arrow_cell = gtk_cell_renderer_text_new();
    output_cell = gtk_cell_renderer_text_new();
    edit_cell = custom_cell_renderer_button_new();
    delete_cell = custom_cell_renderer_button_new();

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_spacing(column, 12);
    gtk_tree_view_column_set_title(column, _("Track Information"));
    gtk_tree_view_column_pack_start(column, source_cell, FALSE);
    gtk_tree_view_column_add_attribute(column, source_cell, "markup", 0);
    gtk_tree_view_column_add_attribute(column, source_cell, "yalign", 5);
    gtk_tree_view_column_pack_start(column, arrow_cell, FALSE);
    gtk_tree_view_column_add_attribute(column, arrow_cell, "text", 1);
    gtk_tree_view_column_pack_start(column, output_cell, TRUE);
    gtk_tree_view_column_add_attribute(column, output_cell, "markup", 2);
    gtk_tree_view_column_add_attribute(column, output_cell, "yalign", 5);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_column_set_max_width(column, 400);

    column = gtk_tree_view_column_new_with_attributes(
                                    _(" "), edit_cell, "icon-name", 3, NULL);
    //gtk_tree_view_column_set_min_width(column, 24);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    column = gtk_tree_view_column_new_with_attributes(
                                    _(" "), delete_cell, "icon-name", 4, NULL);
    //gtk_tree_view_column_set_min_width(column, 24);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    g_signal_connect(selection, "changed", G_CALLBACK(audio_list_selection_changed_cb), ud);
    g_signal_connect(edit_cell, "clicked", G_CALLBACK(audio_edit_clicked_cb), ud);
    g_signal_connect(delete_cell, "clicked", G_CALLBACK(audio_remove_clicked_cb), ud);

    g_debug("Done");
}

// Create and bind the tree model to the tree view for the subtitle track list
// Also, connect up the signal that lets us know the selection has changed
static void
bind_subtitle_tree_model(signal_user_data_t *ud)
{
    GtkCellRenderer *source_cell;
    GtkCellRenderer *arrow_cell;
    GtkCellRenderer *output_cell;
    GtkCellRenderer *edit_cell;
    GtkCellRenderer *delete_cell;
    GtkTreeViewColumn *column;
    GtkTreeStore *treestore;
    GtkTreeView  *treeview;
    GtkTreeSelection *selection;

    ghb_log_func();
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list_view"));
    selection = gtk_tree_view_get_selection(treeview);
    treestore = gtk_tree_store_new(6, G_TYPE_STRING, G_TYPE_STRING,
                                      G_TYPE_STRING, G_TYPE_STRING,
                                      G_TYPE_STRING, G_TYPE_FLOAT);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    source_cell = gtk_cell_renderer_text_new();
    arrow_cell = gtk_cell_renderer_text_new();
    output_cell = gtk_cell_renderer_text_new();
    edit_cell = custom_cell_renderer_button_new();
    delete_cell = custom_cell_renderer_button_new();

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_spacing(column, 12);
    gtk_tree_view_column_set_title(column, _("Track Information"));
    gtk_tree_view_column_pack_start(column, source_cell, FALSE);
    gtk_tree_view_column_add_attribute(column, source_cell, "markup", 0);
    gtk_tree_view_column_add_attribute(column, source_cell, "yalign", 5);
    gtk_tree_view_column_pack_start(column, arrow_cell, FALSE);
    gtk_tree_view_column_add_attribute(column, arrow_cell, "text", 1);
    gtk_tree_view_column_pack_start(column, output_cell, TRUE);
    gtk_tree_view_column_add_attribute(column, output_cell, "markup", 2);
    gtk_tree_view_column_add_attribute(column, output_cell, "yalign", 5);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_column_set_max_width(column, 400);

    column = gtk_tree_view_column_new_with_attributes(
                                    _(" "), edit_cell, "icon-name", 3, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    column = gtk_tree_view_column_new_with_attributes(
                                    _(" "), delete_cell, "icon-name", 4, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    g_signal_connect(selection, "changed", G_CALLBACK(subtitle_list_selection_changed_cb), ud);
    g_signal_connect(edit_cell, "clicked", G_CALLBACK(subtitle_edit_clicked_cb), ud);
    g_signal_connect(delete_cell, "clicked", G_CALLBACK(subtitle_remove_clicked_cb), ud);
}

#if GTK_CHECK_VERSION(4, 4, 0)
static const char * presets_drag_entries[] = {
    "widget/presets-list-row-drop"
};
#else
static GtkTargetEntry presets_drag_entries[] = {
   { "PRESETS_ROW", GTK_TARGET_SAME_WIDGET, 0 }
};
#endif

// Create and bind the tree model to the tree view for the preset list
// Also, connect up the signal that lets us know the selection has changed
static void
bind_presets_tree_model(signal_user_data_t *ud)
{
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;
    GtkTreeStore *treestore;
    GtkTreeView  *treeview;
    GtkTreeSelection *selection;

    ghb_log_func();
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
    selection = gtk_tree_view_get_selection(treeview);
    treestore = gtk_tree_store_new(5, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT,
                                   G_TYPE_STRING, G_TYPE_BOOLEAN);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Preset Name"), cell,
        "text", 0, "weight", 1, "style", 2, "editable", 4, NULL);

    g_signal_connect(cell, "edited", G_CALLBACK(preset_edited_cb), ud);

    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_set_tooltip_column(treeview, 3);

#if GTK_CHECK_VERSION(4, 4, 0)
    GdkContentFormats * targets;

    targets = gdk_content_formats_new(presets_drag_entries,
                                      G_N_ELEMENTS(presets_drag_entries));
    gtk_tree_view_enable_model_drag_dest(treeview, targets, GDK_ACTION_MOVE);
    gtk_tree_view_enable_model_drag_source(treeview, GDK_BUTTON1_MASK,
                                           targets, GDK_ACTION_MOVE);
    gdk_content_formats_unref(targets);
#else
    gtk_tree_view_enable_model_drag_dest(treeview, presets_drag_entries, 1,
                                            GDK_ACTION_MOVE);
    gtk_tree_view_enable_model_drag_source(treeview, GDK_BUTTON1_MASK,
                                           presets_drag_entries, 1,
                                           GDK_ACTION_MOVE);
#endif

    g_signal_connect(treeview, "drag_data_received", G_CALLBACK(presets_drag_data_received_cb), ud);
    g_signal_connect(treeview, "drag_motion", G_CALLBACK(presets_drag_motion_cb), ud);
    g_signal_connect(treeview, "row_expanded", G_CALLBACK(presets_row_expanded_cb), ud);
    g_signal_connect(treeview, "row_collapsed", G_CALLBACK(presets_row_expanded_cb), ud);
    g_signal_connect(selection, "changed", G_CALLBACK(presets_list_selection_changed_cb), ud);
    g_debug("Done");
}

static void
clean_old_logs (void)
{
#if !defined(_WIN32)
    const gchar *file;
    gchar *config;

    config = ghb_get_user_config_dir(NULL);

    if (g_file_test(config, G_FILE_TEST_IS_DIR))
    {
        GDir *gdir = g_dir_open(config, 0, NULL);
        file = g_dir_read_name(gdir);
        while (file)
        {
            if (strncmp(file, "Activity.log.", 13) == 0)
            {
                gchar *path;
                int fd, lock = 1;
                int pid;

                sscanf(file, "Activity.log.%d", &pid);

                path = g_strdup_printf("%s/ghb.pid.%d", config, pid);
                if (g_file_test(path, G_FILE_TEST_EXISTS))
                {
                    fd = open(path, O_RDWR);
                    if (fd >= 0)
                    {
                        lock = lockf(fd, F_TLOCK, 0);
                    }
                    g_free(path);
                    close(fd);
                    if (lock == 0)
                    {
                        path = g_strdup_printf("%s/%s", config, file);
                        g_unlink(path);
                        g_free(path);
                    }
                }
                else
                {
                    g_free(path);
                    path = g_strdup_printf("%s/%s", config, file);
                    g_unlink(path);
                    g_free(path);
                }
            }
            file = g_dir_read_name(gdir);
        }
        g_dir_close(gdir);
    }
    g_free(config);
#else
    const gchar *file;
    gchar *config;

    config = ghb_get_user_config_dir(NULL);

    if (g_file_test(config, G_FILE_TEST_IS_DIR))
    {
        GDir *gdir = g_dir_open(config, 0, NULL);
        file = g_dir_read_name(gdir);
        while (file)
        {
            if (strncmp(file, "Activity.log.", 13) == 0)
            {
                gchar *path;
                int pid;

                sscanf(file, "Activity.log.%d", &pid);

#if 0
                int fd, lock = 1;

                path = g_strdup_printf("%s/ghb.pid.%d", config, pid);
                if (g_file_test(path, G_FILE_TEST_EXISTS))
                {
                    fd = open(path, O_RDWR);
                    if (fd >= 0)
                    {
                        lock = lockf(fd, F_TLOCK, 0);
                    }
                    g_free(path);
                    close(fd);
                    if (lock == 0)
                    {
                        path = g_strdup_printf("%s/%s", config, file);
                        g_unlink(path);
                        g_free(path);
                    }
                }
                else
#endif
                {
                    //g_free(path);
                    path = g_strdup_printf("%s/%s", config, file);
                    g_unlink(path);
                    g_free(path);
                }
            }
            file = g_dir_read_name(gdir);
        }
        g_dir_close(gdir);
    }
    g_free(config);
#endif
}

static void
IoRedirect(signal_user_data_t *ud)
{
    GIOChannel *channel;
    gint pfd[2];
    gchar *config, *path, *str;
    pid_t pid;

    // I'm opening a pipe and attaching the writer end to stderr
    // The reader end will be polled by main event loop and I'll get
    // a callback when there is data available.
    if (pipe( pfd ) < 0)
    {
        g_warning("Failed to redirect IO. Logging impaired\n");
        return;
    }
    clean_old_logs();
    // Open activity log.
    config = ghb_get_user_config_dir(NULL);
    pid = getpid();
    path = g_strdup_printf("%s/Activity.log.%"PRId64, config, (int64_t)pid);
    ud->activity_log = g_io_channel_new_file (path, "w", NULL);
    ud->job_activity_log = NULL;
    str = g_strdup_printf("<big><b>%s</b></big>", path);
    ghb_ui_update(ud, "activity_location", ghb_string_value(str));
    g_free(str);
    g_free(path);
    g_free(config);
    if (ud->activity_log != NULL)
    {
        // Set encoding to raw.
        g_io_channel_set_encoding(ud->activity_log, NULL, NULL);
    }
    // redirect stderr to the writer end of the pipe

#if defined(_WIN32)
    _dup2(pfd[1], _fileno(stderr));
#else
    dup2(pfd[1], STDERR_FILENO);
#endif
    setvbuf(stderr, NULL, _IONBF, 0);

#if defined(_WIN32)
    channel = g_io_channel_win32_new_fd(pfd[0]);
#else
    channel = g_io_channel_unix_new(pfd[0]);
#endif

    // I was getting an this error:
    // "Invalid byte sequence in conversion input"
    // Set disable encoding on the channel.
    g_io_channel_set_encoding(channel, NULL, NULL);
    ud->stderr_src_id =
        g_io_add_watch(channel, G_IO_IN, ghb_log_cb, (gpointer)ud );
}

static gchar *dvd_device = NULL;
static gchar *arg_preset = NULL;
static gboolean ghb_debug = FALSE;
static gchar *arg_config_dir = NULL;
#if defined(_WIN32)
static gboolean win32_console = FALSE;
#endif

static GOptionEntry entries[] =
{
    { "device", 'd', 0, G_OPTION_ARG_FILENAME, &dvd_device, N_("The device or file to encode"), NULL },
    { "preset", 'p', 0, G_OPTION_ARG_STRING, &arg_preset, N_("The preset values to use for encoding"), NULL },
    { "debug",  'x', 0, G_OPTION_ARG_NONE, &ghb_debug, N_("Spam a lot"), NULL },
    { "config", 'o', 0, G_OPTION_ARG_STRING, &arg_config_dir, N_("The path to override user config dir"), NULL },
#if defined(_WIN32)
    { "console",'c', 0, G_OPTION_ARG_NONE, &win32_console, N_("Open a console for debug output"), NULL },
#endif
    { NULL }
};

G_MODULE_EXPORT void drive_changed_cb(GVolumeMonitor *gvm, GDrive *gd, signal_user_data_t *ud);

#if defined(_WIN32)
G_MODULE_EXPORT GdkFilterReturn
win_message_cb(GdkXEvent *wmevent, GdkEvent *event, gpointer data)
{
    signal_user_data_t *ud = (signal_user_data_t*)data;
    MSG *msg = (MSG*)wmevent;

    if (msg->message == WM_DEVICECHANGE)
    {
        wm_drive_changed(wmevent, ud);
    }
    return GDK_FILTER_CONTINUE;
}
#endif

static void
watch_volumes(signal_user_data_t *ud)
{
#if !defined(_WIN32)
    GVolumeMonitor *gvm;
    gvm = g_volume_monitor_get();

    g_signal_connect(gvm, "drive-changed", (GCallback)drive_changed_cb, ud);
#else
    GdkWindow *window;
    GtkWidget *widget;

    widget = GHB_WIDGET(ud->builder, "hb_window");
    window = gtk_widget_get_parent_window(widget);
    gdk_window_add_filter(window, win_message_cb, ud);
#endif
}

G_MODULE_EXPORT void video_option_changed_cb(GtkWidget *widget, signal_user_data_t *ud);
G_MODULE_EXPORT void plot_changed_cb(GtkWidget *widget, signal_user_data_t *ud);
G_MODULE_EXPORT void position_overlay_cb(GtkWidget *widget, signal_user_data_t *ud);
G_MODULE_EXPORT void preview_hud_size_alloc_cb(GtkWidget *widget, signal_user_data_t *ud);

// Important: any widgets named in CSS must have their widget names set
// below before setting CSS properties on them.
const gchar *MyCSS =
"@define-color black  #000000;"
"@define-color gray18 #2e2e2e;"
"@define-color gray22 #383838;"
"@define-color gray26 #424242;"
"@define-color gray32 #525252;"
"@define-color gray40 #666666;"
"@define-color gray46 #757575;"
"@define-color white  #ffffff;"

".progress-overlay"
"{"
"    min-height:1px;"
"}"

".progress-overlay trough"
"{"
"    border-radius: 0px;"
"    border-color: transparent;"
"    border-width: 1px 0px;"
"    min-height: 3px;"
"}"

".progress-overlay progress"
"{"
"    border-radius: 0px;"
"    border-width: 1px;"
"    min-height: 3px;"
"}"

"#preview_hud"
"{"
"    border-radius: 16px;"
"    border-width: 1px;"
"}"

".preview-image-frame"
"{"
"    background-color: black;"
"}"

"textview"
"{"
"    padding: 5px 5px 5px 5px;"
"}"

".entry"
"{"
"    margin: 0px 5px 0px 5px;"
"    padding: 0px 0px 0px 0px;"
"}"

"stackswitcher button.text-button"
"{"
"    min-width: 50px;"
"}"

"#activity_view"
"{"
"    font-family: monospace;"
"    font-size: 8pt;"
"    font-weight: 300;"
"}"

".row:not(:first-child)"
"{"
"    border-top: 1px solid transparent; "
"    border-bottom: 1px solid transparent; "
"}"
".row:first-child"
"{"
"    border-top: 1px solid transparent; "
"    border-bottom: 1px solid transparent; "
"}"
".row:last-child"
"{"
"    border-top: 1px solid transparent; "
"    border-bottom: 1px solid transparent; "
"}"
".row.drag-icon"
"{"
"    background: white; "
"    border: 1px solid black; "
"}"
".row.drag-row"
"{"
"    color: gray; "
"    background: alpha(gray,0.2); "
"}"
".row.drag-row.drag-hover"
"{"
"    border-top: 1px solid #4e9a06; "
"    border-bottom: 1px solid #4e9a06; "
"}"
".row.drag-hover image, "
".row.drag-hover label"
"{"
"    color: #4e9a06; "
"}"
".row.drag-hover-top"
"{"
"    border-top: 1px solid #4e9a06; "
"}"
".row.drag-hover-bottom"
"{"
"    border-bottom: 1px solid #4e9a06; "
"}"
#if !GTK_CHECK_VERSION(3, 24, 0)
"toolbar"
"{"
"    -gtk-icon-style: symbolic; "
"}"
#endif
;

extern G_MODULE_EXPORT void status_icon_query_tooltip_cb(void);

extern G_MODULE_EXPORT void
ghb_shutdown_cb(GApplication * app, signal_user_data_t *ud)
{
}

G_MODULE_EXPORT void
dvd_source_activate_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
source_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
source_dir_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
destination_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
preferences_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
quit_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
title_add_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
title_add_multiple_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
title_add_all_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
queue_start_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
queue_pause_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
queue_play_file_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
queue_export_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
queue_import_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
queue_move_top_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
queue_move_bottom_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
queue_open_source_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
queue_open_dest_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
queue_open_log_dir_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
queue_open_log_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
queue_delete_all_action_cb(GSimpleAction *action, GVariant *param,
                           gpointer ud);
G_MODULE_EXPORT void
queue_delete_complete_action_cb(GSimpleAction *action, GVariant *param,
                                gpointer ud);
G_MODULE_EXPORT void
queue_delete_action_cb(GSimpleAction *action, GVariant *param,
                                gpointer ud);
G_MODULE_EXPORT void
queue_reset_fail_action_cb(GSimpleAction *action, GVariant *param,
                           gpointer ud);
G_MODULE_EXPORT void
queue_reset_all_action_cb(GSimpleAction *action, GVariant *param,
                          gpointer ud);
G_MODULE_EXPORT void
queue_reset_action_cb(GSimpleAction *action, GVariant *param,
                      gpointer ud);
G_MODULE_EXPORT void
queue_edit_action_cb(GSimpleAction *action, GVariant *param,
                     gpointer ud);
G_MODULE_EXPORT void
show_presets_action_cb(GSimpleAction *action, GVariant *value, gpointer ud);
G_MODULE_EXPORT void
hbfd_action_cb(GSimpleAction *action, GVariant *value, gpointer ud);
G_MODULE_EXPORT void
show_queue_action_cb(GSimpleAction *action, GVariant *value, gpointer ud);
G_MODULE_EXPORT void
show_preview_action_cb(GSimpleAction *action, GVariant *value, gpointer ud);
G_MODULE_EXPORT void
show_activity_action_cb(GSimpleAction *action, GVariant *value, gpointer ud);
G_MODULE_EXPORT void
preset_save_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
preset_save_as_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
preset_rename_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
preset_remove_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
preset_default_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
preset_export_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
preset_import_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
presets_reload_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
preview_fullscreen_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
about_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
guide_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
preset_select_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
preset_reload_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
chapters_export_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);
G_MODULE_EXPORT void
chapters_import_action_cb(GSimpleAction *action, GVariant *param, gpointer ud);

static void map_actions(GApplication * app, signal_user_data_t * ud)
{
    const GActionEntry entries[] =
    {
        { "source",                source_action_cb                },
        { "source-dir",            source_dir_action_cb            },
        { "destination",           destination_action_cb           },
        { "preferences",           preferences_action_cb           },
        { "quit",                  quit_action_cb                  },
        { "add-current",           title_add_action_cb             },
        { "add-multiple",          title_add_multiple_action_cb    },
        { "add-all",               title_add_all_action_cb         },
        { "queue-start",           queue_start_action_cb           },
        { "queue-pause",           queue_pause_action_cb           },
        { "queue-play-file",       queue_play_file_action_cb       },
        { "queue-move-top",        queue_move_top_action_cb        },
        { "queue-move-bottom",     queue_move_bottom_action_cb     },
        { "queue-open-source",     queue_open_source_action_cb     },
        { "queue-open-dest",       queue_open_dest_action_cb       },
        { "queue-open-log-dir",    queue_open_log_dir_action_cb    },
        { "queue-open-log",        queue_open_log_action_cb        },
        { "queue-reset-fail",      queue_reset_fail_action_cb      },
        { "queue-reset-all",       queue_reset_all_action_cb       },
        { "queue-reset",           queue_reset_action_cb           },
        { "queue-delete",          queue_delete_action_cb          },
        { "queue-delete-complete", queue_delete_complete_action_cb },
        { "queue-delete-all",      queue_delete_all_action_cb      },
        { "queue-export",          queue_export_action_cb          },
        { "queue-import",          queue_import_action_cb          },
        { "queue-edit",            queue_edit_action_cb            },
        { "dvd-open",              dvd_source_activate_cb, "s"     },
        { "hbfd",                  NULL,
          NULL, "false",           hbfd_action_cb                  },
        { "show-presets",          show_presets_action_cb          },
        { "show-queue",            show_queue_action_cb            },
        { "show-preview",          show_preview_action_cb          },
        { "show-activity",         show_activity_action_cb         },
        { "preset-save",           preset_save_action_cb           },
        { "preset-save-as",        preset_save_as_action_cb        },
        { "preset-rename",         preset_rename_action_cb         },
        { "preset-remove",         preset_remove_action_cb         },
        { "preset-default",        preset_default_action_cb        },
        { "preset-export",         preset_export_action_cb         },
        { "preset-import",         preset_import_action_cb         },
        { "presets-reload",        presets_reload_action_cb        },
        { "about",                 about_action_cb                 },
        { "guide",                 guide_action_cb                 },
        { "preset-select",         preset_select_action_cb, "s"    },
        { "preset-reload",         preset_reload_action_cb         },
        { "preview-fullscreen",    NULL,
          NULL, "false",           preview_fullscreen_action_cb    },
        { "chapters-export",       chapters_export_action_cb       },
        { "chapters-import",       chapters_import_action_cb       },
    };
    g_action_map_add_action_entries(G_ACTION_MAP(app), entries,
                                    G_N_ELEMENTS(entries), ud);
}

gboolean
ghb_idle_ui_init(signal_user_data_t *ud)
{
    ghb_settings_to_ui(ud, ud->globals);
    ghb_settings_to_ui(ud, ud->prefs);
    // Note that ghb_settings_to_ui(ud->settings) happens when initial
    // empty title is initialized.

    // Init settings that are dependent on command line args
    if (dvd_device != NULL)
    {
        // Source overridden from command line option
        ghb_dict_set_string(ud->globals, "scan_source", dvd_device);
        g_idle_add((GSourceFunc)ghb_idle_scan, ud);
    }
    else
    {
        GhbValue *gval = ghb_dict_get_value(ud->prefs, "default_source");
        ghb_dict_set(ud->globals, "scan_source", ghb_value_dup(gval));
    }

    if (arg_preset != NULL)
    {
        ghb_select_preset(ud, arg_preset, HB_PRESET_TYPE_ALL);
    }
    else
    {
        ghb_select_default_preset(ud);
    }

    // Grey out widgets that are dependent on a disabled feature
    ghb_check_all_dependencies(ud);

    return FALSE;
}

extern G_MODULE_EXPORT void easter_egg_multi_cb (GtkGesture *gest, gint n_press, gdouble x,
                                                 gdouble y, signal_user_data_t *ud);
extern G_MODULE_EXPORT void preview_click_cb (GtkGesture *gest, gint n_press, gdouble x,
                                              gdouble y, signal_user_data_t *ud);
extern G_MODULE_EXPORT void on_presets_list_press_cb (GtkGesture *gest, gint n_press, gdouble x,
                                                      gdouble y, signal_user_data_t *ud);
extern G_MODULE_EXPORT void queue_button_press_cb (GtkGesture *gest, gint n_press, gdouble x,
                                                   gdouble y, signal_user_data_t *ud);
#if GTK_CHECK_VERSION(4, 4, 0)
extern G_MODULE_EXPORT void easter_egg_multi_cb(void);
extern G_MODULE_EXPORT void preview_leave_cb(void);
extern G_MODULE_EXPORT void preview_motion_cb(void);
extern G_MODULE_EXPORT void preview_draw_cb(GtkDrawingArea*, cairo_t*, int, int,
                                            gpointer);
extern G_MODULE_EXPORT void hud_enter_cb(void);
extern G_MODULE_EXPORT void hud_leave_cb(void);
#endif

static void
video_file_drop_received (GtkWidget *widget, GdkDragContext *context,
                          int x, int y, GtkSelectionData *data, guint info,
                          guint time, signal_user_data_t *ud)
{
    gchar **uris;
    GFile *file = NULL;
    gchar *filename = NULL;

    uris = gtk_selection_data_get_uris(data);
    if (uris != NULL)
    {
        // Only one file or folder at a time is supported
        file = g_file_new_for_uri(uris[0]);
        g_strfreev(uris);
    }
    if (file != NULL)
    {
        filename = g_file_get_path(file);
        g_object_unref(file);
    }
    if (filename != NULL)
    {
        g_debug("File dropped on window: %s", filename);
        ghb_dict_set_string(ud->prefs, "default_source", filename);
        ghb_pref_save(ud->prefs, "default_source");
        ghb_dvd_set_current(filename, ud);
        ghb_do_scan(ud, filename, 0, TRUE);
        g_free(filename);
    }
}

static void
video_file_drop_init (signal_user_data_t *ud)
{
    GtkWidget *summary = GHB_WIDGET (ud->builder, "hb_window");

    gtk_drag_dest_set(summary, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);
    gtk_drag_dest_add_uri_targets(summary);
    g_signal_connect(summary, "drag-data-received", G_CALLBACK(video_file_drop_received), ud);
}

static void
print_system_information (void)
{
#if GLIB_CHECK_VERSION(2, 64, 0)
    fprintf(stderr, "OS: %s\n", g_get_os_info(G_OS_INFO_KEY_PRETTY_NAME));
#endif
#ifndef _WIN32
    char *exe_path;
    ssize_t result;
    struct utsname *host_info;

    host_info = calloc(1, sizeof(struct utsname));
    uname(host_info);

    fprintf(stderr, "%s\n", HB_PROJECT_TITLE);
    fprintf(stderr, "Kernel: %s %s (%s)\n", host_info->sysname,
            host_info->release, host_info->machine);
    fprintf(stderr, "CPU: %s x %d\n", hb_get_cpu_name(), hb_get_cpu_count());
    free(host_info);
    exe_path = calloc(1, PATH_MAX);
    result = readlink( "/proc/self/exe", exe_path, PATH_MAX);
    if (result > 0)
    {
        fprintf(stderr, "Install Dir: %s\n", g_path_get_dirname(exe_path));
    }
    free(exe_path);
#endif
    fprintf(stderr, "Config Dir:  %s\n", ghb_get_user_config_dir(NULL));
    fprintf(stderr, "_______________________________\n\n");
}

extern G_MODULE_EXPORT void
ghb_activate_cb(GApplication * app, signal_user_data_t * ud)
{
    GtkCssProvider     * provider = gtk_css_provider_new();

    ghb_css_provider_load_from_data(provider, MyCSS, -1);
    color_scheme_set_async(APP_PREFERS_LIGHT);

#if GTK_CHECK_VERSION(4, 4, 0)
    GdkDisplay *dd = gdk_display_get_default();
    gtk_style_context_add_provider_for_display(dd,
                                GTK_STYLE_PROVIDER(provider),
                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#else
    GdkScreen *ss = gdk_screen_get_default();
    gtk_style_context_add_provider_for_screen(ss,
                                GTK_STYLE_PROVIDER(provider),
                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#endif

    g_object_unref(provider);

    ghb_resource_init();
    ghb_load_icons();

    // Override user config dir
    if (arg_config_dir != NULL)
    {
        ghb_override_user_config_dir(arg_config_dir);
    }

    // map application actions (menu callbacks)
    map_actions(app, ud);

    // connect shutdown signal for cleanup
    g_signal_connect(app, "shutdown", (GCallback)ghb_shutdown_cb, ud);

#if GLIB_CHECK_VERSION(2, 72, 0)
    g_log_set_debug_enabled(ghb_debug);
#endif
    ud->globals = ghb_dict_new();
    ud->prefs = ghb_dict_new();
    ud->settings_array = ghb_array_new();
    ud->settings = ghb_dict_new();
    ghb_array_append(ud->settings_array, ud->settings);

    ud->builder = create_builder_or_die(BUILDER_NAME);

    // Initialize D-Bus connections to monitor power settings
    ghb_power_manager_init(ud);

    // Enable drag & drop in queue list
    ghb_queue_drag_n_drop_init(ud);
    video_file_drop_init(ud);

    // Enable events that alert us to media change events
    watch_volumes(ud);

    GtkWidget *widget;

    // Get GtkTextBuffers for activity logs
    widget = GHB_WIDGET(ud->builder, "activity_view");
    ud->activity_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    g_object_ref(ud->activity_buffer);
    widget = GHB_WIDGET(ud->builder, "queue_activity_view");
    ud->extra_activity_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    g_object_ref(ud->extra_activity_buffer);
    ud->queue_activity_buffer = gtk_text_buffer_new(NULL);

    // Must set the names of the widgets that I want to modify
    // style for.
    gtk_widget_set_name(GHB_WIDGET(ud->builder, "preview_hud"), "preview_hud");
    gtk_widget_set_name(GHB_WIDGET(ud->builder, "activity_view"), "activity_view");

    // Redirect stderr to the activity window
    ghb_preview_init(ud);
    IoRedirect(ud);
    print_system_information();
    ghb_init_dep_map();

    GtkTextView   * textview;
    GtkTextBuffer * buffer;

    textview = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "VideoOptionExtra"));
    buffer = gtk_text_view_get_buffer(textview);
    g_signal_connect(buffer, "changed", (GCallback)video_option_changed_cb, ud);

    textview = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "MetaLongDescription"));
    buffer = gtk_text_view_get_buffer(textview);
    g_signal_connect(buffer, "changed", (GCallback)plot_changed_cb, ud);

    // Initialize HB internal tables etc.
    hb_global_init();

    // Set up UI combo boxes.  Some of these rely on HB global settings.
    ghb_combo_init(ud);

    g_debug("ud %p", ud);
    g_debug("ud->builder %p", ud->builder);

    bind_audio_tree_model(ud);
    bind_subtitle_tree_model(ud);
    bind_presets_tree_model(ud);

    // Connect up the signals to their callbacks
    // I wrote my own connector so that I could pass user data
    // to the callbacks.  Builder's standard autoconnect doesn't all this.
    gtk_builder_connect_signals_full(ud->builder, MyConnect, ud);

    ghb_init_audio_defaults_ui(ud);
    ghb_init_subtitle_defaults_ui(ud);

    // Load prefs before presets.  Some preset defaults may depend
    // on preference settings.
    // First load default values
    ghb_settings_init(ud->prefs, "Preferences");
    ghb_settings_init(ud->globals, "Globals");
    ghb_settings_init(ud->settings, "Initialization");
    ghb_settings_init(ud->settings, "OneTimeInitialization");
    // Load user preferences file
    ghb_prefs_load(ud);
    // Store user preferences into ud->prefs
    ghb_prefs_to_settings(ud->prefs);

    if (ghb_dict_get_bool(ud->prefs, "CustomTmpEnable"))
    {
        const char * tmp_dir;

        tmp_dir = ghb_dict_get_string(ud->prefs, "CustomTmpDir");
        if (tmp_dir != NULL && tmp_dir[0] != 0)
        {
#if defined(_WIN32)
    // Tell gdk pixbuf where it's loader config file is.
            _putenv_s("TEMP", tmp_dir);
#else
            setenv("TEMP", tmp_dir, 1);
#endif
        }
    }
    int logLevel = ghb_dict_get_int(ud->prefs, "LoggingLevel");

    // Initialize HB work threads
    ghb_backend_init(logLevel);

    // Load the presets files
    ghb_presets_load(ud);

    // GActions associated with widgets do not fire when the widget
    // is changed from this GtkApplication "activate" signal.
    // So initialize UI when idle.
    g_idle_add((GSourceFunc)ghb_idle_ui_init, ud);

    const gchar *source = ghb_dict_get_string(ud->prefs, "default_source");
    ghb_dvd_set_current(source, ud);

    // Populate the presets tree view
    ghb_presets_list_init(ud, NULL);
    ghb_presets_menu_init(ud);

    // Reload and check status of the last saved queue
    g_idle_add((GSourceFunc)ghb_reload_queue, ud);

    // Start timer for monitoring libhb status, 500ms
    g_timeout_add(200, ghb_timer_cb, (gpointer)ud);

    // Add dvd devices to File menu
    ghb_volname_cache_init();
    GHB_THREAD_NEW("Cache Volume Names", (GThreadFunc)ghb_cache_volnames, ud);

    GtkWidget *ghb_window = GHB_WIDGET(ud->builder, "hb_window");

    gint window_width, window_height;
    window_width = ghb_dict_get_int(ud->prefs, "window_width");
    window_height = ghb_dict_get_int(ud->prefs, "window_height");

    // Grrrr!  Gtk developers !!!hard coded!!! the width of the
    // radio buttons in GtkStackSwitcher to 100!!!
    //
    // Thankfully, GtkStackSwitcher is a regular container object
    // and we can access the buttons to change their width.
    GList *stack_switcher_children, *link;
    GtkContainer * stack_switcher = GTK_CONTAINER(
                            GHB_WIDGET(ud->builder, "SettingsStackSwitcher"));
    link = stack_switcher_children = gtk_container_get_children(stack_switcher);
    while (link != NULL)
    {
        GtkWidget *widget = link->data;
        gtk_widget_set_size_request(widget, -1, -1);
        gtk_widget_set_hexpand(widget, TRUE);
        gtk_widget_set_halign(widget, GTK_ALIGN_FILL);
        link = link->next;
    }
    g_list_free(stack_switcher_children);

    gtk_window_resize(GTK_WINDOW(ghb_window), window_width, window_height);

    ghb_set_custom_filter_tooltip(ud, "PictureDetelecineCustom",
                                  "detelecine", HB_FILTER_DETELECINE);
    ghb_set_custom_filter_tooltip(ud, "PictureCombDetectCustom",
                                  "interlace detection", HB_FILTER_COMB_DETECT);
    ghb_set_custom_filter_tooltip(ud, "PictureColorspaceCustom",
                                  "colorspace", HB_FILTER_COLORSPACE);
    ghb_set_custom_filter_tooltip(ud, "PictureChromaSmoothCustom",
                                  "chroma smooth", HB_FILTER_CHROMA_SMOOTH);

    gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(ghb_window));
    GtkWidget * window = GHB_WIDGET(ud->builder, "presets_window");
    gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(window));
    window = GHB_WIDGET(ud->builder, "queue_window");
    gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(window));
    window = GHB_WIDGET(ud->builder, "preview_window");
    gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(window));

    GMenuModel *menu = G_MENU_MODEL(gtk_builder_get_object(ud->builder, "handbrake-menu"));
    gtk_application_set_menubar(GTK_APPLICATION(app), menu);

    GtkGesture *gest;

    // Easter egg multi-click
    widget = GHB_WIDGET(ud->builder, "eventbox1");
    gest = ghb_gesture_click_new(widget);
    g_signal_connect(gest, "pressed", G_CALLBACK(easter_egg_multi_cb), ud);

    // Preview fullscreen multi-click
    widget = GHB_WIDGET(ud->builder, "preview_image");
    gest = ghb_gesture_click_new(widget);
    g_signal_connect(gest, "pressed", G_CALLBACK(preview_click_cb), ud);

    // Presets list context menu
    widget = GHB_WIDGET(ud->builder, "presets_list");
    gest = ghb_gesture_click_new(widget);
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gest), 3);
    g_signal_connect(gest, "pressed", G_CALLBACK(on_presets_list_press_cb), ud);

    // Queue list context menu
    widget = GHB_WIDGET(ud->builder, "queue_list");
    gest = ghb_gesture_click_new(widget);
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gest), 0);
    g_signal_connect(gest, "pressed", G_CALLBACK(queue_button_press_cb), ud);

#if GTK_CHECK_VERSION(4, 4, 0)
    GtkEventController * econ;

    // Preview HUD popup management via mouse motion
    econ = gtk_event_controller_motion_new();
    widget = GHB_WIDGET(ud->builder, "preview_image");
    gtk_widget_add_controller(widget, econ);
    g_signal_connect(econ, "leave", preview_leave_cb, ud);
    g_signal_connect(econ, "motion", preview_motion_cb, ud);

    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(widget), preview_draw_cb,
                                   ud, NULL);

    econ = gtk_event_controller_motion_new();
    widget = GHB_WIDGET(ud->builder, "preview_hud");
    gtk_widget_add_controller(widget, econ);
    g_signal_connect(econ, "enter", hud_enter_cb, ud);
    g_signal_connect(econ, "leave", hud_leave_cb, ud);
#endif

    gtk_widget_show(ghb_window);
}

extern G_MODULE_EXPORT void
ghb_open_file_cb(GApplication *app, gpointer pfiles, gint nfiles,
                 gchar *hint, signal_user_data_t * ud)
{
    GFile ** files = pfiles;

    if (nfiles < 1)
        return;

    if (dvd_device == NULL)
    {
        dvd_device = g_file_get_path(files[0]);
    }
    ghb_activate_cb(app, ud);
}

int
main(int argc, char *argv[])
{
#if defined(_WIN32)
    // Tell gdk pixbuf where it's loader config file is.
    _putenv_s("GDK_PIXBUF_MODULE_FILE", "ghb.exe.local/loaders.cache");
    _putenv_s("GST_PLUGIN_PATH", "lib/gstreamer-1.0");
#endif


#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
#endif

#if defined(_WIN32)
    if (win32_console)
    {
        // Enable console logging
        if(AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()){
            close(STDOUT_FILENO);
            (void) freopen("CONOUT$", "w", stdout);
            close(STDERR_FILENO);
            (void) freopen("CONOUT$", "w", stderr);
        }
    }
    else
    {
        // Non-console windows apps do not have a stderr->_file
        // assigned properly
        stderr->_file = STDERR_FILENO;
        stdout->_file = STDOUT_FILENO;
    }
#endif

    int                  status;
    signal_user_data_t * ud;

    ghb_ui_register_resource();
    ud = g_malloc0(sizeof(signal_user_data_t));
    ud->app = gtk_application_new("fr.handbrake.ghb",
                                  G_APPLICATION_NON_UNIQUE |
                                  G_APPLICATION_HANDLES_OPEN);
    g_set_prgname("fr.handbrake.ghb");
    g_set_application_name("HandBrake");
    // Connect application signals
    g_signal_connect(ud->app, "activate", (GCallback)ghb_activate_cb, ud);
    g_signal_connect(ud->app, "open", (GCallback)ghb_open_file_cb, ud);
    // Set application options
    g_application_add_main_option_entries(G_APPLICATION(ud->app), entries);
#if defined(_ENABLE_GST)
    g_application_add_option_group(G_APPLICATION(ud->app),
                                   gst_init_get_option_group());
#endif
    status = g_application_run(G_APPLICATION(ud->app), argc, argv);

    ghb_backend_close();

    // Remove stderr redirection
    if (ud->stderr_src_id > 0)
        g_source_remove(ud->stderr_src_id);
    ghb_value_free(&ud->queue);
    ghb_value_free(&ud->settings_array);
    ghb_value_free(&ud->prefs);
    ghb_value_free(&ud->globals);

    if (ud->activity_log != NULL)
        g_io_channel_unref(ud->activity_log);
    ghb_settings_close();
    ghb_resource_free();

    if (ud->builder != NULL)
        g_object_unref(ud->builder);

    ghb_power_manager_dispose(ud);

    g_object_unref(ud->extra_activity_buffer);
    g_object_unref(ud->queue_activity_buffer);
    g_object_unref(ud->activity_buffer);
    g_free(ud->extra_activity_path);

    g_free(ud->current_dvd_device);
    g_free(ud);

    return status;
}
