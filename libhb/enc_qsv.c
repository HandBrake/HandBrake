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

#ifdef USE_QSV

#include "hb.h"
#include "qsv_common.h"
#include "qsv_memory.h"
#include "h264_common.h"

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

int  nal_find_start_code(uint8_t**, size_t*);
void parse_nalus        (uint8_t*,  size_t, hb_buffer_t*);

int  encqsvInit (hb_work_object_t*, hb_job_t*);
int  encqsvWork (hb_work_object_t*, hb_buffer_t**, hb_buffer_t**);
void encqsvClose(hb_work_object_t*);

hb_work_object_t hb_encqsv =
{
    WORK_ENCQSV,
    "H.264/AVC encoder (Intel QSV)",
    encqsvInit,
    encqsvWork,
    encqsvClose
};

struct hb_work_private_s
{
    hb_job_t *job;
    uint32_t  frames_in;
    uint32_t  frames_out;
    int64_t   last_start;

    hb_qsv_param_t param;
    av_qsv_space enc_space;
    hb_qsv_info_t *qsv_info;

    mfxEncodeCtrl force_keyframe;
    hb_list_t *delayed_chapters;
    int64_t next_chapter_pts;

#define BFRM_DELAY_MAX 16
    // for DTS generation (when MSDK API < 1.6 or VFR)
    uint32_t      *init_delay;
    int            bfrm_delay;
    int            bfrm_workaround;
    int64_t        init_pts[BFRM_DELAY_MAX + 1];
    hb_list_t     *list_dts;

    int64_t frame_duration[FRAME_INFO_SIZE];

    int async_depth;
    int max_async_depth;

    // if encode-only, system memory used
    int is_sys_mem;
    mfxSession mfx_session;
    struct SwsContext *sws_context_to_nv12;

    // whether to expect input from VPP or from QSV decode
    int is_vpp_present;

    // whether the encoder is initialized
    int init_done;

    hb_list_t *delayed_processing;
    hb_list_t *encoded_frames;
};

// used in delayed_chapters list
struct chapter_s
{
    int     index;
    int64_t start;
};

// for DTS generation (when MSDK API < 1.6 or VFR)
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

static const char* qsv_h264_profile_xlat(int profile)
{
    switch (profile)
    {
        case MFX_PROFILE_AVC_CONSTRAINED_BASELINE:
            return "Constrained Baseline";
        case MFX_PROFILE_AVC_BASELINE:
            return "Baseline";
        case MFX_PROFILE_AVC_EXTENDED:
            return "Extended";
        case MFX_PROFILE_AVC_MAIN:
            return "Main";
        case MFX_PROFILE_AVC_CONSTRAINED_HIGH:
            return "Constrained High";
        case MFX_PROFILE_AVC_PROGRESSIVE_HIGH:
            return "Progressive High";
        case MFX_PROFILE_AVC_HIGH:
            return "High";
        case MFX_PROFILE_UNKNOWN:
        default:
            return NULL;
    }
}

static const char* qsv_h264_level_xlat(int level)
{
    int i;
    for (i = 0; hb_h264_level_names[i] != NULL; i++)
    {
        if (hb_h264_level_values[i] == level)
        {
            return hb_h264_level_names[i];
        }
    }
    return NULL;
}

