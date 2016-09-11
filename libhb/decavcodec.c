/* decavcodec.c

   Copyright (c) 2003-2016 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/* This module is Handbrake's interface to the ffmpeg decoder library
   (libavcodec & small parts of libavformat). It contains four Handbrake
   "work objects":

    decavcodeca connects HB to an ffmpeg audio decoder
    decavcodecv connects HB to an ffmpeg video decoder

        (Two different routines are needed because the ffmpeg library
        has different decoder calling conventions for audio & video.
        These work objects are self-contained & follow all
        of HB's conventions for a decoder module. They can be used like
        any other HB decoder

    These decoders handle 2 kinds of input.  Streams that are demuxed
    by HandBrake and streams that are demuxed by libavformat.  In the
    case of streams that are demuxed by HandBrake, there is an extra
    parse step required that happens in decodeVideo and decavcodecaWork.
    In the case of streams that are demuxed by libavformat, there is context
    information that we need from the libavformat.  This information is
    propagated from hb_stream_open to these decoders through title->opaque_priv.

    A consequence of the above is that the streams that are demuxed by HandBrake
    *can't* use information from the AVStream because there isn't one - they
    get their data from either the dvd reader or the mpeg reader, not the ffmpeg
    stream reader. That means that they have to make up for deficiencies in the
    AVCodecContext info by using stuff kept in the HB "title" struct. It
    also means that ffmpeg codecs that randomly scatter state needed by
    the decoder across both the AVCodecContext & the AVStream (e.g., the
    VC1 decoder) can't easily be used by the HB mpeg stream reader.
 */

#include "hb.h"
#include "hbffmpeg.h"
#include "audio_resample.h"

#ifdef USE_QSV
#include "qsv_common.h"
#endif

static void compute_frame_duration( hb_work_private_t *pv );
static int  decavcodecaInit( hb_work_object_t *, hb_job_t * );
static int  decavcodecaWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
static void decavcodecClose( hb_work_object_t * );
static int decavcodecaInfo( hb_work_object_t *, hb_work_info_t * );
static int decavcodecaBSInfo( hb_work_object_t *, const hb_buffer_t *, hb_work_info_t * );

static int get_color_prim(int color_primaries, hb_geometry_t geometry, hb_rational_t rate);
static int get_color_transfer(int color_trc);
static int get_color_matrix(int colorspace, hb_geometry_t geometry);

hb_work_object_t hb_decavcodeca =
{
    .id = WORK_DECAVCODEC,
    .name = "Audio decoder (libavcodec)",
    .init = decavcodecaInit,
    .work = decavcodecaWork,
    .close = decavcodecClose,
    .info = decavcodecaInfo,
    .bsinfo = decavcodecaBSInfo
};

typedef struct
{
    uint8_t              * data;
    int                    size;
    int64_t                pts;
    int64_t                dts;
    int                    frametype;
    int                    scr_sequence;
    int                    new_chap;
} packet_info_t;

typedef struct reordered_data_s reordered_data_t;

struct reordered_data_s
{
    int64_t                sequence;
    int64_t                pts;
    int                    scr_sequence;
    int                    new_chap;
};

#define REORDERED_HASH_SZ   (2 << 7)
#define REORDERED_HASH_MASK (REORDERED_HASH_SZ - 1)

struct hb_work_private_s
{
    hb_job_t             * job;
    hb_title_t           * title;
    AVCodecContext       * context;
    AVCodecParserContext * parser;
    AVFrame              * frame;
    hb_buffer_t          * palette;
    int                    threads;
    int                    video_codec_opened;
    hb_buffer_list_t       list;
    double                 duration;        // frame duration (for video)
    double                 field_duration;  // field duration (for video)
    int64_t                chap_time;       // time of next chap mark
    int                    chap_scr;
    int                    new_chap;        // output chapter mark pending
    int64_t                last_pts;
    double                 next_pts;
    uint32_t               nframes;
    uint32_t               decode_errors;
    packet_info_t          packet_info;
    uint8_t                unfinished;
    reordered_data_t     * reordered_hash[REORDERED_HASH_SZ];
    int64_t                sequence;
    int                    last_scr_sequence;
    int                    last_chapter;
    struct SwsContext    * sws_context; // if we have to rescale or convert color space

    int                    sws_width;
    int                    sws_height;
    int                    sws_pix_fmt;

    hb_audio_t           * audio;
    hb_audio_resample_t  * resample;

#ifdef USE_QSV
    // QSV-specific settings
    struct
    {
        int                decode;
        av_qsv_config      config;
        const char       * codec_name;
    } qsv;
#endif

    hb_list_t            * list_subtitle;
};

static void decodeAudio( hb_work_private_t *pv, packet_info_t * packet_info );

/***********************************************************************
 * hb_work_decavcodec_init
 ***********************************************************************
 *
 **********************************************************************/
