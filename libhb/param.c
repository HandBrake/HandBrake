/* param.c
 *
 * Copyright (c) 2003-2017 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit
 * http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb_dict.h"
#include "param.h"
#include "common.h"
#include "colormap.h"
#ifdef USE_QSV
#include "qsv_common.h"
#endif
#include <regex.h>

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
    { 5, "Tape",        "tape",       NULL              },
    { 6, "Sprite",      "sprite",     NULL              },
    { 0, NULL,          NULL,         NULL              }
};

static hb_filter_param_t hqdn3d_presets[] =
{
    { 1, "Custom",      "custom",     NULL              },
    { 5, "Ultralight",  "ultralight",
      "y-spatial=1:cb-spatial=0.7:cr-spatial=0.7:"
      "y-temporal=1:cb-temporal=2:cr-temporal=2"
                                                        },
    { 2, "Light",       "light",
      "y-spatial=2:cb-spatial=1:cr-spatial=1:"
      "y-temporal=2:cb-temporal=3:cr-temporal=3"
                                                        },
    { 3, "Medium",      "medium",
      "y-spatial=3:cb-spatial=2:cr-spatial=2:"
      "y-temporal=2:cb-temporal=3:cr-temporal=3"
                                                        },
    { 4, "Strong",      "strong",
      "y-spatial=7:cb-spatial=7:cr-spatial=7:"
      "y-temporal=5:cb-temporal=5:cr-temporal=5"
                                                        },
    { 0, NULL,          NULL,         NULL              },
    // Legacy and aliases go below the NULL
    { 2, "Weak",        "weak",
      "y-spatial=2:cb-spatial=1:cr-spatial=1:"
      "y-temporal=2:cb-temporal=3:cr-temporal=3"
                                                        },
    { 2, "Default",     "default",
      "y-spatial=2:cb-spatial=1:cr-spatial=1:"
      "y-temporal=2:cb-temporal=3:cr-temporal=3"
                                                        },
};

static hb_filter_param_t unsharp_presets[] =
{
    { 1, "Custom",      "custom",     NULL              },
    { 2, "Ultralight",  "ultralight", NULL              },
    { 3, "Light",       "light",      NULL              },
    { 4, "Medium",      "medium",     NULL              },
    { 5, "Strong",      "strong",     NULL              },
    { 6, "Stronger",    "stronger",   NULL              },
    { 0, NULL,          NULL,         NULL              }
};

static hb_filter_param_t unsharp_tunes[] =
{
    { 0, "None",        "none",       NULL              },
    { 1, "Fine",        "fine",       NULL              },
    { 2, "Medium",      "medium",     NULL              },
    { 3, "Coarse",      "coarse",     NULL              },
    { 0, NULL,          NULL,         NULL              }
};

static hb_filter_param_t detelecine_presets[] =
{
    { 0, "Off",         "off",        "disable=1"       },
    { 1, "Custom",      "custom",     NULL              },
    { 2, "Default",     "default",
      "skip-top=4:skip-bottom=4:skip-left=1:skip-right=1:plane=0"
                                                        },
    { 0, NULL,          NULL,         NULL              }
};

static hb_filter_param_t comb_detect_presets[] =
{
    { 0, "Off",             "off",        "disable=1"       },
    { 1, "Custom",          "custom",     NULL              },
    { 2, "Default",         "default",
      "mode=3:spatial-metric=2:motion-thresh=1:spatial-thresh=1:"
      "filter-mode=2:block-thresh=40:block-width=16:block-height=16"
                                                            },
    { 3, "Less Sensitive",  "permissive",
      "mode=3:spatial-metric=2:motion-thresh=3:spatial-thresh=3:"
      "filter-mode=2:block-thresh=40:block-width=16:block-height=16"
                                                            },
    { 4, "Fast",            "fast",
      "mode=0:spatial-metric=2:motion-thresh=2:spatial-thresh=3:"
      "filter-mode=1:block-thresh=80:block-width=16:block-height=16"
                                                            },
    { 0, NULL,              NULL,         NULL              }
};

static hb_filter_param_t decomb_presets[] =
{
    { 1, "Custom",      "custom",     NULL              },
    { 2, "Default",     "default",    "mode=7"          },
    { 4, "Bob",         "bob",        "mode=23"         },
    { 3, "EEDI2",       "eedi2",      "mode=15"         },
    { 4, "EEDI2 Bob",   "eedi2bob",   "mode=31"         },
    { 0, NULL,          NULL,         NULL              }
};

static hb_filter_param_t deinterlace_presets[] =
{
    { 1, "Custom",             "custom",       NULL             },
    { 3, "Default",            "default",      "mode=3"         },
    { 2, "Skip Spatial Check", "skip-spatial", "mode=1"         },
    { 5, "Bob",                "bob",          "mode=7"         },
#ifdef USE_QSV
    { 6, "QSV",                "qsv",          "mode=11"        },
#endif
    { 0,  NULL,                NULL,           NULL             },
    { 2, "Fast",               "fast",         "mode=1"         },
    { 3, "Slow",               "slow",         "mode=1"         },
    { 4, "Slower",             "slower",       "mode=3"         },
    { 7, "QSV",                "qsv",          "mode=3"         }
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

    { HB_FILTER_HQDN3D,      hqdn3d_presets,      NULL,
      sizeof(hqdn3d_presets) / sizeof(hb_filter_param_t)         },

    { HB_FILTER_UNSHARP,     unsharp_presets,     unsharp_tunes,
      sizeof(unsharp_presets) / sizeof(hb_filter_param_t)        },

    { HB_FILTER_DETELECINE,  detelecine_presets,  NULL,
      sizeof(detelecine_presets) / sizeof(hb_filter_param_t)     },

    { HB_FILTER_COMB_DETECT, comb_detect_presets, NULL,
      sizeof(decomb_presets) / sizeof(hb_filter_param_t)         },

    { HB_FILTER_DECOMB,      decomb_presets,      NULL,
      sizeof(decomb_presets) / sizeof(hb_filter_param_t)         },

    { HB_FILTER_DEINTERLACE, deinterlace_presets, NULL,
      sizeof(deinterlace_presets) / sizeof(hb_filter_param_t)    },

    { HB_FILTER_INVALID,     NULL,                NULL,  0       }
};

void hb_param_configure_qsv(void)
{
#ifdef USE_QSV
    if (!hb_qsv_available())
    {
        memset(&deinterlace_presets[4], 0, sizeof(hb_filter_param_t));
    }
#endif
}

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
 * tape       - analog tape sources such as VHS
 * sprite     - 1-/4-/8-/16-bit 2-dimensional games
 */
