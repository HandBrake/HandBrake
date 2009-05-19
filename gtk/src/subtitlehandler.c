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
#include "subtitlehandler.h"

static void add_to_subtitle_list(signal_user_data_t *ud, GValue *settings);

void
free_subtitle_index_list(gpointer data)
{
	g_free(data);
}

void
free_subtitle_key(gpointer data)
{
	if (data != NULL)
		g_free(data);
}

static void
add_pref(signal_user_data_t *ud, GValue *settings)
{
	// Add the current subtitle settings to the list.
	GtkWidget *widget;
	gint count;
	GValue *subtitle_list;
	
	g_debug("add_pref ()");

	// Only allow up to 8 subtitle entries
	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	if (subtitle_list == NULL)
	{
		subtitle_list = ghb_array_value_new(8);
		ghb_settings_set_value(ud->settings, "subtitle_list", subtitle_list);
	}
	ghb_array_append(subtitle_list, settings);
	add_to_subtitle_list(ud, settings);
	count = ghb_array_len(subtitle_list);
	if (count >= 8)
	{
		widget = GHB_WIDGET(ud->builder, "subtitle_add");
		gtk_widget_set_sensitive(widget, FALSE);
	}
}

void
ghb_set_pref_subtitle(gint titleindex, signal_user_data_t *ud)
{
	gint track;
	GtkWidget *button;
	GHashTable *track_indices;
	char *lang;

	const GValue *pref_subtitle;
	GValue *subtitle;
	gint count, ii;
	
	g_debug("ghb_set_pref_subtitle");
	track_indices = g_hash_table_new_full(g_str_hash, g_str_equal, 
											NULL, free_subtitle_index_list);
	// Clear the subtitle list
	ghb_clear_subtitle_list(ud);
	// Find "best" subtitle based on subtitle preferences
	button = GHB_WIDGET (ud->builder, "subtitle_add");

	pref_subtitle = ghb_settings_get_value(ud->settings, "SubtitleList");

	count = ghb_array_len(pref_subtitle);
	for (ii = 0; ii < count; ii++)
	{
		subtitle = ghb_value_dup(ghb_array_get_nth(pref_subtitle, ii));
		lang = ghb_settings_get_string(subtitle, "SubtitleLanguage");
		// If there are multiple subtitles using the same language, then
		// select sequential tracks for each.  The hash keeps track 
		// of the tracks used for each language.
		track = ghb_find_subtitle_track(titleindex, lang, track_indices);
		ghb_settings_set_int(subtitle, "SubtitleTrack", track);
		// Add to subtitle list
		add_pref(ud, subtitle);
		ghb_ui_update(ud, "SubtitleTrack", ghb_int64_value(track));
		g_free(lang);
	}
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
		gboolean forced, burned;
		gchar *s_track;
		gint i_track;

		// Get the row number
		treepath = gtk_tree_model_get_path (store, &iter);
		indices = gtk_tree_path_get_indices (treepath);
		row = indices[0];
		gtk_tree_path_free(treepath);
		// find subtitle settings
		if (row < 0) return;
		subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
		if (row >= ghb_array_len(subtitle_list))
			return;
		settings = ghb_array_get_nth(subtitle_list, row);

		track = ghb_settings_combo_option(settings, "SubtitleTrack");
		forced = ghb_settings_get_boolean(settings, "SubtitleForced");
		burned = ghb_settings_get_boolean(settings, "SubtitleBurned");

		s_track = ghb_settings_get_string(settings, "SubtitleTrack");
		i_track = ghb_settings_get_int(settings, "SubtitleTrack");
		source = ghb_subtitle_track_source_name(ud, i_track);

		gtk_list_store_set(GTK_LIST_STORE(store), &iter, 
			// These are displayed in list
			0, track,
			1, forced,
			2, burned,
			3, source,
			// These are used to set combo values when a list item is selected
			4, s_track,
			-1);
		g_free(s_track);
	}
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

	g_debug("forced toggled");
	tp = gtk_tree_path_new_from_string (path);
	tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
	g_return_if_fail(tv != NULL);
	tm = gtk_tree_view_get_model(tv);
	g_return_if_fail(tm != NULL);
	gtk_tree_model_get_iter(tm, &ti, tp);
	gtk_tree_model_get(tm, &ti, 1, &active, -1);
	active ^= 1;
	gtk_list_store_set(GTK_LIST_STORE(tm), &ti, 1, active, -1);

	// Get the row number
	indices = gtk_tree_path_get_indices (tp);
	row = indices[0];
	gtk_tree_path_free(tp);

	if (row >= 0)
	{
		GValue *subtitle_list;

		subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
		if (row < ghb_array_len(subtitle_list))
		{
			GValue *settings;

			settings = ghb_array_get_nth(subtitle_list, row);
			ghb_settings_set_boolean(settings, "SubtitleForced", active);
		}
	}
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
	gint count, track;
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
	if (!active && mustBurn(ud, track))
		return;

	ghb_settings_set_boolean(settings, "SubtitleBurned", active);
	gtk_list_store_set(GTK_LIST_STORE(tm), &ti, 2, active, -1);
}

