/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) John Stebbins 2008-2011 <stebbins@stebbins>
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

#if defined(_USE_APP_IND)
#include <libappindicator/app-indicator.h>
#endif

#include <glib/gstdio.h>
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
#include "x264handler.h"
#include "settings.h"
#include "resources.h"
#include "presets.h"
#include "preview.h"
#include "ghbcompositor.h"


/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
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
    GValue *gval;
    GError *error = NULL;
    const gchar *ghb_ui;

    const gchar *markup =
        N_("<b><big>Unable to create %s.</big></b>\n"
        "\n"
        "Internal error. Could not parse UI description.\n"
        "%s");
    g_debug("create_builder_or_die ()\n");
    GtkBuilder *xml = gtk_builder_new();
    gval = ghb_resource_get("ghb-ui");
    ghb_ui = g_value_get_string(gval);
    if (xml != NULL)
        res = gtk_builder_add_from_string(xml, ghb_ui, -1, &error);
    if (!xml || !res) 
    {
        GtkWidget *dialog = gtk_message_dialog_new_with_markup(NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _(markup),
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
bind_chapter_tree_model (signal_user_data_t *ud)
{
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;
    GtkListStore *treestore;
    GtkTreeView  *treeview;

    g_debug("bind_chapter_tree_model ()\n");
    treeview = GTK_TREE_VIEW(GHB_WIDGET (ud->builder, "chapters_list"));
    treestore = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    cell = ghb_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _("Index"), cell, "text", 0, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    cell = ghb_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _("Duration"), cell, "text", 1, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    cell = ghb_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                    _("Title"), cell, "text", 2, "editable", 3, NULL);
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
bind_queue_tree_model (signal_user_data_t *ud)
{
    GtkCellRenderer *cell, *textcell;
    GtkTreeViewColumn *column;
    GtkTreeStore *treestore;
    GtkTreeView  *treeview;
    GtkTreeSelection *selection;
    GtkTargetEntry SrcEntry;
    SrcEntry.target = "DATA";
    SrcEntry.flags = GTK_TARGET_SAME_WIDGET;

    g_debug("bind_queue_tree_model ()\n");
    treeview = GTK_TREE_VIEW(GHB_WIDGET (ud->builder, "queue_list"));
    selection = gtk_tree_view_get_selection (treeview);
    treestore = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title (column, _("Job Information"));
    cell = gtk_cell_renderer_pixbuf_new();
    g_object_set(cell, "yalign", 0.0, NULL);
    gtk_tree_view_column_pack_start (column, cell, FALSE);
    gtk_tree_view_column_add_attribute (column, cell, "icon-name", 0);
    textcell = gtk_cell_renderer_text_new();
    g_object_set(textcell, "wrap-mode", PANGO_WRAP_CHAR, NULL);
    g_object_set(textcell, "wrap-width", 500, NULL);
    gtk_tree_view_column_pack_start (column, textcell, TRUE);
    gtk_tree_view_column_add_attribute (column, textcell, "markup", 1);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_column_set_max_width (column, 550);

    cell = custom_cell_renderer_button_new();
    g_object_set(cell, "yalign", 0.0, NULL);
    column = gtk_tree_view_column_new_with_attributes(
                                    _(""), cell, "icon-name", 2, NULL);
    gtk_tree_view_column_set_min_width (column, 24);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    gtk_tree_view_enable_model_drag_dest (treeview, &SrcEntry, 1, 
                                            GDK_ACTION_MOVE);
    gtk_tree_view_enable_model_drag_source (treeview, GDK_BUTTON1_MASK, 
                                            &SrcEntry, 1, GDK_ACTION_MOVE);

    g_signal_connect(selection, "changed", queue_list_selection_changed_cb, ud);
    g_signal_connect(cell, "clicked", queue_remove_clicked_cb, ud);
    g_signal_connect(treeview, "size-allocate", queue_list_size_allocate_cb, 
                        textcell);
    g_signal_connect(treeview, "drag_data_received", queue_drag_cb, ud);
    g_signal_connect(treeview, "drag_motion", queue_drag_motion_cb, ud);
}

extern G_MODULE_EXPORT void audio_list_selection_changed_cb(void);

// Create and bind the tree model to the tree view for the audio track list
// Also, connect up the signal that lets us know the selection has changed
static void
bind_audio_tree_model (signal_user_data_t *ud)
{
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;
    GtkListStore *treestore;
    GtkTreeView  *treeview;
    GtkTreeSelection *selection;
    GtkWidget *widget;

    g_debug("bind_audio_tree_model ()\n");
    treeview = GTK_TREE_VIEW(GHB_WIDGET (ud->builder, "audio_list"));
    selection = gtk_tree_view_get_selection (treeview);
    // 12 columns in model.  6 are visible, the other 6 are for storing
    // values that I need
    treestore = gtk_list_store_new(7, G_TYPE_STRING, G_TYPE_STRING, 
                                   G_TYPE_STRING, G_TYPE_STRING, 
                                   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _("Track"), cell, "text", 0, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_min_width (column, 200);
    gtk_tree_view_column_set_max_width (column, 200);

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _("Codec"), cell, "text", 1, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_min_width (column, 110);

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _("Bitrate"), cell, "text", 2, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_min_width (column, 50);

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _("Sample Rate"), cell, "text", 3, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_min_width (column, 100);

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _("Mix"), cell, "text", 4, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_min_width (column, 115);

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _("Gain"), cell, "text", 5, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _("DRC"), cell, "text", 6, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));

    g_signal_connect(selection, "changed", audio_list_selection_changed_cb, ud);
    // Need to disable remove and update buttons since there are initially
    // no selections
    widget = GHB_WIDGET (ud->builder, "audio_remove");
    gtk_widget_set_sensitive(widget, FALSE);
    g_debug("Done\n");
}

extern G_MODULE_EXPORT void subtitle_list_selection_changed_cb(void);
extern G_MODULE_EXPORT void subtitle_forced_toggled_cb(void);
extern G_MODULE_EXPORT void subtitle_burned_toggled_cb(void);
extern G_MODULE_EXPORT void subtitle_default_toggled_cb(void);

// Create and bind the tree model to the tree view for the subtitle track list
// Also, connect up the signal that lets us know the selection has changed
static void
bind_subtitle_tree_model (signal_user_data_t *ud)
{
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;
    GtkListStore *treestore;
    GtkTreeView  *treeview;
    GtkTreeSelection *selection;
    GtkWidget *widget;

    g_debug("bind_subtitle_tree_model ()\n");
    treeview = GTK_TREE_VIEW(GHB_WIDGET (ud->builder, "subtitle_list"));
    selection = gtk_tree_view_get_selection (treeview);
    // 6 columns in model.  5 are visible, the other 1 is for storing
    // values that I need
    // Track, force, burn, default, type, srt offset, track short, source
    // force visible, burn visible, offset visible
    treestore = gtk_list_store_new(10, 
                                    G_TYPE_STRING,
                                    G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
                                    G_TYPE_BOOLEAN,
                                    G_TYPE_INT,     G_TYPE_STRING,
                                    G_TYPE_INT,
                                    G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
                                    G_TYPE_BOOLEAN);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
                                    _("Track"), cell, "text", 0, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_min_width (column, 350);
    gtk_tree_view_column_set_max_width (column, 350);

    cell = gtk_cell_renderer_toggle_new();
    column = gtk_tree_view_column_new_with_attributes(
            _("Forced Only"), cell, "active", 1, "visible", 7, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    g_signal_connect(cell, "toggled", subtitle_forced_toggled_cb, ud);

    cell = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(cell), TRUE);
    column = gtk_tree_view_column_new_with_attributes(
            _("Burned In"), cell, "active", 2, "visible", 8, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    g_signal_connect(cell, "toggled", subtitle_burned_toggled_cb, ud);

    cell = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(cell), TRUE);
    column = gtk_tree_view_column_new_with_attributes(
                _("Default"), cell, "active", 3, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    g_signal_connect(cell, "toggled", subtitle_default_toggled_cb, ud);

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(
            _("Srt Offset"), cell, "text", 4, "visible", 9, NULL);
    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));


    g_signal_connect(selection, "changed", subtitle_list_selection_changed_cb, ud);
    // Need to disable remove and update buttons since there are initially
    // no selections
    widget = GHB_WIDGET (ud->builder, "subtitle_remove");
    gtk_widget_set_sensitive(widget, FALSE);
    g_debug("Done\n");
}

