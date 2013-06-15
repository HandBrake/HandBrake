/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * audiohandler.c
 * Copyright (C) John Stebbins 2008-2013 <stebbins@stebbins>
 * 
 * audiohandler.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "ghbcompat.h"
#include "hb.h"
#include "settings.h"
#include "hb-backend.h"
#include "values.h"
#include "callbacks.h"
#include "preview.h"
#include "audiohandler.h"

static gboolean ghb_add_audio_to_settings(GValue *settings, GValue *asettings);
static void ghb_add_audio_to_ui(GtkBuilder *builder, const GValue *settings);
static GValue* get_selected_asettings(signal_user_data_t *ud);
static void ghb_clear_audio_list_settings(GValue *settings);
static void ghb_clear_audio_list_ui(GtkBuilder *builder);

static gboolean block_updates = FALSE;

void
ghb_show_hide_advanced_audio( signal_user_data_t *ud )
{
    GtkWidget *widget;

    g_debug("audio_advanced_clicked_cb ()");
    widget = GHB_WIDGET(ud->builder, "AdvancedAudioPassTable");
    if (!ghb_settings_get_boolean(ud->settings, "AdvancedAutoPassthru"))
        gtk_widget_hide(widget);
    else
        gtk_widget_show(widget);
}

void
check_list_full(signal_user_data_t *ud)
{
    GValue *audio_list;
    gint count;

    audio_list = ghb_settings_get_value(ud->settings, "audio_list");
    count = ghb_array_len(audio_list);
    GtkWidget * widget = GHB_WIDGET(ud->builder, "audio_add");
    gtk_widget_set_sensitive(widget, count != 99);
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
            if ( mux == HB_MUX_MKV )
                fallback = HB_ACODEC_LAME;
            else
                fallback = HB_ACODEC_FAAC;
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

int ghb_get_copy_mask(GValue *settings)
{
    gint mask = 0;

    if (ghb_settings_get_boolean(settings, "AudioAllowMP3Pass"))
    {
        mask |= HB_ACODEC_MP3;
    }
    if (ghb_settings_get_boolean(settings, "AudioAllowAACPass"))
    {
        mask |= HB_ACODEC_FFAAC;
    }
    if (ghb_settings_get_boolean(settings, "AudioAllowAC3Pass"))
    {
        mask |= HB_ACODEC_AC3;
    }
    if (ghb_settings_get_boolean(settings, "AudioAllowDTSPass"))
    {
        mask |= HB_ACODEC_DCA;
    }
    if (ghb_settings_get_boolean(settings, "AudioAllowDTSHDPass"))
    {
        mask |= HB_ACODEC_DCA_HD;
    }
    return mask;
}

int ghb_select_fallback( GValue *settings, int mux, int acodec )
{
    gint fallback = 0;

    switch ( acodec )
    {
        case HB_ACODEC_MP3_PASS:
            return HB_ACODEC_LAME;

        case HB_ACODEC_AAC_PASS:
            return HB_ACODEC_FAAC;

        case HB_ACODEC_AC3_PASS:
            return HB_ACODEC_AC3;

        default:
        {
            fallback = ghb_settings_combo_int(settings, "AudioEncoderFallback");
            return hb_autopassthru_get_encoder(acodec, 0, fallback, mux);
        }
    }
}

void
ghb_sanitize_audio(GValue *settings, GValue *asettings)
{
    gint titleindex, track, acodec, select_acodec, mix;
    hb_audio_config_t *aconfig;
    gint mux;
    gint bitrate;
    gint sr;
    
    g_debug("ghb_santiize_audio ()");
    mux = ghb_settings_combo_int(settings, "FileFormat");
    titleindex = ghb_settings_get_int(settings, "title_no");
    track = ghb_settings_combo_int(asettings, "AudioTrack");
    acodec = ghb_settings_combo_int(asettings, "AudioEncoder");
    mix = ghb_settings_combo_int(asettings, "AudioMixdown");
    bitrate = ghb_settings_combo_int(asettings, "AudioBitrate");
    sr = ghb_settings_combo_int(asettings, "AudioSamplerate");

    aconfig = ghb_get_scan_audio_info(titleindex, track);
    if (sr == 0)
    {
        sr = aconfig ? aconfig->in.samplerate : 48000;
    }
    gint fallback = ghb_select_fallback(settings, mux, acodec);
    gint copy_mask = ghb_get_copy_mask(settings);
    select_acodec = ghb_select_audio_codec(mux, aconfig, acodec, fallback, copy_mask);
    if (ghb_audio_is_passthru (select_acodec))
    {
        if (aconfig)
        {
            bitrate = aconfig->in.bitrate / 1000;

            // Set the values for bitrate and samplerate to the input rates
            mix = 0;
            ghb_settings_set_string(asettings, "AudioMixdown",
                ghb_lookup_combo_string("AudioMixdown", ghb_int_value(mix)));
            select_acodec &= aconfig->in.codec | HB_ACODEC_PASS_FLAG;
            ghb_settings_set_string(asettings, "AudioSamplerate",
                ghb_lookup_combo_string("AudioSamplerate", ghb_int_value(0)));
        }
        else
        {
            mix = 0;
            ghb_settings_set_string(asettings, "AudioMixdown",
                ghb_lookup_combo_string("AudioMixdown", ghb_int_value(mix)));
            ghb_settings_set_string(asettings, "AudioSamplerate",
                ghb_lookup_combo_string("AudioSamplerate", ghb_int_value(0)));
            bitrate = 448;
        }
        ghb_settings_set_double(asettings, "AudioTrackDRCSlider", 0.0);
    }
    else
    {
        if (mix == 0)
            mix = ghb_get_best_mix( aconfig, select_acodec, 0);
        bitrate = hb_audio_bitrate_get_best(select_acodec, bitrate, sr, mix);
        ghb_settings_set_string(asettings, "AudioMixdown",
            ghb_lookup_combo_string("AudioMixdown", ghb_int_value(mix)));
    }
    ghb_settings_set_string(asettings, "AudioBitrate",
        ghb_lookup_combo_string("AudioBitrate", ghb_int_value(bitrate)));

    ghb_settings_take_value(asettings, "AudioEncoderActual", 
                            ghb_lookup_audio_encoder_value(select_acodec));
}

void
ghb_adjust_audio_rate_combos(signal_user_data_t *ud)
{
    gint titleindex, track, acodec, select_acodec, mix;
    hb_audio_config_t *aconfig;
    GtkWidget *widget;
    GValue *gval;
    gint mux;
    gint bitrate;
    gint sr = 48000;
    
    g_debug("ghb_adjust_audio_rate_combos ()");
    mux = ghb_settings_combo_int(ud->settings, "FileFormat");
    titleindex = ghb_settings_combo_int(ud->settings, "title");

    widget = GHB_WIDGET(ud->builder, "AudioTrack");
    gval = ghb_widget_value(widget);
    track = ghb_lookup_combo_int("AudioTrack", gval);
    ghb_value_free(gval);

    widget = GHB_WIDGET(ud->builder, "AudioEncoder");
    gval = ghb_widget_value(widget);
    acodec = ghb_lookup_combo_int("AudioEncoder", gval);
    ghb_value_free(gval);
    widget = GHB_WIDGET(ud->builder, "AudioMixdown");
    gval = ghb_widget_value(widget);
    mix = ghb_lookup_combo_int("AudioMixdown", gval);
    ghb_value_free(gval);

    widget = GHB_WIDGET(ud->builder, "AudioBitrate");
    gval = ghb_widget_value(widget);
    bitrate = ghb_lookup_combo_int("AudioBitrate", gval);

    widget = GHB_WIDGET(ud->builder, "AudioSamplerate");
    gval = ghb_widget_value(widget);
    sr = ghb_lookup_combo_int("AudioSamplerate", gval);


    aconfig = ghb_get_scan_audio_info(titleindex, track);
    if (sr == 0)
    {
        sr = aconfig ? aconfig->in.samplerate : 48000;
    }
    gint fallback = ghb_select_fallback( ud->settings, mux, acodec );
    gint copy_mask = ghb_get_copy_mask(ud->settings);
    select_acodec = ghb_select_audio_codec(mux, aconfig, acodec, fallback, copy_mask);
    gboolean codec_defined_bitrate = FALSE;
    if (ghb_audio_is_passthru (select_acodec))
    {
        if (aconfig)
        {
            bitrate = aconfig->in.bitrate / 1000;

            // Set the values for bitrate and samplerate to the input rates
            ghb_set_bitrate_opts (ud->builder, bitrate, bitrate, bitrate);
            mix = 0;
            ghb_ui_update(ud, "AudioMixdown", ghb_int64_value(mix));
            select_acodec &= aconfig->in.codec | HB_ACODEC_PASS_FLAG;
            codec_defined_bitrate = TRUE;
            ghb_ui_update(ud, "AudioSamplerate", ghb_int64_value(0));
        }
        else
        {
            ghb_ui_update(ud, "AudioSamplerate", ghb_int64_value(0));
            mix = 0;
            ghb_ui_update(ud, "AudioMixdown", ghb_int64_value(mix));
            bitrate = 448;
        }
        ghb_ui_update(ud, "AudioTrackDRCSlider", ghb_double_value(0));
    }
    else
    {
        if (mix == 0)
            mix = ghb_get_best_mix( aconfig, select_acodec, 0);
        bitrate = hb_audio_bitrate_get_best(select_acodec, bitrate, sr, mix);
        ghb_ui_update(ud, "AudioMixdown", ghb_int64_value(mix));
    }
    if (!codec_defined_bitrate)
    {
        int low, high;
        mix = ghb_get_best_mix( aconfig, select_acodec, mix);
        hb_audio_bitrate_get_limits(select_acodec, sr, mix, &low, &high);
        ghb_set_bitrate_opts (ud->builder, low, high, -1);
    }
    ghb_ui_update(ud, "AudioBitrate", ghb_int64_value(bitrate));

    ghb_settings_take_value(ud->settings, "AudioEncoderActual", 
                            ghb_lookup_audio_encoder_value(select_acodec));
    GValue *asettings = get_selected_asettings(ud);
    if (asettings)
    {
        ghb_settings_take_value(asettings, "AudioEncoderActual", 
                            ghb_lookup_audio_encoder_value(select_acodec));
    }
    ghb_audio_list_refresh_selected(ud);
    ghb_check_dependency(ud, NULL, "AudioEncoderActual");
}

static void
free_audio_hash_key_value(gpointer data)
{
    g_free(data);
}

const gchar*
ghb_get_user_audio_lang(GValue *settings, gint titleindex, gint track)
{
    GValue *audio_list, *asettings;
    const gchar *lang;

    audio_list = ghb_settings_get_value(settings, "audio_list");
    if (ghb_array_len(audio_list) <= track)
        return "und";
    asettings = ghb_array_get_nth(audio_list, track);
    track = ghb_settings_get_int(asettings, "AudioTrack");
    lang = ghb_get_source_audio_lang(titleindex, track);
    return lang;
}

void
ghb_set_pref_audio_settings(gint titleindex, GValue *settings)
{
    gint track;
    gchar *source_lang;
    hb_audio_config_t *aconfig;
    GHashTable *track_indices;
    gint mux;

    const GValue *pref_audio;
    const GValue *audio, *drc, *gain, *enable_qual;
    gint acodec, bitrate, mix;
    gdouble rate, quality;
    gint count, ii, list_count;
    
    g_debug("set_pref_audio");
    mux = ghb_settings_combo_int(settings, "FileFormat");
    track_indices = g_hash_table_new_full(g_int_hash, g_int_equal, 
                        free_audio_hash_key_value, free_audio_hash_key_value);
    // Clear the audio list
    ghb_clear_audio_list_settings(settings);

    // Find "best" audio based on audio preferences
    if (!ghb_settings_get_boolean(settings, "AudioDUB"))
    {
        source_lang = g_strdup(ghb_get_source_audio_lang(titleindex, 0));
    }
    else
    {
        source_lang = ghb_settings_get_string(settings, "PreferredLanguage");
    }

    pref_audio = ghb_settings_get_value(settings, "AudioList");

    list_count = 0;
    count = ghb_array_len(pref_audio);
    for (ii = 0; ii < count; ii++)
    {
        gint select_acodec;
        gint fallback;

        audio = ghb_array_get_nth(pref_audio, ii);
        acodec = ghb_settings_combo_int(audio, "AudioEncoder");
        fallback = ghb_select_fallback(settings, mux, acodec);
        gint copy_mask = ghb_get_copy_mask(settings);
        select_acodec = ghb_select_audio_codec(mux, NULL, acodec, fallback, copy_mask);
        bitrate = ghb_settings_combo_int(audio, "AudioBitrate");
        rate = ghb_settings_combo_double(audio, "AudioSamplerate");
        mix = ghb_settings_combo_int(audio, "AudioMixdown");
        drc = ghb_settings_get_value(audio, "AudioTrackDRCSlider");
        gain = ghb_settings_get_value(audio, "AudioTrackGain");
        enable_qual = ghb_settings_get_value(audio, "AudioTrackQualityEnable");
        quality = ghb_settings_get_double(audio, "AudioTrackQuality");
        // If there are multiple audios using the same codec, then
        // select sequential tracks for each.  The hash keeps track 
        // of the tracks used for each codec.
        track = ghb_find_audio_track(titleindex, source_lang, 
                                select_acodec, fallback, track_indices);
        // Check to see if:
        // 1. pref codec is passthru
        // 2. source codec is not passthru
        // 3. next pref is enabled
        aconfig = ghb_get_scan_audio_info(titleindex, track);
        if (aconfig && ghb_audio_is_passthru (acodec))
        {
            // HB_ACODEC_* are bit fields.  Treat acodec as mask
            if (!(aconfig->in.codec & select_acodec & HB_ACODEC_PASS_MASK))
            {
                if (acodec != HB_ACODEC_AUTO_PASS)
                    acodec = fallback;
                // If we can't substitute the passthru with a suitable
                // encoder and
                // If there's more audio to process, or we've already
                // placed one in the list, then we can skip this one
                if (!(select_acodec & fallback) && 
                    ((ii + 1 < count) || (list_count != 0)))
                {
                    // Skip this audio
                    acodec = 0;
                }
            }
            else
            {
                select_acodec &= aconfig->in.codec | HB_ACODEC_PASS_FLAG;
            }
        }
        if (titleindex >= 0 && track < 0)
            acodec = 0;
        if (acodec != 0)
        {
            GValue *asettings = ghb_dict_value_new();
            ghb_settings_set_int(asettings, "AudioTrack", track);
            ghb_settings_set_string(asettings, "AudioEncoder", 
                ghb_lookup_combo_string("AudioEncoder", ghb_int_value(acodec)));
            ghb_settings_set_value(asettings, "AudioEncoderActual", 
                                    ghb_lookup_audio_encoder_value(select_acodec));
            ghb_settings_set_value(asettings, "AudioTrackQualityEnable", enable_qual);
            ghb_settings_set_double(asettings, "AudioTrackQuality", quality);

            // This gets set autimatically if the codec is passthru
            ghb_settings_set_string(asettings, "AudioBitrate",
                ghb_lookup_combo_string("AudioBitrate", ghb_int_value(bitrate)));
            ghb_settings_set_string(asettings, "AudioSamplerate",
                ghb_lookup_combo_string("AudioSamplerate", ghb_int_value(rate)));
            mix = ghb_get_best_mix( aconfig, select_acodec, mix);
            ghb_settings_set_string(asettings, "AudioMixdown",
                ghb_lookup_combo_string("AudioMixdown", ghb_int_value(mix)));
            ghb_settings_set_value(asettings, "AudioTrackDRCSlider", drc);
            ghb_settings_set_value(asettings, "AudioTrackGain", gain);
            ghb_sanitize_audio(settings, asettings);
            ghb_add_audio_to_settings(settings, asettings);
        }
    }
    g_free(source_lang);
    g_hash_table_destroy(track_indices);
}

void
ghb_set_pref_audio_from_settings(signal_user_data_t *ud, GValue *settings)
{
    const GValue *audio_list, *audio;
    gint count, ii;

    // Clear the audio list
    ghb_clear_audio_list_ui(ud->builder);

    audio_list = ghb_settings_get_value(settings, "audio_list");
    count = ghb_array_len(audio_list);
    for (ii = 0; ii < count; ii++)
    {
        audio = ghb_array_get_nth(audio_list, ii);
        ghb_add_audio_to_ui(ud->builder, audio);
        ghb_adjust_audio_rate_combos(ud);
    }
    check_list_full(ud);
}

static GValue*
get_selected_asettings(signal_user_data_t *ud)
{
    GtkTreeView *treeview;
    GtkTreePath *treepath;
    GtkTreeSelection *selection;
    GtkTreeModel *store;
    GtkTreeIter iter;
    gint *indices;
    gint row;
    GValue *asettings = NULL;
    const GValue *audio_list;
    
    g_debug("get_selected_asettings ()");
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
    selection = gtk_tree_view_get_selection (treeview);
    if (gtk_tree_selection_get_selected(selection, &store, &iter))
    {
        // Get the row number
        treepath = gtk_tree_model_get_path (store, &iter);
        indices = gtk_tree_path_get_indices (treepath);
        row = indices[0];
        gtk_tree_path_free(treepath);
        // find audio settings
        if (row < 0) return NULL;
        audio_list = ghb_settings_get_value(ud->settings, "audio_list");
        if (row >= ghb_array_len(audio_list))
            return NULL;
        asettings = ghb_array_get_nth(audio_list, row);
    }
    return asettings;
}

void
ghb_audio_list_refresh_selected(signal_user_data_t *ud)
{
    GtkTreeView *treeview;
    GtkTreePath *treepath;
    GtkTreeSelection *selection;
    GtkTreeModel *store;
    GtkTreeIter iter;
    gint *indices;
    gint row;
    GValue *asettings = NULL;
    const GValue *audio_list;
    
    g_debug("ghb_audio_list_refresh_selected ()");
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
    selection = gtk_tree_view_get_selection (treeview);
    if (gtk_tree_selection_get_selected(selection, &store, &iter))
    {
        const gchar *track, *codec, *br = NULL, *sr, *mix;
        gchar *s_drc, *s_gain, *s_quality = NULL;
        gdouble drc, gain;
        // Get the row number
        treepath = gtk_tree_model_get_path (store, &iter);
        indices = gtk_tree_path_get_indices (treepath);
        row = indices[0];
        gtk_tree_path_free(treepath);
        // find audio settings
        if (row < 0) return;
        audio_list = ghb_settings_get_value(ud->settings, "audio_list");
        if (row >= ghb_array_len(audio_list))
            return;
        asettings = ghb_array_get_nth(audio_list, row);

        track = ghb_settings_combo_option(asettings, "AudioTrack");
        codec = ghb_settings_combo_option(asettings, "AudioEncoderActual");
        double quality = ghb_settings_get_double(asettings, "AudioTrackQuality");
        if (ghb_settings_get_boolean(asettings, "AudioTrackQualityEnable") &&
            quality != HB_INVALID_AUDIO_QUALITY)
        {
            int codec = ghb_settings_combo_int(asettings, "AudioEncoderActual");
            s_quality = ghb_format_quality("Q/", codec, quality);
        }
        else
        {
            br = ghb_settings_combo_option(asettings, "AudioBitrate");
        }
        sr = ghb_settings_combo_option(asettings, "AudioSamplerate");
        mix = ghb_settings_combo_option(asettings, "AudioMixdown");
        gain = ghb_settings_get_double(asettings, "AudioTrackGain");
        s_gain = g_strdup_printf("%ddB", (int)gain);

        drc = ghb_settings_get_double(asettings, "AudioTrackDRCSlider");
        if (drc < 1.0)
            s_drc = g_strdup("Off");
        else
            s_drc = g_strdup_printf("%.1f", drc);

        gtk_list_store_set(GTK_LIST_STORE(store), &iter, 
            // These are displayed in list
            0, track,
            1, codec,
            2, s_quality ? s_quality : br,
            3, sr,
            4, mix,
            5, s_gain,
            6, s_drc,
            -1);
        g_free(s_drc);
        g_free(s_gain);
        g_free(s_quality);
    }
}

void
ghb_audio_list_refresh(signal_user_data_t *ud)
{
    GtkTreeView *treeview;
    GtkTreeIter iter;
    GtkListStore *store;
    gboolean done;
    gint row = 0;
    const GValue *audio_list;

    g_debug("ghb_audio_list_refresh ()");
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
    store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
    {
        do
        {
            const gchar *track, *codec, *br = NULL, *sr, *mix;
            gchar *s_drc, *s_gain, *s_quality = NULL;
            gdouble drc, gain;
            GValue *asettings;

            audio_list = ghb_settings_get_value(ud->settings, "audio_list");
            if (row >= ghb_array_len(audio_list))
                return;
            asettings = ghb_array_get_nth(audio_list, row);

            track = ghb_settings_combo_option(asettings, "AudioTrack");
            codec = ghb_settings_combo_option(asettings, "AudioEncoderActual");
            double quality = ghb_settings_get_double(asettings, "AudioTrackQuality");
            if (ghb_settings_get_boolean(asettings, "AudioTrackQualityEnable") &&
                quality != HB_INVALID_AUDIO_QUALITY)
            {
                int codec = ghb_settings_combo_int(asettings, "AudioEncoderActual");
                s_quality = ghb_format_quality("Q/", codec, quality);
            }
            else
            {
                br = ghb_settings_get_string(asettings, "AudioBitrate");
            }
            sr = ghb_settings_combo_option(asettings, "AudioSamplerate");
            mix = ghb_settings_combo_option(asettings, "AudioMixdown");
            gain = ghb_settings_get_double(asettings, "AudioTrackGain");
            s_gain = g_strdup_printf("%.fdB", gain);

            drc = ghb_settings_get_double(asettings, "AudioTrackDRCSlider");
            if (drc < 1.0)
                s_drc = g_strdup("Off");
            else
                s_drc = g_strdup_printf("%.1f", drc);

            gtk_list_store_set(GTK_LIST_STORE(store), &iter, 
                // These are displayed in list
                0, track,
                1, codec,
                2, s_quality ? s_quality : br,
                3, sr,
                4, mix,
                5, s_gain,
                6, s_drc,
                -1);
            g_free(s_drc);
            g_free(s_gain);
            done = !gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
            row++;
        } while (!done);
    }
}

static void enable_quality_widget(signal_user_data_t *ud, int acodec)
{
    GtkWidget *widget1, *widget2, *widget3;

    widget1 = GHB_WIDGET(ud->builder, "AudioTrackQualityEnable");
    widget2 = GHB_WIDGET(ud->builder, "AudioTrackQualityValue");
    widget3 = GHB_WIDGET(ud->builder, "AudioTrackQuality");
    if (hb_audio_quality_get_default(acodec) == HB_INVALID_AUDIO_QUALITY)
    {
        gtk_widget_hide(widget1);
        gtk_widget_hide(widget2);
        gtk_widget_hide(widget3);
    }
    else
    {
        gtk_widget_show(widget1);
        gtk_widget_show(widget2);
        gtk_widget_show(widget3);
    }
}

G_MODULE_EXPORT void
audio_codec_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    static gint prev_acodec = 0;
    gint acodec_code;
    GValue *asettings, *gval;
    
    g_debug("audio_codec_changed_cb ()");
    gval = ghb_widget_value(widget);
    acodec_code = ghb_lookup_combo_int("AudioEncoder", gval);
    ghb_value_free(gval);

    enable_quality_widget(ud, acodec_code);
    if (block_updates)
    {
        prev_acodec = acodec_code;
        ghb_grey_combo_options (ud);
        ghb_check_dependency(ud, widget, NULL);
        return;
    }

    asettings = get_selected_asettings(ud);
    if (ghb_audio_is_passthru (prev_acodec) && 
        !ghb_audio_is_passthru (acodec_code))
    {
        // Transition from passthru to not, put some audio settings back to 
        // pref settings
        gint titleindex;
        gint track;
        gint br, sr, mix_code;

        if (asettings != NULL)
        {
            br = ghb_settings_get_int(asettings, "AudioBitrate");
            sr = ghb_settings_combo_int(asettings, "AudioSamplerate");
            mix_code = ghb_settings_combo_int(asettings, "AudioMixdown");
        }
        else
        {
            br = 160;
            sr = 0;
            mix_code = 0;
        }

        titleindex = ghb_settings_combo_int(ud->settings, "title");
        track = ghb_settings_combo_int(ud->settings, "AudioTrack");
        if (sr)
        {
            sr = ghb_find_closest_audio_samplerate(sr);
        }
        ghb_ui_update(ud, "AudioSamplerate", ghb_int64_value(sr));

        hb_audio_config_t *aconfig;
        aconfig = ghb_get_scan_audio_info(titleindex, track);
        if (sr == 0)
        {
            sr = aconfig ? aconfig->in.samplerate : 48000;
        }
        mix_code = ghb_get_best_mix( aconfig, acodec_code, mix_code);
        br = hb_audio_bitrate_get_best(acodec_code, br, sr, mix_code);
        ghb_ui_update(ud, "AudioBitrate", ghb_int64_value(br));

        ghb_ui_update(ud, "AudioMixdown", ghb_int64_value(mix_code));
    }
    ghb_adjust_audio_rate_combos(ud);
    ghb_grey_combo_options (ud);
    ghb_check_dependency(ud, widget, NULL);
    prev_acodec = acodec_code;
    if (asettings != NULL)
    {
        ghb_widget_to_setting(asettings, widget);
        ghb_settings_set_value(asettings, "AudioEncoderActual", ghb_settings_get_value(ud->settings, "AudioEncoderActual"));
        ghb_audio_list_refresh_selected(ud);
    }
    ghb_live_reset(ud);

    float low, high, gran, defval;
    int dir;
    hb_audio_quality_get_limits(acodec_code, &low, &high, &gran, &dir);
    defval = hb_audio_quality_get_default(acodec_code);
    GtkScaleButton *sb;
    GtkAdjustment *adj;
    sb = GTK_SCALE_BUTTON(GHB_WIDGET(ud->builder, "AudioTrackQuality"));
    adj = gtk_scale_button_get_adjustment(sb);
    if (dir)
    {
        // Quality values are inverted
        defval = high - defval + low;
    }
    gtk_adjustment_configure (adj, defval, low, high, gran, gran * 10, 0);
}

