/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * audiohandler.c
 * Copyright (C) John Stebbins 2008-2015 <stebbins@stebbins>
 *
 * audiohandler.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <glib/gi18n.h>
#include "ghbcompat.h"
#include "hb.h"
#include "settings.h"
#include "hb-backend.h"
#include "values.h"
#include "callbacks.h"
#include "preview.h"
#include "audiohandler.h"
#include "presets.h"

static void audio_add_to_settings(GhbValue *settings, GhbValue *asettings);
static void ghb_add_audio_to_ui(signal_user_data_t *ud, const GhbValue *settings);
static GhbValue* audio_get_selected_settings(signal_user_data_t *ud, int *index);
static void ghb_clear_audio_list_settings(GhbValue *settings);
static void ghb_clear_audio_list_ui(GtkBuilder *builder);

static gboolean block_updates = FALSE;

static void
ghb_sanitize_audio_settings(GhbValue *settings, GhbValue *asettings)
{
    int title_id, titleindex;
    const hb_title_t *title;
    gint track, acodec, mix;
    double quality;
    hb_audio_config_t *aconfig;
    gint bitrate;
    gint sr;
    GhbValue *val;
    gboolean qe;

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    title_id = ghb_dict_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);

    track   = ghb_dict_get_int(asettings, "Track");
    acodec  = ghb_settings_audio_encoder_codec(asettings, "Encoder");
    mix     = ghb_settings_mixdown_mix(asettings, "Mixdown");
    bitrate = ghb_dict_get_int(asettings, "Bitrate");
    quality = ghb_dict_get_double(asettings, "Quality");
    sr      = ghb_dict_get_int(asettings, "Samplerate");
    val     = ghb_dict_get(asettings, "Description");
    if (val == NULL)
    {
        int                title_id, track;
        const hb_title_t  *title;
        hb_audio_config_t *aconfig;

        title_id = ghb_dict_get_int(settings, "title");
        title    = ghb_lookup_title(title_id, NULL);
        track    = ghb_dict_get_int(asettings, "Track");
        aconfig  = ghb_get_audio_info(title, track);
        if (aconfig != NULL)
        {
            char *desc;
            desc = g_strdup_printf("%d - %s", track + 1,
                                   aconfig->lang.description);
            ghb_dict_set_string(asettings, "Description", desc);
            g_free(desc);
        }
    }
    val     = ghb_dict_get(asettings, "QualityEnable");
    if (val == NULL)
    {
        val = ghb_dict_get(asettings, "Quality");
        if (bitrate > 0 || val == NULL || quality == HB_INVALID_AUDIO_QUALITY)
        {
            ghb_dict_set_bool(asettings, "QualityEnable", 0);
        }
        else
        {
            ghb_dict_set_bool(asettings, "QualityEnable", 1);
        }
    }
    qe = ghb_dict_get_bool(asettings, "QualityEnable");

    aconfig = ghb_get_audio_info(title, track);
    if (sr == 0 && aconfig != NULL)
    {
        sr = aconfig->in.samplerate;
    }

    int fallback, copy_mask;
    fallback  = ghb_select_fallback(settings, acodec);
    copy_mask = ghb_get_copy_mask(settings);
    acodec    = ghb_select_audio_codec(mux->format, aconfig, acodec,
                                       fallback, copy_mask);
    if (ghb_audio_is_passthru(acodec))
    {
        mix = HB_AMIXDOWN_NONE;
        ghb_dict_set_string(asettings, "Mixdown",
                            hb_mixdown_get_short_name(mix));
        ghb_dict_set(asettings, "Samplerate", ghb_int_value_new(0));
        ghb_dict_set_double(asettings, "DRC", 0.0);
    }
    else
    {
        if (mix == HB_AMIXDOWN_NONE)
            mix = ghb_get_best_mix(aconfig, acodec, mix);
        if (qe)
        {
            float low, high, gran;
            int dir;
            hb_audio_quality_get_limits(acodec, &low, &high, &gran, &dir);
            if (quality < low || quality > high)
            {
                quality = hb_audio_quality_get_default(acodec);
            }
            else
            {
                quality = hb_audio_quality_get_best(acodec, quality);
            }
        }
        else
        {
            if (bitrate != -1)
            {
                bitrate = hb_audio_bitrate_get_best(acodec, bitrate, sr, mix);
            }
            else
            {
                bitrate = hb_audio_bitrate_get_default(acodec, sr, mix);
            }
        }
        ghb_dict_set_string(asettings, "Mixdown",
                            hb_mixdown_get_short_name(mix));
    }
    if (qe)
    {
        bitrate = -1;
    }
    else
    {
        quality = HB_INVALID_AUDIO_QUALITY;
    }
    ghb_dict_set_int(asettings, "Quality", quality);
    ghb_dict_set_int(asettings, "Bitrate", bitrate);
    ghb_dict_set_string(asettings, "Encoder",
                        hb_audio_encoder_get_short_name(acodec));
}

static gdouble get_quality(int codec, gdouble quality)
{
    float low, high, gran;
    int dir;
    hb_audio_quality_get_limits(codec, &low, &high, &gran, &dir);
    if (dir)
    {
        // Quality values are inverted
        quality = high - quality + low;
    }
    return quality;
}

static gdouble get_ui_quality(GhbValue *settings)
{
    int codec = ghb_settings_audio_encoder_codec(settings, "Encoder");
    gdouble quality = ghb_dict_get_double(settings, "Quality");
    return get_quality(codec, quality);
}

static void
ghb_adjust_audio_rate_combos(signal_user_data_t *ud, GhbValue *asettings)
{
    if (asettings != NULL)
    {
        ghb_sanitize_audio_settings(ud->settings, asettings);

        int track, title_id, mix, acodec;
        const hb_title_t *title;
        hb_audio_config_t *aconfig;

        title_id = ghb_dict_get_int(ud->settings, "title");
        title = ghb_lookup_title(title_id, NULL);
        track = ghb_dict_get_int(asettings, "Track");
        aconfig = ghb_get_audio_info(title, track);

        acodec = ghb_settings_audio_encoder_codec(asettings, "Encoder");
        mix = ghb_settings_mixdown_mix(asettings, "Mixdown");

        int low, high, sr;
        sr = ghb_dict_get_int(asettings, "Samplerate");
        if (sr == 0 && aconfig != NULL)
        {
            sr = aconfig->in.samplerate;
        }
        mix = ghb_get_best_mix(aconfig, acodec, mix);
        hb_audio_bitrate_get_limits(acodec, sr, mix, &low, &high);

        GtkWidget *w = GHB_WIDGET(ud->builder, "AudioBitrate");
        ghb_audio_bitrate_opts_filter(GTK_COMBO_BOX(w), low, high);

        ghb_ui_update(ud, "AudioEncoder",
                      ghb_dict_get_value(asettings, "Encoder"));
        ghb_ui_update(ud, "AudioBitrate",
                      ghb_dict_get_value(asettings, "Bitrate"));
        gdouble quality = get_ui_quality(asettings);
        ghb_ui_update(ud, "AudioTrackQualityX", ghb_double_value_new(quality));
        ghb_ui_update(ud, "AudioSamplerate",
                      ghb_dict_get_value(asettings, "Samplerate"));
        ghb_ui_update(ud, "AudioMixdown",
                      ghb_dict_get_value(asettings, "Mixdown"));
        ghb_ui_update(ud, "AudioTrackDRCSlider",
                      ghb_dict_get_value(asettings, "DRC"));
        ghb_audio_list_refresh_selected(ud);
    }
}

static void enable_quality_widgets(
    signal_user_data_t *ud,
    gboolean            quality_enable,
    int                 acodec,
    int                 sr,
    int                 mix)
{
    GtkWidget *widget;
    gboolean quality_sensitive = TRUE;
    gboolean bitrate_sensitive = TRUE;

    widget = GHB_WIDGET(ud->builder, "AudioBitrate");
    gtk_widget_set_visible(widget, !quality_enable);
    widget = GHB_WIDGET(ud->builder, "AudioTrackQualityBox");
    gtk_widget_set_visible(widget, quality_enable);

    if (hb_audio_quality_get_default(acodec) == HB_INVALID_AUDIO_QUALITY)
    {
        quality_sensitive = FALSE;
    }
    if (hb_audio_bitrate_get_default(acodec, sr, mix) == -1)
    {
        bitrate_sensitive = FALSE;
    }

    if (!quality_sensitive)
    {
        widget = GHB_WIDGET(ud->builder, "AudioTrackBitrateEnable");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                                     !quality_sensitive);
    }
    widget = GHB_WIDGET(ud->builder, "AudioTrackQualityEnable");
    gtk_widget_set_sensitive(widget, quality_sensitive);

    widget = GHB_WIDGET(ud->builder, "AudioBitrate");
    gtk_widget_set_sensitive(widget, bitrate_sensitive);
    widget = GHB_WIDGET(ud->builder, "AudioTrackQualityEnableBox");
    gtk_widget_set_sensitive(widget, bitrate_sensitive || quality_sensitive);
    widget = GHB_WIDGET(ud->builder, "AudioTrackQualityBox");
    gtk_widget_set_sensitive(widget, quality_sensitive);
}

static void
audio_deps(signal_user_data_t *ud, GhbValue *asettings, GtkWidget *widget)
{
    ghb_adjust_audio_rate_combos(ud, asettings);
    ghb_grey_combo_options(ud);
    if (widget != NULL)
        ghb_check_dependency(ud, widget, NULL);

    int track = -1, title_id, mix = 0, acodec = 0, sr = 0;
    hb_audio_config_t *aconfig = NULL;
    const hb_title_t *title;
    gboolean qe = FALSE;

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, NULL);

    if (asettings != NULL)
    {
        track = ghb_dict_get_int(asettings, "Track");
        acodec = ghb_settings_audio_encoder_codec(asettings, "Encoder");
        aconfig = ghb_get_audio_info(title, track);
        mix = ghb_settings_mixdown_mix(asettings, "Mixdown");
        sr = ghb_dict_get_int(asettings, "Samplerate");
        qe = ghb_dict_get_bool(asettings, "QualityEnable");
        if (sr == 0 && aconfig != NULL)
        {
            sr = aconfig->in.samplerate;
        }
    }

    gboolean is_passthru = (acodec & HB_ACODEC_PASS_FLAG);
    gboolean enable_drc = TRUE;
    if (aconfig != NULL)
    {
        enable_drc = hb_audio_can_apply_drc(aconfig->in.codec,
                                            aconfig->in.codec_param, acodec) &&
                     !is_passthru;
    }

    widget = GHB_WIDGET(ud->builder, "AudioTrackDRCSlider");
    gtk_widget_set_sensitive(widget, enable_drc);
    widget = GHB_WIDGET(ud->builder, "AudioTrackDRCSliderLabel");
    gtk_widget_set_sensitive(widget, enable_drc);
    widget = GHB_WIDGET(ud->builder, "AudioTrackDRCValue");
    gtk_widget_set_sensitive(widget, enable_drc);

    enable_quality_widgets(ud, qe, acodec, sr, mix);

    widget = GHB_WIDGET(ud->builder, "AudioMixdown");
    gtk_widget_set_sensitive(widget, !is_passthru);
    widget = GHB_WIDGET(ud->builder, "AudioSamplerate");
    gtk_widget_set_sensitive(widget, !is_passthru);
    widget = GHB_WIDGET(ud->builder, "AudioTrackGainSlider");
    gtk_widget_set_sensitive(widget, !is_passthru);
    widget = GHB_WIDGET(ud->builder, "AudioTrackGainValue");
    gtk_widget_set_sensitive(widget, !is_passthru);
}

