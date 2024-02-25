/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * queuehandler.c
 * Copyright (C) John Stebbins 2008-2024 <stebbins@stebbins>
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
 * along with queuehandler.c.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#include "compat.h"
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "handbrake/handbrake.h"
#include "settings.h"
#include "application.h"
#include "jobdict.h"
#include "titledict.h"
#include "hb-backend.h"
#include "values.h"
#include "callbacks.h"
#include "presets.h"
#include "audiohandler.h"
#include "subtitlehandler.h"
#include "hb-dvd.h"
#include "queuehandler.h"
#include "title-add.h"
#include "power-manager.h"
#include "notifications.h"

void ghb_queue_buttons_grey (signal_user_data_t *ud);

// Callbacks
G_MODULE_EXPORT void
queue_remove_clicked_cb (GtkWidget *widget, signal_user_data_t *ud);

#if GTK_CHECK_VERSION(4, 4, 0)
G_MODULE_EXPORT void
queue_drag_begin_cb (GtkWidget * widget, GdkDrag * context,
                     signal_user_data_t * ud);
G_MODULE_EXPORT void
queue_drag_end_cb (GtkWidget * widget, GdkDrag * context,
                   signal_user_data_t * ud);
G_MODULE_EXPORT void
queue_drag_data_get_cb (GtkWidget * widget, GdkDrag * context,
                        GtkSelectionData * selection_data,
                        signal_user_data_t * ud);

G_MODULE_EXPORT gboolean
queue_row_key_cb (GtkEventControllerKey * keycon, guint keyval,
                  guint keycode, GdkModifierType state,
                  signal_user_data_t * ud);
#else
G_MODULE_EXPORT void
queue_drag_begin_cb (GtkWidget * widget, GdkDragContext * context,
                     signal_user_data_t * ud);
G_MODULE_EXPORT void
queue_drag_end_cb (GtkWidget * widget, GdkDragContext * context,
                   signal_user_data_t * ud);
G_MODULE_EXPORT void
queue_drag_data_get_cb (GtkWidget * widget, GdkDragContext * context,
                        GtkSelectionData * selection_data,
                        guint info, guint time, signal_user_data_t * ud);
#endif

#if GTK_CHECK_VERSION(4, 4, 0)
static const char * queue_drag_entries[] = {
    "application/queue-list-row-drop"
};

void ghb_queue_drag_n_drop_init (signal_user_data_t * ud)
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

void ghb_queue_drag_n_drop_init (signal_user_data_t * ud)
{
    GtkWidget * widget;

    widget = GHB_WIDGET(ud->builder, "queue_list");
    gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_MOTION|GTK_DEST_DEFAULT_DROP,
                      queue_drag_entries, 1, GDK_ACTION_MOVE);
}
#endif

