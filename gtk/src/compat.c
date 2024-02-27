/* compat.c
 *
 * Copyright (C) 2008-2024 John Stebbins <stebbins@stebbins>
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "compat.h"

/*
 * From https://gitlab.gnome.org/GNOME/gtk/-/blob/3.24.38/gtk/gtkdialog.c#L1240
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 */
typedef struct
{
    GtkDialog *dialog;
    gint response_id;
    GMainLoop *loop;
    gboolean destroyed;
} RunInfo;

static void
shutdown_loop (RunInfo *ri)
{
    if (g_main_loop_is_running(ri->loop))
         g_main_loop_quit(ri->loop);
}

static void
run_unmap_handler (GtkDialog *dialog, gpointer data)
{
    RunInfo *ri = data;

    shutdown_loop(ri);
}

static void
run_response_handler (GtkDialog *dialog, int response_id, gpointer data)
{
    RunInfo *ri;

    ri = data;

    ri->response_id = response_id;

    shutdown_loop(ri);
}

static void
run_destroy_handler (GtkDialog *dialog, gpointer data)
{
    RunInfo *ri = data;

    /* shutdown_loop will be called by run_unmap_handler */

    ri->destroyed = TRUE;
}

int ghb_dialog_run (GtkDialog *dialog)
{
    RunInfo ri = { NULL, GTK_RESPONSE_NONE, NULL, FALSE };
    gboolean was_modal;
    gulong response_handler;
    gulong unmap_handler;
    gulong destroy_handler;

    g_return_val_if_fail(GTK_IS_DIALOG(dialog), -1);

    g_object_ref(dialog);

    was_modal = gtk_window_get_modal(GTK_WINDOW(dialog));
    if (!was_modal)
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    if (!gtk_widget_get_visible(GTK_WIDGET(dialog)))
        gtk_widget_show(GTK_WIDGET(dialog));

    response_handler = g_signal_connect(dialog,
                                        "response",
                                        G_CALLBACK(run_response_handler),
                                        &ri);

    unmap_handler = g_signal_connect(dialog,
                                     "unmap",
                                     G_CALLBACK(run_unmap_handler),
                                     &ri);

    destroy_handler = g_signal_connect(dialog,
                                       "destroy",
                                       G_CALLBACK(run_destroy_handler),
                                       &ri);

    ri.loop = g_main_loop_new(NULL, FALSE);

    g_main_loop_run(ri.loop);

    g_main_loop_unref(ri.loop);

    ri.loop = NULL;

    if (!ri.destroyed)
    {
        if (!was_modal)
            gtk_window_set_modal(GTK_WINDOW(dialog), FALSE);

        g_signal_handler_disconnect(dialog, response_handler);
        g_signal_handler_disconnect(dialog, unmap_handler);
        g_signal_handler_disconnect(dialog, destroy_handler);
    }

    g_object_unref(dialog);

    return ri.response_id;
}

void
ghb_file_chooser_set_initial_file (GtkFileChooser *chooser, const char *file)
{
    if (!file || !file[0])
    {
        return;
    }
    else if (g_file_test(file, G_FILE_TEST_IS_DIR))
    {
        GFile *gfile = g_file_new_for_path(file);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), gfile, NULL);
        g_object_unref(gfile);
    }
    else
    {
        char *dir = g_path_get_dirname(file);
        if (g_file_test(dir, G_FILE_TEST_IS_DIR))
        {
            GFile *gfile = g_file_new_for_path(dir);
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), gfile, NULL);
            g_object_unref(gfile);
        }
        g_free(dir);
        if (gtk_file_chooser_get_action(chooser) == GTK_FILE_CHOOSER_ACTION_SAVE)
        {
            char *base = g_path_get_basename(file);
            gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser), base);
            g_free(base);
        }
    }
}

char *
ghb_file_chooser_get_filename (GtkFileChooser *chooser)
{
    g_autoptr(GFile) file = gtk_file_chooser_get_file(chooser);
    return g_file_get_path(file);
}

char *
ghb_file_chooser_get_current_folder (GtkFileChooser *chooser)
{
    g_autoptr(GFile) folder = gtk_file_chooser_get_current_folder(chooser);
    return g_file_get_path(folder);
}
