/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.c
 * Copyright (C) John Stebbins 2008-2015 <stebbins@stebbins>
 *
 * callbacks.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "ghbcompat.h"
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "hb.h"
#include "settings.h"
#include "hb-backend.h"
#include "values.h"
#include "callbacks.h"
#include "presets.h"
#include "audiohandler.h"
#include "subtitlehandler.h"
#include "ghb-dvd.h"
#include "plist.h"

void ghb_queue_buttons_grey(signal_user_data_t *ud);

G_MODULE_EXPORT void
queue_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
    GtkTreeModel *store;
    GtkTreeIter iter, piter;

    g_debug("queue_list_selection_changed_cb ()");
    // A queue entry is made up of a parent and multiple
    // children that are visible when expanded.  When and entry
    // is selected, I want the parent to be selected.
    // This is purely cosmetic.
    if (gtk_tree_selection_get_selected(selection, &store, &iter))
    {
        GtkWidget *widget = GHB_WIDGET (ud->builder, "queue_edit");
        gtk_widget_set_sensitive (widget, TRUE);
        widget = GHB_WIDGET (ud->builder, "queue_reload");
        gtk_widget_set_sensitive (widget, TRUE);
        if (gtk_tree_model_iter_parent (store, &piter, &iter))
        {
            GtkTreePath *path;
            GtkTreeView *treeview;

            gtk_tree_selection_select_iter (selection, &piter);
            path = gtk_tree_model_get_path (store, &piter);
            treeview = gtk_tree_selection_get_tree_view (selection);
            // Make the parent visible in scroll window if it is not.
            gtk_tree_view_scroll_to_cell (treeview, path, NULL, FALSE, 0, 0);
            gtk_tree_path_free(path);
        }
    }
    else
    {
        GtkWidget *widget = GHB_WIDGET (ud->builder, "queue_edit");
        gtk_widget_set_sensitive (widget, FALSE);
        widget = GHB_WIDGET (ud->builder, "queue_reload");
        gtk_widget_set_sensitive (widget, FALSE);
    }
}

