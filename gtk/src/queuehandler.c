/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * queuehandler.c
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * queuehandler.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * queuehandler.c is distributed in the hope that it will be useful,
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

#include "ghbcompat.h"
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "handbrake/handbrake.h"
#include "settings.h"
#include "jobdict.h"
#include "titledict.h"
#include "hb-backend.h"
#include "values.h"
#include "callbacks.h"
#include "presets.h"
#include "audiohandler.h"
#include "subtitlehandler.h"
#include "ghb-dvd.h"
#include "plist.h"
#include "queuehandler.h"

void ghb_queue_buttons_grey(signal_user_data_t *ud);

// Callbacks
G_MODULE_EXPORT void
queue_remove_clicked_cb(GtkWidget *widget, signal_user_data_t *ud);

#if GTK_CHECK_VERSION(3, 90, 0)
G_MODULE_EXPORT void
queue_drag_begin_cb(GtkWidget * widget, GdkDrag * context,
                    signal_user_data_t * ud);
G_MODULE_EXPORT void
queue_drag_end_cb(GtkWidget * widget, GdkDrag * context,
                  signal_user_data_t * ud);
G_MODULE_EXPORT void
queue_drag_data_get_cb(GtkWidget * widget, GdkDrag * context,
                       GtkSelectionData * selection_data,
                       signal_user_data_t * ud);

G_MODULE_EXPORT gboolean
queue_row_key_cb(GtkEventControllerKey * keycon, guint keyval,
                 guint keycode, GdkModifierType state,
                 signal_user_data_t * ud);
#else
G_MODULE_EXPORT void
queue_drag_begin_cb(GtkWidget * widget, GdkDragContext * context,
                    signal_user_data_t * ud);
G_MODULE_EXPORT void
queue_drag_end_cb(GtkWidget * widget, GdkDragContext * context,
                  signal_user_data_t * ud);
G_MODULE_EXPORT void
queue_drag_data_get_cb(GtkWidget * widget, GdkDragContext * context,
                       GtkSelectionData * selection_data,
                       guint info, guint time, signal_user_data_t * ud);
#endif
G_MODULE_EXPORT void
title_selected_cb(GtkWidget *widget, signal_user_data_t *ud);
G_MODULE_EXPORT void
title_dest_file_cb(GtkWidget *widget, signal_user_data_t *ud);
G_MODULE_EXPORT void
title_dest_dir_cb(GtkWidget *widget, signal_user_data_t *ud);

#if GTK_CHECK_VERSION(3, 90, 0)
static const char * queue_drag_entries[] = {
    "application/queue-list-row-drop"
};

void
ghb_queue_drag_n_drop_init(signal_user_data_t * ud)
{
    GtkWidget * widget;
    GdkContentFormats * targets;

    widget = GHB_WIDGET(ud->builder, "queue_list");
    targets = gdk_content_formats_new(queue_drag_entries,
                                      G_N_ELEMENTS(queue_drag_entries));
    gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_MOTION|GTK_DEST_DEFAULT_DROP,
                      targets, GDK_ACTION_MOVE);
    gdk_content_formats_unref(targets);
}
#else
static GtkTargetEntry queue_drag_entries[] = {
   { "GTK_LIST_BOX_ROW", GTK_TARGET_SAME_APP, 0 }
};

void
ghb_queue_drag_n_drop_init(signal_user_data_t * ud)
{
    GtkWidget * widget;

    widget = GHB_WIDGET(ud->builder, "queue_list");
    gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_MOTION|GTK_DEST_DEFAULT_DROP,
                      queue_drag_entries, 1, GDK_ACTION_MOVE);
}
#endif

static GtkWidget *find_widget(GtkWidget *widget, gchar *name)
{
    const char *wname;
    GtkWidget *result = NULL;

    if (widget == NULL || name == NULL)
        return NULL;

    wname = gtk_widget_get_name(widget);
    if (wname != NULL && !strncmp(wname, name, 80))
    {
        return widget;
    }
    if (GTK_IS_CONTAINER(widget))
    {
        GList *list, *link;
        link = list = gtk_container_get_children(GTK_CONTAINER(widget));
        while (link)
        {
            result = find_widget(GTK_WIDGET(link->data), name);
            if (result != NULL)
                break;
            link = link->next;
        }
        g_list_free(list);
    }
    return result;
}

static void
queue_update_summary(GhbValue * queueDict, signal_user_data_t *ud)
{
    GString            * str;
    char               * text;
    const char         * ctext;
    const char         * sep;
    GtkWidget          * widget;
    GhbValue           * uiDict     = NULL;
    GhbValue           * jobDict    = NULL;
    GhbValue           * sourceDict = NULL;
    GhbValue           * rangeDict  = NULL;
    GhbValue           * titleDict  = NULL;

    if (queueDict != NULL)
    {
        uiDict    = ghb_dict_get(queueDict, "uiSettings");
        jobDict   = ghb_dict_get(queueDict, "Job");
        titleDict = ghb_dict_get(queueDict, "Title");
    }
    if (titleDict == NULL)
    {
        // No title, clear summary
        widget = GHB_WIDGET(ud->builder, "queue_summary_preset");
        gtk_label_set_text(GTK_LABEL(widget), "");
        widget = GHB_WIDGET(ud->builder, "queue_summary_source");
        gtk_label_set_text(GTK_LABEL(widget), "");
        widget = GHB_WIDGET(ud->builder, "queue_summary_dest");
        gtk_label_set_text(GTK_LABEL(widget), "");
        widget = GHB_WIDGET(ud->builder, "queue_summary_dimensions");
        gtk_label_set_text(GTK_LABEL(widget), "");
        widget = GHB_WIDGET(ud->builder, "queue_summary_video");
        gtk_label_set_text(GTK_LABEL(widget), "");
        widget = GHB_WIDGET(ud->builder, "queue_summary_audio");
        gtk_label_set_text(GTK_LABEL(widget), "");
        widget = GHB_WIDGET(ud->builder, "queue_summary_subtitle");
        gtk_label_set_text(GTK_LABEL(widget), "");
        return;
    }

    // Preset: PresetName
    const char * name;
    gboolean preset_modified;

    preset_modified = ghb_dict_get_bool(uiDict, "preset_modified");
    name = ghb_dict_get_string(uiDict, "PresetFullName");

    str = g_string_new("");
    if (preset_modified)
    {
        g_string_append_printf(str, _("%s (Modified)"), name);
    }
    else
    {
        g_string_append_printf(str, "%s", name);
    }

    widget = GHB_WIDGET(ud->builder, "queue_summary_preset");
    text = g_string_free(str, FALSE);
    gtk_label_set_text(GTK_LABEL(widget), text);
    g_free(text);

    // Source
    sourceDict = ghb_dict_get(jobDict, "Source");
    ctext = ghb_dict_get_string(sourceDict, "Path");
    widget = GHB_WIDGET(ud->builder, "queue_summary_source");
    gtk_label_set_text(GTK_LABEL(widget), ctext);

    // Title
    const char * rangeType;
    int titleID;
    int64_t rangeStart, rangeEnd;

    titleID       = ghb_dict_get_int(sourceDict, "Title");
    rangeDict     = ghb_dict_get(sourceDict, "Range");
    rangeType     = ghb_dict_get_string(rangeDict, "Type");
    rangeStart    = ghb_dict_get_int(rangeDict, "Start");
    rangeEnd      = ghb_dict_get_int(rangeDict, "End");

    str = g_string_new("");
    g_string_append_printf(str, "%-8d", titleID);
    if (!strcmp(rangeType, "chapter"))
    {
        g_string_append_printf(str, "Chapters: %ld to %ld",
                               rangeStart, rangeEnd);
    }
    else if (!strcmp(rangeType, "time"))
    {
        int start_hh, start_mm;
        double start_ss;
        int end_hh, end_mm;
        double end_ss;

        ghb_break_pts_duration(rangeStart, &start_hh, &start_mm, &start_ss);
        ghb_break_pts_duration(rangeEnd, &end_hh, &end_mm, &end_ss);
        g_string_append_printf(str,
                               "Time: %02d:%02d:%05.2f to %02d:%02d:%05.2f",
                                start_hh, start_mm, start_ss,
                                end_hh, end_mm, end_ss);
    }
    else if (!strcmp(rangeType, "frame"))
    {
        g_string_append_printf(str, "Frames: %ld to %ld",
                               rangeStart, rangeEnd);
    }
    text = g_string_free(str, FALSE);
    widget = GHB_WIDGET(ud->builder, "queue_summary_title");
    gtk_label_set_text(GTK_LABEL(widget), text);
    g_free(text);
    

    // Destination
    ctext = ghb_dict_get_string(uiDict, "destination");
    str = g_string_new(ctext);

    const char           * mux_name;
    const hb_container_t * container;

    mux_name  = ghb_dict_get_string(uiDict, "FileFormat");
    container = ghb_lookup_container_by_name(mux_name);
    g_string_append_printf(str, "\n%s", container->name);

    gboolean markers, av_align, ipod = FALSE, http = FALSE;
    markers  = ghb_dict_get_bool(uiDict, "ChapterMarkers");
    av_align = ghb_dict_get_bool(uiDict, "AlignAVStart");
    if (container->format & HB_MUX_MASK_MP4)
    {
        ipod = ghb_dict_get_bool(uiDict, "Mp4iPodCompatible");
        http = ghb_dict_get_bool(uiDict, "Mp4HttpOptimize");
    }

    sep = "\n";
    if (markers)
    {
        g_string_append_printf(str, "%s%s", sep, _("Chapter Markers"));
        sep = ", ";
    }
    if (av_align)
    {
        g_string_append_printf(str, "%s%s", sep, _("Align A/V"));
        sep = ", ";
    }
    if (http)
    {
        g_string_append_printf(str, "%s%s", sep, _("Web Optimized"));
        sep = ", ";
    }
    if (ipod)
    {
        g_string_append_printf(str, "%s%s", sep, _("iPod 5G"));
        sep = ", ";
    }

    text = g_string_free(str, FALSE);
    widget = GHB_WIDGET(ud->builder, "queue_summary_dest");
    gtk_label_set_text(GTK_LABEL(widget), text);
    g_free(text);

    // Dimensions
    double display_width;
    int    width, height, display_height, par_width, par_height;
    int    crop[4];
    char * display_aspect;

    width          = ghb_dict_get_int(uiDict, "scale_width");
    height         = ghb_dict_get_int(uiDict, "scale_height");
    display_width  = ghb_dict_get_int(uiDict, "PictureDARWidth");
    display_height = ghb_dict_get_int(uiDict, "DisplayHeight");
    par_width      = ghb_dict_get_int(uiDict, "PicturePARWidth");
    par_height     = ghb_dict_get_int(uiDict, "PicturePARHeight");
    crop[0]        = ghb_dict_get_int(uiDict, "PictureTopCrop");
    crop[1]        = ghb_dict_get_int(uiDict, "PictureBottomCrop");
    crop[2]        = ghb_dict_get_int(uiDict, "PictureLeftCrop");
    crop[3]        = ghb_dict_get_int(uiDict, "PictureRightCrop");


    display_width = (double)width * par_width / par_height;
    display_aspect = ghb_get_display_aspect_string(display_width,
                                                   display_height);

    display_width  = ghb_dict_get_int(uiDict, "PictureDARWidth");
    text = g_strdup_printf(_("%d:%d:%d:%d Crop\n"
                             "%dx%d storage, %dx%d display\n"
                             "%d:%d Pixel Aspect Ratio\n"
                             "%s Display Aspect Ratio"),
                           crop[0], crop[1], crop[2], crop[3],
                           width, height, (int)display_width, display_height,
                           par_width, par_height, display_aspect);
    widget = GHB_WIDGET(ud->builder, "queue_summary_dimensions");
    gtk_label_set_text(GTK_LABEL(widget), text);

    g_free(text);
    g_free(display_aspect);

    // Video Track
    const hb_encoder_t * video_encoder;
    const hb_rate_t    * fps;
    hb_rational_t        vrate;
    char               * rate_str;
    gboolean             two_pass, vqtype;

    str = g_string_new("");
    video_encoder = ghb_settings_video_encoder(uiDict, "VideoEncoder");
    vqtype = ghb_dict_get_bool(uiDict, "vquality_type_constant");
    two_pass = ghb_dict_get_bool(uiDict, "VideoTwoPass");

    if (!vqtype)
    {
        // ABR
        int br = ghb_dict_get_int(uiDict, "VideoAvgBitrate");
        if (!two_pass)
        {
            g_string_append_printf(str, _("%s, Bitrate %dkbps"),
                                   video_encoder->name, br);
        }
        else
        {
            g_string_append_printf(str, _("%s, Bitrate %dkbps (2 Pass)"),
                                   video_encoder->name, br);
        }
    }
    else
    {
        gdouble quality = ghb_dict_get_double(uiDict, "VideoQualitySlider");
        g_string_append_printf(str, _("%s, Constant Quality %.4g(%s)"),
                               video_encoder->name, quality,
                               hb_video_quality_get_name(video_encoder->codec));
    }
    const char * enc_preset  = NULL;
    const char * enc_tune    = NULL;
    const char * enc_level   = NULL;
    const char * enc_profile = NULL;

    if (hb_video_encoder_get_presets(video_encoder->codec) != NULL)
    {
        // The encoder supports presets
        enc_preset  = ghb_dict_get_string(uiDict, "VideoPreset");
    }
    if (hb_video_encoder_get_tunes(video_encoder->codec) != NULL)
    {
        // The encoder supports tunes
        enc_tune    = ghb_dict_get_string(uiDict, "VideoTune");
    }
    if (hb_video_encoder_get_profiles(video_encoder->codec) != NULL)
    {
        // The encoder supports profiles
        enc_profile = ghb_dict_get_string(uiDict, "VideoProfile");
    }
    if (hb_video_encoder_get_levels(video_encoder->codec) != NULL)
    {
        // The encoder supports levels
        enc_level   = ghb_dict_get_string(uiDict, "VideoLevel");
    }

    sep = "\n";
    if (enc_preset != NULL)
    {
        g_string_append_printf(str, _("%sPreset %s"), sep, enc_preset);
        sep = ", ";
    }
    if (enc_tune != NULL)
    {
        g_string_append_printf(str, _("%sTune %s"), sep, enc_tune);
        sep = ", ";
    }
    if (enc_profile != NULL)
    {
        g_string_append_printf(str, _("%sProfile %s"), sep, enc_profile);
        sep = ", ";
    }
    if (enc_level != NULL)
    {
        g_string_append_printf(str, _("%sLevel %s"), sep, enc_level);
        sep = ", ";
    }

    fps = ghb_settings_video_framerate(uiDict, "VideoFramerate");
    if (fps->rate == 0)
    {
        hb_dict_extract_rational(&vrate, titleDict, "FrameRate");
    }
    else
    {
        vrate.num = 27000000;
        vrate.den = fps->rate;
    }
    rate_str = g_strdup_printf("%.6g", (gdouble)vrate.num / vrate.den);
    if (ghb_dict_get_bool(uiDict, "VideoFramerateCFR"))
    {
        g_string_append_printf(str, _("\nConstant Framerate %s fps"), rate_str);
    }
    else if (ghb_dict_get_bool(uiDict, "VideoFrameratePFR"))
    {
        g_string_append_printf(str, _("\nPeak Framerate %s fps (may be lower)"),
                               rate_str);
    }
    else if (ghb_dict_get_bool(uiDict, "VideoFramerateVFR"))
    {
        g_string_append_printf(str, _("\nVariable Framerate %s fps"), rate_str);
    }
    g_free(rate_str);

    // Append Filters to video summary
    gboolean detel, comb_detect, yadif, decomb, deblock, nlmeans, denoise, bwdif;
    gboolean unsharp, lapsharp, hflip, rot, gray, colorspace, chroma_smooth;

    ctext       = ghb_dict_get_string(uiDict, "PictureDetelecine");
    detel       = ctext != NULL && !!strcasecmp(ctext, "off");
    ctext       = ghb_dict_get_string(uiDict, "PictureCombDetectPreset");
    comb_detect = ctext != NULL && !!strcasecmp(ctext, "off");
    ctext       = ghb_dict_get_string(uiDict, "PictureDeinterlaceFilter");
    yadif       = ctext != NULL && !strcasecmp(ctext, "deinterlace");
    decomb      = ctext != NULL && !strcasecmp(ctext, "decomb");
    bwdif       = ctext != NULL && !strcasecmp(ctext, "bwdif");
    ctext       = ghb_dict_get_string(uiDict, "PictureDeblockPreset");
    deblock     = ctext != NULL && !!strcasecmp(ctext, "off");
    ctext       = ghb_dict_get_string(uiDict, "PictureDenoiseFilter");
    nlmeans     = ctext != NULL && !strcasecmp(ctext, "nlmeans");
    denoise     = ctext != NULL && !strcasecmp(ctext, "hqdn3d");
    ctext       = ghb_dict_get_string(uiDict, "PictureSharpenFilter");
    unsharp     = ctext != NULL && !strcasecmp(ctext, "unsharp");
    lapsharp    = ctext != NULL && !strcasecmp(ctext, "lapsharp");
    hflip       = ghb_dict_get_bool(uiDict, "hflip");
    ctext       = ghb_dict_get_string(uiDict, "rotate");
    rot         = ctext != NULL && !!strcasecmp(ctext, "0");
    gray        = ghb_dict_get_bool(uiDict, "VideoGrayScale");
    ctext       = ghb_dict_get_string(uiDict, "PictureColorspacePreset");
    colorspace  = ctext != NULL && !!strcasecmp(ctext, "off");
    ctext       = ghb_dict_get_string(uiDict, "PictureChromaSmoothPreset");
    chroma_smooth = ctext != NULL && !!strcasecmp(ctext, "off");

    sep = "\n";
    if (detel)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_DETELECINE);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }
    if (comb_detect)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_COMB_DETECT);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }
    if (yadif)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_YADIF);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }
    if (bwdif)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_BWDIF);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }
    if (decomb)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_DECOMB);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }
    if (deblock)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_DEBLOCK);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }
    if (nlmeans)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_NLMEANS);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }
    if (denoise)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_DENOISE);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }
    if (chroma_smooth)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_CHROMA_SMOOTH);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }
    if (unsharp)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_UNSHARP);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }
    if (lapsharp)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_LAPSHARP);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }
    if (rot || hflip)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_ROTATE);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }
    if (gray)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_GRAYSCALE);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }
    if (colorspace)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_COLORSPACE);
        g_string_append_printf(str, "%s%s", sep, filter->name);
        sep = ", ";
    }

    text = g_string_free(str, FALSE);
    widget = GHB_WIDGET(ud->builder, "queue_summary_video");
    gtk_label_set_text(GTK_LABEL(widget), text);
    g_free(text);

    GhbValue * audioList;
    GhbValue * sourceAudioList;
    int        ii, count;

    sep             = "";
    str             = g_string_new("");
    sourceAudioList = ghb_dict_get(titleDict, "AudioList");
    audioList       = ghb_get_job_audio_list(queueDict);
    count           = ghb_array_len(audioList);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue           * asettings, * asource;
        const hb_mixdown_t * audio_mix;
        const hb_encoder_t * audio_encoder;
        const char         * lang;
        int                  track;

        asettings     = ghb_array_get(audioList, ii);
        track         = ghb_dict_get_int(asettings, "Track");
        asource       = ghb_array_get(sourceAudioList, track);
        lang          = ghb_dict_get_string(asource, "Language");
        audio_encoder = ghb_settings_audio_encoder(asettings, "Encoder");
        if (audio_encoder->codec & HB_ACODEC_PASS_FLAG)
        {
            g_string_append_printf(str, "%s%s, %s", sep, lang,
                                   audio_encoder->name);
        }
        else
        {
            audio_mix = ghb_settings_mixdown(asettings, "Mixdown");
            g_string_append_printf(str, "%s%s, %s, %s", sep, lang,
                                   audio_encoder->name, audio_mix->name);
        }
        sep = "\n";
    }

    text = g_string_free(str, FALSE);
    widget = GHB_WIDGET(ud->builder, "queue_summary_audio");
    gtk_label_set_text(GTK_LABEL(widget), text);
    g_free(text);

    GhbValue * subtitleDict;
    GhbValue * searchDict;
    GhbValue * subtitleList;
    GhbValue * sourceSubtitleList;
    gboolean   search;

    sep                = "";
    str                = g_string_new("");
    sourceSubtitleList = ghb_dict_get(titleDict, "SubtitleList");
    subtitleDict       = ghb_get_job_subtitle_settings(queueDict);
    subtitleList       = ghb_dict_get(subtitleDict, "SubtitleList");
    searchDict         = ghb_dict_get(subtitleDict, "Search");
    search             = ghb_dict_get_bool(searchDict, "Enable");
    count              = ghb_array_len(subtitleList);
    if (search)
    {
        gboolean force, burn, def;

        force = ghb_dict_get_bool(searchDict, "Forced");
        burn  = ghb_dict_get_bool(searchDict, "Burn");
        def   = ghb_dict_get_bool(searchDict, "Default");

        g_string_append_printf(str, _("Foreign Audio Scan"));
        if (force)
        {
            g_string_append_printf(str, _(", Forced Only"));
        }
        if (burn)
        {
            g_string_append_printf(str, _(", Burned"));
        }
        else if (def)
        {
            g_string_append_printf(str, _(", Default"));
        }
        sep = "\n";
    }
    for (ii = 0; ii < count; ii++)
    {
        GhbValue           * subsettings, * subsource;
        int                  track;
        char               * desc;
        gboolean             force, burn, def;

        subsettings = ghb_array_get(subtitleList, ii);
        track       = ghb_dict_get_int(subsettings, "Track");
        subsource   = ghb_array_get(sourceSubtitleList, track);
        desc        = ghb_subtitle_short_description(subsource, subsettings);
        force       = ghb_dict_get_bool(subsettings, "Forced");
        burn        = ghb_dict_get_bool(subsettings, "Burn");
        def         = ghb_dict_get_bool(subsettings, "Default");

        g_string_append_printf(str, "%s%s", sep, desc);
        free(desc);
        if (force)
        {
            g_string_append_printf(str, _(", Forced Only"));
        }
        if (burn)
        {
            g_string_append_printf(str, _(", Burned"));
        }
        else if (def)
        {
            g_string_append_printf(str, _(", Default"));
        }
        sep = "\n";
    }

    text = g_string_free(str, FALSE);
    widget = GHB_WIDGET(ud->builder, "queue_summary_subtitle");
    gtk_label_set_text(GTK_LABEL(widget), text);
    g_free(text);
}

