/* deskeeter.c

   Copyright (c) 2003-2019 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"

#define DESKEETER_STRENGTH_LUMA_DEFAULT   0.75
#define DESKEETER_STRENGTH_CHROMA_DEFAULT 0.75

#define DESKEETER_KERNELS 4
#define DESKEETER_EDGE_KERNELS 1
#define DESKEETER_KERNEL_LUMA_DEFAULT   3
#define DESKEETER_KERNEL_CHROMA_DEFAULT 3

#define DESKEETER_SORT(a,b) { if (a > b) DESKEETER_SWAP(a, b); }
#define DESKEETER_SWAP(a,b) { a = (a ^ b); b = (a ^ b); a = (b ^ a); }

typedef struct
{
    double strength;  // strength
    int    kernel;    // which kernel to use; kernels[kernel]
    int    edge_kernel;
    double edge_thresh_strong;
    double edge_thresh_weak;
    int    show_edge;
} deskeeter_plane_context_t;

typedef struct {
    const int   *mem;
    const int    size;
    const double coef;
} kernel_t;

// 4-neighbor Laplacian kernel (lap)
// Effective on vertical and horizontal edges, less effective on diagonals
// size = 3, coef = 1.0
static const int kernel_lap[] =
{
 0, -1,  0,
-1,  4, -1,
 0, -1,  0
};

// Isotropic Laplacian kernel (isolap)
// Minimial directionality, affects all edges similarly
// size = 3, coef = 1.0 / 5
static const int kernel_isolap[] =
{
-1, -4, -1,
-4, 20, -4,
-1, -4, -1
};

// Laplacian of Gaussian kernel (log)
// Slight noise and grain rejection
// σ ~= 1
// size = 5, coef = 1.0 / 5
static const int kernel_log[] =
{
 0,  0, -1,  0,  0,
 0, -1, -2, -1,  0,
-1, -2, 16, -2, -1,
 0, -1, -2, -1,  0,
 0,  0, -1,  0,  0
};

// Isotropic Laplacian of Gaussian kernel (isolog)
// Minimial directionality, plus noise and grain rejection
// σ ~= 1.2
// size = 5, coef = 1.0 / 10
static const int kernel_isolog[] =
{
 0, -1, -1, -1,  0,
-1, -3, -4, -3, -1,
-1, -4, 54, -4, -1,
-1, -3, -4, -3, -1,
 0, -1, -1, -1,  0
};

static const int kernel_gauss[] =
{
  2,  4,  5,  4,  2,
  4,  9, 12,  9,  4,
  5, 12, 15, 12,  5,
  4,  9, 12,  9,  4,
  2,  4,  5,  4,  2,
};

static kernel_t kernels[] =
{
    { kernel_lap,    3, 1.0 /  5  },
    { kernel_isolap, 3, 1.0 / 25  },
    { kernel_log,    5, 1.0 / 25  },
    { kernel_isolog, 5, 1.0 / 65  },
    { kernel_gauss,  5, 1.0 / 159 },
};

static kernel_t edge_kernels[] =
{
    { kernel_gauss,  5, 1.0 / 159 },
};

#define KERNEL_LAP      0
#define KERNEL_ISOLAP   1
#define KERNEL_LOG      2
#define KERNEL_ISOLOG   3

#define KERNEL_EDGE_GAUSS   0

#define EDGE_STRONG_DEFAULT 20
#define EDGE_WEAK_DEFAULT   16

struct hb_filter_private_s
{
    deskeeter_plane_context_t plane_ctx[3];
};

static int hb_deskeeter_init(hb_filter_object_t *filter,
                             hb_filter_init_t   *init);

static int hb_deskeeter_work(hb_filter_object_t *filter,
                             hb_buffer_t ** buf_in,
                             hb_buffer_t ** buf_out);

static void hb_deskeeter_close(hb_filter_object_t *filter);

static const char hb_deskeeter_template[] =
    "y-strength=^"HB_FLOAT_REG"$:y-kernel=^"HB_ALL_REG"$:"
    "cb-strength=^"HB_FLOAT_REG"$:cb-kernel=^"HB_ALL_REG"$:"
    "cr-strength=^"HB_FLOAT_REG"$:cr-kernel=^"HB_ALL_REG"$:"
    "edge-kernel=^"HB_ALL_REG"$:edge-show=^"HB_BOOL_REG"$:"
    "edge-thresh-strong=^"HB_FLOAT_REG"$:edge-thresh-weak=^"HB_FLOAT_REG"$";

hb_filter_object_t hb_filter_deskeeter =
{
    .id                = HB_FILTER_DESKEETER,
    .enforce_order     = 1,
    .name              = "Demosquito (deskeeter)",
    .settings          = NULL,
    .init              = hb_deskeeter_init,
    .work              = hb_deskeeter_work,
    .close             = hb_deskeeter_close,
    .settings_template = hb_deskeeter_template,
};

static uint8_t deskeeter_filter_median_opt(uint8_t  *pixels,
                                           const int size)
{
    // Optimized sorting networks
    if (size == 3)
    {
        /* opt_med9() via Nicolas Devillard
         * http://ndevilla.free.fr/median/median.pdf
         */
        DESKEETER_SORT(pixels[1], pixels[2]); DESKEETER_SORT(pixels[4], pixels[5]); DESKEETER_SORT(pixels[7], pixels[8]);
        DESKEETER_SORT(pixels[0], pixels[1]); DESKEETER_SORT(pixels[3], pixels[4]); DESKEETER_SORT(pixels[6], pixels[7]);
        DESKEETER_SORT(pixels[1], pixels[2]); DESKEETER_SORT(pixels[4], pixels[5]); DESKEETER_SORT(pixels[7], pixels[8]);
        DESKEETER_SORT(pixels[0], pixels[3]); DESKEETER_SORT(pixels[5], pixels[8]); DESKEETER_SORT(pixels[4], pixels[7]);
        DESKEETER_SORT(pixels[3], pixels[6]); DESKEETER_SORT(pixels[1], pixels[4]); DESKEETER_SORT(pixels[2], pixels[5]);
        DESKEETER_SORT(pixels[4], pixels[7]); DESKEETER_SORT(pixels[4], pixels[2]); DESKEETER_SORT(pixels[6], pixels[4]);
        DESKEETER_SORT(pixels[4], pixels[2]);
        return pixels[4];
    }
    else if (size == 5)
    {
        /* opt_med25() via Nicolas Devillard
         * http://ndevilla.free.fr/median/median.pdf
         */
        DESKEETER_SORT(pixels[0],  pixels[1]);  DESKEETER_SORT(pixels[3],  pixels[4]);  DESKEETER_SORT(pixels[2],  pixels[4]);
        DESKEETER_SORT(pixels[2],  pixels[3]);  DESKEETER_SORT(pixels[6],  pixels[7]);  DESKEETER_SORT(pixels[5],  pixels[7]);
        DESKEETER_SORT(pixels[5],  pixels[6]);  DESKEETER_SORT(pixels[9],  pixels[10]); DESKEETER_SORT(pixels[8],  pixels[10]);
        DESKEETER_SORT(pixels[8],  pixels[9]);  DESKEETER_SORT(pixels[12], pixels[13]); DESKEETER_SORT(pixels[11], pixels[13]);
        DESKEETER_SORT(pixels[11], pixels[12]); DESKEETER_SORT(pixels[15], pixels[16]); DESKEETER_SORT(pixels[14], pixels[16]);
        DESKEETER_SORT(pixels[14], pixels[15]); DESKEETER_SORT(pixels[18], pixels[19]); DESKEETER_SORT(pixels[17], pixels[19]);
        DESKEETER_SORT(pixels[17], pixels[18]); DESKEETER_SORT(pixels[21], pixels[22]); DESKEETER_SORT(pixels[20], pixels[22]);
        DESKEETER_SORT(pixels[20], pixels[21]); DESKEETER_SORT(pixels[23], pixels[24]); DESKEETER_SORT(pixels[2],  pixels[5]);
        DESKEETER_SORT(pixels[3],  pixels[6]);  DESKEETER_SORT(pixels[0],  pixels[6]);  DESKEETER_SORT(pixels[0],  pixels[3]);
        DESKEETER_SORT(pixels[4],  pixels[7]);  DESKEETER_SORT(pixels[1],  pixels[7]);  DESKEETER_SORT(pixels[1],  pixels[4]);
        DESKEETER_SORT(pixels[11], pixels[14]); DESKEETER_SORT(pixels[8],  pixels[14]); DESKEETER_SORT(pixels[8],  pixels[11]);
        DESKEETER_SORT(pixels[12], pixels[15]); DESKEETER_SORT(pixels[9],  pixels[15]); DESKEETER_SORT(pixels[9],  pixels[12]);
        DESKEETER_SORT(pixels[13], pixels[16]); DESKEETER_SORT(pixels[10], pixels[16]); DESKEETER_SORT(pixels[10], pixels[13]);
        DESKEETER_SORT(pixels[20], pixels[23]); DESKEETER_SORT(pixels[17], pixels[23]); DESKEETER_SORT(pixels[17], pixels[20]);
        DESKEETER_SORT(pixels[21], pixels[24]); DESKEETER_SORT(pixels[18], pixels[24]); DESKEETER_SORT(pixels[18], pixels[21]);
        DESKEETER_SORT(pixels[19], pixels[22]); DESKEETER_SORT(pixels[8],  pixels[17]); DESKEETER_SORT(pixels[9],  pixels[18]);
        DESKEETER_SORT(pixels[0],  pixels[18]); DESKEETER_SORT(pixels[0],  pixels[9]);  DESKEETER_SORT(pixels[10], pixels[19]);
        DESKEETER_SORT(pixels[1],  pixels[19]); DESKEETER_SORT(pixels[1],  pixels[10]); DESKEETER_SORT(pixels[11], pixels[20]);
        DESKEETER_SORT(pixels[2],  pixels[20]); DESKEETER_SORT(pixels[2],  pixels[11]); DESKEETER_SORT(pixels[12], pixels[21]);
        DESKEETER_SORT(pixels[3],  pixels[21]); DESKEETER_SORT(pixels[3],  pixels[12]); DESKEETER_SORT(pixels[13], pixels[22]);
        DESKEETER_SORT(pixels[4],  pixels[22]); DESKEETER_SORT(pixels[4],  pixels[13]); DESKEETER_SORT(pixels[14], pixels[23]);
        DESKEETER_SORT(pixels[5],  pixels[23]); DESKEETER_SORT(pixels[5],  pixels[14]); DESKEETER_SORT(pixels[15], pixels[24]);
        DESKEETER_SORT(pixels[6],  pixels[24]); DESKEETER_SORT(pixels[6],  pixels[15]); DESKEETER_SORT(pixels[7],  pixels[16]);
        DESKEETER_SORT(pixels[7],  pixels[19]); DESKEETER_SORT(pixels[13], pixels[21]); DESKEETER_SORT(pixels[15], pixels[23]);
        DESKEETER_SORT(pixels[7],  pixels[13]); DESKEETER_SORT(pixels[7],  pixels[15]); DESKEETER_SORT(pixels[1],  pixels[9]);
        DESKEETER_SORT(pixels[3],  pixels[11]); DESKEETER_SORT(pixels[5],  pixels[17]); DESKEETER_SORT(pixels[11], pixels[17]);
        DESKEETER_SORT(pixels[9],  pixels[17]); DESKEETER_SORT(pixels[4],  pixels[10]); DESKEETER_SORT(pixels[6],  pixels[12]);
        DESKEETER_SORT(pixels[7],  pixels[14]); DESKEETER_SORT(pixels[4],  pixels[6]);  DESKEETER_SORT(pixels[4],  pixels[7]);
        DESKEETER_SORT(pixels[12], pixels[14]); DESKEETER_SORT(pixels[10], pixels[14]); DESKEETER_SORT(pixels[6],  pixels[7]);
        DESKEETER_SORT(pixels[10], pixels[12]); DESKEETER_SORT(pixels[6],  pixels[10]); DESKEETER_SORT(pixels[6],  pixels[17]);
        DESKEETER_SORT(pixels[12], pixels[17]); DESKEETER_SORT(pixels[7],  pixels[17]); DESKEETER_SORT(pixels[7],  pixels[10]);
        DESKEETER_SORT(pixels[12], pixels[18]); DESKEETER_SORT(pixels[7],  pixels[12]); DESKEETER_SORT(pixels[10], pixels[18]);
        DESKEETER_SORT(pixels[12], pixels[20]); DESKEETER_SORT(pixels[10], pixels[20]); DESKEETER_SORT(pixels[10], pixels[12]);
        return pixels[12];
    }

    // Network for size not implemented
    return pixels[(int)((size * size)/2)];

}

