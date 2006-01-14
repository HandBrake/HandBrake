/* $Id: Ac3Dec.c,v 1.12 2004/01/16 19:04:03 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* liba52 */
#include "a52dec/a52.h"

typedef struct HBAc3Dec
{
    HB_WORK_COMMON_MEMBERS

    HBHandle    * handle;
    HBAudio     * audio;

    /* liba52 stuff */
    a52_state_t * state;
    int           inFlags;
    int           outFlags;
    float         sampleLevel;

    /* Buffers */
    uint8_t       ac3Frame[3840]; /* Max size of a A52 frame */
    int           ac3FrameSize;   /* In bytes */
    HBBuffer    * ac3Buffer;
    int           ac3BufferPos;   /* In bytes */
    int           nextFrameSize;  /* In bytes */
    float         position;
    HBBuffer    * rawBuffer;
} HBAc3Dec;

/* Local prototypes */
static int Ac3DecWork( HBWork * );
static int GetBytes( HBAc3Dec *, int );

HBWork * HBAc3DecInit( HBHandle * handle, HBAudio * audio )
{
    HBAc3Dec * a;
    if( !( a = malloc( sizeof( HBAc3Dec ) ) ) )
    {
        HBLog( "HBAc3DecInit: malloc() failed, gonna crash" );
        return NULL;
    }

    a->name = strdup( "Ac3Dec" );
    a->work = Ac3DecWork;

    a->handle = handle;
    a->audio  = audio;

    /* Init liba52 */
    a->state   = a52_init( 0 );
    a->inFlags = 0;

    /* Let it do the downmixing */
    a->outFlags = A52_STEREO;

    if( audio->codec == HB_CODEC_MP3 )
        /* Lame wants 16 bits samples */
        a->sampleLevel = 32768.0;
    else if( audio->codec == HB_CODEC_AAC )
        /* Faac wants 24 bits samples */
        a->sampleLevel = 8388608.0;
    else if( audio->codec == HB_CODEC_VORBIS )
        /* Vorbis wants FIXME bits samples */
        a->sampleLevel = 32768.0;

    a->ac3FrameSize  = 0;
    a->ac3Buffer     = NULL;
    a->ac3BufferPos  = 0;
    a->nextFrameSize = 0;
    a->position      = 0.0;
    a->rawBuffer     = NULL;

    return (HBWork*) a;
}

void HBAc3DecClose( HBWork ** _a )
{
    HBAc3Dec * a = (HBAc3Dec*) *_a;

    if( a->ac3Buffer ) HBBufferClose( &a->ac3Buffer );
    if( a->rawBuffer ) HBBufferClose( &a->rawBuffer );
    a52_free( a->state );
    free( a->name );
    free( a );

    *_a = NULL;
}

static int Ac3DecWork( HBWork * w )
{
    HBAc3Dec * a     = (HBAc3Dec*) w;
    HBAudio  * audio = a->audio;

    int didSomething = 0;

    /* Push decoded buffer */
    if( a->rawBuffer )
    {
        if( HBFifoPush( audio->rawFifo, &a->rawBuffer ) )
        {
            didSomething = 1;
        }
        else
        {
            return didSomething;
        }
    }

    /* Get a frame header (7 bytes) */
    if( a->ac3FrameSize < 7 )
    {
        if( GetBytes( a, 7 ) )
        {
            didSomething = 1;
        }
        else
        {
            return didSomething;
        }

        a->nextFrameSize = a52_syncinfo( a->ac3Frame, &a->inFlags,
                                         &audio->inSampleRate,
                                         &audio->inBitrate );

        if( !a->nextFrameSize )
        {
            HBLog( "HBAc3Dec: a52_syncinfo() failed" );
            HBErrorOccured( a->handle, HB_ERROR_A52_SYNC );
            return didSomething;
        }
    }

    /* Get the whole frame */
    if( a->ac3FrameSize >= 7 )
    {
        sample_t * samples;
        HBBuffer * rawBuffer;
        int        i;

        if( GetBytes( a, a->nextFrameSize ) )
        {
            didSomething = 1;
        }
        else
        {
            return didSomething;
        }

        /* Feed liba52 */
        a52_frame( a->state, a->ac3Frame, &a->outFlags,
                   &a->sampleLevel, 0 );
        a->ac3FrameSize = 0;

        /* 6 blocks per frame, 256 samples per block, 2 channels */
        rawBuffer           = HBBufferInit( 12 * 256 * sizeof( float ) );
        rawBuffer->position = a->position;
        rawBuffer->samples  = 6 * 256;
        rawBuffer->left     = (float*) rawBuffer->data;
        rawBuffer->right    = rawBuffer->left + 6 * 256;

        for( i = 0; i < 6; i++ )
        {
            /* Decode a block */
            a52_block( a->state );

            /* Get a pointer to the raw data */
            samples = a52_samples( a->state );

            /* Copy left channel data */
            memcpy( rawBuffer->left + i * 256, samples,
                    256 * sizeof( float ) );

            /* Copy right channel data */
            memcpy( rawBuffer->right + i * 256, samples + 256,
                    256 * sizeof( float ) );
        }

        a->rawBuffer = rawBuffer;
    }

    return didSomething;
}

static int GetBytes( HBAc3Dec * a, int size )
{
    int i;

    while( a->ac3FrameSize < size )
    {
        if( !a->ac3Buffer )
        {
            if( !( a->ac3Buffer = HBFifoPop( a->audio->inFifo ) ) )
            {
                return 0;
            }
            a->ac3BufferPos = 0;
            a->position     = a->ac3Buffer->position;
        }

        i = MIN( size - a->ac3FrameSize,
                 a->ac3Buffer->size - a->ac3BufferPos );
        memcpy( a->ac3Frame + a->ac3FrameSize,
                a->ac3Buffer->data + a->ac3BufferPos,
                i );
        a->ac3FrameSize += i;
        a->ac3BufferPos += i;

        if( a->ac3BufferPos == a->ac3Buffer->size )
        {
            HBBufferClose( &a->ac3Buffer );
        }
    }

    return 1;
}

