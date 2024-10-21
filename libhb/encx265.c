/* encx265.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/project.h"

#if HB_PROJECT_FEATURE_X265

#include "libavutil/avutil.h"
#include "handbrake/handbrake.h"
#include "handbrake/hb_dict.h"
#include "handbrake/h265_common.h"
#include "handbrake/dovi_common.h"
#include "handbrake/hdr10plus.h"
#include "handbrake/extradata.h"

#include "x265.h"

int  encx265Init (hb_work_object_t*, hb_job_t*);
int  encx265Work (hb_work_object_t*, hb_buffer_t**, hb_buffer_t**);
void encx265Close(hb_work_object_t*);

hb_work_object_t hb_encx265 =
{
    WORK_ENCX265,
    "H.265/HEVC encoder (libx265)",
    encx265Init,
    encx265Work,
    encx265Close,
};

#define FRAME_INFO_MAX2 (8)  // 2^8  = 256;  90000/256    = 352 frames/sec
#define FRAME_INFO_MIN2 (17) // 2^17 = 128K; 90000/131072 = 1.4 frames/sec
#define FRAME_INFO_SIZE (1 << (FRAME_INFO_MIN2 - FRAME_INFO_MAX2 + 1))
#define FRAME_INFO_MASK (FRAME_INFO_SIZE - 1)

#define MASTERING_CHROMA_DEN 50000
#define MASTERING_LUMA_DEN 10000

static const char * const hb_x265_encopt_synonyms[][2] =
{
    { "me", "motion", },
    { NULL,  NULL,    },
};

struct hb_work_private_s
{
    hb_job_t           * job;
    x265_encoder       * x265;
    x265_param         * param;

    int64_t              last_stop;
    uint32_t             frames_in;

    hb_chapter_queue_t * chapter_queue;

    struct
    {
        int64_t          duration;
    } frame_info[FRAME_INFO_SIZE];

    char               * csvfn;

    // Multiple bit-depth
    const x265_api     * api;
    int                  bit_depth;

    void               * sei_data;
    unsigned int         sei_data_size;
};

static int param_parse(hb_work_private_t *pv, x265_param *param,
                       const char *key, const char *value)
{
    int ret = pv->api->param_parse(param, key, value);
    // let x265 sanity check the options for us
    switch (ret)
    {
    case X265_PARAM_BAD_NAME:
        hb_log("encx265: unknown option '%s'", key);
        break;
    case X265_PARAM_BAD_VALUE:
        hb_log("encx265: bad argument '%s=%s'", key, value ? value : "(null)");
        break;
    default:
        break;
    }
    return ret;
}

int apply_h265_level(hb_work_private_t *pv,  x265_param *param,
                     const char *h265_level)
{
    if (h265_level == NULL ||
        !strcasecmp(h265_level, hb_h265_level_names[0]))
    {
        return 0;
    }
    // Verify that level is valid
    int i;
    for (i = 1; hb_h265_level_values[i]; i++)
    {
        if (!strcmp(hb_h265_level_names[i], h265_level) ||
            !strcmp(hb_h265_level_names2[i], h265_level))
        {
            return param_parse(pv, param, "level-idc",
                    hb_h265_level_names2[i]);
        }
    }

    // error (invalid or unsupported level), abort
    hb_error("apply_h265_level: invalid level %s", h265_level);
    return X265_PARAM_BAD_VALUE;
}


/***********************************************************************
 * hb_work_encx265_init
 ***********************************************************************
 *
 **********************************************************************/
