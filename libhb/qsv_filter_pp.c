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
#include "hbffmpeg.h"
#include "libavcodec/qsv.h"
#include "qsv_filter_pp.h"
#include "qsv_filter.h"
#include "qsv_memory.h"


static int hb_qsv_filter_pre_init( hb_filter_object_t * filter,
                               hb_filter_init_t * init );
static int hb_qsv_filter_pre_work( hb_filter_object_t * filter,
                               hb_buffer_t ** buf_in,
                               hb_buffer_t ** buf_out );
static int hb_qsv_filter_pre_info( hb_filter_object_t * filter,
                               hb_filter_info_t * info );
static void hb_qsv_filter_pre_close( hb_filter_object_t * filter );

static int hb_qsv_filter_post_init( hb_filter_object_t * filter,
                               hb_filter_init_t * init );
static int hb_qsv_filter_post_work( hb_filter_object_t * filter,
                               hb_buffer_t ** buf_in,
                               hb_buffer_t ** buf_out );
static int hb_qsv_filter_post_info( hb_filter_object_t * filter,
                               hb_filter_info_t * info );
static void hb_qsv_filter_post_close( hb_filter_object_t * filter );


hb_filter_object_t hb_filter_qsv_pre =
{
    .id            = HB_FILTER_QSV_PRE,
    .enforce_order = 1,
    .name          = "Quick Sync Video user filter (pre)",
    .settings      = NULL,
    .init          = hb_qsv_filter_pre_init,
    .work          = hb_qsv_filter_pre_work,
    .close         = hb_qsv_filter_pre_close,
    .info          = hb_qsv_filter_pre_info,
};

hb_filter_object_t hb_filter_qsv_post =
{
    .id            = HB_FILTER_QSV_POST,
    .enforce_order = 1,
    .name          = "Quick Sync Video user filter (post)",
    .settings      = NULL,
    .init          = hb_qsv_filter_post_init,
    .work          = hb_qsv_filter_post_work,
    .close         = hb_qsv_filter_post_close,
    .info          = hb_qsv_filter_post_info,
};


