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

#include "hb.h"
#include "hbffmpeg.h"
#include "libavcodec/qsv.h"
#include "qsv_filter.h"
#include "enc_qsv.h"

struct hb_filter_private_s
{
    hb_job_t            *job;
    hb_list_t           *list;

    int                 width_in;
    int                 height_in;
    int                 pix_fmt;
    int                 pix_fmt_out;
    int                 width_out;
    int                 height_out;
    int                 crop[4];
    int                 deinterlace;
    int                 is_frc_used;

    av_qsv_space           *vpp_space;

    // FRC param(s)
    mfxExtVPPFrameRateConversion    frc_config;
};

static int hb_qsv_filter_init( hb_filter_object_t * filter,
                               hb_filter_init_t * init );

static int hb_qsv_filter_work( hb_filter_object_t * filter,
                               hb_buffer_t ** buf_in,
                               hb_buffer_t ** buf_out );

static int hb_qsv_filter_info( hb_filter_object_t * filter,
                               hb_filter_info_t * info );

static void hb_qsv_filter_close( hb_filter_object_t * filter );

hb_filter_object_t hb_filter_qsv =
{
    .id            = HB_FILTER_QSV,
    .enforce_order = 1,
    .name          = "Quick Sync Video VPP",
    .settings      = NULL,
    .init          = hb_qsv_filter_init,
    .work          = hb_qsv_filter_work,
    .close         = hb_qsv_filter_close,
    .info          = hb_qsv_filter_info,
};

