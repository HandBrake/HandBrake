/* audio_remap.c
 *
 * Copyright (c) 2003-2012 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "common.h"
#include "hbffmpeg.h"
#include "audio_remap.h"

// source: libavutil/audioconvert.h
hb_chan_map_t hb_libav_chan_map =
{
    {
        AV_CH_FRONT_LEFT,
        AV_CH_FRONT_RIGHT,
        AV_CH_FRONT_CENTER,
        AV_CH_LOW_FREQUENCY,
        AV_CH_BACK_LEFT,
        AV_CH_BACK_RIGHT,
        AV_CH_FRONT_LEFT_OF_CENTER,
        AV_CH_FRONT_RIGHT_OF_CENTER,
        AV_CH_BACK_CENTER,
        AV_CH_SIDE_LEFT,
        AV_CH_SIDE_RIGHT,
        0
    }
};

// source: liba52 documentation
hb_chan_map_t hb_liba52_chan_map =
{
    {
        AV_CH_LOW_FREQUENCY,
        AV_CH_FRONT_LEFT,
        AV_CH_FRONT_CENTER,
        AV_CH_FRONT_RIGHT,
        AV_CH_BACK_CENTER,
        AV_CH_SIDE_LEFT,
        AV_CH_SIDE_RIGHT,
        0
    }
};

// source: http://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-800004.3.9
hb_chan_map_t hb_vorbis_chan_map =
{
    {
        AV_CH_FRONT_LEFT,
        AV_CH_FRONT_CENTER,
        AV_CH_FRONT_RIGHT,
        AV_CH_SIDE_LEFT,
        AV_CH_SIDE_RIGHT,
        AV_CH_BACK_LEFT,
        AV_CH_BACK_CENTER,
        AV_CH_BACK_RIGHT,
        AV_CH_LOW_FREQUENCY,
        0
    }
};

// source: https://developer.apple.com/library/mac/#documentation/musicaudio/reference/CoreAudioDataTypesRef/Reference/reference.html
hb_chan_map_t hb_aac_chan_map =
{
    {
        AV_CH_FRONT_CENTER,
        AV_CH_FRONT_LEFT_OF_CENTER,
        AV_CH_FRONT_RIGHT_OF_CENTER,
        AV_CH_FRONT_LEFT,
        AV_CH_FRONT_RIGHT,
        AV_CH_SIDE_LEFT,
        AV_CH_SIDE_RIGHT,
        AV_CH_BACK_LEFT,
        AV_CH_BACK_RIGHT,
        AV_CH_BACK_CENTER,
        AV_CH_LOW_FREQUENCY,
        0
    }
};

static void remap_planar(uint8_t *tmp_buf, uint8_t *samples, int nsamples,
                         int nchannels, int sample_size, int *remap_table)
{
    int ii, stride = nsamples * sample_size;
    memcpy(tmp_buf, samples, nchannels * stride);
    for (ii = 0; ii < nchannels; ii++)
    {
        memcpy(samples + (ii              * stride),
               tmp_buf + (remap_table[ii] * stride), stride);
    }
}

static void remap_interleaved(uint8_t *tmp_buf, uint8_t *samples, int nsamples,
                              int nchannels, int sample_size, int *remap_table)
{
    int ii, jj, stride = nchannels * sample_size;
    memcpy(tmp_buf, samples, nsamples * stride);
    for (ii = 0; ii < nsamples; ii++)
    {
        for (jj = 0; jj < nchannels; jj++)
        {
            memcpy(samples + (jj              * sample_size),
                   tmp_buf + (remap_table[jj] * sample_size), sample_size);
        }
        samples += stride;
        tmp_buf += stride;
    }
}

hb_audio_remap_t* hb_audio_remap_init(enum AVSampleFormat sample_fmt,
                                      hb_chan_map_t *channel_map_out,
                                      hb_chan_map_t *channel_map_in)
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
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_DBL:
            remap->remap = &remap_interleaved;
            break;

        default:
            hb_error("hb_audio_remap_init: unsupported sample format '%s'",
                     av_get_sample_fmt_name(sample_fmt));
            goto fail;
    }
    remap->sample_size = av_get_bytes_per_sample(sample_fmt);

    // input/output channel order
    if (channel_map_in == NULL || channel_map_out == NULL)
    {
        hb_error("hb_audio_remap_init: invalid channel map(s)");
        goto fail;
    }
    remap->channel_map_in  = channel_map_in;
    remap->channel_map_out = channel_map_out;

    // temp buffer - we don't know the required size yet
    remap->buf = hb_buffer_init(0);
    if (remap->buf == NULL)
    {
        hb_error("hb_audio_remap_init: failed to allocate remap->buf");
        goto fail;
    }

    // remap can't be done until the channel layout has been set
    remap->remap_needed = 0;

    return remap;

fail:
    hb_audio_remap_free(remap);
    return NULL;
}

void hb_audio_remap_set_channel_layout(hb_audio_remap_t *remap,
                                       uint64_t channel_layout)
{
    if (remap != NULL)
    {
        int ii;
        remap->remap_needed = 0;
        remap->nchannels    = av_get_channel_layout_nb_channels(channel_layout);

        // in some cases, remapping is not necessary and/or supported
        if (remap->nchannels > HB_AUDIO_REMAP_MAX_CHANNELS)
        {
            hb_log("hb_audio_remap_set_channel_layout: too many channels (%d)",
                   remap->nchannels);
            return;
        }
        if (remap->channel_map_in == remap->channel_map_out)
        {
            return;
        }

        // build the table and check whether remapping is necessary
        hb_audio_remap_build_table(remap->channel_map_out,
                                   remap->channel_map_in, channel_layout,
                                   remap->table);
        for (ii = 0; ii < remap->nchannels; ii++)
        {
            if (remap->table[ii] != ii)
            {
                remap->remap_needed = 1;
                break;
            }
        }
    }
}


void hb_audio_remap_free(hb_audio_remap_t *remap)
{
    if (remap != NULL)
    {
        if (remap->buf != NULL)
            hb_buffer_close(&remap->buf);
        free(remap);
    }
}

void hb_audio_remap(hb_audio_remap_t *remap, uint8_t *samples, int nsamples)
{
    if (remap != NULL && remap->remap_needed)
    {
        // make sure our temp buffer can hold a copy of all samples
        hb_buffer_realloc(remap->buf, nsamples * remap->sample_size *
                          remap->nchannels);
        remap->remap(remap->buf->data, samples, nsamples, remap->nchannels,
                     remap->sample_size, remap->table);
    }
}

void hb_audio_remap_build_table(hb_chan_map_t *channel_map_out,
                                hb_chan_map_t *channel_map_in,
                                uint64_t channel_layout,
                                int *remap_table)
{
    int ii, jj, nchannels, out_chan_idx, remap_idx;
    uint64_t *channels_in, *channels_out;

    if (channel_layout == AV_CH_LAYOUT_STEREO_DOWNMIX)
    {
        // Dolby Surround is Stereo when it comes to remapping
        channel_layout = AV_CH_LAYOUT_STEREO;
    }
    nchannels = av_get_channel_layout_nb_channels(channel_layout);

    // clear remap table before (re-)building it
    memset(remap_table, 0, nchannels * sizeof(int));

    out_chan_idx = 0;
    channels_in  = channel_map_in ->channel_order_map;
    channels_out = channel_map_out->channel_order_map;
    for (ii = 0; channels_out[ii] && out_chan_idx < nchannels; ii++)
    {
        if (channel_layout & channels_out[ii])
        {
            remap_idx = 0;
            for (jj = 0; channels_in[jj] && remap_idx < nchannels; jj++)
            {
                if (channels_out[ii] == channels_in[jj])
                {
                    remap_table[out_chan_idx++] = remap_idx++;
                    break;
                }
                else if (channel_layout & channels_in[jj])
                {
                    remap_idx++;
                }
            }
        }
    }
}
