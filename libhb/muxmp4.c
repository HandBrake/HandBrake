/* $Id: muxmp4.c,v 1.24 2005/11/04 13:09:41 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

/* libmp4v2 header */
#include "mp4.h"

#include "hb.h"

void AddIPodUUID(MP4FileHandle, MP4TrackId);

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
    
    /* Flags for enabling/disabling tracks in an MP4. */
    typedef enum { TRACK_DISABLED = 0x0, TRACK_ENABLED = 0x1, TRACK_IN_MOVIE = 0x2, TRACK_IN_PREVIEW = 0x4, TRACK_IN_POSTER = 0x8}  track_header_flags;
    

    /* Create an empty mp4 file */
    if (job->largeFileSize)
    /* Use 64-bit MP4 file */
    {
        m->file = MP4Create( job->file, MP4_DETAILS_ERROR, MP4_CREATE_64BIT_DATA ); 
        hb_log("Using 64-bit MP4 formatting.");
    }
    else
    /* Limit MP4s to less than 4 GB */
    {
        m->file = MP4Create( job->file, MP4_DETAILS_ERROR, 0 );
    }
    
    if (m->file == MP4_INVALID_FILE_HANDLE)
    {
        hb_error("muxmp4.c: MP4Create failed!");
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
        hb_error("muxmp4.c: MP4SetTimeScale failed!");
        *job->die = 1;
        return 0;
    }

    if( job->vcodec == HB_VCODEC_X264 )
    {
        /* Stolen from mp4creator */
        if(!(MP4SetVideoProfileLevel( m->file, 0x7F )))
        {
            hb_error("muxmp4.c: MP4SetVideoProfileLevel failed!");
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

		if( job->h264_level == 30 || job->ipod_atom)
		{
			hb_log("About to add iPod atom");
			AddIPodUUID(m->file, mux_data->track);
		}

    }
    else /* FFmpeg or XviD */
    {
        if(!(MP4SetVideoProfileLevel( m->file, MPEG4_SP_L3 )))
        {
            hb_error("muxmp4.c: MP4SetVideoProfileLevel failed!");
            *job->die = 1;
            return 0;
        }
        mux_data->track = MP4AddVideoTrack( m->file, job->arate,
                MP4_INVALID_DURATION, job->width, job->height,
                MP4_MPEG4_VIDEO_TYPE );
        if (mux_data->track == MP4_INVALID_TRACK_ID)
        {
            hb_error("muxmp4.c: MP4AddVideoTrack failed!");
            *job->die = 1;
            return 0;
        }
        

        /* VOL from FFmpeg or XviD */
        if (!(MP4SetTrackESConfiguration( m->file, mux_data->track,
                job->config.mpeg4.bytes, job->config.mpeg4.length )))
        {
            hb_error("muxmp4.c: MP4SetTrackESConfiguration failed!");
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
			width = job->pixel_aspect_width;
			height = job->pixel_aspect_height;

           uint32_t pixelRatioInt;
           if (width >= height)
           {
               pixelRatioInt = (uint32_t)((width / height) * 0x10000);

#ifdef WORDS_BIGENDIAN
               ptr32[0] = pixelRatioInt;
#else
               /* we need to switch the endianness, as the file format expects big endian */
               ptr32[0] = ((pixelRatioInt & 0x000000FF) << 24) + ((pixelRatioInt & 0x0000FF00) << 8) + ((pixelRatioInt & 0x00FF0000) >> 8) + ((pixelRatioInt & 0xFF000000) >> 24);
#endif

           }
           else
           {
               pixelRatioInt = (uint32_t)((height / width) * 0x10000);
#ifdef WORDS_BIGENDIAN
               ptr32[4] = pixelRatioInt;
#else
                /* we need to switch the endianness, as the file format expects big endian */
                ptr32[4] = ((pixelRatioInt & 0x000000FF) << 24) + ((pixelRatioInt & 0x0000FF00) << 8) + ((pixelRatioInt & 0x00FF0000) >> 8) + ((pixelRatioInt & 0xFF000000) >> 24);
#endif
            }


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

        if( job->acodec & HB_ACODEC_AC3 ||
            job->audio_mixdowns[i] == HB_AMIXDOWN_AC3 )
        {
            mux_data->track = MP4AddAC3AudioTrack( 
                m->file,
                job->arate, 1536, MP4_MPEG4_AUDIO_TYPE );  
            MP4SetTrackBytesProperty( 
                m->file, mux_data->track,
                "udta.name.value", 
                (const u_int8_t*)"Surround", strlen("Surround"));
        } else {
            mux_data->track = MP4AddAudioTrack( 
                m->file,
                job->arate, 1024, MP4_MPEG4_AUDIO_TYPE );
            MP4SetTrackBytesProperty( 
                m->file, mux_data->track,
                "udta.name.value", 
                (const u_int8_t*)"Stereo", strlen("Stereo"));
            
            MP4SetAudioProfileLevel( m->file, 0x0F );
            MP4SetTrackESConfiguration( 
                m->file, mux_data->track,
                audio->config.aac.bytes, audio->config.aac.length );

            /* Set the correct number of channels for this track */
            reserved2[9] = (u_int8_t)HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->amixdown);
            MP4SetTrackBytesProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.mp4a.reserved2", reserved2, sizeof(reserved2));

        }
        /* Set the language for this track */
        /* The language is stored as 5-bit text - 0x60 */
        language_code = audio->iso639_2[0] - 0x60;   language_code <<= 5;
        language_code |= audio->iso639_2[1] - 0x60;  language_code <<= 5;
        language_code |= audio->iso639_2[2] - 0x60;
        MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.mdhd.language", language_code);
        

        /* Set the audio track alternate group */
        MP4SetTrackIntegerProperty(m->file, mux_data->track, "tkhd.alternate_group", 1);
        
        /* If we ever upgrade mpeg4ip, the line above should be replaced with the line below.*/
//        MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.mp4a.channels",  (u_int16_t)HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->amixdown));
        
        /* store a reference to the first audio track,
        so we can use it to feed the chapter text track's sample rate */
        if (i == 0) {
            firstAudioTrack = mux_data->track;
            
            /* Enable the first audio track */
            MP4SetTrackIntegerProperty(m->file, mux_data->track, "tkhd.flags", (TRACK_ENABLED | TRACK_IN_MOVIE));
        }

        else
            /* Disable the other audio tracks so QuickTime doesn't play
               them all at once. */
        {
            MP4SetTrackIntegerProperty(m->file, mux_data->track, "tkhd.flags", (TRACK_DISABLED | TRACK_IN_MOVIE));
            hb_log("Disabled extra audio track %i", mux_data->track-1);
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
	
    /* Add encoded-by metadata listing version and build date */
    char *tool_string;
    tool_string = (char *)malloc(80);
    snprintf( tool_string, 80, "HandBrake %s %i", HB_VERSION, HB_BUILD);
    MP4SetMetadataTool(m->file, tool_string);
    free(tool_string);
	
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
            struct hb_text_sample_s *sample;

            /* If this is an x264 encode with bframes the IDR frame we're
               trying to mark will be displayed offset by its renderOffset
               so we need to offset the chapter by the same amount.
               MP4 render offsets don't seem to work for text tracks so
               we have to fudge the duration instead. */
            duration = m->sum_dur - m->chapter_duration;

            if ( job->areBframes )
            {
                duration += buf->renderOffset * job->arate / 90000;
            }

            sample = MP4GenerateChapterSample( m, duration );
            
            if( !MP4WriteSample(m->file, 
                                m->chapter_track, 
                                sample->sample, 
                                sample->length, 
                                sample->duration, 
                                0, true) )
            {
                hb_error("Failed to write to output file, disk full?");
                *job->die = 1;
            }
            free(sample);
            m->current_chapter++;
            m->chapter_duration += duration;
        }
    
        /* Video */
        /* Because we use the audio samplerate as the timescale,
           we have to use potentially variable durations so the video
           doesn't go out of sync */
        int64_t bias = ( buf->start * job->arate / 90000 ) - m->sum_dur;
        duration = ( buf->stop - buf->start ) * job->arate / 90000 + bias;
        m->sum_dur += duration;
    }
    else
    {
        /* Audio */
        duration = MP4_INVALID_DURATION;
    }

    /* Here's where the sample actually gets muxed. 
       If it's an audio sample, don't offset the sample's playback.
       If it's a video sample and there are no b-frames, ditto.
       If there are b-frames, offset by the initDelay plus the
       difference between the presentation time stamp x264 gives
       and the decoding time stamp from the buffer data. */
    if( !MP4WriteSample( m->file, 
                         mux_data->track, 
                         buf->data, 
                         buf->size,
                         duration, 
                         ((mux_data->track != 1) || 
                          (job->areBframes==0) || 
                          (job->vcodec != HB_VCODEC_X264)) ? 0 : (  buf->renderOffset * job->arate / 90000),
                         ((buf->frametype & HB_FRAME_KEY) != 0) ) )
    {
        hb_error("Failed to write to output file, disk full?");   
        *job->die = 1;
    }
                                
    return 0;
}

