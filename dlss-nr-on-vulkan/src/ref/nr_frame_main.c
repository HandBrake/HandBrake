/*
 * nr_frame — `src/ref/nr_frame.py`, the command, in C.
 *
 *     nr_frame IN.png OUT.png [--weights W] [--size HxW] [--profile P] [--style-index N]
 *              [--local-tone F] [--local-structure F] [--skin-structure F] [--auto-mask F]
 *              [--control-mask MASK.png] [--intensity F] [--intensity-ladder A,B,C]
 *              [--detail-strength F] [--colour-strength F] [--detail-radius F]
 *              [--frame-index N] [-v]
 *
 * The same flags, the same printed lines. Pictures are PNG, read and written with libpng:
 * any PNG in (8- or 16-bit, grey, palette, alpha — all reduced to 8-bit RGB), 8-bit RGB
 * out, the same `byte / 255` and `value * 255 + 0.5` as `image_io.py`. Where the Python
 * shells out to ImageMagick this does not, so `--size` is this project's own bilinear
 * resample (`nr_resize_axis`, the one the daemon uses) rather than ImageMagick's filter.
 * `--gpu`, `--resident` and `--accel` are accepted and mean nothing here: this program has
 * one backend, the resident graph in `libnr_frame`. The network runs once; the ladder
 * composes the head at each intensity as the Python does.
 *
 *     nr_frame --replay DUMP OUT [--render-scale F] [--temporal F] [--hold F] [--release F]
 *              [--cut-limit F] [--min-extent N] [--no-letterbox] [--profile P] [--intensity F] ...
 *
 * `src/bench/restill.py` without the daemon: a `nr_daemon.py --dump` capture's NNN_in.png
 * frames, in their order, through `nr_frame_live` — the daemon's frame, its letterbox, its
 * render scale and its history — into OUT as NNN_in.png and NNN_out.png, numbered from 001
 * as the daemon's own dump numbers them. The render scale is 1 unless set, as restill's is;
 * the other knobs are the daemon's defaults. It runs on whichever runtime is behind
 * libnr_frame (NR_GPU_BACKEND), so a capture can be re-rendered on Metal or Direct3D 12.
 */
#include "nr_frame.h"
#include "nr_image.h"
#include "nr_portable.h"

#include <png.h>
#include <setjmp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <dirent.h>
#endif

static double now(void) { return nr_now(); }

static void die(const char *what)
{
    fprintf(stderr, "nr_frame: %s\n", what);
    exit(1);
}

/* `image_io.load`, through libpng: (H, W, 3) float32 in [0, 1]. Sixteen-bit files are
 * reduced to eight as ImageMagick's `-depth 8` reduces them; grey, palette and alpha are
 * expanded or dropped. */

/* png_create_*_struct returns NULL when the libpng the binary was compiled against is not
 * the one it is running with (libpng refuses across major.minor); the file is not the
 * problem then, the build is, and the message should say so. */
