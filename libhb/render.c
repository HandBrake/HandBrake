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
    int                  dropped_frames;
    int                  extended_frames;
    uint64_t             last_start[4];
    uint64_t             last_stop[4];
    uint64_t             lost_time[4];
    uint64_t             total_lost_time;
    uint64_t             total_gained_time;
    int64_t              chapter_time;
    int                  chapter_val;
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
    int i, j, offset_top, offset_left, margin_top, margin_percent;
    uint8_t * lum, * alpha, * out, * sub_chromaU, * sub_chromaV;

    /*
     * Percent of height of picture that form a margin that subtitles
     * should not be displayed within.
     */
    margin_percent = 2;

    if( !sub )
    {
        return;
    }

    /*
     * If necessary, move the subtitle so it is not in a cropped zone.
     * When it won't fit, we center it so we lose as much on both ends.
     * Otherwise we try to leave a 20px or 2% margin around it.
     */
    margin_top = ( ( title->height - job->crop[0] - job->crop[1] ) *
                   margin_percent ) / 100;

    if( margin_top > 20 )
    {
        /*
         * A maximum margin of 20px regardless of height of the picture.
         */
        margin_top = 20;
    }

    if( sub->height > title->height - job->crop[0] - job->crop[1] -
        ( margin_top * 2 ) )
    {
        /*
         * The subtitle won't fit in the cropped zone, so center
         * it vertically so we fit in as much as we can.
         */
        offset_top = job->crop[0] + ( title->height - job->crop[0] -
                                      job->crop[1] - sub->height ) / 2;
    }
    else if( sub->y < job->crop[0] + margin_top )
    {
        /*
         * The subtitle fits in the cropped zone, but is currently positioned
         * within our top margin, so move it outside of our margin.
         */
        offset_top = job->crop[0] + margin_top;
    }
    else if( sub->y > title->height - job->crop[1] - margin_top - sub->height )
    {
        /*
         * The subtitle fits in the cropped zone, and is not within the top
         * margin but is within the bottom margin, so move it to be above
         * the margin.
         */
        offset_top = title->height - job->crop[1] - margin_top - sub->height;
    }
    else
    {
        /*
         * The subtitle is fine where it is.
         */
        offset_top = sub->y;
    }

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

    /* If there's a chapter mark remember it in case we delay or drop its frame */
    if( in->new_chap && job->vfr )
    {
        pv->chapter_time = in->start;
        pv->chapter_val = in->new_chap;
        in->new_chap = 0;
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
                if( job->vfr )
                {
                    /* We need to compensate for the time lost by dropping this frame.
                       Spread its duration out in quarters, because usually dropped frames
                       maintain a 1-out-of-5 pattern and this spreads it out amongst the remaining ones.
                       Store these in the lost_time array, which has 4 slots in it.
                       Because not every frame duration divides evenly by 4, and we can't lose the
                       remainder, we have to go through an awkward process to preserve it in the 4th array index. */
                    uint64_t temp_duration = buf_tmp_out->stop - buf_tmp_out->start;
                    pv->lost_time[0] += (temp_duration / 4);
                    pv->lost_time[1] += (temp_duration / 4);
                    pv->lost_time[2] += (temp_duration / 4);
                    pv->lost_time[3] += ( temp_duration - (temp_duration / 4) - (temp_duration / 4) - (temp_duration / 4) );

                    pv->total_lost_time += temp_duration;
                    pv->dropped_frames++;

                    /* Pop the frame's subtitle and dispose of it. */
                    hb_buffer_t * subtitles = hb_fifo_get( pv->subtitle_queue );
                    hb_buffer_close( &subtitles );

                    buf_tmp_in = NULL;
                }
                else
                {
                    buf_tmp_in = buf_tmp_out;
                }
                break;
            }
        }
    }

    if( buf_tmp_in )
    {
        /* Cache frame start and stop times, so we can renumber
           time stamps if dropping frames for VFR.              */
        int i;
        for( i = 3; i >= 1; i-- )
        {
            pv->last_start[i] = pv->last_start[i-1];
            pv->last_stop[i] = pv->last_stop[i-1];
        }

        /* In order to make sure we have continuous time stamps, store
           the current frame's duration as starting when the last one stopped. */
        pv->last_start[0] = pv->last_stop[1];
        pv->last_stop[0] = pv->last_start[0] + (in->stop - in->start);
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

    if (*buf_out && job->vfr)
    {
        hb_fifo_push( pv->delay_queue, *buf_out );
        *buf_out = NULL;
    }

    /*
     * Keep the last three frames in our queue, this ensures that we have the last
     * two always in there should we need to rewrite the durations on them.
     */

    if( job->vfr )
    {
        if( hb_fifo_size( pv->delay_queue ) >= 3 )
        {
            *buf_out = hb_fifo_get( pv->delay_queue );
        }
    }

    if( *buf_out && job->vfr)
    {
        /* The current frame exists. That means it hasn't been dropped by a filter.
           Make it accessible as ivtc_buffer so we can edit its duration if needed. */
        ivtc_buffer = *buf_out;

        if( pv->lost_time[3] > 0 )
        {
            /*
             * A frame's been dropped earlier by VFR detelecine.
             * Gotta make up the lost time. This will also
             * slow down the video.
             * The dropped frame's has to be accounted for, so
             * divvy it up amongst the 4 frames left behind.
             * This is what the delay_queue is for;
             * telecined sequences start 2 frames before
             * the dropped frame, so to slow down the right
             * ones you need a 2 frame delay between
             * reading input and writing output.
             */

            /* We want to extend the outputted frame's duration by the value
              stored in the 4th slot of the lost_time array. Because we need
              to adjust all the values in the array so they're contiguous,
              extend the duration inside the array first, before applying
              it to the current frame buffer. */
            pv->last_stop[3] += pv->lost_time[3];

            /* Log how much time has been added back in to the video. */
            pv->total_gained_time += pv->lost_time[3];

            /* We've pulled the 4th value from the lost_time array
               and added it to the last_stop array's 4th slot. Now, rotate the
                lost_time array so the 4th slot now holds the 3rd's value, and
               so on down the line, and set the 0 index to a value of 0. */
            int i;
            for( i=2; i >=  0; i--)
            {
                pv->lost_time[i+1] = pv->lost_time[i];
            }
            pv->lost_time[0] = 0;

            /* Log how many frames have had their durations extended. */
            pv->extended_frames++;
        }

        /* We can't use the given time stamps. Previous frames
           might already have been extended, throwing off the
           raw values fed to render.c. Instead, their
           stop and start times are stored in arrays.
           The 4th cached frame will be the to use.
           If it needed its duration extended to make up
           lost time, it will have happened above. */
        ivtc_buffer->start = pv->last_start[3];
        ivtc_buffer->stop = pv->last_stop[3];

        /* Set the 3rd cached frame to start when this one stops,
           and so on down the line. If any of them need to be
           extended as well to make up lost time, it'll be handled
           on the next loop through the renderer.  */
        int i;
        for (i = 2; i >= 0; i--)
        {
            int temp_duration = pv->last_stop[i] - pv->last_start[i];
            pv->last_start[i] = pv->last_stop[i+1];
            pv->last_stop[i] = pv->last_start[i] + temp_duration;
        }

        /* If we have a pending chapter mark and this frame is at
           or after the time of the mark, mark this frame & clear
           our pending mark. */
        if( pv->chapter_time && pv->chapter_time <= ivtc_buffer->start )
        {
            ivtc_buffer->new_chap = pv->chapter_val;
            pv->chapter_time = 0;
        }

    }

    return HB_WORK_OK;
}

void renderClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    hb_log("render: lost time: %lld (%i frames)", pv->total_lost_time, pv->dropped_frames);
    hb_log("render: gained time: %lld (%i frames) (%lld not accounted for)", pv->total_gained_time, pv->extended_frames, pv->total_lost_time - pv->total_gained_time);
    if (pv->dropped_frames)
        hb_log("render: average dropped frame duration: %lld", (pv->total_lost_time / pv->dropped_frames) );

    /* Cleanup subtitle queue */
    if( pv->subtitle_queue )
    {
        hb_fifo_close( &pv->subtitle_queue );
    }

    if( pv->delay_queue )
    {
        hb_fifo_close( &pv->delay_queue );
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
    uint32_t    swsflags;

    swsflags = SWS_LANCZOS;
#ifndef __x86_64__
    swsflags |= SWS_ACCURATE_RND;
#endif  /* __x86_64__ */

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
                                     swsflags, NULL, NULL, NULL);
    }

    /* Setup FIFO queue for subtitle cache */
    pv->subtitle_queue = hb_fifo_init( 8 );
    pv->delay_queue = hb_fifo_init( 8 );

    /* VFR IVTC needs a bunch of time-keeping variables to track
      how many frames are dropped, how many are extended, what the
      last 4 start and stop times were (so they can be modified),
      how much time has been lost and gained overall, how much time
      the latest 4 frames should be extended by, and where chapter
      markers are (so they can be saved if their frames are dropped.) */
    pv->dropped_frames = 0;
    pv->extended_frames = 0;
    pv->last_start[0] = 0;
    pv->last_stop[0] = 0;
    pv->total_lost_time = 0;
    pv->total_gained_time = 0;
    pv->lost_time[0] = 0; pv->lost_time[1] = 0; pv->lost_time[2] = 0; pv->lost_time[3] = 0;
    pv->chapter_time = 0;
    pv->chapter_val  = 0;

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
