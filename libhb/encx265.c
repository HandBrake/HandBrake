/* encx265.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
#ifdef USE_X265

#include "hb.h"
#include "hb_dict.h"
#include "h265_common.h"
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

static const char * const hb_x265_encopt_synonyms[][2] =
{
    { "me", "motion", },
    { NULL,  NULL,    },
};

struct hb_work_private_s
{
    hb_job_t       *job;
    x265_encoder   *x265;
    x265_param     *param;

    int64_t         last_stop;
    uint32_t        frames_in;

    hb_list_t      *delayed_chapters;
    int64_t         next_chapter_pts;

    struct
    {
        int64_t     duration;
    }
    frame_info[FRAME_INFO_SIZE];

    char            csvfn[1024];

    // Multiple bit-depth
    const x265_api *api;
};

// used in delayed_chapters list
struct chapter_s
{
    int     index;
    int64_t start;
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

/***********************************************************************
 * hb_work_encx265_init
 ***********************************************************************
 *
 **********************************************************************/
int encx265Init(hb_work_object_t *w, hb_job_t *job)
{
    hb_work_private_t  *pv = calloc(1, sizeof(hb_work_private_t));
    int                 ret, depth;
    hb_rational_t       vrate;
    x265_nal           *nal;
    uint32_t            nnal;
    const char * const *profile_names;

    pv->next_chapter_pts = AV_NOPTS_VALUE;
    pv->delayed_chapters = hb_list_init();
    pv->job              = job;
    w->private_data      = pv;

    depth                = hb_video_encoder_get_depth(job->vcodec);
    profile_names        = hb_video_encoder_get_profiles(job->vcodec);
    pv->api              = x265_api_get(depth);

    if (pv->api == NULL)
    {
        hb_error("encx265: x265_api_get failed, bit depth %d.", depth);
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
    hb_reduce(&vrate.num, &vrate.den, job->vrate.num, job->vrate.den);
    param->fpsNum      = vrate.num;
    param->fpsDenom    = vrate.den;
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
    switch (job->color_matrix_code)
    {
        case 1: // ITU BT.601 DVD or SD TV content (NTSC)
            strcpy(colorprim,   "smpte170m");
            strcpy(transfer,        "bt709");
            strcpy(colormatrix, "smpte170m");
            break;
        case 2: // ITU BT.601 DVD or SD TV content (PAL)
            strcpy(colorprim,     "bt470bg");
            strcpy(transfer,        "bt709");
            strcpy(colormatrix, "smpte170m");
            break;
        case 3: // ITU BT.709 HD content
            strcpy(colorprim,   "bt709");
            strcpy(transfer,    "bt709");
            strcpy(colormatrix, "bt709");
            break;
        case 4: // custom
            snprintf(colorprim,   sizeof(colorprim),   "%d", job->color_prim);
            snprintf(transfer,    sizeof(transfer),    "%d", job->color_transfer);
            snprintf(colormatrix, sizeof(colormatrix), "%d", job->color_matrix);
            break;
        default: // detected during scan
            snprintf(colorprim,   sizeof(colorprim),   "%d", job->title->color_prim);
            snprintf(transfer,    sizeof(transfer),    "%d", job->title->color_transfer);
            snprintf(colormatrix, sizeof(colormatrix), "%d", job->title->color_matrix);
            break;
    }
    if (param_parse(pv, param, "colorprim",   colorprim)   ||
        param_parse(pv, param, "transfer",    transfer)    ||
        param_parse(pv, param, "colormatrix", colormatrix))
    {
        goto fail;
    }

    /* iterate through x265_opts and parse the options */
    hb_dict_t *x265_opts;
    x265_opts = hb_encopts_to_dict(job->encoder_options, job->vcodec);

    hb_dict_iter_t iter;
    for (iter  = hb_dict_iter_init(x265_opts);
         iter != HB_DICT_ITER_DONE;
         iter  = hb_dict_iter_next(x265_opts, iter))
    {
        const char *key = hb_dict_iter_key(iter);
        hb_value_t *value = hb_dict_iter_value(iter);
        char *str = hb_value_get_string_xform(value);

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
    job->color_matrix_code = 4;
    job->color_prim        = param->vui.colorPrimaries;
    job->color_transfer    = param->vui.transferCharacteristics;
    job->color_matrix      = param->vui.matrixCoeffs;

    /*
     * Settings which can't be overriden in the encodeer_options string
     * (muxer-specific settings, resolution, ratecontrol, etc.).
     */
    param->bRepeatHeaders = 0;
    param->sourceWidth    = job->width;
    param->sourceHeight   = job->height;

    /*
     * Let x265 determnine whether to use an aspect ratio
     * index vs. the extended SAR index + SAR width/height.
     */
    char sar[22];
    snprintf(sar, sizeof(sar), "%d:%d", job->par.num, job->par.den);
    if (param_parse(pv, param, "sar", sar))
    {
        goto fail;
    }

    if (job->vquality > 0)
    {
        param->rc.rateControlMode = X265_RC_CRF;
        param->rc.rfConstant      = job->vquality;
    }
    else
    {
        param->rc.rateControlMode = X265_RC_ABR;
        param->rc.bitrate         = job->vbitrate;
        if (job->pass_id == HB_PASS_ENCODE_1ST ||
            job->pass_id == HB_PASS_ENCODE_2ND)
        {
            char stats_file[1024] = "";
            char pass[2];
            snprintf(pass, sizeof(pass), "%d", job->pass_id);
            hb_get_tempory_filename(job->h, stats_file, "x265.log");
            if (param_parse(pv, param, "stats", stats_file) ||
                param_parse(pv, param, "pass", pass))
            {
                goto fail;
            }
            if (job->pass_id == HB_PASS_ENCODE_1ST && job->fastfirstpass == 0 &&
                param_parse(pv, param, "slow-firstpass", "1"))
            {
                goto fail;
            }
        }
    }

    /* statsfile (but not 2-pass) */
    memset(pv->csvfn, 0, sizeof(pv->csvfn));
    if (param->logLevel >= X265_LOG_DEBUG)
    {
        if (param->csvfn == NULL)
        {
            hb_get_tempory_filename(job->h, pv->csvfn, "x265.csv");
            param->csvfn = pv->csvfn;
        }
        else
        {
            strncpy(pv->csvfn, param->csvfn, sizeof(pv->csvfn));
        }
    }

    /* Apply profile and level settings last. */
    if (job->encoder_profile                                      != NULL &&
        strcasecmp(job->encoder_profile, profile_names[0])        != 0    &&
        pv->api->param_apply_profile(param, job->encoder_profile) < 0)
    {
        goto fail;
    }

    /* we should now know whether B-frames are enabled */
    job->areBframes = (param->bframes > 0) + (param->bframes   > 0 &&
                                              param->bBPyramid > 0);

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
    if (ret > sizeof(w->config->h265.headers))
    {
        hb_error("encx265: bitstream headers too large (%d)", ret);
        goto fail;
    }
    memcpy(w->config->h265.headers, nal->payload, ret);
    w->config->h265.headers_length = ret;

    return 0;

fail:
    w->private_data = NULL;
    free(pv);
    return 1;
}

void encx265Close(hb_work_object_t *w)
{
    hb_work_private_t *pv = w->private_data;

    if (pv == NULL) return;
    if (pv->delayed_chapters != NULL)
    {
        struct chapter_s *item;
        while ((item = hb_list_item(pv->delayed_chapters, 0)) != NULL)
        {
            hb_list_rem(pv->delayed_chapters, item);
            free(item);
        }
        hb_list_close(&pv->delayed_chapters);
    }

    pv->api->param_free(pv->param);
    pv->api->encoder_close(pv->x265);
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
    hb_job_t *job         = pv->job;
    hb_buffer_t *buf      = NULL;
    int i;

    if (nnal <= 0)
    {
        return NULL;
    }

    buf = hb_video_buffer_init(job->width, job->height);
    if (buf == NULL)
    {
        return NULL;
    }

    buf->size = 0;
    // copy the bitstream data
    for (i = 0; i < nnal; i++)
    {
        memcpy(buf->data + buf->size, nal[i].payload, nal[i].sizeBytes);
        buf->size += nal[i].sizeBytes;
    }

    // use the pts to get the original frame's duration.
    buf->s.duration     = get_frame_duration(pv, pic_out->pts);
    buf->s.stop         = pic_out->pts + buf->s.duration;
    buf->s.start        = pic_out->pts;
    buf->s.renderOffset = pic_out->dts;
    if (w->config->h264.init_delay == 0 && pic_out->dts < 0)
    {
        w->config->h264.init_delay -= pic_out->dts;
    }

    switch (pic_out->sliceType)
    {
        case X265_TYPE_IDR:
            buf->s.frametype = HB_FRAME_IDR;
            break;
        case X265_TYPE_I:
            buf->s.frametype = HB_FRAME_I;
            break;
        case X265_TYPE_P:
            buf->s.frametype = HB_FRAME_P;
            break;
        case X265_TYPE_B:
            buf->s.frametype = HB_FRAME_B;
            break;
        case X265_TYPE_BREF:
            buf->s.frametype = HB_FRAME_BREF;
            break;
        default:
            buf->s.frametype = 0;
            break;
    }

    if (pv->next_chapter_pts != AV_NOPTS_VALUE &&
        pv->next_chapter_pts <= pic_out->pts   &&
        pic_out->sliceType   == X265_TYPE_IDR)
    {
        // we're no longer looking for this chapter
        pv->next_chapter_pts = AV_NOPTS_VALUE;

        // get the chapter index from the list
        struct chapter_s *item = hb_list_item(pv->delayed_chapters, 0);
        if (item != NULL)
        {
            // we're done with this chapter
            hb_list_rem(pv->delayed_chapters, item);
            buf->s.new_chap = item->index;
            free(item);

            // we may still have another pending chapter
            item = hb_list_item(pv->delayed_chapters, 0);
            if (item != NULL)
            {
                // we're looking for this one now
                // we still need it, don't remove it
                pv->next_chapter_pts = item->start;
            }
        }
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
    x265_picture pic_in, pic_out;
    x265_nal *nal;
    uint32_t nnal;

    pv->api->picture_init(pv->param, &pic_in);

    pic_in.stride[0] = in->plane[0].stride;
    pic_in.stride[1] = in->plane[1].stride;
    pic_in.stride[2] = in->plane[2].stride;
    pic_in.planes[0] = in->plane[0].data;
    pic_in.planes[1] = in->plane[1].data;
    pic_in.planes[2] = in->plane[2].data;
    pic_in.poc       = pv->frames_in++;
    pic_in.pts       = in->s.start;
    pic_in.bitDepth  = 8;

    if (in->s.new_chap && job->chapter_markers)
    {
        if (pv->next_chapter_pts == AV_NOPTS_VALUE)
        {
            pv->next_chapter_pts = in->s.start;
        }
        /*
         * Chapter markers are sometimes so close we can get a new one before
         * the previous marker has been through the encoding queue.
         *
         * Dropping markers can cause weird side-effects downstream, including
         * but not limited to missing chapters in the output, so we need to save
         * it somehow.
         */
        struct chapter_s *item = malloc(sizeof(struct chapter_s));
        if (item != NULL)
        {
            item->start = in->s.start;
            item->index = in->s.new_chap;
            hb_list_add(pv->delayed_chapters, item);
        }
        /* don't let 'work_loop' put a chapter mark on the wrong buffer */
        in->s.new_chap = 0;
        /*
         * Chapters have to start with an IDR frame so request that this frame be
         * coded as IDR. Since there may be up to 16 frames currently buffered in
         * the encoder, remember the timestamp so when this frame finally pops out
         * of the encoder we'll mark its buffer as the start of a chapter.
         */
        pic_in.sliceType = X265_TYPE_IDR;
    }
    else
    {
        pic_in.sliceType = X265_TYPE_AUTO;
    }

    if (pv->last_stop != in->s.start)
    {
        hb_log("encx265 input continuity err: last stop %"PRId64"  start %"PRId64,
               pv->last_stop, in->s.start);
    }
    pv->last_stop = in->s.stop;
    save_frame_info(pv, in);

    if (pv->api->encoder_encode(pv->x265, &nal, &nnal, &pic_in, &pic_out) > 0)
    {
        return nal_encode(w, &pic_out, nal, nnal);
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
        x265_picture pic_out;
        hb_buffer_list_t list;

        hb_buffer_list_clear(&list);

        // flush delayed frames
        while (pv->api->encoder_encode(pv->x265, &nal,
                                       &nnal, NULL, &pic_out) > 0)
        {
            hb_buffer_t *buf = nal_encode(w, &pic_out, nal, nnal);
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
