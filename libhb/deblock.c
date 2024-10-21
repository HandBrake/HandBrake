/* deblock.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/avfilter_priv.h"

static int deblock_init(hb_filter_object_t * filter, hb_filter_init_t * init);

const char deblock_template[] =
    "strength=^"HB_ALL_REG"$:thresh=^"HB_INT_REG"$:blocksize=^"HB_INT_REG"$:"
    "disable=^"HB_BOOL_REG"$";

hb_filter_object_t hb_filter_deblock =
{
    .id                = HB_FILTER_DEBLOCK,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Deblock",
    .settings          = NULL,
    .init              = deblock_init,
    .work              = hb_avfilter_null_work,
    .close             = hb_avfilter_alias_close,
    .settings_template = deblock_template,
};

/* Deblock presets and tunes
 *
 * There are currently no presets and tunes for deblock
 * The custom deblock string is converted to an avformat filter graph string
 */
static int deblock_init(hb_filter_object_t * filter, hb_filter_init_t * init)
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

    int      thresh   = -1, blocksize = 8;
    char  *  strength = NULL;

    hb_dict_extract_string(&strength, settings, "strength");
    hb_dict_extract_int(&thresh, settings, "thresh");
    hb_dict_extract_int(&blocksize, settings, "blocksize");

    hb_dict_t * avfilter = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

    if (strength != NULL)
    {
        hb_dict_set_string(avsettings, "filter", strength);
        free(strength);
    }
    hb_dict_set_int(avsettings, "block", blocksize);

    if (thresh > 0)
    {
        double alpha, beta, gamma, delta;

        alpha = thresh * 0.010;
        beta  = gamma = delta = alpha / 2;
        hb_dict_set_double(avsettings, "alpha", alpha);
        hb_dict_set_double(avsettings, "beta", beta);
        hb_dict_set_double(avsettings, "gamma", gamma);
        hb_dict_set_double(avsettings, "delta", delta);
    }
    hb_dict_set(avfilter, "deblock", avsettings);
    pv->avfilters = avfilter;

    pv->output = *init;

    return 0;
}
