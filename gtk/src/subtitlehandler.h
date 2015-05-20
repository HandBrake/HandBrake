/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * subtitlehandler.h
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

#if !defined(_SUBTITLEHANDLER_H_)
#define _SUBTITLEHANDLER_H_

#include "values.h"
#include "settings.h"

void ghb_set_subtitle(signal_user_data_t *ud, gint track, GhbValue *settings);
void ghb_subtitle_prune(signal_user_data_t *ud);
void ghb_subtitle_list_refresh_all(signal_user_data_t *ud);
void ghb_init_subtitle_defaults_ui(signal_user_data_t *ud);
void ghb_subtitle_defaults_to_ui(signal_user_data_t *ud);
void ghb_subtitle_title_change(signal_user_data_t *ud, gboolean show);
void ghb_subtitle_set_pref_lang(GhbValue *settings);
void ghb_clear_subtitle_selection(GtkBuilder *builder);
GhbValue *ghb_get_subtitle_list(GhbValue *settings);
GhbValue *ghb_get_subtitle_settings(GhbValue *settings);

#endif // _SUBTITLEHANDLER_H_
