/* common.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#if BIT_DEPTH > 8
#   define pixel   uint16_t
#   define pixel_2 uint32_t
#   define FUNC(name) name##_##16
#else
#   define pixel   uint8_t
#   define pixel_2 uint16_t
#   define FUNC(name) name##_##8
#endif

static void FUNC(nlmeans_border)(pixel *src,
                                 const int w,
                                 const int h,
                                 const int border)
{
    const int bw = w + 2 * border;
    pixel *image = src + border + bw * border;

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
        memcpy(image - border - (y+1)*bw, image - border +       y*bw, bw * sizeof(pixel));
        memcpy(image - border + (y+h)*bw, image - border + (h-y-1)*bw, bw * sizeof(pixel));
    }

}

static void FUNC(nlmeans_deborder)(const BorderedPlane *src,
                                   void *in_dst,
                                   const int w,
                                   const int s,
                                   const int h)
{
    pixel *dst = in_dst;
    const int bw = src->w + 2 * src->border;
    pixel *image = src->mem + src->border + bw * src->border;

    int width = w;
    if (src->w < width)
    {
        width = src->w;
    }

    // Copy main image
    for (int y = 0; y < h; y++)
    {
        memcpy(dst + y * s, image + y * bw, width * sizeof(pixel));
    }

}

static void FUNC(nlmeans_alloc)(const void *in_src,
                                const int src_w,
                                const int src_s,
                                const int src_h,
                                BorderedPlane *dst,
                                const int border)
{
    const int bw = src_w + 2 * border;
    const int bh = src_h + 2 * border;

    const pixel *src = in_src;

    pixel *mem   = malloc(bw * bh * sizeof(pixel));
    pixel *image = mem + border + bw * border;

    // Copy main image
    for (int y = 0; y < src_h; y++)
    {
        memcpy(image + y * bw, src + y * src_s, src_w * sizeof(pixel));
    }

    dst->mem       = mem;
    dst->image     = image;
    dst->w         = src_w;
    dst->h         = src_h;
    dst->border    = border;

    FUNC(nlmeans_border)(dst->mem, dst->w, dst->h, dst->border);
    dst->mem_pre     = dst->mem;
    dst->image_pre   = dst->image;
    dst->prefiltered = 0;

}

static void FUNC(nlmeans_filter_mean)(const pixel *src,
                                            pixel *dst,
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
    pixel_2 pixel_sum;
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
            *(dst + bw*y + x) = (pixel)(pixel_sum * pixel_weight);
        }
    }

}

static pixel FUNC(nlmeans_filter_median_opt)(pixel  *pixels,
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

static void FUNC(nlmeans_filter_median)(const pixel *src,
                                              pixel *dst,
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
    pixel pixels[size * size];
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
            *(dst + bw*y + x) = FUNC(nlmeans_filter_median_opt)(pixels, size);
        }
    }

}

static void FUNC(nlmeans_filter_csm)(const pixel *src,
                                           pixel *dst,
                                     const int w,
                                     const int h,
                                     const int border,
                                     const int size)
{
    // CSM filter
    const int bw = w + 2 * border;
    const int offset_min = -((size - 1) /2);
    const int offset_max =   (size + 1) /2;
    pixel min,  max,
          min2, max2,
          min3, max3,
          median,
          pixel;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            for (int k = offset_min; k < offset_max; k++)
            {
                for (int j = offset_min; j < offset_max; j++)
                {
                    if (k == 0 && j == 0)
                    {
                        // Ignore origin
                        goto end;
                    }
                    pixel = *(src + bw*(y+j) + (x+k));
                    if (k == offset_min && j == offset_min)
                    {
                        // Start calculating neighborhood thresholds
                        min = pixel;
                        max = min;
                        goto end;
                    }
                    if (pixel < min)
                    {
                        min = pixel;
                    }
                    if (pixel > max)
                    {
                        max = pixel;
                    }
                }
                end:
                    continue;
            }

            // Final neighborhood thresholds
            // min = minimum neighbor pixel value
            // max = maximum neighbor pixel value

            // Median
            median = (min + max) / 2;

            // Additional thresholds for median-like filtering
            min2 = (min + median) / 2;
            max2 = (max + median) / 2;
            min3 = (min2 + median) / 2;
            max3 = (max2 + median) / 2;

            // Clamp to thresholds
            pixel = *(src + bw*(y) + (x));
            if (pixel < min)
            {
                *(dst + bw*y + x) = min;
            }
            else if (pixel > max)
            {
                *(dst + bw*y + x) = max;
            }
            else if (pixel < min2)
            {
                *(dst + bw*y + x) = min2;
            }
            else if (pixel > max2)
            {
                *(dst + bw*y + x) = max2;
            }
            else if (pixel < min3)
            {
                *(dst + bw*y + x) = min3;
            }
            else if (pixel > max3)
            {
                *(dst + bw*y + x) = max3;
            }
        }
    }
}

static void FUNC(nlmeans_filter_edgeboost)(const pixel *src,
                                                 pixel *dst,
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
    pixel_2 pixel1;
    pixel_2 pixel2;
    pixel *mask_mem = calloc(bw * bh, sizeof(pixel));
    pixel *mask = mask_mem + border + bw * border;
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
            pixel1 = (pixel_2)(((double)pixel1 * kernel_coef) + 128);
            pixel2 = (pixel_2)(((double)pixel2 * kernel_coef) + 128);
            *(mask + bw*y + x) = (pixel)(pixel1 + pixel2);
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

static void FUNC(nlmeans_prefilter)(BorderedPlane *src,
                                    const int filter_type)
{
    hb_lock(src->mutex);
    if (src->prefiltered == 1)
    {
        hb_unlock(src->mutex);
        return;
    }

    if (filter_type & NLMEANS_PREFILTER_MODE_MEAN3X3   ||
        filter_type & NLMEANS_PREFILTER_MODE_MEAN5X5   ||
        filter_type & NLMEANS_PREFILTER_MODE_MEDIAN3X3 ||
        filter_type & NLMEANS_PREFILTER_MODE_MEDIAN5X5 ||
        filter_type & NLMEANS_PREFILTER_MODE_CSM3X3    ||
        filter_type & NLMEANS_PREFILTER_MODE_CSM5X5)
    {

        // Source image
        const pixel *mem   = src->mem;
        const pixel *image = src->image;
        const int border     = src->border;
        const int w          = src->w;
        const int h          = src->h;
        const int bw         = w + 2 * border;
        const int bh         = h + 2 * border;

        // Duplicate plane
        pixel *mem_pre = malloc(bw * bh * sizeof(pixel));
        pixel *image_pre = mem_pre + border + bw * border;
        memcpy(mem_pre, mem, bw * bh * sizeof(pixel));

        // Filter plane; should already have at least 2px extra border on each side
        if (filter_type & NLMEANS_PREFILTER_MODE_CSM5X5)
        {
            // CSM 5x5
            FUNC(nlmeans_filter_csm)(image, image_pre, w, h, border, 5);
        }
        else if (filter_type & NLMEANS_PREFILTER_MODE_CSM3X3)
        {
            // CSM 3x3
            FUNC(nlmeans_filter_csm)(image, image_pre, w, h, border, 3);
        }
        else if (filter_type & NLMEANS_PREFILTER_MODE_MEDIAN5X5)
        {
            // Median 5x5
            FUNC(nlmeans_filter_median)(image, image_pre, w, h, border, 5);
        }
        else if (filter_type & NLMEANS_PREFILTER_MODE_MEDIAN3X3)
        {
            // Median 3x3
            FUNC(nlmeans_filter_median)(image, image_pre, w, h, border, 3);
        }
        else if (filter_type & NLMEANS_PREFILTER_MODE_MEAN5X5)
        {
            // Mean 5x5
            FUNC(nlmeans_filter_mean)(image, image_pre, w, h, border, 5);
        }
        else if (filter_type & NLMEANS_PREFILTER_MODE_MEAN3X3)
        {
            // Mean 3x3
            FUNC(nlmeans_filter_mean)(image, image_pre, w, h, border, 3);
        }

        // Restore edges
        if (filter_type & NLMEANS_PREFILTER_MODE_EDGEBOOST)
        {
            FUNC(nlmeans_filter_edgeboost)(image, image_pre, w, h, border);
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
                    *(mem_pre + bw*y + x) = (pixel)((wet * *(mem_pre + bw*y + x) + dry * *(mem + bw*y + x)) / (wet + dry));
                }
            }
        }

        // Recreate borders
        FUNC(nlmeans_border)(mem_pre, w, h, border);

        // Assign result
        src->mem_pre   = mem_pre;
        src->image_pre = image_pre;
        if (filter_type & NLMEANS_PREFILTER_MODE_PASSTHRU)
        {
            src->mem   = src->mem_pre;
            src->image = src->image_pre;
        }

    }
    src->prefiltered = 1;
    hb_unlock(src->mutex);
}

static void FUNC(build_integral_scalar)(uint32_t *integral,
                                        int       integral_stride,
                                  const void  *in_src,
                                  const void  *in_src_pre,
                                  const void  *in_compare,
                                  const void  *in_compare_pre,
                                        int    w,
                                        int    border,
                                        int    dst_w,
                                        int    dst_h,
                                        int    dx,
                                        int    dy,
                                        int    n)
{
    const int bw = w + 2 * border;
    const int n_half = (n-1) /2;

    const pixel *src_pre      = (const pixel *)in_src_pre;
    const pixel *compare_pre  = (const pixel *)in_compare_pre;

    for (int y = 0; y < dst_h + n; y++)
    {
        const pixel *p1 = src_pre     + (y-n_half   )*bw - n_half;
        const pixel *p2 = compare_pre + (y-n_half+dy)*bw - n_half + dx;
        uint32_t *out = integral + (y*integral_stride);

        for (int x = 0; x < dst_w + n; x++)
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

            for (int x = 0; x < dst_w + n; x++)
            {
                *out += *(out - integral_stride);
                out++;
            }
        }
    }
}

static void FUNC(nlmeans_plane)(NLMeansFunctions *functions,
                                Frame *frame,
                                int prefilter,
                                int plane,
                                int nframes,
                                void *in_dst,
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
    pixel *dst = in_dst;
    const int r_half = (r-1) /2;

    // Source image
    const pixel *src     = frame[0].plane[plane].image;
    const pixel *src_pre = frame[0].plane[plane].image_pre;
    const int w      = frame[0].plane[plane].w;
    const int border = frame[0].plane[plane].border;
    const int bw     = w + 2 * border;

    // Allocate temporary pixel sums
    struct PixelSum *tmp_data = calloc(dst_w * dst_h, sizeof(struct PixelSum));

    // Allocate integral image
    const int integral_stride    = ((dst_w + n + 15) / 16 * 16) + 2 * 16;
    uint32_t* const integral_mem = calloc(integral_stride * (dst_h + n + 1), sizeof(uint32_t));
    uint32_t* const integral     = integral_mem + integral_stride + 16;

    // Iterate through available frames
    for (int f = 0; f < nframes; f++)
    {
        FUNC(nlmeans_prefilter)(&frame[f].plane[plane], prefilter);

        // Compare image
        const pixel *compare     = frame[f].plane[plane].image;
        const pixel *compare_pre = frame[f].plane[plane].image_pre;

        // Iterate through all displacements
        for (int dy = -r_half; dy <= r_half; dy++)
        {
            for (int dx = -r_half; dx <= r_half; dx++)
            {

                // Apply special weight tuning to origin patch
                if (dx == 0 && dy == 0 && f == 0)
                {
                    for (int y = 0; y < dst_h; y++)
                    {
                        for (int x = 0; x < dst_w; x++)
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
                                          dy,
                                          n);

                // Average displacement
                for (int y = 0; y < dst_h; y++)
                {
                    const uint32_t *integral_ptr1 = integral + (y  -1)*integral_stride - 1;
                    const uint32_t *integral_ptr2 = integral + (y+n-1)*integral_stride - 1;

                    for (int x = 0; x < dst_w; x++)
                    {

                        // Difference between patches
                        const int diff = (uint32_t)(integral_ptr2[n] - integral_ptr2[0] - integral_ptr1[n] + integral_ptr1[0]);

                        // Sum pixel with weight
                        if (diff < diff_max)
                        {
                            const int diffidx = diff * weight_fact_table;

                            //float weight = exp(-diff*weightFact);
                            const float weight = exptable[diffidx];

                            tmp_data[y*dst_w + x].weight_sum += weight;
                            tmp_data[y*dst_w + x].pixel_sum  += weight * compare[(y+dy)*bw + x + dx];
                        }

                        integral_ptr1++;
                        integral_ptr2++;
                    }
                }
            }
        }
    }

    // Copy image without border
    pixel result;
    for (int y = 0; y < dst_h; y++)
    {
        for (int x = 0; x < dst_w; x++)
        {
            result = (pixel)(tmp_data[y*dst_w + x].pixel_sum / tmp_data[y*dst_w + x].weight_sum);
            *(dst + y*dst_s + x) = result ? result : *(src + y*bw + x);
        }
    }

    free(tmp_data);
    free(integral_mem);
}

#undef pixel_2
#undef pixel
#undef FUNC
