/* nlmeans.c

   Copyright (c) 2013 Dirk Farin
   Copyright (c) 2003-2014 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/* Usage
 *
 * Parameters:
 *     lumaY_strength   : lumaY_origin_tune   : lumaY_patch_size   : lumaY_range   : lumaY_frames   : lumaY_prefilter   :
 *     chromaB_strength : chromaB_origin_tune : chromaB_patch_size : chromaB_range : chromaB_frames : chromaB_prefilter :
 *     chromaR_strength : chromaR_origin_tune : chromaR_patch_size : chromaR_range : chromaR_frames : chromaR_prefilter
 *
 * Defaults:
 *     8:1:7:3:2:0 for each channel (equivalent to 8:1:7:3:2:0:8:1:7:3:2:0:8:1:7:3:2:0)
 *
 * Parameters cascade, e.g. 6:0.8:7:3:3:0:4:1 sets:
 *     strength 6, origin tune 0.8 for luma
 *     patch size 7, range 3, frames 3, prefilter 0 for all channels
 *     strength 4, origin tune 1 for both chroma channels
 *
 * Strength is relative and must be adjusted; ALL parameters affect overall strength.
 * Lower origin tune improves results for noisier input or animation (film 0.5-1, animation 0.15-0.5).
 * Large patch size (>9) may greatly reduce quality by clobbering detail.
 * Larger search range increases quality; however, computation time increases exponentially.
 * Large number of frames (film >3, animation >6) may cause temporal smearing.
 * Prefiltering can potentially improve weight decisions, yielding better results for difficult sources.
 *
 * Prefilter enum combos:
 *     1: Mean 3x3
 *     2: Mean 5x5
 *     3: Mean 5x5 (overrides Mean 3x3)
 *   257: Mean 3x3 reduced by 25%
 *   258: Mean 5x5 reduced by 25%
 *   513: Mean 3x3 reduced by 50%
 *   514: Mean 5x5 reduced by 50%
 *   769: Mean 3x3 reduced by 75%
 *   770: Mean 5x5 reduced by 75%
 *  1025: Mean 3x3 plus edge boost (restores lost edge detail)
 *  1026: Mean 5x5 plus edge boost
 *  1281: Mean 3x3 reduced by 25% plus edge boost
 *        etc...
 *  2049: Mean 3x3 passthru (NL-means off, prefilter is the output)
 *        etc...
 *  3329: Mean 3x3 reduced by 25% plus edge boost, passthru
 *        etc...
 */

#include "hb.h"
#include "hbffmpeg.h"

#define NLMEANS_STRENGTH_LUMA_DEFAULT      8
#define NLMEANS_STRENGTH_CHROMA_DEFAULT    8
#define NLMEANS_ORIGIN_TUNE_LUMA_DEFAULT   1
#define NLMEANS_ORIGIN_TUNE_CHROMA_DEFAULT 1
#define NLMEANS_PATCH_SIZE_LUMA_DEFAULT    7
#define NLMEANS_PATCH_SIZE_CHROMA_DEFAULT  7
#define NLMEANS_RANGE_LUMA_DEFAULT         3
#define NLMEANS_RANGE_CHROMA_DEFAULT       3
#define NLMEANS_FRAMES_LUMA_DEFAULT        2
#define NLMEANS_FRAMES_CHROMA_DEFAULT      2
#define NLMEANS_PREFILTER_LUMA_DEFAULT     0
#define NLMEANS_PREFILTER_CHROMA_DEFAULT   0

#define NLMEANS_PREFILTER_MODE_MEAN3X3       1
#define NLMEANS_PREFILTER_MODE_MEAN5X5       2
#define NLMEANS_PREFILTER_MODE_MEDIAN3X3     4
#define NLMEANS_PREFILTER_MODE_MEDIAN5X5     8
#define NLMEANS_PREFILTER_MODE_RESERVED16   16 // Reserved
#define NLMEANS_PREFILTER_MODE_RESERVED32   32 // Reserved
#define NLMEANS_PREFILTER_MODE_RESERVED64   64 // Reserved
#define NLMEANS_PREFILTER_MODE_RESERVED128 128 // Reserved
#define NLMEANS_PREFILTER_MODE_REDUCE25    256
#define NLMEANS_PREFILTER_MODE_REDUCE50    512
#define NLMEANS_PREFILTER_MODE_EDGEBOOST  1024
#define NLMEANS_PREFILTER_MODE_PASSTHRU   2048

#define NLMEANS_SORT(a,b) { if (a > b) NLMEANS_SWAP(a, b); }
#define NLMEANS_SWAP(a,b) { a = (a ^ b); b = (a ^ b); a = (b ^ a); }

