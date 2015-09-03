/* rorate.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
 
#include "hb.h"
#include "hbffmpeg.h"
#include "taskset.h"

#define MODE_DEFAULT     3
// Settings:
//  degrees:mirror
//
//  degrees - Rotation angle, may be one of 90, 180, or 270
//  mirror  - Mirror image around x axis
//
// Examples:
// Mode 180:1 Mirror then rotate 180'
// Mode   0:1 Mirror
// Mode 180:0 Rotate 180'
// Mode  90:0 Rotate 90'
// Mode 270:0 Rotate 270'
//
// Legacy Mode Examples (also accepted):
// Mode 1: Flip vertically (y0 becomes yN and yN becomes y0) (aka 180:1)
// Mode 2: Flip horizontally (x0 becomes xN and xN becomes x0) (aka 0:1)
// Mode 3: Flip both horizontally and vertically (aka 180:0)
// Mode 4: Rotate 90' (aka 90:0)
// Mode 7: Flip horiz & vert plus Rotate 90' (aka 270:0)

typedef struct rotate_arguments_s {
    hb_buffer_t *dst;
    hb_buffer_t *src;
} rotate_arguments_t;

struct hb_filter_private_s
{
    int              mode;
    int              width;
    int              height;
    hb_rational_t    par;

    int              cpu_count;

    taskset_t         rotate_taskset;        // Threads for Rotate - one per CPU
    rotate_arguments_t *rotate_arguments;     // Arguments to thread for work
};

static int hb_rotate_init( hb_filter_object_t * filter,
                           hb_filter_init_t * init );

static int hb_rotate_work( hb_filter_object_t * filter,
                           hb_buffer_t ** buf_in,
                           hb_buffer_t ** buf_out );

static void hb_rotate_close( hb_filter_object_t * filter );

static int hb_rotate_info( hb_filter_object_t * filter,
                           hb_filter_info_t * info );

hb_filter_object_t hb_filter_rotate =
{
    .id            = HB_FILTER_ROTATE,
    .enforce_order = 0,
    .name          = "Rotate (rotate & flip image axes)",
    .settings      = NULL,
    .init          = hb_rotate_init,
    .work          = hb_rotate_work,
    .close         = hb_rotate_close,
    .info          = hb_rotate_info
};


typedef struct rotate_thread_arg_s {
    hb_filter_private_t *pv;
    int segment;
} rotate_thread_arg_t;

/*
 * rotate this segment of all three planes in a single thread.
 */
void rotate_filter_thread( void *thread_args_v )
{
    rotate_arguments_t *rotate_work = NULL;
    hb_filter_private_t * pv;
    int run = 1;
    int plane;
    int segment, segment_start, segment_stop;
    rotate_thread_arg_t *thread_args = thread_args_v;
    uint8_t *dst;
    hb_buffer_t *dst_buf;
    hb_buffer_t *src_buf;
    int y;


    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_log("Rotate thread started for segment %d", segment);

    while( run )
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->rotate_taskset, segment );

        if( taskset_thread_stop( &pv->rotate_taskset, segment ) )
        {
            /*
             * No more work to do, exit this thread.
             */
            run = 0;
            goto report_completion;
        } 

        rotate_work = &pv->rotate_arguments[segment];
        if( rotate_work->dst == NULL )
        {
            hb_error( "Thread started when no work available" );
            hb_snooze(500);
            goto report_completion;
        }
        
        /*
         * Process all three planes, but only this segment of it.
         */
        dst_buf = rotate_work->dst;
        src_buf = rotate_work->src;
        for( plane = 0; plane < 3; plane++)
        {
            int dst_stride, src_stride;

            dst = dst_buf->plane[plane].data;
            dst_stride = dst_buf->plane[plane].stride;
            src_stride = src_buf->plane[plane].stride;

            int h = src_buf->plane[plane].height;
            int w = src_buf->plane[plane].width;
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
                uint8_t * cur;
                int x, xo, yo;

                cur = &src_buf->plane[plane].data[y * src_stride];
                for( x = 0; x < w; x++)
                {
                    if( pv->mode & 1 )
                    {
                        yo = h - y - 1;
                    }
                    else
                    {
                        yo = y;
                    }
                    if( pv->mode & 2 )
                    {
                        xo = w - x - 1;
                    }
                    else
                    {
                        xo = x;
                    }
                    if( pv->mode & 4 ) // Rotate 90 clockwise
                    {
                        int tmp = xo;
                        xo = h - yo - 1;
                        yo = tmp;
                    }
                    dst[yo*dst_stride + xo] = cur[x];
                }
            }
        }

report_completion:
        /*
         * Finished this segment, let everyone know.
         */
        taskset_thread_complete( &pv->rotate_taskset, segment );
    }
}


/*
 * threaded rotate - each thread rotates a single segment of all
 * three planes. Where a segment is defined as the frame divided by
 * the number of CPUs.
 *
 * This function blocks until the frame is rotated.
 */
