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
    int64_t      count_frames;

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
    int64_t pts_offset_old;
    int64_t next_start;
    int64_t count_frames;
    int64_t count_frames_max;
    int64_t video_sequence;
    hb_buffer_t * cur; /* The next picture to process */

    /* Audio */
    hb_sync_audio_t sync_audio[8];

    /* Flags */
    int discontinuity;

    /* Statistics */
    uint64_t st_counts[4];
    uint64_t st_dates[4];
    uint64_t st_first;

    /* Throttle message flags */
    int   trashing_audio;
    int   inserting_silence;
    int   way_out_of_sync;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void InitAudio( hb_work_object_t * w, int i );
static int  SyncVideo( hb_work_object_t * w );
static void SyncAudio( hb_work_object_t * w, int i );
static int  NeedSilence( hb_work_object_t * w, hb_audio_t * );
static void InsertSilence( hb_work_object_t * w, int i );
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
    pv->pts_offset_old = INT64_MIN;
    pv->count_frames   = 0;

    pv->discontinuity = 0;

    pv->trashing_audio = 0;
    pv->inserting_silence = 0;
    pv->way_out_of_sync = 0;

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

    hb_log( "sync: expecting %lld video frames", pv->count_frames_max );

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

    int i;

    if( pv->cur ) hb_buffer_close( &pv->cur );

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        if( job->acodec & HB_ACODEC_AC3 ||
            job->audio_mixdowns[i] == HB_AMIXDOWN_AC3 )
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

    if( job->acodec & HB_ACODEC_AC3 ||
        job->audio_mixdowns[i] == HB_AMIXDOWN_AC3 )
    {
        /* Have a silent AC-3 frame ready in case we have to fill a
           gap */
        AVCodec        * codec;
        AVCodecContext * c;
        short          * zeros;

        codec = avcodec_find_encoder( CODEC_ID_AC3 );
        c     = avcodec_alloc_context();

        c->bit_rate    = sync->audio->bitrate;
        c->sample_rate = sync->audio->rate;
        c->channels    = 2;

        if( avcodec_open( c, codec ) < 0 )
        {
            hb_log( "sync: avcodec_open failed" );
            return;
        }

        zeros          = calloc( AC3_SAMPLES_PER_FRAME *
                                 sizeof( short ) * c->channels, 1 );
        sync->ac3_size = sync->audio->bitrate * AC3_SAMPLES_PER_FRAME /
                             sync->audio->rate / 8;
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
        sync->state             = src_new( SRC_LINEAR, HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(sync->audio->amixdown), &error );
        sync->data.end_of_input = 0;
    }
}



