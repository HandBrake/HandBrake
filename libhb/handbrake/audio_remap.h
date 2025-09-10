/* audio_remap.h
 *
 * Copyright (c) 2003-2025 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/* This file handles remapping audio channels
 *
 * We only need to support:
 *
 * a) channels found in our non-libavcodec audio decoders' layouts
 * b) channels found in HB_AMIXDOWN_* layouts
 *
 * We consider that:
 *
 * Left/Right Surround == Side Left/Right
 * Left/Right Rear Surround == Back Left/Right */

#ifndef HANDBRAKE_AUDIO_REMAP_H
#define HANDBRAKE_AUDIO_REMAP_H

#include <stdint.h>
#include "libavutil/channel_layout.h"
#include "libavutil/samplefmt.h"

/* we only need to support the 11 "most common" channels */
#define HB_AUDIO_REMAP_MAX_CHANNELS 11

typedef struct
{
    uint64_t channel_order_map[HB_AUDIO_REMAP_MAX_CHANNELS + 1];
} hb_chan_map_t;

/*
 * Predefined channel maps for common channel orders.
 */
extern hb_chan_map_t hb_vorbis_chan_map;
extern hb_chan_map_t hb_aac_chan_map;

/*
 * Configures a AVChannelLayout with a hb_chan_map_t.
*/
void             hb_audio_remap_map_channel_layout(hb_chan_map_t *map,
                                                   AVChannelLayout *ch_layout_out,
                                                   const AVChannelLayout *ch_layout_in);

typedef struct
{
    int nchannels;
    int remap_needed;
    AVChannelLayout ch_layout_in;
    AVChannelLayout ch_layout_out;
    int table[HB_AUDIO_REMAP_MAX_CHANNELS];

    void (*remap)(uint8_t **samples, int nsamples,
                  int nchannels, int *remap_table);
} hb_audio_remap_t;

/*
 * Initialize an hb_audio_remap_t to remap audio with the specified sample
 * format, from the input to the output channel layout (indicated by
 * ch_layout_in and ch_layout_out, respectively).
 */
hb_audio_remap_t* hb_audio_remap_init(enum AVSampleFormat sample_fmt,
                                      const AVChannelLayout *ch_layout_out,
                                      const AVChannelLayout *ch_layout_in);

/*
 * Free an hb_audio_remap_t.
 */
void              hb_audio_remap_free(hb_audio_remap_t *remap);

/*
 * Remap audio between 2 different channel orders, using the settings specified
 * in the remap parameter. Remapping is only done when necessary.
 *
 * The remap parameter can be NULL (no remapping).
 */
void              hb_audio_remap(hb_audio_remap_t *remap, uint8_t **samples,
                                 int nsamples);

/*
 * Generate a table used to remap audio between 2 different channel orders.
 *
 * Usage: output_sample[channel_idx] = input_sample[remap_table[channel_idx]]
 *
 * remap_table is allocated by the caller.
 */
void              hb_audio_remap_build_table(AVChannelLayout *ch_layout_out,
                                             AVChannelLayout *ch_layout_in,
                                             int *remap_table);

#endif /* HANDBRAKE_AUDIO_REMAP_H */
