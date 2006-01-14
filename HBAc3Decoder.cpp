/* $Id: HBAc3Decoder.cpp,v 1.6 2003/08/24 15:03:41 titer Exp $ */

#include "HBCommon.h"
#include "HBAc3Decoder.h"
#include "HBManager.h"
#include "HBFifo.h"

extern "C" {
#include <a52dec/a52.h>
}

HBAc3Decoder::HBAc3Decoder( HBManager * manager, HBAudioInfo * audioInfo )
    : HBThread( "ac3decoder" )
{
    fManager        = manager;
    fAudioInfo      = audioInfo;
    
    fAc3Buffer      = NULL;
    fAc3Frame       = NULL;
    fPosInAc3Buffer = 0;
}

void HBAc3Decoder::DoWork()
{
    /* Init liba52 */
    a52_state_t * state       = a52_init( 0 );
    int           inFlags     = 0;
    int           outFlags    = A52_STEREO;
    float         sampleLevel = 32768;       /* lame wants samples from
                                                -32768 to 32768 */
    
    /* Max size for a A52 frame is 3840 bytes */
    fAc3Frame = new HBBuffer( 3840 ); 

    int           frameSize;
    HBBuffer    * rawBuffer;
    sample_t    * samples;
    
    /* Main loop */
    while( !fDie )
    {
        fAc3Frame->fSize = 0;
        
        /* Get a frame header (7 bytes) */
        if( !( GetBytes( 7 ) ) )
        {
            continue;
        }
        
        /* Get the size of the current frame */
        frameSize = a52_syncinfo( fAc3Frame->fData, &inFlags,
                                  &fAudioInfo->fInSampleRate,
                                  &fAudioInfo->fInBitrate );
        
        if( !frameSize )
        {
            Log( "HBAc3Decoder : a52_syncinfo failed" );
            fManager->Error();
            break;
        }
        
        /* Get the whole frame */
        if( !( GetBytes( (uint32_t) frameSize ) ) )
        {
            continue;
        }

        /* Feed liba52 */
        a52_frame( state, fAc3Frame->fData, &outFlags, &sampleLevel, 0 );
    
        /* 6 blocks per frame, 256 samples per block */
        rawBuffer = new HBBuffer( 12 * 256 * sizeof( float ) );
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
        
        fAudioInfo->fRawFifo->Push( rawBuffer );
    }
    
    /* Clean up */
    delete fAc3Frame;
}

/* GetBytes() : pops buffers from the AC3 fifo until fAc3Frame
   contains <size> bytes */
bool HBAc3Decoder::GetBytes( uint32_t size )
{
    while( fAc3Frame->fSize < size )
    {
        if( !fAc3Buffer )
        {
            if( !( fAc3Buffer = fAudioInfo->fAc3Fifo->Pop() ) )
            {
                return false;
            }
            fPosInAc3Buffer = 0;
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
