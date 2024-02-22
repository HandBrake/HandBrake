/* dovi_common.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <string.h>
#include "handbrake/dovi_common.h"
#include "handbrake/h265_common.h"

int hb_dovi_max_rate(int width, int pps, int bitrate, int level, int high_tier)
{
    int max_rate = 0;
    if (level)
    {
        for (int i = 0; hb_h265_level_limits[i].level_id != 0; i++)
        {
            if (hb_h265_level_limits[i].level_id == level)
            {
                max_rate = high_tier ?
                                    hb_h265_level_limits[i].max_bitrate_high_tier :
                                    hb_h265_level_limits[i].max_bitrate_main_tier;
                break;
            }
        }
    }
    else
    {
        for (int i = 0; hb_dolby_vision_levels[i].id != 0; i++)
        {
            int level_max_rate = high_tier ?
                                    hb_dolby_vision_levels[i].max_bitrate_high_tier :
                                    hb_dolby_vision_levels[i].max_bitrate_main_tier;

            if (pps <= hb_dolby_vision_levels[i].max_pps &&
                width <= hb_dolby_vision_levels[i].max_width &&
                bitrate <= level_max_rate * 1000)
            {
                max_rate = level_max_rate;
                break;
            }
        }
    }

    return max_rate;
}

int hb_dovi_level(int width, int pps, int max_rate, int high_tier)
{
    int dv_level = hb_dolby_vision_levels[12].id;
;

    for (int i = 0; hb_dolby_vision_levels[i].id != 0; i++)
    {
        int max_pps = hb_dolby_vision_levels[i].max_pps;
        int max_width = hb_dolby_vision_levels[i].max_width;
        int tier_max_rate = high_tier ?
                                hb_dolby_vision_levels[i].max_bitrate_high_tier :
                                hb_dolby_vision_levels[i].max_bitrate_main_tier;

        tier_max_rate *= 1000;

        if (pps <= max_pps && max_rate <= tier_max_rate && width <= max_width)
        {
            dv_level = hb_dolby_vision_levels[i].id;
            break;
        }
    }

    return dv_level;
}
