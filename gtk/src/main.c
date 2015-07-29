/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) John Stebbins 2008-2015 <stebbins@stebbins>
 *
 * main.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
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

#include <config.h>

#include "ghbcompat.h"

#if defined(_ENABLE_GST)
#include <gst/gst.h>
#endif

#if !defined(_WIN32)
#include <libnotify/notify.h>
#include <dbus/dbus-glib.h>
#else
#include <windows.h>
#include <io.h>
#define pipe(phandles)  _pipe (phandles, 4096, _O_BINARY)
#endif

#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "hb.h"
#include "renderer_button.h"
#include "hb-backend.h"
#include "ghb-dvd.h"
#include "ghbcellrenderertext.h"
#include "values.h"
#include "icons.h"
#include "callbacks.h"
#include "queuehandler.h"
#include "audiohandler.h"
#include "subtitlehandler.h"
#include "x264handler.h"
#include "settings.h"
#include "resources.h"
#include "presets.h"
#include "preview.h"
#include "ui_res.h"


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

GtkBuilder*
create_builder_or_die(const gchar * name)
{
    guint res = 0;
    GError *error = NULL;
    const gchar *ghb_ui;
    gsize data_size;

    ghb_ui_register_resource();
    GResource *ui_res = ghb_ui_get_resource();
    GBytes *gbytes = g_resource_lookup_data(ui_res, "/org/handbrake/ui/ghb.ui",
                                            0, NULL);
    ghb_ui = g_bytes_get_data(gbytes, &data_size);

    const gchar *markup =
        N_("<b><big>Unable to create %s.</big></b>\n"
        "\n"
        "Internal error. Could not parse UI description.\n"
        "%s");
    g_debug("create_builder_or_die()\n");
    GtkBuilder *xml = gtk_builder_new();
    if (xml != NULL)
        res = gtk_builder_add_from_string(xml, ghb_ui, -1, &error);
    if (!xml || !res)
    {
        GtkWidget *dialog = gtk_message_dialog_new_with_markup(NULL,
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
    g_debug("handler_name %s", handler_name);
    g_debug("signal_name %s", signal_name);
    callback = self_symbol_lookup(handler_name);
    if (!callback)
    {
        g_message("Signal handler (%s) not found", handler_name);
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
    g_debug("changing font for widget %s\n", name);
    if (GTK_IS_CONTAINER(widget))
    {
        gtk_container_foreach((GtkContainer*)widget, change_font, data);
    }
}
    //gtk_container_foreach((GtkContainer*)window, change_font, "sans 20");
#endif

extern G_MODULE_EXPORT void chapter_edited_cb(void);
extern G_MODULE_EXPORT void chapter_keypress_cb(void);

// Create and bind the tree model to the tree view for the chapter list
// Also, connect up the signal that lets us know the selection has changed
static void
bind_chapter_tree_model(signal_user_data_t *ud)
{
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;
    GtkListStore *treestore;
    GtkTreeView  *treeview;

    g_debug("bind_chapter_tree_model()\n");
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "chapters_list"));
    treestore = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    cell = ghb_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _("Index"), cell, "text", 0, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    cell = ghb_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _("Start"), cell, "text", 1, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    cell = ghb_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _("Duration"), cell, "text", 2, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    cell = ghb_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                    _("Title"), cell, "text", 3, "editable", 4, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    g_signal_connect(cell, "key-press-event", chapter_keypress_cb, ud);
    g_signal_connect(cell, "edited", chapter_edited_cb, ud);
    g_debug("Done\n");
}


extern G_MODULE_EXPORT void queue_list_selection_changed_cb(void);
extern G_MODULE_EXPORT void queue_remove_clicked_cb(void);
extern G_MODULE_EXPORT void queue_list_size_allocate_cb(void);
extern G_MODULE_EXPORT void queue_drag_cb(void);
extern G_MODULE_EXPORT void queue_drag_motion_cb(void);

// Create and bind the tree model to the tree view for the queue list
// Also, connect up the signal that lets us know the selection has changed
static void
bind_queue_tree_model(signal_user_data_t *ud)
{
    GtkCellRenderer *cell, *textcell;
    GtkTreeViewColumn *column;
    GtkTreeStore *treestore;
    GtkTreeView  *treeview;
    GtkTreeSelection *selection;
    GtkTargetEntry SrcEntry;
    SrcEntry.target = "DATA";
    SrcEntry.flags = GTK_TARGET_SAME_WIDGET;

    g_debug("bind_queue_tree_model()\n");
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
    selection = gtk_tree_view_get_selection(treeview);
    treestore = gtk_tree_store_new(5, G_TYPE_BOOLEAN, G_TYPE_STRING,
                                      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Job Information"));
    cell = gtk_cell_renderer_spinner_new();
    gtk_tree_view_column_pack_start(column, cell, FALSE);
    gtk_tree_view_column_add_attribute(column, cell, "active", 0);
    gtk_tree_view_column_add_attribute(column, cell, "pulse", 4);
    cell = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, cell, FALSE);
    gtk_tree_view_column_add_attribute(column, cell, "icon-name", 1);
    textcell = gtk_cell_renderer_text_new();
    g_object_set(textcell, "wrap-mode", PANGO_WRAP_CHAR, NULL);
    g_object_set(textcell, "wrap-width", 500, NULL);
    gtk_tree_view_column_pack_start(column, textcell, TRUE);
    gtk_tree_view_column_add_attribute(column, textcell, "markup", 2);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_column_set_max_width(column, 550);
    g_signal_connect(treeview, "size-allocate", queue_list_size_allocate_cb,
                        textcell);

    cell = custom_cell_renderer_button_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _(""), cell, "icon-name", 3, NULL);
    gtk_tree_view_column_set_min_width(column, 24);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    gtk_tree_view_enable_model_drag_dest(treeview, &SrcEntry, 1,
                                            GDK_ACTION_MOVE);
    gtk_tree_view_enable_model_drag_source(treeview, GDK_BUTTON1_MASK,
                                            &SrcEntry, 1, GDK_ACTION_MOVE);

    g_signal_connect(selection, "changed", queue_list_selection_changed_cb, ud);
    g_signal_connect(cell, "clicked", queue_remove_clicked_cb, ud);
    g_signal_connect(treeview, "drag_data_received", queue_drag_cb, ud);
    g_signal_connect(treeview, "drag_motion", queue_drag_motion_cb, ud);
}

