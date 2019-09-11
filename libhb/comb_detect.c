/* comb_detect.c

   Copyright (c) 2003-2019 HandBrake Team
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

typedef struct decomb_thread_arg_s {
    hb_filter_private_t *pv;
    int segment;
    int segment_start[3];
    int segment_height[3];
} decomb_thread_arg_t;

struct hb_filter_private_s
{
    // comb detect parameters
    int                mode;
    int                filter_mode;
    int                spatial_metric;
    int                motion_threshold;
    int                spatial_threshold;
    int                block_threshold;
    int                block_width;
    int                block_height;
    int              * block_score;
    int                comb_check_complete;
    int                comb_check_nthreads;

    float              gamma_lut[256];

    int                comb_detect_ready;

    hb_buffer_t      * ref[3];
    int                ref_used[3];

    /* Make buffers to store a comb masks. */
    hb_buffer_t      * mask;
    hb_buffer_t      * mask_filtered;
    hb_buffer_t      * mask_temp;
    int                mask_box_x;
    int                mask_box_y;
    uint8_t            mask_box_color;

    int                cpu_count;
    int                segment_height[3];

    taskset_t          decomb_filter_taskset; // Threads for comb detection
    taskset_t          decomb_check_taskset;  // Threads for comb check
    taskset_t          mask_filter_taskset; // Threads for decomb mask filter
    taskset_t          mask_erode_taskset;  // Threads for decomb mask erode
    taskset_t          mask_dilate_taskset; // Threads for decomb mask dilate

    hb_buffer_list_t   out_list;

    // Filter statistics
    int                comb_heavy;
    int                comb_light;
    int                comb_none;
    int                frames;
};

static int comb_detect_init( hb_filter_object_t * filter,
                             hb_filter_init_t * init );

static int comb_detect_work( hb_filter_object_t * filter,
                             hb_buffer_t ** buf_in,
                             hb_buffer_t ** buf_out );

static void comb_detect_close( hb_filter_object_t * filter );

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

static void draw_mask_box( hb_filter_private_t * pv )
{
    int x = pv->mask_box_x;
    int y = pv->mask_box_y;
    int box_width = pv->block_width;
    int box_height = pv->block_height;
    int stride;
    uint8_t * mskp;

    if (pv->mode & MODE_FILTER)
    {
        mskp   = pv->mask_filtered->plane[0].data;
        stride = pv->mask_filtered->plane[0].stride;
    }
    else
    {
        mskp   = pv->mask->plane[0].data;
        stride = pv->mask->plane[0].stride;
    }


    int block_x, block_y;
    for (block_x = 0; block_x < box_width; block_x++)
    {
        mskp[ y               * stride + x + block_x] = 128;
        mskp[(y + box_height) * stride + x + block_x] = 128;
    }

    for (block_y = 0; block_y < box_height; block_y++)
    {
        mskp[stride * (y + block_y) + x            ] = 128;
        mskp[stride * (y + block_y) + x + box_width] = 128;
    }
}

static void apply_mask_line( uint8_t * srcp,
                             uint8_t * mskp,
                             int width )
{
    int x;

    for (x = 0; x < width; x++)
    {
        if (mskp[x] == 1)
        {
            srcp[x] = 255;
        }
        if (mskp[x] == 128)
        {
            srcp[x] = 128;
        }
    }
}

static void apply_mask(hb_filter_private_t * pv, hb_buffer_t * b)
{
    /* draw_boxes */
    draw_mask_box( pv );

    int pp, yy;
    hb_buffer_t * m;

    if (pv->mode & MODE_FILTER)
    {
        m = pv->mask_filtered;
    }
    else
    {
        m = pv->mask;
    }
    for (pp = 0; pp < 3; pp++)
    {
        uint8_t * dstp = b->plane[pp].data;
        uint8_t * mskp = m->plane[pp].data;

        for (yy = 0; yy < m->plane[pp].height; yy++)
        {
            if (!(pv->mode & MODE_COMPOSITE) && pp == 0)
            {
                memcpy(dstp, mskp, m->plane[pp].width);
            }
            else if (!(pv->mode & MODE_COMPOSITE))
            {
                memset(dstp, 128, m->plane[pp].width);
            }
            if (pp == 0)
            {
                apply_mask_line(dstp, mskp, m->plane[pp].width);
            }

            dstp += b->plane[pp].stride;
            mskp += m->plane[pp].stride;
        }
    }
}

