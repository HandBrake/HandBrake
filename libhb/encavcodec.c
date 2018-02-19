/* encavcodec.c

   Copyright (c) 2003-2018 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "hb.h"
#include "hb_dict.h"
#include "hbffmpeg.h"

/*
 * The frame info struct remembers information about each frame across calls
 * to avcodec_encode_video. Since frames are uniquely identified by their
 * frame number, we use this as an index.
 *
 * The size of the array is chosen so that two frames can't use the same
 * slot during the encoder's max frame delay (set by the standard as 16
 * frames) and so that, up to some minimum frame rate, frames are guaranteed
 * to map to * different slots.
 */
#define FRAME_INFO_SIZE 32
#define FRAME_INFO_MASK (FRAME_INFO_SIZE - 1)

struct hb_work_private_s
{
    hb_job_t           * job;
    AVCodecContext     * context;
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

static int apply_encoder_preset(int vcodec, AVCodecContext *context, AVDictionary ** av_opts,
                                const char * preset);

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
    "losslesshp", "lossless", "llhp", "llhq", "ll", "bd", "hq", "hp", "fast", "medium", "slow", "default", NULL
};

static const char * const h264_nvenc_profile_names[] =
{
    "auto", "baseline", "main", "high", "high444p", NULL 
};

static const char * const h265_nvenc_profile_names[] =
{
    "auto", "main", "main10", "rext", NULL 
};
static const char * const h26x_nvenc_level_names[] =
{
    "auto", "1", "2", "3", "4", "5", NULL
};

static const char * const h264_vaapi_profile_names[] = // 66 baseline, 77 main, 100 high
{
    "auto", "66", "77", "100", NULL
};
static const char * const h265_vaapi_profile_names[] =
{
    "auto", "1", "2", "3", "4", "5", NULL
};
static const char * const h26x_vaapi_level_names[] =
{
    "auto", "1", "2", "3", "4", "5", "51", NULL
};

static int _oneShotEncodingLog01 = 1;

