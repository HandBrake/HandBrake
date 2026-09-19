/* avaudiofilters.c

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/param.h"
#include "handbrake/avfilter_priv.h"

static void hb_avfilter_apply_settings(hb_filter_object_t *filter, hb_dict_t *dest)
{
    hb_dict_t *settings = filter->settings;
    hb_dict_t *settings_template = hb_parse_filter_settings(filter->settings_template);
    hb_dict_iter_t iter;

    for (iter = hb_dict_iter_init(settings);
         iter != HB_DICT_ITER_DONE;
         iter = hb_dict_iter_next(settings, iter))
    {
        const char *key = hb_dict_iter_key(iter);

        // Check if key found in settings is also found in the template
        hb_value_t *val = hb_dict_get(settings_template, key);
        if (val == NULL)
        {
            // Key is missing from template, indicate invalid settings
            hb_log("Invalid filter key (%s) for filter %s", key, filter->name);
            continue;
        }

        // If a string value is found, and it is non-empty,
        // it is a regex pattern for allowed values.
        const char *regex_pattern = hb_value_get_string(val);
        if (regex_pattern != NULL && regex_pattern[0] != 0)
        {
            char *param;
            param = hb_value_get_string_xform(hb_dict_get(settings, key));
            if (hb_validate_param_string(regex_pattern, param) != 0)
            {
                hb_log("Invalid filter value (%s) for key %s filter %s",
                        param, key, filter->name);
                free(param);
                continue;
            }

            if (!strcasecmp(regex_pattern, "^"HB_FLOAT_REG"$") ||
                !strcasecmp(regex_pattern, "^"HB_NEG_FLOAT_REG"$"))
            {
                double double_val = 0;
                if (hb_dict_extract_double(&double_val, filter->settings, key))
                {
                    hb_dict_set(dest, key, hb_value_double(double_val));
                }
            }
            else if (!strcasecmp(regex_pattern, "^"HB_INT_REG"$") ||
                     !strcasecmp(regex_pattern, "^"HB_NEG_INT_REG"$"))
            {
                int int_val = 0;
                if (hb_dict_extract_int(&int_val, settings, key))
                {
                    hb_dict_set(dest, key, hb_value_int(int_val));
                }
            }
            else if (!strcasecmp(regex_pattern, "^"HB_BOOL_REG"$"))
            {
                int int_val = 0;
                if (hb_dict_extract_bool(&int_val, settings, key))
                {
                    hb_dict_set(dest, key, hb_value_bool(int_val));
                }
            }
            else if (!strcasecmp(regex_pattern, "^"HB_ALL_REG"$"))
            {
                char  *string_val = 0;
                if (hb_dict_extract_string(&string_val, settings, key))
                {
                    hb_dict_set(dest, key, hb_value_string(string_val));
                }
            }


            free(param);
        }
    }
    hb_value_free(&settings_template);
}

#define FFMPEG_AUDIO_FILTER(FILTER_ID, FILTER_NAME, FILTER_SHORT_NAME, OPTS_TEMPLATE, CHANNEL_LAYOUT) \
static int FILTER_SHORT_NAME##_init(hb_filter_object_t *filter, hb_filter_init_t *init)         \
{                                                                                               \
hb_filter_private_t *pv = calloc(1, sizeof(struct hb_filter_private_s));                        \
if (pv == NULL)                                                                                 \
{                                                                                               \
    return 1;                                                                                   \
}                                                                                               \
                                                                                                \
filter->private_data = pv;                                                                      \
hb_filter_init_copy(&pv->input, init);                                                          \
                                                                                                \
hb_value_array_t *avfilters = hb_value_array_init();                                            \
hb_dict_t *avfilter = hb_dict_init();                                                           \
hb_dict_t *avsettings = hb_dict_init();                                                         \
                                                                                                \
hb_avfilter_apply_settings(filter, avsettings);                                                 \
hb_dict_set(avfilter, #FILTER_SHORT_NAME, avsettings);                                          \
hb_value_array_append(avfilters, avfilter);                                                     \
                                                                                                \
if (CHANNEL_LAYOUT > 0)                                                                         \
{                                                                                               \
    avfilter = hb_dict_init();                                                                  \
    avsettings = hb_dict_init();                                                                \
    hb_dict_set_string(avsettings, "channel_layouts", "stereo");                                \
    hb_dict_set(avfilter, "aformat", avsettings);                                               \
    hb_value_array_append(avfilters, avfilter);                                                 \
}                                                                                               \
                                                                                                \
pv->avfilters = avfilters;                                                                      \
                                                                                                \
hb_filter_init_copy(&pv->output, init);                                                         \
                                                                                                \
return 0;                                                                                       \
}                                                                                               \
                                                                                                \
hb_filter_object_t hb_filter_##FILTER_SHORT_NAME =                                              \
{                                                                                               \
    .id                = FILTER_ID,                                                             \
    .enforce_order     = 1,                                                                     \
    .skip              = 1,                                                                     \
    .name              = FILTER_NAME,                                                           \
    .short_name        = #FILTER_SHORT_NAME,                                                    \
    .settings          = NULL,                                                                  \
    .init              = FILTER_SHORT_NAME##_init,                                              \
    .work              = hb_avfilter_null_work,                                                 \
    .close             = hb_avfilter_alias_close,                                               \
    .settings_template = OPTS_TEMPLATE,                                                         \
};                                                                                              \

FFMPEG_AUDIO_FILTER(HB_AUDIO_FILTER_ADECLICK,
                    "Declick",
                    adeclick,
                    "window=^"HB_FLOAT_REG"$:overlap=^"HB_FLOAT_REG"$:arorder=^"HB_FLOAT_REG"$:threshold=^"HB_FLOAT_REG"$:"
                    "burst=^"HB_FLOAT_REG"$:method=^"HB_INT_REG"$:add=^"HB_INT_REG"$:save=^"HB_INT_REG"$",
                    0);

FFMPEG_AUDIO_FILTER(HB_AUDIO_FILTER_ADECLIP,
                    "Declip",
                    adeclip,
                    "window=^"HB_FLOAT_REG"$:overlap=^"HB_FLOAT_REG"$:arorder=^"HB_FLOAT_REG"$:threshold=^"HB_FLOAT_REG"$:"
                    "hsize=^"HB_INT_REG"$:method=^"HB_INT_REG"$:add=^"HB_INT_REG"$:save=^"HB_INT_REG"$",
                    0);

FFMPEG_AUDIO_FILTER(HB_AUDIO_FILTER_AFFTDN,
                    "FFT Denoiser",
                    afftdn,
                    "noise_reduction=^"HB_FLOAT_REG"$:noise_floor=^"HB_NEG_FLOAT_REG"$:noise_type=^"HB_INT_REG"$:"
                    "band_noise=^"HB_ALL_REG"$:residual_floor=^"HB_FLOAT_REG"$:track_noise=^"HB_FLOAT_REG"$:"
                    "track_residual=^"HB_FLOAT_REG"$:output_mode=^"HB_INT_REG"$:adaptivity=^"HB_FLOAT_REG"$:"
                    "floor_offset=^"HB_FLOAT_REG"$:noise_link=^"HB_INT_REG"$:band_multiplier=^"HB_FLOAT_REG"$:"
                    "sample_noise=^"HB_INT_REG"$:gain_smooth=^"HB_INT_REG"$",
                    0);

FFMPEG_AUDIO_FILTER(HB_AUDIO_FILTER_ANLMDN,
                    "NLMeans Denoiser",
                    anlmdn,
                    "strength=^"HB_FLOAT_REG"$:patch=^"HB_FLOAT_REG"$:research=^"HB_FLOAT_REG"$:"
                    "output=^"HB_INT_REG"$:smooth=^"HB_FLOAT_REG"$",
                    0);

FFMPEG_AUDIO_FILTER(HB_AUDIO_FILTER_AGATE,
                    "Noise Gate",
                    agate,
                    "level_in=^"HB_FLOAT_REG"$:mode=^"HB_INT_REG"$:range=^"HB_FLOAT_REG"$:threshold=^"HB_FLOAT_REG"$:"
                    "ratio=^"HB_FLOAT_REG"$:attack=^"HB_FLOAT_REG"$:release=^"HB_FLOAT_REG"$:"
                    "makeup=^"HB_FLOAT_REG"$:knee=^"HB_FLOAT_REG"$:link=^"HB_INT_REG"$:"
                    "detection=^"HB_INT_REG"$:level_sc=^"HB_FLOAT_REG"$",
                    0);

FFMPEG_AUDIO_FILTER(HB_AUDIO_FILTER_ACOMPRESSOR,
                    "Compressor",
                    acompressor,
                    "level_in=^"HB_FLOAT_REG"$:mode=^"HB_INT_REG"$:threshold=^"HB_FLOAT_REG"$:"
                    "ratio=^"HB_FLOAT_REG"$:attack=^"HB_FLOAT_REG"$:release=^"HB_FLOAT_REG"$:"
                    "makeup=^"HB_FLOAT_REG"$:knee=^"HB_FLOAT_REG"$:link=^"HB_INT_REG"$:"
                    "detection=^"HB_INT_REG"$:level_sc=^"HB_FLOAT_REG"$:mix=^"HB_FLOAT_REG"$",
                    0);

FFMPEG_AUDIO_FILTER(HB_AUDIO_FILTER_ALIMITER,
                    "Limiter",
                    alimiter,
                    "level_in=^"HB_FLOAT_REG"$:level_out=^"HB_FLOAT_REG"$:limit=^"HB_FLOAT_REG"$:"
                    "attack=^"HB_FLOAT_REG"$:release=^"HB_FLOAT_REG"$:asc=^"HB_BOOL_REG"$:"
                    "asc_level=^"HB_FLOAT_REG"$:level=^"HB_BOOL_REG"$:latency=^"HB_BOOL_REG"$",
                    0);

FFMPEG_AUDIO_FILTER(HB_AUDIO_FILTER_DIALOGUENHANCE,
                    "Dialogue Enhance",
                    dialoguenhance,
                    "original=^"HB_FLOAT_REG"$:enhance=^"HB_FLOAT_REG"$:voice=^"HB_FLOAT_REG"$",
                    0);


FFMPEG_AUDIO_FILTER(HB_AUDIO_FILTER_CROSSFEED,
                    "Headphone Crossfeed",
                    crossfeed,
                    "strength=^"HB_FLOAT_REG"$:range=^"HB_FLOAT_REG"$:slope=^"HB_FLOAT_REG"$:"
                    "level_in=^"HB_FLOAT_REG"$:level_out=^"HB_FLOAT_REG"$:block_size=^"HB_FLOAT_REG"$",
                    0);

FFMPEG_AUDIO_FILTER(HB_AUDIO_FILTER_STEREOWIDEN,
                    "Stereo Widening",
                    stereowiden,
                    "delay=^"HB_FLOAT_REG"$:feedback=^"HB_FLOAT_REG"$:crossfeed=^"HB_FLOAT_REG"$:drymix=^"HB_FLOAT_REG"$",
                    0);

FFMPEG_AUDIO_FILTER(HB_AUDIO_FILTER_LOUDNORM,
                    "Loudness Normalization",
                    loudnorm,
                    "i=^"HB_NEG_FLOAT_REG"$:lra=^"HB_FLOAT_REG"$:tp=^"HB_NEG_FLOAT_REG"$:"
                    "measured_i=^"HB_FLOAT_REG"$:measured_lra=^"HB_FLOAT_REG"$:measured_tp=^"HB_FLOAT_REG"$:"
                    "measured_thresh=^"HB_FLOAT_REG"$:offset=^"HB_FLOAT_REG"$:linear=^"HB_BOOL_REG"$:"
                    "dual_mono=^"HB_BOOL_REG"$:print_format=^"HB_INT_REG"$",
                    0);
