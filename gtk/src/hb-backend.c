/***************************************************************************
 *            hb-backend.c
 *
 *  Fri Mar 28 10:38:44 2008
 *  Copyright  2008-2015  John Stebbins
 *  <john at stebbins dot name>
 ****************************************************************************/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
#define _GNU_SOURCE
#include <limits.h>
#include <math.h>
#include "hb.h"
#include "ghbcompat.h"
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include "hb-backend.h"
#include "settings.h"
#include "callbacks.h"
#include "subtitlehandler.h"
#include "audiohandler.h"
#include "videohandler.h"
#include "x264handler.h"
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
    const gchar *svalue;
} options_map_t;

typedef struct
{
    gint count;
    options_map_t *map;
} combo_opts_t;

static options_map_t d_subtitle_track_sel_opts[] =
{
    {N_("None"),                                    "none",       0, "0"},
    {N_("First Track Matching Selected Languages"), "first",      1, "1"},
    {N_("All Tracks Matching Selected Languages"),  "all",        2, "2"},
};
combo_opts_t subtitle_track_sel_opts =
{
    sizeof(d_subtitle_track_sel_opts)/sizeof(options_map_t),
    d_subtitle_track_sel_opts
};

static options_map_t d_subtitle_burn_opts[] =
{
    {N_("None"),                                     "none",          0, "0"},
    {N_("Foreign Audio Subtitle Track"),             "foreign",       1, "1"},
    {N_("First Selected Track"),                     "first",         2, "2"},
    {N_("Foreign Audio, then First Selected Track"), "foreign_first", 3, "3"},
};
combo_opts_t subtitle_burn_opts =
{
    sizeof(d_subtitle_burn_opts)/sizeof(options_map_t),
    d_subtitle_burn_opts
};

static options_map_t d_audio_track_sel_opts[] =
{
    {N_("None"),                                    "none",       0, "0"},
    {N_("First Track Matching Selected Languages"), "first",      1, "1"},
    {N_("All Tracks Matching Selected Languages"),  "all",        2, "2"},
};
combo_opts_t audio_track_sel_opts =
{
    sizeof(d_audio_track_sel_opts)/sizeof(options_map_t),
    d_audio_track_sel_opts
};

static options_map_t d_point_to_point_opts[] =
{
    {N_("Chapters:"), "chapter", 0, "0"},
    {N_("Seconds:"),  "time",    1, "1"},
    {N_("Frames:"),   "frame",   2, "2"},
};
combo_opts_t point_to_point_opts =
{
    sizeof(d_point_to_point_opts)/sizeof(options_map_t),
    d_point_to_point_opts
};

static options_map_t d_when_complete_opts[] =
{
    {N_("Do Nothing"),            "nothing",  0, "0"},
    {N_("Show Notification"),     "notify",   1, "1"},
    {N_("Quit Handbrake"),        "quit",     4, "4"},
    {N_("Put Computer To Sleep"), "sleep",    2, "2"},
    {N_("Shutdown Computer"),     "shutdown", 3, "3"},
};
combo_opts_t when_complete_opts =
{
    sizeof(d_when_complete_opts)/sizeof(options_map_t),
    d_when_complete_opts
};

static options_map_t d_par_opts[] =
{
    {N_("Off"),    "off",    0, "0"},
    {N_("Strict"), "strict", 1, "1"},
    {N_("Loose"),  "loose",  2, "2"},
    {N_("Custom"), "custom", 3, "3"},
};
combo_opts_t par_opts =
{
    sizeof(d_par_opts)/sizeof(options_map_t),
    d_par_opts
};

static options_map_t d_alignment_opts[] =
{
    {"2", "2", 2, "2"},
    {"4", "4", 4, "4"},
    {"8", "8", 8, "8"},
    {"16", "16", 16, "16"},
};
combo_opts_t alignment_opts =
{
    sizeof(d_alignment_opts)/sizeof(options_map_t),
    d_alignment_opts
};

static options_map_t d_logging_opts[] =
{
    {"0", "0", 0, "0"},
    {"1", "1", 1, "1"},
    {"2", "2", 2, "2"},
    {"3", "3", 3, "3"},
};
combo_opts_t logging_opts =
{
    sizeof(d_logging_opts)/sizeof(options_map_t),
    d_logging_opts
};

static options_map_t d_log_longevity_opts[] =
{
    {N_("Week"),     "week",     7, "7"},
    {N_("Month"),    "month",    30, "30"},
    {N_("Year"),     "year",     365, "365"},
    {N_("Immortal"), "immortal", 366, "366"},
};
combo_opts_t log_longevity_opts =
{
    sizeof(d_log_longevity_opts)/sizeof(options_map_t),
    d_log_longevity_opts
};

static options_map_t d_appcast_update_opts[] =
{
    {N_("Never"),   "never", 0, "never"},
    {N_("Daily"),   "daily", 1, "daily"},
    {N_("Weekly"),  "weekly", 2, "weekly"},
    {N_("Monthly"), "monthly", 3, "monthly"},
};
combo_opts_t appcast_update_opts =
{
    sizeof(d_appcast_update_opts)/sizeof(options_map_t),
    d_appcast_update_opts
};

static options_map_t d_vqual_granularity_opts[] =
{
    {"0.2",  "0.2",  0.2,  "0.2"},
    {"0.25", "0.25", 0.25, "0.25"},
    {"0.5",  "0.5",  0.5,  "0.5"},
    {"1",    "1",    1,    "1"},
};
combo_opts_t vqual_granularity_opts =
{
    sizeof(d_vqual_granularity_opts)/sizeof(options_map_t),
    d_vqual_granularity_opts
};

