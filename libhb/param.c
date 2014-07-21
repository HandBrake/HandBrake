/* param.c
 *
 * Copyright (c) 2003-2014 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit
 * http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"

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
            hb_log("Unrecognized nlmeans tune (%s).\n", tune);
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
            hb_log("Custom nlmeans parameters specified; ignoring nlmeans tune (%s).\n", tune);
        }
    }

    return opt;
}

/* HQDN3D presets
 *
 * Presets adjust strength:
 * ultralight - visually transparent
 * light
 * medium
 * strong
 */
static char * generate_hqdn3d_settings(const char *preset, const char *tune)
{
    if (!strcasecmp(preset, "strong"))
        return strdup("7:7:7:5:5:5");
    else if (!strcasecmp(preset, "medium"))
        return strdup("3:2:2:2:3:3");
    else if (!strcasecmp(preset, "light") || !strcasecmp(preset, "weak"))
        return strdup("2:1:1:2:3:3");
    else if (!strcasecmp(preset, "ultralight"))
        return strdup("1:0.7:0.7:1:2:2");
    else
        return strdup(preset);
}

char * hb_generate_filter_settings(int filter_id, const char *preset, const char *tune)
{
    switch (filter_id)
    {
        case HB_FILTER_NLMEANS:
            return generate_nlmeans_settings(preset, tune);
        case HB_FILTER_HQDN3D:
            return generate_hqdn3d_settings(preset, tune);
        default:
            hb_log("hb_generate_filter_settings: Unrecognized filter %d\n",
                   filter_id);
            break;
    }
    return NULL;
}

