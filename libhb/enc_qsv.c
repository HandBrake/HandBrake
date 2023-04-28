
/* ********************************************************************* *\

Copyright (C) 2013 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

\* ********************************************************************* */

#include "handbrake/project.h"

#if HB_PROJECT_FEATURE_QSV

#include "handbrake/handbrake.h"
#include "handbrake/nal_units.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/qsv_common.h"
#include "handbrake/qsv_memory.h"
#include "handbrake/h264_common.h"
#include "handbrake/h265_common.h"

#include "libavutil/hwcontext_qsv.h"
#include "libavutil/hwcontext.h"
#include "vpl/mfxvideo.h"

/*
 * The frame info struct remembers information about each frame across calls to
 * the encoder. Since frames are uniquely identified by their timestamp, we use
 * some bits of the timestamp as an index. The LSB is chosen so that two
 * successive frames will have different values in the bits over any plausible
 * range of frame rates (starting with bit 8 allows any frame rate slower than
 * 352fps). The MSB determines the size of the array. It is chosen so that two
 * frames can't use the same slot during the encoder's max frame delay so that,
 * up to some minimum frame rate, frames are guaranteed to map to different
 * slots (an MSB of 17 which is 2^(17-8+1) = 1024 slots guarantees no collisions
 * down to a rate of 0.7 fps).
 */
#define FRAME_INFO_MAX2 (8)  // 2^8  = 256;  90000/256    = 352 frames/sec
#define FRAME_INFO_MIN2 (17) // 2^17 = 128K; 90000/131072 = 0.7 frames/sec
#define FRAME_INFO_SIZE (1 << (FRAME_INFO_MIN2 - FRAME_INFO_MAX2 + 1))
#define FRAME_INFO_MASK (FRAME_INFO_SIZE - 1)

int  encqsvInit (hb_work_object_t*, hb_job_t*);
int  encqsvWork (hb_work_object_t*, hb_buffer_t**, hb_buffer_t**);
void encqsvClose(hb_work_object_t*);

hb_work_object_t hb_encqsv =
{
    WORK_ENCQSV,
    "Quick Sync Video encoder (Intel Media SDK)",
    encqsvInit,
    encqsvWork,
    encqsvClose
};

struct hb_work_private_s
{
    hb_job_t           * job;
    uint32_t             frames_in;
    uint32_t             frames_out;
    int64_t              last_start;

    hb_qsv_param_t       param;
    hb_qsv_space         enc_space;
    hb_qsv_info_t      * qsv_info;

    hb_chapter_queue_t * chapter_queue;

#define BFRM_DELAY_MAX 16
    int                * init_delay;
    int                  bfrm_delay;
    int64_t              init_pts[BFRM_DELAY_MAX + 1];
    hb_list_t          * list_dts;

    int64_t              frame_duration[FRAME_INFO_SIZE];

    int                  async_depth;
    int                  max_async_depth;

    // if encode-only, system memory used
    int                  is_sys_mem;
    mfxSession           mfx_session;

    // whether the encoder is initialized
    int                  init_done;

    hb_list_t          * delayed_processing;
    hb_buffer_list_t     encoded_frames;

    hb_list_t          * loaded_plugins;
};

static void hb_qsv_add_new_dts(hb_list_t *list, int64_t new_dts)
{
    if (list != NULL)
    {
        int64_t *item = malloc(sizeof(int64_t));
        if (item != NULL)
        {
            *item = new_dts;
            hb_list_add(list, item);
        }
    }
}

static int64_t hb_qsv_pop_next_dts(hb_list_t *list)
{
    int64_t next_dts = INT64_MIN;
    if (list != NULL && hb_list_count(list) > 0)
    {
        int64_t *item = hb_list_item(list, 0);
        if (item != NULL)
        {
            next_dts = *item;
            hb_list_rem(list, item);
            free(item);
        }
    }
    return next_dts;
}

static void save_frame_duration(hb_work_private_t *pv, hb_buffer_t *buf)
{
    int i = (buf->s.start >> FRAME_INFO_MAX2) & FRAME_INFO_MASK;
    pv->frame_duration[i] = buf->s.stop - buf->s.start;
}

static int64_t get_frame_duration(hb_work_private_t *pv, hb_buffer_t *buf)
{
    int i = (buf->s.start >> FRAME_INFO_MAX2) & FRAME_INFO_MASK;
    return pv->frame_duration[i];
}

static inline int64_t rescale(hb_rational_t q, int b)
{
    return av_rescale(q.num, b, q.den);
}

static const char* hyper_encode_name(const int hyper_encode_mode)
{
    switch (hyper_encode_mode)
    {
        case MFX_HYPERMODE_OFF:
            return "(HyperEncode Off)";

        case MFX_HYPERMODE_ON:
            return "(HyperEncode On)";

        case MFX_HYPERMODE_ADAPTIVE:
            return "(HyperEncode Adaptive)";

        default:
            return NULL;
    }
}

static int log_encoder_params(const hb_work_private_t *pv, const mfxVideoParam *videoParam)
{
    const mfxExtCodingOption  *option1 = NULL;
    const mfxExtCodingOption2 *option2 = NULL;
    const mfxExtHyperModeParam *extHyperModeOption = NULL;

    for (int i = 0; i < videoParam->NumExtParam; i++)
    {
        mfxExtCodingOption *option = (mfxExtCodingOption*)videoParam->ExtParam[i];
        if (option->Header.BufferId == MFX_EXTBUFF_CODING_OPTION)
        {
            option1 = (mfxExtCodingOption*)videoParam->ExtParam[i];
        }
        else if (option->Header.BufferId == MFX_EXTBUFF_CODING_OPTION2)
        {
            option2 = (mfxExtCodingOption2*)videoParam->ExtParam[i];
        }
        else if (option->Header.BufferId == MFX_EXTBUFF_HYPER_MODE_PARAM)
        {
            extHyperModeOption = (mfxExtHyperModeParam*)videoParam->ExtParam[i];
        }
        else if (option->Header.BufferId != MFX_EXTBUFF_VIDEO_SIGNAL_INFO &&
                 option->Header.BufferId != MFX_EXTBUFF_CHROMA_LOC_INFO &&
                 option->Header.BufferId != MFX_EXTBUFF_MASTERING_DISPLAY_COLOUR_VOLUME &&
                 option->Header.BufferId != MFX_EXTBUFF_CONTENT_LIGHT_LEVEL_INFO &&
                 option->Header.BufferId != MFX_EXTBUFF_AV1_BITSTREAM_PARAM)
        {
            hb_log("Unknown Header.BufferId=%d", option->Header.BufferId);
        }
    }

    // log code path and main output settings
    hb_log("encqsvInit: using%s%s%s path",
           pv->is_sys_mem ? " encode-only" : " full QSV",
           videoParam->mfx.LowPower == MFX_CODINGOPTION_ON ? " (LowPower)" : "",
           extHyperModeOption != NULL ? hyper_encode_name(extHyperModeOption->Mode) : "");
    hb_log("encqsvInit: %s %s profile @ level %s",
           hb_qsv_codec_name  (videoParam->mfx.CodecId),
           hb_qsv_profile_name(videoParam->mfx.CodecId, videoParam->mfx.CodecProfile),
           hb_qsv_level_name  (videoParam->mfx.CodecId, videoParam->mfx.CodecLevel));
    hb_log("encqsvInit: TargetUsage %"PRIu16" AsyncDepth %"PRIu16"",
           videoParam->mfx.TargetUsage, videoParam->AsyncDepth);
    hb_log("encqsvInit: GopRefDist %"PRIu16" GopPicSize %"PRIu16" NumRefFrame %"PRIu16" IdrInterval %"PRIu16"",
           videoParam->mfx.GopRefDist, videoParam->mfx.GopPicSize, videoParam->mfx.NumRefFrame, videoParam->mfx.IdrInterval);

    if (pv->qsv_info->capabilities & HB_QSV_CAP_B_REF_PYRAMID)
    {
        hb_log("encqsvInit: BFramesMax %d BRefType %s",
               videoParam->mfx.GopRefDist > 1 ?
               videoParam->mfx.GopRefDist - 1 : 0,
               pv->param.gop.b_pyramid ? "pyramid" : "off");
    }
    else
    {
        hb_log("encqsvInit: BFramesMax %d",
               videoParam->mfx.GopRefDist > 1 ?
               videoParam->mfx.GopRefDist - 1 : 0);
    }

    if (option2 && (option2->AdaptiveI != MFX_CODINGOPTION_OFF ||
        option2->AdaptiveB != MFX_CODINGOPTION_OFF))
    {
        if (videoParam->mfx.GopRefDist > 1)
        {
            hb_log("encqsvInit: AdaptiveI %s AdaptiveB %s",
                hb_qsv_codingoption_get_name(option2->AdaptiveI),
                hb_qsv_codingoption_get_name(option2->AdaptiveB));
        }
        else
        {
            hb_log("encqsvInit: AdaptiveI %s",
                hb_qsv_codingoption_get_name(option2->AdaptiveI));
        }
    }

    if (videoParam->mfx.RateControlMethod == MFX_RATECONTROL_CQP)
    {
        char qpi[7], qpp[9], qpb[9];
        snprintf(qpi, sizeof(qpi),  "QPI %"PRIu16"", videoParam->mfx.QPI);
        snprintf(qpp, sizeof(qpp), " QPP %"PRIu16"", videoParam->mfx.QPP);
        snprintf(qpb, sizeof(qpb), " QPB %"PRIu16"", videoParam->mfx.QPB);
        hb_log("encqsvInit: RateControlMethod CQP with %s%s%s", qpi,
               videoParam->mfx.GopPicSize > 1 ? qpp : "",
               videoParam->mfx.GopRefDist > 1 ? qpb : "");
    }
    else
    {
        switch (videoParam->mfx.RateControlMethod)
        {
            case MFX_RATECONTROL_LA:
                hb_log("encqsvInit: RateControlMethod LA TargetKbps %"PRIu16" BRCParamMultiplier %"PRIu16" LookAheadDepth %"PRIu16"",
                       videoParam->mfx.TargetKbps, videoParam->mfx.BRCParamMultiplier, (option2 != NULL) ? option2->LookAheadDepth : 0);
                break;
            case MFX_RATECONTROL_LA_ICQ:
                hb_log("encqsvInit: RateControlMethod LA_ICQ ICQQuality %"PRIu16" LookAheadDepth %"PRIu16"",
                       videoParam->mfx.ICQQuality, (option2 != NULL) ? option2->LookAheadDepth : 0);
                break;
            case MFX_RATECONTROL_ICQ:
                hb_log("encqsvInit: RateControlMethod ICQ ICQQuality %"PRIu16"",
                       videoParam->mfx.ICQQuality);
                break;
            case MFX_RATECONTROL_CBR:
            case MFX_RATECONTROL_VBR:
                hb_log("encqsvInit: RateControlMethod %s TargetKbps %"PRIu16" MaxKbps %"PRIu16" BufferSizeInKB %"PRIu16" InitialDelayInKB %"PRIu16" BRCParamMultiplier %"PRIu16"",
                       videoParam->mfx.RateControlMethod == MFX_RATECONTROL_CBR ? "CBR" : "VBR",
                       videoParam->mfx.TargetKbps,     videoParam->mfx.MaxKbps,
                       videoParam->mfx.BufferSizeInKB, videoParam->mfx.InitialDelayInKB, videoParam->mfx.BRCParamMultiplier);
                break;
            default:
                hb_log("encqsvInit: invalid rate control method %"PRIu16"",
                       videoParam->mfx.RateControlMethod);
                return -1;
        }
    }

    if (option2 && (videoParam->mfx.RateControlMethod == MFX_RATECONTROL_LA ||
        videoParam->mfx.RateControlMethod == MFX_RATECONTROL_LA_ICQ))
    {
        switch (option2->LookAheadDS)
        {
            case MFX_LOOKAHEAD_DS_UNKNOWN:
                hb_log("encqsvInit: LookAheadDS unknown (auto)");
                break;
            case MFX_LOOKAHEAD_DS_OFF: // default
                break;
            case MFX_LOOKAHEAD_DS_2x:
                hb_log("encqsvInit: LookAheadDS 2x");
                break;
            case MFX_LOOKAHEAD_DS_4x:
                hb_log("encqsvInit: LookAheadDS 4x");
                break;
            default:
                hb_log("encqsvInit: invalid LookAheadDS value 0x%"PRIx16"",
                       option2->LookAheadDS);
                break;
        }
    }

    switch (videoParam->mfx.FrameInfo.PicStruct)
    {
        case MFX_PICSTRUCT_PROGRESSIVE: // default
            break;
        case MFX_PICSTRUCT_FIELD_TFF:
            hb_log("encqsvInit: PicStruct top field first");
            break;
        case MFX_PICSTRUCT_FIELD_BFF:
            hb_log("encqsvInit: PicStruct bottom field first");
            break;
        default:
            hb_error("encqsvInit: invalid PicStruct value 0x%"PRIx16"",
                     videoParam->mfx.FrameInfo.PicStruct);
            return -1;
    }

    if (videoParam->mfx.CodecId == MFX_CODEC_AVC)
    {
        if (option1 && (option1->CAVLC != MFX_CODINGOPTION_OFF))
        {
            hb_log("encqsvInit: CAVLC %s",
                hb_qsv_codingoption_get_name(option1->CAVLC));
        }
    }

    if (option2 && (option2->ExtBRC != MFX_CODINGOPTION_OFF))
    {
        hb_log("encqsvInit: ExtBRC %s",
            hb_qsv_codingoption_get_name(option2->ExtBRC));
    }

    if (option2 && (option2->MBBRC != MFX_CODINGOPTION_OFF))
    {
        hb_log("encqsvInit: MBBRC %s",
            hb_qsv_codingoption_get_name(option2->MBBRC));
    }

    if (option2)
    {
        switch (option2->Trellis)
        {
            case MFX_TRELLIS_OFF: // default
                break;
            case MFX_TRELLIS_UNKNOWN:
                hb_log("encqsvInit: Trellis unknown (auto)");
                break;
            default:
                hb_log("encqsvInit: Trellis on (%s%s%s)",
                       (option2->Trellis & MFX_TRELLIS_I) ? "I" : "",
                       (option2->Trellis & MFX_TRELLIS_P) &&
                       (videoParam->mfx.GopPicSize > 1)    ? "P" : "",
                       (option2->Trellis & MFX_TRELLIS_B) &&
                       (videoParam->mfx.GopRefDist > 1)    ? "B" : "");
                break;
        }
    }

    if (option2 && (option2->RepeatPPS != MFX_CODINGOPTION_OFF))
    {
        hb_log("encqsvInit: RepeatPPS %s",
            hb_qsv_codingoption_get_name(option2->RepeatPPS));
    }

    return 0;
}

