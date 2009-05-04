/* $Id: scan.c,v 1.52 2005/11/25 15:05:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "a52dec/a52.h"
#include "dca.h"

#define HB_MAX_PREVIEWS 30 // 30 previews = every 5 minutes of a 2.5 hour video

typedef struct
{
    hb_handle_t * h;

    char        * path;
    int           title_index;
    hb_list_t   * list_title;

    hb_dvd_t    * dvd;
	hb_stream_t * stream;
	
    int           preview_count;
    int           store_previews;

} hb_scan_t;

static void ScanFunc( void * );
static int  DecodePreviews( hb_scan_t *, hb_title_t * title );
static void LookForAudio( hb_title_t * title, hb_buffer_t * b );
static int  AllAudioOK( hb_title_t * title );

static const char *aspect_to_string( double aspect )
{
    switch ( (int)(aspect * 9.) )
    {
        case 9 * 4 / 3:    return "4:3";
        case 9 * 16 / 9:   return "16:9";
    }
    static char arstr[32];
    sprintf( arstr, aspect >= 1.? "%.2f:1" : "1:%.2f", aspect );
    return arstr;
}

hb_thread_t * hb_scan_init( hb_handle_t * handle, const char * path,
                            int title_index, hb_list_t * list_title,
                            int preview_count, int store_previews )
{
    hb_scan_t * data = calloc( sizeof( hb_scan_t ), 1 );

    data->h            = handle;
    data->path         = strdup( path );
    data->title_index  = title_index;
    data->list_title   = list_title;
    
    data->preview_count  = preview_count;
    data->store_previews = store_previews;
    
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
            job->anamorphic.par_width  = title->pixel_aspect_width;
            job->anamorphic.par_height = title->pixel_aspect_height;
        }

        if( title->aspect != 0 && title->aspect != 1. &&
            !job->anamorphic.par_width && !job->anamorphic.par_height)
        {
            hb_reduce( &job->anamorphic.par_width, &job->anamorphic.par_height,
                       (int)(title->aspect * title->height + 0.5), title->width );
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
        job->list_subtitle = hb_list_init();

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

// -----------------------------------------------
// stuff related to cropping

#define DARK 32

static inline int absdiff( int x, int y )
{
    return x < y ? y - x : x - y;
}

static inline int clampBlack( int x ) 
{
    // luma 'black' is 16 and anything less should be clamped at 16
    return x < 16 ? 16 : x;
}

static int row_all_dark( hb_title_t *title, uint8_t* luma, int row )
{
    luma += title->width * row;

    // compute the average luma value of the row
    int i, avg = 0;
    for ( i = 0; i < title->width; ++i )
    {
        avg += clampBlack( luma[i] );
    }
    avg /= title->width;
    if ( avg >= DARK )
        return 0;

    // since we're trying to detect smooth borders, only take the row if
    // all pixels are within +-16 of the average (this range is fairly coarse
    // but there's a lot of quantization noise for luma values near black
    // so anything less will fail to crop because of the noise).
    for ( i = 0; i < title->width; ++i )
    {
        if ( absdiff( avg, clampBlack( luma[i] ) ) > 16 )
            return 0;
    }
    return 1;
}

static int column_all_dark( hb_title_t *title, uint8_t* luma, int top, int bottom,
                            int col )
{
    int stride = title->width;
    int height = title->height - top - bottom;
    luma += stride * top + col;

    // compute the average value of the column
    int i = height, avg = 0, row = 0;
    for ( ; --i >= 0; row += stride )
    {
        avg += clampBlack( luma[row] );
    }
    avg /= height;
    if ( avg >= DARK )
        return 0;

    // since we're trying to detect smooth borders, only take the column if
    // all pixels are within +-16 of the average.
    i = height, row = 0;
    for ( ; --i >= 0; row += stride )
    {
        if ( absdiff( avg, clampBlack( luma[row] ) ) > 16 )
            return 0;
    }
    return 1;
}
#undef DARK

typedef struct {
    int n;
    int t[HB_MAX_PREVIEWS];
    int b[HB_MAX_PREVIEWS];
    int l[HB_MAX_PREVIEWS];
    int r[HB_MAX_PREVIEWS];
} crop_record_t;

static void record_crop( crop_record_t *crops, int t, int b, int l, int r )
{
    crops->t[crops->n] = t;
    crops->b[crops->n] = b;
    crops->l[crops->n] = l;
    crops->r[crops->n] = r;
    ++crops->n;
}

static int compare_int( const void *a, const void *b )
{
    return *(const int *)a - *(const int *)b;
}

static void sort_crops( crop_record_t *crops )
{
    qsort( crops->t, crops->n, sizeof(crops->t[0]), compare_int );
    qsort( crops->b, crops->n, sizeof(crops->t[0]), compare_int );
    qsort( crops->l, crops->n, sizeof(crops->t[0]), compare_int );
    qsort( crops->r, crops->n, sizeof(crops->t[0]), compare_int );
}

// -----------------------------------------------
// stuff related to title width/height/aspect info

typedef struct {
    int count;              /* number of times we've seen this info entry */
    hb_work_info_t info;    /* copy of info entry */
} info_list_t;

