/* eedi2.h

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_EEDI2_H
#define HANDBRAKE_EEDI2_H

/**
 * EEDI2 directional limit lookup table
 *
 * These values are used to limit the range of edge direction searches and filtering.
 */
extern const int eedi2_limlut[33] __attribute__ ((aligned (16)));

// Used to order a sequence of metrics for median filtering
void eedi2_sort_metrics(int *order, const int length);

// Aping some Windows API functions AviSynth seems to like
// Taken from here: http://www.gidforums.com/t-8543.html
void *eedi2_aligned_malloc(size_t size, size_t align_size);
void eedi2_aligned_free(void *ptr);

void eedi2_init_limlut_8(void **limlut_out, const int depth);

// Copies bitmaps
void eedi2_bit_blit_8(uint8_t *dstp, const int dst_pitch, const uint8_t *srcp, const int src_pitch,
                      const int row_size, const int height);

// Sets up the initial field-sized bitmap EEDI2 interpolates from
void eedi2_fill_half_height_buffer_plane_8(const uint8_t *src, uint8_t *dst, const int src_pitch, const int dst_pitch, const int height);

// Simple line doubler
void eedi2_upscale_by_2_8(const uint8_t *srcp, uint8_t *dstp, const int height, const int pitch);

// Finds places where vertically adjacent pixels abruptly change intensity
void eedi2_build_edge_mask_8(uint8_t *dstp, const int dst_pitch, const uint8_t *srcp, const int src_pitch,
                             int mthresh, int lthresh, int vthresh, const int height, const int width, const int depth);

// Expands and smooths out the edge mask by considering a pixel
// to be masked if >= dilation threshold adjacent pixels are masked.
void eedi2_dilate_edge_mask_8(const uint8_t *mskp, const int msk_pitch, uint8_t *dstp, const int dst_pitch,
                              const int dstr, const int height, const int width, const int depth);

// Contracts the edge mask by considering a pixel to be masked
// only if > erosion threshold adjacent pixels are masked
void eedi2_erode_edge_mask_8(const uint8_t *mskp, const int msk_pitch, uint8_t *dstp, const int dst_pitch,
                             const int estr, const int height, const int width, const int depth);

// Smooths out horizontally aligned holes in the mask
// If none of the 6 horizontally adjacent pixels are masked,
// don't consider the current pixel masked. If there are any
// masked on both sides, consider the current pixel masked.
void eedi2_remove_small_gaps_8(const uint8_t *mskp, const int msk_pitch, uint8_t *dstp, const int dst_pitch,
                               const int height, const int width, const int depth);

// Spatial vectors. Looks at maximum_search_distance surrounding pixels
// to guess which angle edges follow. This is EEDI2's timesink, and can be
// thought of as YADIF_CHECK on steroids. Both find edge directions.
void eedi2_calc_directions_8(const int plane, const uint8_t *mskp, const int msk_pitch, const uint8_t *srcp, const int src_pitch,
                             uint8_t *dstp, const int dst_pitch, const int maxd, const int nt, const int height, const int width,
                             const int depth, const uint8_t limlut[33]);

void eedi2_filter_map_8(const uint8_t *mskp, const int msk_pitch, const uint8_t *dmskp, const int dmsk_pitch,
                       uint8_t *dstp, const int dst_pitch, const int height, const int width, const int depth);

void eedi2_filter_dir_map_8(const uint8_t *mskp, const int msk_pitch, const uint8_t* dmskp, const int dmsk_pitch, uint8_t *dstp,
                           const int dst_pitch, const int height, const int width, const int depth, const uint8_t limlut[33]);

void eedi2_expand_dir_map_8(const uint8_t *mskp, const int msk_pitch, const uint8_t  *dmskp, const int dmsk_pitch, uint8_t *dstp,
                           const int dst_pitch, const int height, const int width, const int depth, const uint8_t limlut[33]);

void eedi2_mark_directions_2x_8(const uint8_t *mskp, const int msk_pitch, const uint8_t *dmskp, const int dmsk_pitch, uint8_t *dstp,
                               const int dst_pitch, const int tff, const int height, const int width, const int depth, const uint8_t limlut[33]);

