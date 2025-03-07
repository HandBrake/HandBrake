/* vce_common.h
 *
 * Copyright (c) 2003-2025 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_VCE_COMMON_H
#define HANDBRAKE_VCE_COMMON_H

int            hb_vce_h264_available();
int            hb_vce_h265_available();
int            hb_vce_av1_available();
int            hb_vce_available();
int            hb_check_amfdec_available();
int            hb_vce_setup_job(hb_job_t *job);
int            hb_vce_dec_is_enabled(hb_job_t *job);
int            hb_vce_sanitize_filter_list(hb_job_t *job);
int            hb_amf_decode_is_codec_supported(int video_codec_param);

int            hb_vce_are_filters_supported(hb_list_t *filters);
const char*    hb_vce_decode_get_codec_name(enum AVCodecID codec_id);
int            hb_vce_hw_filters_via_video_memory_are_enabled(hb_job_t *job);
hb_buffer_t *  hb_vce_copy_avframe_to_video_buffer(hb_job_t *job, AVFrame *frame, AVRational time_base);

static const char * const hb_vce_h264_profile_names[] = { "auto", "baseline", "main", "high",  NULL, };
static const char * const hb_vce_h265_profile_names[] = { "auto", "main", NULL, };
static const char * const hb_vce_h265_10bit_profile_names[] = { "auto", "main10", NULL, };
static const char * const hb_vce_av1_profile_names[]  = { "auto", "main", NULL, };

static const char * const hb_vce_h264_level_names[] =
{
    "auto", "1.0", "1.1", "1.2", "1.3", "2.0", "2.1", "2.2", "3.0",
    "3.1", "3.2", "4.0", "4.1", "4.2", "5.0", "5.1", "5.2",  NULL,
};


static const char * const hb_vce_av1_level_names[] =
{
    "auto", "2.0", "2.1", "2.2", "3.0", "3.1", "3.2", "3.3", "4.0", "4.1", "4.2", "4.3",
    "5.0", "5.1", "5.2", "5.3", "6.0", "6.1", "6.2", "6.3", "7.0", "7.1", "7.2", "7.3", NULL,
};

#endif // HANDBRAKE_VCE_COMMON_H
