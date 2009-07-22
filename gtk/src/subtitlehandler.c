/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * subtitlehandler.c
 * Copyright (C) John Stebbins 2008 <stebbins@stebbins>
 * 
 * subtitlehandler.c is free software.
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
#include "presets.h"
#include "audiohandler.h"
#include "subtitlehandler.h"

static void add_to_subtitle_list(signal_user_data_t *ud, GValue *settings);
static void add_to_srt_list(signal_user_data_t *ud, GValue *settings);

static void
free_subtitle_index_list(gpointer data)
{
	g_free(data);
}

static void
free_subtitle_key(gpointer data)
{
	if (data != NULL)
		g_free(data);
}

static gboolean
mustBurn(signal_user_data_t *ud, gint track)
{
	gint mux;

	mux = ghb_settings_combo_int(ud->settings, "FileFormat");
	if (mux == HB_MUX_MP4)
	{
		gint source;

		// MP4 can only handle burned vobsubs.  make sure there isn't
		// already something burned in the list
		source = ghb_subtitle_track_source(ud, track);
		if (source == VOBSUB)
		{
			return TRUE;
		}
	}
	return FALSE;
}

void
ghb_subtitle_exclusive_burn(signal_user_data_t *ud, gint index)
{
	GValue *subtitle_list;
	GValue *settings;
	gint ii, count, tt;
	GtkTreeView  *tv;
	GtkTreeModel *tm;
	GtkTreeIter   ti;
	gboolean burned;

	g_debug("ghb_subtitle_exclusive_burn");
	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	count = ghb_array_len(subtitle_list);
	for (ii = 0; ii < count; ii++)
	{
		settings = ghb_array_get_nth(subtitle_list, ii);
		tt = ghb_settings_combo_int(settings, "SubtitleTrack");
		burned = ghb_settings_get_boolean(settings, "SubtitleBurned");

		tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
		g_return_if_fail(tv != NULL);
		tm = gtk_tree_view_get_model(tv);
		gtk_tree_model_iter_nth_child(tm, &ti, NULL, ii);
		if (burned && ii != index && !mustBurn(ud, tt))
		{
			ghb_settings_set_boolean(settings, "SubtitleBurned", FALSE);
			gtk_list_store_set(GTK_LIST_STORE(tm), &ti, 2, FALSE, -1);
		}
	}
}

void
ghb_subtitle_exclusive_default(signal_user_data_t *ud, gint index)
{
	GValue *subtitle_list;
	GValue *settings;
	gint ii, count;
	GtkTreeView  *tv;
	GtkTreeModel *tm;
	GtkTreeIter   ti;
	gboolean def;

	g_debug("ghb_subtitle_exclusive_default");
	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	count = ghb_array_len(subtitle_list);
	for (ii = 0; ii < count; ii++)
	{
		settings = ghb_array_get_nth(subtitle_list, ii);
		def = ghb_settings_get_boolean(settings, "SubtitleDefaultTrack");

		tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
		g_return_if_fail(tv != NULL);
		tm = gtk_tree_view_get_model(tv);
		gtk_tree_model_iter_nth_child(tm, &ti, NULL, ii);
		if (def && ii != index)
		{

			ghb_settings_set_boolean(settings, "SubtitleDefaultTrack", FALSE);
			gtk_list_store_set(GTK_LIST_STORE(tm), &ti, 3, FALSE, -1);
		}
	}
}

void
ghb_add_srt(signal_user_data_t *ud, GValue *settings)
{
	// Add the current subtitle settings to the list.
	GValue *subtitle_list;
	gint count;
	const gchar *lang;
	
	g_debug("ghb_add_srt ()");

	// Add the long track description so the queue can access it
	// when a different title is selected.
	lang = ghb_settings_combo_option(settings, "SrtLanguage");
	ghb_settings_set_string(settings, "SubtitleTrackDescription", lang);

	ghb_settings_set_int(settings, "SubtitleSource", SRTSUB);

	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	if (subtitle_list == NULL)
	{
		subtitle_list = ghb_array_value_new(8);
		ghb_settings_set_value(ud->settings, "subtitle_list", subtitle_list);
	}
	count = ghb_array_len(subtitle_list);

	// Don't allow more than 99
	// This is a had limit imposed by libhb/sync.c:GetFifoForId()
	if (count >= 99)
	{
		ghb_value_free(settings);
		return;
	}

	ghb_array_append(subtitle_list, settings);
	add_to_srt_list(ud, settings);

	if (count == 98)
	{
		GtkWidget *widget;
		widget = GHB_WIDGET (ud->builder, "subtitle_add");
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GHB_WIDGET (ud->builder, "srt_add");
		gtk_widget_set_sensitive(widget, FALSE);
	}
	ghb_live_reset(ud);
}

