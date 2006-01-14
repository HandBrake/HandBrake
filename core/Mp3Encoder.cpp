/* $Id: Mp3Encoder.cpp,v 1.7 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "Mp3Encoder.h"
#include "Manager.h"
#include "Fifo.h"

#include <lame/lame.h>

HBMp3Encoder::HBMp3Encoder( HBManager * manager, HBAudio * audio )
    : HBThread( "mp3encoder" )
{
    fManager      = manager;
    fAudio        = audio;

    fRawBuffer    = NULL;
    fPosInBuffer  = 0;
    fLeftSamples  = NULL;
    fRightSamples = NULL;

    fPosition     = 0;
}

void HBMp3Encoder::DoWork()
{
    /* Wait a first buffer so we are sure that
       fAudio->fInSampleRate (set the AC3 decoder) is not garbage */
    while( !fDie && !fAudio->fRawFifo->Size() )
    {
        Snooze( 5000 );
    }

    if( fDie )
    {
        return;
    }

    /* The idea is to have exactly one mp3 frame (i.e. 1152 samples) by
       output buffer. As we are resampling from fInSampleRate to
       fOutSampleRate, we will give ( 1152 * fInSampleRate ) /
       ( 2 * fOutSampleRate ) to libmp3lame so we are sure we will
       never get more than 1 frame at a time */
    uint32_t count = ( 1152 * fAudio->fInSampleRate ) /
                         ( 2 * fAudio->fOutSampleRate );

    /* Init libmp3lame */
    lame_global_flags * globalFlags = lame_init();
    lame_set_in_samplerate( globalFlags, fAudio->fInSampleRate );
    lame_set_out_samplerate( globalFlags, fAudio->fOutSampleRate );
    lame_set_brate( globalFlags, fAudio->fOutBitrate );

    if( lame_init_params( globalFlags ) == -1 )
    {
        Log( "HBMp3Encoder::DoWork() : lame_init_params() failed" );
        fManager->Error();
        return;
    }

    fLeftSamples  = (float*) malloc( count * sizeof( float ) );
    fRightSamples = (float*) malloc( count * sizeof( float ) );

    HBBuffer * mp3Buffer = new HBBuffer( LAME_MAXMP3BUFFER );

    int ret;
    for( ;; )
    {
        while( fSuspend )
        {
            Snooze( 10000 );
        }

        /* Get new samples */
        if( !GetSamples( count ) )
        {
            break;
        }

        ret = lame_encode_buffer_float( globalFlags,
                                        fLeftSamples, fRightSamples,
                                        count, mp3Buffer->fData,
                                        mp3Buffer->fSize );

        if( ret < 0 )
        {
            /* Something wrong happened */
            Log( "HBMp3Encoder : lame_encode_buffer_float() failed (%d)", ret );
            fManager->Error();
            break;
        }
        else if( ret > 0 )
        {
            /* We got something, send it to the muxer */
            mp3Buffer->fSize     = ret;
            mp3Buffer->fKeyFrame = true;
            mp3Buffer->fPosition = fPosition;
            Push( fAudio->fMp3Fifo, mp3Buffer );
            mp3Buffer = new HBBuffer( LAME_MAXMP3BUFFER );
        }
    }

    /* Clean up */
    delete mp3Buffer;
    free( fLeftSamples );
    free( fRightSamples );

    lame_close( globalFlags );
}

bool HBMp3Encoder::GetSamples( uint32_t count )
{
    uint32_t samplesNb = 0;

    while( samplesNb < count )
    {
        if( !fRawBuffer )
        {
            if( !( fRawBuffer = Pop( fAudio->fRawFifo ) ) )
            {
                return false;
            }

            fPosInBuffer = 0;
            fPosition    = fRawBuffer->fPosition;
        }

        int willCopy = MIN( count - samplesNb, 6 * 256 - fPosInBuffer );

        memcpy( fLeftSamples + samplesNb,
                (float*) fRawBuffer->fData + fPosInBuffer,
                willCopy * sizeof( float ) );
        memcpy( fRightSamples + samplesNb,
                (float*) fRawBuffer->fData + 6 * 256 + fPosInBuffer,
                willCopy * sizeof( float ) );

        samplesNb    += willCopy;
        fPosInBuffer += willCopy;

        if( fPosInBuffer == 6 * 256 )
        {
            delete fRawBuffer;
            fRawBuffer = NULL;
        }
    }

    return true;
}
