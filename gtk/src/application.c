/* application.c
 *
 * Copyright (C) 2008-2025 John Stebbins <stebbins@stebbins>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "application.h"

#include "audiohandler.h"
#include "callbacks.h"
#include "color-scheme.h"
#include "handbrake/handbrake.h"
#include "hb-backend.h"
#include "hb-dvd.h"
#include "icon_res.h"
#include "power-manager.h"
#include "presets.h"
#include "preview.h"
#include "queuehandler.h"
#include "resources.h"
#include "subtitlehandler.h"
#include "ui_res.h"
#include "util.h"
#include "values.h"

#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define pipe(phandles)  _pipe (phandles, 4096, _O_BINARY)
#else
#include <sys/utsname.h>
#endif

struct _GhbApplication
{
    GtkApplication parent_instance;
    signal_user_data_t *ud;
    GtkBuilder *builder;
    char *app_cmd;
    int cancel_encode;
    int when_complete;
    int stderr_src_id;
};

G_DEFINE_TYPE (GhbApplication, ghb_application, GTK_TYPE_APPLICATION)

#define BUILDER_NAME "ghb"

static char *dvd_device = NULL;
static char *arg_preset = NULL;
static gboolean auto_start_queue = FALSE;
static gboolean clear_queue = FALSE;
static gboolean redirect_io = TRUE;

static GtkBuilder*
create_builder_or_die (const gchar *name)
{
    GtkBuilder *xml;
    g_autoptr(GError) error = NULL;

    ghb_log_func();
    xml = gtk_builder_new();
    gtk_builder_add_from_resource(xml, "/fr/handbrake/ghb/ui/menu.ui", &error);
    if (!error)
        gtk_builder_add_from_resource(xml, "/fr/handbrake/ghb/ui/ghb.ui", &error);

    if (error)
    {
        g_error("Unable to load ui file: %s", error->message);
    }
    return xml;
}

// Create and bind the tree model to the tree view for the audio track list
// Also, connect up the signal that lets us know the selection has changed
static void
bind_audio_tree_model(signal_user_data_t *ud)
{
    GtkCellRenderer *source_cell;
    GtkCellRenderer *arrow_cell;
    GtkCellRenderer *output_cell;
    GtkTreeViewColumn *column;
    GtkTreeStore *treestore;
    GtkTreeView  *treeview;
    GtkTreeSelection *selection;

    ghb_log_func();
    treeview = GTK_TREE_VIEW(ghb_builder_widget("audio_list_view"));
    selection = gtk_tree_view_get_selection(treeview);
    treestore = gtk_tree_store_new(6, G_TYPE_STRING, G_TYPE_STRING,
                                      G_TYPE_STRING, G_TYPE_STRING,
                                      G_TYPE_STRING, G_TYPE_FLOAT);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    source_cell = gtk_cell_renderer_text_new();
    arrow_cell = gtk_cell_renderer_text_new();
    output_cell = gtk_cell_renderer_text_new();

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

    g_signal_connect(selection, "changed", G_CALLBACK(audio_list_selection_changed_cb), ud);

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
    GtkTreeViewColumn *column;
    GtkTreeStore *treestore;
    GtkTreeView  *treeview;
    GtkTreeSelection *selection;

    ghb_log_func();
    treeview = GTK_TREE_VIEW(ghb_builder_widget("subtitle_list_view"));
    selection = gtk_tree_view_get_selection(treeview);
    treestore = gtk_tree_store_new(6, G_TYPE_STRING, G_TYPE_STRING,
                                      G_TYPE_STRING, G_TYPE_STRING,
                                      G_TYPE_STRING, G_TYPE_FLOAT);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    source_cell = gtk_cell_renderer_text_new();
    arrow_cell = gtk_cell_renderer_text_new();
    output_cell = gtk_cell_renderer_text_new();

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

    g_signal_connect(selection, "changed", G_CALLBACK(subtitle_list_selection_changed_cb), ud);
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
                path = g_strdup_printf("%s/%s", config, file);
                g_unlink(path);
                g_free(path);
            }
            file = g_dir_read_name(gdir);
        }
        g_dir_close(gdir);
    }
    g_free(config);
#endif
}

static void
io_redirect (GhbApplication *self, signal_user_data_t *ud)
{
    GIOChannel *channel;
    gint pfd[2];
    gchar *config, *path, *str;
    pid_t pid;

    g_return_if_fail(GHB_IS_APPLICATION(self));

    if (!redirect_io)
        return;

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
    ghb_ui_update("activity_location", ghb_string_value(str));
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
    self->stderr_src_id =
        g_io_add_watch(channel, G_IO_IN, ghb_log_cb, (gpointer)ud );

    g_io_channel_unref(channel);
}

G_MODULE_EXPORT void drive_changed_cb(GVolumeMonitor *gvm, GDrive *gd, gpointer data);

#if defined(_WIN32)
G_MODULE_EXPORT GdkFilterReturn
win_message_cb(GdkXEvent *wmevent, GdkEvent *event, gpointer data)
{
    signal_user_data_t *ud = ghb_ud();
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

    g_signal_connect(gvm, "drive-changed", G_CALLBACK(drive_changed_cb), ud);
#else
    GdkWindow *window;
    GtkWidget *widget;

    widget = ghb_builder_widget("hb_window");
    window = gtk_widget_get_parent_window(widget);
    gdk_window_add_filter(window, win_message_cb, ud);
#endif
}

G_MODULE_EXPORT void video_option_changed_cb(GtkWidget *widget, gpointer data);
G_MODULE_EXPORT void plot_changed_cb(GtkWidget *widget, gpointer data);
G_MODULE_EXPORT void position_overlay_cb(GtkWidget *widget, gpointer data);
G_MODULE_EXPORT void preview_hud_size_alloc_cb(GtkWidget *widget, gpointer data);

#define GHB_DECLARE_ACTION_CB(a) G_MODULE_EXPORT void \
    (a)(GSimpleAction *action, GVariant *param, gpointer data)

GHB_DECLARE_ACTION_CB(dvd_source_activate_cb);
GHB_DECLARE_ACTION_CB(source_action_cb);
GHB_DECLARE_ACTION_CB(source_dir_action_cb);
GHB_DECLARE_ACTION_CB(destination_action_cb);
GHB_DECLARE_ACTION_CB(preferences_action_cb);
GHB_DECLARE_ACTION_CB(quit_action_cb);
GHB_DECLARE_ACTION_CB(title_add_action_cb);
GHB_DECLARE_ACTION_CB(title_add_multiple_action_cb);
GHB_DECLARE_ACTION_CB(title_add_all_action_cb);
GHB_DECLARE_ACTION_CB(queue_start_action_cb);
GHB_DECLARE_ACTION_CB(queue_pause_action_cb);
GHB_DECLARE_ACTION_CB(queue_play_file_action_cb);
GHB_DECLARE_ACTION_CB(queue_export_action_cb);
GHB_DECLARE_ACTION_CB(queue_import_action_cb);
GHB_DECLARE_ACTION_CB(queue_move_top_action_cb);
GHB_DECLARE_ACTION_CB(queue_move_bottom_action_cb);
GHB_DECLARE_ACTION_CB(queue_open_source_action_cb);
GHB_DECLARE_ACTION_CB(queue_open_dest_action_cb);
GHB_DECLARE_ACTION_CB(queue_open_log_dir_action_cb);
GHB_DECLARE_ACTION_CB(queue_open_log_action_cb);
GHB_DECLARE_ACTION_CB(queue_delete_all_action_cb);
GHB_DECLARE_ACTION_CB(queue_delete_complete_action_cb);
GHB_DECLARE_ACTION_CB(queue_reset_fail_action_cb);
GHB_DECLARE_ACTION_CB(queue_reset_all_action_cb);
GHB_DECLARE_ACTION_CB(queue_reset_action_cb);
GHB_DECLARE_ACTION_CB(queue_edit_action_cb);
GHB_DECLARE_ACTION_CB(queue_show_sidebar_action_cb);
GHB_DECLARE_ACTION_CB(show_presets_action_cb);
GHB_DECLARE_ACTION_CB(hbfd_action_cb);
GHB_DECLARE_ACTION_CB(show_queue_action_cb);
GHB_DECLARE_ACTION_CB(show_preview_action_cb);
GHB_DECLARE_ACTION_CB(show_activity_action_cb);
GHB_DECLARE_ACTION_CB(show_audio_defaults_cb);
GHB_DECLARE_ACTION_CB(show_subtitle_defaults_cb);
GHB_DECLARE_ACTION_CB(preset_save_action_cb);
GHB_DECLARE_ACTION_CB(preset_save_as_action_cb);
GHB_DECLARE_ACTION_CB(preset_rename_action_cb);
GHB_DECLARE_ACTION_CB(preset_remove_action_cb);
GHB_DECLARE_ACTION_CB(preset_default_action_cb);
GHB_DECLARE_ACTION_CB(preset_export_action_cb);
GHB_DECLARE_ACTION_CB(preset_import_action_cb);
GHB_DECLARE_ACTION_CB(presets_reload_action_cb);
GHB_DECLARE_ACTION_CB(preview_fullscreen_action_cb);
GHB_DECLARE_ACTION_CB(about_action_cb);
GHB_DECLARE_ACTION_CB(guide_action_cb);
GHB_DECLARE_ACTION_CB(preset_select_action_cb);
GHB_DECLARE_ACTION_CB(preset_reload_action_cb);
GHB_DECLARE_ACTION_CB(chapters_export_action_cb);
GHB_DECLARE_ACTION_CB(chapters_import_action_cb);
GHB_DECLARE_ACTION_CB(log_copy_action_cb);
GHB_DECLARE_ACTION_CB(log_directory_action_cb);
GHB_DECLARE_ACTION_CB(title_add_select_all_cb);
GHB_DECLARE_ACTION_CB(title_add_clear_all_cb);
GHB_DECLARE_ACTION_CB(title_add_invert_cb);
GHB_DECLARE_ACTION_CB(audio_add_cb);
GHB_DECLARE_ACTION_CB(audio_add_all_cb);
GHB_DECLARE_ACTION_CB(audio_reset_cb);
GHB_DECLARE_ACTION_CB(audio_remove_cb);
GHB_DECLARE_ACTION_CB(audio_clear_cb);
GHB_DECLARE_ACTION_CB(subtitle_add_cb);
GHB_DECLARE_ACTION_CB(subtitle_add_all_cb);
GHB_DECLARE_ACTION_CB(subtitle_add_fas_cb);
GHB_DECLARE_ACTION_CB(subtitle_reset_cb);
GHB_DECLARE_ACTION_CB(subtitle_remove_cb);
GHB_DECLARE_ACTION_CB(subtitle_clear_cb);

static void
set_action_accel (GtkApplication *app, const char *name, const char *accel)
{
    const char *vaccel[] = { accel, NULL };
    gtk_application_set_accels_for_action(app, name, vaccel);
}

static void
map_actions (GtkApplication *app, signal_user_data_t *ud)
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
        { "queue-delete-complete", queue_delete_complete_action_cb },
        { "queue-delete-all",      queue_delete_all_action_cb      },
        { "queue-export",          queue_export_action_cb          },
        { "queue-import",          queue_import_action_cb          },
        { "queue-edit",            queue_edit_action_cb            },
        { "queue-show-sidebar",    NULL,
          NULL, "false",           queue_show_sidebar_action_cb    },
        { "dvd-open",              dvd_source_activate_cb, "s"     },
        { "hbfd",                  NULL,
          NULL, "false",           hbfd_action_cb                  },
        { "show-presets",          show_presets_action_cb          },
        { "show-queue",            show_queue_action_cb            },
        { "show-preview",          show_preview_action_cb          },
        { "show-activity",         show_activity_action_cb         },
        { "show-audio-defaults",   show_audio_defaults_cb          },
        { "show-sub-defaults",     show_subtitle_defaults_cb       },
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
        { "log-copy",              log_copy_action_cb              },
        { "log-directory",         log_directory_action_cb         },
        { "title-add-select-all",  title_add_select_all_cb         },
        { "title-add-clear-all",   title_add_clear_all_cb          },
        { "title-add-invert",      title_add_invert_cb             },
        { "audio-add",             audio_add_cb                    },
        { "audio-add-all",         audio_add_all_cb                },
        { "audio-reset",           audio_reset_cb                  },
        { "audio-remove",          audio_remove_cb                 },
        { "audio-clear",           audio_clear_cb                  },
        { "subtitle-add",          subtitle_add_cb                 },
        { "subtitle-add-all",      subtitle_add_all_cb             },
        { "subtitle-add-fas",      subtitle_add_fas_cb             },
        { "subtitle-reset",        subtitle_reset_cb               },
        { "subtitle-remove",       subtitle_remove_cb              },
        { "subtitle-clear",        subtitle_clear_cb               },
    };
    g_action_map_add_action_entries(G_ACTION_MAP(app), entries,
                                    G_N_ELEMENTS(entries), ud);

    set_action_accel(app, "app.quit", "<control>q");
    set_action_accel(app, "app.preferences", "<control>comma");
    set_action_accel(app, "app.source", "<control>o");
    set_action_accel(app, "app.source-dir", "<control><shift>o");
    set_action_accel(app, "app.destination", "<control>s");
    set_action_accel(app, "app.add-current", "<alt>a");
    set_action_accel(app, "app.add-multiple", "<alt>m");
    set_action_accel(app, "app.add-all", "<alt><shift>a");
    set_action_accel(app, "app.queue-start", "<control>e");
    set_action_accel(app, "app.queue-pause", "<control>p");
    set_action_accel(app, "app.show-presets", "<alt>e");
    set_action_accel(app, "app.show-queue", "<alt>u");
    set_action_accel(app, "app.show-preview", "<alt>r");
    set_action_accel(app, "app.show-activity", "<alt>w");
    set_action_accel(app, "app.guide", "F1");
    set_action_accel(app, "app.preset-save-as", "<control>n");
}

static gboolean
_ghb_idle_ui_init (signal_user_data_t *ud)
{
    ghb_settings_to_ui(ud, ud->prefs);
    // Note that ghb_settings_to_ui(ud->settings) happens when initial
    // empty title is initialized.

    // Init settings that are dependent on command line args
    if (dvd_device != NULL)
    {
        // Source overridden from command line option
        ghb_set_scan_source(dvd_device);
        g_idle_add((GSourceFunc)ghb_idle_scan, ud);
    }
    else
    {
        const char *source = ghb_dict_get_string(ud->prefs, "default_source");
        ghb_set_scan_source(source);
    }

    if (arg_preset != NULL)
    {
        ghb_select_preset(ud, arg_preset, HB_PRESET_TYPE_ALL);
    }
    else
    {
        ghb_select_default_preset(ud);
    }

    ghb_bind_dependencies();

    return FALSE;
}

extern G_MODULE_EXPORT void preview_leave_cb(GtkEventControllerMotion * econ,
                                             GdkCrossingMode cross, GdkNotifyType notify,
                                             signal_user_data_t *ud);
extern G_MODULE_EXPORT void preview_motion_cb(GtkEventControllerMotion * econ, double x,
                                              double y, signal_user_data_t *ud);
extern G_MODULE_EXPORT void hud_enter_cb(GtkEventControllerMotion * econ, double x, double y,
                                         GdkCrossingMode cross, GdkNotifyType notify,
                                         signal_user_data_t *ud);
extern G_MODULE_EXPORT void hud_leave_cb(GtkEventControllerMotion * econ, GdkCrossingMode cross,
                                         GdkNotifyType notify, signal_user_data_t *ud);

static gboolean
video_file_drop_received (GtkDropTarget* self, const GValue* value,
                          double x, double y, signal_user_data_t *ud)
{
/* The GdkFileList method is preferred where supported as it handles multiple
 * files and also allows access to sandboxed files via the portal */
