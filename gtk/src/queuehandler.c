/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.c
 * Copyright (C) John Stebbins 2008-2013 <stebbins@stebbins>
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
#include "ghb-dvd.h"

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
    }
}

static void
add_to_queue_list(signal_user_data_t *ud, GValue *settings, GtkTreeIter *piter)
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
    title = ghb_settings_get_int(settings, "title");
    start_point = ghb_settings_get_int(settings, "start_point");
    end_point = ghb_settings_get_int(settings, "end_point");
    vol_name = ghb_settings_get_const_string(settings, "volume_label");
    dest = ghb_settings_get_const_string(settings, "destination");
    basename = g_path_get_basename(dest);
    escape = g_markup_escape_text(basename, -1);
    escape2 = g_markup_escape_text(vol_name, -1);

    vqtype = ghb_settings_get_boolean(settings, "vquality_type_constant");
    two_pass = ghb_settings_get_boolean(settings, "VideoTwoPass");

    const gchar *points = _("Chapters");
    if (ghb_settings_combo_int(settings, "PtoPType") == 0)
        points = _("Chapters");
    else if (ghb_settings_combo_int(settings, "PtoPType") == 1)
        points = _("Seconds");
    else if (ghb_settings_combo_int(settings, "PtoPType") == 2)
        points = _("Frames");

    XPRINT("<big><b>%s</b></big> "
            "<small>(Title %d, %s %d through %d, %d Video %s)"
            " --> %s</small>",
            escape2, title, points, start_point, end_point,
            two_pass ? 2:1, two_pass ? "Passes":"Pass", escape
    );
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
    status = ghb_settings_get_int(settings, "job_status");
    const char *status_icon = "hb-queue-job";
    switch (status)
    {
        case GHB_QUEUE_PENDING:
            status_icon = "hb-queue-job";
            break;
        case GHB_QUEUE_CANCELED:
            status_icon = "hb-canceled";
            break;
        case GHB_QUEUE_RUNNING:
            status_icon = "hb-working0";
            break;
        case GHB_QUEUE_DONE:
            status_icon = "hb-complete";
            break;
        default:
            status_icon = "hb-queue-job";
            break;
    }
    // Set the status icon, job description, and delete icon button
    gtk_tree_store_set(store, &iter, 0, status_icon, 1, str->str,
                       2, "hb-queue-delete", -1);

    // Reset the string for the next line
    g_string_assign(str, "");

    // Next line in the display
    // Preset: PresetName
    gchar *preset;
    gboolean markers;
    gboolean preset_modified;
    const GValue *path;

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_settings_get_const_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    preset_modified = ghb_settings_get_boolean(settings, "preset_modified");
    path = ghb_settings_get_value(settings, "preset");
    preset = ghb_preset_path_string(path);
    markers = ghb_settings_get_boolean(settings, "ChapterMarkers");

    if (preset_modified)
    {
        XPRINT("<b>Modified Preset Based On:</b> <small>%s</small>\n", preset);
    }
    else
    {
        XPRINT("<b>Preset:</b> <small>%s</small>\n", preset);
    }
    g_free(preset);

    // Next line in the display (Container type)
    // Format: XXX Container
    XPRINT("<b>Format:</b> <small>%s Container</small>\n", mux->name);

    // Next line in the display (Container options)
    // Container Options: - Chapter Markers
    gboolean ipod = FALSE, http = FALSE, large = FALSE;
    if (mux->format & HB_MUX_MASK_MP4)
    {
        ipod = ghb_settings_get_boolean(settings, "Mp4iPodCompatible");
        http = ghb_settings_get_boolean(settings, "Mp4HttpOptimize");
        large = ghb_settings_get_boolean(settings, "Mp4LargeFile");
    }
    if (http || ipod || large || markers)
    {
        const char *prefix = " ";
        XPRINT("<b>Container Options:</b><small>");
        if (markers)
        {
            XPRINT("%sChapter Markers", prefix);
            prefix = " - ";
        }
        if (ipod)
        {
            XPRINT("%siPod 5G Support", prefix);
            prefix = " - ";
        }
        if (http)
        {
            XPRINT("%sWeb Optimized", prefix);
            prefix = " - ";
        }
        if (large)
        {
            XPRINT("%sLarge File Size (>4GB)", prefix);
            prefix = " - ";
        }
        XPRINT("</small>\n");
    }

    // Next line in the display (Destination)
    // Destination: /Full/Destination/Path.mkv
    escape = g_markup_escape_text(dest, -1);
    XPRINT("<b>Destination:</b> <small>%s</small>\n", escape);
    g_free(escape);

    // Next line in the display (Picture settings)
    // Picture: Source: W x H, Output W x H (Animorphic), Display W x H
    int width, height, pic_par;
    gboolean keep_aspect;

    width = ghb_settings_get_int(settings, "scale_width");
    height = ghb_settings_get_int(settings, "scale_height");
    pic_par = ghb_settings_get_int(settings, "PicturePAR");
    keep_aspect = ghb_settings_get_boolean(settings, "PictureKeepRatio");

    gchar *aspect_desc;
    switch (pic_par)
    {
    case 0:
    {
        if (keep_aspect)
        {
            aspect_desc = "(Aspect Preserved)";
        }
        else
        {
            aspect_desc = "(Aspect Lost)";
        }
    } break;

    case 1:
    {
        aspect_desc = "(Anamorphic)";
    } break;

    case 2:
    {
        aspect_desc = "(Anamorphic)";
    } break;

    case 3:
    {
        aspect_desc = "(Custom Anamorphic)";
    } break;

    default:
    {
        aspect_desc = "";
    } break;
    }

    gint source_width, source_height;
    source_width = ghb_settings_get_int(settings, "source_width");
    source_height = ghb_settings_get_int(settings, "source_height");
    XPRINT(
        "<b>Picture:</b> Source: <small>%d x %d, Output %d x %d %s</small>",
         source_width, source_height, width, height, aspect_desc);
    if (pic_par)
    {
        int display_width, display_height;
        display_width = ghb_settings_get_int(settings, "PictureDisplayWidth");
        display_height = ghb_settings_get_int(settings, "PictureDisplayHeight");
        XPRINT("<small>, Display %d x %d</small>",
                display_width, display_height);
    }
    XPRINT("\n");

    // Next line in the display (Filter settings)
    // Filters: - Deinterlace
    gint decomb, detel, deint, deblock, denoise;
    const gchar *detel_cust, *deint_cust, *decomb_cust, *denoise_cust;
    const gchar *deint_opt, *decomb_opt, *denoise_opt;
    gboolean decomb_deint;
    gboolean grayscale;
    gboolean filters;

    decomb_deint = ghb_settings_get_boolean(settings, "PictureDecombDeinterlace");
    decomb = ghb_settings_combo_int(settings, "PictureDecomb");
    decomb_opt = ghb_settings_combo_option(settings, "PictureDecomb");
    decomb_cust = ghb_settings_get_const_string(settings,
                                                "PictureDecombCustom");
    deint = ghb_settings_combo_int(settings, "PictureDeinterlace");
    deint_opt = ghb_settings_combo_option(settings, "PictureDeinterlace");
    deint_cust = ghb_settings_get_const_string(settings,
                                               "PictureDeinterlaceCustom");
    detel = ghb_settings_combo_int(settings, "PictureDetelecine");
    detel_cust = ghb_settings_get_const_string(settings,
                                               "PictureDetelecineCustom");
    deblock = ghb_settings_get_int(settings, "PictureDeblock");
    denoise = ghb_settings_combo_int(settings, "PictureDenoise");
    denoise_opt = ghb_settings_combo_option(settings, "PictureDenoise");
    denoise_cust = ghb_settings_get_const_string(settings,
                                                 "PictureDenoiseCustom");
    grayscale = ghb_settings_get_boolean(settings, "VideoGrayScale");

    filters = detel || (decomb_deint && decomb) || (!decomb_deint && deint) ||
              denoise || (deblock >= 5) || grayscale;
    if (filters)
    {
        const char *prefix = " ";
        XPRINT("<b>Filters:</b><small>");
        if (detel)
        {
            XPRINT("%sDetelecine", prefix);
            if (detel == 1)
            {
                XPRINT(": %s", detel_cust);
            }
            prefix = " - ";
        }
        if (decomb_deint && decomb)
        {
            XPRINT("%sDecomb", prefix);
            if (decomb == 1)
            {
                XPRINT(": %s", decomb_cust);
            }
            else
            {
                XPRINT(": %s", decomb_opt);
            }
            prefix = " - ";
        }
        else if (!decomb_deint && deint)
        {
            XPRINT("%sDeinterlace", prefix);
            if (deint == 1)
            {
                XPRINT(": %s", deint_cust);
            }
            else
            {
                XPRINT(": %s", deint_opt);
            }
            prefix = " - ";
        }
        if (denoise)
        {
            XPRINT("%sDenoise", prefix);
            if (denoise == 1)
            {
                XPRINT(": %s", denoise_cust);
            }
            else
            {
                XPRINT(": %s", denoise_opt);
            }
            prefix = " - ";
        }
        if (deblock >= 5)
        {
            XPRINT("%sDeblock: %d", prefix, deblock);
            prefix = " - ";
        }
        if (grayscale)
        {
            XPRINT("%sGrayscale", prefix);
            prefix = " - ";
        }
        XPRINT("</small>\n");
    }

    // Next line in the display (Video Encoder)
    // Video: Encoder, Framerate: fps, RF/Bitrate/QP
    const hb_encoder_t *video_encoder;
    video_encoder = ghb_settings_video_encoder(settings, "VideoEncoder");

    XPRINT("<b>Video:</b> <small>%s", video_encoder->name);

    const hb_rate_t *fps;
    fps = ghb_settings_video_framerate(settings, "VideoFramerate");
    if (fps->rate == 0)
    {
        const char *rate_mode;
        if (ghb_settings_get_boolean(settings, "VideoFramerateCFR"))
            rate_mode = "(constant)";
        else
            rate_mode = "(variable)";
        XPRINT(", Framerate: %s %s", fps->name, rate_mode);
    }
    else
    {
        if (ghb_settings_get_boolean(settings, "VideoFrameratePFR"))
        {
            XPRINT(", Framerate: Peak %s (may be lower)", fps->name);
        }
        else
        {
            XPRINT(", Framerate: %s (constant frame rate)", fps->name);
        }
    }
    const gchar *vq_desc = "Error";
    const gchar *vq_units = "";
    gdouble vqvalue;
    if (!vqtype)
    {
        // Has to be bitrate
        vqvalue = ghb_settings_get_int(settings, "VideoAvgBitrate");
        vq_desc = "Bitrate:";
        vq_units = "kbps";
        XPRINT(", %s %d%s",
               vq_desc, (int)vqvalue, vq_units);
    }
    else
    {
        // Constant quality
        vqvalue = ghb_settings_get_double(settings, "VideoQualitySlider");
        vq_desc = "Constant Quality:";
        vq_units = hb_video_quality_get_name(video_encoder->codec);
        XPRINT(", %s %.4g(%s)",
               vq_desc, vqvalue, vq_units);
    }
    XPRINT("</small>\n");

    // Next line in the display (Turbo setting)
    gboolean turbo;
    turbo = ghb_settings_get_boolean(settings, "VideoTurboTwoPass");
    if (!vqtype && two_pass && turbo)
    {
        XPRINT("<b>Turbo 1st Pass:</b> <small>On</small>\n");
    }

    // Next line in the display (Video Encoder Options)
    // Video Options: Preset - Tune - Profile - Level
    if (video_encoder->codec == HB_VCODEC_X264 &&
        !ghb_settings_get_boolean(settings, "x264UseAdvancedOptions"))
    {
        const gchar *preset_opt, *tune_opt;
        const gchar *profile_opt, *level_opt, *extra_opt;
        gboolean fastdecode, zerolatency;

        preset_opt = ghb_settings_get_const_string(settings, "x264Preset");
        tune_opt = ghb_settings_get_const_string(settings, "x264Tune");
        fastdecode = ghb_settings_get_boolean(settings, "x264FastDecode");
        zerolatency = ghb_settings_get_boolean(settings, "x264ZeroLatency");
        profile_opt = ghb_settings_get_const_string(settings, "h264Profile");
        level_opt = ghb_settings_get_const_string(settings, "h264Level");
        extra_opt = ghb_settings_get_const_string(settings, "x264OptionExtra");

        XPRINT("<b>Video Options:</b> <small>Preset: %s</small>", preset_opt);
        if ((tune_opt != NULL && tune_opt[0] != 0) || zerolatency || fastdecode)
        {
            const char *prefix = "";
            XPRINT("<small> - Tune: ");
            if (tune_opt != NULL && tune_opt[0] != 0)
            {
                XPRINT("%s%s", prefix, tune_opt);
                prefix = ",";
            }
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
            XPRINT("</small>");
        }
        if (profile_opt != NULL && profile_opt[0] != 0)
        {
            XPRINT("<small> - Profile: %s</small>", profile_opt);
        }
        if (level_opt != NULL && level_opt[0] != 0)
        {
            XPRINT("<small> - Level: %s</small>", level_opt);
        }
        XPRINT("\n");

        // Next line in the display (Video Encoder Options)
        // Video Advanced Options: detailed settings
        if (extra_opt != NULL && extra_opt[0] != 0)
        {
            XPRINT("<b>Advanced Options:</b> <small>%s</small>\n", extra_opt);
        }
    }
    else if (video_encoder->codec == HB_VCODEC_X264 ||
             (video_encoder->codec | HB_VCODEC_FFMPEG_MASK))
    {
        // Next line in the display (Video Encoder Options)
        // Video Advanced Options: detailed settings
        gchar *opts = ghb_build_advanced_opts_string(settings);
        if (opts != NULL && opts[0] != 0)
        {
            XPRINT("<b>Advanced Options:</b> <small>%s</small>\n", opts);
        }
        g_free(opts);
    }

    // Next line in the display (Audio)
    // Audio Tracks: count
    //      Source description, Encoder, Mix, Samplerate, Bitrate
    //      ...
    gint count, ii;
    const GValue *audio_list;

    audio_list = ghb_settings_get_value(settings, "audio_list");
    count = ghb_array_len(audio_list);
    if (count == 1)
    {
        XPRINT("<b>Audio:</b> ");
    }
    else if (count > 1)
    {
        XPRINT("<b>Audio Tracks: %d</b>\n", count);
    }
    for (ii = 0; ii < count; ii++)
    {
        gchar *quality = NULL, *samplerate, *track;
        GValue *asettings;
        gdouble sr;
        const hb_encoder_t *audio_encoder;

        asettings = ghb_array_get_nth(audio_list, ii);

        audio_encoder = ghb_settings_audio_encoder(asettings, "AudioEncoder");
        double q = ghb_settings_get_double(asettings, "AudioTrackQuality");
        if (ghb_settings_get_boolean(asettings, "AudioTrackQualityEnable") &&
            q != HB_INVALID_AUDIO_QUALITY)
        {
            quality = ghb_format_quality("Quality: ", audio_encoder->codec, q);
        }
        else
        {
            char *br;
            br = ghb_settings_get_string(asettings, "AudioBitrate");
            quality = g_strdup_printf("Bitrate: %s", br);
            g_free(br);
        }
        sr = ghb_settings_get_double(asettings, "AudioSamplerate");
        if ((int)sr == 0)
        {
            samplerate = g_strdup("Same As Source");
        }
        else
        {
            samplerate = g_strdup_printf("%.4g", sr);
        }
        track = ghb_settings_get_string(asettings, "AudioTrackDescription");
        const hb_mixdown_t *mix;
        mix = ghb_settings_mixdown(asettings, "AudioMixdown");
        if (count > 1)
            XPRINT("\t");

        if (audio_encoder->codec & HB_ACODEC_PASS_FLAG)
        {
            XPRINT("<small>%s, Encoder: %s</small>\n",
                   track, audio_encoder->name);
        }
        else
        {
            XPRINT(
            "<small>%s, Encoder: %s, Mixdown: %s, SampleRate: %s, %s</small>\n",
             track, audio_encoder->name, mix->name, samplerate, quality);
        }
        g_free(track);
        g_free(quality);
        g_free(samplerate);
    }

    // Next line in the display (Subtitle)
    // Subtitle Tracks: count
    //      Subtitle description(Subtitle options)
    //      ...
    const GValue *sub_list;

    sub_list = ghb_settings_get_value(settings, "subtitle_list");
    count = ghb_array_len(sub_list);
    if (count == 1)
    {
        XPRINT("<b>Subtitle:</b> ");
    }
    else if (count > 1)
    {
        XPRINT("<b>Subtitle Tracks: %d</b>\n", count);
    }
    for (ii = 0; ii < count; ii++)
    {
        GValue *settings;
        gchar *track;
        gboolean force, burn, def;
        gint source;

        settings = ghb_array_get_nth(sub_list, ii);
        track = ghb_settings_get_string(settings, "SubtitleTrackDescription");
        source = ghb_settings_get_int(settings, "SubtitleSource");
        force = ghb_settings_get_boolean(settings, "SubtitleForced");
        burn = ghb_settings_get_boolean(settings, "SubtitleBurned");
        def = ghb_settings_get_boolean(settings, "SubtitleDefaultTrack");
        if (count > 1)
            XPRINT("\t");

        if (source != SRTSUB)
        {
            XPRINT("<small>%s%s%s%s</small>\n", track,
                    force ? " (Force)":"",
                    burn  ? " (Burn)":"",
                    def   ? " (Default)":""
            );
        }
        else
        {
            gint offset;
            gchar *filename, *basename, *code;

            offset = ghb_settings_get_int(settings, "SrtOffset");
            filename = ghb_settings_get_string(settings, "SrtFile");
            basename = g_path_get_basename(filename);
            code = ghb_settings_get_string(settings, "SrtCodeset");

            XPRINT("<small> %s (%s), %s, Offset (ms) %d%s</small>\n",
                track, code, basename, offset, def   ? " (Default)":"");

            g_free(filename);
            g_free(basename);
            g_free(code);
        }
        g_free(track);
    }

    // Remove the final newline in the string
    if (str->len > 0 && str->str[str->len-1] == '\n')
        str->str[str->len-1] = 0;

    gtk_tree_store_append(store, &citer, &iter);
    gtk_tree_store_set(store, &citer, 1, str->str, -1);

    g_string_free(str, TRUE);
}

