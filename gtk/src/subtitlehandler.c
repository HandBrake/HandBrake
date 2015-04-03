/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * subtitlehandler.c
 * Copyright (C) John Stebbins 2008-2015 <stebbins@stebbins>
 *
 * subtitlehandler.c is free software.
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
#include "presets.h"
#include "audiohandler.h"
#include "subtitlehandler.h"

static void subtitle_add_to_settings(GhbValue *settings, GhbValue *subsettings);
static void ghb_add_subtitle_to_ui(signal_user_data_t *ud, GhbValue *subsettings);
static void add_to_subtitle_list_ui(signal_user_data_t *ud, GhbValue *settings);
static void ghb_clear_subtitle_list_settings(GhbValue *settings);
static void ghb_clear_subtitle_list_ui(GtkBuilder *builder);

static void
subtitle_refresh_list_row_ui(
    GtkTreeModel *tm,
    GtkTreeIter *ti,
    GhbValue *subsettings)
{
    GtkTreeIter cti;
    gboolean forced, burned, def;
    char *info_src, *info_src_2;
    char *info_dst, *info_dst_2;


    info_src_2 = NULL;
    info_dst_2 = NULL;

    forced = ghb_settings_get_boolean(subsettings, "SubtitleForced");
    burned = ghb_settings_get_boolean(subsettings, "SubtitleBurned");
    def = ghb_settings_get_boolean(subsettings, "SubtitleDefaultTrack");
    info_src = g_strdup_printf("<small>%s</small>",
        ghb_settings_get_const_string(subsettings, "SubtitleTrackDescription"));
    if (ghb_settings_get_int(subsettings, "SubtitleSource") == SRTSUB)
    {
        gint offset;
        offset = ghb_settings_get_int(subsettings, "SrtOffset");
        if (offset != 0)
        {
            info_dst_2 = g_strdup_printf("Offset: %dms", offset);
        }
    }

    GString *str = g_string_new("<small>");
    g_string_append_printf(str, "%s ", burned ? "Burned Into Video" :
                                                "Passthrough");
    if (forced)
    {
        g_string_append_printf(str, "(Forced Subtitles Only)");
    }
    if (def)
    {
        g_string_append_printf(str, "(Default)");
    }
    g_string_append_printf(str, "</small>");

    info_dst = g_string_free(str, FALSE);

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
        if (gtk_tree_model_iter_children(tm, &cti, ti))
        {
            gtk_tree_store_remove(GTK_TREE_STORE(tm), &cti);
        }
    }

    g_free(info_src);
    g_free(info_src_2);
    g_free(info_dst);
    g_free(info_dst_2);
}

static void
subtitle_refresh_list_ui_from_settings(signal_user_data_t *ud, GhbValue *settings)
{
    GhbValue *subtitle_list;
    GhbValue *subsettings;
    gint ii, count, tm_count;
    GtkTreeView  *tv;
    GtkTreeModel *tm;
    GtkTreeIter   ti;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list_view"));
    tm = gtk_tree_view_get_model(tv);

    tm_count = gtk_tree_model_iter_n_children(tm, NULL);

    subtitle_list = ghb_settings_get_value(settings, "subtitle_list");
    count = ghb_array_len(subtitle_list);
    if (count != tm_count)
    {
        ghb_clear_subtitle_list_ui(ud->builder);
        for (ii = 0; ii < count; ii++)
        {
            gtk_tree_store_append(GTK_TREE_STORE(tm), &ti, NULL);
        }
    }
    for (ii = 0; ii < count; ii++)
    {
        g_return_if_fail(tv != NULL);
        gtk_tree_model_iter_nth_child(tm, &ti, NULL, ii);
        subsettings = ghb_array_get(subtitle_list, ii);
        subtitle_refresh_list_row_ui(tm, &ti, subsettings);
    }
}

static void
subtitle_refresh_list_ui(signal_user_data_t *ud)
{
    subtitle_refresh_list_ui_from_settings(ud, ud->settings);
}

void
ghb_subtitle_exclusive_burn_settings(GhbValue *settings, gint index)
{
    GhbValue *subtitle_list;
    GhbValue *subsettings;
    gint ii, count;

    subtitle_list = ghb_settings_get_value(settings, "subtitle_list");
    subsettings = ghb_array_get(subtitle_list, index);
    if (subsettings != NULL)
    {
        int track = ghb_settings_get_int(subsettings, "SubtitleTrack");
        if (track == -1)
        {
            // Allow 2 tracks to be marked burned when one is
            // foreign audio search.  Extra burned track will be
            // sanitized away if foreign audio search actually finds
            // something.
            return;
        }
    }
    count = ghb_array_len(subtitle_list);
    for (ii = 0; ii < count; ii++)
    {
        if (ii != index)
        {
            subsettings = ghb_array_get(subtitle_list, ii);
            int track = ghb_settings_get_int(subsettings, "SubtitleTrack");
            if (track != -1)
            {
                // Allow 2 tracks to be marked burned when one is
                // foreign audio search.  Extra burned track will be
                // sanitized away if foreign audio search actually finds
                // something.
                ghb_settings_set_boolean(subsettings, "SubtitleBurned", FALSE);
            }
        }
    }
}

void
ghb_subtitle_exclusive_burn(signal_user_data_t *ud, gint index)
{
    ghb_subtitle_exclusive_burn_settings(ud->settings, index);
    subtitle_refresh_list_ui(ud);
}

void
ghb_subtitle_exclusive_default_settings(GhbValue *settings, gint index)
{
    GhbValue *subtitle_list;
    GhbValue *subtitle;
    gint ii, count;

    subtitle_list = ghb_settings_get_value(settings, "subtitle_list");
    count = ghb_array_len(subtitle_list);
    for (ii = 0; ii < count; ii++)
    {
        if (ii != index)
        {
            subtitle = ghb_array_get(subtitle_list, ii);
            ghb_settings_set_boolean(subtitle, "SubtitleDefaultTrack", FALSE);
        }
    }
}

void
ghb_subtitle_exclusive_default(signal_user_data_t *ud, gint index)
{
    ghb_subtitle_exclusive_default_settings(ud->settings, index);
    subtitle_refresh_list_ui(ud);
}