static void qsv_handle_breftype(hb_work_private_t *pv)
{
    /*
     * If B-pyramid is not possible (not supported, incompatible profile, etc.)
     * we don't need to adjust any settings, just sanitize it to off and return.
     */
    if (!(pv->qsv_info->capabilities & HB_QSV_CAP_B_REF_PYRAMID))
    {
        /* B-pyramid support not implemented */
        goto unsupported;
    }
    else if (pv->param.videoParam->mfx.GopPicSize &&
             pv->param.videoParam->mfx.GopPicSize <= 3)
    {
        /* GOP size must be at least 4 for B-pyramid */
        goto unsupported;
    }
    else if (pv->param.videoParam->mfx.GopRefDist &&
             pv->param.videoParam->mfx.GopRefDist <= 2)
    {
        /* We need 2 consecutive B-frames for B-pyramid (GopRefDist >= 3) */
        goto unsupported;
    }
    else if (pv->param.videoParam->mfx.CodecId == MFX_CODEC_AVC)
    {
        switch (pv->param.videoParam->mfx.CodecProfile)
        {
            case MFX_PROFILE_AVC_BASELINE:
            case MFX_PROFILE_AVC_CONSTRAINED_HIGH:
            case MFX_PROFILE_AVC_CONSTRAINED_BASELINE:
                goto unsupported; // B-frames not allowed by profile
            default:
                break;
        }
    }
    else if (pv->param.videoParam->mfx.CodecId == MFX_CODEC_HEVC)
    {
        switch (pv->param.videoParam->mfx.CodecProfile)
        {
            case MFX_PROFILE_HEVC_MAINSP:
                goto unsupported; // B-frames not allowed by profile
            default:
                break;
        }
    }

    /* Handle B-pyramid auto (on for CQP, off otherwise) */
    if (pv->param.gop.b_pyramid < 0)
    {
        pv->param.gop.b_pyramid = pv->param.videoParam->mfx.RateControlMethod == MFX_RATECONTROL_CQP;
    }

    if (pv->qsv_info->capabilities & HB_QSV_CAP_OPTION2_BREFTYPE)
    {
        /* B-pyramid can be controlled directly */
        if (pv->param.gop.b_pyramid)
        {
            pv->param.codingOption2.BRefType = MFX_B_REF_PYRAMID;
        }
        else
        {
            pv->param.codingOption2.BRefType = MFX_B_REF_OFF;
        }
    }
    else
    {
        /*
         * We can't control B-pyramid directly, do it indirectly by
         * adjusting GopRefDist, GopPicSize and NumRefFrame instead.
         */
        pv->param.codingOption2.BRefType = MFX_B_REF_UNKNOWN;

        /*
         * pyramid_ref_dist is the closest B-pyramid compatible
         * value (multiple of 2, >= 4) to the requested GopRefDist.
         */
        int pyramid_ref_dist = 4;
        while (pv->param.videoParam->mfx.GopRefDist > pyramid_ref_dist)
        {
            pyramid_ref_dist *= 2;
        }

        if (pv->param.gop.b_pyramid)
        {
            /* GopRefDist must be B-pyramid compatible */
            pv->param.videoParam->mfx.GopRefDist = pyramid_ref_dist;

            /*
             * GopPicSize must be a multiple of GopRefDist.
             *
             * Note: GopPicSize == 0 should always result in a value
             *       that doesn't cause Media SDK to disable B-pyramid.
             */
            if (pv->param.videoParam->mfx.GopPicSize)
            {
                pv->param.videoParam->mfx.GopPicSize = FFALIGN(pv->param.videoParam->mfx.GopPicSize,
                                                               pv->param.videoParam->mfx.GopRefDist);
            }

            /*
             * NumRefFrame must be greater than 3 and than half of GopRefDist.
             * Otherwise, Media SDK may sometimes decide to disable B-pyramid
             * (whereas sometimes it will simply sanitize NumRefFrame instead).
             *
             * Note: Media SDK handles the NumRefFrame == 0 case for us.
             */
            if (pv->param.videoParam->mfx.NumRefFrame)
            {
                pv->param.videoParam->mfx.NumRefFrame = FFMAX(pv->param.videoParam->mfx.NumRefFrame,
                                                              pv->param.videoParam->mfx.GopRefDist / 2);
                pv->param.videoParam->mfx.NumRefFrame = FFMAX(pv->param.videoParam->mfx.NumRefFrame, 3);
            }
        }
        else if (pv->param.videoParam->mfx.GopRefDist == 0 ||
                 pv->param.videoParam->mfx.GopRefDist == pyramid_ref_dist)
        {
            /*
             * GopRefDist is either B-pyramid compatible or unknown (and thus
             * potentially compatible), so adjust it to force-disable B-pyramid.
             */
            pv->param.videoParam->mfx.GopRefDist = pyramid_ref_dist - 1;
        }
    }

    return;

unsupported:
    pv->param.gop.b_pyramid          = 0;
    pv->param.codingOption2.BRefType = MFX_B_REF_OFF;
}

static int qsv_hevc_make_header(hb_work_object_t *w, mfxSession session, const mfxVideoParam *videoParam)
{
    size_t len;
    int ret = 0;
    uint8_t *buf, *end;
    mfxBitstream bitstream;
    mfxStatus status;
    mfxSyncPoint syncPoint;
    mfxFrameSurface1 frameSurface1;
    hb_work_private_t *pv = w->private_data;

    memset(&bitstream,     0, sizeof(mfxBitstream));
    memset(&syncPoint,     0, sizeof(mfxSyncPoint));
    memset(&frameSurface1, 0, sizeof(mfxFrameSurface1));

    if (videoParam == NULL)
    {
        hb_log("qsv_hevc_make_header: videoParam is NULL");
        ret = -1;
        goto end;
    }
    /* The bitstream buffer should be able to hold any encoded frame */
    size_t buf_max_size = videoParam->mfx.BufferSizeInKB * 1000 * ( 0 == videoParam->mfx.BRCParamMultiplier ? 1 : videoParam->mfx.BRCParamMultiplier);
    bitstream.Data      = av_mallocz(sizeof(uint8_t) * buf_max_size);
    bitstream.MaxLength = buf_max_size;
    if (bitstream.Data == NULL)
    {
        hb_log("qsv_hevc_make_header: bitstream.Data allocation failed");
        ret = -1;
        goto end;
    }

    /* We only need to encode one frame, so we only need one surface */
    int bpp12                = (pv->param.videoParam->mfx.FrameInfo.FourCC == MFX_FOURCC_P010) ? 6 : 3;
    mfxU16 Height            = pv->param.videoParam->mfx.FrameInfo.Height;
    mfxU16 Width             = pv->param.videoParam->mfx.FrameInfo.Width;
    frameSurface1.Info       = pv->param.videoParam->mfx.FrameInfo;
    frameSurface1.Data.Y     = av_mallocz(Width * Height * (bpp12 / 2.0));
    frameSurface1.Data.VU    = frameSurface1.Data.Y + Width * Height * (bpp12 == 6 ? 2 : 1);
    frameSurface1.Data.Pitch = Width * (bpp12 == 6 ? 2 : 1);

    /* Encode a single blank frame */
    do
    {
        status = MFXVideoENCODE_EncodeFrameAsync(session, NULL,
                                                 &frameSurface1,
                                                 &bitstream,
                                                 &syncPoint);

        if (status == MFX_ERR_MORE_DATA)
        {
            break; // more input needed, but we don't have any
        }
        if (status < MFX_ERR_NONE)
        {
            hb_log("qsv_hevc_make_header: MFXVideoENCODE_EncodeFrameAsync failed (%d)", status);
            status = log_encoder_params(pv, pv->param.videoParam);
            if (status < 0)
            {
                hb_error("qsv_hevc_make_header: log_encoder_params failed (%d)", status);
            }
            ret = -1;
            goto end;
        }
        if (syncPoint)
        {
            break; // we have output
        }
        if (status == MFX_WRN_DEVICE_BUSY)
        {
            hb_qsv_sleep(1);
        }
    }
    while (status >= MFX_ERR_NONE);

    /* If we don't have any output yet, flush the encoder */
    if (!syncPoint)
    {
        do
        {
            status = MFXVideoENCODE_EncodeFrameAsync(session, NULL, NULL,
                                                     &bitstream,
                                                     &syncPoint);

            if (status == MFX_ERR_MORE_DATA)
            {
                break; // done flushing
            }
            if (status < MFX_ERR_NONE)
            {
                hb_log("qsv_hevc_make_header: MFXVideoENCODE_EncodeFrameAsync failed (%d)", status);
                ret = -1;
                goto end;
            }
            if (syncPoint)
            {
                break; // we have output
            }
            if (status == MFX_WRN_DEVICE_BUSY)
            {
                hb_qsv_sleep(1);
            }
        }
        while (status >= MFX_ERR_NONE);
    }

    /* Still no data at this point, we can't proceed */
    if (!syncPoint)
    {
        hb_log("qsv_hevc_make_header: no sync point");
        ret = -1;
        goto end;
    }

    do
    {
        status = MFXVideoCORE_SyncOperation(session, syncPoint, 100);
    }
    while (status == MFX_WRN_IN_EXECUTION);

    if (status != MFX_ERR_NONE)
    {
        hb_log("qsv_hevc_make_header: MFXVideoCORE_SyncOperation failed (%d)", status);
        ret = -1;
        goto end;
    }

    if (!bitstream.DataLength)
    {
        hb_log("qsv_hevc_make_header: no bitstream data");
        ret = -1;
        goto end;
    }

    /* Include any parameter sets and SEI NAL units in the headers. */
    len = bitstream.DataLength;
    buf = bitstream.Data + bitstream.DataOffset;
    end = bitstream.Data + bitstream.DataOffset + bitstream.DataLength;
    w->config->h265.headers_length = 0;

    while ((buf = hb_annexb_find_next_nalu(buf, &len)) != NULL)
    {
        switch ((buf[0] >> 1) & 0x3f)
        {
            case 32: // VPS_NUT
            case 33: // SPS_NUT
            case 34: // PPS_NUT
            case 39: // PREFIX_SEI_NUT
            case 40: // SUFFIX_SEI_NUT
                break;
            default:
                len = end - buf;
                continue;
        }

        size_t size = hb_nal_unit_write_annexb(NULL, buf, len) + w->config->h265.headers_length;
        if (sizeof(w->config->h265.headers) < size)
        {
            /* Will never happen in practice */
            hb_log("qsv_hevc_make_header: header too large (size: %lu, max: %lu)",
                   size, sizeof(w->config->h265.headers));
        }

        w->config->h265.headers_length += hb_nal_unit_write_annexb(w->config->h265.headers +
                                                                   w->config->h265.headers_length, buf, len);
        len = end - buf;
    }

end:
    if (bitstream.Data)
        av_free(bitstream.Data);
    av_free(frameSurface1.Data.Y);
    return ret;
}

