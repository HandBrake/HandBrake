/***************************************************************************
 *            hb-backend.c
 *
 *  Fri Mar 28 10:38:44 2008
 *  Copyright  2008  John Stebbins
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
#include "hbversion.h"
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include "hb-backend.h"
#include "settings.h"
#include "callbacks.h"
#include "values.h"

typedef struct
{
	const gchar *option;
	const gchar *shortOpt;
	gint ivalue;
	const gchar *svalue;
} options_map_t;

typedef struct
{
	gint count;
	options_map_t *map;
} combo_opts_t;

static const gchar *index_str[] =
{
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"10",
};

static options_map_t d_container_opts[] =
{
	{"MKV", "mkv", HB_MUX_MKV, "mkv"},
	{"MP4", "mp4", HB_MUX_MP4, "mp4"},
	{"M4V", "m4v", HB_MUX_MP4, "m4v"},
	{"AVI", "avi", HB_MUX_AVI, "avi"},
	{"OGM", "ogm", HB_MUX_OGM, "ogm"},
};
combo_opts_t container_opts =
{
	sizeof(d_container_opts)/sizeof(options_map_t),
	d_container_opts
};

static options_map_t d_deint_opts[] =
{
	{"None",   "none",   0, ""},
	{"Fast",   "fast",   1, "-1:-1:-1:0:1"},
	{"Slow",   "slow",   2, "2:-1:-1:0:1"},
	{"Slower", "slower", 3, "0:-1:-1:0:1"},
};
combo_opts_t deint_opts =
{
	sizeof(d_deint_opts)/sizeof(options_map_t),
	d_deint_opts
};

static options_map_t d_denoise_opts[] =
{
	{"None",   "none",   0, ""},
	{"Weak",   "weak",   1, "2:1:2:3"},
	{"Medium", "medium", 2, "3:2:2:3"},
	{"Strong", "strong", 3, "7:7:5:5"},
};
combo_opts_t denoise_opts =
{
	sizeof(d_denoise_opts)/sizeof(options_map_t),
	d_denoise_opts
};

static options_map_t d_vcodec_opts[] =
{
	{"H.264 (x264)",    "x264",   HB_VCODEC_X264, ""},
	{"MPEG-4 (XviD)",   "xvid",   HB_VCODEC_XVID, ""},
	{"MPEG-4 (FFMPEG)", "ffmpeg", HB_VCODEC_FFMPEG, ""},
	{"Theora",          "theora", HB_VCODEC_THEORA, ""},
};
combo_opts_t vcodec_opts =
{
	sizeof(d_vcodec_opts)/sizeof(options_map_t),
	d_vcodec_opts
};

static options_map_t d_acodec_opts[] =
{
	{"AAC (faac)",      "faac",   HB_ACODEC_FAAC, "faac"},
	{"MP3 (lame)",      "lame",   HB_ACODEC_LAME, "lame"},
	{"Vorbis",          "vorbis", HB_ACODEC_VORBIS, "vorbis"},
	{"AC3 (pass-thru)", "ac3",    HB_ACODEC_AC3, "ac3"},
};
combo_opts_t acodec_opts =
{
	sizeof(d_acodec_opts)/sizeof(options_map_t),
	d_acodec_opts
};

static options_map_t d_direct_opts[] =
{
	{"None",      "none",     0, "none"},
	{"Spatial",   "spatial",  1, "spatial"},
	{"Temporal",  "temporal", 2, "temporal"},
	{"Automatic", "auto",     3, "auto"},
};
combo_opts_t direct_opts =
{
	sizeof(d_direct_opts)/sizeof(options_map_t),
	d_direct_opts
};

static options_map_t d_me_opts[] =
{
	{"Diamond",              "dia", 0, "dia"},
	{"Hexagon",              "hex", 1, "hex"},
	{"Uneven Multi-Hexagon", "umh", 2, "umh"},
	{"Exhaustive",           "esa", 3, "esa"},
};
combo_opts_t me_opts =
{
	sizeof(d_me_opts)/sizeof(options_map_t),
	d_me_opts
};

static options_map_t d_subme_opts[] =
{
	{"1", "1", 1, "1"},
	{"2", "2", 2, "2"},
	{"3", "3", 3, "3"},
	{"4", "4", 4, "4"},
	{"5", "5", 5, "5"},
	{"6", "6", 6, "6"},
	{"7", "7", 7, "7"},
	{"8", "8", 8, "8"},
	{"9", "9", 9, "9"},
};
combo_opts_t subme_opts =
{
	sizeof(d_subme_opts)/sizeof(options_map_t),
	d_subme_opts
};

static options_map_t d_analyse_opts[] =
{
	{"Some", "some", 0, "some"},
	{"None", "none", 1, "none"},
	{"All",  "all",  2, "all"},
	{"Custom",  "custom",  3, "all"},
};
combo_opts_t analyse_opts =
{
	sizeof(d_analyse_opts)/sizeof(options_map_t),
	d_analyse_opts
};

static options_map_t d_trellis_opts[] =
{
	{"Disabled",          "0",    0, "0"},
	{"Final Macro Block", "1",    1, "1"},
	{"Always",            "2", 2, "2"},
};
combo_opts_t trellis_opts =
{
	sizeof(d_trellis_opts)/sizeof(options_map_t),
	d_trellis_opts
};

combo_opts_t subtitle_opts =
{
	0,
	NULL
};

combo_opts_t title_opts =
{
	0,
	NULL
};

combo_opts_t audio_track_opts =
{
	0,
	NULL
};

typedef struct
{
	const gchar *name;
	combo_opts_t *opts;
} combo_name_map_t;

combo_name_map_t combo_name_map[] =
{
	{"container", &container_opts},
	{"deinterlace", &deint_opts},
	{"tweak_deinterlace", &deint_opts},
	{"denoise", &denoise_opts},
	{"tweak_denoise", &denoise_opts},
	{"video_codec", &vcodec_opts},
	{"audio_codec", &acodec_opts},
	{"x264_direct", &direct_opts},
	{"x264_me", &me_opts},
	{"x264_subme", &subme_opts},
	{"x264_analyse", &analyse_opts},
	{"x264_trellis", &trellis_opts},
	{"subtitle_lang", &subtitle_opts},
	{"title", &title_opts},
	{"audio_track", &audio_track_opts},
	{NULL, NULL}
};

typedef struct iso639_lang_t
{
    char * eng_name;        /* Description in English */
    char * native_name;     /* Description in native language */
    char * iso639_1;       /* ISO-639-1 (2 characters) code */
    char * iso639_2;        /* ISO-639-2/t (3 character) code */
    char * iso639_2b;       /* ISO-639-2/b code (if different from above) */
} iso639_lang_t;

static const iso639_lang_t language_table[] =
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
	{ "Divehi", "", "dv", "div" },
	{ "Dutch", "Nederlands", "nl", "nld", "dut" },
	{ "Dzongkha", "", "dz", "dzo" },
	{ "English", "English", "en", "eng" },
	{ "Esperanto", "", "eo", "epo" },
	{ "Estonian", "", "et", "est" },
	{ "Ewe", "", "ee", "ewe" },
	{ "Faroese", "", "fo", "fao" },
	{ "Fijian", "", "fj", "fij" },
	{ "Finnish", "Suomi", "fi", "fin" },
	{ "French", "Francais", "fr", "fra", "fre" },
	{ "Western Frisian", "", "fy", "fry" },
	{ "Fulah", "", "ff", "ful" },
	{ "Georgian", "", "ka", "kat", "geo" },
	{ "German", "Deutsch", "de", "deu", "ger" },
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
	{ "Hungarian", "Magyar", "hu", "hun" },
	{ "Igbo", "", "ig", "ibo" },
	{ "Icelandic", "Islenska", "is", "isl", "ice" },
	{ "Ido", "", "io", "ido" },
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
	{ "Ndebele, South", "", "nr", "nbl" },
	{ "Ndebele, North", "", "nd", "nde" },
	{ "Ndonga", "", "ng", "ndo" },
	{ "Nepali", "", "ne", "nep" },
	{ "Norwegian Nynorsk", "", "nn", "nno" },
	{ "Norwegian Bokmål", "", "nb", "nob" },
	{ "Norwegian", "Norsk", "no", "nor" },
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
	{ "Croatian", "Hrvatski", "hr", "hrv", "scr" },
	{ "Sinhala", "", "si", "sin" },
	{ "Slovak", "", "sk", "slk", "slo" },
	{ "Slovenian", "", "sl", "slv" },
	{ "Northern Sami", "", "se", "sme" },
	{ "Samoan", "", "sm", "smo" },
	{ "Shona", "", "sn", "sna" },
	{ "Sindhi", "", "sd", "snd" },
	{ "Somali", "", "so", "som" },
	{ "Sotho, Southern", "", "st", "sot" },
	{ "Spanish", "Espanol", "es", "spa" },
	{ "Sardinian", "", "sc", "srd" },
	{ "Swati", "", "ss", "ssw" },
	{ "Sundanese", "", "su", "sun" },
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
};
#define	LANG_TABLE_SIZE (sizeof(language_table)/ sizeof(iso639_lang_t))

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
	return HB_VERSION;
}

void
ghb_vquality_range(signal_user_data_t *ud, gint *min, gint *max)
{
	if (ghb_settings_get_boolean(ud->settings, "directqp"))
	{
		gint vcodec = ghb_settings_combo_int(ud->settings, "video_codec");
		// Only x264 and ffmpeg currently support direct qp/crf entry
		if (vcodec == HB_VCODEC_X264)
		{
			*min = 0;
			*max = 51;
		}
		else if (vcodec == HB_VCODEC_FFMPEG)
		{
			*min = 0;
			*max = 31;
		}
		else
		{
			*min = 0;
			*max = 100;
		}
	}
	else
	{
		*min = 0;
		*max = 100;
	}
}

static gint
lookup_generic_int(combo_opts_t *opts, const GValue *gval)
{
	gint ii;
	gchar *str;
	gint result = -1;

	str = ghb_value_string(gval);
	for (ii = 0; ii < opts->count; ii++)
	{
		if (strcmp(opts->map[ii].shortOpt, str) == 0)
		{
			result = opts->map[ii].ivalue;
			break;
		}
	}
	g_free(str);
	return result;
}

static const gchar*
lookup_generic_option(combo_opts_t *opts, const GValue *gval)
{
	gint ii;
	gchar *str;
	const gchar *result = "";

	str = ghb_value_string(gval);
	for (ii = 0; ii < opts->count; ii++)
	{
		if (strcmp(opts->map[ii].shortOpt, str) == 0)
		{
			result = opts->map[ii].option;
			break;
		}
	}
	g_free(str);
	return result;
}

static gint
lookup_mix_int(const GValue *mix)
{
	gint ii;
	gchar *str;
	gint result = 0;


	str = ghb_value_string(mix);
	for (ii = 0; ii < hb_audio_mixdowns_count; ii++)
	{
		if (strcmp(hb_audio_mixdowns[ii].short_name, str) == 0)
		{
			result = hb_audio_mixdowns[ii].amixdown;
			break;
		}
	}
	g_free(str);
	return result;
}

static const gchar*
lookup_mix_option(const GValue *mix)
{
	gint ii;
	gchar *str;
	gchar *result = "None";


	str = ghb_value_string(mix);
	for (ii = 0; ii < hb_audio_mixdowns_count; ii++)
	{
		if (strcmp(hb_audio_mixdowns[ii].short_name, str) == 0)
		{
			result = hb_audio_mixdowns[ii].human_readable_name;
			break;
		}
	}
	g_free(str);
	return result;
}

static gint
lookup_video_rate_int(const GValue *vrate)
{
	gint ii;
	gchar *str;
	gint result = 0;

	str = ghb_value_string(vrate);
	for (ii = 0; ii < hb_video_rates_count; ii++)
	{
		if (strcmp(hb_video_rates[ii].string, str) == 0)
		{
			result = hb_video_rates[ii].rate;
			break;
		}
	}
	g_free(str);
	// Default to "same as source"
	return result;
}

static const gchar*
lookup_video_rate_option(const GValue *vrate)
{
	gint ii;
	gchar *str;
	const gchar *result = "Same as source";

	str = ghb_value_string(vrate);
	for (ii = 0; ii < hb_video_rates_count; ii++)
	{
		if (strcmp(hb_video_rates[ii].string, str) == 0)
		{
			result = hb_video_rates[ii].string;
			break;
		}
	}
	g_free(str);
	// Default to "same as source"
	return result;
}

static gint
lookup_audio_rate_int(const GValue *rate)
{
	gint ii;
	gchar *str;
	gint result = 0;

	// Coincidentally, the string "source" will return 0
	// which is our flag to use "same as source"
	str = ghb_value_string(rate);
	for (ii = 0; ii < hb_audio_rates_count; ii++)
	{
		if (strcmp(hb_audio_rates[ii].string, str) == 0)
		{
			result = hb_audio_rates[ii].rate;
			break;
		}
	}
	g_free(str);
	return result;
}

