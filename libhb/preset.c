/* preset.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/preset_builtin.h"
#include "handbrake/handbrake.h"
#include "handbrake/hb_dict.h"
#include "handbrake/plist.h"
#include "handbrake/lang.h"

#if defined(SYS_LINUX)
#define HB_PRESET_PLIST_FILE    "ghb/presets"
#define HB_PRESET_JSON_FILE     "ghb/presets.json"
#elif defined(SYS_MINGW)
#define HB_PRESET_PLIST_FILE    "HandBrake\\presets.xml"
#define HB_PRESET_JSON_FILE     "HandBrake\\presets.json"
#elif defined(SYS_DARWIN)
#define HB_PRESET_PLIST_FILE    "HandBrake/UserPresets.plist"
#define HB_PRESET_JSON_FILE     "HandBrake/UserPresets.json"
#endif

int hb_preset_version_major;
int hb_preset_version_minor;
int hb_preset_version_micro;

static hb_value_t *hb_preset_template = NULL;
static hb_value_t *hb_presets = NULL;
static hb_value_t *hb_presets_builtin = NULL;
static hb_value_t *hb_presets_cli_default = NULL;

static void         preset_clean(hb_value_t *preset, hb_value_t *template);
static int          preset_import(hb_value_t *preset, int major, int minor,
                                  int micro);
static hb_value_t * presets_package(const hb_value_t *presets);
static int hb_presets_add_internal(hb_value_t *);

enum
{
    PRESET_DO_SUCCESS,
    PRESET_DO_FAIL,
    PRESET_DO_PARTIAL,
    PRESET_DO_NEXT,
    PRESET_DO_SKIP,
    PRESET_DO_SKIP_LEVEL,
    PRESET_DO_DELETE,
    PRESET_DO_DONE
};

typedef struct
{
    hb_preset_index_t path;
} preset_do_context_t;

typedef struct
{
    preset_do_context_t  do_ctx;
    hb_value_t          *template;
} preset_clean_context_t;

typedef struct
{
    preset_do_context_t  do_ctx;
    int                  major;
    int                  minor;
    int                  micro;
    int                  result;
} preset_import_context_t;

typedef struct
{
    preset_do_context_t  do_ctx;
    const char          *name;
    int                  type;
    int                  recurse;
    int                  last_match_idx;
} preset_search_context_t;

typedef int (*preset_do_f)(hb_value_t *preset, preset_do_context_t *ctx);

static int preset_cmp_idx(hb_value_t *preset, int idx,
                          const char *name, int type)
{
    const char *next, *preset_name;
    int  ii, len;

    if (type != HB_PRESET_TYPE_ALL &&
        type != hb_value_get_int(hb_dict_get(preset, "Type")))
    {
        return PRESET_DO_SKIP;
    }

    // Strip leading '/'
    if (name[0] == '/')
        name++;

    // Find the part of the "name" path we want to match.
    for (ii = 0; ii < idx; ii++)
    {
        next = strchr(name, '/');
        if (next == NULL)
            return PRESET_DO_SKIP_LEVEL;
        next++;
        name = next;
    }

    // Find the end of the part we want to match
    next = strchr(name, '/');
    if (next != NULL)
        len = next - name;
    else
        len = strlen(name);
    if (len <= 0)
        return PRESET_DO_SKIP_LEVEL;

    preset_name = hb_value_get_string(hb_dict_get(preset, "PresetName"));
    if (strlen(preset_name) > len)
        len = strlen(preset_name);

    // If match found and it's the last component of the "name", success!
    if (!strncmp(name, preset_name, len))
    {
        if (name[len] == 0)
            return PRESET_DO_SUCCESS;
        else
            return PRESET_DO_PARTIAL;
    }
    return PRESET_DO_SKIP;
}

static int do_preset_search(hb_value_t *preset, preset_do_context_t *do_ctx)
{
    preset_search_context_t *ctx = (preset_search_context_t*)do_ctx;
    int idx, result;

    idx = ctx->do_ctx.path.depth - 1;
    if (ctx->last_match_idx >= 0 && idx > ctx->last_match_idx)
    {
        // If there was a previous partial match, try to continue the match
        idx -= ctx->last_match_idx;
    }

    result = preset_cmp_idx(preset, idx, ctx->name, ctx->type);
    if (ctx->recurse && result == PRESET_DO_SKIP_LEVEL)
    {
        result = preset_cmp_idx(preset, 0, ctx->name, ctx->type);
        ctx->last_match_idx = idx;
    }
    if (result == PRESET_DO_PARTIAL)
    {
        return PRESET_DO_NEXT;
    }
    else
    {
        ctx->last_match_idx = -1;
    }
    if (ctx->recurse && result == PRESET_DO_SKIP)
    {
        return PRESET_DO_NEXT;
    }

    return result;
}

static int do_preset_import(hb_value_t *preset, preset_do_context_t *do_ctx)
{
    preset_import_context_t *ctx = (preset_import_context_t*)do_ctx;
    ctx->result |= preset_import(preset, ctx->major, ctx->minor, ctx->micro);
    return PRESET_DO_NEXT;
}

static int do_preset_clean(hb_value_t *preset, preset_do_context_t *do_ctx)
{
    preset_clean_context_t *ctx = (preset_clean_context_t*)do_ctx;
    preset_clean(preset, ctx->template);
    return PRESET_DO_NEXT;
}

static int do_make_custom(hb_value_t *preset, preset_do_context_t *ctx)
{
    hb_dict_set_int(preset, "Type", HB_PRESET_TYPE_CUSTOM);
    return PRESET_DO_NEXT;
}

static int do_delete_builtin(hb_value_t *preset, preset_do_context_t *ctx)
{
    if (hb_value_get_int(hb_dict_get(preset, "Type")) == HB_PRESET_TYPE_OFFICIAL)
        return PRESET_DO_DELETE;
    return PRESET_DO_NEXT;
}

static int do_clear_default(hb_value_t *preset, preset_do_context_t *ctx)
{
    hb_dict_set(preset, "Default", hb_value_bool(0));
    return PRESET_DO_NEXT;
}

static int do_find_default(hb_value_t *preset, preset_do_context_t *ctx)
{
    if (!hb_value_get_bool(hb_dict_get(preset, "Folder")) &&
        hb_value_get_bool(hb_dict_get(preset, "Default")))
    {
        return PRESET_DO_SUCCESS;
    }
    return PRESET_DO_NEXT;
}

static int presets_do(preset_do_f do_func, hb_value_t *preset,
                      preset_do_context_t *ctx)
{
    int result;
    hb_value_t *next;

    if (hb_value_type(preset) == HB_VALUE_TYPE_ARRAY)
    {
        // An array of presets, clean each one
        int ii;

        for (ii = 0; ii < hb_value_array_len(preset); )
        {
            ctx->path.index[ctx->path.depth-1] = ii;
            next = hb_value_array_get(preset, ii);
            result = presets_do(do_func, next, ctx);
            if (result == PRESET_DO_DELETE)
            {
                hb_value_array_remove(preset, ii);
                continue;
            }
            ii++;
            if (result == PRESET_DO_SKIP_LEVEL)
                return PRESET_DO_NEXT;
            if (result != PRESET_DO_NEXT && result != PRESET_DO_SKIP)
                return result;
        }
        return PRESET_DO_NEXT;
    }
    else if (hb_value_type(preset) == HB_VALUE_TYPE_DICT &&
             hb_dict_get(preset, "VersionMajor") != NULL)
    {
        // A packaged preset list
        next = hb_dict_get(preset, "PresetList");
        return presets_do(do_func, next, ctx);
    }
    else if (hb_value_type(preset) == HB_VALUE_TYPE_DICT &&
             hb_value_get_bool(hb_dict_get(preset, "Folder")))
    {
        // Perform do_func on the folder...
        result = do_func(preset, ctx);
        if (result != PRESET_DO_NEXT)
            return result;

        // Then perform preset action on the children of the folder
        ctx->path.depth++;
        next = hb_dict_get(preset, "ChildrenArray");
        result = presets_do(do_func, next, ctx);
        if (result == PRESET_DO_SUCCESS)
            return result;
        ctx->path.depth--;
        return result;
    }
    else if (hb_value_type(preset) == HB_VALUE_TYPE_DICT &&
             hb_dict_get(preset, "PresetName") != NULL)
    {
        // An individual, non-folder, preset
        return do_func(preset, ctx);
    }
    else
    {
        hb_error("Error: invalid preset format in presets_do()");
        return PRESET_DO_DELETE;
    }
    return PRESET_DO_DONE;
}

hb_preset_index_t* hb_preset_index_init(const int *index, int depth)
{
    hb_preset_index_t *path;
    path = calloc(1, sizeof(hb_preset_index_t));
    path->depth = depth;
    if (index != NULL)
        memcpy(path->index, index, depth * sizeof(int));
    return path;
}

hb_preset_index_t* hb_preset_index_dup(const hb_preset_index_t *path)
{
    if (path == NULL)
        return NULL;
    return hb_preset_index_init(path->index, path->depth);
}

void hb_preset_index_append(hb_preset_index_t *dst,
                            const hb_preset_index_t *src)
{
    int ii;
    for (ii = 0; ii < src->depth &&
                 dst->depth < HB_MAX_PRESET_FOLDER_DEPTH; ii++, dst->depth++)
    {
        dst->index[dst->depth] = src->index[ii];
    }
}

static int get_job_mux(hb_dict_t *job_dict)
{
    int mux;

    hb_dict_t *dest_dict = hb_dict_get(job_dict, "Destination");
    hb_value_t *mux_value = hb_dict_get(dest_dict, "Mux");
    if (hb_value_type(mux_value) == HB_VALUE_TYPE_STRING)
    {
        mux = hb_container_get_from_name(hb_value_get_string(mux_value));
        if (mux == 0)
            mux = hb_container_get_from_extension(
                                                hb_value_get_string(mux_value));
    }
    else
    {
        mux = hb_value_get_int(mux_value);
    }
    hb_container_t *container = hb_container_get_from_format(mux);
    if (container == NULL)
    {
        char *str = hb_value_get_string_xform(mux_value);
        hb_error("Invalid container (%s)", str);
        free(str);
        return HB_MUX_INVALID;
    }
    return mux;
}

static hb_value_t* get_audio_copy_mask(const hb_dict_t * preset, int *mask)
{
    int copy_mask = 0;
    hb_value_array_t *out_copy_mask, *in_copy_mask;

    if (mask != NULL)
        *mask = 0;
    in_copy_mask  = hb_dict_get(preset, "AudioCopyMask");
    out_copy_mask = hb_value_array_init();
    if (in_copy_mask != NULL)
    {
        int count, ii;
        count = hb_value_array_len(in_copy_mask);
        for (ii = 0; ii < count; ii++)
        {
            int codec;
            hb_value_t *value;
            value = hb_value_array_get(in_copy_mask, ii);
            if (hb_value_type(value) == HB_VALUE_TYPE_STRING)
            {
                char *tmp = NULL;
                const char * s = hb_value_get_string(value);
                // Only codecs that start with 'copy:' can be copied
                if (strncmp(s, "copy:", 5))
                {
                    s = tmp = hb_strdup_printf("copy:%s", s);
                }
                codec = hb_audio_encoder_get_from_name(s);
                if (codec == 0)
                {
                    hb_error("Invalid audio codec in autopassthru copy mask (%s)", s);
                    hb_error("Codec name is invalid or can not be copied");
                    free(tmp);
                    hb_value_free(&out_copy_mask);
                    return NULL;
                }
                free(tmp);
            }
            else
            {
                codec = hb_value_get_int(value);
            }
            hb_value_array_append(out_copy_mask, hb_value_string(
                hb_audio_encoder_get_short_name(codec)));
            copy_mask |= codec;
        }
    }
    if (mask != NULL)
        *mask = copy_mask;
    return out_copy_mask;
}

static hb_dict_t * source_audio_track_used(hb_dict_t *track_dict, int track)
{
    // Kind of hacky, but keys must be strings
    char key[8];
    snprintf(key, sizeof(key), "%d", track);

    hb_dict_t *used = hb_dict_get(track_dict, key);
    if (used == NULL)
    {
        used = hb_dict_init();
        hb_dict_set(track_dict, key, used);
    }
    return used;
}

// Find a source audio track matching given language
static int find_audio_track(const hb_title_t *title,
                            const char *lang, int start, int behavior)
{
    hb_audio_config_t   * audio;
    int                   ii, count;
    const iso639_lang_t * lang_any = lang_get_any();

    count = hb_list_count(title->list_audio);
    for (ii = start; ii < count; ii++)
    {
        audio = hb_list_audio_config_item(title->list_audio, ii);
        // When behavior is "first" matching track,
        // ignore secondary audio types
        //
        // When behavior is "all" matching tracks,
        // allow any audio track type
        if ((behavior == 2 ||
             audio->lang.attributes == HB_AUDIO_ATTR_NONE ||
             (audio->lang.attributes & HB_AUDIO_ATTR_REGULAR_MASK)) &&
            (!strcmp(lang, audio->lang.iso639_2) ||
             !strcmp(lang, lang_any->iso639_2)))
        {
            return ii;
        }
    }
    return -1;
}

static int validate_audio_encoders(const hb_dict_t *preset)
{
    hb_value_array_t * encoder_list = hb_dict_get(preset, "AudioList");
    int count = hb_value_array_len(encoder_list);
    int ii;
    for (ii = 0; ii < count; ii++)
    {
        hb_value_t *audio_dict = hb_value_array_get(encoder_list, ii);
        hb_value_t *value;
        int codec, mix, sr;
        value = hb_dict_get(audio_dict, "AudioEncoder");
        if (hb_value_type(value) == HB_VALUE_TYPE_STRING)
        {
            codec = hb_audio_encoder_get_from_name(hb_value_get_string(value));
        }
        else
        {
            codec = hb_value_get_int(value);
        }
        if (hb_audio_encoder_get_from_codec(codec) == NULL)
        {
            char *str = hb_value_get_string_xform(value);
            hb_error("Invalid audio encoder (%s)", str);
            free(str);
            return -1;
        }

        value = hb_dict_get(audio_dict, "AudioMixdown");
        if (hb_value_type(value) == HB_VALUE_TYPE_STRING)
        {
            mix = hb_mixdown_get_from_name(hb_value_get_string(value));
        }
        else
        {
            mix = hb_value_get_int(value);
        }
        if (hb_mixdown_get_from_mixdown(mix) == NULL)
        {
            char *str = hb_value_get_string_xform(value);
            hb_error("Invalid audio mixdown (%s)", str);
            free(str);
            return -1;
        }

        value = hb_dict_get(audio_dict, "AudioSamplerate");
        if (hb_value_type(value) == HB_VALUE_TYPE_STRING)
        {
            const char *str = hb_value_get_string(value);
            if (!strcasecmp(str, "source") ||
                !strcasecmp(str, "auto")   ||
                !strcasecmp(str, "same as source"))
            {
                sr = 0;
            }
            else
            {
                sr = hb_audio_samplerate_get_from_name(str);
            }
        }
        else
        {
            sr = hb_value_get_int(value);
        }
        if (sr != 0 && hb_audio_samplerate_get_name(sr) == NULL)
        {
            char *str = hb_value_get_string_xform(value);
            hb_error("Invalid audio samplerate (%s)", str);
            free(str);
            return -1;
        }
    }
    return 0;
}

static int sanitize_audio_codec(int in_codec, int out_codec,
                                int copy_mask, int fallback, int mux)
{
    int codec = out_codec;
    if (out_codec == HB_ACODEC_AUTO_PASS)
    {
        codec = hb_autopassthru_get_encoder(in_codec, copy_mask, fallback, mux);
    }
    else if ((out_codec & HB_ACODEC_PASS_FLAG) &&
             !(in_codec & out_codec & HB_ACODEC_PASS_MASK))
    {
        codec = hb_audio_encoder_get_fallback_for_passthru(out_codec);
        if (codec == HB_ACODEC_INVALID)
            codec = fallback;
    }

    // Check that encoder is valid for mux
    const hb_encoder_t *encoder = NULL;
    while ((encoder = hb_audio_encoder_get_next(encoder)) != NULL)
    {
        if (encoder->codec == codec && codec != HB_ACODEC_NONE &&
            !(encoder->muxers & mux))
        {
            codec = hb_audio_encoder_get_default(mux);
            break;
        }
    }
    if (codec == HB_ACODEC_INVALID)
        codec = hb_audio_encoder_get_default(mux);
    return codec;
}

void hb_sanitize_audio_settings(const hb_title_t * title,
                                hb_value_t * audio_settings)
{
    const char        * mix_name;
    const char        * codec_name;
    int                 track, codec, mix, bitrate, samplerate;
    int                 quality_enable;
    double              quality;
    hb_value_t        * val;
    hb_audio_config_t * audio_config = NULL;

    track      = hb_dict_get_int(audio_settings, "Track");
    codec_name = hb_dict_get_string(audio_settings, "Encoder");
    codec      = hb_audio_encoder_get_from_name(codec_name);
    mix_name   = hb_dict_get_string(audio_settings, "Mixdown");
    mix        = hb_mixdown_get_from_name(mix_name);
    bitrate    = hb_dict_get_int(audio_settings, "Bitrate");
    quality    = hb_dict_get_double(audio_settings, "Quality");
    samplerate = hb_dict_get_int(audio_settings, "Samplerate");
    val        = hb_dict_get(audio_settings, "Quality");
    quality_enable = !(bitrate > 0 || val == NULL ||
                       quality == HB_INVALID_AUDIO_QUALITY);

    if (title != NULL)
    {
        audio_config = hb_list_audio_config_item(title->list_audio, track);
    }
    if (samplerate == 0 && audio_config != NULL)
    {
        samplerate = audio_config->in.samplerate;
    }

    if (codec & HB_ACODEC_PASS_FLAG)
    {
        mix = HB_AMIXDOWN_NONE;
        hb_dict_set(audio_settings, "Mixdown",
                    hb_value_string(hb_mixdown_get_short_name(mix)));
        hb_dict_set(audio_settings, "Samplerate", hb_value_int(0));
        hb_dict_set(audio_settings, "DRC", hb_value_double(0.0));
    }
    else
    {
        int layout = AV_CH_LAYOUT_5POINT1;
        if (audio_config != NULL)
        {
            layout = audio_config->in.channel_layout;
        }
        if (mix == HB_AMIXDOWN_NONE)
        {
            mix = HB_INVALID_AMIXDOWN;
        }
        mix = hb_mixdown_get_best(codec, layout, mix);
        if (quality_enable)
        {
            float low, high, gran;
            int dir;
            hb_audio_quality_get_limits(codec, &low, &high, &gran, &dir);
            if (quality < low || quality > high)
            {
                quality = hb_audio_quality_get_default(codec);
            }
            else
            {
                quality = hb_audio_quality_get_best(codec, quality);
            }
        }
        else
        {
            if (bitrate != -1)
            {
                bitrate = hb_audio_bitrate_get_best(codec, bitrate,
                                                    samplerate, mix);
            }
            else
            {
                bitrate = hb_audio_bitrate_get_default(codec, samplerate, mix);
            }
        }
        hb_dict_set(audio_settings, "Mixdown",
                    hb_value_string(hb_mixdown_get_short_name(mix)));
    }
    if (quality_enable)
    {
        bitrate = -1;
    }
    else
    {
        quality = HB_INVALID_AUDIO_QUALITY;
    }
    hb_dict_set(audio_settings, "Quality", hb_value_double(quality));
    hb_dict_set(audio_settings, "Bitrate", hb_value_int(bitrate));
    hb_dict_set(audio_settings, "Encoder",
                hb_value_string(hb_audio_encoder_get_short_name(codec)));
}

static void add_audio_for_lang(hb_value_array_t *list, const hb_dict_t *preset,
                               hb_title_t *title, int mux, int copy_mask,
                               int fallback, const char *lang,
                               int behavior, int mode, hb_dict_t *track_dict)
{
    hb_value_array_t * encoder_list = hb_dict_get(preset, "AudioList");
    int count = hb_value_array_len(encoder_list);
    int track = find_audio_track(title, lang, 0, behavior);
    while (track >= 0)
    {
        int track_count = hb_value_array_len(list);
        char key[8];
        snprintf(key, sizeof(key), "%d", track);

        count = mode && track_count ? 1 : count;
        int ii;
        for (ii = 0; ii < count; ii++)
        {
            // Check if this source track has already been added using these
            // same encoder settings.  If so, continue to next track.
            hb_dict_t *used = source_audio_track_used(track_dict, ii);
            if (hb_value_get_bool(hb_dict_get(used, key)))
                continue;

            // Create new audio output track settings
            hb_dict_t *audio_dict = hb_dict_init();
            hb_value_t *acodec_value;
            hb_dict_t *encoder_dict = hb_value_array_get(encoder_list, ii);
            int out_codec;

            acodec_value = hb_dict_get(encoder_dict, "AudioEncoder");
            if (hb_value_type(acodec_value) == HB_VALUE_TYPE_STRING)
            {
                out_codec = hb_audio_encoder_get_from_name(
                                hb_value_get_string(acodec_value));
            }
            else
            {
                out_codec = hb_value_get_int(acodec_value);
            }
            // Save the encoder value before sanitizing.  This value is
            // useful to the frontends.
            hb_dict_set(audio_dict, "PresetEncoder",
                hb_value_string(hb_audio_encoder_get_short_name(out_codec)));

            hb_audio_config_t *aconfig;
            aconfig = hb_list_audio_config_item(title->list_audio, track);
            out_codec = sanitize_audio_codec(aconfig->in.codec, out_codec,
                                             copy_mask, fallback, mux);
            if (out_codec == HB_ACODEC_NONE || HB_ACODEC_INVALID)
            {
                hb_value_free(&audio_dict);
                continue;
            }
            hb_dict_set(audio_dict, "Track", hb_value_int(track));
            hb_dict_set(audio_dict, "Encoder", hb_value_string(
                        hb_audio_encoder_get_short_name(out_codec)));
            const char * name = hb_dict_get_string(encoder_dict, "AudioTrackName");
            if (name != NULL && name[0] != 0)
            {
                hb_dict_set_string(audio_dict, "Name", name);
            }
            else if (aconfig->in.name != NULL && aconfig->in.name[0] != 0)
            {
                hb_dict_set_string(audio_dict, "Name", aconfig->in.name);
            }
            if (!(out_codec & HB_ACODEC_PASS_FLAG))
            {
                if (hb_dict_get(encoder_dict, "AudioTrackGainSlider") != NULL)
                {
                    hb_dict_set(audio_dict, "Gain", hb_value_dup(
                        hb_dict_get(encoder_dict, "AudioTrackGainSlider")));
                }
                if (hb_dict_get(encoder_dict, "AudioTrackDRCSlider") != NULL)
                {
                    hb_dict_set(audio_dict, "DRC", hb_value_dup(
                        hb_dict_get(encoder_dict, "AudioTrackDRCSlider")));
                }
                if (hb_dict_get(encoder_dict, "AudioMixdown") != NULL)
                {
                    hb_dict_set(audio_dict, "Mixdown", hb_value_dup(
                        hb_dict_get(encoder_dict, "AudioMixdown")));
                }
                if (hb_dict_get(encoder_dict, "AudioNormalizeMixLevel") != NULL)
                {
                    hb_dict_set(audio_dict, "NormalizeMixLevel", hb_value_dup(
                        hb_dict_get(encoder_dict, "AudioNormalizeMixLevel")));
                }
                if (hb_dict_get(encoder_dict, "AudioDitherMethod") != NULL)
                {
                    hb_dict_set(audio_dict, "DitherMethod", hb_value_dup(
                        hb_dict_get(encoder_dict, "AudioDitherMethod")));
                }
                if (hb_dict_get(encoder_dict, "AudioSamplerate") != NULL)
                {
                    const char * sr_name;
                    int          sr;

                    sr_name = hb_dict_get_string(encoder_dict,
                                                 "AudioSamplerate");
                    sr      = hb_audio_samplerate_get_from_name(sr_name);
                    if (sr < 0)
                    {
                        sr = 0;
                    }
                    hb_dict_set(audio_dict, "Samplerate", hb_value_int(sr));
                }
                if (hb_dict_get(encoder_dict, "AudioCompressionLevel") != NULL)
                {
                    hb_dict_set(audio_dict, "CompressionLevel", hb_value_dup(
                        hb_dict_get(encoder_dict, "AudioCompressionLevel")));
                }
                if (hb_value_get_bool(hb_dict_get(encoder_dict,
                                                  "AudioTrackQualityEnable")))
                {
                    hb_dict_set(audio_dict, "Quality", hb_value_xform(
                            hb_dict_get(encoder_dict, "AudioTrackQuality"),
                            HB_VALUE_TYPE_DOUBLE));
                }
                else
                {
                    hb_dict_set(audio_dict, "Bitrate", hb_value_xform(
                        hb_dict_get(encoder_dict, "AudioBitrate"),
                        HB_VALUE_TYPE_INT));
                }
            }

            // Sanitize the settings before adding to the audio list
            hb_sanitize_audio_settings(title,  audio_dict);

            hb_value_array_append(list, audio_dict);
            hb_dict_set(used, key, hb_value_bool(1));
        }
        if (behavior == 2)
            track = find_audio_track(title, lang, track + 1, behavior);
        else
            break;
    }
}

// This function assumes that Mux has already been initialized in
// the job_dict
int hb_preset_job_add_audio(hb_handle_t *h, int title_index,
                            const hb_dict_t *preset, hb_dict_t *job_dict)
{
    hb_title_t *title = hb_find_title_by_index(h, title_index);
    if (title == NULL)
    {
        // Can't create audio track list without knowing source audio tracks
        hb_error("Invalid title index (%d)", title_index);
        return -1;
    }
    if (hb_list_count(title->list_audio) <= 0)
    {
        // Source has no audio
        return 0;
    }

    int mux = get_job_mux(job_dict);
    if (mux == HB_MUX_INVALID)
    {
        return -1;
    }

    hb_dict_t *audio_dict = hb_dict_get(job_dict, "Audio");
    if (audio_dict == NULL)
    {
        audio_dict = hb_dict_init();
        hb_dict_set(job_dict, "Audio", audio_dict);
    }
    int copy_mask;
    hb_value_t *copy_mask_array = get_audio_copy_mask(preset, &copy_mask);
    if (copy_mask_array == NULL)
    {
        return -1;
    }
    int fallback = 0;
    hb_dict_set(audio_dict, "CopyMask", copy_mask_array);
    hb_value_t *fallback_value = hb_dict_get(preset, "AudioEncoderFallback");
    if (fallback_value != NULL)
    {
        hb_dict_set(audio_dict, "FallbackEncoder",
                    hb_value_dup(fallback_value));
        if (hb_value_type(fallback_value) == HB_VALUE_TYPE_STRING)
        {
            const char * s = hb_value_get_string(fallback_value);
            fallback = hb_audio_encoder_get_from_name(s);
            if (fallback == 0)
            {
                hb_error("Invalid fallback audio codec (%s)", s);
                return -1;
            }
        }
        else
        {
            fallback = hb_value_get_int(fallback_value);
        }
    }
    if (validate_audio_encoders(preset) < 0)
        return -1;

    hb_value_array_t *list = hb_dict_get(audio_dict, "AudioList");
    if (list == NULL)
    {
        list = hb_value_array_init();
        hb_dict_set(audio_dict, "AudioList", list);
    }

    int behavior = 1;   // default first
    const char *s;
    s = hb_value_get_string(hb_dict_get(preset, "AudioTrackSelectionBehavior"));
    if (s != NULL)
    {
        if      (!strcasecmp(s, "none"))
            return 0;
        else if (!strcasecmp(s, "all"))
            behavior = 2;
    }

    // Create hash that is used to track which tracks have been already added
    // We do not want to add the same track with the same settings twice
    hb_dict_t *track_dict = hb_dict_init();

    // Add tracks for all languages in the language list
    int mode;
    hb_value_array_t *lang_list = hb_dict_get(preset, "AudioLanguageList");
    mode = hb_value_get_bool(hb_dict_get(preset, "AudioSecondaryEncoderMode"));
    int count = hb_value_array_len(lang_list);
    int ii;
    for (ii = 0; ii < count; ii++)
    {
        const char *lang;
        lang = hb_value_get_string(hb_value_array_get(lang_list, ii));
        add_audio_for_lang(list, preset, title, mux, copy_mask, fallback,
                           lang, behavior, mode, track_dict);
    }
    // If AudioLanguageList is empty, try "any" language option
    if (count <= 0)
    {
        add_audio_for_lang(list, preset, title, mux, copy_mask, fallback,
                           "any", behavior, mode, track_dict);
    }
    hb_dict_free(&track_dict);
    return 0;
}

// Find a source audio track matching given language
static int find_subtitle_track(const hb_title_t *title,
                               const char *lang, int start)
{
    hb_subtitle_t       * subtitle;
    int                   ii, count;
    const iso639_lang_t * lang_any = lang_get_any();

    count = hb_list_count(title->list_subtitle);
    for (ii = start; ii < count; ii++)
    {
        subtitle = hb_list_item(title->list_subtitle, ii);
        if (!strcmp(lang, subtitle->iso639_2) ||
            !strcmp(lang, lang_any->iso639_2))
        {
            return ii;
        }
    }
    return -1;
}

static void add_subtitle(hb_value_array_t *list, int track,
                         int make_default, int force, int burn,
                         const char * name)
{
    hb_dict_t *subtitle_dict = hb_dict_init();
    hb_dict_set_int(subtitle_dict, "Track", track);
    hb_dict_set_bool(subtitle_dict, "Default", make_default);
    hb_dict_set_bool(subtitle_dict, "Forced", force);
    hb_dict_set_bool(subtitle_dict, "Burn", burn);
    if (name != NULL && name[0] != 0)
    {
        hb_dict_set_string(subtitle_dict, "Name", name);
    }
    hb_value_array_append(list, subtitle_dict);
}

typedef struct subtitle_behavior_s
{
    int one;
    int burn_foreign;
    int make_default;
    int burn_first;
    int burn_dvd;
    int burn_bd;
    int one_burned;
    uint8_t *used;
} subtitle_behavior_t;

static int has_default_subtitle(hb_value_array_t *list)
{
    int ii, count, def;

    count = hb_value_array_len(list);
    for (ii = 0; ii < count; ii++)
    {
        hb_value_t *sub = hb_value_array_get(list, ii);
        def = hb_value_get_int(hb_dict_get(sub, "Default"));
        if (def)
        {
            return def;
        }
    }
    return 0;
}

static void add_subtitle_for_lang(hb_value_array_t *list, hb_title_t *title,
                                  int mux, const char *lang,
                                  subtitle_behavior_t *behavior)
{
    int t;
    for (t = find_subtitle_track(title, lang, 0);
         t >= 0;
         t = behavior->one ? -1 : find_subtitle_track(title, lang, t + 1))
    {
        if (behavior->used[t])
        {
            if (behavior->one)
                break;
            continue;
        }
        int burn, make_default;
        hb_subtitle_t *subtitle;
        subtitle = hb_list_item(title->list_subtitle, t);
        burn = !behavior->one_burned &&
               ((subtitle->source == VOBSUB && behavior->burn_dvd) ||
                (subtitle->source == PGSSUB && behavior->burn_bd)  ||
                !hb_subtitle_can_pass(subtitle->source, mux) ||
                behavior->burn_first || behavior->burn_foreign);
        // If the subtitle is added for foreign audio, or the source
        // subtitle was the default subtitle, mark this subtitle as
        // default.
        make_default = (!burn && behavior->make_default) ||
                       (!has_default_subtitle(list) &&
                        subtitle->config.default_track);

        if (!behavior->one_burned || hb_subtitle_can_pass(subtitle->source, mux))
        {
            add_subtitle(list, t, make_default, 0 /*!force*/, burn, subtitle->name);
        }

        behavior->burn_first &= !burn;
        behavior->one_burned |= burn;
        behavior->used[t] = 1;
    }
}

