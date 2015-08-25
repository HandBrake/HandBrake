/*
 Copyright (C) 2006 Michael Niedermayer <michaelni@gmx.at>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "hb.h"
#include "hbffmpeg.h"
#include "taskset.h"

// yadif_mode is a bit vector with the following flags
#define MODE_YADIF_ENABLE       1
#define MODE_YADIF_SPATIAL      2
#define MODE_YADIF_2PASS        4
#define MODE_YADIF_BOB          8

#define YADIF_MODE_DEFAULT      0
#define YADIF_PARITY_DEFAULT   -1

#define ABS(a) ((a) > 0 ? (a) : (-(a)))
#define MIN3(a,b,c) MIN(MIN(a,b),c)
#define MAX3(a,b,c) MAX(MAX(a,b),c)

typedef struct yadif_arguments_s {
    hb_buffer_t * dst;
    int parity;
    int tff;
} yadif_arguments_t;

typedef struct deint_arguments_s {
    hb_buffer_t * src;
    hb_buffer_t * dst;
} deint_arguments_t;

typedef struct deint_thread_arg_s {
    hb_filter_private_t *pv;
    int segment;
} deint_thread_arg_t;

struct hb_filter_private_s
{
    int              width;
    int              height;

    int              yadif_mode;
    int              yadif_parity;
    int              yadif_ready;

    hb_buffer_t      * yadif_ref[3];

    int              cpu_count;
    int              segments;

    int              deint_nsegs;

    taskset_t        deint_taskset;         // Threads for fast deint
    taskset_t        yadif_taskset;         // Threads for Yadif

    deint_arguments_t *deint_arguments;     // Arguments to thread for work
    yadif_arguments_t *yadif_arguments;     // Arguments to thread for work
};

static int hb_deinterlace_init( hb_filter_object_t * filter,
                                hb_filter_init_t * init );

static int hb_deinterlace_work( hb_filter_object_t * filter,
                                hb_buffer_t ** buf_in,
                                hb_buffer_t ** buf_out );

static void hb_deinterlace_close( hb_filter_object_t * filter );

hb_filter_object_t hb_filter_deinterlace =
{
    .id            = HB_FILTER_DEINTERLACE,
    .enforce_order = 1,
    .name          = "Deinterlace",
    .settings      = NULL,
    .init          = hb_deinterlace_init,
    .work          = hb_deinterlace_work,
    .close         = hb_deinterlace_close,
};


static void yadif_store_ref(hb_filter_private_t *pv, hb_buffer_t *b)
{

    hb_buffer_close(&pv->yadif_ref[0]);
    memmove(&pv->yadif_ref[0], &pv->yadif_ref[1], sizeof(hb_buffer_t *) * 2 );
    pv->yadif_ref[2] = b;
}

static void yadif_filter_line(
    hb_filter_private_t * pv,
    uint8_t             * dst,
    uint8_t             * prev,
    uint8_t             * cur,
    uint8_t             * next,
    int                   width,
    int                   stride,
    int                   parity)
{
    uint8_t *prev2 = parity ? prev : cur ;
    uint8_t *next2 = parity ? cur  : next;

    int x;
    for( x = 0; x < width; x++)
    {
        int c              = cur[-stride];
        int d              = (prev2[0] + next2[0])>>1;
        int e              = cur[+stride];
        int temporal_diff0 = ABS(prev2[0] - next2[0]);
        int temporal_diff1 = ( ABS(prev[-stride] - c) + ABS(prev[+stride] - e) ) >> 1;
        int temporal_diff2 = ( ABS(next[-stride] - c) + ABS(next[+stride] - e) ) >> 1;
        int diff           = MAX3(temporal_diff0>>1, temporal_diff1, temporal_diff2);
        int spatial_pred   = (c+e)>>1;
        int spatial_score  = ABS(cur[-stride-1] - cur[+stride-1]) + ABS(c-e) +
                             ABS(cur[-stride+1] - cur[+stride+1]) - 1;

#define YADIF_CHECK(j)\
        {   int score = ABS(cur[-stride-1+j] - cur[+stride-1-j])\
                      + ABS(cur[-stride  +j] - cur[+stride  -j])\
                      + ABS(cur[-stride+1+j] - cur[+stride+1-j]);\
            if( score < spatial_score ){\
                spatial_score = score;\
                spatial_pred  = (cur[-stride  +j] + cur[+stride  -j])>>1;\

        YADIF_CHECK(-1) YADIF_CHECK(-2) }} }}
        YADIF_CHECK( 1) YADIF_CHECK( 2) }} }}

        if( pv->yadif_mode & MODE_YADIF_SPATIAL )
        {
            int b = (prev2[-2*stride] + next2[-2*stride])>>1;
            int f = (prev2[+2*stride] + next2[+2*stride])>>1;

            int max = MAX3(d-e, d-c, MIN(b-c, f-e));
            int min = MIN3(d-e, d-c, MAX(b-c, f-e));

            diff = MAX3( diff, min, -max );
        }

        if( spatial_pred > d + diff )
        {
            spatial_pred = d + diff;
        }
        else if( spatial_pred < d - diff )
        {
            spatial_pred = d - diff;
        }

        dst[0] = spatial_pred;

        dst++;
        cur++;
        prev++;
        next++;
        prev2++;
        next2++;
    }
}

typedef struct yadif_thread_arg_s {
    hb_filter_private_t *pv;
    int segment;
} yadif_thread_arg_t;

/*
 * deinterlace this segment of all three planes in a single thread.
 */
