/*
 Copyright (c) 2003 Daniel Moreno <comac AT comac DOT darktech DOT org>
 Copyright (c) 2010 Baptiste Coudurier
 Copyright (c) 2012 Loren Merritt

 ported from FFmpeg.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "libavutil/intreadwrite.h"

#define HQDN3D_SPATIAL_LUMA_DEFAULT    4.0f
#define HQDN3D_SPATIAL_CHROMA_DEFAULT  3.0f
#define HQDN3D_TEMPORAL_LUMA_DEFAULT   6.0f

#define LUT_BITS (depth==16 ? 8 : 4)
#define LOAD(x) (((depth == 8 ? frame_src[x] : AV_RN16A(frame_src + (x) * 2)) << (16 - depth))\
                 + (((1 << (16 - depth)) - 1) >> 1))
#define STORE(x,val) (depth == 8 ? frame_dst[x] = (val) >> (16 - depth) : \
                                   AV_WN16A(frame_dst + (x) * 2, (val) >> (16 - depth)))

struct hb_filter_private_s
{
    int16_t  *hqdn3d_coef[6];
    uint16_t *hqdn3d_line;
    uint16_t *hqdn3d_frame[3];

    int hsub, vsub;
    int depth;

    hb_filter_init_t input;
    hb_filter_init_t output;
};

static int hb_denoise_init(hb_filter_object_t *filter,
                           hb_filter_init_t *init );

static int hb_denoise_work(hb_filter_object_t *filter,
                           hb_buffer_t **buf_in,
                           hb_buffer_t **buf_out);

static void hb_denoise_close(hb_filter_object_t *filter);

static const char denoise_template[] =
    "y-spatial=^"HB_FLOAT_REG"$:cb-spatial=^"HB_FLOAT_REG"$:"
    "cr-spatial=^"HB_FLOAT_REG"$:"
    "y-temporal=^"HB_FLOAT_REG"$:cb-temporal=^"HB_FLOAT_REG"$:"
    "cr-temporal=^"HB_FLOAT_REG"$";

hb_filter_object_t hb_filter_denoise =
{
    .id                = HB_FILTER_DENOISE,
    .enforce_order     = 1,
    .name              = "Denoise (hqdn3d)",
    .settings          = NULL,
    .init              = hb_denoise_init,
    .work              = hb_denoise_work,
    .close             = hb_denoise_close,
    .settings_template = denoise_template,
};

static void hqdn3d_precalc_coef(int16_t *ct, int depth, double dist25)
{
    int i;
    double gamma, simil, C;

    gamma = log(0.25) / log(1.0 - FFMIN(dist25,252.0)/255.0 - 0.00001);

    for (i = -(256<<LUT_BITS); i < 256<<LUT_BITS; i++)
    {
        double f = ((i<<(9-LUT_BITS)) + (1<<(8-LUT_BITS)) - 1) / 512.0; // midpoint of the bin
        simil = FFMAX(0, 1.0 - fabs(f) / 255.0);
        C = pow(simil, gamma) * 256.0 * f;
        ct[(256<<LUT_BITS)+i] = lrint(C);
    }

    ct[0] = !!dist25;
}

static inline unsigned int hqdn3d_lowpass_mul(int prev_mul, int curr_mul, int16_t *coef, int depth)
{
    int d = (prev_mul - curr_mul) >> (8 - LUT_BITS);
    return curr_mul + coef[d];
}

static void hqdn3d_denoise_temporal(uint8_t *frame_src, uint8_t *frame_dst,
                                    uint16_t *frame_ant,
                                    int w, int h, int sstride, int dstride,
                                    int16_t *temporal, int depth)
{
    long x, y;
    uint32_t tmp;

    temporal += 256 << LUT_BITS;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            frame_ant[x] = tmp = hqdn3d_lowpass_mul(frame_ant[x], LOAD(x), temporal, depth);
            STORE(x, tmp);
        }

        frame_src += sstride;
        frame_dst += dstride;
        frame_ant += w;
    }
}

static void hqdn3d_denoise_spatial(uint8_t *frame_src, uint8_t *frame_dst,
                                   uint16_t *line_ant, uint16_t *frame_ant,
                                   int w, int h, int sstride, int dstride,
                                   int16_t *spatial, int16_t *temporal, int depth)
{
    long x, y;
    uint32_t pixel_ant;
    uint32_t tmp;

    spatial  += 256 << LUT_BITS;
    temporal += 256 << LUT_BITS;

    /* First line has no top neighbor. Only left one for each tmp and last frame */
    pixel_ant = LOAD(0);
    for (x = 0; x < w; x++)
    {
        line_ant[x] = tmp = pixel_ant = hqdn3d_lowpass_mul(pixel_ant, LOAD(x), spatial, depth);
        frame_ant[x] = tmp = hqdn3d_lowpass_mul(frame_ant[x], tmp, temporal, depth);
        STORE(x, tmp);
    }

    for (y = 1; y < h; y++)
    {
        frame_src += sstride;
        frame_dst += dstride;
        frame_ant += w;
        pixel_ant = LOAD(0);

        for (x = 0; x < w-1; x++)
        {
            line_ant[x] = tmp =  hqdn3d_lowpass_mul(line_ant[x], pixel_ant, spatial, depth);
            pixel_ant =          hqdn3d_lowpass_mul(pixel_ant, LOAD(x+1), spatial, depth);
            frame_ant[x] = tmp = hqdn3d_lowpass_mul(frame_ant[x], tmp, temporal, depth);
            STORE(x, tmp);
        }
        line_ant[x] = tmp =  hqdn3d_lowpass_mul(line_ant[x], pixel_ant, spatial, depth);
        frame_ant[x] = tmp = hqdn3d_lowpass_mul(frame_ant[x], tmp, temporal, depth);
        STORE(x, tmp);
    }
}