static options_map_t d_deint_opts[] =
{
    {N_("Off"),         "off",         HB_FILTER_INVALID,     ""},
    {N_("Decomb"),      "decomb",      HB_FILTER_DECOMB,      ""},
    {N_("Deinterlace"), "deinterlace", HB_FILTER_DEINTERLACE, ""},
};
combo_opts_t deint_opts =
{
    sizeof(d_deint_opts)/sizeof(options_map_t),
    d_deint_opts
};

static options_map_t d_denoise_opts[] =
{
    {N_("Off"),     "off",     HB_FILTER_INVALID, ""},
    {N_("NLMeans"), "nlmeans", HB_FILTER_NLMEANS, ""},
    {N_("HQDN3D"),  "hqdn3d",  HB_FILTER_HQDN3D,  ""},
};
combo_opts_t denoise_opts =
{
    sizeof(d_denoise_opts)/sizeof(options_map_t),
    d_denoise_opts
};

static options_map_t d_direct_opts[] =
{
    {N_("None"),      "none",     0, "none"},
    {N_("Spatial"),   "spatial",  1, "spatial"},
    {N_("Temporal"),  "temporal", 2, "temporal"},
    {N_("Automatic"), "auto",     3, "auto"},
};
combo_opts_t direct_opts =
{
    sizeof(d_direct_opts)/sizeof(options_map_t),
    d_direct_opts
};

static options_map_t d_badapt_opts[] =
{
    {N_("Off"),             "0", 0, "0"},
    {N_("Fast"),            "1", 1, "1"},
    {N_("Optimal"),         "2", 2, "2"},
};
combo_opts_t badapt_opts =
{
    sizeof(d_badapt_opts)/sizeof(options_map_t),
    d_badapt_opts
};

static options_map_t d_bpyramid_opts[] =
{
    {N_("Off"),    "none",   0, "none"},
    {N_("Strict"), "strict", 1, "strict"},
    {N_("Normal"), "normal", 2, "normal"},
};
combo_opts_t bpyramid_opts =
{
    sizeof(d_bpyramid_opts)/sizeof(options_map_t),
    d_bpyramid_opts
};

static options_map_t d_weightp_opts[] =
{
    {N_("Off"),    "0", 0, "0"},
    {N_("Simple"), "1", 1, "1"},
    {N_("Smart"),  "2", 2, "2"},
};
combo_opts_t weightp_opts =
{
    sizeof(d_weightp_opts)/sizeof(options_map_t),
    d_weightp_opts
};

static options_map_t d_me_opts[] =
{
    {N_("Diamond"),              "dia",  0, "dia"},
    {N_("Hexagon"),              "hex",  1, "hex"},
    {N_("Uneven Multi-Hexagon"), "umh",  2, "umh"},
    {N_("Exhaustive"),           "esa",  3, "esa"},
    {N_("Hadamard Exhaustive"),  "tesa", 4, "tesa"},
};
combo_opts_t me_opts =
{
    sizeof(d_me_opts)/sizeof(options_map_t),
    d_me_opts
};

static options_map_t d_subme_opts[] =
{
    {N_("0: SAD, no subpel"),          "0", 0, "0"},
    {N_("1: SAD, qpel"),               "1", 1, "1"},
    {N_("2: SATD, qpel"),              "2", 2, "2"},
    {N_("3: SATD: multi-qpel"),        "3", 3, "3"},
    {N_("4: SATD, qpel on all"),       "4", 4, "4"},
    {N_("5: SATD, multi-qpel on all"), "5", 5, "5"},
    {N_("6: RD in I/P-frames"),        "6", 6, "6"},
    {N_("7: RD in all frames"),        "7", 7, "7"},
    {N_("8: RD refine in I/P-frames"), "8", 8, "8"},
    {N_("9: RD refine in all frames"), "9", 9, "9"},
    {N_("10: QPRD in all frames"),     "10", 10, "10"},
    {N_("11: No early terminations in analysis"), "11", 11, "11"},
};
combo_opts_t subme_opts =
{
    sizeof(d_subme_opts)/sizeof(options_map_t),
    d_subme_opts
};

static options_map_t d_analyse_opts[] =
{
    {N_("Most"), "p8x8,b8x8,i8x8,i4x4", 0, "p8x8,b8x8,i8x8,i4x4"},
    {N_("None"), "none", 1, "none"},
    {N_("Some"), "i4x4,i8x8", 2, "i4x4,i8x8"},
    {N_("All"),  "all",  3, "all"},
    {N_("Custom"),  "custom",  4, "all"},
};
combo_opts_t analyse_opts =
{
    sizeof(d_analyse_opts)/sizeof(options_map_t),
    d_analyse_opts
};

