/* $Id: reader.c,v 1.21 2005/11/25 15:05:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

typedef struct
{
    int64_t last;   // last timestamp seen on this stream
    double average; // average time between packets
    int id;         // stream id
} stream_timing_t;

typedef struct
{
    hb_job_t     * job;
    hb_title_t   * title;
    volatile int * die;

    hb_dvd_t     * dvd;
    hb_stream_t  * stream;

    stream_timing_t *stream_timing;
    int64_t        scr_offset;
    hb_psdemux_t   demux;
    int            scr_changes;
    uint           sequence;
    int            saw_video;
    int            st_slots;            // size (in slots) of stream_timing array
} hb_reader_t;

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void        ReaderFunc( void * );
static hb_fifo_t ** GetFifoForId( hb_job_t * job, int id );

/***********************************************************************
 * hb_reader_init
 ***********************************************************************
 *
 **********************************************************************/
hb_thread_t * hb_reader_init( hb_job_t * job )
{
    hb_reader_t * r;

    r = calloc( sizeof( hb_reader_t ), 1 );

    r->job   = job;
    r->title = job->title;
    r->die   = job->die;
    r->sequence = 0;

    r->st_slots = 4;
    r->stream_timing = calloc( sizeof(stream_timing_t), r->st_slots );
    r->stream_timing[0].id = r->title->video_id;
    r->stream_timing[0].average = 90000. * (double)job->vrate_base /
                                           (double)job->vrate;
    r->stream_timing[0].last = -r->stream_timing[0].average;
    r->stream_timing[1].id = -1;

    return hb_thread_init( "reader", ReaderFunc, r,
                           HB_NORMAL_PRIORITY );
}

static void push_buf( const hb_reader_t *r, hb_fifo_t *fifo, hb_buffer_t *buf )
{
    while( !*r->die && !r->job->done && hb_fifo_is_full( fifo ) )
    {
        /*
         * Loop until the incoming fifo is ready to receive
         * this buffer.
         */
        hb_snooze( 50 );
    }
    hb_fifo_push( fifo, buf );
}

// The MPEG STD (Standard Target Decoder) essentially requires that we keep
// per-stream timing so that when there's a timing discontinuity we can
// seemlessly join packets on either side of the discontinuity. This join
// requires that we know the timestamp of the previous packet and the
// average inter-packet time (since we position the new packet at the end
// of the previous packet). The next four routines keep track of this
// per-stream timing.

// find the per-stream timing state for 'buf'

static stream_timing_t *find_st( hb_reader_t *r, const hb_buffer_t *buf )
{
    stream_timing_t *st = r->stream_timing;
    for ( ; st->id != -1; ++st )
    {
        if ( st->id == buf->id )
            return st;
    }
    return NULL;
}

// find or create the per-stream timing state for 'buf'

static stream_timing_t *id_to_st( hb_reader_t *r, const hb_buffer_t *buf )
{
    stream_timing_t *st = r->stream_timing;
    while ( st->id != buf->id && st->id != -1)
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
        st->id = buf->id;
        st->average = 30.*90.;
        st->last = buf->renderOffset - st->average;

        st[1].id = -1;
    }
    return st;
}

// update the average inter-packet time of the stream associated with 'buf'
// using a recursive low-pass filter with a 16 packet time constant.

static void update_ipt( hb_reader_t *r, const hb_buffer_t *buf )
{
    stream_timing_t *st = id_to_st( r, buf );
    double dt = buf->renderOffset - st->last;
    st->average += ( dt - st->average ) * (1./16.);
    st->last = buf->renderOffset;
}

// use the per-stream state associated with 'buf' to compute a new scr_offset
// such that 'buf' will follow the previous packet of this stream separated
// by the average packet time of the stream.

static void new_scr_offset( hb_reader_t *r, hb_buffer_t *buf )
{
    stream_timing_t *st = id_to_st( r, buf );
    int64_t nxt = st->last + st->average;
    r->scr_offset = buf->renderOffset - nxt;
    buf->renderOffset = nxt;
    r->scr_changes = r->demux.scr_changes;
    st->last = buf->renderOffset;
}

