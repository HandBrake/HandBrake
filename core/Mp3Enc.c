/* $Id: Mp3Enc.c,v 1.23 2004/05/02 16:25:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* libmp3lame */
#include "lame/lame.h"

struct HBWork
{
    HB_WORK_COMMON_MEMBERS

    HBHandle          * handle;
    HBAudio           * audio;
    lame_global_flags * globalFlags;
};

/* Local prototypes */
static int Mp3EncWork( HBWork * );

HBWork * HBMp3EncInit( HBHandle * handle, HBAudio * audio )
{
    HBWork * w;
    if( !( w = calloc( sizeof( HBWork ), 1 ) ) )
    {
        HBLog( "HBMp3EncInit: malloc() failed, gonna crash" );
        return NULL;
    }

    w->name   = strdup( "Mp3Enc" );
    w->work   = Mp3EncWork;

    w->handle = handle;
    w->audio  = audio;

    return w;
}

void HBMp3EncClose( HBWork ** _w )
{
    HBWork * w = *_w;

    if( w->globalFlags ) lame_close( w->globalFlags );

    free( w->name );
    free( w );
    *_w = NULL;
}

static int Mp3EncWork( HBWork * w )
{
    HBAudio  * audio = w->audio;

    HBBuffer * mp3Buffer;
    int        ret;

    float   samples_f[1152 * 2];
    int16_t samples_s16[1152 * 2];
    float   position;
    int     i;

    if( !w->globalFlags )
    {
        if( !HBFifoSize( audio->resampleFifo ) )
        {
            return 0;
        }

        HBLog( "HBMp3Enc: opening lame (%d kbps)", audio->outBitrate );

        w->globalFlags = lame_init();
        lame_set_brate( w->globalFlags, audio->outBitrate );

        /* No resampling there - it's been done before */
        lame_set_in_samplerate( w->globalFlags, audio->outSampleRate );
        lame_set_out_samplerate( w->globalFlags, audio->outSampleRate );

        if( lame_init_params( w->globalFlags ) == -1 )
        {
            HBLog( "HBMp3Enc: lame_init_params() failed" );
            HBErrorOccured( w->handle, HB_ERROR_MP3_INIT );
            return 0;
        }
    }

    if( HBFifoIsHalfFull( audio->outFifo ) )
    {
        return 0;
    }

    if( !HBFifoGetBytes( audio->resampleFifo, (uint8_t*) samples_f,
                         1152 * 2 * sizeof( float ), &position ) )
    {
        return 0;
    }

    /* float -> s16 */
    for( i = 0; i < 1152 * 2; i++ )
    {
        samples_s16[i] = samples_f[i];
    }

    mp3Buffer = HBBufferInit( LAME_MAXMP3BUFFER );
    ret       = lame_encode_buffer_interleaved( w->globalFlags,
        samples_s16, 1152, mp3Buffer->data, LAME_MAXMP3BUFFER );

    if( ret < 0 )
    {
        /* Error */
        HBLog( "HBMp3Enc: lame_encode_buffer_float() failed (%d)",
               ret );
        HBErrorOccured( w->handle, HB_ERROR_MP3_ENCODE );
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
        mp3Buffer->position = position;

        if( !HBFifoPush( audio->outFifo, &mp3Buffer ) )
        {
            HBLog( "HBMp3Enc: HBFifoPush failed" );
        }
    }

    return 1;
}

