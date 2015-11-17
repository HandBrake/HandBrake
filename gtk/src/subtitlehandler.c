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

static void subtitle_set_track_description(GhbValue *settings,
                                           GhbValue *subsettings);
static void subtitle_list_refresh_selected(signal_user_data_t *ud,
                                           GhbValue *subsettings);
static void subtitle_add_to_settings(GhbValue *settings, GhbValue *subsettings);
static void add_subtitle_to_ui(signal_user_data_t *ud, GhbValue *subsettings);
static void add_to_subtitle_list_ui(signal_user_data_t *ud, GhbValue *settings);
static void clear_subtitle_list_settings(GhbValue *settings);
static void clear_subtitle_list_ui(GtkBuilder *builder);

static int get_sub_source(GhbValue *settings, GhbValue *subsettings)
{
    if (ghb_dict_get(subsettings, "SRT") != NULL)
    {
        return SRTSUB;
    }

    int title_id = ghb_dict_get_int(settings, "title");
    const hb_title_t *title = ghb_lookup_title(title_id, NULL);
    if (title == NULL) // No title, scan failure?
        return VOBSUB;

    GhbValue *val = ghb_dict_get(subsettings, "Track");
    if (val == NULL) // No track, foreign audio search
        return VOBSUB;

    int track = ghb_dict_get_int(subsettings, "Track");
    hb_subtitle_t *subtitle = hb_list_item(title->list_subtitle, track);
    if (subtitle != NULL) // Invalid track, shouldn't happen
        return subtitle->source;

    return VOBSUB;
}