extern G_MODULE_EXPORT void presets_list_selection_changed_cb(void);
extern G_MODULE_EXPORT void presets_drag_cb(void);
extern G_MODULE_EXPORT void presets_drag_motion_cb(void);
extern G_MODULE_EXPORT void preset_edited_cb(void);
extern void presets_row_expanded_cb(void);

// Create and bind the tree model to the tree view for the preset list
// Also, connect up the signal that lets us know the selection has changed
static void
bind_presets_tree_model (signal_user_data_t *ud)
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

    g_debug("bind_presets_tree_model ()\n");
    treeview = GTK_TREE_VIEW(GHB_WIDGET (ud->builder, "presets_list"));
    selection = gtk_tree_view_get_selection (treeview);
    treestore = gtk_tree_store_new(6, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, 
                                  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Preset Name"), cell, 
        "text", 0, "weight", 1, "style", 2, 
        "foreground", 3, "editable", 5, NULL);

    g_signal_connect(cell, "edited", preset_edited_cb, ud);

    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_set_tooltip_column (treeview, 4);

    gtk_tree_view_enable_model_drag_dest (treeview, &SrcEntry, 1, 
                                            GDK_ACTION_MOVE);
    gtk_tree_view_enable_model_drag_source (treeview, GDK_BUTTON1_MASK, 
                                            &SrcEntry, 1, GDK_ACTION_MOVE);

    g_signal_connect(treeview, "drag_data_received", presets_drag_cb, ud);
    g_signal_connect(treeview, "drag_motion", presets_drag_motion_cb, ud);
    g_signal_connect(treeview, "row_expanded", presets_row_expanded_cb, ud);
    g_signal_connect(treeview, "row_collapsed", presets_row_expanded_cb, ud);
    g_signal_connect(selection, "changed", presets_list_selection_changed_cb, ud);
    widget = GHB_WIDGET (ud->builder, "presets_remove");
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
    g_io_channel_set_encoding (ud->activity_log, NULL, NULL);
    // redirect stderr to the writer end of the pipe
