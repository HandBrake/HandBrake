/* $Id: render.c,v 1.17 2005/04/14 17:37:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "ffmpeg/avcodec.h"

struct hb_work_object_s
{
    HB_WORK_COMMON;

    hb_job_t * job;

    ImgReSampleContext * context;
    AVPicture            pic_raw;
    AVPicture            pic_deint;
    AVPicture            pic_render;
    hb_buffer_t        * buf_deint;
};

static void ApplySub( hb_job_t * job, hb_buffer_t * buf,
                      hb_buffer_t ** _sub )
{
    hb_buffer_t * sub = *_sub;
    hb_title_t * title = job->title;
    int i, j, offset_top, offset_left;
    uint8_t * lum, * alpha, * out;

    if( !sub )
    {
        return;
    }

    if( sub->width > title->width - job->crop[0] - job->crop[1] - 40 ||
        sub->height > title->height - job->crop[2] - job->crop[3] - 40 )
    {
        /* The subtitle won't fit */
        hb_buffer_close( _sub );
        return;
    }

    /* If necessary, move the subtitle so it is 20 pixels far from each
       border of the cropped picture */
    offset_top = sub->y;
    offset_top = MAX( offset_top, job->crop[0] + 20 );
    offset_top = MIN( offset_top,
            title->height - job->crop[1] - 20 - sub->height );
    offset_left = sub->x;
    offset_left = MAX( offset_left, job->crop[2] + 20 );
    offset_left = MIN( offset_left,
            title->width - job->crop[3] - 20 - sub->width );

    lum   = sub->data;
    alpha = lum + sub->width * sub->height;
    out   = buf->data + offset_top * title->width + offset_left;

    for( i = 0; i < sub->height; i++ )
    {
        for( j = 0; j < sub->width; j++ )
        {
            out[j] = ( (uint16_t) out[j] * ( 16 - (uint16_t) alpha[j] ) +
                       (uint16_t) lum[j] * (uint16_t) alpha[j] ) >> 4;
        }
        lum   += sub->width;
        alpha += sub->width;
        out   += title->width;
    }

    hb_buffer_close( _sub );
}

static int Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                 hb_buffer_t ** buf_out )
{
    hb_job_t   * job   = w->job;
    hb_title_t * title = job->title;
    hb_buffer_t * in = *buf_in, * buf;

    avpicture_fill( &w->pic_raw, in->data, PIX_FMT_YUV420P,
                    title->width, title->height );

    buf        = hb_buffer_init( 3 * job->width * job->height / 2 );
    buf->start = in->start;
    buf->stop  = in->stop;

    if( job->deinterlace && w->context )
    {
        avpicture_fill( &w->pic_render, buf->data, PIX_FMT_YUV420P,
                        job->width, job->height );
        avpicture_deinterlace( &w->pic_deint, &w->pic_raw,
                               PIX_FMT_YUV420P, title->width,
                               title->height );
        ApplySub( job, w->buf_deint, &in->sub );
        img_resample( w->context, &w->pic_render, &w->pic_deint );
    }
    else if( job->deinterlace )
    {
        avpicture_fill( &w->pic_deint, buf->data, PIX_FMT_YUV420P,
                        job->width, job->height );
        avpicture_deinterlace( &w->pic_deint, &w->pic_raw,
                               PIX_FMT_YUV420P, title->width,
                               title->height );
        ApplySub( job, buf, &in->sub );
    }
    else if( w->context )
    {
        ApplySub( job, in, &in->sub );
        avpicture_fill( &w->pic_render, buf->data, PIX_FMT_YUV420P,
                        job->width, job->height );
        img_resample( w->context, &w->pic_render, &w->pic_raw );
    }
    else
    {
        hb_buffer_close( &buf );
        ApplySub( job, in, &in->sub );
        buf      = in;
        *buf_in  = NULL;
    }

    (*buf_out) = buf;

    return HB_WORK_OK;
}

static void Close( hb_work_object_t ** _w )
{
    hb_work_object_t * w = *_w;
    free( w->name );
    free( w );
    *_w = NULL;
}

hb_work_object_t * hb_work_render_init( hb_job_t * job )
{
    hb_title_t * title;
    
    hb_work_object_t * w = calloc( sizeof( hb_work_object_t ), 1 );
    w->name  = strdup( "Renderer" );
    w->work  = Work;
    w->close = Close;

    title = job->title;

    w->job = job;

    if( job->crop[0] || job->crop[1] || job->crop[2] || job->crop[3] ||
        job->width != title->width || job->height != title->height )
    {
        w->context = img_resample_full_init(
            job->width, job->height, title->width, title->height,
            job->crop[0], job->crop[1], job->crop[2], job->crop[3],
            0, 0, 0, 0 );
    }

    if( job->deinterlace )
    {
        /* Allocate a constant buffer used for deinterlacing */
        w->buf_deint = hb_buffer_init( 3 * title->width *
                                       title->height / 2 );
        avpicture_fill( &w->pic_deint, w->buf_deint->data,
                        PIX_FMT_YUV420P, title->width, title->height );
    }
    return w;
}
