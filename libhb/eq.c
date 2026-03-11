/* eq.c

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/avfilter_priv.h"
#include "libavutil/pixdesc.h"

static int eq_init(hb_filter_object_t * filter, hb_filter_init_t * init);

const char eq_template[] =
    "brightness=^-?"HB_FLOAT_REG"$:contrast=^-?"HB_FLOAT_REG"$:"
    "saturation=^"HB_FLOAT_REG"$:gamma=^"HB_FLOAT_REG"$";

hb_filter_object_t hb_filter_eq =
{
    .id                = HB_FILTER_EQ,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Equalizer",
    .settings          = NULL,
    .init              = eq_init,
    .work              = hb_avfilter_null_work,
    .close             = hb_avfilter_alias_close,
    .settings_template = eq_template,
};

static int eq_init(hb_filter_object_t * filter, hb_filter_init_t * init)
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

    double brightness = 0.0, contrast = 1.0, saturation = 1.0, gamma = 1.0;

    hb_dict_extract_double(&brightness, settings, "brightness");
    hb_dict_extract_double(&contrast, settings, "contrast");
    hb_dict_extract_double(&saturation, settings, "saturation");
    hb_dict_extract_double(&gamma, settings, "gamma");

    hb_value_array_t * avfilters = hb_value_array_init();

    hb_dict_t * avfilter   = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

    hb_dict_set_double(avsettings, "brightness", brightness);
    hb_dict_set_double(avsettings, "contrast", contrast);
    hb_dict_set_double(avsettings, "saturation", saturation);
    hb_dict_set_double(avsettings, "gamma", gamma);
    hb_dict_set(avfilter, "eq", avsettings);
    hb_value_array_append(avfilters, avfilter);

    // Ensure output pixel format matches input
    avfilter   = hb_dict_init();
    avsettings = hb_dict_init();
    hb_dict_set(avsettings, "pix_fmts",
                hb_value_string(av_get_pix_fmt_name(init->pix_fmt)));
    hb_dict_set(avfilter, "format", avsettings);
    hb_value_array_append(avfilters, avfilter);

    pv->avfilters = avfilters;

    pv->output = *init;

    return 0;
}
