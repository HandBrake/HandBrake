/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * audiohandler.c
 * Copyright (C) John Stebbins 2008 <stebbins@stebbins>
 * 
 * audiohandler.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <gtk/gtk.h>
#include "hb.h"
#include "settings.h"
#include "hb-backend.h"
#include "values.h"
#include "callbacks.h"
#include "preview.h"
#include "audiohandler.h"

static void ghb_add_audio(signal_user_data_t *ud, GValue *settings);

void
ghb_adjust_audio_rate_combos(signal_user_data_t *ud)
{
	gint titleindex, track, acodec, select_acodec, mix;
	ghb_audio_info_t ainfo;
	GtkWidget *widget;
	GValue *gval;
	int mux;
	gint bitrate;
	
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

	select_acodec = acodec;
	if (mux == HB_MUX_MP4)
	{
		select_acodec &= ~HB_ACODEC_DCA;
	}
	if ((select_acodec & HB_ACODEC_MASK) == 0)
	{
		// Unsuported codec in this container.
		select_acodec |= HB_ACODEC_AC3;
		acodec = select_acodec;
	}

	if (ghb_audio_is_passthru (select_acodec))
	{
		ghb_set_default_bitrate_opts (ud->builder, 0, -1);
		if (ghb_get_audio_info (&ainfo, titleindex, track))
		{
			bitrate = ainfo.bitrate / 1000;

			// Set the values for bitrate and samplerate to the input rates
			if (ainfo.codec & select_acodec & HB_ACODEC_PASS_MASK)
			{
				ghb_set_passthru_bitrate_opts (ud->builder, bitrate);
				ghb_ui_update(ud, "AudioMixdown", ghb_int64_value(0));
				select_acodec &= ainfo.codec | HB_ACODEC_PASS_FLAG;
			}
			else
			{
				select_acodec = ghb_select_audio_codec(ud->settings, acodec, track);
				if (acodec != HB_ACODEC_ANY)
				{
					ghb_ui_update(ud, "AudioEncoder", ghb_int64_value(select_acodec));
				}

				int channels;
				mix = ghb_get_best_mix( titleindex, track, select_acodec, mix);
				channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(mix);
				bitrate = ghb_get_default_audio_bitrate(select_acodec, ainfo.samplerate, bitrate, channels);
				ghb_ui_update(ud, "AudioMixdown", ghb_int64_value(mix));
			}
			ghb_ui_update(ud, "AudioBitrate", ghb_int64_value(bitrate));
			ghb_ui_update(ud, "AudioSamplerate", ghb_int64_value(0));
		}
		else
		{
			ghb_ui_update(ud, "AudioSamplerate", ghb_int64_value(0));
			ghb_ui_update(ud, "AudioMixdown", ghb_int64_value(0));
			bitrate = 448;
			mix = ghb_get_best_mix( titleindex, track, select_acodec, 0);
		}
		ghb_ui_update(ud, "AudioTrackDRCSlider", ghb_double_value(0));
	}
	gint channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(mix);
	bitrate = ghb_get_best_audio_bitrate(select_acodec, bitrate, channels);
	ghb_ui_update(ud, "AudioBitrate", ghb_int64_value(bitrate));

	int low, high;
	ghb_get_audio_bitrate_limits(select_acodec, channels, &low, &high);
	ghb_set_default_bitrate_opts (ud->builder, low, high);

	ghb_settings_take_value(ud->settings, "AudioEncoderActual", 
							ghb_lookup_acodec_value(select_acodec));
	ghb_check_dependency(ud, NULL, "AudioEncoderActual");
}

static void
free_audio_hash_key_value(gpointer data)
{
	g_free(data);
}

gchar*
ghb_get_user_audio_lang(signal_user_data_t *ud, gint titleindex, gint track)
{
	GValue *audio_list, *asettings;
	gchar *lang = NULL;

	audio_list = ghb_settings_get_value(ud->settings, "audio_list");
	if (ghb_array_len(audio_list) <= track)
		return NULL;
	asettings = ghb_array_get_nth(audio_list, track);
	track = ghb_settings_get_int(asettings, "AudioTrack");
	lang = ghb_get_source_audio_lang(titleindex, track);
	return lang;
}