int encx265Init(hb_work_object_t *w, hb_job_t *job)
{
    hb_work_private_t  *pv = calloc(1, sizeof(hb_work_private_t));
    int                 ret, depth;
    x265_nal           *nal;
    uint32_t            nnal;
    const char * const *profile_names;

    pv->job              = job;
    pv->last_stop        = AV_NOPTS_VALUE;
    pv->chapter_queue    = hb_chapter_queue_init();
    w->private_data      = pv;

    depth                = hb_video_encoder_get_depth(job->vcodec);
    profile_names        = hb_video_encoder_get_profiles(job->vcodec);
    pv->api              = x265_api_query(depth, X265_BUILD, NULL);

    if (pv->api == NULL)
    {
        hb_error("encx265: x265_api_query failed, bit depth %d.", depth);
        goto fail;
    }

    x265_param *param = pv->param = pv->api->param_alloc();

    if (pv->api->param_default_preset(param, job->encoder_preset,
                                      job->encoder_tune) < 0)
    {
        hb_error("encx265: x265_param_default_preset failed. Preset (%s) Tune (%s)", job->encoder_preset, job->encoder_tune);
        goto fail;
    }

    /* If the PSNR or SSIM tunes are in use, enable the relevant metric */
    param->bEnablePsnr = param->bEnableSsim = 0;
    if (job->encoder_tune != NULL && *job->encoder_tune)
    {
        char *tmp = strdup(job->encoder_tune);
        char *tok = strtok(tmp,   ",./-+");
        do
        {
            if (!strncasecmp(tok, "psnr", 4))
            {
                param->bEnablePsnr = 1;
                break;
            }
            if (!strncasecmp(tok, "ssim", 4))
            {
                param->bEnableSsim = 1;
                break;
            }
        }
        while ((tok = strtok(NULL, ",./-+")) != NULL);
        free(tmp);
    }

    /*
     * Some HandBrake-specific defaults; users can override them
     * using the encoder_options string.
     */
    param->fpsNum      = job->orig_vrate.num;
    param->fpsDenom    = job->orig_vrate.den;
    param->keyframeMin = (double)job->orig_vrate.num / job->orig_vrate.den +
                                 0.5;
    param->keyframeMax = param->keyframeMin * 10;

    /*
     * Video Signal Type (color description only).
     *
     * Use x265_param_parse (let x265 determine which bEnable
     * flags, if any, should be set in the x265_param struct).
     */
    char colorprim[11], transfer[11], colormatrix[11];
    snprintf(colorprim,   sizeof(colorprim),   "%d", hb_output_color_prim(job));
    snprintf(transfer,    sizeof(transfer),    "%d", hb_output_color_transfer(job));
    snprintf(colormatrix, sizeof(colormatrix), "%d", hb_output_color_matrix(job));

    if (param_parse(pv, param, "colorprim",   colorprim)   ||
        param_parse(pv, param, "transfer",    transfer)    ||
        param_parse(pv, param, "colormatrix", colormatrix))
    {
        goto fail;
    }

    const char *colorrange = job->color_range == AVCOL_RANGE_MPEG ? "limited" : "full";

    if (param_parse(pv, param, "range",   colorrange))
    {
        goto fail;
    }

    /*
     * HDR10 Static metadata
     */
    if (job->color_transfer == HB_COLR_TRA_SMPTEST2084)
    {
        if (depth > 8)
        {
            if (param_parse(pv, param, "hdr10-opt", "1"))
            {
                goto fail;
            }
        }

        /*
         * Mastering display metadata.
         */
        if (job->mastering.has_primaries && job->mastering.has_luminance)
        {
            char masteringDisplayColorVolume[256];
            snprintf(masteringDisplayColorVolume, sizeof(masteringDisplayColorVolume),
                     "G(%hu,%hu)B(%hu,%hu)R(%hu,%hu)WP(%hu,%hu)L(%u,%u)",
                     (unsigned short)hb_rescale_rational(job->mastering.display_primaries[1][0], MASTERING_CHROMA_DEN),
                     (unsigned short)hb_rescale_rational(job->mastering.display_primaries[1][1], MASTERING_CHROMA_DEN),
                     (unsigned short)hb_rescale_rational(job->mastering.display_primaries[2][0], MASTERING_CHROMA_DEN),
                     (unsigned short)hb_rescale_rational(job->mastering.display_primaries[2][1], MASTERING_CHROMA_DEN),
                     (unsigned short)hb_rescale_rational(job->mastering.display_primaries[0][0], MASTERING_CHROMA_DEN),
                     (unsigned short)hb_rescale_rational(job->mastering.display_primaries[0][1], MASTERING_CHROMA_DEN),
                     (unsigned short)hb_rescale_rational(job->mastering.white_point[0], MASTERING_CHROMA_DEN),
                     (unsigned short)hb_rescale_rational(job->mastering.white_point[1], MASTERING_CHROMA_DEN),
                     (unsigned)hb_rescale_rational(job->mastering.max_luminance, MASTERING_LUMA_DEN),
                     (unsigned)hb_rescale_rational(job->mastering.min_luminance, MASTERING_LUMA_DEN));

            if (param_parse(pv, param, "master-display", masteringDisplayColorVolume))
            {
                goto fail;
            }
        }

        /*
         * Content light level.
         */
        if (job->coll.max_cll && job->coll.max_fall)
        {
            char contentLightLevel[256];
            snprintf(contentLightLevel, sizeof(contentLightLevel),
                     "%hu,%hu",
                     (unsigned short)job->coll.max_cll, (unsigned short)job->coll.max_fall);

            if (param_parse(pv, param, "max-cll", contentLightLevel))
            {
                goto fail;
            }
        }
    }

    if (job->ambient.ambient_illuminance.num && job->ambient.ambient_illuminance.den)
    {
        param->ambientIlluminance = hb_rescale_rational(job->ambient.ambient_illuminance, 10000);
        param->ambientLightX = hb_rescale_rational(job->ambient.ambient_light_x, 50000);
        param->ambientLightY = hb_rescale_rational(job->ambient.ambient_light_y, 50000);
        param->bEmitAmbientViewingEnvironment = 1;
    }

    if (job->chroma_location != AVCHROMA_LOC_UNSPECIFIED)
    {
        char chromaLocation[256];
        snprintf(chromaLocation, sizeof(chromaLocation),
                 "%d",
                 job->chroma_location - 1);

        if (param_parse(pv, param, "chromaloc", chromaLocation))
        {
            goto fail;
        }
    }

    /* Bit depth */
    pv->bit_depth = hb_get_bit_depth(job->output_pix_fmt);

    /* iterate through x265_opts and parse the options */
    hb_dict_t *x265_opts;
    int override_mastering = 0, override_coll = 0, override_chroma_location = 0;
    x265_opts = hb_encopts_to_dict(job->encoder_options, job->vcodec);

    hb_dict_iter_t iter;
    for (iter  = hb_dict_iter_init(x265_opts);
         iter != HB_DICT_ITER_DONE;
         iter  = hb_dict_iter_next(x265_opts, iter))
    {
        const char *key = hb_dict_iter_key(iter);
        hb_value_t *value = hb_dict_iter_value(iter);
        char *str = hb_value_get_string_xform(value);

        if (!strcmp(key, "master-display"))
        {
            override_mastering = 1;
        }
        if (!strcmp(key, "max-cll"))
        {
            override_coll = 1;
        }
        if (!strcmp(key, "chromaloc"))
        {
            override_chroma_location = 1;
        }

        // here's where the strings are passed to libx265 for parsing
        // unknown options or bad values are non-fatal, see encx264.c
        param_parse(pv, param, key, str);
        free(str);
    }
    hb_dict_free(&x265_opts);

    /*
     * Reload colorimetry settings in case custom
     * values were set in the encoder_options string.
     */
    job->color_prim_override     = param->vui.colorPrimaries;
    job->color_transfer_override = param->vui.transferCharacteristics;
    job->color_matrix_override   = param->vui.matrixCoeffs;

    /*
     * Reload mastering settings in case custom
     * values were set in the encoder_options string.
     */
    if (override_mastering)
    {
        uint16_t display_primaries_x[3];
        uint16_t display_primaries_y[3];
        uint16_t white_point_x, white_point_y;
        uint32_t max_luminance, min_luminance;

        if (sscanf(param->masteringDisplayColorVolume, "G(%hu,%hu)B(%hu,%hu)R(%hu,%hu)WP(%hu,%hu)L(%u,%u)",
                      &display_primaries_x[1], &display_primaries_y[1],
                      &display_primaries_x[2], &display_primaries_y[2],
                      &display_primaries_x[0], &display_primaries_y[0],
                      &white_point_x, &white_point_y,
                      &max_luminance, &min_luminance) == 10)
        {
            for (int i = 0; i < 3; i++)
            {
                job->mastering.display_primaries[i][0] = hb_make_q(display_primaries_x[i], MASTERING_CHROMA_DEN);
                job->mastering.display_primaries[i][1] = hb_make_q(display_primaries_y[i], MASTERING_CHROMA_DEN);
            }

            job->mastering.white_point[0] = hb_make_q(white_point_x, MASTERING_CHROMA_DEN);
            job->mastering.white_point[1] = hb_make_q(white_point_y, MASTERING_CHROMA_DEN);

            job->mastering.min_luminance = hb_make_q(min_luminance, MASTERING_LUMA_DEN);
            job->mastering.max_luminance = hb_make_q(max_luminance, MASTERING_LUMA_DEN);
        }
    }

    /*
     * Reload content light level settings in case custom
     * values were set in the encoder_options string.
     */
    if (override_coll)
    {
        job->coll.max_fall = param->maxFALL;
        job->coll.max_cll  = param->maxCLL;
    }

    /*
     * Reload chroma location settings in case custom
     * values were set in the encoder_options string.
     */
    if (override_chroma_location)
    {
        job->chroma_location = param->vui.chromaSampleLocTypeBottomField + 1;
    }

    if (job->inline_parameter_sets)
    {
        param->bRepeatHeaders = 1;
    }

    /*
     * Settings which can't be overridden in the encoder_options string
     * (muxer-specific settings, resolution, ratecontrol, etc.).
     */
    param->sourceWidth    = job->width;
    param->sourceHeight   = job->height;

    switch (job->output_pix_fmt)
    {
        case AV_PIX_FMT_YUV422P:
        case AV_PIX_FMT_YUV422P10:
        case AV_PIX_FMT_YUV422P12:
        case AV_PIX_FMT_YUV422P16:
            param->internalCsp = X265_CSP_I422;
            break;
        case AV_PIX_FMT_YUV444P:
        case AV_PIX_FMT_YUV444P10:
        case AV_PIX_FMT_YUV444P12:
        case AV_PIX_FMT_YUV444P16:
            param->internalCsp = X265_CSP_I444;
    }

    /*
     * Let x265 determine whether to use an aspect ratio
     * index vs. the extended SAR index + SAR width/height.
     */
    char sar[22];
    snprintf(sar, sizeof(sar), "%d:%d", job->par.num, job->par.den);
    if (param_parse(pv, param, "sar", sar))
    {
        goto fail;
    }

    if (job->vquality > HB_INVALID_VIDEO_QUALITY)
    {
        param->rc.rateControlMode = X265_RC_CRF;
        param->rc.rfConstant      = job->vquality;
    }
    else
    {
        param->rc.rateControlMode = X265_RC_ABR;
        param->rc.bitrate         = job->vbitrate;
        if (job->pass_id == HB_PASS_ENCODE_ANALYSIS ||
            job->pass_id == HB_PASS_ENCODE_FINAL)
        {
            char * stats_file;
            char   pass[2];
            snprintf(pass, sizeof(pass), "%d", job->pass_id);
            stats_file = hb_get_temporary_filename("x265.log");
            if (param_parse(pv, param, "stats", stats_file) ||
                param_parse(pv, param, "pass", pass))
            {
                free(stats_file);
                goto fail;
            }
            free(stats_file);
            if (job->pass_id == HB_PASS_ENCODE_ANALYSIS)
            {
                char slowfirstpass[2];
                snprintf(slowfirstpass, sizeof(slowfirstpass), "%d",
                         !job->fastanalysispass);
                if (param_parse(pv, param, "slow-firstpass", slowfirstpass))
                {
                    goto fail;
                }
            }
        }
    }

    /* statsfile (but not 2-pass) */
    if (param->logLevel >= X265_LOG_DEBUG)
    {
        if (param->csvfn == NULL)
        {
            pv->csvfn = hb_get_temporary_filename("x265.csv");
            param->csvfn = strdup(pv->csvfn);
        }
        else
        {
            pv->csvfn = strdup(param->csvfn);
        }
    }

    /* Apply profile and level settings last. */
    if (job->encoder_profile                                      != NULL &&
        strcasecmp(job->encoder_profile, profile_names[0])        != 0    &&
        pv->api->param_apply_profile(param, job->encoder_profile) < 0)
    {
        goto fail;
    }
    if (apply_h265_level(pv, param, job->encoder_level) < 0)
    {
        goto fail;
    }

    /* we should now know whether B-frames are enabled */
    job->areBframes = (param->bframes > 0) + (param->bframes   > 0 &&
                                              param->bBPyramid > 0);

    /*
     * Update and set Dolby Vision level
     */
    if (job->passthru_dynamic_hdr_metadata & DOVI)
    {
        char dolbyVisionProfile[256];
        snprintf(dolbyVisionProfile, sizeof(dolbyVisionProfile),
                 "%hu%hu",
                 (unsigned short)job->dovi.dv_profile,
                 (unsigned short)job->dovi.dv_bl_signal_compatibility_id);

        if (param_parse(pv, param, "dolby-vision-profile", dolbyVisionProfile))
        {
            goto fail;
        }

        int pps = (double)job->width * job->height * (job->vrate.num / job->vrate.den);
        int bitrate = job->vquality == HB_INVALID_VIDEO_QUALITY ? job->vbitrate : -1;

        // Dolby Vision requires VBV settings to enable HRD
        // set the max value for the current level or guess one
        if (param->rc.vbvMaxBitrate == 0 || param->rc.vbvBufferSize == 0)
        {
            int max_rate = hb_dovi_max_rate(job->vcodec, job->width, pps, bitrate,
                                            param->levelIdc, param->bHighTier);
            param->rc.vbvMaxBitrate = max_rate;
            param->rc.vbvBufferSize = max_rate;
        }

        job->dovi.dv_level = hb_dovi_level(job->width, pps, param->rc.vbvMaxBitrate, param->bHighTier);
    }

    /* Reset global variables before opening a new encoder */
    pv->api->cleanup();

    pv->x265 = pv->api->encoder_open(param);
    if (pv->x265 == NULL)
    {
        hb_error("encx265: x265_encoder_open failed.");
        goto fail;
    }

    /*
     * x265's output (headers and bitstream) are in Annex B format.
     *
     * Write the header as is, and let the muxer reformat
     * the extradata and output bitstream properly for us.
     */
    ret = pv->api->encoder_headers(pv->x265, &nal, &nnal);
    if (ret < 0)
    {
        hb_error("encx265: x265_encoder_headers failed (%d)", ret);
        goto fail;
    }
    if (hb_set_extradata(w->extradata, nal->payload, ret))
    {
        hb_error("encx265: bitstream headers too large (%d)", ret);
        goto fail;
    }

    return 0;

fail:
    w->private_data = NULL;
    free(pv);
    return 1;
}

