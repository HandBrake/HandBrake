/* $Id: encavcodec.c,v 1.23 2005/10/13 23:47:06 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "ffmpeg/avcodec.h"

struct hb_work_private_s
{
    hb_job_t * job;
    AVCodecContext * context;
    FILE * file;
};

int  encavcodecInit( hb_work_object_t *, hb_job_t * );
int  encavcodecWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encavcodecClose( hb_work_object_t * );

hb_work_object_t hb_encavcodec =
{   
    WORK_ENCAVCODEC,
    "MPEG-4 encoder (libavcodec)",
    encavcodecInit,
    encavcodecWork,
    encavcodecClose
}; 

int encavcodecInit( hb_work_object_t * w, hb_job_t * job )
{
    AVCodec * codec;
    AVCodecContext * context;
    
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job = job;

    codec = avcodec_find_encoder( CODEC_ID_MPEG4 );
    if( !codec )
    {
        hb_log( "hb_work_encavcodec_init: avcodec_find_encoder "
                "failed" );
    }
    context = avcodec_alloc_context();
    if( job->vquality < 0.0 || job->vquality > 1.0 )
    {
        /* Rate control */
        context->bit_rate = 1000 * job->vbitrate;
        context->bit_rate_tolerance = 10 * context->bit_rate;
    }
    else
    {
        /* Constant quantizer */
        context->qmin = 31 - job->vquality * 30;
        context->qmax = context->qmin;
        hb_log( "encavcodec: encoding at constant quantizer %d",
                context->qmin );
    }
    context->width     = job->width;
    context->height    = job->height;
    context->time_base = (AVRational) { job->vrate_base, job->vrate };
    context->gop_size  = 10 * job->vrate / job->vrate_base;
    context->pix_fmt   = PIX_FMT_YUV420P;

    if( job->pixel_ratio )
    {
        context->sample_aspect_ratio.num = job->pixel_aspect_width;
        context->sample_aspect_ratio.den = job->pixel_aspect_height;

        hb_log( "encavcodec: encoding with stored aspect %d/%d", 
                job->pixel_aspect_width, job->pixel_aspect_height );
    }

    if( job->mux & ( HB_MUX_MP4 | HB_MUX_PSP ) )
    {
        context->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    if( job->mux & HB_MUX_PSP )
    {
        context->flags |= CODEC_FLAG_BITEXACT;
    }
    if( job->grayscale )
    {
        context->flags |= CODEC_FLAG_GRAY;
    }

    if( job->pass )
    {
        char filename[1024]; memset( filename, 0, 1024 );
        hb_get_tempory_filename( job->h, filename, "ffmpeg.log" );

        if( job->pass == 1 )
        {
            pv->file = fopen( filename, "wb" );
            context->flags |= CODEC_FLAG_PASS1;
        }
        else
        {
            int    size;
            char * log;

            pv->file = fopen( filename, "rb" );
            fseek( pv->file, 0, SEEK_END );
            size = ftell( pv->file );
            fseek( pv->file, 0, SEEK_SET );
            log = malloc( size + 1 );
            log[size] = '\0';
            fread( log, size, 1, pv->file );
            fclose( pv->file );
            pv->file = NULL;

            context->flags    |= CODEC_FLAG_PASS2;
            context->stats_in  = log;
        }
    }

    if( avcodec_open( context, codec ) )
    {
        hb_log( "hb_work_encavcodec_init: avcodec_open failed" );
    }
    pv->context = context;

    if( ( job->mux & ( HB_MUX_MP4 | HB_MUX_PSP ) ) && job->pass != 1 )
    {
#if 0
        /* Hem hem */
        w->config->mpeg4.length = 15;
        memcpy( w->config->mpeg4.bytes, context->extradata + 15, 15 );
#else
        w->config->mpeg4.length = context->extradata_size;
        memcpy( w->config->mpeg4.bytes, context->extradata,
                context->extradata_size );
#endif
    }
    
    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void encavcodecClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if( pv->context )
    {
        hb_log( "encavcodec: closing libavcodec" );
        avcodec_flush_buffers( pv->context );
        avcodec_close( pv->context );
    }
    if( pv->file )
    {
        fclose( pv->file );
    }
    free( pv );
    w->private_data = NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int encavcodecWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                    hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t * job = pv->job;
    AVFrame  * frame;
    hb_buffer_t * in = *buf_in, * buf;

    frame              = avcodec_alloc_frame();
    frame->data[0]     = in->data;
    frame->data[1]     = frame->data[0] + job->width * job->height;
    frame->data[2]     = frame->data[1] + job->width * job->height / 4;
    frame->linesize[0] = job->width;
    frame->linesize[1] = job->width / 2;
    frame->linesize[2] = job->width / 2;

    /* Should be way too large */
    buf = hb_buffer_init( 3 * job->width * job->height / 2 );
    buf->size = avcodec_encode_video( pv->context, buf->data, buf->alloc,
                                      frame );
    buf->start = in->start;
    buf->stop  = in->stop;
    buf->key   = pv->context->coded_frame->key_frame;

    av_free( frame );

    if( job->pass == 1 )
    {
        /* Write stats */
        fprintf( pv->file, "%s", pv->context->stats_out );
    }

    *buf_out = buf;

    return HB_WORK_OK;
}


