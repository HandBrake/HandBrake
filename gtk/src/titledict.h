/*
 * titledict.h
 * Copyright (C) John Stebbins 2008-2019 <stebbins@stebbins>
 *
 * settings.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
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

#if !defined(_TITLEDICT_H_)
#define _TITLEDICT_H_

#include "values.h"

GhbValue* ghb_get_title_settings(GhbValue *settings);
GhbValue* ghb_get_title_audio_list(GhbValue *settings);
GhbValue *ghb_get_title_audio_track(GhbValue *settings, int track);
GhbValue* ghb_get_title_subtitle_list(GhbValue *settings);
GhbValue *ghb_get_title_subtitle_track(GhbValue *settings, int track);

#endif // _TITLEDICT_H_
