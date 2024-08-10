/* decavcodec.c

   Copyright (c) 2003-2024 HandBrake Team
   Copyright 2022 NVIDIA Corporation
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

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/hbavfilter.h"
#include "libavcodec/bsf.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavutil/hwcontext.h"
#include "handbrake/hwaccel.h"
#include "handbrake/lang.h"
#include "handbrake/audio_resample.h"
#include "handbrake/extradata.h"

#if HB_PROJECT_FEATURE_QSV
#include "libavutil/hwcontext_qsv.h"
#include "handbrake/qsv_common.h"
#include "handbrake/qsv_libav.h"
#endif

static void compute_frame_duration( hb_work_private_t *pv );
static int  decavcodecaInit( hb_work_object_t *, hb_job_t * );
static int  decavcodecaWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
static void decavcodecClose( hb_work_object_t * );
static int decavcodecaInfo( hb_work_object_t *, hb_work_info_t * );
static int decavcodecaBSInfo( hb_work_object_t *, const hb_buffer_t *, hb_work_info_t * );

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
    int                    discard;
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

struct video_filters_s
{
    hb_avfilter_graph_t * graph;

    int                   width;
    int                   height;
    int                   color_range;
    int                   pix_fmt;
};

struct hb_work_private_s
{
    hb_job_t             * job;
    hb_title_t           * title;
    const AVCodec        * codec;
    AVCodecContext       * context;
    AVCodecParserContext * parser;
    AVFrame              * frame;
    AVPacket             * pkt;
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
    struct video_filters_s video_filters;

    hb_audio_t           * audio;
    hb_audio_resample_t  * resample;
    int                    drop_samples;
    uint64_t               downmix_mask;

#if HB_PROJECT_FEATURE_QSV
    // QSV-specific settings
    struct
    {
        int                decode;
        hb_qsv_config      config;
        const char       * codec_name;
    } qsv;
#endif

    AVFrame              * hw_frame;
    enum AVPixelFormat     hw_pix_fmt;

    hb_list_t            * list_subtitle;
};

static void decodeAudio( hb_work_private_t *pv, packet_info_t * packet_info );

#define HB_AV_CH_SIDE_MASK (AV_CH_SIDE_LEFT|AV_CH_SIDE_RIGHT)
#define HB_AV_CH_BACK_MASK (AV_CH_BACK_LEFT|AV_CH_BACK_RIGHT)
#define HB_AV_CH_BOTH_MASK (HB_AV_CH_SIDE_MASK|HB_AV_CH_BACK_MASK)

static int downmix_required(uint64_t target_layout_mask, uint64_t input_layout_mask)
{
    /*
     * Side channels can easily be remapped to back channels and vice-versa.
     * Provided the other channels are the same, downmixing is not required.
     */
    if ((input_layout_mask & HB_AV_CH_SIDE_MASK) == 0 &&
        (input_layout_mask & HB_AV_CH_BACK_MASK) == HB_AV_CH_BACK_MASK)
    {
        if ((target_layout_mask & HB_AV_CH_BACK_MASK) == 0 &&
            (target_layout_mask & HB_AV_CH_SIDE_MASK) == HB_AV_CH_SIDE_MASK)
        {
            // input has back channels but not side channels
            // target has the opposite (sides but not backs)
            return ((input_layout_mask & ~HB_AV_CH_BOTH_MASK) !=
                    (target_layout_mask & ~HB_AV_CH_BOTH_MASK));
        }
    }
    if ((input_layout_mask & HB_AV_CH_BACK_MASK) == 0 &&
        (input_layout_mask & HB_AV_CH_SIDE_MASK) == HB_AV_CH_SIDE_MASK)
    {
        if ((target_layout_mask & HB_AV_CH_SIDE_MASK) == 0 &&
            (target_layout_mask & HB_AV_CH_BACK_MASK) == HB_AV_CH_BACK_MASK)
        {
            // input has side channels but not back channels
            // target has the opposite (backs but not sides)
            return ((input_layout_mask & ~HB_AV_CH_BOTH_MASK) !=
                    (target_layout_mask & ~HB_AV_CH_BOTH_MASK));
        }
    }
    return (input_layout_mask != target_layout_mask);
}

static uint64_t ac3_downmix_mask(int hb_mixdown, int normalized, uint64_t input_layout_mask, const char **dmix_mode)
{
    /*
     * ac3/eac3 bitstreams contain mix levels for center, surround and LFE channels.
     *
     * libavcodec's decoder can use them to build a normalized downmix matrix:
     * libavcodec/ac3dec.c static int set_downmix_coeffs()
     *
     * and downmix to either mono or stereo specifically:
     * libavcodec/ac3dsp.c static void ac3_downmix_c()
     *
     * We only do a decoder downmix here for a minor speed boost, as the mix levels
     * are otherwise available to us via AV_FRAME_DATA_DOWNMIX_INFO, which we use
     * in decodeAudio() to build the "regular" (non-normalized) downmix matrix.
     *
     * Note: the decoder ignores Lt/Rt-specific mix levels and is otherwise incapable of
     * producing a Dolby Surround/PLII-compatible downmix: only use it for normal Stereo.
     */
    if (normalized == 1)
    {
        uint64_t mask = 0;
        switch (hb_mixdown)
        {
            case HB_AMIXDOWN_MONO:
            case HB_AMIXDOWN_STEREO:
                mask = hb_ff_mixdown_xlat(hb_mixdown, NULL);
                break;

            default:
                return 0;
        }
        if (mask && downmix_required(mask, input_layout_mask))
        {
            /*
             * We also set the existing decoder option "dmix_mode" to 2 (AC3_DMIXMOD_LORO)
             * which is currently ignored by the decoder but should (theoretically) ensure
             * we always get a regular Lo/Ro downmix, if the decoder were to ever gain the
             * ability to do a Dolby/PLII downmix in the future.
             */
            *dmix_mode = "2";
            return mask;
        }
    }
    return 0;
}

static uint64_t dca_downmix_mask(int hb_mixdown, int normalized, uint64_t input_layout_mask)
{
    /*
     * AV_CODEC_ID_DTS
     *
     * For DTS-HD MA, doing a decoder downmix vs. an hb_audio_resample
     * downmix can result in significant differences e.g. in terms of
     * output bitrate using 24-bit FLAC. Letting the decoder decode
     * the full bistream at least ensures decoding is lossless, at
     * the expense of losing custom downmix coefficients that may
     * exist in the bitstream. So, no decoder downmix for DTS.
     *
     * Long-term, a solution would be to have libavcodec's DTS decoder export
     * downmix coefficients to a new type of AVFrame side data and pass that
     * through to hb_audio_resample (similar to AV_FRAME_DATA_DOWNMIX_INFO).
     */
    return 0;
}

static uint64_t truehd_downmix_mask(int hb_mixdown, int normalized, uint64_t input_layout_mask)
{
    /*
     * TrueHD bitstreams are made up of multiple "substreams" which are
     * combined in order to obtain the final output. Any given substream
     * depends on the previous substream(s), but not the next; by only
     * decoding up to specific substream, a TrueHD decoder can extract
     * an embedded downmix.
     */
    if (normalized == 0)
    {
        uint64_t mask = 0;
        switch (hb_mixdown)
        {
            /*
             * We cannot use an embedded Stereo downmix as we have no way
             * of knowing whether it is Dolby Surround or PLII-compatible.
             * Request 5.1 instead and let hb_audio_resample downmix that.
             */
            case HB_AMIXDOWN_DOLBY:
            case HB_AMIXDOWN_DOLBYPLII:
                mask = AV_CH_LAYOUT_5POINT1;
                break;

            default:
                mask = hb_ff_mixdown_xlat(hb_mixdown, NULL);
                break;
        }
        if (mask && downmix_required(mask, input_layout_mask))
        {
            if (mask == AV_CH_LAYOUT_STEREO)
            {
                /*
                 * The majority of TrueHD tracks have a Stereo first
                 * substream, even when the second substream is Mono.
                 */
                return mask;
            }
            if (hb_mixdown == HB_AMIXDOWN_MONO)
            {
                /*
                 * It is unlikely that any substream configuration will
                 * give us an embedded Mono downmix (except in the case
                 * where the full input layout is Mono, but in said case
                 * a downmix is not required) however it may be possible
                 * to extract a Stereo downmix and let hb_audio_resample
                 * take care of downmixing that to Mono for final output.
                 *
                 * Do it after downmix_required() so we don't accidentally
                 * request a Stereo downmix when the input is already Mono.
                 */
                return AV_CH_LAYOUT_STEREO;
            }
            /*
             * Which downmix(es) are possible depend on the layout for each specific substream
             * combination, but we cannot query the substream-specific layout from the decoder.
             * However, excepting the Stereo to Mono case already handled above, it should be
             * safe to assume that each additional substream contains more channels than the
             * previous one, thus a downmix should only be possible when the all-substreams
             * layout is a superset of the target layout.
             *
             * Note: when requesting a layout with fewer channels than a given substream's
             * layout but more channels than the previous substream, libavcodec's decoder
             * will give us the substream with more channels, so we don't have to worry
             * about having to accidentally upmix in hb_audio_resample down the line.
             * For example input with embedded stereo and 5.1(side) then finally 7.1,
             * and a downmix channel layout of, say, "3.1" (from an imaginary future
             * HB mixdown), the decoder would give us "5.1(side)" rather than stereo.
             */
            if (mask == (mask & input_layout_mask))
            {
                return mask;
            }
        }
    }
    return 0;
}

