/* hwaccel.c
 *
 * Copyright (c) 2003-2025 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/hwaccel.h"
#include "handbrake/handbrake.h"
#include "handbrake/qsv_common.h"

static int hwframe_init(const hb_job_t *job, AVFrame **frame)
{
    AVBufferRef *hw_frames_ctx = NULL;
    AVBufferRef *hw_device_ctx = job->hw_device_ctx;

    if (!hw_device_ctx || !frame)
    {
        hb_error("hwaccel: failed to initialize hw frame");
        return 1;
    }

    *frame = av_frame_alloc();
    hw_frames_ctx = hb_hwaccel_init_hw_frames_ctx(hw_device_ctx,
                                                  job->input_pix_fmt, job->hw_pix_fmt,
                                                  job->width, job->height, 0);
    return av_hwframe_get_buffer(hw_frames_ctx, *frame, 0);
}

static hb_buffer_t * upload(const hb_job_t *job, hb_buffer_t **buf_in)
{
    AVFrame frame = {{0}};
    AVFrame *hw_frame = NULL;

    int ret;

    hb_video_buffer_to_avframe(&frame, buf_in);

    ret = hwframe_init(job, &hw_frame);
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

static int can_filter(hb_list_t *filters)
{
    return 0;
}

static void * find_decoder(int codec_param)
{
    if (codec_param == AV_CODEC_ID_AV1)
    {
        return (void *)avcodec_find_decoder_by_name("av1");
    }
    else
    {
        return (void *)avcodec_find_decoder(codec_param);
    }
}

static hb_list_t *hb_list_hwaccel = NULL;

void hb_register_hwaccel(hb_hwaccel_t *hwaccel)
{
    if (hb_list_hwaccel == NULL)
    {
        hb_list_hwaccel = hb_list_init();
    }
    hb_list_add(hb_list_hwaccel, hwaccel);
}

void hb_hwaccel_common_hwaccel_init()
{
    for (int ii = 0; ii < hb_list_count(hb_list_hwaccel); ii++)
    {
        hb_hwaccel_t *hwaccel = hb_list_item(hb_list_hwaccel, ii);
        if (hwaccel->can_filter == NULL)
        {
            hwaccel->can_filter = can_filter;
        }
        if (hwaccel->find_decoder == NULL)
        {
            hwaccel->find_decoder = find_decoder;
        }
        if (hwaccel->upload == NULL)
        {
            hwaccel->upload = upload;
        }
    }
}

hb_hwaccel_t * hb_get_hwaccel(int hw_decode)
{
    hw_decode &= ~HB_DECODE_FORCE_HW;

    for (int ii = 0; ii < hb_list_count(hb_list_hwaccel); ii++)
    {
        hb_hwaccel_t *hwaccel = hb_list_item(hb_list_hwaccel, ii);

        if (hw_decode == hwaccel->id)
        {
            return hwaccel;
        }
    }

    return NULL;
}

hb_hwaccel_t * hb_get_hwaccel_from_pix_fmt(enum AVPixelFormat hw_pix_fmt)
{
    for (int ii = 0; ii < hb_list_count(hb_list_hwaccel); ii++)
    {
        hb_hwaccel_t *hwaccel = hb_list_item(hb_list_hwaccel, ii);

        if (hw_pix_fmt == hwaccel->hw_pix_fmt)
        {
            return hwaccel;
        }
    }

    return NULL;
}

static int is_encoder_supported(hb_hwaccel_t *hwaccel, int encoder)
{
    const int *encoders = hwaccel->encoders;
    while (*encoders != HB_VCODEC_INVALID)
    {
        if (*encoders == encoder)
        {
            return 1;
        }
        encoders++;
    }
    return 0;
}

int hb_hwaccel_can_use_full_hw_pipeline(hb_hwaccel_t *hwaccel, hb_list_t *list_filter, int encoder)
{
    return hwaccel != NULL &&
        hwaccel->can_filter(list_filter) &&
        is_encoder_supported(hwaccel, encoder);
}

const AVCodecHWConfig *get_hw_config(const AVCodec *codec, enum AVHWDeviceType device_type)
{
    for (int i = 0;; i++)
    {
        const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
        if (config == NULL)
        {
            return NULL;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
            config->device_type == device_type)
        {
            return config;
        }
    }
    return NULL;
}

int hb_hwaccel_is_available(hb_hwaccel_t *hwaccel, int codec_id)
{
    if (hb_is_hardware_disabled() || hwaccel == NULL)
    {
        return 0;
    }

    const AVCodec *codec = hwaccel->find_decoder(codec_id);
    const AVCodecHWConfig *config = get_hw_config(codec, hwaccel->type);

    return config != NULL;
}

enum AVPixelFormat hw_hwaccel_get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    const hb_job_t *job = ctx->opaque;
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++)
    {
        if (job && job->hw_pix_fmt != AV_PIX_FMT_NONE)
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

int hb_hwaccel_hw_device_ctx_init(enum AVHWDeviceType device_type, int device_index, void **hw_device_ctx)
{
    int err = 0;

    AVBufferRef *ctx;
    AVDictionary *dict = NULL;

    if (device_index >= 0)
    {
        char device[32];
        snprintf(device, 32, "%u", device_index);

        err = av_dict_set(&dict, "child_device", device, 0);
        if (err < 0)
        {
            return err;
        }
    }

#if defined(_WIN32) || defined(__MINGW32__)
    if (device_type == AV_HWDEVICE_TYPE_QSV)
    {
        av_dict_set(&dict, "child_device_type", "d3d11va", 0);
    }
#endif

    if ((err = av_hwdevice_ctx_create(&ctx, device_type, NULL, NULL, 0)) < 0)
    {
        hb_error("hwaccel: failed to create hwdevice");
    }
    else
    {
        *hw_device_ctx = ctx;
    }

    av_dict_free(&dict);

    return err;
}

void hb_hwaccel_hw_device_ctx_close(void **hw_device_ctx)
{
    if (hw_device_ctx && *hw_device_ctx)
    {
        av_buffer_unref((AVBufferRef **)hw_device_ctx);
    }
}

AVBufferRef *hb_hwaccel_init_hw_frames_ctx(AVBufferRef *hw_device_ctx,
                                           enum AVPixelFormat sw_fmt,
                                           enum AVPixelFormat hw_fmt,
                                           int width,
                                           int height,
                                           int initial_pool_size)
{
    AVBufferRef *hw_frames_ctx = av_hwframe_ctx_alloc(hw_device_ctx);
    AVHWFramesContext *frames_ctx = (AVHWFramesContext *)hw_frames_ctx->data;
    frames_ctx->format    = hw_fmt;
    frames_ctx->sw_format = sw_fmt;
    frames_ctx->width  = width;
    frames_ctx->height = height;

    if (initial_pool_size > 0)
    {
        frames_ctx->initial_pool_size = initial_pool_size;
    }
    if (av_hwframe_ctx_init(hw_frames_ctx) != 0)
    {
        hb_error("hwaccel: failed to initialize hw frames context");
        av_buffer_unref(&hw_frames_ctx);
        return NULL;
    }

    return hw_frames_ctx;
}

int hb_hwaccel_hwframes_ctx_init(AVCodecContext *ctx,
                                 enum AVPixelFormat sw_pix_fmt,
                                 enum AVPixelFormat hw_pix_fmt)
{
    if (!ctx->hw_device_ctx)
    {
        hb_error("hwaccel: failed to initialize hw frames context - no hw_device_ctx");
        return 1;
    }

    ctx->get_format = hw_hwaccel_get_hw_format;
    ctx->pix_fmt    = hw_pix_fmt;
    ctx->sw_pix_fmt = sw_pix_fmt;
    ctx->hw_frames_ctx = av_hwframe_ctx_alloc(ctx->hw_device_ctx);

    AVHWFramesContext *frames_ctx = (AVHWFramesContext *)ctx->hw_frames_ctx->data;
    frames_ctx->format    = hw_pix_fmt;
    frames_ctx->sw_format = sw_pix_fmt;
    frames_ctx->width  = ctx->width;
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
