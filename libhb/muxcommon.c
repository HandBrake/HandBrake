/* $Id: muxcommon.c,v 1.23 2005/03/30 17:27:19 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

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
    hb_fifo_t     * fifo;
    hb_mux_data_t * mux_data;
    uint64_t        frames;
    uint64_t        bytes;
    mux_fifo_t      mf;
} hb_track_t;

typedef struct
{
    hb_job_t    *job;
    double      pts;        // end time of next muxing chunk
    double      interleave; // size (in 90KHz ticks) of media chunks we mux
    uint32_t    ntracks;    // total number of tracks we're muxing
    uint32_t    eof;        // bitmask of track with eof
    uint32_t    rdy;        // bitmask of tracks ready to output
    uint32_t    allEof;     // valid bits in eof (all tracks)
    uint32_t    allRdy;     // valid bits in rdy (audio & video tracks)
    hb_track_t  *track[32]; // array of tracks to mux ('ntrack' elements)
                            // NOTE- this array could be dynamically allocated
                            // but the eof & rdy logic has to be changed to
                            // handle more than 32 tracks anyway so we keep
                            // it simple and fast.
} hb_mux_t;

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

static void add_mux_track( hb_mux_t *mux, hb_fifo_t *fifo, hb_mux_data_t *mux_data,
                           int is_continuous )
{
    int max_tracks = sizeof(mux->track) / sizeof(*(mux->track));
    if ( mux->ntracks >= max_tracks )
    {
        hb_error( "add_mux_track: too many tracks (>%d)", max_tracks );
        return;
    }

    hb_track_t *track = calloc( sizeof( hb_track_t ), 1 );
    track->fifo = fifo;
    track->mux_data = mux_data;
    track->mf.flen = 8;
    track->mf.fifo = calloc( sizeof(track->mf.fifo[0]), track->mf.flen );

    int t = mux->ntracks++;
    mux->track[t] = track;
    mux->allEof |= 1 << t;
    mux->allRdy |= is_continuous << t;
}

static void mf_push( hb_track_t *track, hb_buffer_t *buf )
{
    uint32_t mask = track->mf.flen - 1;
    uint32_t in = track->mf.in;
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
}

static hb_buffer_t *mf_pull( hb_track_t *track )
{
    hb_buffer_t *b = NULL;
    if ( track->mf.out != track->mf.in )
    {
        // the fifo isn't empty
        b = track->mf.fifo[track->mf.out & (track->mf.flen - 1)];
        ++track->mf.out;
    }
    return b;
}

static void MoveToInternalFifos( hb_mux_t *mux )
{
    int i;
    int discard = mux->job->pass != 0 && mux->job->pass != 2;

    for( i = 0; i < mux->ntracks; ++i )
    {
        if ( ( mux->eof & (1 << i) ) == 0 )
        {
            hb_track_t *track = mux->track[i];
            hb_buffer_t *buf;
            
            // move all the buffers on the track's fifo to our internal
            // fifo so that (a) we don't deadlock in the reader and
            // (b) we can control how data from multiple tracks is
            // interleaved in the output file.
            while ( ( buf = hb_fifo_get( track->fifo ) ) )
            {
                if ( buf->size <= 0 )
                {
                    // EOF - mark this track as done
                    hb_buffer_close( &buf );
                    mux->eof |= ( 1 << i );
                    mux->rdy |= ( 1 << i );
                    continue;
                }
                if ( discard )
                {
                    hb_buffer_close( &buf );
                    continue;
                }
                mf_push( track, buf );
                if ( buf->stop >= mux->pts )
                {
                    // buffer is past our next interleave point so
                    // note that this track is ready to be output.
                    mux->rdy |= ( 1 << i );
                }
            }
        }
    }
}

static void OutputTrackChunk( hb_mux_t *mux, hb_track_t *track, hb_mux_object_t *m )
{
    hb_buffer_t *buf;

    while ( ( buf = mf_pull( track ) ) != NULL )
    {
        m->mux( m, track->mux_data, buf );
        track->frames += 1;
        track->bytes  += buf->size;

        uint64_t pts = buf->stop;
        hb_buffer_close( &buf );
        if ( pts >= mux->pts )
        {
            break;
        }
    }
}

static void MuxerFunc( void * _mux )
{
    hb_mux_t    * mux = _mux;
    hb_job_t    * job = mux->job;
    hb_title_t  * title = job->title;
    hb_track_t  * track;
    int           i;
    hb_mux_object_t * m = NULL;

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
            case HB_MUX_PSP:
			case HB_MUX_IPOD:
                m = hb_mux_mp4_init( job );
                break;
            case HB_MUX_AVI:
                m = hb_mux_avi_init( job );
                break;
            case HB_MUX_OGM:
                m = hb_mux_ogm_init( job );
                break;
            case HB_MUX_MKV:
                m = hb_mux_mkv_init( job );
        }
        /* Create file, write headers */
        m->init( m );
    }

    /* Build list of fifos we're interested in */

    add_mux_track( mux, job->fifo_mpeg4, job->mux_data, 1 );

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        hb_audio_t  *audio = hb_list_item( title->list_audio, i );
        add_mux_track( mux, audio->priv.fifo_out, audio->priv.mux_data, 1 );
    }


    // The following 'while' is the main muxing loop.

	int thread_sleep_interval = 50;
	while( !*job->die )
    {
        MoveToInternalFifos( mux );
        if (  mux->rdy != mux->allRdy )
        {
            hb_snooze( thread_sleep_interval );
            continue;
        }

        // all tracks have at least 'interleave' ticks of data. Output
        // all that we can in 'interleave' size chunks.
        while ( mux->rdy == mux->allRdy )
        {
            for ( i = 0; i < mux->ntracks; ++i )
            {
                track = mux->track[i];
                OutputTrackChunk( mux, track, m );

                // if the track is at eof or still has data that's past
                // our next interleave point then leave it marked as rdy.
                // Otherwise clear rdy.
                if ( ( mux->eof & (1 << i) ) == 0 &&
                     ( track->mf.out == track->mf.in ||
                       track->mf.fifo[(track->mf.in-1) & (track->mf.flen-1)]->stop
                         < mux->pts + mux->interleave ) )
                {
                    mux->rdy &=~ ( 1 << i );
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
                    goto finished;
                }
            }
            mux->pts += mux->interleave;
        }
    }

    // we're all done muxing -- print final stats and cleanup.
