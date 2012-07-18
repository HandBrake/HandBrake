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
#include "mpeg2dec/mpeg2.h"
#include "mcdeint.h"
#include "taskset.h"

// yadif_mode is a bit vector with the following flags
// Note that 2PASS should be enabled when using MCDEINT
#define MODE_YADIF_ENABLE       1
#define MODE_YADIF_SPATIAL      2
#define MODE_YADIF_2PASS        4
#define MODE_YADIF_BOB          8

#define YADIF_MODE_DEFAULT      0
#define YADIF_PARITY_DEFAULT   -1

#define MCDEINT_MODE_DEFAULT   -1
#define MCDEINT_QP_DEFAULT      1

#define ABS(a) ((a) > 0 ? (a) : (-(a)))
#define MIN3(a,b,c) MIN(MIN(a,b),c)
#define MAX3(a,b,c) MAX(MAX(a,b),c)

typedef struct yadif_arguments_s {
    uint8_t **dst;
    int parity;
    int tff;
} yadif_arguments_t;

struct hb_filter_private_s
{
    int              width[3];
    int              height[3];

    int              yadif_mode;
    int              yadif_parity;
    int              yadif_ready;

    uint8_t        * yadif_ref[4][3];
    int              yadif_ref_stride[3];

    int              cpu_count;

    taskset_t        yadif_taskset;         // Threads for Yadif - one per CPU

    yadif_arguments_t *yadif_arguments;     // Arguments to thread for work

    int              mcdeint_mode;
    mcdeint_private_t mcdeint;

    hb_buffer_t *    buf_out[2];
    hb_buffer_t *    buf_settings;
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
    .name          = "Deinterlace (ffmpeg or yadif/mcdeint)",
    .settings      = NULL,
    .init          = hb_deinterlace_init,
    .work          = hb_deinterlace_work,
    .close         = hb_deinterlace_close,
};


static void yadif_store_ref( const uint8_t ** pic,
                             hb_filter_private_t * pv )
{
    memcpy( pv->yadif_ref[3],
            pv->yadif_ref[0],
            sizeof(uint8_t *)*3 );

    memmove( pv->yadif_ref[0],
             pv->yadif_ref[1],
             sizeof(uint8_t *)*3*3 );

    int i;
    for( i = 0; i < 3; i++ )
    {
        const uint8_t * src = pic[i];
        uint8_t * ref = pv->yadif_ref[2][i];

        int w = pv->width[i];
        int ref_stride = pv->yadif_ref_stride[i];

        int y;
        for( y = 0; y < pv->height[i]; y++ )
        {
            memcpy(ref, src, w);
            src = (uint8_t*)src + w;
            ref = (uint8_t*)ref + ref_stride;
        }
    }
}