static gboolean
trackUsed(signal_user_data_t *ud, gint track)
{
	gint ii, count, tt;
	GValue *settings, *subtitle_list;

	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	count = ghb_array_len(subtitle_list);
	for (ii = 0; ii < count; ii++)
	{
		settings = ghb_array_get_nth(subtitle_list, ii);
		tt = ghb_settings_combo_int(settings, "SubtitleTrack");
		if (tt == track)
		{
			return TRUE;
		}
	}
	return FALSE;
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
		const gchar *lang;
		GValue *gval;
		GtkWidget *widget;
		gint track;

		widget = GHB_WIDGET (ud->builder, "SubtitleTrack");
		gval = ghb_widget_value(widget);
		track = ghb_value_int(gval);
		ghb_value_free(gval);
		if (trackUsed(ud, track))
			return;
		ghb_widget_to_setting(settings, widget);
		track = ghb_settings_combo_int(settings, "SubtitleTrack");
		lang = ghb_settings_combo_string(settings, "SubtitleTrack");
		if (mustBurn(ud, track))
		{
			ghb_settings_set_boolean(settings, "SubtitleBurned", TRUE);
		}
		else
		{
			ghb_settings_set_boolean(settings, "SubtitleBurned", FALSE);
		}
		ghb_settings_set_string(settings, "SubtitleLanguage", lang);
		subtitle_list_refresh_selected(ud);
	}
	ghb_live_reset(ud);
}

