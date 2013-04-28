/* sync.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "hbffmpeg.h"
#include <stdio.h>
#include "samplerate.h"

#ifdef INT64_MIN
#undef INT64_MIN /* Because it isn't defined correctly in Zeta */
#endif
#define INT64_MIN (-9223372036854775807LL-1)

typedef struct
{
    hb_lock_t * mutex;
    int         ref;        /* Reference count to tell us when it's unused */
    int         count_frames;
    int64_t     audio_pts_slip;
    int64_t     video_pts_slip;
    int64_t     pts_offset;

    /* Frame based point-to-point support */
    int64_t     audio_pts_thresh;
    int         start_found;
    hb_cond_t * next_frame;
    int         pts_count;
    int64_t   * first_pts;
} hb_sync_common_t;

typedef struct
{
    int          index;
    double       next_start;   /* start time of next output frame */
    int64_t      first_drop;   /* PTS of first 'went backwards' frame dropped */
    int          drop_count;   /* count of 'time went backwards' drops */

    /* Raw */
    SRC_STATE  * state;
    SRC_DATA     data;

    int          silence_size;
    uint8_t    * silence_buf;

    int          drop_video_to_sync;

    double       gain_factor;
} hb_sync_audio_t;

typedef struct
{
    /* Video */
    int        first_frame;
    int64_t    pts_skip;
    int64_t    next_start;    /* start time of next output frame */
    int64_t    first_drop;    /* PTS of first 'went backwards' frame dropped */
    int        drop_count;    /* count of 'time went backwards' drops */
    int        drops;         /* frames dropped to make a cbr video stream */
    int        dups;          /* frames duplicated to make a cbr video stream */
    int        video_sequence;
    int        count_frames_max;
    int        chap_mark;     /* to propagate chapter mark across a drop */
    hb_buffer_t * cur;        /* The next picture to process */

    /* Statistics */
    uint64_t   st_counts[4];
    uint64_t   st_dates[4];
    uint64_t   st_first;
} hb_sync_video_t;

struct hb_work_private_s
{
    hb_job_t * job;
    hb_sync_common_t * common;
    union
    {
        hb_sync_video_t video;
        hb_sync_audio_t audio;
    } type;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void getPtsOffset( hb_work_object_t * w );
static int  checkPtsOffset( hb_work_object_t * w );
static void InitAudio( hb_job_t * job, hb_sync_common_t * common, int i );
static void InsertSilence( hb_work_object_t * w, int64_t d );
static void UpdateState( hb_work_object_t * w );
static void UpdateSearchState( hb_work_object_t * w, int64_t start );
static hb_buffer_t * OutputAudioFrame( hb_audio_t *audio, hb_buffer_t *buf,
                                       hb_sync_audio_t *sync );

/***********************************************************************
 * hb_work_sync_init
 ***********************************************************************
 * Initialize the work object
 **********************************************************************/
hb_work_object_t * hb_sync_init( hb_job_t * job )
{
    hb_title_t        * title = job->title;
    hb_chapter_t      * chapter;
    int                 i;
    uint64_t            duration;
    hb_work_private_t * pv;
    hb_sync_video_t   * sync;
    hb_work_object_t  * w;
    hb_work_object_t  * ret = NULL;

    pv = calloc( 1, sizeof( hb_work_private_t ) );
    sync = &pv->type.video;
    pv->common = calloc( 1, sizeof( hb_sync_common_t ) );
    pv->common->ref++;
    pv->common->mutex = hb_lock_init();
    pv->common->audio_pts_thresh = -1;
    pv->common->next_frame = hb_cond_init();
    pv->common->pts_count = 1;
    if ( job->frame_to_start || job->pts_to_start )
    {
        pv->common->start_found = 0;
    }
    else
    {
        pv->common->start_found = 1;
    }

    ret = w = hb_get_work( WORK_SYNC_VIDEO );
    w->private_data = pv;
    w->fifo_in = job->fifo_raw;

    // When doing subtitle indepth scan, the pipeline ends at sync
    if ( !job->indepth_scan )
        w->fifo_out = job->fifo_sync;
    else
        w->fifo_out = NULL;

    pv->job            = job;
    pv->common->pts_offset   = INT64_MIN;
    sync->first_frame = 1;

    if( job->pass == 2 )
    {
        /* We already have an accurate frame count from pass 1 */
        hb_interjob_t * interjob = hb_interjob_get( job->h );
        sync->count_frames_max = interjob->frame_count;
    }
    else
    {
        /* Calculate how many video frames we are expecting */
        if ( job->pts_to_stop )
        {
            duration = job->pts_to_stop + 90000;
        }
        else if( job->frame_to_stop )
        {
            /* Set the duration to a rough estimate */
            duration = ( job->frame_to_stop / ( title->rate / title->rate_base ) ) * 90000;
        }
        else
        {
            duration = 0;
            for( i = job->chapter_start; i <= job->chapter_end; i++ )
            {
                chapter   = hb_list_item( job->list_chapter, i - 1 );
                duration += chapter->duration;
            }
        }
        sync->count_frames_max = duration * title->rate / title->rate_base / 90000;
    }

    hb_log( "sync: expecting %d video frames", sync->count_frames_max );

    /* Initialize libsamplerate for every audio track we have */
    if ( ! job->indepth_scan )
    {
        for( i = 0; i < hb_list_count( job->list_audio ); i++ )
        {
            InitAudio( job, pv->common, i );
        }
    }
    pv->common->first_pts = malloc( sizeof(int64_t) * pv->common->pts_count );
    for ( i = 0; i < pv->common->pts_count; i++ )
        pv->common->first_pts[i] = INT64_MAX;

    return ret;
}

/***********************************************************************
 * Close Video
 ***********************************************************************
 *
 **********************************************************************/
void syncVideoClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t          * job   = pv->job;
    hb_sync_video_t   * sync = &pv->type.video;

