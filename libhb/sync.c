/* sync.c

   Copyright (c) 2003-2016 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "hbffmpeg.h"
#include <stdio.h>
#include "samplerate.h"

#define SYNC_MAX_VIDEO_QUEUE_LEN    20
#define SYNC_MIN_VIDEO_QUEUE_LEN    12

// Audio is small, buffer a lot.  It helps to ensure that we see
// the initial PTS from all input streams before setting the 'zero' point.
#define SYNC_MAX_AUDIO_QUEUE_LEN    60
#define SYNC_MIN_AUDIO_QUEUE_LEN    30

#define SYNC_MAX_SUBTITLE_QUEUE_LEN INT_MAX
#define SYNC_MIN_SUBTITLE_QUEUE_LEN 0

typedef enum
{
    SYNC_TYPE_VIDEO,
    SYNC_TYPE_AUDIO,
    SYNC_TYPE_SUBTITLE,
} sync_type_t;

typedef struct
{
    int64_t pts;
    int64_t delta;
} sync_delta_t;

typedef struct
{
    int              link;
    int              merge;
    hb_buffer_list_t list_current;
} subtitle_sanitizer_t;

typedef struct sync_common_s sync_common_t;

typedef struct
{
    sync_common_t     * common;

    // Stream I/O control
    hb_list_t         * in_queue;
    int                 max_len;
    int                 min_len;
    hb_cond_t         * cond_full;
    hb_fifo_t         * fifo_out;

    // PTS synchronization
    hb_list_t         * delta_list;
    int64_t             pts_slip;
    double              next_pts;

    // frame statistics
    int64_t             first_pts;
    int64_t             min_frame_duration;
    int64_t             max_frame_duration;
    int64_t             current_duration;
    int                 frame_count;

    // Error reporting stats
    int64_t             drop_duration;
    int                 drop;
    int64_t             drop_pts;
    int64_t             gap_duration;
    int64_t             gap_pts;

    int                 first_frame;

    // stream type specific context
    sync_type_t         type;
    union
    {
        struct
        {
            int     cadence[12];
        } video;

        // Audio stream context
        struct
        {
            hb_audio_t     * audio;

            // Audio filter settings
            // Samplerate conversion
            struct
            {
                SRC_STATE  * ctx;
                SRC_DATA     pkt;
            } src;
            double           gain_factor;
        } audio;

        // Subtitle stream context
        struct
        {
            hb_subtitle_t        * subtitle;
            subtitle_sanitizer_t   sanitizer;
        } subtitle;
    };
} sync_stream_t;

struct sync_common_s
{
    /* Audio/Video sync thread synchronization */
    hb_job_t      * job;
    hb_lock_t     * mutex;
    int             stream_count;
    sync_stream_t * streams;
    int             found_first_pts;
    int             done;

    // point-to-point support
    int         start_found;

    // sync audio work objects
    hb_list_t * list_work;

    // UpdateState Statistics
    int        est_frame_count;
    uint64_t   st_counts[4];
    uint64_t   st_dates[4];
    uint64_t   st_first;

    int             chapter;
};

