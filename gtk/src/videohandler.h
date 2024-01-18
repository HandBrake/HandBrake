/* videohandler.h
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

#pragma once

#include "common.h"
#include "settings.h"
#include "values.h"

G_BEGIN_DECLS

int ghb_get_video_encoder(GhbValue *settings);
void ghb_video_setting_changed(GtkWidget *widget, signal_user_data_t *ud);
int ghb_set_video_preset(GhbValue *settings, int encoder, const char * preset);

G_END_DECLS
