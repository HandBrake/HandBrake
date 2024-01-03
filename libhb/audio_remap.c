/* audio_remap.c
 *
 * Copyright (c) 2003-2024 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/common.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/audio_remap.h"

// source: libavutil/channel_layout.h
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
    if (channel_map_in == NULL || channel_map_out == NULL)
    {
        hb_error("hb_audio_remap_init: invalid channel map(s)");
        goto fail;
    }
    remap->channel_map_in  = channel_map_in;
    remap->channel_map_out = channel_map_out;

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

        // sanitize the layout
        if (channel_layout == AV_CH_LAYOUT_STEREO_DOWNMIX)
        {
            channel_layout = AV_CH_LAYOUT_STEREO;
        }
        remap->nchannels = hb_layout_get_discrete_channel_count(channel_layout);

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
    nchannels = hb_layout_get_discrete_channel_count(channel_layout);

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
