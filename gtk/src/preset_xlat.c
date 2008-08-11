#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <fcntl.h>

#define BUF_SIZ	(128*1024)
#define IS_TAG(a,b)	(strcmp((a),(b)) == 0)
#define IS_KEY(a,b)	(strcmp((a),(b)) == 0)
#define IS_VAL(a,b)	(strcmp((a),(b)) == 0)

enum
{
	NONE,
	START,
	ARRAY,
	DICT,
	KEY,
	INT,
	STR,
	REAL,
};

typedef struct
{
	gint state;
	gchar *preset;
	gchar *key;
	gchar *value;
	GHashTable *settings;
	GHashTable *xlat_key;
	GHashTable *xlat_value;
} parse_data_t;

static void
start_element(
	GMarkupParseContext *ctx, 
	const gchar *name, 
	const gchar **attr_names,
	const gchar **attr_values,
	gpointer ud,
	GError **error)
{
	parse_data_t *pd = (parse_data_t*)ud;

	if (IS_TAG(name, "array"))
	{
		pd->state = ARRAY;
	}
	else if (IS_TAG(name, "dict"))
	{
		g_hash_table_remove_all(pd->settings);
		pd->state = DICT;
	}
	else if (IS_TAG(name, "key"))
	{
		pd->state = KEY;
	}
	else if (IS_TAG(name, "string"))
	{
		pd->state = STR;
	}
	else if (IS_TAG(name, "integer"))
	{
		pd->state = INT;
	}
	else if (IS_TAG(name, "real"))
	{
		pd->state = REAL;
	}
	else
	{
		g_debug("start unrecognized (%s)", name);
	}
}

gchar *settings[] = 
{
	"preset_description",
	"subtitle_lang",
	"forced_subtitles",
	"source_audio_lang",
	"pref_audio_codec",
	"pref_audio_bitrate",
	"pref_audio_rate",
	"pref_audio_mix",
	"pref_audio_drc",
	"chapter_markers",
	"container",
	"ipod_file",
	"large_mp4",
	"autocrop",
	"autoscale",
	"max_width",
	"max_height",
	"anamorphic",
	"round_dimensions",
	"keep_aspect",
	"detelecine",
	"decomb",
	"deinterlace",
	"denoise",
	"grayscale",
	"deblock",
	"video_codec",
	"two_pass",
	"turbo",
	"constant_rate_factor",
	"variable_frame_rate",
	"framerate",
	"vquality_type_constant",
	"vquality_type_bitrate",
	"vquality_type_target",
	"video_bitrate",
	"video_target_size",
	"video_quality",
	"x264_options",
	"directqp",
	NULL
};

static void
verify_keys(parse_data_t *pd)
{
	GList *keys, *link;

	link = keys = g_hash_table_get_keys(pd->settings);
	while (link)
	{
		gboolean found = FALSE;
		gchar *key = (gchar*)link->data;
		gint ii;
		for (ii = 0; settings[ii] != NULL; ii++)
		{
			if (IS_KEY(settings[ii], key))
			{
				found = TRUE;
			}
		}
		if (!found)
		{
			g_message("bad key (%s)", key);
		}
		link = link->next;
	}
	g_list_free(keys);
}

GKeyFile *presets;

static void
save_preset(parse_data_t *pd)
{
	gint ii;
	if (pd->preset == NULL)
	{
		g_message("failed to save preset");
		return;
	}
	for (ii = 0; settings[ii] != NULL; ii++)
	{
		const gchar *value;
		value = (const gchar*)g_hash_table_lookup( pd->settings, settings[ii]);
		if (value)
		{
			g_key_file_set_value(presets, pd->preset, settings[ii], value);
		}
	}
	verify_keys(pd);
}

gchar *audio_track[2];
gchar *audio_enc[2];
gchar *audio_bitrate[2];
gchar *audio_rate[2];
gchar *audio_mix[2];
gchar *audio_drc[2];

static void
do_one(gchar **strs, GString *res)
{
	gint ii;
	for (ii = 0; ii < 2 && strs[ii]; ii++)
	{
		if (audio_track[ii] == NULL) break;
		if (ii)
			g_string_append_c(res, ',');
		g_string_append_printf(res, "%s", strs[ii]);
	}
}

static void
do_audio(parse_data_t *pd)
{
	gint ii;
	GString *enc, *br, *rate, *mix, *drc;
	gchar *res;

	enc = g_string_new("");
	br = g_string_new("");
	rate = g_string_new("");
	mix = g_string_new("");
	drc = g_string_new("");
	do_one(audio_enc, enc);
	do_one(audio_bitrate, br);
	do_one(audio_rate, rate);
	do_one(audio_mix, mix);
	do_one(audio_drc, drc);
	res = g_string_free(enc, FALSE);
	g_hash_table_insert(pd->settings, g_strdup("pref_audio_codec"), res);
	res = g_string_free(br, FALSE);
	g_hash_table_insert(pd->settings, g_strdup("pref_audio_bitrate"), res);
	res = g_string_free(rate, FALSE);
	g_hash_table_insert(pd->settings, g_strdup("pref_audio_rate"), res);
	res = g_string_free(mix, FALSE);
	g_hash_table_insert(pd->settings, g_strdup("pref_audio_mix"), res);
	res = g_string_free(drc, FALSE);
	g_hash_table_insert(pd->settings, g_strdup("pref_audio_drc"), res);
}

