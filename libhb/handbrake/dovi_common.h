/* dovi_common.h

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_DOVI_COMMON_H
#define HANDBRAKE_DOVI_COMMON_H

#include "handbrake/project.h"
#include <stdint.h>

static struct
{
    const uint32_t id;
    const uint32_t max_pps;
    const uint32_t max_width;
    const uint32_t max_bitrate_main_tier;
    const uint32_t max_bitrate_high_tier;
}
hb_dolby_vision_levels[] =
{
    { 1,  22118400,   1280, 20,  50  },
    { 2,  27648000,   1280, 20,  50  },
    { 3,  49766400,   1920, 20,  70  },
    { 4,  62208000,   2560, 20,  70  },
    { 5,  124416000,  3840, 20,  70  },
    { 6,  199065600,  3840, 25,  130 },
    { 7,  248832000,  3840, 25,  130 },
    { 8,  398131200,  3840, 40,  130 },
    { 9,  497664000,  3840, 40,  130 },
    { 10, 995328000,  3840, 60,  240 },
    { 11, 995328000,  7680, 60,  240 },
    { 12, 1990656000, 7680, 120, 480 },
    { 13, 3981312000, 7680, 240, 800 },
    { 0, 0, 0, 0, 0 }
};

int hb_dovi_max_rate(int width, int pps, int bitrate, int level, int high_tier);
int hb_dovi_level(int width, int pps, int max_rate, int high_tier);

#endif // HANDBRAKE_DOVI_COMMON_H
