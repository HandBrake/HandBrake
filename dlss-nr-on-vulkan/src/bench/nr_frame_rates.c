/*
 * nr_frame_rates — `src/bench/live_rates.py` for the C frame library.
 *
 * The daemon's frame, the way a game pays for it, but through `libnr_frame` in one
 * process instead of `nr_daemon.py` behind a socket: the 8-bit decode, the downscale to the
 * render scale (the area mean when the scale divides the extent, else the separable
 * bilinear), the features and the graph at that extent, the head brought back up, the
 * composition at the *swapchain's* resolution with the history, its floor and its release,
 * and the 8-bit encode — the last three in one pass (`nr_frame_compose_encode`), as the
 * daemon runs them since upstream's `nr_compose_encode`; `--separate` measures them as three
 * passes, the same bytes. `--min-extent N` is the daemon's `min_extent` knob. What it leaves out is the socket and the Python between the
 * stages, so the difference from `live_rates.py` on the same machine is what those cost.
 *
 *     nr_frame_rates                                   # the published table's cases
 *     nr_frame_rates 800x600@0.5 --frames 9 -v         # one case, a line per stage
 *     nr_frame_rates --pan 4                           # a moving shot: the history is used
 *
 * The frames are live_rates.py's recipe — a coarse 16-pixel grid under fine noise — from
 * this file's own generator, so not the same bytes. By default each frame is a fresh one,
 * as there: the shot "cuts" every frame, the history is dropped, and the temporal path is
 * not taken, which is what the published table measures. `--pan N` instead slides one
 * frame N columns a present, so the history survives the cut test and the temporal
 * composition runs, as it does in a game. `--still` hands back the same frame each time.
 * The first `--warmup` frames are discarded: they build the extent's buffers.
 *
 * Prints the table and the literal for `nr_knobs.RATES`. Timing is wall time around each
 * whole frame (`nr_now`), the median and the band; `-v` adds the median of each stage and
 * the graph's write+run+read split from `nr_frame_split`, like the daemon's `gpu` field.
 */
#include "nr_frame.h"
#include "nr_image.h"
#include "nr_portable.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { STAGE_DECODE, STAGE_DOWN, STAGE_HEAD, STAGE_UP, STAGE_COMPOSE, STAGE_ENCODE, STAGES };
static const char *STAGE_NAMES[STAGES] = { "decode", "down", "network", "up", "compose", "encode" };

typedef struct { int width, height; float scale; } plan_case;

/* live_rates.PLAN */
static const plan_case PLAN[] = {
    { 512, 288, 0.35f }, { 512, 288, 0.50f }, { 640, 360, 0.35f }, { 640, 360, 0.50f },
    { 854, 480, 0.50f }, { 1024, 768, 0.55f }, { 1920, 1080, 0.55f },
};

static void die(const char *what)
{
    fprintf(stderr, "nr_frame_rates: %s\n", what);
    exit(1);
}

static void *alloc(size_t bytes)
{
    void *p = malloc(bytes ? bytes : 1);
    if (!p) die("out of memory");
    return p;
}

