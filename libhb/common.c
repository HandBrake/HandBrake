/* common.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <locale.h>

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/extradata.h"
#include "x264.h"
#include "handbrake/lang.h"
#include "handbrake/common.h"
#include "handbrake/h264_common.h"
#include "handbrake/h265_common.h"
#include "handbrake/av1_common.h"
#include "handbrake/encx264.h"
#include "handbrake/hwaccel.h"
#if HB_PROJECT_FEATURE_QSV
#include "handbrake/qsv_common.h"
#endif

#if HB_PROJECT_FEATURE_X265
#include "x265.h"
#endif

#ifdef SYS_MINGW
#include <windows.h>
#endif

#if HB_PROJECT_FEATURE_MF
#include "handbrake/mf_common.h"
#endif
#if HB_PROJECT_FEATURE_NVENC
#include "handbrake/nvenc_common.h"
#endif
#if HB_PROJECT_FEATURE_VCE
#include "handbrake/vce_common.h"
#endif

#ifdef __APPLE__
#include "platform/macosx/vt_common.h"
#endif

static int mixdown_get_opus_coupled_stream_count(int mixdown);

/**********************************************************************
 * Global variables
 *********************************************************************/
static hb_error_handler_t *error_handler = NULL;

/* Generic IDs for encoders, containers, etc. */
enum
{
    HB_GID_NONE = -1, // encoders must NEVER use it
    HB_GID_VCODEC_H264_MF,
    HB_GID_VCODEC_H264_NVENC,
    HB_GID_VCODEC_H264_QSV,
    HB_GID_VCODEC_H264_VCE,
    HB_GID_VCODEC_H264_VT,
    HB_GID_VCODEC_H264_X264,
    HB_GID_VCODEC_H265_MF,
    HB_GID_VCODEC_H265_NVENC,
    HB_GID_VCODEC_H265_QSV,
    HB_GID_VCODEC_H265_VCE,
    HB_GID_VCODEC_H265_VT,
    HB_GID_VCODEC_H265_X265,
    HB_GID_VCODEC_MPEG2,
    HB_GID_VCODEC_MPEG4,
    HB_GID_VCODEC_THEORA,
    HB_GID_VCODEC_VP8,
    HB_GID_VCODEC_VP9,
    HB_GID_VCODEC_AV1_SVT,
    HB_GID_VCODEC_AV1_QSV,
    HB_GID_VCODEC_AV1_NVENC,
    HB_GID_VCODEC_AV1_VCE,
    HB_GID_VCODEC_AV1_MF,
    HB_GID_VCODEC_FFV1,
    HB_GID_ACODEC_ALAC,
    HB_GID_ACODEC_ALAC_PASS,
    HB_GID_ACODEC_AAC,
    HB_GID_ACODEC_AAC_HE,
    HB_GID_ACODEC_AAC_PASS,
    HB_GID_ACODEC_AC3,
    HB_GID_ACODEC_AC3_PASS,
    HB_GID_ACODEC_AUTO_PASS,
    HB_GID_ACODEC_DTS_PASS,
    HB_GID_ACODEC_DTSHD_PASS,
    HB_GID_ACODEC_EAC3,
    HB_GID_ACODEC_EAC3_PASS,
    HB_GID_ACODEC_FLAC,
    HB_GID_ACODEC_FLAC_PASS,
    HB_GID_ACODEC_MP2_PASS,
    HB_GID_ACODEC_MP3,
    HB_GID_ACODEC_MP3_PASS,
    HB_GID_ACODEC_TRUEHD,
    HB_GID_ACODEC_TRUEHD_PASS,
    HB_GID_ACODEC_VORBIS,
    HB_GID_ACODEC_VORBIS_PASS,
    HB_GID_ACODEC_OPUS,
    HB_GID_ACODEC_OPUS_PASS,
    HB_GID_MUX_MKV,
    HB_GID_MUX_MP4,
    HB_GID_MUX_WEBM,
};

#define HB_VIDEO_CLOCK    27000000 // 27MHz clock
#define HB_VIDEO_FPS_MIN  1
#define HB_VIDEO_FPS_MAX  1000
int hb_video_rate_clock = HB_VIDEO_CLOCK;
int hb_video_rate_min   = HB_VIDEO_CLOCK / HB_VIDEO_FPS_MAX; // Min clock rate from *max* frame rate
int hb_video_rate_max   = HB_VIDEO_CLOCK / HB_VIDEO_FPS_MIN; // Max clock rate from *min* frame rate

typedef struct
{
    hb_rate_t  item;
    hb_rate_t *next;
    int enabled;
} hb_rate_internal_t;
hb_rate_t *hb_video_rates_first_item = NULL;
hb_rate_t *hb_video_rates_last_item  = NULL;
hb_rate_internal_t hb_video_rates[]  =
{
    // legacy framerates (disabled)
    { { "23.976 (NTSC Film)",  1126125, }, NULL, 0, },
    { { "25 (PAL Film/Video)", 1080000, }, NULL, 0, },
    { { "29.97 (NTSC Video)",   900900, }, NULL, 0, },
    // actual framerates
    { {  "5",                  5400000, }, NULL, 1, },
    { { "10",                  2700000, }, NULL, 1, },
    { { "12",                  2250000, }, NULL, 1, },
    { { "15",                  1800000, }, NULL, 1, },
    { { "20",                  1350000, }, NULL, 1, },
    { { "23.976",              1126125, }, NULL, 1, },
    { { "24",                  1125000, }, NULL, 1, },
    { { "25",                  1080000, }, NULL, 1, },
    { { "29.97",                900900, }, NULL, 1, },
    { { "30",                   900000, }, NULL, 1, },
    { { "48",                   562500, }, NULL, 1, },
    { { "50",                   540000, }, NULL, 1, },
    { { "59.94",                450450, }, NULL, 1, },
    { { "60",                   450000, }, NULL, 1, },
    { { "72",                   375000, }, NULL, 1, },
    { { "75",                   360000, }, NULL, 1, },
    { { "90",                   300000, }, NULL, 1, },
    { { "100",                  270000, }, NULL, 1, },
    { { "120",                  225000, }, NULL, 1, },
};
int hb_video_rates_count = sizeof(hb_video_rates) / sizeof(hb_video_rates[0]);

hb_rate_t *hb_audio_rates_first_item = NULL;
hb_rate_t *hb_audio_rates_last_item  = NULL;
hb_rate_internal_t hb_audio_rates[]  =
{
    { {  "8",      8000, }, NULL, 1, },
    { { "11.025", 11025, }, NULL, 1, },
    { { "12",     12000, }, NULL, 1, },
    { { "16",     16000, }, NULL, 1, },
    { { "22.05",  22050, }, NULL, 1, },
    { { "24",     24000, }, NULL, 1, },
    { { "32",     32000, }, NULL, 1, },
    { { "44.1",   44100, }, NULL, 1, },
    { { "48",     48000, }, NULL, 1, },
    { { "88.2",   88200, }, NULL, 1, },
    { { "96",     96000, }, NULL, 1, },
    { { "176.4",  176400, },NULL, 1, },
    { { "192",    192000, },NULL, 1, },
};
int hb_audio_rates_count = sizeof(hb_audio_rates) / sizeof(hb_audio_rates[0]);

hb_rate_t *hb_audio_bitrates_first_item = NULL;
hb_rate_t *hb_audio_bitrates_last_item  = NULL;
hb_rate_internal_t hb_audio_bitrates[]  =
{
    // AC3-compatible bitrates
    { {   "6",     6, }, NULL, 1, },
    { {   "12",   12, }, NULL, 1, },
    { {   "24",   24, }, NULL, 1, },
    { {   "32",   32, }, NULL, 1, },
    { {   "40",   40, }, NULL, 1, },
    { {   "48",   48, }, NULL, 1, },
    { {   "56",   56, }, NULL, 1, },
    { {   "64",   64, }, NULL, 1, },
    { {   "80",   80, }, NULL, 1, },
    { {   "96",   96, }, NULL, 1, },
    { {  "112",  112, }, NULL, 1, },
    { {  "128",  128, }, NULL, 1, },
    { {  "160",  160, }, NULL, 1, },
    { {  "192",  192, }, NULL, 1, },
    { {  "224",  224, }, NULL, 1, },
    { {  "256",  256, }, NULL, 1, },
    { {  "320",  320, }, NULL, 1, },
    { {  "384",  384, }, NULL, 1, },
    { {  "448",  448, }, NULL, 1, },
    { {  "512",  512, }, NULL, 1, },
    { {  "576",  576, }, NULL, 1, },
    { {  "640",  640, }, NULL, 1, },
    // additional bitrates
    { {  "768",  768, }, NULL, 1, },
    { {  "960",  960, }, NULL, 1, },
    { { "1152", 1152, }, NULL, 1, },
    { { "1344", 1344, }, NULL, 1, },
    { { "1536", 1536, }, NULL, 1, },
    { { "2304", 2304, }, NULL, 1, },
    { { "3072", 3072, }, NULL, 1, },
    { { "4608", 4608, }, NULL, 1, },
    { { "6144", 6144, }, NULL, 1, },
};
int hb_audio_bitrates_count = sizeof(hb_audio_bitrates) / sizeof(hb_audio_bitrates[0]);

typedef struct
{
    hb_dither_t  item;
    hb_dither_t *next;
    int enabled;
} hb_dither_internal_t;
hb_dither_t *hb_audio_dithers_first_item = NULL;
hb_dither_t *hb_audio_dithers_last_item  = NULL;
hb_dither_internal_t hb_audio_dithers[]  =
{
    { { "default",                       "auto",          SWR_DITHER_NONE - 1,            }, NULL, 1, },
    { { "none",                          "none",          SWR_DITHER_NONE,                }, NULL, 1, },
    { { "rectangular",                   "rectangular",   SWR_DITHER_RECTANGULAR,         }, NULL, 1, },
    { { "triangular",                    "triangular",    SWR_DITHER_TRIANGULAR,          }, NULL, 1, },
    { { "triangular with high pass",     "triangular_hp", SWR_DITHER_TRIANGULAR_HIGHPASS, }, NULL, 1, },
    { { "lipshitz noise shaping",        "lipshitz_ns",   SWR_DITHER_NS_LIPSHITZ,         }, NULL, 1, },
};
int hb_audio_dithers_count = sizeof(hb_audio_dithers) / sizeof(hb_audio_dithers[0]);

typedef struct
{
    hb_mixdown_t  item;
    hb_mixdown_t *next;
    int enabled;
} hb_mixdown_internal_t;
hb_mixdown_t *hb_audio_mixdowns_first_item = NULL;
hb_mixdown_t *hb_audio_mixdowns_last_item  = NULL;
hb_mixdown_internal_t hb_audio_mixdowns[]  =
{
    // legacy mixdowns, back to HB 0.9.4 whenever possible (disabled)
    { { "AC3 Passthru",       "",           HB_AMIXDOWN_NONE,      }, NULL, 0, },
    { { "DTS Passthru",       "",           HB_AMIXDOWN_NONE,      }, NULL, 0, },
    { { "DTS-HD Passthru",    "",           HB_AMIXDOWN_NONE,      }, NULL, 0, },
    { { "6-channel discrete", "6ch",        HB_AMIXDOWN_5POINT1,   }, NULL, 0, },
    // actual mixdowns
    { { "None",               "none",       HB_AMIXDOWN_NONE,      }, NULL, 1, },
    { { "Mono",               "mono",       HB_AMIXDOWN_MONO,      }, NULL, 1, },
    { { "Mono (Left Only)",   "left_only",  HB_AMIXDOWN_LEFT,      }, NULL, 1, },
    { { "Mono (Right Only)",  "right_only", HB_AMIXDOWN_RIGHT,     }, NULL, 1, },
    { { "Stereo",             "stereo",     HB_AMIXDOWN_STEREO,    }, NULL, 1, },
    { { "Dolby Surround",     "dpl1",       HB_AMIXDOWN_DOLBY,     }, NULL, 1, },
    { { "Dolby Pro Logic II", "dpl2",       HB_AMIXDOWN_DOLBYPLII, }, NULL, 1, },
    { { "5.1 Channels",       "5point1",    HB_AMIXDOWN_5POINT1,   }, NULL, 1, },
    { { "6.1 Channels",       "6point1",    HB_AMIXDOWN_6POINT1,   }, NULL, 1, },
    { { "7.1 Channels",       "7point1",    HB_AMIXDOWN_7POINT1,   }, NULL, 1, },
    { { "7.1 (5F/2R/LFE)",    "5_2_lfe",    HB_AMIXDOWN_5_2_LFE,   }, NULL, 1, },
};
int hb_audio_mixdowns_count = sizeof(hb_audio_mixdowns) / sizeof(hb_audio_mixdowns[0]);