void encx265Close(hb_work_object_t *w)
{
    hb_work_private_t *pv = w->private_data;

    if (pv == NULL)
    {
        return;
    }
    hb_chapter_queue_close(&pv->chapter_queue);

    pv->api->param_free(pv->param);
    pv->api->encoder_close(pv->x265);
    // x265 has got some global variables that prevents
    // multiple encode with different settings in the same process
    // Clean-up it here to avoid unneccessary memory pressure,
    // they will be recreated in the next encode
    pv->api->cleanup();
    free(pv->csvfn);
    av_freep(&pv->sei_data);
    free(pv);
    w->private_data = NULL;
}

/*
 * see comments in definition of 'frame_info' in pv struct for description
 * of what these routines are doing.
 */
static void save_frame_info(hb_work_private_t *pv, hb_buffer_t *in)
{
    int i = (in->s.start >> FRAME_INFO_MAX2) & FRAME_INFO_MASK;
    pv->frame_info[i].duration = in->s.stop - in->s.start;
}
static int64_t get_frame_duration(hb_work_private_t * pv, int64_t pts)
{
    int i = (pts >> FRAME_INFO_MAX2) & FRAME_INFO_MASK;
    return pv->frame_info[i].duration;
}

static hb_buffer_t* nal_encode(hb_work_object_t *w,
                               x265_picture *pic_out,
                               x265_nal *nal, uint32_t nnal)
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *buf      = NULL;
    int payload_size      = 0;

    if (nnal <= 0)
    {
        return NULL;
    }

    for (int i = 0; i < nnal; i++)
    {
        payload_size += nal[i].sizeBytes;
    }

    buf = hb_buffer_init(payload_size);
    if (buf == NULL)
    {
        return NULL;
    }

    buf->s.flags = 0;
    buf->size = 0;

    // copy the bitstream data
    for (int i = 0; i < nnal; i++)
    {
        if (HB_HEVC_NALU_KEYFRAME(nal[i].type))
        {
            buf->s.flags |= HB_FLAG_FRAMETYPE_REF;
            buf->s.flags |= HB_FLAG_FRAMETYPE_KEY;
        }
        memcpy(buf->data + buf->size, nal[i].payload, nal[i].sizeBytes);
        buf->size += nal[i].sizeBytes;
    }

    // use the pts to get the original frame's duration.
    buf->s.duration     = get_frame_duration(pv, pic_out->pts);
    buf->s.stop         = pic_out->pts + buf->s.duration;
    buf->s.start        = pic_out->pts;
    buf->s.renderOffset = pic_out->dts;
    if (*w->init_delay == 0 && pic_out->dts < 0)
    {
        *w->init_delay -= pic_out->dts;
    }

    switch (pic_out->sliceType)
    {
        case X265_TYPE_IDR:
            buf->s.flags |= HB_FLAG_FRAMETYPE_REF;
            buf->s.flags |= HB_FLAG_FRAMETYPE_KEY;
            buf->s.frametype = HB_FRAME_IDR;
            break;
        case X265_TYPE_P:
            buf->s.flags |= HB_FLAG_FRAMETYPE_REF;
            buf->s.frametype = HB_FRAME_P;
            break;
        case X265_TYPE_B:
            buf->s.frametype = HB_FRAME_B;
            break;
        case X265_TYPE_BREF:
            buf->s.flags |= HB_FLAG_FRAMETYPE_REF;
            buf->s.frametype = HB_FRAME_BREF;
            break;
        case X265_TYPE_I:
        default:
            buf->s.flags |= HB_FLAG_FRAMETYPE_REF;
            buf->s.frametype = HB_FRAME_I;
            break;
    }

    if (buf->s.flags & HB_FLAG_FRAMETYPE_KEY)
    {
        hb_chapter_dequeue(pv->chapter_queue, buf);
    }

    // discard empty buffers (no video)
    if (buf->size <= 0)
    {
        hb_buffer_close(&buf);
    }
    return buf;
}

