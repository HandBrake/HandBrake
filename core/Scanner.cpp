/* $Id: Scanner.cpp,v 1.23 2003/10/13 14:12:18 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "Scanner.h"
#include "Manager.h"
#include "Fifo.h"
#include "MpegDemux.h"

#include <dvdread/ifo_types.h>
#include <dvdplay/dvdplay.h>
#include <dvdplay/info.h>
#include <dvdplay/state.h>
#include <dvdplay/nav.h>

extern "C" {
#include <mpeg2dec/mpeg2.h>
}

HBScanner::HBScanner( HBManager * manager, char * device )
    : HBThread( "scanner", HB_NORMAL_PRIORITY )
{
    fManager    = manager;
    fDevice     = strdup( device );

    Run();
}

void HBScanner::DoWork()
{
    Log( "HBScanner: opening device %s", fDevice );

    dvdplay_ptr vmg;
    vmg = dvdplay_open( fDevice, NULL, NULL );
    if( !vmg )
    {
        Log( "HBScanner: dvdplay_open() failed (%s)",
             fDevice );
        fManager->ScanDone( NULL );
        return;
    }

    /* Detect titles */
    HBList  * titleList = new HBList();
    HBTitle * title;
    for( int i = 0; i < dvdplay_title_nr( vmg ); i++ )
    {
        if( fDie )
        {
            break;
        }

        Log( "HBScanner: scanning title %d", i + 1 );
        fManager->Scanning( fDevice, i + 1 );

        title = new HBTitle( fDevice, i + 1 );

        if( ScanTitle( title, vmg ) )
        {
            titleList->AddItem( title );
        }
        else
        {
            Log( "HBScanner: ignoring title %d", i + 1 );
            delete title;
        }
    }

    Log( "HBScanner: closing device %s", fDevice );
    dvdplay_close( vmg );

    fManager->ScanDone( titleList );
}

