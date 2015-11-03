/* encavcodec.c

   Copyright (c) 2003-2015 HandBrake Team
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
    hb_job_t * job;
    AVCodecContext * context;
    FILE * file;

    int frameno_in;
    int frameno_out;
    hb_buffer_list_t delay_list;

    int64_t dts_delay;

    struct {
        int64_t start;
        int64_t duration;
    } frame_info[FRAME_INFO_SIZE];
};

int  encavcodecInit( hb_work_object_t *, hb_job_t * );
int  encavcodecWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encavcodecClose( hb_work_object_t * );

hb_work_object_t hb_encavcodec =
{
    WORK_ENCAVCODEC,
    "FFMPEG encoder (libavcodec)",
    encavcodecInit,
    encavcodecWork,
    encavcodecClose
};

int encavcodecInit( hb_work_object_t * w, hb_job_t * job )
{
    AVCodec * codec;
    AVCodecContext * context;
    AVRational fps;

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;
    pv->job = job;

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
            hb_log("encavcodecInit: VP8 encoder");
        } break;
        default:
        {
            hb_error("encavcodecInit: unsupported encoder!");
            return 1;
        }
    }

    codec = avcodec_find_encoder( w->codec_param  );
    if( !codec )
    {
        hb_log( "encavcodecInit: avcodec_find_encoder "
                "failed" );
        return 1;
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
    /* iterate through lavc_opts and have avutil parse the options for us */
    AVDictionary * av_opts = NULL;
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
    if( job->vquality < 0.0 )
    {
        /* Average bitrate */
        context->bit_rate = 1000 * job->vbitrate;
        // ffmpeg's mpeg2 encoder requires that the bit_rate_tolerance be >=
        // bitrate * fps
        context->bit_rate_tolerance = context->bit_rate * av_q2d(fps) + 1;
    }
    else
    {
        /* Constant quantizer */
        // These settings produce better image quality than
        // what was previously used
        context->flags |= CODEC_FLAG_QSCALE;
        context->global_quality = FF_QP2LAMBDA * job->vquality + 0.5;
        //Set constant quality for libvpx
        if ( w->codec_param == AV_CODEC_ID_VP8 )
        {
            char quality[7];
            snprintf(quality, 7, "%.2f", job->vquality);
            av_dict_set( &av_opts, "crf", quality, 0 );
            //Setting the deadline to good and cpu-used to 0
            //causes the encoder to balance video quality and
            //encode time, with a bias to video quality.
            av_dict_set( &av_opts, "deadline", "good", 0);
            av_dict_set( &av_opts, "cpu-used", "0", 0);
            //This value was chosen to make the bitrate high enough
            //for libvpx to "turn off" the maximum bitrate feature
            //that is normally applied to constant quality.
            context->bit_rate = job->width * job->height * fps.num / fps.den;
            hb_log( "encavcodec: encoding at CQ %.2f", job->vquality );
        }
        else
        {
            hb_log( "encavcodec: encoding at constant quantizer %d",
                    context->global_quality );
        }
    }
    context->width     = job->width;
    context->height    = job->height;
    context->pix_fmt   = AV_PIX_FMT_YUV420P;

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
        context->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    if( job->grayscale )
    {
        context->flags |= CODEC_FLAG_GRAY;
    }

    if( job->pass_id == HB_PASS_ENCODE_1ST ||
        job->pass_id == HB_PASS_ENCODE_2ND )
    {
        char filename[1024]; memset( filename, 0, 1024 );
        hb_get_tempory_filename( job->h, filename, "ffmpeg.log" );

        if( job->pass_id == HB_PASS_ENCODE_1ST )
        {
            pv->file = hb_fopen(filename, "wb");
            context->flags |= CODEC_FLAG_PASS1;
        }
        else
        {
            int    size;
            char * log;

            pv->file = hb_fopen(filename, "rb");
            fseek( pv->file, 0, SEEK_END );
            size = ftell( pv->file );
            fseek( pv->file, 0, SEEK_SET );
            log = malloc( size + 1 );
            log[size] = '\0';
            fread( log, size, 1, pv->file );
            fclose( pv->file );
            pv->file = NULL;

            context->flags    |= CODEC_FLAG_PASS2;
            context->stats_in  = log;
        }
    }

    if (hb_avcodec_open(context, codec, &av_opts, HB_FFMPEG_THREADS_AUTO))
    {
        hb_log( "encavcodecInit: avcodec_open failed" );
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

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void encavcodecClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if( pv->context && pv->context->codec )
    {
        hb_deep_log( 2, "encavcodec: closing libavcodec" );
        avcodec_flush_buffers( pv->context );
        hb_avcodec_close( pv->context );
        av_free( pv->context );
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
            pv->job->config.h264.init_delay = pv->dts_delay;
        }
    }
}

