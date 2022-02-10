/* grayscale.c

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/taskset.h"

// Settings:
//  This filter has no settings.
//  But at some point it might be interesting to add effects other than
//  just gray.

typedef struct grayscale_arguments_s {
    hb_buffer_t *src;
} grayscale_arguments_t;

struct hb_filter_private_s
{
    int                    cpu_count;

    taskset_t              grayscale_taskset;   // Threads - one per CPU
    grayscale_arguments_t *grayscale_arguments; // Arguments to thread for work
};

static int hb_grayscale_init( hb_filter_object_t * filter,
                              hb_filter_init_t * init );

static int hb_grayscale_work( hb_filter_object_t * filter,
                              hb_buffer_t ** buf_in,
                              hb_buffer_t ** buf_out );

static void hb_grayscale_close( hb_filter_object_t * filter );

static hb_filter_info_t * hb_grayscale_info( hb_filter_object_t * filter );

hb_filter_object_t hb_filter_grayscale =
{
    .id            = HB_FILTER_GRAYSCALE,
    .enforce_order = 1,
    .name          = "Grayscale",
    .settings      = NULL,
    .init          = hb_grayscale_init,
    .work          = hb_grayscale_work,
    .close         = hb_grayscale_close,
    .info          = hb_grayscale_info
};


typedef struct grayscale_thread_arg_s {
    taskset_thread_arg_t arg;
    hb_filter_private_t *pv;
} grayscale_thread_arg_t;

void grayscale_filter_work( void *thread_args_v)
{
    grayscale_arguments_t *grayscale_work = NULL;
    int plane;
    int segment, segment_start, segment_stop;
    grayscale_thread_arg_t *thread_args = thread_args_v;
    hb_buffer_t *src_buf;

    hb_filter_private_t * pv = thread_args->pv;
    segment = thread_args->arg.segment;

    grayscale_work = &pv->grayscale_arguments[segment];
    if (grayscale_work->src == NULL)
    {
        hb_error( "Thread started when no work available" );
        return;
    }

    /*
     * Process all three planes, but only this segment of it.
     */
    src_buf = grayscale_work->src;
    for (plane = 1; plane < 3; plane++)
    {
        int src_stride = src_buf->plane[plane].stride;
        int height     = src_buf->plane[plane].height;
        segment_start = (height / pv->cpu_count) * segment;
        if (segment == pv->cpu_count - 1)
        {
            /*
             * Final segment
             */
            segment_stop = height;
        } else {
            segment_stop = (height / pv->cpu_count) * (segment + 1);
        }

        memset(&src_buf->plane[plane].data[segment_start * src_stride],
               0x80, (segment_stop - segment_start) * src_stride);
    }
}

/*
 * threaded gray - each thread grays a single segment of all
 * three planes. Where a segment is defined as the frame divided by
 * the number of CPUs.
 *
 * This function blocks until the frame is grayed.
 */
static void grayscale_filter( hb_filter_private_t * pv,
                              hb_buffer_t         * in )
{

    int segment;

    for( segment = 0; segment < pv->cpu_count; segment++ )
    {
        /*
         * Setup the work for this plane.
         */
        pv->grayscale_arguments[segment].src = in;
    }

    /*
     * Allow the taskset threads to make one pass over the data.
     */
    taskset_cycle( &pv->grayscale_taskset );

    /*
     * Entire frame is now grayed.
     */
}


static int hb_grayscale_init( hb_filter_object_t * filter,
                              hb_filter_init_t   * init )
{
    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;

    pv->cpu_count = hb_get_cpu_count();

    /*
     * Create gray taskset.
     */
    pv->grayscale_arguments = malloc(sizeof(grayscale_arguments_t) *
                                     pv->cpu_count);
    if (pv->grayscale_arguments == NULL ||
        taskset_init( &pv->grayscale_taskset, "grayscale_filter_segment", pv->cpu_count,
                      sizeof( grayscale_thread_arg_t ), grayscale_filter_work) == 0)
    {
        hb_error( "grayscale could not initialize taskset" );
    }

    int ii;
    for (ii = 0; ii < pv->cpu_count; ii++)
    {
        grayscale_thread_arg_t *thread_args;

        thread_args = taskset_thread_args(&pv->grayscale_taskset, ii);

        thread_args->pv = pv;
        thread_args->arg.taskset = &pv->grayscale_taskset;
        thread_args->arg.segment = ii;
        pv->grayscale_arguments[ii].src = NULL;
    }

    return 0;
}

static hb_filter_info_t * hb_grayscale_info( hb_filter_object_t * filter )
{
    return NULL;
}

static void hb_grayscale_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if( !pv )
    {
        return;
    }

    taskset_fini( &pv->grayscale_taskset );

    /*
     * free memory for grayscale structs
     */
    free( pv->grayscale_arguments );

    free( pv );
    filter->private_data = NULL;
}

static int hb_grayscale_work( hb_filter_object_t * filter,
                              hb_buffer_t ** buf_in,
                              hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;

    *buf_in = NULL;
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        return HB_FILTER_DONE;
    }

    // Grayscale!
    grayscale_filter(pv, in);

    *buf_out = in;

    return HB_FILTER_OK;
}
