/* nlmeans.c

   Copyright (c) 2013 Dirk Farin
   Copyright (c) 2003-2015 HandBrake Team
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
 *  2049: Mean 3x3 passthru (NLMeans off, prefilter is the output)
 *        etc...
 *  3329: Mean 3x3 reduced by 25% plus edge boost, passthru
 *        etc...
 */

#include "hb.h"
#include "hbffmpeg.h"
#include "taskset.h"
#include "nlmeans.h"

#define NLMEANS_STRENGTH_LUMA_DEFAULT      6
#define NLMEANS_STRENGTH_CHROMA_DEFAULT    6
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

#define NLMEANS_FRAMES_MAX  32
#define NLMEANS_EXPSIZE     128

typedef struct
{
    uint8_t *mem;
    uint8_t *mem_pre;
    uint8_t *image;
    uint8_t *image_pre;
    int w;
    int h;
    int border;
    hb_lock_t *mutex;
    int prefiltered;
} BorderedPlane;

typedef struct
{
    int width;
    int height;
    int fmt;
    BorderedPlane plane[3];
    hb_buffer_settings_t s;
} Frame;

struct PixelSum
{
    float weight_sum;
    float pixel_sum;
};

typedef struct
{
    hb_filter_private_t *pv;
    int segment;
    hb_buffer_t *out;
} nlmeans_thread_arg_t;

struct hb_filter_private_s
{
    double strength[3];    // averaging weight decay, larger produces smoother output
    double origin_tune[3]; // weight tuning for origin patch, 0.00..1.00
    int    patch_size[3];  // pixel context region width  (must be odd)
    int    range[3];       // spatial search window width (must be odd)
    int    nframes[3];     // temporal search depth in frames
    int    prefilter[3];   // prefilter mode, can improve weight analysis

    float  exptable[3][NLMEANS_EXPSIZE];
    float  weight_fact_table[3];
    int    diff_max[3];

    NLMeansFunctions functions;

    Frame      *frame;
    int         next_frame;
    int         max_frames;

    taskset_t   taskset;
    int         thread_count;
    nlmeans_thread_arg_t **thread_data;
};

static int nlmeans_init(hb_filter_object_t *filter, hb_filter_init_t *init);
static int nlmeans_work(hb_filter_object_t *filter,
                           hb_buffer_t **buf_in,
                           hb_buffer_t **buf_out);
static void nlmeans_close(hb_filter_object_t *filter);

static void nlmeans_filter_thread(void *thread_args_v);

hb_filter_object_t hb_filter_nlmeans =
{
    .id            = HB_FILTER_NLMEANS,
    .enforce_order = 1,
    .name          = "Denoise (nlmeans)",
    .settings      = NULL,
    .init          = nlmeans_init,
    .work          = nlmeans_work,
    .close         = nlmeans_close,
};

static void nlmeans_border(uint8_t *src,
                           const int w,
                           const int h,
                           const int border)
{
    const int bw = w + 2 * border;
    uint8_t *image = src + border + bw * border;

    // Create faux borders using edge pixels
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < border; x++)
        {
            *(image + y*bw - x - 1) = *(image + y*bw + x);
            *(image + y*bw + x + w) = *(image + y*bw - x + (w-1));
        }
    }
    for (int y = 0; y < border; y++)
    {
        memcpy(image - border - (y+1)*bw, image - border +       y*bw, bw);
        memcpy(image - border + (y+h)*bw, image - border + (h-y-1)*bw, bw);
    }

}

static void nlmeans_deborder(const BorderedPlane *src,
                             uint8_t *dst,
                             const int w,
                             const int s,
                             const int h)
{
    const int bw = src->w + 2 * src->border;
    uint8_t *image = src->mem + src->border + bw * src->border;

    int width = w;
    if (src->w < width)
    {
        width = src->w;
    }

    // Copy main image
    for (int y = 0; y < h; y++)
    {
        memcpy(dst + y * s, image + y * bw, width);
    }

}