#define NLMEANS_FRAMES_MAX 32
#define NLMEANS_EXPSIZE    128

typedef struct
{
    uint8_t *mem;
    uint8_t *mem_pre;
    uint8_t *image;
    uint8_t *image_pre;
    int w;
    int h;
    int border;
} BorderedPlane;

struct PixelSum
{
    float weight_sum;
    float pixel_sum;
};

struct hb_filter_private_s
{
    double strength[3];    // averaging weight decay, larger produces smoother output
    double origin_tune[3]; // weight tuning for origin patch, 0.00..1.00
    int    patch_size[3];  // pixel context region width  (must be odd)
    int    range[3];       // spatial search window width (must be odd)
    int    frames[3];      // temporal search depth in frames
    int    prefilter[3];   // prefilter mode, can improve weight analysis

    BorderedPlane frame_tmp[3][32];
    int           frame_ready[3][32];
};

static int hb_nlmeans_init(hb_filter_object_t *filter,
                           hb_filter_init_t   *init);

static int hb_nlmeans_work(hb_filter_object_t *filter,
                           hb_buffer_t **buf_in,
                           hb_buffer_t **buf_out);

static void hb_nlmeans_close(hb_filter_object_t *filter);

hb_filter_object_t hb_filter_nlmeans =
{
    .id            = HB_FILTER_NLMEANS,
    .enforce_order = 1,
    .name          = "Denoise (nlmeans)",
    .settings      = NULL,
    .init          = hb_nlmeans_init,
    .work          = hb_nlmeans_work,
    .close         = hb_nlmeans_close,
};

static void nlmeans_border(uint8_t *src,
                           int w,
                           int h,
                           int border)
{

    uint8_t *image = src + border + w*border;
    int iw = w - 2*border;
    int ih = h - 2*border;

    // Create faux borders using edge pixels
    for (int y = 0; y < ih; y++)
    {
        for (int x = 0; x < border; x++)
        {
            *(image + y*w - x - 1)  = *(image + y*w + x);
            *(image + y*w + x + iw) = *(image + y*w - x + (iw-1));
        }
    }
    for (int y = 0; y < border; y++)
    {
        memcpy(image - border -  (y+1)*w, image - border +        y*w, w);
        memcpy(image - border + (y+ih)*w, image - border + (ih-y-1)*w, w);
    }

}

static void nlmeans_deborder(uint8_t *src,
                             uint8_t *dst,
                             int w,
                             int h,
                             int border)
{

    uint8_t *image = src + border + w*border;
    int iw = w - 2*border;
    int ih = h - 2*border;

    // Copy main image
    for (int y = 0; y < ih; y++)
    {
        memcpy(dst + y*iw, image + y*w, iw);
    }

}

static void nlmeans_alloc(uint8_t *src,
                          int src_w,
                          int src_h,
                          BorderedPlane *dst,
                          int dst_w,
                          int dst_h,
                          int border)
{

    uint8_t *mem   = malloc(dst_w * dst_h * sizeof(uint8_t));
    uint8_t *image = mem + border + dst_w*border;

    // Copy main image
    for (int y = 0; y < src_h; y++)
    {
        memcpy(image + y*dst_w, src + y*src_w, src_w);
    }

    dst->mem       = mem;
    dst->image     = image;
    dst->w         = dst_w;
    dst->h         = dst_h;
    dst->border    = border;

    nlmeans_border(dst->mem, dst->w, dst->h, dst->border);
    dst->mem_pre   = dst->mem;
    dst->image_pre = dst->image;

}

static void nlmeans_filter_mean(uint8_t *src,
                                uint8_t *dst,
                                int w,
                                int h,
                                int border,
                                int size)
{

    // Mean filter
    int iw = w - 2*border;
    int ih = h - 2*border;
    int offset_min = -((size - 1) /2);
    int offset_max =   (size + 1) /2;
    uint16_t pixel_sum;
    double pixel_weight = 1.0 / (size * size);
    for (int y = 0; y < ih; y++)
    {
        for (int x = 0; x < iw; x++)
        {
            pixel_sum = 0;
            for (int k = offset_min; k < offset_max; k++)
            {
                for (int j = offset_min; j < offset_max; j++)
                {
                    pixel_sum = pixel_sum + *(src + w*(y+j) + (x+k));
                }
            }
            *(dst + w*y + x) = (uint8_t)(pixel_sum * pixel_weight);
        }
    }

}

