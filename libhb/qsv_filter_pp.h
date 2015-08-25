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

#ifndef QSV_FILTER_PP_H
#define QSV_FILTER_PP_H

#include "msdk/mfxplugin.h"

struct qsv_filter_task_s;

typedef struct{
        mfxFrameAllocator *alloc;
        mfxStatus             (*process)(struct qsv_filter_task_s*,void*);
        mfxCoreInterface    *core;
}qsv_filter_processor_t;

typedef  struct qsv_filter_task_s{
        mfxFrameSurface1 *in;
        mfxFrameSurface1 *out;
        int              busy;
        hb_filter_private_t *pv;
        qsv_filter_processor_t processor;
} qsv_filter_task_t;

typedef struct qsv_filter_private_s{

        int                 is_init_done;

        mfxCoreInterface    *core;
        mfxVideoParam       *videoparam;
        mfxPluginParam      pluginparam;

        hb_filter_private_t *pv;

        mfxPlugin           plug;
        hb_list_t           *tasks;
} qsv_filter_t;

typedef struct hb_qsv_sync_s{
    int                 frame_go;
    int                 status;
    hb_cond_t           *frame_completed;
    hb_lock_t           *frame_completed_lock;

    hb_buffer_t         *in;
    hb_buffer_t         *out;
} hb_qsv_sync_t;

typedef struct hb_filter_private_s
{
    hb_job_t            *job;
    hb_list_t           *list;

    hb_qsv_sync_t       pre;
    hb_qsv_sync_t       pre_busy;

    hb_qsv_sync_t       post;
    hb_qsv_sync_t       post_busy;

    av_qsv_space        *vpp_space;
    hb_list_t           *qsv_user;

    struct SwsContext* sws_context_to_nv12;
    struct SwsContext* sws_context_from_nv12;
} hb_filter_private_t_qsv;

// methods to be called by Media SDK
mfxStatus MFX_CDECL qsv_PluginInit(mfxHDL pthis, mfxCoreInterface *core);
mfxStatus MFX_CDECL qsv_PluginClose (mfxHDL pthis);
mfxStatus MFX_CDECL qsv_GetPluginParam(mfxHDL pthis, mfxPluginParam *par);
mfxStatus MFX_CDECL qsv_Submit(mfxHDL pthis, const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task);
mfxStatus MFX_CDECL qsv_Execute(mfxHDL pthis, mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a);
mfxStatus MFX_CDECL qsv_FreeResources(mfxHDL pthis, mfxThreadTask task, mfxStatus sts);

// methods to be called by us
mfxStatus plugin_init(qsv_filter_t*,mfxVideoParam*);
mfxStatus plugin_close(qsv_filter_t*);

//internal functions
mfxExtBuffer* get_ext_buffer(mfxExtBuffer**, mfxU32, mfxU32);
int get_free_task(hb_list_t*);
mfxStatus           process_filter(qsv_filter_task_t*,void*);
mfxStatus lock_frame(mfxFrameAllocator *,mfxFrameSurface1*);
mfxStatus unlock_frame(mfxFrameAllocator *,mfxFrameSurface1*);


#endif //QSV_FILTER_PP_H
