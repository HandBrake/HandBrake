/* h264_common.h

   Copyright (c) 2003-2012 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HB_H264_COMMON_H
#define HB_H264_COMMON_H

static const char * const hb_h264_profile_names[] = { "auto", "high", "main", "baseline", NULL, };
static const char * const   hb_h264_level_names[] = { "auto", "1.0", "1b", "1.1", "1.2", "1.3", "2.0", "2.1", "2.2", "3.0", "3.1", "3.2", "4.0", "4.1", "4.2", "5.0", "5.1", "5.2",  NULL, };
static const int    const  hb_h264_level_values[] = {     -1,    10,    9,    11,    12,    13,    20,    21,    22,    30,    31,    32,    40,    41,    42,    50,    51,    52,     0, };

#endif  //HB_H264_COMMON_H
