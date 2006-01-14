/* $Id: enclame.c,v 1.9 2005/03/05 14:27:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "lame/lame.h"

struct hb_work_object_s
{
    HB_WORK_COMMON;

    hb_job_t   * job;
    hb_audio_t * audio;

    /* LAME handle */
    lame_global_flags * lame;

    unsigned long   input_samples;
    unsigned long   output_bytes;
    uint8_t       * buf;

    hb_list_t     * list;
    int64_t         pts;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void Close( hb_work_object_t ** _w );
static int  Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                  hb_buffer_t ** buf_out );

/***********************************************************************
 * hb_work_enclame_init
 ***********************************************************************
 *
 **********************************************************************/
hb_work_object_t * hb_work_enclame_init( hb_job_t * job, hb_audio_t * audio )
{
    hb_work_object_t * w = calloc( sizeof( hb_work_object_t ), 1 );
    w->name  = strdup( "MP3 encoder (libmp3lame)" );
    w->work  = Work;
    w->close = Close;

    w->job   = job;
    w->audio = audio;

    hb_log( "enclame: opening libmp3lame" );

    w->lame = lame_init();
    lame_set_brate( w->lame, job->abitrate );
    lame_set_in_samplerate( w->lame, job->arate );
    lame_set_out_samplerate( w->lame, job->arate );
    lame_init_params( w->lame );
    
    w->input_samples = 1152 * 2;
    w->output_bytes = LAME_MAXMP3BUFFER;
    w->buf  = malloc( w->input_samples * sizeof( float ) );

    w->list = hb_list_init();
    w->pts  = -1;

    return w;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
static void Close( hb_work_object_t ** _w )
{
    hb_work_object_t * w = *_w;
    free( w->name );
    free( w );
    *_w = NULL;
}

/***********************************************************************
 * Encode
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * Encode( hb_work_object_t * w )
{
    hb_buffer_t * buf;
    int16_t samples_s16[1152 * 2];
    uint64_t pts;
    int      pos, i;

    if( hb_list_bytes( w->list ) < w->input_samples * sizeof( float ) )
    {
        return NULL;
    }

    hb_list_getbytes( w->list, w->buf, w->input_samples * sizeof( float ),
                      &pts, &pos);

    for( i = 0; i < 1152 * 2; i++ )
    {
        samples_s16[i] = ((float*) w->buf)[i];
    }

    buf        = hb_buffer_init( w->output_bytes );
    buf->start = pts + 90000 * pos / 2 / sizeof( float ) / w->job->arate;
    buf->stop  = buf->start + 90000 * 1152 / w->job->arate;
    buf->size  = lame_encode_buffer_interleaved( w->lame, samples_s16,
            1152, buf->data, LAME_MAXMP3BUFFER );
    buf->key   = 1;

    if( !buf->size )
    {
        /* Encoding was successful but we got no data. Try to encode
           more */
        hb_buffer_close( &buf );
        return Encode( w );
    }
    else if( buf->size < 0 )
    {
        hb_log( "enclame: lame_encode_buffer failed" );
        hb_buffer_close( &buf );
        return NULL;
    }

    return buf;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
static int Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_buffer_t * buf;

    hb_list_add( w->list, *buf_in );
    *buf_in = NULL;

    *buf_out = buf = Encode( w );

    while( buf )
    {
        buf->next = Encode( w );
        buf       = buf->next;
    }

    return HB_WORK_OK;
}