static void mids_buf_free(void *opaque, uint8_t *data)
{
    AVBufferRef *hw_frames_ref = opaque;
    av_buffer_unref(&hw_frames_ref);
    av_freep(&data);
}

static enum AVPixelFormat qsv_map_fourcc(uint32_t fourcc)
{
    switch (fourcc) {
    case MFX_FOURCC_NV12: return AV_PIX_FMT_NV12;
    case MFX_FOURCC_P010: return AV_PIX_FMT_P010;
    case MFX_FOURCC_P8:   return AV_PIX_FMT_PAL8;
    }
    return AV_PIX_FMT_NONE;
}

AVBufferRef *hb_qsv_create_mids(AVBufferRef *hw_frames_ref)
{
    AVHWFramesContext    *frames_ctx = (AVHWFramesContext*)hw_frames_ref->data;
    AVQSVFramesContext *frames_hwctx = frames_ctx->hwctx;
    int                  nb_surfaces = frames_hwctx->nb_surfaces;

    AVBufferRef *mids_buf, *hw_frames_ref1;
    QSVMid *mids;
    int i;

    hw_frames_ref1 = av_buffer_ref(hw_frames_ref);
    if (!hw_frames_ref1)
        return NULL;

    mids = av_calloc(nb_surfaces, sizeof(*mids));
    if (!mids) {
        av_buffer_unref(&hw_frames_ref1);
        return NULL;
    }

    mids_buf = av_buffer_create((uint8_t*)mids, nb_surfaces * sizeof(*mids),
                                mids_buf_free, hw_frames_ref1, 0);
    if (!mids_buf) {
        av_buffer_unref(&hw_frames_ref1);
        av_freep(&mids);
        return NULL;
    }

    for (i = 0; i < nb_surfaces; i++) {
        QSVMid *mid = &mids[i];
        mid->handle_pair   = (mfxHDLPair*)frames_hwctx->surfaces[i].Data.MemId;
        mid->hw_frames_ref = hw_frames_ref1;
    }

    return mids_buf;
}

static int qsv_setup_mids(mfxFrameAllocResponse *resp, AVBufferRef *hw_frames_ref,
                          AVBufferRef *mids_buf)
{
    AVHWFramesContext    *frames_ctx = (AVHWFramesContext*)hw_frames_ref->data;
    AVQSVFramesContext *frames_hwctx = frames_ctx->hwctx;
    QSVMid                     *mids = (QSVMid*)mids_buf->data;
    int                  nb_surfaces = frames_hwctx->nb_surfaces;
    int i;

    // the allocated size of the array is two larger than the number of
    // surfaces, we store the references to the frames context and the
    // QSVMid array there
    resp->mids = av_calloc(nb_surfaces + 2, sizeof(*resp->mids));
    if (!resp->mids)
        return AVERROR(ENOMEM);

    for (i = 0; i < nb_surfaces; i++)
        resp->mids[i] = &mids[i];
    resp->NumFrameActual = nb_surfaces;

    resp->mids[resp->NumFrameActual] = (mfxMemId)av_buffer_ref(hw_frames_ref);
    if (!resp->mids[resp->NumFrameActual]) {
        av_freep(&resp->mids);
        return AVERROR(ENOMEM);
    }

    resp->mids[resp->NumFrameActual + 1] = av_buffer_ref(mids_buf);
    if (!resp->mids[resp->NumFrameActual + 1]) {
        av_buffer_unref((AVBufferRef**)&resp->mids[resp->NumFrameActual]);
        av_freep(&resp->mids);
        return AVERROR(ENOMEM);
    }

    return 0;
}

