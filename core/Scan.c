/* $Id: Scan.c,v 1.26 2004/05/12 18:02:35 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"
#include "Languages.h"

#include "dvdread/ifo_read.h"

#include "mpeg2dec/mpeg2.h"

/* Local prototypes */
static void      ScanThread( void * );
static HBTitle * ScanTitle( HBScan *, dvd_reader_t * reader,
                            ifo_handle_t * vmg, int index );
static int       DecodeFrame( HBScan * s, dvd_file_t * dvdFile,
                              HBTitle * title, int which );
static char    * LanguageForCode( int code );

struct HBScan
{
    HBHandle     * handle;
    char         * device;
    int            title;
    volatile int   die;
    HBThread     * thread;
    HBList       * titleList;
};

HBScan * HBScanInit( HBHandle * handle, const char * device, int title )
{
    HBScan * s;
    if( !( s = malloc( sizeof( HBScan ) ) ) )
    {
        HBLog( "HBScanInit: malloc() failed, gonna crash" );
        return NULL;
    }

    s->handle = handle;
    s->device = strdup( device );
    s->title  = title;
    s->die    = 0;
    s->thread = HBThreadInit( "scan", ScanThread, s,
                              HB_NORMAL_PRIORITY );

    return s;
}

void HBScanClose( HBScan ** _s )
{
    HBScan * s = *_s;

    s->die = 1;
    HBThreadClose( &s->thread );

    free( s->device );
    free( s );
    *_s = NULL;
}

static void ScanThread( void * _s )
{
    int            i;
    HBScan       * s = (HBScan*) _s;
    HBList       * titleList = HBListInit();
    HBTitle      * title;
    dvd_reader_t * reader;
    ifo_handle_t * vmg;

    s->titleList = titleList;

    HBLog( "HBScan: opening device %s", s->device );

    reader = DVDOpen( s->device );
    if( !reader )
    {
        HBLog( "HBScan: DVDOpen() failed (%s)", s->device );
        HBListClose( &titleList );
        HBScanDone( s->handle, NULL );
        return;
    }

    vmg = ifoOpen( reader, 0 );

    /* Detect titles */
    i = s->title ? ( s->title - 1 ) : 0;
    while( !s->die )
    {
        if( ( title = ScanTitle( s, reader, vmg, i + 1 ) ) )
        {
            HBListAdd( titleList, title );
        }
        if( s->title || i == vmg->tt_srpt->nr_of_srpts - 1 )
        {
            break;
        }
        i++;
    }

    ifoClose( vmg );

    HBLog( "HBScan: closing device %s", s->device );
    DVDClose( reader );

    if( s->die )
    {
        while( ( title = HBListItemAt( titleList, 0 ) ) )
        {
            HBListRemove( titleList, title );
            HBTitleClose( &title );
        }
        HBListClose( &titleList );
        return;
    }

    if( !HBListCount( titleList ) )
    {
        HBListClose( &titleList );
    }
    HBScanDone( s->handle, titleList );
}


static unsigned int convert_bcd( unsigned int i_x )
{
    int y = 0, z = 1;

    for( ; i_x ; )
    {
        y += z * ( i_x & 0xf );
        i_x = i_x >> 4;
        z = z * 10;
    }

    return y;
}

