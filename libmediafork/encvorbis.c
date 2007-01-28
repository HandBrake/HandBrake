/* $Id: encvorbis.c,v 1.6 2005/03/05 15:08:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "mediafork.h"

#include "vorbis/vorbisenc.h"

#define OGGVORBIS_FRAME_SIZE 1024

int  encvorbisInit( hb_work_object_t *, hb_job_t * );
int  encvorbisWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encvorbisClose( hb_work_object_t * );

hb_work_object_t hb_encvorbis =
{
    WORK_ENCVORBIS,
    "Vorbis encoder (libvorbis)",
    encvorbisInit,
    encvorbisWork,
    encvorbisClose
};

struct hb_work_private_s
{
    hb_job_t   * job;

    vorbis_info        vi;
    vorbis_comment     vc;
    vorbis_dsp_state   vd;
    vorbis_block       vb;

    unsigned long   input_samples;
    uint8_t       * buf;
    uint64_t        pts;

    hb_list_t     * list;
};

int encvorbisInit( hb_work_object_t * w, hb_job_t * job )
{
    int i;
    ogg_packet header[3];

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job   = job;

    hb_log( "encvorbis: opening libvorbis" );

    /* init */
    vorbis_info_init( &pv->vi );
    if( vorbis_encode_setup_managed( &pv->vi, 2,
          job->arate, -1, 1000 * job->abitrate, -1 ) ||
        vorbis_encode_ctl( &pv->vi, OV_ECTL_RATEMANAGE_AVG, NULL ) ||
          vorbis_encode_setup_init( &pv->vi ) )
    {
        hb_log( "encvorbis: vorbis_encode_setup_managed failed" );
    }

    /* add a comment */
    vorbis_comment_init( &pv->vc );
    vorbis_comment_add_tag( &pv->vc, "Encoder", "HandBrake");

    /* set up the analysis state and auxiliary encoding storage */
    vorbis_analysis_init( &pv->vd, &pv->vi);
    vorbis_block_init( &pv->vd, &pv->vb);

    /* get the 3 headers */
    vorbis_analysis_headerout( &pv->vd, &pv->vc,
                               &header[0], &header[1], &header[2] );
    for( i = 0; i < 3; i++ )
    {
        memcpy( w->config->vorbis.headers[i], &header[i],
                sizeof( ogg_packet ) );
        memcpy( w->config->vorbis.headers[i] + sizeof( ogg_packet ),
                header[i].packet, header[i].bytes );
    }

    pv->input_samples = 2 * OGGVORBIS_FRAME_SIZE;
    pv->buf = malloc( pv->input_samples * sizeof( float ) );

    pv->list = hb_list_init();

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void encvorbisClose( hb_work_object_t * w )
{
}

/***********************************************************************
 * Flush
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * Flush( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;

    if( vorbis_analysis_blockout( &pv->vd, &pv->vb ) == 1 )
    {
        ogg_packet op;

        vorbis_analysis( &pv->vb, NULL );
        vorbis_bitrate_addblock( &pv->vb );

        if( vorbis_bitrate_flushpacket( &pv->vd, &op ) )
        {
            buf = hb_buffer_init( sizeof( ogg_packet ) + op.bytes );
            memcpy( buf->data, &op, sizeof( ogg_packet ) );
            memcpy( buf->data + sizeof( ogg_packet ), op.packet,
                    op.bytes );
            buf->key   = 1;
            buf->start = pv->pts; /* No exact, but who cares - the OGM
                                    muxer doesn't use it */
            buf->stop  = buf->start +
                90000 * OGGVORBIS_FRAME_SIZE + pv->job->arate;

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
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;
    float ** buffer;
    int i;

    /* Try to extract more data */
    if( ( buf = Flush( w ) ) )
    {
        return buf;
    }

    if( hb_list_bytes( pv->list ) < pv->input_samples * sizeof( float ) )
    {
        return NULL;
    }

    /* Process more samples */
    hb_list_getbytes( pv->list, pv->buf, pv->input_samples * sizeof( float ),
                      &pv->pts, NULL );
    buffer = vorbis_analysis_buffer( &pv->vd, OGGVORBIS_FRAME_SIZE );
    for( i = 0; i < OGGVORBIS_FRAME_SIZE; i++ )
    {
        buffer[0][i] = ((float *) pv->buf)[2*i]   / 32768.f;
        buffer[1][i] = ((float *) pv->buf)[2*i+1] / 32768.f;
    }
    vorbis_analysis_wrote( &pv->vd, OGGVORBIS_FRAME_SIZE );

    /* Try to extract again */
    return Flush( w );
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int encvorbisWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                   hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;

    hb_list_add( pv->list, *buf_in );
    *buf_in = NULL;

    *buf_out = buf = Encode( w );

    while( buf )
    {
        buf->next = Encode( w );
        buf       = buf->next;
    }

    return HB_WORK_OK;
}
