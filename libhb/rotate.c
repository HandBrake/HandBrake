
#include "hb.h"
#include "hbffmpeg.h"
//#include "mpeg2dec/mpeg2.h"

#define MODE_DEFAULT     3
// Mode 1: Flip vertically (y0 becomes yN and yN becomes y0)
// Mode 2: Flip horizontally (x0 becomes xN and xN becomes x0)
// Mode 3: Flip both horizontally and vertically (modes 1 and 2 combined)

typedef struct rotate_arguments_s {
    hb_buffer_t *dst;
    hb_buffer_t *src;
    int stop;
} rotate_arguments_t;

struct hb_filter_private_s
{
    int              mode;
    int              width;
    int              height;
    int              par_width;
    int              par_height;

    int              cpu_count;

    hb_thread_t    ** rotate_threads;        // Threads for Rotate - one per CPU
    hb_lock_t      ** rotate_begin_lock;     // Thread has work
    hb_lock_t      ** rotate_complete_lock;  // Thread has completed work
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
    .init_index    = 2,
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
         * Wait here until there is work to do. hb_lock() blocks until
         * render releases it to say that there is more work to do.
         */
        hb_lock( pv->rotate_begin_lock[segment] );

        rotate_work = &pv->rotate_arguments[segment];

        if( rotate_work->stop )
        {
            /*
             * No more work to do, exit this thread.
             */
            run = 0;
            continue;
        } 

        if( rotate_work->dst == NULL )
        {
            hb_error( "Thread started when no work available" );
            hb_snooze(500);
            continue;
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
        /*
         * Finished this segment, let everyone know.
         */
        hb_unlock( pv->rotate_complete_lock[segment] );
    }
    free( thread_args_v );
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

        /*
         * Let the thread for this plane know that we've setup work 
         * for it by releasing the begin lock (ensuring that the
         * complete lock is already locked so that we block when
         * we try to lock it again below).
         */
        hb_lock( pv->rotate_complete_lock[segment] );
        hb_unlock( pv->rotate_begin_lock[segment] );
    }

    /*
     * Wait until all three threads have completed by trying to get
     * the complete lock that we locked earlier for each thread, which
     * will block until that thread has completed the work on that
     * plane.
     */
    for( segment = 0; segment < pv->cpu_count; segment++ )
    {
        hb_lock( pv->rotate_complete_lock[segment] );
        hb_unlock( pv->rotate_complete_lock[segment] );
    }

    /*
     * Entire frame is now rotated.
     */
}


static int hb_rotate_init( hb_filter_object_t * filter,
                           hb_filter_init_t * init )
{
    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;

    pv->mode     = MODE_DEFAULT;

    if( filter->settings )
    {
        sscanf( filter->settings, "%d",
                &pv->mode );
    }

    pv->cpu_count = hb_get_cpu_count();


    /*
     * Create threads and locks.
     */
    pv->rotate_threads = malloc( sizeof( hb_thread_t* ) * pv->cpu_count );
    pv->rotate_begin_lock = malloc( sizeof( hb_lock_t * ) * pv->cpu_count );
    pv->rotate_complete_lock = malloc( sizeof( hb_lock_t * ) * pv->cpu_count );
    pv->rotate_arguments = malloc( sizeof( rotate_arguments_t ) * pv->cpu_count );

    int i;
    for( i = 0; i < pv->cpu_count; i++ )
    {
        rotate_thread_arg_t *thread_args;
    
        thread_args = malloc( sizeof( rotate_thread_arg_t ) );
    
        if( thread_args ) {
            thread_args->pv = pv;
            thread_args->segment = i;
    
            pv->rotate_begin_lock[i] = hb_lock_init();
            pv->rotate_complete_lock[i] = hb_lock_init();
    
            /*
             * Important to start off with the threads locked waiting
             * on input.
             */
            hb_lock( pv->rotate_begin_lock[i] );
    
            pv->rotate_arguments[i].stop = 0;
            pv->rotate_arguments[i].dst = NULL;
            
            pv->rotate_threads[i] = hb_thread_init( "rotate_filter_segment",
                                                   rotate_filter_thread,
                                                   thread_args,
                                                   HB_NORMAL_PRIORITY );
        } else {
            hb_error( "rotate could not create threads" );
        }
    }
    // Set init width/height so the next stage in the pipline
    // knows what it will be getting
    if( pv->mode & 4 )
    {
        // 90 degree rotation, exchange width and height
        int tmp = init->width;
        init->width = init->height;
        init->height = tmp;

        tmp = init->par_width;
        init->par_width = init->par_height;
        init->par_height = tmp;
    }
    pv->width = init->width;
    pv->height = init->height;
    pv->par_width = init->par_width;
    pv->par_height = init->par_height;

    return 0;
}

static int hb_rotate_info( hb_filter_object_t * filter,
                           hb_filter_info_t * info )
{
    hb_filter_private_t * pv = filter->private_data;
    if( !pv )
        return 1;

    memset( info, 0, sizeof( hb_filter_info_t ) );
    info->out.width = pv->width;
    info->out.height = pv->height;
    info->out.par_width = pv->par_width;
    info->out.par_height = pv->par_height;
    int pos = 0;
    if( pv->mode & 1 )
        pos += sprintf( &info->human_readable_desc[pos], "flip vertical" );
    if( pv->mode & 2 )
    {
        if( pos )
            pos += sprintf( &info->human_readable_desc[pos], "/" );
        pos += sprintf( &info->human_readable_desc[pos], "flip horizontal" );
    }
    if( pv->mode & 4 )
    {
        if( pos )
            pos += sprintf( &info->human_readable_desc[pos], "/" );
        pos += sprintf( &info->human_readable_desc[pos], "rotate 90" );
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

    int i;
    for( i = 0; i < pv->cpu_count; i++)
    {
        /*
         * Tell each rotate thread to stop, and then cleanup.
         */
        pv->rotate_arguments[i].stop = 1;
        hb_unlock(  pv->rotate_begin_lock[i] );
    
        hb_thread_close( &pv->rotate_threads[i] );
        hb_lock_close( &pv->rotate_begin_lock[i] );
        hb_lock_close( &pv->rotate_complete_lock[i] );
    }
    
    /*
     * free memory for rotate structs
     */
    free( pv->rotate_threads );
    free( pv->rotate_begin_lock );
    free( pv->rotate_complete_lock );
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

    if ( in->size <= 0 )
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
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
    hb_buffer_move_subs( out, in );
    
    *buf_out = out;
    
    return HB_FILTER_OK;
}