static char* channel_layout_name_from_mask(uint64_t mask, char *buf, size_t size)
{
    AVChannelLayout layout = { 0 };
    if (av_channel_layout_from_mask(&layout, mask) == 0 &&
        av_channel_layout_describe(&layout, buf, size) > 0)
    {
        av_channel_layout_uninit(&layout);
        return buf;
    }
    av_channel_layout_uninit(&layout);
    return NULL;
}

/***********************************************************************
 * hb_work_decavcodec_init
 ***********************************************************************
 *
 **********************************************************************/
static int decavcodecaInit( hb_work_object_t * w, hb_job_t * job )
{
    const AVCodec *codec;

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job          = job;
    pv->audio        = w->audio;
    pv->drop_samples = w->audio->config.in.encoder_delay;
    pv->next_pts     = (int64_t)AV_NOPTS_VALUE;
    if (job)
        pv->title    = job->title;
    else
        pv->title    = w->title;
    hb_buffer_list_clear(&pv->list);

    codec       = avcodec_find_decoder(w->codec_param);
    pv->context = avcodec_alloc_context3(codec);

    if (pv->title->opaque_priv != NULL)
    {
        AVFormatContext *ic = (AVFormatContext*)pv->title->opaque_priv;
        avcodec_parameters_to_context(pv->context,
                                      ic->streams[w->audio->id]->codecpar);
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

    // Set decoder opts...
    AVDictionary *av_opts = NULL;

    /* Downmixing & sample_fmt conversion */
    if (!(w->audio->config.out.codec & HB_ACODEC_PASS_FLAG))
    {
        // Currently, samplerate conversion is performed in sync.c
        // So set output samplerate to input samplerate
        // This should someday get reworked to be part of an audio
        // filter pipeline.
        pv->resample =
            hb_audio_resample_init(AV_SAMPLE_FMT_FLT,
                                   w->audio->config.in.samplerate,
                                   w->audio->config.out.mixdown,
                                   w->audio->config.out.normalize_mix_level);
        if (pv->resample == NULL)
        {
            hb_error("decavcodecaInit: hb_audio_resample_init() failed");
            return 1;
        }

        /*
         * Audio decoder downmix.
         *
         * Some codecs (e.g. truehd) contain embedded downmixes for multiple layouts.
         * Others (e.g. ac3/eac3, dca) contain embedded downmix coefficients instead.
         *
         * When applicable, configure corresponding decoder to peform the required downmix.
         */
        char mixname[256];
        char *downmix = NULL;
        uint64_t downmix_mask = 0;
        const char *dmix_mode = NULL;
        switch (w->codec_param)
        {
            case AV_CODEC_ID_AC3:
            case AV_CODEC_ID_EAC3:
                downmix_mask = ac3_downmix_mask(w->audio->config.out.mixdown,
                                                w->audio->config.out.normalize_mix_level,
                                                w->audio->config.in.channel_layout, &dmix_mode);
                break;

            case AV_CODEC_ID_DTS:
                downmix_mask = dca_downmix_mask(w->audio->config.out.mixdown,
                                                w->audio->config.out.normalize_mix_level,
                                                w->audio->config.in.channel_layout);
                break;

            case AV_CODEC_ID_TRUEHD:
                downmix_mask = truehd_downmix_mask(w->audio->config.out.mixdown,
                                                   w->audio->config.out.normalize_mix_level,
                                                   w->audio->config.in.channel_layout);
                break;

            default:
                break;
        }
        if (downmix_mask)
        {
            downmix = channel_layout_name_from_mask(downmix_mask, mixname, sizeof(mixname));
        }
        if (dmix_mode)
        {
            av_dict_set(&av_opts, "dmix_mode", dmix_mode, 0);
        }
        if (downmix)
        {
            pv->downmix_mask = downmix_mask;
            av_dict_set(&av_opts, "downmix", downmix, 0);
            hb_log("decavcodec: requesting decoder downmix '%s' for track %d", downmix, w->audio->config.out.track);
        }
    }

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
    pv->context->pkt_timebase.num = pv->audio->config.in.timebase.num;
    pv->context->pkt_timebase.den = pv->audio->config.in.timebase.den;

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

    pv->pkt = av_packet_alloc();
    if (pv->pkt == NULL)
    {
        hb_log("decavcodecaInit: av_packet_alloc failed");
        return 1;
    }

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
static void close_video_filters(hb_work_private_t *pv)
{
    hb_avfilter_graph_close(&pv->video_filters.graph);
}

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
        av_frame_free(&pv->hw_frame);
        close_video_filters(pv);
        if ( pv->parser )
        {
            av_parser_close(pv->parser);
        }
        if ( pv->context && pv->context->codec )
        {
#if HB_PROJECT_FEATURE_QSV
            /*
             * FIXME: knowingly leaked.
             *
             * If we're using our FFmpeg QSV wrapper, qsv_decode_end() will call
             * MFXClose() on the QSV session. Even if decoding is complete, we
             * still need that session for QSV filtering and/or encoding, so we
             * we can't close the context here until we implement a proper fix.
             *
             * Interestingly, this may cause crashes even when QSV-accelerated
             * decoding and encoding sessions are independent (e.g. decoding via
             * libavcodec, but encoding using libhb, without us requesting any
             * form of communication between the two libmfx sessions).
             */
            //if (!(pv->qsv.decode && pv->job != NULL && (pv->job->vcodec & HB_VCODEC_QSV_MASK)))
            hb_qsv_uninit_dec(pv->context);
#endif
            hb_avcodec_free_context(&pv->context);
        }
        if ( pv->context )
        {
            if (pv->context->hw_device_ctx)
            {
                av_buffer_unref(&pv->context->hw_device_ctx);
            }
            hb_avcodec_free_context(&pv->context);
        }
        av_packet_free(&pv->pkt);
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

static void audioParserFlush(hb_work_object_t * w)
{
    hb_work_private_t * pv = w->private_data;
    uint8_t * pout = NULL;
    int       pout_len = 0;
    int64_t   parser_pts = AV_NOPTS_VALUE;

    do
    {
        if (pv->parser)
        {
            av_parser_parse2(pv->parser, pv->context, &pout, &pout_len,
                                   NULL, 0,
                                   AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0 );
            parser_pts = pv->parser->pts;
        }

        if (pout != NULL && pout_len > 0)
        {
            pv->packet_info.data         = pout;
            pv->packet_info.size         = pout_len;
            pv->packet_info.pts          = parser_pts;

            decodeAudio(pv, &pv->packet_info);
        }
    } while (pout != NULL && pout_len > 0);
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
        audioParserFlush(w);
        decodeAudio(pv, NULL);
        hb_buffer_list_append(&pv->list, in);
        *buf_in = NULL;
        *buf_out = hb_buffer_list_clear(&pv->list);
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
        pv->packet_info.discard      = !!(in->s.flags & HB_FLAG_DISCARD);
    }
    for (pos = 0; pos < in->size; pos += len)
    {
        uint8_t * pout = NULL;
        int       pout_len = 0;
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
            // decodeAudio that is now finished.  The next packet is associated
            // with the input buffer, so set it's chapter and scr info.
            pv->packet_info.scr_sequence = in->s.scr_sequence;
            pv->packet_info.discard      = !!(in->s.flags & HB_FLAG_DISCARD);
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

static int parse_adts_extradata( hb_audio_t * audio, AVCodecContext * context,
                                 AVPacket * pkt )
{
    const AVBitStreamFilter * bsf;
    AVBSFContext            * ctx = NULL;
    int                       ret;

    if (audio == NULL)
    {
        return 1;
    }

    bsf = av_bsf_get_by_name("aac_adtstoasc");
    ret = av_bsf_alloc(bsf, &ctx);
    if (ret < 0)
    {
        hb_error("decavcodec: bitstream filter alloc failure");
        return ret;
    }
    ctx->time_base_in.num = 1;
    ctx->time_base_in.den = audio->config.out.samplerate;
    avcodec_parameters_from_context(ctx->par_in, context);
    ret = av_bsf_init(ctx);
    if (ret < 0)
    {
        hb_error("decavcodec: bitstream filter init failure");
        av_bsf_free(&ctx);
        return ret;
    }

    ret = av_bsf_send_packet(ctx, pkt);
    if (ret < 0)
    {
        hb_error("decavcodec: av_bsf_send_packet failure");
        av_bsf_free(&ctx);
        return ret;
    }

    ret = av_bsf_receive_packet(ctx, pkt);
    av_bsf_free(&ctx);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
        return 0;
    }
    else if (ret < 0)
    {
        if (ret != AVERROR_INVALIDDATA)
        {
            hb_error("decavcodec: av_bsf_receive_packet failure %x", -ret);
        }
        return ret;
    }

    if (audio->priv.extradata == NULL ||
        (audio->priv.extradata && audio->priv.extradata->size == 0))
    {
        const uint8_t * extradata;
        size_t          size;

        extradata = av_packet_get_side_data(pkt, AV_PKT_DATA_NEW_EXTRADATA,
                                            &size);
        if (extradata != NULL && size > 0)
        {
            hb_set_extradata(&audio->priv.extradata, extradata, size);
        }
    }

    return 0;
}

static int decavcodecaBSInfo( hb_work_object_t *w, const hb_buffer_t *buf,
                             hb_work_info_t *info )
{
    hb_work_private_t *pv = w->private_data;
    int result = 0, done = 0;
    hb_audio_t *audio = w->audio;

    memset( info, 0, sizeof(*info) );

    if ( pv && pv->context )
    {
        return decavcodecaInfo( w, info );
    }

    const AVCodec *codec = avcodec_find_decoder( w->codec_param );
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
        avcodec_parameters_to_context(context,
                                      ic->streams[audio->id]->codecpar);
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
    if (audio != NULL)
    {
        context->pkt_timebase.num = audio->config.in.timebase.num;
        context->pkt_timebase.den = audio->config.in.timebase.den;
    }

    av_dict_free( &av_opts );
    unsigned char *parse_buffer;
    int parse_pos, parse_buffer_size;

    int avcodec_result = 0;
    while (buf != NULL && !done)
    {
        parse_pos = 0;
        while (parse_pos < buf->size && !done)
        {
            int parse_len;

            // Start with a clean error slate on each parsing iteration
            avcodec_result = 0;
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

            if (parse_buffer_size == 0)
            {
                parse_pos += parse_len;
                continue;
            }

            AVPacket *avp = av_packet_alloc();
            avp->data = parse_buffer;
            avp->size = parse_buffer_size;
            avp->pts  = buf->s.start;
            avp->dts  = AV_NOPTS_VALUE;

            // Note: The first buffer returned by av_parser_parse2() may
            // not be aligned to start of valid codec data which can cause
            // the first call to avcodec_send_packet() to fail with
            // AVERROR_INVALIDDATA.
            avcodec_result = avcodec_send_packet(context, avp);
            if (avcodec_result < 0 && avcodec_result != AVERROR_EOF)
            {
                parse_pos += parse_len;
                av_packet_free(&avp);
                continue;
            }

            AVFrame *frame = NULL;
            do
            {
                if (frame == NULL)
                {
                    frame = av_frame_alloc();
                }
                avcodec_result = avcodec_receive_frame(context, frame);
                if (avcodec_result >= 0)
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
                    int channels = frame->ch_layout.nb_channels;

                    info->bitrate = bps * channels * info->rate.num;
                    if (info->bitrate <= 0)
                    {
                        if (context->bit_rate > 0)
                        {
                            info->bitrate = context->bit_rate;
                        }
                        else
                        {
                            info->bitrate = 1;
                        }
                    }

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
                        /*
                         * Signal that the input uses matrix encoding via
                         * channel layout for hb_mixdown_has_remix_support.
                         * The latter needs this to allow the corresponding
                         * mixdown for 2-channel matrix stereo input, said
                         * mixdown being required to signal matrix encoding
                         * in the *output* (when using e.g. the ac3 encoder).
                         *
                         * Quicker/faster than propagating side data all the
                         * way through the pipeline, but we lose the ability
                         * to distinguish between different matrix encodings.
                         *
                         * Only do this in BSInfo as overriding the layout
                         * elsewhere could break downmixing, remapping etc.
                         */
                        info->channel_layout = AV_CH_LAYOUT_STEREO_DOWNMIX;
                    }
                    else
                    {
                        info->channel_layout = frame->ch_layout.u.mask;
                    }

                    if (info->channel_layout == 0)
                    {
                        // Channel layout was not set.  Guess a layout based
                        // on number of channels.
                        AVChannelLayout channel_layout;
                        av_channel_layout_default(&channel_layout, frame->ch_layout.nb_channels);
                        info->channel_layout = channel_layout.u.mask;
                        av_channel_layout_uninit(&channel_layout);
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
                        // proper headers in MP4, WebM, and MKV files.
                        parse_adts_extradata(audio, context, avp);
                    }

                    result = 1;
                    done = 1;
                    av_frame_unref(frame);
                    break;
                }
            } while (avcodec_result >= 0);
            av_packet_free(&avp);
            av_frame_free(&frame);
            parse_pos += parse_len;
        }
        buf = buf->next;
    }

    info->profile = context->profile;
    info->level = context->level;
    info->channel_map = &hb_libav_chan_map;

    if ( parser != NULL )
        av_parser_close( parser );
    hb_avcodec_free_context(&context);
    if (!result && avcodec_result < 0 && avcodec_result != AVERROR_EOF)
    {
        result = avcodec_result;
    }
    return result;
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
    hb_buffer_t * buf = hb_buffer_init(size);

    memcpy(buf->data, cc, size);
    return buf;
}

