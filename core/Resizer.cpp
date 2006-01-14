/* $Id: Resizer.cpp,v 1.3 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "Resizer.h"
#include "Manager.h"
#include "Fifo.h"

#include <ffmpeg/avcodec.h>

HBResizer::HBResizer( HBManager * manager, HBTitle * title )
    : HBThread( "resizer" )
{
    fManager     = manager;
    fTitle       = title;
}

void HBResizer::DoWork()
{
    /* Init libavcodec */
    ImgReSampleContext * resampleContext =
        img_resample_full_init( fTitle->fOutWidth, fTitle->fOutHeight,
                                fTitle->fInWidth, fTitle->fInHeight,
                                fTitle->fTopCrop, fTitle->fBottomCrop,
                                fTitle->fLeftCrop, fTitle->fRightCrop );

    /* Buffers & pictures */
    HBBuffer * rawBuffer, * deinterlacedBuffer, * resizedBuffer;
    AVPicture rawPicture, deinterlacedPicture, resizedPicture;

    deinterlacedBuffer = new HBBuffer( 3 * fTitle->fInWidth *
                                       fTitle->fInHeight / 2 );
    avpicture_fill( &deinterlacedPicture, deinterlacedBuffer->fData,
                    PIX_FMT_YUV420P, fTitle->fInWidth,
                    fTitle->fInHeight );

    for( ;; )
    {
        while( fSuspend )
        {
            Snooze( 10000 );
        }

        if( !( rawBuffer = Pop( fTitle->fRawFifo ) ) )
        {
            break;
        }

        avpicture_fill( &rawPicture, rawBuffer->fData,
                        PIX_FMT_YUV420P, fTitle->fInWidth,
                        fTitle->fInHeight );

        resizedBuffer = new HBBuffer( 3 * fTitle->fOutWidth *
                                      fTitle->fOutHeight / 2 );
        resizedBuffer->fPosition = rawBuffer->fPosition;
        resizedBuffer->fPass     = rawBuffer->fPass;
        avpicture_fill( &resizedPicture, resizedBuffer->fData,
                        PIX_FMT_YUV420P, fTitle->fOutWidth,
                        fTitle->fOutHeight );


        if( fTitle->fDeinterlace )
        {
            avpicture_deinterlace( &deinterlacedPicture, &rawPicture,
                                   PIX_FMT_YUV420P,
                                   fTitle->fInWidth,
                                   fTitle->fInHeight );
            img_resample( resampleContext, &resizedPicture,
                          &deinterlacedPicture );
        }
        else
        {
            img_resample( resampleContext, &resizedPicture,
                          &rawPicture );
        }

        Push( fTitle->fResizedFifo, resizedBuffer );
        delete rawBuffer;
    }

    /* Free memory */
    delete deinterlacedBuffer;

    /* Close libavcodec */
    img_resample_close( resampleContext );
}

