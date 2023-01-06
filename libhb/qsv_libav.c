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

#include "handbrake/hbffmpeg.h"
#include "handbrake/qsv_libav.h"

int hb_qsv_get_free_encode_task(hb_qsv_list * tasks)
{
    int ret = MFX_ERR_NOT_FOUND;
    int i = 0;
    if (tasks)
        for (i = 0; i < hb_qsv_list_count(tasks); i++) {
            hb_qsv_task *task = hb_qsv_list_item(tasks, i);
            if (task->stage && task->stage->out.sync)
                if (!(*task->stage->out.sync->p_sync)) {
                    ret = i;
                    break;
                }
        }
    return ret;
}

int hb_qsv_get_free_sync(hb_qsv_space * space, hb_qsv_context * qsv)
{
    int ret = -1;
    int counter = 0;

    while (1) {
        for (int i = 0; i < space->sync_num; i++) {
            if (!(*(space->p_syncp[i]->p_sync)) &&
                0 == space->p_syncp[i]->in_use ) {
                if (i > space->sync_num_max_used)
                    space->sync_num_max_used = i;
                ff_qsv_atomic_inc(&space->p_syncp[i]->in_use);
                return i;
            }
        }
        if (++counter >= HB_QSV_REPEAT_NUM_DEFAULT) {
            hb_error("QSV: not enough to have %d sync point(s) allocated", space->sync_num);
            break;
        }
        hb_qsv_sleep(5);
    }
    return ret;
}

int hb_qsv_get_free_surface(hb_qsv_space * space, hb_qsv_context * qsv,
                     mfxFrameInfo * info, hb_qsv_split part)
{
    int ret = -1;
    int from = 0;
    int up = space->surface_num;
    int counter = 0;

    while (1) {
        from = 0;
        up = space->surface_num;
        if (part == QSV_PART_LOWER)
            up /= 2;
        if (part == QSV_PART_UPPER)
            from = up / 2;

        for (int i = from; i < up; i++) {
            if (0 == space->p_surfaces[i]->Data.Locked) {
                memcpy(&(space->p_surfaces[i]->Info), info,
                       sizeof(mfxFrameInfo));
                if (i > space->surface_num_max_used)
                    space->surface_num_max_used = i;
                return i;
            }
        }
        if (++counter >= HB_QSV_REPEAT_NUM_DEFAULT) {
            hb_error("QSV: not enough to have %d surface(s) allocated", up);
            break;
        }
        hb_qsv_sleep(5);
    }
    return ret;
}

int ff_qsv_is_surface_in_pipe(mfxFrameSurface1 * p_surface, hb_qsv_context * qsv)
{
    int ret = 0;
    int a, b;
    hb_qsv_list *list = 0;
    hb_qsv_stage *stage = 0;

    if (!p_surface)
        return ret;
    if (!qsv->pipes)
        return ret;

    for (a = 0; a < hb_qsv_list_count(qsv->pipes); a++) {
        list = hb_qsv_list_item(qsv->pipes, a);
        for (b = 0; b < hb_qsv_list_count(list); b++) {
            stage = hb_qsv_list_item(list, b);
            if (p_surface == stage->out.p_surface)
                return (stage->type << 16) | 2;
            if (p_surface == stage->in.p_surface)
                return (stage->type << 16) | 1;
        }
    }
    return ret;
}

int ff_qsv_is_sync_in_pipe(mfxSyncPoint * sync, hb_qsv_context * qsv)
{
    int ret = 0;
    int a, b;
    hb_qsv_list *list = 0;
    hb_qsv_stage *stage = 0;

    if (!sync)
        return ret;
    if (!qsv->pipes)
        return ret;

    for (a = 0; a < hb_qsv_list_count(qsv->pipes); a++) {
        list = hb_qsv_list_item(qsv->pipes, a);
        for (b = 0; b < hb_qsv_list_count(list); b++) {
            stage = hb_qsv_list_item(list, b);
            if (sync == stage->out.sync->p_sync) {
                return 1;
            }
        }
    }
    return ret;
}