static void
null_audio()
{
	gint ii;
	for (ii = 0; ii < 2; ii++)
	{
		audio_track[ii] = NULL;
		audio_enc[ii] = NULL;
		audio_bitrate[ii] = NULL;
		audio_rate[ii] = NULL;
		audio_mix[ii] = NULL;
		audio_drc[ii] = NULL;
	}
}

static void
clear_audio()
{
	gint ii;
	for (ii = 0; ii < 2; ii++)
	{
		if (audio_track[ii]) g_free(audio_track[ii]);
		if (audio_enc[ii]) g_free(audio_enc[ii]);
		if (audio_bitrate[ii]) g_free(audio_bitrate[ii]);
		if (audio_rate[ii]) g_free(audio_rate[ii]);
		if (audio_mix[ii]) g_free(audio_mix[ii]);
		if (audio_drc[ii]) g_free(audio_drc[ii]);
		audio_track[ii] = NULL;
		audio_enc[ii] = NULL;
		audio_bitrate[ii] = NULL;
		audio_rate[ii] = NULL;
		audio_mix[ii] = NULL;
		audio_drc[ii] = NULL;
	}
}

static void
end_element(
	GMarkupParseContext *ctx, 
	const gchar *name, 
	gpointer ud,
	GError **error)
{
	parse_data_t *pd = (parse_data_t*)ud;

	if (IS_TAG(name, "string") ||
		IS_TAG(name, "integer") ||
		IS_TAG(name, "real"))
	{
		if (IS_KEY(pd->key, "PresetName"))
		{
			if (pd->preset)
			{
				g_message("Preset named twice");
			}
			else
				pd->preset = g_strdup(pd->value);
			pd->state = NONE;
			return;
		}
		const gchar *my_key;
		my_key = (const gchar*)g_hash_table_lookup(pd->xlat_key, pd->key);
		if (my_key != NULL)
		{ // Do something with it
			if (my_key[0] != 0) // intentionally ignored keys
			{
				if (pd->value != NULL)
				{
					g_hash_table_insert(pd->settings, 
						g_strdup(my_key), g_strdup(pd->value));
				}
				else
				{
					g_message("NULL value");
				}
			}
		}
		else if (IS_KEY(pd->key, "Audio1Encoder"))
		{
			if (audio_enc[0]) g_free(audio_enc[0]);
			audio_enc[0] = g_strdup(pd->value);
		}
		else if (IS_KEY(pd->key, "Audio1Bitrate"))
		{
			if (audio_bitrate[0]) g_free(audio_bitrate[0]);
			audio_bitrate[0] = g_strdup(pd->value);
		}
		else if (IS_KEY(pd->key, "Audio1Samplerate"))
		{
			if (audio_rate[0]) g_free(audio_rate[0]);
			audio_rate[0] = g_strdup(pd->value);
		}
		else if (IS_KEY(pd->key, "Audio1Mixdown"))
		{
			if (audio_mix[0]) g_free(audio_mix[0]);
			audio_mix[0] = g_strdup(pd->value);
		}
		else if (IS_KEY(pd->key, "Audio1TrackDRCSlider"))
		{
			if (audio_drc[0]) g_free(audio_drc[0]);
			audio_drc[0] = g_strdup(pd->value);
		}
		else if (IS_KEY(pd->key, "Audio1Track"))
		{
			if (audio_track[0]) g_free(audio_track[0]);
			audio_track[0] = g_strdup(pd->value);
		}
		else if (IS_KEY(pd->key, "Audio2Encoder"))
		{
			if (audio_enc[1]) g_free(audio_enc[1]);
			audio_enc[1] = g_strdup(pd->value);
		}
		else if (IS_KEY(pd->key, "Audio2Bitrate"))
		{
			if (audio_bitrate[1]) g_free(audio_bitrate[1]);
			audio_bitrate[1] = g_strdup(pd->value);
		}
		else if (IS_KEY(pd->key, "Audio2Samplerate"))
		{
			if (audio_rate[1]) g_free(audio_rate[1]);
			audio_rate[1] = g_strdup(pd->value);
		}
		else if (IS_KEY(pd->key, "Audio2Mixdown"))
		{
			if (audio_mix[1]) g_free(audio_mix[1]);
			audio_mix[1] = g_strdup(pd->value);
		}
		else if (IS_KEY(pd->key, "Audio2TrackDRCSlider"))
		{
			if (audio_drc[1]) g_free(audio_drc[1]);
			audio_drc[1] = g_strdup(pd->value);
		}
		else if (IS_KEY(pd->key, "Audio2Track"))
		{
			if (audio_track[1]) g_free(audio_track[1]);
			audio_track[1] = g_strdup(pd->value);
		}
		else if (IS_KEY(pd->key, "VideoQualityType"))
		{
			// VideoQualityType/0/1/2 - vquality_type_/target/bitrate/constant
			if (IS_VAL(pd->value, "0"))
			{
				g_hash_table_insert(pd->settings, 
						g_strdup("vquality_type_target"), 
						g_strdup("1"));
				g_hash_table_remove(pd->settings, "vquality_type_bitrate");
				g_hash_table_remove(pd->settings, "vquality_type_constant");
			}
			else if (IS_VAL(pd->value, "1"))
			{
				g_hash_table_remove(pd->settings, "vquality_type_target");
				g_hash_table_insert(pd->settings, 
						g_strdup("vquality_type_bitrate"), 
						g_strdup("1"));
				g_hash_table_remove(pd->settings, "vquality_type_constant");
			}
			else if (IS_VAL(pd->value, "2"))
			{
				g_hash_table_remove(pd->settings, "vquality_type_target");
				g_hash_table_remove(pd->settings, "vquality_type_bitrate");
				g_hash_table_insert(pd->settings, 
						g_strdup("vquality_type_constant"), 
						g_strdup("1"));
			}
		}
		else
		{
			g_message("Key not found (%s)", pd->key);
		}
	}
	else if (IS_TAG(name, "dict"))
	{
		gint ii;
		do_audio(pd);
		clear_audio();
		save_preset(pd);

		if (pd->preset) 
		{
			g_free(pd->preset);
			pd->preset = NULL;
		}
		else
			g_message("Preset has no name");
		g_hash_table_remove_all(pd->settings);
	}
	pd->state = NONE;
}