gint
ghb_select_audio_codec(gint mux, hb_audio_config_t *aconfig, gint acodec, gint fallback, gint copy_mask)
{
    guint32 in_codec = aconfig != NULL ? aconfig->in.codec : 0;

    if (acodec == HB_ACODEC_AUTO_PASS)
    {
        return hb_autopassthru_get_encoder(in_codec, copy_mask, fallback, mux);
    }

    // Sanitize fallback
    const hb_encoder_t *enc;
    for (enc = hb_audio_encoder_get_next(NULL); enc != NULL;
         enc = hb_audio_encoder_get_next(enc))
    {
        if (enc->codec == fallback &&
            !(enc->muxers & mux))
        {
            if ( mux & HB_MUX_MASK_MKV )
                fallback = HB_ACODEC_LAME;
            else
                fallback = HB_ACODEC_FFAAC;
            break;
        }
    }
    if ((acodec & HB_ACODEC_PASS_FLAG) &&
        !(acodec & in_codec & HB_ACODEC_PASS_MASK))
    {
        return fallback;
    }
    for (enc = hb_audio_encoder_get_next(NULL); enc != NULL;
         enc = hb_audio_encoder_get_next(enc))
    {
        if (enc->codec == acodec &&
            !(enc->muxers & mux))
        {
            return fallback;
        }
    }
    return acodec;
}

int ghb_get_copy_mask(GhbValue *settings)
{
    gint mask = 0;

    if (ghb_dict_get_bool(settings, "AudioAllowMP3Pass"))
    {
        mask |= HB_ACODEC_MP3_PASS;
    }
    if (ghb_dict_get_bool(settings, "AudioAllowAACPass"))
    {
        mask |= HB_ACODEC_AAC_PASS;
    }
    if (ghb_dict_get_bool(settings, "AudioAllowAC3Pass"))
    {
        mask |= HB_ACODEC_AC3_PASS;
    }
    if (ghb_dict_get_bool(settings, "AudioAllowDTSPass"))
    {
        mask |= HB_ACODEC_DCA_PASS;
    }
    if (ghb_dict_get_bool(settings, "AudioAllowDTSHDPass"))
    {
        mask |= HB_ACODEC_DCA_HD_PASS;
    }
    if (ghb_dict_get_bool(settings, "AudioAllowEAC3Pass"))
    {
        mask |= HB_ACODEC_EAC3_PASS;
    }
    if (ghb_dict_get_bool(settings, "AudioAllowFLACPass"))
    {
        mask |= HB_ACODEC_FLAC_PASS;
    }
    if (ghb_dict_get_bool(settings, "AudioAllowTRUEHDPass"))
    {
        mask |= HB_ACODEC_TRUEHD_PASS;
    }
    return mask;
}

int ghb_select_fallback(GhbValue *settings, int acodec)
{
    gint fallback = 0;

    if (acodec & HB_ACODEC_PASS_FLAG)
    {
        fallback = hb_audio_encoder_get_fallback_for_passthru(acodec);
        if (fallback != 0)
        {
            return fallback;
        }
    }
    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    fallback = ghb_settings_audio_encoder_codec(settings,
                                                "AudioEncoderFallback");
    return hb_autopassthru_get_encoder(acodec, 0, fallback, mux->format);
}

static char * get_quality_string(int codec, gdouble quality)
{
    char *s_quality = ghb_format_quality("", codec, quality);
    return s_quality;
}

void ghb_sanitize_audio_tracks(signal_user_data_t *ud)
{
    int ii;
    GhbValue *alist = ghb_get_job_audio_list(ud->settings);
    int count = ghb_array_len(alist);

    for (ii = 0; ii < count; ii++)
    {
        GhbValue *asettings = ghb_array_get(alist, ii);
        ghb_sanitize_audio_settings(ud->settings, asettings);
    }

    GhbValue *asettings = audio_get_selected_settings(ud, NULL);
    if (asettings != NULL)
    {
        ghb_ui_update(ud, "AudioEncoder",
                      ghb_dict_get_value(asettings, "Encoder"));
        ghb_ui_update(ud, "AudioBitrate",
                      ghb_dict_get_value(asettings, "Bitrate"));
        gdouble quality = get_ui_quality(asettings);
        ghb_ui_update(ud, "AudioTrackQualityX", ghb_double_value_new(quality));
        ghb_ui_update(ud, "AudioSamplerate",
                      ghb_dict_get_value(asettings, "Samplerate"));
        ghb_ui_update(ud, "AudioMixdown",
                      ghb_dict_get_value(asettings, "Mixdown"));
        ghb_ui_update(ud, "AudioTrackDRCSlider",
                      ghb_dict_get_value(asettings, "DRC"));
    }
}

static char * get_drc_string(gdouble drc)
{
    char *s_drc;
    if (drc < 0.99)
        s_drc = g_strdup(_("Off"));
    else
        s_drc = g_strdup_printf("%.1f", drc);
    return s_drc;
}

static char * get_gain_string(gdouble gain)
{
    char *s_gain;
    if ( gain >= 21.0 )
        s_gain = g_strdup_printf("*11*");
    else
        s_gain = g_strdup_printf(_("%ddB"), (int)gain);
    return s_gain;
}

static void
audio_update_dialog_widgets(signal_user_data_t *ud, GhbValue *asettings)
{
    if (asettings != NULL)
    {
        double gain, drc, quality, qualityx;
        char *s_gain, *s_drc, *s_quality;
        gboolean qe;

        block_updates = TRUE;
        ghb_ui_update(ud, "AudioTrack",
                      ghb_dict_get_value(asettings, "Track"));
        ghb_ui_update(ud, "AudioEncoder",
                      ghb_dict_get_value(asettings, "Encoder"));
        ghb_ui_update(ud, "AudioBitrate",
                      ghb_dict_get_value(asettings, "Bitrate"));
        GhbValue *val = ghb_dict_get_value(asettings, "Name");
        if (val != NULL)
        {
            ghb_ui_update(ud, "AudioTrackName", val);
        }
        else
        {
            ghb_ui_update(ud, "AudioTrackName", ghb_string_value(""));
        }
        ghb_ui_update(ud, "AudioSamplerate",
                      ghb_dict_get_value(asettings, "Samplerate"));
        ghb_ui_update(ud, "AudioMixdown",
                      ghb_dict_get_value(asettings, "Mixdown"));
        ghb_ui_update(ud, "AudioTrackDRCSlider",
                      ghb_dict_get_value(asettings, "DRC"));
        drc = ghb_dict_get_double(asettings, "DRC");
        s_drc = get_drc_string(drc);
        ghb_ui_update(ud, "AudioTrackDRCValue", ghb_string_value(s_drc));
        ghb_ui_update(ud, "AudioTrackGainSlider",
                      ghb_dict_get_value(asettings, "Gain"));
        gain = ghb_dict_get_double(asettings, "Gain");
        s_gain = get_gain_string(gain);
        ghb_ui_update(ud, "AudioTrackGainValue", ghb_string_value(s_gain));

        int codec = ghb_settings_audio_encoder_codec(asettings, "Encoder");
        quality = ghb_dict_get_double(asettings, "Quality");
        qualityx = get_quality(codec, quality);
        ghb_ui_update(ud, "AudioTrackQualityX", ghb_double_value_new(qualityx));
        s_quality = get_quality_string(codec, quality);
        ghb_ui_update(ud, "AudioTrackQualityValue", ghb_string_value(s_quality));
        // Setting a radio button to FALSE does not automatically make
        // the other one TRUE
        qe = ghb_dict_get_bool(asettings, "QualityEnable");
        if (qe)
        {
            ghb_ui_update(ud, "AudioTrackQualityEnable",
                          ghb_bool_value_new(qe));
        }
        else
        {
            ghb_ui_update(ud, "AudioTrackBitrateEnable",
                          ghb_bool_value_new(!qe));
        }
        block_updates = FALSE;
    }
    audio_deps(ud, asettings, NULL);
}

const gchar*
ghb_get_user_audio_lang(GhbValue *settings, const hb_title_t *title, gint track)
{
    GhbValue *audio_list, *asettings;
    const gchar *lang;

    audio_list = ghb_get_job_audio_list(settings);
    if (ghb_array_len(audio_list) <= track)
        return "und";
    asettings = ghb_array_get(audio_list, track);
    track = ghb_dict_get_int(asettings, "Track");
    lang = ghb_get_source_audio_lang(title, track);
    return lang;
}

static GhbValue*
audio_add_track(
    GhbValue *settings,
    const hb_title_t *title,
    int track,
    int encoder,
    gboolean enable_quality,
    gdouble quality,
    int bitrate,
    int samplerate,
    int mix,
    gdouble drc,
    gdouble gain)
{
    GhbValue *asettings;
    hb_audio_config_t *aconfig = NULL;

    aconfig = ghb_get_audio_info(title, track);
    asettings = ghb_dict_new();

    ghb_dict_set_int(asettings, "Track", track);
    ghb_dict_set_string(asettings, "Encoder",
                        hb_audio_encoder_get_short_name(encoder));
    ghb_dict_set_bool(asettings, "QualityEnable", enable_quality);
    ghb_dict_set_double(asettings, "Quality", quality);
    if (enable_quality)
    {
        bitrate = -1;
    }
    ghb_dict_set_int(asettings, "Bitrate", bitrate);
    ghb_dict_set_int(asettings, "Samplerate", samplerate);

    if (aconfig != NULL)
    {
        mix = ghb_get_best_mix(aconfig, encoder, mix);
    }
    ghb_dict_set_string(asettings, "Mixdown", hb_mixdown_get_short_name(mix));
    ghb_dict_set_double(asettings, "DRC", drc);
    ghb_dict_set_double(asettings, "Gain", gain);

    ghb_sanitize_audio_settings(settings, asettings);
    audio_add_to_settings(settings, asettings);

    return asettings;
}

