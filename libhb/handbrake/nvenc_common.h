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
char *         hb_map_nvenc_preset_name (const char * preset);

int            hb_nvdec_available(int codec_id);
int            hb_nvdec_hw_ctx_init(struct AVCodecContext *ctx,
                                    struct hb_job_t *job);
int            hb_nvdec_hwframes_ctx_init(struct AVCodecContext *ctx,
                                          struct hb_job_t *job);
int            hb_nvdec_hwframe_init(struct hb_job_t *job, struct AVFrame **frame);

char*          hb_nvdec_get_codec_name(enum AVCodecID codec_id);

int            hb_nvdec_is_enabled(struct hb_job_t *job);

int            hb_nvdec_are_filters_supported(struct hb_list_t *filters);

void           hb_nvdec_disable(struct hb_job_t *job);

#endif // HANDBRAKE_NVENC_COMMON_H