#if defined(_WIN32)
    // dup2 doesn't work on windows for some stupid reason
    stderr->_file = pfd[1];
#else
    dup2(pfd[1], /*stderr*/2);
#endif
    setvbuf(stderr, NULL, _IONBF, 0);
    channel = g_io_channel_unix_new (pfd[0]);
    // I was getting an this error:
    // "Invalid byte sequence in conversion input"
    // Set disable encoding on the channel.
    g_io_channel_set_encoding (channel, NULL, NULL);
    g_io_add_watch (channel, G_IO_IN, ghb_log_cb, (gpointer)ud );
}

typedef struct
{
    gchar *filename;
    gchar *iconname;
} icon_map_t;

static gchar *dvd_device = NULL;
static gchar *arg_preset = NULL;
static gboolean ghb_debug = FALSE;

static GOptionEntry entries[] = 
{
    { "device", 'd', 0, G_OPTION_ARG_FILENAME, &dvd_device, "The device or file to encode", NULL },
    { "preset", 'p', 0, G_OPTION_ARG_STRING, &arg_preset, "The preset values to use for encoding", NULL },
    { "debug", 'x', 0, G_OPTION_ARG_NONE, &ghb_debug, "Spam a lot", NULL },
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
    gvm = g_volume_monitor_get ();

    g_signal_connect(gvm, "drive-changed", (GCallback)drive_changed_cb, ud);
#else
    GdkWindow *window;
    GtkWidget *widget;

    widget = GHB_WIDGET (ud->builder, "hb_window");
    window = gtk_widget_get_parent_window(widget);
    gdk_window_add_filter(window, win_message_cb, ud);
#endif
}