static int filter_init( av_qsv_context* qsv, hb_filter_private_t * pv ){
    mfxStatus sts;
    int i=0;

    if(!qsv) return 3;


    if(!qsv->vpp_space){
        qsv->vpp_space = av_qsv_list_init(HAVE_THREADS);
    }
    if(!pv->vpp_space){
        for(i=0; i<av_qsv_list_count(qsv->vpp_space);i++){
            av_qsv_space *qsv_vpp = av_qsv_list_item( qsv->vpp_space, i );
            if(qsv_vpp->type == AV_QSV_VPP_DEFAULT){
                pv->vpp_space = qsv_vpp;
                break;
            }
        }
    }

    if(!pv->vpp_space){
        pv->vpp_space = calloc( 1, sizeof( av_qsv_space ));
        pv->vpp_space->type = AV_QSV_VPP_DEFAULT;
        av_qsv_list_add( qsv->vpp_space, pv->vpp_space );
    }
    else
        if(pv->vpp_space->is_init_done ) return 1;

    if(!qsv->dec_space || !qsv->dec_space->is_init_done) return 2;

    // we need to know final output settings before we can properly configure
    if (!pv->job->qsv_enc_info.is_init_done)
    {
        return 2;
    }

    av_qsv_add_context_usage(qsv,HAVE_THREADS);

    // see params needed like at mediasdk-man.pdf:"Appendix A: Configuration Parameter Constraints"
    // for now - most will take from the decode
    {
        av_qsv_space *qsv_vpp = pv->vpp_space;
        AV_QSV_ZERO_MEMORY(qsv_vpp->m_mfxVideoParam);

        if (pv->deinterlace)
        {
            /*
             * Input may be progressive, interlaced or even mixed, so init with
             * MFX_PICSTRUCT_UNKNOWN and use per-frame field order information
             * (mfxFrameSurface1.Info.PicStruct)
             */
            qsv_vpp->m_mfxVideoParam.vpp.In.PicStruct  = MFX_PICSTRUCT_UNKNOWN;
            qsv_vpp->m_mfxVideoParam.vpp.Out.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        }
        else
        {
            /* Same PicStruct in/out: no filtering */
            qsv_vpp->m_mfxVideoParam.vpp.In.PicStruct  = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.PicStruct;
            qsv_vpp->m_mfxVideoParam.vpp.Out.PicStruct = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.PicStruct;
        }

        // FrameRate is important for VPP to start with
        if( qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtN == 0 &&
            qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtD == 0 ){
            qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtN = pv->job->title->rate;
            qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtD = pv->job->title->rate_base;
        }

        qsv_vpp->m_mfxVideoParam.vpp.In.FourCC          = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FourCC;
        qsv_vpp->m_mfxVideoParam.vpp.In.ChromaFormat    = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.ChromaFormat;
        qsv_vpp->m_mfxVideoParam.vpp.In.CropX           = pv->crop[2];
        qsv_vpp->m_mfxVideoParam.vpp.In.CropY           = pv->crop[0];
        qsv_vpp->m_mfxVideoParam.vpp.In.CropW           = pv-> width_in - pv->crop[3] - pv->crop[2];
        qsv_vpp->m_mfxVideoParam.vpp.In.CropH           = pv->height_in - pv->crop[1] - pv->crop[0];
        qsv_vpp->m_mfxVideoParam.vpp.In.FrameRateExtN   = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtN;
        qsv_vpp->m_mfxVideoParam.vpp.In.FrameRateExtD   = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtD;
        qsv_vpp->m_mfxVideoParam.vpp.In.AspectRatioW    = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.AspectRatioW;
        qsv_vpp->m_mfxVideoParam.vpp.In.AspectRatioH    = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.AspectRatioH;
        qsv_vpp->m_mfxVideoParam.vpp.In.Width           = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.Width;
        qsv_vpp->m_mfxVideoParam.vpp.In.Height          = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.Height;

        qsv_vpp->m_mfxVideoParam.vpp.Out.FourCC          = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FourCC;
        qsv_vpp->m_mfxVideoParam.vpp.Out.ChromaFormat    = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.ChromaFormat;
        qsv_vpp->m_mfxVideoParam.vpp.Out.CropX           = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.CropX;
        qsv_vpp->m_mfxVideoParam.vpp.Out.CropY           = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.CropY;
        qsv_vpp->m_mfxVideoParam.vpp.Out.CropW           = pv->width_out;
        qsv_vpp->m_mfxVideoParam.vpp.Out.CropH           = pv->height_out;
        qsv_vpp->m_mfxVideoParam.vpp.Out.FrameRateExtN   = pv->job->vrate;
        qsv_vpp->m_mfxVideoParam.vpp.Out.FrameRateExtD   = pv->job->vrate_base;
        qsv_vpp->m_mfxVideoParam.vpp.Out.AspectRatioW    = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.AspectRatioW;
        qsv_vpp->m_mfxVideoParam.vpp.Out.AspectRatioH    = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.AspectRatioH;
        qsv_vpp->m_mfxVideoParam.vpp.Out.Width           = pv->job->qsv_enc_info.align_width;
        qsv_vpp->m_mfxVideoParam.vpp.Out.Height          = pv->job->qsv_enc_info.align_height;

        qsv_vpp->m_mfxVideoParam.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY;

        qsv_vpp->m_mfxVideoParam.AsyncDepth = pv->job->qsv_async_depth;

        memset(&qsv_vpp->request, 0, sizeof(mfxFrameAllocRequest)*2);

        sts = MFXVideoVPP_QueryIOSurf(qsv->mfx_session, &qsv_vpp->m_mfxVideoParam, qsv_vpp->request );
        AV_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        int num_surfaces_in  = qsv_vpp->request[0].NumFrameSuggested;
        int num_surfaces_out = qsv_vpp->request[1].NumFrameSuggested;

        av_qsv_config *config = qsv->qsv_config;


        qsv_vpp->surface_num = FFMIN( num_surfaces_in + num_surfaces_out + qsv_vpp->m_mfxVideoParam.AsyncDepth + config ? config->additional_buffers/2 :0 , AV_QSV_SURFACE_NUM );
        if(qsv_vpp->surface_num <= 0 )
            qsv_vpp->surface_num = AV_QSV_SURFACE_NUM;

        int i = 0;
        for (i = 0; i < qsv_vpp->surface_num; i++){
            qsv_vpp->p_surfaces[i] = av_mallocz( sizeof(mfxFrameSurface1) );
            AV_QSV_CHECK_POINTER(qsv_vpp->p_surfaces[i], MFX_ERR_MEMORY_ALLOC);
            memcpy(&(qsv_vpp->p_surfaces[i]->Info), &(qsv_vpp->m_mfxVideoParam.vpp.Out), sizeof(mfxFrameInfo));
        }

        qsv_vpp->sync_num = FFMIN( qsv_vpp->surface_num, AV_QSV_SYNC_NUM );

        for (i = 0; i < qsv_vpp->sync_num; i++){
            qsv_vpp->p_syncp[i] = av_mallocz(sizeof(av_qsv_sync));
            AV_QSV_CHECK_POINTER(qsv_vpp->p_syncp[i], MFX_ERR_MEMORY_ALLOC);
            qsv_vpp->p_syncp[i]->p_sync = av_mallocz(sizeof(mfxSyncPoint));
            AV_QSV_CHECK_POINTER(qsv_vpp->p_syncp[i]->p_sync, MFX_ERR_MEMORY_ALLOC);
        }
/*
    about available VPP filters, see "Table 4 Configurable VPP filters", mediasdk-man.pdf
    Hints (optional feature) IDs:
    MFX_EXTBUFF_VPP_DENOISE                 // Remove noise
                                            //    Value of 0-100 (inclusive) indicates
                                            //    the level of noise to remove.
    MFX_EXTBUFF_VPP_DETAIL                  // Enhance picture details/edges:
                                            //    0-100 value (inclusive) to indicate
                                            //    the level of details to be enhanced.
    MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION   // Convert input frame rate to match the output, based on frame interpolation:
                                            //    MFX_FRCALGM_PRESERVE_TIMESTAMP,
                                            //    MFX_FRCALGM_DISTRIBUTED_TIMESTAMP,
                                            //    MFX_FRCALGM_FRAME_INTERPOLATION
    MFX_EXTBUFF_VPP_IMAGE_STABILIZATION     // Perform image stabilization
                                            //  Stabilization modes:
                                            //      MFX_IMAGESTAB_MODE_UPSCALE
                                            //      MFX_IMAGESTAB_MODE_BOXING
    MFX_EXTBUFF_VPP_PICSTRUCT_DETECTION     // Perform detection of picture structure:
                                            //    Detected picture structure - top field first, bottom field first, progressive or unknown
                                            //    if video processor cannot detect picture structure.
    MFX_EXTBUFF_VPP_PROCAMP                 // Adjust the brightness, contrast, saturation, and hue settings

    // Initialize extended buffer for frame processing
    // - Process amplifier (ProcAmp) used to control brightness
    // - mfxExtVPPDoUse:   Define the processing algorithm to be used
    // - mfxExtVPPProcAmp: ProcAmp configuration
    // - mfxExtBuffer:     Add extended buffers to VPP parameter configuration
    mfxExtVPPDoUse extDoUse;
    mfxU32 tabDoUseAlg[1];
    extDoUse.Header.BufferId = MFX_EXTBUFF_VPP_DOUSE;
    extDoUse.Header.BufferSz = sizeof(mfxExtVPPDoUse);
    extDoUse.NumAlg  = 1;
    extDoUse.AlgList = tabDoUseAlg;
    tabDoUseAlg[0] = MFX_EXTBUFF_VPP_PROCAMP;

    mfxExtVPPProcAmp procampConfig;
    procampConfig.Header.BufferId = MFX_EXTBUFF_VPP_PROCAMP;
    procampConfig.Header.BufferSz = sizeof(mfxExtVPPProcAmp);
    procampConfig.Hue        = 0.0f;  // Default
    procampConfig.Saturation = 1.0f;  // Default
    procampConfig.Contrast   = 1.0;   // Default
    procampConfig.Brightness = 40.0;  // Adjust brightness

    mfxExtBuffer* ExtBuffer[2];
    ExtBuffer[0] = (mfxExtBuffer*)&extDoUse;
    ExtBuffer[1] = (mfxExtBuffer*)&procampConfig;
    VPPParams.NumExtParam = 2;
    VPPParams.ExtParam = (mfxExtBuffer**)&ExtBuffer[0];
*/
        memset(&qsv_vpp->ext_opaque_alloc, 0, sizeof(qsv_vpp->ext_opaque_alloc));

        if( (qsv_vpp->m_mfxVideoParam.vpp.In.FrameRateExtN  /  qsv_vpp->m_mfxVideoParam.vpp.In.FrameRateExtD ) !=
            (qsv_vpp->m_mfxVideoParam.vpp.Out.FrameRateExtN /  qsv_vpp->m_mfxVideoParam.vpp.Out.FrameRateExtD) )
        {
            pv->is_frc_used = 1;
        }

        qsv_vpp->m_mfxVideoParam.NumExtParam        = qsv_vpp->p_ext_param_num = 1 + pv->is_frc_used;

        qsv_vpp->p_ext_params = av_mallocz(sizeof(mfxExtBuffer *)*qsv_vpp->p_ext_param_num);
        AV_QSV_CHECK_POINTER(qsv_vpp->p_ext_params, MFX_ERR_MEMORY_ALLOC);

        qsv_vpp->m_mfxVideoParam.ExtParam           = qsv_vpp->p_ext_params;

        qsv_vpp->ext_opaque_alloc.In.Surfaces       = qsv->dec_space->p_surfaces;
        qsv_vpp->ext_opaque_alloc.In.NumSurface     = qsv->dec_space->surface_num;
        qsv_vpp->ext_opaque_alloc.In.Type           = qsv->dec_space->request[0].Type;

        qsv_vpp->ext_opaque_alloc.Out.Surfaces      = qsv_vpp->p_surfaces;
        qsv_vpp->ext_opaque_alloc.Out.NumSurface    = qsv_vpp->surface_num;
        qsv_vpp->ext_opaque_alloc.Out.Type          = qsv->dec_space->request[0].Type;

        qsv_vpp->ext_opaque_alloc.Header.BufferId   = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
        qsv_vpp->ext_opaque_alloc.Header.BufferSz   = sizeof(mfxExtOpaqueSurfaceAlloc);
        qsv_vpp->p_ext_params[0]                    = (mfxExtBuffer*)&qsv_vpp->ext_opaque_alloc;

        if(pv->is_frc_used)
        {
            pv->frc_config.Header.BufferId  = MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION;
            pv->frc_config.Header.BufferSz  = sizeof(mfxExtVPPFrameRateConversion);
            pv->frc_config.Algorithm        = MFX_FRCALGM_PRESERVE_TIMESTAMP;

            qsv_vpp->p_ext_params[1] = (mfxExtBuffer*)&pv->frc_config;
        }

        sts = MFXVideoVPP_Init(qsv->mfx_session, &qsv_vpp->m_mfxVideoParam);

        AV_QSV_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        AV_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        qsv_vpp->is_init_done = 1;
    }
    return 0;
}