#if GTK_CHECK_VERSION(4, 6, 0)
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    if (G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST))
    {
        GdkFileList *gdk_file_list = g_value_get_boxed(value);
        GSList *slist = gdk_file_list_get_files(gdk_file_list);
        g_autoptr(GListStore) video_files = g_list_store_new(G_TYPE_FILE);
        g_autoptr(GListStore) subtitle_files = g_list_store_new(G_TYPE_FILE);
        const char *filename = NULL;

        while (slist)
        {
            filename = g_file_peek_path(slist->data);
            if (ghb_file_is_subtitle(filename))
            {
                g_debug("Subtitle file dropped on window: %s", filename);
                g_list_store_append(subtitle_files, slist->data);
            }
            else
            {
                g_debug("Video file dropped on window: %s", filename);
                g_list_store_append(video_files, slist->data);
            }
            slist = slist->next;
        }

        if (g_list_model_get_n_items(G_LIST_MODEL(video_files)))
        {
            ghb_dict_set_string(ud->prefs, "default_source", filename);
            ghb_pref_save(ud->prefs, "default_source");
            ghb_dvd_set_current(filename, ud);
            ghb_do_scan_list(ud, G_LIST_MODEL(video_files), 0, TRUE);
        }
        else if (subtitle_files)
        {
            ghb_add_subtitle_files(G_LIST_MODEL(subtitle_files), ud);
        }
        return TRUE;
    }