static const gchar*
lookup_audio_rate_option(const GValue *rate)
{
	gint ii;
	gchar *str;
	const gchar *result = "Same as source";

	// Coincidentally, the string "source" will return 0
	// which is our flag to use "same as source"
	str = ghb_value_string(rate);
	for (ii = 0; ii < hb_audio_rates_count; ii++)
	{
		if (strcmp(hb_audio_rates[ii].string, str) == 0)
		{
			result = hb_audio_rates[ii].string;
			break;
		}
	}
	g_free(str);
	return result;
}

static gint
lookup_audio_bitrate_int(const GValue *rate)
{
	gint ii;
	gchar *str;
	gint result = 0;

	// Coincidentally, the string "source" will return 0
	// which is our flag to use "same as source"
	str = ghb_value_string(rate);
	for (ii = 0; ii < hb_audio_bitrates_count; ii++)
	{
		if (strcmp(hb_audio_bitrates[ii].string, str) == 0)
		{
			result = hb_audio_bitrates[ii].rate;
			break;
		}
	}
	g_free(str);
	return result;
}

static const gchar*
lookup_audio_bitrate_option(const GValue *rate)
{
	gint ii;
	gchar *str;
	const gchar *result = "Same as source";

	// Coincidentally, the string "source" will return 0
	// which is our flag to use "same as source"
	str = ghb_value_string(rate);
	for (ii = 0; ii < hb_audio_bitrates_count; ii++)
	{
		if (strcmp(hb_audio_bitrates[ii].string, str) == 0)
		{
			result = hb_audio_bitrates[ii].string;
			break;
		}
	}
	g_free(str);
	return result;
}

static gint
lookup_audio_lang_int(const GValue *rate)
{
	gint ii;
	gchar *str;
	gint result = 0;

	// Coincidentally, the string "source" will return 0
	// which is our flag to use "same as source"
	str = ghb_value_string(rate);
	for (ii = 0; ii < LANG_TABLE_SIZE; ii++)
	{
		if (strcmp(language_table[ii].iso639_2, str) == 0)
		{
			result = ii;
			break;
		}
	}
	g_free(str);
	return result;
}

static const gchar*
lookup_audio_lang_option(const GValue *rate)
{
	gint ii;
	gchar *str;
	const gchar *result = "Same as source";

	// Coincidentally, the string "source" will return 0
	// which is our flag to use "same as source"
	str = ghb_value_string(rate);
	for (ii = 0; ii < LANG_TABLE_SIZE; ii++)
	{
		if (strcmp(language_table[ii].iso639_2, str) == 0)
		{
			result = language_table[ii].eng_name;
			break;
		}
	}
	g_free(str);
	return result;
}

static GValue*
get_acodec_value(gint val)
{
	GValue *value = NULL;
	gint ii;

	for (ii = 0; ii < acodec_opts.count; ii++)
	{
		if (acodec_opts.map[ii].ivalue == val)
		{
			value = ghb_string_value_new(acodec_opts.map[ii].shortOpt);
			break;
		}
	}
	return value;
}

static GValue*
get_abitrate_value(gint val)
{
	GValue *value = NULL;
	gint ii;

	for (ii = 0; ii < hb_audio_bitrates_count; ii++)
	{
		if (hb_audio_bitrates[ii].rate == val)
		{
			value = ghb_string_value_new(hb_audio_bitrates[ii].string);
			break;
		}
	}
	return value;
}

static GValue*
get_amix_value(gint val)
{
	GValue *value = NULL;
	gint ii;

	for (ii = 0; ii < hb_audio_mixdowns_count; ii++)
	{
		if (hb_audio_mixdowns[ii].amixdown == val)
		{
			value = ghb_string_value_new(hb_audio_mixdowns[ii].short_name);
			break;
		}
	}
	return value;
}

// Handle for libhb.  Gets set by ghb_backend_init()
static hb_handle_t * h_scan = NULL;
static hb_handle_t * h_queue = NULL;

extern void hb_get_tempory_directory(hb_handle_t *h, char path[512]);

void
ghb_hb_cleanup(gboolean partial)
{
	char dir[512];

	hb_get_tempory_directory(h_scan, dir);
	del_tree(dir, !partial);
}

gint
ghb_get_title_number(gint titleindex)
{
	hb_list_t  * list;
	hb_title_t * title;
	
	if (h_scan == NULL) return 1;
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		return 1;
	}
	title = hb_list_item( list, titleindex );
	if (title == NULL) return 1;	// Bad titleindex
	return title->index;
}

static hb_audio_config_t*
get_hb_audio(gint titleindex, gint track)
{
	hb_list_t  * list;
	hb_title_t * title;
    hb_audio_config_t *audio = NULL;
	
    if (h_scan == NULL) return NULL;
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		return NULL;
	}
    title = hb_list_item( list, titleindex );
	if (title == NULL) return NULL;	// Bad titleindex
	if (!hb_list_count(title->list_audio))
	{
		return NULL;
	}
    audio = (hb_audio_config_t *)hb_list_audio_config_item(title->list_audio, track);
	return audio;
}

static gint
search_rates(hb_rate_t *rates, gint rate, gint count)
{
	gint ii;
	for (ii = 0; ii < count; ii++)
	{
		if (rates[ii].rate == rate)
			return ii;
	}
	return -1;
}

static gboolean find_combo_item_by_int(GtkTreeModel *store, gint value, GtkTreeIter *iter);

static GtkListStore*
get_combo_box_store(GtkBuilder *builder, const gchar *name)
{
	GtkComboBox *combo;
	GtkListStore *store;

	g_debug("get_combo_box_store() %s\n", name);
	// First modify the combobox model to allow greying out of options
	combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
	store = GTK_LIST_STORE(gtk_combo_box_get_model (combo));
	return store;
}

static void
grey_combo_box_item(GtkBuilder *builder, const gchar *name, gint value, gboolean grey)
{
	GtkListStore *store;
	GtkTreeIter iter;
	
	store = get_combo_box_store(builder, name);
	if (find_combo_item_by_int(GTK_TREE_MODEL(store), value, &iter))
	{
		gtk_list_store_set(store, &iter, 
						   1, !grey, 
						   -1);
	}
}

void
ghb_grey_combo_options(GtkBuilder *builder)
{
	GtkWidget *widget;
	gint container, track, titleindex, acodec;
	gboolean httpopt;
    hb_audio_config_t *audio = NULL;
	
	widget = GHB_WIDGET (builder, "title");
	titleindex = ghb_lookup_combo_int("title", ghb_widget_value(widget));
	widget = GHB_WIDGET (builder, "audio_track");
	track = ghb_lookup_combo_int("audio_track", ghb_widget_value(widget));
	audio = get_hb_audio(titleindex, track);
	widget = GHB_WIDGET (builder, "container");
	container = ghb_lookup_combo_int("container", ghb_widget_value(widget));
	widget = GHB_WIDGET (builder, "http_optimize_mp4");
	httpopt = ghb_widget_boolean(widget);

	grey_combo_box_item(builder, "audio_codec", HB_ACODEC_FAAC, FALSE);
	grey_combo_box_item(builder, "audio_codec", HB_ACODEC_LAME, FALSE);
	grey_combo_box_item(builder, "audio_codec", HB_ACODEC_VORBIS, FALSE);

	gboolean allow_ac3 = TRUE;
	allow_ac3 = 
		!(container == HB_MUX_MP4 && httpopt) &&
		(container != HB_MUX_OGM);

	if (allow_ac3)
	{
		grey_combo_box_item(builder, "audio_codec", HB_ACODEC_AC3, FALSE);
	}
	else
	{
		grey_combo_box_item(builder, "audio_codec", HB_ACODEC_AC3, TRUE);
	}
	if (audio && audio->in.codec != HB_ACODEC_AC3)
	{
		grey_combo_box_item(builder, "audio_codec", HB_ACODEC_AC3, TRUE);
	}
	grey_combo_box_item(builder, "video_codec", HB_VCODEC_THEORA, FALSE);

	widget = GHB_WIDGET (builder, "audio_codec");
	acodec = ghb_lookup_combo_int("audio_codec", ghb_widget_value(widget));
	if (acodec != HB_ACODEC_AC3)
	{
		grey_combo_box_item(builder, "audio_mix", 0, TRUE);
	}
	if (container == HB_MUX_MP4)
	{
		grey_combo_box_item(builder, "audio_codec", HB_ACODEC_LAME, TRUE);
		grey_combo_box_item(builder, "audio_codec", HB_ACODEC_VORBIS, TRUE);
		grey_combo_box_item(builder, "video_codec", HB_VCODEC_THEORA, TRUE);
	}
	else if (container == HB_MUX_AVI)
	{
		grey_combo_box_item(builder, "audio_codec", HB_ACODEC_FAAC, TRUE);
		grey_combo_box_item(builder, "audio_codec", HB_ACODEC_VORBIS, TRUE);
		grey_combo_box_item(builder, "video_codec", HB_VCODEC_THEORA, TRUE);
	}
	else if (container == HB_MUX_OGM)
	{
		grey_combo_box_item(builder, "audio_codec", HB_ACODEC_FAAC, TRUE);
	}

	gboolean allow_mono = TRUE;
	gboolean allow_stereo = TRUE;
	gboolean allow_dolby = TRUE;
	gboolean allow_dpl2 = TRUE;
	gboolean allow_6ch = TRUE;
	if (audio)
	{
		allow_mono =
			(audio->in.codec & (HB_ACODEC_AC3|HB_ACODEC_DCA)) &&
			(acodec != HB_ACODEC_LAME);
		gint layout = audio->in.channel_layout & HB_INPUT_CH_LAYOUT_DISCRETE_NO_LFE_MASK;
		allow_stereo =
			((layout == HB_INPUT_CH_LAYOUT_MONO && !allow_mono) || layout >= HB_INPUT_CH_LAYOUT_STEREO);
		allow_dolby =
			(layout == HB_INPUT_CH_LAYOUT_3F1R) || 
			(layout == HB_INPUT_CH_LAYOUT_3F2R) || 
			(layout == HB_INPUT_CH_LAYOUT_DOLBY);
		allow_dpl2 = (layout == HB_INPUT_CH_LAYOUT_3F2R);
		allow_6ch =
			(audio->in.codec & (HB_ACODEC_AC3|HB_ACODEC_DCA)) &&
			(acodec != HB_ACODEC_LAME) &&
			(layout == HB_INPUT_CH_LAYOUT_3F2R) && 
			(audio->in.channel_layout & HB_INPUT_CH_LAYOUT_HAS_LFE);
	}
	grey_combo_box_item(builder, "audio_mix", HB_AMIXDOWN_MONO, !allow_mono);
	grey_combo_box_item(builder, "audio_mix", HB_AMIXDOWN_STEREO, !allow_stereo);
	grey_combo_box_item(builder, "audio_mix", HB_AMIXDOWN_DOLBY, !allow_dolby);
	grey_combo_box_item(builder, "audio_mix", HB_AMIXDOWN_DOLBYPLII, !allow_dpl2);
	grey_combo_box_item(builder, "audio_mix", HB_AMIXDOWN_6CH, !allow_6ch);
}

