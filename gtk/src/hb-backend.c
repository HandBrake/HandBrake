/***************************************************************************
 *            hb-backend.c
 *
 *  Fri Mar 28 10:38:44 2008
 *  Copyright  2008-2013  John Stebbins
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
#include "x264handler.h"
#include "preview.h"
#include "values.h"
#include "lang.h"

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
    {N_("Off"),    "0", 0, "0"},
    {N_("Strict"), "1", 1, "1"},
    {N_("Loose"),  "2", 2, "2"},
    {N_("Custom"), "3", 3, "3"},
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

static options_map_t d_detel_opts[] =
{
    {N_("Off"),    "off",   0, ""},
    {N_("Custom"), "custom", 1, ""},
    {N_("Default"),"default",2, NULL},
};
combo_opts_t detel_opts =
{
    sizeof(d_detel_opts)/sizeof(options_map_t),
    d_detel_opts
};

static options_map_t d_decomb_opts[] =
{
    {N_("Off"),    "off",   0, ""},
    {N_("Custom"), "custom", 1, ""},
    {N_("Default"),"default",2, NULL},
    {N_("Fast"),   "fast",   3, "7:2:6:9:1:80"},
    {N_("Bob"),    "bob",    4, "455"},
};
combo_opts_t decomb_opts =
{
    sizeof(d_decomb_opts)/sizeof(options_map_t),
    d_decomb_opts
};

static options_map_t d_deint_opts[] =
{
    {N_("Off"),    "off",    0,  ""},
    {N_("Custom"), "custom", 1,  ""},
    {N_("Fast"),   "fast",   2,  "0:-1:-1:0:1"},
    {N_("Slow"),   "slow",   3,  "1:-1:-1:0:1"},
    {N_("Slower"), "slower", 4,  "3:-1:-1:0:1"},
    {N_("Bob"),    "bob",    5, "15:-1:-1:0:1"},
};
combo_opts_t deint_opts =
{
    sizeof(d_deint_opts)/sizeof(options_map_t),
    d_deint_opts
};

static options_map_t d_denoise_opts[] =
{
    {N_("Off"),    "off",    0, ""},
    {N_("Custom"), "custom", 1, ""},
    {N_("Weak"),   "weak",   2, "2:1:1:2:3:3"},
    {N_("Medium"), "medium", 3, "3:2:2:2:3:3"},
    {N_("Strong"), "strong", 4, "7:7:7:5:5:5"},
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
    const gchar *name;
    combo_opts_t *opts;
} combo_name_map_t;

combo_name_map_t combo_name_map[] =
{
    {"SubtitleTrackSelectionBehavior", &subtitle_track_sel_opts},
    {"AudioTrackSelectionBehavior", &audio_track_sel_opts},
    {"PtoPType", &point_to_point_opts},
    {"WhenComplete", &when_complete_opts},
    {"PicturePAR", &par_opts},
    {"PictureModulus", &alignment_opts},
    {"LoggingLevel", &logging_opts},
    {"LogLongevity", &log_longevity_opts},
    {"check_updates", &appcast_update_opts},
    {"VideoQualityGranularity", &vqual_granularity_opts},
    {"PictureDeinterlace", &deint_opts},
    {"PictureDecomb", &decomb_opts},
    {"PictureDetelecine", &detel_opts},
    {"PictureDenoise", &denoise_opts},
    {"x264_direct", &direct_opts},
    {"x264_b_adapt", &badapt_opts},
    {"x264_bpyramid", &bpyramid_opts},
    {"x264_weighted_pframes", &weightp_opts},
    {"x264_me", &me_opts},
    {"x264_subme", &subme_opts},
    {"x264_analyse", &analyse_opts},
    {"x264_trellis", &trellis_opts},
    {NULL, NULL}
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

static void audio_bitrate_opts_set(GtkBuilder *builder, const gchar *name);

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
    case HB_VCODEC_X264:
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
find_combo_entry(combo_opts_t *opts, const GValue *gval)
{
    gint ii;

    if (opts == NULL)
        return 0;

    if (G_VALUE_TYPE(gval) == G_TYPE_STRING)
    {
        gchar *str;
        str = ghb_value_string(gval);
        for (ii = 0; ii < opts->count; ii++)
        {
            if (strcmp(opts->map[ii].shortOpt, str) == 0)
            {
                break;
            }
        }
        g_free(str);
        return ii;
    }
    else if (G_VALUE_TYPE(gval) == G_TYPE_DOUBLE)
    {
        gdouble val;
        val = ghb_value_double(gval);
        for (ii = 0; ii < opts->count; ii++)
        {
            if (opts->map[ii].ivalue == val)
            {
                break;
            }
        }
        return ii;
    }
    else if (G_VALUE_TYPE(gval) == G_TYPE_INT ||
             G_VALUE_TYPE(gval) == G_TYPE_BOOLEAN ||
             G_VALUE_TYPE(gval) == G_TYPE_INT64)
    {
        gint64 val;
        val = ghb_value_int64(gval);
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

static const gchar*
lookup_generic_string(combo_opts_t *opts, const GValue *gval)
{
    gint ii;
    const gchar *result = "";

    ii = find_combo_entry(opts, gval);
    if (ii < opts->count)
    {
        result = opts->map[ii].svalue;
    }
    return result;
}

static gint
lookup_generic_int(combo_opts_t *opts, const GValue *gval)
{
    gint ii;
    gint result = -1;

    if (opts == NULL)
        return 0;

    ii = find_combo_entry(opts, gval);
    if (ii < opts->count)
    {
        result = opts->map[ii].ivalue;
    }
    return result;
}

static gdouble
lookup_generic_double(combo_opts_t *opts, const GValue *gval)
{
    gint ii;
    gdouble result = -1;

    ii = find_combo_entry(opts, gval);
    if (ii < opts->count)
    {
        result = opts->map[ii].ivalue;
    }
    return result;
}

static const gchar*
lookup_generic_option(combo_opts_t *opts, const GValue *gval)
{
    gint ii;
    const gchar *result = "";

    ii = find_combo_entry(opts, gval);
    if (ii < opts->count)
    {
        result = opts->map[ii].option;
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

int
ghb_lookup_audio_lang(const GValue *glang)
{
    gint ii;
    gchar *str;

    str = ghb_value_string(glang);
    for (ii = 0; ii < LANG_TABLE_SIZE; ii++)
    {
        if (ghb_language_table[ii].iso639_2 &&
            strcmp(ghb_language_table[ii].iso639_2, str) == 0)
        {
            g_free(str);
            return ii;
        }
        if (ghb_language_table[ii].iso639_2b &&
            strcmp(ghb_language_table[ii].iso639_2b, str) == 0)
        {
            g_free(str);
            return ii;
        }
        if (ghb_language_table[ii].iso639_1 &&
            strcmp(ghb_language_table[ii].iso639_1, str) == 0)
        {
            g_free(str);
            return ii;
        }
        if (ghb_language_table[ii].native_name &&
            strcmp(ghb_language_table[ii].native_name, str) == 0)
        {
            g_free(str);
            return ii;
        }
        if (ghb_language_table[ii].eng_name &&
            strcmp(ghb_language_table[ii].eng_name, str) == 0)
        {
            g_free(str);
            return ii;
        }
    }
    g_free(str);
    return -1;
}

static int
lookup_audio_lang_int(const GValue *glang)
{
    gint ii = ghb_lookup_audio_lang(glang);
    if (ii >= 0)
        return ii;
    return 0;
}

static const gchar*
lookup_audio_lang_option(const GValue *glang)
{
    gint ii = ghb_lookup_audio_lang(glang);
    if (ii >= 0)
    {
        if (ghb_language_table[ii].native_name[0] != 0)
            return ghb_language_table[ii].native_name;
        else
            return ghb_language_table[ii].eng_name;
    }
    return "Any";
}

// Handle for libhb.  Gets set by ghb_backend_init()
static hb_handle_t * h_scan = NULL;
static hb_handle_t * h_queue = NULL;

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
ghb_subtitle_track_source(GValue *settings, gint track)
{
    gint title_id, titleindex;
    const hb_title_t *title;

    if (track == -2)
        return SRTSUB;
    if (track < 0)
        return VOBSUB;
    title_id = ghb_settings_get_int(settings, "title");
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
ghb_subtitle_track_lang(GValue *settings, gint track)
{
    gint title_id, titleindex;
    const hb_title_t * title;

    title_id = ghb_settings_get_int(settings, "title");
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

static gint
search_audio_bitrates(gint rate)
{
    const hb_rate_t *hbrate;
    for (hbrate = hb_audio_bitrate_get_next(NULL); hbrate != NULL;
         hbrate = hb_audio_bitrate_get_next(hbrate))
    {
        if (hbrate->rate == rate)
            return 1;
    }
    return 0;
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

void
ghb_grey_combo_options(signal_user_data_t *ud)
{
    gint track, title_id, titleindex, acodec, fallback;
    const hb_title_t *title;
    hb_audio_config_t *aconfig = NULL;

    title_id = ghb_settings_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);

    track = ghb_settings_get_int(ud->settings, "AudioTrack");
    aconfig = ghb_get_audio_info(title, track);

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_settings_get_const_string(ud->settings, "FileFormat");
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

    if (aconfig && (aconfig->in.codec & HB_ACODEC_MASK) != HB_ACODEC_MP3)
    {
        grey_builder_combo_box_item(ud->builder, "AudioEncoder", HB_ACODEC_MP3_PASS, TRUE);
    }
    if (aconfig && (aconfig->in.codec & HB_ACODEC_MASK) != HB_ACODEC_FFAAC)
    {
        grey_builder_combo_box_item(ud->builder, "AudioEncoder", HB_ACODEC_AAC_PASS, TRUE);
    }
    if (aconfig && (aconfig->in.codec & HB_ACODEC_MASK) != HB_ACODEC_AC3)
    {
        grey_builder_combo_box_item(ud->builder, "AudioEncoder", HB_ACODEC_AC3_PASS, TRUE);
    }
    if (aconfig && (aconfig->in.codec & HB_ACODEC_MASK) != HB_ACODEC_DCA)
    {
        grey_builder_combo_box_item(ud->builder, "AudioEncoder", HB_ACODEC_DCA_PASS, TRUE);
    }
    if (aconfig && (aconfig->in.codec & HB_ACODEC_MASK) != HB_ACODEC_DCA_HD)
    {
        grey_builder_combo_box_item(ud->builder, "AudioEncoder", HB_ACODEC_DCA_HD_PASS, TRUE);
    }

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

    g_debug("audio_samplerate_opts_set ()\n");
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);
    // Add an item for "Same As Source"
    gtk_list_store_append(store, &iter);
    str = g_strdup_printf("<small>%s</small>", _("Same as source"));
    gtk_list_store_set(store, &iter,
                       0, str,
                       1, TRUE,
                       2, "source",
                       3, 0.0,
                       4, "source",
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
audio_samplerate_opts_set(GtkBuilder *builder, const gchar *name)
{
    g_debug("audio_samplerate_opts_set ()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
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
        name = "source";
    return name;
}

const hb_rate_t*
ghb_lookup_audio_samplerate(const char *name)
{
    // Check for special "Same as source" value
    if (!strncmp(name, "source", 8))
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
ghb_settings_audio_samplerate_rate(const GValue *settings, const char *name)
{
    const char *rate_id = ghb_settings_get_const_string(settings, name);
    return ghb_lookup_audio_samplerate_rate(rate_id);
}

const hb_rate_t*
ghb_settings_audio_samplerate(const GValue *settings, const char *name)
{
    const char *rate_id = ghb_settings_get_const_string(settings, name);
    return ghb_lookup_audio_samplerate(rate_id);
}

static void
video_framerate_opts_set(GtkBuilder *builder, const gchar *name)
{
    GtkTreeIter iter;
    GtkListStore *store;

    g_debug("video_framerate_opts_set ()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);
    // Add an item for "Same As Source"
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       0, _("Same as source"),
                       1, TRUE,
                       2, "source",
                       3, 0.0,
                       4, "source",
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
    if (!strncmp(name, "source", 8))
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
ghb_settings_video_framerate_rate(const GValue *settings, const char *name)
{
    const char *rate_id = ghb_settings_get_const_string(settings, name);
    return ghb_lookup_video_framerate_rate(rate_id);
}

const hb_rate_t*
ghb_settings_video_framerate(const GValue *settings, const char *name)
{
    const char *rate_id = ghb_settings_get_const_string(settings, name);
    return ghb_lookup_video_framerate(rate_id);
}

static void
video_encoder_opts_set(
    GtkBuilder *builder,
    const gchar *name)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *str;

    g_debug("video_encoder_opts_set ()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
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
ghb_settings_video_encoder_codec(const GValue *settings, const char *name)
{
    const char *encoder_id = ghb_settings_get_const_string(settings, name);
    return ghb_lookup_video_encoder_codec(encoder_id);
}

const hb_encoder_t*
ghb_settings_video_encoder(const GValue *settings, const char *name)
{
    const char *encoder_id = ghb_settings_get_const_string(settings, name);
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

    g_debug("ghb_audio_encoder_opts_set_with_mask()\n");
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
ghb_settings_audio_encoder_codec(const GValue *settings, const char *name)
{
    const char *encoder_id = ghb_settings_get_const_string(settings, name);
    return ghb_lookup_audio_encoder_codec(encoder_id);
}

const hb_encoder_t*
ghb_settings_audio_encoder(const GValue *settings, const char *name)
{
    const char *encoder_id = ghb_settings_get_const_string(settings, name);
    return ghb_lookup_audio_encoder(encoder_id);
}

static void
audio_encoder_opts_set_with_mask(
    GtkBuilder *builder,
    const gchar *name,
    int mask,
    int neg_mask)
{
    g_debug("audio_encoder_opts_set_with_mask()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
    ghb_audio_encoder_opts_set_with_mask(combo, mask, neg_mask);
}

void
ghb_audio_encoder_opts_set(GtkComboBox *combo)
{
    ghb_audio_encoder_opts_set_with_mask(combo, ~0, 0);
}

static void
audio_encoder_opts_set(
    GtkBuilder *builder,
    const gchar *name)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *str;

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
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
acodec_fallback_opts_set(GtkBuilder *builder, const gchar *name)
{
    audio_encoder_opts_set_with_mask(builder, name, ~0, HB_ACODEC_PASS_FLAG);
}

void
ghb_mix_opts_set(GtkComboBox *combo)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *str;

    g_debug("mix_opts_set ()\n");
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
ghb_settings_mixdown_mix(const GValue *settings, const char *name)
{
    const char *mixdown_id = ghb_settings_get_const_string(settings, name);
    return ghb_lookup_mixdown_mix(mixdown_id);
}

const hb_mixdown_t*
ghb_settings_mixdown(const GValue *settings, const char *name)
{
    const char *mixdown_id = ghb_settings_get_const_string(settings, name);
    return ghb_lookup_mixdown(mixdown_id);
}

static void
mix_opts_set(GtkBuilder *builder, const gchar *name)
{
    g_debug("mix_opts_set ()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
    ghb_mix_opts_set(combo);
}

static void
container_opts_set(
    GtkBuilder *builder,
    const gchar *name)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *str;

    g_debug("hb_container_opts_set ()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
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
srt_codeset_opts_set(GtkBuilder *builder, const gchar *name)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii;

    g_debug("srt_codeset_opts_set ()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
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
language_opts_set(GtkBuilder *builder, const gchar *name)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii;

    g_debug("language_opts_set ()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
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

const iso639_lang_t*
ghb_lookup_lang(const char *name)
{
    int ii;
    for (ii = 0; ii < LANG_TABLE_SIZE; ii++)
    {
        if (!strncmp(name, ghb_language_table[ii].iso639_2, 4) ||
            !strncmp(name, ghb_language_table[ii].iso639_1, 4) ||
            !strncmp(name, ghb_language_table[ii].iso639_2b, 4) ||
            !strncmp(name, ghb_language_table[ii].native_name, 4) ||
            !strncmp(name, ghb_language_table[ii].eng_name, 4))
        {
            return &ghb_language_table[ii];
        }
    }
    return NULL;
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
            tmp  = g_strdup_printf ("%d - %02dh%02dm%02ds - %s",
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
            label = g_strdup_printf("%d (%05d.MPLS) - %02dh%02dm%02ds",
                title->index, title->playlist, title->hours,
                title->minutes, title->seconds);
        }
        else
        {
            label = g_strdup_printf("%d (%05d.MPLS) - Unknown Length",
                title->index, title->playlist);
        }
    }
    else
    {
        if (title->duration != 0)
        {
            label  = g_strdup_printf("%d - %02dh%02dm%02ds",
                title->index, title->hours, title->minutes, title->seconds);
        }
        else
        {
            label  = g_strdup_printf("%d - Unknown Length",
                                    title->index);
        }
    }
    return label;
}

void
title_opts_set(GtkBuilder *builder, const gchar *name)
{
    GtkTreeIter iter;
    GtkListStore *store;
    hb_list_t  * list = NULL;
    const hb_title_t * title = NULL;
    gint ii;
    gint count = 0;


    g_debug("title_opts_set ()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
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
x264_tune_opts_set(GtkBuilder *builder, const gchar *name)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii, count = 0;

    const char * const *tunes;
    tunes = hb_video_encoder_get_tunes(HB_VCODEC_X264);
    while (tunes && tunes[count]) count++;

    g_debug("x264_tune_opts_set ()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
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
h264_profile_opts_set(GtkBuilder *builder, const gchar *name)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii, count = 0;

    const char * const *profiles;
    profiles = hb_video_encoder_get_profiles(HB_VCODEC_X264);
    while (profiles && profiles[count]) count++;

    g_debug("h264_profile_opts_set ()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
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
h264_level_opts_set(GtkBuilder *builder, const gchar *name)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii, count = 0;

    const char * const *levels;
    levels = hb_video_encoder_get_levels(HB_VCODEC_X264);
    while (levels && levels[count]) count++;

    g_debug("h264_level_opts_set ()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
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
audio_track_opts_set(GtkBuilder *builder, const gchar *name, const hb_title_t *title)
{
    GtkTreeIter iter;
    GtkListStore *store;
    hb_audio_config_t * audio;
    gint ii;
    gint count = 0;
    gchar *opt;

    g_debug("audio_track_opts_set ()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
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

void
subtitle_track_opts_set(
    GtkBuilder *builder,
    const gchar *name,
    const hb_title_t *title)
{
    GtkTreeIter iter;
    GtkListStore *store;
    hb_subtitle_t * subtitle;
    gint ii, count = 0;

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);

    if (title != NULL)
    {
        count = hb_list_count( title->list_subtitle );
    }
    if (count > 0)
    {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, _("Foreign Audio Search"),
                           1, TRUE,
                           2, "-1",
                           3, -1.0,
                           4, "auto",
                           -1);

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
    }
    else
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

#if 0
static void
generic_opts_set(GtkBuilder *builder, const gchar *name, combo_opts_t *opts)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii;

    g_debug("generic_opts_set ()\n");
    if (name == NULL || opts == NULL) return;
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    gtk_list_store_clear(store);
    for (ii = 0; ii < opts->count; ii++)
    {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, gettext(opts->map[ii].option),
                           1, TRUE,
                           2, opts->map[ii].shortOpt,
                           3, opts->map[ii].ivalue,
                           4, opts->map[ii].svalue,
                           -1);
    }
}
#endif

static void
small_opts_set(GtkBuilder *builder, const gchar *name, combo_opts_t *opts)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gint ii;
    gchar *str;

    g_debug("small_opts_set ()\n");
    if (name == NULL || opts == NULL) return;
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
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

combo_opts_t*
find_combo_table(const gchar *name)
{
    gint ii;

    for (ii = 0; combo_name_map[ii].name != NULL; ii++)
    {
        if (strcmp(name, combo_name_map[ii].name) == 0)
        {
            return combo_name_map[ii].opts;
        }
    }
    return NULL;
}

gint
ghb_lookup_combo_int(const gchar *name, const GValue *gval)
{
    if (gval == NULL)
        return 0;
    else if (strcmp(name, "SrtLanguage") == 0)
        return lookup_audio_lang_int(gval);
    else
    {
        return lookup_generic_int(find_combo_table(name), gval);
    }
    g_warning("ghb_lookup_combo_int() couldn't find %s", name);
    return 0;
}

gdouble
ghb_lookup_combo_double(const gchar *name, const GValue *gval)
{
    if (gval == NULL)
        return 0;
    else if (strcmp(name, "SrtLanguage") == 0)
        return lookup_audio_lang_int(gval);
    else
    {
        return lookup_generic_double(find_combo_table(name), gval);
    }
    g_warning("ghb_lookup_combo_double() couldn't find %s", name);
    return 0;
}

const gchar*
ghb_lookup_combo_option(const gchar *name, const GValue *gval)
{
    if (gval == NULL)
        return NULL;
    else if (strcmp(name, "SrtLanguage") == 0)
        return lookup_audio_lang_option(gval);
    else
    {
        return lookup_generic_option(find_combo_table(name), gval);
    }
    g_warning("ghb_lookup_combo_int() couldn't find %s", name);
    return NULL;
}

const gchar*
ghb_lookup_combo_string(const gchar *name, const GValue *gval)
{
    if (gval == NULL)
        return NULL;
    else if (strcmp(name, "SrtLanguage") == 0)
        return lookup_audio_lang_option(gval);
    else
    {
        return lookup_generic_string(find_combo_table(name), gval);
    }
    g_warning("ghb_lookup_combo_int() couldn't find %s", name);
    return NULL;
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
    const void *user_data,
    gboolean all)
{
    GtkComboBox *combo = NULL;
    gint signal_id;
    gint handler_id = 0;

    if (name != NULL)
    {
        g_debug("ghb_update_ui_combo_box() %s\n", name);
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
        audio_bitrate_opts_set(ud->builder, "AudioBitrate");
        audio_samplerate_opts_set(ud->builder, "AudioSamplerate");
        video_framerate_opts_set(ud->builder, "VideoFramerate");
        mix_opts_set(ud->builder, "AudioMixdown");
        video_encoder_opts_set(ud->builder, "VideoEncoder");
        audio_encoder_opts_set(ud->builder, "AudioEncoder");
        acodec_fallback_opts_set(ud->builder, "AudioEncoderFallback");
        language_opts_set(ud->builder, "SrtLanguage");
        srt_codeset_opts_set(ud->builder, "SrtCodeset");
        title_opts_set(ud->builder, "title");
        audio_track_opts_set(ud->builder, "AudioTrack", user_data);
        subtitle_track_opts_set(ud->builder, "SubtitleTrack", user_data);
        small_opts_set(ud->builder, "VideoQualityGranularity", &vqual_granularity_opts);
        small_opts_set(ud->builder, "SubtitleTrackSelectionBehavior", &subtitle_track_sel_opts);
        small_opts_set(ud->builder, "AudioTrackSelectionBehavior", &audio_track_sel_opts);
        small_opts_set(ud->builder, "PtoPType", &point_to_point_opts);
        small_opts_set(ud->builder, "WhenComplete", &when_complete_opts);
        small_opts_set(ud->builder, "PicturePAR", &par_opts);
        small_opts_set(ud->builder, "PictureModulus", &alignment_opts);
        small_opts_set(ud->builder, "LoggingLevel", &logging_opts);
        small_opts_set(ud->builder, "LogLongevity", &log_longevity_opts);
        small_opts_set(ud->builder, "check_updates", &appcast_update_opts);
        small_opts_set(ud->builder, "PictureDeinterlace", &deint_opts);
        small_opts_set(ud->builder, "PictureDetelecine", &detel_opts);
        small_opts_set(ud->builder, "PictureDecomb", &decomb_opts);
        small_opts_set(ud->builder, "PictureDenoise", &denoise_opts);
        small_opts_set(ud->builder, "x264_direct", &direct_opts);
        small_opts_set(ud->builder, "x264_b_adapt", &badapt_opts);
        small_opts_set(ud->builder, "x264_bpyramid", &bpyramid_opts);
        small_opts_set(ud->builder, "x264_weighted_pframes", &weightp_opts);
        small_opts_set(ud->builder, "x264_me", &me_opts);
        small_opts_set(ud->builder, "x264_subme", &subme_opts);
        small_opts_set(ud->builder, "x264_analyse", &analyse_opts);
        small_opts_set(ud->builder, "x264_trellis", &trellis_opts);
        x264_tune_opts_set(ud->builder, "x264Tune");
        h264_profile_opts_set(ud->builder, "h264Profile");
        h264_level_opts_set(ud->builder, "h264Level");
        container_opts_set(ud->builder, "FileFormat");
    }
    else
    {
        if (strcmp(name, "AudioBitrate") == 0)
            audio_bitrate_opts_set(ud->builder, "AudioBitrate");
        else if (strcmp(name, "AudioSamplerate") == 0)
            audio_samplerate_opts_set(ud->builder, "AudioSamplerate");
        else if (strcmp(name, "VideoFramerate") == 0)
            video_framerate_opts_set(ud->builder, "VideoFramerate");
        else if (strcmp(name, "AudioMixdown") == 0)
            mix_opts_set(ud->builder, "AudioMixdown");
        else if (strcmp(name, "VideoEncoder") == 0)
            video_encoder_opts_set(ud->builder, "VideoEncoder");
        else if (strcmp(name, "AudioEncoder") == 0)
            audio_encoder_opts_set(ud->builder, "AudioEncoder");
        else if (strcmp(name, "AudioEncoderFallback") == 0)
            acodec_fallback_opts_set(ud->builder, "AudioEncoderFallback");
        else if (strcmp(name, "SrtLanguage") == 0)
            language_opts_set(ud->builder, "SrtLanguage");
        else if (strcmp(name, "SrtCodeset") == 0)
            srt_codeset_opts_set(ud->builder, "SrtCodeset");
        else if (strcmp(name, "title") == 0)
            title_opts_set(ud->builder, "title");
        else if (strcmp(name, "SubtitleTrack") == 0)
            subtitle_track_opts_set(ud->builder, "SubtitleTrack", user_data);
        else if (strcmp(name, "AudioTrack") == 0)
            audio_track_opts_set(ud->builder, "AudioTrack", user_data);
        else if (strcmp(name, "x264Tune") == 0)
            x264_tune_opts_set(ud->builder, "x264Tune");
        else if (strcmp(name, "h264Profile") == 0)
            h264_profile_opts_set(ud->builder, "h264Profile");
        else if (strcmp(name, "h264Level") == 0)
            h264_level_opts_set(ud->builder, "h264Level");
        else if (strcmp(name, "FileFormat") == 0)
            container_opts_set(ud->builder, "FileFormat");
        else
            small_opts_set(ud->builder, name, find_combo_table(name));
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

    init_combo_box(builder, "AudioBitrate");
    init_combo_box(builder, "AudioSamplerate");
    init_combo_box(builder, "VideoFramerate");
    init_combo_box(builder, "AudioMixdown");
    init_combo_box(builder, "SrtLanguage");
    init_combo_box(builder, "SrtCodeset");
    init_combo_box(builder, "title");
    init_combo_box(builder, "AudioTrack");
    init_combo_box(builder, "SubtitleTrack");
    init_combo_box(builder, "VideoEncoder");
    init_combo_box(builder, "AudioEncoder");
    init_combo_box(builder, "AudioEncoderFallback");
    init_combo_box(builder, "x264Tune");
    init_combo_box(builder, "h264Profile");
    init_combo_box(builder, "h264Level");
    init_combo_box(builder, "FileFormat");
    for (ii = 0; combo_name_map[ii].name != NULL; ii++)
    {
        init_combo_box(builder, combo_name_map[ii].name);
    }
}

// Construct the advanced options string
// The result is allocated, so someone must free it at some point.
gchar*
ghb_build_advanced_opts_string(GValue *settings)
{
    gint vcodec;
    vcodec = ghb_settings_video_encoder_codec(settings, "VideoEncoder");
    switch (vcodec)
    {
        case HB_VCODEC_X264:
            return ghb_settings_get_string(settings, "x264Option");

        case HB_VCODEC_FFMPEG_MPEG2:
        case HB_VCODEC_FFMPEG_MPEG4:
            return ghb_settings_get_string(settings, "lavcOption");

        default:
            return NULL;
    }
}

void ghb_set_video_encoder_opts(hb_job_t *job, GValue *js)
{
    gint vcodec = ghb_settings_video_encoder_codec(js, "VideoEncoder");

    switch (vcodec)
    {
        case HB_VCODEC_X264:
        {
            if (ghb_settings_get_boolean(js, "x264UseAdvancedOptions"))
            {
                char *opts = ghb_settings_get_string(js, "x264Option");
                hb_job_set_encoder_options(job, opts);
                g_free(opts);
            }
            else
            {
                GString *str = g_string_new("");
                char *preset = ghb_settings_get_string(js, "x264Preset");
                char *tune = ghb_settings_get_string(js, "x264Tune");
                char *profile = ghb_settings_get_string(js, "h264Profile");
                char *level = ghb_settings_get_string(js, "h264Level");
                char *opts = ghb_settings_get_string(js, "x264OptionExtra");
                char *tunes;

                g_string_append_printf(str, "%s", tune);
                if (ghb_settings_get_boolean(js, "x264FastDecode"))
                {
                    g_string_append_printf(str, "%s%s", str->str[0] ? "," : "", "fastdecode");
                }
                if (ghb_settings_get_boolean(js, "x264ZeroLatency"))
                {
                    g_string_append_printf(str, "%s%s", str->str[0] ? "," : "", "zerolatency");
                }
                tunes = g_string_free(str, FALSE);

                hb_job_set_encoder_preset(job, preset);

                if (tunes != NULL && strcasecmp(tune, "none"))
                    hb_job_set_encoder_tune(job, tunes);

                if (profile != NULL && strcasecmp(profile, "auto"))
                    hb_job_set_encoder_profile(job, profile);

                if (level != NULL && strcasecmp(level, "auto"))
                    hb_job_set_encoder_level(job, level);

                hb_job_set_encoder_options(job, opts);

                g_free(preset);
                g_free(tune);
                g_free(profile);
                g_free(level);
                g_free(opts);
                g_free(tunes);
            }
        } break;

        case HB_VCODEC_FFMPEG_MPEG2:
        case HB_VCODEC_FFMPEG_MPEG4:
        {
            gchar *opts = ghb_settings_get_string(js, "lavcOption");
            if (opts != NULL && opts[0])
            {
                hb_job_set_encoder_options(job, opts);
            }
            g_free(opts);
        } break;

        case HB_VCODEC_THEORA:
        default:
        {
        } break;
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

GValue*
ghb_get_chapters(const hb_title_t *title)
{
    hb_chapter_t * chapter;
    gint count, ii;
    GValue *chapters = NULL;

    chapters = ghb_array_value_new(0);

    if (title == NULL) return chapters;
    count = hb_list_count( title->list_chapter );
    for (ii = 0; ii < count; ii++)
    {
        chapter = hb_list_item(title->list_chapter, ii);
        if (chapter == NULL) break;
        if (chapter->title == NULL || chapter->title[0] == 0)
        {
            gchar *str;
            str = g_strdup_printf ("Chapter %2d", ii+1);
            ghb_array_append(chapters, ghb_string_value_new(str));
            g_free(str);
        }
        else
        {
            ghb_array_append(chapters, ghb_string_value_new(chapter->title));
        }
    }
    return chapters;
}

gboolean
ghb_ac3_in_audio_list(const GValue *audio_list)
{
    gint count, ii;

    count = ghb_array_len(audio_list);
    for (ii = 0; ii < count; ii++)
    {
        GValue *asettings;
        gint acodec;

        asettings = ghb_array_get_nth(audio_list, ii);
        acodec = ghb_settings_audio_encoder_codec(asettings, "AudioEncoder");
        if (acodec & HB_ACODEC_AC3)
            return TRUE;
    }
    return FALSE;
}

static char custom_audio_bitrate_str[8];
static hb_rate_t custom_audio_bitrate =
{
    .name = custom_audio_bitrate_str,
    .rate = 0
};

static void
audio_bitrate_opts_add(GtkBuilder *builder, const gchar *name, gint rate)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *str;

    g_debug("audio_bitrate_opts_add ()\n");

    if (rate >= 0 && rate < 8) return;

    custom_audio_bitrate.rate = rate;
    if (rate < 0)
    {
        snprintf(custom_audio_bitrate_str, 8, "N/A");
    }
    else
    {
        snprintf(custom_audio_bitrate_str, 8, "%d", rate);
    }

    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    if (!find_combo_item_by_int(GTK_TREE_MODEL(store), rate, &iter))
    {
        str = g_strdup_printf ("<small>%s</small>", custom_audio_bitrate.name);
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, str,
                           1, TRUE,
                           2, custom_audio_bitrate.name,
                           3, (gdouble)rate,
                           4, custom_audio_bitrate.name,
                           -1);
        g_free(str);
    }
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

static void
audio_bitrate_opts_update(
    GtkBuilder *builder,
    const gchar *name,
    gint first_rate,
    gint last_rate,
    gint extra_rate)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gdouble ivalue;
    gboolean done = FALSE;

    g_debug("audio_bitrate_opts_clean ()\n");
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
    ghb_audio_bitrate_opts_filter(combo, first_rate, last_rate);

    store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL(store), &iter))
    {
        do
        {
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 3, &ivalue, -1);
            if (!search_audio_bitrates(ivalue) && (ivalue != extra_rate))
            {
                done = !gtk_list_store_remove(store, &iter);
            }
            else
            {
                done = !gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
            }
        } while (!done);
    }
    if (extra_rate > 0 && !search_audio_bitrates(extra_rate))
    {
        audio_bitrate_opts_add(builder, name, extra_rate);
    }
    else
    {
        custom_audio_bitrate.rate = 0;
        custom_audio_bitrate_str[0] = 0;
    }
}

void
ghb_audio_bitrate_opts_set(GtkComboBox *combo, gboolean extra)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *str;

    g_debug("audio_bitrate_opts_set ()\n");
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
    if (extra && custom_audio_bitrate.rate != 0)
    {
        gtk_list_store_append(store, &iter);
        str = g_strdup_printf ("<small>%s</small>", custom_audio_bitrate.name);
        gtk_list_store_set(store, &iter,
                           0, str,
                           1, TRUE,
                           2, custom_audio_bitrate.name,
                           3, (gdouble)custom_audio_bitrate.rate,
                           4, custom_audio_bitrate.name,
                           -1);
        g_free(str);
    }
}

static void
audio_bitrate_opts_set(GtkBuilder *builder, const gchar *name)
{
    GtkComboBox *combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
    ghb_audio_bitrate_opts_set(combo, TRUE);
}

void
ghb_set_bitrate_opts(
    GtkBuilder *builder,
    gint first_rate,
    gint last_rate,
    gint extra_rate)
{
    audio_bitrate_opts_update(builder, "AudioBitrate", first_rate, last_rate, extra_rate);
}

const char*
ghb_audio_bitrate_get_short_name(int rate)
{
    if (rate == custom_audio_bitrate.rate)
    {
        return custom_audio_bitrate.name;
    }

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
    const hb_rate_t *hb_rate, *first;
    for (first = hb_rate = hb_audio_bitrate_get_next(NULL); hb_rate != NULL;
         hb_rate = hb_audio_bitrate_get_next(hb_rate))
    {
        if (!strncmp(name, hb_rate->name, 8))
        {
            return hb_rate;
        }
    }
    // Return a default rate if nothing matches
    return first;
}

int
ghb_lookup_audio_bitrate_rate(const char *name)
{
    return ghb_lookup_audio_bitrate(name)->rate;
}

int
ghb_settings_audio_bitrate_rate(const GValue *settings, const char *name)
{
    const char *rate_id = ghb_settings_get_const_string(settings, name);
    return ghb_lookup_audio_bitrate_rate(rate_id);
}

const hb_rate_t*
ghb_settings_audio_bitrate(const GValue *settings, const char *name)
{
    const char *rate_id = ghb_settings_get_const_string(settings, name);
    return ghb_lookup_audio_bitrate(rate_id);
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
}

void
ghb_backend_close()
{
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
    hb_scan( h_queue, path, titlenum, 10, 0, 0 );
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

void
ghb_track_status()
{
    hb_state_t s_scan;
    hb_state_t s_queue;

    if (h_scan == NULL) return;
    hb_get_state( h_scan, &s_scan );
    switch( s_scan.state )
    {
#define p s_scan.param.scanning
        case HB_STATE_SCANNING:
        {
            hb_status.scan.state |= GHB_STATE_SCANNING;
            hb_status.scan.title_count = p.title_count;
            hb_status.scan.title_cur = p.title_cur;
            hb_status.scan.preview_count = p.preview_count;
            hb_status.scan.preview_cur = p.preview_cur;
            hb_status.scan.progress = p.progress;
        } break;
#undef p

        case HB_STATE_SCANDONE:
        {
            hb_status.scan.state &= ~GHB_STATE_SCANNING;
            hb_status.scan.state |= GHB_STATE_SCANDONE;
        } break;

#define p s_scan.param.working
        case HB_STATE_WORKING:
            hb_status.scan.state |= GHB_STATE_WORKING;
            hb_status.scan.state &= ~GHB_STATE_PAUSED;
            hb_status.scan.job_cur = p.job_cur;
            hb_status.scan.job_count = p.job_count;
            hb_status.scan.progress = p.progress;
            hb_status.scan.rate_cur = p.rate_cur;
            hb_status.scan.rate_avg = p.rate_avg;
            hb_status.scan.hours = p.hours;
            hb_status.scan.minutes = p.minutes;
            hb_status.scan.seconds = p.seconds;
            hb_status.scan.unique_id = p.sequence_id & 0xFFFFFF;
            break;
#undef p

        case HB_STATE_PAUSED:
            hb_status.scan.state |= GHB_STATE_PAUSED;
            break;

        case HB_STATE_MUXING:
        {
            hb_status.scan.state |= GHB_STATE_MUXING;
        } break;

#define p s_scan.param.workdone
        case HB_STATE_WORKDONE:
        {
            hb_job_t *job;

            hb_status.scan.state |= GHB_STATE_WORKDONE;
            hb_status.scan.state &= ~GHB_STATE_MUXING;
            hb_status.scan.state &= ~GHB_STATE_PAUSED;
            hb_status.scan.state &= ~GHB_STATE_WORKING;
            switch (p.error)
            {
            case HB_ERROR_NONE:
                hb_status.scan.error = GHB_ERROR_NONE;
                break;
            case HB_ERROR_CANCELED:
                hb_status.scan.error = GHB_ERROR_CANCELED;
                break;
            default:
                hb_status.scan.error = GHB_ERROR_FAIL;
                break;
            }
            // Delete all remaining jobs of this encode.
            // An encode can be composed of multiple associated jobs.
            // When a job is stopped, libhb removes it from the job list,
            // but does not remove other jobs that may be associated with it.
            // Associated jobs are taged in the sequence id.
            while ((job = hb_job(h_scan, 0)) != NULL)
                hb_rem( h_scan, job );
        } break;
#undef p
    }
    hb_get_state( h_queue, &s_queue );
    switch( s_queue.state )
    {
#define p s_queue.param.scanning
        case HB_STATE_SCANNING:
        {
            hb_status.queue.state |= GHB_STATE_SCANNING;
            hb_status.queue.title_count = p.title_count;
            hb_status.queue.title_cur = p.title_cur;
            hb_status.queue.preview_count = p.preview_count;
            hb_status.queue.preview_cur = p.preview_cur;
            hb_status.queue.progress = p.progress;
        } break;
#undef p

        case HB_STATE_SCANDONE:
        {
            hb_status.queue.state &= ~GHB_STATE_SCANNING;
            hb_status.queue.state |= GHB_STATE_SCANDONE;
        } break;

#define p s_queue.param.working
        case HB_STATE_WORKING:
            hb_status.queue.state |= GHB_STATE_WORKING;
            hb_status.queue.state &= ~GHB_STATE_PAUSED;
            hb_status.queue.state &= ~GHB_STATE_SEARCHING;
            hb_status.queue.job_cur = p.job_cur;
            hb_status.queue.job_count = p.job_count;
            hb_status.queue.progress = p.progress;
            hb_status.queue.rate_cur = p.rate_cur;
            hb_status.queue.rate_avg = p.rate_avg;
            hb_status.queue.hours = p.hours;
            hb_status.queue.minutes = p.minutes;
            hb_status.queue.seconds = p.seconds;
            hb_status.queue.unique_id = p.sequence_id & 0xFFFFFF;
            break;

        case HB_STATE_SEARCHING:
            hb_status.queue.state |= GHB_STATE_SEARCHING;
            hb_status.queue.state &= ~GHB_STATE_WORKING;
            hb_status.queue.state &= ~GHB_STATE_PAUSED;
            hb_status.queue.job_cur = p.job_cur;
            hb_status.queue.job_count = p.job_count;
            hb_status.queue.progress = p.progress;
            hb_status.queue.rate_cur = p.rate_cur;
            hb_status.queue.rate_avg = p.rate_avg;
            hb_status.queue.hours = p.hours;
            hb_status.queue.minutes = p.minutes;
            hb_status.queue.seconds = p.seconds;
            hb_status.queue.unique_id = p.sequence_id & 0xFFFFFF;
            break;
#undef p

        case HB_STATE_PAUSED:
            hb_status.queue.state |= GHB_STATE_PAUSED;
            break;

        case HB_STATE_MUXING:
        {
            hb_status.queue.state |= GHB_STATE_MUXING;
        } break;

#define p s_queue.param.workdone
        case HB_STATE_WORKDONE:
        {
            hb_job_t *job;

            hb_status.queue.state |= GHB_STATE_WORKDONE;
            hb_status.queue.state &= ~GHB_STATE_MUXING;
            hb_status.queue.state &= ~GHB_STATE_PAUSED;
            hb_status.queue.state &= ~GHB_STATE_WORKING;
            hb_status.queue.state &= ~GHB_STATE_SEARCHING;
            switch (p.error)
            {
            case HB_ERROR_NONE:
                hb_status.queue.error = GHB_ERROR_NONE;
                break;
            case HB_ERROR_CANCELED:
                hb_status.queue.error = GHB_ERROR_CANCELED;
                break;
            default:
                hb_status.queue.error = GHB_ERROR_FAIL;
                break;
            }
            // Delete all remaining jobs of this encode.
            // An encode can be composed of multiple associated jobs.
            // When a job is stopped, libhb removes it from the job list,
            // but does not remove other jobs that may be associated with it.
            // Associated jobs are taged in the sequence id.
            while ((job = hb_job(h_queue, 0)) != NULL)
                hb_rem( h_queue, job );
        } break;
#undef p
    }
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
    enable_keep_aspect = (pic_par != 1 && pic_par != 2);
    keep_aspect = ghb_settings_get_boolean(ud->settings, "PictureKeepRatio");
    autoscale = ghb_settings_get_boolean(ud->settings, "autoscale");

    enable_scale_width = !autoscale && (pic_par != 1);
    enable_scale_height = !autoscale && (pic_par != 1);
    enable_disp_width = (pic_par == 3) && !keep_aspect;
    enable_par = (pic_par == 3) && !keep_aspect;
    enable_disp_height = FALSE;

    widget = GHB_WIDGET(ud->builder, "PictureModulus");
    gtk_widget_set_sensitive(widget, pic_par != 1);
    widget = GHB_WIDGET(ud->builder, "PictureLooseCrop");
    gtk_widget_set_sensitive(widget, pic_par != 1);
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
    gtk_widget_set_sensitive(widget, pic_par != 1);
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
ghb_set_scale_settings(GValue *settings, gint mode)
{
    hb_job_t   * job;
    gboolean keep_aspect;
    gint pic_par;
    gboolean autocrop, autoscale, noscale;
    gint crop[4] = {0,};
    gint width, height, par_width, par_height;
    gint crop_width, crop_height;
    gint aspect_n, aspect_d;
    gboolean keep_width = (mode & GHB_PIC_KEEP_WIDTH);
    gboolean keep_height = (mode & GHB_PIC_KEEP_HEIGHT);
    gint mod;
    gint max_width = 0;
    gint max_height = 0;

    g_debug("ghb_set_scale ()\n");

    pic_par = ghb_settings_combo_int(settings, "PicturePAR");
    if (pic_par == 1)
    {
        ghb_settings_set_boolean(settings, "autoscale", TRUE);
        ghb_settings_set_int(settings, "PictureModulus", 2);
        ghb_settings_set_boolean(settings, "PictureLooseCrop", TRUE);
    }
    if (pic_par == 1 || pic_par == 2)
    {
        ghb_settings_set_boolean(settings, "PictureKeepRatio", TRUE);
    }

    int title_id, titleindex;
    const hb_title_t * title;

    title_id = ghb_settings_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL) return;

    job = hb_job_init( (hb_title_t*)title );
    if (job == NULL) return;

    // First configure widgets
    mod = ghb_settings_combo_int(settings, "PictureModulus");
    keep_aspect = ghb_settings_get_boolean(settings, "PictureKeepRatio");
    autocrop = ghb_settings_get_boolean(settings, "PictureAutoCrop");
    autoscale = ghb_settings_get_boolean(settings, "autoscale");
    // "Noscale" is a flag that says we prefer to crop extra to satisfy
    // alignment constraints rather than scaling to satisfy them.
    noscale = ghb_settings_get_boolean(settings, "PictureLooseCrop");
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
        ghb_settings_set_int(settings, "PictureTopCrop", crop[0]);
        ghb_settings_set_int(settings, "PictureBottomCrop", crop[1]);
        ghb_settings_set_int(settings, "PictureLeftCrop", crop[2]);
        ghb_settings_set_int(settings, "PictureRightCrop", crop[3]);
    }
    else
    {
        crop[0] = ghb_settings_get_int(settings, "PictureTopCrop");
        crop[1] = ghb_settings_get_int(settings, "PictureBottomCrop");
        crop[2] = ghb_settings_get_int(settings, "PictureLeftCrop");
        crop[3] = ghb_settings_get_int(settings, "PictureRightCrop");
        // Prevent manual crop from creating too small an image
        if (title->height - crop[0] < crop[1] + 16)
        {
            crop[0] = title->height - crop[1] - 16;
        }
        if (title->width - crop[2] < crop[3] + 16)
        {
            crop[2] = title->width - crop[3] - 16;
        }
    }
    if (noscale)
    {
        gint need1, need2;

        // Adjust the cropping to accomplish the desired width and height
        crop_width = title->width - crop[2] - crop[3];
        crop_height = title->height - crop[0] - crop[1];
        width = MOD_DOWN(crop_width, mod);
        height = MOD_DOWN(crop_height, mod);

        need1 = (crop_height - height) / 2;
        need2 = crop_height - height - need1;
        crop[0] += need1;
        crop[1] += need2;
        need1 = (crop_width - width) / 2;
        need2 = crop_width - width - need1;
        crop[2] += need1;
        crop[3] += need2;
        ghb_settings_set_int(settings, "PictureTopCrop", crop[0]);
        ghb_settings_set_int(settings, "PictureBottomCrop", crop[1]);
        ghb_settings_set_int(settings, "PictureLeftCrop", crop[2]);
        ghb_settings_set_int(settings, "PictureRightCrop", crop[3]);
    }
    hb_reduce(&aspect_n, &aspect_d,
                title->width * title->pixel_aspect_width,
                title->height * title->pixel_aspect_height);
    crop_width = title->width - crop[2] - crop[3];
    crop_height = title->height - crop[0] - crop[1];
    if (autoscale)
    {
        width = crop_width;
        height = crop_height;
    }
    else
    {
        width = ghb_settings_get_int(settings, "scale_width");
        height = ghb_settings_get_int(settings, "scale_height");
        if (mode & GHB_PIC_USE_MAX)
        {
            max_width = MOD_DOWN(
                ghb_settings_get_int(settings, "PictureWidth"), mod);
            max_height = MOD_DOWN(
                ghb_settings_get_int(settings, "PictureHeight"), mod);
        }
    }
    g_debug("max_width %d, max_height %d\n", max_width, max_height);

    if (width < 16)
        width = 16;
    if (height < 16)
        height = 16;

    width = MOD_ROUND(width, mod);
    height = MOD_ROUND(height, mod);

    job->anamorphic.mode = pic_par;
    if (pic_par)
    {
        // The scaler crashes if the dimensions are not divisible by 2
        // Align mod 2.  And so does something in x264_encoder_headers()
        job->modulus = mod;
        job->anamorphic.par_width = title->pixel_aspect_width;
        job->anamorphic.par_height = title->pixel_aspect_height;
        job->anamorphic.dar_width = 0;
        job->anamorphic.dar_height = 0;

        if (keep_height && pic_par == 2)
        {
            width = ((double)height * crop_width / crop_height);
            width = MOD_ROUND(width, mod);
        }
        job->width = width;
        job->height = height;
        job->maxWidth = max_width;
        job->maxHeight = max_height;
        job->crop[0] = crop[0];
        job->crop[1] = crop[1];
        job->crop[2] = crop[2];
        job->crop[3] = crop[3];
        if (job->anamorphic.mode == 3 && !keep_aspect)
        {
            job->anamorphic.keep_display_aspect = 0;
            if (mode & GHB_PIC_KEEP_PAR)
            {
                job->anamorphic.par_width =
                    ghb_settings_get_int(settings, "PicturePARWidth");
                job->anamorphic.par_height =
                    ghb_settings_get_int(settings, "PicturePARHeight");
            }
            else
            {
                job->anamorphic.dar_width =
                    ghb_settings_get_int(settings, "PictureDisplayWidth");
                job->anamorphic.dar_height = height;
            }
        }
        else
        {
            job->anamorphic.keep_display_aspect = 1;
        }
        // hb_set_anamorphic_size will adjust par, dar, and width/height
        // to conform to job parameters that have been set, including
        // maxWidth and maxHeight
        hb_set_anamorphic_size(job, &width, &height, &par_width, &par_height);
        if (job->anamorphic.mode == 3 && !keep_aspect &&
            mode & GHB_PIC_KEEP_PAR)
        {
            // hb_set_anamorphic_size reduces the par, which we
            // don't want in this case because the user is
            // explicitely specifying it.
            par_width = ghb_settings_get_int(settings, "PicturePARWidth");
            par_height = ghb_settings_get_int(settings, "PicturePARHeight");
        }
    }
    else
    {
        // Adjust dims according to max values
        if (max_height) height = MIN(height, max_height);
        if (max_width) width = MIN(width, max_width);

        if (keep_aspect)
        {
            gdouble par;
            gint new_width, new_height;

            // Compute pixel aspect ration.
            par = (gdouble)(title->height * aspect_n) / (title->width * aspect_d);
            // Must scale so that par becomes 1:1
            // Try to keep largest dimension
            new_height = (crop_height * ((gdouble)width/crop_width) / par);
            new_width = (crop_width * ((gdouble)height/crop_height) * par);

            if (max_width && new_width > max_width)
            {
                height = new_height;
            }
            else if (max_height && new_height > max_height)
            {
                width = new_width;
            }
            else if (keep_width)
            {
                height = new_height;
            }
            else if (keep_height)
            {
                width = new_width;
            }
            else if (width > new_width)
            {
                height = new_height;
            }
            else
            {
                width = new_width;
            }
            g_debug("new w %d h %d\n", width, height);
        }
        width = MOD_ROUND(width, mod);
        height = MOD_ROUND(height, mod);
        if (max_height)
            height = MIN(height, max_height);
        if (max_width)
            width = MIN(width, max_width);
        par_width = par_height = 1;
    }
    ghb_settings_set_int(settings, "scale_width", width);
    ghb_settings_set_int(settings, "scale_height", height);

    gint disp_width;

    disp_width = ((gdouble)par_width / par_height) * width + 0.5;
    ghb_limit_rational(&par_width, &par_height, 65535);

    ghb_settings_set_int(settings, "PicturePARWidth", par_width);
    ghb_settings_set_int(settings, "PicturePARHeight", par_height);
    ghb_settings_set_int(settings, "PictureDisplayWidth", disp_width);
    ghb_settings_set_int(settings, "PictureDisplayHeight", height);
    hb_job_close( &job );
}

void
ghb_update_display_aspect_label(signal_user_data_t *ud)
{
    gint disp_width, disp_height, dar_width, dar_height;
    gchar *str;

    disp_width = ghb_settings_get_int(ud->settings, "PictureDisplayWidth");
    disp_height = ghb_settings_get_int(ud->settings, "PictureDisplayHeight");
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

    // "Noscale" is a flag that says we prefer to crop extra to satisfy
    // alignment constraints rather than scaling to satisfy them.
    gboolean noscale = ghb_settings_get_boolean(ud->settings, "PictureLooseCrop");
    if (noscale)
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
set_preview_job_settings(signal_user_data_t *ud, hb_job_t *job, GValue *settings)
{
    job->crop[0] = ghb_settings_get_int(settings, "PictureTopCrop");
    job->crop[1] = ghb_settings_get_int(settings, "PictureBottomCrop");
    job->crop[2] = ghb_settings_get_int(settings, "PictureLeftCrop");
    job->crop[3] = ghb_settings_get_int(settings, "PictureRightCrop");

    job->anamorphic.mode = ghb_settings_combo_int(settings, "PicturePAR");
    job->modulus =
        ghb_settings_combo_int(settings, "PictureModulus");
    job->width = ghb_settings_get_int(settings, "scale_width");
    job->height = ghb_settings_get_int(settings, "scale_height");
    if (ghb_settings_get_boolean(ud->prefs, "preview_show_crop"))
    {
        gdouble xscale = (gdouble)job->width /
            (gdouble)(job->title->width - job->crop[2] - job->crop[3]);
        gdouble yscale = (gdouble)job->height /
            (gdouble)(job->title->height - job->crop[0] - job->crop[1]);

        job->width += xscale * (job->crop[2] + job->crop[3]);
        job->height += yscale * (job->crop[0] + job->crop[1]);
        job->crop[0] = 0;
        job->crop[1] = 0;
        job->crop[2] = 0;
        job->crop[3] = 0;
        job->modulus = 2;
    }

    gboolean decomb_deint = ghb_settings_get_boolean(settings, "PictureDecombDeinterlace");
    if (decomb_deint)
    {
        gint decomb = ghb_settings_combo_int(settings, "PictureDecomb");
        job->deinterlace = (decomb == 0) ? 0 : 1;
    }
    else
    {
        gint deint = ghb_settings_combo_int(settings, "PictureDeinterlace");
        job->deinterlace = (deint == 0) ? 0 : 1;
    }

    gboolean keep_aspect;
    keep_aspect = ghb_settings_get_boolean(settings, "PictureKeepRatio");
    if (job->anamorphic.mode)
    {
        job->anamorphic.par_width = job->title->pixel_aspect_width;
        job->anamorphic.par_height = job->title->pixel_aspect_height;
        job->anamorphic.dar_width = 0;
        job->anamorphic.dar_height = 0;

        if (job->anamorphic.mode == 3 && !keep_aspect)
        {
            job->anamorphic.keep_display_aspect = 0;
            job->anamorphic.par_width =
                ghb_settings_get_int(settings, "PicturePARWidth");
            job->anamorphic.par_height =
                ghb_settings_get_int(settings, "PicturePARHeight");
        }
        else
        {
            job->anamorphic.keep_display_aspect = 1;
        }
    }
}

gboolean
ghb_validate_filter_string(const gchar *str, gint max_fields)
{
    gint fields = 0;
    gchar *end;

    if (str == NULL || *str == 0) return TRUE;
    while (*str)
    {
        g_strtod(str, &end);
        if (str != end)
        { // Found a numeric value
            fields++;
            // negative max_fields means infinate
            if (max_fields >= 0 && fields > max_fields) return FALSE;
            if (*end == 0)
                return TRUE;
            if (*end != ':')
                return FALSE;
            str = end + 1;
        }
        else
            return FALSE;
    }
    return FALSE;
}

gboolean
ghb_validate_filters(GValue *settings)
{
    gchar *str;
    gint index;
    gchar *message;

    gboolean decomb_deint = ghb_settings_get_boolean(settings, "PictureDecombDeinterlace");
    // deinte
    index = ghb_settings_combo_int(settings, "PictureDeinterlace");
    if (!decomb_deint && index == 1)
    {
        str = ghb_settings_get_string(settings, "PictureDeinterlaceCustom");
        if (!ghb_validate_filter_string(str, -1))
        {
            message = g_strdup_printf(
                        _("Invalid Deinterlace Settings:\n\n%s\n"),
                        str);
            ghb_message_dialog(GTK_MESSAGE_ERROR, message, _("Cancel"), NULL);
            g_free(message);
            g_free(str);
            return FALSE;
        }
        g_free(str);
    }
    // detel
    index = ghb_settings_combo_int(settings, "PictureDetelecine");
    if (index == 1)
    {
        str = ghb_settings_get_string(settings, "PictureDetelecineCustom");
        if (!ghb_validate_filter_string(str, -1))
        {
            message = g_strdup_printf(
                        _("Invalid Detelecine Settings:\n\n%s\n"),
                        str);
            ghb_message_dialog(GTK_MESSAGE_ERROR, message, _("Cancel"), NULL);
            g_free(message);
            g_free(str);
            return FALSE;
        }
        g_free(str);
    }
    // decomb
    index = ghb_settings_combo_int(settings, "PictureDecomb");
    if (decomb_deint && index == 1)
    {
        str = ghb_settings_get_string(settings, "PictureDecombCustom");
        if (!ghb_validate_filter_string(str, -1))
        {
            message = g_strdup_printf(
                        _("Invalid Decomb Settings:\n\n%s\n"),
                        str);
            ghb_message_dialog(GTK_MESSAGE_ERROR, message, _("Cancel"), NULL);
            g_free(message);
            g_free(str);
            return FALSE;
        }
        g_free(str);
    }
    // denois
    index = ghb_settings_combo_int(settings, "PictureDenoise");
    if (index == 1)
    {
        str = ghb_settings_get_string(settings, "PictureDenoiseCustom");
        if (!ghb_validate_filter_string(str, -1))
        {
            message = g_strdup_printf(
                        _("Invalid Denoise Settings:\n\n%s\n"),
                        str);
            ghb_message_dialog(GTK_MESSAGE_ERROR, message, _("Cancel"), NULL);
            g_free(str);
            g_free(message);
            return FALSE;
        }
        g_free(str);
    }
    return TRUE;
}

gboolean
ghb_validate_video(GValue *settings)
{
    gint vcodec;
    gchar *message;
    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_settings_get_const_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    vcodec = ghb_settings_video_encoder_codec(settings, "VideoEncoder");
    if ((mux->format & HB_MUX_MASK_MP4) && (vcodec == HB_VCODEC_THEORA))
    {
        // mp4/theora combination is not supported.
        message = g_strdup_printf(
                    _("Theora is not supported in the MP4 container.\n\n"
                    "You should choose a different video codec or container.\n"
                    "If you continue, FFMPEG will be chosen for you."));
        if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, _("Cancel"), _("Continue")))
        {
            g_free(message);
            return FALSE;
        }
        g_free(message);
        vcodec = hb_video_encoder_get_default(mux->format);
        ghb_settings_set_string(settings, "VideoEncoder",
                                hb_video_encoder_get_short_name(vcodec));
    }
    return TRUE;
}

gboolean
ghb_validate_subtitles(GValue *settings)
{
    gint title_id, titleindex;
    const hb_title_t * title;
    gchar *message;

    title_id = ghb_settings_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL)
    {
        /* No valid title, stop right there */
        g_message("No title found.\n");
        return FALSE;
    }

    const GValue *slist, *subtitle;
    gint count, ii, source;
    gboolean burned, one_burned = FALSE;

    slist = ghb_settings_get_value(settings, "subtitle_list");
    count = ghb_array_len(slist);
    for (ii = 0; ii < count; ii++)
    {
        subtitle = ghb_array_get_nth(slist, ii);
        source = ghb_settings_get_int(subtitle, "SubtitleSource");
        burned = ghb_settings_get_boolean(subtitle, "SubtitleBurned");
        if (burned && one_burned)
        {
            // MP4 can only handle burned vobsubs.  make sure there isn't
            // already something burned in the list
            message = g_strdup_printf(
            _("Only one subtitle may be burned into the video.\n\n"
                "You should change your subtitle selections.\n"
                "If you continue, some subtitles will be lost."));
            if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, _("Cancel"), _("Continue")))
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
        if (source == SRTSUB)
        {
            gchar *filename;

            filename = ghb_settings_get_string(subtitle, "SrtFile");
            if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR))
            {
                message = g_strdup_printf(
                _("Srt file does not exist or not a regular file.\n\n"
                    "You should choose a valid file.\n"
                    "If you continue, this subtitle will be ignored."));
                if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message,
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
ghb_validate_audio(GValue *settings)
{
    gint title_id, titleindex;
    const hb_title_t * title;
    gchar *message;

    title_id = ghb_settings_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL)
    {
        /* No valid title, stop right there */
        g_message("No title found.\n");
        return FALSE;
    }

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_settings_get_const_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    const GValue *audio_list;
    gint count, ii;

    audio_list = ghb_settings_get_value(settings, "audio_list");
    count = ghb_array_len(audio_list);
    for (ii = 0; ii < count; ii++)
    {
        GValue *asettings;
        hb_audio_config_t *aconfig;
        int track, codec;

        asettings = ghb_array_get_nth(audio_list, ii);
        track = ghb_settings_get_int(asettings, "AudioTrack");
        codec = ghb_settings_audio_encoder_codec(asettings, "AudioEncoder");
        if (codec == HB_ACODEC_AUTO_PASS)
            continue;

        aconfig = (hb_audio_config_t *) hb_list_audio_config_item(
                                            title->list_audio, track );
        if ( ghb_audio_is_passthru(codec) &&
            !(ghb_audio_can_passthru(aconfig->in.codec) &&
             (aconfig->in.codec & codec)))
        {
            // Not supported.  AC3 is passthrough only, so input must be AC3
            message = g_strdup_printf(
                        _("The source does not support Pass-Thru.\n\n"
                        "You should choose a different audio codec.\n"
                        "If you continue, one will be chosen for you."));
            if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, _("Cancel"), _("Continue")))
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
            ghb_settings_set_string(asettings, "AudioEncoder", name);
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
            if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, _("Cancel"), _("Continue")))
            {
                g_free(message);
                return FALSE;
            }
            g_free(message);
            const char *name = hb_audio_encoder_get_short_name(codec);
            ghb_settings_set_string(asettings, "AudioEncoder", name);
        }

        const hb_mixdown_t *mix;
        mix = ghb_settings_mixdown(asettings, "AudioMixdown");

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
            if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, _("Cancel"), _("Continue")))
            {
                g_free(message);
                return FALSE;
            }
            g_free(message);
            int amixdown = ghb_get_best_mix(aconfig, codec, mix->amixdown);
            ghb_settings_set_string(asettings, "AudioMixdown",
                                    hb_mixdown_get_short_name(amixdown));
        }
    }
    return TRUE;
}