static void rotate_filter( 
    hb_filter_private_t * pv, 
    hb_buffer_t *out, 
    hb_buffer_t *in )
{

    int segment;
    
    for( segment = 0; segment < pv->cpu_count; segment++ )
    {  
        /*
         * Setup the work for this plane.
         */
        pv->rotate_arguments[segment].dst = out;
        pv->rotate_arguments[segment].src = in;
    }

    /*
     * Allow the taskset threads to make one pass over the data.
     */
    taskset_cycle( &pv->rotate_taskset );

    /*
     * Entire frame is now rotated.
     */
}


static int hb_rotate_init( hb_filter_object_t * filter,
                           hb_filter_init_t * init )
{
    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;

    int degrees = MODE_DEFAULT, mirror = 0, num_params = 0;

    if( filter->settings )
    {
        num_params = sscanf(filter->settings, "%d:%d", &degrees, &mirror);
    }
    switch (degrees)
    {
        case 0:
            pv->mode = 0;
            break;
        case 90:
            pv->mode = 4;
            break;
        case 180:
            pv->mode = 3;
            break;
        case 270:
            pv->mode = 7;
            break;
        default:
            if (degrees & ~7)
            {
                // Unsupported rotation angle
                hb_error("Unsupported rotate mode %d", degrees);
            }
            // Legacy "mode" supplied in settings
            pv->mode = degrees & 7;
            break;
    }
    if (num_params > 1 && mirror)
        pv->mode ^= 2;

    pv->cpu_count = hb_get_cpu_count();

    /*
     * Create rotate taskset.
     */
    pv->rotate_arguments = malloc( sizeof( rotate_arguments_t ) * pv->cpu_count );
    if( pv->rotate_arguments == NULL ||
        taskset_init( &pv->rotate_taskset, /*thread_count*/pv->cpu_count,
                      sizeof( rotate_thread_arg_t ) ) == 0 )
    {
            hb_error( "rotate could not initialize taskset" );
    }

    int i;
    for( i = 0; i < pv->cpu_count; i++ )
    {
        rotate_thread_arg_t *thread_args;
    
        thread_args = taskset_thread_args( &pv->rotate_taskset, i );
    
        thread_args->pv = pv;
        thread_args->segment = i;
        pv->rotate_arguments[i].dst = NULL;
    
        if( taskset_thread_spawn( &pv->rotate_taskset, i,
                                  "rotate_filter_segment",
                                  rotate_filter_thread,
                                  HB_NORMAL_PRIORITY ) == 0 )
        {
            hb_error( "rotate could not spawn thread" );
        }
    }
    // Set init width/height so the next stage in the pipline
    // knows what it will be getting
    if( pv->mode & 4 )
    {
        // 90 degree rotation, exchange width and height
        int tmp = init->geometry.width;
        init->geometry.width = init->geometry.height;
        init->geometry.height = tmp;

        tmp = init->geometry.par.num;
        init->geometry.par.num = init->geometry.par.den;
        init->geometry.par.den = tmp;
    }
    pv->width = init->geometry.width;
    pv->height = init->geometry.height;
    pv->par = init->geometry.par;

    return 0;
}

static int hb_rotate_info( hb_filter_object_t * filter,
                           hb_filter_info_t * info )
{
    hb_filter_private_t * pv = filter->private_data;
    if( !pv )
        return 1;

    memset( info, 0, sizeof( hb_filter_info_t ) );
    info->out.geometry.width = pv->width;
    info->out.geometry.height = pv->height;
    info->out.geometry.par = pv->par;
    int pos = 0;

    int mirror_x = !!(pv->mode & 2);
    int mirror_y = !!(pv->mode & 1);
    int degrees = 0;
    if (mirror_y)
    {
        degrees += 180;
        mirror_x ^= 1;
        mirror_y ^= 1;
    }
    degrees += !!(pv->mode & 4) * 90;

    if (degrees > 0)
    {
        sprintf(&info->human_readable_desc[pos], "Rotate %d%s", degrees,
                mirror_x ? " / mirror image" : "");
    }
    else if (mirror_x)
    {
        sprintf(&info->human_readable_desc[pos], "Mirror image");
    }
    else
    {
        sprintf(&info->human_readable_desc[pos], "No rotation or mirror!");
    }
    return 0;
}

static void hb_rotate_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if( !pv )
    {
        return;
    }

    taskset_fini( &pv->rotate_taskset );
    
    /*
     * free memory for rotate structs
     */
    free( pv->rotate_arguments );

    free( pv );
    filter->private_data = NULL;
}

static int hb_rotate_work( hb_filter_object_t * filter,
                           hb_buffer_t ** buf_in,
                           hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in, * out;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }
    if (pv->mode == 0)
    {
        // Short circuit case where filter does nothing
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_OK;
    }

    int width_out, height_out;
    if ( pv->mode & 4 )
    {
        width_out = in->f.height;
        height_out = in->f.width;
    }
    else
    {
        width_out = in->f.width;
        height_out = in->f.height;
    }

    out = hb_video_buffer_init( width_out, height_out );

    // Rotate!
    rotate_filter( pv, out, in );

    out->s = in->s;
    *buf_out = out;
    
    return HB_FILTER_OK;
}
