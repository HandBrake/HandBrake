/*
 * hb-backend.c
 * Copyright (C) John Stebbins 2008-2019 <stebbins@stebbins>
 *
 * hb-backend.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * hb-backend.c is distributed in the hope that it will be useful,
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

#define _GNU_SOURCE
#include <limits.h>
#include <ctype.h>
#include <math.h>
#include "hb.h"
#include "ghbcompat.h"
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include "hb-backend.h"
#include "settings.h"
#include "jobdict.h"
#include "callbacks.h"
#include "subtitlehandler.h"
#include "audiohandler.h"
#include "videohandler.h"
#include "preview.h"
#include "presets.h"
#include "values.h"
#include "lang.h"
#include "jansson.h"

typedef struct
{
    gchar *option;
    const gchar *shortOpt;
    gdouble ivalue;
} options_map_t;

typedef struct
{
    gint count;
    options_map_t *map;
} combo_opts_t;

static options_map_t d_subtitle_track_sel_opts[] =
{
    {N_("None"),                                    "none",       0},
    {N_("First Track Matching Selected Languages"), "first",      1},
    {N_("All Tracks Matching Selected Languages"),  "all",        2},
};
combo_opts_t subtitle_track_sel_opts =
{
    sizeof(d_subtitle_track_sel_opts)/sizeof(options_map_t),
    d_subtitle_track_sel_opts
};

static options_map_t d_subtitle_burn_opts[] =
{
    {N_("None"),                                     "none",          0},
    {N_("Foreign Audio Subtitle Track"),             "foreign",       1},
    {N_("First Selected Track"),                     "first",         2},
    {N_("Foreign Audio, then First Selected Track"), "foreign_first", 3},
};
combo_opts_t subtitle_burn_opts =
{
    sizeof(d_subtitle_burn_opts)/sizeof(options_map_t),
    d_subtitle_burn_opts
};

static options_map_t d_audio_track_sel_opts[] =
{
    {N_("None"),                                    "none",       0},
    {N_("First Track Matching Selected Languages"), "first",      1},
    {N_("All Tracks Matching Selected Languages"),  "all",        2},
};
combo_opts_t audio_track_sel_opts =
{
    sizeof(d_audio_track_sel_opts)/sizeof(options_map_t),
    d_audio_track_sel_opts
};

static options_map_t d_point_to_point_opts[] =
{
    {N_("Chapters:"), "chapter", 0},
    {N_("Seconds:"),  "time",    1},
    {N_("Frames:"),   "frame",   2},
};
combo_opts_t point_to_point_opts =
{
    sizeof(d_point_to_point_opts)/sizeof(options_map_t),
    d_point_to_point_opts
};

static options_map_t d_when_complete_opts[] =
{
    {N_("Do Nothing"),            "nothing",  0},
    {N_("Show Notification"),     "notify",   1},
    {N_("Quit Handbrake"),        "quit",     4},
    {N_("Put Computer To Sleep"), "sleep",    2},
    {N_("Shutdown Computer"),     "shutdown", 3},
};
combo_opts_t when_complete_opts =
{
    sizeof(d_when_complete_opts)/sizeof(options_map_t),
    d_when_complete_opts
};

static options_map_t d_par_opts[] =
{
    {N_("Off"),       "off",    HB_ANAMORPHIC_NONE},
    {N_("Automatic"), "auto",   HB_ANAMORPHIC_AUTO},
    {N_("Loose"),     "loose",  HB_ANAMORPHIC_LOOSE},
    {N_("Custom"),    "custom", HB_ANAMORPHIC_CUSTOM},
};
combo_opts_t par_opts =
{
    sizeof(d_par_opts)/sizeof(options_map_t),
    d_par_opts
};

static options_map_t d_alignment_opts[] =
{
    {"2",   "2",  2},
    {"4",   "4",  4},
    {"8",   "8",  8},
    {"16", "16", 16},
};
combo_opts_t alignment_opts =
{
    sizeof(d_alignment_opts)/sizeof(options_map_t),
    d_alignment_opts
};

static options_map_t d_logging_opts[] =
{
    {"0", "0", 0},
    {"1", "1", 1},
    {"2", "2", 2},
    {"3", "3", 3},
};
combo_opts_t logging_opts =
{
    sizeof(d_logging_opts)/sizeof(options_map_t),
    d_logging_opts
};

static options_map_t d_log_longevity_opts[] =
{
    {N_("Week"),     "week",       7},
    {N_("Month"),    "month",     30},
    {N_("Year"),     "year",     365},
    {N_("Immortal"), "immortal", 366},
};
combo_opts_t log_longevity_opts =
{
    sizeof(d_log_longevity_opts)/sizeof(options_map_t),
    d_log_longevity_opts
};

static options_map_t d_appcast_update_opts[] =
{
    {N_("Never"),   "never",   0},
    {N_("Daily"),   "daily",   1},
    {N_("Weekly"),  "weekly",  2},
    {N_("Monthly"), "monthly", 3},
};
combo_opts_t appcast_update_opts =
{
    sizeof(d_appcast_update_opts)/sizeof(options_map_t),
    d_appcast_update_opts
};

static options_map_t d_vqual_granularity_opts[] =
{
    {"0.2",  "0.2",  0.2 },
    {"0.25", "0.25", 0.25},
    {"0.5",  "0.5",  0.5 },
    {"1",    "1",    1   },
};
combo_opts_t vqual_granularity_opts =
{
    sizeof(d_vqual_granularity_opts)/sizeof(options_map_t),
    d_vqual_granularity_opts
};

static options_map_t d_deint_opts[] =
{
    {N_("Off"),         "off",         HB_FILTER_INVALID    },
    {N_("Decomb"),      "decomb",      HB_FILTER_DECOMB     },
    {N_("Yadif"),       "deinterlace", HB_FILTER_DEINTERLACE},
};
combo_opts_t deint_opts =
{
    sizeof(d_deint_opts)/sizeof(options_map_t),
    d_deint_opts
};

static options_map_t d_denoise_opts[] =
{
    {N_("Off"),     "off",     HB_FILTER_INVALID},
    {N_("NLMeans"), "nlmeans", HB_FILTER_NLMEANS},
    {N_("HQDN3D"),  "hqdn3d",  HB_FILTER_HQDN3D },
};
combo_opts_t denoise_opts =
{
    sizeof(d_denoise_opts)/sizeof(options_map_t),
    d_denoise_opts
};

static options_map_t d_sharpen_opts[] =
{
    {N_("Off"),      "off",      HB_FILTER_INVALID},
    {N_("Unsharp"),  "unsharp",  HB_FILTER_UNSHARP},
    {N_("Lapsharp"), "lapsharp", HB_FILTER_LAPSHARP },
};
combo_opts_t sharpen_opts =
{
    sizeof(d_sharpen_opts)/sizeof(options_map_t),
    d_sharpen_opts
};

static options_map_t d_rotate_opts[] =
{
    {N_("Off"),         "disable=1",   0},
    {N_("90 Degrees"),  "angle=90",   90},
    {N_("180 Degrees"), "angle=180", 180},
    {N_("270 Degrees"), "angle=270", 270},
};
combo_opts_t rotate_opts =
{
    sizeof(d_rotate_opts)/sizeof(options_map_t),
    d_rotate_opts
};

static options_map_t d_direct_opts[] =
{
    {N_("None"),      "none",     0},
    {N_("Spatial"),   "spatial",  1},
    {N_("Temporal"),  "temporal", 2},
    {N_("Automatic"), "auto",     3},
};
combo_opts_t direct_opts =
{
    sizeof(d_direct_opts)/sizeof(options_map_t),
    d_direct_opts
};

static options_map_t d_badapt_opts[] =
{
    {N_("Off"),             "0", 0},
    {N_("Fast"),            "1", 1},
    {N_("Optimal"),         "2", 2},
};
combo_opts_t badapt_opts =
{
    sizeof(d_badapt_opts)/sizeof(options_map_t),
    d_badapt_opts
};

static options_map_t d_bpyramid_opts[] =
{
    {N_("Off"),    "none",   0},
    {N_("Strict"), "strict", 1},
    {N_("Normal"), "normal", 2},
};
combo_opts_t bpyramid_opts =
{
    sizeof(d_bpyramid_opts)/sizeof(options_map_t),
    d_bpyramid_opts
};

static options_map_t d_weightp_opts[] =
{
    {N_("Off"),    "0", 0},
    {N_("Simple"), "1", 1},
    {N_("Smart"),  "2", 2},
};
combo_opts_t weightp_opts =
{
    sizeof(d_weightp_opts)/sizeof(options_map_t),
    d_weightp_opts
};

static options_map_t d_me_opts[] =
{
    {N_("Diamond"),              "dia",  0},
    {N_("Hexagon"),              "hex",  1},
    {N_("Uneven Multi-Hexagon"), "umh",  2},
    {N_("Exhaustive"),           "esa",  3},
    {N_("Hadamard Exhaustive"),  "tesa", 4},
};
combo_opts_t me_opts =
{
    sizeof(d_me_opts)/sizeof(options_map_t),
    d_me_opts
};

static options_map_t d_subme_opts[] =
{
    {N_("0: SAD, no subpel"),                     "0", 0},
    {N_("1: SAD, qpel"),                          "1", 1},
    {N_("2: SATD, qpel"),                         "2", 2},
    {N_("3: SATD: multi-qpel"),                   "3", 3},
    {N_("4: SATD, qpel on all"),                  "4", 4},
    {N_("5: SATD, multi-qpel on all"),            "5", 5},
    {N_("6: RD in I/P-frames"),                   "6", 6},
    {N_("7: RD in all frames"),                   "7", 7},
    {N_("8: RD refine in I/P-frames"),            "8", 8},
    {N_("9: RD refine in all frames"),            "9", 9},
    {N_("10: QPRD in all frames"),                "10", 10},
    {N_("11: No early terminations in analysis"), "11", 11},
};
combo_opts_t subme_opts =
{
    sizeof(d_subme_opts)/sizeof(options_map_t),
    d_subme_opts
};

static options_map_t d_analyse_opts[] =
{
    {N_("Most"),   "p8x8,b8x8,i8x8,i4x4", 0},
    {N_("None"),   "none",                1},
    {N_("Some"),   "i4x4,i8x8",           2},
    {N_("All"),    "all",                 3},
    {N_("Custom"), "custom",              4},
};
combo_opts_t analyse_opts =
{
    sizeof(d_analyse_opts)/sizeof(options_map_t),
    d_analyse_opts
};

static options_map_t d_trellis_opts[] =
{
    {N_("Off"),         "0", 0},
    {N_("Encode only"), "1", 1},
    {N_("Always"),      "2", 2},
};
combo_opts_t trellis_opts =
{
    sizeof(d_trellis_opts)/sizeof(options_map_t),
    d_trellis_opts
};

typedef struct
{
    int      filter_id;
    gboolean preset;
} filter_opts_t;

static filter_opts_t deblock_preset_opts =
{
    .filter_id = HB_FILTER_DEBLOCK,
    .preset    = TRUE
};

static filter_opts_t deblock_tune_opts =
{
    .filter_id = HB_FILTER_DEBLOCK,
    .preset    = FALSE
};

static filter_opts_t deint_preset_opts =
{
    .filter_id = HB_FILTER_DECOMB,
    .preset    = TRUE
};

static filter_opts_t nlmeans_preset_opts =
{
    .filter_id = HB_FILTER_NLMEANS,
    .preset    = TRUE
};

static filter_opts_t nlmeans_tune_opts =
{
    .filter_id = HB_FILTER_NLMEANS,
    .preset    = FALSE
};

static filter_opts_t sharpen_preset_opts =
{
    .filter_id = HB_FILTER_UNSHARP,
    .preset    = TRUE
};

static filter_opts_t sharpen_tune_opts =
{
    .filter_id = HB_FILTER_UNSHARP,
    .preset    = FALSE
};

#if 0
static filter_opts_t hqdn3d_preset_opts =
{
    .filter_id = HB_FILTER_HQDN3D,
    .preset    = TRUE
};
#endif

static filter_opts_t comb_detect_opts =
{
    .filter_id = HB_FILTER_COMB_DETECT,
    .preset    = TRUE
};

static filter_opts_t detel_opts =
{
    .filter_id = HB_FILTER_DETELECINE,
    .preset    = TRUE
};

typedef void (*opts_set_f)(signal_user_data_t *ud, const gchar *name,
                           void *opts, const void* data);
typedef GhbValue* (*opt_get_f)(const gchar *name, const void *opts,
                               const GhbValue *gval, GhbType type);
typedef struct
{
    const gchar  * name;
    void         * opts;
    opts_set_f     opts_set;
    opt_get_f      opt_get;
} combo_name_map_t;

static void small_opts_set(signal_user_data_t *ud, const gchar *name,
                           void *opts, const void* data);
static void audio_bitrate_opts_set(signal_user_data_t *ud, const gchar *name,
                                   void *opts, const void* data);
static void audio_samplerate_opts_set(signal_user_data_t *ud, const gchar *name,
                                      void *opts, const void* data);
static void video_framerate_opts_set(signal_user_data_t *ud, const gchar *name,
                                     void *opts, const void* data);
static void mix_opts_set(signal_user_data_t *ud, const gchar *name,
                         void *opts, const void* data);
static void video_encoder_opts_set(signal_user_data_t *ud, const gchar *name,
                                   void *opts, const void* data);
static void audio_encoder_opts_set(signal_user_data_t *ud, const gchar *name,
                                   void *opts, const void* data);
static void acodec_fallback_opts_set(signal_user_data_t *ud, const gchar *name,
                                     void *opts, const void* data);
static void language_opts_set(signal_user_data_t *ud, const gchar *name,
                              void *opts, const void* data);
static void srt_codeset_opts_set(signal_user_data_t *ud, const gchar *name,
                                 void *opts, const void* data);
static void title_opts_set(signal_user_data_t *ud, const gchar *name,
                           void *opts, const void* data);
static void audio_track_opts_set(signal_user_data_t *ud, const gchar *name,
                                 void *opts, const void* data);
static void subtitle_track_opts_set(signal_user_data_t *ud, const gchar *name,
                                    void *opts, const void* data);
static void video_tune_opts_set(signal_user_data_t *ud, const gchar *name,
                                void *opts, const void* data);
static void video_profile_opts_set(signal_user_data_t *ud, const gchar *name,
                                   void *opts, const void* data);
static void video_level_opts_set(signal_user_data_t *ud, const gchar *name,
                                 void *opts, const void* data);
static void container_opts_set(signal_user_data_t *ud, const gchar *name,
                               void *opts, const void* data);
static void preset_category_opts_set(signal_user_data_t *ud, const gchar *name,
                                     void *opts, const void* data);
static void filter_opts_set(signal_user_data_t *ud, const gchar *name,
                           void *opts, const void* data);
static void deint_opts_set(signal_user_data_t *ud, const gchar *name,
                           void *vopts, const void* data);
static void denoise_opts_set(signal_user_data_t *ud, const gchar *name,
                             void *vopts, const void* data);
static void sharpen_opts_set(signal_user_data_t *ud, const gchar *name,
                             void *vopts, const void* data);

static GhbValue * generic_opt_get(const char *name, const void *opts,
                                  const GhbValue *gval, GhbType type);
static GhbValue * filter_opt_get(const char *name, const void *opts,
                                const GhbValue *gval, GhbType type);

combo_name_map_t combo_name_map[] =
{
    {
        "SubtitleTrackSelectionBehavior",
        &subtitle_track_sel_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "SubtitleBurnBehavior",
        &subtitle_burn_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "AudioTrackSelectionBehavior",
        &audio_track_sel_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "PtoPType",
        &point_to_point_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "WhenComplete",
        &when_complete_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "QueueWhenComplete",
        &when_complete_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "PicturePAR",
        &par_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "PictureModulus",
        &alignment_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "LoggingLevel",
        &logging_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "LogLongevity",
        &log_longevity_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "check_updates",
        &appcast_update_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "VideoQualityGranularity",
        &vqual_granularity_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "PictureCombDetectPreset",
        &comb_detect_opts,
        filter_opts_set,
        filter_opt_get
    },
    {
        "PictureDeinterlaceFilter",
        &deint_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "PictureDeinterlacePreset",
        &deint_preset_opts,
        deint_opts_set,
        filter_opt_get
    },
    {
        "PictureDetelecine",
        &detel_opts,
        filter_opts_set,
        filter_opt_get
    },
    {
        "PictureDeblockPreset",
        &deblock_preset_opts,
        filter_opts_set,
        filter_opt_get
    },
    {
        "PictureDeblockTune",
        &deblock_tune_opts,
        filter_opts_set,
        filter_opt_get
    },
    {
        "PictureDenoiseFilter",
        &denoise_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "PictureDenoisePreset",
        &nlmeans_preset_opts,
        denoise_opts_set,
        filter_opt_get
    },
    {
        "PictureDenoiseTune",
        &nlmeans_tune_opts,
        filter_opts_set,
        filter_opt_get
    },
    {
        "PictureSharpenFilter",
        &sharpen_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "PictureSharpenPreset",
        &sharpen_preset_opts,
        sharpen_opts_set,
        filter_opt_get
    },
    {
        "PictureSharpenTune",
        &sharpen_tune_opts,
        sharpen_opts_set,
        filter_opt_get
    },
    {
        "PictureRotate",
        &rotate_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "AudioBitrate",
        NULL,
        audio_bitrate_opts_set,
        NULL
    },
    {
        "AudioSamplerate",
        NULL,
        audio_samplerate_opts_set,
        NULL
    },
    {
        "VideoFramerate",
        NULL,
        video_framerate_opts_set,
        NULL
    },
    {
        "AudioMixdown",
        NULL,
        mix_opts_set,
        NULL
    },
    {
        "VideoEncoder",
        NULL,
        video_encoder_opts_set,
        NULL
    },
    {
        "AudioEncoder",
        NULL,
        audio_encoder_opts_set,
        NULL
    },
    {
        "AudioEncoderFallback",
        NULL,
        acodec_fallback_opts_set,
        NULL
    },
    {
        "ImportLanguage",
        NULL,
        language_opts_set,
        NULL
    },
    {
        "SrtCodeset",
        NULL,
        srt_codeset_opts_set,
        NULL
    },
    {
        "title",
        NULL,
        title_opts_set,
        NULL
    },
    {
        "AudioTrack",
        NULL,
        audio_track_opts_set,
        NULL
    },
    {
        "SubtitleTrack",
        NULL,
        subtitle_track_opts_set,
        NULL
    },
    {
        "VideoTune",
        NULL,
        video_tune_opts_set,
        NULL
    },
    {
        "VideoProfile",
        NULL,
        video_profile_opts_set,
        NULL
    },
    {
        "VideoLevel",
        NULL,
        video_level_opts_set,
        NULL
    },
    {
        "FileFormat",
        NULL,
        container_opts_set,
        NULL
    },
    {
        "PresetCategory",
        NULL,
        preset_category_opts_set,
        NULL
    },
    {NULL, NULL, NULL, NULL}
};

const gchar *srt_codeset_table[] =
{
    "ANSI_X3.4-1968",
    "ANSI_X3.4-1986",
    "ANSI_X3.4",
    "ANSI_X3.110-1983",
    "ANSI_X3.110",
    "ASCII",
    "ECMA-114",
    "ECMA-118",
    "ECMA-128",
    "ECMA-CYRILLIC",
    "IEC_P27-1",
    "ISO-8859-1",
    "ISO-8859-2",
    "ISO-8859-3",
    "ISO-8859-4",
    "ISO-8859-5",
    "ISO-8859-6",
    "ISO-8859-7",
    "ISO-8859-8",
    "ISO-8859-9",
    "ISO-8859-9E",
    "ISO-8859-10",
    "ISO-8859-11",
    "ISO-8859-13",
    "ISO-8859-14",
    "ISO-8859-15",
    "ISO-8859-16",
    "UTF-7",
    "UTF-8",
    "UTF-16",
    "UTF-16LE",
    "UTF-16BE",
    "UTF-32",
    "UTF-32LE",
    "UTF-32BE",
    NULL
};
#define SRT_TABLE_SIZE (sizeof(srt_codeset_table)/ sizeof(char*)-1)

int
ghb_lookup_lang(const GhbValue *glang)
{
    const gchar *str;

    str = ghb_value_get_string(glang);
    return lang_lookup_index(str);
}

static void
del_tree(const gchar *name, gboolean del_top)
{
    const gchar *file;

    if (g_file_test(name, G_FILE_TEST_IS_DIR))
    {
        GDir *gdir = g_dir_open(name, 0, NULL);
        file = g_dir_read_name(gdir);
        while (file)
        {
            gchar *path;
            path = g_strdup_printf("%s/%s", name, file);
            del_tree(path, TRUE);
            g_free(path);
            file = g_dir_read_name(gdir);
        }
        if (del_top)
            g_rmdir(name);
        g_dir_close(gdir);
    }
    else
    {
        g_unlink(name);
    }
}

const gchar*
ghb_version()
{
    return hb_get_version(NULL);
}

float
ghb_vquality_default(signal_user_data_t *ud)
{
    float quality;
    gint vcodec;
    vcodec = ghb_settings_video_encoder_codec(ud->settings, "VideoEncoder");

    switch (vcodec)
    {
    case HB_VCODEC_X265_8BIT:
    case HB_VCODEC_X265_10BIT:
    case HB_VCODEC_X265_12BIT:
    case HB_VCODEC_X265_16BIT:
    case HB_VCODEC_X264_8BIT:
    case HB_VCODEC_X264_10BIT:
        return 20;
    case HB_VCODEC_THEORA:
        return 45;
    case HB_VCODEC_FFMPEG_MPEG2:
    case HB_VCODEC_FFMPEG_MPEG4:
        return 3;
    default:
    {
        float min, max, step;
        int direction;

        hb_video_quality_get_limits(vcodec, &min, &max, &step, &direction);
        // Pick something that is 70% of max
        // Probably too low for some and too high for others...
        quality = (max - min) * 0.7;
        if (direction)
            quality = max - quality;
    }
    }
    return quality;
}

void
ghb_vquality_range(
    signal_user_data_t *ud,
    float *min,
    float *max,
    float *step,
    float *page,
    gint *digits,
    int *direction)
{
    float min_step;
    gint vcodec;
    vcodec = ghb_settings_video_encoder_codec(ud->settings, "VideoEncoder");

    *page = 10;
    *digits = 0;
    hb_video_quality_get_limits(vcodec, min, max, &min_step, direction);
    *step = ghb_settings_combo_double(ud->prefs, "VideoQualityGranularity");

    if (*step < min_step)
        *step = min_step;

    if ((int)(*step * 100) % 10 != 0)
        *digits = 2;
    else if ((int)(*step * 10) % 10 != 0)
        *digits = 1;
}

gint
find_opt_entry(const combo_opts_t *opts, const GhbValue *gval)
{
    gint ii;

    if (opts == NULL)
        return 0;

    if (ghb_value_type(gval) == GHB_STRING)
    {
        const gchar *str;
        str = ghb_value_get_string(gval);
        for (ii = 0; ii < opts->count; ii++)
        {
            if (strcmp(opts->map[ii].shortOpt, str) == 0 ||
                strcmp(opts->map[ii].option, str) == 0)
            {
                break;
            }
        }
        return ii;
    }
    else if (ghb_value_type(gval) == GHB_INT ||
             ghb_value_type(gval) == GHB_DOUBLE ||
             ghb_value_type(gval) == GHB_BOOL)
    {
        gint64 val;
        val = ghb_value_get_int(gval);
        for (ii = 0; ii < opts->count; ii++)
        {
            if ((gint64)opts->map[ii].ivalue == val)
            {
                break;
            }
        }
        return ii;
    }
    return opts->count;
}

const hb_filter_param_t*
find_param_entry(const hb_filter_param_t *param, const GhbValue *gval)
{
    gint ii;

    if (param == NULL)
        return NULL;

    if (ghb_value_type(gval) == GHB_STRING)
    {
        const gchar *str;
        str = ghb_value_get_string(gval);
        for (ii = 0; param[ii].name != NULL; ii++)
        {
            if (strcmp(param[ii].short_name, str) == 0 ||
                strcmp(param[ii].name, str) == 0)
            {
                return &param[ii];
            }
        }
    }
    else if (ghb_value_type(gval) == GHB_INT ||
             ghb_value_type(gval) == GHB_DOUBLE ||
             ghb_value_type(gval) == GHB_BOOL)
    {
        gint64 val;
        val = ghb_value_get_int(gval);
        for (ii = 0; param[ii].name != NULL; ii++)
        {
            if ((gint64)param[ii].index == val)
            {
                return &param[ii];
            }
        }
    }
    return NULL;
}

static gint
lookup_generic_int(const combo_opts_t *opts, const GhbValue *gval)
{
    gint ii;
    gint result = -1;

    if (opts == NULL)
        return 0;

    ii = find_opt_entry(opts, gval);
    if (ii < opts->count)
    {
        result = opts->map[ii].ivalue;
    }
    return result;
}

static gdouble
lookup_generic_double(const combo_opts_t *opts, const GhbValue *gval)
{
    gint ii;
    gdouble result = -1;

    ii = find_opt_entry(opts, gval);
    if (ii < opts->count)
    {
        result = opts->map[ii].ivalue;
    }
    return result;
}

static const gchar*
lookup_generic_option(const combo_opts_t *opts, const GhbValue *gval)
{
    gint ii;
    const gchar *result = "";

    ii = find_opt_entry(opts, gval);
    if (ii < opts->count)
    {
        result = opts->map[ii].option;
    }
    return result;
}

static gint
lookup_param_int(const hb_filter_param_t *param, const GhbValue *gval)
{
    gint result = -1;

    if (param == NULL)
        return result;

    const hb_filter_param_t *entry;
    entry = find_param_entry(param, gval);
    if (entry != NULL)
    {
        result = entry->index;
    }
    return result;
}

static const gchar*
lookup_param_option(const hb_filter_param_t *param, const GhbValue *gval)
{
    const gchar *result = "";

    const hb_filter_param_t *entry;
    entry = find_param_entry(param, gval);
    if (entry != NULL)
    {
        result = entry->name;
    }
    return result;
}

gint
ghb_find_closest_audio_samplerate(gint irate)
{
    return hb_audio_samplerate_find_closest(irate, HB_ACODEC_INVALID);
}

const iso639_lang_t* ghb_iso639_lookup_by_int(int idx)
{
    return lang_for_index(idx);
}

// Handle for libhb.  Gets set by ghb_backend_init()
static hb_handle_t * h_scan = NULL;
static hb_handle_t * h_queue = NULL;
static hb_handle_t * h_live = NULL;

hb_handle_t* ghb_scan_handle(void)
{
    return h_scan;
}

hb_handle_t* ghb_queue_handle(void)
{
    return h_queue;
}

hb_handle_t* ghb_live_handle(void)
{
    return h_live;
}

gchar*
ghb_get_tmp_dir()
{
    return hb_get_temporary_directory();
}

void
ghb_hb_cleanup(gboolean partial)
{
    char * dir;

    dir = hb_get_temporary_directory();
    del_tree(dir, !partial);
    free(dir);
}

gint
ghb_subtitle_track_source(GhbValue *settings, gint track)
{
    gint title_id, titleindex;
    const hb_title_t *title;

    if (track == -2)
        return IMPORTSRT;
    if (track < 0)
        return VOBSUB;
    title_id = ghb_dict_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL)
        return VOBSUB;

    hb_subtitle_t * sub;
    sub = hb_list_item( title->list_subtitle, track);
    if (sub != NULL)
        return sub->source;
    else
        return VOBSUB;
}

const gchar*
ghb_subtitle_track_lang(GhbValue *settings, gint track)
{
    gint title_id, titleindex;
    const hb_title_t * title;

    title_id = ghb_dict_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL)  // Bad titleindex
        goto fail;
    if (track == -1)
        return ghb_get_user_audio_lang(settings, title, 0);
    if (track < 0)
        goto fail;

    hb_subtitle_t * sub;
    sub = hb_list_item( title->list_subtitle, track);
    if (sub != NULL)
        return sub->iso639_2;

fail:
    return "und";
}

static gboolean find_combo_item_by_int(GtkTreeModel *store, gint value, GtkTreeIter *iter);

static void
grey_combo_box_item(GtkComboBox *combo, gint value, gboolean grey)
{
    GtkListStore *store;
    GtkTreeIter iter;

    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    if (find_combo_item_by_int(GTK_TREE_MODEL(store), value, &iter))
    {
        gtk_list_store_set(store, &iter,
                           1, !grey,
                           -1);
    }
}

static void
grey_builder_combo_box_item(GtkBuilder *builder, const gchar *name, gint value, gboolean grey)
{
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
    grey_combo_box_item(combo, value, grey);
}

void
ghb_mix_opts_filter(GtkComboBox *combo, gint acodec)
{
    g_debug("ghb_mix_opts_filter()\n");

    const hb_mixdown_t *mix;
    for (mix = hb_mixdown_get_next(NULL); mix != NULL;
         mix = hb_mixdown_get_next(mix))
    {
        grey_combo_box_item(combo, mix->amixdown,
                !hb_mixdown_has_codec_support(mix->amixdown, acodec));
    }
}

static void
grey_mix_opts(signal_user_data_t *ud, gint acodec, gint64 layout)
{
    g_debug("grey_mix_opts()\n");

    const hb_mixdown_t *mix;
    for (mix = hb_mixdown_get_next(NULL); mix != NULL;
         mix = hb_mixdown_get_next(mix))
    {
        grey_builder_combo_box_item(ud->builder, "AudioMixdown", mix->amixdown,
                !hb_mixdown_is_supported(mix->amixdown, acodec, layout));
    }
}

static void grey_passthru(signal_user_data_t *ud, hb_audio_config_t *aconfig)
{
    const hb_encoder_t *enc;

    if (aconfig == NULL)
        return;

    for (enc = hb_audio_encoder_get_next(NULL); enc != NULL;
         enc = hb_audio_encoder_get_next(enc))
    {
        if (!(enc->codec & HB_ACODEC_PASS_FLAG))
            continue;
        if ((enc->codec & HB_ACODEC_MASK) !=
            (aconfig->in.codec & HB_ACODEC_MASK))
        {
            grey_builder_combo_box_item(ud->builder, "AudioEncoder",
                enc->codec, TRUE);
        }
    }
}

void
ghb_grey_combo_options(signal_user_data_t *ud)
{
    gint track, title_id, titleindex, acodec, fallback;
    const hb_title_t *title;
    hb_audio_config_t *aconfig = NULL;

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);

    track = ghb_dict_get_int(ud->settings, "AudioTrack");
    aconfig = ghb_get_audio_info(title, track);

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(ud->settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    const hb_encoder_t *enc;
    for (enc = hb_audio_encoder_get_next(NULL); enc != NULL;
         enc = hb_audio_encoder_get_next(enc))
    {
        if (!(mux->format & enc->muxers) && enc->codec != HB_ACODEC_NONE)
        {
            grey_builder_combo_box_item(ud->builder, "AudioEncoder",
                enc->codec, TRUE);
            grey_builder_combo_box_item(ud->builder, "AudioEncoderFallback",
                enc->codec, TRUE);
        }
        else
        {
            grey_builder_combo_box_item(ud->builder, "AudioEncoder",
                enc->codec, FALSE);
            grey_builder_combo_box_item(ud->builder, "AudioEncoderFallback",
                enc->codec, FALSE);
        }
    }
    for (enc = hb_video_encoder_get_next(NULL); enc != NULL;
         enc = hb_video_encoder_get_next(enc))
    {
        if (!(mux->format & enc->muxers))
        {
            grey_builder_combo_box_item(ud->builder, "VideoEncoder",
                enc->codec, TRUE);
        }
        else
        {
            grey_builder_combo_box_item(ud->builder, "VideoEncoder",
                enc->codec, FALSE);
        }
    }
    grey_passthru(ud, aconfig);

    acodec = ghb_settings_audio_encoder_codec(ud->settings, "AudioEncoder");

    gint64 layout = aconfig != NULL ? aconfig->in.channel_layout : ~0;
    guint32 in_codec = aconfig != NULL ? aconfig->in.codec : 0;
    fallback = ghb_select_fallback(ud->settings, acodec);
    gint copy_mask = ghb_get_copy_mask(ud->settings);
    acodec = ghb_select_audio_codec(mux->format, in_codec, acodec,
                                    fallback, copy_mask);
    grey_mix_opts(ud, acodec, layout);
}

gint
ghb_get_best_mix(uint64_t layout, gint acodec, gint mix)
{
    if (mix == HB_AMIXDOWN_NONE)
        mix = HB_INVALID_AMIXDOWN;

    return hb_mixdown_get_best(acodec, layout, mix);
}

// Set up the model for the combo box
void
ghb_init_combo_box(GtkComboBox *combo)
{
    GtkListStore *store;
    GtkCellRenderer *cell;

    g_debug("ghb_init_combo_box()\n");
    // First modify the combobox model to allow greying out of options
    if (combo == NULL)
        return;
    // Store contains:
    // 1 - String to display
    // 2 - bool indicating whether the entry is selectable (grey or not)
    // 3 - String that is used for presets
    // 4 - Int value determined by backend
    store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_BOOLEAN,
                               G_TYPE_STRING, G_TYPE_DOUBLE);
    gtk_combo_box_set_model(combo, GTK_TREE_MODEL(store));

    if (!gtk_combo_box_get_has_entry(combo))
    {
        gtk_cell_layout_clear(GTK_CELL_LAYOUT(combo));
        cell = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
        g_object_set(cell, "max-width-chars", 80, NULL);
        g_object_set(cell, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cell, FALSE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cell,
            "markup", 0, "sensitive", 1, NULL);
    }
    else
    { // Combo box entry
        gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(combo), 0);
    }
}

// Set up the model for the combo box
static void
init_combo_box(GtkBuilder *builder, const gchar *name)
{
    GtkComboBox *combo;

    g_debug("init_combo_box() %s\n", name);
    // First modify the combobox model to allow greying out of options
    combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
    ghb_init_combo_box(combo);
}

void
ghb_audio_samplerate_opts_set(GtkComboBox *combo)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *str;

    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);
    // Add an item for "Same As Source"
    gtk_list_store_append(store, &iter);
    str = g_strdup_printf("<small>%s</small>", _("Same as source"));
    gtk_list_store_set(store, &iter,
                       0, str,
                       1, TRUE,
                       2, "auto",
                       3, 0.0,
                       -1);
    g_free(str);

    const hb_rate_t *rate;
    for (rate = hb_audio_samplerate_get_next(NULL); rate != NULL;
         rate = hb_audio_samplerate_get_next(rate))
    {
        gtk_list_store_append(store, &iter);
        str = g_strdup_printf("<small>%s</small>", rate->name);
        gtk_list_store_set(store, &iter,
                           0, str,
                           1, TRUE,
                           2, rate->name,
                           3, (gdouble)rate->rate,
                           -1);
        g_free(str);
    }
}

static void
audio_samplerate_opts_set(signal_user_data_t *ud, const gchar *name,
                          void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    ghb_audio_samplerate_opts_set(combo);
}

void
ghb_audio_samplerate_opts_filter(GtkComboBox *combo, gint acodec)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gdouble irate;

    store = GTK_LIST_STORE(gtk_combo_box_get_model(combo));
    if (!gtk_tree_model_get_iter_first( GTK_TREE_MODEL(store), &iter))
        return;

    do
    {
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 3, &irate, -1);
        // If irate == 0.0, the item is the "Same as Source" item,
        // so set to TRUE. Otherwise, ask libhb
        gtk_list_store_set(store, &iter, 1, irate == 0.0 ? TRUE :
                hb_audio_samplerate_is_supported(irate, acodec), -1);
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
}


const hb_rate_t sas_rate =
{
    .name = N_("Same as source"),
    .rate = 0,
};

const char*
ghb_audio_samplerate_get_short_name(int rate)
{
    const char *name;
    name = hb_audio_samplerate_get_name(rate);
    if (name == NULL)
        name = "auto";
    return name;
}

const hb_rate_t*
ghb_lookup_audio_samplerate(const char *name)
{
    // Check for special "Same as source" value
    if (!strncmp(name, "auto", 8))
        return &sas_rate;

    // First find an enabled rate
    int rate = hb_audio_samplerate_get_from_name(name);

    // Now find the matching rate info
    const hb_rate_t *hb_rate, *first;
    for (first = hb_rate = hb_audio_samplerate_get_next(NULL); hb_rate != NULL;
         hb_rate = hb_audio_samplerate_get_next(hb_rate))
    {
        if (rate == hb_rate->rate)
        {
            return hb_rate;
        }
    }
    // Return a default rate if nothing matches
    return first;
}

int
ghb_lookup_audio_samplerate_rate(const char *name)
{
    return ghb_lookup_audio_samplerate(name)->rate;
}

int
ghb_settings_audio_samplerate_rate(const GhbValue *settings, const char *name)
{
    int result;
    char *rate_id = ghb_dict_get_string_xform(settings, name);
    result = ghb_lookup_audio_samplerate_rate(rate_id);
    g_free(rate_id);
    return result;
}

const hb_rate_t*
ghb_settings_audio_samplerate(const GhbValue *settings, const char *name)
{
    const hb_rate_t *result;
    char *rate_id = ghb_dict_get_string_xform(settings, name);
    result = ghb_lookup_audio_samplerate(rate_id);
    g_free(rate_id);
    return result;
}

static void
video_framerate_opts_set(signal_user_data_t *ud, const gchar *name,
                         void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning
    GtkTreeIter iter;
    GtkListStore *store;

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);
    // Add an item for "Same As Source"
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       0, _("Same as source"),
                       1, TRUE,
                       2, "auto",
                       3, 0.0,
                       -1);

    const hb_rate_t *rate;
    for (rate = hb_video_framerate_get_next(NULL); rate != NULL;
         rate = hb_video_framerate_get_next(rate))
    {
        gchar *desc = "";
        gchar *option;
        if (strcmp(rate->name, "23.976") == 0)
        {
            desc = _("(NTSC Film)");
        }
        else if (strcmp(rate->name, "25") == 0)
        {
            desc = _("(PAL Film/Video)");
        }
        else if (strcmp(rate->name, "29.97") == 0)
        {
            desc = _("(NTSC Video)");
        }
        option = g_strdup_printf ("%s %s", rate->name, desc);
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, option,
                           1, TRUE,
                           2, rate->name,
                           3, (gdouble)rate->rate,
                           -1);
        g_free(option);
    }
}

const hb_rate_t*
ghb_lookup_video_framerate(const char *name)
{
    // Check for special "Same as source" value
    if (!strncmp(name, "auto", 8))
        return &sas_rate;

    // First find an enabled rate
    int rate = hb_video_framerate_get_from_name(name);

    // Now find the matching rate info
    const hb_rate_t *hb_rate, *first;
    for (first = hb_rate = hb_video_framerate_get_next(NULL); hb_rate != NULL;
         hb_rate = hb_video_framerate_get_next(hb_rate))
    {
        if (rate == hb_rate->rate)
        {
            return hb_rate;
        }
    }
    // Return a default rate if nothing matches
    return first;
}

int
ghb_lookup_video_framerate_rate(const char *name)
{
    return ghb_lookup_video_framerate(name)->rate;
}

int
ghb_settings_video_framerate_rate(const GhbValue *settings, const char *name)
{
    int result;
    char *rate_id = ghb_dict_get_string_xform(settings, name);
    result = ghb_lookup_video_framerate_rate(rate_id);
    g_free(rate_id);
    return result;
}

const hb_rate_t*
ghb_settings_video_framerate(const GhbValue *settings, const char *name)
{
    const hb_rate_t *result;
    char *rate_id = ghb_dict_get_string_xform(settings, name);
    result = ghb_lookup_video_framerate(rate_id);
    g_free(rate_id);
    return result;
}

static void
video_encoder_opts_set(signal_user_data_t *ud, const gchar *name,
                       void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *str;

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);

    const hb_encoder_t *enc;
    for (enc = hb_video_encoder_get_next(NULL); enc != NULL;
         enc = hb_video_encoder_get_next(enc))
    {
        gtk_list_store_append(store, &iter);
        str = g_strdup_printf("<small>%s</small>", enc->name);
        gtk_list_store_set(store, &iter,
                           0, str,
                           1, TRUE,
                           2, enc->short_name,
                           3, (gdouble)enc->codec,
                           -1);
        g_free(str);
    }
}

const hb_encoder_t*
ghb_lookup_video_encoder(const char *name)
{
    // First find an enabled encoder
    int codec = hb_video_encoder_get_from_name(name);

    // Now find the matching encoder info
    const hb_encoder_t *enc, *first;
    for (first = enc = hb_video_encoder_get_next(NULL); enc != NULL;
         enc = hb_video_encoder_get_next(enc))
    {
        if (codec == enc->codec)
        {
            return enc;
        }
    }
    // Return a default encoder if nothing matches
    return first;
}

int
ghb_lookup_video_encoder_codec(const char *name)
{
    return ghb_lookup_video_encoder(name)->codec;
}

int
ghb_settings_video_encoder_codec(const GhbValue *settings, const char *name)
{
    const char *encoder_id = ghb_dict_get_string(settings, name);
    return ghb_lookup_video_encoder_codec(encoder_id);
}

const hb_encoder_t*
ghb_settings_video_encoder(const GhbValue *settings, const char *name)
{
    const char *encoder_id = ghb_dict_get_string(settings, name);
    return ghb_lookup_video_encoder(encoder_id);
}

void
ghb_audio_encoder_opts_set_with_mask(
    GtkComboBox *combo,
    int mask,
    int neg_mask)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *str;

    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);

    const hb_encoder_t *enc;
    for (enc = hb_audio_encoder_get_next(NULL); enc != NULL;
         enc = hb_audio_encoder_get_next(enc))
    {
        if ((mask & enc->codec) && !(neg_mask & enc->codec) &&
            enc->codec != HB_ACODEC_AUTO_PASS)
        {
            gtk_list_store_append(store, &iter);
            str = g_strdup_printf("<small>%s</small>", enc->name);
            gtk_list_store_set(store, &iter,
                               0, str,
                               1, TRUE,
                               2, enc->short_name,
                               3, (gdouble)enc->codec,
                               -1);
            g_free(str);
        }
    }
}

void
ghb_audio_encoder_opts_add_autopass(GtkComboBox *combo)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *str;
    const hb_encoder_t *enc;

    enc = hb_audio_encoder_get_from_codec(HB_ACODEC_AUTO_PASS);
    if (enc != NULL)
    {
        store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
        gtk_list_store_append(store, &iter);
        str = g_strdup_printf("<small>%s</small>", enc->name);
        gtk_list_store_set(store, &iter,
                           0, str,
                           1, TRUE,
                           2, enc->short_name,
                           3, (gdouble)enc->codec,
                           -1);
        g_free(str);
    }
}

const hb_encoder_t*
ghb_lookup_audio_encoder(const char *name)
{
    // First find an enabled encoder
    int codec = hb_audio_encoder_get_from_name(name);

    // Now find the matching encoder info
    const hb_encoder_t *enc, *first;
    for (first = enc = hb_audio_encoder_get_next(NULL); enc != NULL;
         enc = hb_audio_encoder_get_next(enc))
    {
        if (codec == enc->codec)
        {
            return enc;
        }
    }
    // Return a default encoder if nothing matches
    return first;
}

int
ghb_lookup_audio_encoder_codec(const char *name)
{
    return ghb_lookup_audio_encoder(name)->codec;
}

int
ghb_settings_audio_encoder_codec(const GhbValue *settings, const char *name)
{
    const char *encoder_id = ghb_dict_get_string(settings, name);
    return ghb_lookup_audio_encoder_codec(encoder_id);
}

const hb_encoder_t*
ghb_settings_audio_encoder(const GhbValue *settings, const char *name)
{
    const char *encoder_id = ghb_dict_get_string(settings, name);
    return ghb_lookup_audio_encoder(encoder_id);
}

static void
audio_encoder_opts_set_with_mask(
    GtkBuilder *builder,
    const gchar *name,
    int mask,
    int neg_mask)
{
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
    ghb_audio_encoder_opts_set_with_mask(combo, mask, neg_mask);
}

void
ghb_audio_encoder_opts_set(GtkComboBox *combo)
{
    ghb_audio_encoder_opts_set_with_mask(combo, ~0, HB_ACODEC_NONE);
    ghb_audio_encoder_opts_add_autopass(combo);
}

static void
audio_encoder_opts_set(signal_user_data_t *ud, const gchar *name,
                       void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    ghb_audio_encoder_opts_set_with_mask(combo, ~0, HB_ACODEC_NONE);
}

static void
acodec_fallback_opts_set(signal_user_data_t *ud, const gchar *name,
                         void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning

    audio_encoder_opts_set_with_mask(ud->builder, name, ~0, HB_ACODEC_PASS_FLAG);
}

void
ghb_mix_opts_set(GtkComboBox *combo)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *str;

    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);

    const hb_mixdown_t *mix;
    for (mix = hb_mixdown_get_next(NULL); mix != NULL;
         mix = hb_mixdown_get_next(mix))
    {
        gtk_list_store_append(store, &iter);
        str = g_strdup_printf("<small>%s</small>", mix->name);
        gtk_list_store_set(store, &iter,
                           0, str,
                           1, TRUE,
                           2, mix->short_name,
                           3, (gdouble)mix->amixdown,
                           -1);
        g_free(str);
    }
}

const hb_mixdown_t*
ghb_lookup_mixdown(const char *name)
{
    // First find an enabled mixdown
    int mix = hb_mixdown_get_from_name(name);

    // Now find the matching mixdown info
    const hb_mixdown_t *mixdown, *first;
    for (first = mixdown = hb_mixdown_get_next(NULL); mixdown != NULL;
         mixdown = hb_mixdown_get_next(mixdown))
    {
        if (mix == mixdown->amixdown)
        {
            return mixdown;
        }
    }
    // Return a default mixdown if nothing matches
    return first;
}

int
ghb_lookup_mixdown_mix(const char *name)
{
    return ghb_lookup_mixdown(name)->amixdown;
}

int
ghb_settings_mixdown_mix(const GhbValue *settings, const char *name)
{
    const char *mixdown_id = ghb_dict_get_string(settings, name);
    return ghb_lookup_mixdown_mix(mixdown_id);
}

const hb_mixdown_t*
ghb_settings_mixdown(const GhbValue *settings, const char *name)
{
    const char *mixdown_id = ghb_dict_get_string(settings, name);
    return ghb_lookup_mixdown(mixdown_id);
}

static void
mix_opts_set(signal_user_data_t *ud, const gchar *name,
             void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    ghb_mix_opts_set(combo);
}

static void
container_opts_set(signal_user_data_t *ud, const gchar *name,
                   void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *str;

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);

    const hb_container_t *mux;
    for (mux = hb_container_get_next(NULL); mux != NULL;
         mux = hb_container_get_next(mux))
    {
        gtk_list_store_append(store, &iter);
        str = g_strdup_printf("<small>%s</small>", mux->name);
        gtk_list_store_set(store, &iter,
                           0, str,
                           1, TRUE,
                           2, mux->short_name,
                           3, (gdouble)mux->format,
                           -1);
        g_free(str);
    }
}

static void
preset_category_opts_set(signal_user_data_t *ud, const gchar *name,
                         void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning
    GtkTreeIter     iter;
    GtkListStore  * store;
    gint            ii, jj, count;
    hb_value_t    * presets;
    GtkComboBox   * combo;
    char         ** categories;

    presets = hb_presets_get();
    count   = hb_value_array_len(presets);

    combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);

    categories = calloc(count + 1, sizeof(char*));
    for (ii = 0, jj = 0; ii < count; ii++)
    {
        const char * name;
        hb_value_t * folder = hb_value_array_get(presets, ii);

        if (!hb_value_get_bool(hb_dict_get(folder, "Folder")))
        {
            // Only list folders
            continue;
        }

        name = hb_value_get_string(hb_dict_get(folder, "PresetName"));
        if (name == NULL || name[0] == 0)
        {
            continue;
        }

        if (ghb_strv_contains((const char**)categories, name))
        {
            // Category is already in the list
            continue;
        }

        categories[jj++] = g_strdup(name);
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, name,
                           1, TRUE,
                           2, name,
                           3, (gdouble)ii,
                           -1);
    }
    g_strfreev(categories);

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       0, "Add New Category",
                       1, TRUE,
                       2, "new",
                       3, -1.0,
                       -1);
}

const hb_container_t*
ghb_lookup_container_by_name(const gchar *name)
{
    // First find an enabled muxer
    int format = hb_container_get_from_name(name);

    // Now find the matching muxer info
    const hb_container_t *mux, *first;
    for (first = mux = hb_container_get_next(NULL); mux != NULL;
         mux = hb_container_get_next(mux))
    {
        if (format == mux->format)
        {
            return mux;
        }
    }
    // Return a default container if nothing matches
    return first;
}

static void
srt_codeset_opts_set(signal_user_data_t *ud, const gchar *name,
                     void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii;

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);
    for (ii = 0; ii < SRT_TABLE_SIZE; ii++)
    {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, srt_codeset_table[ii],
                           1, TRUE,
                           2, srt_codeset_table[ii],
                           3, (gdouble)ii,
                           -1);
    }
}

extern G_MODULE_EXPORT void combo_search_key_press_cb(void);

static void
language_opts_set(signal_user_data_t *ud, const gchar *name,
                  void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii;

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);
    const iso639_lang_t *iso639;
    for (iso639 = lang_get_next(NULL), ii = 0; iso639 != NULL;
         iso639 = lang_get_next(iso639), ii++)
    {
        gchar *lang;

        if (iso639->native_name[0] != 0)
            lang = g_strdup_printf("%s", iso639->native_name);
        else
            lang = g_strdup_printf("%s", iso639->eng_name);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, lang,
                           1, TRUE,
                           2, iso639->iso639_2,
                           3, (gdouble)ii,
                           -1);
        g_free(lang);
    }
#if !GTK_CHECK_VERSION(3, 90, 0)
    // This is handled by GtkEventControllerKey in gtk4
    // Initialized in ghb_combo_init()
    g_signal_connect(combo, "key-press-event", combo_search_key_press_cb, ud);
#endif
}

static void
ghb_dvd_sanitize_volname(gchar *name)
{
    gchar *a, *b;

    a = b = name;
    while (*b)
    {
        switch(*b)
        {
        case '<':
            b++;
            break;
        case '>':
            b++;
            break;
        default:
            *a = *b & 0x7f;
            a++; b++;
            break;
        }
    }
    *a = 0;
}

gchar*
ghb_create_source_label(const hb_title_t * title)
{
    char * volname;
    char * source;

    if (title != NULL && title->name != NULL && title->name[0] != 0)
    {
        volname = strdup(title->name);
        if (title->type == HB_DVD_TYPE)
        {
            ghb_dvd_sanitize_volname(volname);
        }
        if (title->type == HB_BD_TYPE)
        {
            source = g_strdup_printf(_("%s - (%05d.MPLS)"),
                                     volname, title->playlist);
            g_free(volname);
        }
        else
        {
            source = volname;
        }
    }
    else
    {
        source = g_strdup(_("No Title Found"));
    }
    return source;
}

static gboolean
uppers_and_unders(gchar *str)
{
    if (str == NULL) return FALSE;
    str = g_strchomp(g_strchug(str));
    while (*str)
    {
        if (*str == ' ')
        {
            return FALSE;
        }
        if (*str >= 'a' && *str <= 'z')
        {
            return FALSE;
        }
        str++;
    }
    return TRUE;
}

enum
{
    CAMEL_FIRST_UPPER,
    CAMEL_OTHER
};

static void
camel_convert(gchar *str)
{
    gint state = CAMEL_OTHER;

    if (str == NULL) return;
    while (*str)
    {
        if (*str == '_') *str = ' ';
        switch (state)
        {
            case CAMEL_OTHER:
            {
                if (*str >= 'A' && *str <= 'Z')
                    state = CAMEL_FIRST_UPPER;
                else
                    state = CAMEL_OTHER;

            } break;
            case CAMEL_FIRST_UPPER:
            {
                if (*str >= 'A' && *str <= 'Z')
                    *str = *str - 'A' + 'a';
                else
                    state = CAMEL_OTHER;
            } break;
        }
        str++;
    }
}

static gchar*
get_file_label(const gchar *filename)
{
    gchar *base, *pos, *end;

    base = g_path_get_basename(filename);
    pos = strrchr(base, '.');
    if (pos != NULL)
    {
        // If the last '.' is within 4 chars of end of name, assume
        // there is an extension we want to strip.
        end = &base[strlen(base) - 1];
        if (end - pos <= 4)
            *pos = 0;
    }
    return base;
}

gchar*
ghb_create_volume_label(const hb_title_t * title)
{
    char * volname = NULL;

    if (title != NULL && title->name != NULL && title->name[0] != 0)
    {
        GStatBuf stat_buf;

        if (g_stat(title->path, &stat_buf) == 0)
        {
            if (!S_ISBLK(stat_buf.st_mode))
            {
                volname = get_file_label(title->path);
            }
            else
            {
                // DVD and BD volume labels are often all upper case
                volname = strdup(title->name);
                if (title->type == HB_DVD_TYPE)
                {
                    ghb_dvd_sanitize_volname(volname);
                }
                if (uppers_and_unders(volname))
                {
                    camel_convert(volname);
                }
            }
        }
        if (volname == NULL)
        {
            volname = strdup(title->name);
            if (title->type == HB_DVD_TYPE)
            {
                ghb_dvd_sanitize_volname(volname);
            }
        }
    }
    else
    {
        volname = g_strdup(_("No Title Found"));
    }
    return volname;
}

gchar*
ghb_create_title_label(const hb_title_t *title)
{
    gchar *label;

    if (title == NULL)
    {
        return g_strdup(_("No Title Found"));
    }
    if (title->type == HB_STREAM_TYPE || title->type == HB_FF_STREAM_TYPE)
    {
        if (title->duration != 0)
        {
            char *tmp;
            tmp  = g_strdup_printf (_("%3d - %02dh%02dm%02ds - %s"),
                title->index, title->hours, title->minutes, title->seconds,
                title->name);
            label = g_markup_escape_text(tmp, -1);
            g_free(tmp);
        }
        else
        {
            char *tmp;
            tmp  = g_strdup_printf ("%3d - %s",
                                    title->index, title->name);
            label = g_markup_escape_text(tmp, -1);
            g_free(tmp);
        }
    }
    else if (title->type == HB_BD_TYPE)
    {
        if (title->duration != 0)
        {
            label = g_strdup_printf(_("%3d - %02dh%02dm%02ds - (%05d.MPLS)"),
                                    title->index, title->hours, title->minutes,
                                    title->seconds, title->playlist);
        }
        else
        {
            label = g_strdup_printf(_("%3d - (%05d.MPLS)"),
                title->index, title->playlist);
        }
    }
    else
    {
        label  = g_strdup_printf(_("%3d - %02dh%02dm%02ds"),
            title->index, title->hours, title->minutes, title->seconds);
    }
    return label;
}

static void
title_opts_set(signal_user_data_t *ud, const gchar *name,
               void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning
    GtkTreeIter iter;
    GtkListStore *store;
    hb_list_t  * list = NULL;
    const hb_title_t * title = NULL;
    gint ii;
    gint count = 0;


    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);
    if (h_scan != NULL)
    {
        list = hb_get_titles( h_scan );
        count = hb_list_count( list );
    }
    if( count <= 0 )
    {
        char *opt;

        // No titles.  Fill in a default.
        gtk_list_store_append(store, &iter);
        opt = g_strdup_printf("<small>%s</small>", _("No Titles"));
        gtk_list_store_set(store, &iter,
                           0, opt,
                           1, TRUE,
                           2, "none",
                           3, -1.0,
                           -1);
        g_free(opt);
        return;
    }
    for (ii = 0; ii < count; ii++)
    {
        char *title_opt, *title_index, *opt;

        title = hb_list_item(list, ii);
        title_opt = ghb_create_title_label(title);
        opt = g_strdup_printf("<small>%s</small>", title_opt);
        title_index = g_strdup_printf("%d", title->index);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, opt,
                           1, TRUE,
                           2, title_index,
                           3, (gdouble)title->index,
                           -1);
        g_free(opt);
        g_free(title_opt);
        g_free(title_index);
    }
}

static int
lookup_title_index(hb_handle_t *h, int title_id)
{
    if (h == NULL)
        return -1;

    hb_list_t *list;
    const hb_title_t *title;
    int count, ii;

    list = hb_get_titles(h);
    count = hb_list_count(list);
    for (ii = 0; ii < count; ii++)
    {
        title = hb_list_item(list, ii);
        if (title_id == title->index)
        {
            return ii;
        }
    }
    return -1;
}

const hb_title_t*
lookup_title(hb_handle_t *h, int title_id, int *index)
{
    int ii = lookup_title_index(h, title_id);

    if (index != NULL)
        *index = ii;
    if (ii < 0)
        return NULL;

    hb_list_t *list;
    list = hb_get_titles(h);
    return hb_list_item(list, ii);
}

int
ghb_lookup_title_index(int title_id)
{
    return lookup_title_index(h_scan, title_id);
}

const hb_title_t*
ghb_lookup_title(int title_id, int *index)
{
    return lookup_title(h_scan, title_id, index);
}

GhbValue*
ghb_get_title_dict(int title_id)
{
    return hb_title_to_dict(h_scan, title_id);
}

int
ghb_lookup_queue_title_index(int title_id)
{
    return lookup_title_index(h_queue, title_id);
}

const hb_title_t*
ghb_lookup_queue_title(int title_id, int *index)
{
    return lookup_title(h_queue, title_id, index);
}

static void
video_tune_opts_set(signal_user_data_t *ud, const gchar *name,
                    void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii, count = 0;

    // Check if encoder has been set yet.
    // If not, bail
    GhbValue *value = ghb_dict_get(ud->settings, "VideoEncoder");
    if (value == NULL) return;

    int encoder = ghb_get_video_encoder(ud->settings);
    const char * const *tunes;
    tunes = hb_video_encoder_get_tunes(encoder);

    while (tunes && tunes[count]) count++;
    GtkWidget *w = GHB_WIDGET(ud->builder, "VideoTune");
    gtk_widget_set_visible(w, count > 0);
    w = GHB_WIDGET(ud->builder, "VideoTuneLabel");
    gtk_widget_set_visible(w, count > 0);
    if (count == 0) return;

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       0, _("None"),
                       1, TRUE,
                       2, "none",
                       3, (gdouble)0,
                       -1);

    for (ii = 0; ii < count; ii++)
    {
        if (((encoder & HB_VCODEC_X264_MASK) &&
             !strcmp(tunes[ii], "fastdecode")) ||
            ((encoder & (HB_VCODEC_X264_MASK | HB_VCODEC_X265_MASK)) &&
             !strcmp(tunes[ii], "zerolatency")))
        {
            continue;
        }
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, tunes[ii],
                           1, TRUE,
                           2, tunes[ii],
                           3, (gdouble)ii + 1,
                           -1);
    }
}

static void
video_profile_opts_set(signal_user_data_t *ud, const gchar *name,
                       void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii, count = 0;

    // Check if encoder has been set yet.
    // If not, bail
    GhbValue *value = ghb_dict_get(ud->settings, "VideoEncoder");
    if (value == NULL) return;

    int encoder = ghb_get_video_encoder(ud->settings);
    const char * const *profiles;
    profiles = hb_video_encoder_get_profiles(encoder);

    while (profiles && profiles[count]) count++;
    GtkWidget *w = GHB_WIDGET(ud->builder, "VideoProfile");
    gtk_widget_set_visible(w, count > 0);
    w = GHB_WIDGET(ud->builder, "VideoProfileLabel");
    gtk_widget_set_visible(w, count > 0);
    if (count == 0) return;

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);

    for (ii = 0; ii < count; ii++)
    {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, profiles[ii],
                           1, TRUE,
                           2, profiles[ii],
                           3, (gdouble)ii,
                           -1);
    }
}

static void
video_level_opts_set(signal_user_data_t *ud, const gchar *name,
                     void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii, count = 0;

    // Check if encoder has been set yet.
    // If not, bail
    GhbValue *value = ghb_dict_get(ud->settings, "VideoEncoder");
    if (value == NULL) return;

    int encoder = ghb_get_video_encoder(ud->settings);
    const char * const *levels;
    levels = hb_video_encoder_get_levels(encoder);

    while (levels && levels[count]) count++;
    GtkWidget *w = GHB_WIDGET(ud->builder, "VideoLevel");
    gtk_widget_set_visible(w, count > 0);
    w = GHB_WIDGET(ud->builder, "VideoLevelLabel");
    gtk_widget_set_visible(w, count > 0);
    if (count <= 0) return;

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);

    for (ii = 0; ii < count; ii++)
    {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, levels[ii],
                           1, TRUE,
                           2, levels[ii],
                           3, (gdouble)ii,
                           -1);
    }
}

static gboolean
find_combo_item_by_int(GtkTreeModel *store, gint value, GtkTreeIter *iter)
{
    gdouble ivalue;
    gboolean foundit = FALSE;

    if (gtk_tree_model_get_iter_first (store, iter))
    {
        do
        {
            gtk_tree_model_get(store, iter, 3, &ivalue, -1);
            if (value == (gint)ivalue)
            {
                foundit = TRUE;
                break;
            }
        } while (gtk_tree_model_iter_next (store, iter));
    }
    return foundit;
}

void
audio_track_opts_set(signal_user_data_t *ud, const gchar *name,
                     void *opts, const void* data)
{
    (void)opts;   // Silence "unused variable" warning
    const hb_title_t *title = (const hb_title_t*)data;
    GtkTreeIter iter;
    GtkListStore *store;
    hb_audio_config_t * audio;
    gint ii;
    gint count = 0;
    gchar *opt;

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);
    if (title != NULL)
    {
        count = hb_list_count( title->list_audio );
    }
    if( count <= 0 )
    {
        // No audio. set some default
        opt = g_strdup_printf("<small>%s</small>", _("No Audio"));

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, opt,
                           1, TRUE,
                           2, "none",
                           3, -1.0,
                           -1);
        g_free(opt);
        return;
    }
    for (ii = 0; ii < count; ii++)
    {
        char idx[4];
        audio = hb_list_audio_config_item(title->list_audio, ii);
        opt = g_strdup_printf("<small>%d - %s</small>",
                              ii + 1, audio->lang.description);
        snprintf(idx, 4, "%d", ii);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, opt,
                           1, TRUE,
                           2, idx,
                           3, (gdouble)ii,
                           -1);
        g_free(opt);
    }
    gtk_combo_box_set_active (combo, 0);
}

static void
subtitle_track_opts_set(signal_user_data_t *ud, const gchar *name,
                        void *opts, const void* data)
{
    (void)opts;   // Silence "unused variable" warning
    const hb_title_t *title = (const hb_title_t*)data;
    GtkTreeIter iter;
    GtkListStore *store;
    hb_subtitle_t * subtitle;
    gint ii, count = 0;

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);

    if (title != NULL)
    {
        count = hb_list_count( title->list_subtitle );
    }
    for (ii = 0; ii < count; ii++)
    {
        gchar *opt;
        char idx[4];

        subtitle = hb_list_item(title->list_subtitle, ii);
        opt = g_strdup_printf("%d - %s", ii+1, subtitle->lang);
        snprintf(idx, 4, "%d", ii);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                    0, opt,
                    1, TRUE,
                    2, idx,
                    3, (gdouble)ii,
                    -1);
        g_free(opt);
    }
    if (count <= 0)
    {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, _("None"),
                           1, TRUE,
                           2, "0",
                           3, 0.0,
                           -1);
    }
    gtk_combo_box_set_active (combo, 0);
}

// Get title id of feature or longest title
gint
ghb_longest_title()
{
    hb_title_set_t * title_set;
    const hb_title_t * title;
    gint count = 0, ii, longest = -1;
    int64_t duration = 0;

    g_debug("ghb_longest_title ()\n");
    if (h_scan == NULL) return 0;
    title_set = hb_get_title_set( h_scan );
    count = hb_list_count( title_set->list_title );
    if (count < 1) return -1;

    // Check that the feature title in the title_set exists in the list
    // of titles.  If not, pick the longest.
    for (ii = 0; ii < count; ii++)
    {
        title = hb_list_item(title_set->list_title, ii);
        if (title->index == title_set->feature)
            return title_set->feature;
        if (title->duration > duration)
            longest = title->index;
    }
    return longest;
}

const gchar*
ghb_get_source_audio_lang(const hb_title_t *title, gint track)
{
    hb_audio_config_t * audio;
    const gchar *lang = "und";

    g_debug("ghb_lookup_1st_audio_lang ()\n");
    if (title == NULL)
        return lang;
    if (hb_list_count( title->list_audio ) <= track)
        return lang;

    audio = hb_list_audio_config_item(title->list_audio, track);
    if (audio == NULL)
        return lang;

    lang = audio->lang.iso639_2;
    return lang;
}

gint
ghb_find_audio_track(const hb_title_t *title, const gchar *lang, int start)
{
    hb_audio_config_t * audio;
    gint ii, count = 0;

    if (title != NULL)
    {
        count = hb_list_count(title->list_audio);
    }

    for (ii = start; ii < count; ii++)
    {
        audio = hb_list_audio_config_item(title->list_audio, ii);
        if (!strcmp(lang, audio->lang.iso639_2) || !strcmp(lang, "und"))
        {
            return ii;
        }
    }
    return -1;
}

gint
ghb_find_subtitle_track(const hb_title_t * title, const gchar * lang, int start)
{
    hb_subtitle_t * subtitle;
    gint count, ii;

    count = hb_list_count(title->list_subtitle);

    // Try to find an item that matches the preferred language
    for (ii = start; ii < count; ii++)
    {
        subtitle = (hb_subtitle_t*)hb_list_item( title->list_subtitle, ii );
        if (strcmp(lang, subtitle->iso639_2) == 0 ||
            strcmp(lang, "und") == 0)
        {
            return ii;
        }
    }
    return -1;
}

static void
small_opts_set(signal_user_data_t *ud, const gchar *name,
               void *vopts, const void* data)
{
    (void)data; // Silence "unused variable" warning
    combo_opts_t *opts = (combo_opts_t*)vopts;
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii;
    gchar *str;

    if (name == NULL || opts == NULL) return;
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);
    for (ii = 0; ii < opts->count; ii++)
    {
        gtk_list_store_append(store, &iter);
        str = g_strdup_printf("<small>%s</small>",
                              gettext(opts->map[ii].option));
        gtk_list_store_set(store, &iter,
                           0, str,
                           1, TRUE,
                           2, opts->map[ii].shortOpt,
                           3, opts->map[ii].ivalue,
                           -1);
        g_free(str);
    }
}

static void
filter_opts_set2(signal_user_data_t *ud, const gchar *name,
                 int filter_id, int preset)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii;
    gchar *str;

    if (name == NULL) return;
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);
    hb_filter_param_t * param;
    if (preset)
    {
        param = hb_filter_param_get_presets(filter_id);
    }
    else
    {
        param = hb_filter_param_get_tunes(filter_id);
    }
    for (ii = 0; param != NULL && param[ii].name != NULL; ii++)
    {
        gtk_list_store_append(store, &iter);
        str = g_strdup_printf("<small>%s</small>",
                              gettext(param[ii].name));
        gtk_list_store_set(store, &iter,
                           0, str,
                           1, TRUE,
                           2, param[ii].short_name,
                           3, (double)param[ii].index,
                           -1);
        g_free(str);
    }
}

static void
filter_opts_set(signal_user_data_t *ud, const gchar *name,
                void *vopts, const void* data)
{
    filter_opts_t *opts = (filter_opts_t*)vopts;

    (void)data; // Silence "unused variable" warning
    filter_opts_set2(ud, name, opts->filter_id, opts->preset);
}

static void
deint_opts_set(signal_user_data_t *ud, const gchar *name,
               void *vopts, const void* data)
{
    (void)data;  // Silence "unused variable" warning

    filter_opts_t *opts = (filter_opts_t*)vopts;
    opts->filter_id = ghb_settings_combo_int(ud->settings,
                                             "PictureDeinterlaceFilter");
    filter_opts_set2(ud, name, opts->filter_id, opts->preset);

    ghb_set_custom_filter_tooltip(ud, "PictureDeinterlaceCustom",
                                  "deinterlace", opts->filter_id);
}

static void
denoise_opts_set(signal_user_data_t *ud, const gchar *name,
               void *vopts, const void* data)
{
    (void)data;  // Silence "unused variable" warning

    filter_opts_t *opts = (filter_opts_t*)vopts;
    opts->filter_id = ghb_settings_combo_int(ud->settings,
                                             "PictureDenoiseFilter");
    filter_opts_set2(ud, name, opts->filter_id, opts->preset);

    ghb_set_custom_filter_tooltip(ud, "PictureDenoiseCustom",
                                  "denoise", opts->filter_id);
}

static void
sharpen_opts_set(signal_user_data_t *ud, const gchar *name,
               void *vopts, const void* data)
{
    (void)data;  // Silence "unused variable" warning

    filter_opts_t *opts = (filter_opts_t*)vopts;
    opts->filter_id = ghb_settings_combo_int(ud->settings,
                                             "PictureSharpenFilter");
    filter_opts_set2(ud, name, opts->filter_id, opts->preset);

    ghb_set_custom_filter_tooltip(ud, "PictureSharpenCustom",
                                  "sharpen", opts->filter_id);
}

combo_name_map_t*
find_combo_map(const gchar *name)
{
    gint ii;

    for (ii = 0; combo_name_map[ii].name != NULL; ii++)
    {
        if (strcmp(name, combo_name_map[ii].name) == 0)
        {
            return &combo_name_map[ii];
        }
    }
    return NULL;
}

combo_opts_t*
find_combo_opts(const gchar *name)
{
    combo_name_map_t *entry = find_combo_map(name);
    if (entry != NULL)
    {
        return entry->opts;
    }
    return NULL;
}

static GhbValue *
generic_opt_get(const char *name, const void *vopts,
                const GhbValue *gval, GhbType type)
{
    combo_opts_t *opts = (combo_opts_t*)vopts;
    GhbValue *result = NULL;
    switch (type)
    {
        case GHB_INT:
        case GHB_BOOL:
        {
            int val;
            val = lookup_generic_int(opts, gval);
            return ghb_int_value_new(val);
        } break;
        case GHB_DOUBLE:
        {
            double val;
            val = lookup_generic_double(opts, gval);
            return ghb_double_value_new(val);
        } break;
        case GHB_STRING:
        {
            const char *val;
            val = lookup_generic_option(opts, gval);
            return ghb_string_value_new(val);
        } break;
    }
    return result;
}

static GhbValue *
filter_opt_get2(const char *name, const GhbValue *gval, GhbType type,
               int filter_id, int preset)
{
    GhbValue *result = NULL;
    hb_filter_param_t * param;

    if (preset)
    {
        param = hb_filter_param_get_presets(filter_id);
    }
    else
    {
        param = hb_filter_param_get_tunes(filter_id);
    }
    switch (type)
    {
        case GHB_DOUBLE:
        case GHB_BOOL:
        case GHB_INT:
        {
            int val;
            val = lookup_param_int(param, gval);
            return ghb_int_value_new(val);
        } break;
        case GHB_STRING:
        {
            const char *val;
            val = lookup_param_option(param, gval);
            return ghb_string_value_new(val);
        } break;
    }
    return result;
}

static GhbValue *
filter_opt_get(const char *name, const void *vopts,
               const GhbValue *gval, GhbType type)
{
    filter_opts_t *opts = (filter_opts_t*)vopts;
    return filter_opt_get2(name, gval, type, opts->filter_id, opts->preset);
}

static GhbValue *
lookup_combo_value(const gchar *name, const GhbValue *gval, GhbType type)
{
    combo_name_map_t *entry = find_combo_map(name);
    if (entry != NULL)
    {
        if (entry->opt_get != NULL)
        {
            return entry->opt_get(name, entry->opts, gval, type);
        }
        else
        {
            g_warning("Combobox entry %s missing opt_get()", name);
        }
    }
    else
    {
        g_warning("Combobox entry %s not found", name);
    }
    return NULL;
}

gint
ghb_lookup_combo_int(const gchar *name, const GhbValue *gval)
{
    if (gval == NULL)
        return 0;
    GhbValue *gresult = lookup_combo_value(name, gval, GHB_INT);
    int result = ghb_value_get_int(gresult);
    ghb_value_free(&gresult);
    return result;
}

gdouble
ghb_lookup_combo_double(const gchar *name, const GhbValue *gval)
{
    if (gval == NULL)
        return 0;
    GhbValue *gresult = lookup_combo_value(name, gval, GHB_DOUBLE);
    double result = ghb_value_get_double(gresult);
    ghb_value_free(&gresult);
    return result;
}

char*
ghb_lookup_combo_option(const gchar *name, const GhbValue *gval)
{
    if (gval == NULL)
        return NULL;
    GhbValue *gresult = lookup_combo_value(name, gval, GHB_STRING);
    const char *tmp = ghb_value_get_string(gresult);
    char *result = NULL;
    if (tmp != NULL)
    {
        result = g_strdup(tmp);
    }
    ghb_value_free(&gresult);
    return result;
}

void ghb_init_lang_list_model(GtkTreeView *tv)
{
    GtkTreeViewColumn * column;
    GtkTreeStore      * ts;
    GtkCellRenderer   * lang_cell;

    // Store contains:
    // 0 - Language string to display
    // 1 - Index of language in the libhb language list
    ts = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model(tv, GTK_TREE_MODEL(ts));

    lang_cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_pack_start(column, lang_cell, FALSE);
    gtk_tree_view_column_add_attribute(column, lang_cell, "markup", 0);
    gtk_tree_view_append_column(tv, GTK_TREE_VIEW_COLUMN(column));
}

void ghb_init_lang_list(GtkTreeView *tv, signal_user_data_t *ud)
{
    GtkTreeIter    iter;
    GtkTreeStore * ts;
    int            ii;

    ghb_init_lang_list_model(tv);
    ts = GTK_TREE_STORE(gtk_tree_view_get_model(tv));

    const iso639_lang_t *iso639;
    for (iso639 = lang_get_next(NULL), ii = 0; iso639 != NULL;
         iso639 = lang_get_next(iso639), ii++)
    {
        const char * lang;
        if (ii == 0)
        {
            lang = _("Any");
        }
        else if (iso639->native_name != NULL &&
                 iso639->native_name[0] != 0)
        {
            lang = iso639->native_name;
        }
        else
        {
            lang = iso639->eng_name;
        }
        gtk_tree_store_append(ts, &iter, NULL);
        gtk_tree_store_set(ts, &iter, 0, lang, 1, ii, -1);
    }
}

void
ghb_update_ui_combo_box(
    signal_user_data_t *ud,
    const gchar *name,
    const void* user_data,
    gboolean all)
{
    GtkComboBox *combo = NULL;
    gint signal_id;
    gint handler_id = 0;

    if (name != NULL)
    {
        // Clearing a combo box causes a rash of "changed" events, even when
        // the active item is -1 (inactive).  To control things, I'm disabling
        // the event till things are settled down.
        combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
        signal_id = g_signal_lookup("changed", GTK_TYPE_COMBO_BOX);
        if (signal_id > 0)
        {
            // Valid signal id found.  This should always succeed.
            handler_id = g_signal_handler_find ((gpointer)combo, G_SIGNAL_MATCH_ID,
                                                signal_id, 0, 0, 0, 0);
            if (handler_id > 0)
            {
                // This should also always succeed
                g_signal_handler_block ((gpointer)combo, handler_id);
            }
        }
    }
    if (all)
    {
        int ii;
        for (ii = 0; combo_name_map[ii].name != NULL; ii++)
        {
            combo_name_map_t *entry = &combo_name_map[ii];
            entry->opts_set(ud, entry->name, entry->opts, user_data);
        }
    }
    else
    {
        combo_name_map_t *entry = find_combo_map(name);
        if (entry != NULL)
        {
            entry->opts_set(ud, entry->name, entry->opts, user_data);
        }
    }
    if (handler_id > 0)
    {
        g_signal_handler_unblock ((gpointer)combo, handler_id);
    }
}

static void
init_ui_combo_boxes(GtkBuilder *builder)
{
    gint ii;

    for (ii = 0; combo_name_map[ii].name != NULL; ii++)
    {
        init_combo_box(builder, combo_name_map[ii].name);
    }
}

gint64
ghb_chapter_range_get_duration(const hb_title_t *title, gint sc, gint ec)
{
    hb_chapter_t * chapter;
    gint count, c;
    gint64 duration;

    if (title == NULL) return 0;

    duration = title->duration;

    count = hb_list_count(title->list_chapter);
    if (sc > count) sc = count;
    if (ec > count) ec = count;

    if (sc == 1 && ec == count)
        return duration;

    duration = 0;
    for (c = sc; c <= ec; c++)
    {
        chapter = hb_list_item(title->list_chapter, c-1);
        duration += chapter->duration;
    }
    return duration;
}

gint64
ghb_get_chapter_duration(const hb_title_t *title, gint chap)
{
    hb_chapter_t * chapter;
    gint count;

    if (title == NULL) return 0;
    count = hb_list_count( title->list_chapter );
    if (chap >= count) return 0;
    chapter = hb_list_item(title->list_chapter, chap);
    if (chapter == NULL) return 0;
    return chapter->duration;
}

gint64
ghb_get_chapter_start(const hb_title_t *title, gint chap)
{
    hb_chapter_t * chapter;
    gint count, ii;
    gint64 start = 0;

    if (title == NULL) return 0;
    count = hb_list_count( title->list_chapter );
    if (chap > count) return chap = count;
    for (ii = 0; ii < chap; ii++)
    {
        chapter = hb_list_item(title->list_chapter, ii);
        start += chapter->duration;
    }
    return start;
}

void
ghb_audio_bitrate_opts_filter(
    GtkComboBox *combo,
    gint first_rate,
    gint last_rate)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gdouble ivalue;
    gboolean done = FALSE;

    g_debug("audio_bitrate_opts_filter ()\n");
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL(store), &iter))
    {
        do
        {
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 3, &ivalue, -1);
            if ((int)ivalue < first_rate || (int)ivalue > last_rate)
            {
                gtk_list_store_set(store, &iter, 1, FALSE, -1);
            }
            else
            {
                gtk_list_store_set(store, &iter, 1, TRUE, -1);
            }
            done = !gtk_tree_model_iter_next (GTK_TREE_MODEL(store), &iter);
        } while (!done);
    }
}

void
ghb_audio_bitrate_opts_set(GtkComboBox *combo)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *str;

    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);

    const hb_rate_t *rate;
    for (rate = hb_audio_bitrate_get_next(NULL); rate != NULL;
         rate = hb_audio_bitrate_get_next(rate))
    {
        gtk_list_store_append(store, &iter);
        str = g_strdup_printf ("<small>%s</small>", rate->name);
        gtk_list_store_set(store, &iter,
                           0, str,
                           1, TRUE,
                           2, rate->name,
                           3, (gdouble)rate->rate,
                           -1);
        g_free(str);
    }
}

static void
audio_bitrate_opts_set(signal_user_data_t *ud, const gchar *name,
                       void *opts, const void* data)
{
    (void)opts; // Silence "unused variable" warning
    (void)data; // Silence "unused variable" warning

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(ud->builder, name));
    ghb_audio_bitrate_opts_set(combo);
}

const char*
ghb_audio_bitrate_get_short_name(int rate)
{
    const hb_rate_t *hb_rate, *first;
    for (first = hb_rate = hb_audio_bitrate_get_next(NULL); hb_rate != NULL;
         hb_rate = hb_audio_bitrate_get_next(hb_rate))
    {
        if (rate == hb_rate->rate)
        {
            return hb_rate->name;
        }
    }
    return first->name;
}

const hb_rate_t*
ghb_lookup_audio_bitrate(const char *name)
{
    // Now find the matching rate info
    const hb_rate_t *hb_rate;
    for (hb_rate = hb_audio_bitrate_get_next(NULL); hb_rate != NULL;
         hb_rate = hb_audio_bitrate_get_next(hb_rate))
    {
        if (!strncmp(name, hb_rate->name, 8))
        {
            return hb_rate;
        }
    }
    // Return a default rate if nothing matches
    return NULL;
}

int
ghb_lookup_audio_bitrate_rate(const char *name)
{
    const hb_rate_t *rate;
    rate = ghb_lookup_audio_bitrate(name);
    if (rate == NULL)
        return 0;
    return rate->rate;
}

int
ghb_settings_audio_bitrate_rate(const GhbValue *settings, const char *name)
{
    int result;
    char *rate_id = ghb_dict_get_string_xform(settings, name);
    result = ghb_lookup_audio_bitrate_rate(rate_id);
    g_free(rate_id);
    return result;
}

const hb_rate_t*
ghb_settings_audio_bitrate(const GhbValue *settings, const char *name)
{
    const hb_rate_t *result;
    char *rate_id = ghb_dict_get_string_xform(settings, name);
    result = ghb_lookup_audio_bitrate(rate_id);
    g_free(rate_id);
    return result;
}

static ghb_status_t hb_status;

void
ghb_combo_init(signal_user_data_t *ud)
{
    // Set up the list model for the combos
    init_ui_combo_boxes(ud->builder);
    // Populate all the combos
    ghb_update_ui_combo_box(ud, NULL, NULL, TRUE);

#if GTK_CHECK_VERSION(3, 90, 0)
    GtkWidget          * combo;
    GtkEventController * econ;

    // Set key-press handler for subtitle import language combo.
    // Pressing a key warps to the next language that starts with that key.
    combo = GHB_WIDGET(ud->builder, "ImportLanguage");
    econ  = gtk_event_controller_key_new();
    gtk_widget_add_controller(combo, econ);
    g_signal_connect(econ, "key-pressed",
                     G_CALLBACK(combo_search_key_press_cb), ud);
#endif
}

void
ghb_backend_init(gint debug)
{
    /* Init libhb */
    hb_global_init();
    h_scan = hb_init( debug );
    h_queue = hb_init( debug );
    h_live = hb_init( debug );
}

