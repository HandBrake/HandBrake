#ifdef USE_OPENCL
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hb.h"
#include "scale.h"
#include "scale_kernel.h"
#include "libavutil/pixdesc.h"

#define isScaleRGBinInt(x) \
    (           \
        (x)==AV_PIX_FMT_RGB48BE   ||   \
        (x)==AV_PIX_FMT_RGB48LE   ||   \
        (x)==AV_PIX_FMT_RGB32     ||   \
        (x)==AV_PIX_FMT_RGB32_1   ||   \
        (x)==AV_PIX_FMT_RGB24     ||   \
        (x)==AV_PIX_FMT_RGB565BE  ||   \
        (x)==AV_PIX_FMT_RGB565LE  ||   \
        (x)==AV_PIX_FMT_RGB555BE  ||   \
        (x)==AV_PIX_FMT_RGB555LE  ||   \
        (x)==AV_PIX_FMT_RGB444BE  ||   \
        (x)==AV_PIX_FMT_RGB444LE  ||   \
        (x)==AV_PIX_FMT_RGB8      ||   \
        (x)==AV_PIX_FMT_RGB4      ||   \
        (x)==AV_PIX_FMT_RGB4_BYTE ||   \
        (x)==AV_PIX_FMT_MONOBLACK ||   \
        (x)==AV_PIX_FMT_MONOWHITE   \
    )
#define isScaleBGRinInt(x) \
    (           \
         (x)==AV_PIX_FMT_BGR48BE  ||   \
         (x)==AV_PIX_FMT_BGR48LE  ||   \
         (x)==AV_PIX_FMT_BGR32    ||   \
         (x)==AV_PIX_FMT_BGR32_1  ||   \
         (x)==AV_PIX_FMT_BGR24    ||   \
         (x)==AV_PIX_FMT_BGR565BE ||   \
         (x)==AV_PIX_FMT_BGR565LE ||   \
         (x)==AV_PIX_FMT_BGR555BE ||   \
         (x)==AV_PIX_FMT_BGR555LE ||   \
         (x)==AV_PIX_FMT_BGR444BE ||   \
         (x)==AV_PIX_FMT_BGR444LE ||   \
         (x)==AV_PIX_FMT_BGR8     ||   \
         (x)==AV_PIX_FMT_BGR4     ||   \
         (x)==AV_PIX_FMT_BGR4_BYTE||   \
         (x)==AV_PIX_FMT_MONOBLACK||   \
         (x)==AV_PIX_FMT_MONOWHITE   \
    )

#define isScaleAnyRGB(x) \
    (           \
          isScaleRGBinInt(x)       ||    \
          isScaleBGRinInt(x)             \
    )

#define isScaleGray(x)                      \
    ((x) == AV_PIX_FMT_GRAY8       ||     \
     (x) == AV_PIX_FMT_Y400A       ||     \
     (x) == AV_PIX_FMT_GRAY16BE    ||     \
     (x) == AV_PIX_FMT_GRAY16LE)

static ScaleContext *g_scale;

static double getScaleSplineCoeff(double a, double b, double c, double d,
                             double dist)
{
    if (dist <= 1.0)
        return ((d * dist + c) * dist + b) * dist + a;
    else
        return getScaleSplineCoeff(0.0,
                               b + 2.0 * c + 3.0 * d,
                               c + 3.0 * d,
                              -b - 3.0 * c - 6.0 * d,
                              dist - 1.0);
}

