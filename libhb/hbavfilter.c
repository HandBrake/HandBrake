/* hbavfilter.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "handbrake/hbavfilter.h"
#include "handbrake/avfilter_priv.h"

struct hb_avfilter_graph_s
{
    AVFilterGraph    * avgraph;
    AVFilterContext  * last;
    AVFilterContext  * input;
    AVFilterContext  * output;
    char             * settings;
    AVFrame          * frame;
    AVRational         out_time_base;
};

static AVFilterContext * append_filter( hb_avfilter_graph_t * graph,
                                        const char * name, const char * args)
{
    AVFilterContext * filter;
    int               result;

    result = avfilter_graph_create_filter(&filter, avfilter_get_by_name(name),
                                          name, args, NULL, graph->avgraph);
    if (result < 0)
    {
        return NULL;
    }
    if (graph->last != NULL)
    {
        result = avfilter_link(graph->last, 0, filter, 0);
        if (result < 0)
        {
            avfilter_free(filter);
            return NULL;
        }
    }
    graph->last = filter;

    return filter;
}

hb_avfilter_graph_t *
hb_avfilter_graph_init(hb_value_t * settings, hb_filter_init_t * init)
{
    hb_avfilter_graph_t * graph;
    AVFilterInOut       * in = NULL, * out = NULL;
    AVFilterContext     * avfilter;
    char                * settings_str;
    int                   result;
    char                * filter_args;

    graph = calloc(1, sizeof(hb_avfilter_graph_t));
    if (graph == NULL)
    {
        return NULL;
    }

    settings_str = hb_filter_settings_string(HB_FILTER_AVFILTER, settings);
    if (settings_str == NULL)
    {
        hb_error("hb_avfilter_graph_init: no filter settings specified");
        goto fail;
    }

    graph->settings = settings_str;
    graph->avgraph = avfilter_graph_alloc();
    if (graph->avgraph == NULL)
    {
        hb_error("hb_avfilter_graph_init: avfilter_graph_alloc failed");
        goto fail;
    }

    av_opt_set(graph->avgraph, "scale_sws_opts", "lanczos+accurate_rnd", 0);

    result = avfilter_graph_parse2(graph->avgraph, settings_str, &in, &out);
    if (result < 0 || in == NULL || out == NULL)
    {
        hb_error("hb_avfilter_graph_init: avfilter_graph_parse2 failed (%s)",
                 settings_str);
        goto fail;
    }

    // Build filter input
    filter_args = hb_strdup_printf(
                "width=%d:height=%d:pix_fmt=%d:sar=%d/%d:"
                "time_base=%d/%d:frame_rate=%d/%d",
                init->geometry.width, init->geometry.height, init->pix_fmt,
                init->geometry.par.num, init->geometry.par.den,
                init->time_base.num, init->time_base.den,
                init->vrate.num, init->vrate.den);

    avfilter = append_filter(graph, "buffer", filter_args);
    free(filter_args);
    if (avfilter == NULL)
    {
        hb_error("hb_avfilter_graph_init: failed to create buffer source filter");
        goto fail;
    }
    graph->input = avfilter;

    // Link input to filter chain created by avfilter_graph_parse2
    result = avfilter_link(graph->last, 0, in->filter_ctx, 0);
    if (result < 0)
    {
        goto fail;
    }
    graph->last = out->filter_ctx;

    // Build filter output
    avfilter = append_filter(graph, "buffersink", NULL);
    if (avfilter == NULL)
    {
        hb_error("hb_avfilter_graph_init: failed to create buffer output filter");
        goto fail;
    }
#if 0
    // Set output pix fmt to YUV420P
    enum AVPixelFormat pix_fmts[2] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};
    if (av_opt_set_int_list(avfilter, "pix_fmts", pix_fmts,
                            AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0)
    {
        hb_error("hb_avfilter_graph_init: failed to set buffersink pix_fmt");
        goto fail;
    }
#endif

    graph->output = avfilter;

    result = avfilter_graph_config(graph->avgraph, NULL);
    if (result < 0)
    {
        hb_error("hb_avfilter_graph_init: failed to configure filter graph");
        goto fail;
    }

    graph->frame = av_frame_alloc();
    if (graph->frame == NULL)
    {
        hb_error("hb_avfilter_graph_init: failed to allocate frame filter");
        goto fail;
    }

    graph->out_time_base = graph->output->inputs[0]->time_base;

    avfilter_inout_free(&in);
    avfilter_inout_free(&out);

    return graph;

fail:
    avfilter_inout_free(&in);
    avfilter_inout_free(&out);
    hb_avfilter_graph_close(&graph);

    return NULL;
}

const char * hb_avfilter_graph_settings(hb_avfilter_graph_t * graph)
{
    return graph->settings;
}

void hb_avfilter_graph_close(hb_avfilter_graph_t ** _g)
{
    hb_avfilter_graph_t * graph = *_g;

    if (graph == NULL)
    {
        return;
    }
    if (graph->avgraph != NULL)
    {
        avfilter_graph_free(&graph->avgraph);
    }
    free(graph->settings);
    av_frame_free(&graph->frame);
    free(graph);
    *_g = NULL;
}

void hb_avfilter_graph_update_init(hb_avfilter_graph_t * graph,
                                   hb_filter_init_t    * init)
{
    // Retrieve the parameters of the output filter
    AVFilterLink *link     = graph->output->inputs[0];
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
}

int hb_avfilter_add_frame(hb_avfilter_graph_t * graph, AVFrame * frame)
{
    return av_buffersrc_add_frame(graph->input, frame);
}

int hb_avfilter_get_frame(hb_avfilter_graph_t * graph, AVFrame * frame)
{
    return av_buffersink_get_frame(graph->output, frame);
}

int hb_avfilter_add_buf(hb_avfilter_graph_t * graph, hb_buffer_t * in)
{
    if (in != NULL)
    {
        hb_video_buffer_to_avframe(graph->frame, in);
        return av_buffersrc_add_frame(graph->input, graph->frame);
    }
    else
    {
        return av_buffersrc_add_frame(graph->input, NULL);
    }
}

hb_buffer_t * hb_avfilter_get_buf(hb_avfilter_graph_t * graph)
{
    int           result;

    result = av_buffersink_get_frame(graph->output, graph->frame);
    if (result >= 0)
    {
        hb_buffer_t * buf;
        buf = hb_avframe_to_video_buffer(graph->frame, graph->out_time_base);
        av_frame_unref(graph->frame);
        return buf;
    }

    return NULL;
}

void hb_avfilter_combine( hb_list_t * list)
{
    hb_filter_object_t  * avfilter = NULL;
    hb_value_t          * settings = NULL;
    int                   ii;

    for (ii = 0; ii < hb_list_count(list); ii++)
    {
        hb_filter_object_t * filter = hb_list_item(list, ii);
        hb_filter_private_t * pv = filter->private_data;
        switch (filter->id)
        {
            case HB_FILTER_AVFILTER:
            case HB_FILTER_DEINTERLACE:
            case HB_FILTER_DEBLOCK:
            case HB_FILTER_CROP_SCALE:
            case HB_FILTER_PAD:
            case HB_FILTER_ROTATE:
            {
                settings = pv->avfilters;
            } break;
            default:
            {
                settings = NULL;
                avfilter = NULL;
            } break;
        }
        if (settings != NULL)
        {
            if (avfilter == NULL)
            {
                hb_filter_private_t * avpv = NULL;
                avfilter = hb_filter_init(HB_FILTER_AVFILTER);
                avfilter->aliased = 1;

                avpv = calloc(1, sizeof(struct hb_filter_private_s));
                avfilter->private_data = avpv;
                avpv->input = pv->input;

                avfilter->settings = hb_value_array_init();
                hb_list_insert(list, ii, avfilter);
                ii++;
            }

            hb_value_array_concat(avfilter->settings, settings);
        }
    }
}

void hb_avfilter_append_dict(hb_value_array_t * filters,
                           const char * name, hb_value_t * settings)
{
    hb_dict_t * filter_dict = hb_dict_init();

    hb_dict_set(filter_dict, name, settings);
    hb_value_array_append(filters, filter_dict);
}