static mfxStatus hb_qsv_frame_alloc(mfxHDL pthis, mfxFrameAllocRequest *req,
                                 mfxFrameAllocResponse *resp)
{
    HBQSVFramesContext *ctx = pthis;
    int ret;

    /* this should only be called from an encoder or decoder and
     * only allocates video memory frames */
    if (!(req->Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET |
                       MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET))         ||
        !(req->Type & (MFX_MEMTYPE_FROM_DECODE | MFX_MEMTYPE_FROM_ENCODE)))
        return MFX_ERR_UNSUPPORTED;

    if (req->Type & MFX_MEMTYPE_EXTERNAL_FRAME) {
        /* external frames -- fill from the caller-supplied frames context */
        AVHWFramesContext *frames_ctx = (AVHWFramesContext*)ctx->hw_frames_ctx->data;
        AVQSVFramesContext *frames_hwctx = frames_ctx->hwctx;
        mfxFrameInfo      *i  = &req->Info;
        mfxFrameInfo      *i1 = &frames_hwctx->surfaces[0].Info;

        if (i->Width  > i1->Width  || i->Height > i1->Height ||
            i->FourCC != i1->FourCC || i->ChromaFormat != i1->ChromaFormat) {
            hb_error("Mismatching surface properties in an "
                   "allocation request: %dx%d %d %d vs %dx%d %d %d\n",
                   i->Width,  i->Height,  i->FourCC,  i->ChromaFormat,
                   i1->Width, i1->Height, i1->FourCC, i1->ChromaFormat);
            return MFX_ERR_UNSUPPORTED;
        }

        ret = qsv_setup_mids(resp, ctx->hw_frames_ctx, ctx->mids_buf);
        if (ret < 0) {
            hb_error("Error filling an external frame allocation request\n");
            return MFX_ERR_MEMORY_ALLOC;
        }
    } else if (req->Type & MFX_MEMTYPE_INTERNAL_FRAME) {
        /* internal frames -- allocate a new hw frames context */
        AVHWFramesContext *ext_frames_ctx = (AVHWFramesContext*)ctx->hw_frames_ctx->data;
        mfxFrameInfo      *i  = &req->Info;

        AVBufferRef *frames_ref, *mids_buf;
        AVHWFramesContext *frames_ctx;
        AVQSVFramesContext *frames_hwctx;

        frames_ref = av_hwframe_ctx_alloc(ext_frames_ctx->device_ref);
        if (!frames_ref)
            return MFX_ERR_MEMORY_ALLOC;

        frames_ctx   = (AVHWFramesContext*)frames_ref->data;
        frames_hwctx = frames_ctx->hwctx;

        frames_ctx->format            = AV_PIX_FMT_QSV;
        frames_ctx->sw_format         = qsv_map_fourcc(i->FourCC);
        frames_ctx->width             = i->Width;
        frames_ctx->height            = i->Height;
        frames_ctx->initial_pool_size = req->NumFrameSuggested;

        frames_hwctx->frame_type      = req->Type;

        ret = av_hwframe_ctx_init(frames_ref);
        if (ret < 0) {
            hb_error("Error initializing a frames context for an internal frame "
                   "allocation request\n");
            av_buffer_unref(&frames_ref);
            return MFX_ERR_MEMORY_ALLOC;
        }

        mids_buf = hb_qsv_create_mids(frames_ref);
        if (!mids_buf) {
            av_buffer_unref(&frames_ref);
            return MFX_ERR_MEMORY_ALLOC;
        }

        ret = qsv_setup_mids(resp, frames_ref, mids_buf);
        av_buffer_unref(&mids_buf);
        av_buffer_unref(&frames_ref);
        if (ret < 0) {
            hb_error("Error filling an internal frame allocation request\n");
            return MFX_ERR_MEMORY_ALLOC;
        }
    } else {
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

static mfxStatus hb_qsv_frame_free(mfxHDL pthis, mfxFrameAllocResponse *resp)
{
    av_buffer_unref((AVBufferRef**)&resp->mids[resp->NumFrameActual]);
    av_buffer_unref((AVBufferRef**)&resp->mids[resp->NumFrameActual + 1]);
    av_freep(&resp->mids);
    return MFX_ERR_NONE;
}

static mfxStatus hb_qsv_frame_lock(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr)
{
    QSVMid *qsv_mid = mid;
    AVHWFramesContext *hw_frames_ctx = (AVHWFramesContext*)qsv_mid->hw_frames_ref->data;
    AVQSVFramesContext *hw_frames_hwctx = hw_frames_ctx->hwctx;
    int ret;

    if (qsv_mid->locked_frame)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    /* Allocate a system memory frame that will hold the mapped data. */
    qsv_mid->locked_frame = av_frame_alloc();
    if (!qsv_mid->locked_frame)
        return MFX_ERR_MEMORY_ALLOC;
    qsv_mid->locked_frame->format  = hw_frames_ctx->sw_format;

    /* wrap the provided handle in a hwaccel AVFrame */
    qsv_mid->hw_frame = av_frame_alloc();
    if (!qsv_mid->hw_frame)
        goto fail;

    qsv_mid->hw_frame->data[3] = (uint8_t*)&qsv_mid->surf;
    qsv_mid->hw_frame->format  = AV_PIX_FMT_QSV;

    // doesn't really matter what buffer is used here
    qsv_mid->hw_frame->buf[0]  = av_buffer_alloc(1);
    if (!qsv_mid->hw_frame->buf[0])
        goto fail;

    qsv_mid->hw_frame->width   = hw_frames_ctx->width;
    qsv_mid->hw_frame->height  = hw_frames_ctx->height;

    qsv_mid->hw_frame->hw_frames_ctx = av_buffer_ref(qsv_mid->hw_frames_ref);
    if (!qsv_mid->hw_frame->hw_frames_ctx)
        goto fail;

    qsv_mid->surf.Info = hw_frames_hwctx->surfaces[0].Info;
    qsv_mid->surf.Data.MemId = qsv_mid->handle_pair;

    /* map the data to the system memory */
    ret = av_hwframe_map(qsv_mid->locked_frame, qsv_mid->hw_frame,
                         AV_HWFRAME_MAP_DIRECT);
    if (ret < 0)
        goto fail;

    ptr->Pitch = qsv_mid->locked_frame->linesize[0];
    ptr->Y     = qsv_mid->locked_frame->data[0];
    ptr->U     = qsv_mid->locked_frame->data[1];
    ptr->V     = qsv_mid->locked_frame->data[1] + 1;

    return MFX_ERR_NONE;
fail:
    av_frame_free(&qsv_mid->hw_frame);
    av_frame_free(&qsv_mid->locked_frame);
    return MFX_ERR_MEMORY_ALLOC;
}

static mfxStatus hb_qsv_frame_unlock(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr)
{
    QSVMid *qsv_mid = mid;

    av_frame_free(&qsv_mid->locked_frame);
    av_frame_free(&qsv_mid->hw_frame);

    return MFX_ERR_NONE;
}

static mfxStatus hb_qsv_frame_get_hdl(mfxHDL pthis, mfxMemId mid, mfxHDL *hdl)
{
    QSVMid *qsv_mid = (QSVMid*)mid;
    mfxHDLPair *pair_dst = (mfxHDLPair*)hdl;
    mfxHDLPair *pair_src = (mfxHDLPair*)qsv_mid->handle_pair;

    pair_dst->first = pair_src->first;
    if (pair_src->second != (mfxMemId)MFX_INFINITE)
        pair_dst->second = pair_src->second;

    return MFX_ERR_NONE;
}

#define QSV_RUNTIME_VERSION_ATLEAST(MFX_VERSION, MAJOR, MINOR) \
    (MFX_VERSION.Major > (MAJOR)) ||                           \
    (MFX_VERSION.Major == (MAJOR) && MFX_VERSION.Minor >= (MINOR))

int qsv_enc_init(hb_work_private_t *pv)
{
    hb_qsv_context *qsv = pv->job->qsv.ctx;
    hb_job_t       *job = pv->job;
    mfxVersion version;
    mfxStatus sts;
    mfxIMPL impl;
    int i;

    if (pv->init_done)
    {
        return 0;
    }

    if (qsv == NULL)
    {
        hb_error("qsv_enc_init: no context!");
        return 3;
    }

    hb_qsv_space *qsv_encode = qsv->enc_space;
    if (qsv_encode == NULL)
    {
        // if only for encode
        if (pv->is_sys_mem)
        {
            // re-use the session from encqsvInit
            qsv->mfx_session = pv->mfx_session;
        }
        else
        {
            mfxStatus err;

            mfxVersion    ver;
            mfxIMPL       impl;

            AVHWDeviceContext    *device_ctx = (AVHWDeviceContext*)qsv->hb_hw_device_ctx->data;
            AVQSVDeviceContext *device_hwctx = device_ctx->hwctx;
            mfxSession        parent_session = device_hwctx->session;

            err = MFXQueryIMPL(parent_session, &impl);
            if (err != MFX_ERR_NONE)
            {
                hb_error("Error querying the session attributes");
                return -1;
            }

            err = MFXQueryVersion(parent_session, &ver);
            if (err != MFX_ERR_NONE)
            {
                hb_error("Error querying the session attributes");
                return -1;
            }

            // reuse parent session
            qsv->mfx_session = parent_session;
            mfxFrameAllocator frame_allocator = {
                .pthis  = pv->job->qsv.ctx->hb_dec_qsv_frames_ctx,
                .Alloc  = hb_qsv_frame_alloc,
                .Lock   = hb_qsv_frame_lock,
                .Unlock = hb_qsv_frame_unlock,
                .GetHDL = hb_qsv_frame_get_hdl,
                .Free   = hb_qsv_frame_free,
            };

            if (hb_qsv_hw_filters_are_enabled(pv->job))
            {
                frame_allocator.pthis = pv->job->qsv.ctx->hb_vpp_qsv_frames_ctx;
            }

            err = MFXVideoCORE_SetFrameAllocator(qsv->mfx_session, &frame_allocator);
            if (err != MFX_ERR_NONE)
            {
                hb_log("encqsvInit: MFXVideoCORE_SetFrameAllocator error %d", err);
                return -1;
            }
        }
        qsv->enc_space = qsv_encode = &pv->enc_space;
    }

    if (!pv->is_sys_mem)
    {
        hb_qsv_space *dec_space = qsv->dec_space;
        if (dec_space == NULL || !dec_space->is_init_done)
        {
            return 2;
        }
    }

    // allocate tasks
    qsv_encode->p_buf_max_size = pv->param.videoParam->mfx.BufferSizeInKB * 1000 * ( 0 == pv->param.videoParam->mfx.BRCParamMultiplier ? 1 : pv->param.videoParam->mfx.BRCParamMultiplier);
    qsv_encode->tasks          = hb_qsv_list_init(HAVE_THREADS);
    for (i = 0; i < pv->max_async_depth; i++)
    {
        hb_qsv_task *task    = av_mallocz(sizeof(hb_qsv_task));
        task->bs             = av_mallocz(sizeof(mfxBitstream));
        task->bs->Data       = av_mallocz(sizeof(uint8_t) * qsv_encode->p_buf_max_size);
        task->bs->MaxLength  = qsv_encode->p_buf_max_size;
        task->bs->DataLength = 0;
        task->bs->DataOffset = 0;
        hb_qsv_list_add(qsv_encode->tasks, task);
    }

    // setup surface allocation
    pv->param.videoParam->IOPattern = (pv->is_sys_mem                 ?
                                       MFX_IOPATTERN_IN_SYSTEM_MEMORY :
                                       MFX_IOPATTERN_IN_VIDEO_MEMORY);
    memset(&qsv_encode->request, 0, sizeof(mfxFrameAllocRequest) * 2);
    sts = MFXVideoENCODE_QueryIOSurf(qsv->mfx_session,
                                     pv->param.videoParam,
                                     &qsv_encode->request[0]);
    if (sts < MFX_ERR_NONE) // ignore warnings
    {
        hb_error("qsv_enc_init: MFXVideoENCODE_QueryIOSurf failed (%d)", sts);
        *job->done_error = HB_ERROR_INIT;
        *job->die = 1;
        return -1;
    }

    // allocate surfaces
    if (pv->is_sys_mem)
    {
        qsv_encode->surface_num = FFMIN(qsv_encode->request[0].NumFrameSuggested +
                                        pv->max_async_depth, HB_QSV_SURFACE_NUM);
        if (qsv_encode->surface_num <= 0)
        {
            qsv_encode->surface_num = HB_QSV_SURFACE_NUM;
        }

        /* should have 15bpp/AV_PIX_FMT_YUV420P10LE (almost x2) instead of 12bpp/AV_PIX_FMT_NV12 */
        int bpp12 = (pv->param.videoParam->mfx.CodecProfile == (MFX_PROFILE_HEVC_MAIN10) || (job->vcodec == HB_VCODEC_QSV_AV1_10BIT)) ? 6 : 3;
        for (i = 0; i < qsv_encode->surface_num; i++)
        {
            mfxFrameSurface1 *surface = av_mallocz(sizeof(mfxFrameSurface1));
            mfxFrameInfo info         = pv->param.videoParam->mfx.FrameInfo;
            surface->Info             = info;
            surface->Data.Pitch       = info.Width * (bpp12 == 6 ? 2 : 1);
            surface->Data.Y           = av_mallocz(info.Width * info.Height * (bpp12 / 2.0));
            surface->Data.VU          = surface->Data.Y + info.Width * info.Height * (bpp12 == 6 ? 2 : 1);
            qsv_encode->p_surfaces[i] = surface;
        }
    }
    else
    {
        qsv_encode->surface_num = FFMIN(qsv_encode->request[0].NumFrameSuggested +
                                        pv->max_async_depth, HB_QSV_SURFACE_NUM);
        if (qsv_encode->surface_num <= 0)
        {
            qsv_encode->surface_num = HB_QSV_SURFACE_NUM;
        }
    }

    // allocate sync points
    qsv_encode->sync_num = (qsv_encode->surface_num                         ?
                            FFMIN(qsv_encode->surface_num, HB_QSV_SYNC_NUM) :
                            HB_QSV_SYNC_NUM);

    for (i = 0; i < qsv_encode->sync_num; i++)
    {
        qsv_encode->p_syncp[i] = av_mallocz(sizeof(hb_qsv_sync));
        HB_QSV_CHECK_POINTER(qsv_encode->p_syncp[i], MFX_ERR_MEMORY_ALLOC);
        qsv_encode->p_syncp[i]->p_sync = av_mallocz(sizeof(mfxSyncPoint));
        HB_QSV_CHECK_POINTER(qsv_encode->p_syncp[i]->p_sync, MFX_ERR_MEMORY_ALLOC);
    }

    // initialize the encoder
    sts = MFXVideoENCODE_Init(qsv->mfx_session, pv->param.videoParam);
    if (sts < MFX_ERR_NONE) // ignore warnings
    {
        hb_error("qsv_enc_init: MFXVideoENCODE_Init failed (%d)", sts);
        *job->done_error = HB_ERROR_INIT;
        *job->die = 1;
        return -1;
    }

    // query and log actual implementation details
    if ((MFXQueryIMPL   (qsv->mfx_session, &impl)    == MFX_ERR_NONE) &&
        (MFXQueryVersion(qsv->mfx_session, &version) == MFX_ERR_NONE))
    {
        hb_log("qsv_enc_init: using '%s %s' implementation, API: %"PRIu16".%"PRIu16"",
               hb_qsv_impl_get_name(impl), hb_qsv_impl_get_via_name(impl), version.Major, version.Minor);
    }
    else
    {
        hb_log("qsv_enc_init: MFXQueryIMPL/MFXQueryVersion failure");
    }

    qsv_encode->is_init_done = 1;
    pv->init_done = 1;
    return 0;
}

/***********************************************************************
 * encqsvInit
 ***********************************************************************
 *
 **********************************************************************/
int encqsvInit(hb_work_object_t *w, hb_job_t *job)
{
    if (!hb_qsv_available())
    {
       hb_error("encqsvInit: qsv is not available on the system");
       return -1;
    }

    int brc_param_multiplier;
    hb_work_private_t *pv = calloc(1, sizeof(hb_work_private_t));
    w->private_data       = pv;

    pv->is_sys_mem         = hb_qsv_full_path_is_enabled(job) ? 0 : 1; // TODO: re-implement QSV VPP filtering support
    pv->job                = job;
    pv->qsv_info           = hb_qsv_encoder_info_get(hb_qsv_get_adapter_index(), job->vcodec);
    pv->delayed_processing = hb_list_init();
    pv->last_start         = INT64_MIN;
    hb_buffer_list_clear(&pv->encoded_frames);

    pv->chapter_queue    = hb_chapter_queue_init();

    if (!pv->qsv_info)
    {
        hb_error("encqsvInit: %s codec is not supported by this GPU adapter", hb_video_encoder_get_long_name(job->vcodec));
        return -1;
    }

    // default encoding parameters
    if (hb_qsv_param_default_preset(&pv->param, &pv->enc_space.m_mfxVideoParam,
                                     pv->qsv_info, job->encoder_preset))
    {
        hb_error("encqsvInit: hb_qsv_param_default_preset failed");
        return -1;
    }

    // set AsyncDepth to match that of decode and VPP
    pv->param.videoParam->AsyncDepth = job->qsv.async_depth;

    // set and enable colorimetry (video signal information)
    pv->param.videoSignalInfo.ColourPrimaries          = hb_output_color_prim(job);
    pv->param.videoSignalInfo.TransferCharacteristics  = hb_output_color_transfer(job);
    pv->param.videoSignalInfo.MatrixCoefficients       = hb_output_color_matrix(job);
    pv->param.videoSignalInfo.ColourDescriptionPresent = 1;

    if (job->chroma_location != AVCHROMA_LOC_UNSPECIFIED)
    {
        pv->param.chromaLocInfo.ChromaSampleLocTypeBottomField =
        pv->param.chromaLocInfo.ChromaSampleLocTypeTopField = job->chroma_location - 1;
        pv->param.chromaLocInfo.ChromaLocInfoPresentFlag = 1;
    }

    /* HDR10 Static metadata */
    if (job->color_transfer == HB_COLR_TRA_SMPTEST2084)
    {
        const int masteringChromaDen = 50000;
        const int masteringLumaDen = 10000;

        /* Mastering display metadata */
        if (job->mastering.has_primaries && job->mastering.has_luminance)
        {
            pv->param.masteringDisplayColourVolume.InsertPayloadToggle = MFX_PAYLOAD_IDR;
            pv->param.masteringDisplayColourVolume.DisplayPrimariesX[0] = rescale(job->mastering.display_primaries[0][0], masteringChromaDen);
            pv->param.masteringDisplayColourVolume.DisplayPrimariesY[0] = rescale(job->mastering.display_primaries[0][1], masteringChromaDen);
            pv->param.masteringDisplayColourVolume.DisplayPrimariesX[1] = rescale(job->mastering.display_primaries[1][0], masteringChromaDen);
            pv->param.masteringDisplayColourVolume.DisplayPrimariesY[1] = rescale(job->mastering.display_primaries[1][1], masteringChromaDen);
            pv->param.masteringDisplayColourVolume.DisplayPrimariesX[2] = rescale(job->mastering.display_primaries[2][0], masteringChromaDen);
            pv->param.masteringDisplayColourVolume.DisplayPrimariesY[2] = rescale(job->mastering.display_primaries[2][1], masteringChromaDen);
            pv->param.masteringDisplayColourVolume.WhitePointX = rescale(job->mastering.white_point[0], masteringChromaDen);
            pv->param.masteringDisplayColourVolume.WhitePointY = rescale(job->mastering.white_point[1], masteringChromaDen);
            pv->param.masteringDisplayColourVolume.MaxDisplayMasteringLuminance = rescale(job->mastering.max_luminance, masteringLumaDen);
            pv->param.masteringDisplayColourVolume.MinDisplayMasteringLuminance = rescale(job->mastering.min_luminance, masteringLumaDen);
        }

        /*  Content light level */
        if (job->coll.max_cll && job->coll.max_fall)
        {
            pv->param.contentLightLevelInfo.InsertPayloadToggle = MFX_PAYLOAD_IDR;
            pv->param.contentLightLevelInfo.MaxContentLightLevel  = job->coll.max_cll;
            pv->param.contentLightLevelInfo.MaxPicAverageLightLevel = job->coll.max_fall;
        }
    }

    // parse user-specified encoder options, if present
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
            hb_value_t *value = hb_dict_iter_value(iter);
            char *str = hb_value_get_string_xform(value);

            switch (hb_qsv_param_parse(&pv->param, pv->qsv_info, pv->job, key, str))
            {
                case HB_QSV_PARAM_OK:
                    break;

                case HB_QSV_PARAM_BAD_NAME:
                    hb_log("encqsvInit: hb_qsv_param_parse: bad key %s", key);
                    break;
                case HB_QSV_PARAM_BAD_VALUE:
                    hb_log("encqsvInit: hb_qsv_param_parse: bad value %s for key %s",
                           str, key);
                    break;
                case HB_QSV_PARAM_UNSUPPORTED:
                    hb_log("encqsvInit: hb_qsv_param_parse: unsupported option %s",
                           key);
                    break;

                case HB_QSV_PARAM_ERROR:
                default:
                    hb_log("encqsvInit: hb_qsv_param_parse: unknown error");
                    break;
            }
            free(str);
        }
        hb_dict_free(&options_list);
    }