/***********************************************************************
 * ReaderFunc
 ***********************************************************************
 *
 **********************************************************************/
static void ReaderFunc( void * _r )
{
    hb_reader_t  * r = _r;
    hb_fifo_t   ** fifos;
    hb_buffer_t  * buf;
    hb_list_t    * list;
    int            n;
    int            chapter = -1;
    int            chapter_end = r->job->chapter_end;

    if( !( r->dvd = hb_dvd_init( r->title->dvd ) ) )
    {
        if ( !( r->stream = hb_stream_open( r->title->dvd, r->title ) ) )
        {
          return;
        }
    }

    if (r->dvd)
    {
      /*
       * XXX this code is a temporary hack that should go away if/when
       *     chapter merging goes away in libhb/dvd.c
       * map the start and end chapter numbers to on-media chapter
       * numbers since chapter merging could cause the handbrake numbers
       * to diverge from the media numbers and, if our chapter_end is after
       * a media chapter that got merged, we'll stop ripping too early.
       */
      int start = r->job->chapter_start;
      hb_chapter_t * chap = hb_list_item( r->title->list_chapter, chapter_end - 1 );

      chapter_end = chap->index;
      if (start > 1)
      {
         chap = hb_list_item( r->title->list_chapter, start - 1 );
         start = chap->index;
      }
      /* end chapter mapping XXX */

      if( !hb_dvd_start( r->dvd, r->title->index, start ) )
      {
          hb_dvd_close( &r->dvd );
          return;
      }
    }

    list  = hb_list_init();
    hb_buffer_t *ps = hb_buffer_init( HB_DVD_READ_BUFFER_SIZE );
    r->demux.flaky_clock = r->title->flaky_clock;

    while( !*r->die && !r->job->done )
    {
        if (r->dvd)
          chapter = hb_dvd_chapter( r->dvd );
        else if (r->stream)
          chapter = 1;

        if( chapter < 0 )
        {
            hb_log( "reader: end of the title reached" );
            break;
        }
        if( chapter > chapter_end )
        {
            hb_log( "reader: end of chapter %d (media %d) reached at media chapter %d",
                    r->job->chapter_end, chapter_end, chapter );
            break;
        }

        if (r->dvd)
        {
          if( !hb_dvd_read( r->dvd, ps ) )
          {
              break;
          }
        }
        else if (r->stream)
        {
          if ( !hb_stream_read( r->stream, ps ) )
          {
            break;
          }
        }

        if( r->job->indepth_scan )
        {
            /*
             * Need to update the progress during a subtitle scan
             */
            hb_state_t state;

#define p state.param.working

            state.state = HB_STATE_WORKING;
            p.progress = (double)chapter / (double)r->job->chapter_end;
            if( p.progress > 1.0 )
            {
                p.progress = 1.0;
            }
            p.rate_avg = 0.0;
            p.hours    = -1;
            p.minutes  = -1;
            p.seconds  = -1;
            hb_set_state( r->job->h, &state );
        }

        if ( r->title->demuxer == HB_NULL_DEMUXER )
        {
            hb_demux_null( ps, list, &r->demux );
        }
        else
        {
            hb_demux_ps( ps, list, &r->demux );
        }

        while( ( buf = hb_list_item( list, 0 ) ) )
        {
            hb_list_rem( list, buf );
            fifos = GetFifoForId( r->job, buf->id );

            if ( ! r->saw_video )
            {
                /* The first video packet defines 'time zero' so discard
                   data until we get a video packet with a PTS & DTS */
                if ( buf->id == r->title->video_id && buf->start != -1 &&
                     buf->renderOffset != -1 )
                {
                    r->saw_video = 1;
                    hb_log( "reader: first SCR %lld", r->demux.last_scr );
                }
                else
                {
                    fifos = NULL;
                }
            }
            if( fifos )
            {
                if ( buf->renderOffset != -1 )
                {
                    if ( r->scr_changes == r->demux.scr_changes )
                    {
                        // This packet is referenced to the same SCR as the last.
                        // Adjust timestamp to remove the System Clock Reference
                        // offset then update the average inter-packet time
                        // for this stream.
                        buf->renderOffset -= r->scr_offset;
                        update_ipt( r, buf );
                    }
                    else
                    {
                        // This is the first audio or video packet after an SCR
                        // change. Compute a new scr offset that would make this
                        // packet follow the last of this stream with the correct
                        // average spacing.
                        if ( find_st( r, buf ) )
                        {
                            new_scr_offset( r, buf );
                        }
                        else
                        {
                            // we got a new scr at the same time as the first
                            // packet of a stream we've never seen before. We
                            // have no idea what the timing should be so toss
                            // this buffer & wait for a stream we've already seen.
                            hb_buffer_close( &buf );
                            continue;
                        }
                    }
                }
                if ( buf->start != -1 )
                    buf->start -= r->scr_offset;

                buf->sequence = r->sequence++;
                /* if there are mutiple output fifos, send a copy of the
                 * buffer down all but the first (we have to not ship the
                 * original buffer or we'll race with the thread that's
                 * consuming the buffer & inject garbage into the data stream). */
                for( n = 1; fifos[n] != NULL; n++)
                {
                    hb_buffer_t *buf_copy = hb_buffer_init( buf->size );
                    hb_buffer_copy_settings( buf_copy, buf );
                    memcpy( buf_copy->data, buf->data, buf->size );
                    push_buf( r, fifos[n], buf_copy );
                }
                push_buf( r, fifos[0], buf );
            }
            else
            {
                hb_buffer_close( &buf );
            }
        }
    }

    // send empty buffers downstream to video & audio decoders to signal we're done.
    push_buf( r, r->job->fifo_mpeg2, hb_buffer_init(0) );

    hb_audio_t *audio;
    for( n = 0; ( audio = hb_list_item( r->job->title->list_audio, n ) ); ++n )
    {
        if ( audio->priv.fifo_in )
            push_buf( r, audio->priv.fifo_in, hb_buffer_init(0) );
    }

    hb_list_empty( &list );
    hb_buffer_close( &ps );
    if (r->dvd)
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

    hb_log( "reader: done. %d scr changes", r->demux.scr_changes );
    if ( r->demux.dts_drops )
    {
        hb_log( "reader: %d drops because DTS out of range", r->demux.dts_drops );
    }

    free( r );
    _r = NULL;
}