static void queue_update_summary (GhbValue * queueDict, signal_user_data_t *ud)
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
        g_string_append_printf(str, "%s %" PRId64 " – %" PRId64,
                               _("Chapters:"),
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
                               "%s %02d:%02d:%05.2f – %02d:%02d:%05.2f",
                               _("Time:"),
                                start_hh, start_mm, start_ss,
                                end_hh, end_mm, end_ss);
    }
    else if (!strcmp(rangeType, "frame"))
    {
        g_string_append_printf(str, "%s %" PRId64 " – %" PRId64,
                               _("Frames:"),
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
        http = ghb_dict_get_bool(uiDict, "Optimize");
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
    gboolean             multi_pass, vqtype;

    str = g_string_new("");
    video_encoder = ghb_settings_video_encoder(uiDict, "VideoEncoder");
    vqtype = ghb_dict_get_bool(uiDict, "vquality_type_constant");
    multi_pass = ghb_dict_get_bool(uiDict, "VideoMultiPass");

    if (!vqtype)
    {
        // ABR
        int br = ghb_dict_get_int(uiDict, "VideoAvgBitrate");
        if (!multi_pass)
        {
            g_string_append_printf(str, _("%s, Bitrate %dkbps"),
                                   video_encoder->name, br);
        }
        else
        {
            g_string_append_printf(str, _("%s, Bitrate %dkbps (Multi Pass)"),
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
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
        sep = ", ";
    }
    if (comb_detect)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_COMB_DETECT);
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
        sep = ", ";
    }
    if (yadif)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_YADIF);
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
        sep = ", ";
    }
    if (bwdif)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_BWDIF);
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
        sep = ", ";
    }
    if (decomb)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_DECOMB);
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
        sep = ", ";
    }
    if (deblock)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_DEBLOCK);
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
        sep = ", ";
    }
    if (nlmeans)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_NLMEANS);
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
        sep = ", ";
    }
    if (denoise)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_DENOISE);
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
        sep = ", ";
    }
    if (chroma_smooth)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_CHROMA_SMOOTH);
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
        sep = ", ";
    }
    if (unsharp)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_UNSHARP);
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
        sep = ", ";
    }
    if (lapsharp)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_LAPSHARP);
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
        sep = ", ";
    }
    if (rot || hflip)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_ROTATE);
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
        sep = ", ";
    }
    if (gray)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_GRAYSCALE);
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
        sep = ", ";
    }
    if (colorspace)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_COLORSPACE);
        g_string_append_printf(str, "%s%s", sep, ghb_get_filter_name(filter));
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
        int                  gain;
        int                  track;
        int                  bitrate;

        asettings     = ghb_array_get(audioList, ii);
        track         = ghb_dict_get_int(asettings, "Track");
        asource       = ghb_array_get(sourceAudioList, track);
        lang          = ghb_dict_get_string(asource, "Language");
        name          = ghb_dict_get_string(asettings, "Name");
        gain          = ghb_dict_get_int(asettings, "Gain");
        bitrate       = ghb_dict_get_int(asettings, "Bitrate");
        audio_encoder = ghb_settings_audio_encoder(asettings, "Encoder");
        if (name)
        {
            g_string_append_printf(str, "%s%s - ", sep, name);
        }
        else
        {
            g_string_append_printf(str, "%s", sep);
        }
        if (audio_encoder->codec & HB_ACODEC_PASS_FLAG)
        {
            g_string_append_printf(str, "%s, %s", lang,
                                   audio_encoder->name);
        }
        else
        {
            audio_mix = ghb_settings_mixdown(asettings, "Mixdown");
            g_string_append_printf(str, "%s, %d %s %s, %s", lang,
                                   bitrate, _("kbps"),
                                   audio_encoder->name, audio_mix->name);
        }
        if (gain)
        {
            g_string_append_printf(str, ", %ddB %s", gain, _("Gain"));
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
        name  = ghb_dict_get_string(searchDict, "Name");

        if (name)
        {
            g_string_append_printf(str, "%s - ", name);
        }
        g_string_append_printf(str, "%s", _("Foreign Audio Scan"));
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
        name        = ghb_dict_get_string(subsettings, "Name");

        g_string_append_printf(str, "%s", sep);
        if (name)
        {
            g_string_append_printf(str, "%s - ", name);
        }
        g_string_append_printf(str, "%s", desc);
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

static void
queue_update_stats (GhbValue * queueDict, signal_user_data_t *ud)
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
        gtk_label_set_text(label, _("Pending"));
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

    const char *path;
    GStatBuf stbuf;

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

static void
queue_update_current_stats (signal_user_data_t * ud)
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
static void read_log (signal_user_data_t * ud, const char * log_path)
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
void ghb_queue_select_log (signal_user_data_t * ud)
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

void ghb_queue_selection_init (signal_user_data_t * ud)
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

char *ghb_subtitle_short_description (const GhbValue *subsource,
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

void ghb_queue_progress_set_visible (signal_user_data_t *ud,
                                     int index, gboolean visible)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;

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
    ghb_queue_row_set_progress_bar_visible(GHB_QUEUE_ROW(row), visible);
}

void ghb_queue_progress_set_fraction (signal_user_data_t *ud,
                                      int index, gdouble frac)
{
    GtkListBox     * lb;
    GtkListBoxRow  * row;

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
    ghb_queue_row_set_progress(GHB_QUEUE_ROW(row), frac);
}

void ghb_queue_update_live_stats (signal_user_data_t * ud, int index,
                                  ghb_instance_status_t * status)
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

            case HB_PASS_ENCODE_ANALYSIS:
                pass = _("Encode Analysis Pass");
                break;

            case HB_PASS_ENCODE_FINAL:
                pass = _("Encode Final Pass");
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

    const char *path;
    GStatBuf stbuf;

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

void ghb_queue_update_status_icon (signal_user_data_t *ud, int index)
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
    GtkListBox    * lb;
    GtkListBoxRow * row;

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_row_at_index(lb, index);
    if (row == NULL) // should never happen
    {
        return;
    }
    ghb_queue_row_set_status(GHB_QUEUE_ROW(row), status);
}

void ghb_queue_update_status (signal_user_data_t *ud, int index, int status)
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

