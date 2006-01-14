/* $Id: Mpeg4Encoder.cpp,v 1.15 2003/09/30 14:38:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "Mpeg4Encoder.h"
#include "Manager.h"
#include "Fifo.h"

#include <ffmpeg/avcodec.h>

HBMpeg4Encoder::HBMpeg4Encoder( HBManager * manager, HBTitle * title )
    : HBThread( "mpeg4encoder" )
{
    fManager = manager;
    fTitle   = title;
}

void HBMpeg4Encoder::DoWork()
{
    if( !( fResizedBuffer = Pop( fTitle->fResizedFifo ) ) )
    {
        return;
    }

    fPass = fResizedBuffer->fPass;
    Init();

    do
    {
        while( fSuspend )
        {
            Snooze( 10000 );
        }

        if( fResizedBuffer->fPass != fPass )
        {
            Close();
            fPass = fResizedBuffer->fPass;
            Init();
        }

        fManager->SetPosition( fResizedBuffer->fPosition );
        EncodeBuffer();
    }
    while( ( fResizedBuffer = Pop( fTitle->fResizedFifo ) ) );
}

void HBMpeg4Encoder::Init()
{
    AVCodec * codec = avcodec_find_encoder( CODEC_ID_MPEG4 );
    if( !codec )
    {
        Log( "HBMpeg4Encoder: avcodec_find_encoder() failed" );
        fManager->Error();
        return;
    }

    fContext                      = avcodec_alloc_context();
    fContext->bit_rate            = 1024 * fTitle->fBitrate;
    fContext->bit_rate_tolerance  = 1024 * fTitle->fBitrate;
    fContext->width               = fTitle->fOutWidth;
    fContext->height              = fTitle->fOutHeight;
    fContext->frame_rate          = fTitle->fRate;
    fContext->frame_rate_base     = fTitle->fScale;
    fContext->gop_size            = 10 * fTitle->fRate / fTitle->fScale;

    if( fPass == 1 )
    {
        fContext->flags |= CODEC_FLAG_PASS1;

        char fileName[1024]; memset( fileName, 0, 1024 );
        sprintf( fileName, "/tmp/HB.%d.ffmpeg.log", fManager->GetPid() );
        fFile = fopen( fileName, "w" );
    }
    else if( fPass == 2 )
    {
        fContext->flags |= CODEC_FLAG_PASS2;

        char fileName[1024]; memset( fileName, 0, 1024 );
        sprintf( fileName, "/tmp/HB.%d.ffmpeg.log", fManager->GetPid() );
        fFile = fopen( fileName, "r" );
        fseek( fFile, 0, SEEK_END );
        uint32_t size = ftell( fFile );
        fLog = (char*) malloc( size + 1 );
        fseek( fFile, 0, SEEK_SET );
        fread( fLog, size, 1, fFile );
        fclose( fFile );
        fLog[size] = '\0';
        fContext->stats_in = fLog;
   }

    if( avcodec_open( fContext, codec ) < 0 )
    {
        Log( "HBMpeg4Encoder: avcodec_open() failed" );
        fManager->Error();
        return;
    }

    fFrame = avcodec_alloc_frame();
}

void HBMpeg4Encoder::Close()
{
    if( fPass == 1 )
    {
        fclose( fFile );
    }
}

void HBMpeg4Encoder::EncodeBuffer()
{
    fFrame->data[0] = fResizedBuffer->fData;
    fFrame->data[1] = fFrame->data[0] + fTitle->fOutWidth *
                      fTitle->fOutHeight;
    fFrame->data[2] = fFrame->data[1] + fTitle->fOutWidth *
                      fTitle->fOutHeight / 4;
    fFrame->linesize[0] = fTitle->fOutWidth;
    fFrame->linesize[1] = fTitle->fOutWidth / 2;
    fFrame->linesize[2] = fTitle->fOutWidth / 2;

    HBBuffer * mpeg4Buffer;
    mpeg4Buffer = new HBBuffer( 3 * fTitle->fOutWidth *
                                fTitle->fOutHeight / 2 );
        /* Should be really too much... */

    mpeg4Buffer->fPosition = fResizedBuffer->fPosition;
    mpeg4Buffer->fSize =
        avcodec_encode_video( fContext, mpeg4Buffer->fData,
                              mpeg4Buffer->fAllocSize, fFrame );
    mpeg4Buffer->fKeyFrame = ( fContext->coded_frame->key_frame != 0 );

    delete fResizedBuffer;

    if( fPass == 1 )
    {
        if( fContext->stats_out )
        {
            fprintf( fFile, "%s", fContext->stats_out );
        }
        delete mpeg4Buffer;
    }
    else
    {
        /* Mux it */
        Push( fTitle->fMpeg4Fifo, mpeg4Buffer );
    }
}

