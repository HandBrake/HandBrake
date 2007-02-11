/* $Id: muxmp4.c,v 1.24 2005/11/04 13:09:41 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

/* libmp4v2 header */
#include "mp4.h"

#include "mediafork.h"

void AddIPodUUID(MP4FileHandle, MP4TrackId);


struct hb_mux_object_s
{
    HB_MUX_COMMON;

    hb_job_t * job;

    /* libmp4v2 handle */
    MP4FileHandle file;

    /* Cumulated durations so far, in timescale units (see MP4Mux) */
    uint64_t sum_dur;
};

struct hb_mux_data_s
{
    MP4TrackId track;
};

/**********************************************************************
 * MP4Init
 **********************************************************************
 * Allocates hb_mux_data_t structures, create file and write headers
 *********************************************************************/
static int MP4Init( hb_mux_object_t * m )
{
    hb_job_t   * job   = m->job;
    hb_title_t * title = job->title;
    
    hb_audio_t    * audio;
    hb_mux_data_t * mux_data;
    int i;

    /* Create an empty mp4 file */
    m->file = MP4Create( job->file, MP4_DETAILS_ERROR, 0 );

    /* Video track */
    mux_data      = malloc( sizeof( hb_mux_data_t ) );
    job->mux_data = mux_data;

    /* When using the standard 90000 timescale, QuickTime tends to have
       synchronization issues (audio not playing at the correct speed).
       To workaround this, we use the audio samplerate as the
       timescale */
    MP4SetTimeScale( m->file, job->arate );

    if( job->vcodec == HB_VCODEC_X264 )
    {
        /* Stolen from mp4creator */
        MP4SetVideoProfileLevel( m->file, 0x7F );

        mux_data->track = MP4AddH264VideoTrack( m->file, job->arate,
                MP4_INVALID_DURATION, job->width, job->height,
                job->config.h264.sps[1], /* AVCProfileIndication */
                job->config.h264.sps[2], /* profile_compat */
                job->config.h264.sps[3], /* AVCLevelIndication */
                3 );      /* 4 bytes length before each NAL unit */

        MP4AddH264SequenceParameterSet( m->file, mux_data->track,
                job->config.h264.sps, job->config.h264.sps_length );
        MP4AddH264PictureParameterSet( m->file, mux_data->track,
                job->config.h264.pps, job->config.h264.pps_length );

			if( job->h264_level == 30)
			{
				hb_log("About to add iPod atom");
			 	AddIPodUUID(m->file, mux_data->track);
			}
    }
    else /* FFmpeg or XviD */
    {
        MP4SetVideoProfileLevel( m->file, MPEG4_SP_L3 );
        mux_data->track = MP4AddVideoTrack( m->file, job->arate,
                MP4_INVALID_DURATION, job->width, job->height,
                MP4_MPEG4_VIDEO_TYPE );

        /* VOL from FFmpeg or XviD */
        MP4SetTrackESConfiguration( m->file, mux_data->track,
                job->config.mpeg4.bytes, job->config.mpeg4.length );
    }

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        mux_data = malloc( sizeof( hb_mux_data_t ) );
        audio->mux_data = mux_data;

        mux_data->track = MP4AddAudioTrack( m->file,
                job->arate, 1024, MP4_MPEG4_AUDIO_TYPE );
        MP4SetAudioProfileLevel( m->file, 0x0F );
        MP4SetTrackESConfiguration( m->file, mux_data->track,
                audio->config.aac.bytes, audio->config.aac.length );
    }

    return 0;
}

static int MP4Mux( hb_mux_object_t * m, hb_mux_data_t * mux_data,
                   hb_buffer_t * buf )
{
    hb_job_t * job = m->job;

    uint64_t duration;

    if( mux_data == job->mux_data )
    {
        /* Video */
        /* Because we use the audio samplerate as the timescale,
           we have to use potentially variable durations so the video
           doesn't go out of sync */
        duration    = ( buf->stop * job->arate / 90000 ) - m->sum_dur;
        m->sum_dur += duration;
    }
    else
    {
        /* Audio */
        duration = MP4_INVALID_DURATION;
    }

    MP4WriteSample( m->file, mux_data->track, buf->data, buf->size,
                    duration, 0, buf->key );
    return 0;
}

static int MP4End( hb_mux_object_t * m )
{
#if 0
    hb_job_t * job = m->job;
#endif
    char filename[1024]; memset( filename, 0, 1024 );

    MP4Close( m->file );

#if 0
    hb_log( "muxmp4: optimizing file" );
    snprintf( filename, 1024, "%s.tmp", job->file );
    MP4Optimize( job->file, filename, MP4_DETAILS_ERROR );
    remove( job->file );
    rename( filename, job->file );
#endif

    return 0;
}

hb_mux_object_t * hb_mux_mp4_init( hb_job_t * job )
{
    hb_mux_object_t * m = calloc( sizeof( hb_mux_object_t ), 1 );
    m->init      = MP4Init;
    m->mux       = MP4Mux;
    m->end       = MP4End;
    m->job       = job;
    return m;
}