void
ghb_set_pref_audio(gint titleindex, signal_user_data_t *ud)
{
	gint fallback_acodec, track;
	gchar *source_lang = NULL;
	GtkWidget *button;
	ghb_audio_info_t ainfo;
	GHashTable *track_indices;
	gint mux;

	const GValue *pref_audio;
	const GValue *audio, *drc;
	gint mix_acodec, acodec, bitrate, mix;
	gdouble rate;
	gint count, ii, list_count;
	
	g_debug("set_pref_audio");
	mux = ghb_settings_combo_int(ud->settings, "FileFormat");
	if (mux == HB_MUX_MP4)
	{
		fallback_acodec = HB_ACODEC_FAAC;
	}
	else
	{
		fallback_acodec = HB_ACODEC_LAME;
	}
	track_indices = g_hash_table_new_full(g_int_hash, g_int_equal, 
						free_audio_hash_key_value, free_audio_hash_key_value);
	// Clear the audio list
	ghb_clear_audio_list(ud);
	// Find "best" audio based on audio preferences
	button = GHB_WIDGET (ud->builder, "audio_add");
	if (!ghb_settings_get_boolean(ud->settings, "AudioDUB"))
	{
		source_lang = ghb_get_source_audio_lang(titleindex, 0);
	}
	if (source_lang == NULL)
		source_lang = ghb_settings_get_string(ud->settings, "PreferredLanguage");

	pref_audio = ghb_settings_get_value(ud->settings, "AudioList");

	list_count = 0;
	count = ghb_array_len(pref_audio);
	for (ii = 0; ii < count; ii++)
	{
		int select_acodec;

		audio = ghb_array_get_nth(pref_audio, ii);
		select_acodec = acodec = ghb_settings_combo_int(audio, "AudioEncoder");
		if (mux == HB_MUX_MP4)
		{
			select_acodec &= ~HB_ACODEC_DCA;
		}
		if ((select_acodec & HB_ACODEC_MASK) == 0)
		{
			// Unsuported codec in this container.
			select_acodec |= HB_ACODEC_AC3;
			acodec = select_acodec;
		}
		if ( ghb_audio_can_passthru( select_acodec ) )
		{
			fallback_acodec = HB_ACODEC_AC3;
		}
		mix_acodec = acodec;
		bitrate = ghb_settings_combo_int(audio, "AudioBitrate");
		rate = ghb_settings_combo_double(audio, "AudioSamplerate");
		mix = ghb_settings_combo_int(audio, "AudioMixdown");
		drc = ghb_settings_get_value(audio, "AudioTrackDRCSlider");
		// If there are multiple audios using the same codec, then
		// select sequential tracks for each.  The hash keeps track 
		// of the tracks used for each codec.
		track = ghb_find_audio_track(titleindex, source_lang, 
								select_acodec, fallback_acodec, track_indices);
		// Check to see if:
		// 1. pref codec is passthru
		// 2. source codec is not passthru
		// 3. next pref is enabled
		if (ghb_get_audio_info (&ainfo, titleindex, track) && 
			ghb_audio_is_passthru (acodec))
		{
			// HB_ACODEC_* are bit fields.  Treat acodec as mask
			if (!(ainfo.codec & select_acodec & HB_ACODEC_PASS_MASK))
			{
				if (acodec != HB_ACODEC_ANY)
					acodec = fallback_acodec;
				mix_acodec = fallback_acodec;
				// If we can't substitute the passthru with a suitable
				// encoder and
				// If there's more audio to process, or we've already
				// placed one in the list, then we can skip this one
				if (!(select_acodec & fallback_acodec) && 
					((ii + 1 < count) || (list_count != 0)))
				{
					// Skip this audio
					acodec = 0;
				}
				else
				{
					int channels, min_rate, max_rate;
					mix = ghb_get_best_mix( titleindex, track, mix_acodec, mix);
					channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(mix);
					bitrate = ainfo.bitrate / 1000;
					min_rate = channels * 64;
					max_rate = channels * 160;
					if (bitrate < min_rate)
						bitrate = min_rate;
					if (bitrate > max_rate)
						bitrate = max_rate;
					rate = 0;
				}
			}
		}
		if (titleindex >= 0 && track < 0)
			acodec = 0;
		if (acodec != 0)
		{
			GValue *settings = ghb_dict_value_new();
			ghb_settings_set_int(settings, "AudioTrack", track);
			ghb_settings_set_string(settings, "AudioEncoder", 
				ghb_lookup_combo_string("AudioEncoder", ghb_int_value(acodec)));
			ghb_settings_set_value(settings, "AudioEncoderActual", 
				ghb_settings_get_value(ud->settings, "AudioEncoderActual"));
			// This gets set autimatically if the codec is passthru
			ghb_settings_set_string(settings, "AudioBitrate",
				ghb_lookup_combo_string("AudioBitrate", ghb_int_value(bitrate)));
			ghb_settings_set_string(settings, "AudioSamplerate",
				ghb_lookup_combo_string("AudioSamplerate", ghb_int_value(rate)));
			mix = ghb_get_best_mix( titleindex, track, mix_acodec, mix);
			ghb_settings_set_string(settings, "AudioMixdown",
				ghb_lookup_combo_string("AudioMixdown", ghb_int_value(mix)));
			ghb_settings_set_value(settings, "AudioTrackDRCSlider", drc);
			ghb_add_audio(ud, settings);
			ghb_adjust_audio_rate_combos(ud);
		}
	}
	g_free(source_lang);
	g_hash_table_destroy(track_indices);
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
		const gchar *track, *codec, *br, *sr, *mix;
		gchar *s_drc;
		gint itrack, icodec;
		gdouble drc;
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
		itrack = ghb_settings_combo_int(asettings, "AudioTrack");
		codec = ghb_settings_combo_option(asettings, "AudioEncoder");
		icodec = ghb_settings_combo_int(asettings, "AudioEncoder");
		br = ghb_settings_combo_option(asettings, "AudioBitrate");
		sr = ghb_settings_combo_option(asettings, "AudioSamplerate");
		mix = ghb_settings_combo_option(asettings, "AudioMixdown");

		drc = ghb_settings_get_double(asettings, "AudioTrackDRCSlider");
		if (drc < 1.0)
			s_drc = g_strdup("Off");
		else
			s_drc = g_strdup_printf("%.1f", drc);

		if (icodec == HB_ACODEC_ANY)
			codec = ghb_select_audio_codec_str(ud->settings, icodec, itrack);

		gtk_list_store_set(GTK_LIST_STORE(store), &iter, 
			// These are displayed in list
			0, track,
			1, codec,
			2, br,
			3, sr,
			4, mix,
			5, s_drc,
			-1);
		g_free(s_drc);
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
			sr = ghb_settings_get_int(asettings, "AudioSamplerate");
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

		mix_code = ghb_get_best_mix( titleindex, track, acodec_code, mix_code);
		int channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(mix_code);
		br = ghb_get_best_audio_bitrate(acodec_code, br, channels);
		ghb_ui_update(ud, "AudioBitrate", ghb_int64_value(br));

		sr = ghb_find_closest_audio_rate(sr);
		ghb_ui_update(ud, "AudioSamplerate", ghb_int64_value(sr));
		ghb_ui_update(ud, "AudioMixdown", ghb_int64_value(mix_code));
	}
	ghb_adjust_audio_rate_combos(ud);
	ghb_grey_combo_options (ud->builder);
	ghb_check_dependency(ud, widget, NULL);
	prev_acodec = acodec_code;
	if (asettings != NULL)
	{
		ghb_widget_to_setting(asettings, widget);
		ghb_settings_set_value(asettings, "AudioEncoderActual", ghb_settings_get_value(ud->settings, "AudioEncoderActual"));
		ghb_audio_list_refresh_selected(ud);
	}
	ghb_update_destination_extension(ud);
	ghb_live_reset(ud);
}

