/* $Id: scan.c,v 1.52 2005/11/25 15:05:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
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
static void LookForAudio( hb_title_t * title, hb_buffer_t * b );
static int  AllAudioOK( hb_title_t * title );

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
    else if ( (data->stream = hb_stream_open( data->path, 0 ) ) != NULL )
    {
        hb_list_add( data->list_title, hb_stream_title_scan( data->stream ) );
    }
    else
    {
        hb_log( "scan: unrecognized file type" );
        return;
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

        /* Make sure we found audio rates and bitrates */
        for( j = 0; j < hb_list_count( title->list_audio ); )
        {
            audio = hb_list_item( title->list_audio, j );
            if( !audio->config.in.bitrate )
            {
                hb_log( "scan: removing audio 0x%x because no bitrate found",
                        audio->id );
                hb_list_rem( title->list_audio, audio );
                free( audio );
                continue;
            }
            j++;
        }

        /* If we don't have any audio streams left, remove the title */
        if( !hb_list_count( title->list_audio ) )
        {
            hb_list_rem( data->list_title, title );
            free( title );
            continue;
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

        /* Preserve a source's pixel aspect, if it's available. */
        if( title->pixel_aspect_width && title->pixel_aspect_height )
        {
            job->pixel_aspect_width  = title->pixel_aspect_width;
            job->pixel_aspect_height = title->pixel_aspect_height;
        }

        if( title->aspect == 16 && !job->pixel_aspect_width && !job->pixel_aspect_height)
        {
            hb_reduce( &job->pixel_aspect_width, &job->pixel_aspect_height,
                       16 * title->height, 9 * title->width );
        }
        else if( !job->pixel_aspect_width && !job->pixel_aspect_height )
        {
            hb_reduce( &job->pixel_aspect_width, &job->pixel_aspect_height,
                       4 * title->height, 3 * title->width );
        }

        job->width = title->width - job->crop[2] - job->crop[3];
        hb_fix_aspect( job, HB_KEEP_WIDTH );
        if( job->height > title->height - job->crop[0] - job->crop[1] )
        {
            job->height = title->height - job->crop[0] - job->crop[1];
            hb_fix_aspect( job, HB_KEEP_HEIGHT );
        }

        hb_log( "scan: title (%d) job->width:%d, job->height:%d",
                i, job->width, job->height );

        job->keep_ratio = 1;

        job->vcodec     = HB_VCODEC_FFMPEG;
        job->vquality   = -1.0;
        job->vbitrate   = 1000;
        job->pass       = 0;
        job->vrate      = title->rate;
        job->vrate_base = title->rate_base;

        job->list_audio = hb_list_init();

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
    int             i, npreviews = 0;
    hb_buffer_t   * buf_ps, * buf_es;
    hb_list_t     * list_es;
    int progressive_count = 0;
    int interlaced_preview_count = 0;
    double last_ar = 0;
    int ar16_count = 0, ar4_count = 0;

    buf_ps   = hb_buffer_init( HB_DVD_READ_BUFFER_SIZE );
    list_es  = hb_list_init();

    hb_log( "scan: decoding previews for title %d", title->index );

    if (data->dvd)
      hb_dvd_start( data->dvd, title->index, 1 );

    for( i = 0; i < 10; i++ )
    {
        int j, k;
        FILE * file_preview;
        char   filename[1024];

        if (data->dvd)
        {
          if( !hb_dvd_seek( data->dvd, (float) ( i + 1 ) / 11.0 ) )
          {
              continue;
          }
        }
        else if (data->stream)
        {
          /* we start reading streams at zero rather than 1/11 because
           * short streams may have only one sequence header in the entire
           * file and we need it to decode any previews. */
          if (!hb_stream_seek(data->stream, (float) i / 11.0 ) )
          {
              continue;
          }
        }

        hb_log( "scan: preview %d", i + 1 );

        int vcodec = title->video_codec? title->video_codec : WORK_DECMPEG2;
        hb_work_object_t *vid_decoder = hb_get_work( vcodec );
        vid_decoder->codec_param = title->video_codec_param;
        vid_decoder->init( vid_decoder, NULL );
        hb_buffer_t * vid_buf = NULL;

        for( j = 0; j < 10240 ; j++ )
        {
            if (data->dvd)
            {
              if( !hb_dvd_read( data->dvd, buf_ps ) )
              {
                  hb_log( "Warning: Could not read data for preview %d, skipped", i + 1 );
                  goto skip_preview;
              }
            }
            else if (data->stream)
            {
              if ( !hb_stream_read(data->stream,buf_ps) )
              {
                  hb_log( "Warning: Could not read data for preview %d, skipped", i + 1 );
                  goto skip_preview;
              }
            }
            if ( title->demuxer == HB_NULL_DEMUXER )
            {
                hb_demux_null( buf_ps, list_es, 0 );
            }
            else
            {
                hb_demux_ps( buf_ps, list_es, 0 );
            }

            while( ( buf_es = hb_list_item( list_es, 0 ) ) )
            {
                hb_list_rem( list_es, buf_es );
                if( buf_es->id == title->video_id && vid_buf == NULL )
                {
                    vid_decoder->work( vid_decoder, &buf_es, &vid_buf );
                }
                else if( ! AllAudioOK( title ) )
                {
                    LookForAudio( title, buf_es );
                }
                if ( buf_es )
                    hb_buffer_close( &buf_es );
            }

            if( vid_buf && AllAudioOK( title ) )
                break;
        }

        if( ! vid_buf )
        {
            hb_log( "scan: could not get a decoded picture" );
            continue;
        }

        /* Get size and rate infos */

        hb_work_info_t vid_info;
        vid_decoder->info( vid_decoder, &vid_info );
        vid_decoder->close( vid_decoder );
        free( vid_decoder );

        title->width = vid_info.width;
        title->height = vid_info.height;
        title->pixel_aspect_width = vid_info.pixel_aspect_width;
        title->pixel_aspect_height = vid_info.pixel_aspect_height;
        
        title->rate = vid_info.rate;
        title->rate_base = vid_info.rate_base;
        if ( vid_info.aspect != 0 )
        {
            if ( vid_info.aspect != last_ar && last_ar != 0 )
            {
                hb_log( "aspect ratio changed from %g to %g",
                        last_ar, vid_info.aspect );
            }
            
            if( !title->pixel_aspect_width && !title->pixel_aspect_height )
            {
                /* We don't have pixel aspect info from the source, so we're
                   going to have to make a guess on the display aspect ratio. */
                switch ( (int)vid_info.aspect )
                {
                    case HB_ASPECT_BASE * 4 / 3:
                        ++ar4_count;
                        break;
                    case HB_ASPECT_BASE * 16 / 9:
                        ++ar16_count;
                        break;
                    default:
                        hb_log( "unknown aspect ratio %g", vid_info.aspect );
                        /* if the aspect is closer to 4:3 use that
                         * otherwise use 16:9 */
                        vid_info.aspect < HB_ASPECT_BASE * 14 / 9 ? ++ar4_count :
                                                                    ++ar16_count;
                        break;
                }
            }
            last_ar = vid_info.aspect;
        }

        if( title->rate_base == 1126125 )
        {
            /* Frame FPS is 23.976 (meaning it's progressive), so
               start keeping track of how many are reporting at
               that speed. When enough show up that way, we want
               to make that the overall title FPS.
            */
            progressive_count++;

            if( progressive_count < 6 )
            {
                /* Not enough frames are reporting as progressive,
                   which means we should be conservative and use
                   29.97 as the title's FPS for now.
                */
                title->rate_base = 900900;
            }
            else
            {
                /* A majority of the scan frames are progressive. Make that
                    the title's FPS, and announce it once to the log.
                */
                if( progressive_count == 6 )
                {
                    hb_log("Title's mostly NTSC Film, setting fps to 23.976");
                }
                title->rate_base = 1126125;
            }
        }
        else if( title->rate_base == 900900 && progressive_count >= 6 )
        {
            /*
             * We've already deduced that the frame rate is 23.976, so set it
             * back again.
             */
            title->rate_base = 1126125;
        }

        // start from third frame to skip opening logos
        if( i == 2)
        {
            title->crop[0] = title->crop[1] = title->height / 2;
            title->crop[2] = title->crop[3] = title->width / 2;
        }

        while( ( buf_es = hb_list_item( list_es, 0 ) ) )
        {
            hb_list_rem( list_es, buf_es );
            hb_buffer_close( &buf_es );
        }

        /* Check preview for interlacing artifacts */
        if( hb_detect_comb( vid_buf, title->width, title->height, 10, 30, 9, 10, 30, 9 ) )
        {
            hb_log("Interlacing detected in preview frame %i", i+1);
            interlaced_preview_count++;
        }

        hb_get_tempory_filename( data->h, filename, "%x%d",
                                 (intptr_t)title, i );

        file_preview = fopen( filename, "w" );
        if( file_preview )
        {
            fwrite( vid_buf->data, title->width * title->height * 3 / 2,
                    1, file_preview );
            fclose( file_preview );
        }
        else
        {
            hb_log( "scan: fopen failed (%s)", filename );
        }

#define Y    vid_buf->data
#define DARK 64

        /* Detect black borders */

        for( j = 0; j < title->width; j++ )
        {
            for( k = 2; k < title->crop[0]; k++ )
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
        ++npreviews;

skip_preview:
        if ( vid_buf )
            hb_buffer_close( &vid_buf );
    }
    
    if( title->pixel_aspect_width && title->pixel_aspect_width )
    {
        title->aspect = ( (double)title->pixel_aspect_width * 
                         (double)title->width /
                         (double)title->pixel_aspect_height /
                         (double)title->height + 0.05 ) * HB_ASPECT_BASE;
    }
    else
    {
        /* if we found mostly 4:3 previews use that as the aspect ratio otherwise
           use 16:9 */
        title->aspect = ar4_count > ar16_count ?
                            HB_ASPECT_BASE * 4 / 3 : HB_ASPECT_BASE * 16 / 9;
    }

    title->crop[0] = EVEN( title->crop[0] );
    title->crop[1] = EVEN( title->crop[1] );
    title->crop[2] = EVEN( title->crop[2] );
    title->crop[3] = EVEN( title->crop[3] );

    hb_log( "scan: %d previews, %dx%d, %.3f fps, autocrop = %d/%d/%d/%d, aspect %.2f",
            npreviews, title->width, title->height, (float) title->rate /
            (float) title->rate_base, title->crop[0], title->crop[1],
            title->crop[2], title->crop[3], (float)title->aspect/(float)HB_ASPECT_BASE);

    if( interlaced_preview_count >= ( npreviews / 2 ) )
    {
        hb_log("Title is likely interlaced or telecined (%i out of %i previews). You should do something about that.",
               interlaced_preview_count, npreviews);
        title->detected_interlacing = 1;
    }
    else
    {
        title->detected_interlacing = 0;
    }

    hb_buffer_close( &buf_ps );
    while( ( buf_es = hb_list_item( list_es, 0 ) ) )
    {
        hb_list_rem( list_es, buf_es );
        hb_buffer_close( &buf_es );
    }
    hb_list_close( &list_es );
    if (data->dvd)
      hb_dvd_stop( data->dvd );

    return npreviews;
}

/*
 * This routine is called for every frame from a non-video elementary stream.
 * These are a mix of audio & subtitle streams, some of which we want & some
 * we're ignoring. This routine checks the frame against all our audio streams
 * to see if it's one we want and haven't identified yet. If yes, it passes the
 * frame to a codec-specific id routine which is responsible for filling in
 * the sample rate, bit rate, channels & other audio parameters.
 *
 * Since a sample rate is essential for further audio processing, any audio
 * stream which isn't successfully id'd by is deleted at the end of the scan.
 * This is necessary to avoid ambiguities where things that might be audio
 * aren't (e.g., some European DVD Teletext streams use the same IDs as US ATSC
 * AC-3 audio).
 */
static void LookForAudio( hb_title_t * title, hb_buffer_t * b )
{
    int i;

    hb_audio_t * audio = NULL;
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        /* check if this elementary stream is one we want */
        if ( audio->id == b->id )
        {
            break;
        }
        else
        {
            audio = NULL;
        }
    }
    if( !audio || audio->config.in.bitrate != 0 )
    {
        /* not found or already done */
        return;
    }

    hb_work_object_t *w = hb_codec_decoder( audio->config.in.codec );

    if ( w == NULL || w->bsinfo == NULL )
    {
        hb_log( "Internal error in scan: unhandled audio type %d for id 0x%x",
                audio->config.in.codec, audio->id );
        goto drop_audio;
    }

    hb_work_info_t info;
    w->audio = audio;
    w->codec_param = audio->config.in.codec_param;
    int ret = w->bsinfo( w, b, &info );
    if ( ret < 0 )
    {
        hb_log( "no info on audio type %d/0x%x for id 0x%x",
                audio->config.in.codec, audio->config.in.codec_param,
                audio->id );
        goto drop_audio;
    }
    if ( !info.bitrate )
    {
        /* didn't find any info */
        return;
    }
    audio->config.in.samplerate = info.rate;
    audio->config.in.bitrate = info.bitrate;
    audio->config.in.channel_layout = info.channel_layout;
    audio->config.flags.ac3 = info.flags;

    // update the audio description string based on the info we found
    if ( audio->config.flags.ac3 & AUDIO_F_DOLBY )
    {
        strcat( audio->config.lang.description, " (Dolby Surround)" );
    }
    else
    {
        int layout = audio->config.in.channel_layout;
        char *desc = audio->config.lang.description +
                        strlen( audio->config.lang.description );
        sprintf( desc, " (%d.%d ch)",
                 HB_INPUT_CH_LAYOUT_GET_DISCRETE_FRONT_COUNT(layout) +
                     HB_INPUT_CH_LAYOUT_GET_DISCRETE_REAR_COUNT(layout),
                 HB_INPUT_CH_LAYOUT_GET_DISCRETE_LFE_COUNT(layout) );
    }

    hb_log( "scan: audio 0x%x: %s, rate=%dHz, bitrate=%d %s", audio->id,
            info.name, audio->config.in.samplerate, audio->config.in.bitrate,
            audio->config.lang.description );
 
    free( w );
    return;

    // We get here if there's no hope of finding info on an audio bitstream,
    // either because we don't have a decoder (or a decoder with a bitstream
    // info proc) or because the decoder's info proc said that the stream
    // wasn't something it could handle. Delete the item from the title's
    // audio list so we won't keep reading packets while trying to get its
    // bitstream info.
 drop_audio:
    if ( w )
        free( w );

    hb_list_rem( title->list_audio, audio );
}

/*
 * This routine checks to see if we've ID'd all the audio streams associated
 * with a title. It returns 0 if there are more to ID & 1 if all are done.
 */
static int  AllAudioOK( hb_title_t * title )
{
    int i;
    hb_audio_t * audio;

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        if( audio->config.in.bitrate == 0 )
        {
            return 0;
        }
    }
    return 1;
}