G_MODULE_EXPORT void
audio_track_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GValue *asettings;

    g_debug("audio_track_changed_cb ()");
    if (block_updates)
    {
        ghb_check_dependency(ud, widget, NULL);
        ghb_grey_combo_options (ud);
        return;
    }

    ghb_adjust_audio_rate_combos(ud);
    ghb_check_dependency(ud, widget, NULL);
    ghb_grey_combo_options(ud);
    asettings = get_selected_asettings(ud);
    if (asettings != NULL)
    {
        const gchar *track;

        ghb_widget_to_setting(asettings, widget);
        ghb_audio_list_refresh_selected(ud);
        track = ghb_settings_combo_option(asettings, "AudioTrack");
        ghb_settings_set_string(asettings, "AudioTrackDescription", track);
    }
    ghb_live_reset(ud);
}

G_MODULE_EXPORT void
audio_mix_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GValue *asettings;

    g_debug("audio_mix_changed_cb ()");
    if (block_updates)
    {
        ghb_check_dependency(ud, widget, NULL);
        return;
    }

    ghb_adjust_audio_rate_combos(ud);
    ghb_check_dependency(ud, widget, NULL);
    asettings = get_selected_asettings(ud);
    if (asettings != NULL)
    {
        ghb_widget_to_setting(asettings, widget);
        ghb_audio_list_refresh_selected(ud);
    }
    ghb_live_reset(ud);
}