static uint8_t convert_pict_type( int pict_type, char pkt_flag_key, uint16_t* sflags )
{
    uint8_t retval = 0;
    switch ( pict_type )
    {
        case AV_PICTURE_TYPE_P:
        {
            retval = HB_FRAME_P;
        } break;

        case AV_PICTURE_TYPE_B:
        {
            retval = HB_FRAME_B;
        } break;

        case AV_PICTURE_TYPE_S:
        {
            retval = HB_FRAME_P;
        } break;

        case AV_PICTURE_TYPE_SP:
        {
            retval = HB_FRAME_P;
        } break;

        case AV_PICTURE_TYPE_BI:
        case AV_PICTURE_TYPE_SI:
        case AV_PICTURE_TYPE_I:
        {
            *sflags |= HB_FRAME_REF;
            if ( pkt_flag_key )
            {
                retval = HB_FRAME_IDR;
            }
            else
            {
                retval = HB_FRAME_I;
            }
        } break;

        default:
        {
            if ( pkt_flag_key )
            {
                //buf->s.flags |= HB_FRAME_REF;
                *sflags |= HB_FRAME_REF;
                retval = HB_FRAME_KEY;
            }
            else
            {
                retval = HB_FRAME_REF;
            }
        } break;
    }
    return retval;
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

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int encavcodecWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                    hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t * job = pv->job;
    AVFrame  * frame;
    hb_buffer_t * in = *buf_in, * buf;
    hb_buffer_list_t list;
    char final_flushing_call = !!(in->s.flags & HB_BUF_FLAG_EOF);

    hb_buffer_list_clear(&list);
    if (final_flushing_call)
    {
        /* EOF on input - send it downstream & say we're done */
        // make a flushing call to encode for codecs that can encode
        // out of order
        frame = NULL;
    }
    else
    {
        frame              = av_frame_alloc();
        frame->data[0]     = in->plane[0].data;
        frame->data[1]     = in->plane[1].data;
        frame->data[2]     = in->plane[2].data;
        frame->linesize[0] = in->plane[0].stride;
        frame->linesize[1] = in->plane[1].stride;
        frame->linesize[2] = in->plane[2].stride;

        // For constant quality, setting the quality in AVCodecContext 
        // doesn't do the trick.  It must be set in the AVFrame.
        frame->quality = pv->context->global_quality;

        // Remember info about this frame that we need to pass across
        // the avcodec_encode_video call (since it reorders frames).
        save_frame_info( pv, in );
        compute_dts_offset( pv, in );

        // Bizarro ffmpeg appears to require the input AVFrame.pts to be
        // set to a frame number.  Setting it to an actual pts causes
        // jerky video.
        // frame->pts = in->s.start;
        frame->pts = pv->frameno_in++;
    }

    if ( pv->context->codec )
    {
        int ret;
        AVPacket pkt;
        int got_packet;
        char still_flushing = final_flushing_call;
        do
        {
            av_init_packet(&pkt);
            /* Should be way too large */
            buf = hb_video_buffer_init(job->width, job->height);
            pkt.data = buf->data;
            pkt.size = buf->alloc;

            ret = avcodec_encode_video2( pv->context, &pkt, frame, &got_packet );
            if ( ret < 0 || pkt.size <= 0 || !got_packet )
            {
                hb_buffer_close( &buf );
                still_flushing = 0;
            }
            else
            {
                int64_t frameno = pkt.pts;
                buf->size       = pkt.size;
                buf->s.start    = get_frame_start( pv, frameno );
                buf->s.duration = get_frame_duration( pv, frameno );
                buf->s.stop     = buf->s.stop + buf->s.duration;
                buf->s.flags   &= ~HB_FRAME_REF;
                buf->s.frametype = convert_pict_type( pv->context->coded_frame->pict_type, pkt.flags & AV_PKT_FLAG_KEY, &buf->s.flags );
                buf = process_delay_list( pv, buf );

                hb_buffer_list_append(&list, buf);
            }
            /* Write stats */
            if (job->pass_id == HB_PASS_ENCODE_1ST &&
                pv->context->stats_out != NULL)
            {
                fprintf( pv->file, "%s", pv->context->stats_out );
            }
        } while (still_flushing);

        if (final_flushing_call)
        {
            *buf_in = NULL;
            hb_buffer_list_append(&list, in);
        }
    }
    else
    {
        hb_error( "encavcodec: codec context has uninitialized codec; skipping frame" );
    }

    av_frame_free( &frame );

    *buf_out = hb_buffer_list_clear(&list);
    return final_flushing_call? HB_WORK_DONE : HB_WORK_OK;
}