static uint8_t nlmeans_filter_median_opt(uint8_t *pixels, int size)
{

    // Optimized sorting networks
    if (size == 3)
    {
        /* opt_med9() via Nicolas Devillard
         * http://ndevilla.free.fr/median/median.pdf
         */
        NLMEANS_SORT(pixels[1], pixels[2]); NLMEANS_SORT(pixels[4], pixels[5]); NLMEANS_SORT(pixels[7], pixels[8]);
        NLMEANS_SORT(pixels[0], pixels[1]); NLMEANS_SORT(pixels[3], pixels[4]); NLMEANS_SORT(pixels[6], pixels[7]);
        NLMEANS_SORT(pixels[1], pixels[2]); NLMEANS_SORT(pixels[4], pixels[5]); NLMEANS_SORT(pixels[7], pixels[8]);
        NLMEANS_SORT(pixels[0], pixels[3]); NLMEANS_SORT(pixels[5], pixels[8]); NLMEANS_SORT(pixels[4], pixels[7]);
        NLMEANS_SORT(pixels[3], pixels[6]); NLMEANS_SORT(pixels[1], pixels[4]); NLMEANS_SORT(pixels[2], pixels[5]);
        NLMEANS_SORT(pixels[4], pixels[7]); NLMEANS_SORT(pixels[4], pixels[2]); NLMEANS_SORT(pixels[6], pixels[4]);
        NLMEANS_SORT(pixels[4], pixels[2]);
        return pixels[4];
    }
    else if (size == 5)
    {
        /* opt_med25() via Nicolas Devillard
         * http://ndevilla.free.fr/median/median.pdf
         */
        NLMEANS_SORT(pixels[0],  pixels[1]);  NLMEANS_SORT(pixels[3],  pixels[4]);  NLMEANS_SORT(pixels[2],  pixels[4]);
        NLMEANS_SORT(pixels[2],  pixels[3]);  NLMEANS_SORT(pixels[6],  pixels[7]);  NLMEANS_SORT(pixels[5],  pixels[7]);
        NLMEANS_SORT(pixels[5],  pixels[6]);  NLMEANS_SORT(pixels[9],  pixels[10]); NLMEANS_SORT(pixels[8],  pixels[10]);
        NLMEANS_SORT(pixels[8],  pixels[9]);  NLMEANS_SORT(pixels[12], pixels[13]); NLMEANS_SORT(pixels[11], pixels[13]);
        NLMEANS_SORT(pixels[11], pixels[12]); NLMEANS_SORT(pixels[15], pixels[16]); NLMEANS_SORT(pixels[14], pixels[16]);
        NLMEANS_SORT(pixels[14], pixels[15]); NLMEANS_SORT(pixels[18], pixels[19]); NLMEANS_SORT(pixels[17], pixels[19]);
        NLMEANS_SORT(pixels[17], pixels[18]); NLMEANS_SORT(pixels[21], pixels[22]); NLMEANS_SORT(pixels[20], pixels[22]);
        NLMEANS_SORT(pixels[20], pixels[21]); NLMEANS_SORT(pixels[23], pixels[24]); NLMEANS_SORT(pixels[2],  pixels[5]);
        NLMEANS_SORT(pixels[3],  pixels[6]);  NLMEANS_SORT(pixels[0],  pixels[6]);  NLMEANS_SORT(pixels[0],  pixels[3]);
        NLMEANS_SORT(pixels[4],  pixels[7]);  NLMEANS_SORT(pixels[1],  pixels[7]);  NLMEANS_SORT(pixels[1],  pixels[4]);
        NLMEANS_SORT(pixels[11], pixels[14]); NLMEANS_SORT(pixels[8],  pixels[14]); NLMEANS_SORT(pixels[8],  pixels[11]);
        NLMEANS_SORT(pixels[12], pixels[15]); NLMEANS_SORT(pixels[9],  pixels[15]); NLMEANS_SORT(pixels[9],  pixels[12]);
        NLMEANS_SORT(pixels[13], pixels[16]); NLMEANS_SORT(pixels[10], pixels[16]); NLMEANS_SORT(pixels[10], pixels[13]);
        NLMEANS_SORT(pixels[20], pixels[23]); NLMEANS_SORT(pixels[17], pixels[23]); NLMEANS_SORT(pixels[17], pixels[20]);
        NLMEANS_SORT(pixels[21], pixels[24]); NLMEANS_SORT(pixels[18], pixels[24]); NLMEANS_SORT(pixels[18], pixels[21]);
        NLMEANS_SORT(pixels[19], pixels[22]); NLMEANS_SORT(pixels[8],  pixels[17]); NLMEANS_SORT(pixels[9],  pixels[18]);
        NLMEANS_SORT(pixels[0],  pixels[18]); NLMEANS_SORT(pixels[0],  pixels[9]);  NLMEANS_SORT(pixels[10], pixels[19]);
        NLMEANS_SORT(pixels[1],  pixels[19]); NLMEANS_SORT(pixels[1],  pixels[10]); NLMEANS_SORT(pixels[11], pixels[20]);
        NLMEANS_SORT(pixels[2],  pixels[20]); NLMEANS_SORT(pixels[2],  pixels[11]); NLMEANS_SORT(pixels[12], pixels[21]);
        NLMEANS_SORT(pixels[3],  pixels[21]); NLMEANS_SORT(pixels[3],  pixels[12]); NLMEANS_SORT(pixels[13], pixels[22]);
        NLMEANS_SORT(pixels[4],  pixels[22]); NLMEANS_SORT(pixels[4],  pixels[13]); NLMEANS_SORT(pixels[14], pixels[23]);
        NLMEANS_SORT(pixels[5],  pixels[23]); NLMEANS_SORT(pixels[5],  pixels[14]); NLMEANS_SORT(pixels[15], pixels[24]);
        NLMEANS_SORT(pixels[6],  pixels[24]); NLMEANS_SORT(pixels[6],  pixels[15]); NLMEANS_SORT(pixels[7],  pixels[16]);
        NLMEANS_SORT(pixels[7],  pixels[19]); NLMEANS_SORT(pixels[13], pixels[21]); NLMEANS_SORT(pixels[15], pixels[23]);
        NLMEANS_SORT(pixels[7],  pixels[13]); NLMEANS_SORT(pixels[7],  pixels[15]); NLMEANS_SORT(pixels[1],  pixels[9]);
        NLMEANS_SORT(pixels[3],  pixels[11]); NLMEANS_SORT(pixels[5],  pixels[17]); NLMEANS_SORT(pixels[11], pixels[17]);
        NLMEANS_SORT(pixels[9],  pixels[17]); NLMEANS_SORT(pixels[4],  pixels[10]); NLMEANS_SORT(pixels[6],  pixels[12]);
        NLMEANS_SORT(pixels[7],  pixels[14]); NLMEANS_SORT(pixels[4],  pixels[6]);  NLMEANS_SORT(pixels[4],  pixels[7]);
        NLMEANS_SORT(pixels[12], pixels[14]); NLMEANS_SORT(pixels[10], pixels[14]); NLMEANS_SORT(pixels[6],  pixels[7]);
        NLMEANS_SORT(pixels[10], pixels[12]); NLMEANS_SORT(pixels[6],  pixels[10]); NLMEANS_SORT(pixels[6],  pixels[17]);
        NLMEANS_SORT(pixels[12], pixels[17]); NLMEANS_SORT(pixels[7],  pixels[17]); NLMEANS_SORT(pixels[7],  pixels[10]);
        NLMEANS_SORT(pixels[12], pixels[18]); NLMEANS_SORT(pixels[7],  pixels[12]); NLMEANS_SORT(pixels[10], pixels[18]);
        NLMEANS_SORT(pixels[12], pixels[20]); NLMEANS_SORT(pixels[10], pixels[20]); NLMEANS_SORT(pixels[10], pixels[12]);
        return pixels[12];
    }

    // Network for size not implemented
    return pixels[(int)((size * size)/2)];

}