static gboolean
is_number(const gchar *str)
{
	gboolean result = TRUE;
	gint ii;
	for (ii = 0; str[ii] != 0; ii++)
	{
		if (!g_ascii_isdigit(str[ii]) && str[ii] != '.')
			result = FALSE;
	}
	return result;
}

static void
text_data(
	GMarkupParseContext *ctx, 
	const gchar *text, 
	gsize len,
	gpointer ud,
	GError **error)
{
	gboolean is_value = FALSE;
	parse_data_t *pd = (parse_data_t*)ud;
	const gchar *val = NULL;

	if (pd->state == KEY)
	{
		if (pd->key) g_free(pd->key);
		pd->key = g_strdup(text);
		return;
	}
	if (pd->state == STR)
	{
		val = (gchar*)g_hash_table_lookup(pd->xlat_value, text);
		if (val != NULL)
		{ // Do something with it
		}
		else if (IS_KEY(pd->key, "PresetName") ||
				IS_KEY(pd->key, "PresetDescription") ||
				IS_KEY(pd->key, "x264Option") ||
				is_number(text))
		{
			val = text;
		}
		else
		{
			g_message("Unrecognized val (%s)", text);
		}
	}
	if (pd->state == INT || pd->state == REAL)
	{
		val = text;
	}

	// Some keys need further translation of their values
	if (val)
	{
		if (IS_KEY(pd->key, "PictureDeinterlace"))
		{
			if (IS_VAL(val, "0"))
			{
				val = "none";
			}
			else if (IS_VAL(val, "1"))
			{
				val = "fast";
			}
			else if (IS_VAL(val, "2"))
			{
				val = "slow";
			}
			else if (IS_VAL(val, "3"))
			{
				val = "slower";
			}
		}
		else if (IS_KEY(pd->key, "Audio1Samplerate") ||
				IS_KEY(pd->key, "Audio2Samplerate"))
		{
			if (IS_VAL(val, "auto"))
			{
				val = "source";
			}
		}
		else if (IS_KEY(pd->key, "Audio1Mixdown") ||
				IS_KEY(pd->key, "Audio2Mixdown"))
		{
			if (IS_VAL(val, "ac3"))
			{
				val = "none";
			}
		}
		if (pd->value) g_free(pd->value);
		pd->value = g_strdup(val);
	}
}

static void
passthrough(
	GMarkupParseContext *ctx, 
	const gchar *text, 
	gsize len,
	gpointer ud,
	GError **error)
{
	parse_data_t *pd = (parse_data_t*)ud;

	g_debug("passthrough %s", text);
}

static void
delete_key(gpointer str)
{
	g_free(str);
}

static void
delete_value(gpointer str)
{
	g_free(str);
}

