/* compat.h
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#define G_LOG_USE_STRUCTURED
#define G_LOG_DOMAIN "ghb"

#define ghb_log_func() g_debug("Function: %s", __func__)
#define ghb_log_func_str(x) g_debug("Function: %s (%s)", __func__, (x))

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <string.h>

#if defined(_WIN32)
#define GHB_UNSAFE_FILENAME_CHARS "/:<>\"\\|?*"
#else
#define GHB_UNSAFE_FILENAME_CHARS "/"
#endif

G_BEGIN_DECLS

int ghb_dialog_run(GtkDialog *dialog);
void ghb_file_chooser_set_initial_file(GtkFileChooser *chooser, const char *file);
char *ghb_file_chooser_get_filename(GtkFileChooser *chooser);
char *ghb_file_chooser_get_current_folder(GtkFileChooser *chooser);

G_END_DECLS