static void
subtitle_refresh_list_row_ui(
    GtkTreeModel *tm,
    GtkTreeIter *ti,
    GhbValue *settings,
    GhbValue *subsettings)
{
    GtkTreeIter cti;
    gboolean forced, burned, def;
    char *info_src, *info_src_2;
    char *info_dst, *info_dst_2;
    const char *desc;


    info_src_2 = NULL;
    info_dst_2 = NULL;

    forced = ghb_dict_get_bool(subsettings, "Forced");
    burned = ghb_dict_get_bool(subsettings, "Burn");
    def = ghb_dict_get_bool(subsettings, "Default");
    desc = ghb_dict_get_string(subsettings, "Description");
    if (desc == NULL)
    {
        subtitle_set_track_description(settings, subsettings);
        desc = ghb_dict_get_string(subsettings, "Description");
    }
    info_src = g_strdup_printf("<small>%s</small>", desc);
    if (ghb_dict_get(subsettings, "SRT") != NULL)
    {
        gint offset;
        offset = ghb_dict_get_int(subsettings, "Offset");
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
    GtkTreeView  *tv;
    GtkTreeModel *tm;
    GtkTreeIter   ti;
    GhbValue *subtitle_list, *subtitle_search;
    GhbValue *subsettings;
    gint ii, count, tm_count;
    gboolean search;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list_view"));
    tm = gtk_tree_view_get_model(tv);

    tm_count = gtk_tree_model_iter_n_children(tm, NULL);

    subtitle_list = ghb_get_job_subtitle_list(settings);
    subtitle_search = ghb_get_job_subtitle_search(settings);
    search = ghb_dict_get_bool(subtitle_search, "Enable");
    count = ghb_array_len(subtitle_list);
    if (count + search != tm_count)
    {
        clear_subtitle_list_ui(ud->builder);
        for (ii = 0; ii < count + search; ii++)
        {
            gtk_tree_store_append(GTK_TREE_STORE(tm), &ti, NULL);
        }
    }
    // Enable or Disabel FAS button
    GtkWidget *w = GHB_WIDGET(ud->builder, "subtitle_add_fas");
    gtk_widget_set_sensitive(w, !search);
    if (search)
    {

        gtk_tree_model_iter_nth_child(tm, &ti, NULL, 0);
        subtitle_refresh_list_row_ui(tm, &ti, ud->settings, subtitle_search);
    }
    for (ii = 0; ii < count; ii++)
    {
        gtk_tree_model_iter_nth_child(tm, &ti, NULL, ii + search);
        subsettings = ghb_array_get(subtitle_list, ii);
        subtitle_refresh_list_row_ui(tm, &ti, ud->settings, subsettings);
    }
}

static void
subtitle_refresh_list_ui(signal_user_data_t *ud)
{
    subtitle_refresh_list_ui_from_settings(ud, ud->settings);
}

static void
subtitle_exclusive_burn_settings(GhbValue *settings, gint index)
{
    GhbValue *subtitle_list;
    GhbValue *subsettings;
    gint ii, count;

    subtitle_list = ghb_get_job_subtitle_list(settings);
    count = ghb_array_len(subtitle_list);
    for (ii = 0; ii < count; ii++)
    {
        if (ii != index)
        {
            subsettings = ghb_array_get(subtitle_list, ii);
            ghb_dict_set_bool(subsettings, "Burn", FALSE);
        }
    }
}

static void
subtitle_exclusive_burn(signal_user_data_t *ud, gint index)
{
    subtitle_exclusive_burn_settings(ud->settings, index);
    subtitle_refresh_list_ui(ud);
}

static void
subtitle_exclusive_default_settings(GhbValue *settings, gint index)
{
    GhbValue *subtitle_list;
    GhbValue *subtitle;
    gint ii, count;

    subtitle_list = ghb_get_job_subtitle_list(settings);
    count = ghb_array_len(subtitle_list);
    for (ii = 0; ii < count; ii++)
    {
        if (ii != index)
        {
            subtitle = ghb_array_get(subtitle_list, ii);
            ghb_dict_set_bool(subtitle, "Default", FALSE);
        }
    }
}

static void
subtitle_exclusive_default(signal_user_data_t *ud, gint index)
{
    subtitle_exclusive_default_settings(ud->settings, index);
    subtitle_refresh_list_ui(ud);
}

static void
add_subtitle_to_ui(signal_user_data_t *ud, GhbValue *subsettings)
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
    gboolean burn, forced, def;
    gint source;

    subtitle_list = ghb_get_job_subtitle_list(settings);
    if (subtitle_list == NULL)
    {
        g_warning("No subtitle list!");
        return;
    }

    // Validate some settings
    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    source = get_sub_source(settings, subsettings);
    burn = ghb_dict_get_bool(subsettings, "Burn");
    if (burn && !hb_subtitle_can_burn(source))
    {
        burn = FALSE;
        ghb_dict_set_bool(subsettings, "Burn", burn);
    }
    if (!burn && !hb_subtitle_can_pass(source, mux->format))
    {
        burn = TRUE;
        ghb_dict_set_bool(subsettings, "Burn", burn);
        ghb_dict_set_bool(subsettings, "Default", FALSE);
    }
    def = ghb_dict_get_bool(subsettings, "Default");
    forced = ghb_dict_get_bool(subsettings, "Forced");
    if (forced && !hb_subtitle_can_force(source))
    {
        forced = FALSE;
        ghb_dict_set_bool(subsettings, "Forced", forced);
    }

    ghb_array_append(subtitle_list, subsettings);

    // Check consistancy of exclusive flags
    count = ghb_array_len(subtitle_list);
    if (burn)
        subtitle_exclusive_burn_settings(settings, count-1);
    if (def)
        subtitle_exclusive_default_settings(settings, count-1);
}