static int hb_qsv_filter_init( hb_filter_object_t * filter,
                               hb_filter_init_t * init )
{

    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;

    pv->list = hb_list_init();
    // list of init params provided at work.c:~700
    pv->width_in  = init->width;
    pv->height_in = init->height;
    pv->width_out = init->width;
    pv->height_out = init->height;
    memcpy( pv->crop, init->crop, sizeof( int[4] ) );

    if (filter->settings != NULL)
    {
        sscanf(filter->settings, "%d:%d:%d:%d:%d:%d_dei:%d",
               &pv->width_out, &pv->height_out,
               &pv->crop[0], &pv->crop[1], &pv->crop[2], &pv->crop[3],
               &pv->deinterlace);
    }

    pv->job = init->job;

    // will be later as more params will be known
    // filter_init(pv->job->qsv, pv);

    // just passing
    init->vrate = init->vrate;
    init->vrate_base = init->vrate_base;

    // framerate shaping not yet supported
    init->cfr = 0;

    init->pix_fmt = pv->pix_fmt;
    init->width = pv->width_out;
    init->height = pv->height_out;
    memcpy( init->crop, pv->crop, sizeof( int[4] ) );

    return 0;
}

static int hb_qsv_filter_info( hb_filter_object_t * filter,
                               hb_filter_info_t * info )
{

    hb_filter_private_t *pv = filter->private_data;
    if (pv == NULL)
        return -1;

    sprintf(info->human_readable_desc,
            "source: %d * %d, crop (%d/%d/%d/%d): %d * %d, scale: %d * %d",
            pv->width_in, pv->height_in,
            pv->crop[0], pv->crop[1], pv->crop[2], pv->crop[3],
            pv->width_in  - pv->crop[2] - pv->crop[3],
            pv->height_in - pv->crop[0] - pv->crop[1],
            pv->width_out, pv->height_out);

    if (pv->deinterlace)
    {
        sprintf(info->human_readable_desc + strlen(info->human_readable_desc),
                ", deinterlace");
    }

    return 0;
}