struct hb_work_private_s
{
    sync_common_t * common;
    sync_stream_t * stream;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void UpdateState( sync_common_t * common, int frame_count );
static void UpdateSearchState( sync_common_t * common, int64_t start,
                               int frame_count );
static hb_buffer_t * FilterAudioFrame( sync_stream_t * stream,
                                       hb_buffer_t *buf );
static hb_buffer_t * sanitizeSubtitle(sync_stream_t        * stream,
                                      hb_buffer_t          * sub);


static int fillQueues( sync_common_t * common )
{
    int ii, wait = 0, abort = 0;

    for (ii = 0; ii < common->stream_count; ii++)
    {
        sync_stream_t *stream = &common->streams[ii];

        // Don't let the queues grow indefinitely
        // abort when too large
        if (hb_list_count(stream->in_queue) > stream->max_len)
        {
            abort = 1;
        }
        if (hb_list_count(stream->in_queue) < stream->min_len)
        {
            wait = 1;
        }
    }
    return !wait || abort;
}

static void signalBuffer( sync_stream_t * stream )
{
    if (hb_list_count(stream->in_queue) < stream->max_len)
    {
        hb_cond_signal(stream->cond_full);
    }
}

static void allSlip( sync_common_t * common, int64_t delta )
{
    int ii;
    for (ii = 0; ii < common->stream_count; ii++)
    {
        common->streams[ii].pts_slip += delta;
        if (common->streams[ii].next_pts != (int64_t)AV_NOPTS_VALUE)
        {
            common->streams[ii].next_pts -= delta;
        }
    }
}

static void shiftTS( sync_common_t * common, int64_t delta )
{
    int ii, jj;

    allSlip(common, delta);
    for (ii = 0; ii < common->stream_count; ii++)
    {
        sync_stream_t * stream = &common->streams[ii];
        int count = hb_list_count(stream->in_queue);
        for (jj = 0; jj < count; jj++)
        {
            hb_buffer_t * buf = hb_list_item(stream->in_queue, jj);
            buf->s.start -= delta;
            if (buf->s.stop != AV_NOPTS_VALUE)
            {
                buf->s.stop  -= delta;
            }
        }
    }
}

static void checkFirstPts( sync_common_t * common )
{
    int ii;
    int64_t first_pts = INT64_MAX;

    for (ii = 0; ii < common->stream_count; ii++)
    {
        sync_stream_t *stream = &common->streams[ii];
        if (stream->type == SYNC_TYPE_SUBTITLE)
        {
            // only wait for audio and video
            continue;
        }

        // If buffers are queued, find the lowest initial PTS
        if (hb_list_count(stream->in_queue) > 0)
        {
            hb_buffer_t * buf = hb_list_item(stream->in_queue, 0);
            if (first_pts > buf->s.start)
            {
                first_pts = buf->s.start;
            }
        }
    }
    if (first_pts != INT64_MAX)
    {
        // Add a fudge factor to first pts to prevent negative
        // timestamps from leaking through.  The pipeline can
        // handle a positive offset, but some things choke on
        // negative offsets
        //first_pts -= 500000;
        shiftTS(common, first_pts);
    }
    common->found_first_pts = 1;
}

static void addDelta( sync_common_t * common, int64_t start, int64_t delta)
{
    int ii;

    for (ii = 0; ii < common->stream_count; ii++)
    {
        sync_delta_t * delta_item = malloc(sizeof(sync_delta_t));
        delta_item->pts = start;
        delta_item->delta = delta;
        hb_list_add(common->streams[ii].delta_list, delta_item);
    }
}

static void applyDeltas( sync_common_t * common )
{
    int     ii;

    // Apply delta to any applicable buffers in the queue
    for (ii = 0; ii < common->stream_count; ii++)
    {
        sync_stream_t * stream = &common->streams[ii];

        // Make adjustments for deltas found in other streams
        sync_delta_t * delta = hb_list_item(stream->delta_list, 0);
        if (delta != NULL)
        {
            int           jj, index = -1;
            int64_t       prev_start, max = 0;
            hb_buffer_t * buf;

            prev_start = stream->next_pts;
            for (jj = 0; jj < hb_list_count(stream->in_queue); jj++)
            {
                buf = hb_list_item(stream->in_queue, jj);
                if (stream->type == SYNC_TYPE_SUBTITLE)
                {
                    if (buf->s.start > delta->pts)
                    {
                        // absorb gaps in subtitles as soon as possible
                        index = jj;
                        break;
                    }
                }
                else if (buf->s.start > delta->pts)
                {
                    // absorb gap in largest gap found in this stream.
                    if (buf->s.start - prev_start > max)
                    {
                        max = buf->s.start - prev_start;
                        index = jj;
                    }
                    if (stream->type == SYNC_TYPE_AUDIO && max >= delta->delta)
                    {
                        // absorb gaps in audio as soon as possible
                        // when there is a gap that will absorb it.
                        break;
                    }
                }
                prev_start = buf->s.start;
            }

            if (index >= 0)
            {
                for (jj = index; jj < hb_list_count(stream->in_queue); jj++)
                {
                    buf = hb_list_item(stream->in_queue, jj);
                    buf->s.start -= delta->delta;
                    if (buf->s.stop != AV_NOPTS_VALUE)
                    {
                        buf->s.stop  -= delta->delta;
                    }
                }
                // Correct the duration of the video buffer before
                // the affected timestamp correction.
                if (stream->type == SYNC_TYPE_VIDEO && index > 0)
                {
                    buf = hb_list_item(stream->in_queue, index - 1);
                    if (buf->s.duration > delta->delta)
                    {
                        buf->s.duration -= delta->delta;
                        buf->s.stop -= delta->delta;
                    }
                    else
                    {
                        buf->s.duration = 0;
                        buf->s.stop = buf->s.start;
                    }
                }
                stream->pts_slip += delta->delta;
                hb_list_rem(stream->delta_list, delta);
                free(delta);
            }
        }
    }
}

static void removeVideoJitter( sync_stream_t * stream, int stop )
{
    int           ii;
    hb_buffer_t * buf;
    double        frame_duration, next_pts;

    frame_duration = 90000. * stream->common->job->title->vrate.den /
                              stream->common->job->title->vrate.num;

    buf = hb_list_item(stream->in_queue, 0);
    buf->s.start = stream->next_pts;
    next_pts = stream->next_pts + frame_duration;
    for (ii = 1; ii <= stop; ii++)
    {
        buf->s.duration = frame_duration;
        buf->s.stop = next_pts;
        buf = hb_list_item(stream->in_queue, ii);
        buf->s.start = next_pts;
        next_pts += frame_duration;
    }
}

// Look for a sequence of packets whose duration as measure by
// vrate closely matches the duration as measured by timestamp
// differences.  When close matches are found, smooth the timestamps.
//
// Most often, video dejitter is applied when there is jitter due to
// soft telecine.  I also have a couple sample files that have very
// bad video jitter that this corrects.
static void dejitterVideo( sync_stream_t * stream )
{
    int           ii, count, jitter_stop;
    double        frame_duration, duration;
    hb_buffer_t * buf;

    count = hb_list_count(stream->in_queue);
    if (count < 2)
    {
        return;
    }

    frame_duration = 90000. * stream->common->job->title->vrate.den /
                              stream->common->job->title->vrate.num;

    // Look for start of jittered sequence
    buf      = hb_list_item(stream->in_queue, 1);
    duration = buf->s.start - stream->next_pts;
    if (ABS(duration - frame_duration) < 1.1)
    {
        // Ignore small jitter
        return;
    }

    // Look for end of jittered sequence
    jitter_stop = 0;
    for (ii = 1; ii < count; ii++)
    {
        buf      = hb_list_item(stream->in_queue, ii);
        duration = buf->s.start - stream->next_pts;

        // Only dejitter video that aligns periodically
        // with the frame durations.
        if (ABS(duration - ii * frame_duration) < 0.1)
        {
            jitter_stop = ii;
        }
    }

    if (jitter_stop > 0)
    {
        removeVideoJitter(stream, jitter_stop);
    }
}

// Fix video overlaps that could not be corrected by dejitter
static void fixVideoOverlap( sync_stream_t * stream )
{
    int           drop = 0, new_chap = 0;
    int64_t       overlap;
    hb_buffer_t * buf;

    if (!stream->first_frame)
    {
        // There are no overlaps if we haven't seen the first frame yet.
        return;
    }

    // If time goes backwards drop the frame.
    // Check if subsequent buffers also overlap.
    while ((buf = hb_list_item(stream->in_queue, 0)) != NULL)
    {
        // For video, an overlap is where the entire frame is
        // in the past.
        overlap = stream->next_pts - buf->s.stop;
        if (overlap >= 0)
        {
            if (stream->drop == 0)
            {
                stream->drop_pts = buf->s.start;
            }
            // Preserve chapter marks
            if (buf->s.new_chap > 0)
            {
                new_chap = buf->s.new_chap;
            }
            hb_list_rem(stream->in_queue, buf);
            signalBuffer(stream);
            stream->drop_duration += buf->s.duration;
            stream->drop++;
            drop++;
            hb_buffer_close(&buf);
        }
        else
        {
            if (new_chap > 0)
            {
                buf->s.new_chap = new_chap;
            }
            break;
        }
    }

    if (drop <= 0 && stream->drop > 0)
    {
        hb_log("sync: video time went backwards %d ms, dropped %d frames. "
               "PTS %"PRId64"",
               (int)stream->drop_duration / 90, stream->drop, stream->drop_pts);
        stream->drop_duration = 0;
        stream->drop = 0;
    }
}

static void removeAudioJitter(sync_stream_t * stream, int stop)
{
    int           ii;
    hb_buffer_t * buf;
    double        next_pts;

    // If duration of sum of packet durations is close to duration
    // as measured by timestamps, align timestamps to packet durations.
    // The packet durations are computed based on samplerate and
    // number of samples and are therefore a reliable measure
    // of the actual duration of an audio frame.
    buf = hb_list_item(stream->in_queue, 0);
    buf->s.start = stream->next_pts;
    next_pts = stream->next_pts + buf->s.duration;
    for (ii = 1; ii <= stop; ii++)
    {
        // Duration can be fractional, so track fractional PTS
        buf->s.stop = next_pts;
        buf = hb_list_item(stream->in_queue, ii);
        buf->s.start = next_pts;
        next_pts += buf->s.duration;
    }
}

// Look for a sequence of packets whose duration as measure by packet
// durations closely matches the duration as measured by timestamp
// differences.  When close matches are found, smooth the timestamps.
//
// This fixes issues where there are false overlaps and gaps in audio
// timestamps.  libav creates this type of problem sometimes with it's
// timestamp guessing code.
static void dejitterAudio( sync_stream_t * stream )
{
    int           ii, count, jitter_stop;
    double        duration;
    hb_buffer_t * buf, * buf0, * buf1;

    count = hb_list_count(stream->in_queue);
    if (count < 4)
    {
        return;
    }

    // Look for start of jitter sequence
    jitter_stop = 0;
    buf0 = hb_list_item(stream->in_queue, 0);
    buf1 = hb_list_item(stream->in_queue, 1);
    if (ABS(buf0->s.duration - (buf1->s.start - stream->next_pts)) < 1.1)
    {
        // Ignore very small jitter
        return;
    }
    buf = hb_list_item(stream->in_queue, 0);
    duration = buf->s.duration;

    // Look for end of jitter sequence
    for (ii = 1; ii < count; ii++)
    {
        buf = hb_list_item(stream->in_queue, ii);
        if (ABS(duration - (buf->s.start - stream->next_pts)) < (90 * 40))
        {
            // Finds the largest span that has low jitter
            jitter_stop = ii;
        }
        duration += buf->s.duration;
    }
    if (jitter_stop >= 4)
    {
        removeAudioJitter(stream, jitter_stop);
    }
}

// Fix audio gaps that could not be corrected with dejitter
static void fixAudioGap( sync_stream_t * stream )
{
    int64_t       gap;
    hb_buffer_t * buf;

    if (hb_list_count(stream->in_queue) < 1 || !stream->first_frame)
    {
        // Can't find gaps with < 1 buffers
        return;
    }

    buf  = hb_list_item(stream->in_queue, 0);
    gap = buf->s.start - stream->next_pts;

    // there's a gap of more than a minute between the last
    // frame and this. assume we got a corrupted timestamp
    if (gap > 90 * 20 && gap < 90000LL * 60)
    {
        if (stream->gap_duration <= 0)
        {
            stream->gap_pts = buf->s.start;
        }
        addDelta(stream->common, stream->next_pts, gap);
        applyDeltas(stream->common);
        stream->gap_duration += gap;
    }
    else
    {
        // If not fixing the gap, carry to the next frame
        // so they do not accumulate. Do not carry negative gaps.
        // Those are overlaps and are handled by fixAudioOverlap.
        if (gap >= 90000LL * 60)
        {
            // Fix "corrupted" timestamp
            buf->s.start = stream->next_pts;
        }
        if (stream->gap_duration > 0)
        {
            hb_deep_log(3, "sync: audio 0x%x time gap %d ms. PTS %"PRId64"",
                        stream->audio.audio->id, (int)stream->gap_duration / 90,
                        stream->gap_pts);
            stream->gap_duration = 0;
        }
    }
}

// Fix audio overlaps that could not be corrected with dejitter
static void fixAudioOverlap( sync_stream_t * stream )
{
    int           drop = 0;
    int64_t       overlap;
    hb_buffer_t * buf;

    if (!stream->first_frame)
    {
        // There are no overlaps if we haven't seen the first frame yet.
        return;
    }

    // If time goes backwards drop the frame.
    // Check if subsequent buffers also overlap.
    while ((buf = hb_list_item(stream->in_queue, 0)) != NULL)
    {
        overlap = stream->next_pts - buf->s.start;
        if (overlap > 90 * 20)
        {
            if (stream->drop == 0)
            {
                stream->drop_pts = buf->s.start;
            }
            // This is likely to generate a gap in audio timestamps
            // This will be subsequently handled by the call to
            // fix AudioGap in Synchronize(). Small gaps will be handled
            // by just shifting the timestamps and carrying the gap
            // along.
            hb_list_rem(stream->in_queue, buf);
            signalBuffer(stream);
            stream->drop_duration += buf->s.duration;
            stream->drop++;
            drop++;
            hb_buffer_close(&buf);
        }
        else
        {
            break;
        }
    }

    if (drop <= 0 && stream->drop > 0)
    {
        hb_log("sync: audio 0x%x time went backwards %d ms, dropped %d frames. "
               "PTS %"PRId64"",
               stream->audio.audio->id, (int)stream->drop_duration / 90,
               stream->drop, stream->drop_pts);
        stream->drop_duration = 0;
        stream->drop = 0;
    }
}

static void fixStreamTimestamps( sync_stream_t * stream )
{
    // Fix gaps and overlaps in queue
    if (stream->type == SYNC_TYPE_AUDIO)
    {
        dejitterAudio(stream);
        fixAudioOverlap(stream);
        fixAudioGap(stream);
    }
    else if (stream->type == SYNC_TYPE_VIDEO)
    {
        dejitterVideo(stream);
        fixVideoOverlap(stream);
    }
}

static void fifo_push( hb_fifo_t * fifo, hb_buffer_t * buf )
{
    if (fifo != NULL)
    {
        hb_fifo_push(fifo, buf);
    }
    else
    {
        hb_buffer_close(&buf);
    }
}

static void sendEof( sync_common_t * common )
{
    int ii;

    for (ii = 0; ii < common->stream_count; ii++)
    {
        fifo_push(common->streams[ii].fifo_out, hb_buffer_eof_init());
    }
}

static void streamFlush( sync_stream_t * stream )
{
    hb_lock(stream->common->mutex);

    while (hb_list_count(stream->in_queue) > 0)
    {
        if (!stream->common->found_first_pts)
        {
            checkFirstPts(stream->common);
        }
        fixStreamTimestamps(stream);
        hb_buffer_t * buf = hb_list_item(stream->in_queue, 0);
        if (buf != NULL)
        {
            hb_list_rem(stream->in_queue, buf);
            if (!stream->first_frame && buf->s.start >= 0)
            {
                switch (stream->type)
                {
                    case SYNC_TYPE_VIDEO:
                        hb_log("sync: first pts video is %"PRId64,
                               buf->s.start);
                        break;
                    case SYNC_TYPE_AUDIO:
                        hb_log("sync: first pts audio 0x%x is %"PRId64,
                               stream->audio.audio->id, buf->s.start);
                        break;
                    case SYNC_TYPE_SUBTITLE:
                        hb_log("sync: first pts subtitle 0x%x is %"PRId64,
                               stream->subtitle.subtitle->id, buf->s.start);
                        break;
                    default:
                        break;
                }
                stream->first_frame = 1;
                stream->first_pts = buf->s.start;
                stream->next_pts  = buf->s.start;
                stream->min_frame_duration = buf->s.duration;
            }
            if (stream->type == SYNC_TYPE_AUDIO)
            {
                buf = FilterAudioFrame(stream, buf);
                if (buf == NULL)
                {
                    // FilterAudioFrame can consume the buffer with no output
                    continue;
                }
            }
            int64_t subtitle_next_pts = AV_NOPTS_VALUE;
            if (stream->type == SYNC_TYPE_SUBTITLE)
            {
                buf = sanitizeSubtitle(stream, buf);
                if (buf == NULL)
                {
                    // sanitizeSubtitle can consume the buffer with no output
                    continue;
                }
                // sanitizeSubtitle can return a list of subtitles.
                // Find the pts of the last subtitle in the list
                hb_buffer_t * sub = buf;
                while (sub != NULL)
                {
                    subtitle_next_pts = sub->s.start;
                    sub = sub->next;
                }
            }
            if (stream->type == SYNC_TYPE_AUDIO ||
                stream->type == SYNC_TYPE_VIDEO)
            {
                buf->s.start = stream->next_pts;
                buf->s.stop  = stream->next_pts + buf->s.duration;
                stream->next_pts += buf->s.duration;
            }
            else
            {
                stream->next_pts = subtitle_next_pts;
            }

            if (buf->s.stop > 0)
            {
                stream->current_duration = buf->s.stop - stream->first_pts;
            }
            stream->frame_count++;
            if (buf->s.duration > 0 &&
                stream->min_frame_duration > buf->s.duration)
            {
                stream->min_frame_duration = buf->s.duration;
            }
            if (stream->max_frame_duration < buf->s.duration)
            {
                stream->max_frame_duration = buf->s.duration;
            }
            if ((buf->s.start < 0) ||
                (stream->type == SYNC_TYPE_VIDEO && buf->s.duration < 256))
            {
                // The pipeline can't handle negative timestamps
                // and it is sometimes not possible to avoid one
                // at the start of the video.  There can be a
                // significant delay before we see the first buffers
                // from all streams.  We can't buffer indefinitely
                // until we have seen the first PTS for all streams
                // so sometimes we may start before we have seen
                // the earliest PTS
                //
                // Also, encx264.c can't handle timestamps that are spaced
                // less than 256 ticks apart.
                hb_buffer_close(&buf);
            }
            fifo_push(stream->fifo_out, buf);
        }
    }
    fifo_push(stream->fifo_out, hb_buffer_eof_init());

    hb_unlock(stream->common->mutex);
}

static void log_chapter( sync_common_t *common, int chap_num,
                         int nframes, int64_t pts )
{
    hb_chapter_t *c;

    if ( !common->job )
        return;

    c = hb_list_item( common->job->list_chapter, chap_num - 1 );
    if ( c && c->title )
    {
        hb_log("sync: \"%s\" (%d) at frame %d time %"PRId64,
               c->title, chap_num, nframes, pts);
    }
    else
    {
        hb_log("sync: Chapter %d at frame %d time %"PRId64,
               chap_num, nframes, pts );
    }
}

#define TOP_FIRST PIC_FLAG_TOP_FIELD_FIRST
#define PROGRESSIVE PIC_FLAG_PROGRESSIVE_FRAME
#define REPEAT_FIRST PIC_FLAG_REPEAT_FIRST_FIELD
#define TB 8
#define BT 16
#define BT_PROG 32
#define BTB_PROG 64
#define TB_PROG 128
#define TBT_PROG 256

static void checkCadence( int * cadence, hb_buffer_t * buf )
{
    /*  Rotate the cadence tracking. */
    int i = 0;
    for (i = 11; i > 0; i--)
    {
        cadence[i] = cadence[i-1];
    }

    if (!(buf->s.flags & PROGRESSIVE) && !(buf->s.flags & TOP_FIRST))
    {
        /* Not progressive, not top first...
           That means it's probably bottom
           first, 2 fields displayed.
        */
        //hb_log("MPEG2 Flag: Bottom field first, 2 fields displayed.");
        cadence[0] = BT;
    }
    else if (!(buf->s.flags & PROGRESSIVE) && (buf->s.flags & TOP_FIRST))
    {
        /* Not progressive, top is first,
           Two fields displayed.
        */
        //hb_log("MPEG2 Flag: Top field first, 2 fields displayed.");
        cadence[0] = TB;
    }
    else if ((buf->s.flags & PROGRESSIVE) &&
             !(buf->s.flags & TOP_FIRST) && !(buf->s.flags & REPEAT_FIRST))
    {
        /* Progressive, but noting else.
           That means Bottom first,
           2 fields displayed.
        */
        //hb_log("MPEG2 Flag: Progressive. Bottom field first, 2 fields displayed.");
        cadence[0] = BT_PROG;
    }
    else if ((buf->s.flags & PROGRESSIVE) &&
             !(buf->s.flags & TOP_FIRST) && (buf->s.flags & REPEAT_FIRST))
    {
        /* Progressive, and repeat. .
           That means Bottom first,
           3 fields displayed.
        */
        //hb_log("MPEG2 Flag: Progressive repeat. Bottom field first, 3 fields displayed.");
        cadence[0] = BTB_PROG;
    }
    else if ((buf->s.flags & PROGRESSIVE) &&
             (buf->s.flags & TOP_FIRST) && !(buf->s.flags & REPEAT_FIRST))
    {
        /* Progressive, top first.
           That means top first,
           2 fields displayed.
        */
        //hb_log("MPEG2 Flag: Progressive. Top field first, 2 fields displayed.");
        cadence[0] = TB_PROG;
    }
    else if ((buf->s.flags & PROGRESSIVE) &&
             (buf->s.flags & TOP_FIRST) && (buf->s.flags & REPEAT_FIRST))
    {
        /* Progressive, top, repeat.
           That means top first,
           3 fields displayed.
        */
        //hb_log("MPEG2 Flag: Progressive repeat. Top field first, 3 fields displayed.");
        cadence[0] = TBT_PROG;
    }

    if ((cadence[2] <= TB) && (cadence[1] <= TB) &&
        (cadence[0] > TB) && (cadence[11]))
    {
        hb_log("%fs: Video -> Film", (float)buf->s.start / 90000);
    }
    if ((cadence[2] > TB) && (cadence[1] <= TB) &&
        (cadence[0] <= TB) && (cadence[11]))
    {
        hb_log("%fs: Film -> Video", (float)buf->s.start / 90000);
    }
}

// OutputBuffer pulls buffers from the internal sync buffer queues in
// lowest PTS first order.  It then processes the queue the buffer is
// pulled from for frame overlaps and gaps.
//
// If a queue reaches MAX depth, it is possible another queue is too low
// to achieve both goals of pulling lowest PTS first *and* perform
// timestamp correction.  In this scenario, we forego lowest PTS and pull
// the next lowest PTS that has enough buffers in the queue to perform
// timestamp correction.
static void OutputBuffer( sync_common_t * common )
{
    int             ii, full;
    int64_t         pts;
    sync_stream_t * out_stream;
    hb_buffer_t   * buf;

    if (common->done)
    {
        // It is possible to get here when one stream triggers
        // end of output (i.e. pts_to_stop or frame_to_stop) while
        // another stream is waiting on the mutex.
        return;
    }
    do
    {
        full        = 0;
        out_stream  = NULL;
        pts         = INT64_MAX;

        // Find lowest PTS and output that buffer
        for (ii = 0; ii < common->stream_count; ii++)
        {
            sync_stream_t * stream = &common->streams[ii];
            // We need at least 2 buffers in the queue in order to fix
            // frame overlaps and inter-frame gaps.  So if a queue is
            // low, do not do normal PTS interleaving with this queue.
            // Except for subtitles which are not processed for gaps
            // and overlaps.
            if (hb_list_count(stream->in_queue) > stream->min_len)
            {
                buf = hb_list_item(stream->in_queue, 0);
                if (buf->s.start < pts)
                {
                    pts = buf->s.start;
                    out_stream = stream;
                }
            }
            // But continue output of buffers as long as one of the queues
            // is above the maximum queue level.
            if(hb_list_count(stream->in_queue) > stream->max_len)
            {
                full = 1;
            }
        }
        if (out_stream == NULL)
        {
            // This should only happen if all queues are below the
            // minimum queue level
            break;
        }

        if (out_stream->next_pts == (int64_t)AV_NOPTS_VALUE)
        {
            // Initialize next_pts, it is used to make timestamp corrections
            buf = hb_list_item(out_stream->in_queue, 0);
            out_stream->next_pts  = buf->s.start;
        }

        // Make timestamp adjustments to eliminate jitter, gaps, and overlaps
        fixStreamTimestamps(out_stream);

        buf = hb_list_item(out_stream->in_queue, 0);
        if (buf == NULL)
        {
            // In case some timestamp sanitization causes the one and
            // only buffer in the queue to be deleted...
            // This really shouldn't happen.
            continue;
        }
        if (!common->start_found)
        {
            // pts_to_start or frame_to_start were specified.
            // Wait for the appropriate start point.
            if (out_stream->type == SYNC_TYPE_VIDEO &&
                common->job->frame_to_start > 0 &&
                out_stream->frame_count >= common->job->frame_to_start)
            {
                common->start_found = 1;
                out_stream->frame_count = 0;
            }
            else if (common->job->pts_to_start > 0 &&
                     buf->s.start >= common->job->pts_to_start)
            {
                common->start_found = 1;
                common->streams[0].frame_count = 0;
            }
            else
            {
                if (out_stream->type == SYNC_TYPE_VIDEO)
                {
                    UpdateSearchState(common, buf->s.start,
                                      out_stream->frame_count);
                    out_stream->frame_count++;
                }
                hb_list_rem(out_stream->in_queue, buf);
                signalBuffer(out_stream);
                hb_buffer_close(&buf);
                continue;
            }
            // reset frame count to track number of frames after
            // the start position till the end of encode.
            shiftTS(common, buf->s.start);
        }

        // If pts_to_stop or frame_to_stop were specified, stop output
        if (common->job->pts_to_stop &&
            buf->s.start >= common->job->pts_to_stop )
        {
            hb_log("sync: reached pts %"PRId64", exiting early", buf->s.start);
            common->done = 1;
            sendEof(common);
            return;
        }
        if (out_stream->type == SYNC_TYPE_VIDEO &&
            common->job->frame_to_stop &&
            out_stream->frame_count >= common->job->frame_to_stop)
        {
            hb_log("sync: reached %d frames, exiting early",
                   out_stream->frame_count);
            common->done = 1;
            sendEof(common);
            return;
        }

        if (out_stream->type == SYNC_TYPE_VIDEO)
        {
            checkCadence(out_stream->video.cadence, buf);
        }

        // Out the buffer goes...
        hb_list_rem(out_stream->in_queue, buf);
        signalBuffer(out_stream);
        if (out_stream->type == SYNC_TYPE_VIDEO)
        {
            UpdateState(common, out_stream->frame_count);
        }
        if (!out_stream->first_frame && buf->s.start >= 0)
        {
            switch (out_stream->type)
            {
                case SYNC_TYPE_VIDEO:
                    hb_log("sync: first pts video is %"PRId64,
                           buf->s.start);
                    break;
                case SYNC_TYPE_AUDIO:
                    hb_log("sync: first pts audio 0x%x is %"PRId64,
                           out_stream->audio.audio->id, buf->s.start);
                    break;
                case SYNC_TYPE_SUBTITLE:
                    hb_log("sync: first pts subtitle 0x%x is %"PRId64,
                           out_stream->subtitle.subtitle->id, buf->s.start);
                    break;
                default:
                    break;
            }
            out_stream->first_frame        = 1;
            out_stream->first_pts          = buf->s.start;
            out_stream->next_pts           = buf->s.start;
            out_stream->min_frame_duration = buf->s.duration;
        }

        if (out_stream->type == SYNC_TYPE_AUDIO)
        {
            buf = FilterAudioFrame(out_stream, buf);
            if (buf == NULL)
            {
                // FilterAudioFrame can consume the buffer with no output
                continue;
            }
        }
        int64_t subtitle_next_pts = AV_NOPTS_VALUE;
        if (out_stream->type == SYNC_TYPE_SUBTITLE)
        {
            buf = sanitizeSubtitle(out_stream, buf);
            if (buf == NULL)
            {
                // sanitizeSubtitle can consume the buffer with no output
                continue;
            }
            // sanitizeSubtitle can return a list of subtitles.
            // Find the pts of the last subtitle in the list
            hb_buffer_t * sub = buf;
            while (sub != NULL)
            {
                subtitle_next_pts = sub->s.start;
                sub = sub->next;
            }
        }
        if (out_stream->type == SYNC_TYPE_AUDIO ||
            out_stream->type == SYNC_TYPE_VIDEO)
        {
            buf->s.start = out_stream->next_pts;
            buf->s.stop  = out_stream->next_pts + buf->s.duration;
            out_stream->next_pts += buf->s.duration;
        }
        else
        {
            out_stream->next_pts = subtitle_next_pts;
        }

        if (buf->s.stop > 0)
        {
            out_stream->current_duration = buf->s.stop -
                                           out_stream->first_pts;
        }
        out_stream->frame_count++;
        if (buf->s.duration > 0 &&
            out_stream->min_frame_duration > buf->s.duration)
        {
            out_stream->min_frame_duration = buf->s.duration;
        }
        if (out_stream->max_frame_duration < buf->s.duration)
        {
            out_stream->max_frame_duration = buf->s.duration;
        }
        if (out_stream->type == SYNC_TYPE_VIDEO &&
            buf->s.new_chap   > common->chapter)
        {
            common->chapter = buf->s.new_chap;
            log_chapter(common, buf->s.new_chap, out_stream->frame_count,
                        buf->s.start);
        }
        if ((buf->s.start < 0) ||
            (out_stream->type == SYNC_TYPE_VIDEO && buf->s.duration < 256))
        {
            // The pipeline can't handle negative timestamps
            // and it is sometimes not possible to avoid one
            // at the start of the video.  There can be a
            // significant delay before we see the first buffers
            // from all streams.  We can't buffer indefinitely
            // until we have seen the first PTS for all streams
            // so sometimes we may start before we have seen
            // the earliest PTS
            //
            // Also, encx264.c can't handle timestamps that are spaced
            // less than 256 ticks apart.
            hb_buffer_close(&buf);
        }
        fifo_push(out_stream->fifo_out, buf);
    } while (full);
}

static void Synchronize( sync_stream_t * stream )
{
    sync_common_t * common = stream->common;

    // Sync deposits output directly into fifos, so work_loop is not
    // blocking when output fifos become full.  Wait here before
    // performing any output when the output fifo for the input stream
    // is full
    if (stream->fifo_out != NULL)
    {
        while (!common->job->done && !*common->job->die)
        {
            if (hb_fifo_full_wait(stream->fifo_out))
            {
                break;
            }
        }
    }

    hb_lock(common->mutex);

    if (!fillQueues(common))
    {
        hb_unlock(common->mutex);
        return;
    }
    if (!common->found_first_pts)
    {
        checkFirstPts(common);
    }
    OutputBuffer(common);

    hb_unlock(common->mutex);
}

static void updateDuration( sync_stream_t * stream, int64_t start )
{
    // The video decoder does not set an initial duration for frames.
    // So set it here.
    if (stream->type == SYNC_TYPE_VIDEO)
    {
        int count = hb_list_count(stream->in_queue);
        if (count > 0)
        {
            hb_buffer_t * buf = hb_list_item(stream->in_queue, count - 1);
            double duration = start - buf->s.start;
            if (duration > 0)
            {
                buf->s.duration = duration;
                buf->s.stop = start;
            }
            else
            {
                buf->s.duration = 0.;
                buf->s.stop = buf->s.start;
            }
        }
    }
}

static void QueueBuffer( sync_stream_t * stream, hb_buffer_t * buf )
{
    hb_lock(stream->common->mutex);

    // We do not place a limit on the number of subtitle frames
    // that are buffered becuase there are cases where we will
    // receive all the subtitles for a file all at once (SSA subs).
    // If we did not buffer these subs here, the following deadlock
    // condition would occur:
    //   1. Subtitle decoder blocks trying to generate more subtitle
    //      lines than will fit in sync input buffers.
    //   2. This blocks the reader. Reader doesn't read any more
    //      audio or video, so sync won't receive buffers it needs
    //      to unblock subtitles.
    while (hb_list_count(stream->in_queue) > stream->max_len)
    {
        hb_cond_wait(stream->cond_full, stream->common->mutex);
    }

    buf->s.start -= stream->pts_slip;
    if (buf->s.stop != AV_NOPTS_VALUE)
    {
        buf->s.stop -= stream->pts_slip;
    }
    // Render offset is only useful for decoders, which are all
    // upstream of sync.  Squash it.
    buf->s.renderOffset = AV_NOPTS_VALUE;
    updateDuration(stream, buf->s.start);
    hb_list_add(stream->in_queue, buf);

    // Make adjustments for gaps found in other streams
    applyDeltas(stream->common);

    hb_unlock(stream->common->mutex);
}

static int InitAudio( sync_common_t * common, int index )
{
    hb_work_object_t  * w = NULL;
    hb_work_private_t * pv;
    hb_audio_t        * audio;

    audio = hb_list_item(common->job->list_audio, index);
    if (audio->priv.fifo_raw == NULL)
    {
        // No input fifo, not configured
        return 0;
    }
    pv = calloc(1, sizeof(hb_work_private_t));
    if (pv == NULL) goto fail;

    w = hb_get_work(common->job->h, WORK_SYNC_AUDIO);
    w->private_data = pv;
    w->audio        = audio;
    w->fifo_in      = audio->priv.fifo_raw;
    if (audio->config.out.codec & HB_ACODEC_PASS_FLAG)
    {
        w->fifo_out = audio->priv.fifo_out;
    }
    else
    {
        w->fifo_out = audio->priv.fifo_sync;
    }

    pv->common              = common;
    pv->stream              = &common->streams[1 + index];
    pv->stream->common      = common;
    pv->stream->cond_full   = hb_cond_init();
    if (pv->stream->cond_full == NULL) goto fail;
    pv->stream->in_queue    = hb_list_init();
    pv->stream->max_len     = SYNC_MAX_AUDIO_QUEUE_LEN;
    pv->stream->min_len     = SYNC_MIN_AUDIO_QUEUE_LEN;
    if (pv->stream->in_queue == NULL) goto fail;
    pv->stream->delta_list  = hb_list_init();
    if (pv->stream->delta_list == NULL) goto fail;
    pv->stream->type        = SYNC_TYPE_AUDIO;
    pv->stream->first_pts   = AV_NOPTS_VALUE;
    pv->stream->next_pts    = (int64_t)AV_NOPTS_VALUE;
    pv->stream->audio.audio = audio;
    pv->stream->fifo_out    = w->fifo_out;

    if (!(audio->config.out.codec & HB_ACODEC_PASS_FLAG) &&
        audio->config.in.samplerate != audio->config.out.samplerate)
    {
        /* Initialize libsamplerate */
        int error;
        pv->stream->audio.src.ctx = src_new(SRC_SINC_MEDIUM_QUALITY,
            hb_mixdown_get_discrete_channel_count(audio->config.out.mixdown),
            &error);
        if (pv->stream->audio.src.ctx == NULL) goto fail;
        pv->stream->audio.src.pkt.end_of_input = 0;
    }

    pv->stream->audio.gain_factor = pow(10, audio->config.out.gain / 20);

    hb_list_add(common->list_work, w);

    return 0;

fail:
    if (pv != NULL)
    {
        if (pv->stream != NULL)
        {
            if (pv->stream->audio.src.ctx)
            {
                src_delete(pv->stream->audio.src.ctx);
            }
            hb_list_close(&pv->stream->delta_list);
            hb_list_close(&pv->stream->in_queue);
            hb_cond_close(&pv->stream->cond_full);
        }
    }
    free(pv);
    free(w);

    return 1;
}

static int InitSubtitle( sync_common_t * common, int index )
{
    hb_work_object_t  * w = NULL;
    hb_work_private_t * pv;
    hb_subtitle_t     * subtitle;

    subtitle = hb_list_item(common->job->list_subtitle, index);
    if (subtitle->fifo_raw == NULL)
    {
        // No input fifo, not configured
        return 0;
    }
    pv = calloc(1, sizeof(hb_work_private_t));
    if (pv == NULL) goto fail;

    pv->common  = common;
    pv->stream  =
        &common->streams[1 + hb_list_count(common->job->list_audio) + index];
    pv->stream->common            = common;
    pv->stream->cond_full         = hb_cond_init();
    if (pv->stream->cond_full == NULL) goto fail;
    pv->stream->in_queue          = hb_list_init();
    pv->stream->max_len           = SYNC_MAX_SUBTITLE_QUEUE_LEN;
    pv->stream->min_len           = SYNC_MIN_SUBTITLE_QUEUE_LEN;
    if (pv->stream->in_queue == NULL) goto fail;
    pv->stream->delta_list        = hb_list_init();
    if (pv->stream->delta_list == NULL) goto fail;
    pv->stream->type              = SYNC_TYPE_SUBTITLE;
    pv->stream->first_pts         = AV_NOPTS_VALUE;
    pv->stream->next_pts          = (int64_t)AV_NOPTS_VALUE;
    pv->stream->subtitle.subtitle = subtitle;
    pv->stream->fifo_out          = subtitle->fifo_out;

    w = hb_get_work(common->job->h, WORK_SYNC_SUBTITLE);
    w->private_data = pv;
    w->subtitle     = subtitle;
    w->fifo_in      = subtitle->fifo_raw;
    w->fifo_out     = subtitle->fifo_out;

    memset(&pv->stream->subtitle.sanitizer, 0,
           sizeof(pv->stream->subtitle.sanitizer));
    if (subtitle->format == TEXTSUB && subtitle->config.dest == PASSTHRUSUB &&
        (common->job->mux & HB_MUX_MASK_MP4))
    {
        // Merge overlapping subtitles since mpv tx3g does not support them
        pv->stream->subtitle.sanitizer.merge = 1;
    }
    // PGS subtitles don't need to be linked because there are explicit
    // "clear" subtitle packets that indicate the end time of the
    // previous subtitle
    if (subtitle->config.dest == PASSTHRUSUB &&
        subtitle->source != PGSSUB)
    {
        // Fill in stop time when it is missing
        pv->stream->subtitle.sanitizer.link = 1;
    }
    hb_buffer_list_clear(&pv->stream->subtitle.sanitizer.list_current);

    hb_list_add(common->list_work, w);

    return 0;

fail:
    if (pv != NULL)
    {
        if (pv->stream != NULL)
        {
            hb_list_close(&pv->stream->delta_list);
            hb_list_close(&pv->stream->in_queue);
            hb_cond_close(&pv->stream->cond_full);
        }
    }
    free(pv);
    free(w);

    return 1;
}

/***********************************************************************
 * Initialize the work object
 **********************************************************************/
static int syncVideoInit( hb_work_object_t * w, hb_job_t * job)
{
    hb_work_private_t * pv;
    int                 ii;

    pv = calloc(1, sizeof(hb_work_private_t));
    if (pv == NULL) goto fail;
    w->private_data = pv;
    pv->common = calloc(1, sizeof(sync_common_t));
    if (pv->common == NULL) goto fail;
    pv->common->job = job;

    // count number of streams we need
    pv->common->stream_count = 1;
    pv->common->stream_count += hb_list_count(job->list_audio);
    pv->common->stream_count += hb_list_count(job->list_subtitle);
    pv->common->streams = calloc(pv->common->stream_count,
                                 sizeof(sync_stream_t));

    // Allocate streams
    if (pv->common->streams == NULL) goto fail;

    // create audio and subtitle work list
    pv->common->list_work = hb_list_init();
    if (pv->common->list_work == NULL) goto fail;

    // mutex to mediate access to pv->common
    pv->common->mutex = hb_lock_init();
    if (pv->common->mutex == NULL) goto fail;

    // Set up video sync work object
    pv->stream              = &pv->common->streams[0];
    pv->stream->common      = pv->common;
    pv->stream->cond_full   = hb_cond_init();
    if (pv->stream->cond_full == NULL) goto fail;
    pv->stream->in_queue    = hb_list_init();
    pv->stream->max_len     = SYNC_MAX_VIDEO_QUEUE_LEN;
    pv->stream->min_len     = SYNC_MIN_VIDEO_QUEUE_LEN;
    if (pv->stream->in_queue == NULL) goto fail;
    pv->stream->delta_list  = hb_list_init();
    if (pv->stream->delta_list == NULL) goto fail;
    pv->stream->type        = SYNC_TYPE_VIDEO;
    pv->stream->first_pts   = AV_NOPTS_VALUE;
    pv->stream->next_pts    = (int64_t)AV_NOPTS_VALUE;
    pv->stream->fifo_out    = job->fifo_sync;

    w->fifo_in            = job->fifo_raw;
    // sync performs direct output to fifos
    w->fifo_out           = job->fifo_sync;
    if (job->indepth_scan)
    {
        // When doing subtitle indepth scan, the pipeline ends at sync
        w->fifo_out = NULL;
    }

    if (job->pass_id == HB_PASS_ENCODE_2ND)
    {
        /* We already have an accurate frame count from pass 1 */
        hb_interjob_t * interjob = hb_interjob_get(job->h);
        pv->common->est_frame_count = interjob->frame_count;
    }
    else
    {
        /* Calculate how many video frames we are expecting */
        if (job->frame_to_stop)
        {
            pv->common->est_frame_count = job->frame_to_stop;
        }
        else
        {
            int64_t duration;
            if (job->pts_to_stop)
            {
                duration = job->pts_to_stop + 90000;
            }
            else
            {
                duration = 0;
                for (ii = job->chapter_start; ii <= job->chapter_end; ii++)
                {
                    hb_chapter_t * chapter;
                    chapter = hb_list_item(job->list_chapter, ii - 1);
                    if (chapter != NULL)
                    {
                        duration += chapter->duration;
                    }
                }
            }
            pv->common->est_frame_count = duration * job->title->vrate.num /
                                          job->title->vrate.den / 90000;
        }
    }
    hb_log("sync: expecting %d video frames", pv->common->est_frame_count);

    // Initialize audio sync work objects
    for (ii = 0; ii < hb_list_count(job->list_audio); ii++ )
    {
        if (InitAudio(pv->common, ii)) goto fail;
    }

    // Initialize subtitle sync work objects
    for (ii = 0; ii < hb_list_count(job->list_subtitle); ii++ )
    {
        if (InitSubtitle(pv->common, ii)) goto fail;
    }

    /* Launch work processing threads */
    for (ii = 0; ii < hb_list_count(pv->common->list_work); ii++)
    {
        hb_work_object_t * work;
        work         = hb_list_item(pv->common->list_work, ii);
        work->done   = w->done;
        work->thread = hb_thread_init(work->name, hb_work_loop,
                                      work, HB_LOW_PRIORITY);
    }

    if (job->frame_to_start || job->pts_to_start)
    {
        pv->common->start_found = 0;
    }
    else
    {
        pv->common->start_found = 1;
    }

    return 0;

fail:
    if (pv != NULL)
    {
        if (pv->common != NULL)
        {
            for (ii = 0; ii < hb_list_count(pv->common->list_work); ii++)
            {
                hb_work_object_t * work;
                work = hb_list_item(pv->common->list_work, ii);
                if (work->close) work->close(work);
            }
            hb_list_close(&pv->common->list_work);
            hb_lock_close(&pv->common->mutex);
            if (pv->stream != NULL)
            {
                hb_list_close(&pv->stream->delta_list);
                hb_list_close(&pv->stream->in_queue);
                hb_cond_close(&pv->stream->cond_full);
            }
            free(pv->common->streams);
            free(pv->common);
        }
    }
    free(pv);
    w->private_data = NULL;

    return 1;
}

/***********************************************************************
 * Close Video
 ***********************************************************************
 *
 **********************************************************************/
static void syncVideoClose( hb_work_object_t * w )
{
    hb_work_private_t * pv   = w->private_data;
    hb_job_t          * job;

    if (pv == NULL)
    {
        return;
    }
    job = pv->common->job;

    hb_log("sync: got %d frames, %d expected",
           pv->stream->frame_count, pv->common->est_frame_count );
    if (pv->stream->min_frame_duration > 0 &&
        pv->stream->max_frame_duration > 0 &&
        pv->stream->current_duration > 0)
    {
        hb_log("sync: framerate min %.3f fps, max %.3f fps, avg %.3f fps",
               90000. / pv->stream->max_frame_duration,
               90000. / pv->stream->min_frame_duration,
               (pv->stream->frame_count * 90000.) /
                pv->stream->current_duration);
    }

    /* save data for second pass */
    if( job->pass_id == HB_PASS_ENCODE_1ST )
    {
        /* Preserve frame count for better accuracy in pass 2 */
        hb_interjob_t * interjob = hb_interjob_get( job->h );
        interjob->frame_count = pv->stream->frame_count;
    }
    sync_delta_t * delta;
    while ((delta = hb_list_item(pv->stream->delta_list, 0)) != NULL)
    {
        hb_list_rem(pv->stream->delta_list, delta);
        free(delta);
    }
    hb_list_close(&pv->stream->delta_list);
    hb_list_empty(&pv->stream->in_queue);
    hb_cond_close(&pv->stream->cond_full);

    // Close work threads
    hb_work_object_t * work;
    while ((work = hb_list_item(pv->common->list_work, 0)))
    {
        hb_list_rem(pv->common->list_work, work);
        if (work->thread != NULL)
        {
            hb_thread_close(&work->thread);
        }
        if (work->close) work->close(work);
        free(work);
    }
    hb_list_close(&pv->common->list_work);

    hb_lock_close(&pv->common->mutex);
    free(pv->common->streams);
    free(pv->common);
    free(pv);
    w->private_data = NULL;
}

static hb_buffer_t * merge_ssa(hb_buffer_t *a, hb_buffer_t *b)
{
    int len, ii;
    char *text;
    hb_buffer_t *buf;

    if (a == NULL && b == NULL)
    {
        return NULL;
    }
    if (a == NULL)
    {
        return hb_buffer_dup(b);
    }
    if (b == NULL)
    {
        return hb_buffer_dup(a);
    }

    buf = hb_buffer_init(a->size + b->size);
    buf->s = a->s;

    // Find the text in the second SSA sub
    text = (char*)b->data;
    for (ii = 0; ii < 8; ii++)
    {
        text = strchr(text, ',');
        if (text == NULL)
            break;
        text++;
    }
    if (text != NULL)
    {
        len = sprintf((char*)buf->data, "%s\n%s", a->data, text);
        if (len >= 0)
            buf->size = len + 1;
    }
    else
    {
        memcpy(buf->data, a->data, a->size);
        buf->size = a->size;
    }

    return buf;
}

static hb_buffer_t * setSubDuration(sync_stream_t * stream, hb_buffer_t * sub)
{
    if (sub->s.flags & HB_BUF_FLAG_EOS)
    {
        return sub;
    }
    if (sub->s.stop != AV_NOPTS_VALUE)
    {
        sub->s.duration = sub->s.stop - sub->s.start;
        if (sub->s.duration <= 0)
        {
            hb_log("sync: subtitle 0x%x duration <= 0, PTS %"PRId64"",
                   stream->subtitle.subtitle->id, sub->s.start);
            hb_buffer_close(&sub);
        }
    }
    else if (stream->subtitle.sanitizer.link)
    {
        hb_log("sync: subtitle 0x%x duration not set, PTS %"PRId64"",
               stream->subtitle.subtitle->id, sub->s.start);
        sub->s.duration = (int64_t)AV_NOPTS_VALUE;
    }
    return sub;
}

// Create a list of buffers that overlap the given start time.
// Returns a list of buffers and the smallest stop time of those
// buffers.
static hb_buffer_t * findOverlap(subtitle_sanitizer_t *sanitizer,
                                 int64_t start, int64_t *stop_out)
{
    hb_buffer_list_t   list;
    hb_buffer_t      * buf;
    int64_t            stop;

    stop = INT64_MAX;
    hb_buffer_list_clear(&list);
    buf = hb_buffer_list_head(&sanitizer->list_current);
    while (buf != NULL)
    {
        if (buf->s.flags & HB_BUF_FLAG_EOF)
        {
            break;
        }
        if (buf->s.start > start)
        {
            if (stop > buf->s.start)
            {
                *stop_out = buf->s.start;
            }
            break;
        }
        if (buf->s.start <= start && start < buf->s.stop)
        {
            hb_buffer_t * tmp = hb_buffer_dup(buf);
            tmp->s.start = start;
            hb_buffer_list_append(&list, tmp);
            if (stop > buf->s.stop)
            {
                stop = buf->s.stop;
                *stop_out = stop;
            }
        }
        buf = buf->next;
    }

    return hb_buffer_list_clear(&list);
}

// Find all subtitles in the list that start "now" and overlap for
// some period of time.  Create a new subtitle buffer that is the
// merged results of the overlapping parts and update start times
// of non-overlapping parts.
static int mergeSubtitleOverlaps(subtitle_sanitizer_t *sanitizer)
{
    hb_buffer_t  * merged_buf   = NULL;
    hb_buffer_t  * a, * b;

    a = hb_buffer_list_head(&sanitizer->list_current);
    if (a != NULL && (a->s.flags & HB_BUF_FLAG_EOF))
    {
        // EOF
        return 0;
    }
    if (a == NULL ||
        a->s.start == AV_NOPTS_VALUE || a->s.stop == AV_NOPTS_VALUE)
    {
        // Not enough information to resolve an overlap
        return -1;
    }
    b = a->next;
    if (b != NULL && a->s.stop <= b->s.start)
    {
        // No overlap
        return 0;
    }

    // Check that we have 2 non-overlapping buffers in the list
    // and that all timestamps are valid up to the non-overlap.
    // This ensures that multiple overlapping subtitles have been
    // completely merged.
    while (b != NULL &&
           b->s.start < a->s.stop && !(b->s.flags & HB_BUF_FLAG_EOF))
    {
        if (b->s.start == AV_NOPTS_VALUE || b->s.stop == AV_NOPTS_VALUE)
        {
            // Not enough information to resolve an overlap
            return -1;
        }
        b = b->next;
    }
    if (b == NULL)
    {
        // Not enough information to resolve an overlap
        return -1;
    }

    hb_buffer_list_t   merged_list;
    int64_t            start, stop, last;

    if (b->s.flags & HB_BUF_FLAG_EOF)
    {
        last = INT64_MAX;
    }
    else
    {
        last = b->s.start;
    }

    hb_buffer_list_clear(&merged_list);
    a = hb_buffer_list_head(&sanitizer->list_current);
    start = a->s.start;
    while (start < last)
    {
        hb_buffer_t * merge = findOverlap(sanitizer, start, &stop);
        if (merge == NULL)
        {
            break;
        }
        a = merge;
        merged_buf = NULL;
        while (a != NULL)
        {
            hb_buffer_t * tmp;

            tmp = merge_ssa(merged_buf, a);
            hb_buffer_close(&merged_buf);
            merged_buf = tmp;

            a = a->next;
        }
        merged_buf->s.stop = stop;
        hb_buffer_close(&merge);
        hb_buffer_list_append(&merged_list, merged_buf);
        start = stop;

        // Remove merged buffers
        a = hb_buffer_list_head(&sanitizer->list_current);
        while (a != NULL && a->s.start < stop &&
               !(a->s.flags & HB_BUF_FLAG_EOF))
        {
            hb_buffer_t * next = a->next;
            if (a->s.stop <= stop)
            {
                // Buffer consumed
                hb_buffer_list_rem(&sanitizer->list_current, a);
                hb_buffer_close(&a);
            }
            else
            {
                a->s.start = stop;
            }
            a = next;
        }
    }
    merged_buf = hb_buffer_list_clear(&merged_list);
    hb_buffer_list_prepend(&sanitizer->list_current, merged_buf);

    return 0;
}

static hb_buffer_t * mergeSubtitles(sync_stream_t * stream)
{
    hb_buffer_t          * buf;
    hb_buffer_list_t       list;
    subtitle_sanitizer_t * sanitizer = &stream->subtitle.sanitizer;

    hb_buffer_list_clear(&list);

    if (!sanitizer->merge)
    {
        int limit = sanitizer->link ? 1 : 0;

        // Handle all but the last buffer
        // The last buffer may not have been "linked" yet
        while (hb_buffer_list_count(&sanitizer->list_current) > limit)
        {
            buf = hb_buffer_list_rem_head(&sanitizer->list_current);
            if (!(buf->s.flags & HB_BUF_FLAG_EOF))
            {
                buf = setSubDuration(stream, buf);
                hb_buffer_list_append(&list, buf);
            }
        }
        return hb_buffer_list_clear(&list);
    }

    // We only reach here if we are merging subtitles
    while (hb_buffer_list_count(&sanitizer->list_current) > 0)
    {
        buf = hb_buffer_list_head(&sanitizer->list_current);
        if (buf->s.flags & HB_BUF_FLAG_EOF)
        {
            // remove EOF from list, add to output
            buf = hb_buffer_list_rem_head(&sanitizer->list_current);
            hb_buffer_list_append(&list, buf);
            break;
        }

        int result = mergeSubtitleOverlaps(sanitizer);
        if (result < 0)
        {
            // not enough information available yet to resolve the overlap
            break;
        }

        // Overlap resolved, output a buffer
        buf = hb_buffer_list_rem_head(&sanitizer->list_current);
        if (buf != NULL && !(buf->s.flags & HB_BUF_FLAG_EOF))
        {
            buf = setSubDuration(stream, buf);
            hb_buffer_list_append(&list, buf);
        }
    }

    return hb_buffer_list_clear(&list);
}

static hb_buffer_t * sanitizeSubtitle(
    sync_stream_t        * stream,
    hb_buffer_t          * sub)
{
    hb_buffer_list_t       list;
    subtitle_sanitizer_t * sanitizer = &stream->subtitle.sanitizer;

    if (sub == NULL)
    {
        return NULL;
    }

    hb_buffer_list_set(&list, sub);
    if (!sanitizer->link && !sanitizer->merge)
    {
        hb_buffer_list_t out_list;
        hb_buffer_list_clear(&out_list);

        sub = hb_buffer_list_rem_head(&list);
        while (sub != NULL && !(sub->s.flags & HB_BUF_FLAG_EOF))
        {
            sub = setSubDuration(stream, sub);
            hb_buffer_list_append(&out_list, sub);
            sub = hb_buffer_list_rem_head(&list);
        }
        return hb_buffer_list_clear(&out_list);
    }

    sub = hb_buffer_list_rem_head(&list);
    while (sub != NULL)
    {
        if (sub->s.flags & HB_BUF_FLAG_EOF)
        {
            hb_buffer_list_append(&sanitizer->list_current, sub);
            break;
        }

        hb_buffer_t *last = hb_buffer_list_tail(&sanitizer->list_current);
        if (last != NULL && last->s.stop == AV_NOPTS_VALUE)
        {
            last->s.stop = sub->s.start;
            if (last->s.stop <= last->s.start)
            {
                // Subtitle duration <= 0.  Drop it.
                hb_log("sync: subtitle 0x%x has no duration, PTS %"PRId64"",
                       stream->subtitle.subtitle->id, sub->s.start);
                hb_buffer_list_rem_tail(&sanitizer->list_current);
                hb_buffer_close(&last);
            }
        }

        if (sub->s.flags & HB_BUF_FLAG_EOS)
        {
            // Used to indicate "clear" subtitles when the duration
            // of subtitles is not encoded in the stream
            hb_buffer_close(&sub);
        }
        hb_buffer_list_append(&sanitizer->list_current, sub);
        sub = hb_buffer_list_rem_head(&list);
    }

    return mergeSubtitles(stream);
}

/***********************************************************************
 * syncVideoWork
 ***********************************************************************
 *
 **********************************************************************/
static int syncVideoWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                          hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;