static int filter_pre_init( av_qsv_context* qsv, hb_filter_private_t * pv ){
    mfxStatus sts = MFX_ERR_NONE;
    int i=0;

    if(!qsv) return 3;

    av_qsv_space *prev_vpp = 0;

    if(!qsv->vpp_space){
        qsv->vpp_space = av_qsv_list_init(HAVE_THREADS);
        // note some change as : when no size changes -> no VPP used
        // impact on : prev_vpp
    }

    if(!pv->vpp_space){
        for(i=0; i<av_qsv_list_count(qsv->vpp_space);i++){
            av_qsv_space *qsv_vpp = av_qsv_list_item( qsv->vpp_space, i );
            if(qsv_vpp->type == AV_QSV_VPP_USER){
                pv->vpp_space = qsv_vpp;
                break;
            }
            else
            if(qsv_vpp->type == AV_QSV_VPP_DEFAULT){
                prev_vpp = qsv_vpp;
            }

        }
    }

    if(!pv->vpp_space){
        pv->vpp_space = calloc( 1, sizeof( av_qsv_space ));
        pv->vpp_space->type = AV_QSV_VPP_USER;
        av_qsv_list_add( qsv->vpp_space, pv->vpp_space );
        av_qsv_add_context_usage(qsv,HAVE_THREADS);
    }
    else
        if(pv->vpp_space->is_init_done ) return 1;

    if(!qsv->dec_space || !qsv->dec_space->is_init_done) return 2;

    av_qsv_space *qsv_vpp = pv->vpp_space;

    AV_QSV_ZERO_MEMORY(qsv_vpp->m_mfxVideoParam);


    if (prev_vpp)
    {
        memcpy( &qsv_vpp->m_mfxVideoParam.vpp,  &prev_vpp->m_mfxVideoParam.vpp, sizeof(prev_vpp->m_mfxVideoParam.vpp));
    }
    else
    {
        AV_QSV_ZERO_MEMORY(qsv_vpp->m_mfxVideoParam);

        // FrameRate is important for VPP to start with
        if( qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtN == 0 &&
            qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtD == 0 ){
            qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtN = pv->job->title->vrate.num;
            qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtD = pv->job->title->vrate.den;
        }

        qsv_vpp->m_mfxVideoParam.vpp.In.FourCC          = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FourCC;
        qsv_vpp->m_mfxVideoParam.vpp.In.ChromaFormat    = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.ChromaFormat;
        qsv_vpp->m_mfxVideoParam.vpp.In.CropX           = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.CropX;
        qsv_vpp->m_mfxVideoParam.vpp.In.CropY           = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.CropY;
        qsv_vpp->m_mfxVideoParam.vpp.In.CropW           = pv->job->title->geometry.width;
        qsv_vpp->m_mfxVideoParam.vpp.In.CropH           = pv->job->title->geometry.height;
        qsv_vpp->m_mfxVideoParam.vpp.In.PicStruct       = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.PicStruct;
        qsv_vpp->m_mfxVideoParam.vpp.In.FrameRateExtN   = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtN;
        qsv_vpp->m_mfxVideoParam.vpp.In.FrameRateExtD   = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtD;
        qsv_vpp->m_mfxVideoParam.vpp.In.AspectRatioW    = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.AspectRatioW;
        qsv_vpp->m_mfxVideoParam.vpp.In.AspectRatioH    = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.AspectRatioH;
        qsv_vpp->m_mfxVideoParam.vpp.In.Width           = AV_QSV_ALIGN16(pv->job->title->geometry.width);
        qsv_vpp->m_mfxVideoParam.vpp.In.Height          = (MFX_PICSTRUCT_PROGRESSIVE == qsv_vpp->m_mfxVideoParam.vpp.In.PicStruct)?
                                                            AV_QSV_ALIGN16(pv->job->title->geometry.height) : AV_QSV_ALIGN32(pv->job->title->geometry.height);

        qsv_vpp->m_mfxVideoParam.vpp.Out.FourCC          = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FourCC;
        qsv_vpp->m_mfxVideoParam.vpp.Out.ChromaFormat    = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.ChromaFormat;
        qsv_vpp->m_mfxVideoParam.vpp.Out.CropX           = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.CropX;
        qsv_vpp->m_mfxVideoParam.vpp.Out.CropY           = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.CropY;
        qsv_vpp->m_mfxVideoParam.vpp.Out.CropW           = pv->job->title->geometry.width;
        qsv_vpp->m_mfxVideoParam.vpp.Out.CropH           = pv->job->title->geometry.height;
        qsv_vpp->m_mfxVideoParam.vpp.Out.PicStruct       = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.PicStruct;
        qsv_vpp->m_mfxVideoParam.vpp.Out.FrameRateExtN   = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtN;
        qsv_vpp->m_mfxVideoParam.vpp.Out.FrameRateExtD   = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.FrameRateExtD;
        qsv_vpp->m_mfxVideoParam.vpp.Out.AspectRatioW    = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.AspectRatioW;
        qsv_vpp->m_mfxVideoParam.vpp.Out.AspectRatioH    = qsv->dec_space->m_mfxVideoParam.mfx.FrameInfo.AspectRatioH;
        qsv_vpp->m_mfxVideoParam.vpp.Out.Width           = AV_QSV_ALIGN16(pv->job->title->geometry.width);
        qsv_vpp->m_mfxVideoParam.vpp.Out.Height          = (MFX_PICSTRUCT_PROGRESSIVE == qsv_vpp->m_mfxVideoParam.vpp.In.PicStruct)?
                                                            AV_QSV_ALIGN16(pv->job->title->geometry.height) : AV_QSV_ALIGN32(pv->job->title->geometry.height);

        memset(&qsv_vpp->request, 0, sizeof(mfxFrameAllocRequest)*2);
    }

    qsv_vpp->m_mfxVideoParam.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY | MFX_IOPATTERN_OUT_OPAQUE_MEMORY;

    qsv_vpp->surface_num = FFMIN(prev_vpp ? prev_vpp->surface_num : qsv->dec_space->surface_num/2, AV_QSV_SURFACE_NUM);

    for(i = 0; i < qsv_vpp->surface_num; i++){
        qsv_vpp->p_surfaces[i] = av_mallocz( sizeof(mfxFrameSurface1) );
        AV_QSV_CHECK_POINTER(qsv_vpp->p_surfaces[i], MFX_ERR_MEMORY_ALLOC);
        memcpy(&(qsv_vpp->p_surfaces[i]->Info), &(qsv_vpp->m_mfxVideoParam.vpp.Out), sizeof(mfxFrameInfo));
    }

    qsv_vpp->sync_num = FFMIN(prev_vpp ? prev_vpp->sync_num : qsv->dec_space->sync_num, AV_QSV_SYNC_NUM);
    for (i = 0; i < qsv_vpp->sync_num; i++){
        qsv_vpp->p_syncp[i] = av_mallocz(sizeof(av_qsv_sync));
        AV_QSV_CHECK_POINTER(qsv_vpp->p_syncp[i], MFX_ERR_MEMORY_ALLOC);
        qsv_vpp->p_syncp[i]->p_sync = av_mallocz(sizeof(mfxSyncPoint));
        AV_QSV_CHECK_POINTER(qsv_vpp->p_syncp[i]->p_sync, MFX_ERR_MEMORY_ALLOC);
    }

    memset(&qsv_vpp->ext_opaque_alloc, 0, sizeof(mfxExtOpaqueSurfaceAlloc));
    qsv_vpp->m_mfxVideoParam.NumExtParam        = qsv_vpp->p_ext_param_num = 1;

    qsv_vpp->p_ext_params = av_mallocz(sizeof(mfxExtBuffer *)*qsv_vpp->p_ext_param_num);
    AV_QSV_CHECK_POINTER(qsv_vpp->p_ext_params, MFX_ERR_MEMORY_ALLOC);

    qsv_vpp->m_mfxVideoParam.ExtParam           = qsv_vpp->p_ext_params;

    qsv_vpp->ext_opaque_alloc.Header.BufferId   = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
    qsv_vpp->ext_opaque_alloc.Header.BufferSz   = sizeof(mfxExtOpaqueSurfaceAlloc);
    qsv_vpp->p_ext_params[0]                    = (mfxExtBuffer*)&qsv_vpp->ext_opaque_alloc;

    if(prev_vpp){
        qsv_vpp->ext_opaque_alloc.In.Surfaces       = prev_vpp->p_surfaces;
        qsv_vpp->ext_opaque_alloc.In.NumSurface     = prev_vpp->surface_num;
    }
    else{
        qsv_vpp->ext_opaque_alloc.In.Surfaces       = qsv->dec_space->p_surfaces;
        qsv_vpp->ext_opaque_alloc.In.NumSurface     = qsv->dec_space->surface_num;
    }
    qsv_vpp->ext_opaque_alloc.In.Type           = qsv->dec_space->request[0].Type;

    qsv_vpp->ext_opaque_alloc.Out.Surfaces      = qsv_vpp->p_surfaces;
    qsv_vpp->ext_opaque_alloc.Out.NumSurface    = qsv_vpp->surface_num;
    qsv_vpp->ext_opaque_alloc.Out.Type          = qsv->dec_space->request[0].Type;

    pv->qsv_user = hb_list_init();

    qsv_filter_t *plugin = av_mallocz( sizeof(qsv_filter_t) );

    plugin->pv               = pv;
    plugin->plug.pthis       = plugin;
    plugin->plug.PluginInit  = qsv_PluginInit;
    plugin->plug.PluginClose = qsv_PluginClose;
    plugin->plug.GetPluginParam = qsv_GetPluginParam;
    plugin->plug.Submit      = qsv_Submit;
    plugin->plug.Execute     = qsv_Execute;
    plugin->plug.FreeResources = qsv_FreeResources;

    hb_list_add(pv->qsv_user,plugin);

    sts=MFXVideoUSER_Register(qsv->mfx_session,0,&plugin->plug);
    AV_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    plugin_init(plugin,&qsv_vpp->m_mfxVideoParam);

    qsv_vpp->is_init_done = 1;

    return 0;
}