static void
queue_update_all_status (signal_user_data_t *ud, int status)
{
    int count, ii;

    count = ghb_array_len(ud->queue);
    for (ii = 0; ii < count; ii++)
    {
        ghb_queue_update_status(ud, ii, status);
    }
}

static void
save_queue_file_cb (GtkFileChooser *chooser, gint response,
                    signal_user_data_t *ud)
{
    if (response == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename(chooser);
 
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
    gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(chooser));
}

static void save_queue_file (signal_user_data_t *ud)
{
    GtkFileChooserNative *chooser;
    GtkWindow *hb_window;

    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    chooser = gtk_file_chooser_native_new(_("Export Queue"),
                      hb_window,
                      GTK_FILE_CHOOSER_ACTION_SAVE,
                      GHB_STOCK_SAVE,
                      GHB_STOCK_CANCEL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser), "queue.json");
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser), TRUE);
    ghb_file_chooser_set_initial_file(GTK_FILE_CHOOSER(chooser),
                                      ghb_dict_get_string(ud->prefs, "ExportDirectory"));

    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(chooser), TRUE);
    g_signal_connect(G_OBJECT(chooser), "response", G_CALLBACK(save_queue_file_cb), ud);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser));
}

void ghb_add_to_queue_list (signal_user_data_t *ud, GhbValue *queueDict)
{
    GtkListBox *lb;
    GtkWidget  *row;
    GhbValue   *uiDict;
    const char *dest;

    lb     = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    uiDict = ghb_dict_get(queueDict, "uiSettings");

    dest = ghb_dict_get_string(uiDict, "destination");
    row  = ghb_queue_row_new(dest, GHB_QUEUE_STATUS_READY, ud);

    gtk_list_box_insert(lb, row, -1);

    // set row as a source for drag & drop
#if GTK_CHECK_VERSION(4, 4, 0)
    GdkContentFormats * targets;

    targets = gdk_content_formats_new(queue_drag_entries,
                                      G_N_ELEMENTS(queue_drag_entries));
    gtk_drag_source_set(ebox, GDK_BUTTON1_MASK, targets, GDK_ACTION_MOVE);
    gdk_content_formats_unref(targets);
#else
    //gtk_drag_source_set(ebox, GDK_BUTTON1_MASK, queue_drag_entries, 1,
    //                    GDK_ACTION_MOVE);
#endif
    //g_signal_connect(ebox, "drag-begin", G_CALLBACK(queue_drag_begin_cb), NULL);
    //g_signal_connect(ebox, "drag-end", G_CALLBACK(queue_drag_end_cb), NULL);
    //g_signal_connect(ebox, "drag-data-get",
    //                G_CALLBACK(queue_drag_data_get_cb), NULL);

#if GTK_CHECK_VERSION(4, 4, 0)
    // connect key event controller to capture "delete" key press on row
    GtkEventController * econ = gtk_event_controller_key_new();
    gtk_widget_add_controller(row, econ);
    g_signal_connect(econ, "key-pressed", G_CALLBACK(queue_row_key_cb), ud);
#endif
}

static void
open_queue_file_cb (GtkFileChooser *chooser, gint response,
                    signal_user_data_t *ud)
{
    GFile *file;
    char *filename;
    GhbValue *queue;

    file = gtk_file_chooser_get_file(chooser);

    if (response != GTK_RESPONSE_ACCEPT || file == NULL)
    {
        gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(chooser));
        return;
    }

    filename = g_file_get_path(file);
    g_object_unref(file);
    gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(chooser));

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
            ghb_add_to_queue_list(ud, queueDict);
        }
        ghb_queue_buttons_grey(ud);
        ghb_save_queue(ud->queue);
        ghb_value_free(&queue);
    }
    g_free (filename);
}

static void open_queue_file (signal_user_data_t *ud)
{
    GtkFileChooserNative *chooser;
    GtkWindow *hb_window;

    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    chooser = gtk_file_chooser_native_new(_("Import Queue"),
                      hb_window,
                      GTK_FILE_CHOOSER_ACTION_OPEN,
                      GHB_STOCK_OPEN,
                      GHB_STOCK_CANCEL);

    // Add filters
    ghb_add_file_filter(GTK_FILE_CHOOSER(chooser), ud, _("All Files"), "FilterAll");
    ghb_add_file_filter(GTK_FILE_CHOOSER(chooser), ud, g_content_type_get_description("application/json"), "FilterJSON");
    ghb_file_chooser_set_initial_file(GTK_FILE_CHOOSER(chooser),
                                      ghb_dict_get_string(ud->prefs, "ExportDirectory"));

    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(chooser), TRUE);
    g_signal_connect(G_OBJECT(chooser), "response", G_CALLBACK(open_queue_file_cb), ud);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser));
}