gboolean
ghb_validate_vquality(GValue *settings)
{
    gint vcodec;
    gchar *message;
    gint min, max;

    vcodec = ghb_settings_video_encoder_codec(settings, "VideoEncoder");
    gdouble vquality;
    vquality = ghb_settings_get_double(settings, "VideoQualitySlider");
    if (ghb_settings_get_boolean(settings, "vquality_type_constant"))
    {
        switch (vcodec)
        {
            case HB_VCODEC_X264:
            {
                min = 16;
                max = 30;
            } break;

            case HB_VCODEC_FFMPEG_MPEG2:
            case HB_VCODEC_FFMPEG_MPEG4:
            {
                min = 1;
                max = 8;
            } break;

            case HB_VCODEC_THEORA:
            case HB_VCODEC_FFMPEG_VP8:
            {
                min = 0;
                max = 63;
            } break;

            default:
            {
                min = 48;
                max = 62;
            } break;
        }
        if (vcodec == HB_VCODEC_X264 && vquality == 0.0)
        {
            message = g_strdup_printf(
                        _("Warning: lossless h.264 selected\n\n"
                        "Lossless h.264 is not well supported by\n"
                        "many players and editors.\n\n"
                        "It will produce enormous output files.\n\n"
                        "Are you sure you wish to use this setting?"));
            if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message,
                                    _("Cancel"), _("Continue")))
            {
                g_free(message);
                return FALSE;
            }
            g_free(message);
            ghb_settings_set_string(settings, "h264Profile", "auto");
        }
        else if (vquality < min || vquality > max)
        {
            message = g_strdup_printf(
                        _("Interesting video quality choice: %d\n\n"
                        "Typical values range from %d to %d.\n\n"
                        "Are you sure you wish to use this setting?"),
                        (gint)vquality, min, max);
            if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message,
                                    _("Cancel"), _("Continue")))
            {
                g_free(message);
                return FALSE;
            }
            g_free(message);
        }
    }
    return TRUE;
}

