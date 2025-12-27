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

/**
 * Initializes global vaapi encoder device.
 *
 * For encoding options, see
 * - https://www.ffmpeg.org/ffmpeg-codecs.html#VAAPI-encoders
 * - `ffmpeg -h encoder=h264_vaapi`
 * - `ffmpeg -h encoder=hevc_vaapi`
 * - `ffmpeg -h encoder=av1_vaapi`
 */
void hb_vaapi_init();
void hb_vaapi_free();
int hb_vaapi_available();
int hb_vaapi_encoder_available(int encoder);

int hb_vaapi_avcodec_set_hwframe_ctx(AVCodecContext *ctx, int hw_device_ctxidx, int init_pool_sz);

/**
 * Sends the frame to encode via `avcodec_send_frame`
 * and converts source frame (e.g. AV_PIX_FMT_YUV420P) to AV_PIX_FMT_VAAPI/NV12.
 *
 * @param context the codec context
 * @param frame the source AVFrame
 * @param hw_frame may hold hardware AVFrame pointer, must be released via `av_frame_free` if not NULL
 * @return
 */
int hb_vaapi_avcodec_send_frame(AVCodecContext *context, AVFrame *frame, AVFrame **hw_frame);

#endif // HANDBRAKE_VAAPI_COMMON_H
