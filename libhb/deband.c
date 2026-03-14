/* deband.c

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/avfilter_priv.h"

static int deband_init(hb_filter_object_t * filter, hb_filter_init_t * init);

const char deband_template[] =
    "1thr=^"HB_FLOAT_REG"$:2thr=^"HB_FLOAT_REG"$:"
    "3thr=^"HB_FLOAT_REG"$:4thr=^"HB_FLOAT_REG"$:"
    "range=^"HB_INT_REG"$:blur=^"HB_BOOL_REG"$";

hb_filter_object_t hb_filter_deband =
{
    .id                = HB_FILTER_DEBAND,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Deband",
    .settings          = NULL,
    .init              = deband_init,
    .work              = hb_avfilter_null_work,
    .close             = hb_avfilter_alias_close,
    .settings_template = deband_template,
};

static int deband_init(hb_filter_object_t * filter, hb_filter_init_t * init)
{
    hb_filter_private_t * pv = NULL;

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;
    if (pv == NULL)
    {
        return 1;
    }
    pv->input = *init;

    hb_dict_t * settings = filter->settings;

    double _1thr = 0.02, _2thr = 0.02, _3thr = 0.02, _4thr = 0.02;
    int    range = 16, blur = 1;

    hb_dict_extract_double(&_1thr, settings, "1thr");
    hb_dict_extract_double(&_2thr, settings, "2thr");
    hb_dict_extract_double(&_3thr, settings, "3thr");
    hb_dict_extract_double(&_4thr, settings, "4thr");
    hb_dict_extract_int(&range, settings, "range");
    hb_dict_extract_int(&blur, settings, "blur");

    hb_value_array_t * avfilters = hb_value_array_init();

    hb_dict_t * avfilter   = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

    hb_dict_set_double(avsettings, "1thr", _1thr);
    hb_dict_set_double(avsettings, "2thr", _2thr);
    hb_dict_set_double(avsettings, "3thr", _3thr);
    hb_dict_set_double(avsettings, "4thr", _4thr);
    hb_dict_set_int(avsettings, "range", range);
    hb_dict_set_int(avsettings, "blur", blur);
    hb_dict_set(avfilter, "deband", avsettings);
    hb_value_array_append(avfilters, avfilter);

    pv->avfilters = avfilters;

    pv->output = *init;

    return 0;
}
