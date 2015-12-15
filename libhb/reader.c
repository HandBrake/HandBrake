/* reader.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
#include "hb.h"

static int  reader_init( hb_work_object_t * w, hb_job_t * job );
static void reader_close( hb_work_object_t * w );
static int  reader_work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                         hb_buffer_t ** buf_out);

hb_work_object_t hb_reader =
{
    .id     = WORK_READER,
    .name   = "Reader",
    .init   = reader_init,
    .work   = reader_work,
    .close  = reader_close,
    .info   = NULL,
    .bsinfo = NULL,
    .flush  = NULL
};

typedef struct
{
    int    startup;
    double average; // average time between packets
    double filtered_average; // average time between packets
    int64_t last;   // last timestamp seen on this stream
    int id;         // stream id
    int is_audio;   // != 0 if this is an audio stream
    int valid;      // Stream timing is not valid until next scr.
} stream_timing_t;

typedef struct
{
    int              id;
    hb_buffer_list_t list;
} buffer_splice_list_t;

struct hb_work_private_s
{
    hb_handle_t  * h;
    hb_job_t     * job;
    hb_title_t   * title;
    volatile int * die;

    hb_bd_t      * bd;
    hb_dvd_t     * dvd;
    hb_stream_t  * stream;

    stream_timing_t *stream_timing;
    int64_t        scr_offset;
    int            sub_scr_set;
    hb_psdemux_t   demux;
    int            scr_changes;
    uint32_t       sequence;
    uint8_t        st_slots;        // size (in slots) of stream_timing array
    uint8_t        saw_video;       // != 0 if we've seen video
    uint8_t        saw_audio;       // != 0 if we've seen audio

    int            start_found;     // found pts_to_start point
    int64_t        pts_to_start;
    int            chapter_end;
    uint64_t       st_first;
    uint64_t       duration;
    hb_fifo_t   ** fifos;

    buffer_splice_list_t * splice_list;
    int                    splice_list_size;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static hb_fifo_t ** GetFifoForId( hb_work_private_t * r, int id );
static void UpdateState( hb_work_private_t  * r, int64_t start);
static hb_buffer_list_t * get_splice_list(hb_work_private_t * r, int id);

/***********************************************************************
 * reader_init
 ***********************************************************************
 *
 **********************************************************************/