G_MODULE_EXPORT void x264_entry_changed_cb(GtkWidget *widget, signal_user_data_t *ud);
G_MODULE_EXPORT void x264_option_changed_cb(GtkWidget *widget, signal_user_data_t *ud);
void preview_window_expose_cb(void);

// Some style definitions for the preview window and hud
const gchar *hud_rcstyle =
"style \"ghb-entry\" {\n"
"GtkEntry::inner-border = {2,2,1,1}\n"
"}\n"
"style \"ghb-combo\" {\n"
"xthickness = 1\n"
"ythickness = 1\n"
"}\n"
"style \"ghb-preview\" {\n"
"bg[NORMAL]=\"black\"\n"
"}\n"
"style \"ghb-hud\" {\n"
"bg[NORMAL]=\"gray18\"\n"
"bg[ACTIVE]=\"gray32\"\n"
"bg[PRELIGHT]=\"gray46\"\n"
"bg[SELECTED]=\"black\"\n"
"base[NORMAL]=\"gray40\"\n"
"text[NORMAL]=\"white\"\n"
"text[ACTIVE]=\"white\"\n"
"fg[NORMAL]=\"white\"\n"
"fg[ACTIVE]=\"white\"\n"
"fg[PRELIGHT]=\"white\"\n"
"}\n"
"widget_class \"*.GtkComboBox.GtkToggleButton\" style \"ghb-combo\"\n"
"widget_class \"*.GtkScaleButton\" style \"ghb-combo\"\n"
"widget_class \"*.GtkEntry\" style \"ghb-entry\"\n"
"widget \"preview_window.*.preview_hud.*\" style \"ghb-hud\"\n"
"widget \"preview_window\" style \"ghb-preview\"\n";

#if GTK_CHECK_VERSION(2, 16, 0)
extern G_MODULE_EXPORT void status_icon_query_tooltip_cb(void);
#endif

int
main (int argc, char *argv[])
{
    signal_user_data_t *ud;
    GValue *preset;
    GError *error = NULL;
    GOptionContext *context;

#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    if (!g_thread_supported())
        g_thread_init(NULL);
    context = g_option_context_new ("- Transcode media formats");
    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
#if defined(_ENABLE_GST)
    g_option_context_add_group (context, gst_init_get_option_group ());
#endif
    g_option_context_parse (context, &argc, &argv, &error);
    g_option_context_free(context);

    if (argc > 1 && dvd_device == NULL && argv[1][0] != '-')
    {
        dvd_device = argv[1];
    }
    
    gtk_init (&argc, &argv);
    gtk_rc_parse_string(hud_rcstyle);
    g_type_class_unref(g_type_class_ref(GTK_TYPE_BUTTON));
    g_object_set(gtk_settings_get_default(), "gtk-button-images", TRUE, NULL);
#if !defined(_WIN32)
    notify_init("HandBrake");
#endif
    ghb_register_transforms();
    ghb_resource_init();
    ghb_load_icons();

#if !defined(_WIN32)
    dbus_g_thread_init();
#endif
    ghb_udev_init();

    ghb_write_pid_file();
    ud = g_malloc0(sizeof(signal_user_data_t));
    ud->debug = ghb_debug;
    g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG, debug_log_handler, ud);
    g_log_set_handler ("Gtk", G_LOG_LEVEL_WARNING, warn_log_handler, ud);
    //g_log_set_handler ("Gtk", G_LOG_LEVEL_CRITICAL, warn_log_handler, ud);
    ud->settings = ghb_settings_new();
    ud->builder = create_builder_or_die (BUILDER_NAME);
    // Enable events that alert us to media change events
    watch_volumes (ud);

    //GtkWidget *widget = GHB_WIDGET(ud->builder, "PictureDetelecineCustom");
    //gtk_entry_set_inner_border(widget, 2);

    // Since GtkBuilder no longer assigns object ids to widget names
    // Assign a few that are necessary for style overrides to work
    GtkWidget *widget;
