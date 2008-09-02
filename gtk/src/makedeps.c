#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <string.h>
#include "values.h"
#include "plist.h"

typedef struct
{
	const gchar *widget_name;
	const gchar *dep_name;
	const gchar *enable_value;
	const gboolean disable_if_equal;
} dependency_t;

static dependency_t dep_map[] =
{
	{"title", "queue_add", "none", TRUE},
	{"title", "queue_add_menu", "none", TRUE},
	{"title", "preview_button", "none", TRUE},
	{"title", "show_preview_menu", "none", TRUE},
	{"title", "preview_frame", "none", TRUE},
	{"title", "picture_label", "none", TRUE},
	{"title", "picture_tab", "none", TRUE},
	{"title", "chapters_label", "none", TRUE},
	{"title", "chapters_tab", "none", TRUE},
	{"title", "title", "none", TRUE},
	{"title", "start_chapter", "none", TRUE},
	{"title", "end_chapter", "none", TRUE},
	{"vquality_type_bitrate", "video_bitrate", "TRUE", FALSE},
	{"vquality_type_target", "video_target_size", "TRUE", FALSE},
	{"vquality_type_constant", "video_quality", "TRUE", FALSE},
	{"vquality_type_constant", "constant_rate_factor", "TRUE", FALSE},
	{"vquality_type_constant", "two_pass", "TRUE", TRUE},
	{"vquality_type_constant", "turbo", "TRUE", TRUE},
	{"two_pass", "turbo", "TRUE", FALSE},
	{"container", "large_mp4", "mp4|m4v", FALSE},
	{"container", "http_optimize_mp4", "mp4|m4v", FALSE},
	{"container", "ipod_file", "mp4|m4v", FALSE},
	{"container", "variable_frame_rate", "avi", TRUE},
	{"variable_frame_rate", "framerate", "TRUE", TRUE},
	{"variable_frame_rate", "detelecine", "TRUE", TRUE},
	{"decomb", "deinterlace", "TRUE", TRUE},
	{"decomb", "tweak_deinterlace", "TRUE", TRUE},
	{"autocrop", "crop_top", "FALSE", FALSE},
	{"autocrop", "crop_bottom", "FALSE", FALSE},
	{"autocrop", "crop_left", "FALSE", FALSE},
	{"autocrop", "crop_right", "FALSE", FALSE},
	{"autoscale", "scale_width", "FALSE", FALSE},
	{"autoscale", "scale_height", "FALSE", FALSE},
	{"anamorphic", "keep_aspect", "FALSE", FALSE},
	{"anamorphic", "scale_height", "FALSE", FALSE},
	{"keep_aspect", "scale_height", "FALSE", FALSE},
	{"video_codec", "x264_tab", "x264", FALSE},
	{"video_codec", "x264_tab_label", "x264", FALSE},
	{"video_codec", "ipod_file", "x264", FALSE},
	{"audio_codec", "audio_bitrate", "ac3", TRUE},
	{"audio_codec", "audio_rate", "ac3", TRUE},
	{"audio_codec", "audio_mix", "ac3", TRUE},
	{"audio_codec", "audio_drc", "ac3", TRUE},
	{"x264_bframes", "x264_weighted_bframes", "0", TRUE},
	{"x264_bframes", "x264_brdo", "0", TRUE},
	{"x264_bframes", "x264_bime", "0", TRUE},
	{"x264_bframes", "x264_bpyramid", "<2", TRUE},
	{"x264_bframes", "x264_direct", "0", TRUE},
	{"x264_refs", "x264_mixed_refs", "<2", TRUE},
	{"x264_cabac", "x264_trellis", "TRUE", FALSE},
	{"x264_subme", "x264_brdo", "<6", TRUE},
	{"x264_analyse", "x264_direct", "none", TRUE},
	{"x264_me", "x264_merange", "umh|esa", FALSE},
	{"chapter_markers", "chapters_list", "TRUE", FALSE},
};

int
main(gint argc, gchar *argv[])
{
	gint ii, jj;
	GValue *top;
	gint count = sizeof(dep_map) / sizeof(dependency_t);

	g_type_init();

	top = ghb_dict_value_new();
	for (ii = 0; ii < count; ii++)
	{
		const gchar *name;
		GValue *array;

		name = dep_map[ii].widget_name;
		if (ghb_dict_lookup(top, name))
			continue;
		array = ghb_array_value_new(8);
		for (jj = 0; jj < count; jj++)
		{
			if (strcmp(name, dep_map[jj].widget_name) == 0)
			{
				ghb_array_append(array,
					ghb_value_dup(ghb_string_value(dep_map[jj].dep_name)));
			}
		}
		ghb_dict_insert(top, g_strdup(name), array);
	}
	ghb_plist_write_file("widget_deps", top);

	// reverse map
	top = ghb_dict_value_new();
	for (ii = 0; ii < count; ii++)
	{
		const gchar *name;
		GValue *array;

		name = dep_map[ii].dep_name;
		if (ghb_dict_lookup(top, name))
			continue;
		array = ghb_array_value_new(8);
		for (jj = 0; jj < count; jj++)
		{
			if (strcmp(name, dep_map[jj].dep_name) == 0)
			{
				GValue *data;
				data = ghb_array_value_new(3);
				ghb_array_append(data, ghb_value_dup(
					ghb_string_value(dep_map[jj].widget_name)));
				ghb_array_append(data, ghb_value_dup(
					ghb_string_value(dep_map[jj].enable_value)));
				ghb_array_append(data, ghb_value_dup(
					ghb_boolean_value(dep_map[jj].disable_if_equal)));
				ghb_array_append(array, data);
			}
		}
		ghb_dict_insert(top, g_strdup(name), array);
	}
	ghb_plist_write_file("widget_reverse_deps", top);
	return 0;
}

