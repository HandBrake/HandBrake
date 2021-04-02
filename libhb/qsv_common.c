/* qsv_common.c
 *
 * Copyright (c) 2003-2021 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/project.h"

#if HB_PROJECT_FEATURE_QSV

#include <stdio.h>
#include <string.h>

#include "handbrake/handbrake.h"
#include "handbrake/ports.h"
#include "handbrake/common.h"
#include "handbrake/hb_dict.h"
#include "handbrake/qsv_common.h"
#include "handbrake/h264_common.h"
#include "handbrake/h265_common.h"
#include "handbrake/hbffmpeg.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavutil/hwcontext_qsv.h"
#include "libavutil/hwcontext.h"
#include "mfx/mfxadapter.h"

typedef struct hb_qsv_adapter_details
{
    // DirectX index
    int index;
    // QSV info for each codec
    hb_qsv_info_t *hb_qsv_info_avc;
    hb_qsv_info_t *hb_qsv_info_hevc;
    // API versions
    mfxVersion qsv_software_version;
    mfxVersion qsv_hardware_version;
    // AVC implementations
    hb_qsv_info_t qsv_software_info_avc;
    hb_qsv_info_t qsv_hardware_info_avc;
    // HEVC implementations
    hb_qsv_info_t qsv_software_info_hevc;
    hb_qsv_info_t qsv_hardware_info_hevc;
} hb_qsv_adapter_details_t;

// QSV info about adapters
#if defined(_WIN32) || defined(__MINGW32__)
static mfxAdaptersInfo g_qsv_adapters_info;
static const char* hb_qsv_get_adapter_type(const mfxAdapterInfo* info);
#endif
int hb_qsv_get_platform(int adapter_index);
static hb_list_t *g_qsv_adapters_list         = NULL;
static hb_list_t *g_qsv_adapters_details_list = NULL;
static int g_adapter_index = 0;
static int g_default_adapter_index = 0;

static void init_adapter_details(hb_qsv_adapter_details_t *adapter_details)
{
    adapter_details->index                                 = 0;
    // QSV info for each codec
    adapter_details->hb_qsv_info_avc                       = NULL;
    adapter_details->hb_qsv_info_hevc                      = NULL;
    // API versions
    adapter_details->qsv_software_version.Version          = 0;
    adapter_details->qsv_hardware_version.Version          = 0;
    // AVC implementations
    adapter_details->qsv_software_info_avc.available       = 0;
    adapter_details->qsv_software_info_avc.codec_id        = MFX_CODEC_AVC;
    adapter_details->qsv_software_info_avc.implementation  = MFX_IMPL_SOFTWARE;

    adapter_details->qsv_hardware_info_avc.available       = 0;
    adapter_details->qsv_hardware_info_avc.codec_id        = MFX_CODEC_AVC;
    adapter_details->qsv_hardware_info_avc.implementation  = MFX_IMPL_HARDWARE_ANY|MFX_IMPL_VIA_ANY;
    // HEVC implementations
    adapter_details->qsv_software_info_hevc.available      = 0;
    adapter_details->qsv_software_info_hevc.codec_id       = MFX_CODEC_HEVC;
    adapter_details->qsv_software_info_hevc.implementation = MFX_IMPL_SOFTWARE;

    adapter_details->qsv_hardware_info_hevc.available      = 0;
    adapter_details->qsv_hardware_info_hevc.codec_id       = MFX_CODEC_HEVC;
    adapter_details->qsv_hardware_info_hevc.implementation = MFX_IMPL_HARDWARE_ANY|MFX_IMPL_VIA_ANY;
}

// QSV-supported profile and level lists (not all exposed to the user)
static hb_triplet_t hb_qsv_h264_profiles[] =
{
    { "Baseline",             "baseline",       MFX_PROFILE_AVC_BASELINE,             },
    { "Main",                 "main",           MFX_PROFILE_AVC_MAIN,                 },
    { "Extended",             "extended",       MFX_PROFILE_AVC_EXTENDED,             },
    { "High",                 "high",           MFX_PROFILE_AVC_HIGH,                 },
    { "High 4:2:2",           "high422",        MFX_PROFILE_AVC_HIGH_422,             },
    { "Constrained Baseline", "baseline|set1",  MFX_PROFILE_AVC_CONSTRAINED_BASELINE, },
    { "Constrained High",     "high|set4|set5", MFX_PROFILE_AVC_CONSTRAINED_HIGH,     },
    { "Progressive High",     "high|set4",      MFX_PROFILE_AVC_PROGRESSIVE_HIGH,     },
    { NULL,                                                                           },
};
static hb_triplet_t hb_qsv_h265_profiles[] =
{
    { "Main",               "main",             MFX_PROFILE_HEVC_MAIN,   },
    { "Main 10",            "main10",           MFX_PROFILE_HEVC_MAIN10, },
    { "Main Still Picture", "mainstillpicture", MFX_PROFILE_HEVC_MAINSP, },
    { NULL,                                                              },
};
static hb_triplet_t hb_qsv_vpp_scale_modes[] =
{
    { "lowpower",          "low_power",         MFX_SCALING_MODE_LOWPOWER, },
    { "hq",                "hq",                MFX_SCALING_MODE_QUALITY,  },
    { NULL,                                                                },
};
static hb_triplet_t hb_qsv_vpp_interpolation_methods[] =
{
    { "nearest",            "nearest",          MFX_INTERPOLATION_NEAREST_NEIGHBOR, },
    { "bilinear",           "bilinear",         MFX_INTERPOLATION_BILINEAR,         },
    { "advanced",           "advanced",         MFX_INTERPOLATION_ADVANCED,         },
    { NULL,                                                                         },
};
static hb_triplet_t hb_qsv_h264_levels[] =
{
    { "1.0", "1.0", MFX_LEVEL_AVC_1,  },
    { "1b",  "1b",  MFX_LEVEL_AVC_1b, },
    { "1.1", "1.1", MFX_LEVEL_AVC_11, },
    { "1.2", "1.2", MFX_LEVEL_AVC_12, },
    { "1.3", "1.3", MFX_LEVEL_AVC_13, },
    { "2.0", "2.0", MFX_LEVEL_AVC_2,  },
    { "2.1", "2.1", MFX_LEVEL_AVC_21, },
    { "2.2", "2.2", MFX_LEVEL_AVC_22, },
    { "3.0", "3.0", MFX_LEVEL_AVC_3,  },
    { "3.1", "3.1", MFX_LEVEL_AVC_31, },
    { "3.2", "3.2", MFX_LEVEL_AVC_32, },
    { "4.0", "4.0", MFX_LEVEL_AVC_4,  },
    { "4.1", "4.1", MFX_LEVEL_AVC_41, },
    { "4.2", "4.2", MFX_LEVEL_AVC_42, },
    { "5.0", "5.0", MFX_LEVEL_AVC_5,  },
    { "5.1", "5.1", MFX_LEVEL_AVC_51, },
    { "5.2", "5.2", MFX_LEVEL_AVC_52, },
    { NULL,                           },
};
static hb_triplet_t hb_qsv_h265_levels[] =
{
    { "1.0", "1.0", MFX_LEVEL_HEVC_1,  },
    { "2.0", "2.0", MFX_LEVEL_HEVC_2,  },
    { "2.1", "2.1", MFX_LEVEL_HEVC_21, },
    { "3.0", "3.0", MFX_LEVEL_HEVC_3,  },
    { "3.1", "3.1", MFX_LEVEL_HEVC_31, },
    { "4.0", "4.0", MFX_LEVEL_HEVC_4,  },
    { "4.1", "4.1", MFX_LEVEL_HEVC_41, },
    { "5.0", "5.0", MFX_LEVEL_HEVC_5,  },
    { "5.1", "5.1", MFX_LEVEL_HEVC_51, },
    { "5.2", "5.2", MFX_LEVEL_HEVC_52, },
    { "6.0", "6.0", MFX_LEVEL_HEVC_6,  },
    { "6.1", "6.1", MFX_LEVEL_HEVC_61, },
    { "6.2", "6.2", MFX_LEVEL_HEVC_62, },
    { NULL,                            },
};

#define MFX_IMPL_VIA_MASK(impl) (0x0f00 & (impl))

// check available Intel Media SDK version against a minimum
#define HB_CHECK_MFX_VERSION(MFX_VERSION, MAJOR, MINOR) \
    ((MFX_VERSION.Major * 1000 + MFX_VERSION.Minor) >= (MAJOR * 1000 + MINOR))

int hb_qsv_get_adapter_index()
{
    return g_adapter_index;
}

static int hb_qsv_set_default_adapter_index(int adapter_index)
{
    g_default_adapter_index = adapter_index;
    return 0;
}

static int hb_qsv_get_default_adapter_index()
{
    return g_default_adapter_index;
}

hb_list_t* hb_qsv_adapters_list()
{
    return g_qsv_adapters_list;
}

static hb_qsv_adapter_details_t* hb_qsv_get_adapters_details_by_index(int adapter_index)
{
    for (int i = 0; i < hb_list_count(g_qsv_adapters_details_list); i++)
    {
        hb_qsv_adapter_details_t *details = hb_list_item(g_qsv_adapters_details_list, i);
        if (details->index == adapter_index || adapter_index == -1)
        {
            return details;
        }
    }
    return NULL;
}

static int qsv_impl_set_preferred(hb_qsv_adapter_details_t *details, const char *name)
{
    if (name == NULL || details == NULL)
    {
        return -1;
    }
    if (!strcasecmp(name, "software"))
    {
        if (details->qsv_software_info_avc.available)
        {
            details->hb_qsv_info_avc = &details->qsv_software_info_avc;
        }
        if (details->qsv_software_info_hevc.available)
        {
            details->hb_qsv_info_hevc = &details->qsv_software_info_hevc;
        }
        return 0;
    }
    if (!strcasecmp(name, "hardware"))
    {
        if (details->qsv_hardware_info_avc.available)
        {
            details->hb_qsv_info_avc = &details->qsv_hardware_info_avc;
        }
        if (details->qsv_hardware_info_hevc.available)
        {
            details->hb_qsv_info_hevc = &details->qsv_hardware_info_hevc;
        }
        return 0;
    }
    return -1;
}

int hb_qsv_impl_set_preferred(const char *name)
{
    hb_qsv_adapter_details_t* details = hb_qsv_get_adapters_details_by_index(hb_qsv_get_adapter_index());
    return qsv_impl_set_preferred(details, name);
}

int hb_qsv_hardware_generation(int cpu_platform)
{
    switch (cpu_platform)
    {
        case HB_CPU_PLATFORM_INTEL_BNL:
            return QSV_G0;
        case HB_CPU_PLATFORM_INTEL_SNB:
            return QSV_G1;
        case HB_CPU_PLATFORM_INTEL_IVB:
        case HB_CPU_PLATFORM_INTEL_SLM:
        case HB_CPU_PLATFORM_INTEL_CHT:
            return QSV_G2;
        case HB_CPU_PLATFORM_INTEL_HSW:
            return QSV_G3;
        case HB_CPU_PLATFORM_INTEL_BDW:
            return QSV_G4;
        case HB_CPU_PLATFORM_INTEL_SKL:
            return QSV_G5;
        case HB_CPU_PLATFORM_INTEL_KBL:
        case HB_CPU_PLATFORM_INTEL_CML:
            return QSV_G6;
        case HB_CPU_PLATFORM_INTEL_ICL:
            return QSV_G7;
        case HB_CPU_PLATFORM_INTEL_TGL:
            return QSV_G8;
        default:
            return QSV_FU;
    }
}

/*
 * Determine whether a given mfxIMPL is hardware-accelerated.
 */
int hb_qsv_implementation_is_hardware(mfxIMPL implementation)
{
    return MFX_IMPL_BASETYPE(implementation) != MFX_IMPL_SOFTWARE;
}

int hb_qsv_available()
{
    if (is_hardware_disabled())
    {
        return 0;
    }

    static int init_done = 0;
    if (init_done == 0)
    {
        int result = hb_qsv_info_init();
        if (result != 0)
        {
            init_done = -1;
            hb_log("hb_qsv_available: hb_qsv_info_init failed");
            return -1;
        }
        init_done = 1;
    }
    else if (init_done == -1)
    {
        hb_log("hb_qsv_available: hb_qsv_info_init failed");
        return -1;
    }

    return ((hb_qsv_video_encoder_is_enabled(hb_qsv_get_adapter_index(), HB_VCODEC_QSV_H264) ? HB_VCODEC_QSV_H264 : 0) |
            (hb_qsv_video_encoder_is_enabled(hb_qsv_get_adapter_index(), HB_VCODEC_QSV_H265) ? HB_VCODEC_QSV_H265 : 0) |
            (hb_qsv_video_encoder_is_enabled(hb_qsv_get_adapter_index(), HB_VCODEC_QSV_H265_10BIT) ? HB_VCODEC_QSV_H265_10BIT : 0));
}

int hb_qsv_video_encoder_is_enabled(int adapter_index, int encoder)
{
    hb_qsv_adapter_details_t* details = hb_qsv_get_adapters_details_by_index(adapter_index);

    if (details)
    {
        switch (encoder)
        {
            case HB_VCODEC_QSV_H264:
                return details->hb_qsv_info_avc != NULL && details->hb_qsv_info_avc->available;
            case HB_VCODEC_QSV_H265_10BIT:
                if (hb_qsv_hardware_generation(hb_qsv_get_platform(adapter_index)) < QSV_G6)
                    return 0;
            case HB_VCODEC_QSV_H265:
                return details->hb_qsv_info_hevc != NULL && details->hb_qsv_info_hevc->available;
            default:
                return 0;
        }
    }
    else
    {
        return 0;
    }
}

int hb_qsv_audio_encoder_is_enabled(int encoder)
{
    switch (encoder)
    {
        default:
            return 0;
    }
}

static void init_video_param(mfxVideoParam *videoParam)
{
    if (videoParam == NULL)
    {
        return;
    }

    memset(videoParam, 0, sizeof(mfxVideoParam));
    videoParam->mfx.CodecId                 = MFX_CODEC_AVC;
    videoParam->mfx.CodecLevel              = MFX_LEVEL_UNKNOWN;
    videoParam->mfx.CodecProfile            = MFX_PROFILE_UNKNOWN;
    videoParam->mfx.RateControlMethod       = MFX_RATECONTROL_VBR;
    videoParam->mfx.TargetUsage             = MFX_TARGETUSAGE_BALANCED;
    videoParam->mfx.TargetKbps              = 5000;
    videoParam->mfx.GopOptFlag              = MFX_GOP_CLOSED;
    videoParam->mfx.FrameInfo.FourCC        = MFX_FOURCC_NV12;
    videoParam->mfx.FrameInfo.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
    videoParam->mfx.FrameInfo.PicStruct     = MFX_PICSTRUCT_PROGRESSIVE;
    videoParam->mfx.FrameInfo.FrameRateExtN = 25;
    videoParam->mfx.FrameInfo.FrameRateExtD = 1;
    videoParam->mfx.FrameInfo.Width         = 1920;
    videoParam->mfx.FrameInfo.CropW         = 1920;
    videoParam->mfx.FrameInfo.AspectRatioW  = 1;
    videoParam->mfx.FrameInfo.Height        = 1088;
    videoParam->mfx.FrameInfo.CropH         = 1080;
    videoParam->mfx.FrameInfo.AspectRatioH  = 1;
    videoParam->AsyncDepth                  = HB_QSV_ASYNC_DEPTH_DEFAULT;
    videoParam->IOPattern                   = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
}

static void init_ext_video_signal_info(mfxExtVideoSignalInfo *extVideoSignalInfo)
{
    if (extVideoSignalInfo == NULL)
    {
        return;
    }

    memset(extVideoSignalInfo, 0, sizeof(mfxExtVideoSignalInfo));
    extVideoSignalInfo->Header.BufferId          = MFX_EXTBUFF_VIDEO_SIGNAL_INFO;
    extVideoSignalInfo->Header.BufferSz          = sizeof(mfxExtVideoSignalInfo);
    extVideoSignalInfo->VideoFormat              = 5; // undefined
    extVideoSignalInfo->VideoFullRange           = 0; // TV range
    extVideoSignalInfo->ColourDescriptionPresent = 0; // don't write to bitstream
    extVideoSignalInfo->ColourPrimaries          = 2; // undefined
    extVideoSignalInfo->TransferCharacteristics  = 2; // undefined
    extVideoSignalInfo->MatrixCoefficients       = 2; // undefined
}

static void init_ext_coding_option(mfxExtCodingOption *extCodingOption)
{
    if (extCodingOption == NULL)
    {
        return;
    }

    memset(extCodingOption, 0, sizeof(mfxExtCodingOption));
    extCodingOption->Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
    extCodingOption->Header.BufferSz = sizeof(mfxExtCodingOption);
    extCodingOption->AUDelimiter     = MFX_CODINGOPTION_OFF;
    extCodingOption->PicTimingSEI    = MFX_CODINGOPTION_OFF;
    extCodingOption->CAVLC           = MFX_CODINGOPTION_OFF;
}

static void init_ext_coding_option2(mfxExtCodingOption2 *extCodingOption2)
{
    if (extCodingOption2 == NULL)
    {
        return;
    }

    memset(extCodingOption2, 0, sizeof(mfxExtCodingOption2));
    extCodingOption2->Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
    extCodingOption2->Header.BufferSz = sizeof(mfxExtCodingOption2);
    extCodingOption2->MBBRC           = MFX_CODINGOPTION_ON;
    extCodingOption2->ExtBRC          = MFX_CODINGOPTION_ON;
    extCodingOption2->Trellis         = MFX_TRELLIS_I|MFX_TRELLIS_P|MFX_TRELLIS_B;
    extCodingOption2->RepeatPPS       = MFX_CODINGOPTION_ON;
    extCodingOption2->BRefType        = MFX_B_REF_PYRAMID;
    extCodingOption2->AdaptiveI       = MFX_CODINGOPTION_ON;
    extCodingOption2->AdaptiveB       = MFX_CODINGOPTION_ON;
    extCodingOption2->LookAheadDS     = MFX_LOOKAHEAD_DS_4x;
    extCodingOption2->NumMbPerSlice   = 2040; // 1920x1088/4
}

