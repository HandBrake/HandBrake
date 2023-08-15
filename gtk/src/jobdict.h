/*
 * settings.h
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * settings.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * settings.h is distributed in the hope that it will be useful,
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

#if !defined(_JOBDICT_H_)
#define _JOBDICT_H_

#include "values.h"

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

#endif // _JOBDICT_H_
