// Used to order a sequeunce of metrics for median filtering
void eedi2_sort_metrics( int *order, const int length );

// Aping some Windows API funcctions AviSynth seems to like
// Taken from here: http://www.gidforums.com/t-8543.html
void *eedi2_aligned_malloc(size_t size, size_t align_size);
void eedi2_aligned_free(void *ptr);

// Copies bitmaps
void eedi2_bit_blit( uint8_t * dstp, int dst_pitch, const uint8_t * srcp, int src_pitch,
                     int row_size, int height );

// Sets up the initial field-sized bitmap EEDI2 interpolates from
void eedi2_fill_half_height_buffer_plane( uint8_t * src, uint8_t * dst, int pitch, int height );

// Simple line doubler
void eedi2_upscale_by_2( uint8_t * srcp, uint8_t * dstp, int height, int pitch );

// Finds places where vertically adjacent pixels abruptly change intensity
void eedi2_build_edge_mask( uint8_t * dstp, int dst_pitch, uint8_t *srcp, int src_pitch,
                            int mthresh, int lthresh, int vthresh, int height, int width );

// Expands and smooths out the edge mask by considering a pixel
// to be masked if >= dilation threshold adjacent pixels are masked.
void eedi2_dilate_edge_mask( uint8_t *mskp, int msk_pitch, uint8_t *dstp, int dst_pitch,
                             int dstr, int height, int width );

// Contracts the edge mask by considering a pixel to be masked
// only if > erosion threshold adjacent pixels are masked
void eedi2_erode_edge_mask( uint8_t *mskp, int msk_pitch, uint8_t *dstp, int dst_pitch,
                            int estr, int height, int width );

// Smooths out horizontally aligned holes in the mask
// If none of the 6 horizontally adjacent pixels are masked,
// don't consider the current pixel masked. If there are any
// masked on both sides, consider the current pixel masked.
void eedi2_remove_small_gaps( uint8_t * mskp, int msk_pitch, uint8_t * dstp, int dst_pitch, 
                              int height, int width );

// Spatial vectors. Looks at maximum_search_distance surrounding pixels
// to guess which angle edges follow. This is EEDI2's timesink, and can be
// thought of as YADIF_CHECK on steroids. Both find edge directions.
void eedi2_calc_directions( const int plane, uint8_t * mskp, int msk_pitch, uint8_t * srcp, int src_pitch,
                            uint8_t * dstp, int dst_pitch, int maxd, int nt, int height, int width  );

void eedi2_filter_map( uint8_t *mskp, int msk_pitch, uint8_t *dmskp, int dmsk_pitch,
                       uint8_t * dstp, int dst_pitch, int height, int width );

void eedi2_filter_dir_map( uint8_t * mskp, int msk_pitch, uint8_t * dmskp, int dmsk_pitch, uint8_t * dstp,
                           int dst_pitch, int height, int width );

void eedi2_expand_dir_map( uint8_t * mskp, int msk_pitch, uint8_t  *dmskp, int dmsk_pitch, uint8_t * dstp,
                           int dst_pitch, int height, int width );

void eedi2_mark_directions_2x( uint8_t * mskp, int msk_pitch, uint8_t * dmskp, int dmsk_pitch, uint8_t * dstp,
                               int dst_pitch, int tff, int height, int width );

void eedi2_filter_dir_map_2x( uint8_t * mskp, int msk_pitch, uint8_t * dmskp, int dmsk_pitch, uint8_t * dstp,
                              int dst_pitch, int field, int height, int width );

void eedi2_expand_dir_map_2x( uint8_t * mskp, int msk_pitch, uint8_t * dmskp, int dmsk_pitch, uint8_t * dstp,
                              int dst_pitch, int field, int height, int width );

void eedi2_fill_gaps_2x( uint8_t *mskp, int msk_pitch, uint8_t * dmskp, int dmsk_pitch, uint8_t * dstp,
                         int dst_pitch, int field, int height, int width );

void eedi2_interpolate_lattice( const int plane, uint8_t * dmskp, int dmsk_pitch, uint8_t * dstp,
                                int dst_pitch, uint8_t * omskp, int omsk_pitch, int field, int nt,
                                int height, int width );

void eedi2_post_process( uint8_t * nmskp, int nmsk_pitch, uint8_t * omskp, int omsk_pitch, uint8_t * dstp,
                         int src_pitch, int field, int height, int width );

void eedi2_gaussian_blur1( uint8_t * src, int src_pitch, uint8_t * tmp, int tmp_pitch, uint8_t * dst,
                           int dst_pitch, int height, int width );
                           
void eedi2_gaussian_blur_sqrt2( int *src, int *tmp, int *dst, const int pitch,
                                const int height, const int width );
                                
void eedi2_calc_derivatives( uint8_t *srcp, int src_pitch, int height, int width,
                             int *x2, int *y2, int *xy);

void eedi2_post_process_corner( int *x2, int *y2, int *xy, const int pitch, uint8_t * mskp, int msk_pitch,
                                uint8_t * dstp, int dst_pitch, int height, int width, int field );