static hb_dict_t * generate_nlmeans_settings(const char *preset,
                                             const char *tune,
                                             const char *custom)
{
    hb_dict_t * settings;

    if (preset == NULL)
        return NULL;

    if (preset == NULL || !strcasecmp(preset, "custom"))
    {
        return hb_parse_filter_settings(custom);
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
        else if (!strcasecmp(tune, "tape"))
        {
            strength[0]    = 3;   strength[1]    = 6;
            origin_tune[0] = 0.8; origin_tune[1] = 0.8;
            patch_size[0]  = 3;   patch_size[1]  = 5;
            range[0]       = 5;   range[1]       = 5;
            frames[0]      = 2;   frames[1]      = 2;
            prefilter[0]   = 0;   prefilter[1]   = 0;
            if (!strcasecmp(preset, "ultralight"))
            {
                strength[0]    = 1.5; strength[1]    = 5;
                origin_tune[0] = 0.9; origin_tune[1] = 0.9;
                frames[0]      = 1;   frames[1]      = 1;
            }
            else if (!strcasecmp(preset, "light"))
            {
                strength[0]    = 2;   strength[1]    = 6;
                origin_tune[0] = 0.9; origin_tune[1] = 0.9;
            }
            else if (!strcasecmp(preset, "strong"))
            {
                strength[0]    = 3.5; strength[1]    = 8;
                origin_tune[0] = 0.6; origin_tune[1] = 0.6;
                patch_size[0]  = 5;   patch_size[1]  = 5;
            }
        }
        else if (!strcasecmp(tune, "sprite"))
        {
            strength[0]    = 3;    strength[1]    = 4;
            origin_tune[0] = 0.15; origin_tune[1] = 0.5;
            patch_size[0]  = 5;    patch_size[1]  = 5;
            range[0]       = 5;    range[1]       = 9;
            frames[0]      = 2;    frames[1]      = 4;
            prefilter[0]   = 0;    prefilter[1]   = 0;
            if (!strcasecmp(preset, "ultralight"))
            {
                strength[0]    = 1.5; strength[1]    = 3;
                range[0]       = 5;   range[1]       = 7;
                frames[0]      = 1;   frames[1]      = 2;
            }
            else if (!strcasecmp(preset, "light"))
            {
                strength[0]    = 2; strength[1]    = 4;
                frames[0]      = 2; frames[1]      = 2;
            }
            else if (!strcasecmp(preset, "strong"))
            {
                strength[0]    = 3; strength[1]    = 4;
                range[0]       = 7; range[1]       = 11;
            }
        }
        else
        {
            fprintf(stderr, "Unrecognized nlmeans tune (%s).\n", tune);
            return NULL;
        }

        settings = hb_dict_init();
        hb_dict_set(settings, "y-strength",   hb_value_double(strength[0]));
        hb_dict_set(settings, "y-origin-tune", hb_value_double(origin_tune[0]));
        hb_dict_set(settings, "y-patch-size",  hb_value_int(patch_size[0]));
        hb_dict_set(settings, "y-range",      hb_value_int(range[0]));
        hb_dict_set(settings, "y-frame-count", hb_value_int(frames[0]));
        hb_dict_set(settings, "y-prefilter",  hb_value_int(prefilter[0]));

        hb_dict_set(settings, "cb-strength",   hb_value_double(strength[1]));
        hb_dict_set(settings, "cb-origin-tune", hb_value_double(origin_tune[1]));
        hb_dict_set(settings, "cb-patch-size",  hb_value_int(patch_size[1]));
        hb_dict_set(settings, "cb-range",      hb_value_int(range[1]));
        hb_dict_set(settings, "cb-frame-count", hb_value_int(frames[1]));
        hb_dict_set(settings, "cb-prefilter",  hb_value_int(prefilter[1]));
    }
    else
    {
        settings = hb_parse_filter_settings(preset);
        if (tune != NULL)
        {
            fprintf(stderr, "Custom nlmeans parameters specified; ignoring nlmeans tune (%s).\n", tune);
        }
    }

    return settings;
}