/***********************************************************************
 * GetFifoForId
 ***********************************************************************
 *
 **********************************************************************/
static hb_fifo_t ** GetFifoForId( hb_job_t * job, int id )
{
    hb_title_t    * title = job->title;
    hb_audio_t    * audio;
    hb_subtitle_t * subtitle;
    int             i, n;
    static hb_fifo_t * fifos[8];

    memset(fifos, 0, sizeof(fifos));

    if( id == title->video_id )
    {
        if( job->indepth_scan )
        {
            /*
             * Ditch the video here during the indepth scan until
             * we can improve the MPEG2 decode performance.
             */
            return NULL;
        }
        else
        {
            fifos[0] = job->fifo_mpeg2;
            return fifos;
        }
    }

    if( job->indepth_scan ) {
        /*
         * Count the occurances of the subtitles, don't actually
         * return any to encode unless we are looking fro forced
         * subtitles in which case we need to look in the sub picture
         * to see if it has the forced flag enabled.
         */
        for (i=0; i < hb_list_count(title->list_subtitle); i++) {
            subtitle =  hb_list_item( title->list_subtitle, i);
            if (id == subtitle->id) {
                /*
                 * A hit, count it.
                 */
                subtitle->hits++;
                if( job->subtitle_force )
                {

                    fifos[0] = subtitle->fifo_in;
                    return fifos;
                }
                break;
            }
        }
    } else {
        if( ( subtitle = hb_list_item( title->list_subtitle, 0 ) ) &&
            id == subtitle->id )
        {
            fifos[0] = subtitle->fifo_in;
            return fifos;
        }
    }
    if( !job->indepth_scan )
    {
        n = 0;
        for( i = 0; i < hb_list_count( title->list_audio ); i++ )
        {
            audio = hb_list_item( title->list_audio, i );
            if( id == audio->id )
            {
                fifos[n++] = audio->priv.fifo_in;
            }
        }

        if( n != 0 )
        {
            return fifos;
        }
    }

    return NULL;
}