G_MODULE_EXPORT void
audio_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GValue *asettings;

    g_debug("audio_widget_changed_cb ()");
    if (block_updates)
    {
        ghb_check_dependency(ud, widget, NULL);
        return;
    }

    ghb_adjust_audio_rate_combos(ud);
    asettings = get_selected_asettings(ud);
    if (asettings != NULL)
    {
        ghb_widget_to_setting(asettings, widget);
        ghb_audio_list_refresh_selected(ud);
    }
    ghb_live_reset(ud);
}

G_MODULE_EXPORT void
global_audio_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    g_debug("global_audio_widget_changed_cb ()");
    if (block_updates)
    {
        ghb_check_dependency(ud, widget, NULL);
        return;
    }

    ghb_check_dependency(ud, widget, NULL);
    ghb_widget_to_setting(ud->settings, widget);
    ghb_adjust_audio_rate_combos(ud);
    ghb_grey_combo_options (ud);
    ghb_audio_list_refresh_selected(ud);
    ghb_live_reset(ud);
}

G_MODULE_EXPORT gchar*
format_drc_cb(GtkScale *scale, gdouble val, signal_user_data_t *ud)
{
    if (val < 1.0)
        return g_strdup_printf("%-7s", "Off");
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
    GValue *asettings;

    g_debug("quality_widget_changed_cb ()");

    ghb_check_dependency(ud, widget, NULL);
    float low, high, gran;
    int dir;
    int codec = ghb_settings_combo_int(ud->settings, "AudioEncoderActual");
    hb_audio_quality_get_limits(codec, &low, &high, &gran, &dir);
    if (dir)
    {
        // Quality values are inverted
        quality = high - quality + low;
    }
    char *s_quality = ghb_format_quality("", codec, quality);
    ghb_ui_update( ud, "AudioTrackQualityValue", ghb_string_value(s_quality));
    g_free(s_quality);

    if (block_updates) return;

    asettings = get_selected_asettings(ud);
    if (asettings != NULL)
    {
        ghb_settings_set_double(asettings, "AudioTrackQuality", quality);
        ghb_audio_list_refresh_selected(ud);
    }
    ghb_live_reset(ud);
}