static int decavcodecaInit( hb_work_object_t * w, hb_job_t * job )
{
    AVCodec * codec;

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job       = job;
    pv->audio     = w->audio;
    pv->next_pts  = (int64_t)AV_NOPTS_VALUE;
    if (job)
        pv->title = job->title;
    else
        pv->title = w->title;
    hb_buffer_list_clear(&pv->list);

    codec       = avcodec_find_decoder(w->codec_param);
    pv->context = avcodec_alloc_context3(codec);

    if (pv->title->opaque_priv != NULL)
    {
        AVFormatContext *ic = (AVFormatContext*)pv->title->opaque_priv;
        avcodec_copy_context(pv->context, ic->streams[w->audio->id]->codec);
        // libav's eac3 parser toggles the codec_id in the context as
        // it reads eac3 data between AV_CODEC_ID_AC3 and AV_CODEC_ID_EAC3.
        // It detects an AC3 sync pattern sometimes in ac3_sync() which
        // causes it to eventually set avctx->codec_id to AV_CODEC_ID_AC3
        // in ff_aac_ac3_parse(). Since we are parsing some data before
        // we get here, the codec_id may have flipped.  This will cause an
        // error in hb_avcodec_open().  So flip it back!
        pv->context->codec_id = w->codec_param;
    }
    else
    {
        pv->parser = av_parser_init(w->codec_param);
    }
    hb_ff_set_sample_fmt(pv->context, codec, AV_SAMPLE_FMT_FLT);

    /* Downmixing & sample_fmt conversion */
    if (!(w->audio->config.out.codec & HB_ACODEC_PASS_FLAG))
    {
        pv->resample =
            hb_audio_resample_init(AV_SAMPLE_FMT_FLT,
                                   w->audio->config.out.mixdown,
                                   w->audio->config.out.normalize_mix_level);
        if (pv->resample == NULL)
        {
            hb_error("decavcodecaInit: hb_audio_resample_init() failed");
            return 1;
        }
        /*
         * Some audio decoders can downmix using embedded coefficients,
         * or dedicated audio substreams for a specific channel layout.
         *
         * But some will e.g. use normalized mix coefficients unconditionally,
         * so we need to make sure this matches what the user actually requested.
         */
        int avcodec_downmix = 0;
        switch (w->codec_param)
        {
            case AV_CODEC_ID_AC3:
            case AV_CODEC_ID_EAC3:
                avcodec_downmix = w->audio->config.out.normalize_mix_level != 0;
                break;
            case AV_CODEC_ID_DTS:
                avcodec_downmix = w->audio->config.out.normalize_mix_level == 0;
                break;
            case AV_CODEC_ID_TRUEHD:
                avcodec_downmix = (w->audio->config.out.normalize_mix_level == 0     ||
                                   w->audio->config.out.mixdown == HB_AMIXDOWN_MONO  ||
                                   w->audio->config.out.mixdown == HB_AMIXDOWN_DOLBY ||
                                   w->audio->config.out.mixdown == HB_AMIXDOWN_DOLBYPLII);
                break;
            default:
                break;
        }
        if (avcodec_downmix)
        {
            switch (w->audio->config.out.mixdown)
            {
                // request 5.1 before downmixing to dpl1/dpl2
                case HB_AMIXDOWN_DOLBY:
                case HB_AMIXDOWN_DOLBYPLII:
                    pv->context->request_channel_layout = AV_CH_LAYOUT_5POINT1;
                    break;
                // request the layout corresponding to the selected mixdown
                default:
                    pv->context->request_channel_layout =
                        hb_ff_mixdown_xlat(w->audio->config.out.mixdown, NULL);
                    break;
            }
        }
    }

    // libavcodec can't decode TrueHD Mono (bug #356)
    // work around it by requesting Stereo and downmixing
    if (w->codec_param                     == AV_CODEC_ID_TRUEHD &&
        w->audio->config.in.channel_layout == AV_CH_LAYOUT_MONO)
    {
        pv->context->request_channel_layout = AV_CH_LAYOUT_STEREO;
    }

    // Set decoder opts...
    AVDictionary * av_opts = NULL;
    av_dict_set( &av_opts, "refcounted_frames", "1", 0 );

    // Dynamic Range Compression
    if (w->audio->config.out.dynamic_range_compression >= 0.0f &&
        hb_audio_can_apply_drc(w->audio->config.in.codec,
                               w->audio->config.in.codec_param, 0))
    {
        float drc_scale_max = 1.0f;
        /*
         * avcodec_open will fail if the value for any of the options is out of
         * range, so assume a conservative maximum of 1 and try to determine the
         * option's actual upper limit.
         */
        if (codec != NULL && codec->priv_class != NULL)
        {
            const AVOption *opt;
            opt = av_opt_find2((void*)&codec->priv_class, "drc_scale", NULL,
                               AV_OPT_FLAG_DECODING_PARAM|AV_OPT_FLAG_AUDIO_PARAM,
                               AV_OPT_SEARCH_FAKE_OBJ, NULL);
            if (opt != NULL)
            {
                drc_scale_max = opt->max;
            }
        }
        if (w->audio->config.out.dynamic_range_compression > drc_scale_max)
        {
            hb_log("decavcodecaInit: track %d, sanitizing out-of-range DRC %.2f to %.2f",
                   w->audio->config.out.track,
                   w->audio->config.out.dynamic_range_compression, drc_scale_max);
            w->audio->config.out.dynamic_range_compression = drc_scale_max;
        }

        char drc_scale[5]; // "?.??\n"
        snprintf(drc_scale, sizeof(drc_scale), "%.2f",
                 w->audio->config.out.dynamic_range_compression);
        av_dict_set(&av_opts, "drc_scale", drc_scale, 0);
    }

    if (hb_avcodec_open(pv->context, codec, &av_opts, 0))
    {
        av_dict_free( &av_opts );
        hb_log("decavcodecaInit: avcodec_open failed");
        return 1;
    }
    // avcodec_open populates av_opts with the things it didn't recognize.
    AVDictionaryEntry *t = NULL;
    while ((t = av_dict_get(av_opts, "", t, AV_DICT_IGNORE_SUFFIX)) != NULL)
    {
            hb_log("decavcodecaInit: unknown option '%s'", t->key);
    }
    av_dict_free( &av_opts );

    pv->frame = av_frame_alloc();
    if (pv->frame == NULL)
    {
        hb_log("decavcodecaInit: av_frame_alloc failed");
        return 1;
    }

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
static void closePrivData( hb_work_private_t ** ppv )
{
    hb_work_private_t * pv = *ppv;

    if ( pv )
    {
        hb_buffer_list_close(&pv->list);

        if ( pv->job && pv->context && pv->context->codec )
        {
            hb_log( "%s-decoder done: %u frames, %u decoder errors",
                    pv->context->codec->name, pv->nframes, pv->decode_errors);
        }
        av_frame_free(&pv->frame);
        if ( pv->sws_context )
        {
            sws_freeContext( pv->sws_context );
        }
        if ( pv->parser )
        {
            av_parser_close(pv->parser);
        }
        if ( pv->context && pv->context->codec )
        {
#ifdef USE_QSV
            /*
             * FIXME: knowingly leaked.
             *
             * If we're using our Libav QSV wrapper, qsv_decode_end() will call
             * MFXClose() on the QSV session. Even if decoding is complete, we
             * still need that session for QSV filtering and/or encoding, so we
             * we can't close the context here until we implement a proper fix.
             */
            if (pv->qsv.decode == NULL ||
                pv->qsv.config.io_pattern != MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
#endif
            {
                hb_avcodec_close(pv->context);
            }
        }
        if ( pv->context )
        {
            av_freep( &pv->context->extradata );
            av_freep( &pv->context );
        }
        hb_audio_resample_free(pv->resample);

        int ii;
        for (ii = 0; ii < REORDERED_HASH_SZ; ii++)
        {
            free(pv->reordered_hash[ii]);
        }
        free(pv);
    }
    *ppv = NULL;
}

static void decavcodecClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if ( pv )
    {
        closePrivData( &pv );
        w->private_data = NULL;
    }
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
static int decavcodecaWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                            hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;

    // libavcodec/mpeg12dec.c requires buffers to be zero padded.
    // If not zero padded, it can get stuck in an infinite loop.
    // It's likely there are other decoders that expect the same.
    if (in->data != NULL)
    {
        memset(in->data + in->size, 0, in->alloc - in->size);
    }

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        /* EOF on input stream - send it downstream & say that we're done */
        *buf_out = in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    *buf_out = NULL;

    int     pos, len;
    int64_t pts = in->s.start;

    // There are a 3 scenarios that can happen here.
    // 1. The buffer contains exactly one frame of data
    // 2. The buffer contains multiple frames of data
    // 3. The buffer contains a partial frame of data
    //
    // In scenario 2, we want to be sure that the timestamps are only
    // applied to the first frame in the buffer.  Additional frames
    // in the buffer will have their timestamps computed in sync.
    //
    // In scenario 3, we need to save the ancillary buffer info of an
    // unfinished frame so it can be applied when we receive the last
    // buffer of that frame.
    if (!pv->unfinished)
    {
        // New packet, and no previous data pending
        pv->packet_info.scr_sequence = in->s.scr_sequence;
        pv->packet_info.new_chap     = in->s.new_chap;
        pv->packet_info.frametype    = in->s.frametype;
    }
    for (pos = 0; pos < in->size; pos += len)
    {
        uint8_t * pout;
        int       pout_len;
        int64_t   parser_pts;

        if ( pv->parser != NULL )
        {
            len = av_parser_parse2(pv->parser, pv->context, &pout, &pout_len,
                                   in->data + pos, in->size - pos,
                                   pts, pts, 0 );
            parser_pts = pv->parser->pts;
            pts = AV_NOPTS_VALUE;
        }
        else
        {
            pout = in->data;
            len = pout_len = in->size;
            parser_pts = in->s.start;
        }
        if (pout != NULL && pout_len > 0)
        {
            pv->packet_info.data         = pout;
            pv->packet_info.size         = pout_len;
            pv->packet_info.pts          = parser_pts;

            decodeAudio(pv, &pv->packet_info);

            // There could have been an unfinished packet when we entered
            // decodeVideo that is now finished.  The next packet is associated
            // with the input buffer, so set it's chapter and scr info.
            pv->packet_info.scr_sequence = in->s.scr_sequence;
            pv->unfinished               = 0;
        }
        if (len > 0 && pout_len <= 0)
        {
            pv->unfinished               = 1;
        }
    }
    *buf_out = hb_buffer_list_clear(&pv->list);
    return HB_WORK_OK;
}

static int decavcodecaInfo( hb_work_object_t *w, hb_work_info_t *info )
{
    hb_work_private_t *pv = w->private_data;

    memset( info, 0, sizeof(*info) );

    if ( pv && pv->context )
    {
        AVCodecContext *context = pv->context;
        info->bitrate = context->bit_rate;
        info->rate.num = context->time_base.num;
        info->rate.den = context->time_base.den;
        info->profile = context->profile;
        info->level = context->level;
        return 1;
    }
    return 0;
}

static int decavcodecaBSInfo( hb_work_object_t *w, const hb_buffer_t *buf,
                             hb_work_info_t *info )
{
    hb_work_private_t *pv = w->private_data;
    int ret = 0;
    hb_audio_t *audio = w->audio;

    memset( info, 0, sizeof(*info) );

    if ( pv && pv->context )
    {
        return decavcodecaInfo( w, info );
    }

    AVCodec *codec = avcodec_find_decoder( w->codec_param );
    if ( ! codec )
    {
        // there's no ffmpeg codec for this audio type - give up
        return -1;
    }

    static char codec_name[64];
    info->name =  strncpy( codec_name, codec->name, sizeof(codec_name)-1 );

    AVCodecContext *context      = avcodec_alloc_context3(codec);
    AVCodecParserContext *parser = NULL;

    if (w->title && w->title->opaque_priv != NULL)
    {
        AVFormatContext *ic = (AVFormatContext*)w->title->opaque_priv;
        avcodec_copy_context(context, ic->streams[audio->id]->codec);
        // libav's eac3 parser toggles the codec_id in the context as
        // it reads eac3 data between AV_CODEC_ID_AC3 and AV_CODEC_ID_EAC3.
        // It detects an AC3 sync pattern sometimes in ac3_sync() which
        // causes it to eventually set avctx->codec_id to AV_CODEC_ID_AC3
        // in ff_aac_ac3_parse(). Since we are parsing some data before
        // we get here, the codec_id may have flipped.  This will cause an
        // error in hb_avcodec_open().  So flip it back!
        context->codec_id = w->codec_param;
    }
    else
    {
        parser = av_parser_init(codec->id);
    }

    hb_ff_set_sample_fmt( context, codec, AV_SAMPLE_FMT_FLT );

    AVDictionary * av_opts = NULL;
    av_dict_set( &av_opts, "err_detect", "crccheck+explode", 0 );
    if ( hb_avcodec_open( context, codec, &av_opts, 0 ) )
    {
        av_dict_free( &av_opts );
        return -1;
    }
    av_dict_free( &av_opts );
    unsigned char *parse_buffer;
    int parse_pos, dec_pos, parse_buffer_size;

    while (buf != NULL && !ret)
    {
        parse_pos = 0;
        while (parse_pos < buf->size)
        {
            int parse_len, truehd_mono = 0;

            if (parser != NULL)
            {
                parse_len = av_parser_parse2(parser, context,
                                &parse_buffer, &parse_buffer_size,
                                buf->data + parse_pos, buf->size - parse_pos,
                                buf->s.start, buf->s.start, 0);
            }
            else
            {
                parse_buffer = buf->data + parse_pos;
                parse_len = parse_buffer_size = buf->size - parse_pos;
            }

            // libavcodec can't decode TrueHD Mono (bug #356)
            // work around it by requesting Stereo before decoding
            if (context->codec_id == AV_CODEC_ID_TRUEHD &&
                context->channel_layout == AV_CH_LAYOUT_MONO)
            {
                truehd_mono                     = 1;
                context->request_channel_layout = AV_CH_LAYOUT_STEREO;
            }
            else
            {
                context->request_channel_layout = 0;
            }

            dec_pos = 0;
            while (dec_pos < parse_buffer_size)
            {
                int dec_len;
                int got_frame;
                AVFrame *frame = av_frame_alloc();
                AVPacket avp;
                av_init_packet(&avp);
                avp.data = parse_buffer + dec_pos;
                avp.size = parse_buffer_size - dec_pos;

                dec_len = avcodec_decode_audio4(context, frame, &got_frame, &avp);
                if (dec_len < 0)
                {
                    av_frame_free(&frame);
                    break;
                }
                if (dec_len > 0 && got_frame)
                {
                    // libavcoded doesn't consistently set frame->sample_rate
                    if (frame->sample_rate != 0)
                    {
                        info->rate.num = frame->sample_rate;
                    }
                    else
                    {
                        info->rate.num = context->sample_rate;
                        hb_log("decavcodecaBSInfo: warning: invalid frame sample_rate! Using context sample_rate.");
                    }
                    info->rate.den          = 1;
                    info->samples_per_frame = frame->nb_samples;
                    info->sample_bit_depth  = context->bits_per_raw_sample;

                    int bps = av_get_bits_per_sample(context->codec_id);
                    int channels = av_get_channel_layout_nb_channels(frame->channel_layout);
                    if (bps > 0)
                    {
                        info->bitrate = bps * channels * info->rate.num;
                    }
                    else if (context->bit_rate > 0)
                    {
                        info->bitrate = context->bit_rate;
                    }
                    else
                    {
                        info->bitrate = 1;
                    }

                    if (truehd_mono)
                    {
                        info->channel_layout = AV_CH_LAYOUT_MONO;
                        info->matrix_encoding = AV_MATRIX_ENCODING_NONE;
                    }
                    else
                    {
                        AVFrameSideData *side_data;
                        if ((side_data =
                             av_frame_get_side_data(frame,
                                                    AV_FRAME_DATA_MATRIXENCODING)) != NULL)
                        {
                            info->matrix_encoding = *side_data->data;
                        }
                        else
                        {
                            info->matrix_encoding = AV_MATRIX_ENCODING_NONE;
                        }
                        if (info->matrix_encoding == AV_MATRIX_ENCODING_DOLBY ||
                            info->matrix_encoding == AV_MATRIX_ENCODING_DPLII)
                        {
                            info->channel_layout = AV_CH_LAYOUT_STEREO_DOWNMIX;
                        }
                        else
                        {
                            info->channel_layout = frame->channel_layout;
                        }
                    }
                    if (context->codec_id == AV_CODEC_ID_AC3 ||
                        context->codec_id == AV_CODEC_ID_EAC3)
                    {
                        if (context->audio_service_type == AV_AUDIO_SERVICE_TYPE_KARAOKE)
                        {
                            info->mode = 7;
                        }
                        else
                        {
                            info->mode = context->audio_service_type;
                        }
                    }
                    else if (context->codec_id == AV_CODEC_ID_AAC &&
                             context->extradata_size == 0)
                    {
                        // Parse ADTS AAC streams for AudioSpecificConfig.
                        // This data is required in order to write
                        // proper headers in MP4 and MKV files.
                        AVBitStreamFilterContext* aac_adtstoasc;
                        aac_adtstoasc = av_bitstream_filter_init("aac_adtstoasc");
                        if (aac_adtstoasc)
                        {
                            int ret, size;
                            uint8_t *data;
                            ret = av_bitstream_filter_filter(aac_adtstoasc, context,
                                    NULL, &data, &size, avp.data, avp.size, 0);
                            if (ret >= 0 &&
                                context->extradata_size > 0 &&
                                audio->priv.config.extradata.length == 0)
                            {
                                int len;
                                len = MIN(context->extradata_size, HB_CONFIG_MAX_SIZE);
                                memcpy(audio->priv.config.extradata.bytes,
                                       context->extradata, len);
                                audio->priv.config.extradata.length = len;
                            }
                            av_bitstream_filter_close(aac_adtstoasc);
                        }
                    }

                    ret = 1;
                    av_frame_free(&frame);
                    break;
                }
                dec_pos += dec_len;
                av_frame_free(&frame);
            }
            parse_pos += parse_len;
        }
        buf = buf->next;
    }

    info->profile = context->profile;
    info->level = context->level;
    info->channel_map = &hb_libav_chan_map;

    if ( parser != NULL )
        av_parser_close( parser );
    hb_avcodec_close( context );
    av_freep( &context->extradata );
    av_freep( &context );
    return ret;
}

reordered_data_t *
reordered_hash_rem(hb_work_private_t * pv, int64_t sequence)
{
    reordered_data_t * reordered;
    int                slot = sequence & REORDERED_HASH_MASK;

    reordered = pv->reordered_hash[slot];
    if (reordered == NULL)
    {
        // This shouldn't happen...
        // But, this happens sometimes when libav outputs exactly the same
        // frame twice for some reason.
        hb_deep_log(3, "decavcodec: missing sequence %"PRId64"", sequence);
    }
    pv->reordered_hash[slot] = NULL;
    return reordered;
}

void
reordered_hash_add(hb_work_private_t * pv, reordered_data_t * reordered)
{
    int slot = reordered->sequence & REORDERED_HASH_MASK;

    // Free any unused previous entries.
    // This can happen due to libav parser feeding partial
    // frames data to the decoder.
    // It can also happen due to decoding errors.
    free(pv->reordered_hash[slot]);
    pv->reordered_hash[slot] = reordered;
}

/* -------------------------------------------------------------
 * General purpose video decoder using libavcodec
 */

static uint8_t *copy_plane( uint8_t *dst, uint8_t* src, int dstride, int sstride,
                            int h )
{
    if ( dstride == sstride )
    {
        memcpy( dst, src, dstride * h );
        return dst + dstride * h;
    }
    int lbytes = dstride <= sstride? dstride : sstride;
    while ( --h >= 0 )
    {
        memcpy( dst, src, lbytes );
        src += sstride;
        dst += dstride;
    }
    return dst;
}

// copy one video frame into an HB buf. If the frame isn't in our color space
// or at least one of its dimensions is odd, use sws_scale to convert/rescale it.
// Otherwise just copy the bits.
static hb_buffer_t *copy_frame( hb_work_private_t *pv )
{
    AVCodecContext *context = pv->context;
    int w, h;
    if ( ! pv->job )
    {
        // HandBrake's video pipeline uses yuv420 color.  This means all
        // dimensions must be even.  So we must adjust the dimensions
        // of incoming video if not even.
        w = context->width & ~1;
        h = context->height & ~1;
    }
    else
    {
        w = pv->job->title->geometry.width;
        h = pv->job->title->geometry.height;
    }

    reordered_data_t * reordered = NULL;
    hb_buffer_t      * out = hb_video_buffer_init( w, h );

    if (pv->frame->pkt_pts != AV_NOPTS_VALUE)
    {
        reordered = reordered_hash_rem(pv, pv->frame->pkt_pts);
    }
    if (reordered != NULL)
    {
        out->s.scr_sequence   = reordered->scr_sequence;
        out->s.start          = reordered->pts;
        out->s.new_chap       = reordered->new_chap;
        pv->last_scr_sequence = reordered->scr_sequence;
        pv->last_chapter      = reordered->new_chap;
        free(reordered);
    }
    else
    {
        out->s.scr_sequence   = pv->last_scr_sequence;
        out->s.start          = AV_NOPTS_VALUE;
    }
    if (out->s.new_chap > 0 && out->s.new_chap == pv->new_chap)
    {
        pv->new_chap = 0;
    }
    // It is possible that the buffer with new_chap gets dropped
    // by the decoder.  So also check if the output buffer is after
    // the new_chap in the timeline.
    if (pv->new_chap > 0 &&
        (out->s.scr_sequence > pv->chap_scr ||
         (out->s.scr_sequence == pv->chap_scr && out->s.start > pv->chap_time)))
    {
        out->s.new_chap = pv->new_chap;
        pv->new_chap    = 0;
    }

#ifdef USE_QSV
    // no need to copy the frame data when decoding with QSV to opaque memory
    if (pv->qsv.decode &&
        pv->qsv.config.io_pattern == MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
    {
        out->qsv_details.qsv_atom = pv->frame->data[2];
        out->qsv_details.ctx      = pv->job->qsv.ctx;
        return out;
    }
#endif

    uint8_t *dst = out->data;

    if (context->pix_fmt != AV_PIX_FMT_YUV420P || w != context->width ||
        h != context->height)
    {
        // have to convert to our internal color space and/or rescale
        uint8_t * data[4];
        int       stride[4];
        hb_picture_fill(data, stride, out);

        if (pv->sws_context == NULL            ||
            pv->sws_width   != context->width  ||
            pv->sws_height  != context->height ||
            pv->sws_pix_fmt != context->pix_fmt)
        {
            if (pv->sws_context != NULL)
                sws_freeContext(pv->sws_context);

            hb_geometry_t geometry = {context->width, context->height};
            int color_matrix = get_color_matrix(context->colorspace, geometry);

            pv->sws_context = hb_sws_get_context(context->width,
                                                 context->height,
                                                 context->pix_fmt,
                                                 w, h, AV_PIX_FMT_YUV420P,
                                                 SWS_LANCZOS|SWS_ACCURATE_RND,
                                                 hb_ff_get_colorspace(color_matrix));
            pv->sws_width   = context->width;
            pv->sws_height  = context->height;
            pv->sws_pix_fmt = context->pix_fmt;
        }
        sws_scale(pv->sws_context,
                  (const uint8_t* const *)pv->frame->data,
                  pv->frame->linesize, 0, context->height, data, stride);
    }
    else
    {
        w = out->plane[0].stride;
        h = out->plane[0].height;
        dst = out->plane[0].data;
        copy_plane( dst, pv->frame->data[0], w, pv->frame->linesize[0], h );
        w = out->plane[1].stride;
        h = out->plane[1].height;
        dst = out->plane[1].data;
        copy_plane( dst, pv->frame->data[1], w, pv->frame->linesize[1], h );
        w = out->plane[2].stride;
        h = out->plane[2].height;
        dst = out->plane[2].data;
        copy_plane( dst, pv->frame->data[2], w, pv->frame->linesize[2], h );
    }

    return out;
}

// send cc_buf to the CC decoder(s)
static void cc_send_to_decoder(hb_work_private_t *pv, hb_buffer_t *buf)
{
    if (buf == NULL)
        return;

    // if there's more than one decoder for the captions send a copy
    // of the buffer to all.
    hb_subtitle_t *subtitle;
    int ii = 0, n = hb_list_count(pv->list_subtitle);
    while (--n > 0)
    {
        // make a copy of the buf then forward it to the decoder
        hb_buffer_t *cpy = hb_buffer_dup(buf);

        subtitle = hb_list_item(pv->list_subtitle, ii++);
        hb_fifo_push(subtitle->fifo_in, cpy);
    }
    subtitle = hb_list_item(pv->list_subtitle, ii);
    hb_fifo_push( subtitle->fifo_in, buf );
}

static hb_buffer_t * cc_fill_buffer(hb_work_private_t *pv, uint8_t *cc, int size)
{
    int cc_count[4] = {0,};
    int ii;
    hb_buffer_t *buf = NULL;

    for (ii = 0; ii < size; ii += 3)
    {
        if ((cc[ii] & 0x04) == 0)    // not valid
            continue;
        if ((cc[ii+1] & 0x7f) == 0 && (cc[ii+2] & 0x7f) == 0) // stuffing
            continue;
        int type = cc[ii] & 0x03;
        cc_count[type]++;
    }

    // Only handles CC1 for now.
    if (cc_count[0] > 0)
    {
        buf = hb_buffer_init(cc_count[0] * 2);
        int jj = 0;
        for (ii = 0; ii < size; ii += 3)
        {
            if ((cc[ii] & 0x04) == 0)    // not valid
                continue;
            if ((cc[ii+1] & 0x7f) == 0 && (cc[ii+2] & 0x7f) == 0) // stuffing
                continue;
            int type = cc[ii] & 0x03;
            if (type == 0)
            {
                buf->data[jj++] = cc[ii+1];
                buf->data[jj++] = cc[ii+2];
            }
        }
    }
    return buf;
}

static int get_frame_type(int type)
{
    switch (type)
    {
        case AV_PICTURE_TYPE_B:
            return HB_FRAME_B;

        case AV_PICTURE_TYPE_S:
        case AV_PICTURE_TYPE_P:
        case AV_PICTURE_TYPE_SP:
            return HB_FRAME_P;

        case AV_PICTURE_TYPE_BI:
        case AV_PICTURE_TYPE_SI:
        case AV_PICTURE_TYPE_I:
        default:
            return HB_FRAME_I;
    }
}

/*
 * Decodes a video frame from the specified raw packet data
 *      ('data', 'size').
 * The output of this function is stored in 'pv->list', which contains a list
 * of zero or more decoded packets.
 */
static int decodeFrame( hb_work_object_t *w, packet_info_t * packet_info )
{
    hb_work_private_t *pv = w->private_data;
    int got_picture, oldlevel = 0;
    AVPacket avp;
    reordered_data_t * reordered;

    if ( global_verbosity_level <= 1 )
    {
        oldlevel = av_log_get_level();
        av_log_set_level( AV_LOG_QUIET );
    }

    av_init_packet(&avp);
    if (packet_info != NULL)
    {
        avp.data = packet_info->data;
        avp.size = packet_info->size;
        avp.pts  = pv->sequence;
        avp.dts  = pv->sequence;
        reordered = malloc(sizeof(*reordered));
        if (reordered != NULL)
        {
            reordered->sequence     = pv->sequence++;
            reordered->pts          = packet_info->pts;
            reordered->scr_sequence = packet_info->scr_sequence;
            reordered->new_chap     = packet_info->new_chap;
        }
        reordered_hash_add(pv, reordered);

        // libav avcodec_decode_video2() needs AVPacket flagged with
        // AV_PKT_FLAG_KEY for some codecs. For example, sequence of
        // PNG in a mov container.
        if (packet_info->frametype & HB_FRAME_MASK_KEY)
        {
            avp.flags |= AV_PKT_FLAG_KEY;
        }
    }
    else
    {
        avp.data = NULL;
        avp.size = 0;
    }

    if (pv->palette != NULL)
    {
        uint8_t * palette;
        int size;
        palette = av_packet_new_side_data(&avp, AV_PKT_DATA_PALETTE,
                                          AVPALETTE_SIZE);
        size = MIN(pv->palette->size, AVPALETTE_SIZE);
        memcpy(palette, pv->palette->data, size);
        hb_buffer_close(&pv->palette);
    }

    if (avcodec_decode_video2(pv->context, pv->frame, &got_picture, &avp) < 0)
    {
        ++pv->decode_errors;
    }

#ifdef USE_QSV
    if (pv->qsv.decode &&
        pv->qsv.config.io_pattern == MFX_IOPATTERN_OUT_OPAQUE_MEMORY &&
        pv->job->qsv.ctx == NULL && pv->video_codec_opened > 0)
    {
        // this is quite late, but we can't be certain that the QSV context is
        // available until after we call avcodec_decode_video2() at least once
        pv->job->qsv.ctx = pv->context->priv_data;
    }
#endif

    if ( global_verbosity_level <= 1 )
    {
        av_log_set_level( oldlevel );
    }
    if (got_picture)
    {
        uint16_t flags = 0;

        // ffmpeg makes it hard to attach a pts to a frame. if the MPEG ES
        // packet had a pts we handed it to av_parser_parse (if the packet had
        // no pts we set it to AV_NOPTS_VALUE, but before the parse we can't
        // distinguish between the start of a video frame with no pts & an
        // intermediate packet of some frame which never has a pts). we hope
        // that when parse returns the frame to us the pts we originally
        // handed it will be in parser->pts. we put this pts into avp.pts so
        // that when avcodec_decode_video finally gets around to allocating an
        // AVFrame to hold the decoded frame, avcodec_default_get_buffer can
        // stuff that pts into the it. if all of these relays worked at this
        // point frame.pts should hold the frame's pts from the original data
        // stream or AV_NOPTS_VALUE if it didn't have one. in the latter case
        // we generate the next pts in sequence for it.

        // recompute the frame/field duration, because sometimes it changes
        compute_frame_duration( pv );

        double  frame_dur = pv->duration;
        if ( pv->frame->repeat_pict )
        {
            frame_dur += pv->frame->repeat_pict * pv->field_duration;
        }
        hb_buffer_t * out = copy_frame( pv );
        if (out->s.start == AV_NOPTS_VALUE)
        {
            out->s.start = pv->next_pts;
        }
        else
        {
            pv->next_pts = out->s.start;
        }
        if (pv->next_pts != (int64_t)AV_NOPTS_VALUE)
        {
            pv->next_pts += frame_dur;
            out->s.stop   = pv->next_pts;
        }

        if ( pv->frame->top_field_first )
        {
            flags |= PIC_FLAG_TOP_FIELD_FIRST;
        }
        if ( !pv->frame->interlaced_frame )
        {
            flags |= PIC_FLAG_PROGRESSIVE_FRAME;
        }
        if ( pv->frame->repeat_pict == 1 )
        {
            flags |= PIC_FLAG_REPEAT_FIRST_FIELD;
        }
        if ( pv->frame->repeat_pict == 2 )
        {
            flags |= PIC_FLAG_REPEAT_FRAME;
        }
        int frametype = get_frame_type(pv->frame->pict_type);

        // Check for CC data
        AVFrameSideData *sd;
        sd = av_frame_get_side_data(pv->frame, AV_FRAME_DATA_A53_CC);
        if (sd != NULL)
        {
            if (!pv->job && pv->title && sd->size > 0)
            {
                hb_subtitle_t *subtitle;
                int i = 0;

                while ((subtitle = hb_list_item(pv->title->list_subtitle, i++)))
                {
                    /*
                     * Let's call them 608 subs for now even if they aren't,
                     * since they are the only types we grok.
                     */
                    if (subtitle->source == CC608SUB)
                    {
                        break;
                    }
                }
                if (subtitle == NULL)
                {
                    subtitle = calloc(sizeof( hb_subtitle_t ), 1);
                    subtitle->track = hb_list_count(pv->title->list_subtitle);
                    subtitle->id = 0;
                    subtitle->format = TEXTSUB;
                    subtitle->source = CC608SUB;
                    subtitle->config.dest = PASSTHRUSUB;
                    subtitle->codec = WORK_DECCC608;
                    subtitle->type = 5;
                    snprintf(subtitle->lang, sizeof( subtitle->lang ),
                             "Closed Captions");
                    /*
                     * The language of the subtitles will be the same as the
                     * first audio track, i.e. the same as the video.
                     */
                    hb_audio_t *audio = hb_list_item(pv->title->list_audio, 0);
                    if (audio != NULL)
                    {
                        snprintf(subtitle->iso639_2, sizeof(subtitle->iso639_2),
                                 "%s", audio->config.lang.iso639_2);
                    } else {
                        snprintf(subtitle->iso639_2, sizeof(subtitle->iso639_2),
                                 "und");
                    }
                    hb_list_add(pv->title->list_subtitle, subtitle);
                }
            }
            if (pv->list_subtitle != NULL && sd->size > 0)
            {
                hb_buffer_t *cc_buf;
                cc_buf = cc_fill_buffer(pv, sd->data, sd->size);
                if (cc_buf != NULL)
                {
                    cc_buf->s.start        = out->s.start;
                    cc_buf->s.scr_sequence = out->s.scr_sequence;
                }
                cc_send_to_decoder(pv, cc_buf);
            }
        }

        av_frame_unref(pv->frame);

        out->s.duration  = frame_dur;
        out->s.flags     = flags;
        out->s.frametype = frametype;

        hb_buffer_list_append(&pv->list, out);
        ++pv->nframes;
    }

    return got_picture;
}

static void decodeVideo( hb_work_object_t *w, hb_buffer_t * in)
{
    hb_work_private_t *pv = w->private_data;

    /*
     * The following loop is a do..while because we need to handle both
     * data & the flush at the end (signaled by size=0). At the end there's
     * generally a frame in the parser & one or more frames in the decoder
     * (depending on the bframes setting).
     */
    int      pos, len;
    int64_t  pts = in->s.start;
    int64_t  dts = in->s.renderOffset;

    if (in->s.new_chap > 0)
    {
        pv->new_chap = in->s.new_chap;
        pv->chap_scr = in->s.scr_sequence;
        if (in->s.start != AV_NOPTS_VALUE)
        {
            pv->chap_time = in->s.start;
        }
        else
        {
            pv->chap_time = pv->last_pts + 1;
        }
    }
    if (in->s.start != AV_NOPTS_VALUE)
    {
        pv->last_pts = in->s.start;
    }

    // There are a 3 scenarios that can happen here.
    // 1. The buffer contains exactly one frame of data
    // 2. The buffer contains multiple frames of data
    // 3. The buffer contains a partial frame of data
    //
    // In scenario 2, we want to be sure that the timestamps are only
    // applied to the first frame in the buffer.  Additional frames
    // in the buffer will have their timestamps computed in sync.
    //
    // In scenario 3, we need to save the ancillary buffer info of an
    // unfinished frame so it can be applied when we receive the last
    // buffer of that frame.
    if (!pv->unfinished)
    {
        // New packet, and no previous data pending
        pv->packet_info.scr_sequence = in->s.scr_sequence;
        pv->packet_info.new_chap     = in->s.new_chap;
        pv->packet_info.frametype    = in->s.frametype;
    }
    for (pos = 0; pos < in->size; pos += len)
    {
        uint8_t * pout;
        int       pout_len;
        int64_t   parser_pts, parser_dts;

        if (pv->parser)
        {
            len = av_parser_parse2(pv->parser, pv->context, &pout, &pout_len,
                                   in->data + pos, in->size - pos,
                                   pts, dts, 0 );
            parser_pts = pv->parser->pts;
            parser_dts = pv->parser->dts;
            pts = AV_NOPTS_VALUE;
            dts = AV_NOPTS_VALUE;
        }
        else
        {
            pout = in->data;
            len = pout_len = in->size;
            parser_pts = pts;
            parser_dts = dts;
        }

        if (pout != NULL && pout_len > 0)
        {
            pv->packet_info.data         = pout;
            pv->packet_info.size         = pout_len;
            pv->packet_info.pts          = parser_pts;
            pv->packet_info.dts          = parser_dts;

            decodeFrame(w, &pv->packet_info);

            // There could have been an unfinished packet when we entered
            // decodeVideo that is now finished.  The next packet is associated
            // with the input buffer, so set it's chapter and scr info.
            pv->packet_info.scr_sequence = in->s.scr_sequence;
            pv->packet_info.new_chap     = in->s.new_chap;
            pv->packet_info.frametype    = in->s.frametype;
            pv->unfinished               = 0;
        }
        if (len > 0 && pout_len <= 0)
        {
            pv->unfinished               = 1;
        }
    }

    /* the stuff above flushed the parser, now flush the decoder */
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        while (decodeFrame(w, NULL))
        {
            continue;
        }
#ifdef USE_QSV
        if (pv->qsv.decode &&
            pv->qsv.config.io_pattern == MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
        {
            // flush a second time
            while (decodeFrame(w, NULL))
            {
                continue;
            }
        }
#endif
        if (pv->list_subtitle != NULL)
            cc_send_to_decoder(pv, hb_buffer_eof_init());
    }
}

static int decavcodecvInit( hb_work_object_t * w, hb_job_t * job )
{

    hb_work_private_t *pv = calloc( 1, sizeof( hb_work_private_t ) );

    w->private_data = pv;
    pv->job         = job;
    pv->next_pts    = (int64_t)AV_NOPTS_VALUE;
    if ( job )
        pv->title = job->title;
    else
        pv->title = w->title;
    hb_buffer_list_clear(&pv->list);

#ifdef USE_QSV
    if ((pv->qsv.decode = hb_qsv_decode_is_enabled(job)))
    {
        pv->qsv.codec_name = hb_qsv_decode_get_codec_name(w->codec_param);
        pv->qsv.config.io_pattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
#if 0 // TODO: re-implement QSV zerocopy path
        hb_qsv_info_t *info = hb_qsv_info_get(job->vcodec);
        if (info != NULL)
        {
            // setup the QSV configuration
            pv->qsv.config.io_pattern         = MFX_IOPATTERN_OUT_OPAQUE_MEMORY;
            pv->qsv.config.impl_requested     = info->implementation;
            pv->qsv.config.async_depth        = job->qsv.async_depth;
            pv->qsv.config.sync_need          =  0;
            pv->qsv.config.usage_threaded     =  1;
            pv->qsv.config.additional_buffers = 64; // FIFO_LARGE
            if (info->capabilities & HB_QSV_CAP_RATECONTROL_LA)
            {
                // more surfaces may be needed for the lookahead
                pv->qsv.config.additional_buffers = 160;
            }
        }
#endif // QSV zerocopy path
    }
#endif

    if( pv->job && pv->job->title && !pv->job->title->has_resolution_change )
    {
        pv->threads = HB_FFMPEG_THREADS_AUTO;
    }

    AVCodec *codec = NULL;

#ifdef USE_QSV
    if (pv->qsv.decode)
    {
        codec = avcodec_find_decoder_by_name(pv->qsv.codec_name);
    }
    else
#endif
    {
        codec = avcodec_find_decoder(w->codec_param);
    }
    if ( codec == NULL )
    {
        hb_log( "decavcodecvInit: failed to find codec for id (%d)", w->codec_param );
        return 1;
    }

    if ( pv->title->opaque_priv )
    {
        AVFormatContext *ic = (AVFormatContext*)pv->title->opaque_priv;

        pv->context = avcodec_alloc_context3(codec);
        avcodec_copy_context( pv->context, ic->streams[pv->title->video_id]->codec);
        pv->context->workaround_bugs = FF_BUG_AUTODETECT;
        pv->context->err_recognition = AV_EF_CRCCHECK;
        pv->context->error_concealment = FF_EC_GUESS_MVS|FF_EC_DEBLOCK;

#ifdef USE_QSV
        if (pv->qsv.decode &&
            pv->qsv.config.io_pattern == MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
        {
            // set the QSV configuration before opening the decoder
            pv->context->hwaccel_context = &pv->qsv.config;
        }
#endif

        // Set encoder opts...
        AVDictionary * av_opts = NULL;
        av_dict_set( &av_opts, "refcounted_frames", "1", 0 );
        if (pv->title->flags & HBTF_NO_IDR)
        {
            av_dict_set( &av_opts, "flags", "output_corrupt", 0 );
        }

        if ( hb_avcodec_open( pv->context, codec, &av_opts, pv->threads ) )
        {
            av_dict_free( &av_opts );
            hb_log( "decavcodecvInit: avcodec_open failed" );
            return 1;
        }
        av_dict_free( &av_opts );

        pv->video_codec_opened = 1;
    }
    else
    {
        pv->parser = av_parser_init( w->codec_param );
    }

    pv->frame = av_frame_alloc();
    if (pv->frame == NULL)
    {
        hb_log("decavcodecvInit: av_frame_alloc failed");
        return 1;
    }

    /*
     * If not scanning, then are we supposed to extract Closed Captions
     * and send them to the decoder?
     */
    if (job != NULL && hb_list_count(job->list_subtitle) > 0)
    {
        hb_subtitle_t *subtitle;
        int i = 0;

        while ((subtitle = hb_list_item(job->list_subtitle, i++)) != NULL)
        {
            if (subtitle->source == CC608SUB)
            {
                if (pv->list_subtitle == NULL)
                {
                    pv->list_subtitle = hb_list_init();
                }
                hb_list_add(pv->list_subtitle, subtitle);
            }
        }
    }
    return 0;
}

static int setup_extradata( hb_work_object_t *w, hb_buffer_t *in )
{
    hb_work_private_t *pv = w->private_data;

    // we can't call the avstream funcs but the read_header func in the
    // AVInputFormat may set up some state in the AVContext. In particular
    // vc1t_read_header allocates 'extradata' to deal with header issues
    // related to Microsoft's bizarre engineering notions. We alloc a chunk
    // of space to make vc1 work then associate the codec with the context.
    if (pv->context->extradata == NULL)
    {
        if (pv->parser == NULL || pv->parser == NULL ||
            pv->parser->parser->split == NULL)
        {
            return 0;
        }
        else
        {
            int size;
            size = pv->parser->parser->split(pv->context, in->data, in->size);
            if (size > 0)
            {
                pv->context->extradata_size = size;
                pv->context->extradata =
                                av_malloc(size + FF_INPUT_BUFFER_PADDING_SIZE);
                if (pv->context->extradata == NULL)
                    return 1;
                memcpy(pv->context->extradata, in->data, size);
                return 0;
            }
        }
        return 1;
    }

    return 0;
}

static int decavcodecvWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                            hb_buffer_t ** buf_out )
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *in = *buf_in;

    *buf_in = NULL;
    *buf_out = NULL;

    // libavcodec/mpeg12dec.c requires buffers to be zero padded.
    // If not zero padded, it can get stuck in an infinite loop.
    // It's likely there are other decoders that expect the same.
    if (in->data != NULL)
    {
        memset(in->data + in->size, 0, in->alloc - in->size);
    }

    /* if we got an empty buffer signaling end-of-stream send it downstream */
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        if (pv->context != NULL && pv->context->codec != NULL)
        {
            decodeVideo(w, in);
        }
        hb_buffer_list_append(&pv->list, in);
        *buf_out = hb_buffer_list_clear(&pv->list);
        return HB_WORK_DONE;
    }

    // if this is the first frame open the codec (we have to wait for the
    // first frame because of M$ VC1 braindamage).
    if ( !pv->video_codec_opened )
    {

        AVCodec *codec = NULL;
#ifdef USE_QSV
        if (pv->qsv.decode)
        {
            codec = avcodec_find_decoder_by_name(pv->qsv.codec_name);
        }
        else
#endif
        {
            codec = avcodec_find_decoder(w->codec_param);
        }

        if ( codec == NULL )
        {
            hb_log( "decavcodecvWork: failed to find codec for id (%d)", w->codec_param );
            *buf_out = hb_buffer_eof_init();
            return HB_WORK_DONE;
        }

        pv->context = avcodec_alloc_context3( codec );
        pv->context->workaround_bugs = FF_BUG_AUTODETECT;
        pv->context->err_recognition = AV_EF_CRCCHECK;
        pv->context->error_concealment = FF_EC_GUESS_MVS|FF_EC_DEBLOCK;

        if ( setup_extradata( w, in ) )
        {
            // we didn't find the headers needed to set up extradata.
            // the codec will abort if we open it so just free the buf
            // and hope we eventually get the info we need.
            hb_buffer_close( &in );
            return HB_WORK_OK;
        }

#ifdef USE_QSV
        if (pv->qsv.decode &&
            pv->qsv.config.io_pattern == MFX_IOPATTERN_OUT_OPAQUE_MEMORY)
        {
            // set the QSV configuration before opening the decoder
            pv->context->hwaccel_context = &pv->qsv.config;
        }
#endif

        AVDictionary * av_opts = NULL;
        av_dict_set( &av_opts, "refcounted_frames", "1", 0 );
        if (pv->title->flags & HBTF_NO_IDR)
        {
            av_dict_set( &av_opts, "flags", "output_corrupt", 0 );
        }

        // disable threaded decoding for scan, can cause crashes
        if ( hb_avcodec_open( pv->context, codec, &av_opts, pv->threads ) )
        {
            av_dict_free( &av_opts );
            hb_log( "decavcodecvWork: avcodec_open failed" );
            *buf_out = hb_buffer_eof_init();
            return HB_WORK_DONE;
        }
        av_dict_free( &av_opts );
        pv->video_codec_opened = 1;
    }

    if (in->palette != NULL)
    {
        pv->palette = in->palette;
        in->palette = NULL;
    }
    decodeVideo(w, in);
    hb_buffer_close( &in );
    *buf_out = hb_buffer_list_clear(&pv->list);
    return HB_WORK_OK;
}