static int hb_qsv_filter_pre_info( hb_filter_object_t * filter,
                               hb_filter_info_t * info ){
    hb_filter_private_t * pv = filter->private_data;
    if( !pv )
        return 0;

    sprintf(info->human_readable_desc, "copy data to system memory");

    return 0;
}
static int hb_qsv_filter_pre_init( hb_filter_object_t * filter,
                               hb_filter_init_t * init ){
    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;
    pv->job = init->job;

    pv->pre.frame_go             = 0;
    pv->pre.frame_completed      = hb_cond_init();
    pv->pre.frame_completed_lock = hb_lock_init();

    pv->post.frame_go            = 0;
    pv->post.frame_completed     = hb_cond_init();
    pv->post.frame_completed_lock = hb_lock_init();

    pv->pre_busy.frame_go            = 0;
    pv->pre_busy.frame_completed     = hb_cond_init();
    pv->pre_busy.frame_completed_lock = hb_lock_init();

    pv->post_busy.frame_go               = 0;
    pv->post_busy.frame_completed        = hb_cond_init();
    pv->post_busy.frame_completed_lock   = hb_lock_init();

    pv->list = hb_list_init();

    // just to remind:
    // PIX_FMT_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples) , 3 planes: Y, U, V
    // PIX_FMT_NV12,      ///< planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
    pv->sws_context_from_nv12 = hb_sws_get_context(
        pv->job->title->geometry.width, pv->job->title->geometry.height,
        AV_PIX_FMT_NV12,
        pv->job->title->geometry.width, pv->job->title->geometry.height,
        AV_PIX_FMT_YUV420P,
        SWS_LANCZOS|SWS_ACCURATE_RND);
    pv->sws_context_to_nv12 = hb_sws_get_context(
        pv->job->title->geometry.width, pv->job->title->geometry.height,
        AV_PIX_FMT_YUV420P,
        pv->job->title->geometry.width, pv->job->title->geometry.height,
        AV_PIX_FMT_NV12,
        SWS_LANCZOS|SWS_ACCURATE_RND);
    return 0;
}
int pre_process_frame(hb_buffer_t *in, av_qsv_context* qsv, hb_filter_private_t * pv ){

    // 1 if have results , 0 otherwise
    int ret = 1;

    av_qsv_list* received_item = in->qsv_details.qsv_atom;

    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1 *work_surface = NULL;
    av_qsv_stage* stage = 0;

    av_qsv_space *qsv_vpp = pv->vpp_space;

    if (received_item)
    {
        stage = av_qsv_get_last_stage( received_item );
        work_surface = stage->out.p_surface;
    }

    int sync_idx = av_qsv_get_free_sync(qsv_vpp, qsv);
    int surface_idx = -1;

    for (;;)
    {
            if (sync_idx == -1)
            {
                hb_error("qsv: Not enough resources allocated for the preprocessing filter");
                ret = 0;
                break;
            }

            if (sts == MFX_ERR_MORE_SURFACE || sts == MFX_ERR_NONE)
               surface_idx = av_qsv_get_free_surface(qsv_vpp, qsv,  &(qsv_vpp->m_mfxVideoParam.vpp.Out), QSV_PART_ANY);
            if (surface_idx == -1) {
                hb_error("qsv: Not enough resources allocated for the preprocessing filter");
                ret = 0;
                break;
            }

            sts = MFXVideoUSER_ProcessFrameAsync(qsv->mfx_session, &work_surface, 1, &qsv_vpp->p_surfaces[surface_idx] , 1, qsv_vpp->p_syncp[sync_idx]->p_sync);

            if (MFX_ERR_MORE_DATA == sts)
            {
                if (!qsv_vpp->pending)
                {
                    qsv_vpp->pending = av_qsv_list_init(0);
                }

                // if we have no results, we should not miss resource(s)
                av_qsv_list_add( qsv_vpp->pending, received_item);

                ff_qsv_atomic_dec(&qsv_vpp->p_syncp[sync_idx]->in_use);

                ret = 0;
                break;
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
                    hb_lock(pv->pre_busy.frame_completed_lock);
                    while(!pv->pre_busy.frame_go){
                        hb_cond_timedwait(pv->pre_busy.frame_completed,pv->pre_busy.frame_completed_lock,1000);
                        if(*pv->job->die)
                            break;
                    }
                    pv->pre_busy.frame_go = 0;
                    hb_unlock(pv->pre_busy.frame_completed_lock);

                    continue;
                }
                hb_lock(pv->pre.frame_completed_lock);
                while(!pv->pre.frame_go){
                    hb_cond_timedwait(pv->pre.frame_completed,pv->pre.frame_completed_lock,1000);
                    if(*pv->job->die)
                        break;
                }
                pv->pre.frame_go = 0;
                hb_unlock(pv->pre.frame_completed_lock);

                in = pv->pre.out;

                if (work_surface){
                   ff_qsv_atomic_dec(&work_surface->Data.Locked);
                }

                // inserting for the future, will be locked until very ready
                if(stage){
                        av_qsv_stage* new_stage = av_qsv_stage_init();

                        new_stage->type = AV_QSV_VPP_USER;
                        new_stage->in.p_surface  =  work_surface;
                        new_stage->out.p_surface = qsv_vpp->p_surfaces[surface_idx];
                        new_stage->out.sync = qsv_vpp->p_syncp[sync_idx];
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
                HB_DEBUG_ASSERT(1, "The bitstream buffer size is insufficient.");

            break;
    }

    return ret;
}

static int hb_qsv_filter_pre_work( hb_filter_object_t * filter,
                               hb_buffer_t ** buf_in,
                               hb_buffer_t ** buf_out ){
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * out = *buf_out;
    int sts = 0;

    av_qsv_context* qsv = pv->job->qsv.ctx;

    if(!in->qsv_details.filter_details)
        in->qsv_details.filter_details = pv;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    while(1){
        int ret = filter_pre_init(qsv,pv);
        if(ret >= 2)
            av_qsv_sleep(1);
        else
            break;
    }

    pv->pre.in  = in;
    pv->pre.out = in;

    sts = pre_process_frame(in, qsv, pv);

    if(sts){
        hb_list_add(pv->list,out);
    }

    if( hb_list_count(pv->list) ){
        *buf_out = hb_list_item(pv->list,0);
        hb_list_rem(pv->list,*buf_out);
         *buf_in = NULL;
    }
    else{
        *buf_in = NULL;
        *buf_out = in;
    }

    return HB_FILTER_OK;
}
static void hb_qsv_filter_pre_close( hb_filter_object_t * filter ){
    int i = 0;
    mfxStatus sts = MFX_ERR_NONE;

    hb_filter_private_t * pv = filter->private_data;

    if ( !pv )
    {
        return;
    }

    sws_freeContext(pv->sws_context_to_nv12);
    sws_freeContext(pv->sws_context_from_nv12);

    av_qsv_context* qsv = pv->job->qsv.ctx;
    if(qsv && qsv->vpp_space && av_qsv_list_count(qsv->vpp_space) > 0 ){
        if(pv->qsv_user && qsv->mfx_session){

            sts=MFXVideoUSER_Unregister(qsv->mfx_session,0);
            AV_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            for(i=hb_list_count(pv->qsv_user);i>0;i--){
                qsv_filter_t *plugin = hb_list_item(pv->qsv_user,i-1);
                hb_list_rem(pv->qsv_user,plugin);
                plugin_close(plugin);
            }
            hb_list_close(&pv->qsv_user);
        }

        // closing local stuff
        qsv_filter_close(qsv,AV_QSV_VPP_USER);

        // closing the commong stuff
        av_qsv_context_clean(qsv);
    }
    hb_cond_close(&pv->pre.frame_completed);
    hb_lock_close(&pv->pre.frame_completed_lock);

    hb_cond_close(&pv->post.frame_completed);
    hb_lock_close(&pv->post.frame_completed_lock);

    hb_cond_close(&pv->pre_busy.frame_completed);
    hb_lock_close(&pv->pre_busy.frame_completed_lock);

    hb_cond_close(&pv->post_busy.frame_completed);
    hb_lock_close(&pv->post_busy.frame_completed_lock);

    hb_list_close( &pv->list );

    free( pv );
    filter->private_data = NULL;
}


static int hb_qsv_filter_post_info( hb_filter_object_t * filter,
                               hb_filter_info_t * info ){
    hb_filter_private_t * pv = filter->private_data;
    if( !pv )
        return 0;

    sprintf(info->human_readable_desc, "copy data to opaque memory");

    return 0;
}
static int hb_qsv_filter_post_init( hb_filter_object_t * filter,
                               hb_filter_init_t * init ){
    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;
    pv->job = init->job;
    return 0;
}
static int hb_qsv_filter_post_work( hb_filter_object_t * filter,
                               hb_buffer_t ** buf_in,
                               hb_buffer_t ** buf_out ){
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * out = *buf_out;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    av_qsv_context* qsv = pv->job->qsv.ctx;
    pv = in->qsv_details.filter_details;

    if (!pv)
    {
        *buf_out = NULL;
        *buf_in = NULL;
        return HB_FILTER_OK;
    }

    while(1){
        int ret = filter_pre_init(qsv,pv);
        if(ret >= 2)
            av_qsv_sleep(1);
        else
            break;
    }

    pv->post.in = in;
    pv->post.out = out;

    // signal: input is prepared, can start inserting data back into pipeline
    hb_lock(pv->post.frame_completed_lock);
    pv->post.frame_go = 1;
    hb_cond_broadcast(pv->post.frame_completed);
    hb_unlock(pv->post.frame_completed_lock);

    // wait: on signal that data is ready
    hb_lock(pv->post_busy.frame_completed_lock);
    while(!pv->post_busy.frame_go){
        hb_cond_timedwait(pv->post_busy.frame_completed,pv->post_busy.frame_completed_lock,1000);
        if(*pv->job->die)
            break;
    }
    pv->post_busy.frame_go = 0;
    hb_unlock(pv->post_busy.frame_completed_lock);

    if (pv->post.status == HB_FILTER_OK || pv->post.status == HB_FILTER_DONE)
    {
    *buf_out = in;
    }
    else
    {
        *buf_out = NULL;
        pv->post.status = HB_FILTER_OK;
    }
    *buf_in = NULL;

    return HB_FILTER_OK;
}
static void hb_qsv_filter_post_close( hb_filter_object_t * filter ){
    hb_filter_private_t * pv = filter->private_data;

    if ( !pv )
    {
        return;
    }

    free( pv );
    filter->private_data = NULL;
}


mfxStatus MFX_CDECL qsv_PluginInit(mfxHDL pthis, mfxCoreInterface *core){
    mfxStatus sts = MFX_ERR_NONE;

    if(core && pthis){
        qsv_filter_t *plugin = pthis;
        plugin->core = core;

        plugin->pluginparam.MaxThreadNum = 1;
        plugin->pluginparam.ThreadPolicy = MFX_THREADPOLICY_SERIAL;
    }
    else
        sts = MFX_ERR_NULL_PTR;

    return sts;
}
mfxStatus MFX_CDECL qsv_PluginClose (mfxHDL pthis){
    mfxStatus sts = MFX_ERR_NONE;
    return sts;
}
mfxStatus MFX_CDECL qsv_GetPluginParam(mfxHDL pthis, mfxPluginParam *par){
    mfxStatus sts = MFX_ERR_NONE;

    if(pthis){
        qsv_filter_t *plugin = pthis;
        *par = plugin->pluginparam;
    }
    else
        sts = MFX_ERR_NULL_PTR;
    return sts;
}
mfxStatus MFX_CDECL qsv_Submit(mfxHDL pthis, const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task){
    mfxStatus sts = MFX_ERR_NONE;

    qsv_filter_t *plugin = pthis;

    mfxFrameSurface1 *surface_in = (mfxFrameSurface1 *)in[0];
    mfxFrameSurface1 *surface_out = (mfxFrameSurface1 *)out[0];
    mfxFrameSurface1 *real_surface_in = surface_in;
    mfxFrameSurface1 *real_surface_out = surface_out;

    sts = plugin->core->GetRealSurface(plugin->core->pthis, surface_in, &real_surface_in);
    AV_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, MFX_ERR_MEMORY_ALLOC);

    sts = plugin->core->GetRealSurface(plugin->core->pthis, surface_out, &real_surface_out);
    AV_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, MFX_ERR_MEMORY_ALLOC);

    int task_idx = get_free_task(plugin->tasks);

    if (task_idx == -1)
    {
        return MFX_WRN_DEVICE_BUSY;
    }

    plugin->core->IncreaseReference(plugin->core->pthis, &(real_surface_in->Data));
    plugin->core->IncreaseReference(plugin->core->pthis, &(real_surface_out->Data));

    // to preserve timing if other filters are used in-between
    surface_out->Data.TimeStamp  = surface_in->Data.TimeStamp;
    surface_out->Data.FrameOrder = surface_in->Data.FrameOrder;

    qsv_filter_task_t *current_task = hb_list_item(plugin->tasks,task_idx);
    current_task->in    = real_surface_in;
    current_task->out   = real_surface_out;
    current_task->busy  = 1;
    current_task->pv    = plugin->pv;

    *task = (mfxThreadTask)current_task;

    return sts;
}
mfxStatus MFX_CDECL qsv_Execute(mfxHDL pthis, mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a){
    mfxStatus sts = MFX_ERR_NONE;

    qsv_filter_task_t *current_task = (qsv_filter_task_t *)task;
    qsv_filter_t *plugin = pthis;

    sts = (current_task->processor.process)(current_task,0);
    AV_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    sts =  MFX_TASK_DONE;
    return sts;
}
mfxStatus MFX_CDECL qsv_FreeResources(mfxHDL pthis, mfxThreadTask task, mfxStatus sts){

    qsv_filter_t *plugin = pthis;
    qsv_filter_task_t *current_task = (qsv_filter_task_t *)task;

    plugin->core->DecreaseReference(plugin->core->pthis, &(current_task->in->Data));
    plugin->core->DecreaseReference(plugin->core->pthis, &(current_task->out->Data));

    current_task->busy  = 0;

    hb_lock(plugin->pv->pre_busy.frame_completed_lock);
    plugin->pv->pre_busy.frame_go = 1;
    hb_cond_broadcast(plugin->pv->pre_busy.frame_completed);
    hb_unlock(plugin->pv->pre_busy.frame_completed_lock);

    return MFX_ERR_NONE;
}

