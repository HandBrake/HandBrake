
#include "hb.h"
#include "hbffmpeg.h"
//#include "mpeg2dec/mpeg2.h"

#define MODE_DEFAULT     3
// Mode 1: Flip vertically (y0 becomes yN and yN becomes y0)
// Mode 2: Flip horizontally (x0 becomes xN and xN becomes x0)
// Mode 3: Flip both horizontally and vertically (modes 1 and 2 combined)

typedef struct rotate_arguments_s {
    uint8_t **dst;
    int stop;
} rotate_arguments_t;

struct hb_filter_private_s
{
    int              pix_fmt;
    int              width[3];
    int              height[3];

    int              mode;

    int              ref_stride[3];

    int              cpu_count;

    hb_thread_t    ** rotate_threads;        // Threads for Rotate - one per CPU
    hb_lock_t      ** rotate_begin_lock;     // Thread has work
    hb_lock_t      ** rotate_complete_lock;  // Thread has completed work
    rotate_arguments_t *rotate_arguments;     // Arguments to thread for work

    AVPicture        pic_in;
    AVPicture        pic_out;
    hb_buffer_t *    buf_out;
    hb_buffer_t *    buf_settings;
};

hb_filter_private_t * hb_rotate_init( int pix_fmt,
                                           int width,
                                           int height,
                                           char * settings );

int hb_rotate_work( hb_buffer_t * buf_in,
                         hb_buffer_t ** buf_out,
                         int pix_fmt,
                         int width,
                         int height,
                         hb_filter_private_t * pv );

void hb_rotate_close( hb_filter_private_t * pv );

hb_filter_object_t hb_filter_rotate =
{
    FILTER_ROTATE,
    "Rotate (flips image axes)",
    NULL,
    hb_rotate_init,
    hb_rotate_work,
    hb_rotate_close,
};


static void rotate_filter_line( uint8_t *dst,
                               uint8_t *cur,
                               int plane,
                               hb_filter_private_t * pv )
{

    int w = pv->width[plane];

    int x;
    for( x = 0; x < w; x++)
    {
        if( pv->mode & 2 )
        {
            dst[x] = cur[w-x-1];
        }
        else
        {
            dst[x] = cur[x];
        }
    }
}

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
    uint8_t **dst;
    int y, w, h, ref_stride;


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
        for( plane = 0; plane < 3; plane++)
        {

            dst = rotate_work->dst;
            w = pv->width[plane];
            h = pv->height[plane];
            ref_stride = pv->ref_stride[plane];
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
                
                if( pv->mode & 1 )
                {
                    cur  = &pv->pic_in.data[plane][(h-y-1)*pv->pic_in.linesize[plane]];
                }
                else
                {
                    cur  = &pv->pic_in.data[plane][(y)*pv->pic_in.linesize[plane]];
                }
                uint8_t *dst2 = &dst[plane][y*w];

                rotate_filter_line( dst2, 
                                   cur, 
                                   plane, 
                                   pv );
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
 * threaded rotate - each thread rptates a single segment of all
 * three planes. Where a segment is defined as the frame divided by
 * the number of CPUs.
 *
 * This function blocks until the frame is rotated.
 */
static void rotate_filter( uint8_t ** dst,
                          hb_filter_private_t * pv )
{

    int segment;
    
    for( segment = 0; segment < pv->cpu_count; segment++ )
    {  
        /*
         * Setup the work for this plane.
         */
        pv->rotate_arguments[segment].dst = dst;

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


hb_filter_private_t * hb_rotate_init( int pix_fmt,
                                           int width,
                                           int height,
                                           char * settings )
{
    if( pix_fmt != PIX_FMT_YUV420P )
    {
        return 0;
    }

    hb_filter_private_t * pv = calloc( 1, sizeof(struct hb_filter_private_s) );

    pv->pix_fmt = pix_fmt;

    pv->width[0]  = width;
    pv->height[0] = height;
    pv->width[1]  = pv->width[2]  = width >> 1;
    pv->height[1] = pv->height[2] = height >> 1;

    pv->buf_out = hb_video_buffer_init( width, height );
    pv->buf_settings = hb_buffer_init( 0 );

    pv->mode     = MODE_DEFAULT;

    pv->ref_stride[0] = pv->width[0];
    pv->ref_stride[1] = pv->width[1];
    pv->ref_stride[2] = pv->width[2];
    
    if( settings )
    {
        sscanf( settings, "%d",
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

    return pv;
}

void hb_rotate_close( hb_filter_private_t * pv )
{
    if( !pv )
    {
        return;
    }

    /* Cleanup frame buffers */
    if( pv->buf_out )
    {
        hb_buffer_close( &pv->buf_out );
    }
    if (pv->buf_settings )
    {
        hb_buffer_close( &pv->buf_settings );
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
}

int hb_rotate_work( hb_buffer_t * buf_in,
                         hb_buffer_t ** buf_out,
                         int pix_fmt,
                         int width,
                         int height,
                         hb_filter_private_t * pv )
{
    if( !pv ||
        pix_fmt != pv->pix_fmt ||
        width   != pv->width[0] ||
        height  != pv->height[0] )
    {
        return FILTER_FAILED;
    }

    avpicture_fill( &pv->pic_in, buf_in->data,
                    pix_fmt, width, height );

    avpicture_fill( &pv->pic_out, pv->buf_out->data,
                        pix_fmt, width, height );

    //do stuff here
    rotate_filter( pv->pic_out.data, pv );
    hb_buffer_copy_settings( pv->buf_out, buf_in );
    
    *buf_out = pv->buf_out;
    
    return FILTER_OK;
}