static int initScaleFilter(int16_t **outFilter, int32_t **filterPos,
                      int *outFilterSize, int xInc, int srcW, int dstW,
                      int filterAlign, int one, int flags, int cpu_flags,
                      ScaleVector *srcFilter, ScaleVector *dstFilter,
                      double param[2])
{
    int i;
    int filterSize;
    int filter2Size;
    int minFilterSize;
    int64_t *filter    = NULL;
    int64_t *filter2   = NULL;
    const int64_t fone = 1LL << 54;
    int ret            = -1;

    *filterPos = (int32_t *)av_malloc((dstW + 3) * sizeof(**filterPos));
    if (*filterPos == NULL && ((dstW + 3) * sizeof(**filterPos)) != 0) {
        hb_log("Cannot allocate memory."); 
        goto fail;
    }

    if (FFABS(xInc - 0x10000) < 10) { // unscaled
        int i;
        filterSize = 1;
        filter = (int64_t *)av_mallocz(dstW * sizeof(*filter) * filterSize);
        if (filter == NULL && (dstW * sizeof(*filter) * filterSize) != 0) {
        hb_log("Cannot allocate memory."); 
        goto fail;
        }


        for (i = 0; i < dstW; i++) {
            filter[i * filterSize] = fone;
            (*filterPos)[i]        = i;
        }
    } else if (flags & SWS_POINT) { // lame looking point sampling mode
        int i;
        int64_t xDstInSrc;
        filterSize = 1;
      filter = (int64_t *)av_malloc(dstW * sizeof(*filter) * filterSize);
      if(filter == NULL && (dstW * sizeof(*filter) * filterSize) != 0){
          hb_log("Cannot allocate memory."); 
        goto fail;
      }

        xDstInSrc = xInc / 2 - 0x8000;
        for (i = 0; i < dstW; i++) {
            int xx = (xDstInSrc - ((filterSize - 1) << 15) + (1 << 15)) >> 16;

            (*filterPos)[i] = xx;
            filter[i]       = fone;
            xDstInSrc      += xInc;
        }
    } else if ((xInc <= (1 << 16) && (flags & SWS_AREA)) ||
               (flags & SWS_FAST_BILINEAR)) { // bilinear upscale
        int i;
        int64_t xDstInSrc;
        filterSize = 2;
       filter = (int64_t *)av_malloc(dstW * sizeof(*filter) * filterSize);
      if(filter == NULL && (dstW * sizeof(*filter) * filterSize) != 0){
          hb_log("Cannot allocate memory."); 
        goto fail;
      }

        xDstInSrc = xInc / 2 - 0x8000;
        for (i = 0; i < dstW; i++) {
            int xx = (xDstInSrc - ((filterSize - 1) << 15) + (1 << 15)) >> 16;
            int j;

            (*filterPos)[i] = xx;
            // bilinear upscale / linear interpolate / area averaging
            for (j = 0; j < filterSize; j++) {
                int64_t coeff= fone - FFABS(((int64_t)xx<<16) - xDstInSrc)*(fone>>16);
                if (coeff < 0)
                    coeff = 0;
                filter[i * filterSize + j] = coeff;
                xx++;
            }
            xDstInSrc += xInc;
        }
    } else {
        int64_t xDstInSrc;
        int sizeFactor;

        if (flags & SWS_BICUBIC)
            sizeFactor = 4;
        else if (flags & SWS_X)
            sizeFactor = 8;
        else if (flags & SWS_AREA)
            sizeFactor = 1;     // downscale only, for upscale it is bilinear
        else if (flags & SWS_GAUSS)
            sizeFactor = 8;     // infinite ;)
        else if (flags & SWS_LANCZOS)
            sizeFactor = param[0] != SWS_PARAM_DEFAULT ? ceil(2 * param[0]) : 6;
        else if (flags & SWS_SINC)
            sizeFactor = 20;    // infinite ;)
        else if (flags & SWS_SPLINE)
            sizeFactor = 20;    // infinite ;)
        else if (flags & SWS_BILINEAR)
            sizeFactor = 2;
        else {
            sizeFactor = 0;     // GCC warning killer
            assert(0);
        }

        if (xInc <= 1 << 16)
            filterSize = 1 + sizeFactor;    // upscale
        else
            filterSize = 1 + (sizeFactor * srcW + dstW - 1) / dstW;


        filterSize = FFMIN(filterSize, srcW - 2);
        filterSize = FFMAX(filterSize, 1);

        filter = (int64_t *)av_malloc(dstW * sizeof(*filter) * filterSize);
        if(filter == NULL && (dstW * sizeof(*filter) * filterSize) != 0){
            hb_log("Cannot allocate memory."); 
            goto fail;
        }

        xDstInSrc = xInc - 0x10000;
        for (i = 0; i < dstW; i++) {
            int xx = (xDstInSrc - ((filterSize - 2) << 16)) / (1 << 17);
            int j;
            (*filterPos)[i] = xx;
            for (j = 0; j < filterSize; j++) {
                int64_t d = (FFABS(((int64_t)xx << 17) - xDstInSrc)) << 13;
                double floatd;
                int64_t coeff;

                if (xInc > 1 << 16)
                    d = d * dstW / srcW;
                floatd = d * (1.0 / (1 << 30));

                if (flags & SWS_BICUBIC) {
                    int64_t B = (param[0] != SWS_PARAM_DEFAULT ? param[0] :   0) * (1 << 24);
                    int64_t C = (param[1] != SWS_PARAM_DEFAULT ? param[1] : 0.6) * (1 << 24);

                    if (d >= 1LL << 31) {
                        coeff = 0.0;
                    } else {
                        int64_t dd  = (d  * d) >> 30;
                        int64_t ddd = (dd * d) >> 30;

                        if (d < 1LL << 30)
                            coeff =  (12 * (1 << 24) -  9 * B - 6 * C) * ddd +
                                    (-18 * (1 << 24) + 12 * B + 6 * C) *  dd +
                                      (6 * (1 << 24) -  2 * B)         * (1 << 30);
                        else
                            coeff =      (-B -  6 * C) * ddd +
                                      (6 * B + 30 * C) * dd  +
                                    (-12 * B - 48 * C) * d   +
                                      (8 * B + 24 * C) * (1 << 30);
                    }
                    coeff *= fone >> (30 + 24);
                }
#if 0
                else if (flags & SWS_X) {
                    double p  = param ? param * 0.01 : 0.3;
                    coeff     = d ? sin(d * M_PI) / (d * M_PI) : 1.0;
                    coeff    *= pow(2.0, -p * d * d);
                }
#endif
                else if (flags & SWS_X) {
                    double A = param[0] != SWS_PARAM_DEFAULT ? param[0] : 1.0;
                    double c;

                    if (floatd < 1.0)
                        c = cos(floatd * M_PI);
                    else
                        c = -1.0;
                    if (c < 0.0)
                        c = -pow(-c, A);
                    else
                        c = pow(c, A);
                    coeff = (c * 0.5 + 0.5) * fone;
                } else if (flags & SWS_AREA) {
                    int64_t d2 = d - (1 << 29);
                    if (d2 * xInc < -(1LL << (29 + 16)))
                        coeff = 1.0 * (1LL << (30 + 16));
                    else if (d2 * xInc < (1LL << (29 + 16)))
                        coeff = -d2 * xInc + (1LL << (29 + 16));
                    else
                        coeff = 0.0;
                    coeff *= fone >> (30 + 16);
                } else if (flags & SWS_GAUSS) {
                    double p = param[0] != SWS_PARAM_DEFAULT ? param[0] : 3.0;
                    coeff = (pow(2.0, -p * floatd * floatd)) * fone;
                } else if (flags & SWS_SINC) {
                    coeff = (d ? sin(floatd * M_PI) / (floatd * M_PI) : 1.0) * fone;
                } else if (flags & SWS_LANCZOS) {
                    double p = param[0] != SWS_PARAM_DEFAULT ? param[0] : 3.0;
                    coeff = (d ? sin(floatd * M_PI) * sin(floatd * M_PI / p) /
                             (floatd * floatd * M_PI * M_PI / p) : 1.0) * fone;
                    if (floatd > p)
                        coeff = 0;
                } else if (flags & SWS_BILINEAR) {
                    coeff = (1 << 30) - d;
                    if (coeff < 0)
                        coeff = 0;
                    coeff *= fone >> 30;
                } else if (flags & SWS_SPLINE) {
                    double p = -2.196152422706632;
                    coeff = getScaleSplineCoeff(1.0, 0.0, p, -p - 1.0, floatd) * fone;
                } else {
                    coeff = 0.0; // GCC warning killer
                    assert(0);
                }

                filter[i * filterSize + j] = coeff;
                xx++;
            }
            xDstInSrc += 2 * xInc;
        }
    }

    assert(filterSize > 0);
    filter2Size = filterSize;
    if (srcFilter)
        filter2Size += srcFilter->length - 1;
    if (dstFilter)
        filter2Size += dstFilter->length - 1;
    assert(filter2Size > 0);
    filter2 = (int64_t *)av_mallocz(filter2Size * dstW * sizeof(*filter2));
    if(filter2 == NULL && (filter2Size * dstW * sizeof(*filter2)) != 0)
    {
        hb_log("Can't alloc memory.");
        goto fail;
    }

    for (i = 0; i < dstW; i++) {
        int j, k;

        if (srcFilter) {
            for (k = 0; k < srcFilter->length; k++) {
                for (j = 0; j < filterSize; j++)
                    filter2[i * filter2Size + k + j] +=
                        srcFilter->coeff[k] * filter[i * filterSize + j];
            }
        } else {
            for (j = 0; j < filterSize; j++)
                filter2[i * filter2Size + j] = filter[i * filterSize + j];
        }
        // FIXME dstFilter

        (*filterPos)[i] += (filterSize - 1) / 2 - (filter2Size - 1) / 2;
    }
    av_freep(&filter);

    // Assume it is near normalized (*0.5 or *2.0 is OK but * 0.001 is not).
    minFilterSize = 0;
    for (i = dstW - 1; i >= 0; i--) {
        int min = filter2Size;
        int j;
        int64_t cutOff = 0.0;

        for (j = 0; j < filter2Size; j++) {
            int k;
            cutOff += FFABS(filter2[i * filter2Size]);

            if (cutOff > SWS_MAX_REDUCE_CUTOFF * fone)
                break;

            if (i < dstW - 1 && (*filterPos)[i] >= (*filterPos)[i + 1])
                break;

            // move filter coefficients left
            for (k = 1; k < filter2Size; k++)
                filter2[i * filter2Size + k - 1] = filter2[i * filter2Size + k];
            filter2[i * filter2Size + k - 1] = 0;
            (*filterPos)[i]++;
        }

        cutOff = 0;
        for (j = filter2Size - 1; j > 0; j--) {
            cutOff += FFABS(filter2[i * filter2Size + j]);

            if (cutOff > SWS_MAX_REDUCE_CUTOFF * fone)
                break;
            min--;
        }

        if (min > minFilterSize)
            minFilterSize = min;
    }


    assert(minFilterSize > 0);
    filterSize = (minFilterSize + (filterAlign - 1)) & (~(filterAlign - 1));
    assert(filterSize > 0);
    filter = (int64_t *)av_malloc(filterSize * dstW * sizeof(*filter));
    if (filterSize >= MAX_FILTER_SIZE * 16 /
                      ((flags & SWS_ACCURATE_RND) ? APCK_SIZE : 16) || !filter)
        goto fail;
    *outFilterSize = filterSize;

    if (flags & SWS_PRINT_INFO)
        hb_log("SwScaler: reducing / aligning filtersize %d -> %d",filter2Size,filterSize);
    for (i = 0; i < dstW; i++) {
        int j;

        for (j = 0; j < filterSize; j++) {
            if (j >= filter2Size)
                filter[i * filterSize + j] = 0;
            else
                filter[i * filterSize + j] = filter2[i * filter2Size + j];
            if ((flags & SWS_BITEXACT) && j >= minFilterSize)
                filter[i * filterSize + j] = 0;
        }
    }

    // FIXME try to align filterPos if possible

    // fix borders
    for (i = 0; i < dstW; i++) {
        int j;
        if ((*filterPos)[i] < 0) {
            // move filter coefficients left to compensate for filterPos
            for (j = 1; j < filterSize; j++) {
                int left = FFMAX(j + (*filterPos)[i], 0);
                filter[i * filterSize + left] += filter[i * filterSize + j];
                filter[i * filterSize + j]     = 0;
            }
            (*filterPos)[i]= 0;
        }

        if ((*filterPos)[i] + filterSize > srcW) {
            int shift = (*filterPos)[i] + filterSize - srcW;
            // move filter coefficients right to compensate for filterPos
            for (j = filterSize - 2; j >= 0; j--) {
                int right = FFMIN(j + shift, filterSize - 1);
                filter[i * filterSize + right] += filter[i * filterSize + j];
                filter[i * filterSize + j]      = 0;
            }
            (*filterPos)[i]= srcW - filterSize;
        }
    }

    // Note the +1 is for the MMX scaler which reads over the end
   // FF_ALLOCZ_OR_GOTO(NULL, *outFilter,
   //                 *outFilterSize * (dstW + 3) * sizeof(int16_t), fail);
    *outFilter = (int16_t *)av_mallocz(*outFilterSize * (dstW + 3) * sizeof(int16_t));
    if( *outFilter == NULL && (*outFilterSize * (dstW + 3) * sizeof(int16_t)) != 0)
    {
        hb_log("Can't alloc memory");
        goto fail;
    }

    for (i = 0; i < dstW; i++) {
        int j;
        int64_t error = 0;
        int64_t sum   = 0;

        for (j = 0; j < filterSize; j++) {
            sum += filter[i * filterSize + j];
        }
        sum = (sum + one / 2) / one;
        for (j = 0; j < *outFilterSize; j++) {
            int64_t v = filter[i * filterSize + j] + error;
            int intV  = ROUNDED_DIV(v, sum);
            (*outFilter)[i * (*outFilterSize) + j] = intV;
            error                                  = v - intV * sum;
        }
    }

    (*filterPos)[dstW + 0] =
    (*filterPos)[dstW + 1] =
    (*filterPos)[dstW + 2] = (*filterPos)[dstW - 1]; 
    for (i = 0; i < *outFilterSize; i++) {
        int k = (dstW - 1) * (*outFilterSize) + i;
        (*outFilter)[k + 1 * (*outFilterSize)] =
        (*outFilter)[k + 2 * (*outFilterSize)] =
        (*outFilter)[k + 3 * (*outFilterSize)] = (*outFilter)[k];
    }

    ret = 0;

fail:
    av_free(filter);
    av_free(filter2);
    return ret;
}