static void compute_frame_duration( hb_work_private_t *pv )
{
    double duration = 0.;
    int64_t max_fps = 64LL;

    // context->time_base may be in fields, so set the max *fields* per second
    if ( pv->context->ticks_per_frame > 1 )
        max_fps *= pv->context->ticks_per_frame;

    if ( pv->title->opaque_priv )
    {
        // If ffmpeg is demuxing for us, it collects some additional
        // information about framerates that is often more accurate
        // than context->time_base.
        AVFormatContext *ic = (AVFormatContext*)pv->title->opaque_priv;
        AVStream *st = ic->streams[pv->title->video_id];
        if ( st->nb_frames && st->duration )
        {
            // compute the average frame duration from the total number
            // of frames & the total duration.
            duration = ( (double)st->duration * (double)st->time_base.num ) /
                       ( (double)st->nb_frames * (double)st->time_base.den );
        }
        // Raw demuxers set a default fps of 25 and do not parse
        // a value from the container.  So use the codec time_base
        // for raw demuxers.
        else if (ic->iformat->raw_codec_id == AV_CODEC_ID_NONE)
        {
            // XXX We don't have a frame count or duration so try to use the
            // far less reliable time base info in the stream.
            // Because the time bases are so screwed up, we only take values
            // in the range 8fps - 64fps.
            AVRational *tb = NULL;
            if ( st->avg_frame_rate.den * 64LL > st->avg_frame_rate.num &&
                 st->avg_frame_rate.num > st->avg_frame_rate.den * 8LL )
            {
                tb = &(st->avg_frame_rate);
                duration =  (double)tb->den / (double)tb->num;
            }
            else if ( st->time_base.num * 64LL > st->time_base.den &&
                      st->time_base.den > st->time_base.num * 8LL )
            {
                tb = &(st->time_base);
                duration =  (double)tb->num / (double)tb->den;
            }
        }
        if ( !duration &&
             pv->context->time_base.num * max_fps > pv->context->time_base.den &&
             pv->context->time_base.den > pv->context->time_base.num * 8LL )
        {
            duration =  (double)pv->context->time_base.num /
                        (double)pv->context->time_base.den;
            if ( pv->context->ticks_per_frame > 1 )
            {
                // for ffmpeg 0.5 & later, the H.264 & MPEG-2 time base is
                // field rate rather than frame rate so convert back to frames.
                duration *= pv->context->ticks_per_frame;
            }
        }
    }
    else
    {
        if ( pv->context->time_base.num * max_fps > pv->context->time_base.den &&
             pv->context->time_base.den > pv->context->time_base.num * 8LL )
        {
            duration =  (double)pv->context->time_base.num /
                            (double)pv->context->time_base.den;
            if ( pv->context->ticks_per_frame > 1 )
            {
                // for ffmpeg 0.5 & later, the H.264 & MPEG-2 time base is
                // field rate rather than frame rate so convert back to frames.
                duration *= pv->context->ticks_per_frame;
            }
        }
    }
    if ( duration == 0 )
    {
        // No valid timing info found in the stream, so pick some value
        duration = 1001. / 24000.;
    }
    pv->duration = duration * 90000.;
    pv->field_duration = pv->duration;
    if ( pv->context->ticks_per_frame > 1 )
    {
        pv->field_duration /= pv->context->ticks_per_frame;
    }
}

