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

static value_map_t vcodec_xlat[] =
{
	{"MPEG-4 (FFmpeg)", "ffmpeg"},
	{"MPEG-4 (XviD)", "xvid"},
	{"H.264 (x264)", "x264"},
	{"VP3 (Theora)", "theora"},
	{NULL,NULL}
};

static value_map_t acodec_xlat[] =
{
	{"AAC (faac)", "faac"},
	{"AC3 Passthru", "ac3"},
	{"MP3 (lame)", "lame"},
	{"Vorbis (vorbis)", "vorbis"},
	{NULL,NULL}
};

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

typedef struct
{
	gchar *mac_key;
	gchar *lin_key;
	value_map_t *value_map;
	gboolean ignore;
} key_map_t;

key_map_t key_map[] = 
{
	{"Audio1Bitrate", NULL, NULL, FALSE},
	{"Audio1Encoder", NULL, NULL, FALSE},
	{"Audio1Mixdown", NULL, NULL, FALSE},
	{"Audio1Samplerate", NULL, NULL, FALSE},
	{"Audio1Track", NULL, NULL, FALSE},
	{"Audio1TrackDescription", NULL, NULL, FALSE},
	{"Audio1TrackDRCSlider", NULL, NULL, FALSE},
	{"Audio2Bitrate", NULL, NULL, FALSE},
	{"Audio2Encoder", NULL, NULL, FALSE},
	{"Audio2Mixdown", NULL, NULL, FALSE},
	{"Audio2Samplerate", NULL, NULL, FALSE},
	{"Audio2Track", NULL, NULL, FALSE},
	{"Audio2TrackDescription", NULL, NULL, FALSE},
	{"Audio2TrackDRCSlider", NULL, NULL, FALSE},
	{"ChapterMarkers", "chapter_markers", NULL, FALSE},
	{"Default", "Default", NULL, FALSE},
	{"FileFormat", "container", container_xlat, FALSE},
	{"Folder", NULL, NULL, TRUE},
	{"Mp4HttpOptimize", "http_optimize_mp4", NULL, FALSE},
	{"Mp4iPodCompatible", "ipod_file", NULL, FALSE},
	{"Mp4LargeFile", "large_mp4", NULL, FALSE},
	{"PictureAutoCrop", "autocrop", NULL, FALSE},
	{"PictureBottomCrop", NULL, NULL, TRUE},
	{"PictureDeblock", "deblock", NULL, FALSE},
	{"PictureDecomb", "decomb", NULL, FALSE},
	{"PictureDeinterlace", "deinterlace", deint_xlat, FALSE},
	{"PictureDenoise", "denoise", denoise_xlat, FALSE},
	{"PictureDetelecine", "detelecine", NULL, FALSE},
	{"PictureHeight", "max_height", NULL, FALSE},
	{"PictureKeepRatio", "keep_aspect", NULL, FALSE},
	{"PictureLeftCrop", NULL, NULL, TRUE},
	{"PicturePAR", NULL, NULL, FALSE},
	{"PictureRightCrop", NULL, NULL, TRUE},
	{"PictureTopCrop", NULL, NULL, TRUE},
	{"PictureWidth", "max_width", NULL, FALSE},
	{"PresetDescription", "preset_description", NULL, FALSE},
	{"PresetName", "preset_name", NULL, FALSE},
	{"Subtitles", "subtitle_lang", subtitle_xlat, FALSE},
	{"SubtitlesForced", "forced_subtitles", NULL, FALSE},
	{"Type", NULL, NULL, TRUE},
	{"UsesMaxPictureSettings", NULL, NULL, FALSE},
	{"UsesPictureFilters", NULL, NULL, TRUE},
	{"UsesPictureSettings", NULL, NULL, FALSE},
	{"VFR", NULL, NULL, TRUE},
	{"VideoAvgBitrate", "video_bitrate", NULL, FALSE},
	{"VideoEncoder", "video_codec", vcodec_xlat, FALSE},
	{"VideoFramerate", "framerate", framerate_xlat, FALSE},
	{"VideoGrayScale", "grayscale", NULL, FALSE},
	{"VideoQualitySlider", NULL, NULL, FALSE},
	{"VideoQualityType", NULL, NULL, FALSE},
	{"VideoTargetSize", "video_target_size", NULL, FALSE},
	{"VideoTwoPass", "two_pass", NULL, FALSE},
	{"VideoTurboTwoPass", "turbo", NULL, FALSE},
	{"x264Option", "x264_options", NULL, FALSE},
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
	const gchar *lin_key = key_map[key_index].lin_key;
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
	{"Audio1Bitrate", "audio_bitrate", NULL, FALSE},
	{"Audio1Encoder", "audio_codec", acodec_xlat, FALSE},
	{"Audio1Mixdown", "audio_mix", mix_xlat, FALSE},
	{"Audio1Samplerate", "audio_rate", samplerate_xlat, FALSE},
	{"Audio1Track", NULL, NULL, TRUE},
	{"Audio1TrackDescription", NULL, NULL, TRUE},
	{"Audio1TrackDRCSlider", "audio_drc", NULL, FALSE},
	{"Audio2Bitrate", "audio_bitrate", NULL, FALSE},
	{"Audio2Encoder", "audio_codec", acodec_xlat, FALSE},
	{"Audio2Mixdown", "audio_mix", mix_xlat, FALSE},
	{"Audio2Samplerate", "audio_rate", samplerate_xlat, FALSE},
	{"Audio2Track", NULL, NULL, TRUE},
	{"Audio2TrackDescription", NULL, NULL, TRUE},
	{"Audio2TrackDRCSlider", "audio_drc", NULL, FALSE},
	{NULL, NULL}
};

