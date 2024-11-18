/* qsv_common.h
 *
 * Copyright (c) 2003-2024 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_QSV_COMMON_H
#define HANDBRAKE_QSV_COMMON_H

int            hb_qsv_available();

#include "handbrake/project.h"

#if HB_PROJECT_FEATURE_QSV

// Public API
int  hb_qsv_impl_set_preferred(const char *name);

#ifdef __LIBHB__
// Private API

#include "vpl/mfxvideo.h"
#include "handbrake/hb_dict.h"
#include "handbrake/qsv_libav.h"

/* Minimum Intel Media SDK version (currently 1.3, for Sandy Bridge support) */
#define HB_QSV_MINVERSION_MAJOR HB_QSV_MSDK_VERSION_MAJOR
#define HB_QSV_MINVERSION_MINOR HB_QSV_MSDK_VERSION_MINOR

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
    // each info struct only corresponds to one CodecId and implementation combo
    mfxU32  codec_id;
    mfxIMPL implementation;

    // whether the encoder is available for this implementation
    int available;

    // version-specific or hardware-specific capabilities
    uint64_t capabilities;
    // support for API 1.6 or later
#define HB_QSV_CAP_MSDK_API_1_6      (1LL <<  0)
    // H.264, H.265: B-frames can be used as references
#define HB_QSV_CAP_B_REF_PYRAMID     (1LL <<  1)
#define HB_QSV_CAP_LOWPOWER_ENCODE   (1LL <<  2)
    // mfxExtVideoSignalInfo
#define HB_QSV_CAP_VUI_VSINFO        (1LL <<  3)
    // mfxExtChromaLocInfo
#define HB_QSV_CAP_VUI_CHROMALOCINFO (1LL <<  4)
    // mfxExtMasteringDisplayColourVolume
#define HB_QSV_CAP_VUI_MASTERINGINFO (1LL <<  5)
    // mfxExtContentLightLevelInfo
#define HB_QSV_CAP_VUI_CLLINFO       (1LL <<  6)

    // optional rate control methods
#define HB_QSV_CAP_RATECONTROL_LA    (1LL << 10)
#define HB_QSV_CAP_RATECONTROL_LAi   (1LL << 11)
#define HB_QSV_CAP_RATECONTROL_ICQ   (1LL << 12)
    // mfxExtCodingOption
#define HB_QSV_CAP_OPTION1           (1LL << 20)
    // mfxExtCodingOption2
#define HB_QSV_CAP_OPTION2           (1LL << 30)
#define HB_QSV_CAP_OPTION2_MBBRC     (1LL << 31)
#define HB_QSV_CAP_OPTION2_EXTBRC    (1LL << 32)
#define HB_QSV_CAP_OPTION2_TRELLIS   (1LL << 33)
#define HB_QSV_CAP_OPTION2_REPEATPPS (1LL << 34)
#define HB_QSV_CAP_OPTION2_BREFTYPE  (1LL << 35)
#define HB_QSV_CAP_OPTION2_IB_ADAPT  (1LL << 36)
#define HB_QSV_CAP_OPTION2_LA_DOWNS  (1LL << 37)
#define HB_QSV_CAP_OPTION2_NMPSLICE  (1LL << 38)
#define HB_QSV_CAP_VPP_SCALING       (1LL << 39)
#define HB_QSV_CAP_VPP_INTERPOLATION (1LL << 40)
    // mfxExtAV1BitstreamParam
#define HB_QSV_CAP_AV1_BITSTREAM     (1LL << 41)
    // mfxExtHyperModeParam
#define HB_QSV_CAP_HYPERENCODE       (1LL << 42)
    // mfxExtAV1ScreenContentTools
#define HB_QSV_CAP_AV1_SCREENCONTENT (1LL << 43)

    // TODO: add maximum encode resolution, etc.
} hb_qsv_info_t;

/* Intel Quick Sync Video utilities */
int            hb_qsv_create_mfx_session(mfxIMPL implementation, int adapter_index, mfxVersion *pver, mfxSession *psession);
hb_display_t * hb_qsv_display_init(const uint32_t dri_render_node);
int            hb_qsv_video_encoder_is_enabled(int adapter_index, int encoder);
int            hb_qsv_info_init();
void           hb_qsv_info_close();
void           hb_qsv_info_print();
hb_list_t*     hb_qsv_adapters_list();
int            hb_qsv_hyper_encode_available(int adapter_index);
hb_qsv_info_t* hb_qsv_encoder_info_get(int adapter_index, int encoder);
int            hb_qsv_hardware_generation(int cpu_platform);
int            hb_qsv_get_platform(int adapter_index);
int            hb_qsv_get_adapter_index();
int            hb_qsv_get_adapter_render_node(int adapter_index);
int            hb_qsv_implementation_is_hardware(mfxIMPL implementation);

