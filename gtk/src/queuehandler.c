/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * queuehandler.c
 * Copyright (C) John Stebbins 2008-2019 <stebbins@stebbins>
 *
 * queuehandler.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
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
queue_remove_clicked_cb(GtkWidget *widget, signal_user_data_t *ud);

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
    GhbValue           * uiDict = NULL;
    GhbValue           * titleDict = NULL;

    if (queueDict != NULL)
    {
        uiDict    = ghb_dict_get(queueDict, "uiSettings");
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
        g_string_append_printf(str, _("Modified, %s"), name);
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
    ctext = ghb_dict_get_string(titleDict, "Path");
    widget = GHB_WIDGET(ud->builder, "queue_summary_source");
    gtk_label_set_text(GTK_LABEL(widget), ctext);

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
        g_string_append_printf(str, "%s%s", sep, "Chapter Markers");
        sep = ", ";
    }
    if (av_align)
    {
        g_string_append_printf(str, "%s%s", sep, "Align A/V");
        sep = ", ";
    }
    if (http)
    {
        g_string_append_printf(str, "%s%s", sep, "Web Optimized");
        sep = ", ";
    }
    if (ipod)
    {
        g_string_append_printf(str, "%s%s", sep, "iPod 5G");
        sep = ", ";
    }

    text = g_string_free(str, FALSE);
    widget = GHB_WIDGET(ud->builder, "queue_summary_dest");
    gtk_label_set_text(GTK_LABEL(widget), text);
    g_free(text);

    // Dimenstions
    double display_width;
    int    width, height, display_height, par_width, par_height;
    int    crop[4];
    char * display_aspect;

    width          = ghb_dict_get_int(uiDict, "scale_width");
    height         = ghb_dict_get_int(uiDict, "scale_height");
    display_width  = ghb_dict_get_int(uiDict, "PictureDisplayWidth");
    display_height = ghb_dict_get_int(uiDict, "PictureDisplayHeight");
    par_width      = ghb_dict_get_int(uiDict, "PicturePARWidth");
    par_height     = ghb_dict_get_int(uiDict, "PicturePARHeight");
    crop[0]        = ghb_dict_get_int(uiDict, "PictureTopCrop");
    crop[1]        = ghb_dict_get_int(uiDict, "PictureBottomCrop");
    crop[2]        = ghb_dict_get_int(uiDict, "PictureLeftCrop");
    crop[3]        = ghb_dict_get_int(uiDict, "PictureRightCrop");


    display_width = (double)width * par_width / par_height;
    display_aspect = ghb_get_display_aspect_string(display_width,
                                                   display_height);

    display_width  = ghb_dict_get_int(uiDict, "PictureDisplayWidth");
    text = g_strdup_printf("%d:%d:%d:%d Crop\n"
                           "%dx%d storage, %dx%d display\n"
                           "%d:%d Pixel Aspect Ratio\n"
                            "%s Display Aspect Ratio",
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
            g_string_append_printf(str, "%s, Bitrate %dkbps",
                                   video_encoder->name, br);
        }
        else
        {
            g_string_append_printf(str, "%s, Bitrate %dkbps (2 Pass)",
                                   video_encoder->name, br);
        }
    }
    else
    {
        gdouble quality = ghb_dict_get_double(uiDict, "VideoQualitySlider");
        g_string_append_printf(str, "%s, Constant Quality %.4g(%s)",
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
        g_string_append_printf(str, "%sPreset %s", sep, enc_preset);
        sep = ", ";
    }
    if (enc_tune != NULL)
    {
        g_string_append_printf(str, "%sTune %s", sep, enc_tune);
        sep = ", ";
    }
    if (enc_profile != NULL)
    {
        g_string_append_printf(str, "%sProfile %s", sep, enc_profile);
        sep = ", ";
    }
    if (enc_level != NULL)
    {
        g_string_append_printf(str, "%sLevel %s", sep, enc_level);
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
        g_string_append_printf(str, "\nConstant Framerate %s fps", rate_str);
    }
    else if (ghb_dict_get_bool(uiDict, "VideoFrameratePFR"))
    {
        g_string_append_printf(str, "\nPeak Framerate %s fps (may be lower)",
                               rate_str);
    }
    else if (ghb_dict_get_bool(uiDict, "VideoFramerateVFR"))
    {
        g_string_append_printf(str, "\nVariable Framerate %s fps", rate_str);
    }
    g_free(rate_str);

    // Append Filters to video summary
    gboolean     detel, comb_detect, deint, decomb, deblock, nlmeans, denoise;
    gboolean     unsharp, lapsharp, rot, gray;

    ctext       = ghb_dict_get_string(uiDict, "PictureDetelecine");
    detel       = ctext != NULL && !!strcasecmp(ctext, "off");
    ctext       = ghb_dict_get_string(uiDict, "PictureCombDetectPreset");
    comb_detect = ctext != NULL && !!strcasecmp(ctext, "off");
    ctext       = ghb_dict_get_string(uiDict, "PictureDeinterlaceFilter");
    deint       = ctext != NULL && !strcasecmp(ctext, "deinterlace");
    decomb      = ctext != NULL && !strcasecmp(ctext, "decomb");
    ctext       = ghb_dict_get_string(uiDict, "PictureDeblockPreset");
    deblock     = ctext != NULL && !!strcasecmp(ctext, "off");
    ctext       = ghb_dict_get_string(uiDict, "PictureDenoiseFilter");
    nlmeans     = ctext != NULL && !strcasecmp(ctext, "nlmeans");
    denoise     = ctext != NULL && !strcasecmp(ctext, "hqdn3d");
    ctext       = ghb_dict_get_string(uiDict, "PictureSharpenFilter");
    unsharp     = ctext != NULL && !strcasecmp(ctext, "unsharp");
    lapsharp    = ctext != NULL && !strcasecmp(ctext, "lapsharp");
    ctext       = ghb_dict_get_string(uiDict, "PictureRotate");
    rot         = ctext != NULL && !!strcasecmp(ctext, "disable=1");
    gray        = ghb_dict_get_bool(uiDict, "VideoGrayScale");

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
    if (deint)
    {
        hb_filter_object_t * filter = hb_filter_get(HB_FILTER_DEINTERLACE);
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
    if (rot)
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

        g_string_append_printf(str, "Foreign Audio Scan");
        if (force)
        {
            g_string_append_printf(str, ", Forced Only");
        }
        if (burn)
        {
            g_string_append_printf(str, ", Burned");
        }
        else if (def)
        {
            g_string_append_printf(str, ", Default");
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
        if (force)
        {
            g_string_append_printf(str, ", Forced Only");
        }
        if (burn)
        {
            g_string_append_printf(str, ", Burned");
        }
        else if (def)
        {
            g_string_append_printf(str, ", Default");
        }
        sep = "\n";
    }

    text = g_string_free(str, FALSE);
    widget = GHB_WIDGET(ud->builder, "queue_summary_subtitle");
    gtk_label_set_text(GTK_LABEL(widget), text);
    g_free(text);
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
    GtkWidget     * queue_log_tab;

    queue_log_tab = GHB_WIDGET(ud->builder, "queue_log_tab");
    lb            = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row           = gtk_list_box_get_selected_row(lb);
    if (row != NULL)
    {
        // There is a queue list row selected
        GtkTextView * tv;
        int           status;

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
        if (status == GHB_QUEUE_RUNNING)
        {
            // Selected encode is running, enable display of log and
            // show the live buffer
            gtk_widget_set_visible(queue_log_tab, TRUE);
            if (ud->queue_activity_buffer != current)
            {
                gtk_text_view_set_buffer(tv, ud->queue_activity_buffer);
            }
        }
        else
        {
            const char * log_path;

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
                gtk_widget_set_visible(queue_log_tab, TRUE);
                ghb_ui_update(ud, "queue_activity_location",
                              ghb_string_value(log_path));
                read_log(ud, log_path);
            }
            else
            {
                // No log file, encode is pending
                // disable display of log
                gtk_widget_set_visible(queue_log_tab, FALSE);
            }
        }
    }
    else
    {
        // No row selected, disable display of log
        gtk_widget_set_visible(queue_log_tab, FALSE);
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
    ghb_queue_select_log(ud);
    ghb_queue_buttons_grey(ud);
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

static void
add_to_queue_list(signal_user_data_t *ud, GhbValue *queueDict)
{
    GtkListBox * lb;
    GtkBox     * hbox, * vbox;
    GtkWidget  * status_icon;
    GtkWidget  * dest_label;
    GtkWidget  * delete_button;
    GtkWidget  * progress;
    GhbValue   * uiDict;
    const char * dest;
    gchar      * basename;

    lb     = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    uiDict = ghb_dict_get(queueDict, "uiSettings");

    vbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 6));
    gtk_widget_set_margin_start(GTK_WIDGET(vbox), 12);
    hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6));

    status_icon = gtk_image_new_from_icon_name("hb-source",
                                               GTK_ICON_SIZE_BUTTON);

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

    delete_button = gtk_button_new_from_icon_name("hb-remove",
                                               GTK_ICON_SIZE_BUTTON);
    gtk_button_set_relief(GTK_BUTTON(delete_button), GTK_RELIEF_NONE);
    g_signal_connect(delete_button, "clicked",
                     (GCallback)queue_remove_clicked_cb, ud);
    gtk_widget_set_hexpand(delete_button, FALSE);

    progress = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.0);
    gtk_widget_set_name(progress, "queue_item_progress");

    ghb_box_pack_start(hbox, GTK_WIDGET(status_icon));
    ghb_box_pack_start(hbox, GTK_WIDGET(dest_label));
    ghb_box_pack_start(hbox, GTK_WIDGET(delete_button));

    ghb_box_pack_start(vbox, GTK_WIDGET(hbox));
    ghb_box_pack_start(vbox, GTK_WIDGET(progress));

    gtk_widget_show(GTK_WIDGET(vbox));
    gtk_widget_show(GTK_WIDGET(hbox));
    gtk_widget_show(status_icon);
    gtk_widget_show(dest_label);
    gtk_widget_show(delete_button);
    gtk_list_box_insert(lb, GTK_WIDGET(vbox), -1);
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
             icon_name = "hb-source";
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
    gtk_image_set_from_icon_name(status_icon, icon_name,
                                 GTK_ICON_SIZE_BUTTON);
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
        ghb_queue_progress_set_visible(ud, index, 0);
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
    int ii, count;
    GhbValue *queue = ghb_value_dup(ud->queue);

    count = ghb_array_len(queue);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *queueDict, *uiDict;

        queueDict = ghb_array_get(ud->queue, ii);
        uiDict = ghb_dict_get(queueDict, "uiSettings");
        if (uiDict == NULL)
            continue;
        ghb_dict_set_int(uiDict, "job_status", GHB_QUEUE_PENDING);
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
queue_window_delete_cb(GtkWidget *xwidget, GdkEvent *event, signal_user_data_t *ud)
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
    GtkWidget       *dialog;
    GtkResponseType  response;
    ghb_status_t     status;
    const char      *paused_msg = "";
    const char      *dest;
    gint64           free_size;
    gint64           free_limit;
    GhbValue        *qDict;
    GhbValue        *settings;

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

    filter_list = ghb_get_job_filter_list(settings);
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

    GhbValue *jobDict    = ghb_get_job_settings(settings);
    GhbValue *sourceDict = ghb_get_job_source_settings(settings);
    GhbValue *queueDict  = ghb_dict_new();
    GhbValue *uiDict     = ghb_value_dup(settings);
    ghb_dict_remove(uiDict, "Job");
    int       title_id   = ghb_dict_get_int(sourceDict, "Title");
    GhbValue *titleDict  = ghb_get_title_dict(title_id);
    ghb_dict_set(queueDict, "uiSettings", uiDict);
    ghb_dict_set(queueDict, "Job", ghb_value_dup(jobDict));
    ghb_dict_set(queueDict, "Title", titleDict);

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

