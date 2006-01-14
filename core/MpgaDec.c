/* $Id: MpgaDec.c,v 1.3 2004/05/02 16:25:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

#include "ffmpeg/avcodec.h"

struct HBWork
{
    HB_WORK_COMMON_MEMBERS

    HBHandle    * handle;
    HBAudio     * audio;

    AVCodecContext * context;
};

/* Local prototypes */
static int MpgaDecWork( HBWork * );

HBWork * HBMpgaDecInit( HBHandle * handle, HBAudio * audio )
{
    HBWork  * w;
    AVCodec * codec;

    if( !( w = calloc( sizeof( HBWork ), 1 ) ) )
    {
        HBLog( "HBMpgaDecInit: malloc() failed, gonna crash" );
        return NULL;
    }

    w->name = strdup( "MpgaDec" );
    w->work = MpgaDecWork;

    w->handle = handle;
    w->audio  = audio;

    codec = avcodec_find_decoder( CODEC_ID_MP2 );
    if( !codec )
    {
        HBLog( "HBMpgaDec: avcodec_find_decoder failed" );
    }

    w->context = avcodec_alloc_context();
    if( !w->context )
    {
        HBLog( "HBMpgaDec: avcodec_alloc_context failed" );
    }

    if( avcodec_open( w->context, codec ) < 0 )
    {
        HBLog( "HBMpgaDec: avcodec_open failed" );
    }

    return w;
}

void HBMpgaDecClose( HBWork ** _w )
{
    HBWork * w = *_w;

    avcodec_close( w->context );
    free( w->name );
    free( w );
    *_w = NULL;
}

static int MpgaDecWork( HBWork * w )
{
    HBAudio   * audio  = w->audio;

    HBBuffer  * mpgaBuffer;
    HBBuffer  * rawBuffer;

    int out_size, len, pos;
    short buffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];

    if( HBFifoIsHalfFull( audio->rawFifo ) )
    {
        return 0;
    }

    /* Get a new mpeg buffer */
    mpgaBuffer = HBFifoPop( audio->inFifo );
    if( !mpgaBuffer )
    {
        return 0;
    }

    pos = 0;
    while( pos < mpgaBuffer->size )
    {
        len = avcodec_decode_audio( w->context, buffer, &out_size,
                                    mpgaBuffer->data + pos,
                                    mpgaBuffer->size - pos );
        pos += len;

        if( !audio->inSampleRate )
        {
            audio->inSampleRate = w->context->sample_rate;
            HBLog( "HBMpgaDec: samplerate = %d", audio->inSampleRate );
        }

        if( out_size )
        {
            int i;
            rawBuffer = HBBufferInit( 2 * AVCODEC_MAX_AUDIO_FRAME_SIZE );
            rawBuffer->position = mpgaBuffer->position;

            /* s16 -> float */
            for( i = 0; i < out_size / 2; i++ )
            {
                rawBuffer->dataf[i] = buffer[i];
            }

            rawBuffer->size = out_size * 2;

            if( !HBFifoPush( audio->rawFifo, &rawBuffer ) )
            {
                HBLog( "HBMpgaDec: HBFifoPush failed" );
            }
        }
    }

    HBBufferClose( &mpgaBuffer );

    return 1;
}

