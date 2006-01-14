/* $Id: Mpeg2Decoder.cpp,v 1.21 2003/10/09 14:21:21 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "Mpeg2Decoder.h"
#include "Manager.h"
#include "Fifo.h"

extern "C" {
#include <mpeg2dec/mpeg2.h>
}
#include <ffmpeg/avcodec.h>

HBMpeg2Decoder::HBMpeg2Decoder( HBManager * manager, HBTitle * title )
{
    fManager = manager;
    fTitle   = title;

    fLock = new HBLock();
    fUsed = false;
    
    fPass = 42;
    fRawBuffer = NULL;
    fRawBufferList = new HBList();
    fHandle = NULL;
}

bool HBMpeg2Decoder::Work()
{
    fLock->Lock();
    if( fUsed )
    {
        fLock->Unlock();
        return true;
    }
    fUsed = true;
    fLock->Unlock();

    bool didSomething = false;
    
    for( ;; )
    {
        /* Push decoded buffers */
        while( ( fRawBuffer =
                    (HBBuffer*) fRawBufferList->ItemAt( 0 ) ) )
        {
            if( fTitle->fRawFifo->Push( fRawBuffer ) )
            {
                fRawBufferList->RemoveItem( fRawBuffer );
            }
            else
            {
                break;
            }
        }

        if( fRawBufferList->CountItems() )
        {
            break;
        }
       
        /* Get a new buffer to decode */
        if( !( fMpeg2Buffer = fTitle->fMpeg2Fifo->Pop() ) )
        {
            break;
        }

        /* (Re)init if needed */
        if( fMpeg2Buffer->fPass != fPass )
        {
            fPass = fMpeg2Buffer->fPass;
            Init();
        }

        /* Do the job */
        DecodeBuffer();

        didSomething = true;
    }

    fLock->Lock();
    fUsed = false;
    fLock->Unlock();

    return didSomething;
}

void HBMpeg2Decoder::Init()
{
    if( fHandle )
    {
        mpeg2_close( fHandle );
    }

    fLateField = false;

    fHandle = mpeg2_init();
}

void HBMpeg2Decoder::DecodeBuffer()
{
    const mpeg2_info_t * info = mpeg2_info( fHandle );

    /* Feed libmpeg2 */
    mpeg2_buffer( fHandle, fMpeg2Buffer->fData,
                  fMpeg2Buffer->fData + fMpeg2Buffer->fSize );

    mpeg2_state_t state;
    for( ;; )
    {
        state = mpeg2_parse( fHandle );

        if( state == STATE_BUFFER )
        {
            break;
        }
        else if( ( state == STATE_SLICE || state == STATE_END ) &&
                   info->display_fbuf )
        {
            fRawBuffer = new HBBuffer( 3 * fTitle->fInWidth *
                                       fTitle->fInHeight / 2 );

            /* TODO : make libmpeg2 write directly in our buffer */
            memcpy( fRawBuffer->fData,
                    info->display_fbuf->buf[0],
                    fTitle->fInWidth * fTitle->fInHeight );
            memcpy( fRawBuffer->fData + fTitle->fInWidth *
                        fTitle->fInHeight,
                    info->display_fbuf->buf[1],
                    fTitle->fInWidth * fTitle->fInHeight / 4 );
            memcpy( fRawBuffer->fData + fTitle->fInWidth *
                        fTitle->fInHeight + fTitle->fInWidth *
                        fTitle->fInHeight / 4,
                    info->display_fbuf->buf[2],
                    fTitle->fInWidth * fTitle->fInHeight / 4 );

            fRawBuffer->fPosition = fMpeg2Buffer->fPosition;
            fRawBuffer->fPass     = fMpeg2Buffer->fPass;

            fRawBufferList->AddItem( fRawBuffer );

            /* NTSC pulldown kludge */
            if( info->display_picture->nb_fields == 3 )
            {
                if( fLateField )
                {
                    HBBuffer * pulldownBuffer;
                    pulldownBuffer = new HBBuffer( fRawBuffer->fSize );
                    pulldownBuffer->fPosition = fRawBuffer->fPosition;
                    pulldownBuffer->fPass     = fRawBuffer->fPass;
                    memcpy( pulldownBuffer->fData, fRawBuffer->fData,
                            pulldownBuffer->fSize );
                    fRawBufferList->AddItem( pulldownBuffer );
                }
                fLateField = !fLateField;
            }
        }
        else if( state == STATE_INVALID )
        {
            /* Shouldn't happen on a DVD */
            Log( "HBMpeg2Decoder: STATE_INVALID" );
        }
    }

    delete fMpeg2Buffer;
    fMpeg2Buffer = NULL;
}

