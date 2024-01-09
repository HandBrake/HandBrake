/*
 * settings.h
 * Copyright (C) John Stebbins 2008-2024 <stebbins@stebbins>
 *
 * settings.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * settings.h is distributed in the hope that it will be useful,
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

#if !defined(_SETTINGS_H_)
#define _SETTINGS_H_

#include <gtk/gtk.h>
#include "values.h"

#define GHB_WIDGET(b,n) GTK_WIDGET(gtk_builder_get_object ((b), (n)))
#define GHB_ACTION(b,n) g_action_map_lookup_action(G_ACTION_MAP(gtk_builder_get_application (b)), (n))
//#define GHB_WIDGET(b,n)   GTK_WIDGET(debug_get_object((b), (n)))
#define GHB_OBJECT(b,n) gtk_builder_get_object ((b), (n))

GObject* debug_get_object(GtkBuilder *b, const gchar *n);

enum
{
    GHB_STATE_IDLE      = 0x00,
    GHB_STATE_SCANNING  = 0x02,
    GHB_STATE_SCANDONE  = 0x04,
    GHB_STATE_WORKING   = 0x08,
    GHB_STATE_WORKDONE  = 0x10,
    GHB_STATE_PAUSED    = 0x20,
    GHB_STATE_MUXING    = 0x40,
    GHB_STATE_SEARCHING = 0x80,
};

enum
{
    GHB_CANCEL_NONE,
    GHB_CANCEL_ALL,
    GHB_CANCEL_CURRENT,
    GHB_CANCEL_FINISH
};

typedef struct preview_s preview_t;

typedef struct
{
    gchar *current_dvd_device;
    gboolean              dont_clear_presets;
    gboolean              scale_busy;
    gint                  cancel_encode;
    gint                  when_complete;
    GtkBuilder          * builder;
    GhbValue            * globals;
    GhbValue            * prefs;
    GhbValue            * settings;
    GhbValue            * settings_array;
    GhbValue            * queue;
    GIOChannel          * activity_log;
    GIOChannel          * job_activity_log;
    GtkTextBuffer       * activity_buffer;
    GtkTextBuffer       * queue_activity_buffer;
    GtkTextBuffer       * extra_activity_buffer;
    char                * extra_activity_path;
    gboolean              append_queue_activity;
    preview_t           * preview;
    int                   stderr_src_id;
    GtkFileChooserNative* source_dialog;
} signal_user_data_t;

enum
{
    GHB_QUEUE_PENDING,
    GHB_QUEUE_RUNNING,
    GHB_QUEUE_CANCELED,
    GHB_QUEUE_FAIL,
    GHB_QUEUE_DONE,
};

void ghb_settings_copy(
    GhbValue *settings, const gchar *key, const GhbValue *value);
gint ghb_settings_combo_int(const GhbValue *settings, const gchar *key);
gdouble ghb_settings_combo_double(const GhbValue *settings, const gchar *key);
gchar* ghb_settings_combo_option(const GhbValue *settings, const gchar *key);

GhbValue *ghb_get_job_settings(GhbValue *settings);
GhbValue* ghb_widget_value(GtkWidget *widget);
gchar* ghb_widget_string(GtkWidget *widget);
gdouble ghb_widget_double(GtkWidget *widget);
gint64 ghb_widget_int64(GtkWidget *widget);
gint ghb_widget_int(GtkWidget *widget);
gint ghb_widget_boolean(GtkWidget *widget);

void ghb_widget_to_setting(GhbValue *settings, GtkWidget *widget);
int ghb_ui_update(
    signal_user_data_t *ud, const gchar *name, const GhbValue *value);
int ghb_ui_update_from_settings(
    signal_user_data_t *ud, const gchar *name, const GhbValue *settings);
int ghb_ui_settings_update(
    signal_user_data_t *ud, GhbValue *settings, const gchar *name,
    const GhbValue *value);
const gchar* ghb_get_setting_key(GtkWidget *widget);
void ghb_update_widget(GtkWidget *widget, const GhbValue *value);

#endif // _SETTINGS_H_
