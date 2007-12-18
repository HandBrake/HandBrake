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
    hb_fifo_t          * delay_queue;
    int                  frames_to_extend;
    int                  dropped_frames;
    int                  extended_frames;
    uint64_t             last_start[4];
    uint64_t             last_stop[4];
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

/*
 * getU() & getV()
 *
 * Utility function that finds where the U is in the YUV sub-picture
 *
 * The Y data is at the top, followed by U and V, but the U and V
 * are half the width of the Y, i.e. each chroma element covers 2x2
 * of the Y's.
 */
static uint8_t *getU(uint8_t *data, int width, int height, int x, int y)
{
    return(&data[(((y/2) * (width/2)) + (x/2)) + (width*height)]);
}

static uint8_t *getV(uint8_t *data, int width, int height, int x, int y)
{
    return(&data[(((y/2) * (width/2)) + (x/2)) + (width*height) + 
                 (width*height)/4]);
}

static void ApplySub( hb_job_t * job, hb_buffer_t * buf,
                      hb_buffer_t ** _sub )
{
    hb_buffer_t * sub = *_sub;
    hb_title_t * title = job->title;
    int i, j, offset_top, offset_left;
    uint8_t * lum, * alpha, * out, * sub_chromaU, * sub_chromaV;

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
    sub_chromaU = alpha + sub->width * sub->height;
    sub_chromaV = sub_chromaU + sub->width * sub->height;

    out   = buf->data + offset_top * title->width + offset_left;

    for( i = 0; i < sub->height; i++ )
    {
        if( offset_top + i >= 0 && offset_top + i < title->height )
        {
            for( j = 0; j < sub->width; j++ )
            {
                if( offset_left + j >= 0 && offset_left + j < title->width )
                {
                    uint8_t *chromaU, *chromaV;

                    /*
                     * Merge the luminance and alpha with the picture
                     */
                    out[j] = ( (uint16_t) out[j] * ( 16 - (uint16_t) alpha[j] ) +
                               (uint16_t) lum[j] * (uint16_t) alpha[j] ) >> 4;   
                    /*
                     * Set the chroma (colour) based on whether there is
                     * any alpha at all. Don't try to blend with the picture.
                     */
                    chromaU = getU(buf->data, title->width, title->height,
                                   offset_left+j, offset_top+i);
                    
                    chromaV = getV(buf->data, title->width, title->height,
                                   offset_left+j, offset_top+i);
                    
                    if( alpha[j] > 0 )
                    {
                        /*
                         * Add the chroma from the sub-picture, as this is 
                         * not a transparent element.
                         */
                        *chromaU = sub_chromaU[j];
                        *chromaV = sub_chromaV[j];
                    } 
                }
            }
        }

        lum   += sub->width;
        alpha += sub->width;
        sub_chromaU += sub->width;
        sub_chromaV += sub->width;
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
    hb_buffer_t * ivtc_buffer = NULL;
    
    if(!in->data)
    {
        /* If the input buffer is end of stream, send out an empty one
         * to the next stage as well. Note that this will result in us
         * losing the current contents of the delay queue.
         */
       *buf_out = hb_buffer_init(0);
       return HB_WORK_OK;
    }

    /*
     * During the indepth_scan ditch the buffers here before applying filters or attempting to
     * use the subtitles.
     */
    if( job->indepth_scan )
    {      
        *buf_out = NULL;
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
    
    /* Cache frame start and stop times, so we can renumber
       time stamps if dropping frames for VFR.              */ 
    int i;
    for( i = 3; i >= 1; i-- )
    {
        pv->last_start[i] = pv->last_start[i-1];
        pv->last_stop[i] = pv->last_stop[i-1];
    }
    pv->last_start[0] = in->start;
    pv->last_stop[0] = in->stop;
    
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
                if( job->vfr )
                {
                    pv->frames_to_extend += 4;
                    pv->dropped_frames++;
                }
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
    
    if (*buf_out)
    {
        hb_fifo_push( pv->delay_queue, *buf_out );
        *buf_out = NULL;        
    }

    /*
     * Keep the last three frames in our queue, this ensures that we have the last
     * two always in there should we need to rewrite the durations on them.
     */
    if( hb_fifo_size( pv->delay_queue ) >= 3 )
    {
        *buf_out = hb_fifo_get( pv->delay_queue );
    } 

    if( *buf_out )
    {
        if( pv->frames_to_extend )
        {
            /*
             * A frame's been dropped by VFR detelecine.
             * Gotta make up the lost time. This will also
             * slow down the video to 23.976fps.
             * The dropped frame ran for 3003 ticks, so
             * divvy it up amongst the 4 frames left behind.
             * This is what the delay_queue is for;
             * telecined sequences start 2 frames before
             * the dropped frame, so to slow down the right
             * ones you need a 2 frame delay between
             * reading input and writing output.
             */
            ivtc_buffer = *buf_out;
            
            /* The 4th cached frame will be the to use. */
            ivtc_buffer->start = pv->last_start[3];
            ivtc_buffer->stop = pv->last_stop[3];

            if (pv->frames_to_extend % 4)
                ivtc_buffer->stop += 751;
            else
                ivtc_buffer->stop += 750;
            
            /* Set the 3rd cached frame to start when this one stops,
               and to stop 3003 ticks later -- a normal 29.97fps
               length frame. If it needs to be extended as well to
               make up lost time, it'll be handled on the next
               loop through the renderer.                            */
            pv->last_start[2] = ivtc_buffer->stop;
            pv->last_stop[2] = ivtc_buffer->stop + 3003;
            
            pv->frames_to_extend--;
            pv->extended_frames++;
        }

    }

    return HB_WORK_OK;
}

void renderClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;   
        
    hb_log("render: dropped frames: %i (%i ticks)", pv->dropped_frames, (pv->dropped_frames * 3003) );
    hb_log("render: extended frames: %i (%i ticks)", pv->extended_frames, ( ( pv->extended_frames / 4 ) * 3003 ) );
    hb_log("render: Lost time: %i frames (%i ticks)", (pv->dropped_frames * 4) - (pv->extended_frames), (pv->dropped_frames * 3003) - ( ( pv->extended_frames / 4 ) * 3003 ) );

    /* Cleanup subtitle queue */
    if( pv->subtitle_queue )
    {
        hb_fifo_close( &pv->subtitle_queue );
    }
    
    if( pv->delay_queue )
    {
        hb_fifo_close( &pv->delay_queue );
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
                                     (uint16_t)(SWS_LANCZOS|SWS_ACCURATE_RND), NULL, NULL, NULL);
    }   
    
    /* Setup FIFO queue for subtitle cache */
    pv->subtitle_queue = hb_fifo_init( 8 );    
    pv->delay_queue = hb_fifo_init( 8 );
    pv->frames_to_extend = 0;
    pv->dropped_frames = 0;
    pv->extended_frames = 0;
    pv->last_start[0] = 0;
    pv->last_stop[0] = 0;
    
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