gint
ghb_get_best_mix(gint titleindex, gint track, gint acodec, gint mix)
{
    hb_audio_config_t *audio = NULL;
	gboolean allow_mono = TRUE;
	gboolean allow_stereo = TRUE;
	gboolean allow_dolby = TRUE;
	gboolean allow_dpl2 = TRUE;
	gboolean allow_6ch = TRUE;
	
	if (acodec & (HB_ACODEC_AC3 | HB_ACODEC_DCA))
	{
		// Audio codec pass-thru.  No mixdown
		return 0;
	}
	audio = get_hb_audio(titleindex, track);
	if (audio)
	{
		allow_mono =
			(audio->in.codec & (HB_ACODEC_AC3|HB_ACODEC_DCA)) &&
			(acodec != HB_ACODEC_LAME);
		gint layout = audio->in.channel_layout & HB_INPUT_CH_LAYOUT_DISCRETE_NO_LFE_MASK;
		allow_stereo =
			((layout == HB_INPUT_CH_LAYOUT_MONO && !allow_mono) || layout >= HB_INPUT_CH_LAYOUT_STEREO);
		allow_dolby =
			(layout == HB_INPUT_CH_LAYOUT_3F1R) || 
			(layout == HB_INPUT_CH_LAYOUT_3F2R) || 
			(layout == HB_INPUT_CH_LAYOUT_DOLBY);
		allow_dpl2 = (layout == HB_INPUT_CH_LAYOUT_3F2R);
		allow_6ch =
			(audio->in.codec & (HB_ACODEC_AC3|HB_ACODEC_DCA)) &&
			(acodec != HB_ACODEC_LAME) &&
			(layout == HB_INPUT_CH_LAYOUT_3F2R) && 
			(audio->in.channel_layout & HB_INPUT_CH_LAYOUT_HAS_LFE);
	}
	gboolean greater = FALSE;
	if (mix == 0) 
	{
		// If no mix is specified, select the best available.
		mix = HB_AMIXDOWN_6CH;
	}
	if (mix == HB_AMIXDOWN_6CH)
	{
		greater = TRUE;
		if (allow_6ch) return HB_AMIXDOWN_6CH;
	}
	if (mix == HB_AMIXDOWN_DOLBYPLII || greater)
	{
		greater = TRUE;
		if (allow_dpl2) return HB_AMIXDOWN_DOLBYPLII;
	}
	if (mix == HB_AMIXDOWN_DOLBY || greater)
	{
		greater = TRUE;
		if (allow_dolby) return HB_AMIXDOWN_DOLBY;
	}
	if (mix == HB_AMIXDOWN_STEREO || greater)
	{
		greater = TRUE;
		if (allow_stereo) return HB_AMIXDOWN_STEREO;
	}
	if (mix == HB_AMIXDOWN_MONO || greater)
	{
		greater = TRUE;
		if (allow_mono) return HB_AMIXDOWN_MONO;
	}
	if (allow_stereo) return HB_AMIXDOWN_STEREO;
	if (allow_dolby) return HB_AMIXDOWN_DOLBY;
	if (allow_dpl2) return HB_AMIXDOWN_DOLBYPLII;
	if (allow_6ch) return HB_AMIXDOWN_6CH;
	return 0;
}

// Set up the model for the combo box
static void
init_combo_box(GtkBuilder *builder, const gchar *name)
{
	GtkComboBox *combo;
	GtkListStore *store;
	GtkCellRenderer *cell;

	g_debug("init_combo_box() %s\n", name);
	// First modify the combobox model to allow greying out of options
	combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
	// Store contains:
	// 1 - String to display
	// 2 - bool indicating whether the entry is selectable (grey or not)
	// 3 - String that is used for presets
	// 4 - Int value determined by backend
	// 5 - String value determined by backend
	store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_BOOLEAN, 
							   G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
	gtk_combo_box_set_model(combo, GTK_TREE_MODEL(store));

	if (GTK_WIDGET_TYPE(combo) == GTK_TYPE_COMBO_BOX)
	{
		gtk_cell_layout_clear(GTK_CELL_LAYOUT(combo));
    	cell = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
    	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cell, TRUE);
    	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cell,
      		"text", 0, "sensitive", 1, NULL);
	}
	else
	{ // Combo box entry
		gtk_combo_box_entry_set_text_column(GTK_COMBO_BOX_ENTRY(combo), 0);
	}
}	

static void
audio_samplerate_opts_set(GtkBuilder *builder, const gchar *name, hb_rate_t *rates, gint count)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ii;
	
	g_debug("audio_samplerate_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	// Add an item for "Same As Source"
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
					   0, "Same as source", 
					   1, TRUE, 
					   2, "source", 
					   3, 0, 
					   4, "source", 
					   -1);
	for (ii = 0; ii < count; ii++)
	{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, rates[ii].string, 
						   1, TRUE, 
						   2, rates[ii].string, 
						   3, rates[ii].rate, 
						   4, rates[ii].string, 
						   -1);
	}
}

static void
video_rate_opts_set(GtkBuilder *builder, const gchar *name, hb_rate_t *rates, gint count)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ii;
	
	g_debug("video_rate_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	// Add an item for "Same As Source"
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
					   0, "Same as source", 
					   1, TRUE, 
					   2, "source", 
					   3, 0, 
					   4, "source", 
					   -1);
	for (ii = 0; ii < count; ii++)
	{
		gchar *desc = "";
		gchar *option;
		if (strcmp(rates[ii].string, "23.976") == 0)
		{
			desc = "(NTSC Film)";
		}
		else if (strcmp(rates[ii].string, "25") == 0)
		{
			desc = "(PAL Film/Video)";
		}
		else if (strcmp(rates[ii].string, "29.97") == 0)
		{
			desc = "(NTSC Video)";
		}
		option = g_strdup_printf ("%s %s", rates[ii].string, desc);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, option, 
						   1, TRUE, 
						   2, rates[ii].string, 
						   3, rates[ii].rate, 
						   4, rates[ii].string, 
						   -1);
		g_free(option);
	}
}

static void
mix_opts_set(GtkBuilder *builder, const gchar *name)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ii;
	
	g_debug("mix_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
					   0, "None", 
					   1, TRUE, 
					   2, "none", 
					   3, 0, 
					   4, "none", 
					   -1);
	for (ii = 0; ii < hb_audio_mixdowns_count; ii++)
	{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, hb_audio_mixdowns[ii].human_readable_name, 
						   1, TRUE, 
						   2, hb_audio_mixdowns[ii].short_name, 
						   3, hb_audio_mixdowns[ii].amixdown, 
						   4, hb_audio_mixdowns[ii].internal_name, 
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
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	for (ii = 0; ii < LANG_TABLE_SIZE; ii++)
	{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, language_table[ii].eng_name, 
						   1, TRUE, 
						   2, language_table[ii].iso639_2, 
						   3, ii, 
						   4, language_table[ii].iso639_1, 
						   -1);
	}
}

static gchar **titles = NULL;

void
title_opts_set(GtkBuilder *builder, const gchar *name)
{
	GtkTreeIter iter;
	GtkListStore *store;
	hb_list_t  * list = NULL;
	hb_title_t * title = NULL;
	gint ii;
	gint count = 0;
	
	g_debug("title_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	if (h_scan != NULL)
	{
		list = hb_get_titles( h_scan );
		count = hb_list_count( list );
		if (count > 100) count = 100;
	}
	if (titles) g_strfreev(titles);
	if (title_opts.map) g_free(title_opts.map);
	if (count > 0)
	{
		title_opts.count = count;
		title_opts.map = g_malloc(count*sizeof(options_map_t));
		titles = g_malloc((count+1) * sizeof(gchar*));
	}
	else
	{
		title_opts.count = 1;
		title_opts.map = g_malloc(sizeof(options_map_t));
		titles = NULL;
	}
	if( count <= 0 )
	{
		// No titles.  Fill in a default.
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, "No Titles", 
						   1, TRUE, 
						   2, "none", 
						   3, -1, 
						   4, "none", 
						   -1);
		title_opts.map[0].option = "No Titles";
		title_opts.map[0].shortOpt = "none";
		title_opts.map[0].ivalue = -1;
		title_opts.map[0].svalue = "none";
		return;
	}
	for (ii = 0; ii < count; ii++)
	{
		title = (hb_title_t*)hb_list_item(list, ii);
		if (title->duration != 0)
		{
			titles[ii]  = g_strdup_printf ("%d - %02dh%02dm%02ds",
				title->index, title->hours, title->minutes, title->seconds);
		}
		else
		{
			titles[ii]  = g_strdup_printf ("%d - Unknown Length", title->index);
		}
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, titles[ii], 
						   1, TRUE, 
						   2, titles[ii], 
						   3, ii, 
						   4, titles[ii], 
						   -1);
		title_opts.map[ii].option = titles[ii];
		title_opts.map[ii].shortOpt = titles[ii];
		title_opts.map[ii].ivalue = ii;
		title_opts.map[ii].svalue = titles[ii];
	}
	titles[ii] = NULL;
}

static gboolean
find_combo_item_by_int(GtkTreeModel *store, gint value, GtkTreeIter *iter)
{
	gint ivalue;
	gboolean foundit = FALSE;
	
	if (gtk_tree_model_get_iter_first (store, iter))
	{
		do
		{
			gtk_tree_model_get(store, iter, 3, &ivalue, -1);
			if (value == ivalue)
			{
				foundit = TRUE;
				break;
			}
		} while (gtk_tree_model_iter_next (store, iter));
	}
	return foundit;
}

void
audio_track_opts_set(GtkBuilder *builder, const gchar *name, gint titleindex)
{
	GtkTreeIter iter;
	GtkListStore *store;
	hb_list_t  * list = NULL;
	hb_title_t * title = NULL;
    hb_audio_config_t * audio;
	gint ii;
	gint count = 0;
	
	g_debug("audio_track_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	if (h_scan != NULL)
	{
		list = hb_get_titles( h_scan );
	    title = (hb_title_t*)hb_list_item( list, titleindex );
		if (title != NULL)
		{
			count = hb_list_count( title->list_audio );
		}
	}
	if (count > 10) count = 10;
	if (audio_track_opts.map) g_free(audio_track_opts.map);
	if (count > 0)
	{
		audio_track_opts.count = count;
		audio_track_opts.map = g_malloc(count*sizeof(options_map_t));
	}
	else
	{
		audio_track_opts.count = 1;
		audio_track_opts.map = g_malloc(sizeof(options_map_t));
	}
	if( count <= 0 )
	{
		// No audio. set some default
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, "No Audio", 
						   1, TRUE, 
						   2, "none", 
						   3, -1, 
						   4, "none", 
						   -1);
		audio_track_opts.map[0].option = "No Audio";
		audio_track_opts.map[0].shortOpt = "none";
		audio_track_opts.map[0].ivalue = -1;
		audio_track_opts.map[0].svalue = "none";
		return;
	}
	for (ii = 0; ii < count; ii++)
	{
        audio = (hb_audio_config_t *) hb_list_audio_config_item( title->list_audio, ii );
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, audio->lang.description, 
						   1, TRUE, 
						   2, index_str[ii], 
						   3, ii, 
						   4, index_str[ii], 
						   -1);
		audio_track_opts.map[ii].option = audio->lang.description,
		audio_track_opts.map[ii].shortOpt = index_str[ii];
		audio_track_opts.map[ii].ivalue = ii;
		audio_track_opts.map[ii].svalue = index_str[ii];
	}
}


void
subtitle_opts_set(GtkBuilder *builder, const gchar *name, gint titleindex)
{
	GtkTreeIter iter;
	GtkListStore *store;
	hb_list_t  * list = NULL;
	hb_title_t * title = NULL;
    hb_subtitle_t * subtitle;
	gint ii;
	gint count = 0;
	
	g_debug("subtitle_opts_set () %s\n", name);
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	if (h_scan != NULL)
	{
		list = hb_get_titles( h_scan );
	    title = (hb_title_t*)hb_list_item( list, titleindex );
		if (title != NULL)
		{
			count = hb_list_count( title->list_subtitle );
		}
	}
	if (count > 10) count = 10;
	if (subtitle_opts.map) g_free(subtitle_opts.map);
	if (count > 0)
	{
		subtitle_opts.count = count+2;
		subtitle_opts.map = g_malloc((count+2)*sizeof(options_map_t));
	}
	else
	{
		subtitle_opts.count = LANG_TABLE_SIZE+2;
		subtitle_opts.map = g_malloc((LANG_TABLE_SIZE+2)*sizeof(options_map_t));
	}
	// Add options for "none" and "autoselect"
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
					   0, "None", 
					   1, TRUE, 
					   2, "none", 
					   3, -2, 
					   4, "none", 
					   -1);
	subtitle_opts.map[0].option = "None";
	subtitle_opts.map[0].shortOpt = "none";
	subtitle_opts.map[0].ivalue = -2;
	subtitle_opts.map[0].svalue = "none";
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
					   0, "Same as audio", 
					   1, TRUE, 
					   2, "auto", 
					   3, -1, 
					   4, "auto", 
					   -1);
	subtitle_opts.map[0].option = "Same as audio";
	subtitle_opts.map[0].shortOpt = "auto";
	subtitle_opts.map[0].ivalue = -1;
	subtitle_opts.map[0].svalue = "auto";
	if (count >0)
	{
		for (ii = 0; ii < count; ii++)
		{
       		subtitle = (hb_subtitle_t *)hb_list_item(title->list_subtitle, ii);
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 
					   	0, subtitle->lang, 
					   	1, TRUE, 
					   	2, subtitle->iso639_2, 
					   	3, ii, 
					   	4, subtitle->iso639_2, 
					   	-1);
			subtitle_opts.map[ii+2].option = subtitle->lang;
			subtitle_opts.map[ii+2].shortOpt = subtitle->iso639_2;
			subtitle_opts.map[ii+2].ivalue = ii;
			subtitle_opts.map[ii+2].svalue = subtitle->iso639_2;
		}
	}
	else
	{
		for (ii = 0; ii < LANG_TABLE_SIZE; ii++)
		{
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 
				0, language_table[ii].eng_name, 
				1, TRUE, 
				2, language_table[ii].iso639_2, 
				3, ii, 
				4, language_table[ii].iso639_2, 
				-1);
			subtitle_opts.map[ii+2].option = language_table[ii].eng_name;
			subtitle_opts.map[ii+2].shortOpt = language_table[ii].iso639_2;
			subtitle_opts.map[ii+2].ivalue = ii;
			subtitle_opts.map[ii+2].svalue = language_table[ii].iso639_2;
		}
	}
}

