/* scan.c

   Copyright (c) 2003-2012 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "hbffmpeg.h"
#include "a52dec/a52.h"

typedef struct
{
    hb_handle_t  * h;
    volatile int * die;

    char         * path;
    int            title_index;
    hb_title_set_t * title_set;

    hb_bd_t      * bd;
    hb_dvd_t     * dvd;
    hb_stream_t  * stream;
    hb_batch_t   * batch;

    int            preview_count;
    int            store_previews;

    uint64_t       min_title_duration;

} hb_scan_t;

static void ScanFunc( void * );
static int  DecodePreviews( hb_scan_t *, hb_title_t * title );
static void LookForAudio( hb_title_t * title, hb_buffer_t * b );
static int  AllAudioOK( hb_title_t * title );
static void UpdateState1(hb_scan_t *scan, int title);
static void UpdateState2(hb_scan_t *scan, int title);
static void UpdateState3(hb_scan_t *scan, int preview);

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

hb_thread_t * hb_scan_init( hb_handle_t * handle, volatile int * die,
                            const char * path, int title_index, 
                            hb_title_set_t * title_set, int preview_count, 
                            int store_previews, uint64_t min_duration )
{
    hb_scan_t * data = calloc( sizeof( hb_scan_t ), 1 );

    data->h            = handle;
    data->die          = die;
    data->path         = strdup( path );
    data->title_index  = title_index;
    data->title_set    = title_set;

    data->preview_count  = preview_count;
    data->store_previews = store_previews;
    data->min_title_duration = min_duration;
    
    return hb_thread_init( "scan", ScanFunc, data, HB_NORMAL_PRIORITY );
}

static void ScanFunc( void * _data )
{
    hb_scan_t  * data = (hb_scan_t *) _data;
    hb_title_t * title;
    int          i;
    int          feature = 0;

    data->bd = NULL;
    data->dvd = NULL;
    data->stream = NULL;

    /* Try to open the path as a DVD. If it fails, try as a file */
    if( ( data->bd = hb_bd_init( data->path ) ) )
    {
        hb_log( "scan: BD has %d title(s)",
                hb_bd_title_count( data->bd ) );
        if( data->title_index )
        {
            /* Scan this title only */
            hb_list_add( data->title_set->list_title,
                         hb_bd_title_scan( data->bd,
                         data->title_index, 0 ) );
        }
        else
        {
            /* Scan all titles */
            for( i = 0; i < hb_bd_title_count( data->bd ); i++ )
            {
                UpdateState1(data, i + 1);
                hb_list_add( data->title_set->list_title,
                             hb_bd_title_scan( data->bd, 
                             i + 1, data->min_title_duration ) );
            }
            feature = hb_bd_main_feature( data->bd,
                                          data->title_set->list_title );
        }
    }
    else if( ( data->dvd = hb_dvd_init( data->path ) ) )
    {
        hb_log( "scan: DVD has %d title(s)",
                hb_dvd_title_count( data->dvd ) );
        if( data->title_index )
        {
            /* Scan this title only */
            hb_list_add( data->title_set->list_title,
                         hb_dvd_title_scan( data->dvd,
                            data->title_index, 0 ) );
        }
        else
        {
            /* Scan all titles */
            for( i = 0; i < hb_dvd_title_count( data->dvd ); i++ )
            {
                UpdateState1(data, i + 1);
                hb_list_add( data->title_set->list_title,
                             hb_dvd_title_scan( data->dvd, 
                            i + 1, data->min_title_duration ) );
            }
            feature = hb_dvd_main_feature( data->dvd,
                                           data->title_set->list_title );
        }
    }
    else if ( ( data->batch = hb_batch_init( data->path ) ) )
    {
        if( data->title_index )
        {
            /* Scan this title only */
            title = hb_batch_title_scan( data->batch, data->title_index );
            if ( title )
            {
                hb_list_add( data->title_set->list_title, title );
            }
        }
        else
        {
            /* Scan all titles */
            for( i = 0; i < hb_batch_title_count( data->batch ); i++ )
            {
                hb_title_t * title;

                UpdateState1(data, i + 1);
                title = hb_batch_title_scan( data->batch, i + 1 );
                if ( title != NULL )
                {
                    hb_list_add( data->title_set->list_title, title );
                }
            }
        }
    }
    else
    {
        data->title_index = 1;
        hb_title_t * title = hb_title_init( data->path, data->title_index );
        if ( (data->stream = hb_stream_open( data->path, title, 1 ) ) != NULL )
        {
            title = hb_stream_title_scan( data->stream, title );
            if ( title )
                hb_list_add( data->title_set->list_title, title );
        }
        else
        {
            hb_title_close( &title );
            hb_log( "scan: unrecognized file type" );
            return;
        }
    }

    for( i = 0; i < hb_list_count( data->title_set->list_title ); )
    {
        int j;
        hb_audio_t * audio;

        if ( *data->die )
        {
            goto finish;
        }
        title = hb_list_item( data->title_set->list_title, i );

        UpdateState2(data, i + 1);

        /* Decode previews */
        /* this will also detect more AC3 / DTS information */
        if( !DecodePreviews( data, title ) )
        {
            /* TODO: free things */
            hb_list_rem( data->title_set->list_title, title );
            for( j = 0; j < hb_list_count( title->list_audio ); j++)
            {
                audio = hb_list_item( title->list_audio, j );
                if ( audio->priv.scan_cache )
                {
                    hb_fifo_flush( audio->priv.scan_cache );
                    hb_fifo_close( &audio->priv.scan_cache );
                }
            }
            hb_title_close( &title );
            continue;
        }

        /* Make sure we found audio rates and bitrates */
        for( j = 0; j < hb_list_count( title->list_audio ); )
        {
            audio = hb_list_item( title->list_audio, j );
            if ( audio->priv.scan_cache )
            {
                hb_fifo_flush( audio->priv.scan_cache );
                hb_fifo_close( &audio->priv.scan_cache );
            }
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

        if ( data->dvd || data->bd )
        {
            // The subtitle width and height needs to be set to the 
            // title widht and height for DVDs.  title width and
            // height don't get set until we decode previews, so
            // we can't set subtitle width/height till we get here.
            for( j = 0; j < hb_list_count( title->list_subtitle ); j++ )
            {
                hb_subtitle_t *subtitle = hb_list_item( title->list_subtitle, j );
                if ( subtitle->source == VOBSUB || subtitle->source == PGSSUB )
                {
                    subtitle->width = title->width;
                    subtitle->height = title->height;
                }
            }
        }
        i++;
    }

    data->title_set->feature = feature;

    /* Mark title scan complete and init jobs */
    for( i = 0; i < hb_list_count( data->title_set->list_title ); i++ )
    {
        title      = hb_list_item( data->title_set->list_title, i );
        title->flags |= HBTF_SCAN_COMPLETE;
#if defined(HB_TITLE_JOBS)
        title->job = hb_job_init( title );
#endif
    }

finish:

    if( data->bd )
    {
        hb_bd_close( &data->bd );
    }
    if( data->dvd )
    {
        hb_dvd_close( &data->dvd );
    }
    if (data->stream)
    {
        hb_stream_close(&data->stream);
    }
    if( data->batch )
    {
        hb_batch_close( &data->batch );
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

static int row_all_dark( hb_buffer_t* buf, int row )
{
    int width = buf->plane[0].width;
    int stride = buf->plane[0].stride;
    uint8_t *luma = buf->plane[0].data + stride * row;

    // compute the average luma value of the row
    int i, avg = 0;
    for ( i = 0; i < width; ++i )
    {
        avg += clampBlack( luma[i] );
    }
    avg /= width;
    if ( avg >= DARK )
        return 0;

    // since we're trying to detect smooth borders, only take the row if
    // all pixels are within +-16 of the average (this range is fairly coarse
    // but there's a lot of quantization noise for luma values near black
    // so anything less will fail to crop because of the noise).
    for ( i = 0; i < width; ++i )
    {
        if ( absdiff( avg, clampBlack( luma[i] ) ) > 16 )
            return 0;
    }
    return 1;
}

static int column_all_dark( hb_buffer_t* buf, int top, int bottom, int col )
{
    int stride = buf->plane[0].stride;
    int height = buf->plane[0].height - top - bottom;
    uint8_t *luma = buf->plane[0].data + stride * top + col;

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
    int *t;
    int *b;
    int *l;
    int *r;
} crop_record_t;

static crop_record_t * crop_record_init( int max_previews )
{
    crop_record_t *crops = calloc( 1, sizeof(*crops) );

    crops->t = calloc( max_previews, sizeof(int) );
    crops->b = calloc( max_previews, sizeof(int) );
    crops->l = calloc( max_previews, sizeof(int) );
    crops->r = calloc( max_previews, sizeof(int) );

    return crops;
}

static void crop_record_free( crop_record_t *crops )
{
    free( crops->t );
    free( crops->b );
    free( crops->l );
    free( crops->r );
    free( crops );
}

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
}

static int has_resolution_change( info_list_t *info_list )
{
    int w, h, i;

    if( !info_list[0].count )
        return 0;
    w = info_list[0].info.width;
    h = info_list[0].info.height;
    for ( i = 1; info_list[i].count; ++i )
    {
        if ( w != info_list[i].info.width || h != info_list[i].info.height )
            return 1;
    }
    return 0;
}

static int is_close_to( int val, int target, int thresh )
{
    int diff = val - target;
    diff = diff < 0 ? -diff : diff;
    return diff < thresh;
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
    hb_buffer_t   * buf, * buf_es;
    hb_list_t     * list_es;
    int progressive_count = 0;
    int pulldown_count = 0;
    int doubled_frame_count = 0;
    int interlaced_preview_count = 0;
    info_list_t * info_list = calloc( data->preview_count+1, sizeof(*info_list) );
    crop_record_t *crops = crop_record_init( data->preview_count );

    list_es  = hb_list_init();

    if( data->batch )
    {
        hb_log( "scan: decoding previews for title %d (%s)", title->index, title->path );
    }
    else
    {
        hb_log( "scan: decoding previews for title %d", title->index );
    }

    if (data->bd)
    {
        hb_bd_start( data->bd, title );
        hb_log( "scan: title angle(s) %d", title->angle_count );
    }
    else if (data->dvd)
    {
        hb_dvd_start( data->dvd, title, 1 );
        title->angle_count = hb_dvd_angle_count( data->dvd );
        hb_log( "scan: title angle(s) %d", title->angle_count );
    }
    else if (data->batch)
    {
        data->stream = hb_stream_open( title->path, title, 1 );
    }

    int vcodec = title->video_codec? title->video_codec : WORK_DECMPEG2;
#if defined(USE_FF_MPEG2)
    if (vcodec == WORK_DECMPEG2)
    {
        vcodec = WORK_DECAVCODECV;
        title->video_codec_param = AV_CODEC_ID_MPEG2VIDEO;
    }
#endif
    hb_work_object_t *vid_decoder = hb_get_work( vcodec );
    vid_decoder->codec_param = title->video_codec_param;
    vid_decoder->title = title;
    vid_decoder->init( vid_decoder, NULL );

    for( i = 0; i < data->preview_count; i++ )
    {
        int j;

        UpdateState3(data, i + 1);

        if ( *data->die )
        {
            free( info_list );
            crop_record_free( crops );
            return 0;
        }
        if (data->bd)
        {
            if( !hb_bd_seek( data->bd, (float) ( i + 1 ) / ( data->preview_count + 1.0 ) ) )
          {
              continue;
          }
        }
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

        if ( vid_decoder->flush )
            vid_decoder->flush( vid_decoder );

        hb_buffer_t * vid_buf = NULL;

        for( j = 0; j < 10240 ; j++ )
        {
            if (data->bd)
            {
              if( (buf = hb_bd_read( data->bd )) == NULL )
              {
                  if ( vid_buf )
                  {
                    break;
                  }
                  hb_log( "Warning: Could not read data for preview %d, skipped", i + 1 );
                  goto skip_preview;
              }
            }
            else if (data->dvd)
            {
              if( (buf = hb_dvd_read( data->dvd )) == NULL )
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
              if ( (buf = hb_stream_read( data->stream )) == NULL )
              {
                  if ( vid_buf )
                  {
                    break;
                  }
                  hb_log( "Warning: Could not read data for preview %d, skipped", i + 1 );
                  goto skip_preview;
              }
            }
            else
            {
                // Silence compiler warning
                buf = NULL;
                hb_error( "Error: This can't happen!" );
                goto skip_preview;
            }

            (hb_demux[title->demuxer])(buf, list_es, 0 );

            while( ( buf_es = hb_list_item( list_es, 0 ) ) )
            {
                hb_list_rem( list_es, buf_es );
                if( buf_es->s.id == title->video_id && vid_buf == NULL )
                {
                    vid_decoder->work( vid_decoder, &buf_es, &vid_buf );
                }
                else if( ! AllAudioOK( title ) ) 
                {
                    LookForAudio( title, buf_es );
                    buf_es = NULL;
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
            if (vid_buf)
            {
                hb_buffer_close( &vid_buf );
            }
            hb_log( "scan: could not get a video information" );
            continue;
        }

        remember_info( info_list, &vid_info );

        if( is_close_to( vid_info.rate_base, 900900, 100 ) &&
            ( vid_buf->s.flags & PIC_FLAG_REPEAT_FIRST_FIELD ) )
        {
            /* Potentially soft telecine material */
            pulldown_count++;
        }

        if( vid_buf->s.flags & PIC_FLAG_REPEAT_FRAME )
        {
            // AVCHD-Lite specifies that all streams are
            // 50 or 60 fps.  To produce 25 or 30 fps, camera
            // makers are repeating all frames.
            doubled_frame_count++;
        }

        if( is_close_to( vid_info.rate_base, 1126125, 100 ) )
        {
            // Frame FPS is 23.976 (meaning it's progressive), so start keeping
            // track of how many are reporting at that speed. When enough 
            // show up that way, we want to make that the overall title FPS.
            progressive_count++;
        }

        while( ( buf_es = hb_list_item( list_es, 0 ) ) )
        {
            hb_list_rem( list_es, buf_es );
            hb_buffer_close( &buf_es );
        }

        /* Check preview for interlacing artifacts */
        if( hb_detect_comb( vid_buf, 10, 30, 9, 10, 30, 9 ) )
        {
            hb_deep_log( 2, "Interlacing detected in preview frame %i", i+1);
            interlaced_preview_count++;
        }
        
        if( data->store_previews )
        {
            hb_save_preview( data->h, title->index, i, vid_buf );
        }

        /* Detect black borders */

        int top, bottom, left, right;
        int h4 = vid_info.height / 4, w4 = vid_info.width / 4;

        // When widescreen content is matted to 16:9 or 4:3 there's sometimes
        // a thin border on the outer edge of the matte. On TV content it can be
        // "line 21" VBI data that's normally hidden in the overscan. For HD
        // content it can just be a diagnostic added in post production so that
        // the frame borders are visible. We try to ignore these borders so
        // we can crop the matte. The border width depends on the resolution
        // (12 pixels on 1080i looks visually the same as 4 pixels on 480i)
        // so we allow the border to be up to 1% of the frame height.
        const int border = vid_info.height / 100;

        for ( top = border; top < h4; ++top )
        {
            if ( ! row_all_dark( vid_buf, top ) )
                break;
        }
        if ( top <= border )
        {
            // we never made it past the border region - see if the rows we
            // didn't check are dark or if we shouldn't crop at all.
            for ( top = 0; top < border; ++top )
            {
                if ( ! row_all_dark( vid_buf, top ) )
                    break;
            }
            if ( top >= border )
            {
                top = 0;
            }
        }
        for ( bottom = border; bottom < h4; ++bottom )
        {
            if ( ! row_all_dark( vid_buf, vid_info.height - 1 - bottom ) )
                break;
        }
        if ( bottom <= border )
        {
            for ( bottom = 0; bottom < border; ++bottom )
            {
                if ( ! row_all_dark( vid_buf, vid_info.height - 1 - bottom ) )
                    break;
            }
            if ( bottom >= border )
            {
                bottom = 0;
            }
        }
        for ( left = 0; left < w4; ++left )
        {
            if ( ! column_all_dark( vid_buf, top, bottom, left ) )
                break;
        }
        for ( right = 0; right < w4; ++right )
        {
            if ( ! column_all_dark( vid_buf, top, bottom, vid_info.width - 1 - right ) )
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
        /* Make sure we found audio rates and bitrates */
        for( j = 0; j < hb_list_count( title->list_audio ); j++ )
        {
            hb_audio_t * audio = hb_list_item( title->list_audio, j );
            if ( audio->priv.scan_cache )
            {
                hb_fifo_flush( audio->priv.scan_cache );
            }
        }
        if (vid_buf)
        {
            hb_buffer_close( &vid_buf );
        }
    }
    UpdateState3(data, i);

    vid_decoder->close( vid_decoder );
    free( vid_decoder );

    if ( data->batch && data->stream )
    {
        hb_stream_close( &data->stream );
    }

    if ( npreviews )
    {
        // use the most common frame info for our final title dimensions
        hb_work_info_t vid_info;
        most_common_info( info_list, &vid_info );

        title->has_resolution_change = has_resolution_change( info_list );
        if ( title->video_codec_name == NULL )
        {
            title->video_codec_name = strdup( vid_info.name );
        }
        title->width = vid_info.width;
        title->height = vid_info.height;
        if ( vid_info.rate && vid_info.rate_base )
        {
            // if the frame rate is very close to one of our "common" framerates,
            // assume it actually is said frame rate; e.g. some 24000/1001 sources
            // may have a rate_base of 1126124 (instead of 1126125)
            for( i = 0; i < hb_video_rates_count; i++ )
            {
                if( is_close_to( vid_info.rate_base, hb_video_rates[i].rate, 100 ) )
                {
                    vid_info.rate_base = hb_video_rates[i].rate;
                    break;
                }
            }
            title->rate = vid_info.rate;
            title->rate_base = vid_info.rate_base;
            if( vid_info.rate_base == 900900 )
            {
                if( pulldown_count >= npreviews / 4 )
                {
                    title->rate_base = 1126125;
                    hb_deep_log( 2, "Pulldown detected, setting fps to 23.976" );
                }
                if( progressive_count >= npreviews / 2 )
                {
                    // We've already deduced that the frame rate is 23.976,
                    // so set it back again.
                    title->rate_base = 1126125;
                    hb_deep_log( 2, "Title's mostly NTSC Film, setting fps to 23.976" );
                }
            }
            if( doubled_frame_count >= 3 * npreviews / 4 )
            {
                // We've detected that a significant number of the frames
                // have been doubled in duration by repeat flags.
                title->rate_base = 2 * vid_info.rate_base;
                hb_deep_log( 2, "Repeat frames detected, setting fps to %.3f", (float)title->rate / title->rate_base );
            }
        }
        title->video_bitrate = vid_info.bitrate;

        if( vid_info.pixel_aspect_width && vid_info.pixel_aspect_height )
        {
            title->pixel_aspect_width = vid_info.pixel_aspect_width;
            title->pixel_aspect_height = vid_info.pixel_aspect_height;
        }
        title->color_prim = vid_info.color_prim;
        title->color_transfer = vid_info.color_transfer;
        title->color_matrix = vid_info.color_matrix;

        // compute the aspect ratio based on the storage dimensions and the
        // pixel aspect ratio (if supplied) or just storage dimensions if no PAR.
        title->aspect = (double)title->width / (double)title->height;
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
    crop_record_free( crops );
    free( info_list );

    while( ( buf_es = hb_list_item( list_es, 0 ) ) )
    {
        hb_list_rem( list_es, buf_es );
        hb_buffer_close( &buf_es );
    }
    hb_list_close( &list_es );
    if (data->bd)
      hb_bd_stop( data->bd );
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
        if ( audio->id == b->s.id )
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
        hb_buffer_close( &b );
        return;
    }

    if ( audio->priv.scan_cache == NULL )
        audio->priv.scan_cache = hb_fifo_init( 16, 16 );

    if ( hb_fifo_size_bytes( audio->priv.scan_cache ) >= 16384 )
    {
        hb_buffer_t * tmp;
        tmp = hb_fifo_get( audio->priv.scan_cache );
        hb_buffer_close( &tmp );
    }
    hb_fifo_push( audio->priv.scan_cache, b );

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
    b = hb_fifo_see( audio->priv.scan_cache );
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
        free( w );
        return;
    }
    hb_fifo_flush( audio->priv.scan_cache );
    hb_fifo_close( &audio->priv.scan_cache );

    audio->config.in.samplerate = info.rate;
    audio->config.in.samples_per_frame = info.samples_per_frame;
    audio->config.in.bitrate = info.bitrate;
    audio->config.in.channel_layout = info.channel_layout;
    audio->config.in.channel_map = info.channel_map;
    audio->config.in.version = info.version;
    audio->config.in.flags = info.flags;
    audio->config.in.mode = info.mode;

    // update the audio description string based on the info we found
    if (audio->config.in.channel_layout == AV_CH_LAYOUT_STEREO_DOWNMIX)
    {
        strcat(audio->config.lang.description, " (Dolby Surround)");
    }
    else if (audio->config.in.channel_layout)
    {
        int lfes     = (!!(audio->config.in.channel_layout & AV_CH_LOW_FREQUENCY) +
                        !!(audio->config.in.channel_layout & AV_CH_LOW_FREQUENCY_2));
        int channels = av_get_channel_layout_nb_channels(audio->config.in.channel_layout);
        char *desc   = audio->config.lang.description +
                        strlen(audio->config.lang.description);
        sprintf(desc, " (%d.%d ch)", channels - lfes, lfes);
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

    hb_fifo_flush( audio->priv.scan_cache );
    hb_fifo_close( &audio->priv.scan_cache );
    hb_list_rem( title->list_audio, audio );
    return;
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

static void UpdateState1(hb_scan_t *scan, int title)
{
    hb_state_t state;

#define p state.param.scanning
    /* Update the UI */
    state.state   = HB_STATE_SCANNING;
    p.title_cur   = title;
    p.title_count = scan->dvd ? hb_dvd_title_count( scan->dvd ) :
                    scan->bd ? hb_bd_title_count( scan->bd ) :
                    scan->batch ? hb_batch_title_count( scan->batch ) :
                               hb_list_count(scan->title_set->list_title);
    p.preview_cur = 0;
    p.preview_count = 1;
    p.progress = 0.5 * ((float)p.title_cur + ((float)p.preview_cur / p.preview_count)) / p.title_count;
#undef p

    hb_set_state(scan->h, &state);
}

static void UpdateState2(hb_scan_t *scan, int title)
{
    hb_state_t state;

#define p state.param.scanning
    /* Update the UI */
    state.state   = HB_STATE_SCANNING;
    p.title_cur   = title;
    p.title_count = hb_list_count( scan->title_set->list_title );
    p.preview_cur = 1;
    p.preview_count = scan->preview_count;
    if (scan->title_index)
        p.progress = (float)p.title_cur / p.title_count;
    else
        p.progress = 0.5 + 0.5 * (float)p.title_cur / p.title_count;
#undef p

    hb_set_state(scan->h, &state);
}

static void UpdateState3(hb_scan_t *scan, int preview)
{
    hb_state_t state;

    hb_get_state2(scan->h, &state);
#define p state.param.scanning
    p.preview_cur = preview;
    p.preview_count = scan->preview_count;
    if (scan->title_index)
        p.progress = ((float)p.title_cur - 1 + ((float)p.preview_cur / p.preview_count)) / p.title_count;
    else
        p.progress = 0.5 + 0.5 * ((float)p.title_cur - 1 + ((float)p.preview_cur / p.preview_count)) / p.title_count;
#undef p

    hb_set_state(scan->h, &state);
}
