/* colorspace.c

   Copyright (c) 2003-2025 HandBrake Team
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
    "primaries=^"HB_ALL_REG"$:transfer=^"HB_ALL_REG"$:matrix=^"HB_ALL_REG"$:range=^"HB_ALL_REG"$:"
    "tonemap=^"HB_ALL_REG"$:param=^"HB_FLOAT_REG"$:desat=^"HB_FLOAT_REG"$:npl=^"HB_FLOAT_REG"$";

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

#define REFERENCE_WHITE 100.0f

static double determine_signal_peak(hb_filter_init_t * init)
{
    double peak = init->job->coll.max_cll / REFERENCE_WHITE;
    if (!peak && init->job->mastering.has_luminance)
    {
        peak = hb_q2d(init->job->mastering.max_luminance) / REFERENCE_WHITE;
    }
    if (!peak || peak < 1)
    {
        peak = init->color_transfer == HB_COLR_TRA_SMPTEST2084 ? 100.0f : 10.0f;
    }
    return peak;
}

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

    if (init->color_prim     == HB_COLR_PRI_UNDEF ||
        init->color_transfer == HB_COLR_TRA_UNDEF ||
        init->color_matrix   == HB_COLR_MAT_UNDEF)
    {
        hb_error("colorspace: input color space undefined");
        return -1;
    }

    hb_dict_t        * settings = filter->settings;

    char * range = NULL;
    char * primaries = NULL, * transfer = NULL, * matrix = NULL;
    char * tonemap = NULL;
    double param = 0, desat = 0, npl = 100;

    hb_dict_extract_string(&range, settings, "range");
    hb_dict_extract_string(&primaries, settings, "primaries");
    hb_dict_extract_string(&transfer, settings, "transfer");
    hb_dict_extract_string(&matrix, settings, "matrix");
    hb_dict_extract_string(&tonemap, settings, "tonemap");
    hb_dict_extract_double(&param, settings, "param");
    hb_dict_extract_double(&desat, settings, "desat");
    hb_dict_extract_double(&npl, settings, "npl");

    if (!(range || primaries || transfer || matrix))
    {
        return 0;
    }

    int color_prim, color_transfer, color_matrix, color_range;

    color_prim = init->color_prim;
    color_transfer = init->color_transfer;
    color_matrix = init->color_matrix;
    color_range = init->color_range;

    if (primaries)
    {
        color_prim = av_color_primaries_from_name(primaries);
        free(primaries);
    }
    if (transfer)
    {
        color_transfer = av_color_transfer_from_name(transfer);
        free(transfer);
    }
    if (matrix)
    {
        color_matrix = av_color_space_from_name(matrix);
        free(matrix);
    }
    if (range)
    {
        color_range = av_color_range_from_name(range);
        free(range);
    }

    if (color_prim == init->color_prim && color_transfer == init->color_transfer &&
        color_matrix == init->color_matrix && color_range == init->color_range)
    {
        return 0;
    }

    hb_value_array_t * avfilters = hb_value_array_init();
    hb_dict_t * avfilter   = NULL;
    hb_dict_t * avsettings = NULL;

    if (transfer && init->color_transfer != color_transfer &&
        (init->color_transfer == HB_COLR_TRA_SMPTEST2084 || init->color_transfer == HB_COLR_TRA_ARIB_STD_B67))
    {
        // Zscale
        avfilter   = hb_dict_init();
        avsettings = hb_dict_init();

        hb_dict_set_string(avsettings, "transfer", "linear");
        hb_dict_set_double(avsettings, "npl", npl);
        hb_dict_set(avfilter, "zscale", avsettings);

        hb_value_array_append(avfilters, avfilter);

        // Format
        avfilter   = hb_dict_init();
        avsettings = hb_dict_init();

        hb_dict_set_string(avsettings, "pix_fmts", "gbrpf32le");
        hb_dict_set(avfilter, "format", avsettings);

        hb_value_array_append(avfilters, avfilter);

        // Tonemap
        avfilter   = hb_dict_init();
        avsettings = hb_dict_init();

        const char * tonemap_in = tonemap != NULL ? tonemap : "hable";

        hb_dict_set_string(avsettings, "tonemap", tonemap_in);
        if (strcmp(tonemap_in, "hable") && strcmp(tonemap_in, "none") && param != 0)
        {
            hb_dict_set_double(avsettings, "param", param);
        }
        hb_dict_set_double(avsettings, "desat", desat);
        // FIXME: this could be automated by passing through side data.
        double peak = determine_signal_peak(init);
        hb_dict_set_double(avsettings, "peak", peak);
        hb_dict_set(avfilter, "tonemap", avsettings);

        hb_value_array_append(avfilters, avfilter);
    }

    if (tonemap)
    {
        free(tonemap);
    }

    // Zscale
    avfilter   = hb_dict_init();
    avsettings = hb_dict_init();

    hb_dict_set_int(avsettings, "primaries", color_prim);
    hb_dict_set_int(avsettings, "transfer", color_transfer);
    hb_dict_set_int(avsettings, "matrix", color_matrix);
    hb_dict_set_string(avsettings, "range", av_color_range_name(color_range));

    hb_dict_set(avfilter, "zscale", avsettings);
    hb_value_array_append(avfilters, avfilter);

    // Format
    avfilter   = hb_dict_init();
    avsettings = hb_dict_init();

    hb_dict_set_string(avsettings, "pix_fmts", av_get_pix_fmt_name(init->pix_fmt));
    hb_dict_set(avfilter, "format", avsettings);

    hb_value_array_append(avfilters, avfilter);

    pv->avfilters = avfilters;

    init->color_prim = color_prim;
    init->color_transfer = color_transfer;
    init->color_matrix = color_matrix;
    init->color_range = color_range;

    pv->output = *init;

    return 0;
}
