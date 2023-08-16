/* comb_detect.c

   Copyright (c) 2003-2022 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

*/

/*****
Parameters:
    Mode : Spatial metric : Motion thresh : Spatial thresh : Mask Filter Mode :
    Block thresh : Block width : Block height

Defaults:
    3:2:3:3:2:40:16:16

Original "Faster" settings:
    0:2:6:9:1:80:16:16
*****/

#define MODE_GAMMA        1 // Scale gamma when decombing
#define MODE_FILTER       2 // Filter combing mask
#define MODE_MASK         4 // Output combing masks instead of pictures
#define MODE_COMPOSITE    8 // Overlay combing mask onto picture

#define FILTER_CLASSIC 1
#define FILTER_ERODE_DILATE 2

#include "handbrake/handbrake.h"
#include "handbrake/taskset.h"

typedef struct comb_detect_thread_arg_s
{
    taskset_thread_arg_t arg;
    hb_filter_private_t *pv;
    int segment_start[3];
    int segment_height[3];
} comb_detect_thread_arg_t;

struct hb_filter_private_s
{
    int depth;
    int bps;
    int max_value;
    int half_value;

    // comb detect parameters
    int                mode;
    int                filter_mode;
    int                spatial_metric;
    int                motion_threshold;
    int                spatial_threshold;
    int                block_threshold;
    int                block_width;
    int                block_height;
    int               *block_score;
    int                comb_check_complete;
    int                comb_check_nthreads;

    // Computed parameters
    float              gamma_motion_threshold;
    float              gamma_spatial_threshold;
    float              gamma_spatial_threshold6;
    int                spatial_threshold_squared;
    int                spatial_threshold6;
    int                comb32detect_min;
    int                comb32detect_max;
    float             *gamma_lut;

    int                comb_detect_ready;
    int                force_exaustive_check;

    hb_buffer_t       *ref[3];
    int                ref_used[3];

    // Make buffers to store a comb masks.
    hb_buffer_t       *mask;
    hb_buffer_t       *mask_filtered;
    hb_buffer_t       *mask_temp;
    int                mask_box_x;
    int                mask_box_y;

    int                cpu_count;
    int                segment_height[3];

    taskset_t          comb_detect_filter_taskset; // Threads for comb detection
    taskset_t          comb_detect_check_taskset;  // Threads for comb check
    taskset_t          mask_filter_taskset; // Threads for comb detect mask filter
    taskset_t          mask_erode_taskset;  // Threads for comb detect mask erode
    taskset_t          mask_dilate_taskset; // Threads for comb detect mask dilate

    void (*detect_gamma_combed_segment)(hb_filter_private_t *pv,
                                        int segment_start, int segment_stop);
    void (*detect_combed_segment)(hb_filter_private_t *pv,
                                  int segment_start, int segment_stop);
    void (*apply_mask)(hb_filter_private_t *pv, hb_buffer_t *b);

    hb_buffer_list_t   out_list;

    // Filter statistics
    int                comb_heavy;
    int                comb_light;
    int                comb_none;
    int                frames;
};

static int comb_detect_init(hb_filter_object_t *filter,
                            hb_filter_init_t *init);

static int comb_detect_work(hb_filter_object_t *filter,
                            hb_buffer_t **buf_in,
                            hb_buffer_t **buf_out );

static void comb_detect_close(hb_filter_object_t *filter);

static const char comb_detect_template[] =
    "mode=^"HB_INT_REG"$:spatial-metric=^([012])$:"
    "motion-thresh=^"HB_INT_REG"$:spatial-thresh=^"HB_INT_REG"$:"
    "filter-mode=^([012])$:block-thresh=^"HB_INT_REG"$:"
    "block-width=^"HB_INT_REG"$:block-height=^"HB_INT_REG"$:"
    "disable=^"HB_BOOL_REG"$";

hb_filter_object_t hb_filter_comb_detect =
{
    .id                = HB_FILTER_COMB_DETECT,
    .enforce_order     = 1,
    .name              = "Comb Detect",
    .settings          = NULL,
    .init              = comb_detect_init,
    .work              = comb_detect_work,
    .close             = comb_detect_close,
    .settings_template = comb_detect_template,
};