void qsv_filter_close( av_qsv_context* qsv, AV_QSV_STAGE_TYPE vpp_type ){
    int i = 0;
    av_qsv_space* vpp_space = 0;

    if(qsv && qsv->is_context_active && qsv->vpp_space)
    for(i=av_qsv_list_count( qsv->vpp_space);i>0;i--){

        vpp_space = av_qsv_list_item( qsv->vpp_space, i-1 );
        if( vpp_space->type == vpp_type && vpp_space->is_init_done){

            hb_log( "qsv_filter[%s] done: max_surfaces: %u/%u , max_syncs: %u/%u", ((vpp_type == AV_QSV_VPP_DEFAULT)?"Default": "User") ,vpp_space->surface_num_max_used, vpp_space->surface_num, vpp_space->sync_num_max_used, vpp_space->sync_num );

            for (i = 0; i < vpp_space->surface_num; i++){
                av_freep(&vpp_space->p_surfaces[i]);
            }
            vpp_space->surface_num = 0;

            if( vpp_space->p_ext_param_num || vpp_space->p_ext_params )
                av_freep(&vpp_space->p_ext_params);
            vpp_space->p_ext_param_num = 0;

            for (i = 0; i < vpp_space->sync_num; i++){
                av_freep(&vpp_space->p_syncp[i]->p_sync);
                av_freep(&vpp_space->p_syncp[i]);
            }
            vpp_space->sync_num = 0;

            av_qsv_list_rem(qsv->vpp_space,vpp_space);
            if( av_qsv_list_count(qsv->vpp_space) == 0 )
                av_qsv_list_close(&qsv->vpp_space);

            vpp_space->is_init_done = 0;
            break;
        }
    }
}