static void nlmeans_filter_median(uint8_t *src,
                                  uint8_t *dst,
                                  int w,
                                  int h,
                                  int border,
                                  int size)
{

    // Median filter
    int iw = w - 2*border;
    int ih = h - 2*border;
    int offset_min = -((size - 1) /2);
    int offset_max =   (size + 1) /2;
    int index;
    uint8_t pixels[size * size];
    for (int y = 0; y < ih; y++)
    {
        for (int x = 0; x < iw; x++)
        {
            index = 0;
            for (int k = offset_min; k < offset_max; k++)
            {
                for (int j = offset_min; j < offset_max; j++)
                {
                    pixels[index] = *(src + w*(y+j) + (x+k));
                    index++;
                }
            }
            *(dst + w*y + x) = nlmeans_filter_median_opt(pixels, size);
        }
    }

}

static void nlmeans_filter_edgeboost(uint8_t *src,
                                     uint8_t *dst,
                                     int w,
                                     int h,
                                     int border)
{

    int iw = w - 2*border;
    int ih = h - 2*border;

    // Custom kernel
    int kernel_size = 3;
    int kernel[3][3] = {{-31, 0, 31},
                        {-44, 0, 44},
                        {-31, 0, 31}};
    double kernel_coef = 1.0 / 126.42;

    // Detect edges
    int offset_min = -((kernel_size - 1) /2);
    int offset_max =   (kernel_size + 1) /2;
    uint16_t pixel1;
    uint16_t pixel2;
    uint8_t *mask_mem = calloc(w * h, sizeof(uint8_t));
    uint8_t *mask = mask_mem + border + w*border;
    for (int y = 0; y < ih; y++)
    {
        for (int x = 0; x < iw; x++)
        {
            pixel1 = 0;
            pixel2 = 0;
            for (int k = offset_min; k < offset_max; k++)
            {
                for (int j = offset_min; j < offset_max; j++)
                {
                    pixel1 += kernel[j+1][k+1] * *(src + w*(y+j) + (x+k));
                    pixel2 += kernel[k+1][j+1] * *(src + w*(y+j) + (x+k));
                }
            }
            pixel1 = pixel1 > 0 ? pixel1 : -pixel1;
            pixel2 = pixel2 > 0 ? pixel2 : -pixel2;
            pixel1 = (uint16_t)(((double)pixel1 * kernel_coef) + 128);
            pixel2 = (uint16_t)(((double)pixel2 * kernel_coef) + 128);
            *(mask + w*y + x) = (uint8_t)(pixel1 + pixel2);
            if (*(mask + w*y + x) > 160)
            {
                *(mask + w*y + x) = 235;
            }
            else if (*(mask + w*y + x) > 16)
            {
                *(mask + w*y + x) = 128;
            }
            else
            {
                *(mask + w*y + x) = 16;
            }
        }
    }

    // Post-process and output
    int pixels;
    for (int y = 0; y < ih; y++)
    {
        for (int x = 0; x < iw; x++)
        {
            if (*(mask + w*y + x) > 16)
            {
                // Count nearby edge pixels
                pixels = 0;
                for (int k = offset_min; k < offset_max; k++)
                {
                    for (int j = offset_min; j < offset_max; j++)
                    {
                        if (*(mask + w*(y+j) + (x+k)) > 16)
                        {
                            pixels++;
                        }
                    }
                }
                // Remove false positive
                if (pixels < 3)
                {
                    *(mask + w*y + x) = 16;
                }
                // Filter output
                if (*(mask + w*y + x) > 16)
                {
                    if (*(mask + w*y + x) == 235)
                    {
                        *(dst + w*y + x) = (3 * *(src + w*y + x) + 1 * *(dst + w*y + x)) /4;
                    }
                    else
                    {
                        *(dst + w*y + x) = (2 * *(src + w*y + x) + 3 * *(dst + w*y + x)) /5;
                    }
                    //*(dst + w*y + x) = *(mask + w*y + x); // Overlay mask
                }
            }
            //*(dst + w*y + x) = *(mask + w*y + x); // Full mask
        }
    }

    free(mask_mem);

}

