/* json.c

   Copyright (c) 2003-2014 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <jansson.h>
#include "hb_json.h"
#include "libavutil/base64.h"

/**
 * Convert an hb_state_t to a jansson dict
 * @param state - Pointer to hb_state_t to convert
 */
static json_t* hb_state_to_dict( hb_state_t * state)
{
    json_t *dict = NULL;
    json_error_t error;

    switch (state->state)
    {
    case HB_STATE_IDLE:
        dict = json_pack_ex(&error, 0, "{s:o}",
                    "State", json_integer(state->state));
        break;
    case HB_STATE_SCANNING:
    case HB_STATE_SCANDONE:
        dict = json_pack_ex(&error, 0,
            "{s:o, s{s:o, s:o, s:o, s:o, s:o}}",
            "State", json_integer(state->state),
            "Scanning",
                "Progress",     json_real(state->param.scanning.progress),
                "Preview",      json_integer(state->param.scanning.preview_cur),
                "PreviewCount", json_integer(state->param.scanning.preview_count),
                "Title",        json_integer(state->param.scanning.title_cur),
                "TitleCount",   json_integer(state->param.scanning.title_count));
        break;
    case HB_STATE_WORKING:
    case HB_STATE_PAUSED:
    case HB_STATE_SEARCHING:
        dict = json_pack_ex(&error, 0,
            "{s:o, s{s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o}}",
            "State", json_integer(state->state),
            "Working",
                "Progress",     json_real(state->param.working.progress),
                "Job",          json_integer(state->param.working.job_cur),
                "JobCount",     json_integer(state->param.working.job_count),
                "Rate",         json_real(state->param.working.rate_cur),
                "RateAvg",      json_real(state->param.working.rate_avg),
                "Hours",        json_integer(state->param.working.hours),
                "Minutes",      json_integer(state->param.working.minutes),
                "Seconds",      json_integer(state->param.working.seconds),
                "SequenceID",   json_integer(state->param.working.sequence_id));
        break;
    case HB_STATE_WORKDONE:
        dict = json_pack_ex(&error, 0,
            "{s:o, s{s:o}}",
            "State", json_integer(state->state),
            "WorkDone",
                "Error",    json_integer(state->param.workdone.error));
        break;
    case HB_STATE_MUXING:
        dict = json_pack_ex(&error, 0,
            "{s:o, s{s:o}}",
            "State", json_integer(state->state),
            "Muxing",
                "Progress", json_real(state->param.muxing.progress));
        break;
    default:
        hb_error("hb_state_to_json: unrecognized state %d", state->state);
        break;
    }
    if (dict == NULL)
    {
        hb_error("json pack failure: %s", error.text);
    }
    return dict;
}

/**
 * Get the current state of an hb instance as a json string
 * @param h - Pointer to an hb_handle_t hb instance
 */
char* hb_get_state_json( hb_handle_t * h )
{
    hb_state_t state;

    hb_get_state(h, &state);
    json_t *dict = hb_state_to_dict(&state);

    char *json_state = json_dumps(dict, JSON_INDENT(4)|JSON_PRESERVE_ORDER);
    json_decref(dict);

    return json_state;
}

/**
 * Convert an hb_title_t to a jansson dict
 * @param title - Pointer to the hb_title_t to convert
 */
