/* avfilter_priv.h

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_AVFILTER_PRIV_H
#define HANDBRAKE_AVFILTER_PRIV_H

#include "libavfilter/avfilter.h"
#include "handbrake/hbavfilter.h"

struct hb_filter_private_s
{
    int                   initialized;
    hb_avfilter_graph_t * graph;

    // Buffer list to delay output by one frame.  Required to set stop time.
    hb_buffer_list_t      list;

    // Placeholder settings for AVFilter aliases
    hb_value_t          * avfilters;
    hb_filter_init_t      input;
    hb_filter_init_t      output;
};

int  hb_avfilter_null_work( hb_filter_object_t * filter,
                            hb_buffer_t ** buf_in, hb_buffer_t ** buf_out );
void hb_avfilter_alias_close( hb_filter_object_t * filter );

#endif // HANDBRAKE_AVFILTER_PRIV_H