void
ghb_log_level_set(int level)
{
    hb_log_level_set(h_scan, level);
    hb_log_level_set(h_queue, level);
    hb_log_level_set(h_live, level);
}

void
ghb_backend_close()
{
    if (h_live != NULL)
        hb_close(&h_live);
    if (h_queue != NULL)
        hb_close(&h_queue);
    if (h_scan != NULL)
        hb_close(&h_scan);
    hb_global_close();
}

void ghb_backend_scan_stop()
{
    hb_scan_stop( h_scan );
}

void
ghb_backend_scan(const gchar *path, gint titleindex, gint preview_count, uint64_t min_duration)
{
    hb_scan( h_scan, path, titleindex, preview_count, 1, min_duration );
    hb_status.scan.state |= GHB_STATE_SCANNING;
    // initialize count and cur to something that won't cause FPE
    // when computing progress
    hb_status.scan.title_count = 1;
    hb_status.scan.title_cur = 0;
    hb_status.scan.preview_count = 1;
    hb_status.scan.preview_cur = 0;
    hb_status.scan.progress = 0;
}

void
ghb_backend_queue_scan(const gchar *path, gint titlenum)
{
    g_debug("ghb_backend_queue_scan()");
    hb_scan( h_queue, path, titlenum, -1, 0, 0 );
    hb_status.queue.state |= GHB_STATE_SCANNING;
}