static int hb_reader_open( hb_work_private_t * r )
{
    if ( r->title->type == HB_BD_TYPE )
    {
        if ( !( r->bd = hb_bd_init( r->h, r->title->path ) ) )
            return 1;
        if(!hb_bd_start(r->bd, r->title))
        {
            hb_bd_close(&r->bd);
            return 1;
        }
        if (r->job->start_at_preview)
        {
            // XXX code from DecodePreviews - should go into its own routine
            hb_bd_seek(r->bd, (float)r->job->start_at_preview /
                       (r->job->seek_points ? (r->job->seek_points + 1.0)
                                            : 11.0));
        }
        else if (r->job->pts_to_start)
        {
            // Note, bd seeks always put us to an i-frame.  no need
            // to start decoding early using r->pts_to_start
            hb_bd_seek_pts(r->bd, r->job->pts_to_start);
            r->duration -= r->job->pts_to_start;
            r->job->pts_to_start = 0;
            r->start_found = 1;
        }
        else
        {
            hb_bd_seek_chapter(r->bd, r->job->chapter_start);
        }
        if (r->job->angle > 1)
        {
            hb_bd_set_angle(r->bd, r->job->angle - 1);
        }
    }
    else if (r->title->type == HB_DVD_TYPE)
    {
        if ( !( r->dvd = hb_dvd_init( r->h, r->title->path ) ) )
            return 1;
        if(!hb_dvd_start( r->dvd, r->title, r->job->chapter_start))
        {
            hb_dvd_close(&r->dvd);
            return 1;
        }
        if (r->job->angle)
        {
            hb_dvd_set_angle(r->dvd, r->job->angle);
        }

        if (r->job->start_at_preview)
        {
            hb_dvd_seek(r->dvd, (float)r->job->start_at_preview /
                        (r->job->seek_points ? (r->job->seek_points + 1.0)
                                             : 11.0));
        }
    }
    else if (r->title->type == HB_STREAM_TYPE ||
             r->title->type == HB_FF_STREAM_TYPE)
    {
        if (!(r->stream = hb_stream_open(r->h, r->title->path, r->title, 0)))
            return 1;
        if (r->job->start_at_preview)
        {
            hb_stream_seek(r->stream, (float)(r->job->start_at_preview - 1) /
                           (r->job->seek_points ? (r->job->seek_points + 1.0)
                                                : 11.0));
        }
        else if (r->job->pts_to_start)
        {
            if (hb_stream_seek_ts( r->stream, r->job->pts_to_start ) >= 0)
            {
                // Seek takes us to the nearest I-frame before the timestamp
                // that we want.  So we will retrieve the start time of the
                // first packet we get, subtract that from pts_to_start, and
                // inspect the reset of the frames in sync.
                r->start_found = 2;
                r->duration -= r->job->pts_to_start;
            }
            // hb_stream_seek_ts does nothing for TS streams and will return
            // an error.
        }
        else
        {
            //
            // Standard stream, seek to the starting chapter, if set,
            // and track the end chapter so that we end at the right time.
            hb_chapter_t *chap;
            int start = r->job->chapter_start;
            chap = hb_list_item(r->job->list_chapter, r->job->chapter_end - 1);

            r->chapter_end = chap->index;
            if (start > 1)
            {
                chap = hb_list_item(r->job->list_chapter, start - 1);
                start = chap->index;
            }

            /*
             * Seek to the start chapter.
             */
            hb_stream_seek_chapter(r->stream, start);
        }
    }
    else
    {
        // Unknown type, should never happen
        return 1;
    }

    return 0;
}

static int reader_init( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * r;

    r = calloc( sizeof( hb_work_private_t ), 1 );
    w->private_data = r;

    r->h     = job->h;
    r->job   = job;
    r->title = job->title;
    r->die   = job->die;
    r->sequence = 0;

    r->st_slots = 4;
    r->stream_timing = calloc( sizeof(stream_timing_t), r->st_slots );
    r->stream_timing[0].id = r->title->video_id;
    r->stream_timing[0].average = 90000. * (double)job->vrate.den /
                                           job->vrate.num;
    r->stream_timing[0].filtered_average = r->stream_timing[0].average;
    r->stream_timing[0].last = -r->stream_timing[0].average;
    r->stream_timing[0].valid = 1;
    r->stream_timing[0].startup = 10;
    r->stream_timing[1].id = -1;

    r->demux.last_scr = AV_NOPTS_VALUE;

    r->chapter_end = job->chapter_end;
    if ( !job->pts_to_start )
        r->start_found = 1;
    else
    {
        // The frame at the actual start time may not be an i-frame
        // so can't be decoded without starting a little early.
        // sync.c will drop early frames.
        // Starting a little over 10 seconds early
        r->pts_to_start = MAX(0, job->pts_to_start - 1000000);
    }

    if (job->pts_to_stop)
    {
        r->duration = job->pts_to_start + job->pts_to_stop;
    }
    else if (job->frame_to_stop)
    {
        int frames = job->frame_to_start + job->frame_to_stop;
        r->duration = (int64_t)frames * job->title->vrate.den * 90000 /
                               job->title->vrate.num;
    }
    else
    {
        hb_chapter_t *chapter;
        int ii;

        r->duration = 0;
        for (ii = job->chapter_start; ii < job->chapter_end; ii++)
        {
            chapter = hb_list_item( job->title->list_chapter, ii - 1);
            r->duration += chapter->duration;
        }
    }

    // Count number of splice lists needed for merging buffers
    // that have been split
    int count = 1; // 1 for video
    count += hb_list_count( job->list_subtitle );
    count += hb_list_count( job->list_audio );
    r->splice_list_size = count;
    r->splice_list = calloc(count, sizeof(buffer_splice_list_t));

    // Initialize stream id's of splice lists
    int ii, jj = 0;
    r->splice_list[jj++].id = r->title->video_id;
    for (ii = 0; ii < hb_list_count(job->list_subtitle); ii++)
    {
        hb_subtitle_t * subtitle = hb_list_item(job->list_subtitle, ii);
        r->splice_list[jj++].id = subtitle->id;
    }
    for (ii = 0; ii < hb_list_count(job->list_audio); ii++)
    {
        hb_audio_t * audio = hb_list_item(job->list_audio, ii);
        r->splice_list[jj++].id = audio->id;
    }

    // count also happens to be the upper bound for the number of
    // fifos that will be needed (+1 for null terminator)
    r->fifos = calloc(count + 1, sizeof(hb_fifo_t*));

    // The stream needs to be open before starting the reader thead
    // to prevent a race with decoders that may share information
    // with the reader. Specifically avcodec needs this.
    if ( hb_reader_open( r ) )
    {
        free( r->stream_timing );
        free( r );
        return 1;
    }
    return 0;
}