static void
subtitle_set_track_description(GhbValue *settings, GhbValue *subsettings)
{
    GhbValue *srt;
    char *desc = NULL;

    srt = ghb_dict_get(subsettings, "SRT");
    if (srt != NULL)
    {
        const gchar *filename, *code;
        const gchar *lang;
        const iso639_lang_t *iso;

        lang = ghb_dict_get_string(srt, "Language");
        code = ghb_dict_get_string(srt, "Codeset");
        filename = ghb_dict_get_string(srt, "Filename");

        iso = lang_lookup(lang);
        if (iso != NULL)
        {
            if (iso->native_name != NULL)
                lang = iso->native_name;
            else
                lang = iso->eng_name;
        }

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
    }
    else
    {
        int title_id, titleindex;
        const hb_title_t *title;
        int track;
        hb_subtitle_t *subtitle;
        GhbValue *val;

        title_id = ghb_dict_get_int(settings, "title");
        title = ghb_lookup_title(title_id, &titleindex);
        val = ghb_dict_get(subsettings, "Track");
        track = ghb_dict_get_int(subsettings, "Track");
        if (val == NULL || track < 0)
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
        ghb_dict_set_string(subsettings, "Description", desc);
    }
    else
    {
        ghb_dict_set_string(subsettings, "Description", "Error!");
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
    int source = VOBSUB;

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
    if (srt)
    {
        // Set default SRT settings
        GhbValue *srt_dict;
        const gchar *pref_lang, *dir;
        gchar *filename;

        srt_dict = ghb_dict_new();
        hb_dict_set(subsettings, "SRT", srt_dict);

        pref_lang = ghb_dict_get_string(settings, "PreferredLanguage");
        ghb_dict_set_string(srt_dict, "Language", pref_lang);
        ghb_dict_set_string(srt_dict, "Codeset", "UTF-8");

        dir = ghb_dict_get_string(ud->prefs, "SrtDir");
        filename = g_strdup_printf("%s/none", dir);
        ghb_dict_set_string(srt_dict, "Filename", filename);
        g_free(filename);
    }

    ghb_dict_set_int(subsettings, "Track", track);
    ghb_dict_set_int(subsettings, "Offset", 0);
    ghb_dict_set_bool(subsettings, "Forced", track == -1);
    ghb_dict_set_bool(subsettings, "Default", default_track);
    ghb_dict_set_bool(subsettings, "Burn", burn);
    if (burn && track != -1)
    {
        // Allow 2 tracks to be marked burned when one is
        // foreign audio search.  Extra burned track will be
        // sanitized away if foreign audio search actually finds
        // something.
        *burned = TRUE;
    }

    subtitle_set_track_description(settings, subsettings);

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

    int title_id;
    title_id = ghb_dict_get_int(ud->settings, "title");
    const hb_title_t *title = ghb_lookup_title(title_id, NULL);
    if (title != NULL)
    {
        w = GHB_WIDGET(ud->builder, "SubtitleSrtDisable");
        gtk_widget_set_sensitive(w, !!hb_list_count(title->list_subtitle));
    }
}

void
ghb_set_pref_subtitle(signal_user_data_t *ud)
{
    int               sub_count, title_id;
    GtkWidget        *widget;
    const hb_title_t *title;

    title_id = ghb_dict_get_int(ud->settings, "title");
    title    = ghb_lookup_title(title_id, NULL);

    clear_subtitle_list_ui(ud->builder);
    if (title == NULL)
    {
        // Clear the subtitle list
        clear_subtitle_list_settings(ud->settings);
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
    GhbValue *job = ghb_get_job_settings(ud->settings);
    ghb_dict_remove(job, "Subtitle");
    hb_preset_job_add_subtitles(ghb_scan_handle(), title_id, ud->settings, job);
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
    GtkTreeSelection *ts;
    GtkTreeModel *tm;
    GtkTreeIter iter;
    GhbValue *subsettings = NULL;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list_view"));
    ts = gtk_tree_view_get_selection(tv);
    if (gtk_tree_selection_get_selected(ts, &tm, &iter))
    {
        GhbValue *subtitle_list, *subtitle_search;
        gboolean search;
        gint row, *indices;
        GtkTreePath *tp;

        // Get the row number
        tp = gtk_tree_model_get_path(tm, &iter);
        indices = gtk_tree_path_get_indices(tp);
        row = indices[0];
        gtk_tree_path_free(tp);

        subtitle_search = ghb_get_job_subtitle_search(ud->settings);
        search = ghb_dict_get_bool(subtitle_search, "Enable");
        if (search)
        {
            if (row == 0)
            {
                if (index != NULL)
                    *index = -1;
                return subtitle_search;
            }
            row--;
        }

        subtitle_list = ghb_get_job_subtitle_list(ud->settings);
        if (row < 0 || row >= ghb_array_len(subtitle_list))
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
        GhbValue *val, *srt;
        gboolean burn, force, def;
        int source;

        const char *mux_id;
        const hb_container_t *mux;

        mux_id = ghb_dict_get_string(ud->settings, "FileFormat");
        mux    = ghb_lookup_container_by_name(mux_id);

        srt    = ghb_dict_get(subsettings, "SRT");
        source = get_sub_source(ud->settings, subsettings);
        val    = ghb_dict_get(subsettings, "Track");

        if (val != NULL)
        {
            ghb_ui_update(ud, "SubtitleTrack", val);

            // Hide regular subtitle widgets
            widget = GHB_WIDGET(ud->builder, "subtitle_track_box");
            gtk_widget_set_visible(widget, srt == NULL);

            // Show SRT subitle widgets
            widget = GHB_WIDGET(ud->builder, "subtitle_srt_grid");
            gtk_widget_set_visible(widget, srt != NULL);

            widget = GHB_WIDGET(ud->builder, "subtitle_srt_switch_box");
            gtk_widget_set_visible(widget, TRUE);
        }
        else
        {
            // Hide widgets not needed for "Foreign audio search"
            widget = GHB_WIDGET(ud->builder, "subtitle_track_box");
            gtk_widget_set_visible(widget, FALSE);

            widget = GHB_WIDGET(ud->builder, "subtitle_srt_grid");
            gtk_widget_set_visible(widget, FALSE);

            widget = GHB_WIDGET(ud->builder, "subtitle_srt_switch_box");
            gtk_widget_set_visible(widget, FALSE);
        }

        if (srt != NULL)
        {
            ghb_ui_update(ud, "SubtitleSrtEnable", ghb_boolean_value(TRUE));
            val = ghb_dict_get(srt, "Language");
            ghb_ui_update(ud, "SrtLanguage", val);
            val = ghb_dict_get(srt, "Codeset");
            ghb_ui_update(ud, "SrtCodeset", val);
            val = ghb_dict_get(srt, "Filename");
            ghb_ui_update(ud, "SrtFile", val);
            val = ghb_dict_get(subsettings, "Offset");
            ghb_ui_update(ud, "SrtOffset", val);
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

        burn = ghb_dict_get_bool(subsettings, "Burn");
        force = ghb_dict_get_bool(subsettings, "Forced");
        def = ghb_dict_get_bool(subsettings, "Default");

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
        ghb_dict_set_bool(subsettings, "Burn", burn);
        ghb_dict_set_bool(subsettings, "Forced", force);
        ghb_dict_set_bool(subsettings, "Default", def);
        ghb_ui_update(ud, "SubtitleBurned", ghb_boolean_value(burn));
        ghb_ui_update(ud, "SubtitleForced", ghb_boolean_value(force));
        ghb_ui_update(ud, "SubtitleDefaultTrack", ghb_boolean_value(def));

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
subtitle_update_setting(GhbValue *val, const char *name, signal_user_data_t *ud)
{
    GhbValue *subsettings;

    subsettings = subtitle_get_selected_settings(ud, NULL);
    if (subsettings != NULL)
    {
        ghb_dict_set(subsettings, name, val);
        subtitle_set_track_description(ud->settings, subsettings);
        subtitle_list_refresh_selected(ud, subsettings);
        ghb_live_reset(ud);
    }
    return subsettings;
}

G_MODULE_EXPORT void
subtitle_track_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *subsettings;

    ghb_widget_to_setting(ud->settings, widget);

    int track = ghb_dict_get_int(ud->settings, "SubtitleTrack");
    subsettings = subtitle_update_setting(ghb_int_value_new(track),
                                          "Track", ud);
    if (subsettings != NULL)
    {
        subtitle_set_track_description(ud->settings, subsettings);
        subtitle_update_dialog_widgets(ud, subsettings);
    }
}

G_MODULE_EXPORT void
subtitle_forced_toggled_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    GhbValue *val = ghb_widget_value(widget);
    subtitle_update_setting(ghb_value_dup(val), "Forced", ud);
}

G_MODULE_EXPORT void
subtitle_burned_toggled_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *subsettings;
    int index;

    ghb_widget_to_setting(ud->settings, widget);
    GhbValue *val = ghb_widget_value(widget);
    subtitle_update_setting(ghb_value_dup(val), "Burn", ud);
    subsettings = subtitle_get_selected_settings(ud, &index);
    if (subsettings != NULL)
    {
        if (ghb_dict_get_bool(subsettings, "Burn"))
        {
            ghb_ui_update(ud, "SubtitleDefaultTrack", ghb_boolean_value(FALSE));
            subtitle_exclusive_burn(ud, index);
        }
    }
}

G_MODULE_EXPORT void
subtitle_default_toggled_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *subsettings;
    int index;

    ghb_widget_to_setting(ud->settings, widget);
    GhbValue *val = ghb_widget_value(widget);
    subtitle_update_setting(ghb_value_dup(val), "Default", ud);
    subsettings = subtitle_get_selected_settings(ud, &index);
    if (subsettings != NULL)
    {
        if (ghb_dict_get_bool(subsettings, "Default"))
        {
            ghb_ui_update(ud, "SubtitleBurned", ghb_boolean_value(FALSE));
            subtitle_exclusive_default(ud, index);
        }
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
        if (ghb_dict_get_bool(ud->settings, "SubtitleSrtEnable"))
        {
            const gchar *pref_lang, *dir;
            gchar *filename;
            GhbValue *srt = ghb_dict_new();

            ghb_dict_set(subsettings, "SRT", srt);
            pref_lang = ghb_dict_get_string(ud->settings, "PreferredLanguage");
            ghb_dict_set_string(srt, "Language", pref_lang);
            ghb_dict_set_string(srt, "Codeset", "UTF-8");

            dir = ghb_dict_get_string(ud->prefs, "SrtDir");
            filename = g_strdup_printf("%s/none", dir);
            ghb_dict_set_string(srt, "Filename", filename);
            g_free(filename);
        }
        else
        {
            ghb_dict_remove(subsettings, "SRT");
        }
        subtitle_set_track_description(ud->settings, subsettings);
        subtitle_update_dialog_widgets(ud, subsettings);
        subtitle_list_refresh_selected(ud, subsettings);
        ghb_live_reset(ud);
    }
}

static void
subtitle_list_refresh_selected(signal_user_data_t *ud, GhbValue *subsettings)
{
    GtkTreeView *tv;
    GtkTreeSelection *ts;
    GtkTreeModel *tm;
    GtkTreeIter ti;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list_view"));
    ts = gtk_tree_view_get_selection(tv);
    if (gtk_tree_selection_get_selected(ts, &tm, &ti))
    {
        subtitle_refresh_list_row_ui(tm, &ti, ud->settings, subsettings);
    }
}

void
ghb_subtitle_list_refresh_all(signal_user_data_t *ud)
{
    subtitle_refresh_list_ui(ud);
}

G_MODULE_EXPORT void
srt_setting_update(GhbValue *val, const char *name, signal_user_data_t *ud)
{
    GhbValue *subsettings;
    subsettings = subtitle_get_selected_settings(ud, NULL);
    if (subsettings != NULL)
    {
        GhbValue *srt;
        srt = ghb_dict_get(subsettings, "SRT");
        if (srt != NULL)
        {
            ghb_dict_set(srt, name, val);
            subtitle_set_track_description(ud->settings, subsettings);
            subtitle_list_refresh_selected(ud, subsettings);
            ghb_live_reset(ud);
        }
    }
}

G_MODULE_EXPORT void
srt_offset_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);

    int64_t offset = ghb_dict_get_int(ud->settings, "SrtOffset");
    srt_setting_update(ghb_int_value_new(offset), "Offset", ud);
}

