/* muxcommon.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
#include "handbrake/handbrake.h"

#define MIN_BUFFERING (1024*1024*10)
#define MAX_BUFFERING (1024*1024*50)

struct hb_mux_object_s
{
    HB_MUX_COMMON;
};

typedef struct
{
    int         size;   // Size in bits
    uint32_t  * vec;
} hb_bitvec_t;

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
    int               done;
    hb_mux_object_t * m;
    double            pts;        // end time of next muxing chunk
    double            interleave; // size in 90KHz ticks of media chunks we mux
    uint32_t          max_tracks; // total number of tracks allocated
    uint32_t          ntracks;    // total number of tracks we're muxing
    hb_bitvec_t     * eof;        // bitmask of track with eof
    hb_bitvec_t     * rdy;        // bitmask of tracks ready to output
    hb_bitvec_t     * allEof;     // valid bits in eof (all tracks)
    hb_bitvec_t     * allRdy;     // valid bits in rdy (audio & video tracks)
    hb_track_t     ** track;      // tracks to mux 'max_tracks' elements
    int               buffered_size;
} hb_mux_t;

struct hb_work_private_s
{
    hb_job_t  * job;
    int         track;
    hb_mux_t  * mux;
    hb_list_t * list_work;
};


static int hb_bitvec_add_bits(hb_bitvec_t *bv, int bits)
{
    int ii;
    int words_cur = (bv->size + 31) >> 5;
    int words = (bv->size + bits + 31) >> 5;
    if (words > words_cur)
    {
        uint32_t *tmp = realloc(bv->vec, words * sizeof(uint32_t));
        if (tmp == NULL)
        {
            return -1;
        }
        for (ii = words_cur; ii < words; ii++)
            tmp[ii] = 0;
        bv->vec = tmp;
    }
    bv->size += bits;
    return 0;
}

static hb_bitvec_t* hb_bitvec_new(int size)
{
    hb_bitvec_t *bv = calloc(sizeof(hb_bitvec_t), 1);
    if (bv)
    {
        hb_bitvec_add_bits(bv, size);
    }
    return bv;
}

static void hb_bitvec_free(hb_bitvec_t **_bv)
{
    hb_bitvec_t *bv = *_bv;
    if (bv)
    {
        free(bv->vec);
    }
    free(bv);
    *_bv = NULL;
}

static void hb_bitvec_set(hb_bitvec_t *bv, int n)
{
    if (n >= bv->size)
        return; // Error.  Should never happen.

    int word = n >> 5;
    uint32_t bit = 1 << (n & 0x1F);
    bv->vec[word] |= bit;
}

static void hb_bitvec_clr(hb_bitvec_t *bv, int n)
{
    if (n >= bv->size)
        return; // Error.  Should never happen.

    int word = n >> 5;
    uint32_t bit = 1 << (n & 0x1F);
    bv->vec[word] &= ~bit;
}

static void hb_bitvec_zero(hb_bitvec_t *bv)
{
    int words = (bv->size + 31) >> 5;
    memset(bv->vec, 0, words * sizeof(uint32_t));
}

static int hb_bitvec_bit(hb_bitvec_t *bv, int n)
{
    if (n >= bv->size)
        return 0; // Error.  Should never happen.

    int word = n >> 5;
    uint32_t bit = 1 << (n & 0x1F);
    return !!(bv->vec[word] & bit);
}

static int hb_bitvec_any(hb_bitvec_t *bv)
{
    uint32_t result = 0;;
    int ii;
    int words = (bv->size + 31) >> 5;
    for (ii = 0; ii < words; ii++)
        result |= bv->vec[ii];

    return !!result;
}

static int hb_bitvec_cmp(hb_bitvec_t *bv1, hb_bitvec_t *bv2)
{
    if (bv1->size != bv2->size)
        return 0;

    int ii;
    int words = (bv1->size + 31) >> 5;
    for (ii = 0; ii < words; ii++)
        if (bv1->vec[ii] != bv2->vec[ii])
            return 0;
    return 1;
}

static int hb_bitvec_and_cmp(hb_bitvec_t *bv1, hb_bitvec_t *bv2, hb_bitvec_t *bv3)
{
    if (bv1->size != bv2->size)
        return 0;

    int ii;
    int words = (bv1->size + 31) >> 5;
    for (ii = 0; ii < words; ii++)
        if ((bv1->vec[ii] & bv2->vec[ii]) != bv3->vec[ii])
            return 0;
    return 1;
}

static int hb_bitvec_cpy(hb_bitvec_t *bv1, hb_bitvec_t *bv2)
{
    if (bv1->size < bv2->size)
    {
        int result = hb_bitvec_add_bits(bv1, bv2->size - bv1->size);
        if (result < 0)
            return result;
    }

    int words = (bv1->size + 31) >> 5;
    memcpy(bv1->vec, bv2->vec, words * sizeof(uint32_t));

    return 0;
}

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

static int add_mux_track( hb_mux_t *mux, hb_mux_data_t *mux_data,
                           int is_continuous )
{
    if ( mux->ntracks + 1 > mux->max_tracks )
    {
        int max_tracks = mux->max_tracks ? mux->max_tracks * 2 : 32;
        hb_track_t **tmp;
        tmp = realloc(mux->track, max_tracks * sizeof(hb_track_t*));
        if (tmp == NULL)
        {
            hb_error("add_mux_track: realloc failed, too many tracks (>%d)",
                     max_tracks);
            return -1;
        }
        mux->track = tmp;
        mux->max_tracks = max_tracks;
    }

    hb_track_t *track = calloc( sizeof( hb_track_t ), 1 );
    if (track)
    {
        track->mux_data = mux_data;
        track->mf.flen = 8;
        track->mf.fifo = calloc( sizeof(track->mf.fifo[0]), track->mf.flen );
    }

    if (track == NULL || track->mf.fifo == NULL)
    {
        free(track);
        return -1;
    }

    int t = mux->ntracks++;
    mux->track[t] = track;
    hb_bitvec_set(mux->allEof, t);
    if (is_continuous)
        hb_bitvec_set(mux->allRdy, t);

    return 0;
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
        hb_bitvec_cpy(mux->rdy, mux->allRdy);
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
    if ( buf->s.start >= mux->pts )
    {
        // buffer is past our next interleave point so
        // note that this track is ready to be output.
        hb_bitvec_set(mux->rdy, tk);
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

    if (buf->s.flags & HB_BUF_FLAG_EOF)
    {
        // EOF - mark this track as done
        hb_buffer_close( &buf );
        hb_bitvec_set(mux->eof, pv->track);
        hb_bitvec_set(mux->rdy, pv->track);
    }
    else if ((job->pass_id != HB_PASS_ENCODE &&
              job->pass_id != HB_PASS_ENCODE_FINAL) ||
             hb_bitvec_bit(mux->eof, pv->track))
    {
        hb_buffer_close( &buf );
    }
    else
    {
        MoveToInternalFifos( pv->track, mux, buf );
    }
    *buf_in = NULL;

    if (!hb_bitvec_and_cmp(mux->rdy, mux->allRdy, mux->allRdy) &&
        !hb_bitvec_and_cmp(mux->eof, mux->allEof, mux->allEof))
    {
        hb_unlock( mux->mutex );
        return HB_WORK_OK;
    }

    hb_bitvec_t *more;
    more = hb_bitvec_new(0);
    hb_bitvec_cpy(more, mux->rdy);
    // all tracks have at least 'interleave' ticks of data. Output
    // all that we can in 'interleave' size chunks.
    while ((hb_bitvec_and_cmp(mux->rdy, mux->allRdy, mux->allRdy) &&
            hb_bitvec_any(more) && mux->buffered_size > MIN_BUFFERING ) ||
           (hb_bitvec_cmp(mux->eof, mux->allEof)))
    {
        hb_bitvec_zero(more);
        for ( i = 0; i < mux->ntracks; ++i )
        {
            track = mux->track[i];
            OutputTrackChunk( mux, i, mux->m );
            if ( mf_full( track ) )
            {
                // If the track's fifo is still full, advance
                // the current interleave point and try again.
                hb_bitvec_cpy(mux->rdy, mux->allRdy);
                break;
            }

            // if the track is at eof or still has data that's past
            // our next interleave point then leave it marked as rdy.
            // Otherwise clear rdy.
            if (hb_bitvec_bit(mux->eof, i) &&
                (track->mf.out == track->mf.in ||
                 track->mf.fifo[(track->mf.in-1) & (track->mf.flen-1)]->s.start
                     < mux->pts + mux->interleave))
            {
                hb_bitvec_clr(mux->rdy, i);
            }
            if ( track->mf.out != track->mf.in )
            {
                hb_bitvec_set(more, i);
            }
        }

        // if all the tracks are at eof we're just purging their
        // remaining data -- keep going until all internal fifos are empty.
        if (hb_bitvec_cmp(mux->eof, mux->allEof))
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
                *w->done = 1;
                hb_unlock( mux->mutex );
                hb_bitvec_free(&more);
                return HB_WORK_DONE;
            }
        }
        mux->pts += mux->interleave;
    }
    hb_bitvec_free(&more);

    hb_unlock( mux->mutex );
    return HB_WORK_OK;
}

static void muxFlush(hb_mux_t * mux)
{
    int ii, done = 0;

    while (!done)
    {
        done = 1;
        for (ii = 0; ii < mux->ntracks; ii++)
        {
            OutputTrackChunk(mux, ii, mux->m);
            if (mux->track[ii]->mf.out != mux->track[ii]->mf.in)
            {
                // track buffer is not empty
                done = 0;
            }
        }
        mux->pts += mux->interleave;
    }
}

static void muxClose( hb_work_object_t * muxer )
{
    hb_work_private_t * pv = muxer->private_data;
    if (pv == NULL)
    {
        // Not initialized
        return;
    }

    hb_mux_t          * mux = pv->mux;

    if (mux == NULL)
    {
        free(pv);
        muxer->private_data = NULL;
        return;
    }

    hb_job_t          * job = pv->job;
    hb_track_t        * track;
    hb_work_object_t  * w;
    int                 i;

    hb_lock( mux->mutex );
    muxFlush(mux);

    // Update state before closing muxer.  Closing the muxer
    // may initiate optimization which can take a while and
    // we want the muxing state to be visible while this is
    // happening.
    if( job->pass_id == HB_PASS_ENCODE ||
        job->pass_id == HB_PASS_ENCODE_FINAL )
    {
        /* Update the UI */
        hb_state_t state;
        hb_get_state2(job->h, &state);
        state.state = HB_STATE_MUXING;
        state.param.muxing.progress = 0;
        hb_set_state(job->h, &state);
    }

    if( mux->m )
    {
        mux->m->end( mux->m );
        free( mux->m );
    }

    // we're all done muxing -- print final stats and cleanup.
    if( job->pass_id == HB_PASS_ENCODE ||
        job->pass_id == HB_PASS_ENCODE_FINAL )
    {
        hb_stat_t sb;
        uint64_t bytes_total, frames_total;

        if (!hb_stat(job->file, &sb))
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
                if (!i && job->vquality <= HB_INVALID_VIDEO_QUALITY)
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

    for (i = 0; i < mux->ntracks; ++i)
    {
        hb_buffer_t * b;
        track = mux->track[i];
        while ( (b = mf_pull( mux, i )) != NULL )
        {
            hb_buffer_close( &b );
        }
        free(track->mf.fifo);
        free(track);
    }
    free(mux->track);
    hb_unlock( mux->mutex );
    hb_lock_close( &mux->mutex );
    hb_bitvec_free(&mux->eof);
    hb_bitvec_free(&mux->rdy);
    hb_bitvec_free(&mux->allEof);
    hb_bitvec_free(&mux->allRdy);
    free( mux );

    // Close mux work threads
    while ((w = hb_list_item(pv->list_work, 0)))
    {
        hb_list_rem(pv->list_work, w);
        if (w->thread != NULL)
        {
            hb_thread_close( &w->thread );
        }
        free(w->private_data);
        free(w);
    }
    hb_list_close(&pv->list_work);
    free( pv );
    muxer->private_data = NULL;
}