static void png_version_mismatch(void)
{
    unsigned lib = png_access_version_number();
    if (lib / 100 != PNG_LIBPNG_VER / 100) {
        fprintf(stderr, "libpng mismatch: built against %s, running with %u.%u.%u; rebuild against the "
                        "headers of the libpng that is linked\n", PNG_LIBPNG_VER_STRING, lib / 10000, (lib / 100) % 100, lib % 100);
        exit(1);
    }
}
static unsigned char *read_png_bytes(const char *path, int *height, int *width)
{
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    FILE *f = NULL;
    fopen_s(&f, path, "rb");
#else
    FILE *f = fopen(path, "rb");
#endif

    if (!f) { perror(path); exit(1); }
    unsigned char sig[8];
    if (fread(sig, 1, 8, f) != 8 || png_sig_cmp(sig, 0, 8)) { fprintf(stderr, "%s: not a PNG\n", path); exit(1); }
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) png_version_mismatch();
    png_infop info = png_create_info_struct(png);
    if (!png || !info || setjmp(png_jmpbuf(png))) { fprintf(stderr, "%s: libpng could not read it\n", path); exit(1); }
    png_init_io(png, f);
    png_set_sig_bytes(png, 8);
    png_read_info(png, info);
    png_uint_32 w = png_get_image_width(png, info), h = png_get_image_height(png, info);
    int depth = png_get_bit_depth(png, info), type = png_get_color_type(png, info);
    if (type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (type == PNG_COLOR_TYPE_GRAY && depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (depth == 16) png_set_strip_16(png);
    if (type == PNG_COLOR_TYPE_GRAY || type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);
    png_set_strip_alpha(png);
    png_set_packing(png);
    png_read_update_info(png, info);
    if (png_get_rowbytes(png, info) != (size_t)w * 3) { fprintf(stderr, "%s: unexpected row layout\n", path); exit(1); }
    unsigned char *raw = malloc((size_t)w * h * 3);
    png_bytep *rows = malloc(h * sizeof *rows);
    if (!raw || !rows) die("out of memory");
    for (png_uint_32 y = 0; y < h; y++) rows[y] = raw + (size_t)y * w * 3;
    png_read_image(png, rows);
    png_read_end(png, NULL);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(f);
    free(rows);
    *height = (int)h; *width = (int)w;
    return raw;
}

static float *read_png(const char *path, int *height, int *width)
{
    unsigned char *raw = read_png_bytes(path, height, width);
    size_t n = (size_t)*width * *height * 3;
    float *out = malloc(n * sizeof(float));
    if (!out) die("out of memory");
    for (size_t i = 0; i < n; i++) out[i] = (float)raw[i] / 255.0f;
    free(raw);
    return out;
}

/* `nr_image.bilinear`: the axis plan of `_axis_plan` — pixel centres mapped onto the
 * source, floor and its neighbour clamped, the fraction as the weight — then one axis at
 * a time through `nr_resize_axis`, the intermediate rounded to float32 between them. */
static float *resize(const float *source, int height, int width, int channels, int target_h, int target_w)
{
    const float *current = source;
    float *owned = NULL;
    int h = height, w = width;
    for (int axis = 0; axis < 2; axis++) {
        int extent = axis == 0 ? h : w, count = axis == 0 ? target_h : target_w;
        if (extent == count) continue;
        int32_t *low = malloc((size_t)count * sizeof *low), *high = malloc((size_t)count * sizeof *high);
        float *weight = malloc((size_t)count * sizeof *weight);
        if (!low || !high || !weight) die("out of memory");
        for (int i = 0; i < count; i++) {
            float centre = ((float)i + 0.5f) * ((float)extent / (float)count) - 0.5f;
            float floored = floorf(centre);
            int lo = (int)(floored < 0.0f ? 0.0f : floored > (float)(extent - 1) ? (float)(extent - 1) : floored);
            low[i] = lo;
            high[i] = lo + 1 > extent - 1 ? extent - 1 : lo + 1;
            float frac = centre - (float)lo;
            weight[i] = frac < 0.0f ? 0.0f : frac > 1.0f ? 1.0f : frac;
        }
        int nh = axis == 0 ? count : h, nw = axis == 0 ? w : count;
        float *out = malloc((size_t)nh * nw * channels * sizeof(float));
        if (!out) die("out of memory");
        nr_resize_axis(current, (ptrdiff_t)w * channels, channels, 1, (size_t)nh, (size_t)nw, (size_t)channels,
                       axis, low, high, weight, out);
        free(low); free(high); free(weight); free(owned);
        owned = out; current = out; h = nh; w = nw;
    }
    if (!owned) {
        owned = malloc((size_t)h * w * channels * sizeof(float));
        if (!owned) die("out of memory");
        memcpy(owned, source, (size_t)h * w * channels * sizeof(float));
    }
    return owned;
}

static float *load_image(const char *path, int *height, int *width, int resize_h, int resize_w)
{
    float *image = read_png(path, height, width);
    if (resize_h && (resize_h != *height || resize_w != *width)) {
        float *resized = resize(image, *height, *width, 3, resize_h, resize_w);
        free(image);
        image = resized;
        *height = resize_h; *width = resize_w;
    }
    return image;
}

/* An 8-bit RGB PNG of (height, width, 3) bytes. */
static void save_png_bytes(const unsigned char *raw, int height, int width, const char *path)
{
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    FILE *f = NULL;
    fopen_s(&f, path, "wb");
#else
    FILE *f = fopen(path, "wb");
#endif
    if (!f) { perror(path); exit(1); }
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) png_version_mismatch();
    png_infop info = png_create_info_struct(png);
    if (!png || !info || setjmp(png_jmpbuf(png))) { fprintf(stderr, "%s: libpng could not write it\n", path); exit(1); }
    png_init_io(png, f);
    png_set_IHDR(png, info, (png_uint_32)width, (png_uint_32)height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);
    png_bytep *rows = malloc((size_t)height * sizeof *rows);
    if (!rows) die("out of memory");
    for (int y = 0; y < height; y++) rows[y] = (png_bytep)raw + (size_t)y * width * 3;
    png_write_image(png, rows);
    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(f);
    free(rows);
}

