/* json.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <jansson.h>
#include "handbrake/handbrake.h"
#include "handbrake/hb_json.h"
#include "libavutil/base64.h"
#include "handbrake/qsv_common.h"

/**
 * Convert an hb_state_t to a jansson dict
 * @param state - Pointer to hb_state_t to convert
 */
hb_dict_t* hb_state_to_dict( hb_state_t * state)
{
    const char * state_s;
    hb_dict_t *dict = NULL;
    json_error_t error;

    switch (state->state)
    {
    case HB_STATE_IDLE:
        state_s = "IDLE";
        break;
    case HB_STATE_SCANNING:
        state_s = "SCANNING";
        break;
    case HB_STATE_SCANDONE:
        state_s = "SCANDONE";
        break;
    case HB_STATE_WORKING:
        state_s = "WORKING";
        break;
    case HB_STATE_PAUSED:
        state_s = "PAUSED";
        break;
    case HB_STATE_SEARCHING:
        state_s = "SEARCHING";
        break;
    case HB_STATE_WORKDONE:
        state_s = "WORKDONE";
        break;
    case HB_STATE_MUXING:
        state_s = "MUXING";
        break;
    default:
        state_s = "UNKNOWN";
        break;
    }

    switch (state->state)
    {
    case HB_STATE_IDLE:
        dict = json_pack_ex(&error, 0, "{s:o}",
                    "State", hb_value_string(state_s));
        break;
    case HB_STATE_SCANNING:
    case HB_STATE_SCANDONE:
        dict = json_pack_ex(&error, 0,
            "{s:o, s{s:o, s:o, s:o, s:o, s:o, s:o}}",
            "State", hb_value_string(state_s),
            "Scanning",
                "SequenceID",   hb_value_int(state->sequence_id),
                "Progress",     hb_value_double(state->param.scanning.progress),
                "Preview",      hb_value_int(state->param.scanning.preview_cur),
                "PreviewCount", hb_value_int(state->param.scanning.preview_count),
                "Title",        hb_value_int(state->param.scanning.title_cur),
                "TitleCount",   hb_value_int(state->param.scanning.title_count));
        break;
    case HB_STATE_WORKING:
    case HB_STATE_PAUSED:
    case HB_STATE_SEARCHING:
        dict = json_pack_ex(&error, 0,
            "{s:o, s{s:o, s:o, s:o, s:o, s:o, s:o,"
                   " s:o, s:o, s:o, s:o, s:o, s:o}}",
            "State", hb_value_string(state_s),
            "Working",
                "Progress",     hb_value_double(state->param.working.progress),
                "PassID",       hb_value_int(state->param.working.pass_id),
                "Pass",         hb_value_int(state->param.working.pass),
                "PassCount",    hb_value_int(state->param.working.pass_count),
                "Rate",         hb_value_double(state->param.working.rate_cur),
                "RateAvg",      hb_value_double(state->param.working.rate_avg),
                "ETASeconds",   hb_value_int(state->param.working.eta_seconds),
                "Hours",        hb_value_int(state->param.working.hours),
                "Minutes",      hb_value_int(state->param.working.minutes),
                "Paused",       hb_value_int(state->param.working.paused),
                "Seconds",      hb_value_int(state->param.working.seconds),
                "SequenceID",   hb_value_int(state->sequence_id));
        break;
    case HB_STATE_WORKDONE:
        dict = json_pack_ex(&error, 0,
            "{s:o, s{s:o, s:o}}",
            "State", hb_value_string(state_s),
            "WorkDone",
                "SequenceID",   hb_value_int(state->sequence_id),
                "Error",        hb_value_int(state->param.working.error));
        break;
    case HB_STATE_MUXING:
        dict = json_pack_ex(&error, 0,
            "{s:o, s{s:o}}",
            "State", hb_value_string(state_s),
            "Muxing",
                "Progress", hb_value_double(state->param.muxing.progress));
        break;
    default:
        dict = json_pack_ex(&error, 0, "{s:o}",
                    "State", hb_value_string(state_s));
        hb_error("hb_state_to_dict: unrecognized state %d", state->state);
        break;
    }
    if (dict == NULL)
    {
        hb_error("hb_state_to_dict, json pack failure: %s", error.text);
    }
    return dict;
}