static GhbValue*
audio_select_and_add_track(
    const hb_title_t *title,
    GhbValue *settings,
    GhbValue *pref_audio,
    const char *lang,
    int pref_index,
    int start_track)
{
    GhbValue *audio, *asettings = NULL;
    gdouble drc, gain, quality;
    gboolean enable_quality;
    gint track, acodec, bitrate, samplerate, mix;

    gint select_acodec;
    gint fallback;

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    gint copy_mask = ghb_get_copy_mask(settings);

    audio = ghb_array_get(pref_audio, pref_index);

    acodec = ghb_settings_audio_encoder_codec(audio, "AudioEncoder");
    fallback = ghb_select_fallback(settings, acodec);

    bitrate = ghb_settings_audio_bitrate_rate(audio, "AudioBitrate");
    samplerate = ghb_settings_audio_samplerate_rate(audio, "AudioSamplerate");
    mix = ghb_settings_mixdown_mix(audio, "AudioMixdown");
    drc = ghb_dict_get_double(audio, "AudioTrackDRCSlider");
    gain = ghb_dict_get_double(audio, "AudioTrackGainSlider");
    enable_quality = ghb_dict_get_bool(audio, "AudioTrackQualityEnable");
    quality = ghb_dict_get_double(audio, "AudioTrackQuality");

    track = ghb_find_audio_track(title, lang, start_track);
    if (track >= 0)
    {
        // Check to see if:
        // 1. pref codec is passthru
        // 2. source codec is not passthru
        // 3. next pref is enabled
        hb_audio_config_t *aconfig;
        aconfig = hb_list_audio_config_item(title->list_audio, track);
        select_acodec = ghb_select_audio_codec(
                            mux->format, aconfig, acodec, fallback, copy_mask);

        asettings = audio_add_track(settings, title, track, select_acodec,
                                    enable_quality, quality, bitrate,
                                    samplerate, mix, drc, gain);
    }
    return asettings;
}

void ghb_audio_title_change(signal_user_data_t *ud, gboolean title_valid)
{
    GtkWidget *w = GHB_WIDGET(ud->builder, "audio_add");
    gtk_widget_set_sensitive(w, title_valid);
    w = GHB_WIDGET(ud->builder, "audio_add_all");
    gtk_widget_set_sensitive(w, title_valid);
    w = GHB_WIDGET(ud->builder, "audio_reset");
    gtk_widget_set_sensitive(w, title_valid);
}

void
ghb_set_pref_audio_settings(GhbValue *settings)
{
    int       title_id;
    GhbValue *copy_mask;

    copy_mask = ghb_create_copy_mask(settings);
    ghb_dict_set(settings, "AudioCopyMask", copy_mask);
    title_id = ghb_dict_get_int(settings, "title");
    GhbValue *job = ghb_get_job_settings(settings);
    ghb_dict_remove(job, "Audio");
    hb_preset_job_add_audio(ghb_scan_handle(), title_id, settings, job);
}

static GhbValue*
audio_get_selected_settings(signal_user_data_t *ud, int *index)
{
    GtkTreeView *tv;
    GtkTreePath *tp;
    GtkTreeSelection *ts;
    GtkTreeModel *tm;
    GtkTreeIter ti;
    gint *indices;
    gint row;
    GhbValue *asettings = NULL;
    const GhbValue *audio_list;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list_view"));
    ts = gtk_tree_view_get_selection (tv);
    if (gtk_tree_selection_get_selected(ts, &tm, &ti))
    {
        // Get the row number
        tp = gtk_tree_model_get_path (tm, &ti);
        indices = gtk_tree_path_get_indices (tp);
        row = indices[0];
        gtk_tree_path_free(tp);
        // find audio settings
        if (row < 0) return NULL;

        audio_list = ghb_get_job_audio_list(ud->settings);
        if (row >= ghb_array_len(audio_list))
            return NULL;

        asettings = ghb_array_get(audio_list, row);
        if (index != NULL)
            *index = row;
    }
    return asettings;
}

static void
audio_refresh_list_row_ui(
    GtkTreeModel *tm,
    GtkTreeIter *ti,
    signal_user_data_t *ud,
    const GhbValue *settings)
{
    GtkTreeIter cti;
    char *info_src, *info_src_2;
    char *info_dst, *info_dst_2;


    info_src_2 = NULL;
    info_dst_2 = NULL;

    const gchar *s_track, *s_track_name;
    gchar *s_drc, *s_gain, *s_br_quality, *s_sr;
    gdouble drc, gain;
    hb_audio_config_t *aconfig;
    int track, sr;
    int title_id, titleindex;
    const hb_title_t *title;
    const hb_encoder_t *encoder;

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    track = ghb_dict_get_int(settings, "Track");
    aconfig = ghb_get_audio_info(title, track);
    if (aconfig == NULL)
    {
        return;
    }


    s_track = aconfig->lang.description;
    encoder = ghb_settings_audio_encoder(settings, "Encoder");

    double quality = ghb_dict_get_double(settings, "Quality");
    int    bitrate = ghb_dict_get_int(settings, "Bitrate");
    if (bitrate > 0)
    {
        s_br_quality = g_strdup_printf(_("Bitrate: %dkbps\n"), bitrate);
    }
    else if (quality != HB_INVALID_AUDIO_QUALITY)
    {
        char *tmp = ghb_format_quality(_("Quality: "),
                                       encoder->codec, quality);
        s_br_quality = g_strdup_printf("%s\n", tmp);
        g_free(tmp);
    }
    else
    {
        s_br_quality = g_strdup("");
    }

    sr = ghb_dict_get_int(settings, "Samplerate");
    if (sr == 0)
    {
        sr = aconfig->in.samplerate;
    }
    s_sr = g_strdup_printf(_("%.4gkHz"), (double)sr/1000);

    const hb_mixdown_t *mix;
    mix = ghb_settings_mixdown(settings, "Mixdown");
    gain = ghb_dict_get_double(settings, "Gain");
    s_gain = g_strdup_printf(_("%ddB"), (int)gain);

    drc = ghb_dict_get_double(settings, "DRC");
    if (drc < 1.0)
        s_drc = g_strdup(_("Off"));
    else
        s_drc = g_strdup_printf("%.1f", drc);

    s_track_name = ghb_dict_get_string(settings, "Name");

    info_src = g_strdup_printf(_("<small>%d - %s (%.4gkHz)</small>"),
        track + 1, s_track, (double)aconfig->in.samplerate / 1000);
    if (aconfig->in.bitrate > 0)
    {
        info_src_2 = g_strdup_printf(
            _("Bitrate: %.4gkbps"),
            (double)aconfig->in.bitrate / 1000);
    }

    if (ghb_audio_is_passthru(encoder->codec))
    {
        info_dst = g_strdup_printf(_("<small>Passthrough</small>"));
    }
    else
    {
        info_dst = g_strdup_printf("<small>%s (%s) (%s)</small>",
                                   encoder->name, mix->name, s_sr);
        if (s_track_name && s_track_name[0])
        {
            info_dst_2 = g_strdup_printf(
                _("%sGain: %s\nDRC: %s\nTrack Name: %s"),
                s_br_quality, s_gain, s_drc, s_track_name);
        }
        else
        {
            info_dst_2 = g_strdup_printf(_("%sGain: %s\nDRC: %s"),
                                            s_br_quality, s_gain, s_drc);
        }
    }
    gtk_tree_store_set(GTK_TREE_STORE(tm), ti,
        // These are displayed in list
        0, info_src,
        1, "-->",
        2, info_dst,
        3, "hb-edit",
        4, "hb-remove",
        5, 0.5,
        -1);

    if (info_src_2 != NULL || info_dst_2 != NULL)
    {
        if (info_src_2 == NULL)
            info_src_2 = g_strdup("");
        if (info_dst_2 == NULL)
            info_dst_2 = g_strdup("");

        // Get the child of the selection
        if (!gtk_tree_model_iter_children(tm, &cti, ti))
        {
            gtk_tree_store_append(GTK_TREE_STORE(tm), &cti, ti);
        }
        gtk_tree_store_set(GTK_TREE_STORE(tm), &cti,
            // These are displayed in list
            0, info_src_2,
            2, info_dst_2,
            5, 0.0,
            -1);
    }
    else
    {
        if(gtk_tree_model_iter_children(tm, &cti, ti))
        {
            gtk_tree_store_remove(GTK_TREE_STORE(tm), &cti);
        }
    }

    g_free(info_src);
    g_free(info_src_2);
    g_free(info_dst);
    g_free(info_dst_2);

    g_free(s_sr);
    g_free(s_drc);
    g_free(s_gain);
    g_free(s_br_quality);
}

void
ghb_audio_list_refresh_selected(signal_user_data_t *ud)
{
    GtkTreeView *tv;
    GtkTreePath *tp;
    GtkTreeSelection *ts;
    GtkTreeModel *tm;
    GtkTreeIter ti;
    gint *indices;
    gint row;
    GhbValue *asettings = NULL;
    const GhbValue *audio_list;

    g_debug("ghb_audio_list_refresh_selected ()");
    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list_view"));
    ts = gtk_tree_view_get_selection (tv);
    if (gtk_tree_selection_get_selected(ts, &tm, &ti))
    {
        // Get the row number
        tp = gtk_tree_model_get_path (tm, &ti);
        indices = gtk_tree_path_get_indices (tp);
        row = indices[0];
        gtk_tree_path_free(tp);
        if (row < 0) return;

        audio_list = ghb_get_job_audio_list(ud->settings);
        if (row >= ghb_array_len(audio_list))
            return;

        asettings = ghb_array_get(audio_list, row);
        audio_refresh_list_row_ui(tm, &ti, ud, asettings);
    }
}

static void
audio_refresh_list_ui(signal_user_data_t *ud)
{
    GhbValue *audio_list;
    GhbValue *asettings;
    gint ii, count, tm_count;
    GtkTreeView *tv;
    GtkTreeModel *tm;
    GtkTreeIter ti;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list_view"));
    tm = gtk_tree_view_get_model(tv);

    tm_count = gtk_tree_model_iter_n_children(tm, NULL);

    audio_list = ghb_get_job_audio_list(ud->settings);
    count = ghb_array_len(audio_list);
    if (count != tm_count)
    {
        ghb_clear_audio_list_ui(ud->builder);
        for (ii = 0; ii < count; ii++)
        {
            gtk_tree_store_append(GTK_TREE_STORE(tm), &ti, NULL);
        }
    }
    for (ii = 0; ii < count; ii++)
    {
        gtk_tree_model_iter_nth_child(tm, &ti, NULL, ii);
        asettings = ghb_array_get(audio_list, ii);
        audio_refresh_list_row_ui(tm, &ti, ud, asettings);
    }
}

