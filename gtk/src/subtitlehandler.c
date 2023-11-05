/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * subtitlehandler.c
 * Copyright (C) John Stebbins 2008-2023 <stebbins@stebbins>
 *
 * subtitlehandler.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2, as published by the Free Software
 * Foundation.
 *
 * subtitlehandler.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with main.c.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#include "ghbcompat.h"
#include "handbrake/handbrake.h"
#include "settings.h"
#include "jobdict.h"
#include "titledict.h"
#include "hb-backend.h"
#include "values.h"
#include "callbacks.h"
#include "preview.h"
#include "presets.h"
#include "audiohandler.h"
#include "subtitlehandler.h"

#include <glib/gi18n.h>

static char * subtitle_get_track_description(GhbValue *settings,
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
    GhbValue *import;

    import  = ghb_dict_get(subsettings, "Import");
    if (import != NULL)
    {
        const char * format = ghb_dict_get_string(import, "Format");
        if (format != NULL && !strcasecmp(format, "SSA"))
        {
            return IMPORTSSA;
        }
        return IMPORTSRT;
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
    char *desc;


    info_src_2 = NULL;
    info_dst_2 = NULL;

    forced = ghb_dict_get_bool(subsettings, "Forced");
    burned = ghb_dict_get_bool(subsettings, "Burn");
    def = ghb_dict_get_bool(subsettings, "Default");
    desc = subtitle_get_track_description(settings, subsettings);
    info_src = g_strdup_printf("<small>%s</small>", desc);
    g_free(desc);
    if (ghb_dict_get(subsettings, "Import") != NULL)
    {
        gint offset;
        offset = ghb_dict_get_int(subsettings, "Offset");
        if (offset != 0)
        {
            info_dst_2 = g_strdup_printf(_("Offset: %dms"), offset);
        }
    }

    GString *str = g_string_new("<small>");
    g_string_append(str, burned ? _("Burned Into Video") :
                                  _("Passthrough"));
    if (forced)
    {
        g_string_append_printf(str, " (%s)", _("Forced Subtitles Only"));
    }
    if (def)
    {
        g_string_append_printf(str, " (%s)", _("Default"));
    }
    g_string_append_printf(str, "</small>");

    info_dst = g_string_free(str, FALSE);

    gtk_tree_store_set(GTK_TREE_STORE(tm), ti,
        // These are displayed in list
        0, info_src,
        1, "-->",
        2, info_dst,
        3, "document-edit-symbolic",
        4, "edit-delete-symbolic",
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

    // Check consistency of exclusive flags
    count = ghb_array_len(subtitle_list);
    if (burn)
        subtitle_exclusive_burn_settings(settings, count-1);
    if (def)
        subtitle_exclusive_default_settings(settings, count-1);
}

static char *
subtitle_get_track_description(GhbValue *settings, GhbValue *subsettings)
{
    GhbValue * import;
    char *desc = NULL;

    import = ghb_dict_get(subsettings, "Import");
    if (import != NULL)
    {
        const gchar * format = "SRT";
        const gchar * filename, * code;
        const gchar * lang;
        int           source = IMPORTSRT;
        const iso639_lang_t * iso;

        format   = ghb_dict_get_string(import, "Format");
        filename = ghb_dict_get_string(import, "Filename");
        lang     = ghb_dict_get_string(import, "Language");
        code     = ghb_dict_get_string(import, "Codeset");

        if (format != NULL && !strcasecmp(format, "SSA"))
        {
            source = IMPORTSSA;
        }
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
            if (source == IMPORTSRT)
            {
                desc = g_strdup_printf("%s (%s)(%s)(%s)",
                                       lang, code, format, basename);
            }
            else
            {
                desc = g_strdup_printf("%s (%s)(%s)", lang, format, basename);
            }
            g_free(basename);
        }
        else
        {
            if (source == IMPORTSRT)
            {
                desc = g_strdup_printf("%s (%s)(%s)", lang, code, format);
            }
            else
            {
                desc = g_strdup_printf("%s (%s)", lang, format);
            }
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
            desc = g_strdup(_("Foreign Audio Scan"));
        }
        else
        {
            subtitle = ghb_get_subtitle_info(title, track);
            if (subtitle != NULL)
            {
                desc = g_strdup_printf("%d - %s", track + 1, subtitle->lang);
            }
        }
    }

    return desc;
}

static GhbValue*  subtitle_add_track(
    signal_user_data_t *ud,
    GhbValue *settings,
    int track,
    int mux,
    gboolean default_track,
    gboolean import,
    int source,
    gboolean burn,
    gboolean *burned)
{
    const char * name = NULL;
    if (track >= 0 && !import)
    {
        GhbValue * strack;

        strack = ghb_get_title_subtitle_track(settings, track);
        source = ghb_dict_get_int(strack, "Source");
        name   = ghb_dict_get_string(strack, "Name");
    }

    burn |= !hb_subtitle_can_pass(source, mux);

    if (*burned && burn)
    {
        // Can only burn one.  Skip others that must be burned.
        return NULL;
    }

    GhbValue *subsettings = ghb_dict_new();
    if (import)
    {
        // Set default SRT settings
        GhbValue *import_dict;
        const gchar *pref_lang, *dir;
        gchar *filename;

        import_dict = ghb_dict_new();
        hb_dict_set(subsettings, "Import", import_dict);

        ghb_dict_set_string(import_dict, "Format",
                            source == IMPORTSRT ? "SRT" : "SSA");
        pref_lang = ghb_dict_get_string(settings, "PreferredLanguage");
        ghb_dict_set_string(import_dict, "Language", pref_lang);
        ghb_dict_set_string(import_dict, "Codeset", "UTF-8");

        dir = ghb_dict_get_string(ud->prefs, "SrtDir");
        filename = g_strdup_printf("%s/none", dir);
        ghb_dict_set_string(import_dict, "Filename", filename);
        g_free(filename);
    }

    if (name != NULL && name[0] != 0)
    {
        ghb_dict_set_string(subsettings, "Name", name);
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
        w = GHB_WIDGET(ud->builder, "SubtitleImportDisable");
        gtk_widget_set_sensitive(w, !!hb_list_count(title->list_subtitle));
    }
}

static void
set_pref_subtitle(signal_user_data_t *ud)
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
        widget = GHB_WIDGET(ud->builder, "SubtitleSrtEnable");
        gtk_widget_set_sensitive(widget, TRUE);
    }
    else
    {
        widget = GHB_WIDGET(ud->builder, "SubtitleImportDisable");
        gtk_widget_set_sensitive(widget, TRUE);
    }
    GhbValue *job = ghb_get_job_settings(ud->settings);
    ghb_dict_remove(job, "Subtitle");
    hb_preset_job_add_subtitles(ghb_scan_handle(), title_id, ud->settings, job);
    subtitle_refresh_list_ui(ud);
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
        GhbValue *val, *import;
        gboolean burn, force, def;
        int source;

        const char *mux_id;
        const hb_container_t *mux;

        mux_id = ghb_dict_get_string(ud->settings, "FileFormat");
        mux    = ghb_lookup_container_by_name(mux_id);

        import = ghb_dict_get(subsettings, "Import");
        source = get_sub_source(ud->settings, subsettings);

        val = ghb_dict_get_value(subsettings, "Name");
        if (val != NULL)
        {
            ghb_ui_update(ud, "SubtitleTrackName", val);
        }
        else
        {
            ghb_ui_update(ud, "SubtitleTrackName", ghb_string_value(""));
        }

        val    = ghb_dict_get(subsettings, "Track");
        if (val != NULL)
        {
            ghb_ui_update(ud, "SubtitleTrack", val);

            // Hide regular subtitle widgets
            widget = GHB_WIDGET(ud->builder, "subtitle_track_label");
            gtk_widget_set_visible(widget, import == NULL);
            widget = GHB_WIDGET(ud->builder, "SubtitleTrack");
            gtk_widget_set_visible(widget, import == NULL);

            // Show import subtitle widgets
            widget = GHB_WIDGET(ud->builder, "subtitle_import_grid");
            gtk_widget_set_visible(widget, source == IMPORTSRT ||
                                           source == IMPORTSSA);
            widget = GHB_WIDGET(ud->builder, "srt_code_label");
            gtk_widget_set_visible(widget, source == IMPORTSRT);
            widget = GHB_WIDGET(ud->builder, "SrtCodeset");
            gtk_widget_set_visible(widget, source == IMPORTSRT);

            widget = GHB_WIDGET(ud->builder, "subtitle_import_switch_box");
            gtk_widget_set_visible(widget, TRUE);
        }
        else
        {
            // Hide widgets not needed for "Foreign audio search"
            widget = GHB_WIDGET(ud->builder, "subtitle_track_label");
            gtk_widget_set_visible(widget, FALSE);
            widget = GHB_WIDGET(ud->builder, "SubtitleTrack");
            gtk_widget_set_visible(widget, FALSE);

            widget = GHB_WIDGET(ud->builder, "subtitle_import_grid");
            gtk_widget_set_visible(widget, FALSE);

            widget = GHB_WIDGET(ud->builder, "subtitle_import_switch_box");
            gtk_widget_set_visible(widget, FALSE);
        }

        if (import != NULL)
        {
            if (source == IMPORTSSA)
            {
                ghb_ui_update(ud, "SubtitleSsaEnable", ghb_boolean_value(TRUE));
            }
            else
            {
                ghb_ui_update(ud, "SubtitleSrtEnable", ghb_boolean_value(TRUE));
            }
            val = ghb_dict_get(import, "Language");
            ghb_ui_update(ud, "ImportLanguage", val);
            val = ghb_dict_get(import, "Codeset");
            ghb_ui_update(ud, "SrtCodeset", val);
            val = ghb_dict_get(import, "Filename");
            ghb_ui_update(ud, "ImportFile", val);
            val = ghb_dict_get(subsettings, "Offset");
            ghb_ui_update(ud, "ImportOffset", val);
        }
        else
        {
            ghb_ui_update(ud, "SubtitleImportDisable", ghb_boolean_value(TRUE));
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
        // Hide SRT subtitle widgets
        widget = GHB_WIDGET(ud->builder, "subtitle_import_grid");
        gtk_widget_set_visible(widget, FALSE);

        // Show regular subtitle widgets
        widget = GHB_WIDGET(ud->builder, "subtitle_track_label");
        gtk_widget_set_visible(widget, TRUE);
        widget = GHB_WIDGET(ud->builder, "SubtitleTrack");
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
        if (val != NULL)
            ghb_dict_set(subsettings, name, val);
        else
            ghb_dict_remove(subsettings, name);
        subtitle_list_refresh_selected(ud, subsettings);
        ghb_update_summary_info(ud);
        ghb_live_reset(ud);
    }
    else
    {
        ghb_value_free(&val);
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
        subtitle_update_dialog_widgets(ud, subsettings);
    }
}