static int query_capabilities(mfxSession session, int index, mfxVersion version, hb_qsv_info_t *info)
{
    /*
     * MFXVideoENCODE_Query(mfxSession, mfxVideoParam *in, mfxVideoParam *out);
     *
     * Mode 1:
     * - in is NULL
     * - out has the parameters we want to query set to 1
     * - out->mfx.CodecId field has to be set (mandatory)
     * - MFXVideoENCODE_Query should zero out all unsupported parameters
     *
     * Mode 2:
     * - the parameters we want to query are set for in
     * - in ->mfx.CodecId field has to be set (mandatory)
     * - out->mfx.CodecId field has to be set (mandatory)
     * - MFXVideoENCODE_Query should sanitize all unsupported parameters
     */
    mfxStatus     status;
    hb_list_t    *mfxPluginList;
    mfxExtBuffer *videoExtParam[1];
    mfxVideoParam videoParam, inputParam;
    mfxExtCodingOption    extCodingOption;
    mfxExtCodingOption2   extCodingOption2;
    mfxExtVideoSignalInfo extVideoSignalInfo;

    /* Reset capabilities before querying */
    info->capabilities = 0;

    /* Load required MFX plug-ins */
    if ((mfxPluginList = hb_qsv_load_plugins(index, info, session, version)) == NULL)
    {
        return 0; // the required plugin(s) couldn't be loaded
    }

    /*
     * First of all, check availability of an encoder for
     * this combination of a codec ID and implementation.
     *
     * Note: can error out rather than sanitizing
     * unsupported codec IDs, so don't log errors.
     */
    if (HB_CHECK_MFX_VERSION(version, HB_QSV_MINVERSION_MAJOR, HB_QSV_MINVERSION_MINOR))
    {
        if (info->implementation & MFX_IMPL_AUDIO)
        {
            /* Not yet supported */
            return 0;
        }
        else
        {
            mfxStatus mfxRes;
            init_video_param(&inputParam);
            inputParam.mfx.CodecId = info->codec_id;

            memset(&videoParam, 0, sizeof(mfxVideoParam));
            videoParam.mfx.CodecId = inputParam.mfx.CodecId;

            mfxRes = MFXVideoENCODE_Query(session, &inputParam, &videoParam);
            if (mfxRes >= MFX_ERR_NONE &&
                videoParam.mfx.CodecId == info->codec_id)
            {
                /*
                 * MFXVideoENCODE_Query might tell you that an HEVC encoder is
                 * available on Haswell hardware, but it'll fail to initialize.
                 * So check encoder availability with MFXVideoENCODE_Init too.
                 */
                if ((status = MFXVideoENCODE_Init(session, &videoParam)) >= MFX_ERR_NONE)
                {
                    info->available = 1;
                }
                else if (info->codec_id == MFX_CODEC_AVC)
                {
                    /*
                     * This should not fail for AVC encoders, so we want to know
                     * about it - however, it may fail for other encoders (ignore)
                     */
                    fprintf(stderr,
                            "query_capabilities: MFXVideoENCODE_Init failed"
                            " (0x%"PRIX32", 0x%"PRIX32", %d)\n",
                            info->codec_id, info->implementation, status);
                }
                MFXVideoENCODE_Close(session);
            }
        }
    }
    if (!info->available)
    {
        /* Don't check capabilities for unavailable encoders */
        return 0;
    }

    if (info->implementation & MFX_IMPL_AUDIO)
    {
        /* We don't have any audio capability checks yet */
        return 0;
    }
    else
    {
        /* Implementation-specific features that can't be queried */
        if (info->codec_id == MFX_CODEC_AVC || info->codec_id == MFX_CODEC_HEVC)
        {
            if (hb_qsv_implementation_is_hardware(info->implementation))
            {
                if (hb_qsv_hardware_generation(hb_qsv_get_platform(index)) >= QSV_G3)
                {
                    info->capabilities |= HB_QSV_CAP_B_REF_PYRAMID;
                }
                if (info->codec_id == MFX_CODEC_HEVC &&
                    (hb_qsv_hardware_generation(hb_qsv_get_platform(index)) >= QSV_G7))
                {
                    info->capabilities |= HB_QSV_CAP_LOWPOWER_ENCODE;
                }
            }
            else
            {
                if (HB_CHECK_MFX_VERSION(version, 1, 6))
                {
                    info->capabilities |= HB_QSV_CAP_B_REF_PYRAMID;
                }
            }
        }

        /* API-specific features that can't be queried */
        if (HB_CHECK_MFX_VERSION(version, 1, 6))
        {
            // API >= 1.6 (mfxBitstream::DecodeTimeStamp, mfxExtCodingOption2)
            info->capabilities |= HB_QSV_CAP_MSDK_API_1_6;
        }

        /*
         * Check availability of optional rate control methods.
         *
         * Mode 2 tends to error out, but mode 1 gives false negatives, which
         * is worse. So use mode 2 and assume an error means it's unsupported.
         *
         * Also assume that LA and ICQ combined imply LA_ICQ is
         * supported, so we don't need to check the latter too.
         */
        if (HB_CHECK_MFX_VERSION(version, 1, 7))
        {
            init_video_param(&inputParam);
            inputParam.mfx.CodecId           = info->codec_id;
            inputParam.mfx.RateControlMethod = MFX_RATECONTROL_LA;
            inputParam.mfx.TargetKbps        = 5000;

            memset(&videoParam, 0, sizeof(mfxVideoParam));
            videoParam.mfx.CodecId = inputParam.mfx.CodecId;

            if (MFXVideoENCODE_Query(session, &inputParam, &videoParam) >= MFX_ERR_NONE &&
                videoParam.mfx.RateControlMethod == MFX_RATECONTROL_LA)
            {
                info->capabilities |= HB_QSV_CAP_RATECONTROL_LA;

                // also check for LA + interlaced support
                init_video_param(&inputParam);
                inputParam.mfx.CodecId             = info->codec_id;
                inputParam.mfx.RateControlMethod   = MFX_RATECONTROL_LA;
                inputParam.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_FIELD_TFF;
                inputParam.mfx.TargetKbps          = 5000;

                memset(&videoParam, 0, sizeof(mfxVideoParam));
                videoParam.mfx.CodecId = inputParam.mfx.CodecId;

                if (MFXVideoENCODE_Query(session, &inputParam, &videoParam) >= MFX_ERR_NONE &&
                    videoParam.mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_FIELD_TFF           &&
                    videoParam.mfx.RateControlMethod   == MFX_RATECONTROL_LA)
                {
                    info->capabilities |= HB_QSV_CAP_RATECONTROL_LAi;
                }
            }
        }
        if (HB_CHECK_MFX_VERSION(version, 1, 8))
        {
            init_video_param(&inputParam);
            inputParam.mfx.CodecId           = info->codec_id;
            inputParam.mfx.RateControlMethod = MFX_RATECONTROL_ICQ;
            inputParam.mfx.ICQQuality        = 20;

            memset(&videoParam, 0, sizeof(mfxVideoParam));
            videoParam.mfx.CodecId = inputParam.mfx.CodecId;

            if (MFXVideoENCODE_Query(session, &inputParam, &videoParam) >= MFX_ERR_NONE &&
                videoParam.mfx.RateControlMethod == MFX_RATECONTROL_ICQ)
            {
                info->capabilities |= HB_QSV_CAP_RATECONTROL_ICQ;
            }
        }

        /*
         * Determine whether mfxExtVideoSignalInfo is supported.
         */
        if (HB_CHECK_MFX_VERSION(version, 1, 3))
        {
            init_video_param(&videoParam);
            videoParam.mfx.CodecId = info->codec_id;

            init_ext_video_signal_info(&extVideoSignalInfo);
            videoParam.ExtParam    = videoExtParam;
            videoParam.ExtParam[0] = (mfxExtBuffer*)&extVideoSignalInfo;
            videoParam.NumExtParam = 1;

            status = MFXVideoENCODE_Query(session, NULL, &videoParam);
            if (status >= MFX_ERR_NONE)
            {
                /* Encoder can be configured via mfxExtVideoSignalInfo */
                info->capabilities |= HB_QSV_CAP_VUI_VSINFO;
            }
            else if (info->codec_id == MFX_CODEC_AVC)
            {
                /*
                 * This should not fail for AVC encoders, so we want to know
                 * about it - however, it may fail for other encoders (ignore)
                 */
                fprintf(stderr,
                        "query_capabilities: mfxExtVideoSignalInfo check"
                        " failed (0x%"PRIX32", 0x%"PRIX32", %d)\n",
                        info->codec_id, info->implementation, status);
            }
        }

        /*
         * Determine whether mfxExtCodingOption is supported.
         */
        if (HB_CHECK_MFX_VERSION(version, 1, 0))
        {
            init_video_param(&videoParam);
            videoParam.mfx.CodecId = info->codec_id;

            init_ext_coding_option(&extCodingOption);
            videoParam.ExtParam    = videoExtParam;
            videoParam.ExtParam[0] = (mfxExtBuffer*)&extCodingOption;
            videoParam.NumExtParam = 1;

            status = MFXVideoENCODE_Query(session, NULL, &videoParam);
            if (status >= MFX_ERR_NONE)
            {
                /* Encoder can be configured via mfxExtCodingOption */
                info->capabilities |= HB_QSV_CAP_OPTION1;
            }
            else if (info->codec_id == MFX_CODEC_AVC)
            {
                /*
                 * This should not fail for AVC encoders, so we want to know
                 * about it - however, it may fail for other encoders (ignore)
                 */
                fprintf(stderr,
                        "query_capabilities: mfxExtCodingOption check"
                        " failed (0x%"PRIX32", 0x%"PRIX32", %d)\n",
                        info->codec_id, info->implementation, status);
            }
        }

        /*
         * Determine whether mfxExtCodingOption2 and its fields are supported.
         *
         * Mode 2 suffers from false negatives with some drivers, whereas mode 1
         * suffers from false positives instead. The latter is probably easier
         * and/or safer to sanitize for us, so use mode 1.
         */
        if (HB_CHECK_MFX_VERSION(version, 1, 6) && info->codec_id == MFX_CODEC_AVC)
        {
            init_video_param(&videoParam);
            videoParam.mfx.CodecId = info->codec_id;

            init_ext_coding_option2(&extCodingOption2);
            videoParam.ExtParam    = videoExtParam;
            videoParam.ExtParam[0] = (mfxExtBuffer*)&extCodingOption2;
            videoParam.NumExtParam = 1;

            status = MFXVideoENCODE_Query(session, NULL, &videoParam);
            if (status >= MFX_ERR_NONE)
            {
#if 0
                // testing code that could come in handy
                fprintf(stderr, "-------------------\n");
                fprintf(stderr, "MBBRC:         0x%02X\n",     extCodingOption2.MBBRC);
                fprintf(stderr, "ExtBRC:        0x%02X\n",     extCodingOption2.ExtBRC);
                fprintf(stderr, "Trellis:       0x%02X\n",     extCodingOption2.Trellis);
                fprintf(stderr, "RepeatPPS:     0x%02X\n",     extCodingOption2.RepeatPPS);
                fprintf(stderr, "BRefType:      %4"PRIu16"\n", extCodingOption2.BRefType);
                fprintf(stderr, "AdaptiveI:     0x%02X\n",     extCodingOption2.AdaptiveI);
                fprintf(stderr, "AdaptiveB:     0x%02X\n",     extCodingOption2.AdaptiveB);
                fprintf(stderr, "LookAheadDS:   %4"PRIu16"\n", extCodingOption2.LookAheadDS);
                fprintf(stderr, "-------------------\n");
#endif

                /* Encoder can be configured via mfxExtCodingOption2 */
                info->capabilities |= HB_QSV_CAP_OPTION2;

                /*
                 * Sanitize API 1.6 fields:
                 *
                 * - MBBRC  requires G3 hardware (Haswell or equivalent)
                 * - ExtBRC requires G2 hardware (Ivy Bridge or equivalent)
                 */
                if (hb_qsv_implementation_is_hardware(info->implementation) &&
                    hb_qsv_hardware_generation(hb_qsv_get_platform(index)) >= QSV_G3)
                {
                    if (extCodingOption2.MBBRC)
                    {
                        info->capabilities |= HB_QSV_CAP_OPTION2_MBBRC;
                    }
                }
                if (hb_qsv_implementation_is_hardware(info->implementation) &&
                    hb_qsv_hardware_generation(hb_qsv_get_platform(index)) >= QSV_G2)
                {
                    if (extCodingOption2.ExtBRC)
                    {
                        info->capabilities |= HB_QSV_CAP_OPTION2_EXTBRC;
                    }
                }

                /*
                 * Sanitize API 1.7 fields:
                 *
                 * - Trellis requires G3 hardware (Haswell or equivalent)
                 */
                if (HB_CHECK_MFX_VERSION(version, 1, 7))
                {
                    if (hb_qsv_implementation_is_hardware(info->implementation) &&
                        hb_qsv_hardware_generation(hb_qsv_get_platform(index)) >= QSV_G3)
                    {
                        if (extCodingOption2.Trellis)
                        {
                            info->capabilities |= HB_QSV_CAP_OPTION2_TRELLIS;
                        }
                    }
                }

                /*
                 * Sanitize API 1.8 fields:
                 *
                 * - BRefType    requires B-pyramid support
                 * - LookAheadDS requires lookahead support
                 * - AdaptiveI, AdaptiveB, NumMbPerSlice unknown (trust Query)
                 */
                if (HB_CHECK_MFX_VERSION(version, 1, 8))
                {
                    if (info->capabilities & HB_QSV_CAP_B_REF_PYRAMID)
                    {
                        if (extCodingOption2.BRefType)
                        {
                            info->capabilities |= HB_QSV_CAP_OPTION2_BREFTYPE;
                        }
                    }
                    if (info->capabilities & HB_QSV_CAP_RATECONTROL_LA)
                    {
                        if (extCodingOption2.LookAheadDS)
                        {
                            info->capabilities |= HB_QSV_CAP_OPTION2_LA_DOWNS;
                        }
                    }
                    if (extCodingOption2.AdaptiveI && extCodingOption2.AdaptiveB)
                    {
                        info->capabilities |= HB_QSV_CAP_OPTION2_IB_ADAPT;
                    }
                    if (extCodingOption2.NumMbPerSlice)
                    {
                        info->capabilities |= HB_QSV_CAP_OPTION2_NMPSLICE;
                    }
                }
            }
            else
            {
                fprintf(stderr,
                        "query_capabilities: mfxExtCodingOption2 check failed (0x%"PRIX32", 0x%"PRIX32", %d)\n",
                        info->codec_id, info->implementation, status);
            }
        }
        if (HB_CHECK_MFX_VERSION(version, 1, 19))
        {
            if (hb_qsv_hardware_generation(hb_qsv_get_platform(index)) >= QSV_G7)
            {
                info->capabilities |= HB_QSV_CAP_VPP_SCALING;
            }
        }
        if (HB_CHECK_MFX_VERSION(version, 1, 33))
        {
            if (hb_qsv_hardware_generation(hb_qsv_get_platform(index)) >= QSV_G7)
            {
                info->capabilities |= HB_QSV_CAP_VPP_INTERPOLATION;
            }
        }
    }

    /* Unload MFX plug-ins */
    hb_qsv_unload_plugins(&mfxPluginList, session, version);

    return 0;
}

const char * DRM_INTEL_DRIVER_NAME = "i915";
const char * VA_INTEL_DRIVER_NAMES[] = { "iHD", "i965", NULL};

hb_display_t * hb_qsv_display_init(void)
{
    return hb_display_init(DRM_INTEL_DRIVER_NAME, VA_INTEL_DRIVER_NAMES);
}

#if defined(_WIN32) || defined(__MINGW32__)
static int hb_qsv_query_adapters(mfxAdaptersInfo* adapters_info);
static int hb_qsv_make_adapters_list(const mfxAdaptersInfo* adapters_info, hb_list_t **qsv_adapters_list);
static int hb_qsv_make_adapters_details_list(const mfxAdaptersInfo* adapters_info, hb_list_t **hb_qsv_adapter_details_list);
#endif

mfxIMPL hb_qsv_dx_index_to_impl(int dx_index)
{
    mfxIMPL impl;

    switch (dx_index)
    {
        {
        case 0:
            impl = MFX_IMPL_HARDWARE;
            break;
        case 1:
            impl = MFX_IMPL_HARDWARE2;
            break;
        case 2:
            impl = MFX_IMPL_HARDWARE3;
            break;
        case 3:
            impl = MFX_IMPL_HARDWARE4;
            break;

        default:
            // try searching on all display adapters
            impl = MFX_IMPL_HARDWARE_ANY;
            break;
        }
    }
    return impl;
}

static int hb_qsv_collect_adapters_details(hb_list_t *qsv_adapters_list, hb_list_t *hb_qsv_adapter_details_list)
{
    for (int i = 0; i < hb_list_count(hb_qsv_adapter_details_list); i++)
    {
        int *dx_index = (int *)hb_list_item(qsv_adapters_list, i);
        hb_qsv_adapter_details_t *details = hb_list_item(hb_qsv_adapter_details_list, i);
        details->index = *dx_index;
        /*
        * First, check for any MSDK version to determine whether one or
        * more implementations are present; then check if we can use them.
        *
        * I've had issues using a NULL version with some combinations of
        * hardware and driver, so use a low version number (1.0) instead.
        */
        mfxSession session;
        mfxVersion version = { .Major = 1, .Minor = 0, };
#if defined(_WIN32) || defined(__MINGW32__)
        mfxIMPL hw_preference = MFX_IMPL_VIA_D3D11;
#else
        mfxIMPL hw_preference = MFX_IMPL_VIA_ANY;
#endif
        // check for software fallback
        if (MFXInit(MFX_IMPL_SOFTWARE, &version, &session) == MFX_ERR_NONE)
        {
            // Media SDK software found, but check that our minimum is supported
            MFXQueryVersion(session, &details->qsv_software_version);
            if (HB_CHECK_MFX_VERSION(details->qsv_software_version,
                                    HB_QSV_MINVERSION_MAJOR,
                                    HB_QSV_MINVERSION_MINOR))
            {
                query_capabilities(session, details->index, details->qsv_software_version, &details->qsv_software_info_avc);
                query_capabilities(session, details->index, details->qsv_software_version, &details->qsv_software_info_hevc);
                // now that we know which hardware encoders are
                // available, we can set the preferred implementation
                qsv_impl_set_preferred(details, "software");
            }
            MFXClose(session);
        }
        // check for actual hardware support
        do{
            if (MFXInit(hb_qsv_dx_index_to_impl(*dx_index) | hw_preference, &version, &session) == MFX_ERR_NONE)
            {
                // On linux, the handle to the VA display must be set.
                // This code is essentiall a NOP other platforms.
                hb_display_t * display = hb_qsv_display_init();

                if (display != NULL)
                {
                    MFXVideoCORE_SetHandle(session, display->mfxType,
                                        (mfxHDL)display->handle);
                }
                // Media SDK hardware found, but check that our minimum is supported
                //
                // Note: this-party hardware (QSV_G0) is unsupported for the time being
                MFXQueryVersion(session, &details->qsv_hardware_version);
                if (hb_qsv_hardware_generation(hb_qsv_get_platform(*dx_index)) >= QSV_G1 &&
                    HB_CHECK_MFX_VERSION(details->qsv_hardware_version,
                                        HB_QSV_MINVERSION_MAJOR,
                                        HB_QSV_MINVERSION_MINOR))
                {
                    query_capabilities(session, details->index, details->qsv_hardware_version, &details->qsv_hardware_info_avc);
                    details->qsv_hardware_info_avc.implementation = hb_qsv_dx_index_to_impl(*dx_index) | hw_preference;
                    query_capabilities(session, details->index, details->qsv_hardware_version, &details->qsv_hardware_info_hevc);
                    details->qsv_hardware_info_hevc.implementation = hb_qsv_dx_index_to_impl(*dx_index) | hw_preference;
                    // now that we know which hardware encoders are
                    // available, we can set the preferred implementation
                    qsv_impl_set_preferred(details, "hardware");
                }
                hb_display_close(&display);
                MFXClose(session);
                hw_preference = 0;
            }
            else
            {
#if defined(_WIN32) || defined(__MINGW32__)
                // Windows only: After D3D11 we will try D3D9
                if (hw_preference == MFX_IMPL_VIA_D3D11)
                    hw_preference = MFX_IMPL_VIA_D3D9;
                else
#endif
                    hw_preference = 0;
            }
        }
        while(hw_preference != 0);
    }
    return 0;
}