G_GNUC_END_IGNORE_DEPRECATIONS
#endif

    g_autoptr(GFile) file = NULL;
    g_autofree gchar *filename = NULL;

    if (G_VALUE_HOLDS(value, G_TYPE_FILE))
    {
        file = g_value_dup_object(value);
    }
    else if (G_VALUE_HOLDS(value, G_TYPE_URI))
    {
        file = g_file_new_for_uri(g_value_get_string(value));
    }
    else
    {
        return FALSE;
    }
    if (file != NULL)
    {
        filename = g_file_get_path(file);
    }
    if (filename != NULL)
    {
        g_debug("File dropped on window: %s", filename);
        ghb_dict_set_string(ud->prefs, "default_source", filename);
        ghb_pref_save(ud->prefs, "default_source");
        ghb_dvd_set_current(filename, ud);
        ghb_do_scan(ud, filename, 0, TRUE);
    }
    return TRUE;
}

static void
video_file_drop_init (signal_user_data_t *ud)
{
    GtkWidget *window = ghb_builder_widget("hb_window");
    GType types[] = {
#if GTK_CHECK_VERSION(4, 6, 0)
        GDK_TYPE_FILE_LIST,
#endif
        G_TYPE_FILE,
        G_TYPE_URI,
    };
    GtkDropTarget *target = gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_COPY);
    gtk_drop_target_set_gtypes(target, types, G_N_ELEMENTS(types));
    gtk_widget_add_controller(GTK_WIDGET(window), GTK_EVENT_CONTROLLER(target));
    g_signal_connect(target, "drop", G_CALLBACK(video_file_drop_received), ud);
}

