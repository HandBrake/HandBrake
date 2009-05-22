/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * subtitlehandler.h
 * Copyright (C) John Stebbins 2008 <stebbins@stebbins>
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
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#if !defined(_SUBTITLEHANDLER_H_)
#define _SUBTITLEHANDLER_H_

#include "settings.h"

void ghb_set_pref_subtitle(gint titleindex, signal_user_data_t *ud);
void ghb_clear_subtitle_list(signal_user_data_t *ud);
void ghb_set_subtitle(signal_user_data_t *ud, gint track, GValue *settings);
void ghb_subtitle_adjust_burn(signal_user_data_t *ud);
GValue* ghb_selected_subtitle_settings(signal_user_data_t *ud);
gint ghb_selected_subtitle_row(signal_user_data_t *ud);
void ghb_add_subtitle(
	signal_user_data_t *ud, GValue *settings, gboolean can_delete);
void ghb_reset_subtitles(signal_user_data_t *ud, GValue *settings);

#endif // _SUBTITLEHANDLER_H_
