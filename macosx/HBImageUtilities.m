/*  HBColorUtilities.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBImageUtilities.h"
#import <CoreVideo/CoreVideo.h>
#include "handbrake/handbrake.h"

CGImageRef CreateScaledCGImageFromCGImage(CGImageRef image, CGFloat thumbnailHeight)
{
    // Create the bitmap context
    CGContextRef    context = NULL;
    void *          bitmapData;
    int             bitmapByteCount;
    int             bitmapBytesPerRow;

    // Get image width, height. We'll use the entire image.
    int width = (CGFloat)CGImageGetWidth(image) / (CGFloat)CGImageGetHeight(image) * thumbnailHeight;
    int height = thumbnailHeight;

    // Declare the number of bytes per row. Each pixel in the bitmap in this
    // example is represented by 4 bytes; 8 bits each of red, green, blue, and
    // alpha.
    bitmapBytesPerRow   = (width * 4);
    bitmapByteCount     = (bitmapBytesPerRow * height);

    // Allocate memory for image data. This is the destination in memory
    // where any drawing to the bitmap context will be rendered.
    bitmapData = malloc(bitmapByteCount);
    if (bitmapData == NULL)
    {
        return nil;
    }

    // Create the bitmap context. We want pre-multiplied ARGB, 8-bits
    // per component. Regardless of what the source image format is
    // (CMYK, Grayscale, and so on) it will be converted over to the format
    // specified here by CGBitmapContextCreate.
    CGColorSpaceRef colorspace = CGImageGetColorSpace(image);
    context = CGBitmapContextCreate (bitmapData,width,height,8,bitmapBytesPerRow,
                                     colorspace,kCGImageAlphaNoneSkipFirst);

    if (context == NULL)
    {
        // error creating context
        return nil;
    }

    // Draw the image to the bitmap context. Once we draw, the memory
    // allocated for the context for rendering will then contain the
    // raw image data in the specified color space.
    CGContextDrawImage(context, CGRectMake(0,0,width, height), image);

    CGImageRef imgRef = CGBitmapContextCreateImage(context);
    CGContextRelease(context);
    free(bitmapData);

    return imgRef;
}


CGImageRef CGImageRotated(CGImageRef imgRef, CGFloat angle, BOOL flipped) CF_RETURNS_RETAINED
{
    CGFloat angleInRadians = angle * (M_PI / 180);
    CGFloat width = CGImageGetWidth(imgRef);
    CGFloat height = CGImageGetHeight(imgRef);

    CGRect imgRect = CGRectMake(0, 0, width, height);
    CGAffineTransform transform = CGAffineTransformMakeRotation(angleInRadians);
    CGRect rotatedRect = CGRectApplyAffineTransform(imgRect, transform);

    CGColorSpaceRef colorSpace = CGImageGetColorSpace(imgRef);
    CGContextRef bmContext = CGBitmapContextCreate(NULL,
                                                   (size_t)rotatedRect.size.width,
                                                   (size_t)rotatedRect.size.height,
                                                   8,
                                                   0,
                                                   colorSpace,
                                                   kCGImageAlphaPremultipliedFirst);
    CGContextSetAllowsAntialiasing(bmContext, FALSE);
    CGContextSetInterpolationQuality(bmContext, kCGInterpolationNone);

    // Rotate
    CGContextTranslateCTM(bmContext,
                          + (rotatedRect.size.width / 2),
                          + (rotatedRect.size.height / 2));
    CGContextRotateCTM(bmContext, -angleInRadians);
    CGContextTranslateCTM(bmContext,
                          - (rotatedRect.size.width / 2),
                          - (rotatedRect.size.height / 2));

    // Flip
    if (flipped)
    {
        CGAffineTransform flipHorizontal = CGAffineTransformMake(-1, 0, 0, 1, floor(rotatedRect.size.width), 0);
        CGContextConcatCTM(bmContext, flipHorizontal);
    }

    CGContextDrawImage(bmContext,
                       CGRectMake((rotatedRect.size.width - width)/2.0f,
                                  (rotatedRect.size.height - height)/2.0f,
                                  width,
                                  height),
                       imgRef);

    CGImageRef rotatedImage = CGBitmapContextCreateImage(bmContext);
    CFRelease(bmContext);

    return rotatedImage;
}

CGColorSpaceRef copyColorSpace(int primaries, int transfer, int matrix)
{
    CFStringRef primariesValue = NULL;
    switch (primaries)
    {
        case HB_COLR_PRI_EBUTECH:
            primariesValue = kCVImageBufferColorPrimaries_EBU_3213;
            break;

        case HB_COLR_PRI_SMPTEC:
            primariesValue = kCVImageBufferColorPrimaries_SMPTE_C;
            break;

        case HB_COLR_PRI_BT2020:
            primariesValue = kCVImageBufferColorPrimaries_ITU_R_2020;
            break;

        case HB_COLR_PRI_BT709:
        default:
            primariesValue = kCVImageBufferColorPrimaries_ITU_R_709_2;
    }

    CFStringRef transferValue = NULL;
    CFNumberRef gammaValue = NULL;
    switch (transfer)
    {
        case HB_COLR_TRA_GAMMA22:
        {
            transferValue = kCVImageBufferTransferFunction_UseGamma;
            Float32 gamma = 2.2;
            gammaValue = CFNumberCreate(NULL, kCFNumberFloat32Type, &gamma);
            break;
        }

        case HB_COLR_TRA_GAMMA28:
        {
            transferValue = kCVImageBufferTransferFunction_UseGamma;
            Float32 gamma = 2.8;
            gammaValue = CFNumberCreate(NULL, kCFNumberFloat32Type, &gamma);
            break;
        }

        case HB_COLR_TRA_SMPTE240M:
            transferValue = kCVImageBufferTransferFunction_SMPTE_240M_1995;
            break;

        case HB_COLR_TRA_LINEAR:
            if (@available(macOS 10.14, *)) {
            transferValue = kCVImageBufferTransferFunction_Linear;
            break;
            }

        case HB_COLR_TRA_IEC61966_2_1:
            transferValue = kCVImageBufferTransferFunction_sRGB;
            break;

        case HB_COLR_TRA_BT2020_10:
        case HB_COLR_TRA_BT2020_12:
            transferValue = kCVImageBufferTransferFunction_ITU_R_2020;
            break;

        case HB_COLR_TRA_SMPTEST2084:
            transferValue = kCVImageBufferTransferFunction_SMPTE_ST_2084_PQ;
            break;

        case HB_COLR_TRA_ARIB_STD_B67:
            transferValue = kCVImageBufferTransferFunction_ITU_R_2100_HLG;
            break;

        case HB_COLR_TRA_SMPTE428:
            transferValue = kCVImageBufferTransferFunction_SMPTE_ST_428_1;
            break;

        case HB_COLR_TRA_BT709:
        default:
            transferValue = kCVImageBufferTransferFunction_ITU_R_709_2;
    }

    CFStringRef matrixValue = NULL;
    switch (matrix)
    {
        case HB_COLR_MAT_SMPTE170M:
            matrixValue = kCVImageBufferYCbCrMatrix_ITU_R_601_4;
            break;

        case HB_COLR_MAT_SMPTE240M:
            matrixValue = kCVImageBufferYCbCrMatrix_SMPTE_240M_1995;
            break;

        case HB_COLR_MAT_BT2020_NCL:
        case HB_COLR_MAT_BT2020_CL:
            matrixValue = kCVImageBufferYCbCrMatrix_ITU_R_2020;
            break;

        case HB_COLR_MAT_BT709:
        default:
            matrixValue = kCVImageBufferYCbCrMatrix_ITU_R_709_2;
    }

    CGColorSpaceRef colorSpace = NULL;
    if (transferValue == kCVImageBufferTransferFunction_UseGamma)
    {
        const void *keys[4] = { kCVImageBufferColorPrimariesKey, kCVImageBufferTransferFunctionKey, kCVImageBufferYCbCrMatrixKey, kCVImageBufferGammaLevelKey };
        const void *values[4] = { primariesValue, transferValue, matrixValue, gammaValue };
        CFDictionaryRef attachments = CFDictionaryCreate(NULL, keys, values, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

        colorSpace = CVImageBufferCreateColorSpaceFromAttachments(attachments);
        CFRelease(attachments);
    }
    else
    {
        const void *keys[3] = { kCVImageBufferColorPrimariesKey, kCVImageBufferTransferFunctionKey, kCVImageBufferYCbCrMatrixKey };
        const void *values[3] = { primariesValue, transferValue, matrixValue };
        CFDictionaryRef attachments = CFDictionaryCreate(NULL, keys, values, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

        colorSpace = CVImageBufferCreateColorSpaceFromAttachments(attachments);
        CFRelease(attachments);
    }

    return colorSpace;
}