static gboolean
validate_settings(signal_user_data_t *ud, GValue *settings, gint batch)
{
    // Check to see if the dest file exists or is
    // already in the queue
    gchar *message, *dest;
    gint count, ii;
    gint title_id, titleindex;
    const hb_title_t *title;

    title_id = ghb_settings_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL) return FALSE;
    dest = ghb_settings_get_string(settings, "destination");
    count = ghb_array_len(ud->queue);
    for (ii = 0; ii < count; ii++)
    {
        GValue *js;
        gchar *filename;

        js = ghb_array_get_nth(ud->queue, ii);
        filename = ghb_settings_get_string(js, "destination");
        if (strcmp(dest, filename) == 0)
        {
            message = g_strdup_printf(
                        _("Destination: %s\n\n"
                        "Another queued job has specified the same destination.\n"
                        "Do you want to overwrite?"),
                        dest);
            if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message, _("Cancel"), _("Overwrite")))
            {
                g_free(filename);
                g_free(dest);
                g_free(message);
                return FALSE;
            }
            g_free(message);
            break;
        }
        g_free(filename);
    }
    gchar *destdir = g_path_get_dirname(dest);
    if (!g_file_test(destdir, G_FILE_TEST_IS_DIR))
    {
        message = g_strdup_printf(
                    _("Destination: %s\n\n"
                    "This is not a valid directory."),
                    destdir);
        ghb_message_dialog(GTK_MESSAGE_ERROR, message, _("Cancel"), NULL);
        g_free(dest);
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
        ghb_message_dialog(GTK_MESSAGE_ERROR, message, _("Cancel"), NULL);
        g_free(dest);
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
                    if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message, _("Cancel"), _("Proceed")))
                    {
                        g_free(dest);
                        g_free(message);
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
        if (!ghb_message_dialog(GTK_MESSAGE_QUESTION, message, _("Cancel"), _("Overwrite")))
        {
            g_free(dest);
            g_free(message);
            return FALSE;
        }
        g_free(message);
        g_unlink(dest);
    }
    g_free(dest);
    // Validate video quality is in a reasonable range
    if (!ghb_validate_vquality(settings))
    {
        return FALSE;
    }
    // Validate audio settings
    if (!ghb_validate_audio(settings))
    {
        return FALSE;
    }
    // Validate audio settings
    if (!ghb_validate_subtitles(settings))
    {
        return FALSE;
    }
    // Validate video settings
    if (!ghb_validate_video(settings))
    {
        return FALSE;
    }
    // Validate filter settings
    if (!ghb_validate_filters(settings))
    {
        return FALSE;
    }
    return TRUE;
}