// This function assumes that the AudioList and Mux have already been
// initialized in the job_dict
int hb_preset_job_add_subtitles(hb_handle_t *h, int title_index,
                                const hb_dict_t *preset, hb_dict_t *job_dict)
{
    hb_title_t *title = hb_find_title_by_index(h, title_index);
    if (title == NULL)
    {
        // Can't create subtitle track list without knowing source
        hb_error("Invalid title index (%d)", title_index);
        return -1;
    }

    int mux = get_job_mux(job_dict);
    if (mux == HB_MUX_INVALID)
    {
        return -1;
    }

    // Get the language of the first audio output track
    // Needed for subtitle track selection
    hb_dict_t *audio_dict = hb_dict_get(job_dict, "Audio");
    hb_value_array_t *audio_list = hb_dict_get(audio_dict, "AudioList");
    const char *first_audio_lang = NULL;
    if (hb_value_array_len(audio_list) > 0)
    {
        int track;
        hb_value_t *audio = hb_value_array_get(audio_list, 0);
        track = hb_value_get_int(hb_dict_get(audio, "Track"));
        if (hb_list_count(title->list_audio) > track)
        {
            hb_audio_config_t *aconfig;
            aconfig = hb_list_audio_config_item(title->list_audio, track);
            if (aconfig != NULL)
                first_audio_lang = aconfig->lang.iso639_2;
        }
    }

    int source_subtitle_count = hb_list_count(title->list_subtitle);
    if (source_subtitle_count == 0)
        return 0;

    hb_dict_t *subtitle_dict = hb_dict_get(job_dict, "Subtitle");
    if (subtitle_dict == NULL)
    {
        subtitle_dict = hb_dict_init();
        hb_dict_set(job_dict, "Subtitle", subtitle_dict);
    }
    hb_value_array_t *list = hb_dict_get(subtitle_dict, "SubtitleList");
    if (list == NULL)
    {
        list = hb_value_array_init();
        hb_dict_set(subtitle_dict, "SubtitleList", list);
    }

    int track_behavior = 0;   // default no subtitles
    int burn_behavior = 0;
    int burn_foreign;

    struct subtitle_behavior_s behavior;
    behavior.one = 0;
    behavior.burn_foreign = 0;
    behavior.make_default = 0;
    behavior.burn_first = 0;
    behavior.burn_dvd = 0;
    behavior.burn_bd = 0;
    behavior.one_burned = 0;
    // Create array that is used to track which tracks have been already added
    // We do not want to add the same track with the same settings twice
    behavior.used = calloc(source_subtitle_count, sizeof(*behavior.used));

    // Since this function can be called multiple times, we need to
    // initialize the "used" array from the existing subtitles in the list.
    int count, ii;
    count = hb_value_array_len(list);
    for (ii = 0; ii < count; ii++)
    {
        hb_value_t *sub = hb_value_array_get(list, ii);
        int track = hb_value_get_int(hb_dict_get(sub, "Track"));
        behavior.used[track] = 1;
    }

    const char *s;
    s = hb_value_get_string(hb_dict_get(preset,
                                        "SubtitleTrackSelectionBehavior"));
    if (s != NULL)
    {
        if      (!strcasecmp(s, "first"))
            track_behavior = 1;
        else if (!strcasecmp(s, "all"))
            track_behavior = 2;
    }

    s = hb_value_get_string(hb_dict_get(preset, "SubtitleBurnBehavior"));
    if (s != NULL)
    {
        if      (!strcasecmp(s, "foreign"))
            burn_behavior = 1;
        else if (!strcasecmp(s, "first"))
            burn_behavior = 2;
        else if (!strcasecmp(s, "foreign_first"))
            burn_behavior = 3;
    }

    behavior.burn_dvd = hb_value_get_int(hb_dict_get(preset,
                                                     "SubtitleBurnDVDSub"));
    behavior.burn_bd  = hb_value_get_int(hb_dict_get(preset,
                                                     "SubtitleBurnBDSub"));

    burn_foreign        = burn_behavior == 1 || burn_behavior == 3;
    behavior.burn_first = burn_behavior == 2 || burn_behavior == 3;

    int foreign_audio_search, foreign_first_audio;
    foreign_audio_search = hb_value_get_bool(hb_dict_get(preset,
                                            "SubtitleAddForeignAudioSearch"));
    foreign_first_audio  = hb_value_get_bool(hb_dict_get(preset,
                                            "SubtitleAddForeignAudioSubtitle"));


    // Add tracks for all languages in the language list
    hb_value_array_t * lang_list = hb_dict_get(preset, "SubtitleLanguageList");
    const iso639_lang_t * lang_any  = lang_get_any();
    const char          * pref_lang = lang_any->iso639_2;

    count = hb_value_array_len(lang_list);
    if (count > 0)
    {
        pref_lang = hb_value_get_string(hb_value_array_get(lang_list, 0));
    }
    if (!strcmp(pref_lang, lang_any->iso639_2))
    {
        if (first_audio_lang != NULL)
        {
            pref_lang = first_audio_lang;
            foreign_first_audio = 0;
        }
        else
        {
            foreign_audio_search = foreign_first_audio = 0;
        }
    }

    int track;
    if (first_audio_lang != NULL &&
        foreign_first_audio && strncmp(first_audio_lang, pref_lang, 4))
    {
        // First audio lang does not match the preferred subtitle lang.
        // Preset says to add pref lang subtitle.
        // Foreign audio search is not necessary since entire audio track
        // is foreign.
        foreign_audio_search = 0;
        behavior.one = 1;
        behavior.burn_foreign = burn_foreign;
        behavior.make_default = 1;
        add_subtitle_for_lang(list, title, mux, pref_lang, &behavior);
    }

    hb_dict_t *search_dict = hb_dict_get(subtitle_dict, "Search");
    if (search_dict == NULL)
    {
        search_dict = hb_dict_init();
        hb_dict_set(subtitle_dict, "Search", search_dict);
    }
    if (first_audio_lang != NULL &&
        foreign_audio_search && !strncmp(first_audio_lang, pref_lang, 4))
    {
        // First audio lang matches the preferred subtitle lang.
        // Preset says to add search for foreign audio subtitles.
        int burn = burn_foreign || behavior.burn_first;
        // If not burning, make this the default track.
        hb_dict_set(search_dict, "Enable", hb_value_bool(1));
        hb_dict_set(search_dict, "Default", hb_value_bool(!burn));
        hb_dict_set(search_dict, "Forced", hb_value_bool(1));
        hb_dict_set(search_dict, "Burn", hb_value_bool(burn));
    }
    else
    {
        hb_dict_set(search_dict, "Enable", hb_value_bool(0));
    }

    if (track_behavior > 0)
    {
        int ii;
        behavior.one = track_behavior == 1;
        behavior.burn_foreign = 0;
        behavior.make_default = 0;
        for (ii = 0; ii < count; ii++)
        {
            const char *lang;
            lang = hb_value_get_string(hb_value_array_get(lang_list, ii));
            add_subtitle_for_lang(list, title, mux, lang, &behavior);
        }
        if (count <= 0)
        {
            // No languages in language list, assume "any"
            add_subtitle_for_lang(list, title, mux, "any", &behavior);
        }
    }

    if (hb_value_get_bool(hb_dict_get(preset, "SubtitleAddCC")))
    {
        // Add Closed Caption track
        for (track = 0; track < source_subtitle_count; track++)
        {
            if (behavior.used[track])
            {
                continue;
            }
            hb_subtitle_t *subtitle = hb_list_item(title->list_subtitle, track);
            if (subtitle->source == CC608SUB || subtitle->source == CC708SUB)
            {
                int burn;
                burn = !behavior.one_burned &&
                       (!hb_subtitle_can_pass(subtitle->source, mux) ||
                        behavior.burn_first);
                behavior.used[track] = 1;
                behavior.one_burned |= burn;
                add_subtitle(list, track, 0 /*default*/, 0 /*!force*/, burn,
                             subtitle->name);
                break;
            }
        }
    }
    free(behavior.used);

    return 0;
}

