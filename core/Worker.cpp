/* $Id: Worker.cpp,v 1.9 2003/10/13 17:59:40 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "Ac3Decoder.h"
#include "Mp3Encoder.h"
#include "Mpeg2Decoder.h"
#include "Mpeg4Encoder.h"
#include "MpegDemux.h"
#include "Resizer.h"
#include "Worker.h"

HBWorker::HBWorker( HBTitle * title, HBAudio * audio1,
                    HBAudio * audio2 )
   : HBThread( "worker")
{
    fTitle  = title;
    fAudio1 = audio1;
    fAudio2 = audio2;

    Run();
}

void HBWorker::DoWork()
{
    bool     didSomething;
    uint64_t mpegDemux    = 0;
    uint64_t mpeg2Decoder = 0;
    uint64_t resizer      = 0;
    uint64_t mpeg4Encoder = 0;
    uint64_t ac3Decoder1  = 0;
    uint64_t mp3Encoder1  = 0;
    uint64_t ac3Decoder2  = 0;
    uint64_t mp3Encoder2  = 0;
    uint64_t tmpDate;
    
    while( !fDie )
    {
        didSomething = false;

        tmpDate = GetDate();
        if( fTitle->fMpegDemux->Work() )
        {
            mpegDemux += ( GetDate() - tmpDate );
            didSomething = true;
        }

        if( fDie ) break;

        tmpDate = GetDate();
        if( fTitle->fMpeg2Decoder->Work() )
        {
            mpeg2Decoder += ( GetDate() - tmpDate );
            didSomething = true;
        }

        if( fDie ) break;

        tmpDate = GetDate();
        if( fTitle->fResizer->Work() )
        {
            resizer += ( GetDate() - tmpDate );
            didSomething = true;
        }
        
        if( fDie ) break;

        tmpDate = GetDate();
        if( fTitle->fMpeg4Encoder->Work() )
        {
            mpeg4Encoder += ( GetDate() - tmpDate );
            didSomething = true;
        }
        
        if( fDie ) break;

        if( fAudio1 )
        {
            tmpDate = GetDate();
            if( fAudio1->fAc3Decoder->Work() )
            {
                ac3Decoder1 += ( GetDate() - tmpDate );
                didSomething = true;
            }
            
            if( fDie ) break;
            
            tmpDate = GetDate();
            if( fAudio1->fMp3Encoder->Work() )
            {
                mp3Encoder1 += ( GetDate() - tmpDate );
                didSomething = true;
            }

            if( fDie ) break;
        }

        if( fAudio2 )
        {
            tmpDate = GetDate();
            if( fAudio2->fAc3Decoder->Work() )
            {
                ac3Decoder2 += ( GetDate() - tmpDate );
                didSomething = true;
            }
            
            if( fDie ) break;
            
            tmpDate = GetDate();
            if( fAudio2->fMp3Encoder->Work() )
            {
                mp3Encoder2 += ( GetDate() - tmpDate );
                didSomething = true;
            }

            if( fDie ) break;
        }

        if( !didSomething )
        {
            Snooze( 10000 );
        }

        while( fSuspend )
        {
            Snooze( 10000 );
        }
    }

    tmpDate = mpegDemux + mpeg2Decoder + resizer + mpeg4Encoder +
        ac3Decoder1 + mp3Encoder1 + ac3Decoder2 + mp3Encoder2;
    Log( "HBWorker stopped. CPU utilization:" );
    Log( "- MPEG demuxer:   %.2f %%", 100 * (float) mpegDemux / tmpDate );
    Log( "- MPEG-2 decoder: %.2f %%", 100 * (float) mpeg2Decoder / tmpDate );
    Log( "- Resizer:        %.2f %%", 100 * (float) resizer / tmpDate );
    Log( "- MPEG-4 encoder: %.2f %%", 100 * (float) mpeg4Encoder / tmpDate );
    if( fAudio1 )
    {
    Log( "- AC3 decoder 1:  %.2f %%", 100 * (float) ac3Decoder1 / tmpDate );
    Log( "- MP3 encoder 1:  %.2f %%", 100 * (float) mp3Encoder1 / tmpDate );
    }
    if( fAudio2 )
    {
    Log( "- AC3 decoder 2:  %.2f %%", 100 * (float) ac3Decoder2 / tmpDate );
    Log( "- MP3 encoder 2:  %.2f %%", 100 * (float) mp3Encoder2 / tmpDate );
    }
}