static void nlmeans_alloc(const uint8_t *src,
                          const int src_w,
                          const int src_s,
                          const int src_h,
                          BorderedPlane *dst,
                          const int border)
{
    const int bw = src_w + 2 * border;
    const int bh = src_h + 2 * border;

    uint8_t *mem   = malloc(bw * bh * sizeof(uint8_t));
    uint8_t *image = mem + border + bw * border;

    // Copy main image
    for (int y = 0; y < src_h; y++)
    {
        memcpy(image + y * bw, src + y * src_s, src_w);
    }

    dst->mem       = mem;
    dst->image     = image;
    dst->w         = src_w;
    dst->h         = src_h;
    dst->border    = border;

    nlmeans_border(dst->mem, dst->w, dst->h, dst->border);
    dst->mem_pre   = dst->mem;
    dst->image_pre = dst->image;

}

static void nlmeans_filter_mean(const uint8_t *src,
                                      uint8_t *dst,
                                const int w,
                                const int h,
                                const int border,
                                const int size)
{

    // Mean filter
    const int bw = w + 2 * border;
    const int offset_min = -((size - 1) /2);
    const int offset_max =   (size + 1) /2;
    const double pixel_weight = 1.0 / (size * size);
    uint16_t pixel_sum;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            pixel_sum = 0;
            for (int k = offset_min; k < offset_max; k++)
            {
                for (int j = offset_min; j < offset_max; j++)
                {
                    pixel_sum = pixel_sum + *(src + bw*(y+j) + (x+k));
                }
            }
            *(dst + bw*y + x) = (uint8_t)(pixel_sum * pixel_weight);
        }
    }

}

static uint8_t nlmeans_filter_median_opt(uint8_t  *pixels,
                                         const int size)
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

static void nlmeans_filter_median(const uint8_t *src,
                                        uint8_t *dst,
                                  const int w,
                                  const int h,
                                  const int border,
                                  const int size)
{
    // Median filter
    const int bw = w + 2 * border;
    const int offset_min = -((size - 1) /2);
    const int offset_max =   (size + 1) /2;
    int index;
    uint8_t pixels[size * size];
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            index = 0;
            for (int k = offset_min; k < offset_max; k++)
            {
                for (int j = offset_min; j < offset_max; j++)
                {
                    pixels[index] = *(src + bw*(y+j) + (x+k));
                    index++;
                }
            }
            *(dst + bw*y + x) = nlmeans_filter_median_opt(pixels, size);
        }
    }

}

static void nlmeans_filter_edgeboost(const uint8_t *src,
                                           uint8_t *dst,
                                     const int w,
                                     const int h,
                                     const int border)
{
    const int bw = w + 2 * border;
    const int bh = h + 2 * border;

    // Custom kernel
    const int kernel_size = 3;
    const int kernel[3][3] = {{-31, 0, 31},
                              {-44, 0, 44},
                              {-31, 0, 31}};
    const double kernel_coef = 1.0 / 126.42;

    // Detect edges
    const int offset_min = -((kernel_size - 1) /2);
    const int offset_max =   (kernel_size + 1) /2;
    uint16_t pixel1;
    uint16_t pixel2;
    uint8_t *mask_mem = calloc(bw * bh, sizeof(uint8_t));
    uint8_t *mask = mask_mem + border + bw * border;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            pixel1 = 0;
            pixel2 = 0;
            for (int k = offset_min; k < offset_max; k++)
            {
                for (int j = offset_min; j < offset_max; j++)
                {
                    pixel1 += kernel[j+1][k+1] * *(src + bw*(y+j) + (x+k));
                    pixel2 += kernel[k+1][j+1] * *(src + bw*(y+j) + (x+k));
                }
            }
            pixel1 = pixel1 > 0 ? pixel1 : -pixel1;
            pixel2 = pixel2 > 0 ? pixel2 : -pixel2;
            pixel1 = (uint16_t)(((double)pixel1 * kernel_coef) + 128);
            pixel2 = (uint16_t)(((double)pixel2 * kernel_coef) + 128);
            *(mask + bw*y + x) = (uint8_t)(pixel1 + pixel2);
            if (*(mask + bw*y + x) > 160)
            {
                *(mask + bw*y + x) = 235;
            }
            else if (*(mask + bw*y + x) > 16)
            {
                *(mask + bw*y + x) = 128;
            }
            else
            {
                *(mask + bw*y + x) = 16;
            }
        }
    }

    // Post-process and output
    int pixels;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            if (*(mask + bw*y + x) > 16)
            {
                // Count nearby edge pixels
                pixels = 0;
                for (int k = offset_min; k < offset_max; k++)
                {
                    for (int j = offset_min; j < offset_max; j++)
                    {
                        if (*(mask + bw*(y+j) + (x+k)) > 16)
                        {
                            pixels++;
                        }
                    }
                }
                // Remove false positive
                if (pixels < 3)
                {
                    *(mask + bw*y + x) = 16;
                }
                // Filter output
                if (*(mask + bw*y + x) > 16)
                {
                    if (*(mask + bw*y + x) == 235)
                    {
                        *(dst + bw*y + x) = (3 * *(src + bw*y + x) + 1 * *(dst + bw*y + x)) /4;
                    }
                    else
                    {
                        *(dst + bw*y + x) = (2 * *(src + bw*y + x) + 3 * *(dst + bw*y + x)) /5;
                    }
                    //*(dst + bw*y + x) = *(mask + bw*y + x); // Overlay mask
                }
            }
            //*(dst + bw*y + x) = *(mask + bw*y + x); // Full mask
        }
    }

    free(mask_mem);
}