void eedi2_filter_dir_map_2x_8(const uint8_t *mskp, const int msk_pitch, const uint8_t *dmskp, const int dmsk_pitch, uint8_t *dstp,
                              const int dst_pitch, const int field, const int height, const int width, const int depth, const uint8_t limlut[33]);

void eedi2_expand_dir_map_2x_8(const uint8_t *mskp, const int msk_pitch, const uint8_t *dmskp, const int dmsk_pitch, uint8_t *dstp,
                              const int dst_pitch, const int field, const int height, const int width, const int depth, const uint8_t limlut[33]);

void eedi2_fill_gaps_2x_8(const uint8_t *mskp, const int msk_pitch, const uint8_t *dmskp, const int dmsk_pitch, uint8_t *dstp,
                         const int dst_pitch, const int field, const int height, const int width, const int depth);

void eedi2_interpolate_lattice_8(const int plane, uint8_t * dmskp, int dmsk_pitch, uint8_t * dstp,
                                int dst_pitch, uint8_t * omskp, int omsk_pitch, int field, int nt,
                                int height, int width, const int depth, const uint8_t limlut[33]);

void eedi2_post_process_8(const uint8_t *nmskp, const int nmsk_pitch, const uint8_t *omskp, const int omsk_pitch, uint8_t *dstp,
                         const int src_pitch, const int field, const int height, const int width, const int depth, const uint8_t limlut[33]);

void eedi2_gaussian_blur1_8(const uint8_t *src, const int src_pitch, uint8_t *tmp, int tmp_pitch, uint8_t *dst,
                           const int dst_pitch, const int height, const int width);

void eedi2_gaussian_blur_sqrt2_8(const int *src, int *tmp, int *dst, const int pitch,
                                const int height, const int width );

void eedi2_calc_derivatives_8(const uint8_t *srcp, const int src_pitch, const int height, const int width,
                             int *x2, int *y2, int *xy, const int depth);

void eedi2_post_process_corner_8(int *x2, int *y2, int *xy, const int pitch, const uint8_t *mskp, const int msk_pitch,
                                uint8_t *dstp, const int dst_pitch, const int height, const int width, const int field, const int depth);

void eedi2_init_limlut_16(void **limlut_out, const int depth);

// Copies bitmaps
void eedi2_bit_blit_16(uint16_t *dstp, const int dst_pitch, const uint16_t *srcp, const int src_pitch,
                      const int row_size, const int height);

// Sets up the initial field-sized bitmap EEDI2 interpolates from
void eedi2_fill_half_height_buffer_plane_16(const uint16_t *src, uint16_t *dst, const int src_pitch, const int dst_pitch, const int height);

// Simple line doubler
void eedi2_upscale_by_2_16(const uint16_t *srcp, uint16_t *dstp, const int height, const int pitch);

// Finds places where vertically adjacent pixels abruptly change intensity
void eedi2_build_edge_mask_16(uint16_t *dstp, const int dst_pitch, const uint16_t *srcp, const int src_pitch,
                             int mthresh, int lthresh, int vthresh, const int height, const int width, const int bitsPerSample);

// Expands and smooths out the edge mask by considering a pixel
// to be masked if >= dilation threshold adjacent pixels are masked.
void eedi2_dilate_edge_mask_16(const uint16_t *mskp, const int msk_pitch, uint16_t *dstp, const int dst_pitch,
                              const int dstr, const int height, const int width, const int depth);

// Contracts the edge mask by considering a pixel to be masked
// only if > erosion threshold adjacent pixels are masked
void eedi2_erode_edge_mask_16(const uint16_t *mskp, const int msk_pitch, uint16_t *dstp, const int dst_pitch,
                             const int estr, const int height, const int width, const int depth);