G_MODULE_EXPORT void
srt_codeset_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);

    GhbValue *val = ghb_dict_get(ud->settings, "SrtCodeset");
    srt_setting_update(ghb_value_dup(val), "Codeset", ud);
}

G_MODULE_EXPORT void
srt_file_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);

    GhbValue *val = ghb_dict_get(ud->settings, "SrtFile");
    srt_setting_update(ghb_value_dup(val), "Filename", ud);

    // Update SrtDir preference
    const gchar *filename;
    gchar *dirname;

    filename = ghb_value_get_string(val);
    if (g_file_test(filename, G_FILE_TEST_IS_DIR))
    {
        ghb_dict_set_string(ud->prefs, "SrtDir", filename);
    }
    else
    {
        dirname = g_path_get_dirname(filename);
        ghb_dict_set_string(ud->prefs, "SrtDir", dirname);
        g_free(dirname);
    }
    ghb_pref_save(ud->prefs, "SrtDir");
}

G_MODULE_EXPORT void
srt_lang_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);

    GhbValue *val = ghb_dict_get(ud->settings, "SrtLanguage");
    srt_setting_update(ghb_value_dup(val), "Language", ud);
}

static void
clear_subtitle_list_settings(GhbValue *settings)
{
    GhbValue *subtitle_list, *subtitle_search;

    subtitle_list = ghb_get_job_subtitle_list(settings);
    subtitle_search = ghb_get_job_subtitle_search(settings);
    ghb_array_reset(subtitle_list);
    ghb_dict_set_bool(subtitle_search, "Enable", 0);
}