static void
add_to_queue_list(signal_user_data_t *ud, GhbValue *settings, GtkTreeIter *piter)
{
    GtkTreeView *treeview;
    GtkTreeIter iter;
    GtkTreeStore *store;
    gint status;
    GtkTreeIter citer;
    gchar *basename;
    const char *vol_name, *dest;
    gint title, start_point, end_point;
    gboolean two_pass, vqtype;
    gchar *escape, *escape2;
    GString *str = g_string_new("");

#define XPRINT(fmt, ...) \
    g_string_append_printf(str, fmt, ##__VA_ARGS__)

    g_debug("update_queue_list ()");
    if (settings == NULL) return;
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
    store = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));

    // Collect information for first line in the display
    // Volume (Title X, Chapters Y through Z, N Video Passes) --> Destination
    title = ghb_dict_get_int(settings, "title");
    start_point = ghb_dict_get_int(settings, "start_point");
    end_point = ghb_dict_get_int(settings, "end_point");
    vol_name = ghb_dict_get_string(settings, "volume_label");
    dest = ghb_dict_get_string(settings, "destination");
    basename = g_path_get_basename(dest);
    escape = g_markup_escape_text(basename, -1);
    escape2 = g_markup_escape_text(vol_name, -1);

    vqtype = ghb_dict_get_bool(settings, "vquality_type_constant");
    two_pass = ghb_dict_get_bool(settings, "VideoTwoPass");

    const gchar *points = _("Chapters");
    const gchar *ptop = ghb_dict_get_string(settings, "PtoPType");
    if (!strcasecmp(ptop, "chapter"))
        points = _("Chapters");
    if (!strcasecmp(ptop, "time"))
        points = _("Seconds");
    if (!strcasecmp(ptop, "frame"))
        points = _("Frames");

    if (!vqtype && two_pass)
    {
        XPRINT(_("<big><b>%s</b></big> "
                "<small>(Title %d, %s %d through %d, 2 Video Passes)"
                " --> %s</small>"),
                escape2, title, points, start_point, end_point, escape
        );
    }
    else
    {
        XPRINT(_("<big><b>%s</b></big> "
                "<small>(Title %d, %s %d through %d)"
                " --> %s</small>"),
                escape2, title, points, start_point, end_point, escape
        );
    }
    g_free(basename);
    g_free(escape);
    g_free(escape2);

    if (piter)
    {
        iter = *piter;
    }
    else
    {
        gtk_tree_store_append(store, &iter, NULL);
    }

    // Set the job status icon
    status = ghb_dict_get_int(settings, "job_status");
    const char *status_icon;
    switch (status)
    {
        case GHB_QUEUE_PENDING:
            status_icon = "hb-source";
            break;
        case GHB_QUEUE_FAIL:
        case GHB_QUEUE_CANCELED:
            status_icon = "hb-stop";
            break;
        case GHB_QUEUE_DONE:
            status_icon = "hb-complete";
            break;
        default:
            status_icon = "hb-source";
            break;
    }
    // Set the status icon, job description, and delete icon button
    gtk_tree_store_set(store, &iter, 0, FALSE, 1, status_icon, 2, str->str,
                       3, "hb-remove", -1);

    // Reset the string for the next line
    g_string_assign(str, "");

    // Next line in the display
    // Preset: PresetName
    const char *name;
    gboolean markers;
    gboolean preset_modified;

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    preset_modified = ghb_dict_get_bool(settings, "preset_modified");
    name = ghb_dict_get_string(settings, "PresetFullName");
    markers = ghb_dict_get_bool(settings, "ChapterMarkers");

    if (preset_modified)
    {
        XPRINT(_("<b>Modified Preset Based On:</b> <small>%s</small>\n"), name);
    }
    else
    {
        XPRINT(_("<b>Preset:</b> <small>%s</small>\n"), name);
    }

    // Next line in the display (Container type)
    // Format: XXX Container
    XPRINT(_("<b>Format:</b> <small>%s Container</small>\n"), mux->name);

    // Next line in the display (Container options)
    // Container Options: - Chapter Markers
    gboolean ipod = FALSE, http = FALSE;
    if (mux->format & HB_MUX_MASK_MP4)
    {
        ipod = ghb_dict_get_bool(settings, "Mp4iPodCompatible");
        http = ghb_dict_get_bool(settings, "Mp4HttpOptimize");
    }
    if (http || ipod || markers)
    {
        const char *prefix = " ";
        XPRINT(_("<b>Container Options:</b><small>"));
        if (markers)
        {
            XPRINT(_("%sChapter Markers"), prefix);
            prefix = " - ";
        }
        if (ipod)
        {
            XPRINT(_("%siPod 5G Support"), prefix);
            prefix = " - ";
        }
        if (http)
        {
            XPRINT(_("%sWeb Optimized"), prefix);
            prefix = " - ";
        }
        XPRINT("</small>\n");
    }

    // Next line in the display (Destination)
    // Destination: /Full/Destination/Path.mkv
    escape = g_markup_escape_text(dest, -1);
    XPRINT(_("<b>Destination:</b> <small>%s</small>\n"), escape);
    g_free(escape);

    // Next line in the display (Picture settings)
    // Picture: Source: W x H, Output W x H (Animorphic), Display W x H
    const char *pic_par;
    int width, height;
    int crop[4];
    gboolean keep_aspect;

    width = ghb_dict_get_int(settings, "scale_width");
    height = ghb_dict_get_int(settings, "scale_height");
    pic_par = ghb_dict_get_string(settings, "PicturePAR");
    keep_aspect = ghb_dict_get_bool(settings, "PictureKeepRatio");
    crop[0] = ghb_dict_get_int(settings, "PictureTopCrop");
    crop[1] = ghb_dict_get_int(settings, "PictureBottomCrop");
    crop[2] = ghb_dict_get_int(settings, "PictureLeftCrop");
    crop[3] = ghb_dict_get_int(settings, "PictureRightCrop");

    gchar *aspect_desc;
    if (pic_par == NULL || !strcasecmp(pic_par, "off"))
    {
        if (keep_aspect)
        {
            aspect_desc = _("(Aspect Preserved)");
        }
        else
        {
            aspect_desc = _("(Aspect Lost)");
        }
    }
    else if (!strcasecmp(pic_par, "strict") || !strcasecmp(pic_par, "loose"))
    {
        aspect_desc = _("(Anamorphic)");
    }
    else if (!strcasecmp(pic_par, "custom"))
    {
        aspect_desc = _("(Custom Anamorphic)");
    }
    else
    {
        aspect_desc = "";
    }

    gint source_width, source_height;
    source_width = ghb_dict_get_int(settings, "source_width");
    source_height = ghb_dict_get_int(settings, "source_height");
    XPRINT(_("<b>Picture:</b> <small>"));
    XPRINT(_("Source: %d x %d, Output %d x %d %s, Crop %d:%d:%d:%d"),
           source_width, source_height, width, height, aspect_desc,
           crop[0], crop[1], crop[2], crop[3]);
    if (pic_par)
    {
        int display_width, display_height;
        display_width = ghb_dict_get_int(settings, "PictureDisplayWidth");
        display_height = ghb_dict_get_int(settings, "PictureDisplayHeight");
        XPRINT(_(", Display %d x %d"),
                display_width, display_height);
    }
    XPRINT("</small>\n");

    // Next line in the display (Filter settings)
    // Filters: - Deinterlace
    gint deblock, denoise, deint;
    const gchar *deint_preset, *detel_preset, *denoise_preset;
    const gchar *denoise_tune;
    const gchar *deint_cust, *detel_cust, *denoise_cust;
    gchar *deint_opt, *denoise_opt;
    gboolean grayscale, detel, filters;

    deint = ghb_settings_combo_int(settings, "PictureDeinterlaceFilter");
    deint_opt = ghb_settings_combo_option(settings, "PictureDeinterlaceFilter");
    if (deint != HB_FILTER_INVALID)
    {
        deint_preset = ghb_lookup_filter_name(deint,
                ghb_dict_get_string(settings, "PictureDeinterlacePreset"), 1);
        deint_cust = ghb_dict_get_string(settings, "PictureDeinterlaceCustom");
    }

    detel_preset = ghb_dict_get_string(settings, "PictureDetelecine");
    detel = detel_preset != NULL && !!strcasecmp(detel_preset, "off");
    detel_cust = ghb_dict_get_string(settings, "PictureDetelecineCustom");

    deblock = ghb_dict_get_int(settings, "PictureDeblock");

    denoise = ghb_settings_combo_int(settings, "PictureDenoiseFilter");
    denoise_opt = ghb_settings_combo_option(settings, "PictureDenoiseFilter");
    if (denoise != HB_FILTER_INVALID)
    {
        denoise_preset = ghb_lookup_filter_name(denoise,
                    ghb_dict_get_string(settings, "PictureDenoisePreset"), 1);
        denoise_tune = ghb_lookup_filter_name(denoise,
                    ghb_dict_get_string(settings, "PictureDenoiseTune"), 0);
        denoise_cust = ghb_dict_get_string(settings, "PictureDenoiseCustom");
    }

    grayscale = ghb_dict_get_bool(settings, "VideoGrayScale");

    filters = detel || grayscale ||
              deint   != HB_FILTER_INVALID ||
              denoise != HB_FILTER_INVALID ||
              (deblock >= 5);
    if (filters)
    {
        const char *prefix = " ";
        XPRINT(_("<b>Filters:</b><small>"));
        if (detel)
        {
            XPRINT(_("%sDetelecine"), prefix);
            if (!strcasecmp(detel_preset, "custom"))
            {
                XPRINT(": %s", detel_cust);
            }
            prefix = " - ";
        }
        if (deint != HB_FILTER_INVALID)
        {
            XPRINT(_("%s%s:"), prefix, deint_opt);
            const char *preset;
            preset = ghb_dict_get_string(settings, "PictureDeinterlacePreset");
            if (!strcasecmp(preset, "custom"))
            {
                XPRINT(": %s", deint_cust);
            }
            else
            {
                XPRINT(" %s", deint_preset);
            }
            prefix = " - ";
        }
        if (denoise != HB_FILTER_INVALID)
        {
            XPRINT(_("%sDenoise %s:"), prefix, denoise_opt);
            const char *preset;
            preset = ghb_dict_get_string(settings, "PictureDenoisePreset");
            if (preset && !strcasecmp(preset, "custom"))
            {
                XPRINT(" %s", denoise_cust);
            }
            else
            {
                XPRINT(" %s", denoise_preset);
                const char *tune;
                tune = ghb_dict_get_string(settings, "PictureDenoiseTune");
                if (denoise == HB_FILTER_NLMEANS && denoise_tune != NULL &&
                    tune != NULL && strcasecmp(tune, "none"))
                {
                    XPRINT(",%s", denoise_tune);
                }
            }
            prefix = " - ";
        }
        if (deblock >= 5)
        {
            XPRINT(_("%sDeblock: %d"), prefix, deblock);
            prefix = " - ";
        }
        if (grayscale)
        {
            XPRINT(_("%sGrayscale"), prefix);
            prefix = " - ";
        }
        XPRINT("</small>\n");
    }
    free(deint_opt);
    free(denoise_opt);

    // Next line in the display (Video Encoder)
    // Video: Encoder, Framerate: fps, RF/Bitrate/QP
    const hb_encoder_t *video_encoder;
    video_encoder = ghb_settings_video_encoder(settings, "VideoEncoder");

    XPRINT(_("<b>Video:</b> <small>%s"), video_encoder->name);

    const hb_rate_t *fps;
    fps = ghb_settings_video_framerate(settings, "VideoFramerate");
    if (fps->rate == 0)
    {
        const char *rate_mode;
        if (ghb_dict_get_bool(settings, "VideoFramerateCFR"))
            rate_mode = _("(constant)");
        else
            rate_mode = _("(variable)");
        XPRINT(_(", Framerate: %s %s"), fps->name, rate_mode);
    }
    else
    {
        if (ghb_dict_get_bool(settings, "VideoFrameratePFR"))
        {
            XPRINT(_(", Framerate: Peak %s (may be lower)"), fps->name);
        }
        else
        {
            XPRINT(_(", Framerate: %s (constant frame rate)"), fps->name);
        }
    }
    const gchar *vq_desc = _("Error");
    const gchar *vq_units = "";
    gdouble vqvalue;
    if (!vqtype)
    {
        // Has to be bitrate
        vqvalue = ghb_dict_get_int(settings, "VideoAvgBitrate");
        vq_desc = _("Bitrate:");
        vq_units = _("kbps");
        XPRINT(", %s %d%s",
               vq_desc, (int)vqvalue, vq_units);
    }
    else
    {
        // Constant quality
        vqvalue = ghb_dict_get_double(settings, "VideoQualitySlider");
        vq_desc = _("Constant Quality:");
        vq_units = hb_video_quality_get_name(video_encoder->codec);
        XPRINT(", %s %.4g(%s)",
               vq_desc, vqvalue, vq_units);
    }
    XPRINT("</small>\n");

    // Next line in the display (Turbo setting)
    gboolean turbo;
    turbo = ghb_dict_get_bool(settings, "VideoTurboTwoPass");
    if (!vqtype && two_pass && turbo)
    {
        XPRINT(_("<b>Turbo 1st Pass:</b> <small>On</small>\n"));
    }

    // Next line in the display (Video Encoder Options)
    // Video Options: Preset - Tune - Profile - Level
    if ((video_encoder->codec & HB_VCODEC_X264_MASK) &&
        !ghb_dict_get_bool(settings, "x264UseAdvancedOptions"))
    {
        const gchar *extra_opt = NULL;

        // If the encoder supports presets...
        if (hb_video_encoder_get_presets(video_encoder->codec) != NULL)
        {
            const gchar *preset_opt, *tune_opt;
            const gchar *profile_opt, *level_opt;
            gboolean fastdecode, zerolatency;

            preset_opt = ghb_dict_get_string(settings, "VideoPreset");
            tune_opt = ghb_dict_get_string(settings, "VideoTune");
            fastdecode = ghb_dict_get_bool(settings, "x264FastDecode");
            zerolatency = ghb_dict_get_bool(settings, "x264ZeroLatency");
            profile_opt = ghb_dict_get_string(settings, "VideoProfile");
            level_opt = ghb_dict_get_string(settings, "VideoLevel");
            extra_opt = ghb_dict_get_string(settings, "VideoOptionExtra");

            XPRINT(_("<b>Video Options:</b> <small>Preset: %s</small>"), preset_opt);
            if ((tune_opt != NULL && tune_opt[0] != 0) || zerolatency || fastdecode)
            {
                const char *prefix = "";
                XPRINT(_("<small> - Tune: "));
                if (tune_opt != NULL && tune_opt[0] != 0)
                {
                    XPRINT("%s%s", prefix, tune_opt);
                    prefix = ",";
                }
                if (video_encoder->codec & HB_VCODEC_X264_MASK)
                {
                    if (fastdecode)
                    {
                        XPRINT("%sfastdecode", prefix);
                        prefix = ",";
                    }
                    if (zerolatency)
                    {
                        XPRINT("%szerolatency", prefix);
                        prefix = ",";
                    }
                }
                XPRINT("</small>");
            }
            if (profile_opt != NULL && profile_opt[0] != 0)
            {
                XPRINT(_("<small> - Profile: %s</small>"), profile_opt);
            }
            if (level_opt != NULL && level_opt[0] != 0)
            {
                XPRINT(_("<small> - Level: %s</small>"), level_opt);
            }
            XPRINT("\n");
        }

        // Next line in the display (Video Encoder Options)
        // Video Advanced Options: detailed settings
        if (extra_opt != NULL && extra_opt[0] != 0)
        {
            XPRINT(_("<b>Advanced Options:</b> <small>%s</small>\n"), extra_opt);
        }
    }
    else if (video_encoder->codec & HB_VCODEC_X264_MASK)
    {
        // Next line in the display (Video Encoder Options)
        // Video Advanced Options: detailed settings
        const gchar *opts = ghb_build_advanced_opts_string(settings);
        if (opts != NULL && opts[0] != 0)
        {
            XPRINT(_("<b>Advanced Options:</b> <small>%s</small>\n"), opts);
        }
    }

    // Next line in the display (Audio)
    // Audio Tracks: count
    //      Source description, Encoder, Mix, Samplerate, Bitrate
    //      ...
    gint count, ii;
    const GhbValue *audio_list;

    audio_list = ghb_get_job_audio_list(settings);
    count = ghb_array_len(audio_list);
    if (count == 1)
    {
        XPRINT(_("<b>Audio:</b> <small>"));
    }
    else if (count > 1)
    {
        XPRINT(_("<b>Audio Tracks: %d</b><small>"), count);
    }
    for (ii = 0; ii < count; ii++)
    {
        const gchar *track;
        gchar *quality = NULL;
        GhbValue *asettings;
        const hb_encoder_t *audio_encoder;

        asettings = ghb_array_get(audio_list, ii);

        audio_encoder = ghb_settings_audio_encoder(asettings, "Encoder");
        double q = ghb_dict_get_double(asettings, "Quality");
        if (ghb_dict_get_bool(asettings, "QualityEnable") &&
            q != HB_INVALID_AUDIO_QUALITY)
        {
            quality = ghb_format_quality(_("Quality: "), audio_encoder->codec, q);
        }
        else
        {
            int br = ghb_dict_get_int(asettings, "Bitrate");
            quality = g_strdup_printf(_("Bitrate: %d"), br);
        }
        const char *sr_name;
        int sr_rate = ghb_dict_get_int(asettings, "Samplerate");
        sr_name = hb_audio_samplerate_get_name(sr_rate);
        if (sr_name == NULL)
            sr_name = "Auto";

        track = ghb_dict_get_string(asettings, "Description");
        const hb_mixdown_t *mix;
        mix = ghb_settings_mixdown(asettings, "Mixdown");
        if (count > 1)
            XPRINT("\n\t");

        if (audio_encoder->codec & HB_ACODEC_PASS_FLAG)
        {
            XPRINT(_("%s --> Encoder: %s"), track, audio_encoder->name);
        }
        else
        {
            XPRINT(_("%s --> Encoder: %s, Mixdown: %s, SampleRate: %s, %s"),
             track, audio_encoder->name, mix->name, sr_name, quality);
        }
        g_free(quality);
    }
    if (count > 0)
    {
        XPRINT("</small>\n");
    }

    // Next line in the display (Subtitle)
    // Subtitle Tracks: count
    //      Subtitle description(Subtitle options)
    //      ...
    const GhbValue *sub_dict, *sub_list, *sub_search;
    gboolean search;

    sub_dict = ghb_get_job_subtitle_settings(settings);
    sub_list = ghb_dict_get(sub_dict, "SubtitleList");
    sub_search = ghb_dict_get(sub_dict, "Search");
    search = ghb_dict_get_bool(sub_search, "Enable");
    count = ghb_array_len(sub_list);
    if (count + search == 1)
    {
        XPRINT(_("<b>Subtitle:</b> "));
    }
    else if (count + search > 1)
    {
        XPRINT(_("<b>Subtitle Tracks: %d</b>\n"), count + search);
    }
    if (search)
    {
        const gchar *track;
        gboolean force, burn, def;

        track = ghb_dict_get_string(sub_search, "Description");
        force = ghb_dict_get_bool(sub_search, "Forced");
        burn  = ghb_dict_get_bool(sub_search, "Burn");
        def   = ghb_dict_get_bool(sub_search, "Default");
        if (count + search > 1)
            XPRINT("\t");
        XPRINT("<small>%s%s%s%s</small>\n", track,
                force ? _(" (Forced Only)") : "",
                burn  ? _(" (Burn)")        : "",
                def   ? _(" (Default)")     : ""
        );
    }
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *subsettings, *srt;
        const gchar *track;
        gboolean force, burn, def;

        subsettings = ghb_array_get(sub_list, ii);
        track  = ghb_dict_get_string(subsettings, "Description");
        srt    = ghb_dict_get(subsettings, "SRT");
        force  = ghb_dict_get_bool(subsettings, "Forced");
        burn   = ghb_dict_get_bool(subsettings, "Burn");
        def    = ghb_dict_get_bool(subsettings, "Default");
        if (count + search > 1)
            XPRINT("\t");

        if (srt == NULL)
        {
            XPRINT("<small>%s%s%s%s</small>\n", track,
                    force ? _(" (Forced Only)") : "",
                    burn  ? _(" (Burn)")        : "",
                    def   ? _(" (Default)")     : ""
            );
        }
        else
        {
            int offset = ghb_dict_get_int(subsettings, "Offset");

            XPRINT(_("<small> %s, Offset (ms) %d%s</small>\n"),
                   track, offset, def ? " (Default)" : "");
        }
    }

    // Remove the final newline in the string
    if (str->len > 0 && str->str[str->len-1] == '\n')
        str->str[str->len-1] = 0;

    gtk_tree_store_append(store, &citer, &iter);
    gtk_tree_store_set(store, &citer, 2, str->str, -1);

    g_string_free(str, TRUE);
}

