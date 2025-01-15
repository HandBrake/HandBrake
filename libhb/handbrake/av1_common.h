/* av1_common.h

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_AV1_COMMON_H
#define HANDBRAKE_AV1_COMMON_H

#include "handbrake/project.h"

static const char * const hb_av1_profile_names[] = {
    "auto", "main", "high", "professional", NULL, };

static const char * const hb_av1_level_names[] = {
    "auto", "2.0", "2.1", "2.2", "2.3", "3.0", "3.1", "3.2",
    "3.3", "4.0", "4.1", "4.2", "4.3", "5.0", "5.1", "5.2",
    "5.3", "6.0", "6.1", "6.2", "6.3", NULL, };

static const int          hb_av1_level_values[] = {
     -1,  20,  21,  22,  23,  30,  31,  32,  33,  40,  41,  42,
     43,  50,  51,  52,  53,  60,  61,  62,  63,  0 };

static const char * const hb_av1_svt_preset_names[] =
{
    "13", "12", "11", "10", "9", "8", "7", "6", "5", "4", "3", "2", "1", "0", "-1", NULL
};

static const char * const hb_av1_svt_tune_names[] =
{
    "vq", "psnr", "ssim", "fastdecode", NULL
};

static const char * const hb_av1_svt_profile_names[] =
{
    "auto", "main", NULL // "high", "profesional"
};

#endif // HANDBRAKE_AV1_COMMON_H
