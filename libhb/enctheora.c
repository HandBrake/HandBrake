/* This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "theora/codec.h"
#include "theora/theoraenc.h"

int  enctheoraInit( hb_work_object_t *, hb_job_t * );
int  enctheoraWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void enctheoraClose( hb_work_object_t * );

hb_work_object_t hb_enctheora =
{
    WORK_ENCTHEORA,
    "Theora encoder (libtheora)",
    enctheoraInit,
    enctheoraWork,
    enctheoraClose
};

struct hb_work_private_s
{
    hb_job_t * job;

    th_enc_ctx * ctx;
};

int enctheoraInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job = job;

    th_info ti;
    th_comment tc;
    ogg_packet op;
    th_info_init( &ti );

    /* Frame width and height need to be multiples of 16 */
    ti.pic_width = job->width;
    ti.pic_height = job->height;
    ti.frame_width = (job->width + 0xf) & ~0xf;
    ti.frame_height = (job->height + 0xf) & ~0xf;
    ti.pic_x = ti.pic_y = 0;

    ti.fps_numerator = job->vrate;
    ti.fps_denominator = job->vrate_base;
    if( job->anamorphic.mode )
    {
        ti.aspect_numerator = job->anamorphic.par_width;
        ti.aspect_denominator = job->anamorphic.par_height;
    }
    else
    {
        ti.aspect_numerator = ti.aspect_denominator = 1;
    }
    ti.colorspace = TH_CS_UNSPECIFIED;
    ti.pixel_fmt = TH_PF_420;
    if (job->vquality < 0.0)
    {
        ti.target_bitrate = job->vbitrate * 1000;
        ti.quality = 0;
    }
    else
    {
        ti.target_bitrate = 0;
        
        if( job->vquality > 0 && job->vquality < 1 )
        {
            ti.quality = 63 * job->vquality;            
        }
        else
        {
            ti.quality = job->vquality;
        }
    }

    pv->ctx = th_encode_alloc( &ti );

    th_comment_init( &tc );

    th_encode_flushheader( pv->ctx, &tc, &op );
    memcpy(w->config->theora.headers[0], &op, sizeof(op));
    memcpy(w->config->theora.headers[0] + sizeof(op), op.packet, op.bytes );

    th_encode_flushheader( pv->ctx, &tc, &op );
    memcpy(w->config->theora.headers[1], &op, sizeof(op));
    memcpy(w->config->theora.headers[1] + sizeof(op), op.packet, op.bytes );

    th_encode_flushheader( pv->ctx, &tc, &op );
    memcpy(w->config->theora.headers[2], &op, sizeof(op));
    memcpy(w->config->theora.headers[2] + sizeof(op), op.packet, op.bytes );

    th_comment_clear( &tc );

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void enctheoraClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    th_encode_free( pv->ctx );

    free( pv );
    w->private_data = NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int enctheoraWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t * job = pv->job;
    hb_buffer_t * in = *buf_in, * buf;
    th_ycbcr_buffer ycbcr;
    ogg_packet op;

    int frame_width, frame_height;

    if ( in->size <= 0 )
    {
        // EOF on input - send it downstream & say we're done.
        // XXX may need to flush packets via a call to
        //  th_encode_packetout( pv->ctx, 1, &op );
        // but we don't have a timestamp to put on those packets so we
        // drop them for now.
        *buf_out = in;
        *buf_in = NULL;
       return HB_WORK_DONE;
    }

    memset(&op, 0, sizeof(op));
    memset(&ycbcr, 0, sizeof(ycbcr));

    frame_width = (job->width + 0xf) & ~0xf;
    frame_height = (job->height + 0xf) & ~0xf;

    // Y
    ycbcr[0].width = frame_width;
    ycbcr[0].height = frame_height;
    ycbcr[0].stride = job->width;

    // CbCr decimated by factor of 2 in both width and height
    ycbcr[1].width  = ycbcr[2].width  = (frame_width + 1) / 2;
    ycbcr[1].height = ycbcr[2].height = (frame_height + 1) / 2;
    ycbcr[1].stride = ycbcr[2].stride = (job->width + 1) / 2;

    ycbcr[0].data = in->data;
    ycbcr[1].data = ycbcr[0].data + (ycbcr[0].stride * job->height);
    ycbcr[2].data = ycbcr[1].data + (ycbcr[1].stride * ((job->height+1)/2));

    th_encode_ycbcr_in( pv->ctx, &ycbcr );

    th_encode_packetout( pv->ctx, 0, &op );

    buf = hb_buffer_init( op.bytes + sizeof(op) );
    memcpy(buf->data, &op, sizeof(op));
    memcpy(buf->data + sizeof(op), op.packet, op.bytes);
    buf->frametype = ( th_packet_iskeyframe(&op) ) ? HB_FRAME_KEY : HB_FRAME_REF;
    buf->start = in->start;
    buf->stop  = in->stop;

    *buf_out = buf;

    return HB_WORK_OK;
}