// copy one video frame into an HB buf. If the frame isn't in our color space
// or at least one of its dimensions is odd, use sws_scale to convert/rescale it.
// Otherwise just copy the bits.
static hb_buffer_t *copy_frame( hb_work_private_t *pv )
{
    reordered_data_t * reordered = NULL;
    hb_buffer_t      * out;

#if HB_PROJECT_FEATURE_QSV
    // no need to copy the frame data when decoding with QSV to opaque memory
    if (hb_qsv_full_path_is_enabled(pv->job) && hb_qsv_get_memory_type(pv->job) == MFX_IOPATTERN_OUT_VIDEO_MEMORY)
    {
        out = hb_qsv_copy_avframe_to_video_buffer(pv->job, pv->frame, (AVRational){1,1}, 0);
    }
    else
#endif
    {
        out = hb_avframe_to_video_buffer(pv->frame, (AVRational){1,1}, 1);
    }

    if (pv->frame->pts != AV_NOPTS_VALUE)
    {
        reordered = reordered_hash_rem(pv, pv->frame->pts);
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

    double  frame_dur = pv->duration;
    if (pv->frame->repeat_pict)
    {
        frame_dur += pv->frame->repeat_pict * pv->field_duration;
    }
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
    out->s.duration  = frame_dur;

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
                iso639_lang_t * lang;
                hb_audio_t    * audio;

                subtitle        = calloc(sizeof( hb_subtitle_t ), 1);
                subtitle->track = hb_list_count(pv->title->list_subtitle);
                subtitle->id           = HB_SUBTITLE_EMBEDDED_CC_TAG;
                subtitle->format       = TEXTSUB;
                subtitle->source       = CC608SUB;
                subtitle->config.dest  = PASSTHRUSUB;
                subtitle->codec        = WORK_DECAVSUB;
                subtitle->codec_param  = AV_CODEC_ID_EIA_608;
                subtitle->attributes   = HB_SUBTITLE_ATTR_CC;
                subtitle->timebase.num = 1;
                subtitle->timebase.den = 90000;

                /*
                 * The language of the subtitles will be the same as the
                 * first audio track, i.e. the same as the video.
                 */
                audio = hb_list_item(pv->title->list_audio, 0);
                if (audio != NULL)
                {
                    lang = lang_for_code2( audio->config.lang.iso639_2 );
                } else {
                    lang = lang_for_code2( "und" );
                }
                snprintf(subtitle->lang, sizeof(subtitle->lang),
                         "%s, Closed Caption [%s]",
                         strlen(lang->native_name) ? lang->native_name :
                                                     lang->eng_name,
                         hb_subsource_name(subtitle->source));
                snprintf(subtitle->iso639_2, sizeof(subtitle->iso639_2),
                         "%s", lang->iso639_2);

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
                cc_buf->s.duration     = (int64_t)AV_NOPTS_VALUE;
                cc_buf->s.scr_sequence = out->s.scr_sequence;
            }
            cc_send_to_decoder(pv, cc_buf);
        }
    }

    if (!pv->job && pv->title)
    {
        // Check for HDR mastering data
        sd = av_frame_get_side_data(pv->frame, AV_FRAME_DATA_MASTERING_DISPLAY_METADATA);
        if (sd != NULL && sd->size > 0)
        {
            AVMasteringDisplayMetadata *mastering = (AVMasteringDisplayMetadata *)sd->data;
            pv->title->mastering = hb_mastering_ff_to_hb(*mastering);
        }

        // Check for HDR content light level data
        sd = av_frame_get_side_data(pv->frame, AV_FRAME_DATA_CONTENT_LIGHT_LEVEL);
        if (sd != NULL && sd->size > 0)
        {
            AVContentLightMetadata *coll = (AVContentLightMetadata *)sd->data;
            pv->title->coll.max_cll = coll->MaxCLL;
            pv->title->coll.max_fall = coll->MaxFALL;
        }

        // Check for HDR Plus dynamic metadata
        sd = av_frame_get_side_data(pv->frame, AV_FRAME_DATA_DYNAMIC_HDR_PLUS);
        if (sd != NULL && sd->size > 0)
        {
            pv->title->hdr_10_plus = 1;
        }

        // Check for Ambient Viewing Environment metadata
        sd = av_frame_get_side_data(pv->frame, AV_FRAME_DATA_AMBIENT_VIEWING_ENVIRONMENT);
        if (sd != NULL && sd->size > 0)
        {
            if (pv->title->ambient.ambient_illuminance.num == 0 &&
                pv->title->ambient.ambient_illuminance.den == 0)
            {
                AVAmbientViewingEnvironment *ambient = (AVAmbientViewingEnvironment *)sd->data;
                pv->title->ambient = hb_ambient_ff_to_hb(*ambient);
            }
        }
    }

    return out;
}

