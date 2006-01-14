/* $Id: Resizer.cpp,v 1.8 2003/10/09 13:24:48 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "Resizer.h"
#include "Manager.h"
#include "Fifo.h"

#include <ffmpeg/avcodec.h>

HBResizer::HBResizer( HBManager * manager, HBTitle * title )
{
    fManager = manager;
    fTitle   = title;

    /* Lock */
    fLock = new HBLock();
    fUsed = false;

    /* Init libavcodec */
    fResampleContext =
        img_resample_full_init( fTitle->fOutWidth, fTitle->fOutHeight,
                                fTitle->fInWidth, fTitle->fInHeight,
                                fTitle->fTopCrop, fTitle->fBottomCrop,
                                fTitle->fLeftCrop, fTitle->fRightCrop );

    /* Buffers & pictures */
    fRawBuffer           = NULL;
    fDeinterlacedBuffer  = new HBBuffer( 3 * fTitle->fInWidth *
                                        fTitle->fInHeight / 2 );
    fResizedBuffer       = NULL;
    fRawPicture          = (AVPicture*) malloc( sizeof( AVPicture ) );
    fDeinterlacedPicture = (AVPicture*) malloc( sizeof( AVPicture ) );
    fResizedPicture      = (AVPicture*) malloc( sizeof( AVPicture ) );

    avpicture_fill( fDeinterlacedPicture, fDeinterlacedBuffer->fData,
                    PIX_FMT_YUV420P, fTitle->fInWidth,
                    fTitle->fInHeight );
}

HBResizer::~HBResizer()
{
    /* Free memory */
    free( fResizedPicture );
    free( fDeinterlacedPicture );
    free( fRawPicture );
    if( fResizedBuffer ) delete fResizedBuffer;
    delete fDeinterlacedBuffer;
    img_resample_close( fResampleContext );
    delete fLock;
}

bool HBResizer::Work()
{
    if( !Lock() )
    {
        return false;
    }

    bool didSomething = false;
    
    for( ;; )
    {
        /* Push the latest resized buffer */
        if( fResizedBuffer )
        {
            if( fTitle->fResizedFifo->Push( fResizedBuffer ) )
            {
                fResizedBuffer = NULL;
            }
            else
            {
                break;
            }
        }
        
        /* Get a new raw picture */
        if( !( fRawBuffer = fTitle->fRawFifo->Pop() ) )
        {
            break;
        }

        /* Do the job */
        avpicture_fill( fRawPicture, fRawBuffer->fData,
                        PIX_FMT_YUV420P, fTitle->fInWidth,
                        fTitle->fInHeight );

        fResizedBuffer = new HBBuffer( 3 * fTitle->fOutWidth *
                                      fTitle->fOutHeight / 2 );
        fResizedBuffer->fPosition = fRawBuffer->fPosition;
        fResizedBuffer->fPass     = fRawBuffer->fPass;
        avpicture_fill( fResizedPicture, fResizedBuffer->fData,
                        PIX_FMT_YUV420P, fTitle->fOutWidth,
                        fTitle->fOutHeight );

        if( fTitle->fDeinterlace )
        {
            avpicture_deinterlace( fDeinterlacedPicture, fRawPicture,
                                   PIX_FMT_YUV420P,
                                   fTitle->fInWidth,
                                   fTitle->fInHeight );
            img_resample( fResampleContext, fResizedPicture,
                          fDeinterlacedPicture );
        }
        else
        {
            img_resample( fResampleContext, fResizedPicture,
                          fRawPicture );
        }
        delete fRawBuffer;
        fRawBuffer = NULL;

        didSomething = true;
    }

    Unlock();
    return didSomething;
}

bool HBResizer::Lock()
{
    fLock->Lock();
    if( fUsed )
    {
        fLock->Unlock();
        return false;
    }
    fUsed = true;
    fLock->Unlock();
    return true;
}

void HBResizer::Unlock()
{
    fLock->Lock();
    fUsed = false;
    fLock->Unlock();
}