extern G_MODULE_EXPORT void audio_list_selection_changed_cb(void);
extern G_MODULE_EXPORT void audio_edit_clicked_cb(void);
extern G_MODULE_EXPORT void audio_remove_clicked_cb(void);

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

    g_debug("bind_audio_tree_model()\n");
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
                                    _(""), edit_cell, "icon-name", 3, NULL);
    //gtk_tree_view_column_set_min_width(column, 24);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    column = gtk_tree_view_column_new_with_attributes(
                                    _(""), delete_cell, "icon-name", 4, NULL);
    //gtk_tree_view_column_set_min_width(column, 24);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    g_signal_connect(selection, "changed", audio_list_selection_changed_cb, ud);
    g_signal_connect(edit_cell, "clicked", audio_edit_clicked_cb, ud);
    g_signal_connect(delete_cell, "clicked", audio_remove_clicked_cb, ud);

    g_debug("Done\n");
}

extern G_MODULE_EXPORT void subtitle_list_selection_changed_cb(void);
extern G_MODULE_EXPORT void subtitle_edit_clicked_cb(void);
extern G_MODULE_EXPORT void subtitle_remove_clicked_cb(void);

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

    g_debug("bind_subtitle_tree_model()\n");
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
                                    _(""), edit_cell, "icon-name", 3, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    column = gtk_tree_view_column_new_with_attributes(
                                    _(""), delete_cell, "icon-name", 4, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    g_signal_connect(selection, "changed", subtitle_list_selection_changed_cb, ud);
    g_signal_connect(edit_cell, "clicked", subtitle_edit_clicked_cb, ud);
    g_signal_connect(delete_cell, "clicked", subtitle_remove_clicked_cb, ud);
}

