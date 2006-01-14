/* $Id: Scan.c,v 1.4 2003/11/06 13:03:19 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "Fifo.h"
#include "Languages.h"
#include "Scan.h"
#include "Thread.h"

#include <dvdread/ifo_types.h>
#include <dvdplay/dvdplay.h>
#include <dvdplay/info.h>
#include <dvdplay/state.h>
#include <dvdplay/nav.h>

#include <mpeg2dec/mpeg2.h>

/* Local prototypes */
static void      ScanThread( void * );
static HBTitle * ScanTitle( HBScan *, dvdplay_ptr vmg, int index );
static int       DecodeFrame( HBScan * s, dvdplay_ptr vmg,
                              HBTitle * title, int which );
static char    * LanguageForCode( int code );

struct HBScan
{
    HBHandle * handle;
    char     * device;
    int        title;
    int        die;
    HBThread * thread;
};

HBScan * HBScanInit( HBHandle * handle, char * device, int title )
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
    HBScan * s = (HBScan*) _s;
    dvdplay_ptr vmg;
    HBList  * titleList = HBListInit();
    HBTitle * title;
    int i;

    HBLog( "HBScan: opening device %s", s->device );
    HBScanning( s->handle, 0 );

    vmg = dvdplay_open( s->device, NULL, NULL );
    if( !vmg )
    {
        HBLog( "HBScan: dvdplay_open() failed (%s)", s->device );
        HBScanDone( s->handle, titleList );
        return;
    }

    /* Detect titles */
    for( i = ( s->title ? s->title - 1 : 0 );
         i < ( s->title ? s->title : dvdplay_title_nr( vmg ) );
         i++ )
    {
        if( s->die )
        {
            break;
        }

        if( !( title = ScanTitle( s, vmg, i + 1 ) ) )
        {
            continue;
        }

        HBListAdd( titleList, title );
    }

    HBLog( "HBScan: closing device %s", s->device );
    dvdplay_close( vmg );

    HBScanDone( s->handle, titleList );
}

static HBTitle * ScanTitle( HBScan * s, dvdplay_ptr vmg, int index )
{
    HBTitle * title;
    int audio_nr, foo;
    audio_attr_t * attr;
    HBAudio * audio;
    int i;
    uint8_t dummy[DVD_VIDEO_LB_LEN];
    
    HBLog( "HBScan: scanning title %d", index );
    HBScanning( s->handle, index );

    title = HBTitleInit( s->device, index );
    dvdplay_start( vmg, index );

    /* Length */
    title->length = dvdplay_title_time( vmg );
    HBLog( "HBScan: title length is %d seconds", title->length );

    /* Discard titles under 10 seconds */
    if( title->length < 10 )
    {
        HBLog( "HBScan: ignoring title %d (too short)", index );
        HBTitleClose( &title );
        return NULL;
    }

    /* Detect languages */
    dvdplay_audio_info( vmg, &audio_nr, &foo );

    for( i = 0; i < audio_nr; i++ )
    {
        int id;
        int j;
        
        if( s->die )
        {
            break;
        }

        id = dvdplay_audio_id( vmg, i );

        if( id < 1 )
        {
            continue;
        }

        if( ( id & 0xFF ) != 0xBD )
        {
            HBLog( "HBScan: non-AC3 audio track detected, ignoring" );
            continue;
        }

        /* Check if we don't already found an track with the same id */
        audio = NULL;
        for( j = 0; j < HBListCountItems( title->audioList ); j++ )
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
            HBLog( "HBScan: discarding duplicate track %x", id );
            continue;
        }

        attr = dvdplay_audio_attr( vmg, j );
        audio = HBAudioInit( id, LanguageForCode( attr->lang_code ) );
        HBLog( "HBScan: new language (%x, %s)", id, audio->language );
        HBListAdd( title->audioList, audio );
    }

    /* Discard titles with no audio tracks */
    if( !HBListCountItems( title->audioList ) )
    {
        HBLog( "HBScan: ignoring title %d (no audio track)", index );
        HBTitleClose( &title );
        return NULL;
    }

    /* Kludge : libdvdplay wants we to read a first block before seeking */
    dvdplay_read( vmg, dummy, 1 );


    for( i = 0; i < 10; i++ )
    {
        if( s->die )
        {
            break;
        }

        if( !DecodeFrame( s, vmg, title, i ) )
        {
            HBLog( "HBScan: ignoring title %d (could not decode)",
                   index );
            HBTitleClose( &title );
            return NULL;
        }
    }

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

