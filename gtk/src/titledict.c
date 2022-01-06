/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * titledict.c
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * settings.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * settings.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#include "values.h"
#include "titledict.h"

GhbValue *ghb_get_title_settings(GhbValue *settings)
{
    GhbValue *title;
    title = ghb_dict_get(settings, "Title");
    return title;
}

GhbValue *ghb_get_title_audio_list(GhbValue *settings)
{
    GhbValue *title_dict = ghb_get_title_settings(settings);
    GhbValue *audio_list = ghb_dict_get(title_dict, "AudioList");
    return audio_list;
}

GhbValue *ghb_get_title_audio_track(GhbValue *settings, int track)
{
    GhbValue *audio_list  = ghb_get_title_audio_list(settings);
    GhbValue *audio_track = ghb_array_get(audio_list, track);
    return audio_track;
}

GhbValue *ghb_get_title_subtitle_list(GhbValue *settings)
{
    GhbValue *title_dict = ghb_get_title_settings(settings);
    GhbValue *subtitle_list = ghb_dict_get(title_dict, "SubtitleList");
    return subtitle_list;
}

GhbValue *ghb_get_title_subtitle_track(GhbValue *settings, int track)
{
    GhbValue *subtitle_list  = ghb_get_title_subtitle_list(settings);
    GhbValue *subtitle_track = ghb_array_get(subtitle_list, track);
    return subtitle_track;
}