char *
ghb_application_get_app_path (GhbApplication *self)
{
    g_return_val_if_fail(GHB_IS_APPLICATION(self), NULL);

    // The preferred method, only works on Linux and certain other OSes
    g_autofree char *link = g_file_read_link("/proc/self/exe", NULL);
    if (link != NULL)
        return g_steal_pointer(&link);

    // Alternatively, work out the path from the command name, path and cwd
    g_autofree char *app_cmd = NULL;
    g_object_get(self, "app-cmd", &app_cmd, NULL);

    if (g_path_is_absolute(app_cmd))
        return g_steal_pointer(&app_cmd);

    g_autofree char *path_cmd = g_find_program_in_path(app_cmd);
    if (path_cmd != NULL)
        return g_steal_pointer(&path_cmd);

    g_autofree char *cwd = g_get_current_dir();
    return g_canonicalize_filename(app_cmd, cwd);
}

char *
ghb_application_get_app_dir (GhbApplication *self)
{
    g_return_val_if_fail(GHB_IS_APPLICATION(self), NULL);

    g_autofree char *app_path = ghb_application_get_app_path(self);
    return g_path_get_dirname(app_path);
}

static void
print_system_information (GhbApplication *self)
{
    g_printerr("%s\n", HB_PROJECT_TITLE);
    g_autofree char *os_info = g_get_os_info(G_OS_INFO_KEY_PRETTY_NAME);
    g_printerr("OS: %s\n", os_info);
#ifndef _WIN32
    g_autofree struct utsname *host_info = g_malloc0(sizeof(struct utsname));
    uname(host_info);
    g_printerr("Kernel: %s %s (%s)\n", host_info->sysname,
               host_info->release, host_info->machine);
#endif
    g_autofree char *install_dir = ghb_application_get_app_dir(self);
    g_autofree char *config_dir = ghb_get_user_config_dir(NULL);
    g_printerr("CPU: %s x %d\n", hb_get_cpu_name(), hb_get_cpu_count());
    g_printerr("Install Dir: %s\n", install_dir);
    g_printerr("Config Dir:  %s\n", config_dir);
    g_printerr("_______________________________\n\n");
}

