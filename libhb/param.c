/* param.c
 *
 * Copyright (c) 2003-2015 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit
 * http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "param.h"
#include "common.h"
#include <regex.h>

const char hb_filter_off[] = "off";

static hb_filter_param_t nlmeans_presets[] =
{
    { 1, "Custom",      "custom",     NULL              },
    { 5, "Ultralight",  "ultralight", NULL              },
    { 2, "Light",       "light",      NULL              },
    { 3, "Medium",      "medium",     NULL              },
    { 4, "Strong",      "strong",     NULL              },
    { 0, NULL,          NULL,         NULL              }
};

static hb_filter_param_t nlmeans_tunes[] =
{
    { 0, "None",        "none",       NULL              },
    { 1, "Film",        "film",       NULL              },
    { 2, "Grain",       "grain",      NULL              },
    { 3, "High Motion", "highmotion", NULL              },
    { 4, "Animation",   "animation",  NULL              },
    { 0, NULL,          NULL,         NULL              }
};

static hb_filter_param_t hqdn3d_presets[] =
{
    { 1, "Custom",      "custom",     NULL              },
    { 5, "Ultralight",  "ultralight", "1:0.7:0.7:1:2:2" },
    { 2, "Light",       "light",      "2:1:1:2:3:3"     },
    { 3, "Medium",      "medium",     "3:2:2:2:3:3"     },
    { 4, "Strong",      "strong",     "7:7:7:5:5:5"     },
    { 0, NULL,          NULL,         NULL              },
    // Legacy and aliases go below the NULL
    { 2, "Weak",        "weak",       "2:1:1:2:3:3"     },
    { 2, "Default",     "default",    "2:1:1:2:3:3"     },
};

static hb_filter_param_t detelecine_presets[] =
{
    { 0, "Off",         "off",        hb_filter_off     },
    { 1, "Custom",      "custom",     NULL              },
    { 2, "Default",     "default",    ""                },
    { 0, NULL,          NULL,         NULL              }
};

static hb_filter_param_t decomb_presets[] =
{
    { 1, "Custom",      "custom",     NULL              },
    { 2, "Default",     "default",    ""                },
    { 3, "Fast",        "fast",       "7:2:6:9:1:80"    },
    { 4, "Bob",         "bob",        "455"             },
    { 0, NULL,          NULL,         NULL              }
};

static hb_filter_param_t deinterlace_presets[] =
{
    { 1, "Custom",      "custom",     NULL              },
    { 2, "Fast",        "fast",       "0:-1:-1:0:1"     },
    { 3, "Slow",        "slow",       "1:-1:-1:0:1"     },
    { 4, "Slower",      "slower",     "3:-1:-1:0:1"     },
    { 5, "Bob",         "bob",        "15:-1:-1:0:1"    },
    { 0,  NULL,         NULL,         NULL              },
    { 2, "Default",     "default",    "0:-1:-1:0:1"     }
};

typedef struct
{
    int                filter_id;
    hb_filter_param_t *presets;
    hb_filter_param_t *tunes;
    int                count;
} filter_param_map_t;

static filter_param_map_t param_map[] =
{
    { HB_FILTER_NLMEANS,     nlmeans_presets,     nlmeans_tunes,
      sizeof(nlmeans_presets) / sizeof(hb_filter_param_t)        },

    { HB_FILTER_HQDN3D,      hqdn3d_presets,     NULL,
      sizeof(hqdn3d_presets) / sizeof(hb_filter_param_t)         },

    { HB_FILTER_DETELECINE,  detelecine_presets,  NULL,
      sizeof(detelecine_presets) / sizeof(hb_filter_param_t)     },

    { HB_FILTER_DECOMB,      decomb_presets,      NULL,
      sizeof(decomb_presets) / sizeof(hb_filter_param_t)         },

    { HB_FILTER_DEINTERLACE, deinterlace_presets, NULL,
      sizeof(deinterlace_presets) / sizeof(hb_filter_param_t)    },

    { HB_FILTER_INVALID,     NULL,                NULL, 0        }
};

/* NL-means presets and tunes
 *
 * Presets adjust strength:
 * ultralight - visually transparent
 * light
 * medium
 * strong
 *
 * Tunes adjust settings to the specified content type:
 * none
 * film       - most content, live action
 * grain      - like film but preserves luma grain
 * highmotion - like film but avoids color smearing with stronger settings
 * animation  - cel animation such as cartoons, anime
 */
