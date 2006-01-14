/* $Id: encavcodec.c,v 1.23 2005/10/13 23:47:06 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "ffmpeg/avcodec.h"

struct hb_work_object_s
{
    HB_WORK_COMMON;

    hb_job_t * job;
    AVCodecContext * context;
    FILE * file;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void Close( hb_work_object_t ** _w );
static int  Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                  hb_buffer_t ** buf_out );

/***********************************************************************
 * hb_work_encavcodec_init
 ***********************************************************************
 *
 **********************************************************************/
hb_work_object_t * hb_work_encavcodec_init( hb_job_t * job )
{
    AVCodec * codec;
    AVCodecContext * context;
    
    hb_work_object_t * w = calloc( sizeof( hb_work_object_t ), 1 );
    w->name  = strdup( "MPEG-4 encoder (libavcodec)" );
    w->work  = Work;
    w->close = Close;

    w->job = job;

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

    if( job->mux & HB_MUX_MP4 )
    {
        context->flags |= CODEC_FLAG_GLOBAL_HEADER;
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
            w->file = fopen( filename, "wb" );
            context->flags |= CODEC_FLAG_PASS1;
        }
        else
        {
            int    size;
            char * log;

            w->file = fopen( filename, "rb" );
            fseek( w->file, 0, SEEK_END );
            size = ftell( w->file );
            fseek( w->file, 0, SEEK_SET );
            log = malloc( size + 1 );
            log[size] = '\0';
            fread( log, size, 1, w->file );
            fclose( w->file );
            w->file = NULL;

            context->flags    |= CODEC_FLAG_PASS2;
            context->stats_in  = log;
        }
    }

    if( avcodec_open( context, codec ) )
    {
        hb_log( "hb_work_encavcodec_init: avcodec_open failed" );
    }
    w->context = context;

    if( ( job->mux & HB_MUX_MP4 ) && job->pass != 1 )
    {
#define c job->config.mpeg4
        /* Hem hem */
        c.config        = malloc( 15 );
        c.config_length = 15;
        memcpy( c.config, context->extradata + 15, 15 );
#undef c
    }
    
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
    hb_job_t * job = w->job;

    if( w->context )
    {
        hb_log( "encavcodec: closing libavcodec" );
        avcodec_close( w->context );
    }
    if( w->file )
    {
        fclose( w->file );
    }
    if( job->es_config )
    {
        free( job->es_config );
        job->es_config = NULL;
        job->es_config_length = 0;
    }

    free( w->name );
    free( w );
    *_w = NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
static int Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_job_t * job = w->job;
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
    buf->size = avcodec_encode_video( w->context, buf->data, buf->alloc,
                                      frame );
    buf->start = in->start;
    buf->stop  = in->stop;
    buf->key   = w->context->coded_frame->key_frame;

    av_free( frame );

    if( job->pass == 1 )
    {
        /* Write stats */
        fprintf( w->file, "%s", w->context->stats_out );
    }

    *buf_out = buf;

    return HB_WORK_OK;
}