static int get_video_framerate(hb_value_t *rate_value)
{
    // Predefined by name
    if (hb_value_type(rate_value) == HB_VALUE_TYPE_STRING)
    {
        int rate = 0;
        const char *rate_name = hb_value_get_string(rate_value);
        if (!strcasecmp(rate_name, "source") ||
            !strcasecmp(rate_name, "auto") ||
            !strcasecmp(rate_name, "same as source"))
        {
            return rate;
        }
        else
        {
            rate = hb_video_framerate_get_from_name(rate_name);
            if (rate != -1)
            {
                return rate;
            }
        }
    }

    // Arbitrary
    int clock_min, clock_max, clock,
        frame_min, frame_max;
    hb_video_framerate_get_limits(&clock_min, &clock_max, &clock);
    frame_min = clock / clock_max;
    frame_max = clock / clock_min;
    double rate_d = hb_value_get_double(rate_value);
    if (rate_d >= frame_min && rate_d <= frame_max)
    {
        // Value is a framerate, return clockrate
        return (int)(clock / rate_d);
    }
    else if (rate_d >= clock_min && rate_d <= clock_max)
    {
        // Value is already a clockrate
        return (int)(rate_d);
    }

    // Value out of bounds
    return -1;
}

int hb_preset_apply_filters(const hb_dict_t *preset, hb_dict_t *job_dict)
{
    hb_value_t * filters_dict, * filter_list, * filter_dict;
    hb_dict_t  * filter_settings;

    int clock_min, clock_max, clock;
    hb_video_framerate_get_limits(&clock_min, &clock_max, &clock);

    // Create new filters
    filters_dict = hb_dict_get(job_dict, "Filters");
    if (filters_dict == NULL)
    {
        filters_dict = hb_dict_init();
        hb_dict_set(job_dict, "Filters", filters_dict);
    }
    filter_list = hb_dict_get(filters_dict, "FilterList");
    if (filter_list == NULL)
    {
        filter_list = hb_value_array_init();
        hb_dict_set(filters_dict, "FilterList", filter_list);
    }

    // Detelecine filter
    hb_value_t *detel_val = hb_dict_get(preset, "PictureDetelecine");
    if (detel_val != NULL)
    {
        const char *custom;
        custom = hb_value_get_string(hb_dict_get(preset,
                                                "PictureDetelecineCustom"));
        filter_settings = hb_generate_filter_settings(
            HB_FILTER_DETELECINE, hb_value_get_string(detel_val), NULL, custom);

        if (filter_settings == NULL)
        {
            char *s = hb_value_get_string_xform(detel_val);
            hb_error("Invalid detelecine filter settings (%s)", s);
            free(s);
            return -1;
        }
        else if (!hb_dict_get_bool(filter_settings, "disable"))
        {
            filter_dict = hb_dict_init();
            hb_dict_set(filter_dict, "ID", hb_value_int(HB_FILTER_DETELECINE));
            hb_dict_set(filter_dict, "Settings", filter_settings);
            hb_add_filter2(filter_list, filter_dict);
        }
        else
        {
            hb_value_free(&filter_settings);
        }
    }

    const char *comb_preset;
    comb_preset = hb_value_get_string(hb_dict_get(preset,
                                      "PictureCombDetectPreset"));
    if (comb_preset != NULL)
    {
        const char *comb_custom;
        comb_custom = hb_value_get_string(hb_dict_get(preset,
                                          "PictureCombDetectCustom"));
        filter_settings = hb_generate_filter_settings(HB_FILTER_COMB_DETECT,
                                                comb_preset, NULL, comb_custom);
        if (filter_settings == NULL)
        {
            hb_error("Invalid comb detect filter preset (%s)", comb_preset);
            return -1;
        }
        else if (!hb_dict_get_bool(filter_settings, "disable"))
        {
            filter_dict = hb_dict_init();
            hb_dict_set(filter_dict, "ID", hb_value_int(HB_FILTER_COMB_DETECT));
            hb_dict_set(filter_dict, "Settings", filter_settings);
            hb_add_filter2(filter_list, filter_dict);
        }
        else
        {
            hb_value_free(&filter_settings);
        }
    }

    // Decomb or deinterlace filters
    const char *deint_filter, *deint_preset, *deint_custom;
    deint_filter = hb_value_get_string(hb_dict_get(preset,
                                                   "PictureDeinterlaceFilter"));
    deint_preset = hb_value_get_string(hb_dict_get(preset,
                                                   "PictureDeinterlacePreset"));
    deint_custom = hb_value_get_string(hb_dict_get(preset,
                                                   "PictureDeinterlaceCustom"));
    if (deint_filter != NULL && deint_preset != NULL &&
        strcasecmp(deint_filter, "off"))
    {
        int filter_id;
        if (!strcasecmp(deint_filter, "decomb"))
        {
            filter_id = HB_FILTER_DECOMB;
        }
        else if (!strcasecmp(deint_filter, "deinterlace"))
        {
            filter_id = HB_FILTER_YADIF;
        }
        else if (!strcasecmp(deint_filter, "bwdif"))
        {
            filter_id = HB_FILTER_BWDIF;
        }
        else
        {
            hb_error("Invalid deinterlace filter (%s)", deint_filter);
            return -1;
        }
        filter_settings = hb_generate_filter_settings(
                        filter_id, deint_preset, NULL, deint_custom);
        if (filter_settings == NULL)
        {
            hb_error("Invalid deinterlace filter preset (%s)", deint_preset);
            return -1;
        }
        if (!hb_dict_get_bool(filter_settings, "disable"))
        {
            filter_dict = hb_dict_init();
            hb_dict_set(filter_dict, "ID", hb_value_int(filter_id));
            hb_dict_set(filter_dict, "Settings", filter_settings);
            hb_add_filter2(filter_list, filter_dict);
        }
        else
        {
            hb_value_free(&filter_settings);
        }
    }

    // Denoise filter
    int denoise;
    hb_value_t *denoise_value = hb_dict_get(preset, "PictureDenoiseFilter");
    denoise = hb_value_type(denoise_value) == HB_VALUE_TYPE_STRING ? (
        !strcasecmp(hb_value_get_string(denoise_value), "off") ? 0 :
        !strcasecmp(hb_value_get_string(denoise_value), "nlmeans") ? 1 : 2) :
        hb_value_get_int(denoise_value);

    if (denoise != 0)
    {
        int filter_id = denoise == 1 ? HB_FILTER_NLMEANS : HB_FILTER_HQDN3D;
        const char *denoise_preset, *denoise_tune, *denoise_custom;
        denoise_preset = hb_value_get_string(
                            hb_dict_get(preset, "PictureDenoisePreset"));
        if (denoise_preset != NULL)
        {
            denoise_tune   = hb_value_get_string(
                        hb_dict_get(preset, "PictureDenoiseTune"));
            denoise_custom = hb_value_get_string(
                        hb_dict_get(preset, "PictureDenoiseCustom"));

            filter_settings = hb_generate_filter_settings(filter_id,
                                denoise_preset, denoise_tune, denoise_custom);
            if (filter_settings == NULL)
            {
                hb_error("Invalid denoise filter settings (%s%s%s)",
                         denoise_preset,
                         denoise_tune ? "," : "",
                         denoise_tune ? denoise_tune : "");
                return -1;
            }
            else if (!hb_dict_get_bool(filter_settings, "disable"))
            {
                filter_dict = hb_dict_init();
                hb_dict_set(filter_dict, "ID", hb_value_int(filter_id));
                hb_dict_set(filter_dict, "Settings", filter_settings);
                hb_add_filter2(filter_list, filter_dict);
            }
            else
            {
                hb_value_free(&filter_settings);
            }
        }
    }

    // Chroma Smooth filter
    const char *chroma_smooth_preset, *chroma_smooth_tune, *chroma_smooth_custom;
    chroma_smooth_preset = hb_value_get_string(hb_dict_get(preset,
                                                   "PictureChromaSmoothPreset"));
    chroma_smooth_tune   = hb_value_get_string(hb_dict_get(preset,
                                                   "PictureChromaSmoothTune"));
    chroma_smooth_custom = hb_value_get_string(hb_dict_get(preset,
                                                   "PictureChromaSmoothCustom"));
    if (chroma_smooth_preset != NULL &&
        strcasecmp(chroma_smooth_preset, "off"))
    {
        int filter_id = HB_FILTER_CHROMA_SMOOTH;
        filter_settings = hb_generate_filter_settings(filter_id,
                            chroma_smooth_preset, chroma_smooth_tune, chroma_smooth_custom);
        if (filter_settings == NULL)
        {
            hb_error("Invalid chroma smooth filter settings (%s%s%s)",
                     chroma_smooth_preset,
                     chroma_smooth_tune ? "," : "",
                     chroma_smooth_tune ? chroma_smooth_tune : "");
            return -1;
        }
        else if (!hb_dict_get_bool(filter_settings, "disable"))
        {
            filter_dict = hb_dict_init();
            hb_dict_set(filter_dict, "ID", hb_value_int(filter_id));
            hb_dict_set(filter_dict, "Settings", filter_settings);
            hb_add_filter2(filter_list, filter_dict);
        }
        else
        {
            hb_value_free(&filter_settings);
        }
    }

    // Sharpen filter
    const char *sharpen_filter, *sharpen_preset, *sharpen_tune, *sharpen_custom;
    sharpen_filter = hb_value_get_string(hb_dict_get(preset,
                                                   "PictureSharpenFilter"));
    sharpen_preset = hb_value_get_string(hb_dict_get(preset,
                                                   "PictureSharpenPreset"));
    sharpen_tune   = hb_value_get_string(hb_dict_get(preset,
                                                   "PictureSharpenTune"));
    sharpen_custom = hb_value_get_string(hb_dict_get(preset,
                                                   "PictureSharpenCustom"));
    if (sharpen_filter != NULL && sharpen_preset != NULL &&
        strcasecmp(sharpen_filter, "off"))
    {
        int filter_id;
        if (!strcasecmp(sharpen_filter, "lapsharp"))
        {
            filter_id = HB_FILTER_LAPSHARP;
        }
        else if (!strcasecmp(sharpen_filter, "unsharp"))
        {
            filter_id = HB_FILTER_UNSHARP;
        }
        else
        {
            hb_error("Invalid sharpen filter (%s)", sharpen_filter);
            return -1;
        }
        filter_settings = hb_generate_filter_settings(filter_id,
                            sharpen_preset, sharpen_tune, sharpen_custom);
        if (filter_settings == NULL)
        {
            hb_error("Invalid sharpen filter settings (%s%s%s)",
                     sharpen_preset,
                     sharpen_tune ? "," : "",
                     sharpen_tune ? sharpen_tune : "");
            return -1;
        }
        else if (!hb_dict_get_bool(filter_settings, "disable"))
        {
            filter_dict = hb_dict_init();
            hb_dict_set(filter_dict, "ID", hb_value_int(filter_id));
            hb_dict_set(filter_dict, "Settings", filter_settings);
            hb_add_filter2(filter_list, filter_dict);
        }
        else
        {
            hb_value_free(&filter_settings);
        }
    }

    // Deblock filter
    const char * deblock = hb_value_get_string(
                                hb_dict_get(preset, "PictureDeblockPreset"));
    if (deblock != NULL)
    {
        const char * deblock_tune   = hb_value_get_string(
                                hb_dict_get(preset, "PictureDeblockTune"));
        const char * deblock_custom = hb_value_get_string(
                                hb_dict_get(preset, "PictureDeblockCustom"));
        filter_settings = hb_generate_filter_settings(HB_FILTER_DEBLOCK,
                                    deblock, deblock_tune, deblock_custom);
        if (filter_settings == NULL)
        {
            hb_error("Invalid deblock filter settings (%s)", deblock);
            return -1;
        }
        else if (!hb_dict_get_bool(filter_settings, "disable"))
        {
            filter_dict = hb_dict_init();
            hb_dict_set(filter_dict, "ID", hb_value_int(HB_FILTER_DEBLOCK));
            hb_dict_set(filter_dict, "Settings", filter_settings);
            hb_add_filter2(filter_list, filter_dict);
        }
        else
        {
            hb_value_free(&filter_settings);
        }
    }

    // Rotate filter
    char *rotate = hb_value_get_string_xform(
                        hb_dict_get(preset, "PictureRotate"));
    if (rotate != NULL)
    {
        filter_settings = hb_generate_filter_settings(HB_FILTER_ROTATE,
                                                      NULL, NULL, rotate);
        if (filter_settings == NULL)
        {
            hb_error("Invalid rotate filter settings (%s)", rotate);
            return -1;
        }
        else if (!hb_dict_get_bool(filter_settings, "disable") &&
                 (hb_dict_get_int(filter_settings, "angle") != 0 ||
                  hb_dict_get_bool(filter_settings, "hflip")))
        {
            filter_dict = hb_dict_init();
            hb_dict_set(filter_dict, "ID", hb_value_int(HB_FILTER_ROTATE));
            hb_dict_set(filter_dict, "Settings", filter_settings);
            hb_add_filter2(filter_list, filter_dict);
        }
        else
        {
            hb_value_free(&filter_settings);
        }
    }
    free(rotate);

    // Grayscale filter
    if (hb_value_get_bool(hb_dict_get(preset, "VideoGrayScale")))
    {
        filter_dict = hb_dict_init();
        hb_dict_set(filter_dict, "ID", hb_value_int(HB_FILTER_GRAYSCALE));
        hb_add_filter2(filter_list, filter_dict);
    }

    // Pad filter requires knowing crop/scale settings
    hb_dict_t * scale_filter, * scale_settings = NULL;
    scale_filter = hb_filter_dict_find(filter_list, HB_FILTER_CROP_SCALE);
    if (scale_filter != NULL)
    {
        scale_settings = hb_dict_get(scale_filter, "Settings");
    }

    char       * pad_params = NULL;
    const char * pad_mode = hb_dict_get_string(preset, "PicturePadMode");
    if (pad_mode != NULL && strcmp(pad_mode, "none"))
    {
        // Preset values used by the GUI and built-in presets
        int pad[4] = {0,};
        const char * color;

        if (!strcmp(pad_mode, "custom"))
        {
            pad[0] = hb_dict_get_int(preset, "PicturePadTop");
            pad[1] = hb_dict_get_int(preset, "PicturePadBottom");
            pad[2] = hb_dict_get_int(preset, "PicturePadLeft");
            pad[3] = hb_dict_get_int(preset, "PicturePadRight");
        }

        if (scale_settings != NULL)
        {
            int fillwidth, fillheight;

            int width     = hb_dict_get_int(scale_settings, "width");
            int height    = hb_dict_get_int(scale_settings, "height");
            int maxWidth  = hb_dict_get_int(preset, "PictureWidth");
            int maxHeight = hb_dict_get_int(preset, "PictureHeight");

            fillwidth  = fillheight  = !strcmp(pad_mode, "fill");
            fillheight = fillheight || !strcmp(pad_mode, "letterbox");
            fillwidth  = fillwidth  || !strcmp(pad_mode, "pillarbox");

            if (fillwidth && maxWidth > 0)
            {
                pad[2] = (maxWidth  - width) / 2;
                pad[3] =  maxWidth  - width - pad[2];
            }
            if (fillheight && maxHeight > 0)
            {
                pad[0] = (maxHeight - height) / 2;
                pad[1] =  maxHeight - height - pad[0];
            }
        }
        color  = hb_dict_get_string(preset, "PicturePadColor");
        if (color == NULL)
        {
            color = "black";
        }

        pad_params =
            hb_strdup_printf("top=%d:bottom=%d:left=%d:right=%d:color=%s",
                             pad[0], pad[1], pad[2], pad[3], color);
    }
    else
    {
        // Preset value used by the CLI
        pad_params =
            hb_value_get_string_xform(hb_dict_get(preset, "PicturePad"));
    }
    if (pad_params != NULL)
    {
        filter_settings = hb_generate_filter_settings(HB_FILTER_PAD,
                                                      NULL, NULL, pad_params);
        if (filter_settings == NULL)
        {
            hb_error("Invalid pad filter settings (%s)", pad_params);
            return -1;
        }
        else if (!hb_dict_get_bool(filter_settings, "disable"))
        {
            filter_dict = hb_dict_init();
            hb_dict_set(filter_dict, "ID", hb_value_int(HB_FILTER_PAD));
            hb_dict_set(filter_dict, "Settings", filter_settings);
            hb_add_filter2(filter_list, filter_dict);
        }
        else
        {
            hb_value_free(&filter_settings);
        }
        free(pad_params);
    }

    // Colorspace filter
    const char *colorspace = hb_value_get_string(hb_dict_get(preset, "PictureColorspacePreset"));
    if (colorspace != NULL &&
        strcasecmp(colorspace, "off"))
    {
        const char * colorspace_custom = hb_value_get_string(
                                hb_dict_get(preset, "PictureColorspaceCustom"));
        filter_settings = hb_generate_filter_settings(HB_FILTER_COLORSPACE,
                                    colorspace, NULL, colorspace_custom);
        if (filter_settings == NULL)
        {
            hb_error("Invalid colorspace filter settings (%s%s%s)",
                     colorspace,
                     colorspace_custom ? "," : "",
                     colorspace_custom ? colorspace_custom : "");
            return -1;
        }
        else if (!hb_dict_get_bool(filter_settings, "disable"))
        {
            filter_dict = hb_dict_init();
            hb_dict_set(filter_dict, "ID", hb_value_int(HB_FILTER_COLORSPACE));
            hb_dict_set(filter_dict, "Settings", filter_settings);
            hb_add_filter2(filter_list, filter_dict);
        }
        else
        {
            hb_value_free(&filter_settings);
        }
    }

    hb_value_t *fr_value = hb_dict_get(preset, "VideoFramerate");
    int vrate_den = get_video_framerate(fr_value);
    if (vrate_den < 0)
    {
        char *str = hb_value_get_string_xform(fr_value);
        hb_error("Invalid video framerate (%s)", str);
        free(str);
        return -1;
    }

    int fr_mode;
    hb_value_t *fr_mode_value = hb_dict_get(preset, "VideoFramerateMode");
    fr_mode = hb_value_type(fr_mode_value) == HB_VALUE_TYPE_STRING ? (
        !strcasecmp(hb_value_get_string(fr_mode_value), "cfr") ? 1 :
        !strcasecmp(hb_value_get_string(fr_mode_value), "pfr") ? 2 : 0) :
        hb_value_get_int(fr_mode_value);

    filter_settings = hb_dict_init();
    if (vrate_den == 0)
    {
        hb_dict_set(filter_settings, "mode", hb_value_int(fr_mode));
    }
    else
    {
        char *str = hb_strdup_printf("%d/%d", clock, vrate_den);
        hb_dict_set(filter_settings, "mode", hb_value_int(fr_mode));
        hb_dict_set(filter_settings, "rate", hb_value_string(str));
        free(str);
    }
    if (hb_validate_filter_settings(HB_FILTER_VFR, filter_settings))
    {
        hb_error("hb_preset_apply_filters: Internal error, invalid VFR");
        hb_value_free(&filter_settings);
        return -1;
    }

    filter_dict = hb_dict_init();
    hb_dict_set(filter_dict, "ID", hb_value_int(HB_FILTER_VFR));
    hb_dict_set(filter_dict, "Settings", filter_settings);
    hb_add_filter2(filter_list, filter_dict);
    return 0;
}

