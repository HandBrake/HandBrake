/* pad.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "hbffmpeg.h"
#include "common.h"

struct hb_filter_private_s
{
    hb_job_t            *job;
    int                 width_in;
    int                 height_in;
    int                 width_out;
    int                 height_out;
    int                 x;
    int                 y;
    int                 rgb;
    int                 color[3];
};

static int  pad_init( hb_filter_object_t * filter, hb_filter_init_t * init );
static int  pad_info( hb_filter_object_t * filter, hb_filter_info_t * info );
static void pad_close( hb_filter_object_t * filter );
static int  pad_work( hb_filter_object_t * filter,
                      hb_buffer_t ** buf_in, hb_buffer_t ** buf_out );

hb_filter_object_t hb_filter_pad =
{
    .id            = HB_FILTER_PAD,
    .enforce_order = 1,
    .name          = "Pad Border",
    .settings      = NULL,
    .init          = pad_init,
    .work          = pad_work,
    .close         = pad_close,
    .info          = pad_info,
};

static int pad_init( hb_filter_object_t * filter, hb_filter_init_t * init )
{
    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;

    int color;

    pv->job       = init->job;
    pv->width_in  = pv->width_out  = init->geometry.width;
    pv->height_in = pv->height_out = init->geometry.height;

    if (filter->settings)
    {
        sscanf(filter->settings, "%d:%d:%d:%d:%u",
               &pv->width_out, &pv->height_out, &pv->x, &pv->y, &pv->rgb);
    }

    // TODO: handle other input pix_fmt
    color = hb_rgb2yuv(pv->rgb);
    pv->color[0] = (color >> 16) & 0xff;
    pv->color[1] = (color      ) & 0xff;
    pv->color[2] = (color >>  8) & 0xff;

    if (pv->width_out & 1)
    {
        pv->width_out++;
        hb_log("pad_init: Width must be even! Fixing...");
    }
    if (pv->height_out & 1)
    {
        pv->height_out++;
        hb_log("pad_init: Height must be even! Fixing...");
    }

    // Set init values so the next stage in the pipline
    // knows what it will be getting
    init->geometry.width = pv->width_out;
    init->geometry.height = pv->height_out;

    return 0;
}

static int pad_info( hb_filter_object_t * filter, hb_filter_info_t * info )
{
    hb_filter_private_t * pv = filter->private_data;

    if( !pv )
        return 0;

    // Set init values so the next stage in the pipline
    // knows what it will be getting
    memset( info, 0, sizeof( hb_filter_info_t ) );
    info->out.geometry.width = pv->width_out;
    info->out.geometry.height = pv->height_out;

    int pad[4];
    pad[0] = pv->y;
    pad[1] = pv->height_out - pv->height_in - pv->y;
    pad[2] = pv->x;
    pad[3] = pv->width_out - pv->width_in - pv->x;

    sprintf( info->human_readable_desc,
        "source: %d * %d, color 0x%x, pad (%d/%d/%d/%d): %d * %d",
        pv->width_in, pv->height_in, pv->rgb,
        pad[0], pad[1], pad[2], pad[3],
        pv->width_out, pv->height_out );

    return 0;
}

static void pad_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    free( pv );
    filter->private_data = NULL;
}

static void draw_rect(hb_buffer_t * buf, int pos_x, int pos_y,
                      int width, int height, int *color)
{
    // Sanitize bounds
    if (pos_x >= buf->f.width || pos_y > buf->f.height)
    {
        return;
    }
    if (pos_x < 0)
    {
        width += pos_x;
        pos_x = 0;
    }
    if (pos_y < 0)
    {
        height += pos_y;
        pos_y = 0;
    }
    if (pos_x + width > buf->f.width)
    {
        width = buf->f.width - pos_x;
    }
    if (pos_y + height > buf->f.height)
    {
        height = buf->f.height - pos_y;
    }
    if (width <= 0 || height <= 0)
    {
        return;
    }

    int pp;
    for (pp = 0; pp < 3; pp++)
    {
        int       yy, x_div, y_div, w, h, x, y;
        uint8_t * dst;

        x_div = buf->plane[0].width  / buf->plane[pp].width;
        y_div = buf->plane[0].height / buf->plane[pp].height;

        w = width  / x_div;
        h = height / y_div;
        x = pos_x  / x_div;
        y = pos_y  / y_div;

        dst = buf->plane[pp].data + buf->plane[pp].stride * y + x;
        for (yy = 0; yy < h; yy++)
        {
            memset(dst, color[pp], w);
            dst += buf->plane[pp].stride;
        }
    }
}

static void copy_rect(hb_buffer_t *dst, int dst_x, int dst_y,
                      hb_buffer_t *src, int src_x, int src_y,
                      int width, int height)
{
    // Sanitize bounds
    if (dst_x >= dst->f.width || dst_y > dst->f.height)
    {
        return;
    }
    if (src_x >= src->f.width || src_y > src->f.height)
    {
        return;
    }
    if (dst_x < 0)
    {
        width += dst_x;
        src_x -= dst_x;
        dst_x  = 0;
    }
    if (dst_y < 0)
    {
        height += dst_y;
        src_y  -= dst_y;
        dst_y   = 0;
    }
    if (src_x < 0)
    {
        width += src_x;
        src_x  = 0;
    }
    if (src_y < 0)
    {
        height += src_x;
        src_y   = 0;
    }
    if (dst_x + width > dst->f.width)
    {
        width = dst->f.width - dst_x;
    }
    if (dst_y + height > dst->f.height)
    {
        height = dst->f.height - dst_y;
    }
    if (src_x + width > src->f.width)
    {
        width = src->f.width - src_x;
    }
    if (src_y + height > src->f.height)
    {
        height = src->f.height - src_y;
    }
    if (width <= 0 || height <= 0)
    {
        return;
    }

    int pp;
    for (pp = 0; pp < 3; pp++)
    {
        int       yy, x_div, y_div, w, h, dx, dy, sx, sy;
        uint8_t * dst_data, *src_data;

        x_div = dst->plane[0].width  / dst->plane[pp].width;
        y_div = dst->plane[0].height / dst->plane[pp].height;

        w = width  / x_div;
        h = height / y_div;
        dx = dst_x  / x_div;
        dy = dst_y  / y_div;
        sx = src_x  / x_div;
        sy = src_y  / y_div;

        dst_data = dst->plane[pp].data + dst->plane[pp].stride * dy + dx;
        src_data = src->plane[pp].data + dst->plane[pp].stride * sy + sx;
        for (yy = 0; yy < h; yy++)
        {
            memcpy(dst_data, src_data, w);
            dst_data += dst->plane[pp].stride;
            src_data += src->plane[pp].stride;
        }
    }
}

static hb_buffer_t* pad( hb_filter_private_t * pv, hb_buffer_t * in )
{
    int           width  = pv->width_out;
    int           height = pv->height_out;
    int           bottom = height - in->f.height - pv->y;
    int           right  = width  - in->f.width  - pv->x;
    hb_buffer_t * out    = hb_frame_buffer_init(in->f.fmt, width, height);

    // copy image
    copy_rect(out, pv->x, pv->y, in, 0, 0, in->f.width, in->f.height);
    // top bar
    draw_rect(out, 0, 0, width, pv->y, pv->color);
    // bottom bar
    draw_rect(out, 0, pv->y + in->f.height, width, bottom, pv->color);
    // left bar
    draw_rect(out, 0, pv->y, pv->x, in->f.height, pv->color);
    // right bar
    draw_rect(out, pv->x + in->f.width, pv->y, right, in->f.height, pv->color);

    out->s = in->s;
    return out;
}

static int pad_work( hb_filter_object_t * filter,
                     hb_buffer_t ** buf_in, hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    *buf_out = pad(pv, in);

    return HB_FILTER_OK;
}
