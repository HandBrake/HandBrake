/* Copyright (C) 2025 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include "common.h"
#include "settings.h"

G_BEGIN_DECLS

#define GHB_TYPE_APPLICATION          (ghb_application_get_type())
#define GHB_APPLICATION_DEFAULT       (GHB_APPLICATION(g_application_get_default()))
#define GHB_APPLICATION_ACTION(name)  (G_SIMPLE_ACTION(g_action_map_lookup_action \
    (G_ACTION_MAP(g_application_get_default()), (name))))

typedef struct _GhbApplication GhbApplication;

G_DECLARE_FINAL_TYPE(GhbApplication, ghb_application, GHB, APPLICATION, GtkApplication)

GhbApplication *ghb_application_new(const char *exe_name);

char *ghb_application_get_app_path(GhbApplication *self);
char *ghb_application_get_app_dir(GhbApplication *self);

signal_user_data_t *ghb_ud (void);
int ghb_get_cancel_status(void);
void ghb_set_cancel_status(int status);
gboolean ghb_get_load_queue(void);
gboolean ghb_get_auto_start_queue(void);
int ghb_get_queue_done_action(void);
void ghb_set_queue_done_action(int action);
void ghb_set_scan_source(const char *source);
const char *ghb_get_scan_source(void);

GtkWidget *ghb_builder_widget(const char *name);
GObject *ghb_builder_object(const char *name);

G_END_DECLS
