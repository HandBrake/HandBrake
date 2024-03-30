/* presets.c
 *
 * Copyright (C) 2008-2024 John Stebbins <stebbins@stebbins>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "presets.h"

#include "application.h"
#include "audiohandler.h"
#include "callbacks.h"
#include "handbrake/handbrake.h"
#include "handbrake/lang.h"
#include "hb-backend.h"
#include "resources.h"
#include "subtitlehandler.h"
#include "util.h"
#include "videohandler.h"

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_NESTED_PRESET 3

#if defined(_WIN32)
#define GHB_UNSAFE_FILENAME_CHARS "/:<>\"\\|?*"
#else
#define GHB_UNSAFE_FILENAME_CHARS "/"
#endif

static GhbValue *prefsDict = NULL;
static gboolean prefs_modified = FALSE;
static gchar *override_user_config_dir = NULL;
static gboolean dont_clear_presets = FALSE;

static void store_prefs(void);
static void store_presets(void);

static const char * presets_drag_entries[] = {
    "widget/presets-list-row-drop"
};

// Create and bind the tree model to the tree view for the preset list
// Also, connect up the signal that lets us know the selection has changed
G_MODULE_EXPORT void
ghb_presets_bind_tree_model (signal_user_data_t *ud)
{
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;
    GtkTreeStore *treestore;
    GtkTreeView  *treeview;
    GtkTreeSelection *selection;

    ghb_log_func();
    treeview = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
    selection = gtk_tree_view_get_selection(treeview);
    treestore = gtk_tree_store_new(5, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT,
                                   G_TYPE_STRING, G_TYPE_BOOLEAN);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(treestore));

    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Preset Name"), cell,
        "text", 0, "weight", 1, "style", 2, "editable", 4, NULL);

    g_signal_connect(cell, "edited", G_CALLBACK(preset_edited_cb), ud);

    gtk_tree_view_append_column(treeview, GTK_TREE_VIEW_COLUMN(column));
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_set_tooltip_column(treeview, 3);

    GdkContentFormats  *targets;
    GtkDropTargetAsync *drop_target;

    targets = gdk_content_formats_new(presets_drag_entries,
                                      G_N_ELEMENTS(presets_drag_entries));
    gtk_tree_view_enable_model_drag_source(treeview, GDK_BUTTON1_MASK,
                                           targets, GDK_ACTION_MOVE);
    gtk_tree_view_enable_model_drag_dest(treeview, targets, GDK_ACTION_MOVE);
    gdk_content_formats_unref(targets);

    drop_target = gtk_drop_target_async_new(targets, GDK_ACTION_MOVE);
    g_signal_connect(drop_target, "accept", G_CALLBACK(presets_drag_accept_cb), ud);
    g_signal_connect(drop_target, "drag-motion", G_CALLBACK(presets_drag_motion_cb), ud);
    g_signal_connect(drop_target, "drop", G_CALLBACK(presets_drop_cb), ud);
    gtk_widget_add_controller(GTK_WIDGET(treeview), GTK_EVENT_CONTROLLER(drop_target));

    g_signal_connect(treeview, "row-expanded", G_CALLBACK(presets_row_expanded_cb), ud);
    g_signal_connect(treeview, "row-collapsed", G_CALLBACK(presets_row_expanded_cb), ud);
    g_signal_connect(selection, "changed", G_CALLBACK(presets_list_selection_changed_cb), ud);
    g_debug("Done");
}

static hb_preset_index_t*
tree_get_index(GtkTreeModel *store, GtkTreeIter *iter)
{
    GtkTreePath       *treepath;
    int               *indices, len;
    hb_preset_index_t *path;

    treepath = gtk_tree_model_get_path(store, iter);
    indices  = gtk_tree_path_get_indices(treepath);
    len      = gtk_tree_path_get_depth(treepath);
    path     = hb_preset_index_init(indices, len);
    gtk_tree_path_free(treepath);

    return path;
}

static hb_preset_index_t*
tree_path_get_index(GtkTreePath *treepath)
{
    int *indices, len;

    indices = gtk_tree_path_get_indices(treepath);
    len     = gtk_tree_path_get_depth(treepath);

    return hb_preset_index_init(indices, len);
}

// This only handle limited depth
static GtkTreePath*
tree_path_new_from_index(const hb_preset_index_t *path)
{
    if (path == NULL || path->depth == 0)
        return NULL;

    return gtk_tree_path_new_from_indicesv((int*)path->index, path->depth);
}

#if 0
void
dump_preset_indices(const gchar *msg, hb_preset_index_t *path)
{
    gint ii;

    g_message("%s indices: len %d", msg, path->depth);
    for (ii = 0; ii < path->depth; ii++)
    {
        printf("%d ", path->index[ii]);
    }
    printf("\n");
}
#endif

static void
presets_list_show_default(signal_user_data_t *ud)
{
    hb_preset_index_t *path;

    path = hb_presets_get_default_index();
    if (path == NULL || path->depth == 0)
        return;

    GtkTreeView  *treeview;
    GtkTreeStore *store;
    GtkTreePath  *treepath;
    GtkTreeIter   iter;

    treeview = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
    store    = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
    treepath = tree_path_new_from_index(path);
    if (treepath)
    {
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath))
        {
            gtk_tree_store_set(store, &iter,
                        1, 700,
                        2, 2 ,
                        -1);
        }
        gtk_tree_path_free(treepath);
    }
    free(path);
}

static void
presets_list_clear_default(signal_user_data_t *ud)
{
    hb_preset_index_t *path;

    path = hb_presets_get_default_index();
    if (path == NULL || path->depth == 0)
        return;

    GtkTreeView  *treeview;
    GtkTreeStore *store;
    GtkTreePath  *treepath;
    GtkTreeIter   iter;

    treeview = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
    store    = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
    treepath = tree_path_new_from_index(path);
    if (treepath)
    {
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath))
        {
            gtk_tree_store_set(store, &iter,
                        1, 400,
                        2, 0 ,
                        -1);
        }
        gtk_tree_path_free(treepath);
    }
    free(path);
}

static gint
preset_get_type(hb_preset_index_t *path)
{
    GhbValue *dict;
    dict = hb_preset_get(path);
    if (dict)
    {
        return ghb_dict_get_int(dict, "Type");
    }
    else
    {
        g_warning("ghb_preset_get_type (): internal preset lookup error");
        return 0;
    }
    return 0;
}

static gboolean
preset_is_folder(hb_preset_index_t *path)
{
    GhbValue *dict;
    gboolean folder = FALSE;

    dict = hb_preset_get(path);
    if (dict)
    {
        folder = ghb_dict_get_bool(dict, "Folder");
    }
    return folder;
}

static gboolean
preset_is_default (hb_preset_index_t *path)
{
    GhbValue *dict;
    gboolean def = FALSE;

    dict = hb_preset_get(path);
    if (dict)
    {
        def = ghb_dict_get_bool(dict, "Default");
    }
    return def;
}

void
ghb_preset_to_settings(GhbValue *settings, GhbValue *preset)
{
    // Remove legacy x264Option
    ghb_dict_remove(settings, "x264Option");

    // Initialize defaults
    ghb_settings_init(settings, "Initialization");

    // Initialize the ui settings from a preset
    ghb_dict_copy(settings, preset);

    // Fix up all the internal settings that are derived from preset values.

    ghb_dict_set(settings, "scale_height", ghb_value_dup(
        ghb_dict_get_value(settings, "PictureHeight")));

    ghb_dict_set(settings, "scale_width", ghb_value_dup(
        ghb_dict_get_value(settings, "PictureWidth")));

    int crop_mode = ghb_dict_get_int(settings, "PictureCropMode");

    switch (crop_mode)
    {
        case 0:
            ghb_dict_set_string(settings, "crop_mode", "auto");
            break;
        case 1:
            ghb_dict_set_string(settings, "crop_mode", "conservative");
            break;
        case 2:
            ghb_dict_set_string(settings, "crop_mode", "none");
            break;
        case 3:
            ghb_dict_set_string(settings, "crop_mode", "custom");
            break;
        default:
            ghb_dict_set_string(settings, "crop_mode", "auto");
            break;
    }

    int width, height;
    const gchar * resolution_limit;

    width    = ghb_dict_get_int(settings, "PictureWidth");
    height   = ghb_dict_get_int(settings, "PictureHeight");
    resolution_limit = ghb_lookup_resolution_limit(width, height);
    ghb_dict_set_string(settings, "resolution_limit", resolution_limit);

    const char * rotate = ghb_dict_get_string(settings, "PictureRotate");
    if (rotate != NULL)
    {
        int         angle, hflip;
        char      * angle_opt;
        hb_dict_t * filter_settings;

        filter_settings = hb_generate_filter_settings(HB_FILTER_ROTATE,
                                                      NULL, NULL, rotate);
        angle = hb_dict_get_int(filter_settings, "angle");
        hflip = hb_dict_get_bool(filter_settings, "hflip");
        angle_opt = g_strdup_printf("%d", angle);
        ghb_dict_set_string(settings, "rotate", angle_opt);
        ghb_dict_set_int(settings, "hflip", hflip);
        g_free(angle_opt);
        hb_value_free(&filter_settings);
    }
    gint vqtype;

    vqtype = ghb_dict_get_int(settings, "VideoQualityType");

    // VideoQualityType/0/1/2 - vquality_type_/target/bitrate/constant
    // *note: target is no longer used
    switch (vqtype)
    {
    case 0:
    {
        ghb_dict_set_bool(settings, "vquality_type_bitrate", TRUE);
        ghb_dict_set_bool(settings, "vquality_type_constant", FALSE);
    } break;
    case 1:
    {
        ghb_dict_set_bool(settings, "vquality_type_bitrate", TRUE);
        ghb_dict_set_bool(settings, "vquality_type_constant", FALSE);
    } break;
    case 2:
    {
        ghb_dict_set_bool(settings, "vquality_type_bitrate", FALSE);
        ghb_dict_set_bool(settings, "vquality_type_constant", TRUE);
    } break;
    default:
    {
        ghb_dict_set_bool(settings, "vquality_type_bitrate", FALSE);
        ghb_dict_set_bool(settings, "vquality_type_constant", TRUE);
    } break;
    }

    const gchar *mode = ghb_dict_get_string(settings, "VideoFramerateMode");
    if (mode != NULL && strcmp(mode, "cfr") == 0)
    {
        ghb_dict_set_bool(settings, "VideoFramerateCFR", TRUE);
        ghb_dict_set_bool(settings, "VideoFrameratePFR", FALSE);
        ghb_dict_set_bool(settings, "VideoFramerateVFR", FALSE);
    }
    else if (mode != NULL && strcmp(mode, "pfr") == 0)
    {
        ghb_dict_set_bool(settings, "VideoFramerateCFR", FALSE);
        ghb_dict_set_bool(settings, "VideoFrameratePFR", TRUE);
        ghb_dict_set_bool(settings, "VideoFramerateVFR", FALSE);
    }
    else
    {
        ghb_dict_set_bool(settings, "VideoFramerateCFR", FALSE);
        ghb_dict_set_bool(settings, "VideoFrameratePFR", FALSE);
        ghb_dict_set_bool(settings, "VideoFramerateVFR", TRUE);
    }

    int                 encoder;
    const char         *videoPreset;

    encoder      = ghb_get_video_encoder(settings);
    videoPreset  = ghb_dict_get_string(settings, "VideoPreset");
    ghb_set_video_preset(settings, encoder, videoPreset);

    char *videoTune;
    char *tune = NULL;
    char *saveptr;
    char *tok;

    videoTune = g_strdup(ghb_dict_get_string(settings, "VideoTune"));
    if (videoTune != NULL)
    {
        tok = strtok_r(videoTune, ",./-+", &saveptr);
        ghb_dict_set_bool(settings, "x264FastDecode", FALSE);
        ghb_dict_set_bool(settings, "x264ZeroLatency", FALSE);
        while (tok != NULL)
        {
            if (!strcasecmp(tok, "fastdecode"))
            {
                ghb_dict_set_bool(settings, "x264FastDecode", TRUE);
            }
            else if (!strcasecmp(tok, "zerolatency"))
            {
                ghb_dict_set_bool(settings, "x264ZeroLatency", TRUE);
            }
            else if (tune == NULL)
            {
                tune = g_strdup(tok);
            }
            else
            {
                ghb_log("Superfluous tunes! %s", tok);
            }
            tok = strtok_r(NULL, ",./-+", &saveptr);
        }
        g_free(videoTune);
    }
    if (tune != NULL)
    {
        ghb_dict_set_string(settings, "VideoTune", tune);
        g_free(tune);
    }

    const char *videoProfile;
    videoProfile = ghb_dict_get_string(settings, "VideoProfile");
    if (videoProfile != NULL)
        ghb_dict_set_string(settings, "VideoProfile", videoProfile);

    const char *videoLevel;
    videoLevel = ghb_dict_get_string(settings, "VideoLevel");
    if (videoLevel != NULL)
        ghb_dict_set_string(settings, "VideoLevel", videoLevel);

    if (ghb_dict_get(settings, "x264OptionExtra") != NULL)
    {
        const char *optionExtra;
        optionExtra = ghb_dict_get_string(settings, "x264OptionExtra");
        ghb_dict_set_string(settings, "VideoOptionExtra", optionExtra);
    }

    // Extract copy mask to check box booleans
    GhbValue *copy_mask;
    copy_mask = ghb_dict_get(preset, "AudioCopyMask");
    if (copy_mask != NULL)
    {
        int count = ghb_array_len(copy_mask);
        int ii;
        for (ii = 0; ii < count; ii++)
        {
            GhbValue *val = ghb_array_get(copy_mask, ii);
            const char *s = ghb_value_get_string(val);
            int acodec = hb_audio_encoder_get_from_name(s);
            switch (acodec)
            {
                default:
                    break;
                case HB_ACODEC_MP2_PASS:
                    ghb_dict_set_bool(settings, "AudioAllowMP2Pass", 1);
                    break;
                case HB_ACODEC_LAME:
                case HB_ACODEC_MP3_PASS:
                    ghb_dict_set_bool(settings, "AudioAllowMP3Pass", 1);
                    break;
                case HB_ACODEC_CA_AAC:
                case HB_ACODEC_FDK_AAC:
                case HB_ACODEC_FFAAC:
                case HB_ACODEC_AAC_PASS:
                    ghb_dict_set_bool(settings, "AudioAllowAACPass", 1);
                    break;
                case HB_ACODEC_AC3:
                case HB_ACODEC_AC3_PASS:
                    ghb_dict_set_bool(settings, "AudioAllowAC3Pass", 1);
                    break;
                case HB_ACODEC_DCA:
                case HB_ACODEC_DCA_PASS:
                    ghb_dict_set_bool(settings, "AudioAllowDTSPass", 1);
                    break;
                case HB_ACODEC_DCA_HD:
                case HB_ACODEC_DCA_HD_PASS:
                    ghb_dict_set_bool(settings, "AudioAllowDTSHDPass", 1);
                    break;
                case HB_ACODEC_FFEAC3:
                case HB_ACODEC_EAC3_PASS:
                    ghb_dict_set_bool(settings, "AudioAllowEAC3Pass", 1);
                    break;
                case HB_ACODEC_FFFLAC:
                case HB_ACODEC_FFFLAC24:
                case HB_ACODEC_FLAC_PASS:
                    ghb_dict_set_bool(settings, "AudioAllowFLACPass", 1);
                    break;
                case HB_ACODEC_FFTRUEHD:
                case HB_ACODEC_TRUEHD_PASS:
                    ghb_dict_set_bool(settings, "AudioAllowTRUEHDPass", 1);
                    break;
                case HB_ACODEC_OPUS:
                case HB_ACODEC_OPUS_PASS:
                    ghb_dict_set_bool(settings, "AudioAllowOPUSPass", 1);
                    break;
            }
        }
    }
}

// Initialization order of some widgets matter because the value of
// these widgets are used to establish limits on the values that
// other widgets are allowed to take.
//
// So make sure these get initialized first.
static const char *widget_priority_list[] =
{
    "preview_count",
    "PtoPType",
    "VideoEncoder",
    "VideoQualityGranularity",
    "AudioEncoder",
    "PictureDeinterlaceFilter",
    "PictureDeinterlacePreset",
    NULL
};

void
ghb_settings_to_ui(signal_user_data_t *ud, GhbValue *dict)
{
    GhbDictIter  iter;
    const gchar *key;
    GhbValue    *gval;
    int          ii;
    GhbValue    *tmp = ghb_value_dup(dict);

    if (dict == NULL)
        return;

    for (ii = 0; widget_priority_list[ii] != NULL; ii++)
    {
        key = widget_priority_list[ii];
        gval = ghb_dict_get_value(tmp, key);
        if (gval != NULL)
            ghb_ui_settings_update(ud, dict, key, gval);
    }

    iter = ghb_dict_iter_init(tmp);
    // middle (void*) cast prevents gcc warning "dereferencing type-punned
    // pointer will break strict-aliasing rules"
    while (ghb_dict_iter_next(tmp, &iter, &key, &gval))
    {
        ghb_ui_settings_update(ud, dict, key, gval);
    }
    ghb_value_free(&tmp);
}

static char*
preset_get_fullname(hb_preset_index_t *path, const char * sep, gboolean escape)
{
    int                ii;
    GString           *gstr;
    hb_preset_index_t *tmp;
    GhbValue          *dict;

    gstr = g_string_new("");
    tmp  = hb_preset_index_dup(path);
    for (ii = 1; ii <= path->depth; ii++)
    {
        const char *name;
        tmp->depth = ii;
        dict = hb_preset_get(tmp);
        if (dict == NULL)
        {
            break;
        }
        name = ghb_dict_get_string(dict, "PresetName");
        if (name != NULL)
        {
            g_string_append(gstr, sep);
            if (escape)
            {
                char * esc = g_markup_escape_text(name, -1);
                g_string_append(gstr, esc);
                g_free(esc);
            }
            else
            {
                g_string_append(gstr, name);
            }
        }
    }
    free(tmp);
    char *str = g_string_free(gstr, FALSE);
    return str;
}

static void
set_preset_menu_button_label(signal_user_data_t *ud, hb_preset_index_t *path)
{
    char              * fullname, * text;
    const char        * description;
    GtkWidget         * widget;
    GhbValue          * dict;
    int                 type;

    dict = hb_preset_get(path);
    type = ghb_dict_get_int(dict, "Type");
    fullname = preset_get_fullname(path, "  >  ", TRUE);
    widget = ghb_builder_widget("presets_menu_button");
    text = g_strdup_printf("%s%s", type == HB_PRESET_TYPE_CUSTOM ?
                                   "Custom" : "Official", fullname);
    gtk_menu_button_set_label(GTK_MENU_BUTTON(widget), text);
    free(fullname);
    free(text);

    description = ghb_dict_get_string(dict, "PresetDescription");
    gtk_widget_set_tooltip_text(widget, description);
}

void
ghb_preset_menu_button_refresh(signal_user_data_t *ud,
                               const char *fullname, int type)
{
    hb_preset_index_t * path;

    path = hb_preset_search_index(fullname, 0, type);
    set_preset_menu_button_label(ud, path);
}

static void
select_preset2(signal_user_data_t *ud, hb_preset_index_t *path)
{
    GtkTreeView      *treeview;
    GtkTreeSelection *selection;
    GtkTreeModel     *store;
    GtkTreeIter       iter;
    GtkTreePath      *treepath;

    if (path == NULL || path->depth == 0)
        return;

    treeview  = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
    selection = gtk_tree_view_get_selection (treeview);
    store     = gtk_tree_view_get_model (treeview);
    treepath  = tree_path_new_from_index(path);
    if (treepath != NULL)
    {
        gtk_tree_view_expand_to_path(treeview, treepath);
        if (gtk_tree_model_get_iter(store, &iter, treepath))
        {
            gtk_tree_selection_select_iter(selection, &iter);
        }
        else
        {
            if (gtk_tree_model_get_iter_first(store, &iter))
                gtk_tree_selection_select_iter(selection, &iter);
        }
        // Make the selection visible in scroll window if it is not.
        gtk_tree_view_scroll_to_cell(treeview, treepath, NULL, FALSE, 0, 0);
        gtk_tree_path_free(treepath);
    }
    set_preset_menu_button_label(ud, path);

    int type = preset_get_type(path);
    GSimpleAction * action;

    action =GHB_APPLICATION_ACTION("preset-rename");
    g_simple_action_set_enabled(action, type == HB_PRESET_TYPE_CUSTOM);
    action = GHB_APPLICATION_ACTION("preset-save");
    g_simple_action_set_enabled(action, type == HB_PRESET_TYPE_CUSTOM);
    action = GHB_APPLICATION_ACTION("preset-default");
    g_simple_action_set_enabled(action, !preset_is_default(path));
}

void
ghb_select_preset(signal_user_data_t *ud, const char *name, int type)
{
    hb_preset_index_t *path;

    path = hb_preset_search_index(name, 1, type);
    if (path != NULL)
    {
        select_preset2(ud, path);
        free(path);
    }
}

void
ghb_select_default_preset(signal_user_data_t *ud)
{
    hb_preset_index_t *path;

    path = hb_presets_get_default_index();
    if (path == NULL || path->depth == 0)
    {
        // No default set, find original "default" preset
        g_free(path);
        path = hb_preset_search_index("Fast 1080p30", 1, HB_PRESET_TYPE_ALL);
    }
    if (path == NULL || path->depth == 0)
    {
        int index[2] = {0, 0};

        // Could not find original default, try first available preset
        g_free(path);
        path = hb_preset_index_init(index, 2);
    }
    if (path != NULL)
    {
        select_preset2(ud, path);
        g_free(path);
    }
}

gchar *
ghb_get_user_config_dir (const char *subdir)
{
    const gchar * dir, * ghb = "ghb";
    gchar       * config;

    if (override_user_config_dir != NULL)
    {
        dir = override_user_config_dir;
    }
    else
    {
        dir = g_get_user_config_dir();
    }
    if (dir == NULL || !g_file_test(dir, G_FILE_TEST_IS_DIR))
    {
        dir = g_get_home_dir();
        ghb = ".ghb";
    }
    if (dir == NULL || !g_file_test(dir, G_FILE_TEST_IS_DIR))
    {
        // Last ditch, use CWD
        dir = "./";
        ghb = ".ghb";
    }
    config = g_strdup_printf("%s/%s", dir, ghb);
    if (!g_file_test(config, G_FILE_TEST_IS_DIR))
        g_mkdir (config, 0755);
    if (subdir)
    {
        gchar **split;
        gint    ii;

        split = g_strsplit(subdir, G_DIR_SEPARATOR_S, -1);
        for (ii = 0; split[ii] != NULL; ii++)
        {
            gchar *tmp;

            tmp = g_strdup_printf ("%s/%s", config, split[ii]);
            g_free(config);
            config = tmp;
            if (!g_file_test(config, G_FILE_TEST_IS_DIR))
                g_mkdir (config, 0755);
        }
        g_strfreev(split);
    }
    return config;
}

void
ghb_override_user_config_dir(char *dir)
{
    override_user_config_dir = dir;
}

static void
write_config_file(const gchar *name, GhbValue *dict)
{
    gchar *config, *path;

    config = ghb_get_user_config_dir(NULL);
    path   = g_strdup_printf ("%s/%s", config, name);
    g_free(config);
    ghb_json_write_file(path, dict);
    g_free(path);
}

void
ghb_write_settings_file(const gchar *path, GhbValue *dict)
{
    ghb_json_write_file(path, dict);
}

static int
presets_add_config_file(const gchar *name)
{
    gchar      *config, *path;
    hb_value_t *preset;

    config = ghb_get_user_config_dir(NULL);
    path   = g_strdup_printf ("%s/%s", config, name);
    g_free(config);
    if (!g_file_test(path, G_FILE_TEST_IS_REGULAR))
        return -1;
    preset = hb_presets_read_file(path);
    g_free(path);
    if (preset != NULL)
    {
        int hb_major, hb_minor, hb_micro;
        int major, minor, micro;
        hb_presets_version(preset, &major, &minor, &micro);
        hb_presets_current_version(&hb_major, &hb_minor, &hb_micro);
        if (major != hb_major)
        {
            // Make a backup whenever the preset version changes
            config  = ghb_get_user_config_dir(NULL);
            path    = g_strdup_printf ("%s/presets.%d.%d.%d.json",
                            config, major, minor, micro);
            hb_value_write_json(preset, path);
            g_free(config);
            g_free(path);
        }

        if (major > hb_major)
        {
            // Change in major indicates non-backward compatible preset changes.
            // We can't successfully load presets that were generated by
            // a newer version of handbrake than is currently running.
            hb_value_free(&preset);
            return -2;
        }

        hb_value_t *imported;
        hb_presets_import(preset, &imported);
        hb_presets_add(imported);
        if (major != hb_major || minor != hb_minor || micro != hb_micro)
        {
            // Reload hb builtin presets
            hb_presets_builtin_update();
            store_presets();
        }
        hb_value_free(&imported);
        hb_value_free(&preset);
        return 0;
    }
    return -1;
}

static GhbValue*
read_config_file(const gchar *name)
{
    gchar    *config, *path;
    GhbValue *gval = NULL;

    config = ghb_get_user_config_dir(NULL);
    path   = g_strdup_printf ("%s/%s", config, name);
    g_free(config);
    if (g_file_test(path, G_FILE_TEST_IS_REGULAR))
    {
        gval = ghb_json_parse_file(path);
    }
    g_free(path);
    return gval;
}

GhbValue*
ghb_read_settings_file(const gchar *path)
{
    GhbValue *gval = NULL;

    if (g_file_test(path, G_FILE_TEST_IS_REGULAR))
    {
        gval = ghb_json_parse_file(path);
    }
    return gval;
}

#if 0
// Currently unused, but keeping around just in case...
gboolean
ghb_lock_file(const gchar *name)
{
#if !defined(_WIN32)
    gchar *config, *path;
    int fd, lock = 0;

    config = ghb_get_user_config_dir(NULL);
    path = g_strdup_printf ("%s/%s", config, name);
    fd = open(path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    if (fd >= 0)
        lock = lockf(fd, F_TLOCK, 0);
    if (lock)
        close(fd);
    g_free(config);
    g_free(path);
    return !lock;
#else
    return 1;
#endif
}
#endif

void
ghb_write_pid_file (void)
{
#if !defined(_WIN32)
    gchar *config, *path;
    pid_t  pid;
    FILE  *fp;
    int    fd;

    pid    = getpid();
    config = ghb_get_user_config_dir(NULL);
    path   = g_strdup_printf ("%s/ghb.pid.%d", config, pid);
    fp     = g_fopen(path, "w");

    if (fp != NULL)
    {
        fprintf(fp, "%d\n", pid);
        fclose(fp);
    }

    fd = open(path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    if (fd >= 0)
    {
        (void) !!lockf(fd, F_TLOCK, 0);
    }

    g_free(config);
    g_free(path);
#endif
}

int
ghb_find_pid_file (void)
{
    const gchar *file;
    gchar       *config;

    config = ghb_get_user_config_dir(NULL);

    if (g_file_test(config, G_FILE_TEST_IS_DIR))
    {
        GDir *gdir;
        gdir = g_dir_open(config, 0, NULL);
        file = g_dir_read_name(gdir);
        while (file)
        {
            if (strncmp(file, "ghb.pid.", 8) == 0)
            {
                gchar *path;
                int    pid;

                sscanf(file, "ghb.pid.%d", &pid);
                path = g_strdup_printf("%s/%s", config, file);

#if !defined(_WIN32)
                int fd, lock = 1;

                fd = open(path, O_RDWR);
                if (fd >= 0)
                {
                    lock = lockf(fd, F_TLOCK, 0);
                }
                if (lock == 0)
                {
                    close(fd);
                    g_dir_close(gdir);
                    g_unlink(path);
                    g_free(path);
                    g_free(config);
                    return pid;
                }
                g_free(path);
                close(fd);
#else
                g_dir_close(gdir);
                g_unlink(path);
                g_free(path);
                g_free(config);
                return pid;
#endif
            }
            file = g_dir_read_name(gdir);
        }
        g_dir_close(gdir);
    }
    g_free(config);
    return -1;
}

static void
remove_config_file(const gchar *name)
{
    gchar *config, *path;

    config = ghb_get_user_config_dir(NULL);
    path   = g_strdup_printf ("%s/%s", config, name);
    if (g_file_test(path, G_FILE_TEST_IS_REGULAR))
    {
        g_unlink(path);
    }
    g_free(path);
    g_free(config);
}

void
ghb_pref_save(GhbValue *settings, const gchar *key)
{
    const GhbValue *value, *value2;

    value = ghb_dict_get_value(settings, key);
    if (value != NULL)
    {
        GhbValue *dict;
        dict = ghb_dict_get(prefsDict, "Preferences");
        if (dict == NULL) return;
        value2 = ghb_dict_get(dict, key);
        if (ghb_value_cmp(value, value2) != 0)
        {
            ghb_dict_set(dict, key, ghb_value_dup(value));
            store_prefs();
            prefs_modified = FALSE;
        }
    }
}

void
ghb_pref_set(GhbValue *settings, const gchar *key)
{
    const GhbValue *value, *value2;

    value = ghb_dict_get_value(settings, key);
    if (value != NULL)
    {
        GhbValue *dict;
        dict = ghb_dict_get(prefsDict, "Preferences");
        if (dict == NULL) return;
        value2 = ghb_dict_get(dict, key);
        if (ghb_value_cmp(value, value2) != 0)
        {
            ghb_dict_set(dict, key, ghb_value_dup(value));
            prefs_modified = TRUE;
        }
    }
}

void
ghb_prefs_store(void)
{
    if (prefs_modified)
    {
        store_prefs();
        prefs_modified = FALSE;
    }
}

void
ghb_settings_init(GhbValue *settings, const char *name)
{
    GhbValue    *internal;

    GhbValue *internalDict = ghb_resource_get("internal-defaults");
    // Setting a ui widget will cause the corresponding setting
    // to be set, but it also triggers a callback that can
    // have the side effect of using other settings values
    // that have not yet been set.  So set *all* settings first
    // then update the ui.
    internal = ghb_dict_get(internalDict, name);
    ghb_dict_copy(settings, internal);
}

void
ghb_settings_close (void)
{
    if (prefsDict)
        ghb_value_free(&prefsDict);
}

#if defined(_WIN32)
gchar*
FindFirstCDROM(void)
{
    gint ii, drives;
    gchar drive[5];

    strcpy(drive, "A:" G_DIR_SEPARATOR_S);
    drives = GetLogicalDrives();
    for (ii = 0; ii < 26; ii++)
    {
        if (drives & 0x01)
        {
            guint dtype;

            drive[0] = 'A' + ii;
            dtype = GetDriveType(drive);
            if (dtype == DRIVE_CDROM)
            {
                return g_strdup(drive);
            }
        }
        drives >>= 1;
    }
    return NULL;
}
#endif

void
ghb_prefs_load(signal_user_data_t *ud)
{
    GhbValue    *dict, *internal;
    GhbValue *internalDict;

    internalDict = ghb_resource_get("internal-defaults");
    prefsDict    = read_config_file("preferences.json");
    if (prefsDict == NULL)
        prefsDict    = read_config_file("preferences");
    if (prefsDict == NULL)
        prefsDict = ghb_dict_new();
    dict     = ghb_dict_get(prefsDict, "Preferences");
    internal = ghb_dict_get(internalDict, "Preferences");
    if (dict == NULL && internal != NULL)
    {
        dict = ghb_value_dup(internal);
        ghb_dict_set(prefsDict, "Preferences", dict);

        const char *dir = g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS);
        if (dir == NULL)
        {
            dir = ".";
        }
        ghb_dict_set_string(dict, "ExportDirectory", dir);

        dir = g_get_user_special_dir(G_USER_DIRECTORY_VIDEOS);
        if (dir == NULL)
        {
            dir = ".";
        }
        ghb_dict_set_string(dict, "destination_dir", dir);

        ghb_dict_set_string(dict, "SrtDir", dir);
#if defined(_WIN32)
        gchar *source;

        source = FindFirstCDROM();
        if (source == NULL)
        {
            source = g_strdup("C:" G_DIR_SEPARATOR_S);
        }
        ghb_dict_set_string(dict, "default_source", source);
        g_free(source);
#endif
        store_prefs();
    }
    // Migrate from legacy preferences
    if (ghb_dict_get_int(dict, "DiskFreeLimit"))
    {
        int limit_gb = ghb_dict_get_int(dict, "DiskFreeLimit") / 1000;
        if (limit_gb <= 0) limit_gb = 1;
        ghb_dict_set_int(dict, "DiskFreeLimitGB", limit_gb);
        ghb_dict_remove(dict, "DiskFreeLimit");
    }
    ghb_dict_remove(dict, "show_presets");
}

void
ghb_prefs_to_settings(GhbValue *settings)
{
    // Initialize the ui from presets file.
    GhbValue *dict;

    if (prefsDict == NULL)
        return;

    // Setting a ui widget will cause the corresponding setting
    // to be set, but it also triggers a callback that can
    // have the side effect of using other settings values
    // that have not yet been set.  So set *all* settings first
    // then update the ui.
    dict     = ghb_dict_get(prefsDict, "Preferences");
    ghb_dict_copy(settings, dict);
}

static hb_preset_index_t *
get_selected_path(signal_user_data_t *ud)
{
    GtkTreeView      *treeview;
    GtkTreeSelection *selection;
    GtkTreeModel     *store;
    GtkTreeIter       iter;

    treeview  = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
    selection = gtk_tree_view_get_selection(treeview);
    if (gtk_tree_selection_get_selected(selection, &store, &iter))
    {
        return tree_get_index(store, &iter);
    }
    return NULL;
}

G_MODULE_EXPORT void
show_presets_action_cb(GSimpleAction *action, GVariant *value,
                       signal_user_data_t *ud)
{
    GtkWidget *presets_window;
    GtkWidget *hb_window;
    int w, h;

    hb_window = ghb_builder_widget("hb_window");
    if (!gtk_widget_is_visible(hb_window))
    {
        return;
    }

    w = ghb_dict_get_int(ud->prefs, "presets_window_width");
    h = ghb_dict_get_int(ud->prefs, "presets_window_height");

    presets_window = ghb_builder_widget("presets_window");
    if (w > 200 && h > 200)
    {
        gtk_window_set_default_size(GTK_WINDOW(presets_window), w, h);
    }
    gtk_window_present(GTK_WINDOW(presets_window));
}

G_MODULE_EXPORT void
presets_window_save_size_cb (GtkWidget *widget, GParamSpec *spec, gpointer data)
{
    signal_user_data_t *ud = ghb_ud();
    if (gtk_widget_get_visible(widget))
    {
        gint w, h, ww, wh;
        w = ghb_dict_get_int(ud->prefs, "presets_window_width");
        h = ghb_dict_get_int(ud->prefs, "presets_window_height");
        gtk_window_get_default_size(GTK_WINDOW(widget), &ww, &wh);

        if ( w != ww || h != wh )
        {
            ghb_dict_set_int(ud->prefs, "presets_window_width", ww);
            ghb_dict_set_int(ud->prefs, "presets_window_height", wh);
            ghb_pref_set(ud->prefs, "presets_window_width");
            ghb_pref_set(ud->prefs, "presets_window_height");
            ghb_prefs_store();
        }
    }
}

G_MODULE_EXPORT void
preset_select_action_cb(GSimpleAction *action, GVariant *param,
                        signal_user_data_t *ud)
{
    const char * preset_path = g_variant_get_string(param, NULL);
    int          type        = preset_path[0] - '0';

    ghb_select_preset(ud, &preset_path[1], type);
}

G_MODULE_EXPORT void
preset_reload_action_cb(GSimpleAction *action, GVariant *param,
                        signal_user_data_t *ud)
{
    const char * preset_path;
    int          type;

    type         = ghb_dict_get_int(ud->settings, "Type");
    preset_path  = ghb_dict_get_string(ud->settings, "PresetFullName");
    if (preset_path != NULL)
    {
        ghb_select_preset(ud, preset_path, type);
    }
}

void
ghb_presets_menu_init(signal_user_data_t *ud)
{
    GMenu              * menu = g_menu_new();
    hb_preset_index_t  * path;
    GhbValue           * presets;
    int                  menu_count, submenu_count, type, ii, jj, kk;
    char              ** official_names;

    // Add official presets
    path   = hb_preset_index_init(NULL, 0);
    presets = hb_presets_get_folder_children(path);
    if (presets == NULL)
    {
        g_warning(_("ghb_presets_menu_init: Failed to find presets folder."));
        g_free(path);
        return;
    }

    menu_count = ghb_array_len(presets);
    // Menus can't contain the same name twice.  Since our preset list
    // allows official and custom preset categories with the same name
    // I must modify one of them when duplicates exist :(
    official_names = calloc(menu_count + 1, sizeof(char*));
    kk = 0;
    path->depth++;
    // Process Official Presets in first pass, then Custom Presets
    for (type = 0; type < 2; type++)
    {
        GMenu * section = g_menu_new();
        for (ii = 0; ii < menu_count; ii++)
        {
            GhbValue    * dict;
            const gchar * folder_name;
            gint          folder_type;
            gboolean      is_folder;
            GhbValue    * folder;
            GString     * folder_str;
            char        * menu_item_name;

            path->index[path->depth-1] = ii;

            dict        = ghb_array_get(presets, ii);
            folder_name = ghb_dict_get_string(dict, "PresetName");
            folder_type = ghb_dict_get_int(dict, "Type");
            is_folder   = ghb_dict_get_bool(dict, "Folder");

            if (folder_type != type)
            {
                continue;
            }

            if (type == HB_PRESET_TYPE_OFFICIAL)
            {
                // Add folder name to list of official names
                official_names[kk++] = g_strdup(folder_name);
            }

            folder_str = g_string_new("");
            g_string_append_printf(folder_str, "%d/%s",
                                   folder_type, folder_name);
            if (is_folder)
            {
                GMenu * submenu = g_menu_new();

                folder = hb_presets_get_folder_children(path);
                submenu_count = ghb_array_len(folder);
                for (jj = 0; jj < submenu_count; jj++)
                {
                    const gchar * name;
                    GString     * preset_str = g_string_new(folder_str->str);

                    dict        = ghb_array_get(folder, jj);
                    name        = ghb_dict_get_string(dict, "PresetName");
                    type        = ghb_dict_get_int(dict, "Type");
                    is_folder   = ghb_dict_get_bool(dict, "Folder");

                    // Sanity check, Preset types must match their folder
                    if (type != folder_type)
                    {
                        continue;
                    }
                    // Enforce 2 level limit
                    if (is_folder)
                    {
                        continue;
                    }
                    g_string_append_printf(preset_str, "/%s", name);

                    char * preset_path;
                    char * detail_action;

                    preset_path = g_string_free(preset_str, FALSE);
                    detail_action = g_strdup_printf("app.preset-select(\"%s\")",
                                                    preset_path);
                    g_menu_append(submenu, name, detail_action);
                    free(preset_path);
                    free(detail_action);
                }
                if (type == HB_PRESET_TYPE_CUSTOM &&
                    g_strv_contains((const char**)official_names, folder_name))
                {
                    menu_item_name = g_strdup_printf("My %s", folder_name);
                }
                else
                {
                    menu_item_name = g_strdup(folder_name);
                }
                g_menu_append_submenu(section, menu_item_name,
                                      G_MENU_MODEL(submenu));
                g_free(menu_item_name);
            }
            g_string_free(folder_str, TRUE);
        }
        g_menu_append_section(menu,
                              type == HB_PRESET_TYPE_CUSTOM ?
                              "Custom" : "Official",
                              G_MENU_MODEL(section));
    }
    g_free(path);
    g_strfreev(official_names);

    GtkMenuButton * mb;

    mb = GTK_MENU_BUTTON(ghb_builder_widget("presets_menu_button"));
    gtk_menu_button_set_menu_model(mb, G_MENU_MODEL(menu));
}


static void
presets_menu_clear(signal_user_data_t *ud)
{
    GtkMenuButton * mb;
    GMenuModel    * mm;
    GMenu         * presets;

    mb = GTK_MENU_BUTTON(ghb_builder_widget("presets_menu_button"));
    mm = gtk_menu_button_get_menu_model(mb);
    gtk_menu_button_set_menu_model(mb, NULL);
    g_object_unref(G_OBJECT(mm));

    presets = G_MENU(ghb_builder_object("presets-menu-official"));
    g_menu_remove_all(presets);
    presets = G_MENU(ghb_builder_object("presets-menu-custom"));
    g_menu_remove_all(presets);
}

void
ghb_presets_menu_reinit(signal_user_data_t *ud)
{
    presets_menu_clear(ud);
    ghb_presets_menu_init(ud);
}

void
ghb_presets_list_init(signal_user_data_t *ud, const hb_preset_index_t *path)
{
    hb_preset_index_t *next_path;
    GhbValue          *folder;
    GtkTreeView       *treeview;
    GtkTreeStore      *store;
    GtkTreePath       *parent_path;
    GtkTreeIter        iter, parent_iter, *piter;
    gint               count, ii;

    if (path == NULL)
    {
        hb_preset_index_t *p = hb_preset_index_init(NULL, 0);
        ghb_presets_list_init(ud, p);
        free(p);
        return;
    }
    next_path = hb_preset_index_dup(path);
    folder    = hb_presets_get_folder_children(path);
    if (folder == NULL)
    {
        g_warning(_("Failed to find parent folder when adding child."));
        g_free(next_path);
        return;
    }
    treeview    = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
    store       = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
    parent_path = tree_path_new_from_index(path);
    if (parent_path != NULL)
    {
        gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &parent_iter,
                                parent_path);
        piter = &parent_iter;
        gtk_tree_path_free(parent_path);
    }
    else
    {
        piter = NULL;
    }
    count = ghb_array_len(folder);
    next_path->depth++;
    for (ii = 0; ii < count; ii++)
    {
        GhbValue    *dict;
        const gchar *name;
        gchar       *custom_name = NULL;
        gint         type;
        const gchar *description;
        gboolean     is_folder;
        gboolean     def;

        next_path->index[next_path->depth-1] = ii;

        // Additional settings, add row
        dict        = ghb_array_get(folder, ii);
        name        = ghb_dict_get_string(dict, "PresetName");
        description = ghb_dict_get_string(dict, "PresetDescription");
        type        = ghb_dict_get_int(dict, "Type");
        is_folder   = ghb_dict_get_bool(dict, "Folder");
        def         = ghb_dict_get_bool(dict, "Default");

        gtk_tree_store_append(store, &iter, piter);
        if (is_folder && type == HB_PRESET_TYPE_CUSTOM)
        {
            custom_name = g_strdup_printf("Custom %s", name);
            name = custom_name;
        }
        gtk_tree_store_set(store, &iter,
                            0, name,
                            1, def ? 700 : 400,
                            2, def ? 2   : 0,
                            3, description,
                            4, type == HB_PRESET_TYPE_OFFICIAL ? 0 : 1,
                            -1);
        free(custom_name);
        if (is_folder)
        {
            ghb_presets_list_init(ud, next_path);
            if (ghb_dict_get_bool(dict, "FolderOpen"))
            {
                GtkTreePath *tpath;
                tpath = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
                gtk_tree_view_expand_to_path(treeview, tpath);
                gtk_tree_path_free(tpath);
            }
        }
    }
    g_free(next_path);
    if (path == NULL)
    {
        presets_list_show_default(ud);
    }
}

static void
presets_list_clear(signal_user_data_t *ud)
{
    GtkTreeView  *treeview;
    GtkTreeModel *store;
    GtkTreeIter   iter;
    gboolean      valid;

    treeview = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
    store    = gtk_tree_view_get_model(treeview);
    valid    = gtk_tree_model_get_iter_first(store, &iter);
    while (valid)
    {
        gtk_tree_store_remove(GTK_TREE_STORE(store), &iter);
        valid = gtk_tree_model_get_iter_first(store, &iter);
    }
}

void
ghb_presets_list_reinit(signal_user_data_t *ud)
{
    presets_list_clear(ud);
    ghb_presets_list_init(ud, NULL);
}

static void
presets_list_update_item(
    signal_user_data_t       *ud,
    const hb_preset_index_t  *path,
    gboolean                  recurse)
{
    GhbValue     *dict;
    GtkTreeView  *treeview;
    GtkTreeStore *store;
    GtkTreePath  *treepath;
    GtkTreeIter   iter;
    const gchar  *name;
    const gchar  *description;
    gint          type;
    gboolean      is_folder;
    gboolean      def;

    dict = hb_preset_get(path);
    if (dict == NULL)
        return;

    treeview = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
    store    = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
    treepath = tree_path_new_from_index(path);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath);

    // Additional settings, add row
    name        = ghb_dict_get_string(dict, "PresetName");
    description = ghb_dict_get_string(dict, "PresetDescription");
    type        = ghb_dict_get_int(dict, "Type");
    is_folder   = ghb_dict_get_bool(dict, "Folder");
    def         = ghb_dict_get_bool(dict, "Default");

    gtk_tree_store_set(store, &iter,
                        0, name,
                        1, def ? 700 : 400,
                        2, def ? 2   : 0,
                        3, description,
                        4, type == HB_PRESET_TYPE_OFFICIAL ? 0 : 1,
                        -1);
    if (recurse && is_folder)
    {
        ghb_presets_list_init(ud, path);
    }
}

static void
presets_list_append(signal_user_data_t *ud, const hb_preset_index_t *path)
{
    hb_preset_index_t *folder_path;
    hb_value_t        *dict;
    GtkTreeView       *treeview;
    GtkTreeStore      *store;
    GtkTreePath       *folder_treepath;
    GtkTreeIter        iter, parent_iter, *piter;
    const gchar       *name;
    const gchar       *description;
    gint               type;
    gboolean           is_folder;
    gboolean           def;

    folder_path = hb_preset_index_dup(path);
    folder_path->depth--;

    dict = hb_preset_get(path);
    if (dict == NULL)
    {
        g_message("Ack! Desync between presets and preset list");
        return;
    }

    treeview = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
    store    = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));

    folder_treepath = tree_path_new_from_index(folder_path);
    if (folder_treepath != NULL)
    {
        gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &parent_iter,
                                folder_treepath);
        piter = &parent_iter;
        gtk_tree_path_free(folder_treepath);
    }
    else
    {
        piter = NULL;
    }

    // Additional settings, add row
    name        = ghb_dict_get_string(dict, "PresetName");
    description = ghb_dict_get_string(dict, "PresetDescription");
    type        = ghb_dict_get_int(dict, "Type");
    is_folder   = ghb_dict_get_bool(dict, "Folder");
    def         = ghb_dict_get_bool(dict, "Default");

    gtk_tree_store_append(store, &iter, piter);
    gtk_tree_store_set(store, &iter,
                        0, name,
                        1, def ? 700 : 400,
                        2, def ? 2   : 0,
                        3, description,
                        4, type == HB_PRESET_TYPE_OFFICIAL ? 0 : 1,
                        -1);
    if (is_folder)
    {
        ghb_presets_list_init(ud, path);
    }
}

static void
presets_list_remove(signal_user_data_t *ud, hb_preset_index_t *path)
{
    GtkTreeView  *treeview;
    GtkTreePath  *treepath;
    GtkTreeIter   iter;
    GtkTreeStore *store;

    treeview = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
    store    = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
    treepath = tree_path_new_from_index(path);
    if (treepath)
    {
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath))
            gtk_tree_store_remove(store, &iter);
        gtk_tree_path_free(treepath);
    }
}

void
ghb_save_queue(GhbValue *queue)
{
    pid_t  pid;
    char  *name;

    pid  = getpid();
    name = g_strdup_printf ("queue.%" PRId64, (int64_t)pid);
    write_config_file(name, queue);
    g_free(name);
}

GhbValue*
ghb_load_old_queue(int pid)
{
    GhbValue *queue;
    char     *name;

    name  = g_strdup_printf ("queue.%d", pid);
    queue = read_config_file(name);
    g_free(name);
    return queue;
}

void
ghb_remove_old_queue_file(int pid)
{
    char *name;

    name = g_strdup_printf ("queue.%d", pid);
    remove_config_file(name);
    g_free(name);
}

GhbValue* ghb_create_copy_mask(GhbValue *settings)
{
    GhbValue *copy_mask = ghb_array_new();
    if (ghb_dict_get_bool(settings, "AudioAllowMP2Pass"))
    {
        ghb_array_append(copy_mask, ghb_string_value_new("copy:mp2"));
    }
    if (ghb_dict_get_bool(settings, "AudioAllowMP3Pass"))
    {
        ghb_array_append(copy_mask, ghb_string_value_new("copy:mp3"));
    }
    if (ghb_dict_get_bool(settings, "AudioAllowAACPass"))
    {
        ghb_array_append(copy_mask, ghb_string_value_new("copy:aac"));
    }
    if (ghb_dict_get_bool(settings, "AudioAllowAC3Pass"))
    {
        ghb_array_append(copy_mask, ghb_string_value_new("copy:ac3"));
    }
    if (ghb_dict_get_bool(settings, "AudioAllowDTSPass"))
    {
        ghb_array_append(copy_mask, ghb_string_value_new("copy:dts"));
    }
    if (ghb_dict_get_bool(settings, "AudioAllowDTSHDPass"))
    {
        ghb_array_append(copy_mask, ghb_string_value_new("copy:dtshd"));
    }
    if (ghb_dict_get_bool(settings, "AudioAllowEAC3Pass"))
    {
        ghb_array_append(copy_mask, ghb_string_value_new("copy:eac3"));
    }
    if (ghb_dict_get_bool(settings, "AudioAllowFLACPass"))
    {
        ghb_array_append(copy_mask, ghb_string_value_new("copy:flac"));
    }
    if (ghb_dict_get_bool(settings, "AudioAllowTRUEHDPass"))
    {
        ghb_array_append(copy_mask, ghb_string_value_new("copy:truehd"));
    }
    if (ghb_dict_get_bool(settings, "AudioAllowOPUSPass"))
    {
        ghb_array_append(copy_mask, ghb_string_value_new("copy:opus"));
    }
    return copy_mask;
}

// Translate internal values to preset key, value pairs
GhbValue*
ghb_settings_to_preset(GhbValue *settings)
{
    GhbValue *preset = ghb_value_dup(settings);

    gboolean br, constant;

    ghb_dict_remove(preset, "title");
    ghb_dict_set_bool(preset, "Default", 0);

    br = ghb_dict_get_bool(preset, "vquality_type_bitrate");
    constant = ghb_dict_get_bool(preset, "vquality_type_constant");

    const char * angle    = ghb_dict_get_string(preset, "rotate");
    int          hflip    = ghb_dict_get_int(preset, "hflip");
    char       * rot_flip = g_strdup_printf("angle=%s:hflip=%d", angle, hflip);
    ghb_dict_set_string(preset, "PictureRotate", rot_flip);
    g_free(rot_flip);

    const gchar * crop_mode;
    int autocrop, conservativecrop, customcrop;

    crop_mode = ghb_dict_get_string(settings, "crop_mode");

    autocrop         = !strcmp(crop_mode, "auto");
    conservativecrop = !strcmp(crop_mode, "conservative");
    customcrop       = !strcmp(crop_mode, "custom");

    if (autocrop)
    {
        ghb_dict_set_int(preset, "PictureCropMode", 0);
    }
    else if (conservativecrop)
    {
        ghb_dict_set_int(preset, "PictureCropMode", 1);
    }
    else if (customcrop)
    {
        ghb_dict_set_int(preset, "PictureCropMode", 3);
    }
    else
    {
        ghb_dict_set_int(preset, "PictureCropMode", 2);
    }

    // VideoQualityType/0/1/2 - vquality_type_/target/bitrate/constant
    // *note: target is no longer used
    if (br)
    {
        ghb_dict_set_int(preset, "VideoQualityType", 1);
    }
    else if (constant)
    {
        ghb_dict_set_int(preset, "VideoQualityType", 2);
    }

    if (ghb_dict_get_bool(preset, "VideoFramerateCFR"))
    {
        ghb_dict_set_string(preset, "VideoFramerateMode", "cfr");
    }
    else if (ghb_dict_get_bool(preset, "VideoFrameratePFR"))
    {
        ghb_dict_set_string(preset, "VideoFramerateMode", "pfr");
    }
    else
    {
        ghb_dict_set_string(preset, "VideoFramerateMode", "vfr");
    }

    GhbValue *alist, *adict;
    gint count, ii;

    alist = ghb_dict_get(preset, "AudioList");
    count = ghb_array_len(alist);
    for (ii = 0; ii < count; ii++)
    {
        gdouble drc;

        adict = ghb_array_get(alist, ii);
        drc = ghb_dict_get_double(adict, "AudioTrackDRCSlider");
        if (drc < 1.0)
        {
            ghb_dict_set_double(adict, "AudioTrackDRCSlider", 0.0);
        }
    }

    GhbValue *copy_mask = ghb_create_copy_mask(preset);
    ghb_dict_set(preset, "AudioCopyMask", copy_mask);

    GString *str = g_string_new("");
    const char *sep = "";
    const char *tune = ghb_dict_get_string(preset, "VideoTune");
    if (tune != NULL && strcasecmp(tune, "none"))
    {
        g_string_append_printf(str, "%s", tune);
        sep = ",";
    }
    int encoder = ghb_get_video_encoder(settings);
    if (encoder & (HB_VCODEC_X264_MASK | HB_VCODEC_SVT_AV1_MASK))
    {
        if (ghb_dict_get_bool(preset, "x264FastDecode"))
        {
            g_string_append_printf(str, "%s%s", sep, "fastdecode");
            sep = ",";
        }
        if (ghb_dict_get_bool(preset, "x264ZeroLatency"))
        {
            g_string_append_printf(str, "%s%s", sep, "zerolatency");
        }
    }
    char *tunes;
    tunes = g_string_free(str, FALSE);
    ghb_dict_set_string(preset, "VideoTune", tunes);
    g_free(tunes);

    GhbValue *in_val, *out_val;

    // Convert PictureModulus to correct data type
    in_val = ghb_dict_get(preset, "PictureModulus");
    out_val = ghb_value_xform(in_val, GHB_INT);
    if (out_val != NULL)
        ghb_dict_set(preset, "PictureModulus", out_val);

    return preset;
}

static guint prefs_timeout_id = 0;

static gboolean
delayed_store_prefs(gpointer data)
{
    write_config_file("preferences.json", prefsDict);
    prefs_timeout_id = 0;
    return FALSE;
}

static void
store_presets (void)
{
    gchar      *config, *path;
    hb_value_t *presets;

    config  = ghb_get_user_config_dir(NULL);
    path    = g_strdup_printf ("%s/%s", config, "presets.json");
    presets = hb_presets_get();
    hb_presets_write_json(presets, path);
    g_free(config);
    g_free(path);
}

static void
store_prefs(void)
{
    if (prefs_timeout_id != 0)
    {
        GMainContext *mc;
        GSource *source;

        mc = g_main_context_default();
        source = g_main_context_find_source_by_id(mc, prefs_timeout_id);
        if (source != NULL)
            g_source_destroy(source);
    }
    prefs_timeout_id = g_timeout_add(100, (GSourceFunc)delayed_store_prefs, NULL);
}

void
ghb_presets_load(signal_user_data_t *ud)
{
    int result = presets_add_config_file("presets.json");
    if (result == -2)
    {
        // The above can fail if the presets file was written by a
        // more recent version of HandBrake than is currently running.
        // Look for a backup version that matches the currently running
        // version.
        GtkWindow *hb_window = GTK_WINDOW(ghb_builder_widget("hb_window"));
        if (!ghb_question_dialog_run(hb_window, GHB_ACTION_DESTRUCTIVE,
            _("Load backup presets"), _("Get me out of here!"),
            _("Presets found are newer than what is supported by this version of HandBrake!\n\n"
              "Would you like to continue?"), NULL))
        {
            exit(1);
        }

        gchar *name;
        int major, minor, micro;

        hb_presets_current_version(&major, &minor, &micro);
        name = g_strdup_printf("presets.%d.%d.%d.json", major, minor, micro);
        ghb_log("Failed to read presets file, trying backup (%s)...", name);
        if (presets_add_config_file(name) < 0)
        {
            ghb_log("Failed to read backup presets, using defaults...");
            hb_presets_builtin_update();
            // Don't store defaults unless the user explicitly saves
            // a new preset.  This would overwrite the presets file
            // that was generated by a newer version of HandBrake.
        }
        g_free(name);
    }
    else if (result < 0)
    {
        if (presets_add_config_file("presets") < 0)
        {
            ghb_log("Failed to read presets file, initializing new presets...");
            hb_presets_builtin_update();
            store_presets();
        }
    }
    ghb_update_ui_combo_box(ud, "PresetCategory", NULL, FALSE);
}

static void
settings_save(signal_user_data_t *ud, const char * category,
              const char *name, const char * desc, gboolean set_def)
{
    GhbValue          * preset, * new_preset;
    hb_preset_index_t * folder_path, * path;
    char              * fullname;

    folder_path = hb_preset_search_index(category, 0, HB_PRESET_TYPE_CUSTOM);
    if (folder_path->depth <= 0)
    {
        GhbValue * new_folder;
        new_folder = ghb_dict_new();
        ghb_dict_set_string(new_folder, "PresetName", category);
        ghb_dict_set(new_folder, "ChildrenArray", ghb_array_new());
        ghb_dict_set_int(new_folder, "Type", HB_PRESET_TYPE_CUSTOM);
        ghb_dict_set_bool(new_folder, "Folder", TRUE);
        int index = hb_preset_append(folder_path, new_folder);
        if (index >= 0)
        {
            folder_path->index[folder_path->depth++] = index;
            presets_list_append(ud, folder_path);
        }
        else
        {
            ghb_log("Failed to create category (%s)...", category);
            return;
        }
        ghb_value_free(&new_folder);
    }

    new_preset = ghb_settings_to_preset(ud->settings);
    ghb_dict_set_int(new_preset, "Type", HB_PRESET_TYPE_CUSTOM);
    ghb_dict_set_string(new_preset, "PresetName", name);
    if (desc != NULL)
        ghb_dict_set_string(new_preset, "PresetDescription", desc);

    fullname = g_strdup_printf("/%s/%s", category, name);
    path = hb_preset_search_index(fullname, 0, HB_PRESET_TYPE_CUSTOM);
    preset = hb_preset_get(path);
    if (preset != NULL)
    {
        // Replacing an existing preset
        gboolean def = ghb_dict_get_bool(preset, "Default");

        // If we are replacing the default preset, re-mark it as default
        ghb_dict_set_bool(new_preset, "Default", def);
        // Already exists, update its description
        if (hb_preset_set(path, new_preset) >= 0)
        {
            presets_list_update_item(ud, path, FALSE);
        }
    }
    else
    {
        // Check if the new preset is also the new default preset
        if (set_def)
        {
            ghb_dict_set_bool(new_preset, "Default", set_def);
            presets_list_clear_default(ud);
            hb_presets_clear_default();
        }

        // Adding a new preset
        // Append to the folder
        int index = hb_preset_append(folder_path, new_preset);
        if (index >= 0)
        {
            folder_path->index[folder_path->depth++] = index;
            presets_list_append(ud, folder_path);
        }
        *path = *folder_path;
    }


    free(fullname);
    ghb_value_free(&new_preset);

    store_presets();
    ghb_presets_menu_reinit(ud);

    ghb_set_clear_presets_inhibited(TRUE);
    // Make the new preset the selected item
    select_preset2(ud, path);
    ghb_set_clear_presets_inhibited(FALSE);

    free(folder_path);
    free(path);

    return;
}

static void
preset_import_response_cb (GtkFileChooser *chooser, GtkResponseType response,
                           signal_user_data_t *ud)
{
    const gchar     *exportDir;
    GFile           *file;
    gchar           *filename;
    gchar           *dir;
    int              index;

    file = gtk_file_chooser_get_file(chooser);
    if (response == GTK_RESPONSE_ACCEPT && file != NULL)
    {
        filename = g_file_get_path(file);
        g_object_unref(file);
        // import the preset
        index = hb_presets_add_path(filename);

        exportDir = ghb_dict_get_string(ud->prefs, "ExportDirectory");
        dir = g_path_get_dirname(filename);
        if (strcmp(dir, exportDir) != 0)
        {
            ghb_dict_set_string(ud->prefs, "ExportDirectory", dir);
            ghb_pref_save(ud->prefs, "ExportDirectory");
        }
        g_free(filename);
        g_free(dir);
        store_presets();

        // Re-init the UI preset list
        ghb_presets_list_reinit(ud);
        ghb_presets_menu_reinit(ud);
        if (index < 0)
        {
            ghb_select_default_preset(ud);
        }
        else
        {
            hb_preset_index_t path;
            path.index[0] = index;
            path.depth = 1;
            select_preset2(ud, &path);
        }
    }
    gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(chooser));
}

G_MODULE_EXPORT void
preset_import_action_cb(GSimpleAction *action, GVariant *param,
                        signal_user_data_t *ud)
{
    GtkWindow       *hb_window;
    GtkFileChooserNative *chooser;
    const gchar     *exportDir;
    GtkFileFilter   *filter;

    hb_window = GTK_WINDOW(ghb_builder_widget("hb_window"));
    chooser = gtk_file_chooser_native_new("Import Preset", hb_window,
                GTK_FILE_CHOOSER_ACTION_OPEN,
                _("_Open"),
                _("_Cancel"));

    ghb_add_file_filter(GTK_FILE_CHOOSER(chooser), _("All Files"), "FilterAll");
    filter = ghb_add_file_filter(GTK_FILE_CHOOSER(chooser), _("Presets (*.json)"), "FilterJSON");
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(chooser), filter);

    exportDir = ghb_dict_get_string(ud->prefs, "ExportDirectory");
    if (exportDir == NULL || exportDir[0] == '\0')
    {
        exportDir = ".";
    }
    ghb_file_chooser_set_initial_file(GTK_FILE_CHOOSER(chooser), exportDir);

    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(chooser), TRUE);
    gtk_native_dialog_set_transient_for(GTK_NATIVE_DIALOG(chooser), GTK_WINDOW(hb_window));
    g_signal_connect(chooser, "response", G_CALLBACK(preset_import_response_cb), ud);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser));
}

static void
preset_export_response_cb(GtkFileChooser *chooser,
                          GtkResponseType response, signal_user_data_t *ud)
{
    hb_preset_index_t *path;
    const gchar       *exportDir;
    gchar             *filename;
    GhbValue          *dict;

    if (response == GTK_RESPONSE_ACCEPT)
    {
        path = get_selected_path(ud);
        if (path == NULL || path->depth <= 0)
        {
            const gchar       *name;
            char * new_name;

            free(path);
            dict = ghb_settings_to_preset(ud->settings);
            name = ghb_dict_get_string(dict, "PresetName");
            new_name = g_strdup_printf("%s (modified)", name);
            ghb_dict_set_string(dict, "PresetName", new_name);
            free(new_name);
        }
        else
        {
            dict = hb_value_dup(hb_preset_get(path));
            free(path);
        }

        if (dict == NULL)
        {
            gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(chooser));
            return;
        }

        gchar    *dir;

        filename = ghb_file_chooser_get_filename(chooser);

        // export the preset
        hb_presets_write_json(dict, filename);
        exportDir = ghb_dict_get_string(ud->prefs, "ExportDirectory");
        dir = g_path_get_dirname(filename);
        if (strcmp(dir, exportDir) != 0)
        {
            ghb_dict_set_string(ud->prefs, "ExportDirectory", dir);
            ghb_pref_save(ud->prefs, "ExportDirectory");
        }
        g_free(dir);
        g_free(filename);
        hb_value_free(&dict);
    }
    gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(chooser));
}

G_MODULE_EXPORT void
preset_export_action_cb(GSimpleAction *action, GVariant *param,
                        signal_user_data_t *ud)
{
    hb_preset_index_t    *path;
    GtkWindow            *hb_window;
    GtkFileChooserNative *chooser;
    const gchar          *exportDir;
    gchar                *filename;
    GhbValue             *dict;
    char                 *preset_name;

    path = get_selected_path(ud);
    if (path == NULL || path->depth <= 0)
    {
        const gchar       *name;
        char * new_name;

        free(path);
        dict = ghb_settings_to_preset(ud->settings);
        name = ghb_dict_get_string(dict, "PresetName");
        new_name = g_strdup_printf("%s (modified)", name);
        ghb_dict_set_string(dict, "PresetName", new_name);
        free(new_name);
    }
    else
    {
        dict = hb_value_dup(hb_preset_get(path));
        free(path);
    }

    if (dict == NULL)
    {
        return;
    }
    preset_name = g_strdup(ghb_dict_get_string(dict, "PresetName"));

    hb_window = GTK_WINDOW(ghb_builder_widget("hb_window"));
    chooser = gtk_file_chooser_native_new(_("Export Preset"), hb_window,
                GTK_FILE_CHOOSER_ACTION_SAVE,
                _("_Save"),
                _("_Cancel"));

    exportDir = ghb_dict_get_string(ud->prefs, "ExportDirectory");
    if (exportDir == NULL || exportDir[0] == '\0')
    {
        exportDir = ".";
    }

    // Clean up preset name for use as a filename.  Removing leading
    // and trailing whitespace and filename illegal characters.
    g_strstrip(preset_name);
    g_strdelimit(preset_name, GHB_UNSAFE_FILENAME_CHARS, '_');
    filename = g_strdup_printf("%s.json", preset_name);
    ghb_file_chooser_set_initial_file(GTK_FILE_CHOOSER(chooser), exportDir);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser), filename);
    g_free(filename);
    g_free(preset_name);
    hb_value_free(&dict);

    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(chooser), TRUE);
    g_signal_connect(chooser, "response", G_CALLBACK(preset_export_response_cb), ud);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser));
}

static void
preset_rename_response_cb (GtkDialog *dialog, int response,
                           signal_user_data_t *ud)
{
    const gchar       * name;
    const gchar       * fullname;
    int                 type;
    hb_preset_index_t * path;
    GtkEditable       * entry;
    GtkTextView       * tv;
    GhbValue          * dict;

    g_signal_handlers_disconnect_by_data(dialog, ud);
    gtk_widget_set_visible(GTK_WIDGET(dialog), FALSE);

    entry    = GTK_EDITABLE(ghb_builder_widget("PresetReName"));
    tv       = GTK_TEXT_VIEW(ghb_builder_widget("PresetReDescription"));
    fullname = ghb_dict_get_string(ud->settings, "PresetFullName");
    type     = ghb_dict_get_int(ud->settings, "Type");
    path     = hb_preset_search_index(fullname, 0, type);

    if (response == GTK_RESPONSE_OK)
    {
        GtkTextBuffer * buffer;
        GtkTextIter     start, end;
        char          * desc;

        // save the new name
        name = gtk_editable_get_text(entry);
        dict = hb_preset_get(path);
        if (dict != NULL)
        {
            ghb_dict_set_string(dict, "PresetName", name);
            store_presets();
        }

        buffer = gtk_text_view_get_buffer(tv);
        gtk_text_buffer_get_bounds(buffer, &start, &end);
        desc = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        ghb_dict_set_string(ud->settings, "PresetDescription", desc);
        free(desc);

        char * full = preset_get_fullname(path, "/", FALSE);
        ghb_dict_set_string(ud->settings, "PresetFullName", full);
        ghb_dict_set_string(ud->settings, "PresetName", name);
        free(full);
        ghb_presets_menu_reinit(ud);
        set_preset_menu_button_label(ud, path);
    }
}

G_MODULE_EXPORT void
preset_rename_action_cb(GSimpleAction *action, GVariant *param,
                        signal_user_data_t *ud)
{
    const gchar       * name;
    int                 type;
    GtkWidget         * dialog;
    GtkEditable       * entry;

    name      = ghb_dict_get_string(ud->settings, "PresetName");
    type      = ghb_dict_get_int(ud->settings, "Type");

    if (type != HB_PRESET_TYPE_CUSTOM)
    {
        // Only allow renaming custom presets
        return;
    }

    ghb_ui_update("PresetReDescription",
                  ghb_dict_get_value(ud->settings, "PresetDescription"));

    dialog   = ghb_builder_widget("preset_rename_dialog");
    entry    = GTK_EDITABLE(ghb_builder_widget("PresetReName"));
    gtk_editable_set_text(entry, name);

    g_signal_connect(dialog, "response",
                     G_CALLBACK(preset_rename_response_cb), ud);
    gtk_widget_set_visible(dialog, TRUE);
}

static void
preset_save_as_response_cb (GtkDialog *dialog, int response,
                            signal_user_data_t *ud)
{
    const char *name, *category;
    GtkEditable *entry;
    GtkTextView *tv;

    g_signal_handlers_disconnect_by_data(dialog, ud);
    gtk_widget_set_visible(GTK_WIDGET(dialog), FALSE);

    entry = GTK_EDITABLE(ghb_builder_widget("PresetName"));
    tv = GTK_TEXT_VIEW(ghb_builder_widget("PresetDescription"));

    if (response == GTK_RESPONSE_OK)
    {
        GtkTextBuffer * buffer;
        GtkTextIter     start, end;
        char          * desc;
        gboolean        def;

        // save the preset
        name = gtk_editable_get_text(entry);
        category = ghb_dict_get_string(ud->settings, "PresetCategory");
        if (!g_strcmp0(category, "new"))
        {
            entry = GTK_EDITABLE(ghb_builder_widget("PresetCategoryName"));
            category = gtk_editable_get_text(entry);
        }
        if (category == NULL || category[0] == 0)
        {
            ghb_log("Invalid empty category.");
            return;
        }
        buffer = gtk_text_view_get_buffer(tv);
        gtk_text_buffer_get_bounds(buffer, &start, &end);
        desc = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        def = ghb_dict_get_bool(ud->settings, "PresetSetDefault");
        settings_save(ud, category, name, desc, def);
        free(desc);
    }
}

G_MODULE_EXPORT void
preset_save_as_action_cb (GSimpleAction *action, GVariant *param,
                          signal_user_data_t *ud)
{
    const char        * category = NULL;
    const gchar       * name;
    const gchar       * fullname;
    int                 type;
    hb_preset_index_t * path;
    GtkWidget         * dialog;
    GtkEditable       * entry;
    GhbValue          * dict;

    name      = ghb_dict_get_string(ud->settings, "PresetName");
    type      = ghb_dict_get_int(ud->settings, "Type");
    fullname  = ghb_dict_get_string(ud->settings, "PresetFullName");

    ghb_ui_update("PresetSetDefault", ghb_boolean_value(FALSE));

    path = hb_preset_search_index(fullname, 0, type);

    // Find an appropriate default category
    if (path != NULL)
    {
        path->depth = 1;
        dict        = hb_preset_get(path);
        if (ghb_dict_get_bool(dict, "Folder"))
        {
            category = ghb_dict_get_string(dict, "PresetName");
        }
        free(path);
    }
    if (category == NULL)
    {
        // Find first custom folder
        hb_value_t * presets;
        int          ii, count;

        presets = hb_presets_get();
        count   = hb_value_array_len(presets);
        for (ii = 0; ii < count; ii++)
        {
            dict = hb_value_array_get(presets, ii);
            if (!ghb_dict_get_bool(dict, "Folder"))
            {
                continue;
            }
            if (ghb_dict_get_int(dict, "Type") == HB_PRESET_TYPE_CUSTOM)
            {
                category = ghb_dict_get_string(dict, "PresetName");
                break;
            }
        }
    }
    if (category == NULL)
    {
        // Force creation of new category
        category = "new";
    }
    ghb_ui_update("PresetCategory", ghb_string_value(category));

    dialog   = ghb_builder_widget("preset_save_dialog");
    entry    = GTK_EDITABLE(ghb_builder_widget("PresetName"));
    gtk_editable_set_text(entry, name);

    g_signal_connect(dialog, "response",
                     G_CALLBACK(preset_save_as_response_cb), ud);
    gtk_widget_set_visible(dialog, TRUE);
}

G_MODULE_EXPORT void
preset_save_action_cb(GSimpleAction *action, GVariant *param,
                      signal_user_data_t *ud)
{
    const char        * category = NULL;
    const gchar       * name;
    const gchar       * fullname;
    int                 type;
    hb_preset_index_t * path;
    GhbValue          * dict;
    gboolean            def;

    name      = ghb_dict_get_string(ud->settings, "PresetName");
    type      = ghb_dict_get_int(ud->settings, "Type");
    fullname  = ghb_dict_get_string(ud->settings, "PresetFullName");

    if (type != HB_PRESET_TYPE_CUSTOM)
    {
        // Only allow saving custom presets
        return;
    }
    path = hb_preset_search_index(fullname, 0, type);

    // Find an appropriate default category
    if (path != NULL)
    {
        path->depth = 1;
        dict        = hb_preset_get(path);
        if (ghb_dict_get_bool(dict, "Folder"))
        {
            category = ghb_dict_get_string(dict, "PresetName");
        }
        free(path);
    }
    if (category == NULL)
    {
        // Only save existing presets
        return;
    }
    ghb_ui_update("PresetCategory", ghb_string_value(category));

    def = ghb_dict_get_bool(ud->settings, "PresetSetDefault");
    settings_save(ud, category, name, NULL, def);
}

static void
preset_save_set_ok_sensitive (void)
{
    GtkEditable* entry;
    GtkWidget  * ok_button;
    const char * name;
    const char * category;
    const char * category_name;
    gboolean     sensitive;
    signal_user_data_t *ud = ghb_ud();

    ok_button = ghb_builder_widget("preset_ok");

    category = ghb_dict_get_string(ud->settings, "PresetCategory");
    entry = GTK_EDITABLE(ghb_builder_widget("PresetName"));
    name = gtk_editable_get_text(entry);
    entry = GTK_EDITABLE(ghb_builder_widget("PresetCategoryName"));
    category_name = gtk_editable_get_text(entry);

    sensitive = name[0] && (strcmp(category, "new") || category_name[0]);
    gtk_widget_set_sensitive(ok_button, sensitive);
}

G_MODULE_EXPORT void
preset_category_changed_cb(GtkWidget *widget, gpointer data)
{
    signal_user_data_t *ud = ghb_ud();
    ghb_widget_to_setting(ud->settings, widget);
    preset_save_set_ok_sensitive();
}

G_MODULE_EXPORT void
preset_name_changed_cb(GtkWidget *widget, gpointer data)
{
    preset_save_set_ok_sensitive();
}

G_MODULE_EXPORT void
presets_reload_action_cb(GSimpleAction *action, GVariant *param,
                         signal_user_data_t *ud)
{
    // Reload the builtin presets
    hb_presets_builtin_update();
    store_presets();

    ghb_presets_list_reinit(ud);
    ghb_presets_menu_reinit(ud);
    ghb_select_default_preset(ud);
}

G_MODULE_EXPORT void
preset_remove_action_cb(GSimpleAction *action, GVariant *param,
                        signal_user_data_t *ud)
{
    hb_preset_index_t * path;
    GhbValue          * preset = NULL;

    path = get_selected_path(ud);
    if (path != NULL)
    {
        preset = hb_preset_get(path);
    }
    if (path == NULL || preset == NULL)
    {
        const char * fullname;
        int          type;

        fullname = ghb_dict_get_string(ud->settings, "PresetFullName");
        if (fullname == NULL)
        {
            return;
        }
        type     = ghb_dict_get_int(ud->settings, "Type");
        path = hb_preset_search_index(fullname, 0, type);
        if (path != NULL)
        {
            preset = hb_preset_get(path);
        }
    }
    if (path == NULL || preset == NULL)
    {
        return;
    }

    gboolean    is_folder;
    const char *name;

    name  = ghb_dict_get_string(preset, "PresetName");
    is_folder = preset_is_folder(path);
    if (ghb_question_dialog_run(NULL, GHB_ACTION_DESTRUCTIVE,
            _("_Delete"), _("Cancel"),
            is_folder ? _("Delete Folder?") : _("Delete Preset?"),
            _("Are you sure you want to delete “%s”?\n"
              "Deleted items cannot be recovered."), name))
    {
        int depth = path->depth;

        // Determine which preset to highlight after deletion done
        hb_preset_index_t new_path = *path;
        // Always select a preset, not a folder
        if (depth == 1)
        {
            new_path.depth = 2;
        }
        // Try next
        new_path.index[depth - 1] = path->index[depth - 1] + 1;
        preset = hb_preset_get(&new_path);
        // After deletion, index of new selected item is one less
        new_path.index[depth - 1]--;
        if (preset == NULL && path->index[depth - 1] > 0)
        {
            // Try previous
            new_path.index[depth - 1] = path->index[depth - 1] - 1;
            preset = hb_preset_get(&new_path);
        }
        if (preset == NULL)
        {
            // perhaps we are deleting the last item in a folder
            // Try first item in next and previous folders
            depth = 1;
            new_path.index[1] = 0;
            // Try next
            new_path.index[depth - 1] = path->index[depth - 1] + 1;
            preset = hb_preset_get(&new_path);
            // After deletion, index of new selected item is one less
            new_path.index[depth - 1]--;
            if (preset == NULL && path->index[depth - 1] > 0)
            {
                // Try previous
                new_path.index[depth - 1] = path->index[depth - 1] - 1;
                preset = hb_preset_get(&new_path);
            }
        }

        GtkTreeView      * treeview;
        GtkTreeSelection * selection;

        // unselect item to be deleted
        treeview  = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
        selection = gtk_tree_view_get_selection(treeview);
        gtk_tree_selection_unselect_all(selection);

        // Remove the item
        if (hb_preset_delete(path) >= 0)
        {
            presets_list_remove(ud, path);
            if (path->depth == 2)
            {
                // If deleting this item resulted in an empty folder
                // delete the folder
                int count = 0;
                hb_value_t * folder;

                path->depth = 1;
                folder = hb_presets_get_folder_children(path);
                if (folder != NULL)
                {
                    count = ghb_array_len(folder);
                }
                if (count == 0)
                {
                    // delete the folder
                    hb_preset_delete(path);
                    presets_list_remove(ud, path);
                }
            }
            store_presets();
            ghb_presets_menu_reinit(ud);
        }
        if (preset != NULL)
        {
            select_preset2(ud, &new_path);
        }
    }
    free(path);
    ghb_update_ui_combo_box(ud, "PresetCategory", NULL, FALSE);
}

G_MODULE_EXPORT gboolean
presets_drag_accept_cb (GtkDropTarget *self, GdkDrop *drop, signal_user_data_t *ud)
{
    GdkDragAction actions = gdk_drop_get_actions(drop);
    GdkContentFormats *formats = gdk_drop_get_formats(drop);
    if ((actions & GDK_ACTION_MOVE) &&
        gdk_content_formats_contain_gtype(formats, GTK_TYPE_TREE_ROW_DATA))
    {
        return TRUE;
    }
    return FALSE;
}

// controls where valid drop locations are
G_MODULE_EXPORT GdkDragAction
presets_drag_motion_cb (GtkDropTargetAsync *drop_target, GdkDrop *drop,
                        double x, double y, signal_user_data_t *ud)
{
    GtkTreeView             *tv;
    GtkTreeViewDropPosition  drop_pos;
    GtkTreeIter              iter;
    GtkTreeModel            *model;
    GtkTreeSelection        *select;
    GtkTreePath             *treepath = NULL;
    hb_preset_index_t       *path;
    gint                     src_ptype, dst_ptype;
    gboolean                 src_folder, dst_folder;
    GhbValue                *src_preset, *dst_preset;

    tv = GTK_TREE_VIEW(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(drop_target)));
    g_return_val_if_fail(GTK_IS_TREE_VIEW(tv), 0);

    // Get the type of the object being dragged
    select = gtk_tree_view_get_selection(tv);
    gtk_tree_selection_get_selected(select, &model, &iter);
    path   = tree_get_index(model, &iter);

    src_preset = hb_preset_get(path);
    free(path);
    if (src_preset == NULL)
    {
        return 0;
    }

    src_ptype  = ghb_dict_get_int(src_preset, "Type");
    src_folder = ghb_dict_get_bool(src_preset, "Folder");

    // The rest checks that the destination is a valid position
    // in the list.
    if (!gtk_tree_view_get_dest_row_at_pos(tv, (int) x, (int) y, &treepath, &drop_pos))
    {
        return 0;
    }
    // Don't allow repositioning of builtin presets
    if (src_ptype != HB_PRESET_TYPE_CUSTOM)
    {
        gtk_tree_view_set_drag_dest_row(tv, NULL, drop_pos);
        gtk_tree_path_free(treepath);
        return 0;
    }

    path = tree_path_get_index(treepath);
    dst_preset = hb_preset_get(path);
    free(path);
    if (dst_preset == NULL)
    {
        gtk_tree_path_free(treepath);
        return 0;
    }

    dst_ptype = ghb_dict_get_int(dst_preset, "Type");
    dst_folder = ghb_dict_get_bool(dst_preset, "Folder");

    // Don't allow mixing custom presets in the builtins
    if (dst_ptype != HB_PRESET_TYPE_CUSTOM)
    {
        gtk_tree_view_set_drag_dest_row(tv, NULL, drop_pos);
        gtk_tree_path_free(treepath);
        return 0;
    }

    // Don't allow dropping source folder before/after a non-folder
    if (src_folder && !dst_folder)
    {
        gtk_tree_path_up(treepath);
        path = tree_path_get_index(treepath);
        dst_preset = hb_preset_get(path);
        free(path);
        if (dst_preset == NULL)
        {
            gtk_tree_path_free(treepath);
            return 0;
        }

        dst_folder = ghb_dict_get_bool(dst_preset, "Folder");
        drop_pos = GTK_TREE_VIEW_DROP_AFTER;
    }

    // Don't allow dropping a source folder into another folder
    if ((src_folder || !dst_folder) &&
        drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
    {
        drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
    }
    if ((src_folder || !dst_folder) &&
        drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
    {
        drop_pos = GTK_TREE_VIEW_DROP_AFTER;
    }

    // Don't allow dropping a non-folder before a folder
    if (!src_folder && dst_folder && drop_pos == GTK_TREE_VIEW_DROP_BEFORE)
    {
        gtk_tree_view_set_drag_dest_row(tv, NULL, drop_pos);
        gtk_tree_path_free(treepath);
        return 0;
    }

    if (!src_folder && dst_folder && drop_pos == GTK_TREE_VIEW_DROP_AFTER)
    {
        drop_pos = GTK_TREE_VIEW_DROP_INTO_OR_AFTER;
    }

    gtk_tree_view_set_drag_dest_row(tv, treepath, drop_pos);
    gdk_drop_status(drop, GDK_ACTION_MOVE, GDK_ACTION_MOVE);

    return GDK_ACTION_MOVE;
}

G_MODULE_EXPORT gboolean
presets_drop_cb (GtkDropTargetAsync *drop_target, GdkDrop *drop, double x, double y, signal_user_data_t *ud)
{
    GtkTreeView             *tv;
    GtkTreeModel            *dst_model;
    GtkTreePath             *dst_treepath = NULL;
    GtkTreeViewDropPosition  drop_pos;
    GtkTreeIter              dst_iter, src_iter;
    gint                     src_ptype;
    gboolean                 src_folder, dst_folder;

    tv = GTK_TREE_VIEW(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(drop_target)));
    g_return_val_if_fail(GTK_IS_TREE_VIEW(tv), FALSE);

    dst_model = gtk_tree_view_get_model(tv);
    gtk_tree_view_get_drag_dest_row(tv, &dst_treepath, &drop_pos);
    if (dst_treepath == NULL)
    {
        return FALSE;
    }

    GtkTreeView       *src_widget;
    GtkTreeModel      *src_model;
    GtkTreeSelection  *select;
    hb_preset_index_t *dst_path, *src_path;

    src_widget = tv;
    select     = gtk_tree_view_get_selection (src_widget);
    gtk_tree_selection_get_selected(select, &src_model, &src_iter);

    src_path   = tree_get_index(src_model, &src_iter);
    src_ptype  = preset_get_type(src_path);
    src_folder = preset_is_folder(src_path);

    // Don't allow repositioning of builtin presets
    if (src_ptype != HB_PRESET_TYPE_CUSTOM)
    {
        free(src_path);
        gtk_tree_path_free(dst_treepath);
        dst_treepath = NULL;
        return FALSE;
    }

    dst_path = tree_path_get_index(dst_treepath);
    dst_folder = preset_is_folder(dst_path);

    // Don't allow allow dropping source folders after/before non-folders
    if (src_folder && !dst_folder)
    {
        gtk_tree_path_up(dst_treepath);
        dst_path = tree_path_get_index(dst_treepath);
        dst_folder = preset_is_folder(dst_path);

        drop_pos = GTK_TREE_VIEW_DROP_AFTER;
    }

    // Don't allow dropping folders into other folders, only before/after
    if (src_folder && dst_folder)
    {
        if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
            drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
        else if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
            drop_pos = GTK_TREE_VIEW_DROP_AFTER;
    }

    if (!src_folder && dst_folder)
    {
        // Don't allow dropping preset before a folder
        if (drop_pos == GTK_TREE_VIEW_DROP_BEFORE)
        {
            free(src_path);
            free(dst_path);
            gtk_tree_path_free(dst_treepath);
            return FALSE;
        }
        // Don't allow dropping preset after a folder, only into
        if (drop_pos == GTK_TREE_VIEW_DROP_AFTER)
            drop_pos = GTK_TREE_VIEW_DROP_INTO_OR_AFTER;
    }

    // Don't allow dropping into non-folder items
    if (!dst_folder)
    {
        if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
            drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
        else if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
            drop_pos = GTK_TREE_VIEW_DROP_AFTER;
    }
    free(dst_path);
    if (gtk_tree_model_get_iter(dst_model, &dst_iter, dst_treepath))
    {
        GtkTreeIter iter;

        // Insert new empty row in UI preset list
        // This logic determines the final position of the preset,
        // i.e. before, after or inside the target entry.
        // So the dst_path to move the preset to must be computed
        // after moving the entry in the UI list
        switch (drop_pos)
        {
            case GTK_TREE_VIEW_DROP_BEFORE:
                gtk_tree_store_insert_before(GTK_TREE_STORE(dst_model),
                                            &iter, NULL, &dst_iter);
                break;

            case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
                gtk_tree_store_insert(GTK_TREE_STORE(dst_model),
                                            &iter, &dst_iter, 0);
                break;

            case GTK_TREE_VIEW_DROP_AFTER:
                gtk_tree_store_insert_after(GTK_TREE_STORE(dst_model),
                                            &iter, NULL, &dst_iter);
                break;

            case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
                gtk_tree_store_insert_after(GTK_TREE_STORE(dst_model),
                                            &iter, &dst_iter, NULL);
                break;

            default:
                break;
        }

        // Move source preset at the desired location
        dst_path = tree_get_index(dst_model, &iter);
        hb_preset_move(src_path, dst_path);
        free(dst_path);

        // Remove the old entry in the UI list
        gtk_tree_store_remove(GTK_TREE_STORE(src_model), &src_iter);

        // UI elements were shuffled again.  recompute dst_path
        dst_path = tree_get_index(dst_model, &iter);
        presets_list_update_item(ud, dst_path, TRUE);
        select_preset2(ud, dst_path);
        free(dst_path);

        store_presets();
        ghb_presets_menu_reinit(ud);
    }
    gtk_tree_path_free(dst_treepath);
    free(src_path);
    gdk_drop_finish(drop, GDK_ACTION_MOVE);
    return TRUE;
}

void
presets_row_expanded_cb(
    GtkTreeView        *treeview,
    GtkTreeIter        *iter,
    GtkTreePath        *treepath,
    signal_user_data_t *ud)
{
    hb_preset_index_t *path;
    gboolean           expanded;
    GhbValue          *dict;

    expanded = gtk_tree_view_row_expanded(treeview, treepath);
    path     = tree_path_get_index(treepath);
    dict     = hb_preset_get(path);
    free(path);

    // Sanity check
    if (!ghb_dict_get_bool(dict, "Folder"))
    {
        g_warning("presets_row_expand_cb: Desync between presets and list");
        return;
    }
    if (ghb_dict_get_bool(dict, "FolderOpen"))
    {
        if (expanded)
        {
            return;
        }
    }
    else if (!expanded)
    {
        return;
    }
    ghb_dict_set_bool(dict, "FolderOpen", expanded);
    store_presets();
}

// Makes a copy of the preset and assigns "PresetFullName" which
// is use to look up the preset in the preset list should the need
// arrise, e.g. saving changes to the preset.
GhbValue*
ghb_get_current_preset(signal_user_data_t *ud)
{
    GtkTreeView      *tv;
    GtkTreeModel     *tm;
    GtkTreeSelection *ts;
    GtkTreeIter       ti;
    GhbValue         *preset = NULL;

    tv = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
    ts = gtk_tree_view_get_selection(tv);
    if (gtk_tree_selection_get_selected(ts, &tm, &ti))
    {
        hb_preset_index_t *path;

        path   = tree_get_index(tm, &ti);
        preset = hb_preset_get(path);
        if (preset != NULL)
        {
            char *fullname;

            preset   = hb_value_dup(preset);
            fullname = preset_get_fullname(path, "/", FALSE);
            ghb_dict_set_string(preset, "PresetFullName", fullname);
            free(fullname);
        }
        free(path);
    }
    return preset;
}

G_MODULE_EXPORT void
presets_list_selection_changed_cb(GtkTreeSelection *selection, signal_user_data_t *ud)
{
    hb_preset_index_t * path;

    path   = get_selected_path(ud);
    if (path != NULL)
    {
        GhbValue *dict = hb_preset_get(path);
        if (!ghb_dict_get_bool(dict, "Folder") &&
            !ghb_dict_get_bool(ud->settings, "preset_reload"))
        {
            ghb_preset_to_settings(ud->settings, dict);
            char *fullname = preset_get_fullname(path, "/", FALSE);
            ghb_dict_set_string(ud->settings, "PresetFullName", fullname);
            free(fullname);
            ghb_set_current_title_settings(ud);
            ghb_load_post_settings(ud);
        }
        if (!ghb_dict_get_bool(dict, "Folder"))
        {
            GSimpleAction * action;
            GtkWidget     * widget;

            set_preset_menu_button_label(ud, path);
            action = GHB_APPLICATION_ACTION("preset-reload");
            g_simple_action_set_enabled(action, FALSE);
            widget = ghb_builder_widget("preset_selection_modified_label");
            gtk_widget_set_visible(widget, FALSE);
            widget = ghb_builder_widget("preset_selection_reload");
            gtk_widget_set_visible(widget, FALSE);
            widget = ghb_builder_widget("preset_save_new");
            gtk_widget_set_visible(widget, FALSE);
        }
        int type = preset_get_type(path);
        GSimpleAction * action;

        action = GHB_APPLICATION_ACTION("preset-rename");
        g_simple_action_set_enabled(action, type == HB_PRESET_TYPE_CUSTOM);
        action = GHB_APPLICATION_ACTION("preset-save");
        g_simple_action_set_enabled(action, type == HB_PRESET_TYPE_CUSTOM);
        action = GHB_APPLICATION_ACTION("preset-default");
        g_simple_action_set_enabled(action, !preset_is_default(path));
        free(path);
    }
}

void
ghb_clear_presets_selection(signal_user_data_t *ud)
{
    GtkTreeView      * treeview;
    GtkTreeSelection * selection;
    GSimpleAction    * action;
    GtkWidget        * widget;

    if (dont_clear_presets) return;
    treeview  = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
    selection = gtk_tree_view_get_selection(treeview);
    gtk_tree_selection_unselect_all(selection);
    ghb_dict_set_bool(ud->settings, "preset_modified", TRUE);


    action = GHB_APPLICATION_ACTION("preset-reload");
    g_simple_action_set_enabled(action, TRUE);
    widget = ghb_builder_widget("preset_selection_modified_label");
    gtk_widget_set_visible(widget, TRUE);
    widget = ghb_builder_widget("preset_selection_reload");
    gtk_widget_set_visible(widget, TRUE);
    widget = ghb_builder_widget("preset_save_new");
    gtk_widget_set_visible(widget, TRUE);
}

G_MODULE_EXPORT void
preset_default_action_cb(GSimpleAction *action, GVariant *param,
                         signal_user_data_t *ud)
{
    hb_preset_index_t *path = get_selected_path(ud);
    if (path != NULL)
    {
        hb_value_t *dict = hb_preset_get(path);
        if (dict != NULL && !ghb_dict_get_bool(dict, "Folder"))
        {
            presets_list_clear_default(ud);
            hb_presets_clear_default();
            ghb_dict_set_bool(dict, "Default", 1);
            presets_list_show_default(ud);
            store_presets();
            g_simple_action_set_enabled(action, FALSE);
        }
        g_free(path);
    }
}

G_MODULE_EXPORT void
preset_edited_cb(
    GtkCellRendererText *cell,
    gchar               *treepath_s,
    gchar               *text,
    signal_user_data_t  *ud)
{
    GtkTreePath       *treepath;
    GtkTreeStore      *store;
    GtkTreeView       *treeview;
    GtkTreeIter        iter;
    GhbValue          *dict;
    hb_preset_index_t *path;

    treeview = GTK_TREE_VIEW(ghb_builder_widget("presets_list"));
    store    = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
    treepath = gtk_tree_path_new_from_string(treepath_s);
    path     = tree_path_get_index(treepath);
    if (path != NULL)
    {
        dict = hb_preset_get(path);
        if (dict != NULL)
        {
            gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath);
            gtk_tree_store_set(store, &iter, 0, text, -1);

            ghb_dict_set_string(dict, "PresetName", text);
            store_presets();
        }
    }
    gtk_tree_path_free(treepath);
    free(path);
}

G_MODULE_EXPORT void
preset_widget_changed_cb (GtkWidget *widget, gpointer data)
{
    signal_user_data_t *ud = ghb_ud();
    ghb_widget_to_setting(ud->settings, widget);
}

void
ghb_set_clear_presets_inhibited (gboolean inhibited)
{
    dont_clear_presets = inhibited;
}