mfxStatus plugin_init(qsv_filter_t* plugin, mfxVideoParam *param){
    mfxStatus sts = MFX_ERR_NONE;

    if(plugin->is_init_done) return sts;

    plugin->videoparam = param;

    mfxExtOpaqueSurfaceAlloc* plugin_opaque_alloc = NULL;

    plugin_opaque_alloc = (mfxExtOpaqueSurfaceAlloc*) get_ext_buffer(plugin->videoparam->ExtParam,
                                    plugin->videoparam->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

    if(!plugin_opaque_alloc || !plugin_opaque_alloc->In.Surfaces || !plugin_opaque_alloc->Out.Surfaces)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    sts = plugin->core->MapOpaqueSurface(plugin->core->pthis, plugin_opaque_alloc->In.NumSurface,
            plugin_opaque_alloc->In.Type, plugin_opaque_alloc->In.Surfaces);
    AV_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, sts);


    sts = plugin->core->MapOpaqueSurface(plugin->core->pthis, plugin_opaque_alloc->Out.NumSurface,
            plugin_opaque_alloc->Out.Type, plugin_opaque_alloc->Out.Surfaces);
    AV_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, sts);


    plugin->tasks = hb_list_init();
    qsv_filter_task_t *task = calloc( 1, sizeof( qsv_filter_task_t ));

    task->processor.process = process_filter;
    task->processor.alloc   = &plugin->core->FrameAllocator;
    task->processor.core   = plugin->core;

    hb_list_add(plugin->tasks,task);

    plugin->is_init_done = 1;

    return sts;
}