void
ghb_add_subtitle(signal_user_data_t *ud, GValue *settings)
{
	// Add the current subtitle settings to the list.
	GValue *subtitle_list;
	gint count;
	gboolean burned;
	const gchar *track;
	const gchar *lang;
	gint tt, source;
	
	g_debug("ghb_add_subtitle ()");

	// Add the long track description so the queue can access it
	// when a different title is selected.
	track = ghb_settings_combo_option(settings, "SubtitleTrack");
	ghb_settings_set_string(settings, "SubtitleTrackDescription", track);

	lang = ghb_settings_combo_string(settings, "SubtitleTrack");
	ghb_settings_set_string(settings, "SubtitleLanguage", lang);

	tt = ghb_settings_get_int(settings, "SubtitleTrack");
	source = ghb_subtitle_track_source(ud, tt);
	ghb_settings_set_int(settings, "SubtitleSource", source);

	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	if (subtitle_list == NULL)
	{
		subtitle_list = ghb_array_value_new(8);
		ghb_settings_set_value(ud->settings, "subtitle_list", subtitle_list);
	}
	count = ghb_array_len(subtitle_list);

	// Don't allow more than 99
	// This is a had limit imposed by libhb/sync.c:GetFifoForId()
	if (count >= 99)
	{
		ghb_value_free(settings);
		return;
	}

	ghb_array_append(subtitle_list, settings);
	add_to_subtitle_list(ud, settings);

	burned = ghb_settings_get_boolean(settings, "SubtitleBurned");
	if (burned)
		ghb_subtitle_exclusive_burn(ud, count);
	if (count == 98)
	{
		GtkWidget *widget;
		widget = GHB_WIDGET (ud->builder, "subtitle_add");
		gtk_widget_set_sensitive(widget, FALSE);
		widget = GHB_WIDGET (ud->builder, "srt_add");
		gtk_widget_set_sensitive(widget, FALSE);
	}
	ghb_live_reset(ud);
}

static void
add_all_pref_subtitles(signal_user_data_t *ud)
{
	const GValue *pref_subtitle;
	GValue *subtitle;
	gint count, ii, track;
	char *lang;

	pref_subtitle = ghb_settings_get_value(ud->settings, "SubtitleList");
	count = ghb_array_len(pref_subtitle);
	for (ii = 0; ii < count; ii++)
	{
		subtitle = ghb_value_dup(ghb_array_get_nth(pref_subtitle, ii));
		lang = ghb_settings_get_string(subtitle, "SubtitleLanguage");
		// If there are multiple subtitles using the same language, then
		// select sequential tracks for each.  The hash keeps track 
		// of the tracks used for each language.
		track = ghb_find_pref_subtitle_track(lang);
		g_free(lang);
		if (track >= -1)
		{
			// Add to subtitle list
			ghb_settings_set_int(subtitle, "SubtitleTrack", track);
			ghb_add_subtitle(ud, subtitle);
		}
	}
}