gint
ghb_longest_title()
{
	hb_list_t  * list;
	hb_title_t * title;
	gint ii;
	gint count = 0;
	guint64 longest = 0;
	gint titleindex = 0;
	
	g_debug("ghb_longest_title ()\n");
	if (h_scan == NULL) return 0;
	list = hb_get_titles( h_scan );
	count = hb_list_count( list );
	if (count > 100) count = 100;
	for (ii = 0; ii < count; ii++)
	{
		title = (hb_title_t*)hb_list_item(list, ii);
		if (title->duration > longest)
		{
			titleindex = ii;
			longest = title->duration;
		}
	}
	return titleindex;
}

gint
ghb_find_audio_track(gint titleindex, const gchar *lang, gint index)
{
	hb_list_t  * list;
	hb_title_t * title;
    hb_audio_config_t * audio;
	gint ii;
	gint count = 0;
	gint track = -1;
	gint match = 0;
	
	g_debug("find_audio_track ()\n");
	if (h_scan != NULL)
	{
		list = hb_get_titles( h_scan );
	    title = (hb_title_t*)hb_list_item( list, titleindex );
		if (title != NULL)
		{
			count = hb_list_count( title->list_audio );
		}
	}
	if (count > 10) count = 10;
	for (ii = 0; ii < count; ii++)
	{
        audio = (hb_audio_config_t*)hb_list_audio_config_item( title->list_audio, ii );
		if ((strcmp(lang, audio->lang.iso639_2) == 0) ||
			(strcmp(lang, "und") == 0))
		{
			if (index == match)
			{
				track = ii;
				break;
			}
			match++;
		}
	}
	return track;
}

static void
generic_opts_set(GtkBuilder *builder, const gchar *name, combo_opts_t *opts)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ii;
	
	g_debug("generic_opts_set ()\n");
	if (name == NULL || opts == NULL) return;
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	for (ii = 0; ii < opts->count; ii++)
	{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, opts->map[ii].option, 
						   1, TRUE, 
						   2, opts->map[ii].shortOpt, 
						   3, opts->map[ii].ivalue, 
						   4, opts->map[ii].svalue, 
						   -1);
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
	if (strcmp(name, "audio_bitrate") == 0)
		return lookup_audio_bitrate_int(gval);
	else if (strcmp(name, "audio_rate") == 0)
		return lookup_audio_rate_int(gval);
	else if (strcmp(name, "framerate") == 0)
		return lookup_video_rate_int(gval);
	else if (strcmp(name, "audio_mix") == 0)
		return lookup_mix_int(gval);
	else if (strcmp(name, "source_audio_lang") == 0)
		return lookup_audio_lang_int(gval);
	else
	{
		return lookup_generic_int(find_combo_table(name), gval);
	}
	g_warning("ghb_lookup_combo_int() couldn't find %s", name);
	return 0;
}

const gchar*
ghb_lookup_combo_option(const gchar *name, const GValue *gval)
{
	if (strcmp(name, "audio_bitrate") == 0)
		return lookup_audio_bitrate_option(gval);
	else if (strcmp(name, "audio_rate") == 0)
		return lookup_audio_rate_option(gval);
	else if (strcmp(name, "framerate") == 0)
		return lookup_video_rate_option(gval);
	else if (strcmp(name, "audio_mix") == 0)
		return lookup_mix_option(gval);
	else if (strcmp(name, "source_audio_lang") == 0)
		return lookup_audio_lang_option(gval);
	else
	{
		return lookup_generic_option(find_combo_table(name), gval);
	}
	g_warning("ghb_lookup_combo_int() couldn't find %s", name);
	return 0;
}

void
ghb_update_ui_combo_box(GtkBuilder *builder, const gchar *name, gint user_data, gboolean all)
{
	GtkComboBox *combo = NULL;
	gint signal_id;
	gint handler_id = 0;

	g_debug("ghb_update_ui_combo_box() %s\n", name);
	if (name != NULL)
	{		
		// Clearing a combo box causes a rash of "changed" events, even when
		// the active item is -1 (inactive).  To control things, I'm disabling
		// the event till things are settled down.
		combo = GTK_COMBO_BOX(GHB_WIDGET(builder, name));
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
		audio_bitrate_opts_set(builder, "audio_bitrate");
		audio_samplerate_opts_set(builder, "audio_rate", hb_audio_rates, hb_audio_rates_count);
		video_rate_opts_set(builder, "framerate", hb_video_rates, hb_video_rates_count);
		mix_opts_set(builder, "audio_mix");
		language_opts_set(builder, "source_audio_lang");
		subtitle_opts_set(builder, "subtitle_lang", user_data);
		title_opts_set(builder, "title");
		audio_track_opts_set(builder, "audio_track", user_data);
		generic_opts_set(builder, "container", &container_opts);
		generic_opts_set(builder, "deinterlace", &deint_opts);
		generic_opts_set(builder, "tweak_deinterlace", &deint_opts);
		generic_opts_set(builder, "denoise", &denoise_opts);
		generic_opts_set(builder, "tweak_denoise", &denoise_opts);
		generic_opts_set(builder, "video_codec", &vcodec_opts);
		generic_opts_set(builder, "audio_codec", &acodec_opts);
		generic_opts_set(builder, "x264_direct", &direct_opts);
		generic_opts_set(builder, "x264_me", &me_opts);
		generic_opts_set(builder, "x264_subme", &subme_opts);
		generic_opts_set(builder, "x264_analyse", &analyse_opts);
		generic_opts_set(builder, "x264_trellis", &trellis_opts);
	}
	else
	{
		if (strcmp(name, "audio_bitrate") == 0)
			audio_bitrate_opts_set(builder, "audio_bitrate");
		else if (strcmp(name, "audio_rate") == 0)
			audio_samplerate_opts_set(builder, "audio_rate", hb_audio_rates, hb_audio_rates_count);
		else if (strcmp(name, "framerate") == 0)
			video_rate_opts_set(builder, "framerate", hb_video_rates, hb_video_rates_count);
		else if (strcmp(name, "audio_mix") == 0)
			mix_opts_set(builder, "audio_mix");
		else if (strcmp(name, "source_audio_lang") == 0)
			language_opts_set(builder, "source_audio_lang");
		else if (strcmp(name, "subtitle_lang") == 0)
			subtitle_opts_set(builder, "subtitle_lang", user_data);
		else if (strcmp(name, "title") == 0)
			title_opts_set(builder, "title");
		else if (strcmp(name, "audio_track") == 0)
			audio_track_opts_set(builder, "audio_track", user_data);
		else
			generic_opts_set(builder, name, find_combo_table(name));
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

	init_combo_box(builder, "audio_bitrate");
	init_combo_box(builder, "audio_rate");
	init_combo_box(builder, "framerate");
	init_combo_box(builder, "audio_mix");
	init_combo_box(builder, "source_audio_lang");
	init_combo_box(builder, "subtitle_lang");
	init_combo_box(builder, "title");
	init_combo_box(builder, "audio_track");
	for (ii = 0; combo_name_map[ii].name != NULL; ii++)
	{
		init_combo_box(builder, combo_name_map[ii].name);
	}
}
	
static const char * turbo_opts = 
	"ref=1:subme=1:me=dia:analyse=none:trellis=0:"
	"no-fast-pskip=0:8x8dct=0:weightb=0";

// Construct the x264 options string
// The result is allocated, so someone must free it at some point.
gchar*
ghb_build_x264opts_string(GValue *settings)
{
	gchar *result;
	gchar *opts = ghb_settings_get_string(settings, "x264_options");
	if (opts != NULL)
	{
		result = opts;
	}
	else
	{
		result = g_strdup("");
	}
	return result;
}

GValue*
ghb_get_chapters(gint titleindex)
{
	hb_list_t  * list;
	hb_title_t * title;
    hb_chapter_t * chapter;
	gint count, ii;
	GValue *chapters = NULL;
	
	g_debug("ghb_get_chapters (title = %d)\n", titleindex);
	if (h_scan == NULL) return NULL;
	list = hb_get_titles( h_scan );
    title = (hb_title_t*)hb_list_item( list, titleindex );
	if (title == NULL) return NULL;
	count = hb_list_count( title->list_chapter );
	chapters = ghb_array_value_new(count);
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
		acodec = ghb_settings_combo_int(asettings, "audio_codec");
		if (acodec == HB_ACODEC_AC3)
			return TRUE;
	}
	return FALSE;
}

static void
audio_bitrate_opts_add(GtkBuilder *builder, const gchar *name, gint rate)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gchar *str;
	
	g_debug("audio_rate_opts_add ()\n");
	store = get_combo_box_store(builder, name);
	if (!find_combo_item_by_int(GTK_TREE_MODEL(store), rate, &iter))
	{
		str = g_strdup_printf ("%d", rate);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, str, 
						   1, TRUE, 
						   2, str, 
						   3, rate, 
						   4, str, 
						   -1);
		g_free(str);
	}
}

static void
audio_bitrate_opts_clean(GtkBuilder *builder, const gchar *name, gint last_rate)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ivalue;
	gboolean done = FALSE;
	gint ii = 0;
	guint last = (guint)last_rate;
	
	g_debug("audio_bitrate_opts_clean ()\n");
	store = get_combo_box_store(builder, name);
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL(store), &iter))
	{
		do
		{
			gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 3, &ivalue, -1);
			if (search_rates(
				hb_audio_bitrates, ivalue, hb_audio_bitrates_count) < 0)
			{
				done = !gtk_list_store_remove(store, &iter);
			}
			else if (ivalue > last)
			{
				ii++;
				gtk_list_store_set(store, &iter, 1, FALSE, -1);
				done = !gtk_tree_model_iter_next (GTK_TREE_MODEL(store), &iter);
			}
			else
			{
				ii++;
				gtk_list_store_set(store, &iter, 1, TRUE, -1);
				done = !gtk_tree_model_iter_next (GTK_TREE_MODEL(store), &iter);
			}
		} while (!done);
	}
}

static void
audio_bitrate_opts_set(GtkBuilder *builder, const gchar *name)
{
	GtkTreeIter iter;
	GtkListStore *store;
	gint ii;
	
	g_debug("audio_bitrate_opts_set ()\n");
	store = get_combo_box_store(builder, name);
	gtk_list_store_clear(store);
	for (ii = 0; ii < hb_audio_bitrates_count; ii++)
	{
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, hb_audio_bitrates[ii].string, 
						   1, TRUE, 
						   2, hb_audio_bitrates[ii].string, 
						   3, hb_audio_bitrates[ii].rate, 
						   4, hb_audio_bitrates[ii].string, 
						   -1);
	}
}

void
ghb_set_passthru_bitrate_opts(GtkBuilder *builder, gint bitrate)
{
	audio_bitrate_opts_add(builder, "audio_bitrate", bitrate);
}

void
ghb_set_default_bitrate_opts(GtkBuilder *builder, gint last_rate)
{
	audio_bitrate_opts_clean(builder, "audio_bitrate", last_rate);
}

static ghb_status_t hb_status;

void
ghb_backend_init(GtkBuilder *builder, gint debug, gint update)
{
    /* Init libhb */
    h_scan = hb_init( debug, update );
    h_queue = hb_init( debug, 0 );
	// Set up the list model for the combos
	init_ui_combo_boxes(builder);
	// Populate all the combos
	ghb_update_ui_combo_box(builder, NULL, 0, TRUE);
}

void
ghb_backend_close()
{
	hb_close(&h_queue);
	hb_close(&h_scan);
}

void
ghb_backend_scan(const gchar *path, gint titleindex)
{
    hb_scan( h_scan, path, titleindex );
	hb_status.state |= GHB_STATE_SCANNING;
	// initialize count and cur to something that won't cause FPE
	// when computing progress
	hb_status.title_count = 1;
	hb_status.title_cur = 0;
}

void
ghb_backend_queue_scan(const gchar *path, gint titlenum)
{
	g_debug("ghb_backend_queue_scan()");
	hb_scan( h_queue, path, titlenum );
	hb_status.queue_state |= GHB_STATE_SCANNING;
}

gint
ghb_get_state()
{
	return hb_status.state;
}

gint
ghb_get_queue_state()
{
	return hb_status.queue_state;
}

void
ghb_clear_state(gint state)
{
	hb_status.state &= ~state;
}

void
ghb_clear_queue_state(gint state)
{
	hb_status.queue_state &= ~state;
}

void
ghb_set_state(gint state)
{
	hb_status.state |= state;
}

