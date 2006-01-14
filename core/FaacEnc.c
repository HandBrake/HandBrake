/* $Id: FaacEnc.c,v 1.20 2004/05/02 16:25:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* libfaac */
#include "faac.h"

struct HBWork
{
    HB_WORK_COMMON_MEMBERS

    HBHandle          * handle;
    HBAudio           * audio;

    faacEncHandle     * faac;
    unsigned long       inputSamples;
    unsigned long       maxOutputBytes;
    float             * inputBuffer;
};

/* Local prototypes */
static int FaacEncWork( HBWork * );

HBWork * HBFaacEncInit( HBHandle * handle, HBAudio * audio )
{
    HBWork * w;
    if( !( w = calloc( sizeof( HBWork ), 1 ) ) )
    {
        HBLog( "HBFaacEncInit: malloc() failed, gonna crash" );
        return NULL;
    }

    w->name   = strdup( "FaacEnc" );
    w->work   = FaacEncWork;

    w->handle = handle;
    w->audio  = audio;

    return w;
}

void HBFaacEncClose( HBWork ** _w )
{
    HBWork * w = *_w;

    if( w->faac )
    {
        faacEncClose( w->faac );
        free( w->inputBuffer );
    }

    free( w->name );
    free( w );

    *_w = NULL;
}

static int FaacEncWork( HBWork * w )
{
    HBAudio   * audio = w->audio;

    HBBuffer * aacBuffer;
    float      position;

    if( !w->faac )
    {
        faacEncConfigurationPtr config;

        if( !HBFifoSize( audio->resampleFifo ) )
        {
            return 0;
        }

        HBLog( "HBFaacEnc: opening libfaac (%x)", audio->id );

        w->faac = faacEncOpen( audio->outSampleRate, 2,
                               &w->inputSamples, &w->maxOutputBytes );
        w->inputBuffer = malloc( w->inputSamples * sizeof( float ) );
        config = faacEncGetCurrentConfiguration( w->faac );
        config->mpegVersion   = MPEG4;
        config->aacObjectType = LOW;
        config->allowMidside  = 1;
        config->useLfe        = 0;
        config->useTns        = 0;
        config->bitRate       = audio->outBitrate * 500; /* per channel */
        config->bandWidth     = 0;
        config->outputFormat  = 0;
        config->inputFormat   = FAAC_INPUT_FLOAT;
        if( !faacEncSetConfiguration( w->faac, config ) )
        {
            HBLog( "HBFaacEnc: faacEncSetConfiguration failed" );
        }
        if( faacEncGetDecoderSpecificInfo( w->faac, &audio->esConfig,
                    &audio->esConfigLength ) < 0 )
        {
            HBLog( "HBFaacEnc: faacEncGetDecoderSpecificInfo failed" );
        }
    }

    if( HBFifoIsHalfFull( audio->outFifo ) )
    {
        return 0;
    }

    if( !HBFifoGetBytes( audio->resampleFifo,
                         (uint8_t *) w->inputBuffer,
                         w->inputSamples * sizeof( float ),
                         &position ) )
    {
        return 0;
    }

    aacBuffer = HBBufferInit( w->maxOutputBytes );
    aacBuffer->position = position;
    aacBuffer->size = faacEncEncode( w->faac, (int32_t*)w->inputBuffer,
        w->inputSamples, aacBuffer->data, w->maxOutputBytes );

    if( !aacBuffer->size )
    {
        HBBufferClose( &aacBuffer );
    }
    else if( aacBuffer->size < 0 )
    {
        HBLog( "HBFaacEnc: faacEncEncode() failed" );
    }
    else
    {
        if( !HBFifoPush( audio->outFifo, &aacBuffer ) )
        {
            HBLog( "HBFaacEnc: HBFifoPush failed" );
        }
    }

    return 1;
}