void
ghb_set_pref_subtitle(gint titleindex, signal_user_data_t *ud)
{
	gint track;
	GHashTable *track_indices;
	gchar *lang, *pref_lang = NULL;
	gchar *audio_lang;
	gint foreign_lang_index = -1;
	gboolean found_cc = FALSE;

	const GValue *pref_subtitle;
	GValue *subtitle;
	gint count, ii, jj;
	
	g_debug("ghb_set_pref_subtitle %d", titleindex);

	// Check to see if we need to add a subtitle track for foreign audio
	// language films. A subtitle track will be added if:
	//
	// The first (default) audio track language does NOT match the users
	// chosen Preferred Language AND the Preferred Language is NOT Any (und).
	//
	audio_lang = ghb_get_user_audio_lang(ud, titleindex, 0);
	pref_lang = ghb_settings_get_string(ud->settings, "PreferredLanguage");

	if (audio_lang != NULL && pref_lang != NULL &&
		(strcmp(audio_lang, pref_lang) == 0 || strcmp("und", pref_lang) == 0))
	{
		g_free(pref_lang);
		pref_lang = NULL;
	}

	track_indices = g_hash_table_new_full(g_str_hash, g_str_equal, 
											free_subtitle_key, free_subtitle_index_list);

	ghb_ui_update(ud, "SubtitleTrack", ghb_int_value(0));

	// Clear the subtitle list
	ghb_clear_subtitle_list(ud);
	if (titleindex < 0)
	{
		add_all_pref_subtitles(ud);
		return;
	}

	// Find "best" subtitle based on subtitle preferences
	pref_subtitle = ghb_settings_get_value(ud->settings, "SubtitleList");

	count = ghb_array_len(pref_subtitle);
	jj = 0;
	for (ii = 0; ii < count; ii++)
	{
		subtitle = ghb_array_get_nth(pref_subtitle, ii);
		lang = ghb_settings_get_string(subtitle, "SubtitleLanguage");
		// If there are multiple subtitles using the same language, then
		// select sequential tracks for each.  The hash keeps track 
		// of the tracks used for each language.
		track = ghb_find_subtitle_track(titleindex, lang, track_indices);
		g_free(lang);
		if (track >= -1)
		{
			gint source;
			GValue *dup = ghb_value_dup(subtitle);
			lang = ghb_subtitle_track_lang(ud, track);
			ghb_settings_set_int(dup, "SubtitleTrack", track);
			if (foreign_lang_index < 0 && pref_lang != NULL &&
				strcmp(lang, pref_lang) == 0)
			{
				foreign_lang_index = jj;
				ghb_settings_take_value(dup, "SubtitleForced", 
								ghb_boolean_value_new(FALSE));
				ghb_settings_take_value(dup, "SubtitleDefaultTrack", 
								ghb_boolean_value_new(TRUE));
			}
			source = ghb_subtitle_track_source(ud, track);
			if (source == CC608SUB || source == CC708SUB)
				found_cc = TRUE;
			ghb_add_subtitle(ud, dup);
			jj++;
			g_free(lang);
		}
	}
	if (foreign_lang_index < 0 && pref_lang != NULL)
	{
		// Subtitle for foreign language audio not added yet
		GValue *settings;
		gboolean burn;

		track = ghb_find_subtitle_track(titleindex, pref_lang, track_indices);
		if (track >= -1)
		{
			burn = mustBurn(ud, track);
			settings = ghb_dict_value_new();
			ghb_settings_set_int(settings, "SubtitleTrack", track);
			ghb_settings_take_value(settings, "SubtitleForced", 
							ghb_boolean_value_new(FALSE));
			ghb_settings_take_value(settings, "SubtitleBurned", 
							ghb_boolean_value_new(burn));
			ghb_settings_take_value(settings, "SubtitleDefaultTrack", 
							ghb_boolean_value_new(TRUE));

			ghb_add_subtitle(ud, settings);
			foreign_lang_index = jj;
		}
	}
	if (foreign_lang_index >= 0)
	{
		GValue *subtitle_list;
		gboolean burn, def;

		subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
		subtitle = ghb_array_get_nth(subtitle_list, foreign_lang_index);

		burn = ghb_settings_get_boolean(subtitle, "SubtitleBurned");
		def = ghb_settings_get_boolean(subtitle, "SubtitleDefaultTrack");
		if (burn)
			ghb_subtitle_exclusive_burn(ud, foreign_lang_index);
		if (def)
			ghb_subtitle_exclusive_default(ud, foreign_lang_index);
		ghb_log("adding subtitle for foreign language audio: %s", audio_lang);
	}
	if (ghb_settings_get_boolean(ud->settings, "AddCC") && !found_cc)
	{
		// Subtitle for foreign language audio not added yet
		GValue *settings;

		track = ghb_find_cc_track(titleindex);
		if (track >= 0)
		{
			settings = ghb_dict_value_new();
			ghb_settings_set_int(settings, "SubtitleTrack", track);
			ghb_settings_take_value(settings, "SubtitleForced", 
							ghb_boolean_value_new(FALSE));
			ghb_settings_take_value(settings, "SubtitleBurned", 
							ghb_boolean_value_new(FALSE));
			ghb_settings_take_value(settings, "SubtitleDefaultTrack", 
							ghb_boolean_value_new(FALSE));

			ghb_add_subtitle(ud, settings);
			ghb_log("adding Closed Captions: %s", audio_lang);
		}
	}
	if (pref_lang != NULL)
		g_free(pref_lang);
	if (audio_lang != NULL)
		g_free(audio_lang);
	g_hash_table_destroy(track_indices);
}

gint
ghb_selected_subtitle_row(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gint *indices;
	gint row = -1;
	
	g_debug("ghb_selected_subtitle_row ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		// Get the row number
		treepath = gtk_tree_model_get_path (store, &iter);
		indices = gtk_tree_path_get_indices (treepath);
		row = indices[0];
		gtk_tree_path_free(treepath);
	}
	return row;
}

GValue*
ghb_selected_subtitle_settings(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gint *indices;
	gint row;
	GValue *settings = NULL;
	const GValue *subtitle_list;
	
	g_debug("get_selected_settings ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		// Get the row number
		treepath = gtk_tree_model_get_path (store, &iter);
		indices = gtk_tree_path_get_indices (treepath);
		row = indices[0];
		gtk_tree_path_free(treepath);
		// find subtitle settings
		if (row < 0) return NULL;
		subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
		if (row >= ghb_array_len(subtitle_list))
			return NULL;
		settings = ghb_array_get_nth(subtitle_list, row);
	}
	return settings;
}