G_MODULE_EXPORT void
subtitle_name_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    const char *s = ghb_dict_get_string(ud->settings, "SubtitleTrackName");
    if (s != NULL && s[0] != 0)
    {
        subtitle_update_setting(ghb_widget_value(widget), "Name", ud);
    }
    else
    {
        subtitle_update_setting(NULL, "Name", ud);
    }
}

G_MODULE_EXPORT void
subtitle_forced_toggled_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    GhbValue *val = ghb_widget_value(widget);
    subtitle_update_setting(val, "Forced", ud);
}

G_MODULE_EXPORT void
subtitle_burned_toggled_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *subsettings;
    int index;

    ghb_widget_to_setting(ud->settings, widget);
    GhbValue *val = ghb_widget_value(widget);
    subtitle_update_setting(val, "Burn", ud);
    subsettings = subtitle_get_selected_settings(ud, &index);
    if (subsettings != NULL)
    {
        if (ghb_dict_get_bool(subsettings, "Burn") &&
            ghb_dict_get(subsettings, "Track") != NULL) // !foreign audio search
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
    subtitle_update_setting(val, "Default", ud);
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
subtitle_import_radio_toggled_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GhbValue *subsettings;

    ghb_widget_to_setting(ud->settings, widget);
    if (!ghb_dict_get_bool(ud->settings, "SubtitleSrtEnable") &&
        !ghb_dict_get_bool(ud->settings, "SubtitleSsaEnable") &&
        !ghb_dict_get_bool(ud->settings, "SubtitleImportDisable"))
    {
        // Radio buttons are in an in-between state with none enabled.
        // Wait for the next toggle when something gets enabled.
        return;
    }
    subsettings = subtitle_get_selected_settings(ud, NULL);
    if (subsettings != NULL)
    {
        if (ghb_dict_get_bool(ud->settings, "SubtitleSrtEnable") ||
            ghb_dict_get_bool(ud->settings, "SubtitleSsaEnable"))
        {
            const gchar *pref_lang, *dir;
            gchar *filename;
            GhbValue *import = ghb_dict_get(subsettings, "Import");

            if (import == NULL)
            {
                import = ghb_dict_new();
                ghb_dict_set(subsettings, "Import", import);
                pref_lang = ghb_dict_get_string(ud->settings, "PreferredLanguage");
                ghb_dict_set_string(import, "Language", pref_lang);
                ghb_dict_set_string(import, "Codeset", "UTF-8");

                dir = ghb_dict_get_string(ud->prefs, "SrtDir");
                filename = g_strdup_printf("%s/none", dir);
                ghb_dict_set_string(import, "Filename", filename);
                g_free(filename);
            }
            ghb_dict_set_string(import, "Format",
                        hb_dict_get_bool(ud->settings, "SubtitleSrtEnable") ?
                            "SRT" : "SSA");
        }
        else
        {
            ghb_dict_remove(subsettings, "Import");
        }
        subtitle_update_dialog_widgets(ud, subsettings);
        subtitle_list_refresh_selected(ud, subsettings);
        ghb_update_summary_info(ud);
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
import_setting_update(GhbValue *val, const char *name, signal_user_data_t *ud)
{
    GhbValue *subsettings;
    subsettings = subtitle_get_selected_settings(ud, NULL);
    if (subsettings != NULL)
    {
        GhbValue * import;
        import = ghb_dict_get(subsettings, "Import");
        if (import != NULL)
        {
            ghb_dict_set(import, name, val);
            subtitle_list_refresh_selected(ud, subsettings);
            ghb_update_summary_info(ud);
            ghb_live_reset(ud);
        }
        else
        {
            ghb_value_free(&val);
        }
    }
    else
    {
        ghb_value_free(&val);
    }
}

G_MODULE_EXPORT void
import_offset_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    GhbValue *val = ghb_widget_value(widget);
    subtitle_update_setting(ghb_value_xform(val, GHB_INT), "Offset", ud);
}

G_MODULE_EXPORT void
srt_codeset_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    GhbValue *val = ghb_dict_get(ud->settings, "SrtCodeset");
    import_setting_update(ghb_value_dup(val), "Codeset", ud);
}

