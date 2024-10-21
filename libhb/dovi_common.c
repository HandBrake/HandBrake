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
#include "handbrake/av1_common.h"
#include "handbrake/common.h"

static struct
{
    const uint32_t id;
    const uint32_t max_pps;
    const uint32_t max_width;
    const uint32_t max_bitrate_main_tier;
    const uint32_t max_bitrate_high_tier;
}
hb_dovi_levels[] =
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

static struct
{
    const char *level;
    const int level_id;
    const uint32_t max_luma_sample_rate;
    const uint32_t max_luma_picture_size;
    const uint32_t max_bitrate_main_tier;
    const uint32_t max_bitrate_high_tier;
}

hb_h265_level_limits[] =
{
    { "1.0", 10, 552960,     36864,    128,    128    },
    { "2.0", 20, 3686400,    122880,   1500,   1500   },
    { "2.1", 31, 7372800,    245760,   3000,   3000   },
    { "3.0", 30, 16588800,   552960,   6000,   6000   },
    { "3.1", 31, 33177600,   983040,   10000,  10000  },
    { "4.0", 40, 66846720,   2228224,  12000,  30000  },
    { "4.1", 41, 133693440,  2228224,  20000,  50000  },
    { "5.0", 50, 267386880,  8912896,  25000,  100000 },
    { "5.1", 51, 534773760,  8912896,  40000,  160000 },
    { "5.2", 52, 1069547520, 8912896,  60000,  240000 },
    { "6.0", 60, 1069547520, 35651584, 60000,  240000 },
    { "6.1", 61, 2139095040, 35651584, 120000, 480000 },
    { "6.2", 62, 4278190080, 35651584, 240000, 800000 },
    { NULL,  0,  0,          0,        0,      0      }
};

// From AV1 Annex A
static struct
{
    const char *level;
    const int level_id;
    const uint32_t max_pic_size;
    const uint32_t max_h_size;
    const uint32_t max_v_size;
    const uint32_t max_decode_rate;
    const uint32_t max_bitrate_main_tier;
    const uint32_t max_bitrate_high_tier;
}
hb_av1_level_limits[] =
{
    { "2.0", 20,   147456,  2048, 1152,    4423680,   1500,   1500 },
    { "2.1", 31,   278784,  2816, 1584,    8363520,   3000,   3000 },
    { "2.2", 31,   278784,  2816, 3000,    8363520,   3000,   3000 },
    { "2.3", 31,   278784,  2816, 3000,    8363520,   3000,   3000 },
    { "3.0", 30,   665856,  4352, 2448,   19975680,   6000,   6000 },
    { "3.1", 31,   665856,  5504, 3096,   31950720,  10000,  10000 },
    { "3.2", 31,   665856,  5504, 3096,   31950720,  10000,  10000 },
    { "3.3", 31,   665856,  5504, 3096,   31950720,  10000,  10000 },
    { "4.0", 40,  2359296,  6144, 3456,   70778880,  12000,  30000 },
    { "4.1", 40,  2359296,  6144, 3456,  141557760,  20000,  50000 },
    { "4.2", 40,  2359296,  6144, 3456,  141557760,  20000,  50000 },
    { "4.3", 40,  2359296,  6144, 3456,  141557760,  20000,  50000 },
    { "5.0", 50,  8912896,  8192, 4352,  267386880,  30000, 100000 },
    { "5.1", 51,  8912896,  8192, 4352,  534773760,  40000, 160000 },
    { "5.2", 52,  8912896,  8192, 4352, 1069547520,  60000, 240000 },
    { "5.3", 52,  8912896,  8192, 4352, 1069547520,  60000, 240000 },
    { "6.0", 60, 35651584, 16384, 8704, 1069547520,  60000, 240000 },
    { "6.1", 61, 35651584, 16384, 8704, 2139095040, 100000, 480000 },
    { "6.2", 62, 35651584, 16384, 8704, 4278190080, 160000, 800000 },
    { "6.3", 62, 35651584, 16384, 8704, 4278190080, 160000, 800000 },
    { "7.0", 62, 35651584, 16384, 8704, 4278190080, 160000, 800000 },
    { "7.1", 62, 35651584, 16384, 8704, 4278190080, 160000, 800000 },
    { "7.2", 62, 35651584, 16384, 8704, 4278190080, 160000, 800000 },
    { "7.3", 62, 35651584, 16384, 8704, 4278190080, 160000, 800000 },
    {  NULL,  0,        0,     0,   0,           0,      0,      0 }
};

int hb_dovi_max_rate(int vcodec, int width, int pps, int bitrate, int level, int high_tier)
{
    int max_rate = 0;
    if (level)
    {
        if (vcodec & HB_VCODEC_H265_MASK)
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
        else if (vcodec & HB_VCODEC_AV1_MASK)
        {
            for (int i = 0; hb_av1_level_limits[i].level_id != 0; i++)
            {
                if (i == level)
                {
                    max_rate = high_tier ?
                    hb_av1_level_limits[i].max_bitrate_high_tier :
                    hb_av1_level_limits[i].max_bitrate_main_tier;
                    break;
                }
            }
        }
    }
    else
    {
        for (int i = 0; hb_dovi_levels[i].id != 0; i++)
        {
            int level_max_rate = high_tier ?
                                    hb_dovi_levels[i].max_bitrate_high_tier :
                                    hb_dovi_levels[i].max_bitrate_main_tier;

            if (pps <= hb_dovi_levels[i].max_pps &&
                width <= hb_dovi_levels[i].max_width &&
                bitrate <= level_max_rate * 1000)
            {
                max_rate = level_max_rate * 1000;
                break;
            }
        }
    }

    return max_rate;
}

int hb_dovi_level(int width, int pps, int max_rate, int high_tier)
{
    int dv_level = hb_dovi_levels[12].id;
;

    for (int i = 0; hb_dovi_levels[i].id != 0; i++)
    {
        int max_pps = hb_dovi_levels[i].max_pps;
        int max_width = hb_dovi_levels[i].max_width;
        int tier_max_rate = high_tier ?
                                hb_dovi_levels[i].max_bitrate_high_tier :
                                hb_dovi_levels[i].max_bitrate_main_tier;

        tier_max_rate *= 1000;

        if (pps <= max_pps && max_rate <= tier_max_rate && width <= max_width)
        {
            dv_level = hb_dovi_levels[i].id;
            break;
        }
    }

    return dv_level;
}
