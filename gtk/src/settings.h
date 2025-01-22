/* settings.h
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

#pragma once

#include "common.h"
#include "values.h"

G_BEGIN_DECLS

#define GHB_ACTION(n) g_action_map_lookup_action(G_ACTION_MAP(g_application_get_default()), (n))

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
    char                * current_dvd_device;
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
    preview_t           * preview;
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

GhbValue* ghb_widget_value(GtkWidget *widget);
gchar* ghb_widget_string(GtkWidget *widget);
gdouble ghb_widget_double(GtkWidget *widget);
gint64 ghb_widget_int64(GtkWidget *widget);
gint ghb_widget_int(GtkWidget *widget);
gint ghb_widget_boolean(GtkWidget *widget);

void ghb_widget_to_setting(GhbValue *settings, GtkWidget *widget);
int ghb_ui_update(const char *name, const GhbValue *value);
int ghb_ui_update_from_settings(const char *name, const GhbValue *settings);
int ghb_ui_settings_update(
    signal_user_data_t *ud, GhbValue *settings, const gchar *name,
    const GhbValue *value);
const gchar* ghb_get_setting_key(GtkWidget *widget);
void ghb_update_widget(GtkWidget *widget, const GhbValue *value);

G_END_DECLS