void
ghb_update_status(signal_user_data_t *ud, int status, int index)
{
    const char *status_icon;
    switch (status)
    {
        case GHB_QUEUE_PENDING:
            status_icon = "hb-source";
            break;
        case GHB_QUEUE_FAIL:
        case GHB_QUEUE_CANCELED:
            status_icon = "hb-stop";
            break;
        case GHB_QUEUE_DONE:
            status_icon = "hb-complete";
            break;
        default:
            status_icon = "hb-source";
            break;
    }
    int count = ghb_array_len(ud->queue);
    if (index < 0 || index >= count)
    {
        // invalid index
        return;
    }

    GhbValue *settings;
    settings = ghb_array_get(ud->queue, index);
    if (settings == NULL) // should never happen
        return;

    if (ghb_dict_get_int(settings, "job_status") == GHB_QUEUE_RUNNING)
        return; // Never change the status of currently running jobs

    GtkTreeView *treeview;
    GtkTreeModel *store;
    GtkTreeIter iter;

    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
    store = gtk_tree_view_get_model(treeview);
    gchar *path = g_strdup_printf ("%d", index);
    if (gtk_tree_model_get_iter_from_string(store, &iter, path))
    {
        gtk_tree_store_set(GTK_TREE_STORE(store), &iter, 1, status_icon, -1);
    }
    g_free(path);

    ghb_dict_set_int(settings, "job_status", status);
}