void
ghb_audio_list_refresh_all(signal_user_data_t *ud)
{
    ghb_sanitize_audio_tracks(ud);
    audio_refresh_list_ui(ud);
}

G_MODULE_EXPORT void
audio_codec_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    static gint prev_acodec = 0;
    gint acodec;
    GhbValue *asettings;

    ghb_widget_to_setting(ud->settings, widget);
    asettings = audio_get_selected_settings(ud, NULL);
    if (!block_updates && asettings != NULL)
    {
        ghb_dict_set(asettings, "Encoder", ghb_widget_value(widget));
        audio_deps(ud, asettings, widget);
        ghb_audio_list_refresh_selected(ud);
        ghb_live_reset(ud);
    }

    acodec = ghb_settings_audio_encoder_codec(ud->settings, "AudioEncoder");

    float low, high, gran, defval;
    int dir;
    hb_audio_quality_get_limits(acodec, &low, &high, &gran, &dir);
    defval = hb_audio_quality_get_default(acodec);
    GtkScaleButton *sb;
    GtkAdjustment *adj;
    sb = GTK_SCALE_BUTTON(GHB_WIDGET(ud->builder, "AudioTrackQualityX"));
    adj = gtk_scale_button_get_adjustment(sb);
    if (dir)
    {
        // Quality values are inverted
        defval = high - defval + low;
    }
    gtk_adjustment_configure(adj, defval, low, high, gran, gran * 10, 0);

    if (block_updates)
    {
        prev_acodec = acodec;
        return;
    }

    if (ghb_audio_is_passthru(prev_acodec) &&
        !ghb_audio_is_passthru(acodec))
    {
        // Transition from passthru to not, put some audio settings back to
        // pref settings
        int title_id;
        const hb_title_t *title;
        gint titleindex;
        gint track;
        gint br, sr, mix;

        title_id = ghb_dict_get_int(ud->settings, "title");
        title = ghb_lookup_title(title_id, &titleindex);

        if (asettings != NULL)
        {
            br = ghb_dict_get_int(asettings, "Bitrate");
            sr = ghb_dict_get_int(asettings, "Samplerate");
            mix = ghb_settings_mixdown_mix(asettings, "Mixdown");
        }
        else
        {
            br = 160;
            sr = 0;
            mix = HB_AMIXDOWN_NONE;
        }

        track = ghb_dict_get_int(ud->settings, "AudioTrack");
        if (sr)
        {
            sr = ghb_find_closest_audio_samplerate(sr);
        }
        ghb_ui_update(ud, "AudioSamplerate",
            ghb_string_value(ghb_audio_samplerate_get_short_name(sr)));

        hb_audio_config_t *aconfig;
        aconfig = ghb_get_audio_info(title, track);
        if (sr == 0)
        {
            sr = aconfig ? aconfig->in.samplerate : 48000;
        }
        mix = ghb_get_best_mix( aconfig, acodec, mix);
        br = hb_audio_bitrate_get_best(acodec, br, sr, mix);
        ghb_ui_update(ud, "AudioBitrate",
            ghb_string_value(ghb_audio_bitrate_get_short_name(br)));

        ghb_ui_update(ud, "AudioMixdown",
                      ghb_string_value(hb_mixdown_get_short_name(mix)));
    }
    prev_acodec = acodec;
}

G_MODULE_EXPORT void
audio_track_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *asettings;

    g_debug("audio_track_changed_cb ()");
    ghb_widget_to_setting(ud->settings, widget);
    if (block_updates)
    {
        return;
    }

    asettings = audio_get_selected_settings(ud, NULL);
    if (asettings != NULL)
    {
        GhbValue *val = ghb_widget_value(widget);
        ghb_dict_set(asettings, "Track", ghb_value_xform(val, GHB_INT));
        ghb_value_free(&val);
        audio_deps(ud, asettings, widget);
        ghb_audio_list_refresh_selected(ud);
        ghb_live_reset(ud);

        // Update the track description used by the queue
        int title_id, titleindex;
        const hb_title_t *title;
        int track;
        hb_audio_config_t *aconfig;

        title_id = ghb_dict_get_int(ud->settings, "title");
        title = ghb_lookup_title(title_id, &titleindex);
        track = ghb_dict_get_int(ud->settings, "AudioTrack");
        aconfig = ghb_get_audio_info(title, track);
        if (aconfig != NULL)
        {
            char *desc;
            desc = g_strdup_printf("%d - %s", track + 1,
                                   aconfig->lang.description);
            ghb_dict_set_string(asettings, "Description", desc);
            g_free(desc);
        }
    }
}

void
audio_update_setting(
    GhbValue           *val,
    const char         *name,
    signal_user_data_t *ud)
{
    GhbValue *asettings;

    if (block_updates)
    {
        return;
    }

    asettings = audio_get_selected_settings(ud, NULL);
    if (asettings != NULL)
    {
        if (val != NULL)
            ghb_dict_set(asettings, name, val);
        else
            ghb_dict_remove(asettings, name);
        audio_deps(ud, asettings, NULL);
        ghb_audio_list_refresh_selected(ud);
        ghb_live_reset(ud);
    }
}

G_MODULE_EXPORT void
audio_mix_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    audio_update_setting(ghb_widget_value(widget), "Mixdown", ud);
}

G_MODULE_EXPORT void
audio_name_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    const char *s = ghb_dict_get_string(ud->settings, "AudioTrackName");
    if (s != NULL && s[0] != 0)
    {
        audio_update_setting(ghb_widget_value(widget), "Name", ud);
    }
    else
    {
        audio_update_setting(NULL, "Name", ud);
    }
}

G_MODULE_EXPORT void
audio_bitrate_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    int br;

    ghb_widget_to_setting(ud->settings, widget);
    br = ghb_settings_audio_bitrate_rate(ud->settings, "AudioBitrate");
    audio_update_setting(ghb_int_value_new(br), "Bitrate", ud);
}

G_MODULE_EXPORT void
audio_samplerate_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    int sr;

    ghb_widget_to_setting(ud->settings, widget);
    sr = ghb_settings_audio_samplerate_rate(ud->settings, "AudioSamplerate");
    audio_update_setting(ghb_int_value_new(sr), "Samplerate", ud);
}

G_MODULE_EXPORT void
audio_quality_radio_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    audio_update_setting(ghb_widget_value(widget), "QualityEnable", ud);
}

G_MODULE_EXPORT void
audio_passthru_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *copy_mask, *audio;

    ghb_widget_to_setting(ud->settings, widget);
    copy_mask = ghb_create_copy_mask(ud->settings);
    audio = ghb_get_job_audio_settings(ud->settings);
    ghb_dict_set(audio, "CopyMask", copy_mask);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT gchar*
format_drc_cb(GtkScale *scale, gdouble val, signal_user_data_t *ud)
{
    if (val < 1.0)
        return g_strdup_printf("%-7s", _("Off"));
    else
        return g_strdup_printf("%-7.1f", val);
}

static inline int is_close(float a, float b, float metric)
{
    float diff = a - b;
    diff = (diff > 0) ? diff : -diff;
    return diff < metric;
}

char * ghb_format_quality( const char *prefix, int codec, double quality )
{
    float low, high, gran;
    int dir;
    hb_audio_quality_get_limits(codec, &low, &high, &gran, &dir);

    int digits = 0;
    float tmp = gran;
    while (1)
    {
        if (is_close(tmp, (int)tmp, gran / 10))
            break;
        digits++;
        tmp *= 10;
    }
    return g_strdup_printf("%s%.*f", prefix, digits, quality);
}

G_MODULE_EXPORT void
quality_widget_changed_cb(GtkWidget *widget, gdouble quality, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);

    int codec = ghb_settings_audio_encoder_codec(ud->settings, "AudioEncoder");
    quality = get_quality(codec, quality);
    char *s_quality = get_quality_string(codec, quality);
    ghb_ui_update( ud, "AudioTrackQualityValue", ghb_string_value(s_quality));
    g_free(s_quality);

    audio_update_setting(ghb_double_value_new(quality), "Quality", ud);
}

G_MODULE_EXPORT void
drc_widget_changed_cb(GtkWidget *widget, gdouble drc, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);

    if (drc < 1.0)
        drc = 0.0;

    char *s_drc = get_drc_string(drc);
    ghb_ui_update( ud, "AudioTrackDRCValue", ghb_string_value(s_drc));
    g_free(s_drc);

    audio_update_setting(ghb_double_value_new(drc), "DRC", ud);
}

G_MODULE_EXPORT gchar*
format_gain_cb(GtkScale *scale, gdouble val, signal_user_data_t *ud)
{
    if ( val >= 21.0 )
        return g_strdup_printf("*11*");
    return g_strdup_printf(_("%ddB"), (int)val);
}

G_MODULE_EXPORT void
gain_widget_changed_cb(GtkWidget *widget, gdouble gain, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);

    char *s_gain = get_gain_string(gain);
    ghb_ui_update( ud, "AudioTrackGainValue", ghb_string_value(s_gain));
    g_free(s_gain);

    audio_update_setting(ghb_double_value_new(gain), "Gain", ud);
}

void
ghb_clear_audio_list_settings(GhbValue *settings)
{
    GhbValue *audio_list;

    g_debug("clear_audio_list_settings ()");
    audio_list = ghb_get_job_audio_list(settings);
    ghb_array_reset(audio_list);
}

void
ghb_clear_audio_selection(GtkBuilder *builder)
{
    GtkTreeView *tv;
    GtkTreeSelection *tsel;

    g_debug("clear_audio_list_ui ()");
    tv = GTK_TREE_VIEW(GHB_WIDGET(builder, "audio_list_view"));
    // Clear tree selection so that updates are not triggered
    // that cause a recursive attempt to clear the tree selection (crasher)
    tsel = gtk_tree_view_get_selection(tv);
    gtk_tree_selection_unselect_all(tsel);
}

void
ghb_clear_audio_list_ui(GtkBuilder *builder)
{
    GtkTreeView *tv;
    GtkTreeStore *ts;
    GtkTreeSelection *tsel;

    g_debug("clear_audio_list_ui ()");
    tv = GTK_TREE_VIEW(GHB_WIDGET(builder, "audio_list_view"));
    ts = GTK_TREE_STORE(gtk_tree_view_get_model(tv));
    // Clear tree selection so that updates are not triggered
    // that cause a recursive attempt to clear the tree selection (crasher)
    tsel = gtk_tree_view_get_selection(tv);
    gtk_tree_selection_unselect_all(tsel);
    gtk_tree_store_clear(ts);
}