void
queue_update_stats(GhbValue * queueDict, signal_user_data_t *ud)
{
    GhbValue * uiDict;
    GtkLabel * label;

    uiDict = ghb_dict_get(queueDict, "uiSettings");
    if (uiDict == NULL) // should never happen
    {
        return;
    }

    label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_pass_label"));
    gtk_widget_set_visible(GTK_WIDGET(label), FALSE);
    label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_pass"));
    gtk_widget_set_visible(GTK_WIDGET(label), FALSE);

    const char * result = "";
    int status = ghb_dict_get_int(uiDict, "job_status");

    if (status == GHB_QUEUE_PENDING)
    {
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_start_time"));
        gtk_label_set_text(label, "");
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_finish_time"));
        gtk_label_set_text(label, "");
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_paused"));
        gtk_label_set_text(label, "");
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_encode"));
        gtk_label_set_text(label, "");
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_file_size"));
        gtk_label_set_text(label, "");
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_result"));
        gtk_label_set_text(label, "Pending");
        return;
    }

    switch (status)
    {
        case GHB_QUEUE_RUNNING:
            // This job is running.
            // ghb_queue_update_live_stats() will update stats
            return;

        case GHB_QUEUE_DONE:
            result = _("Completed");
            break;

        case GHB_QUEUE_CANCELED:
            result = _("Canceled");
            break;

        case GHB_QUEUE_FAIL:
            result = _("Failed");
            break;

        case GHB_QUEUE_PENDING:
        default:
            result = _("Pending");
            break;
    }

    struct tm  * tm;
    char         date[40] = "";
    char       * str;
    time_t       start, finish, paused, duration;

    start  = ghb_dict_get_int(uiDict, "job_start_time");
    finish = ghb_dict_get_int(uiDict, "job_finish_time");
    paused = ghb_dict_get_int(uiDict, "job_pause_time_ms") / 1000;

    tm     = localtime( &start );
    strftime(date, 40, "%c", tm);
    label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_start_time"));
    gtk_label_set_text(label, date);

    tm  = localtime( &finish );
    strftime(date, 40, "%c", tm);
    label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_finish_time"));
    gtk_label_set_text(label, date);

    int dd = 0, hh, mm, ss;
    ghb_break_duration(paused, &hh, &mm, &ss);
    if (hh >= 24)
    {
        dd = hh / 24;
        hh = hh - dd * 24;
    }
    switch (dd)
    {
        case 0:
            str = g_strdup_printf("%02d:%02d:%02d", hh, mm, ss);
            break;
        case 1:
            str = g_strdup_printf(_("%d Day %02d:%02d:%02d"), dd, hh, mm, ss);
            break;
        default:
            str = g_strdup_printf(_("%d Days %02d:%02d:%02d"), dd, hh, mm, ss);
            break;
    }
    label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_paused"));
    gtk_label_set_text(label, str);
    g_free(str);

    duration = finish - start;
    if (duration < 0)
    {
        duration = 0;
    }
    dd = 0;
    ghb_break_duration(duration, &hh, &mm, &ss);
    if (hh >= 24)
    {
        dd = hh / 24;
        hh = hh - dd * 24;
    }
    switch (dd)
    {
        case 0:
            str = g_strdup_printf("%02d:%02d:%02d", hh, mm, ss);
            break;
        case 1:
            str = g_strdup_printf(_("%d Day %02d:%02d:%02d"), dd, hh, mm, ss);
            break;
        default:
            str = g_strdup_printf(_("%d Days %02d:%02d:%02d"), dd, hh, mm, ss);
            break;
    }
    label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_encode"));
    gtk_label_set_text(label, str);
    g_free(str);

    const char  * path;
    struct stat   stbuf;

    path = ghb_dict_get_string(uiDict, "destination");
    if (g_stat(path, &stbuf) == 0)
    {
        const char * units = _("B");
        double size = stbuf.st_size;
        if (size > 1024)
        {
            units = _("KB");
            size /= 1024.0;
        }
        if (size > 1024)
        {
            units = _("MB");
            size /= 1024.0;
        }
        if (size > 1024)
        {
            units = _("GB");
            size /= 1024.0;
        }
        str = g_strdup_printf("%.2f %s", size, units);
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_file_size"));
        gtk_label_set_text(label, str);
        g_free(str);
    }
    else
    {
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_file_size"));
        gtk_label_set_text(label, _("Not Available"));
    }

    label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_result"));
    gtk_label_set_text(label, result);
}

void queue_update_current_stats(signal_user_data_t * ud)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;
    gint            index;
    GhbValue      * queueDict;

    lb  = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_selected_row(lb);
    if (row != NULL)
    {
        // There is a queue list row selected
        index = gtk_list_box_row_get_index(row);
        if (index < 0 || index >= ghb_array_len(ud->queue))
        { // Should never happen
            return;
        }
        queueDict = ghb_array_get(ud->queue, index);
        queue_update_stats(queueDict, ud);
    }
}

