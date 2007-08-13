/* $Id: scan.c,v 1.52 2005/11/25 15:05:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "a52dec/a52.h"
#include "dca.h"

typedef struct
{
    hb_handle_t * h;
    
    char        * path;
    int           title_index;
    hb_list_t   * list_title;
    
    hb_dvd_t    * dvd;
	hb_stream_t * stream;
	
} hb_scan_t;

static void ScanFunc( void * );
static int  DecodePreviews( hb_scan_t *, hb_title_t * title );
static void LookForAC3AndDCA( hb_title_t * title, hb_buffer_t * b );
static int  AllAC3AndDCAOK( hb_title_t * title );

hb_thread_t * hb_scan_init( hb_handle_t * handle, const char * path,
                            int title_index, hb_list_t * list_title )
{
    hb_scan_t * data = calloc( sizeof( hb_scan_t ), 1 );

    data->h            = handle;
    data->path         = strdup( path );
    data->title_index  = title_index;
    data->list_title   = list_title;

    return hb_thread_init( "scan", ScanFunc, data, HB_NORMAL_PRIORITY );
}

static void ScanFunc( void * _data )
{
    hb_scan_t  * data = (hb_scan_t *) _data;
    hb_title_t * title;
    int          i;

	data->dvd = NULL;
	data->stream = NULL;
	
    /* Try to open the path as a DVD. If it fails, try as a file */
    hb_log( "scan: trying to open with libdvdread" );
    if( ( data->dvd = hb_dvd_init( data->path ) ) )
    {
        hb_log( "scan: DVD has %d title(s)",
                hb_dvd_title_count( data->dvd ) );
        if( data->title_index )
        {
            /* Scan this title only */
            hb_list_add( data->list_title, hb_dvd_title_scan( data->dvd,
                            data->title_index ) );
        }
        else
        {
            /* Scan all titles */
            for( i = 0; i < hb_dvd_title_count( data->dvd ); i++ )
            {
                hb_list_add( data->list_title,
                             hb_dvd_title_scan( data->dvd, i + 1 ) );
            }
        }
    }
    else
    {
        if ( hb_stream_is_stream_type(data->path) )
        {
          hb_log( "scan: trying to open as MPEG-2 Stream");
		  data->stream = hb_stream_open (data->path);
          hb_list_add( data->list_title, hb_stream_title_scan( data->stream ) );
        }
        else
        {
            hb_log( "scan: unrecognized file type" );
            return;
        }
    }

    for( i = 0; i < hb_list_count( data->list_title ); )
    {
        int j;
        hb_state_t state;
        hb_audio_t * audio;
        hb_title_t * title_tmp = NULL;

        title = hb_list_item( data->list_title, i );

        /* I've seen a DVD with strictly identical titles. Check this
           here and ignore it if redundant */
        for( j = 0; j < i; j++ )
        {
            title_tmp = hb_list_item( data->list_title, j );
            if( title->vts         == title_tmp->vts &&
                title->block_start == title_tmp->block_start &&
                title->block_end   == title_tmp->block_end &&
                title->block_count == title_tmp->block_count )
            {
                break;
            }
            else
            {
                title_tmp = NULL;
            }
        }
        if( title_tmp )
        {
            hb_log( "scan: title %d is duplicate with title %d",
                    title->index, title_tmp->index );
            hb_list_rem( data->list_title, title );
            free( title );      /* This _will_ leak! */
            continue;
        }

#define p state.param.scanning
        /* Update the UI */
        state.state   = HB_STATE_SCANNING;
        p.title_cur   = title->index;
        p.title_count = data->dvd ? hb_dvd_title_count( data->dvd ) : hb_list_count(data->list_title);
        hb_set_state( data->h, &state );
#undef p

        /* Decode previews */
        /* this will also detect more AC3 / DTS information */
        if( !DecodePreviews( data, title ) )
        {
            /* TODO: free things */
            hb_list_rem( data->list_title, title );
            continue;
        }
        
		if (data->stream)
		{
			// Stream based processing uses PID's to handle the different audio options for a given title
			for( j = 0; j < hb_list_count( title->list_audio ); j++ )
			{
				audio = hb_list_item( title->list_audio, j );
				hb_stream_update_audio(data->stream, audio);
			}
		}
		else if (data->dvd)
		{
			/* Make sure we found AC3 rates and bitrates */
			for( j = 0; j < hb_list_count( title->list_audio ); )
			{
				audio = hb_list_item( title->list_audio, j );
				if( audio->codec == HB_ACODEC_AC3 &&
					!audio->bitrate )
				{
					hb_list_rem( title->list_audio, audio );
					free( audio );
					continue;
				}
				j++;
			}
		}
		
        /* Make sure we found AC3 / DCA rates and bitrates */
        for( j = 0; j < hb_list_count( title->list_audio ); )
        {
            audio = hb_list_item( title->list_audio, j );
            if( ( audio->codec == HB_ACODEC_AC3 || audio->codec == HB_ACODEC_DCA ) &&
                !audio->bitrate )
            {
                hb_log( "scan: removing audio with codec of 0x%x because of no bitrate",
                    audio->codec );
                hb_list_rem( title->list_audio, audio );
                free( audio );
                continue;
            }
            j++;
        }

        /* Do we still have audio */
        if( !hb_list_count( title->list_audio ) )
        {
            hb_list_rem( data->list_title, title );
            free( title );
            continue;
        }

        /* set a default input channel layout of stereo for LPCM or MPEG2 audio */
        /* AC3 and DCA will already have had their layout set via DecodePreviews above, */
        /* which calls LookForAC3AndDCA */
        for( j = 0; j < hb_list_count( title->list_audio ); j++ )
        {
            audio = hb_list_item( title->list_audio, j );
            if( audio->codec == HB_ACODEC_LPCM || audio->codec == HB_ACODEC_MPGA )
            {
                audio->input_channel_layout = HB_INPUT_CH_LAYOUT_STEREO;
            }
        }
        
        i++;
    }

    /* Init jobs templates */
    for( i = 0; i < hb_list_count( data->list_title ); i++ )
    {
        hb_job_t * job;

        title      = hb_list_item( data->list_title, i );
        job        = calloc( sizeof( hb_job_t ), 1 );
        title->job = job;

        job->title = title;

        /* Set defaults settings */
        job->chapter_start = 1;
        job->chapter_end   = hb_list_count( title->list_chapter );

        /* Autocrop by default. Gnark gnark */
        memcpy( job->crop, title->crop, 4 * sizeof( int ) );

        if( title->aspect == 16 )
        {
            hb_reduce( &job->pixel_aspect_width, &job->pixel_aspect_height,
                       16 * title->height, 9 * title->width );
        }
        else
        {
            hb_reduce( &job->pixel_aspect_width, &job->pixel_aspect_height,
                       4 * title->height, 3 * title->width );
        }

        job->width = title->width - job->crop[2] - job->crop[3];
//        job->height = title->height - job->crop[0] - job->crop[1];
        hb_fix_aspect( job, HB_KEEP_WIDTH );
        if( job->height > title->height - job->crop[0] - job->crop[1] )
        {
            job->height = title->height - job->crop[0] - job->crop[1];
            hb_fix_aspect( job, HB_KEEP_HEIGHT );
        }

    hb_log( "scan: title (%d) job->width:%d, job->height:%d",
            i,job->width, job->height );

        job->keep_ratio = 1;

        job->vcodec     = HB_VCODEC_FFMPEG;
        job->vquality   = -1.0;
        job->vbitrate   = 1000;
        job->pass       = 0;
        job->vrate      = title->rate;
        job->vrate_base = title->rate_base;

        job->audios[0] = 0;
        job->audios[1] = -1;

        job->acodec   = HB_ACODEC_FAAC;
        job->abitrate = 128;
        job->arate    = 44100;

        job->subtitle = -1;

        job->mux = HB_MUX_MP4;
    }

    if( data->dvd )
    {
        hb_dvd_close( &data->dvd );
    }
	if (data->stream)
	{
		hb_stream_close(&data->stream);
	}
    free( data->path );
    free( data );
    _data = NULL;
}

