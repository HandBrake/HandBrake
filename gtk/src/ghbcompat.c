/*
 * ghbcompat.h
 * Copyright (C) John Stebbins 2008-2024 <stebbins@stebbins>
 *
 * ghbcompat.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * ghbcompat.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with callbacks.h.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#include "ghbcompat.h"

G_MODULE_EXPORT gboolean
ghb_widget_hide_on_close(
    GtkWidget *widget,
#if !GTK_CHECK_VERSION(4, 4, 0)
    GdkEvent *event,
#endif
    gpointer *ud)
{
    gtk_widget_set_visible(widget, FALSE);
    return TRUE;
}

/*
 * From https://gitlab.gnome.org/GNOME/gtk/-/blob/3.24.38/gtk/gtkdialog.c#L1240
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 */
#if GTK_CHECK_VERSION(4, 4, 0)
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

static int
run_delete_handler (GtkDialog *dialog, GdkEventAny *event, gpointer data)
{
    RunInfo *ri = data;

    shutdown_loop(ri);

    return TRUE; /* Do not destroy */
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
    gulong delete_handler;

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

    delete_handler = g_signal_connect(dialog,
                                      "delete-event",
                                      G_CALLBACK(run_delete_handler),
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
        g_signal_handler_disconnect(dialog, delete_handler);
        g_signal_handler_disconnect(dialog, destroy_handler);
    }

    g_object_unref(dialog);

    return ri.response_id;
}
#else
int ghb_dialog_run (GtkDialog *dialog)
{
    return gtk_dialog_run(dialog);
}
#endif