int reinit_video_filters(hb_work_private_t * pv)
{
    int                orig_width;
    int                orig_height;
    hb_value_array_t * filters;
    hb_dict_t        * settings;
    hb_filter_init_t   filter_init;
    enum AVPixelFormat pix_fmt;
    enum AVColorRange  color_range;

    if (!pv->job)
    {
        // HandBrake's preview pipeline uses yuv420 color.  This means all
        // dimensions must be even.  So we must adjust the dimensions
        // of incoming video if not even.
        orig_width = pv->context->width & ~1;
        orig_height = pv->context->height & ~1;
        pix_fmt = AV_PIX_FMT_YUV420P;
        color_range = AVCOL_RANGE_MPEG;
    }
    else
    {
        if (pv->job->hw_pix_fmt == AV_PIX_FMT_VIDEOTOOLBOX)
        {
            // Filtering is done in a separate filter
            return 0;
        }

        if (pv->title->rotation == HB_ROTATION_90 ||
            pv->title->rotation == HB_ROTATION_270)
        {
            orig_width = pv->job->title->geometry.height;
            orig_height = pv->job->title->geometry.width;
        }
        else
        {
            orig_width = pv->job->title->geometry.width;
            orig_height = pv->job->title->geometry.height;
        }
        pix_fmt = pv->job->hw_pix_fmt != AV_PIX_FMT_NONE ? pv->job->hw_pix_fmt : pv->job->input_pix_fmt;
        color_range = pv->job->color_range;
    }

    if (pix_fmt            == pv->frame->format  &&
        orig_width         == pv->frame->width   &&
        orig_height        == pv->frame->height  &&
        color_range        == pv->frame->color_range &&
        HB_ROTATION_0      == pv->title->rotation)
    {
        // No filtering required.
        close_video_filters(pv);
        return 0;
    }

    if (pv->video_filters.graph       != NULL              &&
        pv->video_filters.width       == pv->frame->width  &&
        pv->video_filters.height      == pv->frame->height &&
        pv->video_filters.color_range == pv->frame->color_range &&
        pv->video_filters.pix_fmt     == pv->frame->format)
    {
        // Current filter settings are good
        return 0;
    }

    pv->video_filters.width       = pv->frame->width;
    pv->video_filters.height      = pv->frame->height;
    pv->video_filters.color_range = pv->frame->color_range;
    pv->video_filters.pix_fmt     = pv->frame->format;

    // New filter required, create filter graph
    close_video_filters(pv);

    int clock_min, clock_max, clock;
    hb_rational_t vrate;

    hb_video_framerate_get_limits(&clock_min, &clock_max, &clock);
    vrate.num = clock;
    vrate.den = pv->duration * (clock / 90000.);

    filters = hb_value_array_init();
    if (pix_fmt            != pv->frame->format ||
        orig_width         != pv->frame->width  ||
        orig_height        != pv->frame->height ||
        color_range        != pv->frame->color_range)
    {
        settings = hb_dict_init();
#if HB_PROJECT_FEATURE_QSV && (defined( _WIN32 ) || defined( __MINGW32__ ))
        if (hb_qsv_full_path_is_enabled(pv->job))
        {
            hb_dict_set(settings, "w", hb_value_int(orig_width));
            hb_dict_set(settings, "h", hb_value_int(orig_height));
            hb_dict_set(settings, "format", hb_value_string(av_get_pix_fmt_name(pv->job->input_pix_fmt)));
            hb_dict_set_string(settings, "out_range", ((color_range == AVCOL_RANGE_JPEG) ? "full" : "limited"));
            hb_avfilter_append_dict(filters, "vpp_qsv", settings);
        }
        else
#endif
        if (pv->frame->hw_frames_ctx && pv->job->hw_pix_fmt == AV_PIX_FMT_CUDA)
        {
            if (color_range != pv->frame->color_range)
            {
                hb_dict_set_int(settings, "range", color_range);
                hb_avfilter_append_dict(filters, "colorspace_cuda", settings);
                settings = hb_dict_init();
            }
            hb_dict_set(settings, "w", hb_value_int(orig_width));
            hb_dict_set(settings, "h", hb_value_int(orig_height));
            hb_dict_set(settings, "interp_algo", hb_value_string("lanczos"));
            hb_dict_set(settings, "format", hb_value_string(av_get_pix_fmt_name(pv->job->input_pix_fmt)));
            hb_avfilter_append_dict(filters, "scale_cuda", settings);
        }
        else if (hb_av_can_use_zscale(pv->frame->format,
                                      pv->frame->width, pv->frame->height,
                                      orig_width, orig_height))
        {
            hb_dict_set(settings, "w", hb_value_int(orig_width));
            hb_dict_set(settings, "h", hb_value_int(orig_height));
            hb_dict_set_string(settings, "filter", "lanczos");
            hb_dict_set_string(settings, "range", av_color_range_name(color_range));
            hb_avfilter_append_dict(filters, "zscale", settings);

            settings = hb_dict_init();
            hb_dict_set(settings, "pix_fmts", hb_value_string(av_get_pix_fmt_name(pix_fmt)));
            hb_avfilter_append_dict(filters, "format", settings);
        }
        // Fallback to swscale, zscale requires a mod 2 width and height
        else
        {
            hb_dict_set(settings, "w", hb_value_int(orig_width));
            hb_dict_set(settings, "h", hb_value_int(orig_height));
            hb_dict_set(settings, "flags", hb_value_string("lanczos+accurate_rnd"));
            hb_dict_set_int(settings, "in_range", pv->frame->color_range);
            hb_dict_set_int(settings, "out_range", color_range);
            hb_avfilter_append_dict(filters, "scale", settings);

            settings = hb_dict_init();
            hb_dict_set(settings, "pix_fmts", hb_value_string(av_get_pix_fmt_name(pix_fmt)));
            hb_avfilter_append_dict(filters, "format", settings);
        }
    }
    if (pv->title->rotation != HB_ROTATION_0)
    {
#if HB_PROJECT_FEATURE_QSV
        if (hb_qsv_full_path_is_enabled(pv->job))
        {
            switch (pv->title->rotation)
            {
                case HB_ROTATION_90:
                {
                    settings = hb_dict_init();
                    hb_dict_set(settings, "transpose", hb_value_string("clock"));
                    hb_avfilter_append_dict(filters, "vpp_qsv", settings);
                    hb_log("Auto-Rotating video 90 degrees");
                    break;
                }
                case HB_ROTATION_180:
                    settings = hb_dict_init();
                    hb_dict_set(settings, "transpose", hb_value_string("reversal"));
                    hb_avfilter_append_dict(filters, "vpp_qsv", settings);
                    hb_log("Auto-Rotating video 180 degrees");
                    break;
                case HB_ROTATION_270:
                {
                    settings = hb_dict_init();
                    hb_dict_set(settings, "transpose", hb_value_string("cclock"));
                    hb_avfilter_append_dict(filters, "vpp_qsv", settings);
                    hb_log("Auto-Rotating video 270 degrees");
                    break;
                }
                default:
                    hb_log("reinit_video_filters: Unknown rotation, failed");
            }
        }
        else
#endif
        {
            switch (pv->title->rotation)
            {
                case HB_ROTATION_90:
                    settings = hb_dict_init();
                    hb_dict_set(settings, "dir", hb_value_string("cclock"));
                    hb_avfilter_append_dict(filters, "transpose", settings);
                    hb_log("Auto-Rotating video 90 degrees");
                    break;
                case HB_ROTATION_180:
                    hb_avfilter_append_dict(filters, "hflip", hb_value_null());
                    hb_avfilter_append_dict(filters, "vflip", hb_value_null());
                    hb_log("Auto-Rotating video 180 degrees");
                    break;
                case HB_ROTATION_270:
                    settings = hb_dict_init();
                    hb_dict_set(settings, "dir", hb_value_string("clock"));
                    hb_avfilter_append_dict(filters, "transpose", settings);
                    hb_log("Auto-Rotating video 270 degrees");
                    break;
                default:
                    hb_log("reinit_video_filters: Unknown rotation, failed");
            }
        }
    }

    enum AVPixelFormat sw_pix_fmt = pv->frame->format;
    enum AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;
    enum AVColorSpace color_matrix = pv->frame->colorspace;

    if (!pv->job)
    {
        // Sanitize the color_matrix when decoding preview images
        hb_rational_t par = {pv->frame->sample_aspect_ratio.num, pv->frame->sample_aspect_ratio.den};
        hb_geometry_t geo = {pv->frame->width, pv->frame->height, par};
        color_matrix = hb_get_color_matrix(pv->frame->colorspace, geo);
    }

    AVHWFramesContext *frames_ctx = NULL;
    if (pv->frame->hw_frames_ctx)
    {
        frames_ctx = (AVHWFramesContext *)pv->frame->hw_frames_ctx->data;
        sw_pix_fmt = frames_ctx->sw_format;
        hw_pix_fmt = frames_ctx->format;
    }

    memset((void*)&filter_init, 0, sizeof(filter_init));

    filter_init.job               = pv->job;
    filter_init.pix_fmt           = sw_pix_fmt;
    filter_init.hw_pix_fmt        = hw_pix_fmt;
    filter_init.geometry.width    = pv->frame->width;
    filter_init.geometry.height   = pv->frame->height;
    filter_init.geometry.par.num  = pv->frame->sample_aspect_ratio.num;
    filter_init.geometry.par.den  = pv->frame->sample_aspect_ratio.den;
    filter_init.color_matrix      = color_matrix;
    filter_init.color_range       = pv->frame->color_range;
    filter_init.time_base.num     = 1;
    filter_init.time_base.den     = 1;
    filter_init.vrate.num         = vrate.num;
    filter_init.vrate.den         = vrate.den;

    pv->video_filters.graph = hb_avfilter_graph_init(filters, &filter_init);
    hb_value_free(&filters);
    if (pv->video_filters.graph == NULL)
    {
        hb_error("reinit_video_filters: failed to create filter graph");
        goto fail;
    }

    return 0;

fail:
    close_video_filters(pv);

    return 1;
}