void
ghb_clear_subtitle_selection(GtkBuilder *builder)
{
    GtkTreeView *tv;
    GtkTreeSelection *tsel;

    tv = GTK_TREE_VIEW(GHB_WIDGET(builder, "subtitle_list_view"));
    // Clear tree selection so that updates are not triggered
    // that cause a recursive attempt to clear the tree selection (crasher)
    tsel = gtk_tree_view_get_selection(tv);
    gtk_tree_selection_unselect_all(tsel);
}

static void
clear_subtitle_list_ui(GtkBuilder *builder)
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
    subtitle_refresh_list_row_ui(tm, &ti, ud->settings, subsettings);

    gtk_tree_selection_select_iter(ts, &ti);
}

G_MODULE_EXPORT void
subtitle_list_selection_changed_cb(GtkTreeSelection *ts, signal_user_data_t *ud)
{
    GtkTreeModel *tm;
    GtkTreeIter iter;
    GhbValue *subsettings;

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
    }
    subsettings = subtitle_get_selected_settings(ud, NULL);
    subtitle_update_dialog_widgets(ud, subsettings);
}

static gboolean subtitle_is_one_burned(GhbValue *settings)
{
    GhbValue *subtitle_list, *subsettings;
    int count, ii;

    subtitle_list = ghb_get_job_subtitle_list(settings);
    if (subtitle_list == NULL)
        return FALSE;

    count = ghb_array_len(subtitle_list);
    for (ii = 0; ii < count; ii++)
    {
        subsettings = ghb_array_get(subtitle_list, ii);
        if (ghb_dict_get_bool(subsettings, "Burn"))
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

    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL)
    {
        return;
    }

    // Back up settings in case we need to revert.
    backup = ghb_value_dup(ghb_get_job_subtitle_settings(ud->settings));
    one_burned = subtitle_is_one_burned(ud->settings);

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(ud->settings, "FileFormat");
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
    if (subsettings != NULL)
    {
        add_subtitle_to_ui(ud, subsettings);

        // Pop up the edit dialog
        GtkResponseType response;
        GtkWidget *dialog = GHB_WIDGET(ud->builder, "subtitle_dialog");
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_hide(dialog);
        if (response != GTK_RESPONSE_OK)
        {
            ghb_clear_subtitle_selection(ud->builder);
            ghb_dict_set(ud->settings, "Subtitle", backup);
            subtitle_refresh_list_ui(ud);
        }
        else
        {
            ghb_value_free(&backup);
        }
    }
}