#define BIT_DEPTH 8
#include "templates/comb_detect_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 16
#include "templates/comb_detect_template.c"
#undef BIT_DEPTH

static void check_filtered_combing_mask(hb_filter_private_t *pv, int segment, int start, int stop)
{
    // Go through the mask in X*Y blocks. If any of these windows
    // have threshold or more combed pixels, consider the whole
    // frame to be combed and send it on to be deinterlaced.
    // Block mask threshold -- The number of pixels
    // in a block_width * block_height window of
    // the mask that need to show combing for the
    // whole frame to be seen as such.

    const int threshold     = pv->block_threshold;
    const int block_width   = pv->block_width;
    const int block_height  = pv->block_height;

    const int stride = pv->mask_filtered->plane[0].stride;
    const int width = pv->mask_filtered->plane[0].width;

    for (int y = start; y < (stop - block_height + 1); y = y + block_height)
    {
        for (int x = 0; x < (width - block_width); x = x + block_width)
        {
            int block_score = 0;

            for (int block_y = 0; block_y < block_height; block_y++)
            {
                const int my = y + block_y;
                const uint8_t *mask_p = &pv->mask_filtered->plane[0].data[my * stride + x];

                for (int block_x = 0; block_x < block_width; block_x++)
                {
                    block_score += mask_p[0];
                    mask_p++;
                }
            }

            if (pv->comb_check_complete)
            {
                // Some other thread found coming before this one
                return;
            }

            if (block_score >= (threshold / 2))
            {
                pv->mask_box_x = x;
                pv->mask_box_y = y;

                pv->block_score[segment] = block_score;
                if (block_score > threshold)
                {
                    pv->comb_check_complete = 1;
                    return;
                }
            }
        }
    }
}

static void check_combing_mask(hb_filter_private_t *pv, int segment, int start, int stop)
{
    // Go through the mask in X*Y blocks. If any of these windows
    // have threshold or more combed pixels, consider the whole
    // frame to be combed and send it on to be deinterlaced.
    // Block mask threshold -- The number of pixels
    // in a block_width * block_height window of
    // the mask that need to show combing for the
    // whole frame to be seen as such.

    const int threshold    = pv->block_threshold;
    const int block_width  = pv->block_width;
    const int block_height = pv->block_height;

    const int stride = pv->mask->plane[0].stride;
    const int width = pv->mask->plane[0].width;

    for (int y = start; y < (stop - block_height + 1); y = y + block_height)
    {
        for (int x = 0; x < (width - block_width); x = x + block_width)
        {
            int block_score = 0;

            for (int block_y = 0; block_y < block_height; block_y++)
            {
                const int mask_y = y + block_y;
                const uint8_t *mask_p = &pv->mask->plane[0].data[mask_y * stride + x];

                for (int block_x = 0; block_x < block_width; block_x++)
                {
                    // We only want to mark a pixel in a block as combed
                    // if the adjacent pixels are as well. Got to
                    // handle the sides separately.
                    if ((x + block_x) == 0)
                    {
                        block_score += mask_p[0] & mask_p[1];
                    }
                    else if ((x + block_x) == (width -1))
                    {
                        block_score += mask_p[-1] & mask_p[0];
                    }
                    else
                    {
                        block_score += mask_p[-1] & mask_p[0] & mask_p[1];
                    }

                    mask_p++;
                }
            }

            if (pv->comb_check_complete)
            {
                // Some other thread found coming before this one
                return;
            }

            if (block_score >= (threshold / 2))
            {
                pv->mask_box_x = x;
                pv->mask_box_y = y;

                pv->block_score[segment] = block_score;
                if (block_score > threshold)
                {
                    pv->comb_check_complete = 1;
                    return;
                }
            }
        }
    }
}

