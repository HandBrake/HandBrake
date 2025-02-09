/* hbffmpeg.h

   Copyright (c) 2003-2025 HandBrake Team
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
#include "libavutil/mastering_display_metadata.h"
#include "libavutil/ambient_viewing_environment.h"
#include "libavutil/dovi_meta.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "handbrake/common.h"

#define HB_FFMPEG_THREADS_AUTO (-1) // let hb_avcodec_open() decide thread_count

int  hb_avcodec_open(AVCodecContext *, const AVCodec *, AVDictionary **, int);
void hb_avcodec_free_context(AVCodecContext **avctx);
const char* const* hb_av_preset_get_names(int encoder);
const char* const* hb_av_tune_get_names(int encoder);

uint64_t hb_ff_mixdown_xlat(int hb_mixdown, int *downmix_mode);
void     hb_ff_set_sample_fmt(AVCodecContext *, const AVCodec *, enum AVSampleFormat);

int hb_sws_get_colorspace(int color_matrix);
int hb_colr_pri_hb_to_ff(int colr_prim);
int hb_colr_tra_hb_to_ff(int colr_tra);
int hb_colr_mat_hb_to_ff(int colr_mat);
int hb_colr_pri_ff_to_hb(int colr_prim);
int hb_colr_tra_ff_to_hb(int colr_tra);
int hb_colr_mat_ff_to_hb(int colr_mat);

hb_mastering_display_metadata_t hb_mastering_ff_to_hb(AVMasteringDisplayMetadata mastering);
AVMasteringDisplayMetadata hb_mastering_hb_to_ff(hb_mastering_display_metadata_t mastering);

hb_ambient_viewing_environment_metadata_t hb_ambient_ff_to_hb(AVAmbientViewingEnvironment ambient);
AVAmbientViewingEnvironment hb_ambient_hb_to_ff(hb_ambient_viewing_environment_metadata_t ambient);

AVDOVIDecoderConfigurationRecord hb_dovi_hb_to_ff(hb_dovi_conf_t dovi);
hb_dovi_conf_t hb_dovi_ff_to_hb(AVDOVIDecoderConfigurationRecord dovi);

struct SwsContext*
hb_sws_get_context(int srcW, int srcH, enum AVPixelFormat srcFormat, int srcRange,
                   int dstW, int dstH, enum AVPixelFormat dstFormat, int dstRange,
                   int flags, int colorspace);

static const char* const hb_vce_preset_names[] = { "speed", "balanced", "quality", NULL, };

void            hb_video_buffer_to_avframe(AVFrame *frame, hb_buffer_t **buf);
hb_buffer_t   * hb_avframe_to_video_buffer(AVFrame *frame,
                                           AVRational time_base);
void            hb_avframe_set_video_buffer_flags(hb_buffer_t * buf,
                                           AVFrame *frame,
                                           AVRational time_base);

int hb_av_encoder_present(int encoder);
const char* const* hb_av_profile_get_names(int encoder);
const char* const* hb_av_level_get_names(int encoder);
const int* hb_av_get_pix_fmts(int encoder);

int hb_av_can_use_zscale(enum AVPixelFormat pix_fmt,
                         int in_width, int in_height,
                         int out_width, int out_height);

#endif // HANDBRAKE_FFMPEG_H
