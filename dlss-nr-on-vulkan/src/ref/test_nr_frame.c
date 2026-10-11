/*
 * test_nr_frame — `test_nr_frame_c.py`, in C, with no Python in the process.
 *
 *     test_nr_frame [WEIGHTS] [--reference REF.bin]
 *
 * What a C process can check of the C frame library: the weights reader and its refusals,
 * the feature contract channel by channel (transcribed from `features.py`), the graph's
 * determinism byte for byte across replays and extents, the composition against its own
 * formula, the detail split, the temporal floor, and the two-extent rebuild. Real weights
 * when `work/mlxw/dlssnr-logical.safetensors` exists, else random weights of the real
 * shapes written from MLX-DLSS's `weight_spec.json`, as the Python test does
 * (`NR_TEST_SYNTHETIC=1` forces that).
 *
 * What only Python can supply is the *other side*: the Python resident path's head on the
 * same features. `test_nr_frame_c.py --reference REF.bin` writes the features it used and
 * the head Python computed; given that file, this program feeds the same features to the
 * library and requires the same bytes back. Without it, the bit-identity claim is the
 * Python test's to make, and this one says so rather than counting it.
 */
#include "nr_frame.h"
#include "nr_image.h"
#include "nr_portable.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning (disable : 4456)
#endif

static int failures, checks;

static void check(const char *name, int ok, const char *detail)
{
    printf("  [%s] %s  %s\n", ok ? "ok  " : "FAIL", name, detail ? detail : "");
    checks++;
    if (!ok) failures++;
}

static int exists(const char *path) { FILE *f = fopen(path, "rb"); if (f) fclose(f); return f != NULL; }

static float half_round(float f) { return nr_half_round(f); }
static float unitf(float v) { return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v; }

/* ------------------------------------------------------------------------- */
/* a small deterministic random source                                         */
/* ------------------------------------------------------------------------- */

static uint64_t rng_state = 0x9E3779B97F4A7C15ull;
static uint64_t rng_next(void)
{
    uint64_t x = rng_state;
    x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1Dull;
}
static float uniform(void) { return (float)((rng_next() >> 40) * (1.0 / 16777216.0)); }
static float normal(void)
{
    float u = uniform(), v = uniform();
    if (u < 1e-7f) u = 1e-7f;
    return sqrtf(-2.0f * logf(u)) * cosf(6.2831853f * v);
}

/* ------------------------------------------------------------------------- */
/* synthetic weights from weight_spec.json                                      */
/* ------------------------------------------------------------------------- */

/* The spec is `{"format": ..., "tensor_count": N, "tensors": {name: {"shape": [..],
 * "dtype": "F16"}}}`; a scanner for exactly that. */
static const char *skip_ws(const char *p) { while (*p && strchr(" \t\r\n", *p)) p++; return p; }

static const char *read_string(const char *p, char *out, size_t cap)
{
    p = skip_ws(p);
    if (*p != '"') return NULL;
    p++;
    size_t n = 0;
    while (*p && *p != '"') { if (n + 1 < cap) out[n++] = *p; p++; }
    if (*p != '"') return NULL;
    out[n] = 0;
    return p + 1;
}