static void nlmeans_prefilter(BorderedPlane *src,
                              const int filter_type)
{
    hb_lock(src->mutex);
    if (src->prefiltered)
    {
        hb_unlock(src->mutex);
        return;
    }

    if (filter_type & NLMEANS_PREFILTER_MODE_MEAN3X3   ||
        filter_type & NLMEANS_PREFILTER_MODE_MEAN5X5   ||
        filter_type & NLMEANS_PREFILTER_MODE_MEDIAN3X3 ||
        filter_type & NLMEANS_PREFILTER_MODE_MEDIAN5X5)
    {

        // Source image
        const uint8_t *mem   = src->mem;
        const uint8_t *image = src->image;
        const int border     = src->border;
        const int w          = src->w;
        const int h          = src->h;
        const int bw         = w + 2 * border;
        const int bh         = h + 2 * border;

        // Duplicate plane
        uint8_t *mem_pre = malloc(bw * bh * sizeof(uint8_t));
        uint8_t *image_pre = mem_pre + border + bw * border;
        for (int y = 0; y < h; y++)
        {
            memcpy(mem_pre + y * bw, mem + y * bw, bw);
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
            for (int y = 0; y < bh; y++)
            {
                for (int x = 0; x < bw; x++)
                {
                    *(mem_pre + bw*y + x) = (uint8_t)((wet * *(mem_pre + bw*y + x) + dry * *(mem + bw*y + x)) / (wet + dry));
                }
            }
        }

        // Assign result
        src->mem_pre   = mem_pre;
        src->image_pre = image_pre;

        // Recreate borders
        nlmeans_border(mem_pre, w, h, border);

    }
    src->prefiltered = 1;
    hb_unlock(src->mutex);
}

static void build_integral_scalar(uint32_t *integral,
                                  int       integral_stride,
                            const uint8_t  *src,
                            const uint8_t  *src_pre,
                            const uint8_t  *compare,
                            const uint8_t  *compare_pre,
                                  int       w,
                                  int       border,
                                  int       dst_w,
                                  int       dst_h,
                                  int       dx,
                                  int       dy)
{
    const int bw = w + 2 * border;
    for (int y = 0; y < dst_h; y++)
    {
        const uint8_t *p1 = src_pre + y*bw;
        const uint8_t *p2 = compare_pre + (y+dy)*bw + dx;
        uint32_t *out = integral + (y*integral_stride);

        for (int x = 0; x < dst_w; x++)
        {
            int diff = *p1 - *p2;
            *out = *(out-1) + diff * diff;
            out++;
            p1++;
            p2++;
        }

        if (y > 0)
        {
            out = integral + y*integral_stride;

            for (int x = 0; x < dst_w; x++)
            {
                *out += *(out - integral_stride);
                out++;
            }
        }
    }
}

