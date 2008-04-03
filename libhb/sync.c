/* $Id: sync.c,v 1.38 2005/04/14 21:57:58 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "samplerate.h"
#include "ffmpeg/avcodec.h"

#ifdef INT64_MIN
#undef INT64_MIN /* Because it isn't defined correctly in Zeta */
#endif
#define INT64_MIN (-9223372036854775807LL-1)

#define AC3_SAMPLES_PER_FRAME 1536

typedef struct
{
    hb_audio_t * audio;

    int64_t      next_start;    /* start time of next output frame */
    int64_t      next_pts;      /* start time of next input frame */
    int64_t      start_silence; /* if we're inserting silence, the time we started */
    int64_t      first_drop;    /* PTS of first 'went backwards' frame dropped */
    int          drop_count;    /* count of 'time went backwards' drops */
    int          inserting_silence;

    /* Raw */
    SRC_STATE  * state;
    SRC_DATA     data;

    /* AC-3 */
    int          ac3_size;
    uint8_t    * ac3_buf;

} hb_sync_audio_t;

struct hb_work_private_s
{
    hb_job_t * job;
    int        done;

    /* Video */
    hb_subtitle_t * subtitle;
    int64_t pts_offset;
    int64_t next_start;         /* start time of next output frame */
    int64_t next_pts;           /* start time of next input frame */
    int64_t first_drop;         /* PTS of first 'went backwards' frame dropped */
    int drop_count;             /* count of 'time went backwards' drops */
    int video_sequence;
    int count_frames;
    int count_frames_max;
    hb_buffer_t * cur; /* The next picture to process */

    /* Audio */
    hb_sync_audio_t sync_audio[8];

    /* Statistics */
    uint64_t st_counts[4];
    uint64_t st_dates[4];
    uint64_t st_first;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void InitAudio( hb_work_object_t * w, int i );
static int  SyncVideo( hb_work_object_t * w );
static void SyncAudio( hb_work_object_t * w, int i );
static int  NeedSilence( hb_work_object_t * w, hb_audio_t *, int i );
static void InsertSilence( hb_work_object_t * w, int i, int64_t d );
static void UpdateState( hb_work_object_t * w );

/***********************************************************************
 * hb_work_sync_init
 ***********************************************************************
 * Initialize the work object
 **********************************************************************/
int syncInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_title_t       * title = job->title;
    hb_chapter_t     * chapter;
    int                i;
    uint64_t           duration;
    hb_work_private_t * pv;

    pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job            = job;
    pv->pts_offset     = INT64_MIN;
    pv->count_frames   = 0;

    /* Calculate how many video frames we are expecting */
    duration = 0;
    for( i = job->chapter_start; i <= job->chapter_end; i++ )
    {
        chapter   = hb_list_item( title->list_chapter, i - 1 );
        duration += chapter->duration;
    }
    duration += 90000;
        /* 1 second safety so we're sure we won't miss anything */
    pv->count_frames_max = duration * job->vrate / job->vrate_base / 90000;

    hb_log( "sync: expecting %d video frames", pv->count_frames_max );

    /* Initialize libsamplerate for every audio track we have */
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        InitAudio( w, i );
    }

    /* Get subtitle info, if any */
    pv->subtitle = hb_list_item( title->list_subtitle, 0 );

    pv->video_sequence = 0;

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void syncClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t          * job   = pv->job;
    hb_title_t        * title = job->title;
    hb_audio_t        * audio = NULL;

    int i;

    if( pv->cur ) hb_buffer_close( &pv->cur );

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        if ( pv->sync_audio[i].start_silence )
        {
            hb_log( "sync: added %d ms of silence to audio %d",
                    (int)((pv->sync_audio[i].next_pts -
                              pv->sync_audio[i].start_silence) / 90), i );
        }

        audio = hb_list_item( title->list_audio, i );
        if( audio->config.out.codec == HB_ACODEC_AC3 )
        {
            free( pv->sync_audio[i].ac3_buf );
        }
        else
        {
            src_delete( pv->sync_audio[i].state );
        }
    }

    free( pv );
    w->private_data = NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 * The root routine of this work abject
 *
 * The way this works is that we are syncing the audio to the PTS of
 * the last video that we processed. That's why we skip the audio sync
 * if we haven't got a valid PTS from the video yet.
 *
 **********************************************************************/
