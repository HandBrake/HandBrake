/* $Id: Mpeg2Decoder.cpp,v 1.14 2003/09/30 14:38:15 titer Exp $

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
    : HBThread( "mpeg2decoder" )
{
    fManager     = manager;
    fTitle       = title;
}

void HBMpeg2Decoder::DoWork()
{
    if( !( fMpeg2Buffer = Pop( fTitle->fMpeg2Fifo ) ) )
    {
        return;
    }

    fPass = fMpeg2Buffer->fPass;
    Init();

    do
    {
        while( fSuspend )
        {
            Snooze( 10000 );
        }

        if( fMpeg2Buffer->fPass != fPass )
        {
            Close();
            fPass = fMpeg2Buffer->fPass;
            Init();
        }

        DecodeBuffer();
    }
    while( ( fMpeg2Buffer = Pop( fTitle->fMpeg2Fifo ) ) );
}

void HBMpeg2Decoder::Init()
{
    fLateField = false;

    /* Init libmpeg2 */
    fHandle = mpeg2_init();
}

void HBMpeg2Decoder::Close()
{
    /* Close libmpeg2 */
    mpeg2_close( fHandle );
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
            HBBuffer * rawBuffer = new HBBuffer( 3 * fTitle->fInWidth *
                                                 fTitle->fInHeight / 2 );

            /* TODO : make libmpeg2 write directly in our buffer */
            memcpy( rawBuffer->fData,
                    info->display_fbuf->buf[0],
                    fTitle->fInWidth * fTitle->fInHeight );
            memcpy( rawBuffer->fData + fTitle->fInWidth *
                        fTitle->fInHeight,
                    info->display_fbuf->buf[1],
                    fTitle->fInWidth * fTitle->fInHeight / 4 );
            memcpy( rawBuffer->fData + fTitle->fInWidth *
                        fTitle->fInHeight + fTitle->fInWidth *
                        fTitle->fInHeight / 4,
                    info->display_fbuf->buf[2],
                    fTitle->fInWidth * fTitle->fInHeight / 4 );

            rawBuffer->fPosition = fMpeg2Buffer->fPosition;
            rawBuffer->fPass     = fPass;

            /* NTSC pulldown kludge */
            if( info->display_picture->nb_fields == 3 )
            {
                if( fLateField )
                {
                    HBBuffer * buffer =
                        new HBBuffer( rawBuffer->fSize );
                    buffer->fPosition = rawBuffer->fPosition;
                    buffer->fPass     = rawBuffer->fPass;
                    memcpy( buffer->fData, rawBuffer->fData,
                            buffer->fSize );
                    Push( fTitle->fRawFifo, buffer );
                }
                fLateField = !fLateField;
            }

            /* Send it to the encoder */
            if( !( Push( fTitle->fRawFifo, rawBuffer ) ) )
            {
                break;
            }
        }
        else if( state == STATE_INVALID )
        {
            /* Shouldn't happen on a DVD */
            Log( "HBMpeg2Decoder : STATE_INVALID" );
        }
    }

    delete fMpeg2Buffer;
}