static hb_dict_t * generate_unsharp_settings(const char *preset,
                                             const char *tune,
                                             const char *custom)
{
    hb_dict_t * settings;

    if (preset == NULL)
        return NULL;

    if (preset == NULL || !strcasecmp(preset, "custom"))
    {
        return hb_parse_filter_settings(custom);
    }
    if (!strcasecmp(preset, "ultralight") ||
        !strcasecmp(preset, "light") ||
        !strcasecmp(preset, "medium") ||
        !strcasecmp(preset, "strong") ||
        !strcasecmp(preset, "stronger"))
    {
        double strength[2];
        int    size[2];

        if (tune == NULL || !strcasecmp(tune, "none"))
        {
            strength[0]     = strength[1] = 0.25;
            size[0]         =     size[1] = 7;
            if (!strcasecmp(preset, "ultralight"))
            {
                strength[0] = strength[1] = 0.05;
            }
            else if (!strcasecmp(preset, "light"))
            {
                strength[0] = strength[1] = 0.15;
            }
            else if (!strcasecmp(preset, "strong"))
            {
                strength[0] = strength[1] = 0.5;
            }
            else if (!strcasecmp(preset, "stronger"))
            {
                strength[0] = strength[1] = 0.8;
            }
        }
        else if (!strcasecmp(tune, "fine"))
        {
            strength[0]     = 0.4; strength[1] = 0.25;
            size[0]         =      size[1]     = 3;
            if (!strcasecmp(preset, "ultralight"))
            {
                strength[0] = 0.15; strength[1] = 0.1;
            }
            else if (!strcasecmp(preset, "light"))
            {
                strength[0] = 0.25; strength[1] = 0.15;
            }
            else if (!strcasecmp(preset, "strong"))
            {
                strength[0] = 0.8;  strength[1] = 0.5;
            }
            else if (!strcasecmp(preset, "stronger"))
            {
                strength[0] = 0.8;  strength[1] = 0.5;
            }
        }
        else if (!strcasecmp(tune, "medium"))
        {
            strength[0]     = 0.275; strength[1] = 0.165;
            size[0]         = 7;     size[1] = 5;
            if (!strcasecmp(preset, "ultralight"))
            {
                strength[0] = 0.055; strength[1] = 0.033;
            }
            else if (!strcasecmp(preset, "light"))
            {
                strength[0] = 0.165; strength[1] = 0.1;
            }
            else if (!strcasecmp(preset, "strong"))
            {
                strength[0] = 0.55;  strength[1] = 0.33;
            }
            else if (!strcasecmp(preset, "stronger"))
            {
                strength[0] = 0.9;   strength[1] = 0.6;
            }
        }
        else if (!strcasecmp(tune, "coarse"))
        {
            strength[0]     = 0.275; strength[1] = 0.165;
            size[0]         = 13;    size[1] = 9;
            if (!strcasecmp(preset, "ultralight"))
            {
                strength[0] = 0.055; strength[1] = 0.033;
            }
            else if (!strcasecmp(preset, "light"))
            {
                strength[0] = 0.165; strength[1] = 0.1;
            }
            else if (!strcasecmp(preset, "strong"))
            {
                strength[0] = 0.55;  strength[1] = 0.33;
            }
            else if (!strcasecmp(preset, "stronger"))
            {
                strength[0] = 0.9;   strength[1] = 0.6;
            }
        }
        else
        {
            fprintf(stderr, "Unrecognized unsharp tune (%s).\n", tune);
            return NULL;
        }

        settings = hb_dict_init();
        hb_dict_set(settings, "y-strength", hb_value_double(strength[0]));
        hb_dict_set(settings, "y-size",     hb_value_int(size[0]));

        hb_dict_set(settings, "cb-strength", hb_value_double(strength[1]));
        hb_dict_set(settings, "cb-size",     hb_value_int(size[1]));
    }
    else
    {
        settings = hb_parse_filter_settings(preset);
        if (tune != NULL)
        {
            fprintf(stderr, "Custom unsharp parameters specified; ignoring unsharp tune (%s).\n", tune);
        }
    }

    return settings;
}