static void
ghb_add_audio_to_ui(signal_user_data_t *ud, const GhbValue *asettings)
{
    GtkTreeView *tv;
    GtkTreeIter ti;
    GtkTreeModel *tm;
    GtkTreeSelection *ts;

    if (asettings == NULL)
        return;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list_view"));
    ts = gtk_tree_view_get_selection (tv);
    tm = gtk_tree_view_get_model(tv);

    gtk_tree_store_append(GTK_TREE_STORE(tm), &ti, NULL);
    gtk_tree_selection_select_iter(ts, &ti);

    audio_refresh_list_row_ui(tm, &ti, ud, asettings);
}

G_MODULE_EXPORT void
audio_list_selection_changed_cb(GtkTreeSelection *ts, signal_user_data_t *ud)
{
    GtkTreeModel *tm;
    GtkTreeIter ti;
    GhbValue *asettings = NULL;
    gint row;

    g_debug("audio_list_selection_changed_cb ()");
    if (gtk_tree_selection_get_selected(ts, &tm, &ti))
    {
        GtkTreeIter pti;

        if (gtk_tree_model_iter_parent(tm, &pti, &ti))
        {
            GtkTreePath *path;
            GtkTreeView *tv;

            gtk_tree_selection_select_iter(ts, &pti);
            path = gtk_tree_model_get_path(tm, &pti);
            tv = gtk_tree_selection_get_tree_view(ts);
            // Make the parent visible in scroll window if it is not.
            gtk_tree_view_scroll_to_cell(tv, path, NULL, FALSE, 0, 0);
            gtk_tree_path_free(path);
            return;
        }

        GtkTreePath *tp;
        gint *indices;
        GhbValue *audio_list;

        // Get the row number
        tp = gtk_tree_model_get_path (tm, &ti);
        indices = gtk_tree_path_get_indices (tp);
        row = indices[0];
        gtk_tree_path_free(tp);

        if (row < 0) return;
        audio_list = ghb_get_job_audio_list(ud->settings);
        if (row >= 0 && row < ghb_array_len(audio_list))
            asettings = ghb_array_get(audio_list, row);
    }
    audio_update_dialog_widgets(ud, asettings);
}

static void
audio_add_to_settings(GhbValue *settings, GhbValue *asettings)
{
    GhbValue *audio_list;
    int title_id;
    const hb_title_t *title;
    gint titleindex;
    hb_audio_config_t *aconfig;

    title_id = ghb_dict_get_int(settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    audio_list = ghb_get_job_audio_list(settings);

    int track = ghb_dict_get_int(asettings, "Track");
    aconfig = ghb_get_audio_info(title, track);
    if (aconfig != NULL)
    {
        char *desc;
        desc = g_strdup_printf("%d - %s", track + 1, aconfig->lang.description);
        ghb_dict_set_string(asettings, "Description", desc);
        g_free(desc);
    }

    ghb_array_append(audio_list, asettings);
}

G_MODULE_EXPORT void
audio_add_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    // Add the current audio settings to the list.
    GhbValue *asettings, *audio_dict, *backup;
    int title_id, titleindex;
    const hb_title_t *title;

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);

    // Back up settings in case we need to revert.
    audio_dict = ghb_get_job_audio_settings(ud->settings);
    backup = ghb_value_dup(ghb_get_job_audio_list(ud->settings));
    GhbValue *pref_audio = ghb_dict_get_value(ud->settings, "AudioList");
    asettings = audio_select_and_add_track(title, ud->settings, pref_audio,
                                           "und", 0, 0);
    ghb_add_audio_to_ui(ud, asettings);

    if (asettings != NULL)
    {
        // Pop up the edit dialog
        GtkResponseType response;
        GtkWidget *dialog = GHB_WIDGET(ud->builder, "audio_dialog");
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_hide(dialog);
        if (response != GTK_RESPONSE_OK)
        {
            ghb_dict_set(audio_dict, "AudioList", backup);
            asettings = audio_get_selected_settings(ud, NULL);
            if (asettings != NULL)
            {
                audio_update_dialog_widgets(ud, asettings);
            }
            audio_refresh_list_ui(ud);
        }
        else
        {
            ghb_value_free(&backup);
        }
    }
}

G_MODULE_EXPORT void
audio_add_all_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    int title_id, titleindex;
    const hb_title_t *title;

    // Add the current audio settings to the list.
    ghb_clear_audio_list_settings(ud->settings);
    ghb_clear_audio_list_ui(ud->builder);

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    GhbValue *pref_audio = ghb_dict_get_value(ud->settings, "AudioList");

    int pref_count = ghb_array_len(pref_audio);

    int ii;
    for (ii = 0; ii < pref_count; ii++)
    {
        GhbValue *asettings;
        int track = 0;

        do
        {

            asettings = audio_select_and_add_track(title, ud->settings,
                                                   pref_audio, "und", ii,
                                                   track);
            if (asettings != NULL)
            {
                track = ghb_dict_get_int(asettings, "Track") + 1;
            }
        } while (asettings != NULL);
    }
    audio_refresh_list_ui(ud);
}

G_MODULE_EXPORT void
audio_edit_clicked_cb(GtkWidget *widget, gchar *path, signal_user_data_t *ud)
{
    GtkTreeView *tv;
    GtkTreePath *tp;
    GtkTreeModel *tm;
    GtkTreeSelection *ts;
    GtkTreeIter ti;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list_view"));
    ts = gtk_tree_view_get_selection(tv);
    tm = gtk_tree_view_get_model(tv);
    tp = gtk_tree_path_new_from_string (path);
    if (gtk_tree_path_get_depth(tp) > 1) return;
    if (gtk_tree_model_get_iter(tm, &ti, tp))
    {
        GhbValue *asettings, *audio_dict, *backup;

        gtk_tree_selection_select_iter(ts, &ti);

        audio_dict = ghb_get_job_audio_settings(ud->settings);
        // Back up settings in case we need to revert.
        backup = ghb_value_dup(ghb_get_job_audio_list(ud->settings));

        // Pop up the edit dialog
        GtkResponseType response;
        GtkWidget *dialog = GHB_WIDGET(ud->builder, "audio_dialog");
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_hide(dialog);
        if (response != GTK_RESPONSE_OK)
        {
            ghb_dict_set(audio_dict, "AudioList", backup);
            asettings = audio_get_selected_settings(ud, NULL);
            if (asettings != NULL)
            {
                audio_update_dialog_widgets(ud, asettings);
            }
            audio_refresh_list_ui(ud);
        }
        else
        {
            ghb_value_free(&backup);
        }
    }
}

G_MODULE_EXPORT void
audio_remove_clicked_cb(GtkWidget *widget, gchar *path, signal_user_data_t *ud)
{
    GtkTreeView *tv;
    GtkTreePath *tp;
    GtkTreeModel *tm;
    GtkTreeSelection *ts;
    GtkTreeIter ti, nextIter;
    gint row;
    gint *indices;
    GhbValue *audio_list;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list_view"));
    ts = gtk_tree_view_get_selection(tv);
    tm = gtk_tree_view_get_model(tv);
    tp = gtk_tree_path_new_from_string (path);
    if (gtk_tree_path_get_depth(tp) > 1) return;
    if (gtk_tree_model_get_iter(tm, &ti, tp))
    {
        nextIter = ti;
        if (!gtk_tree_model_iter_next(tm, &nextIter))
        {
            nextIter = ti;
            if (gtk_tree_model_get_iter_first(tm, &nextIter))
            {
                gtk_tree_selection_select_iter(ts, &nextIter);
            }
        }
        else
        {
            gtk_tree_selection_select_iter(ts, &nextIter);
        }

        audio_list = ghb_get_job_audio_list(ud->settings);

        // Get the row number
        indices = gtk_tree_path_get_indices (tp);
        row = indices[0];
        if (row < 0 || row >= ghb_array_len(audio_list))
        {
            gtk_tree_path_free(tp);
            return;
        }

        // Update our settings list before removing the row from the
        // treeview.  Removing from the treeview sometimes provokes an
        // immediate selection change, so the list needs to be up to date
        // when this happens.
        ghb_array_remove(audio_list, row);

        // Remove the selected item
        gtk_tree_store_remove(GTK_TREE_STORE(tm), &ti);

        ghb_live_reset(ud);
    }
    gtk_tree_path_free(tp);
}

G_MODULE_EXPORT void
audio_reset_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_set_pref_audio_settings(ud->settings);
    audio_refresh_list_ui(ud);
}

static GtkWidget *find_widget_recurse(GtkWidget *widget, const gchar *name)
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
            result = find_widget_recurse(GTK_WIDGET(link->data), name);
            if (result != NULL)
                break;
            link = link->next;
        }
        g_list_free(list);
    }
    return result;
}

static GtkWidget *find_widget(GtkWidget *widget, const gchar *name)
{
    GtkWidget *result = NULL;

    result = find_widget_recurse(widget, name);
    if (result == NULL)
    {
        g_debug("Failed to find audio widget %s", name);
    }
    return result;
}

static void audio_def_update_widgets(GtkWidget *row, GhbValue *adict)
{
    GhbDictIter ti;
    const gchar *key;
    GhbValue *gval;
    GtkWidget *widget;

    ti = ghb_dict_iter_init(adict);

    block_updates = TRUE;
    while (ghb_dict_iter_next(adict, &ti, &key, &gval))
    {
        widget = find_widget(row, key);
        if (widget != NULL)
        {
            ghb_update_widget(widget, gval);
        }
    }
    block_updates = FALSE;
}

static GtkListBoxRow*
audio_settings_get_row(GtkWidget *widget)
{
    while (widget != NULL && G_OBJECT_TYPE(widget) != GTK_TYPE_LIST_BOX_ROW)
    {
        widget = gtk_widget_get_parent(widget);
    }
    return GTK_LIST_BOX_ROW(widget);
}

static void audio_def_settings_bitrate_show(GtkWidget *widget, gboolean show)
{
    GtkWidget *bitrate_widget;
    GtkWidget *quality_widget;

    bitrate_widget = find_widget(widget, "AudioBitrate");
    quality_widget = find_widget(widget, "AudioTrackQualityBox");

    if (show)
    {
        gtk_widget_hide(quality_widget);
        gtk_widget_show(bitrate_widget);
    }
    else
    {
        gtk_widget_hide(bitrate_widget);
        gtk_widget_show(quality_widget);
    }
}