static void store_ref(hb_filter_private_t * pv, hb_buffer_t * b)
{
    // Free unused buffer
    if (!pv->ref_used[0])
    {
        hb_buffer_close(&pv->ref[0]);
    }
    memmove(&pv->ref[0],      &pv->ref[1],      sizeof(pv->ref[0])      * 2 );
    memmove(&pv->ref_used[0], &pv->ref_used[1], sizeof(pv->ref_used[0]) * 2 );
    pv->ref[2]      = b;
    pv->ref_used[2] = 0;
}

static void reset_combing_results( hb_filter_private_t * pv )
{
    pv->comb_check_complete = 0;
    int ii;
    for (ii = 0; ii < pv->comb_check_nthreads; ii++)
    {
       pv->block_score[ii] = 0;
    }
}

static int check_combing_results( hb_filter_private_t * pv )
{
    int combed = HB_COMB_NONE;

    int ii;
    for (ii = 0; ii < pv->comb_check_nthreads; ii++)
    {
        if (pv->block_score[ii] >= ( pv->block_threshold / 2 ))
        {
            if (pv->block_score[ii] <= pv->block_threshold)
            {
                // Indicate light combing for block_score that is between
                // ( pv->block_threshold / 2 ) and pv->block_threshold
                combed = HB_COMB_LIGHT;
                pv->mask_box_color = 2;
            }
            else if (pv->block_score[ii] > pv->block_threshold)
            {
                pv->mask_box_color = 1;
                return HB_COMB_HEAVY;
            }
        }
    }

    return combed;
}

