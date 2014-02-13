/* encx265.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
#ifdef USE_X265

#include "hb.h"
#include "hb_dict.h"
#include "x265.h"

int  encx265Init (hb_work_object_t*, hb_job_t*);
int  encx265Work (hb_work_object_t*, hb_buffer_t**, hb_buffer_t**);
void encx265Close(hb_work_object_t*);
void writeNALs   (hb_work_private_t*, const x265_nal*, int);

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
    hb_job_t     *job;
    x265_encoder *x265;
    x265_param   *param;
    x265_picture  pic_in; // TODO: use x265_picture_alloc
    x265_nal     *p_nal;
    uint32_t      nal_count;

    int64_t  last_stop;
    uint32_t frames_in;
    uint32_t frames_out;

    hb_list_t *delayed_chapters;
    int64_t next_chapter_pts;

    struct
    {
        int64_t duration;
    }
    frame_info[FRAME_INFO_SIZE];

    FILE *fout;

    char csvfn[1024];
};

// used in delayed_chapters list
struct chapter_s
{
    int     index;
    int64_t start;
};

/***********************************************************************
 * hb_work_encx265_init
 ***********************************************************************
 *
 **********************************************************************/
int encx265Init(hb_work_object_t *w, hb_job_t *job)
{
    hb_work_private_t *pv = calloc(1, sizeof(hb_work_private_t));
    pv->next_chapter_pts  = AV_NOPTS_VALUE;
    pv->delayed_chapters  = hb_list_init();
    pv->job               = job;
    w->private_data       = pv;

    pv->fout = fopen(job->file, "wb");
    fseek(pv->fout, 0, SEEK_SET);

    x265_param *param = pv->param = x265_param_alloc();

    if (x265_param_default_preset(param,
                                  job->encoder_preset, job->encoder_tune) < 0)
    {
        free(pv);
        pv = NULL;
        return 1;
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
    param->frameRate   = (int)((double)job->vrate / (double)job->vrate_base + 0.5); // yes, this is an int
    param->keyframeMax = param->frameRate * 10;
    param->keyframeMin = param->frameRate;

    /* iterate through x265_opts and parse the options */
    hb_dict_entry_t *entry = NULL;
    hb_dict_t *x265_opts = hb_encopts_to_dict(job->encoder_options, job->vcodec);
    while ((entry = hb_dict_next(x265_opts, entry)) != NULL)
    {
        // here's where the strings are passed to libx265 for parsing
        int ret = x265_param_parse(param, entry->key, entry->value);
        // let x265 sanity check the options for us
        switch (ret)
        {
            case X265_PARAM_BAD_NAME:
                hb_log("encx265: unknown option '%s'", entry->key);
                break;
            case X265_PARAM_BAD_VALUE:
                hb_log("encx265: bad argument '%s=%s'", entry->key,
                       entry->value ? entry->value : "(null)");
                break;
            default:
                break;
        }
    }
    hb_dict_free(&x265_opts);

    /*
     * Settings which can't be overriden in the encodeer_options string
     * (muxer-specific settings, resolution, ratecontrol, etc.).
     */
    param->sourceWidth  = job->width;
    param->sourceHeight = job->height;

    if (job->vquality > 0)
    {
        param->rc.rateControlMode = X265_RC_CRF;
        param->rc.rfConstant      = job->vquality;
    }
    else
    {
        param->rc.rateControlMode = X265_RC_ABR;
        param->rc.bitrate         = job->vbitrate;
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
    if (x265_param_apply_profile(param, job->encoder_profile) < 0)
    {
        free(pv);
        pv = NULL;
        return 1;
    }

    /* we should now know whether B-frames are enabled */
    job->areBframes = (param->bframes > 0) + (param->bframes   > 0 &&
                                              param->bBPyramid > 0);

    pv->x265 = x265_encoder_open(param);
    if (pv->x265 == NULL)
    {
        hb_error("encx265: x265_encoder_open failed.");
        free(pv);
        pv = NULL;
        return 1;
    }
    if (!x265_encoder_headers(pv->x265, &pv->p_nal, &pv->nal_count))
    {
        writeNALs(pv, pv->p_nal, pv->nal_count);
    }
    x265_picture_init(param, &pv->pic_in);
    return 0;
}

void encx265Close(hb_work_object_t *w)
{
    hb_work_private_t *pv = w->private_data;

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

    x265_param_free(pv->param);
    x265_encoder_close(pv->x265);
    fclose(pv->fout);
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

void writeNALs(hb_work_private_t * pv, const x265_nal* nal, int nalcount)
{
    int i;
    for (i = 0; i < nalcount; i++)
    {
        fwrite((const char*)nal->payload, 1, nal->sizeBytes,  pv->fout);
        nal++;
    }
}

static hb_buffer_t *x265_encode(hb_work_object_t *w, hb_buffer_t *in)
{
    hb_work_private_t *pv = w->private_data;
    hb_job_t *job         = pv->job;
    x265_picture pic_out;

    pv->pic_in.stride[0] = in->plane[0].stride;
    pv->pic_in.stride[1] = in->plane[1].stride;
    pv->pic_in.stride[2] = in->plane[2].stride;
    pv->pic_in.planes[0] = in->plane[0].data;
    pv->pic_in.planes[1] = in->plane[1].data;
    pv->pic_in.planes[2] = in->plane[2].data;
    pv->pic_in.poc       = pv->frames_in;
    pv->pic_in.pts       = in->s.start;
    pv->pic_in.bitDepth  = 8;

    if (in->s.new_chap && job->chapter_markers)
    {
        /*
         * Chapters have to start with an IDR frame so request that this frame be
         * coded as IDR. Since there may be up to 16 frames currently buffered in
         * the encoder, remember the timestamp so when this frame finally pops out
         * of the encoder we'll mark its buffer as the start of a chapter.
         */
        pv->pic_in.sliceType = X265_TYPE_IDR;
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
    }
    else
    {
        pv->pic_in.sliceType = X265_TYPE_AUTO;
    }

    if (pv->last_stop != in->s.start)
    {
        hb_log("encx265 input continuity err: last stop %"PRId64"  start %"PRId64,
               pv->last_stop, in->s.start);
    }
    pv->last_stop = in->s.stop;
    save_frame_info(pv, in);

    x265_encoder_encode(pv->x265, &pv->p_nal, &pv->nal_count, &pv->pic_in, &pic_out);
    if (pv->nal_count > 0)
    {
        writeNALs(pv, pv->p_nal, pv->nal_count);
    }

    if (pv->next_chapter_pts != AV_NOPTS_VALUE &&
        pv->next_chapter_pts <= pic_out.pts)
    {
        // we're no longer looking for this chapter
        pv->next_chapter_pts = AV_NOPTS_VALUE;

        // get the chapter index from the list
        struct chapter_s *item = hb_list_item(pv->delayed_chapters, 0);
        if (item != NULL)
        {
            // we're done with this chapter
            hb_list_rem(pv->delayed_chapters, item);
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

    return NULL;
}

int encx265Work(hb_work_object_t *w, hb_buffer_t **buf_in, hb_buffer_t **buf_out)
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *in       = *buf_in;

    *buf_out = NULL;
    if (in->size <= 0)
    {
        x265_picture pic_out;
        uint32_t i_nal;
        x265_nal *nal;
        hb_buffer_t *last_buf = NULL;
        while (1)
        {
            x265_encoder_encode(pv->x265, &nal, &i_nal, NULL, &pic_out);
            if (i_nal <= 0)
                break;
            writeNALs(pv, nal, i_nal);
        }
        // Flushed everything - add the eof to the end of the chain.
        if (last_buf == NULL)
            *buf_out = in;
        else
            last_buf->next = in;


        *buf_in = NULL;

        return HB_WORK_DONE;
    }

    ++pv->frames_in;
    ++pv->frames_out;
    *buf_out = x265_encode(w, in);
    return HB_WORK_OK;
}

const char * hb_x265_encopt_name(const char *name)
{
    int i;
    for (i = 0; hb_x265_encopt_synonyms[i][0] != NULL; i++)
        if (!strcmp(name, hb_x265_encopt_synonyms[i][1]))
            return hb_x265_encopt_synonyms[i][0];
    return name;
}

#endif
