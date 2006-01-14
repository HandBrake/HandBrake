/* $Id: DVDRead.c,v 1.4 2003/11/06 13:03:19 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "DVDRead.h"
#include "Fifo.h"
#include "Thread.h"

#include <dvdread/ifo_types.h>
#include <dvdplay/dvdplay.h>
#include <dvdplay/info.h>
#include <dvdplay/state.h>
#include <dvdplay/nav.h>

/* Local prototypes */
static void DVDReadThread( void * );
static int  DoPass( HBDVDRead * );
static int  Demux( HBDVDRead * );
static int  Push( HBDVDRead *, HBFifo * fifo, HBBuffer ** buffer );

struct HBDVDRead
{
    HBHandle    * handle;
    
    dvdplay_ptr   vmg;
    HBTitle     * title;
    HBAudio     * audio;
    HBAudio     * optAudio;
    int           beginPosition;
    int           endPosition;
    int           pass;
    HBBuffer    * psBuffer;
    HBList      * esBufferList;
    HBBuffer    * videoBuf;
    HBBuffer    * audioBuf;
    HBBuffer    * optAudioBuf;
    int           videoStart;
    int           audioStart;
    int           optAudioStart;

    int           die;
    HBThread    * thread;
};

HBDVDRead * HBDVDReadInit( HBHandle * handle, HBTitle * t,
                           HBAudio * a1, HBAudio * a2 )
{
    HBDVDRead * d;
    if( !( d = malloc( sizeof( HBDVDRead ) ) ) )
    {
        HBLog( "HBDVDReadInit: malloc() failed, gonna crash" );
        return NULL;
    }

    /* Initializations */
    d->handle        = handle;
    d->vmg           = NULL;
    d->title         = t;
    d->audio        = a1;
    d->optAudio        = a2;
    d->beginPosition = 0;
    d->endPosition   = 0;
    d->pass          = 0;
    d->psBuffer      = NULL;
    d->esBufferList  = HBListInit();
    d->videoBuf      = NULL;
    d->audioBuf     = NULL;
    d->optAudioBuf     = NULL;
    d->videoStart    = -1;
    d->audioStart   = -1;
    d->optAudioStart   = -1;

    /* Launch the thread */
    d->die    = 0;
    d->thread = HBThreadInit( "dvd reader", DVDReadThread, d,
                              HB_NORMAL_PRIORITY );

    return d;
}

void HBDVDReadClose( HBDVDRead ** _d )
{
    HBBuffer * buffer;
    
    HBDVDRead * d = *_d;
    
    /* Stop the thread */
    d->die = 1;
    HBThreadClose( &d->thread );
    
    /* Clean up */
    while( ( buffer = (HBBuffer*) HBListItemAt( d->esBufferList, 0 ) ) )
    {
        HBListRemove( d->esBufferList, buffer );
        HBBufferClose( &buffer );
    }
    HBListClose( &d->esBufferList ) ;
    free( d );
    
    (*_d) = NULL;
}

static void DVDReadThread( void * _d )
{
    HBDVDRead * d = (HBDVDRead*) _d;
    uint8_t dummy[DVD_VIDEO_LB_LEN];
    int i;

    /* Open the device */
    d->vmg = dvdplay_open( d->title->device, NULL, NULL );
    if( !d->vmg )
    {
        HBLog( "HBDVDRead: dvdplay_open() failed" );
        HBErrorOccured( d->handle, HB_ERROR_DVD_OPEN );
        return;
    }   
    
    /* Open the title */
    dvdplay_start( d->vmg, d->title->index );
    d->beginPosition = dvdplay_title_first( d->vmg );
    d->endPosition   = dvdplay_title_end( d->vmg );
    
    HBLog( "HBDVDRead: starting, blocks: %d to %d",
         d->beginPosition, d->endPosition );
         
    /* Lalala */
    dvdplay_read( d->vmg, dummy, 1 );
    
    /* Do the job */
    for( i = 0; i < ( d->title->twoPass ? 2 : 1 ); i++ )
    {
        dvdplay_seek( d->vmg, 0 );
        
        HBLog( "HBDVDRead: starting pass %d of %d", i + 1,
             d->title->twoPass ? 2 : 1 );
        
        d->pass = d->title->twoPass ? ( i + 1 ) : 0;

        if( !DoPass( d ) )
        {
            break;
        }   
    }   

    /* Flag the latest buffers so we know when we're done */
    if( !d->die )
    {
        HBLog( "HBDVDRead: done" );
       
        if( d->videoBuf )
        {
            d->videoBuf->last = 1;
            Push( d, d->title->mpeg2Fifo, &d->videoBuf );
        }
        if( d->audioBuf )
        {
            d->audioBuf->last = 1;
            Push( d, d->audio->ac3Fifo, &d->audioBuf );
        }
        if( d->optAudioBuf )
        {
            d->optAudioBuf->last = 1;
            Push( d, d->optAudio->ac3Fifo, &d->optAudioBuf );
        }
    }

    /* Clean up */
    dvdplay_close( d->vmg );
}


