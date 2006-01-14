/* $Id: FfmpegEnc.c,v 1.18 2004/01/21 17:59:33 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* libavcodec */
#include "ffmpeg/avcodec.h"

typedef struct HBFfmpegEnc
{
    HB_WORK_COMMON_MEMBERS

    HBHandle       * handle;
    HBTitle        * title;

    HBBuffer       * mpeg4Buffer;
    int              pass;
    AVCodecContext * context;
    FILE           * file;

    /* Stats */
    int              frames;
    int64_t          bytes;
} HBFfmpegEnc;

/* Local prototypes */
static int  FfmpegEncWork( HBWork * );
static int  InitAvcodec( HBFfmpegEnc * );
static void CloseAvcodec( HBFfmpegEnc * );

HBWork * HBFfmpegEncInit( HBHandle * handle, HBTitle * title )
{
    HBFfmpegEnc * f;
    if( !( f = calloc( sizeof( HBFfmpegEnc ), 1 ) ) )
    {
        HBLog( "HBFfmpegEncInit: malloc() failed, gonna crash" );
        return NULL;
    }

    f->name = strdup( "FfmpegEnc" );
    f->work = FfmpegEncWork;

    f->handle = handle;
    f->title  = title;
    f->pass   = 42;

    return (HBWork*) f;
}

void HBFfmpegEncClose( HBWork ** _f )
{
    HBFfmpegEnc * f = (HBFfmpegEnc*) *_f;

    if( f->context )
    {
        CloseAvcodec( f );
    }

    /* Stats */
    if( f->frames )
    {
        float   bitrate = (float) f->bytes * f->title->rate /
            f->frames / f->title->rateBase / 128;
        int64_t bytes = (int64_t) f->frames * f->title->bitrate * 128 *
            f->title->rateBase / f->title->rate;

        HBLog( "HBFfmpegEnc: %d frames encoded (%lld bytes), %.2f kbps",
               f->frames, f->bytes, bitrate );
        HBLog( "HBFfmpegEnc: error is %lld bytes", f->bytes - bytes );
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
        if( HBFifoPush( title->outFifo, &f->mpeg4Buffer ) )
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
        if( f->context )
        {
            CloseAvcodec( f );
        }
        
        f->pass = scaledBuffer->pass;
        
        if( !InitAvcodec( f ) )
        {
            HBErrorOccured( f->handle, HB_ERROR_MPEG4_INIT );
            return didSomething;
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
    mpeg4Buffer->size     = avcodec_encode_video( f->context,
            mpeg4Buffer->data, mpeg4Buffer->alloc, frame );
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
    else
    {
        f->mpeg4Buffer = mpeg4Buffer;

        /* Stats */
        f->frames++;
        f->bytes += mpeg4Buffer->size;
    }

    HBBufferClose( &scaledBuffer );
    free( frame );

    return didSomething;
}

static int InitAvcodec( HBFfmpegEnc * f )
{
    AVCodec        * codec;
    AVCodecContext * context;
    HBTitle        * title = f->title;

    HBLog( "HBFfmpegEnc: opening libavcodec (pass %d)", f->pass );

    codec = avcodec_find_encoder( CODEC_ID_MPEG4 );
    if( !codec )
    {
        HBLog( "HBFfmpegEnc: avcodec_find_encoder() failed" );
        HBErrorOccured( f->handle, HB_ERROR_MPEG4_INIT );
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

    if( title->mux == HB_MUX_MP4 && f->pass != 1 )
    {
        context->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

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

    if( title->mux == HB_MUX_MP4 && f->pass != 1 )
    {
        /* UGLY */
        title->esConfig = malloc( 15 );
        title->esConfigLength = 15;
        memcpy( title->esConfig, context->extradata + 15, 15 );
    }

    f->context = context;
    return 1;
}

static void CloseAvcodec( HBFfmpegEnc * f )
{
    HBLog( "HBFfmpegEnc: closing libavcodec (pass %d)",
           f->pass );

    if( f->context->stats_in )
    {
        free( f->context->stats_in );
    }
    avcodec_close( f->context );
    if( f->file )
    {
        fclose( f->file );
        f->file = NULL;
    }
    if( f->title->esConfig )
    {
        free( f->title->esConfig );
        f->title->esConfigLength = 0;
    }
}