    if (pv->common->done)
    {
        return HB_WORK_DONE;
    }
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        streamFlush(pv->stream);
        // Ideally, we would only do this subtitle scan check in
        // syncSubtitleWork, but someone might try to do a subtitle
        // scan on a source that has no subtitles :-(
        if (pv->common->job->indepth_scan)
        {
            // When doing subtitle indepth scan, the pipeline ends at sync.
            // Terminate job when EOF reached.
            *w->done = 1;
        }
        return HB_WORK_DONE;
    }

    *buf_in = NULL;
    QueueBuffer(pv->stream, in);
    Synchronize(pv->stream);

    return HB_WORK_OK;
}

hb_work_object_t hb_sync_video =
{
    WORK_SYNC_VIDEO,
    "Video Synchronization",
    syncVideoInit,
    syncVideoWork,
    syncVideoClose
};

/***********************************************************************
 * Close Audio
 ***********************************************************************
 *
 **********************************************************************/
static void syncAudioClose( hb_work_object_t * w )
{
    hb_work_private_t * pv   = w->private_data;

    if (pv == NULL)
    {
        return;
    }

    // Free samplerate conversion context
    if (pv->stream->audio.src.ctx)
    {
        src_delete(pv->stream->audio.src.ctx);
    }

    sync_delta_t * delta;
    while ((delta = hb_list_item(pv->stream->delta_list, 0)) != NULL)
    {
        hb_list_rem(pv->stream->delta_list, delta);
        free(delta);
    }
    hb_list_close(&pv->stream->delta_list);
    hb_list_empty(&pv->stream->in_queue);
    hb_cond_close(&pv->stream->cond_full);
    free(pv);
    w->private_data = NULL;
}

