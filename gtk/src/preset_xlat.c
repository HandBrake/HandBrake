#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "plist.h"
#include "values.h"

static GValue *defaults;

typedef struct
{
	gchar *mac_val;
	gchar *lin_val;
} value_map_t;

#if 0
static value_map_t subtitle_xlat[] =
{ 
	{ "None", "none" },
	{ "Auto", "auto" },
	{ "Any", "und" },
	{ "Afar", "aar" },
	{ "Abkhazian", "abk" },
	{ "Afrikaans", "afr" },
	{ "Akan", "aka" },
	{ "Albanian", "sqi" },
	{ "Amharic", "amh" },
	{ "Arabic", "ara" },
	{ "Aragonese", "arg" },
	{ "Armenian", "hye" },
	{ "Assamese", "asm" },
	{ "Avaric", "ava" },
	{ "Avestan", "ave" },
	{ "Aymara", "aym" },
	{ "Azerbaijani", "aze" },
	{ "Bashkir", "bak" },
	{ "Bambara", "bam" },
	{ "Basque", "eus" },
	{ "Belarusian", "bel" },
	{ "Bengali", "ben" },
	{ "Bihari", "bih" },
	{ "Bislama", "bis" },
	{ "Bosnian", "bos" },
	{ "Breton", "bre" },
	{ "Bulgarian", "bul" },
	{ "Burmese", "mya" },
	{ "Catalan", "cat" },
	{ "Chamorro", "cha" },
	{ "Chechen", "che" },
	{ "Chinese", "zho" },
	{ "Church Slavic", "chu" },
	{ "Chuvash", "chv" },
	{ "Cornish", "cor" },
	{ "Corsican", "cos" },
	{ "Cree", "cre" },
	{ "Czech", "ces" },
	{ "Danish", "dan" },
	{ "Divehi", "div" },
	{ "Dutch", "nld" },
	{ "Dzongkha", "dzo" },
	{ "English", "eng" },
	{ "Esperanto", "epo" },
	{ "Estonian", "est" },
	{ "Ewe", "ewe" },
	{ "Faroese", "fao" },
	{ "Fijian", "fij" },
	{ "Finnish", "fin" },
	{ "French", "fra" },
	{ "Western Frisian", "fry" },
	{ "Fulah", "ful" },
	{ "Georgian", "kat" },
	{ "German", "deu" },
	{ "Gaelic (Scots)", "gla" },
	{ "Irish", "gle" },
	{ "Galician", "glg" },
	{ "Manx", "glv" },
	{ "Greek, Modern", "ell" },
	{ "Guarani", "grn" },
	{ "Gujarati", "guj" },
	{ "Haitian", "hat" },
	{ "Hausa", "hau" },
	{ "Hebrew", "heb" },
	{ "Herero", "her" },
	{ "Hindi", "hin" },
	{ "Hiri Motu", "hmo" },
	{ "Hungarian", "hun" },
	{ "Igbo", "ibo" },
	{ "Icelandic", "isl" },
	{ "Ido", "ido" },
	{ "Sichuan Yi", "iii" },
	{ "Inuktitut", "iku" },
	{ "Interlingue", "ile" },
	{ "Interlingua", "ina" },
	{ "Indonesian", "ind" },
	{ "Inupiaq", "ipk" },
	{ "Italian", "ita" },
	{ "Javanese", "jav" },
	{ "Japanese", "jpn" },
	{ "Kalaallisut", "kal" },
	{ "Kannada", "kan" },
	{ "Kashmiri", "kas" },
	{ "Kanuri", "kau" },
	{ "Kazakh", "kaz" },
	{ "Central Khmer", "khm" },
	{ "Kikuyu", "kik" },
	{ "Kinyarwanda", "kin" },
	{ "Kirghiz", "kir" },
	{ "Komi", "kom" },
	{ "Kongo", "kon" },
	{ "Korean", "kor" },
	{ "Kuanyama", "kua" },
	{ "Kurdish", "kur" },
	{ "Lao", "lao" },
	{ "Latin", "lat" },
	{ "Latvian", "lav" },
	{ "Limburgan", "lim" },
	{ "Lingala", "lin" },
	{ "Lithuanian", "lit" },
	{ "Luxembourgish", "ltz" },
	{ "Luba-Katanga", "lub" },
	{ "Ganda", "lug" },
	{ "Macedonian", "mkd" },
	{ "Marshallese", "mah" },
	{ "Malayalam", "mal" },
	{ "Maori", "mri" },
	{ "Marathi", "mar" },
	{ "Malay", "msa" },
	{ "Malagasy", "mlg" },
	{ "Maltese", "mlt" },
	{ "Moldavian", "mol" },
	{ "Mongolian", "mon" },
	{ "Nauru", "nau" },
	{ "Navajo", "nav" },
	{ "Ndebele, South", "nbl" },
	{ "Ndebele, North", "nde" },
	{ "Ndonga", "ndo" },
	{ "Nepali", "nep" },
	{ "Norwegian Nynorsk", "nno" },
	{ "Norwegian Bokmål", "nob" },
	{ "Norwegian", "nor" },
	{ "Chichewa; Nyanja", "nya" },
	{ "Occitan", "oci" },
	{ "Ojibwa", "oji" },
	{ "Oriya", "ori" },
	{ "Oromo", "orm" },
	{ "Ossetian", "oss" },
	{ "Panjabi", "pan" },
	{ "Persian", "fas" },
	{ "Pali", "pli" },
	{ "Polish", "pol" },
	{ "Portuguese", "por" },
	{ "Pushto", "pus" },
	{ "Quechua", "que" },
	{ "Romansh", "roh" },
	{ "Romanian", "ron" },
	{ "Rundi", "run" },
	{ "Russian", "rus" },
	{ "Sango", "sag" },
	{ "Sanskrit", "san" },
	{ "Serbian", "srp" },
	{ "Croatian", "hrv" },
	{ "Sinhala", "sin" },
	{ "Slovak", "slk" },
	{ "Slovenian", "slv" },
	{ "Northern Sami", "sme" },
	{ "Samoan", "smo" },
	{ "Shona", "sna" },
	{ "Sindhi", "snd" },
	{ "Somali", "som" },
	{ "Sotho, Southern", "sot" },
	{ "Spanish", "spa" },
	{ "Sardinian", "srd" },
	{ "Swati", "ssw" },
	{ "Sundanese", "sun" },
	{ "Swahili", "swa" },
	{ "Swedish", "swe" },
	{ "Tahitian", "tah" },
	{ "Tamil", "tam" },
	{ "Tatar", "tat" },
	{ "Telugu", "tel" },
	{ "Tajik", "tgk" },
	{ "Tagalog", "tgl" },
	{ "Thai", "tha" },
	{ "Tibetan", "bod" },
	{ "Tigrinya", "tir" },
	{ "Tonga", "ton" },
	{ "Tswana", "tsn" },
	{ "Tsonga", "tso" },
	{ "Turkmen", "tuk" },
	{ "Turkish", "tur" },
	{ "Twi", "twi" },
	{ "Uighur", "uig" },
	{ "Ukrainian", "ukr" },
	{ "Urdu", "urd" },
	{ "Uzbek", "uzb" },
	{ "Venda", "ven" },
	{ "Vietnamese", "vie" },
	{ "Volapük", "vol" },
	{ "Welsh", "cym" },
	{ "Walloon", "wln" },
	{ "Wolof", "wol" },
	{ "Xhosa", "xho" },
	{ "Yiddish", "yid" },
	{ "Yoruba", "yor" },
	{ "Zhuang", "zha" },
	{ "Zulu", "zul" },
	{NULL, NULL}
};