G_MODULE_EXPORT void
drc_widget_changed_cb(GtkWidget *widget, gdouble drc, signal_user_data_t *ud)
{
    GValue *asettings;

    g_debug("drc_widget_changed_cb ()");

    ghb_check_dependency(ud, widget, NULL);
    if (block_updates) return;

    char *s_drc;
    if (drc < 0.99)
        s_drc = g_strdup("Off");
    else
        s_drc = g_strdup_printf("%.1f", drc);
    ghb_ui_update( ud, "AudioTrackDRCValue", ghb_string_value(s_drc));
    g_free(s_drc);

    asettings = get_selected_asettings(ud);
    if (asettings != NULL)
    {
        ghb_widget_to_setting(asettings, widget);
        ghb_audio_list_refresh_selected(ud);
    }
    ghb_live_reset(ud);
}

G_MODULE_EXPORT gchar*
format_gain_cb(GtkScale *scale, gdouble val, signal_user_data_t *ud)
{
    if ( val >= 21.0 )
        return g_strdup_printf("*11*");
    return g_strdup_printf("%ddB", (int)val);
}

G_MODULE_EXPORT void
gain_widget_changed_cb(GtkWidget *widget, gdouble gain, signal_user_data_t *ud)
{
    GValue *asettings;

    g_debug("gain_widget_changed_cb ()");

    ghb_check_dependency(ud, widget, NULL);
    if (block_updates) return;
    asettings = get_selected_asettings(ud);

    char *s_gain;
    if ( gain >= 21.0 )
        s_gain = g_strdup_printf("*11*");
    else
        s_gain = g_strdup_printf("%ddB", (int)gain);
    ghb_ui_update( ud, "AudioTrackGainValue", ghb_string_value(s_gain));
    g_free(s_gain);

    if (asettings != NULL)
    {
        ghb_widget_to_setting(asettings, widget);
        ghb_audio_list_refresh_selected(ud);
    }
    ghb_live_reset(ud);
}

