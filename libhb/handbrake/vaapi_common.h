/* vaapi_common.h
 *
 * Copyright (c) 2003-2020 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_VAAPI_COMMON_H
#define HANDBRAKE_VAAPI_COMMON_H

#include "handbrake/hbffmpeg.h"

int            hb_vaapi_h264_available();
int            hb_vaapi_h265_available();
int            hb_vaapi_vp8_available();
int            hb_vaapi_vp9_available();

int hb_avcodec_vaapi_set_hwframe_ctx(AVCodecContext *ctx, int hw_device_ctxidx, int init_pool_sz);

#endif // HANDBRAKE_VAAPI_COMMON_H