static void log_decoder_capabilities(const int log_level, const hb_qsv_adapter_details_t *adapter_details, const char *prefix)
{
    char buffer[128] = "";

    if (hb_qsv_decode_h264_is_supported(adapter_details->index))
    {
        strcat(buffer, " h264");
    }

    if (hb_qsv_decode_h265_10_bit_is_supported(adapter_details->index))
    {
        strcat(buffer, " hevc (8bit: yes, 10bit: yes)");
    }
    else if (hb_qsv_decode_h265_is_supported(adapter_details->index))
    {
        strcat(buffer, " hevc (8bit: yes, 10bit: no)");
    }

    if (hb_qsv_decode_av1_is_supported(adapter_details->index))
    {
        strcat(buffer, " av1 (8bit: yes, 10bit: yes)");
    }

    hb_deep_log(log_level, "%s%s", prefix,
                strnlen(buffer, 1) ? buffer : " no decode support");
}

static void log_encoder_capabilities(const int log_level, const uint64_t caps, const char *prefix)
{
    /*
     * Note: keep the string short, as it may be logged by default.
     */
    char buffer[128] = "";

    if (caps & HB_QSV_CAP_LOWPOWER_ENCODE)
    {
        strcat(buffer, " lowpower");
    }
    /* B-Pyramid, with or without direct control (BRefType) */
    if (caps & HB_QSV_CAP_B_REF_PYRAMID)
    {
        if (caps & HB_QSV_CAP_OPTION2_BREFTYPE)
        {
            strcat(buffer, " breftype");
        }
        else
        {
            strcat(buffer, " bpyramid");
        }
    }
    /* Rate control: ICQ, lookahead (options: interlaced, downsampling) */
    if (caps & HB_QSV_CAP_RATECONTROL_LA)
    {
        if (caps & HB_QSV_CAP_RATECONTROL_ICQ)
        {
            strcat(buffer, " icq+la");
        }
        else
        {
            strcat(buffer, " la");
        }
        if (caps & HB_QSV_CAP_RATECONTROL_LAi)
        {
            strcat(buffer, "+i");
        }
        if (caps & HB_QSV_CAP_OPTION2_LA_DOWNS)
        {
            strcat(buffer, "+downs");
        }
    }
    else if (caps & HB_QSV_CAP_RATECONTROL_ICQ)
    {
        strcat(buffer, " icq");
    }
    if (caps & HB_QSV_CAP_VUI_VSINFO)
    {
        strcat(buffer, " vsinfo");
    }
    if (caps & HB_QSV_CAP_OPTION1)
    {
        strcat(buffer, " opt1");
    }
    if (caps & HB_QSV_CAP_OPTION2)
    {
        {
            strcat(buffer, " opt2");
        }
        if (caps & HB_QSV_CAP_OPTION2_MBBRC)
        {
            strcat(buffer, "+mbbrc");
        }
        if (caps & HB_QSV_CAP_OPTION2_EXTBRC)
        {
            strcat(buffer, "+extbrc");
        }
        if (caps & HB_QSV_CAP_OPTION2_TRELLIS)
        {
            strcat(buffer, "+trellis");
        }
        if (caps & HB_QSV_CAP_OPTION2_IB_ADAPT)
        {
            strcat(buffer, "+ib_adapt");
        }
        if (caps & HB_QSV_CAP_OPTION2_NMPSLICE)
        {
            strcat(buffer, "+nmpslice");
        }
    }

    hb_deep_log(log_level, "%s%s", prefix,
                strnlen(buffer, 1) ? buffer : " standard feature set");
}

static void hb_qsv_adapter_info_print(const hb_qsv_adapter_details_t *adapter_details)
{
    if (adapter_details->qsv_hardware_version.Version)
    {
        hb_log(" - Intel Media SDK hardware: API %"PRIu16".%"PRIu16" (minimum: %"PRIu16".%"PRIu16")",
                adapter_details->qsv_hardware_version.Major, adapter_details->qsv_hardware_version.Minor,
                HB_QSV_MINVERSION_MAJOR, HB_QSV_MINVERSION_MINOR);
    }

    if (adapter_details->qsv_software_version.Version)
    {
        hb_deep_log(3, " - Intel Media SDK software: API %"PRIu16".%"PRIu16" (minimum: %"PRIu16".%"PRIu16")",
                adapter_details->qsv_software_version.Major, adapter_details->qsv_software_version.Minor,
                HB_QSV_MINVERSION_MAJOR, HB_QSV_MINVERSION_MINOR);
    }

    log_decoder_capabilities(1, adapter_details, " - Decode support: ");

    if (adapter_details->hb_qsv_info_avc != NULL && adapter_details->hb_qsv_info_avc->available)
    {
        hb_log(" - H.264 encoder: yes");
        hb_log("    - preferred implementation: %s %s",
                hb_qsv_impl_get_name(adapter_details->hb_qsv_info_avc->implementation),
                hb_qsv_impl_get_via_name(adapter_details->hb_qsv_info_avc->implementation));
        if (adapter_details->qsv_hardware_info_avc.available)
        {
            log_encoder_capabilities(1, adapter_details->qsv_hardware_info_avc.capabilities,
                                "    - capabilities (hardware): ");
        }
        if (adapter_details->qsv_software_info_avc.available)
        {
            log_encoder_capabilities(3, adapter_details->qsv_software_info_avc.capabilities,
                                "    - capabilities (software): ");
        }
    }
    else
    {
        hb_log(" - H.264 encoder: no");
    }
    if (adapter_details->hb_qsv_info_hevc != NULL && adapter_details->hb_qsv_info_hevc->available)
    {
        hb_log(" - H.265 encoder: yes (8bit: yes, 10bit: %s)", (hb_qsv_hardware_generation(hb_qsv_get_platform(adapter_details->index)) < QSV_G6) ? "no" : "yes" );
        hb_log("    - preferred implementation: %s %s",
                hb_qsv_impl_get_name(adapter_details->hb_qsv_info_hevc->implementation),
                hb_qsv_impl_get_via_name(adapter_details->hb_qsv_info_hevc->implementation));
        if (adapter_details->qsv_hardware_info_hevc.available)
        {
            log_encoder_capabilities(1, adapter_details->qsv_hardware_info_hevc.capabilities,
                                "    - capabilities (hardware): ");
        }
        if (adapter_details->qsv_software_info_hevc.available)
        {
            log_encoder_capabilities(3, adapter_details->qsv_software_info_hevc.capabilities,
                                "    - capabilities (software): ");
        }
    }
    else
    {
        hb_log(" - H.265 encoder: no");
    }
}

void hb_qsv_info_print()
{
    // is QSV available and usable?
    if (hb_qsv_available())
    {
#if defined(_WIN32) || defined(__MINGW32__)
        if (g_qsv_adapters_list && hb_list_count(g_qsv_adapters_list))
        {
            char gpu_list_str[256] = "";
            for (int i = 0; i < hb_list_count(g_qsv_adapters_list); i++)
            {
                char value_str[256];
                int *value = hb_list_item(g_qsv_adapters_list, i);
                sprintf(value_str, "%d", *value);
                if (i > 0)
                    strcat(gpu_list_str, ", ");
                strcat(gpu_list_str, value_str);
            }
            hb_log("Intel Quick Sync Video support: yes, gpu list: %s", gpu_list_str);
        }
        else
#endif
        {
            hb_log("Intel Quick Sync Video support: yes");
        }
        // also print the details about all QSV adapters
        for (int i = 0; i < hb_list_count(g_qsv_adapters_details_list); i++)
        {
            const hb_qsv_adapter_details_t *details = hb_list_item(g_qsv_adapters_details_list, i);
#if defined(_WIN32) || defined(__MINGW32__)
            mfxAdapterInfo* info = &g_qsv_adapters_info.Adapters[i];
            hb_log("Intel Quick Sync Video %s adapter with index %d", hb_qsv_get_adapter_type(info), details->index);
#endif
            hb_qsv_adapter_info_print(details);
        }
    }
    else
    {
        hb_log("Intel Quick Sync Video support: no");
    }
}

hb_qsv_info_t* hb_qsv_encoder_info_get(int adapter_index, int encoder)
{
    hb_qsv_adapter_details_t* details = hb_qsv_get_adapters_details_by_index(adapter_index);

    if (details)
    {
        switch (encoder)
        {
            case HB_VCODEC_QSV_H264:
                return details->hb_qsv_info_avc;
            case HB_VCODEC_QSV_H265_10BIT:
            case HB_VCODEC_QSV_H265:
                return details->hb_qsv_info_hevc;
            default:
                return NULL;
        }
    }
    return NULL;
}

hb_list_t* hb_qsv_load_plugins(int index, hb_qsv_info_t *info, mfxSession session, mfxVersion version)
{
    hb_list_t *mfxPluginList = hb_list_init();
    if (mfxPluginList == NULL)
    {
        hb_log("hb_qsv_load_plugins: hb_list_init() failed");
        goto fail;
    }

    if (HB_CHECK_MFX_VERSION(version, 1, 8))
    {
        if (info->codec_id == MFX_CODEC_HEVC && !(hb_qsv_hardware_generation(hb_qsv_get_platform(index)) < QSV_G5))
        {
            if (HB_CHECK_MFX_VERSION(version, 1, 15) &&
                hb_qsv_implementation_is_hardware(info->implementation))
            {
                if (MFXVideoUSER_Load(session, &MFX_PLUGINID_HEVCE_HW, 0) == MFX_ERR_NONE)
                {
                    hb_list_add(mfxPluginList, (void*)&MFX_PLUGINID_HEVCE_HW);
                }
            }
            else if (HB_CHECK_MFX_VERSION(version, 1, 15))
            {
                if (MFXVideoUSER_Load(session, &MFX_PLUGINID_HEVCE_SW, 0) == MFX_ERR_NONE)
                {
                    hb_list_add(mfxPluginList, (void*)&MFX_PLUGINID_HEVCE_SW);
                }
            }
        }
    }

    return mfxPluginList;

fail:
    hb_list_close(&mfxPluginList);
    return NULL;
}

void hb_qsv_unload_plugins(hb_list_t **_l, mfxSession session, mfxVersion version)
{
    mfxPluginUID *pluginUID;
    hb_list_t *mfxPluginList = *_l;

    if (mfxPluginList != NULL && HB_CHECK_MFX_VERSION(version, 1, 8))
    {
        for (int i = 0; i < hb_list_count(mfxPluginList); i++)
        {
            if ((pluginUID = hb_list_item(mfxPluginList, i)) != NULL)
            {
                MFXVideoUSER_UnLoad(session, pluginUID);
            }
        }
    }
    hb_list_close(_l);
}

const char* hb_qsv_decode_get_codec_name(enum AVCodecID codec_id)
{
    switch (codec_id)
    {
        case AV_CODEC_ID_H264:
            return "h264_qsv";

        case AV_CODEC_ID_HEVC:
            return "hevc_qsv";

        case AV_CODEC_ID_MPEG2VIDEO:
            return "mpeg2_qsv";

        case AV_CODEC_ID_AV1:
            return "av1_qsv";

        default:
            return NULL;
    }
}

int hb_qsv_decode_h264_is_supported(int adapter_index)
{
    return hb_qsv_hardware_generation(hb_qsv_get_platform(adapter_index)) >= QSV_G1;
}

int hb_qsv_decode_h265_is_supported(int adapter_index)
{
    return hb_qsv_hardware_generation(hb_qsv_get_platform(adapter_index)) >= QSV_G5;
}

int hb_qsv_decode_h265_10_bit_is_supported(int adapter_index)
{
    return hb_qsv_hardware_generation(hb_qsv_get_platform(adapter_index)) >= QSV_G6;
}

int hb_qsv_decode_av1_is_supported(int adapter_index)
{
    return hb_qsv_hardware_generation(hb_qsv_get_platform(adapter_index)) >= QSV_G8;
}

int hb_qsv_decode_codec_supported_codec(int adapter_index, int video_codec_param, int pix_fmt)
{
    switch (video_codec_param)
    {
        case AV_CODEC_ID_H264:
            if (pix_fmt == AV_PIX_FMT_NV12     ||
                pix_fmt == AV_PIX_FMT_YUV420P  ||
                pix_fmt == AV_PIX_FMT_YUVJ420P ||
                pix_fmt == AV_PIX_FMT_YUV420P10)
            {
                return hb_qsv_decode_h264_is_supported(adapter_index);
            }
            break;
        case AV_CODEC_ID_HEVC:
            if (pix_fmt == AV_PIX_FMT_NV12     ||
                pix_fmt == AV_PIX_FMT_YUV420P  ||
                pix_fmt == AV_PIX_FMT_YUVJ420P)
            {
                return hb_qsv_decode_h265_is_supported(adapter_index);
            }
            else if (pix_fmt == AV_PIX_FMT_P010LE ||
                pix_fmt == AV_PIX_FMT_YUV420P10)
            {
                return hb_qsv_decode_h265_10_bit_is_supported(adapter_index);
            }
            break;
        case AV_CODEC_ID_AV1:
            if (pix_fmt == AV_PIX_FMT_NV12     ||
                pix_fmt == AV_PIX_FMT_P010LE   ||
                pix_fmt == AV_PIX_FMT_YUV420P  ||
                pix_fmt == AV_PIX_FMT_YUVJ420P ||
                pix_fmt == AV_PIX_FMT_YUV420P10)
            {
                return hb_qsv_decode_av1_is_supported(adapter_index);
            }
            break;
        default:
            return 0;
    }
    return 0;
}

int hb_qsv_setup_job(hb_job_t *job)
{
#if defined(_WIN32) || defined(__MINGW32__)
    if (job->qsv.ctx && job->qsv.ctx->dx_index >= 0)
    {
        hb_qsv_param_parse_dx_index(job, job->qsv.ctx->dx_index);
    }
    hb_qsv_parse_adapter_index(job);
#endif
    int async_depth_default = hb_qsv_param_default_async_depth();
    if (job->qsv.async_depth <= 0 || job->qsv.async_depth > async_depth_default)
    {
        job->qsv.async_depth = async_depth_default;
    }
    // Make sure QSV Decode is only True if the selected QSV adapter supports decode.
    job->qsv.decode = job->qsv.decode && hb_qsv_available();
    return 0;
}

int hb_qsv_decode_is_enabled(hb_job_t *job)
{
    return ((job != NULL && job->qsv.decode) &&
            (job->title->video_decode_support & HB_DECODE_SUPPORT_QSV)) &&
            hb_qsv_decode_codec_supported_codec(hb_qsv_get_adapter_index(),
            job->title->video_codec_param, job->pix_fmt);
}

static int hb_dxva2_device_check();
static int hb_d3d11va_device_check();

int hb_qsv_hw_filters_are_enabled(hb_job_t *job)
{
    return job && job->qsv.ctx && job->qsv.ctx->qsv_filters_are_enabled;
}

int hb_qsv_is_enabled(hb_job_t *job)
{
    return hb_qsv_decode_is_enabled(job) || hb_qsv_encoder_info_get(hb_qsv_get_adapter_index(), job->vcodec);
}

int hb_qsv_full_path_is_enabled(hb_job_t *job)
{
    static int device_check_completed = 0;
    static int device_check_succeded = 0;
    int codecs_exceptions = 0;
    int qsv_full_path_is_enabled = 0;
    hb_qsv_info_t *info = hb_qsv_encoder_info_get(hb_qsv_get_adapter_index(), job->vcodec);

    if(!device_check_completed)
    {
       device_check_succeded = (hb_d3d11va_device_check() >= 0) ? 1 : 0;
       device_check_completed = 1;
    }

    codecs_exceptions = (job->title->pix_fmt == AV_PIX_FMT_YUV420P10 && job->vcodec == HB_VCODEC_QSV_H264);

    qsv_full_path_is_enabled = (hb_qsv_decode_is_enabled(job) &&
        info && hb_qsv_implementation_is_hardware(info->implementation) &&
        device_check_succeded && job->qsv.ctx && !job->qsv.ctx->num_cpu_filters) && !codecs_exceptions;
    return qsv_full_path_is_enabled;
}

int hb_qsv_copyframe_is_slow(int encoder)
{
    hb_qsv_info_t *info = hb_qsv_encoder_info_get(hb_qsv_get_adapter_index(), encoder);
    if (info != NULL && hb_qsv_implementation_is_hardware(info->implementation))
    {
        hb_qsv_adapter_details_t* details = hb_qsv_get_adapters_details_by_index(hb_qsv_get_adapter_index());
        if (details)
        {
            // we should really check the driver version, but since it's not
            // available, checking the API version is the best we can do :-(
            return !HB_CHECK_MFX_VERSION(details->qsv_hardware_version, 1, 7);
        }
        return 0;
    }
    return 0;
}

int hb_qsv_codingoption_xlat(int val)
{
    switch (HB_QSV_CLIP3(-1, 2, val))
    {
        case 0:
            return MFX_CODINGOPTION_OFF;
        case 1:
        case 2: // MFX_CODINGOPTION_ADAPTIVE, reserved
            return MFX_CODINGOPTION_ON;
        case -1:
        default:
            return MFX_CODINGOPTION_UNKNOWN;
    }
}

int hb_qsv_trellisvalue_xlat(int val)
{
    switch (HB_QSV_CLIP3(0, 3, val))
    {
        case 0:
            return MFX_TRELLIS_OFF;
        case 1: // I-frames only
            return MFX_TRELLIS_I;
        case 2: // I- and P-frames
            return MFX_TRELLIS_I|MFX_TRELLIS_P;
        case 3: // all frames
            return MFX_TRELLIS_I|MFX_TRELLIS_P|MFX_TRELLIS_B;
        default:
            return MFX_TRELLIS_UNKNOWN;
    }
}