static char * generate_nlmeans_settings(const char *preset, const char *tune)
{
    char   *opt = NULL;

    if (preset == NULL)
        return NULL;

    if (!strcasecmp(preset, "custom") && tune != NULL)
    {
        return strdup(tune);
    }
    if (!strcasecmp(preset, "ultralight") ||
        !strcasecmp(preset, "light") ||
        !strcasecmp(preset, "medium") ||
        !strcasecmp(preset, "strong"))
    {
        double strength[2],
               origin_tune[2];
        int    patch_size[2],
               range[2],
               frames[2],
               prefilter[2];

        if (tune == NULL || !strcasecmp(tune, "none"))
        {
            strength[0]    = strength[1]    = 6;
            origin_tune[0] = origin_tune[1] = 1;
            patch_size[0]  = patch_size[1]  = 7;
            range[0]       = range[1]       = 3;
            frames[0]      = frames[1]      = 2;
            prefilter[0]   = prefilter[1]   = 0;
            if (!strcasecmp(preset, "ultralight"))
            {
                strength[0] = strength[1] = 1.5;
            }
            else if (!strcasecmp(preset, "light"))
            {
                strength[0] = strength[1] = 3;
            }
            else if (!strcasecmp(preset, "strong"))
            {
                strength[0] = strength[1] = 10;
            }
        }
        else if (!strcasecmp(tune, "film"))
        {
            strength[0]    = 6; strength[1] = 8;
            origin_tune[0] = origin_tune[1] = 0.8;
            patch_size[0]  = patch_size[1]  = 7;
            range[0]       = range[1]       = 3;
            frames[0]      = frames[1]      = 2;
            prefilter[0]   = prefilter[1]   = 0;
            if (!strcasecmp(preset, "ultralight"))
            {
                strength[0]    = 1.5;   strength[1]  = 2.4;
                origin_tune[0] = 0.9; origin_tune[1] = 0.9;
            }
            else if (!strcasecmp(preset, "light"))
            {
                strength[0]    = 3;   strength[1]    = 4;
                origin_tune[0] = 0.9; origin_tune[1] = 0.9;
            }
            else if (!strcasecmp(preset, "strong"))
            {
                strength[0]    = 8;   strength[1]    = 10;
                origin_tune[0] = 0.6; origin_tune[1] = 0.6;
            }
        }
        else if (!strcasecmp(tune, "grain"))
        {
            strength[0]    = 0; strength[1] = 6;
            origin_tune[0] = origin_tune[1] = 0.8;
            patch_size[0]  = patch_size[1]  = 7;
            range[0]       = range[1]       = 3;
            frames[0]      = frames[1]      = 2;
            prefilter[0]   = prefilter[1]   = 0;
            if (!strcasecmp(preset, "ultralight"))
            {
                strength[0]    = 0;   strength[1]    = 2.4;
                origin_tune[0] = 0.9; origin_tune[1] = 0.9;
            }
            else if (!strcasecmp(preset, "light"))
            {
                strength[0]    = 0;   strength[1]    = 3.5;
                origin_tune[0] = 0.9; origin_tune[1] = 0.9;
            }
            else if (!strcasecmp(preset, "strong"))
            {
                strength[0]    = 0;   strength[1]    = 8;
                origin_tune[0] = 0.6; origin_tune[1] = 0.6;
            }
        }
        else if (!strcasecmp(tune, "highmotion"))
        {
            strength[0]    = 6;   strength[1]    = 6;
            origin_tune[0] = 0.8; origin_tune[1] = 0.7;
            patch_size[0]  = 7;   patch_size[1]  = 7;
            range[0]       = 3;   range[1]       = 5;
            frames[0]      = 2;   frames[1]      = 1;
            prefilter[0]   = 0;   prefilter[1]   = 0;
            if (!strcasecmp(preset, "ultralight"))
            {
                strength[0]    = 1.5;   strength[1]  = 2.4;
                origin_tune[0] = 0.9; origin_tune[1] = 0.9;
            }
            else if (!strcasecmp(preset, "light"))
            {
                strength[0]    = 3;   strength[1]    = 3.25;
                origin_tune[0] = 0.9; origin_tune[1] = 0.8;
            }
            else if (!strcasecmp(preset, "strong"))
            {
                strength[0]    = 8;   strength[1]    = 6.75;
                origin_tune[0] = 0.6; origin_tune[1] = 0.5;
            }
        }
        else if (!strcasecmp(tune, "animation"))
        {
            strength[0]    = 5; strength[1] = 4;
            origin_tune[0] = origin_tune[1] = 0.15;
            patch_size[0]  = patch_size[1]  = 5;
            range[0]       = range[1]       = 7;
            frames[0]      = frames[1]      = 4;
            prefilter[0]   = prefilter[1]   = 0;
            if (!strcasecmp(preset, "ultralight"))
            {
                strength[0] = 2.5; strength[1] = 2;
                frames[0]   = 2;   frames[1]   = 2;
            }
            else if (!strcasecmp(preset, "light"))
            {
                strength[0] = 3; strength[1] = 2.25;
                frames[0]   = 3; frames[1]   = 3;
            }
            else if (!strcasecmp(preset, "strong"))
            {
                strength[0] = 10; strength[1] = 8;
            }
        }
        else
        {
            fprintf(stderr, "Unrecognized nlmeans tune (%s).\n", tune);
            return NULL;
        }

        opt = hb_strdup_printf("%lf:%lf:%d:%d:%d:%d:%lf:%lf:%d:%d:%d:%d",
                               strength[0], origin_tune[0], patch_size[0],
                               range[0], frames[0], prefilter[0],
                               strength[1], origin_tune[1], patch_size[1],
                               range[1], frames[1], prefilter[1]);


    }
    else
    {
        opt = strdup(preset);
        if (tune != NULL)
        {
            fprintf(stderr, "Custom nlmeans parameters specified; ignoring nlmeans tune (%s).\n", tune);
        }
    }

    return opt;
}

