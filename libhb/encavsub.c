/* encavsub.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/encavsub.h"
#include "handbrake/extradata.h"

#define ENC_BUF_DEFAULT_SZ (4 * 1024)
#define ENC_BUF_MAX_SZ     (1024 * 1024)

struct hb_encavsub_context_s
{
    AVCodecContext *context;
    hb_job_t       *job;
    hb_subtitle_t  *subtitle;

    // List of subtitle packets to be output by this encoder.
    hb_buffer_list_t list;

    // avcodec_encode_subtitle encodes to a raw buffer
    // that we must provide.  *Most* libav subtitle encoders
    // will return an error (of various sorts: -1, EINVAL,
    // AVERROR_BUFFER_TOO_SMALL) when the buffer is too small.
    // Some just overrun the buffer :( DVB )
    //
    // TODO: sanitize libav return codes and fix DVB
    uint8_t        *buffer;
    int             buffer_size;
};

struct hb_work_private_s
{
    hb_encavsub_context_t *ctx;
};

static int realloc_enc_buffer(hb_encavsub_context_t * ctx)
{
    if (ctx->buffer_size == 0)
    {
        ctx->buffer_size = ENC_BUF_DEFAULT_SZ;
    }
    else if (ctx->buffer_size * 2 < ENC_BUF_MAX_SZ)
    {
        ctx->buffer_size *= 2;
    }
    else
    {
        return 0;
    }
    ctx->buffer = av_realloc(ctx->buffer, ctx->buffer_size);
    if (ctx->buffer == NULL)
    {
        return 0;
    }
    return 1;
}

/***********************************************************************
 * encavsubInit
 ***********************************************************************
 * Init function for libav subtitle encoding that may be wrapped
 * by HB subtitle encoder
 **********************************************************************/
hb_encavsub_context_t * encavsubInit(hb_work_object_t *w, hb_job_t *job)
{
    const AVCodec         *codec;
    AVCodecContext        *context;
    hb_encavsub_context_t *ctx = calloc(1, sizeof(hb_encavsub_context_t));

    if (ctx == NULL)
    {
        hb_error("encavsubInit: calloc ctx failed");
        return NULL;
    }
    ctx->job      = job;
    ctx->subtitle = w->subtitle;

    codec   = avcodec_find_encoder(w->codec_param);
    if (codec == NULL)
    {
        hb_error("encavsubInit: avcodec_find_encoder failed");
        goto fail;
    }
    context = avcodec_alloc_context3(codec);
    if (context == NULL)
    {
        hb_error("encavsubInit: avcodec_alloc_context3 failed");
        goto fail;
    }
    ctx->context   = context;
    context->codec = codec;

    // TEXT subtitle encoders require SSA header
    context->subtitle_header = av_malloc(ctx->subtitle->extradata->size + 1);
    if (context->subtitle_header == NULL)
    {
        hb_error("encavsubInit: av_malloc subtitle_header failed");
        goto fail;
    }
    memcpy(context->subtitle_header, ctx->subtitle->extradata->bytes,
                                     ctx->subtitle->extradata->size);
    context->subtitle_header[ctx->subtitle->extradata->size] = '\0';
    context->subtitle_header_size  = ctx->subtitle->extradata->size + 1;
    context->time_base             = AV_TIME_BASE_Q;

    // Set encoder opts...
    AVDictionary *av_opts = NULL;
    if (w->codec_param == AV_CODEC_ID_MOV_TEXT)
    {
        char *height = hb_strdup_printf("%d", job->height);
        av_dict_set( &av_opts, "height", height, 0 );
        free(height);
    }
    if (hb_avcodec_open(ctx->context, codec, &av_opts, 0))
    {
        av_dict_free( &av_opts );
        hb_error("encavsubInit: avcodec_open failed");
        goto fail;
    }
    av_dict_free( &av_opts );

    // avcodec encoder may create subtitle extradata
    hb_data_close(&ctx->subtitle->extradata);

    if (context->extradata != NULL && context->extradata_size > 0)
    {
        int ret = hb_set_extradata(&ctx->subtitle->extradata,
                                   context->extradata,
                                   context->extradata_size);
        if (ret != 0)
        {
            hb_error("encavsubInit: malloc subtitle extradata failed");
            goto fail;
        }
    }

    if (!realloc_enc_buffer(ctx))
    {
        hb_error("encavsubInit: realloc buffer failed");
        goto fail;
    }
    hb_buffer_list_clear(&ctx->list);

    return ctx;


fail:
    if (ctx != NULL)
    {
        if (ctx->context != NULL)
        {
            avcodec_free_context(&ctx->context);
        }
    }
    free(ctx);

    return NULL;
}

/***********************************************************************
 * encavsubClose
 ***********************************************************************
 * Close function for libav subtitle encoding that may be wrapped
 * by HB subtitle encoder
 **********************************************************************/
void encavsubClose(hb_encavsub_context_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }
    avcodec_free_context(&ctx->context);
    free(ctx->buffer);
    free(ctx);
}

/***********************************************************************
 * encavsubWork
 ***********************************************************************
 * Work function for libav subtitle encoding that may be wrapped
 * by HB subtitle encoder
 **********************************************************************/