/* Intel Quick Sync Video DECODE utilities */
const char* hb_qsv_decode_get_codec_name(enum AVCodecID codec_id);
int hb_qsv_decode_is_enabled(hb_job_t *job);

/* Media SDK parameters handling */
enum
{
    HB_QSV_PARAM_OK,
    HB_QSV_PARAM_ERROR,
    HB_QSV_PARAM_BAD_NAME,
    HB_QSV_PARAM_BAD_VALUE,
    HB_QSV_PARAM_UNSUPPORTED,
};

/*
 *  * Determine the "generation" of QSV hardware based on the CPU microarchitecture.
 *   * Anything unknown is assumed to be more recent than the latest known generation.
 *    * This avoids having to order the hb_cpu_platform enum depending on QSV hardware.
 *     */
enum
{
    QSV_G0, // third party hardware
    QSV_G1, // Sandy Bridge or equivalent
    QSV_G2, // Ivy Bridge or equivalent
    QSV_G3, // Haswell or equivalent
    QSV_G4, // Broadwell or equivalent
    QSV_G5, // Skylake or equivalent
    QSV_G6, // Kaby Lake or equivalent
    QSV_G7, // Ice Lake or equivalent
    QSV_G8, // Tiger Lake or equivalent
    QSV_G9, // DG2 or equivalent
    QSV_G10,// Lunar Lake or equivalent
    QSV_FU, // always last (future processors)
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
     * MFX_EXTBUFF_CHROMA_LOC_INFO
     * MFX_EXTBUFF_MASTERING_DISPLAY_COLOUR_VOLUME
     * MFX_EXTBUFF_CONTENT_LIGHT_LEVEL_INFO
     *
     * This should cover all encode-compatible extended
     * buffers that can be attached to an mfxVideoParam.
     */
#define HB_QSV_ENC_NUM_EXT_PARAM_MAX 16
    mfxExtBuffer*         ExtParamArray[HB_QSV_ENC_NUM_EXT_PARAM_MAX];
    mfxExtCodingOption    codingOption;
    mfxExtCodingOption2   codingOption2;
    mfxExtVideoSignalInfo videoSignalInfo;
    mfxExtHyperModeParam hyperEncodeParam;
    mfxExtAV1ScreenContentTools av1ScreenContentToolsParam;
    mfxExtChromaLocInfo   chromaLocInfo;
    mfxExtMasteringDisplayColourVolume masteringDisplayColourVolume;
    mfxExtContentLightLevelInfo        contentLightLevelInfo;
    mfxExtAV1BitstreamParam av1BitstreamParam;
    struct
    {
        int b_pyramid;
        int gop_pic_size;
        int gop_ref_dist;
        int int_ref_cycle_size;
    } gop;
    struct
    {
        int   icq;
        int   lookahead;
        int   cqp_offsets[3];
        int   vbv_max_bitrate;
        int   vbv_buffer_size;
        float vbv_buffer_init;
    } rc;

    // assigned via hb_qsv_param_default, may be shared with another structure
    mfxVideoParam *videoParam;
} hb_qsv_param_t;

static const char* const hb_qsv_preset_names1[] = { "speed", "balanced",            NULL, };
static const char* const hb_qsv_preset_names2[] = { "speed", "balanced", "quality", NULL, };
const char* const* hb_qsv_preset_get_names();
const char* const* hb_qsv_profile_get_names(int encoder);
const char* const* hb_qsv_level_get_names(int encoder);
const int* hb_qsv_get_pix_fmts(int encoder);

const char* hb_qsv_video_quality_get_name(uint32_t codec);
void hb_qsv_video_quality_get_limits(uint32_t codec, float *low, float *high, float *granularity, int *direction);

#define HB_QSV_CLIP3(min, max, val) ((val < min) ? min : (val > max) ? max : val)
int         hb_qsv_codingoption_xlat    (int val);
const char* hb_qsv_codingoption_get_name(int val);

int   hb_qsv_trellisvalue_xlat(int val);
int   hb_qsv_atoindex(const char* const *arr, const char *str, int *err);
int   hb_qsv_atobool (const char *str, int *err);
int   hb_qsv_atoi    (const char *str, int *err);
float hb_qsv_atof    (const char *str, int *err);

int hb_qsv_param_default_async_depth();
int hb_qsv_param_default_preset     (hb_qsv_param_t *param, mfxVideoParam *videoParam, hb_qsv_info_t *info, const char *preset);
int hb_qsv_param_default            (hb_qsv_param_t *param, mfxVideoParam *videoParam, hb_qsv_info_t *info);
int hb_qsv_param_parse              (hb_qsv_param_t *param,                            hb_qsv_info_t *info, hb_job_t *job,  const char *key, const char *value);
int hb_qsv_profile_parse            (hb_qsv_param_t *param,                            hb_qsv_info_t *info, const char *profile_key, const int codec);
int hb_qsv_level_parse              (hb_qsv_param_t *param,                            hb_qsv_info_t *info, const char *level_key);
int hb_qsv_param_parse_dx_index     (hb_job_t *job, const int dx_index);

