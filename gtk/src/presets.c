/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * presets.c
 * Copyright (C) John Stebbins 2008-2015 <stebbins@stebbins>
 *
 * presets.c is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <string.h>
#include "ghbcompat.h"
#include "hb.h"
#include "settings.h"
#include "callbacks.h"
#include "audiohandler.h"
#include "subtitlehandler.h"
#include "hb-backend.h"
#include "resources.h"
#include "presets.h"
#include "values.h"
#include "lang.h"
#include "videohandler.h"

#define MAX_NESTED_PRESET 3

enum
{
    PRESETS_INVALID = -1,
    PRESETS_BUILTIN = 0,
    PRESETS_CUSTOM
};

static GhbValue *prefsDict = NULL;
static gboolean prefs_modified = FALSE;

static void store_prefs(void);
static void store_presets(void);

hb_preset_index_t*
ghb_tree_get_index(GtkTreeModel *store, GtkTreeIter *iter)
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

hb_preset_index_t*
ghb_tree_path_get_index(GtkTreePath *treepath)
{
    int *indices, len;

    indices = gtk_tree_path_get_indices(treepath);
    len     = gtk_tree_path_get_depth(treepath);

    return hb_preset_index_init(indices, len);
}

// This only handle limited depth
GtkTreePath*
ghb_tree_path_new_from_index(const hb_preset_index_t *path)
{
    if (path == NULL || path->depth == 0)
        return NULL;

#if GTK_CHECK_VERSION(3, 12, 0)
    return gtk_tree_path_new_from_indicesv((int*)path->index, path->depth);
#else
    switch (path->depth)
    {
        case 1:
            return gtk_tree_path_new_from_indices(
                path->index[0], -1);
        case 2:
            return gtk_tree_path_new_from_indices(
                path->index[0], path->index[1], -1);
        case 3:
            return gtk_tree_path_new_from_indices(
                path->index[0], path->index[1], path->index[2], -1);
        case 4:
            return gtk_tree_path_new_from_indices(
                path->index[0], path->index[1], path->index[2],
                path->index[3], -1);
        case 5:
            return gtk_tree_path_new_from_indices(
                path->index[0], path->index[1], path->index[2],
                path->index[3], path->index[4], -1);
        case 6:
            return gtk_tree_path_new_from_indices(
                path->index[0], path->index[1], path->index[2],
                path->index[3], path->index[4], path->index[5], -1);
        case 7:
            return gtk_tree_path_new_from_indices(
                path->index[0], path->index[1], path->index[2],
                path->index[3], path->index[4], path->index[5],
                path->index[6], -1);
        case 8:
            return gtk_tree_path_new_from_indices(
                path->index[0], path->index[1], path->index[2],
                path->index[3], path->index[4], path->index[5],
                path->index[6], path->index[7], -1);
        default:
            g_warning("Preset path depth too deep");
            return NULL;
    }
#endif
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

static gint
preset_tree_depth(GhbValue *dict)
{
    if (ghb_dict_get_bool(dict, "Folder"))
    {
        gint      depth = 0;
        gint      count, ii;
        GhbValue *presets;

        presets = ghb_dict_get(dict, "ChildrenArray");
        count   = ghb_array_len(presets);
        for (ii = 0; ii < count; ii++)
        {
            gint tmp;

            dict  = ghb_array_get(presets, ii);
            tmp   = preset_tree_depth(dict);
            depth = MAX(depth, tmp);
        }
        return depth + 1;
    }
    else
    {
        return 1;
    }
}

void
ghb_presets_list_show_default(signal_user_data_t *ud)
{
    hb_preset_index_t *path;

    path = hb_presets_get_default_index();
    if (path == NULL || path->depth == 0)
        return;

    GtkTreeView  *treeview;
    GtkTreeStore *store;
    GtkTreePath  *treepath;
    GtkTreeIter   iter;

    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
    store    = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
    treepath = ghb_tree_path_new_from_index(path);
    if (treepath)
    {
        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath))
        {
            gtk_tree_store_set(store, &iter,
                        1, 800,
                        2, 2 ,
                        -1);
        }
        gtk_tree_path_free(treepath);
    }
    free(path);
}