#define ACTIVITY_MAX_READ_SZ (1024*1024)
static void read_log(signal_user_data_t * ud, const char * log_path)
{
    FILE        * f;
    size_t        size, req_size;
    GtkTextIter   iter;
    char        * buf;

    if (ud->extra_activity_path != NULL &&
        !strcmp(ud->extra_activity_path, log_path))
    {
        return;
    }
    g_free(ud->extra_activity_path);
    ud->extra_activity_path = g_strdup(log_path);

    gtk_text_buffer_set_text(ud->extra_activity_buffer, "", 0);

    f = g_fopen(log_path, "r");
    if (f == NULL)
    {
        return;
    }
    fseek(f, 0, SEEK_END);
    req_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (req_size > ACTIVITY_MAX_READ_SZ)
    {
        req_size = ACTIVITY_MAX_READ_SZ;
    }
    buf = g_malloc(req_size);
    while (!feof(f))
    {
        size = fread(buf, 1, req_size, f);
        if (size <= 0)
        {
            break;
        }
        gtk_text_buffer_get_end_iter(ud->extra_activity_buffer, &iter);
        gtk_text_buffer_insert(ud->extra_activity_buffer, &iter, buf, size);
    }
    fclose(f);
    g_free(buf);
}

// Display/Hide queue activity log pane
// Choose buffer to display. Queue logs use 1 of 2 available buffers
//      queue_activity_buffer - live buffer, updated as encode proceeds
//      extra_activity_buffer - non-live buffer, populated from log file
// If showing non-live buffer, read log file into buffer
void ghb_queue_select_log(signal_user_data_t * ud)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;
    GtkTextBuffer * current;
    gint            index;
    GhbValue      * queueDict, *uiDict;

    lb              = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row             = gtk_list_box_get_selected_row(lb);
    if (row != NULL)
    {
        // There is a queue list row selected
        GtkTextView * tv;
        int           status;
        const char  * log_path;

        index = gtk_list_box_row_get_index(row);
        if (index < 0 || index >= ghb_array_len(ud->queue))
        { // Should never happen
            return;
        }
        queueDict = ghb_array_get(ud->queue, index);
        uiDict = ghb_dict_get(queueDict, "uiSettings");
        // Get the current buffer that is displayed in the queue log
        tv = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "queue_activity_view"));
        current = gtk_text_view_get_buffer(tv);

        status = ghb_dict_get_int(uiDict, "job_status");
        log_path = ghb_dict_get_string(uiDict, "ActivityFilename");
        if (status != GHB_QUEUE_PENDING && log_path != NULL)
        {
            ghb_ui_update(ud, "queue_activity_location",
                          ghb_string_value(log_path));
        }
        else
        {
            ghb_ui_update(ud, "queue_activity_location", ghb_string_value(""));
        }
        if (status == GHB_QUEUE_RUNNING)
        {
            // Selected encode is running, enable display of log and
            // show the live buffer
            if (ud->queue_activity_buffer != current)
            {
                gtk_text_view_set_buffer(tv, ud->queue_activity_buffer);
            }
        }
        else
        {
            // Selected encode is pending/finished/canceled/failed
            // use non-live buffer (aka extra) to display log
            if (ud->extra_activity_buffer != current)
            {
                gtk_text_view_set_buffer(tv, ud->extra_activity_buffer);
            }
            log_path = ghb_dict_get_string(uiDict, "ActivityFilename");
            if (status != GHB_QUEUE_PENDING && log_path != NULL)
            {
                // enable display of log and read log into display buffer
                read_log(ud, log_path);
            }
            else
            {
                // No log file, encode is pending
                // disable display of log
                g_free(ud->extra_activity_path);
                ud->extra_activity_path = NULL;
                gtk_text_buffer_set_text(ud->extra_activity_buffer, "", 0);
            }
        }
    }
}

void
ghb_queue_selection_init(signal_user_data_t * ud)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;

    lb            = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row           = gtk_list_box_get_selected_row(lb);
    if (row == NULL)
    {
        row = gtk_list_box_get_row_at_index(lb, 0);
        if (row != NULL)
        {
            gtk_list_box_select_row(lb, row);
        }
    }
}

char *
ghb_subtitle_short_description(const GhbValue *subsource,
                               const GhbValue *subsettings)
{
    GhbValue *import;
    char *desc = NULL;

    import = ghb_dict_get(subsettings, "Import");
    if (import != NULL)
    {
        const gchar * format = "SRT";
        const gchar * code;
        const gchar * lang;
        int           source = IMPORTSRT;
        const iso639_lang_t *iso;

        format = ghb_dict_get_string(import, "Format");
        lang   = ghb_dict_get_string(import, "Language");
        code   = ghb_dict_get_string(import, "Codeset");

        if (format != NULL && !strcasecmp(format, "SSA"))
        {
            source = IMPORTSSA;
        }
        iso = lang_lookup(lang);
        if (iso != NULL)
        {
            if (iso->native_name != NULL)
                lang = iso->native_name;
            else
                lang = iso->eng_name;
        }

        if (source == IMPORTSRT)
        {
            desc = g_strdup_printf("%s (%s)(%s)", lang, code, format);
        }
        else
        {
            desc = g_strdup_printf("%s (%s)", lang, format);
        }
    }
    else if (subsource == NULL)
    {
        desc = g_strdup(_("Foreign Audio Scan"));
    }
    else
    {
        const char * lang = ghb_dict_get_string(subsource, "Language");
        desc = g_strdup_printf("%s", lang);
    }

    return desc;
}

void
ghb_queue_progress_set_visible(signal_user_data_t *ud, int index,
                               gboolean visible)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;
    GtkWidget     * progress;

    int count = ghb_array_len(ud->queue);
    if (index < 0 || index >= count)
    {
        // invalid index
        return;
    }

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_row_at_index(lb, index);
    if (row == NULL)
    {
        return;
    }
    progress = find_widget(GTK_WIDGET(row), "queue_item_progress");
    gtk_widget_set_visible(progress, visible);
}

void
ghb_queue_progress_set_fraction(signal_user_data_t *ud, int index, gdouble frac)
{
    GtkListBox     * lb;
    GtkListBoxRow  * row;
    GtkProgressBar * progress;

    int count = ghb_array_len(ud->queue);
    if (index < 0 || index >= count)
    {
        // invalid index
        return;
    }

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_row_at_index(lb, index);
    if (row == NULL)
    {
        return;
    }
    progress = GTK_PROGRESS_BAR(find_widget(GTK_WIDGET(row),
                                            "queue_item_progress"));
    gtk_progress_bar_set_fraction(progress, frac);
}

void
ghb_queue_update_live_stats(signal_user_data_t * ud, int index, ghb_instance_status_t * status)
{
    int count = ghb_array_len(ud->queue);
    if (index < 0 || index >= count)
    {
        // invalid index
        return;
    }

    GtkListBox    * lb;
    GtkListBoxRow * row;

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_selected_row(lb);
    if (row == NULL || index != gtk_list_box_row_get_index(row))
    {
        return;
    }

    GhbValue * queueDict, * uiDict;
    queueDict = ghb_array_get(ud->queue, index);
    if (queueDict == NULL) // should never happen
    {
        return;
    }
    uiDict    = ghb_dict_get(queueDict, "uiSettings");
    if (uiDict == NULL) // should never happen
    {
        return;
    }

    GString    * gstr = NULL;
    GtkLabel   * label;
    struct tm  * tm;
    char         date[40] = "";
    char       * str;
    const char * pass = "";
    const char * result = "";
    time_t       start, finish, paused, duration;

    start  = ghb_dict_get_int(uiDict, "job_start_time");
    finish = time(NULL);
    if (status->state & GHB_STATE_WORKING)
    {
        finish += status->eta_seconds;
    }
    paused = status->paused / 1000;
    if ((status->state & GHB_STATE_WORKING) && status->pass_count > 1)
    {
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_pass_label"));
        gtk_widget_set_visible(GTK_WIDGET(label), TRUE);
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_pass"));
        gtk_widget_set_visible(GTK_WIDGET(label), TRUE);
        switch (status->pass_id)
        {
            case HB_PASS_SUBTITLE:
                pass = _("Foreign Audio Search");
                break;

            case HB_PASS_ENCODE:
                pass = _("Encode");
                break;

            case HB_PASS_ENCODE_1ST:
                pass = _("Encode First Pass (Analysis)");
                break;

            case HB_PASS_ENCODE_2ND:
                pass = _("Encode Second Pass (Final)");
                break;

            default:
                // Should never happen
                pass = _("Error");
                break;
        }
        gstr = g_string_new(NULL);
        g_string_append_printf(gstr, _("pass %d of %d\n%s"), status->pass, status->pass_count, pass);
    }
    else
    {
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_pass_label"));
        gtk_widget_set_visible(GTK_WIDGET(label), FALSE);
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_pass"));
        gtk_widget_set_visible(GTK_WIDGET(label), FALSE);
    }

    if (status->state & GHB_STATE_SCANNING)
    {
        result = _("Scanning Title");
    }
    else if (status->state & GHB_STATE_PAUSED)
    {
        result = _("Encoding Paused");
    }
    else if (status->state & GHB_STATE_WORKING)
    {
        result = _("Encoding In Progress");
    }
    else if (status->state & GHB_STATE_WORKDONE)
    {
        switch (status->error)
        {
            case GHB_ERROR_NONE:
                result = _("Completed");
                break;

            case GHB_ERROR_CANCELED:
                result = _("Canceled");
                break;

            case GHB_ERROR_FAIL:
                result = _("Failed");
                break;

            default:
                // Should never happen
                result = _("Unknown");
                break;
        }
    }

    if (gstr != NULL)
    {
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_pass"));
        gtk_label_set_text(label, gstr->str);
        g_string_free(gstr, TRUE);
    }

    tm     = localtime( &start );
    strftime(date, 40, "%c", tm);
    label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_start_time"));
    gtk_label_set_text(label, date);

    tm  = localtime( &finish );
    strftime(date, 40, "%c", tm);
    label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_finish_time"));
    gtk_label_set_text(label, date);

    int dd = 0, hh, mm, ss;
    ghb_break_duration(paused, &hh, &mm, &ss);
    if (hh >= 24)
    {
        dd = hh / 24;
        hh = hh - dd * 24;
    }
    switch (dd)
    {
        case 0:
            str = g_strdup_printf("%02d:%02d:%02d", hh, mm, ss);
            break;
        case 1:
            str = g_strdup_printf(_("%d Day %02d:%02d:%02d"), dd, hh, mm, ss);
            break;
        default:
            str = g_strdup_printf(_("%d Days %02d:%02d:%02d"), dd, hh, mm, ss);
            break;
    }
    label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_paused"));
    gtk_label_set_text(label, str);
    g_free(str);

    duration = finish - start;
    if (duration < 0)
    {
        duration = 0;
    }
    dd = 0;
    ghb_break_duration(duration, &hh, &mm, &ss);
    if (hh >= 24)
    {
        dd = hh / 24;
        hh = hh - dd * 24;
    }
    switch (dd)
    {
        case 0:
            str = g_strdup_printf("%02d:%02d:%02d", hh, mm, ss);
            break;
        case 1:
            str = g_strdup_printf(_("%d Day %02d:%02d:%02d"), dd, hh, mm, ss);
            break;
        default:
            str = g_strdup_printf(_("%d Days %02d:%02d:%02d"), dd, hh, mm, ss);
            break;
    }
    label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_encode"));
    gtk_label_set_text(label, str);
    g_free(str);

    const char  * path;
    struct stat   stbuf;

    path = ghb_dict_get_string(uiDict, "destination");
    if (g_stat(path, &stbuf) == 0)
    {
        const char * units = _("B");
        double size = stbuf.st_size;
        if (size > 1024)
        {
            units = _("KB");
            size /= 1024.0;
        }
        if (size > 1024)
        {
            units = _("MB");
            size /= 1024.0;
        }
        if (size > 1024)
        {
            units = _("GB");
            size /= 1024.0;
        }
        str = g_strdup_printf("%.2f %s", size, units);
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_file_size"));
        gtk_label_set_text(label, str);
        g_free(str);
    }
    else
    {
        label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_file_size"));
        gtk_label_set_text(label, _("Not Available"));
    }

    label = GTK_LABEL(GHB_WIDGET(ud->builder, "queue_stats_result"));
    gtk_label_set_text(label, result);
}

void
ghb_queue_update_status_icon(signal_user_data_t *ud, int index)
{
    int count = ghb_array_len(ud->queue);
    if (index < 0 || index >= count)
    {
        // invalid index
        return;
    }

    GhbValue * queueDict, * uiDict;
    queueDict = ghb_array_get(ud->queue, index);
    if (queueDict == NULL) // should never happen
    {
        return;
    }
    uiDict    = ghb_dict_get(queueDict, "uiSettings");
    if (uiDict == NULL) // should never happen
    {
        return;
    }

    int status = ghb_dict_get_int(uiDict, "job_status");

    // Now update the UI
    const char * icon_name;
    switch (status)
    {
        case GHB_QUEUE_RUNNING:
             icon_name = "hb-start";
            break;
        case GHB_QUEUE_PENDING:
             icon_name = "hb-source";
            break;
        case GHB_QUEUE_FAIL:
        case GHB_QUEUE_CANCELED:
             icon_name = "hb-stop";
            break;
        case GHB_QUEUE_DONE:
             icon_name = "hb-complete";
            break;
        default:
             icon_name = "document-edit";
            break;
    }
    GtkListBox    * lb;
    GtkListBoxRow * row;
    GtkImage      * status_icon;

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_row_at_index(lb, index);
    if (row == NULL) // should never happen
    {
        return;
    }
    status_icon = GTK_IMAGE(find_widget(GTK_WIDGET(row), "queue_item_status"));
    if (status_icon == NULL) // should never happen
    {
        return;
    }
    ghb_image_set_from_icon_name(status_icon, icon_name,
                                 GHB_ICON_SIZE_BUTTON);
}

void
ghb_queue_update_status(signal_user_data_t *ud, int index, int status)
{
    int count = ghb_array_len(ud->queue);
    if (index < 0 || index >= count)
    {
        // invalid index
        return;
    }

    GhbValue * queueDict, * uiDict;
    queueDict = ghb_array_get(ud->queue, index);
    if (queueDict == NULL) // should never happen
    {
        return;
    }
    uiDict    = ghb_dict_get(queueDict, "uiSettings");
    if (uiDict == NULL) // should never happen
    {
        return;
    }

    if (ghb_dict_get_int(uiDict, "job_status") == GHB_QUEUE_RUNNING)
    {
        return; // Never change the status of currently running jobs
    }

    if (status == GHB_QUEUE_PENDING)
    {
        ghb_queue_progress_set_visible(ud, index, FALSE);
    }
    ghb_dict_set_int(uiDict, "job_status", status);
    ghb_queue_update_status_icon(ud, index);
}