static void sanitize_deprecated_pix_fmts(AVFrame *frame)
{
    switch (frame->format)
    {
        case AV_PIX_FMT_YUVJ420P:
            frame->format = AV_PIX_FMT_YUV420P;
            frame->color_range = AVCOL_RANGE_JPEG;
            break;
        case AV_PIX_FMT_YUVJ422P:
            frame->format = AV_PIX_FMT_YUV422P;
            frame->color_range = AVCOL_RANGE_JPEG;
            break;
        case AV_PIX_FMT_YUVJ444P:
            frame->format = AV_PIX_FMT_YUV444P;
            frame->color_range = AVCOL_RANGE_JPEG;
            break;
        case AV_PIX_FMT_YUVJ440P:
            frame->format = AV_PIX_FMT_YUV440P;
            frame->color_range = AVCOL_RANGE_JPEG;
            break;
        case AV_PIX_FMT_YUVJ411P:
            frame->format = AV_PIX_FMT_YUV411P;
            frame->color_range = AVCOL_RANGE_JPEG;
            break;
        default:
            break;
    }
}

static void filter_video(hb_work_private_t *pv)
{
    // Make sure every frame is tagged
    if (pv->job)
    {
        pv->frame->color_primaries = pv->title->color_prim;
        pv->frame->color_trc       = pv->title->color_transfer;
        pv->frame->colorspace      = pv->title->color_matrix;
        pv->frame->color_range     = pv->title->color_range;
    }

    // J pixel formats are mostly deprecated, however
    // they are still set by decoders, breaking some filters
    sanitize_deprecated_pix_fmts(pv->frame);

    reinit_video_filters(pv);
    if (pv->video_filters.graph != NULL)
    {
        int result;

        hb_avfilter_add_frame(pv->video_filters.graph, pv->frame);
        result = hb_avfilter_get_frame(pv->video_filters.graph, pv->frame);
        while (result >= 0)
        {
            hb_buffer_t * buf = copy_frame(pv);
            hb_buffer_list_append(&pv->list, buf);
            av_frame_unref(pv->frame);
            ++pv->nframes;
            result = hb_avfilter_get_frame(pv->video_filters.graph, pv->frame);
        }
    }
    else
    {
        hb_buffer_t * buf = copy_frame(pv);
        hb_buffer_list_append(&pv->list, buf);
        av_frame_unref(pv->frame);
        ++pv->nframes;
    }
}

/*
 * Decodes a video frame from the specified raw packet data
 *      ('data', 'size').
 * The output of this function is stored in 'pv->list', which contains a list
 * of zero or more decoded packets.
 */
static int decodeFrame( hb_work_private_t * pv, packet_info_t * packet_info )
{
    int got_picture = 0, oldlevel = 0, ret;
    AVPacket *avp = pv->pkt;
    reordered_data_t * reordered;
    AVFrame *recv_frame = pv->frame;

    if (pv->hw_frame)
    {
        recv_frame = pv->hw_frame;
    }

    if ( global_verbosity_level <= 1 )
    {
        oldlevel = av_log_get_level();
        av_log_set_level( AV_LOG_QUIET );
    }

    if (packet_info != NULL)
    {
        avp->data = packet_info->data;
        avp->size = packet_info->size;
        avp->pts  = pv->sequence;
        avp->dts  = pv->sequence;
        reordered = malloc(sizeof(*reordered));
        if (reordered != NULL)
        {
            reordered->sequence     = pv->sequence++;
            reordered->pts          = packet_info->pts;
            reordered->scr_sequence = packet_info->scr_sequence;
            reordered->new_chap     = packet_info->new_chap;
            reordered_hash_add(pv, reordered);
        }

        // libav avcodec video decoder needs AVPacket flagged with
        // AV_PKT_FLAG_KEY for some codecs. For example, sequence of
        // PNG in a mov container.
        if (packet_info->frametype & HB_FRAME_MASK_KEY)
        {
            avp->flags |= AV_PKT_FLAG_KEY;
        }
        avp->flags  |= packet_info->discard * AV_PKT_FLAG_DISCARD;
    }
    else
    {
        avp->data = NULL;
        avp->size = 0;
    }

    if (pv->palette != NULL)
    {
        uint8_t * palette;
        int size;
        palette = av_packet_new_side_data(avp, AV_PKT_DATA_PALETTE,
                                          AVPALETTE_SIZE);
        size = MIN(pv->palette->size, AVPALETTE_SIZE);
        memcpy(palette, pv->palette->data, size);
        hb_buffer_close(&pv->palette);
    }

    ret = avcodec_send_packet(pv->context, avp);
    av_packet_unref(avp);
    if (ret < 0 && ret != AVERROR_EOF)
    {
        ++pv->decode_errors;
        return 0;
    }

    do
    {
        ret = avcodec_receive_frame(pv->context, recv_frame);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
        {
            ++pv->decode_errors;
        }
        if (ret < 0)
        {
            break;
        }
        got_picture = 1;

        if (pv->hw_frame)
        {
            if (pv->hw_frame->hw_frames_ctx)
            {
                ret = av_hwframe_transfer_data(pv->frame, pv->hw_frame, 0);
                av_frame_copy_props(pv->frame, pv->hw_frame);
                av_frame_unref(pv->hw_frame);
                if (ret < 0)
                {
                    hb_error("decavcodec: error transferring data to system memory");
                    break;
                }
            }
            else
            {
                // HWAccel falled back to the software decoder
                av_frame_ref(pv->frame, pv->hw_frame);
                av_frame_unref(pv->hw_frame);
                if (ret < 0)
                {
                    hb_error("decavcodec: error hwaccel copying frame");
                }
            }
        }

        // recompute the frame/field duration, because sometimes it changes
        compute_frame_duration( pv );
        filter_video(pv);
    } while (ret >= 0);

    if ( global_verbosity_level <= 1 )
    {
        av_log_set_level( oldlevel );
    }

    return got_picture;
}