static void reader_close( hb_work_object_t * w )
{
    hb_work_private_t * r = w->private_data;

    if ( r == NULL )
    {
        return;
    }
    if (r->bd)
    {
        hb_bd_stop( r->bd );
        hb_bd_close( &r->bd );
    }
    else if (r->dvd)
    {
        hb_dvd_stop( r->dvd );
        hb_dvd_close( &r->dvd );
    }
    else if (r->stream)
    {
        hb_stream_close(&r->stream);
    }

    if ( r->stream_timing )
    {
        free( r->stream_timing );
    }

    free( r->fifos );
    free( r );
}

static hb_buffer_t * splice_discontinuity( hb_work_private_t *r, hb_buffer_t *buf )
{
    // Handle buffers that were split across a PCR discontinuity.
    // Rejoin them into a single buffer.
    hb_buffer_list_t * list = get_splice_list(r, buf->s.id);
    if (list != NULL)
    {
        hb_buffer_list_append(list, buf);
        if (buf->s.split)
        {
            return NULL;
        }

        int count = hb_buffer_list_count(list);
        if (count > 1)
        {
            int size = hb_buffer_list_size(list);
            hb_buffer_t * b = hb_buffer_init(size);
            buf = hb_buffer_list_head(list);
            b->s = buf->s;

            int pos = 0;
            while ((buf = hb_buffer_list_rem_head(list)) != NULL)
            {
                memcpy(b->data + pos, buf->data, buf->size);
                pos += buf->size;
                hb_buffer_close(&buf);
            }
            buf = b;
        }
        else
        {
            buf = hb_buffer_list_clear(list);
        }
    }
    return buf;
}

static void push_buf( hb_work_private_t *r, hb_fifo_t *fifo, hb_buffer_t *buf )
{
    while ( !*r->die && !r->job->done )
    {
        if ( hb_fifo_full_wait( fifo ) )
        {
            hb_fifo_push( fifo, buf );
            buf = NULL;
            break;
        }
    }
    if ( buf )
    {
        hb_buffer_close( &buf );
    }
}

static int is_audio( hb_work_private_t *r, int id )
{
    int i;
    hb_audio_t *audio;

    for( i = 0; ( audio = hb_list_item( r->title->list_audio, i ) ); ++i )
    {
        if ( audio->id == id )
        {
            return 1;
        }
    }
    return 0;
}

static int is_subtitle( hb_work_private_t *r, int id )
{
    int i;
    hb_subtitle_t *sub;

    for( i = 0; ( sub = hb_list_item( r->title->list_subtitle, i ) ); ++i )
    {
        if ( sub->id == id )
        {
            return 1;
        }
    }
    return 0;
}

