/* hwaccel.c
 *
 * Copyright (c) 2003-2025 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/hbffmpeg.h"
#include "handbrake/handbrake.h"
#include "handbrake/nvenc_common.h"
#include "handbrake/qsv_common.h"
#ifdef __APPLE__
#include "platform/macosx/vt_common.h"
#endif

static int is_encoder_supported(int hw_decode, int encoder_id)
{
    int ret = 0;
#ifdef __APPLE__
    if (hw_decode & HB_DECODE_SUPPORT_VIDEOTOOLBOX)
    {
        switch (encoder_id)
        {
            case HB_VCODEC_VT_H264:
            case HB_VCODEC_VT_H265:
            case HB_VCODEC_VT_H265_10BIT:
                ret = 1;
                break;
            default:
                ret = 0;
        }
    }
#endif
    if (hw_decode & HB_DECODE_SUPPORT_NVDEC)
    {
        switch (encoder_id)
        {
            case HB_VCODEC_FFMPEG_NVENC_H264:
            case HB_VCODEC_FFMPEG_NVENC_H265:
            case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
            case HB_VCODEC_FFMPEG_NVENC_AV1:
            case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
                ret = 1;
                break;
            default:
                ret = 0;
        }
    }
#if HB_PROJECT_FEATURE_QSV
    if (hw_decode & HB_DECODE_SUPPORT_QSV)
    {
        switch (encoder_id)
        {
            case HB_VCODEC_FFMPEG_QSV_H264:
            case HB_VCODEC_FFMPEG_QSV_H265:
            case HB_VCODEC_FFMPEG_QSV_H265_10BIT:
            case HB_VCODEC_FFMPEG_QSV_AV1:
            case HB_VCODEC_FFMPEG_QSV_AV1_10BIT:
                ret = 1;
                break;
            default:
                ret = 0;
        }
    }
#endif
    return ret;
}

static int are_filters_supported(hb_job_t *job)
{
    int ret = 0;
#ifdef __APPLE__
    if (job->hw_decode & HB_DECODE_SUPPORT_VIDEOTOOLBOX)
    {
        ret = hb_vt_are_filters_supported(job->list_filter);
    }
#endif
    if (job->hw_decode & HB_DECODE_SUPPORT_NVDEC)
    {
        ret = hb_nvenc_are_filters_supported(job->list_filter);
    }
#if HB_PROJECT_FEATURE_QSV
    if (job->hw_decode & HB_DECODE_SUPPORT_QSV)
    {
        ret = hb_qsv_are_filters_supported(job);
    }
#endif
    return ret;
}

int hb_hwaccel_is_enabled(hb_job_t *job)
{
    return job != NULL && (job->title->video_decode_support & job->hw_decode);
}

int hb_hwaccel_is_full_hardware_pipeline_enabled(hb_job_t *job)
{
    return hb_hwaccel_is_enabled(job) &&
            are_filters_supported(job) &&
            is_encoder_supported(job->hw_decode, job->vcodec);
}

int hb_hwaccel_decode_is_enabled(hb_job_t *job)
{
    if (job != NULL)
    {
        if (job->hw_decode & HB_DECODE_SUPPORT_FORCE_HW)
        {
            return hb_hwaccel_is_enabled(job);
        }
        else
        {
            return hb_hwaccel_is_full_hardware_pipeline_enabled(job);
        }
    }
    else
    {
        return 0;
    }
}

const char * hb_hwaccel_get_name(int hw_decode)
{
    if (hw_decode & HB_DECODE_SUPPORT_VIDEOTOOLBOX)
    {
        return "videotoolbox hwaccel";
    }
    else if (hw_decode & HB_DECODE_SUPPORT_NVDEC)
    {
        return "nvdec hwaccel";
    }
    else if (hw_decode & HB_DECODE_SUPPORT_QSV)
    {
        return "qsv";
    }
    else if (hw_decode & HB_DECODE_SUPPORT_MF)
    {
        return "mf hwaccel";
    }
    else
    {
        return "unknown";
    }
}

enum AVHWDeviceType hb_hwaccel_available(int codec_id, const char *hwdevice_name)
{
    if (is_hardware_disabled())
    {
        return 0;
    }

    const AVCodec *codec = avcodec_find_decoder(codec_id);
    enum AVHWDeviceType hw_type = av_hwdevice_find_type_by_name(hwdevice_name);
    if (hw_type == AV_HWDEVICE_TYPE_QSV)
    {
        return 1;
    }

    if (hw_type != AV_HWDEVICE_TYPE_NONE)
    {
        for (int i = 0;; i++)
        {
            const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
            if (!config)
            {
                return 0;
            }
            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                config->device_type == hw_type)
            {
                return 1;
            }
        }
    }

    return 0;
}

enum AVPixelFormat hw_hwaccel_get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    const hb_job_t *job = ctx->opaque;
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++)
    {
        if (job && job->hw_pix_fmt != AV_PIX_FMT_NONE)
        {
            if (*p == job->hw_pix_fmt)
            {
#if HB_PROJECT_FEATURE_QSV
                if (*p == AV_PIX_FMT_QSV)
                {
                    if (job->qsv_ctx->hw_frames_ctx)
                    {
                        // in case if decoder and encoder have the same size
                        ctx->hw_frames_ctx = av_buffer_ref(job->qsv_ctx->hw_frames_ctx);
                    }
                }
#endif
                return *p;
            }
        }
        else
        {
            return *p;
        }
    }

    hb_error("hwaccel: failed to get HW surface format");
    return AV_PIX_FMT_NONE;
}

int hb_hwaccel_hw_ctx_init(int codec_id, int hw_decode, void **hw_device_ctx, hb_job_t *job)
{
    enum AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_NONE;
    enum AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
    int err = 0;

    const AVCodec *codec = avcodec_find_decoder(codec_id);

    if (hw_decode & HB_DECODE_SUPPORT_VIDEOTOOLBOX)
    {
        hw_type = av_hwdevice_find_type_by_name("videotoolbox");
    }
    else if (hw_decode & HB_DECODE_SUPPORT_NVDEC)
    {
        hw_type = av_hwdevice_find_type_by_name("cuda");
    }
#if HB_PROJECT_FEATURE_QSV
    else if (hw_decode & HB_DECODE_SUPPORT_QSV)
    {
        AVBufferRef *ctx = NULL;
        hw_type = av_hwdevice_find_type_by_name("qsv");
        pix_fmt = AV_PIX_FMT_QSV;
        err = hb_qsv_device_init(job, (void**)&ctx);
        if (err < 0)
        {
            hb_error("hwaccel: failed to create hwdevice");
            return err;
        }
        *hw_device_ctx = av_buffer_ref(ctx);
        return err;
    }
#endif
    else if (hw_decode & HB_DECODE_SUPPORT_MF)
    {
        hw_type = av_hwdevice_find_type_by_name("d3d11va");
    }

    if (hw_type != AV_HWDEVICE_TYPE_NONE)
    {
        for (int i = 0;; i++)
        {
            const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
            if (!config)
            {
                break;
            }
            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                config->device_type == hw_type)
            {
                pix_fmt = config->pix_fmt;
                break;
            }
        }
    }

    if (pix_fmt != AV_PIX_FMT_NONE)
    {
        AVBufferRef *ctx;
        if ((err = av_hwdevice_ctx_create(&ctx, hw_type, NULL, NULL, 0)) < 0)
        {
            hb_error("hwaccel: failed to create hwdevice");
        }
        else
        {
            *hw_device_ctx = ctx;
        }
    }

    return err;
}

void hb_hwaccel_hw_ctx_close(void **hw_device_ctx)
{
    if (hw_device_ctx && *hw_device_ctx)
    {
        av_buffer_unref((AVBufferRef **)hw_device_ctx);
    }
}

int hb_hwaccel_hwframes_ctx_init(AVCodecContext *ctx,
                                 enum AVPixelFormat hw_pix_fmt,
                                 enum AVPixelFormat sw_pix_fmt)
{
    if (!ctx->hw_device_ctx)
    {
        hb_error("hwaccel: failed to initialize hw frames context - no hw_device_ctx");
        return 1;
    }

    ctx->get_format = hw_hwaccel_get_hw_format;
    ctx->pix_fmt = hw_pix_fmt;
    ctx->sw_pix_fmt = sw_pix_fmt;
    ctx->hw_frames_ctx = av_hwframe_ctx_alloc(ctx->hw_device_ctx);

    AVHWFramesContext *frames_ctx = (AVHWFramesContext *)ctx->hw_frames_ctx->data;
    frames_ctx->format = hw_pix_fmt;
    frames_ctx->sw_format = sw_pix_fmt;
    frames_ctx->width = ctx->width;
    frames_ctx->height = ctx->height;

#if HB_PROJECT_FEATURE_QSV
    if (hw_pix_fmt == AV_PIX_FMT_QSV)
    {
        ctx->extra_hw_frames = HB_QSV_FFMPEG_EXTRA_HW_FRAMES;

        frames_ctx->initial_pool_size = HB_QSV_FFMPEG_INITIAL_POOL_SIZE;

        AVQSVFramesContext *frames_hwctx = frames_ctx->hwctx;
        frames_hwctx->frame_type = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;
    }
#endif

    if (av_hwframe_ctx_init(ctx->hw_frames_ctx) != 0)
    {
        hb_error("hwaccel: failed to initialize hw frames context - av_hwframe_ctx_init");
        return 1;
    }

    return 0;
}

AVBufferRef *hb_hwaccel_init_hw_frames_ctx(AVBufferRef *hw_device_ctx,
                                       enum AVPixelFormat sw_fmt,
                                       enum AVPixelFormat hw_fmt,
                                       int width,
                                       int height,
                                       int initial_pool_size)
{
    AVBufferRef *hw_frames_ctx = av_hwframe_ctx_alloc(hw_device_ctx);
    AVHWFramesContext *frames_ctx = (AVHWFramesContext*)hw_frames_ctx->data;
    frames_ctx->format = hw_fmt;
    frames_ctx->sw_format = sw_fmt;
    frames_ctx->width = width;
    frames_ctx->height = height;

    if (initial_pool_size > 0)
    {
        frames_ctx->initial_pool_size = initial_pool_size;
    }
    if (0 != av_hwframe_ctx_init(hw_frames_ctx))
    {
        hb_error("hwaccel: failed to initialize hw frames context");
        av_buffer_unref(&hw_frames_ctx);
        return NULL;
    }

    return hw_frames_ctx;
}

static int hb_hwaccel_hwframe_init(hb_job_t *job, AVFrame **frame)
{
    AVBufferRef *hw_frames_ctx = NULL;
    AVBufferRef *hw_device_ctx = job->hw_device_ctx;

    if (!hw_device_ctx || !frame)
    {
        hb_error("hwaccel: failed to initialize hw frame");
        return 1;
    }

    *frame = av_frame_alloc();
    hw_frames_ctx = hb_hwaccel_init_hw_frames_ctx(hw_device_ctx, job->input_pix_fmt, job->hw_pix_fmt,
                                       job->width, job->height, 0);
    return av_hwframe_get_buffer(hw_frames_ctx, *frame, 0);
}

hb_buffer_t * hb_hwaccel_copy_video_buffer_to_hw_video_buffer(hb_job_t *job, hb_buffer_t **buf_in)
{
#ifdef __APPLE__
    if (job->hw_pix_fmt == AV_PIX_FMT_VIDEOTOOLBOX)
    {
        return hb_vt_copy_video_buffer_to_hw_video_buffer(job, buf_in);
    }
    else
#endif
    {
        AVFrame frame = {{0}};
        AVFrame *hw_frame = NULL;

        int ret;

        hb_video_buffer_to_avframe(&frame, buf_in);

        ret = hb_hwaccel_hwframe_init(job, &hw_frame);
        if (ret < 0)
        {
            goto fail;
        }

        av_frame_copy_props(hw_frame, &frame);
        if (ret < 0)
        {
            goto fail;
        }

        av_hwframe_transfer_data(hw_frame, &frame, 0);
        if (ret < 0)
        {
            goto fail;
        }

        hb_buffer_t *out = hb_avframe_to_video_buffer(hw_frame, (AVRational){1,1});

        av_frame_unref(&frame);
        av_frame_unref(hw_frame);

        return out;

    fail:
        av_frame_unref(&frame);
        av_frame_unref(hw_frame);
        return NULL;
    }

    return NULL;
}