typedef struct
{
	gchar *from;
	gchar *to;
} xlat_t;

static xlat_t keys[] =
{
	{"VFR", "variable_frame_rate"},
	{"ChapterMarkers", "chapter_markers"},
	{"Default", ""},
	{"FileFormat", "container"},
	{"PictureAutoCrop", "autocrop"},
	{"PictureBottomCrop", ""},
	{"PictureTopCrop", ""},
	{"PictureLeftCrop", ""},
	{"PictureRightCrop", ""},
	{"PictureDeblock", "deblock"},
	{"PictureDeinterlace", "deinterlace"}, // v
	{"PictureDenoise", "denoise"}, // v
	{"PictureDetelecine", "detelecine"},
	{"PictureHeight", "max_height"},
	{"PictureWidth", "max_width"},
	{"PictureKeepRatio", "keep_aspect"},
	{"PicturePAR", "anamorphic"}, // v
	{"PresetDescription", "preset_description"},
	{"Subtitles", "subtitle_lang"},
	{"Subtitles", "subtitle_lang"},
	{"Type", ""}, // preset type builtin/custom
	{"UsesMaxPictureSettings", "autoscale"},
	{"UsesPictureFilters", ""},
	{"UsesPictureSettings", ""},
	{"VideoAvgBitrate", "video_bitrate"},
	{"VideoEncoder", "video_codec"},
	{"VideoFramerate", "framerate"},
	{"VideoGrayScale", "grayscale"},
	{"VideoQualitySlider", "video_quality"},
	{"VideoTargetSize", "video_target_size"},
	{"VideoTurboTwoPass", "turbo"},
	{"VideoTwoPass", "two_pass"},
	{"x264Option", "x264_options"},
	{"Mp4LargeFile", "large_mp4"},
	{"Mp4iPodCompatible", "ipod_file"},
	{NULL, NULL}
};

// VideoQualityType/0/1/2 - vquality_type_/target/bitrate/constant
// Audio1Bitrate - pref_audio_bitrate
// Audio1Encoder - pref_audio_codec
// Audio1Mixdown - pref_audio_mix
// Audio1Samplerate - pref_audio_rate
// Audio1Track - na
// Audio1DRCSlider - pref_audio_drc

static xlat_t values[] =
{
	{"AAC (faac)", "faac"},
	{"AC3 Passthru", "ac3"},
	{"H.264 (x264)", "x264"},
	{"MPEG-4 (FFmpeg)", "x264"},
	{"Dolby Pro Logic II", "dpl2"},
	{"Auto", "auto"},
	{"MKV file", "mkv"},
	{"MP4 file", "mp4"},
	{"None", "none"},
	{"Same as source", "source"},
	{"160", "160"},
	{NULL, NULL}
};

static void
store_key_file(GKeyFile *key_file, const gchar *name)
{
    gchar *settingsString;
    gsize length;
    gint fd;

    settingsString = g_key_file_to_data(key_file, &length, NULL);

    fd = g_open(name, O_RDWR|O_CREAT|O_TRUNC, 0777);
    write(fd, settingsString, length);
    close(fd);
    g_free(settingsString);
}

static void
parse_it(gchar *buf, gssize len)
{
	GMarkupParseContext *ctx;
	GMarkupParser parser;
	parse_data_t pd;

	null_audio();
	presets = g_key_file_new();
	pd.state = START;
	pd.key = NULL;
	pd.value = NULL;
	pd.preset = NULL;
	pd.settings = g_hash_table_new_full(g_str_hash, g_str_equal, 
									  delete_key, delete_value);
	pd.xlat_key = g_hash_table_new(g_str_hash, g_str_equal);
	gint ii;
	for (ii = 0; keys[ii].from != NULL; ii++)
	{
		g_hash_table_insert(pd.xlat_key, keys[ii].from, keys[ii].to);
	}
	pd.xlat_value = g_hash_table_new(g_str_hash, g_str_equal);
	for (ii = 0; values[ii].from != NULL; ii++)
	{
		g_hash_table_insert(pd.xlat_value, values[ii].from, values[ii].to);
	}
	parser.start_element = start_element;
	parser.end_element = end_element;
	parser.text = text_data;
	parser.passthrough = passthrough;
	ctx = g_markup_parse_context_new(&parser, 0, &pd, NULL);
	g_markup_parse_context_parse(ctx, buf, len, NULL);
	store_key_file(presets, "xlat_presets");
}

gint
main(gint argc, gchar *argv[])
{
	FILE *fd;
	gchar buffer[BUF_SIZ];
	size_t size;

	fd = fopen(argv[1], "r");
	size = fread(buffer, 1, BUF_SIZ, fd);
	if (size >= BUF_SIZ)
	{
		g_error("buffer too small");
		exit(1);
	}
	buffer[size] = 0;
	parse_it(buffer, (gssize)size);
}