#define PTS_DISCONTINUITY_TOLERANCE 90000

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
    int64_t pts_expected;
    int chap_break;

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
        hb_log( "sync: got %lld frames, %lld expected",
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
           know whether we'll have to repeat the current frame or not */
    while( !hb_fifo_is_full( job->fifo_sync ) &&
           ( next = hb_fifo_see( job->fifo_raw ) ) )
    {
        hb_buffer_t * buf_tmp;

        if( pv->pts_offset == INT64_MIN )
        {
            /* This is our first frame */
            hb_log( "sync: first pts is %lld", cur->start );
            pv->pts_offset = cur->start;
        }

        /*
         * Track the video sequence number localy so that we can sync the audio
         * to it using the sequence number as well as the PTS.
         */
        pv->video_sequence = cur->sequence;

        /* Check for PTS jumps over 0.5 second */
        if( next->start < cur->start - PTS_DISCONTINUITY_TOLERANCE ||
            next->start > cur->start + PTS_DISCONTINUITY_TOLERANCE )
        {
	    hb_log( "Sync: Video PTS discontinuity %s (current buffer start=%lld, next buffer start=%lld)",
                    pv->discontinuity ? "second" : "first", cur->start, next->start );

            /*
             * Do we need to trash the subtitle, is it from the next->start period
             * or is it from our old position. If the latter then trash it.
             */
            if( pv->subtitle )
            {
                while( ( sub = hb_fifo_see( pv->subtitle->fifo_raw ) ) )
                {
                    if( ( sub->start > ( cur->start - PTS_DISCONTINUITY_TOLERANCE ) ) &&
                        ( sub->start < ( cur->start + PTS_DISCONTINUITY_TOLERANCE ) ) )
                    {
                        /*
                         * The subtitle is from our current time region which we are
                         * jumping from. So trash it as we are about to jump backwards
                         * or forwards and don't want it blocking the subtitle fifo.
                         */
                        hb_log("Trashing subtitle 0x%x due to PTS discontinuity", sub);
                        sub = hb_fifo_get( pv->subtitle->fifo_raw );
                        hb_buffer_close( &sub );
                    } else {
                        break;
                    }
                }
            }

            /* Trash current picture */
            /* Also, make sure we don't trash a chapter break */
            chap_break = cur->new_chap;
            hb_buffer_close( &cur );
            pv->cur = cur = hb_fifo_get( job->fifo_raw );
            cur->new_chap |= chap_break; // Don't stomp existing chapter breaks

            /* Calculate new offset */
            pv->pts_offset_old = pv->pts_offset;
            if ( job->vfr )
            {
                pv->pts_offset = cur->start - pv->next_start;
            } else {
                pv->pts_offset = cur->start -
                    pv->count_frames * pv->job->vrate_base / 300;
            }

            if( !pv->discontinuity )
            {
                pv->discontinuity = 1;
            }

            pv->video_sequence = cur->sequence;
            continue;
        }

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
                else
                {
                    /*
                     * The stop time is in the past. But is it due to
                     * it having been played already, or has the PTS
                     * been reset to 0?
                     */
                    if( ( cur->start - sub->stop ) > PTS_DISCONTINUITY_TOLERANCE ) {
                        /*
                         * There is a lot of time between our current
                         * video and where this subtitle is ending,
                         * assume that we are about to reset the PTS
                         * and do not throw away this subtitle.
                         */
                        break;
                    }
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

        if ( job->vfr )
        {
            /*
             * adjust the pts of the current frame so that it's contiguous
             * with the previous frame. pts_offset tracks the time difference
             * between the pts values in the input content (which start at some
             * random time) and our timestamps (which start at zero). We don't
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
            buf_tmp->start = pv->next_start;
            pv->next_start = next->start - pv->pts_offset;
            buf_tmp->stop = pv->next_start;
        }
        else
        {
            /* The PTS of the frame we are expecting now */
            pts_expected = pv->pts_offset +
                pv->count_frames * pv->job->vrate_base / 300;

            //hb_log("Video expecting PTS %lld, current frame: %lld, next frame: %lld, cf: %lld",
            //       pts_expected, cur->start, next->start, pv->count_frames * pv->job->vrate_base / 300 );

            if( cur->start < pts_expected - pv->job->vrate_base / 300 / 2 &&
                next->start < pts_expected + pv->job->vrate_base / 300 / 2 )
            {
                /* The current frame is too old but the next one matches,
                   let's trash */
                /* Also, make sure we don't trash a chapter break */
                chap_break = cur->new_chap;
                hb_buffer_close( &cur );
                pv->cur = cur = hb_fifo_get( job->fifo_raw );
                cur->new_chap |= chap_break; // Make sure we don't stomp the existing one.

                continue;
            }

            if( next->start > pts_expected + 3 * pv->job->vrate_base / 300 / 2 )
            {
                /* We'll need the current frame more than one time. Make a
                   copy of it and keep it */
                buf_tmp = hb_buffer_init( cur->size );
                memcpy( buf_tmp->data, cur->data, cur->size );
                buf_tmp->sequence = cur->sequence;
            }
            else
            {
                /* The frame has the expected date and won't have to be
                   duplicated, just put it through */
                buf_tmp = cur;
                pv->cur = cur = hb_fifo_get( job->fifo_raw );
            }

            /* Replace those MPEG-2 dates with our dates */
            buf_tmp->start = (uint64_t) pv->count_frames *
                pv->job->vrate_base / 300;
            buf_tmp->stop  = (uint64_t) ( pv->count_frames + 1 ) *
                pv->job->vrate_base / 300;
        }

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
            hb_log( "sync: got too many frames (%lld), exiting early", pv->count_frames );
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

/***********************************************************************
 * SyncAudio
 ***********************************************************************
 *
 **********************************************************************/
static void SyncAudio( hb_work_object_t * w, int i )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t        * job;
    hb_audio_t      * audio;
    hb_buffer_t     * buf;
    hb_sync_audio_t * sync;

    hb_fifo_t       * fifo;
    int               rate;

    int64_t           pts_expected;
    int64_t           start;

    job    = pv->job;
    sync   = &pv->sync_audio[i];
    audio  = sync->audio;

    if( job->acodec & HB_ACODEC_AC3 ||
        job->audio_mixdowns[i] == HB_AMIXDOWN_AC3 )
    {
        fifo = audio->fifo_out;
        rate = audio->rate;
    }
    else
    {
        fifo = audio->fifo_sync;
        rate = job->arate;
    }

    while( !hb_fifo_is_full( fifo ) &&
           ( buf = hb_fifo_see( audio->fifo_raw ) ) )
    {
        /* The PTS of the samples we are expecting now */
        pts_expected = pv->pts_offset + sync->count_frames * 90000 / rate;

        // hb_log("Video Sequence: %lld, Audio Sequence: %lld", pv->video_sequence, buf->sequence);

        /*
         * Using the same logic as the Video have we crossed a VOB
         * boundary as detected by the expected PTS and the PTS of our
         * audio being out by more than the tolerance value.
         */
        if( buf->start > pts_expected + PTS_DISCONTINUITY_TOLERANCE ||
            buf->start < pts_expected - PTS_DISCONTINUITY_TOLERANCE )
        {
            /* There has been a PTS discontinuity, and this frame might
               be from before the discontinuity*/

            if( pv->discontinuity )
            {
                /*
                 * There is an outstanding discontinuity, so use the offset from
                 * that discontinuity.
                 */
                pts_expected = pv->pts_offset_old + sync->count_frames *
                    90000 / rate;
            }
            else
            {
                /*
                 * No outstanding discontinuity, so the audio must be leading the
                 * video (or the PTS values are really stuffed). So lets mark this
                 * as a discontinuity ourselves for the audio to use until
                 * the video also crosses the discontinuity.
                 *
                 * pts_offset is used when we are in the same time space as the video
                 * pts_offset_old when in a discontinuity.
                 *
                 * Therefore set the pts_offset_old given the new pts_offset for this
                 * current buffer.
                 */
                pv->discontinuity = 1;
                pv->pts_offset_old = buf->start - sync->count_frames *
                    90000 / rate;
                pts_expected = pv->pts_offset_old + sync->count_frames *
                    90000 / rate;

                hb_log("Sync: Audio discontinuity (sequence: vid %lld aud %lld) (pts %lld < %lld < %lld)",
                       pv->video_sequence, buf->sequence,
                       pts_expected - PTS_DISCONTINUITY_TOLERANCE, buf->start,
                       pts_expected + PTS_DISCONTINUITY_TOLERANCE );
            }

            /*
             * Is the audio from a valid period given the previous
             * Video PTS. I.e. has there just been a video PTS
             * discontinuity and this audio belongs to the vdeo from
             * before?
             */
            if( buf->start > pts_expected + PTS_DISCONTINUITY_TOLERANCE ||
                buf->start < pts_expected - PTS_DISCONTINUITY_TOLERANCE )
            {
                /*
                 * It's outside of our tolerance for where the video
                 * is now, and it's outside of the tolerance for
                 * where we have been in the case of a VOB change.
                 * Try and reconverge regardless. so continue on to
                 * our convergence code below which will kick in as
                 * it will be more than 100ms out.
                 *
                 * Note that trashing the Audio could make things
                 * worse if the Audio is in front because we will end
                 * up diverging even more. We need to hold on to the
                 * audio until the video catches up.
                 */
                if( !pv->way_out_of_sync )
                {
                    hb_log("Sync: Audio is way out of sync, attempt to reconverge from current video PTS");
                    pv->way_out_of_sync = 1;
                }

                /*
                 * It wasn't from the old place, so we must be from
                 * the new, but just too far out. So attempt to
                 * reconverge by resetting the point we want to be to
                 * where we are currently wanting to be.
                 */
		pts_expected = pv->pts_offset + sync->count_frames * 90000 / rate;
                start = pts_expected - pv->pts_offset;
	    } else {
                 /* Use the older offset */
                start = pts_expected - pv->pts_offset_old;
	    }
        }
        else
        {
            start = pts_expected - pv->pts_offset;

            if( pv->discontinuity )
            {
                /*
                 * The Audio is tracking the Video again using the normal pts_offset, so the
                 * discontinuity is over.
                 */
                hb_log( "Sync: Audio joined Video after discontinuity at PTS %lld", buf->start );
                pv->discontinuity = 0;
            }
        }

        /* Tolerance: 100 ms */
        if( buf->start < pts_expected - 9000 )
        {
            if( !pv->trashing_audio )
            {
                /* Audio is behind the Video, trash it, can't use it now. */
                hb_log( "Sync: Audio PTS (%lld) < Video PTS (%lld) by greater than 100ms, trashing audio to reconverge",
                        buf->start, pts_expected);
                pv->trashing_audio = 1;
            }
            buf = hb_fifo_get( audio->fifo_raw );
            hb_buffer_close( &buf );
            continue;
        }
        else if( buf->start > pts_expected + 9000 )
        {
            /* Audio is ahead of the Video, insert silence until we catch up*/
            if( !pv->inserting_silence )
            {
                hb_log("Sync: Audio PTS (%lld) >  Video PTS (%lld) by greater than 100ms insert silence until reconverged", buf->start, pts_expected);
                pv->inserting_silence = 1;
            }
            InsertSilence( w, i );
            continue;
        }
        else
        {
            if( pv->trashing_audio || pv->inserting_silence )
            {
                hb_log( "Sync: Audio back in Sync at PTS %lld", buf->start );
                pv->trashing_audio = 0;
                pv->inserting_silence = 0;
            }
            if( pv->way_out_of_sync )
            {
                hb_log( "Sync: Audio no longer way out of sync at PTS %lld",
                        buf->start );
                pv->way_out_of_sync = 0;
            }
        }

        if( job->acodec & HB_ACODEC_AC3 ||
            job->audio_mixdowns[i] == HB_AMIXDOWN_AC3 )
        {
            buf        = hb_fifo_get( audio->fifo_raw );
            buf->start = start;
            buf->stop  = start + 90000 * AC3_SAMPLES_PER_FRAME / rate;

            sync->count_frames += AC3_SAMPLES_PER_FRAME;
        }
        else
        {
            hb_buffer_t * buf_raw = hb_fifo_get( audio->fifo_raw );

            int count_in, count_out;

            count_in  = buf_raw->size / HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->amixdown) / sizeof( float );
            count_out = ( buf_raw->stop - buf_raw->start ) * job->arate / 90000;
            if( buf->start < pts_expected - 1500 )
                count_out--;
            else if( buf->start > pts_expected + 1500 )
                count_out++;

            sync->data.data_in      = (float *) buf_raw->data;
            sync->data.input_frames = count_in;
            sync->data.output_frames = count_out;

            sync->data.src_ratio = (double) sync->data.output_frames /
                                   (double) sync->data.input_frames;

            buf = hb_buffer_init( sync->data.output_frames * HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->amixdown) *
                                  sizeof( float ) );
            sync->data.data_out = (float *) buf->data;
            if( src_process( sync->state, &sync->data ) )
            {
                /* XXX If this happens, we're screwed */
                hb_log( "sync: src_process failed" );
            }
            hb_buffer_close( &buf_raw );

            buf->size = sync->data.output_frames_gen * HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->amixdown) * sizeof( float );

            /* Set dates for resampled data */
            buf->start = start;
            buf->stop  = start + sync->data.output_frames_gen *
                            90000 / job->arate;

            sync->count_frames += sync->data.output_frames_gen;
        }

        buf->frametype = HB_FRAME_AUDIO;
        hb_fifo_push( fifo, buf );
    }

    if( hb_fifo_is_full( fifo ) &&
        pv->way_out_of_sync )
    {
        /*
         * Trash the top audio packet to avoid dead lock as we reconverge.
         */
        if ( (buf = hb_fifo_get( audio->fifo_raw ) ) != NULL)
            hb_buffer_close( &buf );
    }

    if( NeedSilence( w, audio ) )
    {
        InsertSilence( w, i );
    }
}