int qsv_enc_init(hb_work_private_t *pv)
{
    av_qsv_context *qsv = pv->job->qsv.ctx;
    hb_job_t       *job = pv->job;
    mfxStatus sts;
    int i;

    if (pv->init_done)
    {
        return 0;
    }

    if (qsv == NULL)
    {
        if (!pv->is_sys_mem)
        {
            hb_error("qsv_enc_init: decode enabled but no context!");
            return 3;
        }
        job->qsv.ctx = qsv = av_mallocz(sizeof(av_qsv_context));
    }

    av_qsv_space *qsv_encode = qsv->enc_space;
    if (qsv_encode == NULL)
    {
        // if only for encode
        if (pv->is_sys_mem)
        {
            // no need to use additional sync as encode only -> single thread
            av_qsv_add_context_usage(qsv, 0);

            // re-use the session from encqsvInit
            qsv->mfx_session = pv->mfx_session;
        }
        qsv->enc_space = qsv_encode = &pv->enc_space;
    }

    if (!pv->is_sys_mem)
    {
        if (!pv->is_vpp_present && job->list_filter != NULL)
        {
            for (i = 0; i < hb_list_count(job->list_filter); i++)
            {
                hb_filter_object_t *filter = hb_list_item(job->list_filter, i);
                if (filter->id == HB_FILTER_QSV_PRE  ||
                    filter->id == HB_FILTER_QSV_POST ||
                    filter->id == HB_FILTER_QSV)
                {
                    pv->is_vpp_present = 1;
                    break;
                }
            }
        }

        if (pv->is_vpp_present)
        {
            if (qsv->vpp_space == NULL)
            {
                return 2;
            }
            for (i = 0; i < av_qsv_list_count(qsv->vpp_space); i++)
            {
                av_qsv_space *vpp = av_qsv_list_item(qsv->vpp_space, i);
                if (!vpp->is_init_done)
                {
                    return 2;
                }
            }
        }

        av_qsv_space *dec_space = qsv->dec_space;
        if (dec_space == NULL || !dec_space->is_init_done)
        {
            return 2;
        }
    }
    else
    {
        pv->sws_context_to_nv12 = hb_sws_get_context(job->width, job->height,
                                                     AV_PIX_FMT_YUV420P,
                                                     job->width, job->height,
                                                     AV_PIX_FMT_NV12,
                                                     SWS_LANCZOS|SWS_ACCURATE_RND);
    }

    // allocate tasks
    qsv_encode->p_buf_max_size = AV_QSV_BUF_SIZE_DEFAULT;
    qsv_encode->tasks          = av_qsv_list_init(HAVE_THREADS);
    for (i = 0; i < pv->max_async_depth; i++)
    {
        av_qsv_task *task    = av_mallocz(sizeof(av_qsv_task));
        task->bs             = av_mallocz(sizeof(mfxBitstream));
        task->bs->Data       = av_mallocz(sizeof(uint8_t) * qsv_encode->p_buf_max_size);
        task->bs->MaxLength  = qsv_encode->p_buf_max_size;
        task->bs->DataLength = 0;
        task->bs->DataOffset = 0;
        av_qsv_list_add(qsv_encode->tasks, task);
    }

    // setup surface allocation
    pv->param.videoParam->IOPattern = (pv->is_sys_mem                 ?
                                       MFX_IOPATTERN_IN_SYSTEM_MEMORY :
                                       MFX_IOPATTERN_IN_OPAQUE_MEMORY);
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
                                        pv->max_async_depth, AV_QSV_SURFACE_NUM);
        if (qsv_encode->surface_num <= 0)
        {
            qsv_encode->surface_num = AV_QSV_SURFACE_NUM;
        }
        for (i = 0; i < qsv_encode->surface_num; i++)
        {
            mfxFrameSurface1 *surface = av_mallocz(sizeof(mfxFrameSurface1));
            mfxFrameInfo info         = pv->param.videoParam->mfx.FrameInfo;
            surface->Info             = info;
            surface->Data.Pitch       = info.Width;
            surface->Data.Y           = av_mallocz(info.Width * info.Height);
            surface->Data.VU          = av_mallocz(info.Width * info.Height / 2);
            qsv_encode->p_surfaces[i] = surface;
        }
    }
    else
    {
        av_qsv_space *in_space = qsv->dec_space;
        if (pv->is_vpp_present)
        {
            // we get our input from VPP instead
            in_space = av_qsv_list_item(qsv->vpp_space,
                                        av_qsv_list_count(qsv->vpp_space) - 1);
        }
        // introduced in API 1.3
        memset(&qsv_encode->ext_opaque_alloc, 0, sizeof(mfxExtOpaqueSurfaceAlloc));
        qsv_encode->ext_opaque_alloc.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
        qsv_encode->ext_opaque_alloc.Header.BufferSz = sizeof(mfxExtOpaqueSurfaceAlloc);
        qsv_encode->ext_opaque_alloc.In.Surfaces     = in_space->p_surfaces;
        qsv_encode->ext_opaque_alloc.In.NumSurface   = in_space->surface_num;
        qsv_encode->ext_opaque_alloc.In.Type         = qsv_encode->request[0].Type;
        pv->param.videoParam->ExtParam[pv->param.videoParam->NumExtParam++] = (mfxExtBuffer*)&qsv_encode->ext_opaque_alloc;
    }

    // allocate sync points
    qsv_encode->sync_num = (qsv_encode->surface_num                         ?
                            FFMIN(qsv_encode->surface_num, AV_QSV_SYNC_NUM) :
                            AV_QSV_SYNC_NUM);
    for (i = 0; i < qsv_encode->sync_num; i++)
    {
        qsv_encode->p_syncp[i] = av_mallocz(sizeof(av_qsv_sync));
        AV_QSV_CHECK_POINTER(qsv_encode->p_syncp[i], MFX_ERR_MEMORY_ALLOC);
        qsv_encode->p_syncp[i]->p_sync = av_mallocz(sizeof(mfxSyncPoint));
        AV_QSV_CHECK_POINTER(qsv_encode->p_syncp[i]->p_sync, MFX_ERR_MEMORY_ALLOC);
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
    qsv_encode->is_init_done = 1;

    mfxIMPL impl;
    mfxVersion version;
    // log actual implementation details now that we know them
    if ((MFXQueryIMPL   (qsv->mfx_session, &impl)    == MFX_ERR_NONE) &&
        (MFXQueryVersion(qsv->mfx_session, &version) == MFX_ERR_NONE))
    {
        hb_log("qsv_enc_init: using '%s' implementation, API: %"PRIu16".%"PRIu16"",
               hb_qsv_impl_get_name(impl), version.Major, version.Minor);
    }
    else
    {
        hb_log("qsv_enc_init: MFXQueryIMPL/MFXQueryVersion failure");
    }

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
    hb_work_private_t *pv = calloc(1, sizeof(hb_work_private_t));
    w->private_data       = pv;

    pv->job                = job;
    pv->is_sys_mem         = hb_qsv_decode_is_enabled(job) == 0;
    pv->qsv_info           = hb_qsv_info_get(job->vcodec);
    pv->delayed_processing = hb_list_init();
    pv->encoded_frames     = hb_list_init();
    pv->last_start         = INT64_MIN;

    // set up a re-usable mfxEncodeCtrl to force keyframes (e.g. for chapters)
    pv->force_keyframe.QP          = 0;
    pv->force_keyframe.FrameType   = MFX_FRAMETYPE_I|MFX_FRAMETYPE_IDR|MFX_FRAMETYPE_REF;
    pv->force_keyframe.NumExtParam = 0;
    pv->force_keyframe.NumPayload  = 0;
    pv->force_keyframe.ExtParam    = NULL;
    pv->force_keyframe.Payload     = NULL;

    pv->next_chapter_pts = AV_NOPTS_VALUE;
    pv->delayed_chapters = hb_list_init();

    // default encoding parameters
    if (hb_qsv_param_default_preset(&pv->param, &pv->enc_space.m_mfxVideoParam,
                                     pv->qsv_info, job->encoder_preset))
    {
        hb_error("encqsvInit: hb_qsv_param_default_preset failed");
        return -1;
    }

    // set AsyncDepth to match that of decode and VPP
    pv->param.videoParam->AsyncDepth = job->qsv.async_depth;

    // enable and set colorimetry (video signal information)
    pv->param.videoSignalInfo.ColourDescriptionPresent = 1;
    switch (job->color_matrix_code)
    {
        case 4:
            // custom
            pv->param.videoSignalInfo.ColourPrimaries         = job->color_prim;
            pv->param.videoSignalInfo.TransferCharacteristics = job->color_transfer;
            pv->param.videoSignalInfo.MatrixCoefficients      = job->color_matrix;
            break;
        case 3:
            // ITU BT.709 HD content
            pv->param.videoSignalInfo.ColourPrimaries         = HB_COLR_PRI_BT709;
            pv->param.videoSignalInfo.TransferCharacteristics = HB_COLR_TRA_BT709;
            pv->param.videoSignalInfo.MatrixCoefficients      = HB_COLR_MAT_BT709;
            break;
        case 2:
            // ITU BT.601 DVD or SD TV content (PAL)
            pv->param.videoSignalInfo.ColourPrimaries         = HB_COLR_PRI_EBUTECH;
            pv->param.videoSignalInfo.TransferCharacteristics = HB_COLR_TRA_BT709;
            pv->param.videoSignalInfo.MatrixCoefficients      = HB_COLR_MAT_SMPTE170M;
            break;
        case 1:
            // ITU BT.601 DVD or SD TV content (NTSC)
            pv->param.videoSignalInfo.ColourPrimaries         = HB_COLR_PRI_SMPTEC;
            pv->param.videoSignalInfo.TransferCharacteristics = HB_COLR_TRA_BT709;
            pv->param.videoSignalInfo.MatrixCoefficients      = HB_COLR_MAT_SMPTE170M;
            break;
        default:
            // detected during scan
            pv->param.videoSignalInfo.ColourPrimaries         = job->title->color_prim;
            pv->param.videoSignalInfo.TransferCharacteristics = job->title->color_transfer;
            pv->param.videoSignalInfo.MatrixCoefficients      = job->title->color_matrix;
            break;
    }

    // parse user-specified encoder options, if present
    if (job->encoder_options != NULL && *job->encoder_options)
    {
        hb_dict_t *options_list;
        hb_dict_entry_t *option = NULL;
        options_list = hb_encopts_to_dict(job->encoder_options, job->vcodec);
        while ((option = hb_dict_next(options_list, option)) != NULL)
        {
            switch (hb_qsv_param_parse(&pv->param,  pv->qsv_info,
                                       option->key, option->value))
            {
                case HB_QSV_PARAM_OK:
                    break;

                case HB_QSV_PARAM_BAD_NAME:
                    hb_log("encqsvInit: hb_qsv_param_parse: bad key %s",
                           option->key);
                    break;
                case HB_QSV_PARAM_BAD_VALUE:
                    hb_log("encqsvInit: hb_qsv_param_parse: bad value %s for key %s",
                           option->value, option->key);
                    break;
                case HB_QSV_PARAM_UNSUPPORTED:
                    hb_log("encqsvInit: hb_qsv_param_parse: unsupported option %s",
                           option->key);
                    break;

                case HB_QSV_PARAM_ERROR:
                default:
                    hb_log("encqsvInit: hb_qsv_param_parse: unknown error");
                    break;
            }
        }
        hb_dict_free(&options_list);
    }

    // reload colorimetry in case values were set in encoder_options
    if (pv->param.videoSignalInfo.ColourDescriptionPresent)
    {
        job->color_matrix_code = 4;
        job->color_prim        = pv->param.videoSignalInfo.ColourPrimaries;
        job->color_transfer    = pv->param.videoSignalInfo.TransferCharacteristics;
        job->color_matrix      = pv->param.videoSignalInfo.MatrixCoefficients;
    }
    else
    {
        job->color_matrix_code = 0;
        job->color_prim        = HB_COLR_PRI_UNDEF;
        job->color_transfer    = HB_COLR_TRA_UNDEF;
        job->color_matrix      = HB_COLR_MAT_UNDEF;
    }

    // sanitize values that may exceed the Media SDK variable size
    int64_t vrate, vrate_base;
    int64_t par_width, par_height;
    hb_limit_rational64(&vrate, &vrate_base,
                        job->vrate, job->vrate_base, UINT32_MAX);
    hb_limit_rational64(&par_width, &par_height,
                        job->anamorphic.par_width,
                        job->anamorphic.par_height, UINT16_MAX);

    // some encoding parameters are used by filters to configure their output
    if (pv->param.videoParam->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE)
    {
        job->qsv.enc_info.align_height = AV_QSV_ALIGN32(job->height);
    }
    else
    {
        job->qsv.enc_info.align_height = AV_QSV_ALIGN16(job->height);
    }
    job->qsv.enc_info.align_width  = AV_QSV_ALIGN16(job->width);
    job->qsv.enc_info.pic_struct   = pv->param.videoParam->mfx.FrameInfo.PicStruct;
    job->qsv.enc_info.is_init_done = 1;

    // encode to H.264 and set FrameInfo
    pv->param.videoParam->mfx.CodecId                 = MFX_CODEC_AVC;
    pv->param.videoParam->mfx.CodecLevel              = MFX_LEVEL_UNKNOWN;
    pv->param.videoParam->mfx.CodecProfile            = MFX_PROFILE_UNKNOWN;
    pv->param.videoParam->mfx.FrameInfo.FourCC        = MFX_FOURCC_NV12;
    pv->param.videoParam->mfx.FrameInfo.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
    pv->param.videoParam->mfx.FrameInfo.FrameRateExtN = vrate;
    pv->param.videoParam->mfx.FrameInfo.FrameRateExtD = vrate_base;
    pv->param.videoParam->mfx.FrameInfo.AspectRatioW  = par_width;
    pv->param.videoParam->mfx.FrameInfo.AspectRatioH  = par_height;
    pv->param.videoParam->mfx.FrameInfo.CropX         = 0;
    pv->param.videoParam->mfx.FrameInfo.CropY         = 0;
    pv->param.videoParam->mfx.FrameInfo.CropW         = job->width;
    pv->param.videoParam->mfx.FrameInfo.CropH         = job->height;
    pv->param.videoParam->mfx.FrameInfo.PicStruct     = job->qsv.enc_info.pic_struct;
    pv->param.videoParam->mfx.FrameInfo.Width         = job->qsv.enc_info.align_width;
    pv->param.videoParam->mfx.FrameInfo.Height        = job->qsv.enc_info.align_height;

    // set H.264 profile and level
    if (job->encoder_profile != NULL && *job->encoder_profile &&
        strcasecmp(job->encoder_profile, "auto"))
    {
        if (!strcasecmp(job->encoder_profile, "baseline"))
        {
            pv->param.videoParam->mfx.CodecProfile = MFX_PROFILE_AVC_BASELINE;
        }
        else if (!strcasecmp(job->encoder_profile, "main"))
        {
            pv->param.videoParam->mfx.CodecProfile = MFX_PROFILE_AVC_MAIN;
        }
        else if (!strcasecmp(job->encoder_profile, "high"))
        {
            pv->param.videoParam->mfx.CodecProfile = MFX_PROFILE_AVC_HIGH;
        }
        else
        {
            hb_error("encqsvInit: bad profile %s", job->encoder_profile);
            return -1;
        }
    }
    if (job->encoder_level != NULL && *job->encoder_level &&
        strcasecmp(job->encoder_level, "auto"))
    {
        int err;
        int i = hb_qsv_atoindex(hb_h264_level_names, job->encoder_level, &err);
        if (err || i >= (sizeof(hb_h264_level_values) /
                         sizeof(hb_h264_level_values[0])))
        {
            hb_error("encqsvInit: bad level %s", job->encoder_level);
            return -1;
        }
        else if (pv->qsv_info->capabilities & HB_QSV_CAP_MSDK_API_1_6)
        {
            pv->param.videoParam->mfx.CodecLevel = HB_QSV_CLIP3(MFX_LEVEL_AVC_1,
                                                                MFX_LEVEL_AVC_52,
                                                                hb_h264_level_values[i]);
        }
        else
        {
            // Media SDK API < 1.6, MFX_LEVEL_AVC_52 unsupported
            pv->param.videoParam->mfx.CodecLevel = HB_QSV_CLIP3(MFX_LEVEL_AVC_1,
                                                                MFX_LEVEL_AVC_51,
                                                                hb_h264_level_values[i]);
        }
    }

    // interlaced encoding is not always possible
    if (pv->param.videoParam->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE)
    {
        if (pv->param.videoParam->mfx.CodecProfile == MFX_PROFILE_AVC_CONSTRAINED_BASELINE ||
            pv->param.videoParam->mfx.CodecProfile == MFX_PROFILE_AVC_BASELINE             ||
            pv->param.videoParam->mfx.CodecProfile == MFX_PROFILE_AVC_PROGRESSIVE_HIGH)
        {
            hb_error("encqsvInit: profile %s doesn't support interlaced encoding",
                     qsv_h264_profile_xlat(pv->param.videoParam->mfx.CodecProfile));
            return -1;
        }
        if ((pv->param.videoParam->mfx.CodecLevel >= MFX_LEVEL_AVC_1b &&
             pv->param.videoParam->mfx.CodecLevel <= MFX_LEVEL_AVC_2) ||
            (pv->param.videoParam->mfx.CodecLevel >= MFX_LEVEL_AVC_42))
        {
            hb_error("encqsvInit: level %s doesn't support interlaced encoding",
                     qsv_h264_level_xlat(pv->param.videoParam->mfx.CodecLevel));
            return -1;
        }
    }

    // sanitize ICQ
    if (!(pv->qsv_info->capabilities & HB_QSV_CAP_RATECONTROL_ICQ))
    {
        // ICQ not supported
        pv->param.rc.icq = 0;
    }
    else
    {
        pv->param.rc.icq = !!pv->param.rc.icq;
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
        pv->param.rc.lookahead = !!pv->param.rc.lookahead;
    }

    // set VBV here (this will be overridden for CQP and ignored for LA)
    // only set BufferSizeInKB, InitialDelayInKB and MaxKbps if we have
    // them - otheriwse Media SDK will pick values for us automatically
    if (pv->param.rc.vbv_buffer_size > 0)
    {
        if (pv->param.rc.vbv_buffer_init > 1.0)
        {
            pv->param.videoParam->mfx.InitialDelayInKB = (pv->param.rc.vbv_buffer_init / 8);
        }
        else if (pv->param.rc.vbv_buffer_init > 0.0)
        {
            pv->param.videoParam->mfx.InitialDelayInKB = (pv->param.rc.vbv_buffer_size *
                                                          pv->param.rc.vbv_buffer_init / 8);
        }
        pv->param.videoParam->mfx.BufferSizeInKB = (pv->param.rc.vbv_buffer_size / 8);
    }
    if (pv->param.rc.vbv_max_bitrate > 0)
    {
        pv->param.videoParam->mfx.MaxKbps = pv->param.rc.vbv_max_bitrate;
    }

    // set rate control paremeters
    if (job->vquality >= 0)
    {
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
            pv->param.videoParam->mfx.ICQQuality = HB_QSV_CLIP3(1, 51, job->vquality);
        }
        else
        {
            // introduced in API 1.1
            pv->param.videoParam->mfx.RateControlMethod = MFX_RATECONTROL_CQP;
            pv->param.videoParam->mfx.QPI = HB_QSV_CLIP3(0, 51, job->vquality + pv->param.rc.cqp_offsets[0]);
            pv->param.videoParam->mfx.QPP = HB_QSV_CLIP3(0, 51, job->vquality + pv->param.rc.cqp_offsets[1]);
            pv->param.videoParam->mfx.QPB = HB_QSV_CLIP3(0, 51, job->vquality + pv->param.rc.cqp_offsets[2]);
            // CQP + ExtBRC can cause bad output
            pv->param.codingOption2.ExtBRC = MFX_CODINGOPTION_OFF;
        }
    }
    else if (job->vbitrate > 0)
    {
        if (pv->param.rc.lookahead)
        {
            // introduced in API 1.7
            pv->param.videoParam->mfx.RateControlMethod = MFX_RATECONTROL_LA;
            pv->param.videoParam->mfx.TargetKbps        = job->vbitrate;
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
            pv->param.videoParam->mfx.TargetKbps = job->vbitrate;
        }
    }
    else
    {
        hb_error("encqsvInit: invalid rate control (%d, %d)",
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

    // set B-pyramid
    if (pv->param.gop.b_pyramid < 0)
    {
        if (pv->param.videoParam->mfx.RateControlMethod == MFX_RATECONTROL_CQP)
        {
            pv->param.gop.b_pyramid = 1;
        }
        else
        {
            pv->param.gop.b_pyramid = 0;
        }
    }
    pv->param.gop.b_pyramid = !!pv->param.gop.b_pyramid;

    // set the GOP structure
    if (pv->param.gop.gop_ref_dist < 0)
    {
        if (pv->param.videoParam->mfx.RateControlMethod == MFX_RATECONTROL_CQP)
        {
            pv->param.gop.gop_ref_dist = 4;
        }
        else
        {
            pv->param.gop.gop_ref_dist = 3;
        }
    }
    pv->param.videoParam->mfx.GopRefDist = pv->param.gop.gop_ref_dist;

    // set the keyframe interval
    if (pv->param.gop.gop_pic_size < 0)
    {
        int rate = (int)((double)job->vrate / (double)job->vrate_base + 0.5);
        if (pv->param.videoParam->mfx.RateControlMethod == MFX_RATECONTROL_CQP)
        {
            // ensure B-pyramid is enabled for CQP on Haswell
            pv->param.gop.gop_pic_size = 32;
        }
        else
        {
            // set the keyframe interval based on the framerate
            pv->param.gop.gop_pic_size = rate;
        }
    }
    pv->param.videoParam->mfx.GopPicSize = pv->param.gop.gop_pic_size;

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
                                                       pv->param.rc.lookahead ? 60 : 0);
    }

    if ((pv->qsv_info->capabilities & HB_QSV_CAP_B_REF_PYRAMID) &&
        (pv->param.videoParam->mfx.CodecProfile != MFX_PROFILE_AVC_BASELINE         &&
         pv->param.videoParam->mfx.CodecProfile != MFX_PROFILE_AVC_CONSTRAINED_HIGH &&
         pv->param.videoParam->mfx.CodecProfile != MFX_PROFILE_AVC_CONSTRAINED_BASELINE))
    {
        int gop_ref_dist = 4;
        /*
         * B-pyramid is supported.
         *
         * Set gop_ref_dist to a power of two, >= 4 and <= GopRefDist to ensure
         * Media SDK will not disable B-pyramid if we end up using it below.
         */
        while (pv->param.videoParam->mfx.GopRefDist >= gop_ref_dist * 2)
        {
            gop_ref_dist *= 2;
        }
        /*
         * XXX: B-pyramid + forced keyframes will cause visual artifacts,
         *      force-disable B-pyramid until we insert keyframes properly
         */
        if (pv->param.gop.b_pyramid && job->chapter_markers)
        {
            pv->param.gop.b_pyramid = 0;
            hb_log("encqsvInit: chapter markers enabled, disabling B-pyramid "
                   "to work around a bug in our keyframe insertion code");
        }
        if ((pv->param.gop.b_pyramid) &&
            (pv->param.videoParam->mfx.GopPicSize == 0 ||
             pv->param.videoParam->mfx.GopPicSize > gop_ref_dist))
        {
            /*
             * B-pyramid enabled and GopPicSize is long enough for gop_ref_dist.
             *
             * Use gop_ref_dist. GopPicSize must be a multiple of GopRefDist.
             * NumRefFrame should be >= (GopRefDist / 2) and >= 3, otherwise
             * Media SDK may sometimes decide to disable B-pyramid too (whereas
             * sometimes it will just sanitize NumrefFrame instead).
             *
             * Notes: Media SDK handles the NumRefFrame == 0 case for us.
             *        Also, GopPicSize == 0 should always result in a value that
             *        does NOT cause Media SDK to disable B-pyramid, so it's OK.
             */
            pv->param.videoParam->mfx.GopRefDist = gop_ref_dist;
            pv->param.videoParam->mfx.GopPicSize = (pv->param.videoParam->mfx.GopPicSize /
                                                    pv->param.videoParam->mfx.GopRefDist *
                                                    pv->param.videoParam->mfx.GopRefDist);
            if (pv->param.videoParam->mfx.NumRefFrame)
            {
                pv->param.videoParam->mfx.NumRefFrame = FFMAX(pv->param.videoParam->mfx.NumRefFrame,
                                                              pv->param.videoParam->mfx.GopRefDist / 2);
                pv->param.videoParam->mfx.NumRefFrame = FFMAX(pv->param.videoParam->mfx.NumRefFrame, 3);
            }
        }
        else
        {
            /*
             * B-pyramid disabled or not possible (GopPicSize too short).
             * Sanitize gop.b_pyramid to 0 (off/disabled).
             */
            pv->param.gop.b_pyramid = 0;
            /* Then, adjust settings to actually disable it. */
            if (pv->param.videoParam->mfx.GopRefDist == 0)
            {
                /*
                 * GopRefDist == 0 means the value will be set by Media SDK.
                 * Since we can't be sure what the actual value would be, we
                 * have to make sure that GopRefDist is set explicitly.
                 */
                pv->param.videoParam->mfx.GopRefDist = gop_ref_dist - 1;
            }
            else if (pv->param.videoParam->mfx.GopRefDist == gop_ref_dist)
            {
                /* GopRefDist is compatible with Media SDK's B-pyramid. */
                if (pv->param.videoParam->mfx.GopPicSize == 0)
                {
                    /*
                     * GopPicSize is unknown and could be a multiple of
                     * GopRefDist. Decrement the latter to disable B-pyramid.
                     */
                    pv->param.videoParam->mfx.GopRefDist--;
                }
                else if (pv->param.videoParam->mfx.GopPicSize %
                         pv->param.videoParam->mfx.GopRefDist == 0)
                {
                    /*
                     * GopPicSize is a multiple of GopRefDist.
                     * Increment the former to disable B-pyramid.
                     */
                    pv->param.videoParam->mfx.GopPicSize++;
                }
            }
        }
    }
    else
    {
        /* B-pyramid not supported. */
        pv->param.gop.b_pyramid = 0;
    }

    /*
     * init a dummy encode-only session to get the SPS/PPS
     * and the final output settings sanitized by Media SDK
     * this is fine since the actual encode will use the same
     * values for all parameters relevant to the H.264 bitstream
     */
    mfxStatus err;
    mfxVersion version;
    mfxVideoParam videoParam;
    mfxExtBuffer *extParamArray[3];
    mfxSession session = (mfxSession)0;
    mfxExtCodingOption  option1_buf, *option1 = &option1_buf;
    mfxExtCodingOption2 option2_buf, *option2 = &option2_buf;
    mfxExtCodingOptionSPSPPS sps_pps_buf, *sps_pps = &sps_pps_buf;
    version.Major = HB_QSV_MINVERSION_MAJOR;
    version.Minor = HB_QSV_MINVERSION_MINOR;
    err = MFXInit(pv->qsv_info->implementation, &version, &session);
    if (err != MFX_ERR_NONE)
    {
        hb_error("encqsvInit: MFXInit failed (%d)", err);
        return -1;
    }
    err = MFXVideoENCODE_Init(session, pv->param.videoParam);