static json_t* hb_title_to_dict( const hb_title_t * title )
{
    json_t *dict;
    json_error_t error;
    int ii;

    dict = json_pack_ex(&error, 0,
    "{"
        // Type, Path, Name, Index, Playlist, AngleCount
        "s:o, s:o, s:o, s:o, s:o, s:o,"
        // Duration {Ticks, Hours, Minutes, Seconds}
        "s:{s:o, s:o, s:o, s:o},"
        // Geometry {Width, Height, PAR {Num, Den},
        "s:{s:o, s:o, s:{s:o, s:o}},"
        // Crop[Top, Bottom, Left, Right]}
        "s:[oooo],"
        // Color {Primary, Transfer, Matrix}
        "s:{s:o, s:o, s:o},"
        // FrameRate {Num, Den}
        "s:{s:o, s:o},"
        // InterlaceDetected, VideoCodec
        "s:o, s:o,"
        // MetaData
        "s:{}"
    "}",
    "Type",                 json_integer(title->type),
    "Path",                 json_string(title->path),
    "Name",                 json_string(title->name),
    "Index",                json_integer(title->index),
    "Playlist",             json_integer(title->playlist),
    "AngleCount",           json_integer(title->angle_count),
    "Duration",
        "Ticks",            json_integer(title->duration),
        "Hours",            json_integer(title->hours),
        "Minutes",          json_integer(title->minutes),
        "Seconds",          json_integer(title->seconds),
    "Geometry",
        "Width",            json_integer(title->geometry.width),
        "Height",           json_integer(title->geometry.height),
        "PAR",
            "Num",          json_integer(title->geometry.par.num),
            "Den",          json_integer(title->geometry.par.den),
    "Crop",                 json_integer(title->crop[0]),
                            json_integer(title->crop[1]),
                            json_integer(title->crop[2]),
                            json_integer(title->crop[3]),
    "Color",
        "Primary",          json_integer(title->color_prim),
        "Transfer",         json_integer(title->color_transfer),
        "Matrix",           json_integer(title->color_matrix),
    "FrameRate",
        "Num",              json_integer(title->vrate.num),
        "Den",              json_integer(title->vrate.den),
    "InterlaceDetected",    json_boolean(title->detected_interlacing),
    "VideoCodec",           json_string(title->video_codec_name),
    "MetaData"
    );
    if (dict == NULL)
    {
        hb_error("json pack failure: %s", error.text);
        return NULL;
    }

    if (title->container_name != NULL)
    {
        json_object_set_new(dict, "Container",
                            json_string(title->container_name));
    }

    // Add metadata
    json_t *meta_dict = json_object_get(dict, "MetaData");
    if (title->metadata->name != NULL)
    {
        json_object_set_new(meta_dict, "Name",
                            json_string(title->metadata->name));
    }
    if (title->metadata->artist != NULL)
    {
        json_object_set_new(meta_dict, "Artist",
                            json_string(title->metadata->artist));
    }
    if (title->metadata->composer != NULL)
    {
        json_object_set_new(meta_dict, "Composer",
                            json_string(title->metadata->composer));
    }
    if (title->metadata->comment != NULL)
    {
        json_object_set_new(meta_dict, "Comment",
                            json_string(title->metadata->comment));
    }
    if (title->metadata->genre != NULL)
    {
        json_object_set_new(meta_dict, "Genre",
                            json_string(title->metadata->genre));
    }
    if (title->metadata->album != NULL)
    {
        json_object_set_new(meta_dict, "Album",
                            json_string(title->metadata->album));
    }
    if (title->metadata->album_artist != NULL)
    {
        json_object_set_new(meta_dict, "AlbumArtist",
                            json_string(title->metadata->album_artist));
    }
    if (title->metadata->description != NULL)
    {
        json_object_set_new(meta_dict, "Description",
                            json_string(title->metadata->description));
    }
    if (title->metadata->long_description != NULL)
    {
        json_object_set_new(meta_dict, "LongDescription",
                            json_string(title->metadata->long_description));
    }

    // process chapter list
    json_t * chapter_list = json_array();
    for (ii = 0; ii < hb_list_count(title->list_chapter); ii++)
    {
        json_t *chapter_dict;
        char *name = "";
        hb_chapter_t *chapter = hb_list_item(title->list_chapter, ii);
        if (chapter->title != NULL)
            name = chapter->title;

        chapter_dict = json_pack_ex(&error, 0,
            "{s:o, s:{s:o, s:o, s:o, s:o}}",
            "Name",         json_string(name),
            "Duration",
                "Ticks",    json_integer(chapter->duration),
                "Hours",    json_integer(chapter->hours),
                "Minutes",  json_integer(chapter->minutes),
                "Seconds",  json_integer(chapter->seconds)
        );
        if (chapter_dict == NULL)
        {
            hb_error("json pack failure: %s", error.text);
            return NULL;
        }
        json_array_append_new(chapter_list, chapter_dict);
    }
    json_object_set_new(dict, "ChapterList", chapter_list);

    // process audio list
    json_t * audio_list = json_array();
    for (ii = 0; ii < hb_list_count(title->list_audio); ii++)
    {
        json_t *audio_dict;
        hb_audio_t *audio = hb_list_item(title->list_audio, ii);

        audio_dict = json_pack_ex(&error, 0,
        "{s:o, s:o, s:o, s:o, s:o, s:o}",
            "Description",      json_string(audio->config.lang.description),
            "Language",         json_string(audio->config.lang.simple),
            "LanguageCode",     json_string(audio->config.lang.iso639_2),
            "SampleRate",       json_integer(audio->config.in.samplerate),
            "BitRate",          json_integer(audio->config.in.bitrate),
            "ChannelLayout",    json_integer(audio->config.in.channel_layout));
        if (audio_dict == NULL)
        {
            hb_error("json pack failure: %s", error.text);
            return NULL;
        }
        json_array_append_new(audio_list, audio_dict);
    }
    json_object_set_new(dict, "AudioList", audio_list);

    // process subtitle list
    json_t * subtitle_list = json_array();
    for (ii = 0; ii < hb_list_count(title->list_subtitle); ii++)
    {
        json_t *subtitle_dict;
        hb_subtitle_t *subtitle = hb_list_item(title->list_subtitle, ii);

        subtitle_dict = json_pack_ex(&error, 0,
            "{s:o, s:o, s:o, s:o}",
            "Format",       json_integer(subtitle->format),
            "Source",       json_integer(subtitle->source),
            "Language",     json_string(subtitle->lang),
            "LanguageCode", json_string(subtitle->iso639_2));
        if (subtitle_dict == NULL)
        {
            hb_error("json pack failure: %s", error.text);
            return NULL;
        }
        json_array_append_new(subtitle_list, subtitle_dict);
    }
    json_object_set_new(dict, "SubtitleList", subtitle_list);

    return dict;
}

/**
 * Convert an hb_title_set_t to a jansson dict
 * @param title - Pointer to the hb_title_set_t to convert
 */
static json_t* hb_title_set_to_dict( const hb_title_set_t * title_set )
{
    json_t *dict;
    json_error_t error;
    int ii;

    dict = json_pack_ex(&error, 0,
        "{s:o, s:[]}",
        "MainFeature", json_integer(title_set->feature),
        "TitleList");
    // process title list
    json_t *title_list = json_object_get(dict, "TitleList");
    for (ii = 0; ii < hb_list_count(title_set->list_title); ii++)
    {
        hb_title_t *title = hb_list_item(title_set->list_title, ii);
        json_t *title_dict = hb_title_to_dict(title);
        json_array_append_new(title_list, title_dict);
    }

    return dict;
}

/**
 * Convert an hb_title_t to a json string
 * @param title - Pointer to hb_title_t to convert
 */
char* hb_title_to_json( const hb_title_t * title )
{
    json_t *dict = hb_title_to_dict(title);

    char *json_title = json_dumps(dict, JSON_INDENT(4)|JSON_PRESERVE_ORDER);
    json_decref(dict);

    return json_title;
}

/**
 * Get the current title set of an hb instance as a json string
 * @param h - Pointer to hb_handle_t hb instance
 */
char* hb_get_title_set_json( hb_handle_t * h )
{
    json_t *dict = hb_title_set_to_dict(hb_get_title_set(h));

    char *json_title_set = json_dumps(dict, JSON_INDENT(4)|JSON_PRESERVE_ORDER);
    json_decref(dict);

    return json_title_set;
}

/**
 * Convert an hb_job_t to a json string
 * @param job - Pointer to the hb_job_t to convert
 */