finished:
    if( job->pass == 0 || job->pass == 2 )
    {
        struct stat sb;
        uint64_t bytes_total, frames_total;

#define p state.param.muxing
        /* Update the UI */
        hb_state_t state;
        state.state   = HB_STATE_MUXING;
		p.progress = 0;
        hb_set_state( job->h, &state );
#undef p
        m->end( m );

        if( !stat( job->file, &sb ) )
        {
            hb_deep_log( 2, "mux: file size, %lld bytes", (uint64_t) sb.st_size );

            bytes_total  = 0;
            frames_total = 0;
            for( i = 0; i < mux->ntracks; ++i )
            {
                track = mux->track[i];
                hb_log( "mux: track %d, %lld frames, %lld bytes, %.2f kbps, fifo %d",
                        i, track->frames, track->bytes,
                        90000.0 * track->bytes / mux->pts / 125,
                        track->mf.flen );
                if( !i && ( job->vquality < 0.0 || job->vquality > 1.0 ) )
                {
                    /* Video */
                    hb_deep_log( 2, "mux: video bitrate error, %+lld bytes",
                            track->bytes - mux->pts * job->vbitrate *
                            125 / 90000 );
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

    free( m );

    for( i = 0; i < mux->ntracks; ++i )
    {
        track = mux->track[i];
        if( track->mux_data )
        {
            free( track->mux_data );
            free( track->mf.fifo );
        }
        free( track );
    }

    free( mux );
}

hb_thread_t * hb_muxer_init( hb_job_t * job )
{
    hb_mux_t * mux = calloc( sizeof( hb_mux_t ), 1 );
    mux->job = job;
    return hb_thread_init( "muxer", MuxerFunc, mux,
                           HB_NORMAL_PRIORITY );
}
