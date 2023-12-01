/* decomb_template.c

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

#include "handbrake/eedi2.h"

static void FUNC(init_crop_table)(void **crop_table_out, const int max_value)
{
    const int central_part_size = max_value + 1;
    const int table_size = central_part_size + 2048;

    pixel *crop_table = calloc(table_size, sizeof(pixel));

    for (int i = 0; i < central_part_size; i++)
    {
        crop_table[i + 1024] = i;
    }

    for (int i = central_part_size + 1024; i < table_size; i++)
    {
        crop_table[i] = max_value;
    }

    *crop_table_out = crop_table;
}

static inline int FUNC(cubic_interpolate_pixel)(const pixel *crop_table, int y0, int y1, int y2, int y3)
{
    // From http://www.neuron2.net/library/cubicinterp.html
    int result = (y0 * -3) + (y1 * 23) + (y2 * 23) + (y3 * -3);
    result = crop_table[(result / 40) + 1024];
    return result;
}

static inline void FUNC(cubic_interpolate_line)(pixel *dst,
                                                const pixel *crop_table,
                                                const pixel *cur,
                                                const int width,
                                                const int height,
                                                const int stride,
                                                const int y)
{
    for (int x = 0; x < width; x++)
    {
        int a, b, c, d;
        a = b = c = d = 0;

        if (y >= 3)
        {
            // Normal top
            a = cur[-3*stride];
            b = cur[-stride];
        }
        else if (y == 2 || y == 1)
        {
            // There's only one sample above this pixel, use it twice.
            a = cur[-stride];
            b = cur[-stride];
        }
        else if (y == 0)
        {
            // No samples above, triple up on the one below.
            a = cur[+stride];
            b = cur[+stride];
        }

        if (y <= (height - 4))
        {
            // Normal bottom
            c = cur[+stride];
            d = cur[3*stride];
        }
        else if (y == (height - 3) || y == (height - 2))
        {
            // There's only one sample below, use it twice.
            c = cur[+stride];
            d = cur[+stride];
        }
        else if (y == height - 1)
        {
            // No samples below, triple up on the one above.
            c = cur[-stride];
            d = cur[-stride];
        }

        dst[0] = FUNC(cubic_interpolate_pixel)(crop_table, a, b, c, d);

        dst++;
        cur++;
    }
}

static inline int FUNC(blend_filter_pixel)(const filter_param_t *filter,
                                           const pixel *crop_table,
                                           const int up2, const int up1,
                                           const int current,
                                           const int down1, const int down2)
{
    // Low-pass 5-tap filter
    int result = 0;

    result += up2 * filter->tap[0];
    result += up1 * filter->tap[1];
    result += current * filter->tap[2];
    result += down1 * filter->tap[3];
    result += down2 * filter->tap[4];
    result >>= filter->normalize;

    result = crop_table[result + 1024];

    return result;
}

static void FUNC(blend_filter_line)(const filter_param_t *filter,
                                    const pixel *crop_table,
                                    pixel *dst,
                                    const pixel *cur,
                                    const int width,
                                    const int height,
                                    const int stride,
                                    const int y)
{
    int up1, up2, down1, down2;

    if (y > 1 && y < (height - 2))
    {
        up1 = -1 * stride;
        up2 = -2 * stride;
        down1 = 1 * stride;
        down2 = 2 * stride;
    }
    else if (y == 0)
    {
        // First line, so A and B don't exist.
        up1 = up2 = 0;
        down1 = 1 * stride;
        down2 = 2 * stride;
    }
    else if (y == 1)
    {
        // Second line, no A.
        up1 = up2 = -1 * stride;
        down1 = 1 * stride;
        down2 = 2 * stride;
    }
    else if (y == (height - 2))
    {
        // Second to last line, no E.
        up1 = -1 * stride;
        up2 = -2 * stride;
        down1 = down2 = 1 * stride;
    }
    else if (y == (height -1))
    {
        // Last line, no D or E.
        up1 = -1 * stride;
        up2 = -2 * stride;
        down1 = down2 = 0;
    }
    else
    {
        hb_error("Invalid value y %d height %d", y, height);
        return;
    }

    for (int x = 0; x < width; x++)
    {
        // Low-pass 5-tap filter
        dst[0] = FUNC(blend_filter_pixel)(filter, crop_table,
                                          cur[up2], cur[up1], cur[0],
                                          cur[down1], cur[down2]);
        dst++;
        cur++;
    }
}

/// This function calls all the eedi2 filters in sequence for a given plane.
/// It outputs the final interpolated image to pv->eedi_full[DST2PF].
static void FUNC(eedi2_interpolate_plane)(hb_filter_private_t *pv, int plane)
{
    // We need all these pointers. No, seriously.
    // I swear. It's not a joke. They're used.
    // All nine of them.
    pixel *mskp   = (pixel *)pv->eedi_half[MSKPF]->plane[plane].data;
    pixel *srcp   = (pixel *)pv->eedi_half[SRCPF]->plane[plane].data;
    pixel *tmpp   = (pixel *)pv->eedi_half[TMPPF]->plane[plane].data;
    pixel *dstp   = (pixel *)pv->eedi_half[DSTPF]->plane[plane].data;
    pixel *dst2p  = (pixel *)pv->eedi_full[DST2PF]->plane[plane].data;
    pixel *tmp2p2 = (pixel *)pv->eedi_full[TMP2PF2]->plane[plane].data;
    pixel *msk2p  = (pixel *)pv->eedi_full[MSK2PF]->plane[plane].data;
    pixel *tmp2p  = (pixel *)pv->eedi_full[TMP2PF]->plane[plane].data;
    pixel *dst2mp = (pixel *)pv->eedi_full[DST2MPF]->plane[plane].data;
    int *cx2 = pv->cx2;
    int *cy2 = pv->cy2;
    int *cxy = pv->cxy;
    int *tmpc = pv->tmpc;

    const int pitch = pv->eedi_full[0]->plane[plane].stride / pv->bps;
    const int height = pv->eedi_full[0]->plane[plane].height;
    const int width = pv->eedi_full[0]->plane[plane].width;
    const int half_height = pv->eedi_half[0]->plane[plane].height;

    // edge mask
    FUNC(eedi2_build_edge_mask)(mskp, pitch, srcp, pitch,
                     pv->magnitude_threshold, pv->variance_threshold, pv->laplacian_threshold,
                     half_height, width, pv->depth);
    FUNC(eedi2_erode_edge_mask)(mskp, pitch, tmpp, pitch, pv->erosion_threshold, half_height, width, pv->depth);
    FUNC(eedi2_dilate_edge_mask)(tmpp, pitch, mskp, pitch, pv->dilation_threshold, half_height, width, pv->depth);
    FUNC(eedi2_erode_edge_mask)(mskp, pitch, tmpp, pitch, pv->erosion_threshold, half_height, width, pv->depth);
    FUNC(eedi2_remove_small_gaps)(tmpp, pitch, mskp, pitch, half_height, width, pv->depth);

    // direction mask
    FUNC(eedi2_calc_directions)(plane, mskp, pitch, srcp, pitch, tmpp, pitch,
                     pv->maximum_search_distance, pv->noise_threshold,
                     half_height, width, pv->depth, pv->eedi_limlut);
    FUNC(eedi2_filter_dir_map)(mskp, pitch, tmpp, pitch, dstp, pitch, half_height, width, pv->depth, pv->eedi_limlut);
    FUNC(eedi2_expand_dir_map)(mskp, pitch, dstp, pitch, tmpp, pitch, half_height, width, pv->depth, pv->eedi_limlut);
    FUNC(eedi2_filter_map)(mskp, pitch, tmpp, pitch, dstp, pitch, half_height, width, pv->depth);

    // upscale 2x vertically
    FUNC(eedi2_upscale_by_2)(srcp, dst2p, half_height, pitch);
    FUNC(eedi2_upscale_by_2)(dstp, tmp2p2, half_height, pitch);
    FUNC(eedi2_upscale_by_2)(mskp, msk2p, half_height, pitch);

    // upscale the direction mask
    FUNC(eedi2_mark_directions_2x)(msk2p, pitch, tmp2p2, pitch, tmp2p, pitch, pv->tff, height, width, pv->depth, pv->eedi_limlut);
    FUNC(eedi2_filter_dir_map_2x)(msk2p, pitch, tmp2p, pitch,  dst2mp, pitch, pv->tff, height, width, pv->depth, pv->eedi_limlut);
    FUNC(eedi2_expand_dir_map_2x)(msk2p, pitch, dst2mp, pitch, tmp2p, pitch, pv->tff, height, width, pv->depth, pv->eedi_limlut);
    FUNC(eedi2_fill_gaps_2x)(msk2p, pitch, tmp2p, pitch, dst2mp, pitch, pv->tff, height, width, pv->depth);
    FUNC(eedi2_fill_gaps_2x)(msk2p, pitch, dst2mp, pitch, tmp2p, pitch, pv->tff, height, width, pv->depth);

    // interpolate a full-size plane
    FUNC(eedi2_interpolate_lattice)( plane, tmp2p, pitch, dst2p, pitch, tmp2p2, pitch, pv->tff,
                         pv->noise_threshold, height, width, pv->depth, pv->eedi_limlut);

    if (pv->post_processing == 1 || pv->post_processing == 3)
    {
        // make sure the edge directions are consistent
        FUNC(eedi2_bit_blit)( tmp2p2, pitch, tmp2p, pitch, width, height);
        FUNC(eedi2_filter_dir_map_2x)(msk2p, pitch, tmp2p, pitch, dst2mp, pitch, pv->tff, height, width, pv->depth, pv->eedi_limlut);
        FUNC(eedi2_expand_dir_map_2x)(msk2p, pitch, dst2mp, pitch, tmp2p, pitch, pv->tff, height, width, pv->depth, pv->eedi_limlut);
        FUNC(eedi2_post_process)(tmp2p, pitch, tmp2p2, pitch, dst2p, pitch, pv->tff, height, width, pv->depth, pv->eedi_limlut);
    }
    if (pv->post_processing == 2 || pv->post_processing == 3)
    {
        // filter junctions and corners
        FUNC(eedi2_gaussian_blur1)( srcp, pitch, tmpp, pitch, srcp, pitch, half_height, width );
        FUNC(eedi2_calc_derivatives)(srcp, pitch, half_height, width, cx2, cy2, cxy, pv->depth);
        FUNC(eedi2_gaussian_blur_sqrt2)( cx2, tmpc, cx2, pitch, half_height, width);
        FUNC(eedi2_gaussian_blur_sqrt2)( cy2, tmpc, cy2, pitch, half_height, width);
        FUNC(eedi2_gaussian_blur_sqrt2)( cxy, tmpc, cxy, pitch, half_height, width);
        FUNC(eedi2_post_process_corner)(cx2, cy2, cxy, pitch, tmp2p2, pitch, dst2p, pitch, height, width, pv->tff, pv->depth);
    }
}

static void FUNC(eedi2_filter_work)(void *thread_args_v)
{
    eedi2_thread_arg_t *thread_args = thread_args_v;
    hb_filter_private_t *pv = thread_args->pv;
    int plane = thread_args->arg.segment;

    //Process plane
    FUNC(eedi2_interpolate_plane)(pv, plane);
}

/// Sets up the input field planes for EEDI2 in pv->eedi_half[SRCPF]
/// and then runs eedi2_filter_thread for each plane.
static void FUNC(eedi2_planer)(hb_filter_private_t *pv)
{
    // Copy the first field from the source to a half-height frame.
    for (int pp = 0;  pp < 3; pp++)
    {
        const int pitch = pv->ref[1]->plane[pp].stride / pv->bps;
        const int height = pv->ref[1]->plane[pp].height;
        const int start_line = !pv->tff;

        FUNC(eedi2_fill_half_height_buffer_plane)(&((pixel *)pv->ref[1]->plane[pp].data)[pitch * start_line],
                                                  (pixel *)pv->eedi_half[SRCPF]->plane[pp].data, pitch, height);
    }

    // Now that all data is ready for our threads, fire them off
    // and wait for their completion.
    taskset_cycle(&pv->eedi2_taskset);
}

/// EDDI: Edge Directed Deinterlacing Interpolation
/// Checks 4 different slopes to see if there is more similarity along a diagonal
/// than there was vertically. If a diagonal is more similar, then it indicates
/// an edge, so interpolate along that instead of a vertical line, using either
/// linear or cubic interpolation depending on mode.
#if BIT_DEPTH > 8

#define YADIF_CHECK(j)                                                                                                  \
{                                                                                                                       \
    int score = ABS(cur[stride_cur_p - 1 + j] - cur[stride_cur_n - 1 -  j]) +                                           \
                ABS(cur[stride_cur_p     + j] - cur[stride_cur_n      - j]) +                                           \
                ABS(cur[stride_cur_p + 1 + j] - cur[stride_cur_n + 1 -  j]);                                            \
    if (score < spatial_score)                                                                                          \
    {                                                                                                                   \
        spatial_score = score;                                                                                          \
        if ((pv->mode & MODE_DECOMB_CUBIC) && !vertical_edge)                                                           \
        {                                                                                                               \
            switch (j)                                                                                                  \
            {                                                                                                           \
                case -1:                                                                                                \
                    spatial_pred = cubic_interpolate_pixel_16(crop_table,                                               \
                                                              cur[-3 * stride_cur - 3],                                 \
                                                              cur[-stride_cur - 1],                                     \
                                                              cur[+stride_cur + 1],                                     \
                                                              cur[3 * stride_cur + 3]);                                 \
                break;                                                                                                  \
                case -2:                                                                                                \
                    spatial_pred = cubic_interpolate_pixel_16(crop_table,                                               \
                                                              ((cur[-3 * stride_cur - 4] + cur[-stride_cur - 4]) / 2),  \
                                                                cur[-stride_cur - 2],                                   \
                                                                cur[+stride_cur + 2],                                   \
                                                              ((cur[3 * stride_cur + 4]  + cur[stride_cur + 4]) / 2));  \
                break;                                                                                                  \
                case 1:                                                                                                 \
                    spatial_pred = cubic_interpolate_pixel_16(crop_table,                                               \
                                                              cur[-3 * stride_cur +3],                                  \
                                                              cur[-stride_cur + 1],                                     \
                                                              cur[+stride_cur - 1],                                     \
                                                              cur[3 * stride_cur - 3] );                                \
                break;                                                                                                  \
                case 2:                                                                                                 \
                    spatial_pred = cubic_interpolate_pixel_16(crop_table,                                               \
                                                             ((cur[-3 * stride_cur + 4] + cur[-stride_cur + 4]) / 2),   \
                                                               cur[-stride_cur + 2],                                    \
                                                               cur[+stride_cur - 2],                                    \
                                                             ((cur[3 * stride_cur - 4]  + cur[stride_cur - 4]) / 2));   \
                break;                                                                                                  \
            }                                                                                                           \
        }                                                                                                               \
        else                                                                                                            \
        {                                                                                                               \
            spatial_pred = (cur[stride_cur_p + j] + cur[stride_cur_n - j]) >> 1;                                        \
        }

#else

#define YADIF_CHECK(j)                                                                                                  \
{                                                                                                                       \
    int score = ABS(cur[stride_cur_p - 1 + j] - cur[stride_cur_n - 1 -  j]) +                                           \
                ABS(cur[stride_cur_p     + j] - cur[stride_cur_n      - j]) +                                           \
                ABS(cur[stride_cur_p + 1 + j] - cur[stride_cur_n + 1 -  j]);                                            \
    if (score < spatial_score)                                                                                          \
    {                                                                                                                   \
        spatial_score = score;                                                                                          \
        if ((pv->mode & MODE_DECOMB_CUBIC) && !vertical_edge)                                                           \
        {                                                                                                               \
            switch (j)                                                                                                  \
            {                                                                                                           \
                case -1:                                                                                                \
                    spatial_pred = cubic_interpolate_pixel_8(crop_table,                                                \
                                                             cur[-3 * stride_cur - 3],                                  \
                                                             cur[-stride_cur - 1],                                      \
                                                             cur[+stride_cur + 1],                                      \
                                                             cur[3 * stride_cur + 3]);                                  \
                break;                                                                                                  \
                case -2:                                                                                                \
                    spatial_pred = cubic_interpolate_pixel_8(crop_table,                                                \
                                                            ((cur[-3 * stride_cur - 4] + cur[-stride_cur - 4]) / 2),    \
                                                              cur[-stride_cur - 2],                                     \
                                                              cur[+stride_cur + 2],                                     \
                                                            ((cur[3 * stride_cur + 4]  + cur[stride_cur + 4]) / 2));    \
                break;                                                                                                  \
                case 1:                                                                                                 \
                    spatial_pred = cubic_interpolate_pixel_8(crop_table,                                                \
                                                             cur[-3 * stride_cur +3],                                   \
                                                             cur[-stride_cur + 1],                                      \
                                                             cur[+stride_cur - 1],                                      \
                                                             cur[3 * stride_cur - 3] );                                 \
                break;                                                                                                  \
                case 2:                                                                                                 \
                    spatial_pred = cubic_interpolate_pixel_8(crop_table,                                                \
                                                            ((cur[-3 * stride_cur + 4] + cur[-stride_cur + 4]) / 2),    \
                                                              cur[-stride_cur + 2],                                     \
                                                              cur[+stride_cur - 2],                                     \
                                                            ((cur[3 * stride_cur - 4]  + cur[stride_cur - 4]) / 2));    \
                break;                                                                                                  \
            }                                                                                                           \
        }                                                                                                               \
        else                                                                                                            \
        {                                                                                                               \
            spatial_pred = (cur[stride_cur_p + j] + cur[stride_cur_n - j]) >> 1;                                        \
        }
#endif

static void FUNC(yadif_filter_line)(const hb_filter_private_t *pv,
                                    pixel             *dst,
                                    const pixel       *prev,
                                    const pixel       *cur,
                                    const pixel       *next,
                                    const int          stride_dst,
                                    const int          stride_prev,
                                    const int          stride_cur,
                                    const int          stride_next,
                                    const int          plane,
                                    const int          width,
                                    const int          height,
                                    const int          parity,
                                    const int          y)
{
    const pixel *crop_table = (const pixel *)pv->crop_table;
    // While prev and next point to the previous and next frames,
    // prev2 and next2 will shift depending on the parity, usually 1.
    // They are the previous and next fields, the fields temporally adjacent
    // to the other field in the current frame--the one not being filtered.
    const pixel *prev2 = parity ? prev : cur;
    const int stride_prev2 = parity ? stride_prev : stride_cur;

    const pixel *next2 = parity ? cur  : next;
    const int stride_next2 = parity ? stride_cur : stride_next;

    // Invert the stride for the first and last line
    const int stride_prev_p = y ? -stride_prev : stride_prev;
    const int stride_prev_n = y + 1 < height ? stride_prev : -stride_prev;
    const int stride_cur_p  = y ? -stride_cur : stride_cur;
    const int stride_cur_n  = y + 1 < height ? stride_cur : -stride_cur;
    const int stride_next_p = y ? -stride_next : stride_next;
    const int stride_next_n = y + 1 < height ? stride_next : -stride_next;

    const int eedi2_mode = (pv->mode & MODE_DECOMB_EEDI2);

    // We can replace spatial_pred with this interpolation
    const pixel *eedi2_guess = eedi2_mode ? &((pixel *)pv->eedi_full[DST2PF]->plane[plane].data)[y * stride_dst] : NULL;

    // Decomb's cubic interpolation can only function when there are
    // three samples above and below, so regress to yadif's traditional
    // two-tap interpolation when filtering at the top and bottom edges.
    const int vertical_edge = (y < 3) || (y > (height - 4)) ? 1 : 0;

    // YADIF_CHECK requires a margin to avoid invalid memory access.
    // In MODE_DECOMB_CUBIC, margin needed is 2 + ABS(param).
    // Else, the margin needed is 1 + ABS(param).
    const int margin = pv->mode & MODE_DECOMB_CUBIC ? 3 : 2;

    for (int x = 0; x < width; x++)
    {
        // Pixel above
        const int c = cur[stride_cur_p];
        // Temporal average: the current location in the adjacent fields
        const int d = (prev2[0] + next2[0]) >> 1;
        // Pixel below
        const int e = cur[stride_cur_n];

        // How the current pixel changes between the adjacent fields
        const int temporal_diff0 = ABS(prev2[0] - next2[0]);
        // The average of how much the pixels above and below change from the frame before to now.
        const int temporal_diff1 = (ABS(prev[stride_prev_p] - c) + ABS(prev[stride_prev_n] - e)) >> 1;
        // The average of how much the pixels above and below change from now to the next frame.
        const int temporal_diff2 = (ABS(next[stride_next_p] - c) + ABS(next[stride_next_n] - e)) >> 1;
        // For the actual difference, use the largest of the previous average diffs.
        int diff                 = MAX3(temporal_diff0 >> 1, temporal_diff1, temporal_diff2);

        int spatial_pred;

        if (eedi2_mode)
        {
            // Who needs yadif's spatial predictions when we can have EEDI2's?
            spatial_pred = eedi2_guess[0];
            eedi2_guess++;
        }
        else // Yadif spatial interpolation
        {
            // Spatial pred is either a bilinear or cubic vertical interpolation.
            if ((pv->mode & MODE_DECOMB_CUBIC) && !vertical_edge)
            {
                spatial_pred = FUNC(cubic_interpolate_pixel)(crop_table,
                                                             cur[-3 * stride_cur], cur[-stride_cur],
                                                             cur[+stride_cur],     cur[3 * stride_cur]);
            }
            else
            {
                spatial_pred = (c + e) >> 1;
            }

            if (x > margin && x < width - (margin + 1))
            {
                // SAD of how the pixel-1, the pixel, and the pixel+1 change from the line above to below.
                int spatial_score = ABS(cur[stride_cur_p-1] - cur[stride_cur_n-1]) + ABS(c - e) +
                                    ABS(cur[stride_cur_p+1] - cur[stride_cur_n+1]) - 1;

                YADIF_CHECK(-1) YADIF_CHECK(-2) }} }}
                YADIF_CHECK( 1) YADIF_CHECK( 2) }} }}
            }
        }

        // Temporally adjust the spatial prediction by
        // comparing against lines in the adjacent fields.
        if (!vertical_edge)
        {
            const int b = (prev2[-2 * stride_prev2] + next2[-2 * stride_next2]) >> 1;
            const int f = (prev2[+2 * stride_prev2] + next2[+2 * stride_next2]) >> 1;

            // Find the median value
            const int max = MAX3(d-e, d-c, MIN(b-c, f-e));
            const int min = MIN3(d-e, d-c, MAX(b-c, f-e));
            diff = MAX3(diff, min, -max);
        }

        if (spatial_pred > d + diff)
        {
            spatial_pred = d + diff;
        }
        else if (spatial_pred < d - diff)
        {
            spatial_pred = d - diff;
        }

        dst[0] = spatial_pred;

        dst++;
        cur++;
        prev++;
        next++;
        prev2++;
        next2++;
    }
}

#undef YADIF_CHECK

static void FUNC(yadif_decomb_filter_work)(void *thread_args_v)
{
    yadif_thread_arg_t *thread_args = thread_args_v;
    hb_filter_private_t *pv = thread_args->pv;
    const int segment = thread_args->arg.segment;
    yadif_arguments_t *yadif_work = &pv->yadif_arguments[segment];

    // Process all three planes, but only this segment of it.
    const int mode = pv->yadif_arguments[segment].mode;
    const int tff = yadif_work->tff;
    const int parity = yadif_work->parity;
    const pixel *crop_table = (const pixel *)pv->crop_table;

    hb_buffer_t *dst = yadif_work->dst;

    for (int pp = 0; pp < 3; pp++)
    {
        const int width = dst->plane[pp].width;
        const int height = dst->plane[pp].height;
        const int size = dst->plane[pp].width * pv->bps;

        const int stride_dst  = dst->plane[pp].stride / pv->bps;
        const int stride_prev = pv->ref[0]->plane[pp].stride / pv->bps;
        const int stride_cur  = pv->ref[1]->plane[pp].stride / pv->bps;
        const int stride_next = pv->ref[2]->plane[pp].stride / pv->bps;

        const int segment_start = thread_args->segment_start[pp];
        const int segment_stop = segment_start + thread_args->segment_height[pp];

        // Filter parity lines
        int          start = parity ? (segment_start + 1) & ~1 : segment_start | 1;
        pixel       *dst2 = &((pixel *)dst->plane[pp].data)[start * stride_dst];
        const pixel *prev = &((const pixel *)pv->ref[0]->plane[pp].data)[start * stride_prev];
        const pixel *cur  = &((const pixel *)pv->ref[1]->plane[pp].data)[start * stride_cur];
        const pixel *next = &((const pixel *)pv->ref[2]->plane[pp].data)[start * stride_next];

        if (mode == MODE_DECOMB_BLEND)
        {
            filter_param_t filter;

            filter.tap[0] = -1;
            filter.tap[1] =  2;
            filter.tap[2] =  6;
            filter.tap[3] =  2;
            filter.tap[4] = -1;
            filter.normalize = 3;

            // These will be useful if we ever do temporal blending.
            for (int yy = start; yy < segment_stop; yy += 2)
            {
                // This line gets blend filtered, not yadif filtered.
                FUNC(blend_filter_line)(&filter, crop_table, dst2, cur, width, height, stride_cur, yy);
                dst2 += stride_dst * 2;
                cur  += stride_cur * 2;
            }
        }
        else if (mode == MODE_DECOMB_CUBIC)
        {
            for (int yy = start; yy < segment_stop; yy += 2)
            {
                // Just apply vertical cubic interpolation
                FUNC(cubic_interpolate_line)(dst2, crop_table, cur, width, height, stride_cur, yy);
                dst2 += stride_dst * 2;
                cur  += stride_cur * 2;
            }
        }
        else if (mode & MODE_DECOMB_YADIF)
        {
            for (int yy = start; yy < segment_stop; yy += 2)
            {

                FUNC(yadif_filter_line)(pv, dst2, prev, cur, next,
                                        stride_dst, stride_prev, stride_cur, stride_next,
                                        pp, width, height,
                                        parity ^ tff, yy);
                dst2 += stride_dst  * 2;
                prev += stride_prev * 2;
                cur  += stride_cur  * 2;
                next += stride_next * 2;
            }
        }

        // Copy unfiltered lines
        start = !parity ? (segment_start + 1) & ~1 : segment_start | 1;
        dst2 = &((pixel *)dst->plane[pp].data)[start * stride_dst];
        cur  = &((const pixel *)pv->ref[1]->plane[pp].data)[start * stride_cur];

        for (int yy = start; yy < segment_stop; yy += 2)
        {
            memcpy(dst2, cur, size);
            dst2 += stride_dst * 2;
            cur  += stride_cur * 2;
        }
    }
}

static void FUNC(filter)(hb_filter_private_t *pv,
                         hb_buffer_t *dst,
                         const int parity,
                         const int tff)
{
    int is_combed = HB_COMB_HEAVY;
    int mode = 0;

    if (pv->mode & MODE_DECOMB_SELECTIVE)
    {
        is_combed = pv->ref[1]->s.combed;
    }

    // Pick a mode based on the comb detect state and selected decomb modes
    if ((pv->mode & MODE_DECOMB_BLEND) && is_combed == HB_COMB_LIGHT)
    {
        mode = MODE_DECOMB_BLEND;
    }
    else if (is_combed != HB_COMB_NONE)
    {
        mode = pv->mode & ~MODE_DECOMB_SELECTIVE;
    }

    if (mode == MODE_DECOMB_BLEND)
    {
        pv->blended++;
    }
    else if (mode != 0)
    {
        pv->deinterlaced++;
    }
    else
    {
        pv->unfiltered++;
    }
    pv->frames++;

    if (mode & MODE_DECOMB_EEDI2)
    {
        // Generate an EEDI2 interpolation
        FUNC(eedi2_planer)(pv);
    }

    if (mode != 0)
    {
        if ((mode & MODE_DECOMB_EEDI2) && !(mode & MODE_DECOMB_YADIF))
        {
            // Just pass through the EEDI2 interpolation
            for (int pp = 0; pp < 3; pp++)
            {
                const pixel *ref = (const pixel *)pv->eedi_full[DST2PF]->plane[pp].data;
                const int ref_stride = pv->eedi_full[DST2PF]->plane[pp].stride / pv->bps;

                pixel *dest = (pixel *)dst->plane[pp].data;
                const int size = dst->plane[pp].width * pv->bps;
                const int height = dst->plane[pp].height;
                const int stride = dst->plane[pp].stride / pv->bps;

                for (int yy = 0; yy < height; yy++)
                {
                    memcpy(dest, ref, size);
                    dest += stride;
                    ref += ref_stride;
                }
            }
        }
        else
        {
            for (int segment = 0; segment < pv->cpu_count; segment++)
            {
                // Setup the work for this plane.
                pv->yadif_arguments[segment].parity = parity;
                pv->yadif_arguments[segment].tff = tff;
                pv->yadif_arguments[segment].dst = dst;
                pv->yadif_arguments[segment].mode = mode;
            }

            // Allow the taskset threads to make one pass over the data.
            taskset_cycle(&pv->yadif_taskset);
            // Entire frame is now deinterlaced.
        }
    }
    else
    {
        // Just passing through
        pv->yadif_arguments[0].mode = mode; // 0
        hb_buffer_copy(dst, pv->ref[1]);
    }
}

#undef pixel
#undef FUNC