static int syncAudioInit( hb_work_object_t * w, hb_job_t * job)
{
    return 0;
}

/***********************************************************************
 * SyncAudio
 ***********************************************************************
 *
 **********************************************************************/
static int syncAudioWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                          hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;

    if (pv->common->done)
    {
        return HB_WORK_DONE;
    }
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        streamFlush(pv->stream);
        return HB_WORK_DONE;
    }

    *buf_in = NULL;
    QueueBuffer(pv->stream, in);
    Synchronize(pv->stream);

    return HB_WORK_OK;
}

hb_work_object_t hb_sync_audio =
{
    WORK_SYNC_AUDIO,
    "Audio Synchronization",
    syncAudioInit,
    syncAudioWork,
    syncAudioClose
};

// FilterAudioFrame is called after audio timestamp discontinuities
// have all been corrected.  So we expect smooth continuous audio
// here.
static hb_buffer_t * FilterAudioFrame( sync_stream_t * stream,
                                       hb_buffer_t *buf )
{
    hb_audio_t * audio = stream->audio.audio;

    // Can't count of buf->s.stop - buf->s.start for accurate duration
    // due to integer rounding, so use buf->s.duration when it is set
    // (which should be always if I didn't miss anything)
    if (buf->s.duration <= 0)
    {
        buf->s.duration = buf->s.stop - buf->s.start;
    }

    if (!(audio->config.out.codec & HB_ACODEC_PASS_FLAG))
    {
        // TODO: this should all be replaced by an audio filter chain.

        // Audio is not passthru.  Check if we need to modify the audio
        // in any way.
        if (stream->audio.src.ctx != NULL)
        {
            /* do sample rate conversion */
            int count_in, count_out;
            hb_buffer_t * buf_raw = buf;
            int sample_size = hb_mixdown_get_discrete_channel_count(
                                audio->config.out.mixdown ) * sizeof( float );

            count_in  = buf_raw->size / sample_size;
            /*
             * When using stupid rates like 44.1 there will always be some
             * truncation error. E.g., a 1536 sample AC3 frame will turn into a
             * 1536*44.1/48.0 = 1411.2 sample frame. If we just truncate the .2
             * the error will build up over time and eventually the audio will
             * substantially lag the video. libsamplerate will keep track of the
             * fractional sample & give it to us when appropriate if we give it
             * an extra sample of space in the output buffer.
             */
            count_out = (buf->s.duration * audio->config.out.samplerate) /
                        90000 + 1;

            stream->audio.src.pkt.input_frames  = count_in;
            stream->audio.src.pkt.output_frames = count_out;
            stream->audio.src.pkt.src_ratio =
                (double)audio->config.out.samplerate /
                        audio->config.in.samplerate;

            buf = hb_buffer_init( count_out * sample_size );
            buf->s = buf_raw->s;
            stream->audio.src.pkt.data_in  = (float *) buf_raw->data;
            stream->audio.src.pkt.data_out = (float *) buf->data;
            if (src_process(stream->audio.src.ctx, &stream->audio.src.pkt))
            {
                /* XXX If this happens, we're screwed */
                hb_error("sync: audio 0x%x src_process failed", audio->id);
            }
            hb_buffer_close(&buf_raw);

            if (stream->audio.src.pkt.output_frames_gen <= 0)
            {
                hb_buffer_close(&buf);
                return NULL;
            }
            buf->s.duration = 90000. * stream->audio.src.pkt.output_frames_gen /
                              audio->config.out.samplerate;
        }
        if (audio->config.out.gain > 0.0)
        {
            int count, ii;

            count  = buf->size / sizeof(float);
            for ( ii = 0; ii < count; ii++ )
            {
                double sample;

                sample = (double)*(((float*)buf->data)+ii);
                sample *= stream->audio.gain_factor;
                if (sample > 0)
                    sample = MIN(sample, 1.0);
                else
                    sample = MAX(sample, -1.0);
                *(((float*)buf->data)+ii) = sample;
            }
        }
        else if( audio->config.out.gain < 0.0 )
        {
            int count, ii;

            count  = buf->size / sizeof(float);
            for ( ii = 0; ii < count; ii++ )
            {
                double sample;

                sample = (double)*(((float*)buf->data)+ii);
                sample *= stream->audio.gain_factor;
                *(((float*)buf->data)+ii) = sample;
            }
        }
    }

    buf->s.type = AUDIO_BUF;
    buf->s.frametype = HB_FRAME_AUDIO;

    return buf;
}

