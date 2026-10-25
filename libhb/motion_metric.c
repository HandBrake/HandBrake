/* motionmetric.c

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"

struct hb_motion_metric_private_s
{
    unsigned *gamma_lut;
    int       depth;
    int       bps;
    int       max_value;
    int       sample_shift;

    float (*motion_metric)(hb_motion_metric_private_t *pv,
                           int width, int height,
                           int stride_a, int stride_b,
                           const uint8_t *buf_a, const uint8_t *buf_b);
};

// Create gamma lookup table.
// Note that we are creating a scaled integer lookup table that will
// not cause overflows in sse_block16() below. This results in
// small values being truncated to 0 which is ok for this usage.
static void build_gamma_lut(hb_motion_metric_private_t *pv)
{
    for (int i = 0; i <= pv->max_value; i++)
    {
        pv->gamma_lut[i] = 4095 * pow(((float)i / (float)(pv->max_value -1)), 2.2f);;
    }
}

// Compute the sum of squared errors for a 16x16 block
// Gamma adjusts pixel values so that less visible differences
// count less.

#define DEF_SSE_BLOCK16(nbits, suffix, shift)                                          \
static inline unsigned sse_block16##_##nbits##suffix(                                  \
                       hb_motion_metric_private_t *pv,                                 \
                       const uint##nbits##_t *a, const uint##nbits##_t *b,             \
                       int stride_a, int stride_b)                                     \
{                                                                                      \
    unsigned sum = 0;                                                                  \
    for (int y = 0; y < 16; y++)                                                       \
    {                                                                                  \
        for (int x = 0; x < 16; x++)                                                   \
        {                                                                              \
            int diff = pv->gamma_lut[a[x] >> (shift)] - pv->gamma_lut[b[x] >> (shift)];\
            sum += diff * diff;                                                        \
        }                                                                              \
        a += stride_a;                                                                 \
        b += stride_b;                                                                 \
    }                                                                                  \
    return sum;                                                                        \
}                                                                                      \

DEF_SSE_BLOCK16(8, , 0)
DEF_SSE_BLOCK16(16, , 0)
DEF_SSE_BLOCK16(16, _shift, pv->sample_shift)

// Sum of squared errors.  Computes and sums the SSEs for all
// 16x16 blocks in the images.  Only checks the Y component.
#define DEF_MOTION_METRIC(nbits, suffix)                                                    \
static float motion_metric##_##nbits##suffix(hb_motion_metric_private_t *pv,                \
                                     int width, int height,                                 \
                                     int stride_a, int stride_b,                            \
                                     const uint8_t *a, const uint8_t *b)                    \
{                                                                                           \
                                                                                            \
    int bw, bh;                                                                             \
    uint##nbits##_t *buf_a, *buf_b;                                                         \
                                                                                            \
    buf_a     = (uint##nbits##_t *)a;                                                       \
    buf_b     = (uint##nbits##_t *)b;                                                       \
    bw        = width / 16;                                                                 \
    bh        = height / 16;                                                                \
    stride_a /= pv->bps;                                                                    \
    stride_b /= pv->bps;                                                                    \
                                                                                            \
    uint64_t sum = 0;                                                                       \
    for (int y = 0; y < bh; y++)                                                            \
    {                                                                                       \
        for (int x = 0; x < bw; x++)                                                        \
        {                                                                                   \
            sum += sse_block16##_##nbits##suffix(pv,                                        \
                        buf_a + y * 16 * stride_a + x * 16,                                 \
                        buf_b + y * 16 * stride_b + x * 16,                                 \
                        stride_a, stride_b);                                                \
        }                                                                                   \
    }                                                                                       \
    return (float)sum / (width * height);                                                   \
}                                                                                           \

DEF_MOTION_METRIC(8, )
DEF_MOTION_METRIC(16, )
DEF_MOTION_METRIC(16, _shift)

static int hb_motion_metric_init(hb_motion_metric_object_t *metric,
                                 hb_filter_init_t *init)
{
    metric->private_data = calloc(1, sizeof(struct hb_motion_metric_private_s));
    if (metric->private_data == NULL)
    {
        hb_error("motion_metric: calloc failed");
        return -1;
    }
    hb_motion_metric_private_t *pv = metric->private_data;

    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(init->pix_fmt);
    pv->depth     = desc->comp[0].depth;
    pv->bps       = pv->depth > 8 ? 2 : 1;
    pv->max_value = (1 << pv->depth) - 1;
    pv->sample_shift = desc->comp[0].shift;

    pv->gamma_lut = malloc(sizeof(unsigned) * (pv->max_value + 1));
    if (pv->gamma_lut == NULL)
    {
        hb_error("motion_metric: malloc failed");
        return -1;
    }
    build_gamma_lut(pv);

    switch (pv->depth)
    {
        case 8:
            pv->motion_metric = motion_metric_8;
            break;
        default:
            pv->motion_metric = pv->sample_shift ? motion_metric_16_shift
                                                 : motion_metric_16;
    }

    return 0;
}

static float hb_motion_metric_work(hb_motion_metric_object_t *metric,
                                   hb_buffer_t *buf_a,
                                   hb_buffer_t *buf_b)
{
    hb_motion_metric_private_t *pv = metric->private_data;

    return pv->motion_metric(metric->private_data,
                             buf_a->f.width, buf_a->f.height,
                             buf_a->plane[0].stride, buf_b->plane[0].stride,
                             buf_a->plane[0].data, buf_b->plane[0].data);
}

static void hb_motion_metric_close(hb_motion_metric_object_t *metric)
{
    hb_motion_metric_private_t *pv = metric->private_data;

    if (pv == NULL)
    {
        return;
    }

    free(pv->gamma_lut);
    free(pv);
}

hb_motion_metric_object_t hb_motion_metric =
{
    .name  = "Motion metric",
    .init  = hb_motion_metric_init,
    .work  = hb_motion_metric_work,
    .close = hb_motion_metric_close,
};