#if defined(_WIN32) || defined(__MINGW32__)
    if (pv->is_sys_mem && hb_qsv_implementation_is_hardware(pv->qsv_info->implementation))
    {
        // select the right hardware implementation based on dx index
        if (!job->qsv.ctx->qsv_device)
            hb_qsv_param_parse_dx_index(pv->job, hb_qsv_get_adapter_index());
        mfxIMPL hw_preference = MFX_IMPL_VIA_D3D11;
        pv->qsv_info->implementation = hb_qsv_dx_index_to_impl(job->qsv.ctx->dx_index) | hw_preference;
    }
#endif
    // reload colorimetry in case values were set in encoder_options
    if (pv->param.videoSignalInfo.ColourDescriptionPresent)
    {
        job->color_prim_override     = pv->param.videoSignalInfo.ColourPrimaries;
        job->color_transfer_override = pv->param.videoSignalInfo.TransferCharacteristics;
        job->color_matrix_override   = pv->param.videoSignalInfo.MatrixCoefficients;
    }

    // sanitize values that may exceed the Media SDK variable size
    hb_rational_t par;
    hb_limit_rational(&par.num, &par.den,
                      job->par.num, job->par.den, UINT16_MAX);

    // some encoding parameters are used by filters to configure their output
    switch (pv->qsv_info->codec_id)
    {
        case MFX_CODEC_HEVC:
        case MFX_CODEC_AV1:
            job->qsv.enc_info.align_width  = HB_QSV_ALIGN32(job->width);
            job->qsv.enc_info.align_height = HB_QSV_ALIGN32(job->height);
            break;

        case MFX_CODEC_AVC:
        default:
            job->qsv.enc_info.align_width  = HB_QSV_ALIGN16(job->width);
            job->qsv.enc_info.align_height = HB_QSV_ALIGN16(job->height);
            break;
    }
    if (pv->param.videoParam->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE)
    {
        // additional alignment may be required
        switch (pv->qsv_info->codec_id)
        {
            case MFX_CODEC_AVC:
                job->qsv.enc_info.align_height = HB_QSV_ALIGN32(job->qsv.enc_info.align_height);
                break;

            default:
                break;
        }
    }
    job->qsv.enc_info.pic_struct   = pv->param.videoParam->mfx.FrameInfo.PicStruct;
    job->qsv.enc_info.is_init_done = 1;

    // set codec, profile/level and FrameInfo
    pv->param.videoParam->mfx.CodecId                 = pv->qsv_info->codec_id;
    pv->param.videoParam->mfx.CodecLevel              = MFX_LEVEL_UNKNOWN;
    pv->param.videoParam->mfx.CodecProfile            = MFX_PROFILE_UNKNOWN;
    pv->param.videoParam->mfx.FrameInfo.FourCC        = MFX_FOURCC_NV12;
    pv->param.videoParam->mfx.FrameInfo.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
    pv->param.videoParam->mfx.FrameInfo.FrameRateExtN = job->vrate.num;
    pv->param.videoParam->mfx.FrameInfo.FrameRateExtD = job->vrate.den;
    pv->param.videoParam->mfx.FrameInfo.AspectRatioW  = par.num;
    pv->param.videoParam->mfx.FrameInfo.AspectRatioH  = par.den;
    pv->param.videoParam->mfx.FrameInfo.CropX         = 0;
    pv->param.videoParam->mfx.FrameInfo.CropY         = 0;
    pv->param.videoParam->mfx.FrameInfo.CropW         = job->width;
    pv->param.videoParam->mfx.FrameInfo.CropH         = job->height;
    pv->param.videoParam->mfx.FrameInfo.PicStruct     = job->qsv.enc_info.pic_struct;
    pv->param.videoParam->mfx.FrameInfo.Width         = job->qsv.enc_info.align_width;
    pv->param.videoParam->mfx.FrameInfo.Height        = job->qsv.enc_info.align_height;

    // parse user-specified codec profile and level
    if (hb_qsv_profile_parse(&pv->param, pv->qsv_info, job->encoder_profile, job->vcodec))
    {
        hb_error("encqsvInit: bad profile %s", job->encoder_profile);
        return -1;
    }

    if (hb_qsv_level_parse(&pv->param, pv->qsv_info, job->encoder_level))
    {
        hb_error("encqsvInit: bad level %s", job->encoder_level);
        return -1;
    }

    if ((pv->param.videoParam->mfx.CodecProfile == MFX_PROFILE_HEVC_MAIN10) || (job->vcodec == HB_VCODEC_QSV_AV1_10BIT))
    {
        pv->param.videoParam->mfx.FrameInfo.FourCC         = MFX_FOURCC_P010;
        pv->param.videoParam->mfx.FrameInfo.BitDepthLuma   = 10;
        pv->param.videoParam->mfx.FrameInfo.BitDepthChroma = 10;
        pv->param.videoParam->mfx.FrameInfo.Shift          = 1;
    }

    // interlaced encoding is not always possible
    if (pv->param.videoParam->mfx.CodecId             == MFX_CODEC_AVC &&
        pv->param.videoParam->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE)
    {
        if (pv->param.videoParam->mfx.CodecProfile == MFX_PROFILE_AVC_CONSTRAINED_BASELINE ||
            pv->param.videoParam->mfx.CodecProfile == MFX_PROFILE_AVC_BASELINE             ||
            pv->param.videoParam->mfx.CodecProfile == MFX_PROFILE_AVC_PROGRESSIVE_HIGH)
        {
            hb_error("encqsvInit: profile %s doesn't support interlaced encoding",
                     hb_qsv_profile_name(MFX_CODEC_AVC,
                                         pv->param.videoParam->mfx.CodecProfile));
            return -1;
        }
        if ((pv->param.videoParam->mfx.CodecLevel >= MFX_LEVEL_AVC_1b &&
             pv->param.videoParam->mfx.CodecLevel <= MFX_LEVEL_AVC_2) ||
            (pv->param.videoParam->mfx.CodecLevel >= MFX_LEVEL_AVC_42))
        {
            hb_error("encqsvInit: level %s doesn't support interlaced encoding",
                     hb_qsv_level_name(MFX_CODEC_AVC,
                                       pv->param.videoParam->mfx.CodecLevel));
            return -1;
        }
    }

    int hw_generation = hb_qsv_hardware_generation(hb_qsv_get_platform(hb_qsv_get_adapter_index()));
    // sanitize ICQ
    // workaround for MediaSDK platforms below TGL to disable ICQ if incorrectly detected
    if (!(pv->qsv_info->capabilities & HB_QSV_CAP_RATECONTROL_ICQ) ||
        ((pv->param.videoParam->mfx.LowPower == MFX_CODINGOPTION_ON) && (hw_generation < QSV_G8)))
    {
        // ICQ not supported
        pv->param.rc.icq = 0;
    }
    else
    {
        pv->param.rc.icq = pv->param.rc.icq && job->vquality > HB_INVALID_VIDEO_QUALITY;
    }

    // sanitize lookahead
    if (!(pv->qsv_info->capabilities & HB_QSV_CAP_RATECONTROL_LA))
    {
        // lookahead not supported
        pv->param.rc.lookahead = 0;
    }
    else if ((pv->param.rc.lookahead)                                       &&
             (pv->qsv_info->capabilities & HB_QSV_CAP_RATECONTROL_LAi) == 0 &&
             (pv->param.videoParam->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE))
    {
        // lookahead enabled but we can't use it
        hb_log("encqsvInit: LookAhead not used (LookAhead is progressive-only)");
        pv->param.rc.lookahead = 0;
    }
    else
    {
        pv->param.rc.lookahead = pv->param.rc.lookahead && (pv->param.rc.icq || job->vquality <= HB_INVALID_VIDEO_QUALITY);
    }

    if (pv->job->qsv.ctx != NULL)
    {
        job->qsv.ctx->la_is_enabled = pv->param.rc.lookahead ? 1 : 0;
    }

    // libmfx BRC parameters are 16 bits thus maybe overflow, then BRCParamMultiplier is needed
    // Comparison vbitrate in Kbps (kilobit) with vbv_max_bitrate, vbv_buffer_size, vbv_buffer_init in KB (kilobyte)
    brc_param_multiplier = (FFMAX(FFMAX3(job->vbitrate, pv->param.rc.vbv_max_bitrate, pv->param.rc.vbv_buffer_size),
                            pv->param.rc.vbv_buffer_init) + 0x10000) / 0x10000;
    // set VBV here (this will be overridden for CQP and ignored for LA)
    // only set BufferSizeInKB, InitialDelayInKB and MaxKbps if we have
    // them - otherwise Media SDK will pick values for us automatically
    if (pv->param.rc.vbv_buffer_size > 0)
    {
        if (pv->param.rc.vbv_buffer_init > 1.0)
        {
            pv->param.videoParam->mfx.InitialDelayInKB = (pv->param.rc.vbv_buffer_init / 8) / brc_param_multiplier;
        }
        else if (pv->param.rc.vbv_buffer_init > 0.0)
        {
            pv->param.videoParam->mfx.InitialDelayInKB = (pv->param.rc.vbv_buffer_size *
                                                          pv->param.rc.vbv_buffer_init / 8) / brc_param_multiplier;
        }
        pv->param.videoParam->mfx.BufferSizeInKB       = (pv->param.rc.vbv_buffer_size / 8) / brc_param_multiplier;
        pv->param.videoParam->mfx.BRCParamMultiplier   = brc_param_multiplier;
    }
    if (pv->param.rc.vbv_max_bitrate > 0)
    {
        pv->param.videoParam->mfx.MaxKbps              = pv->param.rc.vbv_max_bitrate / brc_param_multiplier;
        pv->param.videoParam->mfx.BRCParamMultiplier   = brc_param_multiplier;
    }

    // set rate control parameters
    if (job->vquality > HB_INVALID_VIDEO_QUALITY)
    {
        unsigned int upper_limit = 51;

        if (pv->param.rc.icq)
        {
            // introduced in API 1.8
            if (pv->param.rc.lookahead)
            {
                pv->param.videoParam->mfx.RateControlMethod = MFX_RATECONTROL_LA_ICQ;
            }
            else
            {
                pv->param.videoParam->mfx.RateControlMethod = MFX_RATECONTROL_ICQ;
            }
            pv->param.videoParam->mfx.ICQQuality = HB_QSV_CLIP3(1, upper_limit, job->vquality);
        }
        else
        {
            // introduced in API 1.1
            // HEVC 10b has QP range as [-12;51]
            // with shift +12 needed to be in QSV's U16 range
            if (pv->param.videoParam->mfx.CodecProfile == MFX_PROFILE_HEVC_MAIN10)
            {
                upper_limit = 63;
            }
            if (pv->param.videoParam->mfx.CodecId == MFX_CODEC_AV1)
            {
                upper_limit = 255;
            }

            pv->param.videoParam->mfx.RateControlMethod = MFX_RATECONTROL_CQP;
            pv->param.videoParam->mfx.QPI = HB_QSV_CLIP3(0, upper_limit, job->vquality + pv->param.rc.cqp_offsets[0]);
            pv->param.videoParam->mfx.QPP = HB_QSV_CLIP3(0, upper_limit, job->vquality + pv->param.rc.cqp_offsets[1]);
            pv->param.videoParam->mfx.QPB = HB_QSV_CLIP3(0, upper_limit, job->vquality + pv->param.rc.cqp_offsets[2]);

            // CQP + ExtBRC can cause bad output
            pv->param.codingOption2.ExtBRC = MFX_CODINGOPTION_OFF;
        }
    }
    else if (job->vbitrate > 0)
    {
        if (pv->param.rc.lookahead)
        {
            // introduced in API 1.7
            pv->param.videoParam->mfx.RateControlMethod  = MFX_RATECONTROL_LA;
            pv->param.videoParam->mfx.TargetKbps         = job->vbitrate / brc_param_multiplier;
            pv->param.videoParam->mfx.BRCParamMultiplier = brc_param_multiplier;
            // ignored, but some drivers will change AsyncDepth because of it
            pv->param.codingOption2.ExtBRC = MFX_CODINGOPTION_OFF;
        }
        else
        {
            // introduced in API 1.0
            if (job->vbitrate == pv->param.rc.vbv_max_bitrate)
            {
                pv->param.videoParam->mfx.RateControlMethod = MFX_RATECONTROL_CBR;
            }
            else
            {
                pv->param.videoParam->mfx.RateControlMethod = MFX_RATECONTROL_VBR;
            }
            pv->param.videoParam->mfx.TargetKbps            = job->vbitrate / brc_param_multiplier;
            pv->param.videoParam->mfx.BRCParamMultiplier    = brc_param_multiplier;
        }
    }
    else
    {
        hb_error("encqsvInit: invalid rate control (%f, %d)",
                 job->vquality, job->vbitrate);
        return -1;
    }

    // if VBV is enabled but ignored, log it
    if (pv->param.rc.vbv_max_bitrate > 0 || pv->param.rc.vbv_buffer_size > 0)
    {
        switch (pv->param.videoParam->mfx.RateControlMethod)
        {
            case MFX_RATECONTROL_LA:
            case MFX_RATECONTROL_LA_ICQ:
                hb_log("encqsvInit: LookAhead enabled, ignoring VBV");
                break;
            case MFX_RATECONTROL_ICQ:
                hb_log("encqsvInit: ICQ rate control, ignoring VBV");
                break;
            default:
                break;
        }
    }

    // set the GOP structure
    if (pv->param.gop.gop_ref_dist < 0)
    {
        if ((hw_generation >= QSV_G8) &&
            (pv->param.videoParam->mfx.CodecId == MFX_CODEC_HEVC ||
            pv->param.videoParam->mfx.CodecId == MFX_CODEC_AV1))
        {
            pv->param.gop.gop_ref_dist = 8;
        }
        else
        {
            pv->param.gop.gop_ref_dist = 4;
        }
    }
    pv->param.videoParam->mfx.GopRefDist = pv->param.gop.gop_ref_dist;

    // set the keyframe interval
    if (pv->param.gop.gop_pic_size < 0)
    {
        double rate = (double)job->orig_vrate.num / job->orig_vrate.den + 0.5;
        // set the keyframe interval based on the framerate
        pv->param.gop.gop_pic_size = (int)(FFMIN(rate * 2, 120));
    }
    pv->param.videoParam->mfx.GopPicSize = pv->param.gop.gop_pic_size;

    // set the Hyper Encode structure
    if (pv->param.hyperEncodeParam.Mode != MFX_HYPERMODE_OFF)
    {
        if (pv->param.videoParam->mfx.CodecId == MFX_CODEC_HEVC)
        {
            pv->param.videoParam->mfx.IdrInterval = 1;
        }
        else if (pv->param.videoParam->mfx.CodecId == MFX_CODEC_AVC)
        {
            pv->param.videoParam->mfx.IdrInterval = 0;
        }
        pv->param.videoParam->AsyncDepth = 60;
    }
    // sanitize some settings that affect memory consumption
    if (!pv->is_sys_mem)
    {
        // limit these to avoid running out of resources (causes hang)
        pv->param.videoParam->mfx.GopRefDist   = FFMIN(pv->param.videoParam->mfx.GopRefDist,
                                                       pv->param.rc.lookahead ? 8 : 16);
        pv->param.codingOption2.LookAheadDepth = FFMIN(pv->param.codingOption2.LookAheadDepth,
                                                       pv->param.rc.lookahead ? (48 - pv->param.videoParam->mfx.GopRefDist -
                                                                                 3 * !pv->param.videoParam->mfx.GopRefDist) : 0);
    }
    else
    {
        // encode-only is a bit less sensitive to memory issues
        pv->param.videoParam->mfx.GopRefDist   = FFMIN(pv->param.videoParam->mfx.GopRefDist, 16);
        pv->param.codingOption2.LookAheadDepth = FFMIN(pv->param.codingOption2.LookAheadDepth,
                                                       pv->param.rc.lookahead ? 100 : 0);
    }
    if (pv->param.rc.lookahead)
    {
        // LookAheadDepth 10 will cause a hang with some driver versions
        pv->param.codingOption2.LookAheadDepth = FFMAX(pv->param.codingOption2.LookAheadDepth, 11);
    }
    /*
     * We may need to adjust GopRefDist, GopPicSize and
     * NumRefFrame to enable or disable B-pyramid, so do it last.
     */
    qsv_handle_breftype(pv);

    /*
     * init a dummy encode-only session to get the SPS/PPS
     * and the final output settings sanitized by Media SDK
     * this is fine since the actual encode will use the same
     * values for all parameters relevant to the output bitstream
     */
    int err;
    mfxStatus sts;
    mfxVersion version;
    mfxVideoParam videoParam;
    mfxExtBuffer *extParamArray[5];
    mfxSession session = (mfxSession)0;
    mfxExtCodingOption  option1_buf, *option1 = &option1_buf;
    mfxExtCodingOption2 option2_buf, *option2 = &option2_buf;
    mfxExtCodingOptionSPSPPS sps_pps_buf, *sps_pps = &sps_pps_buf;
    mfxExtAV1BitstreamParam av1_bitstream_buf, *av1_bitstream = &av1_bitstream_buf;
    mfxExtHyperModeParam hyper_encode_buf, *hyper_encode = &hyper_encode_buf;
    version.Major = HB_QSV_MINVERSION_MAJOR;
    version.Minor = HB_QSV_MINVERSION_MINOR;
    uint32_t render_node = hb_qsv_get_adapter_render_node(hb_qsv_get_adapter_index());
    sts = hb_qsv_create_mfx_session(pv->qsv_info->implementation, render_node, &version, &session);
    if (sts != MFX_ERR_NONE)
    {
        hb_error("encqsvInit: MFXInit failed (%d) with implementation %d", sts, pv->qsv_info->implementation);
        return -1;
    }

    if (pv->qsv_info->implementation & MFX_IMPL_HARDWARE_ANY)
    {
        // On linux, the handle to the VA display must be set.
        // This code is essentially a NOP other platforms.
        job->qsv.ctx->display = hb_qsv_display_init(render_node);
        if (job->qsv.ctx->display != NULL)
        {
            MFXVideoCORE_SetHandle(session, job->qsv.ctx->display->mfxType,
                                   (mfxHDL)job->qsv.ctx->display->handle);
        }
    }

    /* Query the API version for hb_qsv_load_plugins */
    sts = MFXQueryVersion(session, &version);
    if (sts != MFX_ERR_NONE)
    {
        hb_error("encqsvInit: MFXQueryVersion failed (%d)", sts);
        MFXClose(session);
        return -1;
    }

    /* MFXVideoENCODE_Init with desired encoding parameters */
    sts = MFXVideoENCODE_Init(session, pv->param.videoParam);