G_MODULE_EXPORT void
subtitle_add_fas_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    GhbValue *subtitle_search, *backup;

    subtitle_search = ghb_get_job_subtitle_search(ud->settings);
    if (ghb_dict_get_bool(subtitle_search, "Enable"))
    {
        // Foreign audio search is already enabled
        return;
    }

    backup = ghb_value_dup(ghb_get_job_subtitle_settings(ud->settings));

    ghb_dict_set_bool(subtitle_search, "Enable", 1);
    ghb_dict_set_bool(subtitle_search, "Forced", 1);
    ghb_dict_set_bool(subtitle_search, "Default", 1);
    ghb_dict_set_bool(subtitle_search, "Burn", 0);

    subtitle_set_track_description(ud->settings, subtitle_search);

    GtkTreeView *tv;
    GtkTreeIter ti;
    GtkTreeModel *tm;
    GtkTreeSelection *ts;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list_view"));
    ts = gtk_tree_view_get_selection(tv);
    tm = gtk_tree_view_get_model(tv);

    gtk_tree_store_prepend(GTK_TREE_STORE(tm), &ti, NULL);
    subtitle_refresh_list_row_ui(tm, &ti, ud->settings, subtitle_search);

    gtk_tree_selection_select_iter(ts, &ti);
    ghb_live_reset(ud);

    GtkResponseType response;
    GtkWidget *dialog = GHB_WIDGET(ud->builder, "subtitle_dialog");
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    if (response != GTK_RESPONSE_OK)
    {
        ghb_clear_subtitle_selection(ud->builder);
        ghb_dict_set(ud->settings, "Subtitle", backup);
        subtitle_refresh_list_ui(ud);
    }
    else
    {
        // Disabel FAS button
        GtkWidget *w = GHB_WIDGET(ud->builder, "subtitle_add_fas");
        gtk_widget_set_sensitive(w, 0);

        ghb_value_free(&backup);
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
    title_id = ghb_dict_get_int(ud->settings, "title");
    title = ghb_lookup_title(title_id, &titleindex);
    if (title == NULL)
    {
        return;
    }

    clear_subtitle_list_settings(ud->settings);
    clear_subtitle_list_ui(ud->builder);

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(ud->settings, "FileFormat");
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
    ghb_set_pref_subtitle(ud);
}

