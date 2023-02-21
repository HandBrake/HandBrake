/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * title-add.h
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * title-add.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * title-add.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with title-add.h.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#if !defined(_TITLE_ADD_H_)
#define _TITLE_ADD_H_

#include <gtk/gtk.h>
#include "settings.h"
#include "hb-backend.h"

void ghb_finalize_job (GhbValue *settings);
gboolean ghb_add_title_to_queue (signal_user_data_t *ud,
                                 GhbValue *settings, gint batch);

#endif // _TITLE_ADD_H
