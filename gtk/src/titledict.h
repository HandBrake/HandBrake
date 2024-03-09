/* titledict.h
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
#include "values.h"

G_BEGIN_DECLS

GhbValue* ghb_get_title_settings(GhbValue *settings);
GhbValue* ghb_get_title_audio_list(GhbValue *settings);
GhbValue *ghb_get_title_audio_track(GhbValue *settings, int track);
GhbValue* ghb_get_title_subtitle_list(GhbValue *settings);
GhbValue *ghb_get_title_subtitle_track(GhbValue *settings, int track);

G_END_DECLS