static int decavcodecvInit( hb_work_object_t * w, hb_job_t * job )
{

    hb_work_private_t *pv = calloc( 1, sizeof( hb_work_private_t ) );

    w->private_data = pv;
    pv->job         = job;
    pv->next_pts    = (int64_t)AV_NOPTS_VALUE;
    pv->hw_pix_fmt  = AV_PIX_FMT_NONE;
    if ( job )
        pv->title = job->title;
    else
        pv->title = w->title;
    if (pv->title->flags & HBTF_RAW_VIDEO)
        pv->next_pts = 0;
    hb_buffer_list_clear(&pv->list);

#if HB_PROJECT_FEATURE_QSV
    if ((pv->qsv.decode = hb_qsv_decode_is_enabled(job)))
    {
        pv->qsv.codec_name = hb_qsv_decode_get_codec_name(w->codec_param);
        pv->qsv.config.io_pattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
        if(hb_qsv_get_memory_type(job) == MFX_IOPATTERN_OUT_VIDEO_MEMORY)
        {
            hb_qsv_info_t *info = hb_qsv_encoder_info_get(hb_qsv_get_adapter_index(), job->vcodec);
            if (info != NULL)
            {
                // setup the QSV configuration
                pv->qsv.config.io_pattern         = MFX_IOPATTERN_OUT_VIDEO_MEMORY;
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
                if (!pv->job->qsv.ctx)
                {
                    hb_error( "decavcodecvInit: no context" );
                    return 1;
                }
                pv->job->qsv.ctx->full_path_is_enabled = 1;
                if (!pv->job->qsv.ctx->hb_dec_qsv_frames_ctx)
                {
                    pv->job->qsv.ctx->hb_dec_qsv_frames_ctx = av_mallocz(sizeof(HBQSVFramesContext));
                    if(!pv->job->qsv.ctx->hb_dec_qsv_frames_ctx)
                    {
                        hb_error( "decavcodecvInit: HBQSVFramesContext dec alloc failed" );
                        return 1;
                    }
                }
                if (!pv->job->qsv.ctx->dec_space)
                {
                    pv->job->qsv.ctx->dec_space = av_mallocz(sizeof(hb_qsv_space));
                    if(!pv->job->qsv.ctx->dec_space)
                    {
                        hb_error( "decavcodecvInit: dec_space alloc failed" );
                        return 1;
                    }
                    pv->job->qsv.ctx->dec_space->is_init_done = 1;
                }
            }
        }
    }
#endif

    if( pv->job && pv->job->title && !pv->job->title->has_resolution_change )
    {
        pv->threads = HB_FFMPEG_THREADS_AUTO;
    }

#if HB_PROJECT_FEATURE_QSV
    if (pv->qsv.decode)
    {
        pv->codec = avcodec_find_decoder_by_name(pv->qsv.codec_name);
    }
    else
#endif
    {
        pv->codec = avcodec_find_decoder(w->codec_param);
    }
    if ( pv->codec == NULL )
    {
        hb_log( "decavcodecvInit: failed to find codec for id (%d)", w->codec_param );
        return 1;
    }

    pv->context = avcodec_alloc_context3( pv->codec );
    pv->context->workaround_bugs = FF_BUG_AUTODETECT;
    pv->context->err_recognition = AV_EF_CRCCHECK;
    pv->context->error_concealment = FF_EC_GUESS_MVS|FF_EC_DEBLOCK;

    if (w->hw_device_ctx)
    {
        pv->context->get_format = hw_hwaccel_get_hw_format;
        pv->context->opaque = job;
        av_buffer_replace(&pv->context->hw_device_ctx, w->hw_device_ctx);

        if (job == NULL ||
            (job->hw_pix_fmt == AV_PIX_FMT_NONE && job->hw_decode & HB_DECODE_SUPPORT_FORCE_HW))
        {
            pv->hw_frame = av_frame_alloc();
        }
    }

    if ( pv->title->opaque_priv )
    {
        AVFormatContext *ic = (AVFormatContext*)pv->title->opaque_priv;

        avcodec_parameters_to_context(pv->context,
                                  ic->streams[pv->title->video_id]->codecpar);

#if HB_PROJECT_FEATURE_QSV
        if (pv->qsv.decode &&
            pv->qsv.config.io_pattern == MFX_IOPATTERN_OUT_VIDEO_MEMORY)
        {
            // assign callbacks and job to have access to qsv context from ffmpeg
            pv->context->get_format      = hb_qsv_get_format;
            pv->context->get_buffer2     = hb_qsv_get_buffer;
            pv->context->opaque          = pv->job;
            pv->context->hwaccel_context = 0;
        }
#endif

        // Set decoder opts
        AVDictionary * av_opts = NULL;
        if (pv->title->flags & HBTF_NO_IDR)
        {
            av_dict_set( &av_opts, "flags", "output_corrupt", 0 );
        }

#if HB_PROJECT_FEATURE_QSV
        if (pv->qsv.decode)
        {
            if (pv->context->codec_id == AV_CODEC_ID_HEVC)
                av_dict_set( &av_opts, "load_plugin", "hevc_hw", 0 );
#if defined(_WIN32) || defined(__MINGW32__)
            if (hb_qsv_get_memory_type(job) == MFX_IOPATTERN_OUT_SYSTEM_MEMORY)
            {
                hb_qsv_device_init(job);
                pv->context->hw_device_ctx = av_buffer_ref(job->qsv.ctx->hb_hw_device_ctx);
                pv->context->get_format = hb_qsv_get_format;
                pv->context->opaque = pv->job;
            }
#endif
        }
#endif

        if ( hb_avcodec_open( pv->context, pv->codec, &av_opts, pv->threads ) )
        {
            av_dict_free( &av_opts );
            hb_log( "decavcodecvInit: avcodec_open failed" );
            return 1;
        }
        pv->context->pkt_timebase.num = pv->title->video_timebase.num;
        pv->context->pkt_timebase.den = pv->title->video_timebase.den;
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

    pv->pkt = av_packet_alloc();
    if (pv->pkt == NULL)
    {
        hb_log("decavcodecvInit: av_packet_alloc failed");
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
            if (subtitle->source == CC608SUB &&
                subtitle->id == HB_SUBTITLE_EMBEDDED_CC_TAG)
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

static int setup_extradata( hb_work_private_t * pv, AVCodecContext * context )
{
    const AVBitStreamFilter * bsf;
    AVBSFContext            * ctx = NULL;
    int                       ii, ret;
    AVPacket                * avp = pv->pkt;
    const enum AVCodecID    * ids;

    if (context->extradata != NULL)
    {
        return 0;
    }
    bsf = av_bsf_get_by_name("extract_extradata");
    if (bsf == NULL)
    {
        hb_error("setup_extradata: bitstream filter lookup failure");
        return 0;
    }
    if (bsf->codec_ids == NULL)
    {
        hb_error("setup_extradata: extract_extradata missing codec_ids");
        return 0;
    }
    for (ids = bsf->codec_ids; *ids != AV_CODEC_ID_NONE; ids++)
    {
        if (*ids == context->codec_id)
        {
            break;
        }
    }
    if (*ids == AV_CODEC_ID_NONE)
    {
        // Codec not supported by extract_extradata BSF
        return 0;
    }
    ret = av_bsf_alloc(bsf, &ctx);
    if (ret < 0)
    {
        hb_error("setup_extradata: bitstream filter alloc failure");
        return 0;
    }
    avcodec_parameters_from_context(ctx->par_in, context);
    ret = av_bsf_init(ctx);
    if (ret < 0)
    {
        hb_error("setup_extradata: bitstream filter init failure");
        av_bsf_free(&ctx);
        return 0;
    }

    avp->data = pv->packet_info.data;
    avp->size = pv->packet_info.size;
    avp->pts  = pv->sequence;
    avp->dts  = pv->sequence;
    ret = av_bsf_send_packet(ctx, avp);
    if (ret < 0)
    {
        hb_error("setup_extradata: av_bsf_send_packet failure");
        av_bsf_free(&ctx);
        return 0;
    }

    ret = av_bsf_receive_packet(ctx, avp);
    av_bsf_free(&ctx);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
        return 1;
    }
    else if (ret < 0)
    {
        if (ret != AVERROR_INVALIDDATA)
        {
            hb_error("setup_extradata: av_bsf_receive_packet failure %x", -ret);
        }
        return ret;
    }
    for (ii = 0; ii < avp->side_data_elems; ii++)
    {
        if (avp->side_data[ii].type == AV_PKT_DATA_NEW_EXTRADATA)
        {
            context->extradata_size = avp->side_data[ii].size;
            context->extradata = av_malloc(context->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);

            if (context->extradata != NULL)
            {
                memcpy(context->extradata, avp->side_data[ii].data, context->extradata_size);
                av_packet_unref(avp);
                return 0;
            }
        }
    }
    av_packet_unref(avp);

    return 1;
}

static int decodePacket( hb_work_object_t * w )
{
    hb_work_private_t * pv     = w->private_data;

    // if this is the first frame open the codec (we have to wait for the
    // first frame because of M$ VC1 braindamage).
    if ( !pv->video_codec_opened )
    {
        AVCodecContext * context = avcodec_alloc_context3(pv->codec);
        if (setup_extradata(pv, context))
        {
            // we didn't find the headers needed to set up extradata.
            // the codec will abort if we open it so just free the buf
            // and hope we eventually get the info we need.
            hb_avcodec_free_context(&context);
            return HB_WORK_OK;
        }

        hb_avcodec_free_context(&pv->context);
        pv->context = context;

        pv->context->workaround_bugs   = FF_BUG_AUTODETECT;
        pv->context->err_recognition   = AV_EF_CRCCHECK;
        pv->context->error_concealment = FF_EC_GUESS_MVS|FF_EC_DEBLOCK;

        if (w->hw_device_ctx)
        {
            int ret = av_buffer_replace(&pv->context->hw_device_ctx, w->hw_device_ctx);
            if (ret < 0)
            {
                return HB_WORK_ERROR;
            }
        }

#if HB_PROJECT_FEATURE_QSV
        if (pv->qsv.decode &&
            pv->qsv.config.io_pattern == MFX_IOPATTERN_OUT_VIDEO_MEMORY)
        {
            // set the QSV configuration before opening the decoder
            pv->context->hwaccel_context = &pv->qsv.config;
        }
#endif

        AVDictionary * av_opts = NULL;
        if (pv->title->flags & HBTF_NO_IDR)
        {
            av_dict_set( &av_opts, "flags", "output_corrupt", 0 );
        }

        // disable threaded decoding for scan, can cause crashes
        if ( hb_avcodec_open( pv->context, pv->codec, &av_opts, pv->threads ) )
        {
            av_dict_free( &av_opts );
            hb_log( "decavcodecvWork: avcodec_open failed" );
            // avcodec_open can fail due to incorrectly parsed extradata
            // so try again when this fails
            av_freep( &pv->context->extradata );
            pv->context->extradata_size = 0;
            return HB_WORK_OK;
        }
        pv->context->pkt_timebase.num = pv->title->video_timebase.num;
        pv->context->pkt_timebase.den = pv->title->video_timebase.den;
        av_dict_free( &av_opts );
        pv->video_codec_opened = 1;
    }

    decodeFrame(pv, &pv->packet_info);

    return HB_WORK_OK;
}

static void videoParserFlush(hb_work_object_t * w)
{
    hb_work_private_t * pv = w->private_data;
    int       result;
    uint8_t * pout = NULL;
    int       pout_len = 0;
    int64_t   parser_pts = AV_NOPTS_VALUE;
    int64_t   parser_dts = AV_NOPTS_VALUE;

    do
    {
        if (pv->parser)
        {
            av_parser_parse2(pv->parser, pv->context, &pout, &pout_len,
                                   NULL, 0,
                                   AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0 );
            parser_pts = pv->parser->pts;
            parser_dts = pv->parser->dts;
        }

        if (pout != NULL && pout_len > 0)
        {
            pv->packet_info.data         = pout;
            pv->packet_info.size         = pout_len;
            pv->packet_info.pts          = parser_pts;
            pv->packet_info.dts          = parser_dts;

            result = decodePacket(w);
            if (result != HB_WORK_OK)
            {
                break;
            }
            w->frame_count++;
        }
    } while (pout != NULL && pout_len > 0);
}

static int decavcodecvWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                            hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv     = w->private_data;
    hb_buffer_t       * in     = *buf_in;
    int                 result = HB_WORK_OK;

    *buf_out = NULL;

    // libavcodec/mpeg12dec.c requires buffers to be zero padded.
    // If not zero padded, it can get stuck in an infinite loop.
    // It's likely there are other decoders that expect the same.
    if (in->data != NULL)
    {
        memset(in->data + in->size, 0, in->alloc - in->size);
    }
    if (in->palette != NULL)
    {
        pv->palette = in->palette;
        in->palette = NULL;
    }

    /* if we got an empty buffer signaling end-of-stream send it downstream */
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        if (pv->context != NULL && pv->context->codec != NULL)
        {
            videoParserFlush(w);
            while (decodeFrame(pv, NULL))
            {
                continue;
            }
        }
        hb_buffer_list_append(&pv->list, hb_buffer_dup(in));
        *buf_out = hb_buffer_list_clear(&pv->list);
        return HB_WORK_DONE;
    }

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
        pv->packet_info.discard      = !!(in->s.flags & HB_FLAG_DISCARD);
    }
    for (pos = 0; pos < in->size; pos += len)
    {
        uint8_t * pout = NULL;
        int       pout_len = 0;
        int64_t   parser_pts, parser_dts;

        if (pv->parser)
        {
            int codec_id = pv->context->codec_id;
            len = av_parser_parse2(pv->parser, pv->context, &pout, &pout_len,
                                   in->data + pos, in->size - pos,
                                   pts, dts, 0 );
            parser_pts = pv->parser->pts;
            parser_dts = pv->parser->dts;
            pts = AV_NOPTS_VALUE;
            dts = AV_NOPTS_VALUE;

            if (codec_id != pv->context->codec_id)
            {
                // The parser has decided to change the decoder underneath
                // us.  Update our context to match.  This can happen
                // for MPEG-1/2 video, perhaps others
                pv->codec = avcodec_find_decoder(pv->context->codec_id);
            }
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

            result = decodePacket(w);
            if (result != HB_WORK_OK)
            {
                break;
            }
            w->frame_count++;

            // There could have been an unfinished packet that is now finished.
            // The next packet is associated with the input buffer, so set
            // it's chapter and scr info.
            pv->packet_info.scr_sequence = in->s.scr_sequence;
            pv->packet_info.new_chap     = in->s.new_chap;
            pv->packet_info.frametype    = in->s.frametype;
            pv->packet_info.discard      = !!(in->s.flags & HB_FLAG_DISCARD);
            pv->unfinished               = 0;
        }
        if (len > 0 && pout_len <= 0)
        {
            pv->unfinished = 1;
        }
    }
    *buf_out = hb_buffer_list_clear(&pv->list);

    return result;
}

