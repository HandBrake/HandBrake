/* audio_remap.h
 *
 * Copyright (c) 2003-2012 HandBrake Team
 * This file is part of the HandBrake source code
 * Homepage: <http://handbrake.fr/>
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/* This file handles the following two scenarios:
 *
 * 1) remapping audio from decoder order to libav order (for downmixing)
 *
 * 2) remapping audio from libav order to encoder order (for encoding)
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

#ifndef AUDIO_REMAP_H
#define AUDIO_REMAP_H

#include <stdint.h>

/* we only need to support the 11 "most common" channels */
#define HB_AUDIO_REMAP_MAX_CHANNELS 11

typedef float hb_sample_t;

typedef struct
{
    int nchannels;
    int sample_size;
    int remap_needed;
    int *remap_table;
    hb_sample_t tmp[HB_AUDIO_REMAP_MAX_CHANNELS];
} hb_audio_remap_t;

typedef struct
{
    uint64_t channel_order_map[HB_AUDIO_REMAP_MAX_CHANNELS+1];
} hb_chan_map_t;

/* Predefined channel maps for common channel orders. */
extern hb_chan_map_t hb_libav_chan_map;
extern hb_chan_map_t hb_liba52_chan_map;
extern hb_chan_map_t hb_libdca_chan_map;
extern hb_chan_map_t hb_vorbis_chan_map;
extern hb_chan_map_t hb_aac_chan_map;

/* Initialize an hb_audio_remap_t to remap audio with the specified channel
 * layout, from the input channel order (indicated by map_in) to the output
 * channel order (indicated by map_out).
 */
hb_audio_remap_t* hb_audio_remap_init(uint64_t channel_layout,
                                      hb_chan_map_t *map_out,
                                      hb_chan_map_t *map_in);

/* Free an hb_audio_remap_t. */
void              hb_audio_remap_free(hb_audio_remap_t *remap);

/* Remap audio between 2 different channel orders, using the settings specified
 * in the remap paremeter. Remapping is only done when necessary.
 *
 * The remap parameter can be NULL (no remapping).
 */
void              hb_audio_remap(hb_audio_remap_t *remap,
                                 hb_sample_t *samples,
                                 int nsamples);

/* Generate a table used to remap audio between 2 different channel orders.
 *
 * Usage: output_sample[channel_idx] = input_sample[remap_table[channel_idx]]
 */
int*              hb_audio_remap_build_table(uint64_t channel_layout,
                                             hb_chan_map_t *map_out,
                                             hb_chan_map_t *map_in);

#endif /* AUDIO_REMAP_H */
