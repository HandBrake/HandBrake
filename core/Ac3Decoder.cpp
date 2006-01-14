/* $Id: Ac3Decoder.cpp,v 1.12 2003/09/30 14:38:15 titer Exp $

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
    : HBThread( "ac3decoder" )
{
    fManager     = manager;
    fAudio       = audio;

    /* Max size for a A52 frame is 3840 bytes */
    fAc3Frame    = new HBBuffer( 3840 );
    fAc3Buffer   = NULL;
    fPosInBuffer = 0;
    fPosition    = 0;
}

HBAc3Decoder::~HBAc3Decoder()
{
    delete fAc3Frame;
}

void HBAc3Decoder::DoWork()
{
    /* Init liba52 */
    a52_state_t * state       = a52_init( 0 );
    int           inFlags     = 0;
    int           outFlags    = A52_STEREO;

    /* Lame wants samples from -32768 to 32768 */
    float         sampleLevel = 32768;

    int           frameSize;
    HBBuffer    * rawBuffer;
    sample_t    * samples;

    /* Main loop */
    for( ;; )
    {
        while( fSuspend )
        {
            Snooze( 10000 );
        }

        fAc3Frame->fSize = 0;

        /* Get a frame header (7 bytes) */
        if( !( GetBytes( 7 ) ) )
        {
            break;
        }

        /* Get the size of the current frame */
        frameSize = a52_syncinfo( fAc3Frame->fData, &inFlags,
                                  &fAudio->fInSampleRate,
                                  &fAudio->fInBitrate );

        if( !frameSize )
        {
            Log( "HBAc3Decoder : a52_syncinfo failed" );
            fManager->Error();
            break;
        }

        /* Get the whole frame */
        if( !( GetBytes( (uint32_t) frameSize ) ) )
        {
            break;
        }

        /* Feed liba52 */
        a52_frame( state, fAc3Frame->fData, &outFlags, &sampleLevel, 0 );

        /* 6 blocks per frame, 256 samples per block */
        rawBuffer = new HBBuffer( 12 * 256 * sizeof( float ) );
        rawBuffer->fPosition = fPosition;
        for( int i = 0; i < 6; i++ )
        {
            /* Decode a block */
            a52_block( state );

            /* Get a pointer to the raw data */
            samples = a52_samples( state );

            /* Copy left channel data */
            memcpy( (float*) rawBuffer->fData + i * 256,
                    samples,
                    256 * sizeof( float ) );

            /* Copy right channel data */
            memcpy( (float*) rawBuffer->fData + ( 6 + i ) * 256,
                    samples + 256,
                    256 * sizeof( float ) );
        }

        if( !Push( fAudio->fRawFifo, rawBuffer ) )
        {
            break;
        }
    }
}

/* GetBytes() : pops buffers from the AC3 fifo until fAc3Frame
   contains <size> bytes */
bool HBAc3Decoder::GetBytes( uint32_t size )
{
    while( fAc3Frame->fSize < size )
    {
        if( !fAc3Buffer )
        {
            if( !( fAc3Buffer = Pop( fAudio->fAc3Fifo ) ) )
            {
                return false;
            }
            fPosInBuffer = 0;
            fPosition    = fAc3Buffer->fPosition;
        }

        int willCopy = MIN( size - fAc3Frame->fSize,
                            fAc3Buffer->fSize - fPosInBuffer );
        memcpy( fAc3Frame->fData + fAc3Frame->fSize,
                fAc3Buffer->fData + fPosInBuffer,
                willCopy );
        fAc3Frame->fSize += willCopy;
        fPosInBuffer     += willCopy;

        if( fAc3Buffer->fSize == fPosInBuffer )
        {
            delete fAc3Buffer;
            fAc3Buffer = NULL;
        }
    }

    return true;
}
