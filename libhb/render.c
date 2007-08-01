/* $Id: render.c,v 1.17 2005/04/14 17:37:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "ffmpeg/avcodec.h"
#include "ffmpeg/swscale.h"

struct hb_work_private_s
{
    hb_job_t * job;

    struct SwsContext  * context;
    AVPicture            pic_tmp_in;
    AVPicture            pic_tmp_crop;
    AVPicture            pic_tmp_out;        
    hb_buffer_t        * buf_scale;
    hb_fifo_t          * subtitle_queue;
};

int  renderInit( hb_work_object_t *, hb_job_t * );
int  renderWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void renderClose( hb_work_object_t * );

hb_work_object_t hb_render =
{   
    WORK_RENDER,
    "Renderer",
    renderInit,
    renderWork,
    renderClose
};

static void ApplySub( hb_job_t * job, hb_buffer_t * buf,
                      hb_buffer_t ** _sub )
{
    hb_buffer_t * sub = *_sub;
    hb_title_t * title = job->title;
    int i, j, offset_top, offset_left;
    uint8_t * lum, * alpha, * out;

    if( !sub )
    {
        return;
    }

    /* If necessary, move the subtitle so it is not in a cropped zone.
       When it won't fit, we center it so we loose as much on both ends.
       Otherwise we try to leave a 20px margin around it. */

    if( sub->height > title->height - job->crop[0] - job->crop[1] - 40 )
        offset_top = job->crop[0] + ( title->height - job->crop[0] -
                job->crop[1] - sub->height ) / 2;
    else if( sub->y < job->crop[0] + 20 )
        offset_top = job->crop[0] + 20;
    else if( sub->y > title->height - job->crop[1] - 20 - sub->height )
        offset_top = title->height - job->crop[1] - 20 - sub->height;
    else
        offset_top = sub->y;

    if( sub->width > title->width - job->crop[2] - job->crop[3] - 40 )
        offset_left = job->crop[2] + ( title->width - job->crop[2] -
                job->crop[3] - sub->width ) / 2;
    else if( sub->x < job->crop[2] + 20 )
        offset_left = job->crop[2] + 20;
    else if( sub->x > title->width - job->crop[3] - 20 - sub->width )
        offset_left = title->width - job->crop[3] - 20 - sub->width;
    else
        offset_left = sub->x;

    lum   = sub->data;
    alpha = lum + sub->width * sub->height;
    out   = buf->data + offset_top * title->width + offset_left;

    for( i = 0; i < sub->height; i++ )
    {
        if( offset_top + i >= 0 && offset_top + i < title->height )
        {
            for( j = 0; j < sub->width; j++ )
            {
                if( offset_left + j >= 0 && offset_left + j < title->width )
                {
                    out[j] = ( (uint16_t) out[j] * ( 16 - (uint16_t) alpha[j] ) +
                               (uint16_t) lum[j] * (uint16_t) alpha[j] ) >> 4;
                }
            }
        }

        lum   += sub->width;
        alpha += sub->width;
        out   += title->width;
    }

    hb_buffer_close( _sub );
}

int renderWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t   * job   = pv->job;
    hb_title_t * title = job->title;
    hb_buffer_t * in = *buf_in, * buf_tmp_in = *buf_in;
    
    if(!in->data)
    {
        /* If the input buffer is end of stream, send out an empty one to the next stage as well. */
       *buf_out = hb_buffer_init(0);
       return HB_WORK_OK;
    }
    
    /* Push subtitles onto queue just in case we need to delay a frame */
    if( in->sub )
    {
        hb_fifo_push( pv->subtitle_queue, in->sub );
    }
    else
    {
        hb_fifo_push( pv->subtitle_queue,  hb_buffer_init(0) );
    }
    
    /* Setup render buffer */
    hb_buffer_t * buf_render = hb_buffer_init( 3 * job->width * job->height / 2 );  

    /* Apply filters */
    if( job->filters )
    {
        int filter_count = hb_list_count( job->filters );
        int i;
        
        for( i = 0; i < filter_count; i++ )
        {
            hb_filter_object_t * filter = hb_list_item( job->filters, i );
            
            if( !filter )
            {
                continue;
            }            
            
            hb_buffer_t * buf_tmp_out = NULL;
            
            int result = filter->work( buf_tmp_in,
                                       &buf_tmp_out, 
                                       PIX_FMT_YUV420P, 
                                       title->width, 
                                       title->height, 
                                       filter->private_data );
            
            /* 
             * FILTER_OK:      set temp buffer to filter buffer, continue 
             * FILTER_DELAY:   set temp buffer to NULL, abort 
             * FILTER_DROP:    set temp buffer to NULL, pop subtitle, abort 
             * FILTER_FAILED:  leave temp buffer alone, continue 
             */
            if( result == FILTER_OK )
            {
                buf_tmp_in = buf_tmp_out;
            }
            else if( result == FILTER_DELAY )
            {
                buf_tmp_in = NULL;
                break;
            }            
            else if( result == FILTER_DROP )
            {
                hb_fifo_get( pv->subtitle_queue );
                buf_tmp_in = NULL;
                break;
            }
        }
    }   
        
    /* Apply subtitles */
    if( buf_tmp_in )
    {
        hb_buffer_t * subtitles = hb_fifo_get( pv->subtitle_queue );        
        if( subtitles )
        {
            ApplySub( job, buf_tmp_in, &subtitles );
        }
    }
    
    /* Apply crop/scale if specified */
    if( buf_tmp_in && pv->context )
    {
        avpicture_fill( &pv->pic_tmp_in, buf_tmp_in->data, 
                        PIX_FMT_YUV420P,
                        title->width, title->height );
        
        avpicture_fill( &pv->pic_tmp_out, buf_render->data, 
                        PIX_FMT_YUV420P,
                        job->width, job->height );

        // Crop; this alters the pointer to the data to point to the correct place for cropped frame
        av_picture_crop( &pv->pic_tmp_crop, &pv->pic_tmp_in, PIX_FMT_YUV420P,
                         job->crop[0], job->crop[2] );

        // Scale pic_crop into pic_render according to the context set up in renderInit
        sws_scale(pv->context,
                  pv->pic_tmp_crop.data, pv->pic_tmp_crop.linesize,
                  0, title->height - (job->crop[0] + job->crop[1]),
                  pv->pic_tmp_out.data,  pv->pic_tmp_out.linesize);
        
        hb_buffer_copy_settings( buf_render, buf_tmp_in );
        
        buf_tmp_in = buf_render;
    }  

    /* Set output to render buffer */
    (*buf_out) = buf_render;

    if( buf_tmp_in == NULL )
    {
        /* Teardown and cleanup buffers if we are emitting NULL */
        if( buf_in && *buf_in )
        {
            hb_buffer_close( buf_in );
            *buf_in = NULL;
        }        
        if( buf_out && *buf_out )
        {
            hb_buffer_close( buf_out );        
            *buf_out = NULL;
        }
    }
    else if( buf_tmp_in != buf_render )
    {    
        /* Copy temporary results and settings into render buffer */
        memcpy( buf_render->data, buf_tmp_in->data, buf_render->size );
        hb_buffer_copy_settings( buf_render, buf_tmp_in );
    }

    return HB_WORK_OK;
}

void renderClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;   
        
    /* Cleanup subtitle queue */
    if( pv->subtitle_queue )
    {
        hb_fifo_close( &pv->subtitle_queue );
    }
    
    /* Cleanup filters */
    /* TODO: Move to work.c? */
    if( pv->job->filters )
    {
        int filter_count = hb_list_count( pv->job->filters );
        int i;
        
        for( i = 0; i < filter_count; i++ )
        {
            hb_filter_object_t * filter = hb_list_item( pv->job->filters, i );
            
            if( !filter ) continue;

            filter->close( filter->private_data );
        }
        
        hb_list_close( &pv->job->filters );
    }    
   
    /* Cleanup render work structure */
    free( pv );
    w->private_data = NULL;    
}

int renderInit( hb_work_object_t * w, hb_job_t * job )
{   
    /* Allocate new private work object */
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    pv->job = job;
    w->private_data = pv;

    /* Get title and title size */
    hb_title_t * title = job->title;

    /* If crop or scale is specified, setup rescale context */
    if( job->crop[0] || job->crop[1] || job->crop[2] || job->crop[3] ||
        job->width != title->width || job->height != title->height )
    {
        pv->context = sws_getContext(title->width  - (job->crop[2] + job->crop[3]),
                                     title->height - (job->crop[0] + job->crop[1]),
                                     PIX_FMT_YUV420P,
                                     job->width, job->height, PIX_FMT_YUV420P,
                                     SWS_LANCZOS, NULL, NULL, NULL);
    }   
    
    /* Setup FIFO queue for subtitle cache */
    pv->subtitle_queue = hb_fifo_init( 8 );    
    
    /* Setup filters */
    /* TODO: Move to work.c? */
    if( job->filters )
    {
        int filter_count = hb_list_count( job->filters );
        int i;
        
        for( i = 0; i < filter_count; i++ )
        {
            hb_filter_object_t * filter = hb_list_item( job->filters, i );

            if( !filter ) continue;
            
            filter->private_data = filter->init( PIX_FMT_YUV420P,
                                                 title->width,
                                                 title->height,
                                                 filter->settings );
        }
    }
    
    return 0;
}