#if defined(_NO_UPDATE_CHECK)
    widget = GHB_WIDGET(ud->builder, "check_updates_box");
    gtk_widget_hide(widget);
#endif

    widget = GHB_WIDGET(ud->builder, "preview_hud");
    gtk_widget_set_name(widget, "preview_hud");
    widget = GHB_WIDGET(ud->builder, "preview_window");
    gtk_widget_set_name(widget, "preview_window");

    // Set up the "hud" control overlay for the preview window
    GtkWidget *draw, *hud, *blender, *align;

    align = GHB_WIDGET(ud->builder, "preview_window_alignment");
    draw = GHB_WIDGET(ud->builder, "preview_image_align");
    hud = GHB_WIDGET(ud->builder, "preview_hud");

    // Set up compositing for hud
    blender = ghb_compositor_new();

    gtk_container_add(GTK_CONTAINER(align), blender);
    ghb_compositor_zlist_insert(GHB_COMPOSITOR(blender), draw, 1, 1);
    ghb_compositor_zlist_insert(GHB_COMPOSITOR(blender), hud, 2, .85);
    gtk_widget_show(blender);

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
    textview = GTK_TEXT_VIEW(GHB_WIDGET (ud->builder, "x264Option"));
    buffer = gtk_text_view_get_buffer (textview);
    g_signal_connect(buffer, "changed", (GCallback)x264_entry_changed_cb, ud);

    textview = GTK_TEXT_VIEW(GHB_WIDGET (ud->builder, "x264OptionExtra"));
    buffer = gtk_text_view_get_buffer (textview);
    g_signal_connect(buffer, "changed", (GCallback)x264_option_changed_cb, ud);

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
    gtk_builder_connect_signals_full (ud->builder, MyConnect, ud);

    GtkWidget *presetSlider = GHB_WIDGET(ud->builder, "x264PresetSlider");
    const char * const *x264_presets;
    int count = 0;
    x264_presets = hb_x264_presets();
    while (x264_presets && x264_presets[count]) count++;
    gtk_range_set_range (GTK_RANGE(presetSlider), 0, count-1);

    // Load all internal settings
    ghb_settings_init(ud);
    // Load the presets files
    ghb_presets_load(ud);
    ghb_prefs_load(ud);

    ghb_prefs_to_ui(ud);

    gint logLevel;
    logLevel = ghb_settings_get_int(ud->settings, "LoggingLevel");
    ghb_backend_init(logLevel);

    if (ghb_settings_get_boolean(ud->settings, "hbfd"))
    {
        ghb_hbfd(ud, TRUE);
    }
    gchar *source = ghb_settings_get_string(ud->settings, "default_source");
    ghb_dvd_set_current(source, ud);
    g_free(source);

    // Parsing x264 options "" initializes x264 widgets to proper defaults
    ghb_x264_parse_options(ud, "");

    // Populate the presets tree view
    ghb_presets_list_init(ud, NULL, 0);
    // Get the first preset name
    if (arg_preset != NULL)
    {
        preset = ghb_parse_preset_path(arg_preset);
        if (preset)
        {
            ghb_select_preset(ud->builder, preset);
            ghb_value_free(preset);
        }
    }
    else
    {
        ghb_select_default_preset(ud->builder);
    }

    // Grey out widgets that are dependent on a disabled feature
    ghb_check_all_depencencies (ud);

    if (dvd_device != NULL)
    {
        // Source overridden from command line option
        ghb_settings_set_string(ud->settings, "scan_source", dvd_device);
        g_idle_add((GSourceFunc)ghb_idle_scan, ud);
    }
    // Reload and check status of the last saved queue
    g_idle_add((GSourceFunc)ghb_reload_queue, ud);

    // Start timer for monitoring libhb status, 500ms
    g_timeout_add (500, ghb_timer_cb, (gpointer)ud);

    // Add dvd devices to File menu
    ghb_volname_cache_init();
    g_thread_create((GThreadFunc)ghb_cache_volnames, ud, FALSE, NULL);

