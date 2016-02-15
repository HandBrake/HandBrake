/* avfilter.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "hbffmpeg.h"
#include "common.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"


/*****
 * This is a meta-filter.  By itself it makes no modifications to the video.
 * Other filters use this filter to perform their function (see pad.c)
 */
struct hb_filter_private_s
{
    hb_buffer_list_t   list;
    AVFilterGraph    * graph;
    AVFilterContext  * last;
    AVFilterContext  * input;
    AVFilterContext  * output;
    AVFrame          * frame;
    AVRational         out_time_base;
    char             * settings;
};

static int  avfilter_init( hb_filter_object_t * filter,
                           hb_filter_init_t * init );
static void avfilter_close( hb_filter_object_t * filter );
static int  avfilter_work( hb_filter_object_t * filter,
                           hb_buffer_t ** buf_in, hb_buffer_t ** buf_out );
static hb_filter_info_t * avfilter_info( hb_filter_object_t * filter );

hb_filter_object_t hb_filter_avfilter =
{
    .id            = HB_FILTER_AVFILTER,
    .enforce_order = 0,
    .name          = "avfilter",
    .settings      = NULL,
    .init          = avfilter_init,
    .work          = avfilter_work,
    .close         = avfilter_close,
    .info          = avfilter_info,
};

hb_filter_object_t hb_filter_pad =
{
    .id            = HB_FILTER_PAD,
    .enforce_order = 1,
    .name          = "avfilter",
    .settings      = NULL,
    .init          = avfilter_init,
    .work          = avfilter_work,
    .close         = avfilter_close,
    .info          = avfilter_info,
};

hb_filter_object_t hb_filter_rotate =
{
    .id            = HB_FILTER_ROTATE,
    .enforce_order = 1,
    .name          = "avfilter",
    .settings      = NULL,
    .init          = avfilter_init,
    .work          = avfilter_work,
    .close         = avfilter_close,
    .info          = avfilter_info,
};

hb_filter_object_t hb_filter_deinterlace =
{
    .id            = HB_FILTER_DEINTERLACE,
    .enforce_order = 1,
    .name          = "avfilter",
    .settings      = NULL,
    .init          = avfilter_init,
    .work          = avfilter_work,
    .close         = avfilter_close,
    .info          = avfilter_info,
};

static AVFilterContext * append_filter( hb_filter_private_t * pv,
                                        const char * name, const char * args)
{
    AVFilterContext * filter;
    int               result;

    result = avfilter_graph_create_filter(&filter, avfilter_get_by_name(name),
                                          name, args, NULL, pv->graph);
    if (result < 0)
    {
        return NULL;
    }
    if (pv->last != NULL)
    {
        result = avfilter_link(pv->last, 0, filter, 0);
        if (result < 0)
        {
            avfilter_free(filter);
            return NULL;
        }
    }
    pv->last = filter;

    return filter;
}

