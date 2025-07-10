/* sync.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include <stdio.h>
#include "handbrake/audio_resample.h"
#include "handbrake/hwaccel.h"

#if HB_PROJECT_FEATURE_QSV
#include "handbrake/qsv_common.h"
#endif

#define SYNC_MAX_VIDEO_QUEUE_LEN    40
#define SYNC_MIN_VIDEO_QUEUE_LEN    20

// Audio is small, buffer a lot.  It helps to ensure that we see
// the initial PTS from all input streams before setting the 'zero' point.
#define SYNC_MAX_AUDIO_QUEUE_LEN    200
#define SYNC_MIN_AUDIO_QUEUE_LEN    30

// We do not place a limit on the number of subtitle frames
// that are buffered (max_len == INT_MAX) because there are
// cases where we will receive all the subtitles for a file
// all at once (SSA subs).
//
// If we did not buffer these subs here, the following deadlock
// condition would occur:
//   1. Subtitle decoder blocks trying to generate more subtitle
//      lines than will fit in sync input buffers.
//   2. This blocks the reader. Reader doesn't read any more
//      audio or video, so sync won't receive buffers it needs
//      to unblock subtitles.
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

#define SCR_HASH_SZ   (2 << 3)
#define SCR_HASH_MASK (SCR_HASH_SZ - 1)

typedef struct
{
    int                 scr_sequence;
    int64_t             scr_offset;
} scr_t;

typedef struct
{
    sync_common_t     * common;

    // Stream I/O control
    int                 done;
    int                 flush;
    hb_list_t         * in_queue;
    hb_list_t         * scr_delay_queue;
    int                 max_len;
    int                 min_len;
    hb_fifo_t         * fifo_in;
    hb_fifo_t         * fifo_out;

    // PTS synchronization
    hb_list_t         * delta_list;
    int64_t             pts_slip;
    double              next_pts;
    double              last_pts;

    // SCR recovery
    int                 last_scr_sequence;
    double              last_scr_pts;
    double              last_duration;

    // frame statistics
    int64_t             first_pts;
    double              min_frame_duration;
    double              max_frame_duration;
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
            int     id;
            int     cadence[12];
            int     new_chap;
        } video;

        // Audio stream context
        struct
        {
            hb_audio_t          * audio;

            // Audio filter settings
            // Samplerate conversion
            hb_audio_resample_t * resample;
            double                gain_factor;
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
    // Audio/Video sync thread synchronization
    hb_job_t      * job;
    hb_lock_t     * mutex;
    int             stream_count;
    sync_stream_t * streams;
    int             found_first_pts;
    int             flush;

    // SCR adjustments
    scr_t           scr[SCR_HASH_SZ];
    int             first_scr;

    // point-to-point support
    int             start_found;
    int64_t         pts_to_start;
    int64_t         start_pts;
    int64_t         stop_pts;
    int             wait_for_frame;
    int             wait_for_pts;

    // sync audio work objects
    hb_list_t     * list_work;

    // UpdateState Statistics
    int             est_frame_count;
    uint64_t        st_counts[4];
    uint64_t        st_dates[4];
    uint64_t        st_first;

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
static int  UpdateSCR( sync_stream_t * stream, hb_buffer_t * buf );
static hb_buffer_t * FilterAudioFrame( sync_stream_t * stream,
                                       hb_buffer_t *buf );
static void SortedQueueBuffer( sync_stream_t * stream, hb_buffer_t * buf );
static hb_buffer_t * sanitizeSubtitle(sync_stream_t        * stream,
                                      hb_buffer_t          * sub);
static int OutputBuffer( sync_common_t * common );

static void saveChap( sync_stream_t * stream, hb_buffer_t * buf )
{
    if (stream->type != SYNC_TYPE_VIDEO || buf == NULL)
    {
        return;
    }
    if (buf->s.new_chap > 0)
    {
        stream->video.new_chap = buf->s.new_chap;
    }
}

static void restoreChap( sync_stream_t * stream, hb_buffer_t * buf )
{
    if (stream->type != SYNC_TYPE_VIDEO || buf == NULL)
    {
        return;
    }
    if (stream->video.new_chap > 0 && buf->s.new_chap <= 0)
    {
        buf->s.new_chap = stream->video.new_chap;
        stream->video.new_chap = 0;
    }
}

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
        if (hb_list_count(stream->in_queue) <= stream->min_len)
        {
            wait = 1;
        }
    }
    return !wait || abort;
}

static void scrSlip( sync_common_t * common, int64_t delta )
{
    int ii;
    for (ii = 0; ii < SCR_HASH_SZ; ii++)
    {
        common->scr[ii].scr_offset += delta;
    }
}

static void shiftTS( sync_common_t * common, int64_t delta )
{
    int ii, jj;

    scrSlip(common, delta);
    for (ii = 0; ii < common->stream_count; ii++)
    {
        hb_buffer_t   * buf = NULL;
        sync_stream_t * stream = &common->streams[ii];
        int             count = hb_list_count(stream->in_queue);

        for (jj = 0; jj < count; jj++)
        {
            buf = hb_list_item(stream->in_queue, jj);
            if (buf->s.start != AV_NOPTS_VALUE)
            {
                buf->s.start -= delta;
            }
            if (buf->s.stop != AV_NOPTS_VALUE)
            {
                buf->s.stop  -= delta;
            }
        }
        if (buf != NULL && buf->s.start != AV_NOPTS_VALUE)
        {
            stream->last_scr_pts = buf->s.start + buf->s.duration;
        }
        else
        {
            stream->last_scr_pts = (int64_t)AV_NOPTS_VALUE;
        }
    }
}

static hb_buffer_t * CreateSilenceBuf( sync_stream_t * stream,
                                       int64_t dur, int64_t pts )
{
    double             frame_dur, next_pts, duration;
    int                size;
    hb_buffer_list_t   list;
    hb_buffer_t      * buf;

    if (stream->audio.audio->config.out.codec & HB_ACODEC_PASS_FLAG)
    {
        return NULL;
    }
    duration = dur;
    // Although frame size isn't technically important any more, we
    // keep audio buffer durations <= input audio buffer durations.
    frame_dur = (90000. * stream->audio.audio->config.in.samples_per_frame) /
                          stream->audio.audio->config.in.samplerate;
    // Audio mixdown occurs in decoders before sync.
    // So number of channels here is output channel count.
    // But audio samplerate conversion happens in later here in sync.c
    // FilterAudioFrame, so samples_per_frame is still the input sample count.
    size = sizeof(float) * stream->audio.audio->config.in.samples_per_frame *
                           hb_mixdown_get_discrete_channel_count(
                                    stream->audio.audio->config.out.mixdown);

    hb_buffer_list_clear(&list);
    next_pts = pts;
    while (duration >= frame_dur)
    {
        buf = hb_buffer_init(size);
        memset(buf->data, 0, buf->size);
        buf->s.start     = next_pts;
        next_pts        += frame_dur;
        buf->s.stop      = next_pts;
        buf->s.duration  = frame_dur;
        duration        -= frame_dur;
        hb_buffer_list_append(&list, buf);
    }
    if (duration > 0)
    {
        // Make certain size is even multiple of sample size * num channels
        size = (int)(duration * stream->audio.audio->config.in.samplerate /
                     90000) * sizeof(float) *
               hb_mixdown_get_discrete_channel_count(
                                    stream->audio.audio->config.out.mixdown);
        if (size > 0)
        {
            buf = hb_buffer_init(size);
            memset(buf->data, 0, buf->size);
            buf->s.start     = next_pts;
            next_pts        += duration;
            buf->s.stop      = next_pts;
            buf->s.duration  = duration;
            hb_buffer_list_append(&list, buf);
        }
    }
    return hb_buffer_list_clear(&list);
}

static hb_buffer_t * CreateBlackBuf( sync_stream_t * stream,
                                     int64_t dur, int64_t pts )
{
    // I would like to just allocate one black frame here and give
    // it the full duration.  But encoders that use B-Frames compute
    // the dts delay based on the pts of the 2nd or 3rd frame which
    // can be a very large value in this case.  This large dts delay
    // causes problems with computing the dts of the frames (which is
    // extrapolated from the pts and the computed dts delay). And the
    // dts problems lead to problems with frame duration.
    double             frame_dur, next_pts, duration;
    hb_buffer_list_t   list;
    hb_buffer_t      * buf = NULL;

    hb_buffer_list_clear(&list);
    duration = dur;
    next_pts = pts;

    frame_dur = 90000. * stream->common->job->title->vrate.den /
                         stream->common->job->title->vrate.num;

    // Only create black buffers of frame_dur or longer
    while (duration >= frame_dur)
    {
        if (buf == NULL)
        {
            buf = hb_frame_buffer_init(stream->common->job->input_pix_fmt,
                                   stream->common->job->title->geometry.width,
                                   stream->common->job->title->geometry.height);
            uint8_t *planes[4];
            ptrdiff_t linesizes[4];
            for (int i = 0; i <= buf->f.max_plane; ++i)
            {
                planes[i] = buf->plane[i].data;
                linesizes[i] = buf->plane[i].stride;
            }
            av_image_fill_black(planes, linesizes, stream->common->job->input_pix_fmt,
                                stream->common->job->color_range, buf->f.width, buf->f.height);
            buf->f.color_prim = stream->common->job->title->color_prim;
            buf->f.color_transfer = stream->common->job->title->color_transfer;
            buf->f.color_matrix = stream->common->job->title->color_matrix;
            buf->f.color_range = stream->common->job->color_range;
            buf->f.chroma_location = stream->common->job->chroma_location;

            // Dolby Vision requires a RPU on every buffer, attach the first
            // found during scan in the absence of something better
            if (stream->common->job->title->initial_rpu)
            {
                hb_data_t *rpu = stream->common->job->title->initial_rpu;
                AVBufferRef *ref = av_buffer_alloc(rpu->size);
                memcpy(ref->data, rpu->bytes, rpu->size);

                AVFrameSideData *sd_dst = NULL;
                sd_dst = hb_buffer_new_side_data_from_buf(buf, stream->common->job->title->initial_rpu_type, ref);

                if (!sd_dst)
                {
                    av_buffer_unref(&ref);
                }
            }

            if (stream->common->job->hw_pix_fmt != AV_PIX_FMT_NONE)
            {
                buf = stream->common->job->hw_accel->upload(stream->common->job, &buf);
            }
        }
        else
        {
            {
                buf = hb_buffer_dup(buf);
            }
        }
        buf->s.start     = next_pts;
        next_pts        += frame_dur;
        buf->s.stop      = next_pts;
        buf->s.duration  = frame_dur;
        duration        -= frame_dur;
        hb_buffer_list_append(&list, buf);
    }

    if (buf != NULL)
    {
        if (buf->s.stop < pts + dur)
        {
            // Extend the duration of the last black buffer to fill
            // the remaining gap.
            buf->s.duration += pts + dur - buf->s.stop;
            buf->s.stop = pts + dur;
        }
    }

    return hb_buffer_list_clear(&list);
}

static void setNextPts( sync_common_t * common )
{
    int ii;

    for (ii = 0; ii < common->stream_count; ii++)
    {
        sync_stream_t * stream = &common->streams[ii];
        hb_buffer_t   * buf = hb_list_item(stream->in_queue, 0);
        if (buf != NULL)
        {
            stream->next_pts = buf->s.start;
        }
        else
        {
            stream->next_pts = (int64_t)AV_NOPTS_VALUE;
        }
    }
}

static void alignStream( sync_common_t * common, sync_stream_t * stream,
                         int64_t pts )
{
    if (hb_list_count(stream->in_queue) <= 0 ||
        stream->type == SYNC_TYPE_SUBTITLE)
    {
        return;
    }

    hb_buffer_t * buf = hb_list_item(stream->in_queue, 0);
    int64_t gap = buf->s.start - pts;

    if (gap == 0)
    {
        return;
    }
    if (gap < 0)
    {
        int ii;

        // Drop frames from other streams
        for (ii = 0; ii < common->stream_count; ii++)
        {
            sync_stream_t * other_stream = &common->streams[ii];
            if (stream == other_stream)
            {
                continue;
            }
            while (hb_list_count(other_stream->in_queue) > 0)
            {
                buf = hb_list_item(other_stream->in_queue, 0);
                if (buf->s.start < pts)
                {
                    if (other_stream->type == SYNC_TYPE_SUBTITLE &&
                        buf->s.stop > pts)
                    {
                        // Subtitle ends after start time, keep sub and
                        // adjust it's start time
                        buf->s.start = pts;
                        break;
                    }
                    else
                    {
                        hb_list_rem(other_stream->in_queue, buf);
                        hb_buffer_close(&buf);
                    }
                }
                else
                {
                    // Fill the partial frame gap left after dropping frames
                    alignStream(common, other_stream, pts);
                    break;
                }
            }
        }
    }
    else
    {
        hb_buffer_t * blank_buf = NULL;

        // Insert a blank frame to fill the gap
        if (stream->type == SYNC_TYPE_AUDIO)
        {
            // Can't add silence padding to passthru streams
            if (!(stream->audio.audio->config.out.codec & HB_ACODEC_PASS_FLAG))
            {
                blank_buf = CreateSilenceBuf(stream, gap, pts);
            }
        }
        else if (stream->type == SYNC_TYPE_VIDEO)
        {
            blank_buf = CreateBlackBuf(stream, gap, pts);
        }

        int64_t last_stop = pts;
        hb_buffer_t * next;
        int           pos;
        for (pos = 0; blank_buf != NULL; blank_buf = next, pos++)
        {
            last_stop = blank_buf->s.stop;
            next = blank_buf->next;
            blank_buf->next = NULL;
            hb_list_insert(stream->in_queue, pos, blank_buf);
        }
        if (stream->type == SYNC_TYPE_VIDEO && last_stop < buf->s.start)
        {
            // Extend the duration of the first frame to fill the remaining gap.
            buf->s.duration += buf->s.start - last_stop;
            buf->s.start = last_stop;
        }
    }
}

static void alignStreams( sync_common_t * common, int64_t pts )
{
    int           ii;
    hb_buffer_t * buf;

    if (common->job->align_av_start)
    {
        int64_t first_pts = AV_NOPTS_VALUE;
        int     audio_passthru = 0;

        for (ii = 0; ii < common->stream_count; ii++)
        {
            sync_stream_t * stream = &common->streams[ii];

            buf = hb_list_item(stream->in_queue, 0);

            // P-to-P encoding will pass the start point in pts.
            // Drop any buffers that are before the start point.
            while (buf != NULL && buf->s.start < pts)
            {
                hb_list_rem(stream->in_queue, buf);
                hb_buffer_close(&buf);
                buf = hb_list_item(stream->in_queue, 0);
            }
            if (buf == NULL)
            {
                continue;
            }
            if (stream->type == SYNC_TYPE_AUDIO &&
                stream->audio.audio->config.out.codec & HB_ACODEC_PASS_FLAG)
            {
                // Find the largest initial pts of all passthru audio streams.
                // We can not add silence to passthru audio streams.
                // To align with a passthru audio stream, we must drop
                // buffers from all other streams that are before
                // the first buffer in the passthru audio stream.
                audio_passthru = 1;
                if (first_pts < buf->s.start)
                {
                    first_pts = buf->s.start;
                }
            }
            else if (!audio_passthru)
            {
                // Find the smallest initial pts of all streams when
                // there is *no* passthru audio.
                // When there is no passthru audio stream, we insert
                // silence or black buffers to fill any gaps between
                // the start of any stream and the start of the stream
                // with the smallest pts.
                if (first_pts == AV_NOPTS_VALUE || first_pts > buf->s.start)
                {
                    first_pts = buf->s.start;
                }
            }
        }
        if (first_pts != AV_NOPTS_VALUE)
        {
            for (ii = 0; ii < common->stream_count; ii++)
            {
                // Fill the gap (or drop frames for passthru audio)
                alignStream(common, &common->streams[ii], first_pts);
            }
        }
    }
}

static void computeInitialTS( sync_common_t * common,
                              sync_stream_t * first_stream )
{
    int           ii;
    hb_buffer_t * prev, * buf;

    // Process first_stream first since it has the initial PTS
    prev = NULL;
    for (ii = 0; ii < hb_list_count(first_stream->in_queue);)
    {
        buf = hb_list_item(first_stream->in_queue, ii);

        if (!UpdateSCR(first_stream, buf))
        {
            hb_list_rem(first_stream->in_queue, buf);
        }
        else
        {
            ii++;
            if (first_stream->type == SYNC_TYPE_VIDEO && prev != NULL)
            {
                double duration = buf->s.start - prev->s.start;
                if (duration > 0)
                {
                    prev->s.duration = duration;
                    prev->s.stop = buf->s.start;
                }
            }
            prev = buf;
        }
    }

    for (ii = 0; ii < common->stream_count; ii++)
    {
        sync_stream_t * stream = &common->streams[ii];

        if (stream == first_stream)
        {
            // skip first_stream, already done
            continue;
        }

        int jj;
        prev = NULL;
        for (jj = 0; jj < hb_list_count(stream->in_queue);)
        {
            buf = hb_list_item(stream->in_queue, jj);
            if (!UpdateSCR(stream, buf))
            {
                // Subtitle put into delay queue, remove it from in_queue
                hb_list_rem(stream->in_queue, buf);
            }
            else
            {
                jj++;
                if (stream->type == SYNC_TYPE_VIDEO && prev != NULL)
                {
                    double duration = buf->s.start - prev->s.start;
                    if (duration > 0)
                    {
                        prev->s.duration = duration;
                        prev->s.stop = buf->s.start;
                    }
                }
                prev = buf;
            }
        }
    }
    alignStreams(common, AV_NOPTS_VALUE);
}

static void checkFirstPts( sync_common_t * common )
{
    int             ii;
    int64_t         first_pts = INT64_MAX;
    sync_stream_t * first_stream = NULL;

    for (ii = 0; ii < common->stream_count; ii++)
    {
        sync_stream_t *stream = &common->streams[ii];
        if (stream->type == SYNC_TYPE_SUBTITLE)
        {
            // only wait for audio and video
            continue;
        }

        // If buffers are queued, find the lowest initial PTS
        while (hb_list_count(stream->in_queue) > 0)
        {
            hb_buffer_t * buf = hb_list_item(stream->in_queue, 0);
            if (buf->s.start != AV_NOPTS_VALUE)
            {
                // We require an initial pts for every stream
                if (buf->s.start < first_pts)
                {
                    first_pts = buf->s.start;
                    first_stream = stream;
                }
                break;
            }
            else
            {
                hb_list_rem(stream->in_queue, buf);
                hb_buffer_close(&buf);
            }
        }
    }
    // We should *always* find a first pts because we let the queues
    // fill before performing this test.
    if (first_pts != INT64_MAX)
    {
        common->found_first_pts = 1;
        // We may have buffers that have no timestamps (i.e. AV_NOPTS_VALUE).
        // Compute these timestamps based on previous buffer's timestamp
        // and duration.
        computeInitialTS(common, first_stream);
        // After this initialization, all AV_NOPTS_VALUE timestamps
        // will be filled in by UpdateSCR()
    }
    else
    {
        // This should never happen
        hb_error("checkFirstPts: No initial PTS found!\n");
    }
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
        buf->s.start = stream->next_pts + frame_duration;
        buf = hb_list_item(stream->in_queue, 0);
        buf->s.start = stream->next_pts;
        buf->s.duration = frame_duration;
        buf->s.stop = stream->next_pts + frame_duration;
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
        if (ABS(duration - ii * frame_duration) < frame_duration / 3)
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
    int           drop = 0;
    int64_t       overlap;
    hb_buffer_t * buf;

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
            hb_list_rem(stream->in_queue, buf);
            // Video frame durations are assumed to be variable and are
            // adjusted based on the start time of the next frame before
            // we get to this point.
            //
            // Estimate duration dropped based on average framerate
            stream->drop_duration +=
                            90000. * stream->common->job->title->vrate.den /
                                     stream->common->job->title->vrate.num;
            stream->drop++;
            drop++;
            saveChap(stream, buf);
            hb_buffer_close(&buf);
        }
        else
        {
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

    // If there's a gap of more than a minute between the last
    // frame and this, assume we got a corrupted timestamp.
    if (gap > 90 * 20 && gap < 90000LL * 60)
    {
        if (stream->gap_duration <= 0)
        {
            stream->gap_pts = buf->s.start;
        }
        buf = CreateSilenceBuf(stream, gap, stream->next_pts);
        if (buf != NULL)
        {
            hb_buffer_t * next;
            int           pos;
            for (pos = 0; buf != NULL; buf = next, pos++)
            {
                next = buf->next;
                buf->next = NULL;
                hb_list_insert(stream->in_queue, pos, buf);
            }
        }
        else
        {
            addDelta(stream->common, stream->next_pts, gap);
            applyDeltas(stream->common);
        }
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

static void fixSubtitleOverlap( sync_stream_t * stream )
{
    hb_buffer_t * buf;

    buf = hb_list_item(stream->in_queue, 0);
    if (buf == NULL || (buf->s.flags & HB_BUF_FLAG_EOS) ||
                       (buf->s.flags & HB_BUF_FLAG_EOF))
    {
        // marker to indicate the end of a subtitle
        return;
    }
    // Theoretically only SSA subs can overlap,
    // but there are some SRT subs out there with
    // overlapping samples, so let's try to preserve them too
    if (stream->subtitle.subtitle->source      != SSASUB &&
        stream->subtitle.subtitle->source      != IMPORTSSA &&
        stream->subtitle.subtitle->source      != IMPORTSRT &&
        stream->subtitle.subtitle->config.dest == PASSTHRUSUB &&
        buf->s.start <= stream->last_pts)
    {
        int64_t       overlap;
        overlap = stream->last_pts - buf->s.start;
        hb_log("sync: subtitle 0x%x time went backwards %d ms, PTS %"PRId64"",
               stream->subtitle.subtitle->id, (int)overlap / 90,
               buf->s.start);
        hb_list_rem(stream->in_queue, buf);
        hb_buffer_close(&buf);
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
    else if (stream->type == SYNC_TYPE_SUBTITLE)
    {
        fixSubtitleOverlap(stream);
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

static void streamFlush( sync_stream_t * stream )
{
    while (hb_list_count(stream->in_queue) > 0)
    {
        hb_buffer_t * buf;

        buf = hb_list_item(stream->in_queue, 0);
        hb_list_rem(stream->in_queue, buf);
        hb_buffer_close(&buf);
    }
    fifo_push(stream->fifo_out, hb_buffer_eof_init());
}

static void flushStreams( sync_common_t * common )
{
    int ii;

    // Make sure all streams are complete
    for (ii = 0; ii < common->stream_count; ii++)
    {
        sync_stream_t * stream = &common->streams[ii];
        if (!stream->done && !stream->flush)
        {
            return;
        }
    }

    if (!common->found_first_pts)
    {
        checkFirstPts(common);
    }

    common->flush = 1;
    while (OutputBuffer(common));

    // Flush all streams
    for (ii = 0; ii < common->stream_count; ii++)
    {
        sync_stream_t * stream = &common->streams[ii];
        streamFlush(stream);
    }
}

static void flushStreamsLock( sync_common_t * common )
{
    hb_lock(common->mutex);
    flushStreams(common);
    hb_unlock(common->mutex);
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

// When doing point-to-point encoding, subtitles can cause a long
// delay in finishing the job when the stop point is reached.  This
// is due to the sparse nature of subtitles.  We may not even see
// any additional subtitle till we reach the end of the file.
//
// So when all audio and video streams have reached the end point,
// force the termination of all subtitle streams by pushing an
// end-of-stream buffer into each subtitles input fifo.
// This will cause each subtitle sync worker to wake and return
// HB_WORK_DONE.
static void terminateSubtitleStreams( sync_common_t * common )
{
    int ii;

    // Make sure all streams are complete
    for (ii = 0; ii < common->stream_count; ii++)
    {
        sync_stream_t * stream = &common->streams[ii];
        if (stream->type == SYNC_TYPE_SUBTITLE)
        {
            continue;
        }
        if (!stream->done)
        {
            return;
        }
    }

    // Terminate all subtitle streams
    for (ii = 0; ii < common->stream_count; ii++)
    {
        sync_stream_t * stream = &common->streams[ii];
        if (stream->done || stream->type != SYNC_TYPE_SUBTITLE)
        {
            continue;
        }
        fifo_push(stream->fifo_in,  hb_buffer_eof_init());
        stream->done = 1;
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
static int OutputBuffer( sync_common_t * common )
{
    int             ii, more;
    int64_t         pts;
    sync_stream_t * out_stream;
    hb_buffer_t   * buf;
    int             out_count = 0;

    do
    {
        more        = 0;
        out_stream  = NULL;
        pts         = INT64_MAX;

        // Find lowest PTS and output that buffer
        for (ii = 0; ii < common->stream_count; ii++)
        {
            sync_stream_t * stream = &common->streams[ii];
            int             min    = stream->min_len;

            // Ignore minimum buffer requirements for video if we have
            // not yet found the PtoP start frame.
            if (!common->start_found && common->wait_for_frame &&
                stream->type == SYNC_TYPE_VIDEO)
            {
                min = 0;
            }
            // We need at least 2 buffers in the queue in order to fix
            // frame overlaps and inter-frame gaps.  So if a queue is
            // low, do not do normal PTS interleaving with this queue.
            // Except for subtitles which are not processed for gaps
            // and overlaps.
            if ((common->flush && hb_list_count(stream->in_queue) > 0) ||
                hb_list_count(stream->in_queue) > min)
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
            if ((common->flush && hb_list_count(stream->in_queue) > 0) ||
                hb_list_count(stream->in_queue) > stream->max_len)
            {
                more = 1;
            }
        }
        if (out_stream == NULL)
        {
            // This should only happen if all queues are below the
            // minimum queue level
            break;
        }
        if (out_stream->done)
        {
            buf = hb_list_item(out_stream->in_queue, 0);
            hb_list_rem(out_stream->in_queue, buf);
            hb_buffer_close(&buf);
            continue;
        }

        if (out_stream->next_pts == (int64_t)AV_NOPTS_VALUE)
        {
            // Initialize next_pts, it is used to make timestamp corrections
            // If doing p-to-p encoding, it will get reinitialized when
            // we find the start point.
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
            if (common->wait_for_frame)
            {
                if (out_stream->type != SYNC_TYPE_VIDEO)
                {
                    // We haven't found the PtoP start frame yet and
                    // this buffer is either before the start frame or
                    // the video queue was empty.
                    out_stream->next_pts = buf->s.start + buf->s.duration;
                    hb_list_rem(out_stream->in_queue, buf);
                    hb_buffer_close(&buf);
                    continue;
                }
                common->start_pts = buf->s.start + 1;
                if (out_stream->frame_count >= common->job->frame_to_start)
                {
                    common->start_found = 1;
                    out_stream->frame_count = 0;
                }
            }
            else if (common->wait_for_pts)
            {
                if (buf->s.start >= common->pts_to_start)
                {
                    common->start_found = 1;
                    common->streams[0].frame_count = 0;
                }
            }
            if (!common->start_found)
            {
                if (out_stream->type == SYNC_TYPE_VIDEO)
                {
                    UpdateSearchState(common, buf->s.start,
                                      out_stream->frame_count);
                    out_stream->frame_count++;
                }
                if (out_stream->type == SYNC_TYPE_SUBTITLE &&
                    buf->s.stop > common->start_pts)
                {
                    // Subtitle ends after start time, keep sub and
                    // adjust it's start time
                    buf->s.start = common->start_pts;
                }
                else if (buf->s.start < common->start_pts)
                {
                    out_stream->next_pts = buf->s.start + buf->s.duration;
                    hb_list_rem(out_stream->in_queue, buf);
                    hb_buffer_close(&buf);
                }
                continue;
            }
            // reset frame count to track number of frames after
            // the start position till the end of encode.
            out_stream->frame_count = 0;

            shiftTS(common, buf->s.start);
            alignStreams(common, buf->s.start);
            setNextPts(common);

            buf = hb_list_item(out_stream->in_queue, 0);
            if (buf == NULL)
            {
                // In case aligning timestamps causes all buffers in
                // out_stream to be deleted...
                continue;
            }
        }

        // If pts_to_stop or frame_to_stop were specified, stop output
        if (common->stop_pts &&
            buf->s.start >= common->stop_pts )
        {
            switch (out_stream->type)
            {
                case SYNC_TYPE_VIDEO:
                    hb_log("sync: reached video pts %"PRId64", exiting early",
                           buf->s.start);
                    break;
                case SYNC_TYPE_AUDIO:
                    hb_log("sync: reached audio 0x%x pts %"PRId64
                           ", exiting early",
                           out_stream->audio.audio->id, buf->s.start);
                    break;
                case SYNC_TYPE_SUBTITLE:
                    hb_log("sync: reached subtitle 0x%x pts %"PRId64
                           ", exiting early",
                           out_stream->subtitle.subtitle->id, buf->s.start);
                    break;
                default:
                    break;
            }
            out_stream->done = 1;
            terminateSubtitleStreams(common);
            flushStreams(common);
            continue;
        }
        if (out_stream->type == SYNC_TYPE_VIDEO &&
            common->job->frame_to_stop &&
            out_stream->frame_count >= common->job->frame_to_stop)
        {
            hb_log("sync: reached video frame %d, exiting early",
                   out_stream->frame_count);
            common->stop_pts = buf->s.start;
            out_stream->done = 1;
            terminateSubtitleStreams(common);
            flushStreams(common);
            continue;
        }

        if (out_stream->type == SYNC_TYPE_VIDEO)
        {
            checkCadence(out_stream->video.cadence, buf);
        }

        // Out the buffer goes...
        hb_list_rem(out_stream->in_queue, buf);
        if (out_stream->type == SYNC_TYPE_VIDEO)
        {
            UpdateState(common, out_stream->frame_count);
        }
        if (!out_stream->first_frame)
        {
            if (buf->s.start >= 0)
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
                out_stream->min_frame_duration = buf->s.duration;
            }
            out_stream->next_pts = buf->s.start;
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
        int64_t subtitle_last_pts = AV_NOPTS_VALUE;
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
                subtitle_last_pts = sub->s.start;
                sub = sub->next;
            }
        }
        if (out_stream->type == SYNC_TYPE_AUDIO ||
            out_stream->type == SYNC_TYPE_VIDEO)
        {
            buf->s.start = out_stream->next_pts;
            buf->s.stop  = out_stream->next_pts + buf->s.duration;
            out_stream->last_pts = out_stream->next_pts;
            out_stream->next_pts += buf->s.duration;
        }
        else
        {
            out_stream->next_pts =
            out_stream->last_pts = subtitle_last_pts;
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
        if (buf->s.start < 0)
        {
            // The pipeline can't handle negative timestamps
            // and it is sometimes not possible to avoid one
            // at the start of the video.  There can be a
            // significant delay before we see the first buffers
            // from all streams.  We can't buffer indefinitely
            // until we have seen the first PTS for all streams
            // so sometimes we may start before we have seen
            // the earliest PTS
            saveChap(out_stream, buf);
            hb_buffer_close(&buf);
        }
        restoreChap(out_stream, buf);
        fifo_push(out_stream->fifo_out, buf);
        out_count++;
    } while (more);

    return out_count;
}

static void Synchronize( sync_stream_t * stream )
{
    sync_common_t * common = stream->common;

    // Sync deposits output directly into fifos, so work_loop is not
    // blocking when output fifos become full.  Wait here before
    // performing any output when the output fifo for the input stream
    // is full
    if (stream->fifo_out != NULL && common->start_found)
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

static void updateDuration( sync_stream_t * stream )
{
    // The video decoder sets a nominal duration for frames.  But the
    // actual duration needs to be computed from timestamps.
    if (stream->type == SYNC_TYPE_VIDEO)
    {
        int count = hb_list_count(stream->in_queue);
        if (count >= 2)
        {
            hb_buffer_t * buf1 = hb_list_item(stream->in_queue, count - 1);
            hb_buffer_t * buf2 = hb_list_item(stream->in_queue, count - 2);
            double duration = buf1->s.start - buf2->s.start;
            if (duration > 0)
            {
                buf2->s.duration = duration;
                buf2->s.stop = buf1->s.start;
            }
            else
            {
                buf2->s.duration = 0.;
                buf2->s.start = buf2->s.stop = buf1->s.start;
            }
        }
    }
}

static void ProcessSCRDelayQueue( sync_common_t * common )
{
    int ii, jj;

    for (ii = 0; ii < common->stream_count; ii++)
    {
        sync_stream_t * stream = &common->streams[ii];
        for (jj = 0; jj < hb_list_count(stream->scr_delay_queue);)
        {
            hb_buffer_t * buf = hb_list_item(stream->scr_delay_queue, jj);
            int           hash = buf->s.scr_sequence & SCR_HASH_MASK;
            if (buf->s.scr_sequence < 0)
            {
                // Unset scr_sequence indicates an external stream
                // (e.g. SRT subtitle) that is not on the same timebase
                // as the source tracks. Do not adjust timestamps for
                // scr_offset in this case.
                hb_list_rem(stream->scr_delay_queue, buf);
                SortedQueueBuffer(stream, buf);
            }
            else if (buf->s.scr_sequence == common->scr[hash].scr_sequence)
            {
                if (buf->s.start != AV_NOPTS_VALUE)
                {
                    buf->s.start -= common->scr[hash].scr_offset;
                    buf->s.start -= stream->pts_slip;
                }
                if (buf->s.stop != AV_NOPTS_VALUE)
                {
                    buf->s.stop -= common->scr[hash].scr_offset;
                    buf->s.stop -= stream->pts_slip;
                }
                hb_list_rem(stream->scr_delay_queue, buf);
                SortedQueueBuffer(stream, buf);
            }
            else
            {
                jj++;
            }
        }
    }
}

static const char * getStreamType( sync_stream_t * stream )
{
    switch (stream->type)
    {
        case SYNC_TYPE_VIDEO:
            return "Video";
        case SYNC_TYPE_AUDIO:
            return "Audio";
        case SYNC_TYPE_SUBTITLE:
            return "Subtitle";
        default:
            return "Unknown";
    }
}

static int getStreamId( sync_stream_t * stream )
{
    switch (stream->type)
    {
        case SYNC_TYPE_VIDEO:
            return stream->video.id;
        case SYNC_TYPE_AUDIO:
            return stream->audio.audio->id;
        case SYNC_TYPE_SUBTITLE:
            return stream->subtitle.subtitle->id;
        default:
            return -1;
    }
}

static int UpdateSCR( sync_stream_t * stream, hb_buffer_t * buf )
{
    int             hash = buf->s.scr_sequence & SCR_HASH_MASK;
    sync_common_t * common = stream->common;
    double          last_scr_pts, last_duration;
    int64_t         scr_offset = 0;

    if (buf->s.scr_sequence < stream->last_scr_sequence)
    {
        // In decoder error conditions, the decoder can send us out of
        // order frames.  Often the stream error will also trigger a
        // discontinuity detection.  An out of order frame will
        // cause an incorrect SCR offset, so drop such frames.
        hb_deep_log(3, "SCR sequence went backwards %d -> %d",
                    stream->last_scr_sequence, buf->s.scr_sequence);
        hb_buffer_close(&buf);
        return 0;
    }
    if (buf->s.scr_sequence >= 0)
    {
        if (buf->s.scr_sequence != common->scr[hash].scr_sequence)
        {
            if (stream->type == SYNC_TYPE_SUBTITLE ||
                (stream->last_scr_pts == (int64_t)AV_NOPTS_VALUE &&
                 common->first_scr))
            {
                // We got a new scr, but we have no last_scr_pts to base it
                // off of. Delay till we can compute the scr offset from a
                // different stream.
                hb_list_add(stream->scr_delay_queue, buf);
                return 0;
            }
            if (buf->s.start != AV_NOPTS_VALUE)
            {
                last_scr_pts  = stream->last_scr_pts;
                last_duration = stream->last_duration;
                if (last_scr_pts == (int64_t)AV_NOPTS_VALUE)
                {
                    last_scr_pts      = 0.;
                    last_duration     = 0.;
                    common->first_scr = 1;
                }
                // New SCR.  Compute SCR offset
                common->scr[hash].scr_sequence = buf->s.scr_sequence;
                common->scr[hash].scr_offset   = buf->s.start -
                                                 (last_scr_pts + last_duration);
                hb_deep_log(4,
                    "New SCR: type %8s id %x scr seq %d scr offset %"PRId64" "
                    "start %"PRId64" last %f dur %f",
                    getStreamType(stream), getStreamId(stream),
                    buf->s.scr_sequence, common->scr[hash].scr_offset,
                    buf->s.start, last_scr_pts, last_duration);
                ProcessSCRDelayQueue(common);
            }
        }
        scr_offset = common->scr[hash].scr_offset;
    }

    // Adjust buffer timestamps for SCR offset
    if (buf->s.start != AV_NOPTS_VALUE)
    {
        buf->s.start -= scr_offset;
        last_scr_pts  = buf->s.start;
    }
    else if (stream->last_scr_pts != (int64_t)AV_NOPTS_VALUE)
    {
        last_scr_pts = stream->last_scr_pts + stream->last_duration;
        buf->s.start = last_scr_pts;
    }
    else
    {
        // This should happen extremely rarely if ever.
        // But if we get here, this is the first buffer received for
        // this stream. Normally, some buffers will be queued for
        // every stream before we ever call UpdateSCR.
        // We don't really know what it's timestamp should be.
        // So drop the buffer.
        hb_buffer_close(&buf);
        return 0;
    }
    if (buf->s.stop != AV_NOPTS_VALUE)
    {
        buf->s.stop -= scr_offset;
    }
    if (last_scr_pts > stream->last_scr_pts)
    {
        stream->last_scr_pts = last_scr_pts;
    }
    if (buf->s.scr_sequence > stream->last_scr_sequence)
    {
        stream->last_scr_sequence = buf->s.scr_sequence;
    }
    stream->last_duration = buf->s.duration;

    return 1;
}

// Handle broken timestamps that are out of order
// These are usually due to a broken decoder (e.g. QSV and libav AVI packed
// b-frame support).  But sometimes can come from a severely broken or
// corrupted source file.
//
// We can pretty reliably fix out of order timestamps *if* every frame
// has a timestamp.  But some container formats allow frames with no
// timestamp (e.g. TS and PS).  When there is no timestamp, we will
// compute one based on the last frames timestamp.  If the missing
// timestamp is out of order and really belonged on an earlier frame (A),
// this will result in the frame before (A) being long and the frame
// after the current will overlap current.
//
// The condition above of one long frame and one overlap will most likely
// get fixed by dejitterVideo. dejitterVideo finds sequences where the
// sum of the durations of frames 1..N == (1/fps) * N. When it finds such
// a sequence, it adjusts the frame durations to all be 1/fps. Since the
// vast majority of video is constant framerate, this will fix the above
// problem most of the time.
static void SortedQueueBuffer( sync_stream_t * stream, hb_buffer_t * buf )
{
    int64_t start;
    int     ii, count;

    start = buf->s.start;
    hb_list_add(stream->in_queue, buf);

    // Search for the first earlier timestamp that is < this one.
    // Under normal circumstances where the timestamps are not broken,
    // this will only check the next to last buffer in the queue
    // before aborting.
    count = hb_list_count(stream->in_queue);
    for (ii = count - 2; ii >= 0; ii--)
    {
        buf = hb_list_item(stream->in_queue, ii);
        if (buf->s.start < start || start == AV_NOPTS_VALUE)
        {
            break;
        }
    }
    if (ii < count - 2)
    {
        hb_buffer_t * prev = NULL;
        int           jj;

        // The timestamp was out of order.
        // The timestamp belongs at position ii + 1
        // Every timestamp from ii + 2 to count - 1 needs to be shifted up.
        if (ii >= 0)
        {
            prev = hb_list_item(stream->in_queue, ii);
        }
        for (jj = ii + 1; jj < count; jj++)
        {
            int64_t tmp_start;

            buf = hb_list_item(stream->in_queue, jj);
            tmp_start = buf->s.start;
            buf->s.start = start;
            start = tmp_start;
            if (stream->type == SYNC_TYPE_VIDEO && prev != NULL)
            {
                // recompute video buffer duration
                prev->s.duration = buf->s.start - prev->s.start;
                prev->s.stop     = buf->s.start;
            }
            prev = buf;
        }
    }
}

static void QueueBuffer( sync_stream_t * stream, hb_buffer_t * buf )
{
    hb_lock(stream->common->mutex);

    while (hb_list_count(stream->in_queue) > stream->max_len &&
           !stream->done && !stream->common->job->done &&
           !*stream->common->job->die)
    {
        // If the in_queue is full, we have to force some output to
        // unblock it.  Blocking here would back up the pipeline and
        // stall out reader eventually.
        hb_unlock(stream->common->mutex);
        Synchronize(stream);
        hb_lock(stream->common->mutex);
    }

    // Reader can change job->reader_pts_offset after initialization
    // and before we receive the first buffer here.  Calculate
    // common->pts_to_start here since this is the first opportunity where
    // all the necessary information exists.
    if (stream->common->pts_to_start == AV_NOPTS_VALUE)
    {
        hb_job_t * job = stream->common->job;
        if (job->pts_to_start > 0)
        {
            stream->common->start_pts    =
            stream->common->pts_to_start =
                MAX(0, job->pts_to_start - job->reader_pts_offset);
        }
    }

    // Render offset is only useful for decoders, which are all
    // upstream of sync.  Squash it.
    buf->s.renderOffset = AV_NOPTS_VALUE;

    hb_deep_log(11,
        "type %8s id %x scr seq %d start %"PRId64" stop %"PRId64" dur %f",
        getStreamType(stream), getStreamId(stream), buf->s.scr_sequence,
        buf->s.start, buf->s.stop, buf->s.duration);

    if (stream->common->found_first_pts)
    {
        if (UpdateSCR(stream, buf) > 0)
        {
            // Apply any stream slips.
            // Stream slips will only temporarily differ between
            // the streams.  The slips get updated in applyDeltas.  When
            // all the deltas are absorbed, the stream slips will all
            // be equal.
            buf->s.start -= stream->pts_slip;
            if (buf->s.stop != AV_NOPTS_VALUE)
            {
                buf->s.stop -= stream->pts_slip;
            }

            SortedQueueBuffer(stream, buf);
            updateDuration(stream);
        }
    }
    else
    {
        if (buf->s.start == AV_NOPTS_VALUE &&
            hb_list_count(stream->in_queue) == 0)
        {
            // We require an initial pts to start synchronization
            saveChap(stream, buf);
            hb_buffer_close(&buf);
            hb_unlock(stream->common->mutex);
            return;
        }
        SortedQueueBuffer(stream, buf);
    }

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
    w->fifo_out     = audio->priv.fifo_sync;

    pv->common                  = common;
    pv->stream                  = &common->streams[1 + index];
    pv->stream->common          = common;
    pv->stream->in_queue        = hb_list_init();
    pv->stream->scr_delay_queue = hb_list_init();
    pv->stream->max_len         = SYNC_MAX_AUDIO_QUEUE_LEN;
    pv->stream->min_len         = SYNC_MIN_AUDIO_QUEUE_LEN;
    if (pv->stream->in_queue == NULL) goto fail;
    pv->stream->delta_list      = hb_list_init();
    if (pv->stream->delta_list == NULL) goto fail;
    pv->stream->type            = SYNC_TYPE_AUDIO;
    pv->stream->first_pts       = AV_NOPTS_VALUE;
    pv->stream->next_pts        = (int64_t)AV_NOPTS_VALUE;
    pv->stream->last_pts        = (int64_t)AV_NOPTS_VALUE;
    pv->stream->last_scr_pts    = (int64_t)AV_NOPTS_VALUE;
    pv->stream->last_scr_sequence = -1;
    pv->stream->last_duration   = (int64_t)AV_NOPTS_VALUE;
    pv->stream->audio.audio     = audio;
    pv->stream->fifo_out        = w->fifo_out;

    if (!(audio->config.out.codec & HB_ACODEC_PASS_FLAG) &&
        audio->config.in.samplerate != audio->config.out.samplerate)
    {
        /* Initialize samplerate conversion */
        pv->stream->audio.resample =
            hb_audio_resample_init(AV_SAMPLE_FMT_FLT,
                                   audio->config.out.samplerate,
                                   audio->config.out.mixdown,
                                   audio->config.out.normalize_mix_level);
        if (pv->stream->audio.resample == NULL)
        {
            hb_error("sync: audio 0x%x resample init failed", audio->id);
            goto fail;
        }
        hb_audio_resample_set_sample_rate(pv->stream->audio.resample,
                                          audio->config.in.samplerate);
        if (hb_audio_resample_update(pv->stream->audio.resample))
        {
            hb_error("sync: audio 0x%x resample update failed", audio->id);
            goto fail;
        }
    }

    pv->stream->audio.gain_factor = pow(10, audio->config.out.gain / 20);

    hb_list_add(common->list_work, w);

    return 0;