extern G_MODULE_EXPORT void presets_list_selection_changed_cb(void);
extern G_MODULE_EXPORT void presets_drag_cb(void);
extern G_MODULE_EXPORT void presets_drag_motion_cb(void);
extern G_MODULE_EXPORT void preset_edited_cb(void);
extern void presets_row_expanded_cb(void);

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
    GtkWidget *widget;
    GtkTargetEntry SrcEntry;
    SrcEntry.target = "DATA";
    SrcEntry.flags = GTK_TARGET_SAME_WIDGET;

    g_debug("bind_presets_tree_model()\n");
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
    selection = gtk_tree_view_get_selection(treeview);
    treestore = gtk_tree_store_new(6, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT,
                                  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Preset Name"), cell,
        "text", 0, "weight", 1, "style", 2,
        "foreground", 3, "editable", 5, NULL);

    g_signal_connect(cell, "edited", preset_edited_cb, ud);

    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_set_tooltip_column(treeview, 4);

    gtk_tree_view_enable_model_drag_dest(treeview, &SrcEntry, 1,
                                            GDK_ACTION_MOVE);
    gtk_tree_view_enable_model_drag_source(treeview, GDK_BUTTON1_MASK,
                                            &SrcEntry, 1, GDK_ACTION_MOVE);

    g_signal_connect(treeview, "drag_data_received", presets_drag_cb, ud);
    g_signal_connect(treeview, "drag_motion", presets_drag_motion_cb, ud);
    g_signal_connect(treeview, "row_expanded", presets_row_expanded_cb, ud);
    g_signal_connect(treeview, "row_collapsed", presets_row_expanded_cb, ud);
    g_signal_connect(selection, "changed", presets_list_selection_changed_cb, ud);
    widget = GHB_WIDGET(ud->builder, "presets_remove");
    gtk_widget_set_sensitive(widget, FALSE);
    g_debug("Done\n");
}

static void
clean_old_logs()
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
    path = g_strdup_printf("%s/Activity.log.%d", config, pid);
    ud->activity_log = g_io_channel_new_file (path, "w", NULL);
    ud->job_activity_log = NULL;
    str = g_strdup_printf("<big><b>%s</b></big>", path);
    ghb_ui_update(ud, "activity_location", ghb_string_value(str));
    g_free(str);
    g_free(path);
    g_free(config);
    // Set encoding to raw.
    g_io_channel_set_encoding(ud->activity_log, NULL, NULL);
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
    g_io_add_watch(channel, G_IO_IN, ghb_log_cb, (gpointer)ud );
}

typedef struct
{
    gchar *filename;
    gchar *iconname;
} icon_map_t;

static gchar *dvd_device = NULL;
static gchar *arg_preset = NULL;
static gboolean ghb_debug = FALSE;
#if defined(_WIN32)
static gboolean win32_console = FALSE;
#endif