G_MODULE_EXPORT void
audio_track_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	GValue *asettings;

	g_debug("audio_track_changed_cb ()");
	ghb_adjust_audio_rate_combos(ud);
	ghb_check_dependency(ud, widget, NULL);
	ghb_grey_combo_options(ud->builder);
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
drc_widget_changed_cb(GtkWidget *widget, gdouble val, signal_user_data_t *ud)
{
	GValue *asettings;
	GtkLabel *label;
	gchar *drc;

	g_debug("drc_widget_changed_cb ()");

	label = GTK_LABEL(GHB_WIDGET(ud->builder, "drc_label"));
	if (val < 1.0)
		drc = g_strdup_printf("Off");
	else
		drc = g_strdup_printf("%.1f", val);
	gtk_label_set_text(label, drc);
	g_free(drc);
	ghb_check_dependency(ud, widget, NULL);
	asettings = get_selected_asettings(ud);
	if (asettings != NULL)
	{
		ghb_widget_to_setting(asettings, widget);
		ghb_audio_list_refresh_selected(ud);
	}
	ghb_live_reset(ud);
}

void
ghb_clear_audio_list(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkListStore *store;
	GValue *audio_list;
	
	g_debug("clear_audio_list ()");
	audio_list = ghb_settings_get_value(ud->settings, "audio_list");
	if (audio_list == NULL)
	{
		audio_list = ghb_array_value_new(8);
		ghb_settings_set_value(ud->settings, "audio_list", audio_list);
	}
	else
		ghb_array_value_reset(audio_list, 8);
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	gtk_list_store_clear (store);
}