static void nlmeans_plane(NLMeansFunctions *functions,
                          Frame *frame,
                          int prefilter,
                          int plane,
                          int nframes,
                          uint8_t *dst,
                          int dst_w,
                          int dst_s,
                          int dst_h,
                          double h_param,
                          double origin_tune,
                          int n,
                          int r,
                    const float *exptable,
                    const float  weight_fact_table,
                    const int    diff_max)
{
    const int n_half = (n-1) /2;
    const int r_half = (r-1) /2;

    // Source image
    const uint8_t *src     = frame[0].plane[plane].image;
    const uint8_t *src_pre = frame[0].plane[plane].image_pre;
    const int w      = frame[0].plane[plane].w;
    const int border = frame[0].plane[plane].border;
    const int bw     = w + 2 * border;

    // Allocate temporary pixel sums
    struct PixelSum *tmp_data = calloc(dst_w * dst_h, sizeof(struct PixelSum));

    // Allocate integral image
    const int integral_stride    = ((dst_w + 15) / 16 * 16) + 2 * 16;
    uint32_t* const integral_mem = calloc(integral_stride * (dst_h+1), sizeof(uint32_t));
    uint32_t* const integral     = integral_mem + integral_stride + 16;

    // Iterate through available frames
    for (int f = 0; f < nframes; f++)
    {
        nlmeans_prefilter(&frame[f].plane[plane], prefilter);

        // Compare image
        const uint8_t *compare     = frame[f].plane[plane].image;
        const uint8_t *compare_pre = frame[f].plane[plane].image_pre;

        // Iterate through all displacements
        for (int dy = -r_half; dy <= r_half; dy++)
        {
            for (int dx = -r_half; dx <= r_half; dx++)
            {

                // Apply special weight tuning to origin patch
                if (dx == 0 && dy == 0 && f == 0)
                {
                    // TODO: Parallelize this
                    for (int y = n_half; y < dst_h-n + n_half; y++)
                    {
                        for (int x = n_half; x < dst_w-n + n_half; x++)
                        {
                            tmp_data[y*dst_w + x].weight_sum += origin_tune;
                            tmp_data[y*dst_w + x].pixel_sum  += origin_tune * src[y*bw + x];
                        }
                    }
                    continue;
                }

                // Build integral
                functions->build_integral(integral,
                                          integral_stride,
                                          src,
                                          src_pre,
                                          compare,
                                          compare_pre,
                                          w,
                                          border,
                                          dst_w,
                                          dst_h,
                                          dx,
                                          dy);

                // Average displacement
                // TODO: Parallelize this
                for (int y = 0; y <= dst_h-n; y++)
                {
                    const uint32_t *integral_ptr1 = integral + (y  -1)*integral_stride - 1;
                    const uint32_t *integral_ptr2 = integral + (y+n-1)*integral_stride - 1;

                    for (int x = 0; x <= dst_w-n; x++)
                    {
                        const int xc = x + n_half;
                        const int yc = y + n_half;

                        // Difference between patches
                        const int diff = (uint32_t)(integral_ptr2[n] - integral_ptr2[0] - integral_ptr1[n] + integral_ptr1[0]);

                        // Sum pixel with weight
                        if (diff < diff_max)
                        {
                            const int diffidx = diff * weight_fact_table;

                            //float weight = exp(-diff*weightFact);
                            const float weight = exptable[diffidx];

                            tmp_data[yc*dst_w + xc].weight_sum += weight;
                            tmp_data[yc*dst_w + xc].pixel_sum  += weight * compare[(yc+dy)*bw + xc + dx];
                        }

                        integral_ptr1++;
                        integral_ptr2++;
                    }
                }
            }
        }
    }

    // Copy edges
    for (int y = 0; y < dst_h; y++)
    {
        for (int x = 0; x < n_half; x++)
        {
            *(dst + y * dst_s + x)               = *(src + y * bw - x - 1);
            *(dst + y * dst_s - x + (dst_w - 1)) = *(src + y * bw + x + dst_w);
        }
    }
    for (int y = 0; y < n_half; y++)
    {
        memcpy(dst +           y*dst_s, src -     (y+1)*bw, dst_w);
        memcpy(dst + (dst_h-y-1)*dst_s, src + (y+dst_h)*bw, dst_w);
    }

    // Copy main image
    uint8_t result;
    for (int y = n_half; y < dst_h-n_half; y++)
    {
        for (int x = n_half; x < dst_w-n_half; x++)
        {
            result = (uint8_t)(tmp_data[y*dst_w + x].pixel_sum / tmp_data[y*dst_w + x].weight_sum);
            *(dst + y*dst_s + x) = result ? result : *(src + y*bw + x);
        }
    }

    free(tmp_data);
    free(integral_mem);

}