int hb_validate_param_string(const char *regex_pattern, const char *param_string)
{
    regex_t regex_temp;

    if (regcomp(&regex_temp, regex_pattern, REG_EXTENDED) == 0)
    {
        if (regexec(&regex_temp, param_string, 0, NULL, 0) == 0)
        {
            regfree(&regex_temp);
            return 0;
        }
    }
    else
    {
        fprintf(stderr, "hb_validate_param_string: Error compiling regex for pattern (%s).\n", param_string);
    }

    regfree(&regex_temp);
    return 1;
}

int hb_validate_filter_settings(int filter_id, const char *filter_param)
{
    if (filter_param == NULL)
        return 0;

    // Regex matches "number" followed by one or more ":number", where number is int or float
    const char *hb_colon_separated_params_regex = "^(((([\\-])?[0-9]+([.,][0-9]+)?)|(([\\-])?[.,][0-9]+))((:((([\\-])?[0-9]+([,.][0-9]+)?)|(([\\-])?[,.][0-9]+)))+)?)$";

    const char *regex_pattern = NULL;

    switch (filter_id)
    {
        case HB_FILTER_ROTATE:
        case HB_FILTER_DEBLOCK:
        case HB_FILTER_DETELECINE:
        case HB_FILTER_DECOMB:
        case HB_FILTER_DEINTERLACE:
        case HB_FILTER_NLMEANS:
        case HB_FILTER_HQDN3D:
            if (filter_param[0] == 0)
            {
                return 0;
            }
            regex_pattern = hb_colon_separated_params_regex;
            break;
        default:
            fprintf(stderr, "hb_validate_filter_settings: Unrecognized filter (%d).\n",
                   filter_id);
            return 1;
            break;
    }

    if (hb_validate_param_string(regex_pattern, filter_param) == 0)
    {
        return 0;
    }
    return 1;
}

static hb_filter_param_t*
filter_param_get_presets_internal(int filter_id, int *count)
{
    int ii;
    for (ii = 0; param_map[ii].filter_id != HB_FILTER_INVALID; ii++)
    {
        if (param_map[ii].filter_id == filter_id)
        {
            if (count != NULL)
                *count = param_map[ii].count;
            return param_map[ii].presets;
        }
    }
    return NULL;
}

static hb_filter_param_t*
filter_param_get_tunes_internal(int filter_id, int *count)
{
    int ii;

    if (count != NULL)
        *count = 0;
    for (ii = 0; param_map[ii].filter_id != HB_FILTER_INVALID; ii++)
    {
        if (param_map[ii].filter_id == filter_id)
        {
            if (count != NULL)
                *count = param_map[ii].count;
            return param_map[ii].tunes;
        }
    }
    return NULL;
}

