/* cropscale.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/avfilter_priv.h"
#if HB_PROJECT_FEATURE_QSV && (defined( _WIN32 ) || defined( __MINGW32__ ))
#include "handbrake/qsv_common.h"
#include "libavutil/hwcontext_qsv.h"
#include "libavutil/hwcontext.h"
#endif

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
    int                width, height;
    int                cropped_width, cropped_height;
    int                top = 0, bottom = 0, left = 0, right = 0;

    // Convert crop settings to 'crop' avfilter
    hb_dict_extract_int(&top, settings, "crop-top");
    hb_dict_extract_int(&bottom, settings, "crop-bottom");
    hb_dict_extract_int(&left, settings, "crop-left");
    hb_dict_extract_int(&right, settings, "crop-right");
    cropped_width  = init->geometry.width - left - right;
    cropped_height = init->geometry.height - top - bottom;
    if (top > 0 || bottom > 0 || left > 0 || right > 0)
    {
        hb_dict_t * avfilter   = hb_dict_init();
        hb_dict_t * avsettings = hb_dict_init();

        hb_dict_set_int(avsettings, "x", left);
        hb_dict_set_int(avsettings, "y", top);
        hb_dict_set_int(avsettings, "w", cropped_width);
        hb_dict_set_int(avsettings, "h", cropped_height);
        hb_dict_set(avfilter, "crop", avsettings);
        hb_value_array_append(avfilters, avfilter);
    }

    width  = cropped_width;
    height = cropped_height;

    // Convert scale settings to 'scale' avfilter
    hb_dict_extract_int(&width, settings, "width");
    hb_dict_extract_int(&height, settings, "height");

    hb_dict_t * avfilter   = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

#if HB_PROJECT_FEATURE_QSV && (defined( _WIN32 ) || defined( __MINGW32__ ))
    if (hb_qsv_hw_filters_via_video_memory_are_enabled(init->job) || hb_qsv_hw_filters_via_system_memory_are_enabled(init->job))
    {
        if (hb_qsv_hw_filters_via_video_memory_are_enabled(init->job))
        {
            int result = hb_qsv_create_ffmpeg_vpp_pool(init, width, height);
            if (result < 0)
            {
                hb_error("hb_create_ffmpeg_pool vpp allocation failed");
                return result;
            }
        }

        if (top > 0 || bottom > 0 || left > 0 || right > 0)
        {
            hb_dict_set_int(avsettings, "cx", left);
            hb_dict_set_int(avsettings, "cy", top);
            hb_dict_set_int(avsettings, "cw", cropped_width);
            hb_dict_set_int(avsettings, "ch", cropped_height);
        }

        hb_dict_set_int(avsettings, "w", width);
        hb_dict_set_int(avsettings, "h", height);
        hb_dict_set_int(avsettings, "async_depth", init->job->qsv.async_depth);
        int hw_generation = hb_qsv_hardware_generation(hb_qsv_get_platform(hb_qsv_get_adapter_index()));
        if (init->job->qsv.ctx->vpp_scale_mode)
        {
            hb_dict_set_string(avsettings, "scale_mode", init->job->qsv.ctx->vpp_scale_mode);
            hb_log("qsv: scaling filter mode %s", init->job->qsv.ctx->vpp_scale_mode);
        }
        else if (hw_generation >= QSV_G8)
        {
            hb_dict_set_string(avsettings, "scale_mode", "compute");
            hb_log("qsv: scaling filter mode %s", "compute");
        }
        if (init->job->qsv.ctx->vpp_interpolation_method)
        {
            hb_dict_set_string(avsettings, "method", init->job->qsv.ctx->vpp_interpolation_method);
        }
        hb_dict_set(avfilter, "vpp_qsv", avsettings);
    }
    else
#endif
    {
        if (init->hw_pix_fmt == AV_PIX_FMT_CUDA)
        {
            hb_dict_set_int(avsettings, "w", width);
            hb_dict_set_int(avsettings, "h", height);
            hb_dict_set_string(avsettings, "interp_algo", "lanczos");
            hb_dict_set_string(avsettings, "format", av_get_pix_fmt_name(init->pix_fmt));
            hb_dict_set(avfilter, "scale_cuda", avsettings);
        }
        else if ((width % 2) == 0 && (height % 2) == 0 &&
            (cropped_width % 2) == 0 && (cropped_height % 2) == 0)
        {
            hb_dict_set_int(avsettings, "width", width);
            hb_dict_set_int(avsettings, "height", height);
            hb_dict_set_string(avsettings, "filter", "lanczos");
            hb_dict_set(avfilter, "zscale", avsettings);
        }
        else
        {
            hb_dict_set_int(avsettings, "width", width);
            hb_dict_set_int(avsettings, "height", height);
            hb_dict_set_string(avsettings, "flags", "lanczos+accurate_rnd");
            hb_dict_set(avfilter, "scale", avsettings);
        }
    }
    
    hb_value_array_append(avfilters, avfilter);

    avfilter   = hb_dict_init();
    avsettings = hb_dict_init();

#if HB_PROJECT_FEATURE_QSV && (defined( _WIN32 ) || defined( __MINGW32__ ))
    if (!(hb_qsv_hw_filters_via_video_memory_are_enabled(init->job) || hb_qsv_hw_filters_via_system_memory_are_enabled(init->job)))
#endif
    {
        char * out_pix_fmt = NULL;

        // "out_pix_fmt" is a private option used internally by
        // handbrake for preview generation
        hb_dict_extract_string(&out_pix_fmt, settings, "out_pix_fmt");
        if (out_pix_fmt != NULL)
        {
            hb_dict_set_string(avsettings, "pix_fmts", out_pix_fmt);
            free(out_pix_fmt);
        }
        else
        {
            hb_dict_set_string(avsettings, "pix_fmts",
                av_get_pix_fmt_name(init->pix_fmt));
        }
        hb_dict_set(avfilter, "format", avsettings);
        hb_value_array_append(avfilters, avfilter);
    }

    init->crop[0] = top;
    init->crop[1] = bottom;
    init->crop[2] = left;
    init->crop[3] = right;
    hb_limit_rational(&init->geometry.par.num, &init->geometry.par.den,
        (int64_t)init->geometry.par.num * height * cropped_width,
        (int64_t)init->geometry.par.den * width  * cropped_height, 65535);
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