G_MODULE_EXPORT void
subtitle_forced_toggled_cb(
	GtkCellRendererToggle *cell, 
	gchar                 *path,
	signal_user_data_t    *ud)
{
	GtkTreeView  *tv;
	GtkTreeModel *tm;
	GtkTreeIter   ti;
	gboolean      active;
	gint          row;
	GtkTreePath  *tp;
	gint *indices;
	GValue *subtitle_list, *settings;
	gint source;

	g_debug("forced toggled");
	tp = gtk_tree_path_new_from_string (path);
	tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
	g_return_if_fail(tv != NULL);
	tm = gtk_tree_view_get_model(tv);
	g_return_if_fail(tm != NULL);
	gtk_tree_model_get_iter(tm, &ti, tp);
	gtk_tree_model_get(tm, &ti, 1, &active, -1);
	active ^= 1;

	// Get the row number
	indices = gtk_tree_path_get_indices (tp);
	row = indices[0];
	gtk_tree_path_free(tp);

	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");

	if (row < 0 || row >= ghb_array_len(subtitle_list))
		return;

	settings = ghb_array_get_nth(subtitle_list, row);

	source = ghb_settings_get_int(settings, "SubtitleSource");
	if (source != VOBSUB)
		return;

	ghb_settings_set_boolean(settings, "SubtitleForced", active);
	gtk_list_store_set(GTK_LIST_STORE(tm), &ti, 1, active, -1);
	ghb_live_reset(ud);
}

G_MODULE_EXPORT void
subtitle_burned_toggled_cb(
	GtkCellRendererToggle *cell, 
	gchar                 *path,
	signal_user_data_t    *ud)
{
	GtkTreeView  *tv;
	GtkTreeModel *tm;
	GtkTreeIter   ti;
	GtkTreePath  *tp;
	gboolean      active;
	gint          row;
	gint *indices;
	GValue *subtitle_list;
	gint count, track, source;
	GValue *settings;

	g_debug("burned toggled");
	tp = gtk_tree_path_new_from_string (path);
	tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
	g_return_if_fail(tv != NULL);
	tm = gtk_tree_view_get_model(tv);
	g_return_if_fail(tm != NULL);
	gtk_tree_model_get_iter(tm, &ti, tp);
	gtk_tree_model_get(tm, &ti, 2, &active, -1);
	active ^= 1;

	// Get the row number
	indices = gtk_tree_path_get_indices (tp);
	row = indices[0];
	gtk_tree_path_free(tp);

	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	count = ghb_array_len(subtitle_list);
	if (row < 0 || row >= count)
		return;

	settings = ghb_array_get_nth(subtitle_list, row);

	source = ghb_settings_get_int(settings, "SubtitleSource");
	if (source != VOBSUB)
		return;

	track = ghb_settings_combo_int(settings, "SubtitleTrack");
	if (!active && mustBurn(ud, track))
		return;

	ghb_settings_set_boolean(settings, "SubtitleBurned", active);

	gtk_list_store_set(GTK_LIST_STORE(tm), &ti, 2, active, -1);
	// Unburn the rest
	if (active)
		ghb_subtitle_exclusive_burn(ud, row);
	ghb_live_reset(ud);
}

G_MODULE_EXPORT void
subtitle_default_toggled_cb(
	GtkCellRendererToggle *cell, 
	gchar                 *path,
	signal_user_data_t    *ud)
{
	GtkTreeView  *tv;
	GtkTreeModel *tm;
	GtkTreeIter   ti;
	GtkTreePath  *tp;
	gboolean      active;
	gint          row;
	gint *indices;
	GValue *subtitle_list;
	gint count;
	GValue *settings;

	g_debug("default toggled");
	tp = gtk_tree_path_new_from_string (path);
	tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
	g_return_if_fail(tv != NULL);
	tm = gtk_tree_view_get_model(tv);
	g_return_if_fail(tm != NULL);
	gtk_tree_model_get_iter(tm, &ti, tp);
	gtk_tree_model_get(tm, &ti, 3, &active, -1);
	active ^= 1;

	// Get the row number
	indices = gtk_tree_path_get_indices (tp);
	row = indices[0];
	gtk_tree_path_free(tp);

	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	count = ghb_array_len(subtitle_list);
	if (row < 0 || row >= count)
		return;

	settings = ghb_array_get_nth(subtitle_list, row);

	ghb_settings_set_boolean(settings, "SubtitleDefaultTrack", active);

	gtk_list_store_set(GTK_LIST_STORE(tm), &ti, 3, active, -1);
	// allow only one default
	ghb_subtitle_exclusive_default(ud, row);
	ghb_live_reset(ud);
}

static const char*
subtitle_source_name(gint source)
{
	const gchar * name;

	switch (source)
	{
		case VOBSUB:
			name = "Bitmap";
			break;
		case CC708SUB:
		case CC608SUB:
			name = "Text";
			break;
		case SRTSUB:
			name = "SRT";
			break;
		default:
			name = "Unknown";
			break;
	}
	return name;
}