static int handle_scale_jpeg(enum PixelFormat *format)
{                            
    switch (*format) {       
    case AV_PIX_FMT_YUVJ420P:
        *format = AV_PIX_FMT_YUV420P;
        return 1;              
    case AV_PIX_FMT_YUVJ422P:     
        *format = AV_PIX_FMT_YUV422P;                                             
        return 1;                                                              
    case AV_PIX_FMT_YUVJ444P:                                                
        *format = AV_PIX_FMT_YUV444P;                             
        return 1;                                       
    case AV_PIX_FMT_YUVJ440P:                              
        *format = AV_PIX_FMT_YUV440P;
        return 1;   
    default:
        return 0;
    }        
}

static void scaleGetSubSampleFactors(int *h, int *v, enum PixelFormat format)
{
    *h = av_pix_fmt_descriptors[format].log2_chroma_w;
    *v = av_pix_fmt_descriptors[format].log2_chroma_h;
}

typedef struct FormatEntry {
    int is_supported_in, is_supported_out;
} FormatEntry;

static const FormatEntry format_entries[AV_PIX_FMT_NB] = {
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 0 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 0, 0 },
    { 1, 1 },
    { 0, 1 },
    { 1, 1 },
    { 1, 1 },
    { 0, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 0 },
    { 1, 0 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 0 },
    { 1, 1 },
    { 1, 1 },
    { 0, 0 },
    { 0, 0 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 0 },
    { 1, 0 },
    { 1, 0 },
    { 1, 0 },
    { 1, 0 },
    { 1, 0 },
    { 1, 0 },
};