void
ghb_presets_list_clear_default(signal_user_data_t *ud)
{
    hb_preset_index_t *path;

    path = hb_presets_get_default_index();
    if (path == NULL || path->depth == 0)
        return;

    GtkTreeView  *treeview;
    GtkTreeStore *store;
    GtkTreePath  *treepath;
    GtkTreeIter   iter;

    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
    store    = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
    treepath = ghb_tree_path_new_from_index(path);
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

void
ghb_preset_to_settings(GhbValue *settings, GhbValue *preset)
{
    // Remove troublesome x264Option
    ghb_dict_remove(settings, "x264Option");

    // Initialize defaults
    ghb_settings_init(settings, "Initialization");

    // Initialize the ui settings from a preset
    ghb_dict_copy(settings, preset);

    // Fix up all the internal settings that are derived from preset values.

    int width, height;
    width = ghb_dict_get_int(settings, "PictureWidth");
    height = ghb_dict_get_int(settings, "PictureHeight");

    ghb_dict_set(settings, "scale_height", ghb_value_dup(
        ghb_dict_get_value(settings, "PictureHeight")));

    ghb_dict_set(settings, "scale_width", ghb_value_dup(
        ghb_dict_get_value(settings, "PictureWidth")));

    gint uses_pic;
    gint vqtype;

    uses_pic = ghb_dict_get_int(settings, "UsesPictureSettings");
    vqtype = ghb_dict_get_int(settings, "VideoQualityType");

    ghb_dict_set_bool(settings, "autoscale",
                      uses_pic == 2 || (width == 0 && height == 0));

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
    if (strcmp(mode, "cfr") == 0)
    {
        ghb_dict_set_bool(settings, "VideoFramerateCFR", TRUE);
        ghb_dict_set_bool(settings, "VideoFrameratePFR", FALSE);
        ghb_dict_set_bool(settings, "VideoFramerateVFR", FALSE);
    }
    else if (strcmp(mode, "pfr") == 0)
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

    if (ghb_dict_get_bool(settings, "x264UseAdvancedOptions"))
    {
        // Force preset/tune/profile/level/opts to conform to option string
        ghb_dict_set_string(settings, "VideoPreset", "medium");
        ghb_dict_set_string(settings, "VideoTune", "none");
        ghb_dict_set_string(settings, "VideoProfile", "auto");
        ghb_dict_set_string(settings, "VideoLevel", "auto");
        ghb_dict_set(settings, "VideoOptionExtra", ghb_value_dup(
            ghb_dict_get_value(settings, "x264Option")));
    }
    else
    {
        ghb_dict_remove(settings, "x264Option");
    }

    int                 ii, encoder;
    const char * const *videoPresets;
    const char         *videoPreset;

    encoder      = ghb_get_video_encoder(settings);
    videoPresets = hb_video_encoder_get_presets(encoder);
    videoPreset  = ghb_dict_get_string(settings, "VideoPreset");
    for (ii = 0; videoPreset && videoPresets && videoPresets[ii]; ii++)
    {
        if (!strcasecmp(videoPreset, videoPresets[ii]))
        {
            ghb_dict_set_int(settings, "VideoPresetSlider", ii);
            break;
        }
    }
    if (videoPreset != NULL)
        ghb_dict_set_string(settings, "VideoPreset", videoPreset);

    char *videoTune;
    char *tune = NULL;
    char *saveptr;
    char *tok;

    videoTune = g_strdup(ghb_dict_get_string(settings, "VideoTune"));
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
            }
        }
    }
}

// Initialization order of some widgets matter because the value of
// these widgets are used to establich limits on the values that
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
    // middle (void*) cast prevents gcc warning "defreferencing type-punned
    // pointer will break strict-aliasing rules"
    while (ghb_dict_iter_next(tmp, &iter, &key, &gval))
    {
        ghb_ui_settings_update(ud, dict, key, gval);
    }
    ghb_value_free(&tmp);
}

static void
select_preset2(GtkBuilder *builder, hb_preset_index_t *path)
{
    GtkTreeView      *treeview;
    GtkTreeSelection *selection;
    GtkTreeModel     *store;
    GtkTreeIter       iter;
    GtkTreePath      *treepath;

    if (path == NULL || path->depth == 0)
        return;

    treeview  = GTK_TREE_VIEW(GHB_WIDGET(builder, "presets_list"));
    selection = gtk_tree_view_get_selection (treeview);
    store     = gtk_tree_view_get_model (treeview);
    treepath  = ghb_tree_path_new_from_index(path);
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
}

void
ghb_select_preset(GtkBuilder *builder, const char *name)
{
    hb_preset_index_t *path;

    path = hb_preset_search_index(name, 1);
    if (path != NULL)
    {
        select_preset2(builder, path);
        free(path);
    }
}

void
ghb_select_default_preset(GtkBuilder *builder)
{
    hb_preset_index_t *path;

    path = hb_presets_get_default_index();
    if (path != NULL)
    {
        select_preset2(builder, path);
        g_free(path);
    }
}

gchar*
ghb_get_user_config_dir(gchar *subdir)
{
    const gchar *dir;
    gchar       *config;

    dir = g_get_user_config_dir();
    if (!g_file_test(dir, G_FILE_TEST_IS_DIR))
    {
        dir    = g_get_home_dir();
        config = g_strdup_printf ("%s/.ghb", dir);
        if (!g_file_test(config, G_FILE_TEST_IS_DIR))
            g_mkdir (config, 0755);
    }
    else
    {
        config = g_strdup_printf ("%s/ghb", dir);
        if (!g_file_test(config, G_FILE_TEST_IS_DIR))
            g_mkdir (config, 0755);
    }
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
        if (major > hb_major)
        {
            // Change in major indicates non-backward compatible preset changes.
            // We can't successfully load presets that were generated by
            // a newer version of handbrake than is currently running.
            hb_value_free(&preset);
            return -2;
        }

        hb_value_t *backup = hb_value_dup(preset);
        int result = hb_presets_import(preset);
        if (result)
        {
            // hb_presets_import modified the preset.  So make a backup
            // of the original.
            config  = ghb_get_user_config_dir(NULL);
            path    = g_strdup_printf ("%s/presets.%d.%d.%d.json",
                            config, major, minor, micro);
            hb_value_write_json(backup, path);
            g_free(config);
            g_free(path);
        }
        hb_value_free(&backup);

        hb_presets_add(preset);
        if (major != hb_major || minor != hb_minor || micro != hb_micro)
        {
            // Reload hb builtin presets
            hb_presets_builtin_update();
            store_presets();
        }
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
        if (gval == NULL)
            gval = hb_plist_parse_file(path);
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
        if (gval == NULL)
            gval = hb_plist_parse_file(path);
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
ghb_write_pid_file()
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

    fprintf(fp, "%d\n", pid);
    fclose(fp);

    fd = open(path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    lockf(fd, F_TLOCK, 0);

    g_free(config);
    g_free(path);
#endif
}

int
ghb_find_pid_file()
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
                pid_t  my_pid;
                int    pid;

                sscanf(file, "ghb.pid.%d", &pid);
                my_pid = getpid();
                if (my_pid == pid)
                {
                    file = g_dir_read_name(gdir);
                    continue;
                }
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
ghb_settings_close()
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

        const gchar *dir = g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
        if (dir == NULL)
        {
            dir = ".";
        }
        ghb_dict_set_string(dict, "ExportDirectory", dir);

        dir = g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS);
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