static gboolean
queue_add(signal_user_data_t *ud, GValue *settings, gint batch)
{
    // Add settings to the queue
    g_debug("queue_add ()");
    if (!validate_settings(ud, settings, batch))
    {
        return FALSE;
    }

    if (ud->queue == NULL)
        ud->queue = ghb_array_value_new(32);

    // Copy current prefs into settings
    // The job should run with the preferences that existed
    // when the job was added to the queue.
    ghb_settings_set_value(settings, "Preferences", ud->prefs);

    // Make a copy of current settings to be used for the new job
    ghb_settings_set_int(settings, "job_status", GHB_QUEUE_PENDING);
    ghb_settings_set_int(settings, "job_unique_id", 0);

    ghb_array_append(ud->queue, settings);
    add_to_queue_list(ud, settings, NULL);
    ghb_save_queue(ud->queue);
    ghb_update_pending(ud);

    return TRUE;
}

G_MODULE_EXPORT void
queue_add_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    g_debug("queue_add_clicked_cb ()");
    GValue *settings = ghb_value_dup(ud->settings);
    if (!queue_add(ud, settings, 0))
        ghb_value_free(settings);
    // Validation of settings may have changed audio list
    ghb_audio_list_refresh_all(ud);
}

static gboolean
title_multiple_can_select(GValue *settings_array, int index)
{
    gint count, ii;
    GValue *settings, *gdest;
    const char *dest;

    settings = ghb_array_get_nth(settings_array, index);
    gdest = ghb_settings_get_value(settings, "destination");
    dest = g_value_get_string(gdest);
    if (dest == NULL)
        return FALSE;

    count = ghb_array_len(settings_array);
    count = count < index ? count : index;
    for (ii = 0; ii < count; ii++)
    {
        const char *tmp;

        settings = ghb_array_get_nth(settings_array, ii);
        gdest = ghb_settings_get_value(settings, "destination");
        tmp = g_value_get_string(gdest);
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
    GValue *settings;
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

        settings = ghb_array_get_nth(ud->settings_array, ii);
        can_select = title_multiple_can_select(ud->settings_array, ii);
        ghb_settings_set_boolean(settings, "title_selected", FALSE);
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
    GValue *settings;
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
        settings = ghb_array_get_nth(ud->settings_array, ii);
        can_select = title_multiple_can_select(ud->settings_array, ii);
        ghb_settings_set_boolean(settings, "title_selected", can_select);
        gtk_toggle_button_set_active(selected, TRUE);
        title_add_multiple_set_sensitive(GTK_WIDGET(row), can_select);
    }
    clear_select_all_busy = FALSE;
}