static void
add_to_audio_list(signal_user_data_t *ud, GValue *settings)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	GtkTreeSelection *selection;
	const gchar *track, *codec, *br, *sr, *mix;
	gchar *s_drc;
	gint icodec, itrack;
	gdouble drc;
	
	g_debug("add_to_audio_list ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "audio_list"));
	selection = gtk_tree_view_get_selection (treeview);
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

	track = ghb_settings_combo_option(settings, "AudioTrack");
	itrack = ghb_settings_combo_int(settings, "AudioTrack");
	codec = ghb_settings_combo_option(settings, "AudioEncoder");
	icodec = ghb_settings_combo_int(settings, "AudioEncoder");
	br = ghb_settings_combo_option(settings, "AudioBitrate");
	sr = ghb_settings_combo_option(settings, "AudioSamplerate");
	mix = ghb_settings_combo_option(settings, "AudioMixdown");

	drc = ghb_settings_get_double(settings, "AudioTrackDRCSlider");
	if (drc < 1.0)
		s_drc = g_strdup("Off");
	else
		s_drc = g_strdup_printf("%.1f", drc);

	if (icodec == HB_ACODEC_ANY)
	{
		codec = ghb_select_audio_codec_str(ud->settings, icodec, itrack);
	}

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
		// These are displayed in list
		0, track,
		1, codec,
		2, br,
		3, sr,
		4, mix,
		5, s_drc,
		-1);
	gtk_tree_selection_select_iter(selection, &iter);
	g_free(s_drc);
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

		ghb_ui_update(ud, "AudioTrack", ghb_settings_get_value(asettings, "AudioTrack"));
		ghb_ui_update(ud, "AudioEncoder", ghb_settings_get_value(asettings, "AudioEncoder"));
		ghb_settings_set_value(ud->settings, "AudioEncoderActual", ghb_settings_get_value(asettings, "AudioEncoderActual"));
		ghb_check_dependency(ud, NULL, "AudioEncoderActual");
		ghb_ui_update(ud, "AudioBitrate", ghb_settings_get_value(asettings, "AudioBitrate"));
		ghb_ui_update(ud, "AudioSamplerate", ghb_settings_get_value(asettings, "AudioSamplerate"));
		ghb_ui_update(ud, "AudioMixdown", ghb_settings_get_value(asettings, "AudioMixdown"));
		ghb_ui_update(ud, "AudioTrackDRCSlider", ghb_settings_get_value(asettings, "AudioTrackDRCSlider"));
		widget = GHB_WIDGET (ud->builder, "audio_remove");
		gtk_widget_set_sensitive(widget, TRUE);
	}
	else
	{
		widget = GHB_WIDGET (ud->builder, "audio_remove");
		gtk_widget_set_sensitive(widget, FALSE);
	}
}

