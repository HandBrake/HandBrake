/* $Id: Ac3Dec.c,v 1.17 2004/05/02 16:25:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* liba52 */
#include "a52dec/a52.h"

struct HBWork
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
    int           hasSync;
    int           nextFrameSize;
};

/* Local prototypes */
static int Ac3DecWork( HBWork * );

HBWork * HBAc3DecInit( HBHandle * handle, HBAudio * audio )
{
    HBWork * w;
    if( !( w = calloc( sizeof( HBWork ), 1 ) ) )
    {
        HBLog( "HBAc3DecInit: malloc() failed, gonna crash" );
        return NULL;
    }

    w->name = strdup( "Ac3Dec" );
    w->work = Ac3DecWork;

    w->handle = handle;
    w->audio  = audio;

    /* Init liba52 */
    w->state = a52_init( 0 );

    /* Let it do the downmixing */
    w->outFlags = A52_STEREO;

    /* 16 bits samples */
    w->sampleLevel = 32768.0;

    return w;
}

void HBAc3DecClose( HBWork ** _w )
{
    HBWork * w = *_w;

    a52_free( w->state );
    free( w->name );
    free( w );

    *_w = NULL;
}

static int Ac3DecWork( HBWork * w )
{
    HBAudio  * audio = w->audio;

    float      position;

    if( HBFifoIsHalfFull( audio->rawFifo ) )
    {
        return 0;
    }

    /* Get a frame header (7 bytes) */
    if( !w->hasSync )
    {
        if( !HBFifoGetBytes( audio->inFifo, w->ac3Frame, 7,
                             &position ) )
        {
            return 0;
        }

        /* Check the header */
        w->nextFrameSize = a52_syncinfo( w->ac3Frame, &w->inFlags,
                                         &audio->inSampleRate,
                                         &audio->inBitrate );

        if( !w->nextFrameSize )
        {
            HBLog( "HBAc3Dec: a52_syncinfo() failed" );
            HBErrorOccured( w->handle, HB_ERROR_A52_SYNC );
            return 0;
        }

        w->hasSync = 1;
    }

    /* Get the whole frame */
    if( w->hasSync )
    {
        sample_t * samples;
        HBBuffer * rawBuffer;
        int        i;

        if( !HBFifoGetBytes( audio->inFifo, w->ac3Frame + 7,
                             w->nextFrameSize - 7, &position ) )
        {
            return 0;
        }

        w->hasSync = 0;

        /* Feed liba52 */
        a52_frame( w->state, w->ac3Frame, &w->outFlags,
                   &w->sampleLevel, 0 );

        /* 6 blocks per frame, 256 samples per block, 2 channels */
        rawBuffer           = HBBufferInit( 3072 * sizeof( float ) );
        rawBuffer->position = position;

        for( i = 0; i < 6; i++ )
        {
            int j;

            /* Decode a block */
            a52_block( w->state );

            /* Get a pointer to the float samples */
            samples = a52_samples( w->state );

            /* Interleave samples */
            for( j = 0; j < 256; j++ )
            {
                rawBuffer->dataf[512*i+2*j]   = samples[j];
                rawBuffer->dataf[512*i+2*j+1] = samples[256+j];
            }
        }

        /* Send decoded data to the resampler */
        if( !HBFifoPush( audio->rawFifo, &rawBuffer ) )
        {
            HBLog( "HBAc3Dec: HBFifoPush failed" );
        }
    }

    return 1;
}