static void mask_dilate_work(void *thread_args_v)
{
    comb_detect_thread_arg_t *thread_args = thread_args_v;
    hb_filter_private_t *pv = thread_args->pv;

    const int segment_start = thread_args->segment_start[0];
    const int segment_stop = segment_start + thread_args->segment_height[0];

    const int dilation_threshold = 4;

    const int width = pv->mask_filtered->plane[0].width;
    const int height = pv->mask_filtered->plane[0].height;
    const int stride = pv->mask_filtered->plane[0].stride;

    int start, stop, p, c, n;

    if (segment_start == 0)
    {
        start = 1;
        p = 0;
        c = 1;
        n = 2;
    }
    else
    {
        start = segment_start;
        p = segment_start - 1;
        c = segment_start;
        n = segment_start + 1;
    }

    if (segment_stop == height)
    {
        stop = height -1;
    }
    else
    {
        stop = segment_stop;
    }

    const uint8_t *curp = &pv->mask_filtered->plane[0].data[p * stride + 1];
    const uint8_t *cur  = &pv->mask_filtered->plane[0].data[c * stride + 1];
    const uint8_t *curn = &pv->mask_filtered->plane[0].data[n * stride + 1];
    uint8_t *dst = &pv->mask_temp->plane[0].data[c * stride + 1];

    for (int yy = start; yy < stop; yy++)
    {
        for (int xx = 1; xx < width - 1; xx++)
        {
            if (cur[xx])
            {
                dst[xx] = 1;
                continue;
            }

            const int count = curp[xx-1] + curp[xx] + curp[xx+1] +
                              cur [xx-1] +            cur [xx+1] +
                              curn[xx-1] + curn[xx] + curn[xx+1];

            dst[xx] = count >= dilation_threshold;
        }
        curp += stride;
        cur += stride;
        curn += stride;
        dst += stride;
    }
}

static void mask_erode_work(void *thread_args_v)
{
    comb_detect_thread_arg_t *thread_args = thread_args_v;
    hb_filter_private_t *pv = thread_args->pv;

    const int segment_start = thread_args->segment_start[0];
    const int segment_stop = segment_start + thread_args->segment_height[0];

    const int erosion_threshold = 2;

    const int width = pv->mask_filtered->plane[0].width;
    const int height = pv->mask_filtered->plane[0].height;
    const int stride = pv->mask_filtered->plane[0].stride;

    int start, stop, p, c, n;

    if (segment_start == 0)
    {
        start = 1;
        p = 0;
        c = 1;
        n = 2;
    }
    else
    {
        start = segment_start;
        p = segment_start - 1;
        c = segment_start;
        n = segment_start + 1;
    }

    if (segment_stop == height)
    {
        stop = height -1;
    }
    else
    {
        stop = segment_stop;
    }

    const uint8_t *curp = &pv->mask_temp->plane[0].data[p * stride + 1];
    const uint8_t *cur  = &pv->mask_temp->plane[0].data[c * stride + 1];
    const uint8_t *curn = &pv->mask_temp->plane[0].data[n * stride + 1];
    uint8_t *dst = &pv->mask_filtered->plane[0].data[c * stride + 1];

    for (int yy = start; yy < stop; yy++)
    {
        for (int xx = 1; xx < width - 1; xx++)
        {
            if (cur[xx] == 0)
            {
                dst[xx] = 0;
                continue;
            }

            const int count = curp[xx-1] + curp[xx] + curp[xx+1] +
                              cur [xx-1] +            cur [xx+1] +
                              curn[xx-1] + curn[xx] + curn[xx+1];

            dst[xx] = count >= erosion_threshold;
        }
        curp += stride;
        cur += stride;
        curn += stride;
        dst += stride;
    }
}

static void mask_filter_work(void *thread_args_v)
{
    comb_detect_thread_arg_t *thread_args = thread_args_v;
    hb_filter_private_t *pv = thread_args->pv;

    const int width = pv->mask->plane[0].width;
    const int height = pv->mask->plane[0].height;
    const int stride = pv->mask->plane[0].stride;

    int start, stop, p, c, n;
    int segment_start = thread_args->segment_start[0];
    int segment_stop = segment_start + thread_args->segment_height[0];

    if (segment_start == 0)
    {
        start = 1;
        p = 0;
        c = 1;
        n = 2;
    }
    else
    {
        start = segment_start;
        p = segment_start - 1;
        c = segment_start;
        n = segment_start + 1;
    }

    if (segment_stop == height)
    {
        stop = height - 1;
    }
    else
    {
        stop = segment_stop;
    }

    const uint8_t *curp = &pv->mask->plane[0].data[p * stride + 1];
    const uint8_t *cur  = &pv->mask->plane[0].data[c * stride + 1];
    const uint8_t *curn = &pv->mask->plane[0].data[n * stride + 1];
    uint8_t *dst = (pv->filter_mode == FILTER_CLASSIC) ?
                     &pv->mask_filtered->plane[0].data[c * stride + 1] :
                     &pv->mask_temp->plane[0].data[c * stride + 1] ;

    for (int yy = start; yy < stop; yy++)
    {
        for (int xx = 1; xx < width - 1; xx++)
        {
            const int h_count = cur[xx-1] & cur[xx] & cur[xx+1];
            const int v_count = curp[xx] & cur[xx] & curn[xx];

            if (pv->filter_mode == FILTER_CLASSIC)
            {
                dst[xx] = h_count;
            }
            else
            {
                dst[xx] = h_count & v_count;
            }
        }
        curp += stride;
        cur += stride;
        curn += stride;
        dst += stride;
    }
}