/* `image_io.save`: clip, `(a * 255 + 0.5)` to bytes, an 8-bit RGB PNG. */
static void save_image(const float *image, int height, int width, const char *path)
{
    size_t n = (size_t)height * width * 3;
    unsigned char *raw = malloc(n);
    if (!raw) die("out of memory");
    for (size_t i = 0; i < n; i++) {
        float v = image[i];
        v = v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
        raw[i] = (unsigned char)(v * 255.0f + 0.5f);
    }
    save_png_bytes(raw, height, width, path);
    free(raw);
}

static int readable(const char *path)
{
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    FILE *f = NULL;
    fopen_s(&f, path, "rb");
#else
    FILE *f = fopen(path, "rb");
#endif
    if (!f) return 0;
    fclose(f);
    return 1;
}

/* The weights compiled into libnr_frame when the build has them (NULL asks for those), else
 * `ROOT / work / mlxw / dlssnr-logical.safetensors`: the weights are the owner's and live in
 * work/ whichever build made this executable: beside it when the Makefile put it in work/,
 * one directory up and over when CMake put it in `build/`, else relative to the caller. */
static const char *default_weights(char *buf, size_t cap)
{
    char dir[1024];
    if (nr_frame_embedded_weights_size()) return NULL;
    if (nr_dl_self_dir((const void *)&default_weights, dir, sizeof dir) == 0) {
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
        sprintf_s(buf, cap, "%s/mlxw/dlssnr-logical.safetensors", dir);
#else
        snprintf(buf, cap, "%s/mlxw/dlssnr-logical.safetensors", dir);
#endif

        if (readable(buf)) return buf;
        
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
        sprintf_s(buf, cap, "%s/../work/mlxw/dlssnr-logical.safetensors", dir);
#else
        snprintf(buf, cap, "%s/../work/mlxw/dlssnr-logical.safetensors", dir);
#endif

        if (readable(buf)) return buf;
    }
    
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    sprintf_s(buf, cap, "work/mlxw/dlssnr-logical.safetensors");
#else
    snprintf(buf, cap, "work/mlxw/dlssnr-logical.safetensors");
#endif

    return buf;
}

static const char *PROFILES[] = { "standard", "natural", "cinematic", "neutral", "vendor" };