fail:
    if (pv != NULL)
    {
        if (pv->stream != NULL)
        {
            if (pv->stream->audio.resample)
            {
                hb_audio_resample_free(pv->stream->audio.resample);
            }
            hb_list_close(&pv->stream->delta_list);
            hb_list_close(&pv->stream->in_queue);
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
    pv->stream->in_queue          = hb_list_init();
    pv->stream->scr_delay_queue   = hb_list_init();
    pv->stream->max_len           = SYNC_MAX_SUBTITLE_QUEUE_LEN;
    pv->stream->min_len           = SYNC_MIN_SUBTITLE_QUEUE_LEN;
    if (pv->stream->in_queue == NULL) goto fail;
    pv->stream->delta_list        = hb_list_init();
    if (pv->stream->delta_list == NULL) goto fail;
    pv->stream->type              = SYNC_TYPE_SUBTITLE;
    pv->stream->first_pts         = AV_NOPTS_VALUE;
    pv->stream->next_pts          = (int64_t)AV_NOPTS_VALUE;
    pv->stream->last_pts          = (int64_t)AV_NOPTS_VALUE;
    pv->stream->last_scr_pts      = (int64_t)AV_NOPTS_VALUE;
    pv->stream->last_scr_sequence = -1;
    pv->stream->last_duration     = (int64_t)AV_NOPTS_VALUE;
    pv->stream->subtitle.subtitle = subtitle;
    pv->stream->fifo_out          = subtitle->fifo_sync;
    pv->stream->fifo_in           = subtitle->fifo_in;

    w = hb_get_work(common->job->h, WORK_SYNC_SUBTITLE);
    w->private_data = pv;
    w->subtitle     = subtitle;
    w->fifo_in      = subtitle->fifo_raw;
    w->fifo_out     = subtitle->fifo_sync;

    memset(&pv->stream->subtitle.sanitizer, 0,
           sizeof(pv->stream->subtitle.sanitizer));
    if (subtitle->format == TEXTSUB && subtitle->config.dest == PASSTHRUSUB &&
        (common->job->mux & HB_MUX_MASK_MP4))
    {
        // Merge overlapping subtitles since mpv tx3g does not support them
        pv->stream->subtitle.sanitizer.merge = 1;
    }
    // PGS & DVB subtitles don't need to be linked because there are explicit
    // "clear" subtitle packets that indicate the end time of the
    // previous subtitle
    if (subtitle->config.dest == PASSTHRUSUB &&
        subtitle->source != PGSSUB &&
        subtitle->source != DVBSUB)
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
    pv->stream                  = &pv->common->streams[0];
    pv->stream->common          = pv->common;
    pv->stream->in_queue        = hb_list_init();
    pv->stream->scr_delay_queue = hb_list_init();
    pv->stream->max_len         = SYNC_MAX_VIDEO_QUEUE_LEN;
    pv->stream->min_len         = SYNC_MIN_VIDEO_QUEUE_LEN;
    if (pv->stream->in_queue == NULL) goto fail;
    pv->stream->delta_list      = hb_list_init();
    if (pv->stream->delta_list == NULL) goto fail;
    pv->stream->type            = SYNC_TYPE_VIDEO;
    pv->stream->first_pts       = AV_NOPTS_VALUE;
    pv->stream->next_pts        = (int64_t)AV_NOPTS_VALUE;
    pv->stream->last_pts        = (int64_t)AV_NOPTS_VALUE;
    pv->stream->last_scr_pts    = (int64_t)AV_NOPTS_VALUE;
    pv->stream->last_scr_sequence = -1;
    pv->stream->last_duration   = (int64_t)AV_NOPTS_VALUE;
    pv->stream->fifo_out        = job->fifo_sync;
    pv->stream->video.id        = job->title->video_id;

    w->fifo_in                  = job->fifo_raw;
    w->fifo_out                 = job->fifo_sync;

    if (job->pass_id == HB_PASS_ENCODE_FINAL)
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
            int64_t duration, total_duration, extra_duration = 0;
            if (job->pts_to_stop)
            {
                duration = job->pts_to_stop + 90000;
            }
            else
            {
                total_duration = 0;
                for (ii = 0; ii <= hb_list_count(job->list_chapter); ii++)
                {
                    hb_chapter_t * chapter;
                    chapter = hb_list_item(job->list_chapter, ii - 1);
                    if (chapter != NULL)
                    {
                        total_duration += chapter->duration;
                    }
                }
                // Some titles are longer than the sum duration of their
                // chapters.  Account for this extra duration.
                if (job->title->duration > total_duration)
                {
                    extra_duration = job->title->duration - total_duration;
                }
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
                if (job->chapter_end == hb_list_count(job->list_chapter))
                {
                    duration += extra_duration;
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
        pv->common->start_found    = 0;
        pv->common->start_pts      = job->pts_to_start;
        pv->common->wait_for_frame = !!job->frame_to_start;
        pv->common->wait_for_pts   = !!job->pts_to_start;
        if (job->pts_to_start)
        {
            pv->common->pts_to_start = AV_NOPTS_VALUE;
        }
    }
    else
    {
        pv->common->start_found = 1;
    }
    if (job->pts_to_stop)
    {
        pv->common->stop_pts = job->pts_to_stop;
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
    if( job->pass_id == HB_PASS_ENCODE_ANALYSIS )
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
    hb_list_empty(&pv->stream->scr_delay_queue);

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
        // Strip trailing CR and/or LF
        len = strlen((char*)a->data);
        if (len > 0 && a->data[len - 1] == '\n')
        {
            a->data[len - 1] = 0;
            len--;
            if (len > 0 && a->data[len - 1] == '\r')
            {
                a->data[len - 1] = 0;
            }
        }
        // Text subtitles are SSA internally.  Use SSA newline code
        // and force style reset at beginning of new line.
        len = snprintf((char*)buf->data, buf->size, "%s\\N{\\r}%s", a->data, text);
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
    stop = start = a->s.start;
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
        // Handle all but the last buffer
        // The last buffer may not have been "linked" yet
        while (hb_buffer_list_count(&sanitizer->list_current) > 0)
        {
            buf = hb_buffer_list_head(&sanitizer->list_current);
            if (!(buf->s.flags & HB_BUF_FLAG_EOF) &&
                buf->s.stop != AV_NOPTS_VALUE)
            {
                buf = hb_buffer_list_rem_head(&sanitizer->list_current);
                buf = setSubDuration(stream, buf);
                hb_buffer_list_append(&list, buf);
            }
            else
            {
                break;
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
        if (sub != NULL)
        {
            hb_buffer_close(&sub);
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

#if HB_PROJECT_FEATURE_QSV
    // Fix of LA case allowing use of LA up to 40 in full encode path,
    // as currently for such support we cannot allocate >64 slices per texture
    // due to MSFT limitation, not impacting other cases
    hb_job_t *job = pv->common->job;
    if (job->hw_pix_fmt == AV_PIX_FMT_QSV &&
        job->qsv_ctx->la_is_enabled == 1)
    {
        pv->stream->max_len = SYNC_MIN_VIDEO_QUEUE_LEN;
        pv->common->job->qsv_ctx->la_is_enabled++;
    }
#endif

    if (pv->stream->done)
    {
        flushStreamsLock(pv->common);
        return HB_WORK_DONE;
    }
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        pv->stream->flush = 1;
        flushStreamsLock(pv->common);
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

    if (pv->stream->done)
    {
        flushStreamsLock(pv->common);
        return HB_WORK_DONE;
    }
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
    if (pv->stream->audio.resample)
    {
        hb_audio_resample_free(pv->stream->audio.resample);
    }

    sync_delta_t * delta;
    while ((delta = hb_list_item(pv->stream->delta_list, 0)) != NULL)
    {
        hb_list_rem(pv->stream->delta_list, delta);
        free(delta);
    }
    hb_list_close(&pv->stream->delta_list);
    hb_list_empty(&pv->stream->in_queue);
    hb_list_empty(&pv->stream->scr_delay_queue);
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

    if (pv->stream->done)
    {
        flushStreamsLock(pv->common);
        return HB_WORK_DONE;
    }
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        pv->stream->flush = 1;
        flushStreamsLock(pv->common);
        return HB_WORK_DONE;
    }

    *buf_in = NULL;
    QueueBuffer(pv->stream, in);
    Synchronize(pv->stream);

    if (pv->stream->done)
    {
        flushStreamsLock(pv->common);
        return HB_WORK_DONE;
    }
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
        if (stream->audio.resample != NULL)
        {
            /* do sample rate conversion */
            hb_buffer_t * out;
            int           nsamples, sample_size;

            sample_size = hb_mixdown_get_discrete_channel_count(
                            audio->config.out.mixdown ) * sizeof( float );

            nsamples  = buf->size / sample_size;
            out = hb_audio_resample(stream->audio.resample,
                                    (const uint8_t **)&buf->data,
                                    nsamples);
            hb_buffer_close(&buf);
            if (out == NULL)
            {
                return NULL;
            }
            out->s.start = stream->next_pts;
            out->s.stop  = stream->next_pts + out->s.duration;
            buf = out;
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

    if (job->indepth_scan)
    {
        // Progress for indepth scan is handled by reader
        // frame_count is used during indepth_scan
        // to find start & end points.
        return;
    }

    if (frame_count == 0)
    {
        common->st_first = hb_get_date();
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

    hb_get_state2(job->h, &state);
    state.state = HB_STATE_WORKING;

#define p state.param.working
    p.progress  = (float)frame_count / common->est_frame_count;
    if (p.progress > 1.0)
    {
        p.progress = 1.0;
    }
    p.rate_cur   = 1000.0 * (common->st_counts[3] - common->st_counts[0]) /
                            (common->st_dates[3]  - common->st_dates[0]);
    if (hb_get_date() > common->st_first + 4000)
    {
        p.rate_avg = 1000.0 * common->st_counts[3] /
                     (common->st_dates[3] - common->st_first - job->st_paused);
        if (common->est_frame_count >= common->st_counts[3])
        {
            int eta = (common->est_frame_count - common->st_counts[3]) / p.rate_avg;
            p.eta_seconds = eta;
            p.hours       = eta / 3600;
            p.minutes     = (eta % 3600) / 60;
            p.seconds     = eta % 60;
        }
        else
        {
            p.eta_seconds = 0;
            p.hours    = -1;
            p.minutes  = -1;
            p.seconds  = -1;
        }
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

    if (job->indepth_scan)
    {
        // Progress for indepth scan is handled by reader
        // frame_count is used during indepth_scan
        // to find start & end points.
        return;
    }

    now = hb_get_date();
    if (frame_count == 0)
    {
        common->st_first = now;
    }

    hb_get_state2(job->h, &state);
    state.state = HB_STATE_SEARCHING;

#define p state.param.working
    if (common->wait_for_frame)
        p.progress  = (float)frame_count / job->frame_to_start;
    else if (common->wait_for_pts)
        p.progress  = (float) start / common->pts_to_start;
    else
        p.progress = 0;
    if (p.progress > 1.0)
    {
        p.progress = 1.0;
    }
    if (now > common->st_first)
    {
        int eta = 0;

        if (common->wait_for_frame)
        {
            avg = 1000.0 * frame_count / (now - common->st_first);
            eta = (job->frame_to_start - frame_count ) / avg;
        }
        else if (common->wait_for_pts)
        {
            avg = 1000.0 * start / (now - common->st_first);
            eta = (common->pts_to_start - start) / avg;
        }
        p.eta_seconds = eta;
        p.hours       = eta / 3600;
        p.minutes     = (eta % 3600) / 60;
        p.seconds     = eta % 60;
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
    hb_list_empty(&pv->stream->scr_delay_queue);
    hb_buffer_list_close(&pv->stream->subtitle.sanitizer.list_current);
    free(pv);
    w->private_data = NULL;
}

static int syncSubtitleWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                             hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;

    if (pv->stream->done)
    {
        flushStreamsLock(pv->common);
        return HB_WORK_DONE;
    }
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        pv->stream->flush = 1;
        // sanitizeSubtitle requires EOF buffer to recognize that
        // it needs to flush all subtitles.
        hb_list_add(pv->stream->in_queue, hb_buffer_eof_init());
        flushStreamsLock(pv->common);
        if (pv->common->job->indepth_scan)
        {
            // When doing subtitle indepth scan, the pipeline ends at sync.
            // Terminate job when EOF reached.
            *w->done = 1;
        }
        return HB_WORK_DONE;
    }

    if (pv->common->job->indepth_scan)
    {
        // When doing subtitle indepth scan, the pipeline ends at sync,
        // do not add the subtitles to the queue
        return HB_WORK_OK;
    }
    else
    {
        *buf_in = NULL;
    }

    QueueBuffer(pv->stream, in);
    Synchronize(pv->stream);

    if (pv->stream->done)
    {
        flushStreamsLock(pv->common);
        return HB_WORK_DONE;
    }
    return HB_WORK_OK;
}

hb_work_object_t hb_sync_subtitle =
{
    WORK_SYNC_SUBTITLE,
    "Subtitle Synchronization",
    syncSubtitleInit,
    syncSubtitleWork,
    syncSubtitleClose
};
