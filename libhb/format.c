/* format.c

   Copyright (c) 2003-2021 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/avfilter_priv.h"

static int format_init(hb_filter_object_t *filter,
                           hb_filter_init_t *init);

const char format_template[] =
    "format=^"HB_ALL_REG"$";

hb_filter_object_t hb_filter_format =
{
    .id                = HB_FILTER_FORMAT,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Format",
    .settings          = NULL,
    .init              = format_init,
    .work              = hb_avfilter_null_work,
    .close             = hb_avfilter_alias_close,
    .settings_template = format_template,
};

static int format_init(hb_filter_object_t *filter, hb_filter_init_t *init)
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
    char *format = NULL;

    hb_dict_extract_string(&format, settings, "format");

    if (format == NULL)
    {
        return 0;
    }

    hb_value_array_t *avfilters = hb_value_array_init();
    hb_dict_t *avfilter   = hb_dict_init();
    hb_dict_t *avsettings = hb_dict_init();

    hb_dict_set_string(avsettings, "pix_fmts", format);
    hb_dict_set(avfilter, "format", avsettings);

    hb_value_array_append(avfilters, avfilter);

    pv->avfilters = avfilters;

    init->pix_fmt = av_get_pix_fmt(format);
    pv->output = *init;

    return 0;
}