static HBTitle * ScanTitle( HBScan * s, dvd_reader_t * reader,
                            ifo_handle_t * vmg, int index )
{
    HBTitle      * title;
    HBTitle      * title2;
    HBAudio      * audio;
    int            i, audio_nr;
    int ttn;
    ifo_handle_t * vts;
    int pgc_id, pgn, cell;
    pgc_t * pgc;
    dvd_file_t * dvdFile;

    HBScanning( s->handle, index, vmg->tt_srpt->nr_of_srpts );

    title = HBTitleInit( s->device, index );

    /* VTS in which our title is */
    title->vts_id = vmg->tt_srpt->title[index-1].title_set_nr;

    vts = ifoOpen( reader, title->vts_id );
    if( !vts )
    {
        HBLog( "HBScan: ifoOpen failed (vts %d)", title->vts_id );
        HBTitleClose( &title );
        return NULL;
    }

    /* Position of the title in the VTS */
    ttn = vmg->tt_srpt->title[index-1].vts_ttn;

    /* Get pgc */
    pgc_id = vts->vts_ptt_srpt->title[ttn-1].ptt[0].pgcn;
    pgn    = vts->vts_ptt_srpt->title[ttn-1].ptt[0].pgn;
    pgc    = vts->vts_pgcit->pgci_srp[pgc_id-1].pgc;

    /* Start block */
    cell = pgc->program_map[pgn-1] - 1;
    title->startBlock = pgc->cell_playback[cell].first_sector;

    /* End block */
    cell = pgc->nr_of_cells - 1;
    title->endBlock = pgc->cell_playback[cell].last_sector;

    HBLog( "HBScan: vts=%d, ttn=%d, blocks %d to %d", title->vts_id,
           ttn, title->startBlock, title->endBlock );

    /* I've seen a DVD with strictly identical titles. Check this here,
       and ignore it if redundant */
    title2 = NULL;
    for( i = 0; i < HBListCount( s->titleList ); i++ )
    {
        title2 = (HBTitle*) HBListItemAt( s->titleList, i );
        if( title->vts_id == title2->vts_id &&
            title->startBlock == title2->startBlock &&
            title->endBlock == title2->endBlock )
        {
            break;
        }
        else
        {
            title2 = NULL;
        }
    }
    if( title2 )
    {
        HBLog( "HBScan: title %d is duplicate with title %d",
               index, title2->title );
        HBTitleClose( &title );
        return NULL;
    }

    /* Get time */
    title->hours   = convert_bcd( pgc->playback_time.hour );
    title->minutes = convert_bcd( pgc->playback_time.minute );
    title->seconds = convert_bcd( pgc->playback_time.second );
    HBLog( "HBScan: title %d: length is %02d:%02d:%02d", index,
           title->hours, title->minutes, title->seconds );

    /* Discard titles under 10 seconds */
    if( !title->hours && !title->minutes && title->seconds < 10 )
    {
        HBLog( "HBScan: ignoring title %d (too short)", index );
        HBTitleClose( &title );
        return NULL;
    }

    /* Detect languages */
    audio_nr = vts->vtsi_mat->nr_of_vts_audio_streams;

    for( i = 0; i < audio_nr; i++ )
    {
        uint32_t id = 0;
        int j, codec;
        int audio_format = vts->vtsi_mat->vts_audio_attr[i].audio_format;
        int lang_code = vts->vtsi_mat->vts_audio_attr[i].lang_code;
        int audio_control = vts->vts_pgcit->pgci_srp[pgc_id-1].pgc->audio_control[i];
        int i_position;

        if( s->die )
        {
            break;
        }

        if( !( audio_control & 0x8000 ) )
        {
            continue;
        }

        i_position = ( audio_control & 0x7F00 ) >> 8;

        switch( audio_format )
        {
            case 0x00: /* A52 */
                codec = HB_CODEC_AC3;
                id    = ( ( 0x80 + i_position ) << 8 ) | 0xbd;
                break;

            case 0x02:
            case 0x03:
                codec = HB_CODEC_MPGA;
                id    = 0xc0 + i_position;
                break;

            case 0x04: /* LPCM */
                codec = HB_CODEC_LPCM;
                id    = ( ( 0xa0 + i_position ) << 8 ) | 0xbd;
                break;

            default:
                codec = 0;
                id    = 0;
                HBLog( "HBScan: title %d: unknown audio codec (%x), "
                       "ignoring", index, audio_format );
                break;
        }

        if( !id )
        {
            continue;
        }

        /* Check if we don't already found an track with the same id */
        audio = NULL;
        for( j = 0; j < HBListCount( title->audioList ); j++ )
        {
            audio = (HBAudio*) HBListItemAt( title->audioList, j );
            if( id == audio->id )
            {
                break;
            }
            else
            {
                audio = NULL;
            }
        }

        if( audio )
        {
            HBLog( "HBScan: title %d: discarding duplicate track %x",
                   index, id );
            continue;
        }

        audio = HBAudioInit( id, LanguageForCode( lang_code ), codec );
        audio->inCodec = codec;
        HBLog( "HBScan: title %d: new language (%x, %s)", index, id,
               audio->language );
        HBListAdd( title->audioList, audio );
    }

    /* Discard titles with no audio tracks */
    if( !HBListCount( title->audioList ) )
    {
        HBLog( "HBScan: ignoring title %d (no audio track)", index );
        HBTitleClose( &title );
        return NULL;
    }

    ifoClose( vts );

    dvdFile = DVDOpenFile( reader, title->vts_id, DVD_READ_TITLE_VOBS );
    if( !dvdFile )
    {
        HBLog( "HBScan: DVDOpenFile failed" );
        HBTitleClose( &title );
        return NULL;
    }

    for( i = 0; i < 10; i++ )
    {
        if( s->die )
        {
            break;
        }

        if( !DecodeFrame( s, dvdFile, title, i ) )
        {
            HBLog( "HBScan: ignoring title %d (could not decode)",
                   index );
            HBTitleClose( &title );
            return NULL;
        }
    }

    DVDCloseFile( dvdFile );

    /* Handle ratio */
    if( title->inHeight * title->aspect >
            title->inWidth * VOUT_ASPECT_FACTOR )
    {
        title->outWidthMax  = title->inWidth;
        title->outHeightMax = MULTIPLE_16( (uint64_t)title->inWidth *
                               VOUT_ASPECT_FACTOR / title->aspect );
    }
    else
    {
        title->outWidthMax  = MULTIPLE_16( (uint64_t)title->inHeight *
                               title->aspect / VOUT_ASPECT_FACTOR );
        title->outHeightMax = title->inHeight;
    }

    /* Default picture size */
    title->outWidth  = title->outWidthMax;
    title->outHeight = title->outHeightMax;

    return title;
}

