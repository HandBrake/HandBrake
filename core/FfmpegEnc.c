/* $Id: FfmpegEnc.c,v 1.26 2004/05/12 18:02:35 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* libavcodec */
#include "ffmpeg/avcodec.h"

struct HBWork
{
    HB_WORK_COMMON_MEMBERS

    HBHandle       * handle;
    HBTitle        * title;

    int              pass;
    AVCodecContext * context;
    FILE           * file;
};

/* Local prototypes */
static int  FfmpegEncWork( HBWork * );
static int  InitAvcodec( HBWork * );
static void CloseAvcodec( HBWork * );

HBWork * HBFfmpegEncInit( HBHandle * handle, HBTitle * title )
{
    HBWork * w;
    if( !( w = calloc( sizeof( HBWork ), 1 ) ) )
    {
        HBLog( "HBFfmpegEncInit: malloc() failed, gonna crash" );
        return NULL;
    }

    w->name = strdup( "FfmpegEnc" );
    w->work = FfmpegEncWork;

    w->handle = handle;
    w->title  = title;
    w->pass   = 42;

    return w;
}

void HBFfmpegEncClose( HBWork ** _w )
{
    HBWork * w = *_w;

    if( w->context )
    {
        CloseAvcodec( w );
    }

    free( w->name );
    free( w );

    *_w = NULL;
}

static int FfmpegEncWork( HBWork * w )
{
    HBTitle  * title = w->title;

    HBBuffer * scaledBuffer;
    HBBuffer * mpeg4Buffer;
    AVFrame  * frame;

    if( HBFifoIsHalfFull( title->outFifo ) )
    {
        return 0;
    }

    if( !( scaledBuffer = HBFifoPop( title->scaledFifo ) ) )
    {
        return 0;
    }

    /* Init or re-init if needed */
    if( scaledBuffer->pass != w->pass )
    {
        if( w->context )
        {
            CloseAvcodec( w );
        }

        w->pass = scaledBuffer->pass;

        if( !InitAvcodec( w ) )
        {
            HBErrorOccured( w->handle, HB_ERROR_MPEG4_INIT );
            return 0;
        }
    }

    frame              = avcodec_alloc_frame();
    frame->data[0]     = scaledBuffer->data;
    frame->data[1]     = frame->data[0] + title->outWidth *
                         title->outHeight;
    frame->data[2]     = frame->data[1] + title->outWidth *
                         title->outHeight / 4;
    frame->linesize[0] = title->outWidth;
    frame->linesize[1] = title->outWidth / 2;
    frame->linesize[2] = title->outWidth / 2;

    mpeg4Buffer = HBBufferInit( 3 * title->outWidth *
                                title->outHeight / 2 );
    mpeg4Buffer->position = scaledBuffer->position;
    mpeg4Buffer->size     = avcodec_encode_video( w->context,
            mpeg4Buffer->data, mpeg4Buffer->alloc, frame );
    mpeg4Buffer->keyFrame = w->context->coded_frame->key_frame;

    /* Inform the GUI about the current position */
    HBPosition( w->handle, scaledBuffer->position );

    if( w->pass == 1 )
    {
        if( w->context->stats_out )
        {
            fprintf( w->file, "%s", w->context->stats_out );
        }
        HBBufferClose( &mpeg4Buffer );
    }
    else
    {
        if( !HBFifoPush( title->outFifo, &mpeg4Buffer ) )
        {
            HBLog( "HBFfmpegEnc: HBFifoPush failed" );
        }
    }

    HBBufferClose( &scaledBuffer );
    free( frame );

    return 1;
}

static int InitAvcodec( HBWork * w )
{
    AVCodec        * codec;
    AVCodecContext * context;
    HBTitle        * title = w->title;

    HBLog( "HBFfmpegEnc: opening libavcodec (pass %d)", w->pass );

    codec = avcodec_find_encoder( CODEC_ID_MPEG4 );
    if( !codec )
    {
        HBLog( "HBFfmpegEnc: avcodec_find_encoder() failed" );
        HBErrorOccured( w->handle, HB_ERROR_MPEG4_INIT );
        return 0;
    }

    context                     = avcodec_alloc_context();
    context->bit_rate           = 1024 * title->bitrate;
    context->bit_rate_tolerance = 10 * context->bit_rate;
    context->width              = title->outWidth;
    context->height             = title->outHeight;
    context->frame_rate         = title->rate;
    context->frame_rate_base    = title->rateBase;
    context->gop_size           = 10 * title->rate / title->rateBase;

    if( title->mux == HB_MUX_MP4 && w->pass != 1 )
    {
        context->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    if( w->pass )
    {
        char fileName[1024]; memset( fileName, 0, 1024 );
#ifndef HB_CYGWIN
        sprintf( fileName, "/tmp/HB.%d.ffmpeg.log",
                 HBGetPid( w->handle ) );
#else
        sprintf( fileName, "C:\\HB.%d.ffmpeg.log",
                 HBGetPid( w->handle ) );
#endif

        if( w->pass == 1 )
        {
            w->file = fopen( fileName, "wb" );
            context->flags |= CODEC_FLAG_PASS1;
        }
        else
        {
            FILE * file;
            int    size;
            char * log;

            file = fopen( fileName, "rb" );
            fseek( file, 0, SEEK_END );
            size = ftell( file );
            fseek( file, 0, SEEK_SET );
            if( !( log = malloc( size + 1 ) ) )
            {
                HBLog( "HBFfmpegEnc: malloc() failed, gonna crash" );
            }
            log[size] = '\0';
            fread( log, size, 1, file );
            fclose( file );

            context->flags    |= CODEC_FLAG_PASS2;
            context->stats_in  = log;
        }
    }

#ifdef HB_NOMMX
    context->dct_algo  = FF_DCT_INT;
    context->idct_algo = FF_IDCT_INT;
    context->dsp_mask  = 0x1F;
#endif

    if( avcodec_open( context, codec ) < 0 )
    {
        HBLog( "HBFfmpegEnc: avcodec_open() failed" );
        return 0;
    }

    if( title->mux == HB_MUX_MP4 && w->pass != 1 )
    {
        /* UGLY */
        title->esConfig = malloc( 15 );
        title->esConfigLength = 15;
        memcpy( title->esConfig, context->extradata + 15, 15 );
    }

    w->context = context;
    return 1;
}

static void CloseAvcodec( HBWork * w )
{
    HBLog( "HBFfmpegEnc: closing libavcodec (pass %d)",
           w->pass );

    if( w->context->stats_in )
    {
        free( w->context->stats_in );
    }
    avcodec_close( w->context );
    if( w->file )
    {
        fclose( w->file );
        w->file = NULL;
    }
    if( w->title->esConfig )
    {
        free( w->title->esConfig );
        w->title->esConfig       = NULL;
        w->title->esConfigLength = 0;
    }
}

