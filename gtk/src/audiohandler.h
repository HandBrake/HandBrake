/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * audiohandler.h
 * Copyright (C) John Stebbins 2008-2015 <stebbins@stebbins>
 *
 * audiohandler.h is free software.
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

#if !defined(_AUDIOHANDLER_H_)
#define _AUDIOHANDLER_H_

#include "values.h"
#include "settings.h"

GhbValue *ghb_get_audio_settings(GhbValue *settings);
GhbValue *ghb_get_audio_list(GhbValue *settings);
void ghb_sanitize_audio_tracks(signal_user_data_t *ud);
const gchar* ghb_get_user_audio_lang(GhbValue *settings, const hb_title_t *title, gint track);
void ghb_audio_list_refresh_selected(signal_user_data_t *ud);
gint ghb_select_audio_codec(gint mux, hb_audio_config_t *aconfig, gint acodec, gint fallback_acodec, gint copy_mask);
int ghb_select_fallback( GhbValue *settings, int acodec );
int ghb_get_copy_mask(GhbValue *settings);
void ghb_audio_list_refresh_all(signal_user_data_t *ud);
char * ghb_format_quality( const char *prefix, int codec, double quality );
void ghb_init_audio_defaults_ui(signal_user_data_t *ud);
void ghb_audio_defaults_to_ui(signal_user_data_t *ud);
GtkListBoxRow* ghb_find_lang_row(GtkListBox *list_box, int lang_idx);
void ghb_audio_title_change(signal_user_data_t *ud, gboolean title_valid);
void ghb_clear_audio_selection(GtkBuilder *builder);

#endif // _AUDIOHANDLER_H_
