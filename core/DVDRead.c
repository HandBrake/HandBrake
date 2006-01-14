/* $Id: DVDRead.c,v 1.13 2004/05/02 16:25:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

#include "dvdread/dvd_reader.h"

/* Local prototypes */
static void DVDReadThread( void * );
static int  DoPass( HBDVDRead * );
static int  Demux( HBDVDRead * );
static int  Push( HBDVDRead *, HBFifo * fifo, HBBuffer ** buffer );

struct HBDVDRead
{
    HBHandle     * handle;

    dvd_reader_t * reader;
    dvd_file_t   * file;
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
    d->reader        = NULL;
    d->file          = NULL;
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

    int i;

    /* Open the device */
    d->reader = DVDOpen( title->device );
    if( !d->reader )
    {
        HBLog( "HBDVDRead: DVDOpen() failed" );
        HBErrorOccured( d->handle, HB_ERROR_DVD_OPEN );
        return;
    }

    /* Open the title */
    d->file = DVDOpenFile( d->reader, title->vts_id, DVD_READ_TITLE_VOBS );

    /* Do the job */
    for( i = 0; i < ( title->twoPass ? 2 : 1 ); i++ )
    {
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
    DVDCloseFile( d->file );
    DVDClose( d->reader );
}


static int DoPass( HBDVDRead * d )
{
    int i;

    for( i = d->title->startBlock; i < d->title->endBlock; i++ )
    {
        d->psBuffer = HBBufferInit( DVD_VIDEO_LB_LEN );
        d->psBuffer->position = (float) ( i - d->title->startBlock ) /
            ( d->title->endBlock - d->title->startBlock );

        if( d->pass )
        {
            d->psBuffer->position /= 2;

            if( d->pass == 2 )
            {
                d->psBuffer->position += 0.5;
            }
        }
        d->psBuffer->pass = d->pass;

        if( DVDReadBlocks( d->file, i, 1, d->psBuffer->data ) < 0 )
        {
            HBLog( "HBDVDRead: DVDReadBlocks() failed" );
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

        /* Video */
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

        /* Audio or whatever */
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