void
ghb_update_all_status(signal_user_data_t *ud, int status)
{
    int count, ii;

    count = ghb_array_len(ud->queue);
    for (ii = 0; ii < count; ii++)
    {
        ghb_queue_update_status(ud, ii, status);
    }
}

static void
save_queue_file(signal_user_data_t *ud)
{
    GtkWidget *dialog;
    GtkWindow *hb_window;

    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    dialog = gtk_file_chooser_dialog_new(_("Export Queue"),
                      hb_window,
                      GTK_FILE_CHOOSER_ACTION_SAVE,
                      GHB_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                      GHB_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                      NULL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "queue.json");
    if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
 
        int ii, count;
        GhbValue *queue = ghb_value_dup(ud->queue);
        count = ghb_array_len(queue);
        for (ii = 0; ii < count; ii++)
        {
            GhbValue *queueDict, *uiDict;

            queueDict = ghb_array_get(queue, ii);
            uiDict = ghb_dict_get(queueDict, "uiSettings");
            if (uiDict == NULL)
                continue;
            ghb_dict_set_int(uiDict, "job_status", GHB_QUEUE_PENDING);
        }

        ghb_write_settings_file(filename, queue);
        g_free (filename); 
        ghb_value_free(&queue);
    }
    gtk_widget_destroy(dialog);
}

static void
add_to_queue_list(signal_user_data_t *ud, GhbValue *queueDict)
{
    GtkListBox * lb;
    GtkWidget  * row;
    GtkBox     * hbox, * vbox, * dbox;
    GtkWidget  * ebox;
    GtkWidget  * status_icon;
    GtkWidget  * dest_label;
    GtkWidget  * delete_button;
    GtkWidget  * progress;
    GhbValue   * uiDict;
    const char * dest;
    gchar      * basename;

    lb     = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    uiDict = ghb_dict_get(queueDict, "uiSettings");

    row  = gtk_list_box_row_new();
    vbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 6));
    gtk_widget_set_margin_start(GTK_WIDGET(vbox), 12);
    hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6));
    dbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6));
#if GTK_CHECK_VERSION(3, 90, 0)
    ebox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    ebox = gtk_event_box_new();
#endif

    status_icon = ghb_image_new_from_icon_name("hb-source",
                                               GHB_ICON_SIZE_BUTTON);

    gtk_widget_set_name(status_icon, "queue_item_status");
    gtk_image_set_pixel_size(GTK_IMAGE(status_icon), 16);
    gtk_widget_set_hexpand(status_icon, FALSE);

    dest       = ghb_dict_get_string(uiDict, "destination");
    basename   = g_path_get_basename(dest);
    dest_label = gtk_label_new(basename);
    g_free(basename);
    gtk_widget_set_name(dest_label, "queue_item_dest");
    gtk_widget_set_hexpand(dest_label, TRUE);
    gtk_widget_set_halign(dest_label, GTK_ALIGN_FILL);
    gtk_label_set_justify(GTK_LABEL(dest_label), GTK_JUSTIFY_LEFT);
    gtk_label_set_xalign(GTK_LABEL(dest_label), 0.0);
    gtk_label_set_width_chars(GTK_LABEL(dest_label), 50);
    gtk_label_set_ellipsize(GTK_LABEL(dest_label), PANGO_ELLIPSIZE_END);

    delete_button = ghb_button_new_from_icon_name("edit-delete");
    gtk_button_set_relief(GTK_BUTTON(delete_button), GTK_RELIEF_NONE);
    g_signal_connect(delete_button, "clicked",
                     (GCallback)queue_remove_clicked_cb, ud);
    gtk_widget_set_hexpand(delete_button, FALSE);

    progress = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.0);
    gtk_widget_set_name(progress, "queue_item_progress");
    gtk_widget_set_visible(progress, FALSE);

    ghb_box_append_child(dbox, GTK_WIDGET(status_icon));
    ghb_box_append_child(dbox, GTK_WIDGET(dest_label));
    gtk_container_add(GTK_CONTAINER(ebox), GTK_WIDGET(dbox));
    ghb_box_append_child(hbox, GTK_WIDGET(ebox));
    ghb_box_append_child(hbox, GTK_WIDGET(delete_button));

    ghb_box_append_child(vbox, GTK_WIDGET(hbox));
    ghb_box_append_child(vbox, GTK_WIDGET(progress));
    gtk_container_add(GTK_CONTAINER(row), GTK_WIDGET(vbox));

    gtk_widget_show(GTK_WIDGET(row));
    gtk_widget_show(GTK_WIDGET(vbox));
    gtk_widget_show(GTK_WIDGET(hbox));
    gtk_widget_show(GTK_WIDGET(dbox));
    gtk_widget_show(GTK_WIDGET(ebox));
    gtk_widget_show(status_icon);
    gtk_widget_show(dest_label);
    gtk_widget_show(delete_button);
    gtk_list_box_insert(lb, row, -1);

    // style class for CSS settings
    gtk_style_context_add_class(gtk_widget_get_style_context(row), "row");
    // set row as a source for drag & drop
#if GTK_CHECK_VERSION(3, 90, 0)
    GdkContentFormats * targets;

    targets = gdk_content_formats_new(queue_drag_entries,
                                      G_N_ELEMENTS(queue_drag_entries));
    gtk_drag_source_set(ebox, GDK_BUTTON1_MASK, targets, GDK_ACTION_MOVE);
    gdk_content_formats_unref(targets);
#else
    gtk_drag_source_set(ebox, GDK_BUTTON1_MASK, queue_drag_entries, 1,
                        GDK_ACTION_MOVE);
#endif
    g_signal_connect(ebox, "drag-begin", G_CALLBACK(queue_drag_begin_cb), NULL);
    g_signal_connect(ebox, "drag-end", G_CALLBACK(queue_drag_end_cb), NULL);
    g_signal_connect(ebox, "drag-data-get",
                    G_CALLBACK(queue_drag_data_get_cb), NULL);

#if GTK_CHECK_VERSION(3, 90, 0)
    // connect key event controller to capture "delete" key press on row
    GtkEventController * econ = gtk_event_controller_key_new();
    gtk_widget_add_controller(row, econ);
    g_signal_connect(econ, "key-pressed", G_CALLBACK(queue_row_key_cb), ud);
#endif
}

static void
open_queue_file(signal_user_data_t *ud)
{
    GtkWidget *dialog;
    GtkWindow *hb_window;

    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    dialog = gtk_file_chooser_dialog_new(_("Import Queue"),
                      hb_window,
                      GTK_FILE_CHOOSER_ACTION_OPEN,
                      GHB_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                      GHB_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                      NULL);

    // Add filters
    ghb_add_file_filter(GTK_FILE_CHOOSER(dialog), ud, _("All"), "FilterAll");
    ghb_add_file_filter(GTK_FILE_CHOOSER(dialog), ud, "JSON", "FilterJSON");

    if (gtk_dialog_run(GTK_DIALOG (dialog)) != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_destroy(dialog);
        return;
    }

    GhbValue *queue;
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gtk_widget_destroy(dialog);

    queue = ghb_read_settings_file(filename);
    if (queue != NULL)
    {
        int ii, count;
        count = ghb_array_len(queue);
        for (ii = 0; ii < count; ii++)
        {
            GhbValue *queueDict, *uiDict;

            queueDict = ghb_array_get(queue, ii);
            uiDict = ghb_dict_get(queueDict, "uiSettings");
            ghb_value_incref(queueDict);
            ghb_dict_set_int(uiDict, "job_status", GHB_QUEUE_PENDING);
            ghb_dict_set_int(uiDict, "job_unique_id", 0);

            if (ud->queue == NULL)
                ud->queue = ghb_array_new();
            ghb_array_append(ud->queue, queueDict);
            add_to_queue_list(ud, queueDict);
        }
        ghb_queue_buttons_grey(ud);
        ghb_save_queue(ud->queue);
        ghb_value_free(&queue);
    }
    g_free (filename);
}

gint64
ghb_dest_free_space(GhbValue *settings)
{
    GFile       *gfile;
    GFileInfo   *info;
    guint64      size = -1;
    const gchar *dest     = ghb_dict_get_string(settings, "destination");
    gchar       *destdir  = g_path_get_dirname(dest);
    gchar       *resolved = ghb_resolve_symlink(destdir);

    gfile = g_file_new_for_path(resolved);
    info  = g_file_query_filesystem_info(gfile,
                                G_FILE_ATTRIBUTE_FILESYSTEM_FREE, NULL, NULL);
    if (info != NULL)
    {
        if (g_file_info_has_attribute(info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE))
        {
            size = g_file_info_get_attribute_uint64(info,
                                    G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
        }
        g_object_unref(info);
    }
    g_object_unref(gfile);
    g_free(resolved);
    g_free(destdir);

    return size;
}

gint
ghb_find_queue_job(GhbValue *queue, gint unique_id, GhbValue **job)
{
    GhbValue *queueDict, *uiDict;
    gint ii, count;
    gint job_unique_id;

    if (job != NULL)
    {
        *job = NULL;
    }
    if (unique_id == 0)  // Invalid Id
        return -1;

    count = ghb_array_len(queue);
    for (ii = 0; ii < count; ii++)
    {
        queueDict = ghb_array_get(queue, ii);
        uiDict = ghb_dict_get(queueDict, "uiSettings");
        job_unique_id = ghb_dict_get_int(uiDict, "job_unique_id");
        if (job_unique_id == unique_id)
        {
            if (job != NULL)
            {
                *job = queueDict;
            }
            return ii;
        }
    }
    return -1;
}

void
ghb_low_disk_check(signal_user_data_t *ud)
{
    GtkWindow       *hb_window;
    GtkWidget       *dialog, *cancel;
    GtkResponseType  response;
    ghb_status_t     status;
    const char      *paused_msg = "";
    const char      *dest;
    gint64           free_size;
    gint64           free_limit;
    GhbValue        *qDict;
    GhbValue        *settings;
    GtkStyleContext *style;

    if (ghb_dict_get_bool(ud->globals, "SkipDiskFreeCheck") ||
        !ghb_dict_get_bool(ud->prefs, "DiskFreeCheck"))
    {
        return;
    }

    ghb_get_status(&status);
    if (status.queue.unique_id <= 0)
    {
        // No current job
        return;
    }
    ghb_find_queue_job(ud->queue, status.queue.unique_id, &qDict);
    if (qDict == NULL)
    {
        // Failed to find queue setting!
        return;
    }
    settings = ghb_dict_get(qDict, "uiSettings");
    free_size = ghb_dest_free_space(settings);
    if (free_size < 0)
    {
        // Failed to read free space
        return;
    }
    // limit is in MB
    free_limit = ghb_dict_get_int(ud->prefs, "DiskFreeLimit") * 1024 * 1024;
    if (free_size > free_limit)
    {
        return;
    }

    if ((status.queue.state & GHB_STATE_WORKING) &&
        !(status.queue.state & GHB_STATE_PAUSED))
    {
        paused_msg = "Encoding has been paused.\n\n";
        ghb_pause_queue();
    }
    dest      = ghb_dict_get_string(settings, "destination");
    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    dialog    = gtk_message_dialog_new(hb_window, GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
            _("%sThe destination filesystem is almost full: %"PRId64" MB free.\n"
              "Destination: %s\n"
              "Encode may be incomplete if you proceed.\n"),
            paused_msg, free_size / (1024 * 1024), dest);
    gtk_dialog_add_buttons( GTK_DIALOG(dialog),
                           _("Resume, I've fixed the problem"), 1,
                           _("Resume, Don't tell me again"), 2,
                           _("Cancel Current and Stop"), 3,
                           NULL);

    cancel = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), 3);
    style = gtk_widget_get_style_context(cancel);
    gtk_style_context_add_class(style, "destructive-action");

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    switch ((int)response)
    {
        case 1:
            ghb_resume_queue();
            break;
        case 2:
            ghb_dict_set_bool(ud->globals, "SkipDiskFreeCheck", TRUE);
            ghb_resume_queue();
            break;
        case 3:
            ghb_stop_queue();
            ud->cancel_encode = GHB_CANCEL_ALL;
            break;
        default:
            ghb_resume_queue();
            break;
    }
}

static gboolean
validate_settings(signal_user_data_t *ud, GhbValue *settings, gint batch)
{
    // Check to see if the dest file exists or is
    // already in the queue
    gchar *message;
    const gchar *dest;
    gint count, ii;
    gint title_id, titleindex;
    const hb_title_t *title;
    GtkWindow *hb_window;

    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));

    title_id = ghb_dict_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL) return FALSE;
    dest = ghb_dict_get_string(settings, "destination");
    count = ghb_array_len(ud->queue);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *queueDict, *uiDict;
        const gchar *filename;

        queueDict = ghb_array_get(ud->queue, ii);
        uiDict = ghb_dict_get(queueDict, "uiSettings");
        filename = ghb_dict_get_string(uiDict, "destination");
        if (strcmp(dest, filename) == 0)
        {
            message = g_strdup_printf(
                        _("Destination: %s\n\n"
                        "Another queued job has specified the same destination.\n"
                        "Do you want to overwrite?"),
                        dest);
            if (!ghb_message_dialog(hb_window, GTK_MESSAGE_QUESTION,
                                    message, _("Cancel"), _("Overwrite")))
            {
                g_free(message);
                return FALSE;
            }
            g_free(message);
            break;
        }
    }
    gchar *destdir = g_path_get_dirname(dest);
    if (!g_file_test(destdir, G_FILE_TEST_IS_DIR))
    {
        message = g_strdup_printf(
                    _("Destination: %s\n\n"
                    "This is not a valid directory."),
                    destdir);
        ghb_message_dialog(hb_window, GTK_MESSAGE_ERROR,
                           message, _("Cancel"), NULL);
        g_free(message);
        g_free(destdir);
        return FALSE;
    }