static hb_buffer_t* x265_encode(hb_work_object_t *w, hb_buffer_t *in)
{
    hb_work_private_t *pv = w->private_data;
    hb_job_t *job         = pv->job;

    x265_picture pic_in;
    x265_picture  pic_layers_out[MAX_SCALABLE_LAYERS];
    x265_picture *pic_lyrptr_out[MAX_SCALABLE_LAYERS];

    x265_nal *nal;
    uint32_t nnal;
    int ret;

    for (int i = 0; i < MAX_SCALABLE_LAYERS; i++)
    {
        pic_lyrptr_out[i] = &pic_layers_out[i];
    }

    pv->api->picture_init(pv->param, &pic_in);

    pic_in.stride[0] = in->plane[0].stride;
    pic_in.stride[1] = in->plane[1].stride;
    pic_in.stride[2] = in->plane[2].stride;
    pic_in.planes[0] = in->plane[0].data;
    pic_in.planes[1] = in->plane[1].data;
    pic_in.planes[2] = in->plane[2].data;
    pic_in.poc       = pv->frames_in++;
    pic_in.pts       = in->s.start;
    pic_in.bitDepth  = pv->bit_depth;

    x265_sei *sei = &pic_in.userSEI;
    sei->numPayloads = 0;

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
                void *tmp;
                x265_sei_payload *sei_payload = NULL;

                hb_dynamic_hdr10_plus_to_itu_t_t35((AVDynamicHDRPlus *)side_data->data, &payload, &playload_size);
                if (!playload_size)
                {
                    continue;
                }

                tmp = av_fast_realloc(pv->sei_data, &pv->sei_data_size,
                                      (sei->numPayloads + 1) * sizeof(*sei_payload));

                if (!tmp)
                {
                    continue;
                }

                pv->sei_data = tmp;
                sei->payloads = pv->sei_data;
                sei_payload = &sei->payloads[sei->numPayloads];
                sei_payload->payload = payload;
                sei_payload->payloadSize = playload_size;
                sei_payload->payloadType = USER_DATA_REGISTERED_ITU_T_T35;
                sei->numPayloads++;
            }
            if (job->passthru_dynamic_hdr_metadata & DOVI &&
                side_data->type == AV_FRAME_DATA_DOVI_RPU_BUFFER)
            {
                x265_dolby_vision_rpu *rpu = &pic_in.rpu;
                rpu->payload = side_data->data;
                rpu->payloadSize = side_data->size;
            }
        }
    }

    if (in->s.new_chap && job->chapter_markers)
    {
        /*
         * Chapters have to start with an IDR frame so request that this
         * frame be coded as IDR. Since there may be up to 16 frames
         * currently buffered in the encoder, remember the timestamp so
         * when this frame finally pops out of the encoder we'll mark
         * its buffer as the start of a chapter.
         */
        pic_in.sliceType = X265_TYPE_IDR;
        hb_chapter_enqueue(pv->chapter_queue, in);
    }
    else
    {
        pic_in.sliceType = X265_TYPE_AUTO;
    }

    if (pv->last_stop != AV_NOPTS_VALUE && pv->last_stop != in->s.start)
    {
        hb_log("encx265 input continuity err: last stop %"PRId64"  start %"PRId64,
               pv->last_stop, in->s.start);
    }
    pv->last_stop = in->s.stop;
    save_frame_info(pv, in);

    ret = pv->api->encoder_encode(pv->x265, &nal, &nnal, &pic_in, pic_lyrptr_out);

    for (int i = 0; i < sei->numPayloads; i++)
    {
        x265_sei_payload *sei_payload = &sei->payloads[i];
        av_freep(&sei_payload->payload);
    }

    if (ret > 0)
    {
        return nal_encode(w, pic_lyrptr_out[0], nal, nnal);
    }
    return NULL;
}