static void hb_qsv_filter_close( hb_filter_object_t * filter )
{
    int i = 0;
    hb_filter_private_t * pv = filter->private_data;

    if ( !pv )
    {
        return;
    }

    av_qsv_context* qsv = pv->job->qsv;
    if(qsv && qsv->vpp_space && av_qsv_list_count(qsv->vpp_space) > 0){

        // closing local stuff
        qsv_filter_close(qsv,AV_QSV_VPP_DEFAULT);

        // closing the commong stuff
        av_qsv_context_clean(qsv);
    }
    hb_list_close(&pv->list);
    free( pv );
    filter->private_data = NULL;
}

int process_frame(av_qsv_list* received_item, av_qsv_context* qsv, hb_filter_private_t * pv ){

    // 1 if have results , 0 - otherwise
    int ret = 1;

    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *work_surface = NULL;
    av_qsv_stage* stage = 0;

    av_qsv_space *qsv_vpp = pv->vpp_space;

    if(received_item){
        stage = av_qsv_get_last_stage( received_item );
        work_surface = stage->out.p_surface;
    }

    int sync_idx = av_qsv_get_free_sync(qsv_vpp, qsv);
    int surface_idx = -1;

    for(;;)
    {
            if (sync_idx == -1)
            {
                hb_error("qsv: Not enough resources allocated for QSV filter");
                ret = 0;
                break;
            }
            if( sts == MFX_ERR_MORE_SURFACE || sts == MFX_ERR_NONE )
               surface_idx = av_qsv_get_free_surface(qsv_vpp, qsv,  &(qsv_vpp->m_mfxVideoParam.vpp.Out), QSV_PART_ANY);
            if (surface_idx == -1) {
                hb_error("qsv: Not enough resources allocated for QSV filter");
                ret = 0;
                break;
            }
            if (work_surface) {
                    work_surface->Info.CropX           = pv->crop[2];
                    work_surface->Info.CropY           = pv->crop[0];
                    work_surface->Info.CropW           = pv->width_in - pv->crop[3] - pv->crop[2];
                    work_surface->Info.CropH           = pv->height_in - pv->crop[1] - pv->crop[0];
            }

            sts = MFXVideoVPP_RunFrameVPPAsync(qsv->mfx_session, work_surface, qsv_vpp->p_surfaces[surface_idx] , NULL, qsv_vpp->p_syncp[sync_idx]->p_sync);

            if( MFX_ERR_MORE_DATA == sts ){
                if(!qsv_vpp->pending){
                    qsv_vpp->pending = av_qsv_list_init(0);
                }

                // if we have no results, we should not miss resource(s)
                av_qsv_list_add( qsv_vpp->pending, received_item);

                ff_qsv_atomic_dec(&qsv_vpp->p_syncp[sync_idx]->in_use);

                ret = 0;
                break;
            }

            if( MFX_ERR_MORE_DATA == sts || (MFX_ERR_NONE <= sts && MFX_WRN_DEVICE_BUSY != sts)){
                if (work_surface){
                   ff_qsv_atomic_dec(&work_surface->Data.Locked);
               }
            }

            if( MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE <= sts){
                if( MFX_ERR_MORE_SURFACE == sts )
                    continue;

                if (qsv_vpp->p_surfaces[surface_idx] && MFX_WRN_DEVICE_BUSY != sts )
                   ff_qsv_atomic_inc(&qsv_vpp->p_surfaces[surface_idx]->Data.Locked);
            }

            AV_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            if (MFX_ERR_NONE <= sts ) // repeat the call if warning and no output
            {
                if (MFX_WRN_DEVICE_BUSY == sts){
                    av_qsv_sleep(10); // wait if device is busy
                    continue;
                }

                // shouldnt be a case but drain
                if(stage){
                        av_qsv_stage* new_stage = av_qsv_stage_init();

                        new_stage->type = AV_QSV_VPP_DEFAULT;
                        new_stage->in.p_surface  =  work_surface;
                        new_stage->out.p_surface = qsv_vpp->p_surfaces[surface_idx];
                        new_stage->out.sync      = qsv_vpp->p_syncp[sync_idx];
                        av_qsv_add_stagee( &received_item, new_stage,HAVE_THREADS );

                        // add pending resources for the proper reclaim later
                        if( qsv_vpp->pending ){
                            if( av_qsv_list_count(qsv_vpp->pending)>0 ){
                                new_stage->pending = qsv_vpp->pending;
                }
                            qsv_vpp->pending = 0;

                            // making free via decrement for all pending
                            int i = 0;
                            for (i = av_qsv_list_count(new_stage->pending); i > 0; i--){
                                av_qsv_list *atom_list = av_qsv_list_item(new_stage->pending, i-1);
                                av_qsv_stage *stage = av_qsv_get_last_stage( atom_list );
                                mfxFrameSurface1 *work_surface = stage->out.p_surface;
                                if (work_surface)
                                   ff_qsv_atomic_dec(&work_surface->Data.Locked);
                            }
                        }
                }
                break;
            }

            ff_qsv_atomic_dec(&qsv_vpp->p_syncp[sync_idx]->in_use);

            if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
                DEBUG_ASSERT( 1,"The bitstream buffer size is insufficient." );

            break;
    }

    return ret;
}

