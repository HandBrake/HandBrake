/* decomb.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   The yadif algorithm was created by Michael Niedermayer.
   Tritical's work inspired much of the comb detection code:
   http://web.missouri.edu/~kes25c/
*/

/*
Parameters:
    Mode:
        1 = yadif
        2 = blend
        4 = cubic interpolation
        8 = EEDI2 interpolation
       16 = Deinterlace each field to a separate frame
       32 = Selectively deinterlace based on comb detection

Appended for EEDI2:
    Magnitude thresh : Variance thresh : Laplacian thresh : Dilation thresh :
    Erosion thresh : Noise thresh : Max search distance : Post-processing

Plus:
    Parity

Defaults:
    7:10:20:20:4:2:50:24:1:-1

These modes can be layered. For example, Yadif (1) + EEDI2 (8) = 9,
which will feed EEDI2 interpolations to yadif.

** Working combos:
 1: Just yadif
 2: Just blend
 3: Switch between yadif and blend
 4: Just cubic interpolate
 5: Cubic->yadif
 6: Switch between cubic and blend
 7: Switch between cubic->yadif and blend
 8: Just EEDI2 interpolate
 9: EEDI2->yadif
10: Switch between EEDI2 and blend
11: Switch between EEDI2->yadif and blend
...okay I'm getting bored now listing all these different modes

12-15: EEDI2 will override cubic interpolation
*/

#include "handbrake/handbrake.h"
#include "handbrake/eedi2.h"
#include "handbrake/taskset.h"
#include "handbrake/decomb.h"

#define PARITY_DEFAULT   -1

#define MIN3(a,b,c) MIN(MIN(a,b),c)
#define MAX3(a,b,c) MAX(MAX(a,b),c)

// Some names to correspond to the pv->eedi_half array's contents
#define SRCPF 0
#define MSKPF 1
#define TMPPF 2
#define DSTPF 3
// Some names to correspond to the pv->eedi_full array's contents
#define DST2PF 0
#define TMP2PF2 1
#define MSK2PF 2
#define TMP2PF 3
#define DST2MPF 4

typedef struct yadif_arguments_s
{
    hb_buffer_t *dst;
    int parity;
    int tff;
    int mode;
} yadif_arguments_t;

typedef struct eedi2_thread_arg_s
{
    taskset_thread_arg_t arg;
    hb_filter_private_t *pv;
    int plane;
} eedi2_thread_arg_t;

typedef struct yadif_thread_arg_s
{
    taskset_thread_arg_t arg;
    hb_filter_private_t *pv;
    int segment_start[3];
    int segment_height[3];
} yadif_thread_arg_t;

struct hb_filter_private_s
{
    int                 depth;
    int                 bps;
    int                 max_value;

    // Decomb parameters
    int                 mode;

    // EEDI2 parameters
    int                 magnitude_threshold;
    int                 variance_threshold;
    int                 laplacian_threshold;
    int                 dilation_threshold;
    int                 erosion_threshold;
    int                 noise_threshold;
    int                 maximum_search_distance;
    int                 post_processing;

    // Deinterlace parameters
    int                 parity;
    int                 tff;

    int                 ready;

    int                 deinterlaced;
    int                 blended;
    int                 unfiltered;
    int                 frames;

    hb_buffer_t        *ref[3];

    const void         *eedi_limlut;
    hb_buffer_t        *eedi_half[4];
    hb_buffer_t        *eedi_full[5];
    int                *cx2;
    int                *cy2;
    int                *cxy;
    int                *tmpc;

    const void         *crop_table;
    int                 cpu_count;
    int                 segment_height[3];

    // Functions
    void      (*filter)(hb_filter_private_t *pv,
                        hb_buffer_t *dst,
                        int parity,
                        int tff);

    taskset_t           yadif_taskset;     // Threads for Yadif - one per CPU
    yadif_arguments_t  *yadif_arguments;   // Arguments to thread for work

