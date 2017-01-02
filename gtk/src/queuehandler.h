/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * queuehandler.h
 * Copyright (C) John Stebbins 2008-2017 <stebbins@stebbins>
 *
 * queuehandler.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * queuehandler.h is distributed in the hope that it will be useful,
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

#if !defined(_QUEUEHANDLER_H_)
#define _QUEUEHANDLER_H_

#include <gtk/gtk.h>
#include "settings.h"

void ghb_queue_buttons_grey(signal_user_data_t *ud);
gboolean ghb_reload_queue(signal_user_data_t *ud);
void ghb_queue_remove_row(signal_user_data_t *ud, int row);
void ghb_finalize_job(GhbValue *settings);

#endif // _QUEUEHANDLER_H_