int syncWork( hb_work_object_t * w, hb_buffer_t ** unused1,
              hb_buffer_t ** unused2 )
{
    hb_work_private_t * pv = w->private_data;
    int i;

    /* If we ever got a video frame, handle audio now */
    if( pv->pts_offset != INT64_MIN )
    {
        for( i = 0; i < hb_list_count( pv->job->title->list_audio ); i++ )
        {
            SyncAudio( w, i );
        }
    }

    /* Handle video */
    return SyncVideo( w );
}

hb_work_object_t hb_sync =
{
    WORK_SYNC,
    "Synchronization",
    syncInit,
    syncWork,
    syncClose
};

static void InitAudio( hb_work_object_t * w, int i )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t        * job   = pv->job;
    hb_title_t      * title = job->title;
    hb_sync_audio_t * sync;

    sync        = &pv->sync_audio[i];
    sync->audio = hb_list_item( title->list_audio, i );

    if( sync->audio->config.out.codec == HB_ACODEC_AC3 )
    {
        /* Have a silent AC-3 frame ready in case we have to fill a
           gap */
        AVCodec        * codec;
        AVCodecContext * c;
        short          * zeros;

        codec = avcodec_find_encoder( CODEC_ID_AC3 );
        c     = avcodec_alloc_context();

        c->bit_rate    = sync->audio->config.in.bitrate;
        c->sample_rate = sync->audio->config.in.samplerate;
        c->channels    = HB_INPUT_CH_LAYOUT_GET_DISCRETE_COUNT( sync->audio->config.in.channel_layout );

        if( avcodec_open( c, codec ) < 0 )
        {
            hb_log( "sync: avcodec_open failed" );
            return;
        }

        zeros          = calloc( AC3_SAMPLES_PER_FRAME *
                                 sizeof( short ) * c->channels, 1 );
        sync->ac3_size = sync->audio->config.in.bitrate * AC3_SAMPLES_PER_FRAME /
                             sync->audio->config.in.samplerate / 8;
        sync->ac3_buf  = malloc( sync->ac3_size );

        if( avcodec_encode_audio( c, sync->ac3_buf, sync->ac3_size,
                                  zeros ) != sync->ac3_size )
        {
            hb_log( "sync: avcodec_encode_audio failed" );
        }

        free( zeros );
        avcodec_close( c );
        av_free( c );
    }
    else
    {
        /* Initialize libsamplerate */
        int error;
        sync->state             = src_new( SRC_LINEAR, HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(sync->audio->config.out.mixdown), &error );
        sync->data.end_of_input = 0;
    }
}

/***********************************************************************
 * SyncVideo
 ***********************************************************************
 *
 **********************************************************************/