static void comb_detect_check_work(void *thread_args_v)
{
    comb_detect_thread_arg_t *thread_args = thread_args_v;
    hb_filter_private_t *pv = thread_args->pv;

    int segment = thread_args->arg.segment;
    int segment_start = thread_args->segment_start[0];
    int segment_stop = segment_start + thread_args->segment_height[0];

    if (pv->mode & MODE_FILTER)
    {
        check_filtered_combing_mask(pv, segment, segment_start, segment_stop);
    }
    else
    {
        check_combing_mask(pv, segment, segment_start, segment_stop);
    }
}

static void comb_detect_filter_work(void *thread_args_v)
{
    comb_detect_thread_arg_t *thread_args = thread_args_v;
    hb_filter_private_t *pv = thread_args->pv;

    //Process segment (for now just from luma)
    const int segment_start = thread_args->segment_start[0];
    const int segment_stop = segment_start + thread_args->segment_height[0];

    if (pv->mode & MODE_GAMMA)
    {
        pv->detect_gamma_combed_segment(pv, segment_start, segment_stop);
    }
    else
    {
        pv->detect_combed_segment(pv, segment_start, segment_stop);
    }
}

static void store_ref(hb_filter_private_t *pv, hb_buffer_t *b)
{
    // Free unused buffer
    if (!pv->ref_used[0])
    {
        hb_buffer_close(&pv->ref[0]);
    }
    memmove(&pv->ref[0],      &pv->ref[1],      sizeof(pv->ref[0])      * 2);
    memmove(&pv->ref_used[0], &pv->ref_used[1], sizeof(pv->ref_used[0]) * 2);
    pv->ref[2]      = b;
    pv->ref_used[2] = 0;
}

static void reset_combing_results(hb_filter_private_t *pv)
{
    pv->comb_check_complete = 0;
    for (int ii = 0; ii < pv->comb_check_nthreads; ii++)
    {
       pv->block_score[ii] = 0;
    }
}

static int check_combing_results(hb_filter_private_t *pv)
{
    int combed = HB_COMB_NONE;
    for (int ii = 0; ii < pv->comb_check_nthreads; ii++)
    {
        if (pv->block_score[ii] >= (pv->block_threshold / 2))
        {
            if (pv->block_score[ii] <= pv->block_threshold)
            {
                // Indicate light combing for block_score that is between
                // ( pv->block_threshold / 2 ) and pv->block_threshold
                combed = HB_COMB_LIGHT;
            }
            else if (pv->block_score[ii] > pv->block_threshold)
            {
                return HB_COMB_HEAVY;
            }
        }
    }
    return combed;
}

static int comb_segmenter(hb_filter_private_t *pv)
{
    /*
     * Now that all data for comb detection is ready for
     * our threads, fire them off and wait for their completion.
     */
    taskset_cycle(&pv->comb_detect_filter_taskset);

    if (pv->mode & MODE_FILTER)
    {
        taskset_cycle(&pv->mask_filter_taskset);
        if (pv->filter_mode == FILTER_ERODE_DILATE)
        {
            taskset_cycle(&pv->mask_erode_taskset);
            taskset_cycle(&pv->mask_dilate_taskset);
            taskset_cycle(&pv->mask_erode_taskset);
        }
    }
    reset_combing_results(pv);
    taskset_cycle(&pv->comb_detect_check_taskset);
    return check_combing_results(pv);
}