static void audio_def_settings_quality_set_sensitive(GtkWidget *w, gboolean s)
{
    GtkWidget *bitrate_widget;
    GtkWidget *quality_widget;

    bitrate_widget = find_widget(w, "AudioTrackBitrateEnable");
    quality_widget = find_widget(w, "AudioTrackQualityEnable");
    if (!s)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bitrate_widget), TRUE);
    }
    gtk_widget_set_sensitive(GTK_WIDGET(quality_widget), s);
}

static void audio_def_settings_show(GtkWidget *widget, gboolean show)
{
    GtkWidget *settings_box;
    GtkWidget *add_button;

    settings_box = find_widget(widget, "settings_box");
    add_button = find_widget(widget, "add_button");

    if (show)
    {
        gtk_widget_hide(add_button);
        gtk_widget_show(settings_box);
    }
    else
    {
        gtk_widget_hide(settings_box);
        gtk_widget_show(add_button);
    }
}

static void
audio_def_settings_init_row(GhbValue *adict, GtkWidget *row)
{
    GtkWidget *widget;

    widget = find_widget(row, "AudioEncoder");
    ghb_widget_to_setting(adict, widget);
    widget = find_widget(row, "AudioBitrate");
    ghb_widget_to_setting(adict, widget);
    widget = find_widget(row, "AudioMixdown");
    ghb_widget_to_setting(adict, widget);
    widget = find_widget(row, "AudioSamplerate");
    ghb_widget_to_setting(adict, widget);
    widget = find_widget(row, "AudioTrackGainSlider");
    ghb_widget_to_setting(adict, widget);
    widget = find_widget(row, "AudioTrackDRCSlider");
    ghb_widget_to_setting(adict, widget);
    widget = find_widget(row, "AudioTrackQualityX");
    ghb_widget_to_setting(adict, widget);
    widget = find_widget(row, "AudioTrackQualityEnable");
    ghb_widget_to_setting(adict, widget);
}

G_MODULE_EXPORT void
audio_def_setting_add_cb(GtkWidget *w, signal_user_data_t *ud);
G_MODULE_EXPORT void
audio_def_setting_remove_cb(GtkWidget *w, signal_user_data_t *ud);
G_MODULE_EXPORT void
audio_def_encoder_changed_cb(GtkWidget *w, signal_user_data_t *ud);
G_MODULE_EXPORT void
audio_def_encode_setting_changed_cb(GtkWidget *w, signal_user_data_t *ud);
G_MODULE_EXPORT void
audio_def_drc_changed_cb(GtkWidget *w, gdouble drc, signal_user_data_t *ud);
G_MODULE_EXPORT void
audio_def_gain_changed_cb(GtkWidget *w, gdouble gain, signal_user_data_t *ud);
G_MODULE_EXPORT void
audio_def_quality_changed_cb(GtkWidget *w, gdouble quality, signal_user_data_t *ud);
G_MODULE_EXPORT void
audio_def_quality_enable_changed_cb(GtkWidget *w, signal_user_data_t *ud);

GtkWidget * ghb_create_audio_settings_row(signal_user_data_t *ud)
{
    GtkBox *box, *box2, *box3;
    GtkComboBox *combo;
    GtkScaleButton *scale;
    GtkLabel *label;
    GtkRadioButton *radio;
    GtkButton *button;
    GtkImage *image;

    box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));

    // Add Button
    button = GTK_BUTTON(gtk_button_new_with_label(_("Add")));
    gtk_widget_set_tooltip_markup(GTK_WIDGET(button),
    _("Add an audio encoder.\n"
      "Each selected source track will be encoded with all selected encoders."));
    gtk_widget_set_valign(GTK_WIDGET(button), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(button), "add_button");
    gtk_widget_hide(GTK_WIDGET(button));
    g_signal_connect(button, "clicked", (GCallback)audio_def_setting_add_cb, ud);
    gtk_box_pack_start(box, GTK_WIDGET(button), FALSE, FALSE, 0);

    // Hidden widgets box
    box2 = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_widget_set_name(GTK_WIDGET(box2), "settings_box");

    // Audio Encoder ComboBox
    combo = GTK_COMBO_BOX(gtk_combo_box_new());
    ghb_init_combo_box(combo);
    ghb_audio_encoder_opts_set(combo);
    // Init to first audio encoder
    const hb_encoder_t *aud_enc = hb_audio_encoder_get_next(NULL);
    ghb_update_widget(GTK_WIDGET(combo), ghb_int_value(aud_enc->codec));
    gtk_widget_set_tooltip_markup(GTK_WIDGET(combo),
      _("Set the audio codec to encode this track with."));
    gtk_widget_set_valign(GTK_WIDGET(combo), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(combo), "AudioEncoder");
    gtk_widget_show(GTK_WIDGET(combo));
    g_signal_connect(combo, "changed", (GCallback)audio_def_encoder_changed_cb, ud);
    gtk_box_pack_start(box2, GTK_WIDGET(combo), FALSE, FALSE, 0);

    box3 = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_widget_set_name(GTK_WIDGET(box3), "br_q_box");
    gtk_widget_show(GTK_WIDGET(box3));

    // Bitrate vs Quality RadioButton
    GtkBox *vbox;
    vbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    radio = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(NULL, _("Bitrate")));
    gtk_widget_set_name(GTK_WIDGET(radio), "AudioTrackBitrateEnable");
    gtk_widget_show(GTK_WIDGET(radio));
    gtk_box_pack_start(vbox, GTK_WIDGET(radio), FALSE, FALSE, 0);
    radio = GTK_RADIO_BUTTON(
                gtk_radio_button_new_with_label_from_widget(radio, _("Quality")));
    gtk_widget_set_name(GTK_WIDGET(radio), "AudioTrackQualityEnable");
    g_signal_connect(radio, "toggled", (GCallback)audio_def_quality_enable_changed_cb, ud);
    gtk_widget_show(GTK_WIDGET(radio));
    gtk_box_pack_start(vbox, GTK_WIDGET(radio), FALSE, FALSE, 0);
    gtk_widget_show(GTK_WIDGET(vbox));
    gtk_box_pack_start(box3, GTK_WIDGET(vbox), FALSE, FALSE, 0);

    // Audio Bitrate ComboBox
    combo = GTK_COMBO_BOX(gtk_combo_box_new());
    ghb_init_combo_box(combo);
    ghb_audio_bitrate_opts_set(combo);
    ghb_update_widget(GTK_WIDGET(combo), ghb_int_value(192));
    gtk_widget_set_tooltip_markup(GTK_WIDGET(combo),
      _("Set the bitrate to encode this track with."));
    gtk_widget_set_valign(GTK_WIDGET(combo), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(combo), "AudioBitrate");
    gtk_widget_show(GTK_WIDGET(combo));
    g_signal_connect(combo, "changed", (GCallback)audio_def_encode_setting_changed_cb, ud);
    gtk_box_pack_start(box3, GTK_WIDGET(combo), FALSE, FALSE, 0);

    GtkBox *qbox;
    qbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_widget_set_name(GTK_WIDGET(qbox), "AudioTrackQualityBox");

    // Audio Quality ScaleButton
    const gchar *quality_icons[] = {
        "weather-storm",
        "weather-clear",
        "weather-storm",
        "weather-showers-scattered",
        "weather-showers",
        "weather-overcast",
        "weather-few-clouds",
        "weather-clear",
        NULL
    };
    scale = GTK_SCALE_BUTTON(gtk_scale_button_new(GTK_ICON_SIZE_BUTTON,
                                                  0, 10, 0.1, quality_icons));
    gtk_widget_set_tooltip_markup(GTK_WIDGET(scale),
      _("<b>Audio Quality:</b>\n"
        "For encoders that support it, adjust the quality of the output."));

    gtk_widget_set_valign(GTK_WIDGET(scale), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(scale), "AudioTrackQualityX");
    gtk_widget_show(GTK_WIDGET(scale));
    g_signal_connect(scale, "value-changed", (GCallback)audio_def_quality_changed_cb, ud);
    gtk_box_pack_start(qbox, GTK_WIDGET(scale), FALSE, FALSE, 0);

    // Audio Quality Label
    label = GTK_LABEL(gtk_label_new("0.00"));
    gtk_label_set_width_chars(label, 4);
    gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
    gtk_widget_set_valign(GTK_WIDGET(label), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(label), "AudioTrackQualityValue");
    gtk_widget_show(GTK_WIDGET(label));
    gtk_box_pack_start(qbox, GTK_WIDGET(label), FALSE, FALSE, 0);
    gtk_widget_hide(GTK_WIDGET(qbox));
    gtk_box_pack_start(box3, GTK_WIDGET(qbox), FALSE, FALSE, 0);
    gtk_box_pack_start(box2, GTK_WIDGET(box3), FALSE, FALSE, 0);

    // Audio Mix ComboBox
    combo = GTK_COMBO_BOX(gtk_combo_box_new());
    ghb_init_combo_box(combo);
    ghb_mix_opts_set(combo);
    ghb_update_widget(GTK_WIDGET(combo), ghb_int_value(HB_AMIXDOWN_5POINT1));
    gtk_widget_set_tooltip_markup(GTK_WIDGET(combo),
      _("Set the mixdown of the output audio track."));
    gtk_widget_set_valign(GTK_WIDGET(combo), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(combo), "AudioMixdown");
    gtk_widget_show(GTK_WIDGET(combo));
    g_signal_connect(combo, "changed", (GCallback)audio_def_encode_setting_changed_cb, ud);
    gtk_box_pack_start(box2, GTK_WIDGET(combo), FALSE, FALSE, 0);

    // Audio Sample Rate ComboBox
    combo = GTK_COMBO_BOX(gtk_combo_box_new());
    ghb_init_combo_box(combo);
    ghb_audio_samplerate_opts_set(combo);
    ghb_update_widget(GTK_WIDGET(combo), ghb_int_value(0));
    gtk_widget_set_tooltip_markup(GTK_WIDGET(combo),
      _("Set the sample rate of the output audio track."));
    gtk_widget_set_valign(GTK_WIDGET(combo), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(combo), "AudioSamplerate");
    gtk_widget_show(GTK_WIDGET(combo));
    g_signal_connect(combo, "changed", (GCallback)audio_def_encode_setting_changed_cb, ud);
    gtk_box_pack_start(box2, GTK_WIDGET(combo), FALSE, FALSE, 0);

    box3 = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_widget_set_name(GTK_WIDGET(box3), "gain_box");
    gtk_widget_show(GTK_WIDGET(box3));

    // Audio Gain ScaleButton
    const gchar *gain_icons[] = {
        "audio-volume-muted",
        "audio-volume-high",
        "audio-volume-low",
        "audio-volume-medium",
        NULL
    };
    scale = GTK_SCALE_BUTTON(gtk_scale_button_new(GTK_ICON_SIZE_BUTTON,
                                                  -20, 21, 1, gain_icons));
    gtk_widget_set_tooltip_markup(GTK_WIDGET(scale),
      _("<b>Audio Gain:</b> "
        "Adjust the amplification or attenuation of the output audio track."));

    gtk_widget_set_valign(GTK_WIDGET(scale), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(scale), "AudioTrackGainSlider");
    gtk_widget_show(GTK_WIDGET(scale));
    g_signal_connect(scale, "value-changed", (GCallback)audio_def_gain_changed_cb, ud);
    gtk_box_pack_start(box3, GTK_WIDGET(scale), FALSE, FALSE, 0);

    // Audio Gain Label
    label = GTK_LABEL(gtk_label_new(_("0dB")));
    gtk_label_set_width_chars(label, 6);
    gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
    gtk_widget_set_valign(GTK_WIDGET(label), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(label), "AudioTrackGainValue");
    gtk_widget_show(GTK_WIDGET(label));
    gtk_box_pack_start(box3, GTK_WIDGET(label), FALSE, FALSE, 0);
    gtk_box_pack_start(box2, GTK_WIDGET(box3), FALSE, FALSE, 0);

    box3 = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_widget_set_name(GTK_WIDGET(box3), "drc_box");
    gtk_widget_show(GTK_WIDGET(box3));

    // Audio DRC ComboBox
    const gchar *drc_icons[] = {
        "audio-input-microphone",
        NULL
    };
    scale = GTK_SCALE_BUTTON(gtk_scale_button_new(GTK_ICON_SIZE_BUTTON,
                                                  0.9, 4, 0.1, drc_icons));
    gtk_widget_set_tooltip_markup(GTK_WIDGET(scale),
      _("<b>Dynamic Range Compression:</b> "
        "Adjust the dynamic range of the output audio track.\n\n"
        "For source audio that has a wide dynamic range "
        "(very loud and very soft sequences),\n"
        "DRC allows you to 'compress' the range by making "
        "loud sounds softer and soft sounds louder."));

    gtk_widget_set_valign(GTK_WIDGET(scale), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(scale), "AudioTrackDRCSlider");
    gtk_widget_show(GTK_WIDGET(scale));
    g_signal_connect(scale, "value-changed", (GCallback)audio_def_drc_changed_cb, ud);
    gtk_box_pack_start(box3, GTK_WIDGET(scale), FALSE, FALSE, 0);

    // Audio DRC Label
    label = GTK_LABEL(gtk_label_new(_("Off")));
    gtk_label_set_width_chars(label, 4);
    gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
    gtk_widget_set_valign(GTK_WIDGET(label), GTK_ALIGN_CENTER);
    gtk_widget_set_name(GTK_WIDGET(label), "AudioTrackDRCValue");
    gtk_widget_show(GTK_WIDGET(label));
    gtk_box_pack_start(box3, GTK_WIDGET(label), FALSE, FALSE, 0);
    gtk_box_pack_start(box2, GTK_WIDGET(box3), FALSE, FALSE, 0);

    // Remove button
    image = GTK_IMAGE(gtk_image_new_from_icon_name("hb-remove",
                                                   GTK_ICON_SIZE_BUTTON));
    button = GTK_BUTTON(gtk_button_new());
    gtk_button_set_image(button, GTK_WIDGET(image));
    gtk_widget_set_tooltip_markup(GTK_WIDGET(button),
    _("Remove this audio encoder"));
    gtk_button_set_relief(button, GTK_RELIEF_NONE);
    gtk_widget_set_valign(GTK_WIDGET(button), GTK_ALIGN_CENTER);
    gtk_widget_set_halign(GTK_WIDGET(button), GTK_ALIGN_END);
    gtk_widget_set_name(GTK_WIDGET(button), "remove_button");
    gtk_widget_show(GTK_WIDGET(button));
    g_signal_connect(button, "clicked", (GCallback)audio_def_setting_remove_cb, ud);
    gtk_box_pack_start(box2, GTK_WIDGET(button), TRUE, TRUE, 0);

    gtk_widget_show(GTK_WIDGET(box2));
    gtk_box_pack_start(box, GTK_WIDGET(box2), TRUE, TRUE, 0);

    gtk_widget_show(GTK_WIDGET(box));

    GtkWidget *widget;

    int width;
    widget = find_widget(GTK_WIDGET(box), "AudioEncoder");
    gtk_widget_get_preferred_width(widget, NULL, &width);

    widget = GHB_WIDGET(ud->builder, "audio_defaults_encoder_label");
    gtk_widget_set_size_request(widget, width, -1);
    widget = find_widget(GTK_WIDGET(box), "br_q_box");
    gtk_widget_get_preferred_width(widget, NULL, &width);
    widget = GHB_WIDGET(ud->builder, "audio_defaults_bitrate_label");
    gtk_widget_set_size_request(widget, width, -1);
    widget = find_widget(GTK_WIDGET(box), "AudioMixdown");
    gtk_widget_get_preferred_width(widget, NULL, &width);
    widget = GHB_WIDGET(ud->builder, "audio_defaults_mixdown_label");
    gtk_widget_set_size_request(widget, width, -1);
    widget = find_widget(GTK_WIDGET(box), "AudioSamplerate");
    gtk_widget_get_preferred_width(widget, NULL, &width);
    widget = GHB_WIDGET(ud->builder, "audio_defaults_samplerate_label");
    gtk_widget_set_size_request(widget, width, -1);
    widget = find_widget(GTK_WIDGET(box), "gain_box");
    gtk_widget_get_preferred_width(widget, NULL, &width);
    widget = GHB_WIDGET(ud->builder, "audio_defaults_gain_label");
    gtk_widget_set_size_request(widget, width, -1);
    widget = find_widget(GTK_WIDGET(box), "drc_box");
    gtk_widget_get_preferred_width(widget, NULL, &width);
    widget = GHB_WIDGET(ud->builder, "audio_defaults_drc_label");
    gtk_widget_set_size_request(widget, width, -1);

    return GTK_WIDGET(box);
}

