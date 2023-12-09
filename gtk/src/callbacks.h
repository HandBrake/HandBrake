/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.h
 * Copyright (C) John Stebbins 2008-2023 <stebbins@stebbins>
 *
 * callbacks.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * callbacks.h is distributed in the hope that it will be useful,
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

#if !defined(_CALLBACKS_H_)
#define _CALLBACKS_H_

#if defined(_WIN32)
#include <windows.h>
#endif

#include <gtk/gtk.h>
#include "handbrake/handbrake.h"
#include "values.h"
#include "settings.h"

#if GLIB_CHECK_VERSION(2, 32, 0)
#define GHB_THREAD_NEW(n, f, p) \
                g_thread_new(n, (GThreadFunc)(f), (p))
#else
#define GHB_THREAD_NEW(n, f, p) \
                g_thread_create((GThreadFunc)(f), (p), TRUE, NULL)
#endif

typedef enum {
    GHB_ACTION_NORMAL,
    GHB_ACTION_SUGGESTED,
    GHB_ACTION_DESTRUCTIVE,
} GhbActionStyle;

gboolean ghb_timer_cb(gpointer data);
gboolean ghb_log_cb(GIOChannel *source, GIOCondition cond, gpointer data);
void ghb_hbfd(signal_user_data_t *ud, gboolean hbfd);
gboolean ghb_file_menu_add_dvd(signal_user_data_t *ud);
void ghb_countdown_dialog_show(const gchar *message, const char *action,
    GSourceFunc action_func, int timeout, signal_user_data_t *ud);
gboolean ghb_question_dialog_run(GtkWindow *parent, GhbActionStyle accept_style,
     const char *accept_button, const char *cancel_button,
     const char *title, const char *format, ...) G_GNUC_PRINTF(6, 7);
void ghb_alert_dialog_show(GtkMessageType type, const char *title,
                           const char *format, ...) G_GNUC_PRINTF(3, 4);
GtkWidget *ghb_cancel_dialog_new(GtkWindow *parent,
    const char *title, const char *message, const char *cancel_all_button,
    const char *cancel_current_button, const char *finish_button,
    const char *continue_button);
void ghb_stop_encode_dialog_show(signal_user_data_t *ud);
void ghb_start_next_job(signal_user_data_t *ud);
void ghb_bind_dependencies (signal_user_data_t *ud);
void ghb_do_scan( signal_user_data_t *ud, const gchar *filename,
    gint titlenum, gboolean force);
void ghb_log(gchar *log, ...);
#if defined(_WIN32)
void wm_drive_changed(MSG *msg, signal_user_data_t *ud);
#endif
gpointer ghb_cache_volnames(signal_user_data_t *ud);
gboolean ghb_check_name_template(signal_user_data_t *ud, const char *str);
void ghb_volname_cache_init(void);
void ghb_update_destination_extension(signal_user_data_t *ud);
void ghb_update_pending(signal_user_data_t *ud);
gboolean ghb_idle_scan(signal_user_data_t *ud);
void ghb_add_all_titles(signal_user_data_t *ud);
void ghb_update_title_info(signal_user_data_t *ud);
void ghb_load_settings(signal_user_data_t * ud);
void ghb_load_post_settings(signal_user_data_t * ud);
void ghb_set_current_title_settings(signal_user_data_t *ud);
void ghb_container_empty(GtkContainer *c);
void ghb_show_container_options(signal_user_data_t *ud);
void ghb_scale_configure(signal_user_data_t *ud, char *name, double val,
                         double min, double max, double step, double page,
                         int digits, gboolean inverted);
void ghb_update_summary_info(signal_user_data_t *ud);
void ghb_set_title_settings(signal_user_data_t *ud, GhbValue *settings);
void ghb_browse_uri(signal_user_data_t *ud, const gchar *uri);
void ghb_set_destination(signal_user_data_t *ud);
void ghb_break_pts_duration(gint64 ptsDuration,
                            gint *hh, gint *mm, gdouble *ss);
void ghb_break_duration(gint64 duration, gint *hh, gint *mm, gint *ss);
GtkFileFilter *ghb_add_file_filter(GtkFileChooser *chooser,
                                   signal_user_data_t *ud,
                                   const char *name, const char *id);

#endif // _CALLBACKS_H_