static void
ghb_application_constructed (GObject *object)
{
    GhbApplication *self = GHB_APPLICATION(object);
    g_assert(GHB_IS_APPLICATION(self));

    G_OBJECT_CLASS(ghb_application_parent_class)->constructed(object);

    g_application_set_application_id(G_APPLICATION(self), "fr.handbrake.ghb");
    g_set_prgname("fr.handbrake.ghb");
    g_set_application_name("HandBrake");
    g_application_set_flags(G_APPLICATION(self), G_APPLICATION_HANDLES_OPEN |
                                                 G_APPLICATION_NON_UNIQUE);
}

static void
ghb_application_activate (GApplication *app)
{
    GhbApplication *self = GHB_APPLICATION(app);
    GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(app));

    if (window)
    {
        gtk_window_present(window);
        return;
    }

    signal_user_data_t *ud = self->ud = g_malloc0(sizeof(signal_user_data_t));
    g_autoptr(GtkCssProvider) provider = gtk_css_provider_new();

    if (ghb_dict_get_bool(ud->prefs, "CustomTmpEnable"))
    {
        const char *tmp_dir = ghb_dict_get_string(ud->prefs, "CustomTmpDir");
        if (tmp_dir != NULL && tmp_dir[0] != 0)
        {
#if defined(_WIN32)
           _putenv_s("TEMP", tmp_dir);
#else
            setenv("TEMP", tmp_dir, 1);
#endif
        }
    }

    gtk_css_provider_load_from_resource(provider, "/fr/handbrake/ghb/ui/custom.css");
    color_scheme_set_async(APP_PREFERS_LIGHT);

    GdkDisplay *dd = gdk_display_get_default();
    gtk_style_context_add_provider_for_display(dd,
                                GTK_STYLE_PROVIDER(provider),
                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    ghb_resource_init();
    gtk_icon_theme_add_resource_path(gtk_icon_theme_get_for_display(gdk_display_get_default()),
                                     "/fr/handbrake/ghb/icons");

    // map application actions (menu callbacks)
    map_actions(GTK_APPLICATION(app), ud);

    ud->prefs = ghb_dict_new();
    ud->settings_array = ghb_array_new();
    ud->settings = ghb_dict_new();
    ghb_array_append(ud->settings_array, ud->settings);

    self->builder = create_builder_or_die(BUILDER_NAME);

    // Initialize D-Bus connections to monitor power settings
    ghb_power_manager_init(ud);

    // Enable drag & drop in queue list
    ghb_queue_drag_n_drop_init(ud);
    video_file_drop_init(ud);

    // Enable events that alert us to media change events
    watch_volumes(ud);

    GtkWidget *widget;

    // Get GtkTextBuffers for activity logs
    widget = ghb_builder_widget("activity_view");
    ud->activity_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    g_object_ref(ud->activity_buffer);
    widget = ghb_builder_widget("queue_activity_view");
    ud->extra_activity_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    g_object_ref(ud->extra_activity_buffer);
    ud->queue_activity_buffer = gtk_text_buffer_new(NULL);

    // Redirect stderr to the activity window
    ghb_preview_init(ud);
    io_redirect(self, ud);
    print_system_information(self);

    GtkTextView   * textview;
    GtkTextBuffer * buffer;

    textview = GTK_TEXT_VIEW(ghb_builder_widget("VideoOptionExtra"));
    buffer = gtk_text_view_get_buffer(textview);
    g_signal_connect(buffer, "changed", G_CALLBACK(video_option_changed_cb), ud);

    textview = GTK_TEXT_VIEW(ghb_builder_widget("MetaLongDescription"));
    buffer = gtk_text_view_get_buffer(textview);
    g_signal_connect(buffer, "changed", G_CALLBACK(plot_changed_cb), ud);

    // Initialize HB internal tables etc.
    hb_global_init();

    // Set up UI combo boxes.  Some of these rely on HB global settings.
    ghb_combo_init(ud);

    g_debug("ud %p", (void *)ud);
    g_debug("builder %p", (void *)self->builder);

    bind_audio_tree_model(ud);
    bind_subtitle_tree_model(ud);
    ghb_presets_bind_tree_model(ud);

    ghb_init_audio_defaults_ui(ud);
    ghb_init_subtitle_defaults_ui(ud);

    // Load prefs before presets.  Some preset defaults may depend
    // on preference settings.
    // First load default values
    ghb_settings_init(ud->prefs, "Preferences");
    ghb_settings_init(ud->settings, "Initialization");
    ghb_settings_init(ud->settings, "OneTimeInitialization");
    // Load user preferences file
    ghb_prefs_load(ud);
    // Store user preferences into ud->prefs
    ghb_prefs_to_settings(ud->prefs);

    int logLevel = ghb_dict_get_int(ud->prefs, "LoggingLevel");

    // Initialize HB work threads
    ghb_backend_init(logLevel);

    // Load the presets files
    ghb_presets_load(ud);

    // GActions associated with widgets do not fire when the widget
    // is changed from this GtkApplication "activate" signal.
    // So initialize UI when idle.
    g_idle_add((GSourceFunc)_ghb_idle_ui_init, ud);

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
    g_thread_new("Cache Volume Names", (GThreadFunc)ghb_cache_volnames, ud);

    GtkWindow *hb_window = GTK_WINDOW(ghb_builder_widget("hb_window"));

    gint window_width, window_height;
    window_width = ghb_dict_get_int(ud->prefs, "window_width");
    window_height = ghb_dict_get_int(ud->prefs, "window_height");
    gtk_window_set_default_size(hb_window, window_width, window_height);

    ghb_set_custom_filter_tooltip(ud, "PictureDetelecineCustom",
                                  "detelecine", HB_FILTER_DETELECINE);
    ghb_set_custom_filter_tooltip(ud, "PictureCombDetectCustom",
                                  "interlace detection", HB_FILTER_COMB_DETECT);
    ghb_set_custom_filter_tooltip(ud, "PictureColorspaceCustom",
                                  "colorspace", HB_FILTER_COLORSPACE);
    ghb_set_custom_filter_tooltip(ud, "PictureChromaSmoothCustom",
                                  "chroma smooth", HB_FILTER_CHROMA_SMOOTH);

    gtk_application_add_window(GTK_APPLICATION(app), hb_window);
    window = GTK_WINDOW(ghb_builder_widget("presets_window"));
    gtk_application_add_window(GTK_APPLICATION(app), window);
    window = GTK_WINDOW(ghb_builder_widget("queue_window"));
    gtk_application_add_window(GTK_APPLICATION(app), window);
    window = GTK_WINDOW(ghb_builder_widget("preview_window"));
    gtk_application_add_window(GTK_APPLICATION(app), window);
    window = GTK_WINDOW(ghb_builder_widget("activity_window"));
    gtk_application_add_window(GTK_APPLICATION(app), window);
    window = GTK_WINDOW(ghb_builder_widget("title_add_multiple_dialog"));
    gtk_application_add_window(GTK_APPLICATION(app), window);

    GMenuModel *menu = G_MENU_MODEL(ghb_builder_object("handbrake-menu-bar"));
    gtk_application_set_menubar(GTK_APPLICATION(app), menu);

    gtk_window_present(hb_window);
}