static void UpdateState( sync_common_t * common, int frame_count )
{
    hb_job_t          * job = common->job;
    hb_state_t state;

    hb_get_state2(job->h, &state);
    if (frame_count == 0)
    {
        common->st_first = hb_get_date();
        job->st_pause_date = -1;
        job->st_paused = 0;
    }

    if (job->indepth_scan)
    {
        // Progress for indept scan is handled by reader
        // frame_count is used during indepth_scan
        // to find start & end points.
        return;
    }

    if (hb_get_date() > common->st_dates[3] + 1000)
    {
        memmove( &common->st_dates[0], &common->st_dates[1],
                 3 * sizeof( uint64_t ) );
        memmove( &common->st_counts[0], &common->st_counts[1],
                 3 * sizeof( uint64_t ) );
        common->st_dates[3]  = hb_get_date();
        common->st_counts[3] = frame_count;
    }

#define p state.param.working
    state.state = HB_STATE_WORKING;
    p.progress  = (float)frame_count / common->est_frame_count;
    if (p.progress > 1.0)
    {
        p.progress = 1.0;
    }
    p.rate_cur   = 1000.0 * (common->st_counts[3] - common->st_counts[0]) /
                            (common->st_dates[3]  - common->st_dates[0]);
    if (hb_get_date() > common->st_first + 4000)
    {
        int eta;
        p.rate_avg = 1000.0 * common->st_counts[3] /
                     (common->st_dates[3] - common->st_first - job->st_paused);
        eta = (common->est_frame_count - common->st_counts[3]) / p.rate_avg;
        p.hours   = eta / 3600;
        p.minutes = (eta % 3600) / 60;
        p.seconds = eta % 60;
    }
    else
    {
        p.rate_avg = 0.0;
        p.hours    = -1;
        p.minutes  = -1;
        p.seconds  = -1;
    }
#undef p

    hb_set_state(job->h, &state);
}

