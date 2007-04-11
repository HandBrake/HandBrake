/* $Id: scan.c,v 1.52 2005/11/25 15:05:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "a52dec/a52.h"

typedef struct
{
    hb_handle_t * h;
    
    char        * path;
    int           title_index;
    hb_list_t   * list_title;
    
    hb_dvd_t    * dvd;

} hb_scan_t;

static void ScanFunc( void * );
static int  DecodePreviews( hb_scan_t *, hb_title_t * title );
static void LookForAC3( hb_title_t * title, hb_buffer_t * b );
static int  AllAC3OK( hb_title_t * title );

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
    	/* Open as a VOB file */
        FILE * file;
        hb_log( "scan: trying to open as VOB file" );
        file = fopen( data->path, "rb" );
        if( file )
        {
            /* XXX */
            fclose( file );
        }
        else
        {
            hb_log( "scan: fopen failed" );
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
        p.title_count = hb_dvd_title_count( data->dvd );
        hb_set_state( data->h, &state );
#undef p

        /* Decode previews */
        if( !DecodePreviews( data, title ) )
        {
            /* TODO: free things */
            hb_list_rem( data->list_title, title );
            continue;
        }

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

        /* Do we still have audio */
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

    buf_ps   = hb_buffer_init( 2048 );
    list_es  = hb_list_init();
    list_raw = hb_list_init();

    hb_log( "scan: decoding previews for title %d", title->index );

    hb_dvd_start( data->dvd, title->index, 1 );

    for( i = 0; i < 10; i++ )
    {
        int j, k;
        FILE * file_preview;
        char   filename[1024];

        if( !hb_dvd_seek( data->dvd, (float) ( i + 1 ) / 11.0 ) )
        {
            goto error;
        }

        hb_log( "scan: preview %d", i + 1 );

        mpeg2 = hb_libmpeg2_init();

        for( j = 0; j < 10240 ; j++ )
        {
            if( !hb_dvd_read( data->dvd, buf_ps ) )
            {
                goto error;
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
                    LookForAC3( title, buf_es );
                }
                hb_buffer_close( &buf_es );

                if( hb_list_count( list_raw ) &&
                    ( i || AllAC3OK( title ) ) )
                {
                    /* We got a picture */
                    break;
                }
            }

            if( hb_list_count( list_raw ) &&
                ( i || AllAC3OK( title ) ) )
            {
                break;
            }
        }

        if( !hb_list_count( list_raw ) )
        {
            hb_log( "scan: could not get a decoded picture" );
            goto error;
        }

        if( !i )
        {
            /* Get size and rate infos */
            title->rate = 27000000;
            hb_libmpeg2_info( mpeg2, &title->width, &title->height,
                              &title->rate_base );
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
                                 (int) title, i );

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
    hb_dvd_stop( data->dvd );
    return ret;
}

static void LookForAC3( hb_title_t * title, hb_buffer_t * b ) 
{
    int i;
    int flags;
    int rate;
    int bitrate;

    /* Figure out if this is a AC3 buffer for a known track */
    hb_audio_t * audio = NULL;
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        if( audio->codec == HB_ACODEC_AC3 &&
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
        if( a52_syncinfo( &b->data[i], &flags, &rate, &bitrate ) )
        {
            hb_log( "scan: rate=%dHz, bitrate=%d", rate, bitrate );
            audio->rate    = rate;
            audio->bitrate = bitrate;
            switch( flags & A52_CHANNEL_MASK )
            {
                case A52_MONO:
                case A52_CHANNEL1:
                case A52_CHANNEL2:
                    audio->src_discrete_front_channels = 1;
                    audio->src_discrete_rear_channels = 0;
                    audio->src_encoded_front_channels = 1;
                    audio->src_encoded_rear_channels = 0;
                    break;
                case A52_STEREO:
                case A52_CHANNEL:
                    audio->src_discrete_front_channels = 2;
                    audio->src_discrete_rear_channels = 0;
                    audio->src_encoded_front_channels = 2;
                    audio->src_encoded_rear_channels = 0;
                    break;
                case A52_DOLBY:
                    audio->src_discrete_front_channels = 2;
                    audio->src_discrete_rear_channels = 0;
                    audio->src_encoded_front_channels = 3;
                    audio->src_encoded_rear_channels = 1;
                    break;
                case A52_3F:
                    audio->src_discrete_front_channels = 3;
                    audio->src_discrete_rear_channels = 0;
                    audio->src_encoded_front_channels = 3;
                    audio->src_encoded_rear_channels = 0;
                    break;
                case A52_2F1R:
                    audio->src_discrete_front_channels = 2;
                    audio->src_discrete_rear_channels = 1;
                    audio->src_encoded_front_channels = 2;
                    audio->src_encoded_rear_channels = 1;
                    break;
                case A52_3F1R:
                    audio->src_discrete_front_channels = 3;
                    audio->src_discrete_rear_channels = 1;
                    audio->src_encoded_front_channels = 3;
                    audio->src_encoded_rear_channels = 1;
                    break;
				case A52_2F2R:
                    audio->src_discrete_front_channels = 2;
                    audio->src_discrete_rear_channels = 2;
                    audio->src_encoded_front_channels = 2;
                    audio->src_encoded_rear_channels = 2;
                    break;
                case A52_3F2R:
                    audio->src_discrete_front_channels = 3;
                    audio->src_discrete_rear_channels = 2;
                    audio->src_encoded_front_channels = 3;
                    audio->src_encoded_rear_channels = 2;
                    break;
            }

			if (flags & A52_LFE) {
                audio->src_discrete_lfe_channels = 1;
			} else {
                audio->src_discrete_lfe_channels = 0;
			}
			
			/* store the AC3 FLAGS for future reference
			This enables us to find out if we had a stereo or Dolby source later on */
			audio->config.a52.ac3flags = flags;

            /* XXX */
			if ( (flags & A52_CHANNEL_MASK) == A52_DOLBY ) {
				sprintf( audio->lang + strlen( audio->lang ),
                     " (Dolby Surround)" );
			} else {
				sprintf( audio->lang + strlen( audio->lang ),
                     " (%d.%d ch)",
					 audio->src_discrete_front_channels + audio->src_discrete_rear_channels, audio->src_discrete_lfe_channels );
			}

            break;
        }
    }
}

static int  AllAC3OK( hb_title_t * title )
{
    int i;
    hb_audio_t * audio;

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        if( audio->codec == HB_ACODEC_AC3 &&
            !audio->bitrate )
        {
            return 0;
        }
    }

    return 1;
}