void
ghb_update_all_status(signal_user_data_t *ud, int status)
{
    int count, ii;

    count = ghb_array_len(ud->queue);
    for (ii = 0; ii < count; ii++)
    {
        ghb_update_status(ud, status, ii);
    }
}

static void
save_queue_file(signal_user_data_t *ud)
{
    int ii, count;
    GhbValue *queue = ghb_value_dup(ud->queue);

    count = ghb_array_len(queue);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *settings = ghb_array_get(ud->queue, ii);
        if (settings == NULL)
            continue;
        ghb_dict_set_int(settings, "job_status", GHB_QUEUE_PENDING);
    }

    GtkWidget *dialog;
    GtkWindow *hb_window;

    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    dialog = gtk_file_chooser_dialog_new("Queue Destination",
                      hb_window,
                      GTK_FILE_CHOOSER_ACTION_SAVE,
                      GHB_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                      GHB_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                      NULL);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "queue.json");
    if (gtk_dialog_run(GTK_DIALOG (dialog)) != GTK_RESPONSE_ACCEPT)
    {
        ghb_value_free(&queue);
        gtk_widget_destroy(dialog);
        return;
    }

    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy(dialog);

    ghb_write_settings_file(filename, queue);
    g_free (filename);
    ghb_value_free(&queue);
}

static void
open_queue_file(signal_user_data_t *ud)
{
    GtkWidget *dialog;
    GtkWindow *hb_window;

    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    dialog = gtk_file_chooser_dialog_new("Queue Destination",
                      hb_window,
                      GTK_FILE_CHOOSER_ACTION_OPEN,
                      GHB_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                      GHB_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                      NULL);

    // Add filters
    GtkFileFilter *filter;
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "QueueFilterAll"));
    gtk_file_filter_set_name(filter, _("All"));
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(chooser, filter);
    filter = GTK_FILE_FILTER(GHB_OBJECT(ud->builder, "QueueFilterJSON"));
    gtk_file_filter_set_name(filter, "JSON");
    gtk_file_filter_add_pattern(filter, "*.JSON");
    gtk_file_filter_add_pattern(filter, "*.json");
    gtk_file_chooser_add_filter(chooser, filter);

    if (gtk_dialog_run(GTK_DIALOG (dialog)) != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_destroy(dialog);
        return;
    }

    GhbValue *queue;
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy(dialog);

    queue = ghb_read_settings_file(filename);
    if (queue != NULL)
    {
        int ii, count;
        count = ghb_array_len(queue);
        for (ii = 0; ii < count; ii++)
        {
            GhbValue *settings = ghb_array_get(queue, ii);
            ghb_value_incref(settings);
            ghb_array_remove(queue, ii);
            ghb_dict_set_int(settings, "job_status", GHB_QUEUE_PENDING);
            ghb_dict_set_int(settings, "job_unique_id", 0);

            if (ud->queue == NULL)
                ud->queue = ghb_array_new();
            ghb_array_append(ud->queue, settings);
            add_to_queue_list(ud, settings, NULL);
        }
        ghb_queue_buttons_grey(ud);
        ghb_save_queue(ud->queue);
        ghb_value_free(&queue);
    }
    g_free (filename);
}

G_MODULE_EXPORT void
queue_save_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    save_queue_file(ud);
}

G_MODULE_EXPORT void
queue_open_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    open_queue_file(ud);
}

G_MODULE_EXPORT void
queue_reload_all_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_update_all_status(ud, GHB_QUEUE_PENDING);
    ghb_save_queue(ud->queue);
    ghb_update_pending(ud);
}