// When the container changes, it may be necessary to burn additional
// subtitles.  Since only one can be burned, some subtitles may be
// removed from the list.
void
ghb_subtitle_prune(signal_user_data_t *ud)
{
    GhbValue *subtitle_list;
    GhbValue *subsettings;
    gint ii;
    gboolean one_burned = FALSE;

    subtitle_list = ghb_get_job_subtitle_list(ud->settings);
    if (subtitle_list == NULL)
        return;

    const char *mux_id;
    const hb_container_t *mux;

    mux_id = ghb_dict_get_string(ud->settings, "FileFormat");
    mux = ghb_lookup_container_by_name(mux_id);

    for (ii = 0; ii < ghb_array_len(subtitle_list); )
    {
        gboolean burned;
        int source;

        subsettings = ghb_array_get(subtitle_list, ii);
        burned = ghb_dict_get_bool(subsettings, "Burn");
        source = get_sub_source(ud->settings, subsettings);
        burned = burned || !hb_subtitle_can_pass(source, mux->format);
        if (burned && one_burned)
        {
            ghb_array_remove(subtitle_list, ii);
            continue;
        }
        one_burned = one_burned || burned;
        ghb_dict_set_bool(subsettings, "Burn", burned);
        ii++;
    }
    subsettings = subtitle_get_selected_settings(ud, NULL);
    if (subsettings != NULL)
    {
        subtitle_update_dialog_widgets(ud, subsettings);
    }
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

    ghb_dict_set_string(ud->settings, "PreferredLanguage", code);

    // If there is no preferred language, disable options that require
    // a preferred language to be set.
    gboolean sensitive = !(lang == NULL || !strncmp(code, "und", 4));
    button = GTK_BUTTON(GHB_WIDGET(ud->builder,
                                  "SubtitleAddForeignAudioSubtitle"));
    if (sensitive)
    {
        str = g_strdup_printf(
            _("Add %s subtitle track if default audio is not %s"), name, name);
    }
    else
    {
        str = g_strdup_printf(
            _("Add subtitle track if default audio is not your preferred language"));
    }
    gtk_button_set_label(button, str);
    g_free(str);

    gtk_widget_set_sensitive(GTK_WIDGET(button), sensitive);
    button = GTK_BUTTON(GHB_WIDGET(ud->builder,
                                  "SubtitleAddForeignAudioSearch"));
    gtk_widget_set_sensitive(GTK_WIDGET(button), sensitive);
}