static int SyncVideo( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * cur, * next, * sub = NULL;
    hb_job_t * job = pv->job;

    if( pv->done )
    {
        return HB_WORK_DONE;
    }

    if( hb_thread_has_exited( job->reader ) &&
        !hb_fifo_size( job->fifo_mpeg2 ) &&
        !hb_fifo_size( job->fifo_raw ) )
    {
        /* All video data has been processed already, we won't get
           more */
        hb_log( "sync: got %d frames, %d expected",
                pv->count_frames, pv->count_frames_max );
        pv->done = 1;

        hb_buffer_t * buf_tmp;

       // Drop an empty buffer into our output to ensure that things
       // get flushed all the way out.
        buf_tmp = hb_buffer_init(0); // Empty end buffer
        hb_fifo_push( job->fifo_sync, buf_tmp );

        return HB_WORK_DONE;
    }

    if( !pv->cur && !( pv->cur = hb_fifo_get( job->fifo_raw ) ) )
    {
        /* We haven't even got a frame yet */
        return HB_WORK_OK;
    }
    cur = pv->cur;

    /* At this point we have a frame to process. Let's check
        1) if we will be able to push into the fifo ahead
        2) if the next frame is there already, since we need it to
           compute the duration of the current frame*/
    while( !hb_fifo_is_full( job->fifo_sync ) &&
           ( next = hb_fifo_see( job->fifo_raw ) ) )
    {
        hb_buffer_t * buf_tmp;

        if( pv->pts_offset == INT64_MIN )
        {
            /* This is our first frame */
            pv->pts_offset = 0;
            if ( cur->start != 0 )
            {
                /*
                 * The first pts from a dvd should always be zero but
                 * can be non-zero with a transport or program stream since
                 * we're not guaranteed to start on an IDR frame. If we get
                 * a non-zero initial PTS extend its duration so it behaves
                 * as if it started at zero so that our audio timing will
                 * be in sync.
                 */
                hb_log( "sync: first pts is %lld", cur->start );
                cur->start = 0;
            }
        }

        /*
         * since the first frame is always 0 and the upstream reader code
         * is taking care of adjusting for pts discontinuities, we just have
         * to deal with the next frame's start being in the past. This can
         * happen when the PTS is adjusted after data loss but video frame
         * reordering causes some frames with the old clock to appear after
         * the clock change. This creates frames that overlap in time which
         * looks to us like time going backward. The downstream muxing code
         * can deal with overlaps of up to a frame time but anything larger
         * we handle by dropping frames here.
         */
        if ( (int64_t)( next->start - pv->next_pts ) <= 0 )
        {
            if ( pv->first_drop == 0 )
            {
                pv->first_drop = next->start;
            }
            ++pv->drop_count;
            buf_tmp = hb_fifo_get( job->fifo_raw );
            hb_buffer_close( &buf_tmp );
            continue;
        }
        if ( pv->first_drop )
        {
            hb_log( "sync: video time didn't advance - dropped %d frames "
                    "(delta %d ms, current %lld, next %lld)",
                    pv->drop_count, (int)( pv->next_pts - pv->first_drop ) / 90,
                    pv->next_pts, pv->first_drop );
            pv->first_drop = 0;
            pv->drop_count = 0;
        }

        /*
         * Track the video sequence number localy so that we can sync the audio
         * to it using the sequence number as well as the PTS.
         */
        pv->video_sequence = cur->sequence;

        /* Look for a subtitle for this frame */
        if( pv->subtitle )
        {
            hb_buffer_t * sub2;
            while( ( sub = hb_fifo_see( pv->subtitle->fifo_raw ) ) )
            {
                /* If two subtitles overlap, make the first one stop
                   when the second one starts */
                sub2 = hb_fifo_see2( pv->subtitle->fifo_raw );
                if( sub2 && sub->stop > sub2->start )
                    sub->stop = sub2->start;

                // hb_log("0x%x: video seq: %lld  subtitle sequence: %lld",
                //       sub, cur->sequence, sub->sequence);

                if( sub->sequence > cur->sequence )
                {
                    /*
                     * The video is behind where we are, so wait until
                     * it catches up to the same reader point on the
                     * DVD. Then our PTS should be in the same region
                     * as the video.
                     */
                    sub = NULL;
                    break;
                }

                if( sub->stop > cur->start ) {
                    /*
                     * The stop time is in the future, so fall through
                     * and we'll deal with it in the next block of
                     * code.
                     */
                    break;
                }

                /*
                 * The subtitle is older than this picture, trash it
                 */
                sub = hb_fifo_get( pv->subtitle->fifo_raw );
                hb_buffer_close( &sub );
            }

            /*
             * There is a valid subtitle, is it time to display it?
             */
            if( sub )
            {
                if( sub->stop > sub->start)
                {
                    /*
                     * Normal subtitle which ends after it starts, check to
                     * see that the current video is between the start and end.
                     */
                    if( cur->start > sub->start &&
                        cur->start < sub->stop )
                    {
                        /*
                         * We should be playing this, so leave the
                         * subtitle in place.
                         *
                         * fall through to display
                         */
                        if( ( sub->stop - sub->start ) < ( 3 * 90000 ) )
                        {
                            /*
                             * Subtitle is on for less than three seconds, extend
                             * the time that it is displayed to make it easier
                             * to read. Make it 3 seconds or until the next
                             * subtitle is displayed.
                             *
                             * This is in response to Indochine which only
                             * displays subs for 1 second - too fast to read.
                             */
                            sub->stop = sub->start + ( 3 * 90000 );

                            sub2 = hb_fifo_see2( pv->subtitle->fifo_raw );

                            if( sub2 && sub->stop > sub2->start )
                            {
                                sub->stop = sub2->start;
                            }
                        }
                    }
                    else
                    {
                        /*
                         * Defer until the play point is within the subtitle
                         */
                        sub = NULL;
                    }
                }
                else
                {
                    /*
                     * The end of the subtitle is less than the start, this is a
                     * sign of a PTS discontinuity.
                     */
                    if( sub->start > cur->start )
                    {
                        /*
                         * we haven't reached the start time yet, or
                         * we have jumped backwards after having
                         * already started this subtitle.
                         */
                        if( cur->start < sub->stop )
                        {
                            /*
                             * We have jumped backwards and so should
                             * continue displaying this subtitle.
                             *
                             * fall through to display.
                             */
                        }
                        else
                        {
                            /*
                             * Defer until the play point is within the subtitle
                             */
                            sub = NULL;
                        }
                    } else {
                        /*
                         * Play this subtitle as the start is greater than our
                         * video point.
                         *
                         * fall through to display/
                         */
                    }
                }
            }
        }

        /*
         * Adjust the pts of the current frame so that it's contiguous
         * with the previous frame. The start time of the current frame
         * has to be the end time of the previous frame and the stop
         * time has to be the start of the next frame.  We don't
         * make any adjustments to the source timestamps other than removing
         * the clock offsets (which also removes pts discontinuities).
         * This means we automatically encode at the source's frame rate.
         * MP2 uses an implicit duration (frames end when the next frame
         * starts) but more advanced containers like MP4 use an explicit
         * duration. Since we're looking ahead one frame we set the
         * explicit stop time from the start time of the next frame.
         */
        buf_tmp = cur;
        pv->cur = cur = hb_fifo_get( job->fifo_raw );
        pv->next_pts = next->start;
        int64_t duration = next->start - buf_tmp->start;
        if ( duration <= 0 )
        {
            hb_log( "sync: invalid video duration %lld, start %lld, next %lld",
                    duration, buf_tmp->start, next->start );
        }
        buf_tmp->start = pv->next_start;
        pv->next_start += duration;
        buf_tmp->stop = pv->next_start;

        /* If we have a subtitle for this picture, copy it */
        /* FIXME: we should avoid this memcpy */
        if( sub )
        {
            buf_tmp->sub         = hb_buffer_init( sub->size );
            buf_tmp->sub->x      = sub->x;
            buf_tmp->sub->y      = sub->y;
            buf_tmp->sub->width  = sub->width;
            buf_tmp->sub->height = sub->height;
            memcpy( buf_tmp->sub->data, sub->data, sub->size );
        }

        /* Push the frame to the renderer */
        hb_fifo_push( job->fifo_sync, buf_tmp );

        /* Update UI */
        UpdateState( w );

        /* Make sure we won't get more frames then expected */
        if( pv->count_frames >= pv->count_frames_max * 2)
        {
            hb_log( "sync: got too many frames (%d), exiting early", pv->count_frames );
            pv->done = 1;

           // Drop an empty buffer into our output to ensure that things
           // get flushed all the way out.
           buf_tmp = hb_buffer_init(0); // Empty end buffer
           hb_fifo_push( job->fifo_sync, buf_tmp );

            break;
        }
    }

    return HB_WORK_OK;
}