int encavcodecInit( hb_work_object_t * w, hb_job_t * job )
{
    int ret = 0;
    char reason[80];
    AVCodec * codec = NULL;
    AVCodecContext * context = NULL;
    AVRational fps;
    int64_t bit_rate_ceiling = -1;
    int64_t bit_rate_avgusr = -1;

    _oneShotEncodingLog01 = 1;

    int oldlevel = av_log_get_level();
    av_log_set_level( AV_LOG_VERBOSE );

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data   = pv;
    pv->job           = job;
    pv->chapter_queue = hb_chapter_queue_init();

    hb_buffer_list_clear(&pv->delay_list);

    int clock_min, clock_max, clock;
    hb_video_framerate_get_limits(&clock_min, &clock_max, &clock);

    switch ( w->codec_param )
    {
        case AV_CODEC_ID_MPEG4:
        {
            hb_log("encavcodecInit: MPEG-4 ASP encoder");
        } break;
        case AV_CODEC_ID_MPEG2VIDEO:
        {
            hb_log("encavcodecInit: MPEG-2 encoder");
        } break;
        case AV_CODEC_ID_VP8:
        {
            switch (job->vcodec) {
                case HB_VCODEC_FFMPEG_VP8:
                    hb_log("encavcodecInit: VP8 encoder");
                    break;
                case HB_VCODEC_FFMPEG_VP8_VAAPI:
                    codec = avcodec_find_encoder_by_name("vp8_vaapi");
                    hb_log("encavcodecInit: VP8 hardware encoder, found vaapi %d", NULL!=codec);
                    break;
            }
        } break;
        case AV_CODEC_ID_VP9:
        {
            switch (job->vcodec) {
                case HB_VCODEC_FFMPEG_VP9:
                    hb_log("encavcodecInit: VP9 encoder");
                    break;
                case HB_VCODEC_FFMPEG_VP9_VAAPI:
                    codec = avcodec_find_encoder_by_name("vp9_vaapi");
                    hb_log("encavcodecInit: VP9 hardware encoder, found vaapi %d", NULL!=codec);
                    break;
            }
        } break;
        case AV_CODEC_ID_H264:
        {
            switch (job->vcodec) {
                case HB_VCODEC_FFMPEG_H264_NVENC:
                    codec = avcodec_find_encoder_by_name("h264_nvenc");
                    hb_log("encavcodecInit: H.264 hardware encoder, found nvenc %d", NULL!=codec);
                    break;
                case HB_VCODEC_FFMPEG_H264_VAAPI:
                    codec = avcodec_find_encoder_by_name("h264_vaapi");
                    hb_log("encavcodecInit: H.264 hardware encoder, found vaapi %d", NULL!=codec);
                    break;
            }
        } break;
        case AV_CODEC_ID_HEVC:
        {
            switch (job->vcodec) {
                case HB_VCODEC_FFMPEG_H265_NVENC:
                    codec = avcodec_find_encoder_by_name("hevc_nvenc");
                    hb_log("encavcodecInit: H.265 hardware encoder, found nvenc %d", NULL!=codec);
                    break;
                case HB_VCODEC_FFMPEG_H265_VAAPI:
                    codec = avcodec_find_encoder_by_name("hevc_vaapi");
                    hb_log("encavcodecInit: H.265 hardware encoder, found vaapi %d", NULL!=codec);
                    break;
            }
        } break;
        default:
        {
            hb_error("encavcodecInit: unsupported encoder!");
            ret = 1;
            goto done;
        }
    }

    if( NULL == codec ) 
    {
        codec = avcodec_find_encoder( w->codec_param  );
        hb_log("encavcodecInit: default encoder, found %d", NULL!=codec);
    }
    if( NULL == codec ) 
    {
        hb_log( "encavcodecInit: avcodec_find_encoder "
                "failed" );
        ret = 1;
        goto done;
    }
    context = avcodec_alloc_context3( codec );

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
    context->gop_size  = ((double)job->orig_vrate.num / job->orig_vrate.den +
                                  0.5) * 10;

    /* place job->encoder_options in an hb_dict_t for convenience */
    hb_dict_t * lavc_opts = NULL;
    if (job->encoder_options != NULL && *job->encoder_options)
    {
        lavc_opts = hb_encopts_to_dict(job->encoder_options, job->vcodec);
    }
    
    bit_rate_ceiling = (int64_t)job->width * (int64_t)job->height * (int64_t)fps.num / (int64_t)fps.den;
    bit_rate_avgusr = ( 0 < job->vbitrate ) ? 1000 * (int64_t)job->vbitrate : -1;
    hb_log( "encavcodec: bit_rate.0 ceiling %ld by %d x %d * %d/%d; user %ld", bit_rate_ceiling, job->width, job->height, fps.num, fps.den, bit_rate_avgusr);

    if (job->vquality != HB_INVALID_VIDEO_QUALITY)
    {
        if ( w->codec_param == AV_CODEC_ID_VP8 ||
             w->codec_param == AV_CODEC_ID_VP9 ||
             w->codec_param == AV_CODEC_ID_H264 ||
             w->codec_param == AV_CODEC_ID_HEVC 
           )
        {
            //This value was chosen to make the bitrate high enough
            //for libvpx to "turn off" the maximum bitrate feature
            //that is normally applied to constant quality.
            context->bit_rate = bit_rate_ceiling;
            hb_log( "encavcodec: bit_rate.1 %ld", context->bit_rate);
        }
    }

    AVDictionary * av_opts = NULL;
    if (apply_encoder_preset(job->vcodec, context, &av_opts, job->encoder_preset))
    {
        ret = 1;
        goto done;
    }
    if (job->encoder_profile && *job->encoder_profile && 0!=strcmp("auto", job->encoder_profile) )
    {
        if ( w->codec_param == AV_CODEC_ID_H264 ||
             w->codec_param == AV_CODEC_ID_HEVC )
        {
            av_dict_set( &av_opts, "profile", job->encoder_profile, 0 );
        }
    }
    if (job->encoder_level && *job->encoder_level && 0!=strcmp("auto", job->encoder_level) )
    {
        if ( w->codec_param == AV_CODEC_ID_H264 ||
             w->codec_param == AV_CODEC_ID_HEVC )
        {
            av_dict_set( &av_opts, "level", job->encoder_level, 0 );
        }
    }

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
        av_dict_set( &av_opts, key, str, 0 );
        free(str);
    }
    hb_dict_free( &lavc_opts );

    // Now set the things in context that we don't want to allow
    // the user to override.
    if (job->vquality <= HB_INVALID_VIDEO_QUALITY)
    {
        /* Average bitrate */
        context->bit_rate = bit_rate_avgusr;
        hb_log( "encavcodec: bit_rate.2 %ld", context->bit_rate);
        // ffmpeg's mpeg2 encoder requires that the bit_rate_tolerance be >=
        // bitrate * fps
        context->bit_rate_tolerance = context->bit_rate * av_q2d(fps) + 1;
    }
    else
    {
        /* Constant quantizer */
        // These settings produce better image quality than
        // what was previously used
        context->flags |= AV_CODEC_FLAG_QSCALE;
        context->global_quality = FF_QP2LAMBDA * job->vquality + 0.5;
        //Set constant quality for libvpx
        if ( w->codec_param == AV_CODEC_ID_VP8 ||
             w->codec_param == AV_CODEC_ID_VP9 )
        {
            char quality[7];
            snprintf(quality, 7, "%.2f", job->vquality);
            av_dict_set( &av_opts, "crf", quality, 0 );
            //This value was chosen to make the bitrate high enough
            //for libvpx to "turn off" the maximum bitrate feature
            //that is normally applied to constant quality.
            context->bit_rate = bit_rate_ceiling;
            hb_log( "encavcodec: bit_rate.3 %ld by %d x %d * %d/%d (user: %d kbps))", context->bit_rate, job->width, job->height, fps.num, fps.den, job->vbitrate);
            hb_log( "encavcodec: encoding at CQ %.2f", job->vquality );
        }
        //Set constant quality for nvenc
        else if ( job->vcodec == HB_VCODEC_FFMPEG_H264_NVENC ||
                  job->vcodec == HB_VCODEC_FFMPEG_H265_NVENC )
        {
            char quality[7];
            snprintf(quality, 7, "%.0f", job->vquality);
            av_dict_set( &av_opts, "rc", "vbr", 0 );
            av_dict_set( &av_opts, "cq", quality, 0 );

            // further Advanced Quality Settings in Constant Quality Mode
            av_dict_set( &av_opts, "init_qpP", "1", 0 );
            av_dict_set( &av_opts, "init_qpB", "1", 0 );
            av_dict_set( &av_opts, "init_qpI", "1", 0 );
            av_dict_set( &av_opts, "rc-lookahead", "16", 0 ); // also adds b-frames (h264 only it seems for now), max 32 causes errors
            if( job->vcodec == HB_VCODEC_FFMPEG_H265_NVENC ) {
                av_dict_set( &av_opts, "spatial_aq", "1", 0 ); // oops, nvenc_hevc.c uses an underscore
            } else {
                av_dict_set( &av_opts, "spatial-aq", "1", 0 ); // oops, nvenc_h264.c uses a dash
            }
            // av_dict_set( &av_opts, "temporal_aq", "1", 0 ); // only for h264, either spatial or temporal
            // av_dict_set( &av_opts, "aq-strength", "8", 0 ); // use default value
            hb_log( "encavcodec: encoding at rc=vbr CQ %.0f, init_qp 1, rc-lookahead 16, spatial_aq 1, aq-strength default", job->vquality );

            //This value was chosen to make the bitrate high enough
            //for nvenc to "turn off" the maximum bitrate feature
            //that is normally applied to constant quality.
            context->bit_rate = bit_rate_ceiling;
            hb_log( "encavcodec: bit_rate.4 %ld", context->bit_rate);
        }
        //Set constant quality for vaapi
        else if(HB_VCODEC_IS_VAAPI(job->vcodec)) 
        {
            char quality[7];
            snprintf(quality, 7, "%.0f", job->vquality);
            if ( job->vcodec == HB_VCODEC_FFMPEG_H264_VAAPI ||
                 job->vcodec == HB_VCODEC_FFMPEG_H265_VAAPI ) {
            	// h264 and hevc
				av_dict_set( &av_opts, "qp", quality, 0 );
				hb_log( "encavcodec: encoding at qp %.0f", job->vquality );
            } else {
            	// VP8 and VP9
				av_dict_set( &av_opts, "q", quality, 0 );
				hb_log( "encavcodec: encoding at q %.0f", job->vquality );
            }
            // av_dict_set(&av_opts, "bf", "0", 0); // FIXME: VAAPI 'B frames are not supported"

            //This value was chosen to make the bitrate high enough
            //for nvenc to "turn off" the maximum bitrate feature
            //that is normally applied to constant quality.
            context->bit_rate = bit_rate_ceiling;
            hb_log( "encavcodec: bit_rate.4 %ld", context->bit_rate);
        }
        else
        {
            hb_log( "encavcodec: encoding at constant quantizer %d",
                    context->global_quality );
        }
    }
    context->width     = job->width;
    context->height    = job->height;

    if(HB_VCODEC_IS_VAAPI(job->vcodec)) {
        int err;
        context->pix_fmt   = AV_PIX_FMT_VAAPI;
        if((err = hb_avcodec_vaapi_set_hwframe_ctx(context, 0, 20))<0) {
            hb_log("Failed to set the VAAPI hwframe_ctx. Error code: %s", av_err2str(err));
            ret = 1;
            goto done;
        }
    } else {
        context->pix_fmt   = AV_PIX_FMT_YUV420P;
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

    if( job->mux & HB_MUX_MASK_MP4 )
    {
        context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    if( job->grayscale )
    {
        context->flags |= AV_CODEC_FLAG_GRAY;
    }

    if( job->pass_id == HB_PASS_ENCODE_1ST ||
        job->pass_id == HB_PASS_ENCODE_2ND )
    {
        char filename[1024]; memset( filename, 0, 1024 );
        hb_get_tempory_filename( job->h, filename, "ffmpeg.log" );

        if( job->pass_id == HB_PASS_ENCODE_1ST )
        {
            pv->file = hb_fopen(filename, "wb");
            if (!pv->file) {
                if (strerror_r(errno, reason, 79) != 0)
                    strcpy(reason, "unknown -- strerror_r() failed");

                hb_error("encavcodecInit: Failed to open %s (reason: %s)", filename, reason);
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
    }

    {
        AVDictionaryEntry *t = NULL;
        while( ( t = av_dict_get( av_opts, "", t, AV_DICT_IGNORE_SUFFIX ) ) )
        {
            hb_log( "encavcodecInit: Trying avcodec option %s = %s", t->key, t->value );
        }
    }

    if (hb_avcodec_open(context, codec, &av_opts, HB_FFMPEG_THREADS_AUTO))
    {
        hb_log( "encavcodecInit: avcodec_open failed" );
    }

    if (job->pass_id == HB_PASS_ENCODE_1ST &&
        context->stats_out != NULL)
    {
        // Some encoders may write stats during init in avcodec_open
        fprintf(pv->file, "%s", context->stats_out);
    }

    {
        // avcodec_open populates the opts dictionary with the
        // things it didn't recognize.
        AVDictionaryEntry *t = NULL;
        while( ( t = av_dict_get( av_opts, "", t, AV_DICT_IGNORE_SUFFIX ) ) )
        {
            hb_log( "encavcodecInit: Unknown avcodec option %s ( = %s )", t->key, t->value );
        }
    }
    av_dict_free( &av_opts );

    pv->context = context;

    job->areBframes = 0;
    if ( context->has_b_frames )
    {
        job->areBframes = 1;
    }
    if( ( job->mux & HB_MUX_MASK_MP4 ) && job->pass_id != HB_PASS_ENCODE_1ST )
    {
        w->config->mpeg4.length = context->extradata_size;
        memcpy( w->config->mpeg4.bytes, context->extradata,
                context->extradata_size );
    }

done:
    if( 0 != ret ) {
		if( NULL != context ) {
			if( NULL != codec ) {
				avcodec_flush_buffers( context );
			}
			avcodec_free_context( &context );
		}
		if( NULL != av_opts ) {
			av_dict_free( &av_opts );
		}
    } else {
        hb_log( "encavcodecInit: avcodec_is_open %d, av_codec_is_encoder %d",
        		avcodec_is_open(context), av_codec_is_encoder(codec));
    }
    // av_log_set_level( oldlevel );
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
// delay before it has suffecient information to decode). The number of
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
        AVPacket      pkt;
        hb_buffer_t * out;

        av_init_packet(&pkt);
        ret = avcodec_receive_packet(pv->context, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            break;
        }
        if (ret < 0)
        {
            hb_log("encavcodec: avcodec_receive_packet failed");
        }
        out = hb_buffer_init(pkt.size);
        memcpy(out->data, pkt.data, out->size);

        int64_t frameno = pkt.pts;
        out->size       = pkt.size;
        out->s.start    = get_frame_start(pv, frameno);
        out->s.duration = get_frame_duration(pv, frameno);
        out->s.stop     = out->s.stop + out->s.duration;
        // libav 12 deprecated context->coded_frame, so we can't determine
        // the exact frame type any more. So until I can completely
        // wire up ffmpeg with AV_PKT_DISPOSABLE_FRAME, all frames
        // must be considered to potentially be reference frames
        out->s.flags     = HB_FLAG_FRAMETYPE_REF;
        out->s.frametype = 0;
        if (pkt.flags & AV_PKT_FLAG_KEY)
        {
            out->s.flags |= HB_FLAG_FRAMETYPE_KEY;
            hb_chapter_dequeue(pv->chapter_queue, out);
        }
        out = process_delay_list(pv, out);

        hb_buffer_list_append(list, out);
        av_packet_unref(&pkt);
    }
}

static void Encode( hb_work_object_t *w, hb_buffer_t *in,
                    hb_buffer_list_t *list )
{
    hb_work_private_t * pv = w->private_data;
    AVFrame             frame = {0};
    AVFrame             *hw_frame = NULL;
    int                 ret;

    frame.width = pv->context->width;
    frame.height = pv->context->height;
    if( AV_PIX_FMT_VAAPI == pv->context->pix_fmt ) {
    	if( _oneShotEncodingLog01 ) {
    		hb_log("encavcodec.Encode.0: AV_PIX_FMT_YUV420P -> AV_PIX_FMT_VAAPI");
    	}
    	frame.format = AV_PIX_FMT_YUV420P; // AV_PIX_FMT_YUV420P AV_PIX_FMT_NV12
    } else {
    	frame.format = pv->context->pix_fmt;
    }

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
        hb_chapter_enqueue(pv->chapter_queue, in);
    }

    // For constant quality, setting the quality in AVCodecContext
    // doesn't do the trick.  It must be set in the AVFrame.
    frame.quality = pv->context->global_quality;

    // Remember info about this frame that we need to pass across
    // the avcodec_encode_video call (since it reorders frames).
    save_frame_info(pv, in);
    compute_dts_offset(pv, in);

    // Bizarro ffmpeg appears to require the input AVFrame.pts to be
    // set to a frame number.  Setting it to an actual pts causes
    // jerky video.
    // frame->pts = in->s.start;
    frame.pts = pv->frameno_in++;

    if( NULL != pv->context->hw_frames_ctx ) {
		if (!(hw_frame = av_frame_alloc())) {
			hb_error("encavcodec.Encode: Couldn't allocate hw_frame");
			goto close;
		}
		if ((ret = av_hwframe_get_buffer(pv->context->hw_frames_ctx, hw_frame, 0)) < 0) {
			hb_error("encavcodec.Encode: Error while allocating hw_frame data buffers: %s", av_err2str(ret));
			goto close;
		}
    	if( _oneShotEncodingLog01 ) {
    		hb_log("encavcodec.Encode.0: transfer frame -> hw_frame");
    		fflush(NULL);
    	}
    	hw_frame->pts = frame.pts;
		if ((ret = av_hwframe_transfer_data(hw_frame, &frame, 0)) < 0) {
			hb_error("encavcodec.Encode: Error while transferring frame data to hw_frame: %s.", av_err2str(ret));
			goto close;
		}
	    // Encode
    	if( _oneShotEncodingLog01 ) {
    		hb_log("encavcodec.Encode.0: send hw_frame, same pts %d", (hw_frame->pts == frame.pts));
    		fflush(NULL);
    	}
	    ret = avcodec_send_frame(pv->context, hw_frame);
    } else {
    	// Encode
    	ret = avcodec_send_frame(pv->context, &frame);
    }
    if (ret < 0)
    {
        hb_log("encavcodec: avcodec_send_frame failed, error code: %s",av_err2str(ret));
        goto close;
    }

    // Write stats
    if (pv->job->pass_id == HB_PASS_ENCODE_1ST &&
        pv->context->stats_out != NULL)
    {
        fprintf( pv->file, "%s", pv->context->stats_out );
    }

    get_packets(w, list);

close:
	_oneShotEncodingLog01 = 0;
    if( NULL != hw_frame ) {
    	av_frame_free(&hw_frame);
    }
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

static int apply_encoder_preset(int vcodec, AVCodecContext *context, AVDictionary ** av_opts,
                                const char * preset)
{
    switch (vcodec)
    {
        case HB_VCODEC_FFMPEG_VP8:
        case HB_VCODEC_FFMPEG_VP9:
            return apply_vpx_preset(av_opts, preset);
        case HB_VCODEC_FFMPEG_H264_NVENC:
        case HB_VCODEC_FFMPEG_H265_NVENC:
        case HB_VCODEC_FFMPEG_H264_VAAPI:
        case HB_VCODEC_FFMPEG_H265_VAAPI:
        case HB_VCODEC_FFMPEG_VP8_VAAPI:
        case HB_VCODEC_FFMPEG_VP9_VAAPI:
            av_dict_set( av_opts, "preset", preset, 0);
            break;
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
            return vpx_preset_names;

        case HB_VCODEC_FFMPEG_H264_NVENC:
        case HB_VCODEC_FFMPEG_H265_NVENC:
            return h26x_nvenc_preset_names;

        default:
            return NULL;
    }
}

const char* const* hb_av_profile_get_names(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_FFMPEG_H264_NVENC:
            return h264_nvenc_profile_names;
        case HB_VCODEC_FFMPEG_H265_NVENC:
            return h265_nvenc_profile_names;

        case HB_VCODEC_FFMPEG_H264_VAAPI:
            return h264_vaapi_profile_names;
        case HB_VCODEC_FFMPEG_H265_VAAPI:
            return h265_vaapi_profile_names;

        default:
            return NULL;
    }
}

const char* const* hb_av_level_get_names(int encoder)
{
    switch (encoder)
    {
        case HB_VCODEC_FFMPEG_H264_NVENC:
        case HB_VCODEC_FFMPEG_H265_NVENC:
            return h26x_nvenc_level_names;

        case HB_VCODEC_FFMPEG_H264_VAAPI:
        case HB_VCODEC_FFMPEG_H265_VAAPI:
            return h26x_vaapi_level_names;

        default:
            return NULL;
    }
}

int hb_av_encoder_present(int encoder)
{
    int err;
    AVCodec *codec;
    enum AVPixelFormat fmt;
    switch (encoder)
    {
        case HB_VCODEC_FFMPEG_H264_NVENC:
            codec = avcodec_find_encoder_by_name("h264_nvenc");
            fmt = AV_PIX_FMT_YUV420P;
            break;
        case HB_VCODEC_FFMPEG_H265_NVENC:
            codec = avcodec_find_encoder_by_name("hevc_nvenc");
            fmt = AV_PIX_FMT_YUV420P;
            break;
        case HB_VCODEC_FFMPEG_H264_VAAPI:
            codec = avcodec_find_encoder_by_name("h264_vaapi");
            fmt = AV_PIX_FMT_VAAPI;
            break;
        case HB_VCODEC_FFMPEG_H265_VAAPI:
            codec = avcodec_find_encoder_by_name("hevc_vaapi");
            fmt = AV_PIX_FMT_VAAPI;
            break;
        case HB_VCODEC_FFMPEG_VP8_VAAPI:
            codec = avcodec_find_encoder_by_name("vp8_vaapi");
            fmt = AV_PIX_FMT_VAAPI;
            break;
        case HB_VCODEC_FFMPEG_VP9_VAAPI:
            codec = avcodec_find_encoder_by_name("vp9_vaapi");
            fmt = AV_PIX_FMT_VAAPI;
            break;
        default:
            codec = NULL;
            fmt = AV_PIX_FMT_YUV420P;
    }
    err = hb_avcodec_test_encoder(codec, fmt);
    hb_log("hb_av_encoder_present: test 0x%X, res %d", encoder, err);
    return 0 == err;
}