static void nlmeans_prefilter(BorderedPlane *src,
                              int filter_type)
{

    if (filter_type & NLMEANS_PREFILTER_MODE_MEAN3X3   ||
        filter_type & NLMEANS_PREFILTER_MODE_MEAN5X5   ||
        filter_type & NLMEANS_PREFILTER_MODE_MEDIAN3X3 ||
        filter_type & NLMEANS_PREFILTER_MODE_MEDIAN5X5)
    {

        // Source image
        uint8_t *mem   = src->mem;
        uint8_t *image = src->image;
        int w          = src->w;
        int h          = src->h;
        int border     = src->border;

        // Duplicate plane
        uint8_t *mem_pre = malloc(w * h * sizeof(uint8_t));
        uint8_t *image_pre = mem_pre + border + w*border;
        for (int y = 0; y < h; y++)
        {
            memcpy(mem_pre + y*w, mem + y*w, w);
        }

        // Filter plane; should already have at least 2px extra border on each side
        if (filter_type & NLMEANS_PREFILTER_MODE_MEDIAN5X5)
        {
            // Median 5x5
            nlmeans_filter_median(image, image_pre, w, h, border, 5);
        }
        else if (filter_type & NLMEANS_PREFILTER_MODE_MEDIAN3X3)
        {
            // Median 3x3
            nlmeans_filter_median(image, image_pre, w, h, border, 3);
        }
        else if (filter_type & NLMEANS_PREFILTER_MODE_MEAN5X5)
        {
            // Mean 5x5
            nlmeans_filter_mean(image, image_pre, w, h, border, 5);
        }
        else if (filter_type & NLMEANS_PREFILTER_MODE_MEAN3X3)
        {
            // Mean 3x3
            nlmeans_filter_mean(image, image_pre, w, h, border, 3);
        }

        // Restore edges
        if (filter_type & NLMEANS_PREFILTER_MODE_EDGEBOOST)
        {
            nlmeans_filter_edgeboost(image, image_pre, w, h, border);
        }

        // Blend source and destination for lesser effect
        int wet = 1;
        int dry = 0;
        if (filter_type & NLMEANS_PREFILTER_MODE_REDUCE50 &&
            filter_type & NLMEANS_PREFILTER_MODE_REDUCE25)
        {
            wet = 1;
            dry = 3;
        }
        else if (filter_type & NLMEANS_PREFILTER_MODE_REDUCE50)
        {
            wet = 1;
            dry = 1;
        }
        else if (filter_type & NLMEANS_PREFILTER_MODE_REDUCE25)
        {
            wet = 3;
            dry = 1;
        }
        if (dry > 0)
        {
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    *(mem_pre + w*y + x) = (uint8_t)((wet * *(mem_pre + w*y + x) + dry * *(mem + w*y + x)) / (wet + dry));
                }
            }
        }

        // Assign result
        src->mem_pre   = mem_pre;
        src->image_pre = image_pre;

        // Recreate borders
        nlmeans_border(mem_pre, w, h, border);

    }

}