static value_map_t vcodec_xlat[] =
{
	{"MPEG-4 (FFmpeg)", "ffmpeg"},
	{"MPEG-4 (XviD)", "ffmpeg"},
	{"H.264 (x264)", "x264"},
	{"VP3 (Theora)", "theora"},
	{NULL,NULL}
};

value_map_t container_xlat[] =
{
	{"MP4 file", "mp4"},
	{"M4V file", "m4v"},
	{"MKV file", "mkv"},
	{"AVI file", "avi"},
	{"OGM file", "ogm"},
	{NULL, NULL}
};

value_map_t framerate_xlat[] =
{
	{"Same as source", "source"},
	{"5", "5"},
	{"10", "10"},
	{"12", "12"},
	{"15", "15"},
	{"23.976", "23.976"},
	{"24", "24"},
	{"25", "25"},
	{"29.97", "29.97"},
	{NULL, NULL}
};

value_map_t deint_xlat[] =
{
	{"0", "none"},
	{"1", "fast"},
	{"2", "slow"},
	{"3", "slower"},
	{NULL, NULL}
};

value_map_t denoise_xlat[] =
{
	{"0", "none"},
	{"1", "weak"},
	{"2", "medium"},
	{"3", "strong"},
	{NULL, NULL}
};

static value_map_t acodec_xlat[] =
{
	{"AAC (faac)", "faac"},
	{"AC3 Passthru", "ac3"},
	{"MP3 (lame)", "lame"},
	{"Vorbis (vorbis)", "vorbis"},
	{NULL,NULL}
};