static GOptionEntry option_entries[] =
{
    { "device", 'd', 0, G_OPTION_ARG_FILENAME, &dvd_device, N_("The device or file to encode"), "FILE" },
    { "preset", 'p', 0, G_OPTION_ARG_STRING, &arg_preset, N_("The preset values to use for encoding"), "NAME" },
    { "debug",  'x', 0, G_OPTION_ARG_NONE, NULL, N_("Spam a lot"), NULL },
    { "config", 'o', 0, G_OPTION_ARG_STRING, NULL, N_("The path to override user config dir"), "DIR" },
    { "console",'c', 0, G_OPTION_ARG_NONE, NULL, N_("Write debug output to console instead of capturing it"), NULL },
    { "auto-start-queue", 0, 0, G_OPTION_ARG_NONE, &auto_start_queue, N_("Automatically start the queue"), NULL },
    { "clear-queue", 0, 0, G_OPTION_ARG_NONE, &clear_queue, N_("Clear previous items from the queue"), NULL },
    { NULL }
};

static void
ghb_application_init (GhbApplication *self)
{
    g_application_add_main_option_entries(G_APPLICATION (self), option_entries);
}

GhbApplication *
ghb_application_new (const char *app_cmd)
{
    return g_object_new(GHB_TYPE_APPLICATION, "app-cmd", app_cmd, NULL);
}