G_MODULE_EXPORT void
title_add_multiple_clear_all_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    gint count, ii;
    GValue *settings;
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
        settings = ghb_array_get_nth(ud->settings_array, ii);
        ghb_settings_set_boolean(settings, "title_selected", FALSE);
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
        GValue *settings;

        settings = ghb_value_dup(ghb_array_get_nth(ud->settings_array, ii));
        if (ghb_settings_get_boolean(settings, "title_selected"))
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
    GValue *settings;
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
    settings = ghb_array_get_nth(ud->settings_array, index);
    can_select = title_multiple_can_select(ud->settings_array, index);
    ghb_settings_set_boolean(settings, "title_selected",
                             selected && can_select);
}

G_MODULE_EXPORT void
title_dest_file_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GValue *settings;
    gchar *dest_file, *dest_dir, *dest;
    GtkListBoxRow * row = title_get_row(widget);
    if (row == NULL)
        return;
    gint index = gtk_list_box_row_get_index(row);

    dest_file = ghb_widget_string(widget);
    settings = ghb_array_get_nth(ud->settings_array, index);

    ghb_settings_set_string(settings, "dest_file", dest_file);
    dest_dir = ghb_settings_get_string(settings, "dest_dir");
    dest = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", dest_dir, dest_file);
    ghb_settings_set_string(settings, "destination", dest);

    // Check if changing the destination file name resolves
    // a file name conflict.  Enable selection if so.
    // Disable selection if it creates a confict!!!
    gboolean can_select;
    can_select = title_multiple_can_select(ud->settings_array, index);
    ghb_settings_set_boolean(settings, "title_selected", can_select);
    title_add_multiple_set_sensitive(GTK_WIDGET(row), can_select);

    g_free(dest_file);
    g_free(dest_dir);
    g_free(dest);

    title_add_multiple_set_conflict_label(ud,
        title_add_multiple_are_conflicts(ud));
}

