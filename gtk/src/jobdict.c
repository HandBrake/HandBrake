/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * jobdict.c
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
#include "jobdict.h"

GhbValue *ghb_get_job_settings(GhbValue *settings)
{
    GhbValue *job;
    job = ghb_dict_get(settings, "Job");
    if (job == NULL)
    {
        job = ghb_dict_new();
        ghb_dict_set(settings, "Job", job);
    }
    return job;
}

GhbValue* ghb_get_job_dest_settings(GhbValue *settings)
{
    GhbValue *job  = ghb_get_job_settings(settings);
    GhbValue *dest = ghb_dict_get(job, "Destination");
    if (dest == NULL)
    {
        dest = ghb_dict_new();
        ghb_dict_set(job, "Destination", dest);
    }
    return dest;
}

GhbValue* ghb_get_job_chapter_list(GhbValue *settings)
{
    GhbValue *dest     = ghb_get_job_dest_settings(settings);
    GhbValue *chapters = ghb_dict_get(dest, "ChapterList");
    if (chapters == NULL)
    {
        chapters = ghb_array_new();
        ghb_dict_set(dest, "ChapterList", chapters);
    }
    return chapters;
}

GhbValue* ghb_get_job_container_settings(GhbValue *settings)
{
    GhbValue *dest = ghb_get_job_dest_settings(settings);
    GhbValue *options = ghb_dict_get(dest, "Options");
    if (options == NULL)
    {
        options = ghb_dict_new();
        ghb_dict_set(dest, "Options", options);
    }
    return options;
}

GhbValue* ghb_get_job_source_settings(GhbValue *settings)
{
    GhbValue *job    = ghb_get_job_settings(settings);
    GhbValue *source = ghb_dict_get(job, "Source");
    if (source == NULL)
    {
        source = ghb_dict_new();
        ghb_dict_set(job, "Source", source);
    }
    return source;
}

int ghb_get_job_title_id(GhbValue *settings)
{
    GhbValue * source = ghb_get_job_source_settings(settings);
    GhbValue * title  = ghb_dict_get(source, "Title");
    if (title == NULL)
    {
        return -1;
    }
    return ghb_value_get_int(title);
}

GhbValue* ghb_get_job_range_settings(GhbValue *settings)
{
    GhbValue *source = ghb_get_job_source_settings(settings);
    GhbValue *range  = ghb_dict_get(source, "Range");
    if (range == NULL)
    {
        range = ghb_dict_new();
        ghb_dict_set(source, "Range", range);
    }
    return range;
}

GhbValue* ghb_get_job_par_settings(GhbValue *settings)
{
    GhbValue *job = ghb_get_job_settings(settings);
    GhbValue *par = ghb_dict_get(job, "PAR");
    if (par == NULL)
    {
        par = ghb_dict_new();
        ghb_dict_set(job, "PAR", par);
    }
    return par;
}

GhbValue* ghb_get_job_video_settings(GhbValue *settings)
{
    GhbValue *job   = ghb_get_job_settings(settings);
    GhbValue *video = ghb_dict_get(job, "Video");
    if (video == NULL)
    {
        video = ghb_dict_new();
        ghb_dict_set(job, "Video", video);
    }
    return video;
}

GhbValue *ghb_get_job_audio_settings(GhbValue *settings)
{
    GhbValue *job   = ghb_get_job_settings(settings);
    GhbValue *audio = ghb_dict_get(job, "Audio");
    if (audio == NULL)
    {
        audio = ghb_dict_new();
        ghb_dict_set(job, "Audio", audio);
    }
    return audio;
}

GhbValue *ghb_get_job_audio_list(GhbValue *settings)
{
    GhbValue *audio_dict = ghb_get_job_audio_settings(settings);
    GhbValue *audio_list = ghb_dict_get(audio_dict, "AudioList");
    if (audio_list == NULL)
    {
        audio_list = ghb_array_new();
        ghb_dict_set(audio_dict, "AudioList", audio_list);
    }
    return audio_list;
}

GhbValue *ghb_get_job_subtitle_settings(GhbValue *settings)
{
    GhbValue *job = ghb_get_job_settings(settings);
    GhbValue *sub = ghb_dict_get(job, "Subtitle");
    if (sub == NULL)
    {
        sub = ghb_dict_new();
        ghb_dict_set(job, "Subtitle", sub);
    }
    return sub;
}

GhbValue *ghb_get_job_subtitle_list(GhbValue *settings)
{
    GhbValue *sub_dict = ghb_get_job_subtitle_settings(settings);
    GhbValue *sub_list = ghb_dict_get(sub_dict, "SubtitleList");
    if (sub_list == NULL)
    {
        sub_list = ghb_array_new();
        ghb_dict_set(sub_dict, "SubtitleList", sub_list);
    }
    return sub_list;
}

GhbValue *ghb_get_job_subtitle_search(GhbValue *settings)
{
    GhbValue *sub_dict   = ghb_get_job_subtitle_settings(settings);
    GhbValue *sub_search = ghb_dict_get(sub_dict, "Search");
    if (sub_search == NULL)
    {
        sub_search = ghb_dict_new();
        ghb_dict_set(sub_dict, "Search", sub_search);
        ghb_dict_set_bool(sub_search, "Enable", 0);
    }
    return sub_search;
}

GhbValue* ghb_get_job_metadata_settings(GhbValue *settings)
{
    GhbValue *job  = ghb_get_job_settings(settings);
    GhbValue *meta = ghb_dict_get(job, "Metadata");
    if (meta == NULL)
    {
        meta = ghb_dict_new();
        ghb_dict_set(job, "Metadata", meta);
        meta = ghb_dict_get(job, "Metadata");
    }
    return meta;
}

GhbValue* ghb_get_job_filter_settings(GhbValue *settings)
{
    GhbValue *job    = ghb_get_job_settings(settings);
    GhbValue *filter = ghb_dict_get(job, "Filters");
    if (filter == NULL)
    {
        filter = ghb_dict_new();
        ghb_dict_set(job, "Filters", filter);
    }
    return filter;
}

GhbValue* ghb_get_job_filter_list(GhbValue *settings)
{
    GhbValue *filter = ghb_get_job_filter_settings(settings);
    GhbValue *list   = ghb_dict_get(filter, "FilterList");
    if (list == NULL)
    {
        list = ghb_dict_new();
        ghb_dict_set(filter, "FilterList", list);
    }
    return list;
}