static void
ghb_application_open (GApplication *app, GFile **files, gint n_files,
                      const gchar *hint)
{
    if (n_files < 1)
        return;

    if (dvd_device == NULL)
    {
        dvd_device = g_file_get_path(files[0]);
    }
    ghb_application_activate(app);
}

static int
ghb_application_handle_local_options (GApplication *app, GVariantDict *options)
{
    GhbApplication *self = GHB_APPLICATION(app);
    g_assert(GHB_IS_APPLICATION(self) && options != NULL);

    g_autofree char *config_dir = NULL;

    if (g_variant_dict_lookup(options, "config", "s", &config_dir))
        ghb_override_user_config_dir(config_dir);

#if GLIB_CHECK_VERSION(2, 72, 0)
    if (g_variant_dict_lookup(options, "debug", "b", NULL))
        g_log_set_debug_enabled(TRUE);
#endif

    if (g_variant_dict_lookup(options, "console", "b", NULL))
#if defined(_WIN32)
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
        (void) freopen("NUL", "w", stderr);
        (void) freopen("NUL", "w", stdout);
    }
#else
    redirect_io = FALSE;
#endif

    return G_APPLICATION_CLASS(ghb_application_parent_class)
                ->handle_local_options(app, options);
}

static void
ghb_application_shutdown (GApplication *app)
{
    GhbApplication *self = GHB_APPLICATION(app);
    signal_user_data_t *ud = self->ud;
    g_assert(GHB_IS_APPLICATION(self) && ud != NULL);

    ghb_backend_close();

    // Remove stderr redirection
    if (self->stderr_src_id > 0)
        g_source_remove(self->stderr_src_id);
    ghb_value_free(&ud->queue);
    ghb_value_free(&ud->settings_array);
    ghb_value_free(&ud->prefs);

    if (ud->activity_log != NULL)
        g_io_channel_unref(ud->activity_log);
    ghb_settings_close();
    ghb_resource_free();

    if (self->builder != NULL)
        g_object_unref(self->builder);

    ghb_power_manager_dispose(ud);

    g_object_unref(ud->extra_activity_buffer);
    g_object_unref(ud->queue_activity_buffer);
    g_object_unref(ud->activity_buffer);
    g_clear_pointer(&ud->extra_activity_path, g_free);
    ghb_preview_dispose(ud);

    g_free(ud->current_dvd_device);
    g_free(self->ud);

    G_APPLICATION_CLASS(ghb_application_parent_class)->shutdown(app);
}