gint
ghb_get_scan_state()
{
    return hb_status.scan.state;
}

gint
ghb_get_queue_state()
{
    return hb_status.queue.state;
}

void
ghb_clear_scan_state(gint state)
{
    hb_status.scan.state &= ~state;
}

void
ghb_clear_live_state(gint state)
{
    hb_status.live.state &= ~state;
}

void
ghb_clear_queue_state(gint state)
{
    hb_status.queue.state &= ~state;
}

void
ghb_set_scan_state(gint state)
{
    hb_status.scan.state |= state;
}

void
ghb_set_queue_state(gint state)
{
    hb_status.queue.state |= state;
}

void
ghb_get_status(ghb_status_t *status)
{
    memcpy(status, &hb_status, sizeof(ghb_status_t));
}

static void
update_status(hb_state_t *state, ghb_instance_status_t *status)
{
    status->unique_id   = state->sequence_id;

#define p state->param.scanning
    if (state->state & HB_STATE_SCANNING)
    {
        status->state |= GHB_STATE_SCANNING;
        status->title_count = p.title_count;
        status->title_cur = p.title_cur;
        status->preview_count = p.preview_count;
        status->preview_cur = p.preview_cur;
        status->progress = p.progress;
    }
    else
    {
        // If last state seen was scanning, signal that scan id complete
        if (status->state & GHB_STATE_SCANNING)
        {
            status->state |= GHB_STATE_SCANDONE;
        }
        status->state &= ~GHB_STATE_SCANNING;
    }
#undef p
#define p state->param.working
    if (state->state & (HB_STATE_WORKING | HB_STATE_WORKDONE |
                        HB_STATE_SEARCHING))
    {
        status->pass        = p.pass;
        status->pass_count  = p.pass_count;
        status->pass_id     = p.pass_id;
        status->progress    = p.progress;
        status->rate_cur    = p.rate_cur;
        status->rate_avg    = p.rate_avg;
        status->eta_seconds = p.eta_seconds;
        status->hours       = p.hours;
        status->minutes     = p.minutes;
        status->seconds     = p.seconds;
        status->paused      = p.paused;

        if (state->state & HB_STATE_WORKING)
        {
            status->state |= GHB_STATE_WORKING;
            status->state &= ~GHB_STATE_PAUSED;
            status->state &= ~GHB_STATE_SEARCHING;
        }
        else
        {
            status->state &= ~GHB_STATE_WORKING;
        }
        if (state->state & HB_STATE_SEARCHING)
        {
            status->state |= GHB_STATE_SEARCHING;
            status->state &= ~GHB_STATE_WORKING;
            status->state &= ~GHB_STATE_PAUSED;
        }
        else
        {
            status->state &= ~GHB_STATE_SEARCHING;
        }
        if (state->state & HB_STATE_WORKDONE)
        {
            status->state |= GHB_STATE_WORKDONE;
            status->state &= ~GHB_STATE_MUXING;
            status->state &= ~GHB_STATE_PAUSED;
            status->state &= ~GHB_STATE_WORKING;
            status->state &= ~GHB_STATE_SEARCHING;
            switch (p.error)
            {
            case HB_ERROR_NONE:
                status->error = GHB_ERROR_NONE;
                break;
            case HB_ERROR_CANCELED:
                status->error = GHB_ERROR_CANCELED;
                break;
            default:
                status->error = GHB_ERROR_FAIL;
                break;
            }
        }
    }
    if (state->state & HB_STATE_PAUSED)
    {
        status->paused      = p.paused;
        status->state |= GHB_STATE_PAUSED;
    }
    else
    {
        status->state &= ~GHB_STATE_PAUSED;
    }
#undef p
    if (state->state & HB_STATE_MUXING)
    {
        status->state |= GHB_STATE_MUXING;
    }
    else
    {
        status->state &= ~GHB_STATE_MUXING;
    }
}