mfxStatus plugin_close(qsv_filter_t* plugin){
    int i = 0;
    mfxStatus sts = MFX_ERR_NONE;

    if(!plugin->is_init_done) return sts;

    mfxExtOpaqueSurfaceAlloc* plugin_opaque_alloc = NULL;

    plugin_opaque_alloc = (mfxExtOpaqueSurfaceAlloc*) get_ext_buffer(plugin->videoparam->ExtParam,
                                    plugin->videoparam->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION);

    if(!plugin_opaque_alloc || !plugin_opaque_alloc->In.Surfaces || !plugin_opaque_alloc->Out.Surfaces)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    sts = plugin->core->UnmapOpaqueSurface(plugin->core->pthis, plugin_opaque_alloc->In.NumSurface,
            plugin_opaque_alloc->In.Type, plugin_opaque_alloc->In.Surfaces);
    AV_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, sts);


    sts = plugin->core->UnmapOpaqueSurface(plugin->core->pthis, plugin_opaque_alloc->Out.NumSurface,
            plugin_opaque_alloc->Out.Type, plugin_opaque_alloc->Out.Surfaces);
    AV_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    if(plugin->tasks){
        for(i=hb_list_count(plugin->tasks);i>0;i--){
            qsv_filter_task_t *task = hb_list_item(plugin->tasks,i-1);
            hb_list_rem(plugin->tasks,task);
            free(task);
        }
        hb_list_close(&plugin->tasks);
    }

    plugin->is_init_done = 0;

    return sts;
}