void
ghb_set_queue_state(gint state)
{
	hb_status.queue_state |= state;
}

void
ghb_get_status(ghb_status_t *status)
{
	memcpy(status, &hb_status, sizeof(ghb_status_t));
}

void 
ghb_track_status()
{
    hb_state_t s;
    hb_state_t s_queue;

	if (h_scan == NULL) return;
    hb_get_state( h_scan, &s );
	switch( s.state )
    {
#define p s.param.scanning
        case HB_STATE_SCANNING:
		{
			hb_status.state |= GHB_STATE_SCANNING;
			hb_status.title_count = p.title_count;
			hb_status.title_cur = p.title_cur;
		} break;
#undef p

        case HB_STATE_SCANDONE:
        {
			hb_status.state &= ~GHB_STATE_SCANNING;
			hb_status.state |= GHB_STATE_SCANDONE;
        } break;

    }
    hb_get_state( h_queue, &s_queue );
	switch( s_queue.state )
    {
        case HB_STATE_SCANNING:
		{
			hb_status.queue_state |= GHB_STATE_SCANNING;
		} break;

        case HB_STATE_SCANDONE:
        {
			hb_status.queue_state &= ~GHB_STATE_SCANNING;
			hb_status.queue_state |= GHB_STATE_SCANDONE;
        } break;

#define p s_queue.param.working
        case HB_STATE_WORKING:
			hb_status.queue_state |= GHB_STATE_WORKING;
			hb_status.queue_state &= ~GHB_STATE_PAUSED;
			hb_status.job_cur = p.job_cur;
			hb_status.job_count = p.job_count;
			hb_status.progress = p.progress;
			hb_status.rate_cur = p.rate_cur;
			hb_status.rate_avg = p.rate_avg;
			hb_status.hours = p.hours;
			hb_status.minutes = p.minutes;
			hb_status.seconds = p.seconds;
			hb_status.unique_id = p.sequence_id & 0xFFFFFF;
            break;
#undef p

        case HB_STATE_PAUSED:
			hb_status.queue_state |= GHB_STATE_PAUSED;
            break;
				
        case HB_STATE_MUXING:
        {
			hb_status.queue_state |= GHB_STATE_MUXING;
        } break;

#define p s_queue.param.workdone
        case HB_STATE_WORKDONE:
		{
            hb_job_t *job;

			hb_status.queue_state |= GHB_STATE_WORKDONE;
			hb_status.queue_state &= ~GHB_STATE_MUXING;
			hb_status.queue_state &= ~GHB_STATE_PAUSED;
			hb_status.queue_state &= ~GHB_STATE_WORKING;
			switch (p.error)
			{
			case HB_ERROR_NONE:
				hb_status.error = GHB_ERROR_NONE;
			case HB_ERROR_CANCELED:
				hb_status.error = GHB_ERROR_CANCELED;
			default:
				hb_status.error = GHB_ERROR_FAIL;
			}
			hb_status.error = p.error;
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

gboolean
ghb_get_title_info(ghb_title_info_t *tinfo, gint titleindex)
{
	hb_list_t  * list;
	hb_title_t * title;
	
    if (h_scan == NULL) return FALSE;
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		return FALSE;
	}

    title = hb_list_item( list, titleindex );
	if (title == NULL) return FALSE;	// Bad titleindex
	tinfo->width = title->width;
	tinfo->height = title->height;
	memcpy(tinfo->crop, title->crop, 4 * sizeof(int));
	// Don't allow crop to 0
	if (title->crop[0] + title->crop[1] >= title->height)
		title->crop[0] = title->crop[1] = 0;
	if (title->crop[2] + title->crop[3] >= title->width)
		title->crop[2] = title->crop[3] = 0;
	tinfo->num_chapters = hb_list_count(title->list_chapter);
	tinfo->rate_base = title->rate_base;
	tinfo->rate = title->rate;
	hb_reduce(&(tinfo->aspect_n), &(tinfo->aspect_d), 
				title->width * title->pixel_aspect_width, 
				title->height * title->pixel_aspect_height);
	tinfo->hours = title->hours;
	tinfo->minutes = title->minutes;
	tinfo->seconds = title->seconds;
	tinfo->duration = title->duration;
	return TRUE;
}

gboolean
ghb_get_audio_info(ghb_audio_info_t *ainfo, gint titleindex, gint audioindex)
{
    hb_audio_config_t *audio;
	
	audio = get_hb_audio(titleindex, audioindex);
	if (audio == NULL) return FALSE; // Bad audioindex
	ainfo->codec = audio->in.codec;
	ainfo->bitrate = audio->in.bitrate;
	ainfo->samplerate = audio->in.samplerate;
	return TRUE;
}

gboolean
ghb_audio_is_passthru(gint acodec)
{
	g_debug("ghb_audio_is_passthru () \n");
	g_debug("acodec %d\n", acodec);
	return (acodec == HB_ACODEC_AC3);
}

gint
ghb_get_default_acodec()
{
	return HB_ACODEC_FAAC;
}

void
ghb_set_scale(signal_user_data_t *ud, gint mode)
{
	hb_list_t  * list;
	hb_title_t * title;
	hb_job_t   * job;
	gboolean keep_aspect, round_dims, anamorphic;
	gboolean autocrop, autoscale, noscale;
	gint crop[4], width, height, par_width, par_height;
	gint crop_width, crop_height;
	gint aspect_n, aspect_d;
	gboolean keep_width = (mode == GHB_SCALE_KEEP_WIDTH);
	gboolean keep_height = (mode == GHB_SCALE_KEEP_HEIGHT);
	gint step;
	GtkWidget *widget;
	gint modshift;
	gint modround;
	gint max_width = 0;
	gint max_height = 0;
	
	g_debug("ghb_set_scale ()\n");

	if (h_scan == NULL) return;
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		return;
	}
	gint titleindex;

	titleindex = ghb_settings_combo_int(ud->settings, "title");
    title = hb_list_item( list, titleindex );
	if (title == NULL) return;
	job   = title->job;
	if (job == NULL) return;
	
	// First configure widgets
	round_dims = ghb_settings_get_boolean(ud->settings, "round_dimensions");
	anamorphic = ghb_settings_get_boolean(ud->settings, "anamorphic");
	keep_aspect = ghb_settings_get_boolean(ud->settings, "keep_aspect");
	autocrop = ghb_settings_get_boolean(ud->settings, "autocrop");
	autoscale = ghb_settings_get_boolean(ud->settings, "autoscale");
	// "Noscale" is a flag that says we prefer to crop extra to satisfy
	// alignment constraints rather than scaling to satisfy them.
	noscale = ghb_settings_get_boolean(ud->settings, "noscale");
	// Align dimensions to either 16 or 2 pixels
	// The scaler crashes if the dimensions are not divisible by 2
	// x264 also will not accept dims that are not multiple of 2
	modshift = round_dims ? 4 : 1;
	modround = round_dims ? 8 : 1;
	if (autoscale)
	{
		keep_width = FALSE;
		keep_height = FALSE;
	}
	if (anamorphic || keep_aspect)
	{
		keep_height = FALSE;
	}
	// Step needs to be at least 2 because odd widths cause scaler crash
	step = round_dims ? 16 : 2;
	widget = GHB_WIDGET (ud->builder, "scale_width");
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), step, 16);
	widget = GHB_WIDGET (ud->builder, "scale_height");
	gtk_spin_button_set_increments (GTK_SPIN_BUTTON(widget), step, 16);
	if (autocrop)
	{
		ghb_title_info_t tinfo;

		if (ghb_get_title_info (&tinfo, titleindex))
		{
			crop[0] = tinfo.crop[0];
			crop[1] = tinfo.crop[1];
			crop[2] = tinfo.crop[2];
			crop[3] = tinfo.crop[3];
			if (noscale)
			{
				gint need1, need2;

				// Adjust the cropping to accomplish the desired width and height
				crop_width = tinfo.width - crop[2] - crop[3];
				crop_height = tinfo.height - crop[0] - crop[1];
				width = (crop_width >> modshift) << modshift;
				height = (crop_height >> modshift) << modshift;
				need1 = (crop_height - height) / 2;
				need2 = crop_height - height - need1;
				crop[0] += need1;
				crop[1] += need2;
				need1 = (crop_width - width) / 2;
				need2 = crop_width - width - need1;
				crop[2] += need1;
				crop[3] += need2;
			}
			ghb_ui_update(ud, "crop_top", ghb_int64_value(crop[0]));
			ghb_ui_update(ud, "crop_bottom", ghb_int64_value(crop[1]));
			ghb_ui_update(ud, "crop_left", ghb_int64_value(crop[2]));
			ghb_ui_update(ud, "crop_right", ghb_int64_value(crop[3]));
		}
	}
	crop[0] = ghb_settings_get_int(ud->settings, "crop_top");
	crop[1] = ghb_settings_get_int(ud->settings, "crop_bottom");
	crop[2] = ghb_settings_get_int(ud->settings, "crop_left");
	crop[3] = ghb_settings_get_int(ud->settings, "crop_right");
	hb_reduce(&aspect_n, &aspect_d, 
				title->width * title->pixel_aspect_width, 
				title->height * title->pixel_aspect_height);
	crop_width = title->width - crop[2] - crop[3];
	crop_height = title->height - crop[0] - crop[1];
	if (autoscale)
	{
		width = crop_width;
		height = crop_height;
		max_width = crop_width;
		max_height = crop_height;
	}
	else
	{
		width = ghb_settings_get_int(ud->settings, "scale_width");
		height = ghb_settings_get_int(ud->settings, "scale_height");
		max_width = ghb_settings_get_int(ud->settings, "max_width");
		max_height = ghb_settings_get_int(ud->settings, "max_width");
		// Align max dims 
		max_width = (max_width >> modshift) << modshift;
		max_height = (max_height >> modshift) << modshift;
		// Adjust dims according to max values
		if (!max_height)
		{
			max_height = crop_height;
		}
		if (!max_width)
		{
			max_width = crop_width;
		}
		height = MIN(height, max_height);
		width = MIN(width, max_width);
		g_debug("max_width %d, max_height %d\n", max_width, max_height);
	}
	if (width < 16)
		width = title->width - crop[2] - crop[3];
	if (height < 16)
		height = title->height - crop[0] - crop[1];

	if (anamorphic)
	{
		if (round_dims)
		{
			job->modulus = 0;
		}
		else
		{
			// The scaler crashes if the dimensions are not divisible by 2
			// Align mod 2.  And so does something in x264_encoder_headers()
			job->modulus = 2;
		}
		job->width = width;
		if (max_height) 
			job->maxHeight = max_height;
		job->crop[0] = crop[0];	job->crop[1] = crop[1];
		job->crop[2] = crop[2];	job->crop[3] = crop[3];
		hb_set_anamorphic_size( job, &width, &height, 
								&par_width, &par_height );
	}
	else 
	{
		if (keep_aspect)
		{
			gdouble par;
			gint new_width, new_height;
			
			g_debug("kw %s kh %s\n", keep_width ? "y":"n", keep_height ? "y":"n");
			g_debug("w %d h %d\n", width, height);
			// Compute pixel aspect ration.  
			par = (gdouble)(title->height * aspect_n) / (title->width * aspect_d);
			// Must scale so that par becomes 1:1
			// Try to keep largest dimension
			new_height = (crop_height * ((gdouble)width/crop_width) / par);
			new_width = (crop_width * ((gdouble)height/crop_height) * par);
			// Height and width are always multiples of 2, so do the rounding
			new_height = ((new_height + 1) >> 1) << 1;
			new_width = ((new_width + 1) >> 1) << 1;
			g_debug("max w %d, new w %d\n", max_width, new_width);
			if (max_width && (new_width > max_width))
			{
				height = new_height;
			}
			else if (max_height && (new_height > max_height))
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
		width = ((width + modround) >> modshift) << modshift;
		height = ((height + modround) >> modshift) << modshift;
	}
	ghb_ui_update(ud, "scale_width", ghb_int64_value(width));
	ghb_ui_update(ud, "scale_height", ghb_int64_value(height));
}

static void
set_preview_job_settings(hb_job_t *job, GValue *settings)
{
	job->crop[0] = ghb_settings_get_int(settings, "crop_top");
	job->crop[1] = ghb_settings_get_int(settings, "crop_bottom");
	job->crop[2] = ghb_settings_get_int(settings, "crop_left");
	job->crop[3] = ghb_settings_get_int(settings, "crop_right");

	gboolean anamorphic, round_dimensions;
	anamorphic = ghb_settings_get_boolean(settings, "anamorphic");
	round_dimensions = ghb_settings_get_boolean(settings, "round_dimensions");
	if (round_dimensions && anamorphic)
	{
		job->modulus = 16;
		job->pixel_ratio = 2;
	}
	else if (anamorphic)
	{
		job->modulus = 2;
		job->pixel_ratio = 2;
	}
	else
	{
		job->modulus = 2;
		job->pixel_ratio = 0;
	}
	job->width = ghb_settings_get_int(settings, "scale_width");
	job->height = ghb_settings_get_int(settings, "scale_height");
	gint deint = ghb_settings_combo_int(settings, "deinterlace");
	gboolean decomb = ghb_settings_get_boolean(settings, "decomb");
	job->deinterlace = (!decomb && deint == 0) ? 0 : 1;
}