static int profile(const char *name, nr_frame_params *p)
{
    /* `features.PROFILES` and `nr_frame.VENDOR_DEFAULTS` */
    if (!strcmp(name, "standard"))  { p->normalized_style = 0.0f;        p->local_tone = 1.0f; p->local_structure = 1.0f; return 0; }
    if (!strcmp(name, "natural"))   { p->normalized_style = 1.0f / 128;  p->local_tone = 1.0f; p->local_structure = 1.0f; return 0; }
    if (!strcmp(name, "cinematic")) { p->normalized_style = 2.0f / 128;  p->local_tone = 1.0f; p->local_structure = 1.0f; return 0; }
    if (!strcmp(name, "neutral"))   { p->normalized_style = 0.0f;        p->local_tone = 0.0f; p->local_structure = 0.0f; return 0; }
    if (!strcmp(name, "vendor"))    { p->normalized_style = 0.0f;        p->local_tone = 1.0f; p->local_structure = 1.5f; return 0; }
    return -1;
}

/* ------------------------------------------------------------------------- */
/* --replay: a --dump capture through nr_frame_live, in its order             */
/* ------------------------------------------------------------------------- */

static void join(char *out, size_t cap, const char *dir, const char *name)
{
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    sprintf_s(out, cap, "%s/%s", dir, name);
#else
    snprintf(out, cap, "%s/%s", dir, name);
#endif
}

/* The number of a daemon's NNN_in.png, or -1 for anything else in the folder. */
static long dump_number(const char *name)
{
    const char *p = name;
    if (*p < '0' || *p > '9') return -1;
    long n = 0;
    while (*p >= '0' && *p <= '9') { n = n * 10 + (*p - '0'); p++; if (n > 100000000L) return -1; }
    return strcmp(p, "_in.png") ? -1 : n;
}

static int compare_long(const void *a, const void *b)
{
    long x = *(const long *)a, y = *(const long *)b;
    return (x > y) - (x < y);
}

/* Each entry of `dir` through `visit`; 0 when the folder could be read. */
static int list_dir(const char *dir, void (*visit)(const char *name, void *context), void *context)
{
#ifdef _WIN32
    char pattern[1200];
    join(pattern, sizeof pattern, dir, "*");
    struct _finddata_t found;
    intptr_t handle = _findfirst(pattern, &found);
    if (handle == -1) return errno == ENOENT ? 0 : -1;
    do visit(found.name, context); while (_findnext(handle, &found) == 0);
    _findclose(handle);
#else
    DIR *d = opendir(dir);
    if (!d) return -1;
    for (struct dirent *e; (e = readdir(d));) visit(e->d_name, context);
    closedir(d);
#endif
    return 0;
}

struct numbers { long *values; size_t count, cap; int others; };

static void collect(const char *name, void *context)
{
    struct numbers *n = context;
    if (!strcmp(name, ".") || !strcmp(name, "..")) return;
    long number = dump_number(name);
    if (number < 0) { n->others++; return; }
    if (n->count == n->cap) {
        n->cap = n->cap ? n->cap * 2 : 64;
        n->values = realloc(n->values, n->cap * sizeof *n->values);
        if (!n->values) die("out of memory");
    }
    n->values[n->count++] = number;
}

static void make_dir(const char *dir)
{
#ifdef _WIN32
    int rc = _mkdir(dir);
#else
    int rc = mkdir(dir, 0777);
#endif
    if (rc && errno != EEXIST) { perror(dir); exit(1); }
}