static int muxInit( hb_work_object_t * muxer, hb_job_t * job )
{
    muxer->private_data = calloc( sizeof(hb_work_private_t ), 1);
    if (muxer->private_data == NULL)
    {
        return -1;
    }
    hb_work_private_t *pv = muxer->private_data;

    hb_mux_t *mux = calloc( sizeof( hb_mux_t ), 1 );
    if (mux == NULL)
    {
        goto fail;
    }

    mux->mutex = hb_lock_init();
    if (mux->mutex == NULL)
    {
        goto fail;
    }

    pv->mux = mux;
    pv->job = job;
    pv->track = mux->ntracks;

    /* Get a real muxer */
    if( job->pass_id == HB_PASS_ENCODE || job->pass_id == HB_PASS_ENCODE_FINAL )
    {
        switch( job->mux )
        {
            case HB_MUX_AV_MP4:
            case HB_MUX_AV_MKV:
            case HB_MUX_AV_WEBM:
                mux->m = hb_mux_avformat_init( job );
                break;
            default:
                hb_error("No muxer selected, exiting");
                goto fail;
        }
    }

    pv->list_work = hb_list_init();

    // The bit vectors must be allocated before hb_thread_init for the
    // audio and subtitle muxer jobs below.
    int bit_vec_size = 1 + hb_list_count(job->list_audio) +
                           hb_list_count(job->list_subtitle);
    mux->rdy = hb_bitvec_new(bit_vec_size);
    mux->eof = hb_bitvec_new(bit_vec_size);
    mux->allRdy = hb_bitvec_new(bit_vec_size);
    mux->allEof = hb_bitvec_new(bit_vec_size);

    if (mux->rdy    == NULL || mux->eof    == NULL ||
        mux->allRdy == NULL || mux->allEof == NULL)
    {
        goto fail;
    }

    // set up to interleave track data in blocks of 1 video frame time.
    // (the best case for buffering and playout latency). The container-
    // specific muxers can reblock this into bigger chunks if necessary.
    mux->interleave = 90000. * (double)job->vrate.den / job->vrate.num;
    mux->pts = mux->interleave;

    if( job->pass_id == HB_PASS_ENCODE || job->pass_id == HB_PASS_ENCODE_FINAL )
    {
        /* Create file, write headers */
        if( mux->m )
        {
            mux->m->init( mux->m );
        }
    }

    /* Initialize the work objects that will receive fifo data */
    muxer->fifo_in = job->fifo_mpeg4;
    if (add_mux_track(mux, job->mux_data, 1))
    {
        goto fail;
    }

    for (int i = 0; i < hb_list_count(job->list_audio); i++)
    {
        int err = 0;
        hb_audio_t  *audio = hb_list_item( job->list_audio, i );

        hb_work_object_t *w = hb_get_work(job->h, WORK_MUX);
        w->private_data = calloc(sizeof(hb_work_private_t), 1);
        if (w->private_data)
        {
            w->private_data->job = job;
            w->private_data->mux = mux;
            w->private_data->track = mux->ntracks;
            w->fifo_in = audio->priv.fifo_out;
            err = add_mux_track(mux, audio->priv.mux_data, 1);
            hb_list_add(pv->list_work, w);
        }
        if (w->private_data == NULL || err == -1)
        {
            goto fail;
        }
    }

    for (int i = 0; i < hb_list_count(job->list_subtitle); i++)
    {
        int err = 0;
        hb_subtitle_t  *subtitle = hb_list_item( job->list_subtitle, i );

        if (subtitle->config.dest != PASSTHRUSUB)
            continue;

        hb_work_object_t *w = hb_get_work(job->h, WORK_MUX);
        w->private_data = calloc(sizeof(hb_work_private_t), 1);
        if (w->private_data)
        {
            w->private_data->job = job;
            w->private_data->mux = mux;
            w->private_data->track = mux->ntracks;
            w->fifo_in = subtitle->fifo_out;
            err = add_mux_track(mux, subtitle->mux_data, 0);
            hb_list_add(pv->list_work, w);
        }
        if (w->private_data == NULL || err == -1)
        {
            goto fail;
        }
    }

    /* Launch processing threads */
    for (int i = 0; i < hb_list_count(pv->list_work); i++)
    {
        hb_work_object_t *w = hb_list_item(pv->list_work, i);
        w->done = muxer->done;
        w->thread = hb_thread_init(w->name, hb_work_loop, w, HB_LOW_PRIORITY);
    }
    return 0;

fail:
    if (pv->mux == NULL)
    {
        hb_lock_close(&mux->mutex);
        free(mux);
    }
    *job->done_error = HB_ERROR_INIT;
    *job->die = 1;
    return -1;
}

hb_work_object_t hb_muxer =
{
    WORK_MUX,
    "Muxer",
    muxInit,
    muxWork,
    muxClose
};