G_MODULE_EXPORT void
queue_reload_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GtkTreeView *treeview;
    GtkTreeSelection *selection;
    GtkTreeModel *store;
    GtkTreeIter iter;
    gint row;
    gint *indices;

    g_debug("queue_key_press_cb ()");
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
    store = gtk_tree_view_get_model(treeview);
    selection = gtk_tree_view_get_selection (treeview);
    if (gtk_tree_selection_get_selected(selection, &store, &iter))
    {
        GtkTreePath *treepath;

        treepath = gtk_tree_model_get_path (store, &iter);
        // Find the entry in the queue
        indices = gtk_tree_path_get_indices (treepath);
        row = indices[0];
        // Can only free the treepath After getting what I need from
        // indices since this points into treepath somewhere.
        gtk_tree_path_free (treepath);
        if (row < 0) return;
        if (row >= ghb_array_len(ud->queue)) return;
        ghb_update_status(ud, GHB_QUEUE_PENDING, row);
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
        GhbValue *js;
        const gchar *filename;

        js = ghb_array_get(ud->queue, ii);
        filename = ghb_dict_get_string(js, "destination");
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
    if (!batch)
    {
        GFile *gfile;
        GFileInfo *info;
        guint64 size;
        gchar *resolved = ghb_resolve_symlink(destdir);

        gfile = g_file_new_for_path(resolved);
        info = g_file_query_filesystem_info(gfile,
                            G_FILE_ATTRIBUTE_FILESYSTEM_FREE, NULL, NULL);
        if (info != NULL)
        {
            if (g_file_info_has_attribute(info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE))
            {
                size = g_file_info_get_attribute_uint64(info,
                                        G_FILE_ATTRIBUTE_FILESYSTEM_FREE);

                gint64 fsize = (guint64)10 * 1024 * 1024 * 1024;
                if (size < fsize)
                {
                    message = g_strdup_printf(
                                _("Destination filesystem is almost full: %uM free\n\n"
                                "Encode may be incomplete if you proceed.\n"),
                                (guint)(size / (1024L*1024L)));
                    if (!ghb_message_dialog(hb_window, GTK_MESSAGE_QUESTION,
                                            message, _("Cancel"), _("Proceed")))
                    {
                        g_free(message);
                        g_free(destdir);
                        return FALSE;
                    }
                    g_free(message);
                }
            }
            g_object_unref(info);
        }
        g_object_unref(gfile);
        g_free(resolved);
    }
    g_free(destdir);
    if (g_file_test(dest, G_FILE_TEST_EXISTS))
    {
        message = g_strdup_printf(
                    _("Destination: %s\n\n"
                    "File already exists.\n"
                    "Do you want to overwrite?"),
                    dest);
        if (!ghb_message_dialog(hb_window, GTK_MESSAGE_QUESTION,
                                message, _("Cancel"), _("Overwrite")))
        {
            g_free(message);
            return FALSE;
        }
        g_free(message);
        g_unlink(dest);
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

    // Apply selected preset settings
    hb_preset_apply_mux(preset, job);
    hb_preset_apply_video(preset, job);
    hb_preset_apply_filters(preset, job);

    // Add scale filter since the above does not
    GhbValue *filter_list, *filter_dict;
    int width, height, crop[4];
    char *filter_str;

    filter_list = ghb_get_job_filter_list(settings);
    width = ghb_dict_get_int(settings, "scale_width");
    height = ghb_dict_get_int(settings, "scale_height");

    crop[0] = ghb_dict_get_int(settings, "PictureTopCrop");
    crop[1] = ghb_dict_get_int(settings, "PictureBottomCrop");
    crop[2] = ghb_dict_get_int(settings, "PictureLeftCrop");
    crop[3] = ghb_dict_get_int(settings, "PictureRightCrop");

    filter_str = g_strdup_printf("%d:%d:%d:%d:%d:%d",
                            width, height, crop[0], crop[1], crop[2], crop[3]);
    filter_dict = ghb_dict_new();
    ghb_dict_set_int(filter_dict, "ID", HB_FILTER_CROP_SCALE);
    ghb_dict_set_string(filter_dict, "Settings", filter_str);
    hb_value_array_append(filter_list, filter_dict);
    g_free(filter_str);

    ghb_value_free(&preset);
}

static gboolean
queue_add(signal_user_data_t *ud, GhbValue *settings, gint batch)
{
    // Add settings to the queue
    g_debug("queue_add ()");
    if (!validate_settings(ud, settings, batch))
    {
        return FALSE;
    }

    if (ud->queue == NULL)
        ud->queue = ghb_array_new();

    ghb_finalize_job(settings);

    // Copy current prefs into settings
    // The job should run with the preferences that existed
    // when the job was added to the queue.
    ghb_dict_set(settings, "Preferences", ghb_value_dup(ud->prefs));

    // Make a copy of current settings to be used for the new job
    ghb_dict_set_int(settings, "job_status", GHB_QUEUE_PENDING);
    ghb_dict_set_int(settings, "job_unique_id", 0);

    ghb_array_append(ud->queue, settings);
    add_to_queue_list(ud, settings, NULL);
    ghb_save_queue(ud->queue);
    ghb_update_pending(ud);
    ghb_queue_buttons_grey(ud);

    return TRUE;
}

G_MODULE_EXPORT void
queue_add_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    g_debug("queue_add_clicked_cb ()");
    GhbValue *settings = ghb_value_dup(ud->settings);
    if (!queue_add(ud, settings, 0))
        ghb_value_free(&settings);
    // Validation of settings may have changed audio list
    ghb_audio_list_refresh_all(ud);
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
            "<span foreground='red' weight='bold'>"
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

static void
add_multiple_titles(signal_user_data_t *ud)
{
    gint count, ii;

    count = ghb_array_len(ud->settings_array);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *settings;

        settings = ghb_value_dup(ghb_array_get(ud->settings_array, ii));
        if (ghb_dict_get_bool(settings, "title_selected"))
        {
            queue_add(ud, settings, ii);
        }
    }
}

static GtkListBoxRow*
title_get_row(GtkWidget *widget)
{
    while (widget != NULL && G_OBJECT_TYPE(widget) != GTK_TYPE_LIST_BOX_ROW)
    {
        widget = gtk_widget_get_parent(widget);
    }
    return GTK_LIST_BOX_ROW(widget);
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

    GtkListBoxRow * row = title_get_row(widget);
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
    GtkListBoxRow * row = title_get_row(widget);
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
    // Disable selection if it creates a confict!!!
    gboolean can_select;
    can_select = title_multiple_can_select(ud->settings_array, index);
    ghb_dict_set_bool(settings, "title_selected", can_select);
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
    GtkListBoxRow * row = title_get_row(widget);
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
    // Disable selection if it creates a confict!!!
    gboolean can_select;
    can_select = title_multiple_can_select(ud->settings_array, index);
    ghb_dict_set_bool(settings, "title_selected", can_select);
    title_add_multiple_set_sensitive(GTK_WIDGET(row), can_select);

    g_free(dest_dir);
    g_free(dest);

    title_add_multiple_set_conflict_label(ud,
        title_add_multiple_are_conflicts(ud));
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
    gtk_box_pack_start(hbox, GTK_WIDGET(selected), FALSE, FALSE, 0);

    // Title label
    title = GTK_LABEL(gtk_label_new(_("No Title")));
    gtk_label_set_width_chars(title, 12);
    gtk_widget_set_halign(GTK_WIDGET(title), GTK_ALIGN_START);
    gtk_widget_set_valign(GTK_WIDGET(title), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(title), "title_label");
    gtk_widget_show(GTK_WIDGET(title));
    gtk_box_pack_start(hbox, GTK_WIDGET(title), FALSE, FALSE, 0);

    default_title_attrs = gtk_label_get_attributes(title);
    gtk_widget_set_tooltip_text(GTK_WIDGET(title),
        _("There is another title with the same destination file name.\n"
        "This title will not be added to the queue unless you change\n"
        "the output file name.\n"));
    gtk_widget_set_has_tooltip(GTK_WIDGET(title), FALSE);

    // Destination entry and file chooser
    vbox_dest = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    //gtk_widget_set_hexpand(GTK_WIDGET(vbox_dest), TRUE);
    dest_file = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_width_chars(dest_file, 40);
    gtk_widget_set_name(GTK_WIDGET(dest_file), "title_file");
    //gtk_widget_set_hexpand(GTK_WIDGET(dest_file), TRUE);
    gtk_widget_show(GTK_WIDGET(dest_file));
    g_signal_connect(dest_file, "changed", (GCallback)title_dest_file_cb, ud);
    gtk_box_pack_start(vbox_dest, GTK_WIDGET(dest_file), FALSE, FALSE, 0);
    dest_dir = GTK_FILE_CHOOSER_BUTTON(
            gtk_file_chooser_button_new(_("Destination Directory"),
                                        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER));
    g_signal_connect(dest_dir, "selection-changed",
                    (GCallback)title_dest_dir_cb, ud);
    gtk_widget_set_name(GTK_WIDGET(dest_dir), "title_dir");
    gtk_widget_set_hexpand(GTK_WIDGET(dest_dir), TRUE);
    gtk_widget_show(GTK_WIDGET(dest_dir));
    gtk_box_pack_start(vbox_dest, GTK_WIDGET(dest_dir), FALSE, FALSE, 0);
    gtk_widget_show(GTK_WIDGET(vbox_dest));
    gtk_box_pack_start(hbox, GTK_WIDGET(vbox_dest), TRUE, TRUE, 0);

    return GTK_WIDGET(hbox);
}

