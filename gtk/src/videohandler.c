/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * videohandler.c
 * Copyright (C) John Stebbins 2008-2022 <stebbins@stebbins>
 *
 * videohandler.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * videohandler.c is distributed in the hope that it will be useful,
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

#include <glib/gi18n.h>
#include <string.h>

#include "settings.h"
#include "values.h"
#include "callbacks.h"
#include "presets.h"
#include "preview.h"
#include "hb-backend.h"
#include "videohandler.h"

int ghb_get_video_encoder(GhbValue *settings)
{
    const char *encoder;
    encoder = ghb_dict_get_string(settings, "VideoEncoder");
    return hb_video_encoder_get_from_name(encoder);
}

int ghb_set_video_preset(GhbValue *settings, int encoder, const char * preset)
{
    const char * const * videoPresets;
    int                  ii, result = 0;

    videoPresets = hb_video_encoder_get_presets(encoder);
    for (ii = 0; videoPresets && videoPresets[ii]; ii++)
    {
        if (preset != NULL && !strcasecmp(preset, videoPresets[ii]))
        {
            ghb_dict_set_int(settings, "VideoPresetSlider", ii);
            result = 1;
            break;
        }
    }
    if (preset == NULL && videoPresets != NULL)
    {
        // Pick the center 'medium' preset
        ii = ii / 2;
        preset = videoPresets[ii];
        ghb_dict_set_int(settings, "VideoPresetSlider", ii);
        result = 1;
    }
    if (preset != NULL)
    {
        ghb_dict_set_string(settings, "VideoPreset", preset);
    }
    return result;
}

G_MODULE_EXPORT void
vcodec_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    float val, vqmin, vqmax, step, page;
    int inverted, digits;

    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_show_container_options(ud);
    ghb_update_summary_info(ud);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);

    // Set the range of the video quality slider
    val = ghb_vquality_default(ud);
    ghb_vquality_range(ud, &vqmin, &vqmax, &step, &page, &digits, &inverted);
    ghb_scale_configure(ud, "VideoQualitySlider", val, vqmin, vqmax,
                        step, page, digits, inverted);

    ghb_update_ui_combo_box(ud, "VideoTune", NULL, FALSE);
    ghb_update_ui_combo_box(ud, "VideoProfile", NULL, FALSE);
    ghb_update_ui_combo_box(ud, "VideoLevel", NULL, FALSE);
    ghb_ui_update(ud, "VideoTune", ghb_int_value(0));
    ghb_ui_update(ud, "VideoProfile", ghb_int_value(0));
    ghb_ui_update(ud, "VideoLevel", ghb_int_value(0));
    ghb_ui_update(ud, "VideoOptionExtra", ghb_string_value(""));

    // Set the range of the preset slider
    int encoder = ghb_get_video_encoder(ud->settings);
    GtkWidget *presetSlider = GHB_WIDGET(ud->builder, "VideoPresetSlider");
    GtkWidget *presetLabel = GHB_WIDGET(ud->builder, "VideoPresetLabel");
    const char * const *video_presets;
    int count = 0;
    video_presets = hb_video_encoder_get_presets(encoder);
    while (video_presets && video_presets[count]) count++;
    gtk_widget_set_visible(presetSlider, count > 0);
    gtk_widget_set_visible(presetLabel, count > 0);
    if (count)
    {
        gtk_range_set_range(GTK_RANGE(presetSlider), 0, count-1);
    }
    ghb_set_video_preset(ud->settings, encoder, NULL);
    GhbValue *gval = ghb_dict_get_value(ud->settings, "VideoPresetSlider");
    ghb_ui_settings_update(ud, ud->settings, "VideoPresetSlider", gval);
    if (ghb_check_name_template(ud, "{bit-depth}") ||
        ghb_check_name_template(ud, "{codec}"))
        ghb_set_destination(ud);
}

char *video_option_tooltip = NULL;

