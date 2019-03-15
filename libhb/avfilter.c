/* avfilter.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "hbffmpeg.h"
#include "hbavfilter.h"
#include "common.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "decomb.h"


/*****
 * This is a meta-filter.  By itself it makes no modifications to the video.
 * Other filters use this filter to perform their function (see pad.c)
 */
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

struct hb_filter_private_s
{
    int                   initialized;
    hb_avfilter_graph_t * graph;

    // Buffer list to delay output by one frame.  Required to set stop time.
    hb_buffer_list_t      list;

    // Placeholder settings for AVFilter aliases
    hb_value_t          * avfilters;
    hb_filter_init_t      input;
    hb_filter_init_t      output;
};

static int avfilter_init(hb_filter_object_t * filter, hb_filter_init_t * init);
static int avfilter_post_init( hb_filter_object_t * filter, hb_job_t * job );
static void avfilter_close( hb_filter_object_t * filter );
static void avfilter_alias_close( hb_filter_object_t * filter );
static int  avfilter_work( hb_filter_object_t * filter,
                           hb_buffer_t ** buf_in, hb_buffer_t ** buf_out );
static hb_filter_info_t * avfilter_info( hb_filter_object_t * filter );

static int  null_work( hb_filter_object_t * filter,
                       hb_buffer_t ** buf_in, hb_buffer_t ** buf_out );

static int crop_scale_init(hb_filter_object_t * filter,
                           hb_filter_init_t * init);
static hb_filter_info_t * crop_scale_info( hb_filter_object_t * filter );

static int pad_init(hb_filter_object_t * filter, hb_filter_init_t * init);

static int rotate_init(hb_filter_object_t * filter, hb_filter_init_t * init);

static int deinterlace_init(hb_filter_object_t * filter,
                            hb_filter_init_t * init);

static int colorspace_init(hb_filter_object_t * filter,
                           hb_filter_init_t * init);

hb_filter_object_t hb_filter_avfilter =
{
    .id            = HB_FILTER_AVFILTER,
    .enforce_order = 0,
    .name          = "AVFilter",
    .settings      = NULL,
    .init          = avfilter_init,
    .post_init     = avfilter_post_init,
    .work          = avfilter_work,
    .close         = avfilter_close,
    .info          = avfilter_info,
};

static const char crop_scale_template[] =
    "width=^"HB_INT_REG"$:height=^"HB_INT_REG"$:"
    "crop-top=^"HB_INT_REG"$:crop-bottom=^"HB_INT_REG"$:"
    "crop-left=^"HB_INT_REG"$:crop-right=^"HB_INT_REG"$";

hb_filter_object_t hb_filter_crop_scale =
{
    .id                = HB_FILTER_CROP_SCALE,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Crop and Scale",
    .settings          = NULL,
    .init              = crop_scale_init,
    .work              = null_work,
    .close             = avfilter_alias_close,
    .info              = crop_scale_info,
    .settings_template = crop_scale_template,
};

const char pad_template[] =
    "width=^"HB_INT_REG"$:height=^"HB_INT_REG"$:color=^"HB_ALL_REG"$:"
    "x=^"HB_INT_REG"$:y=^"HB_INT_REG"$";

hb_filter_object_t hb_filter_pad =
{
    .id                = HB_FILTER_PAD,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Pad",
    .settings          = NULL,
    .init              = pad_init,
    .work              = null_work,
    .close             = avfilter_alias_close,
    .settings_template = pad_template,
};

const char rotate_template[] =
    "angle=^(0|90|180|270)$:hflip=^"HB_BOOL_REG"$:disable=^"HB_BOOL_REG"$";

hb_filter_object_t hb_filter_rotate =
{
    .id                = HB_FILTER_ROTATE,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Rotate",
    .settings          = NULL,
    .init              = rotate_init,
    .work              = null_work,
    .close             = avfilter_alias_close,
    .settings_template = rotate_template,
};

const char deint_template[] =
    "mode=^"HB_INT_REG"$:parity=^([01])$";

hb_filter_object_t hb_filter_deinterlace =
{
    .id                = HB_FILTER_DEINTERLACE,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Deinterlace",
    .settings          = NULL,
    .init              = deinterlace_init,
    .work              = null_work,
    .close             = avfilter_alias_close,
    .settings_template = deint_template,
};

