/* deinterlace.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/decomb.h"
#include "handbrake/avfilter_priv.h"

static int deinterlace_init(hb_filter_object_t * filter,
                            hb_filter_init_t * init);

const char deint_template[] =
    "mode=^"HB_INT_REG"$:parity=^([01])$";

hb_filter_object_t hb_filter_deinterlace =
{
    .id                = HB_FILTER_DEINTERLACE,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Deinterlace",
    .settings          = NULL,
    .init              = deinterlace_init,
    .work              = hb_avfilter_null_work,
    .close             = hb_avfilter_alias_close,
    .settings_template = deint_template,
};

/* Deinterlace Settings
 *  mode:parity
 *
 *  mode   - yadif deinterlace mode
 *  parity - field parity
 *
 *  Modes:
 *      1 = Enabled
 *      2 = Spatial
 *      4 = Bob
 *      8 = Selective
 *
 *  Parity:
 *      0  = Top Field First
 *      1  = Bottom Field First
 *      -1 = Automatic detection of field parity
 *
 */
static int deinterlace_init(hb_filter_object_t * filter,
                            hb_filter_init_t * init)
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

    int          mode = 3, parity = -1;

    hb_dict_extract_int(&mode, settings, "mode");
    hb_dict_extract_int(&parity, settings, "parity");

    if (!(mode & MODE_YADIF_ENABLE))
    {
        return 0;
    }

    hb_dict_t * avfilter = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

    if (mode & MODE_YADIF_BOB)
    {
        if (mode & MODE_YADIF_SPATIAL)
        {
            hb_dict_set(avsettings, "mode", hb_value_string("send_field"));
        }
        else
        {
            hb_dict_set(avsettings, "mode",
                        hb_value_string("send_field_nospatial"));
        }
    }
    else
    {
        if (mode & MODE_YADIF_SPATIAL)
        {
            hb_dict_set(avsettings, "mode", hb_value_string("send_frame"));
        }
        else
        {
            hb_dict_set(avsettings, "mode",
                        hb_value_string("send_frame_nospatial"));
        }
    }

    if (mode & MODE_DECOMB_SELECTIVE)
    {
        hb_dict_set(avsettings, "deint", hb_value_string("interlaced"));
    }
    if (parity == 0)
    {
        hb_dict_set(avsettings, "parity", hb_value_string("tff"));
    }
    else if (parity == 1)
    {
        hb_dict_set(avsettings, "parity", hb_value_string("bff"));
    }
    hb_dict_set(avfilter, "yadif", avsettings);
    pv->avfilters = avfilter;

    pv->output = *init;

    return 0;
}