static hb_filter_param_t*
filter_param_get_entry(hb_filter_param_t *table, const char *name, int count)
{
    if (table == NULL || name == NULL)
        return NULL;

    int ii;
    for (ii = 0; ii < count; ii++)
    {
        if ((table[ii].name != NULL && !strcasecmp(name, table[ii].name)) ||
            (table[ii].short_name != NULL &&
             !strcasecmp(name, table[ii].short_name)))
        {
            return &table[ii];
        }
    }
    return NULL;
}

static hb_filter_param_t*
filter_param_get_entry_by_index(hb_filter_param_t *table, int index, int count)
{
    if (table == NULL)
        return NULL;

    int ii;
    for (ii = 0; ii < count; ii++)
    {
        if (table[ii].name != NULL && table[ii].index == index)
        {
            return &table[ii];
        }
    }
    return NULL;
}

static char *
generate_generic_settings(int filter_id, const char *preset, const char *tune)
{
    char *opt = NULL;
    int preset_count, tune_count;
    hb_filter_param_t *preset_table, *tune_table;
    hb_filter_param_t *preset_entry, *tune_entry;

    preset_table = filter_param_get_presets_internal(filter_id, &preset_count);
    tune_table = filter_param_get_tunes_internal(filter_id, &tune_count);
    preset_entry = filter_param_get_entry(preset_table, preset, preset_count);
    tune_entry = filter_param_get_entry(tune_table, tune, tune_count);
    if (preset_entry != NULL)
    {
        if (!strcasecmp(preset, "custom") && tune != NULL)
        {
            opt = strdup(tune);
        }
        else if (preset_entry->settings == hb_filter_off)
        {
            return (char*)hb_filter_off;
        }
        else if (preset_entry->settings != NULL)
        {
            opt = hb_strdup_printf("%s%s%s", preset_entry->settings,
                    tune_entry != NULL ? ":" : "",
                    tune_entry != NULL ? tune_entry->settings : "");
        }
    }
    else if (preset != NULL)
    {
        return strdup(preset);
    }
    return opt;
}

// Legacy: old presets store filter falues as indexes :(
static char *
generate_generic_settings_by_index(int filter_id, int preset,
                                   const char *custom)
{
    char *opt = NULL;
    int preset_count;
    hb_filter_param_t *preset_table;
    hb_filter_param_t *preset_entry;

    preset_table = filter_param_get_presets_internal(filter_id, &preset_count);
    preset_entry = filter_param_get_entry_by_index(preset_table, preset,
                                                   preset_count);
    if (preset_entry != NULL)
    {
        if (!strcasecmp(preset_entry->short_name, "custom") && custom != NULL)
        {
            opt = strdup(custom);
        }
        else if (preset_entry->settings == hb_filter_off)
        {
            return (char*)hb_filter_off;
        }
        else if (preset_entry->settings != NULL)
        {
            opt = hb_strdup_printf("%s", preset_entry->settings);
        }
    }
    return opt;
}

char *
hb_generate_filter_settings_by_index(int filter_id, int preset,
                                     const char *custom)
{
    char *filter_param = NULL;

    switch (filter_id)
    {
        case HB_FILTER_ROTATE:
            if (preset <= 0)
                filter_param = (char*)hb_filter_off;
            else
                filter_param = hb_strdup_printf("%d", preset);
            break;
        case HB_FILTER_DEBLOCK:
            if (preset < 5)
                filter_param = (char*)hb_filter_off;
            else
                filter_param = hb_strdup_printf("%d", preset);
            break;
        case HB_FILTER_DECOMB:
        case HB_FILTER_DEINTERLACE:
        case HB_FILTER_DETELECINE:
        case HB_FILTER_HQDN3D:
            filter_param = generate_generic_settings_by_index(filter_id,
                                                              preset, custom);
            break;
        default:
            fprintf(stderr,
                    "hb_generate_filter_settings: Unrecognized filter (%d).\n",
                    filter_id);
            break;
    }

    if (filter_param == hb_filter_off)
        return filter_param;

    if (filter_param != NULL &&
        hb_validate_filter_settings(filter_id, filter_param) == 0)
    {
        return filter_param;
    }
    free(filter_param);
    return NULL;
}

