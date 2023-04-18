/* vfr.c

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "libavutil/avutil.h"
#include "handbrake/handbrake.h"

//#define HB_DEBUG_CFR_DROPS 1
#define MAX_FRAME_ANALYSIS_DEPTH 10

struct hb_filter_private_s
{
    hb_job_t      * job;
    int             cfr;
    hb_rational_t   input_vrate;
    hb_rational_t   vrate;
    hb_fifo_t     * delay_queue;
    int             dropped_frames;
    int             extended_frames;
    int64_t         last_start[4];
    int64_t         last_stop[4];
    int64_t         lost_time[4];
    int64_t         total_lost_time;
    int64_t         total_gained_time;
    int             count_frames;       // frames output so far
    double          frame_duration;     // 90KHz ticks per frame (for CFR/PFR)
    double          out_last_stop;      // where last frame ended (for CFR/PFR)
    int             drops;              // frames dropped (for CFR/PFR)
    int             dups;               // frames duped (for CFR/PFR)

    // Duplicate frame detection members
    int             frame_analysis_depth;
    int64_t         frame_analysis_duration;
    hb_list_t     * frame_rate_list;
    double        * frame_metric;

    unsigned        gamma_lut[256];
#if defined(HB_DEBUG_CFR_DROPS)
    int64_t         sequence;
#endif
};

static int hb_vfr_init( hb_filter_object_t * filter,
                        hb_filter_init_t * init );

static int hb_vfr_work( hb_filter_object_t * filter,
                        hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out );

static void hb_vfr_close( hb_filter_object_t * filter );
static hb_filter_info_t * hb_vfr_info( hb_filter_object_t * filter );

static const char hb_vfr_template[] =
    "mode=^([012])$:rate=^"HB_RATIONAL_REG"$";

hb_filter_object_t hb_filter_vfr =
{
    .id                = HB_FILTER_VFR,
    .enforce_order     = 1,
    .name              = "Framerate Shaper",
    .settings          = NULL,
    .init              = hb_vfr_init,
    .work              = hb_vfr_work,
    .close             = hb_vfr_close,
    .info              = hb_vfr_info,
    .settings_template = hb_vfr_template,
};

// Create gamma lookup table.
// Note that we are creating a scaled integer lookup table that will
// not cause overflows in sse_block16() below.  This results in
// small values being truncated to 0 which is ok for this usage.
static void build_gamma_lut( hb_filter_private_t * pv )
{
    int i;
    for( i = 0; i < 256; i++ )
    {
        pv->gamma_lut[i] = 4095 * pow( ( (float)i / (float)255 ), 2.2f );
    }
}

#define DUP_THRESH_SSE 5.0

// Compute the sum of squared errors for a 16x16 block
// Gamma adjusts pixel values so that less visible differences
// count less.
static inline unsigned sse_block16( unsigned *gamma_lut, uint8_t *a, uint8_t *b, int stride )
{
    int x, y;
    unsigned sum = 0;
    int diff;

    for( y = 0; y < 16; y++ )
    {
        for( x = 0; x < 16; x++ )
        {
            diff =  gamma_lut[a[x]] - gamma_lut[b[x]];
            sum += diff * diff;
        }
        a += stride;
        b += stride;
    }
    return sum;
}

// Sum of squared errors.  Computes and sums the SSEs for all
// 16x16 blocks in the images.  Only checks the Y component.
static float motion_metric( unsigned * gamma_lut, hb_buffer_t * a, hb_buffer_t * b )
{
    int bw = a->f.width / 16;
    int bh = a->f.height / 16;
    int stride = a->plane[0].stride;
    uint8_t * pa = a->plane[0].data;
    uint8_t * pb = b->plane[0].data;
    int x, y;
    uint64_t sum = 0;

    for( y = 0; y < bh; y++ )
    {
        for( x = 0; x < bw; x++ )
        {
            sum +=  sse_block16( gamma_lut, pa + y * 16 * stride + x * 16,
                                            pb + y * 16 * stride + x * 16,
                                            stride );
        }
    }
    return (float)sum / ( a->f.width * a->f.height );;
}

static void delete_metric(double * metrics, int pos, int size)
{
    double * dst   = &metrics[pos];
    double * src   = &metrics[pos + 1];
    int msize = (size - (pos + 1)) * sizeof(double);
    memmove(dst, src, msize);
}

static int find_drop_frame(hb_filter_private_t * pv, int count)
{
    int           ii, min;
    double      * metrics = pv->frame_metric;
    hb_buffer_t * buf, * first;
    double        cfr_stop;

    // compute where the second to last frame in the frame_rate_list would
    // stop if the frame rate were constant.
    //
    // this is our target stopping time for CFR and earliest possible
    // stopping time for PFR.
    cfr_stop = pv->out_last_stop + pv->frame_duration * (count - 1);

    // If the last frame's stop timestamp is before the calculated
    // CFR stop time of the second to last frame, then we need to drop a frame.
    buf   = hb_list_item(pv->frame_rate_list, count - 1);

    // Shortcut exit when entire list is CFR OK
    if (buf->s.stop >= (int64_t)cfr_stop)
    {
        return -1;
    }

    first = hb_list_item(pv->frame_rate_list, 0);
    min = 0;
    for (ii = 1; ii < count; ii++)
    {
        hb_buffer_t * buf = hb_list_item(pv->frame_rate_list, ii);
        // Don't check buffers outside analysis window
        if (buf->s.stop - first->s.start > pv->frame_analysis_duration)
        {
            break;
        }
        if (min < 0 || metrics[ii] < metrics[min])
        {
            min = ii;
        }
    }

    cfr_stop = pv->out_last_stop + pv->frame_duration * (ii - 1);
    buf      = hb_list_item(pv->frame_rate_list, ii - 1);

    // Don't drop buffers if frames fit in CRF time bounds
    if (buf->s.stop >= (int64_t)cfr_stop)
    {
        return -1;
    }

    return min;
}

// This section of the code implements video frame rate control.
// Since filters are allowed to duplicate and drop frames (which
// changes the timing), this has to be the last thing done in render.
//
// There are three options, selected by the value of cfr:
//   0 - Variable Frame Rate (VFR) or 'same as source': frame times
//       are left alone
//   1 - Constant Frame Rate (CFR): Frame timings are adjusted so that all
//       frames are exactly vrate.den ticks apart. Frames are dropped
//       or duplicated if necessary to maintain this spacing.
//   2 - Peak Frame Rate (PFR): vrate.den is treated as the peak
//       average frame rate. I.e., the average frame rate (current frame
//       end time divided by number of frames so far) is never allowed to be
//       greater than vrate.den and frames are dropped if necessary
//       to keep the average under this value. Other than those drops, frame
//       times are left alone.
//
static hb_buffer_t * adjust_frame_rate( hb_filter_private_t * pv,
                                        hb_buffer_t * in )
{

    if (pv->cfr == 0)
    {
        if (in)
        {
            ++pv->count_frames;
            pv->out_last_stop = in->s.stop;
        }
        return in;
    }

    int count;

    // in == NULL signals we are flushing the frame_rate_list
    if (in != NULL)
    {
        if (pv->out_last_stop == (int64_t)AV_NOPTS_VALUE)
        {
            pv->out_last_stop = in->s.start;
        }

#if defined(HB_DEBUG_CFR_DROPS)
        in->s.pcr = pv->sequence++;
#endif
        hb_list_add(pv->frame_rate_list, in);
        count = hb_list_count(pv->frame_rate_list);
        if (count < 2)
        {
            return NULL;
        }

        hb_buffer_t * penultimate, * ultimate;
        penultimate = hb_list_item(pv->frame_rate_list, count - 2);
        ultimate    = hb_list_item(pv->frame_rate_list, count - 1);

        pv->frame_metric[count - 1] = motion_metric(pv->gamma_lut,
                                                    penultimate, ultimate);

        if (count < pv->frame_analysis_depth)
        {
            return NULL;
        }
    }
    else
    {
        count = hb_list_count(pv->frame_rate_list);
    }

    hb_buffer_list_t   list;
    hb_buffer_t      * out;
    double             cfr_stop;
    int                drop_frame;

    hb_buffer_list_clear(&list);

    drop_frame = find_drop_frame(pv, count);
    if (drop_frame >= 0)
    {

        // We may have to drop multiple frames.  Pick frames to drop
        // that appear to have minimum motion.
        //
        // The main use case we are trying to cover here is
        // "progressive telecine" where there is a repeating pattern
        // of a new frame followed by some number of repeated frames.
        // We want to keep the "new frames" and drop the repeats.
        out = hb_list_item(pv->frame_rate_list, drop_frame);

#if defined(HB_DEBUG_CFR_DROPS)
        hb_log("CFR Drop: %ld metric %d", out->s.pcr, (int)pv->frame_metric[drop_frame]);
        int jj;
        for (jj = 0; jj < count; jj++)
        {
            if (jj == drop_frame)
            {
                fprintf(stderr, "(%4d) ", (int)pv->frame_metric[jj]);
            }
            else
            {
                fprintf(stderr, "%6d ", (int)pv->frame_metric[jj]);
            }
        }
        fprintf(stderr, "\n");
#endif

        hb_list_rem(pv->frame_rate_list, out);
        hb_buffer_close(&out);
        delete_metric(pv->frame_metric, drop_frame, count);
        ++pv->drops;
        return NULL;
    }

    out = hb_list_item(pv->frame_rate_list, 0);

#if defined(HB_DEBUG_CFR_DROPS)
    static int64_t lastpass = 0;
    hb_log("CFR Pass: %ld ~ %ld metric %d", out->s.pcr, out->s.pcr - lastpass, (int)pv->frame_metric[0]);
    lastpass = out->s.pcr;
#endif

    hb_list_rem(pv->frame_rate_list, out);
    hb_buffer_list_append(&list, out);
    delete_metric(pv->frame_metric, 0, count);

    // this frame has to start where the last one stopped.
    out->s.start = pv->out_last_stop;
    cfr_stop = pv->out_last_stop + pv->frame_duration;

    // at this point we know that this frame doesn't push the average
    // rate over the limit so we just pass it on for PFR. For CFR we're
    // going to return it (with its start & stop times modified) and
    // we may have to dup it.
    ++pv->count_frames;
    if (pv->cfr > 1)
    {
        // PFR - we're going to keep the frame but may need to
        // adjust it's stop time to meet the average rate constraint.
        if (out->s.stop < cfr_stop)
        {
            out->s.stop = pv->out_last_stop = cfr_stop;
        }
        else
        {
            pv->out_last_stop = out->s.stop;
        }
    }
    else
    {
        // we're doing CFR so we have to either trim some time from a
        // buffer that ends too far in the future or, if the buffer is
        // two or more frame times long, split it into multiple pieces,
        // each of which is a frame time long.
        double excess = (double)out->s.stop - cfr_stop;
        out->s.stop = pv->out_last_stop = cfr_stop;
        for (; excess >= pv->frame_duration; excess -= pv->frame_duration)
        {
            /* next frame too far ahead - dup current frame */
            hb_buffer_t *dup = hb_buffer_dup( out );
            dup->s.new_chap = 0;
            dup->s.start = cfr_stop;
            cfr_stop += pv->frame_duration;
            dup->s.stop = pv->out_last_stop = cfr_stop;
            hb_buffer_list_append(&list, dup);
            ++pv->dups;
            ++pv->count_frames;
        }
    }

    return hb_buffer_list_clear(&list);
}