int hb_preset_apply_video(const hb_dict_t *preset, hb_dict_t *job_dict)
{
    hb_dict_t    *dest_dict, *video_dict, *qsv;
    hb_value_t   *value, *vcodec_value;
    int           mux, vcodec, vqtype, color_matrix_code;
    hb_encoder_t *encoder;

    dest_dict    = hb_dict_get(job_dict, "Destination");
    mux          = hb_container_get_from_name(hb_value_get_string(
                                                hb_dict_get(dest_dict, "Mux")));
    vcodec_value = hb_dict_get(preset, "VideoEncoder");
    if (hb_value_type(vcodec_value) == HB_VALUE_TYPE_STRING)
    {
        vcodec = hb_video_encoder_get_from_name(
                    hb_value_get_string(vcodec_value));
    }
    else
    {
        vcodec = hb_value_get_int(vcodec_value);
    }
    encoder = hb_video_encoder_get_from_codec(vcodec);
    if (encoder == NULL)
    {
        char *str = hb_value_get_string_xform(vcodec_value);
        hb_error("Invalid video encoder (%s)", str);
        free(str);
        return -1;
    }
    if (!(encoder->muxers & mux))
    {
        hb_error("Incompatible video encoder (%s) for muxer (%s)",
                  hb_video_encoder_get_name(vcodec),
                  hb_container_get_name(mux));
        return -1;
    }

    video_dict = hb_dict_get(job_dict, "Video");
    hb_dict_set(video_dict, "Encoder", hb_value_string(encoder->short_name));

    color_matrix_code = hb_value_get_int(hb_dict_get(preset, "VideoColorMatrixCodeOverride"));
    if (color_matrix_code != 0)
    {
        int color_prim, color_transfer, color_matrix;

        switch (color_matrix_code)
        {
            case 4:
                // ITU BT.2020 UHD content
                color_prim      = HB_COLR_PRI_BT2020;
                color_transfer  = HB_COLR_TRA_BT709;
                color_matrix    = HB_COLR_MAT_BT2020_NCL;
                break;
            case 3:
                // ITU BT.709 HD content
                color_prim      = HB_COLR_PRI_BT709;
                color_transfer  = HB_COLR_TRA_BT709;
                color_matrix    = HB_COLR_MAT_BT709;
                break;
            case 2:
                // ITU BT.601 DVD or SD TV content (PAL)
                color_prim      = HB_COLR_PRI_EBUTECH;
                color_transfer  = HB_COLR_TRA_BT709;
                color_matrix    = HB_COLR_MAT_SMPTE170M;
                break;
            case 1:
            default:
                // ITU BT.601 DVD or SD TV content (NTSC)
                color_prim      = HB_COLR_PRI_SMPTEC;
                color_transfer  = HB_COLR_TRA_BT709;
                color_matrix    = HB_COLR_MAT_SMPTE170M;
                break;
        }

        hb_dict_set(video_dict, "ColorPrimariesOverride",
                    hb_value_int(color_prim));
        hb_dict_set(video_dict, "ColorTransferOverride",
                    hb_value_int(color_transfer));
        hb_dict_set(video_dict, "ColorMatrixOverride",
                    hb_value_int(color_matrix));
    }
    hb_dict_set(video_dict, "Encoder", hb_value_dup(vcodec_value));

    if ((vcodec & HB_VCODEC_X264_MASK) &&
        hb_value_get_bool(hb_dict_get(preset, "x264UseAdvancedOptions")))
    {
        hb_dict_set(video_dict, "Options",
                    hb_value_dup(hb_dict_get(preset, "x264Option")));
    }
    else
    {
        if ((value = hb_dict_get(preset, "VideoPreset")) != NULL)
            hb_dict_set(video_dict, "Preset", hb_value_dup(value));
        if ((value = hb_dict_get(preset, "VideoProfile")) != NULL)
            hb_dict_set(video_dict, "Profile", hb_value_dup(value));
        if ((value = hb_dict_get(preset, "VideoLevel")) != NULL)
            hb_dict_set(video_dict, "Level", hb_value_dup(value));
        if ((value = hb_dict_get(preset, "VideoTune")) != NULL)
            hb_dict_set(video_dict, "Tune", hb_value_dup(value));
        if ((value = hb_dict_get(preset, "VideoOptionExtra")) != NULL)
            hb_dict_set(video_dict, "Options", hb_value_dup(value));
    }

    vqtype = hb_value_get_int(hb_dict_get(preset, "VideoQualityType"));
    if (vqtype == 2 && hb_video_quality_is_supported(vcodec))   // Constant quality
    {
        hb_dict_set(video_dict, "Quality",
                    hb_value_xform(hb_dict_get(preset, "VideoQualitySlider"),
                                   HB_VALUE_TYPE_DOUBLE));
        hb_dict_remove(video_dict, "Bitrate");
    }
    else if (vqtype == 1)   // ABR
    {
        hb_dict_set(video_dict, "Bitrate",
                    hb_value_xform(hb_dict_get(preset, "VideoAvgBitrate"),
                                   HB_VALUE_TYPE_INT));
        if (hb_video_multipass_is_supported(vcodec))
        {
            hb_dict_set(video_dict, "MultiPass",
                        hb_value_xform(hb_dict_get(preset, "VideoMultiPass"),
                                       HB_VALUE_TYPE_BOOL));
            hb_dict_set(video_dict, "Turbo",
                        hb_value_xform(hb_dict_get(preset, "VideoTurboMultiPass"),
                                       HB_VALUE_TYPE_BOOL));
        }
        hb_dict_remove(video_dict, "Quality");
    }
    else
    {
        value = hb_dict_get(preset, "VideoQualitySlider");
        if (value != NULL && hb_value_get_double(value) >= 0 && hb_video_quality_is_supported(vcodec))
        {
            hb_dict_set(video_dict, "Quality",
                        hb_value_xform(value, HB_VALUE_TYPE_DOUBLE));
            hb_dict_remove(video_dict, "Bitrate");
        }
        else
        {
            hb_dict_set(video_dict, "Bitrate",
                        hb_value_xform(hb_dict_get(preset, "VideoAvgBitrate"),
                                       HB_VALUE_TYPE_INT));
            if (hb_video_multipass_is_supported(vcodec))
            {
                hb_dict_set(video_dict, "MultiPass",
                            hb_value_xform(hb_dict_get(preset, "VideoMultiPass"),
                                           HB_VALUE_TYPE_BOOL));
                hb_dict_set(video_dict, "Turbo",
                            hb_value_xform(hb_dict_get(preset, "VideoTurboMultiPass"),
                                           HB_VALUE_TYPE_BOOL));
            }
            hb_dict_remove(video_dict, "Quality");
        }
    }
    
    if ((value = hb_dict_get(preset, "VideoHWDecode")) != NULL)
    {
        hb_dict_set(video_dict, "HardwareDecode", hb_value_xform(value, HB_VALUE_TYPE_INT));
    }
    
    qsv = hb_dict_get(video_dict, "QSV");
    if (qsv == NULL)
    {
        qsv = hb_dict_init();
        hb_dict_set(video_dict, "QSV", qsv);
        qsv = hb_dict_get(video_dict, "QSV");
    }
    if ((value = hb_dict_get(preset, "VideoQSVDecode")) != NULL)
    {
        hb_dict_set(qsv, "Decode",
                    hb_value_xform(value, HB_VALUE_TYPE_BOOL));
    }
    if ((value = hb_dict_get(preset, "VideoQSVAsyncDepth")) != NULL)
    {
        hb_dict_set(qsv, "AsyncDepth",
                    hb_value_xform(value, HB_VALUE_TYPE_INT));
    }
    if ((value = hb_dict_get(preset, "VideoQSVAdapterIndex")) != NULL)
    {
        hb_dict_set(qsv, "AdapterIndex",
                    hb_value_xform(value, HB_VALUE_TYPE_INT));
    }
    return 0;
}

int hb_preset_apply_mux(const hb_dict_t *preset, hb_dict_t *job_dict)
{
    hb_value_t *mux_value = hb_dict_get(preset, "FileFormat");
    int mux;
    if (hb_value_type(mux_value) == HB_VALUE_TYPE_STRING)
    {
        mux = hb_container_get_from_name(hb_value_get_string(mux_value));
        if (mux == 0)
            mux = hb_container_get_from_extension(
                                            hb_value_get_string(mux_value));
    }
    else
    {
        mux = hb_value_get_int(mux_value);
    }
    hb_container_t *container = hb_container_get_from_format(mux);
    if (container == NULL)
    {
        char *str = hb_value_get_string_xform(mux_value);
        hb_error("Invalid container (%s)", str);
        free(str);
        return -1;
    }

    hb_dict_t *dest_dict = hb_dict_get(job_dict, "Destination");
    hb_dict_set(dest_dict, "Mux", hb_value_string(container->short_name));

    hb_dict_set(dest_dict, "AlignAVStart",
                hb_value_xform(hb_dict_get(preset, "AlignAVStart"),
                               HB_VALUE_TYPE_BOOL));
    hb_dict_set(dest_dict, "InlineParameterSets",
                hb_value_xform(hb_dict_get(preset, "InlineParameterSets"),
                               HB_VALUE_TYPE_BOOL));
    if (mux)
    {
        hb_dict_t *options_dict = hb_dict_init();
        hb_dict_set(options_dict, "Optimize",
                    hb_value_xform(hb_dict_get(preset, "Optimize"),
                                   HB_VALUE_TYPE_BOOL));
        hb_dict_set(options_dict, "IpodAtom",
                    hb_value_xform(hb_dict_get(preset, "Mp4iPodCompatible"),
                                   HB_VALUE_TYPE_BOOL));
        hb_dict_set(dest_dict, "Options", options_dict);
    }

    return 0;
}


int hb_preset_apply_dimensions(hb_handle_t *h, int title_index,
                          const hb_dict_t *preset, hb_dict_t *job_dict)
{
    // Apply preset settings  that requires the title
    hb_title_t *title = hb_find_title_by_index(h, title_index);
    if (title == NULL)
        return -1;

    hb_dict_t        * filter_dict;
    hb_dict_t        * filters_dict = hb_dict_get(job_dict, "Filters");
    hb_value_array_t * filter_list = hb_dict_get(filters_dict, "FilterList");

    // Rotate filter settings must be known prior to applying crop/scale
    char * rotate = hb_value_get_string_xform(
                        hb_dict_get(preset, "PictureRotate"));
    hb_dict_t * rotate_settings = NULL;
    if (rotate != NULL)
    {
        rotate_settings = hb_generate_filter_settings(HB_FILTER_ROTATE,
                                                      NULL, NULL, rotate);
        if (rotate_settings == NULL)
        {
            hb_error("Invalid rotate filter settings (%s)", rotate);
            return -1;
        }
        else if (!hb_dict_get_bool(rotate_settings, "disable") &&
                 (hb_dict_get_int(rotate_settings, "angle") != 0 ||
                  hb_dict_get_bool(rotate_settings, "hflip")))
        {
            filter_dict = hb_dict_init();
            hb_dict_set(filter_dict, "ID", hb_value_int(HB_FILTER_ROTATE));
            hb_dict_set(filter_dict, "Settings", rotate_settings);
            hb_add_filter2(filter_list, filter_dict);
        }
        else
        {
            hb_value_free(&rotate_settings);
        }
        free(rotate);
    }

    // Calculate default job geometry settings
    hb_geometry_crop_t     srcGeo;
    hb_geometry_t          resultGeo;
    hb_geometry_settings_t geo;
    int keep_aspect, allow_upscaling, use_maximum_size;

    srcGeo.geometry = title->geometry;
    memcpy(srcGeo.crop, title->crop, sizeof(geo.crop));
    memset(geo.crop, 0, sizeof(geo.crop));

    if (rotate_settings != NULL)
    {
        int angle = hb_dict_get_int(rotate_settings, "angle");
        int hflip = hb_dict_get_bool(rotate_settings, "hflip");
        hb_rotate_geometry(&srcGeo, &srcGeo, angle, hflip);
    }
    
    switch(hb_dict_get_int(preset, "PictureCropMode")) 
    {
        case 0: // Automatic
          memcpy(geo.crop, srcGeo.crop, sizeof(geo.crop));
          break;
        case 1: // Loose
          memcpy(geo.crop, title->loose_crop, sizeof(geo.crop));
          break;
        case 2: // None
            geo.crop[0] = 0;
            geo.crop[1] = 0;
            geo.crop[2] = 0;
            geo.crop[3] = 0;
            break;
        case 3: // Custom
            geo.crop[0] = hb_dict_get_int(preset, "PictureTopCrop");
            geo.crop[1] = hb_dict_get_int(preset, "PictureBottomCrop");
            geo.crop[2] = hb_dict_get_int(preset, "PictureLeftCrop");
            geo.crop[3] = hb_dict_get_int(preset, "PictureRightCrop");
            break;
    }

    const char * pad_mode;
    pad_mode = hb_dict_get_string(preset, "PicturePadMode");
    if (pad_mode != NULL && !strcmp(pad_mode, "custom"))
    {
        geo.pad[0] = hb_value_get_int(hb_dict_get(preset, "PicturePadTop"));
        geo.pad[1] = hb_value_get_int(hb_dict_get(preset, "PicturePadBottom"));
        geo.pad[2] = hb_value_get_int(hb_dict_get(preset, "PicturePadLeft"));
        geo.pad[3] = hb_value_get_int(hb_dict_get(preset, "PicturePadRight"));
    }
    else
    {
        memset(geo.pad, 0, sizeof(geo.pad));
    }

    geo.modulus = hb_value_get_int(hb_dict_get(preset, "PictureModulus"));
    if (geo.modulus < 2)
        geo.modulus = 2;
    hb_value_t *ana_mode_value = hb_dict_get(preset, "PicturePAR");
    if (hb_value_type(ana_mode_value) == HB_VALUE_TYPE_STRING)
    {
        const char *s = hb_value_get_string(ana_mode_value);
        if (!strcasecmp(s, "off"))
            geo.mode = HB_ANAMORPHIC_NONE;
        else if (!strcasecmp(s, "strict"))
            geo.mode = HB_ANAMORPHIC_STRICT;
        else if (!strcasecmp(s, "custom"))
            geo.mode = HB_ANAMORPHIC_CUSTOM;
        else if (!strcasecmp(s, "auto"))
            geo.mode = HB_ANAMORPHIC_AUTO;
        else // default loose
            geo.mode = HB_ANAMORPHIC_LOOSE;
    }
    else
    {
        geo.mode = hb_value_get_int(hb_dict_get(preset, "PicturePAR"));
    }
    keep_aspect = hb_value_get_bool(hb_dict_get(preset, "PictureKeepRatio"));
    geo.keep = keep_aspect * HB_KEEP_DISPLAY_ASPECT;
    allow_upscaling = hb_dict_get_bool(preset, "PictureAllowUpscaling");
    use_maximum_size = hb_dict_get_bool(preset, "PictureUseMaximumSize");
    geo.flags = 0;
    geo.flags |= allow_upscaling  * HB_GEO_SCALE_UP;
    geo.flags |= use_maximum_size * HB_GEO_SCALE_BEST;

    geo.itu_par = hb_value_get_bool(hb_dict_get(preset, "PictureItuPAR"));
    geo.maxWidth = hb_value_get_int(hb_dict_get(preset, "PictureWidth"));
    geo.maxHeight = hb_value_get_int(hb_dict_get(preset, "PictureHeight"));
    geo.geometry = title->geometry;
    int width = hb_value_get_int(hb_dict_get(preset, "PictureForceWidth"));
    int height = hb_value_get_int(hb_dict_get(preset, "PictureForceHeight"));

    if (width > 0)
    {
        geo.geometry.width = width;
        geo.keep |= HB_KEEP_WIDTH;
    }
    if (height > 0)
    {
        geo.geometry.height = height;
        geo.keep |= HB_KEEP_HEIGHT;
    }

    geo.geometry.par.num = hb_dict_get_int(preset, "PicturePARWidth");
    geo.geometry.par.den = hb_dict_get_int(preset, "PicturePARHeight");

    int display_width = hb_dict_get_int(preset, "PictureDARWidth");
    if (display_width <= 0 && geo.geometry.par.den)
    {
        int cropped_width = geo.geometry.width - geo.crop[2] - geo.crop[3];
        display_width = ((double)geo.geometry.par.num / geo.geometry.par.den) * cropped_width + 0.5;
    }
    geo.displayWidth     = display_width;
    geo.displayHeight    = geo.geometry.height;

    hb_set_anamorphic_size2(&srcGeo.geometry, &geo, &resultGeo);

    hb_dict_t *par_dict = hb_dict_get(job_dict, "PAR");
    hb_dict_set(par_dict, "Num", hb_value_int(resultGeo.par.num));
    hb_dict_set(par_dict, "Den", hb_value_int(resultGeo.par.den));
    par_dict = NULL;

    hb_dict_t * scale_settings;

    scale_settings = hb_dict_init();
    hb_dict_set(scale_settings, "width", hb_value_int(resultGeo.width));
    hb_dict_set(scale_settings, "height", hb_value_int(resultGeo.height));
    hb_dict_set(scale_settings, "crop-top", hb_value_int(geo.crop[0]));
    hb_dict_set(scale_settings, "crop-bottom", hb_value_int(geo.crop[1]));
    hb_dict_set(scale_settings, "crop-left", hb_value_int(geo.crop[2]));
    hb_dict_set(scale_settings, "crop-right", hb_value_int(geo.crop[3]));
    if (hb_validate_filter_settings(HB_FILTER_CROP_SCALE, scale_settings))
    {
        hb_error("hb_preset_apply_dimensions: Internal error, invalid CROP_SCALE");
        hb_value_free(&scale_settings);
        goto fail;
    }

    filter_dict = hb_dict_init();
    hb_dict_set(filter_dict, "ID", hb_value_int(HB_FILTER_CROP_SCALE));
    hb_dict_set(filter_dict, "Settings", scale_settings);
    hb_add_filter2(filter_list, filter_dict);

    // Pad filter requires knowing crop/scale settings
    char       * pad_params = NULL;
    if (pad_mode != NULL && strcmp(pad_mode, "none"))
    {
        // Preset values used by the GUI and built-in presets
        int pad[4] = {0,};
        const char * color;

        if (!strcmp(pad_mode, "custom"))
        {
            pad[0] = hb_dict_get_int(preset, "PicturePadTop");
            pad[1] = hb_dict_get_int(preset, "PicturePadBottom");
            pad[2] = hb_dict_get_int(preset, "PicturePadLeft");
            pad[3] = hb_dict_get_int(preset, "PicturePadRight");
        }

        if (scale_settings != NULL)
        {
            int fillwidth, fillheight;

            int width     = hb_dict_get_int(scale_settings, "width");
            int height    = hb_dict_get_int(scale_settings, "height");
            int maxWidth  = hb_dict_get_int(preset, "PictureWidth");
            int maxHeight = hb_dict_get_int(preset, "PictureHeight");

            fillwidth  = fillheight  = !strcmp(pad_mode, "fill");
            fillheight = fillheight || !strcmp(pad_mode, "letterbox");
            fillwidth  = fillwidth  || !strcmp(pad_mode, "pillarbox");

            if (fillwidth && maxWidth > 0)
            {
                pad[2] = (maxWidth  - width) / 2;
                pad[3] =  maxWidth  - width - pad[2];
            }
            if (fillheight && maxHeight > 0)
            {
                pad[0] = (maxHeight - height) / 2;
                pad[1] =  maxHeight - height - pad[0];
            }
        }
        color  = hb_dict_get_string(preset, "PicturePadColor");
        if (color == NULL)
        {
            color = "black";
        }

        pad_params =
            hb_strdup_printf("top=%d:bottom=%d:left=%d:right=%d:color=%s",
                             pad[0], pad[1], pad[2], pad[3], color);
    }
    else
    {
        // Preset value used by the CLI
        pad_params =
            hb_value_get_string_xform(hb_dict_get(preset, "PicturePad"));
    }
    if (pad_params != NULL)
    {
        hb_dict_t * filter_settings;
        filter_settings = hb_generate_filter_settings(HB_FILTER_PAD,
                                                      NULL, NULL, pad_params);
        if (filter_settings == NULL)
        {
            hb_error("Invalid pad filter settings (%s)", pad_params);
            return -1;
        }
        else if (!hb_dict_get_bool(filter_settings, "disable"))
        {
            filter_dict = hb_dict_init();
            hb_dict_set(filter_dict, "ID", hb_value_int(HB_FILTER_PAD));
            hb_dict_set(filter_dict, "Settings", filter_settings);
            hb_add_filter2(filter_list, filter_dict);
        }
        else
        {
            hb_value_free(&filter_settings);
        }
        free(pad_params);
    }

    return 0;

fail:
    return -1;
}