static void OutputAudioFrame( hb_job_t *job, hb_audio_t *audio, hb_buffer_t *buf,
                              hb_sync_audio_t *sync, hb_fifo_t *fifo, int i )
{
    int64_t start = sync->next_start;
    int64_t duration = buf->stop - buf->start;
    if (duration <= 0 ||
        duration > ( 90000 * AC3_SAMPLES_PER_FRAME ) / audio->config.out.samplerate )
    {
        hb_log("sync: audio %d weird duration %lld, start %lld, stop %lld, next %lld",
               i, duration, buf->start, buf->stop, sync->next_pts);
        if ( duration <= 0 )
        {
            duration = ( 90000 * AC3_SAMPLES_PER_FRAME ) / audio->config.out.samplerate;
            buf->stop = buf->start + duration;
        }
    }
    sync->next_pts += duration;

    if( /* audio->rate == job->arate || This should work but doesn't */
        audio->config.out.codec == HB_ACODEC_AC3 ||
        audio->config.out.codec == HB_ACODEC_DCA )
    {
        /*
         * If we don't have to do sample rate conversion or this audio is AC3
         * pass-thru just send the input buffer downstream after adjusting
         * its timestamps to make the output stream continuous.
         */
    }
    else
    {
        /* Not pass-thru - do sample rate conversion */
        int count_in, count_out;
        hb_buffer_t * buf_raw = buf;
        int channel_count = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->config.out.mixdown) *
                            sizeof( float );

        count_in  = buf_raw->size / channel_count;
        count_out = ( buf_raw->stop - buf_raw->start ) * audio->config.out.samplerate / 90000;

        sync->data.input_frames = count_in;
        sync->data.output_frames = count_out;
        sync->data.src_ratio = (double)count_out / (double)count_in;

        buf = hb_buffer_init( count_out * channel_count );
        sync->data.data_in  = (float *) buf_raw->data;
        sync->data.data_out = (float *) buf->data;
        if( src_process( sync->state, &sync->data ) )
        {
            /* XXX If this happens, we're screwed */
            hb_log( "sync: audio %d src_process failed", i );
        }
        hb_buffer_close( &buf_raw );

        buf->size = sync->data.output_frames_gen * channel_count;
    }
    buf->start = start;
    buf->stop  = start + duration;
    buf->frametype = HB_FRAME_AUDIO;
    sync->next_start = start + duration;
    hb_fifo_push( fifo, buf );
}