int hb_validate_param_string(const char *regex_pattern, const char *param_string)
{
    regex_t regex_temp;

    if (regcomp(&regex_temp, regex_pattern, REG_EXTENDED|REG_ICASE) == 0)
    {
        if (regexec(&regex_temp, param_string, 0, NULL, 0) == 0)
        {
            regfree(&regex_temp);
            return 0;
        }
    }
    else
    {
        hb_log("hb_validate_param_string: Error compiling regex for pattern (%s).\n", param_string);
    }

    regfree(&regex_temp);
    return 1;
}

int hb_validate_filter_settings(int filter_id, const hb_dict_t * settings)
{
    hb_filter_object_t * filter;
    hb_dict_t          * settings_template;
    hb_dict_iter_t       iter;

    if (settings == NULL)
        return 0;

    // Verify that all keys in settings are in the filter settings template
    filter = hb_filter_get(filter_id);
    if (filter == NULL)
    {
        hb_log("hb_validate_filter_settings: Unrecognized filter (%d).\n",
               filter_id);
        return 1;
    }
    if (filter->settings_template == NULL)
    {
        // filter has no template to verify settings against
        return 0;
    }
    settings_template = hb_parse_filter_settings(filter->settings_template);
    if (settings_template == NULL)
    {
        hb_log("hb_validate_filter_settings: invalid template!");
        return 0;
    }

    for (iter = hb_dict_iter_init(settings);
         iter != HB_DICT_ITER_DONE;
         iter = hb_dict_iter_next(settings, iter))
    {
        const char * key;
        hb_value_t * val;

        key = hb_dict_iter_key(iter);

        // Check if key found in settings is also found in the template
        val = hb_dict_get(settings_template, key);
        if (val == NULL)
        {
            // Key is missing from template, indicate invalid settings
            hb_log("Invalid filter key (%s) for filter %s",
                    key, filter->name);
            return 1;
        }

        // If a string value is found, and it is non-empty,
        // it is a regex pattern for allowed values.
        const char * regex_pattern = hb_value_get_string(val);
        if (regex_pattern != NULL && regex_pattern[0] != 0)
        {
            char * param;
            param = hb_value_get_string_xform(hb_dict_get(settings, key));
            if (hb_validate_param_string(regex_pattern, param) != 0)
            {
                hb_log("Invalid filter value (%s) for key %s filter %s",
                        param, key, filter->name);
                free(param);
                return 1;
            }
            free(param);
        }
    }
    hb_value_free(&settings_template);

    return 0;
}