/* splitmix64: a generator of our own, so the frames do not depend on a libc's rand() */
static uint64_t mix(uint64_t *state)
{
    uint64_t z = (*state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static double uniform(uint64_t *state) { return (double)(mix(state) >> 11) * (1.0 / 9007199254740992.0); }

/* live_rates.frame: structure at 16 pixels under fine noise, so the letterbox finder would
 * see no bars and the detail passes have something to work on. BGRA, alpha 255. */
static void make_frame(uint8_t *out, int width, int height, uint64_t seed)
{
    uint64_t state = seed * 0x2545F4914F6CDD1Dull + 1;
    int gw = width / 16 + 2, gh = height / 16 + 2;
    double *grid = alloc((size_t)gw * gh * 3 * sizeof *grid);
    for (size_t i = 0; i < (size_t)gw * gh * 3; i++) grid[i] = uniform(&state);
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) {
            const double *g = grid + ((size_t)(y / 16) * gw + (size_t)(x / 16)) * 3;
            uint8_t *o = out + ((size_t)y * width + x) * 4;
            for (int c = 0; c < 3; c++) {
                double v = g[c] * 0.7 + uniform(&state) * 0.3;
                v = v < 0.0 ? 0.0 : v > 1.0 ? 1.0 : v;
                o[2 - c] = (uint8_t)(v * 255.0);          /* RGB into BGRA */
            }
            o[3] = 255;
        }
    free(grid);
}

/* One bilinear axis, `nr_daemon.resample`'s `axis()`: pixel centres onto the source, the
 * floor and its neighbour clamped, the fraction as the weight. */
typedef struct { int32_t *low, *high; float *weight; int count; } axis_plan;

static axis_plan plan_axis(int extent, int count)
{
    axis_plan a = { alloc((size_t)count * sizeof(int32_t)), alloc((size_t)count * sizeof(int32_t)),
                    alloc((size_t)count * sizeof(float)), count };
    for (int i = 0; i < count; i++) {
        float centre = ((float)i + 0.5f) * ((float)extent / (float)count) - 0.5f;
        float floored = floorf(centre);
        int lo = (int)(floored < 0.0f ? 0.0f : floored > (float)(extent - 1) ? (float)(extent - 1) : floored);
        a.low[i] = lo;
        a.high[i] = lo + 1 > extent - 1 ? extent - 1 : lo + 1;
        float frac = centre - (float)lo;
        a.weight[i] = frac < 0.0f ? 0.0f : frac > 1.0f ? 1.0f : frac;
    }
    return a;
}

static void free_axis(axis_plan *a) { free(a->low); free(a->high); free(a->weight); memset(a, 0, sizeof *a); }

/* `nr_daemon.resample` from (h, w, c) to (th, tw, c): the area mean when the target
 * divides the source evenly and is smaller, else rows then columns through
 * `nr_resize_axis`, with `middle` (th, w, c) between them. Plans are made once a case. */
typedef struct {
    int h, w, th, tw, channels, area;
    axis_plan rows, cols;
    float *middle;
} resampler;

static resampler make_resampler(int h, int w, int th, int tw, int channels)
{
    resampler r;
    memset(&r, 0, sizeof r);
    r.h = h; r.w = w; r.th = th; r.tw = tw; r.channels = channels;
    r.area = th && tw && h % th == 0 && w % tw == 0 && h > th && w > tw;
    if (!r.area && (h != th || w != tw)) {
        if (h != th) r.rows = plan_axis(h, th);
        if (w != tw) r.cols = plan_axis(w, tw);
        if (h != th && w != tw) r.middle = alloc((size_t)th * w * channels * sizeof(float));
    }
    return r;
}

static void free_resampler(resampler *r) { free_axis(&r->rows); free_axis(&r->cols); free(r->middle); r->middle = NULL; }

static void resample(const resampler *r, const float *source, float *out)
{
    size_t c = (size_t)r->channels;
    if (r->h == r->th && r->w == r->tw) {
        memcpy(out, source, (size_t)r->h * r->w * c * sizeof(float));
    } else if (r->area) {
        nr_area_mean(source, (ptrdiff_t)(r->w * c), (ptrdiff_t)c, 1, (size_t)r->th, (size_t)r->tw, c,
                     (size_t)(r->h / r->th), (size_t)(r->w / r->tw), out);
    } else {
        const float *current = source;
        if (r->h != r->th) {
            float *target = r->w != r->tw ? r->middle : out;
            nr_resize_axis(current, (ptrdiff_t)(r->w * c), (ptrdiff_t)c, 1, (size_t)r->th, (size_t)r->w, c,
                           0, r->rows.low, r->rows.high, r->rows.weight, target);
            current = target;
        }
        if (r->w != r->tw)
            nr_resize_axis(current, (ptrdiff_t)(r->w * c), (ptrdiff_t)c, 1, (size_t)r->th, (size_t)r->tw, c,
                           1, r->cols.low, r->cols.high, r->cols.weight, out);
    }
}

typedef struct {
    int frames, warmup, verbose, pan, still;
    float temporal, hold, release, cut_limit;
    int min_extent, separate;
} options;

static int compare(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static double median(double *values, int count)
{
    qsort(values, (size_t)count, sizeof *values, compare);
    return values[count / 2];
}

typedef struct { double middle, low, high; int with_history; } result;

static result measure(nr_frame *frame, const plan_case *pc, const options *o)
{
    int width = pc->width, height = pc->height;
    int iw = width, ih = height;
    /* the daemon's inner frame: the scale's own, or the largest on the same network field
     * (`nr_frame.render_extent`) */
    if (pc->scale < 1.0f) nr_frame_render_extent(width, height, pc->scale, o->min_extent, &iw, &ih);
    size_t pixels = (size_t)width * height, inner_pixels = (size_t)iw * ih;
    int total = o->frames + o->warmup;

    nr_frame_params p;
    nr_frame_defaults(&p);
    int temporal = o->temporal > 0.0f;
    p.history_confidence = o->temporal;
    p.hold = o->hold;
    p.slope = (float)(-255.0f * o->hold / 4.0f);               /* nr_frame.HOLD_RAMP */
    p.release = o->release > 0.0f ? (float)(-255.0f / o->release) : 0.0f;  /* release_slope */
    int want_previous = o->hold > 0.0f || o->release > 0.0f;
    p.min_extent = o->min_extent;

    /* Frames are made before the clock starts, as live_rates builds its payload before it
     * connects. One base frame, wider by the whole pan, for --pan and --still. */
    int base_width = width + (o->pan > 0 ? o->pan * total : 0);
    uint8_t *base = NULL;
    uint8_t **payloads = alloc((size_t)total * sizeof *payloads);
    if (o->pan > 0 || o->still) {
        base = alloc((size_t)base_width * height * 4);
        make_frame(base, base_width, height, 1);
    }
    for (int i = 0; i < total; i++) {
        payloads[i] = alloc(pixels * 4);
        if (base) {
            for (int y = 0; y < height; y++)
                memcpy(payloads[i] + (size_t)y * width * 4,
                       base + ((size_t)y * base_width + (size_t)(o->pan * i)) * 4, (size_t)width * 4);
        } else {
            make_frame(payloads[i], width, height, (uint64_t)i + 1);
        }
    }
    free(base);

    resampler down = make_resampler(height, width, ih, iw, 3);
    resampler up = make_resampler(ih, iw, height, width, 4);
    resampler history_down = make_resampler(height, width, ih, iw, 3);
    float *colour = alloc(pixels * 3 * sizeof(float));
    float *inner = alloc(inner_pixels * 3 * sizeof(float));
    float *head_inner = alloc(inner_pixels * 4 * sizeof(float));
    float *head = alloc(pixels * 4 * sizeof(float));
    float *output = alloc(pixels * 3 * sizeof(float));
    float *history_inner = alloc(inner_pixels * 3 * sizeof(float));
    /* what History keeps: the vendor's history — the prediction before the grade and the
     * intensity, truncated to half (`nr_frame_compose_encode_neural`) — the inner source for
     * the cut test, the game's own frame for the floor and the release */
    float *kept_output = alloc(pixels * 3 * sizeof(float));
    float *neural = alloc(pixels * 3 * sizeof(float));
    float *kept_source = alloc(inner_pixels * 3 * sizeof(float));
    float *kept_pixels = alloc(pixels * 3 * sizeof(float));
    int have_history = 0;
    uint8_t *encoded = alloc(pixels * 4);

    double *times = alloc((size_t)o->frames * sizeof *times);
    double *stages[STAGES], *split[3];
    for (int s = 0; s < STAGES; s++) stages[s] = alloc((size_t)o->frames * sizeof(double));
    for (int s = 0; s < 3; s++) split[s] = alloc((size_t)o->frames * sizeof(double));
    int with_history = 0;

    for (int i = 0; i < total; i++) {
        double t[STAGES + 1];
        p.frame_index = 0;          /* the daemon never sets it */
        t[0] = nr_now();
        nr_decode8(payloads[i], pixels, 1, colour);
        t[1] = nr_now();
        const float *net_in = colour;
        if (ih != height || iw != width) { resample(&down, colour, inner); net_in = inner; }

        /* History.take: dropped on a cut, else the kept output brought to the inner extent */
        const float *h_inner = NULL, *h_full = NULL, *h_prev = NULL;
        if (temporal && have_history) {
            double cut = 0.0;
            for (size_t k = 0; k < inner_pixels * 3; k++) cut += fabs((double)net_in[k] - (double)kept_source[k]);
            cut /= (double)(inner_pixels * 3);
            if (cut > o->cut_limit) {
                have_history = 0;
            } else {
                if (ih != height || iw != width) { resample(&history_down, kept_output, history_inner); h_inner = history_inner; }
                else h_inner = kept_output;
                h_full = kept_output;
                h_prev = want_previous ? kept_pixels : NULL;
            }
        }
        t[2] = nr_now();
        if (nr_frame_head(frame, net_in, ih, iw, h_inner, NULL, &p, head_inner)) {
            fprintf(stderr, "nr_frame_head: %s\n", nr_frame_error());
            exit(1);
        }
        t[3] = nr_now();
        if (!o->separate) {
            /* the daemon's end of the frame in one pass: the head brought up, composed and
             * encoded into a copy of the request ("up" is that copy) */
            memcpy(encoded, payloads[i], pixels * 4);
            t[4] = nr_now();
            if (nr_frame_compose_encode_neural(frame, head_inner, ih, iw, colour, height, width, h_full,
                                               h_prev, NULL, &p, output, encoded, width, 0, 0, 1,
                                               temporal ? neural : NULL)) {
                fprintf(stderr, "nr_frame_compose_encode: %s\n", nr_frame_error());
                exit(1);
            }
            t[5] = t[6] = nr_now();
        } else {
            /* All four channels come up: the fourth is the gate, needed with history. */
            const float *full_head = head_inner;
            if (ih != height || iw != width) { resample(&up, head_inner, head); full_head = head; }
            t[4] = nr_now();
            if (nr_frame_compose_neural(frame, full_head, colour, height, width, h_full, h_prev, NULL, &p,
                                        output, temporal ? neural : NULL)) {
                fprintf(stderr, "nr_frame_compose: %s\n", nr_frame_error());
                exit(1);
            }
            t[5] = nr_now();
            nr_encode8(output, (ptrdiff_t)width * 3, 3, 1, payloads[i], (size_t)height, (size_t)width, 1, encoded);
            t[6] = nr_now();
        }
        if (temporal) {
            /* History.keep; the daemon keeps references, a copy here is outside the clock */
            memcpy(kept_output, neural, pixels * 3 * sizeof(float));
            memcpy(kept_source, net_in, inner_pixels * 3 * sizeof(float));
            memcpy(kept_pixels, colour, pixels * 3 * sizeof(float));
            have_history = 1;
        }
        if (i >= o->warmup) {
            int k = i - o->warmup;
            times[k] = t[6] - t[0];
            for (int s = 0; s < STAGES; s++) stages[s][k] = t[s + 1] - t[s];
            for (int s = 0; s < 3; s++) split[s][k] = nr_frame_split(frame, s);
            with_history += h_full != NULL;
        }
    }

    result r;
    double low = times[0], high = times[0];
    for (int k = 0; k < o->frames; k++) { if (times[k] < low) low = times[k]; if (times[k] > high) high = times[k]; }
    r.middle = median(times, o->frames);
    r.low = low; r.high = high; r.with_history = with_history;
    if (o->verbose) {
        int nh, nw;
        nr_frame_geometry_min(ih, iw, o->min_extent, &nh, &nw);
        printf("      %dx%d render extent (network %dx%d), %d of %d timed frames with history;",
               iw, ih, nw, nh, with_history, o->frames);
        for (int s = 0; s < STAGES; s++) printf(" %s %.1f", STAGE_NAMES[s], 1e3 * median(stages[s], o->frames));
        printf(" ms; gpu %.1f+%.1f+%.1f ms\n", 1e3 * median(split[0], o->frames),
               1e3 * median(split[1], o->frames), 1e3 * median(split[2], o->frames));
    }

    for (int s = 0; s < STAGES; s++) free(stages[s]);
    for (int s = 0; s < 3; s++) free(split[s]);
    for (int i = 0; i < total; i++) free(payloads[i]);
    free(payloads); free(times);
    free_resampler(&down); free_resampler(&up); free_resampler(&history_down);
    free(colour); free(inner); free(head_inner); free(head); free(output); free(history_inner);
    free(kept_output); free(neural); free(kept_source); free(kept_pixels); free(encoded);
    return r;
}

static void usage(void)
{
    fprintf(stderr,
        "usage: nr_frame_rates [WxH@scale ...] [--frames N] [--warmup N] [--weights W]\n"
        "                      [--pan N | --still] [--temporal F] [--hold F] [--release F]\n"
        "                      [--cut-limit F] [--min-extent N] [--separate] [-v]\n"
        "  cases default to live_rates.py's table; scale defaults to 0.55 as there\n"
        "  --frames 9 --warmup 3 --temporal 1 --hold 1 --release 24 --cut-limit 0.15, the daemon's\n"
        "  W defaults to the weights compiled into libnr_frame, else work/mlxw/dlssnr-logical.safetensors\n");
    exit(2);
}

int main(int argc, char **argv)
{
    options o = { 9, 3, 0, 0, 0, 1.0f, 1.0f, 24.0f, 0.15f, 320, 0 };
    const char *weights = NULL;
    /* room for every argument as a case, or for the default table when there are none */
    size_t room = (size_t)argc > sizeof PLAN / sizeof *PLAN ? (size_t)argc : sizeof PLAN / sizeof *PLAN;
    plan_case *plan = alloc(room * sizeof *plan);
    int count = 0;

#define NEXT() (i + 1 < argc ? argv[++i] : (usage(), (char *)0))
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "--frames")) o.frames = atoi(NEXT());
        else if (!strcmp(a, "--warmup")) o.warmup = atoi(NEXT());
        else if (!strcmp(a, "--weights")) weights = NEXT();
        else if (!strcmp(a, "--pan")) o.pan = atoi(NEXT());
        else if (!strcmp(a, "--still")) o.still = 1;
        else if (!strcmp(a, "--temporal")) o.temporal = (float)atof(NEXT());
        else if (!strcmp(a, "--hold")) o.hold = (float)atof(NEXT());
        else if (!strcmp(a, "--release")) o.release = (float)atof(NEXT());
        else if (!strcmp(a, "--cut-limit")) o.cut_limit = (float)atof(NEXT());
        else if (!strcmp(a, "--min-extent")) o.min_extent = atoi(NEXT());
        else if (!strcmp(a, "--separate")) o.separate = 1;
        else if (!strcmp(a, "-v") || !strcmp(a, "--verbose")) o.verbose = 1;
        else if (a[0] == '-') usage();
        else {
            plan_case c = { 0, 0, 0.55f };
            float scale = 0.55f;
            int n = sscanf(a, "%dx%d@%f", &c.width, &c.height, &scale);
            if (n < 2) n = sscanf(a, "%dX%d@%f", &c.width, &c.height, &scale);
            if (n < 2 || c.width <= 0 || c.height <= 0) usage();
            c.scale = scale;
            plan[count++] = c;
        }
    }
    if (o.frames < 1 || o.warmup < 0 || o.pan < 0) usage();
    if (o.min_extent < 128 || o.min_extent > 4096) die("--min-extent is 128-4096, as the daemon takes it");
    if (!(o.temporal >= 0.0f && o.temporal <= 1.0f) || !(o.hold >= 0.0f && o.hold <= 1.0f)
        || !(o.cut_limit >= 0.0f && o.cut_limit <= 1.0f)) die("--temporal, --hold and --cut-limit are 0-1");
    if (!(o.release >= 0.0f && o.release <= 255.0f)) die("--release is 0-255");
    for (int k = 0; k < count; k++)
        if (!(plan[k].scale >= 0.05f && plan[k].scale <= 1.0f)) die("a render scale is between 0.05 and 1");
    if (!count) {
        count = (int)(sizeof PLAN / sizeof *PLAN);
        memcpy(plan, PLAN, sizeof PLAN);
    }

    if (!weights && !nr_frame_embedded_weights_size()) weights = "work/mlxw/dlssnr-logical.safetensors";
    double started = nr_now();
    nr_frame *frame = nr_frame_open(weights);
    if (!frame) { fprintf(stderr, "nr_frame_open: %s\n", nr_frame_error()); return 1; }
    printf("%s on %s (%s), %u host threads, ready in %.1fs; %d frames after %d, %s\n",
           nr_frame_runtime(), nr_frame_device(frame), nr_frame_gemm_path(frame), nr_host_threads(),
           nr_now() - started, o.frames, o.warmup,
           o.still ? "one still frame" : o.pan ? "a panning shot" : "a fresh frame each time");
    fflush(stdout);

    printf("  %-14s %6s %8s %8s %18s\n", "swapchain", "scale", "ms", "fps", "min-max ms");
    double *measured = alloc((size_t)count * sizeof *measured);
    for (int k = 0; k < count; k++) {
        result r = measure(frame, &plan[k], &o);
        measured[k] = 1e3 * r.middle;
        char extent[32], band[40];
        snprintf(extent, sizeof extent, "%dx%d", plan[k].width, plan[k].height);
        snprintf(band, sizeof band, "%.0f-%.0f", 1e3 * r.low, 1e3 * r.high);
        printf("  %-14s %6.2f %8.0f %8.1f %18s\n", extent, (double)plan[k].scale, 1e3 * r.middle,
               1.0 / r.middle, band);
        fflush(stdout);
    }
    printf("\n  nr_knobs.RATES = (\n");
    for (int k = 0; k < count; k++)
        printf("      (%d, %d, %.2f, %.1f),\n", plan[k].width, plan[k].height, (double)plan[k].scale, measured[k]);
    printf("  )\n");

    nr_frame_close(frame);
    free(plan); free(measured);
    return 0;
}
