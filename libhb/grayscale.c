/* grayscale.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/avfilter_priv.h"

const char grayscale_template[] =
    "cb=^"HB_FLOAT_REG"$:cr=^"HB_FLOAT_REG"$:size=^"HB_FLOAT_REG"$:high=^"HB_FLOAT_REG"$";

static int grayscale_init(hb_filter_object_t *filter, hb_filter_init_t *init);

hb_filter_object_t hb_filter_grayscale =
{
    .id                = HB_FILTER_GRAYSCALE,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Grayscale",
    .settings          = NULL,
    .init              = grayscale_init,
    .work              = hb_avfilter_null_work,
    .close             = hb_avfilter_alias_close,
    .settings_template = grayscale_template
};

static int grayscale_init(hb_filter_object_t * filter, hb_filter_init_t *init)
{
    hb_filter_private_t *pv = NULL;

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;
    if (pv == NULL)
    {
        return 1;
    }
    pv->input = *init;

    hb_dict_t *settings = filter->settings;

    double cb = 0, cr = 0, size = 1, high = 0;

    hb_dict_extract_double(&cb, settings, "cb");
    hb_dict_extract_double(&cr, settings, "cr");
    hb_dict_extract_double(&size, settings, "size");
    hb_dict_extract_double(&high, settings, "high");

    hb_dict_t *avsettings = hb_dict_init();
    hb_dict_t *avfilter = hb_dict_init();

    hb_dict_set_double(avsettings, "cb", cb);
    hb_dict_set_double(avsettings, "cr", cr);
    hb_dict_set_double(avsettings, "size", size);
    hb_dict_set_double(avsettings, "high", high);

    hb_dict_set(avfilter, "monochrome", avsettings);

    pv->avfilters = avfilter;

    pv->output = *init;

    return 0;
}