static void
ghb_add_subtitle_to_ui(signal_user_data_t *ud, GhbValue *subsettings)
{
    if (subsettings == NULL)
        return;

    // Add the current subtitle settings to the list.
    add_to_subtitle_list_ui(ud, subsettings);
    ghb_live_reset(ud);
}

static void
subtitle_add_to_settings(GhbValue *settings, GhbValue *subsettings)
{
    // Add the current subtitle settings to the list.
    GhbValue *subtitle_list;
    gint count;
    gboolean burned, forced, def;
    gint source;

    subtitle_list = ghb_settings_get_value(settings, "subtitle_list");
    if (subtitle_list == NULL)
    {
        subtitle_list = ghb_array_new();
        ghb_settings_set_value(settings, "subtitle_list", subtitle_list);
    }

    // Validate some settings
    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_settings_get_const_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    source = ghb_settings_get_int(subsettings, "SubtitleSource");
    burned = ghb_settings_get_boolean(subsettings, "SubtitleBurned");
    if (burned && !hb_subtitle_can_burn(source))
    {
        burned = FALSE;
        ghb_settings_set_boolean(subsettings, "SubtitleBurned", burned);
    }
    if (!burned && !hb_subtitle_can_pass(source, mux->format))
    {
        burned = TRUE;
        ghb_settings_set_boolean(subsettings, "SubtitleBurned", burned);
        ghb_settings_set_boolean(subsettings, "SubtitleDefaultTrack", FALSE);
    }
    def = ghb_settings_get_boolean(subsettings, "SubtitleDefaultTrack");
    forced = ghb_settings_get_boolean(subsettings, "SubtitleForced");
    if (forced && !hb_subtitle_can_force(source))
    {
        forced = FALSE;
        ghb_settings_set_boolean(subsettings, "SubtitleForced", forced);
    }

    ghb_array_append(subtitle_list, subsettings);

    // Check consistancy of exclusive flags
    count = ghb_array_len(subtitle_list);
    if (burned)
        ghb_subtitle_exclusive_burn_settings(settings, count-1);
    if (def)
        ghb_subtitle_exclusive_default_settings(settings, count-1);
}

static void
subtitle_set_track_description(GhbValue *settings, GhbValue *subsettings)
{
    char *desc = NULL;

    if (ghb_settings_get_int(subsettings, "SubtitleSource") == SRTSUB)
    {
        gchar *filename, *code;
        const gchar *lang;

        lang = ghb_settings_combo_option(subsettings, "SrtLanguage");
        code = ghb_settings_get_string(subsettings, "SrtCodeset");

        filename = ghb_settings_get_string(subsettings, "SrtFile");
        if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
        {
            gchar *basename;

            basename = g_path_get_basename(filename);
            desc = g_strdup_printf("%s (%s)(SRT)(%s)", lang, code, basename);
            g_free(basename);
        }
        else
        {
            desc = g_strdup_printf("%s (%s)(SRT)", lang, code);
        }
        g_free(code);
    }
    else
    {
        int title_id, titleindex;
        const hb_title_t *title;
        int track;
        hb_subtitle_t *subtitle;

        title_id = ghb_settings_get_int(settings, "title");
        title = ghb_lookup_title(title_id, &titleindex);
        track = ghb_settings_get_int(subsettings, "SubtitleTrack");
        if (track < 0)
        {
            desc = g_strdup(_("Foreign Audio Search"));
        }
        else
        {
            subtitle = ghb_get_subtitle_info(title, track);
            if (subtitle != NULL)
            {
                desc = g_strdup_printf("%d - %s (%s)", track + 1,
                                       subtitle->lang,
                                       hb_subsource_name(subtitle->source));
            }
        }
    }

    if (desc != NULL)
    {
        ghb_settings_set_string(
            subsettings, "SubtitleTrackDescription", desc);
    }
    else
    {
        ghb_settings_set_string(
            subsettings, "SubtitleTrackDescription", "Error!");
    }

    g_free(desc);
}

static GhbValue*  subtitle_add_track(
    signal_user_data_t *ud,
    GhbValue *settings,
    const hb_title_t *title,
    int track,
    int mux,
    gboolean default_track,
    gboolean srt,
    gboolean burn,
    gboolean *burned)
{
    int source = 0;

    if (track >= 0 && !srt)
    {
        hb_subtitle_t *subtitle = hb_list_item(title->list_subtitle, track);
        source = subtitle->source;
    }
    else if (srt)
    {
        source = SRTSUB;
    }

    burn |= !hb_subtitle_can_pass(source, mux);

    if (*burned && burn)
    {
        // Can only burn one.  Skip others that must be burned.
        return NULL;
    }

    GhbValue *subsettings = ghb_dict_new();
    ghb_settings_set_int(subsettings, "SubtitleTrack", track);
    ghb_settings_set_int(subsettings, "SubtitleSource", source);

    // Set default SRT settings
    gchar *pref_lang, *dir, *filename;

    pref_lang = ghb_settings_get_string(settings, "PreferredLanguage");
    ghb_settings_set_string(subsettings, "SrtLanguage", pref_lang);
    g_free(pref_lang);

    ghb_settings_set_string(subsettings, "SrtCodeset", "UTF-8");

    dir = ghb_settings_get_string(ud->prefs, "SrtDir");
    filename = g_strdup_printf("%s/none", dir);
    ghb_settings_set_string(subsettings, "SrtFile", filename);
    g_free(dir);
    g_free(filename);

    ghb_settings_set_int(subsettings, "SrtOffset", 0);

    subtitle_set_track_description(settings, subsettings);

    if (burn)
    {
        ghb_settings_set_boolean(subsettings, "SubtitleBurned", TRUE);
        if (track != -1)
        {
            // Allow 2 tracks to be marked burned when one is
            // foreign audio search.  Extra burned track will be
            // sanitized away if foreign audio search actually finds
            // something.
            *burned = TRUE;
        }
    }
    else
    {
        ghb_settings_set_boolean(subsettings, "SubtitleBurned", FALSE);
    }
    if (track == -1)
    {
        // Foreign audio search "track"
        ghb_settings_set_boolean(subsettings, "SubtitleForced", TRUE);
    }
    else
    {
        ghb_settings_set_boolean(subsettings, "SubtitleForced", FALSE);
    }
    if (default_track)
    {
        ghb_settings_set_boolean(subsettings, "SubtitleDefaultTrack", TRUE);
    }
    else
    {
        ghb_settings_set_boolean(subsettings, "SubtitleDefaultTrack", FALSE);
    }
    subtitle_add_to_settings(settings, subsettings);

    return subsettings;
}