static uint16_t clamp(const uint16_t pixel)
{
    return pixel < 0   ? 0   :
           pixel > 255 ? 255 : pixel;
}

static uint8_t chkbit(uint8_t * vec, int pos)
{
    int byte = pos / 8;
    int bit = pos & 0x7;
    return !!(vec[byte] & (1 << bit));
}

static void setbit(uint8_t * vec, int pos)
{
    int byte = pos / 8;
    int bit = pos & 0x7;
    vec[byte] |= (1 << bit);
}

static void clearbit(uint8_t * vec, int pos)
{
    int byte = pos / 8;
    int bit = pos & 0x7;
    vec[byte] &= 0 ^ (1 << bit);
}

static void convolve(kernel_t * kernel, uint8_t * dst, uint8_t * src,
                     int width, int height, int border)
{
    int        offset_min = -((kernel->size - 1) / 2);
    int        offset_max =   (kernel->size + 1) / 2;
    int        pos;
    for (int y = 0; y < height; y++)
    {
        pos = y * width;
        for (int x = 0; x < width; x++, pos++)
        {
            if ((y < offset_max) ||
                (y > height - offset_max) ||
                (x < offset_max - border) ||
                (x > width + border - offset_max))
            {
                *(dst + pos) = *(src + pos);
                continue;
            }
            int pixel = 0;
            int kpos = 0;
            for (int k = offset_min; k < offset_max; k++)
            {
                int bpos  = width * (y + k) + (x + offset_min);
                for (int j = offset_min; j < offset_max; j++)
                {
                    pixel += kernel->mem[kpos++] * *(src + bpos++);
                }
            }
            *(dst + pos) = pixel * kernel->coef;
        }
    }
}

