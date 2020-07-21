/* vaapi_common.c
 *
 * Copyright (c) 2003-2020 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/hbffmpeg.h"
#include "handbrake/handbrake.h"

#if HB_PROJECT_FEATURE_VAAPI
extern AVBufferRef *vaapi_device_ctx0;
#endif

int hb_check_vaapi_available(int encoder);

int hb_vaapi_h264_available()
{
    #if HB_PROJECT_FEATURE_VAAPI
        return hb_check_vaapi_available(HB_VCODEC_FFMPEG_VAAPI_H264) && 
               hb_avcodec_test_encoder_available(HB_VCODEC_FFMPEG_VAAPI_H264);
    #else
        return 0;
    #endif
}

int hb_vaapi_h265_available()
{
    #if HB_PROJECT_FEATURE_VAAPI
        return hb_check_vaapi_available(HB_VCODEC_FFMPEG_VAAPI_H265) && 
               hb_avcodec_test_encoder_available(HB_VCODEC_FFMPEG_VAAPI_H265);
    #else
        return 0;
    #endif
}

int hb_vaapi_vp8_available()
{
    #if HB_PROJECT_FEATURE_VAAPI
        return hb_check_vaapi_available(HB_VCODEC_FFMPEG_VAAPI_VP8) && 
               hb_avcodec_test_encoder_available(HB_VCODEC_FFMPEG_VAAPI_VP8);
    #else
        return 0;
    #endif
}

int hb_vaapi_vp9_available()
{
    #if HB_PROJECT_FEATURE_VAAPI
        return hb_check_vaapi_available(HB_VCODEC_FFMPEG_VAAPI_VP9) && 
               hb_avcodec_test_encoder_available(HB_VCODEC_FFMPEG_VAAPI_VP9);
    #else
        return 0;
    #endif
}


int hb_check_vaapi_available(int encoder)
{
    #if HB_PROJECT_FEATURE_VAAPI
        if (is_hardware_disabled())
        {
            hb_log("hb_check_vaapi_available encoder=0x%X: hardware disabled -> false", encoder);
            return 0;
        }
        return 1;
    #else
        return 0;
    #endif
}

int hb_avcodec_vaapi_set_hwframe_ctx(AVCodecContext *ctx, int hw_device_ctxidx, int init_pool_sz)
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
    if( NULL == hw_device_ctx ) {
        hb_log("VAAPI device is NULL.");
        return -1;
    }
    if (!(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx))) {
        hb_log("Failed to create VAAPI frame context.");
        return -2;
    }
    frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
    frames_ctx->format    = AV_PIX_FMT_VAAPI;
    frames_ctx->sw_format = AV_PIX_FMT_NV12; // AV_PIX_FMT_NV12; // AV_PIX_FMT_YUV420P
    frames_ctx->width     = ctx->width;
    frames_ctx->height    = ctx->height;
    frames_ctx->initial_pool_size = init_pool_sz;
    if ((err = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
        hb_log("Failed to initialize VAAPI frame context."
                "Error code: %s",av_err2str(err));
        av_buffer_unref(&hw_frames_ref);
        return err;
    }
    ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if (!ctx->hw_frames_ctx) {
        err = AVERROR(ENOMEM);
    }
    av_buffer_unref(&hw_frames_ref);
    return err;
#else
    hb_log("VAAPI n/a.");
    return -1;
#endif
}