int hb_preset_apply_title(hb_handle_t *h, int title_index,
                          const hb_dict_t *preset, hb_dict_t *job_dict)
{
    // Apply preset settings  that requires the title
    hb_title_t *title = hb_find_title_by_index(h, title_index);
    if (title == NULL)
        return -1;

    int chapters = hb_value_get_bool(hb_dict_get(preset, "ChapterMarkers"));
    if (hb_list_count(title->list_chapter) <= 1)
        chapters = 0;

    // Set "Destination" settings in job
    hb_dict_t *dest_dict = hb_dict_get(job_dict, "Destination");
    hb_dict_set(dest_dict, "ChapterMarkers", hb_value_bool(chapters));

    // Audio settings
    if (hb_preset_job_add_audio(h, title_index, preset, job_dict) != 0)
    {
        goto fail;
    }

    // Subtitle settings
    if (hb_preset_job_add_subtitles(h, title_index, preset, job_dict) != 0)
    {
        goto fail;
    }
    return 0;

fail:
    return -1;
}

/**
 * Initialize an hb_job_t and return a hb_dict_t representation of the job.
 * This dict will have key/value pairs compatible with json jobs.
 * @param h             - Pointer to hb_handle_t instance that contains the
 *                        specified title_index
 * @param title_index   - Index of hb_title_t to use for job initialization.
 *                        Index comes from title->index or "Index" key
 *                        in json representation of a title.
 * @param preset        - Preset to initialize job with
 */
hb_dict_t* hb_preset_job_init(hb_handle_t *h, int title_index,
                              const hb_dict_t *preset)
{
    hb_title_t *title = hb_find_title_by_index(h, title_index);
    if (title == NULL)
    {
        hb_error("Invalid title index (%d)", title_index);
        return NULL;
    }

    hb_job_t *job = hb_job_init(title);
    hb_dict_t *job_dict = hb_job_to_dict(job);
    hb_job_close(&job);

    if (hb_preset_apply_mux(preset, job_dict) < 0)
        goto fail;

    if (hb_preset_apply_video(preset, job_dict) < 0)
        goto fail;

    if (hb_preset_apply_dimensions(h, title_index, preset, job_dict) < 0)
        goto fail;

    if (hb_preset_apply_filters(preset, job_dict) < 0)
        goto fail;

    if (hb_preset_apply_title(h, title_index, preset, job_dict) < 0)
        goto fail;

    return job_dict;

fail:
    hb_value_free(&job_dict);
    return NULL;
}

// Clean a dictionary of unwanted keys
// Used to make sure only valid keys are in output presets
static void
dict_clean(hb_value_t *dict, hb_value_t *template)
{
    hb_value_t *tmp = hb_value_dup(dict);
    hb_dict_iter_t iter;
    const char *key;
    hb_value_t *val;
    hb_value_t *template_val;
    hb_value_type_t template_type, val_type;
    const char *preset_name = NULL;

    val = hb_dict_get(dict, "PresetName");
    if (val != NULL)
        preset_name = hb_value_get_string(val);

    // Remove keys that are not in the template and translate compatible
    // data types to the types used in the template.
    for (iter = hb_dict_iter_init(tmp);
         iter != HB_DICT_ITER_DONE;
         iter = hb_dict_iter_next(tmp, iter))
    {
        key = hb_dict_iter_key(iter);
        val = hb_dict_iter_value(iter);
        val_type = hb_value_type(val);

        template_val = hb_dict_get(template, key);
        template_type = hb_value_type(template_val);
        if (template_val == NULL)
        {
            // Unknown key.  These can be keys used privately by the
            // frontend.  So don't make noise about them.
            hb_dict_remove(dict, key);
        }
        else if (val_type != template_type)
        {
            if (val_type      == HB_VALUE_TYPE_DICT  ||
                val_type      == HB_VALUE_TYPE_ARRAY ||
                template_type == HB_VALUE_TYPE_DICT  ||
                template_type == HB_VALUE_TYPE_ARRAY)
            {
                hb_error("Preset %s: Incompatible value types for key %s. "
                         "Dropping.", preset_name, key);
                hb_dict_remove(dict, key);
            }
            else if (hb_value_is_number(val) &&
                     hb_value_is_number(template_val))
            {
                // Silently convert compatible numbers
                hb_value_t *v;
                v = hb_value_xform(val, template_type);
                hb_dict_set(dict, key, v);
            }
            else
            {
                hb_value_t *v;
                hb_error("Preset %s: Incorrect value type for key %s. "
                         "Converting.", preset_name, key);
                v = hb_value_xform(val, template_type);
                hb_dict_set(dict, key, v);
            }
        }
        else if (val_type      == HB_VALUE_TYPE_DICT &&
                 template_type == HB_VALUE_TYPE_DICT)
        {
            val = hb_dict_get(dict, key);
            dict_clean(val, template_val);
        }
        else if (val_type      == HB_VALUE_TYPE_ARRAY &&
                 template_type == HB_VALUE_TYPE_ARRAY &&
                 hb_value_array_len(template_val) > 0)
        {
            template_val = hb_value_array_get(template_val, 0);
            if (hb_value_type(template_val) == HB_VALUE_TYPE_DICT)
            {
                val = hb_dict_get(dict, key);
                int count = hb_value_array_len(val);
                int ii;
                for (ii = 0; ii < count; ii++)
                {
                    hb_value_t *array_val;
                    array_val = hb_value_array_get(val, ii);
                    if (hb_value_type(array_val) == HB_VALUE_TYPE_DICT)
                    {
                        dict_clean(array_val, template_val);
                    }
                }
            }
        }
    }
    hb_value_free(&tmp);

    if (!hb_value_get_bool(hb_dict_get(dict, "Folder")))
    {
        // Add key/value pairs that are in the template but not in the dict.
        for (iter = hb_dict_iter_init(template);
             iter != HB_DICT_ITER_DONE;
             iter = hb_dict_iter_next(template, iter))
        {
            key          = hb_dict_iter_key(iter);
            template_val = hb_dict_iter_value(iter);
            val          = hb_dict_get(dict, key);
            if (val == NULL)
            {
                hb_dict_set(dict, key, hb_value_dup(template_val));
            }
        }
    }
}

static void preset_clean(hb_value_t *preset, hb_value_t *template)
{
    dict_clean(preset, template);

    // Check for proper "short name" values.
    // Convert as necessary.
    hb_value_t *val;
    const char *preset_name = NULL;
    int muxer;

    val = hb_dict_get(preset, "PresetName");
    if (val != NULL)
        preset_name = hb_value_get_string(val);

    val = hb_dict_get(preset, "FileFormat");
    if (val != NULL)
    {
        const char *s, *mux;
        s = hb_value_get_string(val);
        muxer = hb_container_get_from_name(s);
        if (muxer == HB_MUX_INVALID)
        {
            const hb_container_t *c = hb_container_get_next(NULL);
            muxer = c->format;
            hb_error("Preset %s: Invalid container (%s)", preset_name, s);
        }
        mux = hb_container_get_short_name(muxer);
        val = hb_value_string(mux);
        hb_dict_set(preset, "FileFormat", val);
    }
    else
    {
        const hb_container_t *c = hb_container_get_next(NULL);
        muxer = c->format;
    }
    val = hb_dict_get(preset, "VideoEncoder");
    if (val != NULL)
    {
        const char *s, *enc;
        int vcodec;
        s = hb_value_get_string(val);
        vcodec = hb_video_encoder_get_from_name(s);
        if (vcodec == HB_VCODEC_INVALID)
        {
            vcodec = hb_video_encoder_get_default(muxer);
            hb_error("Preset %s: Invalid video encoder (%s)", preset_name, s);
        }
        enc = hb_video_encoder_get_short_name(vcodec);
        val = hb_value_string(enc);
        hb_dict_set(preset, "VideoEncoder", val);

        int disabled = hb_video_encoder_is_supported(vcodec) == 0;
        hb_dict_set_bool(preset, "PresetDisabled", disabled);
    }
    val = hb_dict_get(preset, "VideoFramerate");
    if (val != NULL)
    {
        const char *s;
        s = hb_value_get_string(val);
        if (strcasecmp(s, "auto"))
        {
            int fr = hb_video_framerate_get_from_name(s);
            if (fr < 0)
            {
                if (strcasecmp(s, "same as source"))
                {
                    hb_error("Preset %s: Invalid video framerate (%s)",
                             preset_name, s);
                }
                val = hb_value_string("auto");
                hb_dict_set(preset, "VideoFramerate", val);
            }
        }
    }
    val = hb_dict_get(preset, "AudioEncoderFallback");
    if (val != NULL)
    {
        const char *s, *enc;
        int acodec;
        s = hb_value_get_string(val);
        acodec = hb_audio_encoder_get_from_name(s);
        if (acodec == HB_ACODEC_INVALID)
        {
            acodec = hb_audio_encoder_get_default(muxer);
            hb_error("Preset %s: Invalid audio fallback encoder (%s)",
                     preset_name, s);
        }
        enc = hb_audio_encoder_get_short_name(acodec);
        val = hb_value_string(enc);
        hb_dict_set(preset, "AudioEncoderFallback", val);
    }
    hb_value_t *alist = hb_dict_get(preset, "AudioList");
    int count = hb_value_array_len(alist);
    int ii;
    for (ii = 0; ii < count; ii++)
    {
        hb_value_t *adict = hb_value_array_get(alist, ii);
        val = hb_dict_get(adict, "AudioEncoder");
        if (val != NULL)
        {
            const char *s, *enc;
            int acodec;
            s = hb_value_get_string(val);
            acodec = hb_audio_encoder_get_from_name(s);
            if (acodec == HB_ACODEC_INVALID)
            {
                acodec = hb_audio_encoder_get_default(muxer);
                hb_error("Preset %s: Invalid audio encoder (%s)",
                         preset_name, s);
            }
            enc = hb_audio_encoder_get_short_name(acodec);
            val = hb_value_string(enc);
            hb_dict_set(adict, "AudioEncoder", val);
        }
        val = hb_dict_get(adict, "AudioSamplerate");
        if (val != NULL)
        {
            const char *s;
            s = hb_value_get_string(val);
            if (strcasecmp(s, "auto"))
            {
                int sr = hb_audio_samplerate_get_from_name(s);
                if (sr < 0)
                {
                    hb_error("Preset %s: Invalid audio samplerate (%s)",
                             preset_name, s);
                    val = hb_value_string("auto");
                    hb_dict_set(adict, "AudioSamplerate", val);
                }
            }
        }
        val = hb_dict_get(adict, "AudioMixdown");
        if (val != NULL)
        {
            const char *s, *mix;
            s = hb_value_get_string(val);
            int mixdown = hb_mixdown_get_from_name(s);
            if (mixdown == HB_INVALID_AMIXDOWN)
            {
                // work.c do_job() sanitizes NONE to default mixdown
                mixdown = HB_AMIXDOWN_NONE;
                hb_error("Preset %s: Invalid audio mixdown (%s)",
                         preset_name, s);
            }
            mix = hb_mixdown_get_short_name(mixdown);
            val = hb_value_string(mix);
            hb_dict_set(adict, "AudioMixdown", val);
        }
    }
}

static void presets_clean(hb_value_t *presets, hb_value_t *template)
{
    preset_clean_context_t ctx;
    ctx.do_ctx.path.depth = 1;
    ctx.template = template;
    presets_do(do_preset_clean, presets, (preset_do_context_t*)&ctx);
}

void hb_presets_clean(hb_value_t *preset)
{
    presets_clean(preset, hb_preset_template);
}

static char * fix_name_collisions(hb_value_t * list, const char * name)
{
    char * new_name = strdup(name);
    int    ii, jj, count;

    count = hb_value_array_len(list);
    for (ii = 0, jj = 0; ii < count; ii++)
    {
        hb_value_t * item = hb_value_array_get(list, ii);
        const char * preset_name;

        preset_name = hb_dict_get_string(item, "PresetName");
        if (!strcmp(new_name, preset_name))
        {
            // Collision, add a number and try again
            free(new_name);
            new_name = hb_strdup_printf("%s - %d", name, jj++);
            ii = -1;
        }
    }
    return new_name;
}

static void import_folder_hierarchy_29_0_0(const char * name,
                                    hb_value_t * new_list, hb_value_t * folder)
{
    hb_value_t * list = hb_dict_get(folder, "ChildrenArray");
    int          ii, count;

    count = hb_value_array_len(list);
    for (ii = 0; ii < count;)
    {
        hb_value_t * item = hb_value_array_get(list, ii);

        if (hb_dict_get_bool(item, "Folder"))
        {
            const char * folder_name;
            char       * new_name;
            int          pos = hb_value_array_len(new_list);

            folder_name = hb_dict_get_string(item, "PresetName");
            new_name = hb_strdup_printf("%s - %s", name, folder_name);
            import_folder_hierarchy_29_0_0(new_name, new_list, item);

            hb_value_t * children = hb_dict_get(item, "ChildrenArray");
            if (hb_value_array_len(children) > 0)
            {
                // If the folder has any children, move it to the
                // top level folder list.
                char * unique_name = fix_name_collisions(new_list, new_name);
                hb_dict_set_string(item, "PresetName", unique_name);
                hb_value_incref(item);
                hb_value_array_remove(list, ii);
                hb_value_array_insert(new_list, pos, item);
                free(unique_name);
            }
            else
            {
                hb_value_array_remove(list, ii);
            }
            free(new_name);
        }
        else
        {
            ii++;
        }
    }
}