void
ghb_subtitle_title_change(signal_user_data_t *ud, gboolean show)
{
    GtkWidget *w = GHB_WIDGET(ud->builder, "subtitle_add");
    gtk_widget_set_sensitive(w, show);
    w = GHB_WIDGET(ud->builder, "subtitle_add_all");
    gtk_widget_set_sensitive(w, show);
    w = GHB_WIDGET(ud->builder, "subtitle_reset");
    gtk_widget_set_sensitive(w, show);

    int title_id, titleindex;
    title_id = ghb_settings_get_int(ud->settings, "title");
    const hb_title_t *title = ghb_lookup_title(title_id, &titleindex);
    if (title != NULL)
    {
        w = GHB_WIDGET(ud->builder, "SubtitleSrtDisable");
        gtk_widget_set_sensitive(w, !!hb_list_count(title->list_subtitle));
    }
}

void
ghb_set_pref_subtitle_settings(signal_user_data_t *ud, const hb_title_t *title, GhbValue *settings)
{
    gint track;
    gboolean *used;
    const gchar *audio_lang, *pref_lang = NULL;
    gboolean foreign_audio_search, foreign_audio_subs;
    gboolean burn_foreign, burn_first, burn_dvd, burn_bd;
    gboolean one_burned = FALSE;

    const GhbValue *lang_list;
    gint lang_count, sub_count, ii;
    int behavior;

    behavior = ghb_settings_combo_int(settings,
                                      "SubtitleTrackSelectionBehavior");
    // Clear the subtitle list
    ghb_clear_subtitle_list_settings(settings);

    if (title == NULL)
    {
        // no source title
        return;
    }
    sub_count = hb_list_count(title->list_subtitle);
    if (sub_count == 0)
    {
        // No source subtitles
        return;
    }

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_settings_get_const_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    // Check to see if we need to add a subtitle track for foreign audio
    // language films. A subtitle track will be added if:
    //
    // The first (default) audio track language does NOT match the users
    // chosen Preferred Language AND the Preferred Language is NOT Any (und).
    //
    audio_lang = ghb_get_user_audio_lang(settings, title, 0);
    foreign_audio_search = ghb_settings_get_boolean(
                                settings, "SubtitleAddForeignAudioSearch");
    foreign_audio_subs = ghb_settings_get_boolean(
                                settings, "SubtitleAddForeignAudioSubtitle");
    lang_list = ghb_settings_get_value(settings, "SubtitleLanguageList");
    lang_count = ghb_array_len(lang_list);
    if (lang_count > 0)
    {
        GhbValue *glang = ghb_array_get(lang_list, 0);
        pref_lang = ghb_value_get_string(glang);
    }

    if (pref_lang == NULL || !strncmp(pref_lang, "und", 4))
    {
        foreign_audio_search = foreign_audio_subs = FALSE;
        pref_lang = NULL;
    }

    used = g_malloc0(sub_count * sizeof(gboolean));

    int burn_behavior;
    burn_behavior = ghb_settings_combo_int(settings, "SubtitleBurnBehavior");
    burn_foreign = burn_behavior == 1 || burn_behavior == 3;
    burn_first   = burn_behavior == 2 || burn_behavior == 3;
    burn_dvd = ghb_settings_get_boolean(settings, "SubtitleBurnDVDSub");
    burn_bd = ghb_settings_get_boolean(settings, "SubtitleBurnBDSub");

    if (foreign_audio_subs &&
        (audio_lang == NULL || strncmp(audio_lang, pref_lang, 4)))
    {
        // Add preferred language subtitle since first audio track
        // is foreign language.
        foreign_audio_search = FALSE;
        track = ghb_find_subtitle_track(title, pref_lang, 0);
        if (track >= 0)
        {
            gboolean burn;
            hb_subtitle_t *subtitle;
            subtitle = hb_list_item(title->list_subtitle, track);
            burn = (subtitle->source == VOBSUB && burn_dvd) ||
                   (subtitle->source == PGSSUB && burn_bd)  ||
                    burn_foreign || burn_first;
            used[track] = TRUE;
            subtitle_add_track(ud, settings, title, track, mux->format,
                               !burn, FALSE, burn, &one_burned);
            burn_first &= !burn;
        }
    }

    if (foreign_audio_search &&
        (audio_lang != NULL && !strncmp(audio_lang, pref_lang, 4)))
    {
        // Add search for foreign audio segments
        gboolean burn = burn_foreign || burn_first;
        subtitle_add_track(ud, settings, title, -1, mux->format,
                           !burn, FALSE, burn, &one_burned);
        burn_first &= !burn;
    }

    if (behavior != 0)
    {
        // Find "best" subtitle based on subtitle preferences
        for (ii = 0; ii < lang_count; ii++)
        {
            GhbValue *glang = ghb_array_get(lang_list, ii);
            const gchar *lang = ghb_value_get_string(glang);

            int next_track = 0;
            track = ghb_find_subtitle_track(title, lang, next_track);
            while (track >= 0)
            {
                if (!used[track])
                {
                    gboolean burn;
                    hb_subtitle_t *subtitle;
                    subtitle = hb_list_item(title->list_subtitle, track);
                    burn = (subtitle->source == VOBSUB && burn_dvd) ||
                           (subtitle->source == PGSSUB && burn_bd)  ||
                            burn_first;
                    used[track] = TRUE;
                    subtitle_add_track(ud, settings, title, track, mux->format,
                                       FALSE, FALSE, burn, &one_burned);
                    burn_first &= !burn;
                }
                next_track = track + 1;
                if (behavior == 2)
                {
                    track = ghb_find_subtitle_track(title, lang, next_track);
                }
                else
                {
                    break;
                }
            }
        }
    }

    if (ghb_settings_get_boolean(settings, "SubtitleAddCC"))
    {
        for (track = 0; track < sub_count; track++)
        {
            hb_subtitle_t *subtitle = hb_list_item(title->list_subtitle, track);
            if (subtitle->source == CC608SUB || subtitle->source == CC708SUB)
                break;
        }

        if (track < sub_count && !used[track])
        {
            used[track] = TRUE;
            subtitle_add_track(ud, settings, title, track, mux->format,
                               FALSE, FALSE, burn_first, &one_burned);
        }
    }
    g_free(used);
}