void yadif_filter_thread( void *thread_args_v )
{
    yadif_arguments_t *yadif_work = NULL;
    hb_filter_private_t * pv;
    int run = 1;
    int segment, segment_start, segment_stop;
    yadif_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_log("Yadif Deinterlace thread started for segment %d", segment);

    while( run )
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->yadif_taskset, segment );


        if( taskset_thread_stop( &pv->yadif_taskset, segment ) )
        {
            /*
             * No more work to do, exit this thread.
             */
            run = 0;
            goto report_completion;
        }

        yadif_work = &pv->yadif_arguments[segment];

        if( yadif_work->dst == NULL )
        {
            hb_error( "Thread started when no work available" );
            goto report_completion;
        }

        /*
         * Process all three planes, but only this segment of it.
         */
        int pp;
        for(pp = 0; pp < 3; pp++)
        {
            hb_buffer_t *dst = yadif_work->dst;
            int w = dst->plane[pp].width;
            int s = dst->plane[pp].stride;
            int h = dst->plane[pp].height;
            int yy;
            int parity = yadif_work->parity;
            int tff = yadif_work->tff;
            int penultimate = h - 2;

            int segment_height = (h / pv->segments) & ~1;
            segment_start = segment_height * segment;
            if( segment == pv->segments - 1 )
            {
                /*
                 * Final segment
                 */
                segment_stop = h;
            } else {
                segment_stop = segment_height * ( segment + 1 );
            }

            uint8_t *dst2 = &dst->plane[pp].data[segment_start * s];
            uint8_t *prev = &pv->yadif_ref[0]->plane[pp].data[segment_start * s];
            uint8_t *cur  = &pv->yadif_ref[1]->plane[pp].data[segment_start * s];
            uint8_t *next = &pv->yadif_ref[2]->plane[pp].data[segment_start * s];
            for( yy = segment_start; yy < segment_stop; yy++ )
            {
                if(((yy ^ parity) &  1))
                {
                    /* This is the bottom field when TFF and vice-versa.
                       It's the field that gets filtered. Because yadif
                       needs 2 lines above and below the one being filtered,
                       we need to mirror the edges. When TFF, this means
                       replacing the 2nd line with a copy of the 1st,
                       and the last with the second-to-last.                  */
                    if( yy > 1 && yy < penultimate )
                    {
                        /* This isn't the top or bottom,
                         * proceed as normal to yadif. */
                        yadif_filter_line(pv, dst2, prev, cur, next, w, s,
                                          parity ^ tff);
                    }
                    else
                    {
                        // parity == 0 (TFF), y1 = y0
                        // parity == 1 (BFF), y0 = y1
                        // parity == 0 (TFF), yu = yp
                        // parity == 1 (BFF), yp = yu
                        uint8_t *src  = &pv->yadif_ref[1]->plane[pp].data[(yy^parity)*s];
                        memcpy(dst2, src, w);
                    }
                }
                else
                {
                    /* Preserve this field unfiltered */
                    memcpy(dst2, cur, w);
                }
                dst2 += s;
                prev += s;
                cur += s;
                next += s;
            }
        }

report_completion:
        /*
         * Finished this segment, let everyone know.
         */
        taskset_thread_complete( &pv->yadif_taskset, segment );
    }
}


/*
 * threaded yadif - each thread deinterlaces a single segment of all
 * three planes. Where a segment is defined as the frame divided by
 * the number of CPUs.
 *
 * This function blocks until the frame is deinterlaced.
 */
static void yadif_filter( hb_filter_private_t * pv,
                          hb_buffer_t * dst, int parity, int tff)
{

    int segment;

    for( segment = 0; segment < pv->segments; segment++ )
    {
        /*
         * Setup the work for this plane.
         */
        pv->yadif_arguments[segment].parity = parity;
        pv->yadif_arguments[segment].tff = tff;
        pv->yadif_arguments[segment].dst = dst;
    }

    /* Allow the taskset threads to make one pass over the data. */
    taskset_cycle( &pv->yadif_taskset );

    /*
     * Entire frame is now deinterlaced.
     */
}

