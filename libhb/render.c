/* $Id: render.c,v 1.17 2005/04/14 17:37:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "hbffmpeg.h"

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
    int                  count_frames;      // frames output so far
    double               frame_rate;        // 90KHz ticks per frame (for CFR/PFR)
    double               out_last_stop;     // where last frame ended (for CFR/PFR)
    int                  drops;             // frames dropped (for CFR/PFR)
    int                  dups;              // frames duped (for CFR/PFR)
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
    return(&data[(y>>1) * ((width+1)>>1) + (x>>1) + width*height]);
}

static uint8_t *getV(uint8_t *data, int width, int height, int x, int y)
{
    int w2 = (width+1) >> 1, h2 = (height+1) >> 1;
    return(&data[(y>>1) * w2 + (x>>1) + width*height + w2*h2]);
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

// delete the buffer 'out' from the chain of buffers whose head is 'buf_out'.
// out & buf_out must be non-null (checks prior to anywhere this routine
// can be called guarantee this) and out must be on buf_out's chain
// (all places that call this get 'out' while traversing the chain).
// 'out' is freed and its predecessor is returned.
static hb_buffer_t *delete_buffer_from_chain( hb_buffer_t **buf_out, hb_buffer_t *out)
{
    hb_buffer_t *succ = out->next;
    hb_buffer_t *pred = *buf_out;
    if ( pred == out )
    {
        // we're deleting the first buffer
        *buf_out = succ;
    }
    else
    {
        // target isn't the first buf so search for its predecessor.
        while ( pred->next != out )
        {
            pred = pred->next;
        }
        // found 'out' - remove it from the chain
        pred->next = succ;
    }
    hb_buffer_close( &out );
    return succ;
}

// insert buffer 'succ' after buffer chain element 'pred'.
// caller must guarantee that 'pred' and 'succ' are non-null.
static hb_buffer_t *insert_buffer_in_chain( hb_buffer_t *pred, hb_buffer_t *succ )
{
    succ->next = pred->next;
    pred->next = succ;
    return succ;
}

// This section of the code implements video frame rate control.
// Since filters are allowed to duplicate and drop frames (which
// changes the timing), this has to be the last thing done in render.
//
// There are three options, selected by the value of job->cfr:
//   0 - Variable Frame Rate (VFR) or 'same as source': frame times
//       are left alone
//   1 - Constant Frame Rate (CFR): Frame timings are adjusted so that all
//       frames are exactly job->vrate_base ticks apart. Frames are dropped
//       or duplicated if necessary to maintain this spacing.
//   2 - Peak Frame Rate (PFR): job->vrate_base is treated as the peak
//       average frame rate. I.e., the average frame rate (current frame
//       end time divided by number of frames so far) is never allowed to be
//       greater than job->vrate_base and frames are dropped if necessary
//       to keep the average under this value. Other than those drops, frame
//       times are left alone.
//

static void adjust_frame_rate( hb_work_private_t *pv, hb_buffer_t **buf_out )
{
    hb_buffer_t *out = *buf_out;

    while ( out && out->size > 0 )
    {
        // this frame has to start where the last one stopped.
        out->start = pv->out_last_stop;

        // compute where this frame would stop if the frame rate were constant
        // (this is our target stopping time for CFR and earliest possible
        // stopping time for PFR).
        double cfr_stop = pv->frame_rate * ( pv->count_frames + 1 );

        if ( cfr_stop - (double)out->stop >= pv->frame_rate )
        {
            // This frame stops a frame time or more in the past - drop it
            // but don't lose its chapter mark.
            if ( out->new_chap )
            {
                pv->chapter_time = out->start;
                pv->chapter_val = out->new_chap;
            }
            ++pv->drops;
            out = delete_buffer_from_chain( buf_out, out );
            continue;
        }

        // at this point we know that this frame doesn't push the average
        // rate over the limit so we just pass it on for PFR. For CFR we're
        // going to return it (with its start & stop times modified) and
        // we may have to dup it.
        ++pv->count_frames;
        if ( pv->job->cfr > 1 )
        {
            // PFR - we're going to keep the frame but may need to
            // adjust it's stop time to meet the average rate constraint.
            if ( out->stop <= cfr_stop )
            {
                out->stop = cfr_stop;
            }
        }
        else
        {
            // we're doing CFR so we have to either trim some time from a
            // buffer that ends too far in the future or, if the buffer is
            // two or more frame times long, split it into multiple pieces,
            // each of which is a frame time long.
            double excess_dur = (double)out->stop - cfr_stop;
            out->stop = cfr_stop;
            for ( ; excess_dur >= pv->frame_rate; excess_dur -= cfr_stop )
            {
                /* next frame too far ahead - dup current frame */
                hb_buffer_t *dup = hb_buffer_init( out->size );
                memcpy( dup->data, out->data, out->size );
                hb_buffer_copy_settings( dup, out );
                dup->new_chap = 0;
                dup->start = cfr_stop;
                cfr_stop += pv->frame_rate;
                dup->stop = cfr_stop;
                out = insert_buffer_in_chain( out, dup );
                ++pv->dups;
                ++pv->count_frames;
            }
        }
        pv->out_last_stop = out->stop;
        out = out->next;
    }
}

int renderWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t   * job   = pv->job;
    hb_title_t * title = job->title;
    hb_buffer_t * in = *buf_in, * buf_tmp_in = *buf_in;
    hb_buffer_t * ivtc_buffer = NULL;

    if( in->size <= 0 )
    {
        hb_buffer_t *head = NULL, *tail = NULL, *next;
        int counter = 2;

        /* If the input buffer is end of stream, send out an empty one
         * to the next stage as well. To avoid losing the contents of
         * the delay queue connect the buffers in the delay queue in 
         * the correct order, and add the end of stream buffer to the
         * end.
         */     
        while( ( next = hb_fifo_get( pv->delay_queue ) ) != NULL )
        {
            
            /* We can't use the given time stamps. Previous frames
               might already have been extended, throwing off the
               raw values fed to render.c. Instead, their
               stop and start times are stored in arrays.
               The 4th cached frame will be the to use.
               If it needed its duration extended to make up
               lost time, it will have happened above. */
            next->start = pv->last_start[counter];
            next->stop = pv->last_stop[counter--];
            
            if( !head && !tail )
            {
                head = tail = next;
            } else {
                tail->next = next;
                tail = next;
            }
        }
        if( tail )
        {
            tail->next = in;
            *buf_out = head;
            if ( job->cfr )
            {
                adjust_frame_rate( pv, buf_out );
            }
        } else {
            *buf_out = in;
        }     
        *buf_in = NULL;
        return HB_WORK_DONE;
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
    if( in->new_chap )
    {
        pv->chapter_time = in->start;
        pv->chapter_val = in->new_chap;
        in->new_chap = 0;
    }

    /* Setup render buffer */
    hb_buffer_t * buf_render = hb_video_buffer_init( job->width, job->height );

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

    if (*buf_out )
    {
        hb_fifo_push( pv->delay_queue, *buf_out );
        *buf_out = NULL;
    }

    /*
     * Keep the last three frames in our queue, this ensures that we have the last
     * two always in there should we need to rewrite the durations on them.
     */

    if( hb_fifo_size( pv->delay_queue ) >= 4 )
    {
        *buf_out = hb_fifo_get( pv->delay_queue );
    }

    if( *buf_out )
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

    if ( buf_out && *buf_out && job->cfr )
    {
        adjust_frame_rate( pv, buf_out );
    }
    return HB_WORK_OK;
}

void renderClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if ( pv->job->cfr )
    {
        hb_log("render: %d frames output, %d dropped and %d duped for CFR/PFR",
               pv->count_frames, pv->drops, pv->dups );
    }

    hb_interjob_t * interjob = hb_interjob_get( w->private_data->job->h );
    
    /* Preserve dropped frame count for more accurate framerates in 2nd passes. */
    interjob->render_dropped = pv->dropped_frames;

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

    swsflags = SWS_LANCZOS | SWS_ACCURATE_RND;

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

    pv->frame_rate = (double)job->vrate_base * (1./300.);

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