void
ghb_set_pref_subtitle(const hb_title_t *title, signal_user_data_t *ud)
{
    int sub_count;
    GtkWidget *widget;

    ghb_clear_subtitle_list_ui(ud->builder);
    if (title == NULL)
    {
        // Clear the subtitle list
        ghb_clear_subtitle_list_settings(ud->settings);
        return;
    }
    sub_count = hb_list_count(title->list_subtitle);
    if (sub_count == 0)
    {
        // No source subtitles
        widget = GHB_WIDGET(ud->builder, "SubtitleSrtDisable");
        gtk_widget_set_sensitive(widget, FALSE);
    }
    else
    {
        widget = GHB_WIDGET(ud->builder, "SubtitleSrtDisable");
        gtk_widget_set_sensitive(widget, TRUE);
    }
    ghb_set_pref_subtitle_settings(ud, title, ud->settings);
    subtitle_refresh_list_ui(ud);
}

gint
ghb_selected_subtitle_row(signal_user_data_t *ud)
{
    GtkTreeView *tv;
    GtkTreePath *tp;
    GtkTreeSelection *ts;
    GtkTreeModel *tm;
    GtkTreeIter iter;
    gint *indices;
    gint row = -1;

    g_debug("ghb_selected_subtitle_row ()");
    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list_view"));
    ts = gtk_tree_view_get_selection(tv);
    if (gtk_tree_selection_get_selected(ts, &tm, &iter))
    {
        // Get the row number
        tp = gtk_tree_model_get_path(tm, &iter);
        indices = gtk_tree_path_get_indices(tp);
        row = indices[0];
        gtk_tree_path_free(tp);
    }
    return row;
}

static GhbValue*
subtitle_get_selected_settings(signal_user_data_t *ud, int *index)
{
    GtkTreeView *tv;
    GtkTreePath *tp;
    GtkTreeSelection *ts;
    GtkTreeModel *tm;
    GtkTreeIter iter;
    gint *indices;
    gint row;
    GhbValue *subsettings = NULL;
    const GhbValue *subtitle_list;

    g_debug("get_selected_settings ()");
    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list_view"));
    ts = gtk_tree_view_get_selection(tv);
    if (gtk_tree_selection_get_selected(ts, &tm, &iter))
    {
        // Get the row number
        tp = gtk_tree_model_get_path(tm, &iter);
        indices = gtk_tree_path_get_indices(tp);
        row = indices[0];
        gtk_tree_path_free(tp);
        if (row < 0) return NULL;

        subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
        if (row >= ghb_array_len(subtitle_list))
            return NULL;

        subsettings = ghb_array_get(subtitle_list, row);
        if (index != NULL)
            *index = row;
    }
    return subsettings;
}

static void
subtitle_update_dialog_widgets(signal_user_data_t *ud, GhbValue *subsettings)
{
    GtkWidget *widget;

    if (subsettings != NULL)
    {
        // Update widgets with subsettings
        gboolean burn, force, def;

        const char *mux_id;
        const hb_container_t *mux;

        mux_id = ghb_settings_get_const_string(ud->settings, "FileFormat");
        mux = ghb_lookup_container_by_name(mux_id);

        int source = ghb_settings_get_int(subsettings, "SubtitleSource");

        ghb_ui_update_from_settings(ud, "SubtitleTrack", subsettings);
        ghb_ui_update_from_settings(ud, "SrtLanguage", subsettings);
        ghb_ui_update_from_settings(ud, "SrtCodeset", subsettings);
        ghb_ui_update_from_settings(ud, "SrtFile", subsettings);
        ghb_ui_update_from_settings(ud, "SrtOffset", subsettings);

        if (source == SRTSUB)
        {
            ghb_ui_update(ud, "SubtitleSrtEnable", ghb_boolean_value(TRUE));
        }
        else
        {
            ghb_ui_update(ud, "SubtitleSrtDisable", ghb_boolean_value(TRUE));
        }

        widget = GHB_WIDGET(ud->builder, "SubtitleBurned");
        gtk_widget_set_sensitive(widget, hb_subtitle_can_burn(source) &&
                                    hb_subtitle_can_pass(source, mux->format));
        widget = GHB_WIDGET(ud->builder, "SubtitleForced");
        gtk_widget_set_sensitive(widget, hb_subtitle_can_force(source));
        widget = GHB_WIDGET(ud->builder, "SubtitleDefaultTrack");
        gtk_widget_set_sensitive(widget,
                                    hb_subtitle_can_pass(source, mux->format));

        burn = ghb_settings_get_int(subsettings, "SubtitleBurned");
        force = ghb_settings_get_int(subsettings, "SubtitleForced");
        def = ghb_settings_get_int(subsettings, "SubtitleDefaultTrack");

        if (!hb_subtitle_can_burn(source))
        {
            burn = FALSE;
        }
        if (!hb_subtitle_can_force(source))
        {
            force = FALSE;
        }
        if (!hb_subtitle_can_pass(source, mux->format))
        {
            def = FALSE;
            burn = TRUE;
        }
        ghb_settings_set_boolean(subsettings, "SubtitleBurned", burn);
        ghb_ui_update(ud, "SubtitleBurned", ghb_boolean_value(burn));
        ghb_settings_set_boolean(subsettings, "SubtitleForced", force);
        ghb_ui_update(ud, "SubtitleForced", ghb_boolean_value(force));
        ghb_settings_set_boolean(subsettings, "SubtitleDefaultTrack", def);
        ghb_ui_update(ud, "SubtitleDefaultTrack", ghb_boolean_value(def));

        // Hide regular subtitle widgets
        widget = GHB_WIDGET(ud->builder, "subtitle_track_box");
        gtk_widget_set_visible(widget, source != SRTSUB);

        // Show SRT subitle widgets
        widget = GHB_WIDGET(ud->builder, "subtitle_srt_grid");
        gtk_widget_set_visible(widget, source == SRTSUB);
    }
    else
    {
        // Hide SRT subitle widgets
        widget = GHB_WIDGET(ud->builder, "subtitle_srt_grid");
        gtk_widget_set_visible(widget, FALSE);

        // Show regular subtitle widgets
        widget = GHB_WIDGET(ud->builder, "subtitle_track_box");
        gtk_widget_set_visible(widget, TRUE);
    }
}

