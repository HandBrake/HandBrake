/* nlmeans_x86.c

   Copyright (c) 2013 Dirk Farin
   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"     // needed for ARCH_X86

#if defined(ARCH_X86)

#include <smmintrin.h>

#include "libavutil/cpu.h"
#include "handbrake/nlmeans.h"

ATTR_TARGET_SSE2
static void build_integral_8_sse2(void *integral,
                                  int   integral_stride,
                            const void *in_src,
                            const void *in_src_pre,
                            const void *in_compare,
                            const void *in_compare_pre,
                                  int   w,
                                  int   border,
                                  int   dst_w,
                                  int   dst_h,
                                  int   dx,
                                  int   dy,
                                  int   n)
{
    const __m128i zero = _mm_setzero_si128();
    const int bw = w + 2 * border;
    const int n_half = (n-1) /2;

    const uint8_t *src_pre     = (const uint8_t *)(in_src_pre);
    const uint8_t *compare_pre = (const uint8_t *)(in_compare_pre);

    for (int y = 0; y < dst_h + n; y++)
    {
        __m128i sum_prev = _mm_setzero_si128();

        const uint8_t *pixel_src     = src_pre     + (y-n_half   )*bw - n_half;
        const uint8_t *pixel_compare = compare_pre + (y-n_half+dy)*bw - n_half + dx;
        uint32_t *out = (uint32_t *)(integral) + (y*integral_stride);

        for (int x = 0; x < dst_w + n; x += 16)
        {
            __m128i p1, p1_lo, p1_hi;
            __m128i p2, p2_lo, p2_hi;
            __m128i df_lo, df_hi;
            __m128i sq_lo, sq_hi;
            __m128i sum_grp0, sum_grp1, sum_grp2, sum_grp3, sum_tmp;

            // Load multiple 8-bit source and compare pixels into separate 128-bit registers
            p1 = _mm_loadu_si128((__m128i*)(pixel_src));
            p2 = _mm_loadu_si128((__m128i*)(pixel_compare));

            // Unpack low half of pixels to 16-bit lanes and interleave with zeros
            p1_lo = _mm_unpacklo_epi8(p1, zero);
            p2_lo = _mm_unpacklo_epi8(p2, zero);

            // Square the difference between pixel values
            df_lo = _mm_sub_epi16(p1_lo, p2_lo);
            sq_lo = _mm_mullo_epi16(df_lo, df_lo);

            // Unpack low and high halves of squared diff to 32-bit lanes and interleave with zeros
            sum_grp0 = _mm_unpacklo_epi16(sq_lo, zero);
            sum_grp1 = _mm_unpackhi_epi16(sq_lo, zero);

            // Compute prefix scan of the 4 pixels in the low half of each 32-bit lane and accumulate
            sum_grp0 = _mm_add_epi32(sum_grp0, _mm_slli_si128(sum_grp0, 4));
            sum_grp0 = _mm_add_epi32(sum_grp0, _mm_slli_si128(sum_grp0, 8));
            sum_grp0 = _mm_add_epi32(sum_grp0, sum_prev);
            sum_tmp  = _mm_shuffle_epi32(sum_grp0, _MM_SHUFFLE(3, 3, 3, 3));

            // Compute prefix scan of the 4 pixels in the high half of each 32-bit lane and accumulate
            sum_grp1 = _mm_add_epi32(sum_grp1, _mm_slli_si128(sum_grp1, 4));
            sum_grp1 = _mm_add_epi32(sum_grp1, _mm_slli_si128(sum_grp1, 8));
            sum_grp1 = _mm_add_epi32(sum_grp1, sum_tmp);
            sum_prev = _mm_shuffle_epi32(sum_grp1, _MM_SHUFFLE(3, 3, 3, 3));

            // Unpack high half of pixels to 16-bit lanes and interleave with zeros
            p1_hi = _mm_unpackhi_epi8(p1, zero);
            p2_hi = _mm_unpackhi_epi8(p2, zero);

            // Square the difference between pixel values
            df_hi = _mm_sub_epi16(p1_hi, p2_hi);
            sq_hi = _mm_mullo_epi16(df_hi, df_hi);

            // Unpack low and high halves of squared diff to 32-bit lanes and interleave with zeros
            sum_grp2 = _mm_unpacklo_epi16(sq_hi, zero);
            sum_grp3 = _mm_unpackhi_epi16(sq_hi, zero);

            // Compute prefix scan of the 4 pixels in the low half of each 32-bit lane and accumulate
            sum_grp2 = _mm_add_epi32(sum_grp2, _mm_slli_si128(sum_grp2, 4));
            sum_grp2 = _mm_add_epi32(sum_grp2, _mm_slli_si128(sum_grp2, 8));
            sum_grp2 = _mm_add_epi32(sum_grp2, sum_prev);
            sum_tmp  = _mm_shuffle_epi32(sum_grp2, _MM_SHUFFLE(3, 3, 3, 3));

            // Compute prefix scan of the 4 pixels in the high half of each 32-bit lane and accumulate
            sum_grp3 = _mm_add_epi32(sum_grp3, _mm_slli_si128(sum_grp3, 4));
            sum_grp3 = _mm_add_epi32(sum_grp3, _mm_slli_si128(sum_grp3, 8));
            sum_grp3 = _mm_add_epi32(sum_grp3, sum_tmp);
            sum_prev = _mm_shuffle_epi32(sum_grp3, _MM_SHUFFLE(3, 3, 3, 3));

            // Store
            _mm_storeu_si128((__m128i*)(out),    sum_grp0);
            _mm_storeu_si128((__m128i*)(out+4),  sum_grp1);
            _mm_storeu_si128((__m128i*)(out+8),  sum_grp2);
            _mm_storeu_si128((__m128i*)(out+12), sum_grp3);

            // Increment
            out           += 16;
            pixel_src     += 16;
            pixel_compare += 16;
        }

        if (y > 0)
        {
            out = (uint32_t *)(integral) + y*integral_stride;

            // Add the values from the previous row to the current row
            for (int x = 0; x < dst_w + n; x += 16)
            {
                *((__m128i*)out)      = _mm_add_epi32(*(__m128i*)(out    - integral_stride),
                                                      *(__m128i*)(out));
                *((__m128i*)(out+4))  = _mm_add_epi32(*(__m128i*)(out+4  - integral_stride),
                                                      *(__m128i*)(out+4));
                *((__m128i*)(out+8))  = _mm_add_epi32(*(__m128i*)(out+8  - integral_stride),
                                                      *(__m128i*)(out+8));
                *((__m128i*)(out+12)) = _mm_add_epi32(*(__m128i*)(out+12 - integral_stride),
                                                      *(__m128i*)(out+12));
                out += 16;
            }
        }
    }
}

ATTR_TARGET_SSE4
static void build_integral_8_sse4(void *integral,
                                  int   integral_stride,
                            const void *in_src,
                            const void *in_src_pre,
                            const void *in_compare,
                            const void *in_compare_pre,
                                  int   w,
                                  int   border,
                                  int   dst_w,
                                  int   dst_h,
                                  int   dx,
                                  int   dy,
                                  int   n)
{
    const int bw = w + 2 * border;
    const int n_half = (n-1) /2;

    const uint8_t *src_pre     = (const uint8_t *)(in_src_pre);
    const uint8_t *compare_pre = (const uint8_t *)(in_compare_pre);

    for (int y = 0; y < dst_h + n; y++)
    {
        __m128i sum_prev = _mm_setzero_si128();

        const uint8_t *pixel_src     = src_pre     + (y-n_half   )*bw - n_half;
        const uint8_t *pixel_compare = compare_pre + (y-n_half+dy)*bw - n_half + dx;
        uint32_t *out = (uint32_t *)(integral) + (y*integral_stride);

        for (int x = 0; x < dst_w + n; x += 16)
        {
            __m128i p1, p1_lo, p1_hi;
            __m128i p2, p2_lo, p2_hi;
            __m128i df_lo, df_hi;
            __m128i sq_lo, sq_hi;
            __m128i sum_grp0, sum_grp1, sum_grp2, sum_grp3, sum_tmp;

            // Load multiple 8-bit source and compare pixels into separate 128-bit registers
            p1 = _mm_loadu_si128((__m128i*)(pixel_src));
            p2 = _mm_loadu_si128((__m128i*)(pixel_compare));

            // Unpack low half of pixels to 16-bit lanes and interleave with zeros
            p1_lo = _mm_cvtepu8_epi16(p1);
            p2_lo = _mm_cvtepu8_epi16(p2);

            // Square the difference between pixel values
            df_lo = _mm_sub_epi16(p1_lo, p2_lo);
            sq_lo = _mm_mullo_epi16(df_lo, df_lo);

            // Unpack low and high halves of squared diff to 32-bit lanes and interleave with zeros
            sum_grp0 = _mm_cvtepu16_epi32(sq_lo);
            sum_grp1 = _mm_cvtepu16_epi32(_mm_srli_si128(sq_lo, 8));

            // Compute prefix scan of the 4 pixels in the low half of each 32-bit lane and accumulate
            sum_grp0 = _mm_add_epi32(sum_grp0, _mm_slli_si128(sum_grp0, 4));
            sum_grp0 = _mm_add_epi32(sum_grp0, _mm_slli_si128(sum_grp0, 8));
            sum_grp0 = _mm_add_epi32(sum_grp0, sum_prev);
            sum_tmp  = _mm_shuffle_epi32(sum_grp0, _MM_SHUFFLE(3, 3, 3, 3));

            // Compute prefix scan of the 4 pixels in the high half of each 32-bit lane and accumulate
            sum_grp1 = _mm_add_epi32(sum_grp1, _mm_slli_si128(sum_grp1, 4));
            sum_grp1 = _mm_add_epi32(sum_grp1, _mm_slli_si128(sum_grp1, 8));
            sum_grp1 = _mm_add_epi32(sum_grp1, sum_tmp);
            sum_prev = _mm_shuffle_epi32(sum_grp1, _MM_SHUFFLE(3, 3, 3, 3));

            // Unpack high half of pixels to 16-bit lanes and interleave with zeros
            p1_hi = _mm_cvtepu8_epi16(_mm_srli_si128(p1, 8));
            p2_hi = _mm_cvtepu8_epi16(_mm_srli_si128(p2, 8));

            // Square the difference between pixel values
            df_hi = _mm_sub_epi16(p1_hi, p2_hi);
            sq_hi = _mm_mullo_epi16(df_hi, df_hi);

            // Unpack low and high halves of squared diff to 32-bit lanes and interleave with zeros
            sum_grp2 = _mm_cvtepu16_epi32(sq_hi);
            sum_grp3 = _mm_cvtepu16_epi32(_mm_srli_si128(sq_hi, 8));

            // Compute prefix scan of the 4 pixels in the low half of each 32-bit lane and accumulate
            sum_grp2 = _mm_add_epi32(sum_grp2, _mm_slli_si128(sum_grp2, 4));
            sum_grp2 = _mm_add_epi32(sum_grp2, _mm_slli_si128(sum_grp2, 8));
            sum_grp2 = _mm_add_epi32(sum_grp2, sum_prev);
            sum_tmp  = _mm_shuffle_epi32(sum_grp2, _MM_SHUFFLE(3, 3, 3, 3));

            // Compute prefix scan of the 4 pixels in the high half of each 32-bit lane and accumulate
            sum_grp3 = _mm_add_epi32(sum_grp3, _mm_slli_si128(sum_grp3, 4));
            sum_grp3 = _mm_add_epi32(sum_grp3, _mm_slli_si128(sum_grp3, 8));
            sum_grp3 = _mm_add_epi32(sum_grp3, sum_tmp);
            sum_prev = _mm_shuffle_epi32(sum_grp3, _MM_SHUFFLE(3, 3, 3, 3));

            // Store
            _mm_storeu_si128((__m128i*)(out),    sum_grp0);
            _mm_storeu_si128((__m128i*)(out+4),  sum_grp1);
            _mm_storeu_si128((__m128i*)(out+8),  sum_grp2);
            _mm_storeu_si128((__m128i*)(out+12), sum_grp3);

            // Increment
            out           += 16;
            pixel_src     += 16;
            pixel_compare += 16;
        }

        if (y > 0)
        {
            out = (uint32_t *)(integral) + y*integral_stride;

            // Add the values from the previous row to the current row
            for (int x = 0; x < dst_w + n; x += 16)
            {
                *((__m128i*)out)      = _mm_add_epi32(*(__m128i*)(out    - integral_stride),
                                                      *(__m128i*)(out));
                *((__m128i*)(out+4))  = _mm_add_epi32(*(__m128i*)(out+4  - integral_stride),
                                                      *(__m128i*)(out+4));
                *((__m128i*)(out+8))  = _mm_add_epi32(*(__m128i*)(out+8  - integral_stride),
                                                      *(__m128i*)(out+8));
                *((__m128i*)(out+12)) = _mm_add_epi32(*(__m128i*)(out+12 - integral_stride),
                                                      *(__m128i*)(out+12));
                out += 16;
            }
        }
    }
}

ATTR_TARGET_SSE4
static void build_integral_16_sse4(void *integral,
                                   int   integral_stride,
                             const void *in_src,
                             const void *in_src_pre,
                             const void *in_compare,
                             const void *in_compare_pre,
                                   int   w,
                                   int   border,
                                   int   dst_w,
                                   int   dst_h,
                                   int   dx,
                                   int   dy,
                                   int   n)
{
    const int bw = w + 2 * border;
    const int n_half = (n-1) /2;

    const uint16_t *src_pre     = (const uint16_t *)(in_src_pre);
    const uint16_t *compare_pre = (const uint16_t *)(in_compare_pre);

    for (int y = 0; y < dst_h + n; y++)
    {
        __m128i sum_prev = _mm_setzero_si128();

        const uint16_t *pixel_src     = src_pre     + (y-n_half   )*bw - n_half;
        const uint16_t *pixel_compare = compare_pre + (y-n_half+dy)*bw - n_half + dx;
        uint64_t *out = (uint64_t *)(integral) + (y*integral_stride);

        for (int x = 0; x < dst_w + n; x += 8)
        {
            __m128i p1, p1_lo, p1_hi;
            __m128i p2, p2_lo, p2_hi;
            __m128i df_lo, df_hi;
            __m128i sq_lo, sq_hi;
            __m128i sum_grp0, sum_grp1, sum_grp2, sum_grp3, sum_tmp;

            // Load multiple 16-bit source and compare pixels into separate 128-bit registers
            p1 = _mm_loadu_si128((__m128i*)(pixel_src));
            p2 = _mm_loadu_si128((__m128i*)(pixel_compare));

            // Unpack low half of pixels to 32-bit lanes and interleave with zeros
            p1_lo = _mm_cvtepu16_epi32(p1);
            p2_lo = _mm_cvtepu16_epi32(p2);

            // Square the difference between pixel values
            df_lo = _mm_sub_epi32(p1_lo, p2_lo);
            sq_lo = _mm_mullo_epi32(df_lo, df_lo);

            // Unpack low and high halves of squared diff to 64-bit lanes and interleave with zeros
            sum_grp0 = _mm_cvtepu32_epi64(sq_lo);
            sum_grp1 = _mm_cvtepu32_epi64(_mm_srli_si128(sq_lo, 8));

            // Compute prefix scan of the 2 pixels in the low half of each 64-bit lane and accumulate
            sum_grp0 = _mm_add_epi64(sum_grp0, _mm_slli_si128(sum_grp0, 8));
            sum_grp0 = _mm_add_epi64(sum_grp0, sum_prev);
            sum_tmp  = _mm_shuffle_epi32(sum_grp0, _MM_SHUFFLE(3, 2, 3, 2));

            // Compute prefix scan of the 2 pixels in the high half of each 64-bit lane and accumulate
            sum_grp1 = _mm_add_epi64(sum_grp1, _mm_slli_si128(sum_grp1, 8));
            sum_grp1 = _mm_add_epi64(sum_grp1, sum_tmp);
            sum_prev = _mm_shuffle_epi32(sum_grp1, _MM_SHUFFLE(3, 2, 3, 2));

            // Unpack high half of pixels to 32-bit lanes and interleave with zeros
            p1_hi = _mm_cvtepu16_epi32(_mm_srli_si128(p1, 8));
            p2_hi = _mm_cvtepu16_epi32(_mm_srli_si128(p2, 8));

            // Square the difference between pixel values
            df_hi = _mm_sub_epi32(p1_hi, p2_hi);
            sq_hi = _mm_mullo_epi32(df_hi, df_hi);

            // Unpack low and high halves of squared diff to 64-bit lanes and interleave with zeros
            sum_grp2 = _mm_cvtepu32_epi64(sq_hi);
            sum_grp3 = _mm_cvtepu32_epi64(_mm_srli_si128(sq_hi, 8));

            // Compute prefix scan of the 2 pixels in the low half of each 64-bit lane and accumulate
            sum_grp2 = _mm_add_epi64(sum_grp2, _mm_slli_si128(sum_grp2, 8));
            sum_grp2 = _mm_add_epi64(sum_grp2, sum_prev);
            sum_tmp  = _mm_shuffle_epi32(sum_grp2, _MM_SHUFFLE(3, 2, 3, 2));

            // Compute prefix scan of the 2 pixels in the high half of each 64-bit lane and accumulate
            sum_grp3 = _mm_add_epi64(sum_grp3, _mm_slli_si128(sum_grp3, 8));
            sum_grp3 = _mm_add_epi64(sum_grp3, sum_tmp);
            sum_prev = _mm_shuffle_epi32(sum_grp3, _MM_SHUFFLE(3, 2, 3, 2));

            // Store
            _mm_storeu_si128((__m128i*)(out),   sum_grp0);
            _mm_storeu_si128((__m128i*)(out+2), sum_grp1);
            _mm_storeu_si128((__m128i*)(out+4), sum_grp2);
            _mm_storeu_si128((__m128i*)(out+6), sum_grp3);

            // Increment
            out           += 8;
            pixel_src     += 8;
            pixel_compare += 8;
        }

        if (y > 0)
        {
            out = (uint64_t *)(integral) + y*integral_stride;

            // Add the values from the previous row to the current row
            for (int x = 0; x < dst_w + n; x += 8)
            {
                *((__m128i*)out)     = _mm_add_epi64(*(__m128i*)(out   - integral_stride),
                                                     *(__m128i*)(out));
                *((__m128i*)(out+2)) = _mm_add_epi64(*(__m128i*)(out+2 - integral_stride),
                                                     *(__m128i*)(out+2));
                *((__m128i*)(out+4)) = _mm_add_epi64(*(__m128i*)(out+4 - integral_stride),
                                                     *(__m128i*)(out+4));
                *((__m128i*)(out+6)) = _mm_add_epi64(*(__m128i*)(out+6 - integral_stride),
                                                     *(__m128i*)(out+6));
                out += 8;
            }
        }
    }
}

void nlmeans_init_x86(NLMeansFunctions *functions,
                             const int  depth)
{
    switch (depth)
    {
        case 8:
            if (av_get_cpu_flags() & AV_CPU_FLAG_SSE4)
            {
                functions->build_integral = build_integral_8_sse4;
                hb_log("NLMeans using SSE4.1 optimizations (depth %d)", depth);
            }
            else if (av_get_cpu_flags() & AV_CPU_FLAG_SSE2)
            {
                functions->build_integral = build_integral_8_sse2;
                hb_log("NLMeans using SSE2 optimizations (depth %d)", depth);
            }
            break;
        case 10:
        case 12:
        case 16:
            if (av_get_cpu_flags() & AV_CPU_FLAG_SSE4)
            {
                functions->build_integral = build_integral_16_sse4;
                hb_log("NLMeans using SSE4.1 optimizations (depth %d)", depth);
            }
            break;
        default:
            break;
    }
}

#endif // ARCH_X86