G_MODULE_EXPORT void
queue_add_multiple_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    g_debug("queue_add_multiple_clicked_cb ()");

    GtkListBox *list;
    GtkWidget *row;
    gint count, ii;
    int max_title_len = 0;

    list = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "title_add_multiple_list"));

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
            gtk_entry_set_text(entry, dest_file);
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
    GtkWidget *dialog = GHB_WIDGET(ud->builder, "titla_add_multiple_dialog");
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    if (response == GTK_RESPONSE_OK)
    {
        add_multiple_titles(ud);
    }

    // Clear title list
    ghb_container_empty(GTK_CONTAINER(list));
}

static void
ghb_queue_remove_row_internal(signal_user_data_t *ud, int row)
{
    GtkTreeView *treeview;
    GtkTreeModel *store;
    GtkTreeIter iter;

    if (row < 0) return;
    if (row >= ghb_array_len(ud->queue))
        return;

    GhbValue *settings = ghb_array_get(ud->queue, row);
    int status = ghb_dict_get_int(settings, "job_status");
    if (status == GHB_QUEUE_RUNNING)
    {
        // Ask if wants to stop encode.
        if (!ghb_cancel_encode2(ud, NULL))
        {
            return;
        }
        int unique_id = ghb_dict_get_int(settings, "job_unique_id");
        ghb_remove_job(unique_id);
    }

    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
    store = gtk_tree_view_get_model(treeview);

    gchar *path = g_strdup_printf ("%d", row);
    if (gtk_tree_model_get_iter_from_string(store, &iter, path))
    {
        gtk_tree_store_remove(GTK_TREE_STORE(store), &iter);
    }
    g_free(path);

    ghb_array_remove(ud->queue, row);
}

void
ghb_queue_remove_row(signal_user_data_t *ud, int row)
{
    ghb_queue_remove_row_internal(ud, row);
    ghb_save_queue(ud->queue);
}

G_MODULE_EXPORT void
queue_delete_all_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    int ii, count;

    count = ghb_array_len(ud->queue);
    for (ii = count - 1; ii >= 0; ii--)
    {
        ghb_queue_remove_row_internal(ud, ii);
    }
    ghb_save_queue(ud->queue);
    ghb_update_pending(ud);
}

G_MODULE_EXPORT void
queue_remove_clicked_cb(GtkWidget *widget, gchar *path, signal_user_data_t *ud)
{
    GtkTreeView *treeview;
    GtkTreePath *treepath;
    GtkTreeModel *store;
    GtkTreeIter iter;
    gint row;
    gint *indices;
    gint unique_id;
    GhbValue *settings;
    gint status;

    g_debug("queue_remove_clicked_cb ()");
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
    store = gtk_tree_view_get_model(treeview);
    treepath = gtk_tree_path_new_from_string (path);
    if (gtk_tree_path_get_depth(treepath) > 1) return;
    if (gtk_tree_model_get_iter(store, &iter, treepath))
    {
        // Find the entry in the queue
        indices = gtk_tree_path_get_indices (treepath);
        row = indices[0];
        // Can only free the treepath After getting what I need from
        // indices since this points into treepath somewhere.
        gtk_tree_path_free (treepath);
        if (row < 0) return;
        if (row >= ghb_array_len(ud->queue))
            return;
        settings = ghb_array_get(ud->queue, row);
        status = ghb_dict_get_int(settings, "job_status");
        if (status == GHB_QUEUE_RUNNING)
        {
            // Ask if wants to stop encode.
            if (!ghb_cancel_encode2(ud, NULL))
            {
                return;
            }
            unique_id = ghb_dict_get_int(settings, "job_unique_id");
            ghb_remove_job(unique_id);
        }
        // Remove the selected item
        gtk_tree_store_remove(GTK_TREE_STORE(store), &iter);
        // Remove the corresponding item from the queue list
        ghb_array_remove(ud->queue, row);
        ghb_save_queue(ud->queue);
    }
    else
    {
        gtk_tree_path_free (treepath);
    }
    ghb_update_pending(ud);
}

static gint
find_last_finished(GhbValue *queue)
{
    GhbValue *js;
    gint ii, count;
    gint status;

    g_debug("find_last_finished");
    count = ghb_array_len(queue);
    for (ii = 0; ii < count; ii++)
    {
        js = ghb_array_get(queue, ii);
        status = ghb_dict_get_int(js, "job_status");
        if (status != GHB_QUEUE_DONE && status != GHB_QUEUE_RUNNING)
        {
            return ii-1;
        }
    }
    return -1;
}

