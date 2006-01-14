/* $Id: LpcmDec.c,v 1.3 2004/03/29 00:29:41 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

typedef struct HBLpcmDec
{
    HB_WORK_COMMON_MEMBERS

    HBHandle    * handle;
    HBAudio     * audio;

    int           initDone;
    int           channels;
    float         sampleLevel;
    HBBuffer    * rawBuffer;
} HBLpcmDec;

/* Local prototypes */
static int LpcmDecWork( HBWork * );

HBWork * HBLpcmDecInit( HBHandle * handle, HBAudio * audio )
{
    HBLpcmDec * l;
    if( !( l = calloc( sizeof( HBLpcmDec ), 1 ) ) )
    {
        HBLog( "HBLpcmDecInit: malloc() failed, gonna crash" );
        return NULL;
    }

    l->name = strdup( "LpcmDec" );
    l->work = LpcmDecWork;

    l->handle = handle;
    l->audio  = audio;

    if( audio->outCodec & ( HB_CODEC_MP3 | HB_CODEC_VORBIS ) )
    {
        /* 16 bits samples */
        l->sampleLevel = 1.0;
    }
    else if( audio->outCodec & HB_CODEC_AAC )
    {
        /* 24 bits samples */
        l->sampleLevel = 256.0;
    }

    return (HBWork*) l;
}

void HBLpcmDecClose( HBWork ** _l )
{
    HBLpcmDec * l = (HBLpcmDec*) *_l;

    /* Clean up */
    if( l->rawBuffer )
    {
        HBBufferClose( &l->rawBuffer );
    }
    free( l->name );
    free( l );

    *_l = NULL;
}

#ifndef HB_MACOSX
static int16_t Swap16( int16_t * p )
{
    uint8_t tmp[2];

    tmp[0] = ((uint8_t*)p)[1];
    tmp[1] = ((uint8_t*)p)[0];

    return *(int16_t*)tmp;
}
#endif

static int LpcmDecWork( HBWork * w )
{
    HBLpcmDec * l          = (HBLpcmDec*) w;
    HBAudio   * audio      = l->audio;
    HBBuffer  * lpcmBuffer;
    int16_t   * int16data;

    int i;
    int samples;
    int didSomething = 0;

    /* Push decoded buffer */
    if( l->rawBuffer )
    {
        if( HBFifoPush( audio->rawFifo, &l->rawBuffer ) )
        {
            didSomething = 1;
        }
        else
        {
            return 0;
        }
    }

    /* Get a new LPCM buffer */
    lpcmBuffer = HBFifoPop( audio->inFifo );
    if( !lpcmBuffer )
    {
        return didSomething;
    }

    if( !l->initDone )
    {
        /* SampleRate */
        switch( ( lpcmBuffer->data[4] >> 4 ) & 0x3 )
        {
            case 0:
                audio->inSampleRate = 48000;
                break;
            case 1:
                audio->inSampleRate = 32000;
                break;
            default:
                HBLog( "LpcmDec: unknown samplerate (%d)",
                       ( lpcmBuffer->data[4] >> 4 ) & 0x3 );
        }
        HBLog( "LpcmDec: samplerate = %d Hz", audio->inSampleRate );

        /* Channels */
        HBLog( "LpcmDec: %d channels",
               ( lpcmBuffer->data[4] & 0x7 ) + 1 );

        l->initDone = 1;
    }

    if( lpcmBuffer->data[5] != 0x80 )
    {
        HBLog( "LpcmDec: no frame synx (%02xà", lpcmBuffer->data[5] );
    }

    samples   = ( lpcmBuffer->size - 6 ) / sizeof( int16_t ) / 2;
    int16data = (int16_t*) ( lpcmBuffer->data + 6 );

    l->rawBuffer           = HBBufferInit( samples * sizeof( float ) * 2 );
    l->rawBuffer->left     = (float*) l->rawBuffer->data;
    l->rawBuffer->right    = l->rawBuffer->left + samples;
    l->rawBuffer->position = lpcmBuffer->position;
    l->rawBuffer->samples  = samples;

    for( i = 0; i < samples; i++ )
    {
#ifdef HB_MACOSX
        l->rawBuffer->left[i]  = (float) int16data[2*i]   * l->sampleLevel;
        l->rawBuffer->right[i] = (float) int16data[2*i+1] * l->sampleLevel;
#else
        l->rawBuffer->left[i]  = (float) Swap16(&int16data[2*i])   * l->sampleLevel;
        l->rawBuffer->right[i] = (float) Swap16(&int16data[2*i+1]) * l->sampleLevel;
#endif
    }

    HBBufferClose( &lpcmBuffer );

    return 1;
}

