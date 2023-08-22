/* cropscale_vt.c

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "vt_common.h"

#include <VideoToolbox/VideoToolbox.h>

struct hb_filter_private_s
{
    VTPixelTransferSessionRef session;
    CFDictionaryRef           source_clean_aperture;
    CVPixelBufferPoolRef      pool;

    hb_filter_init_t          input;
    hb_filter_init_t          output;
};

static int crop_scale_vt_init(hb_filter_object_t *filter,
                              hb_filter_init_t   *init);

static int crop_scale_vt_work(hb_filter_object_t *filter,
                              hb_buffer_t **buf_in,
                              hb_buffer_t **buf_out);

static void crop_scale_vt_close(hb_filter_object_t *filter);

static const char crop_scale_vt_template[] =
    "width=^"HB_INT_REG"$:height=^"HB_INT_REG"$:"
    "crop-top=^"HB_INT_REG"$:crop-bottom=^"HB_INT_REG"$:"
    "crop-left=^"HB_INT_REG"$:crop-right=^"HB_INT_REG"$";


hb_filter_object_t hb_filter_crop_scale_vt =
{
    .id                = HB_FILTER_CROP_SCALE_VT,
    .enforce_order     = 1,
    .name              = "Crop and Scale (VideoToolbox)",
    .settings          = NULL,
    .init              = crop_scale_vt_init,
    .work              = crop_scale_vt_work,
    .close             = crop_scale_vt_close,
    .settings_template = crop_scale_vt_template,
};

static int crop_scale_vt_init(hb_filter_object_t *filter,
                              hb_filter_init_t   *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("crop scale videotoolbox: calloc failed");
        return -1;
    }
    hb_filter_private_t *pv = filter->private_data;

    pv->input = *init;

    hb_dict_t         *settings = filter->settings;
    int                width, height;
    int                top = 0, bottom = 0, left = 0, right = 0;
    int                crop_width, crop_height;
    int                crop_offset_left, crop_offset_top;

    // Convert crop settings to 'crop'
    hb_dict_extract_int(&top, settings, "crop-top");
    hb_dict_extract_int(&bottom, settings, "crop-bottom");
    hb_dict_extract_int(&left, settings, "crop-left");
    hb_dict_extract_int(&right, settings, "crop-right");

    crop_width       = init->geometry.width - left - right;
    crop_height      = init->geometry.height - top - bottom;
    crop_offset_left = left / 2 - right / 2;
    crop_offset_top  = top / 2 - bottom / 2;

    // Set up the source clean aperture dictionary
    // VTPixelTransferSessionRef will use it to crop the source buffer
    // before resizing it to fit the destination buffer

    CFNumberRef crop_width_num       = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &crop_width);
    CFNumberRef crop_height_num      = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &crop_height);
    CFNumberRef crop_offset_left_num = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &crop_offset_left);
    CFNumberRef crop_offset_top_num  = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &crop_offset_top);

    const void *clean_aperture_keys[4] =
    {
        kCVImageBufferCleanApertureWidthKey,
        kCVImageBufferCleanApertureHeightKey,
        kCVImageBufferCleanApertureHorizontalOffsetKey,
        kCVImageBufferCleanApertureVerticalOffsetKey
    };
    const void *source_clean_aperture_values[4] =
    {
        crop_width_num,
        crop_height_num,
        crop_offset_left_num,
        crop_offset_top_num
    };

    pv->source_clean_aperture = CFDictionaryCreate(kCFAllocatorDefault,
                                                   clean_aperture_keys,
                                                   source_clean_aperture_values,
                                                   4,
                                                   &kCFTypeDictionaryKeyCallBacks,
                                                   &kCFTypeDictionaryValueCallBacks);

    CFRelease(crop_width_num);
    CFRelease(crop_height_num);
    CFRelease(crop_offset_left_num);
    CFRelease(crop_offset_top_num);

    // Sessions initialization
    OSStatus err = noErr;
    err = VTPixelTransferSessionCreate(kCFAllocatorDefault, &pv->session);

    if (err != noErr)
    {
        hb_log("cropscale_vt: err=%"PRId64"", (int64_t)err);
        return err;
    }

    err = VTSessionSetProperty(pv->session,
                               kVTPixelTransferPropertyKey_ScalingMode,
                               kVTScalingMode_CropSourceToCleanAperture);

    if (err != noErr)
    {
        hb_log("cropscale_vt: kVTPixelTransferPropertyKey_ScalingMode failed");
    }

    // Set yo the destination clean aperture dictionary
    hb_dict_extract_int(&width, settings, "width");
    hb_dict_extract_int(&height, settings, "height");

    int zero = 0;

    CFNumberRef width_num       = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &width);
    CFNumberRef height_num      = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &height);
    CFNumberRef offset_left_num = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &zero);
    CFNumberRef offset_top_num  = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &zero);

    const void *destination_clean_aperture_values[4] =
    {
        width_num,
        height_num,
        offset_left_num,
        offset_top_num
    };

    CFDictionaryRef destination_clean_aperture = CFDictionaryCreate(kCFAllocatorDefault,
                                                                    clean_aperture_keys,
                                                                    destination_clean_aperture_values,
                                                                    4,
                                                                    &kCFTypeDictionaryKeyCallBacks,
                                                                    &kCFTypeDictionaryValueCallBacks);

    CFRelease(offset_left_num);
    CFRelease(offset_top_num);

    err = VTSessionSetProperty(pv->session,
                               kVTPixelTransferPropertyKey_DestinationCleanAperture,
                               destination_clean_aperture);

    CFRelease(destination_clean_aperture);

    if (err != noErr)
    {
        hb_log("cropscale_vt: kVTPixelTransferPropertyKey_DestinationCleanAperture failed");
        return err;
    }

    // CVPixelBuffer pool
    // Set the Metal compatibility key
    // to keep the buffer on the GPU memory
    OSType cv_pix_fmt = hb_vt_get_cv_pixel_format(init->pix_fmt, init->color_range);
    CFNumberRef pix_fmt_num = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &cv_pix_fmt);

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

    err = CVPixelBufferPoolCreate(kCFAllocatorDefault, NULL, attrs, &pv->pool);
    CFRelease(attrs);
    if (err != noErr)
    {
        hb_log("cropscale_vt: CVPixelBufferPoolCreate failed");
        return err;
    }

    init->crop[0] = top;
    init->crop[1] = bottom;
    init->crop[2] = left;
    init->crop[3] = right;
    hb_limit_rational(&init->geometry.par.num, &init->geometry.par.den,
        (int64_t)init->geometry.par.num * height * crop_width,
        (int64_t)init->geometry.par.den * width  * crop_height, 65535);
    init->geometry.width = width;
    init->geometry.height = height;

    pv->output = *init;

    return 0;
}

static void crop_scale_vt_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    VTPixelTransferSessionInvalidate(pv->session);
    CFRelease(pv->session);
    CFRelease(pv->source_clean_aperture);
    CVPixelBufferPoolRelease(pv->pool);

    free(pv);
    filter->private_data = NULL;
}

static CVPixelBufferRef extract_buf(hb_buffer_t *in)
{
    if (in->storage_type == AVFRAME)
    {
        return (CVPixelBufferRef)((AVFrame *)in->storage)->data[3];
    }
    else if (in->storage_type == COREMEDIA)
    {
        return (CVPixelBufferRef)in->storage;
    }
    return nil;
}

static int crop_scale_vt_work(hb_filter_object_t *filter,
                             hb_buffer_t **buf_in,
                             hb_buffer_t **buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in, *out;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    // Setup buffers
    OSStatus err = noErr;

    CVPixelBufferRef source_buf = extract_buf(in);
    if (source_buf == NULL)
    {
        hb_log("cropscale_vt: extract_buf failed");
        return HB_FILTER_FAILED;
    }
    CVBufferSetAttachment(source_buf, kCVImageBufferCleanApertureKey,
                          pv->source_clean_aperture, kCVAttachmentMode_ShouldPropagate);

    CVPixelBufferRef dest_buf = NULL;
    err = CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pv->pool, &dest_buf);
    if (err != noErr)
    {
        hb_log("cropscale_vt: CVPixelBufferPoolCreatePixelBuffer failed");
        return HB_FILTER_FAILED;
    }

    // Do work
    err = VTPixelTransferSessionTransferImage(pv->session, source_buf, dest_buf);
    if (err != noErr)
    {
        hb_log("cropscale_vt: VTPixelTransferSessionTransferImage failed");
        return HB_FILTER_FAILED;
    }

    out = hb_buffer_wrapper_init();
    out->storage_type      = COREMEDIA;
    out->storage           = dest_buf;
    out->f.width           = pv->output.geometry.width;
    out->f.height          = pv->output.geometry.height;
    out->f.fmt             = pv->output.pix_fmt;
    out->f.color_prim      = pv->output.color_prim;
    out->f.color_transfer  = pv->output.color_transfer;
    out->f.color_matrix    = pv->output.color_matrix;
    out->f.color_range     = pv->output.color_range;
    out->f.chroma_location = pv->output.chroma_location;
    hb_buffer_copy_props(out, in);

    *buf_out = out;

    return HB_FILTER_OK;
}