static int nlmeans_init(hb_filter_object_t *filter,
                           hb_filter_init_t *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    hb_filter_private_t *pv = filter->private_data;
    NLMeansFunctions *functions = &pv->functions;

    functions->build_integral = build_integral_scalar;
#if defined(ARCH_X86)
    nlmeans_init_x86(functions);
#endif

    // Mark parameters unset
    for (int c = 0; c < 3; c++)
    {
        pv->strength[c]    = -1;
        pv->origin_tune[c] = -1;
        pv->patch_size[c]  = -1;
        pv->range[c]       = -1;
        pv->nframes[c]     = -1;
        pv->prefilter[c]   = -1;
    }

    // Read user parameters
    if (filter->settings != NULL)
    {
        sscanf(filter->settings, "%lf:%lf:%d:%d:%d:%d:%lf:%lf:%d:%d:%d:%d:%lf:%lf:%d:%d:%d:%d",
               &pv->strength[0], &pv->origin_tune[0], &pv->patch_size[0], &pv->range[0], &pv->nframes[0], &pv->prefilter[0],
               &pv->strength[1], &pv->origin_tune[1], &pv->patch_size[1], &pv->range[1], &pv->nframes[1], &pv->prefilter[1],
               &pv->strength[2], &pv->origin_tune[2], &pv->patch_size[2], &pv->range[2], &pv->nframes[2], &pv->prefilter[2]);
    }

    // Cascade values
    // Cr not set; inherit Cb. Cb not set; inherit Y. Y not set; defaults.
    for (int c = 1; c < 3; c++)
    {
        if (pv->strength[c]    == -1) { pv->strength[c]    = pv->strength[c-1]; }
        if (pv->origin_tune[c] == -1) { pv->origin_tune[c] = pv->origin_tune[c-1]; }
        if (pv->patch_size[c]  == -1) { pv->patch_size[c]  = pv->patch_size[c-1]; }
        if (pv->range[c]       == -1) { pv->range[c]       = pv->range[c-1]; }
        if (pv->nframes[c]     == -1) { pv->nframes[c]     = pv->nframes[c-1]; }
        if (pv->prefilter[c]   == -1) { pv->prefilter[c]   = pv->prefilter[c-1]; }
    }

    for (int c = 0; c < 3; c++)
    {
        // Replace unset values with defaults
        if (pv->strength[c]    == -1) { pv->strength[c]    = c ? NLMEANS_STRENGTH_LUMA_DEFAULT    : NLMEANS_STRENGTH_CHROMA_DEFAULT; }
        if (pv->origin_tune[c] == -1) { pv->origin_tune[c] = c ? NLMEANS_ORIGIN_TUNE_LUMA_DEFAULT : NLMEANS_ORIGIN_TUNE_CHROMA_DEFAULT; }
        if (pv->patch_size[c]  == -1) { pv->patch_size[c]  = c ? NLMEANS_PATCH_SIZE_LUMA_DEFAULT  : NLMEANS_PATCH_SIZE_CHROMA_DEFAULT; }
        if (pv->range[c]       == -1) { pv->range[c]       = c ? NLMEANS_RANGE_LUMA_DEFAULT       : NLMEANS_RANGE_CHROMA_DEFAULT; }
        if (pv->nframes[c]     == -1) { pv->nframes[c]     = c ? NLMEANS_FRAMES_LUMA_DEFAULT      : NLMEANS_FRAMES_CHROMA_DEFAULT; }
        if (pv->prefilter[c]   == -1) { pv->prefilter[c]   = c ? NLMEANS_PREFILTER_LUMA_DEFAULT   : NLMEANS_PREFILTER_CHROMA_DEFAULT; }

        // Sanitize
        if (pv->strength[c] < 0)        { pv->strength[c] = 0; }
        if (pv->origin_tune[c] < 0.01)  { pv->origin_tune[c] = 0.01; } // avoid black artifacts
        if (pv->origin_tune[c] > 1)     { pv->origin_tune[c] = 1; }
        if (pv->patch_size[c] % 2 == 0) { pv->patch_size[c]--; }
        if (pv->patch_size[c] < 1)      { pv->patch_size[c] = 1; }
        if (pv->range[c] % 2 == 0)      { pv->range[c]--; }
        if (pv->range[c] < 1)           { pv->range[c] = 1; }
        if (pv->nframes[c] < 1)         { pv->nframes[c] = 1; }
        if (pv->nframes[c] > NLMEANS_FRAMES_MAX) { pv->nframes[c] = NLMEANS_FRAMES_MAX; }
        if (pv->prefilter[c] < 0)       { pv->prefilter[c] = 0; }

        if (pv->max_frames < pv->nframes[c]) pv->max_frames = pv->nframes[c];

        // Precompute exponential table
        float *exptable = &pv->exptable[c][0];
        float *weight_fact_table = &pv->weight_fact_table[c];
        int   *diff_max = &pv->diff_max[c];
        const float weight_factor        = 1.0/pv->patch_size[c]/pv->patch_size[c] / (pv->strength[c] * pv->strength[c]);
        const float min_weight_in_table  = 0.0005;
        const float stretch              = NLMEANS_EXPSIZE / (-log(min_weight_in_table));
        *(weight_fact_table)             = weight_factor * stretch;
        *(diff_max)                      = NLMEANS_EXPSIZE / *(weight_fact_table);
        for (int i = 0; i < NLMEANS_EXPSIZE; i++)
        {
            exptable[i] = exp(-i/stretch);
        }
        exptable[NLMEANS_EXPSIZE-1] = 0;
    }

    pv->thread_count = hb_get_cpu_count();
    pv->frame = calloc(pv->thread_count + pv->max_frames, sizeof(Frame));
    for (int ii = 0; ii < pv->thread_count + pv->max_frames; ii++)
    {
        for (int c = 0; c < 3; c++)
        {
            pv->frame[ii].plane[c].mutex = hb_lock_init();
        }
    }

    pv->thread_data = malloc(pv->thread_count * sizeof(nlmeans_thread_arg_t*));
    if (taskset_init(&pv->taskset, pv->thread_count,
                     sizeof(nlmeans_thread_arg_t)) == 0)
    {
        hb_error("NLMeans could not initialize taskset");
        goto fail;
    }

    for (int ii = 0; ii < pv->thread_count; ii++)
    {
        pv->thread_data[ii] = taskset_thread_args(&pv->taskset, ii);
        if (pv->thread_data[ii] == NULL)
        {
            hb_error("NLMeans could not create thread args");
            goto fail;
        }
        pv->thread_data[ii]->pv = pv;
        pv->thread_data[ii]->segment = ii;
        if (taskset_thread_spawn(&pv->taskset, ii, "nlmeans_filter",
                                 nlmeans_filter_thread, HB_NORMAL_PRIORITY) == 0)
        {
            hb_error("NLMeans could not spawn thread");
            goto fail;
        }
    }

    return 0;

fail:
    taskset_fini(&pv->taskset);
    free(pv->thread_data);
    free(pv);
    return -1;
}

