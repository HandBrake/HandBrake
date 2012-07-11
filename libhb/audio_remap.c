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

// source: libdca documentation and libavcodec/dca.c
hb_chan_map_t hb_libdca_chan_map =
{
    {
        AV_CH_FRONT_LEFT_OF_CENTER,
        AV_CH_FRONT_CENTER,
        AV_CH_FRONT_RIGHT_OF_CENTER,
        AV_CH_FRONT_LEFT,
        AV_CH_FRONT_RIGHT,
        AV_CH_SIDE_LEFT,
        AV_CH_SIDE_RIGHT,
        AV_CH_LOW_FREQUENCY,
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

int* hb_audio_remap_build_table(uint64_t layout, hb_chan_map_t *map_in, hb_chan_map_t *map_out)
{
    int ii, jj, idx, remap_idx, *remap_table;
    uint64_t *input_order, *output_order;

    remap_table = calloc(HB_AUDIO_REMAP_MAX_CHANNELS, sizeof(int));
    if (!remap_table)
        return NULL;

    idx          = 0;
    input_order  = map_in->channel_order;
    output_order = map_out->channel_order;
    for (ii = 0; output_order[ii]; ii++)
    {
        if (layout & output_order[ii])
        {
            remap_idx = 0;
            for (jj = 0; input_order[jj]; jj++)
            {
                if (output_order[ii] == input_order[jj])
                {
                    remap_table[idx++] = remap_idx++;
                }
                else if (layout & input_order[jj])
                {
                    remap_idx++;
                }
            }
        }
    }

    return remap_table;
}

void hb_audio_remap(int nchannels, int nsamples, hb_sample_t *samples, int *remap_table)
{
    int ii, jj;
    hb_sample_t tmp[HB_AUDIO_REMAP_MAX_CHANNELS];

    for (ii = 0; ii < nsamples; ii++)
    {
        memcpy(tmp, samples, nchannels * sizeof(hb_sample_t));
        for (jj = 0; jj < nchannels; jj++)
        {
            samples[jj] = tmp[remap_table[jj]];
        }
        samples += nchannels;
    }
}