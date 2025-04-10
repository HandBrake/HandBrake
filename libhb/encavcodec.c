/* encavcodec.c

   Copyright (c) 2003-2025 HandBrake Team
   Copyright 2022 NVIDIA Corporation
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hb_dict.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/hwaccel.h"
#include "handbrake/h264_common.h"
#include "handbrake/h265_common.h"
#include "handbrake/av1_common.h"
#include "handbrake/nal_units.h"
#include "handbrake/nvenc_common.h"
#include "handbrake/vce_common.h"
#include "handbrake/extradata.h"
#include "handbrake/qsv_common.h"

/*
 * The frame info struct remembers information about each frame across calls
 * to avcodec_encode_video. Since frames are uniquely identified by their
 * frame number, we use this as an index.
 *
 * The size of the array is chosen so that two frames can't use the same
 * slot during the encoder's max frame delay and so that,
 * up to some minimum frame rate, frames are guaranteed
 * to map to * different slots.
 */
#define FRAME_INFO_SIZE 1024
#define FRAME_INFO_MASK (FRAME_INFO_SIZE - 1)

struct hb_work_private_s
{
    hb_job_t           * job;
    AVCodecContext     * context;
    AVPacket           * pkt;
    FILE               * file;

    int                  frameno_in;
    int                  frameno_out;
    hb_buffer_list_t     delay_list;

    int64_t              dts_delay;

#if HB_PROJECT_FEATURE_QSV
    qsv_data_t         qsv_data;
#endif

    struct {
        int64_t          start;
        int64_t          duration;
    } frame_info[FRAME_INFO_SIZE];

    hb_chapter_queue_t * chapter_queue;
};

int  encavcodecInit( hb_work_object_t *, hb_job_t * );
int  encavcodecWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encavcodecClose( hb_work_object_t * );

static int apply_encoder_preset(int vcodec, AVCodecContext *context,
                                AVDictionary **av_opts,
                                const char *preset);
static int apply_encoder_tune(int vcodec, AVDictionary ** av_opts,
                                const char * tune);

static int apply_encoder_options(hb_job_t *job, AVCodecContext *context,
                                 AVDictionary **av_opts);

static int apply_encoder_level(AVCodecContext *context, AVDictionary **av_opts,
                               int vcodec, const char *encoder_level);

hb_work_object_t hb_encavcodec =
{
    WORK_ENCAVCODEC,
    "FFMPEG encoder (libavcodec)",
    encavcodecInit,
    encavcodecWork,
    encavcodecClose
};

static const char * const empty_tune_names[] =
{
    "none", NULL
};

static const char * const vpx_preset_names[] =
{
    "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow", NULL
};

static const char * const vp9_tune_names[] = 
{
    "none", "screen", "film", NULL
};

static const char * const h264_qsv_profile_name[] =
{
    "auto", "high", "main", "baseline", NULL
};

static const char * const h265_qsv_profile_name[] =
{
    "auto", "main", "main10", "mainsp",  NULL
};

static const char * const h26x_nvenc_preset_names[] =
{
    "fastest", "faster", "fast", "medium", "slow", "slower", "slowest", NULL
};

static const char * const ffv1_preset_names[] =
{
    "default", "preservation", NULL
};

static const char * const h264_nvenc_profile_names[] =
{
    "auto", "baseline", "main", "high", NULL  // "high444p" not supported.
};

static const char * const h265_nvenc_profile_names[] =
{
    "auto", "main", NULL
};

static const char * const h265_nvenc_10bit_profile_names[] =
{
    "auto", "main10", NULL
};

static const char * const h26x_mf_preset_name[] =
{
    "default", NULL
};

static const char * const h264_mf_profile_name[] =
{
    "auto", "baseline", "main", "high", NULL
};

static const char * const h265_mf_profile_name[] =
{
    "auto", "main",  NULL
};
static const char * const av1_mf_profile_name[] =
{
    "auto", "main",  NULL
};
static const char * const ffv1_profile_names[] =
{
    "auto", NULL
};

static const char * const hb_ffv1_level_names[] =
{
    "auto", "1", "3", NULL
};

static const int hb_ffv1_level_values[] =
{
    -1,  1,  3,  0
};