// This little bit is needed to prevent the default drag motion
// handler from expanding rows if you hover over them while
// dragging.
// Also controls where valid drop locations are
G_MODULE_EXPORT gboolean
queue_drag_motion_cb(
    GtkTreeView *tv,
    GdkDragContext *ctx,
    gint x,
    gint y,
    guint time,
    signal_user_data_t *ud)
{
    GtkTreePath *path = NULL;
    GtkTreeViewDropPosition pos;
    gint *indices, row, status, finished;
    GhbValue *js;
    GtkTreeIter iter;
    GtkTreeView *srctv;
    GtkTreeModel *model;
    GtkTreeSelection *select;
    GtkWidget *widget;
    int height;

    height = gtk_widget_get_allocated_height(GTK_WIDGET(tv));
    if (y <= 6 || y >= height - 6)
        return FALSE;

    widget = gtk_drag_get_source_widget(ctx);
    if (widget == NULL || widget != GTK_WIDGET(tv))
        return TRUE;

    // This bit checks to see if the source is allowed to be
    // moved.  Only pending and canceled items may be moved.
    srctv = GTK_TREE_VIEW(gtk_drag_get_source_widget(ctx));
    select = gtk_tree_view_get_selection (srctv);
    gtk_tree_selection_get_selected (select, &model, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    indices = gtk_tree_path_get_indices(path);
    row = indices[0];
    gtk_tree_path_free(path);
    js = ghb_array_get(ud->queue, row);
    status = ghb_dict_get_int(js, "job_status");
    if (status != GHB_QUEUE_PENDING && status != GHB_QUEUE_CANCELED)
    {
        gdk_drag_status(ctx, 0, time);
        return TRUE;
    }

    // The reset checks that the destination is a valid position
    // in the list.  Can not move above any finished or running items
    gtk_tree_view_get_dest_row_at_pos (tv, x, y, &path, &pos);
    if (path == NULL)
    {
        gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
        return TRUE;
    }
    // Don't allow *drop into*
    if (pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
        pos = GTK_TREE_VIEW_DROP_BEFORE;
    if (pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
        pos = GTK_TREE_VIEW_DROP_AFTER;
    // Don't allow droping int child items
    if (gtk_tree_path_get_depth(path) > 1)
    {
        gtk_tree_path_up(path);
        pos = GTK_TREE_VIEW_DROP_AFTER;
    }
    indices = gtk_tree_path_get_indices(path);
    row = indices[0];
    js = ghb_array_get(ud->queue, row);

    finished = find_last_finished(ud->queue);
    if (row < finished)
    {
        gtk_tree_path_free(path);
        gdk_drag_status(ctx, 0, time);
        return TRUE;
    }
    if (pos != GTK_TREE_VIEW_DROP_AFTER &&
        row == finished)
    {
        gtk_tree_path_free(path);
        gdk_drag_status(ctx, 0, time);
        return TRUE;
    }
    gtk_tree_view_set_drag_dest_row(tv, path, pos);
    gtk_tree_path_free(path);
    gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
    return TRUE;
}

G_MODULE_EXPORT void
queue_drag_cb(
    GtkTreeView *dstwidget,
    GdkDragContext *dc,
    gint x, gint y,
    GtkSelectionData *selection_data,
    guint info, guint t,
    signal_user_data_t *ud)
{
    GtkTreePath *path = NULL;
    //GtkTreeModel *model;
    GtkTreeViewDropPosition pos;
    GtkTreeIter dstiter, srciter;
    gint *indices, row;
    GhbValue *js;

    GtkTreeModel *dstmodel = gtk_tree_view_get_model(dstwidget);

    g_debug("queue_drag_cb ()");
    // This doesn't work here for some reason...
    // gtk_tree_view_get_drag_dest_row(dstwidget, &path, &pos);
    gtk_tree_view_get_dest_row_at_pos (dstwidget, x, y, &path, &pos);
    // This little hack is needed because attempting to drop after
    // the last item gives us no path or pos.
    if (path == NULL)
    {
        gint n_children;

        n_children = gtk_tree_model_iter_n_children(dstmodel, NULL);
        if (n_children)
        {
            pos = GTK_TREE_VIEW_DROP_AFTER;
            path = gtk_tree_path_new_from_indices(n_children-1, -1);
        }
        else
        {
            pos = GTK_TREE_VIEW_DROP_BEFORE;
            path = gtk_tree_path_new_from_indices(0, -1);
        }
    }
    if (path)
    {
        if (gtk_tree_path_get_depth(path) > 1)
            gtk_tree_path_up(path);
        if (gtk_tree_model_get_iter (dstmodel, &dstiter, path))
        {
            GtkTreeIter iter;
            GtkTreeView *srcwidget;
            GtkTreeModel *srcmodel;
            GtkTreeSelection *select;
            GtkTreePath *srcpath = NULL;
            GtkTreePath *dstpath = NULL;

            srcwidget = GTK_TREE_VIEW(gtk_drag_get_source_widget(dc));
            //srcmodel = gtk_tree_view_get_model(srcwidget);
            select = gtk_tree_view_get_selection (srcwidget);
            gtk_tree_selection_get_selected (select, &srcmodel, &srciter);

            srcpath = gtk_tree_model_get_path (srcmodel, &srciter);
            indices = gtk_tree_path_get_indices(srcpath);
            row = indices[0];
            gtk_tree_path_free(srcpath);
            js = ghb_array_get(ud->queue, row);

            switch (pos)
            {
                case GTK_TREE_VIEW_DROP_BEFORE:
                case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
                    gtk_tree_store_insert_before (GTK_TREE_STORE (dstmodel),
                                                    &iter, NULL, &dstiter);
                    break;

                case GTK_TREE_VIEW_DROP_AFTER:
                case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
                    gtk_tree_store_insert_after (GTK_TREE_STORE (dstmodel),
                                                    &iter, NULL, &dstiter);
                    break;

                default:
                    break;
            }
            // Reset job to pending
            ghb_dict_set_int(js, "job_status", GHB_QUEUE_PENDING);
            add_to_queue_list(ud, js, &iter);

            dstpath = gtk_tree_model_get_path (dstmodel, &iter);
            indices = gtk_tree_path_get_indices(dstpath);
            row = indices[0];
            gtk_tree_path_free(dstpath);
            ghb_value_incref(js);
            ghb_array_insert(ud->queue, row, js);

            srcpath = gtk_tree_model_get_path (srcmodel, &srciter);
            indices = gtk_tree_path_get_indices(srcpath);
            row = indices[0];
            gtk_tree_path_free(srcpath);
            ghb_array_remove(ud->queue, row);
            gtk_tree_store_remove (GTK_TREE_STORE (srcmodel), &srciter);
            ghb_save_queue(ud->queue);
        }
        gtk_tree_path_free(path);
    }
}

void
ghb_queue_buttons_grey(signal_user_data_t *ud)
{
    GtkWidget *widget;
    gint queue_count;
    gint title_id, titleindex;
    const hb_title_t *title;
    gint queue_state, scan_state;
    gboolean show_start, show_stop, paused;

    queue_count = ghb_array_len(ud->queue);
    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);

    queue_state = ghb_get_queue_state();
    scan_state = ghb_get_scan_state();

    show_stop = queue_state &
                (GHB_STATE_WORKING | GHB_STATE_SEARCHING |
                 GHB_STATE_SCANNING | GHB_STATE_MUXING | GHB_STATE_PAUSED);
    show_start = !(scan_state & GHB_STATE_SCANNING) &&
                    (title !=NULL || queue_count > 0);


    paused = queue_state & GHB_STATE_PAUSED;

    widget = GHB_WIDGET(ud->builder, "queue_save");
    gtk_widget_set_sensitive(widget, !!queue_count);
    widget = GHB_WIDGET(ud->builder, "queue_add");
    gtk_widget_set_sensitive(widget, show_start);
    widget = GHB_WIDGET(ud->builder, "queue_add_menu");
    gtk_widget_set_sensitive(widget, show_start);
    widget = GHB_WIDGET(ud->builder, "queue_add_multiple_menu");
    gtk_widget_set_sensitive(widget, show_start);

    widget = GHB_WIDGET (ud->builder, "queue_start1");
    if (show_stop)
    {
        gtk_widget_set_sensitive (widget, TRUE);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-stop");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Stop\nEncoding"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Stop Encoding"));
    }
    else
    {
        gtk_widget_set_sensitive (widget, show_start);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-start");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Start\nEncoding"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Start Encoding"));
    }
    widget = GHB_WIDGET (ud->builder, "queue_pause1");
    if (paused)
    {
        gtk_widget_set_sensitive (widget, show_stop);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-start");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Resume\nEncoding"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Resume Encoding"));
    }
    else
    {
        gtk_widget_set_sensitive (widget, show_stop);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-pause");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Pause\nEncoding"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Pause Encoding"));
    }

    widget = GHB_WIDGET (ud->builder, "queue_start_menu");
    if (show_stop)
    {
        gtk_widget_set_sensitive (widget, TRUE);
        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), _("S_top Encoding"));
        gtk_widget_set_tooltip_text(widget, _("Stop Encoding"));
    }
    else
    {
        gtk_widget_set_sensitive (widget, show_start);
        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), _("_Start Encoding"));
        gtk_widget_set_tooltip_text(widget, _("Start Encoding"));
    }
    widget = GHB_WIDGET (ud->builder, "queue_pause_menu");
    if (paused)
    {
        gtk_widget_set_sensitive (widget, show_start);
        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), _("_Resume Encoding"));
        gtk_widget_set_tooltip_text(widget, _("Resume Encoding"));
    }
    else
    {
        gtk_widget_set_sensitive (widget, show_stop);
        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), _("_Pause Encoding"));
        gtk_widget_set_tooltip_text(widget, _("Pause Encoding"));
    }
}