static void
subtitle_list_refresh_selected(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gint *indices;
	gint row;
	GValue *settings = NULL;
	const GValue *subtitle_list;
	
	g_debug("subtitle_list_refresh_selected ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
	selection = gtk_tree_view_get_selection (treeview);
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		gchar *track, *source;
		gboolean forced, burned, def;
		gchar *s_track;
		gint offset = 0;
	
		// Get the row number
		treepath = gtk_tree_model_get_path (store, &iter);
		indices = gtk_tree_path_get_indices (treepath);
		row = indices[0];
		gtk_tree_path_free(treepath);
		// find audio settings
		if (row < 0) return;
		subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
		if (row >= ghb_array_len(subtitle_list))
			return;
		settings = ghb_array_get_nth(subtitle_list, row);

		def = ghb_settings_get_boolean(settings, "SubtitleDefaultTrack");

		gint i_source;
		i_source = ghb_settings_get_int(settings, "SubtitleSource");
		if (i_source != VOBSUB)
		{
			// Force and burn only apply to VOBSUBS
			forced = FALSE;
			burned = FALSE;
			ghb_settings_set_boolean(settings, "SubtitleForced", forced);
			ghb_settings_set_boolean(settings, "SubtitleBurned", burned);
		}

		if (i_source == SRTSUB)
		{
			const gchar *lang;
			gchar *code;

			lang = ghb_settings_combo_option(settings, "SrtLanguage");
			code = ghb_settings_get_string(settings, "SrtCodeset");
			track = g_strdup_printf("%s (%s)", lang, code);
			g_free(code);

			s_track = ghb_settings_get_string(settings, "SrtFile");
			if (g_file_test(s_track, G_FILE_TEST_IS_REGULAR))
			{
				gchar *basename;

				basename = g_path_get_basename(s_track);
				source = g_strdup_printf("SRT (%s)", basename);
				g_free(basename);
			}
			else
			{
				source = g_strdup_printf("SRT (none)");
			}
			offset = ghb_settings_get_int(settings, "SrtOffset");

			forced = FALSE;
			burned = FALSE;
		}
		else
		{
			track = g_strdup(
				ghb_settings_combo_option(settings, "SubtitleTrack"));
			source = g_strdup(subtitle_source_name(i_source));
			s_track = ghb_settings_get_string(settings, "SubtitleTrack");

			forced = ghb_settings_get_boolean(settings, "SubtitleForced");
			burned = ghb_settings_get_boolean(settings, "SubtitleBurned");
		}

		gtk_list_store_set(GTK_LIST_STORE(store), &iter, 
			// These are displayed in list
			0, track,
			1, forced,
			2, burned,
			3, def,
			4, source,
			5, offset,
			// These are used to set combo box values when a list item is selected
			6, s_track,
			7, i_source,
			-1);
		g_free(track);
		g_free(source);
		g_free(s_track);
		if (burned)
			ghb_subtitle_exclusive_burn(ud, row);
	}
}

G_MODULE_EXPORT void
subtitle_track_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	GValue *settings;

	g_debug("subtitle_track_changed_cb ()");
	ghb_check_dependency(ud, widget);
	ghb_widget_to_setting(ud->settings, widget);
	settings = ghb_selected_subtitle_settings(ud);
	if (settings != NULL)
	{
		const gchar *track, *lang;
		gint tt, source;

		ghb_widget_to_setting(settings, widget);
		subtitle_list_refresh_selected(ud);
		track = ghb_settings_combo_option(settings, "SubtitleTrack");
		ghb_settings_set_string(settings, "SubtitleTrackDescription", track);
		tt = ghb_settings_get_int(settings, "SubtitleTrack");
		source = ghb_subtitle_track_source(ud, tt);
		ghb_settings_set_int(settings, "SubtitleSource", source);
		lang = ghb_settings_combo_string(settings, "SubtitleTrack");
		ghb_settings_set_string(settings, "SubtitleLanguage", lang);
		ghb_live_reset(ud);
	}
	ghb_live_reset(ud);
}

G_MODULE_EXPORT void
srt_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	GValue *settings;

	g_debug("srt_changed_cb ()");
	ghb_check_dependency(ud, widget);
	ghb_widget_to_setting(ud->settings, widget);
	settings = ghb_selected_subtitle_settings(ud);
	if (settings != NULL)
	{
		ghb_widget_to_setting(settings, widget);
		subtitle_list_refresh_selected(ud);

		ghb_live_reset(ud);
	}
}