static GhbValue*
subtitle_update_setting(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *subsettings;

    ghb_widget_to_setting(ud->settings, widget);
    subsettings = subtitle_get_selected_settings(ud, NULL);
    if (subsettings != NULL)
    {
        ghb_widget_to_setting(subsettings, widget);
        ghb_subtitle_list_refresh_selected(ud);
        ghb_live_reset(ud);
    }
    return subsettings;
}

G_MODULE_EXPORT void
subtitle_track_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *subsettings;

    g_debug("subtitle_track_changed_cb()");
    ghb_widget_to_setting(ud->settings, widget);
    subsettings = subtitle_get_selected_settings(ud, NULL);
    if (subsettings != NULL)
    {
        gint track, source;

        ghb_widget_to_setting(subsettings, widget);
        track = ghb_settings_get_int(subsettings, "SubtitleTrack");
        source = ghb_subtitle_track_source(ud->settings, track);
        ghb_settings_set_int(subsettings, "SubtitleSource", source);
        subtitle_set_track_description(ud->settings, subsettings);
        subtitle_update_dialog_widgets(ud, subsettings);
        ghb_subtitle_list_refresh_selected(ud);
        ghb_live_reset(ud);
    }
}

G_MODULE_EXPORT void
subtitle_forced_toggled_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    subtitle_update_setting(widget, ud);
}

G_MODULE_EXPORT void
subtitle_burned_toggled_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *subsettings;
    int index;

    ghb_widget_to_setting(ud->settings, widget);
    subsettings = subtitle_get_selected_settings(ud, &index);
    if (subsettings != NULL)
    {
        ghb_widget_to_setting(subsettings, widget);
        if (ghb_settings_get_boolean(subsettings, "SubtitleBurned"))
        {
            ghb_ui_update(ud, "SubtitleDefaultTrack", ghb_boolean_value(FALSE));
            ghb_subtitle_exclusive_burn(ud, index);
        }
        ghb_subtitle_list_refresh_selected(ud);
        ghb_live_reset(ud);
    }
}

G_MODULE_EXPORT void
subtitle_default_toggled_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *subsettings;
    int index;

    ghb_widget_to_setting(ud->settings, widget);
    subsettings = subtitle_get_selected_settings(ud, &index);
    if (subsettings != NULL)
    {
        ghb_widget_to_setting(subsettings, widget);
        if (ghb_settings_get_boolean(subsettings, "SubtitleDefaultTrack"))
        {
            ghb_ui_update(ud, "SubtitleBurned", ghb_boolean_value(FALSE));
            ghb_subtitle_exclusive_default(ud, index);
        }
        ghb_subtitle_list_refresh_selected(ud);
        ghb_live_reset(ud);
    }
}

G_MODULE_EXPORT void
subtitle_srt_radio_toggled_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *subsettings;

    ghb_widget_to_setting(ud->settings, widget);
    subsettings = subtitle_get_selected_settings(ud, NULL);
    if (subsettings != NULL)
    {
        if (ghb_settings_get_boolean(ud->settings, "SubtitleSrtEnable"))
        {
            ghb_settings_set_int(subsettings, "SubtitleSource", SRTSUB);
        }
        else
        {
            int track, source;

            track = ghb_settings_get_int(subsettings, "SubtitleTrack");
            source = ghb_subtitle_track_source(ud->settings, track);
            ghb_settings_set_int(subsettings, "SubtitleSource", source);
        }
        subtitle_set_track_description(ud->settings, subsettings);
        subtitle_update_dialog_widgets(ud, subsettings);
        ghb_subtitle_list_refresh_selected(ud);
        ghb_live_reset(ud);
    }
}

void
ghb_subtitle_list_refresh_selected(signal_user_data_t *ud)
{
    GtkTreeView *tv;
    GtkTreeModel *tm;
    GtkTreePath *tp;
    GtkTreeSelection *ts;
    GtkTreeIter ti;
    gint *indices;
    gint row;
    GhbValue *subsettings = NULL;
    const GhbValue *subtitle_list;

    g_debug("subtitle_list_refresh_selected()");
    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list_view"));
    ts = gtk_tree_view_get_selection(tv);
    if (gtk_tree_selection_get_selected(ts, &tm, &ti))
    {
        // Get the row number
        tp = gtk_tree_model_get_path(tm, &ti);
        indices = gtk_tree_path_get_indices(tp);
        row = indices[0];
        gtk_tree_path_free(tp);
        if (row < 0) return;

        subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
        if (row >= ghb_array_len(subtitle_list))
            return;

        subsettings = ghb_array_get(subtitle_list, row);
        subtitle_refresh_list_row_ui(tm, &ti, subsettings);
    }
}

void
ghb_subtitle_list_refresh_all(signal_user_data_t *ud)
{
    subtitle_refresh_list_ui(ud);
}

G_MODULE_EXPORT void
srt_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *subsettings;

    g_debug("srt_changed_cb()");
    ghb_check_dependency(ud, widget, NULL);
    ghb_widget_to_setting(ud->settings, widget);
    subsettings = subtitle_get_selected_settings(ud, NULL);
    if (subsettings != NULL)
    {
        ghb_widget_to_setting(subsettings, widget);
        subtitle_set_track_description(ud->settings, subsettings);
        ghb_subtitle_list_refresh_selected(ud);
        ghb_live_reset(ud);
    }
}

G_MODULE_EXPORT void
srt_file_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *subsettings;

    g_debug("srt_file_changed_cb()");
    ghb_check_dependency(ud, widget, NULL);
    ghb_widget_to_setting(ud->settings, widget);
    subsettings = subtitle_get_selected_settings(ud, NULL);
    if (subsettings != NULL)
    {
        gchar *filename, *dirname;

        ghb_widget_to_setting(subsettings, widget);
        subtitle_set_track_description(ud->settings, subsettings);
        ghb_subtitle_list_refresh_selected(ud);
        ghb_live_reset(ud);

        // Update SrtDir preference
        filename = ghb_settings_get_string(subsettings, "SrtFile");
        if (g_file_test(filename, G_FILE_TEST_IS_DIR))
        {
            ghb_settings_set_string(ud->prefs, "SrtDir", filename);
        }
        else
        {
            dirname = g_path_get_dirname(filename);
            ghb_settings_set_string(ud->prefs, "SrtDir", dirname);
            g_free(dirname);
        }
        ghb_pref_save(ud->prefs, "SrtDir");
        g_free(filename);
    }
}