static hb_buffer_t * edge_detect(const hb_buffer_t * in, int plane,
                                 deskeeter_plane_context_t * ctx)
{
    hb_buffer_t * smooth_buf = NULL, * edge_strong, * edge_weak;
    int           width, height, stride, size;
    uint8_t     * src, * weak, * strong;

    width = in->plane[plane].width;
    height = in->plane[plane].height;
    stride = in->plane[plane].stride;
    if (ctx->edge_kernel >= 0)
    {
        // Create noise filtered buffer that edge detection
        // will be performed in.
        smooth_buf  = hb_frame_buffer_init(in->f.fmt, in->f.width, in->f.height);
        int border  = (stride - width) / 2;
        kernel_t * kernel = &edge_kernels[ctx->edge_kernel];
        convolve(kernel, smooth_buf->plane[plane].data,
                         in->plane[plane].data,
                         stride, height, border);
        src = smooth_buf->plane[plane].data;
    }
    else
    {
        // Edge detection will take place on unfiltered image
        src = in->plane[plane].data;
    }
    edge_strong         = hb_buffer_init((height + 4) * stride / 8 + 4);
    edge_weak           = hb_buffer_init((height + 4) * stride / 8 + 4);
    edge_strong->offset = 2 * stride / 8;
    size                = height * stride / 8;
    strong              = edge_strong->data + edge_strong->offset;
    weak                = edge_weak->data   + edge_strong->offset;
    memset(edge_strong->data, 0, edge_strong->size);
    memset(edge_weak->data,   0, edge_weak->size);
    for (int y = 1; y < height - 1; y++)
    {
        for (int x = (y == 1); x < width; x++)
        {
            int    gx, gy;
            double g;

            gx = (int)*(src + stride * (y - 1) + (x - 1)) * -1 +
                      *(src + stride *  y      + (x - 1)) * -2 +
                      *(src + stride * (y + 1) + (x - 1)) * -1 +
                      *(src + stride * (y - 1) + (x + 1)) *  1 +
                      *(src + stride *  y      + (x + 1)) *  2 +
                      *(src + stride * (y + 1) + (x + 1)) *  1;

            gy = (int)*(src + stride * (y - 1) + (x - 1)) * -1 +
                      *(src + stride * (y - 1) +  x     ) * -2 +
                      *(src + stride * (y - 1) + (x + 1)) * -1 +
                      *(src + stride * (y + 1) + (x - 1)) *  1 +
                      *(src + stride * (y + 1) +  x     ) *  2 +
                      *(src + stride * (y + 1) + (x + 1)) *  1;

            g = sqrt(gx * gx + gy * gy);
            if (g > ctx->edge_thresh_strong)
            {
                setbit(strong, y * stride + x);
            }
            else if (g > ctx->edge_thresh_weak)
            {
                setbit(weak, y * stride + x);
            }
        }
    }
    // Upgrade weak hits if near a strong hit
    for (int i = 0; i < size; i++)
    {
        // Is there a weak hit anywhere in this byte
        if (weak[i])
        {
            int hit = 0;
            // quick check bytes surrounding this one for a strong hit
            for (int j = -2; j < 3 && !hit; j++)
            {
                for (int k = -2; k < 3; k++)
                {
                    int pos = i + stride * j / 8 + k;
                    if (strong[pos])
                    {
                        hit = 1;
                        break;
                    }
                }
            }
            if (!hit)
            {
                // No strong hits in proximity to weak hits in this byte
                weak[i] = 0;
            }
            else
            {
                // More detailed slower check of strong hits for all bits
                // set in this byte
                int b, m;
                for (b = 0, m = 1; b < 8; b++, m <<= 1)
                {
                    if (weak[i] & m)
                    {
                        int y =  (i << 3) / stride;
                        int x = ((i << 3) % stride) + b;
                        hit   = 0;
                        for (int j = -2; j < 3 && !hit; j++)
                        {
                            for (int k = -2; k < 3; k++)
                            {
                                if (chkbit(strong, stride * (y + j) + (x + k)))
                                {
                                    hit = 1;
                                    break;
                                }
                            }
                        }
                        if (!hit)
                        {
                            clearbit(weak, i * 8 + b);
                        }
                    }
                }
            }
        }
    }
    for (int i = 0; i < size; i++)
    {
        // Any bits that were not cleared are weak hits that are in
        // proximity to a strong hit.
        strong[i] |= weak[i];
    }
    hb_buffer_close(&edge_weak);
    hb_buffer_close(&smooth_buf);
    return edge_strong;
}