/***********************************************************************
 * DecodePreviews
 ***********************************************************************
 * Decode 10 pictures for the given title.
 * It assumes that data->reader and data->vts have successfully been
 * DVDOpen()ed and ifoOpen()ed.
 **********************************************************************/
static int DecodePreviews( hb_scan_t * data, hb_title_t * title )
{
    int             i, ret;
    hb_buffer_t   * buf_ps, * buf_es, * buf_raw;
    hb_list_t     * list_es, * list_raw;
    hb_libmpeg2_t * mpeg2;
    int progressive_count = 0;
    
    buf_ps   = hb_buffer_init( HB_DVD_READ_BUFFER_SIZE );
    list_es  = hb_list_init();
    list_raw = hb_list_init();

    hb_log( "scan: decoding previews for title %d", title->index );

    if (data->dvd)
      hb_dvd_start( data->dvd, title->index, 1 );
      
    for( i = 0; i < 10; i++ )
    {
        int j, k;
        FILE * file_preview;
        char   filename[1024];

        //hb_log("Seeking to: %f", (float) ( i + 1 ) / 11.0 );
       
        if (data->dvd)
        {
          if( !hb_dvd_seek( data->dvd, (float) ( i + 1 ) / 11.0 ) )
          {
              goto error;
          }
        }
        else if (data->stream)
        {
          if (!hb_stream_seek(data->stream, (float) ( i + 1 ) / 11.0 ) )
          {
            goto error;
          }
        }
        
        hb_log( "scan: preview %d", i + 1 );

        mpeg2 = hb_libmpeg2_init();

        for( j = 0; j < 10240 ; j++ )
        {
            if (data->dvd)
            {
              if( !hb_dvd_read( data->dvd, buf_ps ) )
              {
                  goto error;
              }
            }
            else if (data->stream)
            {
              if ( !hb_stream_read(data->stream,buf_ps) )
              {
                goto error;
              }
            }
            hb_demux_ps( buf_ps, list_es );

            while( ( buf_es = hb_list_item( list_es, 0 ) ) )
            {
                hb_list_rem( list_es, buf_es );
                if( buf_es->id == 0xE0 && !hb_list_count( list_raw ) )
                {
                    hb_libmpeg2_decode( mpeg2, buf_es, list_raw );
                }
                else if( !i )
                {
                    LookForAC3AndDCA( title, buf_es );
                }
                hb_buffer_close( &buf_es );

                if( hb_list_count( list_raw ) &&
                    ( i || AllAC3AndDCAOK( title ) ) )
                {
                    /* We got a picture */
                    break;
                }
            }

            if( hb_list_count( list_raw ) &&
                ( i || AllAC3AndDCAOK( title ) ) )
            {
                break;
            }
        }

        if( !hb_list_count( list_raw ) )
        {
            hb_log( "scan: could not get a decoded picture" );
            goto error;
        }

        /* Get size and rate infos */
        title->rate = 27000000;
        int ar;
        hb_libmpeg2_info( mpeg2, &title->width, &title->height,
                          &title->rate_base, &ar );
       
        if (title->rate_base == 1126125)
        {
            /* Frame FPS is 23.976 (meaning it's progressive), so
               start keeping track of how many are reporting at
               that speed. When enough show up that way, we want
               to make that the overall title FPS.
            */
            progressive_count++;

            if (progressive_count < 6)
                /* Not enough frames are reporting as progressive,
                   which means we should be conservative and use
                   29.97 as the title's FPS for now.
                */
                title->rate_base = 900900;           
            else
            {
                /* A majority of the scan frames are progressive. Make that
                    the title's FPS, and announce it once to the log.
                */
                if (progressive_count == 6)
                    hb_log("Title's mostly progressive NTSC, setting fps to 23.976");
                title->rate_base = 1126125;               
            }
        }
               
        if( i == 2) // Use the third frame's info, so as to skip opening logos
        {
            // The aspect ratio may have already been set by parsing the VOB/IFO details on a DVD, however
            // if we're working with program/transport streams that data needs to come from within the stream.
            if (title->aspect <= 0)
              title->aspect = ar;
            title->crop[0] = title->crop[1] = title->height / 2;
            title->crop[2] = title->crop[3] = title->width / 2;
        }

        hb_libmpeg2_close( &mpeg2 );

        while( ( buf_es = hb_list_item( list_es, 0 ) ) )
        {
            hb_list_rem( list_es, buf_es );
            hb_buffer_close( &buf_es );
        }

        buf_raw = hb_list_item( list_raw, 0 );

        hb_get_tempory_filename( data->h, filename, "%x%d",
                                 (intptr_t)title, i );

        file_preview = fopen( filename, "w" );
        if( file_preview )
        {
            fwrite( buf_raw->data, title->width * title->height * 3 / 2,
                    1, file_preview );
            fclose( file_preview );
        }
        else
        {
            hb_log( "scan: fopen failed (%s)", filename );
        }

#define Y    buf_raw->data
#define DARK 64
        
        /* Detect black borders */
        
        for( j = 0; j < title->width; j++ )
        {
            for( k = 0; k < title->crop[0]; k++ )
                if( Y[ k * title->width + j ] > DARK )
                {
                    title->crop[0] = k;
                    break;
                }
            for( k = 0; k < title->crop[1]; k++ )
                if( Y[ ( title->height - k - 1 ) *
                       title->width + j ] > DARK )
                {
                    title->crop[1] = k;
                    break;
                }
        }
        for( j = 0; j < title->height; j++ )
        {
            for( k = 0; k < title->crop[2]; k++ ) 
                if( Y[ j * title->width + k ] > DARK )
                {
                    title->crop[2] = k;
                    break;
                }
            for( k = 0; k < title->crop[3]; k++ )
                if( Y[ j * title->width +
                        title->width - k - 1 ] > DARK )
                {
                    title->crop[3] = k;
                    break;
                }
        }

        while( ( buf_raw = hb_list_item( list_raw, 0 ) ) )
        {
            hb_list_rem( list_raw, buf_raw );
            hb_buffer_close( &buf_raw );
        }
    }

    title->crop[0] = EVEN( title->crop[0] );
    title->crop[1] = EVEN( title->crop[1] );
    title->crop[2] = EVEN( title->crop[2] );
    title->crop[3] = EVEN( title->crop[3] );

    hb_log( "scan: %dx%d, %.3f fps, autocrop = %d/%d/%d/%d",
            title->width, title->height, (float) title->rate /
            (float) title->rate_base, title->crop[0], title->crop[1],
            title->crop[2], title->crop[3] );

    ret = 1;
    goto cleanup;

error:
    ret = 0;

cleanup:
    hb_buffer_close( &buf_ps );
    while( ( buf_es = hb_list_item( list_es, 0 ) ) )
    {
        hb_list_rem( list_es, buf_es );
        hb_buffer_close( &buf_es );
    }
    hb_list_close( &list_es );
    while( ( buf_raw = hb_list_item( list_raw, 0 ) ) )
    {
        hb_list_rem( list_raw, buf_raw );
        hb_buffer_close( &buf_raw );
    }
    hb_list_close( &list_raw );
    if (data->dvd)
      hb_dvd_stop( data->dvd );

    return ret;
}