static int NeedSilence( hb_work_object_t * w, hb_audio_t * audio )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t * job = pv->job;

    if( hb_fifo_size( audio->fifo_in ) ||
        hb_fifo_size( audio->fifo_raw ) ||
        hb_fifo_size( audio->fifo_sync ) ||
        hb_fifo_size( audio->fifo_out ) )
    {
        /* We have some audio, we are fine */
        return 0;
    }

    /* No audio left in fifos */

    if( hb_thread_has_exited( job->reader ) )
    {
        /* We might miss some audio to complete encoding and muxing
           the video track */
	hb_log("Reader has exited early, inserting silence.");
        return 1;
    }

    if( hb_fifo_is_full( job->fifo_mpeg2 ) &&
        hb_fifo_is_full( job->fifo_raw ) &&
        hb_fifo_is_full( job->fifo_sync ) &&
        hb_fifo_is_full( job->fifo_render ) &&
        hb_fifo_is_full( job->fifo_mpeg4 ) )
    {
        /* Too much video and no audio, oh-oh */
	hb_log("Still got some video - and nothing in the audio fifo, insert silence");
        return 1;
    }

    return 0;
}

static void InsertSilence( hb_work_object_t * w, int i )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t        * job;
    hb_sync_audio_t * sync;
    hb_buffer_t     * buf;

    job    = pv->job;
    sync   = &pv->sync_audio[i];

    if( job->acodec & HB_ACODEC_AC3 ||
        job->audio_mixdowns[i] == HB_AMIXDOWN_AC3 )
    {
        buf        = hb_buffer_init( sync->ac3_size );
        buf->start = sync->count_frames * 90000 / sync->audio->rate;
        buf->stop  = buf->start + 90000 * AC3_SAMPLES_PER_FRAME /
                     sync->audio->rate;
        memcpy( buf->data, sync->ac3_buf, buf->size );

        hb_log( "sync: adding a silent AC-3 frame for track %x",
                sync->audio->id );
        hb_fifo_push( sync->audio->fifo_out, buf );

        sync->count_frames += AC3_SAMPLES_PER_FRAME;

    }
    else
    {
        buf        = hb_buffer_init( HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(sync->audio->amixdown) * job->arate / 20 *
                                     sizeof( float ) );
        buf->start = sync->count_frames * 90000 / job->arate;
        buf->stop  = buf->start + 90000 / 20;
        memset( buf->data, 0, buf->size );

        hb_fifo_push( sync->audio->fifo_sync, buf );

        sync->count_frames += job->arate / 20;
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
