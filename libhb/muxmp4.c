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
	
    /* Chapter state information for muxing */
    MP4TrackId chapter_track;
    int current_chapter;
    uint64_t chapter_duration;
};

struct hb_mux_data_s
{
    MP4TrackId track;
};

struct hb_text_sample_s
{
    uint8_t     sample[1280];
    uint32_t    length;
    MP4Duration duration;
};

/**********************************************************************
 * MP4CreateTextSample
 **********************************************************************
 * Creates a buffer for a text track sample
 *********************************************************************/
static struct hb_text_sample_s *MP4CreateTextSample( char *textString, uint64_t duration )
{
    struct hb_text_sample_s *sample = NULL;
    int stringLength = strlen(textString);
    int x;
    
    if( stringLength < 1024 )
    {
        sample = malloc( sizeof( struct hb_text_sample_s ) );

        //textLength = (stringLength; // Account for BOM     
        sample->length = stringLength + 2 + 12; // Account for text length code and other marker
        sample->duration = (MP4Duration)duration;
        
        // 2-byte length marker
        sample->sample[0] = (stringLength >> 8) & 0xff;
        sample->sample[1] = stringLength & 0xff;
        
        strncpy( (char *)&(sample->sample[2]), textString, stringLength );
        
        x = 2 + stringLength;

        // Modifier Length Marker
        sample->sample[x] = 0x00;
        sample->sample[x+1] = 0x00;
        sample->sample[x+2] = 0x00;
        sample->sample[x+3] = 0x0C;
        
        // Modifier Type Code
        sample->sample[x+4] = 'e';
        sample->sample[x+5] = 'n';
        sample->sample[x+6] = 'c';
        sample->sample[x+7] = 'd';
        
        // Modifier Value
        sample->sample[x+8] = 0x00;
        sample->sample[x+9] = 0x00;
        sample->sample[x+10] = (256 >> 8) & 0xff;
        sample->sample[x+11] = 256 & 0xff;
    }
    
    return sample;
}
 
/**********************************************************************
 * MP4GenerateChapterSample
 **********************************************************************
 * Creates a buffer for a text track sample
 *********************************************************************/
static struct hb_text_sample_s *MP4GenerateChapterSample( hb_mux_object_t * m, uint64_t duration )
{
    int chapter = m->current_chapter;
    hb_chapter_t *chapter_data = hb_list_item( m->job->title->list_chapter, chapter - 1 );
    char tmp_buffer[1024];
    char *string = tmp_buffer;
    
    tmp_buffer[0] = '\0';
    
    if( chapter_data != NULL )
    {
        string = chapter_data->title;
    }
    
    if( strlen(string) == 0 || strlen(string) >= 1024 )
    {
        snprintf( tmp_buffer, 1023, "Chapter %03i", chapter );
        string = tmp_buffer;
    }
    
