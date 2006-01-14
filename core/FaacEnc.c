/* $Id: FaacEnc.c,v 1.15 2004/02/18 17:07:20 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* libfaac */
#include "faac.h"

typedef struct HBFaacEnc
{
    HB_WORK_COMMON_MEMBERS

    HBHandle          * handle;
    HBAudio           * audio;

    faacEncHandle     * faac;
    unsigned long       inputSamples;
    unsigned long       maxOutputBytes;
    int32_t           * inputBuffer;
    unsigned long       samplesGot;
    HBBuffer          * rawBuffer;
    int                 rawBufferPos; /* in bytes */
    float               position;
    HBBuffer          * aacBuffer;

    /* Stats */
    int64_t             samples;
    int64_t             bytes;
} HBFaacEnc;

/* Local prototypes */
static int FaacEncWork( HBWork * );
static int GetSamples( HBFaacEnc * );

HBWork * HBFaacEncInit( HBHandle * handle, HBAudio * audio )
{
    HBFaacEnc * f;
    if( !( f = calloc( sizeof( HBFaacEnc ), 1 ) ) )
    {
        HBLog( "HBFaacEncInit: malloc() failed, gonna crash" );
        return NULL;
    }

    f->name          = strdup( "FaacEnc" );
    f->work          = FaacEncWork;

    f->handle        = handle;
    f->audio         = audio;

    return (HBWork*) f;
}

void HBFaacEncClose( HBWork ** _f )
{
    HBFaacEnc * f = (HBFaacEnc*) *_f;

    if( f->faac )
    {
        faacEncClose( f->faac );
        free( f->inputBuffer );
    }

    if( f->samples )
    {
        int64_t bytes   = 128 * f->audio->outBitrate * f->samples /
            f->audio->outSampleRate;
        float   bitrate = (float) f->bytes * f->audio->inSampleRate /
            f->samples / 128;

        HBLog( "HBFaacEnc: %lld samples encoded (%lld bytes), %.2f kbps",
                f->samples, f->bytes, bitrate );
        HBLog( "HBFaacEnc: error is %lld bytes", f->bytes - bytes );
    }
    
    free( f->name );
    free( f );

    *_f = NULL;
}

static int FaacEncWork( HBWork * w )
{
    HBFaacEnc * f     = (HBFaacEnc*) w;
    HBAudio   * audio = f->audio;

    int didSomething = 0;

    if( !f->faac )
    {
        faacEncConfigurationPtr config;

        /* Get a first buffer so we know that audio->inSampleRate is
           correct */
        if( ( f->rawBuffer = HBFifoPop( audio->rawFifo ) ) )
        {
            didSomething = 1;
        }
        else
        {
            return didSomething;
        }
        f->rawBufferPos = 0;
        f->position     = f->rawBuffer->position;

        HBLog( "HBFaacEnc: opening libfaac (%x)", audio->id );

        /* No resampling */
        audio->outSampleRate = audio->inSampleRate;

        f->faac = faacEncOpen( audio->outSampleRate, 2,
                               &f->inputSamples, &f->maxOutputBytes );
        f->inputBuffer = malloc( f->inputSamples * sizeof( int32_t ) );
        config = faacEncGetCurrentConfiguration( f->faac );
        config->mpegVersion   = MPEG4;
        config->aacObjectType = LOW;
        config->allowMidside  = 1;
        config->useLfe        = 0;
        config->useTns        = 0;
        config->bitRate       = audio->outBitrate * 512;
        config->bandWidth     = 0;
        config->outputFormat  = 0;
        faacEncSetConfiguration( f->faac, config );
        faacEncGetDecoderSpecificInfo( f->faac, &audio->esConfig,
                                       &audio->esConfigLength );
    }

    /* Push encoded buffer */
    if( f->aacBuffer )
    {
        if( HBFifoPush( audio->outFifo, &f->aacBuffer ) )
        {
            didSomething = 1;
        }
        else
        {
            return didSomething;
        }
    }

    if( GetSamples( f ) )
    {
        didSomething = 1;
    }
    else
    {
        return didSomething;
    }

    f->samplesGot = 0;

    f->aacBuffer = HBBufferInit( f->maxOutputBytes );
    f->aacBuffer->position = f->position;
    f->aacBuffer->size = faacEncEncode( f->faac, f->inputBuffer,
            f->inputSamples, f->aacBuffer->data, f->maxOutputBytes );

    f->samples += f->inputSamples / 2;

    if( !f->aacBuffer->size )
    {
        HBBufferClose( &f->aacBuffer );
    }
    else if( f->aacBuffer->size < 0 )
    {
        HBLog( "HBFaacEnc: faacEncEncode() failed" );
    }
    else
    {
        f->bytes += f->aacBuffer->size;
    }

    return didSomething;
}

static int GetSamples( HBFaacEnc * f )
{
    while( f->samplesGot < f->inputSamples )
    {
        int i, copy;

        if( !f->rawBuffer )
        {
            if( !( f->rawBuffer = HBFifoPop( f->audio->rawFifo ) ) )
            {
                return 0;
            }

            f->rawBufferPos = 0;
            f->position     = f->rawBuffer->position;
        }

        copy = MIN( f->inputSamples - f->samplesGot,
                    ( f->rawBuffer->samples - f->rawBufferPos ) * 2 );

        for( i = 0; i < copy; i += 2 )
        {
            f->inputBuffer[f->samplesGot++] =
                f->rawBuffer->left[f->rawBufferPos];
            f->inputBuffer[f->samplesGot++] =
                f->rawBuffer->right[f->rawBufferPos];
            f->rawBufferPos++;
        }

        if( f->rawBufferPos == f->rawBuffer->samples )
        {
            HBBufferClose( &f->rawBuffer );
        }
    }

    return 1;
}