static void render_edge(hb_buffer_t * out, hb_buffer_t * edge_buf, int plane)
{
    uint8_t * edge   = edge_buf->data + edge_buf->offset;
    int       stride = out->plane[plane].stride;
    if (plane == 0)
    {
        memset(out->plane[0].data, 0, out->plane[0].size);
        memset(out->plane[1].data, 128, out->plane[1].size);
        memset(out->plane[2].data, 128, out->plane[2].size);
    }
    for (int i = 0; i < edge_buf->size - edge_buf->offset; i++)
    {
        if (edge[i])
        {
            int b, m;
            for (b = 0, m = 1; b < 8; b++, m <<= 1)
            {
                if (edge[i] & m)
                {
                    int y =  (i << 3) / stride;
                    int x = ((i << 3) % stride) + b;
                    int luma_x, luma_y, chroma_x, chroma_y;
                    if (plane)
                    {
                        luma_x = x << 1;
                        luma_y = y << 1;
                        chroma_x = x;
                        chroma_y = y;
                    }
                    else
                    {
                        luma_x = x;
                        luma_y = y;
                        chroma_x = x >> 1;
                        chroma_y = y >> 1;
                    }
                    *(out->plane[0].data + out->plane[0].stride * luma_y + luma_x) = 255;
                    if (plane)
                    {
                        *(out->plane[plane].data + out->plane[plane].stride * chroma_y + chroma_x) = 255;
                    }
                }
            }
        }
    }
}

