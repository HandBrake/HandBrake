/* common.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>

#include "hb.h"
#include "x264.h"
#include "lang.h"
#include "common.h"
#include "h264_common.h"
#include "h265_common.h"
#include "encx264.h"
#ifdef USE_QSV
#include "qsv_common.h"
#endif

#ifdef USE_X265
#include "x265.h"
#endif

#ifdef SYS_MINGW
#include <windows.h>
#endif

/**********************************************************************
 * Global variables
 *********************************************************************/
static hb_error_handler_t *error_handler = NULL;

/* Generic IDs for encoders, containers, etc. */
enum
{
    HB_GID_NONE = -1, // encoders must NEVER use it
    HB_GID_VCODEC_H264,
    HB_GID_VCODEC_H265,
    HB_GID_VCODEC_MPEG2,
    HB_GID_VCODEC_MPEG4,
    HB_GID_VCODEC_THEORA,
    HB_GID_VCODEC_VP8,
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
    HB_GID_ACODEC_MP3,
    HB_GID_ACODEC_MP3_PASS,
    HB_GID_ACODEC_TRUEHD_PASS,
    HB_GID_ACODEC_VORBIS,
    HB_GID_MUX_MKV,
    HB_GID_MUX_MP4,
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
};
int hb_audio_rates_count = sizeof(hb_audio_rates) / sizeof(hb_audio_rates[0]);