static int MP4End( hb_mux_object_t * m )
{ 
    hb_job_t   * job   = m->job;

    /* Write our final chapter marker */
    if( m->job->chapter_markers )
    {
        struct hb_text_sample_s *sample = MP4GenerateChapterSample( m, (m->sum_dur - m->chapter_duration) );
    
        if( !MP4WriteSample(m->file, 
                            m->chapter_track, 
                            sample->sample, 
                            sample->length, 
                            sample->duration, 
                            0, true) )
        {
            hb_error("Failed to write to output file, disk full?");      
            *job->die = 1;
        }
        free(sample);
    }
    
    if (job->areBframes)
    {
           // Insert track edit to get A/V back in sync.  The edit amount is
           // the rendering offset of the first sample.
           MP4AddTrackEdit(m->file, 1, MP4_INVALID_EDIT_ID, MP4GetSampleRenderingOffset(m->file,1,1),
               MP4GetTrackDuration(m->file, 1), 0);
            if ( m->job->chapter_markers )
            {
                // apply same edit to chapter track to keep it in sync with video
                MP4AddTrackEdit(m->file, m->chapter_track, MP4_INVALID_EDIT_ID,
                                MP4GetSampleRenderingOffset(m->file,1,1),
                                MP4GetTrackDuration(m->file, m->chapter_track), 0);
            }
     }

    MP4Close( m->file );

    if ( job->mp4_optimize )
    {
        hb_log( "muxmp4: optimizing file" );
        char filename[1024]; memset( filename, 0, 1024 );
        snprintf( filename, 1024, "%s.tmp", job->file );
        MP4Optimize( job->file, filename, MP4_DETAILS_ERROR );
        remove( job->file );
        rename( filename, job->file );
    }

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