#if !defined(_WIN32)
    // This doesn't work properly on windows
    if (g_access(destdir, R_OK|W_OK) != 0)
    {
        message = g_strdup_printf(
                    _("Destination: %s\n\n"
                    "Can not read or write the directory."),
                    destdir);
        ghb_message_dialog(hb_window, GTK_MESSAGE_ERROR,
                           message, _("Cancel"), NULL);
        g_free(message);
        g_free(destdir);
        return FALSE;
    }
#endif
    g_free(destdir);
    if (g_file_test(dest, G_FILE_TEST_EXISTS))
    {
        message = g_strdup_printf(
                    _("Destination: %s\n\n"
                    "File already exists.\n"
                    "Do you want to overwrite?"),
                    dest);
        if (!ghb_message_dialog(hb_window, GTK_MESSAGE_WARNING,
                                message, _("Cancel"), _("Overwrite")))
        {
            g_free(message);
            return FALSE;
        }
        g_free(message);
    }
    // Validate audio settings
    if (!ghb_validate_audio(settings, hb_window))
    {
        return FALSE;
    }
    // Validate audio settings
    if (!ghb_validate_subtitles(settings, hb_window))
    {
        return FALSE;
    }
    // Validate video settings
    if (!ghb_validate_video(settings, hb_window))
    {
        return FALSE;
    }
    // Validate filter settings
    if (!ghb_validate_filters(settings, hb_window))
    {
        return FALSE;
    }
    return TRUE;
}

void ghb_finalize_job(GhbValue *settings)
{
    GhbValue *preset, *job;

    preset = ghb_settings_to_preset(settings);
    job    = ghb_dict_get(settings, "Job");

    // Add scale filter since the above does not
    GhbValue *filter_settings, *filter_list, *filter_dict;
    int width, height, crop[4];

    filter_settings = ghb_get_job_filter_settings(settings);
    filter_list = ghb_array_new();
    ghb_dict_set(filter_settings, "FilterList", filter_list);

    width = ghb_dict_get_int(settings, "scale_width");
    height = ghb_dict_get_int(settings, "scale_height");

    crop[0] = ghb_dict_get_int(settings, "PictureTopCrop");
    crop[1] = ghb_dict_get_int(settings, "PictureBottomCrop");
    crop[2] = ghb_dict_get_int(settings, "PictureLeftCrop");
    crop[3] = ghb_dict_get_int(settings, "PictureRightCrop");

    hb_dict_t * dict = ghb_dict_new();
    ghb_dict_set_int(dict, "width", width);
    ghb_dict_set_int(dict, "height", height);
    ghb_dict_set_int(dict, "crop-top", crop[0]);
    ghb_dict_set_int(dict, "crop-bottom", crop[1]);
    ghb_dict_set_int(dict, "crop-left", crop[2]);
    ghb_dict_set_int(dict, "crop-right", crop[3]);

    filter_dict = ghb_dict_new();
    ghb_dict_set_int(filter_dict, "ID", HB_FILTER_CROP_SCALE);
    ghb_dict_set(filter_dict, "Settings", dict);
    hb_add_filter2(filter_list, filter_dict);

    // Apply selected preset settings
    hb_preset_apply_mux(preset, job);
    hb_preset_apply_video(preset, job);
    hb_preset_apply_filters(preset, job);

    ghb_value_free(&preset);
}

static gboolean
queue_add(signal_user_data_t *ud, GhbValue *settings, gint batch)
{
    // Add settings to the queue
    if (!validate_settings(ud, settings, batch))
    {
        return FALSE;
    }

    if (ud->queue == NULL)
        ud->queue = ghb_array_new();

    ghb_finalize_job(settings);

    GhbValue *titleDict  = ghb_get_title_settings(settings);
    GhbValue *jobDict    = ghb_get_job_settings(settings);
    GhbValue *uiDict     = ghb_value_dup(settings);

    ghb_dict_remove(uiDict, "Job");
    ghb_dict_remove(uiDict, "Title");

    GhbValue *queueDict  = ghb_dict_new();
    ghb_dict_set(queueDict, "uiSettings", uiDict);
    ghb_dict_set(queueDict, "Job", ghb_value_dup(jobDict));
    ghb_dict_set(queueDict, "Title", ghb_value_dup(titleDict));

    // Copy current prefs into settings
    // The job should run with the preferences that existed
    // when the job was added to the queue.
    ghb_dict_set(uiDict, "Preferences", ghb_value_dup(ud->prefs));

    // Make a copy of current settings to be used for the new job
    ghb_dict_set_int(uiDict, "job_status", GHB_QUEUE_PENDING);
    ghb_dict_set_int(uiDict, "job_unique_id", 0);

    ghb_array_append(ud->queue, queueDict);
    add_to_queue_list(ud, queueDict);
    ghb_save_queue(ud->queue);
    ghb_update_pending(ud);
    ghb_queue_buttons_grey(ud);

    return TRUE;
}

static gboolean
title_multiple_can_select(GhbValue *settings_array, int index)
{
    gint count, ii;
    GhbValue *settings, *gdest;
    const char *dest;

    settings = ghb_array_get(settings_array, index);
    gdest = ghb_dict_get_value(settings, "destination");
    dest = ghb_value_get_string(gdest);
    if (dest == NULL)
        return FALSE;

    count = ghb_array_len(settings_array);
    count = count < index ? count : index;
    for (ii = 0; ii < count; ii++)
    {
        const char *tmp;

        settings = ghb_array_get(settings_array, ii);
        gdest = ghb_dict_get_value(settings, "destination");
        tmp = ghb_value_get_string(gdest);
        if (tmp != NULL && !strncmp(dest, tmp, PATH_MAX))
            return FALSE;
    }
    return TRUE;
}

static PangoAttrList *default_title_attrs;

static void
title_add_multiple_set_sensitive(GtkWidget *row, gboolean sensitive)
{
    GtkWidget *widget;
    widget = find_widget(row, "title_selected");
    gtk_widget_set_sensitive(widget, sensitive);

    widget = find_widget(row, "title_label");
    if (!sensitive)
    {
        PangoAttrList *pal;
        PangoAttribute *bg;
        bg = pango_attr_background_new(0xFFFF, 0xFFFF, 0xA000);
        pal = pango_attr_list_new();
        pango_attr_list_insert(pal, bg);
        gtk_label_set_attributes(GTK_LABEL(widget), pal);
        gtk_widget_set_has_tooltip(widget, TRUE);
    }
    else
    {
        gtk_label_set_attributes(GTK_LABEL(widget), default_title_attrs);
        gtk_widget_set_has_tooltip(widget, FALSE);
    }
}

static gboolean
title_add_multiple_are_conflicts(signal_user_data_t *ud)
{
    GtkListBox *list;
    GtkWidget *row;
    gint count, ii;

    list = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "title_add_multiple_list"));
    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        row = GTK_WIDGET(gtk_list_box_get_row_at_index(list, ii));
        if (!title_multiple_can_select(ud->settings_array, ii))
        {
            title_add_multiple_set_sensitive(GTK_WIDGET(row), FALSE);
            return TRUE;
        }
        title_add_multiple_set_sensitive(GTK_WIDGET(row), TRUE);
    }
    return FALSE;
}

static void
title_add_multiple_set_conflict_label(
    signal_user_data_t *ud,
    gboolean are_conflicts)
{
    const gchar *msg;
    static gboolean conflict_showing = FALSE;
    GtkMessageType msg_type;

    if (are_conflicts)
    {
        msg =
            "<span foreground='black' weight='bold'>"
            "Duplicate destination files detected.\n"
            "Duplicates will not be added to the queue."
            "</span>";
            msg_type = GTK_MESSAGE_WARNING;
    }
    else
    {
        msg =
            "Destination files OK.  No duplicates detected.";
            msg_type = GTK_MESSAGE_INFO;
    }
    if (are_conflicts ^ conflict_showing)
    {
        GtkInfoBar *info;
        GtkContainer *content_area;
        GList *list;
        GtkLabel *label;

        info = GTK_INFO_BAR(GHB_WIDGET(ud->builder,
                                       "title_add_multiple_infobar"));
        content_area = GTK_CONTAINER(gtk_info_bar_get_content_area(info));
        list = gtk_container_get_children(content_area);
        // Label is first in list
        label = GTK_LABEL(list->data);
        gtk_label_set_markup(label, msg);
        gtk_info_bar_set_message_type(info, msg_type);
        conflict_showing = are_conflicts;
    }
}

static void
title_add_multiple_check_conflicts(signal_user_data_t *ud)
{
    gint count, ii;
    GhbValue *settings;
    GtkWidget *row;
    GtkListBox *list;
    GtkToggleButton *selected;
    gboolean can_select;
    gboolean are_conflicts = FALSE;

    list = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "title_add_multiple_list"));
    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        row = GTK_WIDGET(gtk_list_box_get_row_at_index(list, ii));
        selected = GTK_TOGGLE_BUTTON(find_widget(row, "title_selected"));

        settings = ghb_array_get(ud->settings_array, ii);
        can_select = title_multiple_can_select(ud->settings_array, ii);
        ghb_dict_set_bool(settings, "title_selected", FALSE);
        gtk_toggle_button_set_active(selected, FALSE);
        title_add_multiple_set_sensitive(GTK_WIDGET(row), can_select);
        are_conflicts |= !can_select;
    }
    title_add_multiple_set_conflict_label(ud, are_conflicts);
}

static void
add_multiple_titles(signal_user_data_t *ud)
{
    gint count, ii;

    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *settings;

        settings = ghb_array_get(ud->settings_array, ii);
        if (ghb_dict_get_bool(settings, "title_selected"))
        {
            queue_add(ud, settings, 1);
        }
    }
    ghb_queue_selection_init(ud);
}

static GtkListBoxRow*
list_box_get_row(GtkWidget *widget)
{
    while (widget != NULL && G_OBJECT_TYPE(widget) != GTK_TYPE_LIST_BOX_ROW)
    {
        widget = gtk_widget_get_parent(widget);
    }
    return GTK_LIST_BOX_ROW(widget);
}

GtkWidget * title_create_row(signal_user_data_t *ud)
{
    GtkBox *hbox, *vbox_dest;
    GtkCheckButton *selected;
    GtkLabel *title;
    GtkEntry *dest_file;
    GtkFileChooserButton *dest_dir;

    hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_box_set_spacing(hbox, 6);
    gtk_widget_show(GTK_WIDGET(hbox));

    // Select checkbox
    selected = GTK_CHECK_BUTTON(gtk_check_button_new());
    gtk_widget_set_tooltip_markup(GTK_WIDGET(selected),
      _("Select this title for adding to the queue.\n"));
    gtk_widget_set_valign(GTK_WIDGET(selected), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(selected), "title_selected");
    gtk_widget_show(GTK_WIDGET(selected));
    g_signal_connect(selected, "toggled", (GCallback)title_selected_cb, ud);
    ghb_box_append_child(hbox, GTK_WIDGET(selected));

    // Title label
    title = GTK_LABEL(gtk_label_new(_("No Title")));
    gtk_label_set_width_chars(title, 12);
    gtk_widget_set_halign(GTK_WIDGET(title), GTK_ALIGN_START);
    gtk_widget_set_valign(GTK_WIDGET(title), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(title), "title_label");
    gtk_widget_show(GTK_WIDGET(title));
    ghb_box_append_child(hbox, GTK_WIDGET(title));

    default_title_attrs = gtk_label_get_attributes(title);
    gtk_widget_set_tooltip_text(GTK_WIDGET(title),
        _("There is another title with the same destination file name.\n"
        "This title will not be added to the queue unless you change\n"
        "the output file name.\n"));
    gtk_widget_set_has_tooltip(GTK_WIDGET(title), FALSE);

    // Destination entry and file chooser
    vbox_dest = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_hexpand(GTK_WIDGET(vbox_dest), TRUE);
    gtk_widget_set_halign(GTK_WIDGET(vbox_dest), GTK_ALIGN_FILL);
    //gtk_widget_set_hexpand(GTK_WIDGET(vbox_dest), TRUE);
    dest_file = GTK_ENTRY(gtk_entry_new());
    ghb_entry_set_width_chars(dest_file, 40);
    gtk_widget_set_name(GTK_WIDGET(dest_file), "title_file");
    //gtk_widget_set_hexpand(GTK_WIDGET(dest_file), TRUE);
    gtk_widget_show(GTK_WIDGET(dest_file));
    g_signal_connect(dest_file, "changed", (GCallback)title_dest_file_cb, ud);
    ghb_box_append_child(vbox_dest, GTK_WIDGET(dest_file));
    dest_dir = GTK_FILE_CHOOSER_BUTTON(
            gtk_file_chooser_button_new(_("Destination Directory"),
                                        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER));
    g_signal_connect(dest_dir, "selection-changed",
                    (GCallback)title_dest_dir_cb, ud);
    gtk_widget_set_name(GTK_WIDGET(dest_dir), "title_dir");
    gtk_widget_set_hexpand(GTK_WIDGET(dest_dir), TRUE);
    gtk_widget_show(GTK_WIDGET(dest_dir));
    ghb_box_append_child(vbox_dest, GTK_WIDGET(dest_dir));
    gtk_widget_show(GTK_WIDGET(vbox_dest));
    ghb_box_append_child(hbox, GTK_WIDGET(vbox_dest));

    return GTK_WIDGET(hbox);
}

static void
ghb_queue_remove_row_internal(signal_user_data_t *ud, int index)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;
    GhbValue      * queueDict, * uiDict;

    if (index < 0 || index >= ghb_array_len(ud->queue))
    {
        return;
    }

    queueDict  = ghb_array_get(ud->queue, index);
    uiDict     = ghb_dict_get(queueDict, "uiSettings");
    int status = ghb_dict_get_int(uiDict, "job_status");
    if (status == GHB_QUEUE_RUNNING)
    {
        // Ask if wants to stop encode.
        if (!ghb_cancel_encode2(ud, NULL))
        {
            return;
        }
        int unique_id = ghb_dict_get_int(uiDict, "job_unique_id");
        ghb_remove_job(unique_id);
    }
    ghb_array_remove(ud->queue, index);

    // Update UI
    lb  = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_row_at_index(lb, index);
    gtk_container_remove(GTK_CONTAINER(lb), GTK_WIDGET(row));
}