static options_map_t d_trellis_opts[] =
{
    {N_("Off"),         "0", 0, "0"},
    {N_("Encode only"), "1", 1, "1"},
    {N_("Always"),      "2", 2, "2"},
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

#if 0
static filter_opts_t hqdn3d_preset_opts =
{
    .filter_id = HB_FILTER_HQDN3D,
    .preset    = TRUE
};
#endif

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
static void filter_opts_set(signal_user_data_t *ud, const gchar *name,
                           void *opts, const void* data);
static void deint_opts_set(signal_user_data_t *ud, const gchar *name,
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
        "PictureDenoiseFilter",
        &denoise_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "PictureDenoisePreset",
        &nlmeans_preset_opts,
        filter_opts_set,
        filter_opt_get
    },
    {
        "PictureDenoiseTune",
        &nlmeans_tune_opts,
        filter_opts_set,
        filter_opt_get
    },
    {
        "x264_direct",
        &direct_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "x264_b_adapt",
        &badapt_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "x264_bpyramid",
        &bpyramid_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "x264_weighted_pframes",
        &weightp_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "x264_me",
        &me_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "x264_subme",
        &subme_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "x264_analyse",
        &analyse_opts,
        small_opts_set,
        generic_opt_get
    },
    {
        "x264_trellis",
        &trellis_opts,
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
        "SrtLanguage",
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

#if 0
typedef struct iso639_lang_t
{
    char * eng_name;        /* Description in English */
    char * native_name;     /* Description in native language */
    char * iso639_1;       /* ISO-639-1 (2 characters) code */
    char * iso639_2;        /* ISO-639-2/t (3 character) code */
    char * iso639_2b;       /* ISO-639-2/b code (if different from above) */
} iso639_lang_t;
#endif

const iso639_lang_t ghb_language_table[] =
{
    { "Any", "", "zz", "und" },
    { "Afar", "", "aa", "aar" },
    { "Abkhazian", "", "ab", "abk" },
    { "Afrikaans", "", "af", "afr" },
    { "Akan", "", "ak", "aka" },
    { "Albanian", "", "sq", "sqi", "alb" },
    { "Amharic", "", "am", "amh" },
    { "Arabic", "", "ar", "ara" },
    { "Aragonese", "", "an", "arg" },
    { "Armenian", "", "hy", "hye", "arm" },
    { "Assamese", "", "as", "asm" },
    { "Avaric", "", "av", "ava" },
    { "Avestan", "", "ae", "ave" },
    { "Aymara", "", "ay", "aym" },
    { "Azerbaijani", "", "az", "aze" },
    { "Bashkir", "", "ba", "bak" },
    { "Bambara", "", "bm", "bam" },
    { "Basque", "", "eu", "eus", "baq" },
    { "Belarusian", "", "be", "bel" },
    { "Bengali", "", "bn", "ben" },
    { "Bihari", "", "bh", "bih" },
    { "Bislama", "", "bi", "bis" },
    { "Bosnian", "", "bs", "bos" },
    { "Breton", "", "br", "bre" },
    { "Bulgarian", "", "bg", "bul" },
    { "Burmese", "", "my", "mya", "bur" },
    { "Catalan", "", "ca", "cat" },
    { "Chamorro", "", "ch", "cha" },
    { "Chechen", "", "ce", "che" },
    { "Chinese", "", "zh", "zho", "chi" },
    { "Church Slavic", "", "cu", "chu" },
    { "Chuvash", "", "cv", "chv" },
    { "Cornish", "", "kw", "cor" },
    { "Corsican", "", "co", "cos" },
    { "Cree", "", "cr", "cre" },
    { "Czech", "", "cs", "ces", "cze" },
    { "Danish", "Dansk", "da", "dan" },
    { "German", "Deutsch", "de", "deu", "ger" },
    { "Divehi", "", "dv", "div" },
    { "Dzongkha", "", "dz", "dzo" },
    { "English", "English", "en", "eng" },
    { "Spanish", "Espanol", "es", "spa" },
    { "Esperanto", "", "eo", "epo" },
    { "Estonian", "", "et", "est" },
    { "Ewe", "", "ee", "ewe" },
    { "Faroese", "", "fo", "fao" },
    { "Fijian", "", "fj", "fij" },
    { "French", "Francais", "fr", "fra", "fre" },
    { "Western Frisian", "", "fy", "fry" },
    { "Fulah", "", "ff", "ful" },
    { "Georgian", "", "ka", "kat", "geo" },
    { "Gaelic (Scots)", "", "gd", "gla" },
    { "Irish", "", "ga", "gle" },
    { "Galician", "", "gl", "glg" },
    { "Manx", "", "gv", "glv" },
    { "Greek, Modern", "", "el", "ell", "gre" },
    { "Guarani", "", "gn", "grn" },
    { "Gujarati", "", "gu", "guj" },
    { "Haitian", "", "ht", "hat" },
    { "Hausa", "", "ha", "hau" },
    { "Hebrew", "", "he", "heb" },
    { "Herero", "", "hz", "her" },
    { "Hindi", "", "hi", "hin" },
    { "Hiri Motu", "", "ho", "hmo" },
    { "Croatian", "Hrvatski", "hr", "hrv", "scr" },
    { "Igbo", "", "ig", "ibo" },
    { "Ido", "", "io", "ido" },
    { "Icelandic", "Islenska", "is", "isl", "ice" },
    { "Sichuan Yi", "", "ii", "iii" },
    { "Inuktitut", "", "iu", "iku" },
    { "Interlingue", "", "ie", "ile" },
    { "Interlingua", "", "ia", "ina" },
    { "Indonesian", "", "id", "ind" },
    { "Inupiaq", "", "ik", "ipk" },
    { "Italian", "Italiano", "it", "ita" },
    { "Javanese", "", "jv", "jav" },
    { "Japanese", "", "ja", "jpn" },
    { "Kalaallisut", "", "kl", "kal" },
    { "Kannada", "", "kn", "kan" },
    { "Kashmiri", "", "ks", "kas" },
    { "Kanuri", "", "kr", "kau" },
    { "Kazakh", "", "kk", "kaz" },
    { "Central Khmer", "", "km", "khm" },
    { "Kikuyu", "", "ki", "kik" },
    { "Kinyarwanda", "", "rw", "kin" },
    { "Kirghiz", "", "ky", "kir" },
    { "Komi", "", "kv", "kom" },
    { "Kongo", "", "kg", "kon" },
    { "Korean", "", "ko", "kor" },
    { "Kuanyama", "", "kj", "kua" },
    { "Kurdish", "", "ku", "kur" },
    { "Lao", "", "lo", "lao" },
    { "Latin", "", "la", "lat" },
    { "Latvian", "", "lv", "lav" },
    { "Limburgan", "", "li", "lim" },
    { "Lingala", "", "ln", "lin" },
    { "Lithuanian", "", "lt", "lit" },
    { "Luxembourgish", "", "lb", "ltz" },
    { "Luba-Katanga", "", "lu", "lub" },
    { "Ganda", "", "lg", "lug" },
    { "Macedonian", "", "mk", "mkd", "mac" },
    { "Hungarian", "Magyar", "hu", "hun" },
    { "Marshallese", "", "mh", "mah" },
    { "Malayalam", "", "ml", "mal" },
    { "Maori", "", "mi", "mri", "mao" },
    { "Marathi", "", "mr", "mar" },
    { "Malay", "", "ms", "msa", "msa" },
    { "Malagasy", "", "mg", "mlg" },
    { "Maltese", "", "mt", "mlt" },
    { "Moldavian", "", "mo", "mol" },
    { "Mongolian", "", "mn", "mon" },
    { "Nauru", "", "na", "nau" },
    { "Navajo", "", "nv", "nav" },
    { "Dutch", "Nederlands", "nl", "nld", "dut" },
    { "Ndebele, South", "", "nr", "nbl" },
    { "Ndebele, North", "", "nd", "nde" },
    { "Ndonga", "", "ng", "ndo" },
    { "Nepali", "", "ne", "nep" },
    { "Norwegian", "Norsk", "no", "nor" },
    { "Norwegian Nynorsk", "", "nn", "nno" },
    { "Norwegian Bokmål", "", "nb", "nob" },
    { "Chichewa; Nyanja", "", "ny", "nya" },
    { "Occitan", "", "oc", "oci" },
    { "Ojibwa", "", "oj", "oji" },
    { "Oriya", "", "or", "ori" },
    { "Oromo", "", "om", "orm" },
    { "Ossetian", "", "os", "oss" },
    { "Panjabi", "", "pa", "pan" },
    { "Persian", "", "fa", "fas", "per" },
    { "Pali", "", "pi", "pli" },
    { "Polish", "", "pl", "pol" },
    { "Portuguese", "Portugues", "pt", "por" },
    { "Pushto", "", "ps", "pus" },
    { "Quechua", "", "qu", "que" },
    { "Romansh", "", "rm", "roh" },
    { "Romanian", "", "ro", "ron", "rum" },
    { "Rundi", "", "rn", "run" },
    { "Russian", "", "ru", "rus" },
    { "Sango", "", "sg", "sag" },
    { "Sanskrit", "", "sa", "san" },
    { "Serbian", "", "sr", "srp", "scc" },
    { "Sinhala", "", "si", "sin" },
    { "Slovak", "", "sk", "slk", "slo" },
    { "Slovenian", "", "sl", "slv" },
    { "Northern Sami", "", "se", "sme" },
    { "Samoan", "", "sm", "smo" },
    { "Shona", "", "sn", "sna" },
    { "Sindhi", "", "sd", "snd" },
    { "Somali", "", "so", "som" },
    { "Sotho, Southern", "", "st", "sot" },
    { "Sardinian", "", "sc", "srd" },
    { "Swati", "", "ss", "ssw" },
    { "Sundanese", "", "su", "sun" },
    { "Finnish", "Suomi", "fi", "fin" },
    { "Swahili", "", "sw", "swa" },
    { "Swedish", "Svenska", "sv", "swe" },
    { "Tahitian", "", "ty", "tah" },
    { "Tamil", "", "ta", "tam" },
    { "Tatar", "", "tt", "tat" },
    { "Telugu", "", "te", "tel" },
    { "Tajik", "", "tg", "tgk" },
    { "Tagalog", "", "tl", "tgl" },
    { "Thai", "", "th", "tha" },
    { "Tibetan", "", "bo", "bod", "tib" },
    { "Tigrinya", "", "ti", "tir" },
    { "Tonga", "", "to", "ton" },
    { "Tswana", "", "tn", "tsn" },
    { "Tsonga", "", "ts", "tso" },
    { "Turkmen", "", "tk", "tuk" },
    { "Turkish", "", "tr", "tur" },
    { "Twi", "", "tw", "twi" },
    { "Uighur", "", "ug", "uig" },
    { "Ukrainian", "", "uk", "ukr" },
    { "Urdu", "", "ur", "urd" },
    { "Uzbek", "", "uz", "uzb" },
    { "Venda", "", "ve", "ven" },
    { "Vietnamese", "", "vi", "vie" },
    { "Volapük", "", "vo", "vol" },
    { "Welsh", "", "cy", "cym", "wel" },
    { "Walloon", "", "wa", "wln" },
    { "Wolof", "", "wo", "wol" },
    { "Xhosa", "", "xh", "xho" },
    { "Yiddish", "", "yi", "yid" },
    { "Yoruba", "", "yo", "yor" },
    { "Zhuang", "", "za", "zha" },
    { "Zulu", "", "zu", "zul" },
    {NULL, NULL, NULL, NULL}
};
#define LANG_TABLE_SIZE (sizeof(ghb_language_table)/ sizeof(iso639_lang_t)-1)

int
ghb_lookup_lang(const GhbValue *glang)
{
    gint ii;
    const gchar *str;

    str = ghb_value_get_string(glang);
    for (ii = 0; ii < LANG_TABLE_SIZE; ii++)
    {
        if (ghb_language_table[ii].iso639_2 &&
            strcmp(ghb_language_table[ii].iso639_2, str) == 0)
        {
            return ii;
        }
        if (ghb_language_table[ii].iso639_2b &&
            strcmp(ghb_language_table[ii].iso639_2b, str) == 0)
        {
            return ii;
        }
        if (ghb_language_table[ii].iso639_1 &&
            strcmp(ghb_language_table[ii].iso639_1, str) == 0)
        {
            return ii;
        }
        if (ghb_language_table[ii].native_name &&
            strcmp(ghb_language_table[ii].native_name, str) == 0)
        {
            return ii;
        }
        if (ghb_language_table[ii].eng_name &&
            strcmp(ghb_language_table[ii].eng_name, str) == 0)
        {
            return ii;
        }
    }
    return -1;
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
    gint result;
    const hb_rate_t *rate;

    result = 0;
    for (rate = hb_audio_samplerate_get_next(NULL); rate != NULL;
         rate = hb_audio_samplerate_get_next(rate))
    {
        if (irate <= rate->rate)
        {
            result = rate->rate;
            break;
        }
    }
    return result;
}

const iso639_lang_t* ghb_iso639_lookup_by_int(int idx)
{
    return &ghb_language_table[idx];
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

extern void hb_get_temporary_directory(char path[512]);

gchar*
ghb_get_tmp_dir()
{
    char dir[512];

    hb_get_temporary_directory(dir);
    return g_strdup(dir);
}

void
ghb_hb_cleanup(gboolean partial)
{
    char dir[512];

    hb_get_temporary_directory(dir);
    del_tree(dir, !partial);
}

gint
ghb_subtitle_track_source(GhbValue *settings, gint track)
{
    gint title_id, titleindex;
    const hb_title_t *title;

    if (track == -2)
        return SRTSUB;
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

    grey_builder_combo_box_item(ud->builder, "x264_analyse", 4, TRUE);

    const hb_encoder_t *enc;
    for (enc = hb_audio_encoder_get_next(NULL); enc != NULL;
         enc = hb_audio_encoder_get_next(enc))
    {
        if (!(mux->format & enc->muxers))
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
    fallback = ghb_select_fallback(ud->settings, acodec);
    gint copy_mask = ghb_get_copy_mask(ud->settings);
    acodec = ghb_select_audio_codec(mux->format, aconfig, acodec,
                                    fallback, copy_mask);
    grey_mix_opts(ud, acodec, layout);
}

gint
ghb_get_best_mix(hb_audio_config_t *aconfig, gint acodec, gint mix)
{
    gint layout;
    layout = aconfig ? aconfig->in.channel_layout : AV_CH_LAYOUT_5POINT1;

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
    // 5 - String value determined by backend
    store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_BOOLEAN,
                               G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_STRING);
    gtk_combo_box_set_model(combo, GTK_TREE_MODEL(store));

    if (!gtk_combo_box_get_has_entry(combo))
    {
        gtk_cell_layout_clear(GTK_CELL_LAYOUT(combo));
        cell = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
        g_object_set(cell, "max-width-chars", 60, NULL);
        g_object_set(cell, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cell, TRUE);
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
                       4, "auto",
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
                           4, rate->name,
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
                       4, "auto",
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
                           4, rate->name,
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
                           4, enc->short_name,
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
        if ((mask & enc->codec) && !(neg_mask & enc->codec))
        {
            gtk_list_store_append(store, &iter);
            str = g_strdup_printf("<small>%s</small>", enc->name);
            gtk_list_store_set(store, &iter,
                               0, str,
                               1, TRUE,
                               2, enc->short_name,
                               3, (gdouble)enc->codec,
                               4, enc->short_name,
                               -1);
            g_free(str);
        }
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
    ghb_audio_encoder_opts_set_with_mask(combo, ~0, 0);
}

static void
audio_encoder_opts_set(signal_user_data_t *ud, const gchar *name,
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
    for (enc = hb_audio_encoder_get_next(NULL); enc != NULL;
         enc = hb_audio_encoder_get_next(enc))
    {
        if (enc->codec != HB_ACODEC_AUTO_PASS)
        {
            gtk_list_store_append(store, &iter);
            str = g_strdup_printf("<small>%s</small>", enc->name);
            gtk_list_store_set(store, &iter,
                               0, str,
                               1, TRUE,
                               2, enc->short_name,
                               3, (gdouble)enc->codec,
                               4, enc->short_name,
                               -1);
            g_free(str);
        }
    }
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
                           4, mix->short_name,
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
                           4, mux->short_name,
                           -1);
        g_free(str);
    }
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
                           4, srt_codeset_table[ii],
                           -1);
    }
}

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
    for (ii = 0; ii < LANG_TABLE_SIZE; ii++)
    {
        const gchar *lang;

        if (ghb_language_table[ii].native_name[0] != 0)
            lang = ghb_language_table[ii].native_name;
        else
            lang = ghb_language_table[ii].eng_name;

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, lang,
                           1, TRUE,
                           2, ghb_language_table[ii].iso639_2,
                           3, (gdouble)ii,
                           4, ghb_language_table[ii].iso639_1,
                           -1);
    }
}