static int replay(nr_frame *frame, const nr_frame_params *p, const nr_frame_live_settings *s,
                  const char *source, const char *out)
{
    struct numbers frames = { 0 };
    if (list_dir(source, collect, &frames)) { perror(source); return 1; }
    if (!frames.count) { fprintf(stderr, "no NNN_in.png in %s\n", source); return 1; }
    qsort(frames.values, frames.count, sizeof *frames.values, compare_long);
    for (size_t k = 1; k < frames.count; k++)
        if (frames.values[k] != frames.values[0] + (long)k) {
            /* a gap means frames the daemon saw and this replay would not: its history differs */
            fprintf(stderr, "%s: the frames are not consecutive (%ld-%ld, %u of them)\n", source,
                    frames.values[0], frames.values[frames.count - 1], (unsigned)frames.count);
            return 1;
        }
    make_dir(out);
    struct numbers there = { 0 };
    if (list_dir(out, collect, &there)) { perror(out); return 1; }
    if (there.count || there.others) {
        fprintf(stderr, "%s is not empty: the dump is numbered from 001, as the daemon's is\n", out);
        return 1;
    }
    free(there.values);

    nr_frame_live *live = nr_frame_live_open(frame);
    if (!live) { fprintf(stderr, "nr_frame_live_open: %s\n", nr_frame_error()); return 1; }
    printf("  %s -> %s, %u frames, render scale %g, temporal %g, hold %g, release %g, cut limit %g\n",
           source, out, (unsigned)frames.count, (double)s->render_scale, (double)s->temporal, (double)s->hold,
           (double)s->release, (double)s->cut_limit);
    fflush(stdout);
    for (size_t k = 0; k < frames.count; k++) {
        char name[64], path[1200];
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
        sprintf_s(name, sizeof name, "%03ld_in.png", frames.values[k]);
#else
        snprintf(name, sizeof name, "%03ld_in.png", frames.values[k]);
#endif
        join(path, sizeof path, source, name);
        int height, width;
        unsigned char *rgb = read_png_bytes(path, &height, &width);
        size_t pixels = (size_t)height * width;
        /* every swapchain behind the captures so far was B8G8R8A8, and the dumps are 8-bit,
         * so the bytes sent are the bytes the game sent */
        unsigned char *bgra = malloc(pixels * 4), *answer = malloc(pixels * 4), *shown = malloc(pixels * 3);
        if (!bgra || !answer || !shown) die("out of memory");
        for (size_t i = 0; i < pixels; i++) {
            bgra[i * 4 + 0] = rgb[i * 3 + 2]; bgra[i * 4 + 1] = rgb[i * 3 + 1];
            bgra[i * 4 + 2] = rgb[i * 3 + 0]; bgra[i * 4 + 3] = 255;
        }
        nr_frame_live_report r;
        double started = now();
        if (nr_frame_live_run(live, p, s, bgra, width, height, 1, answer, &r)) {
            fprintf(stderr, "%s: nr_frame_live_run: %s\n", path, nr_frame_error());
            return 1;
        }
        double elapsed = now() - started;
        for (size_t i = 0; i < pixels; i++) {
            shown[i * 3 + 0] = answer[i * 4 + 2]; shown[i * 3 + 1] = answer[i * 4 + 1];
            shown[i * 3 + 2] = answer[i * 4 + 0];
        }
        char in_path[1200], out_path[1200], in_name[64], out_name[64];
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
        sprintf_s(in_name, sizeof in_name, "%03u_in.png", (unsigned)(k + 1));
        sprintf_s(out_name, sizeof out_name, "%03u_out.png", (unsigned)(k + 1));
#else
        snprintf(in_name, sizeof in_name, "%03u_in.png", (unsigned)(k + 1));
        snprintf(out_name, sizeof out_name, "%03u_out.png", (unsigned)(k + 1));
#endif
        join(in_path, sizeof in_path, out, in_name);
        join(out_path, sizeof out_path, out, out_name);
        save_png_bytes(rgb, height, width, in_path);
        save_png_bytes(shown, height, width, out_path);
        int active_width = r.right - r.left, active_height = r.bottom - r.top;
        printf("  %s -> %s  %dx%d in %.2fs  change %.5f  %s %.4f  network %dx%d scale %g",
               name, out_name, width, height, elapsed, r.change,
               r.with_history ? "history, cut" : "no history, cut", r.cut, r.network_width,
               r.network_height, (double)s->render_scale);
        if (s->render_scale < 1.0f && r.render_width != (int)lround((double)active_width * s->render_scale))
            printf(" (runs as %.3g)", (double)r.render_width / (double)active_width);
        if (active_width != width || active_height != height)
            printf("  letterbox %dpx of rows and %dpx of columns skipped", height - active_height,
                   width - active_width);
        printf("\n");
        fflush(stdout);
        free(rgb); free(bgra); free(answer); free(shown);
    }
    nr_frame_live_close(live);
    free(frames.values);
    return 0;
}