static void hqdn3d_denoise_depth(uint8_t *frame_src, uint8_t *frame_dst,
                                 uint16_t *line_ant, uint16_t **frame_ant_ptr,
                                 int w, int h, int sstride, int dstride,
                                 int16_t *spatial, int16_t *temporal, int depth)
{
    long x, y;
    uint16_t *frame_ant = (*frame_ant_ptr);

    if (!frame_ant)
    {
        uint8_t *src = frame_src;
        (*frame_ant_ptr) = frame_ant = malloc(w*h*sizeof(uint16_t));
        for (y = 0; y < h; y++, frame_src += sstride, frame_ant += w)
        {
            for (x = 0; x < w; x++)
            {
                frame_ant[x] = LOAD(x);
            }
        }
        frame_src = src;
        frame_ant = *frame_ant_ptr;
    }

    /* If no spatial coefficients, do temporal denoise only */
    if (spatial[0])
    {
        hqdn3d_denoise_spatial(frame_src, frame_dst, line_ant, frame_ant,
                               w, h, sstride, dstride, spatial, temporal, depth);
    }
    else
    {
        hqdn3d_denoise_temporal(frame_src, frame_dst, frame_ant,
                                w, h, sstride, dstride, temporal, depth);
    }
}

#define hqdn3d_denoise(...)                                             \
        switch (pv->depth) {                                            \
            case  8: hqdn3d_denoise_depth(__VA_ARGS__,  8); break;      \
            case  9: hqdn3d_denoise_depth(__VA_ARGS__,  9); break;      \
            case 10: hqdn3d_denoise_depth(__VA_ARGS__, 10); break;      \
            case 12: hqdn3d_denoise_depth(__VA_ARGS__, 12); break;      \
            case 14: hqdn3d_denoise_depth(__VA_ARGS__, 14); break;      \
            case 16: hqdn3d_denoise_depth(__VA_ARGS__, 16); break;      \
        }                                                               \