void
ghb_clear_audio_list_settings(GValue *settings)
{
    GValue *audio_list;
    
    g_debug("clear_audio_list_settings ()");
    audio_list = ghb_settings_get_value(settings, "audio_list");
    if (audio_list == NULL)
    {
        audio_list = ghb_array_value_new(8);
        ghb_settings_set_value(settings, "audio_list", audio_list);
    }
    else
        ghb_array_value_reset(audio_list, 8);
}

void
ghb_clear_audio_list_ui(GtkBuilder *builder)
{
    GtkTreeView *treeview;
    GtkListStore *store;
    
    g_debug("clear_audio_list_ui ()");
    treeview = GTK_TREE_VIEW(GHB_WIDGET(builder, "audio_list"));
    store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
    gtk_list_store_clear (store);
}

static void
ghb_add_audio_to_ui(GtkBuilder *builder, const GValue *settings)
{
    GtkTreeView *treeview;
    GtkTreeIter iter;
    GtkListStore *store;
    GtkTreeSelection *selection;
    const gchar *track, *codec, *br = NULL, *sr, *mix;
    gchar *s_drc, *s_gain, *s_quality = NULL;
    gdouble drc;
    gdouble gain;
    
    g_debug("ghb_add_audio_to_ui ()");
    treeview = GTK_TREE_VIEW(GHB_WIDGET(builder, "audio_list"));
    selection = gtk_tree_view_get_selection (treeview);
    store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

    track = ghb_settings_combo_option(settings, "AudioTrack");
    codec = ghb_settings_combo_option(settings, "AudioEncoderActual");
    double quality = ghb_settings_get_double(settings, "AudioTrackQuality");
    if (ghb_settings_get_boolean(settings, "AudioTrackQualityEnable") &&
        quality != HB_INVALID_AUDIO_QUALITY)
    {
        int codec = ghb_settings_combo_int(settings, "AudioEncoderActual");
        s_quality = ghb_format_quality("Q/", codec, quality);
    }
    else
    {
        br = ghb_settings_combo_option(settings, "AudioBitrate");
    }
    sr = ghb_settings_combo_option(settings, "AudioSamplerate");
    mix = ghb_settings_combo_option(settings, "AudioMixdown");
    gain = ghb_settings_get_double(settings, "AudioTrackGain");
    s_gain = g_strdup_printf("%ddB", (int)gain);

    drc = ghb_settings_get_double(settings, "AudioTrackDRCSlider");
    if (drc < 1.0)
        s_drc = g_strdup("Off");
    else
        s_drc = g_strdup_printf("%.1f", drc);

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 
        // These are displayed in list
        0, track,
        1, codec,
        2, s_quality ? s_quality : br,
        3, sr,
        4, mix,
        5, s_gain,
        6, s_drc,
        -1);
    gtk_tree_selection_select_iter(selection, &iter);
    g_free(s_drc);
    g_free(s_gain);
    g_free(s_quality);
}