int scale_isSupportedInput(enum PixelFormat pix_fmt)
{
    return (unsigned)pix_fmt < AV_PIX_FMT_NB ?
           format_entries[pix_fmt].is_supported_in : 0;
}

int scale_isSupportedOutput(enum PixelFormat pix_fmt)
{
    return (unsigned)pix_fmt < AV_PIX_FMT_NB ?
           format_entries[pix_fmt].is_supported_out : 0;
}

static void hcscale_fast_c(ScaleContext *c, int16_t *dst1, int16_t *dst2,
                           int dstWidth, const uint8_t *src1,
                           const uint8_t *src2, int srcW, int xInc)
{
    int i;
    unsigned int xpos = 0;
    for (i = 0; i < dstWidth; i++) {
        register unsigned int xx     = xpos >> 16;
        register unsigned int xalpha = (xpos & 0xFFFF) >> 9;
        dst1[i] = (src1[xx] * (xalpha ^ 127) + src1[xx + 1] * xalpha);
        dst2[i] = (src2[xx] * (xalpha ^ 127) + src2[xx + 1] * xalpha);
        xpos   += xInc;
    }
    for (i=dstWidth-1; (i*xInc)>>16 >=srcW-1; i--) {
        dst1[i] = src1[srcW-1]*128;
        dst2[i] = src2[srcW-1]*128;
    }
}