static void UpdateSearchState( sync_common_t * common, int64_t start,
                               int frame_count )
{
    hb_job_t   * job = common->job;
    hb_state_t   state;
    uint64_t     now;
    double       avg;

    now = hb_get_date();
    if (frame_count == 0)
    {
        common->st_first = now;
        job->st_pause_date = -1;
        job->st_paused = 0;
    }

    if (job->indepth_scan)
    {
        // Progress for indept scan is handled by reader
        // frame_count is used during indepth_scan
        // to find start & end points.
        return;
    }

    hb_get_state2(job->h, &state);

#define p state.param.working
    state.state = HB_STATE_SEARCHING;
    if (job->frame_to_start)
        p.progress  = (float)frame_count / job->frame_to_start;
    else if (job->pts_to_start)
        p.progress  = (float) start / job->pts_to_start;
    else
        p.progress = 0;
    if (p.progress > 1.0)
    {
        p.progress = 1.0;
    }
    if (now > common->st_first)
    {
        int eta = 0;

        if (job->frame_to_start)
        {
            avg = 1000.0 * frame_count / (now - common->st_first);
            eta = (job->frame_to_start - frame_count ) / avg;
        }
        else if (job->pts_to_start)
        {
            avg = 1000.0 * start / (now - common->st_first);
            eta = (job->pts_to_start - start) / avg;
        }
        p.hours   = eta / 3600;
        p.minutes = (eta % 3600) / 60;
        p.seconds = eta % 60;
    }
    else
    {
        p.rate_avg = 0.0;
        p.hours    = -1;
        p.minutes  = -1;
        p.seconds  = -1;
    }
#undef p

    hb_set_state(job->h, &state);
}

