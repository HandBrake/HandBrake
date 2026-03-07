/* audio_avfilter_work.c

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hb_audio_avfilter.h"
#include "libavutil/samplefmt.h"

struct hb_work_private_s
{
    hb_job_t                  * job;
    hb_audio_t                * audio;
    hb_audio_avfilter_graph_t * graph;
    int                         sample_rate;
    int                         in_channels;
    int                         out_channels;
    hb_buffer_list_t            list;
};

static int channels_to_mixdown(int channels)
{
    switch (channels)
    {
        case 1: return HB_AMIXDOWN_MONO;
        case 2: return HB_AMIXDOWN_STEREO;
        case 3: return HB_AMIXDOWN_3POINT0;
        case 6: return HB_AMIXDOWN_5POINT1;
        case 7: return HB_AMIXDOWN_6POINT1;
        case 8: return HB_AMIXDOWN_7POINT1;
        default: return -1;
    }
}

static int audio_avfilter_init(hb_work_object_t * w, hb_job_t * job)
{
    hb_work_private_t * pv = calloc(1, sizeof(hb_work_private_t));
    w->private_data = pv;

    pv->job   = job;
    pv->audio = w->audio;

    pv->sample_rate  = pv->audio->config.out.samplerate;
    pv->in_channels  = hb_mixdown_get_discrete_channel_count(
                       pv->audio->config.out.mixdown);
    pv->out_channels = pv->in_channels;

    hb_buffer_list_clear(&pv->list);

    const char * filter_settings = pv->audio->config.out.avfilter;
    if (filter_settings == NULL || filter_settings[0] == '\0')
    {
        hb_error("audio_avfilter_init: no filter settings");
        return 1;
    }

    hb_log("audio_avfilter_init: track %d, filter: %s",
           pv->audio->config.out.track, filter_settings);

    // First pass: create graph without channel layout constraint
    // to detect if the filter changes the channel count
    pv->graph = hb_audio_avfilter_graph_init(
        filter_settings, pv->sample_rate, AV_SAMPLE_FMT_FLT,
        0, pv->in_channels, 1);

    if (pv->graph == NULL)
    {
        hb_error("audio_avfilter_init: failed to create audio filter graph");
        return 1;
    }

    int graph_out_channels = hb_audio_avfilter_graph_get_out_channels(pv->graph);

    if (graph_out_channels != pv->in_channels)
    {
        int new_mixdown = channels_to_mixdown(graph_out_channels);
        uint32_t codec  = pv->audio->config.out.codec;

        if (new_mixdown >= 0 &&
            hb_mixdown_has_codec_support(new_mixdown, codec))
        {
            // Filter changed channels to a valid, supported mixdown
            hb_log("audio_avfilter_init: channel change %d -> %d "
                   "(mixdown: %s)",
                   pv->in_channels, graph_out_channels,
                   hb_mixdown_get_short_name(new_mixdown));

            pv->audio->config.out.mixdown = new_mixdown;
            pv->out_channels = graph_out_channels;

            // Bump bitrate if current is below minimum for new mixdown
            int low, high;
            hb_audio_bitrate_get_limits(codec, pv->sample_rate,
                                        new_mixdown, &low, &high);
            if (pv->audio->config.out.bitrate < low)
            {
                int new_bitrate = hb_audio_bitrate_get_default(
                    codec, pv->sample_rate, new_mixdown);
                hb_log("audio_avfilter_init: bitrate %d below minimum %d "
                       "for new mixdown, bumping to %d",
                       pv->audio->config.out.bitrate, low, new_bitrate);
                pv->audio->config.out.bitrate = new_bitrate;
            }
        }
        else
        {
            // Invalid channel count or unsupported mixdown — recreate
            // graph with channel layout constraint to force input layout
            hb_log("audio_avfilter_init: filter output %d channels "
                   "(unsupported), constraining to input layout",
                   graph_out_channels);
            hb_audio_avfilter_graph_close(&pv->graph);
            pv->graph = hb_audio_avfilter_graph_init(
                filter_settings, pv->sample_rate, AV_SAMPLE_FMT_FLT,
                0, pv->in_channels, 0);
            if (pv->graph == NULL)
            {
                hb_error("audio_avfilter_init: failed to create "
                         "constrained audio filter graph");
                return 1;
            }
        }
    }

    return 0;
}

static int audio_avfilter_work(hb_work_object_t * w,
                               hb_buffer_t ** buf_in,
                               hb_buffer_t ** buf_out)
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        // Flush the filter graph
        hb_audio_avfilter_add_buf(pv->graph, NULL,
                                   pv->sample_rate, pv->in_channels);

        hb_buffer_t * buf;
        while ((buf = hb_audio_avfilter_get_buf(pv->graph,
                                                pv->sample_rate,
                                                pv->out_channels)) != NULL)
        {
            hb_buffer_list_append(&pv->list, buf);
        }

        hb_buffer_list_append(&pv->list, in);
        *buf_in  = NULL;
        *buf_out = hb_buffer_list_clear(&pv->list);
        return HB_WORK_DONE;
    }

    // Feed input buffer to filter graph
    hb_audio_avfilter_add_buf(pv->graph, buf_in,
                              pv->sample_rate, pv->in_channels);

    // Retrieve all available output
    hb_buffer_t * buf;
    while ((buf = hb_audio_avfilter_get_buf(pv->graph,
                                            pv->sample_rate,
                                            pv->out_channels)) != NULL)
    {
        hb_buffer_list_append(&pv->list, buf);
    }

    *buf_out = hb_buffer_list_clear(&pv->list);
    return HB_WORK_OK;
}

static void audio_avfilter_close(hb_work_object_t * w)
{
    hb_work_private_t * pv = w->private_data;

    if (pv == NULL)
    {
        return;
    }

    hb_audio_avfilter_graph_close(&pv->graph);
    free(pv);
    w->private_data = NULL;
}

hb_work_object_t hb_audio_avfilter_work =
{
    .id    = WORK_AUDIO_AVFILTER,
    .name  = "Audio AVFilter",
    .init  = audio_avfilter_init,
    .work  = audio_avfilter_work,
    .close = audio_avfilter_close,
};
