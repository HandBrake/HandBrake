/* $Id: LpcmDec.c,v 1.10 2004/05/10 16:50:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

struct HBWork
{
    HB_WORK_COMMON_MEMBERS

    HBHandle    * handle;
    HBAudio     * audio;

    int           initDone;
};

/* Local prototypes */
static int LpcmDecWork( HBWork * );

HBWork * HBLpcmDecInit( HBHandle * handle, HBAudio * audio )
{
    HBWork * w;
    if( !( w = calloc( sizeof( HBWork ), 1 ) ) )
    {
        HBLog( "HBLpcmDecInit: malloc() failed, gonna crash" );
        return NULL;
    }

    w->name = strdup( "LpcmDec" );
    w->work = LpcmDecWork;

    w->handle = handle;
    w->audio  = audio;

    return w;
}

void HBLpcmDecClose( HBWork ** _w )
{
    HBWork * w = *_w;
    free( w->name );
    free( w );
    *_w = NULL;
}

static int LpcmDecWork( HBWork * w )
{
    HBAudio   * audio  = w->audio;

    HBBuffer  * lpcmBuffer;
    HBBuffer  * rawBuffer;
    uint8_t   * samples_u8;
    float     * samples_f;
    int         samples_nr, i;

    if( HBFifoIsHalfFull( audio->rawFifo ) )
    {
        return 0;
    }

    /* Get a new LPCM buffer */
    lpcmBuffer = HBFifoPop( audio->inFifo );
    if( !lpcmBuffer )
    {
        return 0;
    }

    if( !w->initDone )
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
                HBLog( "HBLpcmDec: unknown samplerate (%d)",
                       ( lpcmBuffer->data[4] >> 4 ) & 0x3 );
        }

        /* We hope there are 2 channels */
        HBLog( "HBLpcmDec: samplerate: %d Hz, channels: %d",
               audio->inSampleRate, ( lpcmBuffer->data[4] & 0x7 ) + 1 );

        w->initDone = 1;
    }

    if( lpcmBuffer->data[5] != 0x80 )
    {
        HBLog( "HBLpcmDec: no frame sync (%02x)", lpcmBuffer->data[5] );
    }

    /* Allocate raw buffer */
    samples_nr          = ( lpcmBuffer->size - 6 ) / sizeof( int16_t );
    rawBuffer           = HBBufferInit( samples_nr * sizeof( float ) );
    rawBuffer->position = lpcmBuffer->position;

    /* Big endian int16 -> float conversion (happy casting) */
    samples_u8 = lpcmBuffer->data + 6;
    samples_f  = rawBuffer->dataf;
    for( i = 0; i < samples_nr; i++ )
    {
        samples_f[0] = (float) (int16_t)
            ( ( ( (uint16_t) samples_u8[0] ) << 8 ) +
                  (uint16_t) samples_u8[1] );
        samples_u8 += 2;
        samples_f  += 1;
    }

    HBBufferClose( &lpcmBuffer );

    if( !HBFifoPush( audio->rawFifo, &rawBuffer ) )
    {
        HBLog( "HBLpcmDec: HBFifoPush failed" );
    }

    return 1;
}