    taskset_t           eedi2_taskset;     // Threads for eedi2 - one per plane

    hb_buffer_list_t    out_list;

    hb_filter_init_t    input;
    hb_filter_init_t    output;
};

typedef struct
{
    int tap[5];
    int normalize;
} filter_param_t;

static int hb_decomb_init(hb_filter_object_t *filter,
                          hb_filter_init_t *init);

static int hb_decomb_work(hb_filter_object_t *filter,
                           hb_buffer_t **buf_in,
                           hb_buffer_t **buf_out);

static void hb_decomb_close(hb_filter_object_t *filter);

static const char decomb_template[] =
    "mode=^"HB_INT_REG"$:"
    "magnitude-thresh=^"HB_INT_REG"$:variance-thresh=^"HB_INT_REG"$:"
    "laplacian-thresh=^"HB_INT_REG"$:dilation-thresh=^"HB_INT_REG"$:"
    "erosion-thresh=^"HB_INT_REG"$:noise-thresh=^"HB_INT_REG"$:"
    "search-distance=^"HB_INT_REG"$:postproc=^([0-3])$:parity=^([01])$";

hb_filter_object_t hb_filter_decomb =
{
    .id                = HB_FILTER_DECOMB,
    .enforce_order     = 1,
    .name              = "Decomb",
    .settings          = NULL,
    .init              = hb_decomb_init,
    .work              = hb_decomb_work,
    .close             = hb_decomb_close,
    .settings_template = decomb_template,
};

static void store_ref(hb_filter_private_t *pv, hb_buffer_t *b)
{
    hb_buffer_close(&pv->ref[0]);
    memmove(&pv->ref[0], &pv->ref[1], sizeof(hb_buffer_t *) * 2);
    pv->ref[2] = b;
}

#define BIT_DEPTH 8
#include "templates/decomb_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 16
#include "templates/decomb_template.c"
#undef BIT_DEPTH

