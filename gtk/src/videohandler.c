/* videohandler.c
 *
 * Copyright (C) 2008-2024 John Stebbins <stebbins@stebbins>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "videohandler.h"

#include "application.h"
#include "callbacks.h"
#include "hb-backend.h"
#include "presets.h"
#include "preview.h"

#include <string.h>

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
vcodec_changed_cb (GtkWidget *widget, gpointer data)
{
    float val, vqmin, vqmax, step, page;
    int inverted, digits;
    signal_user_data_t *ud = ghb_ud();

    ghb_widget_to_setting(ud->settings, widget);
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
    ghb_ui_update("VideoTune", ghb_int_value(0));
    ghb_ui_update("VideoProfile", ghb_int_value(0));
    ghb_ui_update("VideoLevel", ghb_int_value(0));
    ghb_ui_update("VideoOptionExtra", ghb_string_value(""));

    // Set the range of the preset slider
    int encoder = ghb_get_video_encoder(ud->settings);
    GtkWidget *presetSlider = ghb_builder_widget("VideoPresetSlider");
    const char * const *video_presets;
    int count = 0;
    video_presets = hb_video_encoder_get_presets(encoder);
    while (video_presets && video_presets[count]) count++;
    gtk_widget_set_visible(presetSlider, count > 0);
    if (count)
    {
        gtk_range_set_range(GTK_RANGE(presetSlider), 0, count-1);
        gtk_scale_clear_marks(GTK_SCALE(presetSlider));
        for (double i = 0; i < count; i += 1)
        {
            gtk_scale_add_mark(GTK_SCALE(presetSlider), i, GTK_POS_BOTTOM, NULL);
        }
    }
    ghb_set_video_preset(ud->settings, encoder, NULL);
    GhbValue *gval = ghb_dict_get_value(ud->settings, "VideoPresetSlider");
    ghb_ui_settings_update(ud, ud->settings, "VideoPresetSlider", gval);
    if (ghb_check_name_template(ud, "{bit-depth}") ||
        ghb_check_name_template(ud, "{codec}"))
        ghb_set_destination(ud);
}

static void
update_adv_settings_tooltip(signal_user_data_t *ud)
{
    GtkWidget *eo = GTK_WIDGET(ghb_builder_widget("VideoOptionExtra"));
    const char *tooltip_msg = _("Additional advanced arguments that can be passed to the video encoder.");

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

        char *tt = g_strdup_printf("%s\n%s\n<b>%s</b>", tooltip_msg,
                                   _("The full list of encoder parameters:"),
                                   new_opts ? new_opts : "--");
        gtk_widget_set_tooltip_markup(eo, tt);

        g_free(tt);
        g_free(new_opts);

        g_free(tunes);
    }
    else
    {
        gtk_widget_set_tooltip_text(eo, tooltip_msg);
    }
}

G_MODULE_EXPORT void
video_preset_slider_changed_cb (GtkWidget *widget, gpointer data)
{
    signal_user_data_t *ud = ghb_ud();
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

    ghb_clear_presets_selection(ud);
    update_adv_settings_tooltip(ud);
}

G_MODULE_EXPORT void
video_setting_changed_cb (GtkWidget *widget, gpointer data)
{
    signal_user_data_t *ud = ghb_ud();
    ghb_widget_to_setting(ud->settings, widget);
    update_adv_settings_tooltip(ud);

    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
video_option_changed_cb (GtkWidget *widget, gpointer data)
{
    GtkWidget *textview;

    textview = ghb_builder_widget("VideoOptionExtra");
    video_setting_changed_cb(textview, data);
}

G_MODULE_EXPORT char *
format_video_preset_cb (double val)
{
    const char * const *video_presets;
    const char *preset;
    signal_user_data_t *ud = ghb_ud();
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
        if (ival == 0 || ival == count - 1)
            return g_strdup_printf("%-20s", preset);
        else
            return g_strdup_printf("%s", preset);
    }
    return g_strdup_printf(" %-12s", "ERROR");
}

G_MODULE_EXPORT char *
format_vquality_cb (double val)
{
    gint vcodec;
    const char *vqname;
    char * result;
    signal_user_data_t *ud = ghb_ud();

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
            result = g_strdup_printf("<b>%s</b>    %d", vqname, (int)val);
        } break;

        case HB_VCODEC_X264_8BIT:
        {
            if (val == 0.0)
            {
                result = g_strdup_printf("<b>%s</b>    %.4g    (%s)",
                                       vqname, val, _("Warning: lossless"));
                break;
            }
            G_GNUC_FALLTHROUGH; // Falls through to default
        }
        case HB_VCODEC_X264_10BIT:
        default:
        {
                result = g_strdup_printf("<b>%s</b>    %.4g", vqname, val);
        } break;
    }

    return result;
}

G_MODULE_EXPORT void
framerate_changed_cb (GtkWidget *widget, gpointer data)
{
    signal_user_data_t *ud = ghb_ud();
    ghb_widget_to_setting(ud->settings, widget);
    ghb_update_summary_info(ud);

    if (ghb_settings_video_framerate_rate(ud->settings, "VideoFramerate") != 0)
    {
        if (!ghb_dict_get_bool(ud->settings, "VideoFrameratePFR"))
        {
            ghb_ui_update("VideoFramerateCFR", ghb_boolean_value(TRUE));
        }
    }
    if (ghb_settings_video_framerate_rate(ud->settings, "VideoFramerate") == 0 &&
        ghb_dict_get_bool(ud->settings, "VideoFrameratePFR"))
    {
        ghb_ui_update("VideoFramerateVFR", ghb_boolean_value(TRUE));
    }
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
}

G_MODULE_EXPORT void
framerate_mode_changed_cb (GtkWidget *widget, gpointer data)
{
    signal_user_data_t *ud = ghb_ud();
    ghb_widget_to_setting(ud->settings, widget);
    ghb_update_summary_info(ud);
    ghb_clear_presets_selection(ud);
    ghb_live_reset(ud);
}