    // Wake up audio sync if it's still waiting on condition.
    pv->common->pts_offset = 0;
    pv->common->start_found = 1;
    hb_cond_broadcast( pv->common->next_frame );

    if( sync->cur )
    {
        hb_buffer_close( &sync->cur );
    }

    hb_log( "sync: got %d frames, %d expected",
            pv->common->count_frames, sync->count_frames_max );

    /* save data for second pass */
    if( job->pass == 1 )
    {
        /* Preserve frame count for better accuracy in pass 2 */
        hb_interjob_t * interjob = hb_interjob_get( job->h );
        interjob->frame_count = pv->common->count_frames;
        interjob->last_job = job->sequence_id;
    }

    if (sync->drops || sync->dups )
    {
        hb_log( "sync: %d frames dropped, %d duplicated", 
                sync->drops, sync->dups );
    }

    hb_lock( pv->common->mutex );
    if ( --pv->common->ref == 0 )
    {
        hb_unlock( pv->common->mutex );
        hb_cond_close( &pv->common->next_frame );
        hb_lock_close( &pv->common->mutex );
        free( pv->common->first_pts );
        free( pv->common );
    }
    else
    {
        hb_unlock( pv->common->mutex );
    }

    free( pv );
    w->private_data = NULL;
}

/***********************************************************************
 * syncVideoWork
 ***********************************************************************
 *
 **********************************************************************/
int syncVideoWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
              hb_buffer_t ** buf_out )
{
    hb_buffer_t * cur, * next, * sub = NULL;
    hb_work_private_t * pv = w->private_data;
    hb_job_t          * job = pv->job;
    hb_subtitle_t     * subtitle;
    hb_sync_video_t   * sync = &pv->type.video;
    int i;
    int64_t next_start;

    *buf_out = NULL;
    next = *buf_in;
    *buf_in = NULL;

    /* Wait till we can determine the initial pts of all streams */
    if( next->size != 0 && pv->common->pts_offset == INT64_MIN )
    {
        pv->common->first_pts[0] = next->s.start;
        hb_lock( pv->common->mutex );
        while( pv->common->pts_offset == INT64_MIN )
        {
            // Full fifos will make us wait forever, so get the
            // pts offset from the available streams if full
            if ( hb_fifo_is_full( job->fifo_raw ) )
            {
                getPtsOffset( w );
                hb_cond_broadcast( pv->common->next_frame );
            }
            else if ( checkPtsOffset( w ) )
                hb_cond_broadcast( pv->common->next_frame );
            else
                hb_cond_timedwait( pv->common->next_frame, pv->common->mutex, 200 );
        }
        hb_unlock( pv->common->mutex );
    }

    hb_lock( pv->common->mutex );
    next_start = next->s.start - pv->common->video_pts_slip;
    hb_unlock( pv->common->mutex );

    /* Wait for start of point-to-point encoding */
    if( !pv->common->start_found )
    {
        hb_sync_video_t   * sync = &pv->type.video;

        if( next->size == 0 )
        {
            *buf_out = next;
            pv->common->start_found = 1;
            pv->common->first_pts[0] = INT64_MAX - 1;
            hb_cond_broadcast( pv->common->next_frame );

            /*
             * Push through any subtitle EOFs in case they 
             * were not synced through.
             */
            for( i = 0; i < hb_list_count( job->list_subtitle ); i++)
            {
                subtitle = hb_list_item( job->list_subtitle, i );
                if( subtitle->config.dest == PASSTHRUSUB )
                {
                    hb_fifo_push( subtitle->fifo_out, hb_buffer_init( 0 ) );
                }
            }
            return HB_WORK_DONE;
        }
        if ( pv->common->count_frames < job->frame_to_start ||
             next->s.start < job->pts_to_start )
        {
            // Flush any subtitles that have pts prior to the
            // current frame
            for( i = 0; i < hb_list_count( job->list_subtitle ); i++)
            {
                subtitle = hb_list_item( job->list_subtitle, i );
                while( ( sub = hb_fifo_see( subtitle->fifo_raw ) ) )
                {
                    if ( sub->s.start > next->s.start )
                        break;
                    sub = hb_fifo_get( subtitle->fifo_raw );
                    hb_buffer_close( &sub );
                }
            }
            hb_lock( pv->common->mutex );
            // Tell the audio threads what must be dropped
            pv->common->audio_pts_thresh = next_start + pv->common->video_pts_slip;
            hb_cond_broadcast( pv->common->next_frame );
            hb_unlock( pv->common->mutex );

            UpdateSearchState( w, next_start );
            hb_buffer_close( &next );

            return HB_WORK_OK;
        }
        hb_lock( pv->common->mutex );
        pv->common->audio_pts_thresh = 0;
        pv->common->audio_pts_slip += next_start;
        pv->common->video_pts_slip += next_start;
        next_start = 0;
        pv->common->start_found = 1;
        pv->common->count_frames = 0;
        hb_cond_broadcast( pv->common->next_frame );
        hb_unlock( pv->common->mutex );
        sync->st_first = 0;
    }

    if( !sync->cur )
    {
        sync->cur = next;
        if (next->size == 0)
        {
            /* we got an end-of-stream as our first video packet? 
             * Feed it downstream & signal that we're done. 
             */
            *buf_out = next;

            pv->common->start_found = 1;
            pv->common->first_pts[0] = INT64_MAX - 1;
            hb_cond_broadcast( pv->common->next_frame );

            /*
             * Push through any subtitle EOFs in case they 
             * were not synced through.
             */
            for( i = 0; i < hb_list_count( job->list_subtitle ); i++)
            {
                subtitle = hb_list_item( job->list_subtitle, i );
                if( subtitle->config.dest == PASSTHRUSUB )
                {
                    hb_fifo_push( subtitle->fifo_out, hb_buffer_init( 0 ) );
                }
            }
            return HB_WORK_DONE;
        }
        return HB_WORK_OK;
    }
    cur = sync->cur;
    /* At this point we have a frame to process. Let's check
        1) if we will be able to push into the fifo ahead
        2) if the next frame is there already, since we need it to
           compute the duration of the current frame*/
    if( next->size == 0 )
    {
        hb_buffer_close( &next );

        pv->common->first_pts[0] = INT64_MAX - 1;
        cur->s.start = sync->next_start;
        cur->s.stop = cur->s.start + 90000. / ((double)job->vrate / (double)job->vrate_base);
        sync->next_start += cur->s.stop - cur->s.start;;

        /* Make sure last frame is reflected in frame count */
        pv->common->count_frames++;

        /* Push the frame to the renderer */
        *buf_out = cur;
        sync->cur = NULL;

        /* we got an end-of-stream. Feed it downstream & signal that
         * we're done. Note that this means we drop the final frame of
         * video (we don't know its duration). On DVDs the final frame
         * is often strange and dropping it seems to be a good idea. */
        (*buf_out)->next = hb_buffer_init( 0 );

        /*
         * Push through any subtitle EOFs in case they were not synced through.
         */
        for( i = 0; i < hb_list_count( job->list_subtitle ); i++)
        {
            subtitle = hb_list_item( job->list_subtitle, i );
            if( subtitle->config.dest == PASSTHRUSUB )
            {
                hb_fifo_push( subtitle->fifo_out, hb_buffer_init( 0 ) );
            }
        }
        pv->common->start_found = 1;
        hb_cond_broadcast( pv->common->next_frame );
        return HB_WORK_DONE;
    }

    /* Check for end of point-to-point frame encoding */
    if( job->frame_to_stop && pv->common->count_frames > job->frame_to_stop )
    {
        // Drop an empty buffer into our output to ensure that things
        // get flushed all the way out.
        hb_buffer_close( &sync->cur );
        hb_buffer_close( &next );
        *buf_out = hb_buffer_init( 0 );
        hb_log( "sync: reached %d frames, exiting early",
                pv->common->count_frames );

        /*
         * Push through any subtitle EOFs in case they were not synced through.
         */
        for( i = 0; i < hb_list_count( job->list_subtitle ); i++)
        {
            subtitle = hb_list_item( job->list_subtitle, i );
            if( subtitle->config.dest == PASSTHRUSUB )
            {
                hb_fifo_push( subtitle->fifo_out, hb_buffer_init( 0 ) );
            }
        }
        return HB_WORK_DONE;
    }

    /* Check for end of point-to-point pts encoding */
    if( job->pts_to_stop && sync->next_start >= job->pts_to_stop )
    {
        // Drop an empty buffer into our output to ensure that things
        // get flushed all the way out.
        hb_log( "sync: reached pts %"PRId64", exiting early", cur->s.start );
        hb_buffer_close( &sync->cur );
        hb_buffer_close( &next );
        *buf_out = hb_buffer_init( 0 );

        /*
         * Push through any subtitle EOFs in case they were not synced through.
         */
        for( i = 0; i < hb_list_count( job->list_subtitle ); i++)
        {
            subtitle = hb_list_item( job->list_subtitle, i );
            if( subtitle->config.dest == PASSTHRUSUB )
            {
                hb_fifo_push( subtitle->fifo_out, hb_buffer_init( 0 ) );
            }
        }
        return HB_WORK_DONE;
    }

    if( sync->first_frame )
    {
        /* This is our first frame */
        if ( cur->s.start > 0 )
        {
            /*
             * The first pts from a dvd should always be zero but
             * can be non-zero with a transport or program stream since
             * we're not guaranteed to start on an IDR frame. If we get
             * a non-zero initial PTS extend its duration so it behaves
             * as if it started at zero so that our audio timing will
             * be in sync.
             */
            hb_log( "sync: first pts is %"PRId64, cur->s.start );
            cur->s.start = 0;
        }
        sync->first_frame = 0;
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
    if ( next_start - cur->s.start <= 0 )
    {
        if ( sync->first_drop == 0 )
        {
            sync->first_drop = next_start;
        }
        ++sync->drop_count;
        if ( next->s.new_chap )
        {
            // don't drop a chapter mark when we drop the buffer
            sync->chap_mark = next->s.new_chap;
        }
        hb_buffer_close( &next );
        return HB_WORK_OK;
    }
    if ( sync->first_drop )
    {
        hb_log( "sync: video time didn't advance - dropped %d frames "
                "(delta %d ms, current %"PRId64", next %"PRId64", dur %d)",
                sync->drop_count, (int)( cur->s.start - sync->first_drop ) / 90,
                cur->s.start, next_start, (int)( next_start - cur->s.start ) );
        sync->first_drop = 0;
        sync->drop_count = 0;
    }

    /*
     * Track the video sequence number locally so that we can sync the audio
     * to it using the sequence number as well as the PTS.
     */
    sync->video_sequence = cur->sequence;
    
    /* Process subtitles that apply to this video frame */
    // NOTE: There is no logic in either subtitle-sync algorithm that waits
    // for the subtitle-decoder if it is lagging behind the video-decoder.
    //       
    // Therefore there is the implicit assumption that the subtitle-decoder 
    // is always faster than the video-decoder. This assumption is definitely 
    // incorrect in some cases where the SSA subtitle decoder is used.

    for( i = 0; i < hb_list_count( job->list_subtitle ); i++)
    {
        int64_t sub_start, sub_stop, duration;

        subtitle = hb_list_item( job->list_subtitle, i );
        
        // Sanitize subtitle start and stop times, then pass to 
        // muxer or renderer filter.
        while ( ( sub = hb_fifo_see( subtitle->fifo_raw ) ) != NULL )
        {
            hb_lock( pv->common->mutex );
            sub_start = sub->s.start - pv->common->video_pts_slip;
            hb_unlock( pv->common->mutex );

            if (sub->s.stop == -1)
            {
                if (subtitle->config.dest != RENDERSUB &&
                    hb_fifo_size( subtitle->fifo_raw ) < 2)
                {
                    // For passthru subs, we want to wait for the
                    // next subtitle so that we can fill in the stop time.
                    // This way the muxer can compute the duration of
                    // the subtitle.
                    //
                    // For render subs, we need to ensure that they
                    // get to the renderer before the associated video
                    // that they are to be applied to.  It is the 
                    // responsibility of the renderer to handle
                    // stop == -1.
                    break;
                }
            }

            sub = hb_fifo_get( subtitle->fifo_raw );
            if ( sub->s.stop == -1 )
            {
                hb_buffer_t *next;
                next = hb_fifo_see( subtitle->fifo_raw );
                if (next != NULL)
                    sub->s.stop = next->s.start;
            }
            // Need to re-write subtitle timestamps to account
            // for any slippage.
            sub_stop = -1;
            if ( sub->s.stop != -1 )
            {
                duration = sub->s.stop - sub->s.start;
                sub_stop = sub_start + duration;
            }

            sub->s.start = sub_start;
            sub->s.stop = sub_stop;

            hb_fifo_push( subtitle->fifo_out, sub );
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
    *buf_out = cur;
    int64_t duration = next_start - cur->s.start;
    sync->cur = cur = next;
    cur->sub = NULL;
    cur->s.start -= pv->common->video_pts_slip;
    cur->s.stop -= pv->common->video_pts_slip;
    sync->pts_skip = 0;
    if ( duration <= 0 )
    {
        hb_log( "sync: invalid video duration %"PRId64", start %"PRId64", next %"PRId64"",
                duration, cur->s.start, next_start );
    }

    (*buf_out)->s.start = sync->next_start;
    sync->next_start += duration;
    (*buf_out)->s.stop = sync->next_start;

    if ( sync->chap_mark )
    {
        // we have a pending chapter mark from a recent drop - put it on this
        // buffer (this may make it one frame late but we can't do any better).
        (*buf_out)->s.new_chap = sync->chap_mark;
        sync->chap_mark = 0;
    }

    /* Update UI */
    UpdateState( w );

    return HB_WORK_OK;
}

// sync*Init does nothing because sync has a special initializer
// that takes care of initializing video and all audio tracks
int syncVideoInit( hb_work_object_t * w, hb_job_t * job)
{
    return 0;
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
void syncAudioClose( hb_work_object_t * w )
{
    hb_work_private_t * pv    = w->private_data;
    hb_sync_audio_t   * sync  = &pv->type.audio;

    if( sync->silence_buf )
    {
        free( sync->silence_buf );
    }
    if ( sync->state )
    {
        src_delete( sync->state );
    }

    hb_lock( pv->common->mutex );
    if ( --pv->common->ref == 0 )
    {
        hb_unlock( pv->common->mutex );
        hb_cond_close( &pv->common->next_frame );
        hb_lock_close( &pv->common->mutex );
        free( pv->common->first_pts );
        free( pv->common );
    }
    else
    {
        hb_unlock( pv->common->mutex );
    }

    free( pv );
    w->private_data = NULL;
}

int syncAudioInit( hb_work_object_t * w, hb_job_t * job)
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
    hb_job_t        * job = pv->job;
    hb_sync_audio_t * sync = &pv->type.audio;
    hb_buffer_t     * buf;
    int64_t start;

    *buf_out = NULL;
    buf = *buf_in;
    *buf_in = NULL;
    /* if the next buffer is an eof send it downstream */
    if ( buf->size <= 0 )
    {
        hb_buffer_close( &buf );
        *buf_out = hb_buffer_init( 0 );
        pv->common->first_pts[sync->index+1] = INT64_MAX - 1;
        return HB_WORK_DONE;
    }

    /* Wait till we can determine the initial pts of all streams */
    if( pv->common->pts_offset == INT64_MIN )
    {
        pv->common->first_pts[sync->index+1] = buf->s.start;
        hb_lock( pv->common->mutex );
        while( pv->common->pts_offset == INT64_MIN )
        {
            // Full fifos will make us wait forever, so get the
            // pts offset from the available streams if full
            if (hb_fifo_is_full(w->fifo_in))
            {
                getPtsOffset( w );
                hb_cond_broadcast( pv->common->next_frame );
            }
            else if ( checkPtsOffset( w ) )
                hb_cond_broadcast( pv->common->next_frame );
            else
                hb_cond_timedwait( pv->common->next_frame, pv->common->mutex, 200 );
        }
        hb_unlock( pv->common->mutex );
    }

    /* Wait for start frame if doing point-to-point */
    hb_lock( pv->common->mutex );
    start = buf->s.start - pv->common->audio_pts_slip;
    while ( !pv->common->start_found )
    {
        if ( pv->common->audio_pts_thresh < 0 )
        {
            // I would initialize this in hb_sync_init, but 
            // job->pts_to_start can be modified by reader 
            // after hb_sync_init is called.
            pv->common->audio_pts_thresh = job->pts_to_start;
        }
        if ( buf->s.start < pv->common->audio_pts_thresh )
        {
            hb_buffer_close( &buf );
            hb_unlock( pv->common->mutex );
            return HB_WORK_OK;
        }
        while ( !pv->common->start_found && 
                buf->s.start >= pv->common->audio_pts_thresh )
        {
            hb_cond_timedwait( pv->common->next_frame, pv->common->mutex, 10 );
            // There is an unfortunate unavoidable deadlock that can occur.
            // Since we need to wait for a specific frame in syncVideoWork,
            // syncAudioWork can be stalled indefinitely.  The video decoder
            // often drops multiple of the initial frames after starting
            // because they require references that have not been decoded yet.
            // This allows a lot of audio to be queued in the fifo and the
            // audio fifo fills before we get a single video frame.  So we
            // must drop some audio to unplug the pipeline and allow the first
            // video frame to be decoded.
            if ( hb_fifo_is_full(w->fifo_in) )
            {
                hb_buffer_t *tmp;
                tmp = buf = hb_fifo_get( w->fifo_in );
                while ( tmp )
                {
                    tmp = hb_fifo_get( w->fifo_in );
                    if ( tmp )
                    {
                        hb_buffer_close( &buf );
                        buf = tmp;
                    }
                }
            }
        }
        start = buf->s.start - pv->common->audio_pts_slip;
    }
    if ( start < 0 )
    {
        hb_buffer_close( &buf );
        hb_unlock( pv->common->mutex );
        return HB_WORK_OK;
    }
    hb_unlock( pv->common->mutex );

    if( job->frame_to_stop && pv->common->count_frames >= job->frame_to_stop )
    {
        hb_buffer_close( &buf );
        *buf_out = hb_buffer_init( 0 );
        return HB_WORK_DONE;
    }

    if( job->pts_to_stop && sync->next_start >= job->pts_to_stop )
    {
        hb_buffer_close( &buf );
        *buf_out = hb_buffer_init( 0 );
        return HB_WORK_DONE;
    }

    // audio time went backwards.
    // If our output clock is more than a half frame ahead of the
    // input clock drop this frame to move closer to sync.
    // Otherwise drop frames until the input clock matches the output clock.
    if ( sync->next_start - start > 90*15 )
    {
        // Discard data that's in the past.
        if ( sync->first_drop == 0 )
        {
            sync->first_drop = start;
        }
        ++sync->drop_count;
        hb_buffer_close( &buf );
        return HB_WORK_OK;
    }
    if ( sync->first_drop )
    {
        // we were dropping old data but input buf time is now current
        hb_log( "sync: audio 0x%x time went backwards %d ms, dropped %d frames "
                "(start %"PRId64", next %"PRId64")", w->audio->id,
                (int)( sync->next_start - sync->first_drop ) / 90,
                sync->drop_count, sync->first_drop, (int64_t)sync->next_start );
        sync->first_drop = 0;
        sync->drop_count = 0;
    }
    if ( start - sync->next_start >= (90 * 70) )
    {
        if ( start - sync->next_start > (90000LL * 60) )
        {
            // there's a gap of more than a minute between the last
            // frame and this. assume we got a corrupted timestamp
            // and just drop the next buf.
            hb_log( "sync: %d minute time gap in audio 0x%x - dropping buf"
                    "  start %"PRId64", next %"PRId64,
                    (int)((start - sync->next_start) / (90000*60)),
                    w->audio->id, start, (int64_t)sync->next_start );
            hb_buffer_close( &buf );
            return HB_WORK_OK;
        }
        /*
         * there's a gap of at least 70ms between the last
         * frame we processed & the next. Fill it with silence.
         * Or in the case of DCA, skip some frames from the
         * other streams.
         */
        if ( sync->drop_video_to_sync )
        {
            hb_log( "sync: audio gap %d ms. Skipping frames. Audio 0x%x"
                    "  start %"PRId64", next %"PRId64,
                    (int)((start - sync->next_start) / 90),
                    w->audio->id, start, (int64_t)sync->next_start );
            hb_lock( pv->common->mutex );
            pv->common->audio_pts_slip += (start - sync->next_start);
            pv->common->video_pts_slip += (start - sync->next_start);
            hb_unlock( pv->common->mutex );
            *buf_out = OutputAudioFrame( w->audio, buf, sync );
            return HB_WORK_OK;
        }
        hb_log( "sync: adding %d ms of silence to audio 0x%x"
                "  start %"PRId64", next %"PRId64,
                (int)((start - sync->next_start) / 90),
                w->audio->id, start, (int64_t)sync->next_start );
        InsertSilence( w, start - sync->next_start );
    }

    /*
     * When we get here we've taken care of all the dups and gaps in the
     * audio stream and are ready to inject the next input frame into
     * the output stream.
     */
    *buf_out = OutputAudioFrame( w->audio, buf, sync );
    return HB_WORK_OK;
}

hb_work_object_t hb_sync_audio =
{
    WORK_SYNC_AUDIO,
    "AudioSynchronization",
    syncAudioInit,
    syncAudioWork,
    syncAudioClose
};

static void InitAudio( hb_job_t * job, hb_sync_common_t * common, int i )
{
    hb_work_object_t  * w;
    hb_work_private_t * pv;
    hb_sync_audio_t   * sync;

    pv = calloc( 1, sizeof( hb_work_private_t ) );
    sync = &pv->type.audio;
    sync->index = i;
    pv->job    = job;
    pv->common = common;
    pv->common->ref++;
    pv->common->pts_count++;

    w = hb_get_work( WORK_SYNC_AUDIO );
    w->private_data = pv;
    w->audio = hb_list_item( job->list_audio, i );
    w->fifo_in = w->audio->priv.fifo_raw;

    if ( w->audio->config.out.codec & HB_ACODEC_PASS_FLAG )
    {
        w->fifo_out = w->audio->priv.fifo_out;
    }
    else
    {
        w->fifo_out = w->audio->priv.fifo_sync;
    }

    if( w->audio->config.out.codec == HB_ACODEC_AC3_PASS ||
        w->audio->config.out.codec == HB_ACODEC_AAC_PASS )
    {
        /* Have a silent AC-3/AAC frame ready in case we have to fill a
           gap */
        AVCodec        * codec;
        AVCodecContext * c;

        switch ( w->audio->config.out.codec )
        {
            case HB_ACODEC_AC3_PASS:
            {
                codec = avcodec_find_encoder( AV_CODEC_ID_AC3 );
            } break;
            case HB_ACODEC_AAC_PASS:
            {
                codec = avcodec_find_encoder( AV_CODEC_ID_AAC );
            } break;
            default:
            {
                // Never gets here
                codec = NULL; // Silence compiler warning
            } break;
        }

        c              = avcodec_alloc_context3(codec);
        c->bit_rate    = w->audio->config.in.bitrate;
        c->sample_rate = w->audio->config.in.samplerate;
        c->channels    =
            av_get_channel_layout_nb_channels(w->audio->config.in.channel_layout);
        hb_ff_set_sample_fmt(c, codec, AV_SAMPLE_FMT_FLT);

        if (w->audio->config.in.channel_layout == AV_CH_LAYOUT_STEREO_DOWNMIX)
        {
            c->channel_layout = AV_CH_LAYOUT_STEREO;
        }
        else
        {
            c->channel_layout = w->audio->config.in.channel_layout;
        }

        if (hb_avcodec_open(c, codec, NULL, 0) < 0)
        {
            hb_error("sync: avcodec_open failed");
            *job->die = 1;
            return;
        }

        // Prepare input frame
        AVFrame frame = { .nb_samples = c->frame_size, .pts = 0, };
        int input_size = av_samples_get_buffer_size(NULL, c->channels,
                                                    frame.nb_samples,
                                                    c->sample_fmt, 1);
        uint8_t *zeros = calloc(1, input_size);
        avcodec_fill_audio_frame(&frame, c->channels, c->sample_fmt, zeros,
                                 input_size, 1);

        // Allocate enough space for the encoded silence
        // The output should be < the input
        sync->silence_buf  = malloc( input_size );

        // There is some delay in getting output from some audio encoders.
        // So encode a few packets till we get output.
        int ii;
        for ( ii = 0; ii < 10; ii++ )
        {
            // Prepare output packet
            AVPacket pkt;
            int got_packet;
            av_init_packet(&pkt);
            pkt.data = sync->silence_buf;
            pkt.size = input_size;

            int ret = avcodec_encode_audio2( c, &pkt, &frame, &got_packet);
            if ( ret < 0 )
            {
                hb_log( "sync: avcodec_encode_audio failed" );
                break;
            }

            if ( got_packet )
            {
                sync->silence_size = pkt.size;
                break;
            }
        }
        free( zeros );
        hb_avcodec_close( c );
        av_free( c );
    }
    else
    {
        if( w->audio->config.out.codec & HB_ACODEC_PASS_FLAG )
        {
            sync->drop_video_to_sync = 1;
        }
        else
        {
            /* Not passthru, initialize libsamplerate */
            int error;
            sync->state = src_new( SRC_SINC_MEDIUM_QUALITY,
                                   hb_mixdown_get_discrete_channel_count( w->audio->config.out.mixdown ),
                                   &error );
            sync->data.end_of_input = 0;
        }
    }

    sync->gain_factor = pow(10, w->audio->config.out.gain / 20);

    hb_list_add( job->list_work, w );
}

static hb_buffer_t * OutputAudioFrame( hb_audio_t *audio, hb_buffer_t *buf,
                                       hb_sync_audio_t *sync )
{
    int64_t start = (int64_t)sync->next_start;

    // Can't count of buf->s.stop - buf->s.start for accurate duration
    // due to integer rounding, so use buf->s.duration when it is set
    // (which should be always if I didn't miss anything)
    double duration;
    if ( buf->s.duration > 0 )
        duration = buf->s.duration;
    else
        duration = buf->s.stop - buf->s.start;

    if ( !( audio->config.out.codec & HB_ACODEC_PASS_FLAG ) )
    {
        // Audio is not passthru.  Check if we need to modify the audio
        // in any way.
        if( audio->config.in.samplerate != audio->config.out.samplerate )
        {
            /* do sample rate conversion */
            int count_in, count_out;
            hb_buffer_t * buf_raw = buf;
            int sample_size = hb_mixdown_get_discrete_channel_count( audio->config.out.mixdown ) *
                              sizeof( float );

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
            count_out = ( duration * audio->config.out.samplerate ) / 90000 + 1;

            sync->data.input_frames = count_in;
            sync->data.output_frames = count_out;
            sync->data.src_ratio = (double)audio->config.out.samplerate /
                                   (double)audio->config.in.samplerate;

            buf = hb_buffer_init( count_out * sample_size );
            sync->data.data_in  = (float *) buf_raw->data;
            sync->data.data_out = (float *) buf->data;
            if( src_process( sync->state, &sync->data ) )
            {
                /* XXX If this happens, we're screwed */
                hb_log( "sync: audio 0x%x src_process failed", audio->id );
            }
            hb_buffer_close( &buf_raw );

            if (sync->data.output_frames_gen <= 0)
            {
                // XXX: don't send empty buffers downstream (EOF)
                // possibly out-of-sync audio is better than no audio at all
                hb_buffer_close(&buf);
                return NULL;
            }
            buf->size = sync->data.output_frames_gen * sample_size;
            duration = (double)( sync->data.output_frames_gen * 90000 ) /
                       audio->config.out.samplerate;
        }
        if( audio->config.out.gain > 0.0 )
        {
            int count, ii;

            count  = buf->size / sizeof(float);
            for ( ii = 0; ii < count; ii++ )
            {
                double sample;

                sample = (double)*(((float*)buf->data)+ii);
                sample *= sync->gain_factor;
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
                sample *= sync->gain_factor;
                *(((float*)buf->data)+ii) = sample;
            }
        }
    }

    buf->s.type = AUDIO_BUF;
    buf->s.frametype = HB_FRAME_AUDIO;

    buf->s.start = start;
    sync->next_start += duration;
    buf->s.stop  = (int64_t)sync->next_start;
    return buf;
}

static void InsertSilence( hb_work_object_t * w, int64_t duration )
{
    hb_work_private_t * pv = w->private_data;
    hb_sync_audio_t *sync = &pv->type.audio;
    hb_buffer_t     *buf;
    hb_fifo_t       *fifo;
    int frame_dur;

    // to keep pass-thru and regular audio in sync we generate silence in
    // frame-sized units. If the silence duration isn't an integer multiple
    // of the frame duration we will truncate or round up depending on
    // which minimizes the timing error.
    if( w->audio->config.out.codec & HB_ACODEC_PASS_FLAG )
    {
        frame_dur = ( 90000 * w->audio->config.in.samples_per_frame ) /
                                            w->audio->config.in.samplerate;
    }
    else
    {
        frame_dur = ( 90000 * w->audio->config.out.samples_per_frame ) /
                                            w->audio->config.in.samplerate;
    }

    while (duration >= frame_dur >> 2)
    {
        if( w->audio->config.out.codec & HB_ACODEC_PASS_FLAG )
        {
            buf        = hb_buffer_init( sync->silence_size );
            buf->s.start = sync->next_start;
            buf->s.stop  = buf->s.start + frame_dur;
            memcpy( buf->data, sync->silence_buf, buf->size );
            fifo = w->audio->priv.fifo_out;
            duration -= frame_dur;
        }
        else
        {
            int channel_count = hb_mixdown_get_discrete_channel_count( w->audio->config.out.mixdown );
            int size = sizeof( float ) *
                       w->audio->config.out.samples_per_frame *
                       channel_count;
            if (frame_dur > duration)
            {
                int samples = duration * w->audio->config.in.samplerate / 90000;
                if (samples == 0)
                {
                    break;
                }
                size = sizeof(float) * samples * channel_count;
                frame_dur = (90000 * samples) / w->audio->config.in.samplerate;
            }
            buf = hb_buffer_init(size);
            buf->s.start = sync->next_start;
            buf->s.duration = frame_dur;
            buf->s.stop  = buf->s.start + frame_dur;
            memset( buf->data, 0, buf->size );
            fifo = w->audio->priv.fifo_sync;
            duration -= frame_dur;
        }
        buf = OutputAudioFrame( w->audio, buf, sync );
        hb_fifo_push( fifo, buf );
    }
}

static void UpdateState( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_sync_video_t   * sync = &pv->type.video;
    hb_state_t state;

    if( !pv->common->count_frames )
    {
        sync->st_first = hb_get_date();
        pv->job->st_pause_date = -1;
        pv->job->st_paused = 0;
    }
    pv->common->count_frames++;

    if (pv->job->indepth_scan)
    {
        // Progress for indept scan is handled by reader
        // pv->common->count_frames is used during indepth_scan
        // to find start & end points.
        return;
    }

    if( hb_get_date() > sync->st_dates[3] + 1000 )
    {
        memmove( &sync->st_dates[0], &sync->st_dates[1],
                 3 * sizeof( uint64_t ) );
        memmove( &sync->st_counts[0], &sync->st_counts[1],
                 3 * sizeof( uint64_t ) );
        sync->st_dates[3]  = hb_get_date();
        sync->st_counts[3] = pv->common->count_frames;
    }

#define p state.param.working
    state.state = HB_STATE_WORKING;
    p.progress  = (float) pv->common->count_frames / (float) sync->count_frames_max;
    if( p.progress > 1.0 )
    {
        p.progress = 1.0;
    }
    p.rate_cur   = 1000.0 *
        (float) ( sync->st_counts[3] - sync->st_counts[0] ) /
        (float) ( sync->st_dates[3] - sync->st_dates[0] );
    if( hb_get_date() > sync->st_first + 4000 )
    {
        int eta;
        p.rate_avg = 1000.0 * (float) sync->st_counts[3] /
            (float) ( sync->st_dates[3] - sync->st_first - pv->job->st_paused);
        eta = (float) ( sync->count_frames_max - sync->st_counts[3] ) /
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

static void UpdateSearchState( hb_work_object_t * w, int64_t start )
{
    hb_work_private_t * pv = w->private_data;
    hb_sync_video_t   * sync = &pv->type.video;
    hb_state_t state;
    uint64_t now;
    double avg;

    now = hb_get_date();
    if( !pv->common->count_frames )
    {
        sync->st_first = now;
        pv->job->st_pause_date = -1;
        pv->job->st_paused = 0;
    }
    pv->common->count_frames++;

    if (pv->job->indepth_scan)
    {
        // Progress for indept scan is handled by reader
        // pv->common->count_frames is used during indepth_scan
        // to find start & end points.
        return;
    }

#define p state.param.working
    state.state = HB_STATE_SEARCHING;
    if ( pv->job->frame_to_start )
        p.progress  = (float) pv->common->count_frames / 
                      (float) pv->job->frame_to_start;
    else if ( pv->job->pts_to_start )
        p.progress  = (float) start / (float) pv->job->pts_to_start;
    else
        p.progress = 0;
    if( p.progress > 1.0 )
    {
        p.progress = 1.0;
    }
    if (now > sync->st_first)
    {
        int eta = 0;

        if ( pv->job->frame_to_start )
        {
            avg = 1000.0 * (double)pv->common->count_frames / (now - sync->st_first);
            eta = ( pv->job->frame_to_start - pv->common->count_frames ) / avg;
        }
        else if ( pv->job->pts_to_start )
        {
            avg = 1000.0 * (double)start / (now - sync->st_first);
            eta = ( pv->job->pts_to_start - start ) / avg;
        }
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

static void getPtsOffset( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    int           i ;
    int64_t       first_pts = INT64_MAX;

    for( i = 0; i < pv->common->pts_count; i++ )
    {
        if ( pv->common->first_pts[i] < first_pts )
            first_pts = pv->common->first_pts[i];
    }
    pv->common->video_pts_slip = pv->common->audio_pts_slip = pv->common->pts_offset = first_pts;
    return;
}

static int checkPtsOffset( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    int           i ;

    for( i = 0; i < pv->common->pts_count; i++ )
    {
        if ( pv->common->first_pts[i] == INT64_MAX )
            return 0;
    }
    getPtsOffset( w );
    return 1;
}
