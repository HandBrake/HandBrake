/* $Id: encvorbis.c,v 1.6 2005/03/05 15:08:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "vorbis/vorbisenc.h"

#define OGGVORBIS_FRAME_SIZE 1024

struct hb_work_object_s
{
    HB_WORK_COMMON;

    hb_job_t   * job;
    hb_audio_t * audio;

    vorbis_info        vi;
    vorbis_comment     vc;
    vorbis_dsp_state   vd;
    vorbis_block       vb;

    unsigned long   input_samples;
    uint8_t       * buf;
    uint64_t        pts;

    hb_list_t     * list;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void Close( hb_work_object_t ** _w );
static int  Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                  hb_buffer_t ** buf_out );

/***********************************************************************
 * hb_work_encvorbis_init
 ***********************************************************************
 *
 **********************************************************************/
hb_work_object_t * hb_work_encvorbis_init( hb_job_t * job, hb_audio_t * audio )
{
    int i;
    ogg_packet header[3];

    hb_work_object_t * w = calloc( sizeof( hb_work_object_t ), 1 );
    w->name  = strdup( "Vorbis encoder (libvorbis)" );
    w->work  = Work;
    w->close = Close;

    w->job   = job;
    w->audio = audio;

    hb_log( "encvorbis: opening libvorbis" );

    /* init */
    vorbis_info_init( &w->vi );
    if( vorbis_encode_setup_managed( &w->vi, 2,
          job->arate, -1, 1000 * job->abitrate, -1 ) ||
        vorbis_encode_ctl( &w->vi, OV_ECTL_RATEMANAGE_AVG, NULL ) ||
          vorbis_encode_setup_init( &w->vi ) )
    {
        hb_log( "encvorbis: vorbis_encode_setup_managed failed" );
    }

    /* add a comment */
    vorbis_comment_init( &w->vc );
    vorbis_comment_add_tag( &w->vc, "Encoder", "HandBrake");

    /* set up the analysis state and auxiliary encoding storage */
    vorbis_analysis_init( &w->vd, &w->vi);
    vorbis_block_init( &w->vd, &w->vb);

    /* get the 3 headers */
    vorbis_analysis_headerout( &w->vd, &w->vc,
                               &header[0], &header[1], &header[2] );
    for( i = 0; i < 3; i++ )
    {
        audio->config.vorbis.headers[i] =
            malloc( sizeof( ogg_packet ) + header[i].bytes );
        memcpy( audio->config.vorbis.headers[i], &header[i],
                sizeof( ogg_packet ) );
        memcpy( audio->config.vorbis.headers[i] + sizeof( ogg_packet ),
                header[i].packet, header[i].bytes );
    }

    w->input_samples = 2 * OGGVORBIS_FRAME_SIZE;
    w->buf = malloc( w->input_samples * sizeof( float ) );

    w->list = hb_list_init();

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
 * Flush
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * Flush( hb_work_object_t * w )
{
    hb_buffer_t * buf;

    if( vorbis_analysis_blockout( &w->vd, &w->vb ) == 1 )
    {
        ogg_packet op;

        vorbis_analysis( &w->vb, NULL );
        vorbis_bitrate_addblock( &w->vb );

        if( vorbis_bitrate_flushpacket( &w->vd, &op ) )
        {
            buf = hb_buffer_init( sizeof( ogg_packet ) + op.bytes );
            memcpy( buf->data, &op, sizeof( ogg_packet ) );
            memcpy( buf->data + sizeof( ogg_packet ), op.packet,
                    op.bytes );
            buf->key   = 1;
            buf->start = w->pts; /* No exact, but who cares - the OGM
                                    muxer doesn't use it */
            buf->stop  = buf->start +
                90000 * OGGVORBIS_FRAME_SIZE + w->job->arate;

            return buf;
        }
    }

    return NULL;
}

/***********************************************************************
 * Encode
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * Encode( hb_work_object_t * w )
{
    hb_buffer_t * buf;
    float ** buffer;
    int i;

    /* Try to extract more data */
    if( ( buf = Flush( w ) ) )
    {
        return buf;
    }

    if( hb_list_bytes( w->list ) < w->input_samples * sizeof( float ) )
    {
        return NULL;
    }

    /* Process more samples */
    hb_list_getbytes( w->list, w->buf, w->input_samples * sizeof( float ),
                      &w->pts, NULL );
    buffer = vorbis_analysis_buffer( &w->vd, OGGVORBIS_FRAME_SIZE );
    for( i = 0; i < OGGVORBIS_FRAME_SIZE; i++ )
    {
        buffer[0][i] = ((float *) w->buf)[2*i]   / 32768.f;
        buffer[1][i] = ((float *) w->buf)[2*i+1] / 32768.f;
    }
    vorbis_analysis_wrote( &w->vd, OGGVORBIS_FRAME_SIZE );

    /* Try to extract again */
    return Flush( w );
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
