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
 * 1) remapping from liba52/libdca order to libav order
 *    - this allows downmixing liba52/libdca sources with libavresample
 *
 * 2) remapping from libav order to aac/vorbis order
 *    - this allows encoding  audio without libavcodec (faac, ca_aac, libvorbis)
 *
 * Thus we only need to support:
 *
 * a) channels found in liba52/libdca layouts
 * b) channels found in HB_AMIXDOWN_* layouts
 *
 * Notes:
 *
 * Left/Right Surround      -> Side Left/Right
 * Left/Right Rear Surround -> Back Left/Right */

#ifndef AUDIO_REMAP_H
#define AUDIO_REMAP_H

#include <stdint.h>

// we only need to support the 11 "most common" channels
#define HB_AUDIO_REMAP_MAX_CHANNELS 11

typedef float hb_sample_t;

typedef struct
{
    uint64_t channel_order[HB_AUDIO_REMAP_MAX_CHANNELS+1];
} hb_chan_map_t;

// used to convert between various channel orders
extern hb_chan_map_t hb_libav_chan_map;
extern hb_chan_map_t hb_liba52_chan_map;
extern hb_chan_map_t hb_libdca_chan_map;
extern hb_chan_map_t hb_vorbis_chan_map;
extern hb_chan_map_t hb_aac_chan_map;

int* hb_audio_remap_build_table(uint64_t layout, hb_chan_map_t *map_in, hb_chan_map_t *map_out);
void hb_audio_remap(int nchannels, int nsamples, hb_sample_t *samples, int *remap_table);

#endif /* AUDIO_REMAP_H */