G_MODULE_EXPORT void
queue_add_action_cb(GSimpleAction *action, GVariant *param,
                    signal_user_data_t *ud)
{
    queue_add(ud, ud->settings, 0);
    ghb_queue_selection_init(ud);
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
    ghb_box_pack_start(hbox, GTK_WIDGET(selected));

    // Title label
    title = GTK_LABEL(gtk_label_new(_("No Title")));
    gtk_label_set_width_chars(title, 12);
    gtk_widget_set_halign(GTK_WIDGET(title), GTK_ALIGN_START);
    gtk_widget_set_valign(GTK_WIDGET(title), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(title), "title_label");
    gtk_widget_show(GTK_WIDGET(title));
    ghb_box_pack_start(hbox, GTK_WIDGET(title));

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
    gtk_entry_set_width_chars(dest_file, 40);
    gtk_widget_set_name(GTK_WIDGET(dest_file), "title_file");
    //gtk_widget_set_hexpand(GTK_WIDGET(dest_file), TRUE);
    gtk_widget_show(GTK_WIDGET(dest_file));
    g_signal_connect(dest_file, "changed", (GCallback)title_dest_file_cb, ud);
    ghb_box_pack_start(vbox_dest, GTK_WIDGET(dest_file));
    dest_dir = GTK_FILE_CHOOSER_BUTTON(
            gtk_file_chooser_button_new(_("Destination Directory"),
                                        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER));
    g_signal_connect(dest_dir, "selection-changed",
                    (GCallback)title_dest_dir_cb, ud);
    gtk_widget_set_name(GTK_WIDGET(dest_dir), "title_dir");
    gtk_widget_set_hexpand(GTK_WIDGET(dest_dir), TRUE);
    gtk_widget_show(GTK_WIDGET(dest_dir));
    ghb_box_pack_start(vbox_dest, GTK_WIDGET(dest_dir));
    gtk_widget_show(GTK_WIDGET(vbox_dest));
    ghb_box_pack_start(hbox, GTK_WIDGET(vbox_dest));

    return GTK_WIDGET(hbox);
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
    ghb_queue_select_log(ud);
    ghb_update_pending(ud);
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
        ghb_queue_select_log(ud);
        ghb_update_pending(ud);
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

static gint
find_last_finished(GhbValue *queue)
{
    GhbValue *queueDict, *uiDict;
    gint ii, count;
    gint status;

    g_debug("find_last_finished");
    count = ghb_array_len(queue);
    for (ii = 0; ii < count; ii++)
    {
        queueDict = ghb_array_get(queue, ii);
        uiDict = ghb_dict_get(queueDict, "uiSettings");
        status = ghb_dict_get_int(uiDict, "job_status");
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
    GtkListBox         * lb,
    GdkDragContext     * ctx,
    gint                 x,
    gint                 y,
    guint                time,
    signal_user_data_t * ud)
{
    gint            src_index, dst_index, status, finished, height;
    GhbValue      * queueDict, * uiDict;
    GtkListBox    * src_lb;
    GtkListBoxRow * src_row, * dst_row;

    height = gtk_widget_get_allocated_height(GTK_WIDGET(lb));
    if (y <= 6 || y >= height - 6)
    {
        return FALSE;
    }

    src_lb = GTK_LIST_BOX(gtk_drag_get_source_widget(ctx));
    if (src_lb == NULL || src_lb != lb)
    {
        return TRUE;
    }

    // This bit checks to see if the source is allowed to be
    // moved.  Only pending and canceled items may be moved.
    src_row   = gtk_list_box_get_selected_row(src_lb);
    if (src_row == NULL)
    {
        gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
        return TRUE;
    }
    src_index = gtk_list_box_row_get_index(src_row);
    queueDict = ghb_array_get(ud->queue, src_index);
    uiDict    = ghb_dict_get(queueDict, "uiSettings");
    status    = ghb_dict_get_int(uiDict, "job_status");
    if (status != GHB_QUEUE_PENDING && status != GHB_QUEUE_CANCELED)
    {
        gdk_drag_status(ctx, 0, time);
        return TRUE;
    }

    // The rest checks that the destination is a valid position
    // in the list.  Can not move above any finished or running items
    dst_row   = gtk_list_box_get_row_at_y(lb, y);
    if (dst_row == NULL)
    {
        gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
        return TRUE;
    }
    dst_index = gtk_list_box_row_get_index(dst_row);

    finished = find_last_finished(ud->queue);
    if (dst_index < finished)
    {
        gdk_drag_status(ctx, 0, time);
        return TRUE;
    }
    gtk_list_box_drag_highlight_row(lb, dst_row);
    gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
    return TRUE;
}

G_MODULE_EXPORT void
queue_drag_cb(
    GtkListBox         * lb,
    GdkDragContext     * ctx,
    gint                 x,
    gint                 y,
    GtkSelectionData   * selection_data,
    guint                info,
    guint                t,
    signal_user_data_t * ud)
{
    GtkListBox    * src_lb;
    GtkListBoxRow * src_row, * dst_row;
    gint            src_index, dst_index;
    GhbValue      * queueDict, * uiDict;

    dst_row = gtk_list_box_get_row_at_y(lb, y);
    if (dst_row == NULL)
    {
        // Dropping after last item
        dst_index = ghb_array_len(ud->queue);
    }
    else
    {
        dst_index = gtk_list_box_row_get_index(dst_row);
    }

    src_lb = GTK_LIST_BOX(gtk_drag_get_source_widget(ctx));
    if (src_lb == NULL || src_lb != lb)
    {
        return;
    }

    src_row   = gtk_list_box_get_selected_row(src_lb);
    if (src_row == NULL)
    {
        return;
    }
    src_index = gtk_list_box_row_get_index(src_row);
    if (src_index < dst_index)
    {
        // The source row is removed before re-inserting it elsewhere
        // in the list.  If the source is before the dest, the dest position
        // moves
        dst_index -= 1;
    }
    g_object_ref(src_row);
    gtk_container_remove(GTK_CONTAINER(lb), GTK_WIDGET(src_row));
    gtk_list_box_insert(lb, GTK_WIDGET(src_row), dst_index);
    g_object_unref(src_row);

    queueDict = ghb_array_get(ud->queue, src_index);
    uiDict = ghb_dict_get(queueDict, "uiSettings");

    ghb_value_incref(queueDict);
    ghb_array_remove(ud->queue, src_index);
    ghb_array_insert(ud->queue, dst_index, queueDict);

    // Reset job to pending
    ghb_dict_set_int(uiDict, "job_status", GHB_QUEUE_PENDING);

    // TODO: update job status icon in dst
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

    lb = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_selected_row(lb);

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
    widget = GHB_WIDGET (ud->builder, "queue_start_menu");
    if (show_stop)
    {
        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), _("S_top Encoding"));
        gtk_widget_set_tooltip_text(widget, _("Stop Encoding"));
    }
    else
    {
        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), _("_Start Encoding"));
        gtk_widget_set_tooltip_text(widget, _("Start Encoding"));
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
    widget = GHB_WIDGET (ud->builder, "queue_pause_menu");
    if (paused)
    {
        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), _("_Resume Encoding"));
        gtk_widget_set_tooltip_text(widget, _("Resume Encoding"));
    }
    else
    {
        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), _("_Pause Encoding"));
        gtk_widget_set_tooltip_text(widget, _("Pause Encoding"));
    }
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

G_MODULE_EXPORT gboolean
queue_key_press_cb(
    GtkWidget          * widget,
    GdkEvent           * event,
    signal_user_data_t * ud)
{
    GtkListBox    * lb;
    GtkListBoxRow * row;
    gint            index;
    guint           keyval;

    ghb_event_get_keyval(event, &keyval);
    if (keyval != GDK_KEY_Delete)
        return FALSE;

    lb  = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "queue_list"));
    row = gtk_list_box_get_selected_row(lb);
    if (row != NULL)
    {
        index = gtk_list_box_row_get_index(row);
        ghb_queue_remove_row_internal(ud, index);
        ghb_save_queue(ud->queue);
        return TRUE;
    }
    return FALSE;
}