// workaround for the early 15.33.x driver, should be removed later
#define HB_DRIVER_FIX_33
#ifdef  HB_DRIVER_FIX_33
    int la_workaround = 0;
    if (sts < MFX_ERR_NONE &&
        pv->param.videoParam->mfx.RateControlMethod == MFX_RATECONTROL_LA)
    {
        pv->param.videoParam->mfx.RateControlMethod = MFX_RATECONTROL_CBR;
        sts = MFXVideoENCODE_Init(session, pv->param.videoParam);
        la_workaround = 1;
    }
#endif
    if (sts < MFX_ERR_NONE) // ignore warnings
    {
        hb_error("encqsvInit: MFXVideoENCODE_Init failed (%d)", sts);
        err = log_encoder_params(pv, pv->param.videoParam);
        if (err < 0)
        {
            hb_error("encqsvInit: log_encoder_params failed (%d)", err);
        }
        MFXClose(session);
        return -1;
    }
    /* Prepare the structures for query */
    memset(&videoParam, 0, sizeof(mfxVideoParam));
    videoParam.ExtParam = extParamArray;
    videoParam.NumExtParam = 0;
    // introduced in API 1.3
    memset(sps_pps, 0, sizeof(mfxExtCodingOptionSPSPPS));
    sps_pps->Header.BufferId = MFX_EXTBUFF_CODING_OPTION_SPSPPS;
    sps_pps->Header.BufferSz = sizeof(mfxExtCodingOptionSPSPPS);
    sps_pps->SPSId           = 0;
    sps_pps->SPSBuffer       = w->config->h264.sps;
    sps_pps->SPSBufSize      = sizeof(w->config->h264.sps);
    sps_pps->PPSId           = 0;
    sps_pps->PPSBuffer       = w->config->h264.pps;
    sps_pps->PPSBufSize      = sizeof(w->config->h264.pps);
    if (pv->param.videoParam->mfx.CodecId == MFX_CODEC_AVC)
    {
        videoParam.ExtParam[videoParam.NumExtParam++] = (mfxExtBuffer*)sps_pps;
    }
    // introduced in API 1.0
    memset(option1, 0, sizeof(mfxExtCodingOption));
    option1->Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
    option1->Header.BufferSz = sizeof(mfxExtCodingOption);
    if (pv->qsv_info->capabilities & HB_QSV_CAP_OPTION1)
    {
        videoParam.ExtParam[videoParam.NumExtParam++] = (mfxExtBuffer*)option1;
    }
    // introduced in API 1.6
    memset(option2, 0, sizeof(mfxExtCodingOption2));
    option2->Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
    option2->Header.BufferSz = sizeof(mfxExtCodingOption2);
    if (pv->qsv_info->capabilities & HB_QSV_CAP_OPTION2)
    {
        videoParam.ExtParam[videoParam.NumExtParam++] = (mfxExtBuffer*)option2;
    }
    // introduced in API 2.5
    memset(av1_bitstream, 0, sizeof(mfxExtAV1BitstreamParam));
    av1_bitstream->Header.BufferId = MFX_EXTBUFF_AV1_BITSTREAM_PARAM;
    av1_bitstream->Header.BufferSz = sizeof(mfxExtAV1BitstreamParam);
    if (pv->qsv_info->capabilities & HB_QSV_CAP_AV1_BITSTREAM)
    {
        videoParam.ExtParam[videoParam.NumExtParam++] = (mfxExtBuffer*)av1_bitstream;
    }
    memset(hyper_encode, 0, sizeof(mfxExtHyperModeParam));
    hyper_encode->Header.BufferId = MFX_EXTBUFF_HYPER_MODE_PARAM;
    hyper_encode->Header.BufferSz = sizeof(mfxExtHyperModeParam);
    if (pv->qsv_info->capabilities & HB_QSV_CAP_HYPERENCODE)
    {
        videoParam.ExtParam[videoParam.NumExtParam++] = (mfxExtBuffer*)hyper_encode;
    }
    /* Query actual encoding parameters after MFXVideoENCODE_Init, some of them could be overridden */
    sts = MFXVideoENCODE_GetVideoParam(session, &videoParam);
    if (sts != MFX_ERR_NONE)
    {
        hb_error("encqsvInit: MFXVideoENCODE_GetVideoParam failed (%d)", sts);
        MFXClose(session);
        return -1;
    }

    /* We have the final encoding parameters, now get the headers for muxing */
    if (videoParam.mfx.CodecId == MFX_CODEC_AVC)
    {
        // remove 4-byte Annex B NAL unit prefix (0x00 0x00 0x00 0x01)
        w->config->h264.sps_length = sps_pps->SPSBufSize - 4;
        memmove(w->config->h264.sps, w->config->h264.sps + 4,
                w->config->h264.sps_length);
        w->config->h264.pps_length = sps_pps->PPSBufSize - 4;
        memmove(w->config->h264.pps, w->config->h264.pps + 4,
                w->config->h264.pps_length);
    }
    else if (videoParam.mfx.CodecId == MFX_CODEC_HEVC)
    {
        if (qsv_hevc_make_header(w, session, &videoParam) < 0)
        {
            hb_error("encqsvInit: qsv_hevc_make_header failed");
            MFXVideoENCODE_Close(session);
            MFXClose(session);
            return -1;
        }
    }

    /* We don't need this encode session once we have the header */
    MFXVideoENCODE_Close(session);