static void check_filtered_combing_mask( hb_filter_private_t * pv, int segment,
                                         int start, int stop )
{
    /* Go through the mask in X*Y blocks. If any of these windows
       have threshold or more combed pixels, consider the whole
       frame to be combed and send it on to be deinterlaced.     */

    /* Block mask threshold -- The number of pixels
       in a block_width * block_height window of
       he mask that need to show combing for the
       whole frame to be seen as such.            */
    int threshold       = pv->block_threshold;
    int block_width     = pv->block_width;
    int block_height    = pv->block_height;
    int block_x, block_y;
    int block_score = 0;
    uint8_t * mask_p;
    int x, y, pp;

    for (pp = 0; pp < 1; pp++)
    {
        int stride = pv->mask_filtered->plane[pp].stride;
        int width = pv->mask_filtered->plane[pp].width;

        pv->mask_box_x = -1;
        pv->mask_box_y = -1;
        pv->mask_box_color = 0;

        for (y = start; y < ( stop - block_height + 1 ); y = y + block_height)
        {
            for (x = 0; x < ( width - block_width ); x = x + block_width)
            {
                block_score = 0;

                for (block_y = 0; block_y < block_height; block_y++)
                {
                    int my = y + block_y;
                    mask_p = &pv->mask_filtered->plane[pp].data[my*stride + x];

                    for (block_x = 0; block_x < block_width; block_x++)
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

                if (block_score >= ( threshold / 2 ))
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
}

static void check_combing_mask( hb_filter_private_t * pv, int segment,
                                int start, int stop )
{
    /* Go through the mask in X*Y blocks. If any of these windows
       have threshold or more combed pixels, consider the whole
       frame to be combed and send it on to be deinterlaced.     */

    /* Block mask threshold -- The number of pixels
       in a block_width * block_height window of
       he mask that need to show combing for the
       whole frame to be seen as such.            */
    int threshold       = pv->block_threshold;
    int block_width     = pv->block_width;
    int block_height    = pv->block_height;
    int block_x, block_y;
    int block_score = 0;
    uint8_t * mask_p;
    int x, y, pp;

    for (pp = 0; pp < 1; pp++)
    {
        int stride = pv->mask->plane[pp].stride;
        int width = pv->mask->plane[pp].width;

        for (y = start; y < (stop - block_height + 1); y = y + block_height)
        {
            for (x = 0; x < (width - block_width); x = x + block_width)
            {
                block_score = 0;

                for (block_y = 0; block_y < block_height; block_y++)
                {
                    int mask_y = y + block_y;
                    mask_p = &pv->mask->plane[pp].data[mask_y * stride + x];

                    for (block_x = 0; block_x < block_width; block_x++)
                    {
                        /* We only want to mark a pixel in a block as combed
                           if the adjacent pixels are as well. Got to
                           handle the sides separately.       */
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

                if (block_score >= ( threshold / 2 ))
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
}

static void build_gamma_lut( hb_filter_private_t * pv )
{
    int i;
    for (i = 0; i < 256; i++)
    {
        pv->gamma_lut[i] = pow( ( (float)i / (float)255 ), 2.2f );
    }
}

static void detect_gamma_combed_segment( hb_filter_private_t * pv,
                                         int segment_start, int segment_stop )
{
    /* A mish-mash of various comb detection tricks
       picked up from neuron2's Decomb plugin for
       AviSynth and tritical's IsCombedT and
       IsCombedTIVTC plugins.                       */

    /* Comb scoring algorithm */
    /* Motion threshold */
    float mthresh         = (float)pv->motion_threshold / (float)255;
    /* Spatial threshold */
    float athresh         = (float)pv->spatial_threshold / (float)255;
    float athresh6        = 6 *athresh;

    /* One pas for Y, one pass for U, one pass for V */
    int pp;
    for (pp = 0; pp < 1; pp++)
    {
        int x, y;
        int stride  = pv->ref[0]->plane[pp].stride;
        int width   = pv->ref[0]->plane[pp].width;
        int height  = pv->ref[0]->plane[pp].height;

        /* Comb detection has to start at y = 2 and end at
           y = height - 2, because it needs to examine
           2 pixels above and 2 below the current pixel.      */
        if (segment_start < 2)
            segment_start = 2;
        if (segment_stop > height - 2)
            segment_stop = height - 2;

        for (y =  segment_start; y < segment_stop; y++)
        {
            /* These are just to make the buffer locations easier to read. */
            int up_2    = -2 * stride ;
            int up_1    = -1 * stride;
            int down_1  =      stride;
            int down_2  =  2 * stride;

            /* We need to examine a column of 5 pixels
               in the prev, cur, and next frames.      */
            uint8_t * prev = &pv->ref[0]->plane[pp].data[y * stride];
            uint8_t * cur  = &pv->ref[1]->plane[pp].data[y * stride];
            uint8_t * next = &pv->ref[2]->plane[pp].data[y * stride];
            uint8_t * mask = &pv->mask->plane[pp].data[y * stride];

            memset(mask, 0, stride);

            for (x = 0; x < width; x++)
            {
                float up_diff, down_diff;
                up_diff   = pv->gamma_lut[cur[0]] - pv->gamma_lut[cur[up_1]];
                down_diff = pv->gamma_lut[cur[0]] - pv->gamma_lut[cur[down_1]];

                if (( up_diff >  athresh && down_diff >  athresh ) ||
                    ( up_diff < -athresh && down_diff < -athresh ))
                {
                    /* The pixel above and below are different,
                       and they change in the same "direction" too.*/
                    int motion = 0;
                    if (mthresh > 0)
                    {
                        /* Make sure there's sufficient motion between frame t-1 to frame t+1. */
                        if (fabs(pv->gamma_lut[prev[0]]     - pv->gamma_lut[cur[0]]      ) > mthresh &&
                            fabs(pv->gamma_lut[cur[up_1]]   - pv->gamma_lut[next[up_1]]  ) > mthresh &&
                            fabs(pv->gamma_lut[cur[down_1]] - pv->gamma_lut[next[down_1]]) > mthresh)
                                motion++;
                        if (fabs(pv->gamma_lut[next[0]]      - pv->gamma_lut[cur[0]]     ) > mthresh &&
                            fabs(pv->gamma_lut[prev[up_1]]   - pv->gamma_lut[cur[up_1]]  ) > mthresh &&
                            fabs(pv->gamma_lut[prev[down_1]] - pv->gamma_lut[cur[down_1]]) > mthresh)
                                motion++;

                    }
                    else
                    {
                        /* User doesn't want to check for motion,
                           so move on to the spatial check.       */
                        motion = 1;
                    }

                    if (motion || pv->frames == 0)
                    {
                        float combing;
                        /* Tritical's noise-resistant combing scorer.
                           The check is done on a bob+blur convolution. */
                        combing = fabs(pv->gamma_lut[cur[up_2]] +
                                       (4 * pv->gamma_lut[cur[0]]) +
                                       pv->gamma_lut[cur[down_2]] -
                                       (3 * (pv->gamma_lut[cur[up_1]] +
                                             pv->gamma_lut[cur[down_1]])));
                        /* If the frame is sufficiently combed,
                           then mark it down on the mask as 1. */
                        if (combing > athresh6)
                        {
                            mask[0] = 1;
                        }
                    }
                }

                cur++;
                prev++;
                next++;
                mask++;
            }
        }
    }
}

static void detect_combed_segment( hb_filter_private_t * pv,
                                   int segment_start, int segment_stop )
{
    /* A mish-mash of various comb detection tricks
       picked up from neuron2's Decomb plugin for
       AviSynth and tritical's IsCombedT and
       IsCombedTIVTC plugins.                       */


    /* Comb scoring algorithm */
    int spatial_metric  = pv->spatial_metric;
    /* Motion threshold */
    int mthresh         = pv->motion_threshold;
    /* Spatial threshold */
    int athresh         = pv->spatial_threshold;
    int athresh_squared = athresh * athresh;
    int athresh6        = 6 * athresh;

    /* One pas for Y, one pass for U, one pass for V */
    int pp;
    for (pp = 0; pp < 1; pp++)
    {
        int x, y;
        int stride  = pv->ref[0]->plane[pp].stride;
        int width   = pv->ref[0]->plane[pp].width;
        int height  = pv->ref[0]->plane[pp].height;

        /* Comb detection has to start at y = 2 and end at
           y = height - 2, because it needs to examine
           2 pixels above and 2 below the current pixel.      */
        if (segment_start < 2)
            segment_start = 2;
        if (segment_stop > height - 2)
            segment_stop = height - 2;

        for (y =  segment_start; y < segment_stop; y++)
        {
            /* These are just to make the buffer locations easier to read. */
            int up_2    = -2 * stride ;
            int up_1    = -1 * stride;
            int down_1  =      stride;
            int down_2  =  2 * stride;

            /* We need to examine a column of 5 pixels
               in the prev, cur, and next frames.      */
            uint8_t * prev = &pv->ref[0]->plane[pp].data[y * stride];
            uint8_t * cur  = &pv->ref[1]->plane[pp].data[y * stride];
            uint8_t * next = &pv->ref[2]->plane[pp].data[y * stride];
            uint8_t * mask = &pv->mask->plane[pp].data[y * stride];

            memset(mask, 0, stride);

            for (x = 0; x < width; x++)
            {
                int up_diff = cur[0] - cur[up_1];
                int down_diff = cur[0] - cur[down_1];

                if (( up_diff >  athresh && down_diff >  athresh ) ||
                    ( up_diff < -athresh && down_diff < -athresh ))
                {
                    /* The pixel above and below are different,
                       and they change in the same "direction" too.*/
                    int motion = 0;
                    if (mthresh > 0)
                    {
                        /* Make sure there's sufficient motion between frame t-1 to frame t+1. */
                        if (abs(prev[0]     - cur[0]      ) > mthresh &&
                            abs(cur[up_1]   - next[up_1]  ) > mthresh &&
                            abs(cur[down_1] - next[down_1]) > mthresh)
                                motion++;
                        if (abs(next[0]      - cur[0]     ) > mthresh &&
                            abs(prev[up_1]   - cur[up_1]  ) > mthresh &&
                            abs(prev[down_1] - cur[down_1]) > mthresh)
                                motion++;
                    }
                    else
                    {
                        /* User doesn't want to check for motion,
                           so move on to the spatial check.       */
                        motion = 1;
                    }

                    // If motion, or we can't measure motion yet...
                    if (motion || pv->frames == 0)
                    {
                           /* That means it's time for the spatial check.
                              We've got several options here.             */
                        if (spatial_metric == 0)
                        {
                            /* Simple 32detect style comb detection */
                            if ((abs(cur[0] - cur[down_2]) < 10) &&
                                (abs(cur[0] - cur[down_1]) > 15))
                            {
                                mask[0] = 1;
                            }
                        }
                        else if (spatial_metric == 1)
                        {
                            /* This, for comparison, is what IsCombed uses.
                               It's better, but still noise sensitive.      */
                               int combing = ( cur[up_1] - cur[0] ) *
                                             ( cur[down_1] - cur[0] );

                               if (combing > athresh_squared)
                               {
                                   mask[0] = 1;
                               }
                        }
                        else if (spatial_metric == 2)
                        {
                            /* Tritical's noise-resistant combing scorer.
                               The check is done on a bob+blur convolution. */
                            int combing = abs( cur[up_2]
                                             + ( 4 * cur[0] )
                                             + cur[down_2]
                                             - ( 3 * ( cur[up_1]
                                                     + cur[down_1] ) ) );

                            /* If the frame is sufficiently combed,
                               then mark it down on the mask as 1. */
                            if (combing > athresh6)
                            {
                                mask[0] = 1;
                            }
                        }
                    }
                }

                cur++;
                prev++;
                next++;
                mask++;
            }
        }
    }
}

static void mask_dilate_thread( void *thread_args_v )
{
    hb_filter_private_t * pv;
    int segment, segment_start, segment_stop;
    decomb_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_deep_log(3, "mask dilate thread started for segment %d", segment);

    while (1)
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->mask_dilate_taskset, segment );

        if (taskset_thread_stop(&pv->mask_dilate_taskset, segment))
        {
            /*
             * No more work to do, exit this thread.
             */
            break;
        }

        int xx, yy, pp;

        int count;
        int dilation_threshold = 4;

        for (pp = 0; pp < 1; pp++)
        {
            int width = pv->mask_filtered->plane[pp].width;
            int height = pv->mask_filtered->plane[pp].height;
            int stride = pv->mask_filtered->plane[pp].stride;

            int start, stop, p, c, n;
            segment_start = thread_args->segment_start[pp];
            segment_stop = segment_start + thread_args->segment_height[pp];

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

            uint8_t *curp = &pv->mask_filtered->plane[pp].data[p * stride + 1];
            uint8_t *cur  = &pv->mask_filtered->plane[pp].data[c * stride + 1];
            uint8_t *curn = &pv->mask_filtered->plane[pp].data[n * stride + 1];
            uint8_t *dst = &pv->mask_temp->plane[pp].data[c * stride + 1];

            for (yy = start; yy < stop; yy++)
            {
                for (xx = 1; xx < width - 1; xx++)
                {
                    if (cur[xx])
                    {
                        dst[xx] = 1;
                        continue;
                    }

                    count = curp[xx-1] + curp[xx] + curp[xx+1] +
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

        taskset_thread_complete( &pv->mask_dilate_taskset, segment );
    }

    /*
     * Finished this segment, let everyone know.
     */
    taskset_thread_complete( &pv->mask_dilate_taskset, segment );
}

static void mask_erode_thread( void *thread_args_v )
{
    hb_filter_private_t * pv;
    int segment, segment_start, segment_stop;
    decomb_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_deep_log(3, "mask erode thread started for segment %d", segment);

    while (1)
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->mask_erode_taskset, segment );

        if (taskset_thread_stop( &pv->mask_erode_taskset, segment ))
        {
            /*
             * No more work to do, exit this thread.
             */
            break;
        }

        int xx, yy, pp;

        int count;
        int erosion_threshold = 2;

        for (pp = 0; pp < 1; pp++)
        {
            int width = pv->mask_filtered->plane[pp].width;
            int height = pv->mask_filtered->plane[pp].height;
            int stride = pv->mask_filtered->plane[pp].stride;

            int start, stop, p, c, n;
            segment_start = thread_args->segment_start[pp];
            segment_stop = segment_start + thread_args->segment_height[pp];

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

            uint8_t *curp = &pv->mask_temp->plane[pp].data[p * stride + 1];
            uint8_t *cur  = &pv->mask_temp->plane[pp].data[c * stride + 1];
            uint8_t *curn = &pv->mask_temp->plane[pp].data[n * stride + 1];
            uint8_t *dst = &pv->mask_filtered->plane[pp].data[c * stride + 1];

            for (yy = start; yy < stop; yy++)
            {
                for (xx = 1; xx < width - 1; xx++)
                {
                    if (cur[xx] == 0)
                    {
                        dst[xx] = 0;
                        continue;
                    }

                    count = curp[xx-1] + curp[xx] + curp[xx+1] +
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

        taskset_thread_complete( &pv->mask_erode_taskset, segment );
    }

    /*
     * Finished this segment, let everyone know.
     */
    taskset_thread_complete( &pv->mask_erode_taskset, segment );
}

static void mask_filter_thread( void *thread_args_v )
{
    hb_filter_private_t * pv;
    int segment, segment_start, segment_stop;
    decomb_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_deep_log(3, "mask filter thread started for segment %d", segment);

    while (1)
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->mask_filter_taskset, segment );

        if (taskset_thread_stop( &pv->mask_filter_taskset, segment ))
        {
            /*
             * No more work to do, exit this thread.
             */
            break;
        }

        int xx, yy, pp;

        for (pp = 0; pp < 1; pp++)
        {
            int width = pv->mask->plane[pp].width;
            int height = pv->mask->plane[pp].height;
            int stride = pv->mask->plane[pp].stride;

            int start, stop, p, c, n;
            segment_start = thread_args->segment_start[pp];
            segment_stop = segment_start + thread_args->segment_height[pp];

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

            uint8_t *curp = &pv->mask->plane[pp].data[p * stride + 1];
            uint8_t *cur = &pv->mask->plane[pp].data[c * stride + 1];
            uint8_t *curn = &pv->mask->plane[pp].data[n * stride + 1];
            uint8_t *dst = (pv->filter_mode == FILTER_CLASSIC ) ?
                &pv->mask_filtered->plane[pp].data[c * stride + 1] :
                &pv->mask_temp->plane[pp].data[c * stride + 1] ;

            for (yy = start; yy < stop; yy++)
            {
                for (xx = 1; xx < width - 1; xx++)
                {
                    int h_count, v_count;

                    h_count = cur[xx-1] & cur[xx] & cur[xx+1];
                    v_count = curp[xx] & cur[xx] & curn[xx];

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

        taskset_thread_complete( &pv->mask_filter_taskset, segment );
    }

    /*
     * Finished this segment, let everyone know.
     */
    taskset_thread_complete( &pv->mask_filter_taskset, segment );
}

static void decomb_check_thread( void *thread_args_v )
{
    hb_filter_private_t * pv;
    int segment, segment_start, segment_stop;
    decomb_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_deep_log(3, "decomb check thread started for segment %d", segment);

    while (1)
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->decomb_check_taskset, segment );

        if (taskset_thread_stop( &pv->decomb_check_taskset, segment ))
        {
            /*
             * No more work to do, exit this thread.
             */
            break;
        }

        segment_start = thread_args->segment_start[0];
        segment_stop = segment_start + thread_args->segment_height[0];

        if (pv->mode & MODE_FILTER)
        {
            check_filtered_combing_mask(pv, segment, segment_start, segment_stop);
        }
        else
        {
            check_combing_mask(pv, segment, segment_start, segment_stop);
        }

        taskset_thread_complete( &pv->decomb_check_taskset, segment );
    }

    /*
     * Finished this segment, let everyone know.
     */
    taskset_thread_complete( &pv->decomb_check_taskset, segment );
}

/*
 * comb detect this segment of all three planes in a single thread.
 */
static void decomb_filter_thread( void *thread_args_v )
{
    hb_filter_private_t * pv;
    int segment, segment_start, segment_stop;
    decomb_thread_arg_t *thread_args = thread_args_v;

    pv = thread_args->pv;
    segment = thread_args->segment;

    hb_deep_log(3, "decomb filter thread started for segment %d", segment);

    while (1)
    {
        /*
         * Wait here until there is work to do.
         */
        taskset_thread_wait4start( &pv->decomb_filter_taskset, segment );

        if (taskset_thread_stop( &pv->decomb_filter_taskset, segment ))
        {
            /*
             * No more work to do, exit this thread.
             */
            break;
        }

        /*
         * Process segment (for now just from luma)
         */
        int pp;
        for (pp = 0; pp < 1; pp++)
        {
            segment_start = thread_args->segment_start[pp];
            segment_stop = segment_start + thread_args->segment_height[pp];

            if (pv->mode & MODE_GAMMA)
            {
                detect_gamma_combed_segment( pv, segment_start, segment_stop );
            }
            else
            {
                detect_combed_segment( pv, segment_start, segment_stop );
            }
        }

        taskset_thread_complete( &pv->decomb_filter_taskset, segment );
    }

    /*
     * Finished this segment, let everyone know.
     */
    taskset_thread_complete( &pv->decomb_filter_taskset, segment );
}

static int comb_segmenter( hb_filter_private_t * pv )
{
    /*
     * Now that all data for decomb detection is ready for
     * our threads, fire them off and wait for their completion.
     */
    taskset_cycle( &pv->decomb_filter_taskset );

    if (pv->mode & MODE_FILTER)
    {
        taskset_cycle( &pv->mask_filter_taskset );
        if (pv->filter_mode == FILTER_ERODE_DILATE)
        {
            taskset_cycle( &pv->mask_erode_taskset );
            taskset_cycle( &pv->mask_dilate_taskset );
            taskset_cycle( &pv->mask_erode_taskset );
        }
    }
    reset_combing_results(pv);
    taskset_cycle(&pv->decomb_check_taskset);
    return check_combing_results(pv);
}

static int comb_detect_init( hb_filter_object_t * filter,
                             hb_filter_init_t   * init )
{
    filter->private_data = calloc( 1, sizeof(struct hb_filter_private_s) );
    hb_filter_private_t * pv = filter->private_data;

    hb_buffer_list_clear(&pv->out_list);
    build_gamma_lut( pv );

    pv->frames = 0;
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
        hb_value_t * dict = filter->settings;

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

    pv->cpu_count = hb_get_cpu_count();

    // Make segment sizes an even number of lines
    int height = hb_image_height(init->pix_fmt, init->geometry.height, 0);
    // each segment of each plane must begin on an even row.
    pv->segment_height[0] = (height / pv->cpu_count) & ~3;
    pv->segment_height[1] = hb_image_height(init->pix_fmt, pv->segment_height[0], 1);
    pv->segment_height[2] = hb_image_height(init->pix_fmt, pv->segment_height[0], 2);

    /* Allocate buffers to store comb masks. */
    pv->mask = hb_frame_buffer_init(init->pix_fmt,
                                init->geometry.width, init->geometry.height);
    pv->mask_filtered = hb_frame_buffer_init(init->pix_fmt,
                                init->geometry.width, init->geometry.height);
    pv->mask_temp = hb_frame_buffer_init(init->pix_fmt,
                                init->geometry.width, init->geometry.height);
    memset(pv->mask->data, 0, pv->mask->size);
    memset(pv->mask_filtered->data, 0, pv->mask_filtered->size);
    memset(pv->mask_temp->data, 0, pv->mask_temp->size);

    int ii;

    /*
     * Create comb detection taskset.
     */
    if (taskset_init( &pv->decomb_filter_taskset, pv->cpu_count,
                      sizeof( decomb_thread_arg_t ) ) == 0)
    {
        hb_error( "decomb could not initialize taskset" );
    }

    decomb_thread_arg_t *decomb_prev_thread_args = NULL;
    for (ii = 0; ii < pv->cpu_count; ii++)
    {
        decomb_thread_arg_t *thread_args;

        thread_args = taskset_thread_args( &pv->decomb_filter_taskset, ii );
        thread_args->pv = pv;
        thread_args->segment = ii;

        int pp;
        for (pp = 0; pp < 3; pp++)
        {
            if (decomb_prev_thread_args != NULL)
            {
                thread_args->segment_start[pp] =
                    decomb_prev_thread_args->segment_start[pp] +
                    decomb_prev_thread_args->segment_height[pp];
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

        if (taskset_thread_spawn( &pv->decomb_filter_taskset, ii,
                                 "decomb_filter_segment",
                                 decomb_filter_thread,
                                 HB_NORMAL_PRIORITY ) == 0)
        {
            hb_error( "decomb could not spawn thread" );
        }

        decomb_prev_thread_args = thread_args;
    }

    pv->comb_check_nthreads = init->geometry.height / pv->block_height;

    if (pv->comb_check_nthreads > pv->cpu_count)
        pv->comb_check_nthreads = pv->cpu_count;

    pv->block_score = calloc(pv->comb_check_nthreads, sizeof(int));

    /*
     * Create comb check taskset.
     */
    if (taskset_init( &pv->decomb_check_taskset, pv->comb_check_nthreads,
                      sizeof( decomb_thread_arg_t ) ) == 0)
    {
        hb_error( "decomb check could not initialize taskset" );
    }

    decomb_prev_thread_args = NULL;
    for (ii = 0; ii < pv->comb_check_nthreads; ii++)
    {
        decomb_thread_arg_t *thread_args;

        thread_args = taskset_thread_args( &pv->decomb_check_taskset, ii);
        thread_args->pv = pv;
        thread_args->segment = ii;

        int pp;
        for (pp = 0; pp < 3; pp++)
        {
            if (decomb_prev_thread_args != NULL)
            {
                thread_args->segment_start[pp] =
                    decomb_prev_thread_args->segment_start[pp] +
                    decomb_prev_thread_args->segment_height[pp];
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

        if (taskset_thread_spawn( &pv->decomb_check_taskset, ii,
                                  "decomb_check_segment",
                                  decomb_check_thread,
                                  HB_NORMAL_PRIORITY ) == 0)
        {
            hb_error( "decomb check could not spawn thread" );
        }

        decomb_prev_thread_args = thread_args;
    }

    if (pv->mode & MODE_FILTER)
    {
        if (taskset_init( &pv->mask_filter_taskset, pv->cpu_count,
                          sizeof( decomb_thread_arg_t ) ) == 0)
        {
            hb_error( "mask filter could not initialize taskset" );
        }

        decomb_prev_thread_args = NULL;
        for (ii = 0; ii < pv->cpu_count; ii++)
        {
            decomb_thread_arg_t *thread_args;

            thread_args = taskset_thread_args( &pv->mask_filter_taskset, ii );
            thread_args->pv = pv;
            thread_args->segment = ii;

            int pp;
            for (pp = 0; pp < 3; pp++)
            {
                if (decomb_prev_thread_args != NULL)
                {
                    thread_args->segment_start[pp] =
                        decomb_prev_thread_args->segment_start[pp] +
                        decomb_prev_thread_args->segment_height[pp];
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

            if (taskset_thread_spawn( &pv->mask_filter_taskset, ii,
                                     "mask_filter_segment",
                                     mask_filter_thread,
                                     HB_NORMAL_PRIORITY ) == 0)
            {
                hb_error( "mask filter could not spawn thread" );
            }

            decomb_prev_thread_args = thread_args;
        }

        if (pv->filter_mode == FILTER_ERODE_DILATE)
        {
            if (taskset_init( &pv->mask_erode_taskset, pv->cpu_count,
                              sizeof( decomb_thread_arg_t ) ) == 0)
            {
                hb_error( "mask erode could not initialize taskset" );
            }

            decomb_prev_thread_args = NULL;
            for (ii = 0; ii < pv->cpu_count; ii++)
            {
                decomb_thread_arg_t *thread_args;

                thread_args = taskset_thread_args( &pv->mask_erode_taskset, ii );
                thread_args->pv = pv;
                thread_args->segment = ii;

                int pp;
                for (pp = 0; pp < 3; pp++)
                {
                    if (decomb_prev_thread_args != NULL)
                    {
                        thread_args->segment_start[pp] =
                            decomb_prev_thread_args->segment_start[pp] +
                            decomb_prev_thread_args->segment_height[pp];
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

                if (taskset_thread_spawn( &pv->mask_erode_taskset, ii,
                                         "mask_erode_segment",
                                         mask_erode_thread,
                                         HB_NORMAL_PRIORITY ) == 0)
                {
                    hb_error( "mask erode could not spawn thread" );
                }

                decomb_prev_thread_args = thread_args;
            }

            if (taskset_init( &pv->mask_dilate_taskset, pv->cpu_count,
                              sizeof( decomb_thread_arg_t ) ) == 0)
            {
                hb_error( "mask dilate could not initialize taskset" );
            }

            decomb_prev_thread_args = NULL;
            for (ii = 0; ii < pv->cpu_count; ii++)
            {
                decomb_thread_arg_t *thread_args;

                thread_args = taskset_thread_args( &pv->mask_dilate_taskset, ii );
                thread_args->pv = pv;
                thread_args->segment = ii;

                int pp;
                for (pp = 0; pp < 3; pp++)
                {
                    if (decomb_prev_thread_args != NULL)
                    {
                        thread_args->segment_start[pp] =
                            decomb_prev_thread_args->segment_start[pp] +
                            decomb_prev_thread_args->segment_height[pp];
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

                if (taskset_thread_spawn( &pv->mask_dilate_taskset, ii,
                                         "mask_dilate_segment",
                                         mask_dilate_thread,
                                         HB_NORMAL_PRIORITY ) == 0)
                {
                    hb_error( "mask dilate could not spawn thread" );
                }

                decomb_prev_thread_args = thread_args;
            }
        }
    }

    return 0;
}

static void comb_detect_close( hb_filter_object_t * filter )
{
    hb_filter_private_t * pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    hb_log("comb detect: heavy %i | light %i | uncombed %i | total %i",
           pv->comb_heavy,  pv->comb_light,  pv->comb_none, pv->frames);

    taskset_fini( &pv->decomb_filter_taskset );
    taskset_fini( &pv->decomb_check_taskset );

    if (pv->mode & MODE_FILTER)
    {
        taskset_fini( &pv->mask_filter_taskset );
        if (pv->filter_mode == FILTER_ERODE_DILATE)
        {
            taskset_fini( &pv->mask_erode_taskset );
            taskset_fini( &pv->mask_dilate_taskset );
        }
    }

    /* Cleanup reference buffers. */
    int ii;
    for (ii = 0; ii < 3; ii++)
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

    free(pv->block_score);
    free( pv );
    filter->private_data = NULL;
}

static void process_frame( hb_filter_private_t * pv )
{
    int combed;

    combed = comb_segmenter(pv);
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
    if ((pv->mode & MODE_MASK) && combed)
    {
        hb_buffer_t * out;
        out = hb_buffer_dup(pv->ref[1]);
        apply_mask(pv, out);
        out->s.combed = combed;
        hb_buffer_list_append(&pv->out_list, out);
    }
    else
    {
        pv->ref_used[1] = 1;
        pv->ref[1]->s.combed = combed;
        hb_buffer_list_append(&pv->out_list, pv->ref[1]);
    }
}

static int comb_detect_work( hb_filter_object_t * filter,
                             hb_buffer_t ** buf_in,
                             hb_buffer_t ** buf_out )
{
    hb_filter_private_t * pv = filter->private_data;
    hb_buffer_t         * in = *buf_in;

    // Input buffer is always consumed.
    *buf_in = NULL;
    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        // Duplicate last frame and process refs
        store_ref(pv, hb_buffer_dup(pv->ref[2]));
        if (pv->ref[0] != NULL)
        {
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