static void
update_adv_settings_tooltip(signal_user_data_t *ud)
{
    if (video_option_tooltip == NULL)
    {
        GtkWidget *eo = GTK_WIDGET(GHB_WIDGET(ud->builder, "VideoOptionExtra"));
        video_option_tooltip = gtk_widget_get_tooltip_text(eo);
    }

    int encoder = ghb_get_video_encoder(ud->settings);
    if (encoder & HB_VCODEC_X264_MASK)
    {
        GString *str = g_string_new("");
        const char *preset;
        const char *tune;
        const char *profile;
        const char *level;
        const char *opts;
        char *tunes;

        preset  = ghb_dict_get_string(ud->settings, "VideoPreset");
        tune    = ghb_dict_get_string(ud->settings, "VideoTune");
        profile = ghb_dict_get_string(ud->settings, "VideoProfile");
        level   = ghb_dict_get_string(ud->settings, "VideoLevel");
        opts    = ghb_dict_get_string(ud->settings, "VideoOptionExtra");

        if (tune[0] && strcmp(tune, "none"))
        {
            g_string_append_printf(str, "%s", tune);
        }
        if (ghb_dict_get_bool(ud->settings, "x264FastDecode"))
        {
            g_string_append_printf(str, "%s%s", str->str[0] ? "," : "", "fastdecode");
        }
        if (ghb_dict_get_bool(ud->settings, "x264ZeroLatency"))
        {
            g_string_append_printf(str, "%s%s", str->str[0] ? "," : "", "zerolatency");
        }
        tunes = g_string_free(str, FALSE);

        char * new_opts;

        int w = ghb_dict_get_int(ud->settings, "scale_width");
        int h = ghb_dict_get_int(ud->settings, "scale_height");

        if (w == 0 || h == 0)
        {
            if (!ghb_dict_get_bool(ud->settings, "PictureUseMaximumSize"))
            {
                w = ghb_dict_get_int(ud->settings, "PictureWidth");
                h = ghb_dict_get_int(ud->settings, "PictureHeight");

                if (h == 0 && w != 0)
                {
                    h = w * 9 / 16;
                }
                if (w == 0 && h != 0)
                {
                    w = h * 16 / 9;
                }
            }
            if (w == 0 || h == 0)
            {
                w = 1280;
                h = 720;
            }
        }

        if (!strcasecmp(profile, "auto"))
        {
            profile = "";
        }
        if (!strcasecmp(level, "auto"))
        {
            level = "";
        }
        new_opts = hb_x264_param_unparse(hb_video_encoder_get_depth(encoder),
                        preset, tunes, opts, profile, level, w, h);

        GtkWidget *eo = GTK_WIDGET(GHB_WIDGET(ud->builder, "VideoOptionExtra"));

        char * tt;
        if (new_opts)
            tt = g_strdup_printf(_("%s\n\nExpanded Options:\n\"%s\""),
                                 video_option_tooltip, new_opts);
        else
            tt = g_strdup_printf(_("%s\n\nExpanded Options:\n\"\""),
                                 video_option_tooltip);
        gtk_widget_set_tooltip_text(eo, tt);

        g_free(tt);
        g_free(new_opts);

        g_free(tunes);
    }
}

G_MODULE_EXPORT void
video_preset_slider_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);

    int presetIndex = ghb_dict_get_int(ud->settings, "VideoPresetSlider");
    const char * const *video_presets;
    const char *preset = "medium";
    int count;

    int encoder = ghb_get_video_encoder(ud->settings);
    video_presets = hb_video_encoder_get_presets(encoder);
    if (video_presets != NULL)
    {
        for (count = 0; video_presets[count]; count++);
        if (presetIndex < count)
        {
            preset = video_presets[presetIndex];
        }
    }

    ghb_set_video_preset(ud->settings, encoder, preset);
    GhbValue *gval = ghb_dict_get_value(ud->settings, "VideoPresetSlider");
    ghb_ui_settings_update(ud, ud->settings, "VideoPresetSlider", gval);

    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    update_adv_settings_tooltip(ud);
}

static void
video_setting_changed(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    update_adv_settings_tooltip(ud);

    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
video_setting_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    video_setting_changed(widget, ud);
}

G_MODULE_EXPORT void
video_option_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GtkWidget *textview;

    textview = GTK_WIDGET(GHB_WIDGET(ud->builder, "VideoOptionExtra"));
    video_setting_changed(textview, ud);
}