static int DoPass( HBDVDRead * d )
{
    int i;

    for( i = 0; i < d->endPosition - d->beginPosition; i++ )
    {
        d->psBuffer = HBBufferInit( DVD_VIDEO_LB_LEN );
        d->psBuffer->position =
            (float) i / ( d->endPosition - d->beginPosition );
        
        if( d->pass )
        {
            d->psBuffer->position /= 2;
            
            if( d->pass == 2 )
            {
                d->psBuffer->position += 0.5;
            }
        }
        d->psBuffer->pass = d->pass;
        
        if( dvdplay_read( d->vmg, d->psBuffer->data, 1 ) < 0 )
        {   
            HBLog( "HBDVDRead: dvdplay_read() failed" );
            HBErrorOccured( d->handle, HB_ERROR_DVD_READ );
            HBBufferClose( &d->psBuffer );
            return 0;
        }

        if( !Demux( d ) )
        {
            return 0;
        }
    }

    return 1;
}

static int Demux( HBDVDRead * d )
{
    HBBuffer * esBuffer;

    /* Demux */
    HBPStoES( &d->psBuffer, d->esBufferList );

    /* Push buffers */
    while( ( esBuffer = (HBBuffer*) HBListItemAt( d->esBufferList, 0 ) ) )
    {
        /* First pass: trash audio buffers */
        if( d->pass == 1 && esBuffer->streamId != 0xE0 )
        {
            HBListRemove( d->esBufferList, esBuffer );
            HBBufferClose( &esBuffer );
            continue;
        }

        if( esBuffer->streamId == 0xE0 )
        {
            if( d->videoStart < 0 )
            {
                d->videoStart = esBuffer->pts / 90;
                HBLog( "HBDVDRead: got first 0xE0 packet (%d)",
                       d->videoStart );
            }

            if( d->videoBuf )
            {
                d->videoBuf->last = 0;
                if( !Push( d, d->title->mpeg2Fifo, &d->videoBuf ) )
                {
                    return 0;
                }
            }

            HBListRemove( d->esBufferList, esBuffer );
            d->videoBuf = esBuffer;
        }
        else if( esBuffer->streamId == d->audio->id )
        {
            if( d->audioStart < 0 )
            {
                d->audioStart = esBuffer->pts / 90;
                HBLog( "HBDVDRead: got first 0x%x packet (%d)",
                       d->audio->id, d->audioStart );

                d->audio->delay = d->audioStart - d->videoStart;
            }

            if( d->audioBuf )
            {
                d->audioBuf->last = 0;
                if( !Push( d, d->audio->ac3Fifo, &d->audioBuf ) )
                {
                    return 0;
                }
            }

            HBListRemove( d->esBufferList, esBuffer );
            d->audioBuf = esBuffer;
        }
        else if( d->optAudio && esBuffer->streamId == d->optAudio->id )
        {
            if( d->optAudioStart < 0 )
            {
                d->optAudioStart = esBuffer->pts / 90;
                HBLog( "HBDVDRead: got first 0x%x packet (%d)",
                       d->optAudio->id, d->optAudioStart );

                d->optAudio->delay = d->optAudioStart - d->videoStart;
            }

            if( d->optAudioBuf )
            {
                d->optAudioBuf->last = 0;
                if( !Push( d, d->optAudio->ac3Fifo, &d->optAudioBuf ) )
                {
                    return 0;
                }
            }

            HBListRemove( d->esBufferList, esBuffer );
            d->optAudioBuf = esBuffer;
        }
        else
        {
            HBListRemove( d->esBufferList, esBuffer );
            HBBufferClose( &esBuffer );
        }
    }

    return 1;
}
 
static int Push( HBDVDRead * d, HBFifo * fifo, HBBuffer ** buffer )
{
    for( ;; )
    {
        HBCheckPaused( d->handle );

        if( HBFifoPush( fifo, buffer ) )
        {
            return 1;
        }

        if( d->die )
        {
            break;
        }

        HBSnooze( 10000 );
    }

    return 0;
}

