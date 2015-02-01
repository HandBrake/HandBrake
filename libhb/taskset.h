/* taskset.h

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HB_TASKSET_H
#define HB_TASKSET_H

#define TASKSET_POSIX_COMPLIANT 1

#include "bits.h"

typedef struct hb_taskset_s {
    int                thread_count;
    int                arg_size;
    int                bitmap_elements;
    hb_thread_t     ** task_threads;
    uint8_t          * task_threads_args;
    uint32_t         * task_begin_bitmap;    // Threads can begin
    uint32_t         * task_complete_bitmap; // Threads have completed
    uint32_t         * task_stop_bitmap;     // Threads should exit
    hb_lock_t        * task_cond_lock;       // Held during condition tests
    hb_cond_t        * task_begin;           // Threads can begin work
    hb_cond_t        * task_complete;        // Threads have finished work.
} taskset_t;

int taskset_init( taskset_t *, int /*thread_count*/, size_t /*user_arg_size*/ );
void taskset_cycle( taskset_t * );
void taskset_fini( taskset_t * );

int  taskset_thread_spawn( taskset_t *, int /*thr_idx*/, const char * /*descr*/,
                           thread_func_t *, int /*priority*/ );
void taskset_thread_wait4start( taskset_t *, int );
void taskset_thread_complete( taskset_t *, int );

static inline void *taskset_thread_args( taskset_t *, int );
static inline int   taskset_thread_stop( taskset_t *, int );

static inline void *
taskset_thread_args( taskset_t *ts, int thr_idx )
{
    return( ts->task_threads_args + ( ts->arg_size * thr_idx ) );
}

static inline int
taskset_thread_stop( taskset_t *ts, int thr_idx )
{
    return bit_is_set( ts->task_stop_bitmap, thr_idx );
}

#endif /* HB_TASKSET_H */