G_MODULE_EXPORT void
title_dest_dir_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GValue *settings;
    gchar *dest_file, *dest_dir, *dest;
    GtkListBoxRow * row = title_get_row(widget);
    if (row == NULL)
        return;
    gint index = gtk_list_box_row_get_index(row);

    dest_dir = ghb_widget_string(widget);
    settings = ghb_array_get_nth(ud->settings_array, index);

    ghb_settings_set_string(settings, "dest_dir", dest_dir);
    dest_file = ghb_settings_get_string(settings, "dest_file");
    dest = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", dest_dir, dest_file);
    ghb_settings_set_string(settings, "destination", dest);

    // Check if changing the destination file name resolves
    // a file name conflict.  Enable selection if so.
    // Disable selection if it creates a confict!!!
    gboolean can_select;
    can_select = title_multiple_can_select(ud->settings_array, index);
    ghb_settings_set_boolean(settings, "title_selected", can_select);
    title_add_multiple_set_sensitive(GTK_WIDGET(row), can_select);

    g_free(dest_file);
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
      "Select this title for adding to the queue.\n");
    gtk_widget_set_valign(GTK_WIDGET(selected), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(selected), "title_selected");
    gtk_widget_show(GTK_WIDGET(selected));
    g_signal_connect(selected, "toggled", (GCallback)title_selected_cb, ud);
    gtk_box_pack_start(hbox, GTK_WIDGET(selected), FALSE, FALSE, 0);

    // Title label
    title = GTK_LABEL(gtk_label_new("No Title"));
    gtk_label_set_width_chars(title, 12);
    gtk_misc_set_alignment(GTK_MISC(title), 0, 0.5);
    gtk_widget_set_name(GTK_WIDGET(title), "title_label");
    gtk_widget_show(GTK_WIDGET(title));
    gtk_box_pack_start(hbox, GTK_WIDGET(title), FALSE, FALSE, 0);

    default_title_attrs = gtk_label_get_attributes(title);
    gtk_widget_set_tooltip_text(GTK_WIDGET(title),
        "There is another title with the same destination file name.\n"
        "This title will not be added to the queue unless you change\n"
        "the output file name.\n");
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
            gtk_file_chooser_button_new("Destination Directory",
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
        GValue *settings;
        GtkLabel *label;
        GtkEntry *entry;
        GtkFileChooser *chooser;
        gchar *title_label, *dest_dir, *dest_file;
        int title_id, titleindex;
        const hb_title_t *title;

        row = title_create_row(ud);
        label = GTK_LABEL(find_widget(row, "title_label"));
        entry = GTK_ENTRY(find_widget(row, "title_file"));
        chooser = GTK_FILE_CHOOSER(find_widget(row, "title_dir"));

        settings = ghb_array_get_nth(ud->settings_array, ii);
        title_id = ghb_settings_get_int(settings, "title");
        title = ghb_lookup_title(title_id, &titleindex);
        if (title != NULL)
        {
            int len;

            title_label = ghb_create_title_label(title);
            len = strnlen(title_label, PATH_MAX);
            if (len > max_title_len)
                max_title_len = len;

            dest_file = ghb_settings_get_string(settings, "dest_file");
            dest_dir = ghb_settings_get_string(settings, "dest_dir");

            gtk_label_set_markup(label, title_label);
            gtk_entry_set_text(entry, dest_file);
            gtk_file_chooser_set_filename(chooser, dest_dir);

            g_free(title_label);
            g_free(dest_file);
            g_free(dest_dir);
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
    GValue *settings;
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
        settings = ghb_array_get_nth(ud->queue, row);
        status = ghb_settings_get_int(settings, "job_status");
        if (status == GHB_QUEUE_RUNNING)
        {
            // Ask if wants to stop encode.
            if (!ghb_cancel_encode2(ud, NULL))
            {
                return;
            }
            unique_id = ghb_settings_get_int(settings, "job_unique_id");
            ghb_remove_job(unique_id);
        }
        // Remove the selected item
        gtk_tree_store_remove(GTK_TREE_STORE(store), &iter);
        // Remove the corresponding item from the queue list
        GValue *old = ghb_array_get_nth(ud->queue, row);
        ghb_array_remove(ud->queue, row);
        ghb_value_free(old);
        ghb_save_queue(ud->queue);
    }
    else
    {
        gtk_tree_path_free (treepath);
    }
    ghb_update_pending(ud);
}

