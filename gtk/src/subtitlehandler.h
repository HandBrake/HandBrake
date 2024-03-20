/* subtitlehandler.h
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "common.h"
#include "settings.h"
#include "values.h"

G_BEGIN_DECLS

void ghb_set_subtitle(signal_user_data_t *ud, gint track, GhbValue *settings);
void ghb_subtitle_prune(signal_user_data_t *ud);
void ghb_subtitle_list_refresh_all(signal_user_data_t *ud);
void ghb_init_subtitle_defaults_ui(signal_user_data_t *ud);
void ghb_subtitle_defaults_to_ui(signal_user_data_t *ud);
void ghb_subtitle_set_actions_enabled(signal_user_data_t *ud, gboolean enabled);
void ghb_subtitle_set_pref_lang(GhbValue *settings);
void ghb_clear_subtitle_selection(void);
void ghb_add_subtitle_files(GListModel *files, signal_user_data_t *ud);
GhbValue *ghb_get_subtitle_list(GhbValue *settings);
GhbValue *ghb_get_subtitle_settings(GhbValue *settings);
char * ghb_subtitle_short_description(const GhbValue *subsource,
                                      const GhbValue *subsettings);
void subtitle_list_selection_changed_cb(GtkTreeSelection *ts, gpointer data);

G_END_DECLS