// The MPEG STD (Standard Target Decoder) essentially requires that we keep
// per-stream timing so that when there's a timing discontinuity we can
// seemlessly join packets on either side of the discontinuity. This join
// requires that we know the timestamp of the previous packet and the
// average inter-packet time (since we position the new packet at the end
// of the previous packet). The next four routines keep track of this
// per-stream timing.

// find or create the per-stream timing state for 'buf'

static stream_timing_t *id_to_st( hb_work_private_t *r, const hb_buffer_t *buf, int valid )
{
    stream_timing_t *st = r->stream_timing;
    while ( st->id != buf->s.id && st->id != -1)
    {
        ++st;
    }
    // if we haven't seen this stream add it.
    if ( st->id == -1 )
    {
        // we keep the steam timing info in an array with some power-of-two
        // number of slots. If we don't have two slots left (one for our new
        // entry plus one for the "-1" eol) we need to expand the array.
        int slot = st - r->stream_timing;
        if ( slot + 1 >= r->st_slots )
        {
            r->st_slots *= 2;
            r->stream_timing = realloc( r->stream_timing, r->st_slots *
                                        sizeof(*r->stream_timing) );
            st = r->stream_timing + slot;
        }
        st->id = buf->s.id;
        st->average = 30.*90.;
        st->filtered_average = st->average;
        st->startup = 10;
        st->last = -st->average;
        if ( ( st->is_audio = is_audio( r, buf->s.id ) ) != 0 )
        {
            r->saw_audio = 1;
        }
        st[1].id = -1;
        st->valid = valid;
    }
    return st;
}

// update the average inter-packet time of the stream associated with 'buf'
// using a recursive low-pass filter with a 16 packet time constant.

static void update_ipt( hb_work_private_t *r, const hb_buffer_t *buf )
{
    stream_timing_t *st = id_to_st( r, buf, 1 );

    if (buf->s.renderOffset == AV_NOPTS_VALUE)
    {
        st->last += st->filtered_average;
        return;
    }

    double dt = buf->s.renderOffset - st->last;

    // Protect against spurious bad timestamps
    // timestamps should only move forward and by reasonable increments
    if ( dt > 0 && dt < 5 * 90000LL )
    {
        if( st->startup )
        {
            st->average += ( dt - st->average ) * (1./4.);
            st->startup--;
        }
        else
        {
            st->average += ( dt - st->average ) * (1./32.);
        }
        // Ignore outliers
        if (dt < 1.5 * st->average)
        {
            st->filtered_average += ( dt - st->filtered_average ) * (1./32.);
        }
    }
    st->last = buf->s.renderOffset;
    st->valid = 1;
}

// use the per-stream state associated with 'buf' to compute a new scr_offset
// such that 'buf' will follow the previous packet of this stream separated
// by the average packet time of the stream.

static void new_scr_offset( hb_work_private_t *r, hb_buffer_t *buf )
{
    stream_timing_t *st = id_to_st( r, buf, 1 );
    int64_t last;
    if ( !st->valid )
    {
        // !valid means we've not received any previous data
        // for this stream.  There is no 'last' packet time.
        // So approximate it with video's last time.
        last = r->stream_timing[0].last;
        st->valid = 1;
    }
    else
    {
        last = st->last;
    }
    int64_t nxt = last + st->filtered_average;
    r->scr_offset = buf->s.renderOffset - nxt;
    // This log is handy when you need to debug timing problems...
    //hb_log("id %x last %"PRId64" avg %g nxt %"PRId64" renderOffset %"PRId64
    //       " scr_offset %"PRId64"",
    //    buf->s.id, last, st->filtered_average, nxt,
    //    buf->s.renderOffset, r->scr_offset);
    r->scr_changes = r->demux.scr_changes;
}

