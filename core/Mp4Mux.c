/* $Id: Mp4Mux.c,v 1.22 2004/02/18 17:07:20 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* libmp4v2 */
#include "mp4.h"

int64_t videoFrames;
int64_t videoBytes;
int64_t audioFrames;
int64_t audioBytes;

/* Local prototypes */
static void Mp4MuxThread( void * );

struct HBMp4Mux
{
    HBHandle      * handle;
    HBTitle       * title;

    volatile int    die;
    HBThread      * thread;
};

HBMp4Mux * HBMp4MuxInit( HBHandle * handle, HBTitle * title )
{
    HBMp4Mux * m;
    if( !( m = malloc( sizeof( HBMp4Mux ) ) ) )
    {
        HBLog( "HBMp4MuxInit: malloc() failed, gonna crash" );
        return NULL;
    }

    videoFrames = 0;
    videoBytes  = 0;
    audioFrames = 0;
    audioBytes  = 0;

    m->handle   = handle;
    m->title    = title;

    m->die    = 0;
    m->thread = HBThreadInit( "mp4 muxer", Mp4MuxThread, m,
                              HB_NORMAL_PRIORITY );
    return m;
}

void HBMp4MuxClose( HBMp4Mux ** _m )
{
    HBMp4Mux * m = *_m;
    FILE * file;
    long   size;

    m->die = 1;
    HBThreadClose( &m->thread );

    file = fopen( m->title->file, "r" );
    fseek( file, 0, SEEK_END );
    size = ftell( file );
    fclose( file );

    HBLog( "HBMp4Mux: videoFrames=%lld, %lld bytes", videoFrames, videoBytes );
    HBLog( "HBMp4Mux: audioFrames=%lld, %lld bytes", audioFrames, audioBytes );
    HBLog( "HBMp4Mux: overhead=%.2f bytes / frame",
            ( (float) size - videoBytes - audioBytes ) /
            ( videoFrames + audioFrames ) );

    free( m );

    *_m = NULL;
}

static void Mp4MuxThread( void * _m )
{
    HBMp4Mux * m     = (HBMp4Mux*) _m;
    HBTitle  * title = m->title;
    HBAudio  * audio;
    HBBuffer * buffer;
    char       tmpFile[1024];

    int audioCount = HBListCount( m->title->ripAudioList );
    int i;

    MP4FileHandle file;

    /* Wait until we have one encoded frame for each track */
    while( !m->die && !HBFifoSize( title->outFifo ) )
    {
        HBSnooze( 10000 );
    }
    for( i = 0; i < audioCount; i++ )
    {
        audio = HBListItemAt( title->ripAudioList, i );
        while( !m->die && !HBFifoSize( audio->outFifo ) )
        {
            HBSnooze( 10000 );
        }
    }

    if( m->die )
    {
        return;
    }

    /* Write file headers */
    file = MP4Create( title->file, 0, 0 );
    MP4SetTimeScale( file, 90000 );
    title->track = MP4AddVideoTrack( file, 90000,
                                     MP4_INVALID_DURATION,
                                     title->outWidth, title->outHeight,
                                     MP4_MPEG4_VIDEO_TYPE );
    MP4SetVideoProfileLevel( file, 0x03 );
    MP4SetTrackESConfiguration( file, title->track, title->esConfig,
                                title->esConfigLength );

    for( i = 0; i < audioCount; i++ )
    {
        audio = HBListItemAt( title->ripAudioList, i );
        audio->track = MP4AddAudioTrack( file, audio->outSampleRate,
                                         1024, MP4_MPEG4_AUDIO_TYPE );
        MP4SetAudioProfileLevel( file, 0x0F );
        MP4SetTrackESConfiguration( file, audio->track, audio->esConfig,
                                    audio->esConfigLength );
    }

    for( ;; )
    {
        /* Wait until we have one encoded frame for each track */
        if( !HBFifoWait( title->outFifo ) )
        {
            m->die = 1;
            break;
        }
        for( i = 0; i < audioCount; i++ )
        {
            audio = HBListItemAt( title->ripAudioList, i );
            if( !HBFifoWait( audio->outFifo ) )
            {
                m->die = 1;
                break;
            }
        }

        if( m->die )
        {
            break;
        }

        /* Interleave frames in the same order than they were in the
           original MPEG stream */
        audio = NULL;
        for( i = 0; i < audioCount; i++ )
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
            buffer = HBFifoPop( title->outFifo );
            MP4WriteSample( file, title->track, buffer->data,
                            buffer->size,
                            (uint64_t) 90000 * title->rateBase / title->rate,
                            0, buffer->keyFrame );
            videoFrames++;
            videoBytes += buffer->size;
            HBBufferClose( &buffer );
        }
        else
        {
            buffer = HBFifoPop( audio->outFifo );
            MP4WriteSample( file, audio->track, buffer->data,
                            buffer->size, MP4_INVALID_DURATION,
                            0, buffer->keyFrame );
            audioFrames++;
            audioBytes += buffer->size;
            HBBufferClose( &buffer );
        }
    }

    MP4Close( file );

    if( !MP4MakeIsmaCompliant( title->file, 0 /*MP4_DETAILS_ALL*/, 1 ) )
    {
        HBLog( "HBMp4Mux: MP4MakeIsmaCompliant() failed" );
    }

    sprintf( tmpFile, "%s.tmp", title->file );
    tmpFile[strlen( title->file ) + 4] = '\0';
    if( !MP4Optimize( title->file, tmpFile, 0 /*MP4_DETAILS_ALL*/ ) )
    {
        HBLog( "HBMp4Mux: MP4Optimize() failed" );
        unlink( tmpFile );
    }
    else
    {
        rename( tmpFile, title->file );
    }
}