char *
hb_generate_filter_settings(int filter_id, const char *preset, const char *tune)
{
    char *filter_param = NULL;

    switch (filter_id)
    {
        case HB_FILTER_NLMEANS:
            filter_param = generate_nlmeans_settings(preset, tune);
            break;
        case HB_FILTER_ROTATE:
            if (atoi(preset) == 0)
                filter_param = (char*)hb_filter_off;
            else
                filter_param = strdup(preset);
            break;
        case HB_FILTER_DEBLOCK:
            if (atoi(preset) < 5)
                filter_param = (char*)hb_filter_off;
            else
                filter_param = strdup(preset);
            break;
        case HB_FILTER_DECOMB:
        case HB_FILTER_DEINTERLACE:
        case HB_FILTER_DETELECINE:
        case HB_FILTER_HQDN3D:
            filter_param = generate_generic_settings(filter_id, preset, tune);
            break;
        default:
            fprintf(stderr,
                    "hb_generate_filter_settings: Unrecognized filter (%d).\n",
                    filter_id);
            break;
    }

    if (filter_param == hb_filter_off)
        return filter_param;

    if (filter_param != NULL &&
        hb_validate_filter_settings(filter_id, filter_param) == 0)
    {
        return filter_param;
    }
    free(filter_param);
    return NULL;
}

int
hb_validate_filter_preset(int filter_id, const char *preset, const char *tune)
{
    if (preset == NULL && tune == NULL)
        return 1;

    int preset_count, tune_count;
    hb_filter_param_t *preset_table, *tune_table;
    hb_filter_param_t *preset_entry, *tune_entry;

    preset_table = filter_param_get_presets_internal(filter_id, &preset_count);
    preset_entry = filter_param_get_entry(preset_table, preset, preset_count);
    if (preset_entry == NULL || preset_entry->name == NULL)
        return 1;
    if (tune != NULL)
    {
        if (!strcasecmp(preset, "custom") && tune != NULL)
        {
            return hb_validate_filter_settings(filter_id, tune);
        }
        tune_table = filter_param_get_tunes_internal(filter_id, &tune_count);
        tune_entry = filter_param_get_entry(tune_table, tune, tune_count);
        if (tune_entry == NULL)
            return 1;
    }
    return 0;
}

int
hb_validate_filter_preset_by_index(int filter_id, int preset, const char *tune)
{
    int preset_count, tune_count;
    hb_filter_param_t *preset_table, *tune_table;
    hb_filter_param_t *preset_entry, *tune_entry;

    preset_table = filter_param_get_presets_internal(filter_id, &preset_count);
    preset_entry = filter_param_get_entry_by_index(preset_table, preset, preset_count);
    if (preset_entry == NULL || preset_entry->name == NULL)
        return 1;
    if (tune != NULL)
    {
        if (!strcasecmp(preset_entry->short_name, "custom") && tune != NULL)
        {
            return hb_validate_filter_settings(filter_id, tune);
        }
        tune_table = filter_param_get_tunes_internal(filter_id, &tune_count);
        tune_entry = filter_param_get_entry(tune_table, tune, tune_count);
        if (tune_entry == NULL)
            return 1;
    }
    return 0;
}

int
hb_filter_preset_index(int filter_id, const char *preset)
{
    if (preset == NULL)
        return -1;

    int preset_count;
    hb_filter_param_t *preset_table;
    hb_filter_param_t *preset_entry;

    preset_table = filter_param_get_presets_internal(filter_id, &preset_count);
    preset_entry = filter_param_get_entry(preset_table, preset, preset_count);
    if (preset_entry == NULL)
        return -1;
    return preset_entry->index;
}

int
hb_filter_tune_index(int filter_id, const char *tune)
{
    if (tune == NULL)
        return -1;

    int tune_count;
    hb_filter_param_t *tune_table;
    hb_filter_param_t *tune_entry;

    tune_table = filter_param_get_tunes_internal(filter_id, &tune_count);
    tune_entry = filter_param_get_entry(tune_table, tune, tune_count);
    if (tune_entry == NULL)
    {
        return -1;
    }
    return tune_entry->index;
}

hb_filter_param_t* hb_filter_param_get_presets(int filter_id)
{
    return filter_param_get_presets_internal(filter_id, NULL);
}

hb_filter_param_t* hb_filter_param_get_tunes(int filter_id)
{
    return filter_param_get_tunes_internal(filter_id, NULL);
}