static void hyscale_fast_c(ScaleContext *c, int16_t *dst, int dstWidth,
                           const uint8_t *src, int srcW, int xInc)
{
    int i;
    unsigned int xpos = 0;
    for (i = 0; i < dstWidth; i++) {
        register unsigned int xx     = xpos >> 16;
        register unsigned int xalpha = (xpos & 0xFFFF) >> 9;
        dst[i] = (src[xx] << 7) + (src[xx + 1] - src[xx]) * xalpha;
        xpos  += xInc;
    }
    for (i=dstWidth-1; (i*xInc)>>16 >=srcW-1; i--)
        dst[i] = src[srcW-1]*128;
}

static void hScale16To19_c(ScaleContext *c, int16_t *_dst, int dstW,
                           const uint8_t *_src, const int16_t *filter,
                           const int32_t *filterPos, int filterSize)
{
    int i;
    int32_t *dst        = (int32_t *) _dst;
    const uint16_t *src = (const uint16_t *) _src;
    int bits            = av_pix_fmt_descriptors[c->srcFormat].comp[0].depth_minus1;
    int sh              = bits - 4;

    if((isScaleAnyRGB(c->srcFormat) || c->srcFormat==AV_PIX_FMT_PAL8) && av_pix_fmt_descriptors[c->srcFormat].comp[0].depth_minus1<15)
        sh= 9;

    for (i = 0; i < dstW; i++) {
        int j;
        int srcPos = filterPos[i];
        int val    = 0;

        for (j = 0; j < filterSize; j++) {
            val += src[srcPos + j] * filter[filterSize * i + j];
        }
        dst[i] = FFMIN(val >> sh, (1 << 19) - 1);
    }
}