const char* hb_qsv_codingoption_get_name(int val)
{
    switch (val)
    {
        case MFX_CODINGOPTION_ON:
            return "on";
        case MFX_CODINGOPTION_OFF:
            return "off";
        case MFX_CODINGOPTION_ADAPTIVE:
            return "adaptive";
        case MFX_CODINGOPTION_UNKNOWN:
            return "unknown (auto)";
        default:
            return NULL;
    }
}

int hb_qsv_atoindex(const char* const *arr, const char *str, int *err)
{
    int i;
    for (i = 0; arr[i] != NULL; i++)
    {
        if (!strcasecmp(arr[i], str))
        {
            break;
        }
    }
    *err = (arr[i] == NULL);
    return i;
}

// adapted from libx264
int hb_qsv_atobool(const char *str, int *err)
{
    if (!strcasecmp(str,    "1") ||
        !strcasecmp(str,  "yes") ||
        !strcasecmp(str, "true"))
    {
        return 1;
    }
    if (!strcasecmp(str,     "0") ||
        !strcasecmp(str,    "no") ||
        !strcasecmp(str, "false"))
    {
        return 0;
    }
    *err = 1;
    return 0;
}

// adapted from libx264
int hb_qsv_atoi(const char *str, int *err)
{
    char *end;
    int v = strtol(str, &end, 0);
    if (end == str || end[0] != '\0')
    {
        *err = 1;
    }
    return v;
}

// adapted from libx264
float hb_qsv_atof(const char *str, int *err)
{
    char *end;
    float v = strtod(str, &end);
    if (end == str || end[0] != '\0')
    {
        *err = 1;
    }
    return v;
}

