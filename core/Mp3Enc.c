/* $Id: Mp3Enc.c,v 1.5 2003/11/07 21:52:57 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "Fifo.h"
#include "Mp3Enc.h"
#include "Work.h"

#include <lame/lame.h>

/* Local prototypes */
static int Mp3EncWork( HBWork * );
static int GetBytes( HBMp3Enc * );

struct HBMp3Enc
{
    HB_WORK_COMMON_MEMBERS

    HBHandle          * handle;
    HBAudio           * audio;
    lame_global_flags * globalFlags;
    HBBuffer          * rawBuffer;
    int                 rawBufferPos;
    float               position;
    int                 samplesNeeded;
    int                 samplesGot;
    float             * left;
    float             * right;
    HBBuffer          * mp3Buffer;
};

HBMp3Enc * HBMp3EncInit( HBHandle * handle, HBAudio * audio )
{
    HBMp3Enc * m;
    if( !( m = malloc( sizeof( HBMp3Enc ) ) ) )
    {
        HBLog( "HBMp3EncInit: malloc() failed, gonna crash" );
        return NULL;
    }

    m->name          = strdup( "Mp3Enc" );
    m->work          = Mp3EncWork;
    
    m->handle        = handle;
    m->audio         = audio;
    m->globalFlags   = NULL;
    m->rawBuffer     = NULL;
    m->rawBufferPos  = 0;
    m->position      = 0.0;
    m->samplesNeeded = 0;
    m->samplesGot    = 0;
    m->left          = NULL;
    m->right         = NULL;
    m->mp3Buffer     = NULL;

    return m;
}

void HBMp3EncClose( HBMp3Enc ** _m )
{
    HBMp3Enc * m = *_m;
    
    if( m->globalFlags ) lame_close( m->globalFlags );
    if( m->rawBuffer )   HBBufferClose( &m->rawBuffer );
    if( m->left )        free( m->left );
    if( m->right )       free( m->right );
    if( m->mp3Buffer )   HBBufferClose( &m->mp3Buffer );
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
        m->samplesNeeded =  1152 * audio->inSampleRate /
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

        m->left  = malloc( m->samplesNeeded * sizeof( float ) );
        m->right = malloc( m->samplesNeeded * sizeof( float ) );

        if( !m->left || !m->right )
        {
            HBLog( "HBMp3Enc: malloc() failed, gonna crash" );
        }

        for( i = 0; i < m->samplesNeeded; i++ )
        {
            m->left[i]  = 0.0;
            m->right[i] = 0.0;
        }
    }

    /* Push encoded buffer */
    if( m->mp3Buffer )
    {
        if( HBFifoPush( audio->mp3Fifo, &m->mp3Buffer ) )
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
        int length = m->samplesNeeded * 1000 / audio->inSampleRate;
        
        if( audio->delay > length )
        {
            HBLog( "HBMp3Enc: adding %d ms of silence", length );
            m->samplesGot  = m->samplesNeeded;
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
        int length = m->samplesNeeded * 1000 / audio->inSampleRate;

        if( - audio->delay > length )
        {
            if( GetBytes( m ) )
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
    if( GetBytes( m ) )
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
                                          m->right, m->samplesNeeded,
                                          mp3Buffer->data,
                                          mp3Buffer->size );

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
    }

    return didSomething;
}

static int GetBytes( HBMp3Enc * m )
{
    while( m->samplesGot < m->samplesNeeded )
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

        i = MIN( m->samplesNeeded - m->samplesGot,
                 ( m->rawBuffer->size / 2 -
                   m->rawBufferPos ) / sizeof( float ) );

        memcpy( m->left + m->samplesGot,
                m->rawBuffer->data + m->rawBufferPos,
                i * sizeof( float ) );
        memcpy( m->right + m->samplesGot,
                m->rawBuffer->data + m->rawBuffer->size / 2 +
                    m->rawBufferPos,
                i * sizeof( float ) );

        m->samplesGot   += i;
        m->rawBufferPos += i * sizeof( float );

        if( m->rawBufferPos == m->rawBuffer->size / 2 )
        {
            HBBufferClose( &m->rawBuffer );
        }
    }

    return 1;
}
