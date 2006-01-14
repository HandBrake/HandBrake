/* $Id: Mp3Enc.c,v 1.13 2004/01/21 17:59:33 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* libmp3lame */
#include "lame/lame.h"

typedef struct HBMp3Enc
{
    HB_WORK_COMMON_MEMBERS

    HBHandle          * handle;
    HBAudio           * audio;
    lame_global_flags * globalFlags;
    HBBuffer          * rawBuffer;
    int                 rawBufferPos;
    float               position;
    int                 inputSamples;
    int                 samplesGot;
    float             * left;
    float             * right;
    HBBuffer          * mp3Buffer;

    /* Stats */
    int64_t             samples;
    int64_t             bytes;
} HBMp3Enc;

/* Local prototypes */
static int Mp3EncWork( HBWork * );
static int GetSamples( HBMp3Enc * );

HBWork * HBMp3EncInit( HBHandle * handle, HBAudio * audio )
{
    HBMp3Enc * m;
    if( !( m = calloc( sizeof( HBMp3Enc ), 1 ) ) )
    {
        HBLog( "HBMp3EncInit: malloc() failed, gonna crash" );
        return NULL;
    }

    m->name          = strdup( "Mp3Enc" );
    m->work          = Mp3EncWork;

    m->handle        = handle;
    m->audio         = audio;

    return (HBWork*) m;
}

void HBMp3EncClose( HBWork ** _m )
{
    HBMp3Enc * m = (HBMp3Enc*) *_m;

    if( m->globalFlags ) lame_close( m->globalFlags );
    if( m->rawBuffer )   HBBufferClose( &m->rawBuffer );
    if( m->left )        free( m->left );
    if( m->right )       free( m->right );
    if( m->mp3Buffer )   HBBufferClose( &m->mp3Buffer );

    if( m->samples )
    {
        int64_t bytes = 128 * m->audio->outBitrate * m->samples /
            m->audio->inSampleRate;
        float bitrate = (float) m->bytes * m->audio->inSampleRate /
            m->samples / 128;

        HBLog( "HBMp3Enc: %lld samples encoded (%lld bytes), %.2f kbps",
                m->samples, m->bytes, bitrate );
        HBLog( "HBFaacEnc: error is %lld bytes", m->bytes - bytes );
    }
    
    free( m->name );
    free( m );

    *_m = NULL;
}

