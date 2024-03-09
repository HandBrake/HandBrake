/* jobdict.h
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

GhbValue* ghb_get_job_settings(GhbValue *settings);
GhbValue* ghb_get_job_source_settings(GhbValue *settings);
int       ghb_get_job_title_id(GhbValue *settings);
GhbValue* ghb_get_job_range_settings(GhbValue *settings);
GhbValue* ghb_get_job_par_settings(GhbValue *settings);
GhbValue* ghb_get_job_dest_settings(GhbValue *settings);
GhbValue* ghb_get_job_video_settings(GhbValue *settings);
GhbValue* ghb_get_job_metadata_settings(GhbValue *settings);
GhbValue* ghb_get_job_chapter_list(GhbValue *settings);
GhbValue* ghb_get_job_container_settings(GhbValue *settings);
GhbValue* ghb_get_job_audio_settings(GhbValue *settings);
GhbValue* ghb_get_job_audio_list(GhbValue *settings);
GhbValue* ghb_get_job_subtitle_settings(GhbValue *settings);
GhbValue* ghb_get_job_subtitle_list(GhbValue *settings);
GhbValue* ghb_get_job_subtitle_search(GhbValue *settings);
GhbValue* ghb_get_job_filter_settings(GhbValue *settings);
GhbValue* ghb_get_job_filter_list(GhbValue *settings);

G_END_DECLS