void
ghb_queue_remove_row(signal_user_data_t *ud, int index)
{
    ghb_queue_remove_row_internal(ud, index);
    ghb_save_queue(ud->queue);
    ghb_queue_buttons_grey(ud);
}

void
ghb_queue_buttons_grey(signal_user_data_t *ud)
{
    GtkWidget        * widget;
    GSimpleAction    * action;
    gint               queue_count;
    gint               title_id, titleindex;
    const hb_title_t * title;
    gint               queue_state, scan_state;
    gboolean           allow_start, show_stop, allow_add, paused;
    GtkListBox       * lb;
    GtkListBoxRow    * row;
    gint               index, status = GHB_QUEUE_PENDING;
    GMenu            * menu;
    GMenuItem        * item;

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_selected_row(lb);

    if (row != NULL)
    {
        index = gtk_list_box_row_get_index(row);
        if (index >= 0 && index < ghb_array_len(ud->queue))
        {
            GhbValue * queueDict, *uiDict;

            queueDict = ghb_array_get(ud->queue, index);
            uiDict    = ghb_dict_get(queueDict, "uiSettings");
            status    = ghb_dict_get_int(uiDict, "job_status");
        }
    }

    queue_count = ghb_array_len(ud->queue);
    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);

    queue_state = ghb_get_queue_state();
    scan_state = ghb_get_scan_state();

    show_stop   = queue_state &
                  (GHB_STATE_WORKING | GHB_STATE_SEARCHING |
                   GHB_STATE_SCANNING | GHB_STATE_MUXING | GHB_STATE_PAUSED);
    allow_start = !(scan_state & GHB_STATE_SCANNING) &&
                    (title != NULL || queue_count > 0);
    allow_add   = !(scan_state & GHB_STATE_SCANNING) && title != NULL;


    paused = queue_state & GHB_STATE_PAUSED;

    action = G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(ud->app),
                                                        "queue-export"));
    g_simple_action_set_enabled(action, !!queue_count);
    action = G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(ud->app),
                                                        "queue-add"));
    g_simple_action_set_enabled(action, allow_add);
    action = G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(ud->app),
                                                        "queue-add-all"));
    g_simple_action_set_enabled(action, allow_add);
    action = G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(ud->app),
                                                        "queue-start"));
    g_simple_action_set_enabled(action, allow_start || show_stop);
    action = G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(ud->app),
                                                        "queue-pause"));
    g_simple_action_set_enabled(action, show_stop);

    action = G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(ud->app),
                                                        "queue-reset"));
    g_simple_action_set_enabled(action, row != NULL);

    action = G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(ud->app),
                                                        "queue-edit"));
    g_simple_action_set_enabled(action, row != NULL);

    action = G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(ud->app),
                                                        "queue-open-source"));
    g_simple_action_set_enabled(action, row != NULL);

    action = G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(ud->app),
                                                        "queue-open-dest"));
    g_simple_action_set_enabled(action, row != NULL);

    action = G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(ud->app),
                                                        "queue-open-log-dir"));
    g_simple_action_set_enabled(action, status != GHB_QUEUE_PENDING);

    action = G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(ud->app),
                                                        "queue-open-log"));
    g_simple_action_set_enabled(action, status != GHB_QUEUE_PENDING);

    widget = GHB_WIDGET (ud->builder, "queue_start");
    if (show_stop)
    {
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-stop");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Stop"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Stop Encoding"));
    }
    else
    {
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-start");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Start"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Start Encoding"));
    }
    widget = GHB_WIDGET (ud->builder, "queue_list_start");
    if (show_stop)
    {
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-stop");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Stop"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Stop Encoding"));
    }
    else
    {
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-start");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Start"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Start Encoding"));
    }
    menu = G_MENU(gtk_builder_get_object(ud->builder, "queue-encoding-actions"));
    if (show_stop)
    {
         item = g_menu_item_new_from_model(G_MENU_MODEL(menu), 0);
         g_menu_item_set_label(item, _("S_top Encoding"));
         g_menu_remove(menu, 0);
         g_menu_prepend_item(menu, item);
    }
    else
    {
		 item = g_menu_item_new_from_model(G_MENU_MODEL(menu), 0);
         g_menu_item_set_label(item, _("Start Encoding"));
         g_menu_remove(menu, 0);
         g_menu_prepend_item(menu, item);
    }

    widget = GHB_WIDGET (ud->builder, "queue_pause");
    if (paused)
    {
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-start");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Resume"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Resume Encoding"));
    }
    else
    {
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-pause");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Pause"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Pause Encoding"));
    }
    widget = GHB_WIDGET (ud->builder, "queue_list_pause");
    if (paused)
    {
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-start");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Resume"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Resume Encoding"));
    }
    else
    {
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-pause");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Pause"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Pause Encoding"));
    }
    if (paused)
    {
         item = g_menu_item_new_from_model(G_MENU_MODEL(menu), 1);
         g_menu_item_set_label(item, _("Resume Encoding"));
         g_menu_remove(menu, 1);
         g_menu_append_item(menu, item);
    }
    else
    {
		 item = g_menu_item_new_from_model(G_MENU_MODEL(menu), 1);
         g_menu_item_set_label(item, _("_Pause Encoding"));
         g_menu_remove(menu, 1);
         g_menu_append_item(menu, item);
    }
}

gboolean
ghb_reload_queue(signal_user_data_t *ud)
{
    GhbValue *queue;
    gint count, ii;
    gint pid;
    gint status;
    GhbValue *queueDict, *uiDict;

    g_debug("ghb_reload_queue");

find_pid:
    pid = ghb_find_pid_file();
    if (pid < 0)
        goto done;

    queue = ghb_load_old_queue(pid);
    ghb_remove_old_queue_file(pid);

    // Look for unfinished entries
    count = ghb_array_len(queue);
    for (ii = count-1; ii >= 0; ii--)
    {
        queueDict = ghb_array_get(queue, ii);
        uiDict = ghb_dict_get(queueDict, "uiSettings");
        if (uiDict == NULL ||
            ghb_dict_get_value(uiDict, "job_status") == NULL)
        {
            ghb_array_remove(queue, ii);
            continue;
        }
        status = ghb_dict_get_int(uiDict, "job_status");
        if (status == GHB_QUEUE_DONE || status == GHB_QUEUE_CANCELED)
        {
            ghb_array_remove(queue, ii);
            continue;
        }
    }
    count = ghb_array_len(queue);
    if (count == 0)
    {
        ghb_value_free(&queue);
        goto find_pid;
    }
    else
    {
        GtkWidget *widget = GHB_WIDGET(ud->builder, "show_queue");
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), TRUE);
        ud->queue = queue;
        for (ii = 0; ii < count; ii++)
        {
            queueDict = ghb_array_get(queue, ii);
            uiDict = ghb_dict_get(queueDict, "uiSettings");
            ghb_dict_set_int(uiDict, "job_unique_id", 0);
            ghb_dict_set_int(uiDict, "job_status", GHB_QUEUE_PENDING);
            add_to_queue_list(ud, queueDict);
        }
        ghb_queue_buttons_grey(ud);
        ghb_save_queue(ud->queue);
        ghb_update_pending(ud);
        ghb_queue_selection_init(ud);
    }

done:
    ghb_write_pid_file();

    return FALSE;
}

static gboolean
queue_row_key(GtkListBoxRow * row, guint keyval, signal_user_data_t * ud)
{
    gint            index;

    if (keyval != GDK_KEY_Delete)
        return FALSE;

    if (row != NULL)
    {
        index = gtk_list_box_row_get_index(row);
        ghb_queue_remove_row_internal(ud, index);
        ghb_save_queue(ud->queue);
        return TRUE;
    }
    return FALSE;
}

#if GTK_CHECK_VERSION(3, 90, 0)
G_MODULE_EXPORT gboolean
queue_row_key_cb(
    GtkEventControllerKey * keycon,
    guint                   keyval,
    guint                   keycode,
    GdkModifierType         state,
    signal_user_data_t    * ud)
{
    GtkListBoxRow * row;

    row = GTK_LIST_BOX_ROW(gtk_event_controller_get_widget(
                                        GTK_EVENT_CONTROLLER(keycon)));
    return queue_row_key(row, keyval, ud);
}

#else
G_MODULE_EXPORT gboolean
queue_key_press_cb(
    GtkWidget          * widget,
    GdkEvent           * event,
    signal_user_data_t * ud)
{
    GtkListBoxRow * row;
    guint           keyval;

    row = gtk_list_box_get_selected_row(GTK_LIST_BOX(widget));
    ghb_event_get_keyval(event, &keyval);
    return queue_row_key(row, keyval, ud);
}
#endif

G_MODULE_EXPORT void
show_queue_action_cb(GSimpleAction *action, GVariant *value,
                     signal_user_data_t *ud)
{
    GtkWidget * queue_window;
    gboolean    state = g_variant_get_boolean(value);

    g_simple_action_set_state(action, value);
    queue_window = GHB_WIDGET(ud->builder, "queue_window");
    gtk_widget_set_visible(queue_window, state);
}

G_MODULE_EXPORT gboolean
queue_window_delete_cb(
    GtkWidget *xwidget,
#if !GTK_CHECK_VERSION(3, 90, 0)
    GdkEvent *event,
#endif
    signal_user_data_t *ud)
{
    gtk_widget_set_visible(xwidget, FALSE);
    GtkWidget *widget = GHB_WIDGET (ud->builder, "show_queue");
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), FALSE);
    return TRUE;
}

GhbValue *ghb_queue_edit_settings = NULL;

G_MODULE_EXPORT void
queue_edit_action_cb(GSimpleAction *action, GVariant *param,
                     signal_user_data_t *ud)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;
    gint            index, status;
    GhbValue      * queueDict, *uiDict;

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_selected_row(lb);
    if (row != NULL)
    {
        index = gtk_list_box_row_get_index(row);
        if (index < 0 || index >= ghb_array_len(ud->queue))
            return;

        queueDict = ghb_array_get(ud->queue, index);
        uiDict    = ghb_dict_get(queueDict, "uiSettings");
        ghb_queue_edit_settings = ghb_value_dup(uiDict);
        ghb_dict_set(ghb_queue_edit_settings,
                     "Job", ghb_value_dup(ghb_dict_get(queueDict, "Job")));
        ghb_dict_set(ghb_queue_edit_settings,
                     "Title", ghb_value_dup(ghb_dict_get(queueDict, "Title")));
        status = ghb_dict_get_int(uiDict, "job_status");
        if (status == GHB_QUEUE_PENDING)
        {
            // Remove the selected item
            gtk_container_remove(GTK_CONTAINER(lb), GTK_WIDGET(row));
            // Remove the corresponding item from the queue list
            ghb_array_remove(ud->queue, index);
            ghb_update_pending(ud);
        }
        const gchar *source;
        source = ghb_dict_get_string(ghb_queue_edit_settings, "source");
        ghb_do_scan(ud, source, 0, FALSE);

        GtkWidget *widget = GHB_WIDGET(ud->builder, "show_queue");
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), FALSE);
    }
}

G_MODULE_EXPORT void
queue_export_action_cb(GSimpleAction *action, GVariant *param,
                     signal_user_data_t *ud)
{
    save_queue_file(ud);
}

G_MODULE_EXPORT void
queue_import_action_cb(GSimpleAction *action, GVariant *param,
                     signal_user_data_t *ud)
{
    open_queue_file(ud);
}

G_MODULE_EXPORT void
queue_open_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    open_queue_file(ud);
}

G_MODULE_EXPORT void
queue_open_source_action_cb(GSimpleAction *action, GVariant *param,
                            signal_user_data_t *ud)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;
    gint            index;
    GhbValue      * queueDict, * titleDict;
    GString       * str;
    const char    * path;
    char          * dir, * uri;

    lb  = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_selected_row(lb);
    if (row != NULL)
    {
        index = gtk_list_box_row_get_index(row);
        if (index < 0 || index >= ghb_array_len(ud->queue))
        {
            return;
        }
        queueDict = ghb_array_get(ud->queue, index);
        titleDict = ghb_dict_get(queueDict, "Title");
        path      = ghb_dict_get_string(titleDict, "Path");
        dir       = g_path_get_dirname(path);
        str       = g_string_new(NULL);
        g_string_printf(str, "file://%s", dir);
        uri       = g_string_free(str, FALSE);
        ghb_browse_uri(ud, uri);
        g_free(uri);
        g_free(dir);
    }
}

G_MODULE_EXPORT void
queue_open_dest_action_cb(GSimpleAction *action, GVariant *param,
                          signal_user_data_t *ud)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;
    gint            index;
    GhbValue      * queueDict, * uiDict;
    GString       * str;
    const char    * path;
    char          * dir, * uri;

    lb  = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_selected_row(lb);
    if (row != NULL)
    {
        index = gtk_list_box_row_get_index(row);
        if (index < 0 || index >= ghb_array_len(ud->queue))
        {
            return;
        }
        queueDict = ghb_array_get(ud->queue, index);
        uiDict    = ghb_dict_get(queueDict, "uiSettings");
        path      = ghb_dict_get_string(uiDict, "destination");
        dir       = g_path_get_dirname(path);
        str       = g_string_new(NULL);
        g_string_printf(str, "file://%s", dir);
        uri       = g_string_free(str, FALSE);
        ghb_browse_uri(ud, uri);
        g_free(uri);
        g_free(dir);
    }
}

