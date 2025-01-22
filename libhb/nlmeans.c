/* nlmeans.c

   Copyright (c) 2013 Dirk Farin
   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

/* Usage
 *
 * Parameters:
 *     lumaY_strength   : lumaY_origin_tune   : lumaY_patch_size   : lumaY_range   : lumaY_frames   : lumaY_prefilter   :
 *     chromaB_strength : chromaB_origin_tune : chromaB_patch_size : chromaB_range : chromaB_frames : chromaB_prefilter :
 *     chromaR_strength : chromaR_origin_tune : chromaR_patch_size : chromaR_range : chromaR_frames : chromaR_prefilter
 *
 * Defaults:
 *     8:1:7:3:2:0 for each channel (equivalent to 8:1:7:3:2:0:8:1:7:3:2:0:8:1:7:3:2:0)
 *
 * Parameters cascade, e.g. 6:0.8:7:3:3:0:4:1 sets:
 *     strength 6, origin tune 0.8 for luma
 *     patch size 7, range 3, frames 3, prefilter 0 for all channels
 *     strength 4, origin tune 1 for both chroma channels
 *
 * Strength is relative and must be adjusted; ALL parameters affect overall strength.
 * Lower origin tune improves results for noisier input or animation (film 0.5-1, animation 0.15-0.5).
 * Large patch size (>9) may greatly reduce quality by clobbering detail.
 * Larger search range increases quality; however, computation time increases exponentially.
 * Large number of frames (film >3, animation >6) may cause temporal smearing.
 * Prefiltering can potentially improve weight decisions, yielding better results for difficult sources.
 *
 * Prefilter enum combos:
 *     1: Mean 3x3
 *     2: Mean 5x5
 *     3: Mean 5x5 (overrides Mean 3x3)
 *   257: Mean 3x3 reduced by 25%
 *   258: Mean 5x5 reduced by 25%
 *   513: Mean 3x3 reduced by 50%
 *   514: Mean 5x5 reduced by 50%
 *   769: Mean 3x3 reduced by 75%
 *   770: Mean 5x5 reduced by 75%
 *  1025: Mean 3x3 plus edge boost (restores lost edge detail)
 *  1026: Mean 5x5 plus edge boost
 *  1281: Mean 3x3 reduced by 25% plus edge boost
 *        etc...
 *  2049: Mean 3x3 passthru (NLMeans off, prefilter is the output)
 *        etc...
 *  3329: Mean 3x3 reduced by 25% plus edge boost, passthru
 *        etc...
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/taskset.h"
#include "handbrake/nlmeans.h"

#define NLMEANS_STRENGTH_LUMA_DEFAULT      6
#define NLMEANS_STRENGTH_CHROMA_DEFAULT    6
#define NLMEANS_ORIGIN_TUNE_LUMA_DEFAULT   1
#define NLMEANS_ORIGIN_TUNE_CHROMA_DEFAULT 1
#define NLMEANS_PATCH_SIZE_LUMA_DEFAULT    7
#define NLMEANS_PATCH_SIZE_CHROMA_DEFAULT  7
#define NLMEANS_RANGE_LUMA_DEFAULT         3
#define NLMEANS_RANGE_CHROMA_DEFAULT       3
#define NLMEANS_FRAMES_LUMA_DEFAULT        2
#define NLMEANS_FRAMES_CHROMA_DEFAULT      2
#define NLMEANS_PREFILTER_LUMA_DEFAULT     0
#define NLMEANS_PREFILTER_CHROMA_DEFAULT   0

#define NLMEANS_PREFILTER_MODE_MEAN3X3       1
#define NLMEANS_PREFILTER_MODE_MEAN5X5       2
#define NLMEANS_PREFILTER_MODE_MEDIAN3X3     4
#define NLMEANS_PREFILTER_MODE_MEDIAN5X5     8
#define NLMEANS_PREFILTER_MODE_CSM3X3       16
#define NLMEANS_PREFILTER_MODE_CSM5X5       32
#define NLMEANS_PREFILTER_MODE_RESERVED64   64 // Reserved
#define NLMEANS_PREFILTER_MODE_RESERVED128 128 // Reserved
#define NLMEANS_PREFILTER_MODE_REDUCE25    256
#define NLMEANS_PREFILTER_MODE_REDUCE50    512
#define NLMEANS_PREFILTER_MODE_EDGEBOOST  1024
#define NLMEANS_PREFILTER_MODE_PASSTHRU   2048

#define NLMEANS_SORT(a,b) { if (a > b) NLMEANS_SWAP(a, b); }
#define NLMEANS_SWAP(a,b) { a = (a ^ b); b = (a ^ b); a = (b ^ a); }

#define NLMEANS_FRAMES_MAX  32
#define NLMEANS_EXPSIZE     128

typedef struct
{
    void *mem;
    void *mem_pre;
    void *image;
    void *image_pre;
    int w;
    int h;
    int border;
    hb_lock_t *mutex;
    int prefiltered;
} BorderedPlane;

typedef struct
{
    int width;
    int height;
    int fmt;
    BorderedPlane plane[3];
    hb_buffer_t *buf;        // input buf sidedata
} Frame;

struct PixelSum
{
    float weight_sum;
    float pixel_sum;
};

typedef struct
{
    taskset_thread_arg_t arg;
    hb_filter_private_t *pv;
    hb_buffer_t *out;
} nlmeans_thread_arg_t;

struct hb_filter_private_s
{
    int depth;
    int bps;
    int max_value;

    double strength[3];    // averaging weight decay, larger produces smoother output
    double origin_tune[3]; // weight tuning for origin patch, 0.00..1.00
    int    patch_size[3];  // pixel context region width  (must be odd)
    int    range[3];       // spatial search window width (must be odd)
    int    nframes[3];     // temporal search depth in frames
    int    prefilter[3];   // prefilter mode, can improve weight analysis
    int    threads;        // number of frame threads to use, 0 == auto

    float  exptable[3][NLMEANS_EXPSIZE];
    float  weight_fact_table[3];
    int    diff_max[3];

    NLMeansFunctions functions;

    void (*nlmeans_alloc)(const void *src,
                          const int src_w,
                          const int src_s,
                          const int src_h,
                          BorderedPlane *dst,
                          const int border);
    void (*nlmeans_prefilter)(BorderedPlane *src, const int filter_type);
    void (*nlmeans_deborder)(const BorderedPlane *src, void *in_dst,
                                const int w, const int s, const int h);
    void (*nlmeans_plane)(NLMeansFunctions *functions,
                                    Frame *frame,
                                    int prefilter,
                                    int plane,
                                    int nframes,
                                    void *dst,
                                    int dst_w,
                                    int dst_s,
                                    int dst_h,
                                    double h_param,
                                    double origin_tune,
                                    int n,
                                    int r,
                              const float *exptable,
                              const float  weight_fact_table,
                              const int    diff_max);

    Frame      *frame;
    int         next_frame;
    int         max_frames;

    taskset_t   taskset;
    nlmeans_thread_arg_t ** thread_data;

    hb_filter_init_t        input;
    hb_filter_init_t        output;
};

static int nlmeans_init(hb_filter_object_t *filter, hb_filter_init_t *init);
static int nlmeans_work(hb_filter_object_t *filter,
                           hb_buffer_t **buf_in,
                           hb_buffer_t **buf_out);
static void nlmeans_close(hb_filter_object_t *filter);

static void nlmeans_filter_work(void *thread_args_v);

static const char nlmeans_template[] =
    "y-strength=^"HB_FLOAT_REG"$:y-origin-tune=^"HB_FLOAT_REG"$:"
    "y-patch-size=^"HB_INT_REG"$:y-range=^"HB_INT_REG"$:"
    "y-frame-count=^"HB_INT_REG"$:y-prefilter=^"HB_INT_REG"$:"
    "cb-strength=^"HB_FLOAT_REG"$:cb-origin-tune=^"HB_FLOAT_REG"$:"
    "cb-patch-size=^"HB_INT_REG"$:cb-range=^"HB_INT_REG"$:"
    "cb-frame-count=^"HB_INT_REG"$:cb-prefilter=^"HB_INT_REG"$:"
    "cr-strength=^"HB_FLOAT_REG"$:cr-origin-tune=^"HB_FLOAT_REG"$:"
    "cr-patch-size=^"HB_INT_REG"$:cr-range=^"HB_INT_REG"$:"
    "cr-frame-count=^"HB_INT_REG"$:cr-prefilter=^"HB_INT_REG"$:"
    "threads=^"HB_INT_REG"$";

hb_filter_object_t hb_filter_nlmeans =
{
    .id                = HB_FILTER_NLMEANS,
    .enforce_order     = 1,
    .name              = "Denoise (nlmeans)",
    .settings          = NULL,
    .init              = nlmeans_init,
    .work              = nlmeans_work,
    .close             = nlmeans_close,
    .settings_template = nlmeans_template,
};

#define BIT_DEPTH 8
#include "templates/nlmeans_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 16
#include "templates/nlmeans_template.c"
#undef BIT_DEPTH

static int nlmeans_init(hb_filter_object_t *filter,
                           hb_filter_init_t *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("nlmeans: calloc failed");
        return -1;
    }
    hb_filter_private_t *pv = filter->private_data;
    NLMeansFunctions *functions = &pv->functions;

    pv->input = *init;

    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(init->pix_fmt);
    pv->depth      = desc->comp[0].depth;
    pv->bps        = pv->depth > 8 ? 2 : 1;
    pv->max_value  = (1 << pv->depth) - 1;

    switch (pv->depth)
    {
        case 8:
            functions->build_integral = build_integral_scalar_8;
            pv->nlmeans_alloc         = nlmeans_alloc_8;
            pv->nlmeans_prefilter     = nlmeans_prefilter_8;
            pv->nlmeans_deborder      = nlmeans_deborder_8;
            pv->nlmeans_plane         = nlmeans_plane_8;
        #if defined(ARCH_X86)
            nlmeans_init_x86(functions);
        #endif
            break;

        case 16:
        default:
            functions->build_integral = build_integral_scalar_16;
            pv->nlmeans_alloc         = nlmeans_alloc_16;
            pv->nlmeans_prefilter     = nlmeans_prefilter_16;
            pv->nlmeans_deborder      = nlmeans_deborder_16;
            pv->nlmeans_plane         = nlmeans_plane_16;
            break;
    }


    // Mark parameters unset
    for (int c = 0; c < 3; c++)
    {
        pv->strength[c]    = -1;
        pv->origin_tune[c] = -1;
        pv->patch_size[c]  = -1;
        pv->range[c]       = -1;
        pv->nframes[c]     = -1;
        pv->prefilter[c]   = -1;
    }
    pv->threads = -1;

    // Read user parameters
    if (filter->settings != NULL)
    {
        hb_dict_t * dict = filter->settings;
        hb_dict_extract_double(&pv->strength[0],    dict, "y-strength");
        hb_dict_extract_double(&pv->origin_tune[0], dict, "y-origin-tune");
        hb_dict_extract_int(&pv->patch_size[0],     dict, "y-patch-size");
        hb_dict_extract_int(&pv->range[0],          dict, "y-range");
        hb_dict_extract_int(&pv->nframes[0],        dict, "y-frame-count");
        hb_dict_extract_int(&pv->prefilter[0],      dict, "y-prefilter");

        hb_dict_extract_double(&pv->strength[1],    dict, "cb-strength");
        hb_dict_extract_double(&pv->origin_tune[1], dict, "cb-origin-tune");
        hb_dict_extract_int(&pv->patch_size[1],     dict, "cb-patch-size");
        hb_dict_extract_int(&pv->range[1],          dict, "cb-range");
        hb_dict_extract_int(&pv->nframes[1],        dict, "cb-frame-count");
        hb_dict_extract_int(&pv->prefilter[1],      dict, "cb-prefilter");

        hb_dict_extract_double(&pv->strength[2],    dict, "cr-strength");
        hb_dict_extract_double(&pv->origin_tune[2], dict, "cr-origin-tune");
        hb_dict_extract_int(&pv->patch_size[2],     dict, "cr-patch-size");
        hb_dict_extract_int(&pv->range[2],          dict, "cr-range");
        hb_dict_extract_int(&pv->nframes[2],        dict, "cr-frame-count");
        hb_dict_extract_int(&pv->prefilter[2],      dict, "cr-prefilter");

        hb_dict_extract_int(&pv->threads,           dict, "threads");
    }

    // Cascade values
    // Cr not set; inherit Cb. Cb not set; inherit Y. Y not set; defaults.
    for (int c = 1; c < 3; c++)
    {
        if (pv->strength[c]    == -1) { pv->strength[c]    = pv->strength[c-1]; }
        if (pv->origin_tune[c] == -1) { pv->origin_tune[c] = pv->origin_tune[c-1]; }
        if (pv->patch_size[c]  == -1) { pv->patch_size[c]  = pv->patch_size[c-1]; }
        if (pv->range[c]       == -1) { pv->range[c]       = pv->range[c-1]; }
        if (pv->nframes[c]     == -1) { pv->nframes[c]     = pv->nframes[c-1]; }
        if (pv->prefilter[c]   == -1) { pv->prefilter[c]   = pv->prefilter[c-1]; }
    }

    for (int c = 0; c < 3; c++)
    {
        // Replace unset values with defaults
        if (pv->strength[c]    == -1) { pv->strength[c]    = c ? NLMEANS_STRENGTH_LUMA_DEFAULT    : NLMEANS_STRENGTH_CHROMA_DEFAULT; }
        if (pv->origin_tune[c] == -1) { pv->origin_tune[c] = c ? NLMEANS_ORIGIN_TUNE_LUMA_DEFAULT : NLMEANS_ORIGIN_TUNE_CHROMA_DEFAULT; }
        if (pv->patch_size[c]  == -1) { pv->patch_size[c]  = c ? NLMEANS_PATCH_SIZE_LUMA_DEFAULT  : NLMEANS_PATCH_SIZE_CHROMA_DEFAULT; }
        if (pv->range[c]       == -1) { pv->range[c]       = c ? NLMEANS_RANGE_LUMA_DEFAULT       : NLMEANS_RANGE_CHROMA_DEFAULT; }
        if (pv->nframes[c]     == -1) { pv->nframes[c]     = c ? NLMEANS_FRAMES_LUMA_DEFAULT      : NLMEANS_FRAMES_CHROMA_DEFAULT; }
        if (pv->prefilter[c]   == -1) { pv->prefilter[c]   = c ? NLMEANS_PREFILTER_LUMA_DEFAULT   : NLMEANS_PREFILTER_CHROMA_DEFAULT; }

        // Sanitize
        if (pv->strength[c] < 0)        { pv->strength[c] = 0; }
        if (pv->origin_tune[c] < 0.01)  { pv->origin_tune[c] = 0.01; } // avoid black artifacts
        if (pv->origin_tune[c] > 1)     { pv->origin_tune[c] = 1; }
        if (pv->patch_size[c] % 2 == 0) { pv->patch_size[c]--; }
        if (pv->patch_size[c] < 1)      { pv->patch_size[c] = 1; }
        if (pv->range[c] % 2 == 0)      { pv->range[c]--; }
        if (pv->range[c] < 1)           { pv->range[c] = 1; }
        if (pv->nframes[c] < 1)         { pv->nframes[c] = 1; }
        if (pv->nframes[c] > NLMEANS_FRAMES_MAX) { pv->nframes[c] = NLMEANS_FRAMES_MAX; }
        if (pv->prefilter[c] < 0)       { pv->prefilter[c] = 0; }

        if (pv->max_frames < pv->nframes[c]) pv->max_frames = pv->nframes[c];

        // Scale strength with bit depth
        pv->strength[c] *= pv->depth > 8 ? (pv->depth - 8) * (pv->depth - 8) : 1;

        // Precompute exponential table
        float *exptable = &pv->exptable[c][0];
        float *weight_fact_table = &pv->weight_fact_table[c];
        int   *diff_max = &pv->diff_max[c];
        const float weight_factor        = 1.0/pv->patch_size[c]/pv->patch_size[c] / (pv->strength[c] * pv->strength[c]);
        const float min_weight_in_table  = 0.0005;
        const float stretch              = NLMEANS_EXPSIZE / (-log(min_weight_in_table));
        *(weight_fact_table)             = weight_factor * stretch;
        *(diff_max)                      = NLMEANS_EXPSIZE / *(weight_fact_table);
        for (int i = 0; i < NLMEANS_EXPSIZE; i++)
        {
            exptable[i] = exp(-i/stretch);
        }
        exptable[NLMEANS_EXPSIZE-1] = 0;
    }

    // Threads
    if (pv->threads < 1) {
        pv->threads = hb_get_cpu_count();

        // Reduce internal thread count where we have many logical cores
        // Too many threads increases CPU cache pressure, reducing performance
        if (pv->threads >= 32) {
            pv->threads = pv->threads / 2;
        }
        else if (pv->threads >= 16) {
            pv->threads = (pv->threads / 4) * 3;
        }
    }
    hb_log("NLMeans using %i threads", pv->threads);

    pv->frame = calloc(pv->threads + pv->max_frames, sizeof(Frame));
    if (pv->frame == NULL)
    {
        hb_error("nlmeans: calloc failed");
        goto fail;
    }
    for (int ii = 0; ii < pv->threads + pv->max_frames; ii++)
    {
        for (int c = 0; c < 3; c++)
        {
            pv->frame[ii].plane[c].mutex = hb_lock_init();
        }
    }

    pv->thread_data = malloc(pv->threads * sizeof(nlmeans_thread_arg_t*));
    if (taskset_init(&pv->taskset, "nlmeans_filter", pv->threads,
                     sizeof(nlmeans_thread_arg_t), nlmeans_filter_work) == 0)
    {
        hb_error("NLMeans could not initialize taskset");
        goto fail;
    }

    for (int ii = 0; ii < pv->threads; ii++)
    {
        pv->thread_data[ii] = taskset_thread_args(&pv->taskset, ii);
        if (pv->thread_data[ii] == NULL)
        {
            hb_error("NLMeans could not create thread args");
            goto fail;
        }
        pv->thread_data[ii]->pv = pv;
        pv->thread_data[ii]->arg.taskset = &pv->taskset;
        pv->thread_data[ii]->arg.segment = ii;
    }
    pv->output = *init;

    return 0;

fail:
    taskset_fini(&pv->taskset);
    free(pv->thread_data);
    free(pv);
    return -1;
}

static void nlmeans_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    taskset_fini(&pv->taskset);
    for (int c = 0; c < 3; c++)
    {
        for (int f = 0; f < pv->nframes[c]; f++)
        {
            if (pv->frame[f].plane[c].mem_pre != NULL &&
                pv->frame[f].plane[c].mem_pre != pv->frame[f].plane[c].mem)
            {
                free(pv->frame[f].plane[c].mem_pre);
                pv->frame[f].plane[c].mem_pre = NULL;
            }
            if (pv->frame[f].plane[c].mem != NULL)
            {
                free(pv->frame[f].plane[c].mem);
                pv->frame[f].plane[c].mem = NULL;
            }
            hb_buffer_close(&pv->frame[f].buf);
        }
    }

    for (int ii = 0; ii < pv->threads + pv->max_frames; ii++)
    {
        for (int c = 0; c < 3; c++)
        {
            hb_lock_close(&pv->frame[ii].plane[c].mutex);
        }
    }

    free(pv->frame);
    free(pv->thread_data);
    free(pv);
    filter->private_data = NULL;
}

static void nlmeans_filter_work(void *thread_args_v)
{
    nlmeans_thread_arg_t *thread_data = thread_args_v;
    hb_filter_private_t *pv = thread_data->pv;
    int segment = thread_data->arg.segment;
    Frame *frame = &pv->frame[segment];

    hb_buffer_t *buf;
    buf = hb_frame_buffer_init(pv->output.pix_fmt,
                               frame->width, frame->height);
    buf->f.color_prim      = pv->output.color_prim;
    buf->f.color_transfer  = pv->output.color_transfer;
    buf->f.color_matrix    = pv->output.color_matrix;
    buf->f.color_range     = pv->output.color_range ;
    buf->f.chroma_location = pv->output.chroma_location;


    NLMeansFunctions *functions = &pv->functions;

    for (int c = 0; c < 3; c++)
    {
        if (pv->prefilter[c] & NLMEANS_PREFILTER_MODE_PASSTHRU)
        {
            pv->nlmeans_prefilter(&frame->plane[c], pv->prefilter[c]);
            pv->nlmeans_deborder(&frame->plane[c], buf->plane[c].data,
                                 buf->plane[c].width, buf->plane[c].stride / pv->bps,
                                 buf->plane[c].height);
            continue;
        }
        if (pv->strength[c] == 0)
        {
            pv->nlmeans_deborder(&frame->plane[c], buf->plane[c].data,
                                 buf->plane[c].width, buf->plane[c].stride / pv->bps,
                                 buf->plane[c].height);
            continue;
        }

        // Process current plane
        pv->nlmeans_plane(functions,
                          frame,
                          pv->prefilter[c],
                          c,
                          pv->nframes[c],
                          buf->plane[c].data,
                          buf->plane[c].width,
                          buf->plane[c].stride / pv->bps,
                          buf->plane[c].height,
                          pv->strength[c],
                          pv->origin_tune[c],
                          pv->patch_size[c],
                          pv->range[c],
                          pv->exptable[c],
                          pv->weight_fact_table[c],
                          pv->diff_max[c]);
    }
    hb_buffer_copy_props(buf, pv->frame[segment].buf);
    hb_buffer_close(&pv->frame[segment].buf);
    thread_data->out = buf;
}

static void nlmeans_add_frame(hb_filter_private_t *pv, hb_buffer_t *buf)
{
    for (int c = 0; c < 3; c++)
    {
        // Extend copy of plane with extra border and place in buffer
        const int border = ((pv->patch_size[c] + 2) / 2 + 15) / 16 * 16;
        pv->nlmeans_alloc(buf->plane[c].data,
                          buf->plane[c].width,
                          buf->plane[c].stride / pv->bps,
                          buf->plane[c].height,
                          &pv->frame[pv->next_frame].plane[c],
                          border);
    }
    pv->frame[pv->next_frame].width = buf->f.width;
    pv->frame[pv->next_frame].height = buf->f.height;
    pv->frame[pv->next_frame].fmt = buf->f.fmt;
    pv->frame[pv->next_frame].buf = hb_buffer_init(0);
    hb_buffer_copy_props(pv->frame[pv->next_frame].buf, buf);

    pv->next_frame++;
}

static hb_buffer_t * nlmeans_filter(hb_filter_private_t *pv)
{
    if (pv->next_frame < pv->max_frames + pv->threads)
    {
        return NULL;
    }

    taskset_cycle(&pv->taskset);

    // Free buffers that are not needed for next taskset cycle
    for (int c = 0; c < 3; c++)
    {
        for (int t = 0; t < pv->threads; t++)
        {
            // Release last frame in buffer
            if (pv->frame[t].plane[c].mem_pre != NULL &&
                pv->frame[t].plane[c].mem_pre != pv->frame[t].plane[c].mem)
            {
                free(pv->frame[t].plane[c].mem_pre);
                pv->frame[t].plane[c].mem_pre = NULL;
            }
            if (pv->frame[t].plane[c].mem != NULL)
            {
                free(pv->frame[t].plane[c].mem);
                pv->frame[t].plane[c].mem = NULL;
            }
        }
    }
    // Shift frames in buffer down
    for (int f = 0; f < pv->max_frames; f++)
    {
        // Don't move the mutex!
        Frame frame = pv->frame[f];
        pv->frame[f] = pv->frame[f+pv->threads];
        for (int c = 0; c < 3; c++)
        {
            pv->frame[f].plane[c].mutex = frame.plane[c].mutex;
            pv->frame[f+pv->threads].plane[c].mem_pre = NULL;
            pv->frame[f+pv->threads].plane[c].mem = NULL;
        }
    }
    pv->next_frame -= pv->threads;

    // Collect results from taskset
    hb_buffer_list_t list;
    hb_buffer_list_clear(&list);
    for (int t = 0; t < pv->threads; t++)
    {
        hb_buffer_list_append(&list, pv->thread_data[t]->out);
    }
    return hb_buffer_list_clear(&list);
}

static hb_buffer_t * nlmeans_filter_flush(hb_filter_private_t *pv)
{
    hb_buffer_list_t list;

    hb_buffer_list_clear(&list);
    for (int f = 0; f < pv->next_frame; f++)
    {
        Frame *frame = &pv->frame[f];
        hb_buffer_t *buf;
        buf = hb_frame_buffer_init(pv->output.pix_fmt,
                                   frame->width, frame->height);
        buf->f.color_prim      = pv->output.color_prim;
        buf->f.color_transfer  = pv->output.color_transfer;
        buf->f.color_matrix    = pv->output.color_matrix;
        buf->f.color_range     = pv->output.color_range;
        buf->f.chroma_location = pv->output.chroma_location;

        NLMeansFunctions *functions = &pv->functions;

        for (int c = 0; c < 3; c++)
        {
            if (pv->prefilter[c] & NLMEANS_PREFILTER_MODE_PASSTHRU)
            {
                pv->nlmeans_prefilter(&frame->plane[c], pv->prefilter[c]);
                pv->nlmeans_deborder(&frame->plane[c], buf->plane[c].data,
                                       buf->plane[c].width, buf->plane[c].stride / pv->bps,
                                       buf->plane[c].height);
                continue;
            }
            if (pv->strength[c] == 0)
            {
                pv->nlmeans_deborder(&frame->plane[c], buf->plane[c].data,
                                     buf->plane[c].width, buf->plane[c].stride / pv->bps,
                                     buf->plane[c].height);
                continue;
            }

            int nframes = pv->next_frame - f;
            if (pv->nframes[c] < nframes)
            {
                nframes = pv->nframes[c];
            }
            // Process current plane
            pv->nlmeans_plane(functions,
                              frame,
                              pv->prefilter[c],
                              c,
                              nframes,
                              buf->plane[c].data,
                              buf->plane[c].width,
                              buf->plane[c].stride / pv->bps,
                              buf->plane[c].height,
                              pv->strength[c],
                              pv->origin_tune[c],
                              pv->patch_size[c],
                              pv->range[c],
                              pv->exptable[c],
                              pv->weight_fact_table[c],
                              pv->diff_max[c]);
        }
        hb_buffer_copy_props(buf, frame->buf);
        hb_buffer_close(&frame->buf);
        hb_buffer_list_append(&list, buf);
    }
    return hb_buffer_list_clear(&list);
}

static int nlmeans_work(hb_filter_object_t *filter,
                           hb_buffer_t **buf_in,
                           hb_buffer_t **buf_out )
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        hb_buffer_list_t list;
        hb_buffer_t *buf;

        // Flush buffered frames
        buf = nlmeans_filter_flush(pv);
        hb_buffer_list_set(&list, buf);

        // And terminate the buffer list with a EOF buffer
        hb_buffer_list_append(&list, in);
        *buf_out = hb_buffer_list_clear(&list);

        *buf_in  = NULL;
        return HB_FILTER_DONE;
    }

    nlmeans_add_frame(pv, in);
    *buf_out = nlmeans_filter(pv);

    return HB_FILTER_OK;
}