static int avfilter_init( hb_filter_object_t * filter, hb_filter_init_t * init )
{
    hb_filter_private_t * pv = filter->private_data;
    char                * sws_flags;
    AVFilterContext     * avfilter;
    char                * filter_args;
    int                   result;
    AVFilterInOut       * in = NULL, * out = NULL;

    if (filter->settings == NULL || filter->settings[0] == 0)
    {
        hb_error("avfilter_init: no filter settings specified");
        return 1;
    }

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;

    pv->settings = strdup(filter->settings);
    pv->graph = avfilter_graph_alloc();
    if (pv->graph == NULL)
    {
        hb_error("avfilter_init: avfilter_graph_alloc failed");
        goto fail;
    }

    sws_flags = hb_strdup_printf("flags=%d", SWS_LANCZOS|SWS_ACCURATE_RND);
    pv->graph->scale_sws_opts = sws_flags;

    result = avfilter_graph_parse2(pv->graph, filter->settings, &in, &out);
    if (result < 0 || in == NULL || out == NULL)
    {
        hb_error("avfilter_init: avfilter_graph_parse2 failed (%s)",
                 filter->settings);
        goto fail;
    }

    // Build filter input
    filter_args = hb_strdup_printf(
                "width=%d:height=%d:pix_fmt=%d:sar=%d/%d:"
                "time_base=%d/%d:frame_rate=%d/%d",
                init->geometry.width, init->geometry.height, init->pix_fmt,
                init->geometry.par.num, init->geometry.par.den,
                1, 90000, init->vrate.num, init->vrate.den);

    avfilter = append_filter(pv, "buffer", filter_args);
    free(filter_args);
    if (avfilter == NULL)
    {
        hb_error("avfilter_init: failed to create buffer source filter");
        goto fail;
    }
    pv->input = avfilter;

    avfilter = append_filter(pv, "format", "yuv420p");
    if (avfilter == NULL)
    {
        hb_error("avfilter_init: failed to create pix format filter");
        goto fail;
    }

    // Link input to filter chain created by avfilter_graph_parse2
    result = avfilter_link(pv->last, 0, in->filter_ctx, 0);
    if (result < 0)
    {
        goto fail;
    }
    pv->last = out->filter_ctx;

    // Build filter output
    avfilter = append_filter(pv, "buffersink", NULL);
    if (avfilter == NULL)
    {
        hb_error("avfilter_init: failed to create buffer output filter");
        goto fail;
    }
    pv->output = avfilter;

    result = avfilter_graph_config(pv->graph, NULL);
    if (result < 0)
    {
        hb_error("avfilter_init: failed to configure filter graph");
        goto fail;
    }

    pv->frame = av_frame_alloc();
    if (pv->frame == NULL)
    {
        hb_error("avfilter_init: failed to allocate frame filter");
        goto fail;
    }

    avfilter_inout_free(&in);
    avfilter_inout_free(&out);

    // Retrieve the parameters of the output filter
    AVFilterLink *link     = pv->output->inputs[0];
    init->geometry.width   = link->w;
    init->geometry.height  = link->h;
    init->geometry.par.num = link->sample_aspect_ratio.num;
    init->geometry.par.den = link->sample_aspect_ratio.den;
    init->pix_fmt          = link->format;
    // avfilter can generate "unknown" framerates.  If this happens
    // just pass along the source framerate.
    if (link->frame_rate.num > 0 && link->frame_rate.den > 0)
    {
        init->vrate.num        = link->frame_rate.num;
        init->vrate.den        = link->frame_rate.den;
    }
    pv->out_time_base      = link->time_base;

    hb_buffer_list_clear(&pv->list);

    return 0;

fail:
    if (pv->input != NULL)
        avfilter_free(pv->input);
    if (pv->output != NULL)
        avfilter_free(pv->output);
    avfilter_inout_free(&in);
    avfilter_inout_free(&out);
    avfilter_graph_free(&pv->graph);
    free(pv);

    return 1;
}

static hb_filter_info_t * avfilter_info( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_filter_info_t    * info;

    if( !pv )
        return NULL;

    info = calloc(1, sizeof(hb_filter_info_t));
    info->human_readable_desc = malloc(1024);
    info->human_readable_desc[0] = 0;

    char * dst   = info->human_readable_desc;
    char * start = pv->settings;
    while (start != NULL && *start != 0)
    {
        // Find end of a filter
        char * comma = strchr(start, ',');
        char * quote = strchr(start, '\'');
        if (comma != NULL && quote != NULL && quote < comma)
        {
            // Find end of quote
            quote = strchr(quote+1, '\'');
            comma = strchr(start, ',');
        }
        // pretty print line
        int name = 1;
        while (*start != 0 && (comma == NULL || start < comma))
        {
            switch (*start)
            {
                case '=':
                    if (name)
                    {
                        *dst++ = ':';
                        *dst++ = ' ';
                        name = 0;
                    }
                    else
                    {
                        *dst++ = '=';
                    }
                    break;

                case ':':
                    *dst++ = ',';
                    *dst++ = ' ';
                    break;

                case '\'':
                case ' ':
                    break;

                default:
                    *dst++ = *start;

            }
            start++;
        }
        if (*start != 0)
        {
            *dst++ = '\n';
            start++;
        }
    }
    *dst = 0;
    return info;
}

static void avfilter_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;
    if (pv == NULL)
    {
        // Already closed
        return;
    }

    av_frame_free(&pv->frame);
    avfilter_graph_free(&pv->graph);
    free(pv->settings);
    free(pv);
    filter->private_data = NULL;
}