enum {
  PROP_0,
  PROP_APP_CMD,
  N_PROPS
};

static void
ghb_application_get_property (GObject *object, guint prop_id,
                              GValue *value, GParamSpec *pspec)
{
    GhbApplication *self = GHB_APPLICATION(object);

    switch (prop_id)
    {
        case PROP_APP_CMD:
            g_value_set_string(value, self->app_cmd);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
ghb_application_set_property (GObject *object, guint prop_id,
                              const GValue *value, GParamSpec *pspec)
{
    GhbApplication *self = GHB_APPLICATION(object);

    switch (prop_id)
    {
        case PROP_APP_CMD:
            self->app_cmd = g_value_dup_string(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
ghb_application_class_init (GhbApplicationClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

    object_class->constructed = ghb_application_constructed;

    app_class->activate = ghb_application_activate;
    app_class->open = ghb_application_open;
    app_class->handle_local_options = ghb_application_handle_local_options;
    app_class->shutdown = ghb_application_shutdown;
    object_class->get_property = ghb_application_get_property;
    object_class->set_property = ghb_application_set_property;

    g_autoptr(GParamSpec) prop = g_param_spec_string("app-cmd", "App Command",
            "The full command-line name of the application", "ghb",
            G_PARAM_READABLE | G_PARAM_WRITABLE |
            G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

    g_object_class_install_property (object_class, PROP_APP_CMD, prop);
}

/**
 * An alternative method to get the user data for use in signal callbacks.
 * Returns: (transfer none): the application's user data
 */
signal_user_data_t *
ghb_ud (void)
{
    GhbApplication *app = GHB_APPLICATION_DEFAULT;
    g_assert(GHB_IS_APPLICATION(app));

    return app->ud;
}

GtkWidget *
ghb_builder_widget (const char *name)
{
    GhbApplication *app = GHB_APPLICATION_DEFAULT;
    g_assert(GHB_IS_APPLICATION(app));

    if (!app->builder) return NULL;
    return GTK_WIDGET(gtk_builder_get_object(app->builder, name));
}

GObject *
ghb_builder_object (const char *name)
{
    GhbApplication *app = GHB_APPLICATION_DEFAULT;
    g_assert(GHB_IS_APPLICATION(app));

    if (!app->builder) return NULL;
    return gtk_builder_get_object(app->builder, name);
}

int
ghb_get_cancel_status (void)
{
    GhbApplication *app = GHB_APPLICATION_DEFAULT;
    g_return_val_if_fail(GHB_IS_APPLICATION(app), 0);

    return app->cancel_encode;
}

void
ghb_set_cancel_status (int status)
{
    GhbApplication *app = GHB_APPLICATION_DEFAULT;
    g_return_if_fail(GHB_IS_APPLICATION(app));

    app->cancel_encode = status;
}

int
ghb_get_queue_done_action (void)
{
    GhbApplication *app = GHB_APPLICATION_DEFAULT;
    g_return_val_if_fail(GHB_IS_APPLICATION(app), 0);

    return app->when_complete;
}

void
ghb_set_queue_done_action (int action)
{
    GhbApplication *app = GHB_APPLICATION_DEFAULT;
    g_return_if_fail(GHB_IS_APPLICATION(app));

    app->when_complete = action;
}

const char *
ghb_get_scan_source (void)
{
    GhbApplication *app = GHB_APPLICATION_DEFAULT;
    g_return_val_if_fail(GHB_IS_APPLICATION(app), "");
    g_return_val_if_fail(app->ud != NULL, "");

    return ghb_dict_get_string(app->ud->settings, "source");

}

void
ghb_set_scan_source (const char *source)
{
    GhbApplication *app = GHB_APPLICATION_DEFAULT;
    g_return_if_fail(GHB_IS_APPLICATION(app));
    g_return_if_fail(app->ud != NULL);

    ghb_dict_set_string(app->ud->settings, "source", source);
}

gboolean
ghb_get_load_queue (void)
{
    return !clear_queue;
}

gboolean
ghb_get_auto_start_queue (void)
{
    return auto_start_queue;
}