/*
 * deinterlace a frame in a single thread.
 */
void deint_filter_thread( void *thread_args_v )
{
    deint_arguments_t *args = NULL;
    hb_filter_private_t * pv;
    int run = 1;
    int segment;
    deint_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_log("Fast Deinterlace thread started for segment %d", segment);

    while( run )
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->deint_taskset, segment );


        if( taskset_thread_stop( &pv->deint_taskset, segment ) )
        {
            /*
             * No more work to do, exit this thread.
             */
            run = 0;
            goto report_completion;
        }

        args = &pv->deint_arguments[segment];

        if( args->dst == NULL )
        {
            // This can happen when flushing final buffers.
            goto report_completion;
        }

        /*
         * Process all three planes, but only this segment of it.
         */
        hb_deinterlace(args->dst, args->src);

report_completion:
        /*
         * Finished this segment, let everyone know.
         */
        taskset_thread_complete( &pv->deint_taskset, segment );
    }
}

/*
 * threaded fast deint - each thread deinterlaces a single frame.
 *
 * This function blocks until all frames are deinterlaced.
 */
static hb_buffer_t * deint_fast(hb_filter_private_t * pv, hb_buffer_t * in)
{

    int ii;
    hb_buffer_t *dst, *src;
    hb_buffer_list_t list;

    hb_buffer_list_clear(&list);
    if (in != NULL)
    {
        dst = hb_frame_buffer_init(in->f.fmt, in->f.width, in->f.height);
        pv->deint_arguments[pv->deint_nsegs].src = in;
        pv->deint_arguments[pv->deint_nsegs].dst = dst;
        pv->deint_nsegs++;
    }
    if (in != NULL && pv->deint_nsegs < pv->segments)
    {
        return NULL;
    }

    if (pv->deint_nsegs > 0)
    {
        /* Allow the taskset threads to make one pass over the data. */
        taskset_cycle( &pv->deint_taskset );
    }

    for (ii = 0; ii < pv->deint_nsegs; ii++)
    {
        src = pv->deint_arguments[ii].src;
        dst = pv->deint_arguments[ii].dst;
        pv->deint_arguments[ii].src = NULL;
        pv->deint_arguments[ii].dst = NULL;
        hb_buffer_list_append(&list, dst);

        dst->s = src->s;
        hb_buffer_close(&src);
    }
    pv->deint_nsegs = 0;

    return hb_buffer_list_clear(&list);
}

static int hb_deinterlace_init( hb_filter_object_t * filter,
                                hb_filter_init_t * init )
{
    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;

    pv->width = init->geometry.width;
    pv->height = init->geometry.height;

    pv->yadif_ready    = 0;
    pv->yadif_mode     = YADIF_MODE_DEFAULT;
    pv->yadif_parity   = YADIF_PARITY_DEFAULT;

    if( filter->settings )
    {
        sscanf( filter->settings, "%d:%d",
                &pv->yadif_mode,
                &pv->yadif_parity);
    }

    pv->cpu_count = hb_get_cpu_count();

    /* Allocate yadif specific buffers */
    if( pv->yadif_mode & MODE_YADIF_ENABLE )
    {
        /*
         * Setup yadif taskset.
         */
        pv->segments = pv->cpu_count;
        pv->yadif_arguments = malloc( sizeof( yadif_arguments_t ) * pv->segments );
        if( pv->yadif_arguments == NULL ||
            taskset_init( &pv->yadif_taskset, /*thread_count*/pv->segments,
                          sizeof( yadif_thread_arg_t ) ) == 0 )
        {
            hb_error( "yadif could not initialize taskset" );
        }

        int ii;
        for( ii = 0; ii < pv->segments; ii++ )
        {
            yadif_thread_arg_t *thread_args;

            thread_args = taskset_thread_args( &pv->yadif_taskset, ii );

            thread_args->pv = pv;
            thread_args->segment = ii;
            pv->yadif_arguments[ii].dst = NULL;

            if( taskset_thread_spawn( &pv->yadif_taskset, ii,
                                      "yadif_filter_segment",
                                      yadif_filter_thread,
                                      HB_NORMAL_PRIORITY ) == 0 )
            {
                hb_error( "yadif could not spawn thread" );
            }
        }
    }
    else
    {
        /*
         * Setup fast deint taskset.
         */
        pv->segments = pv->cpu_count;
        pv->deint_arguments = malloc( sizeof( deint_arguments_t ) * pv->segments );
        if( pv->deint_arguments == NULL ||
            taskset_init( &pv->deint_taskset, pv->segments,
                          sizeof( deint_thread_arg_t ) ) == 0 )
        {
            hb_error( "deint could not initialize taskset" );
        }

        int ii;
        for( ii = 0; ii < pv->segments; ii++ )
        {
            deint_thread_arg_t *thread_args;

            thread_args = taskset_thread_args( &pv->deint_taskset, ii );

            thread_args->pv = pv;
            thread_args->segment = ii;
            pv->deint_arguments[ii].dst = NULL;

            if( taskset_thread_spawn( &pv->deint_taskset, ii,
                                      "deint_filter_segment",
                                      deint_filter_thread,
                                      HB_NORMAL_PRIORITY ) == 0 )
            {
                hb_error( "deint could not spawn thread" );
            }
        }
    }

    return 0;
}