static int hb_decomb_init(hb_filter_object_t *filter,
                          hb_filter_init_t *init)
{
    filter->private_data = calloc(1, sizeof(struct hb_filter_private_s));
    if (filter->private_data == NULL)
    {
        hb_error("decomb: calloc failed");
        return -1;
    }
    hb_filter_private_t *pv = filter->private_data;
    pv->input                = *init;
    hb_buffer_list_clear(&pv->out_list);

    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(init->pix_fmt);
    pv->depth      = desc->comp[0].depth;
    pv->bps        = pv->depth > 8 ? 2 : 1;
    pv->max_value  = (1 << pv->depth) - 1;

    pv->deinterlaced = 0;
    pv->blended      = 0;
    pv->unfiltered   = 0;
    pv->frames       = 0;
    pv->ready        = 0;

    pv->mode                    = MODE_DECOMB_YADIF | MODE_DECOMB_BLEND | MODE_DECOMB_CUBIC;
    pv->magnitude_threshold     = 10;
    pv->variance_threshold      = 20;
    pv->laplacian_threshold     = 20;
    pv->dilation_threshold      = 4;
    pv->erosion_threshold       = 2;
    pv->noise_threshold         = 50;
    pv->maximum_search_distance = 24;
    pv->post_processing         = 1;
    pv->parity                  = PARITY_DEFAULT;

    if (filter->settings)
    {
        hb_value_t *dict = filter->settings;

        // Get comb detection settings
        hb_dict_extract_int(&pv->mode, dict, "mode");

        // Get deinterlace settings
        hb_dict_extract_int(&pv->parity, dict, "parity");
        if (pv->mode & MODE_DECOMB_EEDI2)
        {
            hb_dict_extract_int(&pv->magnitude_threshold, dict,
                                "magnitude-thresh");
            hb_dict_extract_int(&pv->variance_threshold, dict,
                                "variance-thresh");
            hb_dict_extract_int(&pv->laplacian_threshold, dict,
                                "laplacian-thresh");
            hb_dict_extract_int(&pv->dilation_threshold, dict,
                                "dilation-thresh");
            hb_dict_extract_int(&pv->erosion_threshold, dict,
                                "erosion-thresh");
            hb_dict_extract_int(&pv->noise_threshold, dict,
                                "noise-thresh");
            hb_dict_extract_int(&pv->maximum_search_distance, dict,
                                "search-distance");
            hb_dict_extract_int(&pv->post_processing, dict,
                                "postproc");
        }
    }

    pv->cpu_count = hb_get_cpu_count();

    // Make segment sizes an even number of lines
    int height = hb_image_height(init->pix_fmt, init->geometry.height, 0);
    // Each segment must begin on the even "parity" row.
    // I.e. each segment of each plane must begin on an even row.
    pv->segment_height[0] = (height / pv->cpu_count) & ~3;
    pv->segment_height[1] = hb_image_height(init->pix_fmt, pv->segment_height[0], 1);
    pv->segment_height[2] = hb_image_height(init->pix_fmt, pv->segment_height[0], 2);

    if (pv->mode & MODE_DECOMB_EEDI2)
    {
        // Allocate half-height eedi2 buffers
        for (int ii = 0; ii < 4; ii++)
        {
            pv->eedi_half[ii] = hb_frame_buffer_init(init->pix_fmt,
                                                     init->geometry.width, init->geometry.height / 2);
        }

        // Allocate full-height eedi2 buffers
        for (int ii = 0; ii < 5; ii++)
        {
            pv->eedi_full[ii] = hb_frame_buffer_init(init->pix_fmt,
                                                     init->geometry.width, init->geometry.height);
        }
    }

    // Set the functions for the current bit depth
    void (*yadif_decomb_filter_work)(void *thread_args_v);
    void (*eedi2_filter_work)(void *thread_args_v);
    void (*init_crop_table)(void **crop_table_out, const int max_value);
    void (*eedi2_init_limlut)(void **limlut_out, const int depth);

    switch (pv->depth)
    {
        case 8:
            yadif_decomb_filter_work = yadif_decomb_filter_work_8;
            eedi2_filter_work        = eedi2_filter_work_8;
            init_crop_table          = init_crop_table_8;
            eedi2_init_limlut        = eedi2_init_limlut_8;
            pv->filter               = filter_8;
            break;

        default:
            yadif_decomb_filter_work = yadif_decomb_filter_work_16;
            eedi2_filter_work        = eedi2_filter_work_16;
            init_crop_table          = init_crop_table_16;
            eedi2_init_limlut        = eedi2_init_limlut_16;
            pv->filter               = filter_16;
            break;
    }

    init_crop_table((void **)&pv->crop_table, pv->max_value);
    eedi2_init_limlut((void **)&pv->eedi_limlut, pv->depth);

    // Setup yadif taskset.
    pv->yadif_arguments = malloc(sizeof(yadif_arguments_t) * pv->cpu_count);
    if (pv->yadif_arguments == NULL ||
        taskset_init(&pv->yadif_taskset, "yadif_filter_segment", pv->cpu_count,
                     sizeof(yadif_thread_arg_t), yadif_decomb_filter_work) == 0)
    {
        hb_error("decomb yadif could not initialize taskset");
        return -1;
    }

    yadif_thread_arg_t *yadif_prev_thread_args = NULL;
    for (int ii = 0; ii < pv->cpu_count; ii++)
    {
        yadif_thread_arg_t *thread_args;

        thread_args = taskset_thread_args(&pv->yadif_taskset, ii);
        thread_args->pv = pv;
        thread_args->arg.segment = ii;
        thread_args->arg.taskset = &pv->yadif_taskset;

        for (int pp = 0; pp < 3; pp++)
        {
            if (yadif_prev_thread_args != NULL)
            {
                thread_args->segment_start[pp] =
                    yadif_prev_thread_args->segment_start[pp] +
                    yadif_prev_thread_args->segment_height[pp];
            }
            if (ii == pv->cpu_count - 1)
            {
                // Final segment
                thread_args->segment_height[pp] =
                    (hb_image_height(init->pix_fmt, init->geometry.height, pp)) - thread_args->segment_start[pp];
            }
            else
            {
                thread_args->segment_height[pp] = pv->segment_height[pp];
            }
        }
        pv->yadif_arguments[ii].dst = NULL;

        yadif_prev_thread_args = thread_args;
    }

    if (pv->mode & MODE_DECOMB_EEDI2)
    {
        // Create eedi2 taskset.
        if (taskset_init(&pv->eedi2_taskset, "eedi2_filter_segment", /*thread_count*/3,
                         sizeof(eedi2_thread_arg_t), eedi2_filter_work) == 0)
        {
            hb_error("decomb eedi2 could not initialize taskset");
            return -1;
        }

        if (pv->post_processing > 1)
        {
            int stride = hb_image_stride(init->pix_fmt, init->geometry.width, 0);

            pv->cx2 = (int *)eedi2_aligned_malloc(init->geometry.height * stride * sizeof(int), 16);
            pv->cy2 = (int *)eedi2_aligned_malloc(init->geometry.height * stride * sizeof(int), 16);
            pv->cxy = (int *)eedi2_aligned_malloc(init->geometry.height * stride * sizeof(int), 16);
            pv->tmpc = (int*)eedi2_aligned_malloc(init->geometry.height * stride * sizeof(int), 16);

            if (!pv->cx2 || !pv->cy2 || !pv->cxy || !pv->tmpc)
            {
                hb_error("EEDI2: failed to malloc derivative arrays");
                return -1;
            }
            else
            {
                hb_log("EEDI2: successfully malloced derivative arrays");
            }
        }

        for (int ii = 0; ii < 3; ii++)
        {
            eedi2_thread_arg_t *eedi2_thread_args;

            eedi2_thread_args = taskset_thread_args(&pv->eedi2_taskset, ii);

            eedi2_thread_args->pv = pv;
            eedi2_thread_args->arg.taskset = &pv->eedi2_taskset;
            eedi2_thread_args->arg.segment = ii;
        }
    }
    if (pv->mode & MODE_DECOMB_BOB)
    {
        init->vrate.num *= 2;
    }

    pv->output = *init;

    return 0;
}

