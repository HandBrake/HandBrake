/* $Id: encx264.c,v 1.21 2005/11/04 13:09:41 titer Exp $

This file is part of the HandBrake source code.
Homepage: <http://handbrake.m0k.org/>.
It may be used under the terms of the GNU General Public License. */

#include <stdarg.h>

#include "hb.h"

#include "x264.h"

int  encx264Init( hb_work_object_t *, hb_job_t * );
int  encx264Work( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encx264Close( hb_work_object_t * );

hb_work_object_t hb_encx264 =
{
    WORK_ENCX264,
    "H.264/AVC encoder (libx264)",
    encx264Init,
    encx264Work,
    encx264Close
};

struct hb_work_private_s
{
    hb_job_t       * job;
    x264_t         * x264;
    x264_picture_t   pic_in;
    x264_picture_t   pic_out;
	
    char             filename[1024];
};

/***********************************************************************
* hb_work_encx264_init
***********************************************************************
*
**********************************************************************/
int encx264Init( hb_work_object_t * w, hb_job_t * job )
{
    x264_param_t       param;
    x264_nal_t       * nal;
    int                nal_count;
    int i, size;
	
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;
	
    pv->job = job;
	
    memset( pv->filename, 0, 1024 );
    hb_get_tempory_filename( job->h, pv->filename, "x264.log" );
	
    x264_param_default( &param );
	
    param.i_threads    = hb_get_cpu_count();
    param.i_width      = job->width;
    param.i_height     = job->height;
    param.i_fps_num    = job->vrate;
    param.i_fps_den    = job->vrate_base;
    param.i_keyint_max = 20 * job->vrate / job->vrate_base;
    param.i_log_level  = X264_LOG_NONE;
	
    if( job->h264_level )
    {
	param.i_threads   = 1;
	param.b_cabac     = 0;
	param.i_level_idc = job->h264_level;
	hb_log( "encx264: encoding at level %i",
		param.i_level_idc );
    }
	
    /* Slightly faster with minimal quality lost */
    param.analyse.i_subpel_refine = 4;
	
    if( job->vquality >= 0.0 && job->vquality <= 1.0 )
    {
        switch(job->crf)
		{
			case 1:
			/*Constant RF*/
			param.rc.i_rc_method = X264_RC_CRF;
			param.rc.f_rf_constant = 51 - job->vquality * 51;
			hb_log( "encx264: Encoding at constant RF %f", 					param.rc.f_rf_constant );
			break;
		
			case 0:
			/*Constant QP*/
			param.rc.i_rc_method = X264_RC_CQP;
        	param.rc.i_qp_constant = 51 - job->vquality * 51;
        	hb_log( "encx264: encoding at constant QP %d",
                param.rc.i_qp_constant );
			break;
		}
    }
    else
    {
	/* Rate control */
        param.rc.i_rc_method = X264_RC_ABR;
        param.rc.i_bitrate = job->vbitrate;
        switch( job->pass )
        {
            case 1:
                param.rc.b_stat_write  = 1;
                param.rc.psz_stat_out = pv->filename;
                break;
            case 2:
                param.rc.b_stat_read = 1;
                param.rc.psz_stat_in = pv->filename;
                break;
        }
    }
	
    hb_log( "encx264: opening libx264 (pass %d)", job->pass );
    pv->x264 = x264_encoder_open( &param );
	
    w->config->mpeg4.length = 0;
	
    x264_encoder_headers( pv->x264, &nal, &nal_count );
	
    for( i = 0; i < nal_count; i++ )
    {
        size = sizeof( w->config->mpeg4.bytes ) - w->config->mpeg4.length;
        x264_nal_encode( &w->config->mpeg4.bytes[w->config->mpeg4.length],
                         &size, 1, &nal[i] );
        w->config->mpeg4.length += size;
    }
	
    x264_picture_alloc( &pv->pic_in, X264_CSP_I420,
                        job->width, job->height );
	
    return 0;
}

void encx264Close( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    x264_encoder_close( pv->x264 );
	
    /* TODO */
}

int encx264Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
				 hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t    * job = pv->job;
    hb_buffer_t * in = *buf_in, * buf;
    int           i_nal;
    x264_nal_t  * nal;
    int i;
	
    /* XXX avoid this memcpy ? */
    memcpy( pv->pic_in.img.plane[0], in->data, job->width * job->height );
    if( job->grayscale )
    {
        /* XXX x264 has currently no option for grayscale encoding */
        memset( pv->pic_in.img.plane[1], 0x80, job->width * job->height / 4 );
        memset( pv->pic_in.img.plane[2], 0x80, job->width * job->height / 4 );
    }
    else
    {
        memcpy( pv->pic_in.img.plane[1], in->data + job->width * job->height,
                job->width * job->height / 4 );
        memcpy( pv->pic_in.img.plane[2], in->data + 5 * job->width *
                job->height / 4, job->width * job->height / 4 );
    }
	
    pv->pic_in.i_type    = X264_TYPE_AUTO;
    pv->pic_in.i_qpplus1 = 0;
	
    x264_encoder_encode( pv->x264, &nal, &i_nal,
                         &pv->pic_in, &pv->pic_out );
	
	
	
    /* Should be way too large */
    buf        = hb_buffer_init( 3 * job->width * job->height / 2 );
    buf->start = in->start;
    buf->stop  = in->stop;
    buf->key   = ( pv->pic_out.i_type == X264_TYPE_IDR );
	
	
    buf->size  = 0;
    for( i = 0; i < i_nal; i++ )
    {
        int size, data;
        data = buf->alloc - buf->size;
        if( ( size = x264_nal_encode( &buf->data[buf->size], &data,
                                      1, &nal[i] ) ) > 0 )
        {
            buf->size += size;
        }
    }
	
    *buf_out = buf;
	
    return HB_WORK_OK;
}