static void
add_job(hb_handle_t *h, GValue *js, gint unique_id, int titleindex)
{
    hb_list_t  * list;
    const hb_title_t * title;
    hb_job_t   * job;
    gint sub_id = 0;
    hb_filter_object_t * filter;
    gchar *filter_str;
    gchar *dest_str = NULL;
    GValue *prefs;

    g_debug("add_job()\n");
    if (h == NULL) return;
    list = hb_get_titles( h );
    if( !hb_list_count( list ) )
    {
        /* No valid title, stop right there */
        return;
    }

    title = hb_list_item( list, titleindex );
    if (title == NULL) return;

    /* Set job settings */
    job = hb_job_init( (hb_title_t*)title );
    if (job == NULL) return;

    prefs = ghb_settings_get_value(js, "Preferences");
    job->angle = ghb_settings_get_int(js, "angle");
    job->start_at_preview = ghb_settings_get_int(js, "start_frame") + 1;
    if (job->start_at_preview)
    {
        job->seek_points = ghb_settings_get_int(prefs, "preview_count");
        job->pts_to_stop = ghb_settings_get_int(prefs, "live_duration") * 90000LL;
    }

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_settings_get_const_string(js, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    job->mux = mux->format;
    if (job->mux & HB_MUX_MASK_MP4)
    {
        job->largeFileSize = ghb_settings_get_boolean(js, "Mp4LargeFile");
        job->mp4_optimize = ghb_settings_get_boolean(js, "Mp4HttpOptimize");
    }
    else
    {
        job->largeFileSize = FALSE;
        job->mp4_optimize = FALSE;
    }
    if (!job->start_at_preview)
    {
        gint start, end;
        gint num_chapters = hb_list_count(title->list_chapter);
        gint duration = title->duration / 90000;
        job->chapter_markers = FALSE;
        job->chapter_start = 1;
        job->chapter_end = num_chapters;

        if (ghb_settings_combo_int(js, "PtoPType") == 0)
        {
            start = ghb_settings_get_int(js, "start_point");
            end = ghb_settings_get_int(js, "end_point");
            job->chapter_start = MIN( num_chapters, start );
            job->chapter_end   = MAX( job->chapter_start, end );

        }
        if (ghb_settings_combo_int(js, "PtoPType") == 1)
        {
            start = ghb_settings_get_int(js, "start_point");
            end = ghb_settings_get_int(js, "end_point");
            job->pts_to_start = (int64_t)MIN(duration-1, start) * 90000;
            job->pts_to_stop = (int64_t)MAX(start+1, end) * 90000 -
                                job->pts_to_start;
        }
        if (ghb_settings_combo_int(js, "PtoPType") == 2)
        {
            start = ghb_settings_get_int(js, "start_point");
            end = ghb_settings_get_int(js, "end_point");
            gint64 max_frames;
            max_frames = (gint64)duration * title->rate / title->rate_base;
            job->frame_to_start = (int64_t)MIN(max_frames-1, start-1);
            job->frame_to_stop = (int64_t)MAX(start, end-1) -
                                 job->frame_to_start;
        }
        if (job->chapter_start != job->chapter_end)
        {
            job->chapter_markers = ghb_settings_get_boolean(js, "ChapterMarkers");
        }
        if (job->chapter_start == job->chapter_end)
            job->chapter_markers = 0;
        if ( job->chapter_markers )
        {
            GValue *chapters;
            GValue *chapter;
            gint chap;
            gint count;

            chapters = ghb_settings_get_value(js, "chapter_list");
            count = ghb_array_len(chapters);
            for(chap = 0; chap < count; chap++)
            {
                hb_chapter_t * chapter_s;
                gchar *name;

                name = NULL;
                chapter = ghb_array_get_nth(chapters, chap);
                name = ghb_value_string(chapter);
                if (name == NULL)
                {
                    name = g_strdup_printf ("Chapter %2d", chap+1);
                }
                chapter_s = hb_list_item( job->list_chapter, chap);
                hb_chapter_set_title(chapter_s, name);
                g_free(name);
            }
        }
    }

    gboolean decomb_deint = ghb_settings_get_boolean(js, "PictureDecombDeinterlace");
    gint decomb = ghb_settings_combo_int(js, "PictureDecomb");
    gint deint = ghb_settings_combo_int(js, "PictureDeinterlace");
    if (!decomb_deint)
        job->deinterlace = (deint != 0) ? 1 : 0;
    else
        job->deinterlace = 0;
    job->grayscale   = ghb_settings_get_boolean(js, "VideoGrayScale");

    gboolean keep_aspect;
    keep_aspect = ghb_settings_get_boolean(js, "PictureKeepRatio");
    job->anamorphic.mode = ghb_settings_combo_int(js, "PicturePAR");
    job->modulus = ghb_settings_combo_int(js, "PictureModulus");
    if (job->anamorphic.mode)
    {
        job->anamorphic.par_width =
            ghb_settings_get_int(js, "PicturePARWidth");
        job->anamorphic.par_height =
            ghb_settings_get_int(js, "PicturePARHeight");
        job->anamorphic.dar_width = 0;
        job->anamorphic.dar_height = 0;

        if (job->anamorphic.mode == 3 && !keep_aspect)
        {
            job->anamorphic.keep_display_aspect = 0;
        }
        else
        {
            job->anamorphic.keep_display_aspect = 1;
        }
    }

    int width, height, crop[4];
    width = ghb_settings_get_int(js, "scale_width");
    height = ghb_settings_get_int(js, "scale_height");

    crop[0] = ghb_settings_get_int(js, "PictureTopCrop");
    crop[1] = ghb_settings_get_int(js, "PictureBottomCrop");
    crop[2] = ghb_settings_get_int(js, "PictureLeftCrop");
    crop[3] = ghb_settings_get_int(js, "PictureRightCrop");

    filter_str = g_strdup_printf("%d:%d:%d:%d:%d:%d",
                            width, height, crop[0], crop[1], crop[2], crop[3]);
    filter = hb_filter_init(HB_FILTER_CROP_SCALE);
    hb_add_filter( job, filter, filter_str );
    g_free(filter_str);

    /* Add selected filters */
    gint detel = ghb_settings_combo_int(js, "PictureDetelecine");
    if ( detel )
    {
        filter_str = NULL;
        if (detel != 1)
        {
            if (detel_opts.map[detel].svalue != NULL)
                filter_str = g_strdup(detel_opts.map[detel].svalue);
        }
        else
            filter_str = ghb_settings_get_string(js, "PictureDetelecineCustom");
        filter = hb_filter_init(HB_FILTER_DETELECINE);
        hb_add_filter( job, filter, filter_str );
        g_free(filter_str);
    }
    if ( decomb_deint && decomb )
    {
        filter_str = NULL;
        if (decomb != 1)
        {
            if (decomb_opts.map[decomb].svalue != NULL)
                filter_str = g_strdup(decomb_opts.map[decomb].svalue);
        }
        else
            filter_str = ghb_settings_get_string(js, "PictureDecombCustom");
        filter = hb_filter_init(HB_FILTER_DECOMB);
        hb_add_filter( job, filter, filter_str );
        g_free(filter_str);
    }
    if( job->deinterlace )
    {
        filter_str = NULL;
        if (deint != 1)
        {
            if (deint_opts.map[deint].svalue != NULL)
                filter_str = g_strdup(deint_opts.map[deint].svalue);
        }
        else
            filter_str = ghb_settings_get_string(js, "PictureDeinterlaceCustom");
        filter = hb_filter_init(HB_FILTER_DEINTERLACE);
        hb_add_filter( job, filter, filter_str );
        g_free(filter_str);
    }
    gint denoise = ghb_settings_combo_int(js, "PictureDenoise");
    if( denoise )
    {
        filter_str = NULL;
        if (denoise != 1)
        {
            if (denoise_opts.map[denoise].svalue != NULL)
                filter_str = g_strdup(denoise_opts.map[denoise].svalue);
        }
        else
            filter_str = ghb_settings_get_string(js, "PictureDenoiseCustom");
        filter = hb_filter_init(HB_FILTER_DENOISE);
        hb_add_filter( job, filter, filter_str );
        g_free(filter_str);
    }
    gint deblock = ghb_settings_get_int(js, "PictureDeblock");
    if( deblock >= 5 )
    {
        filter_str = NULL;
        filter_str = g_strdup_printf("%d", deblock);
        filter = hb_filter_init(HB_FILTER_DEBLOCK);
        hb_add_filter( job, filter, filter_str );
        g_free(filter_str);
    }

    job->vcodec = ghb_settings_video_encoder_codec(js, "VideoEncoder");
    if ((job->mux & HB_MUX_MASK_MP4 ) && (job->vcodec == HB_VCODEC_THEORA))
    {
        // mp4/theora combination is not supported.
        job->vcodec = HB_VCODEC_FFMPEG_MPEG4;
    }
    if ((job->vcodec == HB_VCODEC_X264) && (job->mux & HB_MUX_MASK_MP4))
    {
        job->ipod_atom = ghb_settings_get_boolean(js, "Mp4iPodCompatible");
    }
    if (ghb_settings_get_boolean(js, "vquality_type_constant"))
    {
        gdouble vquality;
        vquality = ghb_settings_get_double(js, "VideoQualitySlider");
        job->vquality =  vquality;
        job->vbitrate = 0;
    }
    else if (ghb_settings_get_boolean(js, "vquality_type_bitrate"))
    {
        job->vquality = -1.0;
        job->vbitrate = ghb_settings_get_int(js, "VideoAvgBitrate");
    }

    gint vrate;
    gint vrate_base = ghb_settings_video_framerate_rate(js, "VideoFramerate");
    gint cfr;
    if (ghb_settings_get_boolean(js, "VideoFrameratePFR"))
        cfr = 2;
    else if (ghb_settings_get_boolean(js, "VideoFramerateCFR"))
        cfr = 1;
    else
        cfr = 0;

    // x264 zero latency requires CFR encode
    if (ghb_settings_get_boolean(js, "x264ZeroLatency"))
    {
        cfr = 1;
        ghb_log("zerolatency x264 tune selected, forcing constant framerate");
    }

    if( vrate_base == 0 )
    {
        vrate = title->rate;
        vrate_base = title->rate_base;
    }
    else
    {
        vrate = 27000000;
    }
    filter_str = g_strdup_printf("%d:%d:%d", cfr, vrate, vrate_base);
    filter = hb_filter_init(HB_FILTER_VFR);
    hb_add_filter( job, filter, filter_str );
    g_free(filter_str);

    const GValue *audio_list;
    gint count, ii;
    gint tcount = 0;

    audio_list = ghb_settings_get_value(js, "audio_list");
    count = ghb_array_len(audio_list);
    for (ii = 0; ii < count; ii++)
    {
        GValue *asettings;
        hb_audio_config_t audio;
        hb_audio_config_t *aconfig;
        gint acodec, fallback;

        hb_audio_config_init(&audio);
        asettings = ghb_array_get_nth(audio_list, ii);
        audio.in.track = ghb_settings_get_int(asettings, "AudioTrack");
        audio.out.track = tcount;

        char * aname = ghb_settings_get_string(asettings, "AudioTrackName");
        if (aname && *aname)
        {
            // This leaks, but there is no easy way to clean up
            // presently
            audio.out.name = aname;
        }
        else
        {
            g_free(aname);
        }

        aconfig = (hb_audio_config_t *) hb_list_audio_config_item(
                                    title->list_audio, audio.in.track );

        acodec = ghb_settings_audio_encoder_codec(asettings, "AudioEncoder");

        fallback = ghb_settings_audio_encoder_codec(js, "AudioEncoderFallback");
        gint copy_mask = ghb_get_copy_mask(js);
        audio.out.codec = ghb_select_audio_codec(job->mux, aconfig, acodec, fallback, copy_mask);

        audio.out.gain =
            ghb_settings_get_double(asettings, "AudioTrackGainSlider");

        audio.out.dynamic_range_compression =
            ghb_settings_get_double(asettings, "AudioTrackDRCSlider");
        if (audio.out.dynamic_range_compression < 1.0)
            audio.out.dynamic_range_compression = 0.0;

        // It would be better if this were done in libhb for us, but its not yet.
        if (ghb_audio_is_passthru(audio.out.codec))
        {
            audio.out.mixdown = 0;
        }
        else
        {
            audio.out.mixdown = ghb_settings_mixdown_mix(asettings, "AudioMixdown");
            // Make sure the mixdown is valid and pick a new one if not.
            audio.out.mixdown = ghb_get_best_mix(aconfig, audio.out.codec,
                                                    audio.out.mixdown);
            gint srate;
            srate = ghb_settings_audio_samplerate_rate(
                                            asettings, "AudioSamplerate");
            if (srate == 0) // 0 is same as source
                audio.out.samplerate = aconfig->in.samplerate;
            else
                audio.out.samplerate = srate;
            double quality = ghb_settings_get_double(asettings, "AudioTrackQuality");
            if (ghb_settings_get_boolean(asettings, "AudioTrackQualityEnable") &&
                quality != HB_INVALID_AUDIO_QUALITY)
            {
                audio.out.quality = quality;
                audio.out.bitrate = -1;
            }
            else
            {
                audio.out.quality = HB_INVALID_AUDIO_QUALITY;
                audio.out.bitrate =
                    ghb_settings_audio_bitrate_rate(asettings, "AudioBitrate");

                audio.out.bitrate = hb_audio_bitrate_get_best(
                    audio.out.codec, audio.out.bitrate,
                    audio.out.samplerate, audio.out.mixdown);
            }
        }

        // Add it to the jobs audio list
        hb_audio_add( job, &audio );
        tcount++;
    }

    dest_str = ghb_settings_get_string(js, "destination");
    hb_job_set_file( job, dest_str);
    g_free(dest_str);

    const GValue *subtitle_list;
    gint subtitle;
    gboolean force, burned, def, one_burned = FALSE;

    ghb_settings_set_boolean(js, "subtitle_scan", FALSE);
    subtitle_list = ghb_settings_get_value(js, "subtitle_list");
    count = ghb_array_len(subtitle_list);
    for (ii = 0; ii < count; ii++)
    {
        GValue *ssettings;
        gint source;

        ssettings = ghb_array_get_nth(subtitle_list, ii);

        force = ghb_settings_get_boolean(ssettings, "SubtitleForced");
        burned = ghb_settings_get_boolean(ssettings, "SubtitleBurned");
        def = ghb_settings_get_boolean(ssettings, "SubtitleDefaultTrack");
        source = ghb_settings_get_int(ssettings, "SubtitleSource");

        if (source == SRTSUB)
        {
            hb_subtitle_config_t sub_config;
            gchar *filename, *lang, *code;

            filename = ghb_settings_get_string(ssettings, "SrtFile");
            if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR))
            {
                continue;
            }
            sub_config.offset = ghb_settings_get_int(ssettings, "SrtOffset");
            lang = ghb_settings_get_string(ssettings, "SrtLanguage");
            code = ghb_settings_get_string(ssettings, "SrtCodeset");
            strncpy(sub_config.src_filename, filename, 255);
            sub_config.src_filename[255] = 0;
            strncpy(sub_config.src_codeset, code, 39);
            sub_config.src_codeset[39] = 0;
            sub_config.force = 0;
            sub_config.default_track = def;
            if (burned && !one_burned && hb_subtitle_can_burn(SRTSUB))
            {
                // Only allow one subtitle to be burned into the video
                sub_config.dest = RENDERSUB;
                one_burned = TRUE;
            }
            else
            {
                sub_config.dest = PASSTHRUSUB;
            }

            hb_srt_add( job, &sub_config, lang);

            g_free(filename);
            g_free(lang);
            g_free(code);
            continue;
        }

        subtitle = ghb_settings_get_int(ssettings, "SubtitleTrack");
        if (subtitle == -1)
        {
            if (burned && !one_burned)
            {
                // Only allow one subtitle to be burned into the video
                job->select_subtitle_config.dest = RENDERSUB;
                one_burned = TRUE;
            }
            else
            {
                job->select_subtitle_config.dest = PASSTHRUSUB;
            }
            job->select_subtitle_config.force = force;
            job->select_subtitle_config.default_track = def;
            job->indepth_scan = 1;
            ghb_settings_set_boolean(js, "subtitle_scan", TRUE);
        }
        else if (subtitle >= 0)
        {
            hb_subtitle_t * subt;
            hb_subtitle_config_t sub_config;

            subt = hb_list_item(title->list_subtitle, subtitle);
            if (subt != NULL)
            {
                sub_config = subt->config;
                if (burned && !one_burned && hb_subtitle_can_burn(subt->source))
                {
                    // Only allow one subtitle to be burned into the video
                    sub_config.dest = RENDERSUB;
                    one_burned = TRUE;
                }
                else
                {
                    sub_config.dest = PASSTHRUSUB;
                }
                sub_config.force = force;
                sub_config.default_track = def;
                hb_subtitle_add( job, &sub_config, subtitle );
            }
        }
    }
    if (one_burned)
    {
        // Add filter that renders vobsubs
        filter = hb_filter_init(HB_FILTER_RENDER_SUB);
        filter_str = g_strdup_printf("%d:%d:%d:%d",
                                crop[0], crop[1], crop[2], crop[3]);
        hb_add_filter( job, filter, filter_str );
        g_free(filter_str);
    }


    char * meta;

    meta = ghb_settings_get_string(js, "MetaName");
    if (meta && *meta)
    {
        hb_metadata_set_name(job->metadata, meta);
    }
    free(meta);
    meta = ghb_settings_get_string(js, "MetaArtist");
    if (meta && *meta)
    {
        hb_metadata_set_artist(job->metadata, meta);
    }
    free(meta);
    meta = ghb_settings_get_string(js, "MetaAlbumArtist");
    if (meta && *meta)
    {
        hb_metadata_set_album_artist(job->metadata, meta);
    }
    free(meta);
    meta = ghb_settings_get_string(js, "MetaReleaseDate");
    if (meta && *meta)
    {
        hb_metadata_set_release_date(job->metadata, meta);
    }
    free(meta);
    meta = ghb_settings_get_string(js, "MetaComment");
    if (meta && *meta)
    {
        hb_metadata_set_comment(job->metadata, meta);
    }
    free(meta);
    meta = ghb_settings_get_string(js, "MetaGenre");
    if (meta && *meta)
    {
        hb_metadata_set_genre(job->metadata, meta);
    }
    free(meta);
    meta = ghb_settings_get_string(js, "MetaDescription");
    if (meta && *meta)
    {
        hb_metadata_set_description(job->metadata, meta);
    }
    free(meta);
    meta = ghb_settings_get_string(js, "MetaLongDescription");
    if (meta && *meta)
    {
        hb_metadata_set_long_description(job->metadata, meta);
    }
    free(meta);

    if (job->indepth_scan == 1)
    {
        // Subtitle scan. Look for subtitle matching audio language

        /*
         * When subtitle scan is enabled do a fast pre-scan job
         * which will determine which subtitles to enable, if any.
         */
        job->pass = -1;
        job->indepth_scan = 1;
        hb_job_set_encoder_options(job, NULL);

        /*
         * Add the pre-scan job
         */
        job->sequence_id = (unique_id & 0xFFFFFF) | (sub_id++ << 24);
        hb_add( h, job );
    }

    if( ghb_settings_get_boolean(js, "VideoTwoPass") &&
        !ghb_settings_get_boolean(js, "vquality_type_constant"))
    {
        /*
         * If subtitle_scan is enabled then only turn it on
         * for the second pass and then off again for the
         * second.
         */
        job->pass = 1;
        job->indepth_scan = 0;
        ghb_set_video_encoder_opts(job, js);

        /*
         * If turbo options have been selected then set job->fastfirstpass
         */
        if( ghb_settings_get_boolean(js, "VideoTurboTwoPass") &&
            job->vcodec == HB_VCODEC_X264 )
        {
            job->fastfirstpass = 1;
        }
        else
        {
            job->fastfirstpass = 0;
        }

        job->sequence_id = (unique_id & 0xFFFFFF) | (sub_id++ << 24);
        hb_add( h, job );

        job->pass = 2;
        /*
         * On the second pass we turn off subtitle scan so that we
         * can actually encode using any subtitles that were auto
         * selected in the first pass (using the whacky select-subtitle
         * attribute of the job).
         */
        job->indepth_scan = 0;
        job->sequence_id = (unique_id & 0xFFFFFF) | (sub_id++ << 24);
        hb_add( h, job );
    }
    else
    {
        ghb_set_video_encoder_opts(job, js);
        job->indepth_scan = 0;
        job->pass = 0;
        job->sequence_id = (unique_id & 0xFFFFFF) | (sub_id++ << 24);
        hb_add( h, job );
    }

    hb_job_close(&job);
}