static void hb_deinterlace_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if( !pv )
    {
        return;
    }

    /* Cleanup yadif specific buffers */
    if( pv->yadif_mode & MODE_YADIF_ENABLE )
    {
        taskset_fini( &pv->yadif_taskset );

        int ii;
        for(ii = 0; ii < 3; ii++)
        {
            hb_buffer_close(&pv->yadif_ref[ii]);
        }

        free( pv->yadif_arguments );
    }
    else
    {
        taskset_fini( &pv->deint_taskset );
        free( pv->deint_arguments );
    }

    free( pv );
    filter->private_data = NULL;
}

static int hb_deinterlace_work( hb_filter_object_t * filter,
                                hb_buffer_t ** buf_in,
                                hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;
    hb_buffer_list_t list;

    *buf_in = NULL;
    hb_buffer_list_clear(&list);
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        if( !( pv->yadif_mode & MODE_YADIF_ENABLE ) )
        {
            // Flush final frames
            hb_buffer_list_append(&list, deint_fast(pv, NULL));
        }
        hb_buffer_list_append(&list, in);

        *buf_out = hb_buffer_list_clear(&list);
        return HB_FILTER_DONE;
    }

    /* Use fast deinterlace if yadif_mode < 0 */
    if( !( pv->yadif_mode & MODE_YADIF_ENABLE ) )
    {
        *buf_out = deint_fast(pv, in);
        return HB_FILTER_OK;
    }

    /* Store current frame in yadif cache */
    yadif_store_ref(pv, in);

    // yadif requires 3 buffers, prev, cur, and next.  For the first
    // frame, there can be no prev, so we duplicate the first frame.
    if (!pv->yadif_ready)
    {
        // If yadif is not ready, store another ref and return HB_FILTER_DELAY
        yadif_store_ref(pv, hb_buffer_dup(in));
        pv->yadif_ready = 1;
        // Wait for next
        return HB_FILTER_DELAY;
    }

    /* Determine if top-field first layout */
    int tff;
    if( pv->yadif_parity < 0 )
    {
        tff = !!(in->s.flags & PIC_FLAG_TOP_FIELD_FIRST);
    }
    else
    {
        tff = (pv->yadif_parity & 1) ^ 1;
    }

    /* deinterlace both fields if yadif 2 pass or bob */
    int frame, num_frames = 1;
    if ((pv->yadif_mode & MODE_YADIF_2PASS) ||
        (pv->yadif_mode & MODE_YADIF_BOB))
    {
        num_frames = 2;
    }

    // Will need up to 2 buffers simultaneously
    int idx = 0;
    hb_buffer_t * o_buf[2] = {NULL,};

    /* Perform yadif filtering */
    for( frame = 0; frame < num_frames; frame++ )
    {
        int parity = frame ^ tff ^ 1;

        if (o_buf[idx] == NULL)
        {
            o_buf[idx] = hb_frame_buffer_init(in->f.fmt,
                                              in->f.width, in->f.height);
        }
        yadif_filter(pv, o_buf[idx], parity, tff);

        // If bob, add both frames
        // else, add only final frame
        if (( pv->yadif_mode & MODE_YADIF_BOB ) || frame == num_frames - 1)
        {
            /* Copy buffered settings to output buffer settings */
            o_buf[idx]->s = pv->yadif_ref[1]->s;
            o_buf[idx]->next = NULL;
            hb_buffer_list_append(&list, o_buf[idx]);

            // Indicate that buffer was consumed
            o_buf[idx] = NULL;
            idx ^= 1;
        }
    }
    hb_buffer_close(&o_buf[0]);
    hb_buffer_close(&o_buf[1]);

    /* if bob mode is engaged, halve the duration of the
     * timestamps. */
    if (pv->yadif_mode & MODE_YADIF_BOB)
    {
        hb_buffer_t *first = hb_buffer_list_head(&list);
        hb_buffer_t *second = hb_buffer_list_tail(&list);
        first->s.stop -= (first->s.stop - first->s.start) / 2LL;
        second->s.start = first->s.stop;
        second->s.new_chap = 0;
    }

    *buf_out = hb_buffer_list_clear(&list);
    return HB_FILTER_OK;
}