static void hScale16To15_c(ScaleContext *c, int16_t *dst, int dstW,
                           const uint8_t *_src, const int16_t *filter,
                           const int32_t *filterPos, int filterSize)
{
    int i;
    const uint16_t *src = (const uint16_t *) _src;
    int sh              = av_pix_fmt_descriptors[c->srcFormat].comp[0].depth_minus1;

    if(sh<15)
        sh= isScaleAnyRGB(c->srcFormat) || c->srcFormat==AV_PIX_FMT_PAL8 ? 13 : av_pix_fmt_descriptors[c->srcFormat].comp[0].depth_minus1;

    for (i = 0; i < dstW; i++) {
        int j;
        int srcPos = filterPos[i];
        int val    = 0;

        for (j = 0; j < filterSize; j++) {
            val += src[srcPos + j] * filter[filterSize * i + j];
        }
        // filter=14 bit, input=16 bit, output=30 bit, >> 15 makes 15 bit
        dst[i] = FFMIN(val >> sh, (1 << 15) - 1);
    }
}

static void hScale8To15_c(ScaleContext *c, int16_t *dst, int dstW,
                          const uint8_t *src, const int16_t *filter,
                          const int32_t *filterPos, int filterSize)
{
    int i;
    for (i = 0; i < dstW; i++) {
        int j;
        int srcPos = filterPos[i];
        int val    = 0;
        for (j = 0; j < filterSize; j++) {
            val += ((int)src[srcPos + j]) * filter[filterSize * i + j];
        }
        dst[i] = FFMIN(val >> 7, (1 << 15) - 1); // the cubic equation does overflow ...
    }
}

static void hScale8To19_c(ScaleContext *c, int16_t *_dst, int dstW,
                          const uint8_t *src, const int16_t *filter,
                          const int32_t *filterPos, int filterSize)
{
    int i;
    int32_t *dst = (int32_t *) _dst;
    for (i = 0; i < dstW; i++) {
        int j;
        int srcPos = filterPos[i];
        int val    = 0;
        for (j = 0; j < filterSize; j++) {
            val += ((int)src[srcPos + j]) * filter[filterSize * i + j];
        }
        dst[i] = FFMIN(val >> 3, (1 << 19) - 1); // the cubic equation does overflow ...
    }
}

static void chrRangeToJpeg_c(int16_t *dstU, int16_t *dstV, int width)
{
    int i;
    for (i = 0; i < width; i++) {
        dstU[i] = (FFMIN(dstU[i], 30775) * 4663 - 9289992) >> 12; // -264
        dstV[i] = (FFMIN(dstV[i], 30775) * 4663 - 9289992) >> 12; // -264
    }
}

static void chrRangeFromJpeg_c(int16_t *dstU, int16_t *dstV, int width)
{
    int i;
    for (i = 0; i < width; i++) {
        dstU[i] = (dstU[i] * 1799 + 4081085) >> 11; // 1469
        dstV[i] = (dstV[i] * 1799 + 4081085) >> 11; // 1469
    }
}

static void lumRangeToJpeg_c(int16_t *dst, int width)
{
    int i;
    for (i = 0; i < width; i++)
        dst[i] = (FFMIN(dst[i], 30189) * 19077 - 39057361) >> 14;
}

static void lumRangeFromJpeg_c(int16_t *dst, int width)
{
    int i;
    for (i = 0; i < width; i++)
        dst[i] = (dst[i] * 14071 + 33561947) >> 14;
}

static void chrRangeToJpeg16_c(int16_t *_dstU, int16_t *_dstV, int width)
{
    int i;
    int32_t *dstU = (int32_t *) _dstU;
    int32_t *dstV = (int32_t *) _dstV;
    for (i = 0; i < width; i++) {
        dstU[i] = (FFMIN(dstU[i], 30775 << 4) * 4663 - (9289992 << 4)) >> 12; // -264
        dstV[i] = (FFMIN(dstV[i], 30775 << 4) * 4663 - (9289992 << 4)) >> 12; // -264
    }
}

static void chrRangeFromJpeg16_c(int16_t *_dstU, int16_t *_dstV, int width)
{
    int i;
    int32_t *dstU = (int32_t *) _dstU;
    int32_t *dstV = (int32_t *) _dstV;
    for (i = 0; i < width; i++) {
        dstU[i] = (dstU[i] * 1799 + (4081085 << 4)) >> 11; // 1469
        dstV[i] = (dstV[i] * 1799 + (4081085 << 4)) >> 11; // 1469
    }
}

static void lumRangeToJpeg16_c(int16_t *_dst, int width)
{
    int i;
    int32_t *dst = (int32_t *) _dst;
    for (i = 0; i < width; i++)
        dst[i] = (FFMIN(dst[i], 30189 << 4) * 4769 - (39057361 << 2)) >> 12;
}

static void lumRangeFromJpeg16_c(int16_t *_dst, int width)
{
    int i;
    int32_t *dst = (int32_t *) _dst;
    for (i = 0; i < width; i++)
        dst[i] = (dst[i]*(14071/4) + (33561947<<4)/4)>>12;
}