G_MODULE_EXPORT void
audio_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
    GtkTreeModel *store;
    GtkTreeIter iter;
    GtkWidget *widget;
    
    GtkTreePath *treepath;
    gint *indices;
    gint row;
    GValue *asettings = NULL;

    const GValue *audio_list;
    g_debug("audio_list_selection_changed_cb ()");
    if (gtk_tree_selection_get_selected(selection, &store, &iter))
    {
        //const gchar *actual_codec, *track, *codec, *bitrate, *sample_rate, *mix;
        //gdouble drc;

        // Get the row number
        treepath = gtk_tree_model_get_path (store, &iter);
        indices = gtk_tree_path_get_indices (treepath);
        row = indices[0];
        gtk_tree_path_free(treepath);
        // find audio settings
        if (row < 0) return;
        audio_list = ghb_settings_get_value(ud->settings, "audio_list");
        if (row >= ghb_array_len(audio_list))
            return;
        asettings = ghb_array_get_nth(audio_list, row);

        block_updates = TRUE;
        ghb_ui_update(ud, "AudioTrack", ghb_settings_get_value(asettings, "AudioTrack"));
        ghb_ui_update(ud, "AudioEncoder", ghb_settings_get_value(asettings, "AudioEncoder"));
        ghb_settings_set_value(ud->settings, "AudioEncoderActual", ghb_settings_get_value(asettings, "AudioEncoderActual"));
        ghb_check_dependency(ud, NULL, "AudioEncoderActual");
        ghb_ui_update(ud, "AudioBitrate", ghb_settings_get_value(asettings, "AudioBitrate"));
        ghb_ui_update(ud, "AudioTrackName", ghb_settings_get_value(asettings, "AudioTrackName"));
        ghb_ui_update(ud, "AudioSamplerate", ghb_settings_get_value(asettings, "AudioSamplerate"));
        ghb_ui_update(ud, "AudioMixdown", ghb_settings_get_value(asettings, "AudioMixdown"));
        ghb_ui_update(ud, "AudioTrackDRCSlider", ghb_settings_get_value(asettings, "AudioTrackDRCSlider"));
        ghb_ui_update(ud, "AudioTrackGain", ghb_settings_get_value(asettings, "AudioTrackGain"));
        ghb_ui_update(ud, "AudioTrackQuality", ghb_settings_get_value(asettings, "AudioTrackQuality"));
        ghb_ui_update(ud, "AudioTrackQualityEnable", ghb_settings_get_value(asettings, "AudioTrackQualityEnable"));
        block_updates = FALSE;
        widget = GHB_WIDGET (ud->builder, "audio_remove");
        gtk_widget_set_sensitive(widget, TRUE);

        ghb_adjust_audio_rate_combos(ud);
    }
    else
    {
        widget = GHB_WIDGET (ud->builder, "audio_remove");
        gtk_widget_set_sensitive(widget, FALSE);
    }
}

