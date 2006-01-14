/* $Id: Ac3Decoder.cpp,v 1.21 2003/10/14 14:35:20 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "Ac3Decoder.h"
#include "Fifo.h"
#include "Manager.h"

extern "C" {
#include <a52dec/a52.h>
}

HBAc3Decoder::HBAc3Decoder( HBManager * manager, HBAudio * audio )
{
    fManager = manager;
    fAudio   = audio;

    fLock = new HBLock();
    fUsed = false;

    /* Init liba52 */
    fState    = a52_init( 0 );
    fInFlags  = 0;
    fOutFlags = A52_STEREO;

    /* Lame wants samples from -32768 to 32768 */
    fSampleLevel = 32768.0;
    
    /* Max size for a A52 frame is 3840 bytes */
    fAc3Frame        = new HBBuffer( 3840 );
    fAc3Frame->fSize = 0;
    fAc3Buffer       = NULL;
    fPosInAc3Buffer  = 0;
    fRawBuffer       = NULL;
}

HBAc3Decoder::~HBAc3Decoder()
{
    if( fRawBuffer ) delete fRawBuffer;
    if( fAc3Buffer ) delete fAc3Buffer;
    delete fAc3Frame;
    a52_free( fState );
    delete fLock;
}

bool HBAc3Decoder::Work()
{
    if( !Lock() )
    {
        return false;
    }

    /* Push the latest decoded buffer */
    if( fRawBuffer )
    {
        if( fAudio->fRawFifo->Push( fRawBuffer ) )
        {
            fRawBuffer = NULL;
        }
        else
        {
            Unlock();
            return false;
        }
    }

    /* Get a frame header (7 bytes) */
    if( fAc3Frame->fSize < 7 )
    {
        if( GetBytes( 7 ) )
        {
            /* Get the size of the coming frame */
            fFrameSize = a52_syncinfo( fAc3Frame->fData, &fInFlags,
                                       &fAudio->fInSampleRate,
                                       &fAudio->fInBitrate );
            if( !fFrameSize )
            {
                Log( "HBAc3Decoder: a52_syncinfo failed" );
                fManager->Error( HB_ERROR_A52_SYNC );
                return false;
            }
        }
        else
        {
            Unlock();
            return false;
        }
    }

    /* In case the audio should start later than the video,
       insert some silence */
    if( fAudio->fDelay > 3 * 256 * 1000 / fAudio->fInSampleRate  )
    {
        fRawBuffer = new HBBuffer( 12 * 256 * sizeof( float ) );
        for( uint32_t i = 0; i < 12 * 256; i++ )
        {
            ((float*)fRawBuffer->fData)[i] = 0;
        }
        fAudio->fDelay -= 6 * 256 * 1000 / fAudio->fInSampleRate;

        Unlock();
        return true;
    }

    if( fAc3Frame->fSize >= 7 )
    {
        /* Get the whole frame */
        if( GetBytes( (uint32_t) fFrameSize ) )
        {
            /* Feed liba52 */
            a52_frame( fState, fAc3Frame->fData, &fOutFlags,
                       &fSampleLevel, 0 );
            fAc3Frame->fSize = 0;

            /* 6 blocks per frame, 256 samples per block */
            fRawBuffer = new HBBuffer( 12 * 256 * sizeof( float ) );
            fRawBuffer->fPosition = fPosition;

            sample_t * samples;
            for( int i = 0; i < 6; i++ )
            {
                /* Decode a block */
                a52_block( fState );

                /* Get a pointer to the raw data */
                samples = a52_samples( fState );

                /* Copy left channel data */
                memcpy( (float*) fRawBuffer->fData + i * 256,
                        samples,
                        256 * sizeof( float ) );

                /* Copy right channel data */
                memcpy( (float*) fRawBuffer->fData + ( 6 + i ) * 256,
                        samples + 256,
                        256 * sizeof( float ) );
            }
        }
        else
        {
            Unlock();
            return false;
        }
    }

    Unlock();
    return true;
}

bool HBAc3Decoder::Lock()
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

void HBAc3Decoder::Unlock()
{
    fLock->Lock();
    fUsed = false;
    fLock->Unlock();
}

/* GetBytes() : pops buffers from the AC3 fifo until fAc3Frame
   contains <size> bytes */
bool HBAc3Decoder::GetBytes( uint32_t size )
{
    while( fAc3Frame->fSize < size )
    {
        if( !fAc3Buffer )
        {
            if( !( fAc3Buffer = fAudio->fAc3Fifo->Pop() ) )
            {
                return false;
            }
            fPosInAc3Buffer = 0;
            fPosition       = fAc3Buffer->fPosition;
        }

        int willCopy = MIN( size - fAc3Frame->fSize,
                            fAc3Buffer->fSize - fPosInAc3Buffer );
        memcpy( fAc3Frame->fData + fAc3Frame->fSize,
                fAc3Buffer->fData + fPosInAc3Buffer,
                willCopy );
        fAc3Frame->fSize += willCopy;
        fPosInAc3Buffer  += willCopy;

        if( fAc3Buffer->fSize == fPosInAc3Buffer )
        {
            delete fAc3Buffer;
            fAc3Buffer = NULL;
        }
    }

    return true;
}