G_MODULE_EXPORT void
queue_open_log_action_cb(GSimpleAction *action, GVariant *param,
                         signal_user_data_t *ud)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;
    gint            index;
    GhbValue      * queueDict, * uiDict;
    GString       * str;
    const char    * path;
    char          * uri;

    lb  = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_selected_row(lb);
    if (row != NULL)
    {
        index = gtk_list_box_row_get_index(row);
        if (index < 0 || index >= ghb_array_len(ud->queue))
        {
            return;
        }
        queueDict = ghb_array_get(ud->queue, index);
        uiDict    = ghb_dict_get(queueDict, "uiSettings");
        path      = ghb_dict_get_string(uiDict, "ActivityFilename");
        if (path == NULL)
        {
            return;
        }
        str       = g_string_new(NULL);
        g_string_printf(str, "file://%s", path);
        uri       = g_string_free(str, FALSE);
        ghb_browse_uri(ud, uri);
        g_free(uri);
    }
}

G_MODULE_EXPORT void
queue_open_log_dir_action_cb(GSimpleAction *action, GVariant *param,
                             signal_user_data_t *ud)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;
    gint            index;
    GhbValue      * queueDict, * uiDict;
    GString       * str;
    const char    * path;
    char          * dir, * uri;

    lb  = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_selected_row(lb);
    if (row != NULL)
    {
        index = gtk_list_box_row_get_index(row);
        if (index < 0 || index >= ghb_array_len(ud->queue))
        {
            return;
        }
        queueDict = ghb_array_get(ud->queue, index);
        uiDict    = ghb_dict_get(queueDict, "uiSettings");
        path      = ghb_dict_get_string(uiDict, "ActivityFilename");
        if (path == NULL)
        {
            return;
        }
        dir       = g_path_get_dirname(path);
        str       = g_string_new(NULL);
        g_string_printf(str, "file://%s", dir);
        uri       = g_string_free(str, FALSE);
        ghb_browse_uri(ud, uri);
        g_free(uri);
        g_free(dir);
    }
}

G_MODULE_EXPORT void
queue_list_selection_changed_cb(GtkListBox *box, GtkListBoxRow *row,
                                signal_user_data_t *ud)
{
    GhbValue  * queueDict = NULL;
    int         index = -1;

    if (row != NULL)
    {
        index = gtk_list_box_row_get_index(row);
    }
    if (index >= 0 && index < ghb_array_len(ud->queue))
    {
        queueDict = ghb_array_get(ud->queue, index);
    }
    queue_update_summary(queueDict, ud);
    queue_update_stats(queueDict, ud);
    ghb_queue_select_log(ud);
    ghb_queue_buttons_grey(ud);
}

G_MODULE_EXPORT void
queue_add_action_cb(GSimpleAction *action, GVariant *param,
                    signal_user_data_t *ud)
{
    queue_add(ud, ud->settings, 0);
    ghb_queue_selection_init(ud);
    // Validation of settings may have changed audio list
    ghb_audio_list_refresh_all(ud);
}

G_MODULE_EXPORT void
queue_add_all_action_cb(GSimpleAction *action, GVariant *param,
                        signal_user_data_t *ud)
{
    GtkListBox *list;
    GtkWidget *row;
    gint count, ii;
    int max_title_len = 0;
    GhbValue * preset = NULL;

    list = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "title_add_multiple_list"));

    if (ghb_dict_get_bool(ud->prefs, "SyncTitleSettings"))
    {
        preset = ghb_settings_to_preset(ud->settings);
    }

    // Set up the list of titles
    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *settings;
        GtkLabel *label;
        GtkEntry *entry;
        GtkFileChooser *chooser;
        gchar *title_label;
        const gchar *dest_dir, *dest_file;
        int title_id, titleindex;
        const hb_title_t *title;

        row = title_create_row(ud);
        label = GTK_LABEL(find_widget(row, "title_label"));
        entry = GTK_ENTRY(find_widget(row, "title_file"));
        chooser = GTK_FILE_CHOOSER(find_widget(row, "title_dir"));

        settings = ghb_array_get(ud->settings_array, ii);
        if (preset != NULL)
        {
            ghb_preset_to_settings(settings, preset);
            ghb_set_title_settings(ud, settings);
        }
        title_id = ghb_dict_get_int(settings, "title");
        title = ghb_lookup_title(title_id, &titleindex);
        if (title != NULL)
        {
            int len;

            title_label = ghb_create_title_label(title);
            len = strnlen(title_label, PATH_MAX);
            if (len > max_title_len)
                max_title_len = len;

            dest_file = ghb_dict_get_string(settings, "dest_file");
            dest_dir = ghb_dict_get_string(settings, "dest_dir");

            gtk_label_set_markup(label, title_label);
            ghb_editable_set_text(entry, dest_file);
            gtk_file_chooser_set_filename(chooser, dest_dir);

            g_free(title_label);
        }

        gtk_list_box_insert(list, row, -1);
    }
    // Now we need to set the width of the title label since it
    // can vary on each row
    if (max_title_len > 60)
        max_title_len = 60;
    for (ii = 0; ii < count; ii++)
    {
        GtkWidget *row;
        GtkLabel *label;

        row = GTK_WIDGET(gtk_list_box_get_row_at_index(list, ii));
        label = GTK_LABEL(find_widget(row, "title_label"));
        gtk_label_set_max_width_chars(label, max_title_len);
        gtk_label_set_width_chars(label, max_title_len);
        gtk_label_set_ellipsize(label, PANGO_ELLIPSIZE_END);
    }

    // Clear "select all" and "clear all" options
    GtkToggleButton *select_all;
    GtkToggleButton *clear_all;

    clear_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_clear_all"));
    select_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_select_all"));
    gtk_toggle_button_set_active(clear_all, FALSE);
    gtk_toggle_button_set_active(select_all, FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(select_all), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(clear_all), TRUE);

    // Disable selection of files with duplicate file names.
    title_add_multiple_check_conflicts(ud);

    // Pop up the title multiple selections dialog
    GtkResponseType response;
    GtkWidget *dialog = GHB_WIDGET(ud->builder, "title_add_multiple_dialog");
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    if (response == GTK_RESPONSE_OK)
    {
        add_multiple_titles(ud);
    }

    // Clear title list
    ghb_container_empty(GTK_CONTAINER(list));
}

G_MODULE_EXPORT void
queue_delete_all_action_cb(GSimpleAction *action, GVariant *param,
                           signal_user_data_t *ud)
{
    int ii, count;

    count = ghb_array_len(ud->queue);
    for (ii = count - 1; ii >= 0; ii--)
    {
        ghb_queue_remove_row_internal(ud, ii);
    }
    ghb_save_queue(ud->queue);
    ghb_update_pending(ud);
    ghb_queue_buttons_grey(ud);
}

G_MODULE_EXPORT void
queue_delete_complete_action_cb(GSimpleAction *action, GVariant *param,
                                signal_user_data_t *ud)
{
    int        ii, count, status;
    GhbValue * queueDict, * uiDict;

    count = ghb_array_len(ud->queue);
    for (ii = count - 1; ii >= 0; ii--)
    {
        queueDict = ghb_array_get(ud->queue, ii);
        uiDict = ghb_dict_get(queueDict, "uiSettings");
        status = ghb_dict_get_int(uiDict, "job_status");
        if (status == GHB_QUEUE_DONE)
        {
            ghb_queue_remove_row_internal(ud, ii);
        }
    }
    ghb_save_queue(ud->queue);
    ghb_update_pending(ud);
    ghb_queue_buttons_grey(ud);
}

G_MODULE_EXPORT void
queue_reset_all_action_cb(GSimpleAction *action, GVariant *param,
                          signal_user_data_t *ud)
{
    ghb_update_all_status(ud, GHB_QUEUE_PENDING);
    ghb_save_queue(ud->queue);
    queue_update_current_stats(ud);
    ghb_queue_select_log(ud);
    ghb_update_pending(ud);
    ghb_queue_buttons_grey(ud);
}

G_MODULE_EXPORT void
queue_reset_fail_action_cb(GSimpleAction *action, GVariant *param,
                          signal_user_data_t *ud)
{
    int        ii, count, status;
    GhbValue * queueDict, * uiDict;

    count = ghb_array_len(ud->queue);
    for (ii = count - 1; ii >= 0; ii--)
    {
        queueDict = ghb_array_get(ud->queue, ii);
        uiDict = ghb_dict_get(queueDict, "uiSettings");
        status = ghb_dict_get_int(uiDict, "job_status");
        if (status == GHB_QUEUE_FAIL)
        {
            ghb_queue_update_status(ud, ii, GHB_QUEUE_PENDING);
        }
    }
    ghb_save_queue(ud->queue);
    queue_update_current_stats(ud);
    ghb_queue_select_log(ud);
    ghb_update_pending(ud);
    ghb_queue_buttons_grey(ud);
}

G_MODULE_EXPORT void
queue_reset_action_cb(GSimpleAction *action, GVariant *param,
                          signal_user_data_t *ud)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;
    gint            index;

    lb  = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_selected_row(lb);
    if (row != NULL)
    {
        index = gtk_list_box_row_get_index(row);
        ghb_queue_update_status(ud, index, GHB_QUEUE_PENDING);
        ghb_save_queue(ud->queue);
        queue_update_current_stats(ud);
        ghb_queue_select_log(ud);
        ghb_update_pending(ud);
        ghb_queue_buttons_grey(ud);
    }
}

G_MODULE_EXPORT void
queue_remove_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GtkListBoxRow * row;
    gint            index;

    row = list_box_get_row(widget);
    if (row != NULL)
    {
        index = gtk_list_box_row_get_index(row);
        ghb_queue_remove_row_internal(ud, index);
        ghb_save_queue(ud->queue);
    }
    ghb_update_pending(ud);
    ghb_queue_buttons_grey(ud);
}

G_MODULE_EXPORT void
queue_start_action_cb(GSimpleAction *action, GVariant *param,
                      signal_user_data_t *ud)
{
    GhbValue *queueDict, *uiDict;
    gboolean running = FALSE;
    gint count, ii;
    gint status;
    gint state;

    state = ghb_get_queue_state();
    if (state & (GHB_STATE_WORKING | GHB_STATE_SEARCHING |
                 GHB_STATE_SCANNING | GHB_STATE_MUXING))
    {
        ghb_cancel_encode(ud, _("You are currently encoding.  "
                                "What would you like to do?\n\n"));
        return;
    }

    count = ghb_array_len(ud->queue);
    for (ii = 0; ii < count; ii++)
    {
        queueDict = ghb_array_get(ud->queue, ii);
        uiDict = ghb_dict_get(queueDict, "uiSettings");
        status = ghb_dict_get_int(uiDict, "job_status");
        if ((status == GHB_QUEUE_RUNNING) ||
            (status == GHB_QUEUE_PENDING))
        {
            running = TRUE;
            break;
        }
    }
    if (!running)
    {
        // The queue has no running or pending jobs.
        // Add current settings to the queue, then run.
        if (!queue_add(ud, ud->settings, 0))
        {
            return;
        }
        ghb_queue_selection_init(ud);
        // Validation of settings may have changed audio list
        ghb_audio_list_refresh_all(ud);
    }
    if (state == GHB_STATE_IDLE)
    {
        // Add the first pending queue item and start
        ghb_start_next_job(ud);
    }
}

G_MODULE_EXPORT void
queue_pause_action_cb(GSimpleAction *action, GVariant *param,
                      signal_user_data_t *ud)
{
    ghb_pause_resume_queue();
}

static gboolean clear_select_all_busy = FALSE;

G_MODULE_EXPORT void
title_add_multiple_select_all_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    gint count, ii;
    GhbValue *settings;
    GtkWidget *row;
    GtkListBox *list;
    GtkToggleButton *selected;
    gboolean can_select;
    GtkToggleButton *clear_all;
    GtkToggleButton *select_all;

    if (!ghb_widget_boolean(widget))
        return;

    clear_select_all_busy = TRUE;

    clear_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_clear_all"));
    select_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_select_all"));
    gtk_toggle_button_set_active(clear_all, FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(select_all), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(clear_all), TRUE);

    list = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "title_add_multiple_list"));
    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        row = GTK_WIDGET(gtk_list_box_get_row_at_index(list, ii));
        selected = GTK_TOGGLE_BUTTON(find_widget(row, "title_selected"));
        settings = ghb_array_get(ud->settings_array, ii);
        can_select = title_multiple_can_select(ud->settings_array, ii);
        ghb_dict_set_bool(settings, "title_selected", can_select);
        gtk_toggle_button_set_active(selected, TRUE);
        title_add_multiple_set_sensitive(GTK_WIDGET(row), can_select);
    }
    clear_select_all_busy = FALSE;
}

G_MODULE_EXPORT void
title_add_multiple_clear_all_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    gint count, ii;
    GhbValue *settings;
    GtkWidget *row;
    GtkListBox *list;
    GtkToggleButton *selected;
    GtkToggleButton *clear_all;
    GtkToggleButton *select_all;

    if (!ghb_widget_boolean(widget))
        return;

    clear_select_all_busy = TRUE;

    clear_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_clear_all"));
    select_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_select_all"));
    gtk_toggle_button_set_active(select_all, FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(select_all), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(clear_all), FALSE);

    list = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "title_add_multiple_list"));
    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        row = GTK_WIDGET(gtk_list_box_get_row_at_index(list, ii));
        selected = GTK_TOGGLE_BUTTON(find_widget(row, "title_selected"));
        settings = ghb_array_get(ud->settings_array, ii);
        ghb_dict_set_bool(settings, "title_selected", FALSE);
        gtk_toggle_button_set_active(selected, FALSE);
    }
    clear_select_all_busy = FALSE;
}

G_MODULE_EXPORT void
title_selected_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *settings;
    gboolean selected;
    GtkToggleButton *select_all;
    GtkToggleButton *clear_all;
    gboolean can_select;

    if (clear_select_all_busy)
        return;

    GtkListBoxRow * row = list_box_get_row(widget);
    if (row == NULL)
        return;

    clear_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_clear_all"));
    select_all = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                           "title_add_multiple_select_all"));
    gtk_toggle_button_set_active(select_all, FALSE);
    gtk_toggle_button_set_active(clear_all, FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(clear_all), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(select_all), TRUE);

    gint index = gtk_list_box_row_get_index(row);
    selected = ghb_widget_boolean(widget);
    settings = ghb_array_get(ud->settings_array, index);
    can_select = title_multiple_can_select(ud->settings_array, index);
    ghb_dict_set_bool(settings, "title_selected",
                             selected && can_select);
}

