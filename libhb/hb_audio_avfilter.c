/* hb_audio_avfilter.c

   Copyright (c) 2003-2026 HandBrake Team
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
#include "libavutil/channel_layout.h"
#include "libavutil/opt.h"
#include "handbrake/hb_audio_avfilter.h"

struct hb_audio_avfilter_graph_s
{
    AVFilterGraph    * avgraph;
    AVFilterContext  * input;
    AVFilterContext  * output;
    char             * settings;
    AVFrame          * frame;
    AVRational         out_time_base;
    int                out_channels;
};

hb_audio_avfilter_graph_t *
hb_audio_avfilter_graph_init(const char * settings, int sample_rate,
                              int sample_fmt, uint64_t channel_layout,
                              int channels, int allow_ch_change)
{
    hb_audio_avfilter_graph_t * graph;
    AVFilterInOut              * in = NULL, * out = NULL;
    char                       * filter_args;
    char                       * full_settings = NULL;
    int                          result;

    graph = calloc(1, sizeof(hb_audio_avfilter_graph_t));
    if (graph == NULL)
    {
        return NULL;
    }

    if (settings == NULL || settings[0] == '\0')
    {
        hb_error("hb_audio_avfilter_graph_init: no filter settings specified");
        goto fail;
    }

    graph->avgraph = avfilter_graph_alloc();
    if (graph->avgraph == NULL)
    {
        hb_error("hb_audio_avfilter_graph_init: avfilter_graph_alloc failed");
        goto fail;
    }

    // Build abuffer source filter args using AVChannelLayout API (FFmpeg 8+)
    {
        AVChannelLayout ch_layout = { 0 };
        char ch_layout_str[64];

        if (channel_layout != 0)
        {
            av_channel_layout_from_mask(&ch_layout, channel_layout);
        }
        else
        {
            av_channel_layout_default(&ch_layout, channels);
        }
        av_channel_layout_describe(&ch_layout, ch_layout_str,
                                    sizeof(ch_layout_str));
        av_channel_layout_uninit(&ch_layout);

        // Append aformat to ensure output matches what HB expects:
        // packed float, and optionally constrain the channel layout
        if (allow_ch_change)
        {
            full_settings = hb_strdup_printf(
                "%s,aformat=sample_fmts=flt", settings);
        }
        else
        {
            full_settings = hb_strdup_printf(
                "%s,aformat=sample_fmts=flt:channel_layouts=%s",
                settings, ch_layout_str);
        }
        graph->settings = strdup(full_settings);

        filter_args = hb_strdup_printf(
            "sample_rate=%d:sample_fmt=%s:channel_layout=%s"
            ":time_base=1/%d",
            sample_rate, av_get_sample_fmt_name(sample_fmt),
            ch_layout_str, sample_rate);
    }

    hb_log("hb_audio_avfilter_graph_init: abuffer args: %s", filter_args);

    result = avfilter_graph_create_filter(&graph->input,
                                           avfilter_get_by_name("abuffer"),
                                           "in", filter_args, NULL,
                                           graph->avgraph);
    free(filter_args);
    if (result < 0)
    {
        hb_error("hb_audio_avfilter_graph_init: failed to create abuffer source (%d)", result);
        goto fail;
    }

    // Parse filter settings and create the graph
    result = avfilter_graph_parse2(graph->avgraph, full_settings, &in, &out);
    if (result < 0)
    {
        hb_error("hb_audio_avfilter_graph_init: avfilter_graph_parse2 failed (%s)",
                 full_settings);
        goto fail;
    }

    // Link input → filter chain
    result = avfilter_link(graph->input, 0, in->filter_ctx, 0);
    if (result != 0)
    {
        hb_error("hb_audio_avfilter_graph_init: failed to link abuffer source");
        goto fail;
    }

    // Create abuffersink
    result = avfilter_graph_create_filter(&graph->output,
                                           avfilter_get_by_name("abuffersink"),
                                           "out", NULL, NULL, graph->avgraph);
    if (result < 0)
    {
        hb_error("hb_audio_avfilter_graph_init: failed to create abuffersink");
        goto fail;
    }

    // Link filter chain → output
    result = avfilter_link(out->filter_ctx, 0, graph->output, 0);
    if (result != 0)
    {
        hb_error("hb_audio_avfilter_graph_init: failed to link abuffersink");
        goto fail;
    }

    // Configure the graph
    result = avfilter_graph_config(graph->avgraph, NULL);
    if (result < 0)
    {
        char errbuf[256];
        av_strerror(result, errbuf, sizeof(errbuf));
        hb_error("hb_audio_avfilter_graph_init: failed to configure filter graph (%d: %s)",
                 result, errbuf);
        goto fail;
    }

    graph->frame = av_frame_alloc();
    if (graph->frame == NULL)
    {
        hb_error("hb_audio_avfilter_graph_init: failed to allocate AVFrame");
        goto fail;
    }

    graph->out_time_base = graph->output->inputs[0]->time_base;
    graph->out_channels  = graph->output->inputs[0]->ch_layout.nb_channels;

    free(full_settings);
    avfilter_inout_free(&in);
    avfilter_inout_free(&out);
    return graph;

fail:
    free(full_settings);
    avfilter_inout_free(&in);
    avfilter_inout_free(&out);
    hb_audio_avfilter_graph_close(&graph);
    return NULL;
}

void hb_audio_avfilter_graph_close(hb_audio_avfilter_graph_t ** _g)
{
    hb_audio_avfilter_graph_t * graph = *_g;

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

int hb_audio_avfilter_graph_get_out_channels(hb_audio_avfilter_graph_t * graph)
{
    return graph->out_channels;
}

int hb_audio_avfilter_add_buf(hb_audio_avfilter_graph_t * graph,
                              hb_buffer_t ** buf_in, int sample_rate,
                              int channels)
{
    int ret;

    if (buf_in != NULL && *buf_in != NULL)
    {
        hb_buffer_t * buf = *buf_in;
        AVFrame * frame = graph->frame;

        int nb_samples = buf->size / (sizeof(float) * channels);

        av_frame_unref(frame);
        frame->nb_samples     = nb_samples;
        frame->format         = AV_SAMPLE_FMT_FLT;
        frame->sample_rate    = sample_rate;
        av_channel_layout_default(&frame->ch_layout, channels);

        // Point frame data directly at buffer data (no copy needed for
        // interleaved format)
        frame->data[0]        = buf->data;
        frame->linesize[0]    = buf->size;
        frame->extended_data  = frame->data;

        // Convert 90kHz timestamps to filter time_base
        frame->pts = av_rescale_q(buf->s.start,
                                   (AVRational){1, 90000},
                                   (AVRational){1, sample_rate});

        ret = av_buffersrc_add_frame(graph->input, frame);

        // Don't unref since we didn't alloc the data
        frame->data[0]       = NULL;
        frame->extended_data = NULL;
        av_frame_unref(frame);

        hb_buffer_close(buf_in);
    }
    else
    {
        // Flush / EOF
        ret = av_buffersrc_add_frame(graph->input, NULL);
    }

    return ret;
}

hb_buffer_t * hb_audio_avfilter_get_buf(hb_audio_avfilter_graph_t * graph,
                                         int sample_rate, int channels)
{
    int result = av_buffersink_get_frame(graph->output, graph->frame);
    if (result >= 0)
    {
        AVFrame * frame = graph->frame;
        int nb_samples = frame->nb_samples;
        int size = nb_samples * sizeof(float) * channels;

        hb_buffer_t * buf = hb_buffer_init(size);
        if (buf == NULL)
        {
            av_frame_unref(frame);
            return NULL;
        }

        // Copy interleaved float data from frame to buffer
        memcpy(buf->data, frame->data[0], size);

        // Convert timestamps back to 90kHz
        buf->s.start = av_rescale_q(frame->pts,
                                     graph->out_time_base,
                                     (AVRational){1, 90000});
        int64_t duration = av_rescale_q(nb_samples,
                                         (AVRational){1, sample_rate},
                                         (AVRational){1, 90000});
        buf->s.stop = buf->s.start + duration;
        buf->s.type = AUDIO_BUF;

        av_frame_unref(frame);
        return buf;
    }

    return NULL;
}