typedef struct
{
    const char *name;
    const char *key;
    const int value;
}
hb_triplet_t;

hb_triplet_t* hb_triplet4value(hb_triplet_t *triplets, const int  value);
hb_triplet_t* hb_triplet4name (hb_triplet_t *triplets, const char *name);
hb_triplet_t* hb_triplet4key  (hb_triplet_t *triplets, const char *key);

const char* hb_qsv_codec_name    (uint32_t codec_id);
const char* hb_qsv_profile_name  (uint32_t codec_id, uint16_t profile_id);
const char* hb_qsv_level_name    (uint32_t codec_id, uint16_t level_id);
const char* hb_qsv_frametype_name(uint16_t qsv_frametype);
uint8_t     hb_qsv_frametype_xlat(uint16_t qsv_frametype, uint16_t *out_flags);

const char* hb_qsv_impl_get_name(int impl);
int         hb_qsv_impl_get_num(int impl);
const char* hb_qsv_impl_get_via_name(int impl);
mfxIMPL     hb_qsv_dx_index_to_impl(int dx_index);

/* Full QSV pipeline helpers */
int hb_qsv_is_enabled(hb_job_t *job);
hb_qsv_context* hb_qsv_context_init();
void hb_qsv_context_uninit(hb_job_t *job);
int hb_qsv_sanitize_filter_list(hb_job_t *job);
int hb_qsv_hw_frames_init(AVCodecContext *s);
int hb_qsv_create_ffmpeg_dec_pool(hb_job_t * job, int width, int height, int sw_pix_fmt);
int hb_qsv_create_ffmpeg_pool(hb_job_t *job, int coded_width, int coded_height, enum AVPixelFormat sw_pix_fmt, int pool_size, int extra_hw_frames, AVBufferRef **out_hw_frames_ctx);
int hb_qsv_create_ffmpeg_vpp_pool(hb_filter_init_t *init, int width, int height);
int hb_qsv_hw_filters_via_system_memory_are_enabled(hb_job_t *job);
int hb_qsv_hw_filters_via_video_memory_are_enabled(hb_job_t *job);
int hb_qsv_get_memory_type(hb_job_t *job);
int hb_qsv_full_path_is_enabled(hb_job_t *job);
AVBufferRef *hb_qsv_create_mids(AVBufferRef *hw_frames_ref);
hb_buffer_t * hb_qsv_copy_video_buffer_to_hw_video_buffer(hb_job_t *job, hb_buffer_t *in, const int is_vpp);
hb_buffer_t * hb_qsv_buffer_dup(hb_job_t *job, hb_buffer_t *in, const int is_vpp);
hb_buffer_t * hb_qsv_copy_avframe_to_video_buffer(hb_job_t *job, AVFrame *frame, AVRational time_base, const int is_vpp);
int hb_qsv_get_free_surface_from_pool(HBQSVFramesContext* hb_enc_qsv_frames_ctx, AVFrame* frame, QSVMid** out_mid);
void hb_qsv_get_free_surface_from_pool_with_range(HBQSVFramesContext* hb_enc_qsv_frames_ctx, const int start_index, const int end_index, QSVMid** out_mid, mfxFrameSurface1** out_surface);
int hb_qsv_get_mid_by_surface_from_pool(HBQSVFramesContext* hb_enc_qsv_frames_ctx, mfxFrameSurface1 *surface, QSVMid **out_mid);
int hb_qsv_replace_surface_mid(HBQSVFramesContext* hb_qsv_frames_ctx, const QSVMid *mid, mfxFrameSurface1 *surface);
int hb_qsv_release_surface_from_pool_by_surface_pointer(HBQSVFramesContext* hb_enc_qsv_frames_ctx, const mfxFrameSurface1 *surface);
int hb_qsv_get_buffer(AVCodecContext *s, AVFrame *frame, int flags);
enum AVPixelFormat hb_qsv_get_format(AVCodecContext *s, const enum AVPixelFormat *pix_fmts);
void hb_qsv_uninit_dec(AVCodecContext *s);
void hb_qsv_uninit_enc(hb_job_t *job);
int hb_qsv_setup_job(hb_job_t *job);
int hb_qsv_decode_h264_is_supported(int adapter_index);
int hb_qsv_decode_h265_is_supported(int adapter_index);
int hb_qsv_decode_h265_10_bit_is_supported(int adapter_index);
int hb_qsv_decode_av1_is_supported(int adapter_index);
int hb_qsv_decode_vvc_is_supported(int adapter_index);
int hb_qsv_decode_is_codec_supported(int adapter_index, int video_codec_param, int pix_fmt, int width, int height);
int hb_qsv_device_init(hb_job_t *job);

#endif // __LIBHB__
#endif // HB_PROJECT_FEATURE_QSV
#endif // HANDBRAKE_QSV_COMMON_H