char* hb_job_to_json( const hb_job_t * job )
{
    json_t * dict;
    json_error_t error;
    int subtitle_search_burn;
    int ii;

    if (job == NULL || job->title == NULL)
        return NULL;

    // Assumes that the UI has reduced geometry settings to only the
    // necessary PAR value

    subtitle_search_burn = job->select_subtitle_config.dest == RENDERSUB;

    dict = json_pack_ex(&error, 0,
    "{"
    // SequenceID
    "s:o,"
    // Destination {Mux, ChapterMarkers, ChapterList}
    "s:{s:o, s:o, s[]},"
    // Source {Title, Angle}
    "s:{s:o, s:o,},"
    // PAR {Num, Den}
    "s:{s:o, s:o},"
    // Video {Codec}
    "s:{s:o},"
    // Audio {CopyMask, FallbackEncoder, AudioList []}
    "s:{s:o, s:o, s:[]},"
    // Subtitles {Search {Enable, Forced, Default, Burn}, SubtitleList []}
    "s:{s:{s:o, s:o, s:o, s:o}, s:[]},"
    // MetaData
    "s:{},"
    // Filters {Grayscale, FilterList []}
    "s:{s:o, s:[]}"
    "}",
        "SequenceID",           json_integer(job->sequence_id),
        "Destination",
            "Mux",              json_integer(job->mux),
            "ChapterMarkers",   json_boolean(job->chapter_markers),
            "ChapterList",
        "Source",
            "Title",            json_integer(job->title->index),
            "Angle",            json_integer(job->angle),
        "PAR",
            "Num",              json_integer(job->par.num),
            "Den",              json_integer(job->par.den),
        "Video",
            "Codec",            json_integer(job->vcodec),
        "Audio",
            "CopyMask",         json_integer(job->acodec_copy_mask),
            "FallbackEncoder",  json_integer(job->acodec_fallback),
            "AudioList",
        "Subtitle",
            "Search",
                "Enable",       json_boolean(job->indepth_scan),
                "Forced",       json_boolean(job->select_subtitle_config.force),
                "Default",      json_boolean(job->select_subtitle_config.default_track),
                "Burn",         json_boolean(subtitle_search_burn),
            "SubtitleList",
        "MetaData",
        "Filter",
            "Grayscale",        json_boolean(job->grayscale),
            "FilterList"
    );
    if (dict == NULL)
    {
        hb_error("json pack failure: %s", error.text);
        return NULL;
    }
    json_t *dest_dict = json_object_get(dict, "Destination");
    if (job->file != NULL)
    {
        json_object_set_new(dest_dict, "File", json_string(job->file));
    }
    if (job->mux & HB_MUX_MASK_MP4)
    {
        json_t *mp4_dict;
        mp4_dict = json_pack_ex(&error, 0, "{s:o, s:o, s:o}",
            "Mp4Optimize",      json_boolean(job->mp4_optimize),
            "LargeFileSize",    json_boolean(job->largeFileSize),
            "IpodAtom",         json_boolean(job->ipod_atom));
        json_object_set_new(dest_dict, "Mp4Options", mp4_dict);
    }
    json_t *source_dict = json_object_get(dict, "Source");
    json_t *range_dict;
    if (job->start_at_preview > 0)
    {
        range_dict = json_pack_ex(&error, 0, "{s:o, s:o, s:o}",
            "StartAtPreview",   json_integer(job->start_at_preview),
            "PtsToStop",        json_integer(job->pts_to_stop),
            "SeekPoints",       json_integer(job->seek_points));
    }
    else if (job->pts_to_start != 0)
    {
        range_dict = json_pack_ex(&error, 0, "{s:o, s:o}",
            "PtsToStart",   json_integer(job->pts_to_start),
            "PtsToStop",    json_integer(job->pts_to_stop));
    }
    else if (job->frame_to_start != 0)
    {
        range_dict = json_pack_ex(&error, 0, "{s:o, s:o}",
            "FrameToStart", json_integer(job->frame_to_start),
            "FrameToStop",  json_integer(job->frame_to_stop));
    }
    else
    {
        range_dict = json_pack_ex(&error, 0, "{s:o, s:o}",
            "ChapterStart", json_integer(job->chapter_start),
            "ChapterEnd",   json_integer(job->chapter_end));
    }
    json_object_set_new(source_dict, "Range", range_dict);

    json_t *video_dict = json_object_get(dict, "Video");
    if (job->color_matrix_code > 0)
    {
        json_object_set_new(video_dict, "ColorMatrixCode",
                            json_integer(job->color_matrix_code));
    }
    if (job->vquality >= 0)
    {
        json_object_set_new(video_dict, "Quality", json_real(job->vquality));
    }
    else
    {
        json_object_set_new(video_dict, "Bitrate", json_integer(job->vbitrate));
        json_object_set_new(video_dict, "Pass", json_integer(job->pass));
        json_object_set_new(video_dict, "Turbo",
                            json_boolean(job->fastfirstpass));
    }
    if (job->encoder_preset != NULL)
    {
        json_object_set_new(video_dict, "Preset",
                            json_string(job->encoder_preset));
    }
    if (job->encoder_tune != NULL)
    {
        json_object_set_new(video_dict, "Tune",
                            json_string(job->encoder_tune));
    }
    if (job->encoder_profile != NULL)
    {
        json_object_set_new(video_dict, "Profile",
                            json_string(job->encoder_profile));
    }
    if (job->encoder_level != NULL)
    {
        json_object_set_new(video_dict, "Level",
                            json_string(job->encoder_level));
    }
    if (job->encoder_options != NULL)
    {
        json_object_set_new(video_dict, "Options",
                            json_string(job->encoder_options));
    }
    json_t *meta_dict = json_object_get(dict, "MetaData");
    if (job->metadata->name != NULL)
    {
        json_object_set_new(meta_dict, "Name",
                            json_string(job->metadata->name));
    }
    if (job->metadata->artist != NULL)
    {
        json_object_set_new(meta_dict, "Artist",
                            json_string(job->metadata->artist));
    }
    if (job->metadata->composer != NULL)
    {
        json_object_set_new(meta_dict, "Composer",
                            json_string(job->metadata->composer));
    }
    if (job->metadata->comment != NULL)
    {
        json_object_set_new(meta_dict, "Comment",
                            json_string(job->metadata->comment));
    }
    if (job->metadata->genre != NULL)
    {
        json_object_set_new(meta_dict, "Genre",
                            json_string(job->metadata->genre));
    }
    if (job->metadata->album != NULL)
    {
        json_object_set_new(meta_dict, "Album",
                            json_string(job->metadata->album));
    }
    if (job->metadata->album_artist != NULL)
    {
        json_object_set_new(meta_dict, "AlbumArtist",
                            json_string(job->metadata->album_artist));
    }
    if (job->metadata->description != NULL)
    {
        json_object_set_new(meta_dict, "Description",
                            json_string(job->metadata->description));
    }
    if (job->metadata->long_description != NULL)
    {
        json_object_set_new(meta_dict, "LongDescription",
                            json_string(job->metadata->long_description));
    }

    // process chapter list
    json_t *chapter_list = json_object_get(dest_dict, "ChapterList");
    for (ii = 0; ii < hb_list_count(job->list_chapter); ii++)
    {
        json_t *chapter_dict;
        char *title = "";
        hb_chapter_t *chapter = hb_list_item(job->list_chapter, ii);
        if (chapter->title != NULL)
            title = chapter->title;

        chapter_dict = json_pack_ex(&error, 0, "{s:o}",
                                "Name", json_string(title));
        json_array_append_new(chapter_list, chapter_dict);
    }

    // process filter list
    json_t *filters_dict = json_object_get(dict, "Filter");
    json_t *filter_list = json_object_get(filters_dict, "FilterList");
    for (ii = 0; ii < hb_list_count(job->list_filter); ii++)
    {
        json_t *filter_dict;
        hb_filter_object_t *filter = hb_list_item(job->list_filter, ii);

        filter_dict = json_pack_ex(&error, 0, "{s:o}",
                                "ID", json_integer(filter->id));
        if (filter->settings != NULL)
        {
            json_object_set_new(filter_dict, "Settings",
                                json_string(filter->settings));
        }

        json_array_append_new(filter_list, filter_dict);
    }

    // process audio list
    json_t *audios_dict = json_object_get(dict, "Audio");
    json_t *audio_list = json_object_get(audios_dict, "AudioList");
    for (ii = 0; ii < hb_list_count(job->list_audio); ii++)
    {
        json_t *audio_dict;
        hb_audio_t *audio = hb_list_item(job->list_audio, ii);

        audio_dict = json_pack_ex(&error, 0,
            "{s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o}",
            "Track",                json_integer(audio->config.in.track),
            "Encoder",              json_integer(audio->config.out.codec),
            "Gain",                 json_real(audio->config.out.gain),
            "DRC",                  json_real(audio->config.out.dynamic_range_compression),
            "Mixdown",              json_integer(audio->config.out.mixdown),
            "NormalizeMixLevel",    json_boolean(audio->config.out.normalize_mix_level),
            "Samplerate",           json_integer(audio->config.out.samplerate),
            "Bitrate",              json_integer(audio->config.out.bitrate),
            "Quality",              json_real(audio->config.out.quality),
            "CompressionLevel",     json_real(audio->config.out.compression_level));
        if (audio->config.out.name != NULL)
        {
            json_object_set_new(audio_dict, "Name",
                                json_string(audio->config.out.name));
        }

        json_array_append_new(audio_list, audio_dict);
    }

    // process subtitle list
    json_t *subtitles_dict = json_object_get(dict, "Subtitle");
    json_t *subtitle_list = json_object_get(subtitles_dict, "SubtitleList");
    for (ii = 0; ii < hb_list_count(job->list_subtitle); ii++)
    {
        json_t *subtitle_dict;
        hb_subtitle_t *subtitle = hb_list_item(job->list_subtitle, ii);

        if (subtitle->source == SRTSUB)
        {
            subtitle_dict = json_pack_ex(&error, 0,
                "{s:o, s:o, s:o, s:{s:o, s:o, s:o}}",
                "Default",  json_boolean(subtitle->config.default_track),
                "Burn",     json_boolean(subtitle->config.dest == RENDERSUB),
                "Offset",   json_integer(subtitle->config.offset),
                "SRT",
                    "Filename", json_string(subtitle->config.src_filename),
                    "Language", json_string(subtitle->iso639_2),
                    "Codeset",  json_string(subtitle->config.src_codeset));
        }
        else
        {
            subtitle_dict = json_pack_ex(&error, 0,
            "{s:o, s:o, s:o, s:o, s:o, s:o}",
                "ID",       json_integer(subtitle->id),
                "Track",    json_integer(subtitle->track),
                "Default",  json_boolean(subtitle->config.default_track),
                "Force",    json_boolean(subtitle->config.force),
                "Burn",     json_boolean(subtitle->config.dest == RENDERSUB),
                "Offset",   json_integer(subtitle->config.offset));
        }
        json_array_append_new(subtitle_list, subtitle_dict);
    }

    char *json_job = json_dumps(dict, JSON_INDENT(4));
    json_decref(dict);

    return json_job;
}

