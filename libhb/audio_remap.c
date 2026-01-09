/* audio_remap.c
 *
 * Copyright (c) 2003-2026 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/audio_remap.h"

// source: http://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-800004.3.9
hb_chan_map_t hb_vorbis_chan_map =
{
    {
        AV_CHAN_FRONT_LEFT,
        AV_CHAN_FRONT_CENTER,
        AV_CHAN_FRONT_RIGHT,
        AV_CHAN_SIDE_LEFT,
        AV_CHAN_SIDE_RIGHT,
        AV_CHAN_BACK_LEFT,
        AV_CHAN_BACK_CENTER,
        AV_CHAN_BACK_RIGHT,
        AV_CHAN_LOW_FREQUENCY,
        AV_CHAN_NONE
    }
};

// source: https://developer.apple.com/library/mac/#documentation/musicaudio/reference/CoreAudioDataTypesRef/Reference/reference.html
hb_chan_map_t hb_aac_chan_map =
{
    {
        AV_CHAN_FRONT_CENTER,
        AV_CHAN_FRONT_LEFT_OF_CENTER,
        AV_CHAN_FRONT_RIGHT_OF_CENTER,
        AV_CHAN_FRONT_LEFT,
        AV_CHAN_FRONT_RIGHT,
        AV_CHAN_SIDE_LEFT,
        AV_CHAN_SIDE_RIGHT,
        AV_CHAN_BACK_LEFT,
        AV_CHAN_BACK_RIGHT,
        AV_CHAN_BACK_CENTER,
        AV_CHAN_LOW_FREQUENCY,
        AV_CHAN_NONE
    }
};

void hb_audio_remap_map_channel_layout(hb_chan_map_t *map,
                                       AVChannelLayout *ch_layout_out,
                                       const AVChannelLayout *ch_layout_in)
{
    int nchannels, out_chan_idx;
    uint64_t *channels_out;

    nchannels = ch_layout_in->nb_channels;
    av_channel_layout_custom_init(ch_layout_out, nchannels);

    out_chan_idx = 0;
    channels_out = map->channel_order_map;
    for (int ii = 0; channels_out[ii] != AV_CHAN_NONE && out_chan_idx < nchannels; ii++)
    {
        enum AVChannel in_channel = channels_out[ii];
        int in_chan_idx = av_channel_layout_index_from_channel(ch_layout_in, in_channel);

        if (in_chan_idx > -1)
        {
            ch_layout_out->u.map[out_chan_idx].id = in_channel;
            out_chan_idx++;
        }
    }
}

static void remap_planar(uint8_t **samples, int nsamples,
                         int nchannels, int *remap_table)
{
    int ii;
    uint8_t *tmp_buf[HB_AUDIO_REMAP_MAX_CHANNELS];
    memcpy(tmp_buf, samples, nchannels * sizeof(uint8_t*));
    for (ii = 0; ii < nchannels; ii++)
    {
        samples[ii] = tmp_buf[remap_table[ii]];
    }
}

static void remap_u8_interleaved(uint8_t **samples, int nsamples,
                                 int nchannels, int *remap_table)
{
    int ii, jj;
    uint8_t *samples_u8 = (*samples);
    uint8_t tmp_buf[HB_AUDIO_REMAP_MAX_CHANNELS];
    for (ii = 0; ii < nsamples; ii++)
    {
        memcpy(tmp_buf, samples_u8, nchannels * sizeof(uint8_t));
        for (jj = 0; jj < nchannels; jj++)
        {
            samples_u8[jj] = tmp_buf[remap_table[jj]];
        }
        samples_u8 += nchannels;
    }
}

static void remap_s16_interleaved(uint8_t **samples, int nsamples,
                                  int nchannels, int *remap_table)
{
    int ii, jj;
    int16_t *samples_s16 = (int16_t*)(*samples);
    int16_t tmp_buf[HB_AUDIO_REMAP_MAX_CHANNELS];
    for (ii = 0; ii < nsamples; ii++)
    {
        memcpy(tmp_buf, samples_s16, nchannels * sizeof(int16_t));
        for (jj = 0; jj < nchannels; jj++)
        {
            samples_s16[jj] = tmp_buf[remap_table[jj]];
        }
        samples_s16 += nchannels;
    }
}

static void remap_s32_interleaved(uint8_t **samples, int nsamples,
                                  int nchannels, int *remap_table)
{
    int ii, jj;
    int32_t *samples_s32 = (int32_t*)(*samples);
    int32_t tmp_buf[HB_AUDIO_REMAP_MAX_CHANNELS];
    for (ii = 0; ii < nsamples; ii++)
    {
        memcpy(tmp_buf, samples_s32, nchannels * sizeof(int32_t));
        for (jj = 0; jj < nchannels; jj++)
        {
            samples_s32[jj] = tmp_buf[remap_table[jj]];
        }
        samples_s32 += nchannels;
    }
}

static void remap_flt_interleaved(uint8_t **samples, int nsamples,
                                  int nchannels, int *remap_table)
{
    int ii, jj;
    float *samples_flt = (float*)(*samples);
    float tmp_buf[HB_AUDIO_REMAP_MAX_CHANNELS];
    for (ii = 0; ii < nsamples; ii++)
    {
        memcpy(tmp_buf, samples_flt, nchannels * sizeof(float));
        for (jj = 0; jj < nchannels; jj++)
        {
            samples_flt[jj] = tmp_buf[remap_table[jj]];
        }
        samples_flt += nchannels;
    }
}

static void remap_dbl_interleaved(uint8_t **samples, int nsamples,
                                  int nchannels, int *remap_table)
{
    int ii, jj;
    double *samples_dbl = (double*)(*samples);
    double tmp_buf[HB_AUDIO_REMAP_MAX_CHANNELS];
    for (ii = 0; ii < nsamples; ii++)
    {
        memcpy(tmp_buf, samples_dbl, nchannels * sizeof(double));
        for (jj = 0; jj < nchannels; jj++)
        {
            samples_dbl[jj] = tmp_buf[remap_table[jj]];
        }
        samples_dbl += nchannels;
    }
}

hb_audio_remap_t* hb_audio_remap_init(enum AVSampleFormat sample_fmt,
                                      const AVChannelLayout *ch_layout_out,
                                      const AVChannelLayout *ch_layout_in)
{
    hb_audio_remap_t *remap = calloc(1, sizeof(hb_audio_remap_t));
    if (remap == NULL)
    {
        hb_error("hb_audio_remap_init: failed to allocate remap");
        goto fail;
    }

    // sample format
    switch (sample_fmt)
    {
        case AV_SAMPLE_FMT_U8P:
        case AV_SAMPLE_FMT_S16P:
        case AV_SAMPLE_FMT_S32P:
        case AV_SAMPLE_FMT_FLTP:
        case AV_SAMPLE_FMT_DBLP:
            remap->remap = &remap_planar;
            break;

        case AV_SAMPLE_FMT_U8:
            remap->remap = &remap_u8_interleaved;
            break;

        case AV_SAMPLE_FMT_S16:
            remap->remap = &remap_s16_interleaved;
            break;

        case AV_SAMPLE_FMT_S32:
            remap->remap = &remap_s32_interleaved;
            break;

        case AV_SAMPLE_FMT_FLT:
            remap->remap = &remap_flt_interleaved;
            break;

        case AV_SAMPLE_FMT_DBL:
            remap->remap = &remap_dbl_interleaved;
            break;

        default:
            hb_error("hb_audio_remap_init: unsupported sample format '%s'",
                     av_get_sample_fmt_name(sample_fmt));
            goto fail;
    }

    // input/output channel order
    if (ch_layout_in == NULL || ch_layout_out == NULL)
    {
        hb_error("hb_audio_remap_init: invalid channel map(s)");
        goto fail;
    }
    av_channel_layout_copy(&remap->ch_layout_in, ch_layout_in);
    av_channel_layout_copy(&remap->ch_layout_out, ch_layout_out);

    remap->remap_needed = 0;
    remap->nchannels = ch_layout_in->nb_channels;

    // sanitize the layout
    AVChannelLayout stereo_dowmix = AV_CHANNEL_LAYOUT_STEREO_DOWNMIX;
    if (av_channel_layout_compare(ch_layout_in, &stereo_dowmix) == 0)
    {
        // Dolby Surround is Stereo when it comes to remapping
        AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
        av_channel_layout_copy(&remap->ch_layout_in, &stereo);
    }

    // in some cases, remapping is not necessary and/or supported
    if (remap->nchannels > HB_AUDIO_REMAP_MAX_CHANNELS)
    {
        hb_log("hb_audio_remap_init: too many channels (%d)",
               remap->nchannels);
        goto fail;
    }
    if (av_channel_layout_compare(&remap->ch_layout_in, &remap->ch_layout_out) != 0)
    {
        // build the table and check whether remapping is necessary
        hb_audio_remap_build_table(&remap->ch_layout_out,
                                   &remap->ch_layout_in,
                                   remap->table);
        for (int ii = 0; ii < remap->nchannels; ii++)
        {
            if (remap->table[ii] != ii)
            {
                remap->remap_needed = 1;
                break;
            }
        }
    }

    return remap;

fail:
    hb_audio_remap_free(remap);
    return NULL;
}

void hb_audio_remap_free(hb_audio_remap_t *remap)
{
    if (remap != NULL)
    {
        av_channel_layout_uninit(&remap->ch_layout_out);
        av_channel_layout_uninit(&remap->ch_layout_in);
        free(remap);
    }
}

void hb_audio_remap(hb_audio_remap_t *remap, uint8_t **samples, int nsamples)
{
    if (remap != NULL && remap->remap_needed)
    {
        remap->remap(samples, nsamples, remap->nchannels, remap->table);
    }
}

void hb_audio_remap_build_table(AVChannelLayout *ch_layout_out,
                                AVChannelLayout *ch_layout_in,
                                int *remap_table)
{
    int nchannels = ch_layout_in->nb_channels;

    // clear remap table before (re-)building it
    memset(remap_table, 0, nchannels * sizeof(int));

    for (int ii = 0; ii < nchannels; ii++)
    {
        enum AVChannel out_channel = av_channel_layout_channel_from_index(ch_layout_out, ii);
        for (int jj = 0; jj < nchannels; jj++)
        {
            enum AVChannel in_channel = av_channel_layout_channel_from_index(ch_layout_in, jj);
            if (out_channel == in_channel)
            {
                remap_table[ii] = jj;
                break;
            }
        }
    }
}
