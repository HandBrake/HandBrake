/* This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "theora/theora.h"

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

    theora_state theora;
};

int enctheoraInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job = job;

    theora_info ti;
    theora_comment tc;
    ogg_packet op;
    theora_info_init( &ti );

    ti.width = ti.frame_width = job->width;
    ti.height = ti.frame_height = job->height;
    ti.offset_x = ti.offset_y = 0;
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
    ti.colorspace = OC_CS_UNSPECIFIED;
    ti.pixelformat = OC_PF_420;
    ti.keyframe_auto_p = 1;
    ti.keyframe_frequency = (job->vrate / job->vrate_base) + 1;
    ti.keyframe_frequency_force = (10 * job->vrate / job->vrate_base) + 1;
    /* From encoder_example.c */
    ti.quick_p = 1;
    ti.dropframes_p = 0;
    ti.keyframe_auto_threshold = 80;
    ti.keyframe_mindistance = 8;
    ti.noise_sensitivity = 1;
    ti.sharpness = 0;
    if (job->vquality < 0.0)
    {
        ti.target_bitrate = job->vbitrate * 1000;
        ti.keyframe_data_target_bitrate = job->vbitrate * 1000 * 1.5;
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

    theora_encode_init( &pv->theora, &ti );
    theora_info_clear( &ti );

    theora_encode_header( &pv->theora, &op );
    memcpy(w->config->theora.headers[0], &op, sizeof(op));
    memcpy(w->config->theora.headers[0] + sizeof(op), op.packet, op.bytes );

    theora_comment_init(&tc);
    theora_encode_comment(&tc,&op);
    memcpy(w->config->theora.headers[1], &op, sizeof(op));
    memcpy(w->config->theora.headers[1] + sizeof(op), op.packet, op.bytes );
    free(op.packet);

    theora_encode_tables(&pv->theora, &op);
    memcpy(w->config->theora.headers[2], &op, sizeof(op));
    memcpy(w->config->theora.headers[2] + sizeof(op), op.packet, op.bytes );

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
    /* TODO: Free alloc'd */

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
    yuv_buffer yuv;
    ogg_packet op;

    if ( in->size <= 0 )
    {
        // EOF on input - send it downstream & say we're done.
        // XXX may need to flush packets via a call to
        //  theora_encode_packetout(&pv->theora, 1, &op);
        // but we don't have a timestamp to put on those packets so we
        // drop them for now.
        *buf_out = in;
        *buf_in = NULL;
       return HB_WORK_DONE;
    }

    memset(&op, 0, sizeof(op));
    memset(&yuv, 0, sizeof(yuv));

    yuv.y_width = job->width;
    yuv.y_height = job->height;
    yuv.y_stride = job->width;

    yuv.uv_width = (job->width + 1) / 2;
    yuv.uv_height = (job->height + 1) / 2;
    yuv.uv_stride = yuv.uv_width;

    yuv.y = in->data;
    yuv.u = in->data + job->width * job->height;
    yuv.v = in->data + ( job->width * job->height ) + ( yuv.uv_width * yuv.uv_height );

    theora_encode_YUVin(&pv->theora, &yuv);

    theora_encode_packetout(&pv->theora, 0, &op);

    buf = hb_buffer_init( op.bytes + sizeof(op) );
    memcpy(buf->data, &op, sizeof(op));
    memcpy(buf->data + sizeof(op), op.packet, op.bytes);
    buf->frametype = ( theora_packet_iskeyframe(&op) ) ? HB_FRAME_KEY : HB_FRAME_REF;
    buf->start = in->start;
    buf->stop  = in->stop;

    *buf_out = buf;

    return HB_WORK_OK;
}