static void build_gamma_lut(hb_filter_private_t *pv)
{
    const int max = pv->max_value;
    for (int i = 0; i < max + 1; i++)
    {
        pv->gamma_lut[i] = pow(((float)i / (float)max), 2.2f);
    }
}

static int comb_detect_init(hb_filter_object_t *filter,
                            hb_filter_init_t   *init)
{
    filter->private_data = calloc(1, sizeof(struct hb_filter_private_s));
    if (filter->private_data == NULL)
    {
        hb_error("comb_detect: calloc failed");
        return -1;
    }
    hb_filter_private_t *pv = filter->private_data;

    hb_buffer_list_clear(&pv->out_list);

    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(init->pix_fmt);
    pv->depth      = desc->comp[0].depth;
    pv->bps        = pv->depth > 8 ? 2 : 1;
    pv->max_value  = (1 << pv->depth) - 1;
    pv->half_value = (1 << pv->depth) / 2;

    pv->gamma_lut = malloc(sizeof(float) * (pv->max_value + 1));
    if (pv->gamma_lut == NULL)
    {
        hb_error("comb_detect: malloc failed");
        return -1;
    }
    build_gamma_lut(pv);

    pv->frames = 0;
    pv->force_exaustive_check = 1;
    pv->comb_heavy = 0;
    pv->comb_light = 0;
    pv->comb_none = 0;

    pv->comb_detect_ready = 0;

    pv->mode              = MODE_GAMMA | MODE_FILTER;
    pv->filter_mode       = FILTER_ERODE_DILATE;
    pv->spatial_metric    = 2;
    pv->motion_threshold  = 3;
    pv->spatial_threshold = 3;
    pv->block_threshold   = 40;
    pv->block_width       = 16;
    pv->block_height      = 16;

    if (filter->settings)
    {
        hb_value_t *dict = filter->settings;

        // Get comb detection settings
        hb_dict_extract_int(&pv->mode, dict, "mode");
        hb_dict_extract_int(&pv->spatial_metric, dict, "spatial-metric");
        hb_dict_extract_int(&pv->motion_threshold, dict, "motion-thresh");
        hb_dict_extract_int(&pv->spatial_threshold, dict, "spatial-thresh");
        hb_dict_extract_int(&pv->filter_mode, dict, "filter-mode");
        hb_dict_extract_int(&pv->block_threshold, dict, "block-thresh");
        hb_dict_extract_int(&pv->block_width, dict, "block-width");
        hb_dict_extract_int(&pv->block_height, dict, "block-height");
    }

    // Scale the thresholds for the current depth
    pv->motion_threshold  <<= (pv->depth - 8);
    pv->spatial_threshold <<= (pv->depth - 8);

    // Compute thresholds
    pv->gamma_motion_threshold    = (float)pv->motion_threshold / (float)pv->max_value;
    pv->gamma_spatial_threshold   = (float)pv->spatial_threshold / (float)pv->max_value;
    pv->gamma_spatial_threshold6  = 6 * pv->gamma_spatial_threshold;
    pv->spatial_threshold_squared = pv->spatial_threshold * pv->spatial_threshold;
    pv->spatial_threshold6        = 6 * pv->spatial_threshold;
    pv->comb32detect_min = 10 << (pv->depth - 8);
    pv->comb32detect_max = 15 << (pv->depth - 8);

    pv->cpu_count = hb_get_cpu_count();

    // Make segment sizes an even number of lines
    int height = hb_image_height(init->pix_fmt, init->geometry.height, 0);
    // each segment of each plane must begin on an even row.
    pv->segment_height[0] = (height / pv->cpu_count) & ~3;
    pv->segment_height[1] = hb_image_height(init->pix_fmt, pv->segment_height[0], 1);
    pv->segment_height[2] = hb_image_height(init->pix_fmt, pv->segment_height[0], 2);

    /* Allocate buffers to store comb masks. */
    pv->mask = hb_frame_buffer_init(AV_PIX_FMT_GRAY8,
                                init->geometry.width, init->geometry.height);
    pv->mask_filtered = hb_frame_buffer_init(AV_PIX_FMT_GRAY8,
                                init->geometry.width, init->geometry.height);
    pv->mask_temp = hb_frame_buffer_init(AV_PIX_FMT_GRAY8,
                                init->geometry.width, init->geometry.height);
    memset(pv->mask->data, 0, pv->mask->size);
    memset(pv->mask_filtered->data, 0, pv->mask_filtered->size);
    memset(pv->mask_temp->data, 0, pv->mask_temp->size);

    // Set the functions for the current bit depth
    switch (pv->depth)
    {
        case 8:
            pv->detect_gamma_combed_segment = detect_gamma_combed_segment_8;
            pv->detect_combed_segment       = detect_combed_segment_8;
            pv->apply_mask                  = apply_mask_8;
            break;

        default:
            pv->detect_gamma_combed_segment = detect_gamma_combed_segment_16;
            pv->detect_combed_segment       = detect_combed_segment_16;
            pv->apply_mask                  = apply_mask_16;
            break;
    }

    /*
     * Create comb detection taskset.
     */
    if (taskset_init(&pv->comb_detect_filter_taskset, "comb_detect_filter_segment", pv->cpu_count,
                     sizeof(comb_detect_thread_arg_t), comb_detect_filter_work) == 0)
    {
        hb_error("comb_detect could not initialize taskset");
        return -1;
    }

    comb_detect_thread_arg_t *comb_detect_prev_thread_args = NULL;
    for (int ii = 0; ii < pv->cpu_count; ii++)
    {
        comb_detect_thread_arg_t *thread_args;

        thread_args = taskset_thread_args( &pv->comb_detect_filter_taskset, ii );
        thread_args->pv = pv;
        thread_args->arg.segment = ii;
        thread_args->arg.taskset = &pv->comb_detect_filter_taskset;

        for (int pp = 0; pp < 3; pp++)
        {
            if (comb_detect_prev_thread_args != NULL)
            {
                thread_args->segment_start[pp] =
                    comb_detect_prev_thread_args->segment_start[pp] +
                    comb_detect_prev_thread_args->segment_height[pp];
            }
            if (ii == pv->cpu_count - 1)
            {
                /*
                 * Final segment
                 */
                thread_args->segment_height[pp] =
                    hb_image_height(init->pix_fmt, init->geometry.height, pp) -
                    thread_args->segment_start[pp];
            } else {
                thread_args->segment_height[pp] = pv->segment_height[pp];
            }
        }

        comb_detect_prev_thread_args = thread_args;
    }

    pv->comb_check_nthreads = init->geometry.height / pv->block_height;

    if (pv->comb_check_nthreads > pv->cpu_count)
    {
        pv->comb_check_nthreads = pv->cpu_count;
    }

    pv->block_score = calloc(pv->comb_check_nthreads, sizeof(int));

    /*
     * Create comb check taskset.
     */
    if (taskset_init(&pv->comb_detect_check_taskset, "comb_detect_check_segment", pv->comb_check_nthreads,
                     sizeof(comb_detect_thread_arg_t), comb_detect_check_work) == 0)
    {
        hb_error("comb_detect check could not initialize taskset");
        return -1;
    }

    comb_detect_prev_thread_args = NULL;
    for (int ii = 0; ii < pv->comb_check_nthreads; ii++)
    {
        comb_detect_thread_arg_t *thread_args;

        thread_args = taskset_thread_args(&pv->comb_detect_check_taskset, ii);
        thread_args->pv = pv;
        thread_args->arg.segment = ii;
        thread_args->arg.taskset = &pv->comb_detect_check_taskset;

        for (int pp = 0; pp < 3; pp++)
        {
            if (comb_detect_prev_thread_args != NULL)
            {
                thread_args->segment_start[pp] =
                    comb_detect_prev_thread_args->segment_start[pp] +
                    comb_detect_prev_thread_args->segment_height[pp];
            }

            // Make segment height a multiple of block_height
            int h = hb_image_height(init->pix_fmt, init->geometry.height, pp) / pv->comb_check_nthreads;
            h = h / pv->block_height * pv->block_height;
            if (h == 0)
                h = pv->block_height;

            if (ii == pv->comb_check_nthreads - 1)
            {
                /*
                 * Final segment
                 */
                thread_args->segment_height[pp] =
                    hb_image_height(init->pix_fmt, init->geometry.height, pp) -
                    thread_args->segment_start[pp];
            } else {
                thread_args->segment_height[pp] = h;
            }
        }

        comb_detect_prev_thread_args = thread_args;
    }

    if (pv->mode & MODE_FILTER)
    {
        if (taskset_init(&pv->mask_filter_taskset, "mask_filter_segment", pv->cpu_count,
                         sizeof(comb_detect_thread_arg_t), mask_filter_work) == 0)
        {
            hb_error( "mask filter could not initialize taskset" );
            return -1;
        }

        comb_detect_prev_thread_args = NULL;
        for (int ii = 0; ii < pv->cpu_count; ii++)
        {
            comb_detect_thread_arg_t *thread_args;

            thread_args = taskset_thread_args(&pv->mask_filter_taskset, ii);
            thread_args->pv = pv;
            thread_args->arg.segment = ii;
            thread_args->arg.taskset = &pv->mask_filter_taskset;

            for (int pp = 0; pp < 3; pp++)
            {
                if (comb_detect_prev_thread_args != NULL)
                {
                    thread_args->segment_start[pp] =
                        comb_detect_prev_thread_args->segment_start[pp] +
                        comb_detect_prev_thread_args->segment_height[pp];
                }

                if (ii == pv->cpu_count - 1)
                {
                    /*
                     * Final segment
                     */
                    thread_args->segment_height[pp] =
                        hb_image_height(init->pix_fmt, init->geometry.height, pp) -
                        thread_args->segment_start[pp];
                } else {
                    thread_args->segment_height[pp] = pv->segment_height[pp];
                }
            }

            comb_detect_prev_thread_args = thread_args;
        }

        if (pv->filter_mode == FILTER_ERODE_DILATE)
        {
            if (taskset_init(&pv->mask_erode_taskset, "mask_erode_segment", pv->cpu_count,
                             sizeof(comb_detect_thread_arg_t), mask_erode_work) == 0)
            {
                hb_error("mask erode could not initialize taskset");
                return -1;
            }

            comb_detect_prev_thread_args = NULL;
            for (int ii = 0; ii < pv->cpu_count; ii++)
            {
                comb_detect_thread_arg_t *thread_args;

                thread_args = taskset_thread_args( &pv->mask_erode_taskset, ii );
                thread_args->pv = pv;
                thread_args->arg.segment = ii;
                thread_args->arg.taskset = &pv->mask_erode_taskset;

                for (int pp = 0; pp < 3; pp++)
                {
                    if (comb_detect_prev_thread_args != NULL)
                    {
                        thread_args->segment_start[pp] =
                            comb_detect_prev_thread_args->segment_start[pp] +
                            comb_detect_prev_thread_args->segment_height[pp];
                    }

                    if (ii == pv->cpu_count - 1)
                    {
                        /*
                         * Final segment
                         */
                        thread_args->segment_height[pp] =
                            hb_image_height(init->pix_fmt, init->geometry.height, pp) -
                            thread_args->segment_start[pp];
                    } else {
                        thread_args->segment_height[pp] = pv->segment_height[pp];
                    }
                }

                comb_detect_prev_thread_args = thread_args;
            }

            if (taskset_init(&pv->mask_dilate_taskset, "mask_dilate_segment", pv->cpu_count,
                             sizeof(comb_detect_thread_arg_t), mask_dilate_work) == 0)
            {
                hb_error("mask dilate could not initialize taskset");
                return -1;
            }

            comb_detect_prev_thread_args = NULL;
            for (int ii = 0; ii < pv->cpu_count; ii++)
            {
                comb_detect_thread_arg_t *thread_args;

                thread_args = taskset_thread_args( &pv->mask_dilate_taskset, ii );
                thread_args->pv = pv;
                thread_args->arg.segment = ii;
                thread_args->arg.taskset = &pv->mask_dilate_taskset;

                for (int pp = 0; pp < 3; pp++)
                {
                    if (comb_detect_prev_thread_args != NULL)
                    {
                        thread_args->segment_start[pp] =
                            comb_detect_prev_thread_args->segment_start[pp] +
                            comb_detect_prev_thread_args->segment_height[pp];
                    }

                    if (ii == pv->cpu_count - 1)
                    {
                        /*
                         * Final segment
                         */
                        thread_args->segment_height[pp] =
                            hb_image_height(init->pix_fmt, init->geometry.height, pp) -
                            thread_args->segment_start[pp];
                    } else {
                        thread_args->segment_height[pp] = pv->segment_height[pp];
                    }
                }

                comb_detect_prev_thread_args = thread_args;
            }
        }
    }

    return 0;
}