static HBBuffer * GetBuffer( HBList * esBufferList,
                             dvd_file_t * dvdFile,
                             int * pictureStart, int pictureEnd )
{
    HBBuffer * esBuffer = NULL;
    HBBuffer * psBuffer = NULL;

    while( !esBuffer )
    {
        while( !HBListCount( esBufferList ) )
        {
            psBuffer = HBBufferInit( DVD_VIDEO_LB_LEN );
            if( DVDReadBlocks( dvdFile, (*pictureStart)++, 1,
                               psBuffer->data ) != 1 )
            {
                HBLog( "HBScan: DVDReadBlocks() failed" );
                HBBufferClose( &psBuffer );
                return NULL;
            }
            if( !HBPStoES( &psBuffer, esBufferList ) )
            {
                HBLog( "HBScan: HBPStoES() failed" );
                return NULL;
            }
            if( *pictureStart >= pictureEnd )
            {
                HBLog( "HBScan: gone too far, aborting" );
                return NULL;
            }
        }

        esBuffer = (HBBuffer*) HBListItemAt( esBufferList, 0 );
        HBListRemove( esBufferList, esBuffer );

        if( esBuffer->streamId != 0xE0 )
        {
            HBBufferClose( &esBuffer );
        }
    }

    return esBuffer;
}