static hb_buffer_t * flush_frames(hb_filter_private_t * pv)
{
    hb_buffer_list_t list;

    hb_buffer_list_clear(&list);
    while (hb_list_count(pv->frame_rate_list) > 0)
    {
        hb_buffer_list_append(&list, adjust_frame_rate(pv, NULL));
    }

    return hb_buffer_list_clear(&list);
}

static int hb_vfr_init(hb_filter_object_t *filter, hb_filter_init_t *init)
{
    filter->private_data    = calloc(1, sizeof(struct hb_filter_private_s));
    hb_filter_private_t *pv = filter->private_data;
    build_gamma_lut(pv);

    pv->cfr              = init->cfr;
    pv->input_vrate = pv->vrate = init->vrate;
    hb_dict_extract_int(&pv->cfr, filter->settings, "mode");
    hb_dict_extract_rational(&pv->vrate, filter->settings, "rate");

    // frame-drop analysis always looks at at least 2 buffers
    pv->frame_analysis_depth = 2;

    // Calculate the number of frames we need to keep in order to
    // detect "best" candidate frames to drop.
    double in_vrate  = (double)pv->input_vrate.num / pv->input_vrate.den;
    double out_vrate = (double)pv->vrate.num / pv->vrate.den;
    if (in_vrate > out_vrate)
    {
        // in_vrate / out_vrate tells us how many consecutive repeated
        // frames we can expect to see.  if the number of consecutive
        // repeated frame is < 2, we need the number of consecutive
        // non-repeated frames.  Then add 1 so that we should have
        // transitions that we can detect at both ends of a sequence.
        double factor = in_vrate / out_vrate;
        if (factor > 1.0 && factor < 2.0)
        {
            factor = 1 / (factor - 1);
        }
        pv->frame_analysis_depth = ceil(factor) + 1;

        // if we end up with an absurdly large value, limit it
        if (pv->frame_analysis_depth > MAX_FRAME_ANALYSIS_DEPTH)
        {
            pv->frame_analysis_depth = MAX_FRAME_ANALYSIS_DEPTH;
        }
    }
    pv->frame_analysis_duration = pv->frame_analysis_depth * 90000 / in_vrate;
    pv->frame_metric = calloc(pv->frame_analysis_depth, sizeof(double));
    pv->frame_metric[0] = INT_MAX;

    pv->job = init->job;

    /* Setup FIFO queue for subtitle cache */
    pv->delay_queue = hb_fifo_init( 8, 1 );
    pv->frame_rate_list = hb_list_init();


    /* VFR IVTC needs a bunch of time-keeping variables to track
      how many frames are dropped, how many are extended, what the
      last 4 start and stop times were (so they can be modified),
      how much time has been lost and gained overall, how much time
      the latest 4 frames should be extended by */
    pv->dropped_frames    = 0;
    pv->extended_frames   = 0;
    pv->last_start[0]     = 0;
    pv->last_stop[0]      = 0;
    pv->total_lost_time   = 0;
    pv->total_gained_time = 0;
    pv->lost_time[0]      = 0;
    pv->lost_time[1]      = 0;
    pv->lost_time[2]      = 0;
    pv->lost_time[3]      = 0;

    if (pv->cfr == 2)
    {
        // For PFR, we want the framerate based on the source's actual
        // framerate, unless it's higher than the specified peak framerate.
        double source_fps = (double)init->vrate.num / init->vrate.den;
        double peak_fps = (double)pv->vrate.num / pv->vrate.den;
        if (source_fps > peak_fps)
        {
            // peak framerate is lower than the source framerate.
            // so signal that the framerate will be the peak fps.
            init->vrate = pv->vrate;
        }
    }
    else
    {
        init->vrate = pv->vrate;
    }
    pv->frame_duration = (double)pv->vrate.den * 90000. / pv->vrate.num;
    pv->out_last_stop  = (int64_t)AV_NOPTS_VALUE;
    init->cfr          = pv->cfr;

    return 0;
}