hb_qsv_stage *hb_qsv_stage_init(void)
{
    hb_qsv_stage *stage = av_mallocz(sizeof(hb_qsv_stage));
    return stage;
}

void hb_qsv_stage_clean(hb_qsv_stage ** stage, int is_clean_content)
{
    if (is_clean_content) {
        if ((*stage)->out.sync) {
            if ((*stage)->out.sync->p_sync)
            {
                *(*stage)->out.sync->p_sync = 0;
            }
            if ((*stage)->out.sync->in_use > 0)
            {
                ff_qsv_atomic_dec(&(*stage)->out.sync->in_use);
            }
            (*stage)->out.sync = 0;
        }
        if ((*stage)->out.p_surface) {
            (*stage)->out.p_surface = 0;

        }
        if ((*stage)->in.p_surface) {
            (*stage)->in.p_surface = 0;
        }
    }
    av_freep(stage);
}

void hb_qsv_add_context_usage(hb_qsv_context * qsv, int is_threaded)
{
    int is_active = 0;
    int mut_ret = 0;

    is_active = ff_qsv_atomic_inc(&qsv->is_context_active);
    if (is_active == 1) {
        memset(&qsv->mfx_session, 0, sizeof(mfxSession));
        hb_qsv_pipe_list_create(&qsv->pipes, is_threaded);

        qsv->dts_seq = hb_qsv_list_init(is_threaded);

        if (is_threaded) {
            qsv->qts_seq_mutex = av_mallocz(sizeof(pthread_mutex_t));
            if (qsv->qts_seq_mutex){
                mut_ret = pthread_mutex_init(qsv->qts_seq_mutex, NULL);
                if(mut_ret)
                    hb_log("QSV: pthread_mutex_init issue[%d] at %s", mut_ret, __FUNCTION__);
            }

        } else
            qsv->qts_seq_mutex = 0;
    }
}

int hb_qsv_context_clean(hb_qsv_context * qsv, int full_job)
{
    int is_active = 0;
    mfxStatus sts = MFX_ERR_NONE;
    int mut_ret = 0;

    is_active = ff_qsv_atomic_dec(&qsv->is_context_active);

    // spaces would have to be cleaned on the own,
    // here we care about the rest, common stuff
    if (is_active == 0) {

        if (qsv->dts_seq) {
            while (hb_qsv_list_count(qsv->dts_seq))
                hb_qsv_dts_pop(qsv);

            hb_qsv_list_close(&qsv->dts_seq);
        }
        if (qsv->qts_seq_mutex) {
            mut_ret = pthread_mutex_destroy(qsv->qts_seq_mutex);
            if(mut_ret)
                hb_log("QSV: pthread_mutex_destroy issue[%d] at %s", mut_ret, __FUNCTION__);
            qsv->qts_seq_mutex = 0;
        }

        if (qsv->pipes)
            hb_qsv_pipe_list_clean(&qsv->pipes);

        if (qsv->mfx_session && !full_job) {
            sts = MFXClose(qsv->mfx_session);
            HB_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
            qsv->mfx_session = 0;
            // display must be closed after MFXClose
            hb_display_close(&qsv->display);
            qsv->display = NULL;
        }
    }
    return 0;
}

void hb_qsv_pipe_list_create(hb_qsv_list ** list, int is_threaded)
{
    if (!*list)
        *list = hb_qsv_list_init(is_threaded);
}

void hb_qsv_pipe_list_clean(hb_qsv_list ** list)
{
    hb_qsv_list *stage;
    int i = 0;

    if (*list) {
        for (i = hb_qsv_list_count(*list); i > 0; i--) {
            stage = hb_qsv_list_item(*list, i - 1);
            hb_qsv_flush_stages(*list, &stage, 0);
        }
        hb_qsv_list_close(list);
    }
}