gint
ghb_calculate_target_bitrate(GValue *settings, gint titleindex)
{
	hb_list_t  * list;
	hb_title_t * title;
	hb_job_t   * job;
	gint size;

	if (h_scan == NULL) return 2000;
	list = hb_get_titles( h_scan );
    title = hb_list_item( list, titleindex );
	if (title == NULL) return 2000;
	job   = title->job;
	if (job == NULL) return 2000;
	size = ghb_settings_get_int(settings, "video_target_size");
	return hb_calc_bitrate( job, size );
}

gint
ghb_guess_bitrate(GValue *settings)
{
	gint bitrate;
	if (ghb_settings_get_boolean(settings, "vquality_type_constant"))
	{
		// This is really rough.  I'm trying to err on the high
		// side since this is used to estimate if there is 
		// sufficient disk space left
		gint vcodec;
		gdouble vquality;

		vcodec = ghb_settings_combo_int(settings, "video_codec");
		vquality = ghb_settings_get_double(settings, "video_quality")/100;
		if (vcodec == HB_VCODEC_X264 && 
				!ghb_settings_get_boolean(settings, "linear_vquality"))
		{
			vquality = 51.0 - vquality * 51.0;
			// Convert log curve to linear
			vquality = exp2((vquality-12)/6);
			// Don't let it go to 0
			if (vquality >= 31) vquality = 30;
			vquality = (31 - vquality) / 31;
		}
		// bitrate seems to be a log relasionship to quality
		// with typical source material
		// This is a real wag
		bitrate = 20*1024*1024*exp10(vquality*14)/exp10(14);
		// Add some bits for audio
		bitrate += 500*1024;
	}
	else
	{
		// Add some fudge to the bitrate to leave breathing room
		bitrate = ghb_settings_get_int(settings, "video_bitrate")*1024;
		// Add some bits for audio
		bitrate += 500*1024;
	}
	return bitrate;
}

gboolean
ghb_validate_filter_string(const gchar *str, gint max_fields)
{
	gint fields = 0;
	gchar *end;
	gdouble val;

	if (str == NULL || *str == 0) return TRUE;
	while (*str)
	{
		val = g_strtod(str, &end);
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
ghb_validate_filters(signal_user_data_t *ud)
{
	gboolean tweaks;
	gchar *str;
	gint index;
	gchar *message;
	gboolean enabled;

	tweaks = ghb_settings_get_boolean(ud->settings, "allow_tweaks");
	if (tweaks)
	{
		// detele 6
		str = ghb_settings_get_string(ud->settings, "tweak_detelecine");
		enabled = ghb_settings_get_boolean(ud->settings, "detelecine");
		if (enabled && !ghb_validate_filter_string(str, 6))
		{
			message = g_strdup_printf(
						"Invalid Detelecine Settings:\n\n%s\n",
						str);
			ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
			g_free(str);
			g_free(message);
			return FALSE;
		}
		g_free(str);
		// decomb 7
		str = ghb_settings_get_string(ud->settings, "tweak_decomb");
		enabled = ghb_settings_get_boolean(ud->settings, "decomb");
		if (enabled && !ghb_validate_filter_string(str, 7))
		{
			message = g_strdup_printf(
						"Invalid Decomb Settings:\n\n%s\n",
						str);
			ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
			g_free(str);
			g_free(message);
			return FALSE;
		}
		g_free(str);
		// deinte 4
		index = ghb_lookup_combo_int("tweak_deinterlace", 
			ghb_settings_get_value(ud->settings, "tweak_deinterlace"));
		if (index < 0)
		{
			str = ghb_settings_get_string(ud->settings, "tweak_deinterlace");
			if (!ghb_validate_filter_string(str, 4))
			{
				message = g_strdup_printf(
							"Invalid Deinterlace Settings:\n\n%s\n",
							str);
				ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
				g_free(message);
				g_free(str);
				return FALSE;
			}
			g_free(str);
		}
#if 0
		// Deblock is being reworked
		// debloc 2
		str = ghb_settings_get_string(ud->settings, "tweak_deblock");
		enabled = ghb_settings_get_boolean(ud->settings, "deblock");
		if (enabled && !ghb_validate_filter_string(str, 2))
		{
			message = g_strdup_printf(
						"Invalid Deblock Settings:\n\n%s\n",
						str);
			ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
			g_free(str);
			g_free(message);
			return FALSE;
		}
		g_free(str);
#endif
		// denois 4
		index = ghb_lookup_combo_int("tweak_denoise", 
				ghb_settings_get_value(ud->settings, "tweak_denoise"));
		if (index < 0)
		{
			str = ghb_settings_get_string(ud->settings, "tweak_denoise");
			if (!ghb_validate_filter_string(str, 4))
			{
				message = g_strdup_printf(
							"Invalid Denoise Settings:\n\n%s\n",
							str);
				ghb_message_dialog(GTK_MESSAGE_ERROR, message, "Cancel", NULL);
				g_free(str);
				g_free(message);
				return FALSE;
			}
			g_free(str);
		}
	}
	return TRUE;
}

gboolean
ghb_validate_video(signal_user_data_t *ud)
{
	gint vcodec, mux;
	gchar *message;

	mux = ghb_settings_combo_int(ud->settings, "container");
	vcodec = ghb_settings_combo_int(ud->settings, "video_codec");
	if ((mux == HB_MUX_MP4 || mux == HB_MUX_AVI) && 
		(vcodec == HB_VCODEC_THEORA))
	{
		// mp4|avi/theora combination is not supported.
		message = g_strdup_printf(
					"Theora is not supported in the MP4 and AVI containers.\n\n"
					"You should choose a different video codec or container.\n"
					"If you continue, XviD will be chosen for you.");
		if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, "Cancel", "Continue"))
		{
			g_free(message);
			return FALSE;
		}
		g_free(message);
		vcodec = HB_VCODEC_XVID;
		ghb_ui_update(ud, "video_codec", ghb_int64_value(vcodec));
	}
	return TRUE;
}

gboolean
ghb_validate_container(signal_user_data_t *ud)
{
	gint container;
	gchar *message;

	container = ghb_settings_combo_int(ud->settings, "container");
	if (container == HB_MUX_MP4)
	{
		const GValue *audio_list;
		gboolean httpopt;

		audio_list = ghb_settings_get_value(ud->settings, "audio_list");
		httpopt = ghb_settings_get_boolean(ud->settings, "http_optimize_mp4");
		if (httpopt && ghb_ac3_in_audio_list(audio_list))
		{
			message = g_strdup_printf(
					"AC3 audio in HTTP optimized MP4 is not supported.\n\n"
					"You should choose a different audio codec.\n"
					"If you continue, FAAC will be chosen for you.");
			if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, "Cancel", "Continue"))
			{
				g_free(message);
				return FALSE;
			}
			g_free(message);

			gint count, ii;

			count = ghb_array_len(audio_list);
			for (ii = 0; ii < count; ii++)
			{
				GValue *asettings;

				asettings = ghb_array_get_nth(audio_list, ii);
				gint acodec = ghb_settings_combo_int(asettings, "audio_codec");
				if (acodec == HB_ACODEC_AC3)
				{
					GValue *value;
					value = get_acodec_value(HB_ACODEC_FAAC);
					ghb_settings_take_value(asettings, "audio_codec", value);
					value = get_abitrate_value(160);
					ghb_settings_take_value(asettings, "audio_bitrate", value);
				}
			}
		}
	}
	return TRUE;
}

gboolean
ghb_validate_audio(signal_user_data_t *ud)
{
	hb_list_t  * list;
	hb_title_t * title;
	gchar *message;
	GValue *value;

	if (h_scan == NULL) return FALSE;
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		g_message("No title found.\n");
		return FALSE;
	}

	gint titleindex;

	titleindex = ghb_settings_combo_int(ud->settings, "title");
    title = hb_list_item( list, titleindex );
	if (title == NULL) return FALSE;
	gint mux = ghb_settings_combo_int(ud->settings, "container");

	const GValue *audio_list;
	gint count, ii;

	audio_list = ghb_settings_get_value(ud->settings, "audio_list");
	count = ghb_array_len(audio_list);
	for (ii = 0; ii < count; ii++)
	{
		GValue *asettings;
	    hb_audio_config_t *taudio;

		asettings = ghb_array_get_nth(audio_list, ii);
		gint track = ghb_settings_combo_int(asettings, "audio_track");
		gint codec = ghb_settings_combo_int(asettings, "audio_codec");
        taudio = (hb_audio_config_t *) hb_list_audio_config_item(
											title->list_audio, track );
		if ((taudio->in.codec != HB_ACODEC_AC3) && (codec == HB_ACODEC_AC3))
		{
			// Not supported.  AC3 is passthrough only, so input must be AC3
			message = g_strdup_printf(
						"The source does not support AC3 Pass-Thru.\n\n"
						"You should choose a different audio codec.\n"
						"If you continue, one will be chosen for you.");
			if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, "Cancel", "Continue"))
			{
				g_free(message);
				return FALSE;
			}
			g_free(message);
			if (mux == HB_MUX_AVI)
			{
				codec = HB_ACODEC_LAME;
			}
			else
			{
				codec = HB_ACODEC_FAAC;
			}
			value = get_acodec_value(codec);
			ghb_settings_take_value(asettings, "audio_codec", value);
		}
		gchar *a_unsup = NULL;
		gchar *mux_s = NULL;
		if (mux == HB_MUX_MP4)
		{ 
			mux_s = "MP4";
			// mp4/mp3|vorbis combination is not supported.
			if (codec == HB_ACODEC_LAME)
			{
				a_unsup = "MP3";
				codec = HB_ACODEC_FAAC;
			}
			if (codec == HB_ACODEC_VORBIS)
			{
				a_unsup = "Vorbis";
				codec = HB_ACODEC_FAAC;
			}
		}
		else if (mux == HB_MUX_AVI)
		{
			mux_s = "AVI";
			// avi/faac|vorbis combination is not supported.
			if (codec == HB_ACODEC_FAAC)
			{
				a_unsup = "FAAC";
				codec = HB_ACODEC_LAME;
			}
			if (codec == HB_ACODEC_VORBIS)
			{
				a_unsup = "Vorbis";
				codec = HB_ACODEC_LAME;
			}
		}
		else if (mux == HB_MUX_OGM)
		{
			mux_s = "OGM";
			// avi/faac|vorbis combination is not supported.
			if (codec == HB_ACODEC_FAAC)
			{
				a_unsup = "FAAC";
				codec = HB_ACODEC_VORBIS;
			}
			if (codec == HB_ACODEC_AC3)
			{
				a_unsup = "AC-3";
				codec = HB_ACODEC_VORBIS;
			}
		}
		if (a_unsup)
		{
			message = g_strdup_printf(
						"%s is not supported in the %s container.\n\n"
						"You should choose a different audio codec.\n"
						"If you continue, one will be chosen for you.", a_unsup, mux_s);
			if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, "Cancel", "Continue"))
			{
				g_free(message);
				return FALSE;
			}
			g_free(message);
			value = get_acodec_value(codec);
			ghb_settings_take_value(asettings, "audio_codec", value);
		}
		gint mix = ghb_settings_combo_int (asettings, "audio_mix");
		gboolean allow_mono = TRUE;
		gboolean allow_stereo = TRUE;
		gboolean allow_dolby = TRUE;
		gboolean allow_dpl2 = TRUE;
		gboolean allow_6ch = TRUE;
		allow_mono =
			(taudio->in.codec & (HB_ACODEC_AC3|HB_ACODEC_DCA)) &&
			(codec != HB_ACODEC_LAME);
		gint layout = taudio->in.channel_layout & HB_INPUT_CH_LAYOUT_DISCRETE_NO_LFE_MASK;
		allow_stereo =
			((layout == HB_INPUT_CH_LAYOUT_MONO && !allow_mono) || layout >= HB_INPUT_CH_LAYOUT_STEREO);
		allow_dolby =
			(layout == HB_INPUT_CH_LAYOUT_3F1R) || 
			(layout == HB_INPUT_CH_LAYOUT_3F2R) || 
			(layout == HB_INPUT_CH_LAYOUT_DOLBY);
		allow_dpl2 = (layout == HB_INPUT_CH_LAYOUT_3F2R);
		allow_6ch =
			(taudio->in.codec & (HB_ACODEC_AC3|HB_ACODEC_DCA)) &&
			(codec != HB_ACODEC_LAME) &&
			(layout == HB_INPUT_CH_LAYOUT_3F2R) && 
			(taudio->in.channel_layout & HB_INPUT_CH_LAYOUT_HAS_LFE);

		gchar *mix_unsup = NULL;
		if (mix == HB_AMIXDOWN_MONO && !allow_mono)
		{
			mix_unsup = "mono";
		}
		if (mix == HB_AMIXDOWN_STEREO && !allow_stereo)
		{
			mix_unsup = "stereo";
		}
		if (mix == HB_AMIXDOWN_DOLBY && !allow_dolby)
		{
			mix_unsup = "Dolby";
		}
		if (mix == HB_AMIXDOWN_DOLBYPLII && !allow_dpl2)
		{
			mix_unsup = "Dolby Pro Logic II";
		}
		if (mix == HB_AMIXDOWN_6CH && !allow_6ch)
		{
			mix_unsup = "6 Channel";
		}
		if (mix_unsup)
		{
			message = g_strdup_printf(
						"The source audio does not support %s mixdown.\n\n"
						"You should choose a different mixdown.\n"
						"If you continue, one will be chosen for you.", mix_unsup);
			if (!ghb_message_dialog(GTK_MESSAGE_WARNING, message, "Cancel", "Continue"))
			{
				g_free(message);
				return FALSE;
			}
			g_free(message);
			mix = ghb_get_best_mix(titleindex, track, codec, mix);
			value = get_amix_value(mix);
			ghb_settings_take_value(asettings, "audio_mix", value);
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

	if (ghb_settings_get_boolean(settings, "nocheckvquality")) return TRUE;
	vcodec = ghb_settings_combo_int(settings, "video_codec");
	if (ghb_settings_get_boolean(settings, "vquality_type_constant"))
	{
		if (!ghb_settings_get_boolean(settings, "directqp"))
		{
			if (vcodec != HB_VCODEC_X264 || 
				ghb_settings_get_boolean(settings, "linear_vquality"))
			{
				min = 68;
				max = 97;
			}
			else if (vcodec == HB_VCODEC_X264)
			{
				min = 40;
				max = 70;
			}
		}
		else
		{
			if (vcodec == HB_VCODEC_X264)
			{
				min = 16;
				max = 30;
			}
			else if (vcodec == HB_VCODEC_FFMPEG)
			{
				min = 1;
				max = 8;
			}
			else
			{
				min = 68;
				max = 97;
			}
		}
		gint vquality = ghb_settings_get_double(settings, "video_quality");
		if (vquality < min || vquality > max)
		{
			message = g_strdup_printf(
						"Interesting video quality choise: %d\n\n"
						"Typical values range from %d to %d.\n"
						"Are you sure you wish to use this setting?",
						vquality, min, max);
			if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message, 
									"Cancel", "Continue"))
			{
				g_free(message);
				return FALSE;
			}
			g_free(message);
		}
	}
	return TRUE;
}

