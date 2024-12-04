/* comb_detect_template.c

   Copyright (c) 2003-2024 HandBrake Team
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

#if defined (__aarch64__) && !defined(__APPLE__)
    #include <arm_neon.h>
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

#if defined (__aarch64__) && !defined(__APPLE__)
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

    float32x4_t v_athresh = vdupq_n_f32(athresh);
    float32x4_t v_athresh_neg = vdupq_n_f32(-athresh);
    float32x4_t v_mthresh = vdupq_n_f32(mthresh);
    float32x4_t v_athresh6 = vdupq_n_f32(athresh6);
    float32x4_t v_four = vdupq_n_f32(4.0f);
    float32x4_t v_three = vdupq_n_f32(3.0f);
    uint32x4_t v_one = vdupq_n_u32(1);
    uint32x4_t v_exhaustive_check = vdupq_n_u32(pv->force_exaustive_check);

    for (int y = segment_start; y < segment_stop; y++)
    {
        // We need to examine a column of 5 pixels
        // in the prev, cur, and next frames.
        const pixel *prev = &((const pixel *)pv->ref[0]->plane[0].data)[y * stride_prev];
        const pixel *cur  = &((const pixel *)pv->ref[1]->plane[0].data)[y * stride_cur];
        const pixel *next = &((const pixel *)pv->ref[2]->plane[0].data)[y * stride_next];
        uint8_t *mask = &pv->mask->plane[0].data[y * mask_stride];
        memset(mask, 0, mask_stride);
        uint32_t mask32[4];

        for (int x = 0; x < width; x += 4)
        {
            uint32x4_t mask_vec = vdupq_n_u32(0);

            float32x4_t cur_vec = {pv->gamma_lut[cur[0]], pv->gamma_lut[cur[1]], pv->gamma_lut[cur[2]], pv->gamma_lut[cur[3]]};
            float32x4_t cur_up1_vec = {pv->gamma_lut[cur[up_1 + 0]], pv->gamma_lut[cur[up_1 + 1]], pv->gamma_lut[cur[up_1 + 2]], pv->gamma_lut[cur[up_1 + 3]]};
            float32x4_t cur_down1_vec = {pv->gamma_lut[cur[down_1 + 0]], pv->gamma_lut[cur[down_1 + 1]], pv->gamma_lut[cur[down_1 + 2]], pv->gamma_lut[cur[down_1 + 3]]};

            float32x4_t up_diff1 = vsubq_f32(cur_vec, cur_up1_vec);
            float32x4_t down_diff1 = vsubq_f32(cur_vec, cur_down1_vec);

            uint32x4_t cond1 = vcgtq_f32(up_diff1, v_athresh);
            uint32x4_t cond2 = vcgtq_f32(down_diff1, v_athresh);
            uint32x4_t cond3 = vcltq_f32(up_diff1, v_athresh_neg);
            uint32x4_t cond4 = vcltq_f32(down_diff1, v_athresh_neg);

            uint32x4_t condition1 = vandq_u32(cond1, cond2);
            uint32x4_t condition2 = vandq_u32(cond3, cond4);
            uint32x4_t condition = vorrq_u32(condition1, condition2);

            if(vmaxvq_u32(condition) > 0)
            {
                uint32x4_t motion = vdupq_n_u32(0);
                uint32x4_t motion1 = vdupq_n_u32(0);
                if (mthresh > 0)
                {
                    float32x4_t prev_vec = {pv->gamma_lut[prev[0]], pv->gamma_lut[prev[1]], pv->gamma_lut[prev[2]], pv->gamma_lut[prev[3]]};
                    float32x4_t next_vec = {pv->gamma_lut[next[0]], pv->gamma_lut[next[1]], pv->gamma_lut[next[2]], pv->gamma_lut[next[3]]};
                    float32x4_t next_up_1_vec =  {pv->gamma_lut[next[up_1_next + 0]], pv->gamma_lut[next[up_1_next + 1]], pv->gamma_lut[next[up_1_next + 2]], pv->gamma_lut[next[up_1_next + 3]]};
                    float32x4_t next_down_1_vec =  {pv->gamma_lut[next[down_1_next + 0]], pv->gamma_lut[next[down_1_next + 1]], pv->gamma_lut[next[down_1_next + 2]], pv->gamma_lut[next[down_1_next + 3]]};
                    float32x4_t abs_diff1 = vabsq_f32(vsubq_f32(prev_vec, cur_vec));
                    float32x4_t abs_diff2 = vabsq_f32(vsubq_f32(cur_up1_vec, next_up_1_vec));
                    float32x4_t abs_diff3 = vabsq_f32(vsubq_f32(cur_down1_vec,next_down_1_vec));

                    uint32x4_t motion_cond1 = vcgtq_f32(abs_diff1, v_mthresh);
                    uint32x4_t motion_cond2 = vcgtq_f32(abs_diff2, v_mthresh);
                    uint32x4_t motion_cond3 = vcgtq_f32(abs_diff3, v_mthresh);

                    motion = vandq_u32(vandq_u32(motion_cond1, motion_cond2), motion_cond3);

                    float32x4_t prev_up_1_vec =  {pv->gamma_lut[prev[up_1_prev + 0]], pv->gamma_lut[prev[up_1_prev + 1]], pv->gamma_lut[prev[up_1_prev + 2]], pv->gamma_lut[prev[up_1_prev + 3]]};
                    float32x4_t prev_down_1_vec =  {pv->gamma_lut[prev[down_1_prev + 0]], pv->gamma_lut[prev[down_1_prev + 1]], pv->gamma_lut[prev[down_1_prev + 2]], pv->gamma_lut[prev[down_1_prev + 3]]};
                    float32x4_t abs_diff4 = vabsq_f32(vsubq_f32(next_vec, cur_vec));
                    float32x4_t abs_diff5 = vabsq_f32(vsubq_f32(prev_up_1_vec, cur_up1_vec));
                    float32x4_t abs_diff6 = vabsq_f32(vsubq_f32(prev_down_1_vec, cur_down1_vec));

                    uint32x4_t motion_cond4 = vcgtq_f32(abs_diff4, v_mthresh);
                    uint32x4_t motion_cond5 = vcgtq_f32(abs_diff5, v_mthresh);
                    uint32x4_t motion_cond6 = vcgtq_f32(abs_diff6, v_mthresh);
                    motion1 = vandq_u32(vandq_u32(motion_cond4, motion_cond5), motion_cond6);
                    motion = vorrq_u32(motion, motion1);
                }
                else
                {
                    motion = vdupq_n_u32(1);
                }

                uint32x4_t motion_check = vorrq_u32(motion, v_exhaustive_check);
                motion_check = vcgtq_u32(motion_check, mask_vec);

                float32x4_t cur_up2_vec = {pv->gamma_lut[cur[up_2 + 0]], pv->gamma_lut[cur[up_2 + 1]], pv->gamma_lut[cur[up_2 + 2]], pv->gamma_lut[cur[up_2 + 3]]};
                float32x4_t cur_down2_vec = {pv->gamma_lut[cur[down_2 + 0]], pv->gamma_lut[cur[down_2 + 1]], pv->gamma_lut[cur[down_2 + 2]], pv->gamma_lut[cur[down_2 + 3]]};

                float32x4_t combing1 = vabsq_f32(vsubq_f32(vaddq_f32(vaddq_f32(cur_up2_vec, vmulq_f32(cur_vec, v_four)),cur_down2_vec), vmulq_f32(vaddq_f32(cur_up1_vec, cur_down1_vec), v_three)));

                uint32x4_t combing_cond = vcgtq_f32(combing1, v_athresh6);
                mask_vec = vandq_u32(combing_cond, motion_check);
                mask_vec = vandq_u32(mask_vec, condition);
                mask_vec = vandq_u32(mask_vec, v_one);

                vst1q_u32(mask32, mask_vec);

                mask[0] = mask32[0];
                mask[1] = mask32[1];
                mask[2] = mask32[2];
                mask[3] = mask32[3];
            }
            cur+=4;
            prev+=4;
            next+=4;
            mask+=4;
        }
    }
}
#else
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
#endif

#if defined (__aarch64__) && !defined(__APPLE__)
#if BIT_DEPTH > 8
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

    int32x4_t v_athresh = vdupq_n_s32(athresh);
    int32x4_t v_athresh_neg = vdupq_n_s32(-athresh);
    int32x4_t v_mthresh = vdupq_n_s32(mthresh);
    int32x4_t v_athresh6 = vdupq_n_s32(athresh6);
    int32x4_t v_athresh_squared = vdupq_n_s32(athresh_squared);
    int32x4_t v_four = vdupq_n_s32(4);
    int32x4_t v_three = vdupq_n_s32(3);
    uint32x4_t v_one = vdupq_n_u32(1);
    int32x4_t v_c32detect_min = vdupq_n_s32(pv->comb32detect_min);
    int32x4_t v_c32detect_max = vdupq_n_s32(pv->comb32detect_max);
    uint32x4_t v_exhaustive_check = vdupq_n_u32(pv->force_exaustive_check);

    for (int y = segment_start; y < segment_stop; y++)
    {
        // We need to examine a column of 5 pixels
        // in the prev, cur, and next frames.
        const pixel *prev = &((const pixel *)pv->ref[0]->plane[0].data)[y * stride_prev];
        const pixel *cur  = &((const pixel *)pv->ref[1]->plane[0].data)[y * stride_cur];
        const pixel *next = &((const pixel *)pv->ref[2]->plane[0].data)[y * stride_next];
        uint8_t *mask = &pv->mask->plane[0].data[y * mask_stride];

        memset(mask, 0, mask_stride);
        uint32_t mask32[4];
        for (int x = 0; x < width; x+=4)
        {
            uint32x4_t mask_vec = vdupq_n_u32(0);

            int32x4_t cur_vec = vreinterpretq_s32_u32(vmovl_u16(vld1_u16(cur)));
            int32x4_t cur_up1_vec = vreinterpretq_s32_u32(vmovl_u16(vld1_u16(cur + up_1)));
            int32x4_t cur_down1_vec = vreinterpretq_s32_u32(vmovl_u16(vld1_u16(cur + down_1)));

            int32x4_t up_diff_vec = vsubq_s32(cur_vec, cur_up1_vec);
            int32x4_t down_diff_vec = vsubq_s32(cur_vec, cur_down1_vec);

            uint32x4_t cond1 = vcgtq_s32(up_diff_vec, v_athresh);
            uint32x4_t cond2 = vcgtq_s32(down_diff_vec, v_athresh);
            uint32x4_t cond3 = vcltq_s32(up_diff_vec, v_athresh_neg);
            uint32x4_t cond4 = vcltq_s32(down_diff_vec, v_athresh_neg);

            uint32x4_t condition1 = vandq_u32(cond1, cond2);
            uint32x4_t condition2 = vandq_u32(cond3, cond4);
            uint32x4_t condition = vorrq_u32(condition1, condition2);

            if(vmaxvq_u32(condition) > 0)
            {
                uint32x4_t motion = vdupq_n_u32(0);
                uint32x4_t motion1 = vdupq_n_u32(0);
                if (mthresh > 0)
                {
                    int32x4_t prev_vec = vreinterpretq_s32_u32(vmovl_u16(vld1_u16(prev)));
                    int32x4_t next_up1_vec = vreinterpretq_s32_u32(vmovl_u16(vld1_u16(next + up_1_next)));
                    int32x4_t next_down1_vec = vreinterpretq_s32_u32(vmovl_u16(vld1_u16(next + down_1_next)));
                    int32x4_t abs_diff1 = vabsq_s32(vsubq_s32(prev_vec, cur_vec));
                    int32x4_t abs_diff2 = vabsq_s32(vsubq_s32(cur_up1_vec, next_up1_vec));
                    int32x4_t abs_diff3 = vabsq_s32(vsubq_s32(cur_down1_vec, next_down1_vec));

                    uint32x4_t motion_cond1 = vcgtq_s32(abs_diff1, v_mthresh);
                    uint32x4_t motion_cond2 = vcgtq_s32(abs_diff2, v_mthresh);
                    uint32x4_t motion_cond3 = vcgtq_s32(abs_diff3, v_mthresh);

                    motion = vandq_u32(vandq_u32(motion_cond1, motion_cond2), motion_cond3);

                    int32x4_t next_vec = vreinterpretq_s32_u32(vmovl_u16(vld1_u16(next)));
                    int32x4_t prev_up1_vec = vreinterpretq_s32_u32(vmovl_u16(vld1_u16(prev + up_1_prev)));
                    int32x4_t prev_down1_vec = vreinterpretq_s32_u32(vmovl_u16(vld1_u16(prev + down_1_prev)));
                    int32x4_t abs_diff4 = vabsq_s32(vsubq_s32(next_vec, cur_vec));
                    int32x4_t abs_diff5 = vabsq_s32(vsubq_s32(prev_up1_vec, cur_up1_vec));
                    int32x4_t abs_diff6 = vabsq_s32(vsubq_s32(prev_down1_vec, cur_down1_vec));

                    uint32x4_t motion_cond4 = vcgtq_s32(abs_diff4, v_mthresh);
                    uint32x4_t motion_cond5 = vcgtq_s32(abs_diff5, v_mthresh);
                    uint32x4_t motion_cond6 = vcgtq_s32(abs_diff6, v_mthresh);

                    motion1 = vandq_u32(vandq_u32(motion_cond4, motion_cond5), motion_cond6);
                    motion = vorrq_u32(motion, motion1);
                }
                else
                {
                    motion = vdupq_n_u32(1);
                }
                uint32x4_t motion_check = vorrq_u32(motion, v_exhaustive_check);
                motion_check = vcgtq_u32(motion_check, mask_vec);

                int32x4_t cur_up2_vec = vreinterpretq_s32_u32(vmovl_u16(vld1_u16(cur + up_2)));
                int32x4_t cur_down2_vec = vreinterpretq_s32_u32(vmovl_u16(vld1_u16(cur + down_2)));


                switch(spatial_metric)
                {
                    case 0:
                    {
                        uint32x4_t cond_c32_detect_min = vcltq_s32(vabsq_s32(vsubq_s32(cur_vec, cur_down2_vec)), v_c32detect_min);
                        uint32x4_t cond_c32_detect_max = vcgtq_s32(vabsq_s32(vsubq_s32(cur_vec, cur_down1_vec)), v_c32detect_max);

                        uint32x4_t s_metric_0_vec = vandq_u32(cond_c32_detect_min, cond_c32_detect_max);
                        mask_vec = vandq_u32(s_metric_0_vec, motion_check);
                        mask_vec = vandq_u32(mask_vec, condition);
                        mask_vec = vandq_u32(mask_vec, v_one);

                        vst1q_u32(mask32, mask_vec);

                        mask[0] = mask32[0];
                        mask[1] = mask32[1];
                        mask[2] = mask32[2];
                        mask[3] = mask32[3];
                        break;
                    }
                    case 1:
                    {
                        int32x4_t s_metric_1_diff1 = vsubq_s32(cur_up1_vec, cur_vec);
                        int32x4_t s_metric_1_diff2 = vsubq_s32(cur_down1_vec, cur_vec);
                        int32x4_t s_metric_1_mul = vmulq_s32(s_metric_1_diff1, s_metric_1_diff2);
                        uint32x4_t s_metric_1_vec = vcgtq_s32(s_metric_1_mul, v_athresh_squared);
                        mask_vec = vandq_u32(s_metric_1_vec, motion_check);
                        mask_vec = vandq_u32(mask_vec, condition);
                        mask_vec = vandq_u32(mask_vec, v_one);

                        vst1q_u32(mask32, mask_vec);

                        mask[0] = mask32[0];
                        mask[1] = mask32[1];
                        mask[2] = mask32[2];
                        mask[3] = mask32[3];
                        break;
                    }
                    case 2:
                    {
                        int32x4_t combing1 = vabsq_s32(vsubq_s32(vaddq_s32(vaddq_s32(cur_up2_vec, vmulq_s32(cur_vec, v_four)),cur_down2_vec), vmulq_s32(vaddq_s32(cur_up1_vec, cur_down1_vec), v_three)));
                        uint32x4_t s_metric_2_vec = vcgtq_s32(combing1, v_athresh6);

                        mask_vec = vandq_u32(s_metric_2_vec, motion_check);
                        mask_vec = vandq_u32(mask_vec, condition);
                        mask_vec = vandq_u32(mask_vec, v_one);

                        vst1q_u32(mask32, mask_vec);

                        mask[0] = mask32[0];
                        mask[1] = mask32[1];
                        mask[2] = mask32[2];
                        mask[3] = mask32[3];
                        break;
                    }
                }

            }
            cur+=4;
            prev+=4;
            next+=4;
            mask+=4;
        }
    }
}
#else
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

    int16x8_t v_athresh = vdupq_n_s16(athresh);
    int16x8_t v_athresh_neg = vdupq_n_s16(-athresh);
    int16x8_t v_mthresh = vdupq_n_s16(mthresh);
    int16x8_t v_athresh6 = vdupq_n_s16(athresh6);
    int16x8_t v_athresh_squared = vdupq_n_s16(athresh_squared);
    int16x8_t v_four = vdupq_n_s16(4);
    int16x8_t v_three = vdupq_n_s16(3);
    uint16x8_t v_one = vdupq_n_u16(1);
    int16x8_t v_c32detect_min = vdupq_n_s16(pv->comb32detect_min);
    int16x8_t v_c32detect_max = vdupq_n_s16(pv->comb32detect_max);
    uint16x8_t v_exhaustive_check = vdupq_n_u16(pv->force_exaustive_check);

    for (int y = segment_start; y < segment_stop; y++)
    {
        // We need to examine a column of 5 pixels
        // in the prev, cur, and next frames.
        const pixel *prev = &((const pixel *)pv->ref[0]->plane[0].data)[y * stride_prev];
        const pixel *cur  = &((const pixel *)pv->ref[1]->plane[0].data)[y * stride_cur];
        const pixel *next = &((const pixel *)pv->ref[2]->plane[0].data)[y * stride_next];
        uint8_t *mask = &pv->mask->plane[0].data[y * mask_stride];

        memset(mask, 0, mask_stride);
        for (int x = 0; x < width; x+=8)
        {
            uint16x8_t mask_vec = vdupq_n_u16(0);

            int16x8_t cur_vec = vreinterpretq_s16_u16(vmovl_u8(vld1_u8((uint8_t*)cur)));
            int16x8_t cur_up1_vec = vreinterpretq_s16_u16(vmovl_u8(vld1_u8((uint8_t*)cur + up_1)));
            int16x8_t cur_down1_vec = vreinterpretq_s16_u16(vmovl_u8(vld1_u8((uint8_t*)cur + down_1)));

            int16x8_t up_diff_vec = vsubq_s16(cur_vec, cur_up1_vec);
            int16x8_t down_diff_vec = vsubq_s16(cur_vec, cur_down1_vec);

            uint16x8_t cond1 = vcgtq_s16(up_diff_vec, v_athresh);
            uint16x8_t cond2 = vcgtq_s16(down_diff_vec, v_athresh);
            uint16x8_t cond3 = vcltq_s16(up_diff_vec, v_athresh_neg);
            uint16x8_t cond4 = vcltq_s16(down_diff_vec, v_athresh_neg);

            uint16x8_t condition1 = vandq_u16(cond1, cond2);
            uint16x8_t condition2 = vandq_u16(cond3, cond4);
            uint16x8_t condition = vorrq_u16(condition1, condition2);

            if(vmaxvq_u16(condition) > 0)
            {
                uint16x8_t motion = vdupq_n_u16(0);
                uint16x8_t motion1 = vdupq_n_u16(0);
                if (mthresh > 0)
                {
                    int16x8_t prev_vec = vreinterpretq_s16_u16(vmovl_u8(vld1_u8((uint8_t*)prev)));
                    int16x8_t next_up1_vec = vreinterpretq_s16_u16(vmovl_u8(vld1_u8((uint8_t*)next + up_1_next)));
                    int16x8_t next_down1_vec = vreinterpretq_s16_u16(vmovl_u8(vld1_u8((uint8_t*)next + down_1_next)));
                    int16x8_t abs_diff1 = vabsq_s16(vsubq_s16(prev_vec, cur_vec));
                    int16x8_t abs_diff2 = vabsq_s16(vsubq_s16(cur_up1_vec, next_up1_vec));
                    int16x8_t abs_diff3 = vabsq_s16(vsubq_s16(cur_down1_vec, next_down1_vec));

                    uint16x8_t motion_cond1 = vcgtq_s16(abs_diff1, v_mthresh);
                    uint16x8_t motion_cond2 = vcgtq_s16(abs_diff2, v_mthresh);
                    uint16x8_t motion_cond3 = vcgtq_s16(abs_diff3, v_mthresh);

                    motion = vandq_u16(vandq_u16(motion_cond1, motion_cond2), motion_cond3);

                    int16x8_t next_vec = vreinterpretq_s16_u16(vmovl_u8(vld1_u8((uint8_t*)next)));
                    int16x8_t prev_up1_vec = vreinterpretq_s16_u16(vmovl_u8(vld1_u8((uint8_t*)prev + up_1_prev)));
                    int16x8_t prev_down1_vec = vreinterpretq_s16_u16(vmovl_u8(vld1_u8((uint8_t*)prev + down_1_prev)));
                    int16x8_t abs_diff4 = vabsq_s16(vsubq_s16(next_vec, cur_vec));
                    int16x8_t abs_diff5 = vabsq_s16(vsubq_s16(prev_up1_vec, cur_up1_vec));
                    int16x8_t abs_diff6 = vabsq_s16(vsubq_s16(prev_down1_vec, cur_down1_vec));

                    uint16x8_t motion_cond4 = vcgtq_s16(abs_diff4, v_mthresh);
                    uint16x8_t motion_cond5 = vcgtq_s16(abs_diff5, v_mthresh);
                    uint16x8_t motion_cond6 = vcgtq_s16(abs_diff6, v_mthresh);

                    motion1 = vandq_u16(vandq_u16(motion_cond4, motion_cond5), motion_cond6);
                    motion = vorrq_u16(motion, motion1);
                }
                else
                {
                    motion = vdupq_n_u16(1);
                }
                uint16x8_t motion_check = vorrq_u16(motion, v_exhaustive_check);
                motion_check = vcgtq_u16(motion_check, mask_vec);

                int16x8_t cur_up2_vec = vreinterpretq_s16_u16(vmovl_u8(vld1_u8((uint8_t*)cur + up_2)));
                int16x8_t cur_down2_vec = vreinterpretq_s16_u16(vmovl_u8(vld1_u8((uint8_t*)cur + down_2)));
                switch(spatial_metric)
                {
                    case 0:
                    {
                        uint16x8_t cond_c32_detect_min = vcltq_s16(vabsq_s16(vsubq_s16(cur_vec, cur_down2_vec)), v_c32detect_min);
                        uint16x8_t cond_c32_detect_max = vcgtq_s16(vabsq_s16(vsubq_s16(cur_vec, cur_down1_vec)), v_c32detect_max);

                        uint16x8_t s_metric_0_vec = vandq_u16(cond_c32_detect_min, cond_c32_detect_max);
                        mask_vec = vandq_u16(s_metric_0_vec, motion_check);
                        mask_vec = vandq_u16(mask_vec, condition);
                        mask_vec = vandq_u16(mask_vec, v_one);

                        vst1_u8(&mask[x], vmovn_u16(mask_vec));
                        break;
                    }
                    case 1:
                    {
                        int16x8_t s_metric_1_diff1 = vsubq_s16(cur_up1_vec, cur_vec);
                        int16x8_t s_metric_1_diff2 = vsubq_s16(cur_down1_vec, cur_vec);
                        int16x8_t s_metric_1_mul = vmulq_s16(s_metric_1_diff1, s_metric_1_diff2);
                        uint16x8_t s_metric_1_vec = vcgtq_s16(s_metric_1_mul, v_athresh_squared);
                        mask_vec = vandq_u16(s_metric_1_vec, motion_check);
                        mask_vec = vandq_u16(mask_vec, condition);
                        mask_vec = vandq_u16(mask_vec, v_one);

                        vst1_u8(&mask[x], vmovn_u16(mask_vec));
                        break;
                    }
                    case 2:
                    {
                        int16x8_t combing1 = vabsq_s16(vsubq_s16(vaddq_s16(vaddq_s16(cur_up2_vec, vmulq_s16(cur_vec, v_four)),cur_down2_vec), vmulq_s16(vaddq_s16(cur_up1_vec, cur_down1_vec), v_three)));
                        uint16x8_t s_metric_2_vec = vcgtq_s16(combing1, v_athresh6);

                        mask_vec = vandq_u16(s_metric_2_vec, motion_check);
                        mask_vec = vandq_u16(mask_vec, condition);
                        mask_vec = vandq_u16(mask_vec, v_one);
                        vst1_u8(&mask[x], vmovn_u16(mask_vec));
                        break;
                    }
                }

            }
            cur+=8;
            prev+=8;
            next+=8;
        }
    }
}
#endif
#else

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
#endif

#undef pixel
#undef FUNC