/***********************************************************************
 * SyncAudio
 ***********************************************************************
 *
 **********************************************************************/
static void SyncAudio( hb_work_object_t * w, int i )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t        * job = pv->job;
    hb_sync_audio_t * sync = &pv->sync_audio[i];
    hb_audio_t      * audio = sync->audio;
    hb_buffer_t     * buf;
    hb_fifo_t       * fifo;
    int               rate;

    if( audio->config.out.codec == HB_ACODEC_AC3 )
    {
        fifo = audio->priv.fifo_out;
        rate = audio->config.in.samplerate;
    }
    else
    {
        fifo = audio->priv.fifo_sync;
        rate = audio->config.out.samplerate;
    }

    while( !hb_fifo_is_full( fifo ) && ( buf = hb_fifo_see( audio->priv.fifo_raw ) ) )
    {
        if ( (int64_t)( buf->start - sync->next_pts ) < 0 )
        {
            /*
             * audio time went backwards by more than a frame time (this can
             * happen when we reset the PTS because of lost data).
             * Discard data that's in the past.
             */
            if ( sync->first_drop == 0 )
            {
                sync->first_drop = buf->start;
            }
            ++sync->drop_count;
            buf = hb_fifo_get( audio->priv.fifo_raw );
            hb_buffer_close( &buf );
            continue;
        }
        if ( sync->first_drop )
        {
            hb_log( "sync: audio %d time went backwards %d ms, dropped %d frames "
                    "(next %lld, current %lld)", i,
                    (int)( sync->next_pts - sync->first_drop ) / 90,
                    sync->drop_count, sync->first_drop, sync->next_pts );
            sync->first_drop = 0;
            sync->drop_count = 0;
        }

        if ( sync->inserting_silence && (int64_t)(buf->start - sync->next_pts) > 0 )
        {
            /*
             * if we're within one frame time of the amount of silence
             * we need, insert just what we need otherwise insert a frame time.
             */
            int64_t framedur = buf->stop - buf->start;
            if ( buf->start - sync->next_pts <= framedur )
            {
                InsertSilence( w, i, buf->start - sync->next_pts );
                sync->inserting_silence = 0;
            }
            else
            {
                InsertSilence( w, i, framedur );
            }
            continue;
        }
        if ( buf->start - sync->next_pts >= (90 * 100) )
        {
            /*
             * there's a gap of at least 100ms between the last
             * frame we processed & the next. Fill it with silence.
             */
            if ( ! sync->inserting_silence )
            {
                hb_log( "sync: adding %d ms of silence to audio %d"
                        "  start %lld, next %lld",
                        (int)((buf->start - sync->next_pts) / 90),
                        i, buf->start, sync->next_pts );
                sync->inserting_silence = 1;
            }
            InsertSilence( w, i, buf->stop - buf->start );
            continue;
        }

        /*
         * When we get here we've taken care of all the dups and gaps in the
         * audio stream and are ready to inject the next input frame into
         * the output stream.
         */
        buf = hb_fifo_get( audio->priv.fifo_raw );
        OutputAudioFrame( job, audio, buf, sync, fifo, i );
    }

    if( NeedSilence( w, audio, i ) )
    {
        InsertSilence( w, i, (90000 * AC3_SAMPLES_PER_FRAME) / sync->audio->config.out.samplerate );
    }
}

