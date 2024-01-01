/* hbavfilter.h

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_AVFILTER_H
#define HANDBRAKE_AVFILTER_H

#include "libavcodec/avcodec.h"
#include "handbrake/common.h"

typedef struct hb_avfilter_graph_s hb_avfilter_graph_t;

hb_avfilter_graph_t *
hb_avfilter_graph_init(hb_value_t * settings, hb_filter_init_t * init);

void    hb_avfilter_graph_close(hb_avfilter_graph_t ** _g);

const char *
hb_avfilter_graph_settings(hb_avfilter_graph_t * graph);

void    hb_avfilter_graph_update_init(hb_avfilter_graph_t * graph,
                                      hb_filter_init_t    * init);

int     hb_avfilter_add_frame(hb_avfilter_graph_t * graph, AVFrame * frame);

int     hb_avfilter_get_frame(hb_avfilter_graph_t * graph, AVFrame * frame);

int     hb_avfilter_add_buf(hb_avfilter_graph_t * graph, hb_buffer_t ** in);

hb_buffer_t *
hb_avfilter_get_buf(hb_avfilter_graph_t * graph);

void    hb_avfilter_append_dict(hb_value_array_t * filters,
                                const char * name, hb_dict_t * settings);

void    hb_avfilter_combine(hb_list_t * list);

#endif // HANDBRAKE_AVFILTER_H
