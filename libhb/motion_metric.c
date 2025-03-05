/* motionmetric.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"

#if defined (__aarch64__) && !defined(__APPLE__)
    #include <arm_neon.h>
#endif
struct hb_motion_metric_private_s
{
    unsigned *gamma_lut;
    int       depth;
    int       bps;
    int       max_value;
    int       use_approximation;
    uint16_t *approx_buf_a;
    uint16_t *approx_buf_b;
};

static int hb_motion_metric_init(hb_motion_metric_object_t *metric,
                                 hb_filter_init_t *init);

static float hb_motion_metric_work(hb_motion_metric_object_t *metric,
                                   hb_buffer_t *buf_a,
                                   hb_buffer_t *buf_b);

static void hb_motion_metric_close(hb_motion_metric_object_t *metric);

hb_motion_metric_object_t hb_motion_metric =
{
    .name  = "Motion metric",
    .init  = hb_motion_metric_init,
    .work  = hb_motion_metric_work,
    .close = hb_motion_metric_close,
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

#define APPROX(a, b, c, d) ((((a + b + 1) >> 1) + ((c + d + 1) >> 1) + 1) >> 1)
#define APPROX_FRAME_DATA(nbits)                                                                                \
static void approximate_frame_data##_##nbits(const uint##nbits##_t* source, uint##nbits##_t* dest,              \
    int source_stride, int dest_stride, int width, int height)                                                  \
{                                                                                                               \
    for (int ii = 0; ii < height; ii++)                                                                         \
    {                                                                                                           \
        const uint##nbits##_t* approx_buf = source + source_stride;                                             \
        for (int jj = 0; jj < width; jj++)                                                                      \
        {                                                                                                       \
            dest[jj] = APPROX(source[2 * jj], approx_buf[2 * jj], source[2 * jj + 1], approx_buf[2 * jj + 1]);  \
        }                                                                                                       \
        source += source_stride * 2;                                                                            \
        dest += dest_stride;                                                                                    \
    }                                                                                                           \
}                                                                                                               \

APPROX_FRAME_DATA(8)
APPROX_FRAME_DATA(16)

// Compute the sum of squared errors for a 16x16 block
// Gamma adjusts pixel values so that less visible differences
// count less.
#if defined (__aarch64__) && !defined(__APPLE__)
static
float motion_metric_neon_8(hb_motion_metric_private_t *pv, hb_buffer_t *a, hb_buffer_t *b)
{
    int width, height, bw, bh;
    int stride_a, stride_b;
    uint8_t *buf_a, *buf_b;

    if( pv->use_approximation == 1 )
    {
        // Proceed with the motion metric computation on downscaled buffers
        width    = a->f.width / 4;
        height   = a->f.height / 4;
        stride_a = width;
        stride_b = width;

        buf_a = (uint8_t *)pv->approx_buf_a;
        buf_b = (uint8_t *)pv->approx_buf_b;

        approximate_frame_data_8((const uint8_t *)a->plane[0].data, buf_a,
            a->plane[0].stride, stride_a, width, height);
        approximate_frame_data_8((const uint8_t *)b->plane[0].data, buf_b,
            b->plane[0].stride, stride_b, width, height);

        bw = width / 16;
        bh = height / 16;

    }
    else
    {
        width    = a->f.width;
        height   = a->f.height;
        bw       = a->f.width / 16;
        bh       = a->f.height / 16;
        stride_a = a->plane[0].stride;
        stride_b = b->plane[0].stride;
        buf_a    = (uint8_t *)a->plane[0].data;
        buf_b    = (uint8_t *)b->plane[0].data;
    }

    uint64_t sum = 0;
    for (int y = 0; y < bh; y++)
    {
        for (int x = 0; x < bw; x++)
        {
            const uint8_t *ra = buf_a + y * 16 * stride_a + x * 16;
            const uint8_t *rb = buf_b + y * 16 * stride_b + x * 16;

            for (int yy = 0; yy < 16; yy++)
            {
                uint32_t arrga[16];
                uint32_t arrgb[16];

                for (int xx = 0; xx < 16; xx++)
                {
                    arrga[xx] = pv->gamma_lut[ra[xx]];
                    arrgb[xx] = pv->gamma_lut[rb[xx]];
                }

                uint32x4_t vga0 = vld1q_u32(arrga);
                uint32x4_t vga1 = vld1q_u32(arrga + 4);
                uint32x4_t vga2 = vld1q_u32(arrga + 8);
                uint32x4_t vga3 = vld1q_u32(arrga + 12);

                uint32x4_t vgb0 = vld1q_u32(arrgb);
                uint32x4_t vgb1 = vld1q_u32(arrgb + 4);
                uint32x4_t vgb2 = vld1q_u32(arrgb + 8);
                uint32x4_t vgb3 = vld1q_u32(arrgb + 12);
                uint32x4_t vdf0 = vsubq_u32(vga0, vgb0);
                uint32x4_t vdf1 = vsubq_u32(vga1, vgb1);
                uint32x4_t vdf2 = vsubq_u32(vga2, vgb2);
                uint32x4_t vdf3 = vsubq_u32(vga3, vgb3);

                uint32x4_t vsq0 = vmulq_u32(vdf0, vdf0);
                uint32x4_t vsq1 = vmulq_u32(vdf1, vdf1);
                uint32x4_t vsq2 = vmulq_u32(vdf2, vdf2);
                uint32x4_t vsq3 = vmulq_u32(vdf3, vdf3);

                sum += vaddvq_u32(vsq0);
                sum += vaddvq_u32(vsq1);
                sum += vaddvq_u32(vsq2);
                sum += vaddvq_u32(vsq3);

                ra += stride_a;
                rb += stride_b;
            }
        }
    }

    return (float)sum / (width * height);
}
#endif

#define DEF_SSE_BLOCK16(nbits)                                                         \
static inline unsigned sse_block16##_##nbits(unsigned *gamma_lut,                      \
                                   const uint##nbits##_t *a, const uint##nbits##_t *b, \
                                   int stride_a, int stride_b)                         \
{                                                                                      \
    unsigned sum = 0;                                                                  \
    for (int y = 0; y < 16; y++)                                                       \
    {                                                                                  \
        for (int x = 0; x < 16; x++)                                                   \
        {                                                                              \
            int diff = gamma_lut[a[x]] - gamma_lut[b[x]];                              \
            sum += diff * diff;                                                        \
        }                                                                              \
        a += stride_a;                                                                 \
        b += stride_b;                                                                 \
    }                                                                                  \
    return sum;                                                                        \
}                                                                                      \

#if !(defined (__aarch64__) && !defined(__APPLE__))
DEF_SSE_BLOCK16(8)
#endif

DEF_SSE_BLOCK16(16)

// Sum of squared errors.  Computes and sums the SSEs for all
// 16x16 blocks in the images.  Only checks the Y component.
#define DEF_MOTION_METRIC(nbits)                                                                \
static float motion_metric##_##nbits(hb_motion_metric_private_t *pv,                            \
                                     hb_buffer_t *a, hb_buffer_t *b)                            \
{                                                                                               \
                                                                                                \
    int width, height, bw, bh;                                                                  \
    int stride_a, stride_b;                                                                     \
    uint##nbits##_t *buf_a, *buf_b;                                                             \
                                                                                                \
    if( pv->use_approximation == 1)                                                             \
    {                                                                                           \
        width = a->f.width / 4;                                                                 \
        height = a->f.height / 4;                                                               \
        stride_a = width;                                                                       \
        stride_b = width;                                                                       \
        buf_a = ( uint##nbits##_t *)pv->approx_buf_a;                                           \
        buf_b = ( uint##nbits##_t *)pv->approx_buf_b;                                           \
                                                                                                \
        approximate_frame_data##_##nbits(                                                       \
            (const uint##nbits##_t *)a->plane[0].data, buf_a,                                   \
            a->plane[0].stride / pv->bps, stride_a, width, height);                             \
        approximate_frame_data##_##nbits(                                                       \
            (const uint##nbits##_t *)b->plane[0].data, buf_b,                                   \
            b->plane[0].stride / pv->bps, stride_b, width, height);                             \
        bw = width / 16;                                                                        \
        bh = height / 16;                                                                       \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
        width    = a->f.width;                                                                  \
        height   = a->f.height;                                                                 \
        stride_a = a->plane[0].stride / pv->bps;                                                \
        stride_b = b->plane[0].stride / pv->bps;                                                \
        buf_a    = (uint##nbits##_t *)a->plane[0].data;                                         \
        buf_b    = (uint##nbits##_t *)b->plane[0].data;                                         \
        bw       = width / 16;                                                                  \
        bh       = height / 16;                                                                 \
    }                                                                                           \
                                                                                                \
    uint64_t sum = 0;                                                                           \
    for (int y = 0; y < bh; y++)                                                                \
    {                                                                                           \
        for (int x = 0; x < bw; x++)                                                            \
        {                                                                                       \
            sum += sse_block16##_##nbits(pv->gamma_lut,                                         \
                        buf_a + y * 16 * stride_a + x * 16,                                     \
                        buf_b + y * 16 * stride_b + x * 16,                                     \
                        stride_a, stride_b);                                                    \
        }                                                                                       \
    }                                                                                           \
    return (float)sum / (width * height);                                                       \
}                                                                                               \

#if !(defined (__aarch64__) && !defined(__APPLE__))
DEF_MOTION_METRIC(8)
#endif

DEF_MOTION_METRIC(16)

static int hb_motion_metric_init(hb_motion_metric_object_t *metric,
                                 hb_filter_init_t *init)
{
    metric->private_data = calloc(sizeof(struct hb_motion_metric_private_s), 1);
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

    pv->gamma_lut = malloc(sizeof(unsigned) * (pv->max_value + 1));
    if (pv->gamma_lut == NULL)
    {
        hb_error("motion_metric: malloc failed");
        return -1;
    }
    build_gamma_lut(pv);

    if(init->geometry.width >= 1920 || init->geometry.height >= 1080)
    {
        pv->use_approximation = 1;
        int approx_height = init->geometry.height / 4;
        int approx_width = init->geometry.width / 4;
        pv->approx_buf_a = malloc(approx_height * approx_width * sizeof(uint16_t));
        pv->approx_buf_b = malloc(approx_height * approx_width * sizeof(uint16_t));
        if (pv->approx_buf_a == NULL || pv->approx_buf_b == NULL)
        {
            hb_error("motion_metric: malloc failed");
            return -1;
        }
    }
    else
    {
        pv->use_approximation = 0;
    }

    return 0;
}

static float hb_motion_metric_work(hb_motion_metric_object_t *metric,
                                   hb_buffer_t *buf_a,
                                   hb_buffer_t *buf_b)
{
    hb_motion_metric_private_t *pv = metric->private_data;

    switch (pv->depth)
    {
        case 8:
#if defined (__aarch64__) && !defined(__APPLE__)
            return motion_metric_neon_8(metric->private_data, buf_a, buf_b);
#else
            return motion_metric_8(metric->private_data, buf_a, buf_b);
#endif
        default:
            return motion_metric_16(metric->private_data, buf_a, buf_b);
    }
}

static void hb_motion_metric_close(hb_motion_metric_object_t *metric)
{
    hb_motion_metric_private_t *pv = metric->private_data;

    if (pv == NULL)
    {
        return;
    }

    free(pv->gamma_lut);
    free(pv->approx_buf_a);
    free(pv->approx_buf_b);
    free(pv);
}