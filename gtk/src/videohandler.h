/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * videohandler.h
 * Copyright (C) John Stebbins 2008-2023 <stebbins@stebbins>
 *
 * videohandler.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * videohandler.h is distributed in the hope that it will be useful,
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

#if !defined(_VIDEOHANDLER_H_)
#define _VIDEOHANDLER_H_

#include "values.h"
#include "settings.h"

int ghb_get_video_encoder(GhbValue *settings);
void ghb_video_setting_changed(GtkWidget *widget, signal_user_data_t *ud);
int ghb_set_video_preset(GhbValue *settings, int encoder, const char * preset);

#endif // _VIDEOHANDLER_H_
