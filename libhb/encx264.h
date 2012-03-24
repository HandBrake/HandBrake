/* This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "x264.h"

static const char * const  h264_level_names[] = { "1.0", "1b", "1.1", "1.2", "1.3", "2.0", "2.1", "2.2", "3.0", "3.1", "3.2", "4.0", "4.1", "4.2", "5.0", "5.1", 0 };
static const int    const h264_level_values[] = {    10,    9,    11,    12,    13,    20,    21,    22,    30,    31,    32,    40,    41,    42,    50,    51, 0 };

void hb_apply_h264_level( x264_param_t * param, const char * level, const char * x264_profile );