G_MODULE_EXPORT gchar*
format_video_preset_cb(GtkScale *scale, gdouble val, signal_user_data_t *ud)
{
    const char * const *video_presets;
    const char *preset;
    int encoder = ghb_get_video_encoder(ud->settings);

    video_presets = hb_video_encoder_get_presets(encoder);
    if (video_presets != NULL)
    {
        int ival = val;
        int count;
        for (count = 0; video_presets[count] != NULL; count++);
        if (ival < 0 || ival >= count)
        {
            return g_strdup_printf(" %-12s", "ERROR");
        }
        preset = video_presets[(int)val];
        // When the range of a slider changes, GTK used to sample all the
        // possible values to determine the correct amount of screen space to
        // allocate for the value strings. Some *genius* decided it would be
        // more efficient to just sample the first and last value which means
        // that if certain characters are wider than others and the middle
        // values happen to use those characters, the space allocated is
        // too small and the string wraps to the next line or is truncated.
        //
        // So, we have to randomly add some extra space to the first and
        // last value string in order for the string to be displayed properly.
        // WTF guys!
        if (ival == 0 || ival == count - 1)
            return g_strdup_printf("%-20s", preset);
        else
            return g_strdup_printf("%s", preset);
    }
    return g_strdup_printf(" %-12s", "ERROR");
}

G_MODULE_EXPORT gchar*
format_vquality_cb(GtkScale *scale, gdouble val, signal_user_data_t *ud)
{
    gint vcodec;
    const char *vqname;
    char * result;
    float vqmin, vqmax, step, page;
    int inverted, digits;

    ghb_vquality_range(ud, &vqmin, &vqmax, &step, &page, &digits, &inverted);
    vcodec = ghb_settings_video_encoder_codec(ud->settings, "VideoEncoder");
    vqname = hb_video_quality_get_name(vcodec);
    switch (vcodec)
    {
        case HB_VCODEC_FFMPEG_MPEG4:
        case HB_VCODEC_FFMPEG_MPEG2:
        case HB_VCODEC_FFMPEG_VP8:
        case HB_VCODEC_FFMPEG_VP9:
        case HB_VCODEC_FFMPEG_VP9_10BIT:
        case HB_VCODEC_THEORA:
        {
            // When the range of a slider changes, GTK used to sample all the
            // possible values to determine the correct amount of screen space to
            // allocate for the value strings. Some *genius* decided it would be
            // more efficient to just sample the first and last value which means
            // that if certain characters are wider than others and the middle
            // values happen to use those characters, the space allocated is
            // too small and the string wraps to the next line or is truncated.
            //
            // So, we have to randomly add some extra space to the first and
            // last value string in order for the string to be displayed properly.
            // WTF guys!
            if (val <= vqmin || val >= vqmax)
                result = g_strdup_printf("%s: %d   ", vqname, (int)val);
            else
                result = g_strdup_printf("%s: %d", vqname, (int)val);
        } break;

        case HB_VCODEC_X264_8BIT:
        {
            if (val == 0.0)
            {
                result = g_strdup_printf(_("%s: %.4g (Warning: lossless)"),
                                       vqname, val);
                break;
            }
        } // Falls through to default
        case HB_VCODEC_X264_10BIT:
        default:
        {
            // When the range of a slider changes, GTK used to sample all the
            // possible values to determine the correct amount of screen space to
            // allocate for the value strings. Some *genius* decided it would be
            // more efficient to just sample the first and last value which means
            // that if certain characters are wider than others and the middle
            // values happen to use those characters, the space allocated is
            // too small and the string wraps to the next line or is truncated.
            //
            // So, we have to randomly add some extra space to the first and
            // last value string in order for the string to be displayed properly.
            // WTF guys!
            if (val <= vqmin || val >= vqmax)
                result = g_strdup_printf("%s: %.4g   ", vqname, val);
            else
                result = g_strdup_printf("%s: %.4g", vqname, val);
        } break;
    }

    return result;
}

G_MODULE_EXPORT void
framerate_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_update_summary_info(ud);

    if (ghb_settings_video_framerate_rate(ud->settings, "VideoFramerate") != 0)
    {
        if (!ghb_dict_get_bool(ud->settings, "VideoFrameratePFR"))
        {
            ghb_ui_update(ud, "VideoFramerateCFR", ghb_boolean_value(TRUE));
        }
    }
    if (ghb_settings_video_framerate_rate(ud->settings, "VideoFramerate") == 0 &&
        ghb_dict_get_bool(ud->settings, "VideoFrameratePFR"))
    {
        ghb_ui_update(ud, "VideoFramerateVFR", ghb_boolean_value(TRUE));
    }
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
}

G_MODULE_EXPORT void
framerate_mode_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_update_summary_info(ud);
    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
}