static hb_value_t * import_hierarchy_29_0_0(hb_value_t *presets)
{
    hb_value_t * list = presets;
    hb_value_t * my_presets = NULL;
    hb_value_t * my_presets_list;
    hb_value_t * new_list;
    int          ii, count;

    if (hb_value_type(presets) == HB_VALUE_TYPE_DICT &&
        hb_dict_get(presets, "VersionMajor") != NULL)
    {
        // A packaged preset list
        list = hb_dict_get(presets, "PresetList");
    }

    // Copy official presets to new list
    new_list = hb_value_array_init();
    count = hb_value_array_len(list);
    for (ii = 0; ii < count; ii++)
    {
        hb_value_t * item = hb_value_array_get(list, ii);
        if (hb_dict_get_int(item, "Type") != HB_PRESET_TYPE_OFFICIAL)
        {
            continue;
        }
        hb_value_array_append(new_list, hb_value_dup(item));
    }

    // First process any custom folder named "My Presets".
    // Any existing "My Presets" folder is sanitized for subfolders.
    // If "My Presets" doesn't exist, one is created.
    count = hb_value_array_len(list);
    for (ii = 0; ii < count; ii++)
    {
        hb_value_t * item = hb_value_array_get(list, ii);

        if (hb_dict_get_int(item, "Type") == HB_PRESET_TYPE_OFFICIAL)
        {
            // Skip official presets.  The don't need to be restructured.
            continue;
        }
        if (hb_dict_get_bool(item, "Folder"))
        {
            int          pos = hb_value_array_len(new_list);
            const char * name = hb_dict_get_string(item, "PresetName");
            if (strcmp(name, "My Presets"))
            {
                continue;
            }
            import_folder_hierarchy_29_0_0(name, new_list, item);

            my_presets = hb_value_dup(item);
            hb_value_array_insert(new_list, pos, my_presets);
            hb_value_array_remove(list, ii);
            break;
        }
    }
    if (my_presets == NULL)
    {
        my_presets = hb_dict_init();
        hb_dict_set_string(my_presets, "PresetName", "My Presets");
        hb_dict_set(my_presets, "ChildrenArray", hb_value_array_init());
        hb_dict_set_int(my_presets, "Type", HB_PRESET_TYPE_CUSTOM);
        hb_dict_set_bool(my_presets, "Folder", 1);
        hb_value_array_append(new_list, my_presets);
    }
    my_presets_list = hb_dict_get(my_presets, "ChildrenArray");

    // Sanitize all other custom folders
    count = hb_value_array_len(list);
    for (ii = 0; ii < count; ii++)
    {
        hb_value_t * item = hb_value_array_get(list, ii);

        if (hb_dict_get_int(item, "Type") == HB_PRESET_TYPE_OFFICIAL)
        {
            // Skip official presets.  The don't need to be restructured.
            continue;
        }
        if (hb_dict_get_bool(item, "Folder"))
        {
            int          pos  = hb_value_array_len(new_list);
            const char * name = hb_dict_get_string(item, "PresetName");
            import_folder_hierarchy_29_0_0(name, new_list, item);

            hb_value_t * children = hb_dict_get(item, "ChildrenArray");
            if (hb_value_array_len(children) > 0)
            {
                // If the folder has any children, move it to the
                // top level folder list.
                char * unique_name = fix_name_collisions(new_list, name);
                hb_dict_set_string(item, "PresetName", unique_name);
                hb_value_array_insert(new_list, pos, hb_value_dup(item));
                free(unique_name);
            }
        }
        else
        {
            hb_value_array_append(my_presets_list, hb_value_dup(item));
        }
    }

    if (hb_value_type(presets) == HB_VALUE_TYPE_DICT &&
        hb_dict_get(presets, "VersionMajor") != NULL)
    {
        // A packaged preset list
        hb_dict_set(presets, "PresetList", new_list);
    }
    else
    {
        presets = new_list;
    }
    return hb_value_dup(presets);
}

static void und_to_any(hb_value_array_t * list)
{
    if (list == NULL)
    {
        return;
    }

    int count = hb_value_array_len(list);
    int ii;
    for (ii = 0; ii < count; ii++)
    {
        const char *lang;
        lang = hb_value_get_string(hb_value_array_get(list, ii));
        if (!strcasecmp(lang, "und"))
        {
            hb_value_array_set(list, ii, hb_value_string("any"));
        }
    }
}

static void import_container_settings_51_0_0(hb_value_t *preset)
{
    int optimize = hb_dict_get_bool(preset, "Mp4HttpOptimize");
    hb_dict_set_bool(preset, "Optimize", optimize);
}

static void import_video_pass_settings_50_0_0(hb_value_t *preset)
{
    int two_pass = hb_dict_get_bool(preset, "VideoTwoPass");
    int turbo_two_pass = hb_dict_get_bool(preset, "VideoTurboTwoPass");
    hb_dict_set_bool(preset, "VideoMultiPass", two_pass);
    hb_dict_set_bool(preset, "VideoTurboMultiPass", turbo_two_pass);
}

static void import_pic_settings_47_0_0(hb_value_t *preset)
{
    int auto_crop = hb_dict_get_bool(preset, "PictureAutoCrop");
    int ct = hb_dict_get_int(preset, "PictureTopCrop");
    int cb = hb_dict_get_int(preset, "PictureBottomCrop");
    int cl = hb_dict_get_int(preset, "PictureLeftCrop");
    int cr = hb_dict_get_int(preset, "PictureRightCrop");

    if (auto_crop)
    {
        hb_dict_set_int(preset, "PictureCropMode", 0);
    }
    else
    {
        if (ct == 0 && cb == 0 && cl == 0 && cr == 0)
        {
            hb_dict_set_int(preset, "PictureCropMode", 2);
        }
        else
        {
            hb_dict_set_int(preset, "PictureCropMode", 3);
        }
    }
}

static void import_pic_settings_44_0_0(hb_value_t *preset)
{
    int uses_pic = hb_dict_get_int(preset, "UsesPictureSettings");

    if (uses_pic != 1)
    {
        hb_dict_set_int(preset, "PictureWidth", 0);
        hb_dict_set_int(preset, "PictureHeight", 0);
    }
    hb_dict_remove(preset, "UsesPictureSettings");

    const char *pic_par = hb_dict_get_string(preset, "PicturePAR");
    if (pic_par == NULL || !strcasecmp(pic_par, "loose"))
    {
        hb_dict_set(preset, "PicturePAR", hb_value_string("auto"));
    }
}

static void import_lang_list_40_0_0(hb_value_t *preset)
{
    hb_value_array_t * lang_list;

    lang_list = hb_dict_get(preset, "AudioLanguageList");
    und_to_any(lang_list);
    lang_list = hb_dict_get(preset, "SubtitleLanguageList");
    und_to_any(lang_list);
}

static void import_deblock_35_0_0(hb_value_t *preset)
{
    int deblock = hb_dict_get_int(preset, "PictureDeblock");

    if (deblock < 5)
    {
        hb_dict_set_string(preset, "PictureDeblockPreset", "off");
    }
    else if (deblock < 7)
    {
        hb_dict_set_string(preset, "PictureDeblockPreset", "medium");
    }
    else
    {
        hb_dict_set_string(preset, "PictureDeblockPreset", "strong");
    }
    hb_dict_set_string(preset, "PictureDeblockTune", "medium");
    hb_dict_set_string(preset, "PictureDeblockCustom",
                       "strength=strong:thresh=20");
    hb_dict_remove(preset, "PictureDeblock");
}

static void import_video_scaler_25_0_0(hb_value_t *preset)
{
    hb_dict_set(preset, "VideoScaler", hb_value_string("swscale"));
}

static void import_anamorphic_20_0_0(hb_value_t *preset)
{
    hb_value_t *val = hb_dict_get(preset, "PicturePAR");
    if (hb_value_type(val) == HB_VALUE_TYPE_STRING)
    {
        const char *s = hb_value_get_string(val);
        if (!strcasecmp(s, "strict"))
        {
            hb_dict_set(preset, "PicturePAR", hb_value_string("loose"));
            hb_dict_set(preset, "UsesPictureSettings", hb_value_int(2));
            hb_dict_set(preset, "PictureModulus", hb_value_int(2));
        }
    }
}

static void import_custom_11_1_0(hb_value_t * preset, int filter_id,
                                 const char * key)
{
    char *str = hb_value_get_string_xform(hb_dict_get(preset, key));
    if (str == NULL)
    {
        return;
    }
    hb_filter_object_t * filter = hb_filter_get(filter_id);
    if (filter == NULL)
    {
        hb_log("import_custom_11_1_0: invalid filter id %d\n", filter_id);
        return;
    }
    if (filter->settings_template == NULL)
    {
        return;
    }
    char ** values = hb_str_vsplit(str, ':');
    char ** tmpl   = hb_str_vsplit(filter->settings_template, ':');
    int     ii;

    free(str);
    hb_dict_t * dict = hb_dict_init();
    for (ii = 0; values[ii] != NULL; ii++)
    {
        if (tmpl[ii] == NULL)
        {
            // Incomplete template?
            break;
        }
        char ** pair = hb_str_vsplit(tmpl[ii], '=');
        if (pair[0] != NULL)
        {
            hb_dict_set(dict, pair[0], hb_value_string(values[ii]));
        }
        hb_str_vfree(pair);
    }
    hb_str_vfree(tmpl);
    hb_str_vfree(values);

    char * result = hb_filter_settings_string(filter_id, dict);
    hb_dict_set(preset, key, hb_value_string(result));
    free(result);
}

static void import_filters_11_1_0(hb_value_t *preset)
{
    hb_value_t *val = hb_dict_get(preset, "PictureDeinterlaceFilter");
    if (val != NULL)
    {
        const char * str = hb_value_get_string(val);
        if (str != NULL)
        {
            if (strcasecmp(str, "deinterlace"))
            {
                import_custom_11_1_0(preset, HB_FILTER_YADIF,
                                     "PictureDeinterlaceCustom");
            }
            else if (strcasecmp(str, "decomb"))
            {
                import_custom_11_1_0(preset, HB_FILTER_DECOMB,
                                     "PictureDeinterlaceCustom");
            }
        }
    }
    val = hb_dict_get(preset, "PictureDenoiseFilter");
    if (val != NULL)
    {
        const char * str = hb_value_get_string(val);
        if (str != NULL)
        {
            if (strcasecmp(str, "hqdn3d"))
            {
                import_custom_11_1_0(preset, HB_FILTER_HQDN3D,
                                     "PictureDenoiseCustom");
            }
            else if (strcasecmp(str, "nlmeans"))
            {
                import_custom_11_1_0(preset, HB_FILTER_NLMEANS,
                                     "PictureDenoiseCustom");
            }
        }
    }
    import_custom_11_1_0(preset, HB_FILTER_DETELECINE,
                         "PictureDetelecineCustom");
    import_custom_11_1_0(preset, HB_FILTER_ROTATE,
                         "PictureRotate");
}

// Split the old decomb filter into separate comb detection
// and decomb filters.
static void import_deint_12_0_0(hb_value_t *preset)
{
    hb_value_t *val = hb_dict_get(preset, "PictureDeinterlaceFilter");
    if (val == NULL)
    {
        return;
    }
    const char * deint = hb_value_get_string(val);
    if (deint == NULL)
    {
        // This really shouldn't happen for a valid preset
        return;
    }
    if (strcasecmp(deint, "decomb"))
    {
        return;
    }
    val = hb_dict_get(preset, "PictureDeinterlacePreset");
    if (val == NULL)
    {
        hb_dict_set(preset, "PictureDeinterlacePreset",
                    hb_value_string("default"));
        return;
    }
    deint = hb_value_get_string(val);
    if (deint == NULL)
    {
        // This really shouldn't happen for a valid preset
        return;
    }
    if (!strcasecmp(deint, "fast"))
    {
        // fast -> PictureCombDetectPreset fast
        //         PictureDeinterlacePreset default
        hb_dict_set(preset, "PictureCombDetectPreset",
                    hb_value_string("fast"));
        hb_dict_set(preset, "PictureDeinterlacePreset",
                    hb_value_string("default"));
        return;
    }
    else if (!strcasecmp(deint, "bob") || !strcasecmp(deint, "default"))
    {
        hb_dict_set(preset, "PictureCombDetectPreset",
                    hb_value_string("default"));
        return;
    }
    else if (strcasecmp(deint, "custom"))
    {
        // not custom -> default
        hb_dict_set(preset, "PictureCombDetectPreset",
                    hb_value_string("default"));
        hb_dict_set(preset, "PictureDeinterlacePreset",
                    hb_value_string("default"));
        return;
    }
    val = hb_dict_get(preset, "PictureDeinterlaceCustom");
    if (val == NULL)
    {
        hb_dict_set(preset, "PictureDeinterlacePreset",
                    hb_value_string("default"));
        return;
    }
    // Translate custom values
    deint = hb_value_get_string(val);
    if (deint == NULL)
    {
        // This really shouldn't happen for a valid preset
        return;
    }

    hb_dict_t * dict;
    dict = hb_parse_filter_settings(deint);

    int yadif, blend, cubic, eedi2, mask, bob, gamma, filter, composite;
    int detect_mode, decomb_mode;

    int mode = 7, spatial_metric = 2, motion_threshold = 3;
    int spatial_threshold = 3, filter_mode = 2;
    int block_threshold = 40, block_width = 16, block_height = 16;
    int magnitude_threshold = 10, variance_threshold = 20;
    int laplacian_threshold = 20;
    int dilation_threshold = 4, erosion_threshold = 2, noise_threshold = 50;
    int maximum_search_distance = 24, post_processing = 1, parity = -1;

    hb_dict_extract_int(&mode, dict, "mode");
    hb_dict_extract_int(&spatial_metric, dict, "spatial-metric");
    hb_dict_extract_int(&motion_threshold, dict, "motion-thresh");
    hb_dict_extract_int(&spatial_threshold, dict, "spatial-thresh");
    hb_dict_extract_int(&filter_mode, dict, "filter-mode");
    hb_dict_extract_int(&block_threshold, dict, "block-thresh");
    hb_dict_extract_int(&block_width, dict, "block-width");
    hb_dict_extract_int(&block_height, dict, "block-height");
    hb_dict_extract_int(&magnitude_threshold, dict, "magnitude-thresh");
    hb_dict_extract_int(&variance_threshold, dict, "variance-thresh");
    hb_dict_extract_int(&laplacian_threshold, dict, "laplacian-thresh");
    hb_dict_extract_int(&dilation_threshold, dict, "dilation-thresh");
    hb_dict_extract_int(&erosion_threshold, dict, "erosion-thresh");
    hb_dict_extract_int(&noise_threshold, dict, "noise-thresh");
    hb_dict_extract_int(&maximum_search_distance, dict, "search-distance");
    hb_dict_extract_int(&post_processing, dict, "postproc");
    hb_dict_extract_int(&parity, dict, "parity");
    hb_value_free(&dict);

    yadif     = !!(mode & 1);
    blend     = !!(mode & 2);
    cubic     = !!(mode & 4);
    eedi2     = !!(mode & 8);
    mask      = !!(mode & 32);
    bob       = !!(mode & 64);
    gamma     = !!(mode & 128);
    filter    = !!(mode & 256);
    composite = !!(mode & 512);

    detect_mode = gamma + filter * 2 + mask * 4 + composite * 8;
    decomb_mode = yadif + blend * 2 + cubic * 4 + eedi2 * 8 + bob * 16;

    char * custom = hb_strdup_printf("mode=%d:spatial-metric=%d:"
                                     "motion-thresh=%d:spatial-thresh=%d:"
                                     "filter-mode=%d:block-thresh=%d:"
                                     "block-width=%d:block-height=%d",
                                     detect_mode, spatial_metric,
                                     motion_threshold, spatial_threshold,
                                     filter_mode, block_threshold,
                                     block_width, block_height);
    hb_dict_set(preset, "PictureCombDetectCustom", hb_value_string(custom));
    free(custom);

    custom = hb_strdup_printf("mode=%d:magnitude-thresh=%d:variance-thresh=%d:"
                              "laplacian-thresh=%d:dilation-thresh=%d:"
                              "erosion-thresh=%d:noise-thresh=%d:"
                              "search-distance=%d:postproc=%d:parity=%d",
                              decomb_mode, magnitude_threshold,
                              variance_threshold, laplacian_threshold,
                              dilation_threshold, erosion_threshold,
                              noise_threshold, maximum_search_distance,
                              post_processing, parity);
    hb_dict_set(preset, "PictureDeinterlaceCustom", hb_value_string(custom));
    free(custom);
}

static void import_deint_11_0_0(hb_value_t *preset)
{
    hb_value_t *val = hb_dict_get(preset, "PictureDeinterlaceFilter");
    if (val == NULL)
    {
        return;
    }
    const char * deint = hb_value_get_string(val);
    if (deint == NULL)
    {
        // This really shouldn't happen for a valid preset
        return;
    }
    if (strcasecmp(deint, "deinterlace"))
    {
        return;
    }
    val = hb_dict_get(preset, "PictureDeinterlacePreset");
    if (val == NULL)
    {
        hb_dict_set(preset, "PictureDeinterlacePreset",
                    hb_value_string("default"));
        return;
    }
    deint = hb_value_get_string(val);
    if (deint == NULL)
    {
        // This really shouldn't happen for a valid preset
        return;
    }
    if (!strcasecmp(deint, "fast") || !strcasecmp(deint, "slow"))
    {
        // fast and slow -> skip-spatial
        hb_dict_set(preset, "PictureDeinterlacePreset",
                    hb_value_string("skip-spatial"));
        return;
    }
    else if (!strcasecmp(deint, "bob") || !strcasecmp(deint, "default"))
    {
        return;
    }
    else if (strcasecmp(deint, "custom"))
    {
        // not custom -> default
        hb_dict_set(preset, "PictureDeinterlacePreset",
                    hb_value_string("default"));
        return;
    }
    val = hb_dict_get(preset, "PictureDeinterlaceCustom");
    if (val == NULL)
    {
        hb_dict_set(preset, "PictureDeinterlacePreset",
                    hb_value_string("default"));
        return;
    }
    // Translate custom values
    deint = hb_value_get_string(val);
    if (deint == NULL)
    {
        // This really shouldn't happen for a valid preset
        return;
    }
    int bob, spatial, yadif, mode = 3, parity = -1;
    sscanf(deint, "%d:%d", &mode, &parity);
    yadif   = !!(mode & 1);
    spatial = !!(mode & 2);
    bob     = !!(mode & 8);
    mode = yadif + (yadif && spatial) * 2 + bob * 4;
    char * custom = hb_strdup_printf("%d:%d", mode, parity);
    hb_dict_set(preset, "PictureDeinterlaceCustom", hb_value_string(custom));
    free(custom);
}

static void import_deint_10_0_0(hb_value_t *preset)
{
    hb_value_t *val = hb_dict_get(preset, "PictureDecombDeinterlace");
    if (val != NULL)
    {
        int decomb_or_deint = hb_value_get_bool(val);
        const char * deint_preset;
        if (decomb_or_deint)
        {
            deint_preset = hb_value_get_string(
                                hb_dict_get(preset, "PictureDecomb"));
        }
        else
        {
            deint_preset = hb_value_get_string(
                                hb_dict_get(preset, "PictureDeinterlace"));
        }
        if (deint_preset != NULL && strcasecmp(deint_preset, "off"))
        {
            hb_dict_set(preset, "PictureDeinterlaceFilter",
                        decomb_or_deint ? hb_value_string("decomb")
                                        : hb_value_string("deinterlace"));
            hb_dict_set(preset, "PictureDeinterlacePreset",
                        hb_value_string(deint_preset));
        }
        else
        {
            hb_dict_set(preset, "PictureDeinterlaceFilter",
                        hb_value_string("off"));
            hb_dict_set(preset, "PictureDeinterlacePreset",
                        hb_value_string("default"));
        }
    }
}