void
ghb_add_job(GValue *js, gint unique_id)
{
	hb_list_t  * list;
	hb_title_t * title;
	hb_job_t   * job;
	static gchar *x264opts;
	gint sub_id = 0;
	gboolean tweaks = FALSE;
	gchar *detel_str = NULL;
	gchar *decomb_str = NULL;
	gchar *deint_str = NULL;
	gchar *deblock_str = NULL;
	gchar *denoise_str = NULL;
	gchar *dest_str = NULL;

	g_debug("ghb_add_job()\n");
	if (h_queue == NULL) return;
	list = hb_get_titles( h_queue );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		g_message("No title found.\n");
		return;
	}

	// Since I'm doing a scan of the single title I want just prior 
	// to adding the job, there is only the one title to choose from.
	//gint titleindex = ghb_settings_get_int(js, "title");
    gint titleindex = 0;
    title = hb_list_item( list, titleindex );
	if (title == NULL) return;

	/* Set job settings */
	job   = title->job;
	if (job == NULL) return;

	tweaks = ghb_settings_get_boolean(js, "allow_tweaks");
	job->mux = ghb_settings_combo_int(js, "container");
	if (job->mux == HB_MUX_MP4)
	{
		job->largeFileSize = ghb_settings_get_boolean(js, "large_mp4");
		job->mp4_optimize = ghb_settings_get_boolean(js, "http_optimize_mp4");
	}
	else
	{
		job->largeFileSize = FALSE;
		job->mp4_optimize = FALSE;
	}
	gint chapter_start, chapter_end;
	chapter_start = ghb_settings_get_int(js, "start_chapter");
	chapter_end = ghb_settings_get_int(js, "end_chapter");
	gint num_chapters = hb_list_count(title->list_chapter);
	job->chapter_start = MIN( num_chapters, chapter_start );
	job->chapter_end   = MAX( job->chapter_start, chapter_end );

	job->chapter_markers = ghb_settings_get_boolean(js, "chapter_markers");
	if ( job->chapter_markers )
	{
		GValue *chapters;
		GValue *chapter;
		gint chap;
		gint count;
		
		chapters = ghb_settings_get_value(js, "chapter_list");
		count = ghb_array_len(chapters);
		for(chap = chapter_start; chap <= chapter_end; chap++)
		{
			hb_chapter_t * chapter_s;
			gchar *name;
			
			name = NULL;
			if (chap-1 < count)
			{
				chapter = ghb_array_get_nth(chapters, chap-1);
				name = ghb_value_string(chapter); 
			}
			if (name == NULL)
			{
				name = g_strdup_printf ("Chapter %2d", chap);
			}
			chapter_s = hb_list_item( job->title->list_chapter, chap - 1);
			strncpy(chapter_s->title, name, 1023);
			chapter_s->title[1023] = '\0';
			g_free(name);
		}
	}
	job->crop[0] = ghb_settings_get_int(js, "crop_top");
	job->crop[1] = ghb_settings_get_int(js, "crop_bottom");
	job->crop[2] = ghb_settings_get_int(js, "crop_left");
	job->crop[3] = ghb_settings_get_int(js, "crop_right");

	
	gboolean decomb = ghb_settings_get_boolean(js, "decomb");
	gint deint = ghb_settings_combo_int(js, 
					tweaks ? "tweak_deinterlace":"deinterlace");
	if (!decomb)
		job->deinterlace = (deint != 0) ? 1 : 0;
	else
		job->deinterlace = 0;
    job->grayscale   = ghb_settings_get_boolean(js, "grayscale");

	gboolean anamorphic = ghb_settings_get_boolean(js, "anamorphic");
	gboolean round_dimensions = ghb_settings_get_boolean(js, "round_dimensions");
	if (round_dimensions && anamorphic)
	{
		job->pixel_ratio = 2;
		job->modulus = 16;
	}
	else if (anamorphic)
	{
		// Huh! I thought I wanted to use pixel_ratio 1 for this case, but
		// when its 1, libhb discards the width and height and uses original
		// title dims - crop.  Thats not what I want.
		// Also, x264 requires things to divisible by 2.
		job->pixel_ratio = 2;
		job->modulus = 2;
	}
	else
	{
		job->pixel_ratio = 0;
		job->modulus = 2;
	}
	/* Add selected filters */
	job->filters = hb_list_init();
	gint vrate = ghb_settings_combo_int(js, "framerate");
	if( vrate == 0 && ghb_settings_get_boolean(js, "detelecine" ) )
		job->vfr = 1;
	else
		job->vfr = 0;

	if( ghb_settings_get_boolean(js, "detelecine" ) )
	{
		hb_filter_detelecine.settings = NULL;
		if (tweaks)
		{
			detel_str = ghb_settings_get_string(js, "tweak_detelecine");
			if (detel_str && detel_str[0])
			{
				hb_filter_detelecine.settings = detel_str;
			}
		}
		hb_list_add( job->filters, &hb_filter_detelecine );
	}
	if( decomb )
	{
		// Use default settings
		hb_filter_decomb.settings = NULL;
		if (tweaks)
		{
			decomb_str = ghb_settings_get_string(js, "tweak_decomb");
			if (decomb_str && decomb_str[0])
			{
				hb_filter_decomb.settings = (gchar*)decomb_str;
			}
		}
		hb_list_add( job->filters, &hb_filter_decomb );
	}
	if( job->deinterlace )
	{
		if (deint > 0)
			deint_str = g_strdup(deint_opts.map[deint].svalue);
		else
			deint_str = ghb_settings_get_string(js, 
					tweaks ? "tweak_deinterlace" : "deinterlace");
		hb_filter_deinterlace.settings = deint_str;
		hb_list_add( job->filters, &hb_filter_deinterlace );
	}
	gint deblock = ghb_settings_get_int(js, "deblock");
	if( deblock >= 5 )
	{
		deblock_str = g_strdup_printf("%d", deblock);
		hb_filter_deblock.settings = deblock_str;
		hb_list_add( job->filters, &hb_filter_deblock );
	}
	gint denoise = ghb_settings_combo_int(js, 
					tweaks ? "tweak_denoise" : "denoise");
	if( denoise != 0 )
	{
		if (denoise > 0)
			denoise_str = g_strdup(denoise_opts.map[denoise].svalue);
		else
			denoise_str = (gchar*)ghb_settings_get_string(
				js, tweaks ? "tweak_denoise" : "denoise");
		hb_filter_denoise.settings = denoise_str;
		hb_list_add( job->filters, &hb_filter_denoise );
	}
	job->width = ghb_settings_get_int(js, "scale_width");
	job->height = ghb_settings_get_int(js, "scale_height");

	job->vcodec = ghb_settings_combo_int(js, "video_codec");
	if ((job->mux == HB_MUX_MP4 || job->mux == HB_MUX_AVI) && 
		(job->vcodec == HB_VCODEC_THEORA))
	{
		// mp4|avi/theora combination is not supported.
		job->vcodec = HB_VCODEC_XVID;
	}
	if ((job->vcodec == HB_VCODEC_X264) && (job->mux == HB_MUX_MP4))
	{
		job->ipod_atom = ghb_settings_get_boolean(js, "ipod_file");
	}
	if (ghb_settings_get_boolean(js, "vquality_type_constant"))
	{
		gdouble vquality;
		vquality = ghb_settings_get_double(js, "video_quality");
		if (!ghb_settings_get_boolean(js, "directqp"))
		{
			vquality /= 100.0;
			if (ghb_settings_get_boolean(js, "linear_vquality"))
			{
				if (job->vcodec == HB_VCODEC_X264)
				{
					// Adjust to same range as xvid and ffmpeg
					vquality = 31.0 - vquality * 31.0;
					if (vquality > 0)
					{
						// Convert linear to log curve
						vquality = 12 + 6 * log2(vquality);
						if (vquality > 51) vquality = 51;
						if (vquality < 1) vquality = 1;
					}
					else
						vquality = 0;
				}
			}
			else
			{
				if (vquality == 0.0) vquality = 0.01;
				if (vquality == 1.0) vquality = 0.0;
			}
		}
		job->vquality =  vquality;
		job->vbitrate = 0;
	}
	else if (ghb_settings_get_boolean(js, "vquality_type_bitrate"))
	{
		job->vquality = -1.0;
		job->vbitrate = ghb_settings_get_int(js, "video_bitrate");
	}
	// AVI container does not support variable frame rate.
	if (job->mux == HB_MUX_AVI)
	{
		job->vfr = FALSE;
		job->cfr = 1;
	}

	if( vrate == 0 )
	{
		job->vrate = title->rate;
		job->vrate_base = title->rate_base;
		job->cfr = 0;
	}
	else
	{
		job->vrate = 27000000;
		job->vrate_base = vrate;
		job->cfr = 1;
	}
	// First remove any audios that are already in the list
	// This happens if you are encoding the same title a second time.
	gint num_audio_tracks = hb_list_count(job->list_audio);
	gint ii;
    for(ii = 0; ii < num_audio_tracks; ii++)
    {
        hb_audio_t *audio = (hb_audio_t*)hb_list_item(job->list_audio, 0);
        hb_list_rem(job->list_audio, audio);
    }

	const GValue *audio_list;
	gint count;
	gint tcount = 0;
	
	audio_list = ghb_settings_get_value(js, "audio_list");
	count = ghb_array_len(audio_list);
	for (ii = 0; ii < count; ii++)
	{
		GValue *asettings;
	    hb_audio_config_t audio;
	    hb_audio_config_t *taudio;

		hb_audio_config_init(&audio);
		asettings = ghb_array_get_nth(audio_list, ii);
		audio.in.track = ghb_settings_get_int(asettings, "audio_track");
		audio.out.track = tcount;
		audio.out.codec = ghb_settings_combo_int(asettings, "audio_codec");
        taudio = (hb_audio_config_t *) hb_list_audio_config_item(
									title->list_audio, audio.in.track );
		if ((taudio->in.codec != HB_ACODEC_AC3) && (audio.out.codec == HB_ACODEC_AC3))
		{
			// Not supported.  AC3 is passthrough only, so input must be AC3
			if (job->mux == HB_MUX_AVI)
			{
				audio.out.codec = HB_ACODEC_LAME;
			}
			else
			{
				audio.out.codec = HB_ACODEC_FAAC;
			}
		}
		if ((job->mux == HB_MUX_MP4) && 
			((audio.out.codec == HB_ACODEC_LAME) ||
			(audio.out.codec == HB_ACODEC_VORBIS)))
		{
			// mp4/mp3|vorbis combination is not supported.
			audio.out.codec = HB_ACODEC_FAAC;
		}
		if ((job->mux == HB_MUX_AVI) && 
			((audio.out.codec == HB_ACODEC_FAAC) ||
			(audio.out.codec == HB_ACODEC_VORBIS)))
		{
			// avi/faac|vorbis combination is not supported.
			audio.out.codec = HB_ACODEC_LAME;
		}
		if ((job->mux == HB_MUX_OGM) && 
			((audio.out.codec == HB_ACODEC_FAAC) ||
			(audio.out.codec == HB_ACODEC_AC3)))
		{
			// ogm/faac|ac3 combination is not supported.
			audio.out.codec = HB_ACODEC_VORBIS;
		}
        audio.out.dynamic_range_compression = 
			ghb_settings_get_double(asettings, "audio_drc");
		// It would be better if this were done in libhb for us, but its not yet.
		if (audio.out.codec == HB_ACODEC_AC3 || audio.out.codec == HB_ACODEC_DCA)
		{
			audio.out.mixdown = 0;
		}
		else
		{
			audio.out.mixdown = ghb_settings_combo_int(asettings, "audio_mix");
			// Make sure the mixdown is valid and pick a new one if not.
			audio.out.mixdown = ghb_get_best_mix(titleindex, 
				audio.in.track, audio.out.codec, audio.out.mixdown);
			audio.out.bitrate = 
				ghb_settings_combo_int(asettings, "audio_bitrate");
			gint srate = ghb_settings_combo_int(asettings, "audio_rate");
			if (srate == 0)	// 0 is same as source
				audio.out.samplerate = taudio->in.samplerate;
			else
				audio.out.samplerate = srate;
		}

		// Add it to the jobs audio list
        hb_audio_add( job, &audio );
		tcount++;
	}
	// I was tempted to move this up with the reset of the video quality
	// settings, but I suspect the settings above need to be made
	// first in order for hb_calc_bitrate to be accurate.
	if (ghb_settings_get_boolean(js, "vquality_type_target"))
	{
		gint size;
		
		size = ghb_settings_get_int(js, "video_target_size");
        job->vbitrate = hb_calc_bitrate( job, size );
		job->vquality = -1.0;
	}

	dest_str = ghb_settings_get_string(js, "destination");
	job->file = dest_str;
	job->crf = ghb_settings_get_boolean(js, "constant_rate_factor");
	// TODO: libhb holds onto a reference to the x264opts and is not
	// finished with it until encoding the job is done.  But I can't
	// find a way to get at the job before it is removed in order to
	// free up the memory I am allocating here.
	// The short story is THIS LEAKS.
	x264opts = ghb_build_x264opts_string(js);
	
	if( x264opts != NULL && *x264opts != '\0' )
	{
		job->x264opts = x264opts;
	}
	else /*avoids a bus error crash when options aren't specified*/
	{
		job->x264opts =  NULL;
	}
	gint subtitle;
	gchar *slang = ghb_settings_get_string(js, "subtitle_lang");
	subtitle = -2; // default to none
	if (strcmp(slang, "auto") == 0)
	{
		subtitle = -1;
	}
	else
	{
		gint scount;
    	hb_subtitle_t * subt;

		scount = hb_list_count(title->list_subtitle);
		for (ii = 0; ii < scount; ii++)
		{
        	subt = (hb_subtitle_t *)hb_list_item(title->list_subtitle, ii);
			if (strcmp(slang, subt->iso639_2) == 0)
			{
				subtitle = ii;
				break;
			}
		}
	}
	gboolean forced_subtitles = ghb_settings_get_boolean(js, "forced_subtitles");
	job->subtitle_force = forced_subtitles;
	if (subtitle >= 0)
		job->subtitle = subtitle;
	else
		job->subtitle = -1;
	if (subtitle == -1)
	{
		// Subtitle scan. Look for subtitle matching audio language
		char *x264opts_tmp;

		/*
		 * When subtitle scan is enabled do a fast pre-scan job
		 * which will determine which subtitles to enable, if any.
		 */
		job->pass = -1;
		job->indepth_scan = 1;

		x264opts_tmp = job->x264opts;
		job->x264opts = NULL;

		job->select_subtitle = malloc(sizeof(hb_subtitle_t*));
		*(job->select_subtitle) = NULL;

		/*
		 * Add the pre-scan job
		 */
		job->sequence_id = (unique_id & 0xFFFFFF) | (sub_id++ << 24);
		hb_add( h_queue, job );
		//if (job->x264opts != NULL)
		//	g_free(job->x264opts);

		job->x264opts = x264opts_tmp;
	}
	else
	{
		job->select_subtitle = NULL;
	}
	if( ghb_settings_get_boolean(js, "two_pass") &&
		!ghb_settings_get_boolean(js, "vquality_type_constant"))
	{
		/*
		 * If subtitle_scan is enabled then only turn it on
		 * for the second pass and then off again for the
		 * second.
		 */
		hb_subtitle_t **subtitle_tmp = job->select_subtitle;
		job->select_subtitle = NULL;
		job->pass = 1;
		job->indepth_scan = 0;
		gchar *x264opts2 = NULL;
		if (x264opts)
		{
			x264opts2 = g_strdup(x264opts);
		}
		/*
		 * If turbo options have been selected then append them
		 * to the x264opts now (size includes one ':' and the '\0')
		 */
		if( ghb_settings_get_boolean(js, "turbo") )
		{
			char *tmp_x264opts;

			if ( x264opts )
			{
				tmp_x264opts = g_strdup_printf("%s:%s", x264opts, turbo_opts);
				g_free(x264opts);
			} 
			else 
			{
				/*
				 * No x264opts to modify, but apply the turbo options
				 * anyway as they may be modifying defaults
				 */
				tmp_x264opts = g_strdup_printf("%s", turbo_opts);
			}
			x264opts = tmp_x264opts;

			job->x264opts = x264opts;
		}
		job->sequence_id = (unique_id & 0xFFFFFF) | (sub_id++ << 24);
		hb_add( h_queue, job );
		//if (job->x264opts != NULL)
		//	g_free(job->x264opts);

		job->select_subtitle = subtitle_tmp;
		job->pass = 2;
		/*
		 * On the second pass we turn off subtitle scan so that we
		 * can actually encode using any subtitles that were auto
		 * selected in the first pass (using the whacky select-subtitle
		 * attribute of the job).
		 */
		job->indepth_scan = 0;
		job->x264opts = x264opts2;
		job->sequence_id = (unique_id & 0xFFFFFF) | (sub_id++ << 24);
		hb_add( h_queue, job );
		//if (job->x264opts != NULL)
		//	g_free(job->x264opts);
	}
	else
	{
		job->indepth_scan = 0;
		job->pass = 0;
		job->sequence_id = (unique_id & 0xFFFFFF) | (sub_id++ << 24);
		hb_add( h_queue, job );
		//if (job->x264opts != NULL)
		//	g_free(job->x264opts);
	}
	if (detel_str) g_free(detel_str);
	if (decomb_str) g_free(decomb_str);
	if (deint_str) g_free(deint_str);
	if (deblock_str) g_free(deblock_str);
	if (denoise_str) g_free(denoise_str);
	if (dest_str) g_free(dest_str);
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
ghb_pause_queue()
{
    hb_state_t s;
    hb_get_state2( h_queue, &s );

    if( s.state == HB_STATE_PAUSED )
    {
        hb_resume( h_queue );
    }
    else
    {
        hb_pause( h_queue );
    }
}

