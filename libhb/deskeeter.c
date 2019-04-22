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
#define DESKEETER_KERNEL_LUMA_DEFAULT   3
#define DESKEETER_KERNEL_CHROMA_DEFAULT 3

#define DESKEETER_SORT(a,b) { if (a > b) DESKEETER_SWAP(a, b); }
#define DESKEETER_SWAP(a,b) { a = (a ^ b); b = (a ^ b); a = (b ^ a); }

typedef struct
{
    double strength;  // strength
    int    kernel;    // which kernel to use; kernels[kernel]
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

static kernel_t kernels[] =
{
    { kernel_lap,    3, 1.0 /  5 },
    { kernel_isolap, 3, 1.0 / 25 },
    { kernel_log,    5, 1.0 / 25 },
    { kernel_isolog, 5, 1.0 / 65 }
};

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
    "cr-strength=^"HB_FLOAT_REG"$:cr-kernel=^"HB_ALL_REG"$";

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

static void hb_deskeeter(const uint8_t *src,
                               uint8_t *tmp,
                               uint8_t *dst,
                         const int width,
                         const int height,
                         const int stride,
                         deskeeter_plane_context_t * ctx)
{
    const kernel_t *kernel = &kernels[ctx->kernel];

    int offset_min    = -((kernel->size - 1) / 2);
    int offset_max    =   (kernel->size + 1) / 2;
    int stride_border =   (stride - width) / 2;
    int16_t pixel;
    float   strength_scaled;

    int index;
    int median_size = 5;
    uint8_t pixels[median_size * median_size];

    // Extract high frequencies with high pass filter
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < stride; x++)
        {
            if ((y < offset_max) ||
                (y > height - offset_max) ||
                (x < offset_max - stride_border) ||
                (x > width + stride_border - offset_max))
            {
                pixel = *(src + stride*y + x) + 128;
                pixel = pixel < 0 ? 0 : pixel;
                pixel = pixel > 255 ? 255 : pixel;
                *(dst + stride*y + x) = pixel;
                continue;
            }
            pixel = 0;
            for (int k = offset_min; k < offset_max; k++)
            {
                for (int j = offset_min; j < offset_max; j++)
                {
                    pixel += kernel->mem[((j - offset_min) * kernel->size) + k - offset_min] * *(src + stride*(y + j) + (x + k));
                }
            }
            pixel = (int16_t)*(src + stride*y + x) + ((pixel * kernel->coef) - *(src + stride*y + x));
            pixel = *(src + stride*y + x) - pixel + 128;
            pixel = pixel < 0 ? 0 : pixel;
            pixel = pixel > 255 ? 255 : pixel;
            *(dst + stride*y + x) = (uint8_t)(pixel);
        }
    }

    // Median filter high frequencies
    offset_min = -((median_size - 1) / 2);
    offset_max =   (median_size + 1) / 2;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < stride; x++)
        {
            if ((y < offset_max) ||
                (y > height - offset_max) ||
                (x < offset_max - stride_border) ||
                (x > width + stride_border - offset_max))
            {
                *(tmp + stride*y + x) = *(src + stride*y + x);
                continue;
            }
            index = 0;
            for (int k = offset_min; k < offset_max; k++)
            {
                for (int j = offset_min; j < offset_max; j++)
                {
                    pixels[index] = *(dst + stride*(y+j) + (x+k));
                    index++;
                }
            }
            strength_scaled = (float)*(src + stride*y + x) / 64 * ctx->strength;  // Avoid contrast loss in dark areas
            strength_scaled = strength_scaled < ctx->strength / 2 ? ctx->strength / 2: strength_scaled;
            strength_scaled = strength_scaled > ctx->strength ? ctx->strength : strength_scaled;
            pixel = (int16_t)(deskeeter_filter_median_opt(pixels, median_size));
            pixel = (int16_t)*(dst + stride*y + x) + ((pixel - *(dst + stride*y + x)) * strength_scaled);
            pixel = pixel < 0 ? 0 : pixel;
            pixel = pixel > 255 ? 255 : pixel;
            *(tmp + stride*y + x) = (uint8_t)(pixel);
        }
    }

    // Reconstruct image
    offset_min = -((kernel->size - 1) / 2);
    offset_max =   (kernel->size + 1) / 2;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if ((y < offset_max) ||
                (y > height - offset_max) ||
                (x < offset_max - stride_border) ||
                (x > width + stride_border - offset_max))// ||
                //*(src + stride*y + x) < 48)  // Avoid contrast loss in dark edges
            {
                *(dst + stride*y + x) = *(src + stride*y + x);
                continue;
            }
            pixel = (int16_t)*(src + stride*y + x) + *(tmp + stride*y + x) - *(dst + stride*y + x);
            pixel = pixel < 0 ? 0 : pixel;
            pixel = pixel > 255 ? 255 : pixel;
            *(dst + stride*y + x) = (uint8_t)(pixel);
        }
    }
}

static int hb_deskeeter_init(hb_filter_object_t *filter,
                             hb_filter_init_t   *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    hb_filter_private_t * pv = filter->private_data;

    char *kernel_string[3];

    // Mark parameters unset
    for (int c = 0; c < 3; c++)
    {
        pv->plane_ctx[c].strength = -1;
        pv->plane_ctx[c].kernel   = -1;
        kernel_string[c]          = NULL;
    }

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
    }

    // Convert kernel user string to internal id
    for (int c = 0; c < 3; c++)
    {
        deskeeter_plane_context_t * ctx = &pv->plane_ctx[c];

        ctx->kernel = -1;

        if (kernel_string[c] == NULL)
        {
            continue;
        }

        if (!strcasecmp(kernel_string[c], "lap"))
        {
            ctx->kernel = 0;
        }
        else if (!strcasecmp(kernel_string[c], "isolap"))
        {
            ctx->kernel = 1;
        }
        else if (!strcasecmp(kernel_string[c], "log"))
        {
            ctx->kernel = 2;
        }
        else if (!strcasecmp(kernel_string[c], "isolog"))
        {
            ctx->kernel = 3;
        }

        free(kernel_string[c]);
    }

    // Cascade values
    // Cr not set; inherit Cb. Cb not set; inherit Y. Y not set; defaults.
    for (int c = 1; c < 3; c++)
    {
        deskeeter_plane_context_t * prev_ctx = &pv->plane_ctx[c - 1];
        deskeeter_plane_context_t * ctx      = &pv->plane_ctx[c];

        if (ctx->strength == -1) ctx->strength = prev_ctx->strength;
        if (ctx->kernel   == -1) ctx->kernel   = prev_ctx->kernel;
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
    tmp = hb_frame_buffer_init(in->f.fmt, in->f.width, in->f.height);
    out = hb_frame_buffer_init(in->f.fmt, in->f.width, in->f.height);

    int c;
    for (c = 0; c < 3; c++)
    {
        deskeeter_plane_context_t * ctx = &pv->plane_ctx[c];
        hb_deskeeter(in->plane[c].data,
                 tmp->plane[c].data,
                 out->plane[c].data,
                 in->plane[c].width,
                 in->plane[c].height,
                 in->plane[c].stride,
                 ctx);
    }
    hb_buffer_close(&tmp);

    out->s = in->s;
    *buf_out = out;

    return HB_FILTER_OK;
}
