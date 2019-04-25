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
    double                strength;  // strength
    int                   kernel;    // which kernel to use; kernels[kernel]
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

#define EDGE_STRONG_DEFAULT 35
// In deskeeter, we only want to filter around fairly hard edges
// so the weak threshold is essentially ignored here.  Can
// be changed with 'edge-thresh-weak' filter setting
#define EDGE_WEAK_DEFAULT   35

#define MASK_RADIUS_DEFAULT 15
#define MASK_RADIUS_MAX     15

typedef struct
{
    hb_filter_private_t * pv;
    hb_buffer_t         * edge;
    hb_buffer_t         * mask;
} deskeeter_work_context_t;

struct hb_filter_private_s
{
    deskeeter_plane_context_t    plane_ctx[3];

    int           edge_kernel;
    double        edge_thresh_strong;
    double        edge_thresh_weak;
    int           show_edge;
    int           mask_radius;
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
    "edge-thresh-strong=^"HB_FLOAT_REG"$:edge-thresh-weak=^"HB_FLOAT_REG"$"
    "mask-radius=^"HB_INT_REG"$";

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

static void scale_edge(int plane, deskeeter_work_context_t * wctx)
{
    uint8_t * src             = wctx->edge->plane[0].data;
    uint8_t * dst             = wctx->edge->plane[plane].data;
    int       src_stride_bits = wctx->edge->plane[0].stride * 8;
    int       dst_stride_bits = wctx->edge->plane[plane].stride * 8;

    for (int i = 0; i < wctx->edge->plane[0].size; i++)
    {
        if (src[i])
        {
            int b, m;
            for (b = 0, m = 1; b < 8; b++, m <<= 1)
            {
                if (src[i] & m)
                {
                    int y =  ((i << 3) / src_stride_bits) / 2;
                    int x = (((i << 3) % src_stride_bits) + b) / 2;
                    setbit(dst, y * dst_stride_bits + x);
                }
            }
        }
    }
}

static hb_buffer_t * alloc_mask(int width, int height)
{
    hb_buffer_t * edge;

    // Leave border around edge mask to allow processing outside
    // image boundary
    int  y_size           = height       * width / 8;
    int  cr_size          = (height / 2) * (width / 16);
    edge                  = hb_buffer_init(y_size + 2 * cr_size);
    edge->f.width         = width;
    edge->f.height        = height;
    edge->plane[0].width  = width;
    edge->plane[0].height = height;
    edge->plane[0].stride = width / 8;
    edge->plane[0].size   = edge->plane[0].height * edge->plane[0].stride;
    edge->plane[1].width  = width / 2;
    edge->plane[1].height = height / 2;
    edge->plane[1].stride = width / 16;
    edge->plane[1].size   = edge->plane[1].height * edge->plane[1].stride;
    edge->plane[2].width  = width / 2;
    edge->plane[2].height = height / 2;
    edge->plane[2].stride = width / 16;
    edge->plane[2].size   = edge->plane[2].height * edge->plane[2].stride;
    edge->plane[0].data   = edge->data;
    edge->plane[1].data   = edge->data + y_size;
    edge->plane[2].data   = edge->data + y_size + cr_size;
    return edge;
}

static void edge_detect(const hb_buffer_t * in, int plane,
                        deskeeter_work_context_t * wctx)
{
    hb_buffer_t * smooth_buf = NULL, * edge_weak;
    int           width, height, stride, edge_stride_bits;
    uint8_t     * src, * weak, * strong;

    if (plane)
    {
        // construct CR/CB from Y mask
        scale_edge(plane, wctx);
        return;
    }
    width  = in->plane[plane].width;
    height = in->plane[plane].height;
    stride = in->plane[plane].stride;
    if (wctx->pv->edge_kernel >= 0)
    {
        // Create noise filtered buffer that edge detection
        // will be performed in.
        smooth_buf  = hb_frame_buffer_init(in->f.fmt, in->f.width, in->f.height);
        int border  = (stride - width) / 2;
        kernel_t * kernel = &edge_kernels[wctx->pv->edge_kernel];
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

    hb_buffer_close(&wctx->edge);

    wctx->edge       = alloc_mask(width, height);
    edge_weak        = alloc_mask(width, height);
    strong           = wctx->edge->plane[plane].data;
    weak             = edge_weak->plane[plane].data;
    edge_stride_bits = wctx->edge->plane[plane].stride * 8;
    memset(wctx->edge->data, 0, wctx->edge->size);
    memset(edge_weak->data,     0, edge_weak->size);

    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
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
            if (g > wctx->pv->edge_thresh_strong)
            {
                setbit(strong, y * edge_stride_bits + x);
            }
            else if (g > wctx->pv->edge_thresh_weak)
            {
                setbit(weak, y * edge_stride_bits + x);
            }
        }
    }

