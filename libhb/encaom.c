/* encaom.c

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hb_dict.h"
#include "handbrake/av1_common.h"
#include "handbrake/hdr10plus.h"
#include "handbrake/bitstream.h"
#include "libavutil/avutil.h"
#include "aom/aom_encoder.h"
#include "aom/aomcx.h"

int  encaomInit(hb_work_object_t *, hb_job_t *);
int  encaomWork(hb_work_object_t *, hb_buffer_t **, hb_buffer_t **);
void encaomClose(hb_work_object_t *);

hb_work_object_t hb_encaom =
{
    WORK_ENCAOM,
    "AV1 encoder (AOM)",
    encaomInit,
    encaomWork,
    encaomClose
};

struct hb_work_private_s
{
    hb_job_t           *job;
    hb_chapter_queue_t *chapter_queue;

    struct aom_codec_ctx encoder;
    struct aom_image     img;

    struct aom_fixed_buf stats;
    unsigned             stats_size;

    const uint8_t *mdcv;
    size_t         mdcv_size;
    const uint8_t *cll;
    size_t         cll_size;
};

#define PROFILE_AV1_MAIN         0
#define PROFILE_AV1_HIGH         1
#define PROFILE_AV1_PROFESSIONAL 2

static inline int64_t rescale(hb_rational_t q, int b)
{
    return av_rescale(q.num, b, q.den);
}

static int init_mastering_display(hb_work_private_t *pv, hb_mastering_display_metadata_t mastering)
{
    const size_t size = 24;
    uint8_t *buf = av_mallocz(size);
    hb_bitstream_t bs;
    hb_bitstream_init(&bs, buf, size, 0);

    uint16_t display_primaries_rx = rescale(mastering.display_primaries[0][0], 1 << 16);
    uint16_t display_primaries_ry = rescale(mastering.display_primaries[0][1], 1 << 16);
    uint16_t display_primaries_gx = rescale(mastering.display_primaries[1][0], 1 << 16);
    uint16_t display_primaries_gy = rescale(mastering.display_primaries[1][1], 1 << 16);
    uint16_t display_primaries_bx = rescale(mastering.display_primaries[2][0], 1 << 16);
    uint16_t display_primaries_by = rescale(mastering.display_primaries[2][1], 1 << 16);

    uint16_t white_point_x = rescale(mastering.white_point[0], 1 << 16);
    uint16_t white_point_y = rescale(mastering.white_point[1], 1 << 16);

    uint32_t max_display_mastering_luminance = rescale(mastering.max_luminance, 1 << 8);
    uint32_t min_display_mastering_luminance = rescale(mastering.min_luminance, 1 << 14);

    hb_bitstream_put_bits(&bs, display_primaries_rx, 16);
    hb_bitstream_put_bits(&bs, display_primaries_ry, 16);
    hb_bitstream_put_bits(&bs, display_primaries_gx, 16);
    hb_bitstream_put_bits(&bs, display_primaries_gy, 16);
    hb_bitstream_put_bits(&bs, display_primaries_bx, 16);
    hb_bitstream_put_bits(&bs, display_primaries_by, 16);

    hb_bitstream_put_bits(&bs, white_point_x, 16);
    hb_bitstream_put_bits(&bs, white_point_y, 16);

    hb_bitstream_put_bits(&bs, max_display_mastering_luminance, 32);
    hb_bitstream_put_bits(&bs, min_display_mastering_luminance, 32);

    pv->mdcv = buf;
    pv->mdcv_size = size;

    return 0;
}

static int init_content_light_level(hb_work_private_t *pv, hb_content_light_metadata_t coll)
{
    const size_t size = 4;
    uint8_t *buf = av_mallocz(size);
    hb_bitstream_t bs;

    hb_bitstream_init(&bs, buf, size, 0);

    uint16_t max_cll = coll.max_cll;
    uint16_t max_fall =  coll.max_fall;

    hb_bitstream_put_bits(&bs, max_cll, 16);
    hb_bitstream_put_bits(&bs, max_fall, 16);

    pv->cll = buf;
    pv->cll_size = size;

    return 0;
}

static int set_control_int(struct aom_codec_ctx *encoder, enum aome_enc_control_id id, int value)
{
    int ret = aom_codec_control(encoder, id, value);
    if (ret != AOM_CODEC_OK)
    {
        hb_log("encaom: failed to set %d codec control", id);
        return 1;
    }
    return 0;
}

int encaomInit(hb_work_object_t *w, hb_job_t *job)
{
    hb_work_private_t *pv = calloc(1, sizeof(hb_work_private_t));
    w->private_data = pv;
    pv->job = job;
    pv->chapter_queue = hb_chapter_queue_init();

    const struct aom_codec_iface *iface = aom_codec_av1_cx();
    struct aom_codec_enc_cfg enccfg = {0};

    int ret;

    if ((ret = aom_codec_enc_config_default(iface, &enccfg, AOM_USAGE_GOOD_QUALITY)) != AOM_CODEC_OK)
    {
        hb_error("encaom: error initializing encoder handle");
        return 1;
    }

    if (job->encoder_profile != NULL && *job->encoder_profile)
    {
        if (!strcasecmp(job->encoder_profile, "main"))
        {
            enccfg.g_profile = PROFILE_AV1_MAIN;
        }
        else if (!strcasecmp(job->encoder_profile, "high"))
        {
            enccfg.g_profile = PROFILE_AV1_HIGH;
        }
        else if (!strcasecmp(job->encoder_profile, "professional"))
        {
            enccfg.g_profile = PROFILE_AV1_PROFESSIONAL;
        }
    }

    aom_codec_flags_t flags = 0;
    aom_img_fmt_t img_fmt = AOM_IMG_FMT_I420;

    switch (hb_video_encoder_get_depth(job->vcodec))
    {
        case 12:
            flags |= AOM_CODEC_USE_HIGHBITDEPTH;
            enccfg.g_bit_depth = AOM_BITS_12;
            break;
        case 10:
            flags |= AOM_CODEC_USE_HIGHBITDEPTH;
            enccfg.g_bit_depth = AOM_BITS_10;
            break;
        default:
            enccfg.g_bit_depth = AOM_BITS_8;
            break;
    }

    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(job->output_pix_fmt);
    enccfg.g_input_bit_depth = desc->comp[0].depth;
    switch (job->output_pix_fmt)
    {
        case AV_PIX_FMT_YUV420P:
            img_fmt = AOM_IMG_FMT_I420;
            break;
        case AV_PIX_FMT_YUV422P:
            img_fmt = AOM_IMG_FMT_I422;
            break;
        case AV_PIX_FMT_YUV444P:
            img_fmt = AOM_IMG_FMT_I444;
            break;
        case AV_PIX_FMT_YUV420P10:
        case AV_PIX_FMT_YUV420P12:
            img_fmt = AOM_IMG_FMT_I42016;
            break;
        case AV_PIX_FMT_YUV422P10:
        case AV_PIX_FMT_YUV422P12:
            img_fmt = AOM_IMG_FMT_I42216;
            break;
        case AV_PIX_FMT_YUV444P10:
        case AV_PIX_FMT_YUV444P12:
            img_fmt = AOM_IMG_FMT_I44416;
            break;
        default:
            break;
    }

    enccfg.g_w            = job->width;
    enccfg.g_h            = job->height;
    enccfg.g_timebase.num = job->orig_vrate.den;
    enccfg.g_timebase.den = job->orig_vrate.num;
    enccfg.g_threads      = hb_get_cpu_count();

    if (job->vquality <= HB_INVALID_VIDEO_QUALITY)
    {
        enccfg.rc_target_bitrate = job->vbitrate;
        enccfg.rc_end_usage = AOM_VBR;
    }
    else
    {
        enccfg.rc_end_usage = AOM_Q;
    }

    enccfg.kf_max_dist = ((double)job->orig_vrate.num / job->orig_vrate.den + 0.5) * 10;;

    if (job->pass_id == HB_PASS_ENCODE_ANALYSIS)
    {
        enccfg.g_pass = AOM_RC_FIRST_PASS;
    }
    else if (job->pass_id == HB_PASS_ENCODE_FINAL)
    {
        hb_interjob_t *interjob = hb_interjob_get(job->h);
        pv->stats.buf = interjob->context;
        pv->stats.sz  = interjob->context_size;

        enccfg.g_pass = AOM_RC_SECOND_PASS;
        enccfg.rc_twopass_stats_in = pv->stats;
    }

    ret = aom_codec_enc_init(&pv->encoder, iface, &enccfg, flags);
    if (ret != AOM_CODEC_OK)
    {
        hb_error("encaom: error initializing encoder handle");
        return 1;
    }

    if (job->vquality > HB_INVALID_VIDEO_QUALITY)
    {
        set_control_int(&pv->encoder, AOME_SET_CQ_LEVEL, job->vquality);
    }

    if (job->encoder_tune != NULL && !strcmp("screen", job->encoder_tune))
    {
        set_control_int(&pv->encoder, AOME_SET_TUNING, AOM_CONTENT_SCREEN);
    }
    else if (job->encoder_tune != NULL && !strcmp("film", job->encoder_tune))
    {
        set_control_int(&pv->encoder, AOME_SET_TUNING, AOM_CONTENT_FILM);
    }
    else
    {
        set_control_int(&pv->encoder, AOME_SET_TUNING, AOM_CONTENT_DEFAULT);
    }

    if (job->encoder_preset != NULL)
    {
        set_control_int(&pv->encoder, AOME_SET_CPUUSED, atoi(job->encoder_preset));
    }
    else
    {
        set_control_int(&pv->encoder, AOME_SET_CPUUSED, 6);
    }

    set_control_int(&pv->encoder, AV1E_SET_FP_MT, 1);

    set_control_int(&pv->encoder, AV1E_SET_COLOR_PRIMARIES, job->color_prim);
    set_control_int(&pv->encoder, AV1E_SET_TRANSFER_CHARACTERISTICS, job->color_transfer);
    set_control_int(&pv->encoder, AV1E_SET_MATRIX_COEFFICIENTS, job->color_matrix);

    aom_color_range_t aom_cr;
    switch (job->color_range)
    {
        case AVCOL_RANGE_UNSPECIFIED:
        case AVCOL_RANGE_MPEG:
            aom_cr = AOM_CR_STUDIO_RANGE;
            break;
        case AVCOL_RANGE_JPEG:
            aom_cr = AOM_CR_FULL_RANGE;
            break;
    }
    set_control_int(&pv->encoder, AV1E_SET_COLOR_RANGE, aom_cr);

    aom_chroma_sample_position_t aom_csp;
    switch (job->chroma_location)
    {
        case AVCHROMA_LOC_LEFT:
            aom_csp = AOM_CSP_VERTICAL;
            break;
        case AVCHROMA_LOC_TOPLEFT:
            aom_csp = AOM_CSP_COLOCATED;
            break;
        default:
            aom_csp = AOM_CSP_UNKNOWN;
            break;
    }
    set_control_int(&pv->encoder, AV1E_SET_CHROMA_SAMPLE_POSITION, aom_csp);

    if (job->color_transfer == HB_COLR_TRA_SMPTEST2084)
    {
        if (job->mastering.has_primaries && job->mastering.has_luminance)
        {
            init_mastering_display(pv, job->mastering);
        }

        if (job->coll.max_cll && job->coll.max_fall)
        {
            init_content_light_level(pv, job->coll);
        }
    }

    hb_dict_t *encoder_options = NULL;
    if (job->encoder_options != NULL && *job->encoder_options)
    {
        encoder_options = hb_encopts_to_dict(job->encoder_options, job->vcodec);
    }
    // Iterate through encoder_options and parse the options
    hb_dict_iter_t iter;
    for (iter  = hb_dict_iter_init(encoder_options);
         iter != HB_DICT_ITER_DONE;
         iter  = hb_dict_iter_next(encoder_options, iter))
    {
        const char *key = hb_dict_iter_key(iter);
        hb_value_t *value = hb_dict_iter_value(iter);
        char *str = hb_value_get_string_xform(value);

        // Here's where the strings are passed to aom for parsing
        int ret = aom_codec_set_option(&pv->encoder, key, str);
        if (ret != AOM_CODEC_OK)
        {
            hb_log("encaom: error parsing option %s: %s", key, str);
        }
        free(str);
    }
    hb_dict_free(&encoder_options);

    aom_img_wrap(&pv->img, img_fmt, job->width, job->height, 1, (unsigned char*)1);
    pv->img.bit_depth = enccfg.g_input_bit_depth;

    aom_fixed_buf_t *extradata = aom_codec_get_global_headers(&pv->encoder);

    if (extradata == NULL)
    {
        hb_error("encaom: error initializing extradata");
        return 1;
    }

    w->config->extradata.length = extradata->sz;
    memcpy(w->config->extradata.bytes, extradata->buf, extradata->sz);

    free(extradata->buf);
    free(extradata);

    return 0;
}

void encaomClose(hb_work_object_t *w)
{
    hb_work_private_t *pv = w->private_data;

    if (pv == NULL)
    {
        return;
    }

    hb_chapter_queue_close(&pv->chapter_queue);
    aom_codec_destroy(&pv->encoder);

    if (pv->job->pass_id == HB_PASS_ENCODE_FINAL || *pv->job->die)
    {
        hb_interjob_t *interjob = hb_interjob_get(pv->job->h);
        av_freep(&interjob->context);
    }

    av_freep(&pv->mdcv);
    av_freep(&pv->cll);
    free(pv);
    w->private_data = NULL;
}

static int send(hb_work_private_t *pv, hb_buffer_t *in)
{
    hb_job_t *job         = pv->job;

    struct aom_image *img = NULL;
    int64_t timestamp = 0;
    unsigned long duration = 0;
    int res;
    aom_enc_frame_flags_t flags = 0;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        aom_codec_encode(&pv->encoder, img, timestamp, duration, flags);
        return 0;
    }

    img                      = &pv->img;
    img->planes[AOM_PLANE_Y] = in->plane[0].data;
    img->planes[AOM_PLANE_U] = in->plane[1].data;
    img->planes[AOM_PLANE_V] = in->plane[2].data;
    img->stride[AOM_PLANE_Y] = in->plane[0].stride;
    img->stride[AOM_PLANE_U] = in->plane[1].stride;
    img->stride[AOM_PLANE_V] = in->plane[2].stride;
    timestamp                = in->s.start;
    duration                 = in->s.duration;

    switch (in->f.color_range)
    {
        case AVCOL_RANGE_MPEG:
            img->range = AOM_CR_STUDIO_RANGE;
            break;
        case AVCOL_RANGE_JPEG:
            img->range = AOM_CR_FULL_RANGE;
            break;
    }

    if (aom_img_num_metadata(img))
    {
        aom_img_remove_metadata(img);
    }

    if (pv->mdcv)
    {
        aom_img_add_metadata(img, OBU_METADATA_TYPE_HDR_MDCV, pv->mdcv, pv->mdcv_size, AOM_MIF_KEY_FRAME);
    }

    if (pv->cll)
    {
        aom_img_add_metadata(img, OBU_METADATA_TYPE_HDR_CLL, pv->cll, pv->cll_size, AOM_MIF_KEY_FRAME);
    }

    if (job->passthru_dynamic_hdr_metadata)
    {
        for (int i = 0; i < in->nb_side_data; i++)
        {
            const AVFrameSideData *side_data = in->side_data[i];
            if (job->passthru_dynamic_hdr_metadata & HDR_10_PLUS &&
                side_data->type == AV_FRAME_DATA_DYNAMIC_HDR_PLUS)
            {
                uint8_t *payload = NULL;
                uint32_t playload_size = 0;

                hb_dynamic_hdr10_plus_to_itu_t_t35((AVDynamicHDRPlus *)side_data->data, &payload, &playload_size);
                if (!playload_size)
                {
                    continue;
                }

                aom_img_add_metadata(img, OBU_METADATA_TYPE_ITUT_T35, payload, playload_size, AOM_MIF_ANY_FRAME);
                av_freep(&payload);
            }
        }
    }

    if (in->s.new_chap > 0 && pv->job->chapter_markers)
    {
        flags |= AOM_EFLAG_FORCE_KF;
        hb_chapter_enqueue(pv->chapter_queue, in);
    }

    res = aom_codec_encode(&pv->encoder, img, timestamp, duration, flags);
    if (res != AOM_CODEC_OK)
    {
        hb_log("encaom: error encoding frame");
        return 1;
    }

    return 0;
}

static int receive(hb_work_private_t *pv, hb_buffer_list_t *list)
{
    const struct aom_codec_cx_pkt *pkt;
    const void *iter = NULL;
    int count = 0;

    while ((pkt = aom_codec_get_cx_data(&pv->encoder, &iter)))
    {
        switch (pkt->kind)
        {
            case AOM_CODEC_CX_FRAME_PKT:
            {
                hb_buffer_t *out = hb_buffer_init(pkt->data.frame.sz);

                out->size            = pkt->data.frame.sz;
                out->s.start         = pkt->data.frame.pts;
                out->s.duration      = pkt->data.frame.duration;
                out->s.stop          = out->s.start + out->s.duration;
                out->s.renderOffset  = out->s.start;

                memcpy(out->data, pkt->data.frame.buf, pkt->data.frame.sz);

                hb_chapter_dequeue(pv->chapter_queue, out);

                if (pkt->data.frame.flags & AOM_FRAME_IS_KEY)
                {
                    out->s.flags |= HB_FLAG_FRAMETYPE_KEY;
                    out->s.frametype = HB_FRAME_IDR;
                }
                else if (pkt->data.frame.flags & AOM_FRAME_IS_INTRAONLY)
                {
                    out->s.frametype = HB_FRAME_IDR;
                }
                else
                {
                    out->s.frametype = HB_FRAME_P;
                }
                // TODO: AOM_FRAME_IS_DROPPABLE

                hb_buffer_list_append(list, out);
                count += 1;
                break;
            }
            case AOM_CODEC_STATS_PKT:
            {
                struct aom_fixed_buf *stats = &pv->stats;
                uint8_t *tmp = av_fast_realloc(stats->buf, &pv->stats_size,
                                               stats->sz + pkt->data.twopass_stats.sz);
                if (!tmp)
                {
                    av_freep(&stats->buf);
                    stats->sz = 0;
                    hb_log("encaom: stat buffer realloc failed");
                }
                stats->buf = tmp;
                memcpy((uint8_t *)stats->buf + stats->sz, pkt->data.twopass_stats.buf, pkt->data.twopass_stats.sz);
                stats->sz += pkt->data.twopass_stats.sz;
                break;
            }
            default:
                // Unsupported
                break;
        }
    }

    return count;
}

static void encode(hb_work_private_t *pv, hb_buffer_t *in, hb_buffer_list_t *list)
{
    send(pv, in);
    receive(pv, list);
}

static void flush(hb_work_private_t *pv, hb_buffer_t *in, hb_buffer_list_t *list)
{
    do {
        send(pv, in);
    } while (receive(pv, list));

    if (pv->job->pass_id == HB_PASS_ENCODE_ANALYSIS)
    {
        hb_interjob_t *interjob = hb_interjob_get(pv->job->h);
        interjob->context       = pv->stats.buf;
        interjob->context_size  = pv->stats.sz;
    }
}

int encaomWork(hb_work_object_t *w, hb_buffer_t **buf_in,
               hb_buffer_t **buf_out)
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t       *in = *buf_in;
    hb_buffer_list_t   list;

    *buf_out = NULL;
    hb_buffer_list_clear(&list);

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        // EOF on input. Flush any frames still in the decoder then
        // send the eof downstream to tell the muxer we're done.
        flush(pv, in, &list);
        hb_buffer_list_append(&list, hb_buffer_eof_init());

        *buf_out = hb_buffer_list_clear(&list);
        return HB_WORK_DONE;
    }

    // Not EOF - encode the packet
    encode(pv, in, &list);
    *buf_out = hb_buffer_list_clear(&list);

    return HB_WORK_OK;
}