static gint64 dest_free_space (GhbValue *settings)
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

gint ghb_find_queue_job (GhbValue *queue, gint unique_id, GhbValue **job)
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

static void
low_disk_check_response_cb (GtkDialog *dialog, int response,
                            signal_user_data_t *ud)
{
    g_signal_handlers_disconnect_by_data(dialog, ud);
    gtk_widget_destroy(GTK_WIDGET(dialog));
    ghb_withdraw_notification(GHB_NOTIFY_PAUSED_LOW_DISK_SPACE);
    switch (response)
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

void ghb_low_disk_check (signal_user_data_t *ud)
{
    GtkWindow       *hb_window;
    GtkWidget       *dialog, *cancel;
    ghb_status_t     status;
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
    free_size = dest_free_space(settings);
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

    ghb_pause_queue();
    ghb_send_notification(GHB_NOTIFY_PAUSED_LOW_DISK_SPACE,
                          free_size / (1024 * 1024), ud);
    dest      = ghb_dict_get_string(settings, "destination");
    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    dialog    = gtk_message_dialog_new(hb_window, GTK_DIALOG_MODAL,
                    GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
                    _("Low Disk Space: Encoding Paused"));
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
        _("The destination filesystem is almost full: %"PRId64" MB free.\n"
          "Destination: %s\n"
          "Encode may be incomplete if you proceed."),
        free_size / (1024 * 1024), dest);
    gtk_dialog_add_buttons( GTK_DIALOG(dialog),
                           _("Resume, I've fixed the problem"), 1,
                           _("Resume, Don't tell me again"), 2,
                           _("Cancel Current and Stop"), 3,
                           NULL);

    cancel = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), 3);
    style = gtk_widget_get_style_context(cancel);
    gtk_style_context_add_class(style, "destructive-action");
    g_signal_connect(dialog, "response",
                     G_CALLBACK(low_disk_check_response_cb), ud);
    gtk_widget_set_visible(dialog, TRUE);
}

static int queue_remove_index     = -1;
static int queue_remove_unique_id = -1;

static void
queue_remove_response (GtkWidget *dialog, int response, signal_user_data_t *ud)
{
    if (dialog != NULL)
    {
        gtk_widget_destroy(dialog);
    }

    if (response != 1 || queue_remove_index < 0)
    {
        return;
    }

    if (queue_remove_unique_id >= 0)
    {
        ghb_stop_queue();
        ud->cancel_encode = GHB_CANCEL_ALL;
        ghb_remove_job(queue_remove_unique_id);
    }
    ghb_array_remove(ud->queue, queue_remove_index);

    // Update UI
    GtkListBox    *lb  = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    GtkListBoxRow *row = gtk_list_box_get_row_at_index(lb, queue_remove_index);
    gtk_container_remove(GTK_CONTAINER(lb), GTK_WIDGET(row));

    queue_remove_index = -1;
}

static void
queue_remove_dialog_show (signal_user_data_t *ud)
{
    GtkWidget *dialog;
    GtkWindow *queue_window;

    queue_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "queue_window"));
    dialog = ghb_cancel_dialog_new(queue_window, _("Remove Item in Progress?"),
                _("Your movie will be lost if you don't continue encoding."),
                _("Cancel and Remove"), NULL, NULL, _("Continue Encoding"));

    g_signal_connect(dialog, "response", G_CALLBACK(queue_remove_response), ud);
    gtk_widget_show(dialog);
}

static void
ghb_queue_remove_row_internal (signal_user_data_t *ud, int index)
{
    GhbValue *queueDict, *uiDict;

    if (index < 0 || index >= ghb_array_len(ud->queue))
    {
        return;
    }

    queue_remove_index = index;

    queueDict  = ghb_array_get(ud->queue, index);
    uiDict     = ghb_dict_get(queueDict, "uiSettings");
    int status = ghb_dict_get_int(uiDict, "job_status");
    if (status == GHB_QUEUE_RUNNING)
    {
        // Ask if wants to stop encode.
        queue_remove_unique_id = ghb_dict_get_int(uiDict, "job_unique_id");
        queue_remove_dialog_show(ud);
    }
    else
    {
        queue_remove_response(NULL, 1, ud);
    }
}