static void hb_deskeeter(const hb_buffer_t * in,
                               hb_buffer_t * tmp_buf,
                               hb_buffer_t * out,
                               int           plane,
                               deskeeter_plane_context_t * ctx)
{
    uint8_t        * tmp           = tmp_buf->plane[plane].data;
    uint8_t        * dst           = out->plane[plane].data;
    uint8_t        * src           = in->plane[plane].data;
    int              width         = in->plane[plane].width;
    int              stride        = in->plane[plane].stride;
    int              height        = in->plane[plane].height;
    int              stride_border = (stride - width) / 2;
    hb_buffer_t    * edge_buf = NULL;
    uint8_t        * edge;

    const kernel_t * kernel;

    int              offset_min, offset_max, pos;
    int16_t          pixel;
    float            strength_scaled;

    int              index;
    int              median_size = 5;
    uint8_t          pixels[median_size * median_size];

    edge_buf = edge_detect(in, plane, ctx);
    edge     = edge_buf->data + edge_buf->offset;
    if (ctx->show_edge)
    {
        render_edge(out, edge_buf, plane);
        hb_buffer_close(&edge_buf);
        return;
    }
    
    kernel     = &kernels[ctx->kernel];
    offset_min = -((kernel->size - 1) / 2);
    offset_max =   (kernel->size + 1) / 2;
    for (int y = 0; y < height; y++)
    {
        pos = stride * y;
        for (int x = 0; x < stride; x++, pos++)
        {
            if (!chkbit(edge, pos) ||
                (y < offset_max) ||
                (y > height - offset_max) ||
                (x < offset_max - stride_border) ||
                (x > width + stride_border - offset_max))
            {
                *(dst + pos) = clamp(*(src + pos) + 128);
                continue;
            }
            pixel = 0;
            int kpos = 0;
            for (int k = offset_min; k < offset_max; k++)
            {
                int bpos  = stride * (y + k) + (x + offset_min);
                for (int j = offset_min; j < offset_max; j++)
                {
                    pixel += kernel->mem[kpos++] * *(src + bpos++);
                }
            }
            pixel *= kernel->coef;
            *(dst + pos) = clamp(*(src + pos) - pixel + 128);
        }
    }

    // Median filter high frequencies
    offset_min = -((median_size - 1) / 2);
    offset_max =   (median_size + 1) / 2;
    for (int y = 0; y < height; y++)
    {
        pos = stride * y;
        for (int x = 0; x < stride; x++, pos++)
        {
            if (!chkbit(edge, pos) ||
                (y < offset_max) ||
                (y > height - offset_max) ||
                (x < offset_max - stride_border) ||
                (x > width + stride_border - offset_max))
            {
                *(tmp + pos) = clamp(*(src + pos) + 128);
                continue;
            }
            index = 0;
            for (int k = offset_min; k < offset_max; k++)
            {
                int bpos  = stride * (y + k) + (x + offset_min);
                for (int j = offset_min; j < offset_max; j++)
                {
                    pixels[index++] = *(dst + bpos++);
                }
            }

            strength_scaled = (float)*(src + pos) / 64 * ctx->strength;  // Avoid contrast loss in dark areas
            strength_scaled = strength_scaled < ctx->strength / 2 ? ctx->strength / 2: strength_scaled;
            strength_scaled = strength_scaled > ctx->strength ? ctx->strength : strength_scaled;
            uint8_t median  = deskeeter_filter_median_opt(pixels, median_size);
            pixel = *(dst + pos);
            *(tmp + pos) = clamp(pixel + ((median - pixel) * strength_scaled));
        }
    }

    // Reconstruct image
    offset_min = -((kernel->size - 1) / 2);
    offset_max =   (kernel->size + 1) / 2;
    for (int y = 0; y < height; y++)
    {
        pos = stride * y;
        for (int x = 0; x < width; x++, pos++)
        {
            if (!chkbit(edge, pos) ||
                (y < offset_max) ||
                (y > height - offset_max) ||
                (x < offset_max - stride_border) ||
                (x > width + stride_border - offset_max))
                //*(src + stride*y + x) < 48)  // Avoid contrast loss in dark edges
            {
                *(dst + pos) = *(src + pos);
                continue;
            }
            pixel = *(dst + pos);
            *(dst + pos) = clamp((int16_t)*(src + pos) + *(tmp + pos) - *(dst + pos));
        }
    }
    hb_buffer_close(&edge_buf);
}