static av_cold void sws_init_swScale_c(ScaleContext *c)
{
    enum PixelFormat srcFormat = c->srcFormat;

    ff_sws_init_output_funcs(c, &c->yuv2plane1, &c->yuv2planeX,
                             &c->yuv2nv12cX, &c->yuv2packed1,
                             &c->yuv2packed2, &c->yuv2packedX);

    ff_sws_init_input_funcs(c);

    if (c->srcBpc == 8) {
        if (c->dstBpc <= 10) {
            c->hyScale = c->hcScale = hScale8To15_c;
            if (c->flags & SWS_FAST_BILINEAR) {
                c->hyscale_fast = hyscale_fast_c;
                c->hcscale_fast = hcscale_fast_c;
            }
        } else {
            c->hyScale = c->hcScale = hScale8To19_c;
        }
    } else {
        c->hyScale = c->hcScale = c->dstBpc > 10 ? hScale16To19_c
                                                 : hScale16To15_c;
    }

    if (c->srcRange != c->dstRange && !isScaleAnyRGB(c->dstFormat)) {
        if (c->dstBpc <= 10) {
            if (c->srcRange) {
                c->lumConvertRange = lumRangeFromJpeg_c;
                c->chrConvertRange = chrRangeFromJpeg_c;
            } else {
                c->lumConvertRange = lumRangeToJpeg_c;
                c->chrConvertRange = chrRangeToJpeg_c;
            }
        } else {
            if (c->srcRange) {
                c->lumConvertRange = lumRangeFromJpeg16_c;
                c->chrConvertRange = chrRangeFromJpeg16_c;
            } else {
                c->lumConvertRange = lumRangeToJpeg16_c;
                c->chrConvertRange = chrRangeToJpeg16_c;
            }
        }
    }

    if (!(isScaleGray(srcFormat) || isScaleGray(c->dstFormat) ||
          srcFormat == AV_PIX_FMT_MONOBLACK || srcFormat == AV_PIX_FMT_MONOWHITE))
        c->needs_hcscale = 1;
}

int scale_init_context(ScaleContext *c, ScaleFilter *srcFilter, ScaleFilter *dstFilter)
{
    ScaleFilter dummyFilter = { NULL, NULL, NULL, NULL };
    int srcW              = c->srcW;
    int srcH              = c->srcH;
    int dstW              = c->dstW;
    int dstH              = c->dstH;
    int flags, cpu_flags;
    enum PixelFormat srcFormat = c->srcFormat;
    enum PixelFormat dstFormat = c->dstFormat;

    cpu_flags = 0;
    flags     = c->flags;

    if(srcFormat != c->srcFormat || dstFormat != c->dstFormat){
        hb_log("deprecated pixel format used, make sure you did set range correctly.");
        c->srcFormat = srcFormat;
        c->dstFormat = dstFormat;
    }

    if (srcW < 4 || srcH < 1 || dstW < 8 || dstH < 1) {
        hb_log("%dx%d -> %dx%d is invalid scaling dimension.",srcW,srcH,dstW,dstH);
        return -1;
    }

    if (!dstFilter)
        dstFilter = &dummyFilter;
    if (!srcFilter)
        srcFilter = &dummyFilter;

    c->lumXInc      = (((int64_t)srcW << 16) + (dstW >> 1)) / dstW;
    c->lumYInc      = (((int64_t)srcH << 16) + (dstH >> 1)) / dstH;
    c->dstFormatBpp = av_get_bits_per_pixel(&av_pix_fmt_descriptors[dstFormat]);
    c->srcFormatBpp = av_get_bits_per_pixel(&av_pix_fmt_descriptors[srcFormat]);
    c->vRounder     = 4 * 0x0001000100010001ULL;

    scaleGetSubSampleFactors(&c->chrSrcHSubSample, &c->chrSrcVSubSample, srcFormat);
    scaleGetSubSampleFactors(&c->chrDstHSubSample, &c->chrDstVSubSample, dstFormat);

    // drop some chroma lines if the user wants it
    c->vChrDrop          = (flags & SWS_SRC_V_CHR_DROP_MASK) >> SWS_SRC_V_CHR_DROP_SHIFT;
    c->chrSrcVSubSample += c->vChrDrop;
    c->chrSrcW = -((-srcW) >> c->chrSrcHSubSample);
    c->chrSrcH = -((-srcH) >> c->chrSrcVSubSample);
    c->chrDstW = -((-dstW) >> c->chrDstHSubSample);
    c->chrDstH = -((-dstH) >> c->chrDstVSubSample);
    c->chrXInc = (((int64_t)c->chrSrcW << 16) + (c->chrDstW >> 1)) / c->chrDstW;
    c->chrYInc = (((int64_t)c->chrSrcH << 16) + (c->chrDstH >> 1)) / c->chrDstH;

    const int filterAlign = 1;

    if (initScaleFilter(&c->hLumFilter, &c->hLumFilterPos,
                        &c->hLumFilterSize, c->lumXInc,
                        srcW, dstW, filterAlign, 1 << 14,
                        (flags & SWS_BICUBLIN) ? (flags | SWS_BICUBIC) : flags,
                        cpu_flags, srcFilter->lumH, dstFilter->lumH,
                        c->param) < 0)
        goto fail;

    if (initScaleFilter(&c->hChrFilter, &c->hChrFilterPos,
                        &c->hChrFilterSize, c->chrXInc,
                        c->chrSrcW, c->chrDstW, filterAlign, 1 << 14,
                        (flags & SWS_BICUBLIN) ? (flags | SWS_BILINEAR) : flags,
                        cpu_flags, srcFilter->chrH, dstFilter->chrH,
                        c->param) < 0)
        goto fail;

    if (initScaleFilter(&c->vLumFilter, &c->vLumFilterPos, &c->vLumFilterSize,
                       c->lumYInc, srcH, dstH, filterAlign, (1 << 12),
                       (flags & SWS_BICUBLIN) ? (flags | SWS_BICUBIC) : flags,
                       cpu_flags, srcFilter->lumV, dstFilter->lumV,
                       c->param) < 0)
        goto fail;

    if (initScaleFilter(&c->vChrFilter, &c->vChrFilterPos, &c->vChrFilterSize,
                       c->chrYInc, c->chrSrcH, c->chrDstH,
                       filterAlign, (1 << 12),
                       (flags & SWS_BICUBLIN) ? (flags | SWS_BILINEAR) : flags,
                       cpu_flags, srcFilter->chrV, dstFilter->chrV,
                       c->param) < 0)
        goto fail;
    return 0;
fail:
    return -1;
}