void
ghb_track_status()
{
    hb_state_t state;

    if (h_scan == NULL) return;
    hb_get_state( h_scan, &state );
    update_status(&state, &hb_status.scan);
    hb_get_state( h_queue, &state );
    update_status(&state, &hb_status.queue);
    hb_get_state( h_live, &state );
    update_status(&state, &hb_status.live);
}

hb_audio_config_t*
ghb_get_audio_info(const hb_title_t *title, gint track)
{
    if (title == NULL) return NULL;
    if (!hb_list_count(title->list_audio))
    {
        return NULL;
    }
    return hb_list_audio_config_item(title->list_audio, track);
}

hb_subtitle_t*
ghb_get_subtitle_info(const hb_title_t *title, gint track)
{
    if (title == NULL) return NULL;
    if (!hb_list_count(title->list_subtitle))
    {
        return NULL;
    }
    return hb_list_item(title->list_subtitle, track);
}

hb_list_t *
ghb_get_title_list()
{
    if (h_scan == NULL) return NULL;
    return hb_get_titles( h_scan );
}

gboolean
ghb_audio_is_passthru(gint acodec)
{
    g_debug("ghb_audio_is_passthru () \n");
    return (acodec & HB_ACODEC_PASS_FLAG) != 0;
}

gboolean
ghb_audio_can_passthru(gint acodec)
{
    g_debug("ghb_audio_can_passthru () \n");
    return (acodec & HB_ACODEC_PASS_MASK) != 0;
}