#ifdef HB_DRIVER_FIX_33
    if (la_workaround)
    {
        videoParam.mfx.RateControlMethod =
        pv->param.videoParam->mfx.RateControlMethod = MFX_RATECONTROL_LA;
        option2->LookAheadDepth = pv->param.codingOption2.LookAheadDepth;
        hb_log("encqsvInit: using LookAhead workaround (\"early 33 fix\")");
    }
#endif

    // when using system memory, we re-use this same session
    if (pv->is_sys_mem)
    {
        pv->mfx_session = session;
    }
    else
    {
        MFXClose(session);
    }

    /* B-frame related setup */
    if (videoParam.mfx.GopRefDist > 1)
    {
        /* the muxer needs to know to the init_delay */
        switch (videoParam.mfx.CodecId)
        {
            case MFX_CODEC_AVC:
            case MFX_CODEC_HEVC:
                pv->init_delay = &w->config->init_delay;
                break;
            default:
                break;
        }

        /* let the muxer know that it should expect B-frames */
        job->areBframes = 1;

        /* holds the PTS sequence in display order, used to generate DTS */
        pv->list_dts = hb_list_init();
    }

    err = log_encoder_params(pv, &videoParam);
    if (err < 0)
    {
        hb_error("encqsvInit: log_encoder_params failed (%d)", err);
        return -1;
    }
    pv->param.videoParam->mfx = videoParam.mfx;
    // AsyncDepth has now been set and/or modified by Media SDK
    // fall back to default if zero
    pv->max_async_depth = videoParam.AsyncDepth ? videoParam.AsyncDepth : hb_qsv_param_default_async_depth();
    pv->async_depth     = 0;

    return 0;
}

void encqsvClose(hb_work_object_t *w)
{
    hb_work_private_t *pv = w->private_data;
    int i;

    if (pv != NULL && pv->job != NULL && pv->job->qsv.ctx != NULL &&
        pv->job->qsv.ctx->is_context_active)
    {

        hb_qsv_context *qsv_ctx       = pv->job->qsv.ctx;
        hb_qsv_space   *qsv_enc_space = pv->job->qsv.ctx->enc_space;

        if (qsv_ctx != NULL)
        {
            hb_qsv_uninit_enc(pv->job);
            if (qsv_enc_space != NULL)
            {
                if (qsv_enc_space->is_init_done)
                {
                    for (i = hb_qsv_list_count(qsv_enc_space->tasks); i >= 1; i--)
                    {
                        hb_qsv_task *task = hb_qsv_list_item(qsv_enc_space->tasks,
                                                             i - 1);
                        if (task != NULL)
                        {
                            if (task->bs != NULL)
                            {
                                av_freep(&task->bs->Data);
                            }
                            hb_qsv_list_rem(qsv_enc_space->tasks, task);
                            av_freep(&task->bs);
                            av_freep(&task);
                        }
                    }
                    hb_qsv_list_close(&qsv_enc_space->tasks);

                    for (i = 0; i < qsv_enc_space->surface_num; i++)
                    {
                        if (pv->is_sys_mem)
                        {
                            av_freep(&qsv_enc_space->p_surfaces[i]->Data.Y);
                        }
                        av_freep(&qsv_enc_space->p_surfaces[i]);
                    }
                    qsv_enc_space->surface_num = 0;

                    for (i = 0; i < qsv_enc_space->sync_num; i++)
                    {
                        av_freep(&qsv_enc_space->p_syncp[i]->p_sync);
                        av_freep(&qsv_enc_space->p_syncp[i]);
                    }
                    qsv_enc_space->sync_num = 0;
                }
                qsv_enc_space->is_init_done = 0;
            }
        }
    }

    if (pv != NULL)
    {
        hb_chapter_queue_close(&pv->chapter_queue);
        if (pv->delayed_processing != NULL)
        {
            /* the list is already empty */
            hb_list_close(&pv->delayed_processing);
        }
        if (pv->list_dts != NULL)
        {
            int64_t *item;
            while ((item = hb_list_item(pv->list_dts, 0)) != NULL)
            {
                hb_list_rem(pv->list_dts, item);
                free(item);
            }
            hb_list_close(&pv->list_dts);
        }
        hb_buffer_list_close(&pv->encoded_frames);
    }

    free(pv);
    w->private_data = NULL;
}

static void compute_init_delay(hb_work_private_t *pv, mfxBitstream *bs)
{
    if (pv->init_delay == NULL)
    {
        return; // not needed or already set
    }

    /*
     * In the MP4 container, DT(0) = STTS(0) = 0.
     *
     * Which gives us:
     * CT(0) = CTTS(0) + STTS(0) = CTTS(0) = PTS(0) - DTS(0)
     * When DTS(0) < PTS(0), we then have:
     * CT(0) > 0 for video, but not audio (breaks A/V sync).
     *
     * This is typically solved by writing an edit list shifting
     * video samples by the initial delay, PTS(0) - DTS(0).
     *
     * See:
     * ISO/IEC 14496-12:2008(E), ISO base media file format
     *  - 8.6.1.2 Decoding Time to Sample Box
     */
    if (pv->qsv_info->capabilities & HB_QSV_CAP_MSDK_API_1_6)
    {
        /* compute init_delay (in ticks) based on the DTS provided by MSDK. */
        int64_t init_delay = bs->TimeStamp - bs->DecodeTimeStamp;

        /*
         * we also need to know the delay in frames to generate DTS.
         *
         * compute it based on the init_delay and average frame duration,
         * and account for potential rounding errors due to the timebase.
         */
        double avg_frame_dur = ((double)pv->job->vrate.den /
                                (double)pv->job->vrate.num * 90000.);

        pv->bfrm_delay = (init_delay + (avg_frame_dur / 2)) / avg_frame_dur;

        if (pv->bfrm_delay < 1 || pv->bfrm_delay > BFRM_DELAY_MAX)
        {
            hb_log("compute_init_delay: "
                   "invalid delay %d (PTS: %"PRIu64", DTS: %"PRId64")",
                   pv->bfrm_delay, bs->TimeStamp, bs->DecodeTimeStamp);

            /* we have B-frames, the frame delay should be at least 1 */
            if (pv->bfrm_delay < 1)
            {
                mfxStatus sts;
                mfxVideoParam videoParam;
                mfxSession session = pv->job->qsv.ctx->mfx_session;

                memset(&videoParam, 0, sizeof(mfxVideoParam));

                sts = MFXVideoENCODE_GetVideoParam(session, &videoParam);
                if (sts != MFX_ERR_NONE)
                {
                    hb_log("compute_init_delay: "
                           "MFXVideoENCODE_GetVideoParam failed (%d)", sts);
                    pv->bfrm_delay = 1;
                }
                else
                {
                    /* usually too large, but should cover all cases */
                    pv->bfrm_delay = FFMIN(pv->frames_in             - 1,
                                           videoParam.mfx.GopRefDist - 1);
                }
            }

            pv->bfrm_delay = FFMIN(BFRM_DELAY_MAX, pv->bfrm_delay);
        }

        pv->init_delay[0] = pv->init_pts[pv->bfrm_delay] - pv->init_pts[0];
    }
    else
    {
        /*
         * we can't get the DTS from MSDK, so we need to generate our own.
         *
         * B-pyramid not possible here, so the delay in frames is always 1.
         */
        pv->bfrm_delay    = 1;
        pv->init_delay[0] = pv->init_pts[1] - pv->init_pts[0];
    }

    /* This can come in handy */
    hb_deep_log(2, "compute_init_delay: %d (%d frames)", pv->init_delay[0], pv->bfrm_delay);

    /* The delay only needs to be set once. */
    pv->init_delay = NULL;
}

static void qsv_bitstream_slurp(hb_work_private_t *pv, mfxBitstream *bs)
{
    hb_buffer_t *buf;

    if (pv->param.videoParam->mfx.CodecId == MFX_CODEC_AVC)
    {
        /*
         * We provided the muxer with the parameter sets in an MP4-compatible
         * format (ISO/IEC 14496-15). We need to convert the bitstream to the
         * same format to match the extradata.
         */
        if ((buf = hb_nal_bitstream_annexb_to_mp4(bs->Data + bs->DataOffset,
                                                  bs->DataLength)) == NULL)
        {
            hb_error("encqsv: hb_nal_bitstream_annexb_to_mp4 failed");
            goto fail;
        }
    }
    else
    {
        /* Both extradata and bitstream are in Annex B format. */
        if ((buf = hb_buffer_init(bs->DataLength)) == NULL)
        {
            hb_error("encqsv: hb_buffer_init failed");
            goto fail;
        }
        memcpy(buf->data, bs->Data + bs->DataOffset, bs->DataLength);
    }
    bs->DataLength = bs->DataOffset = 0;
    bs->MaxLength  = pv->job->qsv.ctx->enc_space->p_buf_max_size;

    buf->s.frametype = hb_qsv_frametype_xlat(bs->FrameType, &buf->s.flags);
    if (pv->param.videoParam->mfx.CodecId == MFX_CODEC_HEVC)
    {
        size_t   len = buf->size;
        uint8_t *pos = buf->data;
        uint8_t *end = pos + len;
        while ((pos = hb_annexb_find_next_nalu(pos, &len)) != NULL)
        {
            if (HB_HEVC_NALU_KEYFRAME((pos[0] >> 1) & 0x3f))
            {
                buf->s.flags |= HB_FLAG_FRAMETYPE_KEY;
                break;
            }
            len = end - pos;
            continue;
        }
    }
    buf->s.start     = buf->s.renderOffset = bs->TimeStamp;
    buf->s.stop      = buf->s.start + get_frame_duration(pv, buf);
    buf->s.duration  = buf->s.stop  - buf->s.start;

    /* compute the init_delay before setting the DTS */
    compute_init_delay(pv, bs);

    /*
     * Generate VFR-compatible output DTS based on input PTS.
     *
     * Depends on the B-frame delay:
     *
     * 0: ipts0,  ipts1, ipts2...
     * 1: ipts0 - ipts1, ipts1 - ipts1, ipts1,  ipts2...
     * 2: ipts0 - ipts2, ipts1 - ipts2, ipts2 - ipts2, ipts1...
     * ...and so on.
     */
    if (pv->bfrm_delay)
    {
        if (pv->frames_out <= pv->bfrm_delay)
        {
            buf->s.renderOffset = (pv->init_pts[pv->frames_out] -
                                   pv->init_pts[pv->bfrm_delay]);
        }
        else
        {
            buf->s.renderOffset = hb_qsv_pop_next_dts(pv->list_dts);
        }
    }

    /* check if B-pyramid is used even though it's disabled */
    if ((pv->param.gop.b_pyramid == 0)    &&
        (bs->FrameType & MFX_FRAMETYPE_B) &&
        (bs->FrameType & MFX_FRAMETYPE_REF))
    {
        hb_log("encqsv: BPyramid off not respected (delay: %d)", pv->bfrm_delay);

        /* don't pollute the log unnecessarily */
        pv->param.gop.b_pyramid = 1;
    }

    /* check for PTS < DTS */
    if (buf->s.start < buf->s.renderOffset)
    {
        hb_log("encqsv: PTS %"PRId64" < DTS %"PRId64" for frame %d with type '%s'",
               buf->s.start, buf->s.renderOffset, pv->frames_out + 1,
               hb_qsv_frametype_name(bs->FrameType));
    }

    /*
     * If we have a chapter marker pending and this frame's PTS
     * is at or after the marker's PTS, use it as the chapter start.
     */
    if (buf->s.flags & HB_FLAG_FRAMETYPE_KEY)
    {
        hb_chapter_dequeue(pv->chapter_queue, buf);
    }

    hb_buffer_list_append(&pv->encoded_frames, buf);
    pv->frames_out++;
    return;

fail:
    *pv->job->done_error = HB_ERROR_UNKNOWN;
    *pv->job->die        = 1;
}

