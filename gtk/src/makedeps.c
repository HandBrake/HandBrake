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
	{"vquality_type_bitrate", "VideoAvgBitrate", "TRUE", FALSE},
	{"vquality_type_target", "VideoTargetSize", "TRUE", FALSE},
	{"vquality_type_constant", "VideoQualitySlider", "TRUE", FALSE},
	{"vquality_type_constant", "constant_rate_factor", "TRUE", FALSE},
	{"vquality_type_constant", "VideoTwoPass", "TRUE", TRUE},
	{"vquality_type_constant", "VideoTurboTwoPass", "TRUE", TRUE},
	{"VideoTwoPass", "VideoTurboTwoPass", "TRUE", FALSE},
	{"FileFormat", "Mp4LargeFile", "mp4|m4v", FALSE},
	{"FileFormat", "Mp4HttpOptimize", "mp4|m4v", FALSE},
	{"FileFormat", "Mp4iPodCompatible", "mp4|m4v", FALSE},
	{"PictureDecomb", "PictureDeinterlace", "TRUE", TRUE},
	{"PictureDecomb", "tweak_PictureDeinterlace", "TRUE", TRUE},
	{"PictureAutoCrop", "PictureTopCrop", "FALSE", FALSE},
	{"PictureAutoCrop", "PictureBottomCrop", "FALSE", FALSE},
	{"PictureAutoCrop", "PictureLeftCrop", "FALSE", FALSE},
	{"PictureAutoCrop", "PictureRightCrop", "FALSE", FALSE},
	{"autoscale", "scale_width", "FALSE", FALSE},
	{"autoscale", "scale_height", "FALSE", FALSE},
	{"anamorphic", "PictureKeepRatio", "FALSE", FALSE},
	{"anamorphic", "scale_height", "FALSE", FALSE},
	{"PictureKeepRatio", "scale_height", "FALSE", FALSE},
	{"VideoEncoder", "x264_tab", "x264", FALSE},
	{"VideoEncoder", "x264_tab_label", "x264", FALSE},
	{"VideoEncoder", "Mp4iPodCompatible", "x264", FALSE},
	{"VideoEncoder", "directqp", "x264|ffmpeg", FALSE},
	{"AudioEncoder", "AudioBitrate", "ac3", TRUE},
	{"AudioEncoder", "AudioSamplerate", "ac3", TRUE},
	{"AudioEncoder", "AudioMixdown", "ac3", TRUE},
	{"AudioEncoder", "AudioTrackDRCSlider", "ac3", TRUE},
	{"x264_bframes", "x264_weighted_bframes", "0", TRUE},
	{"x264_bframes", "x264_bpyramid", "<2", TRUE},
	{"x264_bframes", "x264_direct", "0", TRUE},
	{"x264_refs", "x264_mixed_refs", "<2", TRUE},
	{"x264_cabac", "x264_trellis", "TRUE", FALSE},
	{"x264_me", "x264_merange", "umh|esa", FALSE},
	{"ChapterMarkers", "chapters_list", "TRUE", FALSE},
	{"use_source_name", "chapters_in_destination", "TRUE", FALSE},
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

