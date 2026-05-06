/* nvenc_common.c
 *
 * Copyright (c) 2003-2026 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/hbffmpeg.h"
#include "handbrake/nvenc_common.h"
#include "handbrake/handbrake.h"

#if HB_PROJECT_FEATURE_NVENC
#include <ffnvcodec/nvEncodeAPI.h>
#include <ffnvcodec/dynlink_loader.h>
#endif

static int is_nvenc_available = -1;

typedef struct {
    int probed;
    int has_h264;
    int has_h264_10bit;
    int has_hevc;
    int has_av1;
} nvenc_caps_t;

static nvenc_caps_t g_nvenc_caps = { 0 };

int hb_check_nvenc_available()
{
    if (hb_is_hardware_disabled())
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
        if (loadErr < 0)
        {
            is_nvenc_available = 0;
            return 0;
        }

        NVENCSTATUS apiErr = nvenc_dl->NvEncodeAPIGetMaxSupportedVersion(&nvenc_ver);
        if (apiErr != NV_ENC_SUCCESS)
        {
            is_nvenc_available = 0;
            hb_log("nvenc: not available");
            return 0;
        }
        else
        {
            hb_log("nvenc: version %d.%d is available", nvenc_ver >> 4, nvenc_ver & 0xf);
            is_nvenc_available = 1;

            if (hb_check_nvdec_available())
            {
                hb_log("nvdec: is available");
            }
            else
            {
                hb_log("nvdec: is not compiled into this build");
            }
            return 1;
        }

        return 1;
    #else
        is_nvenc_available = 0;
        hb_log("nvenc: not available");
        return 0;
    #endif
}

#if HB_PROJECT_FEATURE_NVENC
static int hb_nvenc_probe_cap(NV_ENCODE_API_FUNCTION_LIST *fl, void *enc,
                              GUID guid, NV_ENC_CAPS cap)
{
    NV_ENC_CAPS_PARAM p;
    memset(&p, 0, sizeof(p));
    p.version     = NV_ENC_CAPS_PARAM_VER;
    p.capsToQuery = cap;
    int val = 0;
    return (fl->nvEncGetEncodeCaps(enc, guid, &p, &val) == NV_ENC_SUCCESS) ? val : 0;
}

static int hb_nvenc_guid_eq(GUID a, GUID b)
{
    return memcmp(&a, &b, sizeof(GUID)) == 0;
}
#endif

static void hb_nvenc_probe_caps(void)
{
    if (g_nvenc_caps.probed)
    {
        return;
    }
    g_nvenc_caps.probed = 1;

    if (!hb_check_nvenc_available())
    {
        return;
    }

#if HB_PROJECT_FEATURE_NVENC
    CudaFunctions  *cu  = NULL;
    NvencFunctions *nv  = NULL;
    CUcontext       ctx = NULL;
    void           *enc = NULL;
    GUID           *guids = NULL;
    NV_ENCODE_API_FUNCTION_LIST fl;

    memset(&fl, 0, sizeof(fl));
    fl.version = NV_ENCODE_API_FUNCTION_LIST_VER;

    if (cuda_load_functions(&cu, NULL) < 0)
    {
        goto done;
    }
    if (cu->cuInit(0) != CUDA_SUCCESS)
    {
        goto done;
    }
    if (nvenc_load_functions(&nv, NULL) < 0)
    {
        goto done;
    }
    if (nv->NvEncodeAPICreateInstance(&fl) != NV_ENC_SUCCESS)
    {
        goto done;
    }

    int dev_count = 0;
    cu->cuDeviceGetCount(&dev_count);
    if (dev_count <= 0)
    {
        goto done;
    }

    CUdevice dev;
    if (cu->cuDeviceGet(&dev, 0) != CUDA_SUCCESS)
    {
        goto done;
    }
    if (cu->cuCtxCreate(&ctx, 0, dev) != CUDA_SUCCESS)
    {
        goto done;
    }

    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS sess;
    memset(&sess, 0, sizeof(sess));
    sess.version    = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    sess.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
    sess.device     = ctx;
    sess.apiVersion = NVENCAPI_VERSION;

    if (fl.nvEncOpenEncodeSessionEx(&sess, &enc) != NV_ENC_SUCCESS || enc == NULL)
    {
        goto done;
    }

    uint32_t count = 0;
    if (fl.nvEncGetEncodeGUIDCount(enc, &count) != NV_ENC_SUCCESS || count == 0)
    {
        goto done;
    }

    guids = (GUID *)calloc(count, sizeof(GUID));
    if (guids == NULL)
    {
        goto done;
    }

    uint32_t got = 0;
    if (fl.nvEncGetEncodeGUIDs(enc, guids, count, &got) != NV_ENC_SUCCESS)
    {
        goto done;
    }

    for (uint32_t i = 0; i < got; i++)
    {
        if (hb_nvenc_guid_eq(guids[i], NV_ENC_CODEC_H264_GUID))
        {
            g_nvenc_caps.has_h264       = 1;
            g_nvenc_caps.has_h264_10bit = hb_nvenc_probe_cap(
                &fl, enc, NV_ENC_CODEC_H264_GUID,
                NV_ENC_CAPS_SUPPORT_10BIT_ENCODE) > 0;
        }
        else if (hb_nvenc_guid_eq(guids[i], NV_ENC_CODEC_HEVC_GUID))
        {
            g_nvenc_caps.has_hevc = 1;
        }
        else if (hb_nvenc_guid_eq(guids[i], NV_ENC_CODEC_AV1_GUID))
        {
            g_nvenc_caps.has_av1 = 1;
        }
    }

    hb_log("nvenc: caps probe -> h264=%d h264_10bit=%d hevc=%d av1=%d",
           g_nvenc_caps.has_h264, g_nvenc_caps.has_h264_10bit,
           g_nvenc_caps.has_hevc, g_nvenc_caps.has_av1);

done:
    free(guids);
    if (enc != NULL)
    {
        fl.nvEncDestroyEncoder(enc);
    }
    if (ctx != NULL && cu != NULL)
    {
        cu->cuCtxDestroy(ctx);
    }
    nvenc_free_functions(&nv);
    cuda_free_functions(&cu);
#endif
}

int hb_nvenc_h264_available()
{
    hb_nvenc_probe_caps();
    return g_nvenc_caps.has_h264;
}

int hb_nvenc_h264_10bit_available()
{
    hb_nvenc_probe_caps();
    return g_nvenc_caps.has_h264_10bit;
}

int hb_nvenc_h265_available()
{
    hb_nvenc_probe_caps();
    return g_nvenc_caps.has_hevc;
}

int hb_nvenc_av1_available()
{
    hb_nvenc_probe_caps();
    return g_nvenc_caps.has_av1;
}

int hb_check_nvdec_available()
{
    #if HB_PROJECT_FEATURE_NVDEC
        return 1;
    #else
        return 0;
    #endif
}

const char * hb_map_nvenc_preset_name (const char * preset)
{
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

static int hb_nvenc_are_filters_supported(hb_list_t *filters)
{
    int ret = 1;

    for (int i = 0; i < hb_list_count(filters); i++)
    {
        int supported = 1;
        hb_filter_object_t *filter = hb_list_item(filters, i);

        switch (filter->id)
        {
            case HB_FILTER_VFR:
                // Mode 0 doesn't require access to the frame data
                supported = hb_dict_get_int(filter->settings, "mode") == 0;
                break;
            case HB_FILTER_FORMAT:
            case HB_FILTER_AVFILTER:
                break;
            default:
                supported = 0;
        }

        if (supported == 0)
        {
            hb_deep_log(2, "hwaccel: %s isn't yet supported for hw video frames", filter->name);
            ret = 0;
        }
    }

    return ret;
}

static const int nv_encoders[] =
{
    HB_VCODEC_FFMPEG_NVENC_H264, HB_VCODEC_FFMPEG_NVENC_H264_10BIT,
    HB_VCODEC_FFMPEG_NVENC_H265, HB_VCODEC_FFMPEG_NVENC_H265_10BIT,
    HB_VCODEC_FFMPEG_NVENC_AV1, HB_VCODEC_FFMPEG_NVENC_AV1_10BIT,
    HB_VCODEC_INVALID
};

hb_hwaccel_t hb_hwaccel_nvdec =
{
    .id         = HB_DECODE_NVDEC,
    .name       = "nvdec hwaccel",
    .encoders   = nv_encoders,
    .type       = AV_HWDEVICE_TYPE_CUDA,
    .hw_pix_fmt = AV_PIX_FMT_CUDA,
    .can_filter = hb_nvenc_are_filters_supported,
    .caps       = HB_HWACCEL_CAP_SCAN | HB_HWACCEL_CAP_COLOR_RANGE
};
