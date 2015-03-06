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

#if !defined(_VIDEOHANDLER_H_)
#define _VIDEOHANDLER_H_

#include "values.h"
#include "settings.h"

int ghb_get_video_encoder(GhbValue *settings);
void ghb_video_setting_changed(GtkWidget *widget, signal_user_data_t *ud);

#endif // _VIDEOHANDLER_H_