void
ghb_add_job(GValue *js, gint unique_id)
{
    // Since I'm doing a scan of the single title I want just prior
    // to adding the job, there is only the one title to choose from.
    add_job(h_queue, js, unique_id, 0);
}

void
ghb_add_live_job(GValue *js, gint unique_id)
{
    int title_id, titleindex;
    const hb_title_t *title;

    title_id = ghb_settings_get_int(js, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    (void)title; // Silence "unused variable" warning
    add_job(h_scan, js, unique_id, titleindex);
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
        if ((job->sequence_id & 0xFFFFFF) == unique_id)
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
    hb_start( h_scan );
}

void
ghb_stop_live_encode()
{
    hb_stop( h_scan );
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
    hb_job_t *job;

    if( title == NULL ) return NULL;

    job = hb_job_init( (hb_title_t*)title );
    if (job == NULL) return NULL;

    set_preview_job_settings(ud, job, ud->settings);

    // hb_get_preview doesn't compensate for anamorphic, so lets
    // calculate scale factors
    gint width, height, par_width = 1, par_height = 1;
    gint pic_par = ghb_settings_combo_int(ud->settings, "PicturePAR");
    if (pic_par)
    {
        hb_set_anamorphic_size( job, &width, &height,
                                &par_width, &par_height );
    }

    // Make sure we have a big enough buffer to receive the image from libhb
    gint dstWidth = job->width;
    gint dstHeight= job->height;

    static guint8 *buffer = NULL;
    static gint bufferSize = 0;
    gint newSize;

    newSize = dstWidth * dstHeight * 4;
    if( bufferSize < newSize )
    {
        bufferSize = newSize;
        buffer     = (guint8*) g_realloc( buffer, bufferSize );
    }
    hb_get_preview( h_scan, job, index, buffer );
    hb_job_close( &job );

    // Create an GdkPixbuf and copy the libhb image into it, converting it from
    // libhb's format something suitable.

    // The image data returned by hb_get_preview is 4 bytes per pixel,
    // BGRA format. Alpha is ignored.

    GdkPixbuf *preview = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, dstWidth, dstHeight);
    guint8 *pixels = gdk_pixbuf_get_pixels (preview);

    guint32 *src = (guint32*)buffer;
    guint8 *dst = pixels;

    gint ii, jj;
    gint channels = gdk_pixbuf_get_n_channels (preview);
    gint stride = gdk_pixbuf_get_rowstride (preview);
    guint8 *tmp;

    for (ii = 0; ii < dstHeight; ii++)
    {
        tmp = dst;
        for (jj = 0; jj < dstWidth; jj++)
        {
            tmp[0] = src[0] >> 16;
            tmp[1] = src[0] >> 8;
            tmp[2] = src[0] >> 0;
            tmp += channels;
            src++;
        }
        dst += stride;
    }
    gint w = ghb_settings_get_int(ud->settings, "scale_width");
    gint h = ghb_settings_get_int(ud->settings, "scale_height");
    ghb_par_scale(ud, &w, &h, par_width, par_height);

    gint c0, c1, c2, c3;
    c0 = ghb_settings_get_int(ud->settings, "PictureTopCrop");
    c1 = ghb_settings_get_int(ud->settings, "PictureBottomCrop");
    c2 = ghb_settings_get_int(ud->settings, "PictureLeftCrop");
    c3 = ghb_settings_get_int(ud->settings, "PictureRightCrop");

    gdouble xscale = (gdouble)w / (gdouble)(title->width - c2 - c3);
    gdouble yscale = (gdouble)h / (gdouble)(title->height - c0 - c1);

    ghb_par_scale(ud, &dstWidth, &dstHeight, par_width, par_height);
    *out_width = w;
    *out_height = h;
    if (ghb_settings_get_boolean(ud->prefs, "reduce_hd_preview"))
    {
        GdkScreen *ss;
        gint s_w, s_h;
        gint orig_w, orig_h;
        gint factor = 80;

        if (ghb_settings_get_boolean(ud->prefs, "preview_fullscreen"))
        {
            factor = 100;
        }
        ss = gdk_screen_get_default();
        s_w = gdk_screen_get_width(ss);
        s_h = gdk_screen_get_height(ss);
        orig_w = dstWidth;
        orig_h = dstHeight;

        if (dstWidth > s_w * factor / 100)
        {
            dstWidth = s_w * factor / 100;
            dstHeight = dstHeight * dstWidth / orig_w;
        }
        if (dstHeight > s_h * factor / 100)
        {
            dstHeight = s_h * factor / 100;
            dstWidth = orig_w * dstHeight / orig_h;
        }
        xscale *= (gdouble)dstWidth / orig_w;
        yscale *= (gdouble)dstHeight / orig_h;
        w *= (gdouble)dstWidth / orig_w;
        h *= (gdouble)dstHeight / orig_h;
    }
    GdkPixbuf *scaled_preview;
    scaled_preview = gdk_pixbuf_scale_simple(preview, dstWidth, dstHeight, GDK_INTERP_HYPER);
    if (ghb_settings_get_boolean(ud->prefs, "preview_show_crop"))
    {
        c0 *= yscale;
        c1 *= yscale;
        c2 *= xscale;
        c3 *= xscale;
        // Top
        hash_pixbuf(scaled_preview, c2, 0, w, c0, 32, 0);
        // Bottom
        hash_pixbuf(scaled_preview, c2, dstHeight-c1, w, c1, 32, 0);
        // Left
        hash_pixbuf(scaled_preview, 0, c0, c2, h, 32, 1);
        // Right
        hash_pixbuf(scaled_preview, dstWidth-c3, c0, c3, h, 32, 1);
    }
    g_object_unref (preview);
    return scaled_preview;
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