static hb_filter_info_t * hb_vfr_info( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_filter_info_t    * info;

    if( !pv )
        return NULL;

    info = calloc(1, sizeof(hb_filter_info_t));
    info->human_readable_desc = malloc(128);
    info->human_readable_desc[0] = 0;

    info->output.vrate      = pv->input_vrate;
    if (pv->cfr == 2)
    {
        // For PFR, we want the framerate based on the source's actual
        // framerate, unless it's higher than the specified peak framerate.
        double source_fps = (double)pv->input_vrate.num / pv->input_vrate.den;
        double peak_fps = (double)pv->vrate.num / pv->vrate.den;
        if (source_fps > peak_fps)
        {
            // peak framerate is lower than the source framerate.
            // so signal that the framerate will be the peak fps.
            info->output.vrate = pv->vrate;
        }
    }
    else
    {
        info->output.vrate = pv->vrate;
    }
    info->output.cfr = pv->cfr;
    if ( pv->cfr == 0 )
    {
        /* Ensure we're using "Same as source" FPS */
        snprintf( info->human_readable_desc, 128,
                  "frame rate: same as source (around %.3f fps)",
                  (float)pv->vrate.num / pv->vrate.den );
    }
    else if ( pv->cfr == 2 )
    {
        // For PFR, we want the framerate based on the source's actual
        // framerate, unless it's higher than the specified peak framerate.
        double source_fps = (double)pv->input_vrate.num / pv->input_vrate.den;
        double peak_fps = (double)pv->vrate.num / pv->vrate.den;
        snprintf( info->human_readable_desc, 128,
                  "frame rate: %.3f fps -> peak rate limited to %.3f fps",
                  source_fps , peak_fps );
    }
    else
    {
        // Constant framerate. Signal the framerate we are using.
        double source_fps = (double)pv->input_vrate.num / pv->input_vrate.den;
        double constant_fps = (double)pv->vrate.num / pv->vrate.den;
        snprintf( info->human_readable_desc, 128,
                  "frame rate: %.3f fps -> constant %.3f fps",
                  source_fps , constant_fps );
    }

    return info;
}