void
ghb_queue_remove_row (signal_user_data_t *ud, int index)
{
    ghb_queue_remove_row_internal(ud, index);
    ghb_save_queue(ud->queue);
    ghb_queue_buttons_grey(ud);
}

void
ghb_queue_buttons_grey (signal_user_data_t *ud)
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

    action = GHB_APPLICATION_ACTION("queue-export");
    g_simple_action_set_enabled(action, !!queue_count);
    action = GHB_APPLICATION_ACTION("add-current");
    g_simple_action_set_enabled(action, allow_add);
    action = GHB_APPLICATION_ACTION("add-multiple");
    g_simple_action_set_enabled(action, allow_add);
    action = GHB_APPLICATION_ACTION("add-all");
    g_simple_action_set_enabled(action, allow_add);
    action = GHB_APPLICATION_ACTION("queue-start");
    g_simple_action_set_enabled(action, allow_start || show_stop);
    action = GHB_APPLICATION_ACTION("queue-pause");
    g_simple_action_set_enabled(action, show_stop);

    action = GHB_APPLICATION_ACTION("queue-reset");
    g_simple_action_set_enabled(action, row != NULL);

    action = GHB_APPLICATION_ACTION("queue-edit");
    g_simple_action_set_enabled(action, row != NULL);

    action = GHB_APPLICATION_ACTION("queue-move-top");
    g_simple_action_set_enabled(action, row != NULL);

    action = GHB_APPLICATION_ACTION("queue-move-bottom");
    g_simple_action_set_enabled(action, row != NULL);

    action = GHB_APPLICATION_ACTION("queue-open-source");
    g_simple_action_set_enabled(action, row != NULL);

    action = GHB_APPLICATION_ACTION("queue-open-dest");
    g_simple_action_set_enabled(action, row != NULL);

    action = GHB_APPLICATION_ACTION("queue-play-file");
    g_simple_action_set_enabled(action, status == GHB_QUEUE_DONE);

    action = GHB_APPLICATION_ACTION("queue-open-log-dir");
    g_simple_action_set_enabled(action, status != GHB_QUEUE_PENDING);

    action = GHB_APPLICATION_ACTION("queue-open-log");
    g_simple_action_set_enabled(action, status != GHB_QUEUE_PENDING);

    action = GHB_APPLICATION_ACTION("chapters-import");
    g_simple_action_set_enabled(action, allow_add);

    action = GHB_APPLICATION_ACTION("chapters-export");
    g_simple_action_set_enabled(action, allow_add);

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
        g_object_unref(item);
    }
    else
    {
        item = g_menu_item_new_from_model(G_MENU_MODEL(menu), 0);
        g_menu_item_set_label(item, _("_Start Encoding"));
        g_menu_remove(menu, 0);
        g_menu_prepend_item(menu, item);
        g_object_unref(item);
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
        g_menu_item_set_label(item, _("_Resume Encoding"));
        g_menu_remove(menu, 1);
        g_menu_append_item(menu, item);
        g_object_unref(item);
    }
    else
    {
        item = g_menu_item_new_from_model(G_MENU_MODEL(menu), 1);
        g_menu_item_set_label(item, _("_Pause Encoding"));
        g_menu_remove(menu, 1);
        g_menu_append_item(menu, item);
        g_object_unref(item);
    }
}