static void nlmeans_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    taskset_fini(&pv->taskset);
    for (int c = 0; c < 3; c++)
    {
        for (int f = 0; f < pv->nframes[c]; f++)
        {
            if (pv->frame[f].plane[c].mem_pre != NULL &&
                pv->frame[f].plane[c].mem_pre != pv->frame[f].plane[c].mem)
            {
                free(pv->frame[f].plane[c].mem_pre);
                pv->frame[f].plane[c].mem_pre = NULL;
            }
            if (pv->frame[f].plane[c].mem != NULL)
            {
                free(pv->frame[f].plane[c].mem);
                pv->frame[f].plane[c].mem = NULL;
            }
        }
    }

    for (int ii = 0; ii < pv->thread_count + pv->max_frames; ii++)
    {
        for (int c = 0; c < 3; c++)
        {
            hb_lock_close(&pv->frame[ii].plane[c].mutex);
        }
    }

    free(pv->frame);
    free(pv->thread_data);
    free(pv);
    filter->private_data = NULL;
}

static void nlmeans_filter_thread(void *thread_args_v)
{
    nlmeans_thread_arg_t *thread_data = thread_args_v;
    hb_filter_private_t *pv = thread_data->pv;
    int segment = thread_data->segment;

    hb_log("NLMeans thread started for segment %d", segment);

    while (1)
    {
        // Wait until there is work to do.
        taskset_thread_wait4start(&pv->taskset, segment);

        if (taskset_thread_stop(&pv->taskset, segment))
        {
            break;
        }

        Frame *frame = &pv->frame[segment];
        hb_buffer_t *buf;
        buf = hb_frame_buffer_init(frame->fmt, frame->width, frame->height);

        NLMeansFunctions *functions = &pv->functions;

        for (int c = 0; c < 3; c++)
        {
            if (pv->strength[c] == 0)
            {
                nlmeans_deborder(&frame->plane[c], buf->plane[c].data,
                                 buf->plane[c].width, buf->plane[c].stride,
                                 buf->plane[c].height);
                continue;
            }
            if (pv->prefilter[c] & NLMEANS_PREFILTER_MODE_PASSTHRU)
            {
                nlmeans_prefilter(&pv->frame->plane[c], pv->prefilter[c]);
                nlmeans_deborder(&frame->plane[c], buf->plane[c].data,
                                 buf->plane[c].width, buf->plane[c].stride,
                                 buf->plane[c].height);
                continue;
            }

            // Process current plane
            nlmeans_plane(functions,
                          frame,
                          pv->prefilter[c],
                          c,
                          pv->nframes[c],
                          buf->plane[c].data,
                          buf->plane[c].width,
                          buf->plane[c].stride,
                          buf->plane[c].height,
                          pv->strength[c],
                          pv->origin_tune[c],
                          pv->patch_size[c],
                          pv->range[c],
                          pv->exptable[c],
                          pv->weight_fact_table[c],
                          pv->diff_max[c]);
        }
        buf->s = pv->frame[segment].s;
        thread_data->out = buf;

        // Finished this segment, notify.
        taskset_thread_complete(&pv->taskset, segment);
    }
    taskset_thread_complete(&pv->taskset, segment);
}