typedef struct
{
    hb_encoder_t  item;
    hb_encoder_t *next;
    int deprecated;
    int enabled;
    int gid;
} hb_encoder_internal_t;
hb_encoder_t *hb_video_encoders_first_item = NULL;
hb_encoder_t *hb_video_encoders_last_item  = NULL;
hb_encoder_internal_t hb_video_encoders[]  =
{
    // legacy encoders, back to HB 0.9.4 whenever possible (disabled)
    { { "FFmpeg",                      "ffmpeg",           NULL,                             HB_VCODEC_FFMPEG_MPEG4,      HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, 0, HB_GID_VCODEC_MPEG4,  },
    { { "MPEG-4 (FFmpeg)",             "ffmpeg4",          NULL,                             HB_VCODEC_FFMPEG_MPEG4,      HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, 0, HB_GID_VCODEC_MPEG4,  },
    { { "MPEG-2 (FFmpeg)",             "ffmpeg2",          NULL,                             HB_VCODEC_FFMPEG_MPEG2,      HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, 0, HB_GID_VCODEC_MPEG2,  },
    { { "VP3 (Theora)",                "libtheora",        NULL,                             HB_VCODEC_THEORA,                            HB_MUX_MASK_MKV, }, NULL, 1, 0, HB_GID_VCODEC_THEORA, },
    // actual encoders
    { { "AV1 (SVT)",                   "svt_av1",          "AV1 (SVT)",                      HB_VCODEC_SVT_AV1,           HB_MUX_MASK_MP4|HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_AV1_SVT,    },
    { { "AV1 10-bit (SVT)",            "svt_av1_10bit",    "AV1 10-bit (SVT)",               HB_VCODEC_SVT_AV1_10BIT,     HB_MUX_MASK_MP4|HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_AV1_SVT,    },
    { { "AV1 (Intel QSV)",             "qsv_av1",          "AV1 (Intel QSV)",                HB_VCODEC_FFMPEG_QSV_AV1,    HB_MUX_MASK_MP4|HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_AV1_QSV,    },
    { { "AV1 10-bit (Intel QSV)",      "qsv_av1_10bit",    "AV1 10-bit (Intel QSV)",         HB_VCODEC_FFMPEG_QSV_AV1_10BIT, HB_MUX_MASK_MP4|HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_AV1_QSV, },
    { { "AV1 (NVEnc)",                 "nvenc_av1",        "AV1 (NVEnc)",                    HB_VCODEC_FFMPEG_NVENC_AV1,  HB_MUX_MASK_MP4|HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_AV1_NVENC,  },
    { { "AV1 10-bit (NVEnc)",          "nvenc_av1_10bit",  "AV1 10-bit (NVEnc)",             HB_VCODEC_FFMPEG_NVENC_AV1_10BIT, HB_MUX_MASK_MP4|HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_AV1_NVENC,  },
    { { "AV1 (AMD VCE)",               "vce_av1",          "AV1 (AMD VCE)",                  HB_VCODEC_FFMPEG_VCE_AV1,    HB_MUX_MASK_MP4|HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_AV1_VCE,    },
    { { "AV1 (MediaFoundation)",       "mf_av1",           "AV1 (MediaFoundation)",          HB_VCODEC_FFMPEG_MF_AV1,     HB_MUX_MASK_MP4|HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_AV1_MF,     },
    { { "FFV1",                        "ffv1",             "FFV1 (libavcodec)",              HB_VCODEC_FFMPEG_FFV1,                                        HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_FFV1,       },
    { { "H.264 (x264)",                "x264",             "H.264 (libx264)",                HB_VCODEC_X264_8BIT,                          HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H264_X264,  },
    { { "H.264 10-bit (x264)",         "x264_10bit",       "H.264 10-bit (libx264)",         HB_VCODEC_X264_10BIT,                         HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H264_X264,  },
    { { "H.264 (Intel QSV)",           "qsv_h264",         "H.264 (Intel QSV)",              HB_VCODEC_FFMPEG_QSV_H264,                    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H264_QSV,   },
    { { "H.264 (AMD VCE)",             "vce_h264",         "H.264 (AMD VCE)",                HB_VCODEC_FFMPEG_VCE_H264,                    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H264_VCE,   },
    { { "H.264 (NVEnc)",               "nvenc_h264",       "H.264 (NVEnc)",                  HB_VCODEC_FFMPEG_NVENC_H264,                  HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H264_NVENC, },
    { { "H.264 (MediaFoundation)",     "mf_h264",          "H.264 (MediaFoundation)",        HB_VCODEC_FFMPEG_MF_H264,                     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H264_MF,    },
    { { "H.264 (VideoToolbox)",        "vt_h264",          "H.264 (VideoToolbox)",           HB_VCODEC_VT_H264,                            HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H264_VT,    },
    { { "H.265 (x265)",                "x265",             "H.265 (libx265)",                HB_VCODEC_X265_8BIT,                            HB_MUX_AV_MP4|HB_MUX_AV_MKV,   }, NULL, 0, 1, HB_GID_VCODEC_H265_X265,  },
    { { "H.265 10-bit (x265)",         "x265_10bit",       "H.265 10-bit (libx265)",         HB_VCODEC_X265_10BIT,                           HB_MUX_AV_MP4|HB_MUX_AV_MKV,   }, NULL, 0, 1, HB_GID_VCODEC_H265_X265,  },
    { { "H.265 12-bit (x265)",         "x265_12bit",       "H.265 12-bit (libx265)",         HB_VCODEC_X265_12BIT,                           HB_MUX_AV_MP4|HB_MUX_AV_MKV,   }, NULL, 0, 1, HB_GID_VCODEC_H265_X265,  },
    { { "H.265 16-bit (x265)",         "x265_16bit",       "H.265 16-bit (libx265)",         HB_VCODEC_X265_16BIT,                           HB_MUX_AV_MP4|HB_MUX_AV_MKV,   }, NULL, 0, 1, HB_GID_VCODEC_H265_X265,  },
    { { "H.265 (Intel QSV)",           "qsv_h265",         "H.265 (Intel QSV)",              HB_VCODEC_FFMPEG_QSV_H265,                           HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H265_QSV, },
    { { "H.265 10-bit (Intel QSV)",    "qsv_h265_10bit",   "H.265 10-bit (Intel QSV)",       HB_VCODEC_FFMPEG_QSV_H265_10BIT,                     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H265_QSV, },
    { { "H.265 (AMD VCE)",             "vce_h265",         "H.265 (AMD VCE)",                HB_VCODEC_FFMPEG_VCE_H265,                    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H265_VCE,   },
    { { "H.265 10-bit (AMD VCE)",      "vce_h265_10bit",   "H.265 10-bit (AMD VCE)",         HB_VCODEC_FFMPEG_VCE_H265_10BIT,              HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H265_VCE,   },
    { { "H.265 (NVEnc)",               "nvenc_h265",       "H.265 (NVEnc)",                  HB_VCODEC_FFMPEG_NVENC_H265,                  HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H265_NVENC, },
    { { "H.265 10-bit (NVEnc)",        "nvenc_h265_10bit", "H.265 10-bit (NVEnc)",           HB_VCODEC_FFMPEG_NVENC_H265_10BIT,            HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H265_NVENC, },
    { { "H.265 (MediaFoundation)",     "mf_h265",          "H.265 (MediaFoundation)",        HB_VCODEC_FFMPEG_MF_H265,                     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H265_MF,    },
    { { "H.265 (VideoToolbox)",        "vt_h265",          "H.265 (VideoToolbox)",           HB_VCODEC_VT_H265,                            HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H265_VT,    },
    { { "H.265 10-bit (VideoToolbox)", "vt_h265_10bit",    "H.265 10-bit (VideoToolbox)",    HB_VCODEC_VT_H265_10BIT,                      HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_H265_VT,    },
    { { "MPEG-4",                      "mpeg4",            "MPEG-4 (libavcodec)",            HB_VCODEC_FFMPEG_MPEG4,                       HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_MPEG4,      },
    { { "MPEG-2",                      "mpeg2",            "MPEG-2 (libavcodec)",            HB_VCODEC_FFMPEG_MPEG2,                       HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_MPEG2,      },
    { { "VP8",                         "VP8",              "VP8 (libvpx)",                   HB_VCODEC_FFMPEG_VP8,                        HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_VP8,        },
    { { "VP9",                         "VP9",              "VP9 (libvpx)",                   HB_VCODEC_FFMPEG_VP9,        HB_MUX_MASK_MP4|HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_VP9,        },
    { { "VP9 10-bit",                  "VP9_10bit",        "VP9 10-bit (libvpx)",            HB_VCODEC_FFMPEG_VP9_10BIT,  HB_MUX_MASK_MP4|HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_VP9,        },
    { { "Theora",                      "theora",           "Theora (libtheora)",             HB_VCODEC_THEORA,                                             HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_VCODEC_THEORA,     },
};
int hb_video_encoders_count = sizeof(hb_video_encoders) / sizeof(hb_video_encoders[0]);
static int hb_video_encoder_is_enabled(int encoder, int disable_hardware)
{
    // Hardware Encoders
    if (!disable_hardware)
    {
#if HB_PROJECT_FEATURE_QSV
        if (encoder & HB_VCODEC_QSV_MASK)
        {
            return hb_qsv_video_encoder_is_available(encoder);
        }
#endif

        switch (encoder)
        {
#if HB_PROJECT_FEATURE_VCE
            case HB_VCODEC_FFMPEG_VCE_H264:
               return hb_vce_h264_available();
            case HB_VCODEC_FFMPEG_VCE_H265:
            case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
                return hb_vce_h265_available();
            case HB_VCODEC_FFMPEG_VCE_AV1:
                return hb_vce_av1_available();
#endif

#if HB_PROJECT_FEATURE_NVENC
            case HB_VCODEC_FFMPEG_NVENC_H264:
                return hb_nvenc_h264_available();
            case HB_VCODEC_FFMPEG_NVENC_H265:
            case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
                return hb_nvenc_h265_available();
            case HB_VCODEC_FFMPEG_NVENC_AV1:
            case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
                return hb_nvenc_av1_available();
#endif

#ifdef __APPLE__
            case HB_VCODEC_VT_H264:
            case HB_VCODEC_VT_H265:
            case HB_VCODEC_VT_H265_10BIT:
                return hb_vt_is_encoder_available(encoder);
#endif

#if HB_PROJECT_FEATURE_MF
            // TODO: Try to instantiate a throwaway encoder to see if a suitable MediaFoundation encoder can be found?
            // Alt, implement logic similar to ffmpeg's encoder selection, to see if one would be found.
            case HB_VCODEC_FFMPEG_MF_H264:
                return hb_mf_h264_available();
            case HB_VCODEC_FFMPEG_MF_H265:
                return hb_mf_h265_available();
            case HB_VCODEC_FFMPEG_MF_AV1:
                return hb_mf_av1_available();
#endif
        }
    }

    // Software Encoders
    switch (encoder)
    {
        // the following encoders are always enabled
        case HB_VCODEC_THEORA:
        case HB_VCODEC_FFMPEG_MPEG4:
        case HB_VCODEC_FFMPEG_MPEG2:
        case HB_VCODEC_FFMPEG_VP8:
        case HB_VCODEC_FFMPEG_VP9:
        case HB_VCODEC_FFMPEG_VP9_10BIT:
        case HB_VCODEC_SVT_AV1:
        case HB_VCODEC_SVT_AV1_10BIT:
        case HB_VCODEC_FFMPEG_FFV1:
            return 1;

#if HB_PROJECT_FEATURE_X265
        case HB_VCODEC_X265_8BIT:
        case HB_VCODEC_X265_10BIT:
        case HB_VCODEC_X265_12BIT:
        case HB_VCODEC_X265_16BIT:
        {
            const x265_api *api;
            api = x265_api_query(hb_video_encoder_get_depth(encoder), X265_BUILD, NULL);
            return (api != NULL);
        };
#endif

        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X264_10BIT:
        {
            const x264_api_t *api;
            api = hb_x264_api_get(hb_video_encoder_get_depth(encoder));
            return (api != NULL);
        }

        default:
            return 0;
    }
}

hb_encoder_t *hb_audio_encoders_first_item = NULL;
hb_encoder_t *hb_audio_encoders_last_item  = NULL;
hb_encoder_internal_t hb_audio_encoders[]  =
{
    // legacy encoders, back to HB 0.9.4 whenever possible (disabled)
    { { "",                   "dts",        NULL,                          HB_ACODEC_DCA_PASS,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, 0, HB_GID_ACODEC_DTS_PASS,   },
    { { "AAC (faac)",         "faac",       NULL,                          0,                     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, 0, HB_GID_ACODEC_AAC,        },
    { { "AAC (ffmpeg)",       "ffaac",      NULL,                          HB_ACODEC_FFAAC,       HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, 0, HB_GID_ACODEC_AAC,        },
    { { "AC3 (ffmpeg)",       "ffac3",      NULL,                          HB_ACODEC_AC3,         HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, 0, HB_GID_ACODEC_AC3,        },
    { { "MP3 (lame)",         "lame",       NULL,                          HB_ACODEC_LAME,        HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, 0, HB_GID_ACODEC_MP3,        },
    { { "Vorbis (vorbis)",    "libvorbis",  NULL,                          HB_ACODEC_VORBIS,                      HB_MUX_MASK_MKV, }, NULL, 1, 0, HB_GID_ACODEC_VORBIS,     },
    { { "FLAC (ffmpeg)",      "ffflac",     NULL,                          HB_ACODEC_FFFLAC,                      HB_MUX_MASK_MKV, }, NULL, 1, 0, HB_GID_ACODEC_FLAC,       },
    { { "FLAC (24-bit)",      "ffflac24",   NULL,                          HB_ACODEC_FFFLAC24,                    HB_MUX_MASK_MKV, }, NULL, 1, 0, HB_GID_ACODEC_FLAC,       },
    // generic names
    { { "AAC",                "aac",        NULL,                          0,                     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 0, HB_GID_ACODEC_AAC,        },
    { { "HE-AAC",             "haac",       NULL,                          0,                     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 0, HB_GID_ACODEC_AAC_HE,     },
    // actual encoders
    { { "None",               "none",       "None",                        HB_ACODEC_NONE,        0,                               }, NULL, 0, 1, HB_GID_NONE,              },
    { { "AAC (CoreAudio)",    "ca_aac",     "AAC (Apple AudioToolbox)",    HB_ACODEC_CA_AAC,      HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_AAC,        },
    { { "HE-AAC (CoreAudio)", "ca_haac",    "HE-AAC (Apple AudioToolbox)", HB_ACODEC_CA_HAAC,     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_AAC_HE,     },
    { { "AAC (FDK)",          "fdk_aac",    "AAC (libfdk_aac)",            HB_ACODEC_FDK_AAC,     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_AAC,        },
    { { "HE-AAC (FDK)",       "fdk_haac",   "HE-AAC (libfdk_aac)",         HB_ACODEC_FDK_HAAC,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_AAC_HE,     },
    { { "AAC (avcodec)",      "av_aac",     "AAC (libavcodec)",            HB_ACODEC_FFAAC,       HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_AAC,        },
    { { "AAC Passthru",       "copy:aac",   "AAC Passthru",                HB_ACODEC_AAC_PASS,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_AAC_PASS,   },
    { { "AC3",                "ac3",        "AC3 (libavcodec)",            HB_ACODEC_AC3,         HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_AC3,        },
    { { "AC3 Passthru",       "copy:ac3",   "AC3 Passthru",                HB_ACODEC_AC3_PASS,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_AC3_PASS,   },
    { { "E-AC3",              "eac3",       "E-AC3 (libavcodec)",          HB_ACODEC_FFEAC3,      HB_MUX_MASK_MP4|HB_MUX_AV_MKV,   }, NULL, 0, 1, HB_GID_ACODEC_EAC3,       },
    { { "E-AC3 Passthru",     "copy:eac3",  "E-AC3 Passthru",              HB_ACODEC_EAC3_PASS,   HB_MUX_MASK_MP4|HB_MUX_AV_MKV,   }, NULL, 0, 1, HB_GID_ACODEC_EAC3_PASS,  },
    { { "TrueHD",             "truehd",     "TrueHD",                      HB_ACODEC_FFTRUEHD,    HB_MUX_MASK_MP4|HB_MUX_AV_MKV,   }, NULL, 0, 1, HB_GID_ACODEC_TRUEHD,     },
    { { "TrueHD Passthru",    "copy:truehd","TrueHD Passthru",             HB_ACODEC_TRUEHD_PASS, HB_MUX_MASK_MP4|HB_MUX_AV_MKV,   }, NULL, 0, 1, HB_GID_ACODEC_TRUEHD_PASS,},
    { { "DTS Passthru",       "copy:dts",   "DTS Passthru",                HB_ACODEC_DCA_PASS,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_DTS_PASS,   },
    { { "DTS-HD Passthru",    "copy:dtshd", "DTS-HD Passthru",             HB_ACODEC_DCA_HD_PASS, HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_DTSHD_PASS, },
    { { "MP2 Passthru",       "copy:mp2",   "MP2 Passthru",                HB_ACODEC_MP2_PASS,                    HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_MP2_PASS,   },
    { { "MP3",                "mp3",        "MP3 (libmp3lame)",            HB_ACODEC_LAME,        HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_MP3,        },
    { { "MP3 Passthru",       "copy:mp3",   "MP3 Passthru",                HB_ACODEC_MP3_PASS,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_MP3_PASS,   },
    { { "Opus",               "opus",       "Opus (libopus)",              HB_ACODEC_OPUS,        HB_MUX_MASK_MP4|HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_OPUS,      },
    { { "Opus Passthru",      "copy:opus",  "Opus Passthru",               HB_ACODEC_OPUS_PASS,   HB_MUX_MASK_MP4|HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_OPUS_PASS, },
    { { "Vorbis",             "vorbis",     "Vorbis (libvorbis)",          HB_ACODEC_VORBIS,      HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_VORBIS,    },
    { { "Vorbis Passthru",    "copy:vorbis","Vorbis Passthru",             HB_ACODEC_VORBIS_PASS, HB_MUX_MASK_WEBM|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_VORBIS_PASS, },
    { { "FLAC 16-bit",        "flac16",     "FLAC 16-bit (libavcodec)",    HB_ACODEC_FFFLAC,      HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_FLAC,       },
    { { "FLAC 24-bit",        "flac24",     "FLAC 24-bit (libavcodec)",    HB_ACODEC_FFFLAC24,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_FLAC,       },
    { { "FLAC Passthru",      "copy:flac",  "FLAC Passthru",               HB_ACODEC_FLAC_PASS,   HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_FLAC_PASS,  },
    { { "ALAC 16-bit",        "alac16",     "ALAC 16-bit (libavcodec)",    HB_ACODEC_FFALAC,      HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_ALAC,       },
    { { "ALAC 24-bit",        "alac24",     "ALAC 24-bit (libavcodec)",    HB_ACODEC_FFALAC24,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_ALAC,       },
    { { "ALAC Passthru",      "copy:alac",  "ALAC Passthru",               HB_ACODEC_ALAC_PASS,   HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_ALAC_PASS,  },
    { { "Auto Passthru",      "copy",       "Auto Passthru",               HB_ACODEC_AUTO_PASS,   HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, 1, HB_GID_ACODEC_AUTO_PASS,  },
};
int hb_audio_encoders_count = sizeof(hb_audio_encoders) / sizeof(hb_audio_encoders[0]);
static int hb_audio_encoder_is_enabled(int encoder)
{
    if (encoder & HB_ACODEC_PASS_FLAG)
    {
        // Passthru encoders are always enabled
        return 1;
    }
    switch (encoder)
    {
#ifdef __APPLE__
        case HB_ACODEC_CA_AAC:
        case HB_ACODEC_CA_HAAC:
            return 1;
#endif

#if HB_PROJECT_FEATURE_FFMPEG_AAC
        case HB_ACODEC_FFAAC:
            return avcodec_find_encoder_by_name("aac") != NULL;
#endif

        case HB_ACODEC_FDK_AAC:
        case HB_ACODEC_FDK_HAAC:
            return avcodec_find_encoder_by_name("libfdk_aac") != NULL;

        case HB_ACODEC_AC3:
            return avcodec_find_encoder(AV_CODEC_ID_AC3) != NULL;

        case HB_ACODEC_FFALAC:
        case HB_ACODEC_FFALAC24:
            return avcodec_find_encoder(AV_CODEC_ID_ALAC) != NULL;

        case HB_ACODEC_FFEAC3:
            return avcodec_find_encoder(AV_CODEC_ID_EAC3) != NULL;

        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
            return avcodec_find_encoder(AV_CODEC_ID_FLAC) != NULL;

        case HB_ACODEC_OPUS:
            return avcodec_find_encoder(AV_CODEC_ID_OPUS) != NULL;

        case HB_ACODEC_FFTRUEHD:
            return avcodec_find_encoder(AV_CODEC_ID_TRUEHD) != NULL;

        // the following encoders are always enabled
        case HB_ACODEC_LAME:
        case HB_ACODEC_VORBIS:
        case HB_ACODEC_NONE:
            return 1;

        default:
            return 0;
    }
}

typedef struct
{
    hb_container_t  item;
    hb_container_t *next;
    int enabled;
    int gid;
} hb_container_internal_t;
hb_container_t *hb_containers_first_item = NULL;
hb_container_t *hb_containers_last_item  = NULL;
hb_container_internal_t hb_containers[]  =
{
    // legacy muxers, back to HB 0.9.4 whenever possible (disabled)
    { { "M4V file",            "m4v",     NULL,                     "m4v",  0,              }, NULL, 0, HB_GID_MUX_MP4,  },
    { { "MP4 file",            "mp4",     NULL,                     "mp4",  0,              }, NULL, 0, HB_GID_MUX_MP4,  },
    { { "MKV file",            "mkv",     NULL,                     "mkv",  0,              }, NULL, 0, HB_GID_MUX_MKV,  },
    // actual muxers
    { { "MPEG-4 (avformat)",   "av_mp4",  "MPEG-4 (libavformat)",   "mp4",  HB_MUX_AV_MP4,  }, NULL, 1, HB_GID_MUX_MP4,  },
    { { "MPEG-4 (mp4v2)",      "mp4v2",   "MPEG-4 (libmp4v2)",      "mp4",  HB_MUX_MP4V2,   }, NULL, 1, HB_GID_MUX_MP4,  },
    { { "Matroska (avformat)", "av_mkv",  "Matroska (libavformat)", "mkv",  HB_MUX_AV_MKV,  }, NULL, 1, HB_GID_MUX_MKV,  },
    { { "Matroska (libmkv)",   "libmkv",  "Matroska (libmkv)",      "mkv",  HB_MUX_LIBMKV,  }, NULL, 1, HB_GID_MUX_MKV,  },
    { { "WebM (avformat)",     "av_webm", "WebM (libavformat)",     "webm", HB_MUX_AV_WEBM, }, NULL, 1, HB_GID_MUX_WEBM, },
};
int hb_containers_count = sizeof(hb_containers) / sizeof(hb_containers[0]);
static int hb_container_is_enabled(int format)
{
    switch (format)
    {
        case HB_MUX_AV_MP4:
        case HB_MUX_AV_MKV:
        case HB_MUX_AV_WEBM:
            return 1;

        default:
            return 0;
    }
}

int hb_str_ends_with(const char *base, const char *str)
{
    int blen = strlen(base);
    int slen = strlen(str);
    return (blen >= slen) && (0 == strcasecmp(base + blen - slen, str));
}

static void hb_common_global_hw_init()
{
#ifdef __APPLE__
    hb_register_hwaccel(&hb_hwaccel_videotoolbox);
#endif
#if HB_PROJECT_FEATURE_NVDEC
    hb_register_hwaccel(&hb_hwaccel_nvdec);
#endif
#if HB_PROJECT_FEATURE_NVENC
    hb_nvenc_h264_available();
#endif
#if HB_PROJECT_FEATURE_VCE
    hb_vce_h264_available();
#endif
#if HB_PROJECT_FEATURE_MF
    hb_directx_available();
    hb_register_hwaccel(&hb_hwaccel_mf);
#endif
#if HB_PROJECT_FEATURE_QSV
    // First initialization and QSV adapters list collection should happen
    // after other hw vendors initializations to prevent device order issues
    hb_qsv_available();
    hb_register_hwaccel(&hb_hwaccel_qsv);
#endif

    hb_hwaccel_common_hwaccel_init();
}

void hb_common_global_init(int disable_hardware)
{
    static int common_init_done = 0;
    if (common_init_done)
        return;

    if (!disable_hardware)
    {
        hb_common_global_hw_init();
    }

    int i, j;

    // video framerates
    for (i = 0; i < hb_video_rates_count; i++)
    {
        if (hb_video_rates[i].enabled)
        {
            if (hb_video_rates_first_item == NULL)
            {
                hb_video_rates_first_item = &hb_video_rates[i].item;
            }
            else
            {
                ((hb_rate_internal_t*)hb_video_rates_last_item)->next =
                    &hb_video_rates[i].item;
            }
            hb_video_rates_last_item = &hb_video_rates[i].item;
        }
    }
    // fallbacks are static for now (no setup required)

    // audio samplerates
    for (i = 0; i < hb_audio_rates_count; i++)
    {
        if (hb_audio_rates[i].enabled)
        {
            if (hb_audio_rates_first_item == NULL)
            {
                hb_audio_rates_first_item = &hb_audio_rates[i].item;
            }
            else
            {
                ((hb_rate_internal_t*)hb_audio_rates_last_item)->next =
                    &hb_audio_rates[i].item;
            }
            hb_audio_rates_last_item = &hb_audio_rates[i].item;
        }
    }
    // fallbacks are static for now (no setup required)

    // audio bitrates
    for (i = 0; i < hb_audio_bitrates_count; i++)
    {
        if (hb_audio_bitrates[i].enabled)
        {
            if (hb_audio_bitrates_first_item == NULL)
            {
                hb_audio_bitrates_first_item = &hb_audio_bitrates[i].item;
            }
            else
            {
                ((hb_rate_internal_t*)hb_audio_bitrates_last_item)->next =
                    &hb_audio_bitrates[i].item;
            }
            hb_audio_bitrates_last_item = &hb_audio_bitrates[i].item;
        }
    }
    // fallbacks are static for now (no setup required)

    // audio dithers
    for (i = 0; i < hb_audio_dithers_count; i++)
    {
        if (hb_audio_dithers[i].enabled)
        {
            if (hb_audio_dithers_first_item == NULL)
            {
                hb_audio_dithers_first_item = &hb_audio_dithers[i].item;
            }
            else
            {
                ((hb_dither_internal_t*)hb_audio_dithers_last_item)->next =
                    &hb_audio_dithers[i].item;
            }
            hb_audio_dithers_last_item = &hb_audio_dithers[i].item;
        }
    }
    // fallbacks are static for now (no setup required)

    // audio mixdowns
    for (i = 0; i < hb_audio_mixdowns_count; i++)
    {
        if (hb_audio_mixdowns[i].enabled)
        {
            if (hb_audio_mixdowns_first_item == NULL)
            {
                hb_audio_mixdowns_first_item = &hb_audio_mixdowns[i].item;
            }
            else
            {
                ((hb_mixdown_internal_t*)hb_audio_mixdowns_last_item)->next =
                    &hb_audio_mixdowns[i].item;
            }
            hb_audio_mixdowns_last_item = &hb_audio_mixdowns[i].item;
        }
    }
    // fallbacks are static for now (no setup required)

    // video encoders
    for (i = 0; i < hb_video_encoders_count; i++)
    {
        if (hb_video_encoders[i].enabled)
        {
            // we still need to check
            hb_video_encoders[i].enabled =
                hb_video_encoder_is_enabled(hb_video_encoders[i].item.codec, disable_hardware);
        }
        if (hb_video_encoders[i].enabled)
        {
            if (hb_video_encoders_first_item == NULL)
            {
                hb_video_encoders_first_item = &hb_video_encoders[i].item;
            }
            else
            {
                ((hb_encoder_internal_t*)hb_video_encoders_last_item)->next =
                    &hb_video_encoders[i].item;
            }
            hb_video_encoders_last_item = &hb_video_encoders[i].item;
        }
    }
    // setup fallbacks
    for (i = 0; i < hb_video_encoders_count; i++)
    {
        if (!hb_video_encoders[i].enabled)
        {
            if ((hb_video_encoders[i].item.codec) &&
                (hb_video_encoder_is_enabled(hb_video_encoders[i].item.codec, disable_hardware)))
            {
                // we have a specific fallback and it's enabled
                continue;
            }
            for (j = 0; j < hb_video_encoders_count; j++)
            {
                if (hb_video_encoders[j].enabled &&
                    hb_video_encoders[j].gid == hb_video_encoders[i].gid)
                {
                    hb_video_encoders[i].item.codec = hb_video_encoders[j].item.codec;
                    break;
                }
            }
        }
    }

    // audio encoders
    for (i = 0; i < hb_audio_encoders_count; i++)
    {
        if (hb_audio_encoders[i].enabled)
        {
            // we still need to check
            hb_audio_encoders[i].enabled =
                hb_audio_encoder_is_enabled(hb_audio_encoders[i].item.codec);
        }
        if (hb_audio_encoders[i].enabled)
        {
            if (hb_audio_encoders_first_item == NULL)
            {
                hb_audio_encoders_first_item = &hb_audio_encoders[i].item;
            }
            else
            {
                ((hb_encoder_internal_t*)hb_audio_encoders_last_item)->next =
                    &hb_audio_encoders[i].item;
            }
            hb_audio_encoders_last_item = &hb_audio_encoders[i].item;
        }
    }
    // setup fallbacks
    for (i = 0; i < hb_audio_encoders_count; i++)
    {
        if (!hb_audio_encoders[i].enabled)
        {
            if ((hb_audio_encoders[i].item.codec & HB_ACODEC_MASK) &&
                (hb_audio_encoder_is_enabled(hb_audio_encoders[i].item.codec)))
            {
                // we have a specific fallback and it's enabled
                continue;
            }
            for (j = 0; j < hb_audio_encoders_count; j++)
            {
                if (hb_audio_encoders[j].enabled &&
                    hb_audio_encoders[j].gid == hb_audio_encoders[i].gid)
                {
                    hb_audio_encoders[i].item.codec = hb_audio_encoders[j].item.codec;
                    break;
                }
            }
            if (hb_audio_encoders[i].gid == HB_GID_ACODEC_AAC_HE)
            {
                // try to find an AAC fallback if no HE-AAC encoder is available
                for (j = 0; j < hb_audio_encoders_count; j++)
                {
                    if (hb_audio_encoders[j].enabled &&
                        hb_audio_encoders[j].gid == HB_GID_ACODEC_AAC)
                    {
                        hb_audio_encoders[i].item.codec = hb_audio_encoders[j].item.codec;
                        break;
                    }
                }
            }
        }
    }

    // video containers
    for (i = 0; i < hb_containers_count; i++)
    {
        if (hb_containers[i].enabled)
        {
            // we still need to check
            hb_containers[i].enabled =
                hb_container_is_enabled(hb_containers[i].item.format);
        }
        if (hb_containers[i].enabled)
        {
            if (hb_containers_first_item == NULL)
            {
                hb_containers_first_item = &hb_containers[i].item;
            }
            else
            {
                ((hb_container_internal_t*)hb_containers_last_item)->next =
                    &hb_containers[i].item;
            }
            hb_containers_last_item = &hb_containers[i].item;
        }
    }
    // setup fallbacks
    for (i = 0; i < hb_containers_count; i++)
    {
        if (!hb_containers[i].enabled)
        {
            if ((hb_containers[i].item.format & HB_MUX_MASK) &&
                (hb_container_is_enabled(hb_containers[i].item.format)))
            {
                // we have a specific fallback and it's enabled
                continue;
            }
            for (j = 0; j < hb_containers_count; j++)
            {
                if (hb_containers[j].enabled &&
                    hb_containers[j].gid == hb_containers[i].gid)
                {
                    hb_containers[i].item.format = hb_containers[j].item.format;
                    break;
                }
            }
        }
    }

    // we're done, yay!
    common_init_done = 1;
}

int hb_video_framerate_get_from_name(const char *name)
{
    if (name == NULL || *name == '\0')
        goto fail;

    int i;
    for (i = 0; i < hb_video_rates_count; i++)
    {
        if (!strcasecmp(hb_video_rates[i].item.name, name))
        {
            return hb_video_rates[i].item.rate;
        }
    }

fail:
    return -1;
}

const char* hb_video_framerate_get_name(int framerate)
{
    if (framerate > hb_video_rates_first_item->rate ||
        framerate < hb_video_rates_last_item ->rate)
        goto fail;

    const hb_rate_t *video_framerate = NULL;
    while ((video_framerate = hb_video_framerate_get_next(video_framerate)) != NULL)
    {
        if (video_framerate->rate == framerate)
        {
            return video_framerate->name;
        }
    }

fail:
    return NULL;
}

const char* hb_video_framerate_sanitize_name(const char *name)
{
    return hb_video_framerate_get_name(hb_video_framerate_get_from_name(name));
}

void hb_video_framerate_get_limits(int *low, int *high, int *clock)
{
    *low   = hb_video_rate_min;
    *high  = hb_video_rate_max;
    *clock = hb_video_rate_clock;
}

const hb_rate_t* hb_video_framerate_get_next(const hb_rate_t *last)
{
    if (last == NULL)
    {
        return hb_video_rates_first_item;
    }
    return ((hb_rate_internal_t*)last)->next;
}

int hb_video_framerate_get_close(hb_rational_t *framerate, double thresh)
{
    double            fps_in;
    const hb_rate_t * rate = NULL;
    int               result = -1;
    double            closest = thresh;

    fps_in = (double)framerate->num / framerate->den;
    while ((rate = hb_video_framerate_get_next(rate)) != NULL)
    {
        double fps = (double)hb_video_rate_clock / rate->rate;
        if (ABS(fps - fps_in) < closest)
        {
            result = rate->rate;
            closest = ABS(fps - fps_in);
        }
    }
    return result;
}

int hb_audio_samplerate_is_supported(int samplerate, uint32_t codec)
{
    switch (codec)
    {
        case HB_ACODEC_AC3:
        case HB_ACODEC_FFEAC3:
        case HB_ACODEC_CA_HAAC:
            // ca_haac can't do samplerates < 32 kHz
            // libav's E-AC-3 encoder can't do samplerates < 32 kHz
            // AC-3 < 32 kHz suffers from poor hardware compatibility
            if (samplerate < 32000 || samplerate > 48000)
                return 0;
            else
                return 1;
        case HB_ACODEC_FDK_HAAC:
            // fdk_haac can't do samplerates < 16 kHz
            if (samplerate < 16000 || samplerate > 48000)
                return 0;
             else
                return 1;
        case HB_ACODEC_OPUS:
            switch (samplerate)
            {
                // Opus only supports samplerates 8kHz, 12kHz, 16kHz,
                // 24kHz, 48kHz
                case 8000:
                case 12000:
                case 16000:
                case 24000:
                case 48000:
                    return 1;
                default:
                    return 0;
            }
        case HB_ACODEC_FFTRUEHD:
            switch (samplerate)
            {
                // TrueHD only supports samplerates 44.1kHz, 48kHz, 88.2kHz,
                // 96kHz, 176.4kHz, and 192kHz.
                case 44100:
                case 48000:
                case 88200:
                case 96000:
                case 176400:
                case 192000:
                    return 1;
                default:
                    return 0;
            }
        case HB_ACODEC_LAME:
        case HB_ACODEC_VORBIS:
        case HB_ACODEC_CA_AAC:
        case HB_ACODEC_FFAAC:
            if (samplerate > 48000)
            {
                return 0;
            }
            else
            {
                return 1;
            }
        default:
            return 1;
    }
}

int hb_audio_samplerate_get_sr_shift(int samplerate)
{
    /* sr_shift: 0 -> 48000, 44100, 32000 Hz
     *           1 -> 24000, 22050, 16000 Hz
     *           2 -> 12000, 11025,  8000 Hz
     *
     * also, since samplerates are sanitized downwards:
     *
     * (samplerate < 32000) implies (samplerate <= 24000)
     */
    return ((samplerate < 16000) ? 2 : (samplerate < 32000) ? 1 : 0);
}

int hb_audio_samplerate_get_from_name(const char *name)
{
    if (name == NULL || *name == '\0')
        goto fail;

    int i;
    for (i = 0; i < hb_audio_rates_count; i++)
    {
        if (!strcasecmp(hb_audio_rates[i].item.name, name))
        {
            return hb_audio_rates[i].item.rate;
        }
    }

    // maybe the samplerate was specified in Hz
    i = atoi(name);
    if (i >= hb_audio_rates_first_item->rate &&
        i <= hb_audio_rates_last_item ->rate)
    {
        return hb_audio_samplerate_find_closest(i, HB_ACODEC_INVALID);
    }

fail:
    return -1;
}

const char* hb_audio_samplerate_get_name(int samplerate)
{
    if (samplerate < hb_audio_rates_first_item->rate ||
        samplerate > hb_audio_rates_last_item ->rate)
        goto fail;

    const hb_rate_t *audio_samplerate = NULL;
    while ((audio_samplerate = hb_audio_samplerate_get_next(audio_samplerate)) != NULL)
    {
        if (audio_samplerate->rate == samplerate)
        {
            return audio_samplerate->name;
        }
    }

fail:
    return NULL;
}

const hb_rate_t* hb_audio_samplerate_get_next(const hb_rate_t *last)
{
    if (last == NULL)
    {
        return hb_audio_rates_first_item;
    }
    return ((hb_rate_internal_t*)last)->next;
}

const hb_rate_t* hb_audio_samplerate_get_next_for_codec(const hb_rate_t *last,
                                                        uint32_t codec)
{
    while ((last = hb_audio_samplerate_get_next(last)) != NULL)
        if (hb_audio_samplerate_is_supported(last->rate, codec))
            return last;

    // None found or end of list
    return NULL;
}

int hb_audio_samplerate_find_closest(int samplerate, uint32_t codec)
{
    const hb_rate_t * rate, * prev, * next;

    rate = prev = next = hb_audio_samplerate_get_next_for_codec(NULL, codec);

    if (rate == NULL)
    {
        // This codec doesn't support any samplerate
        return 0;
    }

    while (rate != NULL && next->rate < samplerate)
    {
        rate = hb_audio_samplerate_get_next_for_codec(rate, codec);
        if (rate != NULL)
        {
            prev = next;
            next = rate;
        }
    }

    int delta_prev = samplerate - prev->rate;
    int delta_next = next->rate - samplerate;
    if (delta_prev <= delta_next)
    {
        return prev->rate;
    }
    else
    {
        return next->rate;
    }
}

// Given an input bitrate, find closest match in the set of allowed bitrates
static int hb_audio_bitrate_find_closest(int bitrate)
{
    // Check if bitrate mode was disabled
    if (bitrate <= 0)
        return bitrate;

    int closest_bitrate            = hb_audio_bitrates_first_item->rate;
    const hb_rate_t *audio_bitrate = NULL;
    while ((audio_bitrate = hb_audio_bitrate_get_next(audio_bitrate)) != NULL)
    {
        if (bitrate == audio_bitrate->rate)
        {
            // valid bitrate
            closest_bitrate = audio_bitrate->rate;
            break;
        }
        if (bitrate > audio_bitrate->rate)
        {
            // bitrates are sanitized downwards
            closest_bitrate = audio_bitrate->rate;
        }
    }
    return closest_bitrate;
}

// Given an input bitrate, sanitize it.
// Check low and high limits and make sure it is in the set of allowed bitrates.
int hb_audio_bitrate_get_best(uint32_t codec, int bitrate, int samplerate,
                              int mixdown)
{
    int low, high;
    hb_audio_bitrate_get_limits(codec, samplerate, mixdown, &low, &high);
    if (bitrate > high)
        bitrate = high;
    if (bitrate < low)
        bitrate = low;
    return hb_audio_bitrate_find_closest(bitrate);
}

// Get the default bitrate for a given codec/samplerate/mixdown triplet.
int hb_audio_bitrate_get_default(uint32_t codec, int samplerate, int mixdown)
{
    if ((codec & HB_ACODEC_PASS_FLAG) || !(codec & HB_ACODEC_MASK))
        goto fail;

    int bitrate, nchannels, nlfe, sr_shift;
    /* full-bandwidth channels, sr_shift */
    nlfe      = hb_mixdown_get_low_freq_channel_count(mixdown);
    nchannels = hb_mixdown_get_discrete_channel_count(mixdown) - nlfe;
    sr_shift  = hb_audio_samplerate_get_sr_shift(samplerate);

    switch (codec)
    {
        case HB_ACODEC_FFALAC:
        case HB_ACODEC_FFALAC24:
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
        case HB_ACODEC_FFTRUEHD:
            goto fail;

        // 96, 224, 640 Kbps
        case HB_ACODEC_AC3:
            bitrate = (nchannels * 128) - (32 * (nchannels < 5));
            break;

        // Our E-AC-3 encoder is pretty similar to our AC-3 encoder but it does
        // allow for higher bitrates, should some users want a bit more quality
        // at the expense of compression efficiency - still, let's remain
        // compatible with passthru over S/PDIF by default: 384, 768, 1536 Kbps
        case HB_ACODEC_FFEAC3:
            bitrate = (nchannels * 384) - (128 * (nchannels - 2) * (nchannels > 2));
            break;

        case HB_ACODEC_CA_HAAC:
        case HB_ACODEC_FDK_HAAC:
            bitrate = nchannels * 32;
            break;

        case HB_ACODEC_OPUS:
        {
            int coupled = mixdown_get_opus_coupled_stream_count(mixdown);
            int uncoupled = nchannels + nlfe - 2 * coupled;

            bitrate = coupled * 96 + uncoupled * 64;
        } break;

        default:
            bitrate = nchannels * 80;
            break;
    }
    // sample_rate adjustment
    bitrate >>= sr_shift;
    return hb_audio_bitrate_get_best(codec, bitrate, samplerate, mixdown);

fail:
    return -1;
}

/* Get the bitrate low and high limits for a codec/samplerate/mixdown triplet.
 *
 * Encoder    1.0 channel    2.0 channels    5.1 channels    6.1 channels    7.1 channels
 * --------------------------------------------------------------------------------------
 *
 * ffaac
 * -----
 * supported samplerates: 8 - 48 kHz
 * libavcodec/aacenc.c defines a maximum bitrate:
 * -> 6144 * samplerate / 1024 bps (per channel, incl. LFE).
 * But output bitrates don't go as high as the theoretical maximums:
 * 12 kHz        61  (72)       123 (144)
 * 24 kHz       121 (144)       242 (288)
 * 48 kHz       236 (288)       472 (576)
 * Also, ffaac isn't a great encoder, so you don't want to allow too low a bitrate.
 * Limits: minimum of  32 Kbps per channel
 *         maximum of 192 Kbps per channel at 32 kHz, adjusted for sr_shift
 *         maximum of 256 Kbps per channel at 44.1-48 kHz, adjusted for sr_shift
 *
 * vorbis
 * ------
 * supported samplerates: 8 - 48 kHz
 * lib/modes/setup_*.h provides a range of allowed bitrates for various configurations.
 * for each samplerate, the highest minimums and lowest maximums are:
 *  8 kHz        Minimum  8 Kbps, maximum  32 Kbps (per channel, incl. LFE).
 * 12 kHz        Minimum 14 Kbps, maximum  44 Kbps (per channel, incl. LFE).
 * 16 kHz        Minimum 16 Kbps, maximum  86 Kbps (per channel, incl. LFE).
 * 24 kHz        Minimum 22 Kbps, maximum  86 Kbps (per channel, incl. LFE).
 * 32 kHz        Minimum 26 Kbps, maximum 190 Kbps (per channel, incl. LFE).
 * 48 kHz        Minimum 28 Kbps, maximum 240 Kbps (per channel, incl. LFE).
 * Limits: minimum of 14/22/28 Kbps per channel (8-12, 16-24, 32-48 kHz)
 *         maximum of 32/86/190/240 Kbps per channel (8-12, 16-24, 32, 44.1-48 kHz)
 *
 * lame
 * ----
 * supported samplerates: 8 - 48 kHz
 * lame_init_params() allows the following bitrates:
 * 12 kHz        Minimum  8 Kbps, maximum  64 Kbps
 * 24 kHz        Minimum  8 Kbps, maximum 160 Kbps
 * 48 kHz        Minimum 32 Kbps, maximum 320 Kbps
 * Limits: minimum of 8/8/32 Kbps (8-12, 16-24, 32-48 kHz)
 *         maximum of 64/160/320 Kbps (8-12, 16-24, 32-48 kHz)
 *
 * ffac3
 * -----
 * supported samplerates: 32 - 48 kHz (< 32 kHz disabled for compatibility reasons)
 * Dolby's encoder has a min. of 224 Kbps for 5 full-bandwidth channels (5.0, 5.1)
 * The maximum AC3 bitrate is 640 Kbps
 * Limits: minimum of 224/5 Kbps per full-bandwidth channel, maximum of 640 Kbps
 *
 * ffeac3
 * ------
 * supported samplerates: 32 - 48 kHz (< 32 kHz not supported by libav encoder)
 * Dolby's encoder has a min. of 224 Kbps for 5 full-bandwidth channels (5.0, 5.1)
 * The maximum bitrate is 128 bits per sample per second
 * Limits: minimum of 224/5 Kbps per full-bandwidth channel
 *         maximum of 128/1000 * samplerate Kbps
 *
 * ca_aac
 * ------
 * supported samplerates: 8 - 48 kHz
 * Core Audio API provides a range of allowed bitrates:
 *  8 kHz         8 -  24        16 -  48        40 - 112        48 - 144        56 - 160
 * 12 kHz        12 -  32        24 -  64        64 - 160        72 - 192        96 - 224
 * 16 kHz        12 -  48        24 -  96        64 - 224        72 - 288        96 - 320
 * 24 kHz        16 -  64        32 - 128        80 - 320        96 - 384       112 - 448
 * 32 kHz        24 -  96        48 - 192       128 - 448       144 - 576       192 - 640
 * 48 kHz        32 - 256        64 - 320       160 - 768       192 - 960       224 - 960
 * Limits:
 *  8 kHz -> minimum of  8 Kbps and maximum of  24 Kbps per full-bandwidth channel
 * 12 kHz -> minimum of 12 Kbps and maximum of  32 Kbps per full-bandwidth channel
 * 16 kHz -> minimum of 12 Kbps and maximum of  48 Kbps per full-bandwidth channel
 * 24 kHz -> minimum of 16 Kbps and maximum of  64 Kbps per full-bandwidth channel
 * 32 kHz -> minimum of 24 Kbps and maximum of  96 Kbps per full-bandwidth channel
 * 48 kHz -> minimum of 32 Kbps and maximum of 160 Kbps per full-bandwidth channel
 * 48 kHz ->                        maximum of +96 Kbps for Mono
 * Note: encCoreAudioInit() will sanitize any mistake made here.
 *
 * ca_haac
 * -------
 * supported samplerates: 32 - 48 kHz
 * Core Audio API provides a range of allowed bitrates:
 * 32 kHz         12 - 40         24 - 80        64 - 192          N/A           96 - 256
 * 48 kHz         16 - 40         32 - 80        80 - 192          N/A          112 - 256
 * Limits: minimum of 12 Kbps per full-bandwidth channel (<= 32 kHz)
 *         minimum of 16 Kbps per full-bandwidth channel ( > 32 kHz)
 *         maximum of 40 Kbps per full-bandwidth channel
 * Note: encCoreAudioInit() will sanitize any mistake made here.
 *
 * fdk_aac
 * -------
 * supported samplerates: 8 - 48 kHz
 * libfdk limits the bitrate to the following values:
 *  8 kHz              48              96             240
 * 12 kHz              72             144             360
 * 16 kHz              96             192             480
 * 24 kHz             144             288             720
 * 32 kHz             192             384             960
 * 48 kHz             288             576            1440
 * Limits: minimum of samplerate * 2/3 Kbps per full-bandwidth channel (see ca_aac)
 *         maximum of samplerate * 6.0 Kbps per full-bandwidth channel
 *
 * fdk_haac
 * --------
 * supported samplerates: 16 - 48 kHz
 * libfdk limits the bitrate to the following values:
 * 16 kHz         8 -  48        16 -  96        45 - 199
 * 24 kHz         8 -  63        16 - 127        45 - 266
 * 32 kHz         8 -  63        16 - 127        45 - 266
 * 48 kHz        12 -  63        16 - 127        50 - 266
 * Limits: minimum of 12 Kbps per full-bandwidth channel  (<= 32 kHz) (see ca_haac)
 *         minimum of 16 Kbps per full-bandwidth channel  ( > 32 kHz) (see ca_haac)
 *         maximum of 48,  96 or 192 Kbps (1.0, 2.0, 5.1) (<= 16 kHz)
 *         maximum of 64, 128 or 256 Kbps (1.0, 2.0, 5.1) ( > 16 kHz)
 */
void hb_audio_bitrate_get_limits(uint32_t codec, int samplerate, int mixdown,
                                 int *low, int *high)
{
    /*
     * samplerate == 0 means "auto" (same as source) and the UIs know the source
     * samplerate -- except where there isn't a source (audio defaults panel);
     * but we have enough info to return the global bitrate limits for this
     * mixdown, since the first/last samplerate are known to us and non-zero.
     */
    if (samplerate == 0)
    {
        int dummy;
        hb_audio_bitrate_get_limits(codec, hb_audio_rates_first_item->rate, mixdown, low, &dummy);
        hb_audio_bitrate_get_limits(codec, hb_audio_rates_last_item->rate, mixdown, &dummy, high);
        return;
    }

    /* samplerate, sr_shift */
    int sr_shift;
    samplerate = hb_audio_samplerate_find_closest(samplerate, codec);
    sr_shift = hb_audio_samplerate_get_sr_shift(samplerate);

    /* LFE, full-bandwidth channels */
    int lfe_count, nchannels;
    lfe_count = hb_mixdown_get_low_freq_channel_count(mixdown);
    nchannels = hb_mixdown_get_discrete_channel_count(mixdown) - lfe_count;

    switch (codec)
    {
        // Bitrates don't apply to "lossless" audio
        case HB_ACODEC_FFALAC:
        case HB_ACODEC_FFALAC24:
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
        case HB_ACODEC_FFTRUEHD:
            *low = *high = -1;
            return;

        case HB_ACODEC_AC3:
            *low  = 224 * nchannels / 5;
            *high = 640;
            break;

        case HB_ACODEC_FFEAC3:
            *low  = 224 * nchannels  /    5;
            *high = 128 * samplerate / 1000;
            break;

        case HB_ACODEC_CA_AAC:
        {
            switch (samplerate)
            {
                case 8000:
                    *low  = nchannels *  8;
                    *high = nchannels * 24;
                    break;

                case 11025:
                case 12000:
                    *low  = nchannels * 12;
                    *high = nchannels * 32;
                    break;

                case 16000:
                    *low  = nchannels * 12;
                    *high = nchannels * 48;
                    break;

                case 22050:
                case 24000:
                    *low  = nchannels * 16;
                    *high = nchannels * 64;
                    break;

                case 32000:
                    *low  = nchannels * 24;
                    *high = nchannels * 96;
                    break;

                case 44100:
                case 48000:
                default:
                    *low  = nchannels * 32;
                    *high = nchannels * (160 + (96 * (nchannels == 1)));
                    break;
            } break;
        }

        case HB_ACODEC_CA_HAAC:
            *low  = nchannels * (12 + (4 * (samplerate >= 44100)));
            *high = nchannels * 40;
            break;

        case HB_ACODEC_FDK_AAC:
            *low  = nchannels * samplerate * 2 / 3000;
            *high = nchannels * samplerate * 6 / 1000;
            break;

        case HB_ACODEC_FDK_HAAC:
            *low  = (nchannels * (12 + (4 * (samplerate >= 44100))));
            *high = (nchannels - (nchannels > 2)) * (48 +
                                                     (16 *
                                                      (samplerate >= 22050)));
            break;

        case HB_ACODEC_FFAAC:
            *low  = ((nchannels + lfe_count) * 32);
            *high = ((nchannels + lfe_count) *
                     ((192 + (64 * ((samplerate << sr_shift) >= 44100)))
                      >> sr_shift));
            break;

        case HB_ACODEC_LAME:
            *low  =  8 + (24 * (sr_shift < 1));
            *high = 64 + (96 * (sr_shift < 2)) + (160 * (sr_shift < 1));
            break;

        case HB_ACODEC_VORBIS:
            *low  = (nchannels + lfe_count) * (14 +
                                               (8 * (sr_shift < 2)) +
                                               (6 * (sr_shift < 1)));
            *high = (nchannels + lfe_count) * (32 +
                                               ( 54 * (sr_shift < 2)) +
                                               (104 * (sr_shift < 1)) +
                                               ( 50 * (samplerate >= 44100)));
            break;

        case HB_ACODEC_OPUS:
            *low  = (nchannels + lfe_count) * 6;
            *high = (nchannels + lfe_count) * 256;
            break;

        // Bitrates don't apply to passthru audio, but may apply if we
        // fall back to an encoder when the source can't be passed through.
        default:
            *low  = hb_audio_bitrates_first_item->rate;
            *high = hb_audio_bitrates_last_item ->rate;
            break;
    }

    // sanitize max. bitrate
    if (*high < hb_audio_bitrates_first_item->rate)
        *high = hb_audio_bitrates_first_item->rate;
    if (*high > hb_audio_bitrates_last_item ->rate)
        *high = hb_audio_bitrates_last_item ->rate;
}

const hb_rate_t* hb_audio_bitrate_get_next(const hb_rate_t *last)
{
    if (last == NULL)
    {
        return hb_audio_bitrates_first_item;
    }
    return ((hb_rate_internal_t*)last)->next;
}

const char * hb_audio_name_get_default(uint64_t layout, int mixdown)
{
    int mix_channels = 2;

    if (mixdown != HB_AMIXDOWN_NONE)
    {
        mix_channels = hb_mixdown_get_discrete_channel_count(mixdown);
    }
    else
    {
        mix_channels = hb_layout_get_discrete_channel_count(layout);
    }

    switch (mix_channels)
    {
        case 1:
            return "Mono";
            break;

        case 2:
            return "Stereo";
            break;

        default:
            return "Surround";
            break;
    }
}

int hb_audio_autonaming_behavior_get_from_name(const char *name)
{
    hb_audio_autonaming_behavior_t behavior = HB_AUDIO_AUTONAMING_NONE;

    if (name)
    {
        if (!strcasecmp(name, "all"))
        {
            behavior = HB_AUDIO_AUTONAMING_ALL;
        }
        else if (!strcasecmp(name, "unnamed"))
        {
            behavior = HB_AUDIO_AUTONAMING_UNNAMED;
        }
    }

    return behavior;
}

const char * hb_audio_name_generate(const char *name,
                                    uint64_t layout, int mixdown, int keep_name,
                                    hb_audio_autonaming_behavior_t behavior)
{
    const char *out = NULL;

    if (name == NULL || name[0] == 0)
    {
        name = NULL;
    }

    if (keep_name)
    {
        out = name;
    }

    if (name != NULL &&
        (!strcmp(name, "Mono") ||
         !strcmp(name, "Stereo") ||
         !strcmp(name, "Surround")))
    {
        out = NULL;
    }

    if (behavior == HB_AUDIO_AUTONAMING_ALL ||
        (behavior == HB_AUDIO_AUTONAMING_UNNAMED && (name == NULL || name[0] == 0)))
    {
        out = hb_audio_name_get_default(layout, mixdown);
    }

    return out;
}

// Get limits and hints for the UIs.
//
// granularity sets the minimum step increments that should be used
// (it's ok to round up to some nice multiple if you like)
//
// direction says whether 'low' limit is highest or lowest
// quality (direction 0 == lowest value is worst quality)
void hb_video_quality_get_limits(uint32_t codec, float *low, float *high,
                                 float *granularity, int *direction)
{
#if HB_PROJECT_FEATURE_QSV
    if (codec & HB_VCODEC_QSV_MASK)
    {
        return hb_qsv_video_quality_get_limits(codec, low, high, granularity,
                                               direction);
    }
#endif

    switch (codec)
    {
        /*
         * H.264/H.265: *low
         * = 51 - (QP_MAX_SPEC)
         * = 51 - (51 + QP_BD_OFFSET)
         * =  0 - (QP_BD_OFFSET)
         * =  0 - (6*(BIT_DEPTH-8))     (libx264)
         * =  0 - (6*(X265_DEPTH-8))    (libx265)
         */
        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X265_8BIT:
        case HB_VCODEC_FFMPEG_VCE_H264:
        case HB_VCODEC_FFMPEG_VCE_H265:
        case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
            *direction   = 1;
            *granularity = 0.1;
            *low         = 0.;
            *high        = 51.;
            break;
        case HB_VCODEC_FFMPEG_NVENC_H264:
        case HB_VCODEC_FFMPEG_NVENC_H265:
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
            *direction   = 1;
            *granularity = 0.1;
            *low         = 1.;
            *high        = 51.;
            break;
        case HB_VCODEC_FFMPEG_NVENC_AV1:
        case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
            *direction   = 1;
            *granularity = 0.1;
            *low         = 1.;
            *high        = 63.;
            break;
        case HB_VCODEC_X264_10BIT:
        case HB_VCODEC_X265_10BIT:
            *direction   = 1;
            *granularity = 0.1;
            *low         = -12.;
            *high        = 51.;
            break;
        case HB_VCODEC_FFMPEG_VCE_AV1:
            *direction   = 1;
            *granularity = 1;
            *low         = 0.;
            *high        = 255.;
            break;
        case HB_VCODEC_X265_12BIT:
            *direction   = 1;
            *granularity = 0.1;
            *low         = -24.;
            *high        = 51.;
            break;
        case HB_VCODEC_X265_16BIT:
            *direction   = 1;
            *granularity = 0.1;
            *low         = -48.;
            *high        = 51.;
            break;

        case HB_VCODEC_THEORA:
            *direction   = 0;
            *granularity = 1.;
            *low         = 0.;
            *high        = 63.;
            break;

        case HB_VCODEC_FFMPEG_VP8:
        case HB_VCODEC_FFMPEG_VP9:
        case HB_VCODEC_FFMPEG_VP9_10BIT:
        case HB_VCODEC_SVT_AV1:
        case HB_VCODEC_SVT_AV1_10BIT:
            *direction   = 1;
            *granularity = 1.;
            *low         = 0.;
            *high        = 63.;
            break;

        case HB_VCODEC_VT_H264:
        case HB_VCODEC_VT_H265:
        case HB_VCODEC_VT_H265_10BIT:
            *direction   = 0;
            *granularity = 1.;
            *low         = 0.;
            *high        = 100.;
            break;

        case HB_VCODEC_FFMPEG_MF_H264:
        case HB_VCODEC_FFMPEG_MF_H265:
        case HB_VCODEC_FFMPEG_MF_AV1:
            *direction   = 0;
            *granularity = 1;
            *low         = 0;
            *high        = 100;
            break;

        case HB_VCODEC_FFMPEG_FFV1:
            *direction   = 0;
            *granularity = 1;
            *low         = 0;
            *high        = 0;
            break;

        case HB_VCODEC_FFMPEG_MPEG2:
        case HB_VCODEC_FFMPEG_MPEG4:
        default:
            *direction   = 1;
            *granularity = 1.;
            *low         = 1.;
            *high        = 31.;
            break;
    }
}

const char* hb_video_quality_get_name(uint32_t codec)
{
#if HB_PROJECT_FEATURE_QSV
    if (codec & HB_VCODEC_QSV_MASK)
    {
        return hb_qsv_video_quality_get_name(codec);
    }
#endif

    switch (codec)
    {
        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X264_10BIT:
        case HB_VCODEC_X265_8BIT:
        case HB_VCODEC_X265_10BIT:
        case HB_VCODEC_X265_12BIT:
        case HB_VCODEC_X265_16BIT:
        case HB_VCODEC_SVT_AV1:
        case HB_VCODEC_SVT_AV1_10BIT:
            return "RF";

        case HB_VCODEC_FFMPEG_VP8:
        case HB_VCODEC_FFMPEG_VP9:
        case HB_VCODEC_FFMPEG_VP9_10BIT:
        case HB_VCODEC_FFMPEG_NVENC_H264:
        case HB_VCODEC_FFMPEG_NVENC_H265:
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
        case HB_VCODEC_FFMPEG_NVENC_AV1:
        case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
        case HB_VCODEC_FFMPEG_VCE_H264:
        case HB_VCODEC_FFMPEG_VCE_H265:
        case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
        case HB_VCODEC_FFMPEG_VCE_AV1:
        case HB_VCODEC_VT_H264:
        case HB_VCODEC_VT_H265:
        case HB_VCODEC_VT_H265_10BIT:
            return "CQ";

        case HB_VCODEC_FFMPEG_MF_H264:
        case HB_VCODEC_FFMPEG_MF_H265:
        case HB_VCODEC_FFMPEG_MF_AV1:
            return "Quality";

        default:
            return "QP";
    }
}

int hb_video_quality_is_supported(uint32_t codec)
{
    switch (codec)
    {
#ifdef __APPLE__
        case HB_VCODEC_VT_H264:
        case HB_VCODEC_VT_H265:
        case HB_VCODEC_VT_H265_10BIT:
            return hb_vt_is_constant_quality_available(codec);
#endif

        default:
            return 1;
    }
}

int hb_video_bitrate_is_supported(uint32_t codec)
{
    switch (codec)
    {
        case HB_VCODEC_FFMPEG_FFV1:
            return 0;

        default:
            return 1;
    }
}

int hb_video_multipass_is_supported(uint32_t codec, int constant_quality)
{
    switch (codec)
    {
#ifdef __APPLE__
        case HB_VCODEC_VT_H264:
        case HB_VCODEC_VT_H265:
        case HB_VCODEC_VT_H265_10BIT:
            return !constant_quality && hb_vt_is_multipass_available(codec);
#endif

        case HB_VCODEC_FFMPEG_MF_H264:
        case HB_VCODEC_FFMPEG_MF_H265:
        case HB_VCODEC_FFMPEG_MF_AV1:
        case HB_VCODEC_FFMPEG_VCE_H264:
        case HB_VCODEC_FFMPEG_VCE_H265:
        case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
        case HB_VCODEC_FFMPEG_VCE_AV1:
        case HB_VCODEC_FFMPEG_NVENC_H264:
        case HB_VCODEC_FFMPEG_NVENC_H265:
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
        case HB_VCODEC_FFMPEG_NVENC_AV1:
        case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
        case HB_VCODEC_FFMPEG_QSV_H264:
        case HB_VCODEC_FFMPEG_QSV_H265:
        case HB_VCODEC_FFMPEG_QSV_H265_10BIT:
        case HB_VCODEC_FFMPEG_QSV_AV1:
        case HB_VCODEC_FFMPEG_QSV_AV1_10BIT:
            return 0;

        case HB_VCODEC_FFMPEG_VP9:
        case HB_VCODEC_FFMPEG_VP9_10BIT:
        case HB_VCODEC_FFMPEG_FFV1:
            return 1;

        default:
            return !constant_quality;
    }
}

int hb_video_hdr_dynamic_metadata_is_supported(uint32_t codec, int hdr_dynamic_metadata, int profile)
{
    if (hdr_dynamic_metadata == HB_HDR_DYNAMIC_METADATA_HDR10PLUS)
    {
        switch (codec)
        {
            case HB_VCODEC_X265_10BIT:
            case HB_VCODEC_VT_H265_10BIT:
            case HB_VCODEC_SVT_AV1_10BIT:
                return 1;
        }
    }
#if HB_PROJECT_FEATURE_LIBDOVI
    else if (hdr_dynamic_metadata == HB_HDR_DYNAMIC_METADATA_DOVI)
    {
        if (profile != 5 &&
            profile != 7 &&
            profile != 8 &&
            profile != 10)
        {
            return 0;
        }

        switch (codec)
        {
            case HB_VCODEC_X265_10BIT:
            case HB_VCODEC_VT_H265_10BIT:
            case HB_VCODEC_SVT_AV1_10BIT:
                return 1;
        }
    }
#endif
    return 0;
}

int hb_video_encoder_is_supported(int encoder)
{
    const hb_encoder_t *video_encoder = NULL;
    while ((video_encoder = hb_video_encoder_get_next(video_encoder)) != NULL)
    {
        if (video_encoder->codec == encoder)
        {
            return 1;
        }
    }
    return 0;
}

int hb_video_encoder_get_count_of_analysis_passes(int encoder)
{
    switch (encoder)
    {
        default:
            return 1;
    }
}

int hb_video_encoder_pix_fmt_is_supported(int encoder, int pix_fmt, const char *profile)
{
    const int *pix_fmts = hb_video_encoder_get_pix_fmts(encoder, profile);
    while (*pix_fmts != AV_PIX_FMT_NONE)
    {
        if (pix_fmt == *pix_fmts)
        {
            return 1;
        }
        pix_fmts++;
    }
    return 0;
}

int hb_video_encoder_get_depth(int encoder)
{
    switch (encoder)
    {
#if HB_PROJECT_FEATURE_QSV
        case HB_VCODEC_FFMPEG_QSV_H265_10BIT:
        case HB_VCODEC_FFMPEG_QSV_AV1_10BIT:
#endif
#ifdef __APPLE__
        case HB_VCODEC_VT_H265_10BIT:
#endif
        case HB_VCODEC_X264_10BIT:
        case HB_VCODEC_X265_10BIT:
        case HB_VCODEC_SVT_AV1_10BIT:
        case HB_VCODEC_FFMPEG_VP9_10BIT:
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
        case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
        case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
            return 10;
        case HB_VCODEC_X265_12BIT:
            return 12;
        case HB_VCODEC_X265_16BIT:
            return 16;
        default:
            return 8;
    }
}

static const char * const hb_empty_list_names[] =
{
    "auto", NULL
};

const char* const* hb_video_encoder_get_presets(int encoder)
{
    if (encoder & HB_VCODEC_FFMPEG_MASK)
    {
        return hb_av_preset_get_names(encoder);
    }

    switch (encoder)
    {
        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X264_10BIT:
            return x264_preset_names;

#if HB_PROJECT_FEATURE_X265
        case HB_VCODEC_X265_8BIT:
        case HB_VCODEC_X265_10BIT:
        case HB_VCODEC_X265_12BIT:
        case HB_VCODEC_X265_16BIT:
            return x265_preset_names;
#endif
#ifdef __APPLE__
        case HB_VCODEC_VT_H264:
        case HB_VCODEC_VT_H265:
        case HB_VCODEC_VT_H265_10BIT:
            return hb_vt_preset_get_names(encoder);
#endif

        case HB_VCODEC_SVT_AV1:
        case HB_VCODEC_SVT_AV1_10BIT:
            return hb_av1_svt_preset_names;

        default:
            return hb_empty_list_names;
    }
}

static const char * const hb_empty_tune_names[] =
{
    "none", NULL
};


const char* const* hb_video_encoder_get_tunes(int encoder)
{
    if (encoder & HB_VCODEC_FFMPEG_MASK)
    {
        return hb_av_tune_get_names(encoder);
    }

    switch (encoder)
    {
        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X264_10BIT:
            return hb_x264_tune_names;

#if HB_PROJECT_FEATURE_X265
        case HB_VCODEC_X265_8BIT:
        case HB_VCODEC_X265_10BIT:
        case HB_VCODEC_X265_12BIT:
        case HB_VCODEC_X265_16BIT:
            return hb_x265_tune_names;
#endif

        case HB_VCODEC_SVT_AV1:
        case HB_VCODEC_SVT_AV1_10BIT:
            return hb_av1_svt_tune_names;

        default:
            return hb_empty_tune_names;
    }
}

const char* const* hb_video_encoder_get_profiles(int encoder)
{
#if HB_PROJECT_FEATURE_QSV
    if (encoder & HB_VCODEC_QSV_MASK)
    {
        return hb_qsv_profile_get_names(encoder);
    }
#endif

    switch (encoder)
    {
        case HB_VCODEC_X264_8BIT:
            return hb_x264_profile_names_8bit;
        case HB_VCODEC_X264_10BIT:
            return hb_x264_profile_names_10bit;

        case HB_VCODEC_X265_8BIT:
            return hb_x265_profile_names_8bit;
        case HB_VCODEC_X265_10BIT:
            return hb_x265_profile_names_10bit;
        case HB_VCODEC_X265_12BIT:
            return hb_x265_profile_names_12bit;
        case HB_VCODEC_X265_16BIT:
            return hb_h265_profile_names_16bit;

#if HB_PROJECT_FEATURE_VCE
        case HB_VCODEC_FFMPEG_VCE_H264:
            return hb_vce_h264_profile_names;
        case HB_VCODEC_FFMPEG_VCE_H265:
            return hb_vce_h265_profile_names;
        case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
            return hb_vce_h265_10bit_profile_names;
        case HB_VCODEC_FFMPEG_VCE_AV1:
            return hb_vce_av1_profile_names;
#endif
#if __APPLE__
        case HB_VCODEC_VT_H264:
        case HB_VCODEC_VT_H265:
        case HB_VCODEC_VT_H265_10BIT:
            return hb_vt_profile_get_names(encoder);
#endif
        case HB_VCODEC_FFMPEG_NVENC_H264:
        case HB_VCODEC_FFMPEG_NVENC_H265:
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
        case HB_VCODEC_FFMPEG_MF_H264:
        case HB_VCODEC_FFMPEG_MF_H265:
        case HB_VCODEC_FFMPEG_MF_AV1:
            return hb_av_profile_get_names(encoder);

        case HB_VCODEC_SVT_AV1:
        case HB_VCODEC_SVT_AV1_10BIT:
            return hb_av1_svt_profile_names;

        default:
            return hb_empty_list_names;
    }
}

const char* const* hb_video_encoder_get_levels(int encoder)
{
#if HB_PROJECT_FEATURE_QSV
    if (encoder & HB_VCODEC_QSV_MASK)
    {
        return hb_qsv_level_get_names(encoder);
    }
#endif

    if (encoder & HB_VCODEC_FFMPEG_MASK)
    {
        return hb_av_level_get_names(encoder);
    }

    switch (encoder)
    {
        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X264_10BIT:
            return hb_h264_level_names;

#if HB_PROJECT_FEATURE_VCE
     case HB_VCODEC_FFMPEG_VCE_H264:
            return hb_vce_h264_level_names; // Not quite the same as x264
#endif

        case HB_VCODEC_X265_8BIT:
        case HB_VCODEC_X265_10BIT:
        case HB_VCODEC_X265_12BIT:
        case HB_VCODEC_X265_16BIT:
            return hb_h265_level_names;

#ifdef __APPLE__
        case HB_VCODEC_VT_H264:
        case HB_VCODEC_VT_H265:
        case HB_VCODEC_VT_H265_10BIT:
            return hb_vt_level_get_names(encoder);
#endif

        case HB_VCODEC_SVT_AV1:
        case HB_VCODEC_SVT_AV1_10BIT:
            return hb_av1_level_names;

        default:
            return hb_empty_list_names;
    }
}

static const enum AVPixelFormat standard_pix_fmts[] =
{
    AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat standard_422_pix_fmts[] =
{
    AV_PIX_FMT_YUV422P, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat standard_444_pix_fmts[] =
{
    AV_PIX_FMT_YUV444P, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat standard_10bit_only_pix_fmts[] =
{
    AV_PIX_FMT_YUV420P10, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat standard_10bit_pix_fmts[] =
{
    AV_PIX_FMT_YUV420P10, AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat standard_422_10bit_pix_fmts[] =
{
    AV_PIX_FMT_YUV422P10, AV_PIX_FMT_YUV422P, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat standard_444_10bit_pix_fmts[] =
{
    AV_PIX_FMT_YUV444P10, AV_PIX_FMT_YUV444P, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat standard_12bit_pix_fmts[] =
{
    AV_PIX_FMT_YUV420P12, AV_PIX_FMT_YUV420P10, AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat standard_422_12bit_pix_fmts[] =
{
    AV_PIX_FMT_YUV422P12, AV_PIX_FMT_YUV422P10, AV_PIX_FMT_YUV422P, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat standard_444_12bit_pix_fmts[] =
{
    AV_PIX_FMT_YUV444P12, AV_PIX_FMT_YUV444P10, AV_PIX_FMT_YUV444P, AV_PIX_FMT_NONE
};

const int* hb_video_encoder_get_pix_fmts(int encoder, const char *profile)
{
    if (encoder & HB_VCODEC_FFMPEG_MASK)
    {
        return hb_av_get_pix_fmts(encoder);
    }

    switch (encoder)
    {
        case HB_VCODEC_X264_8BIT:
        {
            if (profile && !strcasecmp(profile, "high422"))
            {
                return standard_422_pix_fmts;
            }
            else if (profile && !strcasecmp(profile, "high444"))
            {
                return standard_444_pix_fmts;
            }
            else
            {
                return standard_pix_fmts;
            }
        }
        case HB_VCODEC_X264_10BIT:
        {
            if (profile && !strcasecmp(profile, "high422"))
            {
                return standard_422_10bit_pix_fmts;
            }
            else if (profile && !strcasecmp(profile, "high444"))
            {
                return standard_444_10bit_pix_fmts;
            }
            else
            {
                return standard_10bit_pix_fmts;
            }
        }
#if HB_PROJECT_FEATURE_X265
        case HB_VCODEC_X265_8BIT:
        {
            if (profile && (!strcasecmp(profile, "main444-8") ||
                            !strcasecmp(profile, "main444-intra")))
            {
                return standard_444_pix_fmts;
            }
            else
            {
                return standard_pix_fmts;
            }
        }
        case HB_VCODEC_X265_10BIT:
        {
            if (profile && (!strcasecmp(profile, "main422-10") ||
                            !strcasecmp(profile, "main422-10-intra")))
            {
                return standard_422_10bit_pix_fmts;
            }
            else if (profile && (!strcasecmp(profile, "main444-10") ||
                                 !strcasecmp(profile, "main444-10-intra")))
            {
                return standard_444_10bit_pix_fmts;
            }
            else
            {
                return standard_10bit_pix_fmts;
            }
        }
        case HB_VCODEC_X265_12BIT:
        {
            if (profile && (!strcasecmp(profile, "main422-12") ||
                            !strcasecmp(profile, "main422-12-intra")))
            {
                return standard_422_12bit_pix_fmts;
            }
            else if (profile && (!strcasecmp(profile, "main444-12") ||
                                 !strcasecmp(profile, "main444-12-intra")))
            {
                return standard_444_12bit_pix_fmts;
            }

            else
            {
                return standard_12bit_pix_fmts;
            }
        }
#endif
#if __APPLE__
        case HB_VCODEC_VT_H264:
        case HB_VCODEC_VT_H265:
        case HB_VCODEC_VT_H265_10BIT:
            return hb_vt_get_pix_fmts(encoder);
#endif

        case HB_VCODEC_SVT_AV1_10BIT:
            return standard_10bit_only_pix_fmts;

        default:
            return standard_pix_fmts;
    }
}

// Get limits and hints for the UIs.
//
// granularity sets the minimum step increments that should be used
// (it's ok to round up to some nice multiple if you like)
//
// direction says whether 'low' limit is highest or lowest
// quality (direction 0 == lowest value is worst quality)
void hb_audio_quality_get_limits(uint32_t codec, float *low, float *high,
                                 float *granularity, int *direction)
{
    switch (codec)
    {
        case HB_ACODEC_FFAAC:
            *direction   = 0;
            *granularity = 1.;
            *low         = 1.;
            *high        = 5.;
            break;

        case HB_ACODEC_FDK_HAAC:
        case HB_ACODEC_FDK_AAC:
            *direction   = 0;
            *granularity = 1.;
            *low         = 1.;
            *high        = 5.;
            break;

        case HB_ACODEC_LAME:
            *direction   = 1;
            *granularity = 0.5;
            *low         = 0.;
            *high        = 10.;
            break;

        case HB_ACODEC_VORBIS:
            *direction   = 0;
            *granularity = 0.5;
            *low         = -2.;
            *high        = 10.;
            break;

        case HB_ACODEC_CA_AAC:
            *direction   = 0;
            *granularity = 9.;
            *low         = 1.;
            *high        = 127.;
            break;

        default:
            *direction   = 0;
            *granularity = 1.;
            *low = *high = HB_INVALID_AUDIO_QUALITY;
            break;
    }
}

float hb_audio_quality_get_best(uint32_t codec, float quality)
{
    int direction;
    float low, high, granularity;
    hb_audio_quality_get_limits(codec, &low, &high, &granularity, &direction);
    if (quality > high)
        quality = high;
    if (quality < low)
        quality = low;
    return quality;
}

float hb_audio_quality_get_default(uint32_t codec)
{
    switch (codec)
    {
        case HB_ACODEC_FFAAC:
            return 3.;

        case HB_ACODEC_FDK_HAAC:
        case HB_ACODEC_FDK_AAC:
            return 3.;

        case HB_ACODEC_LAME:
            return 2.;

        case HB_ACODEC_VORBIS:
            return 5.;

        case HB_ACODEC_CA_AAC:
            return 91.;

        default:
            return HB_INVALID_AUDIO_QUALITY;
    }
}

// Get limits and hints for the UIs.
//
// granularity sets the minimum step increments that should be used
// (it's ok to round up to some nice multiple if you like)
//
// direction says whether 'low' limit is highest or lowest
// compression level (direction 0 == lowest value is worst compression level)
void hb_audio_compression_get_limits(uint32_t codec, float *low, float *high,
                                     float *granularity, int *direction)
{
    switch (codec)
    {
        case HB_ACODEC_FFALAC:
        case HB_ACODEC_FFALAC24:
            *direction   = 0;
            *granularity = 1.;
            *high        = 2.;
            *low         = 0.;
            break;

        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
            *direction   = 0;
            *granularity = 1.;
            *high        = 12.;
            *low         = 0.;
            break;

        case HB_ACODEC_LAME:
            *direction   = 1;
            *granularity = 1.;
            *high        = 9.;
            *low         = 0.;
            break;

        case HB_ACODEC_OPUS:
            *direction   = 0;
            *granularity = 1.;
            *high        = 10.;
            *low         = 0.;
            break;

        default:
            *direction   = 0;
            *granularity = 1.;
            *low = *high = -1.;
            break;
    }
}

float hb_audio_compression_get_best(uint32_t codec, float compression)
{
    int direction;
    float low, high, granularity;
    hb_audio_compression_get_limits(codec, &low, &high, &granularity, &direction);
    if( compression > high )
        compression = high;
    if( compression < low )
        compression = low;
    return compression;
}

float hb_audio_compression_get_default(uint32_t codec)
{
    switch (codec)
    {
        case HB_ACODEC_FFALAC:
        case HB_ACODEC_FFALAC24:
            return 2.;

        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
            return 5.;

        case HB_ACODEC_LAME:
            return 2.;

        case HB_ACODEC_OPUS:
            return 10.;

        default:
            return -1.;
    }
}

int hb_audio_dither_get_default()
{
    // "auto"
    return hb_audio_dithers_first_item->method;
}

int hb_audio_dither_get_default_method()
{
    /*
     * input could be s16 (possibly already dithered) converted to flt, so
     * let's use a "low-risk" dither algorithm (standard triangular).
     */
    return SWR_DITHER_TRIANGULAR;
}

int hb_audio_dither_is_supported(uint32_t codec, int depth)
{
    // Since dithering is performed by swresample, all codecs are supported
    switch (codec)
    {
        case HB_ACODEC_FFFLAC:
            if (depth == 0 || depth > 16)
                return 1;
    }
    return 0;
}

int hb_audio_dither_get_from_name(const char *name)
{
    if (name == NULL || *name == '\0')
        goto fail;

    int i;
    for ( i = 0; i < hb_audio_dithers_count; i++)
    {
        if (!strcasecmp(hb_audio_dithers[i].item.short_name,  name) ||
            !strcasecmp(hb_audio_dithers[i].item.description, name))
        {
            return hb_audio_dithers[i].item.method;
        }
    }

fail:
    return hb_audio_dither_get_default();
}

const char* hb_audio_dither_get_description(int method)
{
    if (method < hb_audio_dithers_first_item->method ||
        method > hb_audio_dithers_last_item ->method)
        goto fail;

    const hb_dither_t *audio_dither = NULL;
    while ((audio_dither = hb_audio_dither_get_next(audio_dither)) != NULL)
    {
        if (audio_dither->method == method)
        {
            return audio_dither->description;
        }
    }

fail:
    return NULL;
}

const hb_dither_t* hb_audio_dither_get_next(const hb_dither_t *last)
{
    if (last == NULL)
    {
        return hb_audio_dithers_first_item;
    }
    return ((hb_dither_internal_t*)last)->next;
}

static int mixdown_get_opus_coupled_stream_count(int mixdown)
{
    switch (mixdown)
    {
        case HB_AMIXDOWN_7POINT1:
        case HB_AMIXDOWN_6POINT1:
            return 3;

        case HB_AMIXDOWN_5POINT1:
            return 2;

        case HB_AMIXDOWN_MONO:
        case HB_AMIXDOWN_LEFT:
        case HB_AMIXDOWN_RIGHT:
            return 0;

        case HB_AMIXDOWN_NONE:
        case HB_INVALID_AMIXDOWN:
        case HB_AMIXDOWN_5_2_LFE:
            // The 5F/2R/LFE configuration is currently not supported by Opus,
            // so don't set coupled streams.
            return 0;

        default:
            return 1;
    }
}

int hb_mixdown_is_supported(int mixdown, uint32_t codec, uint64_t layout)
{
    return (hb_mixdown_has_codec_support(mixdown, codec) &&
            hb_mixdown_has_remix_support(mixdown, layout));
}

int hb_mixdown_has_codec_support(int mixdown, uint32_t codec)
{
    // Passthru, only "None" mixdown is supported
    if (codec & HB_ACODEC_PASS_FLAG)
        return (mixdown == HB_AMIXDOWN_NONE);

    // Not passthru, "None" mixdown never supported
    if (mixdown == HB_AMIXDOWN_NONE)
        return 0;

    switch (codec)
    {
        case HB_ACODEC_VORBIS:
        case HB_ACODEC_FFALAC:
        case HB_ACODEC_FFALAC24:
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
        case HB_ACODEC_OPUS:
        case HB_ACODEC_CA_AAC:
        case HB_ACODEC_CA_HAAC:
        case HB_ACODEC_FFAAC:
            return (mixdown <= HB_AMIXDOWN_7POINT1);

        case HB_ACODEC_LAME:
            return (mixdown <= HB_AMIXDOWN_DOLBYPLII);

        case HB_ACODEC_FDK_AAC:
        case HB_ACODEC_FDK_HAAC:
            return ((mixdown <= HB_AMIXDOWN_5POINT1) ||
                    (mixdown == HB_AMIXDOWN_7POINT1));

        default:
            return (mixdown <= HB_AMIXDOWN_5POINT1);
    }
}

int hb_mixdown_has_remix_support(int mixdown, uint64_t layout)
{
    /*
     * Where there isn't a source (e.g. audio defaults panel), we have no input
     * layout; assume remix support, as the mixdown will be sanitized later on.
     */
    if (!layout)
    {
        return 1;
    }
    switch (mixdown)
    {
        // stereo + front left/right of center
        case HB_AMIXDOWN_5_2_LFE:
            return ((layout & AV_CH_FRONT_LEFT_OF_CENTER) &&
                    (layout & AV_CH_FRONT_RIGHT_OF_CENTER) &&
                    (layout & AV_CH_LAYOUT_STEREO) == AV_CH_LAYOUT_STEREO);

        // 7.0 or better
        case HB_AMIXDOWN_7POINT1:
            return ((layout & AV_CH_LAYOUT_7POINT0) == AV_CH_LAYOUT_7POINT0);

        // 6.0 or better
        case HB_AMIXDOWN_6POINT1:
            return ((layout & AV_CH_LAYOUT_7POINT0) == AV_CH_LAYOUT_7POINT0 ||
                    (layout & AV_CH_LAYOUT_6POINT0) == AV_CH_LAYOUT_6POINT0 ||
                    (layout & AV_CH_LAYOUT_HEXAGONAL) == AV_CH_LAYOUT_HEXAGONAL);

        // stereo + either of front center, side or back left/right, back center
        case HB_AMIXDOWN_5POINT1:
            return ((layout & AV_CH_LAYOUT_2_1) == AV_CH_LAYOUT_2_1 ||
                    (layout & AV_CH_LAYOUT_2_2) == AV_CH_LAYOUT_2_2 ||
                    (layout & AV_CH_LAYOUT_QUAD) == AV_CH_LAYOUT_QUAD ||
                    (layout & AV_CH_LAYOUT_SURROUND) == AV_CH_LAYOUT_SURROUND);

        // stereo + either of side or back left/right, back center
        // also, allow Dolby Surround output if the input is already Dolby
        case HB_AMIXDOWN_DOLBY:
        case HB_AMIXDOWN_DOLBYPLII:
            return ((layout & AV_CH_LAYOUT_2_1) == AV_CH_LAYOUT_2_1 ||
                    (layout & AV_CH_LAYOUT_2_2) == AV_CH_LAYOUT_2_2 ||
                    (layout & AV_CH_LAYOUT_QUAD) == AV_CH_LAYOUT_QUAD ||
                    (layout == AV_CH_LAYOUT_STEREO_DOWNMIX && // decavcodecaBSInfo tells us the input signals matrix encoding
                     mixdown == HB_AMIXDOWN_DOLBY)); // allows signaling matrix encoding in output w/encoders that support it

        // more than 1 channel
        case HB_AMIXDOWN_STEREO:
            return (hb_layout_get_discrete_channel_count(layout) > 1);

        /*
         * The following mixdowns have a very specific purpose!!!
         *
         * Given 2-channel input, they allow discarding either of the right or
         * left channel to produce mono output instead of downmixing (the latter
         * may clip or lower volume depending on whether the downmix coefficients
         * are normalized or not). They are meant to be used for sources which
         * are known to actually be Mono (identical content in the L/R channels
         * or one of L/R is actually empty). They hardly make any sense when the
         * input has more than two channels (be they discrete or matrix-encoded).
         *
         * Thus specifically only allow them for non-matrix Stereo input (layout == STEREO).
         */
        case HB_AMIXDOWN_LEFT:
        case HB_AMIXDOWN_RIGHT:
            return (layout == AV_CH_LAYOUT_STEREO);

        // mono remix always supported
        // HB_AMIXDOWN_NONE always supported (for Passthru)
        case HB_AMIXDOWN_MONO:
        case HB_AMIXDOWN_NONE:
            return 1;

        // unknown mixdown, should never happen
        default:
            return 0;
    }
}

int hb_mixdown_get_discrete_channel_count(int amixdown)
{
    switch (amixdown)
    {
        case HB_AMIXDOWN_5_2_LFE:
        case HB_AMIXDOWN_7POINT1:
            return 8;

        case HB_AMIXDOWN_6POINT1:
            return 7;

        case HB_AMIXDOWN_5POINT1:
            return 6;

        case HB_AMIXDOWN_MONO:
        case HB_AMIXDOWN_LEFT:
        case HB_AMIXDOWN_RIGHT:
            return 1;

        case HB_AMIXDOWN_NONE:
            return 0;

        default:
            return 2;
    }
}

int hb_mixdown_get_low_freq_channel_count(int amixdown)
{
    switch (amixdown)
    {
        case HB_AMIXDOWN_5POINT1:
        case HB_AMIXDOWN_6POINT1:
        case HB_AMIXDOWN_7POINT1:
        case HB_AMIXDOWN_5_2_LFE:
            return 1;

        default:
            return 0;
    }
}

int hb_mixdown_get_best(uint32_t codec, uint64_t layout, int mixdown)
{
    // Passthru, only "None" mixdown is supported
    if (codec & HB_ACODEC_PASS_FLAG)
        return HB_AMIXDOWN_NONE;

    int best_mixdown                  = HB_INVALID_AMIXDOWN;
    const hb_mixdown_t *audio_mixdown = hb_mixdown_get_next(NULL);
    // test all non-None mixdowns while the value is <= the requested mixdown
    // HB_INVALID_AMIXDOWN means the highest supported mixdown was requested
    while ((audio_mixdown = hb_mixdown_get_next(audio_mixdown)) != NULL)
    {
        if ((mixdown == HB_INVALID_AMIXDOWN || audio_mixdown->amixdown <= mixdown) &&
            (hb_mixdown_is_supported(audio_mixdown->amixdown, codec, layout)))
        {
            best_mixdown = audio_mixdown->amixdown;
        }
    }
    return best_mixdown;
}

int hb_mixdown_get_default(uint32_t codec, uint64_t layout)
{
    int mixdown;
    switch (codec)
    {
        // the FLAC encoder defaults to the best mixdown up to 7.1
        case HB_ACODEC_FFALAC:
        case HB_ACODEC_FFALAC24:
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
        case HB_ACODEC_OPUS:
        case HB_ACODEC_CA_AAC:
        case HB_ACODEC_CA_HAAC:
        case HB_ACODEC_FFAAC:
        case HB_ACODEC_FDK_AAC:
        case HB_ACODEC_FDK_HAAC:
            mixdown = HB_AMIXDOWN_7POINT1;
            break;

        // the (E-)AC-3 and TrueHD encoder defaults to the best mixdown up to 5.1
        case HB_ACODEC_AC3:
        case HB_ACODEC_FFEAC3:
        case HB_ACODEC_FFTRUEHD:
            mixdown = HB_AMIXDOWN_5POINT1;
            break;

        // other encoders default to the best mixdown up to DPLII
        default:
            mixdown = HB_AMIXDOWN_DOLBYPLII;
            break;
    }

    // return the best available mixdown up to the selected default
    return hb_mixdown_get_best(codec, layout, mixdown);
}

hb_mixdown_t* hb_mixdown_get_from_mixdown(int mixdown)
{
    int i;
    for (i = 0; i < hb_audio_mixdowns_count; i++)
    {
        if (hb_audio_mixdowns[i].item.amixdown == mixdown)
        {
            return &hb_audio_mixdowns[i].item;
        }
    }

    return NULL;
}

int hb_mixdown_get_from_name(const char *name)
{
    if (name == NULL || *name == '\0')
        goto fail;

    int i;
    for (i = 0; i < hb_audio_mixdowns_count; i++)
    {
        if (!strcasecmp(hb_audio_mixdowns[i].item.name,       name) ||
            !strcasecmp(hb_audio_mixdowns[i].item.short_name, name))
        {
            return hb_audio_mixdowns[i].item.amixdown;
        }
    }

fail:
    return HB_INVALID_AMIXDOWN;
}

const char* hb_mixdown_get_name(int mixdown)
{
    if (mixdown < hb_audio_mixdowns_first_item->amixdown ||
        mixdown > hb_audio_mixdowns_last_item ->amixdown)
        goto fail;

    const hb_mixdown_t *audio_mixdown = NULL;
    while ((audio_mixdown = hb_mixdown_get_next(audio_mixdown)) != NULL)
    {
        if (audio_mixdown->amixdown == mixdown)
        {
            return audio_mixdown->name;
        }
    }

fail:
    return NULL;
}

const char* hb_mixdown_get_short_name(int mixdown)
{
    if (mixdown < hb_audio_mixdowns_first_item->amixdown ||
        mixdown > hb_audio_mixdowns_last_item ->amixdown)
        goto fail;

    const hb_mixdown_t *audio_mixdown = NULL;
    while ((audio_mixdown = hb_mixdown_get_next(audio_mixdown)) != NULL)
    {
        if (audio_mixdown->amixdown == mixdown)
        {
            return audio_mixdown->short_name;
        }
    }

fail:
    return NULL;
}

const char* hb_mixdown_sanitize_name(const char *name)
{
    return hb_mixdown_get_name(hb_mixdown_get_from_name(name));
}

const hb_mixdown_t* hb_mixdown_get_next(const hb_mixdown_t *last)
{
    if (last == NULL)
    {
        return hb_audio_mixdowns_first_item;
    }
    return ((hb_mixdown_internal_t*)last)->next;
}

void hb_layout_get_name(char *name, int size, int64_t layout)
{
    AVChannelLayout ch_layout = {0};
    av_channel_layout_from_mask(&ch_layout, layout);
    av_channel_layout_describe(&ch_layout, name, size);
    av_channel_layout_uninit(&ch_layout);
}

int hb_layout_get_discrete_channel_count(int64_t layout)
{
    int nb_channels = 0;
    AVChannelLayout ch_layout = {0};
    av_channel_layout_from_mask(&ch_layout, layout);
    nb_channels = ch_layout.nb_channels;
    av_channel_layout_uninit(&ch_layout);
    return nb_channels;
}

int hb_layout_get_low_freq_channel_count(int64_t layout)
{
    return !!(layout & AV_CH_LOW_FREQUENCY) +
           !!(layout & AV_CH_LOW_FREQUENCY_2);
}

int hb_video_encoder_get_default(int muxer)
{
    if (!(muxer & HB_MUX_MASK))
        goto fail;

    const hb_encoder_t *video_encoder = NULL;
    while ((video_encoder = hb_video_encoder_get_next(video_encoder)) != NULL)
    {
        if (video_encoder->muxers & muxer)
        {
            return video_encoder->codec;
        }
    }

fail:
    return HB_VCODEC_INVALID;
}

hb_encoder_t * hb_video_encoder_get_from_codec(int codec)
{
    int i;
    for (i = 0; i < hb_video_encoders_count; i++)
    {
        if (hb_video_encoders[i].item.codec == codec)
        {
            return &hb_video_encoders[i].item;
        }
    }

    return NULL;
}

int hb_video_encoder_get_from_name(const char *name)
{
    if (name == NULL || *name == '\0')
        goto fail;

    int i;
    for (i = 0; i < hb_video_encoders_count; i++)
    {
        if (!strcasecmp(hb_video_encoders[i].item.name,       name) ||
            !strcasecmp(hb_video_encoders[i].item.short_name, name))
        {
            return hb_video_encoders[i].item.codec;
        }
    }

fail:
    return HB_VCODEC_INVALID;
}

const char* hb_video_encoder_get_name(int encoder)
{
    int i;
    for (i = 0; i < hb_video_encoders_count; i++)
    {
        if (hb_video_encoders[i].item.codec == encoder && hb_video_encoders[i].deprecated == 0)
        {
            return hb_video_encoders[i].item.name;
        }
    }

    return NULL;
}

const char* hb_video_encoder_get_short_name(int encoder)
{
    int i;
    for (i = 0; i < hb_video_encoders_count; i++)
    {
        if (hb_video_encoders[i].item.codec == encoder && hb_video_encoders[i].deprecated == 0)
        {
            return hb_video_encoders[i].item.short_name;
        }
    }

    return NULL;
}

const char* hb_video_encoder_get_long_name(int encoder)
{
    int i;
    for (i = 0; i < hb_video_encoders_count; i++)
    {
        if (hb_video_encoders[i].item.codec == encoder && hb_video_encoders[i].deprecated == 0)
        {
            return hb_video_encoders[i].item.long_name;
        }
    }

    return NULL;
}

const char* hb_video_encoder_sanitize_name(const char *name)
{
    return hb_video_encoder_get_name(hb_video_encoder_get_from_name(name));
}

const hb_encoder_t* hb_video_encoder_get_next(const hb_encoder_t *last)
{
    if (last == NULL)
    {
        return hb_video_encoders_first_item;
    }
    return ((hb_encoder_internal_t*)last)->next;
}

// for a valid passthru, return the matching encoder for that codec (if any),
// else return -1 (i.e. drop the track)
int hb_audio_encoder_get_fallback_for_passthru(int passthru)
{
    int gid;
    const hb_encoder_t *audio_encoder = NULL;
    switch (passthru)
    {
        case HB_ACODEC_ALAC_PASS:
            gid = HB_GID_ACODEC_ALAC;
            break;

        case HB_ACODEC_AAC_PASS:
            gid = HB_GID_ACODEC_AAC;
            break;

        case HB_ACODEC_AC3_PASS:
            gid = HB_GID_ACODEC_AC3;
            break;

        case HB_ACODEC_EAC3_PASS:
            gid = HB_GID_ACODEC_EAC3;
            break;

        case HB_ACODEC_FLAC_PASS:
            gid = HB_GID_ACODEC_FLAC;
            break;

        case HB_ACODEC_MP3_PASS:
            gid = HB_GID_ACODEC_MP3;
            break;

        default:
            return HB_ACODEC_INVALID;
            break;
    }
    while ((audio_encoder = hb_audio_encoder_get_next(audio_encoder)) != NULL)
    {
        if (((hb_encoder_internal_t*)audio_encoder)->gid == gid)
        {
            return audio_encoder->codec;
        }
    }

    // passthru tracks are often the second audio from the same source track
    // if we don't have an encoder matching the passthru codec, return INVALID
    // dropping the track, as well as ensuring that there is at least one
    // audio track in the output is then up to the UIs
    return HB_ACODEC_INVALID;
}

int hb_audio_encoder_get_default(int muxer)
{
    if (!(muxer & HB_MUX_MASK))
        goto fail;

    int codec                         = 0;
    const hb_encoder_t *audio_encoder = NULL;
    while ((audio_encoder = hb_audio_encoder_get_next(audio_encoder)) != NULL)
    {
        // default encoder should not be passthru
        if ((audio_encoder->muxers & muxer) &&
            (audio_encoder->codec  & HB_ACODEC_PASS_FLAG) == 0)
        {
            codec = audio_encoder->codec;
            break;
        }
    }

    // Lame is better than our low-end AAC encoders
    // if the container is MKV, use the former
    // AAC is still used when the container is MP4 (for better compatibility)
    if (codec == HB_ACODEC_FFAAC && (muxer & HB_MUX_MASK_MKV) == muxer)
    {
        return HB_ACODEC_LAME;
    }
    else
    {
        return codec;
    }

fail:
    return HB_ACODEC_INVALID;
}

hb_encoder_t* hb_audio_encoder_get_from_codec(int codec)
{
    int i;
    for (i = 0; i < hb_audio_encoders_count; i++)
    {
        if (hb_audio_encoders[i].item.codec == codec)
        {
            return &hb_audio_encoders[i].item;
        }
    }

    return NULL;
}

int hb_audio_encoder_get_from_name(const char *name)
{
    if (name == NULL || *name == '\0')
        goto fail;

    int i;
    for (i = 0; i < hb_audio_encoders_count; i++)
    {
        if (!strcasecmp(hb_audio_encoders[i].item.name,       name) ||
            !strcasecmp(hb_audio_encoders[i].item.short_name, name))
        {
            return hb_audio_encoders[i].item.codec;
        }
    }

fail:
    return HB_ACODEC_INVALID;
}

const char* hb_audio_encoder_get_name(int encoder)
{
    if (!(encoder & HB_ACODEC_ANY))
        goto fail;

    const hb_encoder_t *audio_encoder = NULL;
    while ((audio_encoder = hb_audio_encoder_get_next(audio_encoder)) != NULL)
    {
        if (audio_encoder->codec == encoder)
        {
            return audio_encoder->name;
        }
    }

fail:
    return NULL;
}

const char* hb_audio_encoder_get_short_name(int encoder)
{
    if (!(encoder & HB_ACODEC_ANY))
        goto fail;

    const hb_encoder_t *audio_encoder = NULL;
    while ((audio_encoder = hb_audio_encoder_get_next(audio_encoder)) != NULL)
    {
        if (audio_encoder->codec == encoder)
        {
            return audio_encoder->short_name;
        }
    }

fail:
    return NULL;
}

const char* hb_audio_encoder_get_long_name(int encoder)
{
    if (!(encoder & HB_ACODEC_ANY))
        goto fail;

    const hb_encoder_t *audio_encoder = NULL;
    while ((audio_encoder = hb_audio_encoder_get_next(audio_encoder)) != NULL)
    {
        if (audio_encoder->codec == encoder)
        {
            return audio_encoder->long_name;
        }
    }

fail:
    return NULL;
}

const char* hb_audio_encoder_sanitize_name(const char *name)
{
    return hb_audio_encoder_get_name(hb_audio_encoder_get_from_name(name));
}

const hb_encoder_t* hb_audio_encoder_get_next(const hb_encoder_t *last)
{
    if (last == NULL)
    {
        return  hb_audio_encoders_first_item;
    }
    return ((hb_encoder_internal_t*)last)->next;
}

void hb_autopassthru_apply_settings(hb_job_t *job)
{
    hb_audio_t *audio;
    int i, already_printed;
    for (i = already_printed = 0; i < hb_list_count(job->list_audio);)
    {
        audio = hb_list_item(job->list_audio, i);
        if (audio->config.out.codec == HB_ACODEC_AUTO_PASS)
        {
            if (!already_printed)
                hb_autopassthru_print_settings(job);
            already_printed = 1;
            audio->config.out.codec = hb_autopassthru_get_encoder(audio->config.in.codec,
                                                                  job->acodec_copy_mask,
                                                                  job->acodec_fallback,
                                                                  job->mux);
            if (audio->config.out.codec == HB_ACODEC_NONE ||
                audio->config.out.codec == HB_ACODEC_INVALID)
            {
                hb_log("Auto Passthru: passthru not possible and no valid fallback specified, dropping track %d",
                       audio->config.out.track );
                hb_list_rem(job->list_audio, audio);
                hb_audio_close(&audio);
                continue;
            }
            if (!(audio->config.out.codec & HB_ACODEC_PASS_FLAG))
            {
                hb_log("Auto Passthru: passthru not possible for track %d, using fallback",
                       audio->config.out.track);
                if (audio->config.out.mixdown <= 0)
                {
                    audio->config.out.mixdown =
                        hb_mixdown_get_default(audio->config.out.codec,
                                               audio->config.in.channel_layout);
                }
                else
                {
                    audio->config.out.mixdown =
                        hb_mixdown_get_best(audio->config.out.codec,
                                            audio->config.in.channel_layout,
                                            audio->config.out.mixdown);
                }
                if (audio->config.out.samplerate <= 0)
                    audio->config.out.samplerate = audio->config.in.samplerate;
                audio->config.out.samplerate =
                    hb_audio_samplerate_find_closest(
                        audio->config.out.samplerate, audio->config.out.codec);
                int quality_not_allowed =
                    hb_audio_quality_get_default(audio->config.out.codec)
                            == HB_INVALID_AUDIO_QUALITY;
                if (audio->config.out.bitrate > 0)
                {
                    // Use best bitrate
                    audio->config.out.bitrate =
                        hb_audio_bitrate_get_best(audio->config.out.codec,
                                                  audio->config.out.bitrate,
                                                  audio->config.out.samplerate,
                                                  audio->config.out.mixdown);
                }
                else if (quality_not_allowed ||
                         audio->config.out.quality != HB_INVALID_AUDIO_QUALITY)
                {
                    // Use default bitrate
                    audio->config.out.bitrate =
                        hb_audio_bitrate_get_default(audio->config.out.codec,
                                                 audio->config.out.samplerate,
                                                 audio->config.out.mixdown);
                }
                else
                {
                    // Use best quality
                    audio->config.out.quality =
                        hb_audio_quality_get_best(audio->config.out.codec,
                                                  audio->config.out.quality);
                }
                if (audio->config.out.compression_level < 0)
                {
                    audio->config.out.compression_level =
                        hb_audio_compression_get_default(
                                        audio->config.out.codec);
                }
                else
                {
                    audio->config.out.compression_level =
                        hb_audio_compression_get_best(audio->config.out.codec,
                                        audio->config.out.compression_level);
                }
            }
            else
            {
                const hb_encoder_t *audio_encoder = NULL;
                while ((audio_encoder = hb_audio_encoder_get_next(audio_encoder)) != NULL)
                {
                    if (audio_encoder->codec == audio->config.out.codec)
                    {
                        hb_log("Auto Passthru: using %s for track %d",
                               audio_encoder->name,
                               audio->config.out.track);
                        break;
                    }
                }
            }
        }
        /* Adjust output track number, in case we removed one.
         * Output tracks sadly still need to be in sequential order.
         * Note: out.track starts at 1, i starts at 0 */
        audio->config.out.track = ++i;
    }
}

void hb_autopassthru_print_settings(hb_job_t *job)
{
    char *mask = NULL, *tmp;
    const char *fallback = NULL;
    const hb_encoder_t *audio_encoder = NULL;
    while ((audio_encoder = hb_audio_encoder_get_next(audio_encoder)) != NULL)
    {
        if ((audio_encoder->codec &  HB_ACODEC_PASS_FLAG) &&
            (audio_encoder->codec != HB_ACODEC_AUTO_PASS) &&
            (audio_encoder->codec & (job->acodec_copy_mask &
                                     HB_ACODEC_PASS_MASK)))
        {
            if (mask != NULL)
            {
                tmp = hb_strncat_dup(mask, ", ", 2);
                if (tmp != NULL)
                {
                    free(mask);
                    mask = tmp;
                }
            }
            // passthru name without " Passthru"
            tmp = hb_strncat_dup(mask,  audio_encoder->name,
                                 strlen(audio_encoder->name) - 9);
            if (tmp != NULL)
            {
                free(mask);
                mask = tmp;
            }
        }
        else if ((audio_encoder->codec & HB_ACODEC_PASS_FLAG) == 0 &&
                 (audio_encoder->codec == job->acodec_fallback))
        {
            fallback = audio_encoder->name;
        }
    }
    if (mask == NULL)
        hb_log("Auto Passthru: no codecs allowed");
    else
        hb_log("Auto Passthru: allowed codecs are %s", mask);
    if (fallback == NULL)
        hb_log("Auto Passthru: no valid fallback specified");
    else
        hb_log("Auto Passthru: fallback is %s", fallback);
}

int hb_autopassthru_get_encoder(int in_codec, int copy_mask, int fallback,
                                int muxer)
{
    int out_codec_result_set = 0;
    int fallback_result_set  = 0;
    int out_codec_result = HB_ACODEC_INVALID;
    int fallback_result  = HB_ACODEC_INVALID;
    const hb_encoder_t *audio_encoder = NULL;
    int out_codec = (copy_mask & in_codec) | HB_ACODEC_PASS_FLAG;

    // sanitize fallback encoder and selected passthru
    // note: invalid fallbacks are caught in hb_autopassthru_apply_settings
    while ((audio_encoder = hb_audio_encoder_get_next(audio_encoder)) != NULL)
    {
        if (!out_codec_result_set && audio_encoder->codec == out_codec)
        {
            out_codec_result_set = 1;
            if (audio_encoder->muxers & muxer)
                out_codec_result = out_codec;
        }
        else if (!fallback_result_set && audio_encoder->codec == fallback)
        {
            fallback_result_set  = 1;
            if ((audio_encoder->muxers & muxer) || fallback == HB_ACODEC_NONE)
                fallback_result = fallback;
        }
        if (out_codec_result_set && fallback_result_set)
        {
            break;
        }
    }
    return (out_codec_result != HB_ACODEC_INVALID) ? out_codec_result :
                                                     fallback_result;
}

const char* hb_audio_decoder_get_name(int codec, int codec_param)
{
    if (codec & HB_ACODEC_FF_MASK)
    {
        const AVCodec *codec;

        codec = avcodec_find_decoder(codec_param);
        if (codec != NULL)
        {
            return codec->name;
        }
    }
    else
    {
        switch (codec)
        {
            case HB_ACODEC_LPCM:
                return "pcm_dvd";
            default:
                break;
        }
    }
    return "unknown";
}

hb_container_t* hb_container_get_from_format(int format)
{
    int i;
    for (i = 0; i < hb_containers_count; i++)
    {
        if (hb_containers[i].item.format == format)
        {
            return &hb_containers[i].item;
        }
    }

    return NULL;
}

int hb_container_get_from_name(const char *name)
{
    if (name == NULL || *name == '\0')
        goto fail;

    int i;
    for (i = 0; i < hb_containers_count; i++)
    {
        if (!strcasecmp(hb_containers[i].item.name,       name) ||
            !strcasecmp(hb_containers[i].item.short_name, name))
        {
            return hb_containers[i].item.format;
        }
    }

fail:
    return HB_MUX_INVALID;
}

int hb_container_get_from_extension(const char *extension)
{
    if (extension == NULL || *extension == '\0')
        goto fail;

    int i;
    for (i = 0; i < hb_containers_count; i++)
    {
        if (!strcasecmp(hb_containers[i].item.default_extension, extension))
        {
            return hb_containers[i].item.format;
        }
    }

fail:
    return HB_MUX_INVALID;
}

const char* hb_container_get_name(int format)
{
    if (!(format & HB_MUX_MASK))
        goto fail;

    const hb_container_t *container = NULL;
    while ((container = hb_container_get_next(container)) != NULL)
    {
        if (container->format == format)
        {
            return container->name;
        }
    }

fail:
    return NULL;
}

const char* hb_container_get_short_name(int format)
{
    if (!(format & HB_MUX_MASK))
        goto fail;

    const hb_container_t *container = NULL;
    while ((container = hb_container_get_next(container)) != NULL)
    {
        if (container->format == format)
        {
            return container->short_name;
        }
    }

fail:
    return NULL;
}

const char* hb_container_get_long_name(int format)
{
    if (!(format & HB_MUX_MASK))
        goto fail;

    const hb_container_t *container = NULL;
    while ((container = hb_container_get_next(container)) != NULL)
    {
        if (container->format == format)
        {
            return container->long_name;
        }
    }

fail:
    return NULL;
}

const char* hb_container_get_default_extension(int format)
{
    if (!(format & HB_MUX_MASK))
        goto fail;

    const hb_container_t *container = NULL;
    while ((container = hb_container_get_next(container)) != NULL)
    {
        if (container->format == format)
        {
            return container->default_extension;
        }
    }

fail:
    return NULL;
}

const char* hb_container_sanitize_name(const char *name)
{
    return hb_container_get_name(hb_container_get_from_name(name));
}

const hb_container_t* hb_container_get_next(const hb_container_t *last)
{
    if (last == NULL)
    {
        return  hb_containers_first_item;
    }
    return ((hb_container_internal_t*)last)->next;
}

/**********************************************************************
 * hb_reduce
 **********************************************************************
 * Given a numerator (num) and a denominator (den), reduce them to an
 * equivalent fraction and store the result in x and y.
 *********************************************************************/
void hb_reduce( int *x, int *y, int num, int den )
{
    // find the greatest common divisor of num & den by Euclid's algorithm
    int n = num, d = den;
    while ( d )
    {
        int t = d;
        d = n % d;
        n = t;
    }

    // at this point n is the gcd. if it's non-zero remove it from num
    // and den. Otherwise just return the original values.
    if ( n )
    {
        *x = num / n;
        *y = den / n;
    }
    else
    {
        *x = num;
        *y = den;
    }
}

void hb_limit_rational( int *x, int *y, int64_t num, int64_t den, int limit )
{
    hb_reduce64( &num, &den, num, den );
    if ( num < limit && den < limit )
    {
        *x = num;
        *y = den;
        return;
    }

    if ( num > den )
    {
        double div = (double)limit / num;
        num = limit;
        den *= div;
    }
    else
    {
        double div = (double)limit / den;
        den = limit;
        num *= div;
    }
    *x = num;
    *y = den;
}

int64_t hb_rescale_rational(hb_rational_t q, int b)
{
    return av_rescale(q.num, b, q.den);
}

/**********************************************************************
 * hb_reduce64
 **********************************************************************
 * Given a numerator (num) and a denominator (den), reduce them to an
 * equivalent fraction and store the result in x and y.
 *********************************************************************/
void hb_reduce64( int64_t *x, int64_t *y, int64_t num, int64_t den )
{
    // find the greatest common divisor of num & den by Euclid's algorithm
    int64_t n = num, d = den;
    while ( d )
    {
        int64_t t = d;
        d = n % d;
        n = t;
    }

    // at this point n is the gcd. if it's non-zero remove it from num
    // and den. Otherwise just return the original values.
    if ( n )
    {
        num /= n;
        den /= n;
    }

    *x = num;
    *y = den;

}

void hb_limit_rational64( int64_t *x, int64_t *y, int64_t num, int64_t den, int64_t limit )
{
    hb_reduce64( &num, &den, num, den );
    if ( num < limit && den < limit )
    {
        *x = num;
        *y = den;
        return;
    }

    if ( num > den )
    {
        double div = (double)limit / num;
        num = limit;
        den *= div;
    }
    else
    {
        double div = (double)limit / den;
        den = limit;
        num *= div;
    }
    *x = num;
    *y = den;
}

/**********************************************************************
 * hb_buffer_list implementation
 *********************************************************************/
void hb_buffer_list_append(hb_buffer_list_t *list, hb_buffer_t *buf)
{
    int count = 1;
    int size = 0;
    hb_buffer_t *end = buf;

    if (buf == NULL)
    {
        return;
    }

    // Input buffer may be a list of buffers, find the end.
    size += buf->size;
    while (end != NULL && end->next != NULL)
    {
        end = end->next;
        size += end->size;
        count++;
    }
    if (list->tail == NULL)
    {
        list->head = buf;
        list->tail = end;
    }
    else
    {
        list->tail->next = buf;
        list->tail = end;
    }
    list->count += count;
    list->size += size;
}

void hb_buffer_list_prepend(hb_buffer_list_t *list, hb_buffer_t *buf)
{
    int count = 1;
    int size = 0;
    hb_buffer_t *end = buf;

    if (buf == NULL)
    {
        return;
    }

    // Input buffer may be a list of buffers, find the end.
    size += buf->size;
    while (end != NULL && end->next != NULL)
    {
        end = end->next;
        size += end->size;
        count++;
    }
    if (list->tail == NULL)
    {
        list->head = buf;
        list->tail = end;
    }
    else
    {
        end->next = list->head;
        list->head = buf;
    }
    list->count += count;
    list->size += size;
}

hb_buffer_t* hb_buffer_list_rem_head(hb_buffer_list_t *list)
{
    if (list == NULL)
    {
        return NULL;
    }
    hb_buffer_t *head = list->head;
    if (list->head != NULL)
    {
        if (list->head == list->tail)
        {
            list->tail = NULL;
        }
        list->head = list->head->next;
        list->count--;
        list->size -= head->size;
    }
    if (head != NULL)
    {
        head->next = NULL;
    }
    return head;
}

hb_buffer_t* hb_buffer_list_rem_tail(hb_buffer_list_t *list)
{
    if (list == NULL)
    {
        return NULL;
    }
    hb_buffer_t *tail = list->tail;

    if (list->head == list->tail)
    {
        list->head = list->tail = NULL;
        list->count = 0;
        list->size = 0;
    }
    else if (list->tail != NULL)
    {
        hb_buffer_t *end = list->head;
        while (end->next != list->tail)
        {
            end = end->next;
        }
        end->next = NULL;
        list->tail = end;
        list->count--;
        list->size -= tail->size;
    }
    if (tail != NULL)
    {
        tail->next = NULL;
    }
    return tail;
}

hb_buffer_t* hb_buffer_list_rem(hb_buffer_list_t *list, hb_buffer_t * b)
{
    hb_buffer_t * a;

    if (list == NULL)
    {
        return NULL;
    }
    if (b == list->head)
    {
        return hb_buffer_list_rem_head(list);
    }
    a = list->head;
    while (a != NULL && a->next != b)
    {
        a = a->next;
    }
    if (a == NULL)
    {
        // Buffer is not in the list
        return NULL;
    }
    list->count--;
    list->size -= b->size;
    a->next = b->next;
    if (list->tail == b)
    {
        list->tail = a;
    }
    b->next = NULL;

    return b;
}

hb_buffer_t* hb_buffer_list_head(hb_buffer_list_t *list)
{
    if (list == NULL)
    {
        return NULL;
    }
    return list->head;
}

hb_buffer_t* hb_buffer_list_tail(hb_buffer_list_t *list)
{
    if (list == NULL)
    {
        return NULL;
    }
    return list->tail;
}

hb_buffer_t* hb_buffer_list_set(hb_buffer_list_t *list, hb_buffer_t *buf)
{
    int count = 0;
    int size = 0;

    if (list == NULL)
    {
        return NULL;
    }

    hb_buffer_t *head = list->head;
    hb_buffer_t *end = buf;
    if (end != NULL)
    {
        count++;
        size += end->size;
        while (end->next != NULL)
        {
            end = end->next;
            count++;
            size += end->size;
        }
    }
    list->head = buf;
    list->tail = end;
    list->count = count;
    list->size = size;
    return head;
}

hb_buffer_t* hb_buffer_list_clear(hb_buffer_list_t *list)
{
    if (list == NULL)
    {
        return NULL;
    }
    hb_buffer_t *head = list->head;
    list->head = list->tail = NULL;
    list->count = 0;
    list->size = 0;
    return head;
}

void hb_buffer_list_close(hb_buffer_list_t *list)
{
    hb_buffer_t *buf = hb_buffer_list_clear(list);
    hb_buffer_close(&buf);
}

int hb_buffer_list_count(hb_buffer_list_t *list)
{
    if (list == NULL) return 0;
    return list->count;
}

int hb_buffer_list_size(hb_buffer_list_t *list)
{
    return list->size;
}

/**********************************************************************
 * hb_list implementation
 **********************************************************************
 * Basic and slow, but enough for what we need
 *********************************************************************/

#define HB_LIST_DEFAULT_SIZE 20

struct hb_list_s
{
    /* Pointers to items in the list */
    void ** items;

    /* How many (void *) allocated in 'items' */
    int     items_alloc;

    /* How many valid pointers in 'items' */
    int     items_count;
};

/**********************************************************************
 * hb_list_init
 **********************************************************************
 * Allocates an empty list ready for HB_LIST_DEFAULT_SIZE items
 *********************************************************************/
hb_list_t * hb_list_init()
{
    hb_list_t * l;

    l              = calloc( sizeof( hb_list_t ), 1 );
    l->items       = calloc( HB_LIST_DEFAULT_SIZE * sizeof( void * ), 1 );
    l->items_alloc = HB_LIST_DEFAULT_SIZE;

    return l;
}

/**********************************************************************
 * hb_list_count
 **********************************************************************
 * Returns the number of items currently in the list
 *********************************************************************/
int hb_list_count( const hb_list_t * l )
{
    if (l == NULL) return 0;
    return l->items_count;
}

/**********************************************************************
 * hb_list_add
 **********************************************************************
 * Adds an item at the end of the list, making it bigger if necessary.
 * Can safely be called with a NULL pointer to add, it will be ignored.
 *********************************************************************/
void hb_list_add( hb_list_t * l, void * p )
{
    if( !p )
    {
        return;
    }

    if( l->items_count == l->items_alloc )
    {
        /* We need a bigger boat */
        l->items_alloc += HB_LIST_DEFAULT_SIZE;
        l->items        = realloc( l->items,
                                   l->items_alloc * sizeof( void * ) );
    }

    l->items[l->items_count] = p;
    (l->items_count)++;
}

void hb_list_add_dup( hb_list_t * l, void * p, int size )
{
    void * item = malloc(size);

    memcpy(item, p, size);
    hb_list_add(l, item);
}

/**********************************************************************
 * hb_list_insert
 **********************************************************************
 * Adds an item at the specified position in the list, making it bigger
 * if necessary.
 * Can safely be called with a NULL pointer to add, it will be ignored.
 *********************************************************************/
void hb_list_insert( hb_list_t * l, int pos, void * p )
{
    if( !p )
    {
        return;
    }

    if( l->items_count == l->items_alloc )
    {
        /* We need a bigger boat */
        l->items_alloc += HB_LIST_DEFAULT_SIZE;
        l->items        = realloc( l->items,
                                   l->items_alloc * sizeof( void * ) );
    }

    if ( l->items_count != pos )
    {
        /* Shift all items after it sizeof( void * ) bytes earlier */
        memmove( &l->items[pos+1], &l->items[pos],
                 ( l->items_count - pos ) * sizeof( void * ) );
    }


    l->items[pos] = p;
    (l->items_count)++;
}

/**********************************************************************
 * hb_list_rem
 **********************************************************************
 * Remove an item from the list. Bad things will happen if called
 * with a NULL pointer or if the item is not in the list.
 *********************************************************************/
void hb_list_rem( hb_list_t * l, void * p )
{
    int i;

    /* Find the item in the list */
    for( i = 0; i < l->items_count; i++ )
    {
        if( l->items[i] == p )
        {
            /* Shift all items after it sizeof( void * ) bytes earlier */
            memmove( &l->items[i], &l->items[i+1],
                     ( l->items_count - i - 1 ) * sizeof( void * ) );

            (l->items_count)--;
            break;
        }
    }
}

/**********************************************************************
 * hb_list_item
 **********************************************************************
 * Returns item at position i, or NULL if there are not that many
 * items in the list
 *********************************************************************/
void * hb_list_item( const hb_list_t * l, int i )
{
    if( l == NULL || i < 0 || i >= l->items_count )
    {
        return NULL;
    }

    return l->items[i];
}

/**********************************************************************
 * hb_list_bytes
 **********************************************************************
 * Assuming all items are of type hb_buffer_t, returns the total
 * number of bytes in the list
 *********************************************************************/
int hb_list_bytes( hb_list_t * l )
{
    hb_buffer_t * buf;
    int           ret;
    int           i;

    ret = 0;
    for( i = 0; i < hb_list_count( l ); i++ )
    {
        buf  = hb_list_item( l, i );
        ret += buf->size - buf->offset;
    }

    return ret;
}

/**********************************************************************
 * hb_list_seebytes
 **********************************************************************
 * Assuming all items are of type hb_buffer_t, copy <size> bytes from
 * the list to <dst>, keeping the list unmodified.
 *********************************************************************/
void hb_list_seebytes( hb_list_t * l, uint8_t * dst, int size )
{
    hb_buffer_t * buf;
    int           copied;
    int           copying;
    int           i;

    for( i = 0, copied = 0; copied < size; i++ )
    {
        buf     = hb_list_item( l, i );
        copying = MIN( buf->size - buf->offset, size - copied );
        memcpy( &dst[copied], &buf->data[buf->offset], copying );
        copied += copying;
    }
}

/**********************************************************************
 * hb_list_getbytes
 **********************************************************************
 * Assuming all items are of type hb_buffer_t, copy <size> bytes from
 * the list to <dst>. What's copied is removed from the list.
 * The variable pointed by <pts> is set to the PTS of the buffer the
 * first byte has been got from.
 * The variable pointed by <pos> is set to the position of that byte
 * in that buffer.
 *********************************************************************/
void hb_list_getbytes( hb_list_t * l, uint8_t * dst, int size,
                       uint64_t * pts, uint64_t * pos )
{
    hb_buffer_t * buf;
    int           copied;
    int           copying;
    uint8_t       has_pts;

    /* So we won't have to deal with NULL pointers */
     uint64_t dummy1, dummy2;

    if( !pts ) pts = &dummy1;
    if( !pos ) pos = &dummy2;

    for( copied = 0, has_pts = 0; copied < size;  )
    {
        buf     = hb_list_item( l, 0 );
        copying = MIN( buf->size - buf->offset, size - copied );
        memcpy( &dst[copied], &buf->data[buf->offset], copying );

        if( !has_pts )
        {
            *pts    = buf->s.start;
            *pos    = buf->offset;
            has_pts = 1;
        }

        buf->offset += copying;
        if( buf->offset >= buf->size )
        {
            hb_list_rem( l, buf );
            hb_buffer_close( &buf );
        }

        copied += copying;
    }
}

/**********************************************************************
 * hb_list_empty
 **********************************************************************
 * Assuming all items are of type hb_buffer_t, close them all and
 * close the list.
 *********************************************************************/
void hb_list_empty( hb_list_t ** _l )
{
    hb_list_t * l = *_l;
    hb_buffer_t * b;

    while( ( b = hb_list_item( l, 0 ) ) )
    {
        hb_list_rem( l, b );
        hb_buffer_close( &b );
    }

    hb_list_close( _l );
}

/**********************************************************************
 * hb_list_close
 **********************************************************************
 * Free memory allocated by hb_list_init. Does NOT free contents of
 * items still in the list.
 *********************************************************************/
void hb_list_close( hb_list_t ** _l )
{
    hb_list_t * l = *_l;

    if (l == NULL)
    {
        return;
    }

    free( l->items );
    free( l );

    *_l = NULL;
}

/**********************************************************************
 * hb_string_list_copy
 **********************************************************************
 * Make a copy of a string list.
 *********************************************************************/
hb_list_t *hb_string_list_copy(const hb_list_t *src)
{
    hb_list_t *list = hb_list_init();
    char *string = NULL;

    if (src)
    {
        for (int i = 0; i < hb_list_count(src); i++)
        {
            if ((string = hb_list_item(src, i)))
            {
                hb_list_add(list, strdup(string));
            }
        }
    }
    return list;
}

hb_data_t * hb_data_init(size_t size)
{
    hb_data_t *data = av_mallocz(sizeof(hb_data_t));
    if (data == NULL)
    {
        goto fail;
    }

    data->bytes = av_mallocz(size);
    if (data->bytes == NULL)
    {
        goto fail;
    }

    data->size = size;
    return data;

fail:
    hb_data_close(&data);
    return NULL;
}

void hb_data_close(hb_data_t **data)
{
    if (data == NULL || *data == NULL)
    {
        return;
    }

    if ((*data)->bytes)
    {
        av_freep(&(*data)->bytes);
    }

    av_free(*data);
    *data = NULL;
}

hb_data_t * hb_data_dup(const hb_data_t *src)
{
    if (src == NULL)
    {
        return NULL;
    }

    hb_data_t *data = hb_data_init(src->size);
    if (data)
    {
        memcpy(data->bytes, src->bytes, src->size);
    }
    return data;
}

int global_verbosity_level; //Necessary for hb_deep_log
/**********************************************************************
 * hb_valog
 **********************************************************************
 * If verbose mode is one, print message with timestamp. Messages
 * longer than 180 characters are stripped ;p
 *********************************************************************/
void hb_valog( hb_debug_level_t level, const char * prefix, const char * log, va_list args)
{
    char      * string;
    time_t      _now;
    struct tm * now;
    char        preamble[362];

    if( global_verbosity_level < level )
    {
        /* Hiding message */
        return;
    }

    /* Get the time */
    _now = time( NULL );
    now  = localtime( &_now );
    if ( prefix && *prefix )
    {
        // limit the prefix length
        snprintf( preamble, 361, "[%02d:%02d:%02d] %s %s\n",
                 now->tm_hour, now->tm_min, now->tm_sec, prefix, log );
    }
    else
    {
        snprintf( preamble, 361, "[%02d:%02d:%02d] %s\n",
                  now->tm_hour, now->tm_min, now->tm_sec, log );
    }

    string = hb_strdup_vaprintf(preamble, args);

#ifdef SYS_MINGW
    wchar_t     *wstring; /* 360 chars + \n + \0 */
    int          len;

    len = strlen(string) + 1;
    wstring = malloc(2 * len);

    // Convert internal utf8 to "console output code page".
    //
    // This is just bizarre windows behavior.  You would expect that
    // printf would automatically convert a wide character string to
    // the current "console output code page" when using the "%ls" format
    // specifier.  But it doesn't... so we must do it.
    if (!MultiByteToWideChar(CP_UTF8, 0, string, -1, wstring, len))
    {
        free(string);
        free(wstring);
        return;
    }
    free(string);
    string = malloc(2 * len);
    if (!WideCharToMultiByte(GetConsoleOutputCP(), 0, wstring, -1, string, len,
                             NULL, NULL))
    {
        free(string);
        free(wstring);
        return;
    }
    free(wstring);
#endif

    /* Print it */
    fprintf( stderr, "%s", string );
    free(string);
}

/**********************************************************************
 * hb_log
 **********************************************************************
 * If verbose mode is one, print message with timestamp. Messages
 * longer than 180 characters are stripped ;p
 *********************************************************************/
void hb_log( char * log, ... )
{
    va_list     args;

    va_start( args, log );
    hb_valog( 0, NULL, log, args );
    va_end( args );
}

/**********************************************************************
 * hb_deep_log
 **********************************************************************
 * If verbose mode is >= level, print message with timestamp. Messages
 * longer than 360 characters are stripped ;p
 *********************************************************************/
void hb_deep_log( hb_debug_level_t level, char * log, ... )
{
    va_list     args;

    va_start( args, log );
    hb_valog( level, NULL, log, args );
    va_end( args );
}

/**********************************************************************
 * hb_error
 **********************************************************************
 * Using whatever output is available display this error.
 *********************************************************************/
void hb_error( char * log, ... )
{
    char        string[181]; /* 180 chars + \0 */
    char        rep_string[181];
    static char last_string[181];
    static int  last_error_count = 0;
    static uint64_t last_series_error_time = 0;
    static hb_lock_t *mutex = 0;
    va_list     args;
    uint64_t time_now;

    /* Convert the message to a string */
    va_start( args, log );
    vsnprintf( string, 180, log, args );
    va_end( args );

    if( !mutex )
    {
        mutex = hb_lock_init();
    }

    hb_lock( mutex );

    time_now = hb_get_date();

    if( strcmp( string, last_string) == 0 )
    {
        /*
         * The last error and this one are the same, don't log it
         * just count it instead, unless it was more than one second
         * ago.
         */
        last_error_count++;
        if( last_series_error_time + ( 1000 * 1 ) > time_now )
        {
            hb_unlock( mutex );
            return;
        }
    }

    /*
     * A new error, or the same one more than 10sec since the last one
     * did we have any of the same counted up?
     */
    if( last_error_count > 0 )
    {
        /*
         * Print out the last error to ensure context for the last
         * repeated message.
         */
        if( error_handler )
        {
            error_handler( last_string );
        } else {
            hb_log( "%s", last_string );
        }

        if( last_error_count > 1 )
        {
            /*
             * Only print out the repeat message for more than 2 of the
             * same, since we just printed out two of them already.
             */
            snprintf( rep_string, 180, "Last error repeated %d times",
                      last_error_count - 1 );

            if( error_handler )
            {
                error_handler( rep_string );
            } else {
                hb_log( "%s", rep_string );
            }
        }

        last_error_count = 0;
    }

    last_series_error_time = time_now;

    strcpy( last_string, string );

    /*
     * Got the error in a single string, send it off to be dispatched.
     */
    if( error_handler )
    {
        error_handler( string );
    } else {
        hb_log( "%s", string );
    }

    hb_unlock( mutex );
}

void hb_register_error_handler( hb_error_handler_t * handler )
{
    error_handler = handler;
}

void hb_update_str( char **dst, const char *src )
{
    if ( dst )
    {
        free( *dst );
        *dst = NULL;
        if ( src )
        {
            *dst = strdup( src );
        }
    }
}

/**********************************************************************
 * hb_title_init
 **********************************************************************
 *
 *********************************************************************/
hb_title_t * hb_title_init( char * path, int index )
{
    hb_title_t * t;

    t = calloc( sizeof( hb_title_t ), 1 );

    t->index              = index;
    t->playlist           = -1;
    t->list_audio         = hb_list_init();
    t->list_chapter       = hb_list_init();
    t->list_subtitle      = hb_list_init();
    t->list_attachment    = hb_list_init();
    t->metadata           = hb_metadata_init();
    t->path               = strdup(path);
    // default to decoding mpeg2
    t->video_id           = 0xE0;
    t->video_codec        = WORK_DECAVCODECV;
    t->video_codec_param  = AV_CODEC_ID_MPEG2VIDEO;
    t->video_timebase.num = 1;
    t->video_timebase.den = 90000;
    t->angle_count        = 1;
    t->geometry.par.num   = 0;
    t->geometry.par.den   = 1;
    t->color_prim         = HB_COLR_PRI_UNSET;
    t->color_transfer     = HB_COLR_TRA_UNSET;
    t->color_matrix       = HB_COLR_MAT_UNSET;

    return t;
}

/**********************************************************************
 * hb_title_close
 **********************************************************************
 *
 *********************************************************************/
void hb_title_close( hb_title_t ** _t )
{
    hb_title_t * t = *_t;
    hb_audio_t * audio;
    hb_chapter_t * chapter;
    hb_subtitle_t * subtitle;
    hb_attachment_t * attachment;

    hb_data_close(&t->initial_rpu);

    while( ( chapter = hb_list_item( t->list_chapter, 0 ) ) )
    {
        hb_list_rem( t->list_chapter, chapter );
        hb_chapter_close( &chapter );
    }
    hb_list_close( &t->list_chapter );

    while( ( audio = hb_list_item( t->list_audio, 0 ) ) )
    {
        hb_list_rem( t->list_audio, audio );
        hb_audio_close( &audio );
    }
    hb_list_close( &t->list_audio );

    while( ( subtitle = hb_list_item( t->list_subtitle, 0 ) ) )
    {
        hb_list_rem( t->list_subtitle, subtitle );
        hb_subtitle_close( &subtitle );
    }
    hb_list_close( &t->list_subtitle );

    while( ( attachment = hb_list_item( t->list_attachment, 0 ) ) )
    {
        hb_list_rem( t->list_attachment, attachment );
        hb_attachment_close( &attachment );
    }
    hb_list_close( &t->list_attachment );

    hb_metadata_close( &t->metadata );

    free((char*)t->name);
    free((char*)t->path);
    free(t->video_codec_name);
    free(t->container_name);

    free( t );
    *_t = NULL;
}

static void job_setup(hb_job_t * job, hb_title_t * title)
{
    if ( job == NULL || title == NULL )
        return;

    job->title = title;

    /* Set defaults settings */
    job->chapter_start = 1;
    job->chapter_end   = hb_list_count( title->list_chapter );
    job->list_chapter = hb_chapter_list_copy( title->list_chapter );
    job->keep_duplicate_titles = title->keep_duplicate_titles;
    /* Autocrop by default. Gnark gnark */
    memcpy( job->crop, title->crop, 4 * sizeof( int ) );

    hb_geometry_t          srcGeo, resultGeo;
    hb_geometry_settings_t uiGeo;

    srcGeo = title->geometry;

    memset(&uiGeo, 0, sizeof(uiGeo));
    memcpy(uiGeo.crop, title->crop, 4 * sizeof( int ));
    uiGeo.geometry.width  = srcGeo.width  - uiGeo.crop[2] - uiGeo.crop[3];
    uiGeo.geometry.height = srcGeo.height - uiGeo.crop[0] - uiGeo.crop[1];
    uiGeo.mode = HB_ANAMORPHIC_NONE;
    uiGeo.keep = HB_KEEP_DISPLAY_ASPECT;

    hb_set_anamorphic_size2(&srcGeo, &uiGeo, &resultGeo);
    job->width  = resultGeo.width;
    job->height = resultGeo.height;
    job->par    = resultGeo.par;

    job->vcodec     = HB_VCODEC_FFMPEG_MPEG4;
    job->vquality   = HB_INVALID_VIDEO_QUALITY;
    job->vbitrate   = 1000;
    job->multipass  = 0;
    job->pass_id    = HB_PASS_ENCODE;
    job->vrate      = title->vrate;

    job->input_pix_fmt   = AV_PIX_FMT_YUV420P;
    job->output_pix_fmt  = AV_PIX_FMT_YUV420P;
    job->color_prim      = title->color_prim;
    job->color_transfer  = title->color_transfer;
    job->color_matrix    = title->color_matrix;
    job->color_range     = title->color_range;
    job->chroma_location = title->chroma_location;
    job->color_prim_override     = HB_COLR_PRI_UNSET;
    job->color_transfer_override = HB_COLR_TRA_UNSET;
    job->color_matrix_override   = HB_COLR_MAT_UNSET;

    job->mastering      = title->mastering;
    job->coll           = title->coll;
    job->ambient        = title->ambient;
    job->dovi           = title->dovi;
    job->passthru_dynamic_hdr_metadata |= title->dovi.dv_profile ? HB_HDR_DYNAMIC_METADATA_DOVI : HB_HDR_DYNAMIC_METADATA_NONE;
    job->passthru_dynamic_hdr_metadata |= title->hdr_10_plus ? HB_HDR_DYNAMIC_METADATA_HDR10PLUS : HB_HDR_DYNAMIC_METADATA_NONE;

    job->mux = HB_MUX_MP4;

    job->list_audio = hb_list_init();
    job->list_subtitle = hb_list_init();
    job->list_filter = hb_list_init();

    job->list_attachment = hb_attachment_list_copy( title->list_attachment );
    job->metadata = hb_metadata_copy( title->metadata );

    job->hw_device_index = -1;

#if HB_PROJECT_FEATURE_QSV
    job->qsv_ctx = hb_qsv_context_init();
#endif
}

int hb_output_color_prim(hb_job_t * job)
{
    if (job->color_prim_override != HB_COLR_PRI_UNSET)
        return job->color_prim_override;
    else
        return job->color_prim;
}

int hb_output_color_transfer(hb_job_t * job)
{
    if (job->color_transfer_override != HB_COLR_TRA_UNSET)
        return job->color_transfer_override;
    else
        return job->color_transfer;
}

int hb_output_color_matrix(hb_job_t * job)
{
    if (job->color_matrix_override != HB_COLR_MAT_UNSET)
        return job->color_matrix_override;
    else
        return job->color_matrix;
}

static void job_clean( hb_job_t * job )
{
    if (job)
    {
        hb_chapter_t *chapter;
        hb_audio_t *audio;
        hb_subtitle_t *subtitle;
        hb_filter_object_t *filter;
        hb_attachment_t *attachment;

        free((void*)job->json);
        job->json = NULL;
        free(job->encoder_preset);
        job->encoder_preset = NULL;
        free(job->encoder_tune);
        job->encoder_tune = NULL;
        free(job->encoder_options);
        job->encoder_options = NULL;
        free(job->encoder_profile);
        job->encoder_profile = NULL;
        free(job->encoder_level);
        job->encoder_level = NULL;
        free(job->file);
        job->file = NULL;

        hb_data_close(&job->extradata);

        // clean up chapter list
        while( ( chapter = hb_list_item( job->list_chapter, 0 ) ) )
        {
            hb_list_rem( job->list_chapter, chapter );
            hb_chapter_close( &chapter );
        }
        hb_list_close( &job->list_chapter );

        // clean up audio list
        while( ( audio = hb_list_item( job->list_audio, 0 ) ) )
        {
            hb_list_rem( job->list_audio, audio );
            hb_audio_close( &audio );
        }
        hb_list_close( &job->list_audio );

        // clean up subtitle list
        while( ( subtitle = hb_list_item( job->list_subtitle, 0 ) ) )
        {
            hb_list_rem( job->list_subtitle, subtitle );
            hb_subtitle_close( &subtitle );
        }
        hb_list_close( &job->list_subtitle );

        // clean up filter list
        while( ( filter = hb_list_item( job->list_filter, 0 ) ) )
        {
            hb_list_rem( job->list_filter, filter );
            hb_filter_close( &filter );
        }
        hb_list_close( &job->list_filter );

        // clean up attachment list
        while( ( attachment = hb_list_item( job->list_attachment, 0 ) ) )
        {
            hb_list_rem( job->list_attachment, attachment );
            hb_attachment_close( &attachment );
        }
        hb_list_close( &job->list_attachment );

        // clean up metadata
        hb_metadata_close( &job->metadata );

#if HB_PROJECT_FEATURE_QSV
        // cleanup qsv specific data
        hb_qsv_context_close(&job->qsv_ctx);
#endif
    }
}

hb_title_t * hb_find_title_by_index( hb_handle_t *h, int title_index )
{
    hb_title_set_t *title_set = hb_get_title_set( h );
    int ii;

    for (ii = 0; ii < hb_list_count(title_set->list_title); ii++)
    {
        hb_title_t *title = hb_list_item(title_set->list_title, ii);
        if (title_index == title->index)
        {
            return title;
        }
    }
    return NULL;
}

/*
 * Create a pristine job structure from a title
 * title_index is 1 based
 */
hb_job_t * hb_job_init_by_index( hb_handle_t * h, int title_index )
{
    hb_title_t * title = hb_find_title_by_index(h, title_index);
    if (title == NULL)
        return NULL;
    return hb_job_init(title);
}

hb_job_t * hb_job_init( hb_title_t * title )
{
    hb_job_t * job;

    if ( title == NULL )
        return NULL;

    job = calloc( sizeof( hb_job_t ), 1 );
    job_setup(job, title);

    return job;
}

/**********************************************************************
 * hb_job_close
 **********************************************************************
 *
 *********************************************************************/
void hb_job_close( hb_job_t ** _j )
{
    if (_j && *_j)
    {
        job_clean(*_j);
        free( *_j );
        _j = NULL;
    }
}

void hb_job_set_encoder_preset(hb_job_t *job, const char *preset)
{
    if (job != NULL)
    {
        if (preset == NULL || preset[0] == 0)
        {
            preset = NULL;
        }
        hb_update_str(&job->encoder_preset, preset);
    }
}

void hb_job_set_encoder_tune(hb_job_t *job, const char *tune)
{
    if (job != NULL)
    {
        if (tune == NULL || tune[0] == 0)
        {
            tune = NULL;
        }
        hb_update_str(&job->encoder_tune, tune);
    }
}

void hb_job_set_encoder_options(hb_job_t *job, const char *options)
{
    if (job != NULL)
    {
        if (options == NULL || options[0] == 0)
        {
            options = NULL;
        }
        hb_update_str(&job->encoder_options, options);
    }
}

void hb_job_set_encoder_profile(hb_job_t *job, const char *profile)
{
    if (job != NULL)
    {
        if (profile == NULL || profile[0] == 0)
        {
            profile = NULL;
        }
        hb_update_str(&job->encoder_profile, profile);
    }
}

void hb_job_set_encoder_level(hb_job_t *job, const char *level)
{
    if (job != NULL)
    {
        if (level == NULL || level[0] == 0)
        {
            level = NULL;
        }
        hb_update_str(&job->encoder_level, level);
    }
}

void hb_job_set_file(hb_job_t *job, const char *file)
{
    if (job != NULL)
    {
        hb_update_str(&job->file, file);
    }
}

hb_filter_object_t * hb_filter_copy( hb_filter_object_t * filter )
{
    if( filter == NULL )
        return NULL;

    hb_filter_object_t * filter_copy = malloc( sizeof( hb_filter_object_t ) );
    memcpy( filter_copy, filter, sizeof( hb_filter_object_t ) );
    if( filter->settings )
        filter_copy->settings = hb_value_dup(filter->settings);
    filter_copy->sub_filter = hb_filter_copy(filter->sub_filter);
    return filter_copy;
}

/**********************************************************************
 * hb_filter_list_copy
 **********************************************************************
 *
 *********************************************************************/
hb_list_t *hb_filter_list_copy(const hb_list_t *src)
{
    hb_list_t *list = hb_list_init();
    hb_filter_object_t *filter = NULL;
    int i;

    if( src )
    {
        for( i = 0; i < hb_list_count(src); i++ )
        {
            if( ( filter = hb_list_item( src, i ) ) )
            {
                hb_list_add( list, hb_filter_copy(filter) );
            }
        }
    }
    return list;
}

hb_dict_t * hb_filter_dict_find(const hb_value_array_t * list, int filter_id)
{
    hb_dict_t * filter = NULL;
    int ii;

    if (list == NULL)
    {
        return NULL;
    }
    for (ii = 0; ii < hb_value_array_len(list); ii++)
    {
        filter = hb_value_array_get(list, ii);
        if (hb_dict_get_int(filter, "ID") == filter_id)
        {
            return filter;
        }
    }

    return NULL;
}

hb_filter_object_t * hb_filter_find(const hb_list_t *list, int filter_id)
{
    hb_filter_object_t *filter = NULL;
    int ii;

    if (list == NULL)
    {
        return NULL;
    }
    for (ii = 0; ii < hb_list_count(list); ii++)
    {
        filter = hb_list_item(list, ii);
        if (filter->id == filter_id)
        {
            return filter;
        }
    }

    return NULL;
}

/**
 * Gets a filter object with the given type
 * @param filter_id The type of filter to get.
 * @returns The requested filter object.
 */
hb_filter_object_t * hb_filter_get( int filter_id )
{
    hb_filter_object_t * filter;
    switch( filter_id )
    {
        case HB_FILTER_DETELECINE:
            filter = &hb_filter_detelecine;
            break;

        case HB_FILTER_COMB_DETECT:
            filter = &hb_filter_comb_detect;
            break;

        case HB_FILTER_DECOMB:
            filter = &hb_filter_decomb;
            break;

        case HB_FILTER_YADIF:
            filter = &hb_filter_yadif;
            break;

        case HB_FILTER_BWDIF:
            filter = &hb_filter_bwdif;
            break;

        case HB_FILTER_COLORSPACE:
            filter = &hb_filter_colorspace;
            break;

        case HB_FILTER_VFR:
            filter = &hb_filter_vfr;
            break;

        case HB_FILTER_DEBLOCK:
            filter = &hb_filter_deblock;
            break;

        case HB_FILTER_DENOISE:
            filter = &hb_filter_denoise;
            break;

        case HB_FILTER_NLMEANS:
            filter = &hb_filter_nlmeans;
            break;

        case HB_FILTER_CHROMA_SMOOTH:
            filter = &hb_filter_chroma_smooth;
            break;

        case HB_FILTER_RENDER_SUB:
            filter = &hb_filter_render_sub;
            break;

        case HB_FILTER_RPU:
            filter = &hb_filter_rpu;
            break;

        case HB_FILTER_CROP_SCALE:
            filter = &hb_filter_crop_scale;
            break;

        case HB_FILTER_LAPSHARP:
            filter = &hb_filter_lapsharp;
            break;

        case HB_FILTER_UNSHARP:
            filter = &hb_filter_unsharp;
            break;

        case HB_FILTER_AVFILTER:
            filter = &hb_filter_avfilter;
            break;

        case HB_FILTER_PAD:
            filter = &hb_filter_pad;
            break;

        case HB_FILTER_ROTATE:
            filter = &hb_filter_rotate;
            break;

        case HB_FILTER_GRAYSCALE:
            filter = &hb_filter_grayscale;
            break;

        case HB_FILTER_FORMAT:
            filter = &hb_filter_format;
            break;

        case HB_FILTER_MT_FRAME:
            filter = &hb_filter_mt_frame;
            break;

#if defined(__APPLE__)
        case HB_FILTER_PRE_VT:
            filter = &hb_filter_prefilter_vt;
            break;

        case HB_FILTER_COMB_DETECT_VT:
            filter = &hb_filter_comb_detect_vt;
            break;

        case HB_FILTER_YADIF_VT:
            filter = &hb_filter_yadif_vt;
            break;

        case HB_FILTER_BWDIF_VT:
            filter = &hb_filter_bwdif_vt;
            break;

        case HB_FILTER_CHROMA_SMOOTH_VT:
            filter = &hb_filter_chroma_smooth_vt;
            break;

        case HB_FILTER_CROP_SCALE_VT:
            filter = &hb_filter_crop_scale_vt;
            break;

        case HB_FILTER_PAD_VT:
            filter = &hb_filter_pad_vt;
            break;

        case HB_FILTER_ROTATE_VT:
            filter = &hb_filter_rotate_vt;
            break;

        case HB_FILTER_GRAYSCALE_VT:
            filter = &hb_filter_grayscale_vt;
            break;

        case HB_FILTER_LAPSHARP_VT:
            filter = &hb_filter_lapsharp_vt;
            break;

        case HB_FILTER_UNSHARP_VT:
            filter = &hb_filter_unsharp_vt;
            break;
#endif

        default:
            filter = NULL;
            break;
    }
    return filter;
}

hb_filter_object_t * hb_filter_init( int filter_id )
{
    switch (filter_id)
    {
        case HB_FILTER_UNSHARP:
        case HB_FILTER_LAPSHARP:
        case HB_FILTER_CHROMA_SMOOTH:
        {
            hb_filter_object_t * wrapper;

            wrapper = hb_filter_copy(hb_filter_get(HB_FILTER_MT_FRAME));
            wrapper->sub_filter = hb_filter_copy(hb_filter_get(filter_id));
            wrapper->id = filter_id;
            wrapper->name = wrapper->sub_filter->name;
            return wrapper;
        } break;

        default:
            return hb_filter_copy(hb_filter_get(filter_id));
    }
}

/**********************************************************************
 * hb_filter_close
 **********************************************************************
 *
 *********************************************************************/
void hb_filter_close( hb_filter_object_t ** _f )
{
    hb_filter_object_t * f = *_f;

    if (f == NULL)
    {
        return;
    }
    hb_filter_close(&f->sub_filter);
    hb_value_free(&f->settings);

    free( f );
    *_f = NULL;
}

/**********************************************************************
 * hb_filter_info_close
 **********************************************************************
 *
 *********************************************************************/
void hb_filter_info_close( hb_filter_info_t ** _fi )
{
    hb_filter_info_t * fi = *_fi;

    if (fi != NULL)
    {
        free(fi->human_readable_desc);
    }

    free( fi );
    *_fi = NULL;
}

static char * append_string(char * dst, const char * src)
{
    int dst_len = 0, src_len, len;

    if (src == NULL)
    {
        return dst;
    }

    src_len = len = strlen(src) + 1;
    if (dst != NULL)
    {
        dst_len = strlen(dst);
        len += dst_len;
    }
    char * tmp = realloc(dst, len);
    if (tmp == NULL)
    {
        // Failed to allocate required space
        return dst;
    }
    dst = tmp;
    memcpy(dst + dst_len, src, src_len);
    return dst;
}

static char * stringify_array(int filter_id, hb_value_array_t * array)
{
    char * result = strdup("");
    int    ii;
    int    len = hb_value_array_len(array);
    int    first = 1;

    if (hb_value_array_len(array) == 0)
    {
        return result;
    }
    for (ii = 0; ii < len; ii++)
    {
        hb_value_t * val = hb_value_array_get(array, ii);
        char       * str = hb_filter_settings_string(filter_id, val);
        if (str != NULL)
        {
            if (!first)
            {
                result = append_string(result, ",");
            }
            first = 0;
            if (hb_value_type(val) == HB_VALUE_TYPE_DICT)
            {
                result = append_string(result, str);
            }
            else if (hb_value_type(val) == HB_VALUE_TYPE_ARRAY)
            {
                result = append_string(result, "[");
                result = append_string(result, str);
                result = append_string(result, "]");
            }
            else
            {
                result = append_string(result, str);
            }
            free(str);
        }
    }

    return result;
}

static char * stringify_dict(int filter_id, hb_dict_t * dict)
{
    char            * result = strdup("");
    const char      * key;
    char           ** keys = NULL;
    hb_value_t      * val;
    hb_dict_iter_t    iter;
    int               first = 1;

    if (hb_dict_elements(dict) == 0)
    {
        return result;
    }
    // Check for dict containing rational value
    if (hb_dict_elements(dict) == 2)
    {
        hb_value_t *num_val = hb_dict_get(dict, "Num");
        hb_value_t *den_val = hb_dict_get(dict, "Den");
        if (num_val != NULL && den_val != NULL &&
            hb_value_type(num_val) == HB_VALUE_TYPE_INT &&
            hb_value_type(den_val) == HB_VALUE_TYPE_INT)
        {
            int num = hb_value_get_int(num_val);
            int den = hb_value_get_int(den_val);
            char * str = hb_strdup_printf("%d/%d", num, den);
            result = append_string(result, str);
            free(str);
            return result;
        }
    }
    hb_filter_object_t * filter = hb_filter_get(filter_id);
    if (filter != NULL)
    {
        keys = hb_filter_get_keys(filter_id);
        if (keys != NULL && keys[0] == NULL)
        {
            hb_str_vfree(keys);
            keys = NULL;
        }
    }

    int done, ii = 0;
    iter = hb_dict_iter_init(dict);
    if (keys == NULL)
    {
        done = !hb_dict_iter_next_ex(dict, &iter, &key, NULL);
    }
    else
    {
        done = (key = keys[ii]) == NULL;
    }
    while (!done)
    {
        val = hb_dict_get(dict, key);
        if (val != NULL)
        {
            if (!first)
            {
                result = append_string(result, ":");
            }
            first = 0;
            result = append_string(result, key);
            int elements = 1;

            if (hb_value_type(val) == HB_VALUE_TYPE_NULL)
            {
                elements = 0;
            }
            else if (hb_value_type(val) == HB_VALUE_TYPE_DICT)
            {
                elements = hb_dict_elements(val);
            }
            else if (hb_value_type(val) == HB_VALUE_TYPE_ARRAY)
            {
                elements = hb_value_array_len(val);
            }
            if (elements != 0)
            {
                char * str = hb_filter_settings_string(filter_id, val);
                if (str != NULL)
                {
                    result = append_string(result, "=");
                    if (hb_value_type(val) == HB_VALUE_TYPE_DICT)
                    {
                        result = append_string(result, "'");
                        result = append_string(result, str);
                        result = append_string(result, "'");
                    }
                    else if (hb_value_type(val) == HB_VALUE_TYPE_ARRAY)
                    {
                        result = append_string(result, "[");
                        result = append_string(result, str);
                        result = append_string(result, "]");
                    }
                    else
                    {
                        result = append_string(result, str);
                    }
                    free(str);
                }
            }
        }
        ii++;
        if (keys == NULL)
        {
            done = !hb_dict_iter_next_ex(dict, &iter, &key, NULL);
        }
        else
        {
            done = (key = keys[ii]) == NULL;
        }
    }
    hb_str_vfree(keys);

    return result;
}

char * hb_filter_settings_string(int filter_id, hb_value_t * value)
{
    if (value == NULL || hb_value_type(value) == HB_VALUE_TYPE_NULL)
    {
        return strdup("");
    }
    if (hb_value_type(value) == HB_VALUE_TYPE_DICT)
    {
        return stringify_dict(filter_id, value);
    }
    if (hb_value_type(value) == HB_VALUE_TYPE_ARRAY)
    {
        return stringify_array(filter_id, value);
    }
    return hb_value_get_string_xform(value);
}

char * hb_filter_settings_string_json(int filter_id, const char * json)
{
    hb_value_t * value  = hb_value_json(json);
    char       * result = hb_filter_settings_string(filter_id, value);
    hb_value_free(&value);

    return result;
}

hb_dict_t * hb_parse_filter_settings(const char * settings_str)
{
    hb_dict_t  * dict = hb_dict_init();
    char      ** settings_list = hb_str_vsplit(settings_str, ':');
    int          ii;

    for (ii = 0; settings_list[ii] != NULL; ii++)
    {
        char * key, * value;
        char ** settings_pair = hb_str_vsplit(settings_list[ii], '=');
        if (settings_pair[0] == NULL || settings_pair[1] == NULL)
        {
            // Parse error. Not key=value pair.
            hb_str_vfree(settings_list);
            hb_str_vfree(settings_pair);
            hb_value_free(&dict);
            hb_log("hb_parse_filter_settings: Error parsing (%s)",
                   settings_str);
            return NULL;
        }
        key   = settings_pair[0];
        value = settings_pair[1];

        int last = strlen(value) - 1;
        // Check if value was quoted dictionary and remove quotes
        // and parse the sub-dictionary.  This should only happen
        // for avfilter settings.
        if (last > 0 && value[0] == '\'' && value[last] == '\'')
        {
            char * str = strdup(value + 1);
            str[last - 1] = 0;
            hb_dict_t * sub_dict = hb_parse_filter_settings(str);
            free(str);
            if (sub_dict == NULL)
            {
                // Parse error. Not key=value pair.
                hb_str_vfree(settings_list);
                hb_str_vfree(settings_pair);
                hb_value_free(&dict);
                hb_log("hb_parse_filter_settings: Error parsing (%s)",
                       settings_str);
                return NULL;
            }
            hb_dict_case_set(dict, key, sub_dict);
        }
        // Check if value was quoted string and remove quotes
        else if (last > 0 && value[0] == '"' && value[last] == '"')
        {
            char * str = strdup(value + 1);
            str[last - 1] = 0;
            hb_dict_case_set(dict, key, hb_value_string(str));
            free(str);
        }
        else
        {
            hb_dict_case_set(dict, key, hb_value_string(value));
        }

        hb_str_vfree(settings_pair);
    }
    hb_str_vfree(settings_list);

    return dict;
}

char * hb_parse_filter_settings_json(const char * settings_str)
{
    hb_dict_t * dict = hb_parse_filter_settings(settings_str);
    char      * result = hb_value_get_json(dict);
    hb_value_free(&dict);

    return result;
}

/**********************************************************************
 * hb_chapter_copy
 **********************************************************************
 *
 *********************************************************************/
hb_chapter_t *hb_chapter_copy(const hb_chapter_t *src)
{
    hb_chapter_t *chap = NULL;

    if ( src )
    {
        chap = calloc( 1, sizeof(*chap) );
        memcpy( chap, src, sizeof(*chap) );
        if ( src->title )
        {
            chap->title = strdup( src->title );
        }
    }
    return chap;
}

/**********************************************************************
 * hb_chapter_list_copy
 **********************************************************************
 *
 *********************************************************************/
hb_list_t *hb_chapter_list_copy(const hb_list_t *src)
{
    hb_list_t *list = hb_list_init();
    hb_chapter_t *chapter = NULL;
    int i;

    if( src )
    {
        for( i = 0; i < hb_list_count(src); i++ )
        {
            if( ( chapter = hb_list_item( src, i ) ) )
            {
                hb_list_add( list, hb_chapter_copy(chapter) );
            }
        }
    }
    return list;
}

/**********************************************************************
 * hb_chapter_close
 **********************************************************************
 *
 *********************************************************************/
void hb_chapter_close(hb_chapter_t **chap)
{
    if ( chap && *chap )
    {
        free((*chap)->title);
        free(*chap);
        *chap = NULL;
    }
}

/**********************************************************************
 * hb_chapter_set_title
 **********************************************************************
 *
 *********************************************************************/
void hb_chapter_set_title( hb_chapter_t *chapter, const char *title )
{
    if ( chapter )
    {
        hb_update_str( &chapter->title, title );
    }
}

/**********************************************************************
 * hb_chapter_set_title_by_index
 **********************************************************************
 * Applies information from the given job to the official job instance.
 * @param job Handle to hb_job_t.
 * @param chapter The chapter to apply the name to (1-based).
 * @param title to apply.
 *********************************************************************/
void hb_chapter_set_title_by_index( hb_job_t * job, int chapter_index, const char * title )
{
    hb_chapter_t * chapter = hb_list_item( job->list_chapter, chapter_index - 1 );
    hb_chapter_set_title( chapter, title );
}

/**********************************************************************
 * hb_audio_copy
 **********************************************************************
 *
 *********************************************************************/
hb_audio_t *hb_audio_copy(const hb_audio_t *src)
{
    hb_audio_t *audio = NULL;

    if( src )
    {
        audio = calloc(1, sizeof(*audio));
        memcpy(audio, src, sizeof(*audio));
        if ( src->config.out.name )
        {
            audio->config.out.name = strdup(src->config.out.name);
        }
        if ( src->config.in.name )
        {
            audio->config.in.name = strdup(src->config.in.name);
        }
        if ( src->config.list_linked_index != NULL )
        {
            int ii;
            int count = hb_list_count(src->config.list_linked_index);
            audio->config.list_linked_index = hb_list_init();
            for (ii = 0; ii < count; ii++)
            {
                hb_list_add_dup(audio->config.list_linked_index,
                                hb_list_item(src->config.list_linked_index, ii),
                                sizeof(int));
            }
        }
        audio->priv.extradata = hb_data_dup(src->priv.extradata);
    }
    return audio;
}

/**********************************************************************
 * hb_audio_list_copy
 **********************************************************************
 *
 *********************************************************************/
hb_list_t *hb_audio_list_copy(const hb_list_t *src)
{
    hb_list_t *list = hb_list_init();
    hb_audio_t *audio = NULL;
    int i;

    if( src )
    {
        for( i = 0; i < hb_list_count(src); i++ )
        {
            if( ( audio = hb_list_item( src, i ) ) )
            {
                hb_list_add( list, hb_audio_copy(audio) );
            }
        }
    }
    return list;
}

/**********************************************************************
 * hb_audio_close
 **********************************************************************
 *
 *********************************************************************/
void hb_audio_close( hb_audio_t **_audio )
{
    if ( _audio && *_audio )
    {
        hb_audio_t * audio = *_audio;
        void       * item;

        hb_data_close(&(audio)->priv.extradata);
        while ((item = hb_list_item(audio->config.list_linked_index, 0)))
        {
            hb_list_rem(audio->config.list_linked_index, item);
            free(item);
        }
        hb_list_close(&audio->config.list_linked_index);
        free((char*)audio->config.in.name);
        free((char*)audio->config.out.name);
        free(audio);
        *_audio = NULL;
    }
}

/**********************************************************************
 * hb_audio_new
 **********************************************************************
 *
 *********************************************************************/
void hb_audio_config_init(hb_audio_config_t * audiocfg)
{
    audiocfg->index = 0;
    audiocfg->list_linked_index = NULL;

    /* Set read-only parameters to invalid values */
    audiocfg->in.codec = 0;
    audiocfg->in.codec_param = 0;
    audiocfg->in.reg_desc = 0;
    audiocfg->in.stream_type = 0;
    audiocfg->in.substream_type = 0;
    audiocfg->in.version = 0;
    audiocfg->in.flags = 0;
    audiocfg->in.mode = 0;
    audiocfg->in.samplerate = -1;
    audiocfg->in.sample_bit_depth = 0;
    audiocfg->in.samples_per_frame = -1;
    audiocfg->in.bitrate = -1;
    audiocfg->in.matrix_encoding = AV_MATRIX_ENCODING_NONE;
    audiocfg->in.channel_layout = 0;
    audiocfg->in.channel_map = NULL;
    audiocfg->lang.description[0] = 0;
    audiocfg->lang.simple[0] = 0;
    audiocfg->lang.iso639_2[0] = 0;

    /* Initialize some sensible defaults */
    audiocfg->in.track = audiocfg->out.track = 0;
    audiocfg->out.codec = hb_audio_encoder_get_default(HB_MUX_MP4); // default container
    audiocfg->out.samplerate = -1;
    audiocfg->out.samples_per_frame = -1;
    audiocfg->out.bitrate = -1;
    audiocfg->out.quality = HB_INVALID_AUDIO_QUALITY;
    audiocfg->out.compression_level = -1;
    audiocfg->out.mixdown = HB_INVALID_AMIXDOWN;
    audiocfg->out.dynamic_range_compression = 0;
    audiocfg->out.gain = 0;
    audiocfg->out.normalize_mix_level = 0;
    audiocfg->out.dither_method = hb_audio_dither_get_default();
    audiocfg->out.name = NULL;
}

/**********************************************************************
 * hb_audio_add
 **********************************************************************
 *
 *********************************************************************/
int hb_audio_add(const hb_job_t * job, const hb_audio_config_t * audiocfg)
{
    hb_title_t *title = job->title;
    hb_audio_t *audio;

    audio = hb_audio_copy( hb_list_item( title->list_audio, audiocfg->index ) );
    if( audio == NULL )
    {
        /* We fail! */
        return 0;
    }

    if( (audiocfg->in.bitrate != -1) && (audiocfg->in.codec != 0xDEADBEEF) )
    {
        /* This most likely means the client didn't call hb_audio_config_init
         * so bail. */
        hb_audio_close(&audio);
        return 0;
    }

    /* Really shouldn't ignore the passed out track, but there is currently no
     * way to handle duplicates or out-of-order track numbers. */
    audio->config.out = audiocfg->out;
    audio->config.out.track = hb_list_count(job->list_audio) + 1;
    if (audiocfg->out.name && *audiocfg->out.name)
    {
        audio->config.out.name = strdup(audiocfg->out.name);
    }

    hb_list_add(job->list_audio, audio);
    return 1;
}

hb_audio_config_t * hb_list_audio_config_item(hb_list_t * list, int i)
{
    hb_audio_t *audio = NULL;

    if( (audio = hb_list_item(list, i)) )
        return &(audio->config);

    return NULL;
}

/**********************************************************************
 * hb_subtitle_config_copy
 **********************************************************************
 *
 *********************************************************************/
void hb_subtitle_config_copy(hb_subtitle_config_t *dst,
                             const hb_subtitle_config_t *src)
{
    if (dst == NULL || src == NULL)
    {
        return;
    }
    memcpy(dst, src, sizeof(*src));
    if (src->name != NULL)
    {
        dst->name = strdup(src->name);
    }
    if (src->src_filename != NULL)
    {
        dst->src_filename = strdup(src->src_filename);
    }
    if (src->external_filename != NULL)
    {
        dst->external_filename = strdup(src->external_filename);
    }
}

/**********************************************************************
 * hb_subtitle_copy
 **********************************************************************
 *
 *********************************************************************/
hb_subtitle_t *hb_subtitle_copy(const hb_subtitle_t *src)
{
    hb_subtitle_t *subtitle = NULL;

    if( src )
    {
        subtitle = calloc(1, sizeof(*subtitle));
        memcpy(subtitle, src, sizeof(*subtitle));
        if (src->extradata)
        {
            subtitle->extradata = hb_data_dup(src->extradata);
        }
        if (src->name != NULL)
        {
            subtitle->name = strdup(src->name);
        }
        if (src->config.name != NULL)
        {
            subtitle->config.name = strdup(src->config.name);
        }
        if (src->config.src_filename)
        {
            subtitle->config.src_filename = strdup(src->config.src_filename);
        }
    }
    return subtitle;
}

/**********************************************************************
 * hb_subtitle_list_copy
 **********************************************************************
 *
 *********************************************************************/
hb_list_t *hb_subtitle_list_copy(const hb_list_t *src)
{
    hb_list_t *list = hb_list_init();
    hb_subtitle_t *subtitle = NULL;
    int i;

    if( src )
    {
        for( i = 0; i < hb_list_count(src); i++ )
        {
            if( ( subtitle = hb_list_item( src, i ) ) )
            {
                hb_list_add( list, hb_subtitle_copy(subtitle) );
            }
        }
    }
    return list;
}

/**********************************************************************
 * hb_subtitle_close
 **********************************************************************
 *
 *********************************************************************/
void hb_subtitle_close( hb_subtitle_t **_sub )
{
    hb_subtitle_t * sub = *_sub;
    if ( _sub && sub )
    {
        free((char*)sub->name);
        free((char*)sub->config.name);
        free((char*)sub->config.src_filename);
        hb_data_close(&sub->extradata);
        free(sub);
        *_sub = NULL;
    }
}

/**********************************************************************
 * hb_subtitle_extradata_init
 **********************************************************************
 * Initializes avcodec format subtitle extradata
 * Returns:
 *  0  - no action taken, already set or does not require extradata
 *  1  - set extradata
 *  -1 - Error
 *********************************************************************/
int hb_subtitle_extradata_init(hb_subtitle_t * subtitle)
{
    if (subtitle->extradata != NULL)
    {
        // Already initialized
         return 0;
     }

    if (subtitle->source == VOBSUB)
    {
        uint32_t  rgb[16];
        char     *sub_idx;

        for (int ii = 0; ii < 16; ii++)
        {
            rgb[ii] = hb_yuv2rgb(subtitle->palette[ii]);
        }

        sub_idx = hb_strdup_printf(
            "size: %dx%d\n"
            "org: %d, %d\n"
            "scale: 100%%, 100%%\n"
            "alpha: 100%%\n"
            "smooth: OFF\n"
            "fadein/out: 50, 50\n"
            "align: OFF at LEFT TOP\n"
            "time offset: 0\n"
            "forced subs: %s\n"
            "palette: %06x, %06x, %06x, %06x, %06x, %06x, "
            "%06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x, %06x\n"
            "custom colors: OFF, tridx: 0000, "
            "colors: 000000, 000000, 000000, 000000\n",
            subtitle->width, subtitle->height, 0, 0, "OFF",
            rgb[0],  rgb[1],  rgb[2],  rgb[3],
            rgb[4],  rgb[5],  rgb[6],  rgb[7],
            rgb[8],  rgb[9],  rgb[10], rgb[11],
            rgb[12], rgb[13], rgb[14], rgb[15]);
        if (sub_idx == NULL)
        {
            return -1;
        }

        hb_set_extradata(&subtitle->extradata,
                         (const uint8_t *)sub_idx,
                         strlen(sub_idx));
        free(sub_idx);

        return 1;
    }
    return 0;
}

/**********************************************************************
 * hb_subtitle_add
 **********************************************************************
 *
 *********************************************************************/
int hb_subtitle_add(const hb_job_t * job, const hb_subtitle_config_t * subtitlecfg, int track)
{
    hb_title_t *title = job->title;
    hb_subtitle_t *subtitle;

    subtitle = hb_subtitle_copy( hb_list_item( title->list_subtitle, track ) );
    if( subtitle == NULL )
    {
        /* We fail! */
        return 0;
    }

    // "track" in title->list_audio is an index into the source's tracks.
    // "track" in job->list_audio is an index into title->list_audio
    subtitle->track = track;
    hb_subtitle_config_copy(&subtitle->config, subtitlecfg);
    subtitle->config.src_filename = NULL;
    subtitle->out_track = hb_list_count(job->list_subtitle) + 1;
    hb_list_add(job->list_subtitle, subtitle);
    return 1;
}

int hb_import_subtitle_add( const hb_job_t * job,
                const hb_subtitle_config_t * subtitlecfg,
                const char *lang_code, int source )
{
    hb_subtitle_t *subtitle;
    iso639_lang_t *lang = NULL;

    subtitle = calloc( 1, sizeof( *subtitle ) );
    if (subtitle == NULL)
    {
        hb_error("hb_srt_add: malloc failed");
        return 0;
    }

    subtitle->id           = (hb_list_count(job->list_subtitle) << 8) |
                             HB_SUBTITLE_IMPORT_TAG;
    subtitle->format       = TEXTSUB;
    subtitle->source       = source;
    subtitle->codec        = WORK_DECSRTSUB;
    subtitle->codec        = source == IMPORTSRT ? WORK_DECSRTSUB :
                                                   WORK_DECSSASUB;
    subtitle->codec_param  = source == IMPORTSRT ? AV_CODEC_ID_SUBRIP :
                                                   AV_CODEC_ID_ASS;
    subtitle->timebase.num = 1;
    subtitle->timebase.den = 90000;

    lang = lang_for_code2(lang_code);
    if (lang == NULL)
    {
        hb_log("hb_srt_add: unknown language code (%s)", lang_code);
        lang = lang_for_code2("und");
    }
    snprintf(subtitle->lang, sizeof(subtitle->lang), "%s (%s)",
             strlen(lang->native_name) ? lang->native_name : lang->eng_name,
             hb_subsource_name(subtitle->source));
    strcpy(subtitle->iso639_2, lang->iso639_2);

    hb_subtitle_config_copy(&subtitle->config, subtitlecfg);
    hb_list_add(job->list_subtitle, subtitle);

    return 1;
}

int hb_srt_add( const hb_job_t * job,
                const hb_subtitle_config_t * subtitlecfg,
                const char *lang_code )
{
    return hb_import_subtitle_add(job, subtitlecfg, lang_code, IMPORTSRT);
}

int hb_subtitle_can_force( int source )
{
    return source == VOBSUB || source == PGSSUB;
}

int hb_subtitle_can_burn( int source )
{
    return source == VOBSUB    || source == PGSSUB    || source == SSASUB  ||
           source == CC608SUB  || source == UTF8SUB   || source == TX3GSUB ||
           source == IMPORTSRT || source == IMPORTSSA || source == DVBSUB;
}

int hb_subtitle_can_export( int source )
{
    switch (source)
    {
        case CC608SUB:
        case CC708SUB:
        case UTF8SUB:
        case TX3GSUB:
        case SSASUB:
        case IMPORTSRT:
        case IMPORTSSA:
        case PGSSUB:
            return 1;
        default:
            return 0;
    }
}

int hb_subtitle_can_pass( int source, int mux )
{
    switch (mux)
    {
        case HB_MUX_AV_MKV:
            switch( source )
            {
                case DVBSUB:
                case PGSSUB:
                case VOBSUB:
                case SSASUB:
                case UTF8SUB:
                case TX3GSUB:
                case CC608SUB:
                case CC708SUB:
                case IMPORTSRT:
                case IMPORTSSA:
                    return 1;

                default:
                    return 0;
            } break;

        case HB_MUX_AV_MP4:
            switch( source )
            {
                case VOBSUB:
                case SSASUB:
                case UTF8SUB:
                case TX3GSUB:
                case CC608SUB:
                case CC708SUB:
                case IMPORTSRT:
                case IMPORTSSA:
                    return 1;

                default:
                    return 0;
            } break;

        // webm can't support subtitles unless they're burned.
        // there's ambiguity in the spec about WebVTT... TODO
        case HB_MUX_AV_WEBM:
            return 0;
        default:
            // Internal error. Should never get here.
            hb_error("internal error.  Bad mux %d\n", mux);
            return 0;
    }
}

int hb_subtitle_must_burn(hb_subtitle_t *subtitle, int mux)
{
    return (subtitle->config.dest == RENDERSUB ||
             (!(subtitle->config.external_filename != NULL &&
                hb_subtitle_can_export(subtitle->source)) &&
              !hb_subtitle_can_pass(subtitle->source, mux)));
}

int hb_audio_can_apply_drc(uint32_t codec, uint32_t codec_param, int encoder)
{
    if (encoder & HB_ACODEC_PASS_FLAG)
    {
        // can't apply DRC to passthru audio
        return 0;
    }
    else if (codec & HB_ACODEC_FF_MASK)
    {
        return (codec_param == AV_CODEC_ID_AC3 ||
                codec_param == AV_CODEC_ID_EAC3);
    }
    else if (codec == HB_ACODEC_AC3)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int hb_audio_can_apply_drc2(hb_handle_t *h, int title_idx, int audio_idx, int encoder)
{
    hb_title_t *title = hb_find_title_by_index(h, title_idx);
    if (title == NULL)
        return 0;

    hb_audio_t *audio = hb_list_item(title->list_audio, audio_idx);
    if (audio == NULL)
        return 0;

    return hb_audio_can_apply_drc(audio->config.in.codec,
                                  audio->config.in.codec_param, encoder);
}

/**********************************************************************
 * hb_metadata_init
 **********************************************************************
 *
 *********************************************************************/
hb_metadata_t *hb_metadata_init()
{
    hb_metadata_t *metadata = calloc( 1, sizeof(*metadata) );
    if (metadata != NULL)
    {
        metadata->dict = hb_dict_init();
    }
    return metadata;
}

/**********************************************************************
 * hb_metadata_copy
 **********************************************************************
 *
 *********************************************************************/
hb_metadata_t *hb_metadata_copy( const hb_metadata_t *src )
{
    hb_metadata_t *metadata = NULL;

    if ( src )
    {
        metadata = calloc( 1, sizeof(*metadata) );
        if (src->dict != NULL)
        {
            metadata->dict = hb_value_dup(src->dict);
        }
        if ( src->list_coverart )
        {
            int ii;
            for ( ii = 0; ii < hb_list_count( src->list_coverart ); ii++ )
            {
                hb_coverart_t *art = hb_list_item( src->list_coverart, ii );
                hb_metadata_add_coverart(
                        metadata, art->data, art->size, art->type, art->name );
            }
        }
    }
    return metadata;
}

/**********************************************************************
 * hb_metadata_close
 **********************************************************************
 *
 *********************************************************************/
void hb_metadata_close( hb_metadata_t **_m )
{
    if ( _m && *_m )
    {
        hb_metadata_t *m = *_m;
        hb_coverart_t *art;

        if (m->dict != NULL)
        {
            hb_value_free(&m->dict);
        }

        if ( m->list_coverart )
        {
            while( ( art = hb_list_item( m->list_coverart, 0 ) ) )
            {
                hb_list_rem( m->list_coverart, art );
                free( art->name );
                free( art->data );
                free( art );
            }
            hb_list_close( &m->list_coverart );
        }

        free( m );
        *_m = NULL;
    }
}

/**********************************************************************
 * hb_metadata_set_*
 **********************************************************************
 *
 *********************************************************************/
void hb_update_meta_dict(hb_dict_t * dict, const char * key, const char * value)
{
    if (value != NULL)
    {
        hb_dict_set_string(dict, key, value);
    }
    else
    {
        hb_dict_remove(dict, key);
    }
}

void hb_metadata_add_coverart( hb_metadata_t *metadata,
                               const uint8_t *data, int size,
                               int type, const char *name )
{
    if ( metadata )
    {
        if ( metadata->list_coverart == NULL )
        {
            metadata->list_coverart = hb_list_init();
        }
        hb_coverart_t *art = calloc( 1, sizeof(hb_coverart_t) );
        if (name)
        {
            art->name = strdup( name );
        }
        art->data = malloc( size );
        memcpy( art->data, data, size );
        art->size = size;
        art->type = type;
        hb_list_add( metadata->list_coverart, art );
    }
}

void hb_metadata_rem_coverart( hb_metadata_t *metadata, int idx )
{
    if ( metadata )
    {
        hb_coverart_t *art = hb_list_item( metadata->list_coverart, idx );
        if ( art )
        {
            hb_list_rem( metadata->list_coverart, art );
            free( art->name );
            free( art->data );
            free( art );
        }
    }
}

// Copied from jansson strconv.c

void hb_str_to_locale(char *str)
{
    const char *point;
    char *pos;

    point = localeconv()->decimal_point;
    if (*point == '.')
    {
        // No conversion needed
        return;
    }

    pos = strchr(str, '.');
    if (pos)
    {
        *pos = *point;
    }
}

void hb_str_from_locale(char *str)
{
    const char *point;
    char *pos;

    point = localeconv()->decimal_point;
    if (*point == '.')
    {
        // No conversion needed
        return;
    }

    pos = strchr(str, *point);
    if (pos)
    {
        *pos = '.';
    }
}

char * hb_strdup_vaprintf( const char * fmt, va_list args )
{
    int       len;
    int       size = 256;
    char    * str;
    char    * tmp;
    va_list   copy;

    str = malloc( size );
    if ( str == NULL )
        return NULL;

    while (1)
    {
        // vsnprintf modifies it's va_list.  Since we may need to do this
        // more than once, use a copy of the va_list.
        va_copy(copy, args);

        /* Try to print in the allocated space. */
        len = vsnprintf( str, size, fmt, copy );

        /* If that worked, return the string. */
        if ( len > -1 && len < size )
        {
            return str;
        }

        /* Else try again with more space. */
        if ( len > -1 )     /* glibc 2.1 */
            size = len + 1; /* precisely what is needed */
        else                /* glibc 2.0 */
            size *= 2;      /* twice the old size */
        tmp = realloc( str, size );
        if ( tmp == NULL )
        {
            free( str );
            return NULL;
        }
        else
            str = tmp;
    }

    return str;
}

char * hb_strdup_printf( const char * fmt, ... )
{
    char    * str;
    va_list   args;

    va_start( args, fmt );
    str = hb_strdup_vaprintf( fmt, args );
    va_end( args );

    return str;
}

char * hb_strncat_dup( const char * s1, const char * s2, size_t n )
{
    size_t len;
    char * str;

    len = 0;
    if( s1 )
        len += strlen( s1 );
    if( s2 )
        len += MAX( strlen( s2 ), n );
    if( !len )
        return NULL;

    str = malloc( len + 1 );
    if( !str )
        return NULL;

    if( s1 )
        strcpy( str, s1 );
    else
        strcpy( str, "" );

    if (s2)
    {
        strncat( str, s2, n );
    }

    return str;
}

/**********************************************************************
 * hb_attachment_copy
 **********************************************************************
 *
 *********************************************************************/
hb_attachment_t *hb_attachment_copy(const hb_attachment_t *src)
{
    hb_attachment_t *attachment = NULL;

    if( src )
    {
        attachment = calloc(1, sizeof(*attachment));
        memcpy(attachment, src, sizeof(*attachment));
        if ( src->name )
        {
            attachment->name = strdup( src->name );
        }
        if ( src->data )
        {
            attachment->data = malloc( src->size );
            memcpy( attachment->data, src->data, src->size );
        }
    }
    return attachment;
}

/**********************************************************************
 * hb_attachment_list_copy
 **********************************************************************
 *
 *********************************************************************/
hb_list_t *hb_attachment_list_copy(const hb_list_t *src)
{
    hb_list_t *list = hb_list_init();
    hb_attachment_t *attachment = NULL;
    int i;

    if( src )
    {
        for( i = 0; i < hb_list_count(src); i++ )
        {
            if( ( attachment = hb_list_item( src, i ) ) )
            {
                hb_list_add( list, hb_attachment_copy(attachment) );
            }
        }
    }
    return list;
}

/**********************************************************************
 * hb_attachment_close
 **********************************************************************
 *
 *********************************************************************/
void hb_attachment_close( hb_attachment_t **attachment )
{
    if ( attachment && *attachment )
    {
        free((*attachment)->data);
        free((*attachment)->name);
        free(*attachment);
        *attachment = NULL;
    }
}

/**********************************************************************
 * hb_yuv2rgb
 **********************************************************************
 * Converts a YCrCb pixel to an RGB pixel.
 *
 * This conversion is lossy (due to rounding and clamping).
 *
 * Algorithm:
 *   http://en.wikipedia.org/w/index.php?title=YCbCr&oldid=361987695#Technical_details
 *********************************************************************/
int hb_yuv2rgb(int yuv)
{
    double y, Cr, Cb;
    int r, g, b;

    y  = (yuv >> 16) & 0xff;
    Cr = (yuv >>  8) & 0xff;
    Cb = (yuv      ) & 0xff;

    r = 1.164 * (y - 16)                      + 1.596 * (Cr - 128);
    g = 1.164 * (y - 16) - 0.392 * (Cb - 128) - 0.813 * (Cr - 128);
    b = 1.164 * (y - 16) + 2.017 * (Cb - 128);

    r = (r < 0) ? 0 : r;
    g = (g < 0) ? 0 : g;
    b = (b < 0) ? 0 : b;

    r = (r > 255) ? 255 : r;
    g = (g > 255) ? 255 : g;
    b = (b > 255) ? 255 : b;

    return (r << 16) | (g << 8) | b;
}

/**********************************************************************
 * hb_rgb2yuv
 **********************************************************************
 * Converts an RGB pixel to a limited range Bt.601 YCrCb pixel.
 *
 * This conversion is lossy (due to rounding and clamping).
 *
 * Algorithm:
 *   http://en.wikipedia.org/w/index.php?title=YCbCr&oldid=361987695#Technical_details
 *********************************************************************/
int hb_rgb2yuv(int rgb)
{
    double r, g, b;
    int y, Cr, Cb;

    r = (rgb >> 16) & 0xff;
    g = (rgb >>  8) & 0xff;
    b = (rgb      ) & 0xff;

    y  =  16. + ( 0.257 * r) + (0.504 * g) + (0.098 * b);
    Cb = 128. + (-0.148 * r) - (0.291 * g) + (0.439 * b);
    Cr = 128. + ( 0.439 * r) - (0.368 * g) - (0.071 * b);

    y = (y < 0) ? 0 : y;
    Cb = (Cb < 0) ? 0 : Cb;
    Cr = (Cr < 0) ? 0 : Cr;

    y = (y > 255) ? 255 : y;
    Cb = (Cb > 255) ? 255 : Cb;
    Cr = (Cr > 255) ? 255 : Cr;

    return (y << 16) | (Cr << 8) | Cb;
}

int hb_rgb2yuv_bt709(int rgb)
{
    double r, g, b;
    int y, Cr, Cb;

    r = (rgb >> 16) & 0xff;
    g = (rgb >>  8) & 0xff;
    b = (rgb      ) & 0xff;

    y  =  16. + ( 0.1826 * r) + (0.6142 * g) + (0.0620 * b);
    Cb = 128. + (-0.1006 * r) - (0.3386 * g) + (0.4392 * b);
    Cr = 128. + ( 0.4392 * r) - (0.3986 * g) - (0.0403 * b);

    y = (y < 0) ? 0 : y;
    Cb = (Cb < 0) ? 0 : Cb;
    Cr = (Cr < 0) ? 0 : Cr;

    y = (y > 255) ? 255 : y;
    Cb = (Cb > 255) ? 255 : Cb;
    Cr = (Cr > 255) ? 255 : Cr;

    return (y << 16) | (Cr << 8) | Cb;
}

int hb_rgb2yuv_bt2020(int rgb)
{
    double r, g, b;
    int y, Cr, Cb;

    r = (rgb >> 16) & 0xff;
    g = (rgb >>  8) & 0xff;
    b = (rgb      ) & 0xff;

    y  =  16. + ( 0.2307 * r) + (0.5956 * g) + (0.0521 * b);
    Cb = 128. + (-0.1227 * r) - (0.3166 * g) + (0.4392 * b);
    Cr = 128. + ( 0.4392 * r) - (0.4039 * g) - (0.0353 * b);

    y = (y < 0) ? 0 : y;
    Cb = (Cb < 0) ? 0 : Cb;
    Cr = (Cr < 0) ? 0 : Cr;

    y = (y > 255) ? 255 : y;
    Cb = (Cb > 255) ? 255 : Cb;
    Cr = (Cr > 255) ? 255 : Cr;

    return (y << 16) | (Cr << 8) | Cb;
}

hb_csp_convert_f hb_get_rgb2yuv_function(int color_matrix)
{
    switch (color_matrix)
    {
    case HB_COLR_MAT_BT470BG:
    case HB_COLR_MAT_SMPTE170M:
    case HB_COLR_MAT_FCC:
        return hb_rgb2yuv;
    case HB_COLR_MAT_BT2020_NCL:
    case HB_COLR_MAT_BT2020_CL: //wrong
        return hb_rgb2yuv_bt2020;
    default: //assume 709 for the rest
        break;
    }
    return hb_rgb2yuv_bt709;
}

void hb_compute_chroma_smoothing_coefficient(uint32_t chroma_coeffs[2][4], int pix_fmt, int chroma_location)
{
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);

    // Compute chroma smoothing coefficients wrt video chroma location
    int wX, wY;
    wX = 4 - (1 << desc->log2_chroma_w);
    wY = 4 - (1 << desc->log2_chroma_h);

    switch (chroma_location)
    {
        case AVCHROMA_LOC_TOPLEFT:
            wX += (1 << desc->log2_chroma_w) - 1;
        case AVCHROMA_LOC_TOP:
            wY += (1 << desc->log2_chroma_h) - 1;
            break;
        case AVCHROMA_LOC_LEFT:
            wX += (1 << desc->log2_chroma_w) - 1;
            break;
        case AVCHROMA_LOC_BOTTOMLEFT:
            wX += (1 << desc->log2_chroma_w) - 1;
        case AVCHROMA_LOC_BOTTOM:
            wY += (1 << desc->log2_chroma_h) - 1;
        case AVCHROMA_LOC_CENTER:
        default: // Center chroma value for unknown/unsupported
            break;
    }

    const uint32_t base_coefficients[] = {1, 3, 9, 27, 9, 3, 1};
    // If wZ is even, an intermediate value is interpolated for symmetry.
    for (int x = 0; x < 4; x++)
    {
        chroma_coeffs[0][x] = (base_coefficients[x + wX] +
                               base_coefficients[x + wX + !(wX & 0x1)]) >> 1;
        chroma_coeffs[1][x] = (base_coefficients[x + wY] +
                               base_coefficients[x + wY + !(wY & 0x1)]) >> 1;
    }
}

const char * hb_subsource_name( int source )
{
    switch (source)
    {
        case VOBSUB:
            return "VOBSUB";
        case IMPORTSRT:
            return "SRT";
        case CC608SUB:
            return "CC608";
        case CC708SUB:
            return "CC708";
        case UTF8SUB:
            return "UTF-8";
        case TX3GSUB:
            return "TX3G";
        case IMPORTSSA:
        case SSASUB:
            return "SSA";
        case PGSSUB:
            return "PGS";
        case DVBSUB:
            return "DVB";
        default:
            return "Unknown";
    }
}

void hb_hexdump( hb_debug_level_t level, const char * label, const uint8_t * data, int len )
{
    int ii;
    char line[80], ascii[19], *p;

    ascii[18] = 0;
    ascii[0] = '|';
    ascii[17] = '|';
    memset(&ascii[1], '.', 16);
    p = line;
    if( label )
        hb_deep_log(level, "++++ %s ++++", label);
    else
        hb_deep_log(level, "++++++++++++");
    for( ii = 0; ii < len; ii++ )
    {
        if( ( ii & 0x0f ) == 0x0f )
        {
            hb_deep_log( level, "    %-50s%20s", line, ascii );
            memset(&ascii[1], '.', 16);
            p = line;
        }
        else if( ( ii & 0x07 ) == 0x07 )
        {
            p += snprintf( p, sizeof(line), "%02x  ", data[ii] );
        }
        else
        {
            p += snprintf( p, sizeof(line), "%02x ", data[ii] );
        }
        if( isgraph( data[ii] ) )
            ascii[(ii & 0x0f) + 1] = data[ii];
        else
            ascii[(ii & 0x0f) + 1] = '.';
    }
    if( p != line )
    {
        hb_deep_log( level, "    %-50s%20s", line, ascii );
    }
}

int hb_str_vlen(char **strv)
{
    int i;
    if (strv == NULL)
        return 0;
    for (i = 0; strv[i]; i++);
    return i;
}

static const char* strchr_quote(const char *pos, char c, char q)
{
    if (pos == NULL)
        return NULL;

    while (*pos != 0 && *pos != c)
    {
        if (*pos == q)
        {
            pos = strchr_quote(pos+1, q, 0);
            if (pos == NULL)
                return NULL;
            pos++;
        }
        else if (*pos == '\\' && *(pos+1) != 0)
            pos += 2;
        else
            pos++;
    }
    if (*pos != c)
        return NULL;
    return pos;
}

static char *strndup_quote(const char *str, char q, int len)
{
    if (str == NULL)
        return NULL;

    char * res;
    int str_len = strlen( str );
    int src = 0, dst = 0;
    res = malloc( len > str_len ? str_len + 1 : len + 1 );
    if ( res == NULL ) return res;

    while (str[src] != 0 && src < len)
    {
        if (str[src] == q)
            src++;
        else
            res[dst++] = str[src++];
    }
    res[dst] = '\0';
    return res;
}

char** hb_str_vsplit( const char *str, char delem )
{
    const char *  pos;
    const char *  end;
    char       ** ret;
    int           count, i;
    char          quote = '"';

    if (delem == '"')
    {
        quote = '\'';
    }
    if ( str == NULL || str[0] == 0 )
    {
        ret = malloc( sizeof(char*) );
        if ( ret == NULL ) return ret;
        *ret = NULL;
        return ret;
    }

    // Find number of elements in the string
    count = 1;
    pos = str;
    while ( ( pos = strchr_quote( pos, delem, quote ) ) != NULL )
    {
        count++;
        pos++;
    }

    ret = calloc( ( count + 1 ), sizeof(char*) );
    if ( ret == NULL ) return ret;

    pos = str;
    for ( i = 0; i < count - 1; i++ )
    {
        end = strchr_quote( pos, delem, quote );
        ret[i] = strndup_quote(pos, quote, end - pos);
        pos = end + 1;
    }
    ret[i] = strndup_quote(pos, quote, strlen(pos));

    return ret;
}

void hb_str_vfree( char **strv )
{
    int i;

    if (strv == NULL)
        return;

    for ( i = 0; strv[i]; i++ )
    {
        free( strv[i] );
    }
    free( strv );
}

hb_chapter_queue_t * hb_chapter_queue_init(void)
{
    hb_chapter_queue_t * q;

    q = calloc(1, sizeof(*q));
    if (q != NULL)
    {
        q->list_chapter = hb_list_init();
        if (q->list_chapter == NULL)
        {
            free(q);
            q = NULL;
        }
    }
    return q;
}

void hb_chapter_queue_close(hb_chapter_queue_t **_q)
{
    hb_chapter_queue_t      * q = *_q;
    hb_chapter_queue_item_t * item;

    if (q == NULL)
    {
        return;
    }
    while ((item = hb_list_item(q->list_chapter, 0)) != NULL)
    {
        hb_list_rem(q->list_chapter, item);
        free(item);
    }
    hb_list_close(&q->list_chapter);
    free(q);
    *_q = NULL;
}

void hb_chapter_enqueue(hb_chapter_queue_t *q, hb_buffer_t *buf)
{
    /*
     * Chapter markers are sometimes so close we can get a new
     * one before the previous goes through the encoding queue.
     *
     * Dropping markers can cause weird side-effects downstream,
     * including but not limited to missing chapters in the
     * output, so we need to save it somehow.
     */
    hb_chapter_queue_item_t *item = malloc(sizeof(hb_chapter_queue_item_t));
    if (item != NULL)
    {
        item->start = buf->s.start;
        item->new_chap = buf->s.new_chap;
        // Make sure work_loop doesn't also copy the chapter mark
        buf->s.new_chap = 0;
        hb_list_add(q->list_chapter, item);
    }
}

void hb_chapter_dequeue(hb_chapter_queue_t *q, hb_buffer_t *buf)
{
    hb_chapter_queue_item_t *item = hb_list_item(q->list_chapter, 0);
    if (item != NULL)
    {
        if (buf->s.start < item->start)
        {
            // Have not reached the next chapter yet.
            return;
        }

        // we're done with this chapter
        hb_list_rem(q->list_chapter, item);
        buf->s.new_chap = item->new_chap;
        free(item);
    }
}

int hb_get_bit_depth(int format)
{
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(format);
    int i, min, max;

    if (!desc || !desc->nb_components)
    {
        return -1;
    }

    min = INT_MAX, max = -INT_MAX;
    for (i = 0; i < desc->nb_components; i++)
    {
        min = FFMIN(desc->comp[i].depth, min);
        max = FFMAX(desc->comp[i].depth, max);
    }

    return max;
}

int hb_get_color_prim(int color_primaries, hb_geometry_t geometry, hb_rational_t rate)
{
    switch (color_primaries)
    {
        case AVCOL_PRI_BT709:
            return HB_COLR_PRI_BT709;
        case AVCOL_PRI_BT470M:
            return HB_COLR_PRI_BT470M;
        case AVCOL_PRI_BT470BG:
            return HB_COLR_PRI_EBUTECH;
        case AVCOL_PRI_SMPTE170M:
        case AVCOL_PRI_SMPTE240M:
            return HB_COLR_PRI_SMPTEC;
        case AVCOL_PRI_FILM:
            return HB_COLR_PRI_FILM;
        case AVCOL_PRI_SMPTE428:
            return HB_COLR_PRI_SMPTE428;
        case AVCOL_PRI_SMPTE431:
            return HB_COLR_PRI_SMPTE431;
        case AVCOL_PRI_SMPTE432:
            return HB_COLR_PRI_SMPTE432;
        case AVCOL_PRI_JEDEC_P22:
            return HB_COLR_PRI_JEDEC_P22;
        case AVCOL_PRI_BT2020:
            return HB_COLR_PRI_BT2020;
        default:
        {
            if ((geometry.width >= 1280 || geometry.height >= 720)||
                (geometry.width >   720 && geometry.height >  576 ))
                // ITU BT.709 HD content
                return HB_COLR_PRI_BT709;
            else if (rate.den == 1080000)
                // ITU BT.601 DVD or SD TV content (PAL)
                return HB_COLR_PRI_EBUTECH;
            else
                // ITU BT.601 DVD or SD TV content (NTSC)
                return HB_COLR_PRI_SMPTEC;
        }
    }
}

int hb_get_color_transfer(int color_trc)
{
    switch (color_trc)
    {
        case AVCOL_TRC_GAMMA22:
            return HB_COLR_TRA_GAMMA22;
        case AVCOL_TRC_GAMMA28:
            return HB_COLR_TRA_GAMMA28;
        case AVCOL_TRC_SMPTE170M:
            return HB_COLR_TRA_SMPTE170M;
        case AVCOL_TRC_LINEAR:
            return HB_COLR_TRA_LINEAR;
        case AVCOL_TRC_LOG:
            return HB_COLR_TRA_LOG;
        case AVCOL_TRC_LOG_SQRT:
            return HB_COLR_TRA_LOG_SQRT;
        case AVCOL_TRC_IEC61966_2_4:
            return HB_COLR_TRA_IEC61966_2_4;
        case AVCOL_TRC_BT1361_ECG:
            return HB_COLR_TRA_BT1361_ECG;
        case AVCOL_TRC_IEC61966_2_1:
            return HB_COLR_TRA_IEC61966_2_1;
        case AVCOL_TRC_SMPTE240M:
            return HB_COLR_TRA_SMPTE240M;
        case AVCOL_TRC_SMPTEST2084:
            return HB_COLR_TRA_SMPTEST2084;
        case AVCOL_TRC_ARIB_STD_B67:
            return HB_COLR_TRA_ARIB_STD_B67;
        case AVCOL_TRC_BT2020_10:
            return HB_COLR_TRA_BT2020_10;
        case AVCOL_TRC_BT2020_12:
            return HB_COLR_TRA_BT2020_12;
        default:
            // ITU BT.601, BT.709, anything else
            return HB_COLR_TRA_BT709;
    }
}

int hb_get_color_matrix(int colorspace, hb_geometry_t geometry)
{
    switch (colorspace)
    {
        case AVCOL_SPC_RGB:
            return HB_COLR_MAT_RGB;
        case AVCOL_SPC_BT709:
            return HB_COLR_MAT_BT709;
        case AVCOL_SPC_FCC:
            return HB_COLR_MAT_FCC;
        case AVCOL_SPC_BT470BG:
            return HB_COLR_MAT_BT470BG;
        case AVCOL_SPC_SMPTE170M:
            return HB_COLR_MAT_SMPTE170M;
        case AVCOL_SPC_SMPTE240M:
            return HB_COLR_MAT_SMPTE240M;
        case AVCOL_SPC_YCGCO:
            return HB_COLR_MAT_YCGCO;
        case AVCOL_SPC_BT2020_NCL:
            return HB_COLR_MAT_BT2020_NCL;
        case AVCOL_SPC_BT2020_CL:
            return HB_COLR_MAT_BT2020_CL;
        case AVCOL_SPC_CHROMA_DERIVED_NCL:
            return HB_COLR_MAT_CD_NCL;
        case AVCOL_SPC_CHROMA_DERIVED_CL:
            return HB_COLR_MAT_CD_CL;
        case AVCOL_SPC_ICTCP:
            return HB_COLR_MAT_ICTCP;
        case AVCOL_SPC_IPT_C2:
            return HB_COLR_MAT_IPT_C2;
        case AVCOL_SPC_YCGCO_RE:
            return HB_COLR_MAT_YCGCO_RE;
        case AVCOL_SPC_YCGCO_RO:
            return HB_COLR_MAT_YCGCO_RO;
        default:
        {
            if ((geometry.width >= 1280 || geometry.height >= 720)||
                (geometry.width >   720 && geometry.height >  576 ))
                // ITU BT.709 HD content
                return HB_COLR_MAT_BT709;
            else
                // ITU BT.601 DVD or SD TV content (PAL)
                // ITU BT.601 DVD or SD TV content (NTSC)
                return HB_COLR_MAT_SMPTE170M;
        }
    }
}

int hb_get_color_range(int color_range)
{
    switch (color_range)
    {
        case AVCOL_RANGE_MPEG:
            return AVCOL_RANGE_MPEG;
        case AVCOL_RANGE_JPEG:
            return AVCOL_RANGE_JPEG;
        default:
            return AVCOL_RANGE_MPEG;
    }
}


int hb_get_chroma_sub_sample(int format, int *h_shift, int *v_shift)
{
    return av_pix_fmt_get_chroma_sub_sample(format, h_shift, v_shift);
}

static int pix_fmt_is_supported(hb_job_t *job, int pix_fmt)
{
    const int title_bit_depth = hb_get_bit_depth(job->title->pix_fmt);
    const int pix_fmt_bit_depth = hb_get_bit_depth(pix_fmt);

    if (pix_fmt_bit_depth > title_bit_depth)
    {
        return 0;
    }

    const AVPixFmtDescriptor *title_desc = av_pix_fmt_desc_get(job->title->pix_fmt);
    const AVPixFmtDescriptor *pix_fmt_desc = av_pix_fmt_desc_get(pix_fmt);

    if (pix_fmt_desc->log2_chroma_w < title_desc->log2_chroma_w ||
        pix_fmt_desc->log2_chroma_h < title_desc->log2_chroma_h)
    {
        return 0;
    }

    // Allow biplanar formats only if hardware decoder is enabled
    const int planes_count = av_pix_fmt_count_planes(pix_fmt);

    if (planes_count == 2)
    {
        if (job->hw_accel == NULL)
        {
            return 0;
        }
    }

    // These filters support only planar pixel formats
    for (int i = 0; i < hb_list_count(job->list_filter); i++)
    {
        hb_filter_object_t *filter = hb_list_item(job->list_filter, i);

        switch (filter->id)
        {
            case HB_FILTER_DETELECINE:
            case HB_FILTER_DECOMB:
            case HB_FILTER_YADIF:
            case HB_FILTER_BWDIF:
            case HB_FILTER_DENOISE:
            case HB_FILTER_NLMEANS:
            case HB_FILTER_CHROMA_SMOOTH:
            case HB_FILTER_LAPSHARP:
            case HB_FILTER_UNSHARP:
            case HB_FILTER_GRAYSCALE:
               if (planes_count == 2 &&
                   job->hw_pix_fmt != AV_PIX_FMT_VIDEOTOOLBOX)
               {
                   return 0;
               }
        }
    }

    return 1;
}

static const enum AVPixelFormat pipeline_pix_fmts[] =
{
    AV_PIX_FMT_P416, AV_PIX_FMT_YUV444P16,
    AV_PIX_FMT_P412, AV_PIX_FMT_YUV444P12,
    AV_PIX_FMT_P410, AV_PIX_FMT_YUV444P10,
    AV_PIX_FMT_NV24, AV_PIX_FMT_YUV444P,
    AV_PIX_FMT_P212, AV_PIX_FMT_YUV422P12,
    AV_PIX_FMT_P210, AV_PIX_FMT_YUV422P10,
    AV_PIX_FMT_NV16, AV_PIX_FMT_YUV422P,
    AV_PIX_FMT_P012, AV_PIX_FMT_YUV420P12,
    AV_PIX_FMT_P010, AV_PIX_FMT_YUV420P10,
    AV_PIX_FMT_NV12, AV_PIX_FMT_YUV420P,
    AV_PIX_FMT_NONE
};

int hb_get_best_pix_fmt(hb_job_t * job)
{
    const int *pix_fmts = pipeline_pix_fmts;

    while (*pix_fmts != AV_PIX_FMT_NONE)
    {
        if (pix_fmt_is_supported(job, *pix_fmts))
        {
            return *pix_fmts;
        }
        pix_fmts++;
    }

    return AV_PIX_FMT_YUV420P;
}