void
ghb_subtitle_set_pref_lang(GhbValue *settings)
{
    GhbValue *lang_list;
    gboolean set = FALSE;
    lang_list = ghb_dict_get_value(settings, "SubtitleLanguageList");
    if (ghb_array_len(lang_list) > 0)
    {
        GhbValue *glang = ghb_array_get(lang_list, 0);
        if (glang != NULL)
        {
            ghb_dict_set_string(settings, "PreferredLanguage",
                                    ghb_value_get_string(glang));
            set = TRUE;
        }
    }
    if (!set)
    {
        ghb_dict_set_string(settings, "PreferredLanguage", "und");
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
        lang_list = ghb_dict_get_value(ud->settings, "SubtitleLanguageList");
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
        lang_list = ghb_dict_get_value(ud->settings, "SubtitleLanguageList");
        ghb_array_remove(lang_list, index);

        ghb_clear_presets_selection(ud);

        if (ghb_array_len(lang_list) > 0)
        {
            const iso639_lang_t *lang;
            GhbValue *entry = ghb_array_get(lang_list, 0);
            lang = ghb_iso639_lookup_by_int(ghb_lookup_lang(entry));
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

    lang_list = ghb_dict_get_value(ud->settings, "SubtitleLanguageList");
    if (lang_list == NULL)
    {
        lang_list = ghb_array_new();
        ghb_dict_set(ud->settings, "SubtitleLanguageList", lang_list);
    }

    int ii, count;
    count = ghb_array_len(lang_list);
    for (ii = 0; ii < count; )
    {
        GhbValue *lang_val = ghb_array_get(lang_list, ii);
        int idx = ghb_lookup_lang(lang_val);
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
        backup = ghb_value_dup(ghb_get_job_subtitle_settings(ud->settings));

        // Pop up the edit dialog
        GtkResponseType response;
        GtkWidget *dialog = GHB_WIDGET(ud->builder, "subtitle_dialog");
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_hide(dialog);
        if (response != GTK_RESPONSE_OK)
        {
            ghb_dict_set(ud->settings, "Subtitle", backup);
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
    GhbValue *subtitle_list, *subtitle_search;

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

        subtitle_search = ghb_get_job_subtitle_search(ud->settings);
        subtitle_list = ghb_get_job_subtitle_list(ud->settings);

        // Get the row number
        indices = gtk_tree_path_get_indices(tp);
        row = indices[0];
        if (ghb_dict_get_bool(subtitle_search, "Enable"))
        {
            if (row == 0)
            {
                ghb_dict_set_bool(subtitle_search, "Enable", 0);
                gtk_tree_store_remove(GTK_TREE_STORE(tm), &ti);

                // Enable FAS button
                GtkWidget *w = GHB_WIDGET(ud->builder, "subtitle_add_fas");
                gtk_widget_set_sensitive(w, 1);
                return;
            }
            row--;
        }
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

