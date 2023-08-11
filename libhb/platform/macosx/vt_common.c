/* vt_common.c

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "vt_common.h"
#include "handbrake/hbffmpeg.h"

#include <VideoToolbox/VideoToolbox.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CoreVideo.h>

#pragma mark - Availability

static OSStatus encoder_properties(CMVideoCodecType codecType, CFStringRef *encoderIDOut, CFDictionaryRef *supportedPropertiesOut)
{
    const void *keys[1] = { kVTVideoEncoderSpecification_RequireHardwareAcceleratedVideoEncoder };
    const void *values[1] = { kCFBooleanTrue };
    CFDictionaryRef specification = CFDictionaryCreate(NULL, keys, values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    OSStatus err = VTCopySupportedPropertyDictionaryForEncoder(1920, 1080,
                                                               codecType, specification,
                                                               encoderIDOut,
                                                               supportedPropertiesOut);
    CFRelease(specification);
    return err;
}

static int is_hardware_encoder_available(CMVideoCodecType codecType, CFStringRef level)
{
    Boolean found = false;
    CFStringRef encoderIDOut;
    CFDictionaryRef supportedPropertiesOut;

    OSStatus err = encoder_properties(codecType, &encoderIDOut, &supportedPropertiesOut);

    if (err == noErr)
    {
        if (level != NULL)
        {
            CFDictionaryRef profiles = CFDictionaryGetValue(supportedPropertiesOut, kVTCompressionPropertyKey_ProfileLevel);
            if (profiles != NULL)
            {
                CFArrayRef listOfValues = CFDictionaryGetValue(profiles, kVTPropertySupportedValueListKey);
                if (listOfValues != NULL)
                {
                    found = CFArrayContainsValue(listOfValues, CFRangeMake(0, CFArrayGetCount(listOfValues)), level);
                }
            }
        }
        else
        {
            found = true;
        }

        CFRelease(encoderIDOut);
        CFRelease(supportedPropertiesOut);
    }
    return found;
}

static int vt_h264_available;
static int vt_h265_available;
static int vt_h265_10bit_available;
static int vt_h265_422_10bit_available;

int hb_vt_is_encoder_available(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_VT_H264:
        {
            static dispatch_once_t onceToken;
            dispatch_once(&onceToken, ^{
                vt_h264_available = is_hardware_encoder_available(kCMVideoCodecType_H264, NULL);
            });
            return vt_h264_available;
        }
        case HB_VCODEC_VT_H265:
        {
            static dispatch_once_t onceToken;
            dispatch_once(&onceToken, ^{
                vt_h265_available = is_hardware_encoder_available(kCMVideoCodecType_HEVC, NULL);
            });
            return vt_h265_available;
        }
        case HB_VCODEC_VT_H265_10BIT:
        {
            static dispatch_once_t onceToken;
            dispatch_once(&onceToken, ^{
                if (__builtin_available (macOS 11, *))
                {
                    vt_h265_10bit_available = is_hardware_encoder_available(kCMVideoCodecType_HEVC, kVTProfileLevel_HEVC_Main10_AutoLevel);
                    vt_h265_422_10bit_available = is_hardware_encoder_available(kCMVideoCodecType_HEVC, CFSTR("HEVC_Main42210_AutoLevel"));
                }
                else
                {
                    vt_h265_10bit_available = 0;
                }
            });
            return vt_h265_10bit_available;
        }
    }
    return 0;
}

#pragma mark - Constant Quality

static int is_constant_quality_available(CMVideoCodecType codecType)
{
#if defined(__aarch64__)
    if (__builtin_available (macOS 11, *))
    {
        CFStringRef encoderIDOut;
        CFDictionaryRef supportedPropertiesOut;

        OSStatus err = encoder_properties(codecType, &encoderIDOut, &supportedPropertiesOut);

        if (err == noErr)
        {
            Boolean keyExists;
            CFBooleanRef value = kCFBooleanFalse;
            keyExists = CFDictionaryGetValueIfPresent(supportedPropertiesOut,
                                                      kVTCompressionPropertyKey_Quality,
                                                      (const void **)&value);

            CFRelease(encoderIDOut);
            CFRelease(supportedPropertiesOut);

            if (keyExists)
            {
                return 1;
            }
        }
    }
#endif
    return 0;
}

static int vt_h264_constant_quality;
static int vt_h265_constant_quality;

int hb_vt_is_constant_quality_available(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_VT_H264:
        {
            static dispatch_once_t onceToken;
            dispatch_once(&onceToken, ^{
                vt_h264_constant_quality = is_constant_quality_available(kCMVideoCodecType_H264);
            });
            return vt_h264_constant_quality;

        }
        case HB_VCODEC_VT_H265:
        case HB_VCODEC_VT_H265_10BIT:
        {
            static dispatch_once_t onceToken;
            dispatch_once(&onceToken, ^{
                vt_h265_constant_quality = is_constant_quality_available(kCMVideoCodecType_HEVC);
            });
            return vt_h265_constant_quality;
        }
    }
    return 0;
}

int hb_vt_is_multipass_available(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_VT_H264:
        {
            return 1;
        }
        case HB_VCODEC_VT_H265:
        case HB_VCODEC_VT_H265_10BIT:
        {
#if defined(__aarch64__)
            if (__builtin_available (macOS 12, *))
            {
                return 1;
            }
#endif
            return 0;
        }
    }
    return 0;

}

#pragma mark - Settings

static const char * const vt_h26x_preset_name[] =
{
    "speed", "quality", NULL
};

static const char * const vt_h264_profile_name[] =
{
    "auto", "baseline", "main", "high", NULL
};

static const char * const vt_h265_profile_name[] =
{
    "auto", "main", NULL
};

static const char * const vt_h265_10_profile_name[] =
{
    "auto", "main10", NULL
};

static const char * const vt_h265_422_10_profile_name[] =
{
    "auto", "main10", "main422-10", NULL
};

static const char * vt_h264_level_names[] =
{
    "auto", "1.3", "3.0", "3.1", "3.2", "4.0", "4.1", "4.2", "5.0", "5.1", "5.2", NULL
};

static const char * const vt_h265_level_names[] =
{
    "auto",  NULL,
};

static const enum AVPixelFormat vt_h26x_pix_fmts[] =
{
    AV_PIX_FMT_VIDEOTOOLBOX, AV_PIX_FMT_YUV420P, AV_PIX_FMT_NV12, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat vt_h265_10bit_pix_fmts[] =
{
    AV_PIX_FMT_P210, AV_PIX_FMT_P010LE, AV_PIX_FMT_NONE
};

const int* hb_vt_get_pix_fmts(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_VT_H264:
        case HB_VCODEC_VT_H265:
            return vt_h26x_pix_fmts;
        case HB_VCODEC_VT_H265_10BIT:
            return vt_h265_10bit_pix_fmts;
    }
    return NULL;
}

const char* const* hb_vt_preset_get_names(int encoder)
{
    return vt_h26x_preset_name;
}

const char* const* hb_vt_profile_get_names(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_VT_H264:
            return vt_h264_profile_name;
        case HB_VCODEC_VT_H265:
            return vt_h265_profile_name;
        case HB_VCODEC_VT_H265_10BIT:
        {
            if (vt_h265_422_10bit_available)
            {
                return vt_h265_422_10_profile_name;
            }
            else
            {
                return vt_h265_10_profile_name;
            }
        }
    }
    return NULL;
}

const char* const* hb_vt_level_get_names(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_VT_H264:
            return vt_h264_level_names;
        case HB_VCODEC_VT_H265:
        case HB_VCODEC_VT_H265_10BIT:
            return vt_h265_level_names;
    }
    return NULL;
}

unsigned int hb_vt_get_cv_pixel_format(int pix_fmt, int color_range)
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
    else if (pix_fmt == AV_PIX_FMT_P010LE)
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
    else if (pix_fmt == AV_PIX_FMT_P416)
    {
        return kCVPixelFormatType_444YpCbCr16BiPlanarVideoRange;
    }
    else
    {
        return 0;
    }
}

int hb_vt_are_filters_supported(hb_list_t *filters)
{
    int ret = 1;

    for (int i = 0; i < hb_list_count(filters); i++)
    {
        int supported = 1;
        hb_filter_object_t *filter = hb_list_item(filters, i);

        switch (filter->id)
        {
            case HB_FILTER_CROP_SCALE:
            case HB_FILTER_CROP_SCALE_VT:
            case HB_FILTER_PRE_VT:
                break;
            case HB_FILTER_ROTATE:
            case HB_FILTER_ROTATE_VT:
            {
#ifdef MAC_OS_VERSION_13_0
                if (__builtin_available(macOS 13, *)) {}
                else { supported = 0; }
                break;
#else
                supported = 0;
                break;
#endif
            }
            case HB_FILTER_VFR:
                // Mode 0 doesn't require access to the frame data
                supported = hb_dict_get_int(filter->settings, "mode") == 0;
                break;
            default:
                supported = 0;
        }

        if (supported == 0)
        {
            hb_deep_log(2, "hwaccel: %s isn't yet supported for hw video frames", filter->name);
            ret = 0;
        }
    }

    return ret;
}

void hb_vt_setup_hw_filters(hb_job_t *job)
{
    if (job->hw_pix_fmt == AV_PIX_FMT_VIDEOTOOLBOX)
    {
        hb_list_t *list = job->list_filter;
        hb_filter_object_t *filter;

        filter = hb_filter_init(HB_FILTER_PRE_VT);
        hb_add_filter(job, filter, NULL);

        filter = hb_filter_find(list, HB_FILTER_CROP_SCALE);
        if (filter != NULL)
        {
            hb_dict_t *settings = filter->settings;
            if (settings != NULL)
            {
                int width, height, top, bottom, left, right;
                width = hb_dict_get_int(settings, "width");
                height = hb_dict_get_int(settings, "height");
                top = hb_dict_get_int(settings, "crop-top");
                bottom = hb_dict_get_int(settings, "crop-bottom");
                left = hb_dict_get_int(settings, "crop-left");
                right = hb_dict_get_int(settings, "crop-right");

                hb_list_rem(list, filter);
                hb_filter_close(&filter);

                filter = hb_filter_init(HB_FILTER_CROP_SCALE_VT);
                char *settings = hb_strdup_printf("width=%d:height=%d:crop-top=%d:crop-bottom=%d:crop-left=%d:crop-right=%d",
                                                  width, height, top, bottom, left, right);
                hb_add_filter(job, filter, settings);
                free(settings);
            }
        }

        filter = hb_filter_find(list, HB_FILTER_ROTATE);
        if (filter != NULL)
        {
            hb_dict_t *settings = filter->settings;
            if (settings != NULL)
            {
                int angle, hflip;
                angle = hb_dict_get_int(settings, "angle");
                hflip = hb_dict_get_int(settings, "hflip");

                hb_list_rem(list, filter);
                hb_filter_close(&filter);

                filter = hb_filter_init(HB_FILTER_ROTATE_VT);
                char *settings = hb_strdup_printf("angle=%d:hflip=%d",
                                                  angle, hflip);
                hb_add_filter(job, filter, settings);
                free(settings);
            }
        }
    }
}
