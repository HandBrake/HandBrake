/* $Id: Mp3Encoder.cpp,v 1.13 2003/10/08 15:00:20 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "Mp3Encoder.h"
#include "Manager.h"
#include "Fifo.h"

#include <lame/lame.h>

HBMp3Encoder::HBMp3Encoder( HBManager * manager, HBAudio * audio )
{
    fManager = manager;
    fAudio   = audio;

    fLock = new HBLock();
    fUsed = false;

    fRawBuffer    = NULL;
    fPosInBuffer  = 0;
    fSamplesNb    = 0;
    fLeftSamples  = NULL;
    fRightSamples = NULL;

    fPosition     = 0;
    fInitDone = false;
    fMp3Buffer = NULL;
}

bool HBMp3Encoder::Work()
{
    if( !Lock() )
    {
        return false;
    }
    
    if( !fInitDone )
    {
        /* Wait for a first buffer so we know fAudio->fInSampleRate
           is correct */
        if( !fAudio->fRawFifo->Size() )
        {
            Unlock();
            return false;
        }
        
        /* The idea is to have exactly one mp3 frame (i.e. 1152 samples) by
           output buffer. As we are resampling from fInSampleRate to
           fOutSampleRate, we will give ( 1152 * fInSampleRate ) /
           ( 2 * fOutSampleRate ) to libmp3lame so we are sure we will
           never get more than 1 frame at a time */
        fCount = ( 1152 * fAudio->fInSampleRate ) /
                    ( 2 * fAudio->fOutSampleRate );

        /* Init libmp3lame */
        fGlobalFlags = lame_init();
        lame_set_in_samplerate( fGlobalFlags, fAudio->fInSampleRate );
        lame_set_out_samplerate( fGlobalFlags, fAudio->fOutSampleRate );
        lame_set_brate( fGlobalFlags, fAudio->fOutBitrate );

        if( lame_init_params( fGlobalFlags ) == -1 )
        {
            Log( "HBMp3Encoder: lame_init_params() failed" );
            fManager->Error( HB_ERROR_MP3_INIT );
            return false;
        }

        fLeftSamples  = (float*) malloc( fCount * sizeof( float ) );
        fRightSamples = (float*) malloc( fCount * sizeof( float ) );

        fInitDone = true;
    }

    bool didSomething = false;

    for( ;; )
    {
        if( fMp3Buffer )
        {
            if( fAudio->fMp3Fifo->Push( fMp3Buffer ) )
            {
                fMp3Buffer = NULL;
            }
            else
            {
                break;
            }
        }
        
        /* Get new samples */
        if( !GetSamples() )
        {
            break;
        }

        int ret;
        fMp3Buffer = new HBBuffer( LAME_MAXMP3BUFFER );
        ret = lame_encode_buffer_float( fGlobalFlags, fLeftSamples,
                                        fRightSamples, fCount,
                                        fMp3Buffer->fData,
                                        fMp3Buffer->fSize );

        if( ret < 0 )
        {
            /* Something wrong happened */
            Log( "HBMp3Encoder : lame_encode_buffer_float() failed "
                 "(%d)", ret );
            fManager->Error( HB_ERROR_MP3_ENCODE );
            return false;
        }
        else if( ret > 0 )
        {
            /* We got something, send it to the muxer */
            fMp3Buffer->fSize     = ret;
            fMp3Buffer->fKeyFrame = true;
            fMp3Buffer->fPosition = fPosition;
        }
        else
        {
            delete fMp3Buffer;
            fMp3Buffer = NULL;
        }
        fSamplesNb = 0;

        didSomething = true;
    }

    Unlock();
    return didSomething;
}

bool HBMp3Encoder::Lock()
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

void HBMp3Encoder::Unlock()
{
    fLock->Lock();
    fUsed = false;
    fLock->Unlock();
}

bool HBMp3Encoder::GetSamples()
{
    while( fSamplesNb < fCount )
    {
        if( !fRawBuffer )
        {
            if( !( fRawBuffer = fAudio->fRawFifo->Pop() ) )
            {
                return false;
            }

            fPosInBuffer = 0;
            fPosition    = fRawBuffer->fPosition;
        }

        int willCopy = MIN( fCount - fSamplesNb, 6 * 256 - fPosInBuffer );

        memcpy( fLeftSamples + fSamplesNb,
                (float*) fRawBuffer->fData + fPosInBuffer,
                willCopy * sizeof( float ) );
        memcpy( fRightSamples + fSamplesNb,
                (float*) fRawBuffer->fData + 6 * 256 + fPosInBuffer,
                willCopy * sizeof( float ) );

        fSamplesNb    += willCopy;
        fPosInBuffer += willCopy;

        if( fPosInBuffer == 6 * 256 )
        {
            delete fRawBuffer;
            fRawBuffer = NULL;
        }
    }

    return true;
}
