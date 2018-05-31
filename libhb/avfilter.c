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
#include "decomb.h"


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


static int  null_init( hb_filter_object_t * filter, hb_filter_init_t * init );
static void null_close( hb_filter_object_t * filter );
static int  null_work( hb_filter_object_t * filter,
                       hb_buffer_t ** buf_in, hb_buffer_t ** buf_out );
static hb_filter_info_t * null_info( hb_filter_object_t * filter );

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

const char pad_template[] =
    "width=^"HB_INT_REG"$:height=^"HB_INT_REG"$:color=^"HB_ALL_REG"$:"
    "x=^"HB_INT_REG"$:y=^"HB_INT_REG"$";

hb_filter_object_t hb_filter_pad =
{
    .id                = HB_FILTER_PAD,
    .enforce_order     = 1,
    .name              = "Pad",
    .settings          = NULL,
    .init              = null_init,
    .work              = null_work,
    .close             = null_close,
    .info              = null_info,
    .settings_template = pad_template,
};

const char rotate_template[] =
    "angle=^(0|90|180|270)$:hflip=^"HB_BOOL_REG"$:disable=^"HB_BOOL_REG"$";

hb_filter_object_t hb_filter_rotate =
{
    .id                = HB_FILTER_ROTATE,
    .enforce_order     = 1,
    .name              = "Rotate",
    .settings          = NULL,
    .init              = null_init,
    .work              = null_work,
    .close             = null_close,
    .info              = null_info,
    .settings_template = rotate_template,
};

const char deint_template[] =
    "mode=^"HB_INT_REG"$:parity=^([01])$";

hb_filter_object_t hb_filter_deinterlace =
{
    .id                = HB_FILTER_DEINTERLACE,
    .enforce_order     = 1,
    .name              = "Deinterlace",
    .settings          = NULL,
    .init              = null_init,
    .work              = null_work,
    .close             = null_close,
    .info              = null_info,
    .settings_template = deint_template,
};

static int null_init( hb_filter_object_t * filter, hb_filter_init_t * init )
{
    hb_log("null_init: It is an error to call this function.");
    return 1;
}

static void null_close( hb_filter_object_t * filter )
{
    hb_log("null_close: It is an error to call this function.");
    return;
}

static int  null_work( hb_filter_object_t * filter,
                       hb_buffer_t ** buf_in, hb_buffer_t ** buf_out )
{
    hb_log("null_work: It is an error to call this function.");
    return HB_WORK_DONE;
}

static hb_filter_info_t * null_info( hb_filter_object_t * filter )
{
    hb_log("null_info: It is an error to call this function.");
    return NULL;
}


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
    hb_filter_private_t * pv = NULL;
    char                * sws_flags;
    AVFilterContext     * avfilter;
    char                * filter_args;
    int                   result;
    AVFilterInOut       * in = NULL, * out = NULL;
    char                * avfilter_settings = NULL;

    avfilter_settings = hb_filter_settings_string(HB_FILTER_AVFILTER,
                                                  filter->settings);
    if (avfilter_settings == NULL)
    {
        hb_error("avfilter_init: no filter settings specified");
        return 1;
    }

    pv = calloc(1, sizeof(struct hb_filter_private_s));
    filter->private_data = pv;

    pv->settings = avfilter_settings;
    pv->graph = avfilter_graph_alloc();
    if (pv->graph == NULL)
    {
        hb_error("avfilter_init: avfilter_graph_alloc failed");
        goto fail;
    }

    sws_flags = hb_strdup_printf("flags=%d", SWS_LANCZOS|SWS_ACCURATE_RND);
    // avfilter_graph_free uses av_free to release scale_sws_opts.  Due
    // to the hacky implementation of av_free/av_malloc on windows,
    // you must av_malloc anything that is av_free'd.
    pv->graph->scale_sws_opts = av_malloc(strlen(sws_flags) + 1);
    strcpy(pv->graph->scale_sws_opts, sws_flags);
    free(sws_flags);

    result = avfilter_graph_parse2(pv->graph, avfilter_settings, &in, &out);
    if (result < 0 || in == NULL || out == NULL)
    {
        hb_error("avfilter_init: avfilter_graph_parse2 failed (%s)",
                 avfilter_settings);
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
    free(pv->settings);
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

    hb_buffer_list_close(&pv->list);
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
    frame->reordered_opaque = buf->s.duration;
    frame->width            = buf->f.width;
    frame->height           = buf->f.height;
    frame->format           = buf->f.fmt;
    frame->interlaced_frame = !!buf->s.combed;
    frame->top_field_first  = !!(buf->s.flags & PIC_FLAG_TOP_FIELD_FIRST);
}

