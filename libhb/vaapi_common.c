/* vaapi_common.c
 *
 * Copyright (c) 2003-2020 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/vaapi_common.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/handbrake.h"

#if HB_PROJECT_FEATURE_VAAPI
static AVBufferRef *vaapi_device_ctx0 = NULL;
#endif

void hb_vaapi_init()
{
#if HB_PROJECT_FEATURE_VAAPI
    {
        int err;
        if( (err = av_hwdevice_ctx_create(&vaapi_device_ctx0, AV_HWDEVICE_TYPE_VAAPI,
                                          NULL, NULL, 0)) < 0 ) {
            hb_log("Failed to create a VAAPI device. Error code: %s %p\n", av_err2str(err), vaapi_device_ctx0);
            vaapi_device_ctx0 = NULL;
        }
    }
#endif
}

void hb_vaapi_free()
{
#if HB_PROJECT_FEATURE_VAAPI
    if( NULL != vaapi_device_ctx0 ) {
        av_buffer_unref(&vaapi_device_ctx0);
        vaapi_device_ctx0 = NULL;
    }
#endif
}

int hb_vaapi_available()
{
    #if HB_PROJECT_FEATURE_VAAPI
        if (hb_is_hardware_disabled())
        {
            hb_log("hb_vaapi_available: hardware disabled");
            return 0;
        }
        if( NULL == vaapi_device_ctx0 ) {
            hb_log("hb_vaapi_available: device ctx null");
            return 0;
        }
        return 1;
    #else
        return 0;
    #endif
}

int hb_vaapi_encoder_available(int encoder)
{
    #if HB_PROJECT_FEATURE_VAAPI
        return hb_vaapi_available() &&
               hb_avcodec_test_encoder_available(encoder);
    #else
        return 0;
    #endif
}

int hb_vaapi_avcodec_set_hwframe_ctx(AVCodecContext *ctx, int hw_device_ctxidx, int init_pool_sz)
{
#if HB_PROJECT_FEATURE_VAAPI
   // AV_PIX_FMT_NV12    planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
   // NV12 0x3231564E  12  8-bit Y plane followed by an interleaved U/V plane with 2x2 subsampling
   // NV21 0x3132564E  12  As NV12 with U and V reversed in the interleaved plane
   // AV_PIX_FMT_YUV420P planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    AVBufferRef *hw_device_ctx = vaapi_device_ctx0;
    AVBufferRef *hw_frames_ref;
    AVHWFramesContext *frames_ctx = NULL;
    int err = 0;
    if( NULL == hw_device_ctx )
    {
        hb_log("vaapi: Device is null.");
        return -1;
    }
    if (!(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx)))
    {
        hb_log("vaapi: Failed to create frame context.");
        return -2;
    }
    frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
    frames_ctx->format    = AV_PIX_FMT_VAAPI;
    frames_ctx->sw_format = AV_PIX_FMT_NV12; // AV_PIX_FMT_NV12; // AV_PIX_FMT_YUV420P
    frames_ctx->width     = ctx->width;
    frames_ctx->height    = ctx->height;
    frames_ctx->initial_pool_size = init_pool_sz;
    if ((err = av_hwframe_ctx_init(hw_frames_ref)) < 0)
    {
        hb_log("vaapi: Failed to initialize frame context."
                "Error code: %s",av_err2str(err));
        av_buffer_unref(&hw_frames_ref);
        return err;
    }
    ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if (!ctx->hw_frames_ctx)
    {
        err = AVERROR(ENOMEM);
    }
    av_buffer_unref(&hw_frames_ref);
    return err;
#else
    return -1;
#endif
}

int hb_vaapi_avcodec_send_frame(AVCodecContext *context, AVFrame *frame, AVFrame **hw_frame)
{
    int ret = -1;

    // Convert incompatible source frame (e.g. AV_PIX_FMT_YUV420P) to AV_PIX_FMT_VAAPI/NV12
    if (!(*hw_frame = av_frame_alloc()))
    {
       hb_error("vaapi: Encode: Couldn't allocate hw_frame");
       return ret;
    }
    if ((ret = av_hwframe_get_buffer(context->hw_frames_ctx, *hw_frame, 0)) < 0)
    {
       hb_error("vaapi: Encode: Error while allocating hw_frame data buffers: %s", av_err2str(ret));
       return ret;
    }
    (*hw_frame)->pts = frame->pts;
    if ((ret = av_hwframe_transfer_data(*hw_frame, frame, 0)) < 0)
    {
       hb_error("vaapi: Encode: Error while transferring frame data to hw_frame: %s.", av_err2str(ret));
       return ret;
    }
    return avcodec_send_frame(context, *hw_frame);
}
