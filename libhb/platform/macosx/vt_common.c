/* vt_common.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "vt_common.h"
#include "cv_utils.h"
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
    AV_PIX_FMT_P410, AV_PIX_FMT_NV24, AV_PIX_FMT_P210, AV_PIX_FMT_NV16, AV_PIX_FMT_P010, AV_PIX_FMT_YUV420P, AV_PIX_FMT_NV12, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat vt_h265_10bit_pix_fmts[] =
{
    AV_PIX_FMT_P410, AV_PIX_FMT_NV24, AV_PIX_FMT_P210, AV_PIX_FMT_NV16, AV_PIX_FMT_P010, AV_PIX_FMT_NV12, AV_PIX_FMT_NONE
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

int hb_vt_get_best_pix_fmt(int encoder, const char *profile)
{
    switch (encoder)
    {
        case HB_VCODEC_VT_H264:
        case HB_VCODEC_VT_H265:
            return AV_PIX_FMT_NV12;
        case HB_VCODEC_VT_H265_10BIT:
            if (!strcasecmp(profile, "main422-10"))
            {
                return AV_PIX_FMT_P210;
            }
            else
            {
                return AV_PIX_FMT_P010;
            }
    }
    return AV_PIX_FMT_NV12;
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

hb_buffer_t * hb_vt_buffer_dup(const hb_buffer_t *src)
{
    CVPixelBufferRef pix_buf = hb_cv_get_pixel_buffer(src);

    if (pix_buf == NULL)
    {
        return NULL;
    }

    CFRetain(pix_buf);

    hb_buffer_t *out  = hb_buffer_wrapper_init();
    out->storage_type = COREMEDIA;
    out->storage      = pix_buf;
    out->f            = src->f;
    hb_buffer_copy_props(out, src);

    return out;
}

hb_buffer_t * copy_video_buffer_to_hw_video_buffer(const hb_job_t *job, hb_buffer_t **in)
{
    hb_buffer_t *buf = *in;

    OSType cv_pix_fmt = hb_cv_get_pixel_format(buf->f.fmt, buf->f.color_range);
    CFNumberRef pix_fmt_num = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &cv_pix_fmt);
    CFNumberRef width_num   = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &buf->f.width);
    CFNumberRef height_num  = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &buf->f.height);

    const void *attrs_keys[4] =
    {
        kCVPixelBufferWidthKey, kCVPixelBufferHeightKey,
        kCVPixelBufferPixelFormatTypeKey, kCVPixelBufferMetalCompatibilityKey
    };
    const void *attrs_values[4] =
    {
        width_num, height_num, pix_fmt_num, kCFBooleanTrue
    };

    CFDictionaryRef attrs = CFDictionaryCreate(kCFAllocatorDefault,
                                               attrs_keys, attrs_values, 4,
                                               &kCFTypeDictionaryKeyCallBacks,
                                               &kCFTypeDictionaryValueCallBacks);

    CFRelease(width_num);
    CFRelease(height_num);
    CFRelease(pix_fmt_num);

    CVPixelBufferRef pix_buf = NULL;
    CVReturn ret = CVPixelBufferCreate(kCFAllocatorDefault,
                                       buf->f.width, buf->f.height,
                                       cv_pix_fmt, attrs,
                                       &pix_buf);
    CFRelease(attrs);

    if (ret != kCVReturnSuccess)
    {
        hb_buffer_close(&buf);
        return NULL;
    }

    CVPixelBufferLockBaseAddress(pix_buf, 0);

    for (int pp = 0; pp <= buf->f.max_plane; pp++)
    {
        void *dst         = CVPixelBufferGetBaseAddressOfPlane(pix_buf, pp);
        size_t dst_stride = CVPixelBufferGetBytesPerRowOfPlane(pix_buf, pp);

        void *src         = buf->plane[pp].data;
        size_t src_height = buf->plane[pp].height;
        size_t src_stride = buf->plane[pp].stride;

        size_t stride = MIN(dst_stride, src_stride);

        for (int y = 0; y < src_height; y++)
        {
            memcpy(dst, src, stride);
            src += src_stride;
            dst += dst_stride;
        }
    }

    CVPixelBufferUnlockBaseAddress(pix_buf, 0);

    hb_buffer_t *out  = hb_buffer_wrapper_init();
    out->storage_type = COREMEDIA;
    out->storage      = pix_buf;
    out->f            = buf->f;
    hb_buffer_copy_props(out, buf);

    hb_buffer_close(&buf);

    return out;
}

static int are_filters_supported(hb_list_t *filters)
{
    int ret = 1;

    for (int i = 0; i < hb_list_count(filters); i++)
    {
        int supported = 1;
        hb_filter_object_t *filter = hb_list_item(filters, i);

        switch (filter->id)
        {
            case HB_FILTER_DETELECINE:
            case HB_FILTER_DECOMB:
            case HB_FILTER_DEBLOCK:
            case HB_FILTER_DENOISE:
            case HB_FILTER_NLMEANS:
            case HB_FILTER_COLORSPACE:
            case HB_FILTER_FORMAT:
                supported = 0;
                break;
            default:
                break;
        }

        if (supported == 0)
        {
            hb_deep_log(2, "videotoolbox: %s isn't yet supported for hw video frames", filter->name);
            ret = 0;
        }
    }

    return ret;
}

static void fix_prores_pix_fmt(hb_job_t *job)
{
    // TODO: Find a better way
    // VideoToolbox ProRes decoder uses an higher bitdepth
    // than FFmpeg software decoder. We get only the pixel format
    // from the software decoder in decavcodec.c, so set
    // a better one here
    if (job->title->video_codec_param == AV_CODEC_ID_PRORES)
    {
        switch (job->title->video_codec_profile)
        {
            case AV_PROFILE_PRORES_XQ:
            case AV_PROFILE_PRORES_4444:
                job->input_pix_fmt = AV_PIX_FMT_P416;
                break;
            default:
                break;
        }
    }
}

static void replace_filter(hb_job_t *job, int prev_filter_id, int new_filter_id)
{
    hb_list_t *list = job->list_filter;
    hb_filter_object_t *filter = hb_filter_find(list, prev_filter_id);

    if (filter != NULL)
    {
        hb_dict_t *settings = filter->settings;
        if (settings != NULL)
        {
            hb_list_rem(list, filter);
            hb_filter_object_t *new_filter = hb_filter_init(new_filter_id);
            hb_add_filter_dict(job, new_filter, settings);
            hb_filter_close(&filter);
        }
    }
}

void hb_vt_setup_hw_filters(hb_job_t *job)
{
    if (job->hw_pix_fmt == AV_PIX_FMT_VIDEOTOOLBOX)
    {
        fix_prores_pix_fmt(job);

        // Add adapter
        hb_filter_object_t *filter = hb_filter_init(HB_FILTER_PRE_VT);
        char *settings = hb_strdup_printf("rotation=%d", job->title->rotation);
        hb_add_filter(job, filter, settings);
        free(settings);

        replace_filter(job, HB_FILTER_COMB_DETECT, HB_FILTER_COMB_DETECT_VT);
        replace_filter(job, HB_FILTER_YADIF, HB_FILTER_YADIF_VT);
        replace_filter(job, HB_FILTER_BWDIF, HB_FILTER_BWDIF_VT);
        replace_filter(job, HB_FILTER_CROP_SCALE, HB_FILTER_CROP_SCALE_VT);
        replace_filter(job, HB_FILTER_CHROMA_SMOOTH, HB_FILTER_CHROMA_SMOOTH_VT);
        replace_filter(job, HB_FILTER_ROTATE, HB_FILTER_ROTATE_VT);
        replace_filter(job, HB_FILTER_PAD, HB_FILTER_PAD_VT);
        replace_filter(job, HB_FILTER_GRAYSCALE, HB_FILTER_GRAYSCALE_VT);
        replace_filter(job, HB_FILTER_LAPSHARP, HB_FILTER_LAPSHARP_VT);
        replace_filter(job, HB_FILTER_UNSHARP, HB_FILTER_UNSHARP_VT);

        int count = hb_list_count(job->list_filter);
        if (count)
        {
            // Avoid an additional VTPixelTransferSession, when possible
            // do the scale and pixel format conversion in one pass
            hb_filter_object_t *last = hb_list_item(job->list_filter, count - 1);
            if (last->id == HB_FILTER_CROP_SCALE_VT)
            {
                int pix_fmt = hb_vt_get_best_pix_fmt(job->vcodec, job->encoder_profile);
                hb_dict_set(last->settings, "format", hb_value_int(pix_fmt));
            }
        }
    }
}

static const int vt_encoders[] =
{
    HB_VCODEC_VT_H264,
    HB_VCODEC_VT_H265,
    HB_VCODEC_VT_H265_10BIT,
    HB_VCODEC_INVALID
};

hb_hwaccel_t hb_hwaccel_videotoolbox =
{
    .id         = HB_DECODE_VIDEOTOOLBOX,
    .name       = "videotoolbox hwaccel",
    .encoders   = vt_encoders,
    .type       = AV_HWDEVICE_TYPE_VIDEOTOOLBOX,
    .hw_pix_fmt = AV_PIX_FMT_VIDEOTOOLBOX,
    .can_filter = are_filters_supported,
    .upload     = copy_video_buffer_to_hw_video_buffer,
    .caps       = HB_HWACCEL_CAP_SCAN
};