static void hb_decomb_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (!pv)
    {
        return;
    }

    if (pv->frames > 1)
    {
        hb_log("decomb: deinterlaced %i | blended %i | unfiltered %i | total %i",
               pv->deinterlaced, pv->blended, pv->unfiltered, pv->frames);
    }

    taskset_fini(&pv->yadif_taskset);

    if (pv->mode & MODE_DECOMB_EEDI2)
    {
        taskset_fini(&pv->eedi2_taskset);
    }

    hb_buffer_list_close(&pv->out_list);

    // Cleanup reference buffers
    for (int ii = 0; ii < 3; ii++)
    {
        hb_buffer_close(&pv->ref[ii]);
    }

    if (pv->mode & MODE_DECOMB_EEDI2)
    {
        // Cleanup eedi-half buffers
        for (int ii = 0; ii < 4; ii++)
        {
            hb_buffer_close(&pv->eedi_half[ii]);
        }

        // Cleanup eedi-full buffers
        for (int ii = 0; ii < 5; ii++)
        {
            hb_buffer_close(&pv->eedi_full[ii]);
        }
    }

    if (pv->post_processing > 1  && (pv->mode & MODE_DECOMB_EEDI2))
    {
        if (pv->cx2) eedi2_aligned_free(pv->cx2);
        if (pv->cy2) eedi2_aligned_free(pv->cy2);
        if (pv->cxy) eedi2_aligned_free(pv->cxy);
        if (pv->tmpc) eedi2_aligned_free(pv->tmpc);
    }

    free((void *)pv->eedi_limlut);
    free((void *)pv->crop_table);

    // free memory for yadif structs
    free(pv->yadif_arguments);

    free(pv);
    filter->private_data = NULL;
}