static void reader_send_eof( hb_work_private_t * r )
{
    int ii;

    // send eof buffers downstream to decoders to signal we're done.
    push_buf(r, r->job->fifo_mpeg2, hb_buffer_eof_init());

    hb_audio_t *audio;
    for (ii = 0; (audio = hb_list_item(r->job->list_audio, ii)); ++ii)
    {
        if (audio->priv.fifo_in)
            push_buf(r, audio->priv.fifo_in, hb_buffer_eof_init());
    }

    hb_subtitle_t *subtitle;
    for (ii = 0; (subtitle = hb_list_item(r->job->list_subtitle, ii)); ++ii)
    {
        if (subtitle->fifo_in && subtitle->source != SRTSUB)
            push_buf(r, subtitle->fifo_in, hb_buffer_eof_init());
    }
    hb_log("reader: done. %d scr changes", r->demux.scr_changes);
}

static int reader_work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                        hb_buffer_t ** buf_out)
{
    hb_work_private_t  * r = w->private_data;
    hb_fifo_t         ** fifos;
    hb_buffer_t        * buf;
    hb_buffer_list_t     list;
    int                  ii, chapter = -1;

    hb_buffer_list_clear(&list);

    if (r->bd)
        chapter = hb_bd_chapter( r->bd );
    else if (r->dvd)
        chapter = hb_dvd_chapter( r->dvd );
    else if (r->stream)
        chapter = hb_stream_chapter( r->stream );

    if( chapter < 0 )
    {
        hb_log( "reader: end of the title reached" );
        reader_send_eof(r);
        return HB_WORK_DONE;
    }
    if( chapter > r->chapter_end )
    {
        hb_log("reader: end of chapter %d (media %d) reached at media chapter %d",
                r->job->chapter_end, r->chapter_end, chapter);
        reader_send_eof(r);
        return HB_WORK_DONE;
    }

    if (r->bd)
    {
        if( (buf = hb_bd_read( r->bd )) == NULL )
        {
            reader_send_eof(r);
            return HB_WORK_DONE;
        }
    }
    else if (r->dvd)
    {
        if( (buf = hb_dvd_read( r->dvd )) == NULL )
        {
            reader_send_eof(r);
            return HB_WORK_DONE;
        }
    }
    else if (r->stream)
    {
        if ( (buf = hb_stream_read( r->stream )) == NULL )
        {
            reader_send_eof(r);
            return HB_WORK_DONE;
        }
    }

    (hb_demux[r->title->demuxer])(buf, &list, &r->demux);

    while ((buf = hb_buffer_list_rem_head(&list)) != NULL)
    {
        fifos = GetFifoForId( r, buf->s.id );

        if (fifos && r->stream && r->start_found == 2 )
        {
            // We will inspect the timestamps of each frame in sync
            // to skip from this seek point to the timestamp we
            // want to start at.
            if (buf->s.start != AV_NOPTS_VALUE &&
                buf->s.start < r->job->pts_to_start)
            {
                r->job->pts_to_start -= buf->s.start;
            }
            else if ( buf->s.start >= r->job->pts_to_start )
            {
                r->job->pts_to_start = 0;
            }
            r->start_found = 1;
        }

        if ( fifos && ! r->saw_video && !r->job->indepth_scan )
        {
            // The first data packet with a PTS from an audio or video stream
            // that we're decoding defines 'time zero'. Discard packets until
            // we get one.
            if (buf->s.start != AV_NOPTS_VALUE &&
                buf->s.renderOffset != AV_NOPTS_VALUE &&
                 (buf->s.id == r->title->video_id ||
                  is_audio( r, buf->s.id)))
            {
                // force a new scr offset computation
                r->scr_changes = r->demux.scr_changes - 1;
                // create a stream state if we don't have one so the
                // offset will get computed correctly.
                id_to_st( r, buf, 1 );
                r->saw_video = 1;
                hb_log( "reader: first SCR %"PRId64" id 0x%x DTS %"PRId64,
                        r->demux.last_scr, buf->s.id, buf->s.renderOffset );
            }
            else
            {
                fifos = NULL;
            }
        }

        if ( r->job->indepth_scan || fifos )
        {
            if ( buf->s.renderOffset != AV_NOPTS_VALUE )
            {
                if ( r->scr_changes != r->demux.scr_changes )
                {
                    // This is the first audio or video packet after an SCR
                    // change. Compute a new scr offset that would make this
                    // packet follow the last of this stream with the
                    // correct average spacing.
                    stream_timing_t *st = id_to_st( r, buf, 0 );

                    // if this is the video stream and we don't have
                    // audio yet or this is an audio stream
                    // generate a new scr
                    if ( st->is_audio ||
                         ( st == r->stream_timing && !r->saw_audio ) )
                    {
                        new_scr_offset( r, buf );
                        r->sub_scr_set = 0;
                    }
                    else
                    {
                        // defer the scr change until we get some
                        // audio since audio has a timestamp per
                        // frame but video & subtitles don't. Clear
                        // the timestamps so the decoder will generate
                        // them from the frame durations.
                        if (is_subtitle(r, buf->s.id) &&
                            buf->s.start != AV_NOPTS_VALUE)
                        {
                            if (!r->sub_scr_set)
                            {
                                // We can't generate timestamps in the
                                // subtitle decoder as we can for
                                // audio & video.  So we need to make
                                // the closest guess that we can
                                // for the subtitles start time here.
                                int64_t last = r->stream_timing[0].last;
                                r->scr_offset = buf->s.start - last;
                                r->sub_scr_set = 1;
                            }
                        }
                        else
                        {
                            buf->s.start = AV_NOPTS_VALUE;
                            buf->s.renderOffset = AV_NOPTS_VALUE;
                        }
                    }
                }
            }
            if ( buf->s.start != AV_NOPTS_VALUE )
            {
                int64_t start = buf->s.start - r->scr_offset;

                if (!r->start_found || r->job->indepth_scan)
                {
                    UpdateState( r, start );
                }

                if (r->job->indepth_scan && r->job->pts_to_stop &&
                    start >= r->pts_to_start + r->job->pts_to_stop)
                {
                    // sync normally would terminate p-to-p
                    // but sync doesn't run during indepth scan
                    hb_log("reader: reached pts %"PRId64", exiting early", start);
                    reader_send_eof(r);
                    hb_buffer_list_close(&list);
                    return HB_WORK_DONE;
                }

                if (!r->start_found && start >= r->pts_to_start)
                {
                    // pts_to_start point found
                    r->start_found = 1;
                    if (r->stream)
                    {
                        // libav multi-threaded decoders can get into
                        // a bad state if the initial data is not
                        // decodable.  So try to improve the chances of
                        // a good start by waiting for an initial iframe
                        hb_stream_set_need_keyframe(r->stream, 1);
                        hb_buffer_close( &buf );
                        continue;
                    }
                }
                // This log is handy when you need to debug timing problems
                //hb_log("id %x scr_offset %"PRId64
                //       " start %"PRId64" --> %"PRId64"",
                //        buf->s.id, r->scr_offset, buf->s.start,
                //        buf->s.start - r->scr_offset);
                buf->s.start -= r->scr_offset;
                if ( buf->s.stop != AV_NOPTS_VALUE )
                {
                    buf->s.stop -= r->scr_offset;
                }
            }
            if ( buf->s.renderOffset != AV_NOPTS_VALUE )
            {
                // This packet is referenced to the same SCR as the last.
                // Adjust timestamp to remove the System Clock Reference
                // offset then update the average inter-packet time
                // for this stream.
                buf->s.renderOffset -= r->scr_offset;
                update_ipt( r, buf );
            }
#if 0
            // JAS: This was added to fix a rare "audio time went backward"
            // sync error I found in one sample.  But it has a bad side
            // effect on DVDs, causing frequent "adding silence" sync
            // errors. So I am disabling it.
            else
            {
                update_ipt( r, buf );
            }
#endif
        }
        buf = splice_discontinuity(r, buf);
        if( fifos && buf != NULL )
        {
            if ( !r->start_found )
            {
                hb_buffer_close( &buf );
                continue;
            }

            buf->sequence = r->sequence++;
            /* if there are mutiple output fifos, send a copy of the
             * buffer down all but the first (we have to not ship the
             * original buffer or we'll race with the thread that's
             * consuming the buffer & inject garbage into the data stream). */
            for (ii = 1; fifos[ii] != NULL; ii++)
            {
                hb_buffer_t *buf_copy = hb_buffer_init(buf->size);
                buf_copy->s = buf->s;
                memcpy(buf_copy->data, buf->data, buf->size);
                push_buf(r, fifos[ii], buf_copy);
            }
            push_buf(r, fifos[0], buf);
            buf = NULL;
        }
        else
        {
            hb_buffer_close(&buf);
        }
    }

    hb_buffer_list_close(&list);
    return HB_WORK_OK;
}