// These functions exist only to perform type checking when using
// json_unpack_ex().
static double*      unpack_f(double *f)     { return f; }
static int*         unpack_i(int *i)        { return i; }
static json_int_t*  unpack_I(json_int_t *i) { return i; }
static int *        unpack_b(int *b)        { return b; }
static char**       unpack_s(char **s)      { return s; }
static json_t**     unpack_o(json_t** o)    { return o; }

/**
 * Convert a json string representation of a job to an hb_job_t
 * @param h        - Pointer to the hb_hanle_t hb instance which contains the
 *                   title that the job refers to.
 * @param json_job - Pointer to json string representation of a job
 */
hb_job_t* hb_json_to_job( hb_handle_t * h, const char * json_job )
{
    json_t * dict;
    hb_job_t * job;
    int result;
    json_error_t error;
    int titleindex;

    dict = json_loads(json_job, 0, NULL);

    result = json_unpack_ex(dict, &error, 0, "{s:{s:i}}",
                            "Source", "Title", unpack_i(&titleindex));
    if (result < 0)
    {
        hb_error("json unpack failure, failed to find title: %s", error.text);
        return NULL;
    }

    job = hb_job_init_by_index(h, titleindex);

    char *destfile = NULL;
    char *video_preset = NULL, *video_tune = NULL;
    char *video_profile = NULL, *video_level = NULL;
    char *video_options = NULL;
    int   subtitle_search_burn = 0;
    char *meta_name = NULL, *meta_artist = NULL, *meta_album_artist = NULL;
    char *meta_release = NULL, *meta_comment = NULL, *meta_genre = NULL;
    char *meta_composer = NULL, *meta_desc = NULL, *meta_long_desc = NULL;
    json_int_t pts_to_start = 0, pts_to_stop = 0;

    result = json_unpack_ex(dict, &error, 0,
    "{"
    "s:i,"
    // Destination {File, Mux, ChapterMarkers, Mp4Options {
    //                          Mp4Optimize, LargeFileSize, IpodAtom}
    "s:{s?s, s:i, s:b s?{s?b, s?b, s?b}},"
    // Source {Angle, Range {ChapterStart, ChapterEnd, PtsToStart, PtsToStop,
    //                  FrameToStart, FrameToStop, StartAtPreview, SeekPoints}
    "s:{s?i, s:{s?i, s?i, s?I, s?I, s?i, s?i, s?i, s?i}},"
    // PAR {Num, Den}
    "s?{s:i, s:i},"
    // Video {Codec, Quality, Bitrate, Preset, Tune, Profile, Level,
    //        Options, Pass, Turbo, ColorMatrixCode}
    "s:{s:i, s?f, s?i, s?s, s?s, s?s, s?s, s?s, s?i, s?b, s?i},"
    // Audio {CopyMask, FallbackEncoder}
    "s?{s?i, s?i},"
    // Subtitle {Search {Enable, Forced, Default, Burn}}
    "s?{s?{s:b, s?b, s?b, s?b}},"
    // MetaData {Name, Artist, Composer, AlbumArtist, ReleaseDate,
    //           Comment, Genre, Description, LongDescription}
    "s?{s?s, s?s, s?s, s?s, s?s, s?s, s?s, s?s, s?s},"
    // Filters {}
    "s?{s?b}"
    "}",
        "SequenceID",               unpack_i(&job->sequence_id),
        "Destination",
            "File",                 unpack_s(&destfile),
            "Mux",                  unpack_i(&job->mux),
            "ChapterMarkers",       unpack_b(&job->chapter_markers),
            "Mp4Options",
                "Mp4Optimize",      unpack_b(&job->mp4_optimize),
                "LargeFileSize",    unpack_b(&job->largeFileSize),
                "IpodAtom",         unpack_b(&job->ipod_atom),
        "Source",
            "Angle",                unpack_i(&job->angle),
            "Range",
                "ChapterStart",     unpack_i(&job->chapter_start),
                "ChapterEnd",       unpack_i(&job->chapter_end),
                "PtsToStart",       unpack_I(&pts_to_start),
                "PtsToStop",        unpack_I(&pts_to_stop),
                "FrameToStart",     unpack_i(&job->frame_to_start),
                "FrameToStop",      unpack_i(&job->frame_to_stop),
                "StartAtPreview",   unpack_i(&job->start_at_preview),
                "SeekPoints",       unpack_i(&job->seek_points),
        "PAR",
            "Num",                  unpack_i(&job->par.num),
            "Den",                  unpack_i(&job->par.den),
        "Video",
            "Codec",                unpack_i(&job->vcodec),
            "Quality",              unpack_f(&job->vquality),
            "Bitrate",              unpack_i(&job->vbitrate),
            "Preset",               unpack_s(&video_preset),
            "Tune",                 unpack_s(&video_tune),
            "Profile",              unpack_s(&video_profile),
            "Level",                unpack_s(&video_level),
            "Options",              unpack_s(&video_options),
            "Pass",                 unpack_i(&job->pass),
            "Turbo",                unpack_b(&job->fastfirstpass),
            "ColorMatrixCode",      unpack_i(&job->color_matrix_code),
        "Audio",
            "CopyMask",             unpack_i(&job->acodec_copy_mask),
            "FallbackEncoder",      unpack_i(&job->acodec_fallback),
        "Subtitle",
            "Search",
                "Enable",           unpack_b(&job->indepth_scan),
                "Forced",           unpack_b(&job->select_subtitle_config.force),
                "Default",          unpack_b(&job->select_subtitle_config.default_track),
                "Burn",             unpack_b(&subtitle_search_burn),
        "MetaData",
            "Name",                 unpack_s(&meta_name),
            "Artist",               unpack_s(&meta_artist),
            "Composer",             unpack_s(&meta_composer),
            "AlbumArtist",          unpack_s(&meta_album_artist),
            "ReleaseDate",          unpack_s(&meta_release),
            "Comment",              unpack_s(&meta_comment),
            "Genre",                unpack_s(&meta_genre),
            "Description",          unpack_s(&meta_desc),
            "LongDescription",      unpack_s(&meta_long_desc),
        "Filter",
            "Grayscale",            unpack_b(&job->grayscale)
    );
    if (result < 0)
    {
        hb_error("json unpack failure: %s", error.text);
        hb_job_close(&job);
        return NULL;
    }
    job->pts_to_start = pts_to_start;
    job->pts_to_stop = pts_to_stop;

    if (destfile != NULL && destfile[0] != 0)
    {
        hb_job_set_file(job, destfile);
    }

    hb_job_set_encoder_preset(job, video_preset);
    hb_job_set_encoder_tune(job, video_tune);
    hb_job_set_encoder_profile(job, video_profile);
    hb_job_set_encoder_level(job, video_level);
    hb_job_set_encoder_options(job, video_options);

    job->select_subtitle_config.dest = subtitle_search_burn ?
                                            RENDERSUB : PASSTHRUSUB;
    if (meta_name != NULL && meta_name[0] != 0)
    {
        hb_metadata_set_name(job->metadata, meta_name);
    }
    if (meta_artist != NULL && meta_artist[0] != 0)
    {
        hb_metadata_set_artist(job->metadata, meta_artist);
    }
    if (meta_composer != NULL && meta_composer[0] != 0)
    {
        hb_metadata_set_composer(job->metadata, meta_composer);
    }
    if (meta_album_artist != NULL && meta_album_artist[0] != 0)
    {
        hb_metadata_set_album_artist(job->metadata, meta_album_artist);
    }
    if (meta_release != NULL && meta_release[0] != 0)
    {
        hb_metadata_set_release_date(job->metadata, meta_release);
    }
    if (meta_comment != NULL && meta_comment[0] != 0)
    {
        hb_metadata_set_comment(job->metadata, meta_comment);
    }
    if (meta_genre != NULL && meta_genre[0] != 0)
    {
        hb_metadata_set_genre(job->metadata, meta_genre);
    }
    if (meta_desc != NULL && meta_desc[0] != 0)
    {
        hb_metadata_set_description(job->metadata, meta_desc);
    }
    if (meta_long_desc != NULL && meta_long_desc[0] != 0)
    {
        hb_metadata_set_long_description(job->metadata, meta_long_desc);
    }

    if (job->indepth_scan == 1)
    {
        job->pass = -1;
        hb_job_set_encoder_options(job, NULL);
    }
    // process chapter list
    json_t * chapter_list = NULL;
    result = json_unpack_ex(dict, &error, 0,
                            "{s:{s:o}}",
                            "Destination",
                                "ChapterList", unpack_o(&chapter_list));
    if (result < 0)
    {
        hb_error("json unpack failure: %s", error.text);
        hb_job_close(&job);
        return NULL;
    }
    if (json_is_array(chapter_list))
    {
        int ii;
        json_t *chapter_dict;
        json_array_foreach(chapter_list, ii, chapter_dict)
        {
            char *name = NULL;
            result = json_unpack_ex(chapter_dict, &error, 0,
                                    "{s:s}", "Name", unpack_s(&name));
            if (result < 0)
            {
                hb_error("json unpack failure: %s", error.text);
                hb_job_close(&job);
                return NULL;
            }
            if (name != NULL && name[0] != 0)
            {
                hb_chapter_t *chapter;
                chapter = hb_list_item(job->list_chapter, ii);
                if (chapter != NULL)
                {
                    hb_chapter_set_title(chapter, name);
                }
            }
        }
    }

    // process filter list
    json_t * filter_list = NULL;
    result = json_unpack_ex(dict, &error, 0,
                            "{s:{s:o}}",
                            "Filter", "FilterList", unpack_o(&filter_list));
    if (result < 0)
    {
        hb_error("json unpack failure: %s", error.text);
        hb_job_close(&job);
        return NULL;
    }
    if (json_is_array(filter_list))
    {
        int ii;
        json_t *filter_dict;
        json_array_foreach(filter_list, ii, filter_dict)
        {
            int filter_id = -1;
            char *filter_settings = NULL;
            result = json_unpack_ex(filter_dict, &error, 0, "{s:i, s?s}",
                                    "ID",       unpack_i(&filter_id),
                                    "Settings", unpack_s(&filter_settings));
            if (result < 0)
            {
                hb_error("json unpack failure: %s", error.text);
                hb_job_close(&job);
                return NULL;
            }
            if (filter_id >= HB_FILTER_FIRST && filter_id <= HB_FILTER_LAST)
            {
                hb_filter_object_t *filter;
                filter = hb_filter_init(filter_id);
                hb_add_filter(job, filter, filter_settings);
            }
        }
    }

    // process audio list
    json_t * audio_list = NULL;
    result = json_unpack_ex(dict, &error, 0, "{s:{s:o}}",
                            "Audio", "AudioList", unpack_o(&audio_list));
    if (result < 0)
    {
        hb_error("json unpack failure: %s", error.text);
        hb_job_close(&job);
        return NULL;
    }
    if (json_is_array(audio_list))
    {
        int ii;
        json_t *audio_dict;
        json_array_foreach(audio_list, ii, audio_dict)
        {
            hb_audio_config_t audio;

            hb_audio_config_init(&audio);
            result = json_unpack_ex(audio_dict, &error, 0,
                "{s:i, s?s, s?i, s?F, s?F, s?i, s?b, s?i, s?i, s?F, s?F}",
                "Track",                unpack_i(&audio.in.track),
                "Name",                 unpack_s(&audio.out.name),
                "Encoder",              unpack_i((int*)&audio.out.codec),
                "Gain",                 unpack_f(&audio.out.gain),
                "DRC",                  unpack_f(&audio.out.dynamic_range_compression),
                "Mixdown",              unpack_i(&audio.out.mixdown),
                "NormalizeMixLevel",    unpack_b(&audio.out.normalize_mix_level),
                "Samplerate",           unpack_i(&audio.out.samplerate),
                "Bitrate",              unpack_i(&audio.out.bitrate),
                "Quality",              unpack_f(&audio.out.quality),
                "CompressionLevel",     unpack_f(&audio.out.compression_level));
            if (result < 0)
            {
                hb_error("json unpack failure: %s", error.text);
                hb_job_close(&job);
                return NULL;
            }
            if (audio.in.track >= 0)
            {
                audio.out.track = ii;
                hb_audio_add(job, &audio);
            }
        }
    }

    // process subtitle list
    json_t * subtitle_list = NULL;
    result = json_unpack_ex(dict, &error, 0,
                            "{s:{s:o}}",
                            "Subtitle",
                                "SubtitleList", unpack_o(&subtitle_list));
    if (result < 0)
    {
        hb_error("json unpack failure: %s", error.text);
        hb_job_close(&job);
        return NULL;
    }
    if (json_is_array(subtitle_list))
    {
        int ii;
        json_t *subtitle_dict;
        json_array_foreach(subtitle_list, ii, subtitle_dict)
        {
            hb_subtitle_config_t sub_config;
            int track = -1;
            int burn = 0;
            char *srtfile = NULL;
            json_int_t offset = 0;

            result = json_unpack_ex(subtitle_dict, &error, 0,
                                    "{s?i, s?{s:s}}",
                                    "Track", unpack_i(&track),
                                    "SRT",
                                        "Filename", unpack_s(&srtfile));
            if (result < 0)
            {
                hb_error("json unpack failure: %s", error.text);
                hb_job_close(&job);
                return NULL;
            }
            // Embedded subtitle track
            if (track >= 0)
            {
                hb_subtitle_t *subtitle;
                subtitle = hb_list_item(job->title->list_subtitle, track);
                if (subtitle != NULL)
                {
                    sub_config = subtitle->config;
                    result = json_unpack_ex(subtitle_dict, &error, 0,
                        "{s?b, s?b, s?b, s?i}",
                        "Default",  unpack_i(&sub_config.default_track),
                        "Force",    unpack_b(&sub_config.force),
                        "Burn",     unpack_b(&burn),
                        "Offset",   unpack_I(&offset));
                    if (result < 0)
                    {
                        hb_error("json unpack failure: %s", error.text);
                        hb_job_close(&job);
                        return NULL;
                    }
                    sub_config.offset = offset;
                    sub_config.dest = burn ? RENDERSUB : PASSTHRUSUB;
                    hb_subtitle_add(job, &sub_config, track);
                }
            }
            else if (srtfile != NULL)
            {
                strncpy(sub_config.src_filename, srtfile, 255);
                sub_config.src_filename[255] = 0;

                char *srtlang = "und";
                char *srtcodeset = "UTF-8";
                result = json_unpack_ex(subtitle_dict, &error, 0,
                    "{s?b, s?b, s?i, "      // Common
                    "s?{s?s, s?s, s?s}}",   // SRT
                    "Default",  unpack_b(&sub_config.default_track),
                    "Burn",     unpack_b(&burn),
                    "Offset",   unpack_I(&offset),
                    "SRT",
                        "Filename", unpack_s(&srtfile),
                        "Language", unpack_s(&srtlang),
                        "Codeset",  unpack_s(&srtcodeset));
                    if (result < 0)
                    {
                        hb_error("json unpack failure: %s", error.text);
                        hb_job_close(&job);
                        return NULL;
                    }
                sub_config.offset = offset;
                sub_config.dest = burn ? RENDERSUB : PASSTHRUSUB;
                strncpy(sub_config.src_codeset, srtcodeset, 39);
                sub_config.src_filename[39] = 0;
                hb_srt_add(job, &sub_config, srtlang);
            }
        }
    }
    json_decref(dict);

    return job;
}