static void LookForAC3AndDCA( hb_title_t * title, hb_buffer_t * b ) 
{
    int i;
    int flags;
    int rate;
    int bitrate;
    int frame_length;
    dca_state_t * state;

    /* Figure out if this is a AC3 or DCA buffer for a known track */
    hb_audio_t * audio = NULL;
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        /* check if we have an AC3 or DCA which we recognise */
        if( ( audio->codec == HB_ACODEC_AC3 || audio->codec == HB_ACODEC_DCA ) &&
            audio->id    == b->id )
        {
            break;
        }
        else
        {
            audio = NULL;
        }
    }
    if( !audio )
    {
        return;
    }

    if( audio->bitrate )
    {
        /* Already done for this track */
        return;
    }

    for( i = 0; i < b->size - 7; i++ )
    {

        if ( audio->codec == HB_ACODEC_AC3 )
        {

            /* check for a52 */
            if( a52_syncinfo( &b->data[i], &flags, &rate, &bitrate ) )
            {
                hb_log( "scan: AC3, rate=%dHz, bitrate=%d", rate, bitrate );
                audio->rate    = rate;
                audio->bitrate = bitrate;
                switch( flags & A52_CHANNEL_MASK )
                {
                    /* mono sources */
                    case A52_MONO:
                    case A52_CHANNEL1:
                    case A52_CHANNEL2:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_MONO;
                        break;
                    /* stereo input */
                    case A52_CHANNEL:
                    case A52_STEREO:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_STEREO;
                        break;
                    /* dolby (DPL1 aka Dolby Surround = 4.0 matrix-encoded) input */
                    case A52_DOLBY:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_DOLBY;
                        break;
                    /* 3F/2R input */
                    case A52_3F2R:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_3F2R;
                        break;
                    /* 3F/1R input */
                    case A52_3F1R:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_3F1R;
                        break;
                    /* other inputs */
                    case A52_3F:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_3F;
                        break;
                    case A52_2F1R:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_2F1R;
                        break;
                    case A52_2F2R:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_2F2R;
                        break;
                    /* unknown */
                    default:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_STEREO;
                }
                
                /* add in our own LFE flag if the source has LFE */
                if (flags & A52_LFE)
                {
                    audio->input_channel_layout = audio->input_channel_layout | HB_INPUT_CH_LAYOUT_HAS_LFE;
                }

                /* store the AC3 flags for future reference
                This enables us to find out if we had a stereo or Dolby source later on */
                audio->config.a52.ac3flags = flags;

                /* store the ac3 flags in the public ac3flags property too, so we can access it from the GUI */
                audio->ac3flags = audio->config.a52.ac3flags;

                /* XXX */
                if ( (flags & A52_CHANNEL_MASK) == A52_DOLBY ) {
                    sprintf( audio->lang + strlen( audio->lang ),
                         " (Dolby Surround)" );
                } else {
                    sprintf( audio->lang + strlen( audio->lang ),
                         " (%d.%d ch)",
                        HB_INPUT_CH_LAYOUT_GET_DISCRETE_FRONT_COUNT(audio->input_channel_layout) +
                        HB_INPUT_CH_LAYOUT_GET_DISCRETE_REAR_COUNT(audio->input_channel_layout),
                        HB_INPUT_CH_LAYOUT_GET_DISCRETE_LFE_COUNT(audio->input_channel_layout));
                }

                break;
            
            }

        }
        else if ( audio->codec == HB_ACODEC_DCA )
        {

            hb_log( "scan: checking for DCA syncinfo" );

            /* check for dca */
            state = dca_init( 0 );
            if( dca_syncinfo( state, &b->data[i], &flags, &rate, &bitrate, &frame_length ) )
            {
                hb_log( "scan: DCA, rate=%dHz, bitrate=%d", rate, bitrate );
                audio->rate    = rate;
                audio->bitrate = bitrate;
                switch( flags & DCA_CHANNEL_MASK )
                {
                    /* mono sources */
                    case DCA_MONO:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_MONO;
                        break;
                    /* stereo input */
                    case DCA_CHANNEL:
                    case DCA_STEREO:
                    case DCA_STEREO_SUMDIFF:
                    case DCA_STEREO_TOTAL:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_STEREO;
                        break;
                    /* 3F/2R input */
                    case DCA_3F2R:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_3F2R;
                        break;
                    /* 3F/1R input */
                    case DCA_3F1R:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_3F1R;
                        break;
                    /* other inputs */
                    case DCA_3F:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_3F;
                        break;
                    case DCA_2F1R:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_2F1R;
                        break;
                    case DCA_2F2R:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_2F2R;
                        break;
                    case DCA_4F2R:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_4F2R;
                        break;
                    /* unknown */
                    default:
                        audio->input_channel_layout = HB_INPUT_CH_LAYOUT_STEREO;
                }

                /* add in our own LFE flag if the source has LFE */
                if (flags & DCA_LFE)
                {
                    audio->input_channel_layout = audio->input_channel_layout | HB_INPUT_CH_LAYOUT_HAS_LFE;
                }

                /* store the DCA flags for future reference
                This enables us to find out if we had a stereo or Dolby source later on */
                audio->config.dca.dcaflags = flags;

                /* store the dca flags in the public dcaflags property too, so we can access it from the GUI */
                audio->dcaflags = audio->config.dca.dcaflags;

                /* XXX */
                if ( (flags & DCA_CHANNEL_MASK) == DCA_DOLBY ) {
                    sprintf( audio->lang + strlen( audio->lang ),
                         " (Dolby Surround)" );
                } else {
                    sprintf( audio->lang + strlen( audio->lang ),
                         " (%d.%d ch)",
                        HB_INPUT_CH_LAYOUT_GET_DISCRETE_FRONT_COUNT(audio->input_channel_layout) +
                        HB_INPUT_CH_LAYOUT_GET_DISCRETE_REAR_COUNT(audio->input_channel_layout),
                        HB_INPUT_CH_LAYOUT_GET_DISCRETE_LFE_COUNT(audio->input_channel_layout));
                }

                break;
            }
        }
    }

}

static int  AllAC3AndDCAOK( hb_title_t * title )
{
    int i;
    hb_audio_t * audio;

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        if( ( audio->codec == HB_ACODEC_AC3 || audio->codec == HB_ACODEC_DCA ) &&
            !audio->bitrate )
        {
            return 0;
        }
    }

    return 1;
}
