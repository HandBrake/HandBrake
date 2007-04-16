/* $Id: render.c,v 1.17 2005/04/14 17:37:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "ffmpeg/avcodec.h"

struct hb_work_private_s
{
    hb_job_t * job;

    ImgReSampleContext * context;
    AVPicture            pic_raw;
    AVPicture            pic_deint;
    AVPicture            pic_render;
    hb_buffer_t        * buf_deint;
};

int  renderInit( hb_work_object_t *, hb_job_t * );
int  renderWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void renderClose( hb_work_object_t * );

hb_work_object_t hb_render =
{   
    WORK_RENDER,
    "Renderer",
    renderInit,
    renderWork,
    renderClose
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

    /* If necessary, move the subtitle so it is not in a cropped zone.
       When it won't fit, we center it so we loose as much on both ends.
       Otherwise we try to leave a 20px margin around it. */

    if( sub->height > title->height - job->crop[0] - job->crop[1] - 40 )
        offset_top = job->crop[0] + ( title->height - job->crop[0] -
                job->crop[1] - sub->height ) / 2;
    else if( sub->y < job->crop[0] + 20 )
        offset_top = job->crop[0] + 20;
    else if( sub->y > title->height - job->crop[1] - 20 - sub->height )
        offset_top = title->height - job->crop[1] - 20 - sub->height;
    else
        offset_top = sub->y;

    if( sub->width > title->width - job->crop[2] - job->crop[3] - 40 )
        offset_left = job->crop[2] + ( title->width - job->crop[2] -
                job->crop[3] - sub->width ) / 2;
    else if( sub->x < job->crop[2] + 20 )
        offset_left = job->crop[2] + 20;
    else if( sub->x > title->width - job->crop[3] - 20 - sub->width )
        offset_left = title->width - job->crop[3] - 20 - sub->width;
    else
        offset_left = sub->x;

    lum   = sub->data;
    alpha = lum + sub->width * sub->height;
    out   = buf->data + offset_top * title->width + offset_left;

    for( i = 0; i < sub->height; i++ )
    {
        if( offset_top + i >= 0 && offset_top + i < title->height )
        {
            for( j = 0; j < sub->width; j++ )
            {
                if( offset_left + j >= 0 && offset_left + j < title->width )
                {
                    out[j] = ( (uint16_t) out[j] * ( 16 - (uint16_t) alpha[j] ) +
                               (uint16_t) lum[j] * (uint16_t) alpha[j] ) >> 4;
                }
            }
        }

        lum   += sub->width;
        alpha += sub->width;
        out   += title->width;
    }

    hb_buffer_close( _sub );
}

int renderWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t   * job   = pv->job;
    hb_title_t * title = job->title;
    hb_buffer_t * in = *buf_in, * buf;

    if(!in->data)
    {
        /* If the input buffer is end of stream, send out an empty one to the next stage as well. */
        *buf_out = hb_buffer_init(0);
        return HB_WORK_OK;
    }

    avpicture_fill( &pv->pic_raw, in->data, PIX_FMT_YUV420P,
                    title->width, title->height );

    buf        = hb_buffer_init( 3 * job->width * job->height / 2 );
    buf->start = in->start;
    buf->stop  = in->stop;

    if( job->deinterlace && pv->context )
    {
        avpicture_fill( &pv->pic_render, buf->data, PIX_FMT_YUV420P,
                        job->width, job->height );
        avpicture_deinterlace( &pv->pic_deint, &pv->pic_raw,
                               PIX_FMT_YUV420P, title->width,
                               title->height );
        ApplySub( job, pv->buf_deint, &in->sub );
        img_resample( pv->context, &pv->pic_render, &pv->pic_deint );
    }
    else if( job->deinterlace )
    {
        avpicture_fill( &pv->pic_deint, buf->data, PIX_FMT_YUV420P,
                        job->width, job->height );
        avpicture_deinterlace( &pv->pic_deint, &pv->pic_raw,
                               PIX_FMT_YUV420P, title->width,
                               title->height );
        ApplySub( job, buf, &in->sub );
    }
    else if( pv->context )
    {
        ApplySub( job, in, &in->sub );
        avpicture_fill( &pv->pic_render, buf->data, PIX_FMT_YUV420P,
                        job->width, job->height );
        img_resample( pv->context, &pv->pic_render, &pv->pic_raw );
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

void renderClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    
    free( pv );
    w->private_data = NULL;
}

int renderInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_title_t * title;
    
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    title = job->title;

    pv->job = job;

    if( job->crop[0] || job->crop[1] || job->crop[2] || job->crop[3] ||
        job->width != title->width || job->height != title->height )
    {
        pv->context = img_resample_full_init(
            job->width, job->height, title->width, title->height,
            job->crop[0], job->crop[1], job->crop[2], job->crop[3],
            0, 0, 0, 0 );
    }

    if( job->deinterlace )
    {
        /* Allocate a constant buffer used for deinterlacing */
        pv->buf_deint = hb_buffer_init( 3 * title->width *
                                       title->height / 2 );
        avpicture_fill( &pv->pic_deint, pv->buf_deint->data,
                        PIX_FMT_YUV420P, title->width, title->height );
    }

    return 0;
}