G_MODULE_EXPORT void
import_file_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    GhbValue *val = ghb_dict_get(ud->settings, "ImportFile");
    import_setting_update(ghb_value_dup(val), "Filename", ud);

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
import_lang_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    GhbValue *val = ghb_dict_get(ud->settings, "ImportLanguage");
    import_setting_update(ghb_value_dup(val), "Language", ud);
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
        subsettings = subtitle_add_track(ud, ud->settings, track,
                                mux->format, FALSE, FALSE, VOBSUB,
                                FALSE, &one_burned);
    }
    if (subsettings == NULL)
    {
        subsettings = subtitle_add_track(ud, ud->settings, 0,
                                mux->format, FALSE, TRUE, IMPORTSRT,
                                FALSE, &one_burned);
    }
    if (subsettings != NULL)
    {
        add_subtitle_to_ui(ud, subsettings);

        // Pop up the edit dialog
        GtkResponseType response;
        GtkWidget *dialog = GHB_WIDGET(ud->builder, "subtitle_dialog");
        gtk_window_set_title(GTK_WINDOW(dialog), _("Add Subtitles"));
        response = ghb_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_hide(dialog);
        if (response != GTK_RESPONSE_OK)
        {
            ghb_clear_subtitle_selection(ud->builder);
            ghb_dict_set(ghb_get_job_settings(ud->settings),
                         "Subtitle", backup);
            subtitle_refresh_list_ui(ud);
        }
        else
        {
            ghb_value_free(&backup);
        }
        ghb_update_summary_info(ud);
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
    gtk_window_set_title(GTK_WINDOW(dialog), _("Foreign Audio Scan"));
    response = ghb_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    if (response != GTK_RESPONSE_OK)
    {
        ghb_clear_subtitle_selection(ud->builder);
        ghb_dict_set(ghb_get_job_settings(ud->settings),
                     "Subtitle", backup);
        subtitle_refresh_list_ui(ud);
    }
    else
    {
        // Disable FAS button
        GtkWidget *w = GHB_WIDGET(ud->builder, "subtitle_add_fas");
        gtk_widget_set_sensitive(w, 0);

        ghb_value_free(&backup);
    }
    ghb_update_summary_info(ud);
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
        subtitle_add_track(ud, ud->settings, track, mux->format,
                           FALSE, FALSE, VOBSUB, FALSE, &one_burned);
    }
    subtitle_refresh_list_ui(ud);
    ghb_update_summary_info(ud);
    ghb_live_reset(ud);
}