static int hb_qsv_filter_work( hb_filter_object_t * filter,
                               hb_buffer_t ** buf_in,
                               hb_buffer_t ** buf_out )
{

    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * out = *buf_out;
    int sts = 0;

    av_qsv_context* qsv = pv->job->qsv;

    if ( !pv )
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_OK;
    }

    while(1){
        int ret = filter_init(qsv,pv);
        if(ret >= 2)
            av_qsv_sleep(1);
        else
            break;
    }

    *buf_in = NULL;

    if ( in->size <= 0 )
    {
        while(1){
            sts = process_frame(in->qsv_details.qsv_atom, qsv, pv);
            if(sts)
                hb_list_add(pv->list,in);
            else
                break;
        }

        hb_list_add( pv->list, in );
        *buf_out = link_buf_list( pv );
        return HB_FILTER_DONE;
    }

    sts = process_frame(in->qsv_details.qsv_atom, qsv, pv);

    if(sts){
        hb_list_add(pv->list,in);
    }

    if( hb_list_count(pv->list) ){
        *buf_out = hb_list_item(pv->list,0);
        out = *buf_out;
        if(pv->is_frc_used && out)
        {
            mfxStatus sts = MFX_ERR_NONE;
                if(out->qsv_details.qsv_atom){
                    av_qsv_stage* stage = av_qsv_get_last_stage( out->qsv_details.qsv_atom );
                    mfxFrameSurface1 *work_surface = stage->out.p_surface;

                    av_qsv_wait_on_sync( qsv,stage );

                    av_qsv_space *qsv_vpp = pv->vpp_space;
                    int64_t duration  = ((double)qsv_vpp->m_mfxVideoParam.vpp.Out.FrameRateExtD/(double)qsv_vpp->m_mfxVideoParam.vpp.Out.FrameRateExtN ) * 90000.;
                    out->s.start = work_surface->Data.TimeStamp;
                    out->s.stop = work_surface->Data.TimeStamp + duration;
                }
        }
        hb_list_rem(pv->list,*buf_out);
    }
    else
        *buf_out = NULL;

    return HB_FILTER_OK;
}

// see devavcode.c
hb_buffer_t *link_buf_list( hb_filter_private_t *pv )
{
        hb_buffer_t *head = hb_list_item( pv->list, 0 );

        if ( head )
        {
            hb_list_rem( pv->list, head );
            hb_buffer_t *last = head, *buf;
            while ( ( buf = hb_list_item( pv->list, 0 ) ) != NULL )
            {
                hb_list_rem( pv->list, buf );
                last->next = buf;
                last = buf;
            }
        }
    return head;
}