static void
audio_def_setting_update(signal_user_data_t *ud, GtkWidget *widget)
{
    GtkListBoxRow *row = audio_settings_get_row(widget);
    gint index = gtk_list_box_row_get_index(row);

    GhbValue *alist = ghb_dict_get_value(ud->settings, "AudioList");
    int count = ghb_array_len(alist);
    if (!block_updates && index >= 0 && index < count)
    {
        GhbValue *adict = ghb_array_get(alist, index);
        ghb_widget_to_setting(adict, widget);
    }
}

G_MODULE_EXPORT void
audio_add_lang_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GtkListBox *avail = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "audio_avail_lang"));
    GtkListBox *selected = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "audio_selected_lang"));
    GtkListBoxRow *row;
    GtkWidget *label;

    row = gtk_list_box_get_selected_row(avail);
    if (row != NULL)
    {
        int idx;
        const iso639_lang_t *lang;
        GhbValue *glang, *alang_list;

        // Remove from UI available language list box
        label = gtk_bin_get_child(GTK_BIN(row));
        g_object_ref(G_OBJECT(label));
        gtk_widget_destroy(GTK_WIDGET(row));
        gtk_widget_show(label);
        // Add to UI selected language list box
        gtk_list_box_insert(selected, label, -1);

        // Add to preset language list
        idx = (intptr_t)g_object_get_data(G_OBJECT(label), "lang_idx");
        lang = ghb_iso639_lookup_by_int(idx);
        glang = ghb_string_value_new(lang->iso639_2);
        alang_list = ghb_dict_get_value(ud->settings, "AudioLanguageList");
        ghb_array_append(alang_list, glang);
        ghb_clear_presets_selection(ud);
    }
}

G_MODULE_EXPORT void
audio_remove_lang_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{

    GtkListBox *avail, *selected;
    GtkListBoxRow *row;
    GtkWidget *label;

    avail = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "audio_avail_lang"));
    selected = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "audio_selected_lang"));
    row = gtk_list_box_get_selected_row(selected);
    if (row != NULL)
    {
        gint index;
        GhbValue *alang_list;

        index = gtk_list_box_row_get_index(row);

        // Remove from UI selected language list box
        label = gtk_bin_get_child(GTK_BIN(row));
        g_object_ref(G_OBJECT(label));
        int idx = (intptr_t)g_object_get_data(G_OBJECT(label), "lang_idx");
        gtk_widget_destroy(GTK_WIDGET(row));
        gtk_widget_show(label);
        // Add to UI available language list box
        gtk_list_box_insert(avail, label, idx);

        // Remove from preset language list
        alang_list = ghb_dict_get_value(ud->settings, "AudioLanguageList");
        ghb_array_remove(alang_list, index);
        ghb_clear_presets_selection(ud);
    }
}

static void audio_quality_update_limits(
    GtkWidget *widget,
    int encoder,
    gboolean set_default,
    gdouble value)
{
    float low, high, gran;
    int dir;

    hb_audio_quality_get_limits(encoder, &low, &high, &gran, &dir);
    if (set_default)
    {
        value = hb_audio_quality_get_default(encoder);
    }
    if (dir)
    {
        // Quality values are inverted
        value = high - value + low;
    }
    GtkScaleButton *sb;
    GtkAdjustment *adj;
    sb = GTK_SCALE_BUTTON(widget);
    adj = gtk_scale_button_get_adjustment(sb);
    gtk_adjustment_configure (adj, value, low, high, gran, gran * 10, 0);
}