static void usage(void)
{
    fprintf(stderr,
        "usage: nr_frame IN OUT [--weights W] [--size HxW] [--profile standard|natural|cinematic|neutral|vendor]\n"
        "                (W defaults to the weights compiled into libnr_frame, else work/mlxw/dlssnr-logical.safetensors)\n"
        "                [--style-index N] [--local-tone F] [--local-structure F] [--skin-structure F]\n"
        "                [--auto-mask F] [--control-mask MASK] [--intensity F] [--intensity-ladder A,B,..]\n"
        "                [--detail-strength F] [--colour-strength F] [--detail-radius F] [--frame-index N] [-v]\n"
        "       nr_frame --replay DUMP OUT [--render-scale F] [--temporal F] [--hold F] [--release F]\n"
        "                [--cut-limit F] [--min-extent N] [--no-letterbox] [--profile P] [--intensity F] ...\n"
        "                (a nr_daemon.py --dump capture through the daemon's frame, history and all;\n"
        "                 render scale 1 unless set, the other knobs the daemon's defaults)\n"
        "       nr_frame --prepare OUT.safetensors [--weights W]\n"
        "                (the weights written with the loaders' layout work done, for faster opens;\n"
        "                 derived from NVIDIA's weights, so never committed or redistributed)\n");
    exit(2);
}

