#include "hb.h"
#include "hbffmpeg.h"

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

void hb_avframe_set_video_buffer_flags(hb_buffer_t * buf, AVFrame *frame,
                                       AVRational time_base)
{
    if (buf == NULL || frame == NULL)
    {
        return;
    }

    buf->s.start = av_rescale_q(frame->pts, time_base, (AVRational){1, 90000});
    buf->s.duration = frame->reordered_opaque;

    if (frame->top_field_first)
    {
        buf->s.flags |= PIC_FLAG_TOP_FIELD_FIRST;
    }
    if (!frame->interlaced_frame)
    {
        buf->s.flags |= PIC_FLAG_PROGRESSIVE_FRAME;
    }
    else
    {
        buf->s.combed = HB_COMB_HEAVY;
    }
    if (frame->repeat_pict == 1)
    {
        buf->s.flags |= PIC_FLAG_REPEAT_FIRST_FIELD;
    }
    if (frame->repeat_pict == 2)
    {
        buf->s.flags |= PIC_FLAG_REPEAT_FRAME;
    }
    buf->s.frametype = get_frame_type(frame->pict_type);
}

hb_buffer_t * hb_avframe_to_video_buffer(AVFrame *frame, AVRational time_base)
{
    hb_buffer_t * buf;

    buf = hb_frame_buffer_init(frame->format, frame->width, frame->height);
    if (buf == NULL)
    {
        return NULL;
    }

    hb_avframe_set_video_buffer_flags(buf, frame, time_base);

    int pp;
    for (pp = 0; pp < 3; pp++)
    {
        int yy;
        int width     = buf->plane[pp].width;
        int stride    = buf->plane[pp].stride;
        int height    = buf->plane[pp].height;
        int linesize  = frame->linesize[pp];
        uint8_t * dst = buf->plane[pp].data;
        uint8_t * src = frame->data[pp];

        for (yy = 0; yy < height; yy++)
        {
            memcpy(dst, src, width);
            dst += stride;
            src += linesize;
        }
    }

    return buf;
}

static int handle_jpeg(enum AVPixelFormat *format)
{
    switch (*format)
    {
        case AV_PIX_FMT_YUVJ420P: *format = AV_PIX_FMT_YUV420P; return 1;
        case AV_PIX_FMT_YUVJ422P: *format = AV_PIX_FMT_YUV422P; return 1;
        case AV_PIX_FMT_YUVJ444P: *format = AV_PIX_FMT_YUV444P; return 1;
        case AV_PIX_FMT_YUVJ440P: *format = AV_PIX_FMT_YUV440P; return 1;
        default:                                                return 0;
    }
}

struct SwsContext*
hb_sws_get_context(int srcW, int srcH, enum AVPixelFormat srcFormat,
                   int dstW, int dstH, enum AVPixelFormat dstFormat,
                   int flags, int colorspace)
{
    struct SwsContext * ctx;

    ctx = sws_alloc_context();
    if ( ctx )
    {
        int srcRange, dstRange;

        srcRange = handle_jpeg(&srcFormat);
        dstRange = handle_jpeg(&dstFormat);
        flags |= SWS_FULL_CHR_H_INT | SWS_FULL_CHR_H_INP;

        av_opt_set_int(ctx, "srcw", srcW, 0);
        av_opt_set_int(ctx, "srch", srcH, 0);
        av_opt_set_int(ctx, "src_range", srcRange, 0);
        av_opt_set_int(ctx, "src_format", srcFormat, 0);
        av_opt_set_int(ctx, "dstw", dstW, 0);
        av_opt_set_int(ctx, "dsth", dstH, 0);
        av_opt_set_int(ctx, "dst_range", dstRange, 0);
        av_opt_set_int(ctx, "dst_format", dstFormat, 0);
        av_opt_set_int(ctx, "sws_flags", flags, 0);

        sws_setColorspaceDetails( ctx,
                      sws_getCoefficients( colorspace ), // src colorspace
                      srcRange, // src range 0 = MPG, 1 = JPG
                      sws_getCoefficients( colorspace ), // dst colorspace
                      dstRange, // dst range 0 = MPG, 1 = JPG
                      0,         // brightness
                      1 << 16,   // contrast
                      1 << 16 ); // saturation

        if (sws_init_context(ctx, NULL, NULL) < 0) {
            hb_error("Cannot initialize resampling context");
            sws_freeContext(ctx);
            ctx = NULL;
        }
    }
    return ctx;
}