void audio_def_set_limits(signal_user_data_t *ud, GtkWidget *widget, gboolean set_default)
{
    GtkListBoxRow *row = audio_settings_get_row(widget);
    gint index = gtk_list_box_row_get_index(row);

    GhbValue *alist = ghb_dict_get_value(ud->settings, "AudioList");
    int count = ghb_array_len(alist);
    if (index < 0 || index >= count)
        return;

    GhbValue *adict = ghb_array_get(alist, index);

    int codec = ghb_settings_audio_encoder_codec(adict, "AudioEncoder");
    int fallback = ghb_settings_audio_encoder_codec(ud->settings,
                                                    "AudioEncoderFallback");
    gdouble quality = ghb_dict_get_double(adict, "AudioTrackQuality");

    // Allow quality settings if the current encoder supports quality
    // or if the encoder is auto-passthru and the fallback encoder
    // supports quality.
    gboolean sensitive =
        hb_audio_quality_get_default(codec) != HB_INVALID_AUDIO_QUALITY ||
        (codec == HB_ACODEC_AUTO_PASS &&
         hb_audio_quality_get_default(fallback) != HB_INVALID_AUDIO_QUALITY);
    audio_def_settings_quality_set_sensitive(GTK_WIDGET(row), sensitive);

    int enc;
    if (sensitive)
    {
        enc = codec;
        if (hb_audio_quality_get_default(codec) == HB_INVALID_AUDIO_QUALITY)
        {
            enc = fallback;
        }
        audio_quality_update_limits(
            find_widget(GTK_WIDGET(row), "AudioTrackQualityX"),
            enc, set_default, quality);
    }

    enc = codec;
    if (enc & HB_ACODEC_PASS_FLAG)
    {
        enc = ghb_select_fallback(ud->settings, enc);
    }
    int sr = ghb_settings_audio_samplerate_rate(adict, "AudioSamplerate");
    int mix = ghb_settings_mixdown_mix(adict, "AudioMixdown");
    int low, high;
    hb_audio_bitrate_get_limits(enc, sr, mix, &low, &high);
    GtkWidget *w = find_widget(GTK_WIDGET(row), "AudioBitrate");
    ghb_audio_bitrate_opts_filter(GTK_COMBO_BOX(w), low, high);
    w = find_widget(GTK_WIDGET(row), "AudioMixdown");
    ghb_mix_opts_filter(GTK_COMBO_BOX(w), enc);
}

void audio_def_set_all_limits_cb(GtkWidget *widget, gpointer data)
{
    signal_user_data_t *ud = (signal_user_data_t*)data;
    audio_def_set_limits(ud, widget, FALSE);
}

void audio_def_set_all_limits(signal_user_data_t *ud)
{
    GtkListBox *list_box;

    list_box = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "audio_list_default"));
    gtk_container_foreach(GTK_CONTAINER(list_box),
                          audio_def_set_all_limits_cb, (gpointer)ud);
}

G_MODULE_EXPORT void
audio_def_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
audio_fallback_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    GhbValue *audio = ghb_get_job_audio_settings(ud->settings);
    ghb_dict_set(audio, "FallbackEncoder", ghb_value_dup(
                 ghb_dict_get(ud->settings, "AudioEncoderFallback")));
    audio_def_set_all_limits(ud);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
audio_def_quality_enable_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    audio_def_setting_update(ud, widget);

    GtkListBoxRow *row = audio_settings_get_row(widget);
    gint index = gtk_list_box_row_get_index(row);

    GhbValue *alist = ghb_dict_get_value(ud->settings, "AudioList");
    GhbValue *adict = ghb_array_get(alist, index);

    audio_def_settings_bitrate_show(GTK_WIDGET(row),
                !ghb_dict_get_bool(adict, "AudioTrackQualityEnable"));
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
audio_def_quality_changed_cb(GtkWidget *widget, gdouble quality, signal_user_data_t *ud)
{

    GtkListBoxRow *row = audio_settings_get_row(widget);
    GtkWidget *quality_label = find_widget(GTK_WIDGET(row),
                                           "AudioTrackQualityValue");
    gint index = gtk_list_box_row_get_index(row);

    GhbValue *alist = ghb_dict_get_value(ud->settings, "AudioList");
    GhbValue *adict = ghb_array_get(alist, index);
    int codec = ghb_settings_audio_encoder_codec(adict, "AudioEncoder");

    quality = get_quality(codec, quality);
    ghb_dict_set_double(adict, "AudioTrackQuality", quality);
    char *s_quality = get_quality_string(codec, quality);
    ghb_update_widget(quality_label, ghb_string_value(s_quality));
    g_free(s_quality);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
audio_def_gain_changed_cb(GtkWidget *widget, gdouble gain, signal_user_data_t *ud)
{
    audio_def_setting_update(ud, widget);

    GtkListBoxRow *row = audio_settings_get_row(widget);
    GtkWidget *gain_label = find_widget(GTK_WIDGET(row), "AudioTrackGainValue");
    char *s_gain = get_gain_string(gain);
    ghb_update_widget(gain_label, ghb_string_value(s_gain));
    g_free(s_gain);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
audio_def_drc_changed_cb(GtkWidget *widget, gdouble drc, signal_user_data_t *ud)
{
    audio_def_setting_update(ud, widget);

    GtkListBoxRow *row = audio_settings_get_row(widget);
    GtkWidget *drc_label = find_widget(GTK_WIDGET(row), "AudioTrackDRCValue");

    char *s_drc = get_drc_string(drc);
    ghb_update_widget(drc_label, ghb_string_value(s_drc));
    g_free(s_drc);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
audio_def_setting_add_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GtkListBoxRow *row = audio_settings_get_row(widget);

    GhbValue *adict;
    GhbValue *alist = ghb_dict_get_value(ud->settings, "AudioList");
    int count = ghb_array_len(alist);
    if (count > 0)
    {
        // Use first item in list as defaults for new entries.
        adict = ghb_value_dup(ghb_array_get(alist, 0));
        audio_def_update_widgets(GTK_WIDGET(row), adict);
    }
    else
    {
        // Use hard coded defaults
        adict = ghb_dict_new();
        audio_def_settings_init_row(adict, GTK_WIDGET(row));
    }
    ghb_array_append(alist, adict);
    audio_def_settings_show(GTK_WIDGET(row), TRUE);

    // Add new "Add" button
    widget = ghb_create_audio_settings_row(ud);
    audio_def_settings_show(widget, FALSE);
    GtkListBox *list_box;
    list_box = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "audio_list_default"));
    gtk_list_box_insert(list_box, widget, -1);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
audio_def_setting_remove_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GtkListBoxRow *row = audio_settings_get_row(widget);
    gint index = gtk_list_box_row_get_index(row);

    GhbValue *alist = ghb_dict_get_value(ud->settings, "AudioList");
    int count = ghb_array_len(alist);
    if (index < 0 || index >= count)
    {
        return;
    }
    gtk_widget_destroy(GTK_WIDGET(row));
    ghb_array_remove(alist, index);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
audio_def_encoder_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    audio_def_setting_update(ud, widget);
    audio_def_set_limits(ud, widget, TRUE);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
audio_def_encode_setting_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    audio_def_setting_update(ud, widget);
    audio_def_set_limits(ud, widget, FALSE);
    ghb_clear_presets_selection(ud);
}

GtkListBoxRow* ghb_find_lang_row(GtkListBox *list_box, int lang_idx)
{
    GList *list, *link;
    GtkListBoxRow *result = NULL;

    list = link = gtk_container_get_children(GTK_CONTAINER(list_box));
    while (link != NULL)
    {
        GtkListBoxRow *row = (GtkListBoxRow*)link->data;
        GtkWidget *label = gtk_bin_get_child(GTK_BIN(row));
        int idx = (intptr_t)g_object_get_data(G_OBJECT(label), "lang_idx");
        if (idx == lang_idx)
        {
            result = row;
            break;
        }
        link = link->next;
    }
    g_list_free(list);

    return result;
}

static void audio_def_lang_list_clear_cb(GtkWidget *row, gpointer data)
{
    GtkListBox *avail = (GtkListBox*)data;
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(row));
    g_object_ref(G_OBJECT(label));
    gtk_widget_destroy(GTK_WIDGET(row));
    gtk_widget_show(label);
    int idx = (intptr_t)g_object_get_data(G_OBJECT(label), "lang_idx");
    gtk_list_box_insert(avail, label, idx);
}

static void
audio_def_selected_lang_list_clear(signal_user_data_t *ud)
{
    GtkListBox *avail, *selected;
    avail = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "audio_avail_lang"));
    selected = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "audio_selected_lang"));
    gtk_container_foreach(GTK_CONTAINER(selected),
                          audio_def_lang_list_clear_cb, (gpointer)avail);
}

static void
audio_def_lang_list_init(signal_user_data_t *ud)
{
    GhbValue *lang_list;

    // Clear selected languages.
    audio_def_selected_lang_list_clear(ud);

    lang_list = ghb_dict_get_value(ud->settings, "AudioLanguageList");
    if (lang_list == NULL)
    {
        lang_list = ghb_array_new();
        ghb_dict_set(ud->settings, "AudioLanguageList", lang_list);
    }

    int ii, count;
    count = ghb_array_len(lang_list);
    for (ii = 0; ii < count; )
    {
        GhbValue *lang_val = ghb_array_get(lang_list, ii);
        int idx = ghb_lookup_lang(lang_val);

        GtkListBox *avail, *selected;
        GtkListBoxRow *row;
        avail = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "audio_avail_lang"));
        selected = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "audio_selected_lang"));
        row = ghb_find_lang_row(avail, idx);
        if (row)
        {
            GtkWidget *label = gtk_bin_get_child(GTK_BIN(row));
            g_object_ref(G_OBJECT(label));
            gtk_widget_destroy(GTK_WIDGET(row));
            gtk_widget_show(label);
            gtk_list_box_insert(selected, label, -1);
            ii++;
        }
        else
        {
            // Error in list.  Probably duplicate languages.  Remove
            // this item from the list.
            ghb_array_remove(lang_list, ii);
            count--;
        }
    }
}

void ghb_audio_defaults_to_ui(signal_user_data_t *ud)
{
    GtkListBox *list_box;
    GhbValue *alist;
    int count, ii;

    audio_def_lang_list_init(ud);

    // Init the AudioList settings if necessary
    alist = ghb_dict_get_value(ud->settings, "AudioList");
    if (alist == NULL)
    {
        alist = ghb_array_new();
        ghb_dict_set(ud->settings, "AudioList", alist);
    }

    // Empty the current list
    list_box = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "audio_list_default"));
    ghb_container_empty(GTK_CONTAINER(list_box));

    GtkWidget *widget;
    // Populate with new values
    count = ghb_array_len(alist);
    for (ii = 0; ii < count; ii++)
    {
        GhbValue *adict;

        adict = ghb_array_get(alist, ii);
        widget = ghb_create_audio_settings_row(ud);
        gtk_list_box_insert(list_box, widget, -1);
        audio_def_update_widgets(widget, adict);
    }
    // Add row with "Add" button
    widget = ghb_create_audio_settings_row(ud);
    audio_def_settings_show(widget, FALSE);
    gtk_list_box_insert(list_box, widget, -1);
}

void ghb_init_audio_defaults_ui(signal_user_data_t *ud)
{
    GtkListBox *list_box;

    list_box = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "audio_avail_lang"));
    ghb_init_lang_list_box(list_box);
}