gint
ghb_get_default_acodec()
{
    return HB_ACODEC_FFAAC;
}

void
ghb_picture_settings_deps(signal_user_data_t *ud)
{
    gboolean autoscale, keep_aspect, enable_keep_aspect;
    gboolean enable_scale_width, enable_scale_height;
    gboolean enable_disp_width, enable_disp_height, enable_par;
    gint pic_par;
    GtkWidget *widget;

    pic_par = ghb_settings_combo_int(ud->settings, "PicturePAR");
    enable_keep_aspect = (pic_par != HB_ANAMORPHIC_STRICT &&
                          pic_par != HB_ANAMORPHIC_AUTO   &&
                          pic_par != HB_ANAMORPHIC_LOOSE);
    keep_aspect = ghb_dict_get_bool(ud->settings, "PictureKeepRatio");
    autoscale = ghb_dict_get_bool(ud->settings, "autoscale");

    enable_scale_width = enable_scale_height =
                         !autoscale && (pic_par != HB_ANAMORPHIC_STRICT);
    enable_disp_width = (pic_par == HB_ANAMORPHIC_CUSTOM) && !keep_aspect;
    enable_par = (pic_par == HB_ANAMORPHIC_CUSTOM) && !keep_aspect;
    enable_disp_height = FALSE;

    widget = GHB_WIDGET(ud->builder, "PictureModulus");
    gtk_widget_set_sensitive(widget, pic_par != HB_ANAMORPHIC_STRICT);
    widget = GHB_WIDGET(ud->builder, "PictureLooseCrop");
    gtk_widget_set_sensitive(widget, pic_par != HB_ANAMORPHIC_STRICT);
    widget = GHB_WIDGET(ud->builder, "scale_width");
    gtk_widget_set_sensitive(widget, enable_scale_width);
    widget = GHB_WIDGET(ud->builder, "scale_height");
    gtk_widget_set_sensitive(widget, enable_scale_height);
    widget = GHB_WIDGET(ud->builder, "PictureDisplayWidth");
    gtk_widget_set_sensitive(widget, enable_disp_width);
    widget = GHB_WIDGET(ud->builder, "PictureDisplayHeight");
    gtk_widget_set_sensitive(widget, enable_disp_height);
    widget = GHB_WIDGET(ud->builder, "PicturePARWidth");
    gtk_widget_set_sensitive(widget, enable_par);
    widget = GHB_WIDGET(ud->builder, "PicturePARHeight");
    gtk_widget_set_sensitive(widget, enable_par);
    widget = GHB_WIDGET(ud->builder, "PictureKeepRatio");
    gtk_widget_set_sensitive(widget, enable_keep_aspect);
    widget = GHB_WIDGET(ud->builder, "autoscale");
    gtk_widget_set_sensitive(widget, pic_par != HB_ANAMORPHIC_STRICT);
}

