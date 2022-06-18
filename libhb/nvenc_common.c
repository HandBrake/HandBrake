/* nvenc_common.c
 *
 * Copyright (c) 2003-2022 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/hbffmpeg.h"
#include "handbrake/handbrake.h"

#if HB_PROJECT_FEATURE_NVENC
#include <ffnvcodec/nvEncodeAPI.h>
#include <ffnvcodec/dynlink_loader.h>
#endif

static int is_nvenc_available = -1;

int hb_check_nvenc_available()
{
    if (is_hardware_disabled())
    {
        return 0;
    }

    if (is_nvenc_available != -1)
    {
        return is_nvenc_available;
    }

    #if HB_PROJECT_FEATURE_NVENC
        uint32_t nvenc_ver;
        void *context = NULL;
        NvencFunctions *nvenc_dl = NULL;

        int loadErr = nvenc_load_functions(&nvenc_dl, context);
        if (loadErr < 0) {
            is_nvenc_available = 0;
            return 0;
        }

        NVENCSTATUS apiErr = nvenc_dl->NvEncodeAPIGetMaxSupportedVersion(&nvenc_ver);
        if (apiErr != NV_ENC_SUCCESS) {
            is_nvenc_available = 0;
            return 0;
        } else {
            hb_deep_log(1, "NVENC version %d.%d\n", nvenc_ver >> 4, nvenc_ver & 0xf);
            is_nvenc_available = 1;
            return 1;
        }

        return 1;
    #else
        return 0;
    #endif
}

int hb_nvenc_h264_available()
{
    #if HB_PROJECT_FEATURE_NVENC
        return hb_check_nvenc_available();
    #else
        return is_nvenc_available;
    #endif
}

int hb_nvenc_h265_available()
{
    #if HB_PROJECT_FEATURE_NVENC
        return hb_check_nvenc_available();
    #else
        return is_nvenc_available;
    #endif
}

char * hb_map_nvenc_preset_name (const char * preset){

    if (preset == NULL)
    {
        return "p4";
    }

    if (strcmp(preset, "fastest") == 0) {
      return "p1";
    }  else if (strcmp(preset, "faster") == 0) {
      return "p2";
    } else if (strcmp(preset, "fast") == 0) {
       return "p3";
    } else if (strcmp(preset, "medium") == 0) {
      return "p4";
    } else if (strcmp(preset, "slow") == 0) {
      return "p5";
    } else if (strcmp(preset, "slower") == 0) {
       return "p6";
    } else if (strcmp(preset, "slowest") == 0) {
      return "p7";
    }

    return "p4"; // Default to Medium
}

int hb_nvdec_available(int codec_id)
{
    if (is_hardware_disabled())
    {
        return 0;
    }

    AVCodec *codec = avcodec_find_decoder(codec_id);
    enum AVHWDeviceType type = av_hwdevice_find_type_by_name("cuda");
    for (int i = 0; codec; i++)
    {
        const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
        if (!config)
        {
            return 0;
        }
        if ((AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX & config->methods) &&
            (type == config->device_type))
        {
            return 1;
        }
    }

    return 0;
}

static enum AVPixelFormat get_hw_pix_fmt(AVCodecContext *ctx,
                                         const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++)
    {
        if (*p == AV_PIX_FMT_CUDA)
        {
            return *p;
        }
    }

    hb_error("Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

int hb_nvdec_hw_ctx_init(AVCodecContext *ctx, hb_job_t *job)
{
#if HB_PROJECT_FEATURE_NVENC
    ctx->get_format = get_hw_pix_fmt;
    int err = av_hwdevice_ctx_create(&ctx->hw_device_ctx, AV_HWDEVICE_TYPE_CUDA,
                                     NULL, NULL, 0);
    if (err < 0)
    {
        hb_error("Failed to create specified HW device.\n");
    }

    job->nv_hw_ctx.hw_device_ctx = av_buffer_ref(ctx->hw_device_ctx);

    return err;
#else
    return -1;
#endif
}

int hb_nvdec_hwframes_ctx_init(AVCodecContext *ctx, hb_job_t *job)
{
    if (!ctx->hw_device_ctx)
    {
        hb_error("failed to initialize hw frames context");
        return 1;
    }

    ctx->get_format = get_hw_pix_fmt;
    ctx->pix_fmt = AV_PIX_FMT_CUDA;
    ctx->hw_frames_ctx = av_hwframe_ctx_alloc(ctx->hw_device_ctx);

    AVHWFramesContext *frames_ctx = ctx->hw_frames_ctx->data;
    frames_ctx->format = AV_PIX_FMT_CUDA;
    frames_ctx->sw_format = job->output_pix_fmt;
    frames_ctx->width = ctx->width;
    frames_ctx->height = ctx->height;

    if (0 != av_hwframe_ctx_init(ctx->hw_frames_ctx))
    {
        hb_error("failed to initialize hw frames context");
        return 1;
    }

    return 0;
}

static AVBufferRef *init_hw_frames_ctx(AVBufferRef *hw_device_ctx,
                                       enum AVPixelFormat sw_fmt,
                                       int width,
                                       int height)
{
    AVBufferRef *hw_frames_ctx = av_hwframe_ctx_alloc(hw_device_ctx);
    AVHWFramesContext *frames_ctx = hw_frames_ctx->data;
    frames_ctx->format = AV_PIX_FMT_CUDA;
    frames_ctx->sw_format = sw_fmt;
    frames_ctx->width = width;
    frames_ctx->height = height;
    if (0 != av_hwframe_ctx_init(hw_frames_ctx))
    {
        hb_error("failed to initialize hw frames context");
        av_buffer_unref(&hw_frames_ctx);
        return NULL;
    }

    return hw_frames_ctx;
}

int hb_nvdec_hwframe_init(hb_job_t *job, AVFrame **frame)
{
#if HB_PROJECT_FEATURE_NVENC
    AVBufferRef *hw_frames_ctx = NULL;
    AVBufferRef *hw_device_ctx = job->nv_hw_ctx.hw_device_ctx;

    if (!hw_device_ctx || !frame)
    {
        hb_error("failed to initialize hw frame");
        return 1;
    }

    *frame = av_frame_alloc();
    hw_frames_ctx = init_hw_frames_ctx(hw_device_ctx, job->input_pix_fmt,
                                       job->width, job->height);
    return av_hwframe_get_buffer(hw_frames_ctx, *frame, 0);
#else
    return -1;
#endif
}