static void remember_info( info_list_t *info_list, hb_work_info_t *info )
{
    for ( ; info_list->count; ++info_list )
    {
        if ( memcmp( &info_list->info, info, sizeof(*info) ) == 0 )
        {
            // we found a match - bump its count
            ++info_list->count;
            return;
        }
    }
    // no match found - add new entry to list (info_list points to
    // the first free slot). NB - we assume that info_list was allocated
    // so that it's big enough even if there are no dups. I.e., 10 slots
    // allocated if there are 10 previews.
    info_list->count = 1;
    info_list->info = *info;
}

static void most_common_info( info_list_t *info_list, hb_work_info_t *info )
{
    int i, biggest = 0;
    for ( i = 1; info_list[i].count; ++i )
    {
        if ( info_list[i].count > info_list[biggest].count )
            biggest = i;
    }
    *info = info_list[biggest].info;
    free( info_list );
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
    info_list_t * info_list = calloc( data->preview_count+1, sizeof(*info_list) );
    crop_record_t *crops = calloc( 1, sizeof(*crops) );

    buf_ps   = hb_buffer_init( HB_DVD_READ_BUFFER_SIZE );
    list_es  = hb_list_init();

    hb_log( "scan: decoding previews for title %d", title->index );

    if (data->dvd)
    {
      hb_dvd_start( data->dvd, title, 1 );
      title->angle_count = hb_dvd_angle_count( data->dvd );
      hb_log( "scan: title angle(s) %d", title->angle_count );
    }

    for( i = 0; i < data->preview_count; i++ )
    {
        int j;
        FILE * file_preview;
        char   filename[1024];

        if (data->dvd)
        {
          if( !hb_dvd_seek( data->dvd, (float) ( i + 1 ) / ( data->preview_count + 1.0 ) ) )
          {
              continue;
          }
        }
        else if (data->stream)
        {
          /* we start reading streams at zero rather than 1/11 because
           * short streams may have only one sequence header in the entire
           * file and we need it to decode any previews. */
          if (!hb_stream_seek(data->stream, (float) i / ( data->preview_count + 1.0 ) ) )
          {
              continue;
          }
        }

        hb_deep_log( 2, "scan: preview %d", i + 1 );

        int vcodec = title->video_codec? title->video_codec : WORK_DECMPEG2;
        hb_work_object_t *vid_decoder = hb_get_work( vcodec );
        vid_decoder->codec_param = title->video_codec_param;
        vid_decoder->title = title;
        vid_decoder->init( vid_decoder, NULL );
        hb_buffer_t * vid_buf = NULL;
        int vidskip = 0;

        if ( title->flags & HBTF_NO_IDR )
        {
            // title doesn't have IDR frames so we decode but drop one second's
            // worth of frames to allow the decoder to converge.
            if ( ! title->rate_base )
            {
                vidskip = 30;
            }
            else
            {
                vidskip = (double)title->rate / (double)title->rate_base + 0.5;
            }
        }

        for( j = 0; j < 10240 ; j++ )
        {
            if (data->dvd)
            {
              if( !hb_dvd_read( data->dvd, buf_ps ) )
              {
                  if ( vid_buf )
                  {
                    break;
                  }
                  hb_log( "Warning: Could not read data for preview %d, skipped", i + 1 );
                  goto skip_preview;
              }
            }
            else if (data->stream)
            {
              if ( !hb_stream_read(data->stream,buf_ps) )
              {
                  if ( vid_buf )
                  {
                    break;
                  }
                  hb_log( "Warning: Could not read data for preview %d, skipped", i + 1 );
                  goto skip_preview;
              }
            }
            (hb_demux[title->demuxer])(buf_ps, list_es, 0 );

            while( ( buf_es = hb_list_item( list_es, 0 ) ) )
            {
                hb_list_rem( list_es, buf_es );
                if( buf_es->id == title->video_id && vid_buf == NULL )
                {
                    vid_decoder->work( vid_decoder, &buf_es, &vid_buf );
                    if ( vid_buf && vidskip && --vidskip > 0 )
                    {
                        // we're dropping frames to get the video decoder in sync
                        // when the video stream doesn't contain IDR frames
                        hb_buffer_close( &vid_buf );
                        vid_buf = NULL;
                    }
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
        if( !vid_decoder->info( vid_decoder, &vid_info ) )
        {
            /*
             * Could not fill vid_info, don't continue and try to use vid_info
             * in this case.
             */
            vid_decoder->close( vid_decoder );
            free( vid_decoder );
            continue;
        }
        vid_decoder->close( vid_decoder );
        free( vid_decoder );

        remember_info( info_list, &vid_info );

        title->video_codec_name = strdup( vid_info.name );
        title->width = vid_info.width;
        title->height = vid_info.height;
        title->rate = vid_info.rate;
        title->rate_base = vid_info.rate_base;
        title->video_bitrate = vid_info.bitrate;

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
                    hb_deep_log( 2, "Title's mostly NTSC Film, setting fps to 23.976");
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

        while( ( buf_es = hb_list_item( list_es, 0 ) ) )
        {
            hb_list_rem( list_es, buf_es );
            hb_buffer_close( &buf_es );
        }

        /* Check preview for interlacing artifacts */
        if( hb_detect_comb( vid_buf, title->width, title->height, 10, 30, 9, 10, 30, 9 ) )
        {
            hb_deep_log( 2, "Interlacing detected in preview frame %i", i+1);
            interlaced_preview_count++;
        }
        
        if( data->store_previews )
        {
            hb_get_tempory_filename( data->h, filename, "%" PRIxPTR "%d",
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
        }

        /* Detect black borders */

#define Y    vid_buf->data
        int top, bottom, left, right;
        int h4 = title->height / 4, w4 = title->width / 4;

        // When widescreen content is matted to 16:9 or 4:3 there's sometimes
        // a thin border on the outer edge of the matte. On TV content it can be
        // "line 21" VBI data that's normally hidden in the overscan. For HD
        // content it can just be a diagnostic added in post production so that
        // the frame borders are visible. We try to ignore these borders so
        // we can crop the matte. The border width depends on the resolution
        // (12 pixels on 1080i looks visually the same as 4 pixels on 480i)
        // so we allow the border to be up to 1% of the frame height.
        const int border = title->height / 100;

        for ( top = border; top < h4; ++top )
        {
            if ( ! row_all_dark( title, Y, top ) )
                break;
        }
        if ( top <= border )
        {
            // we never made it past the border region - see if the rows we
            // didn't check are dark or if we shouldn't crop at all.
            for ( top = 0; top < border; ++top )
            {
                if ( ! row_all_dark( title, Y, top ) )
                    break;
            }
            if ( top >= border )
            {
                top = 0;
            }
        }
        for ( bottom = border; bottom < h4; ++bottom )
        {
            if ( ! row_all_dark( title, Y, title->height - 1 - bottom ) )
                break;
        }
        if ( bottom <= border )
        {
            for ( bottom = 0; bottom < border; ++bottom )
            {
                if ( ! row_all_dark( title, Y, title->height - 1 - bottom ) )
                    break;
            }
            if ( bottom >= border )
            {
                bottom = 0;
            }
        }
        for ( left = 0; left < w4; ++left )
        {
            if ( ! column_all_dark( title, Y, top, bottom, left ) )
                break;
        }
        for ( right = 0; right < w4; ++right )
        {
            if ( ! column_all_dark( title, Y, top, bottom, title->width - 1 - right ) )
                break;
        }

        // only record the result if all the crops are less than a quarter of
        // the frame otherwise we can get fooled by frames with a lot of black
        // like titles, credits & fade-thru-black transitions.
        if ( top < h4 && bottom < h4 && left < w4 && right < w4 )
        {
            record_crop( crops, top, bottom, left, right );
        }
        ++npreviews;

skip_preview:
        if ( vid_buf )
            hb_buffer_close( &vid_buf );
    }

    if ( npreviews )
    {
        // use the most common frame info for our final title dimensions
        hb_work_info_t vid_info;
        most_common_info( info_list, &vid_info );

        title->width = vid_info.width;
        title->height = vid_info.height;
        title->pixel_aspect_width = vid_info.pixel_aspect_width;
        title->pixel_aspect_height = vid_info.pixel_aspect_height;

        // compute the aspect ratio based on the storage dimensions and the
        // pixel aspect ratio (if supplied) or just storage dimensions if no PAR.
        title->aspect = (double)title->width / (double)title->height;
        if( title->pixel_aspect_width && title->pixel_aspect_height )
        {
            title->aspect *= (double)title->pixel_aspect_width /
                             (double)title->pixel_aspect_height;

            // For unknown reasons some French PAL DVDs put the original
            // content's aspect ratio into the mpeg PAR even though it's
            // the wrong PAR for the DVD. Apparently they rely on the fact
            // that DVD players ignore the content PAR and just use the
            // aspect ratio from the DVD metadata. So, if the aspect computed
            // from the PAR is different from the container's aspect we use
            // the container's aspect & recompute the PAR from it.
            if( title->container_aspect && (int)(title->aspect * 9) != (int)(title->container_aspect * 9) )
            {
                hb_log("scan: content PAR gives wrong aspect %.2f; "
                       "using container aspect %.2f", title->aspect,
                       title->container_aspect );
                title->aspect = title->container_aspect;
                hb_reduce( &title->pixel_aspect_width, &title->pixel_aspect_height,
                           (int)(title->aspect * title->height + 0.5), title->width );
            }
        }

        // don't try to crop unless we got at least 3 previews
        if ( crops->n > 2 )
        {
            sort_crops( crops );
            // The next line selects median cropping - at least
            // 50% of the frames will have their borders removed.
            // Other possible choices are loose cropping (i = 0) where 
            // no non-black pixels will be cropped from any frame and a
            // tight cropping (i = crops->n - (crops->n >> 2)) where at
            // least 75% of the frames will have their borders removed.
            i = crops->n >> 1;
            title->crop[0] = EVEN( crops->t[i] );
            title->crop[1] = EVEN( crops->b[i] );
            title->crop[2] = EVEN( crops->l[i] );
            title->crop[3] = EVEN( crops->r[i] );
        }
        free( crops );

        hb_log( "scan: %d previews, %dx%d, %.3f fps, autocrop = %d/%d/%d/%d, "
                "aspect %s, PAR %d:%d",
                npreviews, title->width, title->height, (float) title->rate /
                (float) title->rate_base,
                title->crop[0], title->crop[1], title->crop[2], title->crop[3],
                aspect_to_string( title->aspect ), title->pixel_aspect_width,
                title->pixel_aspect_height );

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
    audio->config.in.version = info.version;
    audio->config.in.mode = info.mode;
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
