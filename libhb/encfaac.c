/* $Id: encfaac.c,v 1.13 2005/03/03 17:21:57 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "faac.h"

struct hb_work_object_s
{
    HB_WORK_COMMON;

    hb_job_t   * job;
    hb_audio_t * audio;

    faacEncHandle * faac;
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
 * hb_work_encfaac_init
 ***********************************************************************
 *
 **********************************************************************/
hb_work_object_t * hb_work_encfaac_init( hb_job_t * job, hb_audio_t * audio )
{
    hb_work_object_t * w = calloc( sizeof( hb_work_object_t ), 1 );
    faacEncConfigurationPtr cfg;
    w->name  = strdup( "AAC encoder (libfaac)" );
    w->work  = Work;
    w->close = Close;

    w->job   = job;
    w->audio = audio;

    w->faac = faacEncOpen( job->arate, 2, &w->input_samples,
                           &w->output_bytes );
    w->buf  = malloc( w->input_samples * sizeof( float ) );
    
    cfg                = faacEncGetCurrentConfiguration( w->faac );
    cfg->mpegVersion   = MPEG4;
    cfg->aacObjectType = LOW;
    cfg->allowMidside  = 1;
    cfg->useLfe        = 0;
    cfg->useTns        = 0;
    cfg->bitRate       = job->abitrate * 500; /* Per channel */
    cfg->bandWidth     = 0;
    cfg->outputFormat  = 0;
    cfg->inputFormat   =  FAAC_INPUT_FLOAT;
    if( !faacEncSetConfiguration( w->faac, cfg ) )
    {
        hb_log( "faacEncSetConfiguration failed" );
    }
    if( faacEncGetDecoderSpecificInfo( w->faac, &audio->config.faac.decinfo,
                                       &audio->config.faac.size ) < 0 )
    {
        hb_log( "faacEncGetDecoderSpecificInfo failed" );
    }

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

    faacEncClose( w->faac );
    free( w->buf );
    hb_list_empty( &w->list );
    free( w->audio->config.faac.decinfo );

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
    uint64_t      pts;
    int           pos;

    if( hb_list_bytes( w->list ) < w->input_samples * sizeof( float ) )
    {
        /* Need more data */
        return NULL;
    }

    hb_list_getbytes( w->list, w->buf, w->input_samples * sizeof( float ),
                      &pts, &pos );

    buf        = hb_buffer_init( w->output_bytes );
    buf->start = pts + 90000 * pos / 2 / sizeof( float ) / w->job->arate;
    buf->stop  = buf->start + 90000 * w->input_samples / w->job->arate / 2;
    buf->size  = faacEncEncode( w->faac, (int32_t *) w->buf,
            w->input_samples, buf->data, w->output_bytes );
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
        hb_log( "faacEncEncode failed" );
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