static void comb_detect_close(hb_filter_object_t *filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    hb_log("comb detect: heavy %i | light %i | uncombed %i | total %i",
           pv->comb_heavy,  pv->comb_light,  pv->comb_none, pv->frames);

    taskset_fini(&pv->comb_detect_filter_taskset);
    taskset_fini(&pv->comb_detect_check_taskset);

    if (pv->mode & MODE_FILTER)
    {
        taskset_fini( &pv->mask_filter_taskset );
        if (pv->filter_mode == FILTER_ERODE_DILATE)
        {
            taskset_fini(&pv->mask_erode_taskset);
            taskset_fini(&pv->mask_dilate_taskset);
        }
    }

    /* Cleanup reference buffers. */
    for (int ii = 0; ii < 3; ii++)
    {
        if (!pv->ref_used[ii])
        {
            hb_buffer_close(&pv->ref[ii]);
        }
    }

    /* Cleanup combing masks. */
    hb_buffer_close(&pv->mask);
    hb_buffer_close(&pv->mask_filtered);
    hb_buffer_close(&pv->mask_temp);

    free(pv->gamma_lut);
    free(pv->block_score);
    free(pv);
    filter->private_data = NULL;
}

static void process_frame(hb_filter_private_t *pv)
{
    int combed = comb_segmenter(pv);

    switch (combed)
    {
        case HB_COMB_HEAVY:
            pv->comb_heavy++;
            break;

        case HB_COMB_LIGHT:
            pv->comb_light++;
            break;

        case HB_COMB_NONE:
        default:
            pv->comb_none++;
            break;
    }
    pv->frames++;
    if (((pv->mode & MODE_MASK) || (pv->mode & MODE_COMPOSITE)) && combed)
    {
        hb_buffer_t *out;
        out = hb_buffer_dup(pv->ref[1]);
        pv->apply_mask(pv, out);
        out->s.combed = combed;
        hb_buffer_list_append(&pv->out_list, out);
    }
    else
    {
        pv->ref_used[1] = 1;
        pv->ref[1]->s.combed = combed;
        hb_buffer_list_append(&pv->out_list, pv->ref[1]);
    }

    pv->force_exaustive_check = 0;
}