static const gchar*
get_preset_color(gint type, gboolean is_folder)
{
    const gchar *color;

    if (type == PRESETS_CUSTOM)
    {
        color = "DimGray";
        if (is_folder)
        {
            color = "black";
        }
    }
    else
    {
        color = "blue";
        if (is_folder)
        {
            color = "Navy";
        }
    }
    return color;
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
    treeview    = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
    store       = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
    parent_path = ghb_tree_path_new_from_index(path);
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
        const gchar *color;
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
        color       = get_preset_color(type, is_folder);

        gtk_tree_store_append(store, &iter, piter);
        gtk_tree_store_set(store, &iter,
                            0, name,
                            1, def ? 800 : 400,
                            2, def ? 2   : 0,
                            3, color,
                            4, description,
                            5, type == PRESETS_BUILTIN ? 0 : 1,
                            -1);
        if (is_folder)
        {
            ghb_presets_list_init(ud, next_path);
            if (ghb_dict_get_bool(dict, "FolderOpen"))
            {
                GtkTreePath *path;
                path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
                gtk_tree_view_expand_to_path(treeview, path);
                gtk_tree_path_free(path);
            }
        }
    }
    g_free(next_path);
    if (path == NULL)
    {
        ghb_presets_list_show_default(ud);
    }
}

static void
presets_list_clear(signal_user_data_t *ud)
{
    GtkTreeView  *treeview;
    GtkTreeModel *store;
    GtkTreeIter   iter;
    gboolean      valid;

    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
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
    const gchar  *color;

    dict = hb_preset_get(path);
    if (dict == NULL)
        return;

    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
    store    = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
    treepath = ghb_tree_path_new_from_index(path);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, treepath);

    // Additional settings, add row
    name        = ghb_dict_get_string(dict, "PresetName");
    description = ghb_dict_get_string(dict, "PresetDescription");
    type        = ghb_dict_get_int(dict, "Type");
    is_folder   = ghb_dict_get_bool(dict, "Folder");
    def         = ghb_dict_get_bool(dict, "Default");
    color       = get_preset_color(type, is_folder);

    gtk_tree_store_set(store, &iter,
                        0, name,
                        1, def ? 800 : 400,
                        2, def ? 2   : 0,
                        3, color,
                        4, description,
                        5, type == PRESETS_BUILTIN ? 0 : 1,
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
    const gchar       *color;

    folder_path = hb_preset_index_dup(path);
    folder_path->depth--;

    dict = hb_preset_get(path);
    if (dict == NULL)
    {
        g_message("Ack! Desync between presets and preset list");
        return;
    }

    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
    store    = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));

    folder_treepath = ghb_tree_path_new_from_index(folder_path);
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
    color       = get_preset_color(type, is_folder);

    gtk_tree_store_append(store, &iter, piter);
    gtk_tree_store_set(store, &iter,
                        0, name,
                        1, def ? 800 : 400,
                        2, def ? 2   : 0,
                        3, color,
                        4, description,
                        5, type == PRESETS_BUILTIN ? 0 : 1,
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

    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
    store    = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
    treepath = ghb_tree_path_new_from_index(path);
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
    name = g_strdup_printf ("queue.%d", pid);
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
    return copy_mask;
}