/**
 * Initialize an hb_job_t and return a json string representation of the job
 * @param h             - Pointer to hb_handle_t instance that contains the
 *                        specified title_index
 * @param title_index   - Index of hb_title_t to use for job initialization.
 *                        Index comes from title->index or "Index" key
 *                        in json representation of a title.
 */
char* hb_job_init_json(hb_handle_t *h, int title_index)
{
    hb_job_t *job = hb_job_init_by_index(h, title_index);
    char *json_job = hb_job_to_json(job);
    hb_job_close(&job);
    return json_job;
}

/**
 * Add a json string job to the hb queue
 * @param h         - Pointer to hb_handle_t instance that job is added to
 * @param json_job  - json string representation of job to add
 */
int hb_add_json( hb_handle_t * h, const char * json_job )
{
    hb_job_t *job = hb_json_to_job(h, json_job);
    if (job == NULL)
        return -1;

    hb_add(h, job);
    hb_job_close(&job);

    return 0;
}


/**
 * Calculates destination width and height for anamorphic content
 *
 * Returns geometry as json string {Width, Height, PAR {Num, Den}}
 * @param json_param - contains source and destination geometry params.
 *                     This encapsulates the values that are in
 *                     hb_geometry_t and hb_geometry_settings_t
 */
char* hb_set_anamorphic_size_json(const char * json_param)
{
    int json_result;
    json_error_t error;
    json_t * dict;
    hb_geometry_t geo_result;
    hb_geometry_t src;
    hb_geometry_settings_t ui_geo;

    // Clear dest geometry since some fields are optional.
    memset(&ui_geo, 0, sizeof(ui_geo));

    dict = json_loads(json_param, 0, NULL);
    json_result = json_unpack_ex(dict, &error, 0,
    "{"
    // SourceGeometry
    //  {Width, Height, PAR {Num, Den}}
    "s:{s:i, s:i, s:{s:i, s:i}},"
    // DestSettings
    "s:{"
    //   Geometry {Width, Height, PAR {Num, Den}},
    "s:{s:i, s:i, s:{s:i, s:i}},"
    //   AnamorphicMode, Keep, ItuPAR, Modulus, MaxWidth, MaxHeight,
    "s:i, s?i, s?b, s:i, s:i, s:i,"
    //   Crop [Top, Bottom, Left, Right]
    "s?[iiii]"
    "  }"
    "}",
    "SourceGeometry",
        "Width",                unpack_i(&src.width),
        "Height",               unpack_i(&src.height),
        "PAR",
            "Num",              unpack_i(&src.par.num),
            "Den",              unpack_i(&src.par.den),
    "DestSettings",
        "Geometry",
            "Width",            unpack_i(&ui_geo.geometry.width),
            "Height",           unpack_i(&ui_geo.geometry.height),
            "PAR",
                "Num",          unpack_i(&ui_geo.geometry.par.num),
                "Den",          unpack_i(&ui_geo.geometry.par.den),
        "AnamorphicMode",       unpack_i(&ui_geo.mode),
        "Keep",                 unpack_i(&ui_geo.keep),
        "ItuPAR",               unpack_b(&ui_geo.itu_par),
        "Modulus",              unpack_i(&ui_geo.modulus),
        "MaxWidth",             unpack_i(&ui_geo.maxWidth),
        "MaxHeight",            unpack_i(&ui_geo.maxHeight),
        "Crop",                 unpack_i(&ui_geo.crop[0]),
                                unpack_i(&ui_geo.crop[1]),
                                unpack_i(&ui_geo.crop[2]),
                                unpack_i(&ui_geo.crop[3])
    );
    json_decref(dict);

    if (json_result < 0)
    {
        hb_error("json unpack failure: %s", error.text);
        return NULL;
    }

    hb_set_anamorphic_size2(&src, &ui_geo, &geo_result);

    dict = json_pack_ex(&error, 0,
        "{s:o, s:o, s:{s:o, s:o}}",
            "Width",        json_integer(geo_result.width),
            "Height",       json_integer(geo_result.height),
            "PAR",
                "Num",      json_integer(geo_result.par.num),
                "Den",      json_integer(geo_result.par.den));
    if (dict == NULL)
    {
        hb_error("hb_set_anamorphic_size_json: pack failure: %s", error.text);
        return NULL;
    }
    char *result = json_dumps(dict, JSON_INDENT(4)|JSON_PRESERVE_ORDER);
    json_decref(dict);

    return result;
}

