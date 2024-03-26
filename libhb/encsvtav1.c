/* encsvtav1.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   partially based on FFmpeg libsvtav1.c
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hb_dict.h"
#include "handbrake/av1_common.h"
#include "handbrake/hdr10plus.h"
#include "handbrake/dovi_common.h"
#include "handbrake/extradata.h"

#include "libavutil/avutil.h"
#include "svt-av1/EbSvtAv1ErrorCodes.h"
#include "svt-av1/EbSvtAv1Enc.h"
#include "svt-av1/EbSvtAv1Metadata.h"

int  encsvtInit(hb_work_object_t *, hb_job_t *);
int  encsvtWork(hb_work_object_t *, hb_buffer_t **, hb_buffer_t **);
void encsvtClose(hb_work_object_t *);

#define FRAME_INFO_SIZE 2048
#define FRAME_INFO_MASK (FRAME_INFO_SIZE - 1)

hb_work_object_t hb_encsvtav1 =
{
    WORK_ENCSVTAV1,
    "AV1 encoder (SVT)",
    encsvtInit,
    encsvtWork,
    encsvtClose
};

struct hb_work_private_s
{
    hb_job_t           *job;
    hb_chapter_queue_t *chapter_queue;

    EbSvtAv1EncConfiguration    enc_params;
    EbComponentType            *svt_handle;
    EbBufferHeaderType         *in_buf;

    struct {
        int64_t duration;
    } frame_info[FRAME_INFO_SIZE];

    int frameno_in;
    int frameno_out;
};

static void save_frame_duration(hb_work_private_t *pv, hb_buffer_t *in)
{
    int i = pv->frameno_in & FRAME_INFO_MASK;
    pv->frameno_in++;
    pv->frame_info[i].duration = in->s.stop - in->s.start;
}

static int64_t get_frame_duration(hb_work_private_t *pv)
{
    int i = pv->frameno_out & FRAME_INFO_MASK;
    pv->frameno_out++;
    return pv->frame_info[i].duration;
}

static int alloc_buffer(EbSvtAv1EncConfiguration *config, hb_work_private_t *pv)
{
    EbSvtIOFormat *in_data;

    // allocate buffer for in and out
    pv->in_buf = av_mallocz(sizeof(*pv->in_buf));
    if (pv->in_buf == NULL)
    {
        return 1;
    }

    pv->in_buf->p_buffer = av_mallocz(sizeof(*in_data));
    if (pv->in_buf->p_buffer == NULL)
    {
        return 1;
    }

    pv->in_buf->size = sizeof(*pv->in_buf);

    return 0;
}

int encsvtInit(hb_work_object_t *w, hb_job_t *job)
{
    hb_work_private_t *pv = calloc(1, sizeof(hb_work_private_t));
    w->private_data = pv;
    pv->job = job;
    pv->chapter_queue = hb_chapter_queue_init();

    EbErrorType svt_ret;
    int ret;

    svt_ret = svt_av1_enc_init_handle(&pv->svt_handle, pv, &pv->enc_params);
    if (svt_ret != EB_ErrorNone)
    {
        hb_error("encsvtav1: error initializing encoder handle");
        return 1;
    }

    EbSvtAv1EncConfiguration *param = &pv->enc_params;

    if (job->encoder_preset == NULL)
    {
        param->enc_mode = 5;
    }
    else
    {
        param->enc_mode = atoi(job->encoder_preset);
    }

    if (job->vquality <= HB_INVALID_VIDEO_QUALITY)
    {
        param->target_bit_rate   = 1000 * job->vbitrate;
        param->rate_control_mode = SVT_AV1_RC_MODE_VBR;
    }
    else
    {
        param->qp                = job->vquality;
        param->rate_control_mode = SVT_AV1_RC_MODE_CQP_OR_CRF;
        param->force_key_frames = 1;
    }

    param->color_primaries          = hb_output_color_prim(job);
    param->transfer_characteristics = hb_output_color_transfer(job);
    param->matrix_coefficients      = hb_output_color_matrix(job);

    switch (job->color_range)
    {
        case AVCOL_RANGE_MPEG:
            param->color_range = EB_CR_STUDIO_RANGE;
            break;
        case AVCOL_RANGE_JPEG:
            param->color_range = EB_CR_FULL_RANGE;
            break;
        default:
            break;
    }

    switch (job->chroma_location)
    {
        case AVCHROMA_LOC_LEFT:
            param->chroma_sample_position = EB_CSP_VERTICAL;
            break;
        case AVCHROMA_LOC_TOPLEFT:
            param->chroma_sample_position = EB_CSP_COLOCATED;
            break;
        default:
            break;
    }

    if (job->encoder_profile != NULL && *job->encoder_profile)
    {
        if (!strcasecmp(job->encoder_profile, "main"))
        {
            param->profile = MAIN_PROFILE;
        }
        else if (!strcasecmp(job->encoder_profile, "high"))
        {
            param->profile = HIGH_PROFILE;
        }
        else if (!strcasecmp(job->encoder_profile, "professional"))
        {
            param->profile = PROFESSIONAL_PROFILE;
        }
    }

    if (job->encoder_level != NULL && *job->encoder_level)
    {
        for (int i = 1; hb_av1_level_names[i]; i++)
        {
            if (!strcasecmp(job->encoder_level, hb_av1_level_names[i]))
            {
                param->level = hb_av1_level_values[i];
            }
        }
    }

    if (job->encoder_tune != NULL && strstr("ssim", job->encoder_tune) != NULL)
    {
        param->tune = 2;
    }
    else if (job->encoder_tune != NULL && strstr("psnr", job->encoder_tune) != NULL)
    {
        param->tune = 1;
    }
    else
    {
        param->tune = 0;
    }

    if (job->encoder_tune != NULL && strstr("fastdecode", job->encoder_tune) != NULL)
    {
        param->fast_decode = 1;
    }
    else
    {
        param->fast_decode = 0;
    }

    param->intra_period_length = ((double)job->orig_vrate.num / job->orig_vrate.den + 0.5) * 10;
    // VFR isn't supported, the rate control will ignore
    // the frames timestamps and use the values below
    param->frame_rate_numerator = job->orig_vrate.num;
    param->frame_rate_denominator = job->orig_vrate.den;

    // HDR10 Static metadata
    if (job->color_transfer == HB_COLR_TRA_SMPTEST2084)
    {
        // Mastering display metadata
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

            EbErrorType ret = svt_av1_enc_parse_parameter(param, "mastering-display", mastering_display_color_volume);
            if (ret != EB_ErrorNone)
            {
                hb_log("encsvtav1: error parsing option mastering-display: %s", mastering_display_color_volume);
            }
        }

        //  Content light level
        if (job->coll.max_cll && job->coll.max_fall)
        {
            char content_light_level[256];
            snprintf(content_light_level, sizeof(content_light_level),
                     "%u,%u", job->coll.max_cll, job->coll.max_fall);

            EbErrorType ret = svt_av1_enc_parse_parameter(param, "content-light", content_light_level);
            if (ret != EB_ErrorNone)
            {
                hb_log("encsvtav1: error parsing option content-light: %s", content_light_level);
            }
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

        // Here's where the strings are passed to svt-av1 for parsing
        EbErrorType ret = svt_av1_enc_parse_parameter(param, key, str);
        if (ret != EB_ErrorNone)
        {
            hb_log("encsvtav1: error parsing option %s: %s", key, str);
        }
        free(str);
    }
    hb_dict_free(&encoder_options);

    // Reload colorimetry settings in case custom values were set
    // in the encoder_options string
    job->color_prim_override     = param->color_primaries;
    job->color_transfer_override = param->transfer_characteristics;
    job->color_matrix_override   = param->matrix_coefficients;

    param->source_width  = job->width;
    param->source_height = job->height;
    param->encoder_bit_depth = hb_get_bit_depth(job->output_pix_fmt);
    param->encoder_color_format = EB_YUV420;

    if (job->pass_id == HB_PASS_ENCODE_ANALYSIS ||
        job->pass_id == HB_PASS_ENCODE_FINAL)
    {
        hb_interjob_t *interjob = hb_interjob_get(job->h);
        param->rc_stats_buffer.buf = interjob->context;
        param->rc_stats_buffer.sz  = interjob->context_size;
        param->pass = job->pass_id == HB_PASS_ENCODE_ANALYSIS ? 1 : 2;
    }

    svt_ret = svt_av1_enc_set_parameter(pv->svt_handle, &pv->enc_params);
    if (svt_ret != EB_ErrorNone)
    {
        hb_error("encsvtav1: error setting encoder parameters");
        return 1;
    }

    svt_ret = svt_av1_enc_init(pv->svt_handle);
    if (svt_ret != EB_ErrorNone)
    {
        hb_error("encsvtav1: error initializing encoder");
        return 1;
    }

    EbBufferHeaderType *headerPtr = NULL;

    svt_ret = svt_av1_enc_stream_header(pv->svt_handle, &headerPtr);
    if (svt_ret != EB_ErrorNone)
    {
        hb_error("encsvtav1: error building stream header");
        return 1;
    }

    if (hb_set_extradata(w->extradata, headerPtr->p_buffer, headerPtr->n_filled_len))
    {
        hb_error("encsvtav1: error setting extradata");
        return 1;
    }

    // Update and set Dolby Vision level
    if (job->passthru_dynamic_hdr_metadata & DOVI)
    {
        int level_idx, high_tier;
        hb_parse_av1_extradata(*w->extradata, &level_idx, &high_tier);

        int pps = (double)job->width * job->height * (job->vrate.num / job->vrate.den);
        int bitrate = job->vquality == HB_INVALID_VIDEO_QUALITY ? job->vbitrate : -1;

        // Dolby Vision requires VBV settings to enable HRD
        // but SVT-AV1 supports max-bit-rate only in CFR mode
        // so that the Dolby Vision level to something comparable
        // to the current AV1 level
        if (param->max_bit_rate == 0)
        {
            int max_rate = hb_dovi_max_rate(job->vcodec, job->width, pps, bitrate,
                                            level_idx, high_tier);
            param->max_bit_rate = max_rate * 1000;
        }

        job->dovi.dv_level = hb_dovi_level(job->width, pps, param->max_bit_rate / 1000, high_tier);
    }

    svt_ret = svt_av1_enc_stream_header_release(headerPtr);
    if (svt_ret != EB_ErrorNone)
    {
        hb_error("encsvtav1: error freeing stream header");
        return 1;
    }

    ret = alloc_buffer(&pv->enc_params, pv);
    if (ret)
    {
        hb_error("encsvtav1: couldn't allocate buffer");
        return 1;
    }

    return 0;
}

void encsvtClose(hb_work_object_t *w)
{
    hb_work_private_t *pv = w->private_data;

    if (pv == NULL)
    {
        return;
    }

    hb_chapter_queue_close(&pv->chapter_queue);

    if (pv->svt_handle)
    {
        svt_av1_enc_deinit(pv->svt_handle);
        svt_av1_enc_deinit_handle(pv->svt_handle);
    }
    if (pv->in_buf)
    {
        if (pv->in_buf->metadata)
        {
            svt_metadata_array_free(&pv->in_buf->metadata);
        }
        av_free(pv->in_buf->p_buffer);
        av_freep(&pv->in_buf);
    }
    if (pv->job->pass_id == HB_PASS_ENCODE_FINAL || *pv->job->die)
    {
        hb_interjob_t *interjob = hb_interjob_get(pv->job->h);
        av_freep(&interjob->context);
    }

    free(pv);
    w->private_data = NULL;
}

static int read_in_data(EbSvtAv1EncConfiguration *param, const hb_buffer_t *in, EbBufferHeaderType *header_ptr)
{
    EbSvtIOFormat *in_data = (EbSvtIOFormat *)header_ptr->p_buffer;
    ptrdiff_t linesizes[4];
    size_t sizes[4];
    int bytes_shift = param->encoder_bit_depth > 8 ? 1 : 0;
    int ret, frame_size;

    for (int i = 0; i < 4; i++)
    {
        linesizes[i] = in->plane[i].stride;
    }

    ret = av_image_fill_plane_sizes(sizes, in->f.fmt, in->f.height, linesizes);
    if (ret < 0)
    {
        return ret;
    }

    frame_size = 0;
    for (int i = 0; i < 4; i++)
    {
        if (sizes[i] > INT_MAX - frame_size)
        {
            return 1;
        }
        frame_size += sizes[i];
    }

    in_data->luma = in->plane[0].data;
    in_data->cb   = in->plane[1].data;
    in_data->cr   = in->plane[2].data;

    in_data->y_stride  = AV_CEIL_RSHIFT(in->plane[0].stride, bytes_shift);
    in_data->cb_stride = AV_CEIL_RSHIFT(in->plane[1].stride, bytes_shift);
    in_data->cr_stride = AV_CEIL_RSHIFT(in->plane[2].stride, bytes_shift);

    header_ptr->n_filled_len = frame_size;

    return 0;
}

static int send(hb_work_object_t *w, hb_buffer_t *in)
{
    hb_work_private_t *pv = w->private_data;
    hb_job_t *job         = pv->job;

    EbBufferHeaderType *headerPtr = pv->in_buf;
    int ret;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        EbBufferHeaderType headerPtrLast;

        memset(&headerPtrLast, 0, sizeof(headerPtrLast));
        headerPtrLast.pic_type = EB_AV1_INVALID_PICTURE;
        headerPtrLast.flags    = EB_BUFFERFLAG_EOS;

        svt_av1_enc_send_picture(pv->svt_handle, &headerPtrLast);
        return 0;
    }

    ret = read_in_data(&pv->enc_params, in, headerPtr);
    if (ret < 0)
    {
        return ret;
    }

    headerPtr->flags         = 0;
    headerPtr->p_app_private = NULL;
    headerPtr->pts           = in->s.start;
    save_frame_duration(pv, in);

    if (headerPtr->metadata)
    {
        svt_metadata_array_free(&headerPtr->metadata);
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

                svt_add_metadata(headerPtr, EB_AV1_METADATA_TYPE_ITUT_T35, payload, playload_size);
                av_freep(&payload);
            }
            else if (job->passthru_dynamic_hdr_metadata & DOVI &&
                     side_data->type == AV_FRAME_DATA_DOVI_RPU_BUFFER)
            {
                svt_add_metadata(headerPtr, EB_AV1_METADATA_TYPE_ITUT_T35, side_data->data, side_data->size);
            }

        }
    }

    if (in->s.new_chap > 0 && pv->job->chapter_markers)
    {
        headerPtr->pic_type = EB_AV1_KEY_PICTURE;
        hb_chapter_enqueue(pv->chapter_queue, in);
    }
    else
    {
        headerPtr->pic_type = EB_AV1_INVALID_PICTURE;
    }

    svt_av1_enc_send_picture(pv->svt_handle, headerPtr);

    return 0;
}

static int receive(hb_work_object_t *w, hb_buffer_t **out, int done)
{
    hb_work_private_t  *pv = w->private_data;
    EbBufferHeaderType *headerPtr;
    EbErrorType svt_ret;
    hb_buffer_t *buf;

    svt_ret = svt_av1_enc_get_packet(pv->svt_handle, &headerPtr, done);
    if (svt_ret == EB_NoErrorEmptyQueue)
    {
        *out = NULL;
        return 1;
    }

    if (headerPtr->flags & EB_BUFFERFLAG_EOS)
    {
        svt_av1_enc_release_out_buffer(&headerPtr);
        *out = NULL;
        return 2;
    }

    buf = hb_buffer_init(headerPtr->n_filled_len);
    if (buf == NULL)
    {
        hb_error("encsvtav1: failed to allocate output packet");
        svt_av1_enc_release_out_buffer(&headerPtr);
        *out = NULL;
        return 2;
    }

    memcpy(buf->data, headerPtr->p_buffer, headerPtr->n_filled_len);

    buf->size            = headerPtr->n_filled_len;
    buf->s.start         = headerPtr->pts;
    buf->s.duration      = get_frame_duration(pv);
    buf->s.stop          = buf->s.start + buf->s.duration;
    buf->s.renderOffset  = headerPtr->dts;

    // SVT-AV1 doesn't always respect forced keyframes,
    // so always check for chapters
    hb_chapter_dequeue(pv->chapter_queue, buf);

    switch (headerPtr->pic_type)
    {
        case EB_AV1_KEY_PICTURE:
            buf->s.flags |= HB_FLAG_FRAMETYPE_KEY;
            // fall-through
        case EB_AV1_INTRA_ONLY_PICTURE:
            buf->s.frametype = HB_FRAME_IDR;
            break;
        default:
            buf->s.frametype = HB_FRAME_P;
            break;
    }

    if (headerPtr->pic_type != EB_AV1_NON_REF_PICTURE)
    {
        buf->s.flags |= HB_FLAG_FRAMETYPE_REF;
    }

    svt_av1_enc_release_out_buffer(&headerPtr);

    *out = buf;
    return 0;
}

static void encode(hb_work_object_t *w, hb_buffer_t *in, hb_buffer_list_t *list)
{
    send(w, in);

    hb_buffer_t *out;
    while (receive(w, &out, 0) == 0)
    {
        hb_buffer_list_append(list, out);
    }
}

static void flush(hb_work_object_t *w, hb_buffer_t *in, hb_buffer_list_t *list)
{
    hb_work_private_t  *pv = w->private_data;
    hb_job_t *job = pv->job;
    hb_interjob_t *interjob = hb_interjob_get(job->h);

    send(w, in);

    hb_buffer_t *out = NULL;
    while (receive(w, &out, 1) == 0)
    {
        hb_buffer_list_append(list, out);
    }

    // Store the first pass stats for the next
    if (job->pass_id == HB_PASS_ENCODE_ANALYSIS && interjob->context == NULL)
    {
        SvtAv1FixedBuf first_pass_stat;
        EbErrorType ret = svt_av1_enc_get_stream_info(pv->svt_handle,
                                                      SVT_AV1_STREAM_INFO_FIRST_PASS_STATS_OUT,
                                                      &first_pass_stat);
        if (ret == EB_ErrorNone)
        {
            interjob->context = av_malloc(first_pass_stat.sz);
            if (interjob->context)
            {
                memcpy(interjob->context, first_pass_stat.buf, first_pass_stat.sz);
                interjob->context_size = first_pass_stat.sz;
            }
        }
    }
}

int encsvtWork(hb_work_object_t *w, hb_buffer_t **buf_in,
               hb_buffer_t **buf_out)
{
    hb_buffer_t      *in = *buf_in;
    hb_buffer_list_t  list;

    *buf_out = NULL;
    hb_buffer_list_clear(&list);

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        // EOF on input. Flush any frames still in the decoder then
        // send the EOF downstream to tell the muxer we're done.
        flush(w, in, &list);
        hb_buffer_list_append(&list, hb_buffer_eof_init());

        *buf_out = hb_buffer_list_clear(&list);
        return HB_WORK_DONE;
    }

    // Not EOF - encode the packet
    encode(w, in, &list);
    *buf_out = hb_buffer_list_clear(&list);

    return HB_WORK_OK;
}
