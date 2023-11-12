/* taskset.h

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_TASKSET_H
#define HANDBRAKE_TASKSET_H

#define TASKSET_POSIX_COMPLIANT 1

typedef struct hb_tasket_thread_t {
    hb_thread_t      * thread;
    hb_lock_t        * lock;
    hb_cond_t        * begin_cond;
    hb_cond_t        * complete_cond;
    int                begin;
    int                complete;
    int                stop;
} taskset_thread_t;

typedef struct hb_taskset_s {
    int                thread_count;
    thread_func_t    * work_func;
    int                arg_size;
    const char       * task_descr;
    uint8_t          * task_threads_args;
    int                task_thread_started;
    taskset_thread_t * task_threads;
} taskset_t;

typedef struct hb_taskset_thread_arg_s {
    taskset_t *taskset;
    int segment;
} taskset_thread_arg_t;

int taskset_init( taskset_t *, const char* /* descr */, int /*thread_count*/, size_t /*user_arg_size*/, thread_func_t *);
void taskset_cycle( taskset_t * );
void taskset_fini( taskset_t * );

static inline void *taskset_thread_args( taskset_t *, int );

static inline void *
taskset_thread_args( taskset_t *ts, int thr_idx )
{
    return( ts->task_threads_args + ( ts->arg_size * thr_idx ) );
}

#endif /* HANDBRAKE_TASKSET_H */