static void UpdateState( hb_work_private_t  * r, int64_t start)
{
    hb_state_t state;
    uint64_t now;
    double avg;

    now = hb_get_date();
    if( !r->st_first )
    {
        r->st_first = now;
    }

    hb_get_state2(r->job->h, &state);
#define p state.param.working
    if ( !r->job->indepth_scan )
    {
        state.state = HB_STATE_SEARCHING;
        p.progress  = (float) start / (float) r->job->pts_to_start;
    }
    else
    {
        state.state = HB_STATE_WORKING;
        p.progress  = (float) start / (float) r->duration;
    }
    if( p.progress > 1.0 )
    {
        p.progress = 1.0;
    }
    p.rate_cur = 0.0;
    p.rate_avg = 0.0;
    if (now > r->st_first)
    {
        int eta;

        avg = 1000.0 * (double)start / (now - r->st_first);
        if ( !r->job->indepth_scan )
            eta = ( r->job->pts_to_start - start ) / avg;
        else
            eta = ( r->duration - start ) / avg;
        p.hours   = eta / 3600;
        p.minutes = ( eta % 3600 ) / 60;
        p.seconds = eta % 60;
    }
    else
    {
        p.hours    = -1;
        p.minutes  = -1;
        p.seconds  = -1;
    }
#undef p

    hb_set_state( r->job->h, &state );
}
/***********************************************************************
 * GetFifoForId
 ***********************************************************************
 *
 **********************************************************************/
