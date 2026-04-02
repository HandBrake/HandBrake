/* audio_filters.c

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/audio_filters.h"

#include <string.h>

// Array order must match the enum order in audio_filters.h.
// The enum order defines the recommended signal chain:
//   restore -> denoise -> dynamics -> enhance -> spatial -> upmix -> limit -> normalize
static const hb_audio_filter_info_t hb_audio_filters[] =
{
    // Restoration
    [HB_AUDIO_FILTER_ADECLICK] = {
        .id          = HB_AUDIO_FILTER_ADECLICK,
        .name        = "Click Removal",
        .short_name  = "adeclick",
        .ffmpeg_name = "adeclick",
        .default_str = "adeclick=w=55:o=75:a=2:t=2:b=2",
        .prefix      = NULL,
        .description = "Removes clicks and pops from audio",
    },
    [HB_AUDIO_FILTER_ADECLIP] = {
        .id          = HB_AUDIO_FILTER_ADECLIP,
        .name        = "Declipping",
        .short_name  = "adeclip",
        .ffmpeg_name = "adeclip",
        .default_str = "adeclip=w=55:o=75:a=8:t=10",
        .prefix      = NULL,
        .description = "Repairs clipped audio samples",
    },
    // Denoising
    [HB_AUDIO_FILTER_AFFTDN] = {
        .id          = HB_AUDIO_FILTER_AFFTDN,
        .name        = "FFT Denoiser",
        .short_name  = "afftdn",
        .ffmpeg_name = "afftdn",
        .default_str = "afftdn=nr=12:nf=-50",
        .prefix      = NULL,
        .description = "FFT-based noise reduction (hiss/hum)",
    },
    [HB_AUDIO_FILTER_ANLMDN] = {
        .id          = HB_AUDIO_FILTER_ANLMDN,
        .name        = "Non-Local Means Denoiser",
        .short_name  = "anlmdn",
        .ffmpeg_name = "anlmdn",
        .default_str = "anlmdn=s=0.00001:p=2:r=6",
        .prefix      = NULL,
        .description = "Non-local means audio denoising",
    },
    // Dynamics
    [HB_AUDIO_FILTER_ACOMPRESSOR] = {
        .id          = HB_AUDIO_FILTER_ACOMPRESSOR,
        .name        = "Dynamic Range Compression",
        .short_name  = "acompressor",
        .ffmpeg_name = "acompressor",
        .default_str = "acompressor=threshold=0.125:ratio=2:attack=20:release=250:mix=1",
        .prefix      = NULL,
        .description = "Reduces loud/quiet variation",
    },
    [HB_AUDIO_FILTER_AGATE] = {
        .id          = HB_AUDIO_FILTER_AGATE,
        .name        = "Noise Gate",
        .short_name  = "agate",
        .ffmpeg_name = "agate",
        .default_str = "agate=threshold=0.125:ratio=2:attack=20:release=250",
        .prefix      = NULL,
        .description = "Silences audio below threshold",
    },
    // Enhancement
    [HB_AUDIO_FILTER_DIALOGUENHANCE] = {
        .id          = HB_AUDIO_FILTER_DIALOGUENHANCE,
        .name        = "Dialog Enhancement",
        .short_name  = "dialoguenhance",
        .ffmpeg_name = "dialoguenhance",
        .default_str = "aformat=channel_layouts=stereo,dialoguenhance=original=1:enhance=1:voice=2",
        .prefix      = "aformat=channel_layouts=stereo,",
        .description = "Improves speech clarity",
    },
    // Spatial
    [HB_AUDIO_FILTER_CROSSFEED] = {
        .id          = HB_AUDIO_FILTER_CROSSFEED,
        .name        = "Headphone Crossfeed",
        .short_name  = "crossfeed",
        .ffmpeg_name = "crossfeed",
        .default_str = "aformat=channel_layouts=stereo,crossfeed=strength=0.2:range=0.5:slope=0.5",
        .prefix      = "aformat=channel_layouts=stereo,",
        .description = "Stereo crossfeed for headphones",
    },
    [HB_AUDIO_FILTER_STEREOWIDEN] = {
        .id          = HB_AUDIO_FILTER_STEREOWIDEN,
        .name        = "Stereo Widening",
        .short_name  = "stereowiden",
        .ffmpeg_name = "stereowiden",
        .default_str = "aformat=channel_layouts=stereo,stereowiden=delay=20:feedback=0.3:crossfeed=0.3:drymix=0.8",
        .prefix      = "aformat=channel_layouts=stereo,",
        .description = "Enhances stereo image width",
    },
    // Upmix (prefix is NULL; build_string handles it dynamically
    // based on chl_in parameter since input layout varies)
    [HB_AUDIO_FILTER_SURROUND] = {
        .id          = HB_AUDIO_FILTER_SURROUND,
        .name        = "Surround Upmix",
        .short_name  = "surround",
        .ffmpeg_name = "surround",
        .default_str = "aformat=channel_layouts=stereo,surround=chl_in=stereo:chl_out=5.1",
        .prefix      = NULL,
        .description = "Upmixes stereo to surround",
    },
    // Limiting
    [HB_AUDIO_FILTER_ALIMITER] = {
        .id          = HB_AUDIO_FILTER_ALIMITER,
        .name        = "Audio Limiter",
        .short_name  = "alimiter",
        .ffmpeg_name = "alimiter",
        .default_str = "alimiter=limit=1:attack=5:release=50:level=enabled",
        .prefix      = NULL,
        .description = "Prevents audio clipping",
    },
    // Normalization (always last)
    [HB_AUDIO_FILTER_LOUDNORM] = {
        .id          = HB_AUDIO_FILTER_LOUDNORM,
        .name        = "Loudness Normalization",
        .short_name  = "loudnorm",
        .ffmpeg_name = "loudnorm",
        .default_str = "loudnorm=I=-24.0:TP=-2.0:LRA=7.0",
        .prefix      = NULL,
        .description = "EBU R128 loudness normalization",
    },
};

int hb_audio_filter_get_count(void)
{
    return HB_AUDIO_FILTER_COUNT;
}

const hb_audio_filter_info_t * hb_audio_filter_get_info(int filter_id)
{
    if (filter_id < 0 || filter_id >= HB_AUDIO_FILTER_COUNT)
    {
        return NULL;
    }
    return &hb_audio_filters[filter_id];
}

const hb_audio_filter_info_t * hb_audio_filter_get_info_by_name(const char * name)
{
    if (name == NULL)
    {
        return NULL;
    }
    for (int i = 0; i < HB_AUDIO_FILTER_COUNT; i++)
    {
        if (!strcasecmp(name, hb_audio_filters[i].short_name))
        {
            return &hb_audio_filters[i];
        }
    }
    return NULL;
}

// Extract the value of a key from a colon-separated param string.
// e.g. extract_param("chl_in=stereo:chl_out=5.1", "chl_in") -> "stereo"
// Returns a malloc'd string or NULL. Caller must free().
static char * extract_param(const char * params, const char * key)
{
    if (params == NULL || key == NULL)
    {
        return NULL;
    }
    size_t klen = strlen(key);
    const char * p = params;
    while (p != NULL)
    {
        if (strncmp(p, key, klen) == 0 && p[klen] == '=')
        {
            const char * val = p + klen + 1;
            const char * end = strchr(val, ':');
            if (end != NULL)
            {
                return hb_strndup(val, end - val);
            }
            return strdup(val);
        }
        p = strchr(p, ':');
        if (p != NULL)
        {
            p++;
        }
    }
    return NULL;
}

char * hb_audio_filter_build_string(int filter_id, const char * value)
{
    const hb_audio_filter_info_t * info = hb_audio_filter_get_info(filter_id);
    if (info == NULL || value == NULL || value[0] == '\0')
    {
        return NULL;
    }

    // "default" maps to the built-in default string
    if (!strcmp(value, "default"))
    {
        return strdup(info->default_str);
    }

    // If value already starts with the ffmpeg filter name, use as-is
    // (also check for prefix if present, e.g. "aformat=...,crossfeed")
    if (strncmp(value, info->ffmpeg_name, strlen(info->ffmpeg_name)) == 0)
    {
        return strdup(value);
    }

    // If there's a prefix (e.g. "aformat=channel_layouts=stereo,")
    // and the value already contains it, use as-is
    if (info->prefix != NULL && strncmp(value, info->prefix, strlen(info->prefix)) == 0)
    {
        return strdup(value);
    }

    // Surround filter: dynamically build aformat prefix from chl_in param
    if (filter_id == HB_AUDIO_FILTER_SURROUND)
    {
        char * chl_in = extract_param(value, "chl_in");
        if (chl_in == NULL)
        {
            chl_in = strdup("stereo");
        }
        char * result = hb_strdup_printf(
            "aformat=channel_layouts=%s,surround=%s", chl_in, value);
        free(chl_in);
        return result;
    }

    // Build "prefix + ffmpeg_name=value" or just "ffmpeg_name=value"
    if (info->prefix != NULL)
    {
        return hb_strdup_printf("%s%s=%s", info->prefix,
                                info->ffmpeg_name, value);
    }
    return hb_strdup_printf("%s=%s", info->ffmpeg_name, value);
}

hb_audio_filter_entry_t * hb_audio_filter_entry_init(int id, const char * settings)
{
    hb_audio_filter_entry_t * entry = calloc(1, sizeof(hb_audio_filter_entry_t));
    if (entry == NULL)
    {
        return NULL;
    }
    entry->id = id;
    entry->settings = settings != NULL ? strdup(settings) : NULL;
    return entry;
}

void hb_audio_filter_entry_close(hb_audio_filter_entry_t ** _entry)
{
    if (_entry == NULL || *_entry == NULL)
    {
        return;
    }
    hb_audio_filter_entry_t * entry = *_entry;
    free(entry->settings);
    free(entry);
    *_entry = NULL;
}

hb_list_t * hb_audio_filter_list_copy(const hb_list_t * src)
{
    if (src == NULL)
    {
        return NULL;
    }
    int count = hb_list_count(src);
    if (count == 0)
    {
        return NULL;
    }
    hb_list_t * dst = hb_list_init();
    for (int i = 0; i < count; i++)
    {
        hb_audio_filter_entry_t * entry = hb_list_item(src, i);
        hb_list_add(dst, hb_audio_filter_entry_init(entry->id, entry->settings));
    }
    return dst;
}

void hb_audio_filter_list_close(hb_list_t ** _list)
{
    if (_list == NULL || *_list == NULL)
    {
        return;
    }
    hb_list_t * list = *_list;
    hb_audio_filter_entry_t * entry;
    while ((entry = hb_list_item(list, 0)) != NULL)
    {
        hb_list_rem(list, entry);
        hb_audio_filter_entry_close(&entry);
    }
    hb_list_close(_list);
}

char * hb_audio_filter_list_build_string(const hb_list_t * list)
{
    if (list == NULL)
    {
        return NULL;
    }
    char * chain = NULL;
    int count = hb_list_count(list);

    for (int i = 0; i < count; i++)
    {
        hb_audio_filter_entry_t * entry = hb_list_item(list, i);
        char * segment = hb_audio_filter_build_string(entry->id, entry->settings);
        if (segment == NULL)
        {
            continue;
        }
        if (chain == NULL)
        {
            chain = segment;
        }
        else
        {
            char * tmp = hb_strdup_printf("%s,%s", chain, segment);
            free(chain);
            free(segment);
            chain = tmp;
        }
    }
    return chain;
}