mfxExtBuffer* get_ext_buffer(mfxExtBuffer** buffers, mfxU32 buffers_num, mfxU32 buffer_id){
    int i = 0;
    if(!buffers) return 0;
    for(i=0;i<buffers_num;i++){
        if(!buffers[i]) continue;
        if(buffers[i]->BufferId == buffer_id)
            return buffers[i];
    }
    return 0;
}

int get_free_task(hb_list_t* tasks){
    int ret = -1;
    int i = 0;
    for(i=0;i<hb_list_count(tasks);i++){
        qsv_filter_task_t* task = hb_list_item(tasks,i);
        if(!task->busy){
            ret = i;
            break;
        }
    }
    return ret;
}

mfxStatus lock_frame(mfxFrameAllocator *alloc,mfxFrameSurface1 *surface){
    mfxStatus sts = MFX_ERR_NONE;
    // prevent double lock
    if (surface->Data.Y != 0 && surface->Data.MemId !=0){
        return MFX_ERR_UNSUPPORTED;
    }
    // not allocated, therefore no lock
    if (surface->Data.Y != 0){
        return MFX_ERR_NONE;
    }
    sts = alloc->Lock(alloc->pthis,surface->Data.MemId,&surface->Data);
    return sts;
}

mfxStatus unlock_frame(mfxFrameAllocator *alloc,mfxFrameSurface1 *surface){
    mfxStatus sts = MFX_ERR_NONE;
    // not allocated
    if (surface->Data.Y != 0 && surface->Data.MemId == 0){
        return MFX_ERR_NONE;
    }
    // not locked
    if (surface->Data.Y == 0){
        return MFX_ERR_NONE;
    }
    sts = alloc->Unlock(alloc->pthis,surface->Data.MemId,&surface->Data);
    return sts;
}