G_MODULE_EXPORT void
srt_lang_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *subsettings;

    g_debug("srt_lang_changed_cb()");
    ghb_check_dependency(ud, widget, NULL);
    ghb_widget_to_setting(ud->settings, widget);
    subsettings = subtitle_get_selected_settings(ud, NULL);
    if (subsettings != NULL)
    {
        ghb_widget_to_setting(subsettings, widget);
        subtitle_set_track_description(ud->settings, subsettings);
        ghb_subtitle_list_refresh_selected(ud);
        ghb_live_reset(ud);
    }
}

static void
ghb_clear_subtitle_list_settings(GhbValue *settings)
{
    GhbValue *subtitle_list;

    subtitle_list = ghb_settings_get_value(settings, "subtitle_list");
    if (subtitle_list == NULL)
    {
        subtitle_list = ghb_array_new();
        ghb_settings_set_value(settings, "subtitle_list", subtitle_list);
    }
    else
        ghb_array_reset(subtitle_list);
}

static void
ghb_clear_subtitle_list_ui(GtkBuilder *builder)
{
    GtkTreeView *tv;
    GtkTreeStore *ts;
    GtkTreeSelection *tsel;

    tv = GTK_TREE_VIEW(GHB_WIDGET(builder, "subtitle_list_view"));
    ts = GTK_TREE_STORE(gtk_tree_view_get_model(tv));
    // Clear tree selection so that updates are not triggered
    // that cause a recursive attempt to clear the tree selection (crasher)
    tsel = gtk_tree_view_get_selection(tv);
    gtk_tree_selection_unselect_all(tsel);
    gtk_tree_store_clear(ts);
}

static void
add_to_subtitle_list_ui(signal_user_data_t *ud, GhbValue *subsettings)
{
    GtkTreeView *tv;
    GtkTreeIter ti;
    GtkTreeModel *tm;
    GtkTreeSelection *ts;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list_view"));
    ts = gtk_tree_view_get_selection(tv);
    tm = gtk_tree_view_get_model(tv);

    gtk_tree_store_append(GTK_TREE_STORE(tm), &ti, NULL);
    subtitle_refresh_list_row_ui(tm, &ti, subsettings);

    gtk_tree_selection_select_iter(ts, &ti);
}

G_MODULE_EXPORT void
subtitle_list_selection_changed_cb(GtkTreeSelection *ts, signal_user_data_t *ud)
{
    GtkTreeModel *tm;
    GtkTreeIter iter;
    GhbValue *subsettings = NULL;
    int row;

    g_debug("subtitle_list_selection_changed_cb()");
    if (gtk_tree_selection_get_selected(ts, &tm, &iter))
    {
        GtkTreeIter piter;

        if (gtk_tree_model_iter_parent(tm, &piter, &iter))
        {
            GtkTreePath *path;
            GtkTreeView *tv;

            gtk_tree_selection_select_iter(ts, &piter);
            path = gtk_tree_model_get_path(tm, &piter);
            tv = gtk_tree_selection_get_tree_view(ts);
            // Make the parent visible in scroll window if it is not.
            gtk_tree_view_scroll_to_cell(tv, path, NULL, FALSE, 0, 0);
            gtk_tree_path_free(path);
            return;
        }

        GtkTreePath *tp;
        gint *indices;
        GhbValue *subtitle_list;

        tp = gtk_tree_model_get_path(tm, &iter);
        indices = gtk_tree_path_get_indices(tp);
        row = indices[0];
        gtk_tree_path_free(tp);

        subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
        if (row >= 0 && row < ghb_array_len(subtitle_list))
            subsettings = ghb_array_get(subtitle_list, row);
    }
    subtitle_update_dialog_widgets(ud, subsettings);
    if (subsettings)
    {
        if (ghb_settings_get_boolean(subsettings, "SubtitleBurned"))
        {
            ghb_subtitle_exclusive_burn(ud, row);
        }
    }
}

static gboolean subtitle_is_one_burned(GhbValue *settings)
{
    GhbValue *subtitle_list, *subsettings;
    int count, ii;

    subtitle_list = ghb_settings_get_value(settings, "subtitle_list");
    if (subtitle_list == NULL)
        return FALSE;

    count = ghb_array_len(subtitle_list);
    for (ii = 0; ii < count; ii++)
    {
        subsettings = ghb_array_get(subtitle_list, ii);
        if (ghb_settings_get_boolean(subsettings, "SubtitleBurned"))
        {
            return TRUE;
        }
    }
    return FALSE;
}

G_MODULE_EXPORT void
subtitle_add_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    // Add the current subtitle settings to the list.
    GhbValue *subsettings, *backup;
    gboolean one_burned;
    gint track;

    int title_id, titleindex;
    const hb_title_t *title;

    title_id = ghb_settings_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL)
    {
        return;
    }

    // Back up settings in case we need to revert.
    backup = ghb_value_dup(
                ghb_settings_get_value(ud->settings, "subtitle_list"));

    one_burned = subtitle_is_one_burned(ud->settings);

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_settings_get_const_string(ud->settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    int count = hb_list_count(title->list_subtitle);
    for (subsettings = NULL, track = 0;
         subsettings == NULL && track < count; track++)
    {
        subsettings = subtitle_add_track(ud, ud->settings, title, track,
                                mux->format, FALSE, FALSE, FALSE, &one_burned);
    }
    if (subsettings == NULL)
    {
        subsettings = subtitle_add_track(ud, ud->settings, title, 0,
                                mux->format, FALSE, TRUE, FALSE, &one_burned);
    }
    ghb_add_subtitle_to_ui(ud, subsettings);

    if (subsettings != NULL)
    {
        // Pop up the edit dialog
        GtkResponseType response;
        GtkWidget *dialog = GHB_WIDGET(ud->builder, "subtitle_dialog");
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_hide(dialog);
        if (response != GTK_RESPONSE_OK)
        {
            ghb_settings_take_value(ud->settings, "subtitle_list", backup);
            subsettings = subtitle_get_selected_settings(ud, NULL);
            if (subsettings != NULL)
            {
                subtitle_update_dialog_widgets(ud, subsettings);
            }
            subtitle_refresh_list_ui(ud);
        }
        else
        {
            ghb_value_free(&backup);
        }
    }
}

