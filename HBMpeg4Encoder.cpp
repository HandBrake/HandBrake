/* $Id: HBMpeg4Encoder.cpp,v 1.18 2003/08/23 19:38:47 titer Exp $ */

#include "HBCommon.h"
#include "HBMpeg4Encoder.h"
#include "HBManager.h"
#include "HBFifo.h"

#include <ffmpeg/avcodec.h>

HBMpeg4Encoder::HBMpeg4Encoder( HBManager * manager, HBTitleInfo * titleInfo )
    : HBThread( "mpeg4encoder" )
{
    fManager   = manager;
    fTitleInfo = titleInfo;
}

void HBMpeg4Encoder::DoWork()
{
    /* Init libavcodec */
    AVCodec * codec = avcodec_find_encoder( CODEC_ID_MPEG4 );
    if( !codec )
    {
        Log( "HBMpeg4Encoder: avcodec_find_encoder() failed" );
        fManager->Error();
        return;
    }

#define WIDTH  fTitleInfo->fOutWidth
#define HEIGHT fTitleInfo->fOutHeight
#define RATE   fTitleInfo->fRate
#define SCALE  fTitleInfo->fScale

    AVCodecContext * context;
    context                      = avcodec_alloc_context();
    context->bit_rate            = 1024 * fTitleInfo->fBitrate;
    context->bit_rate_tolerance  = 1024 * fTitleInfo->fBitrate;
    context->flags              |= CODEC_FLAG_HQ;
    context->width               = WIDTH;
    context->height              = HEIGHT;
    context->frame_rate          = RATE;
    context->frame_rate_base     = SCALE;
    context->gop_size            = 10 * RATE / SCALE;
    
    if( avcodec_open( context, codec ) < 0 )
    {
        Log( "HBMpeg4Encoder: avcodec_open() failed" );
        fManager->Error();
        return;
    }

    AVFrame  * frame = avcodec_alloc_frame();
    HBBuffer * mpeg4Buffer;

    for( ;; )
    {
        /* Get another frame */
        if( !( fRawBuffer = fTitleInfo->fRawFifo->Pop() ) )
            break;

        frame->data[0] = fRawBuffer->fData;
        frame->data[1] = frame->data[0] + WIDTH * HEIGHT;
        frame->data[2] = frame->data[1] + WIDTH * HEIGHT / 4;
        frame->linesize[0] = WIDTH;
        frame->linesize[1] = WIDTH / 2;
        frame->linesize[2] = WIDTH / 2;

        mpeg4Buffer = new HBBuffer( 3 * WIDTH * HEIGHT / 2 );
            /* Should be too much. It can't be bigger than the raw video ! */

        mpeg4Buffer->fSize =
            avcodec_encode_video( context, mpeg4Buffer->fData,
                                  mpeg4Buffer->fAllocSize, frame );
        mpeg4Buffer->fKeyFrame = ( context->coded_frame->key_frame != 0 );
        
#undef WIDTH
#undef HEIGHT
#undef RATE
#undef SCALE

        delete fRawBuffer;

        /* Mux it */
        if( !fTitleInfo->fMpeg4Fifo->Push( mpeg4Buffer ) )
        {
            break;
        }
    }
}
