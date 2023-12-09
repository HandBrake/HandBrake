/* comb_detect_template.c

   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#if BIT_DEPTH > 8
#   define pixel  uint16_t
#   define FUNC(name) name##_##16
#else
#   define pixel  uint8_t
#   define FUNC(name) name##_##8
#endif

static inline void FUNC(draw_mask_box)(hb_filter_private_t *pv)
{
    const int x = pv->mask_box_x;
    const int y = pv->mask_box_y;
    const int box_width = pv->block_width;
    const int box_height = pv->block_height;

    int stride;
    uint8_t *mskp;

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

    for (int block_x = 0; block_x < box_width; block_x++)
    {
        mskp[ y               * stride + x + block_x] = 128;
        mskp[(y + box_height) * stride + x + block_x] = 128;
    }

    for (int block_y = 0; block_y < box_height; block_y++)
    {
        mskp[stride * (y + block_y) + x            ] = 128;
        mskp[stride * (y + block_y) + x + box_width] = 128;
    }
}

static inline void FUNC(apply_mask_line)(pixel *srcp,
                                         const uint8_t *mskp,
                                         const int width,
                                         const int max,
                                         const int half)
{
    for (int x = 0; x < width; x++)
    {
        if (mskp[x] == 1)
        {
            srcp[x] = max;
        }
        else if (mskp[x] == 128)
        {
            srcp[x] = half;
        }
    }
}

static void FUNC(apply_mask)(hb_filter_private_t *pv, hb_buffer_t *b)
{
    // Draw boxes
    FUNC(draw_mask_box)(pv);

    const hb_buffer_t *m;
    const int max = pv->max_value;
    const int half = pv->half_value;

    if (pv->mode & MODE_FILTER)
    {
        m = pv->mask_filtered;
    }
    else
    {
        m = pv->mask;
    }

    for (int pp = 0; pp < 3; pp++)
    {
        pixel *dstp = (pixel *)b->plane[pp].data;
        const int dstp_stride = b->plane[pp].stride / pv->bps;
        const int width  = m->plane[pp].width;
        const int height = m->plane[pp].height;

        if (!(pv->mode & MODE_COMPOSITE))
        {
            if (pp == 0)
            {
                memset(dstp, 0, b->plane[pp].size);
            }
            else
            {
                if (pv->depth == 8)
                {
                    memset(dstp, half, b->plane[pp].size);
                }
                else
                {
                    for (int i = 0; i < b->plane[pp].size / pv->bps; i++)
                    {
                        dstp[i] = half;
                    }
                }
            }
        }

        if (pp == 0)
        {
            const uint8_t *mskp = m->plane[0].data;
            const int mskp_stride = m->plane[0].stride;

            for (int yy = 0; yy < height; yy++)
            {
                FUNC(apply_mask_line)(dstp, mskp, width, max, half);

                dstp += dstp_stride;
                mskp += mskp_stride;
            }
        }
    }
}

static void FUNC(detect_gamma_combed_segment)(hb_filter_private_t *pv,
                                              int segment_start, int segment_stop)
{
    // A mishmash of various comb detection tricks
    // picked up from neuron2's Decomb plugin for
    // AviSynth and tritical's IsCombedT and
    // IsCombedTIVTC plugins.

    // Comb scoring algorithm
    const float mthresh  = pv->gamma_motion_threshold;
    const float athresh  = pv->gamma_spatial_threshold;
    const float athresh6 = pv->gamma_spatial_threshold6;

    // One pass for Y
    const int stride_prev  = pv->ref[0]->plane[0].stride / pv->bps;
    const int stride_cur   = pv->ref[1]->plane[0].stride / pv->bps;
    const int stride_next  = pv->ref[2]->plane[0].stride / pv->bps;
    const int width   = pv->ref[0]->plane[0].width;
    const int height  = pv->ref[0]->plane[0].height;
    const int mask_stride = pv->mask->plane[0].stride;

    // Comb detection has to start at y = 2 and end at
    // y = height - 2, because it needs to examine
    // 2 pixels above and 2 below the current pixel.
    if (segment_start < 2)
    {
        segment_start = 2;
    }
    if (segment_stop > height - 2)
    {
        segment_stop = height - 2;
    }

    // These are just to make the buffer locations easier to read.
    const int up_1_prev    = -1 * stride_prev;
    const int down_1_prev  =      stride_prev;

    const int up_2    = -2 * stride_cur;
    const int up_1    = -1 * stride_cur;
    const int down_1  =      stride_cur;
    const int down_2  =  2 * stride_cur;

    const int up_1_next    = -1 * stride_next;
    const int down_1_next =       stride_next;

    for (int y = segment_start; y < segment_stop; y++)
    {
        // We need to examine a column of 5 pixels
        // in the prev, cur, and next frames.
        const pixel *prev = &((const pixel *)pv->ref[0]->plane[0].data)[y * stride_prev];
        const pixel *cur  = &((const pixel *)pv->ref[1]->plane[0].data)[y * stride_cur];
        const pixel *next = &((const pixel *)pv->ref[2]->plane[0].data)[y * stride_next];
        uint8_t *mask = &pv->mask->plane[0].data[y * mask_stride];

        memset(mask, 0, mask_stride);

        for (int x = 0; x < width; x++)
        {
            const float up_diff    = pv->gamma_lut[cur[0]] - pv->gamma_lut[cur[up_1]];
            const float down_diff  = pv->gamma_lut[cur[0]] - pv->gamma_lut[cur[down_1]];

            if ((up_diff >  athresh && down_diff >  athresh) ||
                (up_diff < -athresh && down_diff < -athresh))
            {
                // The pixel above and below are different,
                // and they change in the same "direction" too.
                int motion = 0;
                if (mthresh > 0)
                {
                    // Make sure there's sufficient motion between frame t-1 to frame t+1.
                    if (fabs(pv->gamma_lut[prev[0]]     - pv->gamma_lut[cur[0]]           ) > mthresh &&
                        fabs(pv->gamma_lut[cur[up_1]]   - pv->gamma_lut[next[up_1_next]]  ) > mthresh &&
                        fabs(pv->gamma_lut[cur[down_1]] - pv->gamma_lut[next[down_1_next]]) > mthresh)
                    {
                        motion++;
                    }
                    if (fabs(pv->gamma_lut[next[0]]           - pv->gamma_lut[cur[0]]     ) > mthresh &&
                        fabs(pv->gamma_lut[prev[up_1_prev]]   - pv->gamma_lut[cur[up_1]]  ) > mthresh &&
                        fabs(pv->gamma_lut[prev[down_1_prev]] - pv->gamma_lut[cur[down_1]]) > mthresh)
                    {
                        motion++;
                    }
                }
                else
                {
                    // User doesn't want to check for motion,
                    // so move on to the spatial check.
                    motion = 1;
                }

                if (motion || pv->force_exaustive_check)
                {
                    // Tritical's noise-resistant combing scorer.
                    // The check is done on a bob+blur convolution.
                    float combing = fabs(pv->gamma_lut[cur[up_2]] +
                                         (4 * pv->gamma_lut[cur[0]]) +
                                         pv->gamma_lut[cur[down_2]] -
                                         (3 * (pv->gamma_lut[cur[up_1]] +
                                               pv->gamma_lut[cur[down_1]])));
                    // If the frame is sufficiently combed,
                    // then mark it down on the mask as 1.
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

static void FUNC(detect_combed_segment)(hb_filter_private_t *pv,
                                        int segment_start, int segment_stop)
{
    // A mishmash of various comb detection tricks
    // picked up from neuron2's Decomb plugin for
    // AviSynth and tritical's IsCombedT and
    // IsCombedTIVTC plugins.

    // Comb scoring algorithm
    const int spatial_metric  = pv->spatial_metric;
    const int mthresh         = pv->motion_threshold;
    const int athresh         = pv->spatial_threshold;
    const int athresh_squared = pv->spatial_threshold_squared;
    const int athresh6        = pv->spatial_threshold6;

    // One pass for Y
    const int stride_prev  = pv->ref[0]->plane[0].stride / pv->bps;
    const int stride_cur   = pv->ref[1]->plane[0].stride / pv->bps;
    const int stride_next  = pv->ref[2]->plane[0].stride / pv->bps;
    const int width   = pv->ref[0]->plane[0].width;
    const int height  = pv->ref[0]->plane[0].height;
    const int mask_stride = pv->mask->plane[0].stride;

    // Comb detection has to start at y = 2 and end at
    // y = height - 2, because it needs to examine
    // 2 pixels above and 2 below the current pixel.
    if (segment_start < 2)
    {
        segment_start = 2;
    }
    if (segment_stop > height - 2)
    {
        segment_stop = height - 2;
    }

    // These are just to make the buffer locations easier to read.
    const int up_1_prev    = -1 * stride_prev;
    const int down_1_prev  =      stride_prev;

    const int up_2    = -2 * stride_cur;
    const int up_1    = -1 * stride_cur;
    const int down_1  =      stride_cur;
    const int down_2  =  2 * stride_cur;

    const int up_1_next    = -1 * stride_next;
    const int down_1_next =       stride_next;

    for (int y = segment_start; y < segment_stop; y++)
    {
        // We need to examine a column of 5 pixels
        // in the prev, cur, and next frames.
        const pixel *prev = &((const pixel *)pv->ref[0]->plane[0].data)[y * stride_prev];
        const pixel *cur  = &((const pixel *)pv->ref[1]->plane[0].data)[y * stride_cur];
        const pixel *next = &((const pixel *)pv->ref[2]->plane[0].data)[y * stride_next];
        uint8_t *mask = &pv->mask->plane[0].data[y * mask_stride];

        memset(mask, 0, mask_stride);

        for (int x = 0; x < width; x++)
        {
            const int up_diff = cur[0] - cur[up_1];
            const int down_diff = cur[0] - cur[down_1];

            if ((up_diff >  athresh && down_diff >  athresh) ||
                (up_diff < -athresh && down_diff < -athresh))
            {
                // The pixel above and below are different,
                // and they change in the same "direction" too.
                int motion = 0;
                if (mthresh > 0)
                {
                    // Make sure there's sufficient motion between frame t-1 to frame t+1.
                    if (abs(prev[0]     - cur[0]           ) > mthresh &&
                        abs(cur[up_1]   - next[up_1_next]  ) > mthresh &&
                        abs(cur[down_1] - next[down_1_next]) > mthresh)
                    {
                        motion++;
                    }
                    if (abs(next[0]           - cur[0]     ) > mthresh &&
                        abs(prev[up_1_prev]   - cur[up_1]  ) > mthresh &&
                        abs(prev[down_1_prev] - cur[down_1]) > mthresh)
                    {
                        motion++;
                    }
                }
                else
                {
                    // User doesn't want to check for motion,
                    // so move on to the spatial check.
                    motion = 1;
                }

                // If motion, or we can't measure motion yet...
                if (motion || pv->force_exaustive_check)
                {
                    // That means it's time for the spatial check.
                    // We've got several options here.
                    if (spatial_metric == 0)
                    {
                        // Simple 32detect style comb detection.
                        if ((abs(cur[0] - cur[down_2]) < pv->comb32detect_min) &&
                            (abs(cur[0] - cur[down_1]) > pv->comb32detect_max))
                        {
                            mask[0] = 1;
                        }
                    }
                    else if (spatial_metric == 1)
                    {
                        // This, for comparison, is what IsCombed uses.
                        // It's better, but still noise sensitive.
                        const int combing = (cur[up_1] - cur[0]) *
                                            (cur[down_1] - cur[0]);

                        if (combing > athresh_squared)
                        {
                            mask[0] = 1;
                        }
                    }
                    else if (spatial_metric == 2)
                    {
                        // Tritical's noise-resistant combing scorer.
                        // The check is done on a bob+blur convolution.
                        const int combing = abs( cur[up_2]
                                            + ( 4 * cur[0] )
                                            + cur[down_2]
                                            - ( 3 * ( cur[up_1]
                                                     + cur[down_1] ) ) );

                        // If the frame is sufficiently combed,
                        // then mark it down on the mask as 1.
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

#undef pixel
#undef FUNC
