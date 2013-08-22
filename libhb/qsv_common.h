/* qsv_common.h
 *
 * Copyright (c) 2003-2013 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HB_QSV_COMMON_H
#define HB_QSV_COMMON_H

#include "msdk/mfxvideo.h"
#include "libavcodec/avcodec.h"

/* Minimum Intel Media SDK version (currently 1.3, for Sandy Bridge support) */
#define HB_QSV_MINVERSION_MAJOR AV_QSV_MSDK_VERSION_MAJOR
#define HB_QSV_MINVERSION_MINOR AV_QSV_MSDK_VERSION_MINOR

/*
 * Get & store all available Intel Quick Sync information:
 *
 * - general availability
 * - available implementations (hardware-accelerated, software fallback, etc.)
 * - available codecs, filters, etc. for direct access (convenience)
 * - supported API version
 * - supported resolutions
 */
typedef struct hb_qsv_info_s
{
    // supported version-specific or hardware-specific capabilities
    int capabilities;
#define HB_QSV_CAP_H264_BPYRAMID     (1 << 0) // H.264: reference B-frames
#define HB_QSV_CAP_MSDK_API_1_6      (1 << 1) // Support for API 1.6 or later
#define HB_QSV_CAP_OPTION2_BRC       (1 << 2) // mfxExtCodingOption2: MBBRC/ExtBRC
#define HB_QSV_CAP_OPTION2_LOOKAHEAD (1 << 3) // mfxExtCodingOption2: LookAhead
#define HB_QSV_CAP_OPTION2_TRELLIS   (1 << 4) // mfxExtCodingOption2: Trellis

    // TODO: add available decoders, filters, encoders,
    //       maximum decode and encode resolution, etc.
} hb_qsv_info_t;

/* Global Intel QSV information for use by the UIs */
extern hb_qsv_info_t *hb_qsv_info;

/* Intel Quick Sync Video utilities */
int  hb_qsv_available();
int  hb_qsv_info_init();
void hb_qsv_info_print();

/* Intel Quick Sync Video DECODE utilities */
const char* hb_qsv_decode_get_codec_name(enum AVCodecID codec_id);
int hb_qsv_decode_is_enabled(hb_job_t *job);
int hb_qsv_decode_is_supported(enum AVCodecID codec_id, enum AVPixelFormat pix_fmt);

/* Media SDK parameters handling */
enum
{
    HB_QSV_PARAM_OK,
    HB_QSV_PARAM_ERROR,
    HB_QSV_PARAM_BAD_NAME,
    HB_QSV_PARAM_BAD_VALUE,
    HB_QSV_PARAM_UNSUPPORTED,
};

typedef struct
{
    /*
     * Supported mfxExtBuffer.BufferId values:
     *
     * MFX_EXTBUFF_AVC_REFLIST_CTRL
     * MFX_EXTBUFF_AVC_TEMPORAL_LAYERS
     * MFX_EXTBUFF_CODING_OPTION
     * MFX_EXTBUFF_CODING_OPTION_SPSPPS
     * MFX_EXTBUFF_CODING_OPTION2
     * MFX_EXTBUFF_ENCODER_CAPABILITY
     * MFX_EXTBUFF_ENCODER_RESET_OPTION
     * MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION
     * MFX_EXTBUFF_PICTURE_TIMING_SEI
     * MFX_EXTBUFF_VIDEO_SIGNAL_INFO
     *
     * This should cover all encode-compatible extended
     * buffers that can be attached to an mfxVideoParam.
     */
#define HB_QSV_ENC_NUM_EXT_PARAM_MAX 10
    mfxExtBuffer*         ExtParamArray[HB_QSV_ENC_NUM_EXT_PARAM_MAX];
    mfxExtCodingOption    codingOption;
    mfxExtCodingOption2   codingOption2;
    mfxExtVideoSignalInfo videoSignalInfo;
    struct
    {
        int gop_pic_size;
        int int_ref_cycle_size;
    } gop;
    struct
    {
        int   lookahead;
        int   cqp_offsets[3];
        int   vbv_max_bitrate;
        int   vbv_buffer_size;
        float vbv_buffer_init;
    } rc;

    // assigned via hb_qsv_param_default, may be shared with another structure
    mfxVideoParam *videoParam;
} hb_qsv_param_t;

#define HB_QSV_CLIP3(min, max, val) ((val < min) ? min : (val > max) ? max : val)
int   hb_qsv_codingoption_xlat(int val);
int   hb_qsv_trellisvalue_xlat(int val);
int   hb_qsv_atoindex(const char* const *arr, const char *str, int *err);
int   hb_qsv_atobool (const char *str, int *err);
int   hb_qsv_atoi    (const char *str, int *err);
float hb_qsv_atof    (const char *str, int *err);

int hb_qsv_param_default(hb_qsv_param_t *param, mfxVideoParam *videoParam);
int hb_qsv_param_parse  (hb_qsv_param_t *param, const char *key, const char *value, int vcodec);

#endif