const char colorspace_template[] =
    "format=^"HB_ALL_REG"$:range=^"HB_ALL_REG"$:primaries=^"HB_ALL_REG"$:"
    "matrix=^"HB_ALL_REG"$:transfer=^"HB_ALL_REG"$";

hb_filter_object_t hb_filter_colorspace =
{
    .id                = HB_FILTER_COLORSPACE,
    .enforce_order     = 1,
    .skip              = 1,
    .name              = "Colorspace",
    .settings          = NULL,
    .init              = colorspace_init,
    .work              = null_work,
    .close             = avfilter_alias_close,
    .settings_template = colorspace_template,
};

static int  null_work( hb_filter_object_t * filter,
                       hb_buffer_t ** buf_in, hb_buffer_t ** buf_out )
{
    hb_log("null_work: It is an error to call this function.");
    return HB_WORK_DONE;
}

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

static int avfilter_init( hb_filter_object_t * filter, hb_filter_init_t * init )
{
    hb_filter_private_t * pv = NULL;

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;
    if (pv == NULL)
    {
        return 1;
    }
    pv->input = *init;
    pv->initialized = 1;

    pv->graph = hb_avfilter_graph_init(filter->settings, init);
    if (pv->graph == NULL)
    {
        goto fail;
    }

    // Retrieve the parameters of the output filter
    AVFilterLink *link     = pv->graph->output->inputs[0];
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
    pv->output = *init;

    hb_buffer_list_clear(&pv->list);

    return 0;

fail:
    hb_avfilter_graph_close(&pv->graph);
    free(pv);

    return 1;
}

static int avfilter_post_init( hb_filter_object_t * filter, hb_job_t * job )
{
    hb_filter_private_t * pv = filter->private_data;

    if (pv == NULL)
    {
        return 1;
    }
    if (pv->initialized)
    {
        return 0;
    }

    pv->graph = hb_avfilter_graph_init(filter->settings, &pv->input);
    if (pv->graph == NULL)
    {
        goto fail;
    }

    // Retrieve the parameters of the output filter
    hb_filter_init_t * init = &pv->output;
    AVFilterLink *link      = pv->graph->output->inputs[0];
    *init                   = pv->input;
    init->geometry.width    = link->w;
    init->geometry.height   = link->h;
    init->geometry.par.num  = link->sample_aspect_ratio.num;
    init->geometry.par.den  = link->sample_aspect_ratio.den;
    init->pix_fmt           = link->format;
    // avfilter can generate "unknown" framerates.  If this happens
    // just pass along the source framerate.
    if (link->frame_rate.num > 0 && link->frame_rate.den > 0)
    {
        init->vrate.num     = link->frame_rate.num;
        init->vrate.den     = link->frame_rate.den;
    }

    hb_buffer_list_clear(&pv->list);

    return 0;

fail:
    hb_avfilter_graph_close(&pv->graph);
    free(pv);

    return 1;
}

static hb_filter_info_t * avfilter_info(hb_filter_object_t * filter)
{
    hb_filter_private_t * pv = filter->private_data;
    hb_filter_info_t    * info;

    if (global_verbosity_level < 2)
    {
        // Only show this for log levels 2 and above
        return NULL;
    }
    if (pv == NULL)
    {
        return NULL;
    }

    info = calloc(1, sizeof(hb_filter_info_t));
    if (info == NULL)
    {
        hb_error("avfilter_info: allocation failure");
        return NULL;
    }
    info->output = pv->output;
    info->human_readable_desc = malloc(1024);
    if (info->human_readable_desc == NULL)
    {
        free(info);
        hb_error("avfilter_info: allocation failure");
        return NULL;
    }
    info->human_readable_desc[0] = 0;

    char * dst   = info->human_readable_desc;
    char * start = pv->graph->settings;
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

    hb_buffer_list_close(&pv->list);
    hb_avfilter_graph_close(&pv->graph);
    free(pv);
    filter->private_data = NULL;
}