G_MODULE_EXPORT void
subtitle_reset_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    set_pref_subtitle(ud);
    ghb_update_summary_info(ud);
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

static void
subtitle_add_lang_iter(GtkTreeModel *tm, GtkTreeIter *iter,
                       signal_user_data_t *ud)
{
    GtkTreeView         * selected;
    GtkTreeStore        * selected_ts;
    GtkTreeIter           pos;
    GtkTreePath         * tp;
    char                * lang;
    int                   index;
    const iso639_lang_t * iso_lang;
    GhbValue            * glang, * slang_list;
    GtkTreeSelection    * tsel;

    selected    = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_selected_lang"));
    selected_ts = GTK_TREE_STORE(gtk_tree_view_get_model(selected));
    tsel        = gtk_tree_view_get_selection(selected);

    // Add to UI selected language list box
    gtk_tree_model_get(tm, iter, 0, &lang, 1, &index, -1);
    gtk_tree_store_append(selected_ts, &pos, NULL);
    gtk_tree_store_set(selected_ts, &pos, 0, lang, 1, index, -1);
    g_free(lang);

    // Select the item added to the selected list and make it
    // visible in the scrolled window
    tp = gtk_tree_model_get_path(GTK_TREE_MODEL(selected_ts), &pos);
    gtk_tree_selection_select_iter(tsel, &pos);
    gtk_tree_view_scroll_to_cell(selected, tp, NULL, FALSE, 0, 0);
    gtk_tree_path_free(tp);

    // Remove from UI available language list box
    gtk_tree_store_remove(GTK_TREE_STORE(tm), iter);

    // Add to preset language list
    iso_lang = ghb_iso639_lookup_by_int(index);
    glang = ghb_string_value_new(iso_lang->iso639_2);
    slang_list = ghb_dict_get_value(ud->settings, "SubtitleLanguageList");
    if (ghb_array_len(slang_list) == 0)
    {
        subtitle_update_pref_lang(ud, iso_lang);
    }
    ghb_array_append(slang_list, glang);
    ghb_clear_presets_selection(ud);
}