static void fill_frame(hb_filter_private_t * pv,
                       AVFrame * frame, hb_buffer_t * buf)
{
    frame->data[0]     = buf->plane[0].data;
    frame->data[1]     = buf->plane[1].data;
    frame->data[2]     = buf->plane[2].data;
    frame->linesize[0] = buf->plane[0].stride;
    frame->linesize[1] = buf->plane[1].stride;
    frame->linesize[2] = buf->plane[2].stride;

    frame->pts              = buf->s.start;
    frame->reordered_opaque = buf->s.start;
    frame->width            = buf->f.width;
    frame->height           = buf->f.height;
    frame->format           = buf->f.fmt;
    frame->interlaced_frame = !!buf->s.combed;
}

static hb_buffer_t* avframe_to_buffer(hb_filter_private_t * pv, AVFrame *frame)
{
    hb_buffer_t * buf;

    buf = hb_frame_buffer_init(frame->format, frame->width, frame->height);
    if (buf == NULL)
    {
        return NULL;
    }

    int pp;
    for (pp = 0; pp < 3; pp++)
    {
        int yy;
        int width     = buf->plane[pp].width;
        int stride    = buf->plane[pp].stride;
        int height    = buf->plane[pp].height;
        int linesize  = frame->linesize[pp];
        uint8_t * dst = buf->plane[pp].data;
        uint8_t * src = frame->data[pp];

        for (yy = 0; yy < height; yy++)
        {
            memcpy(dst, src, width);
            dst += stride;
            src += linesize;
        }
    }
    buf->s.start = av_rescale_q(frame->pts, pv->out_time_base,
                                (AVRational){1, 90000});

    return buf;
}

static hb_buffer_t* filterFrame( hb_filter_private_t * pv, hb_buffer_t * in )
{
    int                result;
    hb_buffer_list_t   list;

    fill_frame(pv, pv->frame, in);
    result = av_buffersrc_add_frame(pv->input, pv->frame);
    if (result < 0)
    {
        return NULL;
    }

    hb_buffer_list_clear(&list);
    result = av_buffersink_get_frame(pv->output, pv->frame);
    while (result >= 0)
    {
        hb_buffer_t * buf = avframe_to_buffer(pv, pv->frame);
        hb_buffer_list_append(&pv->list, buf);
        av_frame_unref(pv->frame);

        result = av_buffersink_get_frame(pv->output, pv->frame);
    }
    while (hb_buffer_list_count(&pv->list) > 1)
    {
        hb_buffer_t * buf  = hb_buffer_list_rem_head(&pv->list);
        hb_buffer_t * next = hb_buffer_list_head(&pv->list);

        buf->s.stop = next->s.start;
        hb_buffer_list_append(&list, buf);
    }

    return hb_buffer_list_head(&list);
}

static int avfilter_work( hb_filter_object_t * filter,
                     hb_buffer_t ** buf_in, hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t * in = *buf_in;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        hb_buffer_list_append(&pv->list, in);
        *buf_out = hb_buffer_list_clear(&pv->list);
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    *buf_out = filterFrame(pv, in);

    return HB_FILTER_OK;
}

void hb_avfilter_combine( hb_list_t * list )
{
    hb_filter_object_t * avfilter = NULL;
    int                  ii;

    for (ii = 0; ii < hb_list_count(list);)
    {
        hb_filter_object_t * filter = hb_list_item(list, ii);
        switch (filter->id)
        {
            case HB_FILTER_AVFILTER:
            case HB_FILTER_ROTATE:
            case HB_FILTER_DEINTERLACE:
            case HB_FILTER_PAD:
                if (avfilter != NULL)
                {
                    // Chain filter together
                    char * settings;
                    settings = hb_strdup_printf("%s, %s", avfilter->settings,
                                                          filter->settings);
                    free(avfilter->settings);
                    avfilter->settings = settings;
                    hb_list_rem(list, filter);
                    hb_filter_close(&filter);
                    continue;
                }
                else
                {
                    avfilter = filter;
                    avfilter->id = HB_FILTER_AVFILTER;
                }
                break;
            default:
                avfilter = NULL;
        }
        ii++;
    }
}

