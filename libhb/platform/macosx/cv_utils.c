/* cv_utils.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "libavutil/avutil.h"
#include "cv_utils.h"

OSType hb_cv_get_pixel_format(enum AVPixelFormat pix_fmt, enum AVColorRange color_range)
{
    if (pix_fmt == AV_PIX_FMT_NV12)
    {
        return color_range == AVCOL_RANGE_JPEG ?
                                kCVPixelFormatType_420YpCbCr8BiPlanarFullRange :
                                kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
    }
    else if (pix_fmt == AV_PIX_FMT_YUV420P)
    {
        return color_range == AVCOL_RANGE_JPEG ?
                                kCVPixelFormatType_420YpCbCr8PlanarFullRange :
                                kCVPixelFormatType_420YpCbCr8Planar;
    }
    else if (pix_fmt == AV_PIX_FMT_BGRA)
    {
        return kCVPixelFormatType_32BGRA;
    }
    else if (pix_fmt == AV_PIX_FMT_P010)
    {
        return color_range == AVCOL_RANGE_JPEG ?
                                kCVPixelFormatType_420YpCbCr10BiPlanarFullRange :
                                kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange;
    }
    else if (pix_fmt == AV_PIX_FMT_NV16)
    {
        return color_range == AVCOL_RANGE_JPEG ?
                                kCVPixelFormatType_422YpCbCr8BiPlanarFullRange :
                                kCVPixelFormatType_422YpCbCr8BiPlanarVideoRange;
    }
    else if (pix_fmt == AV_PIX_FMT_P210)
    {
        return color_range == AVCOL_RANGE_JPEG ?
                                kCVPixelFormatType_422YpCbCr10BiPlanarFullRange :
                                kCVPixelFormatType_422YpCbCr10BiPlanarVideoRange;
    }
    else if (pix_fmt == AV_PIX_FMT_P212 ||
             pix_fmt == AV_PIX_FMT_P216)
    {
        return color_range == kCVPixelFormatType_422YpCbCr16BiPlanarVideoRange;
    }
    else if (pix_fmt == AV_PIX_FMT_NV24)
    {
        return color_range == AVCOL_RANGE_JPEG ?
                                kCVPixelFormatType_444YpCbCr8BiPlanarFullRange :
                                kCVPixelFormatType_444YpCbCr8BiPlanarVideoRange;
    }
    else if (pix_fmt == AV_PIX_FMT_P410)
    {
        return color_range == AVCOL_RANGE_JPEG ?
                                kCVPixelFormatType_444YpCbCr10BiPlanarFullRange :
                                kCVPixelFormatType_444YpCbCr10BiPlanarVideoRange;
    }
    else if (pix_fmt == AV_PIX_FMT_P412 ||
             pix_fmt == AV_PIX_FMT_P416)
    {
        return kCVPixelFormatType_444YpCbCr16BiPlanarVideoRange;
    }
    else
    {
        return 0;
    }
}

CVPixelBufferRef hb_cv_get_pixel_buffer(const hb_buffer_t *buf)
{
    if (buf->storage_type == AVFRAME)
    {
        return (CVPixelBufferRef)((AVFrame *)buf->storage)->data[3];
    }
    else if (buf->storage_type == COREMEDIA)
    {
        return (CVPixelBufferRef)buf->storage;
    }
    else
    {
        hb_log("corevideo: unknown storage");
    }
    return NULL;
}

CVPixelBufferPoolRef hb_cv_create_pixel_buffer_pool(int width, int height, enum AVPixelFormat pix_fmt, enum AVColorRange color_range)
{
    // CVPixelBuffer pool
    // Set the Metal compatibility key
    // to keep the buffer on the GPU memory
    OSType cv_pix_fmt = hb_cv_get_pixel_format(pix_fmt, color_range);
    CFNumberRef pix_fmt_num = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &cv_pix_fmt);
    CFNumberRef width_num   = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &width);
    CFNumberRef height_num  = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &height);

    const void *attrs_keys[4] =
    {
        kCVPixelBufferWidthKey,
        kCVPixelBufferHeightKey,
        kCVPixelBufferPixelFormatTypeKey,
        kCVPixelBufferMetalCompatibilityKey
    };
    const void *attrs_values[4] =
    {
        width_num,
        height_num,
        pix_fmt_num,
        kCFBooleanTrue
    };

    CFDictionaryRef attrs = CFDictionaryCreate(kCFAllocatorDefault,
                                               attrs_keys,
                                               attrs_values,
                                               4,
                                               &kCFTypeDictionaryKeyCallBacks,
                                               &kCFTypeDictionaryValueCallBacks);

    CFRelease(width_num);
    CFRelease(height_num);
    CFRelease(pix_fmt_num);

    CVPixelBufferPoolRef pool;
    CVReturn ret = CVPixelBufferPoolCreate(kCFAllocatorDefault, NULL, attrs, &pool);
    CFRelease(attrs);
    if (ret != kCVReturnSuccess)
    {
        hb_log("corevideo: CVPixelBufferPoolCreate failed");
        return NULL;
    }

    return pool;
}

CFStringRef hb_cv_colr_pri_xlat(int color_prim)
{
    switch (color_prim)
    {
        case HB_COLR_PRI_BT2020:
            return kCMFormatDescriptionColorPrimaries_ITU_R_2020;
        case HB_COLR_PRI_BT709:
            return kCMFormatDescriptionColorPrimaries_ITU_R_709_2;
        case HB_COLR_PRI_EBUTECH:
            return kCMFormatDescriptionColorPrimaries_EBU_3213;
        case HB_COLR_PRI_SMPTEC:
            return kCMFormatDescriptionColorPrimaries_SMPTE_C;
        default:
            return NULL;
    }
}

CFStringRef hb_cv_colr_tra_xlat(int color_transfer)
{
    switch (color_transfer)
    {
        case HB_COLR_TRA_BT709:
            return kCMFormatDescriptionTransferFunction_ITU_R_709_2;
        case HB_COLR_TRA_SMPTE240M:
            return kCMFormatDescriptionTransferFunction_SMPTE_240M_1995;
        case HB_COLR_TRA_SMPTEST2084:
            return kCVImageBufferTransferFunction_SMPTE_ST_2084_PQ;
        case HB_COLR_TRA_LINEAR:
            if (__builtin_available(macOS 10.14, *)) { return kCVImageBufferTransferFunction_Linear; }
        case HB_COLR_TRA_IEC61966_2_1:
            return kCVImageBufferTransferFunction_sRGB;
        case HB_COLR_TRA_ARIB_STD_B67:
            return kCVImageBufferTransferFunction_ITU_R_2100_HLG;
        case HB_COLR_TRA_GAMMA22:
            return kCVImageBufferTransferFunction_UseGamma;
        case HB_COLR_TRA_GAMMA28:
            return kCVImageBufferTransferFunction_UseGamma;
        case HB_COLR_TRA_BT2020_10:
        case HB_COLR_TRA_BT2020_12:
            return kCVImageBufferTransferFunction_ITU_R_2020;
        default:
            return NULL;
    }
}

CFNumberRef hb_cv_colr_gamma_xlat(int color_transfer) CF_RETURNS_RETAINED
{
    Float32 gamma = 0;
    switch (color_transfer)
    {
        case HB_COLR_TRA_GAMMA22:
            gamma = 2.2;
        case HB_COLR_TRA_GAMMA28:
            gamma = 2.8;
    }

    return gamma > 0 ? CFNumberCreate(NULL, kCFNumberFloat32Type, &gamma) : NULL;
}

CFStringRef hb_cv_colr_mat_xlat(int color_matrix)
{
    switch (color_matrix)
    {
        case HB_COLR_MAT_BT2020_NCL:
            return kCMFormatDescriptionYCbCrMatrix_ITU_R_2020;
        case HB_COLR_MAT_BT709:
            return kCMFormatDescriptionYCbCrMatrix_ITU_R_709_2;
        case HB_COLR_MAT_SMPTE170M:
            return kCMFormatDescriptionYCbCrMatrix_ITU_R_601_4;
        case HB_COLR_MAT_SMPTE240M:
            return kCMFormatDescriptionYCbCrMatrix_SMPTE_240M_1995;
        default:
            return NULL;
    }
}

CFStringRef hb_cv_chroma_loc_xlat(int chroma_location)
{
    switch (chroma_location)
    {
        case AVCHROMA_LOC_LEFT:
            return kCVImageBufferChromaLocation_Left;
        case AVCHROMA_LOC_CENTER:
            return kCVImageBufferChromaLocation_Center;
        case AVCHROMA_LOC_TOPLEFT:
            return kCVImageBufferChromaLocation_TopLeft;
        case AVCHROMA_LOC_TOP:
            return kCVImageBufferChromaLocation_Top;
        case AVCHROMA_LOC_BOTTOMLEFT:
            return kCVImageBufferChromaLocation_BottomLeft;
        case AVCHROMA_LOC_BOTTOM:
            return kCVImageBufferChromaLocation_Bottom;
        default:
            return NULL;
    }
}

void hb_cv_add_color_tag(CVPixelBufferRef pix_buf,
                         int color_prim, int color_transfer,
                         int color_matrix, int chroma_location)
{
    CFStringRef prim       = hb_cv_colr_pri_xlat(color_prim);
    CFStringRef transfer   = hb_cv_colr_tra_xlat(color_transfer);
    CFNumberRef gamma      = hb_cv_colr_gamma_xlat(color_transfer);
    CFStringRef matrix     = hb_cv_colr_mat_xlat(color_matrix);
    CFStringRef chroma_loc = hb_cv_chroma_loc_xlat(chroma_location);

    CVBufferRemoveAllAttachments(pix_buf);

    if (prim)
    {
        CVBufferSetAttachment(pix_buf, kCVImageBufferColorPrimariesKey, prim, kCVAttachmentMode_ShouldPropagate);
    }
    if (transfer)
    {
        CVBufferSetAttachment(pix_buf, kCVImageBufferTransferFunctionKey, transfer, kCVAttachmentMode_ShouldPropagate);
    }
    if (gamma)
    {
        CVBufferSetAttachment(pix_buf, kCVImageBufferGammaLevelKey, gamma, kCVAttachmentMode_ShouldPropagate);
        CFRelease(gamma);
    }
    if (matrix)
    {
        CVBufferSetAttachment(pix_buf, kCVImageBufferYCbCrMatrixKey, matrix, kCVAttachmentMode_ShouldPropagate);
    }
    if (chroma_loc)
    {
        CVBufferSetAttachment(pix_buf, kCVImageBufferChromaLocationTopFieldKey, chroma_loc, kCVAttachmentMode_ShouldPropagate);
        CVBufferSetAttachment(pix_buf, kCVImageBufferChromaLocationBottomFieldKey, chroma_loc, kCVAttachmentMode_ShouldPropagate);
    }
}

int hb_cv_match_rgb_to_colorspace(int rgb,
                                  int color_prim,
                                  int color_transfer,
                                  int color_matrix)
{
    const unsigned r = (rgb >> 16) & 0xff;
    const unsigned g = (rgb >> 8) & 0xff;
    const unsigned b = (rgb) & 0xff;

    if (__builtin_available(macOS 10.15, *)) {

        CGColorRef rgb_color = CGColorCreateSRGB(r / 255.f, g / 255.f, b / 255.f, 1.f);

        CFStringRef prim     = CVColorPrimariesGetStringForIntegerCodePoint(color_prim);
        CFStringRef transfer = CVTransferFunctionGetStringForIntegerCodePoint(color_transfer);
        CFNumberRef gamma    = hb_cv_colr_gamma_xlat(color_transfer);
        CFStringRef matrix   = CVYCbCrMatrixGetStringForIntegerCodePoint(color_matrix);

        CGColorSpaceRef colorspace = NULL;
        if (transfer == kCVImageBufferTransferFunction_UseGamma)
        {
            const void *keys[4] = { kCVImageBufferColorPrimariesKey, kCVImageBufferTransferFunctionKey,
                kCVImageBufferYCbCrMatrixKey, kCVImageBufferGammaLevelKey };
            const void *values[4] = { prim, transfer, matrix, gamma };
            CFDictionaryRef attachments = CFDictionaryCreate(NULL, keys, values, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

            colorspace = CVImageBufferCreateColorSpaceFromAttachments(attachments);
            CFRelease(attachments);
        }
        else
        {
            const void *keys[3] = { kCVImageBufferColorPrimariesKey, kCVImageBufferTransferFunctionKey, kCVImageBufferYCbCrMatrixKey };
            const void *values[3] = { prim, transfer, gamma };
            CFDictionaryRef attachments = CFDictionaryCreate(NULL, keys, values, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

            colorspace = CVImageBufferCreateColorSpaceFromAttachments(attachments);
            CFRelease(attachments);
        }

        if (colorspace == NULL)
        {
            CFRelease(rgb_color);
            hb_log("cgcolor: unable to match color to colorspace");
            return rgb;
        }

        CGColorRef matched_color = CGColorCreateCopyByMatchingToColorSpace(colorspace,
                                                                           kCGRenderingIntentPerceptual,
                                                                           rgb_color, NULL);
        CFRelease(colorspace);
        CFRelease(rgb_color);
        if (matched_color == NULL)
        {
            hb_log("cgcolor: unable to match color to colorspace");
            return rgb;
        }

        const CGFloat *components = CGColorGetComponents(matched_color);
        const int color = ((int)(components[0] * 255) << 16) | ((int)(components[1] * 255) << 8) | (int)(components[2] * 255);
        CFRelease(matched_color);
        return color;
    }
    else
    {
        return rgb;
    }
}
