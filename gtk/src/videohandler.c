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

#include <glib/gi18n.h>
#include "ghbcompat.h"
#include <string.h>
#include "settings.h"
#include "values.h"
#include "callbacks.h"
#include "presets.h"
#include "preview.h"
#include "hb-backend.h"
#include "x264handler.h"

int ghb_get_video_encoder(GhbValue *settings)
{
    const char *encoder;
    encoder = ghb_dict_get_string(settings, "VideoEncoder");
    return hb_video_encoder_get_from_name(encoder);
}

G_MODULE_EXPORT void
vcodec_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    float val, vqmin, vqmax, step, page;
    int inverted, digits;

    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
    ghb_show_container_options(ud);
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

    // Advanced options are only for x264
    if (!(encoder & HB_VCODEC_X264_MASK))
    {
        ghb_ui_update(ud, "x264UseAdvancedOptions", ghb_boolean_value(FALSE));
    }
}

char *video_option_tooltip = NULL;

void
ghb_video_setting_changed(GtkWidget *widget, signal_user_data_t *ud)
{
    if (video_option_tooltip == NULL)
    {
        GtkWidget *eo = GTK_WIDGET(GHB_WIDGET(ud->builder, "VideoOptionExtra"));
        video_option_tooltip = gtk_widget_get_tooltip_text(eo);
    }

    ghb_widget_to_setting(ud->settings, widget);

    int encoder = ghb_get_video_encoder(ud->settings);
    int presetIndex = ghb_dict_get_int(ud->settings, "VideoPresetSlider");
    const char * const *video_presets;
    const char *preset;
    video_presets = hb_video_encoder_get_presets(encoder);
    if (video_presets != NULL)
    {
        preset = video_presets[presetIndex];
        ghb_dict_set_string(ud->settings, "VideoPreset", preset);
    }

    if (!ghb_dict_get_bool(ud->settings, "x264UseAdvancedOptions") &&
        (encoder & HB_VCODEC_X264_MASK))
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
            if (!ghb_dict_get_bool(ud->settings, "autoscale"))
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
        if (new_opts)
            ghb_update_x264Option(ud, new_opts);
        else
            ghb_update_x264Option(ud, "");

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
    else if (ghb_dict_get_bool(ud->settings, "x264UseAdvancedOptions"))
    {
        const char *opts = ghb_dict_get_string(ud->settings, "x264Option");

        GtkWidget *eo = GTK_WIDGET(GHB_WIDGET(ud->builder, "VideoOptionExtra"));
        char * tt;
        if (opts)
            tt = g_strdup_printf(_("%s\n\nExpanded Options:\n\"%s\""),
                                 video_option_tooltip, opts);
        else
            tt = g_strdup_printf(_("%s\n\nExpanded Options:\n\"\""),
                                 video_option_tooltip);
        gtk_widget_set_tooltip_text(eo, tt);
        g_free(tt);
    }

    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
video_setting_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_video_setting_changed(widget, ud);
}

G_MODULE_EXPORT void
x264_use_advanced_options_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);

    if (ghb_dict_get_bool(ud->prefs, "HideAdvancedVideoSettings") &&
        ghb_dict_get_bool(ud->settings, "x264UseAdvancedOptions"))
    {
        ghb_ui_update(ud, "x264UseAdvancedOptions", ghb_boolean_value(FALSE));
        return;
    }

    if (ghb_dict_get_bool(ud->settings, "x264UseAdvancedOptions"))
    {
        ghb_ui_update(ud, "VideoPresetSlider", ghb_int_value(5));
        ghb_ui_update(ud, "VideoTune", ghb_string_value("none"));
        ghb_ui_update(ud, "VideoProfile", ghb_string_value("auto"));
        ghb_ui_update(ud, "VideoLevel", ghb_string_value("auto"));

        const char *options = ghb_dict_get_string(ud->settings, "x264Option");
        ghb_ui_update(ud, "VideoOptionExtra", ghb_string_value(options));
    }

    ghb_check_dependency(ud, widget, NULL);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
video_option_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GtkWidget *textview;

    textview = GTK_WIDGET(GHB_WIDGET(ud->builder, "VideoOptionExtra"));
    ghb_video_setting_changed(textview, ud);
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
        preset = video_presets[(int)val];
        return g_strdup_printf(" %-12s", preset);
    }
    return g_strdup_printf(" %-12s", "ERROR");
}

