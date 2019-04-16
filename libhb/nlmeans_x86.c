/* nlmeans_x86.c

   Copyright (c) 2013 Dirk Farin
   Copyright (c) 2003-2019 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"     // needed for ARCH_X86

#if defined(ARCH_X86)

#include <immintrin.h>

#include "libavutil/cpu.h"
#include "nlmeans.h"

static void build_integral_avx2(uint32_t *integral,
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
    const __m256i zero = _mm256_set1_epi8(0);
    const int bw = w + 2 * border;
    const int bs = NLMEANS_ALIGN(bw);

    for (int y = 0; y < dst_h; y++)
    {
        __m256i prevadd = _mm256_set1_epi32(0);

        const uint8_t *p1 = src_pre + y*bs;
        const uint8_t *p2 = compare_pre + (y+dy)*bs + dx;
        uint32_t *out = integral + (y*integral_stride);

        for (int x = 0; x < dst_w; x += 32)
        {
            __m256i pa, pb;
            __m256i pla, plb;
            __m256i ldiff, lldiff, lhdiff;
            __m256i ltmp,htmp;
            __m256i ladd,hadd;
            __m256i pha,phb;
            __m256i hdiff,hldiff,hhdiff;
            __m256i l2tmp,h2tmp;

            pa = _mm256_loadu_si256((__m256i*)p1);      // Load source  pixels into register 1
            pb = _mm256_loadu_si256((__m256i*)p2);      // Load compare pixels into register 2

            // Low
            pla = _mm256_unpacklo_epi8(pa,zero);        // Unpack and interleave source  low with zeros
            plb = _mm256_unpacklo_epi8(pb,zero);        // Unpack and interleave compare low with zeros

            ldiff = _mm256_sub_epi16(pla,plb);          // Diff source and compare lows (subtract)
            ldiff = _mm256_mullo_epi16(ldiff,ldiff);    // Square low diff (multiply at 32-bit precision)

            lldiff = _mm256_unpacklo_epi16(ldiff,zero); // Unpack and interleave diff low  with zeros
            lhdiff = _mm256_unpackhi_epi16(ldiff,zero); // Unpack and interleave diff high with zeros

            ltmp = _mm256_slli_si256(lldiff, 4);        // Temp shift diff low left 4 bytes
            lldiff = _mm256_add_epi32(lldiff, ltmp);    // Add above to diff low
            ltmp = _mm256_slli_si256(lldiff, 8);        // Temp shift diff low left 8 bytes
            lldiff = _mm256_add_epi32(lldiff, ltmp);    // Add above to diff low
            lldiff = _mm256_add_epi32(lldiff, prevadd); // Add previous total to diff low

            ladd = _mm256_shuffle_epi32(lldiff, 0xff);  // Shuffle diff low

            htmp = _mm256_slli_si256(lhdiff, 4);        // Temp shift diff high left 4 bytes
            lhdiff = _mm256_add_epi32(lhdiff, htmp);    // Add above to diff high
            htmp = _mm256_slli_si256(lhdiff, 8);        // Temp shift diff high left 8 bytes
            lhdiff = _mm256_add_epi32(lhdiff, htmp);    // Add above to diff high
            lhdiff = _mm256_add_epi32(lhdiff, ladd);    // Add shuffled diff low to diff high

            prevadd = _mm256_shuffle_epi32(lhdiff, 0xff); // Shuffle diff high

            // High
            pha = _mm256_unpackhi_epi8(pa,zero);        // Unpack and interleave source  high with zeros
            phb = _mm256_unpackhi_epi8(pb,zero);        // Unpack and interleave compare high with zeros

            hdiff = _mm256_sub_epi16(pha,phb);          // Diff source and compare highs (subtract)
            hdiff = _mm256_mullo_epi16(hdiff,hdiff);    // Square high diff (multiply at 32-bit precision)

            hldiff = _mm256_unpacklo_epi16(hdiff,zero); // Unpack and interleave diff low  with zeros
            hhdiff = _mm256_unpackhi_epi16(hdiff,zero); // Unpack and interleave diff high with zeros

            l2tmp = _mm256_slli_si256(hldiff, 4);       // Temp shift diff low 4 bytes
            hldiff = _mm256_add_epi32(hldiff, l2tmp);   // Add above to diff low
            l2tmp = _mm256_slli_si256(hldiff, 8);       // Temp shift diff low left 8 bytes
            hldiff = _mm256_add_epi32(hldiff, l2tmp);   // Add above to diff low
            hldiff = _mm256_add_epi32(hldiff, prevadd); // Add previous total to diff low

            hadd = _mm256_shuffle_epi32(hldiff, 0xff);  // Shuffle diff low

            h2tmp = _mm256_slli_si256(hhdiff, 4);       // Temp shift diff high left 4 bytes
            hhdiff = _mm256_add_epi32(hhdiff, h2tmp);   // Add above to diff high
            h2tmp = _mm256_slli_si256(hhdiff, 8);       // Temp shift diff high left 8 bytes
            hhdiff = _mm256_add_epi32(hhdiff, h2tmp);   // Add above to diff high
            hhdiff = _mm256_add_epi32(hhdiff, hadd);    // Add shuffled diff low to diff high

            prevadd = _mm256_shuffle_epi32(hhdiff, 0xff); // Shuffle diff high

            // Store, lane:0
            _mm_store_si128((__m128i*)(out),    _mm256_extractf128_si256(lldiff,0)); // Store low  diff low  in memory
            _mm_store_si128((__m128i*)(out+4),  _mm256_extractf128_si256(lhdiff,0)); // Store low  diff high in memory
            _mm_store_si128((__m128i*)(out+8),  _mm256_extractf128_si256(hldiff,0)); // Store high diff low  in memory
            _mm_store_si128((__m128i*)(out+12), _mm256_extractf128_si256(hhdiff,0)); // Store high diff high in memory

            // Cross lanes for (out-1)
            lldiff = _mm256_add_epi32(lldiff,_mm256_permutevar8x32_epi32(hhdiff, _mm256_set_epi32(3,3,3,3,3,3,3,3)));
            lhdiff = _mm256_add_epi32(lhdiff,_mm256_permutevar8x32_epi32(hhdiff, _mm256_set_epi32(3,3,3,3,3,3,3,3)));
            hldiff = _mm256_add_epi32(hldiff,_mm256_permutevar8x32_epi32(hhdiff, _mm256_set_epi32(3,3,3,3,3,3,3,3)));
            hhdiff = _mm256_add_epi32(hhdiff,_mm256_permutevar8x32_epi32(hhdiff, _mm256_set_epi32(3,3,3,3,3,3,3,3)));

            // Store, lane:1
            _mm_store_si128((__m128i*)(out+16), _mm256_extractf128_si256(lldiff,1)); // Store low  diff low  in memory
            _mm_store_si128((__m128i*)(out+20), _mm256_extractf128_si256(lhdiff,1)); // Store low  diff high in memory
            _mm_store_si128((__m128i*)(out+24), _mm256_extractf128_si256(hldiff,1)); // Store high diff low  in memory
            _mm_store_si128((__m128i*)(out+28), _mm256_extractf128_si256(hhdiff,1)); // Store high diff high in memory

            // Adjust prevadd for next iteration
            prevadd = _mm256_permutevar8x32_epi32(hhdiff, _mm256_set_epi32(7,7,7,7,7,7,7,7));
            prevadd = _mm256_blend_epi32(prevadd, zero, 0xf0);

            // Increment
            out += 32;
            p1  += 32;
            p2  += 32;
        }

        if (y > 0)
        {
            out = integral + y*integral_stride;

            for (int x = 0; x < dst_w; x += 32)
            {
                *((__m256i*)out)      = _mm256_add_epi32(*(__m256i*)(out-integral_stride),    *(__m256i*)(out));
                *((__m256i*)(out+8))  = _mm256_add_epi32(*(__m256i*)(out+8-integral_stride),  *(__m256i*)(out+8));
                *((__m256i*)(out+16)) = _mm256_add_epi32(*(__m256i*)(out+16-integral_stride), *(__m256i*)(out+16));
                *((__m256i*)(out+24)) = _mm256_add_epi32(*(__m256i*)(out+24-integral_stride), *(__m256i*)(out+24));

                out += 32;
            }
        }
    }
}

static void build_integral_sse2(uint32_t *integral,
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
    const __m128i zero = _mm_set1_epi8(0);
    const int bw = w + 2 * border;
    const int bs = NLMEANS_ALIGN(bw);

    for (int y = 0; y < dst_h; y++)
    {
        __m128i prevadd = _mm_set1_epi32(0);

        const uint8_t *p1 = src_pre + y*bs;
        const uint8_t *p2 = compare_pre + (y+dy)*bs + dx;
        uint32_t *out = integral + (y*integral_stride);

        for (int x = 0; x < dst_w; x += 16)
        {
            __m128i pa, pb;
            __m128i pla, plb;
            __m128i ldiff, lldiff, lhdiff;
            __m128i ltmp,htmp;
            __m128i ladd,hadd;
            __m128i pha,phb;
            __m128i hdiff,hldiff,hhdiff;
            __m128i l2tmp,h2tmp;

            pa = _mm_loadu_si128((__m128i*)p1);      // Load source  pixels into register 1
            pb = _mm_loadu_si128((__m128i*)p2);      // Load compare pixels into register 2

            // Low
            pla = _mm_unpacklo_epi8(pa,zero);        // Unpack and interleave source  low with zeros
            plb = _mm_unpacklo_epi8(pb,zero);        // Unpack and interleave compare low with zeros

            ldiff = _mm_sub_epi16(pla,plb);          // Diff source and compare lows (subtract)
            ldiff = _mm_mullo_epi16(ldiff,ldiff);    // Square low diff (multiply at 32-bit precision)

            lldiff = _mm_unpacklo_epi16(ldiff,zero); // Unpack and interleave diff low  with zeros
            lhdiff = _mm_unpackhi_epi16(ldiff,zero); // Unpack and interleave diff high with zeros

            ltmp = _mm_slli_si128(lldiff, 4);        // Temp shift diff low left 4 bytes
            lldiff = _mm_add_epi32(lldiff, ltmp);    // Add above to diff low
            ltmp = _mm_slli_si128(lldiff, 8);        // Temp shift diff low left 8 bytes
            lldiff = _mm_add_epi32(lldiff, ltmp);    // Add above to diff low
            lldiff = _mm_add_epi32(lldiff, prevadd); // Add previous total to diff low

            ladd = _mm_shuffle_epi32(lldiff, 0xff);  // Shuffle diff low

            htmp = _mm_slli_si128(lhdiff, 4);        // Temp shift diff high left 4 bytes
            lhdiff = _mm_add_epi32(lhdiff, htmp);    // Add above to diff high
            htmp = _mm_slli_si128(lhdiff, 8);        // Temp shift diff high left 8 bytes
            lhdiff = _mm_add_epi32(lhdiff, htmp);    // Add above to diff high
            lhdiff = _mm_add_epi32(lhdiff, ladd);    // Add shuffled diff low to diff high

            prevadd = _mm_shuffle_epi32(lhdiff, 0xff); // Shuffle diff high

            // High
            pha = _mm_unpackhi_epi8(pa,zero);        // Unpack and interleave source  high with zeros
            phb = _mm_unpackhi_epi8(pb,zero);        // Unpack and interleave compare high with zeros

            hdiff = _mm_sub_epi16(pha,phb);          // Diff source and compare highs (subtract)
            hdiff = _mm_mullo_epi16(hdiff,hdiff);    // Square high diff (multiply at 32-bit precision)

            hldiff = _mm_unpacklo_epi16(hdiff,zero); // Unpack and interleave diff low  with zeros
            hhdiff = _mm_unpackhi_epi16(hdiff,zero); // Unpack and interleave diff high with zeros

            l2tmp = _mm_slli_si128(hldiff, 4);       // Temp shift diff low 4 bytes
            hldiff = _mm_add_epi32(hldiff, l2tmp);   // Add above to diff low
            l2tmp = _mm_slli_si128(hldiff, 8);       // Temp shift diff low left 8 bytes
            hldiff = _mm_add_epi32(hldiff, l2tmp);   // Add above to diff low
            hldiff = _mm_add_epi32(hldiff, prevadd); // Add previous total to diff low

            hadd = _mm_shuffle_epi32(hldiff, 0xff);  // Shuffle diff low

            h2tmp = _mm_slli_si128(hhdiff, 4);       // Temp shift diff high left 4 bytes
            hhdiff = _mm_add_epi32(hhdiff, h2tmp);   // Add above to diff high
            h2tmp = _mm_slli_si128(hhdiff, 8);       // Temp shift diff high left 8 bytes
            hhdiff = _mm_add_epi32(hhdiff, h2tmp);   // Add above to diff high
            hhdiff = _mm_add_epi32(hhdiff, hadd);    // Add shuffled diff low to diff high

            prevadd = _mm_shuffle_epi32(hhdiff, 0xff); // Shuffle diff high

            // Store
            _mm_store_si128((__m128i*)(out),    lldiff); // Store low  diff low  in memory
            _mm_store_si128((__m128i*)(out+4),  lhdiff); // Store low  diff high in memory
            _mm_store_si128((__m128i*)(out+8),  hldiff); // Store high diff low  in memory
            _mm_store_si128((__m128i*)(out+12), hhdiff); // Store high diff high in memory

            // Increment
            out += 16;
            p1  += 16;
            p2  += 16;
        }

        if (y > 0)
        {
            out = integral + y*integral_stride;

            for (int x = 0; x < dst_w; x += 16)
            {
                *((__m128i*)out) = _mm_add_epi32(*(__m128i*)(out-integral_stride),
                                                 *(__m128i*)(out));

                *((__m128i*)(out+4)) = _mm_add_epi32(*(__m128i*)(out+4-integral_stride),
                                                     *(__m128i*)(out+4));

                *((__m128i*)(out+8)) = _mm_add_epi32(*(__m128i*)(out+8-integral_stride),
                                                     *(__m128i*)(out+8));

                *((__m128i*)(out+12)) = _mm_add_epi32(*(__m128i*)(out+12-integral_stride),
                                                      *(__m128i*)(out+12));

                out += 16;
            }
        }
    }
}

void nlmeans_init_x86(NLMeansFunctions *functions)
{
    if (av_get_cpu_flags() & AV_CPU_FLAG_AVX2)
    {
        functions->build_integral = build_integral_avx2;
        hb_log("NLMeans using AVX2 optimizations");
    }
    else if (av_get_cpu_flags() & AV_CPU_FLAG_SSE2)
    {
        functions->build_integral = build_integral_sse2;
        hb_log("NLMeans using SSE2 optimizations");
    }
}

#endif // ARCH_X86
