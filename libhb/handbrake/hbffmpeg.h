/* hbffmpeg.h

   Copyright (c) 2003-2019 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_FFMPEG_H
#define HANDBRAKE_FFMPEG_H

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/channel_layout.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/avutil.h"
#include "libavutil/downmix_info.h"
#include "libavutil/display.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "handbrake/common.h"

#define HB_FFMPEG_THREADS_AUTO (-1) // let hb_avcodec_open() decide thread_count

void hb_avcodec_init(void);
int  hb_avcodec_open(AVCodecContext *, AVCodec *, AVDictionary **, int);
void hb_avcodec_free_context(AVCodecContext **avctx);
const char* const* hb_av_preset_get_names(int encoder);

uint64_t hb_ff_mixdown_xlat(int hb_mixdown, int *downmix_mode);
void     hb_ff_set_sample_fmt(AVCodecContext *, AVCodec *, enum AVSampleFormat);

int hb_sws_get_colorspace(int color_matrix);
int hb_colr_pri_hb_to_ff(int colr_prim);
int hb_colr_tra_hb_to_ff(int colr_tra);
int hb_colr_mat_hb_to_ff(int colr_mat);
int hb_colr_pri_ff_to_hb(int colr_prim);
int hb_colr_tra_ff_to_hb(int colr_tra);
int hb_colr_mat_ff_to_hb(int colr_mat);

struct SwsContext*
hb_sws_get_context(int srcW, int srcH, enum AVPixelFormat srcFormat,
                   int dstW, int dstH, enum AVPixelFormat dstFormat,
                   int flags, int colorspace);

static const char* const hb_vce_preset_names[] = { "speed", "balanced", "quality", NULL, };

void            hb_video_buffer_to_avframe(AVFrame *frame, hb_buffer_t * buf);
hb_buffer_t   * hb_avframe_to_video_buffer(AVFrame *frame,
                                           AVRational time_base);
void            hb_avframe_set_video_buffer_flags(hb_buffer_t * buf,
                                           AVFrame *frame,
                                           AVRational time_base);

int hb_av_encoder_present(int encoder);
const char* const* hb_av_profile_get_names(int encoder);
const char* const* hb_av_level_get_names(int encoder);

#endif // HANDBRAKE_FFMPEG_H
