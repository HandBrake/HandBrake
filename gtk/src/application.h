/* Copyright (C) 2024 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <glib.h>
#include "config.h"

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

G_END_DECLS
