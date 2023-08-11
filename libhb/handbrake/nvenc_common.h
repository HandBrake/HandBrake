/* nvenc_common.h
 *
 * Copyright (c) 2003-2022 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_NVENC_COMMON_H
#define HANDBRAKE_NVENC_COMMON_H

#include "handbrake/hbffmpeg.h"

int            hb_nvenc_h264_available();
int            hb_nvenc_h265_available();
int            hb_nvenc_av1_available();
int            hb_check_nvenc_available();
int            hb_check_nvdec_available();

const char * hb_map_nvenc_preset_name (const char *preset);

int hb_nvenc_are_filters_supported(hb_list_t *filters);

#endif // HANDBRAKE_NVENC_COMMON_H