G_MODULE_EXPORT void
title_dest_file_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *settings;
    const gchar *dest_dir;
    gchar *dest_file, *dest;
    GtkListBoxRow * row = list_box_get_row(widget);
    if (row == NULL)
        return;
    gint index = gtk_list_box_row_get_index(row);

    dest_file = ghb_widget_string(widget);
    settings = ghb_array_get(ud->settings_array, index);

    ghb_dict_set_string(settings, "dest_file", dest_file);
    dest_dir = ghb_dict_get_string(settings, "dest_dir");
    dest = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", dest_dir, dest_file);
    ghb_dict_set_string(settings, "destination", dest);
    GhbValue *dest_dict = ghb_get_job_dest_settings(settings);
    ghb_dict_set_string(dest_dict, "File", dest);

    // Check if changing the destination file name resolves
    // a file name conflict.  Enable selection if so.
    // Disable selection if it creates a conflict!!!
    gboolean selected, can_select;

    widget     = find_widget(GTK_WIDGET(row), "title_selected");
    selected   = ghb_widget_boolean(widget);
    can_select = title_multiple_can_select(ud->settings_array, index);

    ghb_dict_set_bool(settings, "title_selected", selected && can_select);
    title_add_multiple_set_sensitive(GTK_WIDGET(row), can_select);

    g_free(dest_file);
    g_free(dest);

    title_add_multiple_set_conflict_label(ud,
        title_add_multiple_are_conflicts(ud));
}

G_MODULE_EXPORT void
title_dest_dir_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *settings;
    const gchar *dest_file;
    gchar *dest_dir, *dest;
    GtkListBoxRow * row = list_box_get_row(widget);
    if (row == NULL)
        return;
    gint index = gtk_list_box_row_get_index(row);

    dest_dir = ghb_widget_string(widget);
    settings = ghb_array_get(ud->settings_array, index);

    ghb_dict_set_string(settings, "dest_dir", dest_dir);
    dest_file = ghb_dict_get_string(settings, "dest_file");
    dest = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", dest_dir, dest_file);
    ghb_dict_set_string(settings, "destination", dest);
    GhbValue *dest_dict = ghb_get_job_dest_settings(settings);
    ghb_dict_set_string(dest_dict, "File", dest);

    // Check if changing the destination file name resolves
    // a file name conflict.  Enable selection if so.
    // Disable selection if it creates a conflict!!!
    gboolean selected, can_select;

    widget     = find_widget(GTK_WIDGET(row), "title_selected");
    selected   = ghb_widget_boolean(widget);
    can_select = title_multiple_can_select(ud->settings_array, index);

    ghb_dict_set_bool(settings, "title_selected", selected && can_select);
    title_add_multiple_set_sensitive(GTK_WIDGET(row), can_select);

    g_free(dest_dir);
    g_free(dest);

    title_add_multiple_set_conflict_label(ud,
        title_add_multiple_are_conflicts(ud));
}

// Set up view of row to be dragged
G_MODULE_EXPORT void
#if GTK_CHECK_VERSION(3, 90, 0)
queue_drag_begin_cb(GtkWidget          * widget,
                    GdkDrag            * context,
                    signal_user_data_t * ud)
#else
queue_drag_begin_cb(GtkWidget          * widget,
                    GdkDragContext     * context,
                    signal_user_data_t * ud)
#endif
{
    GtkListBox      * lb;
    GtkWidget       * row;

    row = gtk_widget_get_ancestor(widget, GTK_TYPE_LIST_BOX_ROW);
    lb  = GTK_LIST_BOX(gtk_widget_get_parent(row));
    gtk_list_box_select_row(lb, GTK_LIST_BOX_ROW(row));

#if GTK_CHECK_VERSION(3, 90, 0)
    GdkPaintable * paintable = gtk_widget_paintable_new(row);
    gtk_drag_set_icon_paintable(context, paintable, 0, 0);
    g_object_unref(paintable);
#else
    GtkAllocation     alloc;
    cairo_surface_t * surface;
    cairo_t         * cr;
    int               x, y;

    gtk_widget_get_allocation(row, &alloc);
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                         alloc.width, alloc.height);
    cr = cairo_create(surface);

    gtk_style_context_add_class(gtk_widget_get_style_context(row), "drag-icon");
    gtk_widget_draw(row, cr);
    gtk_style_context_remove_class(gtk_widget_get_style_context(row),
                                   "drag-icon");

    gtk_widget_translate_coordinates(widget, row, 0, 0, &x, &y);
    cairo_surface_set_device_offset(surface, -x, -y);
    gtk_drag_set_icon_surface(context, surface);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
#endif

    g_object_set_data(G_OBJECT(gtk_widget_get_parent(row)), "drag-row", row);
    gtk_style_context_add_class(gtk_widget_get_style_context(row), "drag-row");
}

G_MODULE_EXPORT void
#if GTK_CHECK_VERSION(3, 90, 0)
queue_drag_end_cb(GtkWidget          * widget,
                  GdkDrag            * context,
                  signal_user_data_t * ud)
#else
queue_drag_end_cb(GtkWidget          * widget,
                  GdkDragContext     * context,
                  signal_user_data_t * ud)
#endif
{
    GtkWidget * row;

    row = gtk_widget_get_ancestor(widget, GTK_TYPE_LIST_BOX_ROW);
    g_object_set_data(G_OBJECT(gtk_widget_get_parent(row)), "drag-row", NULL);
    gtk_style_context_remove_class(gtk_widget_get_style_context(row),
                                   "drag-row");
    gtk_style_context_remove_class(gtk_widget_get_style_context(row),
                                   "drag-hover");
}

// Set selection to the row being dragged
G_MODULE_EXPORT void
#if GTK_CHECK_VERSION(3, 90, 0)
queue_drag_data_get_cb(GtkWidget          * widget,
                       GdkDrag            * context,
                       GtkSelectionData   * selection_data,
                       signal_user_data_t * ud)
#else
queue_drag_data_get_cb(GtkWidget          * widget,
                       GdkDragContext     * context,
                       GtkSelectionData   * selection_data,
                       guint                info,
                       guint                time,
                       signal_user_data_t * ud)
#endif
{
    GtkWidget * row;

    row = gtk_widget_get_ancestor(widget, GTK_TYPE_LIST_BOX_ROW);
    gtk_selection_data_set(selection_data,
                       ghb_atom_string("GTK_LIST_BOX_ROW"), 32,
                       (const guchar *)&row, sizeof (gpointer));
}

G_MODULE_EXPORT void
#if GTK_CHECK_VERSION(3, 90, 0)
queue_drag_leave_cb(
    GtkListBox         * lb,
    GdkDrop            * ctx,
    signal_user_data_t * ud)
#else
queue_drag_leave_cb(
    GtkListBox         * lb,
    GdkDragContext     * ctx,
    guint                time,
    signal_user_data_t * ud)
#endif
{
    GtkWidget * drag_row;
    GtkWidget * row_before;
    GtkWidget * row_after;

    drag_row   = GTK_WIDGET(g_object_get_data(G_OBJECT(lb), "drag-row"));
    row_before = GTK_WIDGET(g_object_get_data(G_OBJECT(lb), "row-before"));
    row_after  = GTK_WIDGET(g_object_get_data(G_OBJECT(lb), "row-after"));

    gtk_style_context_remove_class(gtk_widget_get_style_context(drag_row),
                                   "drag-hover");
    if (row_before)
    {
        gtk_style_context_remove_class(gtk_widget_get_style_context(row_before),
                                       "drag-hover-bottom");
    }
    if (row_after)
    {
        gtk_style_context_remove_class(gtk_widget_get_style_context(row_after),
                                       "drag-hover-top");
    }
}

static GtkListBoxRow *
get_last_row(GtkListBox * list)
{
    int i;
    GtkListBoxRow * row;

    row = NULL;
    for (i = 0; ; i++)
    {
        GtkListBoxRow * tmp;
        tmp = gtk_list_box_get_row_at_index(list, i);
        if (tmp == NULL)
            return row;
        row = tmp;
    }
    return row;
}

static GtkListBoxRow *
get_row_before (GtkListBox    * list, GtkListBoxRow * row)
{
    int pos = gtk_list_box_row_get_index(row);
    return gtk_list_box_get_row_at_index(list, pos - 1);
}

static GtkListBoxRow *
get_row_after (GtkListBox    * list, GtkListBoxRow * row)
{
    int pos = gtk_list_box_row_get_index(row);
    return gtk_list_box_get_row_at_index(list, pos + 1);
}

G_MODULE_EXPORT gboolean
#if GTK_CHECK_VERSION(3, 90, 0)
queue_drag_motion_cb(
    GtkListBox         * lb,
    GdkDrop            * ctx,
    gint                 x,
    gint                 y,
    signal_user_data_t * ud)
#else
queue_drag_motion_cb(
    GtkListBox         * lb,
    GdkDragContext     * ctx,
    gint                 x,
    gint                 y,
    guint                time,
    signal_user_data_t * ud)
#endif
{
    GtkAllocation   alloc;
    GtkWidget     * row;
    int             hover_row_y;
    int             hover_row_height;
    GtkWidget     * drag_row;
    GtkWidget     * row_before;
    GtkWidget     * row_after;

    row = GTK_WIDGET(gtk_list_box_get_row_at_y(GTK_LIST_BOX(lb), y));

    drag_row   = GTK_WIDGET(g_object_get_data(G_OBJECT(lb), "drag-row"));
    row_before = GTK_WIDGET(g_object_get_data(G_OBJECT(lb), "row-before"));
    row_after  = GTK_WIDGET(g_object_get_data(G_OBJECT(lb), "row-after"));

    gtk_style_context_remove_class(gtk_widget_get_style_context(drag_row),
                                   "drag-hover");
    if (row_before)
    {
        gtk_style_context_remove_class(gtk_widget_get_style_context(row_before),
                                       "drag-hover-bottom");
    }
    if (row_after)
    {
        gtk_style_context_remove_class(gtk_widget_get_style_context(row_after),
                                       "drag-hover-top");
    }

    if (row)
    {
        gtk_widget_get_allocation(row, &alloc);
        hover_row_y = alloc.y;
        hover_row_height = alloc.height;

        if (y < hover_row_y + hover_row_height/2)
        {
            row_after = row;
            row_before = GTK_WIDGET(get_row_before(lb, GTK_LIST_BOX_ROW(row)));
        }
        else
        {
            row_before = row;
            row_after = GTK_WIDGET(get_row_after(lb, GTK_LIST_BOX_ROW(row)));
        }
    }
    else
    {
        row_before = GTK_WIDGET(get_last_row(lb));
        row_after = NULL;
    }

    g_object_set_data(G_OBJECT(lb), "row-before", row_before);
    g_object_set_data(G_OBJECT(lb), "row-after", row_after);

    if (drag_row == row_before || drag_row == row_after)
    {
        gtk_style_context_add_class(gtk_widget_get_style_context(drag_row),
                                    "drag-hover");
        return FALSE;
    }

    if (row_before)
    {
        gtk_style_context_add_class(gtk_widget_get_style_context(row_before),
                                    "drag-hover-bottom");
    }
    if (row_after)
    {
        gtk_style_context_add_class(gtk_widget_get_style_context(row_after), "drag-hover-top");
    }

    return TRUE;
}

G_MODULE_EXPORT void
#if GTK_CHECK_VERSION(3, 90, 0)
queue_drag_data_received_cb(GtkListBox         * lb,
                            GdkDrop            * context,
                            GtkSelectionData   * selection_data,
                            signal_user_data_t * ud)
#else
queue_drag_data_received_cb(GtkListBox         * lb,
                            GdkDragContext     * context,
                            gint                 x,
                            gint                 y,
                            GtkSelectionData   * selection_data,
                            guint                info,
                            guint32              time,
                            signal_user_data_t * ud)
#endif
{
    GtkWidget * row_before;
    GtkWidget * row_after;
    GtkWidget * src_row;
    int         src_index, dst_index;

    row_before = GTK_WIDGET(g_object_get_data(G_OBJECT(lb), "row-before"));
    row_after  = GTK_WIDGET(g_object_get_data(G_OBJECT(lb), "row-after"));

    g_object_set_data(G_OBJECT(lb), "row-before", NULL);
    g_object_set_data(G_OBJECT(lb), "row-after", NULL);

    if (row_before)
    {
        gtk_style_context_remove_class(gtk_widget_get_style_context(row_before),
                                       "drag-hover-bottom");
    }
    if (row_after)
    {
        gtk_style_context_remove_class(gtk_widget_get_style_context(row_after),
                                       "drag-hover-top");
    }

    src_row = GTK_WIDGET(*(gpointer*)gtk_selection_data_get_data(selection_data));

    if (src_row == row_after)
    {
        gtk_list_box_select_row(lb, GTK_LIST_BOX_ROW(src_row));
        return;
    }

    src_index = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(src_row));
    if (row_after)
    {
        dst_index = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row_after));
    }
    else
    {
        dst_index = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row_before)) + 1;
    }

    if (src_index < dst_index)
    {
        // The source row is removed before re-inserting it elsewhere
        // in the list.  If the source is before the dest, the dest position
        // moves
        dst_index -= 1;
    }
    g_object_ref(src_row);

    gtk_container_remove(GTK_CONTAINER(lb), src_row);
    gtk_list_box_insert(lb, src_row, dst_index);
    g_object_unref(src_row);

    GhbValue   * queueDict;

    queueDict = ghb_array_get(ud->queue, src_index);
    ghb_value_incref(queueDict);
    ghb_array_remove(ud->queue, src_index);
    ghb_array_insert(ud->queue, dst_index, queueDict);

    // Force refresh of current selection
    gtk_list_box_unselect_row(lb, GTK_LIST_BOX_ROW(src_row));
    gtk_list_box_select_row(lb, GTK_LIST_BOX_ROW(src_row));

    ghb_save_queue(ud->queue);
}