static void compute_frame_duration( hb_work_private_t *pv )
{
    int64_t max_fps = 256LL;
    int64_t min_fps = 8LL;
    double duration = 0.;

    // context->time_base may be in fields, so set the max *fields* per second
    const AVCodecDescriptor *desc = avcodec_descriptor_get(pv->context->codec_id);
    int ticks_per_frame = desc && (desc->props & AV_CODEC_PROP_FIELDS) ? 2 : 1;

    if (pv->title->opaque_priv)
    {
        // If ffmpeg is demuxing for us, it collects some additional
        // information about framerates that is often accurate
        AVFormatContext *ic = (AVFormatContext*)pv->title->opaque_priv;
        AVStream *st = ic->streams[pv->title->video_id];
        if (st->nb_frames && st->duration > 0)
        {
            // compute the average frame duration from the total number
            // of frames & the total duration.
            duration = ( (double)st->duration * (double)st->time_base.num ) /
                       ( (double)st->nb_frames * (double)st->time_base.den );
        }
        else
        {
            AVRational *tb = NULL;
            // We don't have a frame count or duration so try to use the
            // far less reliable avg_frame_rate info in the stream.
            if (st->avg_frame_rate.den && st->avg_frame_rate.num)
            {
                tb = &(st->avg_frame_rate);
            }
            // Try r_frame_rate, which is usually set for cfr streams
            else if (st->r_frame_rate.num && st->r_frame_rate.den)
            {
                tb = &(st->r_frame_rate);
            }
            // Because the time bases are so screwed up, we only take values
            // in a restricted range.
            else if (st->time_base.num * max_fps > st->time_base.den &&
                     st->time_base.den > st->time_base.num * min_fps)
            {
                tb = &(st->time_base);
            }

            if (tb != NULL)
            {
                duration = (double)tb->den / (double)tb->num;
            }
        }
    }
    else if (pv->context->framerate.num && pv->context->framerate.den)
    {
        duration = (double)pv->context->framerate.den / (double)pv->context->framerate.num;
    }

    int clock_min, clock_max, clock;
    hb_video_framerate_get_limits(&clock_min, &clock_max, &clock);

    if (duration == 0 || duration > INT_MAX / clock || duration < 1. / clock)
    {
        // No valid timing info found in the stream
        // or not representable, probably a broken file, so pick some value
        duration = 1001. / 24000.;
    }

    pv->duration = duration * 90000.;
    pv->field_duration = pv->duration;
    if ( ticks_per_frame > 1 )
    {
        pv->field_duration /= ticks_per_frame;
    }
}

