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
    int                 pad[4];
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

    int ii;
    int color;

    pv->job = init->job;
    pv->width_in   = init->geometry.width;
    pv->height_in  = init->geometry.height;

    memcpy(pv->pad, init->pad, sizeof( int[4] ));
    if (filter->settings)
    {
        sscanf(filter->settings, "%d:%d:%d:%d:%u",
               &pv->pad[0], &pv->pad[1], &pv->pad[2], &pv->pad[3], &pv->rgb);
    }

    // TODO: handle other input pix_fmt
    color = hb_rgb2yuv(pv->rgb);
    pv->color[0] = (color >> 16) & 0xff;
    pv->color[1] = (color      ) & 0xff;
    pv->color[2] = (color >>  8) & 0xff;

    for (ii = 0; ii < 4; ii++)
    {
        if (pv->pad[ii] & 1)
        {
            hb_log("Pad must be even! Fixing...");
            pv->pad[ii]++;
        }
    }
    pv->width_out  = init->geometry.width  + pv->pad[2] + pv->pad[3];
    pv->height_out = init->geometry.height + pv->pad[0] + pv->pad[1];

    // Set init values so the next stage in the pipline
    // knows what it will be getting
    init->geometry.width = pv->width_out;
    init->geometry.height = pv->height_out;
    memcpy(init->pad, pv->pad, sizeof(int[4]));

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

    sprintf( info->human_readable_desc,
        "source: %d * %d, color 0x%x, pad (%d/%d/%d/%d): %d * %d",
        pv->width_in, pv->height_in, pv->rgb,
        pv->pad[0], pv->pad[1], pv->pad[2], pv->pad[3],
        pv->width_out, pv->height_out );

    return 0;
}

static void pad_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    free( pv );
    filter->private_data = NULL;
}

static hb_buffer_t* pad( hb_filter_private_t * pv, hb_buffer_t * in )
{
    AVPicture     pic_in;
    AVPicture     pic_out;
    hb_buffer_t * out;

    out = hb_frame_buffer_init(in->f.fmt, pv->width_out, pv->height_out);

    hb_avpicture_fill(&pic_in,  in );
    hb_avpicture_fill(&pic_out, out );

    av_picture_pad(&pic_out, &pic_in, pv->height_out, pv->width_out, in->f.fmt,
                   pv->pad[0], pv->pad[1], pv->pad[2], pv->pad[3],
                   pv->color);

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
