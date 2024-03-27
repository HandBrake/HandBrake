/* dovi_common.h

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_DOVI_COMMON_H
#define HANDBRAKE_DOVI_COMMON_H

#include <stdint.h>

int hb_dovi_max_rate(int width, int pps, int bitrate, int level, int high_tier);
int hb_dovi_level(int width, int pps, int max_rate, int high_tier);

#endif // HANDBRAKE_DOVI_COMMON_H