static gboolean
ghb_add_audio_to_settings(GValue *settings, GValue *asettings)
{
    GValue *audio_list;
    const gchar * track;
    int count;

    audio_list = ghb_settings_get_value(settings, "audio_list");
    if (audio_list == NULL)
    {
        audio_list = ghb_array_value_new(8);
        ghb_settings_set_value(settings, "audio_list", audio_list);
    }
    count = ghb_array_len(audio_list);
    // Don't allow more than 99
    // This is a hard limit imposed by libhb/reader.c:GetFifoForId()
    if (count >= 99)
    {
        ghb_value_free(asettings);
        return FALSE;
    }

    int title_no = ghb_settings_get_int(settings, "title_no");
    int track_no = ghb_settings_get_int(asettings, "AudioTrack");
    track = ghb_audio_track_description(track_no, title_no);
    ghb_settings_set_string(asettings, "AudioTrackDescription", track);

    GValue *aname;
    aname = ghb_dict_lookup(asettings, "AudioTrackName");
    if (aname == NULL)
    {
        ghb_settings_set_string(asettings, "AudioTrackName", "");
    }
    if (ghb_array_len(audio_list) >= 99)
    {
        ghb_value_free(asettings);
        return FALSE;
    }
    ghb_array_append(audio_list, asettings);
    return TRUE;
}

