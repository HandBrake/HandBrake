/* $Id: Resample.c,v 1.4 2004/05/02 16:25:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "HBInternal.h"

#include "samplerate.h"

struct HBWork
{
    HB_WORK_COMMON_MEMBERS

    HBHandle  * handle;
    HBAudio   * audio;

    float     * samples;
    SRC_STATE * state;
    SRC_DATA    data;

    uint64_t    in;
    uint64_t    out;
};

/* Local prototypes */
static int ResampleWork( HBWork * );

HBWork * HBResampleInit( HBHandle * handle, HBAudio * audio )
{
    HBWork * w;
    if( !( w = calloc( sizeof( HBWork ), 1 ) ) )
    {
        HBLog( "HBResampleInit: malloc() failed, gonna crash" );
        return NULL;
    }

    w->name = strdup( "Resample" );
    w->work = ResampleWork;

    w->handle = handle;
    w->audio  = audio;

    return w;
}

void HBResampleClose( HBWork ** _w )
{
    HBWork * w = *_w;

    if( w->samples ) free( w->samples );
    if( w->state )   src_delete( w->state );

    free( w->name );
    free( w );
    *_w = NULL;
}

static int ResampleWork( HBWork * w )
{
    HBAudio  * audio = w->audio;

    HBBuffer * resampleBuffer;
    float      position;

    if( HBFifoIsHalfFull( audio->resampleFifo ) )
    {
        return 0;
    }

    /* Initialization */
    if( !w->samples )
    {
        int error;

        /* Until a first packet comes, audio->inSampleRate is
           undefined */
        if( !HBFifoSize( audio->rawFifo ) )
        {
            return 0;
        }

        /* No, the user can't choose. 44100 Hz, take it or leave it */
        audio->outSampleRate = 44100;
        HBLog( "HBResample: in = %d Hz, out = %d Hz",
                audio->inSampleRate, audio->outSampleRate );

        /* Buffer in which we'll pop the samples from the decoder */
        w->samples = malloc( audio->inSampleRate * 2 *
                             sizeof( float ) / 10 );

        /* Init libsamplerate */
        w->state = src_new( SRC_SINC_FASTEST, 2, &error );

        /* Prepare the SRC_DATA structure */
        w->data.data_in       = w->samples;
        w->data.input_frames  = audio->inSampleRate / 10;
        w->data.output_frames = audio->outSampleRate / 10;
        w->data.src_ratio     = (double) audio->outSampleRate /
                                (double) audio->inSampleRate;
        w->data.end_of_input  = 0;
    }

    /* Fix A/V synchro in case the audio track starts later than the
       video */
    if( audio->delay > 0 )
    {
        HBLog( "HBResample: adding %d ms of silence", audio->delay );

        resampleBuffer = HBBufferInit( audio->delay *
                audio->outSampleRate * 2 * sizeof( float ) / 1000 );
        memset( resampleBuffer->data, 0, resampleBuffer->size );
        if( !HBFifoPush( audio->resampleFifo, &resampleBuffer ) )
        {
            HBLog( "HBResample: HBFifoPush failed" );
        }

        audio->delay = 0;
        return 1;
    }

    /* Get samples from the decoder */
    if( !HBFifoGetBytes( audio->rawFifo, (uint8_t *) w->samples,
                         audio->inSampleRate * 2 * sizeof( float ) / 10,
                         &position ) )
    {
        return 0;
    }

    /* Init resampled buffer */
    resampleBuffer = HBBufferInit( audio->outSampleRate * 2 *
                                   sizeof( float ) / 10 );
    resampleBuffer->position = position;

    /* Resample */
    w->data.data_out = resampleBuffer->dataf;
    if( src_process( w->state, &w->data ) )
    {
        HBLog( "HBResample: src_process failed" );
    }
    resampleBuffer->size = w->data.output_frames_gen * 2 *
                           sizeof( float );

    if( w->data.input_frames_used != w->data.input_frames )
    {
        /* Here we're basically f*cked */
        HBLog( "HBResample: ohoh, %d/%d used",
               w->data.input_frames_used, w->data.input_frames );
    }

    /* Send resampled data to the encoder */
    if( !HBFifoPush( audio->resampleFifo, &resampleBuffer ) )
    {
        HBLog( "HBResample: HBFifoPush failed" );
    }

    return 1;
}