static hb_fifo_t ** GetFifoForId( hb_work_private_t * r, int id )
{
    hb_job_t      * job = r->job;
    hb_title_t    * title = job->title;
    hb_audio_t    * audio;
    hb_subtitle_t * subtitle;
    int             i, n;

    if( id == title->video_id )
    {
        if (job->indepth_scan && !job->frame_to_stop)
        {
            /*
             * Ditch the video here during the indepth scan until
             * we can improve the MPEG2 decode performance.
             *
             * But if we specify a stop frame, we must decode the
             * frames in order to count them.
             */
            return NULL;
        }
        else
        {
            r->fifos[0] = job->fifo_mpeg2;
            r->fifos[1] = NULL;
            return r->fifos;
        }
    }

    for( i = n = 0; i < hb_list_count( job->list_subtitle ); i++ )
    {
        subtitle =  hb_list_item( job->list_subtitle, i );
        if (id == subtitle->id)
        {
            /* pass the subtitles to be processed */
            r->fifos[n++] = subtitle->fifo_in;
        }
    }
    if ( n != 0 )
    {
        r->fifos[n] = NULL;
        return r->fifos;
    }

    if( !job->indepth_scan )
    {
        for( i = n = 0; i < hb_list_count( job->list_audio ); i++ )
        {
            audio = hb_list_item( job->list_audio, i );
            if( id == audio->id )
            {
                r->fifos[n++] = audio->priv.fifo_in;
            }
        }

        if( n != 0 )
        {
            r->fifos[n] = NULL;
            return r->fifos;
        }
    }

    return NULL;
}

static hb_buffer_list_t * get_splice_list(hb_work_private_t * r, int id)
{
    int ii;

    for (ii = 0; ii < r->splice_list_size; ii++)
    {
        if (r->splice_list[ii].id == id)
        {
            return &r->splice_list[ii].list;
        }
    }
    return NULL;
}