static int DecodeFrame( HBScan * s, dvd_file_t * dvdFile,
                        HBTitle * title, int which )
{
    int pictureStart = title->startBlock + ( which + 1 ) *
        ( title->endBlock - title->startBlock ) / 11;
    int pictureEnd   = title->startBlock + ( which + 2 ) *
        ( title->endBlock - title->startBlock ) / 11;

    mpeg2dec_t         * handle;
    const mpeg2_info_t * info;
    mpeg2_state_t        state;
    char                 fileName[1024];
    FILE               * file;
    int                  ret = 0;

    HBList   * esBufferList = HBListInit();
    HBBuffer * esBuffer     = NULL;


    /* Init libmpeg2 */
    handle = mpeg2_init();
    info   = mpeg2_info( handle );

    /* Init the destination file */
    memset( fileName, 0, 1024 );
#ifndef HB_CYGWIN
    sprintf( fileName, "/tmp/HB.%d.%d.%d", HBGetPid( s->handle ),
             title->title, which );
#else
    sprintf( fileName, "C:\\HB.%d.%d.%d", HBGetPid( s->handle ),
             title->title, which );
#endif
    file = fopen( fileName, "wb" );
    if( !file )
    {
        HBLog( "HBScan: fopen failed" );
        HBListClose( &esBufferList );
        return 0;
    }

    for( ;; )
    {
        state = mpeg2_parse( handle );

        if( state == STATE_BUFFER )
        {
            /* Free the previous buffer */
            if( esBuffer )
            {
                HBBufferClose( &esBuffer );
            }

            /* Get a new one */
            esBuffer = GetBuffer( esBufferList, dvdFile, &pictureStart,
                                  pictureEnd );
            if( !esBuffer )
            {
                break;
            }

            /* Feed libmpeg2 */
            mpeg2_buffer( handle, esBuffer->data,
                          esBuffer->data + esBuffer->size );
        }
        else if( state == STATE_SEQUENCE )
        {
            if( !which )
            {
                /* Get size & framerate info */
                title->inWidth     = info->sequence->width;
                title->inHeight    = info->sequence->height;
                title->aspect      = (uint64_t)info->sequence->display_width *
                    info->sequence->pixel_width * VOUT_ASPECT_FACTOR /
                    ( info->sequence->display_height *
                      info->sequence->pixel_height );
                title->rate        = 27000000;
                title->rateBase    = info->sequence->frame_period;

                title->autoTopCrop    = title->inHeight / 2;
                title->autoBottomCrop = title->inHeight / 2;
                title->autoLeftCrop   = title->inWidth / 2;
                title->autoRightCrop  = title->inWidth / 2;
            }
        }
        else if( ( state == STATE_SLICE || state == STATE_END ) &&
                 ( info->display_fbuf ) &&
                 ( info->display_picture->flags & PIC_MASK_CODING_TYPE )
                     == PIC_FLAG_CODING_TYPE_I )
        {
#define Y info->display_fbuf->buf[0]
#define DARK 64
            /* Detect black borders */
            int i, j;
            for( i = 0; i < title->inWidth; i++ )
            {
                for( j = 0; j < title->autoTopCrop; j++ )
                    if( Y[ j * title->inWidth + i ] > DARK )
                    {
                        title->autoTopCrop = j;
                        break;
                    }
                for( j = 0; j < title->autoBottomCrop; j++ )
                    if( Y[ ( title->inHeight - j - 1 ) *
                           title->inWidth + i ] > DARK )
                    {
                        title->autoBottomCrop = j;
                        break;
                    }
            }
            for( i = 0; i < title->inHeight; i++ )
            {
                for( j = 0; j < title->autoLeftCrop; j++ )
                    if( Y[ i * title->inWidth + j ] > DARK )
                    {
                        title->autoLeftCrop = j;
                        break;
                    }
                for( j = 0; j < title->autoRightCrop; j++ )
                    if( Y[ i * title->inWidth +
                            title->inWidth - j - 1 ] > DARK )
                    {
                        title->autoRightCrop = j;
                        break;
                    }
            }
#undef Y
#undef DARK

            /* Write the raw picture to a file */
            fwrite( info->display_fbuf->buf[0],
                    title->inWidth * title->inHeight, 1, file );
            fwrite( info->display_fbuf->buf[1],
                    title->inWidth * title->inHeight / 4, 1, file );
            fwrite( info->display_fbuf->buf[2],
                    title->inWidth * title->inHeight / 4, 1, file );
            ret = 1;
            break;
        }
        else if( state == STATE_INVALID )
        {
            /* Reset libmpeg2 */
            mpeg2_close( handle );
#ifdef HB_NOMMX
            mpeg2_accel( 0 );
#endif
            handle = mpeg2_init();
        }
    }

    while( ( esBuffer = (HBBuffer*) HBListItemAt( esBufferList, 0 ) ) )
    {
        HBListRemove( esBufferList, esBuffer );
        HBBufferClose( &esBuffer );
    }
    HBListClose( &esBufferList );
    if( esBuffer ) HBBufferClose( &esBuffer );
    mpeg2_close( handle );
    fclose( file );

    return ret;
}

static char * LanguageForCode( int code )
{
    char codeString[2];
    iso639_lang_t * lang;

    codeString[0] = ( code >> 8 ) & 0xFF;
    codeString[1] = code & 0xFF;

    for( lang = languages; lang->engName; lang++ )
    {
        if( !strncmp( lang->iso639_1, codeString, 2 ) )
        {
            if( *lang->nativeName )
            {
                return lang->nativeName;
            }

            return lang->engName;
        }
    }

    return "Unknown";
}