static int Mp3EncWork( HBWork * w )
{
    HBMp3Enc * m     = (HBMp3Enc*) w;
    HBAudio  * audio = m->audio;

    HBBuffer * mp3Buffer;
    int        ret;

    int didSomething = 0;

    if( !m->globalFlags )
    {
        int i;

        /* Get a first buffer so we know that audio->inSampleRate is
           correct */
        if( ( m->rawBuffer = HBFifoPop( audio->rawFifo ) ) )
        {
            didSomething = 1;
        }
        else
        {
            return didSomething;
        }
        m->rawBufferPos = 0;
        m->position     = m->rawBuffer->position;

        /* The idea is to have exactly one mp3 frame (i.e. 1152 samples) by
           output buffer. As we are resampling from inSampleRate to
           outSampleRate, we will give ( 1152 * inSampleRate ) /
           ( 2 * outSampleRate ) samples to libmp3lame so we are sure we
           will never get more than 1 frame at a time */
        audio->outSampleRate = 44100;
        m->inputSamples =  1152 * audio->inSampleRate /
                            audio->outSampleRate / 2;

        HBLog( "HBMp3Enc: opening lame (%d->%d Hz, %d kbps)",
               audio->inSampleRate, audio->outSampleRate,
               audio->outBitrate );
        m->globalFlags = lame_init();
        lame_set_in_samplerate( m->globalFlags, audio->inSampleRate );
        lame_set_out_samplerate( m->globalFlags, audio->outSampleRate );
        lame_set_brate( m->globalFlags, audio->outBitrate );

        if( lame_init_params( m->globalFlags ) == -1 )
        {
            HBLog( "HBMp3Enc: lame_init_params() failed" );
            HBErrorOccured( m->handle, HB_ERROR_MP3_INIT );
            return didSomething;
        }

        m->left  = malloc( m->inputSamples * sizeof( float ) );
        m->right = malloc( m->inputSamples * sizeof( float ) );

        if( !m->left || !m->right )
        {
            HBLog( "HBMp3Enc: malloc() failed, gonna crash" );
        }

        for( i = 0; i < m->inputSamples; i++ )
        {
            m->left[i]  = 0.0;
            m->right[i] = 0.0;
        }
    }

    /* Push encoded buffer */
    if( m->mp3Buffer )
    {
        if( HBFifoPush( audio->outFifo, &m->mp3Buffer ) )
        {
            didSomething = 1;
        }
        else
        {
            return didSomething;
        }
    }

    /* A/V synchro fix in case audio doesn't start at the same time
       than video */
    if( audio->delay > 0 )
    {
        /* Audio starts later - insert some silence */
        int length = m->inputSamples * 1000 / audio->inSampleRate;

        if( audio->delay > length )
        {
            HBLog( "HBMp3Enc: adding %d ms of silence", length );
            m->samplesGot  = m->inputSamples;
            audio->delay  -= length;
        }
        else
        {
            audio->delay = 0;
        }
    }
    else if( audio->delay < 0 )
    {
        /* Audio starts sooner - trash some */
        int length = m->inputSamples * 1000 / audio->inSampleRate;

        if( - audio->delay > length )
        {
            if( GetSamples( m ) )
            {
                didSomething = 1;
                HBLog( "HBMp3Enc: trashing %d ms", length );
                m->samplesGot  = 0;
                audio->delay  += length;
                return didSomething;
            }
            else
            {
                return didSomething;
            }
        }
        else
        {
            audio->delay = 0;
        }
    }

    /* Get new samples */
    if( GetSamples( m ) )
    {
        didSomething = 1;
    }
    else
    {
        return didSomething;
    }

    m->samplesGot = 0;

    mp3Buffer = HBBufferInit( LAME_MAXMP3BUFFER );
    ret       = lame_encode_buffer_float( m->globalFlags, m->left,
                                          m->right, m->inputSamples,
                                          mp3Buffer->data,
                                          mp3Buffer->size );
    /* Stats */
    m->samples += m->inputSamples;

    if( ret < 0 )
    {
        /* Error */
        HBLog( "HBMp3Enc: lame_encode_buffer_float() failed (%d)",
               ret );
        HBErrorOccured( m->handle, HB_ERROR_MP3_ENCODE );
        HBBufferClose( &mp3Buffer );
    }
    else if( ret == 0 )
    {
        /* No error, but nothing encoded */
        HBBufferClose( &mp3Buffer );
    }
    else
    {
        /* Encoding was successful */
        mp3Buffer->size     = ret;
        mp3Buffer->keyFrame = 1;
        mp3Buffer->position = m->position;
        m->mp3Buffer = mp3Buffer;

        /* Stats */
        m->bytes   += ret;
    }

    return didSomething;
}

static int GetSamples( HBMp3Enc * m )
{
    while( m->samplesGot < m->inputSamples )
    {
        int i;

        if( !m->rawBuffer )
        {
            if( !( m->rawBuffer = HBFifoPop( m->audio->rawFifo ) ) )
            {
                return 0;
            }

            m->rawBufferPos = 0;
            m->position     = m->rawBuffer->position;
        }

        i = MIN( m->inputSamples - m->samplesGot,
                 m->rawBuffer->samples - m->rawBufferPos );

        memcpy( m->left + m->samplesGot,
                m->rawBuffer->left + m->rawBufferPos,
                i * sizeof( float ) );
        memcpy( m->right + m->samplesGot,
                m->rawBuffer->right + m->rawBufferPos,
                i * sizeof( float ) );

        m->samplesGot   += i;
        m->rawBufferPos += i;

        if( m->rawBufferPos == m->rawBuffer->samples )
        {
            HBBufferClose( &m->rawBuffer );
        }
    }

    return 1;
}