G_MODULE_EXPORT void
srt_file_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	GValue *settings;

	g_debug("srt_changed_cb ()");
	ghb_check_dependency(ud, widget);
	ghb_widget_to_setting(ud->settings, widget);
	settings = ghb_selected_subtitle_settings(ud);
	if (settings != NULL)
	{
		gchar *filename, *dirname;

		ghb_widget_to_setting(settings, widget);
		subtitle_list_refresh_selected(ud);

		ghb_live_reset(ud);

		filename = ghb_settings_get_string(settings, "SrtFile");
		if (g_file_test(filename, G_FILE_TEST_IS_DIR))
		{
			ghb_settings_set_string(ud->settings, "SrtDir", filename);
		}
		else
		{
			dirname = g_path_get_dirname(filename);
			ghb_settings_set_string(ud->settings, "SrtDir", dirname);
			g_free(dirname);
		}
		ghb_pref_save(ud->settings, "SrtDir");
		g_free(filename);
	}
}

G_MODULE_EXPORT void
srt_lang_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	GValue *settings;

	g_debug("srt_lang_changed_cb ()");
	ghb_check_dependency(ud, widget);
	ghb_widget_to_setting(ud->settings, widget);
	settings = ghb_selected_subtitle_settings(ud);
	if (settings != NULL)
	{
		const gchar *lang;

		ghb_widget_to_setting(settings, widget);
		subtitle_list_refresh_selected(ud);

		ghb_live_reset(ud);

		lang = ghb_settings_combo_option(settings, "SrtLanguage");
		ghb_settings_set_string(settings, "SubtitleTrackDescription", lang);
	}
}

void
ghb_clear_subtitle_list(signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkListStore *store;
	GValue *subtitle_list;
	
	g_debug("clear_subtitle_list ()");
	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	if (subtitle_list == NULL)
	{
		subtitle_list = ghb_array_value_new(8);
		ghb_settings_set_value(ud->settings, "subtitle_list", subtitle_list);
	}
	else
		ghb_array_value_reset(subtitle_list, 8);
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
	gtk_list_store_clear (store);
}

static void
add_to_subtitle_list(
	signal_user_data_t *ud, 
	GValue *settings)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	GtkTreeSelection *selection;
	const gchar *track, *source;
	gboolean forced, burned, def;
	gchar *s_track;
	gint i_source;
	
	g_debug("add_to_subtitle_list ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
	selection = gtk_tree_view_get_selection (treeview);
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

	track = ghb_settings_combo_option(settings, "SubtitleTrack");
	forced = ghb_settings_get_boolean(settings, "SubtitleForced");
	burned = ghb_settings_get_boolean(settings, "SubtitleBurned");
	def = ghb_settings_get_boolean(settings, "SubtitleDefaultTrack");

	s_track = ghb_settings_get_string(settings, "SubtitleTrack");
	i_source = ghb_settings_get_int(settings, "SubtitleSource");
	source = subtitle_source_name(i_source);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
		// These are displayed in list
		0, track,
		1, forced,
		2, burned,
		3, def,
		4, source,
		// These are used to set combo box values when a list item is selected
		6, s_track,
		7, i_source,
		8, TRUE,
		9, TRUE,
		10, FALSE,
		-1);
	gtk_tree_selection_select_iter(selection, &iter);
	g_free(s_track);
}

static void
add_to_srt_list(
	signal_user_data_t *ud, 
	GValue *settings)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	GtkTreeSelection *selection;
	const gchar *lang;
	gboolean forced, burned, def;
	gchar *filename, *code, *track, *source;
	gint i_source, offset;
	
	g_debug("add_to_srt_list ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
	selection = gtk_tree_view_get_selection (treeview);
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

	lang = ghb_settings_combo_option(settings, "SrtLanguage");
	code = ghb_settings_get_string(settings, "SrtCodeset");
	track = g_strdup_printf("%s (%s)", lang, code);
	forced = FALSE;
	burned = FALSE;
	def = ghb_settings_get_boolean(settings, "SubtitleDefaultTrack");

	filename = ghb_settings_get_string(settings, "SrtFile");
	if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
	{
		gchar *basename;

		basename = g_path_get_basename(filename);
		source = g_strdup_printf("SRT (%s)", basename);
		g_free(basename);
	}
	else
	{
		source = g_strdup_printf("SRT (none)");
	}
	i_source = SRTSUB;
	offset = ghb_settings_get_int(settings, "SrtOffset");

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
		// These are displayed in list
		0, track,
		1, forced,
		2, burned,
		3, def,
		4, source,
		5, offset,
		// These are used to set combo box values when a list item is selected
		6, filename,
		7, i_source,
		8, FALSE,
		9, FALSE,
		10, TRUE,
		-1);
	gtk_tree_selection_select_iter(selection, &iter);
	g_free(code);
	g_free(track);
	g_free(filename);
	g_free(source);
}

