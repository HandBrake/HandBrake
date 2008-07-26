/* $Id: encxvid.c,v 1.10 2005/03/09 23:28:39 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "xvid.h"

int  encxvidInit( hb_work_object_t *, hb_job_t * );
int  encxvidWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encxvidClose( hb_work_object_t * );

hb_work_object_t hb_encxvid =
{
    WORK_ENCXVID,
    "MPEG-4 encoder (libxvidcore)",
    encxvidInit,
    encxvidWork,
    encxvidClose
};

struct hb_work_private_s
{
    hb_job_t * job;
    void     * xvid;
    char       filename[1024];
    int        quant;
    int        configDone;
};

int encxvidInit( hb_work_object_t * w, hb_job_t * job )
{
    xvid_gbl_init_t xvid_gbl_init;
    xvid_enc_create_t create;
    xvid_plugin_single_t single;
    xvid_plugin_2pass1_t rc2pass1;
    xvid_plugin_2pass2_t rc2pass2;
    xvid_enc_plugin_t plugins[1];

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job = job;

    memset( pv->filename, 0, 1024 );
    hb_get_tempory_filename( job->h, pv->filename, "xvid.log" );

    memset( &xvid_gbl_init, 0, sizeof( xvid_gbl_init ) );
    xvid_gbl_init.version = XVID_VERSION;
    xvid_global( NULL, XVID_GBL_INIT, &xvid_gbl_init, NULL );

    memset( &create, 0, sizeof( create ) );
    create.version   = XVID_VERSION;
    create.width     = job->width;
    create.height    = job->height;
    create.zones     = NULL;
    create.num_zones = 0;

    switch( job->pass )
    {
        case 0:
            memset( &single, 0, sizeof( single ) );
            single.version   = XVID_VERSION;
            if( job->vquality < 0.0 || job->vquality > 1.0 )
            {
                /* Rate control */
                single.bitrate = 1000 * job->vbitrate;
                pv->quant = 0;
            }
            else
            {
                /* Constant quantizer */
                pv->quant = 31 - job->vquality * 30;
                hb_log( "encxvid: encoding at constant quantizer %d",
                        pv->quant );
            }
            plugins[0].func  = xvid_plugin_single;
            plugins[0].param = &single;
            break;

        case 1:
            memset( &rc2pass1, 0, sizeof( rc2pass1 ) );
            rc2pass1.version  = XVID_VERSION;
            rc2pass1.filename = pv->filename;
            plugins[0].func   = xvid_plugin_2pass1;
            plugins[0].param  = &rc2pass1;
            break;

        case 2:
            memset( &rc2pass2, 0, sizeof( rc2pass2 ) );
            rc2pass2.version  = XVID_VERSION;
            rc2pass2.filename = pv->filename;
            rc2pass2.bitrate  = 1000 * job->vbitrate;
            plugins[0].func   = xvid_plugin_2pass2;
            plugins[0].param  = &rc2pass2;
            break;
    }

    create.plugins     = plugins;
    create.num_plugins = 1;

    create.num_threads      = 0;
    create.fincr            = job->vrate_base;
    create.fbase            = job->vrate;
    create.max_key_interval = 10 * job->vrate / job->vrate_base;
    create.max_bframes      = 0;
    create.bquant_ratio     = 150;
    create.bquant_offset    = 100;
    create.frame_drop_ratio = 0;
    create.global           = 0;

    xvid_encore( NULL, XVID_ENC_CREATE, &create, NULL );
    pv->xvid = create.handle;

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void encxvidClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if( pv->xvid )
    {
        hb_log( "encxvid: closing libxvidcore" );
        xvid_encore( pv->xvid, XVID_ENC_DESTROY, NULL, NULL);
    }

    free( pv );
    w->private_data = NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int encxvidWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t * job = pv->job;
    xvid_enc_frame_t frame;
    hb_buffer_t * in = *buf_in, * buf;

    if ( in->size <= 0 )
    {
        /* EOF on input - send it downstream & say we're done */
        *buf_out = in;
        *buf_in = NULL;
       return HB_WORK_DONE;
    }

    /* Should be way too large */
    buf = hb_buffer_init( 3 * job->width * job->height / 2 );
    buf->start = in->start;
    buf->stop  = in->stop;

    memset( &frame, 0, sizeof( frame ) );

    frame.version = XVID_VERSION;
    frame.bitstream = buf->data;
    frame.length = -1;
    frame.input.plane[0] = in->data;
    frame.input.csp = XVID_CSP_I420;
    frame.input.stride[0] = job->width;
    frame.vol_flags = 0;
    frame.vop_flags = XVID_VOP_HALFPEL | XVID_VOP_INTER4V |
                      XVID_VOP_TRELLISQUANT | XVID_VOP_HQACPRED;
    if( job->pixel_ratio )
    {
        frame.par = XVID_PAR_EXT;
        frame.par_width = job->pixel_aspect_width;
        frame.par_height = job->pixel_aspect_height;
    }

    if( job->grayscale )
    {
        frame.vop_flags |= XVID_VOP_GREYSCALE;
    }
    frame.type = XVID_TYPE_AUTO;
    frame.quant = pv->quant;
    frame.motion = XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 |
                   XVID_ME_EXTSEARCH16 | XVID_ME_ADVANCEDDIAMOND8 |
                   XVID_ME_HALFPELREFINE8 | XVID_ME_EXTSEARCH8 |
                   XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP;
    frame.quant_intra_matrix = NULL;
    frame.quant_inter_matrix = NULL;

    buf->size = xvid_encore( pv->xvid, XVID_ENC_ENCODE, &frame, NULL );
    buf->frametype = ( frame.out_flags & XVID_KEYFRAME ) ? HB_FRAME_KEY : HB_FRAME_REF;

    if( !pv->configDone )
    {
        int vol_start, vop_start;
        for( vol_start = 0; ; vol_start++ )
        {
            if( buf->data[vol_start]   == 0x0 &&
                buf->data[vol_start+1] == 0x0 &&
                buf->data[vol_start+2] == 0x1 &&
                buf->data[vol_start+3] == 0x20 )
            {
                break;
            }
        }
        for( vop_start = vol_start + 4; ; vop_start++ )
        {
            if( buf->data[vop_start]   == 0x0 &&
                buf->data[vop_start+1] == 0x0 &&
                buf->data[vop_start+2] == 0x1 &&
                buf->data[vop_start+3] == 0xB6 )
            {
                break;
            }
        }

        hb_log( "encxvid: VOL size is %d bytes", vop_start - vol_start );
        job->config.mpeg4.length = vop_start - vol_start;
        memcpy( job->config.mpeg4.bytes, &buf->data[vol_start],
                job->config.mpeg4.length );
        pv->configDone = 1;
    }

    *buf_out = buf;

    return HB_WORK_OK;
}