static int DecodeFrame( HBScan * s, dvdplay_ptr vmg,
                        HBTitle * title, int which )
{
    int titleFirst   = dvdplay_title_first( vmg );
    int titleEnd     = dvdplay_title_end( vmg );
    int pictureStart = ( which + 1 ) * ( titleEnd - titleFirst ) / 11;
    int pictureEnd   = titleFirst + ( which + 2 ) *
                       ( titleEnd - titleFirst ) / 11;

    mpeg2dec_t         * handle;
    const mpeg2_info_t * info;
    mpeg2_state_t        state;
    char fileName[1024];
    FILE * file;

    HBList   * esBufferList = HBListInit();
    HBBuffer * psBuffer     = NULL;
    HBBuffer * esBuffer     = NULL;

    /* Seek to the right place */
    dvdplay_seek( vmg, pictureStart );

    /* Init libmpeg2 */
    handle = mpeg2_init();
    info   = mpeg2_info( handle );
                        
    /* Init the destination file */
    memset( fileName, 0, 1024 );
    sprintf( fileName, "/tmp/HB.%d.%d.%d", HBGetPid( s->handle ), 
             title->index, which );
    file = fopen( fileName, "w" );

#define CLEANUP \
    while( ( esBuffer = (HBBuffer*) HBListItemAt( esBufferList, 0 ) ) ) \
    { \
        HBListRemove( esBufferList, esBuffer ); \
        HBBufferClose( &esBuffer ); \
    } \
    HBListClose( &esBufferList ); \
    if( psBuffer ) HBBufferClose( &psBuffer ); \
    if( esBuffer ) HBBufferClose( &esBuffer ); \
    mpeg2_close( handle ); \
    fclose( file )
                  
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
            while( !esBuffer )
            {
                while( !HBListCountItems( esBufferList ) )
                {
                    psBuffer = HBBufferInit( DVD_VIDEO_LB_LEN );
                    if( dvdplay_read( vmg, psBuffer->data, 1 ) != 1 ||
                            !HBPStoES( &psBuffer, esBufferList ) )
                    {
                        HBLog( "HBScan: failed to get a valid PS "
                               "packet" );
                        CLEANUP;
                        return 0;
                    }

                    if( dvdplay_position( vmg ) >= pictureEnd )
                    {
                        HBLog( "HBScan: gone too far, aborting" );
                        CLEANUP;
                        return 0;
                    }
                }

                esBuffer = (HBBuffer*) HBListItemAt( esBufferList, 0 );
                HBListRemove( esBufferList, esBuffer );

                if( esBuffer->streamId != 0xE0 )
                {
                    HBBufferClose( &esBuffer );
                }
            }

            /* Feed libmpeg2 */
            mpeg2_buffer( handle, esBuffer->data,
                          esBuffer->data + esBuffer->size );
        }
        else if( state == STATE_SEQUENCE )
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
        }
        else if( ( state == STATE_SLICE || state == STATE_END ) &&
                 ( info->display_fbuf ) &&
                 ( info->display_picture->flags & PIC_MASK_CODING_TYPE )
                     == PIC_FLAG_CODING_TYPE_I )
        {
            /* Write the raw picture to a file */
            fwrite( info->display_fbuf->buf[0],
                    title->inWidth * title->inHeight, 1, file );
            fwrite( info->display_fbuf->buf[1],
                    title->inWidth * title->inHeight / 4, 1, file );
            fwrite( info->display_fbuf->buf[2],
                    title->inWidth * title->inHeight / 4, 1, file );
            break;
        }
        else if( state == STATE_INVALID )
        {
            /* Reset libmpeg2 */
            mpeg2_close( handle );
            handle = mpeg2_init();
        }
    }

    CLEANUP;

    return 1;

#undef CLEANUP
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

