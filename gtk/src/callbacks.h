/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.h
 * Copyright (C) John Stebbins 2008-2015 <stebbins@stebbins>
 *
 * callbacks.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
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
#include "hb.h"
#include "values.h"
#include "settings.h"

#if GLIB_CHECK_VERSION(2, 32, 0)
#define GHB_THREAD_NEW(n, f, p) \
                g_thread_new(n, (GThreadFunc)(f), (p))
#else
#define GHB_THREAD_NEW(n, f, p) \
                g_thread_create((GThreadFunc)(f), (p), TRUE, NULL)
#endif

void ghb_check_all_depencencies(signal_user_data_t *ud);
gboolean ghb_timer_cb(gpointer data);
gboolean ghb_log_cb(GIOChannel *source, GIOCondition cond, gpointer data);
void warn_log_handler(
    const gchar *domain, GLogLevelFlags flags, const gchar *msg, gpointer ud);
void debug_log_handler(
    const gchar *domain, GLogLevelFlags flags, const gchar *msg, gpointer ud);
void ghb_hbfd(signal_user_data_t *ud, gboolean hbfd);
gboolean ghb_file_menu_add_dvd(signal_user_data_t *ud);
void ghb_udev_init(void);
gboolean ghb_message_dialog(
    GtkWindow *parent, GtkMessageType type, const gchar *message,
    const gchar *no, const gchar *yes);
void ghb_error_dialog(
    GtkWindow *parent, GtkMessageType type,
    const gchar *message, const gchar *cancel);
void ghb_init_dep_map(void);
void ghb_cancel_encode(signal_user_data_t *ud, const gchar *extra_msg);
gboolean ghb_cancel_encode2(signal_user_data_t *ud, const gchar *extra_msg);
GhbValue* ghb_start_next_job(signal_user_data_t *ud);
void ghb_check_dependency(
    signal_user_data_t *ud, GtkWidget *widget, const gchar *alt_name);
void ghb_do_scan( signal_user_data_t *ud, const gchar *filename,
    gint titlenum, gboolean force);
void ghb_log(gchar *log, ...);
gpointer ghb_check_update(signal_user_data_t *ud);
void ghb_uninhibit_gsm(void);
void ghb_inhibit_gsm(signal_user_data_t *ud);
#if defined(_WIN32)
void wm_drive_changed(MSG *msg, signal_user_data_t *ud);
#endif
gpointer ghb_cache_volnames(signal_user_data_t *ud);
void ghb_volname_cache_init(void);
void ghb_update_destination_extension(signal_user_data_t *ud);
void ghb_update_pending(signal_user_data_t *ud);
gboolean ghb_idle_scan(signal_user_data_t *ud);
void ghb_add_all_titles(signal_user_data_t *ud);
void ghb_update_title_info(signal_user_data_t *ud);
void ghb_chapter_list_refresh_all(signal_user_data_t *ud);
void ghb_load_settings(signal_user_data_t * ud);
void ghb_load_post_settings(signal_user_data_t * ud);
void ghb_set_current_title_settings(signal_user_data_t *ud);
void ghb_container_empty(GtkContainer *c);
void ghb_show_container_options(signal_user_data_t *ud);
void ghb_scale_configure(signal_user_data_t *ud, char *name, double val,
                         double min, double max, double step, double page,
                         int digits, gboolean inverted);

#endif // _CALLBACKS_H_

