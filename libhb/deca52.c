/* $Id: deca52.c,v 1.14 2005/03/03 17:21:57 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "a52dec/a52.h"

struct hb_work_object_s
{
    HB_WORK_COMMON;

    hb_job_t    * job;
    hb_audio_t  * audio;

    /* liba52 handle */
    a52_state_t * state;

    int           flags_in;
    int           flags_out;
    int           rate;
    int           bitrate;
    float         level;
    
    int           error;
    int           sync;
    int           size;

    uint8_t       frame[3840];

    hb_list_t   * list;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void          Close( hb_work_object_t ** _w );
static int           Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                           hb_buffer_t ** buf_out );
static hb_buffer_t * Decode( hb_work_object_t * w );

/***********************************************************************
 * hb_work_deca52_init
 ***********************************************************************
 * Allocate the work object, initialize liba52
 **********************************************************************/
hb_work_object_t * hb_work_deca52_init( hb_job_t * job, hb_audio_t * audio )
{
    hb_work_object_t * w = calloc( sizeof( hb_work_object_t ), 1 );
    w->name  = strdup( "AC3 decoder" );
    w->work  = Work;
    w->close = Close;

    w->job   = job;
    w->audio = audio;

    w->list      = hb_list_init();
    w->state     = a52_init( 0 );
    w->flags_out = A52_STEREO;
    w->level     = 32768.0;

    return w;
}

/***********************************************************************
 * Close
 ***********************************************************************
 * Free memory
 **********************************************************************/
static void Close( hb_work_object_t ** _w )
{
    hb_work_object_t * w = *_w;
    a52_free( w->state );
    free( w->name );
    free( w );
    *_w = NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 * Add the given buffer to the data we already have, and decode as much
 * as we can
 **********************************************************************/
static int Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_buffer_t * buf;

    hb_list_add( w->list, *buf_in );
    *buf_in = NULL;

    /* If we got more than a frame, chain raw buffers */
    *buf_out = buf = Decode( w );
    while( buf )
    {
        buf->next = Decode( w );
        buf       = buf->next;
    }

    return HB_WORK_OK;
}

/***********************************************************************
 * Decode
 ***********************************************************************
 * 
 **********************************************************************/
static hb_buffer_t * Decode( hb_work_object_t * w )
{
    hb_buffer_t * buf;
    int           i, j;
    uint64_t      pts;
    int           pos;

    /* Get a frame header if don't have one yet */
    if( !w->sync )
    {
        while( hb_list_bytes( w->list ) >= 7 )
        {
            /* We have 7 bytes, check if this is a correct header */
            hb_list_seebytes( w->list, w->frame, 7 );
            w->size = a52_syncinfo( w->frame, &w->flags_in, &w->rate,
                                    &w->bitrate );
            if( w->size )
            {
                /* It is. W00t. */
                if( w->error )
                {
                    hb_log( "a52_syncinfo ok" );
                }
                w->error = 0;
                w->sync  = 1;
                break;
            }

            /* It is not */
            if( !w->error )
            {
                hb_log( "a52_syncinfo failed" );
                w->error = 1;
            }

            /* Try one byte later */
            hb_list_getbytes( w->list, w->frame, 1, NULL, NULL );
        }
    }

    if( !w->sync ||
        hb_list_bytes( w->list ) < w->size )
    {
        /* Need more data */
        return NULL;
    }

    /* Get the whole frame */
    hb_list_getbytes( w->list, w->frame, w->size, &pts, &pos );

    /* AC3 passthrough: don't decode the AC3 frame */
    if( w->job->acodec & HB_ACODEC_AC3 )
    {
        buf = hb_buffer_init( w->size );
        memcpy( buf->data, w->frame, w->size );
        buf->start = pts + ( pos / w->size ) * 6 * 256 * 90000 / w->rate;
        buf->stop  = buf->start + 6 * 256 * 90000 / w->rate;
        w->sync = 0;
        return buf;
    }

    /* Feed liba52 */
    a52_frame( w->state, w->frame, &w->flags_out, &w->level, 0 );

    /* 6 blocks per frame, 256 samples per block, 2 channels */
    buf        = hb_buffer_init( 3072 * sizeof( float ) );
    buf->start = pts + ( pos / w->size ) * 6 * 256 * 90000 / w->rate;
    buf->stop  = buf->start + 6 * 256 * 90000 / w->rate;

    for( i = 0; i < 6; i++ )
    {
        sample_t * samples_in;
        float    * samples_out;

        a52_block( w->state );
        samples_in  = a52_samples( w->state );
        samples_out = ((float *) buf->data) + 512 * i;

        /* Interleave */
        for( j = 0; j < 256; j++ )
        {
            samples_out[2*j]   = samples_in[j];
            samples_out[2*j+1] = samples_in[256+j];
        }
    }

    w->sync = 0;
    return buf;
}