static void hb_vfr_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if( !pv )
        return;

    if ( pv->cfr )
    {
        hb_log("vfr: %d frames output, %d dropped and %d duped for CFR/PFR",
               pv->count_frames, pv->drops, pv->dups );
    }
    else
    {
        hb_log("vfr: %d frames output, %d dropped",
               pv->count_frames, pv->drops);
    }

    if( pv->job )
    {
        hb_interjob_t * interjob = hb_interjob_get( pv->job->h );

        /* Preserve dropped frame count for more accurate
         * framerates in 2nd passes.
         */
        interjob->out_frame_count = pv->count_frames;
        interjob->total_time = pv->out_last_stop;
    }

    hb_log("vfr: lost time: %"PRId64" (%i frames)",
           pv->total_lost_time, pv->dropped_frames);
    hb_log("vfr: gained time: %"PRId64" (%i frames) (%"PRId64" not accounted for)",
           pv->total_gained_time, pv->extended_frames,
           pv->total_lost_time - pv->total_gained_time);

    if (pv->dropped_frames)
    {
        hb_log("vfr: average dropped frame duration: %"PRId64,
               (pv->total_lost_time / pv->dropped_frames) );
    }

    if( pv->delay_queue )
    {
        hb_fifo_close( &pv->delay_queue );
    }
    free(pv->frame_metric);
    hb_list_close(&pv->frame_rate_list);

    /* Cleanup render work structure */
    free( pv );
    filter->private_data = NULL;
}