static int comb_detect_work(hb_filter_object_t *filter,
                            hb_buffer_t **buf_in,
                            hb_buffer_t **buf_out )
{
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t         *in = *buf_in;

    // Input buffer is always consumed.
    *buf_in = NULL;
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        // Duplicate last frame and process refs
        store_ref(pv, hb_buffer_dup(pv->ref[2]));
        if (pv->ref[0] != NULL)
        {
            pv->force_exaustive_check = 1;
            process_frame(pv);
        }
        hb_buffer_list_append(&pv->out_list, in);
        *buf_out = hb_buffer_list_clear(&pv->out_list);
        return HB_FILTER_DONE;
    }

    // comb detect requires 3 buffers, prev, cur, and next.  For the first
    // frame, there can be no prev, so we duplicate the first frame.
    if (!pv->comb_detect_ready)
    {
        // If not ready, store duplicate ref and return HB_FILTER_DELAY
        store_ref(pv, hb_buffer_dup(in));
        store_ref(pv, in);
        pv->comb_detect_ready = 1;
        // Wait for next
        return HB_FILTER_DELAY;
    }

    store_ref(pv, in);
    process_frame(pv);

    // Output buffers may also be in comb detect's internal ref list.
    // Since buffers are not reference counted, we must wait until
    // we are certain they are no longer in the ref list before sending
    // down the pipeline where they will ultimately get closed.
    if (hb_buffer_list_count(&pv->out_list) > 3)
    {
        *buf_out = hb_buffer_list_rem_head(&pv->out_list);
    }
    return HB_FILTER_OK;
}