int hb_ff_get_colorspace(int color_matrix)
{
    int color_space = SWS_CS_DEFAULT;

    switch (color_matrix)
    {
        case HB_COLR_MAT_SMPTE170M:
            color_space = SWS_CS_ITU601;
            break;
        case HB_COLR_MAT_SMPTE240M:
            color_space = SWS_CS_SMPTE240M;
            break;
        case HB_COLR_MAT_BT709:
            color_space = SWS_CS_ITU709;
            break;
        /* enable this when implemented in FFmpeg
        case HB_COLR_MAT_BT2020:
            color_space = SWS_CS_BT2020;
            break;
         */
        default:
            break;
    }

    return color_space;
}

uint64_t hb_ff_mixdown_xlat(int hb_mixdown, int *downmix_mode)
{
    uint64_t ff_layout = 0;
    int mode = AV_MATRIX_ENCODING_NONE;
    switch (hb_mixdown)
    {
        // Passthru
        case HB_AMIXDOWN_NONE:
            break;

        case HB_AMIXDOWN_MONO:
        case HB_AMIXDOWN_LEFT:
        case HB_AMIXDOWN_RIGHT:
            ff_layout = AV_CH_LAYOUT_MONO;
            break;

        case HB_AMIXDOWN_DOLBY:
            ff_layout = AV_CH_LAYOUT_STEREO;
            mode = AV_MATRIX_ENCODING_DOLBY;
            break;

        case HB_AMIXDOWN_DOLBYPLII:
            ff_layout = AV_CH_LAYOUT_STEREO;
            mode = AV_MATRIX_ENCODING_DPLII;
            break;

        case HB_AMIXDOWN_STEREO:
            ff_layout = AV_CH_LAYOUT_STEREO;
            break;

        case HB_AMIXDOWN_5POINT1:
            ff_layout = AV_CH_LAYOUT_5POINT1;
            break;

        case HB_AMIXDOWN_6POINT1:
            ff_layout = AV_CH_LAYOUT_6POINT1;
            break;

        case HB_AMIXDOWN_7POINT1:
            ff_layout = AV_CH_LAYOUT_7POINT1;
            break;

        case HB_AMIXDOWN_5_2_LFE:
            ff_layout = (AV_CH_LAYOUT_5POINT1_BACK|
                         AV_CH_FRONT_LEFT_OF_CENTER|
                         AV_CH_FRONT_RIGHT_OF_CENTER);
            break;

        default:
            ff_layout = AV_CH_LAYOUT_STEREO;
            hb_log("hb_ff_mixdown_xlat: unsupported mixdown %d", hb_mixdown);
            break;
    }
    if (downmix_mode != NULL)
        *downmix_mode = mode;
    return ff_layout;
}

/*
 * Set sample format to the request format if supported by the codec.
 * The planar/packed variant of the requested format is the next best thing.
 */
void hb_ff_set_sample_fmt(AVCodecContext *context, AVCodec *codec,
                          enum AVSampleFormat request_sample_fmt)
{
    if (context != NULL && codec != NULL &&
        codec->type == AVMEDIA_TYPE_AUDIO && codec->sample_fmts != NULL)
    {
        const enum AVSampleFormat *fmt;
        enum AVSampleFormat next_best_fmt;

        next_best_fmt = (av_sample_fmt_is_planar(request_sample_fmt)  ?
                         av_get_packed_sample_fmt(request_sample_fmt) :
                         av_get_planar_sample_fmt(request_sample_fmt));

        context->request_sample_fmt = AV_SAMPLE_FMT_NONE;

        for (fmt = codec->sample_fmts; *fmt != AV_SAMPLE_FMT_NONE; fmt++)
        {
            if (*fmt == request_sample_fmt)
            {
                context->request_sample_fmt = request_sample_fmt;
                break;
            }
            else if (*fmt == next_best_fmt)
            {
                context->request_sample_fmt = next_best_fmt;
            }
        }

        /*
         * When encoding and AVCodec.sample_fmts exists, avcodec_open2()
         * will error out if AVCodecContext.sample_fmt isn't set.
         */
        if (context->request_sample_fmt == AV_SAMPLE_FMT_NONE)
        {
            context->request_sample_fmt = codec->sample_fmts[0];
        }
        context->sample_fmt = context->request_sample_fmt;
    }
}