void hb_qsv_add_stagee(hb_qsv_list ** list, hb_qsv_stage * stage, int is_threaded)
{
    if (!*list)
        *list = hb_qsv_list_init(is_threaded);
    hb_qsv_list_add(*list, stage);
}

hb_qsv_stage *hb_qsv_get_last_stage(hb_qsv_list * list)
{
    hb_qsv_stage *stage = 0;
    int size = 0;

    hb_qsv_list_lock(list);
    size = hb_qsv_list_count(list);
    if (size > 0)
        stage = hb_qsv_list_item(list, size - 1);
    hb_qsv_list_unlock(list);

    return stage;
}

void hb_qsv_flush_stages(hb_qsv_list * list, hb_qsv_list ** item, int is_flush_content)
{
    int i = 0;
    int x = 0;
    hb_qsv_stage *stage = 0;
    hb_qsv_list *to_remove_list = 0;
    hb_qsv_list *to_remove_atom_list = 0;
    hb_qsv_list *to_remove_atom = 0;

    for (i = 0; i < hb_qsv_list_count(*item); i++) {
        stage = hb_qsv_list_item(*item, i);
        if(stage->pending){
            if(!to_remove_list)
                to_remove_list = hb_qsv_list_init(0);
            hb_qsv_list_add(to_remove_list, stage->pending);
        }
        hb_qsv_stage_clean(&stage, is_flush_content);
        // should actually remove from the list but ok...
    }
    hb_qsv_list_rem(list, *item);
    hb_qsv_list_close(item);

    if(to_remove_list){
        for (i = hb_qsv_list_count(to_remove_list); i > 0; i--){
            to_remove_atom_list = hb_qsv_list_item(to_remove_list, i-1);
            for (x = hb_qsv_list_count(to_remove_atom_list); x > 0; x--){
                to_remove_atom = hb_qsv_list_item(to_remove_atom_list, x-1);
                hb_qsv_flush_stages(list, &to_remove_atom, is_flush_content);
            }
        }
        hb_qsv_list_close(&to_remove_list);
    }
}

hb_qsv_list *hb_qsv_pipe_by_stage(hb_qsv_list * list, hb_qsv_stage * stage)
{
    hb_qsv_list *item = 0;
    hb_qsv_stage *cur_stage = 0;
    int i = 0;
    int a = 0;
    for (i = 0; i < hb_qsv_list_count(list); i++) {
        item = hb_qsv_list_item(list, i);
        for (a = 0; a < hb_qsv_list_count(item); a++) {
            cur_stage = hb_qsv_list_item(item, a);
            if (cur_stage == stage)
                return item;
        }
    }
    return 0;
}

// no duplicate of the same value, if end == 0 : working over full length
void hb_qsv_dts_ordered_insert(hb_qsv_context * qsv, int start, int end,
                            int64_t dts, int iter)
{
    hb_qsv_dts *cur_dts = 0;
    hb_qsv_dts *new_dts = 0;
    int i = 0;
    int mut_ret = 0;


    if (iter == 0 && qsv->qts_seq_mutex){
        mut_ret = pthread_mutex_lock(qsv->qts_seq_mutex);
        if(mut_ret)
            hb_log("QSV: pthread_mutex_lock issue[%d] at %s", mut_ret, __FUNCTION__);
    }

    if (end == 0)
        end = hb_qsv_list_count(qsv->dts_seq);

    if (end <= start) {
        new_dts = av_mallocz(sizeof(hb_qsv_dts));
        if( new_dts ) {
            new_dts->dts = dts;
            hb_qsv_list_add(qsv->dts_seq, new_dts);
        }
    } else
        for (i = end; i > start; i--) {
            cur_dts = hb_qsv_list_item(qsv->dts_seq, i - 1);
            if (cur_dts->dts < dts) {
                new_dts = av_mallocz(sizeof(hb_qsv_dts));
                if( new_dts ) {
                    new_dts->dts = dts;
                    hb_qsv_list_insert(qsv->dts_seq, i, new_dts);
                }
                break;
            } else if (cur_dts->dts == dts)
                break;
        }
    if (iter == 0 && qsv->qts_seq_mutex){
        mut_ret = pthread_mutex_unlock(qsv->qts_seq_mutex);
        if(mut_ret)
            hb_log("QSV: pthread_mutex_unlock issue[%d] at %s", mut_ret, __FUNCTION__);
    }
}

