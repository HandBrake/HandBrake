/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * subtitlehandler.h
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * subtitlehandler.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * subtitlehandler.h is distributed in the hope that it will be useful,
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
char * ghb_subtitle_short_description(const GhbValue *subsource,
                                      const GhbValue *subsettings);


void subtitle_list_selection_changed_cb(GtkTreeSelection *ts, signal_user_data_t *ud);
void subtitle_edit_clicked_cb(GtkWidget *widget, gchar *path, signal_user_data_t *ud);
void subtitle_remove_clicked_cb(GtkWidget *widget, gchar *path, signal_user_data_t *ud);


#endif // _SUBTITLEHANDLER_H_
