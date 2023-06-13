/* hwaccel.c
 *
 * Copyright (c) 2003-2023 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/hbffmpeg.h"
#include "handbrake/handbrake.h"

enum AVHWDeviceType hb_hwaccel_available(int codec_id, const char *hwdevice_name)
{
    if (is_hardware_disabled())
    {
        return 0;
    }

    const AVCodec *codec = avcodec_find_decoder(codec_id);
    enum AVHWDeviceType hw_type = av_hwdevice_find_type_by_name(hwdevice_name);

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
        if (job->hw_pix_fmt != AV_PIX_FMT_NONE)
        {
            if (*p == job->hw_pix_fmt)
            {
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

int hb_hwaccel_hw_ctx_init(hb_job_t *job)
{
    enum AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_NONE;
    enum AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
    int err = 0;

    const AVCodec *codec = avcodec_find_decoder(job->title->video_codec_param);

    if (job->hw_decode == HB_DECODE_SUPPORT_VIDEOTOOLBOX)
    {
        hw_type = av_hwdevice_find_type_by_name("videotoolbox");
    }
    else if (job->hw_decode == HB_DECODE_SUPPORT_NVDEC)
    {
        hw_type = av_hwdevice_find_type_by_name("cuda");
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
        AVBufferRef *hw_device_ctx;
        if ((err = av_hwdevice_ctx_create(&hw_device_ctx, hw_type, NULL, NULL, 0)) < 0)
        {
            hb_error("hwaccel: failed to create hwdevice");
        }
        else
        {
            job->hw_device_ctx = hw_device_ctx;
        }
    }

    return err;
}

void hb_hwaccel_hw_ctx_close(hb_job_t *job)
{
    if (job->hw_device_ctx)
    {
        AVBufferRef *hw_device_ctx = (AVBufferRef *)job->hw_device_ctx;
        av_buffer_unref(&hw_device_ctx);
    }
}

int hb_hwaccel_hwframes_ctx_init(AVCodecContext *ctx, hb_job_t *job)
{
    if (!ctx->hw_device_ctx)
    {
        hb_error("hwaccel: failed to initialize hw frames context");
        return 1;
    }

    ctx->get_format = hw_hwaccel_get_hw_format;
    ctx->pix_fmt = job->hw_pix_fmt;
    ctx->hw_frames_ctx = av_hwframe_ctx_alloc(ctx->hw_device_ctx);

    AVHWFramesContext *frames_ctx = (AVHWFramesContext *)ctx->hw_frames_ctx->data;
    frames_ctx->format = job->hw_pix_fmt;
    frames_ctx->sw_format = job->output_pix_fmt;
    frames_ctx->width = ctx->width;
    frames_ctx->height = ctx->height;

    if (av_hwframe_ctx_init(ctx->hw_frames_ctx) != 0)
    {
        hb_error("hwaccel: failed to initialize hw frames context");
        return 1;
    }

    return 0;
}

AVBufferRef *hb_hwaccel_init_hw_frames_ctx(AVBufferRef *hw_device_ctx,
                                       enum AVPixelFormat sw_fmt,
                                       enum AVPixelFormat hw_fmt,
                                       int width,
                                       int height)
{
    AVBufferRef *hw_frames_ctx = av_hwframe_ctx_alloc(hw_device_ctx);
    AVHWFramesContext *frames_ctx = (AVHWFramesContext*)hw_frames_ctx->data;
    frames_ctx->format = hw_fmt;
    frames_ctx->sw_format = sw_fmt;
    frames_ctx->width = width;
    frames_ctx->height = height;
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
                                       job->width, job->height);
    return av_hwframe_get_buffer(hw_frames_ctx, *frame, 0);
}

hb_buffer_t * hb_hwaccel_copy_video_buffer_to_hw_video_buffer(hb_job_t *job, hb_buffer_t *buf)
{
    AVFrame frame = {{0}};
    AVFrame *hw_frame = NULL;

    int ret;

    hb_video_buffer_to_avframe(&frame, buf);

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

    hb_buffer_close(&buf);

    hb_buffer_t *out = hb_avframe_to_video_buffer(hw_frame, (AVRational){1,1});

    av_frame_unref(&frame);
    av_frame_unref(hw_frame);

    return out;

fail:
    av_frame_unref(&frame);
    av_frame_unref(hw_frame);
    return NULL;
}

const char * hb_hwaccel_get_codec_name(enum AVCodecID codec_id)
{
    switch (codec_id)
    {
        case AV_CODEC_ID_H264:
            return "h264_hwaccel";

        case AV_CODEC_ID_HEVC:
            return "hevc_hwaccel";

        case AV_CODEC_ID_AV1:
            return "av1_hwaccel";

        default:
            return "hwaccel";
    }
}

static int is_codec_supported(int codec_id)
{
    switch (codec_id)
    {
        case HB_VCODEC_FFMPEG_NVENC_H264:
        case HB_VCODEC_FFMPEG_NVENC_H265:
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
        case HB_VCODEC_FFMPEG_NVENC_AV1:
        case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
        case HB_VCODEC_VT_H264:
        case HB_VCODEC_VT_H265:
        case HB_VCODEC_VT_H265_10BIT:
            return 1;
        default:
            return 0;
    }
}

static int are_filters_supported(hb_list_t *filters)
{
    int ret = 1;

    for (int i = 0; i < hb_list_count(filters); i++)
    {
        hb_filter_object_t *filter = hb_list_item(filters, i);
        switch (filter->id)
        {
            case HB_FILTER_VFR:
            {
                // Mode 0 doesn't require access to the frame data
                int mode = hb_dict_get_int(filter->settings, "mode");
                if (mode == 0)
                {
                    break;
                }
            }
            // TODO: enable after fixing hw crop
            //case HB_FILTER_CROP_SCALE:
            //case HB_FILTER_AVFILTER:
            //    break;
            default:
                hb_deep_log(2, "hwaccel: %s isn't yet supported for hw video frames", filter->name);
                ret = 0;
                break;
        }
    }

    return ret;
}

int hb_hwaccel_is_enabled(hb_job_t *job)
{
    return job != NULL &&
           (job->title->video_decode_support & HB_DECODE_SUPPORT_HWACCEL) &&
           (job->hw_decode & HB_DECODE_SUPPORT_HWACCEL);
}

int hb_hwaccel_is_full_hardware_pipeline_enabled(hb_job_t *job)
{
    return hb_hwaccel_is_enabled(job) &&
            are_filters_supported(job->list_filter) &&
            is_codec_supported(job->vcodec);
}

int hb_hwaccel_decode_is_enabled(hb_job_t *job)
{
    return hb_hwaccel_is_full_hardware_pipeline_enabled(job);
}