int encx265Work(hb_work_object_t *w, hb_buffer_t **buf_in, hb_buffer_t **buf_out)
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t       *in = *buf_in;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        uint32_t nnal;
        x265_nal *nal;
        x265_picture  pic_layers_out[MAX_SCALABLE_LAYERS];
        x265_picture *pic_lyrptr_out[MAX_SCALABLE_LAYERS];
        hb_buffer_list_t list;

        hb_buffer_list_clear(&list);

        for (int i = 0; i < MAX_SCALABLE_LAYERS; i++)
        {
            pic_lyrptr_out[i] = &pic_layers_out[i];
        }

        // flush delayed frames
        while (pv->api->encoder_encode(pv->x265, &nal,
                                       &nnal, NULL, pic_lyrptr_out) > 0)
        {
            hb_buffer_t *buf = nal_encode(w, pic_lyrptr_out[0], nal, nnal);
            hb_buffer_list_append(&list, buf);
        }
        // add the EOF to the end of the chain
        hb_buffer_list_append(&list, in);

        *buf_out = hb_buffer_list_clear(&list);
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    *buf_out = x265_encode(w, in);
    return HB_WORK_OK;
}

const char* hb_x265_encopt_name(const char *name)
{
    int i;
    for (i = 0; hb_x265_encopt_synonyms[i][0] != NULL; i++)
        if (!strcmp(name, hb_x265_encopt_synonyms[i][1]))
            return hb_x265_encopt_synonyms[i][0];
    return name;
}

#endif