hb_dict_t * hb_version_dict()
{
    hb_dict_t * dict;
    json_error_t error;

    dict = json_pack_ex(&error, 0,
        "{s:o, s:o, s:o, s{s:o, s:o, s:o}, s:o, s:o, s:o, s:o, s:o}",
        "Name",          hb_value_string(HB_PROJECT_NAME),
        "Official",      hb_value_bool(HB_PROJECT_REPO_OFFICIAL),
        "Type",          hb_value_string(HB_PROJECT_REPO_TYPE),
        "Version",
            "Major",     hb_value_int(HB_PROJECT_VERSION_MAJOR),
            "Minor",     hb_value_int(HB_PROJECT_VERSION_MINOR),
            "Point",     hb_value_int(HB_PROJECT_VERSION_POINT),
        "VersionString", hb_value_string(HB_PROJECT_VERSION),
        "RepoHash",      hb_value_string(HB_PROJECT_REPO_HASH),
        "RepoDate",      hb_value_string(HB_PROJECT_REPO_DATE),
        "System",        hb_value_string(HB_PROJECT_HOST_SYSTEMF),
        "Arch",          hb_value_string(HB_PROJECT_HOST_ARCH));
    if (dict == NULL)
    {
        hb_error("hb_version_dict, json pack failure: %s", error.text);
        return NULL;
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
    hb_dict_t *dict = hb_state_to_dict(&state);

    char *json_state = hb_value_get_json(dict);
    hb_value_free(&dict);

    return json_state;
}

hb_dict_t * hb_audio_attributes_to_dict(uint32_t attributes)
{
    json_error_t error;
    hb_dict_t * dict;

    dict = json_pack_ex(&error, 0,
        "{s:o, s:o, s:o, s:o, s:o, s:o}",
        "Normal",           hb_value_bool(attributes & HB_AUDIO_ATTR_NORMAL),
        "VisuallyImpaired", hb_value_bool(attributes &
                                          HB_AUDIO_ATTR_VISUALLY_IMPAIRED),
        "Commentary",       hb_value_bool(attributes &
                                          HB_AUDIO_ATTR_COMMENTARY),
        "AltCommentary",    hb_value_bool(attributes &
                                          HB_AUDIO_ATTR_ALT_COMMENTARY),
        "Secondary",        hb_value_bool(attributes & HB_AUDIO_ATTR_SECONDARY),
        "Default",          hb_value_bool(attributes & HB_AUDIO_ATTR_DEFAULT));
    if (dict == NULL)
    {
        hb_error("hb_audio_attributes_to_dict, json pack failure: %s", error.text);
    }
    return dict;
}

hb_dict_t * hb_subtitle_attributes_to_dict(uint32_t attributes)
{
    json_error_t error;
    hb_dict_t * dict;

    dict = json_pack_ex(&error, 0,
        "{s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o}",
        "Normal",        hb_value_bool(attributes & HB_SUBTITLE_ATTR_NORMAL),
        "Large",         hb_value_bool(attributes & HB_SUBTITLE_ATTR_LARGE),
        "Children",      hb_value_bool(attributes & HB_SUBTITLE_ATTR_CHILDREN),
        "ClosedCaption", hb_value_bool(attributes & HB_SUBTITLE_ATTR_CC),
        "Forced",        hb_value_bool(attributes & HB_SUBTITLE_ATTR_FORCED),
        "Commentary",    hb_value_bool(attributes &
                                       HB_SUBTITLE_ATTR_COMMENTARY),
        "4By3",          hb_value_bool(attributes & HB_SUBTITLE_ATTR_4_3),
        "Wide",          hb_value_bool(attributes & HB_SUBTITLE_ATTR_WIDE),
        "Letterbox",     hb_value_bool(attributes & HB_SUBTITLE_ATTR_LETTERBOX),
        "PanScan",       hb_value_bool(attributes & HB_SUBTITLE_ATTR_PANSCAN),
        "Default",       hb_value_bool(attributes & HB_SUBTITLE_ATTR_DEFAULT));
    if (dict == NULL)
    {
        hb_error("hb_subtitle_attributes_to_dict, json pack failure: %s", error.text);
    }
    return dict;
}

static hb_dict_t* hb_title_to_dict_internal( hb_title_t *title )
{
    hb_dict_t *dict;
    json_error_t error;
    int ii;

    if (title == NULL)
        return NULL;

    int h_shift, v_shift;
    int chroma_available = hb_get_chroma_sub_sample(title->pix_fmt, &h_shift, &v_shift);
    char *chroma_subsampling = NULL;

    if (chroma_available == 0)
    {
        int h_value = 4 >> h_shift;
        int v_value = v_shift ? 0 : h_value;
        chroma_subsampling = hb_strdup_printf("4:%d:%d", h_value, v_value);
    }

    dict = json_pack_ex(&error, 0,
    "{"
        // Type, Path, Name, Index, KeepDuplicateTitles, Playlist, AngleCount
        "s:o, s:o, s:o, s:o, s:o, s:o, s:o,"
        // Duration {Ticks, Hours, Minutes, Seconds}
        "s:{s:o, s:o, s:o, s:o},"
        // Geometry {Width, Height, PAR {Num, Den},
        "s:{s:o, s:o, s:{s:o, s:o}},"
        // Crop[Top, Bottom, Left, Right]}
        "s:[oooo],"
        // LooseCrop[Top, Bottom, Left, Right]}
        "s:[oooo],"
        // Color {Format, Range, Primary, Transfer, Matrix, ChromaLocation, ChromaSubsampling, BitDepth}
        "s:{s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o},"
        // FrameRate {Num, Den}
        "s:{s:o, s:o},"
        // InterlaceDetected, VideoCodec
        "s:o, s:o,"
        // Metadata
        "s:o"
    "}",
    "Type",                 hb_value_int(title->type),
    "Path",                 hb_value_string(title->path),
    "Name",                 hb_value_string(title->name),
    "Index",                hb_value_int(title->index),
    "KeepDuplicateTitles",  hb_value_bool(title->keep_duplicate_titles),
    "Playlist",             hb_value_int(title->playlist),
    "AngleCount",           hb_value_int(title->angle_count),
    "Duration",
        "Ticks",            hb_value_int(title->duration),
        "Hours",            hb_value_int(title->hours),
        "Minutes",          hb_value_int(title->minutes),
        "Seconds",          hb_value_int(title->seconds),
    "Geometry",
        "Width",            hb_value_int(title->geometry.width),
        "Height",           hb_value_int(title->geometry.height),
        "PAR",
            "Num",          hb_value_int(title->geometry.par.num),
            "Den",          hb_value_int(title->geometry.par.den),
    "Crop",                 hb_value_int(title->crop[0]),
                            hb_value_int(title->crop[1]),
                            hb_value_int(title->crop[2]),
                            hb_value_int(title->crop[3]),
    "LooseCrop",            hb_value_int(title->loose_crop[0]),
                            hb_value_int(title->loose_crop[1]),
                            hb_value_int(title->loose_crop[2]),
                            hb_value_int(title->loose_crop[3]),
    "Color",
        "Format",           hb_value_int(title->pix_fmt),
        "Range",            hb_value_int(title->color_range),
        "Primary",          hb_value_int(title->color_prim),
        "Transfer",         hb_value_int(title->color_transfer),
        "Matrix",           hb_value_int(title->color_matrix),
        "ChromaLocation",   hb_value_int(title->chroma_location),
        "ChromaSubsampling", hb_value_string(chroma_subsampling ? chroma_subsampling : "unknown"),
        "BitDepth",          hb_value_int(hb_get_bit_depth(title->pix_fmt)),
    "FrameRate",
        "Num",              hb_value_int(title->vrate.num),
        "Den",              hb_value_int(title->vrate.den),
    "InterlaceDetected",    hb_value_bool(title->detected_interlacing),
    "VideoCodec",           hb_value_string(title->video_codec_name),
    "Metadata",             hb_value_dup(title->metadata->dict)
    );
    if (dict == NULL)
    {
        hb_error("hb_title_to_dict_internal, json pack failure: %s", error.text);
        free(chroma_subsampling);
        return NULL;
    }

    free(chroma_subsampling);

    // Mastering Display Color Volume metadata
    hb_dict_t *mastering_dict;
    if (title->mastering.has_primaries || title->mastering.has_luminance)
    {
        mastering_dict = json_pack_ex(&error, 0,
        "{"
        // DisplayPrimaries[3][2]
        "s:[[[ii],[ii]],[[ii],[ii]],[[ii],[ii]]],"
        // WhitePoint[2],
        "s:[[i,i],[i,i]],"
        // MinLuminance, MaxLuminance, HasPrimaries, HasLuminance
        "s:[i,i],s:[i,i],s:b,s:b"
        "}",
            "DisplayPrimaries", title->mastering.display_primaries[0][0].num,
                                title->mastering.display_primaries[0][0].den,
                                title->mastering.display_primaries[0][1].num,
                                title->mastering.display_primaries[0][1].den,
                                title->mastering.display_primaries[1][0].num,
                                title->mastering.display_primaries[1][0].den,
                                title->mastering.display_primaries[1][1].num,
                                title->mastering.display_primaries[1][1].den,
                                title->mastering.display_primaries[2][0].num,
                                title->mastering.display_primaries[2][0].den,
                                title->mastering.display_primaries[2][1].num,
                                title->mastering.display_primaries[2][1].den,
            "WhitePoint", title->mastering.white_point[0].num,
                          title->mastering.white_point[0].den,
                          title->mastering.white_point[1].num,
                          title->mastering.white_point[1].den,
            "MinLuminance", title->mastering.min_luminance.num,
                            title->mastering.min_luminance.den,
            "MaxLuminance", title->mastering.max_luminance.num,
                            title->mastering.max_luminance.den,
            "HasPrimaries", title->mastering.has_primaries,
            "HasLuminance", title->mastering.has_luminance
        );
        hb_dict_set(dict, "MasteringDisplayColorVolume", mastering_dict);
    }

    // Content Light Level metadata
    hb_dict_t *coll_dict;
    if (title->coll.max_cll && title->coll.max_fall)
    {
        coll_dict = json_pack_ex(&error, 0, "{s:i, s:i}",
            "MaxCLL",  title->coll.max_cll,
            "MaxFALL", title->coll.max_fall);
        hb_dict_set(dict, "ContentLightLevel", coll_dict);
    }

    // Dolby Vision Configuration Record
    hb_dict_t *dovi_dict;
    if (title->dovi.dv_profile)
    {
        dovi_dict = json_pack_ex(&error, 0, "{s:i, s:i, s:i, s:i, s:i, s:i, s:i, s:i}",
            "DVVersionMajor",          title->dovi.dv_version_major,
            "DVVersionMinor",          title->dovi.dv_version_minor,
            "DVProfile",               title->dovi.dv_profile,
            "DVLevel",                 title->dovi.dv_level,
            "RPUPresentFlag",          title->dovi.rpu_present_flag,
            "ELPresentFlag",           title->dovi.el_present_flag,
            "BLPresentFlag",           title->dovi.bl_present_flag,
            "BLSignalCompatibilityId", title->dovi.dv_bl_signal_compatibility_id);
        hb_dict_set(dict, "DolbyVisionConfigurationRecord", dovi_dict);
    }

    // HDR10+ Flag
    if (title->hdr_10_plus)
    {
        hb_dict_set(dict, "HDR10+", hb_value_int(title->hdr_10_plus));
    }

    if (title->container_name != NULL)
    {
        hb_dict_set(dict, "Container", hb_value_string(title->container_name));
    }

    // process chapter list
    hb_dict_t * chapter_list = hb_value_array_init();
    for (ii = 0; ii < hb_list_count(title->list_chapter); ii++)
    {
        hb_dict_t *chapter_dict;
        char *name = "";
        hb_chapter_t *chapter = hb_list_item(title->list_chapter, ii);
        if (chapter->title != NULL)
            name = chapter->title;

        chapter_dict = json_pack_ex(&error, 0,
            "{s:o, s:{s:o, s:o, s:o, s:o}}",
            "Name",         hb_value_string(name),
            "Duration",
                "Ticks",    hb_value_int(chapter->duration),
                "Hours",    hb_value_int(chapter->hours),
                "Minutes",  hb_value_int(chapter->minutes),
                "Seconds",  hb_value_int(chapter->seconds)
        );
        if (chapter_dict == NULL)
        {
            hb_error("hb_title_to_dict_internal, chapter, json pack failure: %s", error.text);
            return NULL;
        }
        hb_value_array_append(chapter_list, chapter_dict);
    }
    hb_dict_set(dict, "ChapterList", chapter_list);

    // process audio list
    hb_dict_t * audio_list = hb_value_array_init();
    for (ii = 0; ii < hb_list_count(title->list_audio); ii++)
    {
        const char * codec_name;
        char         channel_layout_name[64];
        int          channel_count, lfe_count;
        hb_dict_t  * audio_dict, * attributes;
        hb_audio_t * audio = hb_list_item(title->list_audio, ii);

        codec_name = hb_audio_decoder_get_name(audio->config.in.codec,
                                               audio->config.in.codec_param);
        hb_layout_get_name(channel_layout_name, sizeof(channel_layout_name),
                           audio->config.in.channel_layout);
        channel_count = hb_layout_get_discrete_channel_count(
                                     audio->config.in.channel_layout);
        lfe_count     = hb_layout_get_low_freq_channel_count(
                                     audio->config.in.channel_layout);


        attributes = hb_audio_attributes_to_dict(audio->config.lang.attributes);
        audio_dict = json_pack_ex(&error, 0,
        "{s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o}",
            "TrackNumber",       hb_value_int(ii + 1),
            "Description",       hb_value_string(audio->config.lang.description),
            "Language",          hb_value_string(audio->config.lang.simple),
            "LanguageCode",      hb_value_string(audio->config.lang.iso639_2),
            "Attributes",        attributes,
            "Codec",             hb_value_int(audio->config.in.codec),
            "CodecParam",        hb_value_int(audio->config.in.codec_param),
            "CodecName",         hb_value_string(codec_name),
            "SampleRate",        hb_value_int(audio->config.in.samplerate),
            "BitRate",           hb_value_int(audio->config.in.bitrate),
            "ChannelLayout",     hb_value_int(audio->config.in.channel_layout),
            "ChannelLayoutName", hb_value_string(channel_layout_name),
            "ChannelCount",      hb_value_int(channel_count),
            "LFECount",          hb_value_int(lfe_count));
        if (audio_dict == NULL)
        {
            hb_error("hb_title_to_dict_internal, audio, json pack failure: %s", error.text);
            return NULL;
        }
        if (audio->config.in.name != NULL)
        {
            hb_dict_set_string(audio_dict, "Name", audio->config.in.name);
        }
        hb_value_array_append(audio_list, audio_dict);
    }
    hb_dict_set(dict, "AudioList", audio_list);

    // process subtitle list
    hb_value_array_t * subtitle_list = hb_value_array_init();
    for (ii = 0; ii < hb_list_count(title->list_subtitle); ii++)
    {
        const char    * format;
        hb_dict_t     * subtitle_dict, * attributes;
        hb_subtitle_t * subtitle = hb_list_item(title->list_subtitle, ii);

        format = subtitle->format == PICTURESUB ? "bitmap" : "text";
        attributes = hb_subtitle_attributes_to_dict(subtitle->attributes);
        subtitle_dict = json_pack_ex(&error, 0,
            "{s:o, s:o, s:o, s:o, s:o, s:o, s:o}",
            "TrackNumber",  hb_value_int(ii + 1),
            "Format",       hb_value_string(format),
            "Source",       hb_value_int(subtitle->source),
            "SourceName",   hb_value_string(hb_subsource_name(subtitle->source)),
            "Attributes",   attributes,
            "Language",     hb_value_string(subtitle->lang),
            "LanguageCode", hb_value_string(subtitle->iso639_2));
        if (subtitle_dict == NULL)
        {
            hb_error("hb_title_to_dict_internal, subtitle, json pack failure: %s", error.text);
            return NULL;
        }
        if (subtitle->name != NULL)
        {
            hb_dict_set_string(subtitle_dict, "Name", subtitle->name);
        }
        hb_value_array_append(subtitle_list, subtitle_dict);
    }
    hb_dict_set(dict, "SubtitleList", subtitle_list);

    return dict;
}

/**
 * Convert an hb_title_t to a jansson dict
 * @param title - Pointer to the hb_title_t to convert
 */
hb_dict_t* hb_title_to_dict( hb_handle_t *h, int title_index )
{
    hb_title_t *title = hb_find_title_by_index(h, title_index);
    return hb_title_to_dict_internal(title);
}

/**
 * Convert an hb_title_set_t to a jansson dict
 * @param title - Pointer to the hb_title_set_t to convert
 */
hb_dict_t* hb_title_set_to_dict( const hb_title_set_t * title_set )
{
    hb_dict_t *dict;
    json_error_t error;
    int ii;

    dict = json_pack_ex(&error, 0,
        "{s:o, s:[]}",
        "MainFeature", hb_value_int(title_set->feature),
        "TitleList");
    // process title list
    hb_dict_t *title_list = hb_dict_get(dict, "TitleList");
    for (ii = 0; ii < hb_list_count(title_set->list_title); ii++)
    {
        hb_title_t *title = hb_list_item(title_set->list_title, ii);
        hb_dict_t *title_dict = hb_title_to_dict_internal(title);
        hb_value_array_append(title_list, title_dict);
    }

    return dict;
}

/**
 * Convert an hb_title_t to a json string
 * @param title - Pointer to hb_title_t to convert
 */
char* hb_title_to_json( hb_handle_t *h, int title_index )
{
    hb_dict_t *dict = hb_title_to_dict(h, title_index);
    if (dict == NULL)
        return NULL;

    char *json_title = hb_value_get_json(dict);
    hb_value_free(&dict);

    return json_title;
}

/**
 * Get the current title set of an hb instance as a json string
 * @param h - Pointer to hb_handle_t hb instance
 */
char* hb_get_title_set_json( hb_handle_t * h )
{
    hb_dict_t *dict = hb_title_set_to_dict(hb_get_title_set(h));

    char *json_title_set = hb_value_get_json(dict);
    hb_value_free(&dict);

    return json_title_set;
}

/**
 * Convert an hb_job_t to an hb_dict_t
 * @param job - Pointer to the hb_job_t to convert
 */
hb_dict_t* hb_job_to_dict( const hb_job_t * job )
{
    hb_dict_t * dict;
    json_error_t error;
    int subtitle_search_burn;
    int ii;
    int adapter_index = 0;

#if HB_PROJECT_FEATURE_QSV
    if (job->qsv.ctx){
        adapter_index = job->qsv.ctx->dx_index;
    }
#endif

    if (job == NULL || job->title == NULL)
        return NULL;

    // Assumes that the UI has reduced geometry settings to only the
    // necessary PAR value

    subtitle_search_burn = job->select_subtitle_config.dest == RENDERSUB;
    dict = json_pack_ex(&error, 0,
    "{"
    // SequenceID
    "s:o,"
    // Destination {Mux, InlineParameterSets, AlignAVStart,
    //              ChapterMarkers, ChapterList}
    "s:{s:o, s:o, s:o, s:o, s:[]},"
    // Source {Path, Title, Angle, HWDecode, KeepDuplicateTitles}
    "s:{s:o, s:o, s:o, s:o, s:o},"
    // PAR {Num, Den}
    "s:{s:o, s:o},"
    // Video {Encoder, HardwareDecode, QSV {Decode, AsyncDepth, AdapterIndex}}
    "s:{s:o, s:o, s:{s:o, s:o, s:o}},"
    // Audio {CopyMask, FallbackEncoder, AudioList []}
    "s:{s:[], s:o, s:[]},"
    // Subtitles {Search {Enable, Forced, Default, Burn}, SubtitleList []}
    "s:{s:{s:o, s:o, s:o, s:o}, s:[]},"
    // Metadata
    "s:o,"
    // Filters {FilterList []}
    "s:{s:[]}"
    "}",
        "SequenceID",           hb_value_int(job->sequence_id),
        "Destination",
            "Mux",              hb_value_int(job->mux),
            "InlineParameterSets", hb_value_bool(job->inline_parameter_sets),
            "AlignAVStart",     hb_value_bool(job->align_av_start),
            "ChapterMarkers",   hb_value_bool(job->chapter_markers),
            "ChapterList",
        "Source",
            "Path",             hb_value_string(job->title->path),
            "Title",            hb_value_int(job->title->index),
            "Angle",            hb_value_int(job->angle),
            "HWDecode",         hb_value_int(job->hw_decode),
            "KeepDuplicateTitles", hb_value_bool(job->keep_duplicate_titles),
        "PAR",
            "Num",              hb_value_int(job->par.num),
            "Den",              hb_value_int(job->par.den),
        "Video",
            "Encoder",          hb_value_int(job->vcodec),
            "HardwareDecode",   hb_value_int(job->hw_decode),
            "QSV",
                "Decode",       hb_value_bool(job->qsv.decode),
                "AsyncDepth",   hb_value_int(job->qsv.async_depth),
                "AdapterIndex", hb_value_int(adapter_index),
        "Audio",
            "CopyMask",
            "FallbackEncoder",  hb_value_int(job->acodec_fallback),
            "AudioList",
        "Subtitle",
            "Search",
                "Enable",       hb_value_bool(job->indepth_scan),
                "Forced",       hb_value_bool(job->select_subtitle_config.force),
                "Default",      hb_value_bool(job->select_subtitle_config.default_track),
                "Burn",         hb_value_bool(subtitle_search_burn),
            "SubtitleList",
        "Metadata",             hb_value_dup(job->metadata->dict),
        "Filters",
            "FilterList"
    );
    if (dict == NULL)
    {
        hb_error("hb_job_to_dict, json pack failure: %s", error.text);
        return NULL;
    }
    hb_dict_t *dest_dict = hb_dict_get(dict, "Destination");
    if (job->file != NULL)
    {
        hb_dict_set(dest_dict, "File", hb_value_string(job->file));
    }
    if (job->mux)
    {
        hb_dict_t *options_dict;
        options_dict = json_pack_ex(&error, 0, "{s:o, s:o}",
            "Optimize",         hb_value_bool(job->optimize),
            "IpodAtom",         hb_value_bool(job->ipod_atom));
        hb_dict_set(dest_dict, "Options", options_dict);
    }
    hb_dict_t *source_dict = hb_dict_get(dict, "Source");
    hb_dict_t *range_dict;
    if (job->start_at_preview > 0)
    {
        range_dict = json_pack_ex(&error, 0, "{s:o, s:o, s:o, s:o}",
            "Type",       hb_value_string("preview"),
            "Start",      hb_value_int(job->start_at_preview),
            "End",        hb_value_int(job->pts_to_stop),
            "SeekPoints", hb_value_int(job->seek_points));
    }
    else if (job->pts_to_start != 0 || job->pts_to_stop != 0)
    {
        range_dict = hb_dict_init();
        hb_dict_set(range_dict, "Type", hb_value_string("time"));
        if (job->pts_to_start > 0)
        {
            hb_dict_set(range_dict, "Start", hb_value_int(job->pts_to_start));
        }
        if (job->pts_to_stop > 0)
        {
            hb_dict_set(range_dict, "End",
                        hb_value_int(job->pts_to_start + job->pts_to_stop));
        }
    }
    else if (job->frame_to_start != 0 || job->frame_to_stop != 0)
    {
        range_dict = hb_dict_init();
        hb_dict_set(range_dict, "Type", hb_value_string("frame"));
        if (job->frame_to_start > 0)
        {
            hb_dict_set(range_dict, "Start",
                        hb_value_int(job->frame_to_start + 1));
        }
        if (job->frame_to_stop > 0)
        {
            hb_dict_set(range_dict, "End",
                        hb_value_int(job->frame_to_start + job->frame_to_stop));
        }
    }
    else
    {
        range_dict = json_pack_ex(&error, 0, "{s:o, s:o, s:o}",
            "Type",  hb_value_string("chapter"),
            "Start", hb_value_int(job->chapter_start),
            "End",   hb_value_int(job->chapter_end));
    }
    hb_dict_set(source_dict, "Range", range_dict);

    hb_dict_t *video_dict = hb_dict_get(dict, "Video");
    hb_dict_set(video_dict, "ColorInputFormat",
                hb_value_int(job->input_pix_fmt));
    hb_dict_set(video_dict, "ColorOutputFormat",
                hb_value_int(job->output_pix_fmt));
    hb_dict_set(video_dict, "ColorRange",
                hb_value_int(job->color_range));
    hb_dict_set(video_dict, "ColorPrimaries",
                hb_value_int(job->color_prim));
    hb_dict_set(video_dict, "ColorTransfer",
                hb_value_int(job->color_transfer));
    hb_dict_set(video_dict, "ColorMatrix",
                hb_value_int(job->color_matrix));
    hb_dict_set(video_dict, "ChromaLocation",
                hb_value_int(job->chroma_location));
    if (job->color_prim_override != HB_COLR_PRI_UNSET)
    {
        hb_dict_set(video_dict, "ColorPrimariesOverride",
                    hb_value_int(job->color_prim_override));
    }
    if (job->color_transfer_override != HB_COLR_TRA_UNSET)
    {
        hb_dict_set(video_dict, "ColorTransferOverride",
                    hb_value_int(job->color_transfer_override));
    }
    if (job->color_matrix_override != HB_COLR_MAT_UNSET)
    {
        hb_dict_set(video_dict, "ColorMatrixOverride",
                    hb_value_int(job->color_matrix_override));
    }

    // Mastering Display Color Volume metadata
    hb_dict_t *mastering_dict;
    if (job->mastering.has_primaries || job->mastering.has_luminance)
    {
        mastering_dict = json_pack_ex(&error, 0,
        "{"
        // DisplayPrimaries[3][2]
        "s:[[[ii],[ii]],[[ii],[ii]],[[ii],[ii]]],"
        // WhitePoint[2],
        "s:[[i,i],[i,i]],"
        // MinLuminance, MaxLuminance, HasPrimaries, HasLuminance
        "s:[i,i],s:[i,i],s:b,s:b"
        "}",
            "DisplayPrimaries", job->mastering.display_primaries[0][0].num,
                                job->mastering.display_primaries[0][0].den,
                                job->mastering.display_primaries[0][1].num,
                                job->mastering.display_primaries[0][1].den,
                                job->mastering.display_primaries[1][0].num,
                                job->mastering.display_primaries[1][0].den,
                                job->mastering.display_primaries[1][1].num,
                                job->mastering.display_primaries[1][1].den,
                                job->mastering.display_primaries[2][0].num,
                                job->mastering.display_primaries[2][0].den,
                                job->mastering.display_primaries[2][1].num,
                                job->mastering.display_primaries[2][1].den,
            "WhitePoint", job->mastering.white_point[0].num,
                          job->mastering.white_point[0].den,
                          job->mastering.white_point[1].num,
                          job->mastering.white_point[1].den,
            "MinLuminance", job->mastering.min_luminance.num,
                            job->mastering.min_luminance.den,
            "MaxLuminance", job->mastering.max_luminance.num,
                            job->mastering.max_luminance.den,
            "HasPrimaries", job->mastering.has_primaries,
            "HasLuminance", job->mastering.has_luminance
        );
        hb_dict_set(video_dict, "MasteringDisplayColorVolume", mastering_dict);
    }

    // Content Light Level metadata
    hb_dict_t *coll_dict;
    if (job->coll.max_cll && job->coll.max_fall)
    {
        coll_dict = json_pack_ex(&error, 0, "{s:i, s:i}",
            "MaxCLL",  job->coll.max_cll,
            "MaxFALL", job->coll.max_fall);
        hb_dict_set(video_dict, "ContentLightLevel", coll_dict);
    }

    // Dolby Vision Configuration Record
    hb_dict_t *dovi_dict;
    if (job->dovi.dv_profile)
    {
        dovi_dict = json_pack_ex(&error, 0, "{s:i, s:i, s:i, s:i, s:i, s:i, s:i, s:i}",
            "DVVersionMajor",          job->dovi.dv_version_major,
            "DVVersionMinor",          job->dovi.dv_version_minor,
            "DVProfile",               job->dovi.dv_profile,
            "DVLevel",                 job->dovi.dv_level,
            "RPUPresentFlag",          job->dovi.rpu_present_flag,
            "ELPresentFlag",           job->dovi.el_present_flag,
            "BLPresentFlag",           job->dovi.bl_present_flag,
            "BLSignalCompatibilityId", job->dovi.dv_bl_signal_compatibility_id);
        hb_dict_set(video_dict, "DolbyVisionConfigurationRecord", dovi_dict);
    }

    if (job->vquality > HB_INVALID_VIDEO_QUALITY)
    {
        hb_dict_set(video_dict, "Quality", hb_value_double(job->vquality));
    }
    else
    {
        hb_dict_set(video_dict, "Bitrate", hb_value_int(job->vbitrate));
        hb_dict_set(video_dict, "MultiPass", hb_value_bool(job->multipass));
        hb_dict_set(video_dict, "Turbo",
                            hb_value_bool(job->fastanalysispass));
    }
    if (job->encoder_preset != NULL)
    {
        hb_dict_set(video_dict, "Preset",
                            hb_value_string(job->encoder_preset));
    }
    if (job->encoder_tune != NULL)
    {
        hb_dict_set(video_dict, "Tune", hb_value_string(job->encoder_tune));
    }
    if (job->encoder_profile != NULL)
    {
        hb_dict_set(video_dict, "Profile",
                    hb_value_string(job->encoder_profile));
    }
    if (job->encoder_level != NULL)
    {
        hb_dict_set(video_dict, "Level", hb_value_string(job->encoder_level));
    }
    if (job->encoder_options != NULL)
    {
        hb_dict_set(video_dict, "Options",
                    hb_value_string(job->encoder_options));
    }

    // process chapter list
    hb_dict_t *chapter_list = hb_dict_get(dest_dict, "ChapterList");
    for (ii = 0; ii < hb_list_count(job->list_chapter); ii++)
    {
        hb_dict_t *chapter_dict;
        char *name = "";
        hb_chapter_t *chapter = hb_list_item(job->list_chapter, ii);
        if (chapter->title != NULL)
            name = chapter->title;

        chapter_dict = json_pack_ex(&error, 0,
            "{s:o, s:{s:o, s:o, s:o, s:o}}",
            "Name",         hb_value_string(name),
            "Duration",
                "Ticks",    hb_value_int(chapter->duration),
                "Hours",    hb_value_int(chapter->hours),
                "Minutes",  hb_value_int(chapter->minutes),
                "Seconds",  hb_value_int(chapter->seconds)
        );
        hb_value_array_append(chapter_list, chapter_dict);
    }

    // process filter list
    hb_dict_t *filters_dict = hb_dict_get(dict, "Filters");
    hb_value_array_t *filter_list = hb_dict_get(filters_dict, "FilterList");
    for (ii = 0; ii < hb_list_count(job->list_filter); ii++)
    {
        hb_dict_t *filter_dict;
        hb_filter_object_t *filter = hb_list_item(job->list_filter, ii);

        filter_dict = json_pack_ex(&error, 0, "{s:o}",
                                "ID", hb_value_int(filter->id));
        if (filter->settings != NULL)
        {
            hb_dict_set(filter_dict, "Settings",
                        hb_value_dup(filter->settings));
        }

        hb_value_array_append(filter_list, filter_dict);
    }

    hb_dict_t *audios_dict = hb_dict_get(dict, "Audio");
    // Construct audio CopyMask
    hb_value_array_t *copy_mask = hb_dict_get(audios_dict, "CopyMask");
    int acodec;
    for (acodec = 1; acodec != HB_ACODEC_PASS_FLAG; acodec <<= 1)
    {
        if (acodec & job->acodec_copy_mask)
        {
            const char *name;
            name = hb_audio_encoder_get_name(acodec | HB_ACODEC_PASS_FLAG);
            if (name != NULL)
            {
                hb_value_t *val = hb_value_string(name);
                hb_value_array_append(copy_mask, val);
            }
        }
    }
    // process audio list
    hb_dict_t *audio_list = hb_dict_get(audios_dict, "AudioList");
    for (ii = 0; ii < hb_list_count(job->list_audio); ii++)
    {
        hb_dict_t *audio_dict;
        hb_audio_t *audio = hb_list_item(job->list_audio, ii);

        audio_dict = json_pack_ex(&error, 0,
            "{s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o, s:o}",
            "Track",                hb_value_int(audio->config.index),
            "Encoder",              hb_value_int(audio->config.out.codec),
            "Gain",                 hb_value_double(audio->config.out.gain),
            "DRC",                  hb_value_double(audio->config.out.dynamic_range_compression),
            "Mixdown",              hb_value_int(audio->config.out.mixdown),
            "NormalizeMixLevel",    hb_value_bool(audio->config.out.normalize_mix_level),
            "DitherMethod",         hb_value_int(audio->config.out.dither_method),
            "Samplerate",           hb_value_int(audio->config.out.samplerate),
            "Bitrate",              hb_value_int(audio->config.out.bitrate),
            "Quality",              hb_value_double(audio->config.out.quality),
            "CompressionLevel",     hb_value_double(audio->config.out.compression_level));
        if (audio->config.out.name != NULL)
        {
            hb_dict_set_string(audio_dict, "Name", audio->config.out.name);
        }

        hb_value_array_append(audio_list, audio_dict);
    }

    // process subtitle list
    hb_dict_t *subtitles_dict = hb_dict_get(dict, "Subtitle");
    hb_dict_t *subtitle_list = hb_dict_get(subtitles_dict, "SubtitleList");
    for (ii = 0; ii < hb_list_count(job->list_subtitle); ii++)
    {
        hb_dict_t *subtitle_dict;
        hb_subtitle_t *subtitle = hb_list_item(job->list_subtitle, ii);

        if (subtitle->source == IMPORTSRT ||
            subtitle->source == IMPORTSSA)
        {
            subtitle_dict = json_pack_ex(&error, 0,
                "{s:o, s:o, s:o, s:{s:o, s:o, s:o}}",
                "Default",  hb_value_bool(subtitle->config.default_track),
                "Burn",     hb_value_bool(subtitle->config.dest == RENDERSUB),
                "Offset",   hb_value_int(subtitle->config.offset),
                "Import",
                    "Format",   hb_value_string(subtitle->source == IMPORTSRT ?
                                                "SRT" : "SSA"),
                    "Filename", hb_value_string(subtitle->config.src_filename),
                    "Language", hb_value_string(subtitle->iso639_2));
            if (subtitle->source == IMPORTSRT)
            {
                hb_dict_t *import_dict = hb_dict_get(subtitle_dict, "Import");
                hb_dict_set(import_dict, "Codeset",
                            hb_value_string(subtitle->config.src_codeset));
            }
        }
        else
        {
            subtitle_dict = json_pack_ex(&error, 0,
            "{s:o, s:o, s:o, s:o, s:o}",
                "Track",    hb_value_int(subtitle->track),
                "Default",  hb_value_bool(subtitle->config.default_track),
                "Forced",   hb_value_bool(subtitle->config.force),
                "Burn",     hb_value_bool(subtitle->config.dest == RENDERSUB),
                "Offset",   hb_value_int(subtitle->config.offset));
        }
        if (subtitle->config.name != NULL)
        {
            hb_dict_set_string(subtitle_dict, "Name", subtitle->config.name);
        }
        hb_value_array_append(subtitle_list, subtitle_dict);
    }

    return dict;
}

/**
 * Convert an hb_job_t to a json string
 * @param job - Pointer to the hb_job_t to convert
 */
char* hb_job_to_json( const hb_job_t * job )
{
    hb_dict_t *dict = hb_job_to_dict(job);

    if (dict == NULL)
        return NULL;

    char *json_job = hb_value_get_json(dict);
    hb_value_free(&dict);

    return json_job;
}

// These functions exist only to perform type checking when using
// json_unpack_ex().
typedef const char * const_str_t;

static double*      unpack_f(double *f)     { return f; }
static int*         unpack_i(int *i)        { return i; }
static unsigned*    unpack_u(unsigned *u)   { return u; }
static json_int_t*  unpack_I(json_int_t *i) { return i; }
static int *        unpack_b(int *b)        { return b; }
static const_str_t* unpack_s(const_str_t *s){ return s; }
static json_t**     unpack_o(json_t** o)    { return o; }

void hb_json_job_scan( hb_handle_t * h, const char * json_job )
{
    hb_dict_t * dict;
    int result;
    json_error_t error;

    dict = hb_value_json(json_job);

    int title_index, hw_decode, keep_duplicate_titles;
    const char *path = NULL;

    result = json_unpack_ex(dict, &error, 0, "{s:{s:s, s:i, s?i, s?b}}",
                            "Source",
                                "Path",     unpack_s(&path),
                                "Title",    unpack_i(&title_index),
                                "HWDecode", unpack_i(&hw_decode),
                                "KeepDuplicateTitles", unpack_b(&keep_duplicate_titles)
                           );
    if (result < 0)
    {
        hb_error("json unpack failure, failed to find title: %s", error.text);
        hb_value_free(&dict);
        return;
    }

    // If the job wants to use Hardware decode, it must also be
    // enabled during scan.  So enable it here.
    hb_list_t *file_paths = hb_list_init();
    hb_list_add(file_paths, (char *)path);
    hb_scan(h, file_paths, title_index, -1, 0, 0, 0, 0, 0, NULL, hw_decode, keep_duplicate_titles);
    hb_list_close(&file_paths);

    // Wait for scan to complete
    hb_state_t state;
    hb_get_state2(h, &state);
    while (state.state == HB_STATE_SCANNING)
    {
        hb_snooze(50);
        hb_get_state2(h, &state);
    }
    hb_value_free(&dict);
}

static int validate_audio_codec_mux(int codec, int mux, int track)
{
    const hb_encoder_t *enc = NULL;
    while ((enc = hb_audio_encoder_get_next(enc)) != NULL)
    {
        if ((enc->codec == codec) && (enc->muxers & mux) == 0)
        {
            if (codec != HB_ACODEC_NONE)
            {
                hb_error("track %d: incompatible encoder '%s' for muxer '%s'",
                         track + 1, enc->short_name,
                         hb_container_get_short_name(mux));
            }
            return -1;
        }
    }
    return 0;
}

/**
 * Convert a json string representation of a job to an hb_job_t
 * @param h        - Pointer to the hb_handle_t hb instance which contains the
 *                   title that the job refers to.
 * @param json_job - Pointer to json string representation of a job
 */
hb_job_t* hb_dict_to_job( hb_handle_t * h, hb_dict_t *dict )
{
    hb_job_t * job;
    int result;
    json_error_t error;
    int titleindex;

    if (dict == NULL)
        return NULL;

    result = json_unpack_ex(dict, &error, 0, "{s:{s:i}}",
                            "Source", "Title", unpack_i(&titleindex));
    if (result < 0)
    {
        hb_error("hb_dict_to_job: failed to find title: %s", error.text);
        return NULL;
    }

    job = hb_job_init_by_index(h, titleindex);
    if (job == NULL)
    {
        hb_error("hb_dict_to_job: Title %d doesn't exist", titleindex);
        return NULL;
    }

    hb_value_array_t * chapter_list = NULL;
    hb_value_array_t * audio_list = NULL;
    hb_value_array_t * subtitle_list = NULL;
    hb_value_array_t * filter_list = NULL;
    hb_value_t       * mux = NULL, * vcodec = NULL;
    hb_dict_t        * mastering_dict = NULL;
    hb_dict_t        * coll_dict = NULL;
    hb_dict_t        * dovi_dict = NULL;
    hb_value_t       * acodec_copy_mask = NULL, * acodec_fallback = NULL;
    const char       * destfile = NULL;
    const char       * range_type = NULL;
    const char       * video_preset = NULL, * video_tune = NULL;
    const char       * video_profile = NULL, * video_level = NULL;
    const char       * video_options = NULL;
    int                subtitle_search_burn = 0;
    json_int_t         range_start = -1, range_end = -1, range_seek_points = -1;
    int                vbitrate = -1;
    double             vquality = HB_INVALID_VIDEO_QUALITY;
    int                adapter_index = -1;
    hb_dict_t        * meta_dict = NULL;

    result = json_unpack_ex(dict, &error, 0,
    "{"
    // SequenceID
    "s:i,"
    // Destination {File, Mux, InlineParameterSets, AlignAVStart,
    //              ChapterMarkers, ChapterList,
    //              Options {Optimize, IpodAtom}}
    "s:{s?s, s:o, s?b, s?b, s:b, s?o s?{s?b, s?b}},"
    // Source {Angle, KeepDuplicateTitles, Range {Type, Start, End, SeekPoints}}
    "s:{s?i, s?b, s?{s:s, s?I, s?I, s?I}},"
    // PAR {Num, Den}
    "s?{s:i, s:i},"
    // Video {Codec, Quality, Bitrate, Preset, Tune, Profile, Level, Options
    //       MultiPass, Turbo,
    //       ColorInputFormat, ColorOutputFormat, ColorRange,
    //       ColorPrimaries, ColorTransfer, ColorMatrix, ChromaLocation,
    //       MasteringDisplayColorVolume,
    //       ContentLightLevel,
    //       DolbyVisionConfigurationRecord
    //       ColorPrimariesOverride, ColorTransferOverride, ColorMatrixOverride,
    //       HardwareDecode
    //       QSV {Decode, AsyncDepth, AdapterIndex}}
    "s:{s:o, s?F, s?i, s?s, s?s, s?s, s?s, s?s,"
    "   s?b, s?b,"
    "   s?i, s?i, s?i,"
    "   s?i, s?i, s?i, s?i,"
    "   s?o,"
    "   s?o,"
    "   s?o,"
    "   s?i, s?i, s?i,"
    "   s?i,"
    "   s?{s?b, s?i, s?i}},"
    // Audio {CopyMask, FallbackEncoder, AudioList}
    "s?{s?o, s?o, s?o},"
    // Subtitle {Search {Enable, Forced, Default, Burn}, SubtitleList}
    "s?{s?{s:b, s?b, s?b, s?b}, s?o},"
    // Metadata
    "s?o,"
    // Filters {FilterList}
    "s?{s?o}"
    "}",
        "SequenceID",               unpack_i(&job->sequence_id),
        "Destination",
            "File",                 unpack_s(&destfile),
            "Mux",                  unpack_o(&mux),
            "InlineParameterSets",  unpack_b(&job->inline_parameter_sets),
            "AlignAVStart",         unpack_b(&job->align_av_start),
            "ChapterMarkers",       unpack_b(&job->chapter_markers),
            "ChapterList",          unpack_o(&chapter_list),
            "Options",
                "Optimize",         unpack_b(&job->optimize),
                "IpodAtom",         unpack_b(&job->ipod_atom),
        "Source",
            "Angle",                unpack_i(&job->angle),
            "KeepDuplicateTitles",  unpack_b(&job->keep_duplicate_titles),
            "Range",
                "Type",             unpack_s(&range_type),
                "Start",            unpack_I(&range_start),
                "End",              unpack_I(&range_end),
                "SeekPoints",       unpack_I(&range_seek_points),
        "PAR",
            "Num",                  unpack_i(&job->par.num),
            "Den",                  unpack_i(&job->par.den),
        "Video",
            "Encoder",              unpack_o(&vcodec),
            "Quality",              unpack_f(&vquality),
            "Bitrate",              unpack_i(&vbitrate),
            "Preset",               unpack_s(&video_preset),
            "Tune",                 unpack_s(&video_tune),
            "Profile",              unpack_s(&video_profile),
            "Level",                unpack_s(&video_level),
            "Options",              unpack_s(&video_options),
            "MultiPass",            unpack_b(&job->multipass),
            "Turbo",                unpack_b(&job->fastanalysispass),
            "ColorInputFormat",     unpack_i(&job->input_pix_fmt),
            "ColorOutputFormat",    unpack_i(&job->output_pix_fmt),
            "ColorRange",           unpack_i(&job->color_range),
            "ColorPrimaries",       unpack_i(&job->color_prim),
            "ColorTransfer",        unpack_i(&job->color_transfer),
            "ColorMatrix",          unpack_i(&job->color_matrix),
            "ChromaLocation",       unpack_i(&job->chroma_location),
            "MasteringDisplayColorVolume", unpack_o(&mastering_dict),
            "ContentLightLevel",    unpack_o(&coll_dict),
            "DolbyVisionConfigurationRecord", unpack_o(&dovi_dict),
            "ColorPrimariesOverride", unpack_i(&job->color_prim_override),
            "ColorTransferOverride",  unpack_i(&job->color_transfer_override),
            "ColorMatrixOverride",    unpack_i(&job->color_matrix_override),
            "HardwareDecode",         unpack_i(&job->hw_decode),
            "QSV",
                "Decode",           unpack_b(&job->qsv.decode),
                "AsyncDepth",       unpack_i(&job->qsv.async_depth),
                "AdapterIndex",     unpack_i(&adapter_index),
        "Audio",
            "CopyMask",             unpack_o(&acodec_copy_mask),
            "FallbackEncoder",      unpack_o(&acodec_fallback),
            "AudioList",            unpack_o(&audio_list),
        "Subtitle",
            "Search",
                "Enable",           unpack_b(&job->indepth_scan),
                "Forced",           unpack_b(&job->select_subtitle_config.force),
                "Default",          unpack_b(&job->select_subtitle_config.default_track),
                "Burn",             unpack_b(&subtitle_search_burn),
            "SubtitleList",         unpack_o(&subtitle_list),
        "Metadata",                 unpack_o(&meta_dict),
        "Filters",
            "FilterList",           unpack_o(&filter_list)
    );
    if (result < 0)
    {
        hb_error("hb_dict_to_job: failed to parse dict: %s", error.text);
        goto fail;
    }
    if (meta_dict != NULL)
    {
        hb_value_free(&job->metadata->dict);
        job->metadata->dict = hb_value_dup(meta_dict);
    }
    // Lookup mux id
    if (hb_value_type(mux) == HB_VALUE_TYPE_STRING)
    {
        const char *s = hb_value_get_string(mux);
        job->mux = hb_container_get_from_name(s);
        if (job->mux == 0)
            job->mux = hb_container_get_from_extension(s);
    }
    else
    {
        job->mux = hb_value_get_int(mux);
    }

    // Lookup video codec
    if (hb_value_type(vcodec) == HB_VALUE_TYPE_STRING)
    {
        const char *s = hb_value_get_string(vcodec);
        job->vcodec = hb_video_encoder_get_from_name(s);
    }
    else
    {
        job->vcodec = hb_value_get_int(vcodec);
    }

    if (range_type != NULL)
    {
        if (!strcasecmp(range_type, "preview"))
        {
            if (range_start >= 0)
                job->start_at_preview = range_start;
            if (range_end >= 0)
                job->pts_to_stop = range_end;
            if (range_seek_points >= 0)
                job->seek_points = range_seek_points;
        }
        else if (!strcasecmp(range_type, "chapter"))
        {
            if (range_start >= 0)
                job->chapter_start = range_start;
            if (range_end >= 0)
                job->chapter_end = range_end;
        }
        else if (!strcasecmp(range_type, "time"))
        {
            if (range_start >= 0)
                job->pts_to_start = range_start;
            if (range_end >= 0)
                job->pts_to_stop = range_end - job->pts_to_start;
        }
        else if (!strcasecmp(range_type, "frame"))
        {
            if (range_start > 0)
                job->frame_to_start = range_start - 1;
            if (range_end > 0)
                job->frame_to_stop = range_end - job->frame_to_start;
        }
    }

    if (destfile != NULL && destfile[0] != 0)
    {
        hb_job_set_file(job, destfile);
    }

    hb_job_set_encoder_preset(job, video_preset);
    hb_job_set_encoder_tune(job, video_tune);
    hb_job_set_encoder_profile(job, video_profile);
    hb_job_set_encoder_level(job, video_level);
    hb_job_set_encoder_options(job, video_options);

#if HB_PROJECT_FEATURE_QSV
    if (job->qsv.ctx) {
        job->qsv.ctx->dx_index = adapter_index;
    }
#endif
    // If both vbitrate and vquality were specified, vbitrate is used;
    // we need to ensure the unused rate control mode is always set to an
    // invalid value, as if both values are valid, behavior is undefined
    // (some encoders first check for a valid vquality, whereas others
    //  check for a valid vbitrate instead)
    if (vbitrate > 0)
    {
        job->vbitrate = vbitrate;
        job->vquality = HB_INVALID_VIDEO_QUALITY;
    }
    else if (vquality > HB_INVALID_VIDEO_QUALITY)
    {
        job->vbitrate = -1;
        job->vquality = vquality;
    }
    // If neither were specified, defaults are used (set in job_setup())

    job->select_subtitle_config.dest = subtitle_search_burn ?
                                            RENDERSUB : PASSTHRUSUB;

    if (mastering_dict != NULL)
    {
        result = json_unpack_ex(mastering_dict, &error, 0,
        "{"
        // DisplayPrimaries[3][2]
        "s:[[[ii],[ii]],[[ii],[ii]],[[ii],[ii]]],"
        // WhitePoint[2],
        "s:[[i,i],[i,i]],"
        // MinLuminance, MaxLuminance, HasPrimaries, HasLuminance
        "s:[i,i],s:[i,i],s:b,s:b"
        "}",
            "DisplayPrimaries", unpack_i(&job->mastering.display_primaries[0][0].num),
                                unpack_i(&job->mastering.display_primaries[0][0].den),
                                unpack_i(&job->mastering.display_primaries[0][1].num),
                                unpack_i(&job->mastering.display_primaries[0][1].den),
                                unpack_i(&job->mastering.display_primaries[1][0].num),
                                unpack_i(&job->mastering.display_primaries[1][0].den),
                                unpack_i(&job->mastering.display_primaries[1][1].num),
                                unpack_i(&job->mastering.display_primaries[1][1].den),
                                unpack_i(&job->mastering.display_primaries[2][0].num),
                                unpack_i(&job->mastering.display_primaries[2][0].den),
                                unpack_i(&job->mastering.display_primaries[2][1].num),
                                unpack_i(&job->mastering.display_primaries[2][1].den),
            "WhitePoint", unpack_i(&job->mastering.white_point[0].num),
                          unpack_i(&job->mastering.white_point[0].den),
                          unpack_i(&job->mastering.white_point[1].num),
                          unpack_i(&job->mastering.white_point[1].den),
            "MinLuminance", unpack_i(&job->mastering.min_luminance.num),
                            unpack_i(&job->mastering.min_luminance.den),
            "MaxLuminance", unpack_i(&job->mastering.max_luminance.num),
                            unpack_i(&job->mastering.max_luminance.den),
            "HasPrimaries", unpack_b(&job->mastering.has_primaries),
            "HasLuminance", unpack_b(&job->mastering.has_luminance)
        );
        if (result < 0)
        {
            hb_error("hb_dict_to_job: failed to parse mastering_dict: %s", error.text);
            goto fail;
        }
    }

    if (coll_dict != NULL)
    {
        result = json_unpack_ex(coll_dict, &error, 0,
        // {MaxCLL, MaxFALL}
        "{s:i, s:i}",
            "MaxCLL",   unpack_u(&job->coll.max_cll),
            "MaxFALL",  unpack_u(&job->coll.max_fall)
        );
        if (result < 0)
        {
            hb_error("hb_dict_to_job: failed to parse coll_dict: %s", error.text);
            goto fail;
        }
    }

    if (dovi_dict != NULL)
    {
        result = json_unpack_ex(dovi_dict, &error, 0,
        "{s:i, s:i, s:i, s:i, s:i, s:i, s:i, s:i}",
            "DVVersionMajor",          unpack_u(&job->dovi.dv_version_major),
            "DVVersionMinor",          unpack_u(&job->dovi.dv_version_minor),
            "DVProfile",               unpack_u(&job->dovi.dv_profile),
            "DVLevel",                 unpack_u(&job->dovi.dv_level),
            "RPUPresentFlag",          unpack_u(&job->dovi.rpu_present_flag),
            "ELPresentFlag",           unpack_u(&job->dovi.el_present_flag),
            "BLPresentFlag",           unpack_u(&job->dovi.bl_present_flag),
            "BLSignalCompatibilityId", unpack_u(&job->dovi.dv_bl_signal_compatibility_id)
        );
        if (result < 0)
        {
            hb_error("hb_dict_to_job: failed to parse dovi_dict: %s", error.text);
            goto fail;
        }
    }

    // process chapter list
    if (chapter_list != NULL &&
        hb_value_type(chapter_list) == HB_VALUE_TYPE_ARRAY)
    {
        int ii, count;
        hb_dict_t *chapter_dict;
        count = hb_value_array_len(chapter_list);
        for (ii = 0; ii < count; ii++)
        {
            chapter_dict = hb_value_array_get(chapter_list, ii);
            const char *name = NULL;
            result = json_unpack_ex(chapter_dict, &error, 0,
                                    "{s:s}", "Name", unpack_s(&name));
            if (result < 0)
            {
                hb_error("hb_dict_to_job: failed to find chapter name: %s",
                         error.text);
                goto fail;
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
    if (filter_list != NULL &&
        hb_value_type(filter_list) == HB_VALUE_TYPE_ARRAY)
    {
        int ii, count;
        hb_dict_t *filter_dict;
        count = hb_value_array_len(filter_list);
        for (ii = 0; ii < count; ii++)
        {
            filter_dict = hb_value_array_get(filter_list, ii);
            int filter_id = -1;
            hb_value_t *filter_settings = NULL;
            result = json_unpack_ex(filter_dict, &error, 0, "{s:i, s?o}",
                                    "ID",       unpack_i(&filter_id),
                                    "Settings", unpack_o(&filter_settings));
            if (result < 0)
            {
                hb_error("hb_dict_to_job: failed to find filter settings: %s",
                         error.text);
                goto fail;
            }
            if (filter_id >= HB_FILTER_FIRST && filter_id <= HB_FILTER_LAST)
            {
                hb_filter_object_t *filter;
                filter = hb_filter_init(filter_id);
                hb_add_filter_dict(job, filter, filter_settings);
            }
        }
    }

    // process audio list
    if (acodec_fallback != NULL)
    {
        if (hb_value_type(acodec_fallback) == HB_VALUE_TYPE_STRING)
        {
            const char *s = hb_value_get_string(acodec_fallback);
            job->acodec_fallback = hb_audio_encoder_get_from_name(s);
        }
        else
        {
            job->acodec_fallback = hb_value_get_int(acodec_fallback);
        }
    }
    if (acodec_copy_mask != NULL)
    {
        if (hb_value_type(acodec_copy_mask) == HB_VALUE_TYPE_ARRAY)
        {
            int count, ii;
            count = hb_value_array_len(acodec_copy_mask);
            for (ii = 0; ii < count; ii++)
            {
                hb_value_t *value = hb_value_array_get(acodec_copy_mask, ii);
                if (hb_value_type(value) == HB_VALUE_TYPE_STRING)
                {
                    const char *s = hb_value_get_string(value);
                    job->acodec_copy_mask |= hb_audio_encoder_get_from_name(s);
                }
                else
                {
                    job->acodec_copy_mask |= hb_value_get_int(value);
                }
            }
        }
        else if (hb_value_type(acodec_copy_mask) == HB_VALUE_TYPE_STRING)
        {
            // Split the string at ','
            char *s = strdup(hb_value_get_string(acodec_copy_mask));
            char *cur = s;
            while (cur != NULL && cur[0] != 0)
            {
                char *next = strchr(cur, ',');
                if (next != NULL)
                {
                    *next = 0;
                    next++;
                }
                job->acodec_copy_mask |= hb_audio_encoder_get_from_name(cur);
                cur = next;
            }
            free(s);
        }
        else
        {
            job->acodec_copy_mask = hb_value_get_int(acodec_copy_mask);
        }
    }
    if (audio_list != NULL && hb_value_type(audio_list) == HB_VALUE_TYPE_ARRAY)
    {
        int ii, count;
        hb_dict_t *audio_dict;
        count = hb_value_array_len(audio_list);
        for (ii = 0; ii < count; ii++)
        {
            audio_dict = hb_value_array_get(audio_list, ii);
            hb_audio_config_t audio;
            hb_value_t *acodec = NULL, *samplerate = NULL, *mixdown = NULL;
            hb_value_t *dither = NULL;
            const char *name = NULL;

            hb_audio_config_init(&audio);
            result = json_unpack_ex(audio_dict, &error, 0,
                "{s:i, s?s, s?o, s?F, s?F, s?o, s?b, s?o, s?o, s?i, s?F, s?F}",
                "Track",                unpack_i(&audio.index),
                "Name",                 unpack_s(&name),
                "Encoder",              unpack_o(&acodec),
                "Gain",                 unpack_f(&audio.out.gain),
                "DRC",                  unpack_f(&audio.out.dynamic_range_compression),
                "Mixdown",              unpack_o(&mixdown),
                "NormalizeMixLevel",    unpack_b(&audio.out.normalize_mix_level),
                "DitherMethod",         unpack_o(&dither),
                "Samplerate",           unpack_o(&samplerate),
                "Bitrate",              unpack_i(&audio.out.bitrate),
                "Quality",              unpack_f(&audio.out.quality),
                "CompressionLevel",     unpack_f(&audio.out.compression_level));
            if (result < 0)
            {
                hb_error("hb_dict_to_job: failed to find audio settings: %s",
                         error.text);
                goto fail;
            }
            if (acodec != NULL)
            {
                if (hb_value_type(acodec) == HB_VALUE_TYPE_STRING)
                {
                    const char *s = hb_value_get_string(acodec);
                    audio.out.codec = hb_audio_encoder_get_from_name(s);
                }
                else
                {
                    audio.out.codec = hb_value_get_int(acodec);
                }
            }
            if (mixdown != NULL)
            {
                if (hb_value_type(mixdown) == HB_VALUE_TYPE_STRING)
                {
                    const char *s = hb_value_get_string(mixdown);
                    audio.out.mixdown = hb_mixdown_get_from_name(s);
                }
                else
                {
                    audio.out.mixdown = hb_value_get_int(mixdown);
                }
            }
            if (samplerate != NULL)
            {
                if (hb_value_type(samplerate) == HB_VALUE_TYPE_STRING)
                {
                    const char *s = hb_value_get_string(samplerate);
                    audio.out.samplerate = hb_audio_samplerate_get_from_name(s);
                    if (audio.out.samplerate < 0)
                        audio.out.samplerate = 0;
                }
                else
                {
                    audio.out.samplerate = hb_value_get_int(samplerate);
                }
            }
            if (dither != NULL)
            {
                if (hb_value_type(dither) == HB_VALUE_TYPE_STRING)
                {
                    const char *s = hb_value_get_string(dither);
                    audio.out.dither_method = hb_audio_dither_get_from_name(s);
                }
                else
                {
                    audio.out.dither_method = hb_value_get_int(dither);
                }
            }
            if (name != NULL)
            {
                audio.out.name = name;
            }
            if (audio.index >= 0)
            {
                audio.out.track = ii;
                hb_audio_add(job, &audio);
            }
        }
    }

    // Audio sanity checks
    int ii;
    for (ii = 0; ii < hb_list_count(job->list_audio); )
    {
        hb_audio_config_t *acfg;
        acfg = hb_list_audio_config_item(job->list_audio, ii);
        if (validate_audio_codec_mux(acfg->out.codec, job->mux, ii))
        {
            // drop the track
            hb_audio_t * audio = hb_list_item(job->list_audio, ii);
            hb_list_rem(job->list_audio, audio);
            hb_audio_close(&audio);
            continue;
        }
        ii++;
    }

    // process subtitle list
    if (subtitle_list != NULL &&
        hb_value_type(subtitle_list) == HB_VALUE_TYPE_ARRAY)
    {
        int ii, count;
        hb_dict_t *subtitle_dict;
        count = hb_value_array_len(subtitle_list);
        for (ii = 0; ii < count; ii++)
        {
            subtitle_dict = hb_value_array_get(subtitle_list, ii);
            hb_subtitle_config_t sub_config = {0};
            int track = -1;
            int burn = 0;
            const char *importfile = NULL;
            json_int_t offset = 0;
            const char *name = NULL;

            result = json_unpack_ex(subtitle_dict, &error, 0,
                                    "{s?i, s?s, s?{s:s}, s?{s:s}}",
                                    "Track", unpack_i(&track),
                                    "Name",  unpack_s(&name),
                                    // Support legacy "SRT" import
                                    "SRT",
                                        "Filename", unpack_s(&importfile),
                                    "Import",
                                        "Filename", unpack_s(&importfile));
            if (result < 0)
            {
                hb_error("json unpack failure: %s", error.text);
                hb_job_close(&job);
                return NULL;
            }

            // Embedded subtitle track
            if (track >= 0 && importfile == NULL)
            {
                hb_subtitle_t *subtitle;
                subtitle = hb_list_item(job->title->list_subtitle, track);
                if (subtitle != NULL)
                {
                    sub_config = subtitle->config;
                    if (name != NULL)
                    {
                        sub_config.name = name;
                    }
                    result = json_unpack_ex(subtitle_dict, &error, 0,
                        "{s?b, s?b, s?b, s?I}",
                        "Default",  unpack_b(&sub_config.default_track),
                        "Forced",   unpack_b(&sub_config.force),
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
            else if (importfile != NULL)
            {
                sub_config.src_filename = strdup(importfile);

                const char * lang = "und";
                const char * srtcodeset = "UTF-8";
                const char * format = "SRT";
                int          source = IMPORTSRT;
                result = json_unpack_ex(subtitle_dict, &error, 0,
                    "{s?b, s?b, s?I, "         // Common
                    "s?{s?s, s?s, s?s},"       // Legacy SRT settings
                    "s?{s?s, s?s, s?s, s?s}}", // Import settings
                    "Default",  unpack_b(&sub_config.default_track),
                    "Burn",     unpack_b(&burn),
                    "Offset",   unpack_I(&offset),
                    "SRT",
                        "Filename", unpack_s(&importfile),
                        "Language", unpack_s(&lang),
                        "Codeset",  unpack_s(&srtcodeset),
                    "Import",
                        "Format",   unpack_s(&format),
                        "Filename", unpack_s(&importfile),
                        "Language", unpack_s(&lang),
                        "Codeset",  unpack_s(&srtcodeset));
                if (result < 0)
                {
                    hb_error("json unpack failure: %s", error.text);
                    hb_job_close(&job);
                    return NULL;
                }
                if (name != NULL)
                {
                    sub_config.name = name;
                }
                sub_config.offset = offset;
                sub_config.dest = burn ? RENDERSUB : PASSTHRUSUB;
                strncpy(sub_config.src_codeset, srtcodeset, 39);
                sub_config.src_codeset[39] = 0;
                if (!strcasecmp(format, "SSA"))
                {
                    source = IMPORTSSA;
                }
                hb_import_subtitle_add(job, &sub_config, lang, source);
            }
        }
    }

    return job;

fail:
    hb_job_close(&job);
    return NULL;
}

hb_job_t* hb_json_to_job( hb_handle_t * h, const char * json_job )
{
    hb_dict_t *dict = hb_value_json(json_job);
    hb_job_t *job = hb_dict_to_job(h, dict);
    hb_value_free(&dict);
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

char* hb_preset_job_init_json(hb_handle_t *h, int title_index,
                              const char *json_preset)
{
    hb_dict_t * preset   = hb_value_json(json_preset);
    hb_dict_t * job      = hb_preset_job_init(h, title_index, preset);
    char      * json_job = hb_value_get_json(job);

    hb_value_free(&preset);
    hb_value_free(&job);

    return json_job;
}

/**
 * Add a json string job to the hb queue
 * @param h         - Pointer to hb_handle_t instance that job is added to
 * @param json_job  - json string representation of job to add
 */
int hb_add_json( hb_handle_t * h, const char * json_job )
{
    hb_job_t job;

    job.json = json_job;
    return hb_add(h, &job);
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
    hb_dict_t * dict;
    hb_geometry_t geo_result;
    hb_geometry_t src;
    hb_geometry_settings_t ui_geo;

    // Clear dest geometry since some fields are optional.
    memset(&ui_geo, 0, sizeof(ui_geo));

    dict = hb_value_json(json_param);
    json_result = json_unpack_ex(dict, &error, 0,
    "{"
    // SourceGeometry
    //  {Width, Height, PAR {Num, Den}}
    "s:{s:i, s:i, s:{s:i, s:i}},"
    // DestSettings
    "s:{"
    //   Geometry {Width, Height, PAR {Num, Den}},
    "s:{s:i, s:i, s:{s:i, s:i}},"
    //   AnamorphicMode, Flags, Keep, ItuPAR, Modulus, MaxWidth, MaxHeight,
    //   DisplayWidth, DisplayHeight
    "s:i, s?i, s?i, s?b, s?i, s?i, s?i, s?i, s?i"
    //   Crop [Top, Bottom, Left, Right]
    "s?[iiii]"
    //   Pad [Top, Bottom, Left, Right]
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
        "Flags",                unpack_i(&ui_geo.flags),
        "Keep",                 unpack_i(&ui_geo.keep),
        "ItuPAR",               unpack_b(&ui_geo.itu_par),
        "Modulus",              unpack_i(&ui_geo.modulus),
        "MaxWidth",             unpack_i(&ui_geo.maxWidth),
        "MaxHeight",            unpack_i(&ui_geo.maxHeight),
        "DisplayWidth",         unpack_i(&ui_geo.displayWidth),
        "DisplayHeight",        unpack_i(&ui_geo.displayHeight),
        "Crop",                 unpack_i(&ui_geo.crop[0]),
                                unpack_i(&ui_geo.crop[1]),
                                unpack_i(&ui_geo.crop[2]),
                                unpack_i(&ui_geo.crop[3]),
        "Pad",                  unpack_i(&ui_geo.pad[0]),
                                unpack_i(&ui_geo.pad[1]),
                                unpack_i(&ui_geo.pad[2]),
                                unpack_i(&ui_geo.pad[3])
    );
    hb_value_free(&dict);

    if (json_result < 0)
    {
        hb_error("json unpack failure: %s", error.text);
        return NULL;
    }

    hb_set_anamorphic_size2(&src, &ui_geo, &geo_result);

    dict = json_pack_ex(&error, 0,
        "{s:o, s:o, s:{s:o, s:o}}",
            "Width",        hb_value_int(geo_result.width),
            "Height",       hb_value_int(geo_result.height),
            "PAR",
                "Num",      hb_value_int(geo_result.par.num),
                "Den",      hb_value_int(geo_result.par.den));
    if (dict == NULL)
    {
        hb_error("hb_set_anamorphic_size_json: pack failure: %s", error.text);
        return NULL;
    }
    char *result = hb_value_get_json(dict);
    hb_value_free(&dict);

    return result;
}

hb_image_t * hb_get_preview3_json(hb_handle_t * h, int picture, const char *json_job)
{
    hb_image_t * image;
    hb_dict_t  * job_dict;

    job_dict = hb_value_json(json_job);
    image = hb_get_preview3(h, picture, job_dict);
    hb_value_free(&job_dict);

    return image;
}

char* hb_get_preview_params_json(int title_idx, int preview_idx,
                            int deinterlace, hb_geometry_settings_t *settings)
{
    json_error_t error;
    hb_dict_t * dict;

    dict = json_pack_ex(&error, 0,
        "{"
        "s:o, s:o, s:o,"
        "s:{"
        "   s:{s:o, s:o, s:{s:o, s:o}},"
        "   s:o, s:o, s:o, s:o, s:o, s:o"
        "   s:[oooo]"
        "  }"
        "}",
        "Title",                hb_value_int(title_idx),
        "Preview",              hb_value_int(preview_idx),
        "Deinterlace",          hb_value_bool(deinterlace),
        "DestSettings",
            "Geometry",
                "Width",        hb_value_int(settings->geometry.width),
                "Height",       hb_value_int(settings->geometry.height),
                "PAR",
                    "Num",      hb_value_int(settings->geometry.par.num),
                    "Den",      hb_value_int(settings->geometry.par.den),
            "AnamorphicMode",   hb_value_int(settings->mode),
            "Keep",             hb_value_int(settings->keep),
            "ItuPAR",           hb_value_bool(settings->itu_par),
            "Modulus",          hb_value_int(settings->modulus),
            "MaxWidth",         hb_value_int(settings->maxWidth),
            "MaxHeight",        hb_value_int(settings->maxHeight),
            "Crop",             hb_value_int(settings->crop[0]),
                                hb_value_int(settings->crop[1]),
                                hb_value_int(settings->crop[2]),
                                hb_value_int(settings->crop[3])
    );
    if (dict == NULL)
    {
        hb_error("hb_get_preview_params_json: pack failure: %s", error.text);
        return NULL;
    }

    char *result = hb_value_get_json(dict);
    hb_value_free(&dict);

    return result;
}

hb_image_t* hb_json_to_image(char *json_image)
{
    int json_result;
    json_error_t error;
    hb_dict_t * dict;
    int pix_fmt, width, height;
    dict = hb_value_json(json_image);
    json_result = json_unpack_ex(dict, &error, 0,
        "{"
        // Format, Width, Height
        "s:i, s:i, s:i,"
        "}",
        "Format",                   unpack_i(&pix_fmt),
        "Width",                    unpack_i(&width),
        "Height",                   unpack_b(&height)
    );
    if (json_result < 0)
    {
        hb_error("image: json unpack failure: %s", error.text);
        hb_value_free(&dict);
        return NULL;
    }

    hb_image_t *image = hb_image_init(pix_fmt, width, height);
    if (image == NULL)
    {
        hb_value_free(&dict);
        return NULL;
    }

    hb_value_array_t * planes = NULL;
    json_result = json_unpack_ex(dict, &error, 0,
                                 "{s:o}", "Planes", unpack_o(&planes));
    if (json_result < 0)
    {
        hb_error("image::planes: json unpack failure: %s", error.text);
        hb_value_free(&dict);
        return image;
    }
    if (hb_value_type(planes) == HB_VALUE_TYPE_ARRAY)
    {
        int ii, count;
        hb_dict_t *plane_dict;
        count = hb_value_array_len(planes);
        for (ii = 0; ii < count; ii++)
        {
            plane_dict = hb_value_array_get(planes, ii);
            const char *data = NULL;
            int size;
            json_result = json_unpack_ex(plane_dict, &error, 0,
                                         "{s:i, s:s}",
                                         "Size", unpack_i(&size),
                                         "Data", unpack_s(&data));
            if (json_result < 0)
            {
                hb_error("image::plane::data: json unpack failure: %s", error.text);
                hb_value_free(&dict);
                return image;
            }
            if (image->plane[ii].size > 0 && data != NULL)
            {
                av_base64_decode(image->plane[ii].data, data,
                                 image->plane[ii].size);
            }
        }
    }
    hb_value_free(&dict);

    return image;
}