value_map_t samplerate_xlat[] =
{
	{"Auto", "source"},
	{"22.05", "22.05"},
	{"24", "24"},
	{"32", "32"},
	{"44.1", "44.1"},
	{"48", "48"},
	{NULL, NULL}
};

value_map_t mix_xlat[] =
{
	{"Mono", "mono"},
	{"Stereo", "stereo"},
	{"Dolby Surround", "dpl1"},
	{"Dolby Pro Logic II", "dpl2"},
	{"6-channel discrete", "6ch"},
	{"AC3 Passthru", "none"},
	{NULL, NULL}
};
#endif

typedef struct
{
	gchar *mac_key;
	gchar *lin_key;
	value_map_t *value_map;
	gboolean same;
	gboolean ignore;
} key_map_t;

key_map_t key_map[] = 
{
	{"Audio1Bitrate", NULL, NULL, FALSE, FALSE},
	{"Audio1Encoder", NULL, NULL, FALSE, FALSE},
	{"Audio1Mixdown", NULL, NULL, FALSE, FALSE},
	{"Audio1Samplerate", NULL, NULL, FALSE, FALSE},
	{"Audio1Track", NULL, NULL, FALSE, FALSE},
	{"Audio1TrackDescription", NULL, NULL, FALSE, FALSE},
	{"Audio1TrackDRCSlider", NULL, NULL, FALSE, FALSE},
	{"Audio2Bitrate", NULL, NULL, FALSE, FALSE},
	{"Audio2Encoder", NULL, NULL, FALSE, FALSE},
	{"Audio2Mixdown", NULL, NULL, FALSE, FALSE},
	{"Audio2Samplerate", NULL, NULL, FALSE, FALSE},
	{"Audio2Track", NULL, NULL, FALSE, FALSE},
	{"Audio2TrackDescription", NULL, NULL, FALSE, FALSE},
	{"Audio2TrackDRCSlider", NULL, NULL, FALSE, FALSE},
	{"ChapterMarkers", NULL, NULL, TRUE, FALSE},
	{"Default", NULL, NULL, TRUE, FALSE},
	{"FileFormat", NULL, NULL, TRUE, FALSE},
	{"Folder", NULL, NULL, TRUE, FALSE},
	{"Mp4HttpOptimize", NULL, NULL, TRUE, FALSE},
	{"Mp4iPodCompatible", NULL, NULL, TRUE, FALSE},
	{"Mp4LargeFile", NULL, NULL, TRUE, FALSE},
	{"PictureAutoCrop", NULL, NULL, TRUE, FALSE},
	{"PictureBottomCrop", NULL, NULL, TRUE, FALSE},
	{"PictureDeblock", NULL, NULL, TRUE, FALSE},
	{"PictureDecomb", NULL, NULL, TRUE, FALSE},
	{"PictureDeinterlace", NULL, NULL, TRUE, FALSE},
	{"PictureDenoise", NULL, NULL, TRUE, FALSE},
	{"PictureDetelecine", NULL, NULL, TRUE, FALSE},
	{"PictureHeight", NULL, NULL, TRUE, FALSE},
	{"PictureKeepRatio", NULL, NULL, TRUE, FALSE},
	{"PictureLeftCrop", NULL, NULL, TRUE, FALSE},
	{"PicturePAR", NULL, NULL, TRUE, FALSE},
	{"PictureRightCrop", NULL, NULL, TRUE, FALSE},
	{"PictureTopCrop", NULL, NULL, TRUE, FALSE},
	{"PictureWidth", NULL, NULL, TRUE, FALSE},
	{"PresetDescription", NULL, NULL, TRUE, FALSE},
	{"PresetName", NULL, NULL, TRUE, FALSE},
	{"Subtitles", NULL, NULL, TRUE, FALSE},
	{"SubtitlesForced", NULL, NULL, TRUE, FALSE},
	{"Type", NULL, NULL, TRUE, FALSE},
	{"UsesMaxPictureSettings", NULL, NULL, TRUE, FALSE},
	{"UsesPictureFilters", NULL, NULL, TRUE, FALSE},
	{"UsesPictureSettings", NULL, NULL, TRUE, FALSE},
	{"VFR", NULL, NULL, FALSE, TRUE},
	{"VideoAvgBitrate", NULL, NULL, TRUE, FALSE},
	{"VideoEncoder", NULL, NULL, TRUE, FALSE},
	{"VideoFramerate", NULL, NULL, TRUE, FALSE},
	{"VideoGrayScale", NULL, NULL, TRUE, FALSE},
	{"VideoQualitySlider", NULL, NULL, TRUE, FALSE},
	{"VideoQualityType", NULL, NULL, TRUE, FALSE},
	{"VideoTargetSize", NULL, NULL, TRUE, FALSE},
	{"VideoTwoPass", NULL, NULL, TRUE, FALSE},
	{"VideoTurboTwoPass", NULL, NULL, TRUE, FALSE},
	{"x264Option", NULL, NULL, TRUE, FALSE},
	{NULL, NULL}
};

