/*
 * dump_embedded_weights — write the logical safetensors compiled into libnr_frame back
 * out as a file.
 *
 *     dump_embedded_weights OUT.safetensors
 *
 * The Python graph (`nr_frame.py`, `frame_profile.py`, every test under src/gpu) reads
 * `work/mlxw/dlssnr-prepared.safetensors` (else the logical file), and a checkout without
 * `work/` has the weights only inside the library (`weights/`, through bin2c) — since
 * 2026-10-10 in the prepared form, so write them to the prepared name. This walks the chunk
 * table the library exports and concatenates it, which is the file byte for byte: the slices
 * are contiguous and in order (`nr_weights_embedded.h`). Linked against libnr_frame.
 */
#include <stdio.h>
#include <stdlib.h>

#define NR_EMBEDDED_WEIGHTS 1
#include "../ref/nr_weights_embedded.h"

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: dump_embedded_weights OUT.safetensors\n");
        return 2;
    }
    FILE *f = fopen(argv[1], "wb");
    if (!f) { perror(argv[1]); return 1; }
    size_t total = 0;
    for (size_t i = 0; i < nr_embedded_weights_chunk_count; i++) {
        const struct nr_embedded_chunk *c = &nr_embedded_weights_chunks[i];
        if (fwrite(c->data, 1, c->size, f) != c->size) { perror("write"); return 1; }
        total += c->size;
    }
    if (fclose(f)) { perror("close"); return 1; }
    if (total != nr_embedded_weights_size) {
        fprintf(stderr, "chunks sum to %zu bytes, table says %zu\n", total, nr_embedded_weights_size);
        return 1;
    }
    printf("%s: %zu bytes in %zu chunks\n", argv[1], total, nr_embedded_weights_chunk_count);
    return 0;
}