int encavsubWork(hb_encavsub_context_t *ctx,
                 hb_buffer_t **buf_in,
                 hb_buffer_t **buf_out)
{
    hb_buffer_t *in = *buf_in;
    AVSubtitle   subtitle;
    int          num_rects = 1;
    int64_t      duration;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        /* EOF on input stream - send it downstream & say that we're done */
        *buf_in = NULL;

        *buf_out = in;
        return HB_WORK_DONE;
    }

    // Create an AVSubtitle from the input buffer
    memset(&subtitle, 0, sizeof(subtitle));
    subtitle.rects = av_mallocz(num_rects * sizeof(subtitle.rects));
    if (subtitle.rects == NULL)
    {
        *buf_out = hb_buffer_eof_init();
        hb_error("encavsubInit: av_mallocz_array failed");
        return HB_WORK_DONE;
    }

    for (int ii = 0; ii < num_rects; ii++)
    {
        subtitle.rects[ii]  = av_mallocz(sizeof(*subtitle.rects[0]));
        if (!subtitle.rects[ii]) {
            avsubtitle_free(&subtitle);
            *buf_out = hb_buffer_eof_init();
            hb_error("encavsubInit: av_mallocz failed");
            return HB_WORK_DONE;
        }
        subtitle.num_rects++;
    }

    if (ctx->subtitle->format == TEXTSUB)
    {
        subtitle.rects[0]->type = SUBTITLE_ASS;
        subtitle.rects[0]->ass  = av_strdup((char *)in->data);
        if (subtitle.rects[0]->ass == NULL)
        {
            avsubtitle_free(&subtitle);
            *buf_out = hb_buffer_eof_init();
            hb_error("encavsubInit: av_strdup failed");
            return HB_WORK_DONE;
        }
    }
    else
    {
        // TODO: bitmap subtitles
        // Must simplify format of hb_buffer_t bitmap representation
        for (int ii = 0; ii < num_rects; ii++)
        {
            subtitle.rects[0]->type = SUBTITLE_BITMAP;
        }
    }

    subtitle.pts = av_rescale(in->s.start, AV_TIME_BASE, 90000);
    if (in->s.stop != AV_NOPTS_VALUE)
    {
        duration = in->s.stop - in->s.start;
        subtitle.end_display_time = av_rescale(duration, 1000, 90000);
    }

    int size;
    do
    {
        size = avcodec_encode_subtitle(ctx->context,
                                       ctx->buffer, ctx->buffer_size, &subtitle);
        if (size < 0)
        {
            if (!realloc_enc_buffer(ctx))
            {
                avsubtitle_free(&subtitle);
                *buf_out = hb_buffer_eof_init();
                hb_error("encavsubInit: realloc failed");
                return HB_WORK_DONE;
            }
        }
        else if (size > 0)
        {
            // Next iteration, flush additional subtitle packets that
            // the encoder might generate
            subtitle.num_rects = 0;

            hb_buffer_t *out = hb_buffer_init(size);
            memcpy(out->data, ctx->buffer, size);
            out->s.start = av_rescale(subtitle.pts, 90000, AV_TIME_BASE);
            out->s.duration = (int64_t)AV_NOPTS_VALUE;
            if (subtitle.end_display_time > 0)
            {
                duration = av_rescale(subtitle.end_display_time, 90000, 1000);
                out->s.stop = out->s.start + duration;
                out->s.duration = duration;
            }
            hb_buffer_list_append(&ctx->list, out);
        }
    } while (size < 0);

    subtitle.num_rects = num_rects;
    avsubtitle_free(&subtitle);

    *buf_out = hb_buffer_list_clear(&ctx->list);
    return HB_WORK_OK;
}

/***********************************************************************
 * Init
 ***********************************************************************
 * Initialize hb_work_private_t data
 **********************************************************************/
static int Init(hb_work_object_t *w, hb_job_t *job)
{
    hb_work_private_t *pv;

    pv = calloc(1, sizeof(hb_work_private_t));
    if (pv == NULL)
    {
        hb_error("encsubInit: calloc private data failed");
        return 1;
    }

    pv->ctx = encavsubInit(w, job);
    if (pv->ctx == NULL)
    {
        free(pv);
        return 1;
    }
    w->private_data = pv;

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 * Free any allocation in hb_work_private_t
 **********************************************************************/
static void Close(hb_work_object_t *w)
{
    hb_work_private_t * pv = w->private_data;
    if (pv == NULL)
    {
        return;
    }
    encavsubClose(pv->ctx);
    free(pv);
    w->private_data = NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 * Take an input buffer, send an output buffer
 **********************************************************************/
static int Work(hb_work_object_t *w, hb_buffer_t **buf_in,
                hb_buffer_t **buf_out)
{
    hb_work_private_t *pv = w->private_data;

    return encavsubWork(pv->ctx, buf_in, buf_out);
}

/***********************************************************************
 * Info
 ***********************************************************************
 * Retrieve current info about context initialized during Init
 **********************************************************************/
static int Info(hb_work_object_t *w, hb_work_info_t *info)
{
    memset(info, 0, sizeof(*info));

    // Indicate no info is returned
    return 0;
}

/***********************************************************************
 * BSInfo
 ***********************************************************************
 * Retrieve info, does not require Init(), but uses current context
 * if Init has already been called.
 * buf contains stream data to extract info from.
 **********************************************************************/
static int BSInfo(hb_work_object_t *w, const hb_buffer_t *buf,
                  hb_work_info_t *info)
{
    memset(info, 0, sizeof(*info));

    // Indicate no info is returned
    return 0;
}

/***********************************************************************
 * Flush
 ***********************************************************************
 * Reset context without closing, kind of poorly named :(
 **********************************************************************/
static void Flush(hb_work_object_t *w)
{
}

hb_work_object_t hb_encavsub =
{
    .id     = WORK_ENCAVSUB,
    .name   = "Subtitle encoder (libavcodec)",
    .init   = Init,
    .work   = Work,
    .close  = Close,
    .info   = Info,
    .bsinfo = BSInfo,
    .flush  = Flush,
};