void hb_qsv_dts_pop(hb_qsv_context * qsv)
{
    hb_qsv_dts *item = 0;
    int mut_ret = 0;

    if (qsv && qsv->qts_seq_mutex){
        mut_ret = pthread_mutex_lock(qsv->qts_seq_mutex);
        if(mut_ret)
            hb_log("QSV: pthread_mutex_lock issue[%d] at %s", mut_ret, __FUNCTION__);
    }

    if (hb_qsv_list_count(qsv->dts_seq)) {
        item = hb_qsv_list_item(qsv->dts_seq, 0);
        hb_qsv_list_rem(qsv->dts_seq, item);
        av_free(item);
    }
    if (qsv && qsv->qts_seq_mutex){
        mut_ret = pthread_mutex_unlock(qsv->qts_seq_mutex);
        if(mut_ret)
            hb_log("QSV: pthread_mutex_lock issue[%d] at %s", mut_ret, __FUNCTION__);
        }
}


hb_qsv_list *hb_qsv_list_init(int is_threaded)
{
    hb_qsv_list *l;
    int mut_ret;

    l = av_mallocz(sizeof(hb_qsv_list));
    if (!l)
        return 0;
    l->items = av_mallocz(HB_QSV_JOB_SIZE_DEFAULT * sizeof(void *));
    if (!l->items)
        return 0;
    l->items_alloc = HB_QSV_JOB_SIZE_DEFAULT;

    if (is_threaded) {
        l->mutex = av_mallocz(sizeof(pthread_mutex_t));
        if (l->mutex){
            mut_ret = pthread_mutexattr_init(&l->mta);
            if( mut_ret )
                hb_log("QSV: pthread_mutexattr_init issue[%d] at %s", mut_ret, __FUNCTION__);
            mut_ret = pthread_mutexattr_settype(&l->mta, PTHREAD_MUTEX_RECURSIVE /*PTHREAD_MUTEX_ERRORCHECK*/);
            if( mut_ret )
                hb_log("QSV: pthread_mutexattr_settype issue[%d] at %s", mut_ret, __FUNCTION__);
            mut_ret = pthread_mutex_init(l->mutex, &l->mta);
            if( mut_ret )
                hb_log("QSV: pthread_mutex_init issue[%d] at %s", mut_ret, __FUNCTION__);
        }
    } else
        l->mutex = 0;
    return l;
}

int hb_qsv_list_count(hb_qsv_list * l)
{
    int count;

    hb_qsv_list_lock(l);
    count = l->items_count;
    hb_qsv_list_unlock(l);
    return count;
}

int hb_qsv_list_add(hb_qsv_list * l, void *p)
{
    int pos = -1;

    if (!p) {
        return pos;
    }

    hb_qsv_list_lock(l);

    if (l->items_count == l->items_alloc) {
        /* We need a bigger boat */
        l->items_alloc += HB_QSV_JOB_SIZE_DEFAULT;
        l->items = av_realloc(l->items, l->items_alloc * sizeof(void *));
    }

    l->items[l->items_count] = p;
    pos = (l->items_count);
    l->items_count++;

    hb_qsv_list_unlock(l);

    return pos;
}