#if defined(_USE_APP_IND)
    GtkUIManager * uim = GTK_UI_MANAGER(GHB_OBJECT(ud->builder, "uimanager1"));

    GtkMenu *ai_menu = GTK_MENU(gtk_ui_manager_get_widget(uim, "/ui/tray_menu"));
    ud->ai = app_indicator_new("HandBrake", "hb-icon", APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    app_indicator_set_menu( ud->ai, ai_menu );
    app_indicator_set_label( ud->ai, "", "99.99%");
    if (ghb_settings_get_boolean(ud->settings, "show_status"))
    {
        app_indicator_set_status( ud->ai, APP_INDICATOR_STATUS_ACTIVE );
    }
    else
    {
        app_indicator_set_status( ud->ai, APP_INDICATOR_STATUS_PASSIVE );
    }
    GtkStatusIcon *si;
    si = GTK_STATUS_ICON(GHB_OBJECT(ud->builder, "hb_status"));

    gtk_status_icon_set_visible(si, FALSE );
#else
    GtkStatusIcon *si;
    si = GTK_STATUS_ICON(GHB_OBJECT(ud->builder, "hb_status"));

    gtk_status_icon_set_visible(si,
            ghb_settings_get_boolean(ud->settings, "show_status"));

#if GTK_CHECK_VERSION(2, 16, 0)
    gtk_status_icon_set_has_tooltip(si, TRUE);
    g_signal_connect(si, "query-tooltip", 
                    status_icon_query_tooltip_cb, ud);
#else
    gtk_status_icon_set_tooltip(si, "HandBrake");
#endif
#endif

    // Ugly hack to keep subtitle table from bouncing around as I change
    // which set of controls are visible
    GtkRequisition req;
    gint width, height;
    
    widget = GHB_WIDGET(ud->builder, "SrtCodeset");
    gtk_widget_size_request( widget, &req );
    height = req.height;
    widget = GHB_WIDGET(ud->builder, "srt_code_label");
    gtk_widget_size_request( widget, &req );
    height += req.height;
    widget = GHB_WIDGET(ud->builder, "subtitle_table");
    gtk_widget_set_size_request(widget, -1, height);
    
    widget = GHB_WIDGET (ud->builder, "hb_window");

    GdkGeometry geo = { 
        -1, -1, 1024, 768, -1, -1, 10, 10, 0, 0, GDK_GRAVITY_NORTH_WEST
    };
    GdkWindowHints geo_mask;
    geo_mask = GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_BASE_SIZE;
    gtk_window_set_geometry_hints( GTK_WINDOW(widget), widget, &geo, geo_mask);
    width = ghb_settings_get_int(ud->settings, "window_width");
    height = ghb_settings_get_int(ud->settings, "window_height");
    gtk_window_resize(GTK_WINDOW(widget), width, height);
    gtk_widget_show(widget);

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
    gtk_file_filter_set_name(filter, "All");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "SourceFilterVideo"));
    gtk_file_filter_set_name(filter, "Video");
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

    PangoFontDescription *font_desc;
    font_desc = pango_font_description_from_string ("monospace 10");
    textview = GTK_TEXT_VIEW(GHB_WIDGET (ud->builder, "activity_view"));
    gtk_widget_modify_font(GTK_WIDGET(textview), font_desc);      
    pango_font_description_free (font_desc);      

    // Everything should be go-to-go.  Lets rock!

    gtk_main ();
    gtk_status_icon_set_visible(si, FALSE);
    ghb_backend_close();
    if (ud->queue)
        ghb_value_free(ud->queue);
    ghb_value_free(ud->settings);
    g_io_channel_unref(ud->activity_log);
    ghb_settings_close();
#if !defined(_WIN32)
    notify_uninit();
#endif
    g_free(ud);

    return 0;
}