static int decavcodecvInfo( hb_work_object_t *w, hb_work_info_t *info )
{
    hb_work_private_t *pv = w->private_data;

    int clock_min, clock_max, clock;
    hb_video_framerate_get_limits(&clock_min, &clock_max, &clock);

    memset( info, 0, sizeof(*info) );

    if (pv->context == NULL || pv->context->codec == NULL)
        return 0;

    info->bitrate = pv->context->bit_rate;
    if (w->title->rotation == HB_ROTATION_90 ||
        w->title->rotation == HB_ROTATION_270)
    {
        // HandBrake's video pipeline uses yuv420 color.  This means all
        // dimensions must be even.  So we must adjust the dimensions
        // of incoming video if not even.
        info->geometry.width = pv->context->height & ~1;
        info->geometry.height = pv->context->width & ~1;

        info->geometry.par.num = pv->context->sample_aspect_ratio.den;
        info->geometry.par.den = pv->context->sample_aspect_ratio.num;
    }
    else
    {
        // HandBrake's video pipeline uses yuv420 color.  This means all
        // dimensions must be even.  So we must adjust the dimensions
        // of incoming video if not even.
        info->geometry.width = pv->context->width & ~1;
        info->geometry.height = pv->context->height & ~1;

        info->geometry.par.num = pv->context->sample_aspect_ratio.num;
        info->geometry.par.den = pv->context->sample_aspect_ratio.den;
    }

    compute_frame_duration( pv );
    info->rate.num = clock;
    info->rate.den = pv->duration * (clock / 90000.);

    info->profile = pv->context->profile;
    info->level = pv->context->level;
    info->name = pv->context->codec->name;

    info->pix_fmt        = pv->context->sw_pix_fmt != AV_PIX_FMT_NONE ? pv->context->sw_pix_fmt : pv->context->pix_fmt;
    info->color_prim     = pv->context->color_primaries;
    info->color_transfer = pv->context->color_trc;
    info->color_matrix   = pv->context->colorspace;
    info->color_range     = pv->context->color_range;
    info->chroma_location = pv->context->chroma_sample_location;

    info->video_decode_support = HB_DECODE_SUPPORT_SW;

#if HB_PROJECT_FEATURE_QSV
    if (hb_qsv_available())
    {
        if (hb_qsv_decode_is_codec_supported(hb_qsv_get_adapter_index(), pv->context->codec_id, pv->context->pix_fmt, pv->context->width, pv->context->height))
        {
            info->video_decode_support |= HB_DECODE_SUPPORT_QSV;
        }
    }
#endif

    if (pv->context->pix_fmt == AV_PIX_FMT_CUDA)
    {
        info->video_decode_support |= HB_DECODE_SUPPORT_NVDEC;
    }
    else if (pv->context->pix_fmt == AV_PIX_FMT_VIDEOTOOLBOX)
    {
        info->video_decode_support |= HB_DECODE_SUPPORT_VIDEOTOOLBOX;
    }
    else if (pv->context->pix_fmt == AV_PIX_FMT_D3D11)
    {
        info->video_decode_support |= HB_DECODE_SUPPORT_MF;
    }

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
            hb_avcodec_free_context(&pv->context);
            if ( pv->parser )
            {
                av_parser_close(pv->parser);
            }
            pv->parser = av_parser_init( w->codec_param );
            pv->context = avcodec_alloc_context3( pv->codec );
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

static void log_decoder_downmix_mismatch(uint64_t downmix_mask, uint64_t decoded_mask)
{
    char buf[2][256];
    char *req = channel_layout_name_from_mask(downmix_mask, buf[0], sizeof(buf[0]));
    char *got = channel_layout_name_from_mask(decoded_mask, buf[1], sizeof(buf[1]));
    hb_deep_log(2,
                "decavcodec: requested channel layout '%s' via decoder downmix "
                "but decoded audio has channel layout '%s' instead",
                req ? req : "(null)", got ? got : "(null)");
}

static void decodeAudio(hb_work_private_t *pv, packet_info_t * packet_info)
{
    AVCodecContext * context = pv->context;
    AVPacket       * avp = pv->pkt;
    int              ret;

    // libav does not supply timestamps for wmapro audio (possibly others)
    // if there is an input timestamp, initialize next_pts
    if (pv->next_pts     == (int64_t)AV_NOPTS_VALUE &&
        packet_info      != NULL &&
        packet_info->pts != AV_NOPTS_VALUE)
    {
        pv->next_pts = packet_info->pts;
    }
    if (packet_info != NULL)
    {
        avp->data = packet_info->data;
        avp->size = packet_info->size;
        avp->pts  = packet_info->pts;
        avp->dts  = AV_NOPTS_VALUE;
        avp->flags |= packet_info->discard * AV_PKT_FLAG_DISCARD;
    }
    else
    {
        avp->data = NULL;
        avp->size = 0;
    }

    ret = avcodec_send_packet(context, avp);
    if (ret < 0 && ret != AVERROR_EOF)
    {
        av_packet_unref(avp);
        return;
    }

    do
    {
        ret = avcodec_receive_frame(context, pv->frame);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
        {
            ++pv->decode_errors;
        }
        if (ret < 0)
        {
            break;
        }

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

        int64_t pts = pv->frame->pts;
        double  duration = pv->duration;

        if (pv->audio->config.out.codec & HB_ACODEC_PASS_FLAG)
        {
            // Note that even though we are doing passthru, we had to decode
            // so that we know the stop time and the pts of the next audio
            // packet.
            out = hb_buffer_init(avp->size);
            memcpy(out->data, avp->data, avp->size);
        }
        else
        {
            AVFrameSideData *side_data;
            AVChannelLayout  channel_layout;
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
            channel_layout = pv->frame->ch_layout;
            if (pv->downmix_mask && pv->downmix_mask != channel_layout.u.mask)
            {
                log_decoder_downmix_mismatch(pv->downmix_mask, channel_layout.u.mask);
                pv->downmix_mask = 0; // don't spam the log
            }
            if (channel_layout.order != AV_CHANNEL_ORDER_NATIVE || channel_layout.u.mask == 0)
            {
                AVChannelLayout default_ch_layout;
                av_channel_layout_default(&default_ch_layout, pv->frame->ch_layout.nb_channels);
                channel_layout = default_ch_layout;
            }
            hb_audio_resample_set_ch_layout(pv->resample, &channel_layout);
            hb_audio_resample_set_sample_fmt(pv->resample,
                                             pv->frame->format);
            hb_audio_resample_set_sample_rate(pv->resample,
                                             pv->frame->sample_rate);
            if (hb_audio_resample_update(pv->resample))
            {
                hb_log("decavcodec: hb_audio_resample_update() failed");
                av_frame_unref(pv->frame);
                av_packet_unref(avp);
                return;
            }
            out = hb_audio_resample(pv->resample,
                                    (const uint8_t **)pv->frame->extended_data,
                                    pv->frame->nb_samples);
            if (out != NULL && pv->drop_samples > 0)
            {
                /* drop audio samples that are part of the encoder delay */
                int channels = hb_mixdown_get_discrete_channel_count(
                                                pv->audio->config.out.mixdown);
                int sample_size = channels * sizeof(float);
                int samples = out->size / sample_size;
                if (samples <= pv->drop_samples)
                {
                    hb_buffer_close(&out);
                    pv->drop_samples -= samples;
                }
                else
                {
                    int size = pv->drop_samples * sample_size;
                    double drop_duration = pv->drop_samples * 90000L /
                                           pv->audio->config.out.samplerate;
                    memmove(out->data, out->data + size, out->size - size);
                    out->size -= size;
                    pts += drop_duration;
                    duration -= drop_duration;
                    pv->drop_samples = 0;
                }
            }
        }

        if (out != NULL)
        {
            if (packet_info != NULL)
            {
                out->s.scr_sequence = packet_info->scr_sequence;
            }
            out->s.start        = pts;
            out->s.duration     = duration;
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
        }
        av_frame_unref(pv->frame);
        ++pv->nframes;
    } while (ret >= 0);
    av_packet_unref(avp);
}