// Translate internal values to preset key, value pairs
GhbValue*
ghb_settings_to_preset(GhbValue *settings)
{
    GhbValue *preset = ghb_value_dup(settings);

    gboolean autoscale, br, constant;

    ghb_dict_set_bool(preset, "Default", 0);
    ghb_dict_set_int(preset, "Type", PRESETS_CUSTOM);
    if (!ghb_dict_get_bool(preset, "PictureWidthEnable"))
    {
        ghb_dict_remove(preset, "PictureWidth");
    }
    if (!ghb_dict_get_bool(preset, "PictureHeightEnable"))
    {
        ghb_dict_remove(preset, "PictureHeight");
    }
    autoscale = !ghb_dict_get_bool(preset, "PictureWidthEnable") &&
                !ghb_dict_get_bool(preset, "PictureHeightEnable");

    br = ghb_dict_get_bool(preset, "vquality_type_bitrate");
    constant = ghb_dict_get_bool(preset, "vquality_type_constant");
    if (autoscale)
    {
        ghb_dict_set_int(preset, "UsesPictureSettings", 2);
    }
    else
    {
        ghb_dict_set_int(preset, "UsesPictureSettings", 1);
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

    if (ghb_dict_get_int(preset, "PictureDeblock") < 5)
    {
        ghb_dict_set_int(preset, "PictureDeblock", 0);
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

    if (ghb_dict_get_bool(preset, "x264UseAdvancedOptions"))
    {
        ghb_dict_remove(preset, "VideoPreset");
        ghb_dict_remove(preset, "VideoTune");
        ghb_dict_remove(preset, "VideoProfile");
        ghb_dict_remove(preset, "VideoLevel");
        ghb_dict_remove(preset, "VideoOptionExtra");
    }

    GString *str = g_string_new("");
    const char *sep = "";
    const char *tune = ghb_dict_get_string(preset, "VideoTune");
    if (tune != NULL && strcasecmp(tune, "none"))
    {
        g_string_append_printf(str, "%s", tune);
        sep = ",";
    }
    if (ghb_dict_get_bool(preset, "x264FastDecode"))
    {
        g_string_append_printf(str, "%s%s", sep, "fastdecode");
        sep = ",";
    }
    if (ghb_dict_get_bool(preset, "x264ZeroLatency"))
    {
        g_string_append_printf(str, "%s%s", sep, "zerolatency");
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
store_presets()
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
    prefs_timeout_id = g_timeout_add_seconds(1, (GSourceFunc)delayed_store_prefs, NULL);
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
        GtkWindow *hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
        gchar *message = g_strdup_printf(
            _("Presets found are newer than what is supported by this version of HandBrake!\n\n"
              "Would you like to continue?"));
        if (!ghb_message_dialog(hb_window, GTK_MESSAGE_WARNING, message,
            _("Get me out of here!"), _("Load backup presets")))
        {
            g_free(message);
            exit(1);
        }
        g_free(message);

        gchar *name;
        int major, minor, micro;

        hb_presets_current_version(&major, &minor, &micro);
        name = g_strdup_printf("presets.%d.%d.%d.json", major, minor, micro);
        ghb_log("Failed to read presets file, trying backup (%s)...", name);
        if (presets_add_config_file(name) < 0)
        {
            ghb_log("Failed to read backup presets, using defaults...");
            hb_presets_builtin_update();
            // Don't store defaults unless the user explicitely saves
            // a new preset.  This would overwrite the presets file
            // that was generated by a newer version of HandBrake.
        }
        g_free(name);
    }
    else if (result < 0)
    {
        if (presets_add_config_file("presets") < 0)
        {
            ghb_log("Failed to read presets file, initailizing new presets...");
            hb_presets_builtin_update();
            store_presets();
        }
    }
}

static void
settings_save(signal_user_data_t *ud, hb_preset_index_t *path, const char *name)
{
    GhbValue *dict;
    gboolean  replace = FALSE;

    dict = hb_preset_get(path);
    if (dict != NULL)
    {
        gboolean    is_folder;
        int         type;
        const char *s;

        is_folder = ghb_dict_get_bool(dict, "Folder");
        type      = ghb_dict_get_int(dict, "Type");
        s         = ghb_dict_get_string(dict, "PresetName");
        if (s == NULL || strcmp(s, name) || type != PRESETS_CUSTOM || is_folder)
        {
            // Name changed or original preset was builtin or original
            // was a folder. Don't replace it.
            replace = FALSE;
            path->depth--;
            if (type != PRESETS_CUSTOM)
            {
                // Don't put new custom presets in a builtin folder
                path->depth = 0;
            }
        }
        else
        {
            replace = TRUE;
        }
    }
    char * new_name = strdup(name);
    if (!replace)
    {
        // We are creating a new preset.  Make sure there is not
        // another preset in this folder that has the same name
        int       ii, count, index = 1;
        GhbValue *children;

        children = hb_presets_get_folder_children(path);
        count   = ghb_array_len(children);
        do
        {
            for (ii = 0; ii < count; ii++)
            {
                GhbValue   *preset;
                const char *s;

                preset = ghb_array_get(children, ii);
                s      = ghb_dict_get_string(preset, "PresetName");
                if (s != NULL && !strcmp(s, new_name))
                {
                    free(new_name);
                    new_name = g_strdup_printf("%s (%d)", name, index++);
                    break;
                }
            }
        } while (ii < count);
    }
    dict = ghb_settings_to_preset(ud->settings);
    ghb_dict_set_string(dict, "PresetName", new_name);
    free(new_name);
    if (replace)
    {
        // Already exists, update its description
        if (hb_preset_set(path, dict) >= 0)
        {
            presets_list_update_item(ud, path, FALSE);
        }
    }
    else
    {
        // Append to the folder list the source came from
        int index = hb_preset_append(path, dict);
        if (index >= 0)
        {
            path->index[path->depth++] = index;
            presets_list_append(ud, path);
        }
    }
    ghb_value_free(&dict);
    store_presets();

    ud->dont_clear_presets = TRUE;
    // Make the new preset the selected item
    select_preset2(ud->builder, path);
    ud->dont_clear_presets = FALSE;
    return;
}

static void
folder_save(signal_user_data_t *ud, hb_preset_index_t *path, const char *name)
{
    GhbValue *dict;

    dict = hb_preset_get(path);
    if (dict != NULL)
    {
        gboolean    is_folder;
        int         type;
        const char *s;

        is_folder = ghb_dict_get_bool(dict, "Folder");
        type      = ghb_dict_get_int(dict, "Type");
        s         = ghb_dict_get_string(dict, "PresetName");
        if (s == NULL || strcmp(s, name) || type != PRESETS_CUSTOM || !is_folder)
        {
            // Name changed or original preset was builtin or original
            // was a not a folder. Don't replace it.
            dict = NULL;
            path->depth--;
            if (type != PRESETS_CUSTOM)
            {
                // Don't put new custom presets in a builtin folder
                path->depth = 1;
            }
        }
    }
    if (dict != NULL)
    {
        // Already exists, update its description
        dict = hb_preset_get(path);
        ghb_dict_set(dict, "PresetDescription",
            ghb_value_dup(ghb_dict_get(ud->settings, "PresetDescription")));
        presets_list_update_item(ud, path, FALSE);
    }
    else
    {
        dict = ghb_dict_new();
        ghb_dict_set(dict, "PresetDescription",
            ghb_value_dup(ghb_dict_get(ud->settings, "PresetDescription")));
        ghb_dict_set_string(dict, "PresetName", name);
        ghb_dict_set(dict, "ChildrenArray", ghb_array_new());
        ghb_dict_set_int(dict, "Type", PRESETS_CUSTOM);
        ghb_dict_set_bool(dict, "Folder", TRUE);
        int index = hb_preset_append(path, dict);
        if (index >= 0)
        {
            path->index[path->depth++] = index;
            presets_list_append(ud, path);
        }
        ghb_value_free(&dict);
    }
    store_presets();

    return;
}

G_MODULE_EXPORT void
preset_import_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    GtkWindow       *hb_window;
    GtkWidget       *dialog;
    GtkResponseType  response;
    const gchar     *exportDir;
    gchar           *filename;
    GtkFileFilter   *filter;

    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    dialog = gtk_file_chooser_dialog_new("Import Preset", hb_window,
                GTK_FILE_CHOOSER_ACTION_OPEN,
                GHB_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                GHB_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                NULL);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("All (*)"));
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("Presets (*.json)"));
    gtk_file_filter_add_pattern(filter, "*.json");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("Legacy Presets (*.plist)"));
    gtk_file_filter_add_pattern(filter, "*.plist");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    exportDir = ghb_dict_get_string(ud->prefs, "ExportDirectory");
    if (exportDir == NULL || exportDir[0] == '\0')
    {
        exportDir = ".";
    }
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), exportDir);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    if (response == GTK_RESPONSE_ACCEPT)
    {
        gchar    *dir;
        int       index;

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR))
        {
            gtk_widget_destroy(dialog);
            g_free(filename);
            return;
        }
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
        if (index < 0)
        {
            ghb_select_default_preset(ud->builder);
        }
        else
        {
            hb_preset_index_t path;
            path.index[0] = index;
            path.depth = 1;
            select_preset2(ud->builder, &path);
        }
    }
    gtk_widget_destroy(dialog);
}