static void
ghb_add_audio(signal_user_data_t *ud, GValue *settings)
{
	GValue *audio_list;
	int count;
	const gchar * track;

	track = ghb_settings_combo_option(settings, "AudioTrack");
	ghb_settings_set_string(settings, "AudioTrackDescription", track);

	audio_list = ghb_settings_get_value(ud->settings, "audio_list");
	if (audio_list == NULL)
	{
		audio_list = ghb_array_value_new(8);
		ghb_settings_set_value(ud->settings, "audio_list", audio_list);
	}
	ghb_array_append(audio_list, settings);
	add_to_audio_list(ud, settings);
	count = ghb_array_len(audio_list);
	if (count >= 99)
	{
		GtkWidget * widget = GHB_WIDGET(ud->builder, "audio_add");
		gtk_widget_set_sensitive(widget, FALSE);
	}
	ghb_update_destination_extension(ud);
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
	widget = GHB_WIDGET(ud->builder, "AudioBitrate");
	ghb_settings_take_value(asettings, "AudioBitrate", ghb_widget_value(widget));
	widget = GHB_WIDGET(ud->builder, "AudioSamplerate");
	ghb_settings_take_value(asettings, "AudioSamplerate", ghb_widget_value(widget));
	widget = GHB_WIDGET(ud->builder, "AudioMixdown");
	ghb_settings_take_value(asettings, "AudioMixdown", ghb_widget_value(widget));
	widget = GHB_WIDGET(ud->builder, "AudioTrackDRCSlider");
	ghb_settings_take_value(asettings, "AudioTrackDRCSlider", ghb_widget_value(widget));

	ghb_add_audio(ud, asettings);
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
		// Remove the selected item
		gtk_list_store_remove (GTK_LIST_STORE(store), &iter);
		// remove from audio settings list
		if (row < 0) return;
		widget = GHB_WIDGET (ud->builder, "audio_add");
		gtk_widget_set_sensitive(widget, TRUE);
		audio_list = ghb_settings_get_value(ud->settings, "audio_list");
		if (row >= ghb_array_len(audio_list))
			return;
		GValue *old = ghb_array_get_nth(audio_list, row);
		ghb_value_free(old);
		ghb_array_remove(audio_list, row);
	}
}

void
ghb_set_audio(signal_user_data_t *ud, GValue *settings)
{
	gint acodec_code;
	GtkWidget *button;

	GValue *alist;
	GValue *track, *audio, *acodec, *bitrate, *rate, *mix, *drc;
	gint count, ii;
	
	g_debug("set_audio");
	// Clear the audio list
	ghb_clear_audio_list(ud);
	button = GHB_WIDGET (ud->builder, "audio_add");
	alist = ghb_settings_get_value(settings, "audio_list");

	count = ghb_array_len(alist);
	for (ii = 0; ii < count; ii++)
	{
		audio = ghb_array_get_nth(alist, ii);
		track = ghb_settings_get_value(audio, "AudioTrack");
		acodec = ghb_settings_get_value(audio, "AudioEncoder");
		bitrate = ghb_settings_get_value(audio, "AudioBitrate");
		rate = ghb_settings_get_value(audio, "AudioSamplerate");
		mix = ghb_settings_get_value(audio, "AudioMixdown");
		drc = ghb_settings_get_value(audio, "AudioTrackDRCSlider");
		acodec_code = ghb_lookup_combo_int("AudioEncoder", acodec);

		if (acodec_code != 0)
		{
			GValue *settings = ghb_dict_value_new();
			ghb_settings_set_value(settings, "AudioTrack", track);
			ghb_settings_set_value(settings, "AudioEncoder", acodec);
			ghb_settings_set_value(settings, "AudioEncoderActual", 
				ghb_settings_get_value(ud->settings, "AudioEncoderActual"));
			// This gets set autimatically if the codec is passthru
			ghb_settings_set_value(settings, "AudioBitrate", bitrate);
			ghb_settings_set_value(settings, "AudioSamplerate", rate);
			ghb_settings_set_value(settings, "AudioMixdown", mix);
			ghb_settings_set_value(settings, "AudioTrackDRCSlider", drc);
			ghb_add_audio(ud, settings);
			ghb_adjust_audio_rate_combos(ud);
		}
	}
}

