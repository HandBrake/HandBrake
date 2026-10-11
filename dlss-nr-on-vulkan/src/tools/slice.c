/*
 * slice — copy LENGTH bytes of IN starting at OFFSET into OUT.
 *
 *     slice IN OUT OFFSET LENGTH
 *
 * The one job bin2c does not do. A 292 MB byte array in a single C initializer costs a
 * compiler about 90 bytes of memory per element (clang: 1.4 GB on a 16 MB file), so the
 * weights are embedded as a run of slices, each its own header and its own translation
 * unit, and this is what cuts them. Nothing seeks: the offset is skipped by reading, so a
 * 32-bit `long` on Windows is never asked to hold a file position.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char buf[1 << 16];

static int fail(const char *what, const char *name)
{
    perror(name ? name : what);
    if (name) fprintf(stderr, "slice: %s\n", what);
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "usage: slice IN OUT OFFSET LENGTH\n");
        return 2;
    }
    unsigned long long offset = strtoull(argv[3], NULL, 10);
    unsigned long long length = strtoull(argv[4], NULL, 10);
    FILE *in = fopen(argv[1], "rb");
    if (!in) return fail("cannot open the input", argv[1]);
    FILE *out = fopen(argv[2], "wb");
    if (!out) { fclose(in); return fail("cannot create the output", argv[2]); }

    while (offset) {
        size_t want = offset < sizeof buf ? (size_t)offset : sizeof buf;
        size_t got = fread(buf, 1, want, in);
        if (got == 0) { fclose(in); fclose(out); fprintf(stderr, "slice: %s ends before offset %llu\n", argv[1], offset); return 1; }
        offset -= got;
    }
    unsigned long long copied = 0;
    while (copied < length) {
        size_t want = length - copied < sizeof buf ? (size_t)(length - copied) : sizeof buf;
        size_t got = fread(buf, 1, want, in);
        if (got == 0) break;
        if (fwrite(buf, 1, got, out) != got) { fclose(in); fclose(out); return fail("short write", argv[2]); }
        copied += got;
    }
    fclose(in);
    if (fclose(out)) return fail("cannot finish the output", argv[2]);
    if (copied != length) {
        fprintf(stderr, "slice: %s holds %llu bytes past the offset, not %llu\n", argv[1], copied, length);
        return 1;
    }
    return 0;
}