static int get_color_prim(int color_primaries, hb_geometry_t geometry, hb_rational_t rate)
{
    switch (color_primaries)
    {
        case AVCOL_PRI_BT709:
            return HB_COLR_PRI_BT709;
        case AVCOL_PRI_BT470BG:
            return HB_COLR_PRI_EBUTECH;
        case AVCOL_PRI_BT470M:
        case AVCOL_PRI_SMPTE170M:
        case AVCOL_PRI_SMPTE240M:
            return HB_COLR_PRI_SMPTEC;
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

static int get_color_transfer(int color_trc)
{
    switch (color_trc)
    {
        case AVCOL_TRC_SMPTE240M:
            return HB_COLR_TRA_SMPTE240M;
        default:
            // ITU BT.601, BT.709, anything else
            return HB_COLR_TRA_BT709;
    }
}

static int get_color_matrix(int colorspace, hb_geometry_t geometry)
{
    switch (colorspace)
    {
        case AVCOL_SPC_BT709:
            return HB_COLR_MAT_BT709;
        case AVCOL_SPC_FCC:
        case AVCOL_SPC_BT470BG:
        case AVCOL_SPC_SMPTE170M:
        case AVCOL_SPC_RGB: // libswscale rgb2yuv
            return HB_COLR_MAT_SMPTE170M;
        case AVCOL_SPC_SMPTE240M:
            return HB_COLR_MAT_SMPTE240M;
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


static int decavcodecvInfo( hb_work_object_t *w, hb_work_info_t *info )
{
    hb_work_private_t *pv = w->private_data;

    int clock_min, clock_max, clock;
    hb_video_framerate_get_limits(&clock_min, &clock_max, &clock);

    memset( info, 0, sizeof(*info) );

    if (pv->context == NULL)
        return 0;

    info->bitrate = pv->context->bit_rate;
    // HandBrake's video pipeline uses yuv420 color.  This means all
    // dimensions must be even.  So we must adjust the dimensions
    // of incoming video if not even.
    info->geometry.width = pv->context->width & ~1;
    info->geometry.height = pv->context->height & ~1;

    info->geometry.par.num = pv->context->sample_aspect_ratio.num;
    info->geometry.par.den = pv->context->sample_aspect_ratio.den;

    compute_frame_duration( pv );
    info->rate.num = clock;
    info->rate.den = pv->duration * (clock / 90000.);

    info->profile = pv->context->profile;
    info->level = pv->context->level;
    info->name = pv->context->codec->name;

    info->color_prim = get_color_prim(pv->context->color_primaries, info->geometry, info->rate);
    info->color_transfer = get_color_transfer(pv->context->color_trc);
    info->color_matrix = get_color_matrix(pv->context->colorspace, info->geometry);

    info->video_decode_support = HB_DECODE_SUPPORT_SW;

#ifdef USE_QSV
    if (avcodec_find_decoder_by_name(hb_qsv_decode_get_codec_name(pv->context->codec_id)))
    {
        switch (pv->context->codec_id)
        {
            case AV_CODEC_ID_H264:
                if (pv->context->pix_fmt == AV_PIX_FMT_YUV420P ||
                    pv->context->pix_fmt == AV_PIX_FMT_YUVJ420P)
                {
                    info->video_decode_support |= HB_DECODE_SUPPORT_QSV;
                }
                break;

            default:
                break;
        }
    }
#endif

    return 1;
}

static int decavcodecvBSInfo( hb_work_object_t *w, const hb_buffer_t *buf,
                              hb_work_info_t *info )
{
    return 0;
}

static void decavcodecvFlush( hb_work_object_t *w )
{
    hb_work_private_t *pv = w->private_data;

    if (pv->context != NULL && pv->context->codec != NULL)
    {
        hb_buffer_list_close(&pv->list);
        if ( pv->title->opaque_priv == NULL )
        {
            pv->video_codec_opened = 0;
            hb_avcodec_close( pv->context );
            av_freep( &pv->context->extradata );
            av_freep( &pv->context );
            if ( pv->parser )
            {
                av_parser_close(pv->parser);
            }
            pv->parser = av_parser_init( w->codec_param );
        }
        else
        {
            avcodec_flush_buffers( pv->context );
        }
    }
}

hb_work_object_t hb_decavcodecv =
{
    .id = WORK_DECAVCODECV,
    .name = "Video decoder (libavcodec)",
    .init = decavcodecvInit,
    .work = decavcodecvWork,
    .close = decavcodecClose,
    .flush = decavcodecvFlush,
    .info = decavcodecvInfo,
    .bsinfo = decavcodecvBSInfo
};

static void decodeAudio(hb_work_private_t *pv, packet_info_t * packet_info)
{
    AVCodecContext * context = pv->context;
    int              loop_limit = 256;
    int              pos = 0;
    int64_t          pts = packet_info->pts;

    while (pos < packet_info->size)
    {
        int got_frame;
        AVPacket avp;

        av_init_packet(&avp);
        avp.data = packet_info->data + pos;
        avp.size = packet_info->size - pos;
        avp.pts  = pts;
        avp.dts  = AV_NOPTS_VALUE;

        int len = avcodec_decode_audio4(context, pv->frame, &got_frame, &avp);
        if (len < 0)
        {
            if (pts != AV_NOPTS_VALUE)
            {
                // Update next_pts since subsequent packets may have no
                // pts and depend on next_pts being up to date
                pv->next_pts = pts + pv->duration;
            }
            ++pv->decode_errors;
        }
        if ((len < 0) || (!got_frame && !(loop_limit--)))
        {
            return;
        }
        else
        {
            loop_limit = 256;
        }

        pos += len;

        if (got_frame)
        {
            hb_buffer_t * out;
            int           samplerate;

            // libavcoded doesn't yet consistently set frame->sample_rate
            if (pv->frame->sample_rate != 0)
            {
                samplerate = pv->frame->sample_rate;
            }
            else
            {
                samplerate = context->sample_rate;
            }
            pv->duration = (90000. * pv->frame->nb_samples / samplerate);

            if (pv->audio->config.out.codec & HB_ACODEC_PASS_FLAG)
            {
                // Note that even though we are doing passthru, we had to decode
                // so that we know the stop time and the pts of the next audio
                // packet.
                out = hb_buffer_init(avp.size);
                memcpy(out->data, avp.data, avp.size);
            }
            else
            {
                AVFrameSideData *side_data;
                if ((side_data =
                     av_frame_get_side_data(pv->frame,
                                            AV_FRAME_DATA_DOWNMIX_INFO)) != NULL)
                {
                    double          surround_mix_level, center_mix_level;
                    AVDownmixInfo * downmix_info;

                    downmix_info = (AVDownmixInfo*)side_data->data;
                    if (pv->audio->config.out.mixdown == HB_AMIXDOWN_DOLBY ||
                        pv->audio->config.out.mixdown == HB_AMIXDOWN_DOLBYPLII)
                    {
                        surround_mix_level = downmix_info->surround_mix_level_ltrt;
                        center_mix_level   = downmix_info->center_mix_level_ltrt;
                    }
                    else
                    {
                        surround_mix_level = downmix_info->surround_mix_level;
                        center_mix_level   = downmix_info->center_mix_level;
                    }
                    hb_audio_resample_set_mix_levels(pv->resample,
                                                     surround_mix_level,
                                                     center_mix_level,
                                                     downmix_info->lfe_mix_level);
                }
                hb_audio_resample_set_channel_layout(pv->resample,
                                                     pv->frame->channel_layout);
                hb_audio_resample_set_sample_fmt(pv->resample,
                                                 pv->frame->format);
                if (hb_audio_resample_update(pv->resample))
                {
                    hb_log("decavcodec: hb_audio_resample_update() failed");
                    av_frame_unref(pv->frame);
                    return;
                }
                out = hb_audio_resample(pv->resample, pv->frame->extended_data,
                                        pv->frame->nb_samples);
            }

            if (out != NULL)
            {
                out->s.scr_sequence = packet_info->scr_sequence;
                out->s.start        = pv->frame->pkt_pts;
                out->s.duration     = pv->duration;
                if (out->s.start == AV_NOPTS_VALUE)
                {
                    out->s.start = pv->next_pts;
                }
                else
                {
                    pv->next_pts = out->s.start;
                }
                if (pv->next_pts != (int64_t)AV_NOPTS_VALUE)
                {
                    pv->next_pts += pv->duration;
                    out->s.stop  = pv->next_pts;
                }
                hb_buffer_list_append(&pv->list, out);

                pts = AV_NOPTS_VALUE;
            }
            av_frame_unref(pv->frame);
            ++pv->nframes;
        }
    }
}