static int qsv_enc_work(hb_work_private_t *pv,
                        hb_qsv_list *qsv_atom,
                        mfxFrameSurface1 *surface,
                        HBQSVFramesContext *frames_ctx)
{
    int err;
    mfxStatus sts;
    hb_qsv_context *qsv_ctx       = pv->job->qsv.ctx;
    hb_qsv_space   *qsv_enc_space = pv->job->qsv.ctx->enc_space;

    do
    {
        int sync_idx = hb_qsv_get_free_sync(qsv_enc_space, qsv_ctx);
        if (sync_idx == -1)
        {
            hb_error("encqsv: hb_qsv_get_free_sync failed");
            return -1;
        }
        hb_qsv_task *task = hb_qsv_list_item(qsv_enc_space->tasks,
                                             pv->async_depth);

        do
        {
            sts = MFXVideoENCODE_EncodeFrameAsync(qsv_ctx->mfx_session,
                                                  NULL, surface, task->bs,
                                                  qsv_enc_space->p_syncp[sync_idx]->p_sync);

            if (sts == MFX_ERR_MORE_DATA)
            {
                if(!pv->is_sys_mem && surface)
                {
                    hb_qsv_release_surface_from_pool_by_surface_pointer(frames_ctx, surface);
                }

                if (qsv_atom != NULL)
                {
                    hb_list_add(pv->delayed_processing, qsv_atom);
                }
                ff_qsv_atomic_dec(&qsv_enc_space->p_syncp[sync_idx]->in_use);
                break;
            }
            else if (sts < MFX_ERR_NONE)
            {
                hb_error("encqsv: MFXVideoENCODE_EncodeFrameAsync failed (%d)", sts);
                return -1;
            }
            else if (sts == MFX_WRN_DEVICE_BUSY)
            {
                hb_qsv_sleep(10); // device is busy, wait then repeat the call
                continue;
            }
            else
            {
                hb_qsv_stage *new_stage        = hb_qsv_stage_init();
                new_stage->type                = HB_QSV_ENCODE;
                new_stage->in.p_surface        = surface;
                new_stage->in.p_frames_ctx     = frames_ctx;
                new_stage->out.sync            = qsv_enc_space->p_syncp[sync_idx];
                new_stage->out.p_bs            = task->bs;
                task->stage                    = new_stage;
                pv->async_depth++;

                if(!pv->is_sys_mem && surface)
                {
                    hb_qsv_release_surface_from_pool_by_surface_pointer(frames_ctx, surface);
                }

                if (qsv_atom != NULL)
                {
                    hb_qsv_add_stagee(&qsv_atom, new_stage, HAVE_THREADS);
                }
                else
                {
                    /* encode-only or flushing */
                    hb_qsv_list *new_qsv_atom = hb_qsv_list_init(HAVE_THREADS);
                    hb_qsv_add_stagee(&new_qsv_atom,  new_stage, HAVE_THREADS);
                    hb_qsv_list_add  (qsv_ctx->pipes, new_qsv_atom);
                }

                int i = hb_list_count(pv->delayed_processing);
                while (--i >= 0)
                {
                    hb_qsv_list *item = hb_list_item(pv->delayed_processing, i);

                    if (item != NULL)
                    {
                        hb_list_rem(pv->delayed_processing,  item);
                        hb_qsv_flush_stages(qsv_ctx->pipes, &item, 1);
                    }
                }
                break;
            }

            ff_qsv_atomic_dec(&qsv_enc_space->p_syncp[sync_idx]->in_use);
            break;
        }
        while (sts >= MFX_ERR_NONE);

        do
        {
            if (pv->async_depth == 0) break;

            /* we've done enough asynchronous operations or we're flushing */
            if (pv->async_depth >= pv->max_async_depth || surface == NULL)
            {
                hb_qsv_task *task = hb_qsv_list_item(qsv_enc_space->tasks, 0);
                pv->async_depth--;

                /* perform a sync operation to get the output bitstream */
                err = hb_qsv_wait_on_sync(qsv_ctx, task->stage);
                if (err < 0)
                {
                    hb_error("encqsv: hb_qsv_wait_on_sync failed (%d)", err);
                    return err;
                }

                if (task->bs->DataLength > 0)
                {
                    hb_qsv_list *pipe = hb_qsv_pipe_by_stage(qsv_ctx->pipes,
                                                             task->stage);
                    hb_qsv_flush_stages(qsv_ctx->pipes, &pipe, 1);

                    /* get the encoded frame from the bitstream */
                    qsv_bitstream_slurp(pv, task->bs);

                    /* shift for fifo */
                    if (pv->async_depth)
                    {
                        hb_qsv_list_rem(qsv_enc_space->tasks, task);
                        hb_qsv_list_add(qsv_enc_space->tasks, task);
                    }
                    task->stage = NULL;
                }
            }
        }
        while (surface == NULL);
    }
    while (surface == NULL && sts != MFX_ERR_MORE_DATA);

    return 0;
}

int encqsvWork(hb_work_object_t *w, hb_buffer_t **buf_in, hb_buffer_t **buf_out)
{
    int err;
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *in       = *buf_in;
    hb_job_t *job         = pv->job;
    while (qsv_enc_init(pv) >= 2)
    {
        hb_qsv_sleep(1); // encoding not initialized, wait and repeat the call
    }

    if (*job->die)
    {
        goto fail; // unrecoverable error, don't attempt to encode
    }

    /*
     * EOF on input. Flush the decoder, then send the
     * EOF downstream to let the muxer know we're done.
     */
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        err = qsv_enc_work(pv, NULL, NULL, NULL);
        if (err < 0)
        {
            hb_error("encqsvWork: EOF qsv_enc_work failed %d", err);
            goto fail;
        }
        hb_buffer_list_append(&pv->encoded_frames, in);
        *buf_out = hb_buffer_list_clear(&pv->encoded_frames);
        *buf_in = NULL; // don't let 'work_loop' close this buffer
        return HB_WORK_DONE;
    }

    mfxFrameSurface1   *surface     = NULL;
    HBQSVFramesContext *frames_ctx  = NULL;
    hb_qsv_list      *qsv_atom      = NULL;
    hb_qsv_context   *qsv_ctx       = job->qsv.ctx;
    hb_qsv_space     *qsv_enc_space = job->qsv.ctx->enc_space;

    if (pv->is_sys_mem)
    {
        mfxFrameInfo *info = &pv->param.videoParam->mfx.FrameInfo;
        int surface_index  = hb_qsv_get_free_surface(qsv_enc_space, qsv_ctx, info,
                                                     QSV_PART_ANY);
        if (surface_index == -1)
        {
            hb_error("encqsv: hb_qsv_get_free_surface failed");
            goto fail;
        }

        surface = qsv_enc_space->p_surfaces[surface_index];
        qsv_copy_buffer_to_surface(surface, in);
    }
    else
    {
        QSVMid *mid = NULL;
        if (in->qsv_details.frame && in->qsv_details.frame->data[3])
        {
            surface = ((mfxFrameSurface1*)in->qsv_details.frame->data[3]);
            frames_ctx = in->qsv_details.qsv_frames_ctx;
            hb_qsv_get_mid_by_surface_from_pool(frames_ctx, surface, &mid);
            hb_qsv_replace_surface_mid(frames_ctx, mid, surface);
        }
        else
        {
            hb_error("encqsv: in->qsv_details no surface available");
            goto fail;
        }

        // At this point, enc_qsv takes ownership of the QSV resources
        // in the 'in' buffer.
        in->qsv_details.qsv_atom = NULL;

        /*
         * QSV decoding fills the QSV context's dts_seq list, we need to
         * pop this surface's DTS so dts_seq doesn't grow unnecessarily.
         */
        hb_qsv_dts_pop(qsv_ctx);
    }

    /*
     * Debugging code to check that the upstream modules have generated
     * a continuous, self-consistent frame stream.
     */
    if (pv->last_start > in->s.start)
    {
        hb_log("encqsv: input continuity error, "
               "last start %"PRId64" start %"PRId64"",
               pv->last_start, in->s.start);
    }
    pv->last_start = in->s.start;

    /* for DTS generation */
    if (pv->frames_in <= BFRM_DELAY_MAX)
    {
        pv->init_pts[pv->frames_in] = in->s.start;
    }
    if (pv->frames_in)
    {
        hb_qsv_add_new_dts(pv->list_dts, in->s.start);
    }
    pv->frames_in++;

    /*
     * Chapters have to start with a keyframe, so request one here.
     *
     * Using an mfxEncodeCtrl structure to force key frame generation is not
     * possible when using a lookahead and frame reordering, so instead do
     * the following before encoding the frame attached to the chapter:
     *
     * - flush the encoder to encode and retrieve any buffered frames
     *
     * - do a hard reset (MFXVideoENCODE_Close, then Init) of
     *   the encoder to make sure the next frame is a keyframe
     *
     * The hard reset ensures encoding resumes with a clean state, avoiding
     * miscellaneous hard-to-diagnose issues that may occur when resuming
     * an encode after flushing the encoder or using MFXVideoENCODE_Reset.
     */
    if (in->s.new_chap > 0 && job->chapter_markers)
    {
        mfxStatus sts;

        err = qsv_enc_work(pv, NULL, NULL, NULL);
        if (err < 0)
        {
            hb_error("encqsvWork: new_chap qsv_enc_work failed %d", err);
            goto fail;
        }

        sts = MFXVideoENCODE_Close(qsv_ctx->mfx_session);
        if (sts != MFX_ERR_NONE)
        {
            hb_error("encqsv: MFXVideoENCODE_Close failed (%d)", sts);
            goto fail;
        }

        sts = MFXVideoENCODE_Init(qsv_ctx->mfx_session, pv->param.videoParam);
        if (sts < MFX_ERR_NONE)
        {
            hb_error("encqsv: MFXVideoENCODE_Init failed (%d)", sts);
            goto fail;
        }

        hb_chapter_enqueue(pv->chapter_queue, in);
    }

    /*
     * If interlaced encoding is requested during encoder initialization,
     * but the input mfxFrameSurface1 is flagged as progressive here,
     * the output bitstream will be progressive (according to MediaInfo).
     *
     * Assume the user knows what he's doing (say he is e.g. encoding a
     * progressive-flagged source using interlaced compression - he may
     * well have a good reason to do so; mis-flagged sources do exist).
     */
    surface->Info.PicStruct = pv->param.videoParam->mfx.FrameInfo.PicStruct;
    surface->Data.TimeStamp = in->s.start;
    save_frame_duration(pv, in);

    /*
     * Now that the input surface is setup, we can encode it.
     */
    err = qsv_enc_work(pv, qsv_atom, surface, frames_ctx);
    if (err < 0)
    {
        hb_error("encqsvWork: qsv_enc_work failed %d", err);
        goto fail;
    }

    if (in->qsv_details.frame)
    {
        in->qsv_details.frame->data[3] = 0;
    }

    *buf_out = hb_buffer_list_clear(&pv->encoded_frames);
    return HB_WORK_OK;

fail:
    if (*job->done_error == HB_ERROR_NONE)
    {
        *job->done_error  = HB_ERROR_UNKNOWN;
    }
    *job->die = 1;
    *buf_out  = NULL;
    return HB_WORK_ERROR;
}

#endif // HB_PROJECT_FEATURE_QSV