void
ghb_subtitle_adjust_burn(signal_user_data_t *ud)
{
	GValue *subtitle_list;
	GValue *settings;
	gint ii, count, track;
	GtkTreeView  *tv;
	GtkTreeModel *tm;
	GtkTreeIter   ti;

	g_debug("ghb_subtitle_adjust_burn");
	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	count = ghb_array_len(subtitle_list);
	for (ii = 0; ii < count; ii++)
	{
		settings = ghb_array_get_nth(subtitle_list, ii);
		track = ghb_settings_combo_int(settings, "SubtitleTrack");
		if (mustBurn(ud, track))
		{
			tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
			g_return_if_fail(tv != NULL);
			tm = gtk_tree_view_get_model(tv);
			gtk_tree_model_iter_nth_child(tm, &ti, NULL, ii);

			ghb_settings_set_boolean(settings, "SubtitleBurned", TRUE);
			gtk_list_store_set(GTK_LIST_STORE(tm), &ti, 2, TRUE, -1);
		}
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
add_to_subtitle_list(signal_user_data_t *ud, GValue *settings)
{
	GtkTreeView *treeview;
	GtkTreeIter iter;
	GtkListStore *store;
	GtkTreeSelection *selection;
	const gchar *track, *source;
	gboolean forced, burned;
	gchar *s_track;
	gint i_track;
	
	g_debug("add_to_subtitle_list ()");
	treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list"));
	selection = gtk_tree_view_get_selection (treeview);
	store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));

	track = ghb_settings_combo_option(settings, "SubtitleTrack");
	forced = ghb_settings_get_boolean(settings, "SubtitleForced");
	burned = ghb_settings_get_boolean(settings, "SubtitleBurned");

	s_track = ghb_settings_get_string(settings, "SubtitleTrack");
	i_track = ghb_settings_get_int(settings, "SubtitleTrack");
	source = ghb_subtitle_track_source_name(ud, i_track);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 
		// These are displayed in list
		0, track,
		1, forced,
		2, burned,
		3, source,
		// These are used to set combo box values when a list item is selected
		4, s_track,
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

		gtk_tree_model_get(store, &iter, 4, &track, -1);
		ghb_ui_update(ud, "SubtitleTrack", ghb_string_value(track));
		widget = GHB_WIDGET (ud->builder, "subtitle_remove");
		gtk_widget_set_sensitive(widget, TRUE);
	}
	else
	{
		widget = GHB_WIDGET (ud->builder, "subtitle_remove");
		gtk_widget_set_sensitive(widget, FALSE);
	}
}

G_MODULE_EXPORT void
subtitle_add_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
	// Add the current subtitle settings to the list.
	GValue *settings;
	GtkWidget *widget;
	gint count;
	GValue *subtitle_list;
	gboolean burned = FALSE;
	gint track;
	
	g_debug("subtitle_add_clicked_cb ()");
	track = ghb_pick_subtitle_track(ud);
	if (track < 0)
		return;

	if (mustBurn(ud, track))
	{
		burned = TRUE;
	}
	settings = ghb_dict_value_new();
	// Only allow up to 8 subtitle entries
	widget = GHB_WIDGET(ud->builder, "SubtitleTrack");
	ghb_settings_set_int(settings, "SubtitleTrack", track);
	ghb_settings_take_value(settings, "SubtitleForced", 
							ghb_boolean_value_new(FALSE));
	ghb_settings_take_value(settings, "SubtitleBurned", 
							ghb_boolean_value_new(burned));

	subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
	if (subtitle_list == NULL)
	{
		subtitle_list = ghb_array_value_new(8);
		ghb_settings_set_value(ud->settings, "subtitle_list", subtitle_list);
	}
	ghb_array_append(subtitle_list, settings);
	add_to_subtitle_list(ud, settings);
	count = ghb_array_len(subtitle_list);
	if (count >= 8)
	{
		gtk_widget_set_sensitive(xwidget, FALSE);
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
ghb_set_subtitle(signal_user_data_t *ud, GValue *settings)
{
	GtkWidget *button;

	GValue *slist;
	GValue *track, *subtitle;
	//GValue *forced, *burned;
	gint count, ii, i_track;
	
	g_debug("set_subtitle");
	// Clear the subtitle list
	ghb_clear_subtitle_list(ud);
	button = GHB_WIDGET (ud->builder, "subtitle_add");
	slist = ghb_settings_get_value(settings, "subtitle_list");

	count = ghb_array_len(slist);
	for (ii = 0; ii < count; ii++)
	{
		subtitle = ghb_array_get_nth(slist, ii);
		track = ghb_settings_get_value(subtitle, "SubtitleTrack");
		i_track = ghb_settings_get_int(subtitle, "SubtitleTrack");
		//forced = ghb_settings_get_value(subtitle, "SubtitleForced");
		//burned = ghb_settings_get_value(subtitle, "SubtitleBurned");

		if (i_track != -2)
		{
			// Add to subtitle list
			g_signal_emit_by_name(button, "clicked", ud);
			ghb_ui_update(ud, "AudioTrack", track);
		}
	}
}