    return MP4CreateTextSample( string, duration );
}

 
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
    if (m->file == MP4_INVALID_FILE_HANDLE)
    {
        hb_log("muxmp4.c: MP4Create failed!");
        *job->die = 1;
        return 0;
    }

    /* Video track */
    mux_data      = malloc( sizeof( hb_mux_data_t ) );
    job->mux_data = mux_data;

    /* When using the standard 90000 timescale, QuickTime tends to have
       synchronization issues (audio not playing at the correct speed).
       To workaround this, we use the audio samplerate as the
       timescale */
    if (!(MP4SetTimeScale( m->file, job->arate )))
    {
        hb_log("muxmp4.c: MP4SetTimeScale failed!");
        *job->die = 1;
        return 0;
    }

    if( job->vcodec == HB_VCODEC_X264 )
    {
        /* Stolen from mp4creator */
        if(!(MP4SetVideoProfileLevel( m->file, 0x7F )))
        {
            hb_log("muxmp4.c: MP4SetVideoProfileLevel failed!");
            *job->die = 1;
            return 0;
        }

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
        if(!(MP4SetVideoProfileLevel( m->file, MPEG4_SP_L3 )))
        {
            hb_log("muxmp4.c: MP4SetVideoProfileLevel failed!");
            *job->die = 1;
            return 0;
        }
        mux_data->track = MP4AddVideoTrack( m->file, job->arate,
                MP4_INVALID_DURATION, job->width, job->height,
                MP4_MPEG4_VIDEO_TYPE );
        if (mux_data->track == MP4_INVALID_TRACK_ID)
        {
            hb_log("muxmp4.c: MP4AddVideoTrack failed!");
            *job->die = 1;
            return 0;
        }
        

        /* VOL from FFmpeg or XviD */
        if (!(MP4SetTrackESConfiguration( m->file, mux_data->track,
                job->config.mpeg4.bytes, job->config.mpeg4.length )))
        {
            hb_log("muxmp4.c: MP4SetTrackESConfiguration failed!");
            *job->die = 1;
            return 0;
        }
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
	MP4TrackId firstAudioTrack = 0;

	/* add the audio tracks */
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
    	static u_int8_t reserved2[16] = {
    		0x00, 0x00, 0x00, 0x00, 
    		0x00, 0x00, 0x00, 0x00, 
    		0x00, 0x02, 0x00, 0x10,
    		0x00, 0x00, 0x00, 0x00, 
	    };
	    
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
        
        /* Set the correct number of channels for this track */
        reserved2[9] = (u_int8_t)HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->amixdown);
        MP4SetTrackBytesProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.mp4a.reserved2", reserved2, sizeof(reserved2));
				
		/* store a reference to the first audio track,
		so we can use it to feed the chapter text track's sample rate */
		if (i == 0) {
			firstAudioTrack = mux_data->track;
		}
		
    }

	if (job->chapter_markers) 
    {
		/* add a text track for the chapters */
		MP4TrackId textTrack;
		textTrack = MP4AddChapterTextTrack(m->file, firstAudioTrack);
        
        m->chapter_track = textTrack;
        m->chapter_duration = 0;
        m->current_chapter = job->chapter_start;
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
        /* Add the sample before the new frame.
           It is important that this be calculated prior to the duration
           of the new video sample, as we want to sync to right after it.
           (This is because of how durations for text tracks work in QT) */
        if( job->chapter_markers && buf->new_chap )
        {
            struct hb_text_sample_s *sample = MP4GenerateChapterSample( m, (m->sum_dur - m->chapter_duration) );
        
            MP4WriteSample(m->file, m->chapter_track, sample->sample, sample->length, sample->duration, 0, true);
            free(sample);
            m->current_chapter++;
            m->chapter_duration = m->sum_dur;
        }
    
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

    /* When we do get the first keyframe, use its duration as the
       initial delay added to the frame order offset for b-frames.
       Because of b-pyramid, double this duration when there are
       b-pyramids, as denoted by job->areBframes equalling 2. */
    if ((mux_data->track == 1) && (thisSample == 0) && (buf->key == 1) && (job->vcodec == HB_VCODEC_X264))
    {
        initDelay = buf->renderOffset;
        thisSample++;
    }

    /* Here's where the sample actually gets muxed. 
       If it's an audio sample, don't offset the sample's playback.
       If it's a video sample and there are no b-frames, ditto.
       If there are b-frames, offset by the initDelay plus the
       difference between the presentation time stamp x264 gives
       and the decoding time stamp from the buffer data. */
       MP4WriteSample( m->file, mux_data->track, buf->data, buf->size,
            duration, ((mux_data->track != 1) || (job->areBframes==0) || (job->vcodec != HB_VCODEC_X264)) ? 0 : (  buf->renderOffset * job->arate / 90000),
            (buf->key == 1) );
                                
    return 0;
}

static int MP4End( hb_mux_object_t * m )
{
    /* Write our final chapter marker */
    if( m->job->chapter_markers )
    {
        struct hb_text_sample_s *sample = MP4GenerateChapterSample( m, (m->sum_dur - m->chapter_duration) );
    
        MP4WriteSample(m->file, m->chapter_track, sample->sample, sample->length, sample->duration, 0, true);
        free(sample);
    }
    
#if 0
    hb_job_t * job = m->job;
    char filename[1024]; memset( filename, 0, 1024 );
#endif

    hb_job_t * job = m->job;
    
    if (job->areBframes)
    /* Walk the entire video sample table and find the minumum ctts value. */
    {
           MP4SampleId count = MP4GetTrackNumberOfSamples( m->file, 1);
           MP4SampleId i;
           MP4Duration renderingOffset = 2000000000, tmp;
           
           // Find the smallest rendering offset
           for(i = 1; i <= count; i++)
           {
               tmp = MP4GetSampleRenderingOffset(m->file, 1, i);
               if(tmp < renderingOffset)
                   renderingOffset = tmp;
           }
           
           // Adjust all ctts values down by renderingOffset
           for(i = 1; i <= count; i++)
           {
               MP4SetSampleRenderingOffset(m->file,1,i,
                   MP4GetSampleRenderingOffset(m->file,1,i) - renderingOffset);
           }
           
           // Insert track edit to get A/V back in sync.  The edit amount is
           // the rendering offset of the first sample.
           MP4AddTrackEdit(m->file, 1, MP4_INVALID_EDIT_ID, MP4GetSampleRenderingOffset(m->file,1,1),
               MP4GetTrackDuration(m->file, 1), 0);
     }

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