static void yadif_filter_line( uint8_t *dst,
                               uint8_t *prev,
                               uint8_t *cur,
                               uint8_t *next,
                               int plane,
                               int parity,
                               hb_filter_private_t * pv )
{
    uint8_t *prev2 = parity ? prev : cur ;
    uint8_t *next2 = parity ? cur  : next;

    int w = pv->width[plane];
    int refs = pv->yadif_ref_stride[plane];

    int x;
    for( x = 0; x < w; x++)
    {
        int c              = cur[-refs];
        int d              = (prev2[0] + next2[0])>>1;
        int e              = cur[+refs];
        int temporal_diff0 = ABS(prev2[0] - next2[0]);
        int temporal_diff1 = ( ABS(prev[-refs] - c) + ABS(prev[+refs] - e) ) >> 1;
        int temporal_diff2 = ( ABS(next[-refs] - c) + ABS(next[+refs] - e) ) >> 1;
        int diff           = MAX3(temporal_diff0>>1, temporal_diff1, temporal_diff2);
        int spatial_pred   = (c+e)>>1;
        int spatial_score  = ABS(cur[-refs-1] - cur[+refs-1]) + ABS(c-e) +
                             ABS(cur[-refs+1] - cur[+refs+1]) - 1;

#define YADIF_CHECK(j)\
        {   int score = ABS(cur[-refs-1+j] - cur[+refs-1-j])\
                      + ABS(cur[-refs  +j] - cur[+refs  -j])\
                      + ABS(cur[-refs+1+j] - cur[+refs+1-j]);\
            if( score < spatial_score ){\
                spatial_score = score;\
                spatial_pred  = (cur[-refs  +j] + cur[+refs  -j])>>1;\

        YADIF_CHECK(-1) YADIF_CHECK(-2) }} }}
        YADIF_CHECK( 1) YADIF_CHECK( 2) }} }}

        if( pv->yadif_mode & MODE_YADIF_SPATIAL )
        {
            int b = (prev2[-2*refs] + next2[-2*refs])>>1;
            int f = (prev2[+2*refs] + next2[+2*refs])>>1;

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
    int plane;
    int segment, segment_start, segment_stop;
    yadif_thread_arg_t *thread_args = thread_args_v;
    uint8_t **dst;
    int parity, tff, y, w, h, ref_stride, penultimate, ultimate;


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
            hb_snooze(500);
            goto report_completion;
        }
        
        /*
         * Process all three planes, but only this segment of it.
         */
        for( plane = 0; plane < 3; plane++)
        {

            dst = yadif_work->dst;
            parity = yadif_work->parity;
            tff = yadif_work->tff;
            w = pv->width[plane];
            h = pv->height[plane];
            penultimate = h -2;
            ultimate = h - 1;
            ref_stride = pv->yadif_ref_stride[plane];
            segment_start = ( h / pv->cpu_count ) * segment;
            if( segment == pv->cpu_count - 1 )
            {
                /*
                 * Final segment
                 */
                segment_stop = h;
            } else {
                segment_stop = ( h / pv->cpu_count ) * ( segment + 1 );
            }

            for( y = segment_start; y < segment_stop; y++ )
            {
                if( ( ( y ^ parity ) &  1 ) )
                {
                    /* This is the bottom field when TFF and vice-versa.
                       It's the field that gets filtered. Because yadif
                       needs 2 lines above and below the one being filtered,
                       we need to mirror the edges. When TFF, this means
                       replacing the 2nd line with a copy of the 1st,
                       and the last with the second-to-last.                  */
                    if( y > 1 && y < ( h -2 ) )
                    {
                        /* This isn't the top or bottom, proceed as normal to yadif. */
                        uint8_t *prev = &pv->yadif_ref[0][plane][y*ref_stride];
                        uint8_t *cur  = &pv->yadif_ref[1][plane][y*ref_stride];
                        uint8_t *next = &pv->yadif_ref[2][plane][y*ref_stride];
                        uint8_t *dst2 = &dst[plane][y*w];

                        yadif_filter_line( dst2, 
                                           prev, 
                                           cur, 
                                           next, 
                                           plane, 
                                           parity ^ tff, 
                                           pv );
                    }
                    else if( y == 0 )
                    {
                        /* BFF, so y0 = y1 */
                        memcpy( &dst[plane][y*w],
                                &pv->yadif_ref[1][plane][1*ref_stride],
                                w * sizeof(uint8_t) );
                    }
                    else if( y == 1 )
                    {
                        /* TFF, so y1 = y0 */
                        memcpy( &dst[plane][y*w],
                                &pv->yadif_ref[1][plane][0],
                                w * sizeof(uint8_t) );
                    }
                    else if( y == penultimate )
                    {
                        /* BFF, so penultimate y = ultimate y */
                        memcpy( &dst[plane][y*w],
                                &pv->yadif_ref[1][plane][ultimate*ref_stride],
                                w * sizeof(uint8_t) );
                    }
                    else if( y == ultimate )
                    {
                        /* TFF, so ultimate y = penultimate y */
                        memcpy( &dst[plane][y*w],
                                &pv->yadif_ref[1][plane][penultimate*ref_stride],
                                w * sizeof(uint8_t) );
                    }
                }
                else
                {
                    /* Preserve this field unfiltered */
                    memcpy( &dst[plane][y*w],
                            &pv->yadif_ref[1][plane][y*ref_stride],
                            w * sizeof(uint8_t) );
                }
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
static void yadif_filter( uint8_t ** dst,
                          int parity,
                          int tff,
                          hb_filter_private_t * pv )
{

    int segment;

    for( segment = 0; segment < pv->cpu_count; segment++ )
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

static int hb_deinterlace_init( hb_filter_object_t * filter,
                                hb_filter_init_t * init )
{
    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;

    pv->width[0]  = hb_image_stride( init->pix_fmt, init->width, 0 );
    pv->height[0] = hb_image_height( init->pix_fmt, init->height, 0 );
    pv->width[1]  = pv->width[2]  = hb_image_stride( init->pix_fmt, init->width, 1 );
    pv->height[1] = pv->height[2] = hb_image_height( init->pix_fmt, init->height, 1 );

    pv->buf_out[0] = hb_video_buffer_init( init->width, init->height );
    pv->buf_out[1] = hb_video_buffer_init( init->width, init->height );
    pv->buf_settings = hb_buffer_init( 0 );

    pv->yadif_ready    = 0;
    pv->yadif_mode     = YADIF_MODE_DEFAULT;
    pv->yadif_parity   = YADIF_PARITY_DEFAULT;

    pv->mcdeint_mode   = MCDEINT_MODE_DEFAULT;
    int mcdeint_qp     = MCDEINT_QP_DEFAULT;

    if( filter->settings )
    {
        sscanf( filter->settings, "%d:%d:%d:%d",
                &pv->yadif_mode,
                &pv->yadif_parity,
                &pv->mcdeint_mode,
                &mcdeint_qp );
    }

    pv->cpu_count = hb_get_cpu_count();

    /* Allocate yadif specific buffers */
    if( pv->yadif_mode & MODE_YADIF_ENABLE )
    {
        int i, j;
        for( i = 0; i < 3; i++ )
        {
            int is_chroma = !!i;
            int w = ((init->width   + 31) & (~31))>>is_chroma;
            int h = ((init->height+6+ 31) & (~31))>>is_chroma;

            pv->yadif_ref_stride[i] = w;

            for( j = 0; j < 3; j++ )
            {
                pv->yadif_ref[j][i] = malloc( w*h*sizeof(uint8_t) ) + 3*w;
            }
        }

        /*
         * Setup yadif taskset.
         */
        pv->yadif_arguments = malloc( sizeof( yadif_arguments_t ) * pv->cpu_count );
        if( pv->yadif_arguments == NULL ||
            taskset_init( &pv->yadif_taskset, /*thread_count*/pv->cpu_count,
                          sizeof( yadif_arguments_t ) ) == 0 )
        {
            hb_error( "yadif could not initialize taskset" );
        }

        for( i = 0; i < pv->cpu_count; i++ )
        {
            yadif_thread_arg_t *thread_args;

            thread_args = taskset_thread_args( &pv->yadif_taskset, i );

            thread_args->pv = pv;
            thread_args->segment = i;
            pv->yadif_arguments[i].dst = NULL;

            if( taskset_thread_spawn( &pv->yadif_taskset, i,
                                      "yadif_filter_segment",
                                      yadif_filter_thread,
                                      HB_NORMAL_PRIORITY ) == 0 )
            {
                hb_error( "yadif could not spawn thread" );
            }
        }
    }

    mcdeint_init( &pv->mcdeint, pv->mcdeint_mode, mcdeint_qp, 
                  init->pix_fmt, init->width, init->height );
    
    return 0;
}

static void hb_deinterlace_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if( !pv )
    {
        return;
    }

    /* Cleanup frame buffers */
    if( pv->buf_out[0] )
    {
        hb_buffer_close( &pv->buf_out[0] );
    }
    if( pv->buf_out[1] )
    {
        hb_buffer_close( &pv->buf_out[1] );
    }
    if (pv->buf_settings )
    {
        hb_buffer_close( &pv->buf_settings );
    }

    /* Cleanup yadif specific buffers */
    if( pv->yadif_mode & MODE_YADIF_ENABLE )
    {
        int i;
        for( i = 0; i<3*3; i++ )
        {
            uint8_t **p = &pv->yadif_ref[i%3][i/3];
            if (*p)
            {
                free( *p - 3*pv->yadif_ref_stride[i/3] );
                *p = NULL;
            }
        }

        taskset_fini( &pv->yadif_taskset );
        free( pv->yadif_arguments );
    }

    mcdeint_close( &pv->mcdeint );
    
    free( pv );
    filter->private_data = NULL;
}

static int hb_deinterlace_work( hb_filter_object_t * filter,
                                hb_buffer_t ** buf_in,
                                hb_buffer_t ** buf_out )
{
    AVPicture pic_in;
    AVPicture pic_out;
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;
    hb_buffer_t * last = NULL, * out = NULL;
    uint8_t duplicate = 0;

    if ( in->size <= 0 )
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    do
    {
        hb_avpicture_fill( &pic_in, in );

        /* Use libavcodec deinterlace if yadif_mode < 0 */
        if( !( pv->yadif_mode & MODE_YADIF_ENABLE ) )
        {
            int width = (pv->buf_out[0]->plane[0].width + 3) & ~0x3;
            int height = (pv->buf_out[0]->plane[0].height + 3) & ~0x3;

            hb_avpicture_fill( &pic_out, pv->buf_out[0] );

            // avpicture_deinterlace requires 4 pixel aligned width and height
            // we have aligned all buffers to 16 byte width and height strides
            // so there is room in the buffers to accomodate a litte
            // overscan.
            avpicture_deinterlace( &pic_out, &pic_in, pv->buf_out[0]->f.fmt, 
                                   width, height );

            pv->buf_out[0]->s = in->s;
            hb_buffer_move_subs( pv->buf_out[0], in );

            *buf_out = pv->buf_out[0];

            // Allocate a replacement for the buffer we just consumed
            hb_buffer_t * b = pv->buf_out[0];
            pv->buf_out[0] = hb_video_buffer_init( b->f.width, b->f.height );

            return HB_FILTER_OK;
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

        /* Store current frame in yadif cache */
        if (!duplicate)
        {
            yadif_store_ref( (const uint8_t**)pic_in.data, pv );
        }

        /* If yadif is not ready, store another ref and return HB_FILTER_DELAY */
        if( pv->yadif_ready == 0 )
        {
            yadif_store_ref( (const uint8_t**)pic_in.data, pv );

            pv->buf_settings->s = in->s;
            hb_buffer_move_subs( pv->buf_settings, in );

            pv->yadif_ready = 1;

            return HB_FILTER_DELAY;
        }

        /* deinterlace both fields if mcdeint is enabled without bob */
        int frame, num_frames = 1;
        if( ( pv->yadif_mode & MODE_YADIF_2PASS ) &&
           !( pv->yadif_mode & MODE_YADIF_BOB ) )
        {
            num_frames = 2;
        }

        /* Perform yadif and mcdeint filtering */
        int out_frame;
        hb_buffer_t * b;
        for( frame = 0; frame < num_frames; frame++ )
        {
            AVPicture pic_yadif_out;
            int parity = frame ^ tff ^ 1 ^ duplicate;

            b = pv->buf_out[!(frame^1)];
            hb_avpicture_fill( &pic_yadif_out, b );

            yadif_filter( pic_yadif_out.data, parity, tff, pv );

            if( pv->mcdeint_mode >= 0 )
            {
                b = pv->buf_out[(frame^1)];
                hb_avpicture_fill( &pic_out, b );

                mcdeint_filter( pic_out.data, pic_yadif_out.data, parity, 
                                pv->width, pv->height, &pv->mcdeint );

                out_frame = (frame^1);
            }
            else
            {
                out_frame = !(frame^1);
            }
        }

        // Add to list of output buffers (should be at most 2)
        if ( out == NULL )
        {
            last = out = pv->buf_out[out_frame];
        }
        else
        {
            last->next = pv->buf_out[out_frame];
            last = last->next;
        }

        // Allocate a replacement for the buffer we just consumed
        b = pv->buf_out[out_frame];
        pv->buf_out[out_frame] = hb_video_buffer_init( b->f.width, b->f.height );

        /* Copy buffered settings to output buffer settings */
        last->s = pv->buf_settings->s;

        if ( !duplicate )
        {
            hb_buffer_move_subs( last, pv->buf_settings );
        }

        /* if bob mode is engaged, halve the duration of the
         * timestamp, and request a duplicate. */
        if( pv->yadif_mode & MODE_YADIF_BOB )
        {
            if ( !duplicate )
            {
                last->s.stop -= (last->s.stop - last->s.start) / 2LL;
                duplicate = 1;
            }
            else
            {
                last->s.start = out->s.stop;
                last->s.new_chap = 0;
                duplicate = 0;
            }
        }
    } while ( duplicate );

    /* Replace buffered settings with input buffer settings */
    pv->buf_settings->s = in->s;
    hb_buffer_move_subs( pv->buf_settings, in );

    *buf_out = out;

    return HB_FILTER_OK;
}