hb_preset_index_t *
get_selected_path(signal_user_data_t *ud)
{
    GtkTreeView      *treeview;
    GtkTreeSelection *selection;
    GtkTreeModel     *store;
    GtkTreeIter       iter;

    treeview  = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
    selection = gtk_tree_view_get_selection(treeview);
    if (gtk_tree_selection_get_selected(selection, &store, &iter))
    {
        return ghb_tree_get_index(store, &iter);
    }
    return NULL;
}

G_MODULE_EXPORT void
preset_export_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    hb_preset_index_t *path;
    const gchar       *name;
    GtkWindow         *hb_window;
    GtkWidget         *dialog;
    GtkResponseType    response;
    const gchar       *exportDir;
    gchar             *filename;

    path = get_selected_path(ud);
    if (path == NULL || path->depth <= 0)
    {
        free(path);
        return;
    }

    GhbValue *dict = hb_preset_get(path);
    free(path);
    if (dict == NULL)
    {
        return;
    }
    name = ghb_dict_get_string(dict, "PresetName");

    hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
    dialog = gtk_file_chooser_dialog_new(_("Export Preset"), hb_window,
                GTK_FILE_CHOOSER_ACTION_SAVE,
                GHB_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                GHB_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                NULL);

    exportDir = ghb_dict_get_string(ud->prefs, "ExportDirectory");
    if (exportDir == NULL || exportDir[0] == '\0')
    {
        exportDir = ".";
    }
    filename = g_strdup_printf("%s.json", name);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), exportDir);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), filename);
    g_free(filename);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    if (response == GTK_RESPONSE_ACCEPT)
    {
        gchar    *dir;

        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

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
    }
    gtk_widget_destroy(dialog);
}