gboolean
ghb_reload_queue (signal_user_data_t *ud)
{
    GhbValue *queue;
    gint count, ii;
    gint pid;
    gint status;
    GhbValue *queueDict, *uiDict;

    ghb_log_func();

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
        GtkWidget *widget = GHB_WIDGET(ud->builder, "queue_window");
        gtk_window_present(GTK_WINDOW(widget));
        ud->queue = queue;
        for (ii = 0; ii < count; ii++)
        {
            queueDict = ghb_array_get(queue, ii);
            uiDict = ghb_dict_get(queueDict, "uiSettings");
            ghb_dict_set_int(uiDict, "job_unique_id", 0);
            ghb_dict_set_int(uiDict, "job_status", GHB_QUEUE_PENDING);
            ghb_add_to_queue_list(ud, queueDict);
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
queue_row_key (guint keyval, signal_user_data_t * ud)
{
    if (keyval != GDK_KEY_Delete)
        return FALSE;

    g_action_activate(GHB_ACTION(ud->builder, "queue-delete"), NULL);
    return TRUE;
}

#if GTK_CHECK_VERSION(4, 4, 0)
G_MODULE_EXPORT gboolean
queue_row_key_cb (GtkEventControllerKey *keycon,
                  guint                  keyval,
                  guint                  keycode,
                  GdkModifierType        state,
                  signal_user_data_t    *ud)
{
    return queue_row_key(keyval, ud);
}

#else
G_MODULE_EXPORT gboolean
queue_key_press_cb (GtkWidget          *widget,
                    GdkEvent           *event,
                    signal_user_data_t *ud)
{
    guint           keyval;

    ghb_event_get_keyval(event, &keyval);
    return queue_row_key(keyval, ud);
}
#endif

G_MODULE_EXPORT void
queue_button_press_cb (GtkGesture *gest, gint n_press, gdouble x, gdouble y,
                       signal_user_data_t *ud)
{
    GtkListBox *lb;
    GtkListBoxRow *row;
    gint button;

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
	button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gest));


    if (button == 1 && n_press == 2)
    {
        row = gtk_list_box_get_row_at_y(lb, y);
        if (row)
            g_action_activate(GHB_ACTION(ud->builder, "queue-play-file"), NULL);
    }
    if (button == 3 && n_press == 1)
    {
        row = gtk_list_box_get_row_at_y(lb, y);
        if (!row)
        {
            return;
        }
        if (!gtk_list_box_row_is_selected(row))
        {
            gtk_list_box_unselect_all(lb);
            gtk_list_box_select_row(lb, row);
        }
        GtkMenu *context_menu = GTK_MENU(GHB_WIDGET(ud->builder, "queue_list_menu"));
        gtk_menu_popup_at_pointer(context_menu, NULL);
    }
}

G_MODULE_EXPORT void
show_queue_action_cb (GSimpleAction *action, GVariant *value,
                      signal_user_data_t *ud)
{
    GtkWidget *queue_window = GHB_WIDGET(ud->builder, "queue_window");
    gtk_window_present(GTK_WINDOW(queue_window));
}

G_MODULE_EXPORT gboolean
queue_window_delete_cb(
    GtkWidget *xwidget,
#if !GTK_CHECK_VERSION(4, 4, 0)
    GdkEvent *event,
#endif
    signal_user_data_t *ud)
{
    gtk_widget_set_visible(xwidget, FALSE);
    return TRUE;
}

GhbValue *ghb_queue_edit_settings = NULL;

G_MODULE_EXPORT void
queue_edit_action_cb (GSimpleAction *action, GVariant *param,
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

        GtkWidget *widget = GHB_WIDGET(ud->builder, "hb_window");
        gtk_window_present(GTK_WINDOW(widget));
    }
}

G_MODULE_EXPORT void
queue_export_action_cb (GSimpleAction *action, GVariant *param,
                        signal_user_data_t *ud)
{
    save_queue_file(ud);
}

G_MODULE_EXPORT void
queue_import_action_cb (GSimpleAction *action, GVariant *param,
                        signal_user_data_t *ud)
{
    open_queue_file(ud);
}

G_MODULE_EXPORT void
queue_open_clicked_cb (GtkWidget *widget, signal_user_data_t *ud)
{
    open_queue_file(ud);
}

