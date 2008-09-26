/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * x264handler.h
 * Copyright (C) John Stebbins 2008 <stebbins@stebbins>
 * 
 * x264handler.h is free software.
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

#if !defined(_X264HANDLER_H_)
#define _X264HANDLER_H_

#include "settings.h"

void ghb_x264_parse_options(signal_user_data_t *ud, const gchar *options);

#endif // _X264HANDLER_H_
