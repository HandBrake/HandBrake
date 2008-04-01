/* $Id: muxcommon.c,v 1.23 2005/03/30 17:27:19 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

struct hb_mux_object_s
{
    HB_MUX_COMMON;
};

typedef struct
{
    hb_job_t * job;
    uint64_t   pts;

} hb_mux_t;

typedef struct
{
    hb_fifo_t     * fifo;
    hb_mux_data_t * mux_data;
    uint64_t        frames;
    uint64_t        bytes;

} hb_track_t;

static hb_track_t * GetTrack( hb_list_t * list )
{
    hb_buffer_t * buf;
    hb_track_t  * track = NULL, * track2;
    int64_t       pts = 0;
    int           i;

    for( i = 0; i < hb_list_count( list ); i++ )
    {
        track2 = hb_list_item( list, i );
        buf    = hb_fifo_see( track2->fifo );
        if( !buf )
        {
            return NULL;
        }
        if( !track || buf->start < pts )
        {
            track = track2;
            pts   = buf->start;
        }
    }
    return track;
}

static void MuxerFunc( void * _mux )
{
    hb_mux_t    * mux = _mux;
    hb_job_t    * job = mux->job;
    hb_title_t  * title = job->title;
    hb_audio_t  * audio;
    hb_list_t   * list;
    hb_buffer_t * buf;
    hb_track_t  * track;
    int           i;

    hb_mux_object_t * m = NULL;

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
    }

    /* Wait for one buffer for each track */
    while( !*job->die && !job->done )
    {
        int ready;

        ready = 1;
        if( !hb_fifo_size( job->fifo_mpeg4 ) )
        {
            ready = 0;
        }
        for( i = 0; i < hb_list_count( title->list_audio ); i++ )
        {
            audio = hb_list_item( title->list_audio, i );
            if( !hb_fifo_size( audio->priv.fifo_out ) )
            {
                ready = 0;
                break;
            }
        }

        if( ready )
        {
            break;
        }

        hb_snooze( 50 );
    }

    /* Create file, write headers */
    if( job->pass == 0 || job->pass == 2 )
    {
        m->init( m );
    }

    /* Build list of fifos we're interested in */
    list = hb_list_init();

    track           = calloc( sizeof( hb_track_t ), 1 );
    track->fifo     = job->fifo_mpeg4;
    track->mux_data = job->mux_data;
    hb_list_add( list, track );

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio           = hb_list_item( title->list_audio, i );
        track           = calloc( sizeof( hb_track_t ), 1 );
        track->fifo     = audio->priv.fifo_out;
        track->mux_data = audio->priv.mux_data;
        hb_list_add( list, track );
    }

	int thread_sleep_interval = 50;
	while( !*job->die && !job->done )
    {
        if( !( track = GetTrack( list ) ) )
        {
            hb_snooze( thread_sleep_interval );
//			thread_sleep_interval += 1;
            continue;
        }
//		thread_sleep_interval = MAX(1, (thread_sleep_interval - 1));

        buf = hb_fifo_get( track->fifo );
        if( job->pass == 0 || job->pass == 2 )
        {
            m->mux( m, track->mux_data, buf );
            track->frames += 1;
            track->bytes  += buf->size;
            mux->pts = buf->stop;
        }
        hb_buffer_close( &buf );
    }

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
            hb_log( "mux: file size, %lld bytes", (uint64_t) sb.st_size );

            bytes_total  = 0;
            frames_total = 0;
            for( i = 0; i < hb_list_count( list ); i++ )
            {
                track = hb_list_item( list, i );
                hb_log( "mux: track %d, %lld bytes, %.2f kbps",
                        i, track->bytes,
                        90000.0 * track->bytes / mux->pts / 125 );
                if( !i && ( job->vquality < 0.0 || job->vquality > 1.0 ) )
                {
                    /* Video */
                    hb_log( "mux: video bitrate error, %+lld bytes",
                            track->bytes - mux->pts * job->vbitrate *
                            125 / 90000 );
                }
                bytes_total  += track->bytes;
                frames_total += track->frames;
            }

            if( bytes_total && frames_total )
            {
                hb_log( "mux: overhead, %.2f bytes per frame",
                        (float) ( sb.st_size - bytes_total ) /
                        frames_total );
            }
        }
    }

    free( m );

    for( i = 0; i < hb_list_count( list ); i++ )
    {
        track = hb_list_item( list, i );
        if( track->mux_data )
        {
            free( track->mux_data );
        }
        free( track );
    }
    hb_list_close( &list );

    free( mux );
}

hb_thread_t * hb_muxer_init( hb_job_t * job )
{
    hb_mux_t * mux = calloc( sizeof( hb_mux_t ), 1 );
    mux->job = job;
    return hb_thread_init( "muxer", MuxerFunc, mux,
                           HB_NORMAL_PRIORITY );
}
