/* taskset.c

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/ports.h"
#include "handbrake/taskset.h"

static void taskset_thread_f( void *thread_args_v );

int
taskset_init( taskset_t *ts, const char *descr, int thread_count, size_t arg_size, thread_func_t *work_func)
{
    int init_step, i;

    init_step = 0;
    memset( ts, 0, sizeof( *ts ) );
    ts->work_func = work_func;
    ts->thread_count = thread_count;
    ts->task_descr = descr;

    ts->arg_size = arg_size;

    if( arg_size != 0 )
    {
        ts->task_threads_args = malloc( arg_size * ts->thread_count );
        if( ts->task_threads_args == NULL )
            goto fail;
    }
    /*
     * Initialize all arg data to 0.
     */
    memset(ts->task_threads_args, 0, ts->arg_size * ts->thread_count );

    init_step++;

    ts->task_thread_started = 0;
    ts->task_threads = calloc( ts->thread_count, sizeof( taskset_thread_t) );
    if( ts->task_threads == NULL )
        goto fail;

    init_step++;

    for ( i = 0; i < ts->thread_count; i++ ) {
        taskset_thread_t *thread = &ts->task_threads[i];

        thread->lock = hb_lock_init();
        if ( thread->lock == NULL )
            goto fail;

        thread->begin_cond = hb_cond_init();
        if ( thread->begin_cond == NULL )
            goto fail;

        thread->complete_cond = hb_cond_init();
        if ( thread->complete_cond == NULL )
            goto fail;
    }
    return (1);

fail:
    switch (init_step)
    {
        default:
        case 3:
            for ( i = 0; i < ts->thread_count; i++ ) {
                taskset_thread_t *thread = &ts->task_threads[i];
                if ( thread->begin_cond )
                    hb_cond_close( &thread->begin_cond );
                if ( thread->complete_cond )
                    hb_cond_close( &thread->complete_cond );
                if ( thread->lock )
                    hb_lock_close( &thread->lock );
            }
            /* FALL THROUGH */
        case 2:
            free( ts->task_threads );
            /* FALL THROUGH */
        case 1:
            free( ts->task_threads_args );
            /* FALL THROUGH */
        case 0:
            break;
    }
    return (0);
}

static taskset_thread_t*
taskset_thread( taskset_t *ts, int thr_idx )
{
    return &ts->task_threads[thr_idx];
}

void
taskset_cycle( taskset_t *ts )
{
    int i;
    if ( !ts->task_thread_started ) {
        for ( i = 0; i < ts->thread_count; i++ ) {
            taskset_thread_t *thread = taskset_thread( ts, i );
            thread->thread = hb_thread_init( ts->task_descr, taskset_thread_f,
                                           taskset_thread_args( ts, i ),
                                            HB_NORMAL_PRIORITY );
        }
        ts->task_thread_started = 1;
    }

    /*
     * Signal all threads that their work is available.
     */
    for (i = 0; i < ts->thread_count; i++) {
        taskset_thread_t *thread = taskset_thread( ts, i );
        hb_lock( thread->lock );
        thread->begin = 1;
        hb_cond_signal( thread->begin_cond );
        hb_unlock( thread->lock );
    }

    /*
     * Wait until all threads have completed.  Note that we must
     * loop here as hb_cond_wait() on some platforms (e.g pthread_cond_wait)
     * may unblock prematurely.
     */
    for ( i = 0; i < ts->thread_count; i++ )
    {
        taskset_thread_t *thread = taskset_thread( ts, i );
        hb_lock( thread->lock );
        while (!thread->complete) {
            hb_cond_wait( thread->complete_cond, thread->lock);
        }
        thread->complete = 0;
        hb_unlock( thread->lock );
    }
}

/*
 * Block current thread until work is available for it.
 */
static void
taskset_thread_wait4start( taskset_thread_t *thread )
{
    hb_lock( thread->lock );
    while ( !thread->begin )
    {
        hb_cond_wait( thread->begin_cond, thread->lock );
    }

    /*
     * We've been released for one run.  Insure we block the next
     * time through the loop.
     */
    thread->begin = 0;
    hb_unlock( thread->lock );
}

/*
 * Current thread has completed its work.  Indicate completion,
 * and if all threads in this task set have completed, wakeup
 * anyone waiting for this condition.
 */
static void
taskset_thread_complete( taskset_thread_t *thread )
{
    hb_lock( thread->lock );
    thread->complete = 1;
    hb_cond_signal( thread->complete_cond );
    hb_unlock( thread->lock );
}

static void
taskset_thread_f( void *thread_args_v )
{
    taskset_thread_arg_t *thread_args = thread_args_v;
    int segment = thread_args->segment;
    taskset_thread_t *thread = taskset_thread( thread_args->taskset, segment );

    while (1)
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( thread );

        if( thread->stop )
        {
            /*
             * No more work to do, exit this thread.
             */
            break;
        }

        thread_args->taskset->work_func( thread_args_v );

        taskset_thread_complete( thread );
    }

    /*
     * Finished this segment, let everyone know.
     */
    taskset_thread_complete( thread );
}


void
taskset_fini( taskset_t *ts )
{
    if (ts == NULL)
    {
        return;
    }

    int i;
    if ( ts->task_thread_started ) {
        /*
         * Tell each thread to stop, and then cleanup.
         */
        for ( i = 0; i < ts->thread_count; i++ )
        {
            taskset_thread_t *thread = taskset_thread( ts, i );
            hb_lock( thread->lock );
            thread->begin = 1;
            thread->stop = 1;
            hb_cond_signal( thread->begin_cond );
            while ( !thread->complete ) {
                hb_cond_wait( thread->complete_cond, thread->lock );
            }
            hb_unlock( thread->lock );
        }
        /*
         * Clean up thread memory.
         */
        for( i = 0; i < ts->thread_count; i++ )
        {
            taskset_thread_t *thread = taskset_thread( ts, i );
            hb_thread_close( &thread->thread );
        }
    }

    /*
     * Clean up taskset memory.
     */
    for( i = 0; i < ts->thread_count; i++ )
    {
        taskset_thread_t *thread = taskset_thread( ts, i );
        hb_lock_close( &thread->lock );
        hb_cond_close( &thread->begin_cond );
        hb_cond_close( &thread->complete_cond );
    }

    free( ts->task_threads );

    if( ts->task_threads_args != NULL )
        free( ts->task_threads_args );
}
