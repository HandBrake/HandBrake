/* $Id: DVDRead.c,v 1.9 2004/01/16 19:04:03 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* libdvdplay */
#include "dvdread/ifo_types.h"
#include "dvdplay/dvdplay.h"
#include "dvdplay/info.h"
#include "dvdplay/state.h"
#include "dvdplay/nav.h"

/* Local prototypes */
static void DVDReadThread( void * );
static int  DoPass( HBDVDRead * );
static int  Demux( HBDVDRead * );
static int  Push( HBDVDRead *, HBFifo * fifo, HBBuffer ** buffer );

struct HBDVDRead
{
    HBHandle     * handle;

    dvdplay_ptr    vmg;
    HBTitle      * title;
    int            beginPosition;
    int            endPosition;
    int            pass;
    HBBuffer     * psBuffer;
    HBList       * esBufferList;

    volatile int   die;
    HBThread     * thread;
};

HBDVDRead * HBDVDReadInit( HBHandle * handle, HBTitle * title )
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
    d->title         = title;
    d->beginPosition = 0;
    d->endPosition   = 0;
    d->pass          = 0;
    d->psBuffer      = NULL;
    d->esBufferList  = HBListInit();

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
    HBDVDRead * d     = (HBDVDRead*) _d;
    HBTitle   * title = d->title;

    uint8_t dummy[DVD_VIDEO_LB_LEN];
    int i;

    /* Open the device */
    d->vmg = dvdplay_open( title->device, NULL, NULL );
    if( !d->vmg )
    {
        HBLog( "HBDVDRead: dvdplay_open() failed" );
        HBErrorOccured( d->handle, HB_ERROR_DVD_OPEN );
        return;
    }

    /* Open the title */
    dvdplay_start( d->vmg, title->index );
    d->beginPosition = dvdplay_title_first( d->vmg );
    d->endPosition   = dvdplay_title_end( d->vmg );

    HBLog( "HBDVDRead: starting, blocks: %d to %d",
         d->beginPosition, d->endPosition );

    /* Lalala */
    dvdplay_read( d->vmg, dummy, 1 );

    /* Do the job */
    for( i = 0; i < ( title->twoPass ? 2 : 1 ); i++ )
    {
        dvdplay_seek( d->vmg, 0 );

        HBLog( "HBDVDRead: starting pass %d of %d", i + 1,
               title->twoPass ? 2 : 1 );

        d->pass = title->twoPass ? ( i + 1 ) : 0;

        if( !DoPass( d ) )
        {
            break;
        }
    }

    if( !d->die )
    {
        HBLog( "HBDVDRead: done" );
        HBDone( d->handle );
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
    HBTitle * title = d->title;

    HBAudio  * audio;
    HBBuffer * esBuffer;
    int i;

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
            if( title->start < 0 )
            {
                title->start = esBuffer->pts / 90;
                HBLog( "HBDVDRead: got first 0xE0 packet (%d)",
                       title->start );
            }

            HBListRemove( d->esBufferList, esBuffer );
            if( !Push( d, title->inFifo, &esBuffer ) )
            {
                return 0;
            }
            continue;
        }

        for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
        {
            audio = (HBAudio*) HBListItemAt( title->ripAudioList, i );

            if( esBuffer->streamId != audio->id )
            {
                continue;
            }

            if( audio->start < 0 )
            {
                audio->start = esBuffer->pts / 90;
                HBLog( "HBDVDRead: got first 0x%x packet (%d)",
                       audio->id, audio->start );

                audio->delay = audio->start - title->start;
            }

            HBListRemove( d->esBufferList, esBuffer );
            if( !Push( d, audio->inFifo, &esBuffer ) )
            {
                return 0;
            }
            break;
        }

        if( esBuffer )
        {
            HBListRemove( d->esBufferList, esBuffer );
            HBBufferClose( &esBuffer );
        }
    }

    return 1;
}

static int Push( HBDVDRead * d, HBFifo * fifo, HBBuffer ** buffer )
{
    while( !d->die )
    {
        if( HBFifoPush( fifo, buffer ) )
        {
            return 1;
        }
        HBSnooze( 5000 );
    }

    return 0;
}

