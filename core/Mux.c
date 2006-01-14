/* $Id: Mux.c,v 1.9 2004/05/25 17:36:40 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <sys/types.h>
#include <sys/stat.h>

#include "HBInternal.h"

HBMux * HBAviMuxInit( HBHandle * handle, HBTitle * title );
void    HBAviMuxClose( HBMux ** );
HBMux * HBMp4MuxInit( HBHandle * handle, HBTitle * title );
void    HBMp4MuxClose( HBMux ** );
HBMux * HBOgmMuxInit( HBHandle * handle, HBTitle * title );
void    HBOgmMuxClose( HBMux ** );

/* Local prototypes */
static void MuxThread( void * t );

struct HBMux
{
    HB_MUX_COMMON_MEMBERS
};

struct HBMuxThread
{
    HBHandle     * handle;
    HBTitle      * title;
    HBMux        * mux;

    uint64_t       videoFrames;
    uint64_t       videoBytes;
    uint64_t       audioFrames;
    uint64_t       audioBytes;

    volatile int   die;
    HBThread     * thread;
};

HBMuxThread * HBMuxThreadInit( HBHandle * handle, HBTitle * title )
{
    HBMuxThread * t;
    if( !( t = calloc( sizeof( HBMuxThread ), 1 ) ) )
    {
        HBLog( "HBMuxThreadInit: malloc() failed, gonna crash" );
        return NULL;
    }
    t->handle = handle;
    t->title  = title;

    /* Init muxer */
    if( title->mux == HB_MUX_AVI )
        t->mux = HBAviMuxInit( handle, title );
    else if( title->mux == HB_MUX_MP4 )
        t->mux = HBMp4MuxInit( handle, title );
    else if( title->mux == HB_MUX_OGM )
        t->mux = HBOgmMuxInit( handle, title );

    /* Launch the thread */
    t->thread = HBThreadInit( "mux thread", MuxThread, t,
                              HB_NORMAL_PRIORITY );

    return t;
}

void HBMuxThreadClose( HBMuxThread ** _t )
{
    HBMuxThread * t     = (*_t);
    HBTitle     * title = t->title;
    struct stat   sb;

    /* Stop the thread */
    t->die = 1;
    HBThreadClose( &t->thread );

    /* Close muxer */
    if( title->mux == HB_MUX_AVI )
        HBAviMuxClose( &t->mux );
    else if( title->mux == HB_MUX_MP4 )
        HBMp4MuxClose( &t->mux );
    else if( title->mux == HB_MUX_OGM )
        HBOgmMuxClose( &t->mux );

    /* Stats */
    if( !stat( title->file, &sb ) )
    {
        uint64_t   overhead;
        HBAudio  * audio;

        overhead = (uint64_t) sb.st_size - t->videoBytes - t->audioBytes;
        HBLog( "HBMuxThread: file size:  "LLD" bytes",
                (uint64_t) sb.st_size );
        HBLog( "HBMuxThread: video data: "LLD" bytes ("LLD" frames)",
               t->videoBytes, t->videoFrames );
        HBLog( "HBMuxThread: audio data: "LLD" bytes ("LLD" frames)",
               t->audioBytes, t->audioFrames );
        HBLog( "HBMuxThread: overhead:   "LLD" bytes (%.2f bytes per "
               "frame)", overhead, (float) overhead / ( t->videoFrames +
               t->audioFrames ) );

        HBLog( "HBMuxThread: video bitrate: %.2f kbps",
               (float) t->videoBytes * title->rate / t->videoFrames /
               title->rateBase / 128 );
        HBLog( "HBMuxThread: video error:   "LLD" bytes", t->videoBytes -
               t->videoFrames * title->bitrate * 128 * title->rateBase /
               title->rate );

        /* FIXME - handle multi-audio encoding */
        audio = (HBAudio *) HBListItemAt( title->ripAudioList, 0 );
        if( audio )
        {
            int samplesPerFrame = ( audio->outCodec == HB_CODEC_MP3 ?
                                       1152 : 1024 );
            HBLog( "HBMuxThread: audio bitrate: %.2f kbps",
                   (float) t->audioBytes * audio->outSampleRate /
                   t->audioFrames / samplesPerFrame / 125 );
            HBLog( "HBMuxThread: audio error:   "LLD" bytes",
                   t->audioBytes - audio->outBitrate * t->audioFrames *
                   125 * samplesPerFrame / audio->outSampleRate );
        }
    }

    free( t );

    *_t = NULL;
}

static int MuxWait( HBTitle * title )
{
    int       i;
    HBAudio * audio;

    if( !HBFifoWait( title->outFifo ) )
    {
        return 0;
    }
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = HBListItemAt( title->ripAudioList, i );
        if( !HBFifoWait( audio->outFifo ) )
        {
            return 0;
        }
    }
    return 1;
}

static void MuxThread( void * _t )
{
    HBMuxThread * t     = (HBMuxThread*) _t;
    HBTitle     * title = t->title;
    HBMux       * m     = t->mux;
    HBAudio     * audio;
    HBBuffer    * buffer;
    int           i;

    /* Remove the file if already existing */
    unlink( title->file );

    /* Wait until we have at least one video frame and 3 audio frames
       for each track (Vorbis...) */
    if( !HBFifoWait( title->outFifo ) )
    {
        return;
    }
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = HBListItemAt( title->ripAudioList, i );
        while( !t->die && HBFifoSize( audio->outFifo ) < 3 )
        {
            HBSnooze( 10000 );
        }
        if( t->die )
        {
            return;
        }
    }

    m->start( m );

    /* Mux */
    for( ;; )
    {
        /* Wait until we have one frame for each track */
        if( !MuxWait( title ) )
        {
            break;
        }

        /* Interleave frames in the same order than they were in the
           original MPEG stream */
        audio = NULL;
        for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
        {
            HBAudio * otherAudio;
            otherAudio = HBListItemAt( title->ripAudioList, i );
            if( !audio || HBFifoPosition( otherAudio->outFifo ) <
                          HBFifoPosition( audio->outFifo ) )
            {
                audio = otherAudio;
            }
        }
        if( !audio || HBFifoPosition( title->outFifo ) <
                HBFifoPosition( audio->outFifo ) )
        {
            /* Video */
            buffer = HBFifoPop( title->outFifo );
            m->muxVideo( m, title->muxData, buffer );
            t->videoBytes += buffer->size;
            t->videoFrames++;
            HBBufferClose( &buffer );
        }
        else
        {
            /* Audio */
            buffer = HBFifoPop( audio->outFifo );
            m->muxAudio( m, audio->muxData, buffer );
            t->audioBytes += buffer->size;
            t->audioFrames++;
            HBBufferClose( &buffer );
        }
    }

    m->end( m );
}

