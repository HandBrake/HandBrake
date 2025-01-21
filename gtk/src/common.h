/* Copyright (C) 2025 HandBrake Team
 * SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#define G_LOG_USE_STRUCTURED
#ifndef G_LOG_DOMAIN
#define G_LOG_DOMAIN "ghb"
#endif

#include "config.h"

#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ghb_log_func() g_debug("Function: %s", __func__)
#define ghb_log_func_str(x) g_debug("Function: %s (%s)", __func__, (x))

G_END_DECLS
