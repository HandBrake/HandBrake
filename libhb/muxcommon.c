/* muxcommon.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
#include "hb.h"

#define MIN_BUFFERING (1024*1024*10)
#define MAX_BUFFERING (1024*1024*50)

struct hb_mux_object_s
{
    HB_MUX_COMMON;
};

typedef struct
{
    hb_buffer_t **fifo;
    uint32_t    in;     // number of bufs put into fifo
    uint32_t    out;    // number of bufs taken out of fifo
    uint32_t    flen;   // fifo length (must be power of two)
} mux_fifo_t;

typedef struct
{
    hb_mux_data_t * mux_data;
    uint64_t        frames;
    uint64_t        bytes;
    mux_fifo_t      mf;
    int             buffered_size;
} hb_track_t;

typedef struct
{
    hb_lock_t       * mutex;
    int               ref;
    int               done;
    hb_mux_object_t * m;
    double            pts;        // end time of next muxing chunk
    double            interleave; // size in 90KHz ticks of media chunks we mux
    uint32_t          ntracks;    // total number of tracks we're muxing
    uint32_t          eof;        // bitmask of track with eof
    uint32_t          rdy;        // bitmask of tracks ready to output
    uint32_t          allEof;     // valid bits in eof (all tracks)
    uint32_t          allRdy;     // valid bits in rdy (audio & video tracks)
    hb_track_t      * track[32];  // array of tracks to mux ('ntrack' elements)
                                  // NOTE- this array could be dynamically 
                                  // allocated but the eof & rdy logic has to 
                                  // be changed to handle more than 32 tracks 
                                  // anyway so we keep it simple and fast.
    int               buffered_size;
} hb_mux_t;

struct hb_work_private_s
{
    hb_job_t * job;
    int        track;
    hb_mux_t * mux;
};

// The muxer handles two different kinds of media: Video and audio tracks
// are continuous: once they start they generate continuous, consecutive
// sequence of bufs until they end. The muxer will time align all continuous
// media tracks so that their data will be well interleaved in the output file.
// (Smooth, low latency playback with minimal player buffering requires that
// data that's going to be presented close together in time also be close
// together in the output file). Since HB's audio and video encoders run at
// different speeds, the time-aligning involves buffering *all* the continuous
// media tracks until a frame with a timestamp beyond the current alignment
// point arrives on the slowest fifo (usually the video encoder).
//
// The other kind of media, subtitles, close-captions, vobsubs and
// similar tracks, are intermittent. They generate frames sporadically or on
// human time scales (seconds) rather than near the video frame rate (milliseconds).
// If intermittent sources were treated like continuous sources huge sections of
// audio and video would get buffered waiting for the next subtitle to show up.
// To keep this from happening the muxer doesn't wait for intermittent tracks
// (essentially it assumes that they will always go through the HB processing
// pipeline faster than the associated video). They are still time aligned and
// interleaved at the appropriate point in the output file.

// This routine adds another track for the muxer to process. The media input
// stream will be read from HandBrake fifo 'fifo'. Buffers read from that
// stream will be time-aligned with all the other media streams then passed
// to the container-specific 'mux' routine with argument 'mux_data' (see
// routine OutputTrackChunk). 'is_continuous' must be 1 for an audio or video
// track and 0 otherwise (see above).

static void add_mux_track( hb_mux_t *mux, hb_mux_data_t *mux_data,
                           int is_continuous )
{
    int max_tracks = sizeof(mux->track) / sizeof(*(mux->track));
    if ( mux->ntracks >= max_tracks )
    {
        hb_error( "add_mux_track: too many tracks (>%d)", max_tracks );
        return;
    }

    hb_track_t *track = calloc( sizeof( hb_track_t ), 1 );
    track->mux_data = mux_data;
    track->mf.flen = 8;
    track->mf.fifo = calloc( sizeof(track->mf.fifo[0]), track->mf.flen );

    int t = mux->ntracks++;
    mux->track[t] = track;
    mux->allEof |= 1 << t;
    mux->allRdy |= is_continuous << t;
}

static int mf_full( hb_track_t * track )
{
    if ( track->buffered_size > MAX_BUFFERING )
        return 1;

    return 0;
}

static void mf_push( hb_mux_t * mux, int tk, hb_buffer_t *buf )
{
    hb_track_t * track = mux->track[tk];
    uint32_t mask = track->mf.flen - 1;
    uint32_t in = track->mf.in;

    hb_buffer_reduce( buf, buf->size );
    if ( track->buffered_size > MAX_BUFFERING )
    {
        mux->rdy = mux->allRdy;
    }
    if ( ( ( in + 1 ) & mask ) == ( track->mf.out & mask ) )
    {
        // fifo is full - expand it to double the current size.
        // This is a bit tricky because when we change the size
        // it changes the modulus (mask) used to convert the in
        // and out counters to fifo indices. Since existing items
        // will be referenced at a new location after the expand
        // we can't just realloc the fifo. If there were
        // hundreds of fifo entries it would be worth it to have code
        // for each of the four possible before/after configurations
        // but these fifos are small so we just allocate a new chunk
        // of memory then do element by element copies using the old &
        // new masks then free the old fifo's memory..
        track->mf.flen *= 2;
        uint32_t nmask = track->mf.flen - 1;
        hb_buffer_t **nfifo = malloc( track->mf.flen * sizeof(*nfifo) );
        int indx = track->mf.out;
        while ( indx != track->mf.in )
        {
            nfifo[indx & nmask] = track->mf.fifo[indx & mask];
            ++indx;
        }
        free( track->mf.fifo );
        track->mf.fifo = nfifo;
        mask = nmask;
    }
    track->mf.fifo[in & mask] = buf;
    track->mf.in = in + 1;
    track->buffered_size += buf->size;
    mux->buffered_size += buf->size;
}

static hb_buffer_t *mf_pull( hb_mux_t * mux, int tk )
{
    hb_track_t *track =mux->track[tk];
    hb_buffer_t *b = NULL;
    if ( track->mf.out != track->mf.in )
    {
        // the fifo isn't empty
        b = track->mf.fifo[track->mf.out & (track->mf.flen - 1)];
        ++track->mf.out;

        track->buffered_size -= b->size;
        mux->buffered_size -= b->size;
    }
    return b;
}

static hb_buffer_t *mf_peek( hb_track_t *track )
{
    return track->mf.out == track->mf.in ?
                NULL : track->mf.fifo[track->mf.out & (track->mf.flen - 1)];
}

static void MoveToInternalFifos( int tk, hb_mux_t *mux, hb_buffer_t * buf )
{
    // move all the buffers on the track's fifo to our internal
    // fifo so that (a) we don't deadlock in the reader and
    // (b) we can control how data from multiple tracks is
    // interleaved in the output file.
    mf_push( mux, tk, buf );
    if ( buf->s.stop >= mux->pts )
    {
        // buffer is past our next interleave point so
        // note that this track is ready to be output.
        mux->rdy |= ( 1 << tk );
    }
}

static void OutputTrackChunk( hb_mux_t *mux, int tk, hb_mux_object_t *m )
{
    hb_track_t *track = mux->track[tk];
    hb_buffer_t *buf;

    while ( ( buf = mf_peek( track ) ) != NULL && buf->s.start < mux->pts )
    {
        buf = mf_pull( mux, tk );
        track->frames += 1;
        track->bytes  += buf->size;
        m->mux( m, track->mux_data, buf );
    }
}

static int muxWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                     hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t    * job = pv->job;
    hb_mux_t    * mux = pv->mux;
    hb_track_t  * track;
    int           i;
    hb_buffer_t * buf = *buf_in;

    hb_lock( mux->mutex );
    if ( mux->done )
    {
        hb_unlock( mux->mutex );
        return HB_WORK_DONE;
    }

    if ( buf->size <= 0 )
    {
        // EOF - mark this track as done
        hb_buffer_close( &buf );
        mux->eof |= ( 1 << pv->track );
        mux->rdy |= ( 1 << pv->track );
    }
    else if ( ( job->pass != 0 && job->pass != 2 ) ||
              ( mux->eof & (1 << pv->track) ) )
    {
        hb_buffer_close( &buf );
    }
    else
    {
        MoveToInternalFifos( pv->track, mux, buf );
    }
    *buf_in = NULL;

    if ( ( mux->rdy & mux->allRdy ) != mux->allRdy )
    {
        hb_unlock( mux->mutex );
        return HB_WORK_OK;
    }

    int more = mux->rdy;
    // all tracks have at least 'interleave' ticks of data. Output
    // all that we can in 'interleave' size chunks.
    while ( (( mux->rdy & mux->allRdy ) == mux->allRdy && 
            more && mux->buffered_size > MIN_BUFFERING ) ||
            ( mux->eof == mux->allEof ) ) 
    {
        more = 0;
        for ( i = 0; i < mux->ntracks; ++i )
        {
            track = mux->track[i];
            OutputTrackChunk( mux, i, mux->m );
            if ( mf_full( track ) )
            {
                // If the track's fifo is still full, advance
                // the currint interleave point and try again.
                mux->rdy = mux->allRdy;
                break;
            }

            // if the track is at eof or still has data that's past
            // our next interleave point then leave it marked as rdy.
            // Otherwise clear rdy.
            if ( ( mux->eof & (1 << i) ) == 0 &&
                 ( track->mf.out == track->mf.in ||
                   track->mf.fifo[(track->mf.in-1) & (track->mf.flen-1)]->s.stop
                     < mux->pts + mux->interleave ) )
            {
                mux->rdy &=~ ( 1 << i );
            }
            if ( track->mf.out != track->mf.in )
            {
                more |= ( 1 << i );
            }
        }

        // if all the tracks are at eof we're just purging their
        // remaining data -- keep going until all internal fifos are empty.
        if ( mux->eof == mux->allEof )
        {
            for ( i = 0; i < mux->ntracks; ++i )
            {
                if ( mux->track[i]->mf.out != mux->track[i]->mf.in )
                {
                    break;
                }
            }
            if ( i >= mux->ntracks )
            {
                mux->done = 1;
                hb_unlock( mux->mutex );
                return HB_WORK_DONE;
            }
        }
        mux->pts += mux->interleave;
    }
    hb_unlock( mux->mutex );
    return HB_WORK_OK;
}

void muxClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_mux_t    * mux = pv->mux;
    hb_job_t    * job = pv->job;
    hb_track_t  * track;
    int           i;

    hb_lock( mux->mutex );
    if ( --mux->ref == 0 )
    {
        // Update state before closing muxer.  Closing the muxer
        // may initiate optimization which can take a while and
        // we want the muxing state to be visible while this is
        // happening.
        if( job->pass == 0 || job->pass == 2 )
        {
            /* Update the UI */
            hb_state_t state;
            state.state = HB_STATE_MUXING;
            state.param.muxing.progress = 0;
            hb_set_state( job->h, &state );
        }

        if( mux->m )
        {
            mux->m->end( mux->m );
            free( mux->m );
        }

        // we're all done muxing -- print final stats and cleanup.
        if( job->pass == 0 || job->pass == 2 )
        {
            struct stat sb;
            uint64_t bytes_total, frames_total;

            if( !stat( job->file, &sb ) )
            {
                hb_deep_log( 2, "mux: file size, %"PRId64" bytes", (uint64_t) sb.st_size );

                bytes_total  = 0;
                frames_total = 0;
                for( i = 0; i < mux->ntracks; ++i )
                {
                    track = mux->track[i];
                    hb_log( "mux: track %d, %"PRId64" frames, %"PRId64" bytes, %.2f kbps, fifo %d",
                            i, track->frames, track->bytes,
                            90000.0 * track->bytes / mux->pts / 125,
                            track->mf.flen );
                    if( !i && job->vquality < 0 )
                    {
                        /* Video */
                        hb_deep_log( 2, "mux: video bitrate error, %+"PRId64" bytes",
                                (int64_t)(track->bytes - mux->pts * job->vbitrate * 125 / 90000) );
                    }
                    bytes_total  += track->bytes;
                    frames_total += track->frames;
                }

                if( bytes_total && frames_total )
                {
                    hb_deep_log( 2, "mux: overhead, %.2f bytes per frame",
                            (float) ( sb.st_size - bytes_total ) /
                            frames_total );
                }
            }
        }
    
        for( i = 0; i < mux->ntracks; ++i )
        {
            hb_buffer_t * b;
            track = mux->track[i];
            while ( (b = mf_pull( mux, i )) != NULL )
            {
                hb_buffer_close( &b );
            }
            if( track->mux_data )
            {
                free( track->mux_data );
                free( track->mf.fifo );
            }
            free( track );
        }
        hb_unlock( mux->mutex );
        hb_lock_close( &mux->mutex );
        free( mux );
    }
    else
    {
        hb_unlock( mux->mutex );
    }
    free( pv );
    w->private_data = NULL;
}

