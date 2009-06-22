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
ghb_add_subtitle(signal_user_data_t *ud, GValue *settings)
{
	// Add the current subtitle settings to the list.
	GValue *subtitle_list;
	gint count;
	gboolean burned;
	
	g_debug("ghb_add_subtitle ()");

	// Only allow up to 8 subtitle entries
	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	if (subtitle_list == NULL)
	{
		subtitle_list = ghb_array_value_new(8);
		ghb_settings_set_value(ud->settings, "subtitle_list", subtitle_list);
	}
	count = ghb_array_len(subtitle_list);
	ghb_array_append(subtitle_list, settings);
	add_to_subtitle_list(ud, settings);

	burned = ghb_settings_get_boolean(settings, "SubtitleBurned");
	if (burned)
		ghb_subtitle_exclusive_burn(ud, count);
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
	pref_lang = ghb_settings_get_string(ud->settings, "SourceAudioLang");

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
		if (track >= -1)
		{
			GValue *dup = ghb_value_dup(subtitle);
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
			ghb_add_subtitle(ud, dup);
			jj++;
		}
		g_free(lang);
	}
	if (foreign_lang_index < 0 && pref_lang != NULL)
	{
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
	gint source, track;

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
	track = ghb_settings_combo_int(settings, "SubtitleTrack");

	source = ghb_subtitle_track_source(ud, track);
	if (source != VOBSUB)
		return;

	ghb_settings_set_boolean(settings, "SubtitleForced", active);
	gtk_list_store_set(GTK_LIST_STORE(tm), &ti, 1, active, -1);
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
	track = ghb_settings_combo_int(settings, "SubtitleTrack");

	source = ghb_subtitle_track_source(ud, track);
	if (source != VOBSUB)
		return;

	if (!active && mustBurn(ud, track))
		return;

	ghb_settings_set_boolean(settings, "SubtitleBurned", active);

	gtk_list_store_set(GTK_LIST_STORE(tm), &ti, 2, active, -1);
	// Unburn the rest
	if (active)
		ghb_subtitle_exclusive_burn(ud, row);
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
	gint count, track;
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
	track = ghb_settings_combo_int(settings, "SubtitleTrack");

	ghb_settings_set_boolean(settings, "SubtitleDefaultTrack", active);

	gtk_list_store_set(GTK_LIST_STORE(tm), &ti, 3, active, -1);
	// allow only one default
	ghb_subtitle_exclusive_default(ud, row);
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
		const gchar *track, *source;
		gboolean forced, burned, def;
		gchar *s_track;
		gint i_track;
	
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

		track = ghb_settings_combo_option(settings, "SubtitleTrack");
		forced = ghb_settings_get_boolean(settings, "SubtitleForced");
		burned = ghb_settings_get_boolean(settings, "SubtitleBurned");
		def = ghb_settings_get_boolean(settings, "SubtitleDefaultTrack");

		s_track = ghb_settings_get_string(settings, "SubtitleTrack");
		i_track = ghb_settings_get_int(settings, "SubtitleTrack");
		source = ghb_subtitle_track_source_name(ud, i_track);

		gint i_source;
		i_source = ghb_subtitle_track_source(ud, i_track);
		if (i_source != VOBSUB)
		{
			// Force and burn only apply to VOBSUBS
			forced = FALSE;
			burned = FALSE;
			ghb_settings_set_boolean(settings, "SubtitleForced", forced);
			ghb_settings_set_boolean(settings, "SubtitleBurned", burned);
		}

		gtk_list_store_set(GTK_LIST_STORE(store), &iter, 
			// These are displayed in list
			0, track,
			1, forced,
			2, burned,
			3, def,
			4, source,
			// These are used to set combo box values when a list item is selected
			5, s_track,
			-1);
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
	settings = ghb_selected_subtitle_settings(ud);
	if (settings != NULL)
	{
		ghb_widget_to_setting(settings, widget);
		subtitle_list_refresh_selected(ud);
	}
	ghb_live_reset(ud);
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
	gint i_track;
	
	g_debug("add_to_subtitle_list ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
	selection = gtk_tree_view_get_selection (treeview);
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

	track = ghb_settings_combo_option(settings, "SubtitleTrack");
	forced = ghb_settings_get_boolean(settings, "SubtitleForced");
	burned = ghb_settings_get_boolean(settings, "SubtitleBurned");
	def = ghb_settings_get_boolean(settings, "SubtitleDefaultTrack");

	s_track = ghb_settings_get_string(settings, "SubtitleTrack");
	i_track = ghb_settings_get_int(settings, "SubtitleTrack");
	source = ghb_subtitle_track_source_name(ud, i_track);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
		// These are displayed in list
		0, track,
		1, forced,
		2, burned,
		3, def,
		4, source,
		// These are used to set combo box values when a list item is selected
		5, s_track,
		-1);
	gtk_tree_selection_select_iter(selection, &iter);
	g_free(s_track);
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
		const gchar *track;

		gtk_tree_model_get(store, &iter, 5, &track, -1);
		ghb_ui_update(ud, "SubtitleTrack", ghb_string_value(track));

		widget = GHB_WIDGET (ud->builder, "subtitle_remove");
		gtk_widget_set_sensitive(widget, TRUE);
	}
}

G_MODULE_EXPORT void
subtitle_add_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	// Add the current subtitle settings to the list.
	GValue *settings;
	gint count;
	GValue *subtitle_list;
	gboolean burned = FALSE;
	gint track;
	
	g_debug("subtitle_add_clicked_cb ()");
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
		return;

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

	ghb_array_append(subtitle_list, settings);
	add_to_subtitle_list(ud, settings);
	if (burned)
		ghb_subtitle_exclusive_burn(ud, count);
	if (count == 98)
	{
		GtkWidget *widget;
		widget = GHB_WIDGET (ud->builder, "subtitle_add");
		gtk_widget_set_sensitive(widget, FALSE);
	}
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
		subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
		if (row >= ghb_array_len(subtitle_list))
			return;
		GValue *old = ghb_array_get_nth(subtitle_list, row);
		ghb_value_free(old);
		ghb_array_remove(subtitle_list, row);
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