static const char* import_indexed_filter(int filter_id, int index)
{
    hb_filter_param_t *filter_presets;
    filter_presets = hb_filter_param_get_presets(filter_id);

    int ii;
    for (ii = 0; filter_presets[ii].name != NULL; ii++)
    {
        if (filter_presets[ii].index == index)
            break;
    }
    return filter_presets[ii].short_name;
}

static void import_deint_0_0_0(hb_value_t *preset)
{
    hb_value_t *val = hb_dict_get(preset, "PictureDecomb");
    if (hb_value_is_number(val))
    {
        const char *s;
        int index = hb_value_get_int(val);
        s = import_indexed_filter(HB_FILTER_DECOMB, index);
        if (s != NULL)
        {
            hb_dict_set(preset, "PictureDecomb", hb_value_string(s));
        }
        else
        {
            hb_error("Invalid decomb index %d", index);
            hb_dict_set(preset, "PictureDecomb", hb_value_string("off"));
        }
    }

    val = hb_dict_get(preset, "PictureDeinterlace");
    if (hb_value_is_number(val))
    {
        const char *s;
        int index = hb_value_get_int(val);
        s = import_indexed_filter(HB_FILTER_YADIF, index);
        if (s != NULL)
        {
            hb_dict_set(preset, "PictureDeinterlace", hb_value_string(s));
        }
        else
        {
            hb_error("Invalid deinterlace index %d", index);
            hb_dict_set(preset, "PictureDeinterlace", hb_value_string("off"));
        }
    }
}

static void import_detel_0_0_0(hb_value_t *preset)
{
    hb_value_t *val = hb_dict_get(preset, "PictureDetelecine");
    if (hb_value_is_number(val))
    {
        const char *s;
        int index = hb_value_get_int(val);
        s = import_indexed_filter(HB_FILTER_DETELECINE, index);
        if (s != NULL)
        {
            hb_dict_set(preset, "PictureDetelecine", hb_value_string(s));
        }
        else
        {
            hb_error("Invalid detelecine index %d", index);
            hb_dict_set(preset, "PictureDetelecine", hb_value_string("off"));
        }
    }
}

static void import_denoise_0_0_0(hb_value_t *preset)
{
    hb_value_t *val = hb_dict_get(preset, "PictureDenoise");
    if (hb_value_is_number(val))
    {
        const char *s;
        int index = hb_value_get_int(val);
        s = import_indexed_filter(HB_FILTER_HQDN3D, index);
        if (s != NULL)
        {
            hb_dict_set(preset, "PictureDenoiseFilter",
                        hb_value_string("hqdn3d"));
            hb_dict_set(preset, "PictureDenoisePreset", hb_value_string(s));
        }
        else
        {
            if (index != 0)
                hb_error("Invalid denoise index %d", index);
            hb_dict_set(preset, "PictureDenoiseFilter", hb_value_string("off"));
        }
    }
}

static void import_pic_0_0_0(hb_value_t *preset)
{
    if (hb_value_get_bool(hb_dict_get(preset, "UsesMaxPictureSettings")))
    {
        // UsesMaxPictureSettings was deprecated
        hb_dict_set(preset, "UsesPictureSettings", hb_value_int(2));
    }

    hb_value_t *val = hb_dict_get(preset, "PicturePAR");
    if (hb_value_is_number(val))
    {
        const char *s;
        int pic_par = hb_value_get_int(val);
        switch (pic_par)
        {
            default:
            case 0:
                s = "off";
                break;
            case 1:
                s = "strict";
                break;
            case 2:
                s = "loose";
                break;
            case 3:
                s = "custom";
                break;
        }
        hb_dict_set(preset, "PicturePAR", hb_value_string(s));
    }
    else if (hb_value_type(val) == HB_VALUE_TYPE_STRING)
    {
        const char *v = hb_value_get_string(val);
        const char *s;
        char *end;
        int pic_par = strtol(v, &end, 0);
        if (end != v)
        {
            switch (pic_par)
            {
                default:
                case 2:
                    s = "loose";
                    break;
                case 0:
                    s = "off";
                    break;
                case 1:
                    s = "strict";
                    break;
                case 3:
                    s = "custom";
                    break;
            }
            hb_dict_set(preset, "PicturePAR", hb_value_string(s));
        }
        else
        {
            if (strcasecmp(v, "off") &&
                strcasecmp(v, "strict") &&
                strcasecmp(v, "loose") &&
                strcasecmp(v, "custom"))
            {
                hb_dict_set(preset, "PicturePAR", hb_value_string("loose"));
            }
        }
    }
}

static void import_audio_0_0_0(hb_value_t *preset)
{
    hb_value_t *copy = hb_dict_get(preset, "AudioCopyMask");
    if (copy != NULL)
        return;

    copy = hb_value_array_init();
    hb_dict_set(preset, "AudioCopyMask", copy);
    if (hb_value_get_bool(hb_dict_get(preset, "AudioAllowMP2Pass")))
        hb_value_array_append(copy, hb_value_string("copy:mp2"));
    if (hb_value_get_bool(hb_dict_get(preset, "AudioAllowMP3Pass")))
        hb_value_array_append(copy, hb_value_string("copy:mp3"));
    if (hb_value_get_bool(hb_dict_get(preset, "AudioAllowAACPass")))
        hb_value_array_append(copy, hb_value_string("copy:aac"));
    if (hb_value_get_bool(hb_dict_get(preset, "AudioAllowOPUSPass")))
        hb_value_array_append(copy, hb_value_string("copy:opus"));
    if (hb_value_get_bool(hb_dict_get(preset, "AudioAllowAC3Pass")))
        hb_value_array_append(copy, hb_value_string("copy:ac3"));
    if (hb_value_get_bool(hb_dict_get(preset, "AudioAllowDTSPass")))
        hb_value_array_append(copy, hb_value_string("copy:dts"));
    if (hb_value_get_bool(hb_dict_get(preset, "AudioAllowDTSHDPass")))
        hb_value_array_append(copy, hb_value_string("copy:dtshd"));
    if (hb_value_get_bool(hb_dict_get(preset, "AudioAllowEAC3Pass")))
        hb_value_array_append(copy, hb_value_string("copy:eac3"));
    if (hb_value_get_bool(hb_dict_get(preset, "AudioAllowFLACPass")))
        hb_value_array_append(copy, hb_value_string("copy:flac"));
    if (hb_value_get_bool(hb_dict_get(preset, "AudioAllowTRUEHDPass")))
        hb_value_array_append(copy, hb_value_string("copy:truehd"));
}

static void import_video_0_0_0(hb_value_t *preset)
{
    hb_value_t *val;

    if ((val = hb_dict_get(preset, "x264Preset")) != NULL)
         hb_dict_set(preset, "VideoPreset", hb_value_dup(val));
    if ((val = hb_dict_get(preset, "x264Tune")) != NULL)
         hb_dict_set(preset, "VideoTune", hb_value_dup(val));
    if ((val = hb_dict_get(preset, "h264Profile")) != NULL)
         hb_dict_set(preset, "VideoProfile", hb_value_dup(val));
    if ((val = hb_dict_get(preset, "h264Level")) != NULL)
         hb_dict_set(preset, "VideoLevel", hb_value_dup(val));
    if ((val = hb_dict_get(preset, "x264OptionExtra")) != NULL)
        hb_dict_set(preset, "VideoOptionExtra", hb_value_dup(val));

    // Remove invalid "none" tune from VideoTune.  Frontends should
    // be removing this before saving a preset.
    if ((val = hb_dict_get(preset, "VideoTune")) != NULL)
    {
        const char *tune;
        tune = hb_value_get_string(val);
        // "none" is not a valid tune, but is used by HandBrake
        // to indicate no tune options.
        if (tune != NULL && !strncasecmp(tune, "none", 4))
        {
            tune += 4;
            if (tune[0] == ',')
            {
                tune++;
            }
        }
        if (tune != NULL)
        {
            hb_dict_set(preset, "VideoTune", hb_value_string(tune));
        }
    }

    if (hb_value_get_int(hb_dict_get(preset, "VideoQualityType")) == 0)
    {
        // Target size no longer supported
        hb_dict_set(preset, "VideoQualityType", hb_value_int(1));
    }

    if (hb_value_get_bool(hb_dict_get(preset, "VideoFrameratePFR")))
    {
        hb_dict_set(preset, "VideoFramerateMode", hb_value_string("pfr"));
    }
    else if (hb_value_get_bool(hb_dict_get(preset, "VideoFramerateCFR")))
    {
        hb_dict_set(preset, "VideoFramerateMode", hb_value_string("cfr"));
    }
    else if (hb_value_get_bool(hb_dict_get(preset, "VideoFramerateVFR")))
    {
        hb_dict_set(preset, "VideoFramerateMode", hb_value_string("vfr"));
    }

    const char *enc;
    int codec;
    enc = hb_value_get_string(hb_dict_get(preset, "VideoEncoder"));
    codec = hb_video_encoder_get_from_name(enc);
    if (codec & HB_VCODEC_FFMPEG_MASK)
    {
        if ((val = hb_dict_get(preset, "lavcOption")) != NULL)
            hb_dict_set(preset, "VideoOptionExtra", hb_value_dup(val));
    }
}

static void import_51_0_0(hb_value_t *preset)
{
    import_container_settings_51_0_0(preset);
}

static void import_50_0_0(hb_value_t *preset)
{
    import_video_pass_settings_50_0_0(preset);
}

static void import_47_0_0(hb_value_t *preset)
{
    import_pic_settings_47_0_0(preset);
}

static void import_44_0_0(hb_value_t *preset)
{
    import_pic_settings_44_0_0(preset);

    import_47_0_0(preset);
}

static void import_40_0_0(hb_value_t *preset)
{
    import_lang_list_40_0_0(preset);

    import_44_0_0(preset);
}

static void import_35_0_0(hb_value_t *preset)
{
    import_deblock_35_0_0(preset);

    import_40_0_0(preset);
}

static void import_25_0_0(hb_value_t *preset)
{
    import_video_scaler_25_0_0(preset);

    import_35_0_0(preset);
}

static void import_20_0_0(hb_value_t *preset)
{
    import_anamorphic_20_0_0(preset);

    import_25_0_0(preset);
}

static void import_12_0_0(hb_value_t *preset)
{
    import_deint_12_0_0(preset);

    import_20_0_0(preset);
}

static void import_11_1_0(hb_value_t *preset)
{
    import_filters_11_1_0(preset);

    // Import next...
    import_12_0_0(preset);
}

static void import_11_0_0(hb_value_t *preset)
{
    import_deint_11_0_0(preset);

    // Import next...
    import_11_1_0(preset);
}

static void import_10_0_0(hb_value_t *preset)
{
    import_deint_10_0_0(preset);

    // Import next...
    import_11_0_0(preset);
}

static void import_0_0_0(hb_value_t *preset)
{
    import_video_0_0_0(preset);
    import_pic_0_0_0(preset);
    import_audio_0_0_0(preset);
    import_deint_0_0_0(preset);
    import_detel_0_0_0(preset);
    import_denoise_0_0_0(preset);

    // Import next...
    import_10_0_0(preset);
}

static int cmpVersion(int a_major, int a_minor, int a_micro,
                      int b_major, int b_minor, int b_micro)
{
    if (a_major > b_major) return 1;
    if (a_major < b_major) return -1;
    if (a_minor > b_minor) return 1;
    if (a_minor < b_minor) return -1;
    if (a_micro > b_micro) return 1;
    if (a_micro < b_micro) return -1;
    return 0;
}

static int preset_import(hb_value_t *preset, int major, int minor, int micro)
{
    int result = 0;

    if (!hb_value_get_bool(hb_dict_get(preset, "Folder")))
    {
        if (cmpVersion(major, minor, micro, 0, 0, 0) <= 0)
        {
            // Convert legacy presets (before versioning introduced)
            import_0_0_0(preset);
            result = 1;
        }
        else if (cmpVersion(major, minor, micro, 10, 0, 0) <= 0)
        {
            import_10_0_0(preset);
            result = 1;
        }
        else if (cmpVersion(major, minor, micro, 11, 0, 0) <= 0)
        {
            import_11_0_0(preset);
            result = 1;
        }
        else if (cmpVersion(major, minor, micro, 11, 1, 0) <= 0)
        {
            import_11_1_0(preset);
            result = 1;
        }
        else if (cmpVersion(major, minor, micro, 12, 0, 0) <= 0)
        {
            import_12_0_0(preset);
            result = 1;
        }
        else if (cmpVersion(major, minor, micro, 20, 0, 0) <= 0)
        {
            import_20_0_0(preset);
            result = 1;
        }
        else if (cmpVersion(major, minor, micro, 25, 0, 0) <= 0)
        {
            import_25_0_0(preset);
            result = 1;
        }
        else if (cmpVersion(major, minor, micro, 35, 0, 0) <= 0)
        {
            import_35_0_0(preset);
            result = 1;
        }
        else if (cmpVersion(major, minor, micro, 40, 0, 0) <= 0)
        {
            import_40_0_0(preset);
            result = 1;
        }
        else if (cmpVersion(major, minor, micro, 44, 0, 0) <= 0)
        {
            import_44_0_0(preset);
            result = 1;
        }
        else if (cmpVersion(major, minor, micro, 47, 0, 0) <= 0)
        {
            import_47_0_0(preset);
            result = 1;
        }
        else if (cmpVersion(major, minor, micro, 50, 0, 0) <= 0)
        {
            import_50_0_0(preset);
            result = 1;
        }
        else if (cmpVersion(major, minor, micro, 51, 0, 0) <= 0)
        {
            import_51_0_0(preset);
            result = 1;
        }

        preset_clean(preset, hb_preset_template);
    }
    return result;
}

int hb_presets_version(hb_value_t *preset, int *major, int *minor, int *micro)
{
    *major = 0; *minor = 0; *micro = 0;
    if (hb_value_type(preset) == HB_VALUE_TYPE_DICT)
    {
        // Is this a single preset or a packaged collection of presets?
        hb_value_t *val = hb_dict_get(preset, "PresetName");
        if (val == NULL)
        {
            val = hb_dict_get(preset, "VersionMajor");
            if (val != NULL)
            {
                *major = hb_value_get_int(hb_dict_get(preset, "VersionMajor"));
                *minor = hb_value_get_int(hb_dict_get(preset, "VersionMinor"));
                *micro = hb_value_get_int(hb_dict_get(preset, "VersionMicro"));
                return 0;
            }
        }
    }
    return -1;
}

hb_value_t * hb_presets_update_version(hb_value_t *presets)
{
    if (hb_value_type(presets) == HB_VALUE_TYPE_DICT)
    {
        // Is this a single preset or a packaged collection of presets?
        hb_value_t *val = hb_dict_get(presets, "PresetName");
        if (val == NULL)
        {
            val = hb_dict_get(presets, "VersionMajor");
            if (val != NULL)
            {
                hb_dict_set(presets, "VersionMajor",
                            hb_value_int(hb_preset_version_major));
                hb_dict_set(presets, "VersionMinor",
                            hb_value_int(hb_preset_version_minor));
                hb_dict_set(presets, "VersionMicro",
                            hb_value_int(hb_preset_version_micro));
                hb_value_incref(presets);
                return presets;
            }
            // Unrecognizable preset file format
            return NULL;
        }
    }
    return presets_package(presets);
}

int hb_presets_import(const hb_value_t *in, hb_value_t **out)
{
    preset_import_context_t   ctx;
    hb_value_t              * dup;

    ctx.do_ctx.path.depth = 1;
    ctx.result = 0;

    // Done modify the input
    dup = hb_value_dup(in);
    hb_presets_version(dup, &ctx.major, &ctx.minor, &ctx.micro);
    presets_do(do_preset_import, dup, (preset_do_context_t*)&ctx);
    if (cmpVersion(ctx.major, ctx.minor, ctx.micro, 29, 0, 0) <= 0)
    {
        hb_value_t * tmp;

        tmp  = import_hierarchy_29_0_0(dup);
        *out = hb_presets_update_version(tmp);
        hb_value_free(&tmp);
    }
    else if (ctx.result)
    {
        *out = hb_presets_update_version(dup);
    }
    else
    {
        *out = hb_value_dup(dup);
    }
    hb_value_free(&dup);

    return ctx.result;
}

int hb_presets_import_json(const char *in, char **out)
{
    int result;

    if (out != NULL)
    {
        *out = NULL;
    }
    hb_value_t * dict = hb_value_json(in);
    if (dict == NULL)
        return 0;

    hb_value_t * imported;
    result = hb_presets_import(dict, &imported);
    if (out != NULL)
    {
        *out = hb_value_get_json(imported);
    }
    hb_value_free(&dict);
    hb_value_free(&imported);
    return result;
}

char * hb_presets_clean_json(const char *json)
{
    hb_value_t * dict = hb_value_json(json);
    if (dict == NULL)
        return NULL;

    presets_clean(dict, hb_preset_template);
    char * result = hb_value_get_json(dict);
    hb_value_free(&dict);
    return result;
}

// Note that unpackage does not make any copies.
// In one increases the reference count.
static hb_value_t * presets_unpackage(const hb_value_t *packaged_presets)
{
    // Do any legacy translations.
    hb_value_t * imported;
    hb_value_t * tmp = hb_value_dup(packaged_presets);
    hb_presets_import(tmp, &imported);
    hb_value_free(&tmp);
    if (imported == NULL)
    {
        // Invalid preset format, return an empty array
        return hb_value_array_init();
    }
    hb_value_t *presets = hb_dict_get(imported, "PresetList");
    hb_value_incref(presets);
    hb_value_free(&imported);
    return presets;
}

static hb_value_t * presets_package(const hb_value_t *presets)
{
    hb_dict_t *packaged_presets;
    if (hb_value_type(presets) != HB_VALUE_TYPE_DICT ||
        hb_dict_get(presets, "VersionMajor") == NULL)
    {
        // Preset is not packaged
        packaged_presets = hb_dict_init();
        hb_dict_set(packaged_presets, "VersionMajor",
                    hb_value_int(hb_preset_version_major));
        hb_dict_set(packaged_presets, "VersionMinor",
                    hb_value_int(hb_preset_version_minor));
        hb_dict_set(packaged_presets, "VersionMicro",
                    hb_value_int(hb_preset_version_micro));

        // TODO: What else do we want in the preset containers header?
        hb_dict_t *tmp = hb_value_dup(presets);
        if (hb_value_type(presets) == HB_VALUE_TYPE_DICT)
        {
            hb_value_array_t *array = hb_value_array_init();
            hb_value_array_append(array, tmp);
            tmp = array;
        }
        presets_clean(tmp, hb_preset_template);
        hb_dict_set(packaged_presets, "PresetList", tmp);
    }
    else
    {
        // Preset is already packaged
        hb_dict_t *tmp = hb_value_dup(presets);
        presets_clean(tmp, hb_preset_template);
        packaged_presets = tmp;
    }
    return packaged_presets;
}