static void nlmeans_add_frame(hb_filter_private_t *pv, hb_buffer_t *buf)
{
    for (int c = 0; c < 3; c++)
    {
        // Extend copy of plane with extra border and place in buffer
        const int border = ((pv->range[c] + 2) / 2 + 15) / 16 * 16;
        nlmeans_alloc(buf->plane[c].data,
                      buf->plane[c].width,
                      buf->plane[c].stride,
                      buf->plane[c].height,
                      &pv->frame[pv->next_frame].plane[c],
                      border);
        pv->frame[pv->next_frame].s = buf->s;
        pv->frame[pv->next_frame].width = buf->f.width;
        pv->frame[pv->next_frame].height = buf->f.height;
        pv->frame[pv->next_frame].fmt = buf->f.fmt;
    }
    pv->next_frame++;
}

static hb_buffer_t * nlmeans_filter(hb_filter_private_t *pv)
{
    if (pv->next_frame < pv->max_frames + pv->thread_count)
    {
        return NULL;
    }

    taskset_cycle(&pv->taskset);

    // Free buffers that are not needed for next taskset cycle
    for (int c = 0; c < 3; c++)
    {
        for (int t = 0; t < pv->thread_count; t++)
        {
            // Release last frame in buffer
            if (pv->frame[t].plane[c].mem_pre != NULL &&
                pv->frame[t].plane[c].mem_pre != pv->frame[t].plane[c].mem)
            {
                free(pv->frame[t].plane[c].mem_pre);
                pv->frame[t].plane[c].mem_pre = NULL;
            }
            if (pv->frame[t].plane[c].mem != NULL)
            {
                free(pv->frame[t].plane[c].mem);
                pv->frame[t].plane[c].mem = NULL;
            }
        }
    }
    // Shift frames in buffer down
    for (int f = 0; f < pv->max_frames; f++)
    {
        // Don't move the mutex!
        Frame frame = pv->frame[f];
        pv->frame[f] = pv->frame[f+pv->thread_count];
        for (int c = 0; c < 3; c++)
        {
            pv->frame[f].plane[c].mutex = frame.plane[c].mutex;
            pv->frame[f+pv->thread_count].plane[c].mem_pre = NULL;
            pv->frame[f+pv->thread_count].plane[c].mem = NULL;
        }
    }
    pv->next_frame -= pv->thread_count;

    // Collect results from taskset
    hb_buffer_list_t list;
    hb_buffer_list_clear(&list);
    for (int t = 0; t < pv->thread_count; t++)
    {
        hb_buffer_list_append(&list, pv->thread_data[t]->out);
    }
    return hb_buffer_list_clear(&list);
}