ScaleContext *scale_getContext(int srcW, int srcH, enum PixelFormat srcFormat,
                           int dstW, int dstH, enum PixelFormat dstFormat,
                           int flags, ScaleFilter *srcFilter,
                           ScaleFilter *dstFilter, const double *param)
{
    ScaleContext *sc = (ScaleContext*)malloc(sizeof(ScaleContext));
    sc->flags     = flags;
    sc->srcW      = srcW;
    sc->srcH      = srcH;
    sc->dstW      = dstW;
    sc->dstH      = dstH;
    sc->srcRange  = handle_scale_jpeg(&srcFormat);
    sc->dstRange  = handle_scale_jpeg(&dstFormat);
    sc->srcFormat = srcFormat;
    sc->dstFormat = dstFormat;
    sc->hyscale_fast = 0;
    sc->hcscale_fast = 0;

    if (param) {
        sc->param[0] = param[0];
        sc->param[1] = param[1];
    }    

    if (scale_init_context(sc, srcFilter, dstFilter) < 0) { 
        sws_freeContext(sc);
        return NULL;
    }    

    return sc;
}

int scale_opencl(ScaleContext *c, 
                 void *cl_inbuf,
                 void *cl_outbuf,
                 int *srcStride,
                 int *dstStride)
{
    int should_dither = is9_OR_10BPS(c->srcFormat) || is16BPS(c->srcFormat);   
                                                                      
    av_scale_frame(c,cl_outbuf,cl_inbuf,srcStride,dstStride,&should_dither);
                                                       
    return 1;                       
}

void scale_init( int width, int height, int dstwidth, int dstheight )
{
    int srcW = width;
    int srcH = height;
    int dstW = dstwidth;
    int dstH = dstheight;
    enum PixelFormat inputfmt = AV_PIX_FMT_YUV420P;
    enum PixelFormat outputfmt = AV_PIX_FMT_YUV420P;
    int flags = SWS_BILINEAR;

    g_scale = scale_getContext(srcW,srcH,inputfmt,dstW,dstH,outputfmt,flags,NULL,NULL,NULL);
}

void scale_release()
{
    sws_freeContext( g_scale );
}
#ifdef USE_OPENCL
int scale_run( cl_mem inbuf, cl_mem outbuf, int linesizey, int linesizeuv, int height )
{
    g_scale->cl_src = inbuf;
    g_scale->cl_dst = outbuf;

    int src_stride[4] = { linesizey, linesizeuv, linesizeuv, 0 };
    int dst_stride[4] = { g_scale->dstW, g_scale->chrDstW, g_scale->chrDstW, 0 };
    int ret = -1;

    ret = scale_opencl( g_scale, inbuf, outbuf, src_stride, dst_stride );

    return ret;
}
#endif
#endif