bool HBScanner::ScanTitle( HBTitle * title, dvdplay_ptr vmg )
{
    dvdplay_start( vmg, title->fIndex );

    /* Length */
    title->fLength = dvdplay_title_time( vmg );
    Log( "HBScanner::ScanTitle: title length is %lld seconds",
         title->fLength );

    /* Discard titles under 10 seconds */
    if( title->fLength < 10 )
    {
        return false;
    }

    /* Detect languages */
    int audio_nr, foo;
    dvdplay_audio_info( vmg, &audio_nr, &foo );

    audio_attr_t * attr;
    HBAudio * audio;
    for( int i = 0; i < audio_nr; i++ )
    {
        if( fDie )
        {
            break;
        }

        int id = dvdplay_audio_id( vmg, i );

        if( id < 1 )
        {
            continue;
        }

        if( ( id & 0xFF ) != 0xBD )
        {
            Log( "HBScanner::ScanTitle: non-AC3 audio track "
                 "detected, ignoring" );
            continue;
        }

        /* Check if we don't already found an track with the same id */
        audio = NULL;
        for( uint32_t j = 0; j < title->fAudioList->CountItems(); j++ )
        {
            audio = (HBAudio*) title->fAudioList->ItemAt( j );
            if( (uint32_t) id == audio->fId )
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
            Log( "HBScanner::ScanTitle: discarding duplicate track %x",
                  id );
            continue;
        }
        
        attr = dvdplay_audio_attr( vmg, i );
        audio = new HBAudio( id, LanguageForCode( attr->lang_code ) );
        Log( "HBScanner::ScanTitle: new language (%x, %s)",
             id, audio->fDescription );
        title->fAudioList->AddItem( audio );
    }

    /* Discard titles with no audio tracks */
    if( !title->fAudioList->CountItems() )
    {
        return false;
    }

    /* Kludge : libdvdplay wants we to read a first block before seeking */
    uint8_t dummyBuf[DVD_VIDEO_LB_LEN];
    dvdplay_read( vmg, dummyBuf, 1 );

    for( int i = 0; i < 10; i++ )
    {
        if( fDie )
        {
            break;
        }

        if( !DecodeFrame( title, vmg, i ) )
        {
            return false;
        }
    }

    if( title->fInHeight * title->fAspect >
            title->fInWidth * VOUT_ASPECT_FACTOR )
    {
        title->fOutWidthMax  = title->fInWidth;
        title->fOutHeightMax = MULTIPLE_16( (uint64_t)title->fInWidth *
                               VOUT_ASPECT_FACTOR / title->fAspect );
    }
    else
    {
        title->fOutWidthMax  = MULTIPLE_16( (uint64_t)title->fInHeight *
                               title->fAspect / VOUT_ASPECT_FACTOR );
        title->fOutHeightMax = title->fInHeight;
    }

    /* Default picture size */
    title->fOutWidth  = title->fOutWidthMax;
    title->fOutHeight = title->fOutHeightMax;

    return true;
}
bool HBScanner::DecodeFrame( HBTitle * title, dvdplay_ptr vmg, int i )
{
    /* Seek to the right place */
    int titleFirst = dvdplay_title_first ( vmg );
    int titleEnd   = dvdplay_title_end( vmg );

    dvdplay_seek( vmg, ( i + 1 ) * ( titleEnd - titleFirst ) / 11 ) ;

    /* Init libmpeg2 */
    mpeg2dec_t         * handle = mpeg2_init();
    const mpeg2_info_t * info   = mpeg2_info( handle );
    mpeg2_state_t        state;

    /* Init the destination file */
    char fileName[1024];
    memset( fileName, 0, 1024 );
    sprintf( fileName, "/tmp/HB.%d.%x.%d", fManager->GetPid(),
             (uint32_t) title, i );
    FILE * file = fopen( fileName, "w" );

    HBList   * esBufferList = NULL;
    HBBuffer * psBuffer     = NULL;
    HBBuffer * esBuffer     = NULL;

    for( ;; )
    {
        state = mpeg2_parse( handle );

        if( state == STATE_BUFFER )
        {
            /* Free the previous buffer */
            if( esBuffer )
            {
                delete esBuffer;
                esBuffer = NULL;
            }

            /* Get a new one */
            while( !esBuffer )
            {
                while( !esBufferList )
                {
                    psBuffer = new HBBuffer( DVD_VIDEO_LB_LEN );
                    if( dvdplay_read( vmg, psBuffer->fData, 1 ) != 1 ||
                            !PStoES( psBuffer, &esBufferList ) )
                    {
                        Log( "HBScanner::DecodeFrame: failed to get "
                             "a valid PS packet" );
                        mpeg2_close( handle );
                        fclose( file );
                        return false;
                    }
                }

                esBuffer = (HBBuffer*) esBufferList->ItemAt( 0 );
                esBufferList->RemoveItem( esBuffer );
                if( !esBufferList->CountItems() )
                {
                    delete esBufferList;
                    esBufferList = NULL;
                }

                if( esBuffer->fStreamId != 0xE0 )
                {
                    delete esBuffer;
                    esBuffer = NULL;
                }
            }

            /* Feed libmpeg2 */
            mpeg2_buffer( handle, esBuffer->fData,
                          esBuffer->fData + esBuffer->fSize );
        }
        else if( state == STATE_SEQUENCE )
        {
            /* Get size & framerate info */
            title->fInWidth     = info->sequence->width;
            title->fInHeight    = info->sequence->height;
            title->fAspect      = (uint64_t)info->sequence->display_width *
                info->sequence->pixel_width * VOUT_ASPECT_FACTOR /
                ( info->sequence->display_height *
                  info->sequence->pixel_height );
            title->fRate        = 27000000;
            title->fScale       = info->sequence->frame_period;
        }
        else if( ( state == STATE_SLICE || state == STATE_END ) &&
                 ( info->display_fbuf ) &&
                 ( info->display_picture->flags & PIC_MASK_CODING_TYPE )
                     == PIC_FLAG_CODING_TYPE_I )
        {
            /* Write the raw picture to a file */
            fwrite( info->display_fbuf->buf[0],
                    title->fInWidth * title->fInHeight, 1, file );
            fwrite( info->display_fbuf->buf[1],
                    title->fInWidth * title->fInHeight / 4, 1, file );
            fwrite( info->display_fbuf->buf[2],
                    title->fInWidth * title->fInHeight / 4, 1, file );
            break;
        }
        else if( state == STATE_INVALID )
        {
            /* Reset libmpeg2 */
            mpeg2_close( handle );
            handle = mpeg2_init();
        }
    }

    mpeg2_close( handle );

    fclose( file );

    return true;
}