G_MODULE_EXPORT void
queue_open_source_action_cb (GSimpleAction *action, GVariant *param,
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
queue_open_dest_action_cb (GSimpleAction *action, GVariant *param,
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
queue_open_log_action_cb (GSimpleAction *action, GVariant *param,
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
queue_open_log_dir_action_cb (GSimpleAction *action, GVariant *param,
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
queue_play_file_action_cb (GSimpleAction *action, GVariant *param,
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
        path      = ghb_dict_get_string(uiDict, "destination");
        str       = g_string_new(NULL);
        g_string_printf(str, "file://%s", path);
        uri       = g_string_free(str, FALSE);
        ghb_browse_uri(ud, uri);
        g_free(uri);
    }
}

G_MODULE_EXPORT void
queue_list_selection_changed_cb (GtkListBox *box, GtkListBoxRow *row,
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
queue_delete_all_action_cb (GSimpleAction *action, GVariant *param,
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
queue_delete_complete_action_cb (GSimpleAction *action, GVariant *param,
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
queue_reset_all_action_cb (GSimpleAction *action, GVariant *param,
                           signal_user_data_t *ud)
{
    queue_update_all_status(ud, GHB_QUEUE_PENDING);
    ghb_save_queue(ud->queue);
    queue_update_current_stats(ud);
    ghb_queue_select_log(ud);
    ghb_update_pending(ud);
    ghb_queue_buttons_grey(ud);
}

G_MODULE_EXPORT void
queue_reset_fail_action_cb (GSimpleAction *action, GVariant *param,
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
queue_reset_action_cb (GSimpleAction *action, GVariant *param,
                       signal_user_data_t *ud)
{
    GList         * rows, * r;
    GtkListBox    * lb;
    GtkListBoxRow * row;

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    rows = r = gtk_list_box_get_selected_rows(lb);

    while (r != NULL)
    {
        row = (GtkListBoxRow *)r->data;
        ghb_queue_update_status(ud, gtk_list_box_row_get_index(row), GHB_QUEUE_PENDING);
        r = r->next;
    }
    ghb_save_queue(ud->queue);
    queue_update_current_stats(ud);
    ghb_queue_select_log(ud);
    ghb_update_pending(ud);
    ghb_queue_buttons_grey(ud);
    g_list_free(rows);
}

G_MODULE_EXPORT void
queue_delete_action_cb (GSimpleAction *action, GVariant *param,
                        signal_user_data_t *ud)
{
    GList         * rows, * r;
    GtkListBox    * lb;
    GtkListBoxRow * row;

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    rows = r = gtk_list_box_get_selected_rows(lb);

    while (r != NULL)
    {
        row = (GtkListBoxRow *)r->data;
        ghb_queue_remove_row_internal(ud, gtk_list_box_row_get_index(row));
        r = r->next;
    }

    ghb_save_queue(ud->queue);
    ghb_update_pending(ud);
    ghb_queue_buttons_grey(ud);
    g_list_free(rows);
}

G_MODULE_EXPORT void
ghb_queue_row_remove (GhbQueueRow *row, signal_user_data_t *ud)
{

    if (row != NULL)
    {
        int index = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row));
        ghb_queue_remove_row_internal(ud, index);
        ghb_save_queue(ud->queue);
    }
    ghb_update_pending(ud);
    ghb_queue_buttons_grey(ud);
}

G_MODULE_EXPORT void
queue_start_action_cb (GSimpleAction *action, GVariant *param,
                       signal_user_data_t *ud)
{
    GhbValue *queueDict, *uiDict;
    gboolean running = FALSE;
    gint count, ii;
    gint status;
    gint state;

    ghb_power_manager_reset();
    state = ghb_get_queue_state();
    if (state & (GHB_STATE_WORKING | GHB_STATE_SEARCHING |
                 GHB_STATE_SCANNING | GHB_STATE_MUXING))
    {
        ghb_stop_encode_dialog_show(ud);
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
        if (!ghb_add_title_to_queue(ud, ud->settings, 0))
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
queue_pause_action_cb (GSimpleAction *action, GVariant *param,
                       signal_user_data_t *ud)
{
    ghb_power_manager_reset();
    ghb_pause_resume_queue();
}

// Set up view of row to be dragged
G_MODULE_EXPORT void
#if GTK_CHECK_VERSION(4, 4, 0)
queue_drag_begin_cb (GtkWidget          *widget,
                     GdkDrag            *context,
                     signal_user_data_t *ud)
#else
queue_drag_begin_cb (GtkWidget          *widget,
                     GdkDragContext     *context,
                     signal_user_data_t *ud)
#endif
{
    GtkListBox      * lb;
    GtkWidget       * row;

    row = gtk_widget_get_ancestor(widget, GTK_TYPE_LIST_BOX_ROW);
    lb  = GTK_LIST_BOX(gtk_widget_get_parent(row));
    // If the user started dragging an item which wasn't selected,
    // only that one should be moved. Unselect everything else
    if (!gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row)))
    {
        gtk_list_box_unselect_all(lb);
        gtk_list_box_select_row(lb, GTK_LIST_BOX_ROW(row));
    }

#if GTK_CHECK_VERSION(4, 4, 0)
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
#if GTK_CHECK_VERSION(4, 4, 0)
queue_drag_end_cb (GtkWidget          *widget,
                   GdkDrag            *context,
                   signal_user_data_t *ud)
#else
queue_drag_end_cb (GtkWidget          *widget,
                   GdkDragContext     *context,
                   signal_user_data_t *ud)
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
#if GTK_CHECK_VERSION(4, 4, 0)
queue_drag_data_get_cb (GtkWidget          *widget,
                        GdkDrag            *context,
                        GtkSelectionData   *selection_data,
                        signal_user_data_t *ud)
#else
queue_drag_data_get_cb (GtkWidget          *widget,
                        GdkDragContext     *context,
                        GtkSelectionData   *selection_data,
                        guint               info,
                        guint               time,
                        signal_user_data_t *ud)
#endif
{
    GtkWidget * row;

    row = gtk_widget_get_ancestor(widget, GTK_TYPE_LIST_BOX_ROW);
    gtk_selection_data_set(selection_data,
                       ghb_atom_string("GTK_LIST_BOX_ROW"), 32,
                       (const guchar *)&row, sizeof (gpointer));
}

G_MODULE_EXPORT void
#if GTK_CHECK_VERSION(4, 4, 0)
queue_drag_leave_cb (GtkListBox         * lb,
                     GdkDrop            * ctx,
                     signal_user_data_t * ud)
#else
queue_drag_leave_cb (GtkListBox         * lb,
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

static GtkListBoxRow *get_last_row (GtkListBox *list)
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

static GtkListBoxRow *get_row_before (GtkListBox *list, GtkListBoxRow *row)
{
    int pos = gtk_list_box_row_get_index(row);
    return gtk_list_box_get_row_at_index(list, pos - 1);
}

static GtkListBoxRow *get_row_after (GtkListBox *list, GtkListBoxRow *row)
{
    int pos = gtk_list_box_row_get_index(row);
    return gtk_list_box_get_row_at_index(list, pos + 1);
}

G_MODULE_EXPORT gboolean
#if GTK_CHECK_VERSION(4, 4, 0)
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

static void
queue_move_item (GtkListBox *lb, GtkListBoxRow *row,
                 gint32 dst_index, signal_user_data_t *ud)
{
    gint32     src_index;
    GhbValue * queue_dict;

    src_index = gtk_list_box_row_get_index(row);

    if (src_index < dst_index)
    {
        // The source row is removed before re-inserting it elsewhere
        // in the list.  If the source is before the dest, the dest position
        // moves
        dst_index -= 1;
    }
    g_object_ref(G_OBJECT(row));
    gtk_container_remove(GTK_CONTAINER(lb), GTK_WIDGET(row));
    gtk_list_box_insert(lb, GTK_WIDGET(row), dst_index);
    g_object_unref(G_OBJECT(row));

    queue_dict = ghb_array_get(ud->queue, src_index);
    ghb_value_incref(queue_dict);
    ghb_array_remove(ud->queue, src_index);
    if (dst_index < 0)
        ghb_array_append(ud->queue, queue_dict);
    else
        ghb_array_insert(ud->queue, dst_index, queue_dict);

    // Force refresh of current selection
    gtk_list_box_unselect_row(lb, GTK_LIST_BOX_ROW(row));
    gtk_list_box_select_row(lb, GTK_LIST_BOX_ROW(row));

    ghb_save_queue(ud->queue);
}

G_MODULE_EXPORT void
queue_move_top_action_cb (GSimpleAction *action, GVariant *param,
                          signal_user_data_t *ud)
{
    GList         * rows, * r;
    GtkListBox    * lb;
    GtkListBoxRow * row;
    gint32          move_index;

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    rows = r = gtk_list_box_get_selected_rows(lb);

    move_index = 0;
    while (r != NULL)
    {
        row = (GtkListBoxRow *)r->data;
        queue_move_item(lb, row, move_index, ud);
        r = r->next;
        move_index += 1;
    }

    g_list_free(rows);
}

G_MODULE_EXPORT void
queue_move_bottom_action_cb (GSimpleAction *action, GVariant *param,
                             signal_user_data_t *ud)
{
    GList         * rows, * r;
    GtkListBox    * lb;
    GtkListBoxRow * row;

    lb  = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    rows = r = gtk_list_box_get_selected_rows(lb);

    while (r != NULL)
    {
        row = (GtkListBoxRow *)r->data;
        queue_move_item(lb, row, -1, ud);
        r = r->next;
    }

    g_list_free(rows);
}

G_MODULE_EXPORT void
#if GTK_CHECK_VERSION(4, 4, 0)
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
    GtkWidget     * row_before;
    GtkWidget     * row_after;
    GList         * rows, * r;
    GtkListBoxRow * row;
    gint32          dst_index;

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

    rows = r = gtk_list_box_get_selected_rows(lb);

    while (r != NULL)
    {
        row = GTK_LIST_BOX_ROW(r->data);
        if (row_after)
        {
            dst_index = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row_after));
        }
        else
        {
            dst_index = -1;
        }
        queue_move_item(lb, GTK_LIST_BOX_ROW(row), dst_index, ud);
        r = r->next;
    }
    g_list_free(rows);
}
