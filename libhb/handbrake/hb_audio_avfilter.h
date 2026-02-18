/* hb_audio_avfilter.h

   Copyright (c) 2003-2026 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_AUDIO_AVFILTER_H
#define HANDBRAKE_AUDIO_AVFILTER_H

#include "handbrake/handbrake.h"

typedef struct hb_audio_avfilter_graph_s hb_audio_avfilter_graph_t;

hb_audio_avfilter_graph_t *
hb_audio_avfilter_graph_init(const char * settings, int sample_rate,
                              int sample_fmt, uint64_t channel_layout,
                              int channels, int allow_ch_change);
void    hb_audio_avfilter_graph_close(hb_audio_avfilter_graph_t ** graph);
int     hb_audio_avfilter_graph_get_out_channels(hb_audio_avfilter_graph_t * graph);
int     hb_audio_avfilter_add_buf(hb_audio_avfilter_graph_t * graph,
                                   hb_buffer_t ** buf_in, int sample_rate,
                                   int channels);
hb_buffer_t * hb_audio_avfilter_get_buf(hb_audio_avfilter_graph_t * graph,
                                         int sample_rate, int channels);

#endif // HANDBRAKE_AUDIO_AVFILTER_H
