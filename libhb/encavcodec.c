/* encavcodec.c

   Copyright (c) 2003-2022 HandBrake Team
   Copyright 2022 NVIDIA Corporation
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hb_dict.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/h264_common.h"
#include "handbrake/h265_common.h"
#include "handbrake/av1_common.h"
#include "handbrake/nal_units.h"

#if HB_PROJECT_FEATURE_NVENC
#include "handbrake/nvenc_common.h"
#endif


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

    struct {
        int64_t          start;
        int64_t          duration;
    } frame_info[FRAME_INFO_SIZE];

    hb_chapter_queue_t * chapter_queue;
};

int  encavcodecInit( hb_work_object_t *, hb_job_t * );
int  encavcodecWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encavcodecClose( hb_work_object_t * );

static int apply_encoder_preset(int vcodec, AVDictionary ** av_opts,
                                const char * preset);
static int apply_encoder_options(hb_job_t *job, AVCodecContext *context,
                                 AVDictionary **av_opts);

hb_work_object_t hb_encavcodec =
{
    WORK_ENCAVCODEC,
    "FFMPEG encoder (libavcodec)",
    encavcodecInit,
    encavcodecWork,
    encavcodecClose
};

static const char * const vpx_preset_names[] =
{
    "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow", NULL
};

static const char * const h26x_nvenc_preset_names[] =
{
    "fastest", "faster", "fast", "medium", "slow", "slower", "slowest", NULL
};

static const char * const av1_svt_preset_names[] =
{
    "12", "11", "10", "9", "8", "7", "6", "5", "4", "3", "2", "1", "0", NULL
};

static const char * const av1_svt_tune_names[] =
{
    "psnr", "fastdecode", NULL
};

static const char * const av1_svt_profile_names[] =
{
    "auto", "main", NULL // "high", "profesional"
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

static const enum AVPixelFormat standard_pix_fmts[] =
{
    AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE
};

static const enum AVPixelFormat standard_10bit_pix_fmts[] =
{
    AV_PIX_FMT_YUV420P10, AV_PIX_FMT_NONE
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

int encavcodecInit( hb_work_object_t * w, hb_job_t * job )
{
    int ret = 0;
    char reason[80];
    char * codec_name = NULL;
    const AVCodec * codec = NULL;
    AVCodecContext * context;
    AVRational fps;

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
                    hb_log("encavcodecInit: H.265 (AMD VCE)");
                    codec_name = "hevc_amf";
                    break;
                case HB_VCODEC_FFMPEG_MF_H265:
                    hb_log("encavcodecInit: H.265 (MediaFoundation)");
                    codec_name = "hevc_mf";
                    break;
            }
        }break;
        case AV_CODEC_ID_AV1:
        {
            hb_log("encavcodecInit: AV1 encoder");
            codec_name = "libsvtav1";
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
    if (codec->supported_framerates)
    {
        AVRational supported_fps;
        supported_fps = codec->supported_framerates[av_find_nearest_q_idx(fps, codec->supported_framerates)];
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
    if ((job->vcodec == HB_VCODEC_FFMPEG_VCE_H264) || (job->vcodec == HB_VCODEC_FFMPEG_VCE_H265))
    {
        // Set encoder preset
        context->profile = FF_PROFILE_UNKNOWN;
        if (job->encoder_preset != NULL && *job->encoder_preset)
        {
            if ((!strcasecmp(job->encoder_preset, "balanced"))
                || (!strcasecmp(job->encoder_preset, "speed"))
                || (!strcasecmp(job->encoder_preset, "quality")))
            {
                av_opt_set(context, "quality", job->encoder_preset, AV_OPT_SEARCH_CHILDREN);
            }
        }
    }

    AVDictionary * av_opts = NULL;
    if (apply_encoder_preset(job->vcodec, &av_opts, job->encoder_preset))
    {
        av_free( context );
        av_dict_free( &av_opts );
        ret = 1;
        goto done;
    }

    if (apply_encoder_options(job, context, &av_opts))
    {
        av_free( context );
        av_dict_free( &av_opts );
        ret = 1;
        goto done;
    }

    // Now set the things in context that we don't want to allow
    // the user to override.
    if (job->vquality <= HB_INVALID_VIDEO_QUALITY)
    {
        /* Average bitrate */
        context->bit_rate = 1000 * job->vbitrate;
        // ffmpeg's mpeg2 encoder requires that the bit_rate_tolerance be >=
        // bitrate * fps
        context->bit_rate_tolerance = context->bit_rate * av_q2d(fps) + 1;

        if ( job->vcodec == HB_VCODEC_FFMPEG_NVENC_H264 || job->vcodec == HB_VCODEC_FFMPEG_NVENC_H265 || job->vcodec == HB_VCODEC_FFMPEG_NVENC_H265_10BIT) {
            av_dict_set( &av_opts, "rc", "vbr", 0 );
            av_dict_set( &av_opts, "multipass", "fullres", 0 );
            hb_log( "encavcodec: encoding at rc=vbr, multipass=fullres, Bitrate %d", job->vbitrate );
        }

        if ( job->vcodec == HB_VCODEC_FFMPEG_VCE_H264 || job->vcodec == HB_VCODEC_FFMPEG_VCE_H265 )
        {
            av_dict_set( &av_opts, "rc", "vbr_peak", 0 );

            // since we do not have scene change detection, set a
            // relatively short gop size to help avoid stale references
            context->gop_size = (int)(FFMIN(av_q2d(fps) * 2, 120));

            //Work around an ffmpeg issue mentioned in issue #3447
            if (job->vcodec == HB_VCODEC_FFMPEG_VCE_H265)
            {
               av_dict_set( &av_opts, "qmin",  "0", 0 );
               av_dict_set( &av_opts, "qmax", "51", 0 );
            }
            hb_log( "encavcodec: encoding at rc=vbr_peak Bitrate %d", job->vbitrate );
        }

        if (job->vcodec == HB_VCODEC_FFMPEG_MF_H264 ||
            job->vcodec == HB_VCODEC_FFMPEG_MF_H265) {
            av_dict_set(&av_opts, "rate_control", "u_vbr", 0); // options are cbr, pc_vbr, u_vbr, ld_vbr, g_vbr, gld_vbr

            // On Qualcomm encoders, the VBR modes can easily drop frames if
            // the rate control feels like it needs it (in certain
            // configurations), unless scenario is set to camera_record.
            av_dict_set(&av_opts, "scenario", "camera_record", 0);
        }
    }
    else
    {
        /* Constant quantizer */

        //Set constant quality for libvpx
        if ( w->codec_param == AV_CODEC_ID_VP8 ||
             w->codec_param == AV_CODEC_ID_VP9 )
        {
            // These settings produce better image quality than
            // what was previously used
            context->flags |= AV_CODEC_FLAG_QSCALE;
            context->global_quality = FF_QP2LAMBDA * job->vquality + 0.5;

            char quality[7];
            snprintf(quality, 7, "%.2f", job->vquality);
            av_dict_set( &av_opts, "crf", quality, 0 );
            //This value was chosen to make the bitrate high enough
            //for libvpx to "turn off" the maximum bitrate feature
            //that is normally applied to constant quality.
            context->bit_rate = (int64_t)job->width * job->height *
                                         fps.num / fps.den;
            hb_log( "encavcodec: encoding at CQ %.2f", job->vquality );
        }
        //Set constant quality for svt-av1
        else if (job->vcodec == HB_VCODEC_FFMPEG_SVT_AV1 ||
                 job->vcodec == HB_VCODEC_FFMPEG_SVT_AV1_10BIT)
        {
            char quality[7];
            snprintf(quality, 7, "%.2f", job->vquality);
            av_dict_set( &av_opts, "crf", quality, 0 );
            hb_log( "encavcodec: encoding at CRF %.2f", job->vquality );
        }
        //Set constant quality for nvenc
        else if ( job->vcodec == HB_VCODEC_FFMPEG_NVENC_H264 ||
                  job->vcodec == HB_VCODEC_FFMPEG_NVENC_H265 ||
                  job->vcodec == HB_VCODEC_FFMPEG_NVENC_H265_10BIT)
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
            av_dict_set( &av_opts, "multipass", "fullres", 0 );

            // further Advanced Quality Settings in Constant Quality Mode
            av_dict_set( &av_opts, "init_qpP", quality, 0 );
            av_dict_set( &av_opts, "init_qpB", qualityB, 0 );
            av_dict_set( &av_opts, "init_qpI", qualityI, 0 );
            hb_log( "encavcodec: encoding at rc=vbr, multipass=fullres, %.2f", job->vquality );
        }
        else if ( job->vcodec == HB_VCODEC_FFMPEG_VCE_H264 )
        {
            // since we do not have scene change detection, set a
            // relatively short gop size to help avoid stale references
            context->gop_size = (int)(FFMIN(av_q2d(fps) * 2, 120));

            char quality[7];
            char qualityB[7];
            double adjustedQualityB = job->vquality + 2;

            snprintf(quality, 7, "%.2f", job->vquality);
            snprintf(qualityB, 7, "%.2f", adjustedQualityB);

            if (adjustedQualityB > 51) {
                adjustedQualityB = 51;
            }

            av_dict_set( &av_opts, "rc", "cqp", 0 );

            av_dict_set( &av_opts, "qp_i", quality, 0 );
            av_dict_set( &av_opts, "qp_p", quality, 0 );
            av_dict_set( &av_opts, "qp_b", qualityB, 0 );
            hb_log( "encavcodec: encoding at QP %.2f", job->vquality );
        }
        else if ( job->vcodec == HB_VCODEC_FFMPEG_VCE_H265 )
        {
            char  *vce_h265_max_au_size_char,
                   vce_h265_q_char[4];
            int    vce_h265_cq_step,
                   vce_h265_max_au_size,
                   vce_h265_max_au_size_length,
                   vce_h265_qmin,
                   vce_h265_qmax,
                   vce_h265_qmin_p,
                   vce_h265_qmax_p,
                   vce_h265_threshold;
            double vce_h265_bit_rate,
                   vce_h265_buffer_size,
                   vce_h265_comp_factor,
                   vce_h265_exp_scale,
                   vce_h265_max_rate = 0;

            /*
              constant quality tuning for peak constrained vbr rate control

              summary of settings:
              - set a relatively short gop size to help avoid stale references, since we do not have scene detection
              - constrain qmin and qmax to ensure consistent visual quality window regardless of complexity in detail and/or motion
              - limit max rate to 1.3x (effectively ~2x) target bit rate to allow adequate variance while avoiding extreme peaks
              - calculate hrd buffer size based on max rate to manage short term data rate in conjunction with max au size
              - set max au size to 1/4 hrd buffer size to improve intra-gop bit allocation and minimize refresh/recovery effects at gop transitions

              additional settings for low quality / bit rates:
              - increase gop size slightly to save bits by reducing total keyframe count
              - increase qmin to save bits by minimizing overallocation to static scenes
              - increase max rate to give the encoder flexibility in reallocating saved bits
              - increase qmax as a last resort to avoid overshooting data rate
            */

            // set gop size to two seconds in frames
            context->gop_size = (int)(FFMIN(av_q2d(fps) * 2, 120));

            // the fun part
            if (job->vquality > 0 && job->vquality < 51)
            {
                // set rc mode to peak constrained vbr
                av_dict_set( &av_opts, "rc", "vbr_peak", 0 );

                /*
                // calculate CQ 33 bit rate, which is the basis for all other CQ bit rates
                vce_h265_bit_rate = ((sqrt(sqrt(job->width * job->height) * job->width * job->height / 1000) * 1.2) + 150) * 1000;

                // initial compounding factor for calculating bit rates for other CQ values
                vce_h265_comp_factor = 1.16;

                // calculate CQ 39 bit rate, which is the low bit rate quality threshold
                vce_h265_threshold = vce_h265_bit_rate * pow(1.0L / vce_h265_comp_factor, 5);

                // calculate bit rate for user specified CQ value
                if (job->vquality < 33)
                {
                    for (vce_h265_cq_step = 32; vce_h265_cq_step >= job->vquality; vce_h265_cq_step--)
                    {
                        if (vce_h265_cq_step < 18)
                        {
                            vce_h265_comp_factor = 1.14; // vce_h265_comp_factor * 0.9827586207;
                        }
                        if (vce_h265_cq_step < 15)
                        {
                            vce_h265_comp_factor = 1.12; // vce_h265_comp_factor * 0.9824561404;
                        }
                        vce_h265_bit_rate = vce_h265_bit_rate * vce_h265_comp_factor;
                    }
                }
                else if (job->vquality > 33)
                {
                    for (vce_h265_cq_step = 34; vce_h265_cq_step <= job->vquality; vce_h265_cq_step++)
                    {
                        if (vce_h265_cq_step > 39)
                        {
                            vce_h265_comp_factor = 1.15; // vce_h265_comp_factor * 0.9913793103;
                        }
                        if (vce_h265_cq_step > 49)
                        {
                            vce_h265_comp_factor = 1.25; // vce_h265_comp_factor * 1.0869565217;
                        }
                        vce_h265_bit_rate = vce_h265_bit_rate / vce_h265_comp_factor;
                    }
                }
                context->bit_rate = (int)(vce_h265_bit_rate);
                */

                // calculate CQ 30 bit rate, which is the basis for all other CQ bit rates
                vce_h265_bit_rate = sqrt(job->width * job->height * pow(sqrt(job->width * job->height) / 1000, 2.5) + 400000) * 1000;

                // initial compounding factor for calculating bit rates for other CQ values
                vce_h265_comp_factor = 1.15;

                // calculate CQ 39 bit rate, which is the low bit rate quality threshold
                vce_h265_threshold = vce_h265_bit_rate * pow(1.0L / vce_h265_comp_factor, 8);

                // calculate bit rate for user specified CQ value
                if (job->vquality < 30)
                {
                    vce_h265_comp_factor = 1.18;
                    for (vce_h265_cq_step = 29; vce_h265_cq_step >= job->vquality; vce_h265_cq_step--)
                    {
                        // reticulate splines
                        if (vce_h265_cq_step < 21)
                        {
                            vce_h265_comp_factor = 1.15;
                        }
                        if (vce_h265_cq_step < 15)
                        {
                            vce_h265_comp_factor = 1.12;
                        }
                        if (vce_h265_cq_step < 8)
                        {
                            vce_h265_comp_factor = 1.1;
                        }
                        if (vce_h265_cq_step < 3)
                        {
                            vce_h265_comp_factor = 1.08;
                        }
                        vce_h265_bit_rate = vce_h265_bit_rate * vce_h265_comp_factor;
                    }
                }
                else
                {
                    for (vce_h265_cq_step = 31; vce_h265_cq_step <= job->vquality; vce_h265_cq_step++)
                    {
                        vce_h265_bit_rate = vce_h265_bit_rate / vce_h265_comp_factor;
                    }
                }
                context->bit_rate = (int)(vce_h265_bit_rate);

                // QP 1-19
                // constrain qmax to ensure bits are not underallocated to motion
                vce_h265_qmin   = 0;
                vce_h265_qmax   = (int)(sqrt(job->vquality - 0.75) * 8);
                vce_h265_qmin_p = 0;
                vce_h265_qmax_p = vce_h265_qmax + 2;

                if (vce_h265_bit_rate < vce_h265_threshold * 12)
                {
                    // CQ 20-22
                    // constrain qmin to ensure bits are not overallocated to low motion, static scenes
                    vce_h265_qmin   =  4;
                    vce_h265_qmax   = 34;
                    vce_h265_qmin_p =  8;
                    vce_h265_qmax_p = 38;

                    if (vce_h265_bit_rate < vce_h265_threshold * 8)
                    {
                        // CQ 23-27
                        vce_h265_qmin   =  8;
                        vce_h265_qmax   = 36;
                        vce_h265_qmin_p = 12;
                        vce_h265_qmax_p = 40;

                        if (vce_h265_bit_rate < vce_h265_threshold * 4)
                        {
                            // CQ 28-32
                            vce_h265_qmin   = 16;
                            vce_h265_qmax   = 38;
                            vce_h265_qmin_p = 19;
                            vce_h265_qmax_p = 42;

                            if (vce_h265_bit_rate < vce_h265_threshold * 2)
                            {
                                // CQ 33-38
                                // bit rate is at or just above the starvation threshold
                                // increase qmax to baseline for decent references (I) and minimal motion trails, recovery effects (P)
                                vce_h265_qmin   = 19;
                                vce_h265_qmax   = 39;
                                vce_h265_qmin_p = 22;
                                vce_h265_qmax_p = 44;

                                if (vce_h265_bit_rate <= vce_h265_threshold)
                                {
                                    // CQ 39-40
                                    // bit rate is at or just below the starvation threshold
                                    // increase gop size to save bits by reducing total keyframe count
                                    // increase qmin to continue saving bits by minimizing overallocation to static scenes
                                    // increase qmax beyond baseline for decent references (I) and minimal motion trails, recovery effects (P)
                                    // increase max rate to allow higher relative peaks in short bursts
                                    context->gop_size = (int)(FFMIN(av_q2d(fps) * 3, 180));
                                    vce_h265_qmin   = 22;
                                    vce_h265_qmax   = 40;
                                    vce_h265_qmin_p = 24;
                                    vce_h265_qmax_p = 45;
                                    vce_h265_max_rate = vce_h265_bit_rate * 1.5;

                                    if (vce_h265_bit_rate < vce_h265_threshold * 0.85)
                                    {
                                        // CQ 41
                                        vce_h265_qmin   = 24;
                                        vce_h265_qmax   = 41;
                                        vce_h265_qmin_p = 27;
                                        vce_h265_qmax_p = 46;
                                        vce_h265_max_rate = vce_h265_bit_rate * 2.5;

                                        if (vce_h265_bit_rate < vce_h265_threshold * 0.7)
                                        {
                                            // CQ 42
                                            vce_h265_qmin   = 27;
                                            vce_h265_qmax   = 42;
                                            vce_h265_qmin_p = 30;
                                            vce_h265_qmax_p = 47;
                                            vce_h265_max_rate = vce_h265_bit_rate * 6.5;

                                            if (vce_h265_bit_rate < vce_h265_threshold * 0.6)
                                            {
                                                // CQ 43
                                                vce_h265_qmin   = 31;
                                                vce_h265_qmax   = 44;
                                                vce_h265_qmin_p = 34;
                                                vce_h265_qmax_p = 48;
                                                vce_h265_max_rate = vce_h265_bit_rate * 15;

                                                if (vce_h265_bit_rate < vce_h265_threshold * 0.51)
                                                {
                                                    // CQ 44-45
                                                    vce_h265_qmin   = 35;
                                                    vce_h265_qmax   = 46;
                                                    vce_h265_qmin_p = 38;
                                                    vce_h265_qmax_p = 49;
                                                    vce_h265_max_rate = vce_h265_bit_rate * 19;

                                                    if (vce_h265_bit_rate < vce_h265_threshold * 0.42)
                                                    {
                                                        // CQ 46-47
                                                        // bit rate is insufficient for any motion
                                                        vce_h265_qmin   = 39;
                                                        vce_h265_qmax   = 48;
                                                        vce_h265_qmin_p = 42;
                                                        vce_h265_qmax_p = 50;
                                                        vce_h265_max_rate = vce_h265_bit_rate * 22;

                                                        if (vce_h265_bit_rate < vce_h265_threshold * 0.32)
                                                        {
                                                            // CQ 48-49
                                                            // bit rate is entirely insufficient
                                                            vce_h265_qmin   = 43;
                                                            vce_h265_qmax   = 49;
                                                            vce_h265_qmin_p = 46;
                                                            vce_h265_qmax_p = 50;
                                                            vce_h265_max_rate = vce_h265_bit_rate * 24;

                                                            if (vce_h265_bit_rate < vce_h265_threshold * 0.24)
                                                            {
                                                                // CQ 50
                                                                // there are no bits
                                                                vce_h265_qmin   = 45;
                                                                vce_h265_qmax   = 49;
                                                                vce_h265_qmin_p = 49;
                                                                vce_h265_qmax_p = 51;
                                                                vce_h265_max_rate = vce_h265_bit_rate * 10;
                                                            }

                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // factor for calculating max rate and buffer size
                vce_h265_exp_scale = FFMIN(1.0L * 1000000 / vce_h265_bit_rate, 1.0L);

                // ideal max rate is ~1.3x target bit rate, diminishing on a curve as target bit rate increases
                // this allows allows the actual bit rate to vary as needed to ensure consistent visual quality,
                // while limiting potentially exteme one-second peaks to approximately double the target bit rate
                if (vce_h265_max_rate == 0)
                {
                    vce_h265_max_rate = vce_h265_bit_rate * ((vce_h265_exp_scale * 10) + 1);
                    vce_h265_max_rate = FFMAX(vce_h265_bit_rate * 1.05L, FFMIN(vce_h265_max_rate, vce_h265_bit_rate * 1.3L));
                }
                context->rc_max_rate = (int)(vce_h265_max_rate);

                // ideal hrd buffer size is the calculated max rate, diminishing on a curve as target bit rate increases
                // minimum 1/3 target bit rate ensures the buffer size is not too constrained at higher target bit rates
                vce_h265_buffer_size = FFMAX(vce_h265_bit_rate / 3, vce_h265_max_rate - (vce_h265_max_rate / FFMAX(vce_h265_exp_scale * 150.0L, 1.5L)));
                if (vce_h265_buffer_size / vce_h265_max_rate > 0.98L)
                {
                    // buffer size is nearly identical to max rate, make them equal
                    vce_h265_buffer_size = vce_h265_max_rate;
                }
                context->rc_buffer_size = (int)(vce_h265_buffer_size);
                context->rc_initial_buffer_occupancy = context->rc_buffer_size;

                // ideal max au size (frame size + overhead) is 1/4 the hrd buffer size
                // this improves bit allocation by preventing the encoder from spending too many bits early in the gop
                // or during periods of low motion, leaving too few bits for remaining frames and objects in motion
                // better intra-gop quality consistency also helps minimize refresh/recovery effects at gop transitions
                // if max_au_size is set, libavcodec will also set enforce_hrd for us
                vce_h265_max_au_size        = (int)(vce_h265_buffer_size * 0.25);
                vce_h265_max_au_size_length = snprintf(NULL, 0, "%d", vce_h265_max_au_size);
                vce_h265_max_au_size_char   = malloc(vce_h265_max_au_size_length + 1);
                if( !vce_h265_max_au_size_char )
                {
                    hb_log( "encavcodecInit: malloc of size %d "
                            "failed", vce_h265_max_au_size_length );
                    ret = 1;
                    goto done;
                }
                snprintf(vce_h265_max_au_size_char, vce_h265_max_au_size_length + 1, "%d", vce_h265_max_au_size);
                av_dict_set( &av_opts, "max_au_size",  vce_h265_max_au_size_char, 0 );
                free(vce_h265_max_au_size_char);
            }
            else
            {
                // set rc mode to cqp
                av_dict_set( &av_opts, "rc", "cqp", 0 );

                // set relatively long gop size for CQ 0
                // does not affect quality; only IDR frequency and thus file size
                context->gop_size = job->vquality < 1 ? 250 : (int)(FFMIN(av_q2d(fps) * 3, 180));

                // CQ 0  == CQP 0
                // CQ 51 == CQP 51
                vce_h265_qmin   = job->vquality < 1 ? 0 : 51;
                vce_h265_qmax   = vce_h265_qmin;
                vce_h265_qmin_p = vce_h265_qmin;
                vce_h265_qmax_p = vce_h265_qmin;
                snprintf(vce_h265_q_char, 4, "%d", vce_h265_qmin);
                av_dict_set( &av_opts, "qp_i", vce_h265_q_char, 0 );
                av_dict_set( &av_opts, "qp_p", vce_h265_q_char, 0 );
            }

            context->qmin = vce_h265_qmin;
            context->qmax = vce_h265_qmax;
            snprintf(vce_h265_q_char, 4, "%d", vce_h265_qmin_p);
            av_dict_set( &av_opts, "min_qp_p", vce_h265_q_char, 0 );
            snprintf(vce_h265_q_char, 4, "%d", vce_h265_qmax_p);
            av_dict_set( &av_opts, "max_qp_p", vce_h265_q_char, 0 );

            hb_log( "encavcodec: encoding at constant quality %d", (int)(job->vquality) );
            hb_log( "encavcodec: QP (I)      %d-%d", vce_h265_qmin, vce_h265_qmax );
            hb_log( "encavcodec: QP (P)      %d-%d", vce_h265_qmin_p, vce_h265_qmax_p );
            hb_log( "encavcodec: GOP Size    %d",    context->gop_size );
            if (vce_h265_max_rate > 0)
            {
                hb_log( "encavcodec: Max Rate    %"PRId64"", context->rc_max_rate/1000 );
                hb_log( "encavcodec: Buffer Size %d", context->rc_buffer_size/1000 );
                hb_log( "encavcodec: Max AU Size %d", vce_h265_max_au_size/1000 );
            }
        }
        else if (job->vcodec == HB_VCODEC_FFMPEG_MF_H264 ||
                 job->vcodec == HB_VCODEC_FFMPEG_MF_H265)
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
#if HB_PROJECT_FEATURE_NVENC
    if (hb_nvdec_is_enabled(pv->job))
    {
        context->hw_device_ctx = pv->job->nv_hw_ctx.hw_device_ctx;
        hb_nvdec_hwframes_ctx_init(context, job);
    }
    else
    {
        context->pix_fmt = job->output_pix_fmt;
    }
#else
    context->pix_fmt = job->output_pix_fmt;
#endif

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
        // Set profile and level
        context->profile = FF_PROFILE_UNKNOWN;
        if (job->encoder_profile != NULL && *job->encoder_profile)
        {
            if (!strcasecmp(job->encoder_profile, "baseline"))
                context->profile = FF_PROFILE_H264_BASELINE;
            else if (!strcasecmp(job->encoder_profile, "main"))
                 context->profile = FF_PROFILE_H264_MAIN;
            else if (!strcasecmp(job->encoder_profile, "high"))
                context->profile = FF_PROFILE_H264_HIGH;
        }
        context->level = FF_LEVEL_UNKNOWN;
        if (job->encoder_level != NULL && *job->encoder_level)
        {
            int i = 1;
            while (hb_h264_level_names[i] != NULL)
            {
                if (!strcasecmp(job->encoder_level, hb_h264_level_names[i]))
                    context->level = hb_h264_level_values[i];
                ++i;
            }
        }
    }

    if (job->vcodec == HB_VCODEC_FFMPEG_VCE_H265)
    {
        // Set profile and level
        context->profile = FF_PROFILE_UNKNOWN;
        if (job->encoder_profile != NULL && *job->encoder_profile)
        {
            if (!strcasecmp(job->encoder_profile, "main"))
                 context->profile = FF_PROFILE_HEVC_MAIN;
        }
        context->level = FF_LEVEL_UNKNOWN;
        if (job->encoder_level != NULL && *job->encoder_level)
        {
            int i = 1;
            while (hb_h265_level_names[i] != NULL)
            {
                if (!strcasecmp(job->encoder_level, hb_h265_level_names[i]))
                    context->level = hb_h265_level_values[i];
                ++i;
            }
        }
        // FIXME
        //context->tier = FF_TIER_UNKNOWN;
    }

    if (job->vcodec == HB_VCODEC_FFMPEG_NVENC_H264 ||
        job->vcodec == HB_VCODEC_FFMPEG_NVENC_H265 ||
        job->vcodec == HB_VCODEC_FFMPEG_NVENC_H265_10BIT)
    {
        // Force IDR frames when we force a new keyframe for chapters
        av_dict_set( &av_opts, "forced-idr", "1", 0 );

        // Set profile and level
        if (job->encoder_profile != NULL && *job->encoder_profile)
        {
            if (!strcasecmp(job->encoder_profile, "baseline"))
                av_dict_set(&av_opts, "profile", "baseline", 0);
            else if (!strcasecmp(job->encoder_profile, "main"))
                av_dict_set(&av_opts, "profile", "main", 0);
            else if (!strcasecmp(job->encoder_profile, "high"))
                av_dict_set(&av_opts, "profile", "high", 0);
        }

        if (job->encoder_level != NULL && *job->encoder_level)
        {
            int i = 1;
            while (hb_h264_level_names[i] != NULL)
            {
                if (!strcasecmp(job->encoder_level, hb_h264_level_names[i]))
                    av_dict_set(&av_opts, "level", job->encoder_level, 0);
                ++i;
            }
        }
    }

    // Make VCE h.265 encoder emit an IDR for every GOP
    if (job->vcodec == HB_VCODEC_FFMPEG_VCE_H265)
    {
        av_dict_set(&av_opts, "gops_per_idr", "1", 0);
    }

    if (job->vcodec == HB_VCODEC_FFMPEG_MF_H264)
    {
        context->profile = FF_PROFILE_UNKNOWN;
        if (job->encoder_profile != NULL && *job->encoder_profile)
        {
            if (!strcasecmp(job->encoder_profile, "baseline"))
                context->profile = FF_PROFILE_H264_BASELINE;
            else if (!strcasecmp(job->encoder_profile, "main"))
                 context->profile = FF_PROFILE_H264_MAIN;
            else if (!strcasecmp(job->encoder_profile, "high"))
                context->profile = FF_PROFILE_H264_HIGH;
        }

    }

    if (job->vcodec == HB_VCODEC_FFMPEG_MF_H264 ||
        job->vcodec == HB_VCODEC_FFMPEG_MF_H265)
    {
        av_dict_set(&av_opts, "hw_encoding", "1", 0);
    }

    if (job->vcodec == HB_VCODEC_FFMPEG_MF_H265)
    {
        // Qualcomm's HEVC encoder does support b-frames. Some chipsets
        // support setting this to either 1 or 2, while others only support
        // setting it to 1.
        context->max_b_frames = 1;
    }

    if( job->pass_id == HB_PASS_ENCODE_1ST ||
        job->pass_id == HB_PASS_ENCODE_2ND )
    {
        char * filename = hb_get_temporary_filename("ffmpeg.log");

        if( job->pass_id == HB_PASS_ENCODE_1ST )
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

    if (job->pass_id == HB_PASS_ENCODE_1ST &&
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
    av_dict_free( &av_opts );

    pv->context = context;

    job->areBframes = 0;
    if (context->has_b_frames > 0)
    {
        job->areBframes = context->has_b_frames;
    }

    if (context->extradata != NULL)
    {
        memcpy(w->config->extradata.bytes, context->extradata,
                                           context->extradata_size);
        w->config->extradata.length = context->extradata_size;
    }

done:
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
            pv->job->config.init_delay = pv->dts_delay;
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
        // libav 12 deprecated context->coded_frame, so we can't determine
        // the exact frame type any more. So until I can completely
        // wire up ffmpeg with AV_PKT_DISPOSABLE_FRAME, all frames
        // must be considered to potentially be reference frames
        out->s.flags     = HB_FLAG_FRAMETYPE_REF;
        out->s.frametype = 0;
        if (pv->pkt->flags & AV_PKT_FLAG_KEY)
        {
            out->s.flags |= HB_FLAG_FRAMETYPE_KEY;
            hb_chapter_dequeue(pv->chapter_queue, out);
        }
        out = process_delay_list(pv, out);

        hb_buffer_list_append(list, out);
        av_packet_unref(pv->pkt);
    }
}

static void Encode( hb_work_object_t *w, hb_buffer_t *in,
                    hb_buffer_list_t *list )
{
    hb_work_private_t * pv = w->private_data;
    AVFrame             frame = {{0}};
    int                 ret;

    frame.width       = in->f.width;
    frame.height      = in->f.height;
    frame.format      = in->f.fmt;
    frame.data[0]     = in->plane[0].data;
    frame.data[1]     = in->plane[1].data;
    frame.data[2]     = in->plane[2].data;
    frame.linesize[0] = in->plane[0].stride;
    frame.linesize[1] = in->plane[1].stride;
    frame.linesize[2] = in->plane[2].stride;

    if (in->s.new_chap > 0 && pv->job->chapter_markers)
    {
        /* chapters have to start with an IDR frame so request that this
           frame be coded as IDR. Since there may be multiple frames
           currently buffered in the encoder remember the timestamp so
           when this frame finally pops out of the encoder we'll mark
           its buffer as the start of a chapter. */
        frame.pict_type = AV_PICTURE_TYPE_I;
        frame.key_frame = 1;
        hb_chapter_enqueue(pv->chapter_queue, in);
    }

    // For constant quality, setting the quality in AVCodecContext
    // doesn't do the trick.  It must be set in the AVFrame.
    frame.quality = pv->context->global_quality;

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

    frame.pts = pv->frameno_in++;

    // Encode
#if HB_PROJECT_FEATURE_NVENC
    if (in->hw_ctx.frame)
    {
        AVFrame *p_frame = in->hw_ctx.frame;
        av_frame_copy_props(p_frame, &frame);
        ret = avcodec_send_frame(pv->context, p_frame);
    }
    else
    {
        ret = avcodec_send_frame(pv->context, &frame);
    }
#else
    ret = avcodec_send_frame(pv->context, &frame);
#endif
    if (ret < 0)
    {
        hb_log("encavcodec: avcodec_send_frame failed");
        return;
    }

    // Write stats
    if (pv->job->pass_id == HB_PASS_ENCODE_1ST &&
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
    if (pv->job->pass_id == HB_PASS_ENCODE_1ST &&
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

    hb_buffer_list_clear(&list);
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        Flush(w, &list);
        hb_buffer_list_append(&list, hb_buffer_eof_init());
        *buf_out = hb_buffer_list_clear(&list);
        return HB_WORK_DONE;
    }

    Encode(w, in, &list);
    *buf_out = hb_buffer_list_clear(&list);

    return HB_WORK_OK;
}

/**
 * Encoder options and presets
 */

static int apply_svt_av1_options(hb_job_t *job, AVCodecContext *context, AVDictionary **av_opts, hb_dict_t *opts)
{
    context->profile = FF_PROFILE_UNKNOWN;
    if (job->encoder_profile != NULL && *job->encoder_profile)
    {
        if (!strcasecmp(job->encoder_profile, "main"))
            context->profile = FF_PROFILE_AV1_MAIN;
        else if (!strcasecmp(job->encoder_profile, "high"))
             context->profile = FF_PROFILE_AV1_HIGH;
        else if (!strcasecmp(job->encoder_profile, "professional"))
            context->profile = FF_PROFILE_AV1_PROFESSIONAL;
    }
    context->level = FF_LEVEL_UNKNOWN;
    if (job->encoder_level != NULL && *job->encoder_level)
    {
        int i = 1;
        while (hb_av1_level_names[i] != NULL)
        {
            if (!strcasecmp(job->encoder_level, hb_av1_level_names[i]))
                context->level = hb_av1_level_values[i];
            ++i;
        }
    }

    if (job->encoder_tune != NULL && !strstr("psnr", job->encoder_tune))
    {
        hb_dict_set_int(opts, "tune", 1);
    }
    else
    {
        hb_dict_set_int(opts, "tune", 0);
    }

    if (job->encoder_tune != NULL && !strstr("fastdecode", job->encoder_tune))
    {
        hb_dict_set_int(opts, "fast-decode", 1);
    }
    else
    {
        hb_dict_set_int(opts, "fast-decode", 0);
    }

    if (job->color_transfer == HB_COLR_TRA_SMPTEST2084)
    {
        // Mastering display metadata.
        if (job->mastering.has_primaries && job->mastering.has_luminance)
        {
            char mastering_display_color_volume[256];
            snprintf(mastering_display_color_volume, sizeof(mastering_display_color_volume),
                     "G(%5.4f,%5.4f)B(%5.4f,%5.4f)R(%5.4f,%5.4f)WP(%5.4f,%5.4f)L(%5.4f,%5.4f)",
                     hb_q2d(job->mastering.display_primaries[1][0]),
                     hb_q2d(job->mastering.display_primaries[1][1]),
                     hb_q2d(job->mastering.display_primaries[2][0]),
                     hb_q2d(job->mastering.display_primaries[2][1]),
                     hb_q2d(job->mastering.display_primaries[0][0]),
                     hb_q2d(job->mastering.display_primaries[0][1]),
                     hb_q2d(job->mastering.white_point[0]),
                     hb_q2d(job->mastering.white_point[1]),
                     hb_q2d(job->mastering.max_luminance),
                     hb_q2d(job->mastering.min_luminance));

            hb_dict_set_string(opts, "mastering-display", mastering_display_color_volume);
        }

        // Content light level.
        if (job->coll.max_cll && job->coll.max_fall)
        {
            char content_light_level[256];
            snprintf(content_light_level, sizeof(content_light_level),
                     "%u,%u", job->coll.max_cll, job->coll.max_fall);

            hb_dict_set_string(opts, "content-light", content_light_level);
        }
    }

    if (hb_dict_get(opts, "compressed-ten-bit-format"))
    {
        hb_log("apply_svt_av1_options [warning]: compressed-ten-bit-format is not supported, disabling");
        hb_dict_remove(opts, "compressed-ten-bit-format");
    }

    char *param_str = hb_value_get_string_xform(opts);
    av_dict_set(av_opts, "svtav1-params", param_str, 0);
    free(param_str);

    return 0;
}

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

    switch (job->vcodec) {
        case HB_VCODEC_FFMPEG_SVT_AV1:
        case HB_VCODEC_FFMPEG_SVT_AV1_10BIT:
            apply_svt_av1_options(job, context, av_opts, lavc_opts);
            break;

        default:
            apply_options(job, context, av_opts, lavc_opts);
            break;
    }

    hb_dict_free(&lavc_opts);

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

static int apply_av1_preset(AVDictionary ** av_opts, const char * preset)
{
    if (preset == NULL)
    {
        av_dict_set( av_opts, "preset", "5", 0);
    }
    else
    {
        av_dict_set( av_opts, "preset", preset, 0);
    }

    return 0;
}

static int apply_encoder_preset(int vcodec, AVDictionary ** av_opts,
                                const char * preset)
{
    switch (vcodec)
    {
        case HB_VCODEC_FFMPEG_VP8:
            return apply_vp8_preset(av_opts, preset);
        case HB_VCODEC_FFMPEG_VP9:
            return apply_vp9_preset(av_opts, preset);
        case HB_VCODEC_FFMPEG_VP9_10BIT:
            return apply_vp9_10bit_preset(av_opts, preset);
        case HB_VCODEC_FFMPEG_SVT_AV1:
        case HB_VCODEC_FFMPEG_SVT_AV1_10BIT:
            return apply_av1_preset(av_opts, preset);

#if HB_PROJECT_FEATURE_NVENC
        case HB_VCODEC_FFMPEG_NVENC_H264:
        case HB_VCODEC_FFMPEG_NVENC_H265:
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
            preset = hb_map_nvenc_preset_name(preset);
            av_dict_set( av_opts, "preset", preset, 0);
            break;
#endif
        default:
            break;
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
            return hb_vce_preset_names;

        case HB_VCODEC_FFMPEG_NVENC_H264:
        case HB_VCODEC_FFMPEG_NVENC_H265:
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
            return h26x_nvenc_preset_names;

        case HB_VCODEC_FFMPEG_MF_H264:
        case HB_VCODEC_FFMPEG_MF_H265:
            return h26x_mf_preset_name;

        case HB_VCODEC_FFMPEG_SVT_AV1:
        case HB_VCODEC_FFMPEG_SVT_AV1_10BIT:
            return av1_svt_preset_names;

        default:
            return NULL;
    }
}

const char* const* hb_av_tune_get_names(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_FFMPEG_SVT_AV1:
        case HB_VCODEC_FFMPEG_SVT_AV1_10BIT:
            return av1_svt_tune_names;

        default:
            return NULL;
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
        case HB_VCODEC_FFMPEG_SVT_AV1:
        case HB_VCODEC_FFMPEG_SVT_AV1_10BIT:
            return av1_svt_profile_names;

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
            return h26x_mf_pix_fmts;

        case HB_VCODEC_FFMPEG_NVENC_H264:
        case HB_VCODEC_FFMPEG_NVENC_H265:
            return nvenc_pix_formats;

        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
            return nvenc_pix_formats_10bit;

        case HB_VCODEC_FFMPEG_VP9_10BIT:
        case HB_VCODEC_FFMPEG_SVT_AV1_10BIT:
            return standard_10bit_pix_fmts;

         default:
             return standard_pix_fmts;
     }
}