static void process_frame(hb_filter_private_t *pv)
{
    if ((pv->mode & MODE_DECOMB_SELECTIVE) &&
        pv->ref[1]->s.combed == HB_COMB_NONE)
    {
        // Input buffer is not combed.  Just make a dup of it.
        hb_buffer_t *buf = hb_buffer_shallow_dup(pv->ref[1]);
        hb_buffer_list_append(&pv->out_list, buf);
        pv->frames++;
        pv->unfiltered++;
    }
    else
    {
        // Determine if top-field first layout
        int tff;
        if (pv->parity < 0)
        {
            uint16_t flags = pv->ref[1]->s.flags;
            tff = ((flags & PIC_FLAG_PROGRESSIVE_FRAME) == 0) ?
                  !!(flags & PIC_FLAG_TOP_FIELD_FIRST) : 1;
        }
        else
        {
            tff = (pv->parity & 1) ^ 1;
        }

        // Deinterlace both fields if bob
        int frame, num_frames = 1;
        if (pv->mode & MODE_DECOMB_BOB)
        {
            num_frames = 2;
        }

        // Will need up to 2 buffers simultaneously

        // Perform filtering
        for (frame = 0; frame < num_frames; frame++)
        {
            hb_buffer_t *buf;
            int parity = frame ^ tff ^ 1;

            // tff for eedi2
            pv->tff = !parity;

            buf = hb_frame_buffer_init(pv->ref[1]->f.fmt,
                                       pv->ref[1]->f.width,
                                       pv->ref[1]->f.height);
            buf->f.color_prim      = pv->output.color_prim;
            buf->f.color_transfer  = pv->output.color_transfer;
            buf->f.color_matrix    = pv->output.color_matrix;
            buf->f.color_range     = pv->output.color_range;
            buf->f.chroma_location = pv->output.chroma_location;
            pv->filter(pv, buf, parity, tff);

            // Copy buffered settings to output buffer settings
            hb_buffer_copy_props(buf, pv->ref[1]);

            hb_buffer_list_append(&pv->out_list, buf);
        }

        // if this frame was deinterlaced and bob mode is engaged, halve
        // the duration of the saved timestamps.
        if (pv->mode & MODE_DECOMB_BOB)
        {
            hb_buffer_t *first  = hb_buffer_list_head(&pv->out_list);
            hb_buffer_t *second = hb_buffer_list_tail(&pv->out_list);
            first->s.stop -= (first->s.stop - first->s.start) / 2LL;
            second->s.start = first->s.stop;
            second->s.new_chap = 0;
        }
    }
}

static int hb_decomb_work(hb_filter_object_t *filter,
                          hb_buffer_t **buf_in,
                          hb_buffer_t **buf_out)
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in;

    // Input buffer is always consumed.
    *buf_in = NULL;
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        if (pv->ref[2] != NULL)
        {
            // Duplicate last frame and process refs
            store_ref(pv, hb_buffer_shallow_dup(pv->ref[2]));
            process_frame(pv);
        }
        hb_buffer_list_append(&pv->out_list, in);
        *buf_out = hb_buffer_list_clear(&pv->out_list);
        return HB_FILTER_DONE;
    }

    // yadif requires 3 buffers, prev, cur, and next.  For the first
    // frame, there can be no prev, so we duplicate the first frame.
    if (!pv->ready)
    {
        // If yadif is not ready, store another ref and return HB_FILTER_DELAY
        store_ref(pv, hb_buffer_shallow_dup(in));
        store_ref(pv, in);
        pv->ready = 1;
        // Wait for next
        return HB_FILTER_DELAY;
    }

    store_ref(pv, in);
    process_frame(pv);

    *buf_out = hb_buffer_list_clear(&pv->out_list);
    return HB_FILTER_OK;
}
