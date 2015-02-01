/* taskset.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "ports.h"
#include "taskset.h"

int
taskset_init( taskset_t *ts, int thread_count, size_t arg_size )
{
    int init_step;

    init_step = 0;
    memset( ts, 0, sizeof( *ts ) );
    ts->thread_count = thread_count;
    ts->arg_size = arg_size;
    ts->bitmap_elements = ( ts->thread_count + 31 ) / 32;
    ts->task_threads = malloc( sizeof( hb_thread_t* ) * ts->thread_count );
    if( ts->task_threads == NULL )
        goto fail;
    init_step++;

    if( arg_size != 0 )
    {
        ts->task_threads_args = malloc( arg_size * ts->thread_count );
        if( ts->task_threads == NULL )
            goto fail;
    }
    init_step++;

    ts->task_begin_bitmap = malloc( sizeof( uint32_t  ) * ts->bitmap_elements );
    if( ts->task_begin_bitmap == NULL )
        goto fail;
    init_step++;

    ts->task_complete_bitmap = malloc( sizeof( uint32_t ) * ts->bitmap_elements );
    if( ts->task_complete_bitmap == NULL )
        goto fail;
    init_step++;

    ts->task_stop_bitmap = malloc( sizeof( uint32_t ) * ts->bitmap_elements );
    if( ts->task_stop_bitmap == NULL )
        goto fail;
    init_step++;

    ts->task_cond_lock = hb_lock_init();
    if( ts->task_cond_lock == NULL)
        goto fail;
    init_step++;

    ts->task_begin = hb_cond_init();
    if( ts->task_begin == NULL)
        goto fail;
    init_step++;

    ts->task_complete = hb_cond_init();
    if( ts->task_complete == NULL)
        goto fail;
    init_step++;

    /*
     * Initialize all arg data to 0.
     */
    memset(ts->task_threads_args, 0, ts->arg_size * ts->thread_count );

    /*
     * Inialize bitmaps to all bits set.  This means that any unused bits
     * in the bitmap are already in the "condition satisfied" state allowing
     * us to test the bitmap 32bits at a time without having to mask off
     * the end.
     */
    memset(ts->task_begin_bitmap, 0xFF, sizeof( uint32_t ) * ts->bitmap_elements );
    memset(ts->task_complete_bitmap, 0xFF, sizeof( uint32_t ) * ts->bitmap_elements );
    memset(ts->task_stop_bitmap, 0, sizeof( uint32_t ) * ts->bitmap_elements );
    
    /*
     * Important to start off with the threads locked waiting
     * on input, no work completed, and not asked to stop.
     */
    bit_nclear( ts->task_begin_bitmap, 0, ts->thread_count - 1 );
    bit_nclear( ts->task_complete_bitmap, 0, ts->thread_count - 1 );
    bit_nclear( ts->task_stop_bitmap, 0, ts->thread_count - 1 );
    return (1);

fail:
    switch (init_step)
    {
        default:
            hb_cond_close( &ts->task_complete );
            /* FALL THROUGH */
        case 7:
            hb_cond_close( &ts->task_begin );
            /* FALL THROUGH */
        case 6:
            hb_lock_close( &ts->task_cond_lock );
            /* FALL THROUGH */
        case 5:
            free( ts->task_stop_bitmap );
            /* FALL THROUGH */
        case 4:
            free( ts->task_complete_bitmap );
            /* FALL THROUGH */
        case 3:
            free( ts->task_begin_bitmap );
            /* FALL THROUGH */
        case 2:
            if( ts->task_threads_args == NULL )
                free( ts->task_threads_args );
            /* FALL THROUGH */
        case 1:
            free( ts->task_threads );
            /* FALL THROUGH */
        case 0:
            break;
    }
    return (0);
}

int
taskset_thread_spawn( taskset_t *ts, int thr_idx, const char *descr,
                      thread_func_t *func, int priority )
{
    ts->task_threads[thr_idx] = hb_thread_init( descr, func,
                                                taskset_thread_args( ts, thr_idx ),
                                                priority);
    return( ts->task_threads[thr_idx] != NULL );
}

void
taskset_cycle( taskset_t *ts )
{
    hb_lock( ts->task_cond_lock );

    /*
     * Signal all threads that their work is available.
     */
    bit_nset( ts->task_begin_bitmap, 0, ts->thread_count - 1 );
    hb_cond_broadcast( ts->task_begin );

    /*
     * Wait until all threads have completed.  Note that we must
     * loop here as hb_cond_wait() on some platforms (e.g pthead_cond_wait)
     * may unblock prematurely.
     */
    do
    {
        hb_cond_wait( ts->task_complete, ts->task_cond_lock );
    } while ( !allbits_set( ts->task_complete_bitmap, ts->bitmap_elements ) );

    /*
     * Clear completion indications for next time.
     */
    bit_nclear( ts->task_complete_bitmap, 0, ts->thread_count - 1 );

    hb_unlock( ts->task_cond_lock );
}

/*
 * Block current thread until work is available for it.
 */
void
taskset_thread_wait4start( taskset_t *ts, int thr_idx )
{
    hb_lock( ts->task_cond_lock );
    while ( bit_is_clear( ts->task_begin_bitmap, thr_idx ) )
        hb_cond_wait( ts->task_begin, ts->task_cond_lock );

    /*
     * We've been released for one run.  Insure we block the next
     * time through the loop.
     */
    bit_clear( ts->task_begin_bitmap, thr_idx );
    hb_unlock( ts->task_cond_lock );
}

/*
 * Current thread has completed its work.  Indicate completion,
 * and if all threads in this task set have completed, wakeup
 * anyone waiting for this condition.
 */
void
taskset_thread_complete( taskset_t *ts, int thr_idx )
{
    hb_lock( ts->task_cond_lock );
    bit_set( ts->task_complete_bitmap, thr_idx );
    if( allbits_set( ts->task_complete_bitmap, ts->bitmap_elements ) )
    {
        hb_cond_signal( ts->task_complete );
    }
    hb_unlock( ts->task_cond_lock );
}

void
taskset_fini( taskset_t *ts )
{
    int i;

    hb_lock( ts->task_cond_lock );
    /*
     * Tell each thread to stop, and then cleanup.
     */
    bit_nset( ts->task_stop_bitmap, 0, ts->thread_count - 1 );
    bit_nset( ts->task_begin_bitmap, 0, ts->thread_count - 1 );
    hb_cond_broadcast( ts->task_begin );

    /*
     * Wait for all threads to exit.
     */
    hb_cond_wait( ts->task_complete, ts->task_cond_lock );
    hb_unlock( ts->task_cond_lock );

    /*
     * Clean up taskset memory.
     */
    for( i = 0; i < ts->thread_count; i++)
    {
        hb_thread_close( &ts->task_threads[i] );
    }
    hb_lock_close( &ts->task_cond_lock );
    hb_cond_close( &ts->task_begin );
    hb_cond_close( &ts->task_complete );
    free( ts->task_threads );
    if( ts->task_threads_args != NULL )
        free( ts->task_threads_args );
    free( ts->task_begin_bitmap );
    free( ts->task_complete_bitmap );
    free( ts->task_stop_bitmap );
}