    // Upgrade weak hits if near a strong hit
    for (int i = 0; i < wctx->edge->plane[plane].size; i++)
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
                    int pos = i + edge_stride_bits * j / 8 + k;
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
                        int y =  (i << 3) / edge_stride_bits;
                        int x = ((i << 3) % edge_stride_bits) + b;
                        hit   = 0;
                        for (int j = -2; j < 3 && !hit; j++)
                        {
                            for (int k = -2; k < 3; k++)
                            {
                                if (chkbit(strong, edge_stride_bits * (y + j) + (x + k)))
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
    for (int i = 0; i < wctx->edge->plane[plane].size; i++)
    {
        // Any bits that were not cleared are weak hits that are in
        // proximity to a strong hit.
        strong[i] |= weak[i];
    }
    hb_buffer_close(&edge_weak);
    hb_buffer_close(&smooth_buf);
}

static void render_mask(hb_buffer_t * out, hb_buffer_t * edge_buf, int plane)
{
    uint8_t * edge             = edge_buf->plane[plane].data;
    int       edge_stride_bits = edge_buf->plane[plane].stride * 8;

    for (int i = 0; i < edge_buf->plane[plane].size; i++)
    {
        if (edge[i])
        {
            int b, m;
            for (b = 0, m = 1; b < 8; b++, m <<= 1)
            {
                if (edge[i] & m)
                {
                    int y =  (i << 3) / edge_stride_bits;
                    int x = ((i << 3) % edge_stride_bits) + b;
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

static void fill_thin_features(int plane, hb_buffer_t * edge)
{
    uint8_t     * src             = edge->plane[plane].data;
    int           src_height      = edge->plane[plane].height;
    int           src_stride      = edge->plane[plane].stride;
    int           src_size        = edge->plane[plane].size;
    uint16_t      mask            = 0;
    uint16_t      mask2           = 0;
    hb_buffer_t * tmp_mask        = alloc_mask(edge->f.width, edge->f.height);
    uint8_t     * tmp             = tmp_mask->plane[plane].data;

    for (int y = 0; y < src_height; y++)
    {
        int pos = src_stride * y;
        for (int i = 0; i < src_stride - 1; i++, pos++)
        {
            if (!src[pos])
            {
                tmp[pos] |= mask;
                tmp[pos] |= mask2;
                mask      = 0;
                mask2     = 0;
                continue;
            }
            // Fill 1-pixel horizontal gaps
            mask     |= (((uint16_t)src[pos] >> 2 | ((uint16_t)src[pos + 1] << 6)) & src[pos]) << 1;
            tmp[pos] |= mask;
            // Fill 2-pixel horizontal gaps
            mask2    |= (((uint16_t)src[pos] >> 3 | ((uint16_t)src[pos + 1] << 5)) & src[pos]) << 1;
            mask2    |= mask2 << 1;
            tmp[pos] |= mask2;
            mask  >>= 8;
            mask2 >>= 8;
            if (y < 2)
            {
                continue;
            }
            // Fill 1-pixel vertical gaps
            tmp[pos - 1 * src_stride] |= src[pos] & src[pos - 2 * src_stride];
            if (y < 3)
            {
                continue;
            }
            // Fill 2-pixel vertical gaps
            tmp[pos - 1 * src_stride] |= src[pos] & src[pos - 3 * src_stride];
            tmp[pos - 2 * src_stride] |= src[pos] & src[pos - 3 * src_stride];
        }
    }
    for (int i = 0; i < src_size; i++)
    {
        src[i] |= tmp[i];
    }
    hb_buffer_close(&tmp_mask);
}

static void create_mask(int plane, deskeeter_work_context_t * wctx)
{
    uint8_t * src             = wctx->edge->plane[plane].data;
    int       src_stride      = wctx->edge->plane[plane].stride;
    int       src_size        = wctx->edge->plane[plane].size;

    if (plane == 0)
    {
        hb_buffer_close(&wctx->mask);
        wctx->mask = alloc_mask(wctx->edge->f.width, wctx->edge->f.height);
        memset(wctx->mask->data, 0, wctx->mask->size);
    }
    uint8_t * dst = wctx->mask->plane[plane].data;
    int       radius;

    if (plane)
    {
        radius = wctx->pv->mask_radius / 2;
    }
    else
    {
        radius = wctx->pv->mask_radius;
    }
    for (int i = 0; i < src_size; i++)
    {
        if (src[i])
        {
            int b, m;
            for (b = 0, m = 1; b < 8; b++, m <<= 1)
            {
                if (!(src[i] & m))
                {
                    continue;
                }

                int      offset_min = -((radius - 1) / 2);
                int      offset_max =   (radius + 1) / 2;
                int      first = 8 + b + offset_min;
                uint32_t bits  = ~(~0 << radius) << first;
                for (int k = offset_min; k < offset_max; k++)
                {
                    int j;
                    int pos = i + k * src_stride;
                    if (pos < 0 || pos >= src_size) continue;

                    uint32_t mask  = 0;
                    j = pos % src_stride;

                    if (j > 0)
                    {
                        mask |= src[pos-1];
                    }
                    mask |= src[pos] << 8;
                    if (j < src_stride - 1)
                    {
                        mask |= src[pos+1] << 16;
                    }
                    mask ^= ~0;
                    mask &= bits;
                    if (j > 0)
                    {
                        dst[pos-1] |= mask & 0xff;
                    }
                    dst[pos] |= (mask >> 8) & 0xff;
                    if (j < src_stride - 1)
                    {
                        dst[pos+1] |= (mask >> 16) & 0xff;
                    }
                }
            }
        }
    }
    // Blank out edges
    for (int i = 0; i < src_size; i++)
    {
        dst[i] &= ~src[i];
    }
}

static void hb_deskeeter(const hb_buffer_t * in,
                               hb_buffer_t * tmp_buf,
                               hb_buffer_t * out,
                               int           plane,
                               deskeeter_plane_context_t * ctx,
                               deskeeter_work_context_t * wctx)
{
    uint8_t        * tmp           = tmp_buf->plane[plane].data;
    uint8_t        * dst           = out->plane[plane].data;
    uint8_t        * src           = in->plane[plane].data;
    int              width         = in->plane[plane].width;
    int              stride        = in->plane[plane].stride;
    int              height        = in->plane[plane].height;
    int              stride_border = (stride - width) / 2;
    int              mask_stride_bits;
    uint8_t        * mask;

    const kernel_t * kernel;

    int              offset_min, offset_max, pos, mask_pos;
    int16_t          pixel;
    float            strength_scaled;

    int              index;
    int              median_size = 5;
    uint8_t          pixels[median_size * median_size];

    edge_detect(in, plane, wctx);
    if (plane == 0)
    {
        fill_thin_features(plane, wctx->edge);
    }
    if (wctx->pv->show_edge)
    {
        if (plane == 0)
        {
            memset(out->plane[0].data, 0, out->plane[0].size);
            memset(out->plane[1].data, 128, out->plane[1].size);
            memset(out->plane[2].data, 128, out->plane[2].size);
            render_mask(out, wctx->edge, plane);
        }
        return;
    }

    // We want to filter around the edges, create mask of region to filter
    create_mask(plane, wctx);
    mask = wctx->mask->plane[plane].data;
    mask_stride_bits = wctx->mask->plane[plane].stride * 8;

    kernel     = &kernels[ctx->kernel];
    offset_min = -((kernel->size - 1) / 2);
    offset_max =   (kernel->size + 1) / 2;
    for (int y = 0; y < height; y++)
    {
        pos = stride * y;
        mask_pos = mask_stride_bits * y;
        for (int x = 0; x < stride; x++, pos++, mask_pos++)
        {
            if (!chkbit(mask, mask_pos) ||
                (y < offset_max) ||
                (y > height - offset_max) ||
                (x < offset_max - stride_border) ||
                (x > width + stride_border - offset_max))
            {
                *(dst + pos) = clamp(*(src + pos) + 128);
                continue;
            }
            int     kpos = 0;
            pixel = 0;
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
        mask_pos = mask_stride_bits * y;
        for (int x = 0; x < stride; x++, pos++, mask_pos++)
        {
            if (!chkbit(mask, mask_pos) ||
                (y < offset_max) ||
                (y > height - offset_max) ||
                (x < offset_max - stride_border) ||
                (x > width + stride_border - offset_max))
            {
                *(tmp + pos) = *(dst + pos);
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
        mask_pos = mask_stride_bits * y;
        for (int x = 0; x < width; x++, pos++, mask_pos++)
        {
            if (!chkbit(mask, mask_pos) ||
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
    }

    pv->edge_kernel        = -1;
    pv->edge_thresh_strong = EDGE_STRONG_DEFAULT;
    pv->edge_thresh_weak   = EDGE_WEAK_DEFAULT;
    pv->show_edge          = 0;
    pv->mask_radius         = MASK_RADIUS_DEFAULT;

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

        hb_dict_extract_double(&pv->edge_thresh_strong, dict, "edge-thresh-strong");
        hb_dict_extract_double(&pv->edge_thresh_weak,   dict, "edge-thresh-weak");
        hb_dict_extract_string(&edge_kernel_string,     dict, "edge-kernel");
        hb_dict_extract_bool(&pv->show_edge,            dict, "edge-show");
        hb_dict_extract_int(&pv->mask_radius,           dict, "mask-radius");
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
            pv->edge_kernel = KERNEL_EDGE_GAUSS;
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

    if (pv->mask_radius > MASK_RADIUS_MAX)
    {
        pv->mask_radius = MASK_RADIUS_MAX;
    }
    // Radius should always be odd so it has a center
    pv->mask_radius |= 1;

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

    deskeeter_work_context_t * wctx = calloc(sizeof(deskeeter_work_context_t), 1);

    int c;
    for (c = 0; c < 3; c++)
    {
        deskeeter_plane_context_t  * ctx  = &pv->plane_ctx[c];
        wctx->pv = pv;
        hb_deskeeter(in, tmp, out, c, ctx, wctx);
    }
    hb_buffer_close(&wctx->edge);
    hb_buffer_close(&wctx->mask);
    free(wctx);
    hb_buffer_close(&tmp);

    out->s = in->s;
    *buf_out = out;

    return HB_FILTER_OK;
}
