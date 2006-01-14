/* $Id: Mp4Mux.c,v 1.31 2004/05/13 21:10:56 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

/* libmp4v2 */
#include "mp4.h"

struct HBMux
{
    HB_MUX_COMMON_MEMBERS

    HBHandle      * handle;
    HBTitle       * title;

    MP4FileHandle   file;

    /* QuickTime sync workaround */
    int             sampleRate;
    uint64_t        frames;
    uint64_t        date;

};

typedef struct
{
    int      track;

} Mp4MuxData;

/* Local prototypes */
static int Mp4Start( HBMux * );
static int Mp4MuxVideo( HBMux *, void *, HBBuffer *);
static int Mp4MuxAudio( HBMux *, void *, HBBuffer *);
static int Mp4End( HBMux * );

HBMux * HBMp4MuxInit( HBHandle * handle, HBTitle * title )
{
    HBMux   * m;
    HBAudio * audio;
    int       i;

    if( !( m = calloc( sizeof( HBMux ), 1 ) ) )
    {
        HBLog( "HBMp4Mux: malloc() failed, gonna crash" );
        return NULL;
    }
    m->start    = Mp4Start;
    m->muxVideo = Mp4MuxVideo;
    m->muxAudio = Mp4MuxAudio;
    m->end      = Mp4End;

    m->handle   = handle;
    m->title    = title;

    /* Alloc muxer data */
    title->muxData = calloc( sizeof( Mp4MuxData ), 1 );
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = (HBAudio *) HBListItemAt( title->ripAudioList, i );
        audio->muxData = calloc( sizeof( Mp4MuxData ), 1 );
    }

    return m;
}

void HBMp4MuxClose( HBMux ** _m )
{
    HBMux   * m     = *_m;
    HBTitle * title = m->title;
    HBAudio * audio;
    int       i;

    /* Free muxer data */
    free( title->muxData );
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = (HBAudio *) HBListItemAt( title->ripAudioList, i );
        free( audio->muxData );
    }

    free( m );
    *_m = NULL;
}

static int Mp4Start( HBMux * m )
{
    HBTitle    * title = m->title;
    HBAudio    * audio;
    Mp4MuxData * muxData;
    int          i;

    /* Create file */
    m->file = MP4Create( title->file, 0, 0 );

    /* Add video track */
    muxData = (Mp4MuxData *) title->muxData;
    if( HBListCount( title->ripAudioList ) )
    {
        /* QuickTime sync workaround */
        audio = (HBAudio *) HBListItemAt( title->ripAudioList, 0 );
        m->sampleRate = audio->outSampleRate;
        MP4SetTimeScale( m->file, m->sampleRate );
        muxData->track = MP4AddVideoTrack( m->file, m->sampleRate,
                MP4_INVALID_DURATION, title->outWidth, title->outHeight,
                MP4_MPEG4_VIDEO_TYPE );
    }
    else
    {
        MP4SetTimeScale( m->file, 90000 );
        muxData->track = MP4AddVideoTrack( m->file, 90000,
                (uint64_t) 90000 * title->rateBase / title->rate,
                title->outWidth, title->outHeight,
                MP4_MPEG4_VIDEO_TYPE );
    }
    MP4SetVideoProfileLevel( m->file, 0x03 );
    MP4SetTrackESConfiguration( m->file, muxData->track,
            title->esConfig, title->esConfigLength );

    /* Add audio tracks */
    for( i = 0; i < HBListCount( title->ripAudioList ); i++ )
    {
        audio = HBListItemAt( title->ripAudioList, i );
        muxData = (Mp4MuxData *) audio->muxData;
        muxData->track = MP4AddAudioTrack( m->file,
                audio->outSampleRate, 1024, MP4_MPEG4_AUDIO_TYPE );
        MP4SetAudioProfileLevel( m->file, 0x0F );
        MP4SetTrackESConfiguration( m->file, muxData->track,
                audio->esConfig, audio->esConfigLength );
    }

    return 0;
}

static int Mp4MuxVideo( HBMux * m, void * _muxData, HBBuffer * buffer )
{
    Mp4MuxData * muxData = (Mp4MuxData *) _muxData;
    HBTitle    * title   = m->title;

    if( HBListCount( title->ripAudioList ) )
    {
        /* QuickTime sync workaround */
        int dur = (uint64_t) m->sampleRate * ( ++m->frames ) *
            title->rateBase / title->rate - m->date;
        MP4WriteSample( m->file, muxData->track, buffer->data, buffer->size,
                        dur, 0, buffer->keyFrame );
        m->date += dur;
    }
    else
    {
        MP4WriteSample( m->file, muxData->track, buffer->data,
                buffer->size, MP4_INVALID_DURATION, 0,
                buffer->keyFrame );
    }
    return 0;
}

static int Mp4MuxAudio( HBMux * m, void * _muxData, HBBuffer * buffer )
{
    Mp4MuxData * muxData = (Mp4MuxData *) _muxData;

    MP4WriteSample( m->file, muxData->track, buffer->data, buffer->size,
                    MP4_INVALID_DURATION, 0, buffer->keyFrame );
    return 0;
}

static int Mp4End( HBMux * m )
{
    HBTitle  * title = m->title;
    char       tmpFile[1024];

    MP4Close( m->file );

    HBLog( "HBMp4Mux: making the file ISMA compliant" );
    if( !MP4MakeIsmaCompliant( title->file, 0 /*MP4_DETAILS_ALL*/, 1 ) )
    {
        HBLog( "HBMp4Mux: MP4MakeIsmaCompliant() failed" );
    }

    HBLog( "HBMp4Mux: optimizing" );
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
    return 0;
}

