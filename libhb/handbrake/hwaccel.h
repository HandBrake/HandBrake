/* hwaccel.h
 *
 * Copyright (c) 2003-2025 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_HWACCEL_COMMON_H
#define HANDBRAKE_HWACCEL_COMMON_H

#include "handbrake/hbffmpeg.h"

void hb_register_hwaccel(hb_hwaccel_t *hwaccel);
void hb_hwaccel_common_hwaccel_init();

hb_hwaccel_t * hb_get_hwaccel(int hw_decode);
hb_hwaccel_t * hb_get_hwaccel_from_pix_fmt(enum AVPixelFormat hw_pix_fmt);

int hb_hwaccel_hw_device_ctx_init(enum AVHWDeviceType device_type, int device_index, void **hw_device_ctx);
void hb_hwaccel_hw_device_ctx_close(void **hw_device_ctx);

enum AVPixelFormat hw_hwaccel_get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts);

int hb_hwaccel_hwframes_ctx_init(AVCodecContext *ctx,
                                 enum AVPixelFormat sw_pix_fmt,
                                 enum AVPixelFormat hw_pix_fmt);

AVBufferRef *hb_hwaccel_init_hw_frames_ctx(AVBufferRef *hw_device_ctx,
                                       enum AVPixelFormat sw_fmt,
                                       enum AVPixelFormat hw_fmt,
                                       int width,
                                       int height,
                                       int initial_pool_size);

int hb_hwaccel_is_available(hb_hwaccel_t *hwaccel, int codec_id);
int hb_hwaccel_can_use_full_hw_pipeline(hb_hwaccel_t *hwaccel, hb_list_t *list_filter, int encoder);

#endif // HANDBRAKE_HWACCEL_COMMON_H