static void nlmeans_plane(BorderedPlane *plane_tmp,
                          int *plane_ready,
                          uint8_t *dst,
                          int w,
                          int h,
                          double h_param,
                          double origin_tune,
                          int n,
                          int r)
{

    int n_half = (n-1) /2;
    int r_half = (r-1) /2;

    // Source image
    uint8_t *src     = plane_tmp[0].image;
    uint8_t *src_pre = plane_tmp[0].image_pre;
    int src_w        = plane_tmp[0].w;

    // Allocate temporary pixel sums
    struct PixelSum *tmp_data = calloc(w * h, sizeof(struct PixelSum));

    // Allocate integral image
    int integral_stride = w + 2*16;
    uint32_t *integral_mem = malloc(integral_stride * (h+1) * sizeof(uint32_t));
    uint32_t *integral     = integral_mem + integral_stride + 16;

    // Precompute exponential table
    float exptable[NLMEANS_EXPSIZE];
    const float weight_factor       = 1.0/n/n / (h_param * h_param);
    const float min_weight_in_table = 0.0005;
    const float stretch             = NLMEANS_EXPSIZE / (-log(min_weight_in_table));
    const float weight_fact_table   = weight_factor * stretch;
    const int   diff_max            = NLMEANS_EXPSIZE / weight_fact_table;
    for (int i = 0; i < NLMEANS_EXPSIZE; i++)
    {
        exptable[i] = exp(-i/stretch);
    }
    exptable[NLMEANS_EXPSIZE-1] = 0;

    // Iterate through available frames
    for (int plane_index = 0; plane_ready[plane_index] == 1; plane_index++)
    {

        // Compare image
        uint8_t *compare     = plane_tmp[plane_index].image;
        uint8_t *compare_pre = plane_tmp[plane_index].image_pre;
        int compare_w        = plane_tmp[plane_index].w;

        // Iterate through all displacements
        for (int dy = -r_half; dy <= r_half; dy++)
        {
            for (int dx = -r_half; dx <= r_half; dx++)
            {

                // Apply special weight tuning to origin patch
                if (dx == 0 && dy == 0 && plane_index == 0)
                {
                    // TODO: Parallelize this
                    for (int y = n_half; y < h-n + n_half; y++)
                    {
                        for (int x = n_half; x < w-n + n_half; x++)
                        {
                            tmp_data[y*w + x].weight_sum += origin_tune;
                            tmp_data[y*w + x].pixel_sum  += origin_tune * src[y*src_w + x];
                        }
                    }
                    continue;
                }

                // Build integral
                memset(integral-1 - integral_stride, 0, (w+1) * sizeof(uint32_t));
                for (int y = 0; y < h; y++)
                {
                    const uint8_t *p1 = src_pre + y*src_w;
                    const uint8_t *p2 = compare_pre + (y+dy)*compare_w + dx;
                    uint32_t *out = integral + (y*integral_stride) - 1;

                    *out++ = 0;

                    for (int x = 0; x < w; x++)
                    {
                        int diff = *p1++ - *p2++;
                        *out = *(out-1) + diff * diff;
                        out++;
                    }

                    if (y > 0)
                    {
                        out = integral + y*integral_stride;

                        for (int x = 0; x < w; x++)
                        {
                            *out += *(out - integral_stride);
                            out++;
                        }
                    }
                }

                // Average displacement
                // TODO: Parallelize this
                for (int y = 0; y <= h-n; y++)
                {
                    const uint32_t *integral_ptr1 = integral + (y  -1)*integral_stride - 1;
                    const uint32_t *integral_ptr2 = integral + (y+n-1)*integral_stride - 1;

                    for (int x = 0; x <= w-n; x++)
                    {
                        int xc = x + n_half;
                        int yc = y + n_half;

                        // Difference between patches
                        int diff = (uint32_t)(integral_ptr2[n] - integral_ptr2[0] - integral_ptr1[n] + integral_ptr1[0]);

                        // Sum pixel with weight
                        if (diff < diff_max)
                        {
                            int diffidx = diff * weight_fact_table;

                            //float weight = exp(-diff*weightFact);
                            float weight = exptable[diffidx];

                            tmp_data[yc*w + xc].weight_sum += weight;
                            tmp_data[yc*w + xc].pixel_sum  += weight * compare[(yc+dy)*compare_w + xc + dx];
                        }

                        integral_ptr1++;
                        integral_ptr2++;
                    }
                }
            }
        }
    }

    // Copy edges
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < n_half; x++)
        {
            *(dst + y*w + x)         = *(src + y*src_w - x - 1);
            *(dst + y*w - x + (w-1)) = *(src + y*src_w + x + w);
        }
    }
    for (int y = 0; y < n_half; y++)
    {
        memcpy(dst +       y*w, src - (y+1)*src_w, w);
        memcpy(dst + (h-y-1)*w, src + (y+h)*src_w, w);
    }

    // Copy main image
    uint8_t result;
    for (int y = n_half; y < h-n_half; y++)
    {
        for (int x = n_half; x < w-n_half; x++)
        {
            result = (uint8_t)(tmp_data[y*w + x].pixel_sum / tmp_data[y*w + x].weight_sum);
            *(dst + y*w + x) = result ? result : *(src + y*src_w + x);
        }
    }

    free(tmp_data);
    free(integral_mem);

}