const gint
key_xlat(key_map_t *key_map, const gchar *mac_key)
{
	gint ii;

	for (ii = 0; key_map[ii].mac_key; ii++)
	{
		if (strcmp(mac_key, key_map[ii].mac_key) == 0)
		{
			if (key_map[ii].ignore)
				return -1;
			return ii;
		}
	}
	g_warning("Unrecognized key: (%s)", mac_key);
	return -1;
}

static GValue*
value_xlat(
	GValue *defaults, 
	key_map_t *key_map, 
	gint key_index, 
	GValue *mac_val)
{
	GValue *gval, *def_val;
	const gchar *lin_key;
	if (key_map[key_index].same)
		lin_key = key_map[key_index].mac_key;
	else
		lin_key = key_map[key_index].lin_key;
	value_map_t *value_map = key_map[key_index].value_map;

	def_val = ghb_dict_lookup(defaults, lin_key);
	if (def_val)
	{
		if (value_map)
		{
			gint ii;
			gchar *str;
			GValue *sval;

			str = ghb_value_string(mac_val);
			for (ii = 0; value_map[ii].mac_val; ii++)
			{
				if (strcmp(str, value_map[ii].mac_val) == 0)
				{
					sval = ghb_string_value_new(value_map[ii].lin_val);
					g_free(str);
					gval = ghb_value_new(G_VALUE_TYPE(def_val));
					if (!g_value_transform(sval, gval))
					{
						g_warning("1 can't transform");
						ghb_value_free(gval);
						ghb_value_free(sval);
						return NULL;
					}
					ghb_value_free(sval);
					return gval;
				}
			}
			g_warning("Can't map value: (%s)", str);
			g_free(str);
		}
		else
		{
			gval = ghb_value_new(G_VALUE_TYPE(def_val));
			if (!g_value_transform(mac_val, gval))
			{
				g_warning("2 can't transform");
				ghb_value_free(gval);
				return NULL;
			}
			return gval;
		}
	}
	else
	{
		g_warning("Bad key: (%s)", lin_key);
		return NULL;
	}
	return NULL;
}

key_map_t audio_key_map[] =
{
	{"Audio1Bitrate", "AudioBitrate", NULL, FALSE, FALSE},
	{"Audio1Encoder", "AudioEncoder", NULL, FALSE, FALSE},
	{"Audio1Mixdown", "AudioMixdown", NULL, FALSE},
	{"Audio1Samplerate", "AudioSamplerate", NULL, FALSE, FALSE},
	{"Audio1Track", "AudioTrack", NULL, FALSE, FALSE},
	{"Audio1TrackDescription", "AudioTrackDescription", NULL, FALSE, FALSE},
	{"Audio1TrackDRCSlider", "AudioTrackDRCSlider", NULL, FALSE, FALSE},
	{"Audio2Bitrate", "AudioBitrate", NULL, FALSE, FALSE},
	{"Audio2Encoder", "AudioEncoder", NULL, FALSE, FALSE},
	{"Audio2Mixdown", "AudioMixdown", NULL, FALSE, FALSE},
	{"Audio2Samplerate", "AudioSamplerate", NULL, FALSE, FALSE},
	{"Audio2Track", "AudioTrack", NULL, FALSE, FALSE},
	{"Audio2TrackDescription", "AudioTrackDescription", NULL, FALSE, FALSE},
	{"Audio2TrackDRCSlider", "AudioTrackDRCSlider", NULL, FALSE, FALSE},
	{NULL, NULL}
};