static int syncSubtitleInit( hb_work_object_t * w, hb_job_t * job )
{
    return 0;
}

static void syncSubtitleClose( hb_work_object_t * w )
{
    hb_work_private_t * pv   = w->private_data;

    if (pv == NULL)
    {
        return;
    }

    sync_delta_t * delta;
    while ((delta = hb_list_item(pv->stream->delta_list, 0)) != NULL)
    {
        hb_list_rem(pv->stream->delta_list, delta);
        free(delta);
    }
    hb_list_close(&pv->stream->delta_list);
    hb_list_empty(&pv->stream->in_queue);
    hb_cond_close(&pv->stream->cond_full);
    hb_buffer_list_close(&pv->stream->subtitle.sanitizer.list_current);
    free(pv);
    w->private_data = NULL;
}

static int syncSubtitleWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                             hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;

    if (pv->common->done)
    {
        return HB_WORK_DONE;
    }
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        // sanitizeSubtitle requires EOF buffer to recognize that
        // it needs to flush all subtitles.
        hb_list_add(pv->stream->in_queue, hb_buffer_eof_init());
        streamFlush(pv->stream);
        if (pv->common->job->indepth_scan)
        {
            // When doing subtitle indepth scan, the pipeline ends at sync.
            // Terminate job when EOF reached.
            *w->done = 1;
        }
        return HB_WORK_DONE;
    }

    *buf_in = NULL;
    QueueBuffer(pv->stream, in);
    Synchronize(pv->stream);

    return HB_WORK_OK;
}

hb_work_object_t hb_sync_subtitle =
{
    WORK_SYNC_SUBTITLE,
    "Subitle Synchronization",
    syncSubtitleInit,
    syncSubtitleWork,
    syncSubtitleClose
};