void hb_qsv_list_rem(hb_qsv_list * l, void *p)
{
    int i;

    hb_qsv_list_lock(l);

    /* Find the item in the list */
    for (i = 0; i < l->items_count; i++) {
        if (l->items[i] == p) {
            /* Shift all items after it sizeof( void * ) bytes earlier */
            memmove(&l->items[i], &l->items[i + 1],
                    (l->items_count - i - 1) * sizeof(void *));

            l->items_count--;
            break;
        }
    }

    hb_qsv_list_unlock(l);
}

void *hb_qsv_list_item(hb_qsv_list * l, int i)
{
    void *ret = NULL;

    if (i < 0)
        return NULL;

    hb_qsv_list_lock(l);
    if( i < l->items_count)
        ret = l->items[i];
    hb_qsv_list_unlock(l);
    return ret;
}

void hb_qsv_list_insert(hb_qsv_list * l, int pos, void *p)
{

    if (!p)
        return;

    hb_qsv_list_lock(l);

    if (l->items_count == l->items_alloc) {
        l->items_alloc += HB_QSV_JOB_SIZE_DEFAULT;
        l->items = av_realloc(l->items, l->items_alloc * sizeof(void *));
    }

    if (l->items_count != pos) {
        memmove(&l->items[pos + 1], &l->items[pos],
                (l->items_count - pos) * sizeof(void *));
    }

    l->items[pos] = p;
    l->items_count--;

    hb_qsv_list_unlock(l);
}

void hb_qsv_list_close(hb_qsv_list ** _l)
{
    hb_qsv_list *l = *_l;
    int mut_ret;

    hb_qsv_list_lock(l);

    av_free(l->items);

    if (l->mutex){
        mut_ret = pthread_mutex_unlock(l->mutex);
        if (mut_ret)
            hb_log("QSV: pthread_mutex_unlock issue[%d] at %s", mut_ret, __FUNCTION__);
        mut_ret = pthread_mutex_destroy(l->mutex);
        if (mut_ret)
            hb_log("QSV: pthread_mutex_destroy issue[%d] at %s", mut_ret, __FUNCTION__);
        mut_ret = pthread_mutexattr_destroy(&l->mta);
        if (mut_ret)
            hb_log("QSV: pthread_mutexattr_destroy issue[%d] at %s", mut_ret, __FUNCTION__);
    }
    av_freep(_l);
}

int hb_qsv_list_lock(hb_qsv_list *l){
    int ret = 0;
    if (l->mutex){
        ret = pthread_mutex_lock(l->mutex);
        if( ret )
            hb_log("QSV: pthread_mutex_lock issue[%d] at %s", ret, __FUNCTION__);
    }
    return ret;
}

int hb_qsv_list_unlock(hb_qsv_list *l){
    int ret = 0;
    if (l->mutex){
        ret = pthread_mutex_unlock(l->mutex);
        if( ret )
            hb_log("QSV: pthread_mutex_unlock issue[%d] at %s", ret, __FUNCTION__);
    }
    return ret;
}

int av_is_qsv_available(mfxIMPL impl, mfxVersion * ver)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxSession mfx_session;

    memset(&mfx_session, 0, sizeof(mfxSession));
    sts = MFXInit(impl, ver, &mfx_session);
    if (sts >= 0)
        MFXClose(mfx_session);
    return sts;
}

int hb_qsv_wait_on_sync(hb_qsv_context *qsv, hb_qsv_stage *stage)
{
    int iter = 0;
    mfxStatus sts = MFX_ERR_NONE;
    if( stage )
        if(*stage->out.sync->p_sync){
            while(1){
                iter++;
                sts = MFXVideoCORE_SyncOperation(qsv->mfx_session,*stage->out.sync->p_sync, HB_QSV_SYNC_TIME_DEFAULT);
                if(MFX_WRN_IN_EXECUTION == sts){

                    if(iter>20)
                        HB_QSV_DEBUG_ASSERT(1, "Sync failed");

                    hb_qsv_sleep(10);
                    continue;
                }
                HB_QSV_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
                break;
            }
        }
    return 0;
}

#endif // HB_PROJECT_FEATURE_QSV