static void
hard_value_xlat(GValue *lin_dict, const gchar *mac_key, GValue *mac_val)
{
	if (strcmp(mac_key, "VideoQualitySlider") == 0)
	{
		gint vquality;

		vquality = (ghb_value_double(mac_val) + 0.005) * 100.0;
		ghb_dict_insert(lin_dict, "video_quality", 
							ghb_int_value_new(vquality));
	}
	else if (strcmp(mac_key, "UsesMaxPictureSettings") == 0)
	{
		GValue *gval;

		gval = ghb_dict_lookup(lin_dict, "autoscale");
		if (gval == NULL && ghb_value_boolean(mac_val))
		{
			ghb_dict_insert(lin_dict, "autoscale", ghb_boolean_value_new(TRUE));
		}
	}
	else if (strcmp(mac_key, "UsesPictureSettings") == 0)
	{
		GValue *gval;

		gval = ghb_dict_lookup(lin_dict, "autoscale");
		if (gval == NULL && ghb_value_int(mac_val) == 2)
		{
			ghb_dict_insert(lin_dict, "autoscale", ghb_boolean_value_new(TRUE));
		}
	}
	else if (strcmp(mac_key, "PicturePAR") == 0)
	{
		gint ana;

		ana = ghb_value_int(mac_val);
		switch (ana)
		{
		case 0:
		{
			ghb_dict_insert(lin_dict, "anamorphic", 
							ghb_boolean_value_new(FALSE));
			ghb_dict_insert(lin_dict, "round_dimensions", 
							ghb_boolean_value_new(TRUE));
		} break;
		case 1:
		{
			ghb_dict_insert(lin_dict, "anamorphic", 
							ghb_boolean_value_new(TRUE));
			ghb_dict_insert(lin_dict, "round_dimensions", 
							ghb_boolean_value_new(FALSE));
		} break;
		case 2:
		{
			ghb_dict_insert(lin_dict, "anamorphic", 
							ghb_boolean_value_new(TRUE));
			ghb_dict_insert(lin_dict, "round_dimensions", 
							ghb_boolean_value_new(TRUE));
		} break;
		default:
		{
			ghb_dict_insert(lin_dict, "anamorphic", 
							ghb_boolean_value_new(TRUE));
			ghb_dict_insert(lin_dict, "round_dimensions", 
							ghb_boolean_value_new(TRUE));
		} break;
		}
	}
	else if (strcmp(mac_key, "VideoQualityType") == 0)
	{
		// VideoQualityType/0/1/2 - vquality_type_/target/bitrate/constant
		gint vqtype;

		vqtype = ghb_value_int(mac_val);
		switch (vqtype)
		{
		case 0:
		{
			ghb_dict_insert(lin_dict, "vquality_type_target", 
							ghb_boolean_value_new(TRUE));
			ghb_dict_insert(lin_dict, "vquality_type_bitrate", 
							ghb_boolean_value_new(FALSE));
			ghb_dict_insert(lin_dict, "vquality_type_constant", 
							ghb_boolean_value_new(FALSE));
		} break;
		case 1:
		{
			ghb_dict_insert(lin_dict, "vquality_type_target", 
							ghb_boolean_value_new(FALSE));
			ghb_dict_insert(lin_dict, "vquality_type_bitrate", 
							ghb_boolean_value_new(TRUE));
			ghb_dict_insert(lin_dict, "vquality_type_constant", 
							ghb_boolean_value_new(FALSE));
		} break;
		case 2:
		{
			ghb_dict_insert(lin_dict, "vquality_type_target", 
							ghb_boolean_value_new(FALSE));
			ghb_dict_insert(lin_dict, "vquality_type_bitrate", 
							ghb_boolean_value_new(FALSE));
			ghb_dict_insert(lin_dict, "vquality_type_constant", 
							ghb_boolean_value_new(TRUE));
		} break;
		default:
		{
			ghb_dict_insert(lin_dict, "vquality_type_target", 
							ghb_boolean_value_new(FALSE));
			ghb_dict_insert(lin_dict, "vquality_type_bitrate", 
							ghb_boolean_value_new(FALSE));
			ghb_dict_insert(lin_dict, "vquality_type_constant", 
							ghb_boolean_value_new(TRUE));
		} break;
		}
	}
	else
	{
		gint key_index;
		GValue *audio_defaults;

		audio_defaults = ghb_array_get_nth(
			ghb_dict_lookup(defaults, "pref_audio_list"), 0);
		key_index = key_xlat(audio_key_map, mac_key);
		if (key_index >= 0)
		{
			gint audio_index, count, ii;
			GValue *alist, *adict, *val;

			audio_index = mac_key[5] - '1';
			alist = ghb_dict_lookup(lin_dict, "pref_audio_list");
			if (alist == NULL)
			{
				alist = ghb_array_value_new(8);
				ghb_dict_insert(lin_dict, "pref_audio_list", alist);
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
				ghb_dict_insert(adict, 
							g_strdup(audio_key_map[key_index].lin_key), val);
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
			if (key_map[key_index].lin_key)
			{
				val = value_xlat(defaults, key_map, key_index, mac_val);
				if (val)
				{
					ghb_dict_insert(lin_dict, 
								g_strdup(key_map[key_index].lin_key), val);
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
			ghb_dict_insert(lin_dict, g_strdup("preset_name"), 
							ghb_value_dup(gval));
		}
		gval = ghb_dict_lookup(mac_dict, "PresetDescription");
		if (gval)
		{
			ghb_dict_insert(lin_dict, g_strdup("preset_description"), 
							ghb_value_dup(gval));
		}
		gval = ghb_dict_lookup(mac_dict, "Folder");
		if (gval && ghb_value_boolean(gval))
		{ // Folder
			GValue *mval, *lval;

			mval = ghb_dict_lookup(mac_dict, "ChildrenArray");
			lval = ghb_array_value_new(32);
			ghb_dict_insert(lin_dict, g_strdup("preset_folder"), lval);
			ghb_dict_insert(lin_dict, g_strdup("preset_type"), 
							ghb_int_value_new(2));
			parse_preset_array(mval, lval);
		}
		else
		{ // Normal preset
			ghb_dict_insert(lin_dict, g_strdup("preset_type"), 
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