int hb_qsv_param_parse(hb_qsv_param_t *param, hb_qsv_info_t *info, hb_job_t *job,
                       const char *key, const char *value)
{
    float fvalue;
    int ivalue, error = 0;
    if (param == NULL || info == NULL)
    {
        return HB_QSV_PARAM_ERROR;
    }
    if (value == NULL || value[0] == '\0')
    {
        value = "true";
    }
    else if (value[0] == '=')
    {
        value++;
    }
    if (key == NULL || key[0] == '\0')
    {
        return HB_QSV_PARAM_BAD_NAME;
    }
    else if (!strncasecmp(key, "no-", 3))
    {
        key  += 3;
        value = hb_qsv_atobool(value, &error) ? "false" : "true";
        if (error)
        {
            return HB_QSV_PARAM_BAD_VALUE;
        }
    }
    if (!strcasecmp(key, "target-usage") ||
        !strcasecmp(key, "tu"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->videoParam->mfx.TargetUsage = HB_QSV_CLIP3(MFX_TARGETUSAGE_1,
                                                              MFX_TARGETUSAGE_7,
                                                              ivalue);
        }
    }
    else if (!strcasecmp(key, "num-ref-frame") ||
             !strcasecmp(key, "ref"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->videoParam->mfx.NumRefFrame = HB_QSV_CLIP3(0, 16, ivalue);
        }
    }
    else if (!strcasecmp(key, "gop-ref-dist"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->gop.gop_ref_dist = HB_QSV_CLIP3(-1, 32, ivalue);
        }
    }
    else if (!strcasecmp(key, "gop-pic-size") ||
             !strcasecmp(key, "keyint"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->gop.gop_pic_size = HB_QSV_CLIP3(-1, UINT16_MAX, ivalue);
        }
    }
    else if (!strcasecmp(key, "b-pyramid"))
    {
        if (info->capabilities & HB_QSV_CAP_B_REF_PYRAMID)
        {
            ivalue = hb_qsv_atoi(value, &error);
            if (!error)
            {
                param->gop.b_pyramid = HB_QSV_CLIP3(-1, 1, ivalue);
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "scenecut"))
    {
        ivalue = hb_qsv_atobool(value, &error);
        if (!error)
        {
            if (!ivalue)
            {
                param->videoParam->mfx.GopOptFlag |= MFX_GOP_STRICT;
            }
            else
            {
                param->videoParam->mfx.GopOptFlag &= ~MFX_GOP_STRICT;
            }
        }
    }
    else if (!strcasecmp(key, "adaptive-i") ||
             !strcasecmp(key, "i-adapt"))
    {
        if (info->capabilities & HB_QSV_CAP_OPTION2_IB_ADAPT)
        {
            ivalue = hb_qsv_atobool(value, &error);
            if (!error)
            {
                param->codingOption2.AdaptiveI = hb_qsv_codingoption_xlat(ivalue);
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "adaptive-b") ||
             !strcasecmp(key, "b-adapt"))
    {
        if (info->capabilities & HB_QSV_CAP_OPTION2_IB_ADAPT)
        {
            ivalue = hb_qsv_atobool(value, &error);
            if (!error)
            {
                param->codingOption2.AdaptiveB = hb_qsv_codingoption_xlat(ivalue);
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "force-cqp"))
    {
        ivalue = hb_qsv_atobool(value, &error);
        if (!error)
        {
            param->rc.icq = !ivalue;
        }
    }
    else if (!strcasecmp(key, "cqp-offset-i"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->rc.cqp_offsets[0] = HB_QSV_CLIP3(INT16_MIN, INT16_MAX, ivalue);
        }
    }
    else if (!strcasecmp(key, "cqp-offset-p"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->rc.cqp_offsets[1] = HB_QSV_CLIP3(INT16_MIN, INT16_MAX, ivalue);
        }
    }
    else if (!strcasecmp(key, "cqp-offset-b"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->rc.cqp_offsets[2] = HB_QSV_CLIP3(INT16_MIN, INT16_MAX, ivalue);
        }
    }
    else if (!strcasecmp(key, "vbv-init"))
    {
        fvalue = hb_qsv_atof(value, &error);
        if (!error)
        {
            param->rc.vbv_buffer_init = HB_QSV_CLIP3(0, UINT16_MAX, fvalue);
        }
    }
    else if (!strcasecmp(key, "vbv-bufsize"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->rc.vbv_buffer_size = HB_QSV_CLIP3(0, UINT16_MAX, ivalue);
        }
    }
    else if (!strcasecmp(key, "vbv-maxrate"))
    {
        ivalue = hb_qsv_atoi(value, &error);
        if (!error)
        {
            param->rc.vbv_max_bitrate = HB_QSV_CLIP3(0, UINT16_MAX, ivalue);
        }
    }
    else if (!strcasecmp(key, "cavlc") || !strcasecmp(key, "cabac"))
    {
        if (info->capabilities & HB_QSV_CAP_OPTION1)
        {
            switch (info->codec_id)
            {
                case MFX_CODEC_AVC:
                    ivalue = hb_qsv_atobool(value, &error);
                    break;
                default:
                    return HB_QSV_PARAM_UNSUPPORTED;
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            if (!strcasecmp(key, "cabac"))
            {
                ivalue = !ivalue;
            }
            param->codingOption.CAVLC = hb_qsv_codingoption_xlat(ivalue);
        }
    }
    else if (!strcasecmp(key, "videoformat"))
    {
        if (info->capabilities & HB_QSV_CAP_VUI_VSINFO)
        {
            switch (info->codec_id)
            {
                case MFX_CODEC_AVC:
                    ivalue = hb_qsv_atoindex(hb_h264_vidformat_names, value, &error);
                    break;
                case MFX_CODEC_HEVC:
                    ivalue = hb_qsv_atoindex(hb_h265_vidformat_names, value, &error);
                    break;
                default:
                    return HB_QSV_PARAM_UNSUPPORTED;
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoSignalInfo.VideoFormat = ivalue;
        }
    }
    else if (!strcasecmp(key, "fullrange"))
    {
        if (info->capabilities & HB_QSV_CAP_VUI_VSINFO)
        {
            switch (info->codec_id)
            {
                case MFX_CODEC_AVC:
                    ivalue = hb_qsv_atoindex(hb_h264_fullrange_names, value, &error);
                    break;
                case MFX_CODEC_HEVC:
                    ivalue = hb_qsv_atoindex(hb_h265_fullrange_names, value, &error);
                    break;
                default:
                    return HB_QSV_PARAM_UNSUPPORTED;
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoSignalInfo.VideoFullRange = ivalue;
        }
    }
    else if (!strcasecmp(key, "colorprim"))
    {
        if (info->capabilities & HB_QSV_CAP_VUI_VSINFO)
        {
            switch (info->codec_id)
            {
                case MFX_CODEC_AVC:
                    ivalue = hb_qsv_atoindex(hb_h264_colorprim_names, value, &error);
                    break;
                case MFX_CODEC_HEVC:
                    ivalue = hb_qsv_atoindex(hb_h265_colorprim_names, value, &error);
                    break;
                default:
                    return HB_QSV_PARAM_UNSUPPORTED;
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoSignalInfo.ColourDescriptionPresent = 1;
            param->videoSignalInfo.ColourPrimaries = ivalue;
        }
    }
    else if (!strcasecmp(key, "transfer"))
    {
        if (info->capabilities & HB_QSV_CAP_VUI_VSINFO)
        {
            switch (info->codec_id)
            {
                case MFX_CODEC_AVC:
                    ivalue = hb_qsv_atoindex(hb_h264_transfer_names, value, &error);
                    break;
                case MFX_CODEC_HEVC:
                    ivalue = hb_qsv_atoindex(hb_h265_transfer_names, value, &error);
                    break;
                default:
                    return HB_QSV_PARAM_UNSUPPORTED;
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoSignalInfo.ColourDescriptionPresent = 1;
            param->videoSignalInfo.TransferCharacteristics = ivalue;
        }
    }
    else if (!strcasecmp(key, "colormatrix"))
    {
        if (info->capabilities & HB_QSV_CAP_VUI_VSINFO)
        {
            switch (info->codec_id)
            {
                case MFX_CODEC_AVC:
                    ivalue = hb_qsv_atoindex(hb_h264_colmatrix_names, value, &error);
                    break;
                case MFX_CODEC_HEVC:
                    ivalue = hb_qsv_atoindex(hb_h265_colmatrix_names, value, &error);
                    break;
                default:
                    return HB_QSV_PARAM_UNSUPPORTED;
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoSignalInfo.ColourDescriptionPresent = 1;
            param->videoSignalInfo.MatrixCoefficients = ivalue;
        }
    }
    else if (!strcasecmp(key, "tff") ||
             !strcasecmp(key, "interlaced"))
    {
        switch (info->codec_id)
        {
            case MFX_CODEC_AVC:
                ivalue = hb_qsv_atobool(value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoParam->mfx.FrameInfo.PicStruct = (ivalue                  ?
                                                          MFX_PICSTRUCT_FIELD_TFF :
                                                          MFX_PICSTRUCT_PROGRESSIVE);
        }
    }
    else if (!strcasecmp(key, "bff"))
    {
        switch (info->codec_id)
        {
            case MFX_CODEC_AVC:
                ivalue = hb_qsv_atobool(value, &error);
                break;
            default:
                return HB_QSV_PARAM_UNSUPPORTED;
        }
        if (!error)
        {
            param->videoParam->mfx.FrameInfo.PicStruct = (ivalue                  ?
                                                          MFX_PICSTRUCT_FIELD_BFF :
                                                          MFX_PICSTRUCT_PROGRESSIVE);
        }
    }
    else if (!strcasecmp(key, "mbbrc"))
    {
        if (info->capabilities & HB_QSV_CAP_OPTION2_MBBRC)
        {
            ivalue = hb_qsv_atobool(value, &error);
            if (!error)
            {
                param->codingOption2.MBBRC = hb_qsv_codingoption_xlat(ivalue);
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "extbrc"))
    {
        if (info->capabilities & HB_QSV_CAP_OPTION2_EXTBRC)
        {
            ivalue = hb_qsv_atobool(value, &error);
            if (!error)
            {
                param->codingOption2.ExtBRC = hb_qsv_codingoption_xlat(ivalue);
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "lookahead") ||
             !strcasecmp(key, "la"))
    {
        if (info->capabilities & HB_QSV_CAP_RATECONTROL_LA)
        {
            ivalue = hb_qsv_atobool(value, &error);
            if (!error)
            {
                param->rc.lookahead = ivalue;
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "lookahead-depth") ||
             !strcasecmp(key, "la-depth"))
    {
        if (info->capabilities & HB_QSV_CAP_RATECONTROL_LA)
        {
            ivalue = hb_qsv_atoi(value, &error);
            if (!error)
            {
                param->codingOption2.LookAheadDepth = HB_QSV_CLIP3(10, 100,
                                                                   ivalue);
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "lookahead-ds") ||
             !strcasecmp(key, "la-ds"))
    {
        if (info->capabilities & HB_QSV_CAP_OPTION2_LA_DOWNS)
        {
            ivalue = hb_qsv_atoi(value, &error);
            if (!error)
            {
                param->codingOption2.LookAheadDS = HB_QSV_CLIP3(MFX_LOOKAHEAD_DS_UNKNOWN,
                                                                MFX_LOOKAHEAD_DS_4x,
                                                                ivalue);
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "trellis"))
    {
        if (info->capabilities & HB_QSV_CAP_OPTION2_TRELLIS)
        {
            ivalue = hb_qsv_atoi(value, &error);
            if (!error)
            {
                param->codingOption2.Trellis = hb_qsv_trellisvalue_xlat(ivalue);
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "lowpower"))
    {
        if (info->capabilities & HB_QSV_CAP_LOWPOWER_ENCODE)
        {
            ivalue = hb_qsv_atobool(value, &error);
            if (!error)
            {
                param->videoParam->mfx.LowPower = ivalue ? MFX_CODINGOPTION_ON : MFX_CODINGOPTION_OFF;
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
#if defined(_WIN32) || defined(__MINGW32__)
    else if (!strcasecmp(key, "gpu"))
    {
        // Check if was parsed already in decoder initialization
        if (job->qsv.ctx && !job->qsv.ctx->qsv_device)
        {
            int gpu_index = hb_qsv_atoi(value, &error);
            if (!error)
            {
                hb_qsv_param_parse_dx_index(job, gpu_index);
            }
        }
    }
#endif
    else if (!strcasecmp(key, "scalingmode") ||
             !strcasecmp(key, "vpp-sm"))
    {
        // Already parsed it in decoder but need to check support
        if (info->capabilities & HB_QSV_CAP_VPP_SCALING)
        {
            hb_triplet_t *mode = NULL;
            mode = hb_triplet4key(hb_qsv_vpp_scale_modes, value);
            if (!mode)
            {
                error = HB_QSV_PARAM_BAD_VALUE;
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else if (!strcasecmp(key, "interpolationmethod") ||
             !strcasecmp(key, "vpp-im"))
    {
        // Already parsed it in decoder but need to check support
        if (info->capabilities & HB_QSV_CAP_VPP_INTERPOLATION)
        {
            hb_triplet_t *method = NULL;
            method = hb_triplet4key(hb_qsv_vpp_interpolation_methods, value);
            if (!method)
            {
                error = HB_QSV_PARAM_BAD_VALUE;
            }
        }
        else
        {
            return HB_QSV_PARAM_UNSUPPORTED;
        }
    }
    else
    {
        /*
         * TODO:
         * - slice count (num-slice/slices, num-mb-per-slice/slice-max-mbs)
         * - open-gop
         * - fake-interlaced (mfxExtCodingOption.FramePicture???)
         * - intra-refresh
         */
        return HB_QSV_PARAM_BAD_NAME;
    }
    return error ? HB_QSV_PARAM_BAD_VALUE : HB_QSV_PARAM_OK;
}

int hb_qsv_profile_parse(hb_qsv_param_t *param, hb_qsv_info_t *info, const char *profile_key, const int codec)
{
    hb_triplet_t *profile = NULL;
    if (profile_key != NULL && *profile_key && strcasecmp(profile_key, "auto"))
    {
        switch (param->videoParam->mfx.CodecId)
        {
            case MFX_CODEC_AVC:
                profile = hb_triplet4key(hb_qsv_h264_profiles, profile_key);
                break;

            case MFX_CODEC_HEVC:
                profile = hb_triplet4key(hb_qsv_h265_profiles, profile_key);

                /* HEVC10 supported starting from KBL/G6 */
                if (profile->value == MFX_PROFILE_HEVC_MAIN10 &&
                    hb_qsv_hardware_generation(hb_qsv_get_platform(hb_qsv_get_adapter_index())) < QSV_G6)
                {
                    hb_log("qsv: HEVC Main10 is not supported on this platform");
                    profile = NULL;
                }

                break;

            default:
                break;
        }
        if (profile == NULL)
        {
            return -1;
        }
        param->videoParam->mfx.CodecProfile = profile->value;
    }
    /* HEVC 10 bits defaults to Main 10 */
    else if (((profile_key != NULL && !strcasecmp(profile_key, "auto")) || profile_key == NULL) &&
              codec == HB_VCODEC_QSV_H265_10BIT &&
              param->videoParam->mfx.CodecId == MFX_CODEC_HEVC &&
              hb_qsv_hardware_generation(hb_qsv_get_platform(hb_qsv_get_adapter_index())) >= QSV_G6)
    {
         profile = &hb_qsv_h265_profiles[1];
         param->videoParam->mfx.CodecProfile = profile->value;
    }
    return 0;
}

int hb_qsv_level_parse(hb_qsv_param_t *param, hb_qsv_info_t *info, const char *level_key)
{
    hb_triplet_t *level = NULL;
    if (level_key != NULL && *level_key && strcasecmp(level_key, "auto"))
    {
        switch (param->videoParam->mfx.CodecId)
        {
            case MFX_CODEC_AVC:
                level = hb_triplet4key(hb_qsv_h264_levels, level_key);
                break;

            case MFX_CODEC_HEVC:
                level = hb_triplet4key(hb_qsv_h265_levels, level_key);
                break;

            default:
                break;
        }
        if (level == NULL)
        {
            return -1;
        }
        if (param->videoParam->mfx.CodecId == MFX_CODEC_AVC)
        {
            if (info->capabilities & HB_QSV_CAP_MSDK_API_1_6)
            {
                param->videoParam->mfx.CodecLevel = FFMIN(MFX_LEVEL_AVC_52, level->value);
            }
            else
            {
                // Media SDK API < 1.6, MFX_LEVEL_AVC_52 unsupported
                param->videoParam->mfx.CodecLevel = FFMIN(MFX_LEVEL_AVC_51, level->value);
            }
        }
        else
        {
            param->videoParam->mfx.CodecLevel = level->value;
        }
    }
    return 0;
}

const char* const* hb_qsv_preset_get_names()
{
    if (hb_qsv_hardware_generation(hb_qsv_get_platform(hb_qsv_get_adapter_index())) >= QSV_G3)
    {
        return hb_qsv_preset_names2;
    }
    else
    {
        return hb_qsv_preset_names1;
    }
}

const char* const* hb_qsv_profile_get_names(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_QSV_H264:
            return hb_h264_profile_names_8bit;
        case HB_VCODEC_QSV_H265_8BIT:
            return hb_h265_profile_names_8bit;
        case HB_VCODEC_QSV_H265_10BIT:
            return hb_h265_qsv_profile_names_10bit;
        default:
            return NULL;
    }
}

const char* const* hb_qsv_level_get_names(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_QSV_H264:
            return hb_h264_level_names;
        case HB_VCODEC_QSV_H265_10BIT:
        case HB_VCODEC_QSV_H265:
            return hb_h265_level_names;
        default:
            return NULL;
    }
}

const char* hb_qsv_video_quality_get_name(uint32_t codec)
{
    uint64_t caps = 0;
    hb_qsv_adapter_details_t* details = hb_qsv_get_adapters_details_by_index(hb_qsv_get_adapter_index());
    if (details)
    {
        switch (codec)
        {
            case HB_VCODEC_QSV_H264:
                if (details->hb_qsv_info_avc != NULL) caps = details->hb_qsv_info_avc->capabilities;
                break;

            case HB_VCODEC_QSV_H265_10BIT:
            case HB_VCODEC_QSV_H265:
                if (details->hb_qsv_info_hevc != NULL) caps = details->hb_qsv_info_hevc->capabilities;
                break;

            default:
                break;
        }
    }
    return (caps & HB_QSV_CAP_RATECONTROL_ICQ) ? "ICQ" : "QP";
}

void hb_qsv_video_quality_get_limits(uint32_t codec, float *low, float *high,
                                     float *granularity, int *direction)
{
    uint64_t caps = 0;
    hb_qsv_adapter_details_t* details = hb_qsv_get_adapters_details_by_index(hb_qsv_get_adapter_index());
    if (details)
    {
        switch (codec)
        {
            case HB_VCODEC_QSV_H265_10BIT:
            case HB_VCODEC_QSV_H265:
                if (details->hb_qsv_info_hevc != NULL) caps = details->hb_qsv_info_hevc->capabilities;
                *direction   = 1;
                *granularity = 1.;
                *low         = (caps & HB_QSV_CAP_RATECONTROL_ICQ) ? 1. : 0.;
                *high        = 51.;
                break;

            case HB_VCODEC_QSV_H264:
            default:
                if (details->hb_qsv_info_avc != NULL) caps = details->hb_qsv_info_avc->capabilities;
                *direction   = 1;
                *granularity = 1.;
                *low         = (caps & HB_QSV_CAP_RATECONTROL_ICQ) ? 1. : 0.;
                *high        = 51.;
                break;
        }
    }
}

int hb_qsv_param_default_preset(hb_qsv_param_t *param,
                                mfxVideoParam *videoParam,
                                hb_qsv_info_t *info, const char *preset)
{
    if (param != NULL && videoParam != NULL && info != NULL)
    {
        int ret = hb_qsv_param_default(param, videoParam, info);
        if (ret)
        {
            return ret;
        }
    }
    else
    {
        hb_error("hb_qsv_param_default_preset: invalid pointer(s) param=%p videoParam=%p info=%p preset=%p", param, videoParam, info, preset);
        return -1;
    }
    if (preset != NULL && preset[0] != '\0')
    {
        if (!strcasecmp(preset, "quality"))
        {
            /*
             * HSW TargetUsage:     2
             *     NumRefFrame:     0
             *     GopRefDist:      4 (CQP), 3 (VBR)        -> -1 (set by encoder)
             *     GopPicSize:     32 (CQP), 1 second (VBR) -> -1 (set by encoder)
             *     BPyramid:        1 (CQP), 0 (VBR)        -> -1 (set by encoder)
             *     LookAhead:       1 (on)
             *     LookAheadDepth: 40
             *
             *
             * SNB
             * IVB Preset Not Available
             *
             * Note: this preset is the libhb default (like x264's "medium").
             */
        }
        else if (!strcasecmp(preset, "balanced"))
        {
            /*
             * HSW TargetUsage:     4
             *     NumRefFrame:     1
             *     GopRefDist:      4 (CQP), 3 (VBR)        -> -1 (set by encoder)
             *     GopPicSize:     32 (CQP), 1 second (VBR) -> -1 (set by encoder)
             *     BPyramid:        1 (CQP), 0 (VBR)        -> -1 (set by encoder)
             *     LookAhead:       0 (off)
             *     LookAheadDepth: Not Applicable
             */
            if (hb_qsv_hardware_generation(hb_qsv_get_platform(hb_qsv_get_adapter_index())) >= QSV_G3)
            {
                param->rc.lookahead                = 0;
                param->videoParam->mfx.NumRefFrame = 1;
                param->videoParam->mfx.TargetUsage = MFX_TARGETUSAGE_4;
            }
            else
            {
                /*
                 * SNB
                 * IVB TargetUsage:     2
                 *     NumRefFrame:     0
                 *     GopRefDist:      4 (CQP), 3 (VBR)        -> -1 (set by encoder)
                 *     GopPicSize:     32 (CQP), 1 second (VBR) -> -1 (set by encoder)
                 *     BPyramid:       Not Applicable
                 *     LookAhead:      Not Applicable
                 *     LookAheadDepth: Not Applicable
                 *
                 * Note: this preset is not the libhb default,
                 * but the settings are the same so do nothing.
                 */
            }
        }
        else if (!strcasecmp(preset, "speed"))
        {
            if (hb_qsv_hardware_generation(hb_qsv_get_platform(hb_qsv_get_adapter_index())) >= QSV_G7)
            {
                // Since IceLake only
                param->rc.lookahead                = 0;
                param->videoParam->mfx.NumRefFrame = 1;
                param->videoParam->mfx.TargetUsage = MFX_TARGETUSAGE_7;
            }
            else if (hb_qsv_hardware_generation(hb_qsv_get_platform(hb_qsv_get_adapter_index())) >= QSV_G3)
            {
                /*
                 * HSW TargetUsage:     6
                 *     NumRefFrame:     0 (CQP), 1 (VBR)        -> see note
                 *     GopRefDist:      4 (CQP), 3 (VBR)        -> -1 (set by encoder)
                 *     GopPicSize:     32 (CQP), 1 second (VBR) -> -1 (set by encoder)
                 *     BPyramid:        1 (CQP), 0 (VBR)        -> -1 (set by encoder)
                 *     LookAhead:       0 (off)
                 *     LookAheadDepth: Not Applicable
                 *
                 * Note: NumRefFrame depends on the RC method, which we don't
                 *       know here. Rather than have an additional variable and
                 *       having the encoder set it, we set it to 1 and let the
                 *       B-pyramid code sanitize it. Since BPyramid is 1 w/CQP,
                 *       the result (3) is the same as what MSDK would pick for
                 *       NumRefFrame 0 GopRefDist 4 GopPicSize 32.
                 */
                param->rc.lookahead                = 0;
                param->videoParam->mfx.NumRefFrame = 1;
                param->videoParam->mfx.TargetUsage = MFX_TARGETUSAGE_6;
            }
            else
            {
                /*
                 * SNB
                 * IVB TargetUsage:     4
                 *     NumRefFrame:     0
                 *     GopRefDist:      4 (CQP), 3 (VBR)        -> -1 (set by encoder)
                 *     GopPicSize:     32 (CQP), 1 second (VBR) -> -1 (set by encoder)
                 *     BPyramid:       Not Applicable
                 *     LookAhead:      Not Applicable
                 *     LookAheadDepth: Not Applicable
                 */
                param->videoParam->mfx.TargetUsage = MFX_TARGETUSAGE_4;
            }
        }
        else
        {
            hb_error("hb_qsv_param_default_preset: invalid preset '%s'", preset);
            return -1;
        }
    }
    return 0;
}

int hb_qsv_param_default_async_depth()
{
    return hb_qsv_hardware_generation(hb_qsv_get_platform(hb_qsv_get_adapter_index())) >= QSV_G7 ? 6 : HB_QSV_ASYNC_DEPTH_DEFAULT;
}

int hb_qsv_param_default(hb_qsv_param_t *param, mfxVideoParam *videoParam,
                         hb_qsv_info_t  *info)
{
    if (param != NULL && videoParam != NULL && info != NULL)
    {
        // introduced in API 1.0
        memset(&param->codingOption, 0, sizeof(mfxExtCodingOption));
        param->codingOption.Header.BufferId      = MFX_EXTBUFF_CODING_OPTION;
        param->codingOption.Header.BufferSz      = sizeof(mfxExtCodingOption);
        param->codingOption.MECostType           = 0; // reserved, must be 0
        param->codingOption.MESearchType         = 0; // reserved, must be 0
        param->codingOption.MVSearchWindow.x     = 0; // reserved, must be 0
        param->codingOption.MVSearchWindow.y     = 0; // reserved, must be 0
        param->codingOption.RefPicListReordering = 0; // reserved, must be 0
        param->codingOption.IntraPredBlockSize   = 0; // reserved, must be 0
        param->codingOption.InterPredBlockSize   = 0; // reserved, must be 0
        param->codingOption.MVPrecision          = 0; // reserved, must be 0
        param->codingOption.EndOfSequence        = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.RateDistortionOpt    = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.ResetRefList         = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.MaxDecFrameBuffering = 0; // unspecified
        param->codingOption.AUDelimiter          = MFX_CODINGOPTION_OFF;
        param->codingOption.SingleSeiNalUnit     = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.PicTimingSEI         = MFX_CODINGOPTION_OFF;
        param->codingOption.VuiNalHrdParameters  = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.FramePicture         = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.CAVLC                = MFX_CODINGOPTION_OFF;
        // introduced in API 1.3
        param->codingOption.RefPicMarkRep        = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.FieldOutput          = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.NalHrdConformance    = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.SingleSeiNalUnit     = MFX_CODINGOPTION_UNKNOWN;
        param->codingOption.VuiVclHrdParameters  = MFX_CODINGOPTION_UNKNOWN;
        // introduced in API 1.4
        param->codingOption.ViewOutput           = MFX_CODINGOPTION_UNKNOWN;
        // introduced in API 1.6
        param->codingOption.RecoveryPointSEI     = MFX_CODINGOPTION_UNKNOWN;

        // introduced in API 1.3
        memset(&param->videoSignalInfo, 0, sizeof(mfxExtVideoSignalInfo));
        param->videoSignalInfo.Header.BufferId          = MFX_EXTBUFF_VIDEO_SIGNAL_INFO;
        param->videoSignalInfo.Header.BufferSz          = sizeof(mfxExtVideoSignalInfo);
        param->videoSignalInfo.VideoFormat              = 5; // undefined
        param->videoSignalInfo.VideoFullRange           = 0; // TV range
        param->videoSignalInfo.ColourDescriptionPresent = 0; // don't write to bitstream
        param->videoSignalInfo.ColourPrimaries          = 2; // undefined
        param->videoSignalInfo.TransferCharacteristics  = 2; // undefined
        param->videoSignalInfo.MatrixCoefficients       = 2; // undefined

        // introduced in API 1.6
        memset(&param->codingOption2, 0, sizeof(mfxExtCodingOption2));
        param->codingOption2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
        param->codingOption2.Header.BufferSz = sizeof(mfxExtCodingOption2);
        param->codingOption2.IntRefType      = 0;
        param->codingOption2.IntRefCycleSize = 2;
        param->codingOption2.IntRefQPDelta   = 0;
        param->codingOption2.MaxFrameSize    = 0;
        param->codingOption2.BitrateLimit    = MFX_CODINGOPTION_ON;
        param->codingOption2.MBBRC           = MFX_CODINGOPTION_ON;
        param->codingOption2.ExtBRC          = MFX_CODINGOPTION_OFF;
        // introduced in API 1.7
        param->codingOption2.LookAheadDepth  = 40;
        param->codingOption2.Trellis         = MFX_TRELLIS_OFF;
        // introduced in API 1.8
        param->codingOption2.RepeatPPS       = MFX_CODINGOPTION_ON;
        param->codingOption2.BRefType        = MFX_B_REF_UNKNOWN; // controlled via gop.b_pyramid
        param->codingOption2.AdaptiveI       = MFX_CODINGOPTION_OFF;
        param->codingOption2.AdaptiveB       = MFX_CODINGOPTION_OFF;
        param->codingOption2.LookAheadDS     = MFX_LOOKAHEAD_DS_OFF;
        param->codingOption2.NumMbPerSlice   = 0;

        // GOP & rate control
        param->gop.b_pyramid          = -1; // set automatically
        param->gop.gop_pic_size       = -1; // set automatically
        param->gop.gop_ref_dist       = -1; // set automatically
        param->gop.int_ref_cycle_size = -1; // set automatically
        param->rc.icq                 =  1; // enabled by default (if supported)
        param->rc.lookahead           =  1; // enabled by default (if supported)
        param->rc.cqp_offsets[0]      =  0;
        param->rc.cqp_offsets[1]      =  2;
        param->rc.cqp_offsets[2]      =  4;
        param->rc.vbv_max_bitrate     =  0; // set automatically
        param->rc.vbv_buffer_size     =  0; // set automatically
        param->rc.vbv_buffer_init     = .0; // set automatically

        // introduced in API 1.0
        memset(videoParam, 0, sizeof(mfxVideoParam));
        param->videoParam                   = videoParam;
        param->videoParam->Protected        = 0; // reserved, must be 0
        param->videoParam->NumExtParam      = 0;
        param->videoParam->IOPattern        = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
        param->videoParam->mfx.TargetUsage  = MFX_TARGETUSAGE_BALANCED;
        param->videoParam->mfx.GopOptFlag   = MFX_GOP_CLOSED;
        param->videoParam->mfx.NumThread    = 0; // deprecated, must be 0
        param->videoParam->mfx.EncodedOrder = 0; // input is in display order
        param->videoParam->mfx.IdrInterval  = 0; // all I-frames are IDR
        param->videoParam->mfx.NumSlice     = 0; // use Media SDK default
        param->videoParam->mfx.NumRefFrame  = 0; // use Media SDK default
        param->videoParam->mfx.GopPicSize   = 0; // use Media SDK default
        param->videoParam->mfx.GopRefDist   = 0; // use Media SDK default
        param->videoParam->mfx.LowPower     = MFX_CODINGOPTION_OFF; // use Media SDK default
        // introduced in API 1.1
        param->videoParam->AsyncDepth       = hb_qsv_param_default_async_depth();
        // introduced in API 1.3
        param->videoParam->mfx.BRCParamMultiplier = 0; // no multiplier

        // FrameInfo: set by video encoder, except PicStruct
        param->videoParam->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;

        // attach supported mfxExtBuffer structures to the mfxVideoParam
        param->videoParam->NumExtParam                                = 0;
        param->videoParam->ExtParam                                   = param->ExtParamArray;
        if (info->capabilities & HB_QSV_CAP_VUI_VSINFO)
        {
            param->videoParam->ExtParam[param->videoParam->NumExtParam++] = (mfxExtBuffer*)&param->videoSignalInfo;
        }
        if (info->capabilities & HB_QSV_CAP_OPTION1)
        {
            param->videoParam->ExtParam[param->videoParam->NumExtParam++] = (mfxExtBuffer*)&param->codingOption;
        }
        if (info->capabilities & HB_QSV_CAP_OPTION2)
        {
            param->videoParam->ExtParam[param->videoParam->NumExtParam++] = (mfxExtBuffer*)&param->codingOption2;
        }
        if (info->capabilities & HB_QSV_CAP_LOWPOWER_ENCODE)
        {
            param->videoParam->mfx.LowPower = MFX_CODINGOPTION_ON;
        }
    }
    else
    {
        hb_error("hb_qsv_param_default: invalid pointer(s)");
        return -1;
    }
    return 0;
}

hb_triplet_t* hb_triplet4value(hb_triplet_t *triplets, const int value)
{
    for (int i = 0; triplets[i].name != NULL; i++)
    {
        if (triplets[i].value == value)
        {
            return &triplets[i];
        }
    }
    return NULL;
}

hb_triplet_t* hb_triplet4name(hb_triplet_t *triplets, const char *name)
{
    for (int i = 0; triplets[i].name != NULL; i++)
    {
        if (!strcasecmp(triplets[i].name, name))
        {
            return &triplets[i];
        }
    }
    return NULL;
}

hb_triplet_t* hb_triplet4key(hb_triplet_t *triplets, const char *key)
{
    for (int i = 0; triplets[i].name != NULL; i++)
    {
        if (!strcasecmp(triplets[i].key, key))
        {
            return &triplets[i];
        }
    }
    return NULL;
}

const char* hb_qsv_codec_name(uint32_t codec_id)
{
    switch (codec_id)
    {
        case MFX_CODEC_AVC:
            return "H.264/AVC";

        case MFX_CODEC_HEVC:
            return "H.265/HEVC";

        default:
            return NULL;
    }
}

const char* hb_qsv_profile_name(uint32_t codec_id, uint16_t profile_id)
{
    hb_triplet_t *profile = NULL;
    switch (codec_id)
    {
        case MFX_CODEC_AVC:
            profile = hb_triplet4value(hb_qsv_h264_profiles, profile_id);
            break;

        case MFX_CODEC_HEVC:
            profile = hb_triplet4value(hb_qsv_h265_profiles, profile_id);
            break;

        default:
            break;
    }
    return profile != NULL ? profile->name : NULL;
}

const char* hb_qsv_level_name(uint32_t codec_id, uint16_t level_id)
{
    hb_triplet_t *level = NULL;
    switch (codec_id)
    {
        case MFX_CODEC_AVC:
            level = hb_triplet4value(hb_qsv_h264_levels, level_id);
            break;

        case MFX_CODEC_HEVC:
            level = hb_triplet4value(hb_qsv_h265_levels, level_id);
            break;

        default:
            break;
    }
    return level != NULL ? level->name : NULL;
}

const char* hb_qsv_frametype_name(uint16_t qsv_frametype)
{
    if      (qsv_frametype & MFX_FRAMETYPE_IDR)
    {
        return qsv_frametype & MFX_FRAMETYPE_REF ? "IDR (ref)" : "IDR";
    }
    else if (qsv_frametype & MFX_FRAMETYPE_I)
    {
        return qsv_frametype & MFX_FRAMETYPE_REF ? "I (ref)"   : "I";
    }
    else if (qsv_frametype & MFX_FRAMETYPE_P)
    {
        return qsv_frametype & MFX_FRAMETYPE_REF ? "P (ref)"   : "P";
    }
    else if (qsv_frametype & MFX_FRAMETYPE_B)
    {
        return qsv_frametype & MFX_FRAMETYPE_REF ? "B (ref)"   : "B";
    }
    else
    {
        return "unknown";
    }
}

uint8_t hb_qsv_frametype_xlat(uint16_t qsv_frametype, uint16_t *out_flags)
{
    uint16_t flags     = 0;
    uint8_t  frametype = 0;

    if (qsv_frametype & MFX_FRAMETYPE_IDR)
    {
        flags |= HB_FLAG_FRAMETYPE_KEY;
        frametype = HB_FRAME_IDR;
    }
    else if (qsv_frametype & MFX_FRAMETYPE_I)
    {
        frametype = HB_FRAME_I;
    }
    else if (qsv_frametype & MFX_FRAMETYPE_P)
    {
        frametype = HB_FRAME_P;
    }
    else if (qsv_frametype & MFX_FRAMETYPE_B)
    {
        frametype = HB_FRAME_B;
    }

    if (qsv_frametype & MFX_FRAMETYPE_REF)
    {
        flags |= HB_FLAG_FRAMETYPE_REF;
    }

    if (out_flags != NULL)
    {
       *out_flags = flags;
    }
    return frametype;
}

const char* hb_qsv_impl_get_name(int impl)
{
    switch (MFX_IMPL_BASETYPE(impl))
    {
        case MFX_IMPL_SOFTWARE:
            return "software";

        case MFX_IMPL_HARDWARE:
            return "hardware (1)";
        case MFX_IMPL_HARDWARE2:
            return "hardware (2)";
        case MFX_IMPL_HARDWARE3:
            return "hardware (3)";
        case MFX_IMPL_HARDWARE4:
            return "hardware (4)";
        case MFX_IMPL_HARDWARE_ANY:
            return "hardware (any)";

        case MFX_IMPL_AUTO:
            return "automatic";
        case MFX_IMPL_AUTO_ANY:
            return "automatic (any)";

        default:
            return NULL;
    }
}

const char* hb_qsv_impl_get_via_name(int impl)
{
    if      ((impl & 0xF00) == MFX_IMPL_VIA_VAAPI)
        return "via VAAPI";
    else if ((impl & 0xF00) == MFX_IMPL_VIA_D3D11)
        return "via D3D11";
    else if ((impl & 0xF00) == MFX_IMPL_VIA_D3D9)
        return "via D3D9";
    else if ((impl & 0xF00) == MFX_IMPL_VIA_ANY)
        return "via ANY";
    else return NULL;
}

void hb_qsv_force_workarounds()
{
#define FORCE_WORKAROUNDS ~(HB_QSV_CAP_OPTION2_BREFTYPE)
    hb_qsv_adapter_details_t* details = hb_qsv_get_adapters_details_by_index(hb_qsv_get_adapter_index());
    if (details)
    {
        details->qsv_software_info_avc.capabilities  &= FORCE_WORKAROUNDS;
        details->qsv_hardware_info_avc.capabilities  &= FORCE_WORKAROUNDS;
        details->qsv_software_info_hevc.capabilities &= FORCE_WORKAROUNDS;
        details->qsv_hardware_info_hevc.capabilities &= FORCE_WORKAROUNDS;
    }
#undef FORCE_WORKAROUNDS
}

#if defined(_WIN32) || defined(__MINGW32__)
// Direct X
#define COBJMACROS
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3d9.h>
#include <dxva2api.h>

#if HAVE_DXGIDEBUG_H
#include <dxgidebug.h>
#endif

static mfxHDL device_manager_handle = NULL;
static mfxHandleType device_manager_handle_type;

static ID3D11DeviceContext *device_context = NULL;

typedef IDirect3D9* WINAPI pDirect3DCreate9(UINT);
typedef HRESULT WINAPI pDirect3DCreate9Ex(UINT, IDirect3D9Ex **);
typedef HRESULT(WINAPI *HB_PFN_CREATE_DXGI_FACTORY)(REFIID riid, void **ppFactory);

int hb_qsv_info_init()
{
    // Collect the information about qsv adapters
    g_qsv_adapters_info.Adapters = NULL;
    g_qsv_adapters_info.NumAlloc = 0;
    g_qsv_adapters_info.NumActual = 0;
    int err = hb_qsv_query_adapters(&g_qsv_adapters_info);
    if (err)
    {
        hb_error("hb_qsv_info_init: failed to query qsv adapters");
        return -1;
    }
    hb_qsv_make_adapters_list(&g_qsv_adapters_info, &g_qsv_adapters_list);
    hb_qsv_make_adapters_details_list(&g_qsv_adapters_info, &g_qsv_adapters_details_list);
    hb_qsv_collect_adapters_details(g_qsv_adapters_list, g_qsv_adapters_details_list);
    return 0;
}

int hb_qsv_set_adapter_index(int adapter_index)
{
    if (g_adapter_index == adapter_index)
        return 0;

    for (int i = 0; i < g_qsv_adapters_info.NumActual; i++)
    {
        mfxAdapterInfo* info = &g_qsv_adapters_info.Adapters[i];
        if (info && (info->Number == adapter_index))
        {
            g_adapter_index = adapter_index;
            return 0;
        }
    }
    hb_error("hb_qsv_set_adapter_index: incorrect qsv device index %d", adapter_index);
    return -1;
}

int qsv_map_mfx_platform_codename(int mfx_platform_codename)
{
    int platform = HB_CPU_PLATFORM_UNSPECIFIED;

    switch (mfx_platform_codename)
    {
    case MFX_PLATFORM_SANDYBRIDGE:
        platform = HB_CPU_PLATFORM_INTEL_SNB;
        break;
    case MFX_PLATFORM_IVYBRIDGE:
        platform = HB_CPU_PLATFORM_INTEL_IVB;
        break;
    case MFX_PLATFORM_HASWELL:
        platform = HB_CPU_PLATFORM_INTEL_HSW;
        break;
    case MFX_PLATFORM_BAYTRAIL:
    case MFX_PLATFORM_BROADWELL:
        platform = HB_CPU_PLATFORM_INTEL_BDW;
        break;
    case MFX_PLATFORM_CHERRYTRAIL:
        platform = HB_CPU_PLATFORM_INTEL_CHT;
        break;
    case MFX_PLATFORM_SKYLAKE:
        platform = HB_CPU_PLATFORM_INTEL_SKL;
        break;
    case MFX_PLATFORM_APOLLOLAKE:
    case MFX_PLATFORM_KABYLAKE:
        platform = HB_CPU_PLATFORM_INTEL_KBL;
        break;
#if (MFX_VERSION >= 1025)
    case MFX_PLATFORM_GEMINILAKE:
    case MFX_PLATFORM_COFFEELAKE:
    case MFX_PLATFORM_CANNONLAKE:
        platform = HB_CPU_PLATFORM_INTEL_KBL;
        break;
#endif
#if (MFX_VERSION >= 1027)
    case MFX_PLATFORM_ICELAKE:
        platform = HB_CPU_PLATFORM_INTEL_ICL;
        break;
#endif
#if (MFX_VERSION >= 1031)
    case MFX_PLATFORM_ELKHARTLAKE:
    case MFX_PLATFORM_JASPERLAKE:
    case MFX_PLATFORM_TIGERLAKE:
        platform = HB_CPU_PLATFORM_INTEL_TGL;
        break;
    // TODO: update mfx_dispatch to add MFX_PLATFORM_ALDERLAKE_S
    // case MFX_PLATFORM_ALDERLAKE_S:
    //     platform = HB_CPU_PLATFORM_INTEL_TGL;
    //     break;
#endif
    default:
        platform = HB_CPU_PLATFORM_UNSPECIFIED;
    }
    return platform;
}

static void hb_qsv_free_adapters_details()
{
    for (int i = 0; i < hb_list_count(g_qsv_adapters_details_list); i++)
    {
        hb_qsv_adapter_details_t *details = hb_list_item(g_qsv_adapters_details_list, i);
        if (details->index)
        {
            av_free(details);
        }
    }
}

static const char* hb_qsv_get_adapter_type(const mfxAdapterInfo* info)
{
    if (info)
    {
        return (info->Platform.MediaAdapterType == MFX_MEDIA_INTEGRATED) ? "integrated" :
        (info->Platform.MediaAdapterType == MFX_MEDIA_DISCRETE) ? "discrete" : "unknown";
    }
    return NULL;
}

int hb_qsv_get_platform(int adapter_index)
{
    for (int i = 0; i < g_qsv_adapters_info.NumActual; i++)
    {
        mfxAdapterInfo* info = &g_qsv_adapters_info.Adapters[i];
        // find DirectX adapter with given index in list of QSV adapters
        // if -1 use first adapter with highest priority
        if (info && ((info->Number == adapter_index) || (adapter_index == -1)))
        {
            return qsv_map_mfx_platform_codename(info->Platform.CodeName);
        }
    }
    return HB_CPU_PLATFORM_UNSPECIFIED;
}

int hb_qsv_param_parse_dx_index(hb_job_t *job, const int dx_index)
{
    for (int i = 0; i < g_qsv_adapters_info.NumActual; i++)
    {
        mfxAdapterInfo* info = &g_qsv_adapters_info.Adapters[i];
        // find DirectX adapter with given index in list of QSV adapters
        // if -1 use first adapter with highest priority
        if (info && ((info->Number == dx_index) || (dx_index == -1)))
        {
            if (job->qsv.ctx && !job->qsv.ctx->qsv_device)
            {
                job->qsv.ctx->qsv_device = av_mallocz_array(32, sizeof(*job->qsv.ctx->qsv_device));
                if (!job->qsv.ctx->qsv_device)
                {
                    hb_error("hb_qsv_param_parse_dx_index: failed to allocate memory for qsv device");
                    return -1;
                }
            }
            sprintf(job->qsv.ctx->qsv_device, "%u", info->Number);
            job->qsv.ctx->dx_index = info->Number;
            hb_log("qsv: %s qsv adapter with index %s has been selected", hb_qsv_get_adapter_type(info), job->qsv.ctx->qsv_device);
            hb_qsv_set_adapter_index(info->Number);
            return 0;
        }
    }
    hb_error("qsv: hb_qsv_param_parse_dx_index incorrect qsv device index %d", dx_index);
    return -1;
}

static int hb_dxva2_device_create9(HMODULE d3dlib, UINT adapter, IDirect3D9 **d3d9_out)
{
    pDirect3DCreate9 *createD3D = (pDirect3DCreate9 *)hb_dlsym(d3dlib, "Direct3DCreate9");
    if (!createD3D) {
        hb_error("hb_dxva2_device_create9: failed to locate Direct3DCreate9");
        return -1;
    }

    IDirect3D9 *d3d9 = createD3D(D3D_SDK_VERSION);
    if (!d3d9) {
        hb_error("hb_dxva2_device_create9: createD3D failed");
        return -1;
    }
    *d3d9_out = d3d9;
    return 0;
}

static int hb_dxva2_device_create9ex(HMODULE d3dlib, UINT adapter, IDirect3D9 **d3d9_out)
{
    IDirect3D9Ex *d3d9ex = NULL;
    HRESULT hr;
    pDirect3DCreate9Ex *createD3DEx = (pDirect3DCreate9Ex *)hb_dlsym(d3dlib, "Direct3DCreate9Ex");
    if (!createD3DEx)
    {
        hb_error("hb_dxva2_device_create9ex: failed to locate Direct3DCreate9Ex");
        return -1;
    }

    hr = createD3DEx(D3D_SDK_VERSION, &d3d9ex);
    if (FAILED(hr))
    {
        hb_error("hb_dxva2_device_create9ex: createD3DEx failed %d", hr);
        return -1;
    }
    *d3d9_out = (IDirect3D9 *)d3d9ex;
    return 0;
}

static int hb_d3d11va_device_create(int adapter_id, ID3D11Device** d3d11_out)
{
    HANDLE d3dlib, dxgilib;

    d3dlib  = hb_dlopen("d3d11.dll");
    dxgilib = hb_dlopen("dxgi.dll");
    if (!d3dlib || !dxgilib)
    {
        hb_error("hb_d3d11va_device_check: failed to load d3d11.dll and dxgi.dll");
        return -1;
    }

    PFN_D3D11_CREATE_DEVICE mD3D11CreateDevice;
    HB_PFN_CREATE_DXGI_FACTORY mCreateDXGIFactory;
    mD3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)hb_dlsym(d3dlib, "D3D11CreateDevice");
    mCreateDXGIFactory = (HB_PFN_CREATE_DXGI_FACTORY)hb_dlsym(dxgilib, "CreateDXGIFactory1");

    if (!mD3D11CreateDevice || !mCreateDXGIFactory) {
        hb_error("hb_d3d11va_device_check: failed to locate D3D11CreateDevice and CreateDXGIFactory1 functions");
        return -1;
    }

    HRESULT hr;
    IDXGIAdapter *pAdapter = NULL;
    IDXGIFactory2 *pDXGIFactory;
    hr = mCreateDXGIFactory(&IID_IDXGIFactory2, (void **)&pDXGIFactory);
    if (FAILED(hr)) {
        hb_error("hb_d3d11va_create_device: mCreateDXGIFactory returned %d", hr);
        return -1;
    }

    if (adapter_id == -1)
    {
        adapter_id = 0;
    }

    while (IDXGIFactory2_EnumAdapters(pDXGIFactory, adapter_id++, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        ID3D11Device* pd3dDevice = NULL;
        DXGI_ADAPTER_DESC adapterDesc;

        hr = IDXGIAdapter2_GetDesc(pAdapter, &adapterDesc);
        if (SUCCEEDED(hr)) {
            if (adapterDesc.VendorId == 0x8086) {
                hr = mD3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, D3D11_CREATE_DEVICE_VIDEO_SUPPORT, NULL, 0, D3D11_SDK_VERSION, &pd3dDevice, NULL, NULL);
                if (SUCCEEDED(hr)) {
                    IDXGIFactory2_Release(pDXGIFactory);
                    *d3d11_out = (ID3D11Device *)pd3dDevice;
                    return adapter_id - 1;
                } else {
                    hb_error("hb_d3d11va_device_check: D3D11CreateDevice returned %d", hr);
                }
            }
        } else {
            hb_error("hb_d3d11va_device_check: IDXGIAdapter2_GetDesc returned %d", hr);
        }

        if (pAdapter)
            IDXGIAdapter_Release(pAdapter);
    }

    IDXGIFactory2_Release(pDXGIFactory);
    return -1;
}

static int hb_d3d11va_device_check()
{
    ID3D11Device* d3d11 = NULL;
    return hb_d3d11va_device_create(-1, &d3d11);
}

static int hb_dxva2_device_check()
{
    HRESULT hr;
    HMODULE d3dlib = NULL;
    IDirect3D9 *d3d9 = NULL;
    D3DADAPTER_IDENTIFIER9 identifier;
    D3DADAPTER_IDENTIFIER9 *d3dai = &identifier;
    UINT adapter = D3DADAPTER_DEFAULT;

    d3dlib = hb_dlopen("d3d9.dll");
    if (!d3dlib)
    {
        hb_error("hb_dxva2_device_check: failed to load d3d9 library");
        return -1;
    }

    if (hb_dxva2_device_create9ex(d3dlib, adapter, &d3d9) < 0)
    {
        // Retry with "classic" d3d9
        hr = hb_dxva2_device_create9(d3dlib, adapter, &d3d9);
        if (hr < 0)
        {
            hr = -1;
            goto clean_up;
        }
    }

    hr = IDirect3D9_GetAdapterIdentifier(d3d9, D3DADAPTER_DEFAULT, 0, d3dai);
    if (FAILED(hr))
    {
        hb_error("hb_dxva2_device_check: IDirect3D9_GetAdapterIdentifier failed");
        hr = -1;
        goto clean_up;
    }

    unsigned intel_id = 0x8086;
    if(d3dai)
    {
        if(d3dai->VendorId != intel_id)
        {
            hb_error("hb_dxva2_device_check: adapter that was found does not support QSV. It is required for zero-copy QSV path");
            hr = -1;
            goto clean_up;
        }
    }
    hr = 0;

clean_up:
    if (d3d9)
        IDirect3D9_Release(d3d9);

    if (d3dlib)
        hb_dlclose(d3dlib);

    return hr;
}

static HRESULT lock_device(
    IDirect3DDeviceManager9 *pDeviceManager,
    BOOL fBlock,
    IDirect3DDevice9 **ppDevice, // Receives a pointer to the device.
    HANDLE *pHandle              // Receives a device handle.
    )
{
    *pHandle = NULL;
    *ppDevice = NULL;

    HANDLE hDevice = 0;

    HRESULT hr = pDeviceManager->lpVtbl->OpenDeviceHandle(pDeviceManager, &hDevice);

    if (SUCCEEDED(hr))
    {
        hr = pDeviceManager->lpVtbl->LockDevice(pDeviceManager, hDevice, ppDevice, fBlock);
    }

    if (hr == DXVA2_E_NEW_VIDEO_DEVICE)
    {
        // Invalid device handle. Try to open a new device handle.
        hr = pDeviceManager->lpVtbl->CloseDeviceHandle(pDeviceManager, hDevice);

        if (SUCCEEDED(hr))
        {
            hr = pDeviceManager->lpVtbl->OpenDeviceHandle(pDeviceManager, &hDevice);
        }

        // Try to lock the device again.
        if (SUCCEEDED(hr))
        {
            hr = pDeviceManager->lpVtbl->LockDevice(pDeviceManager, hDevice, ppDevice, TRUE);
        }
    }

    if (SUCCEEDED(hr))
    {
        *pHandle = hDevice;
    }
    return hr;
}

static HRESULT unlock_device(
    IDirect3DDeviceManager9 *pDeviceManager,
    HANDLE handle              // Receives a device handle.
    )
{
    HRESULT hr = pDeviceManager->lpVtbl->UnlockDevice(pDeviceManager, handle, 0);
    if (SUCCEEDED(hr))
    {
        hr = pDeviceManager->lpVtbl->CloseDeviceHandle(pDeviceManager, handle);
    }
    return hr;
}

static int hb_qsv_find_surface_idx(const QSVMid *mids, const int nb_mids, const QSVMid *mid)
{
    if (mids)
    {
        int i;
        for (i = 0; i < nb_mids; i++) {
            const QSVMid *m = &mids[i];
            if ((m->handle_pair->first == mid->handle_pair->first) &&
                (m->handle_pair->second == mid->handle_pair->second))
                return i;
        }
    }
    return -1;
}

int hb_qsv_replace_surface_mid(HBQSVFramesContext* hb_enc_qsv_frames_ctx, const QSVMid *mid, mfxFrameSurface1 *surface)
{
    if (!hb_enc_qsv_frames_ctx || !surface)
        return -1;

    int ret = hb_qsv_find_surface_idx(hb_enc_qsv_frames_ctx->mids, hb_enc_qsv_frames_ctx->nb_mids, mid);
    if (ret < 0)
    {
        hb_error("hb_qsv_replace_surface_mid: Surface with MemId=%p has not been found in the pool", mid);
        return -1;
    }
    else
    {
        surface->Data.MemId = &hb_enc_qsv_frames_ctx->mids[ret];
    }
    return 0;
}

int hb_qsv_release_surface_from_pool_by_surface_pointer(HBQSVFramesContext* hb_enc_qsv_frames_ctx, const mfxFrameSurface1 *surface)
{
    if (!hb_enc_qsv_frames_ctx || !surface)
        return -1;

    AVHWFramesContext *frames_ctx = (AVHWFramesContext*)hb_enc_qsv_frames_ctx->hw_frames_ctx->data;
    AVQSVFramesContext *frames_hwctx = frames_ctx->hwctx;

    for(int i = 0; i < hb_enc_qsv_frames_ctx->nb_mids; i++)
    {
        mfxFrameSurface1 *pool_surface = &frames_hwctx->surfaces[i];
        if(surface == pool_surface)
        {
            ff_qsv_atomic_dec(&hb_enc_qsv_frames_ctx->pool[i]);
            return 0;
        }
    }
    return -1;
}

int hb_qsv_get_mid_by_surface_from_pool(HBQSVFramesContext* hb_enc_qsv_frames_ctx, mfxFrameSurface1 *surface, QSVMid **out_mid)
{
    if (!hb_enc_qsv_frames_ctx || !surface)
        return -1;

    QSVMid *mid = NULL;
    
    AVHWFramesContext *frames_ctx = (AVHWFramesContext*)hb_enc_qsv_frames_ctx->hw_frames_ctx->data;
    AVQSVFramesContext *frames_hwctx = frames_ctx->hwctx;
    // find the first available surface in the pool
    int count = 0;
    while(1)
    {
        if(count > 30)
        {
            hb_error("hb_qsv_get_mid_by_surface_from_pool has not been found or busy", mid);
            hb_qsv_sleep(10); // prevent hang when all surfaces all used
            count = 0;
        }

        for(int i = 0; i < hb_enc_qsv_frames_ctx->nb_mids; i++)
        {
            mid = &hb_enc_qsv_frames_ctx->mids[i];
            mfxFrameSurface1 *pool_surface = &frames_hwctx->surfaces[i];
            if( (pool_surface->Data.Locked == 0) && (surface == pool_surface))
            {
                *out_mid = mid;
                return 0;
            }
        }
        count++;
    }
}

int hb_qsv_get_free_surface_from_pool(HBQSVFramesContext* hb_enc_qsv_frames_ctx, AVFrame* frame, QSVMid** out_mid)
{
    if (!hb_enc_qsv_frames_ctx || !frame)
        return -1;

    AVHWFramesContext *frames_ctx = (AVHWFramesContext*)hb_enc_qsv_frames_ctx->hw_frames_ctx->data;
    AVQSVFramesContext *frames_hwctx = frames_ctx->hwctx;

    // find the first available surface in the pool
    int count = 0;
    while(1)
    {
        if(count > 30)
        {
            hb_qsv_sleep(10); // prevent hang when all surfaces all used
            count = 0;
        }

        int ret = av_hwframe_get_buffer(hb_enc_qsv_frames_ctx->hw_frames_ctx, frame, 0);
        if (ret)
        {
            return -1;
        }
        else
        {
            mfxFrameSurface1 *output_surface = (mfxFrameSurface1 *)frame->data[3];
            for(int i = 0; i < hb_enc_qsv_frames_ctx->nb_mids; i++)
            {
                QSVMid* mid = &hb_enc_qsv_frames_ctx->mids[i];
                mfxFrameSurface1* cur_surface = &frames_hwctx->surfaces[i];
                if(cur_surface == output_surface)
                {
                    if((hb_enc_qsv_frames_ctx->pool[i] == 0) && (output_surface->Data.Locked == 0))
                    {
                        *out_mid = mid;
                        ff_qsv_atomic_inc(&hb_enc_qsv_frames_ctx->pool[i]);
                        return 0;
                    }
                    else
                    {
                        // we need to do unref if surface is not taken to be used, otherwise -12.
                        av_frame_unref(frame);
                        break;
                    }
                }
            }
        }
        count++;
    }
}

static int hb_qsv_allocate_dx11_encoder_pool(HBQSVFramesContext* frames_ctx, ID3D11Device *device, ID3D11Texture2D* input_texture)
{
    D3D11_TEXTURE2D_DESC desc = { 0 };
    ID3D11Texture2D_GetDesc(input_texture, &desc);
    desc.ArraySize = 1;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;

    for (size_t i = 0; i < frames_ctx->nb_mids; i++)
    {
        ID3D11Texture2D* texture;
        HRESULT hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
        if (hr != S_OK)
        {
            hb_error("hb_qsv_allocate_dx11_encoder_pool: ID3D11Device_CreateTexture2D error");
            return -1;
        }

        QSVMid *mid = &frames_ctx->mids[i];
        mid->handle_pair->first = texture;
        mid->handle_pair->second = 0;
    }
    return 0;
}

static int hb_qsv_get_dx_device(hb_job_t *job)
{
    AVHWDeviceContext    *device_ctx = (AVHWDeviceContext*)job->qsv.ctx->hb_hw_device_ctx->data;
    AVQSVDeviceContext *device_hwctx = device_ctx->hwctx;
    mfxSession        parent_session = device_hwctx->session;

    if (device_manager_handle == NULL)
    {
        mfxIMPL device_impl;
        int err = MFXQueryIMPL(parent_session, &device_impl);
        if (err != MFX_ERR_NONE)
        {
            hb_error("hb_qsv_get_dx_device: no impl could be retrieved");
            return -1;
        }

        if (MFX_IMPL_VIA_D3D11 == MFX_IMPL_VIA_MASK(device_impl))
        {
            device_manager_handle_type = MFX_HANDLE_D3D11_DEVICE;
        }
        else if (MFX_IMPL_VIA_D3D9 == MFX_IMPL_VIA_MASK(device_impl))
        {
            device_manager_handle_type = MFX_HANDLE_D3D9_DEVICE_MANAGER;
        }
        else
        {
            hb_error("hb_qsv_get_dx_device: unsupported impl");
            return -1;
        }

        err = MFXVideoCORE_GetHandle(parent_session, device_manager_handle_type, &device_manager_handle);
        if (err != MFX_ERR_NONE)
        {
            hb_error("hb_qsv_get_dx_device: no supported hw handle could be retrieved "
                "from the session\n");
            return -1;
        }
        if (device_manager_handle_type == MFX_HANDLE_D3D11_DEVICE)
        {
            ID3D11Device *device = (ID3D11Device *)device_manager_handle;
            ID3D11Texture2D* input_texture = job->qsv.ctx->hb_dec_qsv_frames_ctx->input_texture;
            err = hb_qsv_allocate_dx11_encoder_pool(job->qsv.ctx->hb_dec_qsv_frames_ctx, device, input_texture);
            if (err < 0)
            {
                hb_error("hb_qsv_get_dx_device: hb_qsv_allocate_dx11_encoder_pool failed");
                return -1;
            }
            if (device_context == NULL)
            {
                ID3D11Device_GetImmediateContext(device, &device_context);
                if (!device_context)
                    return -1;
            }
        }
    }
    return 0;
}

static int hb_qsv_make_adapters_list(const mfxAdaptersInfo* adapters_info, hb_list_t **qsv_adapters_list)
{
    int max_generation = QSV_G0;
    int default_adapter = 0;

    if (!qsv_adapters_list)
    {
        hb_error("hb_qsv_make_adapters_list: destination pointer is NULL");
        return -1;
    }
    if (*qsv_adapters_list)
    {
        hb_error("hb_qsv_make_adapters_list: qsv_adapters_list is allocated already");
        return -1;
    }
    hb_list_t *list = hb_list_init();
    if (list == NULL)
    {
        hb_error("hb_qsv_make_adapters_list: hb_list_init() failed");
        return -1;
    }
    for (int i = 0; i < adapters_info->NumActual; i++)
    {
        mfxAdapterInfo* info = &adapters_info->Adapters[i];
        if (info)
        {
            int generation = hb_qsv_hardware_generation(qsv_map_mfx_platform_codename(info->Platform.CodeName));
            // select default QSV adapter
            if (generation > max_generation || info->Platform.MediaAdapterType == MFX_MEDIA_DISCRETE)
            {
                max_generation = generation;
                default_adapter = info->Number;
            }
            hb_list_add(list, (void*)&info->Number);
        }
    }
    hb_qsv_set_default_adapter_index(default_adapter);
    hb_qsv_set_adapter_index(default_adapter);
    *qsv_adapters_list = list;
    return 0;
}

static int hb_qsv_make_adapters_details_list(const mfxAdaptersInfo* adapters_info, hb_list_t **hb_qsv_adapters_details_list)
{
    if (*hb_qsv_adapters_details_list)
    {
        hb_error("hb_qsv_make_adapters_details_list: hb_qsv_adapter_details_list is allocated already");
        return -1;
    }
    hb_list_t *list = hb_list_init();
    if (list == NULL)
    {
        hb_error("hb_qsv_make_adapters_details_list: hb_list_init() failed");
        return -1;
    }
    for (int i = 0; i < adapters_info->NumActual; i++)
    {
        mfxAdapterInfo* info = &adapters_info->Adapters[i];
        if (info)
        {
            hb_qsv_adapter_details_t* adapter_details = av_mallocz(sizeof(hb_qsv_adapter_details_t));
            if (!adapter_details)
            {
                hb_error("hb_qsv_make_adapters_details_list: adapter_details allocation failed");
                return -1;
            }
            init_adapter_details(adapter_details);
            hb_list_add(list, (void*)adapter_details);
        }
    }
    *hb_qsv_adapters_details_list = list;
    return 0;
}

static int hb_qsv_query_adapters(mfxAdaptersInfo* adapters_info)
{
    // Get number of Intel graphics adapters
    mfxU32 num_adapters_available = 0;
    mfxStatus sts = MFXQueryAdaptersNumber(&num_adapters_available);
    if (sts != MFX_ERR_NONE)
    {
        hb_error("hb_qsv_query_adapters: failed to get number of Intel graphics adapters %d", sts);
        return -1;
    }

    if (num_adapters_available > 0)
    {
        adapters_info->Adapters = av_mallocz_array(num_adapters_available, sizeof(*adapters_info->Adapters));
        if (adapters_info->Adapters)
        {
            adapters_info->NumActual = 0;
            adapters_info->NumAlloc = num_adapters_available;
            // Collect information about Intel graphics adapters
            sts = MFXQueryAdapters(NULL, adapters_info);
            if (sts != MFX_ERR_NONE)
            {
                hb_error("hb_qsv_query_adapters: failed to collect information about Intel graphics adapters %d", sts);
                return -1;
            }
        }
    }
    return 0;
}

void hb_qsv_get_free_surface_from_pool_with_range(HBQSVFramesContext* hb_enc_qsv_frames_ctx, const int start_index, const int end_index, QSVMid** out_mid, mfxFrameSurface1** out_surface)
{
    AVHWFramesContext *frames_ctx = (AVHWFramesContext*)hb_enc_qsv_frames_ctx->hw_frames_ctx->data;
    AVQSVFramesContext *frames_hwctx = frames_ctx->hwctx;

    // find the first available surface in the pool
    int count = 0;
    while(1)
    {
        if (count > 30)
        {
            hb_qsv_sleep(10); // prevent hang when all surfaces all used
            count = 0;
        }

        for (int i = start_index; i < end_index; i++)
        {
            if ((hb_enc_qsv_frames_ctx->pool[i] == 0) && (frames_hwctx->surfaces[i].Data.Locked == 0))
            {
                *out_mid = &hb_enc_qsv_frames_ctx->mids[i];
                *out_surface = &frames_hwctx->surfaces[i];
                ff_qsv_atomic_inc(&hb_enc_qsv_frames_ctx->pool[i]);
                return;
            }
        }
        count++;
    }
}

hb_buffer_t* hb_qsv_copy_frame(hb_job_t *job, AVFrame *frame, int is_vpp)
{
    hb_buffer_t *out;
    out = hb_frame_buffer_init(frame->format, frame->width, frame->height);
    hb_avframe_set_video_buffer_flags(out, frame, (AVRational){1,1});

    // alloc new frame
    out->qsv_details.frame = av_frame_alloc();
    if (!out->qsv_details.frame) {
        return out;
    }

    out->qsv_details.frame->format         = frame->format;
    out->qsv_details.frame->width          = frame->width;
    out->qsv_details.frame->height         = frame->height;
    out->qsv_details.frame->channels       = frame->channels;
    out->qsv_details.frame->channel_layout = frame->channel_layout;
    out->qsv_details.frame->nb_samples     = frame->nb_samples;

    int ret = av_frame_copy_props(out->qsv_details.frame, frame);
    if (ret < 0)
    {
        hb_error("hb_qsv_copy_frame: av_frame_copy_props error %d", ret);
    }
    
    QSVMid *mid = NULL;
    mfxFrameSurface1* output_surface = NULL;
    HBQSVFramesContext* hb_qsv_frames_ctx = NULL;

    if (is_vpp)
    {
        hb_qsv_frames_ctx = job->qsv.ctx->hb_vpp_qsv_frames_ctx;
    }
    else
    {
        hb_qsv_frames_ctx = job->qsv.ctx->hb_dec_qsv_frames_ctx;
    }

    if (!is_vpp && hb_qsv_hw_filters_are_enabled(job))
    {
        ret = hb_qsv_get_free_surface_from_pool(hb_qsv_frames_ctx, out->qsv_details.frame, &mid);
        if (ret < 0)
            return out;
        output_surface = (mfxFrameSurface1*)out->qsv_details.frame->data[3];
    }
    else
    {
        if (job->qsv.ctx && job->qsv.ctx->la_is_enabled)
        {
            hb_qsv_get_free_surface_from_pool_with_range(hb_qsv_frames_ctx, 0, HB_QSV_POOL_SURFACE_SIZE, &mid, &output_surface);
        }
        else
        {
            hb_qsv_get_free_surface_from_pool_with_range(hb_qsv_frames_ctx, 0, HB_QSV_POOL_SURFACE_SIZE - HB_QSV_POOL_ENCODER_SIZE, &mid, &output_surface);
        }
    }

    if (device_manager_handle_type == MFX_HANDLE_D3D9_DEVICE_MANAGER)
    {
        mfxFrameSurface1* input_surface = (mfxFrameSurface1*)frame->data[3];
        mfxHDLPair* input_pair = (mfxHDLPair*)input_surface->Data.MemId;
        // copy all surface fields
        *output_surface = *input_surface;
        output_surface->Info.CropW = frame->width;
        output_surface->Info.CropH = frame->height;
        if (hb_qsv_hw_filters_are_enabled(job))
        {
            output_surface->Data.MemId = mid->handle_pair;
        }
        else
        {
            // replace the mem id to mem id from the pool
            output_surface->Data.MemId = mid;
        }
        // copy input sufrace to sufrace from the pool
        IDirect3DDevice9 *pDevice = NULL;
        HANDLE handle;

        HRESULT result = lock_device((IDirect3DDeviceManager9 *)device_manager_handle, 1, &pDevice, &handle);
        if (FAILED(result))
        {
            hb_error("hb_qsv_copy_frame: lock_device failed %d", result);
            return out;
        }
        result = IDirect3DDevice9_StretchRect(pDevice, input_pair->first, 0, mid->handle_pair->first, 0, D3DTEXF_LINEAR);
        if (FAILED(result))
        {
            hb_error("hb_qsv_copy_frame: IDirect3DDevice9_StretchRect failed %d", result);
            return out;
        }
        result = unlock_device((IDirect3DDeviceManager9 *)device_manager_handle, handle);
        if (FAILED(result))
        {
            hb_error("hb_qsv_copy_frame: unlock_device failed %d", result);
            return out;
        }
    }
    else if (device_manager_handle_type == MFX_HANDLE_D3D11_DEVICE)
    {
        mfxFrameSurface1* input_surface = (mfxFrameSurface1*)frame->data[3];
        mfxHDLPair* input_pair = (mfxHDLPair*)input_surface->Data.MemId;
        // Need to pass 0 instead of MFX_INFINITE to DirectX as index of surface
        int input_index = (int)(intptr_t)input_pair->second == MFX_INFINITE ? 0 : (int)(intptr_t)input_pair->second;
        int output_index = (int)(intptr_t)mid->handle_pair->second == MFX_INFINITE ? 0 : (int)(intptr_t)mid->handle_pair->second;
        // copy all surface fields
        *output_surface = *input_surface;
        output_surface->Info.CropW = frame->width;
        output_surface->Info.CropH = frame->height;
        if (hb_qsv_hw_filters_are_enabled(job))
        {
            // Make sure that we pass handle_pair to scale_qsv
            output_surface->Data.MemId = mid->handle_pair;
        }
        else
        {
            // Make sure that we pass QSVMid to QSV encoder
            output_surface->Data.MemId = mid;
        }
        // copy input sufrace to sufrace from the pool
        ID3D11DeviceContext_CopySubresourceRegion(device_context, mid->handle_pair->first, output_index, 0, 0, 0, input_pair->first, input_index, NULL);
        ID3D11DeviceContext_Flush(device_context);
    }
    else
    {
        hb_error("hb_qsv_copy_frame: incorrect mfx impl");
        return out;
    }
    out->qsv_details.frame->data[3] = (uint8_t*)output_surface;
    out->qsv_details.qsv_frames_ctx = hb_qsv_frames_ctx;
    out->qsv_details.qsv_atom       = 0;
    out->qsv_details.ctx            = job->qsv.ctx;
    return out;
}

static int qsv_get_buffer(AVCodecContext *s, AVFrame *frame, int flags)
{
    int ret = -1;
    if(s->hw_frames_ctx)
    {
        ret = av_hwframe_get_buffer(s->hw_frames_ctx, frame, 0);
    }
    return ret;
}

void hb_qsv_uninit_dec(AVCodecContext *s)
{
    if(s && s->hw_frames_ctx)
        av_buffer_unref(&s->hw_frames_ctx);
}

void hb_qsv_uninit_enc(hb_job_t *job)
{
    if(job->qsv.ctx && job->qsv.ctx->hb_dec_qsv_frames_ctx)
    {
        av_buffer_unref(&job->qsv.ctx->hb_dec_qsv_frames_ctx->hw_frames_ctx);
        job->qsv.ctx->hb_dec_qsv_frames_ctx->hw_frames_ctx = NULL;
        av_free(job->qsv.ctx->hb_dec_qsv_frames_ctx);
        job->qsv.ctx->hb_dec_qsv_frames_ctx = NULL;
    }
    if(job->qsv.ctx && job->qsv.ctx->hb_vpp_qsv_frames_ctx)
    {
        av_buffer_unref(&job->qsv.ctx->hb_vpp_qsv_frames_ctx->hw_frames_ctx);
        job->qsv.ctx->hb_vpp_qsv_frames_ctx->hw_frames_ctx = NULL;
        av_free(job->qsv.ctx->hb_vpp_qsv_frames_ctx);
        job->qsv.ctx->hb_vpp_qsv_frames_ctx = NULL;
    }
    if (device_context)
    {
        ID3D11DeviceContext_Release(device_context);
        device_context = NULL;
    }
    if (job->qsv.ctx)
    {
        job->qsv.ctx->hb_hw_device_ctx = NULL;
        if (job->qsv.ctx->qsv_device)
        {
            av_free(job->qsv.ctx->qsv_device);
            job->qsv.ctx->qsv_device = NULL;
        }
    }
    device_manager_handle = NULL;
}

static int qsv_device_init(hb_job_t *job)
{
    int err;
    AVDictionary *dict = NULL;

    if (job->qsv.ctx && job->qsv.ctx->qsv_device)
    {
        err = av_dict_set(&dict, "child_device", job->qsv.ctx->qsv_device, 0);
        if (err < 0)
            return err;
    }
    else
    {
        av_dict_set(&dict, "vendor", "0x8086", 0);
    }
    av_dict_set(&dict, "child_device_type", "d3d11va", 0);

    err = av_hwdevice_ctx_create(&job->qsv.ctx->hb_hw_device_ctx, AV_HWDEVICE_TYPE_QSV,
                                 0, dict, 0);
    if (err < 0) {
        hb_error("qsv_device_init: error creating a QSV device %d", err);
        goto err_out;
    }

err_out:
    if (dict)
        av_dict_free(&dict);

    return err;
}

int hb_qsv_parse_adapter_index(hb_job_t *job)
{
    int ret = 0;

    if (job->encoder_options != NULL && *job->encoder_options)
    {
        hb_dict_t *options_list;
        options_list = hb_encopts_to_dict(job->encoder_options, job->vcodec);
        hb_dict_iter_t iter;
        for (iter  = hb_dict_iter_init(options_list);
            iter != HB_DICT_ITER_DONE;
            iter  = hb_dict_iter_next(options_list, iter))
        {
            const char *key = hb_dict_iter_key(iter);
            if (!strcasecmp(key, "gpu"))
            {
                hb_value_t *value = hb_dict_iter_value(iter);
                char *str = hb_value_get_string_xform(value);
                int dx_index = hb_qsv_atoi(str, &ret);
                free(str);
                if (!ret)
                {
                    hb_qsv_param_parse_dx_index(job, dx_index);
                }
            }
        }
        hb_dict_free(&options_list);
    }
    return 0;
}

int hb_create_ffmpeg_pool(hb_job_t *job, int coded_width, int coded_height, enum AVPixelFormat sw_pix_fmt, int pool_size, int extra_hw_frames, AVBufferRef **out_hw_frames_ctx)
{
    AVHWFramesContext *frames_ctx;
    AVQSVFramesContext *frames_hwctx;

    AVBufferRef *hw_frames_ctx = *out_hw_frames_ctx;

    int ret = 0;

    if (job->qsv.ctx && !job->qsv.ctx->hb_hw_device_ctx) {
        // parse and use user-specified encoder options for decoder, if present
        if (job->encoder_options != NULL && *job->encoder_options)
        {
            hb_dict_t *options_list;
            options_list = hb_encopts_to_dict(job->encoder_options, job->vcodec);

            hb_dict_iter_t iter;
            for (iter  = hb_dict_iter_init(options_list);
                iter != HB_DICT_ITER_DONE;
                iter  = hb_dict_iter_next(options_list, iter))
            {
                const char *key = hb_dict_iter_key(iter);
                if ((!strcasecmp(key, "scalingmode") || !strcasecmp(key, "vpp-sm")) && hb_qsv_hw_filters_are_enabled(job))
                {
                    hb_qsv_info_t *info = hb_qsv_encoder_info_get(hb_qsv_get_adapter_index(), job->vcodec);
                    if (info && (info->capabilities & HB_QSV_CAP_VPP_SCALING))
                    {
                        hb_value_t *value = hb_dict_iter_value(iter);
                        char *mode_key = hb_value_get_string_xform(value);
                        hb_triplet_t *mode = NULL;
                        if (mode_key != NULL)
                        {
                            mode = hb_triplet4key(hb_qsv_vpp_scale_modes, mode_key);
                        }
                        if (mode != NULL)
                        {
                            job->qsv.ctx->vpp_scale_mode = mode->key;
                        }
                    }
                }
                if ((!strcasecmp(key, "interpolationmethod") || !strcasecmp(key, "vpp-im")) && hb_qsv_hw_filters_are_enabled(job))
                {
                    hb_qsv_info_t *info = hb_qsv_encoder_info_get(hb_qsv_get_adapter_index(), job->vcodec);
                    if (info && (info->capabilities & HB_QSV_CAP_VPP_INTERPOLATION))
                    {
                        hb_value_t *value = hb_dict_iter_value(iter);
                        char *mode_key = hb_value_get_string_xform(value);
                        hb_triplet_t *mode = NULL;
                        if (mode_key != NULL)
                        {
                            mode = hb_triplet4key(hb_qsv_vpp_interpolation_methods, mode_key);
                        }
                        if (mode != NULL)
                        {
                            job->qsv.ctx->vpp_interpolation_method = mode->key;
                        }
                    }
                }
            }
            hb_dict_free(&options_list);
        }

        if (!job->qsv.ctx->qsv_device)
            hb_qsv_param_parse_dx_index(job, hb_qsv_get_adapter_index());

        ret = qsv_device_init(job);
        if (ret < 0)
            return ret;
    }

    av_buffer_unref(&hw_frames_ctx);
    hw_frames_ctx = av_hwframe_ctx_alloc(job->qsv.ctx->hb_hw_device_ctx);
    if (!hw_frames_ctx)
        return AVERROR(ENOMEM);

    *out_hw_frames_ctx = hw_frames_ctx;

    frames_ctx   = (AVHWFramesContext*)hw_frames_ctx->data;
    frames_hwctx = frames_ctx->hwctx;

    frames_ctx->width             = FFALIGN(coded_width,  32);
    frames_ctx->height            = FFALIGN(coded_height, 32);
    frames_ctx->format            = AV_PIX_FMT_QSV;
    frames_ctx->sw_format         = sw_pix_fmt;
    frames_ctx->initial_pool_size = pool_size;
    if (extra_hw_frames >= 0)
        frames_ctx->initial_pool_size += extra_hw_frames;
    frames_hwctx->frame_type      = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;

    ret = av_hwframe_ctx_init(hw_frames_ctx);
    if (ret < 0) {
        hb_error("hb_create_ffmpeg_pool: av_hwframe_ctx_init failed %d", ret);
        return ret;
    }
    return 0;
}

int hb_qsv_hw_frames_init(AVCodecContext *s)
{
    AVHWFramesContext *frames_ctx;
    AVQSVFramesContext *frames_hwctx;
    AVBufferRef *hw_frames_ctx;
    int ret;

    hb_job_t *job = s->opaque;
    if (!job) {
        hb_error("hb_qsv_hw_frames_init: job is NULL");
        return -1;
    }

    HBQSVFramesContext *hb_dec_qsv_frames_ctx = job->qsv.ctx->hb_dec_qsv_frames_ctx;
    int                           coded_width = s->coded_width;
    int                          coded_height = s->coded_height;
    enum AVPixelFormat             sw_pix_fmt = s->sw_pix_fmt;
    int                       extra_hw_frames = s->extra_hw_frames;
    AVBufferRef           **out_hw_frames_ctx = &s->hw_frames_ctx;

    ret = hb_create_ffmpeg_pool(job, coded_width, coded_height, sw_pix_fmt, HB_QSV_POOL_FFMPEG_SURFACE_SIZE, extra_hw_frames, out_hw_frames_ctx);
    if (ret < 0) {
        hb_error("hb_qsv_hw_frames_init: hb_create_ffmpeg_pool decoder failed %d", ret);
        return ret;
    }

    hw_frames_ctx = *out_hw_frames_ctx;
    frames_ctx   = (AVHWFramesContext*)hw_frames_ctx->data;
    frames_hwctx = frames_ctx->hwctx;
    mfxHDLPair* handle_pair = (mfxHDLPair*)frames_hwctx->surfaces[0].Data.MemId;
    hb_dec_qsv_frames_ctx->input_texture = ((size_t)handle_pair->second != MFX_INFINITE) ? handle_pair->first : NULL;

    ret = hb_create_ffmpeg_pool(job, coded_width, coded_height, sw_pix_fmt, HB_QSV_POOL_SURFACE_SIZE, extra_hw_frames, &hb_dec_qsv_frames_ctx->hw_frames_ctx);
    if (ret < 0) {
        hb_error("hb_qsv_hw_frames_init: hb_create_ffmpeg_pool qsv surface allocation failed %d", ret);
        return ret;
    }

    /* allocate the memory ids for the external frames */
    av_buffer_unref(&hb_dec_qsv_frames_ctx->mids_buf);
    hb_dec_qsv_frames_ctx->mids_buf = hb_qsv_create_mids(hb_dec_qsv_frames_ctx->hw_frames_ctx);
    if (!hb_dec_qsv_frames_ctx->mids_buf)
        return AVERROR(ENOMEM);
    hb_dec_qsv_frames_ctx->mids    = (QSVMid*)hb_dec_qsv_frames_ctx->mids_buf->data;
    hb_dec_qsv_frames_ctx->nb_mids = frames_hwctx->nb_surfaces;
    memset(hb_dec_qsv_frames_ctx->pool, 0, hb_dec_qsv_frames_ctx->nb_mids * sizeof(hb_dec_qsv_frames_ctx->pool[0]));

    ret = hb_qsv_get_dx_device(job);
    if (ret < 0) {
        hb_error("qsv_init: hb_qsv_get_dx_device failed %d", ret);
        return ret;
    }
    return 0;
}

int hb_qsv_get_buffer(AVCodecContext *s, AVFrame *frame, int flags)
{
    if (frame->format == AV_PIX_FMT_QSV)
        return qsv_get_buffer(s, frame, flags);

    return avcodec_default_get_buffer2(s, frame, flags);
}

enum AVPixelFormat hb_qsv_get_format(AVCodecContext *s, const enum AVPixelFormat *pix_fmts)
{
    while (*pix_fmts != AV_PIX_FMT_NONE) {
        if (*pix_fmts == AV_PIX_FMT_QSV) {
            int ret = hb_qsv_hw_frames_init(s);
            if (ret < 0) {
                hb_error("hb_qsv_get_format: QSV hwaccel initialization failed");
                return AV_PIX_FMT_NONE;
            }
            if (s->hw_frames_ctx) {
                s->hw_frames_ctx = av_buffer_ref(s->hw_frames_ctx);
                if (!s->hw_frames_ctx)
                    return AV_PIX_FMT_NONE;
            }
            return AV_PIX_FMT_QSV;
        }

        pix_fmts++;
    }

    hb_error("hb_qsv_get_format: the QSV pixel format not offered in get_format()");
    return AV_PIX_FMT_NONE;
}

int hb_qsv_preset_is_zero_copy_enabled(const hb_dict_t *job_dict)
{
    hb_dict_t *video_dict, *qsv, *encoder;
    int qsv_encoder_enabled = 0;
    int qsv_decoder_enabled = 0;
    video_dict = hb_dict_get(job_dict, "Video");
    if(video_dict)
    {
        encoder = hb_dict_get(video_dict, "Encoder");
        if(encoder)
        {
            if (hb_value_type(encoder) == HB_VALUE_TYPE_STRING)
            {
                if(!strcasecmp(hb_value_get_string(encoder), "qsv_h264") ||
                    !strcasecmp(hb_value_get_string(encoder), "qsv_h265"))
                {
                    qsv_encoder_enabled = 1;
                }
            }
        }
        qsv = hb_dict_get(video_dict, "QSV");
        if (qsv != NULL)
        {
            hb_dict_t *decode;
            decode = hb_dict_get(qsv, "Decode");
            if(decode)
            {
                if (hb_value_type(decode) == HB_VALUE_TYPE_BOOL)
                {
                    qsv_decoder_enabled = hb_value_get_bool(decode);
                }
            }
        }
    }
    return (qsv_encoder_enabled && qsv_decoder_enabled);
}

int hb_qsv_sanitize_filter_list(hb_job_t *job)
{
    /*
     * When QSV's VPP is used for filtering, not all CPU filters
     * are supported, so we need to do a little extra setup here.
     */
    if (job->vcodec & HB_VCODEC_QSV_MASK)
    {
        int i = 0;
        int num_cpu_filters = 0;
        if (job->list_filter != NULL && hb_list_count(job->list_filter) > 0)
        {
            for (i = 0; i < hb_list_count(job->list_filter); i++)
            {
                hb_filter_object_t *filter = hb_list_item(job->list_filter, i);

                switch (filter->id)
                {
                    // cropping and scaling always done via VPP filter
                    case HB_FILTER_CROP_SCALE:
                        break;

                    case HB_FILTER_DEINTERLACE:
                    case HB_FILTER_ROTATE:
                    case HB_FILTER_RENDER_SUB:
                    case HB_FILTER_AVFILTER:
                        num_cpu_filters++;
                        break;
                    default:
                        num_cpu_filters++;
                        break;
                }
            }
        }
        job->qsv.ctx->num_cpu_filters = num_cpu_filters;
        job->qsv.ctx->qsv_filters_are_enabled = ((hb_list_count(job->list_filter) == 1) && hb_qsv_full_path_is_enabled(job)) ? 1 : 0;
        if (job->qsv.ctx->qsv_filters_are_enabled)
        {
            job->qsv.ctx->hb_vpp_qsv_frames_ctx = av_mallocz(sizeof(HBQSVFramesContext));
            if (!job->qsv.ctx->hb_vpp_qsv_frames_ctx)
            {
                hb_error( "sanitize_qsv: HBQSVFramesContext vpp alloc failed" );
                return 1;
            }
        }
    }
    return 0;
}

#else // other OS

int hb_qsv_get_platform(int adapter_index)
{
    return hb_get_cpu_platform();
}

int hb_qsv_info_init()
{
    if (g_qsv_adapters_list)
    {
        hb_error("hb_qsv_info_init: qsv_adapters_list is allocated already");
        return -1;
    }
    g_qsv_adapters_list = hb_list_init();
    if (g_qsv_adapters_list == NULL)
    {
        hb_error("hb_qsv_info_init: g_qsv_adapters_list allocation failed");
        return -1;
    }
    if (g_qsv_adapters_details_list)
    {
        hb_error("hb_qsv_info_init: g_qsv_adapters_details_list is allocated already");
        return -1;
    }
    g_qsv_adapters_details_list = hb_list_init();
    if (g_qsv_adapters_details_list == NULL)
    {
        hb_error("hb_qsv_info_init: g_qsv_adapters_details_list allocation failed");
        return -1;
    }
    static int adapter_index = 0;
    static hb_qsv_adapter_details_t adapter_details;
    init_adapter_details(&adapter_details);
    hb_list_add(g_qsv_adapters_list, (void*)&adapter_index);
    hb_list_add(g_qsv_adapters_details_list, (void*)&adapter_details);
    hb_qsv_collect_adapters_details(g_qsv_adapters_list, g_qsv_adapters_details_list);
    return 0;
}

int hb_create_ffmpeg_pool(hb_job_t *job, int coded_width, int coded_height, enum AVPixelFormat sw_pix_fmt, int pool_size, int extra_hw_frames, AVBufferRef **out_hw_frames_ctx)
{
    return -1;
}

int hb_qsv_hw_frames_init(AVCodecContext *s)
{
    return -1;
}

hb_buffer_t* hb_qsv_copy_frame(hb_job_t *job, AVFrame *frame, int is_vpp)
{
    return NULL;
}

int hb_qsv_get_free_surface_from_pool(HBQSVFramesContext* hb_enc_qsv_frames_ctx, AVFrame* frame, QSVMid** out_mid)
{
    return -1;
}

void hb_qsv_get_free_surface_from_pool_with_range(HBQSVFramesContext* hb_enc_qsv_frames_ctx, const int start_index, const int end_index, QSVMid** out_mid, mfxFrameSurface1** out_surface)
{
    return;
}

int hb_qsv_replace_surface_mid(HBQSVFramesContext* hb_qsv_frames_ctx, const QSVMid *mid, mfxFrameSurface1 *surface)
{
    return -1;
}

enum AVPixelFormat hb_qsv_get_format(AVCodecContext *s, const enum AVPixelFormat *pix_fmts)
{
    return AV_PIX_FMT_NONE;
}

int hb_qsv_get_buffer(AVCodecContext *s, AVFrame *frame, int flags)
{
    return -1;
}

void hb_qsv_uninit_dec(AVCodecContext *s)
{
}

void hb_qsv_uninit_enc(hb_job_t *job)
{
}

int hb_qsv_preset_is_zero_copy_enabled(const hb_dict_t *job_dict)
{
    return 0;
}

static int hb_dxva2_device_check()
{
    return -1;
}

static int hb_d3d11va_device_check()
{
    return -1;
}

int hb_qsv_get_mid_by_surface_from_pool(HBQSVFramesContext* hb_enc_qsv_frames_ctx, mfxFrameSurface1 *surface, QSVMid **out_mid)
{
}

int hb_qsv_release_surface_from_pool_by_surface_pointer(HBQSVFramesContext* hb_enc_qsv_frames_ctx, const mfxFrameSurface1 *surface)
{
    return -1;
}

#endif

hb_qsv_context* hb_qsv_context_init()
{
    hb_qsv_context *ctx;
    ctx = av_mallocz(sizeof(hb_qsv_context));
    if (!ctx)
    {
        hb_error( "hb_qsv_context_init: qsv ctx alloc failed" );
        return NULL;
    }
    hb_qsv_add_context_usage(ctx, 0);
    return ctx;
}

void hb_qsv_context_uninit(hb_job_t *job)
{
    hb_qsv_context *ctx = job->qsv.ctx;
    if ( ctx == NULL )
    {
        hb_error( "hb_qsv_context_uninit: ctx is NULL" );
        return;
    }
    /* QSV context cleanup and MFXClose */
    hb_qsv_context_clean(ctx, hb_qsv_full_path_is_enabled(job));
    av_free(ctx);
    job->qsv.ctx = NULL;

    /* Structures below are needed until the end life of the process
        It has been collected once in hb_qsv_info_init() and no need to recollect every time.
    */
#if 0
    if (g_qsv_adapters_details_list)
    {
#if defined(_WIN32) || defined(__MINGW32__)
        hb_qsv_free_adapters_details(g_qsv_adapters_details_list);
#endif
        hb_list_close(&g_qsv_adapters_details_list);
        g_qsv_adapters_details_list = NULL;
    }
    if (g_qsv_adapters_list)
    {
        hb_list_close(&g_qsv_adapters_list);
        g_qsv_adapters_list = NULL;
    }
#if defined(_WIN32) || defined(__MINGW32__)
    if (g_qsv_adapters_info.Adapters)
    {
        av_free(g_qsv_adapters_info.Adapters);
    }
    g_qsv_adapters_info.Adapters = NULL;
    g_qsv_adapters_info.NumAlloc = 0;
    g_qsv_adapters_info.NumActual = 0;
#endif
#endif
    g_adapter_index = hb_qsv_get_default_adapter_index();
}

#else // HB_PROJECT_FEATURE_QSV

int hb_qsv_available()
{
    return -1;
}

#endif // HB_PROJECT_FEATURE_QSV
