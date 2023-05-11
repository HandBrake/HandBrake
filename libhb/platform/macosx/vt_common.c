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