static gint
find_last_finished(GValue *queue)
{
    GValue *js;
    gint ii, count;
    gint status;

    g_debug("find_last_finished");
    count = ghb_array_len(queue);
    for (ii = 0; ii < count; ii++)
    {
        js = ghb_array_get_nth(queue, ii);
        status = ghb_settings_get_int(js, "job_status");
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
    GValue *js;
    GtkTreeIter iter;
    GtkTreeView *srctv;
    GtkTreeModel *model;
    GtkTreeSelection *select;
    GtkWidget *widget;

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
    js = ghb_array_get_nth(ud->queue, row);
    status = ghb_settings_get_int(js, "job_status");
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
    js = ghb_array_get_nth(ud->queue, row);

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
    GValue *js;

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
            js = ghb_array_get_nth(ud->queue, row);

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
            ghb_settings_set_int(js, "job_status", GHB_QUEUE_PENDING);
            add_to_queue_list(ud, js, &iter);

            dstpath = gtk_tree_model_get_path (dstmodel, &iter);
            indices = gtk_tree_path_get_indices(dstpath);
            row = indices[0];
            gtk_tree_path_free(dstpath);
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
    title_id = ghb_settings_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);

    queue_state = ghb_get_queue_state();
    scan_state = ghb_get_scan_state();

    show_stop = queue_state &
                (GHB_STATE_WORKING | GHB_STATE_SEARCHING |
                 GHB_STATE_SCANNING | GHB_STATE_MUXING);
    show_start = !(scan_state & GHB_STATE_SCANNING) &&
                    (title !=NULL || queue_count > 0);


    paused = queue_state & GHB_STATE_PAUSED;

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
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Stop"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Stop Encoding"));
    }
    else
    {
        gtk_widget_set_sensitive (widget, show_start);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-start");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Start"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Start Encoding"));
    }
    widget = GHB_WIDGET (ud->builder, "queue_start2");
    if (show_stop)
    {
        gtk_widget_set_sensitive (widget, TRUE);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-stop");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Stop"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Stop Encoding"));
    }
    else
    {
        gtk_widget_set_sensitive (widget, show_start);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-start");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Start"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Start Encoding"));
    }
    widget = GHB_WIDGET (ud->builder, "queue_pause1");
    if (paused)
    {
        gtk_widget_set_sensitive (widget, show_stop);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-start");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Resume"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Resume Encoding"));
    }
    else
    {
        gtk_widget_set_sensitive (widget, show_stop);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-pause");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Pause"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Pause Encoding"));
    }
    widget = GHB_WIDGET (ud->builder, "queue_pause2");
    if (paused)
    {
        gtk_widget_set_sensitive (widget, show_stop);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-start");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Resume"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Resume Encoding"));
    }
    else
    {
        gtk_widget_set_sensitive (widget, show_stop);
        gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), "hb-pause");
        gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _("Pause"));
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _("Pause Encoding"));
    }

    widget = GHB_WIDGET (ud->builder, "queue_start_menu");
    if (show_stop)
    {
        gtk_widget_set_sensitive (widget, TRUE);
#if GTK_CHECK_VERSION(2, 16, 0)
        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), _("S_top Queue"));
        gtk_widget_set_tooltip_text(widget, _("Stop Encoding"));