static void mux_loop( void * _w )
{
    hb_work_object_t  * w = _w;
    hb_work_private_t * pv = w->private_data;
    hb_job_t          * job = pv->job;
    hb_buffer_t       * buf_in;

    while ( !*job->die && w->status != HB_WORK_DONE )
    {
        buf_in = hb_fifo_get_wait( w->fifo_in );
        if ( pv->mux->done )
            break;
        if ( buf_in == NULL )
            continue;
        if ( *job->die )
        {
            if( buf_in )
            {
                hb_buffer_close( &buf_in );
            }
            break;
        }

        w->status = w->work( w, &buf_in, NULL );
        if( buf_in )
        {
            hb_buffer_close( &buf_in );
        }
    }
}

hb_work_object_t * hb_muxer_init( hb_job_t * job )
{
    int           i;
    hb_mux_t    * mux = calloc( sizeof( hb_mux_t ), 1 );
    hb_work_object_t  * w;
    hb_work_object_t  * muxer;

    mux->mutex = hb_lock_init();

    // set up to interleave track data in blocks of 1 video frame time.
    // (the best case for buffering and playout latency). The container-
    // specific muxers can reblock this into bigger chunks if necessary.
    mux->interleave = 90000. * (double)job->vrate_base / (double)job->vrate;
    mux->pts = mux->interleave;

    /* Get a real muxer */
    if( job->pass == 0 || job->pass == 2)
    {
        switch( job->mux )
        {
        case HB_MUX_MP4:
            mux->m = hb_mux_mp4_init( job );
            break;
        case HB_MUX_MKV:
            mux->m = hb_mux_mkv_init( job );
            break;
        default:
            hb_error( "No muxer selected, exiting" );
            *job->die = 1;
            return NULL;
        }
        /* Create file, write headers */
        if( mux->m )
        {
            mux->m->init( mux->m );
        }
    }

    /* Initialize the work objects that will receive fifo data */

    muxer = hb_get_work( WORK_MUX );
    muxer->private_data = calloc( sizeof( hb_work_private_t ), 1 );
    muxer->private_data->job = job;
    muxer->private_data->mux = mux;
    mux->ref++;
    muxer->private_data->track = mux->ntracks;
    muxer->fifo_in = job->fifo_mpeg4;
    add_mux_track( mux, job->mux_data, 1 );
    muxer->done = &muxer->private_data->mux->done;

    for( i = 0; i < hb_list_count( job->list_audio ); i++ )
    {
        hb_audio_t  *audio = hb_list_item( job->list_audio, i );

        w = hb_get_work( WORK_MUX );
        w->private_data = calloc( sizeof( hb_work_private_t ), 1 );
        w->private_data->job = job;
        w->private_data->mux = mux;
        mux->ref++;
        w->private_data->track = mux->ntracks;
        w->fifo_in = audio->priv.fifo_out;
        add_mux_track( mux, audio->priv.mux_data, 1 );
        w->done = &job->done;
        hb_list_add( job->list_work, w );
        w->thread = hb_thread_init( w->name, mux_loop, w, HB_NORMAL_PRIORITY );
    }

    for( i = 0; i < hb_list_count( job->list_subtitle ); i++ )
    {
        hb_subtitle_t  *subtitle = hb_list_item( job->list_subtitle, i );

        if (subtitle->config.dest != PASSTHRUSUB)
            continue;

        w = hb_get_work( WORK_MUX );
        w->private_data = calloc( sizeof( hb_work_private_t ), 1 );
        w->private_data->job = job;
        w->private_data->mux = mux;
        mux->ref++;
        w->private_data->track = mux->ntracks;
        w->fifo_in = subtitle->fifo_out;
        add_mux_track( mux, subtitle->mux_data, 0 );
        w->done = &job->done;
        hb_list_add( job->list_work, w );
        w->thread = hb_thread_init( w->name, mux_loop, w, HB_NORMAL_PRIORITY );
    }
    return muxer;
}

// muxInit does nothing because the muxer has a special initializer
// that takes care of initializing all muxer work objects
static int muxInit( hb_work_object_t * w, hb_job_t * job )
{
    return 0;
}

hb_work_object_t hb_muxer =
{
    WORK_MUX,
    "Muxer",
    muxInit,
    muxWork,
    muxClose
};