GdkPixbuf*
ghb_get_preview_image(
	gint titleindex, 
	gint index, 
	GValue *settings, 
	gboolean borders)
{
	hb_title_t *title;
	hb_list_t  *list;
	
	list = hb_get_titles( h_scan );
	if( !hb_list_count( list ) )
	{
		/* No valid title, stop right there */
		return NULL;
	}
    title = hb_list_item( list, titleindex );
	if (title == NULL) return NULL;
	if (title->job == NULL) return NULL;
	set_preview_job_settings(title->job, settings);

	// hb_get_preview can't handle sizes that are larger than the original title
	// dimensions
	if (title->job->width > title->width)
		title->job->width = title->width;
	
	if (title->job->height > title->height)
		title->job->height = title->height;
	// And also creates artifacts if the width is not a multiple of 8
	//title->job->width = ((title->job->width + 4) >> 3) << 3;
	// And the height must be a multiple of 2
	//title->job->height = ((title->job->height + 1) >> 1) << 1;
	
	// Make sure we have a big enough buffer to receive the image from libhb. libhb
	// creates images with a one-pixel border around the original content. Hence we
	// add 2 pixels horizontally and vertically to the buffer size.
	gint srcWidth = title->width + 2;
	gint srcHeight= title->height + 2;
	gint dstWidth = title->width;
	gint dstHeight= title->height;
	gint borderTop = 1;
	gint borderLeft = 1;
    if (borders)
    {
        //     |<---------- title->width ----------->|
        //     |   |<---- title->job->width ---->|   |
        //     |   |                             |   |
        //     .......................................
        //     ....+-----------------------------+....
        //     ....|                             |....<-- gray border
        //     ....|                             |....
        //     ....|                             |....
        //     ....|                             |<------- image
        //     ....|                             |....
        //     ....|                             |....
        //     ....|                             |....
        //     ....|                             |....
        //     ....|                             |....
        //     ....+-----------------------------+....
        //     .......................................
		dstWidth = title->job->width;
        dstHeight = title->job->height;
		borderTop = (srcHeight - dstHeight) / 2;
		borderLeft = (srcWidth - dstWidth) / 2;
		g_debug("boarders removed\n");
	}

	g_debug("src %d x %d\n", srcWidth, srcHeight);
	g_debug("dst %d x %d\n", dstWidth, dstHeight);
	g_debug("job dim %d x %d\n", title->job->width, title->job->height);
	g_debug("title crop %d:%d:%d:%d\n", 
			title->crop[0],
			title->crop[1],
			title->crop[2],
			title->crop[3]);
	g_debug("job crop %d:%d:%d:%d\n", 
			title->job->crop[0],
			title->job->crop[1],
			title->job->crop[2],
			title->job->crop[3]);
	static guint8 *buffer = NULL;
	static gint bufferSize = 0;

	gint newSize;
	newSize = srcWidth * srcHeight * 4;
	if( bufferSize < newSize )
	{
		bufferSize = newSize;
		buffer     = (guint8*) g_realloc( buffer, bufferSize );
	}
	hb_get_preview( h_scan, title, index, buffer );

	// Create an GdkPixbuf and copy the libhb image into it, converting it from
	// libhb's format something suitable. Along the way, we'll strip off the
	// border around libhb's image.
	
	// The image data returned by hb_get_preview is 4 bytes per pixel, BGRA format.
	// Alpha is ignored.

	GdkPixbuf *preview = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, dstWidth, dstHeight);
	guint8 *pixels = gdk_pixbuf_get_pixels (preview);
	
	guint32 *src = (guint32*)buffer;
	guint8 *dst = pixels;
	src += borderTop * srcWidth;    // skip top rows in src to get to first row of dst
	src += borderLeft;              // skip left pixels in src to get to first pixel of dst
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
		src += (srcWidth - dstWidth);   // skip to next row in src
	}
	// Got it, but hb_get_preview doesn't compensate for anamorphic, so lets
	// scale
	gint width, height, par_width, par_height;
	gboolean anamorphic = ghb_settings_get_boolean(settings, "anamorphic");
	if (anamorphic)
	{
		hb_set_anamorphic_size( title->job, &width, &height, &par_width, &par_height );
		if (par_width > par_height)
			dstWidth = dstWidth * par_width / par_height;
		else
			dstHeight = dstHeight * par_height / par_width;
	}
	
	g_debug("scaled %d x %d\n", dstWidth, dstHeight);
	GdkPixbuf *scaled_preview;
	scaled_preview = gdk_pixbuf_scale_simple(preview, dstWidth, dstHeight, GDK_INTERP_HYPER);
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
			*a = *b;
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
	if (name != NULL)
	{
		sanitize_volname(name);
		return g_strdup(name);
	}
	return name;
}