G_MODULE_EXPORT void
presets_new_folder_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    GtkWidget         *dialog;
    GtkEntry          *entry;
    GtkTextView       *desc;
    GtkResponseType    response;
    hb_preset_index_t *path;
    const gchar       *name;
    const gchar       *description;

    path        = get_selected_path(ud);
    name        = ghb_dict_get_string(ud->settings, "PresetName");
    description = ghb_dict_get_string(ud->settings, "PresetDescription");
    ghb_ui_update(ud, "FolderDescription", ghb_string_value(description));

    desc   = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "FolderDescription"));
    dialog = GHB_WIDGET(ud->builder, "preset_new_folder_dialog");
    entry  = GTK_ENTRY(GHB_WIDGET(ud->builder, "FolderName"));
    gtk_entry_set_text(entry, name);
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    if (response == GTK_RESPONSE_OK)
    {
        // save the preset
        const gchar *name = gtk_entry_get_text(entry);
        GhbValue *val     = ghb_widget_value(GTK_WIDGET(desc));
        ghb_dict_set(ud->settings, "PresetDescription", ghb_value_dup(val));
        folder_save(ud, path, name);
    }
    free(path);
}

G_MODULE_EXPORT void
presets_save_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    const gchar       *name;
    const gchar       *fullname;
    hb_preset_index_t *path;
    int                width, height;
    gboolean           autoscale;
    GtkWidget         *dialog;
    GtkEntry          *entry;
    GtkTextView       *desc;
    GtkResponseType    response;

    name      = ghb_dict_get_string(ud->settings, "PresetName");
    fullname  = ghb_dict_get_string(ud->settings, "PresetFullName");
    width     = ghb_dict_get_int(ud->settings, "PictureWidth");
    height    = ghb_dict_get_int(ud->settings, "PictureHeight");
    autoscale = ghb_dict_get_bool(ud->settings, "autoscale");

    ghb_ui_update(ud, "PictureWidthEnable", ghb_boolean_value(!autoscale));
    ghb_ui_update(ud, "PictureHeightEnable", ghb_boolean_value(!autoscale));

    path     = hb_preset_search_index(fullname, 0);
    desc     = GTK_TEXT_VIEW(GHB_WIDGET(ud->builder, "PresetDescription"));
    if (!width)
    {
        width = ghb_dict_get_int(ud->settings, "scale_width");
        ghb_ui_update(ud, "PictureWidth", ghb_int_value(width));
    }
    if (!height)
    {
        height = ghb_dict_get_int(ud->settings, "scale_height");
        ghb_ui_update(ud, "PictureHeight", ghb_int_value(height));
    }

    dialog   = GHB_WIDGET(ud->builder, "preset_save_dialog");
    entry    = GTK_ENTRY(GHB_WIDGET(ud->builder, "PresetName"));
    gtk_entry_set_text(entry, name);
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    if (response == GTK_RESPONSE_OK)
    {
        // save the preset
        name = gtk_entry_get_text(entry);
        ghb_widget_to_setting(ud->settings, GTK_WIDGET(desc));
        settings_save(ud, path, name);
    }
    free(path);
}

G_MODULE_EXPORT void
preset_type_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
}

G_MODULE_EXPORT void
presets_restore_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    // Reload the builtin presets
    hb_presets_builtin_update();
    store_presets();

    ghb_presets_list_reinit(ud);
    ghb_select_default_preset(ud->builder);
}

G_MODULE_EXPORT void
presets_remove_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    GtkTreeView      *treeview;
    GtkTreeSelection *selection;
    GtkTreeModel     *store;
    GtkTreeIter       iter;
    gchar            *preset;
    GtkResponseType   response;

    treeview  = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
    selection = gtk_tree_view_get_selection (treeview);
    if (gtk_tree_selection_get_selected(selection, &store, &iter))
    {
        GtkWindow         *hb_window;
        GtkWidget         *dialog;
        gboolean           is_folder;
        hb_preset_index_t *path;

        gtk_tree_model_get(store, &iter, 0, &preset, -1);

        path      = ghb_tree_get_index(store, &iter);
        is_folder = preset_is_folder(path);
        hb_window = GTK_WINDOW(GHB_WIDGET(ud->builder, "hb_window"));
        dialog = gtk_message_dialog_new(hb_window, GTK_DIALOG_MODAL,
                            GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                            _("Confirm deletion of %s:\n\n%s"),
                            is_folder ? _("folder") : _("preset"),
                            preset);
        response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy (dialog);
        if (response == GTK_RESPONSE_YES)
        {
            GtkTreeIter nextIter = iter;
            gboolean    valid    = TRUE;

            // Determine which preset to highlight after deletion done
            if (!gtk_tree_model_iter_next(store, &nextIter))
            {
                nextIter = iter;
                if (!gtk_tree_model_iter_previous(store, &nextIter))
                {
                    nextIter = iter;
                    if (!gtk_tree_model_iter_parent(store, &nextIter, &iter))
                    {
                        nextIter = iter;
                        valid = gtk_tree_model_get_iter_first(store, &nextIter);
                    }
                }
            }

            // Remove the selected item
            // First unselect it so that selecting the new item works properly
            gtk_tree_selection_unselect_iter(selection, &iter);
            if (hb_preset_delete(path) >= 0)
            {
                store_presets();
                presets_list_remove(ud, path);
            }
            if (valid)
            {
                hb_preset_index_t *path;

                path = ghb_tree_get_index(store, &nextIter);
                select_preset2(ud->builder, path);
                free(path);
            }
        }
        free(path);
        g_free(preset);
    }
}