void
ghb_limit_rational( gint *num, gint *den, gint limit )
{
    if (*num < limit && *den < limit)
        return;

    if (*num > *den)
    {
        gdouble factor = (double)limit / *num;
        *num = limit;
        *den = factor * *den;
    }
    else
    {
        gdouble factor = (double)limit / *den;
        *den = limit;
        *num = factor * *num;
    }
}

void
ghb_apply_crop(GhbValue *settings, const hb_title_t * title)
{
    gboolean autocrop, loosecrop;
    gint crop[4] = {0,};

    autocrop = ghb_dict_get_bool(settings, "PictureAutoCrop");
    // "PictureLooseCrop" is a flag that says we prefer to crop extra to
    // satisfy alignment constraints rather than scaling to satisfy them.
    loosecrop = ghb_dict_get_bool(settings, "PictureLooseCrop");

    if (autocrop)
    {
        crop[0] = title->crop[0];
        crop[1] = title->crop[1];
        crop[2] = title->crop[2];
        crop[3] = title->crop[3];
    }
    else
    {
        crop[0] = ghb_dict_get_int(settings, "PictureTopCrop");
        crop[1] = ghb_dict_get_int(settings, "PictureBottomCrop");
        crop[2] = ghb_dict_get_int(settings, "PictureLeftCrop");
        crop[3] = ghb_dict_get_int(settings, "PictureRightCrop");
    }
    if (loosecrop)
    {
        gint need1, need2;
        gint crop_width, crop_height, width, height;
        gint mod;

        mod = ghb_settings_combo_int(settings, "PictureModulus");
        if (mod <= 0)
            mod = 16;

        // Adjust the cropping to accomplish the desired width and height
        crop_width = title->geometry.width - crop[2] - crop[3];
        crop_height = title->geometry.height - crop[0] - crop[1];
        width = MOD_DOWN(crop_width, mod);
        height = MOD_DOWN(crop_height, mod);

        need1 = EVEN((crop_height - height) / 2);
        need2 = crop_height - height - need1;
        crop[0] += need1;
        crop[1] += need2;
        need1 = EVEN((crop_width - width) / 2);
        need2 = crop_width - width - need1;
        crop[2] += need1;
        crop[3] += need2;
    }
    // Prevent crop from creating too small an image
    if (title->geometry.height - crop[0] -crop[1] < 16)
    {
        crop[0] = title->geometry.height - crop[1] - 16;
        if (crop[0] < 0)
        {
            crop[1] += crop[0];
            crop[0] = 0;
        }
    }
    if (title->geometry.width - crop[2] - crop[3] < 16)
    {
        crop[2] = title->geometry.width - crop[3] - 16;
        if (crop[2] < 0)
        {
            crop[3] += crop[2];
            crop[2] = 0;
        }
    }
    ghb_dict_set_int(settings, "PictureTopCrop", crop[0]);
    ghb_dict_set_int(settings, "PictureBottomCrop", crop[1]);
    ghb_dict_set_int(settings, "PictureLeftCrop", crop[2]);
    ghb_dict_set_int(settings, "PictureRightCrop", crop[3]);
}

void
ghb_set_scale_settings(GhbValue *settings, gint mode)
{
    gboolean keep_aspect;
    gint pic_par;
    gboolean autoscale;
    gint crop[4] = {0,};
    gint width, height;
    gint crop_width, crop_height;
    gboolean keep_width = (mode & GHB_PIC_KEEP_WIDTH);
    gboolean keep_height = (mode & GHB_PIC_KEEP_HEIGHT);
    gint mod;
    gint max_width = 0;
    gint max_height = 0;

    pic_par = ghb_settings_combo_int(settings, "PicturePAR");
    if (pic_par == HB_ANAMORPHIC_STRICT)
    {
        ghb_dict_set_bool(settings, "autoscale", TRUE);
        ghb_dict_set_int(settings, "PictureModulus", 2);
        ghb_dict_set_bool(settings, "PictureLooseCrop", TRUE);
    }
    if (pic_par == HB_ANAMORPHIC_STRICT ||
        pic_par == HB_ANAMORPHIC_AUTO   ||
        pic_par == HB_ANAMORPHIC_LOOSE)
    {
        ghb_dict_set_bool(settings, "PictureKeepRatio", TRUE);
    }

    int title_id, titleindex;
    const hb_title_t * title;

    title_id = ghb_dict_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL) return;

    hb_geometry_t srcGeo, resultGeo;
    hb_geometry_settings_t uiGeo;

    srcGeo.width   = title->geometry.width;
    srcGeo.height  = title->geometry.height;
    srcGeo.par     = title->geometry.par;

    // First configure widgets
    mod = ghb_settings_combo_int(settings, "PictureModulus");
    if (mod <= 0)
        mod = 16;
    keep_aspect = ghb_dict_get_bool(settings, "PictureKeepRatio");
    autoscale = ghb_dict_get_bool(settings, "autoscale");
    // Align dimensions to either 16 or 2 pixels
    // The scaler crashes if the dimensions are not divisible by 2
    // x264 also will not accept dims that are not multiple of 2
    if (autoscale)
    {
        keep_width = FALSE;
        keep_height = FALSE;
    }

    ghb_apply_crop(settings, title);
    crop[0] = ghb_dict_get_int(settings, "PictureTopCrop");
    crop[1] = ghb_dict_get_int(settings, "PictureBottomCrop");
    crop[2] = ghb_dict_get_int(settings, "PictureLeftCrop");
    crop[3] = ghb_dict_get_int(settings, "PictureRightCrop");
    uiGeo.crop[0] = crop[0];
    uiGeo.crop[1] = crop[1];
    uiGeo.crop[2] = crop[2];
    uiGeo.crop[3] = crop[3];

    crop_width = title->geometry.width - crop[2] - crop[3];
    crop_height = title->geometry.height - crop[0] - crop[1];
    if (autoscale)
    {
        width = crop_width;
        height = crop_height;
    }
    else
    {
        width = ghb_dict_get_int(settings, "scale_width");
        height = ghb_dict_get_int(settings, "scale_height");
        if (mode & GHB_PIC_USE_MAX)
        {
            max_width = MOD_DOWN(
                ghb_dict_get_int(settings, "PictureWidth"), mod);
            max_height = MOD_DOWN(
                ghb_dict_get_int(settings, "PictureHeight"), mod);
        }
    }
    g_debug("max_width %d, max_height %d\n", max_width, max_height);

    if (width < 16)
        width = 16;
    if (height < 16)
        height = 16;

    width = MOD_ROUND(width, mod);
    height = MOD_ROUND(height, mod);

    uiGeo.mode = pic_par;
    uiGeo.keep = 0;
    if (keep_width)
        uiGeo.keep |= HB_KEEP_WIDTH;
    if (keep_height)
        uiGeo.keep |= HB_KEEP_HEIGHT;
    if (keep_aspect)
        uiGeo.keep |= HB_KEEP_DISPLAY_ASPECT;
    uiGeo.itu_par = 0;
    uiGeo.modulus = mod;
    uiGeo.geometry.width = width;
    uiGeo.geometry.height = height;
    uiGeo.geometry.par = title->geometry.par;
    uiGeo.maxWidth = max_width;
    uiGeo.maxHeight = max_height;
    if (pic_par != HB_ANAMORPHIC_NONE)
    {
        if (pic_par == HB_ANAMORPHIC_CUSTOM && !keep_aspect)
        {
            if (mode & GHB_PIC_KEEP_PAR)
            {
                uiGeo.geometry.par.num =
                    ghb_dict_get_int(settings, "PicturePARWidth");
                uiGeo.geometry.par.den =
                    ghb_dict_get_int(settings, "PicturePARHeight");
            }
            else if (mode & (GHB_PIC_KEEP_DISPLAY_HEIGHT |
                             GHB_PIC_KEEP_DISPLAY_WIDTH))
            {
                uiGeo.geometry.par.num =
                        ghb_dict_get_int(settings, "PictureDisplayWidth");
                uiGeo.geometry.par.den = width;
                hb_reduce(&uiGeo.geometry.par.num, &uiGeo.geometry.par.den,
                           uiGeo.geometry.par.num,  uiGeo.geometry.par.den);
            }
        }
        else
        {
            uiGeo.keep |= HB_KEEP_DISPLAY_ASPECT;
        }
    }
    // hb_set_anamorphic_size will adjust par, dar, and width/height
    // to conform to job parameters that have been set, including
    // maxWidth and maxHeight
    hb_set_anamorphic_size2(&srcGeo, &uiGeo, &resultGeo);

    ghb_dict_set_int(settings, "scale_width", resultGeo.width);
    ghb_dict_set_int(settings, "scale_height", resultGeo.height);

    gint disp_width;

    disp_width = ((gdouble)resultGeo.par.num / resultGeo.par.den) *
                 resultGeo.width + 0.5;

    ghb_dict_set_int(settings, "PicturePARWidth", resultGeo.par.num);
    ghb_dict_set_int(settings, "PicturePARHeight", resultGeo.par.den);
    ghb_dict_set_int(settings, "PictureDisplayWidth", disp_width);
    ghb_dict_set_int(settings, "PictureDisplayHeight", resultGeo.height);

    // Update Job PAR
    GhbValue *par = ghb_get_job_par_settings(settings);
    ghb_dict_set_int(par, "Num", resultGeo.par.num);
    ghb_dict_set_int(par, "Den", resultGeo.par.den);
}

