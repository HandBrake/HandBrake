/*
 * nr_weights_embedded — the logical weights compiled into libnr_frame.
 *
 * The weights directory (`weights/` in the source tree, `NR_WEIGHTS_DIR`) holds the logical
 * safetensors as C: one bin2c byte array per slice, each in its own header and translation
 * unit, and a table listing the slices in order. CMake compiles that directory into
 * libnr_frame, and `nr_frame_open(NULL)` reads the safetensors from the table instead of
 * from a file. With `NR_BIN2C_WEIGHTS` and `dlssnr-logical.safetensors` present, CMake
 * regenerates the directory first (slice, then bin2c); without the file it is used as it
 * is. This is the shape the table takes. The slices are contiguous: chunk k holds bytes
 * [sum of the sizes before it, + size) of the file, in order.
 *
 * Nothing here exists in a build without the weights; `NR_EMBEDDED_WEIGHTS` says so.
 */
#ifndef NR_WEIGHTS_EMBEDDED_H
#define NR_WEIGHTS_EMBEDDED_H
#include <stddef.h>

struct nr_embedded_chunk {
    const unsigned char *data;
    size_t size;
};

#ifdef NR_EMBEDDED_WEIGHTS
extern const struct nr_embedded_chunk nr_embedded_weights_chunks[];
extern const size_t nr_embedded_weights_chunk_count;
extern const size_t nr_embedded_weights_size;
#endif

#endif