// controls where valid drop locations are
G_MODULE_EXPORT gboolean
presets_drag_motion_cb(
    GtkTreeView        *tv,
    GdkDragContext     *ctx,
    gint                x,
    gint                y,
    guint               time,
    signal_user_data_t *ud)
{
    GtkTreeViewDropPosition  drop_pos;
    GtkTreeIter              iter;
    GtkTreeView             *srctv;
    GtkTreeModel            *model;
    GtkTreeSelection        *select;
    GtkTreePath             *treepath;
    hb_preset_index_t       *path;
    gint                     src_ptype, dst_ptype;
    gboolean                 src_folder, dst_folder;
    GhbValue                *src_preset, *dst_preset;
    gint                     tree_depth, ii;
    GtkWidget               *widget;

    widget = gtk_drag_get_source_widget(ctx);
    if (widget == NULL || widget != GTK_WIDGET(tv))
        return TRUE;

    // Get the type of the object being dragged
    srctv  = GTK_TREE_VIEW(gtk_drag_get_source_widget(ctx));
    select = gtk_tree_view_get_selection(srctv);
    gtk_tree_selection_get_selected (select, &model, &iter);
    path   = ghb_tree_get_index(model, &iter);

    src_preset = hb_preset_get(path);
    free(path);
    if (src_preset == NULL)
    {
        gdk_drag_status(ctx, 0, time);
        return TRUE;
    }

    tree_depth = preset_tree_depth(src_preset);
    src_ptype  = ghb_dict_get_int(src_preset, "Type");
    src_folder = ghb_dict_get_bool(src_preset, "Folder");

    if (src_folder && tree_depth == 1)
        tree_depth = 2;

    // The rest checks that the destination is a valid position
    // in the list.
    gtk_tree_view_get_dest_row_at_pos(tv, x, y, &treepath, &drop_pos);
    if (treepath == NULL)
    {
        gdk_drag_status(ctx, 0, time);
        return TRUE;
    }
    // Don't allow repositioning of builtin presets
    if (src_ptype != PRESETS_CUSTOM)
    {
        gdk_drag_status(ctx, 0, time);
        return TRUE;
    }

    int len = gtk_tree_path_get_depth(treepath);
    if (len+tree_depth-1 >= MAX_NESTED_PRESET)
    {
        if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
            drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
        if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
            drop_pos = GTK_TREE_VIEW_DROP_AFTER;
    }
    for (ii = len+tree_depth-1; ii > MAX_NESTED_PRESET; ii--)
        gtk_tree_path_up(treepath);
    path = ghb_tree_path_get_index(treepath);

    dst_preset = hb_preset_get(path);
    free(path);
    if (dst_preset != NULL)
    {
        dst_ptype = ghb_dict_get_int(dst_preset, "Type");
        dst_folder = ghb_dict_get_bool(dst_preset, "Folder");
    }
    else
    {
        dst_ptype = PRESETS_CUSTOM;
        dst_folder = FALSE;
    }

    // Don't allow mixing custom presets in the builtins
    if (dst_ptype != PRESETS_CUSTOM)
    {
        gdk_drag_status(ctx, 0, time);
        return TRUE;
    }

    // Only allow *drop into* for folders
    if (!dst_folder)
    {
        if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
            drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
        if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
            drop_pos = GTK_TREE_VIEW_DROP_AFTER;
    }

    len = gtk_tree_path_get_depth(treepath);
    gtk_tree_view_set_drag_dest_row(tv, treepath, drop_pos);
    gtk_tree_path_free(treepath);
    gdk_drag_status(ctx, GDK_ACTION_MOVE, time);
    return TRUE;
}