char *
ghb_get_display_aspect_string(double disp_width, double disp_height)
{
    gchar *str;

    gint iaspect = disp_width * 9 / disp_height;
    if (disp_width > 2 * disp_height)
    {
        str = g_strdup_printf("%.2f:1", disp_width / disp_height);
    }
    else if (iaspect <= 16 && iaspect >= 15)
    {
        str = g_strdup_printf("%.4g:9", disp_width * 9 / disp_height);
    }
    else if (iaspect <= 12 && iaspect >= 11)
    {
        str = g_strdup_printf("%.4g:3", disp_width * 3 / disp_height);
    }
    else
    {
        gint dar_width, dar_height;
        hb_reduce(&dar_width, &dar_height, disp_width, disp_height);
        str = g_strdup_printf("%d:%d", dar_width, dar_height);
    }
    return str;
}

void
ghb_update_display_aspect_label(signal_user_data_t *ud)
{
    gint width, disp_height;
    gint par_num, par_den;
    double disp_width;
    gchar *str;

    width  = ghb_dict_get_int(ud->settings, "scale_width");
    disp_height = ghb_dict_get_int(ud->settings, "scale_height");
    par_num = ghb_dict_get_int(ud->settings, "PicturePARWidth");
    par_den = ghb_dict_get_int(ud->settings, "PicturePARHeight");
    disp_width = (double)width * par_num / par_den;
    str        = ghb_get_display_aspect_string(disp_width, disp_height);
    ghb_ui_update(ud, "display_aspect", ghb_string_value(str));
    g_free(str);
}

void
ghb_set_scale(signal_user_data_t *ud, gint mode)
{
    if (ud->scale_busy) return;
    ud->scale_busy = TRUE;

    ghb_set_scale_settings(ud->settings, mode);
    ghb_update_summary_info(ud);
    ghb_picture_settings_deps(ud);

    // Step needs to be at least 2 because odd widths cause scaler crash
    // subsampled chroma requires even crop values.
    GtkWidget *widget;
    int mod = ghb_settings_combo_int(ud->settings, "PictureModulus");
    widget = GHB_WIDGET (ud->builder, "scale_width");
    gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), mod, 16);
    widget = GHB_WIDGET (ud->builder, "scale_height");
    gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), mod, 16);

    // "PictureLooseCrop" is a flag that says we prefer to crop extra to
    // satisfy alignment constraints rather than scaling to satisfy them.
    gboolean loosecrop = ghb_dict_get_bool(ud->settings, "PictureLooseCrop");
    if (loosecrop)
    {
        widget = GHB_WIDGET (ud->builder, "PictureTopCrop");
        gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), mod, 16);
        widget = GHB_WIDGET (ud->builder, "PictureBottomCrop");
        gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), mod, 16);
        widget = GHB_WIDGET (ud->builder, "PictureLeftCrop");
        gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), mod, 16);
        widget = GHB_WIDGET (ud->builder, "PictureRightCrop");
        gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), mod, 16);
    }
    else
    {
        widget = GHB_WIDGET (ud->builder, "PictureTopCrop");
        gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), 2, 16);
        widget = GHB_WIDGET (ud->builder, "PictureBottomCrop");
        gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), 2, 16);
        widget = GHB_WIDGET (ud->builder, "PictureLeftCrop");
        gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), 2, 16);
        widget = GHB_WIDGET (ud->builder, "PictureRightCrop");
        gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), 2, 16);
    }

    ghb_ui_update_from_settings(ud, "autoscale", ud->settings);
    ghb_ui_update_from_settings(ud, "PictureModulus", ud->settings);
    ghb_ui_update_from_settings(ud, "PictureLooseCrop", ud->settings);
    ghb_ui_update_from_settings(ud, "PictureKeepRatio", ud->settings);

    ghb_ui_update_from_settings(ud, "PictureTopCrop", ud->settings);
    ghb_ui_update_from_settings(ud, "PictureBottomCrop", ud->settings);
    ghb_ui_update_from_settings(ud, "PictureLeftCrop", ud->settings);
    ghb_ui_update_from_settings(ud, "PictureRightCrop", ud->settings);

    ghb_ui_update_from_settings(ud, "scale_width", ud->settings);
    ghb_ui_update_from_settings(ud, "scale_height", ud->settings);

    ghb_ui_update_from_settings(ud, "PicturePARWidth", ud->settings);
    ghb_ui_update_from_settings(ud, "PicturePARHeight", ud->settings);
    ghb_ui_update_from_settings(ud, "PictureDisplayWidth", ud->settings);
    ghb_ui_update_from_settings(ud, "PictureDisplayHeight", ud->settings);
    ghb_update_display_aspect_label(ud);
    ud->scale_busy = FALSE;
}

static void
get_preview_geometry(signal_user_data_t *ud, const hb_title_t *title,
                     hb_geometry_t *srcGeo, hb_geometry_settings_t *uiGeo)
{
    srcGeo->width  = title->geometry.width;
    srcGeo->height = title->geometry.height;
    srcGeo->par    = title->geometry.par;

    uiGeo->mode = ghb_settings_combo_int(ud->settings, "PicturePAR");
    uiGeo->keep = (ghb_dict_get_bool(ud->settings, "PictureKeepRatio") ||
                                 uiGeo->mode == HB_ANAMORPHIC_STRICT  ||
                                 uiGeo->mode == HB_ANAMORPHIC_LOOSE) ?
                                 HB_KEEP_DISPLAY_ASPECT : 0;
    uiGeo->itu_par = 0;
    uiGeo->modulus = ghb_settings_combo_int(ud->settings, "PictureModulus");
    uiGeo->crop[0] = ghb_dict_get_int(ud->settings, "PictureTopCrop");
    uiGeo->crop[1] = ghb_dict_get_int(ud->settings, "PictureBottomCrop");
    uiGeo->crop[2] = ghb_dict_get_int(ud->settings, "PictureLeftCrop");
    uiGeo->crop[3] = ghb_dict_get_int(ud->settings, "PictureRightCrop");
    uiGeo->geometry.width = ghb_dict_get_int(ud->settings, "scale_width");
    uiGeo->geometry.height = ghb_dict_get_int(ud->settings, "scale_height");
    uiGeo->geometry.par.num = ghb_dict_get_int(ud->settings, "PicturePARWidth");
    uiGeo->geometry.par.den = ghb_dict_get_int(ud->settings, "PicturePARHeight");
    uiGeo->maxWidth = 0;
    uiGeo->maxHeight = 0;
    if (ghb_dict_get_bool(ud->prefs, "preview_show_crop"))
    {
        gdouble xscale = (gdouble)uiGeo->geometry.width /
                  (title->geometry.width - uiGeo->crop[2] - uiGeo->crop[3]);
        gdouble yscale = (gdouble)uiGeo->geometry.height /
                  (title->geometry.height - uiGeo->crop[0] - uiGeo->crop[1]);

        uiGeo->geometry.width += xscale * (uiGeo->crop[2] + uiGeo->crop[3]);
        uiGeo->geometry.height += yscale * (uiGeo->crop[0] + uiGeo->crop[1]);
        uiGeo->crop[0] = 0;
        uiGeo->crop[1] = 0;
        uiGeo->crop[2] = 0;
        uiGeo->crop[3] = 0;
        uiGeo->modulus = 2;
    }
}

const char*
ghb_lookup_filter_name(int filter_id, const char *short_name, int preset)
{
    hb_filter_param_t *map;
    int ii;

    if (short_name == NULL)
    {
        return NULL;
    }
    if (preset)
    {
        map = hb_filter_param_get_presets(filter_id);
    }
    else
    {
        map = hb_filter_param_get_tunes(filter_id);
    }
    if (map == NULL)
    {
        return NULL;
    }
    for (ii = 0; map[ii].name != NULL; ii++)
    {
        if (!strcasecmp(map[ii].short_name, short_name))
        {
            return map[ii].name;
        }
    }
    return NULL;
}

void
ghb_set_custom_filter_tooltip(signal_user_data_t *ud,
                              const char *name, const char * desc,
                              int filter_id)
{
    char ** keys = hb_filter_get_keys(filter_id);
    char  * colon = "", * newline;
    char    tooltip[1024];
    int     ii, linelen = 0, pos = 0;

    if (keys == NULL)
    {
        // Filter not set
        return;
    }
    pos += snprintf(tooltip + pos, 1024 - pos,
                    "Custom %s filter string format:\n\n", desc);
    for (ii = 0; keys[ii] != NULL && pos < 1024; ii++)
    {
        int c = tolower(keys[ii][0]);
        int len = strlen(keys[ii]) + 3;
        if (linelen + len > 60)
        {
            newline = "\n";
            linelen = 0;
        }
        else
        {
            newline = "";
        }
        pos += snprintf(tooltip + pos, 1024 - pos, "%s%s%s=%c",
                       colon, newline, keys[ii], c);
        linelen += len;
        colon = ":";
    }
    hb_str_vfree(keys);

    GtkWidget *widget = GHB_WIDGET(ud->builder, name);
    gtk_widget_set_tooltip_text(widget, tooltip);
}

gboolean
ghb_validate_filters(GhbValue *settings, GtkWindow *parent)
{
    gchar *message;

    // Detelecine
    const char *detel_preset;
    detel_preset = ghb_dict_get_string(settings, "PictureDetelecine");
    if (strcasecmp(detel_preset, "off"))
    {
        const char *detel_custom = NULL;
        int filter_id;

        filter_id = HB_FILTER_DETELECINE;
        detel_custom = ghb_dict_get_string(settings, "PictureDetelecineCustom");
        if (hb_validate_filter_preset(filter_id, detel_preset, NULL,
                                      detel_custom))
        {
            if (detel_custom != NULL)
            {
                message = g_strdup_printf(
                            _("Invalid Detelecine Settings:\n\n"
                              "Preset:\t%s\n"
                              "Custom:\t%s\n"), detel_preset, detel_custom);
            }
            else
            {
                message = g_strdup_printf(
                            _("Invalid Detelecine Settings:\n\n"
                              "Preset:\t%s\n"), detel_preset);
            }
            ghb_message_dialog(parent, GTK_MESSAGE_ERROR,
                               message, _("Cancel"), NULL);
            g_free(message);
            return FALSE;
        }
    }

    // Comb Detect
    const char *comb_preset;
    comb_preset = ghb_dict_get_string(settings, "PictureCombDetectPreset");
    if (strcasecmp(comb_preset, "off"))
    {
        const char *comb_custom = NULL;
        int filter_id;

        filter_id = HB_FILTER_COMB_DETECT;
        comb_custom = ghb_dict_get_string(settings, "PictureCombDetectCustom");
        if (hb_validate_filter_preset(filter_id, comb_preset, NULL,
                                      comb_custom))
        {
            if (comb_custom != NULL && comb_custom[0] != 0)
            {
                message = g_strdup_printf(
                            _("Invalid Comb Detect Settings:\n\n"
                              "Preset:\t%s\n"
                              "Custom:\t%s\n"), comb_preset, comb_custom);
            }
            else
            {
                message = g_strdup_printf(
                            _("Invalid Comb Detect Settings:\n\n"
                              "Preset:\t%s\n"), comb_preset);
            }
            ghb_message_dialog(parent, GTK_MESSAGE_ERROR,
                               message, _("Cancel"), NULL);
            g_free(message);
            return FALSE;
        }
    }

    // Deinterlace
    int filter_id;
    filter_id = ghb_settings_combo_int(settings, "PictureDeinterlaceFilter");
    if (filter_id != HB_FILTER_INVALID)
    {
        const char *deint_filter, *deint_preset, *deint_custom = NULL;

        deint_filter = ghb_dict_get_string(settings,
                                           "PictureDeinterlaceFilter");
        deint_preset = ghb_dict_get_string(settings,
                                           "PictureDeinterlacePreset");
        deint_custom = ghb_dict_get_string(settings,
                                           "PictureDeinterlaceCustom");
        if (hb_validate_filter_preset(filter_id, deint_preset, NULL,
                                      deint_custom))
        {
            if (deint_custom != NULL)
            {
                message = g_strdup_printf(
                            _("Invalid Deinterlace Settings:\n\n"
                              "Filter:\t%s\n"
                              "Preset:\t%s\n"
                              "Custom:\t%s\n"), deint_filter, deint_preset,
                                                deint_custom);
            }
            else
            {
                message = g_strdup_printf(
                            _("Invalid Deinterlace Settings:\n\n"
                              "Filter:\t%s\n"
                              "Preset:\t%s\n"), deint_filter, deint_preset);
            }
            ghb_message_dialog(parent, GTK_MESSAGE_ERROR,
                               message, _("Cancel"), NULL);
            g_free(message);
            return FALSE;
        }
    }

    // Denoise
    filter_id = ghb_settings_combo_int(settings, "PictureDenoiseFilter");
    if (filter_id != HB_FILTER_INVALID)
    {
        const char *denoise_filter, *denoise_preset;
        const char *denoise_tune = NULL, *denoise_custom = NULL;

        denoise_filter = ghb_dict_get_string(settings, "PictureDenoiseFilter");
        denoise_preset = ghb_dict_get_string(settings, "PictureDenoisePreset");
        if (filter_id == HB_FILTER_NLMEANS)
        {
            denoise_tune = ghb_dict_get_string(settings, "PictureDenoiseTune");
        }
        denoise_custom = ghb_dict_get_string(settings, "PictureDenoiseCustom");
        if (hb_validate_filter_preset(filter_id, denoise_preset, denoise_tune,
                                      denoise_custom))
        {
            message = g_strdup_printf(
                        _("Invalid Denoise Settings:\n\n"
                          "Filter:\t%s\n"
                          "Preset:\t%s\n"
                          "Tune:\t%s\n"
                          "Custom:\t%s\n"), denoise_filter, denoise_preset,
                                           denoise_tune, denoise_custom);
            ghb_message_dialog(parent, GTK_MESSAGE_ERROR,
                               message, _("Cancel"), NULL);
            g_free(message);
            return FALSE;
        }
    }

    // Sharpen
    filter_id = ghb_settings_combo_int(settings, "PictureSharpenFilter");
    if (filter_id != HB_FILTER_INVALID)
    {
        const char *sharpen_filter, *sharpen_preset;
        const char *sharpen_tune = NULL, *sharpen_custom = NULL;

        sharpen_filter = ghb_dict_get_string(settings, "PictureSharpenFilter");
        sharpen_preset = ghb_dict_get_string(settings, "PictureSharpenPreset");
        sharpen_tune = ghb_dict_get_string(settings, "PictureSharpenTune");
        sharpen_custom = ghb_dict_get_string(settings, "PictureSharpenCustom");
        if (hb_validate_filter_preset(filter_id, sharpen_preset, sharpen_tune,
                                      sharpen_custom))
        {
            message = g_strdup_printf(
                        _("Invalid Sharpen Settings:\n\n"
                          "Filter:\t%s\n"
                          "Preset:\t%s\n"
                          "Tune:\t%s\n"
                          "Custom:\t%s\n"), sharpen_filter, sharpen_preset,
                                           sharpen_tune, sharpen_custom);
            ghb_message_dialog(parent, GTK_MESSAGE_ERROR,
                               message, _("Cancel"), NULL);
            g_free(message);
            return FALSE;
        }
    }

    return TRUE;
}