static hb_buffer_t* filterFrame( hb_filter_private_t * pv, hb_buffer_t * in )
{
    int                result;
    hb_buffer_list_t   list;

    if (in != NULL)
    {
        fill_frame(pv, pv->frame, in);
        result = av_buffersrc_add_frame(pv->input, pv->frame);
    }
    else
    {
        result = av_buffersrc_add_frame(pv->input, NULL);
    }
    if (result < 0)
    {
        return NULL;
    }

    hb_buffer_list_clear(&list);
    result = av_buffersink_get_frame(pv->output, pv->frame);
    while (result >= 0)
    {
        hb_buffer_t * buf = hb_avframe_to_video_buffer(pv->frame,
                                                       pv->out_time_base);
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
static hb_dict_t *
convert_deint_settings(const hb_dict_t * settings)
{
    int          mode = 3, parity = -1;

    hb_dict_extract_int(&mode, settings, "mode");
    hb_dict_extract_int(&parity, settings, "parity");

    if (!(mode & MODE_YADIF_ENABLE))
    {
        return hb_value_null();
    }
    int automatic  = !!(mode & MODE_DECOMB_SELECTIVE);
    int bob        = !!(mode & MODE_YADIF_BOB);
    int no_spatial = !(mode & MODE_YADIF_SPATIAL);
    mode = bob | (no_spatial << 1);

    hb_dict_t * result = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

    hb_dict_set(avsettings, "mode", hb_value_int(mode));
    if (automatic)
    {
        hb_dict_set(avsettings, "auto", hb_value_int(automatic));
    }
    if (parity != -1)
    {
        hb_dict_set(avsettings, "parity", hb_value_int(parity));
    }
    hb_dict_set(result, "yadif", avsettings);

    return result;
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
static hb_dict_t *
convert_rotate_settings(const hb_dict_t * settings)
{
    const char *  trans = NULL;
    int           angle = 180, flip = 0, hflip = 0, vflip = 0;

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
            trans = clock;
            break;
        case 180:
            vflip = 1;
            hflip = !flip;
            break;
        case 270:
            trans = cclock;
            break;
        default:
            break;
    }
    if (trans != NULL)
    {
        hb_dict_t * result = hb_dict_init();
        hb_dict_t * avsettings = hb_dict_init();

        hb_dict_set(avsettings, "dir", hb_value_string(trans));
        hb_dict_set(result, "transpose", avsettings);

        return result;
    }
    else if (hflip || vflip)
    {
        hb_dict_t * result = hb_value_array_init();
        hb_dict_t * avfilter;
        if (vflip)
        {
            avfilter = hb_dict_init();
            hb_dict_set(avfilter, "vflip", hb_value_null());
            hb_value_array_append(result, avfilter);
        }
        if (hflip)
        {
            avfilter = hb_dict_init();
            hb_dict_set(avfilter, "hflip", hb_value_null());
            hb_value_array_append(result, avfilter);
        }
        return result;
    }
    else
    {
        return hb_value_null();
    }
}

/* Pad presets and tunes
 *
 * There are currently no presets and tunes for pad
 * The custom pad string is converted to an avformat filter graph string
 */
static hb_dict_t *
convert_pad_settings(const hb_dict_t * settings)
{
    int      width  = 0, height = 0, rgb = 0;
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
    hb_dict_t * result = hb_dict_init();
    hb_dict_t * avsettings = hb_dict_init();

    hb_dict_set(avsettings, "width", hb_value_int(width));
    hb_dict_set(avsettings, "height", hb_value_int(height));
    hb_dict_set(avsettings, "x", hb_value_string(x_str));
    hb_dict_set(avsettings, "y", hb_value_string(y_str));
    if (color != NULL)
    {
        hb_dict_set(avsettings, "color", hb_value_string(color));
    }
    hb_dict_set(result, "pad", avsettings);

    free(color);

    return result;
}

static hb_dict_t * convert_settings(int filter_id, hb_dict_t * settings)
{
    switch (filter_id)
    {
        case HB_FILTER_ROTATE:
            return convert_rotate_settings(settings);
        case HB_FILTER_DEINTERLACE:
            return convert_deint_settings(settings);
        case HB_FILTER_PAD:
            return convert_pad_settings(settings);
        default:
            return NULL;
    }
}

void hb_avfilter_combine( hb_list_t * list )
{
    hb_filter_object_t * avfilter = NULL;
    hb_value_t         * settings = NULL;
    int                  ii;

    for (ii = 0; ii < hb_list_count(list);)
    {
        hb_filter_object_t * filter = hb_list_item(list, ii);
        switch (filter->id)
        {
            case HB_FILTER_AVFILTER:
            {
                settings = hb_value_dup(filter->settings);
            } break;
            case HB_FILTER_ROTATE:
            case HB_FILTER_DEINTERLACE:
            case HB_FILTER_PAD:
            {
                settings = convert_settings(filter->id, filter->settings);
            } break;
            default:
                avfilter = NULL;
        }
        if (settings != NULL)
        {
            // Some filter values can result in no filter.
            // E.g. rotate angle=0:hflip=0
            if (hb_value_type(settings) == HB_VALUE_TYPE_NULL)
            {
                hb_list_rem(list, filter);
                hb_filter_close(&filter);
                hb_value_free(&settings);
                continue;
            }
            if (avfilter == NULL)
            {
                avfilter = hb_filter_init(HB_FILTER_AVFILTER);
                avfilter->settings = hb_value_array_init();
                hb_list_insert(list, ii, avfilter);
                ii++;
            }
            hb_list_rem(list, filter);
            hb_filter_close(&filter);

            hb_value_array_concat(avfilter->settings, settings);
            hb_value_free(&settings);
            continue;
        }
        ii++;
    }
}

char * hb_append_filter_string(char * graph_str, char * filter_str)
{
    char * tmp;
    int    size = 1, len = 0;

    if (graph_str != NULL)
    {
        len = strlen(graph_str);
        size += len + 1;
    }
    if (filter_str != NULL)
    {
        size += strlen(filter_str);
    }
    tmp = realloc(graph_str, size);
    if (tmp == NULL)
    {
        return graph_str;
    }
    graph_str = tmp;
    if (len > 0)
    {
        graph_str[len++] = ',';
    }
    strcpy(&graph_str[len], filter_str);
    return graph_str;
}