static int hb_denoise_init( hb_filter_object_t * filter,
                            hb_filter_init_t * init )
{
    filter->private_data = calloc( sizeof(struct hb_filter_private_s), 1 );
    hb_filter_private_t * pv = filter->private_data;

    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(init->pix_fmt);
    int i, depth;

    pv->hsub  = desc->log2_chroma_w;
    pv->vsub  = desc->log2_chroma_h;
    pv->depth = depth = desc->comp[0].depth;

    double spatial_luma, spatial_chroma_b, spatial_chroma_r;
    double temporal_luma, temporal_chroma_b, temporal_chroma_r;

    pv->input = *init;
    if (!hb_dict_extract_double(&spatial_luma, filter->settings, "y-spatial"))
    {
        spatial_luma      = HQDN3D_SPATIAL_LUMA_DEFAULT;
    }
    if (!hb_dict_extract_double(&spatial_chroma_b, filter->settings, "cb-spatial"))
    {
        spatial_chroma_b  = HQDN3D_SPATIAL_CHROMA_DEFAULT *
                            spatial_luma / HQDN3D_SPATIAL_LUMA_DEFAULT;
    }
    if (!hb_dict_extract_double(&spatial_chroma_r, filter->settings, "cr-spatial"))
    {
        spatial_chroma_r  = spatial_chroma_b;
    }
    if (!hb_dict_extract_double(&temporal_luma, filter->settings, "y-temporal"))
    {
        temporal_luma     = HQDN3D_TEMPORAL_LUMA_DEFAULT *
                            spatial_luma / HQDN3D_SPATIAL_LUMA_DEFAULT;
    }
    if (!hb_dict_extract_double(&temporal_chroma_b, filter->settings, "cb-temporal"))
    {
        temporal_chroma_b = temporal_luma * spatial_chroma_b / spatial_luma;
    }
    if (!hb_dict_extract_double(&temporal_chroma_r, filter->settings, "cr-temporal"))
    {
        temporal_chroma_r = temporal_chroma_b;
    }

    for (i = 0; i < 6; i++)
    {
        pv->hqdn3d_coef[i] = av_malloc((512<<LUT_BITS) * sizeof(int16_t));
        if (!pv->hqdn3d_coef[i])
        {
            return 0;
        }
    }

    hqdn3d_precalc_coef(pv->hqdn3d_coef[0], pv->depth, spatial_luma);
    hqdn3d_precalc_coef(pv->hqdn3d_coef[1], pv->depth, temporal_luma);
    hqdn3d_precalc_coef(pv->hqdn3d_coef[2], pv->depth, spatial_chroma_b);
    hqdn3d_precalc_coef(pv->hqdn3d_coef[3], pv->depth, temporal_chroma_b);
    hqdn3d_precalc_coef(pv->hqdn3d_coef[4], pv->depth, spatial_chroma_r);
    hqdn3d_precalc_coef(pv->hqdn3d_coef[5], pv->depth, temporal_chroma_r);

    pv->output = *init;

    return 0;
}

static void hb_denoise_close(hb_filter_object_t * filter)
{
    hb_filter_private_t *pv = filter->private_data;
    int i;

    if (!pv)
    {
        return;
    }

    for (i = 0; i < 6; i++)
    {
        av_freep(&pv->hqdn3d_coef[i]);
    }

	if (pv->hqdn3d_line)
    {
        free(pv->hqdn3d_line);
        pv->hqdn3d_line = NULL;
    }
	if (pv->hqdn3d_frame[0])
    {
        free(pv->hqdn3d_frame[0]);
        pv->hqdn3d_frame[0] = NULL;
    }
	if (pv->hqdn3d_frame[1])
    {
        free(pv->hqdn3d_frame[1]);
        pv->hqdn3d_frame[1] = NULL;
    }
	if (pv->hqdn3d_frame[2])
    {
        free(pv->hqdn3d_frame[2]);
        pv->hqdn3d_frame[2] = NULL;
    }

    free(pv);
    filter->private_data = NULL;
}

static int hb_denoise_work(hb_filter_object_t *filter,
                           hb_buffer_t **buf_in,
                           hb_buffer_t **buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in, *out;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    out = hb_frame_buffer_init(pv->output.pix_fmt, in->f.width, in->f.height);
    out->f.color_prim      = pv->output.color_prim;
    out->f.color_transfer  = pv->output.color_transfer;
    out->f.color_matrix    = pv->output.color_matrix;
    out->f.color_range     = pv->output.color_range;
    out->f.chroma_location = pv->output.chroma_location;

    if (!pv->hqdn3d_line)
    {
        pv->hqdn3d_line = malloc(in->plane[0].stride * sizeof(uint16_t));
    }

    int c, coef_index;

    for (c = 0; c < 3; c++)
    {
        coef_index = c * 2;
        hqdn3d_denoise(in->plane[c].data,
                       out->plane[c].data,
                       pv->hqdn3d_line,
                       &pv->hqdn3d_frame[c],
                       AV_CEIL_RSHIFT(in->f.width, (!!c * pv->hsub)),
                       AV_CEIL_RSHIFT(in->f.height, (!!c * pv->vsub)),
                       in->plane[c].stride,
                       out->plane[c].stride,
                       pv->hqdn3d_coef[coef_index],
                       pv->hqdn3d_coef[coef_index+1]);
    }

    hb_buffer_copy_props(out, in);
    *buf_out = out;

    return HB_FILTER_OK;
}