G_MODULE_EXPORT void
presets_drag_cb(
    GtkTreeView        *dst_widget,
    GdkDragContext     *dc,
    gint                x,
    gint                y,
    GtkSelectionData   *selection_data,
    guint               info,
    guint               t,
    signal_user_data_t *ud)
{
    GtkTreePath             *dst_treepath = NULL;
    GtkTreeViewDropPosition  drop_pos;
    GtkTreeIter              dst_iter, src_iter;
    gint                     src_ptype;
    gboolean                 src_folder, dst_folder;

    GtkTreeModel *dst_model = gtk_tree_view_get_model(dst_widget);

    // This doesn't work here for some reason...
    // gtk_tree_view_get_drag_dest_row(dstwidget, &path, &drop_pos);
    gtk_tree_view_get_dest_row_at_pos(dst_widget, x, y,
                                      &dst_treepath, &drop_pos);
    // This little hack is needed because attempting to drop after
    // the last item gives us no dst_treepath or drop_pos.
    if (dst_treepath == NULL)
    {
        gint n_children;

        n_children = gtk_tree_model_iter_n_children(dst_model, NULL);
        if (n_children)
        {
            drop_pos = GTK_TREE_VIEW_DROP_AFTER;
            dst_treepath = gtk_tree_path_new_from_indices(n_children-1, -1);
        }
        else
        {
            drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
            dst_treepath = gtk_tree_path_new_from_indices(0, -1);
        }
    }
    if (dst_treepath)
    {
        GtkTreeView       *src_widget;
        GtkTreeModel      *src_model;
        GtkTreeSelection  *select;
        gint               tree_depth, ii;
        hb_preset_index_t *dst_path, *src_path;
        hb_value_t        *src_preset;

        src_widget = GTK_TREE_VIEW(gtk_drag_get_source_widget(dc));
        select     = gtk_tree_view_get_selection (src_widget);
        gtk_tree_selection_get_selected(select, &src_model, &src_iter);

        src_path   = ghb_tree_get_index(src_model, &src_iter);
        src_ptype  = preset_get_type(src_path);
        src_folder = preset_is_folder(src_path);

        // Don't allow repositioning of builtin presets
        if (src_ptype != PRESETS_CUSTOM)
        {
            free(src_path);
            return;
        }

        src_preset = hb_preset_get(src_path);
        tree_depth = preset_tree_depth(src_preset);
        if (src_folder && tree_depth == 1)
            tree_depth = 2;

        dst_path = ghb_tree_path_get_index(dst_treepath);
        if (dst_path->depth + tree_depth - 1 >= MAX_NESTED_PRESET)
        {
            if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
                drop_pos = GTK_TREE_VIEW_DROP_BEFORE;
            else if (drop_pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
                drop_pos = GTK_TREE_VIEW_DROP_AFTER;
        }

        for (ii = dst_path->depth + tree_depth-1; ii > MAX_NESTED_PRESET; ii--)
        {
            gtk_tree_path_up(dst_treepath);
            dst_path->depth--;
        }

        dst_folder = preset_is_folder(dst_path);

        // Only allow *drop into* for folders
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
            dst_path = ghb_tree_get_index(dst_model, &iter);
            hb_preset_move(src_path, dst_path);
            free(dst_path);

            // Remove the old entry in the UI list
            gtk_tree_store_remove(GTK_TREE_STORE(src_model), &src_iter);

            // UI elements were shuffled again.  recompute dst_path
            dst_path = ghb_tree_get_index(dst_model, &iter);
            presets_list_update_item(ud, dst_path, TRUE);
            select_preset2(ud->builder, dst_path);
            free(dst_path);

            store_presets();
        }
        gtk_tree_path_free(dst_treepath);
        free(src_path);
    }
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
    path     = ghb_tree_path_get_index(treepath);
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

char*
preset_get_fullname(hb_preset_index_t *path)
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
            g_string_append(gstr, "/");
            g_string_append(gstr, name);
        }
    }
    free(tmp);
    char *str = g_string_free(gstr, FALSE);
    return str;
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

    tv = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
    ts = gtk_tree_view_get_selection(tv);
    if (gtk_tree_selection_get_selected(ts, &tm, &ti))
    {
        hb_preset_index_t *path;

        path   = ghb_tree_get_index(tm, &ti);
        preset = hb_preset_get(path);
        if (preset != NULL)
        {
            char *fullname;

            preset   = hb_value_dup(preset);
            fullname = preset_get_fullname(path);
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
    GtkWidget         *widget;
    hb_preset_index_t *path;

    widget = GHB_WIDGET (ud->builder, "presets_remove");
    path   = get_selected_path(ud);
    if (path != NULL)
    {
        GhbValue *dict = hb_preset_get(path);
        if (!ghb_dict_get_bool(dict, "Folder") &&
            !ghb_dict_get_bool(ud->settings, "preset_reload"))
        {
            ghb_preset_to_settings(ud->settings, dict);
            char *fullname = preset_get_fullname(path);
            ghb_dict_set_string(ud->settings, "PresetFullName", fullname);
            free(fullname);
            ghb_set_current_title_settings(ud);
            ghb_load_post_settings(ud);
        }
        gtk_widget_set_sensitive(widget, TRUE);
    }
    else
    {
        g_debug(_("No selection???  Perhaps unselected."));
        gtk_widget_set_sensitive(widget, FALSE);
    }
}

void
ghb_clear_presets_selection(signal_user_data_t *ud)
{
    GtkTreeView      *treeview;
    GtkTreeSelection *selection;

    if (ud->dont_clear_presets) return;
    treeview  = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
    selection = gtk_tree_view_get_selection (treeview);
    gtk_tree_selection_unselect_all (selection);
    ghb_dict_set_bool(ud->settings, "preset_modified", TRUE);
}

G_MODULE_EXPORT void
presets_default_clicked_cb(GtkWidget *xwidget, signal_user_data_t *ud)
{
    hb_preset_index_t *path = get_selected_path(ud);
    if (path != NULL)
    {
        hb_value_t *dict = hb_preset_get(path);
        if (dict != NULL && !ghb_dict_get_bool(dict, "Folder"))
        {
            ghb_presets_list_clear_default(ud);
            ghb_dict_set_bool(dict, "Default", 1);
            ghb_presets_list_show_default(ud);
            store_presets();
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

    treeview = GTK_TREE_VIEW(GHB_WIDGET(ud->builder, "presets_list"));
    store    = GTK_TREE_STORE(gtk_tree_view_get_model(treeview));
    treepath = gtk_tree_path_new_from_string(treepath_s);
    path     = ghb_tree_path_get_index(treepath);
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
preset_widget_changed_cb(GtkWidget *widget, signal_user_data_t *ud)
{
    ghb_widget_to_setting(ud->settings, widget);
    ghb_check_dependency(ud, widget, NULL);
}