int main(int argc, char **argv)
{
    const char *input = NULL, *output = NULL, *weights = NULL, *size = NULL, *ladder = NULL, *mask_path = NULL;
    const char *profile_name = "standard";
    int verbose = 0, have_style = 0, style_index = 0, have_tone = 0, have_structure = 0;
    float local_tone = 0, local_structure = 0;
    int have_skin = 0, have_auto = 0;
    float skin = -1.0f, automatic = -1.0f;
    nr_frame_params p;
    nr_frame_defaults(&p);
    int replaying = 0;
    const char *prepare_out = NULL;
    nr_frame_live_settings live;
    nr_frame_live_defaults(&live);

#define NEXT() (i + 1 < argc ? argv[++i] : (usage(), (char *)0))
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "--weights")) weights = NEXT();
        else if (!strcmp(a, "--size")) size = NEXT();
        else if (!strcmp(a, "--profile")) profile_name = NEXT();
        else if (!strcmp(a, "--style-index")) { have_style = 1; style_index = atoi(NEXT()); }
        else if (!strcmp(a, "--local-tone")) { have_tone = 1; local_tone = (float)atof(NEXT()); }
        else if (!strcmp(a, "--local-structure")) { have_structure = 1; local_structure = (float)atof(NEXT()); }
        else if (!strcmp(a, "--skin-structure")) { have_skin = 1; skin = (float)atof(NEXT()); }
        else if (!strcmp(a, "--auto-mask")) { have_auto = 1; automatic = (float)atof(NEXT()); }
        else if (!strcmp(a, "--control-mask")) mask_path = NEXT();
        else if (!strcmp(a, "--intensity")) p.intensity = (float)atof(NEXT());
        else if (!strcmp(a, "--intensity-ladder")) ladder = NEXT();
        else if (!strcmp(a, "--detail-strength")) p.detail_strength = (float)atof(NEXT());
        else if (!strcmp(a, "--colour-strength")) p.colour_strength = (float)atof(NEXT());
        else if (!strcmp(a, "--detail-radius")) p.detail_radius = (float)atof(NEXT());
        else if (!strcmp(a, "--frame-index")) p.frame_index = atoi(NEXT());
        else if (!strcmp(a, "--replay")) replaying = 1;
        else if (!strcmp(a, "--prepare")) prepare_out = NEXT();
        else if (!strcmp(a, "--render-scale")) live.render_scale = (float)atof(NEXT());
        else if (!strcmp(a, "--temporal")) live.temporal = (float)atof(NEXT());
        else if (!strcmp(a, "--hold")) live.hold = (float)atof(NEXT());
        else if (!strcmp(a, "--release")) live.release = (float)atof(NEXT());
        else if (!strcmp(a, "--cut-limit")) live.cut_limit = (float)atof(NEXT());
        else if (!strcmp(a, "--min-extent")) p.min_extent = atoi(NEXT());
        else if (!strcmp(a, "--no-letterbox")) live.letterbox = 0;
        else if (!strcmp(a, "--gpu") || !strcmp(a, "--resident") || !strcmp(a, "--accel"))
            ;   /* one backend here: the resident graph */
        else if (!strcmp(a, "-v") || !strcmp(a, "--verbose")) verbose = 1;
        else if (a[0] == '-' && a[1]) usage();
        else if (!input) input = a;
        else if (!output) output = a;
        else usage();
    }
    if (prepare_out) {
        /* no device: the weights in, the prepared file out */
        if (nr_frame_prepare(weights, prepare_out)) {
            fprintf(stderr, "nr_frame_prepare: %s\n", nr_frame_error());
            return 1;
        }
        fprintf(stderr, "prepared weights written to %s\n", prepare_out);
        return 0;
    }
    if (!input || !output) usage();
    if (profile(profile_name, &p)) { fprintf(stderr, "unknown profile %s; one of", profile_name);
        for (size_t k = 0; k < sizeof PROFILES / sizeof *PROFILES; k++) fprintf(stderr, " %s", PROFILES[k]);
        fprintf(stderr, "\n"); return 2; }
    /* `nr_frame.controls`: the profile, then the explicit overrides */
    if (have_style) p.normalized_style = (float)style_index / 128.0f;
    if (have_tone) p.local_tone = local_tone;
    if (have_structure) p.local_structure = local_structure;
    if (have_skin || have_auto) { p.automatic_mask = 1; p.skin_structure = skin; p.automatic_structure = automatic; }

    char default_path[1200];
    if (!weights) weights = default_weights(default_path, sizeof default_path);

    double started = now();
    nr_frame *frame = nr_frame_open(weights);
    if (!frame) { fprintf(stderr, "nr_frame_open: %s\n", nr_frame_error()); return 1; }
    printf("resident backend ready in %.1fs (%s) on %s\n", now() - started, weights ? weights : "embedded weights",
           nr_frame_device(frame));
    fflush(stdout);

    if (replaying) {
        if (!(live.render_scale >= 0.05f && live.render_scale <= 1.0f)) die("--render-scale is 0.05-1");
        if (!(live.temporal >= 0.0f && live.temporal <= 1.0f) || !(live.hold >= 0.0f && live.hold <= 1.0f)
            || !(live.cut_limit >= 0.0f && live.cut_limit <= 1.0f)) die("--temporal, --hold and --cut-limit are 0-1");
        if (!(live.release >= 0.0f && live.release <= 255.0f)) die("--release is 0-255");
        if (p.min_extent < 128 || p.min_extent > 4096) die("--min-extent is 128-4096, as the daemon takes it");
        int rc = replay(frame, &p, &live, input, output);
        nr_frame_close(frame);
        return rc;
    }

    int height, width, resize_h = 0, resize_w = 0;
    if (size && sscanf(size, "%dx%d", &resize_h, &resize_w) != 2) usage();
    float *colour = load_image(input, &height, &width, resize_h, resize_w);
    printf("input %dx%d\n", width, height);
    fflush(stdout);
    float *mask = NULL;
    if (mask_path) {
        int mh, mw;
        mask = load_image(mask_path, &mh, &mw, 0, 0);
        if (mh != height || mw != width) die("the control mask must match the colour image shape");
    }

    int H, W;
    nr_frame_geometry(height, width, &H, &W);
    if (verbose) { printf("  network extent %dx%d for output %dx%d\n", W, H, width, height); fflush(stdout); }
    size_t pixels = (size_t)height * width;
    float *features = malloc((size_t)H * W * 16 * sizeof(float));
    float *wide = malloc((size_t)H * W * 4 * sizeof(float));
    float *head = malloc(pixels * 4 * sizeof(float));
    float *composed = malloc(pixels * 3 * sizeof(float));
    if (!features || !wide || !head || !composed) die("out of memory");

    started = now();
    if (nr_frame_features_masked(frame, colour, height, width, NULL, mask, &p, features)
        || nr_frame_run_features(frame, features, H, W, wide)) {
        fprintf(stderr, "%s\n", nr_frame_error());
        return 1;
    }
    for (int y = 0; y < height; y++)               /* `geometry.crop(head)` */
        memcpy(head + (size_t)y * width * 4, wide + (size_t)y * W * 4, (size_t)width * 4 * sizeof(float));
    double elapsed = now() - started;
    printf("network %.1fs\n", elapsed);
    if (verbose)
        printf("    write %.1f ms, graph %.1f ms, read %.1f ms\n", nr_frame_split(frame, 0) * 1e3,
               nr_frame_split(frame, 1) * 1e3, nr_frame_split(frame, 2) * 1e3);
    double lo = head[0], hi = head[0], sum = 0, sumsq = 0;
    for (size_t i = 0; i < pixels * 4; i++) {
        double v = head[i];
        if (v < lo) lo = v;
        if (v > hi) hi = v;
        sum += v; sumsq += v * v;
    }
    double mean = sum / (double)(pixels * 4);
    printf("head    min %+.4f max %+.4f sd %.4f\n", lo, hi, sqrt(sumsq / (double)(pixels * 4) - mean * mean));
    fflush(stdout);

    /* one render, one file per intensity */
    float ladder_values[64];
    int count = 0;
    if (ladder) {
        char *copy = nr_strdup(ladder), *save = NULL;
        for (char *tok = nr_strtok_r(copy, ",", &save); tok && count < 64; tok = nr_strtok_r(NULL, ",", &save))
            ladder_values[count++] = (float)atof(tok);
        free(copy);
    } else {
        ladder_values[count++] = p.intensity;
    }
    for (int k = 0; k < count; k++) {
        nr_frame_params q = p;
        q.intensity = ladder_values[k];
        if (nr_frame_compose(frame, head, colour, height, width, NULL, NULL, mask, &q, composed)) {
            fprintf(stderr, "nr_frame_compose: %s\n", nr_frame_error());
            return 1;
        }
        double total = 0, worst = 0;
        for (size_t i = 0; i < pixels * 3; i++) {
            double d = fabs((double)composed[i] - (double)colour[i]);
            total += d;
            if (d > worst) worst = d;
        }
        printf("intensity %5.2f  change mean|d| %.5f max|d| %.5f\n", q.intensity, total / (double)(pixels * 3), worst);
        char path[1200];
        if (ladder) {
            /* `stem_i{value:g}{suffix}` */
            const char *dot = strrchr(output, '.');
            const char *slash = strrchr(output, '/');
            if (dot && slash && dot < slash) dot = NULL;
            int stem = dot ? (int)(dot - output) : (int)strlen(output);

#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
            sprintf_s(path, sizeof path, "%.*s_i%g%s", stem, output, (double)q.intensity, dot ? dot : "");
#else
            snprintf(path, sizeof path, "%.*s_i%g%s", stem, output, (double)q.intensity, dot ? dot : "");
#endif
        } else {
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
            sprintf_s(path, sizeof path, "%s", output);
#else
            snprintf(path, sizeof path, "%s", output);
#endif
        }
        save_image(composed, height, width, path);
        printf("wrote %s\n", path);
        fflush(stdout);
    }
    nr_frame_close(frame);
    free(colour); free(mask); free(features); free(wide); free(head); free(composed);
    return 0;
}