// workaround for the early 15.33.x driver, should be removed later
#define HB_DRIVER_FIX_33
#ifdef  HB_DRIVER_FIX_33
    int la_workaround = 0;
    if (err < MFX_ERR_NONE &&
        pv->param.videoParam->mfx.RateControlMethod == MFX_RATECONTROL_LA)
    {
        pv->param.videoParam->mfx.RateControlMethod = MFX_RATECONTROL_CBR;
        err = MFXVideoENCODE_Init(session, pv->param.videoParam);
        la_workaround = 1;
    }
#endif
    if (err < MFX_ERR_NONE) // ignore warnings
    {
        hb_error("encqsvInit: MFXVideoENCODE_Init failed (%d)", err);
        MFXClose(session);
        return -1;
    }
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
    videoParam.ExtParam[videoParam.NumExtParam++] = (mfxExtBuffer*)sps_pps;
    // introduced in API 1.0
    memset(option1, 0, sizeof(mfxExtCodingOption));
    option1->Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
    option1->Header.BufferSz = sizeof(mfxExtCodingOption);
    videoParam.ExtParam[videoParam.NumExtParam++] = (mfxExtBuffer*)option1;
    // introduced in API 1.6
    memset(option2, 0, sizeof(mfxExtCodingOption2));
    option2->Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
    option2->Header.BufferSz = sizeof(mfxExtCodingOption2);
    if (pv->qsv_info->capabilities & HB_QSV_CAP_MSDK_API_1_6)
    {
        // attach to get the final output mfxExtCodingOption2 settings
        videoParam.ExtParam[videoParam.NumExtParam++] = (mfxExtBuffer*)option2;
    }
    err = MFXVideoENCODE_GetVideoParam(session, &videoParam);
    MFXVideoENCODE_Close(session);
    if (err == MFX_ERR_NONE)
    {
        // remove 32-bit NAL prefix (0x00 0x00 0x00 0x01)
        w->config->h264.sps_length = sps_pps->SPSBufSize - 4;
        memmove(w->config->h264.sps, w->config->h264.sps + 4,
                w->config->h264.sps_length);
        w->config->h264.pps_length = sps_pps->PPSBufSize - 4;
        memmove(w->config->h264.pps, w->config->h264.pps + 4,
                w->config->h264.pps_length);
    }
    else
    {
        hb_error("encqsvInit: MFXVideoENCODE_GetVideoParam failed (%d)", err);
        MFXClose(session);
        return -1;
    }

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

    /* check whether B-frames are used */
    if (videoParam.mfx.GopRefDist > 1 && videoParam.mfx.GopPicSize > 2)
    {
        /* the muxer needs to know to the init_delay */
        switch (pv->qsv_info->codec_id)
        {
            case MFX_CODEC_AVC:
                pv->init_delay = &w->config->h264.init_delay;
                break;
            default: // unreachable
                break;
        }

        /* let the muxer know that it should expect B-frames */
        job->areBframes = 1;

        /* holds the PTS sequence in display order, used to generate DTS */
        pv->list_dts = hb_list_init();
    }

    // log code path and main output settings
    hb_log("encqsvInit: using %s path",
           pv->is_sys_mem ? "encode-only" : "full QSV");
    hb_log("encqsvInit: TargetUsage %"PRIu16" AsyncDepth %"PRIu16"",
           videoParam.mfx.TargetUsage, videoParam.AsyncDepth);
    hb_log("encqsvInit: GopRefDist %"PRIu16" GopPicSize %"PRIu16" NumRefFrame %"PRIu16"",
           videoParam.mfx.GopRefDist, videoParam.mfx.GopPicSize, videoParam.mfx.NumRefFrame);
    if (pv->qsv_info->capabilities & HB_QSV_CAP_B_REF_PYRAMID)
    {
        hb_log("encqsvInit: BFrames %s BPyramid %s",
               pv->bfrm_delay                            ? "on" : "off",
               pv->bfrm_delay && pv->param.gop.b_pyramid ? "on" : "off");
    }
    else
    {
        hb_log("encqsvInit: BFrames %s", pv->bfrm_delay ? "on" : "off");
    }
    if (pv->qsv_info->capabilities & HB_QSV_CAP_OPTION2_IB_ADAPT)
    {
        if (pv->bfrm_delay > 0)
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
    if (videoParam.mfx.RateControlMethod == MFX_RATECONTROL_CQP)
    {
        char qpi[7], qpp[9], qpb[9];
        snprintf(qpi, sizeof(qpi),  "QPI %"PRIu16"", videoParam.mfx.QPI);
        snprintf(qpp, sizeof(qpp), " QPP %"PRIu16"", videoParam.mfx.QPP);
        snprintf(qpb, sizeof(qpb), " QPB %"PRIu16"", videoParam.mfx.QPB);
        hb_log("encqsvInit: RateControlMethod CQP with %s%s%s", qpi,
               videoParam.mfx.GopPicSize > 1 ? qpp : "",
               videoParam.mfx.GopRefDist > 1 ? qpb : "");
    }
    else
    {
        switch (videoParam.mfx.RateControlMethod)
        {
            case MFX_RATECONTROL_LA:
                hb_log("encqsvInit: RateControlMethod LA TargetKbps %"PRIu16" LookAheadDepth %"PRIu16"",
                       videoParam.mfx.TargetKbps, option2->LookAheadDepth);
                break;
            case MFX_RATECONTROL_LA_ICQ:
                hb_log("encqsvInit: RateControlMethod LA_ICQ ICQQuality %"PRIu16" LookAheadDepth %"PRIu16"",
                       videoParam.mfx.ICQQuality, option2->LookAheadDepth);
                break;
            case MFX_RATECONTROL_ICQ:
                hb_log("encqsvInit: RateControlMethod ICQ ICQQuality %"PRIu16"",
                       videoParam.mfx.ICQQuality);
                break;
            case MFX_RATECONTROL_CBR:
            case MFX_RATECONTROL_VBR:
                hb_log("encqsvInit: RateControlMethod %s TargetKbps %"PRIu16" MaxKbps %"PRIu16" BufferSizeInKB %"PRIu16" InitialDelayInKB %"PRIu16"",
                       videoParam.mfx.RateControlMethod == MFX_RATECONTROL_CBR ? "CBR" : "VBR",
                       videoParam.mfx.TargetKbps,     videoParam.mfx.MaxKbps,
                       videoParam.mfx.BufferSizeInKB, videoParam.mfx.InitialDelayInKB);
                break;
            default:
                hb_log("encqsvInit: invalid rate control method %"PRIu16"",
                       videoParam.mfx.RateControlMethod);
                return -1;
        }
    }
    if ((pv->qsv_info->capabilities & HB_QSV_CAP_OPTION2_LA_DOWNS) &&
        (videoParam.mfx.RateControlMethod == MFX_RATECONTROL_LA ||
         videoParam.mfx.RateControlMethod == MFX_RATECONTROL_LA_ICQ))
    {
        switch (option2->LookAheadDS)
        {
            case MFX_LOOKAHEAD_DS_UNKNOWN:
                hb_log("encqsvInit: LookAheadDS unknown (auto)");
                break;
            case MFX_LOOKAHEAD_DS_OFF:
                hb_log("encqsvInit: LookAheadDS off");
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
    switch (videoParam.mfx.FrameInfo.PicStruct)
    {
        // quiet, most people don't care
        case MFX_PICSTRUCT_PROGRESSIVE:
            break;
        // interlaced encoding is intended for advanced users only, who do care
        case MFX_PICSTRUCT_FIELD_TFF:
            hb_log("encqsvInit: PicStruct top field first");
            break;
        case MFX_PICSTRUCT_FIELD_BFF:
            hb_log("encqsvInit: PicStruct bottom field first");
            break;
        default:
            hb_error("encqsvInit: invalid PicStruct value 0x%"PRIx16"",
                     videoParam.mfx.FrameInfo.PicStruct);
            return -1;
    }
    hb_log("encqsvInit: CAVLC %s",
           hb_qsv_codingoption_get_name(option1->CAVLC));
    if (pv->param.rc.lookahead           == 0 &&
        videoParam.mfx.RateControlMethod != MFX_RATECONTROL_CQP)
    {
        // LA/CQP and ExtBRC/MBBRC are mutually exclusive
        if (pv->qsv_info->capabilities & HB_QSV_CAP_OPTION2_EXTBRC)
        {
            hb_log("encqsvInit: ExtBRC %s",
                   hb_qsv_codingoption_get_name(option2->ExtBRC));
        }
        if (pv->qsv_info->capabilities & HB_QSV_CAP_OPTION2_MBBRC)
        {
            hb_log("encqsvInit: MBBRC %s",
                   hb_qsv_codingoption_get_name(option2->MBBRC));
        }
    }
    if (pv->qsv_info->capabilities & HB_QSV_CAP_OPTION2_TRELLIS)
    {
        switch (option2->Trellis)
        {
            case MFX_TRELLIS_OFF:
                hb_log("encqsvInit: Trellis off");
                break;
            case MFX_TRELLIS_UNKNOWN:
                hb_log("encqsvInit: Trellis unknown (auto)");
                break;
            default:
                hb_log("encqsvInit: Trellis on (%s%s%s)",
                       (option2->Trellis & MFX_TRELLIS_I) ? "I" : "",
                       (option2->Trellis & MFX_TRELLIS_P) &&
                       (videoParam.mfx.GopPicSize > 1)    ? "P" : "",
                       (option2->Trellis & MFX_TRELLIS_B) &&
                       (videoParam.mfx.GopRefDist > 1)    ? "B" : "");
                break;
        }
    }
    hb_log("encqsvInit: H.264 profile %s @ level %s",
           qsv_h264_profile_xlat(videoParam.mfx.CodecProfile),
           qsv_h264_level_xlat  (videoParam.mfx.CodecLevel));

    // AsyncDepth has now been set and/or modified by Media SDK
    pv->max_async_depth = videoParam.AsyncDepth;
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

        av_qsv_context *qsv_ctx       = pv->job->qsv.ctx;
        av_qsv_space   *qsv_enc_space = pv->job->qsv.ctx->enc_space;

        if (qsv_enc_space != NULL)
        {
            if (qsv_enc_space->is_init_done)
            {
                for (i = av_qsv_list_count(qsv_enc_space->tasks); i > 1; i--)
                {
                    av_qsv_task *task = av_qsv_list_item(qsv_enc_space->tasks,
                                                         i - 1);
                    if (task != NULL)
                    {
                        if (task->bs != NULL)
                        {
                            av_freep(&task->bs->Data);
                        }
                        av_qsv_list_rem(qsv_enc_space->tasks, task);
                        av_freep(&task->bs);
                        av_freep(&task);
                    }
                }
                av_qsv_list_close(&qsv_enc_space->tasks);

                for (i = 0; i < qsv_enc_space->surface_num; i++)
                {
                    if (pv->is_sys_mem)
                    {
                        av_freep(&qsv_enc_space->p_surfaces[i]->Data.VU);
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

        if (qsv_ctx != NULL)
        {
            /* QSV context cleanup and MFXClose */
            av_qsv_context_clean(qsv_ctx);

            if (pv->is_sys_mem)
            {
                av_freep(&qsv_ctx);
            }
        }
    }

    if (pv != NULL)
    {
        if (pv->delayed_processing != NULL)
        {
            /* the list is already empty */
            hb_list_close(&pv->delayed_processing);
        }
        if (pv->sws_context_to_nv12 != NULL)
        {
            sws_freeContext(pv->sws_context_to_nv12);
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
        if (pv->delayed_chapters != NULL)
        {
            struct chapter_s *item;
            while ((item = hb_list_item(pv->delayed_chapters, 0)) != NULL)
            {
                hb_list_rem(pv->delayed_chapters, item);
                free(item);
            }
            hb_list_close(&pv->delayed_chapters);
        }
        if (pv->encoded_frames != NULL)
        {
            hb_buffer_t *item;
            while ((item = hb_list_item(pv->encoded_frames, 0)) != NULL)
            {
                hb_list_rem(pv->encoded_frames, item);
                hb_buffer_close(&item);
            }
            hb_list_close(&pv->encoded_frames);
        }
    }

    free(pv);
    w->private_data = NULL;
}

static void save_chapter(hb_work_private_t *pv, hb_buffer_t *buf)
{
    /*
     * Since there may be several frames buffered in the encoder, remember the
     * timestamp so when this frame finally pops out of the encoder we'll mark
     * its buffer as the start of a chapter.
     */
    if (pv->next_chapter_pts == AV_NOPTS_VALUE)
    {
        pv->next_chapter_pts = buf->s.start;
    }

    /*
     * Chapter markers are sometimes so close we can get a new
     * one before the previous goes through the encoding queue.
     *
     * Dropping markers can cause weird side-effects downstream,
     * including but not limited to missing chapters in the
     * output, so we need to save it somehow.
     */
    struct chapter_s *item = malloc(sizeof(struct chapter_s));

    if (item != NULL)
    {
        item->start = buf->s.start;
        item->index = buf->s.new_chap;
        hb_list_add(pv->delayed_chapters, item);
    }

    /* don't let 'work_loop' put a chapter mark on the wrong buffer */
    buf->s.new_chap = 0;
}

static void restore_chapter(hb_work_private_t *pv, hb_buffer_t *buf)
{
    /* we're no longer looking for this chapter */
    pv->next_chapter_pts = AV_NOPTS_VALUE;

    /* get the chapter index from the list */
    struct chapter_s *item = hb_list_item(pv->delayed_chapters, 0);

    if (item != NULL)
    {
        /* we're done with this chapter */
        hb_list_rem(pv->delayed_chapters, item);
        buf->s.new_chap = item->index;
        free(item);

        /* we may still have another pending chapter */
        item = hb_list_item(pv->delayed_chapters, 0);

        if (item != NULL)
        {
            /*
             * we're looking for this chapter now
             * we still need it, don't remove it
             */
            pv->next_chapter_pts = item->start;
        }
    }
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

        if (pv->job->cfr != 1)
        {
            /* variable frame rate video, so we need to generate our own DTS */
            pv->bfrm_workaround = 1;

            /*
             * we also need to know the delay in frames to generate DTS.
             *
             * compute it based on the init_delay and average frame duration,
             * and account for potential rounding errors due to the timebase.
             */
            double avg_frame_dur = ((double)pv->job->vrate_base /
                                    (double)pv->job->vrate * 90000.);

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
                        pv->bfrm_delay = videoParam.mfx.GopRefDist - 1;
                    }
                }

                pv->bfrm_delay = FFMIN(BFRM_DELAY_MAX, pv->bfrm_delay);
            }

            pv->init_delay[0] = pv->init_pts[pv->bfrm_delay] - pv->init_pts[0];
        }
        else
        {
            pv->init_delay[0] = init_delay;
        }
    }
    else
    {
        /*
         * we can't get the DTS from MSDK, so we need to generate our own.
         *
         * B-pyramid not possible here, so the delay in frames is always 1.
         */
        pv->bfrm_delay    = pv->bfrm_workaround = 1;
        pv->init_delay[0] = pv->init_pts[1] - pv->init_pts[0];
    }

    /* The delay only needs to be set once. */
    pv->init_delay = NULL;
}

static int qsv_frame_is_key(mfxU16 FrameType)
{
    return ((FrameType  & MFX_FRAMETYPE_IDR) ||
            (FrameType == MFX_FRAMETYPE_UNKNOWN));
}

static void qsv_bitstream_slurp(hb_work_private_t *pv, mfxBitstream *bs)
{
    /* allocate additional data for parse_nalus */
    hb_buffer_t *buf = hb_buffer_init(bs->DataLength * 2);
    if (buf == NULL)
    {
        hb_error("encqsv: hb_buffer_init failed");
        goto fail;
    }
    buf->size = 0;

    /*
     * we need to convert the encoder's Annex B output
     * to an MP4-compatible format (ISO/IEC 14496-15).
     */
    parse_nalus(bs->Data + bs->DataOffset, bs->DataLength, buf);
    bs->DataLength = bs->DataOffset = 0;
    bs->MaxLength  = pv->job->qsv.ctx->enc_space->p_buf_max_size;

    buf->s.frametype = hb_qsv_frametype_xlat(bs->FrameType, &buf->s.flags);
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

        /* if the DTS provided by MSDK is trustworthy, use it */
        if (!pv->bfrm_workaround)
        {
            buf->s.renderOffset = bs->DecodeTimeStamp;
        }
    }

    /* check if B-pyramid is used even though it's disabled */
    if ((pv->param.gop.b_pyramid == 0)    &&
        (bs->FrameType & MFX_FRAMETYPE_B) &&
        (bs->FrameType & MFX_FRAMETYPE_REF))
    {
        hb_log("encqsv: BPyramid off not respected (delay: %d)",
               pv->bfrm_delay);

        /* don't pollute the log unnecessarily */
        pv->param.gop.b_pyramid = 1;
    }

    /* check for PTS < DTS */
    if (buf->s.start < buf->s.renderOffset)
    {
        hb_log("encqsv: PTS %"PRId64" < DTS %"PRId64" for "
               "frame %d with type '%s' (bfrm_workaround: %d)",
               buf->s.start, buf->s.renderOffset, pv->frames_out + 1,
               hb_qsv_frametype_name(bs->FrameType), pv->bfrm_workaround);
    }

    /*
     * If we have a chapter marker pending and this frame's PTS
     * is at or after the marker's PTS, use it as the chapter start.
     */
    if (pv->next_chapter_pts != AV_NOPTS_VALUE &&
        pv->next_chapter_pts <= buf->s.start   &&
        qsv_frame_is_key(bs->FrameType))
    {
        restore_chapter(pv, buf);
    }

    hb_list_add(pv->encoded_frames, buf);
    pv->frames_out++;
    return;

fail:
    *pv->job->done_error = HB_ERROR_UNKNOWN;
    *pv->job->die        = 1;
}

static int qsv_enc_work(hb_work_private_t *pv, av_qsv_list *qsv_atom,
                        mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface)
{
    mfxStatus sts;
    av_qsv_context *qsv_ctx       = pv->job->qsv.ctx;
    av_qsv_space   *qsv_enc_space = pv->job->qsv.ctx->enc_space;

    do
    {
        int sync_idx = av_qsv_get_free_sync(qsv_enc_space, qsv_ctx);
        if (sync_idx == -1)
        {
            hb_error("encqsv: av_qsv_get_free_sync failed");
            return -1;
        }
        av_qsv_task *task = av_qsv_list_item(qsv_enc_space->tasks,
                                             pv->async_depth);

        do
        {
            sts = MFXVideoENCODE_EncodeFrameAsync(qsv_ctx->mfx_session,
                                                  ctrl, surface, task->bs,
                                                  qsv_enc_space->p_syncp[sync_idx]->p_sync);

            if (sts == MFX_ERR_MORE_DATA || (sts >= MFX_ERR_NONE &&
                                             sts != MFX_WRN_DEVICE_BUSY))
            {
                if (surface != NULL && !pv->is_sys_mem)
                {
                    ff_qsv_atomic_dec(&surface->Data.Locked);
                }
            }

            if (sts == MFX_ERR_MORE_DATA)
            {
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
                av_qsv_sleep(10); // device is busy, wait then repeat the call
                continue;
            }
            else
            {
                av_qsv_stage *new_stage = av_qsv_stage_init();
                new_stage->type         = AV_QSV_ENCODE;
                new_stage->in.p_surface = surface;
                new_stage->out.sync     = qsv_enc_space->p_syncp[sync_idx];
                new_stage->out.p_bs     = task->bs;
                task->stage             = new_stage;
                pv->async_depth++;

                if (qsv_atom != NULL)
                {
                    av_qsv_add_stagee(&qsv_atom, new_stage, HAVE_THREADS);
                }
                else
                {
                    /* encode-only or flushing */
                    av_qsv_list *new_qsv_atom = av_qsv_list_init(HAVE_THREADS);
                    av_qsv_add_stagee(&new_qsv_atom,  new_stage, HAVE_THREADS);
                    av_qsv_list_add  (qsv_ctx->pipes, new_qsv_atom);
                }

                int i = hb_list_count(pv->delayed_processing);
                while (--i >= 0)
                {
                    av_qsv_list *item = hb_list_item(pv->delayed_processing, i);

                    if (item != NULL)
                    {
                        hb_list_rem(pv->delayed_processing,  item);
                        av_qsv_flush_stages(qsv_ctx->pipes, &item);
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
                av_qsv_task *task = av_qsv_list_item(qsv_enc_space->tasks, 0);
                pv->async_depth--;

                /* perform a sync operation to get the output bitstream */
                av_qsv_wait_on_sync(qsv_ctx, task->stage);

                if (task->bs->DataLength > 0)
                {
                    av_qsv_list *pipe = av_qsv_pipe_by_stage(qsv_ctx->pipes,
                                                             task->stage);
                    av_qsv_flush_stages(qsv_ctx->pipes, &pipe);

                    /* get the encoded frame from the bitstream */
                    qsv_bitstream_slurp(pv, task->bs);

                    /* shift for fifo */
                    if (pv->async_depth)
                    {
                        av_qsv_list_rem(qsv_enc_space->tasks, task);
                        av_qsv_list_add(qsv_enc_space->tasks, task);
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

static hb_buffer_t* link_buffer_list(hb_list_t *list)
{
    hb_buffer_t *buf, *prev = NULL, *out = NULL;

    while ((buf = hb_list_item(list, 0)) != NULL)
    {
        hb_list_rem(list, buf);

        if (prev == NULL)
        {
            prev = out = buf;
        }
        else
        {
            prev->next = buf;
            prev       = buf;
        }
    }

    return out;
}

int encqsvWork(hb_work_object_t *w, hb_buffer_t **buf_in, hb_buffer_t **buf_out)
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *in       = *buf_in;
    hb_job_t *job         = pv->job;

    while (qsv_enc_init(pv) >= 2)
    {
        av_qsv_sleep(1); // encoding not initialized, wait and repeat the call
    }

    if (*job->die)
    {
        goto fail; // unrecoverable error, don't attempt to encode
    }

    /*
     * EOF on input. Flush the decoder, then send the
     * EOF downstream to let the muxer know we're done.
     */
    if (in->size <= 0)
    {
        qsv_enc_work(pv, NULL, NULL, NULL);
        hb_list_add(pv->encoded_frames, in);
        *buf_out = link_buffer_list(pv->encoded_frames);
        *buf_in = NULL; // don't let 'work_loop' close this buffer
        return HB_WORK_DONE;
    }

    mfxEncodeCtrl    *ctrl          = NULL;
    mfxFrameSurface1 *surface       = NULL;
    av_qsv_list      *qsv_atom      = NULL;
    av_qsv_context   *qsv_ctx       = job->qsv.ctx;
    av_qsv_space     *qsv_enc_space = job->qsv.ctx->enc_space;

    if (pv->is_sys_mem)
    {
        mfxFrameInfo *info = &pv->param.videoParam->mfx.FrameInfo;
        int surface_index  = av_qsv_get_free_surface(qsv_enc_space, qsv_ctx, info,
                                                     QSV_PART_ANY);
        if (surface_index == -1)
        {
            hb_error("encqsv: av_qsv_get_free_surface failed");
            goto fail;
        }

        surface = qsv_enc_space->p_surfaces[surface_index];
        qsv_yuv420_to_nv12(pv->sws_context_to_nv12, surface, in);
    }
    else
    {
        qsv_atom = in->qsv_details.qsv_atom;
        surface  = av_qsv_get_last_stage(qsv_atom)->out.p_surface;

        /*
         * QSV decoding fills the QSV context's dts_seq list, we need to
         * pop this surface's DTS so dts_seq doesn't grow unnecessarily.
         */
        av_qsv_dts_pop(qsv_ctx);
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

    /* for DTS generation (when MSDK API < 1.6 or VFR) */
    if (pv->frames_in <= BFRM_DELAY_MAX)
    {
        pv->init_pts[pv->frames_in] = in->s.start;
    }
    if (pv->frames_in)
    {
        hb_qsv_add_new_dts(pv->list_dts, in->s.start);
    }
    pv->frames_in++;

    if (in->s.new_chap > 0 && job->chapter_markers)
    {
        save_chapter(pv, in);

        /* Chapters have to start with a keyframe, so request an IDR */
        ctrl = &pv->force_keyframe;
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
    if (qsv_enc_work(pv, qsv_atom, ctrl, surface) < 0)
    {
        goto fail;
    }

    *buf_out = link_buffer_list(pv->encoded_frames);
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

int nal_find_start_code(uint8_t **pb, size_t *size)
{
    if ((int)*size < 4)
    {
        return 0;
    }

    // find start code by MSDK, see ff_prefix_code[]
    while ((4 <= *size) && ((*pb)[0] || (*pb)[1] || (*pb)[2] != 1))
    {
        *pb   += 1;
        *size -= 1;
    }

    if (4 <= *size)
    {
        return (((*pb)[0] << 24) |
                ((*pb)[1] << 16) |
                ((*pb)[2] <<  8) |
                ((*pb)[3]));
    }

    return 0;
}

void parse_nalus(uint8_t *nal_inits, size_t length, hb_buffer_t *buf)
{
    uint8_t *offset = nal_inits;
    size_t   size   = length;

    if (!nal_find_start_code(&offset, &size))
    {
        size = 0;
    }

    while (size > 0)
    {
        uint8_t *current_nal  = offset + sizeof(ff_prefix_code) - 1;
        size_t   current_size = size   - sizeof(ff_prefix_code);
        uint8_t *next_offset  = current_nal + 1;
        size_t   next_size    = current_size;

        if (!nal_find_start_code(&next_offset, &next_size))
        {
            size          = 0;
            current_size += 1;
        }
        else
        {
            if (next_offset > 0 && *(next_offset - 1))
            {
                current_size += 1;
            }
            current_size -= next_size;
        }

        char size_position[4];
        size_position[1] = (current_size >> 24) & 0xFF;
        size_position[1] = (current_size >> 16) & 0xFF;
        size_position[2] = (current_size >>  8) & 0xFF;
        size_position[3] = (current_size      ) & 0xFF;

        memcpy(buf->data + buf->size, &size_position, sizeof(size_position));
        buf->size += sizeof(size_position);

        memcpy(buf->data + buf->size, current_nal, current_size);
        buf->size += current_size;

        if (size)
        {
            size   = next_size;
            offset = next_offset;
        }
    }
}

#endif // USE_QSV
