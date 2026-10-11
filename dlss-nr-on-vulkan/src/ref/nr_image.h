/* The CPU image passes in nr_image.c, for callers in C. nr_image.py binds the same
 * functions for NumPy; the contract there — byte-identical to the NumPy they replace —
 * is the contract here. */
#ifndef NR_IMAGE_H
#define NR_IMAGE_H
#include <stddef.h>
#include <stdint.h>

/* Every pass splits its rows across the host threads (nr_image.c's pool); a caller in C
 * may hand it its own row loop too. `fn` gets contiguous [y0, y1) bands — four rows at a
 * time, taken by whichever thread is free — and must not depend on which thread runs a
 * band or in what order the bands run. */
typedef void (*nr_rows_fn)(const void *args, size_t y0, size_t y1);
void nr_parallel_rows(size_t rows, nr_rows_fn fn, const void *args);
unsigned nr_host_threads(void);

void nr_decode8(const uint8_t *source, size_t pixels, int bgra, float *output);
void nr_encode8(const float *image, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                const uint8_t *raw, size_t height, size_t width, int bgra, uint8_t *output);
/* `grade`, NULL or `nr_frame.grade_for`'s three factors (exposure multiplier, contrast,
 * saturation multiplier), is the vendor's colour grade for a style; `neural`, NULL or RGB
 * triples of the output's extent, receives the prediction after the history's blend and
 * before the grade and the intensity, truncated to half — the vendor's history for the
 * next frame (notes/phase70-post-process.md). With both NULL nothing changes by a bit. */
void nr_compose(const float *head, ptrdiff_t hy, ptrdiff_t hx, ptrdiff_t hc,
                const float *colour, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                size_t height, size_t width, float intensity,
                const float *grade, float *neural, float *output);
/* One pixel's end of the composition, for a caller with its own loop (nr_frame.c's masked
 * still path): `predicted` (the prediction after any history blend) kept in `neural` as half
 * and graded when those are given, then blended by `blend` towards `rgb` into `out`. */
void nr_finish_pixel(float predicted[3], const float *rgb, float blend, const float *grade,
                     float *neural, float *out);
void nr_features(const float *colour, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                 const float *history, ptrdiff_t ty, ptrdiff_t tx, ptrdiff_t tc,
                 const int32_t *rows, const int32_t *columns,
                 size_t height, size_t width, const float *noise,
                 const float *controls, float *output);
/* nr_features stored as half bit patterns — what the graph's first GEMM reads — and
 * float32 to half on its own; both round to nearest even, as the GPU's to_half does. */
void nr_features_half(const float *colour, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                      const float *history, ptrdiff_t ty, ptrdiff_t tx, ptrdiff_t tc,
                      const int32_t *rows, const int32_t *columns,
                      size_t height, size_t width, const float *noise,
                      const float *controls, uint16_t *output);
void nr_to_half(const float *source, size_t count, uint16_t *target);
/* The area mean of a downscale by whole factors fy x fx, summed as nr_daemon.resample's
 * NumPy does; height and width are the output's. */
void nr_area_mean(const float *source, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                  size_t height, size_t width, size_t channels, size_t fy, size_t fx,
                  float *output);
void nr_resize_axis(const float *source, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                    size_t height, size_t width, size_t channels, int axis,
                    const int32_t *low, const int32_t *high,
                    const float *weight, float *output);
void nr_compose_temporal(const float *head, ptrdiff_t hy, ptrdiff_t hx, ptrdiff_t hc,
                         const float *colour, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                         const float *history, ptrdiff_t ry, ptrdiff_t rx, ptrdiff_t rc,
                         const float *previous, ptrdiff_t py, ptrdiff_t px, ptrdiff_t pc,
                         const float *gate, ptrdiff_t gy, ptrdiff_t gx,
                         const float *table, float confidence,
                         const float *mask, ptrdiff_t my, ptrdiff_t mx,
                         size_t height, size_t width, float intensity,
                         float scale, float hold, float slope, float release,
                         const float *grade, float *neural, float *output);
/* The head's bilinear upscale to the colour's extent, then nr_compose_temporal (a history)
 * or nr_compose (none), then nr_encode8 into `encoded` at (top, left) of a frame
 * `frame_width` wide — one pass, the same bytes as the three. The axis plans are
 * nr_image.py's `_axis_plan`, NULL for an axis already at the colour's extent; `channels`
 * is 4 with a history and 3 without. `samples`, with a nonzero `step`, receives the
 * upscaled head on every step-th row and column. */
void nr_compose_encode(const float *head, ptrdiff_t hy, ptrdiff_t hx, ptrdiff_t hc,
                       size_t head_width, size_t channels,
                       const int32_t *low_y, const int32_t *high_y, const float *weight_y,
                       const int32_t *low_x, const int32_t *high_x, const float *weight_x,
                       const float *colour, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                       const float *history, ptrdiff_t ry, ptrdiff_t rx, ptrdiff_t rc,
                       const float *previous, ptrdiff_t py, ptrdiff_t px, ptrdiff_t pc,
                       const float *table, float confidence,
                       const float *mask, ptrdiff_t my, ptrdiff_t mx,
                       size_t height, size_t width, float intensity,
                       float scale, float hold, float slope, float release,
                       float *output, const float *grade, float *neural,
                       uint8_t *encoded, size_t frame_width,
                       size_t top, size_t left, int bgra, float *samples, size_t step);
#endif