static void avfilter_alias_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;
    if (pv == NULL)
    {
        // Already closed
        return;
    }

    hb_value_free(&pv->avfilters);
    free(pv);
    filter->private_data = NULL;
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
        return hb_avframe_to_video_buffer(graph->frame, graph->out_time_base);
    }

    return NULL;
}

int64_t hb_avfilter_get_frame_pts(hb_avfilter_graph_t * graph)
{
    return graph->frame->pts;
}

static hb_buffer_t* filterFrame( hb_filter_private_t * pv, hb_buffer_t * in )
{
    hb_buffer_list_t   list;
    hb_buffer_t      * buf, * next;

    hb_avfilter_add_buf(pv->graph, in);
    buf = hb_avfilter_get_buf(pv->graph);
    while (buf != NULL)
    {
        hb_buffer_list_append(&pv->list, buf);
        buf = hb_avfilter_get_buf(pv->graph);
    }

    // Delay one frame so we can set the stop time of the output buffer
    hb_buffer_list_clear(&list);
    while (hb_buffer_list_count(&pv->list) > 1)
    {
        buf  = hb_buffer_list_rem_head(&pv->list);
        next = hb_buffer_list_head(&pv->list);

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
        hb_buffer_t * out  = filterFrame(pv, NULL);
        hb_buffer_t * last = hb_buffer_list_tail(&pv->list);
        if (last != NULL && last->s.start != AV_NOPTS_VALUE)
        {
            last->s.stop = last->s.start + last->s.duration;
        }
        hb_buffer_list_prepend(&pv->list, out);
        hb_buffer_list_append(&pv->list, in);
        *buf_out = hb_buffer_list_clear(&pv->list);
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    *buf_out = filterFrame(pv, in);

    return HB_FILTER_OK;
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
            {
                settings = pv->avfilters;
            } break;
            case HB_FILTER_ROTATE:
            {
                settings = pv->avfilters;
            } break;
            case HB_FILTER_DEINTERLACE:
            {
                settings = pv->avfilters;
            } break;
            case HB_FILTER_PAD:
            {
                settings = pv->avfilters;
            } break;
            case HB_FILTER_CROP_SCALE:
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

void hb_append_filter_dict(hb_value_array_t * filters,
                           const char * name, hb_value_t * settings)
{
    hb_dict_t * filter_dict = hb_dict_init();

    hb_dict_set(filter_dict, name, settings);
    hb_value_array_append(filters, filter_dict);
}

static const char * color_format_range(enum AVPixelFormat format, int range)
{
    switch (format)
    {
        case AV_PIX_FMT_YUVJ420P:
        case AV_PIX_FMT_YUVJ422P:
        case AV_PIX_FMT_YUVJ444P:
        case AV_PIX_FMT_YUVJ440P:
            return "full";
        default:
            return range == AVCOL_RANGE_JPEG ? "full" : "limited";
    }
}

/* CropScale Settings
 *  mode:parity
 *
 *  width       - scale width
 *  height       - scale height
 *  crop-top    - top crop margin
 *  crop-bottom - bottom crop margin
 *  crop-left   - left crop margin
 *  crop-right  - right crop margin
 *
 */
static int crop_scale_init(hb_filter_object_t * filter, hb_filter_init_t * init)
{
    hb_filter_private_t * pv = NULL;

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;
    if (pv == NULL)
    {
        return 1;
    }
    pv->input = *init;

    hb_dict_t        * settings = filter->settings;
    hb_value_array_t * avfilters = hb_value_array_init();
    int                width, height, top, bottom, left, right;
    const char       * matrix;

    // Convert crop settings to 'crop' avfilter
    hb_dict_extract_int(&top, settings, "crop-top");
    hb_dict_extract_int(&bottom, settings, "crop-bottom");
    hb_dict_extract_int(&left, settings, "crop-left");
    hb_dict_extract_int(&right, settings, "crop-right");
    if (top > 0 || bottom > 0 || left > 0 || right > 0)
    {
        hb_dict_t * avfilter   = hb_dict_init();
        hb_dict_t * avsettings = hb_dict_init();

        hb_dict_set_int(avsettings, "x", left);
        hb_dict_set_int(avsettings, "y", top);
        hb_dict_set_int(avsettings, "w", init->geometry.width - left - right);
        hb_dict_set_int(avsettings, "h", init->geometry.height - top - bottom);
        hb_dict_set(avfilter, "crop", avsettings);
        hb_value_array_append(avfilters, avfilter);
    }

    // Convert scale settings to 'scale' avfilter
    hb_dict_extract_int(&width, settings, "width");
    hb_dict_extract_int(&height, settings, "height");

    hb_dict_t * avfilter   = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

    hb_dict_set_int(avsettings, "width", width);
    hb_dict_set_int(avsettings, "height", height);
    hb_dict_set_string(avsettings, "flags", "lanczos+accurate_rnd");
    switch (init->color_matrix)
    {
        case HB_COLR_MAT_BT709:
            matrix = "bt709";
            break;
        case HB_COLR_MAT_FCC:
            matrix = "fcc";
            break;
        case HB_COLR_MAT_SMPTE240M:
            matrix = "smpte240m";
            break;
        case HB_COLR_MAT_BT470BG:
        case HB_COLR_MAT_SMPTE170M:
            matrix = "smpte170m";
            break;
        case HB_COLR_MAT_BT2020_NCL:
        case HB_COLR_MAT_BT2020_CL:
            matrix = "bt2020";
            break;
        default:
        case HB_COLR_MAT_UNDEF:
            matrix = NULL;
            break;

    }
    if (matrix != NULL)
    {
        hb_dict_set_string(avsettings, "in_color_matrix", matrix);
        hb_dict_set_string(avsettings, "out_color_matrix", matrix);
    }
    hb_dict_set_string(avsettings, "in_range",
                       color_format_range(init->pix_fmt, init->color_range));
    hb_dict_set_string(avsettings, "out_range", "limited");
    hb_dict_set(avfilter, "scale", avsettings);
    hb_value_array_append(avfilters, avfilter);

    init->crop[0] = top;
    init->crop[1] = bottom;
    init->crop[2] = left;
    init->crop[3] = right;
    init->geometry.width = width;
    init->geometry.height = height;
    pv->output = *init;

    pv->avfilters = avfilters;

    return 0;
}

static hb_filter_info_t * crop_scale_info( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if (pv == NULL)
    {
        return NULL;
    }

    hb_filter_info_t * info;
    hb_dict_t        * settings = filter->settings;
    int                width, height, top, bottom, left, right;

    info = calloc(1, sizeof(hb_filter_info_t));
    if (info == NULL)
    {
        hb_error("crop_scale_info: allocation failure");
        return NULL;
    }
    info->output = pv->output;

    hb_dict_extract_int(&top, settings, "crop-top");
    hb_dict_extract_int(&bottom, settings, "crop-bottom");
    hb_dict_extract_int(&left, settings, "crop-left");
    hb_dict_extract_int(&right, settings, "crop-right");
    hb_dict_extract_int(&width, settings, "width");
    hb_dict_extract_int(&height, settings, "height");

    int cropped_width  = pv->input.geometry.width - (left + right);
    int cropped_height = pv->input.geometry.height - (top + bottom);

    info->human_readable_desc = hb_strdup_printf(
        "source: %d * %d, crop (%d/%d/%d/%d): %d * %d, scale: %d * %d",
        pv->input.geometry.width, pv->input.geometry.height,
        top, bottom, left, right,
        cropped_width, cropped_height, width, height);

    return info;
}

/* Pad presets and tunes
 *
 * There are currently no presets and tunes for pad
 * The custom pad string is converted to an avformat filter graph string
 */
static int pad_init(hb_filter_object_t * filter, hb_filter_init_t * init)
{
    hb_filter_private_t * pv = NULL;

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;
    if (pv == NULL)
    {
        return 1;
    }
    pv->input = *init;

    hb_dict_t        * settings = filter->settings;

    int      width  = -1, height = -1, rgb = 0;
    int      x = -1, y = -1;
    char  *  color = NULL;

    hb_dict_extract_int(&width, settings, "width");
    hb_dict_extract_int(&height, settings, "height");
    hb_dict_extract_string(&color, settings, "color");
    hb_dict_extract_int(&x, settings, "x");
    hb_dict_extract_int(&y, settings, "y");

    if (color != NULL)
    {
        char * end;
        rgb  = strtol(color, &end, 0);
        if (end == color)
        {
            // Not a numeric value, lookup by name
            rgb = hb_rgb_lookup_by_name(color);
        }
        free(color);
        color = hb_strdup_printf("0x%06x", rgb);
    }

    char x_str[20];
    char y_str[20];
    if (x < 0)
    {
        snprintf(x_str, 20, "(out_w-in_w)/2");
    }
    else
    {
        snprintf(x_str, 20, "%d", x);
    }
    if (y < 0)
    {
        snprintf(y_str, 20, "(out_h-in_h)/2");
    }
    else
    {
        snprintf(y_str, 20, "%d", y);
    }
    if (width < 0)
    {
        width = init->geometry.width;
    }
    if (height < 0)
    {
        height = init->geometry.height;
    }
    hb_dict_t * avfilter = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

    hb_dict_set(avsettings, "width", hb_value_int(width));
    hb_dict_set(avsettings, "height", hb_value_int(height));
    hb_dict_set(avsettings, "x", hb_value_string(x_str));
    hb_dict_set(avsettings, "y", hb_value_string(y_str));
    if (color != NULL)
    {
        hb_dict_set(avsettings, "color", hb_value_string(color));
        free(color);
    }
    hb_dict_set(avfilter, "pad", avsettings);
    pv->avfilters = avfilter;

    init->geometry.width = width;
    init->geometry.height = height;
    pv->output = *init;

    return 0;
}

/* Rotate Settings:
 *  degrees:mirror
 *
 *  degrees - Rotation angle, may be one of 90, 180, or 270
 *  mirror  - Mirror image around x axis
 *
 * Examples:
 * Mode 180:1 Mirror then rotate 180'
 * Mode   0:1 Mirror
 * Mode 180:0 Rotate 180'
 * Mode  90:0 Rotate 90'
 * Mode 270:0 Rotate 270'
 *
 * Legacy Mode Examples (also accepted):
 * Mode 1: Flip vertically (y0 becomes yN and yN becomes y0) (aka 180:1)
 * Mode 2: Flip horizontally (x0 becomes xN and xN becomes x0) (aka 0:1)
 * Mode 3: Flip both horizontally and vertically (aka 180:0)
 * Mode 4: Rotate 90' (aka 90:0)
 * Mode 7: Flip horiz & vert plus Rotate 90' (aka 270:0)
 */
static int rotate_init(hb_filter_object_t * filter, hb_filter_init_t * init)
{
    hb_filter_private_t * pv = NULL;

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;
    if (pv == NULL)
    {
        return 1;
    }
    pv->input = *init;

    hb_dict_t        * settings = filter->settings;

    int           width  = init->geometry.width;
    int           height = init->geometry.height;
    const char *  trans  = NULL;
    int           angle  = 0, flip = 0, hflip = 0, vflip = 0, tmp;

    hb_dict_extract_int(&angle, settings, "angle");
    hb_dict_extract_bool(&flip, settings, "hflip");

    const char * clock;
    const char * cclock;
    if (flip)
    {
        clock  = "clock_flip";
        cclock = "cclock_flip";
    }
    else
    {
        clock  = "clock";
        cclock = "cclock";
    }
    switch (angle)
    {
        case 0:
            hflip = flip;
            break;
        case 90:
            trans  = clock;
            tmp    = width;
            width  = height;
            height = tmp;
            break;
        case 180:
            vflip = 1;
            hflip = !flip;
            break;
        case 270:
            trans  = cclock;
            tmp    = width;
            width  = height;
            height = tmp;
            break;
        default:
            break;
    }
    if (trans != NULL)
    {
        hb_dict_t * avfilter = hb_dict_init();
        hb_dict_t * avsettings = hb_dict_init();

        hb_dict_set(avsettings, "dir", hb_value_string(trans));
        hb_dict_set(avfilter, "transpose", avsettings);
        pv->avfilters = avfilter;
    }
    else if (hflip || vflip)
    {
        hb_value_array_t * avfilters = hb_value_array_init();
        hb_dict_t        * avfilter;
        if (vflip)
        {
            avfilter = hb_dict_init();
            hb_dict_set(avfilter, "vflip", hb_value_null());
            hb_value_array_append(avfilters, avfilter);
        }
        if (hflip)
        {
            avfilter = hb_dict_init();
            hb_dict_set(avfilter, "hflip", hb_value_null());
            hb_value_array_append(avfilters, avfilter);
        }
        pv->avfilters = avfilter;
    }
    else
    {
        pv->avfilters = hb_value_null();
    }

    init->geometry.width = width;
    init->geometry.height = height;
    pv->output = *init;

    return 0;
}

/* Deinterlace Settings
 *  mode:parity
 *
 *  mode   - yadif deinterlace mode
 *  parity - field parity
 *
 *  Modes:
 *      1 = Enabled
 *      2 = Spatial
 *      4 = Bob
 *      8 = Selective
 *
 *  Parity:
 *      0  = Top Field First
 *      1  = Bottom Field First
 *      -1 = Automatic detection of field parity
 *
 */
static int deinterlace_init(hb_filter_object_t * filter,
                            hb_filter_init_t * init)
{
    hb_filter_private_t * pv = NULL;

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;
    if (pv == NULL)
    {
        return 1;
    }
    pv->input = *init;

    hb_dict_t        * settings = filter->settings;

    int          mode = 3, parity = -1;

    hb_dict_extract_int(&mode, settings, "mode");
    hb_dict_extract_int(&parity, settings, "parity");

    if (!(mode & MODE_YADIF_ENABLE))
    {
        return 0;
    }

    hb_dict_t * avfilter = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

    if (mode & MODE_YADIF_BOB)
    {
        if (mode & MODE_YADIF_SPATIAL)
        {
            hb_dict_set(avsettings, "mode", hb_value_string("send_field"));
        }
        else
        {
            hb_dict_set(avsettings, "mode",
                        hb_value_string("send_field_nospatial"));
        }
    }
    else
    {
        if (mode & MODE_YADIF_SPATIAL)
        {
            hb_dict_set(avsettings, "mode", hb_value_string("send_frame"));
        }
        else
        {
            hb_dict_set(avsettings, "mode",
                        hb_value_string("send_frame_nospatial"));
        }
    }

    if (mode & MODE_DECOMB_SELECTIVE)
    {
        hb_dict_set(avsettings, "deint", hb_value_string("interlaced"));
    }
    if (parity == 0)
    {
        hb_dict_set(avsettings, "parity", hb_value_string("tff"));
    }
    else if (parity == 1)
    {
        hb_dict_set(avsettings, "parity", hb_value_string("bff"));
    }
    hb_dict_set(avfilter, "yadif", avsettings);
    pv->avfilters = avfilter;

    pv->output = *init;

    return 0;
}

static int colorspace_init(hb_filter_object_t * filter, hb_filter_init_t * init)
{
    hb_filter_private_t * pv = NULL;

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;
    if (pv == NULL)
    {
        return 1;
    }
    pv->input = *init;

    hb_dict_t        * settings = filter->settings;

    char * format = NULL, * range = NULL;
    char * primaries = NULL, * transfer = NULL, * matrix = NULL;

    hb_dict_extract_string(&format, settings, "format");
    hb_dict_extract_string(&range, settings, "range");
    hb_dict_extract_string(&primaries, settings, "primaries");
    hb_dict_extract_string(&transfer, settings, "transfer");
    hb_dict_extract_string(&matrix, settings, "matrix");

    if (!(format || range || primaries || transfer || matrix))
    {
        return 0;
    }

    hb_dict_t * avfilter   = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

    if (format)
    {
        hb_dict_set_string(avsettings, "format", format);
    }
    if (range)
    {
        hb_dict_set_string(avsettings, "range", range);
    }
    if (primaries)
    {
        hb_dict_set_string(avsettings, "primaries", primaries);
    }
    if (transfer)
    {
        hb_dict_set_string(avsettings, "trc", transfer);
    }
    if (matrix)
    {
        hb_dict_set_string(avsettings, "space", matrix);
    }
    hb_dict_set(avfilter, "colorspace", avsettings);
    pv->avfilters = avfilter;

    pv->output = *init;

    return 0;
}
