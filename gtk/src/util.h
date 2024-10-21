/* util.h
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

#pragma once

#include "common.h"

G_BEGIN_DECLS

int ghb_dialog_run(GtkDialog *dialog);
GtkFileChooser *ghb_file_chooser_new(const char *title, GtkWindow *parent,
    GtkFileChooserAction action, const char *accept_label, const char *cancel_label);
void ghb_file_chooser_set_modal(GtkFileChooser *chooser, gboolean modal);
void ghb_file_chooser_show(GtkFileChooser *chooser);
void ghb_file_chooser_destroy(GtkFileChooser *chooser);
void ghb_file_chooser_set_initial_file(GtkFileChooser *chooser, const char *file);
char *ghb_file_chooser_get_filename(GtkFileChooser *chooser);
char *ghb_file_chooser_get_current_folder(GtkFileChooser *chooser);
gboolean ghb_file_is_subtitle(const char *filename);
gboolean ghb_file_is_ssa_subtitle(const char *filename);
gboolean ghb_file_is_srt_subtitle(const char *filename);

G_END_DECLS