static int write_synthetic(const char *spec_path, const char *out_path)
{
    FILE *f = fopen(spec_path, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", spec_path); return -1; }
    fseek(f, 0, SEEK_END); long len = ftell(f); fseek(f, 0, SEEK_SET);
    char *text = malloc(len + 1);
    if (fread(text, 1, len, f) != (size_t)len) { fclose(f); free(text); return -1; }
    text[len] = 0;
    fclose(f);
    const char *p = strstr(text, "\"tensors\"");
    if (!p) { free(text); return -1; }
    p = skip_ws(strchr(p, ':') + 1);
    if (*p != '{') { free(text); return -1; }
    p++;
    /* two passes are simpler than growing buffers: first the header, then the data */
    char *header = malloc(1 << 20);
    size_t hlen = (size_t)snprintf(header, 1 << 20, "{\"__metadata__\":{\"fully_logical\":\"true\",\"format\":\"test\"}");
    FILE *data = tmpfile();
    if (!data) { free(text); free(header); return -1; }
    size_t offset = 0;
    char name[256], key[64], dtype[16];
    int count = 0;
    for (;;) {
        p = skip_ws(p);
        if (*p == '}') break;
        if (*p == ',') { p++; continue; }
        p = read_string(p, name, sizeof name);
        if (!p) goto bad;
        p = skip_ws(p); if (*p != ':') goto bad; p = skip_ws(p + 1);
        if (*p != '{') {
            goto bad;
        }
        p++;
        long shape[8]; int ndim = 0; dtype[0] = 0;
        for (;;) {
            p = skip_ws(p);
            if (*p == '}') { p++; break; }
            if (*p == ',') { p++; continue; }
            p = read_string(p, key, sizeof key);
            if (!p) goto bad;
            p = skip_ws(p); if (*p != ':') goto bad; p = skip_ws(p + 1);
            if (!strcmp(key, "shape")) {
                if (*p != '[') {
                    goto bad;
                }
                p++;
                ndim = 0;
                for (;;) {
                    p = skip_ws(p);
                    if (*p == ']') { p++; break; }
                    if (*p == ',') { p++; continue; }
                    char *stop; long d = strtol(p, &stop, 10);
                    if (stop == p || ndim == 8) goto bad;
                    shape[ndim++] = d; p = stop;
                }
            } else if (!strcmp(key, "dtype")) {
                p = read_string(p, dtype, sizeof dtype);
                if (!p) goto bad;
            } else {
                while (*p && *p != ',' && *p != '}') p++;
            }
        }
        size_t n = 1;
        for (int d = 0; d < ndim; d++) n *= (size_t)shape[d];
        int is16 = !strcmp(dtype, "F16");
        size_t len_name = strlen(name);
        int is_scale = len_name >= 10 && !strcmp(name + len_name - 10, "attn_scale");
        int is_cos_sin = (len_name >= 3 && (!strcmp(name + len_name - 3, "sin") || !strcmp(name + len_name - 3, "cos")))
                      || (len_name >= 8 && !strcmp(name + len_name - 8, "cos_skip"));
        float fan = ndim > 1 ? (float)shape[0] : 1.0f;
        size_t bytes = n * (is16 ? 2 : 4);
        for (size_t i = 0; i < n; i++) {
            float v = is_scale ? 0.5f + uniform() : is_cos_sin ? 0.3f + 0.7f * uniform() : normal() / sqrtf(fan);
            if (is16) { uint16_t h = nr_float_to_half(v); fwrite(&h, 2, 1, data); }
            else fwrite(&v, 4, 1, data);
        }
        hlen += (size_t)snprintf(header + hlen, (1 << 20) - hlen, ",\"%s\":{\"dtype\":\"%s\",\"shape\":[", name, dtype);
        for (int d = 0; d < ndim; d++) hlen += (size_t)snprintf(header + hlen, (1 << 20) - hlen, "%s%ld", d ? "," : "", shape[d]);
        hlen += (size_t)snprintf(header + hlen, (1 << 20) - hlen, "],\"data_offsets\":[%zu,%zu]}", offset, offset + bytes);
        offset += bytes;
        count++;
    }
    hlen += (size_t)snprintf(header + hlen, (1 << 20) - hlen, "}");
    while (hlen % 8) header[hlen++] = ' ';
    FILE *out = fopen(out_path, "wb");
    if (!out) goto bad;
    uint64_t hl = hlen;
    unsigned char lenb[8];
    for (int i = 0; i < 8; i++) lenb[i] = (unsigned char)(hl >> (8 * i));
    fwrite(lenb, 1, 8, out);
    fwrite(header, 1, hlen, out);
    rewind(data);
    char buf[1 << 16]; size_t got;
    while ((got = fread(buf, 1, sizeof buf, data)) > 0) fwrite(buf, 1, got, out);
    fclose(out);
    fclose(data);
    free(text); free(header);
    printf("random weights of the real shapes: %d tensors at %s\n", count, out_path);
    return 0;
bad:
    fprintf(stderr, "%s: cannot read the tensor spec\n", spec_path);
    fclose(data); free(text); free(header);
    return -1;
}

/* ------------------------------------------------------------------------- */
/* helpers                                                                      */
/* ------------------------------------------------------------------------- */

static int same(const float *a, const float *b, size_t n) { return memcmp(a, b, n * sizeof(float)) == 0; }

static double max_abs_diff(const float *a, const float *b, size_t n, size_t *differing)
{
    double m = 0.0; size_t d = 0;
    for (size_t i = 0; i < n; i++) { double v = fabs((double)a[i] - (double)b[i]); if (v > 0) d++; if (v > m) m = v; }
    if (differing) *differing = d;
    return m;
}

static int all_finite(const float *a, size_t n) { for (size_t i = 0; i < n; i++) if (!isfinite(a[i])) return 0; return 1; }

static float *random_image(int h, int w) { float *img = malloc((size_t)h * w * 3 * sizeof(float)); for (size_t i = 0; i < (size_t)h * w * 3; i++) img[i] = uniform(); return img; }

/* `NetworkGeometry.extended_indices` */
static int extended(int i, int extent) { int m = 2 * extent - 2 - i; return i < extent ? i : (m > 0 ? m : 0); }
/* `features.scaled_color` */
static float scaled(float c) { return half_round(half_round(half_round(c) - 0.5f) * 0.125f); }

int main(int argc, char **argv)
{
    const char *weights = NULL, *reference = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--reference") && i + 1 < argc) reference = argv[++i];
        else weights = argv[i];
    }
    char synthetic[512] = { 0 };
    const char *real = "work/mlxw/dlssnr-logical.safetensors";
    const char *force = getenv("NR_TEST_SYNTHETIC");
    if (!weights) {
        if (nr_frame_embedded_weights_size() && !(force && atoi(force))) {
            /* `weights` stays NULL: nr_frame_open(NULL) reads the compiled-in slices */
            printf("embedded weights: %zu bytes\n", nr_frame_embedded_weights_size());
        }
        else if (exists(real) && !(force && atoi(force))) { weights = real; printf("real weights: %s\n", real); }
        else {
            snprintf(synthetic, sizeof synthetic, "%s/nr-frame-random-%d.safetensors", nr_temp_dir(), (int)nr_getpid());
            if (write_synthetic("work/mlx-dlss/python/mlxdlss/weight_spec.json", synthetic)) return 2;
            weights = synthetic;
        }
    }

    /* 1. the reader refuses what it must */
    {
        nr_frame *bad = nr_frame_open("/nonexistent/weights.safetensors");
        check("reader: a missing file is refused with a message", bad == NULL && strstr(nr_frame_error(), "cannot open") != NULL, nr_frame_error());
        char path[1200]; snprintf(path, sizeof path, "%s/nr-frame-packed-%d.safetensors", nr_temp_dir(), (int)nr_getpid());
        FILE *f = fopen(path, "wb");
        const char *hdr = "{\"__metadata__\":{\"fully_logical\":\"false\"},\"t\":{\"dtype\":\"F16\",\"shape\":[2],\"data_offsets\":[0,4]}}   ";
        uint64_t hl = strlen(hdr); unsigned char lenb[8];
        for (int i = 0; i < 8; i++) lenb[i] = (unsigned char)(hl >> (8 * i));
        fwrite(lenb, 1, 8, f); fwrite(hdr, 1, hl, f); fwrite("\0\0\0\0", 1, 4, f); fclose(f);
        bad = nr_frame_open(path);
        check("reader: a file without fully_logical=true is refused", bad == NULL && strstr(nr_frame_error(), "fully_logical") != NULL, nr_frame_error());
        remove(path);
        /* NULL asks for the weights compiled in: an error naming the build when there are
         * none, and a readable safetensors — a size that starts with its header — when there are */
        if (!nr_frame_embedded_weights_size()) {
            bad = nr_frame_open(NULL);
            check("reader: NULL without embedded weights is refused with a message", bad == NULL && strstr(nr_frame_error(), "embedded") != NULL, nr_frame_error());
        } else {
            char detail[96]; snprintf(detail, sizeof detail, "%zu bytes compiled in", nr_frame_embedded_weights_size());
            check("reader: this build carries the weights", nr_frame_embedded_weights_size() > 8, detail);
        }
    }

    nr_frame *frame = nr_frame_open(weights);
    if (!frame) { fprintf(stderr, "nr_frame_open: %s\n", nr_frame_error()); return 1; }
    printf("C library on %s: %s\n", nr_frame_device(frame), nr_frame_gemm_path(frame));
    printf("  discrete %d, float16 subnormals kept %d, half_round by cast %d, packed staged copies %d, "
           "packed portable fetch %d, features in the mapped input %d\n",
           nr_frame_discrete(), nr_frame_preserve16(), nr_frame_half_by_cast(), nr_frame_staged_packed(),
           nr_frame_portable_packed(), nr_frame_input_view(frame));
    check("runtime: the five answers about it are a yes or a no once a frame is open",
          (nr_frame_discrete() == 0 || nr_frame_discrete() == 1)
          && (nr_frame_preserve16() == 0 || nr_frame_preserve16() == 1)
          && (nr_frame_half_by_cast() == 0 || nr_frame_half_by_cast() == 1)
          && (nr_frame_staged_packed() == 0 || nr_frame_staged_packed() == 1)
          && (nr_frame_portable_packed() == 0 || nr_frame_portable_packed() == 1), NULL);
    {
        /* on by default where there is a staged kernel (Vulkan, Metal), off under the switch
         * and on Direct3D 12, which has none; the portable fetch is off unless asked for */
        const char *staged = getenv("XMX_STAGED_PACKED"), *portable = getenv("XMX_PORTABLE_PACKED");
        int d3d12 = !strcmp(nr_frame_runtime(), "d3d12");
        check("runtime: the packed staged copies follow XMX_STAGED_PACKED, on by default",
              nr_frame_staged_packed() == (d3d12 || (staged && !strcmp(staged, "0")) ? 0 : 1), NULL);
        check("runtime: the packed portable fetch follows XMX_PORTABLE_PACKED, off by default",
              nr_frame_portable_packed() == (portable && !strcmp(portable, "1") ? 1 : 0), NULL);
    }
    check("runtime: the input view is a yes or a no", nr_frame_input_view(frame) == 0 || nr_frame_input_view(frame) == 1, NULL);
    {
        /* the weights this open read: prepared (the derived tensors in) and, prepared or a
         * plain logical file, every tensor read in place rather than copied — the compiled-in
         * slices are cut at tensor boundaries and a file is mapped whole */
        char detail[128];
        snprintf(detail, sizeof detail, "prepared %d, %zu of %zu tensors read in place",
                 nr_frame_prepared(frame), nr_frame_weights_aliased(frame), nr_frame_weights_count(frame));
        /* a logical file's F32 tensors may sit at an offset only 2-aligned, and those are
         * copied; a prepared file (F32 first) and the compiled-in slices alias every one */
        int all = nr_frame_weights_aliased(frame) == nr_frame_weights_count(frame);
        check("weights: the open says whether they were prepared, and reads them in place",
              (nr_frame_prepared(frame) == 0 || nr_frame_prepared(frame) == 1)
              && nr_frame_weights_count(frame) > 0 && nr_frame_weights_aliased(frame) > 0
              && (all || !nr_frame_prepared(frame)), detail);
        if (!weights)
            check("weights: the compiled-in weights are the prepared format", nr_frame_prepared(frame) == 1, detail);
    }

    int height = 200, width = 176;                       /* network extent 320x320, both mirrored */
    int H, W;
    nr_frame_geometry(height, width, &H, &W);
    char detail[256];
    snprintf(detail, sizeof detail, "%dx%d", W, H);
    check("network extent: at least 320, the vendor's field", H == 320 && W == 320, detail);
    nr_frame_geometry(720, 1280, &H, &W);
    /* the vendor's field: 1280x768 would pool to a bottleneck with no padding token */
    check("network extent: 1280x720 -> 1344x768", H == 768 && W == 1344, NULL);
    nr_frame_geometry(1080, 1920, &H, &W);
    check("network extent: 1920x1080 -> 1920x1152", H == 1152 && W == 1920, NULL);
    nr_frame_geometry(height, width, &H, &W);
    size_t pixels = (size_t)height * width, net = (size_t)H * W;

    float *colour = random_image(height, width);
    float *history = malloc(pixels * 3 * sizeof(float));
    for (size_t i = 0; i < pixels * 3; i++) history[i] = unitf(colour[i] + 0.05f * normal());
    float *previous = malloc(pixels * 3 * sizeof(float));
    memcpy(previous, colour, pixels * 3 * sizeof(float));
    for (int y = 0; y < height; y += 3)                    /* two thirds of the rows unchanged */
        for (int i = 0; i < width * 3; i++) previous[(size_t)y * width * 3 + i] = unitf(previous[(size_t)y * width * 3 + i] + 0.1f);

    nr_frame_params p;
    nr_frame_defaults(&p);

    /* 2. the feature contract, channel by channel */
    float *features = malloc(net * 16 * sizeof(float));
    if (nr_frame_features(frame, colour, height, width, NULL, &p, features)) { fprintf(stderr, "%s\n", nr_frame_error()); return 1; }
    {
        int constant = 1, zero = 1, colours = 1, repeated = 1, controls = 1, noise_half = 1, finite = 1;
        float want_controls[5] = { half_round(0.0f), half_round(1.0f), half_round(1.0f), -1.0f, -1.0f };
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++) {
                const float *o = features + ((size_t)y * W + x) * 16;
                const float *rgb = colour + ((size_t)extended(y, height) * width + extended(x, width)) * 3;
                if (o[3] != 1.0f) constant = 0;
                if (o[15] != 0.0f) zero = 0;
                for (int c = 0; c < 3; c++) {
                    if (o[4 + c] != scaled(rgb[c])) colours = 0;
                    if (o[7 + c] != o[4 + c]) repeated = 0;
                    if (half_round(o[c]) != o[c]) noise_half = 0;
                    if (!isfinite(o[c])) finite = 0;
                }
                for (int c = 0; c < 5; c++) if (o[10 + c] != want_controls[c]) controls = 0;
            }
        check("features: channel 3 is one and channel 15 is zero everywhere", constant && zero, NULL);
        check("features: channels 4-6 are the mirrored, scaled colour", colours, "half(half(half(c) - 0.5) * 0.125) at index 2*extent-2-i past the edge");
        check("features: channels 7-9 repeat the colour without a history", repeated, NULL);
        check("features: channels 10-14 carry the standard profile", controls, NULL);
        check("features: noise is finite and half-valued", finite && noise_half, NULL);
    }
    {
        float *other = malloc(net * 16 * sizeof(float));
        nr_frame_params q = p; q.frame_index = 7;
        nr_frame_features(frame, colour, height, width, NULL, &q, other);
        size_t changed = 0; int rest_same = 1;
        for (size_t px = 0; px < net; px++) {
            for (int c = 0; c < 3; c++) if (features[px * 16 + c] != other[px * 16 + c]) changed++;
            if (!same(features + px * 16 + 3, other + px * 16 + 3, 13)) rest_same = 0;
        }
        snprintf(detail, sizeof detail, "%zu of %zu noise values change", changed, net * 3);
        check("features: the frame index changes the noise and nothing else", changed > net * 3 * 9 / 10 && rest_same, detail);
        nr_frame_features(frame, colour, height, width, NULL, &p, other);
        check("features: the same request twice gives the same bytes", same(features, other, net * 16), NULL);
        nr_frame_params s = p; s.normalized_style = 1.0f / 128; s.local_structure = 1.5f;
        nr_frame_features(frame, colour, height, width, NULL, &s, other);
        int placed = 1;
        for (size_t px = 0; px < net; px++) {
            const float *o = other + px * 16;
            if (o[10] != half_round(1.0f / 128) || o[12] != half_round(1.5f)) placed = 0;
        }
        check("features: style and structure land in channels 10 and 12, half-rounded", placed, NULL);
        nr_frame_features(frame, colour, height, width, history, &p, other);
        int hist = 1;
        for (int y = 0; y < H && hist; y++)
            for (int x = 0; x < W; x++) {
                const float *o = other + ((size_t)y * W + x) * 16;
                const float *was = history + ((size_t)extended(y, height) * width + extended(x, width)) * 3;
                for (int c = 0; c < 3; c++) if (o[7 + c] != scaled(was[c])) hist = 0;
            }
        check("features with history: channels 7-9 are the mirrored, scaled history", hist, NULL);
        free(other);
    }

    /* 3. the graph: deterministic byte for byte, and the same through both entry points */
    float *head = malloc(net * 4 * sizeof(float)), *head2 = malloc(net * 4 * sizeof(float));
    if (nr_frame_run_features(frame, features, H, W, head)) { fprintf(stderr, "%s\n", nr_frame_error()); return 1; }
    check("head: finite", all_finite(head, net * 4), NULL);
    nr_frame_run_features(frame, features, H, W, head2);
    check("head: the captured graph replays the same bytes", same(head, head2, net * 4), NULL);
    {
        double sd = 0, mean = 0;
        for (size_t i = 0; i < net * 4; i++) mean += head[i];
        mean /= (double)(net * 4);
        for (size_t i = 0; i < net * 4; i++) sd += (head[i] - mean) * (head[i] - mean);
        sd = sqrt(sd / (double)(net * 4));
        snprintf(detail, sizeof detail, "mean %+.4f sd %.4f", mean, sd);
        check("head: not constant", sd > 1e-6, detail);
        snprintf(detail, sizeof detail, "write %.1f ms, graph %.1f ms, read %.1f ms", nr_frame_split(frame, 0) * 1e3,
                 nr_frame_split(frame, 1) * 1e3, nr_frame_split(frame, 2) * 1e3);
        check("head: the split accounts for the run", nr_frame_split(frame, 1) > 0.0, detail);
    }
    float *output = malloc(pixels * 3 * sizeof(float)), *head_out = malloc(pixels * 4 * sizeof(float));
    if (nr_frame_update(frame, colour, height, width, NULL, NULL, &p, output, head_out)) { fprintf(stderr, "%s\n", nr_frame_error()); return 1; }
    {
        int cropped = 1;
        for (int y = 0; y < height && cropped; y++)
            for (int x = 0; x < width; x++)
                if (!same(head_out + ((size_t)y * width + x) * 4, head + ((size_t)y * W + x) * 4, 4)) cropped = 0;
        check("update: the head it hands back is the graph's, cropped", cropped, NULL);
    }

    /* 4. the composition against its own formula: `compose_head`, transcribed */
    {
        float *want = malloc(pixels * 3 * sizeof(float));
        const float intensities[3] = { 0.5f, 1.0f, 1.66f };
        for (int k = 0; k < 3; k++) {
            float blend = intensities[k] > 1.0f ? intensities[k] : unitf(intensities[k]);
            for (size_t px = 0; px < pixels; px++)
                for (int c = 0; c < 3; c++) {
                    float source = colour[px * 3 + c];
                    float predicted = unitf(source + half_round(head_out[px * 4 + c]) * 0.25f);
                    want[px * 3 + c] = unitf(source + blend * (predicted - source));
                }
            nr_frame_params q = p; q.intensity = intensities[k];
            nr_frame_update(frame, colour, height, width, NULL, NULL, &q, output, NULL);
            snprintf(detail, sizeof detail, "intensity %.2f", intensities[k]);
            check("compose: source + blend * (clip(source + half(head) / 4) - source), bit for bit", same(want, output, pixels * 3), detail);
        }
        nr_frame_params z = p; z.intensity = 0.0f;
        nr_frame_update(frame, colour, height, width, NULL, NULL, &z, output, NULL);
        check("compose: intensity 0 returns the source exactly", same(output, colour, pixels * 3), NULL);
        int inside = 1;
        for (size_t i = 0; i < pixels * 3; i++) if (output[i] < 0.0f || output[i] > 1.0f) inside = 0;
        check("compose: the output stays in [0, 1]", inside, NULL);
        free(want);
    }

    /* 5. the detail split */
    {
        float *plain = malloc(pixels * 3 * sizeof(float)), *split = malloc(pixels * 3 * sizeof(float));
        nr_frame_update(frame, colour, height, width, NULL, NULL, &p, plain, NULL);
        nr_frame_params q = p; q.detail_strength = 1.0f; q.colour_strength = 1.0f;
        nr_frame_update(frame, colour, height, width, NULL, NULL, &q, split, NULL);
        check("detail: strengths of 1 and 1 leave the composition untouched", same(plain, split, pixels * 3), NULL);
        q.detail_strength = 1.5f; q.colour_strength = 0.0f;
        nr_frame_update(frame, colour, height, width, NULL, NULL, &q, split, NULL);
        size_t d; double m = max_abs_diff(plain, split, pixels * 3, &d);
        snprintf(detail, sizeof detail, "max |d| %.3e over %zu values", m, d);
        check("detail: 1.5 / 0 changes the frame and keeps it finite", d > 0 && all_finite(split, pixels * 3), detail);
        /* with no change to split there is nothing to split: a head of zeros composes to
         * the source and the split of zero is zero */
        nr_frame_params r = p; r.intensity = 0.0f; r.detail_strength = 2.0f; r.colour_strength = 0.5f;
        nr_frame_update(frame, colour, height, width, NULL, NULL, &r, split, NULL);
        check("detail: splitting a zero change returns the source", same(split, colour, pixels * 3), NULL);
        free(plain); free(split);
    }

    /* 6. the temporal path: the history's head, the gate, the floor */
    {
        float *hist_features = malloc(net * 16 * sizeof(float)), *hist_head = malloc(net * 4 * sizeof(float));
        float *temporal = malloc(pixels * 3 * sizeof(float)), *floored = malloc(pixels * 3 * sizeof(float));
        float *t_head = malloc(pixels * 4 * sizeof(float));
        nr_frame_features(frame, colour, height, width, history, &p, hist_features);
        nr_frame_run_features(frame, hist_features, H, W, hist_head);
        nr_frame_update(frame, colour, height, width, history, NULL, &p, temporal, t_head);
        int cropped = 1;
        for (int y = 0; y < height && cropped; y++)
            for (int x = 0; x < width; x++)
                if (!same(t_head + ((size_t)y * width + x) * 4, hist_head + ((size_t)y * W + x) * 4, 4)) cropped = 0;
        check("temporal: the head is the graph's on the history features", cropped, NULL);
        /* the composition, transcribed: alpha = clip(sigmoid(half(logit)) * half(scale), 0, 1) */
        float *want = malloc(pixels * 3 * sizeof(float));
        float scale = half_round(p.blend_scale);
        for (size_t px = 0; px < pixels; px++) {
            float logit = half_round(t_head[px * 4 + 3]);
            float alpha = unitf(1.0f / (1.0f + expf(-logit)) * scale);
            for (int c = 0; c < 3; c++) {
                float source = colour[px * 3 + c];
                float predicted = unitf(source + half_round(t_head[px * 4 + c]) * 0.25f);
                predicted += alpha * (history[px * 3 + c] - predicted);
                want[px * 3 + c] = unitf(source + 1.0f * (predicted - source));
            }
        }
        check("temporal: the gate is sigmoid(half(logit)) * half(blend_scale), composed as the NumPy does", same(want, temporal, pixels * 3), NULL);
        nr_frame_params z = p; z.history_confidence = 0.0f;
        nr_frame_update(frame, colour, height, width, history, NULL, &z, floored, NULL);
        for (size_t px = 0; px < pixels; px++)
            for (int c = 0; c < 3; c++) {
                float source = colour[px * 3 + c];
                float predicted = unitf(source + half_round(t_head[px * 4 + c]) * 0.25f);
                predicted += 0.0f * (history[px * 3 + c] - predicted);
                want[px * 3 + c] = unitf(source + (predicted - source));
            }
        check("temporal: confidence 0 is the still path with the history's head", same(want, floored, pixels * 3), NULL);
        nr_frame_params fl = p; fl.hold = 0.6f; fl.slope = -0.6f * 255.0f / 4.0f;
        nr_frame_update(frame, colour, height, width, history, previous, &fl, floored, NULL);
        double held = 0, loose = 0; size_t n = 0;
        for (int y = 1; y < height; y += 3)                 /* rows the previous frame left unchanged */
            for (int i = 0; i < width * 3; i++) {
                size_t k = (size_t)y * width * 3 + i;
                held += fabs(floored[k] - history[k]); loose += fabs(temporal[k] - history[k]); n++;
            }
        snprintf(detail, sizeof detail, "mean |out - history| %.5f with the floor, %.5f without", held / n, loose / n);
        check("temporal: the floor holds unchanged rows closer to the history than the gate alone", held < loose, detail);
        /* Where colour + 0.1 clipped at 1 in all three channels the game moved the pixel
         * by less than the ramp, and the floor holds it rightly; those pixels are not what
         * this check is about (one of them decides it on the vendor's graph). */
        int moved_rows_same = 1;
        for (int y = 0; y < height; y += 3)
            for (int x = 0; x < width; x++) {
                size_t k = ((size_t)y * width + x) * 3;
                float moved = 0.0f;
                for (int c = 0; c < 3; c++) {
                    float step = fabsf(previous[k + c] - colour[k + c]);
                    if (step > moved) moved = step;
                }
                if (moved * 255.0f < 4.0f) continue;
                for (int c = 0; c < 3; c++)
                    if (floored[k + c] != temporal[k + c]) moved_rows_same = 0;
            }
        check("temporal: rows the game changed by 0.1 take no floor", moved_rows_same, "the floor is gone by four levels of 255");
        /* the release: where the game's pixel moved by 16 levels of 255 or more none of the
         * gate survives, so the pixel is the confidence-0 frame exactly; where it did not
         * move the release keeps all of the gate, and the floor's frame is untouched */
        float *still = malloc(pixels * 3 * sizeof(float)), *released = malloc(pixels * 3 * sizeof(float));
        nr_frame_update(frame, colour, height, width, history, NULL, &z, still, NULL);
        nr_frame_params rl = fl; rl.release = (float)(-255.0 / 16.0);
        nr_frame_update(frame, colour, height, width, history, previous, &rl, released, NULL);
        int dropped = 1, kept = 1; size_t gone = 0;
        for (size_t px = 0; px < pixels; px++) {
            float moved = 0.0f;
            for (int c = 0; c < 3; c++) {
                float step = fabsf(colour[px * 3 + c] - previous[px * 3 + c]);
                if (step > moved) moved = step;
            }
            for (int c = 0; c < 3; c++) {
                size_t k = px * 3 + c;
                if (moved == 0.0f && released[k] != floored[k]) kept = 0;
                if (moved * rl.release + 1.0f <= 0.0f && released[k] != still[k]) dropped = 0;
            }
            gone += moved * rl.release + 1.0f <= 0.0f;
        }
        snprintf(detail, sizeof detail, "%zu of %zu pixels released", gone, pixels);
        check("temporal: released pixels are the confidence-0 frame, bit for bit", dropped && gone > pixels / 10, detail);
        check("temporal: the release leaves unchanged pixels to the gate and the floor", kept, NULL);
        free(still); free(released);
        free(hist_features); free(hist_head); free(temporal); free(floored); free(t_head); free(want);
    }

    /* 7. a second extent rebuilds the graph and keeps the weights; the first comes back */
    {
        float *first = malloc(pixels * 3 * sizeof(float));
        nr_frame_update(frame, colour, height, width, NULL, NULL, &p, first, NULL);
        int sh = 96, sw = 128;
        float *small = random_image(sh, sw);
        float *out1 = malloc((size_t)sh * sw * 3 * sizeof(float)), *out2 = malloc((size_t)sh * sw * 3 * sizeof(float));
        if (nr_frame_update(frame, small, sh, sw, NULL, NULL, &p, out1, NULL)) { fprintf(stderr, "%s\n", nr_frame_error()); return 1; }
        nr_frame_update(frame, small, sh, sw, NULL, NULL, &p, out2, NULL);
        check("second extent: renders, finite, and replays the same bytes", all_finite(out1, (size_t)sh * sw * 3) && same(out1, out2, (size_t)sh * sw * 3), NULL);
        nr_frame_update(frame, colour, height, width, NULL, NULL, &p, output, NULL);
        check("back to the first extent: the same bytes as before the rebuild", same(first, output, pixels * 3), NULL);
        free(first); free(small); free(out1); free(out2);
    }

    /* 7b. min_extent: the graph's own floor, where the bottleneck is 16 tokens */
    {
        int mh, mw;
        nr_frame_geometry_min(height, width, 128, &mh, &mw);
        snprintf(detail, sizeof detail, "%dx%d", mw, mh);
        check("min_extent 128: the network extent is 256x192", mh == 256 && mw == 192, detail);
        nr_frame_geometry_min(height, width, 0, &mh, &mw);
        check("min_extent 0: the vendor's 320", mh == 320 && mw == 320, NULL);
        nr_frame_params q = p;
        q.min_extent = 128;
        float *out1 = malloc(pixels * 3 * sizeof(float)), *out2 = malloc(pixels * 3 * sizeof(float));
        if (nr_frame_update(frame, colour, height, width, NULL, NULL, &q, out1, NULL)) { fprintf(stderr, "%s\n", nr_frame_error()); return 1; }
        nr_frame_update(frame, colour, height, width, NULL, NULL, &q, out2, NULL);
        check("min_extent 128: renders, finite, and replays the same bytes",
              all_finite(out1, pixels * 3) && same(out1, out2, pixels * 3), NULL);
        free(out1); free(out2);
    }

    /* 7c. the daemon's end of a frame in one pass: a smaller head brought up, composed and
     * encoded, against the upscale, nr_frame_compose and nr_encode8 one after the other */
    {
        int hh = height * 3 / 5, hw = width * 3 / 5, fw = width + 10;
        float *low_head = malloc((size_t)hh * hw * 4 * sizeof(float));
        float *up = malloc(pixels * 4 * sizeof(float));
        float *middle = malloc((size_t)height * hw * 4 * sizeof(float));
        float *fused = malloc(pixels * 3 * sizeof(float)), *want = malloc(pixels * 3 * sizeof(float));
        for (size_t i = 0; i < (size_t)hh * hw * 4; i++) low_head[i] = normal();
        int32_t *ly = malloc(height * sizeof(int32_t)), *hy = malloc(height * sizeof(int32_t));
        int32_t *lx = malloc(width * sizeof(int32_t)), *hx = malloc(width * sizeof(int32_t));
        float *wy = malloc(height * sizeof(float)), *wx = malloc(width * sizeof(float));
        for (int axis = 0; axis < 2; axis++) {
            int extent = axis ? hw : hh, count = axis ? width : height;
            int32_t *lo = axis ? lx : ly, *hi = axis ? hx : hy;
            float *wt = axis ? wx : wy, ratio = (float)((double)extent / (double)count);
            for (int i = 0; i < count; i++) {
                float centre = ((float)i + 0.5f) * ratio - 0.5f, fl = floorf(centre);
                int l = fl < 0.0f ? 0 : fl > (float)(extent - 1) ? extent - 1 : (int)fl;
                lo[i] = l; hi[i] = l + 1 > extent - 1 ? extent - 1 : l + 1;
                float fr = centre - (float)l;
                wt[i] = fr < 0.0f ? 0.0f : fr > 1.0f ? 1.0f : fr;
            }
        }
        nr_resize_axis(low_head, (ptrdiff_t)hw * 4, 4, 1, (size_t)height, (size_t)hw, 4, 0, ly, hy, wy, middle);
        nr_resize_axis(middle, (ptrdiff_t)hw * 4, 4, 1, (size_t)height, (size_t)width, 4, 1, lx, hx, wx, up);
        size_t frame_bytes = (size_t)(height + 6) * fw * 4;
        uint8_t *request = malloc(frame_bytes), *got = malloc(frame_bytes), *expect = malloc(frame_bytes);
        for (size_t i = 0; i < frame_bytes; i++) request[i] = (uint8_t)(i * 2654435761u >> 24);
        const char *names[3] = { "still", "history", "history, floor and release" };
        int all = 1;
        for (int c = 0; c < 3; c++) {
            nr_frame_params q = p;
            const float *hist = c ? history : NULL, *prev = c == 2 ? previous : NULL;
            if (c == 2) { q.hold = 0.6f; q.slope = -0.6f * 255.0f / 4.0f; q.release = -255.0f / 16.0f; }
            memcpy(got, request, frame_bytes);
            memcpy(expect, request, frame_bytes);
            int bad = nr_frame_compose_encode(frame, low_head, hh, hw, colour, height, width, hist, prev, NULL, &q,
                                              fused, got, fw, 3, 5, 1)
                      || nr_frame_compose(frame, up, colour, height, width, hist, prev, NULL, &q, want);
            for (int y = 0; y < height && !bad; y++) {
                uint8_t *row = expect + ((size_t)(3 + y) * fw + 5) * 4;
                nr_encode8(want + (size_t)y * width * 3, (ptrdiff_t)width * 3, 3, 1, row, 1, (size_t)width, 1, row);
            }
            int ok = !bad && same(fused, want, pixels * 3) && !memcmp(got, expect, frame_bytes);
            all &= ok;
            snprintf(detail, sizeof detail, "%s", names[c]);
            check("compose_encode: one pass, the separate passes' composition and bytes", ok, detail);
        }
        (void)all;
        free(low_head); free(up); free(middle); free(fused); free(want); free(ly); free(hy); free(lx); free(hx);
        free(wy); free(wx); free(request); free(got); free(expect);
    }

    /* 8. the other side, when Python wrote it down: the same features, the same head */
    if (reference) {
        FILE *f = fopen(reference, "rb");
        uint32_t hdr[4] = { 0 };
        if (!f || fread(hdr, 4, 4, f) != 4 || hdr[0] != 0x4E524631u) {
            check("reference: readable", 0, reference);
        } else {
            int rh = (int)hdr[1], rw = (int)hdr[2];
            size_t rn = (size_t)rh * rw;
            float *rf = malloc(rn * 16 * sizeof(float)), *rhd = malloc(rn * 4 * sizeof(float)), *got = malloc(rn * 4 * sizeof(float));
            int ok = fread(rf, sizeof(float), rn * 16, f) == rn * 16 && fread(rhd, sizeof(float), rn * 4, f) == rn * 4;
            fclose(f);
            check("reference: readable", ok, reference);
            if (ok) {
                if (nr_frame_run_features(frame, rf, rh, rw, got)) { fprintf(stderr, "%s\n", nr_frame_error()); return 1; }
                size_t d; double m = max_abs_diff(rhd, got, rn * 4, &d);
                snprintf(detail, sizeof detail, "%dx%d: %zu of %zu values differ, max |d| %.3e", rw, rh, d, rn * 4, m);
                check("reference: the head is bit-identical to the Python resident path", same(rhd, got, rn * 4), detail);
            }
            free(rf); free(rhd); free(got);
        }
    } else {
        printf("  [skip] reference: no Python head to compare against (test_nr_frame_c.py --reference REF.bin writes one) — a skip is not a pass\n");
    }

    nr_frame_close(frame);
    free(colour); free(history); free(previous); free(features); free(head); free(head2); free(output); free(head_out);
    if (synthetic[0]) remove(synthetic);
    if (failures) { printf("\n%d of %d checks failed\n", failures, checks); return 1; }
    printf("\nnr_frame in C: %d checks, the library against its own contract%s\n", checks,
           reference ? ", and against the Python head" : "");
    return 0;
}
