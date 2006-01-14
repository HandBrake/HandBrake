/* $Id: FfmpegEnc.c,v 1.5 2003/11/06 13:03:19 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "FfmpegEnc.h"
#include "Fifo.h"
#include "Work.h"

#include <ffmpeg/avcodec.h>

/* Extern functions */
void HBSetPosition( HBHandle *, float );

/* Local prototypes */
static int FfmpegEncWork( HBWork * );

struct HBFfmpegEnc
{
    HB_WORK_COMMON_MEMBERS

    HBHandle * handle;
    HBTitle * title;

    HBBuffer * mpeg4Buffer;
    int pass;
    AVCodecContext * context;
    FILE * file;
};

HBFfmpegEnc * HBFfmpegEncInit( HBHandle * handle, HBTitle * title )
{
    HBFfmpegEnc * f;
    if( !( f = malloc( sizeof( HBFfmpegEnc ) ) ) )
    {
        HBLog( "HBFfmpegEncInit: malloc() failed, gonna crash" );
        return NULL;
    }

    f->name = strdup( "FfmpegEnc" );
    f->work = FfmpegEncWork;

    f->handle = handle;
    f->title = title;

    f->mpeg4Buffer = NULL;
    f->pass = 42;
    f->context = NULL;
    f->file = NULL;

    return f;
}

void HBFfmpegEncClose( HBFfmpegEnc ** _f )
{
    HBFfmpegEnc * f = *_f;

    if( f->context )
    {
        HBLog( "HBFfmpegEnc: closing libavcodec (pass %d)",
               f->pass );

        avcodec_close( f->context );
        if( f->file )
        {
            fclose( f->file );
            f->file = NULL;
        }
    }
    free( f->name );
    free( f );

    *_f = NULL;
}

static int FfmpegEncWork( HBWork * w )
{
    HBFfmpegEnc * f     = (HBFfmpegEnc*) w;
    HBTitle     * title = f->title;

    HBBuffer * scaledBuffer;
    HBBuffer * mpeg4Buffer;
    AVFrame  * frame;

    int didSomething = 0;

    if( f->mpeg4Buffer )
    {
        if( HBFifoPush( title->mpeg4Fifo, &f->mpeg4Buffer ) )
        {
            didSomething = 1;
        }
        else
        {
            return didSomething;
        }
    }

    if( ( scaledBuffer = HBFifoPop( title->scaledFifo ) ) )
    {
        didSomething = 1;
    }
    else
    {
        return didSomething;
    }

    /* Init or re-init if needed */
    if( scaledBuffer->pass != f->pass )
    {
        AVCodec        * codec;
        AVCodecContext * context;

        if( f->context )
        {
            HBLog( "HBFfmpegEnc: closing libavcodec (pass %d)",
                   f->pass );

            avcodec_close( f->context );
            if( f->file )
            {
                fclose( f->file );
                f->file = NULL;
            }
        }

        f->pass = scaledBuffer->pass;

        HBLog( "HBFfmpegEnc: opening libavcodec (pass %d)", f->pass );
        codec = avcodec_find_encoder( CODEC_ID_MPEG4 );
        if( !codec )
        {
            HBLog( "HBFfmpegEnc: avcodec_find_encoder() failed" );
            HBErrorOccured( f->handle, HB_ERROR_MPEG4_INIT );
            return didSomething;
        }

        context                     = avcodec_alloc_context();
        context->bit_rate           = 1024 * title->bitrate;
        context->bit_rate_tolerance = 10240 * title->bitrate;
        context->width              = title->outWidth;
        context->height             = title->outHeight;
        context->frame_rate         = title->rate;
        context->frame_rate_base    = title->rateBase;
        context->gop_size           = 10 * title->rate /
                                          title->rateBase;

        if( f->pass )
        {
            char fileName[1024]; memset( fileName, 0, 1024 );
            sprintf( fileName, "/tmp/HB.%d.ffmpeg.log",
                     HBGetPid( f->handle ) );

            if( f->pass == 1 )
            {
                f->file = fopen( fileName, "w" );

                context->flags |= CODEC_FLAG_PASS1;
            }
            else
            {
                FILE * file;
                int    size;
                char * log;

                file = fopen( fileName, "r" );
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

        if( avcodec_open( context, codec ) < 0 )
        {
            HBLog( "HBFfmpegEnc: avcodec_open() failed" );
            HBErrorOccured( f->handle, HB_ERROR_MPEG4_INIT );
            return didSomething;
        }

        f->context = context;
    }

    frame = avcodec_alloc_frame();
    frame->data[0] = scaledBuffer->data;
    frame->data[1] = frame->data[0] + title->outWidth *
                     title->outHeight;
    frame->data[2] = frame->data[1] + title->outWidth *
                     title->outHeight / 4;
    frame->linesize[0] = title->outWidth;
    frame->linesize[1] = title->outWidth / 2;
    frame->linesize[2] = title->outWidth / 2;

    mpeg4Buffer = HBBufferInit( 3 * title->outWidth *
                                title->outHeight / 2 );
    mpeg4Buffer->position = scaledBuffer->position;
    mpeg4Buffer->size =
        avcodec_encode_video( f->context, mpeg4Buffer->data,
                              mpeg4Buffer->alloc, frame );
    mpeg4Buffer->keyFrame = f->context->coded_frame->key_frame;

    /* Inform the GUI about the current position */
    HBPosition( f->handle, scaledBuffer->position );

    if( f->pass == 1 )
    {
        if( f->context->stats_out )
        {
            fprintf( f->file, "%s", f->context->stats_out );
        }
        HBBufferClose( &mpeg4Buffer );
    }

    HBBufferClose( &scaledBuffer );
    free( frame );

    f->mpeg4Buffer = mpeg4Buffer;

    return didSomething;
}
