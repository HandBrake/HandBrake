/* audiohandler.h
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
#include "settings.h"
#include "values.h"

G_BEGIN_DECLS

GhbValue *ghb_get_audio_settings(GhbValue *settings);
GhbValue *ghb_get_audio_list(GhbValue *settings);
void ghb_sanitize_audio_track_settings(GhbValue *settings);
const gchar* ghb_get_user_audio_lang(GhbValue *settings, const hb_title_t *title, gint track);
void ghb_audio_list_refresh_selected(signal_user_data_t *ud);
gint ghb_select_audio_codec(gint mux, guint32 in_codec, gint acodec, gint fallback_acodec, gint copy_mask);
int ghb_select_fallback( GhbValue *settings, int acodec );
int ghb_get_copy_mask(GhbValue *settings);
void ghb_audio_list_refresh_all(signal_user_data_t *ud);
char * ghb_format_quality( const char *prefix, int codec, double quality );
void ghb_init_audio_defaults_ui(signal_user_data_t *ud);
void ghb_audio_defaults_to_ui(signal_user_data_t *ud);
gboolean ghb_find_lang_row(GtkTreeModel *model, GtkTreeIter *iter, int idx);
void ghb_audio_set_actions_enabled(signal_user_data_t *ud, gboolean enabled);
void ghb_clear_audio_selection(void);
gboolean ghb_audio_quality_enabled(const GhbValue *asettings);

void audio_list_selection_changed_cb(GtkTreeSelection *ts, gpointer data);

G_END_DECLS