gchar*
ghb_create_title_label(const hb_title_t *title)
{
    gchar *label;

    if (title->type == HB_STREAM_TYPE || title->type == HB_FF_STREAM_TYPE)
    {
        if (title->duration != 0)
        {
            char *tmp;
            tmp  = g_strdup_printf (_("%d - %02dh%02dm%02ds - %s"),
                title->index, title->hours, title->minutes, title->seconds,
                title->name);
            label = g_markup_escape_text(tmp, -1);
            g_free(tmp);
        }
        else
        {
            char *tmp;
            tmp  = g_strdup_printf ("%d - %s",
                                    title->index, title->name);
            label = g_markup_escape_text(tmp, -1);
            g_free(tmp);
        }
    }
    else if (title->type == HB_BD_TYPE)
    {
        if (title->duration != 0)
        {
            label = g_strdup_printf(_("%d (%05d.MPLS) - %02dh%02dm%02ds"),
                title->index, title->playlist, title->hours,
                title->minutes, title->seconds);
        }
        else
        {
            label = g_strdup_printf(_("%d (%05d.MPLS) - Unknown Length"),
                title->index, title->playlist);
        }
    }
    else
    {
        if (title->duration != 0)
        {
            label  = g_strdup_printf(_("%d - %02dh%02dm%02ds"),
                title->index, title->hours, title->minutes, title->seconds);
        }
        else
        {
            label  = g_strdup_printf(_("%d - Unknown Length"),
                                    title->index);
        }
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
                           4, "none",
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
                           4, title_index,
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
                       4, "none",
                       -1);

    for (ii = 0; ii < count; ii++)
    {
        if (strcmp(tunes[ii], "fastdecode") && strcmp(tunes[ii], "zerolatency"))
        {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               0, tunes[ii],
                               1, TRUE,
                               2, tunes[ii],
                               3, (gdouble)ii + 1,
                               4, tunes[ii],
                               -1);
        }
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
                           4, profiles[ii],
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
                           4, levels[ii],
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
                           4, "none",
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
                           4, idx,
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
        opt = g_strdup_printf("%d - %s (%s)", ii+1, subtitle->lang,
                              hb_subsource_name(subtitle->source));
        snprintf(idx, 4, "%d", ii);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                    0, opt,
                    1, TRUE,
                    2, idx,
                    3, (gdouble)ii,
                    4, idx,
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
                           4, "none",
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
                           4, opts->map[ii].svalue,
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
                           4, param[ii].settings != NULL ?
                                param[ii].settings : "",
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

void ghb_init_lang_list_box(GtkListBox *list_box)
{
    int ii;

    for (ii = 0; ii < LANG_TABLE_SIZE; ii++)
    {
        const char *lang;
        if (ghb_language_table[ii].native_name != NULL &&
            ghb_language_table[ii].native_name[0] != 0)
        {
            lang = ghb_language_table[ii].native_name;
        }
        else
        {
            lang = ghb_language_table[ii].eng_name;
        }
        GtkWidget *label = gtk_label_new(lang);
        g_object_set_data(G_OBJECT(label), "lang_idx", (gpointer)(intptr_t)ii);
        gtk_widget_show(label);
        gtk_list_box_insert(list_box, label, -1);
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

// Construct the advanced options string
// The result is allocated, so someone must free it at some point.
const gchar*
ghb_build_advanced_opts_string(GhbValue *settings)
{
    gint vcodec;
    vcodec = ghb_settings_video_encoder_codec(settings, "VideoEncoder");
    switch (vcodec)
    {
        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X264_10BIT:
            return ghb_dict_get_string(settings, "x264Option");

        default:
            return NULL;
    }
}

void
ghb_part_duration(const hb_title_t *title, gint sc, gint ec, gint *hh, gint *mm, gint *ss)
{
    hb_chapter_t * chapter;
    gint count, c;
    gint64 duration;

    *hh = *mm = *ss = 0;
    if (title == NULL) return;

    *hh = title->hours;
    *mm = title->minutes;
    *ss = title->seconds;

    count = hb_list_count(title->list_chapter);
    if (sc > count) sc = count;
    if (ec > count) ec = count;

    if (sc == 1 && ec == count)
        return;

    duration = 0;
    for (c = sc; c <= ec; c++)
    {
        chapter = hb_list_item(title->list_chapter, c-1);
        duration += chapter->duration;
    }

    *hh =   duration / 90000 / 3600;
    *mm = ((duration / 90000) % 3600) / 60;
    *ss =  (duration / 90000) % 60;
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
                           4, rate->name,
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
}

void
ghb_backend_init(gint debug)
{
    /* Init libhb */
    h_scan = hb_init( debug, 0 );
    h_queue = hb_init( debug, 0 );
    h_live = hb_init( debug, 0 );
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
    hb_close(&h_live);
    hb_close(&h_queue);
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
        if (status->state & GHB_STATE_SCANNING)
        {
            status->state |= GHB_STATE_SCANDONE;
        }
        status->state &= ~GHB_STATE_SCANNING;
    }
#undef p
#define p state->param.working
    if (state->state & HB_STATE_WORKING)
    {
        if (status->state & GHB_STATE_SCANNING)
        {
            status->state &= ~GHB_STATE_SCANNING;
            status->state |= GHB_STATE_SCANDONE;
        }
        status->state |= GHB_STATE_WORKING;
        status->state &= ~GHB_STATE_PAUSED;
        status->state &= ~GHB_STATE_SEARCHING;
        status->pass = p.pass;
        status->pass_count = p.pass_count;
        status->pass_id = p.pass_id;
        status->progress = p.progress;
        status->rate_cur = p.rate_cur;
        status->rate_avg = p.rate_avg;
        status->hours = p.hours;
        status->minutes = p.minutes;
        status->seconds = p.seconds;
        status->unique_id = p.sequence_id;
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
        status->pass = p.pass;
        status->pass_count = p.pass_count;
        status->pass_id = p.pass_id;
        status->progress = p.progress;
        status->rate_cur = p.rate_cur;
        status->rate_avg = p.rate_avg;
        status->hours = p.hours;
        status->minutes = p.minutes;
        status->seconds = p.seconds;
        status->unique_id = p.sequence_id;
    }
    else
    {
        status->state &= ~GHB_STATE_SEARCHING;
    }
#undef p
#define p state->param.workdone
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
#undef p
    if (state->state & HB_STATE_PAUSED)
    {
        status->state |= GHB_STATE_PAUSED;
    }
    else
    {
        status->state &= ~GHB_STATE_PAUSED;
    }
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
ghb_set_scale_settings(GhbValue *settings, gint mode)
{
    gboolean keep_aspect;
    gint pic_par;
    gboolean autocrop, autoscale, loosecrop;
    gint crop[4] = {0,};
    gint width, height;
    gint crop_width, crop_height;
    gboolean keep_width = (mode & GHB_PIC_KEEP_WIDTH);
    gboolean keep_height = (mode & GHB_PIC_KEEP_HEIGHT);
    gint mod;
    gint max_width = 0;
    gint max_height = 0;

    g_debug("ghb_set_scale ()\n");

    pic_par = ghb_settings_combo_int(settings, "PicturePAR");
    if (pic_par == HB_ANAMORPHIC_STRICT)
    {
        ghb_dict_set_bool(settings, "autoscale", TRUE);
        ghb_dict_set_int(settings, "PictureModulus", 2);
        ghb_dict_set_bool(settings, "PictureLooseCrop", TRUE);
    }
    if (pic_par == HB_ANAMORPHIC_STRICT || pic_par == HB_ANAMORPHIC_LOOSE)
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
    autocrop = ghb_dict_get_bool(settings, "PictureAutoCrop");
    autoscale = ghb_dict_get_bool(settings, "autoscale");
    // "PictureLooseCrop" is a flag that says we prefer to crop extra to
    // satisfy alignment constraints rather than scaling to satisfy them.
    loosecrop = ghb_dict_get_bool(settings, "PictureLooseCrop");
    // Align dimensions to either 16 or 2 pixels
    // The scaler crashes if the dimensions are not divisible by 2
    // x264 also will not accept dims that are not multiple of 2
    if (autoscale)
    {
        keep_width = FALSE;
        keep_height = FALSE;
    }

    if (autocrop)
    {
        crop[0] = title->crop[0];
        crop[1] = title->crop[1];
        crop[2] = title->crop[2];
        crop[3] = title->crop[3];
        ghb_dict_set_int(settings, "PictureTopCrop", crop[0]);
        ghb_dict_set_int(settings, "PictureBottomCrop", crop[1]);
        ghb_dict_set_int(settings, "PictureLeftCrop", crop[2]);
        ghb_dict_set_int(settings, "PictureRightCrop", crop[3]);
    }
    else
    {
        crop[0] = ghb_dict_get_int(settings, "PictureTopCrop");
        crop[1] = ghb_dict_get_int(settings, "PictureBottomCrop");
        crop[2] = ghb_dict_get_int(settings, "PictureLeftCrop");
        crop[3] = ghb_dict_get_int(settings, "PictureRightCrop");
        // Prevent manual crop from creating too small an image
        if (title->geometry.height - crop[0] < crop[1] + 16)
        {
            crop[0] = title->geometry.height - crop[1] - 16;
        }
        if (title->geometry.width - crop[2] < crop[3] + 16)
        {
            crop[2] = title->geometry.width - crop[3] - 16;
        }
    }
    if (loosecrop)
    {
        gint need1, need2;

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
        ghb_dict_set_int(settings, "PictureTopCrop", crop[0]);
        ghb_dict_set_int(settings, "PictureBottomCrop", crop[1]);
        ghb_dict_set_int(settings, "PictureLeftCrop", crop[2]);
        ghb_dict_set_int(settings, "PictureRightCrop", crop[3]);
    }
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

void
ghb_update_display_aspect_label(signal_user_data_t *ud)
{
    gint disp_width, disp_height, dar_width, dar_height;
    gchar *str;

    disp_width = ghb_dict_get_int(ud->settings, "PictureDisplayWidth");
    disp_height = ghb_dict_get_int(ud->settings, "PictureDisplayHeight");
    hb_reduce(&dar_width, &dar_height, disp_width, disp_height);
    gint iaspect = dar_width * 9 / dar_height;
    if (dar_width > 2 * dar_height)
    {
        str = g_strdup_printf("%.2f : 1", (gdouble)dar_width / dar_height);
    }
    else if (iaspect <= 16 && iaspect >= 15)
    {
        str = g_strdup_printf("%.2f : 9", (gdouble)dar_width * 9 / dar_height);
    }
    else if (iaspect <= 12 && iaspect >= 11)
    {
        str = g_strdup_printf("%.2f : 3", (gdouble)dar_width * 3 / dar_height);
    }
    else
    {
        str = g_strdup_printf("%d : %d", dar_width, dar_height);
    }
    ghb_ui_update(ud, "display_aspect", ghb_string_value(str));
    g_free(str);
}

void
ghb_set_scale(signal_user_data_t *ud, gint mode)
{
    if (ud->scale_busy) return;
    ud->scale_busy = TRUE;

    ghb_set_scale_settings(ud->settings, mode);
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

gboolean
ghb_validate_filters(GhbValue *settings, GtkWindow *parent)
{
    gchar *message;

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
        if (!strcasecmp(deint_preset, "custom"))
        {
            deint_custom = ghb_dict_get_string(settings,
                                               "PictureDeinterlaceCustom");
        }
        if (hb_validate_filter_preset(filter_id, deint_preset, deint_custom))
        {
            if (deint_custom != NULL)
            {
                message = g_strdup_printf(
                            _("Invalid Deinterlace Settings:\n\n"
                              "Filter: %s\n"
                              "Preset: %s\n"
                              "Custom: %s\n"), deint_filter, deint_preset,
                                               deint_custom);
            }
            else
            {
                message = g_strdup_printf(
                            _("Invalid Deinterlace Settings:\n\n"
                              "Filter: %s\n"
                              "Preset: %s\n"), deint_filter, deint_preset);
            }
            ghb_message_dialog(parent, GTK_MESSAGE_ERROR,
                               message, _("Cancel"), NULL);
            g_free(message);
            return FALSE;
        }
    }

    // Detelecine
    const char *detel_preset;
    detel_preset = ghb_dict_get_string(settings, "PictureDetelecine");
    if (strcasecmp(detel_preset, "off"))
    {
        const char *detel_custom = NULL;
        int filter_id;

        filter_id = HB_FILTER_DETELECINE;
        if (!strcasecmp(detel_preset, "custom"))
        {
            detel_custom = ghb_dict_get_string(settings,
                                               "PictureDetelecineCustom");
        }
        if (hb_validate_filter_preset(filter_id, detel_preset, detel_custom))
        {
            if (detel_custom != NULL)
            {
                message = g_strdup_printf(
                            _("Invalid Detelecine Settings:\n\n"
                              "Preset: %s\n"
                              "Custom: %s\n"), detel_preset, detel_custom);
            }
            else
            {
                message = g_strdup_printf(
                            _("Invalid Detelecine Settings:\n\n"
                              "Preset: %s\n"), detel_preset);
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
        if (!strcasecmp(denoise_preset, "custom"))
        {
            denoise_custom = ghb_dict_get_string(settings,
                                               "PictureDenoiseCustom");
        }
        if (hb_validate_filter_preset(filter_id, denoise_preset,
                denoise_custom != NULL ? denoise_custom : denoise_tune))
        {
            if (denoise_custom != NULL)
            {
                message = g_strdup_printf(
                            _("Invalid Denoise Settings:\n\n"
                              "Filter: %s\n"
                              "Preset: %s\n"
                              "Custom: %s\n"), denoise_filter, denoise_preset,
                                               denoise_custom);
            }
            else
            {
                message = g_strdup_printf(
                            _("Invalid Denoise Settings:\n\n"
                              "Filter: %s\n"
                              "Preset: %s\n"
                              "Tune:   %s\n"), denoise_filter, denoise_preset,
                                               denoise_tune);
            }
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

    vcodec = ghb_settings_video_encoder_codec(settings, "VideoEncoder");
    if ((mux->format & HB_MUX_MASK_MP4) && (vcodec == HB_VCODEC_THEORA))
    {
        // mp4/theora combination is not supported.
        message = g_strdup_printf(
                    _("Theora is not supported in the MP4 container.\n\n"
                    "You should choose a different video codec or container.\n"
                    "If you continue, FFMPEG will be chosen for you."));
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

    const GhbValue *slist, *subtitle, *srt;
    gint count, ii, track;
    gboolean burned, one_burned = FALSE;

    slist = ghb_get_job_subtitle_list(settings);
    count = ghb_array_len(slist);
    for (ii = 0; ii < count; ii++)
    {
        subtitle = ghb_array_get(slist, ii);
        track = ghb_dict_get_int(subtitle, "Track");
        srt = ghb_dict_get(subtitle, "SRT");
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
        if (srt != NULL)
        {
            const gchar *filename;

            filename = ghb_dict_get_string(srt, "Filename");
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
        if (codec == HB_ACODEC_AUTO_PASS)
            continue;

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
            else
            {
                codec = HB_ACODEC_FFAAC;
            }
            const char *name = hb_audio_encoder_get_short_name(codec);
            ghb_dict_set_string(asettings, "Encoder", name);
        }
        gchar *a_unsup = NULL;
        gchar *mux_s = NULL;
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

        const hb_mixdown_t *mix;
        mix = ghb_settings_mixdown(asettings, "Mixdown");

        const gchar *mix_unsup = NULL;
        if (!hb_mixdown_is_supported(mix->amixdown, codec, aconfig->in.channel_layout))
        {
            mix_unsup = mix->name;
        }
        if (mix_unsup)
        {
            message = g_strdup_printf(
                        _("The source audio does not support %s mixdown.\n\n"
                        "You should choose a different mixdown.\n"
                        "If you continue, one will be chosen for you."), mix_unsup);
            if (!ghb_message_dialog(parent, GTK_MESSAGE_WARNING,
                                    message, _("Cancel"), _("Continue")))
            {
                g_free(message);
                return FALSE;
            }
            g_free(message);
            int amixdown = ghb_get_best_mix(aconfig, codec, mix->amixdown);
            ghb_dict_set_string(asettings, "Mixdown",
                                    hb_mixdown_get_short_name(amixdown));
        }
    }
    return TRUE;
}

int
ghb_add_job(hb_handle_t *h, GhbValue *js)
{
    GhbValue *job;
    char     *json_job;
    int       sequence_id;

    job      = ghb_dict_get(js, "Job");
    json_job = hb_value_get_json(job);
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
    hb_state_t s;
    hb_get_state2( h_queue, &s );

    if( s.state == HB_STATE_PAUSED )
    {
        hb_status.queue.state &= ~GHB_STATE_PAUSED;
        hb_resume( h_queue );
    }
    else
    {
        hb_status.queue.state |= GHB_STATE_PAUSED;
        hb_pause( h_queue );
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

        if (ghb_dict_get_bool(ud->prefs, "preview_fullscreen"))
        {
            factor = 100;
        }

        GdkScreen *ss;
        gint s_w, s_h;

        ss = gdk_screen_get_default();
        s_w = gdk_screen_get_width(ss);
        s_h = gdk_screen_get_height(ss);

        if (previewWidth > s_w * factor / 100 ||
            previewHeight > s_h * factor / 100)
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

static void
sanitize_volname(gchar *name)
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
ghb_dvd_volname(const gchar *device)
{
    gchar *name;
    name = hb_dvd_name((gchar*)device);
    if (name != NULL && name[0] != 0)
    {
        name = g_strdup(name);
        sanitize_volname(name);
        return name;
    }
    return NULL;
}