static const enum AVPixelFormat standard_pix_fmts[] =
{
    AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat standard_10bit_pix_fmts[] =
{
    AV_PIX_FMT_YUV420P10, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat qsv_pix_formats[] =
{
    AV_PIX_FMT_NV12, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat qsv_10bit_pix_formats[] =
{
    AV_PIX_FMT_P010LE, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat h26x_mf_pix_fmts[] =
{
    AV_PIX_FMT_NV12, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat nvenc_pix_formats_10bit[] =
{
    AV_PIX_FMT_P010, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat nvenc_pix_formats[] =
{
    AV_PIX_FMT_YUV420P, AV_PIX_FMT_NV12, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat vce_pix_formats_10bit[] =
{
     AV_PIX_FMT_P010, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat ffv1_pix_formats[] =
{
    AV_PIX_FMT_YUV444P16, AV_PIX_FMT_YUV444P12, AV_PIX_FMT_YUV444P10, AV_PIX_FMT_YUV444P,
    AV_PIX_FMT_YUV422P16, AV_PIX_FMT_YUV422P12, AV_PIX_FMT_YUV422P10, AV_PIX_FMT_YUV422P,
    AV_PIX_FMT_YUV420P16, AV_PIX_FMT_YUV420P12, AV_PIX_FMT_YUV420P10, AV_PIX_FMT_YUV420P,
    AV_PIX_FMT_NONE
};

int encavcodecInit( hb_work_object_t * w, hb_job_t * job )
{
    int ret = 0;
    char reason[80];
    char * codec_name = NULL;
    const AVCodec * codec = NULL;
    AVCodecContext * context;
    AVRational fps;
    AVDictionary *av_opts = NULL;
    const AVRational *frame_rates = NULL;

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data   = pv;
    pv->job           = job;
    pv->chapter_queue = hb_chapter_queue_init();
    pv->pkt           = av_packet_alloc();

    if (pv->pkt == NULL)
    {
        hb_log("encavcodecInit: av_packet_alloc failed");
        ret = 1;
        goto done;
    }

    hb_buffer_list_clear(&pv->delay_list);

    int clock_min, clock_max, clock;
    hb_video_framerate_get_limits(&clock_min, &clock_max, &clock);

    switch ( w->codec_param )
    {
        case AV_CODEC_ID_MPEG4:
        {
            hb_log("encavcodecInit: MPEG-4 ASP encoder");
            codec_name = "mpeg4";
        } break;
        case AV_CODEC_ID_MPEG2VIDEO:
        {
            hb_log("encavcodecInit: MPEG-2 encoder");
            codec_name = "mpeg2video";
        } break;
        case AV_CODEC_ID_VP8:
        {
            hb_log("encavcodecInit: VP8 encoder");
            codec_name = "libvpx";
        } break;
        case AV_CODEC_ID_VP9:
        {
            hb_log("encavcodecInit: VP9 encoder");
            codec_name = "libvpx-vp9";
        } break;
        case AV_CODEC_ID_H264:
        {
            switch (job->vcodec) {
                case HB_VCODEC_FFMPEG_NVENC_H264:
                    hb_log("encavcodecInit: H.264 (Nvidia NVENC)");
                    codec_name = "h264_nvenc";
                    break;
                case HB_VCODEC_FFMPEG_VCE_H264:
                    hb_log("encavcodecInit: H.264 (AMD VCE)");
                    codec_name = "h264_amf";
                    break;
                case HB_VCODEC_FFMPEG_MF_H264:
                    hb_log("encavcodecInit: H.264 (MediaFoundation)");
                    codec_name = "h264_mf";
                    break;
                case HB_VCODEC_FFMPEG_QSV_H264:
                    hb_log("encavcodecInit: H.264 (Intel Quick Sync Video)");
                    codec_name = "h264_qsv";
                    break;
            }
        }break;
        case AV_CODEC_ID_HEVC:
        {
            switch (job->vcodec) {
                case HB_VCODEC_FFMPEG_NVENC_H265:
                case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
                    hb_log("encavcodecInit: H.265 (Nvidia NVENC)");
                    codec_name = "hevc_nvenc";
                    break;
                case HB_VCODEC_FFMPEG_VCE_H265:
                case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
                    hb_log("encavcodecInit: H.265 (AMD VCE)");
                    codec_name = "hevc_amf";
                    break;
                case HB_VCODEC_FFMPEG_MF_H265:
                    hb_log("encavcodecInit: H.265 (MediaFoundation)");
                    codec_name = "hevc_mf";
                    break;
                case HB_VCODEC_FFMPEG_QSV_H265:
                case HB_VCODEC_FFMPEG_QSV_H265_10BIT:
                    hb_log("encavcodecInit: H.265 (Intel Quick Sync Video)");
                    codec_name = "hevc_qsv";
                    break;
            }
        }break;
        case AV_CODEC_ID_AV1:
        {
            switch (job->vcodec) {
                case HB_VCODEC_FFMPEG_NVENC_AV1:
                case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
                    hb_log("encavcodecInit: AV1 (Nvidia NVENC)");
                    codec_name = "av1_nvenc";
                    break;
                case HB_VCODEC_FFMPEG_VCE_AV1:
                    hb_log("encavcodecInit: AV1 (AMD VCE)");
                    codec_name = "av1_amf";
                    break;
                case HB_VCODEC_FFMPEG_QSV_AV1:
                case HB_VCODEC_FFMPEG_QSV_AV1_10BIT:
                    hb_log("encavcodecInit: AV1 (Intel Quick Sync Video)");
                    codec_name = "av1_qsv";
                    break;
                case HB_VCODEC_FFMPEG_MF_AV1:
                    hb_log("encavcodecInit: AV1 (MediaFoundation)");
                    codec_name = "av1_mf";
                    break;
            }
        }break;
        case AV_CODEC_ID_FFV1:
        {
            switch (job->vcodec) {
                case HB_VCODEC_FFMPEG_FFV1:
                    hb_log("encavcodecInit: FFV1 (libavcodec)");
                    codec_name = "ffv1";
                    break;
            }
        }break;
    }

    if (codec_name == NULL)
    {
        // Catch all when the switch above fails
        hb_log( "encavcodecInit: Unable to determine codec_name "
                "from hb_work_object_t.codec_param=%d and "
                "hb_job_t.vcodec=%x", w->codec_param,
                job->vcodec );
        ret = 1;
        goto done;
    }

    codec = avcodec_find_encoder_by_name(codec_name);
    if( !codec )
    {
        hb_log( "encavcodecInit: avcodec_find_encoder_by_name(%s) "
                "failed", codec_name );
        ret = 1;
        goto done;
    }

    context = avcodec_alloc_context3(codec);

    // Set things in context that we will allow the user to
    // override with advanced settings.
    fps.den = job->vrate.den;
    fps.num = job->vrate.num;

    // If the fps.num is the internal clock rate, there's a good chance
    // this is a standard rate that we have in our hb_video_rates table.
    // Because of rounding errors and approximations made while
    // measuring framerate, the actual value may not be exact.  So
    // we look for rates that are "close" and make an adjustment
    // to fps.den.
    if (fps.num == clock)
    {
        const hb_rate_t *video_framerate = NULL;
        while ((video_framerate = hb_video_framerate_get_next(video_framerate)) != NULL)
        {
            if (abs(fps.den - video_framerate->rate) < 10)
            {
                fps.den = video_framerate->rate;
                break;
            }
        }
    }
    hb_reduce(&fps.den, &fps.num, fps.den, fps.num);

    // Check that the framerate is supported.  If not, pick the closest.
    // The mpeg2 codec only supports a specific list of frame rates.
    if (avcodec_get_supported_config(context, NULL, AV_CODEC_CONFIG_FRAME_RATE,
                                     0, (const void **)&frame_rates, NULL) == 0 && frame_rates)
    {
        AVRational supported_fps;
        supported_fps = frame_rates[av_find_nearest_q_idx(fps, frame_rates)];
        if (supported_fps.num != fps.num || supported_fps.den != fps.den)
        {
            hb_log( "encavcodec: framerate %d / %d is not supported. Using %d / %d.",
                    fps.num, fps.den, supported_fps.num, supported_fps.den );
            fps = supported_fps;
        }
    }
    else if ((fps.num & ~0xFFFF) || (fps.den & ~0xFFFF))
    {
        // This may only be required for mpeg4 video. But since
        // our only supported options are mpeg2 and mpeg4, there is
        // no need to check codec type.
        hb_log( "encavcodec: truncating framerate %d / %d",
                fps.num, fps.den );
        while ((fps.num & ~0xFFFF) || (fps.den & ~0xFFFF))
        {
            fps.num >>= 1;
            fps.den >>= 1;
        }
    }

    context->time_base.den = fps.num;
    context->time_base.num = fps.den;
    context->framerate     = fps;
    context->gop_size  = ((double)job->orig_vrate.num / job->orig_vrate.den +
                                  0.5) * 10;

    if (apply_encoder_level(context, &av_opts, job->vcodec, job->encoder_level))
    {
        av_free(context);
        ret = 1;
        goto done;
    }

    if (apply_encoder_preset(job->vcodec, context, &av_opts, job->encoder_preset))
    {
        av_free( context );
        ret = 1;
        goto done;
    }

    if (apply_encoder_tune(job->vcodec, &av_opts, job->encoder_tune))
    {
        av_free(context);
        ret = 1;
        goto done;
    }

    if (apply_encoder_options(job, context, &av_opts))
    {
        av_free( context );
        ret = 1;
        goto done;
    }

#if HB_PROJECT_FEATURE_QSV
    if (hb_qsv_is_ffmpeg_supported_codec(job->vcodec))
    {
        hb_qsv_apply_encoder_options(&pv->qsv_data, job, &av_opts);
    }
#endif

    // Now set the things in context that we don't want to allow
    // the user to override.
    if (job->vquality <= HB_INVALID_VIDEO_QUALITY)
    {
        /* Average bitrate */
        context->bit_rate = 1000 * job->vbitrate;
        // ffmpeg's mpeg2 encoder requires that the bit_rate_tolerance be >=
        // bitrate * fps
        context->bit_rate_tolerance = context->bit_rate * av_q2d(fps) + 1;

        if ( job->vcodec == HB_VCODEC_FFMPEG_NVENC_H264 || job->vcodec == HB_VCODEC_FFMPEG_NVENC_H265 || job->vcodec == HB_VCODEC_FFMPEG_NVENC_H265_10BIT
            || job->vcodec == HB_VCODEC_FFMPEG_NVENC_AV1 || job->vcodec == HB_VCODEC_FFMPEG_NVENC_AV1_10BIT) {
            av_dict_set( &av_opts, "rc", "vbr", 0 );
            hb_log( "encavcodec: encoding at rc=vbr, Bitrate %d", job->vbitrate );
        }

#if HB_PROJECT_FEATURE_QSV
        if (hb_qsv_is_ffmpeg_supported_codec(job->vcodec))
        {
            if (pv->qsv_data.param.rc.lookahead)
            {
                // introduced in API 1.7
                av_dict_set( &av_opts, "look_ahead", "1", 0 );
            }

            if (job->vbitrate == pv->qsv_data.param.rc.vbv_max_bitrate)
            {
                char maxrate[7];
                snprintf(maxrate, 7, "%d", context->bit_rate);
                av_dict_set( &av_opts, "maxrate", maxrate, 0 );
            }
        }
#endif

        if ((job->vcodec == HB_VCODEC_FFMPEG_VCE_H264)
            || (job->vcodec == HB_VCODEC_FFMPEG_VCE_H265)
            || (job->vcodec == HB_VCODEC_FFMPEG_VCE_H265_10BIT)
            || (job->vcodec == HB_VCODEC_FFMPEG_VCE_AV1))
        {
            av_dict_set( &av_opts, "rc", "vbr_peak", 0 );

            // since we do not have scene change detection, set a
            // relatively short gop size to help avoid stale references
            context->gop_size = (int)(FFMIN(av_q2d(fps) * 2, 120));

            //Work around an ffmpeg issue mentioned in issue #3447
            if (job->vcodec == HB_VCODEC_FFMPEG_VCE_H265 || job->vcodec == HB_VCODEC_FFMPEG_VCE_H265_10BIT)
            {
               av_dict_set( &av_opts, "qmin",  "0", 0 );
               av_dict_set( &av_opts, "qmax", "51", 0 );
            }
            hb_log( "encavcodec: encoding at rc=vbr_peak Bitrate %d", job->vbitrate );
        }

        if (job->vcodec == HB_VCODEC_FFMPEG_MF_H264 ||
            job->vcodec == HB_VCODEC_FFMPEG_MF_H265 ||
            job->vcodec == HB_VCODEC_FFMPEG_MF_AV1) {
            av_dict_set(&av_opts, "rate_control", "u_vbr", 0); // options are cbr, pc_vbr, u_vbr, ld_vbr, g_vbr, gld_vbr
        }
    }
    else
    {
        /* Constant quantizer */

        //Set constant quality for libvpx
        if ( w->codec_param == AV_CODEC_ID_VP8 ||
             w->codec_param == AV_CODEC_ID_VP9 )
        {
            if (w->codec_param == AV_CODEC_ID_VP9 && job->vquality == 0)
            {
                av_dict_set( &av_opts, "lossless", "1", 0 );
            }
            else
            {
                // These settings produce better image quality than
                // what was previously used
                context->flags |= AV_CODEC_FLAG_QSCALE;
                context->global_quality = FF_QP2LAMBDA * job->vquality + 0.5;

                char quality[7];
                snprintf(quality, 7, "%.2f", job->vquality);
                av_dict_set( &av_opts, "crf", quality, 0 );
            }

            if (w->codec_param == AV_CODEC_ID_VP8)
            {
                //This value was chosen to make the bitrate high enough
                //for libvpx to "turn off" the maximum bitrate feature
                //that is normally applied to constant quality.
                context->bit_rate = (int64_t)job->width * job->height *
                                    fps.num / fps.den;
            }
            hb_log( "encavcodec: encoding at CQ %.2f", job->vquality );
        }
        //Set constant quality for nvenc
        else if ( job->vcodec == HB_VCODEC_FFMPEG_NVENC_H264 ||
                  job->vcodec == HB_VCODEC_FFMPEG_NVENC_H265 ||
                  job->vcodec == HB_VCODEC_FFMPEG_NVENC_H265_10BIT ||
                  job->vcodec == HB_VCODEC_FFMPEG_NVENC_AV1 ||
                  job->vcodec == HB_VCODEC_FFMPEG_NVENC_AV1_10BIT)
        {
            char qualityI[7];
            char quality[7];
            char qualityB[7];

            double adjustedQualityI = job->vquality - 2;
            double adjustedQualityB = job->vquality + 2;
            if (adjustedQualityB > 51) {
                adjustedQualityB = 51;
            }

            if (adjustedQualityI < 0){
                adjustedQualityI = 0;
            }

            snprintf(quality, 7, "%.2f", job->vquality);
            snprintf(qualityI, 7, "%.2f", adjustedQualityI);
            snprintf(qualityB, 7, "%.2f", adjustedQualityB);

            context->bit_rate = 0;

            av_dict_set( &av_opts, "rc", "vbr", 0 );
            av_dict_set( &av_opts, "cq", quality, 0 );

            // further Advanced Quality Settings in Constant Quality Mode
            av_dict_set( &av_opts, "init_qpP", quality, 0 );
            av_dict_set( &av_opts, "init_qpB", qualityB, 0 );
            av_dict_set( &av_opts, "init_qpI", qualityI, 0 );
            hb_log( "encavcodec: encoding at rc=vbr, %.2f", job->vquality );
        }
#if HB_PROJECT_FEATURE_QSV
        else if (hb_qsv_is_ffmpeg_supported_codec(job->vcodec))
        {
            context->bit_rate = 0;
            if (pv->qsv_data.param.rc.icq)
            {
                char global_quality[7];
                int upper_limit = 51;
                snprintf(global_quality, 7, "%d", HB_QSV_CLIP3(1, upper_limit, (int)job->vquality));
                av_dict_set(&av_opts, "global_quality", global_quality, 0);
                hb_log("encavcodec: encoding with brc ICQ %s", global_quality);
            }
        }
#endif
        else if ( job->vcodec == HB_VCODEC_FFMPEG_VCE_H264 ||
                  job->vcodec == HB_VCODEC_FFMPEG_VCE_H265 ||
                  job->vcodec == HB_VCODEC_FFMPEG_VCE_H265_10BIT ||
                  job->vcodec == HB_VCODEC_FFMPEG_VCE_AV1 )
        {
            // since we do not have scene change detection, set a
            // relatively short gop size to help avoid stale references
            context->gop_size = (int)(FFMIN(av_q2d(fps) * 2, 120));

            char   quality[7];
            char   qualityP[7];
            char   qualityB[7];
            int    maxQuality = 51;
            double qualityOffsetThreshold = 8;
            double qualityOffsetP = 2;
            double qualityOffsetB;
            double adjustedQualityP;
            double adjustedQualityB;

            if (job->vcodec == HB_VCODEC_FFMPEG_VCE_AV1)
            {
                maxQuality = 255;
                qualityOffsetThreshold = 32;
                qualityOffsetP = 8;
            }

            if (job->vquality <= qualityOffsetThreshold)
            {
                qualityOffsetP = job->vquality / qualityOffsetThreshold * qualityOffsetP;
            }
            qualityOffsetB = qualityOffsetP * 2;

            adjustedQualityP = job->vquality + qualityOffsetP;
            adjustedQualityB = job->vquality + qualityOffsetB;
            if (adjustedQualityP > maxQuality)
            {
                adjustedQualityP = maxQuality;
            }
            if (adjustedQualityB > maxQuality)
            {
                adjustedQualityB = maxQuality;
            }

            snprintf(quality, 7, "%.2f", job->vquality);
            snprintf(qualityP, 7, "%.2f", adjustedQualityP);
            snprintf(qualityB, 7, "%.2f", adjustedQualityB);

            av_dict_set( &av_opts, "rc", "cqp", 0 );

            av_dict_set( &av_opts, "qp_i", quality, 0 );
            av_dict_set( &av_opts, "qp_p", qualityP, 0 );
            // H.265 encoders do not support B frames
            if (job->vcodec != HB_VCODEC_FFMPEG_VCE_H265 &&
                job->vcodec != HB_VCODEC_FFMPEG_VCE_H265_10BIT)
            {
                av_dict_set( &av_opts, "qp_b", qualityB, 0 );
            }

            hb_log( "encavcodec: encoding at CQ %.2f", job->vquality );
            hb_log( "encavcodec: QP (I)   %.2f", job->vquality );
            hb_log( "encavcodec: QP (P)   %.2f", adjustedQualityP );
            if (job->vcodec != HB_VCODEC_FFMPEG_VCE_H265 &&
                job->vcodec != HB_VCODEC_FFMPEG_VCE_H265_10BIT)
            {
                hb_log( "encavcodec: QP (B)   %.2f", adjustedQualityB );
            }
            hb_log( "encavcodec: GOP Size %d", context->gop_size );
        }
        else if (job->vcodec == HB_VCODEC_FFMPEG_MF_H264 ||
                 job->vcodec == HB_VCODEC_FFMPEG_MF_H265 ||
                 job->vcodec == HB_VCODEC_FFMPEG_MF_AV1)
        {
            char quality[7];
            snprintf(quality, 7, "%d", (int)job->vquality);
            av_dict_set(&av_opts, "rate_control", "quality", 0);
            av_dict_set(&av_opts, "quality", quality, 0);
        }
        else
        {
            // These settings produce better image quality than
            // what was previously used
            context->flags |= AV_CODEC_FLAG_QSCALE;
            context->global_quality = FF_QP2LAMBDA * job->vquality + 0.5;

            hb_log( "encavcodec: encoding at constant quantizer %d",
                    context->global_quality );
        }
    }
    context->width     = job->width;
    context->height    = job->height;

    if (hb_hwaccel_is_full_hardware_pipeline_enabled(pv->job))
    {
        context->hw_device_ctx = av_buffer_ref(pv->job->hw_device_ctx);
#if HB_PROJECT_FEATURE_QSV
        if (!hb_qsv_is_ffmpeg_supported_codec(job->vcodec))
#endif
        {
            hb_hwaccel_hwframes_ctx_init(context, job);
        }
        context->pix_fmt = job->hw_pix_fmt;
    }
    else
    {
#if HB_PROJECT_FEATURE_QSV
        if (hb_qsv_is_ffmpeg_supported_codec(job->vcodec) && !job->hw_device_ctx)
        {
            hb_qsv_device_init(job, &job->hw_device_ctx);
            context->hw_device_ctx = av_buffer_ref(job->hw_device_ctx);
        }
#endif
        context->pix_fmt = job->output_pix_fmt;
    }

    context->sample_aspect_ratio.num = job->par.num;
    context->sample_aspect_ratio.den = job->par.den;
    if (job->vcodec == HB_VCODEC_FFMPEG_MPEG4)
    {
        // MPEG-4 Part 2 stores the PAR num/den as unsigned 8-bit fields,
        // and libavcodec's encoder fails to initialize if we don't
        // reduce it to fit 8-bits.
        hb_limit_rational(&context->sample_aspect_ratio.num,
                          &context->sample_aspect_ratio.den,
                           context->sample_aspect_ratio.num,
                           context->sample_aspect_ratio.den, 255);
    }

    hb_log( "encavcodec: encoding with stored aspect %d/%d",
            job->par.num, job->par.den );

    // set colorimetry
    context->color_primaries = hb_output_color_prim(job);
    context->color_trc       = hb_output_color_transfer(job);
    context->colorspace      = hb_output_color_matrix(job);
    context->color_range     = job->color_range;
    context->chroma_sample_location = job->chroma_location;

    if (!job->inline_parameter_sets)
    {
        context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    if( job->grayscale )
    {
        context->flags |= AV_CODEC_FLAG_GRAY;
    }

    if (job->vcodec == HB_VCODEC_FFMPEG_VCE_H264)
    {
        context->profile = AV_PROFILE_UNKNOWN;
        if (job->encoder_profile != NULL && *job->encoder_profile)
        {
            if (!strcasecmp(job->encoder_profile, "baseline"))
                context->profile = AV_PROFILE_H264_BASELINE;
            else if (!strcasecmp(job->encoder_profile, "main"))
                 context->profile = AV_PROFILE_H264_MAIN;
            else if (!strcasecmp(job->encoder_profile, "high"))
                context->profile = AV_PROFILE_H264_HIGH;
        }
        av_dict_set(&av_opts, "forced_idr", "1", 0);
    }
    else if (job->vcodec == HB_VCODEC_FFMPEG_VCE_H265 || job->vcodec == HB_VCODEC_FFMPEG_VCE_H265_10BIT)
    {
        context->profile = AV_PROFILE_UNKNOWN;
        if (job->encoder_profile != NULL && *job->encoder_profile)
        {
            if (!strcasecmp(job->encoder_profile, "main")) {
                 context->profile = AV_PROFILE_HEVC_MAIN;
            }

            if (!strcasecmp(job->encoder_profile, "main10")) {
                 context->profile = AV_PROFILE_HEVC_MAIN_10;
            }
        }

        av_dict_set(&av_opts, "forced_idr", "1", 0);
        // Make VCE h.265 encoder emit an IDR for every GOP
        av_dict_set(&av_opts, "gops_per_idr", "1", 0);
    }
    else if (job->vcodec == HB_VCODEC_FFMPEG_VCE_AV1)
    {
        context->profile = AV_PROFILE_UNKNOWN;
        if (job->encoder_profile != NULL && *job->encoder_profile)
        {
            if (!strcasecmp(job->encoder_profile, "main"))
                 context->profile = AV_PROFILE_AV1_MAIN;
        }
        av_dict_set(&av_opts, "forced_idr", "1", 0);
    }
    else if (job->vcodec == HB_VCODEC_FFMPEG_NVENC_H264 ||
             job->vcodec == HB_VCODEC_FFMPEG_NVENC_H265 ||
             job->vcodec == HB_VCODEC_FFMPEG_NVENC_H265_10BIT ||
             job->vcodec == HB_VCODEC_FFMPEG_NVENC_AV1 ||
             job->vcodec == HB_VCODEC_FFMPEG_NVENC_AV1_10BIT)
    {
        // Force IDR frames when we force a new keyframe for chapters
        av_dict_set( &av_opts, "forced-idr", "1", 0 );

        if (job->encoder_profile != NULL && *job->encoder_profile)
        {
            if (!strcasecmp(job->encoder_profile, "baseline"))
                av_dict_set(&av_opts, "profile", "baseline", 0);
            else if (!strcasecmp(job->encoder_profile, "main"))
                av_dict_set(&av_opts, "profile", "main", 0);
            else if (!strcasecmp(job->encoder_profile, "main10"))
                av_dict_set(&av_opts, "profile", "main10", 0);
            else if (!strcasecmp(job->encoder_profile, "high"))
                av_dict_set(&av_opts, "profile", "high", 0);
        }

        // Disable unhandled SEI
        // These might be present in the source video
        // and passed through in side_data, but we
        // don't want to automatically preserve them
        av_dict_set(&av_opts, "a53cc", "0", 0);
        av_dict_set(&av_opts, "s12m_tc", "0", 0);
    }
    else if (job->vcodec == HB_VCODEC_FFMPEG_FFV1)
    {
        int slices[] = {4, 6, 9, 12, 16, 24, 30};
        context->slices = hb_get_cpu_count();

        int slice_index = 0;
        for (int i = 0; i < sizeof(slices) / sizeof(int); i++)
        {
            if (context->slices >= slices[i])
            {
                slice_index = i;
            }
        }
        context->slices = slices[slice_index];
    }
    else if (job->vcodec == HB_VCODEC_FFMPEG_MF_H264 ||
             job->vcodec == HB_VCODEC_FFMPEG_MF_H265 ||
             job->vcodec == HB_VCODEC_FFMPEG_MF_AV1)
    {
        if (job->vcodec == HB_VCODEC_FFMPEG_MF_H264)
        {
            context->profile = AV_PROFILE_UNKNOWN;
            if (job->encoder_profile != NULL && *job->encoder_profile)
            {
                if (!strcasecmp(job->encoder_profile, "baseline"))
                    context->profile = AV_PROFILE_H264_BASELINE;
                else if (!strcasecmp(job->encoder_profile, "main"))
                    context->profile = AV_PROFILE_H264_MAIN;
                else if (!strcasecmp(job->encoder_profile, "high"))
                    context->profile = AV_PROFILE_H264_HIGH;
            }
        }
        else if (job->vcodec == HB_VCODEC_FFMPEG_MF_H265)
        {
            // Qualcomm's HEVC encoder does support b-frames. Some chipsets
            // support setting this to either 1 or 2, while others only support
            // setting it to 1.
            context->max_b_frames = 1;
        }
        av_dict_set(&av_opts, "hw_encoding", "1", 0);
        if (!av_dict_get(av_opts, "scenario", NULL, 0))
        {
            av_dict_set(&av_opts, "scenario", "archive", 0);
        }
    }

    if( job->pass_id == HB_PASS_ENCODE_ANALYSIS ||
        job->pass_id == HB_PASS_ENCODE_FINAL )
    {
        char * filename = hb_get_temporary_filename("ffmpeg.log");

        if( job->pass_id == HB_PASS_ENCODE_ANALYSIS )
        {
            pv->file = hb_fopen(filename, "wb");
            if (!pv->file)
            {
                if (strerror_r(errno, reason, 79) != 0)
                    strcpy(reason, "unknown -- strerror_r() failed");

                hb_error("encavcodecInit: Failed to open %s (reason: %s)", filename, reason);
                free(filename);
                ret = 1;
                goto done;
            }
            context->flags |= AV_CODEC_FLAG_PASS1;
        }
        else
        {
            int    size;
            char * log;

            pv->file = hb_fopen(filename, "rb");
            if (!pv->file) {
                if (strerror_r(errno, reason, 79) != 0)
                    strcpy(reason, "unknown -- strerror_r() failed");

                hb_error("encavcodecInit: Failed to open %s (reason: %s)", filename, reason);
                free(filename);
                ret = 1;
                goto done;
            }
            fseek( pv->file, 0, SEEK_END );
            size = ftell( pv->file );
            fseek( pv->file, 0, SEEK_SET );
            log = malloc( size + 1 );
            log[size] = '\0';
            if (size > 0 &&
                fread( log, size, 1, pv->file ) < size)
            {
                if (ferror(pv->file))
                {
                    if (strerror_r(errno, reason, 79) != 0)
                        strcpy(reason, "unknown -- strerror_r() failed");

                    hb_error( "encavcodecInit: Failed to read %s (reason: %s)" , filename, reason);
                    free(filename);
                    ret = 1;
                    fclose( pv->file );
                    pv->file = NULL;
                    goto done;
                }
            }
            fclose( pv->file );
            pv->file = NULL;

            context->flags    |= AV_CODEC_FLAG_PASS2;
            context->stats_in  = log;
        }
        free(filename);
    }

#if HB_PROJECT_FEATURE_QSV
    if (hb_hwaccel_is_full_hardware_pipeline_enabled(pv->job) &&
            hb_qsv_decode_is_enabled(job))
    {
        pv->context = context;
        pv->qsv_data.codec = codec;
        pv->qsv_data.av_opts = av_opts;
        return 0;
    }
#endif

    if (hb_avcodec_open(context, codec, &av_opts, HB_FFMPEG_THREADS_AUTO))
    {
        hb_log( "encavcodecInit: avcodec_open failed" );
        ret = 1;
        goto done;
    }

    /*
     * Reload colorimetry settings in case custom
     * values were set in the encoder_options string.
     */
    job->color_prim_override     = context->color_primaries;
    job->color_transfer_override = context->color_trc;
    job->color_matrix_override   = context->colorspace;

    if (job->pass_id == HB_PASS_ENCODE_ANALYSIS &&
        context->stats_out != NULL)
    {
        // Some encoders may write stats during init in avcodec_open
        fprintf(pv->file, "%s", context->stats_out);
    }

    // avcodec_open populates the opts dictionary with the
    // things it didn't recognize.
    AVDictionaryEntry *t = NULL;
    while( ( t = av_dict_get( av_opts, "", t, AV_DICT_IGNORE_SUFFIX ) ) )
    {
        hb_log( "encavcodecInit: Unknown avcodec option %s", t->key );
    }

    pv->context = context;

    job->areBframes = 0;
    if (context->has_b_frames > 0)
    {
        job->areBframes = context->has_b_frames;
    }

    if (context->extradata != NULL)
    {
        hb_set_extradata(w->extradata, context->extradata, context->extradata_size);
    }

done:
    av_dict_free(&av_opts);
    return ret;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void encavcodecClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if (pv == NULL)
    {
        return;
    }
    av_packet_free(&pv->pkt);
    hb_chapter_queue_close(&pv->chapter_queue);
    if( pv->context )
    {
        hb_deep_log( 2, "encavcodec: closing libavcodec" );
        if( pv->context->codec ) {
            avcodec_flush_buffers( pv->context );
        }
        hb_avcodec_free_context(&pv->context);
    }
    if( pv->file )
    {
        fclose( pv->file );
    }
    free( pv );
    w->private_data = NULL;
}

/*
 * see comments in definition of 'frame_info' in pv struct for description
 * of what these routines are doing.
 */
static void save_frame_info( hb_work_private_t * pv, hb_buffer_t * in )
{
    int i = pv->frameno_in & FRAME_INFO_MASK;
    pv->frame_info[i].start = in->s.start;
    pv->frame_info[i].duration = in->s.stop - in->s.start;
}

static int64_t get_frame_start( hb_work_private_t * pv, int64_t frameno )
{
    int i = frameno & FRAME_INFO_MASK;
    return pv->frame_info[i].start;
}

static int64_t get_frame_duration( hb_work_private_t * pv, int64_t frameno )
{
    int i = frameno & FRAME_INFO_MASK;
    return pv->frame_info[i].duration;
}

static void compute_dts_offset( hb_work_private_t * pv, hb_buffer_t * buf )
{
    if ( pv->job->areBframes )
    {
        if ( ( pv->frameno_in ) == pv->job->areBframes )
        {
            pv->dts_delay = buf->s.start;
            pv->job->init_delay = pv->dts_delay;
        }
    }
}

// Generate DTS by rearranging PTS in this sequence:
// pts0 - delay, pts1 - delay, pts2 - delay, pts1, pts2, pts3...
//
// Where pts0 - ptsN are in decoded monotonically increasing presentation
// order and delay == pts1 (1 being the number of frames the decoder must
// delay before it has sufficient information to decode). The number of
// frames to delay is set by job->areBframes, so it is configurable.
// This guarantees that DTS <= PTS for any frame.
//
// This is similar to how x264 generates DTS
static hb_buffer_t * process_delay_list( hb_work_private_t * pv, hb_buffer_t * buf )
{
    if (pv->job->areBframes)
    {
        // Has dts_delay been set yet?
        hb_buffer_list_append(&pv->delay_list, buf);
        if (pv->frameno_in <= pv->job->areBframes)
        {
            // dts_delay not yet set.  queue up buffers till it is set.
            return NULL;
        }

        // We have dts_delay.  Apply it to any queued buffers renderOffset
        // and return all queued buffers.
        buf = hb_buffer_list_head(&pv->delay_list);
        while (buf != NULL)
        {
            // Use the cached frame info to get the start time of Nth frame
            // Note that start Nth frame != start time this buffer since the
            // output buffers have rearranged start times.
            if (pv->frameno_out < pv->job->areBframes)
            {
                int64_t start = get_frame_start( pv, pv->frameno_out );
                buf->s.renderOffset = start - pv->dts_delay;
            }
            else
            {
                buf->s.renderOffset = get_frame_start(pv,
                                        pv->frameno_out - pv->job->areBframes);
            }
            buf = buf->next;
            pv->frameno_out++;
        }
        buf = hb_buffer_list_clear(&pv->delay_list);
        return buf;
    }
    else if (buf != NULL)
    {
        buf->s.renderOffset = buf->s.start;
        return buf;
    }
    return NULL;
}

static uint8_t convert_pict_type(const AVPacket *pkt, uint16_t *sflags)
{
    const uint8_t *sd = av_packet_get_side_data(pkt, AV_PKT_DATA_QUALITY_STATS, NULL);

    enum AVPictureType pict_type = sd ? sd[4] : AV_PICTURE_TYPE_NONE;

    uint16_t flags = HB_FLAG_FRAMETYPE_REF;
    uint8_t retval = 0;

    switch (pict_type)
    {
        case AV_PICTURE_TYPE_B:
            retval = HB_FRAME_B;
            break;

        case AV_PICTURE_TYPE_S:
        case AV_PICTURE_TYPE_P:
        case AV_PICTURE_TYPE_SP:
            retval = HB_FRAME_P;
            break;

        case AV_PICTURE_TYPE_BI:
        case AV_PICTURE_TYPE_SI:
        case AV_PICTURE_TYPE_I:
        default:
            retval = HB_FRAME_I;
            break;
    }

    if (pkt->flags & AV_PKT_FLAG_KEY)
    {
        flags |= HB_FLAG_FRAMETYPE_KEY;
    }

    if (pkt->flags & AV_PKT_FLAG_DISPOSABLE)
    {
        flags &= ~HB_FLAG_FRAMETYPE_REF;
    }

    *sflags = flags;
    return retval;
}

static void get_packets( hb_work_object_t * w, hb_buffer_list_t * list )
{
    hb_work_private_t * pv = w->private_data;

    while (1)
    {
        int           ret;
        hb_buffer_t * out;

        ret = avcodec_receive_packet(pv->context, pv->pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            break;
        }
        if (ret < 0)
        {
            hb_log("encavcodec: avcodec_receive_packet failed");
        }

        out = hb_buffer_init(pv->pkt->size);
        memcpy(out->data, pv->pkt->data, out->size);

        int64_t frameno = pv->pkt->pts;
        out->size       = pv->pkt->size;
        out->s.start    = get_frame_start(pv, frameno);
        out->s.duration = get_frame_duration(pv, frameno);
        out->s.stop     = out->s.stop + out->s.duration;
        out->s.frametype = convert_pict_type(pv->pkt, &out->s.flags);

        if (out->s.flags & HB_FLAG_FRAMETYPE_KEY)
        {
            hb_chapter_dequeue(pv->chapter_queue, out);
        }

        out = process_delay_list(pv, out);

        hb_buffer_list_append(list, out);
        av_packet_unref(pv->pkt);
    }
}

static void Encode( hb_work_object_t *w, hb_buffer_t **buf_in,
                    hb_buffer_list_t *list )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t       * in = *buf_in;
    AVFrame             frame = {{0}};
    int                 key_frame = 0;
    int                 ret;

    if (in->s.new_chap > 0 && pv->job->chapter_markers)
    {
        /* chapters have to start with an IDR frame so request that this
           frame be coded as IDR. Since there may be multiple frames
           currently buffered in the encoder remember the timestamp so
           when this frame finally pops out of the encoder we'll mark
           its buffer as the start of a chapter. */
        key_frame = 1;
        hb_chapter_enqueue(pv->chapter_queue, in);
    }

    // Bizarro ffmpeg requires timestamp time_base to be == framerate
    // for the encoders we care about.  It writes AVCodecContext.time_base
    // to the framerate field of encoded bitstream headers, so if we
    // want correct bitstreams, we must set time_base = framerate.
    // We can't pass timestamps that are not based on the time_base
    // because encoders require accurately based timestamps in order to
    // do proper rate control.
    //
    // I.e. ffmpeg doesn't support VFR timestamps.
    //
    // Because of this, we have to do some fugly things, like storing
    // PTS values and computing DTS ourselves.
    //
    // Remember timestamp info about this frame
    save_frame_info(pv, in);
    compute_dts_offset(pv, in);

    // Convert the hb_buffer_t to avframe
    // This will consume the hb_buffer_t and make it NULL
    hb_video_buffer_to_avframe(&frame, buf_in);
    frame.pts = pv->frameno_in++;
    frame.duration = 0;

    // For constant quality, setting the quality in AVCodecContext
    // doesn't do the trick.  It must be set in the AVFrame.
    frame.quality = pv->context->global_quality;

    if (key_frame)
    {
        frame.pict_type = AV_PICTURE_TYPE_I;
        frame.flags = AV_FRAME_FLAG_KEY;
    }
    else
    {
        frame.pict_type = AV_PICTURE_TYPE_NONE;
        frame.flags = 0;
    }

    // Encode
    ret = avcodec_send_frame(pv->context, &frame);
    av_frame_unref(&frame);

    if (ret < 0)
    {
        hb_log("encavcodec: avcodec_send_frame failed");
        return;
    }

    // Write stats
    if (pv->job->pass_id == HB_PASS_ENCODE_ANALYSIS &&
        pv->context->stats_out != NULL)
    {
        fprintf( pv->file, "%s", pv->context->stats_out );
    }

    get_packets(w, list);
}

static void Flush( hb_work_object_t * w, hb_buffer_list_t * list )
{
    hb_work_private_t * pv = w->private_data;

    avcodec_send_frame(pv->context, NULL);

    // Write stats
    // vpx only writes stats at final flush
    if (pv->job->pass_id == HB_PASS_ENCODE_ANALYSIS &&
        pv->context->stats_out != NULL)
    {
        fprintf( pv->file, "%s", pv->context->stats_out );
    }

    get_packets(w, list);
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int encavcodecWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                    hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t       * in = *buf_in;
    hb_buffer_list_t    list;

    if (pv->context == NULL || pv->context->codec == NULL)
    {
        hb_error("encavcodec: codec context is uninitialized");
        return HB_WORK_DONE;
    }

#if HB_PROJECT_FEATURE_QSV
    // postponed encoder initialization, reused code from encavcodecInit()
    if (hb_hwaccel_is_full_hardware_pipeline_enabled(pv->job) &&
        hb_qsv_decode_is_enabled(pv->job) && pv->context->hw_frames_ctx == NULL && pv->job->qsv.ctx->hb_ffmpeg_qsv_hw_frames_ctx != NULL)
    {
        // use the same hw frames context as for decoder or filter graph hw frames context
        pv->context->hw_frames_ctx = pv->job->qsv.ctx->hb_ffmpeg_qsv_hw_frames_ctx;
        int open_ret = 0;
        if ((open_ret = hb_avcodec_open(pv->context, pv->qsv_data.codec, &pv->qsv_data.av_opts, HB_FFMPEG_THREADS_AUTO)))
        {
            hb_log( "encavcodecWork: avcodec_open failed: %s", av_err2str(open_ret) );
            return HB_WORK_ERROR;
        }

        /*
        * Reload colorimetry settings in case custom
        * values were set in the encoder_options string.
        */
        pv->job->color_prim_override     = pv->context->color_primaries;
        pv->job->color_transfer_override = pv->context->color_trc;
        pv->job->color_matrix_override   = pv->context->colorspace;

        // avcodec_open populates the opts dictionary with the
        // things it didn't recognize.
        AVDictionaryEntry *t = NULL;
        while( ( t = av_dict_get( pv->qsv_data.av_opts, "", t, AV_DICT_IGNORE_SUFFIX ) ) )
        {
            hb_log( "encavcodecWork: Unknown avcodec option %s", t->key );
        }
        
        pv->job->areBframes = 0;
        if (pv->context->has_b_frames > 0)
        {
            pv->job->areBframes = pv->context->has_b_frames;
        }

        if (pv->context->extradata != NULL)
        {
            hb_set_extradata(w->extradata, pv->context->extradata, pv->context->extradata_size);
        }
        av_dict_free(&pv->qsv_data.av_opts);
    }
#endif

    hb_buffer_list_clear(&list);
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        Flush(w, &list);
        hb_buffer_list_append(&list, hb_buffer_eof_init());
        *buf_out = hb_buffer_list_clear(&list);
        return HB_WORK_DONE;
    }

    Encode(w, buf_in, &list);
    *buf_out = hb_buffer_list_clear(&list);

    return HB_WORK_OK;
}

/**
 * Encoder options and presets
 */

static int apply_options(hb_job_t *job, AVCodecContext *context, AVDictionary **av_opts, hb_dict_t *lavc_opts)
{
    /* iterate through lavc_opts and have avutil parse the options for us */
    hb_dict_iter_t iter;
    for (iter  = hb_dict_iter_init(lavc_opts);
         iter != HB_DICT_ITER_DONE;
         iter  = hb_dict_iter_next(lavc_opts, iter))
    {
        const char *key = hb_dict_iter_key(iter);
        hb_value_t *value = hb_dict_iter_value(iter);
        char *str = hb_value_get_string_xform(value);

        /* Here's where the strings are passed to avutil for parsing. */
        av_dict_set(av_opts, key, str, 0);
        free(str);
    }

    return 0;
}

static int apply_encoder_options(hb_job_t *job, AVCodecContext *context, AVDictionary **av_opts)
{
#if HB_PROJECT_FEATURE_QSV
    if (hb_qsv_is_ffmpeg_supported_codec(job->vcodec))
    {
        // options applied separately via hb_qsv_apply_encoder_options() call
        return 0;
    }
#endif
    /* place job->encoder_options in an hb_dict_t for convenience */
    hb_dict_t *lavc_opts = NULL;
    if (job->encoder_options != NULL && *job->encoder_options)
    {
        lavc_opts = hb_encopts_to_dict(job->encoder_options, job->vcodec);
    }
    else
    {
        lavc_opts = hb_dict_init();
    }

    switch (job->vcodec)
    {
        default:
            apply_options(job, context, av_opts, lavc_opts);
            break;
    }

    hb_dict_free(&lavc_opts);

    return 0;
}

static int apply_vce_preset(AVDictionary **av_opts, const char *preset)
{
    if (preset)
    {
        if (!strcasecmp(preset, "balanced")
            || !strcasecmp(preset, "speed")
            || !strcasecmp(preset, "quality"))
        {
            av_opt_set(av_opts, "quality", preset, AV_OPT_SEARCH_CHILDREN);
        }
    }

    return 0;
}

static int apply_vpx_preset(AVDictionary ** av_opts, const char * preset)
{
    if (preset == NULL)
    {
        // default "medium"
        av_dict_set( av_opts, "deadline", "good", 0);
        av_dict_set( av_opts, "cpu-used", "2", 0);
    }
    else if (!strcasecmp("veryfast", preset))
    {
        av_dict_set( av_opts, "deadline", "good", 0);
        av_dict_set( av_opts, "cpu-used", "5", 0);
    }
    else if (!strcasecmp("faster", preset))
    {
        av_dict_set( av_opts, "deadline", "good", 0);
        av_dict_set( av_opts, "cpu-used", "4", 0);
    }
    else if (!strcasecmp("fast", preset))
    {
        av_dict_set( av_opts, "deadline", "good", 0);
        av_dict_set( av_opts, "cpu-used", "3", 0);
    }
    else if (!strcasecmp("medium", preset))
    {
        av_dict_set( av_opts, "deadline", "good", 0);
        av_dict_set( av_opts, "cpu-used", "2", 0);
    }
    else if (!strcasecmp("slow", preset))
    {
        av_dict_set( av_opts, "deadline", "good", 0);
        av_dict_set( av_opts, "cpu-used", "1", 0);
    }
    else if (!strcasecmp("slower", preset))
    {
        av_dict_set( av_opts, "deadline", "good", 0);
        av_dict_set( av_opts, "cpu-used", "0", 0);
    }
    else if (!strcasecmp("veryslow", preset))
    {
        av_dict_set( av_opts, "deadline", "best", 0);
        av_dict_set( av_opts, "cpu-used", "0", 0);
    }
    else
    {
        // default "medium"
        hb_log("apply_vpx_preset: Unknown VPx encoder preset %s", preset);
        return -1;
    }

    return 0;
}

// VP8 and VP9 have some options in common and some different
static int apply_vp8_preset(AVDictionary ** av_opts, const char * preset)
{
    return apply_vpx_preset(av_opts, preset);
}

static int apply_vp9_preset(AVDictionary ** av_opts, const char * preset)
{
    av_dict_set(av_opts, "row-mt", "1", 0);
    return apply_vpx_preset(av_opts, preset);
}

static int apply_vp9_10bit_preset(AVDictionary ** av_opts, const char * preset)
{
    av_dict_set(av_opts, "row-mt", "1", 0);
    av_dict_set(av_opts, "profile", "2", 0);
    return apply_vpx_preset(av_opts, preset);
}

static int apply_ffv1_preset(AVCodecContext *context, AVDictionary **av_opts, const char *preset)
{
    if (!strcasecmp(preset, "preservation"))
    {
        context->gop_size = 1;
        context->level = 3;
        av_dict_set(av_opts, "coder", "1", 0);
        av_dict_set(av_opts, "context", "1", 0);
        av_dict_set(av_opts, "slicecrc", "1", 0);
    }
    return 0;
}

static int apply_encoder_preset(int vcodec, AVCodecContext *context,
                                AVDictionary **av_opts,
                                const char *preset)
{
    switch (vcodec)
    {
        case HB_VCODEC_FFMPEG_VP8:
            return apply_vp8_preset(av_opts, preset);
        case HB_VCODEC_FFMPEG_VP9:
            return apply_vp9_preset(av_opts, preset);
        case HB_VCODEC_FFMPEG_VP9_10BIT:
            return apply_vp9_10bit_preset(av_opts, preset);

        case HB_VCODEC_FFMPEG_VCE_H264:
        case HB_VCODEC_FFMPEG_VCE_H265:
        case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
        case HB_VCODEC_FFMPEG_VCE_AV1:
            return apply_vce_preset(av_opts, preset);

#if HB_PROJECT_FEATURE_NVENC
        case HB_VCODEC_FFMPEG_NVENC_H264:
        case HB_VCODEC_FFMPEG_NVENC_H265:
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
        case HB_VCODEC_FFMPEG_NVENC_AV1:
        case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
            preset = hb_map_nvenc_preset_name(preset);
            av_dict_set( av_opts, "preset", preset, 0);
            break;
#endif

#if HB_PROJECT_FEATURE_QSV
        case HB_VCODEC_FFMPEG_QSV_H264:
        case HB_VCODEC_FFMPEG_QSV_H265:
        case HB_VCODEC_FFMPEG_QSV_H265_10BIT:
        case HB_VCODEC_FFMPEG_QSV_AV1:
        case HB_VCODEC_FFMPEG_QSV_AV1_10BIT:
            preset = hb_map_qsv_preset_name(preset);
            av_dict_set( av_opts, "preset", preset, 0);
            hb_log("encavcodec: encoding with preset %s", preset);
            break;
#endif

        case HB_VCODEC_FFMPEG_FFV1:
            return apply_ffv1_preset(context, av_opts, preset);
        default:
            break;
    }

    return 0;
}

static int apply_vp9_tune(AVDictionary ** av_opts, const char * tune)
{
    av_dict_set(av_opts, "tune-content", tune, 0);
    return 0;
}

static int apply_encoder_tune(int vcodec, AVDictionary ** av_opts,
                                const char * tune)
{
    switch (vcodec)
    {
        case HB_VCODEC_FFMPEG_VP9:
        case HB_VCODEC_FFMPEG_VP9_10BIT:
            return apply_vp9_tune(av_opts, tune);
        default:
            break;
    }

    return 0;
}

static int apply_encoder_level(AVCodecContext *context, AVDictionary **av_opts, int vcodec, const char *encoder_level)
{
    const char * const *level_names = NULL;
    const int  *level_values = NULL;

    switch (vcodec)
    {
        case HB_VCODEC_FFMPEG_VCE_H264:
        case HB_VCODEC_FFMPEG_NVENC_H264:
        case HB_VCODEC_FFMPEG_MF_H264:
            level_names = hb_h264_level_names;
            level_values = hb_h264_level_values;
            break;

#if HB_PROJECT_FEATURE_QSV
        case HB_VCODEC_FFMPEG_QSV_H264:
            level_names = hb_qsv_h264_level_names;
            level_values = hb_qsv_h264_levels;
            break;
#endif

        case HB_VCODEC_FFMPEG_VCE_H265:
        case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
        case HB_VCODEC_FFMPEG_NVENC_H265:
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
        case HB_VCODEC_FFMPEG_MF_H265:
            level_names = hb_h265_level_names;
            level_values = hb_h265_level_values;
            break;

#if HB_PROJECT_FEATURE_QSV
        case HB_VCODEC_FFMPEG_QSV_H265:
        case HB_VCODEC_FFMPEG_QSV_H265_10BIT:
            level_names = hb_qsv_h265_level_names;
            level_values = hb_qsv_h265_levels;
            break;
#endif

        case HB_VCODEC_FFMPEG_VCE_AV1:
        case HB_VCODEC_FFMPEG_NVENC_AV1:
        case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
        case HB_VCODEC_FFMPEG_MF_AV1:
            level_names = hb_av1_level_names;
            level_values = hb_av1_level_values;
            break;

#if HB_PROJECT_FEATURE_QSV
        case HB_VCODEC_FFMPEG_QSV_AV1:
        case HB_VCODEC_FFMPEG_QSV_AV1_10BIT:
            level_names = hb_qsv_av1_level_names;
            level_values = hb_qsv_av1_levels;
            break;
#endif

        case HB_VCODEC_FFMPEG_FFV1:
            level_names = hb_ffv1_level_names;
            level_values = hb_ffv1_level_values;
            break;
    }

    context->level = FF_LEVEL_UNKNOWN;

    if (level_names == NULL || level_values == NULL)
    {
        return 0;
    }

    if (encoder_level != NULL && *encoder_level)
    {
        int i = 1;
        while (level_names[i] != NULL)
        {
            if (!strcasecmp(encoder_level, level_names[i]))
            {
                if (vcodec == HB_VCODEC_FFMPEG_NVENC_H264 ||
                    vcodec == HB_VCODEC_FFMPEG_NVENC_H265 ||
                    vcodec == HB_VCODEC_FFMPEG_NVENC_H265_10BIT ||
                    vcodec == HB_VCODEC_FFMPEG_NVENC_AV1 ||
                    vcodec == HB_VCODEC_FFMPEG_NVENC_AV1_10BIT)
                {
                    av_dict_set(av_opts, "level", level_names[i], 0);
                }
                else
                {
                    context->level = level_values[i];
                }
            }
            ++i;
        }
    }

    return 0;
}

const char* const* hb_av_preset_get_names(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_FFMPEG_VP8:
        case HB_VCODEC_FFMPEG_VP9:
        case HB_VCODEC_FFMPEG_VP9_10BIT:
            return vpx_preset_names;

        case HB_VCODEC_FFMPEG_VCE_H264:
        case HB_VCODEC_FFMPEG_VCE_H265:
        case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
        case HB_VCODEC_FFMPEG_VCE_AV1:
            return hb_vce_preset_names;

        case HB_VCODEC_FFMPEG_NVENC_H264:
        case HB_VCODEC_FFMPEG_NVENC_H265:
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
        case HB_VCODEC_FFMPEG_NVENC_AV1:
        case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
            return h26x_nvenc_preset_names;

        case HB_VCODEC_FFMPEG_MF_H264:
        case HB_VCODEC_FFMPEG_MF_H265:
        case HB_VCODEC_FFMPEG_MF_AV1:
            return h26x_mf_preset_name;

        case HB_VCODEC_FFMPEG_FFV1:
            return ffv1_preset_names;

#if HB_PROJECT_FEATURE_QSV
        case HB_VCODEC_FFMPEG_QSV_H264:
        case HB_VCODEC_FFMPEG_QSV_H265:
        case HB_VCODEC_FFMPEG_QSV_H265_10BIT:
        case HB_VCODEC_FFMPEG_QSV_AV1:
        case HB_VCODEC_FFMPEG_QSV_AV1_10BIT:
            return hb_qsv_preset_get_names();
#endif

        default:
            return NULL;
    }
}

const char* const* hb_av_tune_get_names(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_FFMPEG_VP9:
        case HB_VCODEC_FFMPEG_VP9_10BIT:
            return vp9_tune_names;
        default:
            return empty_tune_names;
    }
}

const char* const* hb_av_profile_get_names(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_FFMPEG_NVENC_H264:
            return h264_nvenc_profile_names;
        case HB_VCODEC_FFMPEG_NVENC_H265:
            return h265_nvenc_profile_names;
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
            return h265_nvenc_10bit_profile_names;
        case HB_VCODEC_FFMPEG_MF_H264:
            return h264_mf_profile_name;
        case HB_VCODEC_FFMPEG_MF_H265:
            return h265_mf_profile_name;
        case HB_VCODEC_FFMPEG_MF_AV1:
            return av1_mf_profile_name;
        case HB_VCODEC_FFMPEG_FFV1:
            return ffv1_profile_names;
        case HB_VCODEC_FFMPEG_QSV_H264:
            return h264_qsv_profile_name;
        case HB_VCODEC_FFMPEG_QSV_H265:
        case HB_VCODEC_FFMPEG_QSV_H265_10BIT:
            return h265_qsv_profile_name;
         default:
             return NULL;
     }
}

const char* const* hb_av_level_get_names(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_FFMPEG_NVENC_H264:
        case HB_VCODEC_FFMPEG_MF_H264:
            return hb_h264_level_names;

        case HB_VCODEC_FFMPEG_VCE_H264:
            return hb_vce_h264_level_names; // Not quite the same as x264

        case HB_VCODEC_FFMPEG_NVENC_H265:
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
        case HB_VCODEC_FFMPEG_VCE_H265:
        case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
        case HB_VCODEC_FFMPEG_QSV_H265:
        case HB_VCODEC_FFMPEG_QSV_H265_10BIT:
        case HB_VCODEC_FFMPEG_MF_H265:
            return hb_h265_level_names;

        case HB_VCODEC_FFMPEG_VCE_AV1:
        case HB_VCODEC_FFMPEG_NVENC_AV1:
        case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
        case HB_VCODEC_FFMPEG_QSV_AV1:
        case HB_VCODEC_FFMPEG_QSV_AV1_10BIT:
        case HB_VCODEC_FFMPEG_MF_AV1:
            return hb_av1_level_names;

        case HB_VCODEC_FFMPEG_FFV1:
            return hb_ffv1_level_names;

         default:
             return NULL;
     }
}

const int* hb_av_get_pix_fmts(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_FFMPEG_MF_H264:
        case HB_VCODEC_FFMPEG_MF_H265:
        case HB_VCODEC_FFMPEG_MF_AV1:   // NV12 only
            return h26x_mf_pix_fmts;

        case HB_VCODEC_FFMPEG_NVENC_H264:
        case HB_VCODEC_FFMPEG_NVENC_H265:
        case HB_VCODEC_FFMPEG_NVENC_AV1:
            return nvenc_pix_formats;

        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
        case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
            return nvenc_pix_formats_10bit;

        case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
            return vce_pix_formats_10bit;

        case HB_VCODEC_FFMPEG_VP9_10BIT:
            return standard_10bit_pix_fmts;

        case HB_VCODEC_FFMPEG_FFV1:
            return ffv1_pix_formats;

        case HB_VCODEC_FFMPEG_QSV_H264:
        case HB_VCODEC_FFMPEG_QSV_H265:
        case HB_VCODEC_FFMPEG_QSV_AV1:
            return qsv_pix_formats;

        case HB_VCODEC_FFMPEG_QSV_H265_10BIT:
        case HB_VCODEC_FFMPEG_QSV_AV1_10BIT:
            return qsv_10bit_pix_formats;

         default:
             return standard_pix_fmts;
     }
}
