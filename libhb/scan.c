/* scan.c

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/hwaccel.h"

typedef struct
{
    hb_handle_t  * h;
    volatile int * die;

    hb_list_t    * paths;
    int            title_index;
    hb_title_set_t * title_set;

    hb_bd_t      * bd;
    hb_dvd_t     * dvd;
    hb_stream_t  * stream;
    hb_batch_t   * batch;

    int            preview_count;
    int            store_previews;

    uint64_t       min_title_duration;
    
    int            crop_threshold_frames;
    int            crop_threshold_pixels;
    
    hb_list_t    * exclude_extensions;

    int            hw_decode;
    
} hb_scan_t;

#define PREVIEW_READ_THRESH (200)

static void ScanFunc( void * );
static int  DecodePreviews( hb_scan_t *, hb_title_t * title, int flush );
static void LookForAudio(hb_scan_t *scan, hb_title_t *title, hb_buffer_t *b);
static int  AllAudioOK( hb_title_t * title );
static void UpdateState1(hb_scan_t *scan, int title);
static void UpdateState2(hb_scan_t *scan, int title);
static void UpdateState3(hb_scan_t *scan, int preview);

static int get_color_prim(int color_primaries, hb_geometry_t geometry, hb_rational_t rate);
static int get_color_transfer(int color_trc);
static int get_color_matrix(int colorspace, hb_geometry_t geometry);
static int get_color_range(int color_range);

static const char *aspect_to_string(hb_rational_t *dar)
{
    double aspect = (double)dar->num / dar->den;
    switch ( (int)(aspect * 9.) )
    {
        case 9 * 4 / 3:    return "4:3";
        case 9 * 16 / 9:   return "16:9";
    }
    static char arstr[32];
    if (aspect >= 1)
        snprintf(arstr, sizeof(arstr), "%.2f:1", aspect);
    else
        snprintf(arstr, sizeof(arstr), "1:%.2f", 1. / aspect );
    return arstr;
}

static int get_color_prim(int color_primaries, hb_geometry_t geometry, hb_rational_t rate)
{
    switch (color_primaries)
    {
        case AVCOL_PRI_BT709:
            return HB_COLR_PRI_BT709;
        case AVCOL_PRI_BT470M:
            return HB_COLR_PRI_BT470M;
        case AVCOL_PRI_BT470BG:
            return HB_COLR_PRI_EBUTECH;
        case AVCOL_PRI_SMPTE170M:
        case AVCOL_PRI_SMPTE240M:
            return HB_COLR_PRI_SMPTEC;
        case AVCOL_PRI_FILM:
            return HB_COLR_PRI_FILM;
        case AVCOL_PRI_SMPTE428:
            return HB_COLR_PRI_SMPTE428;
        case AVCOL_PRI_SMPTE431:
            return HB_COLR_PRI_SMPTE431;
        case AVCOL_PRI_SMPTE432:
            return HB_COLR_PRI_SMPTE432;
        case AVCOL_PRI_JEDEC_P22:
            return HB_COLR_PRI_JEDEC_P22;
        case AVCOL_PRI_BT2020:
            return HB_COLR_PRI_BT2020;
        default:
        {
            if ((geometry.width >= 1280 || geometry.height >= 720)||
                (geometry.width >   720 && geometry.height >  576 ))
                // ITU BT.709 HD content
                return HB_COLR_PRI_BT709;
            else if (rate.den == 1080000)
                // ITU BT.601 DVD or SD TV content (PAL)
                return HB_COLR_PRI_EBUTECH;
            else
                // ITU BT.601 DVD or SD TV content (NTSC)
                return HB_COLR_PRI_SMPTEC;
        }
    }
}

static int get_color_transfer(int color_trc)
{
    switch (color_trc)
    {
        case AVCOL_TRC_GAMMA22:
            return HB_COLR_TRA_GAMMA22;
        case AVCOL_TRC_GAMMA28:
            return HB_COLR_TRA_GAMMA28;
        case AVCOL_TRC_SMPTE170M:
            return HB_COLR_TRA_SMPTE170M;
        case AVCOL_TRC_LINEAR:
            return HB_COLR_TRA_LINEAR;
        case AVCOL_TRC_LOG:
            return HB_COLR_TRA_LOG;
        case AVCOL_TRC_LOG_SQRT:
            return HB_COLR_TRA_LOG_SQRT;
        case AVCOL_TRC_IEC61966_2_4:
            return HB_COLR_TRA_IEC61966_2_4;
        case AVCOL_TRC_BT1361_ECG:
            return HB_COLR_TRA_BT1361_ECG;
        case AVCOL_TRC_IEC61966_2_1:
            return HB_COLR_TRA_IEC61966_2_1;
        case AVCOL_TRC_SMPTE240M:
            return HB_COLR_TRA_SMPTE240M;
        case AVCOL_TRC_SMPTEST2084:
            return HB_COLR_TRA_SMPTEST2084;
        case AVCOL_TRC_ARIB_STD_B67:
            return HB_COLR_TRA_ARIB_STD_B67;
        case AVCOL_TRC_BT2020_10:
            return HB_COLR_TRA_BT2020_10;
        case AVCOL_TRC_BT2020_12:
            return HB_COLR_TRA_BT2020_12;
        default:
            // ITU BT.601, BT.709, anything else
            return HB_COLR_TRA_BT709;
    }
}

static int get_color_matrix(int colorspace, hb_geometry_t geometry)
{
    switch (colorspace)
    {
        case AVCOL_SPC_RGB:
            return HB_COLR_MAT_RGB;
        case AVCOL_SPC_BT709:
            return HB_COLR_MAT_BT709;
        case AVCOL_SPC_FCC:
            return HB_COLR_MAT_FCC;
        case AVCOL_SPC_BT470BG:
            return HB_COLR_MAT_BT470BG;
        case AVCOL_SPC_SMPTE170M:
            return HB_COLR_MAT_SMPTE170M;
        case AVCOL_SPC_SMPTE240M:
            return HB_COLR_MAT_SMPTE240M;
        case AVCOL_SPC_YCGCO:
            return HB_COLR_MAT_YCGCO;
        case AVCOL_SPC_BT2020_NCL:
            return HB_COLR_MAT_BT2020_NCL;
        case AVCOL_SPC_BT2020_CL:
            return HB_COLR_MAT_BT2020_CL;
        case AVCOL_SPC_CHROMA_DERIVED_NCL:
            return HB_COLR_MAT_CD_NCL;
        case AVCOL_SPC_CHROMA_DERIVED_CL:
            return HB_COLR_MAT_CD_CL;
        case AVCOL_SPC_ICTCP:
            return HB_COLR_MAT_ICTCP;
        default:
        {
            if ((geometry.width >= 1280 || geometry.height >= 720)||
                (geometry.width >   720 && geometry.height >  576 ))
                // ITU BT.709 HD content
                return HB_COLR_MAT_BT709;
            else
                // ITU BT.601 DVD or SD TV content (PAL)
                // ITU BT.601 DVD or SD TV content (NTSC)
                return HB_COLR_MAT_SMPTE170M;
        }
    }
}

static int get_color_range(int color_range)
{
    switch (color_range)
    {
        case AVCOL_RANGE_MPEG:
            return AVCOL_RANGE_MPEG;
        case AVCOL_RANGE_JPEG:
            return AVCOL_RANGE_JPEG;
        default:
            return AVCOL_RANGE_MPEG;
    }
}

static const char * const known_file_types[] =
{
    "mp4", "m4v", "mov", "flv", "mkv", "avi", "webm", "wmv",  NULL
};

static int is_known_filetype(const char *filename)
{
    for (int i = 0; known_file_types[i] != NULL; i++)
    {
        if (hb_str_ends_with(filename, known_file_types[i]))
        {
            return 1;
        }
    }
    return 0;
}

hb_thread_t * hb_scan_init( hb_handle_t * handle, volatile int * die,
                            hb_list_t *  paths, int title_index,
                            hb_title_set_t * title_set, int preview_count,
                            int store_previews, uint64_t min_duration,
                            int crop_threshold_frames, int crop_threshold_pixels,
                            hb_list_t * exclude_extensions, int hw_decode)
{
    hb_scan_t * data = calloc( sizeof( hb_scan_t ), 1 );

    data->h            = handle;
    data->die          = die;
    data->paths        = hb_string_list_copy(paths);
    data->title_index  = title_index;
    data->title_set    = title_set;

    data->preview_count  = preview_count;
    data->store_previews = store_previews;
    data->min_title_duration = min_duration;
    
    data->crop_threshold_frames = crop_threshold_frames;
    data->crop_threshold_pixels = crop_threshold_pixels;
    data->exclude_extensions    = hb_string_list_copy(exclude_extensions);
    data->hw_decode             = hw_decode;
    
    // Initialize scan state
    hb_state_t state;
    hb_get_state2(handle, &state);
#define p state.param.scanning
    state.state   = HB_STATE_SCANNING;
    p.title_cur   = 1;
    p.title_count = 1;
    p.preview_cur = 0;
    p.preview_count = 1;
    p.progress = 0.0;
#undef p
    hb_set_state(handle, &state);

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
        
    char *single_path = NULL;
    if (hb_list_count(data->paths) == 1)
    {
        single_path = hb_list_item(data->paths, 0);
    }
        
    /* Try to open the path as a DVD. If it fails, try as a file */
    if( single_path != NULL && !is_known_filetype(single_path) && ( data->bd = hb_bd_init( data->h, single_path ) ) )
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
    else if( single_path != NULL && !is_known_filetype(single_path) && ( data->dvd = hb_dvd_init( data->h, single_path ) ) )
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
    else if (single_path != NULL && ( data->batch = hb_batch_init( data->h, single_path, data->exclude_extensions ) ) )
    {
        if( data->title_index )
        {
            /* Scan this title only */
            title = hb_batch_title_scan(data->batch, data->title_index);
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
                title = hb_batch_title_scan(data->batch, i + 1);
                if ( title != NULL )
                {
                    hb_list_add( data->title_set->list_title, title );
                }
            }
        }
    }
    else if (hb_list_count(data->paths) > 1) // We have many file paths to process.
    {
        // If dragging a batch of files, maybe not, but if the UI's implement a recursive folder maybe?
        for (i = 0; i < hb_list_count( data->paths ); i++)
        {
            single_path = hb_list_item(data->paths, i);

            UpdateState1(data, i + 1);

            if (hb_is_valid_batch_path(single_path))
            {
                title = hb_batch_title_scan_single(data->h, single_path, (int)i + 1);
                if (title != NULL)
                {
                    hb_list_add(data->title_set->list_title, title);
                }
            }
        }
    }
    else if (single_path != NULL) // Single File.
    {
        // Title index 0 is not a valid title number and means scan all titles.
        // So set title index to 1 in this scenario.
        //
        // Otherwise, set title index in new title to the index that was
        // requested.  This preserves the original index created in batch
        // mode.
        if (data->title_index == 0)
            data->title_index = 1;
        hb_title_t * title = hb_title_init( single_path, data->title_index );
        data->stream = hb_stream_open(data->h, single_path, title, 1);
        if (data->stream != NULL)
        {
            title = hb_stream_title_scan( data->stream, title );
            if ( title )
                hb_list_add( data->title_set->list_title, title );
        }
        else
        {
            hb_title_close( &title );
            hb_log( "scan: unrecognized file type" );
            goto finish;
        }
    }

    for( i = 0; i < hb_list_count( data->title_set->list_title ); )
    {
        int j, npreviews;
        hb_audio_t * audio;

        if ( *data->die )
        {
            goto finish;
        }
        title = hb_list_item( data->title_set->list_title, i );

        UpdateState2(data, i + 1);

        /* Decode previews */
        /* this will also detect more AC3 / DTS information */
        npreviews = DecodePreviews( data, title, 1 );
        if (npreviews < 2)
        {
            // Try harder to get some valid frames
            // Allow libav to return "corrupt" frames
            hb_log("scan: Too few previews (%d), trying harder", npreviews);
            title->flags |= HBTF_NO_IDR;
            npreviews = DecodePreviews( data, title, 0 );
        }
        if (npreviews == 0)
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
        title->preview_count = npreviews;

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

        // VOBSUB and PGS width and height needs to be set to the
        // title width and height for any stream type that does
        // not provide this information (DVDs, BDs, VOBs, and M2TSs).
        // Title width and height don't get set until we decode
        // previews, so we can't set subtitle width/height till
        // we get here.
        for (j = 0; j < hb_list_count(title->list_subtitle); j++)
        {
            hb_subtitle_t *subtitle = hb_list_item(title->list_subtitle, j);
            if ((subtitle->source == VOBSUB || subtitle->source == PGSSUB) &&
                (subtitle->width <= 0 || subtitle->height <= 0))
            {
                subtitle->width  = title->geometry.width;
                subtitle->height = title->geometry.height;
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
    }

    if (hb_list_count(data->title_set->list_title) > 0)
    {
        if (single_path != NULL)
        {
            data->title_set->path = strdup(single_path);
        } else
        {
            data->title_set->path = NULL; // we have many paths.
        }
    }
    else
    {
        free((char*)data->title_set->path);
        data->title_set->path = NULL;
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

    // Clear down any file paths.
    char *output_filepath;
    while ((output_filepath = hb_list_item(data->paths, 0)))
    {
        hb_list_rem(data->paths, output_filepath);
        free(output_filepath);
    }
    hb_list_close(&data->paths);

    // clean up excluded extensions list
    char *extension;
    while ((extension = hb_list_item(data->exclude_extensions, 0)))
    {
        hb_list_rem(data->exclude_extensions, extension);
        free(extension);
    }
    hb_list_close(&data->exclude_extensions);

    free( data );
    _data = NULL;
    hb_buffer_pool_free();
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
    w = info_list[0].info.geometry.width;
    h = info_list[0].info.geometry.height;
    for ( i = 1; info_list[i].count; ++i )
    {
        if (w != info_list[i].info.geometry.width ||
            h != info_list[i].info.geometry.height)
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

static hb_buffer_t * read_buf(hb_scan_t * data, hb_stream_t * stream)
{
    if (data->bd)
    {
        return hb_bd_read(data->bd);
    }
    else if (data->dvd)
    {
        return hb_dvd_read(data->dvd);
    }
    else if (stream)
    {
        return hb_stream_read(stream);
    }

    // This shouldn't happen
    return NULL;
}

/***********************************************************************
 * DecodePreviews
 ***********************************************************************
 * Decode 10 pictures for the given title.
 * It assumes that data->reader and data->vts have successfully been
 * DVDOpen()ed and ifoOpen()ed.
 **********************************************************************/
static int DecodePreviews( hb_scan_t * data, hb_title_t * title, int flush )
{
    int                i, npreviews = 0, abort = 0;
    hb_buffer_t      * buf, * buf_es;
    hb_buffer_list_t   list_es;
    int                progressive_count = 0;
    int                pulldown_count = 0;
    int                doubled_frame_count = 0;
    int                interlaced_preview_count = 0;
    int                vid_samples = 0;
    int                frame_wait = 0;
    int                cc_wait = 10;
    int                frames;
    hb_stream_t      * stream = NULL;
    info_list_t      * info_list;
    int                abort_audio = 0;

    info_list = calloc(data->preview_count+1, sizeof(*info_list));
    crop_record_t *crops = crop_record_init( data->preview_count );

    hb_buffer_list_clear(&list_es);

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
        stream = hb_stream_open(data->h, title->path, title, 0);
    }
    else 
    {
        // We have a batch of files.
        stream = hb_stream_open(data->h, title->path, title, 0);
    }

    if (title->video_codec == WORK_NONE)
    {
        hb_error("No video decoder set!");
        free(info_list);
        crop_record_free(crops);
        hb_stream_close(&stream);
        return 0;
    }

    int hw_decode = 0;

    if (data->hw_decode == HB_DECODE_SUPPORT_NVDEC &&
        hb_hwaccel_available(title->video_codec_param, "cuda"))
    {
        hw_decode = HB_DECODE_SUPPORT_NVDEC;
    }
    else if (data->hw_decode == HB_DECODE_SUPPORT_VIDEOTOOLBOX &&
             hb_hwaccel_available(title->video_codec_param, "videotoolbox"))
    {
        hw_decode = HB_DECODE_SUPPORT_VIDEOTOOLBOX;
    }

    void *hw_device_ctx = NULL;
    if (hw_decode)
    {
        hb_hwaccel_hw_ctx_init(title->video_codec_param, hw_decode, &hw_device_ctx);
    }

    hb_work_object_t *vid_decoder = hb_get_work(data->h, title->video_codec);
    vid_decoder->codec_param = title->video_codec_param;
    vid_decoder->hw_device_ctx = hw_device_ctx;
    vid_decoder->title = title;

    if (vid_decoder->init(vid_decoder, NULL))
    {
        hb_error("Decoder init failed!");
        free(info_list);
        crop_record_free(crops);
        free( vid_decoder );
        hb_stream_close(&stream);
        return 0;
    }

    for( i = 0; i < data->preview_count; i++ )
    {
        int j;

        UpdateState3(data, i + 1);

        if ( *data->die )
        {
            free( info_list );
            crop_record_free( crops );
            vid_decoder->close( vid_decoder );
            free( vid_decoder );
            hb_stream_close(&stream);
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
        else if (stream)
        {
            /* we start reading streams at zero rather than 1/11 because
             * short streams may have only one sequence header in the entire
             * file and we need it to decode any previews.
             *
             * Also, seeking to position 0 loses the palette of avi files
             * so skip initial seek */
            if (i != 0)
            {
                if (!hb_stream_seek(stream,
                                    (float)i / (data->preview_count + 1.0)))
                {
                    continue;
                }
            }
            else
            {
                hb_stream_set_need_keyframe(stream, 1);
            }
        }

        hb_deep_log( 2, "scan: preview %d", i + 1 );

        if (flush && vid_decoder->flush)
            vid_decoder->flush( vid_decoder );
        if (title->flags & HBTF_NO_IDR)
        {
            if (!flush)
            {
                // If we are doing the first previews decode attempt,
                // set this threshold high so that we get the best
                // quality frames possible.
                frame_wait = 100;
            }
            else
            {
                // If we failed to get enough valid frames in the first
                // previews decode attempt, lower the threshold to improve
                // our chances of getting something to work with.
                frame_wait = 10;
            }
        }
        else
        {
            // For certain mpeg-2 streams, libav is delivering a
            // dummy first frame that is all black.  So always skip
            // one frame
            frame_wait = 1;
        }
        frames = 0;

        hb_buffer_t * vid_buf = NULL, * last_vid_buf = NULL;

        int packets = 0;
        vid_decoder->frame_count = 0;
        while (vid_decoder->frame_count < PREVIEW_READ_THRESH ||
              (!AllAudioOK(title) && packets < 10000))
        {
            if ((buf = read_buf(data, stream)) == NULL)
            {
                // If we reach EOF and no audio, don't continue looking for
                // audio
                abort_audio = 1;
                if (vid_buf != NULL || last_vid_buf != NULL)
                {
                    break;
                }
                hb_log("Warning: Could not read data for preview %d, skipped",
                       i + 1 );

                // If we reach EOF and no video, don't continue looking for
                // video
                abort = 1;
                goto skip_preview;
            }

            packets++;
            if (buf->size <= 0)
            {
                // Ignore "null" frames
                hb_buffer_close(&buf);
                continue;
            }

            (hb_demux[title->demuxer])(buf, &list_es, 0 );

            while ((buf_es = hb_buffer_list_rem_head(&list_es)) != NULL)
            {
                if( buf_es->s.id == title->video_id && vid_buf == NULL )
                {
                    vid_decoder->work( vid_decoder, &buf_es, &vid_buf );
                    // There are 2 conditions we decode additional
                    // video frames for during scan.
                    // 1. We did not detect IDR frames, so the initial video
                    //    frames may be corrupt.  We decode extra frames to
                    //    increase the probability of a complete preview frame
                    // 2. Some frames do not contain CC data, even though
                    //    CCs are present in the stream.  So we need to decode
                    //    additional frames to find the CCs.
                    if (vid_buf != NULL && (frame_wait || cc_wait))
                    {
                        hb_work_info_t vid_info;
                        if (vid_decoder->info(vid_decoder, &vid_info))
                        {
                            if (is_close_to(vid_info.rate.den, 900900, 100) &&
                                (vid_buf->s.flags & PIC_FLAG_REPEAT_FIRST_FIELD))
                            {
                                /* Potentially soft telecine material */
                                pulldown_count++;
                            }

                            if (vid_buf->s.flags & PIC_FLAG_REPEAT_FRAME)
                            {
                                // AVCHD-Lite specifies that all streams are
                                // 50 or 60 fps.  To produce 25 or 30 fps, camera
                                // makers are repeating all frames.
                                doubled_frame_count++;
                            }

                            if (is_close_to(vid_info.rate.den, 1126125, 100 ))
                            {
                                // Frame FPS is 23.976 (meaning it's
                                // progressive), so start keeping track of
                                // how many are reporting at that speed. When
                                // enough show up that way, we want to make
                                // that the overall title FPS.
                                progressive_count++;
                            }
                            vid_samples++;
                        }

                        if (frames > 0 && vid_buf->s.frametype == HB_FRAME_I)
                            frame_wait = 0;
                        if (frame_wait || cc_wait)
                        {
                            hb_buffer_close(&last_vid_buf);
                            last_vid_buf = vid_buf;
                            vid_buf = NULL;
                            if (frame_wait) frame_wait--;
                            if (cc_wait) cc_wait--;
                        }
                        frames++;
                    }
                }
                else if (!AllAudioOK(title) && !abort_audio)
                {
                    LookForAudio( data, title, buf_es );
                    buf_es = NULL;
                }
                if ( buf_es )
                    hb_buffer_close( &buf_es );
            }

            if (vid_buf && (abort_audio || AllAudioOK(title)))
                break;
        }
        hb_buffer_list_close(&list_es);

        if (vid_buf == NULL)
        {
            vid_buf = last_vid_buf;
            last_vid_buf = NULL;
        }
        hb_buffer_close(&last_vid_buf);

        if (vid_buf == NULL)
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
            hb_log( "scan: could not get a video information" );
            hb_buffer_close( &vid_buf );
            continue;
        }

        if (vid_info.geometry.width  != vid_buf->f.width ||
            vid_info.geometry.height != vid_buf->f.height)
        {
            hb_log( "scan: video geometry information does not match buffer" );
            hb_buffer_close( &vid_buf );
            continue;
        }
        remember_info( info_list, &vid_info );

        /* Check preview for interlacing artifacts */
        if( hb_detect_comb( vid_buf, 10, 30, 9, 10, 30, 9 ) )
        {
            hb_deep_log( 2, "Interlacing detected in preview frame %i", i+1);
            interlaced_preview_count++;
        }

        if( data->store_previews )
        {
            hb_save_preview( data->h, title->index, i, vid_buf, HB_PREVIEW_FORMAT_JPG );
        }

        /* Detect black borders */

        int top, bottom, left, right;
        int h4 = vid_info.geometry.height / 4, w4 = vid_info.geometry.width / 4;

        // When widescreen content is matted to 16:9 or 4:3 there's sometimes
        // a thin border on the outer edge of the matte. On TV content it can be
        // "line 21" VBI data that's normally hidden in the overscan. For HD
        // content it can just be a diagnostic added in post production so that
        // the frame borders are visible. We try to ignore these borders so
        // we can crop the matte. The border width depends on the resolution
        // (12 pixels on 1080i looks visually the same as 4 pixels on 480i)
        // so we allow the border to be up to 1% of the frame height.
        const int border = vid_info.geometry.height / 100;

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
            if ( ! row_all_dark( vid_buf, vid_info.geometry.height - 1 - bottom ) )
                break;
        }
        if ( bottom <= border )
        {
            for ( bottom = 0; bottom < border; ++bottom )
            {
                if ( ! row_all_dark( vid_buf, vid_info.geometry.height - 1 - bottom ) )
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
            if ( ! column_all_dark( vid_buf, top, bottom, vid_info.geometry.width - 1 - right ) )
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
        if (abort)
        {
            break;
        }
    }
    UpdateState3(data, i);

    vid_decoder->close( vid_decoder );
    free( vid_decoder );

    hb_hwaccel_hw_ctx_close(&hw_device_ctx);

    if (stream != NULL)
    {
        hb_stream_close(&stream);
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
        title->video_codec_profile = vid_info.profile;
        title->geometry.width = vid_info.geometry.width;
        title->geometry.height = vid_info.geometry.height;
        if (vid_info.rate.num && vid_info.rate.den)
        {
            // if the frame rate is very close to one of our "common"
            // framerates, assume it actually is said frame rate;
            // e.g. some 24000/1001 sources may have a rate.den of 1126124
            // instead of 1126125
            const hb_rate_t *video_framerate = NULL;
            while ((video_framerate = hb_video_framerate_get_next(video_framerate)) != NULL)
            {
                if (is_close_to(vid_info.rate.den, video_framerate->rate, 100))
                {
                    vid_info.rate.den = video_framerate->rate;
                    break;
                }
            }
            title->vrate = vid_info.rate;
            if( vid_info.rate.den == 900900 )
            {
                if (vid_samples >= 4 && pulldown_count >= vid_samples / 4)
                {
                    title->vrate.den = 1126125;
                    hb_deep_log( 2, "Pulldown detected, setting fps to 23.976" );
                }
                if (vid_samples >= 2 && progressive_count >= vid_samples / 2)
                {
                    // We've already deduced that the frame rate is 23.976,
                    // so set it back again.
                    title->vrate.den = 1126125;
                    hb_deep_log( 2, "Title's mostly NTSC Film, setting fps to 23.976" );
                }
            }
            if (vid_samples >= 2 && doubled_frame_count >= 3 * vid_samples / 4)
            {
                // We've detected that a significant number of the frames
                // have been doubled in duration by repeat flags.
                title->vrate.den = 2 * vid_info.rate.den;
                hb_deep_log(2, "Repeat frames detected, setting fps to %.3f",
                            (float)title->vrate.num / title->vrate.den );
            }
        }
        title->video_bitrate = vid_info.bitrate;

        if (data->dvd || data->bd)
        {
            // DVD/BD doesn't have a container PAR, but it has container DAR
            // which can be used to compute container PAR
            hb_reduce(&title->geometry.par.num, &title->geometry.par.den,
                      title->geometry.height * title->container_dar.num,
                      title->geometry.width * title->container_dar.den);
        }
        if (vid_info.geometry.par.num && vid_info.geometry.par.den)
        {
            // title->geometry.par is initially container PAR, but
            // the video stream almost always also supplies PAR and
            // is generally more reliable, so use it.
            //
            // Check if container PAR and video stream PAR are in agreement
            if (title->geometry.par.num && title->geometry.par.den &&
                title->geometry.par.num != vid_info.geometry.par.num &&
                title->geometry.par.den != vid_info.geometry.par.den)
            {
                hb_log("WARNING: bitstream PAR %d:%d != container PAR %d:%d",
                       vid_info.geometry.par.num, vid_info.geometry.par.den,
                       title->geometry.par.num, title->geometry.par.den);
            }
            /*
             * Don't override container-level non-square
             * pixels with bitstream-level square pixels.
             *
             * Allows fixing absent bitstream PAR at the container level.
             *
             * Still prefer bitstream-level PAR when set, as e.g. mkvmerge will sadly round
             * it when muxing from elementary streams, making the bitstream PAR more precise:
             * 720x480 [SAR 32:27 DAR 16:9], SAR 853:720 DAR 853:480, 24 fps, 24 tbr, 1k tbn (default)
             */
            if (vid_info.geometry.par.num != 1 ||
                vid_info.geometry.par.den != 1 ||
                !title->geometry.par.num ||
                !title->geometry.par.den)
            {
                hb_log("using bitstream PAR %d:%d", vid_info.geometry.par.num, vid_info.geometry.par.den);
                title->geometry.par = vid_info.geometry.par;
            }
            else
            {
                hb_log("using container PAR %d:%d", title->geometry.par.num, title->geometry.par.den);
            }
        }
        else if (!title->geometry.par.num || !title->geometry.par.den)
        {
            // No video PAR found, assume 1:1
            title->geometry.par.num = title->geometry.par.den = 1;
        }
        title->pix_fmt = vid_info.pix_fmt;

        if ((title->color_prim     != HB_COLR_PRI_UNDEF &&
             title->color_prim     != HB_COLR_PRI_UNSET) ||
            (title->color_transfer != HB_COLR_TRA_UNDEF &&
             title->color_transfer != HB_COLR_TRA_UNSET) ||
            (title->color_matrix   != HB_COLR_MAT_UNDEF &&
             title->color_matrix != HB_COLR_MAT_UNSET))
        {
            title->color_prim     = get_color_prim(title->color_prim, vid_info.geometry, vid_info.rate);
            title->color_transfer = get_color_transfer(title->color_transfer);
            title->color_matrix   = get_color_matrix(title->color_matrix, vid_info.geometry);
        }
        // Let's try to guess a color profile only if the source is not Dolby Vision 5
        // which requires the values to be unset
        else if (title->dovi.dv_profile != 5)
        {
            title->color_prim     = get_color_prim(vid_info.color_prim, vid_info.geometry, vid_info.rate);
            title->color_transfer = get_color_transfer(vid_info.color_transfer);
            title->color_matrix   = get_color_matrix(vid_info.color_matrix, vid_info.geometry);
        }

        title->color_range = get_color_range(vid_info.color_range);
        title->chroma_location = vid_info.chroma_location;

        title->video_decode_support = vid_info.video_decode_support;

        // compute the aspect ratio based on the storage dimensions and PAR.
        hb_reduce(&title->dar.num, &title->dar.den,
                  title->geometry.par.num * title->geometry.width,
                  title->geometry.height * title->geometry.par.den);

        // For unknown reasons some French PAL DVDs put the original
        // content's aspect ratio into the mpeg PAR even though it's
        // the wrong PAR for the DVD. Apparently they rely on the fact
        // that DVD players ignore the content PAR and just use the
        // aspect ratio from the DVD metadata. So, if the aspect computed
        // from the PAR is different from the container's aspect we use
        // the container's aspect & recompute the PAR from it.
        if (data->dvd &&
            (title->dar.num != title->container_dar.num ||
             title->dar.den != title->container_dar.den))
        {
            hb_log("scan: content PAR gives wrong aspect %d:%d; "
                   "using container aspect %d:%d",
                   title->dar.num, title->dar.den,
                   title->container_dar.num, title->container_dar.den);
            title->dar = title->container_dar;
            hb_reduce(&title->geometry.par.num, &title->geometry.par.den,
                      title->geometry.height * title->dar.num,
                      title->geometry.width * title->dar.den);
        }

        // don't try to crop unless we got at least 3 previews
        if ( crops->n > 2 )
        {
            sort_crops( crops );

            // Available crop modes:
            // - Median: Selects median cropping - at least
            // 50% of the frames will have their borders removed.
            // - Loose cropping (i = 0) where
            // no non-black pixels will be cropped from any frame and a
            // - Tight cropping (i = crops->n - (crops->n >> 2)) where at
            // least 75% of the frames will have their borders removed.
            // - Smart: A blend between Median and Loose depending on whether 
            // mixed AR content is found.
            
            i = crops->n >> 1; // Median

            int crop_switch_frame_count = data->crop_threshold_frames;
            int less_than_median_crop_threshold = data->crop_threshold_pixels;
            
            if (crop_switch_frame_count == 0) 
            {
                // Values seem like sensible defaults.
                // Have observed that the optimal value does not always linearly increase with preview count.
                 crop_switch_frame_count = 4;
                
                if (data->preview_count >= 30){
                    crop_switch_frame_count = 6;
                }
                
                if (data->preview_count > 40){
                    crop_switch_frame_count = 8;
                }
            }
            
            if (less_than_median_crop_threshold == 0) 
            {
                // It's not uncommon to see 2~12 px variance in cropping.
                // Defaulting to 9 to account for that variance before switching to loose.
                // This accounts for variance that is unlikely to be caused by mixed AR. 
                less_than_median_crop_threshold = 9;
            }

            // Count the number of frames "substantially" less than the median.
            int less_than_median_frame_count = 0;
            for (int x = 0; x < crops->n; x++)
            {
                
                if (crops->t[x] < (crops->t[i] - less_than_median_crop_threshold) ||
                    crops->b[x] < (crops->b[i] - less_than_median_crop_threshold) ||
                    crops->l[x] < (crops->l[i] - less_than_median_crop_threshold) ||
                    crops->r[x] < (crops->r[i] - less_than_median_crop_threshold)){
                    less_than_median_frame_count = less_than_median_frame_count +1;
                }
                               
                 hb_deep_log(2, "crop: [%d] %d/%d/%d/%d", x, crops->t[x], crops->b[x],  crops->l[x], crops->r[x]);
            }
            
            hb_deep_log(2, "crop: less_than_median_frame_count: %d,", less_than_median_frame_count);
             
            // If we have a reasonable number of samples and it appears we have mixed aspect ratio, switch to loose crop.
            if (less_than_median_frame_count >= crop_switch_frame_count) 
            {
                hb_deep_log(2, "crop: switching to loose crop for this source. May be mixed aspect ratio. (%d)", crop_switch_frame_count);
                i = 0;
            }
            
            // Automatic "Smart" Crop.
            title->crop[0] = EVEN( crops->t[i] );
            title->crop[1] = EVEN( crops->b[i] );
            title->crop[2] = EVEN( crops->l[i] );
            title->crop[3] = EVEN( crops->r[i] );
            
            // Loose / Conservative  (i = 0)
            i = 0;
            title->loose_crop[0] = EVEN( crops->t[i] );
            title->loose_crop[1] = EVEN( crops->b[i] );
            title->loose_crop[2] = EVEN( crops->l[i] );
            title->loose_crop[3] = EVEN( crops->r[i] );
        }

        hb_log( "scan: %d previews, %dx%d, %.3f fps, autocrop = %d/%d/%d/%d, "
                "aspect %s, PAR %d:%d, color profile: %d-%d-%d, chroma location: %s",
                npreviews, title->geometry.width, title->geometry.height,
                (float)title->vrate.num / title->vrate.den,
                title->crop[0], title->crop[1], title->crop[2], title->crop[3],
                aspect_to_string(&title->dar),
                title->geometry.par.num, title->geometry.par.den,
                title->color_prim, title->color_transfer, title->color_matrix,
                av_chroma_location_name(title->chroma_location));

        if (title->mastering.has_primaries || title->mastering.has_luminance)
        {
            hb_log("scan: mastering display metadata: r(%5.4f,%5.4f) g(%5.4f,%5.4f) b(%5.4f %5.4f) wp(%5.4f, %5.4f) min_luminance=%f, max_luminance=%f",
                   hb_q2d(title->mastering.display_primaries[0][0]),
                   hb_q2d(title->mastering.display_primaries[0][1]),
                   hb_q2d(title->mastering.display_primaries[1][0]),
                   hb_q2d(title->mastering.display_primaries[1][1]),
                   hb_q2d(title->mastering.display_primaries[2][0]),
                   hb_q2d(title->mastering.display_primaries[2][1]),
                   hb_q2d(title->mastering.white_point[0]), hb_q2d(title->mastering.white_point[1]),
                   hb_q2d(title->mastering.min_luminance), hb_q2d(title->mastering.max_luminance));
        }

        if (title->coll.max_cll || title->coll.max_fall)
        {
            hb_log("scan: content light level: max_cll=%u, max_fall=%u",
                   title->coll.max_cll,
                   title->coll.max_fall);
        }

        if (title->ambient.ambient_illuminance.num)
        {
            hb_log("scan: ambient viewing environment: ambient_illuminance=%f, ambient_light_x=%f, ambient_light_x=%f",
                   hb_q2d(title->ambient.ambient_illuminance),
                   hb_q2d(title->ambient.ambient_light_x),
                   hb_q2d(title->ambient.ambient_light_y));
        }

        if (title->dovi.dv_profile > 0)
        {
            hb_log("scan: dolby vision configuration record: version: %d.%d, profile: %d, level: %d, rpu flag: %d, el flag: %d, bl flag: %d, compatibility id: %d",
                   title->dovi.dv_version_major,
                   title->dovi.dv_version_minor,
                   title->dovi.dv_profile,
                   title->dovi.dv_level,
                   title->dovi.rpu_present_flag,
                   title->dovi.el_present_flag,
                   title->dovi.bl_present_flag,
                   title->dovi.dv_bl_signal_compatibility_id);
        }

        if (title->hdr_10_plus)
        {
            hb_log("scan: hdr10+ dynamic metadata found");
        }

        if (title->video_decode_support != HB_DECODE_SUPPORT_SW)
        {
            hb_log("scan: supported video decoders:%s%s%s",
                   !(title->video_decode_support & HB_DECODE_SUPPORT_SW)      ? "" : " avcodec",
                   !(title->video_decode_support & HB_DECODE_SUPPORT_QSV)     ? "" : " qsv",
                   !(title->video_decode_support & HB_DECODE_SUPPORT_HWACCEL) ? "" : " hwaccel");
        }

        if (interlaced_preview_count && interlaced_preview_count >= (npreviews / 2))
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

    hb_buffer_list_close(&list_es);

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
static void LookForAudio(hb_scan_t *scan, hb_title_t * title, hb_buffer_t * b)
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

    hb_work_object_t *w = hb_audio_decoder(scan->h, audio->config.in.codec);

    if ( w == NULL || w->bsinfo == NULL )
    {
        hb_log( "Internal error in scan: unhandled audio type %d for id 0x%x",
                audio->config.in.codec, audio->id );
        goto drop_audio;
    }

    hb_work_info_t info;
    w->title = title;
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

    audio->config.in.samplerate = info.rate.num;
    audio->config.in.sample_bit_depth = info.sample_bit_depth;
    audio->config.in.samples_per_frame = info.samples_per_frame;
    audio->config.in.bitrate = info.bitrate;
    audio->config.in.matrix_encoding = info.matrix_encoding;
    audio->config.in.channel_layout = info.channel_layout;
    audio->config.in.channel_map = info.channel_map;
    audio->config.in.version = info.version;
    audio->config.in.flags = info.flags;
    audio->config.in.mode = info.mode;

    // now that we have all the info, set the audio description
    const char *codec_name   = NULL;
    const char *profile_name = NULL;
    if (audio->config.in.codec & HB_ACODEC_FF_MASK)
    {
        const AVCodec *codec = avcodec_find_decoder(audio->config.in.codec_param);
        if (codec != NULL)
        {
            if (info.profile != FF_PROFILE_UNKNOWN)
            {
                profile_name = av_get_profile_name(codec, info.profile);
            }

            // use our own capitalization for the most common codecs
            switch (audio->config.in.codec_param)
            {
                case AV_CODEC_ID_AAC:
                    codec_name = "AAC";
                    break;
                case AV_CODEC_ID_AC3:
                    codec_name = "AC3";
                    break;
                case AV_CODEC_ID_EAC3:
                    codec_name = "E-AC3";
                    break;
                case AV_CODEC_ID_TRUEHD:
                    codec_name = "TrueHD";
                    break;
                case AV_CODEC_ID_DTS:
                    if (profile_name == NULL &&
                        audio->config.in.codec == HB_ACODEC_DCA_HD)
                        codec_name = "DTS-HD";
                    else
                        codec_name = "DTS";
                    break;
                case AV_CODEC_ID_FLAC:
                    codec_name = "FLAC";
                    break;
                case AV_CODEC_ID_MP2:
                    codec_name = "MPEG";
                    break;
                case AV_CODEC_ID_MP3:
                    codec_name = "MP3";
                    break;
                case AV_CODEC_ID_PCM_BLURAY:
                    codec_name = "BD LPCM";
                    break;
                case AV_CODEC_ID_OPUS:
                    codec_name = "Opus";
                    break;
                case AV_CODEC_ID_VORBIS:
                    codec_name = "Vorbis";
                    break;
                default:
                    codec_name = codec->name;
                    break;
            }
            if (profile_name != NULL && codec_name != NULL &&
                strstr(profile_name, codec_name) != NULL)
            {
                codec_name = NULL;
            }
        }
        else
        {
            switch (audio->config.in.codec)
            {
                case HB_ACODEC_AC3:
                    codec_name = "AC3";
                    break;
                case HB_ACODEC_FFEAC3:
                    codec_name = "E-AC3";
                    break;
                case HB_ACODEC_FFTRUEHD:
                    codec_name = "TrueHD";
                    break;
                case HB_ACODEC_DCA:
                    codec_name = "DTS";
                    break;
                case HB_ACODEC_DCA_HD:
                    codec_name = "DTS-HD";
                    break;
                case HB_ACODEC_FFAAC:
                    codec_name = "AAC";
                    break;
                case HB_ACODEC_FFFLAC:
                    codec_name = "FLAC";
                    break;
                case HB_ACODEC_MP2:
                    codec_name = "MPEG";
                    break;
                case HB_ACODEC_MP3:
                    codec_name = "MP3";
                    break;
                default:
                    codec_name = "Unknown (libav)";
                    break;
            }
        }
    }
    else
    {
        switch (audio->config.in.codec)
        {
            case HB_ACODEC_AC3:
                codec_name = "AC3";
                break;
            case HB_ACODEC_LPCM:
                codec_name = "LPCM";
                break;
            default:
                codec_name = "Unknown";
                break;
        }
    }
    if (codec_name != NULL && profile_name != NULL)
    {
        snprintf(audio->config.lang.description, sizeof(audio->config.lang.description),
                "%s (%s %s", audio->config.lang.simple, codec_name, profile_name);
    }
    else if (codec_name != NULL)
    {
        snprintf(audio->config.lang.description, sizeof(audio->config.lang.description),
                "%s (%s", audio->config.lang.simple, codec_name);
    }
    else if (profile_name != NULL)
    {
        snprintf(audio->config.lang.description, sizeof(audio->config.lang.description),
                "%s (%s", audio->config.lang.simple, profile_name);
    }

    if (audio->config.lang.attributes & HB_AUDIO_ATTR_VISUALLY_IMPAIRED)
    {
        strncat(audio->config.lang.description, ", Visually Impaired",
                sizeof(audio->config.lang.description) -
                strlen(audio->config.lang.description) - 1);
    }
    if (audio->config.lang.attributes & HB_AUDIO_ATTR_COMMENTARY)
    {
        strncat(audio->config.lang.description, ", Director's Commentary 1",
                sizeof(audio->config.lang.description) -
                strlen(audio->config.lang.description) - 1);
    }
    if (audio->config.lang.attributes & HB_AUDIO_ATTR_ALT_COMMENTARY)
    {
        strncat(audio->config.lang.description, ", Director's Commentary 2",
                sizeof(audio->config.lang.description) -
                strlen(audio->config.lang.description) - 1);
    }
    if (audio->config.lang.attributes & HB_AUDIO_ATTR_SECONDARY)
    {
        strncat(audio->config.lang.description, ", Secondary",
                sizeof(audio->config.lang.description) -
                strlen(audio->config.lang.description) - 1);
    }

    if (audio->config.in.channel_layout)
    {
        int lfes     = (!!(audio->config.in.channel_layout & AV_CH_LOW_FREQUENCY) +
                        !!(audio->config.in.channel_layout & AV_CH_LOW_FREQUENCY_2));
        int channels = hb_layout_get_discrete_channel_count(audio->config.in.channel_layout);
        char *desc   = audio->config.lang.description +
                        strlen(audio->config.lang.description);
        size_t size = sizeof(audio->config.lang.description) - strlen(audio->config.lang.description);
        snprintf(desc, size, ", %d.%d ch", channels - lfes, lfes);

        // describe the matrix encoding mode, if any
        switch (audio->config.in.matrix_encoding)
        {
            case AV_MATRIX_ENCODING_DOLBY:
                if (audio->config.in.codec       == HB_ACODEC_AC3    ||
                    audio->config.in.codec_param == AV_CODEC_ID_AC3  ||
                    audio->config.in.codec_param == AV_CODEC_ID_EAC3 ||
                    audio->config.in.codec_param == AV_CODEC_ID_TRUEHD)
                {
                    strcat(audio->config.lang.description, ", Dolby Surround");
                    break;
                }
                strcat(audio->config.lang.description, ", Lt/Rt)");
                break;
            case AV_MATRIX_ENCODING_DPLII:
                strcat(audio->config.lang.description, ", Dolby Pro Logic II");
                break;
            case AV_MATRIX_ENCODING_DPLIIX:
                strcat(audio->config.lang.description, ", Dolby Pro Logic IIx");
                break;
            case AV_MATRIX_ENCODING_DPLIIZ:
                strcat(audio->config.lang.description, ", Dolby Pro Logic IIz");
                break;
            case AV_MATRIX_ENCODING_DOLBYEX:
                strcat(audio->config.lang.description, ", Dolby Digital EX");
                break;
            case AV_MATRIX_ENCODING_DOLBYHEADPHONE:
                strcat(audio->config.lang.description, ", Dolby Headphone");
                break;
            default:
                break;
        }
    }

    // Append input bitrate in kbps to the end of the description if greater than 1
    // ffmpeg may report some audio bitrates as 1, not an issue
    if (audio->config.in.bitrate > 1)
    {
        char in_bitrate_str[19];
        snprintf(in_bitrate_str, 18, ", %d kbps", audio->config.in.bitrate / 1000);
        strncat(audio->config.lang.description, in_bitrate_str,
                sizeof(audio->config.lang.description) - strlen(audio->config.lang.description) - 1);
    }
    strcat(audio->config.lang.description, ")");

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
    
    int is_multi_file = hb_list_count(scan->paths) > 0;

    hb_get_state2(scan->h, &state);
#define p state.param.scanning
    /* Update the UI */
    state.state   = HB_STATE_SCANNING;
    p.title_cur   = title;
    p.title_count = scan->dvd ? hb_dvd_title_count( scan->dvd ) :
                    scan->bd ? hb_bd_title_count( scan->bd ) :
                    scan->batch ? hb_batch_title_count( scan->batch ) :
                    is_multi_file ? hb_list_count(scan->paths) :
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

    hb_get_state2(scan->h, &state);
#define p state.param.scanning
    /* Update the UI */
    state.state   = HB_STATE_SCANNING;
    p.title_cur   = title;
    p.title_count = hb_list_count( scan->title_set->list_title );
    p.preview_cur = 1;
    p.preview_count = scan->preview_count;
    if (scan->title_index)
        p.progress = (float)(p.title_cur - 1) / p.title_count;
    else
        p.progress = 0.5 + 0.5 * (float)(p.title_cur - 1) / p.title_count;
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
