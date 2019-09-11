/*  HBColorUtilities.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBImageUtilities.h"
#import <Cocoa/Cocoa.h>
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

static CGColorSpaceRef copyColorSpaceOld(int colorPrimaries)
{
    const CGFloat whitePoint[] = {0.95047, 1.0, 1.08883};
    const CGFloat blackPoint[] = {0, 0, 0};

    // See https://developer.apple.com/library/content/technotes/tn2257/_index.html
    const CGFloat gamma[] = {1.961, 1.961, 1.961};

    // RGB/XYZ Matrices (D65 white point)
    switch (colorPrimaries) {
        case HB_COLR_PRI_EBUTECH:
        {
            // Rec. 601, 625 line
            const CGFloat matrix[] = {0.4305538, 0.2220043, 0.0201822,
                0.3415498, 0.7066548, 0.1295534,
                0.1783523, 0.0713409, 0.9393222};
            return CGColorSpaceCreateCalibratedRGB(whitePoint, blackPoint, gamma, matrix);
        }
        case HB_COLR_PRI_SMPTEC:
        {
            // Rec. 601, 525 line
            const CGFloat matrix[] = {0.3935209, 0.2123764, 0.0187391,
                0.3652581, 0.7010599, 0.1119339,
                0.1916769, 0.0865638, 0.9583847};
            return CGColorSpaceCreateCalibratedRGB(whitePoint, blackPoint, gamma, matrix);
        }
        case HB_COLR_PRI_BT2020:
        {
            // Rec. 2020
            const CGFloat matrix[] = {0.6369580, 0.2627002, 0.0000000,
                0.1446169, 0.6779981, 0.0280727,
                0.1688810, 0.0593017, 1.0609851};
            return CGColorSpaceCreateCalibratedRGB(whitePoint, blackPoint, gamma, matrix);
        }
        case HB_COLR_PRI_BT709:
        default:
        {
            // Rec. 709
            const CGFloat matrix[] = {0.4124564, 0.2126729, 0.0193339,
                0.3575761, 0.7151522, 0.1191920,
                0.1804375, 0.0721750, 0.9503041};
            return CGColorSpaceCreateCalibratedRGB(whitePoint, blackPoint, gamma, matrix);
        }
    }
}

CGColorSpaceRef copyColorSpace(int primaries, int transfer, int matrix)
{
    if (NSAppKitVersionNumber < NSAppKitVersionNumber10_11)
    {
        return copyColorSpaceOld(primaries);
    }

    CFStringRef primariesKey = NULL;
    switch (primaries)
    {
        case HB_COLR_PRI_EBUTECH:
            primariesKey = kCVImageBufferColorPrimaries_EBU_3213;
            break;

        case HB_COLR_PRI_SMPTEC:
            primariesKey = kCVImageBufferColorPrimaries_SMPTE_C;
            break;

        case HB_COLR_PRI_BT2020:
            primariesKey = kCVImageBufferColorPrimaries_ITU_R_2020;
            break;

        case HB_COLR_PRI_BT709:
        default:
            primariesKey = kCVImageBufferColorPrimaries_ITU_R_709_2;
    }

    CFStringRef transferKey = NULL;
    switch (transfer)
    {
        case HB_COLR_TRA_SMPTE240M:
            transferKey = kCVImageBufferTransferFunction_SMPTE_240M_1995;
            break;

        case HB_COLR_TRA_BT2020_10:
        case HB_COLR_TRA_BT2020_12:
            transferKey = kCVImageBufferTransferFunction_ITU_R_2020;
            break;

        case HB_COLR_TRA_SMPTEST2084:
            transferKey = CFSTR("SMPTE_ST_2084_PQ"); //kCVImageBufferTransferFunction_SMPTE_ST_2084_PQ;
            break;

        case HB_COLR_TRA_ARIB_STD_B67:
            transferKey = CFSTR("ITU_R_2100_HLG"); //kCVImageBufferTransferFunction_ITU_R_2100_HLG;
            break;

        case HB_COLR_TRA_BT709:
        default:
            transferKey = kCVImageBufferTransferFunction_ITU_R_709_2;
    }

    CFStringRef matrixKey = NULL;
    switch (matrix)
    {
        case HB_COLR_MAT_SMPTE170M:
            matrixKey = kCVImageBufferYCbCrMatrix_ITU_R_601_4;
            break;

        case HB_COLR_MAT_SMPTE240M:
            matrixKey = kCVImageBufferYCbCrMatrix_SMPTE_240M_1995;
            break;

        case HB_COLR_MAT_BT2020_NCL:
        case HB_COLR_MAT_BT2020_CL:
            matrixKey = kCVImageBufferYCbCrMatrix_ITU_R_2020;
            break;

        case HB_COLR_MAT_BT709:
        default:
            matrixKey = kCVImageBufferYCbCrMatrix_ITU_R_709_2;;
    }

    const void *keys[3] = { kCVImageBufferColorPrimariesKey, kCVImageBufferTransferFunctionKey, kCVImageBufferYCbCrMatrixKey };
    const void *values[3] = { primariesKey, transferKey, matrixKey};
    CFDictionaryRef attachments = CFDictionaryCreate(NULL, keys, values, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CGColorSpaceRef colorSpace = CVImageBufferCreateColorSpaceFromAttachments(attachments);
    CFRelease(attachments);

    return colorSpace;
}
