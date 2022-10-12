/* vce_common.h
 *
 * Copyright (c) 2003-2022 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_VCE_COMMON_H
#define HANDBRAKE_VCE_COMMON_H

int            hb_vce_h264_available();
int            hb_vce_h265_available();

static const char * const hb_vce_h264_profile_names[] = { "baseline", "main", "high",  NULL, };
static const char * const hb_vce_h265_profile_names[] = { "main", NULL, };
static const char * const hb_vce_h265_10bit_profile_names[] = { "main10", NULL, };

static const char * const hb_vce_h264_level_names[] =
{
    "auto", "1.0", "1.1", "1.2", "1.3", "2.0", "2.1", "2.2", "3.0",
    "3.1", "3.2", "4.0", "4.1", "4.2", "5.0", "5.1", "5.2",  NULL,
};

#endif // HANDBRAKE_VCE_COMMON_H