G_MODULE_EXPORT void
subtitle_avail_lang_activated_cb(GtkTreeView *tv, GtkTreePath *tp,
                                 GtkTreeViewColumn *column,
                                 signal_user_data_t *ud)
{
    GtkTreeIter    iter;
    GtkTreeModel * tm = gtk_tree_view_get_model(tv);

    if (gtk_tree_model_get_iter(tm, &iter, tp))
    {
        subtitle_add_lang_iter(tm, &iter, ud);
    }
}

G_MODULE_EXPORT void
subtitle_add_lang_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GtkTreeView      * avail;
    GtkTreeModel     * avail_tm;
    GtkTreeSelection * tsel;
    GtkTreeIter        iter;

    avail       = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_avail_lang"));
    avail_tm    = gtk_tree_view_get_model(avail);
    tsel        = gtk_tree_view_get_selection(avail);
    if (gtk_tree_selection_get_selected(tsel, NULL, &iter))
    {
        subtitle_add_lang_iter(avail_tm, &iter, ud);
    }
}

void
subtitle_remove_lang_iter(GtkTreeModel *tm, GtkTreeIter *iter,
                          signal_user_data_t *ud)
{
    GtkTreeView      * avail;
    GtkTreeStore     * avail_ts;
    GtkTreeIter        pos, sibling;
    char             * lang;
    int                index;
    GtkTreePath      * tp  = gtk_tree_model_get_path(tm, iter);
    int              * ind = gtk_tree_path_get_indices(tp);
    int                row = ind[0];
    GhbValue         * slang_list;
    GtkTreeSelection * tsel;

    gtk_tree_path_free(tp);
    avail    = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_avail_lang"));
    avail_ts = GTK_TREE_STORE(gtk_tree_view_get_model(avail));
    tsel     = gtk_tree_view_get_selection(avail);

    // Add to UI available language list box
    gtk_tree_model_get(tm, iter, 0, &lang, 1, &index, -1);
    if (ghb_find_lang_row(GTK_TREE_MODEL(avail_ts), &sibling, index))
    {
        gtk_tree_store_insert_before(avail_ts, &pos, NULL, &sibling);
    }
    else
    {
        gtk_tree_store_append(avail_ts, &pos, NULL);
    }
    gtk_tree_store_set(avail_ts, &pos, 0, lang, 1, index, -1);
    g_free(lang);

    // Select the item added to the available list and make it
    // visible in the scrolled window
    tp = gtk_tree_model_get_path(GTK_TREE_MODEL(avail_ts), &pos);
    gtk_tree_selection_select_iter(tsel, &pos);
    gtk_tree_view_scroll_to_cell(avail, tp, NULL, FALSE, 0, 0);
    gtk_tree_path_free(tp);

    // Remove from UI selected language list box
    gtk_tree_store_remove(GTK_TREE_STORE(tm), iter);

    // Remove from preset language list
    slang_list = ghb_dict_get_value(ud->settings, "SubtitleLanguageList");
    ghb_array_remove(slang_list, row);
    ghb_clear_presets_selection(ud);

    if (row == 0)
    {
        if (ghb_array_len(slang_list) > 0)
        {
            const iso639_lang_t *lang_id;
            GhbValue *entry = ghb_array_get(slang_list, 0);
            lang_id = ghb_iso639_lookup_by_int(ghb_lookup_lang(entry));
            subtitle_update_pref_lang(ud, lang_id);
        }
        else
        {
            subtitle_update_pref_lang(ud, NULL);
        }
    }
}