G_MODULE_EXPORT void
subtitle_add_all_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    // Add the current subtitle settings to the list.
    gboolean one_burned = FALSE;
    gint track;

    const hb_title_t *title;
    int title_id, titleindex;
    title_id = ghb_settings_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL)
    {
        return;
    }

    ghb_clear_subtitle_list_settings(ud->settings);
    ghb_clear_subtitle_list_ui(ud->builder);

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_settings_get_const_string(ud->settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    int count = hb_list_count(title->list_subtitle);
    for (track = 0; track < count; track++)
    {
        subtitle_add_track(ud, ud->settings, title, track, mux->format,
                           FALSE, FALSE, FALSE, &one_burned);
    }
    subtitle_refresh_list_ui(ud);
    ghb_live_reset(ud);
}

G_MODULE_EXPORT void
subtitle_reset_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    int title_id, titleindex;
    const hb_title_t *title;

    title_id = ghb_settings_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    ghb_set_pref_subtitle(title, ud);
}

void
ghb_subtitle_prune(signal_user_data_t *ud)
{
    GhbValue *subtitle_list;
    GhbValue *subsettings;
    gint ii;
    gboolean one_burned = FALSE;

    subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");
    if (subtitle_list == NULL)
        return;

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_settings_get_const_string(ud->settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    for (ii = 0; ii < ghb_array_len(subtitle_list); )
    {
        gboolean burned;
        int source;

        subsettings = ghb_array_get(subtitle_list, ii);
        burned = ghb_settings_get_boolean(subsettings, "SubtitleBurned");
        source = ghb_settings_get_boolean(subsettings, "SubtitleSource");
        burned = burned || !hb_subtitle_can_pass(source, mux->format);
        if (burned && one_burned)
        {
            ghb_array_remove(subtitle_list, ii);
            continue;
        }
        one_burned = one_burned || burned;
        ghb_settings_set_boolean(subsettings, "SubtitleBurned", burned);
        ii++;
    }
    subsettings = subtitle_get_selected_settings(ud, NULL);
    if (subsettings != NULL)
    {
        subtitle_update_dialog_widgets(ud, subsettings);
    }
}

void
ghb_reset_subtitles(signal_user_data_t *ud, GhbValue *settings)
{
    GhbValue *slist;
    GhbValue *subtitle;
    gint count, ii;
    gint title_id, titleindex;
    const hb_title_t *title;

    g_debug("ghb_reset_subtitles");
    ghb_clear_subtitle_list_settings(ud->settings);
    ghb_clear_subtitle_list_ui(ud->builder);
    title_id = ghb_settings_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL)
        return;

    slist = ghb_settings_get_value(settings, "subtitle_list");
    count = ghb_array_len(slist);
    for (ii = 0; ii < count; ii++)
    {
        subtitle = ghb_value_dup(ghb_array_get(slist, ii));
        subtitle_add_to_settings(ud->settings, subtitle);
    }
    subtitle_refresh_list_ui(ud);
}

G_MODULE_EXPORT void
subtitle_def_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_clear_presets_selection(ud);
}

static void
subtitle_update_pref_lang(signal_user_data_t *ud, const iso639_lang_t *lang)
{
    GtkLabel *label;
    GtkButton *button;
    gchar *str;
    const char * name = _("None");
    const char * code = "und";

    label = GTK_LABEL(GHB_WIDGET(ud->builder, "subtitle_preferred_language"));
    if (lang != NULL)
    {
        code = lang->iso639_2;
        if (strncmp(code, "und", 4))
        {
            name = lang->native_name && lang->native_name[0] ?
                                lang->native_name : lang->eng_name;
        }
    }

    str = g_strdup_printf(_("Preferred Language: %s"), name);
    gtk_label_set_text(label, str);
    g_free(str);

    ghb_settings_set_string(ud->settings, "PreferredLanguage", code);

    button = GTK_BUTTON(GHB_WIDGET(ud->builder,
                                  "SubtitleAddForeignAudioSubtitle"));
    str = g_strdup_printf(_("Add %s subtitle track if default audio is not %s"),
                          name, name);
    gtk_button_set_label(button, str);
    g_free(str);

    // If there is no preferred language, hide options that require
    // a preferred language to be set.
    gboolean visible = !(lang == NULL || !strncmp(code, "und", 4));
    gtk_widget_set_visible(GTK_WIDGET(button), visible);
    button = GTK_BUTTON(GHB_WIDGET(ud->builder,
                                  "SubtitleAddForeignAudioSearch"));
    gtk_widget_set_visible(GTK_WIDGET(button), visible);
}

void
ghb_subtitle_set_pref_lang(GhbValue *settings)
{
    GhbValue *lang_list;
    gboolean set = FALSE;
    lang_list = ghb_settings_get_value(settings, "SubtitleLanguageList");
    if (ghb_array_len(lang_list) > 0)
    {
        GhbValue *glang = ghb_array_get(lang_list, 0);
        if (glang != NULL)
        {
            ghb_settings_set_string(settings, "PreferredLanguage",
                                    ghb_value_get_string(glang));
            set = TRUE;
        }
    }
    if (!set)
    {
        ghb_settings_set_string(settings, "PreferredLanguage", "und");
    }
}

G_MODULE_EXPORT void
subtitle_add_lang_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GtkListBox *avail = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "subtitle_avail_lang"));
    GtkListBox *selected = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "subtitle_selected_lang"));
    GtkListBoxRow *row;
    GtkWidget *label;

    row = gtk_list_box_get_selected_row(avail);
    if (row != NULL)
    {
        int idx;
        const iso639_lang_t *lang;
        GhbValue *glang, *lang_list;

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
        lang_list = ghb_settings_get_value(ud->settings, "SubtitleLanguageList");
        if (ghb_array_len(lang_list) == 0)
        {
            subtitle_update_pref_lang(ud, lang);
        }
        ghb_array_append(lang_list, glang);
        ghb_clear_presets_selection(ud);
    }
}