G_MODULE_EXPORT void
queue_list_size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation, GtkCellRenderer *cell)
{
    GtkTreeViewColumn *column;
    gint width;

    column = gtk_tree_view_get_column (GTK_TREE_VIEW(widget), 0);
    width = gtk_tree_view_column_get_width(column);
    g_debug("col width %d alloc width %d", width, allocation->width);
    // Set new wrap-width.  Shave a little off to accomidate the icons
    // that share this column.
    if (width >= 564) // Don't allow below a certain size
        g_object_set(cell, "wrap-width", width-70, NULL);
}

G_MODULE_EXPORT void
queue_start_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    GhbValue *js;
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
        js = ghb_array_get(ud->queue, ii);
        status = ghb_dict_get_int(js, "job_status");
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
        GhbValue *settings = ghb_value_dup(ud->settings);
        if (!queue_add(ud, settings, 0))
        {
            ghb_value_free(&settings);
            return;
        }
        // Validation of settings may have changed audio list
        ghb_audio_list_refresh_all(ud);
    }
    if (state == GHB_STATE_IDLE)
    {
        // Add the first pending queue item and start
        ud->current_job = ghb_start_next_job(ud);
    }
}

G_MODULE_EXPORT void
queue_pause_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    ghb_pause_queue();
}

gboolean
ghb_reload_queue(signal_user_data_t *ud)
{
    GhbValue *queue;
    gint unfinished = 0;
    gint count, ii;
    gint pid;
    gint status;
    GhbValue *settings;

    g_debug("ghb_reload_queue");

find_pid:
    pid = ghb_find_pid_file();
    if (pid < 0)
        return FALSE;

    queue = ghb_load_old_queue(pid);
    ghb_remove_old_queue_file(pid);
    // Look for unfinished entries
    count = ghb_array_len(queue);
    for (ii = 0; ii < count; ii++)
    {
        settings = ghb_array_get(queue, ii);
        status = ghb_dict_get_int(settings, "job_status");
        if (status != GHB_QUEUE_DONE && status != GHB_QUEUE_CANCELED)
        {
            unfinished++;
        }
    }
    if (!unfinished)
    {
        ghb_value_free(&queue);
        goto find_pid;
    }
    else
    {
        GtkWidget *widget = GHB_WIDGET(ud->builder, "show_queue");
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), TRUE);
        ud->queue = queue;
        // First get rid of any old items we don't want
        for (ii = count-1; ii >= 0; ii--)
        {
            settings = ghb_array_get(queue, ii);
            status = ghb_dict_get_int(settings, "job_status");
            if (status == GHB_QUEUE_DONE || status == GHB_QUEUE_CANCELED)
            {
                ghb_array_remove(queue, ii);
            }
        }
        count = ghb_array_len(queue);
        for (ii = 0; ii < count; ii++)
        {
            settings = ghb_array_get(queue, ii);
            ghb_dict_set_int(settings, "job_unique_id", 0);
            ghb_dict_set_int(settings, "job_status", GHB_QUEUE_PENDING);
            add_to_queue_list(ud, settings, NULL);
        }
        ghb_queue_buttons_grey(ud);
        ghb_save_queue(ud->queue);
        ghb_update_pending(ud);
    }
    return FALSE;
}

G_MODULE_EXPORT gboolean
queue_key_press_cb(
    GtkWidget *widget,
    GdkEventKey *event,
    signal_user_data_t *ud)
{
    GtkTreeView *treeview;
    GtkTreeSelection *selection;
    GtkTreeModel *store;
    GtkTreeIter iter;
    gint row;
    gint *indices;
    gint unique_id;
    GhbValue *settings;
    gint status;

    g_debug("queue_key_press_cb ()");
    if (event->keyval != GDK_KEY_Delete)
        return FALSE;
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
    store = gtk_tree_view_get_model(treeview);

    selection = gtk_tree_view_get_selection (treeview);
    if (gtk_tree_selection_get_selected(selection, &store, &iter))
    {
        GtkTreePath *treepath;

        treepath = gtk_tree_model_get_path (store, &iter);
        // Find the entry in the queue
        indices = gtk_tree_path_get_indices (treepath);
        row = indices[0];
        // Can only free the treepath After getting what I need from
        // indices since this points into treepath somewhere.
        gtk_tree_path_free (treepath);
        if (row < 0) return FALSE;
        if (row >= ghb_array_len(ud->queue))
            return FALSE;
        settings = ghb_array_get(ud->queue, row);
        status = ghb_dict_get_int(settings, "job_status");
        if (status == GHB_QUEUE_RUNNING)
        {
            // Ask if wants to stop encode.
            if (!ghb_cancel_encode2(ud, NULL))
            {
                return TRUE;
            }
            unique_id = ghb_dict_get_int(settings, "job_unique_id");
            ghb_remove_job(unique_id);
        }
        // Remove the selected item
        gtk_tree_store_remove(GTK_TREE_STORE(store), &iter);
        // Remove the corresponding item from the queue list
        ghb_array_remove(ud->queue, row);
        ghb_save_queue(ud->queue);
        return TRUE;
    }
    return FALSE;
}

GhbValue *ghb_queue_edit_settings = NULL;

G_MODULE_EXPORT void
queue_edit_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    GtkTreeView *treeview;
    GtkTreeSelection *selection;
    GtkTreeModel *store;
    GtkTreeIter iter;
    gint row;
    gint *indices;
    gint status;

    g_debug("queue_key_press_cb ()");
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "queue_list"));
    store = gtk_tree_view_get_model(treeview);

    selection = gtk_tree_view_get_selection (treeview);
    if (gtk_tree_selection_get_selected(selection, &store, &iter))
    {
        GtkTreePath *treepath;

        treepath = gtk_tree_model_get_path (store, &iter);
        // Find the entry in the queue
        indices = gtk_tree_path_get_indices (treepath);
        row = indices[0];
        // Can only free the treepath After getting what I need from
        // indices since this points into treepath somewhere.
        gtk_tree_path_free (treepath);
        if (row < 0) return;
        if (row >= ghb_array_len(ud->queue))
            return;
        ghb_queue_edit_settings = ghb_array_get(ud->queue, row);
        status = ghb_dict_get_int(ghb_queue_edit_settings, "job_status");
        if (status == GHB_QUEUE_PENDING)
        {
            // Remove the selected item
            gtk_tree_store_remove(GTK_TREE_STORE(store), &iter);
            // Remove the corresponding item from the queue list
            ghb_value_incref(ghb_queue_edit_settings);
            ghb_array_remove(ud->queue, row);
            ghb_update_pending(ud);
        }
        else
        {
            ghb_queue_edit_settings = ghb_value_dup(ghb_queue_edit_settings);
        }
        const gchar *source;
        source = ghb_dict_get_string(ghb_queue_edit_settings, "source");
        ghb_do_scan(ud, source, 0, FALSE);

        GtkWidget *widget = GHB_WIDGET(ud->builder, "show_queue");
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), FALSE);
    }
}