G_MODULE_EXPORT void
subtitle_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkWidget *widget;
	
	g_debug("subtitle_list_selection_changed_cb ()");
	if (gtk_tree_selection_get_selected(selection, &store, &iter))
	{
		gint source;
		GtkTreePath *treepath;
		gint *indices, row;
		GValue *subtitle_list, *settings;

		treepath = gtk_tree_model_get_path (store, &iter);
		indices = gtk_tree_path_get_indices (treepath);
		row = indices[0];
		gtk_tree_path_free(treepath);

		subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
		if (row >= ghb_array_len(subtitle_list))
			return;

		settings = ghb_array_get_nth(subtitle_list, row);

		source = ghb_settings_get_int(settings, "SubtitleSource");
		if (source == SRTSUB)
		{
			gchar *str;
			gint offset;

			str = ghb_settings_get_string(settings, "SrtLanguage");
			ghb_ui_update(ud, "SrtLanguage", ghb_string_value(str));
			g_free(str);

			str = ghb_settings_get_string(settings, "SrtCodeset");
			ghb_ui_update(ud, "SrtCodeset", ghb_string_value(str));
			g_free(str);

			str = ghb_settings_get_string(settings, "SrtFile");
			ghb_ui_update(ud, "SrtFile", ghb_string_value(str));
			g_free(str);

			offset = ghb_settings_get_int(settings, "SrtOffset");
			ghb_ui_update(ud, "SrtOffset", ghb_int_value(offset));

			widget = GHB_WIDGET(ud->builder, "subtitle_track_label");
			gtk_widget_hide(widget);
			widget = GHB_WIDGET(ud->builder, "SubtitleTrack");
			gtk_widget_hide(widget);
			widget = GHB_WIDGET(ud->builder, "srt_lang_label");
			gtk_widget_show(widget);
			widget = GHB_WIDGET(ud->builder, "srt_code_label");
			gtk_widget_show(widget);
			widget = GHB_WIDGET(ud->builder, "srt_file_label");
			gtk_widget_show(widget);
			widget = GHB_WIDGET(ud->builder, "srt_offset_label");
			gtk_widget_show(widget);
			widget = GHB_WIDGET(ud->builder, "SrtLanguage");
			gtk_widget_show(widget);
			widget = GHB_WIDGET(ud->builder, "SrtCodeset");
			gtk_widget_show(widget);
			widget = GHB_WIDGET(ud->builder, "SrtFile");
			gtk_widget_show(widget);
			widget = GHB_WIDGET(ud->builder, "SrtOffset");
			gtk_widget_show(widget);
		}
		else
		{
			gchar *track;

			track = ghb_settings_get_string(settings, "SubtitleTrack");
			ghb_ui_update(ud, "SubtitleTrack", ghb_string_value(track));
			g_free(track);

			widget = GHB_WIDGET(ud->builder, "srt_lang_label");
			gtk_widget_hide(widget);
			widget = GHB_WIDGET(ud->builder, "srt_code_label");
			gtk_widget_hide(widget);
			widget = GHB_WIDGET(ud->builder, "srt_file_label");
			gtk_widget_hide(widget);
			widget = GHB_WIDGET(ud->builder, "srt_offset_label");
			gtk_widget_hide(widget);
			widget = GHB_WIDGET(ud->builder, "SrtLanguage");
			gtk_widget_hide(widget);
			widget = GHB_WIDGET(ud->builder, "SrtCodeset");
			gtk_widget_hide(widget);
			widget = GHB_WIDGET(ud->builder, "SrtFile");
			gtk_widget_hide(widget);
			widget = GHB_WIDGET(ud->builder, "SrtOffset");
			gtk_widget_hide(widget);
			widget = GHB_WIDGET(ud->builder, "subtitle_track_label");
			gtk_widget_show(widget);
			widget = GHB_WIDGET(ud->builder, "SubtitleTrack");
			gtk_widget_show(widget);
		}
		widget = GHB_WIDGET (ud->builder, "subtitle_remove");
		gtk_widget_set_sensitive(widget, TRUE);
	}
	else
	{
		widget = GHB_WIDGET(ud->builder, "srt_lang_label");
		gtk_widget_hide(widget);
		widget = GHB_WIDGET(ud->builder, "srt_code_label");
		gtk_widget_hide(widget);
		widget = GHB_WIDGET(ud->builder, "srt_file_label");
		gtk_widget_hide(widget);
		widget = GHB_WIDGET(ud->builder, "srt_offset_label");
		gtk_widget_hide(widget);
		widget = GHB_WIDGET(ud->builder, "SrtLanguage");
		gtk_widget_hide(widget);
		widget = GHB_WIDGET(ud->builder, "SrtCodeset");
		gtk_widget_hide(widget);
		widget = GHB_WIDGET(ud->builder, "SrtFile");
		gtk_widget_hide(widget);
		widget = GHB_WIDGET(ud->builder, "SrtOffset");
		gtk_widget_hide(widget);
		widget = GHB_WIDGET(ud->builder, "subtitle_track_label");
		gtk_widget_show(widget);
		widget = GHB_WIDGET(ud->builder, "SubtitleTrack");
		gtk_widget_show(widget);
	}
}