gboolean
ghb_validate_video(GhbValue *settings, GtkWindow *parent)
{
    gint vcodec;
    gchar *message;
    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    gboolean v_unsup = FALSE;

    vcodec = ghb_settings_video_encoder_codec(settings, "VideoEncoder");
    if ((mux->format & HB_MUX_MASK_MP4) && (vcodec == HB_VCODEC_THEORA))
    {
        // mp4/theora combination is not supported.
        message = g_strdup_printf(
                    _("Theora is not supported in the MP4 container.\n\n"
                    "You should choose a different video codec or container.\n"
                    "If you continue, FFMPEG will be chosen for you."));
        v_unsup = TRUE;
    }
    else if ((mux->format & HB_MUX_MASK_WEBM) &&
             (vcodec != HB_VCODEC_FFMPEG_VP8 && vcodec != HB_VCODEC_FFMPEG_VP9))
    {
        // webm only supports vp8 and vp9.
        message = g_strdup_printf(
                    _("Only VP8 or VP9 is supported in the WebM container.\n\n"
                    "You should choose a different video codec or container.\n"
                    "If you continue, one will be chosen for you."));
        v_unsup = TRUE;
    }

    if (v_unsup)
    {
        if (!ghb_message_dialog(parent, GTK_MESSAGE_WARNING,
                                message, _("Cancel"), _("Continue")))
        {
            g_free(message);
            return FALSE;
        }
        g_free(message);
        vcodec = hb_video_encoder_get_default(mux->format);
        ghb_dict_set_string(settings, "VideoEncoder",
                                hb_video_encoder_get_short_name(vcodec));
    }

    return TRUE;
}

gboolean
ghb_validate_subtitles(GhbValue *settings, GtkWindow *parent)
{
    gint title_id, titleindex;
    const hb_title_t * title;
    gchar *message;

    title_id = ghb_dict_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL)
    {
        /* No valid title, stop right there */
        g_message(_("No title found.\n"));
        return FALSE;
    }

    const GhbValue *slist, *subtitle, *import;
    gint count, ii, track;
    gboolean burned, one_burned = FALSE;

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    slist = ghb_get_job_subtitle_list(settings);
    count = ghb_array_len(slist);
    for (ii = 0; ii < count; ii++)
    {
        subtitle = ghb_array_get(slist, ii);
        track = ghb_dict_get_int(subtitle, "Track");
        import = ghb_dict_get(subtitle, "Import");
        burned = track != -1 && ghb_dict_get_bool(subtitle, "Burn");
        if (burned && one_burned)
        {
            // MP4 can only handle burned vobsubs.  make sure there isn't
            // already something burned in the list
            message = g_strdup_printf(
            _("Only one subtitle may be burned into the video.\n\n"
                "You should change your subtitle selections.\n"
                "If you continue, some subtitles will be lost."));
            if (!ghb_message_dialog(parent, GTK_MESSAGE_WARNING,
                                    message, _("Cancel"), _("Continue")))
            {
                g_free(message);
                return FALSE;
            }
            g_free(message);
            break;
        }
        else if (burned)
        {
            one_burned = TRUE;
        }
        else if (mux->format & HB_MUX_MASK_WEBM)
        {
            // WebM can only handle burned subs afaik. Their specs are ambiguous here
            message = g_strdup_printf(
            _("WebM in HandBrake only supports burned subtitles.\n\n"
                "You should change your subtitle selections.\n"
                "If you continue, some subtitles will be lost."));
            if (!ghb_message_dialog(parent, GTK_MESSAGE_WARNING,
                                    message, _("Cancel"), _("Continue")))
            {
                g_free(message);
                return FALSE;
            }
            g_free(message);
            break;
        }
        if (import != NULL)
        {
            const gchar *filename;

            filename = ghb_dict_get_string(import, "Filename");
            if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR))
            {
                message = g_strdup_printf(
                _("SRT file does not exist or not a regular file.\n\n"
                    "You should choose a valid file.\n"
                    "If you continue, this subtitle will be ignored."));
                if (!ghb_message_dialog(parent, GTK_MESSAGE_WARNING, message,
                    _("Cancel"), _("Continue")))
                {
                    g_free(message);
                    return FALSE;
                }
                g_free(message);
                break;
            }
        }
    }
    return TRUE;
}

gboolean
ghb_validate_audio(GhbValue *settings, GtkWindow *parent)
{
    gint title_id, titleindex;
    const hb_title_t * title;
    gchar *message;

    title_id = ghb_dict_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL)
    {
        /* No valid title, stop right there */
        g_message(_("No title found.\n"));
        return FALSE;
    }

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    const GhbValue *audio_list;
    gint count, ii;

    audio_list = ghb_get_job_audio_list(settings);
    count = ghb_array_len(audio_list);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *asettings;
        hb_audio_config_t *aconfig;
        int track, codec;

        asettings = ghb_array_get(audio_list, ii);
        track = ghb_dict_get_int(asettings, "Track");
        codec = ghb_settings_audio_encoder_codec(asettings, "Encoder");

        aconfig = hb_list_audio_config_item(title->list_audio, track);
        if (ghb_audio_is_passthru(codec) &&
            !(ghb_audio_can_passthru(aconfig->in.codec) &&
              (aconfig->in.codec & codec)))
        {
            // Not supported.  AC3 is passthrough only, so input must be AC3
            message = g_strdup_printf(
                        _("The source does not support Pass-Thru.\n\n"
                        "You should choose a different audio codec.\n"
                        "If you continue, one will be chosen for you."));
            if (!ghb_message_dialog(parent, GTK_MESSAGE_WARNING,
                                    message, _("Cancel"), _("Continue")))
            {
                g_free(message);
                return FALSE;
            }
            g_free(message);
            if ((codec & HB_ACODEC_AC3) ||
                (aconfig->in.codec & HB_ACODEC_MASK) == HB_ACODEC_DCA)
            {
                codec = HB_ACODEC_AC3;
            }
            else if (mux->format & HB_MUX_MASK_MKV)
            {
                codec = HB_ACODEC_LAME;
            }
            else if (mux->format & HB_MUX_MASK_WEBM)
            {
                codec = hb_audio_encoder_get_default(mux->format);
            }
            else
            {
                codec = HB_ACODEC_FFAAC;
            }
            const char *name = hb_audio_encoder_get_short_name(codec);
            ghb_dict_set_string(asettings, "Encoder", name);
        }
        const gchar *a_unsup = NULL;
        const gchar *mux_s = NULL;
        if (mux->format & HB_MUX_MASK_MP4)
        {
            mux_s = "MP4";
            // mp4/vorbis|DTS combination is not supported.
            if (codec == HB_ACODEC_VORBIS)
            {
                a_unsup = "Vorbis";
                codec = HB_ACODEC_FFAAC;
            }
        }
        if (mux->format & HB_MUX_MASK_WEBM)
        {
            mux_s = "WebM";
            // WebM only supports Vorbis and Opus codecs
            if (codec != HB_ACODEC_VORBIS && codec != HB_ACODEC_OPUS)
            {
                a_unsup = hb_audio_encoder_get_short_name(codec);
                codec = hb_audio_encoder_get_default(mux->format);
            }
        }
        if (a_unsup)
        {
            message = g_strdup_printf(
                        _("%s is not supported in the %s container.\n\n"
                        "You should choose a different audio codec.\n"
                        "If you continue, one will be chosen for you."), a_unsup, mux_s);
            if (!ghb_message_dialog(parent, GTK_MESSAGE_WARNING,
                                    message, _("Cancel"), _("Continue")))
            {
                g_free(message);
                return FALSE;
            }
            g_free(message);
            const char *name = hb_audio_encoder_get_short_name(codec);
            ghb_dict_set_string(asettings, "Encoder", name);
        }
    }
    return TRUE;
}

int
ghb_add_job(hb_handle_t *h, GhbValue *job_dict)
{
    char     * json_job;
    int        sequence_id;

    json_job = hb_value_get_json(job_dict);
    sequence_id = hb_add_json(h, json_job);
    free(json_job);

    return sequence_id;
}

void
ghb_remove_job(gint unique_id)
{
    hb_job_t * job;
    gint ii;

    // Multiples passes all get the same id
    // remove them all.
    // Go backwards through list, so reordering doesn't screw me.
    ii = hb_count(h_queue) - 1;
    while ((job = hb_job(h_queue, ii--)) != NULL)
    {
        if (job->sequence_id == unique_id)
            hb_rem(h_queue, job);
    }
}

void
ghb_start_queue()
{
    hb_start( h_queue );
}

void
ghb_stop_queue()
{
    hb_stop( h_queue );
}

void
ghb_start_live_encode()
{
    hb_start( h_live );
}

void
ghb_stop_live_encode()
{
    hb_stop( h_live );
}

void
ghb_pause_queue()
{
    hb_status.queue.state |= GHB_STATE_PAUSED;
    hb_pause( h_queue );
}

void
ghb_resume_queue()
{
    hb_status.queue.state &= ~GHB_STATE_PAUSED;
    hb_resume( h_queue );
}

void
ghb_pause_resume_queue()
{
    hb_state_t s;
    hb_get_state2( h_queue, &s );

    if( s.state == HB_STATE_PAUSED )
    {
        ghb_resume_queue();
    }
    else
    {
        ghb_pause_queue();
    }
}

static void
vert_line(
    GdkPixbuf * pb,
    guint8 r,
    guint8 g,
    guint8 b,
    gint x,
    gint y,
    gint len,
    gint width)
{
    guint8 *pixels = gdk_pixbuf_get_pixels (pb);
    guint8 *dst;
    gint ii, jj;
    gint channels = gdk_pixbuf_get_n_channels (pb);
    gint stride = gdk_pixbuf_get_rowstride (pb);

    for (jj = 0; jj < width; jj++)
    {
        dst = pixels + y * stride + (x+jj) * channels;
        for (ii = 0; ii < len; ii++)
        {
            dst[0] = r;
            dst[1] = g;
            dst[2] = b;
            dst += stride;
        }
    }
}

static void
horz_line(
    GdkPixbuf * pb,
    guint8 r,
    guint8 g,
    guint8 b,
    gint x,
    gint y,
    gint len,
    gint width)
{
    guint8 *pixels = gdk_pixbuf_get_pixels (pb);
    guint8 *dst;
    gint ii, jj;
    gint channels = gdk_pixbuf_get_n_channels (pb);
    gint stride = gdk_pixbuf_get_rowstride (pb);

    for (jj = 0; jj < width; jj++)
    {
        dst = pixels + (y+jj) * stride + x * channels;
        for (ii = 0; ii < len; ii++)
        {
            dst[0] = r;
            dst[1] = g;
            dst[2] = b;
            dst += channels;
        }
    }
}

static void
hash_pixbuf(
    GdkPixbuf * pb,
    gint        x,
    gint        y,
    gint        w,
    gint        h,
    gint        step,
    gint        orientation)
{
    gint ii, jj;
    gint line_width = 8;
    struct
    {
        guint8 r;
        guint8 g;
        guint8 b;
    } c[4] =
    {{0x80, 0x80, 0x80},{0xC0, 0x80, 0x70},{0x80, 0xA0, 0x80},{0x70, 0x80, 0xA0}};

    if (!orientation)
    {
        // vertical lines
        for (ii = x, jj = 0; ii+line_width < x+w; ii += step, jj++)
        {
            vert_line(pb, c[jj&3].r, c[jj&3].g, c[jj&3].b, ii, y, h, line_width);
        }
    }
    else
    {
        // horizontal lines
        for (ii = y, jj = 0; ii+line_width < y+h; ii += step, jj++)
        {
            horz_line(pb, c[jj&3].r, c[jj&3].g, c[jj&3].b, x, ii, w, line_width);
        }
    }
}

GdkPixbuf*
ghb_get_preview_image(
    const hb_title_t *title,
    gint index,
    signal_user_data_t *ud,
    gint *out_width,
    gint *out_height)
{
    hb_geometry_t srcGeo, resultGeo;
    hb_geometry_settings_t uiGeo;

    if( title == NULL ) return NULL;

    gboolean deinterlace;
    deinterlace = ghb_settings_combo_int(ud->settings,
                            "PictureDeinterlaceFilter") != HB_FILTER_INVALID;

    // Get the geometry settings for the preview.  This will disable
    // cropping if the setting to show the cropped region is enabled.
    get_preview_geometry(ud, title, &srcGeo, &uiGeo);

    // hb_get_preview doesn't compensate for anamorphic, so lets
    // calculate scale factors
    hb_set_anamorphic_size2(&srcGeo, &uiGeo, &resultGeo);

    // Rescale preview dimensions to adjust for screen PAR and settings PAR
    ghb_par_scale(ud, &uiGeo.geometry.width, &uiGeo.geometry.height,
                      resultGeo.par.num, resultGeo.par.den);
    uiGeo.geometry.par.num = 1;
    uiGeo.geometry.par.den = 1;

    GdkPixbuf *preview;
    hb_image_t *image;
    image = hb_get_preview2(h_scan, title->index, index, &uiGeo, deinterlace);

    if (image == NULL)
    {
        preview = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
                                 title->geometry.width, title->geometry.height);
        return preview;
    }

    // Create an GdkPixbuf and copy the libhb image into it, converting it from
    // libhb's format something suitable.
    // The image data returned by hb_get_preview is 4 bytes per pixel,
    // BGRA format. Alpha is ignored.
    preview = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
                             image->width, image->height);
    guint8 *pixels = gdk_pixbuf_get_pixels(preview);

    guint8 *src_line = image->data;
    guint8 *dst = pixels;

    gint ii, jj;
    gint channels = gdk_pixbuf_get_n_channels(preview);
    gint stride = gdk_pixbuf_get_rowstride(preview);
    guint8 *tmp;

    for (ii = 0; ii < image->height; ii++)
    {
        guint32 *src = (guint32*)src_line;
        tmp = dst;
        for (jj = 0; jj < image->width; jj++)
        {
            tmp[0] = src[0] >> 16;
            tmp[1] = src[0] >> 8;
            tmp[2] = src[0] >> 0;
            tmp += channels;
            src++;
        }
        src_line += image->plane[0].stride;
        dst += stride;
    }

    *out_width = ghb_dict_get_int(ud->settings, "scale_width");
    *out_height = ghb_dict_get_int(ud->settings, "scale_height");
    ghb_par_scale(ud, out_width, out_height,
                  resultGeo.par.num, resultGeo.par.den);

    gint c0, c1, c2, c3;
    c0 = ghb_dict_get_int(ud->settings, "PictureTopCrop");
    c1 = ghb_dict_get_int(ud->settings, "PictureBottomCrop");
    c2 = ghb_dict_get_int(ud->settings, "PictureLeftCrop");
    c3 = ghb_dict_get_int(ud->settings, "PictureRightCrop");

    gdouble xscale, yscale;
    if (ghb_dict_get_bool(ud->prefs, "preview_show_crop"))
    {
        xscale = (gdouble)image->width / title->geometry.width;
        yscale = (gdouble)image->height / title->geometry.height;
    }
    else
    {
        xscale = (gdouble)image->width / (title->geometry.width - c2 - c3);
        yscale = (gdouble)image->height / (title->geometry.height - c0 - c1);
    }

    int previewWidth = image->width;
    int previewHeight = image->height;

    // If the preview is too large to fit the screen, reduce it's size.
    if (ghb_dict_get_bool(ud->prefs, "reduce_hd_preview"))
    {
        gint factor = 80;
        gint s_w, s_h;

        ghb_monitor_get_size(GHB_WIDGET(ud->builder, "hb_window"), &s_w, &s_h);
        if (s_w > 0 && s_h > 0 &&
            (previewWidth  > s_w * factor / 100 ||
             previewHeight > s_h * factor / 100))
        {
            GdkPixbuf *scaled_preview;
            int orig_w = previewWidth;
            int orig_h = previewHeight;

            if (previewWidth > s_w * factor / 100)
            {
                previewWidth = s_w * factor / 100;
                previewHeight = previewHeight * previewWidth / orig_w;
            }
            if (previewHeight > s_h * factor / 100)
            {
                previewHeight = s_h * factor / 100;
                previewWidth = orig_w * previewHeight / orig_h;
            }
            xscale *= (gdouble)previewWidth / orig_w;
            yscale *= (gdouble)previewHeight / orig_h;
            scaled_preview = gdk_pixbuf_scale_simple(preview,
                            previewWidth, previewHeight, GDK_INTERP_HYPER);
            g_object_unref(preview);
            preview = scaled_preview;
        }
    }

    if (ghb_dict_get_bool(ud->prefs, "preview_show_crop"))
    {
        c0 *= yscale;
        c1 *= yscale;
        c2 *= xscale;
        c3 *= xscale;

        // Top
        hash_pixbuf(preview, 0, 0, previewWidth, c0, 32, 0);
        // Bottom
        hash_pixbuf(preview, 0, previewHeight-c1, previewWidth, c1, 32, 0);
        // Left
        hash_pixbuf(preview, 0, 0, c2, previewHeight, 32, 1);
        // Right
        hash_pixbuf(preview, previewWidth-c3, 0, c3, previewHeight, 32, 1);
    }
    hb_image_close(&image);
    return preview;
}

gchar*
ghb_dvd_volname(const gchar *device)
{
    gchar *name;
    name = hb_dvd_name((gchar*)device);
    if (name != NULL && name[0] != 0)
    {
        name = g_strdup(name);
        ghb_dvd_sanitize_volname(name);
        return name;
    }
    return NULL;
}
