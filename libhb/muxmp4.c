/* $Id: muxmp4.c,v 1.24 2005/11/04 13:09:41 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

/* libmp4v2 header */
#include "mp4.h"

#include "hb.h"

void AddIPodUUID(MP4FileHandle, MP4TrackId);

/* B-frame muxing variables */
MP4SampleId thisSample = 0;
uint64_t initDelay;

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
    u_int16_t language_code;

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

		if (job->areBframes >= 1)
		{
			hb_log("muxmp4: Adjusting duration for B-frames");
		    mux_data->track = MP4AddH264VideoTrack( m->file, job->arate,
		            MP4_INVALID_DURATION+1, job->width, job->height,
		            job->config.h264.sps[1], /* AVCProfileIndication */
		            job->config.h264.sps[2], /* profile_compat */
		            job->config.h264.sps[3], /* AVCLevelIndication */
		            3 );      /* 4 bytes length before each NAL unit */			
		}
		else
		{
			hb_log("muxmp4: Using default duration as there are no B-frames");
		mux_data->track = MP4AddH264VideoTrack( m->file, job->arate,
		        MP4_INVALID_DURATION, job->width, job->height,
		        job->config.h264.sps[1], /* AVCProfileIndication */
		        job->config.h264.sps[2], /* profile_compat */
		        job->config.h264.sps[3], /* AVCLevelIndication */
		        3 );      /* 4 bytes length before each NAL unit */
		}

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

	/* apply the anamorphic transformation matrix if needed */

	if( job->pixel_ratio ) {

		uint8_t* val;
		uint8_t nval[38];
		uint32_t *ptr32 = (uint32_t*) (nval + 2);
		uint32_t size;

		MP4GetBytesProperty(m->file, "moov.trak.tkhd.reserved3", &val, &size);

		if (size == 38) {

			memcpy(nval, val, size);

			float width, height;
			float widthRatio;
			width = job->pixel_aspect_width;
			height = job->pixel_aspect_height;
			widthRatio = (width / height) * 0x10000;

			uint32_t widthRatioInt;
			widthRatioInt = (uint32_t)widthRatio;

#ifdef WORDS_BIGENDIAN
			ptr32[0] = widthRatioInt;
#else
			/* we need to switch the endianness, as the file format expects big endian */
			ptr32[0] = ((widthRatioInt & 0x000000FF) << 24) + ((widthRatioInt & 0x0000FF00) << 8) + ((widthRatioInt & 0x00FF0000) >> 8) + ((widthRatioInt & 0xFF000000) >> 24);
#endif

			if(!MP4SetBytesProperty(m->file, "moov.trak.tkhd.reserved3", nval, size)) {
				hb_log("Problem setting transform matrix");
			}
			
		}

	}

	/* end of transformation matrix */

	/* firstAudioTrack will be used to reference the first audio track when we add a chapter track */
	MP4TrackId firstAudioTrack;

	/* add the audio tracks */
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
                
        /* Set the language for this track */
        /* The language is stored as 5-bit text - 0x60 */
        language_code = audio->iso639_2[0] - 0x60;   language_code <<= 5;
        language_code |= audio->iso639_2[1] - 0x60;  language_code <<= 5;
        language_code |= audio->iso639_2[2] - 0x60;
        MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.mdhd.language", language_code);
				
		/* store a reference to the first audio track,
		so we can use it to feed the chapter text track's sample rate */
		if (i == 0) {
			firstAudioTrack = mux_data->track;
		}
		
    }

	if (job->chapter_markers) {

		/* add a text track for the chapters */
		MP4TrackId textTrack;

		textTrack = MP4AddChapterTextTrack(m->file, firstAudioTrack);

		/* write the chapter markers for each selected chapter */
		char markerBuf[13];
		hb_chapter_t  * chapter;
		MP4Duration chapterDuration;
		float fOrigDuration, fTimescale;
		float fTSDuration;

		for( i = job->chapter_start - 1; i <= job->chapter_end - 1; i++ )
		{
			chapter = hb_list_item( title->list_chapter, i );

			fOrigDuration = chapter->duration;
			fTimescale = job->arate;
			fTSDuration = (fOrigDuration / 90000) * fTimescale;
			chapterDuration = (MP4Duration)fTSDuration;
			
			sprintf(markerBuf, "  Chapter %03i", i + 1);
			markerBuf[0] = 0;
			markerBuf[1] = 11; // "Chapter xxx"
			MP4WriteSample(m->file, textTrack, (u_int8_t*)markerBuf, 13, chapterDuration, 0, true);

		}
		
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

    /* If for some reason the first frame muxmp4 gets isn't a key-frame,
       drop frames until we get one. (Yes, very bad. Quick and dirty.)
       This is for QuickTime--when it sees a non-IDR frame first, it
       displays a white box instead of video until the second GOP.
       Also, you've got to save the skipped duration to keep from
       throwing off the offset values. */
    if((mux_data->track == 1) && (thisSample == 0) && (buf->key != 1))
    {
	    initDelay +=duration;
	    return 0;
    }
    /* When we do get the first keyframe, use its duration as the
       initial delay added to the frame order offset for b-frames.
       Because of b-pyramid, double this duration when there are
       b-pyramids, as denoted by job->areBframes equalling 2. */
    if ((mux_data->track == 1) && (thisSample == 0) && (buf->key == 1))
    {
        initDelay += duration * job->areBframes;
        thisSample++;
    }

    /* Here's where the sample actually gets muxed. 
       If it's an audio sample, don't offset the sample's playback.
       If it's a video sample and there are no b-frames, ditto.
       If there are b-frames, offset by the initDelay plus the
       difference between the presentation time stamp x264 gives
       and the decoding time stamp from the buffer data. */
       MP4WriteSample( m->file, mux_data->track, buf->data, buf->size,
            duration, ((mux_data->track != 1) || (job->areBframes==0)) ? 0 : ( initDelay + ((buf->encodedPTS - buf->start) * job->arate / 90000)),
            (buf->key == 1) );
                                
    return 0;
}

static int MP4End( hb_mux_object_t * m )
{
#if 0
    hb_job_t * job = m->job;
    char filename[1024]; memset( filename, 0, 1024 );
#endif

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