G_MODULE_EXPORT void
srt_add_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	// Add the current subtitle settings to the list.
	GValue *settings;
	gboolean burned = FALSE;
	gint track;
	gchar *dir, *filename;
	
	g_debug("subtitle_add_clicked_cb ()");

	track = ghb_settings_get_int(ud->settings, "SubtitleTrack");
	if (mustBurn(ud, track))
	{
		burned = TRUE;
	}
	settings = ghb_dict_value_new();
	ghb_settings_set_string(settings, "SrtLanguage", "und");
	ghb_settings_set_string(settings, "SrtCodeset", "UTF-8");

	dir = ghb_settings_get_string(ud->settings, "SrtDir");
	filename = g_strdup_printf("%s/none", dir);
	ghb_settings_set_string(settings, "SrtFile", filename);
	g_free(dir);
	g_free(filename);

	ghb_settings_set_int(settings, "SrtOffset", 0);
	ghb_settings_take_value(settings, "SubtitleDefaultTrack", 
							ghb_boolean_value_new(FALSE));

	ghb_add_srt(ud, settings);
}

G_MODULE_EXPORT void
subtitle_add_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	// Add the current subtitle settings to the list.
	GValue *settings;
	gboolean burned = FALSE;
	gint track;
	
	g_debug("subtitle_add_clicked_cb ()");

	track = ghb_settings_get_int(ud->settings, "SubtitleTrack");
	if (mustBurn(ud, track))
	{
		burned = TRUE;
	}
	settings = ghb_dict_value_new();
	ghb_settings_set_int(settings, "SubtitleTrack", track);
	ghb_settings_take_value(settings, "SubtitleForced", 
							ghb_boolean_value_new(FALSE));
	ghb_settings_take_value(settings, "SubtitleBurned", 
							ghb_boolean_value_new(burned));
	ghb_settings_take_value(settings, "SubtitleDefaultTrack", 
							ghb_boolean_value_new(FALSE));

	ghb_add_subtitle(ud, settings);
}

G_MODULE_EXPORT void
subtitle_remove_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
	GtkTreeView *treeview;
	GtkTreePath *treepath;
	GtkTreeSelection *selection;
	GtkTreeModel *store;
	GtkTreeIter iter, nextIter;
	gint *indices;
	gint row;
	GValue *subtitle_list;

	g_debug("subtitle_remove_clicked_cb ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
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
		// remove from subtitle settings list
		if (row < 0) return;
		widget = GHB_WIDGET (ud->builder, "subtitle_add");
		gtk_widget_set_sensitive(widget, TRUE);
		widget = GHB_WIDGET (ud->builder, "srt_add");
		gtk_widget_set_sensitive(widget, TRUE);
		subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
		if (row >= ghb_array_len(subtitle_list))
			return;
		GValue *old = ghb_array_get_nth(subtitle_list, row);
		ghb_value_free(old);
		ghb_array_remove(subtitle_list, row);
		ghb_live_reset(ud);
	}
}

void
ghb_subtitle_prune(signal_user_data_t *ud)
{
	GtkTreeView  *tv;
	GtkTreeModel *tm;
	GtkTreeIter   ti;
	GValue *subtitle_list, *settings;
	gint count, ii, track;
	gboolean burned;
	gint first_track = 0, one_burned = 0;

	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	if (subtitle_list == NULL)
		return;
	count = ghb_array_len(subtitle_list);

	tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
	g_return_if_fail(tv != NULL);
	tm = gtk_tree_view_get_model(tv);
	for (ii = count-1; ii >= 0; ii--)
	{
		settings = ghb_array_get_nth(subtitle_list, ii);
		burned = ghb_settings_get_boolean(settings, "SubtitleBurned");
		track = ghb_settings_combo_int(settings, "SubtitleTrack");
		if (!burned && mustBurn(ud, track))
		{
			gtk_tree_model_iter_nth_child(tm, &ti, NULL, ii);
			gtk_list_store_remove (GTK_LIST_STORE(tm), &ti);
			ghb_array_remove(subtitle_list, ii);
		}
		if (burned)
		{
			first_track = ii;
			one_burned++;
		}
	}
	if (one_burned)
	{
		ghb_subtitle_exclusive_burn(ud, first_track);
	}
}

void
ghb_reset_subtitles(signal_user_data_t *ud, GValue *settings)
{
	GValue *slist;
	GValue *subtitle;
	gint count, ii;
	gint titleindex;
	
	g_debug("ghb_reset_subtitles");
	ghb_clear_subtitle_list(ud);
	titleindex = ghb_settings_combo_int(ud->settings, "title");
	if (titleindex < 0)
		return;

	slist = ghb_settings_get_value(settings, "subtitle_list");
	count = ghb_array_len(slist);
	for (ii = 0; ii < count; ii++)
	{
		subtitle = ghb_value_dup(ghb_array_get_nth(slist, ii));
		ghb_add_subtitle(ud, subtitle);
	}
}