static int hb_deskeeter_init(hb_filter_object_t *filter,
                             hb_filter_init_t   *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    hb_filter_private_t * pv = filter->private_data;

    char *kernel_string[3];
    char *edge_kernel_string = NULL;

    // Mark parameters unset
    for (int c = 0; c < 3; c++)
    {
        pv->plane_ctx[c].strength = -1;
        pv->plane_ctx[c].kernel   = -1;
        kernel_string[c]          = NULL;

        pv->plane_ctx[c].edge_kernel        = -1;
        pv->plane_ctx[0].edge_thresh_strong = -1;
        pv->plane_ctx[0].edge_thresh_weak   = -1;
    }

    pv->plane_ctx[0].edge_thresh_strong = EDGE_STRONG_DEFAULT;
    pv->plane_ctx[0].edge_thresh_weak   = EDGE_WEAK_DEFAULT;
    pv->plane_ctx[0].show_edge          = 0;

    // Read user parameters
    if (filter->settings != NULL)
    {
        hb_dict_t * dict = filter->settings;
        hb_dict_extract_double(&pv->plane_ctx[0].strength, dict, "y-strength");
        hb_dict_extract_string(&kernel_string[0],          dict, "y-kernel");

        hb_dict_extract_double(&pv->plane_ctx[1].strength, dict, "cb-strength");
        hb_dict_extract_string(&kernel_string[1],          dict, "cb-kernel");

        hb_dict_extract_double(&pv->plane_ctx[2].strength, dict, "cr-strength");
        hb_dict_extract_string(&kernel_string[2],          dict, "cr-kernel");

        hb_dict_extract_double(&pv->plane_ctx[0].edge_thresh_strong, dict, "edge-thresh-strong");
        hb_dict_extract_double(&pv->plane_ctx[0].edge_thresh_weak,   dict, "edge-thresh-weak");
        hb_dict_extract_string(&edge_kernel_string,                  dict, "edge-kernel");

        hb_dict_extract_bool(&pv->plane_ctx[0].show_edge,            dict, "edge-show");
    }

    // Convert kernel user string to internal id
    for (int c = 0; c < 3; c++)
    {
        deskeeter_plane_context_t * ctx = &pv->plane_ctx[c];

        if (kernel_string[c] == NULL)
        {
            continue;
        }

        if (!strcasecmp(kernel_string[c], "lap"))
        {
            ctx->kernel = KERNEL_LAP;
        }
        else if (!strcasecmp(kernel_string[c], "isolap"))
        {
            ctx->kernel = KERNEL_ISOLAP;
        }
        else if (!strcasecmp(kernel_string[c], "log"))
        {
            ctx->kernel = KERNEL_LOG;
        }
        else if (!strcasecmp(kernel_string[c], "isolog"))
        {
            ctx->kernel = KERNEL_ISOLOG;
        }

        free(kernel_string[c]);
    }

    if (edge_kernel_string != NULL)
    {
        if (!strcasecmp(edge_kernel_string, "gaussian"))
        {
            pv->plane_ctx[0].edge_kernel = KERNEL_EDGE_GAUSS;
        }
        free(edge_kernel_string);
    }

    // Cascade values
    // Cr not set; inherit Cb. Cb not set; inherit Y. Y not set; defaults.
    for (int c = 1; c < 3; c++)
    {
        deskeeter_plane_context_t * prev_ctx = &pv->plane_ctx[c - 1];
        deskeeter_plane_context_t * ctx      = &pv->plane_ctx[c];

        if (ctx->strength == -1)             ctx->strength = prev_ctx->strength;
        if (ctx->kernel   == -1)             ctx->kernel   = prev_ctx->kernel;
        if (ctx->edge_kernel   == -1)        ctx->edge_kernel   = prev_ctx->edge_kernel;
        if (ctx->edge_thresh_strong   == -1) ctx->edge_thresh_strong = prev_ctx->edge_thresh_strong;
        if (ctx->edge_thresh_weak   == -1)   ctx->edge_thresh_weak   = prev_ctx->edge_thresh_weak;

        ctx->show_edge = prev_ctx->show_edge;
    }

    for (int c = 0; c < 3; c++)
    {
        deskeeter_plane_context_t * ctx = &pv->plane_ctx[c];

        // Replace unset values with defaults
        if (ctx->strength == -1)
        {
            ctx->strength = c ? DESKEETER_STRENGTH_CHROMA_DEFAULT :
                                DESKEETER_STRENGTH_LUMA_DEFAULT;
        }
        if (ctx->kernel   == -1)
        {
            ctx->kernel   = c ? DESKEETER_KERNEL_CHROMA_DEFAULT :
                                DESKEETER_KERNEL_LUMA_DEFAULT;
        }

        // Sanitize
        if (ctx->strength < 0)  ctx->strength = 0;
        if (ctx->strength > 1.5) ctx->strength = 1.5;
        if ((ctx->kernel < 0) || (ctx->kernel >= DESKEETER_KERNELS))
        {
            ctx->kernel = c ? DESKEETER_KERNEL_CHROMA_DEFAULT : DESKEETER_KERNEL_LUMA_DEFAULT;
        }
    }

    return 0;
}

static void hb_deskeeter_close(hb_filter_object_t * filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    free(pv);
    filter->private_data = NULL;
}

static int hb_deskeeter_work(hb_filter_object_t *filter,
                             hb_buffer_t ** buf_in,
                             hb_buffer_t ** buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in, *tmp, *out;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    hb_frame_buffer_mirror_stride(in);
    tmp  = hb_frame_buffer_init(in->f.fmt, in->f.width, in->f.height);
    out  = hb_frame_buffer_init(in->f.fmt, in->f.width, in->f.height);

    int c;
    for (c = 0; c < 3; c++)
    {
        deskeeter_plane_context_t * ctx = &pv->plane_ctx[c];
        hb_deskeeter(in, tmp, out, c, ctx);
    }
    hb_buffer_close(&tmp);

    out->s = in->s;
    *buf_out = out;

    return HB_FILTER_OK;
}