void hb_presets_builtin_init(void)
{
    hb_value_t * dict = hb_value_json(hb_builtin_presets_json);
    hb_value_t * template = hb_dict_get(dict, "PresetTemplate");
    hb_preset_version_major = hb_value_get_int(
                              hb_dict_get(template, "VersionMajor"));
    hb_preset_version_minor = hb_value_get_int(
                              hb_dict_get(template, "VersionMinor"));
    hb_preset_version_micro = hb_value_get_int(
                              hb_dict_get(template, "VersionMicro"));
    hb_preset_template = hb_value_dup(hb_dict_get(template, "Preset"));

    hb_presets_builtin = hb_value_dup(hb_dict_get(dict, "PresetBuiltin"));
    hb_presets_clean(hb_presets_builtin);

    hb_presets = hb_value_array_init();
    hb_value_free(&dict);
}

int hb_presets_cli_default_init(void)
{
    hb_value_t * dict = hb_value_json(hb_builtin_presets_json);
    hb_presets_cli_default = hb_value_dup(hb_dict_get(dict, "PresetCLIDefault"));
    hb_presets_clean(hb_presets_cli_default);

    int result = hb_presets_add_internal(hb_presets_cli_default);
    hb_value_free(&dict);
    return result;
}

void hb_presets_current_version(int *major, int* minor, int *micro)
{
    *major = hb_preset_version_major;
    *minor = hb_preset_version_minor;
    *micro = hb_preset_version_micro;
}

int hb_presets_gui_init(void)
{
    char path[1024];
    hb_value_t * dict = NULL;

#if defined(HB_PRESET_JSON_FILE)
    hb_get_user_config_filename(path, "%s", HB_PRESET_JSON_FILE);
    dict = hb_value_read_json(path);
#endif
#if defined(HB_PRESET_PLIST_FILE)
    if (dict == NULL)
    {
        hb_get_user_config_filename(path, "%s", HB_PRESET_PLIST_FILE);
        dict = hb_plist_parse_file(path);
    }
#endif
    if (dict == NULL)
    {
        hb_error("Failed to load GUI presets file");
#if defined(HB_PRESET_JSON_FILE)
        hb_error("Attempted: %s", HB_PRESET_JSON_FILE);
#endif
#if defined(HB_PRESET_PLIST_FILE)
        hb_error("Attempted: %s", HB_PRESET_PLIST_FILE);
#endif
        return -1;
    }
    else
    {
        preset_do_context_t ctx;
        ctx.path.depth = 1;
        presets_do(do_delete_builtin, dict, &ctx);
        int result = hb_presets_add(dict);
        hb_value_free(&dict);
        return result;
    }
    return -1;
}

hb_value_t * hb_presets_builtin_get(void)
{
    return hb_value_dup(hb_presets_builtin);
}

char * hb_presets_builtin_get_json(void)
{
    char *json = hb_value_get_json(hb_presets_builtin);
    return json;
}

// Lookup a preset in the preset list.  The "name" may contain '/'
// separators to explicitly specify a preset within the preset lists
// folder structure.
//
// If 'recurse' is specified, a recursive search for the first component
// in the name will be performed.
//
// I assume that the actual preset name does not include any '/'
//
// A reference to the preset is returned
static hb_preset_index_t * preset_lookup_path(const char *name,
                                              int recurse, int type)
{
    preset_search_context_t ctx;
    int result;

    ctx.do_ctx.path.depth = 1;
    ctx.name = name;
    ctx.type = type;
    ctx.recurse = recurse;
    ctx.last_match_idx = -1;
    result = presets_do(do_preset_search, hb_presets,
                        (preset_do_context_t*)&ctx);
    if (result != PRESET_DO_SUCCESS)
        ctx.do_ctx.path.depth = 0;

    return hb_preset_index_dup(&ctx.do_ctx.path);
}

// Lookup a preset in the preset list.  The "name" may contain '/'
// separators to explicitly specify a preset within the preset lists
// folder structure.
//
// If 'recurse' is specified, a recursive search for the first component
// in the name will be performed.
//
// I assume that the actual preset name does not include any '/'
//
// A copy of the preset is returned
hb_preset_index_t * hb_preset_search_index(const char *name,
                                           int recurse, int type)
{
    return preset_lookup_path(name, recurse, type);
}

hb_value_t * hb_preset_search(const char *name, int recurse, int type)
{
    hb_preset_index_t *path = preset_lookup_path(name, recurse, type);
    hb_value_t *preset = hb_preset_get(path);
    free(path);
    return preset;
}

char * hb_preset_search_json(const char *name, int recurse, int type)
{
    hb_value_t * preset;
    char *json;
    preset = hb_preset_search(name, recurse, type);
    if (preset == NULL)
        return NULL;
    json = hb_value_get_json(preset);
    return json;
}

static hb_preset_index_t * lookup_default_index(hb_value_t *list)
{
    preset_do_context_t ctx;
    int result;

    ctx.path.depth = 1;
    result = presets_do(do_find_default, list, &ctx);
    if (result != PRESET_DO_SUCCESS)
        ctx.path.depth = 0;
    return hb_preset_index_dup(&ctx.path);
}

hb_preset_index_t * hb_presets_get_default_index(void)
{
    hb_preset_index_t *path = lookup_default_index(hb_presets);
    return path;
}

hb_dict_t * hb_presets_get_default(void)
{
    hb_dict_t *         preset;
    hb_preset_index_t * path = hb_presets_get_default_index();

    preset = hb_preset_get(path);
    free(path);
    return preset;
}

char * hb_presets_get_default_json(void)
{
    // Look for default preset
    hb_value_t *def = hb_presets_get_default();
    return hb_value_get_json(def);
}

void hb_presets_clear_default()
{
    preset_do_context_t ctx;
    ctx.path.depth = 1;
    presets_do(do_clear_default, hb_presets, &ctx);
}

void hb_presets_builtin_update(void)
{
    preset_do_context_t ctx;
    hb_preset_index_t *path;
    hb_value_t *builtin;
    int ii;

    ctx.path.depth = 1;
    presets_do(do_delete_builtin, hb_presets, &ctx);

    builtin = hb_value_dup(hb_presets_builtin);
    path = lookup_default_index(hb_presets);
    if (path != NULL && path->depth != 0)
    {
        // The "Default" preset is an existing custom preset.
        // Clear the default preset in builtins
        ctx.path.depth = 1;
        presets_do(do_clear_default, builtin, &ctx);
    }
    free(path);

    for (ii = hb_value_array_len(builtin) - 1; ii >= 0; ii--)
    {
        hb_value_t *dict;
        dict = hb_value_array_get(builtin, ii);
        hb_value_incref(dict);
        hb_value_array_insert(hb_presets, 0, dict);
    }
    hb_value_free(&builtin);
}

static int hb_presets_add_internal(hb_value_t *preset)
{
    hb_preset_index_t *path;
    int                added = 0;

    if (preset == NULL)
        return -1;

    path = lookup_default_index(preset);
    if (path != NULL && path->depth != 0)
    {
        // There is a "Default" preset in the preset(s) being added.
        // Clear any existing default preset.
        hb_presets_clear_default();
    }
    free(path);

    int index = hb_value_array_len(hb_presets);
    if (hb_value_type(preset) == HB_VALUE_TYPE_DICT)
    {
        // A standalone preset or folder of presets. Add to preset array.
        hb_value_array_append(hb_presets, hb_value_dup(preset));
        added++;
    }
    else if (hb_value_type(preset) == HB_VALUE_TYPE_ARRAY)
    {
        // An array of presets. Add each element.
        int count = hb_value_array_len(preset);
        int ii;
        for (ii = 0; ii < count; ii++)
        {
            hb_value_t *value = hb_value_array_get(preset, ii);
            hb_value_array_append(hb_presets, hb_value_dup(value));
            added++;
        }
    }

    hb_value_free(&preset);
    if (added == 0)
    {
        return -1;
    }

    return index;
}

int hb_presets_add(hb_value_t *preset)
{
    if (preset == NULL)
        return -1;

    preset = presets_unpackage(preset);
    if (preset == NULL)
        return -1;

    return hb_presets_add_internal(preset);
}

int hb_presets_add_json(const char *json)
{
    hb_value_t *preset = hb_value_json(json);
    if (preset == NULL)
        return -1;
    int result = hb_presets_add(preset);
    hb_value_free(&preset);
    return result;
}

int hb_presets_version_file(const char *filename,
                            int *major, int *minor, int *micro)
{
    int result;

    hb_value_t *preset = hb_value_read_json(filename);
    if (preset == NULL)
        preset = hb_plist_parse_file(filename);
    if (preset == NULL)
        return -1;

    result = hb_presets_version(preset, major, minor, micro);
    hb_value_free(&preset);

    return result;
}

hb_value_t* hb_presets_read_file(const char *filename)
{
    hb_value_t *preset = hb_value_read_json(filename);
    if (preset == NULL)
        preset = hb_plist_parse_file(filename);
    if (preset == NULL)
        return NULL;

    return preset;
}

char * hb_presets_read_file_json(const char *filename)
{
    char *result;
    hb_value_t *preset = hb_value_read_json(filename);
    if (preset == NULL)
        preset = hb_plist_parse_file(filename);
    if (preset == NULL)
        return NULL;

    result = hb_value_get_json(preset);
    return result;
}

int hb_presets_add_file(const char *filename)
{
    hb_value_t *preset = hb_value_read_json(filename);
    if (preset == NULL)
        preset = hb_plist_parse_file(filename);
    if (preset == NULL)
        return -1;

    preset_do_context_t ctx;

    ctx.path.depth = 1;
    presets_do(do_make_custom, preset, &ctx);

    int result = hb_presets_add(preset);
    hb_value_free(&preset);

    return result;
}

static int compare_str(const void *a, const void *b)
{
    return strncmp(*(const char**)a, *(const char**)b, PATH_MAX);
}

int hb_presets_add_path(char * path)
{
    hb_stat_t       sb;
    HB_DIR        * dir;
    struct dirent * entry;
    char          * filename;
    int             count, ii;
    char         ** files;
    int             result = -1;

    if (hb_stat(path, &sb))
        return -1;

    if (S_ISREG(sb.st_mode))
    {
        return hb_presets_add_file(path);
    }

    if (!S_ISDIR(sb.st_mode))
        return -1;

    dir = hb_opendir(path);
    if ( dir == NULL )
        return -1;

    // Count the total number of entries
    count = 0;
    while (hb_readdir(dir))
    {
        count++;
    }

    if (count == 0)
    {
        return -1;
    }

    files = malloc(count * sizeof(char*));

    // Find all regular files
    ii = 0;
    hb_rewinddir(dir);
    while ((entry = hb_readdir(dir)))
    {
        filename = hb_strdup_printf("%s" DIR_SEP_STR "%s", path, entry->d_name);
        if (hb_stat(filename, &sb))
        {
            free(filename);
            continue;
        }

        // Only load regular files
        if (!S_ISREG(sb.st_mode))
        {
            free(filename);
            continue;
        }
        // Only load files with .json extension
        if (strcmp(".json", filename + strlen(filename) - 5))
        {
            free(filename);
            continue;
        }

        files[ii++] = filename;
    }
    count = ii;

    // Sort the files so presets get added in a consistent order
    qsort(files, count, sizeof(char*), compare_str);

    // Add preset files to preset list
    for (ii = 0; ii < count; ii++)
    {
        int res = hb_presets_add_file(files[ii]);
        // return success if any one of the files is successfully loaded
        if (res >= 0)
            result = res;
    }
    hb_closedir( dir );
    free(files);

    return result;
}

hb_value_t * hb_presets_get(void)
{
    return hb_presets;
}

char * hb_presets_get_json(void)
{
    char * result;
    hb_value_t *presets = hb_presets_get();
    result = hb_value_get_json(presets);
    return result;
}

int hb_presets_write_json(const hb_value_t *preset, const char *path)
{
    hb_value_t *packaged_preset = presets_package(preset);
    // Packaging does some validity checks and can fail
    if (packaged_preset == NULL)
        return -1;
    int result = hb_value_write_json(packaged_preset, path);
    hb_value_free(&packaged_preset);
    return result;
}

char * hb_presets_package_json(const hb_value_t *preset)
{
    hb_value_t *packaged_preset = presets_package(preset);
    // Packaging does some validity checks and can fail
    if (packaged_preset == NULL)
        return NULL;
    char *out_json = hb_value_get_json(packaged_preset);
    hb_value_free(&packaged_preset);
    return out_json;
}

char * hb_presets_json_package(const char *in_json)
{
    hb_value_t *preset = hb_value_json(in_json);
    hb_value_t *packaged_preset = presets_package(preset);
    // Packaging does some validity checks and can fail
    if (packaged_preset == NULL)
        return NULL;
    char *out_json = hb_value_get_json(packaged_preset);
    hb_value_free(&packaged_preset);
    hb_value_free(&preset);
    return out_json;
}

void hb_presets_free(void)
{
    hb_value_free(&hb_preset_template);
    hb_value_free(&hb_presets);
    hb_value_free(&hb_presets_builtin);
}

hb_value_t *
hb_presets_get_folder_children(const hb_preset_index_t *path)
{
    int ii, count, folder;
    hb_value_t *dict;

    if (path == NULL)
        return hb_presets;

    hb_value_t *presets = hb_presets;
    for (ii = 0; ii < path->depth; ii++)
    {
        count = hb_value_array_len(presets);
        if (path->index[ii] >= count) return NULL;
        dict = hb_value_array_get(presets, path->index[ii]);
        folder = hb_value_get_bool(hb_dict_get(dict, "Folder"));
        if (!folder)
            break;
        presets = hb_dict_get(dict, "ChildrenArray");
    }
    if (ii < path->depth)
        return NULL;
    return presets;
}

hb_value_t *
hb_preset_get(const hb_preset_index_t *path)
{
    hb_value_t *folder = NULL;

    if (path == NULL || path->depth <= 0)
        return NULL;

    hb_preset_index_t folder_path = *path;
    folder_path.depth--;
    folder = hb_presets_get_folder_children(&folder_path);
    if (folder)
    {
        if (hb_value_array_len(folder) <= path->index[path->depth-1])
        {
            hb_error("hb_preset_get: not found");
        }
        else
        {
            return hb_value_array_get(folder, path->index[path->depth-1]);
        }
    }
    else
    {
        hb_error("hb_preset_get: not found");
    }
    return NULL;
}

int
hb_preset_set(const hb_preset_index_t *path, const hb_value_t *dict)
{
    hb_value_t *folder = NULL;

    if (dict == NULL || path == NULL || path->depth <= 0)
        return -1;

    hb_preset_index_t folder_path = *path;
    folder_path.depth--;
    folder = hb_presets_get_folder_children(&folder_path);
    if (folder)
    {
        if (hb_value_array_len(folder) <= path->index[path->depth-1])
        {
            hb_error("hb_preset_replace: not found");
            return -1;
        }
        else
        {
            hb_value_t *dup = hb_value_dup(dict);
            presets_clean(dup, hb_preset_template);
            hb_value_array_set(folder, path->index[path->depth-1], dup);
        }
    }
    else
    {
        hb_error("hb_preset_replace: not found");
        return -1;
    }
    return 0;
}

int hb_preset_insert(const hb_preset_index_t *path, const hb_value_t *dict)
{
    hb_value_t *folder = NULL;

    if (dict == NULL || path == NULL || path->depth < 0)
        return -1;

    int index = path->index[path->depth - 1];
    hb_preset_index_t folder_path = *path;
    folder_path.depth--;
    folder = hb_presets_get_folder_children(&folder_path);
    if (folder)
    {
        hb_value_t *dup = hb_value_dup(dict);
        presets_clean(dup, hb_preset_template);
        if (hb_value_array_len(folder) <= index)
        {
            index = hb_value_array_len(folder);
            hb_value_array_append(folder, dup);
        }
        else
        {
            hb_value_array_insert(folder, index, dup);
        }
    }
    else
    {
        hb_error("hb_preset_insert: not found");
        return -1;
    }
    return index;
}

int hb_preset_append(const hb_preset_index_t *path, const hb_value_t *dict)
{
    hb_value_t *folder = NULL;

    if (dict == NULL)
        return -1;

    folder = hb_presets_get_folder_children(path);
    if (folder)
    {
        int index;
        hb_value_t *dup = hb_value_dup(dict);
        presets_clean(dup, hb_preset_template);
        index = hb_value_array_len(folder);
        hb_value_array_append(folder, dup);
        return index;
    }
    else
    {
        hb_error("hb_preset_append: not found");
        return -1;
    }
    return 0;
}

int
hb_preset_delete(const hb_preset_index_t *path)
{
    hb_value_t *folder = NULL;

    if (path == NULL)
        return -1;

    hb_preset_index_t folder_path = *path;
    folder_path.depth--;
    folder = hb_presets_get_folder_children(&folder_path);
    if (folder)
    {
        if (hb_value_array_len(folder) <= path->index[path->depth-1])
        {
            hb_error("hb_preset_delete: not found");
            return -1;
        }
        else
        {
            hb_value_array_remove(folder, path->index[path->depth-1]);
        }
    }
    else
    {
        hb_error("hb_preset_delete: not found");
        return -1;
    }
    return 0;
}

int hb_preset_move(const hb_preset_index_t *src_path,
                   const hb_preset_index_t *dst_path)
{
    hb_value_t *src_folder = NULL;
    hb_value_t *dst_folder = NULL;

    hb_preset_index_t src_folder_path = *src_path;
    hb_preset_index_t dst_folder_path = *dst_path;
    src_folder_path.depth--;
    dst_folder_path.depth--;
    src_folder = hb_presets_get_folder_children(&src_folder_path);
    dst_folder = hb_presets_get_folder_children(&dst_folder_path);
    if (src_folder == NULL || dst_folder == NULL)
    {
        hb_error("hb_preset_move: not found");
        return -1;
    }

    hb_value_t *dict;
    int         src_index, dst_index;

    src_index = src_path->index[src_path->depth-1];
    dst_index = dst_path->index[dst_path->depth-1];
    dict      = hb_value_array_get(src_folder, src_index);
    hb_value_incref(dict);
    hb_value_array_remove(src_folder, src_index);

    // Be careful about indexes in the case that they are in the same folder
    if (src_folder == dst_folder && src_index < dst_index)
        dst_index--;
    if (hb_value_array_len(dst_folder) <= dst_index)
        hb_value_array_append(dst_folder, dict);
    else
        hb_value_array_insert(dst_folder, dst_index, dict);

    return 0;
}