G_MODULE_EXPORT void
subtitle_selected_lang_activated_cb(GtkTreeView *tv, GtkTreePath *tp,
                                    GtkTreeViewColumn *column,
                                    signal_user_data_t *ud)
{
    GtkTreeIter    iter;
    GtkTreeModel * tm = gtk_tree_view_get_model(tv);

    if (gtk_tree_model_get_iter(tm, &iter, tp))
    {
        subtitle_remove_lang_iter(tm, &iter, ud);
    }
}

G_MODULE_EXPORT void
subtitle_remove_lang_clicked_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    GtkTreeView      * selected;
    GtkTreeModel     * selected_tm;
    GtkTreeSelection * tsel;
    GtkTreeIter        iter;

    selected    = GTK_TREE_VIEW(GHB_WIDGET(ud->builder,
                                           "subtitle_selected_lang"));
    selected_tm = gtk_tree_view_get_model(selected);
    tsel        = gtk_tree_view_get_selection(selected);
    if (gtk_tree_selection_get_selected(tsel, NULL, &iter))
    {
        subtitle_remove_lang_iter(selected_tm, &iter, ud);
    }
}

static void subtitle_def_selected_lang_list_clear(signal_user_data_t *ud)
{
    GtkTreeView  * tv;
    GtkTreeModel * selected_tm;
    GtkTreeStore * avail_ts;
    GtkTreeIter    iter;

    tv          = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_avail_lang"));
    avail_ts    = GTK_TREE_STORE(gtk_tree_view_get_model(tv));
    tv          = GTK_TREE_VIEW(GHB_WIDGET(ud->builder,
                                           "subtitle_selected_lang"));
    selected_tm = gtk_tree_view_get_model(tv);
    if (gtk_tree_model_get_iter_first(selected_tm, &iter))
    {
        do
        {
            char        * lang;
            gint          index;
            GtkTreeIter   pos, sibling;

            gtk_tree_model_get(selected_tm, &iter, 0, &lang, 1, &index, -1);
            if (ghb_find_lang_row(GTK_TREE_MODEL(avail_ts), &sibling, index))
            {
                gtk_tree_store_insert_before(avail_ts, &pos, NULL, &sibling);
            }
            else
            {
                gtk_tree_store_append(avail_ts, &pos, NULL);
            }
            gtk_tree_store_set(avail_ts, &pos, 0, lang, 1, index, -1);
            g_free(lang);
        } while (gtk_tree_model_iter_next(selected_tm, &iter));
    }
    gtk_tree_store_clear(GTK_TREE_STORE(selected_tm));
}