static void
hard_value_xlat(GValue *lin_dict, const gchar *mac_key, GValue *mac_val)
{
	{
		gint key_index;
		GValue *audio_defaults;

		audio_defaults = ghb_array_get_nth(
			ghb_dict_lookup(defaults, "AudioList"), 0);
		key_index = key_xlat(audio_key_map, mac_key);
		if (key_index >= 0)
		{
			gint audio_index, count, ii;
			GValue *alist, *adict, *val;
			const gchar *lin_key;

			if (audio_key_map[key_index].same)
				lin_key = audio_key_map[key_index].mac_key;
			else
				lin_key = audio_key_map[key_index].lin_key;
			audio_index = mac_key[5] - '1';
			alist = ghb_dict_lookup(lin_dict, "AudioList");
			if (alist == NULL)
			{
				alist = ghb_array_value_new(8);
				ghb_dict_insert(lin_dict, "AudioList", alist);
			}
			count = ghb_array_len(alist);
			for (ii = count; ii <= audio_index; ii++)
			{
				adict = ghb_value_dup(audio_defaults);
				ghb_array_append(alist, adict);
			}
			adict = ghb_array_get_nth(alist, audio_index);
			val = value_xlat(audio_defaults, audio_key_map, key_index, mac_val);
			if (val)
			{
				ghb_dict_insert(adict, g_strdup(lin_key), val);
			}
		}
	}
}

static void
parse_preset_dict(GValue *mac_dict, GValue *lin_dict)
{
    GHashTableIter iter;
    gchar *key;
    GValue *mac_val, *val;

    ghb_dict_iter_init(&iter, mac_dict);
    // middle (void*) cast prevents gcc warning "defreferencing type-punned
    // pointer will break strict-aliasing rules"
	while (g_hash_table_iter_next(
			&iter, (gpointer*)(void*)&key, (gpointer*)(void*)&mac_val))
	{
		gint key_index;

		key_index = key_xlat(key_map, key);
		if (key_index >= 0)
		{ // The simple translations
			const gchar *lin_key;

			if (key_map[key_index].same)
				lin_key = key_map[key_index].mac_key;
			else
				lin_key = key_map[key_index].lin_key;
			if (lin_key)
			{
				val = value_xlat(defaults, key_map, key_index, mac_val);
				if (val)
				{
					ghb_dict_insert(lin_dict, g_strdup(lin_key), val);
				}
			}
			else
			{
				hard_value_xlat(lin_dict, key, mac_val);
			}
		}
	}
}

static void
parse_preset_array(GValue *mac_array, GValue *lin_array)
{
	gint count, ii;
	GValue *mac_dict, *lin_dict, *gval;

	count = ghb_array_len(mac_array);
	for (ii = 0; ii < count; ii++)
	{
		mac_dict = ghb_array_get_nth(mac_array, ii);
		
		// Only process builtin types
		if (ghb_value_int(ghb_dict_lookup(mac_dict, "Type")) != 0)
			continue;

		lin_dict = ghb_dict_value_new();
		ghb_array_append(lin_array, lin_dict);
		gval = ghb_dict_lookup(mac_dict, "PresetName");
		if (gval)
		{
			ghb_dict_insert(lin_dict, g_strdup("PresetName"), 
							ghb_value_dup(gval));
		}
		gval = ghb_dict_lookup(mac_dict, "PresetDescription");
		if (gval)
		{
			ghb_dict_insert(lin_dict, g_strdup("PresetDescription"), 
							ghb_value_dup(gval));
		}
		gval = ghb_dict_lookup(mac_dict, "Folder");
		if (gval && ghb_value_boolean(gval))
		{ // Folder
			GValue *mval, *lval;

			mval = ghb_dict_lookup(mac_dict, "ChildrenArray");
			lval = ghb_array_value_new(32);
			ghb_dict_insert(lin_dict, g_strdup("ChildrenArray"), lval);
			ghb_dict_insert(lin_dict, g_strdup("Folder"), 
							ghb_boolean_value_new(TRUE));
			ghb_dict_insert(lin_dict, g_strdup("Type"), 
							ghb_int_value_new(0));
			parse_preset_array(mval, lval);
		}
		else
		{ // Normal preset
			ghb_dict_insert(lin_dict, g_strdup("Type"), 
							ghb_int_value_new(0));
			parse_preset_dict(mac_dict, lin_dict);
		}
	}
}

static void
xlat(GValue *mac, GValue *lin)
{
	return parse_preset_array(mac, lin);
}

gint
main(gint argc, gchar *argv[])
{
	GValue *mac_plist, *lin_plist;
	GValue *internal;

	if (argc < 3)
	{
		fprintf(stderr, "Usage: <mac plist> <lin plist>\n");
		return 1;
	}
	g_type_init();

	ghb_register_transforms();
	internal = ghb_plist_parse_file("internal_defaults.xml");
	defaults = ghb_dict_lookup(internal, "Presets");
	mac_plist = ghb_plist_parse_file(argv[1]);
	lin_plist = ghb_array_value_new(32);
	xlat(mac_plist, lin_plist);
	ghb_plist_write_file(argv[2], lin_plist);
	return 0;
}