G_MODULE_EXPORT void
subtitle_remove_lang_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{

    GtkListBox *avail, *selected;
    GtkListBoxRow *row;
    GtkWidget *label;

    avail = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "subtitle_avail_lang"));
    selected = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "subtitle_selected_lang"));
    row = gtk_list_box_get_selected_row(selected);
    if (row != NULL)
    {
        gint index;
        GhbValue *lang_list;

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
        lang_list = ghb_settings_get_value(ud->settings, "SubtitleLanguageList");
        ghb_array_remove(lang_list, index);

        ghb_clear_presets_selection(ud);

        if (ghb_array_len(lang_list) > 0)
        {
            const iso639_lang_t *lang;
            GhbValue *entry = ghb_array_get(lang_list, 0);
            lang = ghb_iso639_lookup_by_int(ghb_lookup_audio_lang(entry));
            subtitle_update_pref_lang(ud, lang);
        }
        else
        {
            subtitle_update_pref_lang(ud, NULL);
        }
    }
}

static void subtitle_def_lang_list_clear_cb(GtkWidget *row, gpointer data)
{
    GtkListBox *avail = (GtkListBox*)data;
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(row));
    g_object_ref(G_OBJECT(label));
    gtk_widget_destroy(GTK_WIDGET(row));
    gtk_widget_show(label);
    int idx = (intptr_t)g_object_get_data(G_OBJECT(label), "lang_idx");
    gtk_list_box_insert(avail, label, idx);
}

static void subtitle_def_selected_lang_list_clear(signal_user_data_t *ud)
{
    GtkListBox *avail, *selected;
    avail = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "subtitle_avail_lang"));
    selected = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "subtitle_selected_lang"));
    gtk_container_foreach(GTK_CONTAINER(selected),
                          subtitle_def_lang_list_clear_cb, (gpointer)avail);
}

static void subtitle_def_lang_list_init(signal_user_data_t *ud)
{
    GhbValue *lang_list;

    // Clear selected languages.
    subtitle_def_selected_lang_list_clear(ud);

    lang_list = ghb_settings_get_value(ud->settings, "SubtitleLanguageList");
    if (lang_list == NULL)
    {
        lang_list = ghb_array_new();
        ghb_settings_set_value(ud->settings, "SubtitleLanguageList", lang_list);
    }

    int ii, count;
    count = ghb_array_len(lang_list);
    for (ii = 0; ii < count; )
    {
        GhbValue *lang_val = ghb_array_get(lang_list, ii);
        int idx = ghb_lookup_audio_lang(lang_val);
        if (ii == 0)
        {
            const iso639_lang_t *lang;
            lang = ghb_iso639_lookup_by_int(idx);
            subtitle_update_pref_lang(ud, lang);
        }

        GtkListBox *avail, *selected;
        GtkListBoxRow *row;
        avail = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "subtitle_avail_lang"));
        selected = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "subtitle_selected_lang"));
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
    if (count == 0)
    {
        subtitle_update_pref_lang(ud, NULL);
    }
}

void ghb_subtitle_defaults_to_ui(signal_user_data_t *ud)
{
    subtitle_def_lang_list_init(ud);
}

void ghb_init_subtitle_defaults_ui(signal_user_data_t *ud)
{
    GtkListBox *list_box;

    list_box = GTK_LIST_BOX(GHB_WIDGET(ud->builder, "subtitle_avail_lang"));
    ghb_init_lang_list_box(list_box);
}

G_MODULE_EXPORT void
subtitle_edit_clicked_cb(GtkWidget *widget, gchar *path, signal_user_data_t *ud)
{
    GtkTreeView *tv;
    GtkTreePath *tp;
    GtkTreeModel *tm;
    GtkTreeSelection *ts;
    GtkTreeIter ti;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list_view"));
    ts = gtk_tree_view_get_selection(tv);
    tm = gtk_tree_view_get_model(tv);
    tp = gtk_tree_path_new_from_string (path);
    if (gtk_tree_path_get_depth(tp) > 1) return;
    if (gtk_tree_model_get_iter(tm, &ti, tp))
    {
        GhbValue *subsettings, *backup;

        gtk_tree_selection_select_iter(ts, &ti);

        // Back up settings in case we need to revert.
        backup = ghb_value_dup(
                    ghb_settings_get_value(ud->settings, "subtitle_list"));

        // Pop up the edit dialog
        GtkResponseType response;
        GtkWidget *dialog = GHB_WIDGET(ud->builder, "subtitle_dialog");
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_hide(dialog);
        if (response != GTK_RESPONSE_OK)
        {
            ghb_settings_take_value(ud->settings, "subtitle_list", backup);
            subsettings = subtitle_get_selected_settings(ud, NULL);
            if (subsettings != NULL)
            {
                subtitle_update_dialog_widgets(ud, subsettings);
            }
            subtitle_refresh_list_ui(ud);
        }
        else
        {
            ghb_value_free(&backup);
        }
    }
}

G_MODULE_EXPORT void
subtitle_remove_clicked_cb(GtkWidget *widget, gchar *path, signal_user_data_t *ud)
{
    GtkTreeView *tv;
    GtkTreePath *tp;
    GtkTreeModel *tm;
    GtkTreeSelection *ts;
    GtkTreeIter ti, nextIter;
    gint row;
    gint *indices;
    GhbValue *subtitle_list;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list_view"));
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

        subtitle_list = ghb_settings_get_value(ud->settings, "subtitle_list");

        // Get the row number
        indices = gtk_tree_path_get_indices (tp);
        row = indices[0];
        if (row < 0 || row >= ghb_array_len(subtitle_list))
        {
            gtk_tree_path_free(tp);
            return;
        }

        // Update our settings list before removing the row from the
        // treeview.  Removing from the treeview sometimes provokes an
        // immediate selection change, so the list needs to be up to date
        // when this happens.
        ghb_array_remove(subtitle_list, row);

        // Remove the selected item
        gtk_tree_store_remove(GTK_TREE_STORE(tm), &ti);

        ghb_live_reset(ud);
    }
    gtk_tree_path_free(tp);
}