// Smooths out horizontally aligned holes in the mask
// If none of the 6 horizontally adjacent pixels are masked,
// don't consider the current pixel masked. If there are any
// masked on both sides, consider the current pixel masked.
void eedi2_remove_small_gaps_16(const uint16_t *mskp, const int msk_pitch, uint16_t *dstp, const int dst_pitch,
                               const int height, const int width, const int depth);

// Spatial vectors. Looks at maximum_search_distance surrounding pixels
// to guess which angle edges follow. This is EEDI2's timesink, and can be
// thought of as YADIF_CHECK on steroids. Both find edge directions.
void eedi2_calc_directions_16(const int plane, const uint16_t *mskp, const int msk_pitch, const uint16_t *srcp, const int src_pitch,
                             uint16_t *dstp, const int dst_pitch, const int maxd, const int nt, const int height, const int width,
                              const int depth, const uint16_t limlut[33]);

void eedi2_filter_map_16(const uint16_t *mskp, const int msk_pitch, const uint16_t *dmskp, const int dmsk_pitch,
                       uint16_t *dstp, const int dst_pitch, const int height, const int width, const int depth);

void eedi2_filter_dir_map_16(const uint16_t *mskp, const int msk_pitch, const uint16_t* dmskp, const int dmsk_pitch, uint16_t *dstp,
                           const int dst_pitch, const int height, const int width, const int depth, const uint16_t limlut[33]);

void eedi2_expand_dir_map_16(const uint16_t *mskp, const int msk_pitch, const uint16_t  *dmskp, const int dmsk_pitch, uint16_t *dstp,
                           const int dst_pitch, const int height, const int width, const int depth, const uint16_t limlut[33]);

void eedi2_mark_directions_2x_16(const uint16_t *mskp, const int msk_pitch, const uint16_t *dmskp, const int dmsk_pitch, uint16_t *dstp,
                               const int dst_pitch, const int tff, const int height, const int width, const int depth, const uint16_t limlut[33]);

void eedi2_filter_dir_map_2x_16(const uint16_t *mskp, const int msk_pitch, const uint16_t *dmskp, const int dmsk_pitch, uint16_t *dstp,
                              const int dst_pitch, const int field, const int height, const int width, const int depth, const uint16_t limlut[33]);

void eedi2_expand_dir_map_2x_16(const uint16_t *mskp, const int msk_pitch, const uint16_t *dmskp, const int dmsk_pitch, uint16_t *dstp,
                              const int dst_pitch, const int field, const int height, const int width, const int depth, const uint16_t limlut[33]);

void eedi2_fill_gaps_2x_16(const uint16_t *mskp, const int msk_pitch, const uint16_t *dmskp, const int dmsk_pitch, uint16_t *dstp,
                         const int dst_pitch, const int field, const int height, const int width, const int depth);

void eedi2_interpolate_lattice_16(const int plane, uint16_t * dmskp, int dmsk_pitch, uint16_t * dstp,
                                int dst_pitch, uint16_t * omskp, int omsk_pitch, int field, int nt,
                                int height, int width, const int depth, const uint16_t limlut[33]);

void eedi2_post_process_16(const uint16_t *nmskp, const int nmsk_pitch, const uint16_t *omskp, const int omsk_pitch, uint16_t *dstp,
                         const int src_pitch, const int field, const int height, const int width, const int depth, const uint16_t limlut[33]);

void eedi2_gaussian_blur1_16(const uint16_t *src, const int src_pitch, uint16_t *tmp, int tmp_pitch, uint16_t *dst,
                           const int dst_pitch, const int height, const int width);

void eedi2_gaussian_blur_sqrt2_16(const int *src, int *tmp, int *dst, const int pitch,
                                const int height, const int width );

void eedi2_calc_derivatives_16(const uint16_t *srcp, const int src_pitch, const int height, const int width,
                             int *x2, int *y2, int *xy, const int depth);

void eedi2_post_process_corner_16(int *x2, int *y2, int *xy, const int pitch, const uint16_t *mskp, const int msk_pitch,
                                uint16_t *dstp, const int dst_pitch, const int height, const int width, const int field, const int depth);

#endif // HANDBRAKE_EEDI2_H
