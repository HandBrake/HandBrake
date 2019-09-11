/* cropscale.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/avfilter_priv.h"

static int crop_scale_init(hb_filter_object_t * filter,
                           hb_filter_init_t * init);
static hb_filter_info_t * crop_scale_info( hb_filter_object_t * filter );

static const char crop_scale_template[] =
    "width=^"HB_INT_REG"$:height=^"HB_INT_REG"$:"
    "crop-top=^"HB_INT_REG"$:crop-bottom=^"HB_INT_REG"$:"
    "crop-left=^"HB_INT_REG"$:crop-right=^"HB_INT_REG"$";

hb_filter_object_t hb_filter_crop_scale =
{
    .id                = HB_FILTER_CROP_SCALE,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Crop and Scale",
    .settings          = NULL,
    .init              = crop_scale_init,
    .work              = hb_avfilter_null_work,
    .close             = hb_avfilter_alias_close,
    .info              = crop_scale_info,
    .settings_template = crop_scale_template,
};

static const char * color_format_range(enum AVPixelFormat format, int range)
{
    switch (format)
    {
        case AV_PIX_FMT_YUVJ420P:
        case AV_PIX_FMT_YUVJ422P:
        case AV_PIX_FMT_YUVJ444P:
        case AV_PIX_FMT_YUVJ440P:
            return "full";
        default:
            return range == AVCOL_RANGE_JPEG ? "full" : "limited";
    }
}

/* CropScale Settings
 *  mode:parity
 *
 *  width       - scale width
 *  height       - scale height
 *  crop-top    - top crop margin
 *  crop-bottom - bottom crop margin
 *  crop-left   - left crop margin
 *  crop-right  - right crop margin
 *
 */
static int crop_scale_init(hb_filter_object_t * filter, hb_filter_init_t * init)
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
    hb_value_array_t * avfilters = hb_value_array_init();
    int                width, height, top, bottom, left, right;
    const char       * matrix;

    // Convert crop settings to 'crop' avfilter
    hb_dict_extract_int(&top, settings, "crop-top");
    hb_dict_extract_int(&bottom, settings, "crop-bottom");
    hb_dict_extract_int(&left, settings, "crop-left");
    hb_dict_extract_int(&right, settings, "crop-right");
    if (top > 0 || bottom > 0 || left > 0 || right > 0)
    {
        hb_dict_t * avfilter   = hb_dict_init();
        hb_dict_t * avsettings = hb_dict_init();

        hb_dict_set_int(avsettings, "x", left);
        hb_dict_set_int(avsettings, "y", top);
        hb_dict_set_int(avsettings, "w", init->geometry.width - left - right);
        hb_dict_set_int(avsettings, "h", init->geometry.height - top - bottom);
        hb_dict_set(avfilter, "crop", avsettings);
        hb_value_array_append(avfilters, avfilter);
    }

    // Convert scale settings to 'scale' avfilter
    hb_dict_extract_int(&width, settings, "width");
    hb_dict_extract_int(&height, settings, "height");

    hb_dict_t * avfilter   = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

    hb_dict_set_int(avsettings, "width", width);
    hb_dict_set_int(avsettings, "height", height);
    hb_dict_set_string(avsettings, "flags", "lanczos+accurate_rnd");
    switch (init->color_matrix)
    {
        case HB_COLR_MAT_BT709:
            matrix = "bt709";
            break;
        case HB_COLR_MAT_FCC:
            matrix = "fcc";
            break;
        case HB_COLR_MAT_SMPTE240M:
            matrix = "smpte240m";
            break;
        case HB_COLR_MAT_BT470BG:
        case HB_COLR_MAT_SMPTE170M:
            matrix = "smpte170m";
            break;
        case HB_COLR_MAT_BT2020_NCL:
        case HB_COLR_MAT_BT2020_CL:
            matrix = "bt2020";
            break;
        default:
        case HB_COLR_MAT_UNDEF:
            matrix = NULL;
            break;

    }
    if (matrix != NULL)
    {
        hb_dict_set_string(avsettings, "in_color_matrix", matrix);
        hb_dict_set_string(avsettings, "out_color_matrix", matrix);
    }
    hb_dict_set_string(avsettings, "in_range",
                       color_format_range(init->pix_fmt, init->color_range));
    hb_dict_set_string(avsettings, "out_range", "limited");
    hb_dict_set(avfilter, "scale", avsettings);
    hb_value_array_append(avfilters, avfilter);

    avfilter   = hb_dict_init();
    avsettings = hb_dict_init();

    // TODO: Support other pix formats
    // Force output to YUV420P for until other formats are supported
    hb_dict_set(avsettings, "pix_fmts", hb_value_string("yuv420p"));
    hb_dict_set(avfilter, "format", avsettings);
    hb_value_array_append(avfilters, avfilter);

    init->crop[0] = top;
    init->crop[1] = bottom;
    init->crop[2] = left;
    init->crop[3] = right;
    init->geometry.width = width;
    init->geometry.height = height;
    pv->output = *init;

    pv->avfilters = avfilters;

    return 0;
}

static hb_filter_info_t * crop_scale_info( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if (pv == NULL)
    {
        return NULL;
    }

    hb_filter_info_t * info;
    hb_dict_t        * settings = filter->settings;
    int                width, height, top, bottom, left, right;

    info = calloc(1, sizeof(hb_filter_info_t));
    if (info == NULL)
    {
        hb_error("crop_scale_info: allocation failure");
        return NULL;
    }
    info->output = pv->output;

    hb_dict_extract_int(&top, settings, "crop-top");
    hb_dict_extract_int(&bottom, settings, "crop-bottom");
    hb_dict_extract_int(&left, settings, "crop-left");
    hb_dict_extract_int(&right, settings, "crop-right");
    hb_dict_extract_int(&width, settings, "width");
    hb_dict_extract_int(&height, settings, "height");

    int cropped_width  = pv->input.geometry.width - (left + right);
    int cropped_height = pv->input.geometry.height - (top + bottom);

    info->human_readable_desc = hb_strdup_printf(
        "source: %d * %d, crop (%d/%d/%d/%d): %d * %d, scale: %d * %d",
        pv->input.geometry.width, pv->input.geometry.height,
        top, bottom, left, right,
        cropped_width, cropped_height, width, height);

    return info;
}