static int hb_vfr_work( hb_filter_object_t * filter,
                        hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t         * in = *buf_in;
    hb_buffer_t         * out = NULL;

    *buf_in = NULL;
    *buf_out = NULL;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        hb_buffer_list_t   list;
        hb_buffer_t      * next;
        int                counter = 2;

        // Flush the delay_queue and frame rate adjustment
        hb_buffer_list_clear(&list);
        while ((next = hb_fifo_get(pv->delay_queue)) != NULL)
        {

            /* We can't use the given time stamps. Previous frames
               might already have been extended, throwing off the
               raw values fed to render.c. Instead, their
               stop and start times are stored in arrays.
               The 4th cached frame will be the to use.
               If it needed its duration extended to make up
               lost time, it will have happened above. */
            next->s.start = pv->last_start[counter];
            next->s.stop  = pv->last_stop[counter--];

            hb_buffer_list_append(&list, adjust_frame_rate(pv, next));
        }
        hb_buffer_list_append(&list, flush_frames(pv));
        hb_buffer_list_append(&list, in);
        *buf_out = hb_buffer_list_clear(&list);
        return HB_FILTER_DONE;
    }

    // If there is a gap between the last stop and the current start
    // then frame(s) were dropped.
    if (hb_fifo_size(pv->delay_queue) > 0 && in->s.start > pv->last_stop[0])
    {
        /* We need to compensate for the time lost by dropping frame(s).
           Spread its duration out in quarters, because usually dropped frames
           maintain a 1-out-of-5 pattern and this spreads it out amongst
           the remaining ones.  Store these in the lost_time array, which
           has 4 slots in it.  Because not every frame duration divides
           evenly by 4, and we can't lose the remainder, we have to go
           through an awkward process to preserve it in the 4th array index.
        */
        int64_t temp_duration = in->s.start - pv->last_stop[0];
        pv->lost_time[0] += (temp_duration / 4);
        pv->lost_time[1] += (temp_duration / 4);
        pv->lost_time[2] += (temp_duration / 4);
        pv->lost_time[3] += ( temp_duration - 3 * (temp_duration / 4) );

        pv->total_lost_time += temp_duration;
    }
    else if ( in->s.stop <= pv->last_stop[0] )
    {
        // This is generally an error somewhere (bad source or hb bug).
        // But lets do our best to straighten out the mess.
        ++pv->drops;
        hb_buffer_close(&in);
        return HB_FILTER_OK;
    }

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
    if (hb_fifo_size(pv->delay_queue) == 0)
    {
        pv->last_start[0] = in->s.start;
        pv->last_stop[0]  = in->s.stop;
    }
    else
    {
        pv->last_start[0] = pv->last_stop[1];
        pv->last_stop[0] = pv->last_start[0] + (in->s.stop - in->s.start);
    }

    hb_fifo_push( pv->delay_queue, in );

    /*
     * Keep the last three frames in our queue, this ensures that we have
     * the last two always in there should we need to rewrite the
     * durations on them.
     */

    if (hb_fifo_size(pv->delay_queue) < 4)
    {
        *buf_out = NULL;
        return HB_FILTER_OK;
    }

    out = hb_fifo_get(pv->delay_queue);
    /* The current frame exists. That means it hasn't been dropped by a
     * filter. We may edit its duration if needed.
     */
    if( pv->lost_time[3] > 0 )
    {
        int time_shift = 0;

        for( i = 3; i >= 0; i-- )
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
            pv->last_start[i] += time_shift;
            pv->last_stop[i] += pv->lost_time[i] + time_shift;

            /* Log how much time has been added back in to the video. */
            pv->total_gained_time += pv->lost_time[i];
            time_shift += pv->lost_time[i];

            pv->lost_time[i] = 0;

            /* Log how many frames have had their durations extended. */
            pv->extended_frames++;
        }
    }

    /* We can't use the given time stamps. Previous frames
       might already have been extended, throwing off the
       raw values fed to render.c. Instead, their
       stop and start times are stored in arrays.
       The 4th cached frame will be the to use.
       If it needed its duration extended to make up
       lost time, it will have happened above. */
    out->s.start = pv->last_start[3];
    out->s.stop = pv->last_stop[3];

    *buf_out = adjust_frame_rate(pv, out);

    return HB_FILTER_OK;
}