static int hb_nlmeans_init(hb_filter_object_t *filter,
                           hb_filter_init_t *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    hb_filter_private_t *pv = filter->private_data;

    // Mark parameters unset
    for (int c = 0; c < 3; c++)
    {
        pv->strength[c]    = -1;
        pv->origin_tune[c] = -1;
        pv->patch_size[c]  = -1;
        pv->range[c]       = -1;
        pv->frames[c]      = -1;
        pv->prefilter[c]   = -1;
    }

    // Read user parameters
    if (filter->settings != NULL)
    {
        sscanf(filter->settings, "%lf:%lf:%d:%d:%d:%d:%lf:%lf:%d:%d:%d:%d:%lf:%lf:%d:%d:%d:%d",
               &pv->strength[0], &pv->origin_tune[0], &pv->patch_size[0], &pv->range[0], &pv->frames[0], &pv->prefilter[0],
               &pv->strength[1], &pv->origin_tune[1], &pv->patch_size[1], &pv->range[1], &pv->frames[1], &pv->prefilter[1],
               &pv->strength[2], &pv->origin_tune[2], &pv->patch_size[2], &pv->range[2], &pv->frames[2], &pv->prefilter[2]);
    }

    // Cascade values
    // Cr not set; inherit Cb. Cb not set; inherit Y. Y not set; defaults.
    for (int c = 1; c < 3; c++)
    {
        if (pv->strength[c]    == -1) { pv->strength[c]    = pv->strength[c-1]; }
        if (pv->origin_tune[c] == -1) { pv->origin_tune[c] = pv->origin_tune[c-1]; }
        if (pv->patch_size[c]  == -1) { pv->patch_size[c]  = pv->patch_size[c-1]; }
        if (pv->range[c]       == -1) { pv->range[c]       = pv->range[c-1]; }
        if (pv->frames[c]      == -1) { pv->frames[c]      = pv->frames[c-1]; }
        if (pv->prefilter[c]   == -1) { pv->prefilter[c]   = pv->prefilter[c-1]; }
    }

    for (int c = 0; c < 3; c++)
    {
        // Replace unset values with defaults
        if (pv->strength[c]    == -1) { pv->strength[c]    = c ? NLMEANS_STRENGTH_LUMA_DEFAULT    : NLMEANS_STRENGTH_CHROMA_DEFAULT; }
        if (pv->origin_tune[c] == -1) { pv->origin_tune[c] = c ? NLMEANS_ORIGIN_TUNE_LUMA_DEFAULT : NLMEANS_ORIGIN_TUNE_CHROMA_DEFAULT; }
        if (pv->patch_size[c]  == -1) { pv->patch_size[c]  = c ? NLMEANS_PATCH_SIZE_LUMA_DEFAULT  : NLMEANS_PATCH_SIZE_CHROMA_DEFAULT; }
        if (pv->range[c]       == -1) { pv->range[c]       = c ? NLMEANS_RANGE_LUMA_DEFAULT       : NLMEANS_RANGE_CHROMA_DEFAULT; }
        if (pv->frames[c]      == -1) { pv->frames[c]      = c ? NLMEANS_FRAMES_LUMA_DEFAULT      : NLMEANS_FRAMES_CHROMA_DEFAULT; }
        if (pv->prefilter[c]   == -1) { pv->prefilter[c]   = c ? NLMEANS_PREFILTER_LUMA_DEFAULT   : NLMEANS_PREFILTER_CHROMA_DEFAULT; }

        // Sanitize
        if (pv->strength[c] < 0)        { pv->strength[c] = 0; }
        if (pv->origin_tune[c] < 0.01)  { pv->origin_tune[c] = 0.01; } // avoid black artifacts
        if (pv->origin_tune[c] > 1)     { pv->origin_tune[c] = 1; }
        if (pv->patch_size[c] % 2 == 0) { pv->patch_size[c]--; }
        if (pv->patch_size[c] < 1)      { pv->patch_size[c] = 1; }
        if (pv->range[c] % 2 == 0)      { pv->range[c]--; }
        if (pv->range[c] < 1)           { pv->range[c] = 1; }
        if (pv->frames[c] < 1)          { pv->frames[c] = 1; }
        if (pv->frames[c] > NLMEANS_FRAMES_MAX) { pv->frames[c] = NLMEANS_FRAMES_MAX; }
        if (pv->prefilter[c] < 0)       { pv->prefilter[c] = 0; }

        // Mark buffer empty
        for (int f = 0; f < NLMEANS_FRAMES_MAX; f++)
        {
            pv->frame_ready[c][f] = 0;
        }
    }

    return 0;
}