G_MODULE_EXPORT void
audio_add_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    // Add the current audio settings to the list.
    GValue *asettings;
    GtkWidget *widget;
    
    g_debug("audio_add_clicked_cb ()");
    asettings = ghb_dict_value_new();
    widget = GHB_WIDGET(ud->builder, "AudioTrack");
    ghb_settings_take_value(asettings, "AudioTrack", ghb_widget_value(widget));
    widget = GHB_WIDGET(ud->builder, "AudioEncoder");
    ghb_settings_take_value(asettings, "AudioEncoder", ghb_widget_value(widget));
    ghb_settings_set_value(asettings, "AudioEncoderActual", 
        ghb_settings_get_value(ud->settings, "AudioEncoderActual"));
    widget = GHB_WIDGET(ud->builder, "AudioTrackQualityEnable");
    ghb_settings_take_value(asettings, "AudioTrackQualityEnable", ghb_widget_value(widget));
    widget = GHB_WIDGET(ud->builder, "AudioTrackQuality");
    ghb_settings_take_value(asettings, "AudioTrackQuality", ghb_widget_value(widget));
    widget = GHB_WIDGET(ud->builder, "AudioBitrate");
    ghb_settings_take_value(asettings, "AudioBitrate", ghb_widget_value(widget));
    widget = GHB_WIDGET(ud->builder, "AudioSamplerate");
    ghb_settings_take_value(asettings, "AudioSamplerate", ghb_widget_value(widget));
    widget = GHB_WIDGET(ud->builder, "AudioMixdown");
    ghb_settings_take_value(asettings, "AudioMixdown", ghb_widget_value(widget));
    widget = GHB_WIDGET(ud->builder, "AudioTrackGain");
    ghb_settings_take_value(asettings, "AudioTrackGain", ghb_widget_value(widget));
    widget = GHB_WIDGET(ud->builder, "AudioTrackDRCSlider");
    ghb_settings_take_value(asettings, "AudioTrackDRCSlider", ghb_widget_value(widget));

    if (!ghb_add_audio_to_settings(ud->settings, asettings))
        return;

    ghb_add_audio_to_ui(ud->builder, asettings);
    check_list_full(ud);
}

G_MODULE_EXPORT void
audio_remove_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GtkTreeView *treeview;
    GtkTreePath *treepath;
    GtkTreeSelection *selection;
    GtkTreeModel *store;
    GtkTreeIter iter, nextIter;
    gint *indices;
    gint row;
    GValue *audio_list;

    g_debug("audio_remove_clicked_cb ()");
    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
    selection = gtk_tree_view_get_selection (treeview);
    if (gtk_tree_selection_get_selected(selection, &store, &iter))
    {
        nextIter = iter;
        if (!gtk_tree_model_iter_next(store, &nextIter))
        {
            nextIter = iter;
            if (gtk_tree_model_get_iter_first(store, &nextIter))
            {
                gtk_tree_selection_select_iter (selection, &nextIter);
            }
        }
        else
        {
            gtk_tree_selection_select_iter (selection, &nextIter);
        }
        // Get the row number
        treepath = gtk_tree_model_get_path (store, &iter);
        indices = gtk_tree_path_get_indices (treepath);
        row = indices[0];
        gtk_tree_path_free(treepath);
        if (row < 0) return;

        audio_list = ghb_settings_get_value(ud->settings, "audio_list");
        if (row >= ghb_array_len(audio_list))
            return;

        // Update our settings list before removing the row from the
        // treeview.  Removing from the treeview sometimes provokes an
        // immediate selection change, so the list needs to be up to date
        // when this happens.
        GValue *old = ghb_array_get_nth(audio_list, row);
        ghb_value_free(old);
        ghb_array_remove(audio_list, row);

        // Remove the selected item
        gtk_list_store_remove (GTK_LIST_STORE(store), &iter);
        // remove from audio settings list
        widget = GHB_WIDGET (ud->builder, "audio_add");
        gtk_widget_set_sensitive(widget, TRUE);
    }
}

void
ghb_set_audio(signal_user_data_t *ud, GValue *settings)
{
    gint acodec_code;

    GValue *alist;
    GValue *track, *audio, *acodec, *acodec_actual, *bitrate, *rate, 
            *mix, *drc, *gain, *quality, *enable_quality;
    gint count, ii;
    
    g_debug("set_audio");
    // Clear the audio list
    ghb_clear_audio_list_settings(ud->settings);
    ghb_clear_audio_list_ui(ud->builder);
    alist = ghb_settings_get_value(settings, "audio_list");

    count = ghb_array_len(alist);
    for (ii = 0; ii < count; ii++)
    {
        audio = ghb_array_get_nth(alist, ii);
        track = ghb_settings_get_value(audio, "AudioTrack");
        acodec = ghb_settings_get_value(audio, "AudioEncoder");
        acodec_actual = ghb_settings_get_value(audio, "AudioEncoderActual");
        enable_quality = ghb_settings_get_value(audio, "AudioTrackQualityEnable");
        quality = ghb_settings_get_value(audio, "AudioTrackQuality");
        bitrate = ghb_settings_get_value(audio, "AudioBitrate");
        rate = ghb_settings_get_value(audio, "AudioSamplerate");
        mix = ghb_settings_get_value(audio, "AudioMixdown");
        gain = ghb_settings_get_value(audio, "AudioTrackGain");
        drc = ghb_settings_get_value(audio, "AudioTrackDRCSlider");
        acodec_code = ghb_lookup_combo_int("AudioEncoder", acodec);

        if (acodec_code != 0)
        {
            GValue *asettings = ghb_dict_value_new();
            ghb_settings_set_value(asettings, "AudioTrack", track);
            ghb_settings_set_value(asettings, "AudioEncoder", acodec);
            ghb_settings_set_value(asettings, "AudioEncoderActual", acodec_actual);
            ghb_settings_set_value(asettings, "AudioTrackQualityEnable", enable_quality);
            ghb_settings_set_value(asettings, "AudioTrackQuality", quality);

            // This gets set autimatically if the codec is passthru
            ghb_settings_set_value(asettings, "AudioBitrate", bitrate);
            ghb_settings_set_value(asettings, "AudioSamplerate", rate);
            ghb_settings_set_value(asettings, "AudioMixdown", mix);
            ghb_settings_set_value(asettings, "AudioTrackGain", gain);
            ghb_settings_set_value(asettings, "AudioTrackDRCSlider", drc);

            ghb_add_audio_to_settings(ud->settings, asettings);

            ghb_add_audio_to_ui(ud->builder, asettings);
            ghb_adjust_audio_rate_combos(ud);
        }
    }
    check_list_full(ud);
}

