/* rotate.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/avfilter_priv.h"

static int rotate_init(hb_filter_object_t * filter, hb_filter_init_t * init);

const char rotate_template[] =
    "angle=^(0|90|180|270)$:hflip=^"HB_BOOL_REG"$:disable=^"HB_BOOL_REG"$";

hb_filter_object_t hb_filter_rotate =
{
    .id                = HB_FILTER_ROTATE,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Rotate",
    .settings          = NULL,
    .init              = rotate_init,
    .work              = hb_avfilter_null_work,
    .close             = hb_avfilter_alias_close,
    .settings_template = rotate_template,
};

/* Rotate Settings:
 *  degrees:mirror
 *
 *  degrees - Rotation angle, may be one of 90, 180, or 270
 *  mirror  - Mirror image around x axis
 *
 * Examples:
 * Mode 180:1 Mirror then rotate 180'
 * Mode   0:1 Mirror
 * Mode 180:0 Rotate 180'
 * Mode  90:0 Rotate 90'
 * Mode 270:0 Rotate 270'
 *
 * Legacy Mode Examples (also accepted):
 * Mode 1: Flip vertically (y0 becomes yN and yN becomes y0) (aka 180:1)
 * Mode 2: Flip horizontally (x0 becomes xN and xN becomes x0) (aka 0:1)
 * Mode 3: Flip both horizontally and vertically (aka 180:0)
 * Mode 4: Rotate 90' (aka 90:0)
 * Mode 7: Flip horiz & vert plus Rotate 90' (aka 270:0)
 */
static int rotate_init(hb_filter_object_t * filter, hb_filter_init_t * init)
{
    hb_filter_private_t * pv = NULL;

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;
    if (pv == NULL)
    {
        return 1;
    }
    pv->input = *init;

    hb_dict_t        * settings = filter->settings;

    int           width  = init->geometry.width;
    int           height = init->geometry.height;
    const char *  trans  = NULL;
    int           angle  = 0, flip = 0, hflip = 0, vflip = 0, tmp;

    hb_dict_extract_int(&angle, settings, "angle");
    hb_dict_extract_bool(&flip, settings, "hflip");

    const char * clock;
    const char * cclock;
    if (flip)
    {
        clock  = "clock_flip";
        cclock = "cclock_flip";
    }
    else
    {
        clock  = "clock";
        cclock = "cclock";
    }
    switch (angle)
    {
        case 0:
            hflip = flip;
            break;
        case 90:
            trans  = clock;
            tmp    = width;
            width  = height;
            height = tmp;
            break;
        case 180:
            vflip = 1;
            hflip = !flip;
            break;
        case 270:
            trans  = cclock;
            tmp    = width;
            width  = height;
            height = tmp;
            break;
        default:
            break;
    }
    if (trans != NULL)
    {
        hb_dict_t * avfilter = hb_dict_init();
        hb_dict_t * avsettings = hb_dict_init();

        hb_dict_set(avsettings, "dir", hb_value_string(trans));
        hb_dict_set(avfilter, "transpose", avsettings);
        pv->avfilters = avfilter;
    }
    else if (hflip || vflip)
    {
        hb_value_array_t * avfilters = hb_value_array_init();
        hb_dict_t        * avfilter;
        if (vflip)
        {
            avfilter = hb_dict_init();
            hb_dict_set(avfilter, "vflip", hb_value_null());
            hb_value_array_append(avfilters, avfilter);
        }
        if (hflip)
        {
            avfilter = hb_dict_init();
            hb_dict_set(avfilter, "hflip", hb_value_null());
            hb_value_array_append(avfilters, avfilter);
        }
        pv->avfilters = avfilters;
    }
    else
    {
        pv->avfilters = hb_value_null();
    }

    init->geometry.width = width;
    init->geometry.height = height;
    pv->output = *init;

    return 0;
}
