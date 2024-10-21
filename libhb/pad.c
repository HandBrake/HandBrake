/* pad.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/colormap.h"
#include "handbrake/avfilter_priv.h"

static int pad_init(hb_filter_object_t * filter, hb_filter_init_t * init);

const char pad_template[] =
    "width=^"HB_INT_REG"$:height=^"HB_INT_REG"$:color=^"HB_ALL_REG"$:"
    "x=^"HB_INT_REG"$:y=^"HB_INT_REG"$:"
    "top=^"HB_INT_REG"$:bottom=^"HB_INT_REG"$:"
    "left=^"HB_INT_REG"$:right=^"HB_INT_REG"$";

hb_filter_object_t hb_filter_pad =
{
    .id                = HB_FILTER_PAD,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Pad",
    .settings          = NULL,
    .init              = pad_init,
    .work              = hb_avfilter_null_work,
    .close             = hb_avfilter_alias_close,
    .settings_template = pad_template,
};

/* Pad presets and tunes
 *
 * There are currently no presets and tunes for pad
 * The custom pad string is converted to an avformat filter graph string
 */
static int pad_init(hb_filter_object_t * filter, hb_filter_init_t * init)
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

    int      width  = -1, height = -1, rgb = 0;
    int      top = -1, bottom = -1, left = -1, right = -1;
    int      x = -1, y = -1;
    char  *  color = NULL;

    hb_dict_extract_int(&top, settings, "top");
    hb_dict_extract_int(&bottom, settings, "bottom");
    hb_dict_extract_int(&left, settings, "left");
    hb_dict_extract_int(&right, settings, "right");

    hb_dict_extract_int(&width, settings, "width");
    hb_dict_extract_int(&height, settings, "height");
    hb_dict_extract_string(&color, settings, "color");
    hb_dict_extract_int(&x, settings, "x");
    hb_dict_extract_int(&y, settings, "y");

    if (x < 0)
    {
        x = left;
    }
    if (y < 0)
    {
        y = top;
    }
    if (top >= 0 && bottom >= 0 && height < 0)
    {
        height = init->geometry.height + top + bottom;
    }
    if (left >= 0 && right >= 0 && width < 0)
    {
        width = init->geometry.width + left + right;
    }
    if (color != NULL)
    {
        char * end;
        rgb  = strtol(color, &end, 0);
        if (end == color)
        {
            // Not a numeric value, lookup by name
            rgb = hb_rgb_lookup_by_name(color);
        }
        free(color);
        color = hb_strdup_printf("0x%06x", rgb);
    }

    char x_str[20];
    char y_str[20];
    if (x < 0)
    {
        snprintf(x_str, 20, "(out_w-in_w)/2");
    }
    else
    {
        snprintf(x_str, 20, "%d", x);
    }
    if (y < 0)
    {
        snprintf(y_str, 20, "(out_h-in_h)/2");
    }
    else
    {
        snprintf(y_str, 20, "%d", y);
    }
    if (width < init->geometry.width)
    {
        width = init->geometry.width;
    }
    if (height < init->geometry.height)
    {
        height = init->geometry.height;
    }

    hb_dict_t * avfilter = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

    hb_dict_set(avsettings, "width", hb_value_int(width));
    hb_dict_set(avsettings, "height", hb_value_int(height));
    hb_dict_set(avsettings, "x", hb_value_string(x_str));
    hb_dict_set(avsettings, "y", hb_value_string(y_str));
    if (color != NULL)
    {
        hb_dict_set(avsettings, "color", hb_value_string(color));
        free(color);
    }
    hb_dict_set(avfilter, "pad", avsettings);
    pv->avfilters = avfilter;

    init->geometry.width = width;
    init->geometry.height = height;
    pv->output = *init;

    return 0;
}