#else
        g_object_set_property(G_OBJECT(widget), "label",
                                            ghb_string_value(_("S_top Queue")));
        g_object_set_property(G_OBJECT(widget), "tooltip",
                                            ghb_string_value(_("Stop Encoding")));
#endif
    }
    else
    {
        gtk_widget_set_sensitive (widget, show_start);
#if GTK_CHECK_VERSION(2, 16, 0)
        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), _("_Start Queue"));
        gtk_widget_set_tooltip_text(widget, _("Start Encoding"));
#else
        g_object_set_property(G_OBJECT(widget), "label",
                                            ghb_string_value(_("_Start Queue")));
        g_object_set_property(G_OBJECT(widget), "tooltip",
                                            ghb_string_value(_("Start Encoding")));
#endif
    }
    widget = GHB_WIDGET (ud->builder, "queue_pause_menu");
    if (paused)
    {
        gtk_widget_set_sensitive (widget, show_start);
#if GTK_CHECK_VERSION(2, 16, 0)
        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), "_Resume Queue");
        gtk_widget_set_tooltip_text(widget, "Resume Encoding");
#else
        g_object_set_property(G_OBJECT(widget), "label",
                                        ghb_string_value(_("_Resume Queue")));
        g_object_set_property(G_OBJECT(widget), "tooltip",
                                        ghb_string_value(_("Resume Encoding")));