static void hb_nlmeans_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    for (int c = 0; c < 3; c++)
    {
        for (int f = 0; f < pv->frames[c]; f++)
        {
            if (pv->frame_tmp[c][f].mem_pre != NULL &&
                pv->frame_tmp[c][f].mem_pre != pv->frame_tmp[c][f].mem)
            {
                free(pv->frame_tmp[c][f].mem_pre);
                pv->frame_tmp[c][f].mem_pre = NULL;
            }
            if (pv->frame_tmp[c][f].mem != NULL)
            {
                free(pv->frame_tmp[c][f].mem);
                pv->frame_tmp[c][f].mem = NULL;
            }
        }
    }

    free(pv);
    filter->private_data = NULL;
}

static int hb_nlmeans_work(hb_filter_object_t *filter,
                           hb_buffer_t **buf_in,
                           hb_buffer_t **buf_out )
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in, *out;

    if (in->size <= 0)
    {
        *buf_out = in;
        *buf_in  = NULL;
        return HB_FILTER_DONE;
    }

    out = hb_video_buffer_init(in->f.width, in->f.height);

    for (int c = 0; c < 3; c++)
    {

        if (pv->strength[c] == 0)
        {
            out->plane[c].data = in->plane[c].data;
            continue;
        }

        int frames = pv->frames[c];

        // Release last frame in buffer
        if (pv->frame_tmp[c][frames-1].mem_pre != NULL &&
            pv->frame_tmp[c][frames-1].mem_pre != pv->frame_tmp[c][frames-1].mem)
        {
            free(pv->frame_tmp[c][frames-1].mem_pre);
            pv->frame_tmp[c][frames-1].mem_pre = NULL;
        }
        if (pv->frame_tmp[c][frames-1].mem != NULL)
        {
            free(pv->frame_tmp[c][frames-1].mem);
            pv->frame_tmp[c][frames-1].mem = NULL;
        }
        pv->frame_ready[c][frames-1] = 0;

        // Shift frames in buffer down one level
        for (int f = frames-1; f > 0; f--)
        {
            pv->frame_tmp[c][f]   = pv->frame_tmp[c][f-1];
            pv->frame_ready[c][f] = pv->frame_ready[c][f-1];
        }

        // Extend copy of plane with extra border and place in buffer
        int border = ((pv->range[c] + 2) / 2 + 15) /16*16;
        int w = in->plane[c].stride + 2*border;
        int h = in->plane[c].height + 2*border;
        nlmeans_alloc(in->plane[c].data,
                      in->plane[c].stride,
                      in->plane[c].height,
                      &pv->frame_tmp[c][0],
                      w,
                      h,
                      border);
        nlmeans_prefilter(&pv->frame_tmp[c][0], pv->prefilter[c]);
        pv->frame_ready[c][0] = 1;

        if (pv->prefilter[c] & NLMEANS_PREFILTER_MODE_PASSTHRU)
        {
            nlmeans_deborder(pv->frame_tmp[c][0].mem_pre, out->plane[c].data, w, h, border);
            continue;
        }

        // Process current plane
        nlmeans_plane(pv->frame_tmp[c],
                      pv->frame_ready[c],
                      out->plane[c].data,
                      in->plane[c].stride,
                      in->plane[c].height,
                      pv->strength[c],
                      pv->origin_tune[c],
                      pv->patch_size[c],
                      pv->range[c]);

    }

    out->s = in->s;
    hb_buffer_move_subs(out, in);

    *buf_out = out;

    return HB_FILTER_OK;
}
