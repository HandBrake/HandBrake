/* $Id: encx264.c,v 1.21 2005/11/04 13:09:41 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <stdarg.h>

#include "hb.h"

#include "x264.h"

struct hb_work_object_s
{
    HB_WORK_COMMON;

    hb_job_t       * job;
    x264_t         * x264;
    x264_picture_t   pic_in;
    x264_picture_t   pic_out;

    char             filename[1024];
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static int  Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                  hb_buffer_t ** buf_out );
static void Close( hb_work_object_t ** _w );

/***********************************************************************
 * hb_work_encx264_init
 ***********************************************************************
 *
 **********************************************************************/
hb_work_object_t * hb_work_encx264_init( hb_job_t * job )
{
    hb_work_object_t * w;
    x264_param_t       param;
    x264_nal_t       * nal;
    int                nal_count;

    w        = calloc( sizeof( hb_work_object_t ), 1 );
    w->name  = strdup( "AVC encoder (libx264)" );
    w->work  = Work;
    w->close = Close;

    w->job = job;

    memset( w->filename, 0, 1024 );
    hb_get_tempory_filename( job->h, w->filename, "x264.log" );

    x264_param_default( &param );

    param.i_threads    = hb_get_cpu_count();
    param.i_width      = job->width;
    param.i_height     = job->height;
    param.i_fps_num    = job->vrate;
    param.i_fps_den    = job->vrate_base;
    param.i_keyint_max = 20 * job->vrate / job->vrate_base;
    param.i_log_level  = X264_LOG_NONE;
    if( job->h264_13 )
    {
        param.b_cabac     = 0;
        param.i_level_idc = 13;
    }

    /* Slightly faster with minimal quality lost */
    param.analyse.i_subpel_refine = 4;

    if( job->vquality >= 0.0 && job->vquality <= 1.0 )
    {
        /* Constant QP */
        param.rc.i_qp_constant = 51 - job->vquality * 51;
        hb_log( "encx264: encoding at constant QP %d",
                param.rc.i_qp_constant );
    }
    else
    {
        /* Rate control */
        param.rc.b_cbr     = 1;
        param.rc.i_bitrate = job->vbitrate;
        switch( job->pass )
        {
            case 1:
                param.rc.b_stat_write  = 1;
                param.rc.psz_stat_out = w->filename;
                break;
            case 2:
                param.rc.b_stat_read = 1;
                param.rc.psz_stat_in = w->filename;
                break;
        }
    }

    hb_log( "encx264: opening libx264 (pass %d)", job->pass );
    w->x264 = x264_encoder_open( &param );

#define c job->config.h264
    x264_encoder_headers( w->x264, &nal, &nal_count );

    /* Sequence Parameter Set */
    c.sps_length = 1 + nal[1].i_payload;
    c.sps        = malloc( c.sps_length);
    c.sps[0]     = 0x67;
    memcpy( &c.sps[1], nal[1].p_payload, nal[1].i_payload );

    /* Picture Parameter Set */
    c.pps_length = 1 + nal[2].i_payload;
    c.pps        = malloc( c.pps_length );
    c.pps[0]     = 0x68;
    memcpy( &c.pps[1], nal[2].p_payload, nal[2].i_payload );
#undef c

    x264_picture_alloc( &w->pic_in, X264_CSP_I420,
                        job->width, job->height );

    return w;
}

static void Close( hb_work_object_t ** _w )
{
    hb_work_object_t * w = *_w;

    x264_encoder_close( w->x264 );

    /* TODO */

    free( w->name );
    free( w );
    *_w = NULL;
}

static int Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_job_t    * job = w->job;
    hb_buffer_t * in = *buf_in, * buf;
    int           i_nal;
    x264_nal_t  * nal;
    int i;

    /* XXX avoid this memcpy ? */
    memcpy( w->pic_in.img.plane[0], in->data, job->width * job->height );
    if( job->grayscale )
    {
        /* XXX x264 has currently no option for grayscale encoding */
        memset( w->pic_in.img.plane[1], 0x80, job->width * job->height / 4 );
        memset( w->pic_in.img.plane[2], 0x80, job->width * job->height / 4 );
    }
    else
    {
        memcpy( w->pic_in.img.plane[1], in->data + job->width * job->height,
                job->width * job->height / 4 );
        memcpy( w->pic_in.img.plane[2], in->data + 5 * job->width *
                job->height / 4, job->width * job->height / 4 );
    }

    w->pic_in.i_type    = X264_TYPE_AUTO;
    w->pic_in.i_qpplus1 = 0;

    x264_encoder_encode( w->x264, &nal, &i_nal,
                         &w->pic_in, &w->pic_out );

    /* Should be way too large */
    buf        = hb_buffer_init( 3 * job->width * job->height / 2 );
    buf->size  = 0;
    buf->start = in->start;
    buf->stop  = in->stop;
    buf->key   = 0;

    for( i = 0; i < i_nal; i++ )
    {
        int size, data;

        data = buf->alloc - buf->size;
        if( ( size = x264_nal_encode( buf->data + buf->size, &data,
                                      1, &nal[i] ) ) < 1 )
        {
            continue;
        }

        if( job->mux & HB_MUX_AVI )
        {
            if( nal[i].i_ref_idc == NAL_PRIORITY_HIGHEST )
            {
                buf->key = 1;
            }
            buf->size += size;
            continue;
        }

        /* H.264 in .mp4 */
        switch( buf->data[buf->size+4] & 0x1f )
        {
            case 0x7:
            case 0x8:
                /* SPS, PPS */
                break;

            default:
                /* H.264 in mp4 (stolen from mp4creator) */
                buf->data[buf->size+0] = ( ( size - 4 ) >> 24 ) & 0xFF;
                buf->data[buf->size+1] = ( ( size - 4 ) >> 16 ) & 0xFF;
                buf->data[buf->size+2] = ( ( size - 4 ) >>  8 ) & 0xFF;
                buf->data[buf->size+3] = ( ( size - 4 ) >>  0 ) & 0xFF;
                if( nal[i].i_ref_idc == NAL_PRIORITY_HIGHEST )
                {
                    buf->key = 1;
                }
                buf->size += size;
        }
    }

    *buf_out = buf;

    return HB_WORK_OK;
}


