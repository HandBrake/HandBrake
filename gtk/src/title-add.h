/* title-add.h
 *
 * Copyright (C) 2008-2024 John Stebbins <stebbins@stebbins>
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

#if !defined(_TITLE_ADD_H_)
#define _TITLE_ADD_H_

#include "common.h"
#include "settings.h"
#include "hb-backend.h"

void ghb_finalize_job (GhbValue *settings);
gboolean ghb_add_title_to_queue (signal_user_data_t *ud,
                                 GhbValue *settings, gint batch);

#endif // _TITLE_ADD_H
