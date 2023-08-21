/* format.c

   Copyright (c) 2003-2022 HandBrake Team
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

#if HB_PROJECT_FEATURE_QSV && (defined( _WIN32 ) || defined( __MINGW32__ ))
    if (hb_qsv_full_path_is_enabled(init->job))
    {
        init->job->qsv.ctx->num_hw_filters++;
        hb_dict_set_string(avsettings, "format", format);
        init->pix_fmt = av_get_pix_fmt(format);

        if (init->job->qsv.ctx->out_range != AVCOL_RANGE_UNSPECIFIED)
            hb_dict_set_string(avsettings, "out_range", (init->job->qsv.ctx->out_range == AVCOL_RANGE_JPEG) ? "full" : "limited");

        if (hb_qsv_hw_filters_via_video_memory_are_enabled(init->job))
        {
            int result = hb_qsv_create_ffmpeg_vpp_pool(init, init->geometry.width, init->geometry.height);
            if (result < 0)
            {
                hb_error("hb_create_ffmpeg_pool vpp allocation failed");
                return result;
            }
        }
        hb_dict_set(avfilter, "vpp_qsv", avsettings);
    }
    else
#endif
    {
        hb_dict_set_string(avsettings, "pix_fmts", format);
        hb_dict_set(avfilter, "format", avsettings);
    }

    hb_value_array_append(avfilters, avfilter);

    pv->avfilters = avfilters;

    init->pix_fmt = av_get_pix_fmt(format);
    pv->output = *init;

    free(format);

    return 0;
}