int hb_validate_filter_settings_json(int filter_id, const char * json)
{
    hb_value_t * value  = hb_value_json(json);
    int          result = hb_validate_filter_settings(filter_id, value);
    hb_value_free(&value);

    return result;
}

static hb_filter_param_t*
filter_param_get_presets_internal(int filter_id, int *count)
{
    int ii;

    if (count != NULL)
    {
        *count = 0;
    }
    for (ii = 0; param_map[ii].filter_id != HB_FILTER_INVALID; ii++)
    {
        if (param_map[ii].filter_id == filter_id)
        {
            if (count != NULL)
            {
                *count = param_map[ii].count;
            }
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
    {
        *count = 0;
    }
    for (ii = 0; param_map[ii].filter_id != HB_FILTER_INVALID; ii++)
    {
        if (param_map[ii].filter_id == filter_id)
        {
            if (count != NULL)
            {
                *count = param_map[ii].count;
            }
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

static hb_dict_t *
generate_generic_settings(int filter_id, const char *preset, const char *custom)
{
    int preset_count;
    hb_filter_param_t *preset_table;
    hb_filter_param_t *preset_entry;

    if (preset == NULL || !strcasecmp(preset, "custom"))
    {
        return hb_parse_filter_settings(custom);
    }

    preset_table = filter_param_get_presets_internal(filter_id, &preset_count);
    preset_entry = filter_param_get_entry(preset_table, preset, preset_count);
    if (preset_entry != NULL && preset_entry->settings != NULL)
    {
        return hb_parse_filter_settings(preset_entry->settings);
    }
    return NULL;
}

static hb_value_t *
generate_deblock_settings(const char * preset, const char * custom)
{
    hb_dict_t * settings = NULL;

    // Deblock "presets" are just the QP value.  0 disables.
    if ((preset == NULL || !strcasecmp(preset, "custom")))
    {
        settings = hb_parse_filter_settings(custom);
    }
    else
    {
        settings = hb_dict_init();
        int qp = strtol(preset, NULL, 0);
        hb_dict_set(settings, "qp", hb_value_int(qp));
    }

    return settings;
}

static void check_filter_status(int filter_id, hb_value_t *settings)
{
    int disable = 0;

    if (settings == NULL)
    {
        return;
    }
    switch (filter_id)
    {
        case HB_FILTER_ROTATE:
        {
            int angle = hb_dict_get_int(settings, "angle");
            int hflip = hb_dict_get_int(settings, "hflip");
            disable = angle == 0 && hflip == 0;
        } break;
        case HB_FILTER_DEBLOCK:
        {
            int qp = hb_dict_get_int(settings, "qp");
            disable = qp < 5;
        } break;
        default:
        {
        } break;
    }
    if (disable)
    {
        hb_dict_set(settings, "disable", hb_value_bool(disable));
    }
}

hb_value_t *
hb_generate_filter_settings(int filter_id, const char *preset, const char *tune,
                            const char *custom)
{
    hb_value_t * settings = NULL;

    switch (filter_id)
    {
        case HB_FILTER_DEBLOCK:
            settings = generate_deblock_settings(preset, custom);
            break;
        case HB_FILTER_PAD:
        case HB_FILTER_ROTATE:
        case HB_FILTER_CROP_SCALE:
        case HB_FILTER_VFR:
        case HB_FILTER_RENDER_SUB:
        case HB_FILTER_GRAYSCALE:
        case HB_FILTER_QSV:
            settings = hb_parse_filter_settings(custom);
            break;
        case HB_FILTER_NLMEANS:
            settings = generate_nlmeans_settings(preset, tune, custom);
            break;
        case HB_FILTER_UNSHARP:
            settings = generate_unsharp_settings(preset, tune, custom);
            break;
        case HB_FILTER_COMB_DETECT:
        case HB_FILTER_DECOMB:
        case HB_FILTER_DETELECINE:
        case HB_FILTER_HQDN3D:
        case HB_FILTER_DEINTERLACE:
            settings = generate_generic_settings(filter_id, preset, custom);
            break;
        default:
            fprintf(stderr,
                    "hb_generate_filter_settings: Unrecognized filter (%d).\n",
                    filter_id);
            break;
    }
    check_filter_status(filter_id, settings);

    if (settings != NULL &&
        hb_validate_filter_settings(filter_id, settings) == 0)
    {
        return settings;
    }
    hb_value_free(&settings);
    return NULL;
}

char *
hb_generate_filter_settings_json(int filter_id, const char *preset,
                                 const char *tune, const char *custom)
{
    hb_value_t * settings;

    settings = hb_generate_filter_settings(filter_id, preset, tune, custom);
    if (settings == NULL)
    {
        return NULL;
    }

    char * result = hb_value_get_json(settings);
    hb_value_free(&settings);
    return result;
}

int hb_validate_filter_string(int filter_id, const char * filter_str)
{
    hb_dict_t * settings = hb_parse_filter_settings(filter_str);
    if (settings == NULL)
    {
        return 1;
    }
    int result = hb_validate_filter_settings(filter_id, settings);
    hb_value_free(&settings);
    return result;
}

int
hb_validate_filter_preset(int filter_id, const char *preset, const char *tune,
                          const char *custom)
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
    if (!strcasecmp(preset, "custom") && custom != NULL)
    {
        hb_dict_t * settings = hb_parse_filter_settings(custom);
        if (settings == NULL)
        {
            return 1;
        }
        int result = hb_validate_filter_settings(filter_id, settings);
        hb_value_free(&settings);
        return result;
    }
    if (tune != NULL)
    {
        tune_table = filter_param_get_tunes_internal(filter_id, &tune_count);
        tune_entry = filter_param_get_entry(tune_table, tune, tune_count);
        if (tune_entry == NULL)
        {
            return 1;
        }
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

// Get json array of filter preset name and short_name
char * hb_filter_get_presets_json(int filter_id)
{
    hb_value_array_t  * array = hb_value_array_init();
    int                 ii, count = 0;
    hb_filter_param_t * table;

    table = filter_param_get_presets_internal(filter_id, NULL);

    for (count = 0; table[count].name != NULL; count++);
    for (ii = 0; ii < count; ii++)
    {
        hb_dict_t * dict = hb_dict_init();
        hb_dict_set(dict, "short_name", hb_value_string(table[ii].short_name));
        hb_dict_set(dict, "name", hb_value_string(table[ii].name));
        hb_value_array_append(array, dict);
    }

    char * result = hb_value_get_json(array);
    hb_value_free(&array);
    return result;
}

// Get json array of filter tune name and short_name
char * hb_filter_get_tunes_json(int filter_id)
{
    hb_value_array_t  * array = hb_value_array_init();
    int                 ii, count = 0;
    hb_filter_param_t * table;

    table = filter_param_get_tunes_internal(filter_id, NULL);

    for (count = 0; table[count].name != NULL; count++);
    for (ii = 0; ii < count; ii++)
    {
        hb_dict_t * dict = hb_dict_init();
        hb_dict_set(dict, "short_name", hb_value_string(table[ii].short_name));
        hb_dict_set(dict, "name", hb_value_string(table[ii].name));
        hb_value_array_append(array, dict);
    }

    char * result = hb_value_get_json(array);
    hb_value_free(&array);
    return result;
}

char ** hb_filter_get_presets_short_name(int filter_id)
{
    int                 ii, count = 0;
    hb_filter_param_t * table;

    table = filter_param_get_presets_internal(filter_id, NULL);

    for (count = 0; table[count].name != NULL; count++);
    char ** result = calloc(count + 1, sizeof(char*));
    for (ii = 0; ii < count; ii++)
    {
        result[ii] = strdup(table[ii].short_name);
    }
    result[ii] = NULL;

    return result;
}

char ** hb_filter_get_presets_name(int filter_id)
{
    int                 ii, count = 0;
    hb_filter_param_t * table;

    table = filter_param_get_presets_internal(filter_id, NULL);

    for (count = 0; table[count].name != NULL; count++);
    char ** result = calloc(count + 1, sizeof(char*));
    for (ii = 0; ii < count; ii++)
    {
        result[ii] = strdup(table[ii].name);
    }
    result[ii] = NULL;

    return result;
}

char ** hb_filter_get_tunes_short_name(int filter_id)
{
    int                 ii, count = 0;
    hb_filter_param_t * table;

    table = filter_param_get_tunes_internal(filter_id, NULL);

    for (count = 0; table[count].name != NULL; count++);
    char ** result = calloc(count + 1, sizeof(char*));
    for (ii = 0; ii < count; ii++)
    {
        result[ii] = strdup(table[ii].short_name);
    }
    result[ii] = NULL;

    return result;
}

char ** hb_filter_get_tunes_name(int filter_id)
{
    int                 ii, count = 0;
    hb_filter_param_t * table;

    table = filter_param_get_tunes_internal(filter_id, NULL);

    for (count = 0; table[count].name != NULL; count++);
    char ** result = calloc(count + 1, sizeof(char*));
    for (ii = 0; ii < count; ii++)
    {
        result[ii] = strdup(table[ii].name);
    }
    result[ii] = NULL;

    return result;
}

char ** hb_filter_get_keys(int filter_id)
{
    hb_filter_object_t * filter = hb_filter_get(filter_id);

    if (filter == NULL || filter->settings_template == NULL)
    {
        return NULL;
    }

    char ** tmpl = hb_str_vsplit(filter->settings_template, ':');
    int     ii, count = 0;

    for (ii = 0; tmpl[ii] != NULL; ii++)
    {
        count++;
    }
    char ** result = calloc(count + 1, sizeof(char*));
    for (ii = 0; tmpl[ii] != NULL; ii++)
    {
        char ** pair = hb_str_vsplit(tmpl[ii], '=');
        result[ii] = strdup(pair[0]);
        hb_str_vfree(pair);
    }
    result[ii] = NULL;
    hb_str_vfree(tmpl);

    return result;
}