static int NeedSilence( hb_work_object_t * w, hb_audio_t * audio, int i )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t * job = pv->job;
    hb_sync_audio_t * sync = &pv->sync_audio[i];

    if( hb_fifo_size( audio->priv.fifo_in ) ||
        hb_fifo_size( audio->priv.fifo_raw ) ||
        hb_fifo_size( audio->priv.fifo_sync ) ||
        hb_fifo_size( audio->priv.fifo_out ) )
    {
        /* We have some audio, we are fine */
        return 0;
    }

    /* No audio left in fifos */

    if( hb_thread_has_exited( job->reader ) )
    {
        /* We might miss some audio to complete encoding and muxing
           the video track */
        if ( sync->start_silence == 0 )
        {
            hb_log("sync: reader has exited, adding silence to audio %d", i);
            sync->start_silence = sync->next_pts;
        }
        return 1;
    }
    return 0;
}

static void InsertSilence( hb_work_object_t * w, int i, int64_t duration )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t        *job = pv->job;
    hb_sync_audio_t *sync = &pv->sync_audio[i];
    hb_buffer_t     *buf;

    if( sync->audio->config.out.codec == HB_ACODEC_AC3 )
    {
        buf        = hb_buffer_init( sync->ac3_size );
        buf->start = sync->next_pts;
        buf->stop  = buf->start + duration;
        memcpy( buf->data, sync->ac3_buf, buf->size );
        OutputAudioFrame( job, sync->audio, buf, sync, sync->audio->priv.fifo_out, i );
    }
    else
    {
        buf = hb_buffer_init( duration * sizeof( float ) *
                    HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(sync->audio->config.out.mixdown) );
        buf->start = sync->next_pts;
        buf->stop  = buf->start + duration;
        memset( buf->data, 0, buf->size );
        OutputAudioFrame( job, sync->audio, buf, sync, sync->audio->priv.fifo_sync, i );
    }
}

static void UpdateState( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_state_t state;

    if( !pv->count_frames )
    {
        pv->st_first = hb_get_date();
    }
    pv->count_frames++;

    if( hb_get_date() > pv->st_dates[3] + 1000 )
    {
        memmove( &pv->st_dates[0], &pv->st_dates[1],
                 3 * sizeof( uint64_t ) );
        memmove( &pv->st_counts[0], &pv->st_counts[1],
                 3 * sizeof( uint64_t ) );
        pv->st_dates[3]  = hb_get_date();
        pv->st_counts[3] = pv->count_frames;
    }

#define p state.param.working
    state.state = HB_STATE_WORKING;
    p.progress  = (float) pv->count_frames / (float) pv->count_frames_max;
    if( p.progress > 1.0 )
    {
        p.progress = 1.0;
    }
    p.rate_cur   = 1000.0 *
        (float) ( pv->st_counts[3] - pv->st_counts[0] ) /
        (float) ( pv->st_dates[3] - pv->st_dates[0] );
    if( hb_get_date() > pv->st_first + 4000 )
    {
        int eta;
        p.rate_avg = 1000.0 * (float) pv->st_counts[3] /
            (float) ( pv->st_dates[3] - pv->st_first );
        eta = (float) ( pv->count_frames_max - pv->st_counts[3] ) /
            p.rate_avg;
        p.hours   = eta / 3600;
        p.minutes = ( eta % 3600 ) / 60;
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

    hb_set_state( pv->job->h, &state );
}