static hb_buffer_t * nlmeans_filter_flush(hb_filter_private_t *pv)
{
    hb_buffer_list_t list;

    hb_buffer_list_clear(&list);
    for (int f = 0; f < pv->next_frame; f++)
    {
        Frame *frame = &pv->frame[f];
        hb_buffer_t *buf;
        buf = hb_frame_buffer_init(frame->fmt, frame->width, frame->height);

        NLMeansFunctions *functions = &pv->functions;

        for (int c = 0; c < 3; c++)
        {
            if (pv->strength[c] == 0)
            {
                nlmeans_deborder(&frame->plane[c], buf->plane[c].data,
                                 buf->plane[c].width, buf->plane[c].stride,
                                 buf->plane[c].height);
                continue;
            }
            if (pv->prefilter[c] & NLMEANS_PREFILTER_MODE_PASSTHRU)
            {
                nlmeans_prefilter(&pv->frame[f].plane[c], pv->prefilter[c]);
                nlmeans_deborder(&frame->plane[c], buf->plane[c].data,
                                 buf->plane[c].width, buf->plane[c].stride,
                                 buf->plane[c].height);
                continue;
            }

            int nframes = pv->next_frame - f;
            if (pv->nframes[c] < nframes)
            {
                nframes = pv->nframes[c];
            }
            // Process current plane
            nlmeans_plane(functions,
                          frame,
                          pv->prefilter[c],
                          c,
                          nframes,
                          buf->plane[c].data,
                          buf->plane[c].width,
                          buf->plane[c].stride,
                          buf->plane[c].height,
                          pv->strength[c],
                          pv->origin_tune[c],
                          pv->patch_size[c],
                          pv->range[c],
                          pv->exptable[c],
                          pv->weight_fact_table[c],
                          pv->diff_max[c]);
        }
        buf->s = frame->s;
        hb_buffer_list_append(&list, buf);
    }
    return hb_buffer_list_clear(&list);
}

static int nlmeans_work(hb_filter_object_t *filter,
                           hb_buffer_t **buf_in,
                           hb_buffer_t **buf_out )
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        hb_buffer_list_t list;
        hb_buffer_t *buf;

        // Flush buffered frames
        buf = nlmeans_filter_flush(pv);
        hb_buffer_list_set(&list, buf);

        // And terminate the buffer list with a EOF buffer
        hb_buffer_list_append(&list, in);
        *buf_out = hb_buffer_list_clear(&list);

        *buf_in  = NULL;
        return HB_FILTER_DONE;
    }

    nlmeans_add_frame(pv, in);
    *buf_out = nlmeans_filter(pv);

    return HB_FILTER_OK;
}