static void subtitle_def_lang_list_init(signal_user_data_t *ud)
{
    GhbValue     * lang_list;
    GtkTreeView  * tv;
    GtkTreeModel * avail;
    GtkTreeStore * selected;
    int            ii, count;

    tv       = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_avail_lang"));
    avail    = gtk_tree_view_get_model(tv);
    tv       = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_selected_lang"));
    selected = GTK_TREE_STORE(gtk_tree_view_get_model(tv));

    // Clear selected languages.
    subtitle_def_selected_lang_list_clear(ud);

    lang_list = ghb_dict_get_value(ud->settings, "SubtitleLanguageList");
    if (lang_list == NULL)
    {
        lang_list = ghb_array_new();
        ghb_dict_set(ud->settings, "SubtitleLanguageList", lang_list);
    }

    count = ghb_array_len(lang_list);
    for (ii = 0; ii < count; )
    {
        GhbValue    * lang_val = ghb_array_get(lang_list, ii);
        int           idx      = ghb_lookup_lang(lang_val);
        GtkTreeIter   iter;

        if (ii == 0)
        {
            const iso639_lang_t *lang;
            lang = ghb_iso639_lookup_by_int(idx);
            subtitle_update_pref_lang(ud, lang);
        }
        if (ghb_find_lang_row(avail, &iter, idx))
        {
            gchar       * lang;
            gint          index;
            GtkTreeIter   pos;

            gtk_tree_model_get(avail, &iter, 0, &lang, 1, &index, -1);
            gtk_tree_store_append(selected, &pos, NULL);
            gtk_tree_store_set(selected, &pos, 0, lang, 1, index, -1);
            g_free(lang);
            gtk_tree_store_remove(GTK_TREE_STORE(avail), &iter);
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
    GtkTreeView * tv;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_avail_lang"));
    ghb_init_lang_list(tv, ud);
    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_selected_lang"));
    ghb_init_lang_list_model(tv);
}

static void
subtitle_edit(GtkTreeView *tv, GtkTreePath *tp, signal_user_data_t *ud)
{
    GtkTreeModel *tm;
    GtkTreeSelection *ts;
    GtkTreeIter ti;

    ts = gtk_tree_view_get_selection(tv);
    tm = gtk_tree_view_get_model(tv);
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
        gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Subtitles"));
        response = ghb_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_hide(dialog);
        if (response != GTK_RESPONSE_OK)
        {
            ghb_dict_set(ghb_get_job_settings(ud->settings),
                         "Subtitle", backup);
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
        ghb_update_summary_info(ud);
    }
}

G_MODULE_EXPORT void
subtitle_edit_clicked_cb(GtkWidget *widget, gchar *path, signal_user_data_t *ud)
{
    GtkTreeView *tv;
    GtkTreePath *tp;

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "subtitle_list_view"));
    tp = gtk_tree_path_new_from_string (path);
    subtitle_edit(tv, tp, ud);
}

G_MODULE_EXPORT void
subtitle_row_activated_cb(GtkTreeView *tv, GtkTreePath *tp,
                          GtkTreeViewColumn *col, signal_user_data_t *ud)
{
    subtitle_edit(tv, tp, ud);
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
                ghb_update_summary_info(ud);
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

        ghb_update_summary_info(ud);
        ghb_live_reset(ud);
    }
    gtk_tree_path_free(tp);
}

G_MODULE_EXPORT void
subtitle_list_toggled_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    GtkToggleButton * selection = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                                "subtitle_selection_toggle"));
    gtk_toggle_button_set_active(selection, !active);

    GtkStack  * stack;
    GtkWidget * tab;

    stack = GTK_STACK(GHB_WIDGET(ud->builder, "SubtitleStack"));
    if (active)
        tab = GHB_WIDGET(ud->builder, "subtitle_list_tab");
    else
        tab = GHB_WIDGET(ud->builder, "subtitle_selection_tab");
    gtk_stack_set_visible_child(stack, tab);
}

G_MODULE_EXPORT void
subtitle_selection_toggled_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    GtkToggleButton * list = GTK_TOGGLE_BUTTON(GHB_WIDGET(ud->builder,
                                                    "subtitle_list_toggle"));
    gtk_toggle_button_set_active(list, !active);
}