#endif
    }
    else
    {
        gtk_widget_set_sensitive (widget, show_stop);
#if GTK_CHECK_VERSION(2, 16, 0)
        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), _("_Pause Queue"));
        gtk_widget_set_tooltip_text(widget, _("Pause Encoding"));
#else
        g_object_set_property(G_OBJECT(widget), "label",
                                        ghb_string_value(_("_Pause Queue")));
        g_object_set_property(G_OBJECT(widget), "tooltip",
                                        ghb_string_value(_("Pause Encoding")));
#endif
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
    GValue *js;
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
        js = ghb_array_get_nth(ud->queue, ii);
        status = ghb_settings_get_int(js, "job_status");
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
        GValue *settings = ghb_value_dup(ud->settings);
        if (!queue_add(ud, settings, 0))
        {
            ghb_value_free(settings);
            return;
        }
        // Validation of settings may have changed audio list
        ghb_audio_list_refresh_all(ud);
    }
    if (state == GHB_STATE_IDLE)
    {
        // Add the first pending queue item and start
        ud->current_job = ghb_start_next_job(ud, TRUE);
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
    GValue *queue;
    gint unfinished = 0;
    gint count, ii;
    gint pid;
    gint status;
    GValue *settings;
    gchar *message;

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
        settings = ghb_array_get_nth(queue, ii);
        status = ghb_settings_get_int(settings, "job_status");
        if (status != GHB_QUEUE_DONE && status != GHB_QUEUE_CANCELED)
        {
            unfinished++;
        }
    }
    if (!unfinished)
        goto find_pid;

    if (unfinished)
    {
        message = g_strdup_printf(
                    _("You have %d unfinished job(s) in a saved queue.\n\n"
                    "Would you like to reload them?"), unfinished);
        if (ghb_message_dialog(GTK_MESSAGE_QUESTION, message, _("No"), _("Yes")))
        {
            GtkWidget *widget = GHB_WIDGET (ud->builder, "queue_window");
            gtk_widget_show (widget);
            widget = GHB_WIDGET (ud->builder, "show_queue");
            gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(widget), TRUE);

            ud->queue = queue;
            // First get rid of any old items we don't want
            for (ii = count-1; ii >= 0; ii--)
            {
                settings = ghb_array_get_nth(queue, ii);
                status = ghb_settings_get_int(settings, "job_status");
                if (status == GHB_QUEUE_DONE || status == GHB_QUEUE_CANCELED)
                {
                    GValue *old = ghb_array_get_nth(queue, ii);
                    ghb_value_free(old);
                    ghb_array_remove(queue, ii);
                }
            }
            count = ghb_array_len(queue);
            for (ii = 0; ii < count; ii++)
            {
                settings = ghb_array_get_nth(queue, ii);
                ghb_settings_set_int(settings, "job_unique_id", 0);
                ghb_settings_set_int(settings, "job_status", GHB_QUEUE_PENDING);
                add_to_queue_list(ud, settings, NULL);
            }
            ghb_queue_buttons_grey(ud);
            ghb_save_queue(ud->queue);
        }
        else
        {
            ghb_value_free(queue);
        }
        g_free(message);
    }
    else
    {
        ghb_value_free(queue);
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
    GValue *settings;
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
        settings = ghb_array_get_nth(ud->queue, row);
        status = ghb_settings_get_int(settings, "job_status");
        if (status == GHB_QUEUE_RUNNING)
        {
            // Ask if wants to stop encode.
            if (!ghb_cancel_encode2(ud, NULL))
            {
                return TRUE;
            }
            unique_id = ghb_settings_get_int(settings, "job_unique_id");
            ghb_remove_job(unique_id);
        }
        // Remove the selected item
        gtk_tree_store_remove(GTK_TREE_STORE(store), &iter);
        // Remove the corresponding item from the queue list
        GValue *old = ghb_array_get_nth(ud->queue, row);
        ghb_value_free(old);
        ghb_array_remove(ud->queue, row);
        ghb_save_queue(ud->queue);
        return TRUE;
    }
    return FALSE;
}

GValue *ghb_queue_edit_settings = NULL;

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
        ghb_queue_edit_settings = ghb_array_get_nth(ud->queue, row);
        status = ghb_settings_get_int(ghb_queue_edit_settings, "job_status");
        if (status == GHB_QUEUE_PENDING)
        {
            // Remove the selected item
            gtk_tree_store_remove(GTK_TREE_STORE(store), &iter);
            // Remove the corresponding item from the queue list
            ghb_array_remove(ud->queue, row);
        }
        else
        {
            ghb_queue_edit_settings = ghb_value_dup(ghb_queue_edit_settings);
        }
        gchar *source;
        source = ghb_settings_get_string(ghb_queue_edit_settings, "source");
        ghb_do_scan(ud, source, 0, FALSE);
        g_free(source);
    }
}

