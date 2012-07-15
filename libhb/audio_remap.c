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

hb_audio_remap_t* hb_audio_remap_init(uint64_t channel_layout,
                                      hb_chan_map_t *map_out,
                                      hb_chan_map_t *map_in)
{
    hb_audio_remap_t *remap = malloc(sizeof(hb_audio_remap_t));
    if (remap == NULL)
        return NULL;

    remap->remap_table = hb_audio_remap_build_table(channel_layout,
                                                    map_out, map_in);
    if (remap->remap_table == NULL)
    {
        hb_audio_remap_free(remap);
        return NULL;
    }

    int ii;
    remap->nchannels    = av_get_channel_layout_nb_channels(channel_layout);
    remap->sample_size  = remap->nchannels * sizeof(hb_sample_t);
    remap->remap_needed = 0;
    for (ii = 0; ii < remap->nchannels; ii++)
    {
        if (remap->remap_table[ii] != ii)
        {
            remap->remap_needed = 1;
            break;
        }
    }

    return remap;
}

void hb_audio_remap_free(hb_audio_remap_t *remap)
{
    if (remap != NULL)
    {
        if (remap->remap_table != NULL)
        {
            free(remap->remap_table);
        }
        free(remap);
    }
}

void hb_audio_remap(hb_audio_remap_t *remap, hb_sample_t *samples, int nsamples)
{
    if (remap != NULL && remap->remap_needed)
    {
        int ii, jj;

        for (ii = 0; ii < nsamples; ii++)
        {
            memcpy(remap->tmp, samples, remap->sample_size);
            for (jj = 0; jj < remap->nchannels; jj++)
            {
                samples[jj] = remap->tmp[remap->remap_table[jj]];
            }
            samples += remap->nchannels;
        }
    }
}

int* hb_audio_remap_build_table(uint64_t channel_layout,
                                hb_chan_map_t *map_out,
                                hb_chan_map_t *map_in)
{
    int ii, jj, nchannels, out_chan_idx, remap_idx, *remap_table;
    uint64_t *channels_in, *channels_out;

    nchannels = av_get_channel_layout_nb_channels(channel_layout);
    remap_table = malloc(nchannels * sizeof(int));
    if (remap_table == NULL)
        return NULL;

    out_chan_idx = 0;
    channels_in  = map_in->channel_order_map;
    channels_out = map_out->channel_order_map;
    for (ii = 0; channels_out[ii] && out_chan_idx < nchannels; ii++)
    {
        if (channel_layout & channels_out[ii])
        {
            remap_idx = 0;
            for (jj = 0; channels_in[jj]; jj++)
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

    return remap_table;
}
