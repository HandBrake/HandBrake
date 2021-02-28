/* vt_common.c

   Copyright (c) 2003-2021 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "vt_common.h"

#include <VideoToolbox/VideoToolbox.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CoreVideo.h>

static const CFStringRef encoder_id_h264 = CFSTR("com.apple.videotoolbox.videoencoder.h264.gva");
static const CFStringRef encoder_id_h265 = CFSTR("com.apple.videotoolbox.videoencoder.hevc.gva");

static int is_encoder_available(CFStringRef encoder)
{
    CFArrayRef encoder_list;
    VTCopyVideoEncoderList(NULL, &encoder_list);
    CFIndex size = CFArrayGetCount(encoder_list);

    for (CFIndex i = 0; i < size; i++ )
    {
        CFDictionaryRef encoder_dict = CFArrayGetValueAtIndex(encoder_list, i);
        CFStringRef encoder_id = CFDictionaryGetValue(encoder_dict, kVTVideoEncoderSpecification_EncoderID);
        if (CFEqual(encoder_id, encoder))
        {
            CFRelease(encoder_list);
            return 1;
        }
    }
    CFRelease(encoder_list);
    return 0;
}

static OSStatus encoder_properties(CMVideoCodecType codecType, CFStringRef *encoderIDOut, CFDictionaryRef *supportedPropertiesOut) API_AVAILABLE(macosx(10.13), ios(11.0), tvos(11.0))
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
#if defined(__MAC_11_0)
    if (@available (macOS 11, *))
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
    else
    {
#endif
        CFStringRef encoder_id;

        switch (codecType) {
            case kCMVideoCodecType_H264:
                encoder_id = encoder_id_h264;
                break;
            case kCMVideoCodecType_HEVC:
                encoder_id = encoder_id_h265;
                break;
            default:
                return 0;
        }

        return is_encoder_available(encoder_id);
#if defined(__MAC_11_0)
    }
#endif
}

static int is_constant_quality_available(CMVideoCodecType codecType)
{
#if defined(__MAC_11_0) && defined(__aarch64__)
    if (@available (macOS 11, *))
    {
        CFStringRef encoderIDOut;
        CFDictionaryRef supportedPropertiesOut;

        OSStatus err = encoder_properties(codecType, &encoderIDOut, &supportedPropertiesOut);

        if (err == noErr) {
            Boolean keyExists;
            CFBooleanRef value = kCFBooleanFalse;
            keyExists = CFDictionaryGetValueIfPresent(supportedPropertiesOut,
                                                      kVTCompressionPropertyKey_Quality,
                                                      (const void **)&value);

            CFRelease(encoderIDOut);
            CFRelease(supportedPropertiesOut);

            if (keyExists) {
                return 1;
            }
        }
    }
#endif
    return 0;
}

static int vt_h264_available;

int hb_vt_h264_is_available()
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        vt_h264_available = is_hardware_encoder_available(kCMVideoCodecType_H264, NULL);
    });
    return vt_h264_available;
}

static int vt_h265_available;

int hb_vt_h265_is_available()
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        vt_h265_available = is_hardware_encoder_available(kCMVideoCodecType_HEVC, NULL);
    });
    return vt_h265_available;
}

static int vt_h265_10bit_available;

int hb_vt_h265_10bit_is_available()
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        if (@available (macOS 11, *))
        {
            vt_h265_10bit_available = is_hardware_encoder_available(kCMVideoCodecType_HEVC, kVTProfileLevel_HEVC_Main10_AutoLevel);
        }
        else
        {
            vt_h265_10bit_available = 0;
        }
    });
    return vt_h265_10bit_available;
}

static int vt_h264_constant_quality;

int hb_vt_h264_is_constant_quality_available()
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        vt_h264_constant_quality = is_constant_quality_available(kCMVideoCodecType_H264);
    });
    return vt_h264_constant_quality;
}

static int vt_h265_constant_quality;

int hb_vt_h265_is_constant_quality_available()
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        vt_h265_constant_quality = is_constant_quality_available(kCMVideoCodecType_HEVC);
    });
    return vt_h265_constant_quality;
}