static GOptionEntry entries[] =
{
    { "device", 'd', 0, G_OPTION_ARG_FILENAME, &dvd_device, N_("The device or file to encode"), NULL },
    { "preset", 'p', 0, G_OPTION_ARG_STRING, &arg_preset, N_("The preset values to use for encoding"), NULL },
    { "debug",  'x', 0, G_OPTION_ARG_NONE, &ghb_debug, N_("Spam a lot"), NULL },
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

void
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

G_MODULE_EXPORT void x264_entry_changed_cb(GtkWidget *widget, signal_user_data_t *ud);
G_MODULE_EXPORT void video_option_changed_cb(GtkWidget *widget, signal_user_data_t *ud);
G_MODULE_EXPORT void plot_changed_cb(GtkWidget *widget, signal_user_data_t *ud);
G_MODULE_EXPORT void position_overlay_cb(GtkWidget *widget, signal_user_data_t *ud);
G_MODULE_EXPORT void preview_hud_size_alloc_cb(GtkWidget *widget, signal_user_data_t *ud);

// Important: any widgets named in CSS must have their widget names set
// below before setting CSS properties on them.
const gchar *MyCSS =
"                                   \n\
GtkRadioButton .button {            \n\
    border-width: 0px;              \n\
    padding: 0px;                   \n\
}                                   \n\
GtkComboBox {                       \n\
    padding: 0px;                   \n\
}                                   \n\
GtkComboBox .button {               \n\
    padding: 2px;                   \n\
    border-width: 0px;              \n\
}                                   \n\
GtkEntry {                          \n\
    padding: 4px 4px;               \n\
}                                   \n\
                                    \n\
@define-color black  #000000;       \n\
@define-color gray18 #2e2e2e;       \n\
@define-color gray22 #383838;       \n\
@define-color gray26 #424242;       \n\
@define-color gray32 #525252;       \n\
@define-color gray40 #666666;       \n\
@define-color gray46 #757575;       \n\
@define-color white  #ffffff;       \n\
                                    \n\
#preview_hud,                       \n\
#live_preview_play,                 \n\
#live_duration,                     \n\
#preview_fullscreen                 \n\
{                                   \n\
    background: @black;             \n\
    background-color: @gray18;      \n\
    color: @white;                  \n\
}                                   \n\
                                    \n\
#preview_show_crop                  \n\
{                                   \n\
    background-color: @gray22;      \n\
    border-color: @white;           \n\
    color: @white;                  \n\
}                                   \n\
                                    \n\
#live_encode_progress,              \n\
#live_preview_progress,             \n\
#preview_frame                      \n\
{                                   \n\
    background: @black;             \n\
    background-color: @gray46;      \n\
    color: @white;                  \n\
}                                   \n\
                                    \n\
#preview_fullscreen:prelight        \n\
{                                   \n\
    background: @black;             \n\
    background-color: @gray32;      \n\
    color: @white;                  \n\
}                                   \n\
                                    \n\
#preview_fullscreen:active          \n\
{                                   \n\
    background: @black;             \n\
    background-color: @gray32;      \n\
    color: @white;                  \n\
}                                   \n\
                                    \n\
#preview_fullscreen:active          \n\
{                                   \n\
    background: @black;             \n\
    background-color: @gray32;      \n\
    color: @white;                  \n\
}                                   \n\
"
#if GTK_CHECK_VERSION(3, 16, 0)
"                                   \n\
                                    \n\
#activity_view                      \n\
{                                   \n\
    font-family: monospace;         \n\
    font-size: 7pt;                 \n\
    font-weight: lighter;           \n\
}                                   \n\
"
#endif
;

extern G_MODULE_EXPORT void status_icon_query_tooltip_cb(void);

int
main(int argc, char *argv[])
{
    signal_user_data_t *ud;
    GError *error = NULL;
    GOptionContext *context;

    hb_global_init();

#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
#endif

    context = g_option_context_new(_("- Transcode media formats"));
    g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
#if defined(_ENABLE_GST)
    g_option_context_add_group(context, gst_init_get_option_group());
#endif
    g_option_context_parse(context, &argc, &argv, &error);
    if (error != NULL)
    {
        g_warning("%s: %s", G_STRFUNC, error->message);
        g_clear_error(&error);
    }
    g_option_context_free(context);

#if defined(_WIN32)
    if (win32_console)
    {
        // Enable console logging
        if(AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()){
            close(STDOUT_FILENO);
            freopen("CONOUT$", "w", stdout);
            close(STDERR_FILENO);
            freopen("CONOUT$", "w", stderr);
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

    if (argc > 1 && dvd_device == NULL && argv[1][0] != '-')
    {
        dvd_device = argv[1];
    }

    gtk_init(&argc, &argv);

    GtkCssProvider *css = gtk_css_provider_new();
    error = NULL;
    gtk_css_provider_load_from_data(css, MyCSS, -1, &error);
    if (error == NULL)
    {
        GdkScreen *ss = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(ss, GTK_STYLE_PROVIDER(css),
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    else
    {
        g_warning("%s: %s", G_STRFUNC, error->message);
        g_clear_error(&error);
    }

#if !defined(_WIN32)
    notify_init("HandBrake");
#endif
    ghb_resource_init();
    ghb_load_icons();

#if !defined(_WIN32)
    dbus_g_thread_init();
#endif
    ghb_udev_init();

    ghb_write_pid_file();
    ud = g_malloc0(sizeof(signal_user_data_t));
    ud->debug = ghb_debug;
    g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG, debug_log_handler, ud);
    g_log_set_handler("Gtk", G_LOG_LEVEL_WARNING, warn_log_handler, ud);
    //g_log_set_handler("Gtk", G_LOG_LEVEL_CRITICAL, warn_log_handler, ud);

    ud->globals = ghb_dict_new();
    ud->prefs = ghb_dict_new();
    ud->settings_array = ghb_array_new();
    ud->settings = ghb_dict_new();
    ghb_array_append(ud->settings_array, ud->settings);

    ud->builder = create_builder_or_die(BUILDER_NAME);
    // Enable events that alert us to media change events
    watch_volumes(ud);

    //GtkWidget *widget = GHB_WIDGET(ud->builder, "PictureDetelecineCustom");
    //gtk_entry_set_inner_border(widget, 2);

    // Since GtkBuilder no longer assigns object ids to widget names
    // Assign a few that are necessary for style overrides to work
#if defined(_NO_UPDATE_CHECK)
    GtkWidget *widget;
    widget = GHB_WIDGET(ud->builder, "check_updates_box");
    gtk_widget_hide(widget);
#endif

    // Must set the names of the widgets that I want to modify
    // style for.
    gtk_widget_set_name(GHB_WIDGET(ud->builder, "preview_hud"), "preview_hud");
    gtk_widget_set_name(GHB_WIDGET(ud->builder, "preview_frame"), "preview_frame");
    gtk_widget_set_name(GHB_WIDGET(ud->builder, "live_preview_play"), "live_preview_play");
    gtk_widget_set_name(GHB_WIDGET(ud->builder, "live_preview_progress"), "live_preview_progress");
    gtk_widget_set_name(GHB_WIDGET(ud->builder, "live_encode_progress"), "live_encode_progress");
    gtk_widget_set_name(GHB_WIDGET(ud->builder, "live_duration"), "live_duration");
    gtk_widget_set_name(GHB_WIDGET(ud->builder, "preview_show_crop"), "preview_show_crop");
    gtk_widget_set_name(GHB_WIDGET(ud->builder, "preview_fullscreen"), "preview_fullscreen");
    gtk_widget_set_name(GHB_WIDGET(ud->builder, "activity_view"), "activity_view");

    // Redirect stderr to the activity window
    ghb_preview_init(ud);
    IoRedirect(ud);
    ghb_log( "%s - %s - %s",
        HB_PROJECT_TITLE, HB_PROJECT_BUILD_TITLE, HB_PROJECT_URL_WEBSITE );
    ghb_init_dep_map();

    // Need to connect x264_options textview buffer to the changed signal
    // since it can't be done automatically
    GtkTextView *textview;
    GtkTextBuffer *buffer;
    textview = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "x264Option"));
    buffer = gtk_text_view_get_buffer(textview);
    g_signal_connect(buffer, "changed", (GCallback)x264_entry_changed_cb, ud);

    textview = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "VideoOptionExtra"));
    buffer = gtk_text_view_get_buffer(textview);
    g_signal_connect(buffer, "changed", (GCallback)video_option_changed_cb, ud);

    textview = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "MetaLongDescription"));
    buffer = gtk_text_view_get_buffer(textview);
    g_signal_connect(buffer, "changed", (GCallback)plot_changed_cb, ud);

    ghb_combo_init(ud);

    g_debug("ud %p\n", ud);
    g_debug("ud->builder %p\n", ud->builder);

    bind_audio_tree_model(ud);
    bind_subtitle_tree_model(ud);
    bind_presets_tree_model(ud);
    bind_queue_tree_model(ud);
    bind_chapter_tree_model(ud);
    // Connect up the signals to their callbacks
    // I wrote my own connector so that I could pass user data
    // to the callbacks.  Builder's standard autoconnect doesn't all this.
    gtk_builder_connect_signals_full(ud->builder, MyConnect, ud);

    ghb_init_audio_defaults_ui(ud);
    ghb_init_subtitle_defaults_ui(ud);

    // Parsing x264 options "" initializes x264 widgets to proper defaults
    ghb_x264_init(ud);

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

    int logLevel = ghb_dict_get_int(ud->prefs, "LoggingLevel");
    ghb_backend_init(logLevel);

    // Load the presets files
    ghb_presets_load(ud);
    // Note that ghb_preset_to_settings(ud->settings) is called when
    // the default preset is selected.

    ghb_settings_to_ui(ud, ud->globals);
    ghb_settings_to_ui(ud, ud->prefs);
    // Note that ghb_settings_to_ui(ud->settings) happens when initial
    // empty title is initialized.


    if (ghb_dict_get_bool(ud->prefs, "hbfd"))
    {
        ghb_hbfd(ud, TRUE);
    }
    const gchar *source = ghb_dict_get_string(ud->prefs, "default_source");
    ghb_dvd_set_current(source, ud);

    // Populate the presets tree view
    ghb_presets_list_init(ud, NULL);
    // Get the first preset name
    if (arg_preset != NULL)
    {
        ghb_select_preset(ud->builder, arg_preset);
    }
    else
    {
        ghb_select_default_preset(ud->builder);
    }

    // Grey out widgets that are dependent on a disabled feature
    ghb_check_all_depencencies(ud);

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
    // Reload and check status of the last saved queue
    g_idle_add((GSourceFunc)ghb_reload_queue, ud);

    // Start timer for monitoring libhb status, 500ms
    g_timeout_add(200, ghb_timer_cb, (gpointer)ud);

    // Add dvd devices to File menu
    ghb_volname_cache_init();
    GHB_THREAD_NEW("Cache Volume Names", (GThreadFunc)ghb_cache_volnames, ud);

    GtkWidget *ghb_window = GHB_WIDGET(ud->builder, "hb_window");

    gint window_width, window_height;
    GdkGeometry geo = {
        -1, -1, 1920, 768, -1, -1, 10, 10, 0, 0, GDK_GRAVITY_NORTH_WEST
    };
    GdkWindowHints geo_mask;
    geo_mask = GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_BASE_SIZE;
    gtk_window_set_geometry_hints(GTK_WINDOW(ghb_window), ghb_window,
                                  &geo, geo_mask);
    window_width = ghb_dict_get_int(ud->prefs, "window_width");
    window_height = ghb_dict_get_int(ud->prefs, "window_height");

    /*
     * Filter objects in GtkBuilder xml
     * Unfortunately, GtkFilter is poorly supported by GtkBuilder,
     * so a lot of the setup must happen in code.
        SourceFilterAll
        SourceFilterVideo
        SourceFilterTS
        SourceFilterMPG
        SourceFilterEVO
        SourceFilterVOB
        SourceFilterMKV
        SourceFilterMP4
        SourceFilterAVI
        SourceFilterMOV
        SourceFilterOGG
        SourceFilterFLV
        SourceFilterWMV
    */
    // Add filters to source chooser
    GtkFileFilter *filter;
    GtkFileChooser *chooser;
    chooser = GTK_FILE_CHOOSER(GHB_WIDGET(ud->builder, "source_dialog"));
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterAll"));
    gtk_file_filter_set_name(filter, _("All"));
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterVideo"));
    gtk_file_filter_set_name(filter, _("Video"));
    gtk_file_filter_add_mime_type(filter, "video/*");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterTS"));
    gtk_file_filter_set_name(filter, "TS");
    gtk_file_filter_add_pattern(filter, "*.ts");
    gtk_file_filter_add_pattern(filter, "*.TS");
    gtk_file_filter_add_pattern(filter, "*.m2ts");
    gtk_file_filter_add_pattern(filter, "*.M2TS");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterMPG"));
    gtk_file_filter_set_name(filter, "MPG");
    gtk_file_filter_add_pattern(filter, "*.mpg");
    gtk_file_filter_add_pattern(filter, "*.MPG");
    gtk_file_filter_add_pattern(filter, "*.mepg");
    gtk_file_filter_add_pattern(filter, "*.MEPG");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterEVO"));
    gtk_file_filter_set_name(filter, "EVO");
    gtk_file_filter_add_pattern(filter, "*.evo");
    gtk_file_filter_add_pattern(filter, "*.EVO");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterVOB"));
    gtk_file_filter_set_name(filter, "VOB");
    gtk_file_filter_add_pattern(filter, "*.vob");
    gtk_file_filter_add_pattern(filter, "*.VOB");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterMKV"));
    gtk_file_filter_set_name(filter, "MKV");
    gtk_file_filter_add_pattern(filter, "*.mkv");
    gtk_file_filter_add_pattern(filter, "*.MKV");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterMP4"));
    gtk_file_filter_set_name(filter, "MP4");
    gtk_file_filter_add_pattern(filter, "*.mp4");
    gtk_file_filter_add_pattern(filter, "*.MP4");
    gtk_file_filter_add_pattern(filter, "*.m4v");
    gtk_file_filter_add_pattern(filter, "*.M4V");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterMOV"));
    gtk_file_filter_set_name(filter, "MOV");
    gtk_file_filter_add_pattern(filter, "*.mov");
    gtk_file_filter_add_pattern(filter, "*.MOV");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterAVI"));
    gtk_file_filter_set_name(filter, "AVI");
    gtk_file_filter_add_pattern(filter, "*.avi");
    gtk_file_filter_add_pattern(filter, "*.AVI");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterOGG"));
    gtk_file_filter_set_name(filter, "OGG");
    gtk_file_filter_add_pattern(filter, "*.ogg");
    gtk_file_filter_add_pattern(filter, "*.OGG");
    gtk_file_filter_add_pattern(filter, "*.ogv");
    gtk_file_filter_add_pattern(filter, "*.OGV");
    gtk_file_filter_add_pattern(filter, "*.ogm");
    gtk_file_filter_add_pattern(filter, "*.OGM");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterFLV"));
    gtk_file_filter_set_name(filter, "FLV");
    gtk_file_filter_add_pattern(filter, "*.flv");
    gtk_file_filter_add_pattern(filter, "*.FLV");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterWMV"));
    gtk_file_filter_set_name(filter, "WMV");
    gtk_file_filter_add_pattern(filter, "*.wmv");
    gtk_file_filter_add_pattern(filter, "*.WMV");
    gtk_file_chooser_add_filter(chooser, filter);

    // Gtk has a really stupid bug.  If the file chooser is showing
    // hidden files AND there is no filter set, it will not select
    // the filename when gtk_file_chooser_set_filename is called.
    // So add a completely unnessary filter to prevent this behavior.
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterAll"));
    gtk_file_chooser_set_filter(chooser, filter);

#if !GTK_CHECK_VERSION(3, 16, 0)
    PangoFontDescription *font_desc;
    font_desc = pango_font_description_from_string("monospace 10");
    textview = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "activity_view"));
    gtk_widget_override_font(GTK_WIDGET(textview), font_desc);
    pango_font_description_free(font_desc);
#endif

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
    gtk_widget_show(ghb_window);

    // Everything should be go-to-go.  Lets rock!

    gtk_main();
    ghb_backend_close();

    ghb_value_free(&ud->queue);
    ghb_value_free(&ud->settings_array);
    ghb_value_free(&ud->prefs);
    ghb_value_free(&ud->globals);
    ghb_value_free(&ud->x264_priv);

    g_io_channel_unref(ud->activity_log);
    ghb_settings_close();
    ghb_resource_free();
#if !defined(_WIN32)
    notify_uninit();
#endif

    g_object_unref(ud->builder);

    g_free(ud->current_dvd_device);
    g_free(ud);

    return 0;
}