hb_rate_t *hb_audio_bitrates_first_item = NULL;
hb_rate_t *hb_audio_bitrates_last_item  = NULL;
hb_rate_internal_t hb_audio_bitrates[]  =
{
    // AC3-compatible bitrates
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
    { { "default",                       "auto",          AV_RESAMPLE_DITHER_NONE - 1,      }, NULL, 1, },
    { { "none",                          "none",          AV_RESAMPLE_DITHER_NONE,          }, NULL, 1, },
    { { "rectangular",                   "rectangular",   AV_RESAMPLE_DITHER_RECTANGULAR,   }, NULL, 1, },
    { { "triangular",                    "triangular",    AV_RESAMPLE_DITHER_TRIANGULAR,    }, NULL, 1, },
    { { "triangular with high pass",     "triangular_hp", AV_RESAMPLE_DITHER_TRIANGULAR_HP, }, NULL, 1, },
    { { "triangular with noise shaping", "triangular_ns", AV_RESAMPLE_DITHER_TRIANGULAR_NS, }, NULL, 1, },
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
    int enabled;
    int gid;
} hb_encoder_internal_t;
hb_encoder_t *hb_video_encoders_first_item = NULL;
hb_encoder_t *hb_video_encoders_last_item  = NULL;
hb_encoder_internal_t hb_video_encoders[]  =
{
    // legacy encoders, back to HB 0.9.4 whenever possible (disabled)
    { { "FFmpeg",              "ffmpeg",     NULL,                      HB_VCODEC_FFMPEG_MPEG4, HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_VCODEC_MPEG4,  },
    { { "MPEG-4 (FFmpeg)",     "ffmpeg4",    NULL,                      HB_VCODEC_FFMPEG_MPEG4, HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_VCODEC_MPEG4,  },
    { { "MPEG-2 (FFmpeg)",     "ffmpeg2",    NULL,                      HB_VCODEC_FFMPEG_MPEG2, HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_VCODEC_MPEG2,  },
    { { "VP3 (Theora)",        "libtheora",  NULL,                      HB_VCODEC_THEORA,                       HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_VCODEC_THEORA, },
    // actual encoders
    { { "H.264 (x264)",        "x264",       "H.264 (libx264)",         HB_VCODEC_X264_8BIT,         HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_VCODEC_H264,   },
    { { "H.264 10-bit (x264)", "x264_10bit", "H.264 10-bit (libx264)",  HB_VCODEC_X264_10BIT,   HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_VCODEC_H264,   },
    { { "H.264 (Intel QSV)",   "qsv_h264",   "H.264 (Intel Media SDK)", HB_VCODEC_QSV_H264,     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_VCODEC_H264,   },
    { { "H.265 (x265)",        "x265",       "H.265 (libx265)",         HB_VCODEC_X265_8BIT,      HB_MUX_AV_MP4|HB_MUX_AV_MKV,   }, NULL, 1, HB_GID_VCODEC_H265,   },
    { { "H.265 10-bit (x265)", "x265_10bit", "H.265 10-bit (libx265)",  HB_VCODEC_X265_10BIT,     HB_MUX_AV_MP4|HB_MUX_AV_MKV,   }, NULL, 1, HB_GID_VCODEC_H265,   },
    { { "H.265 12-bit (x265)", "x265_12bit", "H.265 12-bit (libx265)",  HB_VCODEC_X265_12BIT,     HB_MUX_AV_MP4|HB_MUX_AV_MKV,   }, NULL, 1, HB_GID_VCODEC_H265,   },
    { { "H.265 16-bit (x265)", "x265_16bit", "H.265 16-bit (libx265)",  HB_VCODEC_X265_16BIT,     HB_MUX_AV_MP4|HB_MUX_AV_MKV,   }, NULL, 1, HB_GID_VCODEC_H265,   },
    { { "H.265 (Intel QSV)",   "qsv_h265",   "H.265 (Intel Media SDK)", HB_VCODEC_QSV_H265,     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_VCODEC_H265,   },
    { { "MPEG-4",              "mpeg4",      "MPEG-4 (libavcodec)",     HB_VCODEC_FFMPEG_MPEG4, HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_VCODEC_MPEG4,  },
    { { "MPEG-2",              "mpeg2",      "MPEG-2 (libavcodec)",     HB_VCODEC_FFMPEG_MPEG2, HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_VCODEC_MPEG2,  },
    { { "VP8",                 "VP8",        "VP8 (libvpx)",            HB_VCODEC_FFMPEG_VP8,                   HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_VCODEC_VP8,    },
    { { "Theora",              "theora",     "Theora (libtheora)",      HB_VCODEC_THEORA,                       HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_VCODEC_THEORA, },
};
int hb_video_encoders_count = sizeof(hb_video_encoders) / sizeof(hb_video_encoders[0]);
static int hb_video_encoder_is_enabled(int encoder)
{
#ifdef USE_QSV
    if (encoder & HB_VCODEC_QSV_MASK)
    {
        return hb_qsv_video_encoder_is_enabled(encoder);
    }
#endif
    switch (encoder)
    {
        // the following encoders are always enabled
        case HB_VCODEC_THEORA:
        case HB_VCODEC_FFMPEG_MPEG4:
        case HB_VCODEC_FFMPEG_MPEG2:
        case HB_VCODEC_FFMPEG_VP8:
            return 1;

#ifdef USE_X265
        case HB_VCODEC_X265_8BIT:
        case HB_VCODEC_X265_10BIT:
        case HB_VCODEC_X265_12BIT:
        case HB_VCODEC_X265_16BIT:
        {
            const x265_api *api;
            api = x265_api_get(hb_video_encoder_get_depth(encoder));
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
    { { "",                   "dts",        NULL,                          HB_ACODEC_DCA_PASS,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_ACODEC_DTS_PASS,   },
    { { "AAC (faac)",         "faac",       NULL,                          0,                     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_ACODEC_AAC,        },
    { { "AAC (ffmpeg)",       "ffaac",      NULL,                          HB_ACODEC_FFAAC,       HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_ACODEC_AAC,        },
    { { "AC3 (ffmpeg)",       "ffac3",      NULL,                          HB_ACODEC_AC3,         HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_ACODEC_AC3,        },
    { { "MP3 (lame)",         "lame",       NULL,                          HB_ACODEC_LAME,        HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_ACODEC_MP3,        },
    { { "Vorbis (vorbis)",    "libvorbis",  NULL,                          HB_ACODEC_VORBIS,                      HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_ACODEC_VORBIS,     },
    { { "FLAC (ffmpeg)",      "ffflac",     NULL,                          HB_ACODEC_FFFLAC,                      HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_ACODEC_FLAC,       },
    { { "FLAC (24-bit)",      "ffflac24",   NULL,                          HB_ACODEC_FFFLAC24,                    HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_ACODEC_FLAC,       },
    // generic names
    { { "AAC",                "aac",        NULL,                          0,                     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_ACODEC_AAC,        },
    { { "HE-AAC",             "haac",       NULL,                          0,                     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 0, HB_GID_ACODEC_AAC_HE,     },
    // actual encoders
    { { "AAC (CoreAudio)",    "ca_aac",     "AAC (Apple AudioToolbox)",    HB_ACODEC_CA_AAC,      HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_AAC,        },
    { { "HE-AAC (CoreAudio)", "ca_haac",    "HE-AAC (Apple AudioToolbox)", HB_ACODEC_CA_HAAC,     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_AAC_HE,     },
    { { "AAC (FDK)",          "fdk_aac",    "AAC (libfdk_aac)",            HB_ACODEC_FDK_AAC,     HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_AAC,        },
    { { "HE-AAC (FDK)",       "fdk_haac",   "HE-AAC (libfdk_aac)",         HB_ACODEC_FDK_HAAC,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_AAC_HE,     },
    { { "AAC (avcodec)",      "av_aac",     "AAC (libavcodec)",            HB_ACODEC_FFAAC,       HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_AAC,        },
    { { "AAC Passthru",       "copy:aac",   "AAC Passthru",                HB_ACODEC_AAC_PASS,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_AAC_PASS,   },
    { { "AC3",                "ac3",        "AC3 (libavcodec)",            HB_ACODEC_AC3,         HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_AC3,        },
    { { "AC3 Passthru",       "copy:ac3",   "AC3 Passthru",                HB_ACODEC_AC3_PASS,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_AC3_PASS,   },
    { { "E-AC3",              "eac3",       "E-AC3 (libavcodec)",          HB_ACODEC_FFEAC3,                      HB_MUX_AV_MKV,   }, NULL, 1, HB_GID_ACODEC_EAC3,       },
    { { "E-AC3 Passthru",     "copy:eac3",  "E-AC3 Passthru",              HB_ACODEC_EAC3_PASS,                   HB_MUX_AV_MKV,   }, NULL, 1, HB_GID_ACODEC_EAC3_PASS,  },
    { { "TrueHD Passthru",    "copy:truehd","TrueHD Passthru",             HB_ACODEC_TRUEHD_PASS,                 HB_MUX_AV_MKV,   }, NULL, 1, HB_GID_ACODEC_TRUEHD_PASS,},
    { { "DTS Passthru",       "copy:dts",   "DTS Passthru",                HB_ACODEC_DCA_PASS,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_DTS_PASS,   },
    { { "DTS-HD Passthru",    "copy:dtshd", "DTS-HD Passthru",             HB_ACODEC_DCA_HD_PASS, HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_DTSHD_PASS, },
    { { "MP3",                "mp3",        "MP3 (libmp3lame)",            HB_ACODEC_LAME,        HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_MP3,        },
    { { "MP3 Passthru",       "copy:mp3",   "MP3 Passthru",                HB_ACODEC_MP3_PASS,    HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_MP3_PASS,   },
    { { "Vorbis",             "vorbis",     "Vorbis (libvorbis)",          HB_ACODEC_VORBIS,                      HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_VORBIS,     },
    { { "FLAC 16-bit",        "flac16",     "FLAC 16-bit (libavcodec)",    HB_ACODEC_FFFLAC,                      HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_FLAC,       },
    { { "FLAC 24-bit",        "flac24",     "FLAC 24-bit (libavcodec)",    HB_ACODEC_FFFLAC24,                    HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_FLAC,       },
    { { "FLAC Passthru",      "copy:flac",  "FLAC Passthru",               HB_ACODEC_FLAC_PASS,                   HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_FLAC_PASS,  },
    { { "Auto Passthru",      "copy",       "Auto Passthru",               HB_ACODEC_AUTO_PASS,   HB_MUX_MASK_MP4|HB_MUX_MASK_MKV, }, NULL, 1, HB_GID_ACODEC_AUTO_PASS,  },
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

#ifdef USE_LIBAV_AAC
        case HB_ACODEC_FFAAC:
            return avcodec_find_encoder_by_name("aac") != NULL;
#endif

        case HB_ACODEC_FDK_AAC:
        case HB_ACODEC_FDK_HAAC:
            return avcodec_find_encoder_by_name("libfdk_aac") != NULL;

        case HB_ACODEC_AC3:
            return avcodec_find_encoder(AV_CODEC_ID_AC3) != NULL;

        case HB_ACODEC_FFEAC3:
            return avcodec_find_encoder(AV_CODEC_ID_EAC3) != NULL;

        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
            return avcodec_find_encoder(AV_CODEC_ID_FLAC) != NULL;

        // the following encoders are always enabled
        case HB_ACODEC_LAME:
        case HB_ACODEC_VORBIS:
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
    { { "M4V file",            "m4v",    NULL,                     "m4v",  0,             }, NULL, 0, HB_GID_MUX_MP4, },
    { { "MP4 file",            "mp4",    NULL,                     "mp4",  0,             }, NULL, 0, HB_GID_MUX_MP4, },
    { { "MKV file",            "mkv",    NULL,                     "mkv",  0,             }, NULL, 0, HB_GID_MUX_MKV, },
    // actual muxers
    { { "MPEG-4 (avformat)",   "av_mp4", "MPEG-4 (libavformat)",   "mp4",  HB_MUX_AV_MP4, }, NULL, 1, HB_GID_MUX_MP4, },
    { { "MPEG-4 (mp4v2)",      "mp4v2",  "MPEG-4 (libmp4v2)",      "mp4",  HB_MUX_MP4V2,  }, NULL, 1, HB_GID_MUX_MP4, },
    { { "Matroska (avformat)", "av_mkv", "Matroska (libavformat)", "mkv",  HB_MUX_AV_MKV, }, NULL, 1, HB_GID_MUX_MKV, },
    { { "Matroska (libmkv)",   "libmkv", "Matroska (libmkv)",      "mkv",  HB_MUX_LIBMKV, }, NULL, 1, HB_GID_MUX_MKV, },
};
int hb_containers_count = sizeof(hb_containers) / sizeof(hb_containers[0]);
static int hb_container_is_enabled(int format)
{
    switch (format)
    {
        case HB_MUX_AV_MP4:
        case HB_MUX_AV_MKV:
            return 1;

        default:
            return 0;
    }
}

void hb_common_global_init()
{
    static int common_init_done = 0;
    if (common_init_done)
        return;

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
                hb_video_encoder_is_enabled(hb_video_encoders[i].item.codec);
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
            if ((hb_video_encoders[i].item.codec & HB_VCODEC_MASK) &&
                (hb_video_encoder_is_enabled(hb_video_encoders[i].item.codec)))
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
            if ((hb_audio_encoders[i].item.codec & HB_ACODEC_MASK) == 0 &&
                (hb_audio_encoders[i].gid == HB_GID_ACODEC_AAC_HE))
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

int hb_audio_samplerate_get_best(uint32_t codec, int samplerate, int *sr_shift)
{
    int best_samplerate;
    if (samplerate < 32000 && (codec == HB_ACODEC_AC3    ||
                               codec == HB_ACODEC_FFEAC3 ||
                               codec == HB_ACODEC_CA_HAAC))
    {
        // ca_haac can't do samplerates < 32 kHz
        // libav's E-AC-3 encoder can't do samplerates < 32 kHz
        // AC-3 < 32 kHz suffers from poor hardware compatibility
        best_samplerate = 32000;
    }
    else if (samplerate < 16000 && codec == HB_ACODEC_FDK_HAAC)
    {
        // fdk_haac can't do samplerates < 16 kHz
        best_samplerate = 16000;
    }
    else
    {
        best_samplerate                   = hb_audio_rates_first_item->rate;
        const hb_rate_t *audio_samplerate = NULL;
        while ((audio_samplerate = hb_audio_samplerate_get_next(audio_samplerate)) != NULL)
        {
            if (samplerate == audio_samplerate->rate)
            {
                // valid samplerate
                best_samplerate = audio_samplerate->rate;
                break;
            }
            if (samplerate > audio_samplerate->rate)
            {
                // samplerates are sanitized downwards
                best_samplerate = audio_samplerate->rate;
            }
        }
    }
    if (sr_shift != NULL)
    {
        /* sr_shift: 0 -> 48000, 44100, 32000 Hz
         *           1 -> 24000, 22050, 16000 Hz
         *           2 -> 12000, 11025,  8000 Hz
         *
         * also, since samplerates are sanitized downwards:
         *
         * (samplerate < 32000) implies (samplerate <= 24000)
         */
        *sr_shift = ((best_samplerate < 16000) ? 2 :
                     (best_samplerate < 32000) ? 1 : 0);
    }
    return best_samplerate;
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
        return hb_audio_samplerate_get_best(0, i, NULL);
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

    int bitrate, nchannels, sr_shift;
    /* full-bandwidth channels, sr_shift */
    nchannels = (hb_mixdown_get_discrete_channel_count(mixdown) -
                 hb_mixdown_get_low_freq_channel_count(mixdown));
    hb_audio_samplerate_get_best(codec, samplerate, &sr_shift);

    switch (codec)
    {
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
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
    samplerate = hb_audio_samplerate_get_best(codec, samplerate, &sr_shift);

    /* LFE, full-bandwidth channels */
    int lfe_count, nchannels;
    lfe_count = hb_mixdown_get_low_freq_channel_count(mixdown);
    nchannels = hb_mixdown_get_discrete_channel_count(mixdown) - lfe_count;

    switch (codec)
    {
        // Bitrates don't apply to "lossless" audio
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
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

        // Bitrates don't apply to passthrough audio, but may apply if we
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
#ifdef USE_QSV
    if (codec & HB_VCODEC_QSV_MASK)
    {
        return hb_qsv_video_quality_get_limits(codec, low, high, granularity,
                                               direction);
    }
#endif

    switch (codec)
    {
        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X264_10BIT:
#ifdef USE_X265
        case HB_VCODEC_X265_8BIT:
        case HB_VCODEC_X265_10BIT:
        case HB_VCODEC_X265_12BIT:
        case HB_VCODEC_X265_16BIT:
#endif
            *direction   = 1;
            *granularity = 0.1;
            *low         = 0.;
            *high        = 51.;
            break;

        case HB_VCODEC_THEORA:
            *direction   = 0;
            *granularity = 1.;
            *low         = 0.;
            *high        = 63.;
            break;

        case HB_VCODEC_FFMPEG_VP8:
            *direction   = 1;
            *granularity = 1.;
            *low         = 0.;
            *high        = 63.;
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
#ifdef USE_QSV
    if (codec & HB_VCODEC_QSV_MASK)
    {
        return hb_qsv_video_quality_get_name(codec);
    }
#endif

    switch (codec)
    {
        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X264_10BIT:
#ifdef USE_X265
        case HB_VCODEC_X265_8BIT:
        case HB_VCODEC_X265_10BIT:
        case HB_VCODEC_X265_12BIT:
        case HB_VCODEC_X265_16BIT:
#endif
            return "RF";

        case HB_VCODEC_FFMPEG_VP8:
            return "CQ";

        default:
            return "QP";
    }
}

int hb_video_encoder_get_depth(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_X264_10BIT:
        case HB_VCODEC_X265_10BIT:
            return 10;
        case HB_VCODEC_X265_12BIT:
            return 12;
        case HB_VCODEC_X265_16BIT:
            return 16;
        default:
            return 8;
    }
}

const char* const* hb_video_encoder_get_presets(int encoder)
{
#ifdef USE_QSV
    if (encoder & HB_VCODEC_QSV_MASK)
    {
        return hb_qsv_preset_get_names();
    }
#endif

    switch (encoder)
    {
        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X264_10BIT:
            return x264_preset_names;

#ifdef USE_X265
        case HB_VCODEC_X265_8BIT:
        case HB_VCODEC_X265_10BIT:
        case HB_VCODEC_X265_12BIT:
        case HB_VCODEC_X265_16BIT:
            return x265_preset_names;
#endif
        default:
            return NULL;
    }
}

const char* const* hb_video_encoder_get_tunes(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X264_10BIT:
            return x264_tune_names;

#ifdef USE_X265
        case HB_VCODEC_X265_8BIT:
        case HB_VCODEC_X265_10BIT:
        case HB_VCODEC_X265_12BIT:
        case HB_VCODEC_X265_16BIT:
            return x265_tune_names;
#endif
        default:
            return NULL;
    }
}

const char* const* hb_video_encoder_get_profiles(int encoder)
{
#ifdef USE_QSV
    if (encoder & HB_VCODEC_QSV_MASK)
    {
        return hb_qsv_profile_get_names(encoder);
    }
#endif

    switch (encoder)
    {
        case HB_VCODEC_X264_8BIT:
            return hb_h264_profile_names_8bit;
        case HB_VCODEC_X264_10BIT:
            return hb_h264_profile_names_10bit;

        case HB_VCODEC_X265_8BIT:
            return hb_h265_profile_names_8bit;
        case HB_VCODEC_X265_10BIT:
            return hb_h265_profile_names_10bit;
        case HB_VCODEC_X265_12BIT:
            return hb_h265_profile_names_12bit;
        case HB_VCODEC_X265_16BIT:
            return hb_h265_profile_names_16bit;

        default:
            return NULL;
    }
}

const char* const* hb_video_encoder_get_levels(int encoder)
{
#ifdef USE_QSV
    if (encoder & HB_VCODEC_QSV_MASK)
    {
        return hb_qsv_level_get_names(encoder);
    }
#endif

    switch (encoder)
    {
        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X264_10BIT:
            return hb_h264_level_names;

        default:
            return NULL;
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
            *high        = 10.;
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
            return 5.;

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
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
            return 5.;

        case HB_ACODEC_LAME:
            return 2.;

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
    return AV_RESAMPLE_DITHER_TRIANGULAR;
}

int hb_audio_dither_is_supported(uint32_t codec)
{
    // encoder's input sample format must be s16(p)
    switch (codec)
    {
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FDK_AAC:
        case HB_ACODEC_FDK_HAAC:
            return 1;

        default:
            return 0;
    }
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
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
            return (mixdown <= HB_AMIXDOWN_7POINT1);

        case HB_ACODEC_LAME:
            return (mixdown <= HB_AMIXDOWN_DOLBYPLII);

        case HB_ACODEC_CA_AAC:
        case HB_ACODEC_CA_HAAC:
            return ((mixdown <= HB_AMIXDOWN_5POINT1) ||
                    (mixdown == HB_AMIXDOWN_5_2_LFE));

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
        // also, allow Dolby Surrounbd output if the input is already Dolby
        case HB_AMIXDOWN_DOLBY:
        case HB_AMIXDOWN_DOLBYPLII:
            return ((layout & AV_CH_LAYOUT_2_1) == AV_CH_LAYOUT_2_1 ||
                    (layout & AV_CH_LAYOUT_2_2) == AV_CH_LAYOUT_2_2 ||
                    (layout & AV_CH_LAYOUT_QUAD) == AV_CH_LAYOUT_QUAD ||
                    (layout == AV_CH_LAYOUT_STEREO_DOWNMIX &&
                     mixdown == HB_AMIXDOWN_DOLBY));

        // more than 1 channel
        case HB_AMIXDOWN_STEREO:
            return (av_get_channel_layout_nb_channels(layout) > 1);

        // regular stereo (not Dolby)
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
        case HB_ACODEC_FFFLAC:
        case HB_ACODEC_FFFLAC24:
            mixdown = HB_AMIXDOWN_7POINT1;
            break;

        // the (E-)AC-3 encoder defaults to the best mixdown up to 5.1
        case HB_ACODEC_AC3:
        case HB_ACODEC_FFEAC3:
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
    if (!(encoder & HB_VCODEC_MASK))
        goto fail;

    const hb_encoder_t *video_encoder = NULL;
    while ((video_encoder = hb_video_encoder_get_next(video_encoder)) != NULL)
    {
        if (video_encoder->codec == encoder)
        {
            return video_encoder->name;
        }
    }

fail:
    return NULL;
}

const char* hb_video_encoder_get_short_name(int encoder)
{
    if (!(encoder & HB_VCODEC_MASK))
        goto fail;

    const hb_encoder_t *video_encoder = NULL;
    while ((video_encoder = hb_video_encoder_get_next(video_encoder)) != NULL)
    {
        if (video_encoder->codec == encoder)
        {
            return video_encoder->short_name;
        }
    }

fail:
    return NULL;
}

const char* hb_video_encoder_get_long_name(int encoder)
{
    if (!(encoder & HB_VCODEC_MASK))
        goto fail;

    const hb_encoder_t *video_encoder = NULL;
    while ((video_encoder = hb_video_encoder_get_next(video_encoder)) != NULL)
    {
        if (video_encoder->codec == encoder)
        {
            return video_encoder->long_name;
        }
    }

fail:
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
            gid = HB_GID_NONE; // will never match an enabled encoder
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
            if (!(audio->config.out.codec & HB_ACODEC_PASS_FLAG) &&
                !(audio->config.out.codec & HB_ACODEC_MASK))
            {
                hb_log("Auto Passthru: passthru not possible and no valid fallback specified, dropping track %d",
                       audio->config.out.track );
                hb_list_rem(job->list_audio, audio);
                hb_audio_close(&audio);
                continue;
            }
            if (!(audio->config.out.codec & HB_ACODEC_PASS_FLAG))
            {
                if (audio->config.out.codec == job->acodec_fallback)
                {
                    hb_log("Auto Passthru: passthru not possible for track %d, using fallback",
                           audio->config.out.track);
                }
                else
                {
                    hb_log("Auto Passthru: passthru and fallback not possible for track %d, using default encoder",
                           audio->config.out.track);
                }
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
                    hb_audio_samplerate_get_best(audio->config.out.codec,
                                                 audio->config.out.samplerate,
                                                 NULL);
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
    int i = 0;
    const hb_encoder_t *audio_encoder = NULL;
    int out_codec = (copy_mask & in_codec) | HB_ACODEC_PASS_FLAG;
    // sanitize fallback encoder and selected passthru
    // note: invalid fallbacks are caught in hb_autopassthru_apply_settings
    while ((audio_encoder = hb_audio_encoder_get_next(audio_encoder)) != NULL)
    {
        if (audio_encoder->codec == out_codec)
        {
            i++;
            if (!(audio_encoder->muxers & muxer))
                out_codec = 0;
        }
        else if (audio_encoder->codec == fallback)
        {
            i++;
            if (!(audio_encoder->muxers & muxer))
                fallback = hb_audio_encoder_get_default(muxer);
        }
        if (i > 1)
        {
            break;
        }
    }
    return (out_codec & HB_ACODEC_PASS_MASK) ? out_codec : fallback;
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

void hb_limit_rational( int *x, int *y, int num, int den, int limit )
{
    hb_reduce( &num, &den, num, den );
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
        size += end->size;
        end = end->next;
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
        size += end->size;
        end = end->next;
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
        while (end != NULL && end->next != list->tail)
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

/**********************************************************************
 * hb_list_insert
 **********************************************************************
 * Adds an item at the specifiec position in the list, making it bigger
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

static void hb_update_str( char **dst, const char *src )
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

    t->index         = index;
    t->playlist      = -1;
    t->list_audio    = hb_list_init();
    t->list_chapter  = hb_list_init();
    t->list_subtitle = hb_list_init();
    t->list_attachment = hb_list_init();
    t->metadata      = hb_metadata_init();
    strncat(t->path, path, sizeof(t->path) - 1);
    // default to decoding mpeg2
    t->video_id      = 0xE0;
    t->video_codec   = WORK_DECAVCODECV;
    t->video_codec_param = AV_CODEC_ID_MPEG2VIDEO;
    t->angle_count   = 1;
    t->geometry.par.num = 1;
    t->geometry.par.den = 1;

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

    free( t->video_codec_name );
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

    /* Autocrop by default. Gnark gnark */
    memcpy( job->crop, title->crop, 4 * sizeof( int ) );


    hb_geometry_t resultGeo, srcGeo;
    hb_geometry_settings_t uiGeo;

    srcGeo = title->geometry;

    memset(&uiGeo, 0, sizeof(uiGeo));
    memcpy(uiGeo.crop, title->crop, 4 * sizeof( int ));
    uiGeo.geometry.width = srcGeo.width - uiGeo.crop[2] - uiGeo.crop[3];
    uiGeo.geometry.height = srcGeo.height - uiGeo.crop[0] - uiGeo.crop[1];
    uiGeo.mode = HB_ANAMORPHIC_NONE;
    uiGeo.keep = HB_KEEP_DISPLAY_ASPECT;

    hb_set_anamorphic_size2(&srcGeo, &uiGeo, &resultGeo);
    job->width = resultGeo.width;
    job->height = resultGeo.height;
    job->par = resultGeo.par;

    job->vcodec     = HB_VCODEC_FFMPEG_MPEG4;
    job->vquality   = -1.0;
    job->vbitrate   = 1000;
    job->twopass    = 0;
    job->pass_id    = HB_PASS_ENCODE;
    job->vrate      = title->vrate;

    job->mux = HB_MUX_MP4;

    job->list_audio = hb_list_init();
    job->list_subtitle = hb_list_init();
    job->list_filter = hb_list_init();

    job->list_attachment = hb_attachment_list_copy( title->list_attachment );
    job->metadata = hb_metadata_copy( title->metadata );

#ifdef USE_QSV
    job->qsv.enc_info.is_init_done = 0;
    job->qsv.async_depth           = AV_QSV_ASYNC_DEPTH_DEFAULT;
    job->qsv.decode                = !!(title->video_decode_support &
                                        HB_DECODE_SUPPORT_QSV);
#endif
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
        filter_copy->settings = strdup( filter->settings );
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

/**
 * Gets a filter object with the given type
 * @param filter_id The type of filter to get.
 * @returns The requested filter object.
 */
hb_filter_object_t * hb_filter_init( int filter_id )
{
    hb_filter_object_t * filter;
    switch( filter_id )
    {
        case HB_FILTER_DETELECINE:
            filter = &hb_filter_detelecine;
            break;

        case HB_FILTER_DECOMB:
            filter = &hb_filter_decomb;
            break;

        case HB_FILTER_DEINTERLACE:
            filter = &hb_filter_deinterlace;
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

        case HB_FILTER_RENDER_SUB:
            filter = &hb_filter_render_sub;
            break;

        case HB_FILTER_CROP_SCALE:
            filter = &hb_filter_crop_scale;
            break;

        case HB_FILTER_ROTATE:
            filter = &hb_filter_rotate;
            break;

        case HB_FILTER_GRAYSCALE:
            filter = &hb_filter_grayscale;
            break;

#ifdef USE_QSV
        case HB_FILTER_QSV:
            filter = &hb_filter_qsv;
            break;

        case HB_FILTER_QSV_PRE:
            filter = &hb_filter_qsv_pre;
            break;

        case HB_FILTER_QSV_POST:
            filter = &hb_filter_qsv_post;
            break;
#endif

        default:
            filter = NULL;
            break;
    }
    return hb_filter_copy( filter );
}

/**********************************************************************
 * hb_filter_close
 **********************************************************************
 *
 *********************************************************************/
void hb_filter_close( hb_filter_object_t ** _f )
{
    hb_filter_object_t * f = *_f;

    if( f->settings )
        free( f->settings );

    free( f );
    *_f = NULL;
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
 * @param titel to apply.
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
void hb_audio_close( hb_audio_t **audio )
{
    if ( audio && *audio )
    {
        free((*audio)->config.out.name);
        free(*audio);
        *audio = NULL;
    }
}

/**********************************************************************
 * hb_audio_new
 **********************************************************************
 *
 *********************************************************************/
void hb_audio_config_init(hb_audio_config_t * audiocfg)
{
    /* Set read-only paramaters to invalid values */
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

    /* Initalize some sensible defaults */
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

    audio = hb_audio_copy( hb_list_item( title->list_audio, audiocfg->in.track ) );
    if( audio == NULL )
    {
        /* We fail! */
        return 0;
    }

    if( (audiocfg->in.bitrate != -1) && (audiocfg->in.codec != 0xDEADBEEF) )
    {
        /* This most likely means the client didn't call hb_audio_config_init
         * so bail. */
        return 0;
    }

    /* Set the job's "in track" to the value passed in audiocfg.
     * HandBrakeCLI assumes this value is preserved in the jobs
     * audio list, but in.track in the title's audio list is not
     * required to be the same. */
    // "track" in title->list_audio is an index into the source's tracks.
    // "track" in job->list_audio is an index into title->list_audio
    audio->config.in.track = audiocfg->in.track;

    /* Really shouldn't ignore the passed out track, but there is currently no
     * way to handle duplicates or out-of-order track numbers. */
    audio->config.out.track = hb_list_count(job->list_audio) + 1;
    audio->config.out.codec = audiocfg->out.codec;
    if((audiocfg->out.codec & HB_ACODEC_PASS_FLAG) &&
       ((audiocfg->out.codec == HB_ACODEC_AUTO_PASS) ||
        (audiocfg->out.codec & audio->config.in.codec & HB_ACODEC_PASS_MASK)))
    {
        /* Pass-through, copy from input. */
        audio->config.out.samplerate = audio->config.in.samplerate;
        audio->config.out.bitrate = audio->config.in.bitrate;
        audio->config.out.mixdown = HB_AMIXDOWN_NONE;
        audio->config.out.dynamic_range_compression = 0;
        audio->config.out.gain = 0;
        audio->config.out.normalize_mix_level = 0;
        audio->config.out.compression_level = -1;
        audio->config.out.quality = HB_INVALID_AUDIO_QUALITY;
        audio->config.out.dither_method = hb_audio_dither_get_default();
    }
    else
    {
        /* Non pass-through, use what is given. */
        audio->config.out.codec &= ~HB_ACODEC_PASS_FLAG;
        audio->config.out.samplerate = audiocfg->out.samplerate;
        audio->config.out.bitrate = audiocfg->out.bitrate;
        audio->config.out.compression_level = audiocfg->out.compression_level;
        audio->config.out.quality = audiocfg->out.quality;
        audio->config.out.dynamic_range_compression = audiocfg->out.dynamic_range_compression;
        audio->config.out.mixdown = audiocfg->out.mixdown;
        audio->config.out.gain = audiocfg->out.gain;
        audio->config.out.normalize_mix_level = audiocfg->out.normalize_mix_level;
        audio->config.out.dither_method = audiocfg->out.dither_method;
    }
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
        if ( src->extradata )
        {
            subtitle->extradata = malloc( src->extradata_size );
            memcpy( subtitle->extradata, src->extradata, src->extradata_size );
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
void hb_subtitle_close( hb_subtitle_t **sub )
{
    if ( sub && *sub )
    {
        free ((*sub)->extradata);
        free(*sub);
        *sub = NULL;
    }
}

/**********************************************************************
 * hb_subtitle_add
 **********************************************************************
 *
 *********************************************************************/
int hb_subtitle_add_ssa_header(hb_subtitle_t *subtitle, const char *font,
                               int fs, int w, int h)
{
    // Free any pre-existing extradata
    free(subtitle->extradata);

    // SRT subtitles are represented internally as SSA
    // Create an SSA header
    const char * ssa_header =
        "[Script Info]\r\n"
        "ScriptType: v4.00+\r\n"
        "Collisions: Normal\r\n"
        "PlayResX: %d\r\n"
        "PlayResY: %d\r\n"
        "Timer: 100.0\r\n"
        "WrapStyle: 0\r\n"
        "\r\n"
        "[V4+ Styles]\r\n"
        "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\r\n"
        "Style: Default,%s,%d,&H00FFFFFF,&H00FFFFFF,&H000F0F0F,&H000F0F0F,0,0,0,0,100,100,0,0.00,1,2,3,2,20,20,20,0\r\n";

    subtitle->extradata = (uint8_t*)hb_strdup_printf(ssa_header, w, h, font, fs);
    if (subtitle->extradata == NULL)
    {
        hb_error("hb_subtitle_add_ssa_header: malloc failed");
        return 0;
    }
    subtitle->extradata_size = strlen((char*)subtitle->extradata) + 1;

    return 1;
}

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
    subtitle->config = *subtitlecfg;
    subtitle->out_track = hb_list_count(job->list_subtitle) + 1;
    hb_list_add(job->list_subtitle, subtitle);
    return 1;
}

int hb_srt_add( const hb_job_t * job,
                const hb_subtitle_config_t * subtitlecfg,
                const char *lang )
{
    hb_subtitle_t *subtitle;
    iso639_lang_t *language = NULL;

    subtitle = calloc( 1, sizeof( *subtitle ) );
    if (subtitle == NULL)
    {
        hb_error("hb_srt_add: malloc failed");
        return 0;
    }

    subtitle->id = (hb_list_count(job->list_subtitle) << 8) | 0xFF;
    subtitle->format = TEXTSUB;
    subtitle->source = SRTSUB;
    subtitle->codec = WORK_DECSRTSUB;

    language = lang_for_code2(lang);
    if (language == NULL)
    {
        hb_log("hb_srt_add: unknown language code (%s)", lang);
        language = lang_for_code2("und");
    }
    strcpy(subtitle->lang, language->eng_name);
    strcpy(subtitle->iso639_2, language->iso639_2);

    subtitle->config = *subtitlecfg;
    hb_list_add(job->list_subtitle, subtitle);

    return 1;
}

int hb_subtitle_can_force( int source )
{
    return source == VOBSUB || source == PGSSUB;
}

int hb_subtitle_can_burn( int source )
{
    return source == VOBSUB  || source == PGSSUB   || source == SSASUB  ||
           source == SRTSUB  || source == CC608SUB || source == UTF8SUB ||
           source == TX3GSUB;
}

int hb_subtitle_can_pass( int source, int mux )
{
    switch (mux)
    {
        case HB_MUX_AV_MKV:
            switch( source )
            {
                case PGSSUB:
                case VOBSUB:
                case SSASUB:
                case SRTSUB:
                case UTF8SUB:
                case TX3GSUB:
                case CC608SUB:
                case CC708SUB:
                    return 1;

                default:
                    return 0;
            } break;

        case HB_MUX_AV_MP4:
            switch( source )
            {
                case VOBSUB:
                case SSASUB:
                case SRTSUB:
                case UTF8SUB:
                case TX3GSUB:
                case CC608SUB:
                case CC708SUB:
                    return 1;

                default:
                    return 0;
            } break;

        default:
            // Internal error. Should never get here.
            hb_error("internel error.  Bad mux %d\n", mux);
            return 0;
    }
}

int hb_audio_can_apply_drc(uint32_t codec, uint32_t codec_param, int encoder)
{
    if (encoder & HB_ACODEC_PASS_FLAG)
    {
        // can't apply DRC to passthrough audio
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
        if ( src->name )
        {
            metadata->name = strdup(src->name);
        }
        if ( src->artist )
        {
            metadata->artist = strdup(src->artist);
        }
        if ( src->album_artist )
        {
            metadata->album_artist = strdup(src->album_artist);
        }
        if ( src->composer )
        {
            metadata->composer = strdup(src->composer);
        }
        if ( src->release_date )
        {
            metadata->release_date = strdup(src->release_date);
        }
        if ( src->comment )
        {
            metadata->comment = strdup(src->comment);
        }
        if ( src->album )
        {
            metadata->album = strdup(src->album);
        }
        if ( src->genre )
        {
            metadata->genre = strdup(src->genre);
        }
        if ( src->description )
        {
            metadata->description = strdup(src->description);
        }
        if ( src->long_description )
        {
            metadata->long_description = strdup(src->long_description);
        }
        if ( src->list_coverart )
        {
            int ii;
            for ( ii = 0; ii < hb_list_count( src->list_coverart ); ii++ )
            {
                hb_coverart_t *art = hb_list_item( src->list_coverart, ii );
                hb_metadata_add_coverart(
                        metadata, art->data, art->size, art->type );
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

        free( m->name );
        free( m->artist );
        free( m->composer );
        free( m->release_date );
        free( m->comment );
        free( m->album );
        free( m->album_artist );
        free( m->genre );
        free( m->description );
        free( m->long_description );

        if ( m->list_coverart )
        {
            while( ( art = hb_list_item( m->list_coverart, 0 ) ) )
            {
                hb_list_rem( m->list_coverart, art );
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
void hb_metadata_set_name( hb_metadata_t *metadata, const char *name )
{
    if ( metadata )
    {
        hb_update_str( &metadata->name, name );
    }
}

void hb_metadata_set_artist( hb_metadata_t *metadata, const char *artist )
{
    if ( metadata )
    {
        hb_update_str( &metadata->artist, artist );
    }
}

void hb_metadata_set_composer( hb_metadata_t *metadata, const char *composer )
{
    if ( metadata )
    {
        hb_update_str( &metadata->composer, composer );
    }
}

void hb_metadata_set_release_date( hb_metadata_t *metadata, const char *release_date )
{
    if ( metadata )
    {
        hb_update_str( &metadata->release_date, release_date );
    }
}

void hb_metadata_set_comment( hb_metadata_t *metadata, const char *comment )
{
    if ( metadata )
    {
        hb_update_str( &metadata->comment, comment );
    }
}

void hb_metadata_set_genre( hb_metadata_t *metadata, const char *genre )
{
    if ( metadata )
    {
        hb_update_str( &metadata->genre, genre );
    }
}

void hb_metadata_set_album( hb_metadata_t *metadata, const char *album )
{
    if ( metadata )
    {
        hb_update_str( &metadata->album, album );
    }
}

void hb_metadata_set_album_artist( hb_metadata_t *metadata, const char *album_artist )
{
    if ( metadata )
    {
        hb_update_str( &metadata->album_artist, album_artist );
    }
}

void hb_metadata_set_description( hb_metadata_t *metadata, const char *description )
{
    if ( metadata )
    {
        hb_update_str( &metadata->description, description );
    }
}

void hb_metadata_set_long_description( hb_metadata_t *metadata, const char *long_description )
{
    if ( metadata )
    {
        hb_update_str( &metadata->long_description, long_description );
    }
}

void hb_metadata_add_coverart( hb_metadata_t *metadata, const uint8_t *data, int size, int type )
{
    if ( metadata )
    {
        if ( metadata->list_coverart == NULL )
        {
            metadata->list_coverart = hb_list_init();
        }
        hb_coverart_t *art = calloc( 1, sizeof(hb_coverart_t) );
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
            free( art->data );
            free( art );
        }
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
    strncat( str, s2, n );
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
 * Converts an RGB pixel to a YCrCb pixel.
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

const char * hb_subsource_name( int source )
{
    switch (source)
    {
        case VOBSUB:
            return "VOBSUB";
        case SRTSUB:
            return "SRT";
        case CC608SUB:
            return "CC";
        case CC708SUB:
            return "CC";
        case UTF8SUB:
            return "UTF-8";
        case TX3GSUB:
            return "TX3G";
        case SSASUB:
            return "SSA";
        case PGSSUB:
            return "PGS";
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
            p += sprintf( p, "%02x", data[ii] );
            hb_deep_log( level, "    %-50s%20s", line, ascii );
            memset(&ascii[1], '.', 16);
            p = line;
        }
        else if( ( ii & 0x07 ) == 0x07 )
        {
            p += sprintf( p, "%02x  ", data[ii] );
        }
        else
        {
            p += sprintf( p, "%02x ", data[ii] );
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