char* hb_get_preview_json(hb_handle_t * h, const char *json_param)
{
    hb_image_t *image;
    int ii, title_idx, preview_idx, deinterlace = 0;

    int json_result;
    json_error_t error;
    json_t * dict;
    hb_geometry_settings_t settings;

    // Clear dest geometry since some fields are optional.
    memset(&settings, 0, sizeof(settings));

    dict = json_loads(json_param, 0, NULL);
    json_result = json_unpack_ex(dict, &error, 0,
    "{"
    // Title, Preview, Deinterlace
    "s:i, s:i, s?b,"
    // DestSettings
    "s:{"
    //   Geometry {Width, Height, PAR {Num, Den}},
    "s:{s:i, s:i, s:{s:i, s:i}},"
    //   AnamorphicMode, Keep, ItuPAR, Modulus, MaxWidth, MaxHeight,
    "s:i, s?i, s?b, s:i, s:i, s:i,"
    //   Crop [Top, Bottom, Left, Right]
    "s?[iiii]"
    "  }"
    "}",
    "Title",                    unpack_i(&title_idx),
    "Preview",                  unpack_i(&preview_idx),
    "Deinterlace",              unpack_b(&deinterlace),
    "DestSettings",
        "Geometry",
            "Width",            unpack_i(&settings.geometry.width),
            "Height",           unpack_i(&settings.geometry.height),
            "PAR",
                "Num",          unpack_i(&settings.geometry.par.num),
                "Den",          unpack_i(&settings.geometry.par.den),
        "AnamorphicMode",       unpack_i(&settings.mode),
        "Keep",                 unpack_i(&settings.keep),
        "ItuPAR",               unpack_b(&settings.itu_par),
        "Modulus",              unpack_i(&settings.modulus),
        "MaxWidth",             unpack_i(&settings.maxWidth),
        "MaxHeight",            unpack_i(&settings.maxHeight),
        "Crop",                 unpack_i(&settings.crop[0]),
                                unpack_i(&settings.crop[1]),
                                unpack_i(&settings.crop[2]),
                                unpack_i(&settings.crop[3])
    );
    json_decref(dict);

    if (json_result < 0)
    {
        hb_error("json unpack failure: %s", error.text);
        return NULL;
    }

    image = hb_get_preview2(h, title_idx, preview_idx, &settings, deinterlace);

    dict = json_pack_ex(&error, 0,
        "{s:o, s:o, s:o}",
            "Format",       json_integer(image->width),
            "Width",        json_integer(image->width),
            "Height",       json_integer(image->height));
    if (dict == NULL)
    {
        hb_error("hb_get_preview_json: pack failure: %s", error.text);
        return NULL;
    }

    json_t * planes = json_array();
    for (ii = 0; ii < 4; ii++)
    {
        int base64size = AV_BASE64_SIZE(image->plane[ii].size);
        if (base64size < 0)
            continue;

        char *plane_base64 = calloc(base64size, 1);
        av_base64_encode(plane_base64, base64size,
                         image->plane[ii].data, image->plane[ii].size);

        json_t *plane_dict;
        plane_dict = json_pack_ex(&error, 0,
            "{s:o, s:o, s:o, s:o, s:o, s:o}",
            "Width",        json_integer(image->plane[ii].width),
            "Height",       json_integer(image->plane[ii].height),
            "Stride",       json_integer(image->plane[ii].stride),
            "HeightStride", json_integer(image->plane[ii].height_stride),
            "Size",         json_integer(base64size),
            "Data",         json_string(plane_base64)
        );
        if (plane_dict == NULL)
        {
            hb_error("plane_dict: json pack failure: %s", error.text);
            return NULL;
        }
        json_array_append_new(planes, plane_dict);
    }
    json_object_set_new(dict, "Planes", planes);
    hb_image_close(&image);

    char *result = json_dumps(dict, JSON_INDENT(4)|JSON_PRESERVE_ORDER);
    json_decref(dict);

    return result;
}

