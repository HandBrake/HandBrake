/* colorspace.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/avfilter_priv.h"

static int colorspace_init(hb_filter_object_t * filter,
                           hb_filter_init_t * init);

const char colorspace_template[] =
    "format=^"HB_ALL_REG"$:range=^"HB_ALL_REG"$:primaries=^"HB_ALL_REG"$:"
    "matrix=^"HB_ALL_REG"$:transfer=^"HB_ALL_REG"$";

hb_filter_object_t hb_filter_colorspace =
{
    .id                = HB_FILTER_COLORSPACE,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Colorspace",
    .settings          = NULL,
    .init              = colorspace_init,
    .work              = hb_avfilter_null_work,
    .close             = hb_avfilter_alias_close,
    .settings_template = colorspace_template,
};

static int colorspace_init(hb_filter_object_t * filter, hb_filter_init_t * init)
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

    char * format = NULL, * range = NULL;
    char * primaries = NULL, * transfer = NULL, * matrix = NULL;

    hb_dict_extract_string(&format, settings, "format");
    hb_dict_extract_string(&range, settings, "range");
    hb_dict_extract_string(&primaries, settings, "primaries");
    hb_dict_extract_string(&transfer, settings, "transfer");
    hb_dict_extract_string(&matrix, settings, "matrix");

    if (!(format || range || primaries || transfer || matrix))
    {
        return 0;
    }

    hb_dict_t * avfilter   = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

    if (format)
    {
        hb_dict_set_string(avsettings, "format", format);
    }
    if (range)
    {
        hb_dict_set_string(avsettings, "range", range);
    }
    if (primaries)
    {
        hb_dict_set_string(avsettings, "primaries", primaries);
    }
    if (transfer)
    {
        hb_dict_set_string(avsettings, "trc", transfer);
    }
    if (matrix)
    {
        hb_dict_set_string(avsettings, "space", matrix);
    }
    hb_dict_set(avfilter, "colorspace", avsettings);
    pv->avfilters = avfilter;

    pv->output = *init;

    return 0;
}