int process_filter(qsv_filter_task_t* task, void* params){
    mfxStatus sts = MFX_ERR_NONE;

    if (MFX_ERR_NONE != (sts = lock_frame(task->processor.alloc,task->in)))return sts;
    if (MFX_ERR_NONE != (sts = lock_frame(task->processor.alloc,task->out)))
    {
        unlock_frame(task->processor.alloc,task->in);
        return sts;
    }

    qsv_nv12_to_yuv420(task->pv->sws_context_from_nv12,task->pv->pre.out, task->in, task->processor.core);

    // signal: input is prepared, converted from pipeline into internal buffer
    hb_lock(task->pv->pre.frame_completed_lock);
    task->pv->pre.frame_go = 1;
    hb_cond_broadcast(task->pv->pre.frame_completed);
    hb_unlock(task->pv->pre.frame_completed_lock);

    // wait: input is prepared, converted from pipeline into internal buffer
    hb_lock(task->pv->post.frame_completed_lock);
    while(!task->pv->post.frame_go){
        hb_cond_timedwait(task->pv->post.frame_completed,task->pv->post.frame_completed_lock,1000);
        if(*task->pv->job->die)
            break;
    }
    task->pv->post.frame_go = 0;
    hb_unlock(task->pv->post.frame_completed_lock);

// this is just a simple fun/test case
#if 0
    {
        int i = 0;
        char *cur_line;
        char* luma =  task->pv->post.in->plane[0].data;
        int pitch =  task->pv->post.in->plane[0].stride;
        int h = task->pv->post.in->plane[0].height;
        int w = task->pv->post.in->plane[0].width;
        for (i = 0; i < h; i++){

            cur_line = luma + i * pitch;
            if(i>h/4 && i < 3*h/4 && i % 5 == 0 )
                memset(cur_line, 0 , w );
        }
    }
#endif

    if(task->pv->post.in)
    {
    qsv_yuv420_to_nv12(task->pv->sws_context_to_nv12, task->out, task->pv->post.in);
    }

    // signal: output is prepared, converted from internal buffer into pipeline
    hb_lock(task->pv->post_busy.frame_completed_lock);
    task->pv->post_busy.frame_go = 1;
    hb_cond_broadcast(task->pv->post_busy.frame_completed);
    hb_unlock(task->pv->post_busy.frame_completed_lock);

    unlock_frame(task->processor.alloc,task->in);
    unlock_frame(task->processor.alloc,task->out);

    return sts;
}

#endif // USE_QSV
