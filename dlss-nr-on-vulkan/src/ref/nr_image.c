/* CPU image passes. Keep the NumPy operation order and every FP16 rounding point.
 * Built locally for the host CPU; no fast-math or fused multiply-add is allowed.
 * Allocation, shape validation and buffer lifetime belong to nr_image.py.
 *
 * Taken from the parallel ProjectsCodex tree, which wrote it and measured it
 * (`notes/phase57`): feature assembly 146 -> 24 ms, composition 53 -> 9, the two
 * resizes 60 -> 8. Extended here for the two passes this tree has and that one does
 * not — history in the feature channels, and the temporal composition with its floor.
 *
 * Every function is a transcription of the NumPy above it, not a reimplementation.
 * The outputs are required to be byte-identical, and `test_native_image.py` checks it.
 *
 * Each pass's outer row loop is split across threads. No row reads another row's result,
 * so which thread computes a row changes nothing about its bytes; at a 1080p output it
 * changes the composition from 15.5 ms to 3.3 on an eight-core Lunar Lake. Rows are
 * handed out four at a time as threads come free (upstream's OpenMP
 * `schedule(dynamic, 4)`) rather than split evenly up front: four of Lunar Lake's eight
 * cores are low-power ones, and an even split left the others waiting on them — the
 * fused composition 2.6 -> 2.2 ms at 1280x720. The two loops over single pixels or
 * elements (`nr_decode8`, `nr_to_half`) keep the even split, where a chunk of four is
 * all overhead.
 *
 * The threads are this file's own small pool rather than OpenMP: Apple's clang has no
 * OpenMP, MSVC's is 2.0 (no unsigned loop variables), and this object is linked into
 * libdlssnr, which a host such as VBA-M links into a sandboxed application bundle — an
 * OpenMP runtime would be one more shared library to carry and sign. The pool waits
 * passively on a condition variable, which is what upstream's OMP_WAIT_POLICY=passive
 * asks for: between frames the threads have a whole graph to wait through.
 * `NR_HOST_THREADS` (else `OMP_NUM_THREADS`) sets the count, 1 runs every pass on the
 * calling thread; the default is one per online core, at most 64.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "nr_image.h"
#include "nr_portable.h"

/* -- the row pool --------------------------------------------------------- */

#if defined(_WIN32) && (!defined(_WIN32_WINNT) || _WIN32_WINNT >= 0x0600)
#define NR_ROWS_WIN32 1
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
typedef SRWLOCK nr_lock;
typedef CONDITION_VARIABLE nr_cond;
#define nr_lock_take(l) AcquireSRWLockExclusive(l)
#define nr_lock_give(l) ReleaseSRWLockExclusive(l)
#define nr_cond_wait(c, l) SleepConditionVariableSRW((c), (l), INFINITE, 0)
#define nr_cond_wake_all(c) WakeAllConditionVariable(c)
#define nr_cond_wake_one(c) WakeConditionVariable(c)
#define nr_fetch_add(p, n) ((size_t)InterlockedExchangeAdd64((volatile LONG64 *)(p), (LONG64)(n)))
#elif !defined(_WIN32) && !(defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__))
#define NR_ROWS_PTHREAD 1
#include <pthread.h>
#include <unistd.h>
typedef pthread_mutex_t nr_lock;
typedef pthread_cond_t nr_cond;
#define nr_lock_take(l) pthread_mutex_lock(l)
#define nr_lock_give(l) pthread_mutex_unlock(l)
#define nr_cond_wait(c, l) pthread_cond_wait((c), (l))
#define nr_cond_wake_all(c) pthread_cond_broadcast(c)
#define nr_cond_wake_one(c) pthread_cond_signal(c)
#define nr_fetch_add(p, n) __atomic_fetch_add((p), (n), __ATOMIC_RELAXED)
#endif

#define NR_ROWS_MAX 64

#if defined(NR_ROWS_WIN32) || defined(NR_ROWS_PTHREAD)
static struct {
    nr_lock lock;
    nr_cond wake, done;
    unsigned threads;               /* the workers and the caller */
    unsigned long generation;       /* bumped once per job */
    unsigned pending;               /* workers still on the current job */
    int busy;                       /* a job is running: a second caller goes serial */
    nr_rows_fn fn;
    const void *args;
    size_t rows;
    size_t chunk;                   /* 0: one even band a thread; else rows a grab */
#ifdef NR_ROWS_WIN32
    volatile LONG64 next;           /* the next row not yet taken */
#else
    size_t next;
#endif
} pool;

/* Band `i` of `n`: the contiguous rows OpenMP's static schedule would hand thread `i`. */
static void band(size_t rows, unsigned i, unsigned n, size_t *y0, size_t *y1)
{
    *y0 = (size_t)((unsigned long long)rows * i / n);
    *y1 = (size_t)((unsigned long long)rows * (i + 1) / n);
}

/* Thread `i` of `n`'s share of the current job: its even band, or chunks of `chunk` rows
 * taken from the shared counter until none are left. */
static void run_share(nr_rows_fn fn, const void *args, size_t rows, size_t chunk,
                      unsigned i, unsigned n)
{
    size_t y0, y1;
    if (!chunk) {
        band(rows, i, n, &y0, &y1);
        if (y0 < y1) fn(args, y0, y1);
        return;
    }
    while ((y0 = nr_fetch_add(&pool.next, chunk)) < rows)
        fn(args, y0, y0 + chunk < rows ? y0 + chunk : rows);
}

static void worker_loop(unsigned index)
{
    unsigned long seen = 0;
    for (;;) {
        nr_lock_take(&pool.lock);
        while (pool.generation == seen) nr_cond_wait(&pool.wake, &pool.lock);
        seen = pool.generation;
        nr_rows_fn fn = pool.fn;
        const void *args = pool.args;
        size_t rows = pool.rows, chunk = pool.chunk;
        unsigned n = pool.threads;
        nr_lock_give(&pool.lock);
        run_share(fn, args, rows, chunk, index, n);
        nr_lock_take(&pool.lock);
        if (--pool.pending == 0) nr_cond_wake_one(&pool.done);
        nr_lock_give(&pool.lock);
    }
}

static unsigned wanted_threads(void)
{
    const char *names[] = { "NR_HOST_THREADS", "OMP_NUM_THREADS" };
    for (int i = 0; i < 2; i++) {
        const char *v = getenv(names[i]);
        int n = v ? atoi(v) : 0;
        if (n > 0) return n > NR_ROWS_MAX ? NR_ROWS_MAX : (unsigned)n;
    }
#ifdef NR_ROWS_WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    long cores = (long)info.dwNumberOfProcessors;
#else
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    if (cores < 1) cores = 1;
    return cores > NR_ROWS_MAX ? NR_ROWS_MAX : (unsigned)cores;
}

#ifdef NR_ROWS_WIN32
/* worker_loop never returns; once MSVC inlines it the return is C4702 under /WX. */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
static DWORD WINAPI worker_main(LPVOID arg) { worker_loop((unsigned)(uintptr_t)arg); return 0; }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
static INIT_ONCE pool_once = INIT_ONCE_STATIC_INIT;
static BOOL CALLBACK pool_start(PINIT_ONCE once, PVOID param, PVOID *context)
{
    (void)once; (void)param; (void)context;
    InitializeSRWLock(&pool.lock);
    InitializeConditionVariable(&pool.wake);
    InitializeConditionVariable(&pool.done);
    unsigned want = wanted_threads(), have = 1;
    for (; have < want; have++) {
        HANDLE t = CreateThread(NULL, 0, worker_main, (LPVOID)(uintptr_t)have, 0, NULL);
        if (!t) break;
        CloseHandle(t);
    }
    pool.threads = have;
    return TRUE;
}
static void pool_init(void) { InitOnceExecuteOnce(&pool_once, pool_start, NULL, NULL); }
#else
static void *worker_main(void *arg) { worker_loop((unsigned)(uintptr_t)arg); return NULL; }
static pthread_once_t pool_once = PTHREAD_ONCE_INIT;
static void pool_start(void)
{
    pthread_mutex_init(&pool.lock, NULL);
    pthread_cond_init(&pool.wake, NULL);
    pthread_cond_init(&pool.done, NULL);
    unsigned want = wanted_threads(), have = 1;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for (; have < want; have++) {
        pthread_t t;
        if (pthread_create(&t, &attr, worker_main, (void *)(uintptr_t)have) != 0) break;
    }
    pthread_attr_destroy(&attr);
    pool.threads = have;
}
static void pool_init(void) { pthread_once(&pool_once, pool_start); }
#endif

unsigned nr_host_threads(void)
{
    pool_init();
    return pool.threads;
}

static void parallel(size_t rows, size_t chunk, nr_rows_fn fn, const void *args)
{
    if (rows == 0) return;
    pool_init();
    unsigned n = pool.threads;
    /* A few rows are not worth a wake-up; and a job already running (a second host
     * thread, or a pass called from inside a band) takes the calling thread alone. */
    if (n <= 1 || rows < 2u * n) { fn(args, 0, rows); return; }
    nr_lock_take(&pool.lock);
    if (pool.busy) { nr_lock_give(&pool.lock); fn(args, 0, rows); return; }
    pool.busy = 1;
    pool.fn = fn; pool.args = args; pool.rows = rows;
    pool.chunk = chunk; pool.next = 0;
    pool.pending = n - 1;
    pool.generation++;
    nr_cond_wake_all(&pool.wake);
    nr_lock_give(&pool.lock);
    run_share(fn, args, rows, chunk, 0, n);
    nr_lock_take(&pool.lock);
    while (pool.pending) nr_cond_wait(&pool.done, &pool.lock);
    pool.busy = 0;
    nr_lock_give(&pool.lock);
}

void nr_parallel_rows(size_t rows, nr_rows_fn fn, const void *args)
{
    parallel(rows, 4, fn, args);
}

static void parallel_even(size_t rows, nr_rows_fn fn, const void *args)
{
    parallel(rows, 0, fn, args);
}
#else
unsigned nr_host_threads(void) { return 1; }
void nr_parallel_rows(size_t rows, nr_rows_fn fn, const void *args) { if (rows) fn(args, 0, rows); }
static void parallel_even(size_t rows, nr_rows_fn fn, const void *args) { if (rows) fn(args, 0, rows); }
#endif

/* -- the passes ----------------------------------------------------------- */

static float half(float value) { return nr_half_round(value); }

static float unit(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

#if defined(_MSC_VER) && !defined(__clang__)
#define NR_RESTRICT __restrict
#define NR_ALWAYS_INLINE static __forceinline
#define NR_IVDEP __pragma(loop(ivdep))
#else
#define NR_RESTRICT __restrict__
#define NR_ALWAYS_INLINE static inline __attribute__((always_inline))
#if defined(__clang__)
#define NR_IVDEP     /* clang vectorises what it can without being asked, and warns if asked */
#else
#define NR_IVDEP _Pragma("GCC ivdep")
#endif
#endif

/* Toward zero onto the half grid, which is what the vendor's RGBA16F history keeps of the
 * prediction (`nr_frame.truncate_half`): a normal half keeps ten mantissa bits, a subnormal
 * one is a multiple of 2^-24. Selects rather than branches, so the fused pass vectorises. */
static inline float truncate_half(float x)
{
    uint32_t f, t;
    memcpy(&f, &x, sizeof f);
    t = f & 0xffffe000u;
    float kept, small = floorf(fabsf(x) * 16777216.0f) * 5.9604644775390625e-8f;
    memcpy(&kept, &t, sizeof kept);
    small = x < 0.0f ? -small : small;
    return (f & 0x7fffffffu) >= 0x38800000u ? kept : small;
}

static inline float grade_unit(float value)
{
    value = value < 0.0f ? 0.0f : value;
    return value > 1.0f ? 1.0f : value;
}

NR_ALWAYS_INLINE float hue_channel(float p, float q, float t)
{
    float up = t + 1.0f;
    t = t < 0.0f ? up : t;
    float down = t - 1.0f;
    t = t > 1.0f ? down : t;
    float rising = p + t * ((q - p) * 6.0f);
    float falling = p + (2.0f / 3.0f - t) * (q - p) * 6.0f;
    return t < 1.0f / 6.0f ? rising : t < 0.5f ? q : t < 2.0f / 3.0f ? falling : p;
}

/* The vendor's colour grade for a style on one pixel, `nr_frame.style_grade` operation for
 * operation: per channel an exposure and a smoothstep contrast, then a saturation
 * multiplier in HSL. `grade` is `nr_frame.grade_for`'s three factors. The stages of
 * `cg2r_post_process_kernel` no style moves are identities and are not here
 * (`notes/phase70-post-process.md`). Every branch is a select, as NumPy's `where` computes
 * both sides — the unused one on a stand-in divisor — so the fused rows vectorise it. */
NR_ALWAYS_INLINE void style_grade(float c[3], const float *grade)
{
    const float exposure = grade[0], contrast = grade[1], saturation = grade[2];
    for (int i = 0; i < 3; ++i) {
        float v = grade_unit(grade_unit(c[i]) * exposure);
        c[i] = grade_unit(v + contrast * (v * v * (3.0f - (v + v)) - v));
    }
    float r = c[0], g = c[1], b = c[2];
    float high = r > g ? r : g, low = r < g ? r : g;
    high = high > b ? high : b;
    low = low < b ? low : b;
    float light = (high + low) * 0.5f;
    float delta = high - low;
    int grey = !(high > low);
    float spread = grey ? 1.0f : delta;
    float bright = (2.0f - high) - low, dark = high + low;
    float by_bright = delta / (grey ? 1.0f : bright), by_dark = delta / (grey ? 1.0f : dark);
    float chroma = light > 0.5f ? by_bright : by_dark;
    float from_r = ((g - b) / spread + (g < b ? 6.0f : 0.0f)) / 6.0f;
    float from_g = ((b - r) / spread + 2.0f) / 6.0f;
    float from_b = ((r - g) / spread + 4.0f) / 6.0f;
    float hue = grey ? 0.0f : high == r ? from_r : high == g ? from_g : from_b;
    chroma = grade_unit((grey ? 0.0f : chroma) * saturation);
    float q_dark = light * (chroma + 1.0f), q_bright = (light + chroma) - light * chroma;
    float q = light < 0.5f ? q_dark : q_bright;
    float p = (light + light) - q;
    int flat = chroma <= 0.0f;
    float lit = grade_unit(light);
    c[0] = flat ? lit : grade_unit(hue_channel(p, q, hue + 1.0f / 3.0f));
    c[1] = flat ? lit : grade_unit(hue_channel(p, q, hue));
    c[2] = flat ? lit : grade_unit(hue_channel(p, q, hue - 1.0f / 3.0f));
}

/* The prediction after the history's blend, as the vendor goes on with it
 * (`nr_frame._vendor_prediction`): kept in `neural` truncated to half, the next frame's
 * history, and that half through the style's grade; then the blend back towards the frame.
 * With neither `grade` nor `neural` it is the blend alone, as it always was. */
static inline void finish_pixel(float predicted[3], const float *rgb, ptrdiff_t sc,
                                float blend, const float *grade, float *neural, float *out)
{
    if (grade || neural) {
        float kept[3];
        for (int c = 0; c < 3; ++c) kept[c] = truncate_half(predicted[c]);
        if (neural)
            for (int c = 0; c < 3; ++c) neural[c] = kept[c];
        if (grade) {
            style_grade(kept, grade);
            for (int c = 0; c < 3; ++c) predicted[c] = kept[c];
        }
    }
    for (int c = 0; c < 3; ++c) {
        float source = rgb[(ptrdiff_t)c * sc];
        out[c] = unit(source + blend * (predicted[c] - source));
    }
}

void nr_finish_pixel(float predicted[3], const float *rgb, float blend, const float *grade,
                     float *neural, float *out)
{
    finish_pixel(predicted, rgb, 1, blend, grade, neural, out);
}

/* A grading row's grade, over the row in a loop of its own: inside the composition's pixel
 * it would not vectorise, and took the whole row's composition down to one pixel at a time
 * with it (upstream: 47 -> 66 ms a 1080p frame). GCC's jump threading turns the grade's
 * chain of clamps into branches its vectoriser cannot take back, so it is off for this loop
 * alone; it moves no arithmetic, and the bytes are checked (`test_native_image.py`). */
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((optimize("no-thread-jumps")))
#endif
static void grade_row(const float *NR_RESTRICT kept, float *NR_RESTRICT graded, int width,
                      const float *NR_RESTRICT grade)
{
    for (int x = 0; x < width; ++x) {
        float c[3] = { kept[3 * x], kept[3 * x + 1], kept[3 * x + 2] };
        style_grade(c, grade);
        for (int i = 0; i < 3; ++i) graded[3 * x + i] = c[i];
    }
}

struct decode8_args { const uint8_t *source; int bgra; float *output; };

static void decode8_rows(const void *args, size_t p0, size_t p1)
{
    const struct decode8_args *a = args;
    const uint8_t *source = a->source;
    int bgra = a->bgra;
    float *output = a->output;
    for (size_t p = p0; p < p1; ++p) {
        output[p * 3] = (float)source[p * 4 + (bgra ? 2 : 0)] / 255.0f;
        output[p * 3 + 1] = (float)source[p * 4 + 1] / 255.0f;
        output[p * 3 + 2] = (float)source[p * 4 + (bgra ? 0 : 2)] / 255.0f;
    }
}

void nr_decode8(const uint8_t *source, size_t pixels, int bgra, float *output)
{
    struct decode8_args a = { source, bgra, output };
    parallel_even(pixels, decode8_rows, &a);
}

struct encode8_args {
    const float *image; ptrdiff_t sy, sx, sc; const uint8_t *raw; size_t width; int bgra;
    uint8_t *output;
};

static void encode8_rows(const void *args, size_t y0, size_t y1)
{
    const struct encode8_args *a = args;
    const float *image = a->image;
    ptrdiff_t sy = a->sy, sx = a->sx, sc = a->sc;
    const uint8_t *raw = a->raw;
    size_t width = a->width;
    int bgra = a->bgra;
    uint8_t *output = a->output;
    for (size_t y = y0; y < y1; ++y) {
        for (size_t x = 0; x < width; ++x) {
            const float *rgb = image + (ptrdiff_t)y * sy + (ptrdiff_t)x * sx;
            size_t p = y * width + x;
            for (size_t c = 0; c < 3; ++c) {
                float value = rgb[(ptrdiff_t)c * sc];
                /* NumPy's byte cast maps NaN to zero; do not cast NaN in C. */
                value = value == value ? unit(value) : 0.0f;
                output[p * 4 + (bgra ? 2 - c : c)] = (uint8_t)(value * 255.0f + 0.5f);
            }
            output[p * 4 + 3] = raw[p * 4 + 3];
        }
    }
}

void nr_encode8(const float *image, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                 const uint8_t *raw, size_t height, size_t width, int bgra,
                 uint8_t *output)
{
    struct encode8_args a = { image, sy, sx, sc, raw, width, bgra, output };
    nr_parallel_rows(height, encode8_rows, &a);
}

/* The row forms of nr_compose and nr_compose_temporal for the layout nr_frame.c hands
 * them — the head four floats a pixel, the images RGB triples — defined with the fused
 * composition below, whose helpers they share. Each returns 0, having done nothing, when
 * its scratch cannot be had, and the caller takes the general loop. */
static int still_rows_fast(const float *head, ptrdiff_t hy, const float *colour, ptrdiff_t sy,
                           size_t y0, size_t y1, size_t width, float still,
                           const float *grade, float *neural, float *output);
struct temporal_args;
static int temporal_rows_fast(const struct temporal_args *a, size_t y0, size_t y1);

struct compose_args {
    const float *head; ptrdiff_t hy, hx, hc; const float *colour; ptrdiff_t sy, sx, sc;
    size_t width; float blend; const float *grade; float *neural; float *output;
};

static void compose_rows(const void *args, size_t y0, size_t y1)
{
    const struct compose_args *a = args;
    if (a->hx == 4 && a->hc == 1 && a->sx == 3 && a->sc == 1 && a->width < (1u << 24)
        && still_rows_fast(a->head, a->hy, a->colour, a->sy, y0, y1, a->width, a->blend,
                           a->grade, a->neural, a->output))
        return;
    const float *head = a->head, *colour = a->colour;
    ptrdiff_t hy = a->hy, hx = a->hx, hc = a->hc, sy = a->sy, sx = a->sx, sc = a->sc;
    size_t width = a->width;
    float blend = a->blend;
    float *output = a->output;
    for (size_t y = y0; y < y1; ++y) {
        for (size_t x = 0; x < width; ++x) {
            const float *h = head + (ptrdiff_t)y * hy + (ptrdiff_t)x * hx;
            const float *rgb = colour + (ptrdiff_t)y * sy + (ptrdiff_t)x * sx;
            float predicted[3];
            for (size_t c = 0; c < 3; ++c)
                predicted[c] = unit(rgb[(ptrdiff_t)c * sc] + half(h[(ptrdiff_t)c * hc]) * 0.25f);
            finish_pixel(predicted, rgb, sc, blend, a->grade,
                         a->neural ? a->neural + (y * width + x) * 3 : NULL,
                         output + (y * width + x) * 3);
        }
    }
}

/* `grade` is `nr_frame.grade_for`'s three factors or NULL, and `neural`, RGB triples of the
 * output's extent or NULL, receives the prediction truncated to half — the vendor's
 * history for the next frame (`nr_frame._vendor_prediction`). */
void nr_compose(const float *head, ptrdiff_t hy, ptrdiff_t hx, ptrdiff_t hc,
                const float *colour, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                size_t height, size_t width, float intensity,
                const float *grade, float *neural, float *output)
{
    /* Our intensity > 1 extrapolates; the vendor recipe clamps negative intensity.
     * Keep both subtract/add operations even at intensity 1: simplifying to the
     * prediction would remove an FP32 rounding and can change encoded pixels.
     */
    float blend = intensity > 1.0f ? intensity : unit(intensity);
    struct compose_args a = { head, hy, hx, hc, colour, sy, sx, sc, width, blend,
                              grade, neural, output };
    nr_parallel_rows(height, compose_rows, &a);
}

/* `history` is this tree's addition: the previous output, at the same logical extent as
 * the colour and mirrored onto the network extent the same way, standing in channels 7-9
 * where the first-frame layout repeats the colour. Identity reprojection, because a
 * layer at vkQueuePresentKHR has no motion vectors (notes/phase54). NULL for a still
 * frame, which is then bit-identical to the vendor's own first-frame layout.
 */
struct features_args {
    const float *colour; ptrdiff_t sy, sx, sc; const float *history; ptrdiff_t ty, tx, tc;
    const int32_t *rows, *columns; size_t width; const float *noise, *controls;
    float *output; uint16_t *output_half;
};

/* The sixteen channels of one pixel, as float. Both stores below are this. */
static inline void feature_pixel(const float *rgb, ptrdiff_t sc, const float *was,
                                 ptrdiff_t tc, const float *noise, const float *controls,
                                 float out[16])
{
    for (size_t c = 0; c < 3; ++c) {
        float scaled = half(half(half(rgb[(ptrdiff_t)c * sc]) - 0.5f) * 0.125f);
        out[c] = noise[c];
        out[4 + c] = scaled;
        out[7 + c] = was
            ? half(half(half(was[(ptrdiff_t)c * tc]) - 0.5f) * 0.125f)
            : scaled;
    }
    out[3] = 1.0f;
    for (size_t c = 0; c < 5; ++c) out[10 + c] = controls[c];
    out[15] = 0.0f;
}

static void features_rows(const void *args, size_t y0, size_t y1)
{
    const struct features_args *a = args;
    const float *colour = a->colour, *history = a->history, *noise = a->noise;
    const float *controls = a->controls;
    ptrdiff_t sy = a->sy, sx = a->sx, sc = a->sc, ty = a->ty, tx = a->tx, tc = a->tc;
    const int32_t *rows = a->rows, *columns = a->columns;
    size_t width = a->width;
    for (size_t y = y0; y < y1; ++y) {
        const float *row = colour + rows[y] * sy;
        const float *old = history ? history + rows[y] * ty : 0;
        for (size_t x = 0; x < width; ++x) {
            size_t pixel = y * width + x;
            const float *rgb = row + columns[x] * sx;
            const float *was = old ? old + columns[x] * tx : 0;
            if (a->output) {
                feature_pixel(rgb, sc, was, tc, noise + pixel * 3, controls,
                              a->output + pixel * 16);
            } else {
                float values[16];
                feature_pixel(rgb, sc, was, tc, noise + pixel * 3, controls, values);
                uint16_t *out = a->output_half + pixel * 16;
                for (size_t c = 0; c < 16; ++c) out[c] = nr_float_to_half(values[c]);
            }
        }
    }
}

void nr_features(const float *colour, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                 const float *history, ptrdiff_t ty, ptrdiff_t tx, ptrdiff_t tc,
                 const int32_t *rows, const int32_t *columns,
                 size_t height, size_t width, const float *noise,
                 const float *controls, float *output)
{
    struct features_args a = { colour, sy, sx, sc, history, ty, tx, tc, rows, columns,
                               width, noise, controls, output, NULL };
    nr_parallel_rows(height, features_rows, &a);
}

/* The same features stored as half, which is what the graph's first GEMM reads: the
 * to_half pass the GPU ran over them goes, and so do half the bytes written here. Every
 * rounding is to nearest even, as that pass's, so the half values are its values
 * (test_native_image.py, test_input_fp16.py). The output is half bit patterns, since
 * `_Float16` is not in every compiler this is built with (nr_portable.h). */
void nr_features_half(const float *colour, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                      const float *history, ptrdiff_t ty, ptrdiff_t tx, ptrdiff_t tc,
                      const int32_t *rows, const int32_t *columns,
                      size_t height, size_t width, const float *noise,
                      const float *controls, uint16_t *output)
{
    struct features_args a = { colour, sy, sx, sc, history, ty, tx, tc, rows, columns,
                               width, noise, controls, NULL, output };
    nr_parallel_rows(height, features_rows, &a);
}

/* float32 to half, rounding to nearest even — for features a caller built as float.
 * "Rows" here are 4096-element runs, so the pool splits a long buffer into bands. */
struct to_half_args { const float *source; size_t count; uint16_t *target; };
enum { TO_HALF_RUN = 4096 };

static void to_half_rows(const void *args, size_t r0, size_t r1)
{
    const struct to_half_args *a = args;
    size_t end = r1 * TO_HALF_RUN < a->count ? r1 * TO_HALF_RUN : a->count;
    for (size_t i = r0 * TO_HALF_RUN; i < end; ++i)
        a->target[i] = nr_float_to_half(a->source[i]);
}

void nr_to_half(const float *source, size_t count, uint16_t *target)
{
    struct to_half_args a = { source, count, target };
    parallel_even((count + TO_HALF_RUN - 1) / TO_HALF_RUN, to_half_rows, &a);
}

/* The area mean of a downscale by whole factors — `nr_daemon.resample`'s other branch,
 * which averages instead of sampling so the network is not handed aliasing to enhance.
 * The order is that NumPy code's: a block's first sample, each other one added in
 * row-major order, then one division by the count. */
struct area_args {
    const float *source; ptrdiff_t sy, sx, sc; size_t width, channels, fy, fx;
    float *output;
};

static void area_rows(const void *args, size_t y0, size_t y1)
{
    const struct area_args *a = args;
    ptrdiff_t sy = a->sy, sx = a->sx, sc = a->sc;
    size_t width = a->width, channels = a->channels, fy = a->fy, fx = a->fx;
    float count = (float)(fy * fx);
    for (size_t y = y0; y < y1; ++y) {
        for (size_t x = 0; x < width; ++x) {
            const float *block = a->source + (ptrdiff_t)(y * fy) * sy
                               + (ptrdiff_t)(x * fx) * sx;
            float *out = a->output + (y * width + x) * channels;
            for (size_t c = 0; c < channels; ++c) {
                const float *first = block + (ptrdiff_t)c * sc;
                float total = first[0];
                for (size_t dy = 0; dy < fy; ++dy)
                    for (size_t dx = 0; dx < fx; ++dx)
                        if (dy || dx)
                            total += first[(ptrdiff_t)dy * sy + (ptrdiff_t)dx * sx];
                out[c] = total / count;
            }
        }
    }
}

void nr_area_mean(const float *source, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                  size_t height, size_t width, size_t channels, size_t fy, size_t fx,
                  float *output)
{
    struct area_args a = { source, sy, sx, sc, width, channels, fy, fx, output };
    nr_parallel_rows(height, area_rows, &a);
}

/* One axis at a time: the intermediate is deliberately rounded to FP32 before
 * the second axis. Coordinates/weights come from the unchanged NumPy formula.
 * Signed strides permit padded crops and reversed views without another copy.
 */
struct resize_args {
    const float *source; ptrdiff_t sy, sx, sc; size_t width, channels; int axis;
    const int32_t *low, *high; const float *weight; float *output;
};

static void resize_rows(const void *args, size_t y0, size_t y1)
{
    const struct resize_args *r = args;
    const float *source = r->source, *weight = r->weight;
    ptrdiff_t sy = r->sy, sx = r->sx, sc = r->sc;
    size_t width = r->width, channels = r->channels;
    int axis = r->axis;
    const int32_t *low = r->low, *high = r->high;
    float *output = r->output;
    for (size_t y = y0; y < y1; ++y) {
        if (axis == 0 && sx == (ptrdiff_t)channels && sc == 1) {
            const float *a = source + low[y] * sy;
            const float *b = source + high[y] * sy;
            float w = weight[y], other = 1.0f - w;
            for (size_t i = 0; i < width * channels; ++i)
                output[y * width * channels + i] = a[i] * other + b[i] * w;
        } else {
            for (size_t x = 0; x < width; ++x) {
                size_t index = axis == 0 ? y : x;
                const float *a = axis == 0 ? source + low[y] * sy + (ptrdiff_t)x * sx
                                          : source + (ptrdiff_t)y * sy + low[x] * sx;
                const float *b = axis == 0 ? source + high[y] * sy + (ptrdiff_t)x * sx
                                          : source + (ptrdiff_t)y * sy + high[x] * sx;
                float w = weight[index], other = 1.0f - w;
                for (size_t c = 0; c < channels; ++c)
                    output[(y * width + x) * channels + c] =
                        a[(ptrdiff_t)c * sc] * other + b[(ptrdiff_t)c * sc] * w;
            }
        }
    }
}

void nr_resize_axis(const float *source, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                    size_t height, size_t width, size_t channels, int axis,
                    const int32_t *low, const int32_t *high,
                    const float *weight, float *output)
{
    struct resize_args r = { source, sy, sx, sc, width, channels, axis, low, high, weight,
                             output };
    nr_parallel_rows(height, resize_rows, &r);
}


/* One pixel of the temporal composition, as nr_compose_temporal describes it below: `h`
 * the head's four channels, `gate_at` this pixel's gate where there is no table, `before`
 * the game's previous pixel or NULL, `mask_at` the control mask's red here or NULL. Shared
 * by nr_compose_temporal and nr_compose_encode, so the two are one arithmetic. */
static inline void temporal_pixel(const float *h, ptrdiff_t hc,
                                  const float *rgb, ptrdiff_t sc,
                                  const float *was, ptrdiff_t rc,
                                  const float *before, ptrdiff_t pc,
                                  const float *gate_at, const float *table, float confidence,
                                  const float *mask_at, float intensity,
                                  float scale, float hold, float slope, float release,
                                  const float *grade, float *neural, float *out)
{
    /* What the game itself did to this pixel since its previous frame: the
     * largest step of the three channels. Both the floor and the release read it. */
    float moved = 0.0f;
    if (before) {
        for (size_t c = 0; c < 3; ++c) {
            float step = rgb[(ptrdiff_t)c * sc] - before[(ptrdiff_t)c * pc];
            if (step < 0.0f) step = -step;
            if (step > moved) moved = step;
        }
    }
    float alpha;
    if (table) {
        /* The model's gate looked up rather than recomputed: `nr_frame.gate_table`
         * holds NumPy's own expression evaluated on every half value, so the exp
         * that kept the gate in NumPy is inside the table, and the rounding to half
         * here is the one `half` does. Then the confidence, as NumPy applies it. */
        alpha = table[nr_float_to_half(h[3 * hc])];
        if (confidence != 1.0f) alpha *= confidence;
    } else {
        alpha = *gate_at;
    }
    if (before && release != 0.0f) {
        /* The release: where the game's pixel changed, what the previous output
         * holds there is something that has since moved, and the gate is not local
         * enough to know it — it reads 0.6 over a whole Tekken frame. Its share
         * fades from all of it at no change to none by `-1 / release`, folded like
         * `slope` and for the same reason. Before the floor, which it never lowers. */
        float kept = moved * release + 1.0f;
        if (kept < 0.0f) kept = 0.0f;
        if (kept > 1.0f) kept = 1.0f;
        alpha *= kept;
    }
    if (before) {
        /* `moved * slope + hold`, clamped to [0, hold], and `slope` arrives
         * already folded: NumPy multiplies by one constant and adds another,
         * and `clip(1 - moved * 255 / ramp, 0, 1) * hold` is the same value by
         * algebra and a different one in float32. */
        float floored = moved * slope + hold;
        if (floored < 0.0f) floored = 0.0f;
        if (floored > hold) floored = hold;
        if (floored > 1.0f) floored = 1.0f;     /* `compose` clips the floor to [0, 1] */
        floored *= scale;
        if (floored > alpha) alpha = floored;
    }
    /* No clamp on the blend: the NumPy this transcribes does not clamp it
     * either, and the vendor's clamp is the one thing our composition
     * deliberately drops so that intensity above 1 can extrapolate. */
    float blend = intensity;
    if (mask_at) blend *= *mask_at;
    float predicted[3];
    for (size_t c = 0; c < 3; ++c) {
        predicted[c] = unit(rgb[(ptrdiff_t)c * sc] + half(h[(ptrdiff_t)c * hc]) * 0.25f);
        predicted[c] += alpha * (was[(ptrdiff_t)c * rc] - predicted[c]);
    }
    finish_pixel(predicted, rgb, sc, blend, grade, neural, out);
}

/* The temporal composition, which this tree has and the other does not.
 *
 * The model's own history weight comes from `table`: NumPy's sigmoid evaluated once on
 * every half value, indexed here by the half logit's sixteen bits. `expf` and NumPy's
 * float32 exponential do not agree in the last bit, and the contract for all of this is
 * byte-identical output, not nearly — but the logit is rounded to half before the
 * sigmoid, so there are only 65536 inputs and NumPy can own every one of them.
 * `confidence` then scales it, as NumPy does. With no table, `gate` carries the weight
 * already computed, confidence and all.
 *
 * `previous` is the game's own frame from the present before, or NULL. Where it is
 * unchanged the history is right for that pixel by construction, so the gate gets a
 * floor: full at no change, gone by four levels of 255, never above `scale`
 * (notes/phase54). `mask` is the interface control mask's red channel, or NULL.
 */
struct temporal_args {
    const float *head; ptrdiff_t hy, hx, hc;
    const float *colour; ptrdiff_t sy, sx, sc;
    const float *history; ptrdiff_t ry, rx, rc;
    const float *previous; ptrdiff_t py, px, pc;
    const float *gate; ptrdiff_t gy, gx;
    const float *table; float confidence;
    const float *mask; ptrdiff_t my, mx;
    size_t width; float intensity, scale, hold, slope, release;
    const float *grade; float *neural; float *output;
};

static void temporal_rows(const void *args, size_t y0, size_t y1)
{
    const struct temporal_args *a = args;
    if (temporal_rows_fast(a, y0, y1)) return;
    for (size_t y = y0; y < y1; ++y) {
        for (size_t x = 0; x < a->width; ++x) {
            temporal_pixel(a->head + (ptrdiff_t)y * a->hy + (ptrdiff_t)x * a->hx, a->hc,
                           a->colour + (ptrdiff_t)y * a->sy + (ptrdiff_t)x * a->sx, a->sc,
                           a->history + (ptrdiff_t)y * a->ry + (ptrdiff_t)x * a->rx, a->rc,
                           a->previous ? a->previous + (ptrdiff_t)y * a->py
                                                     + (ptrdiff_t)x * a->px : NULL, a->pc,
                           a->table ? NULL : a->gate + (ptrdiff_t)y * a->gy
                                                     + (ptrdiff_t)x * a->gx,
                           a->table, a->confidence,
                           a->mask ? a->mask + (ptrdiff_t)y * a->my
                                             + (ptrdiff_t)x * a->mx : NULL,
                           a->intensity, a->scale, a->hold, a->slope, a->release, a->grade,
                           a->neural ? a->neural + (y * a->width + x) * 3 : NULL,
                           a->output + (y * a->width + x) * 3);
        }
    }
}

void nr_compose_temporal(const float *head, ptrdiff_t hy, ptrdiff_t hx, ptrdiff_t hc,
                         const float *colour, ptrdiff_t sy, ptrdiff_t sx, ptrdiff_t sc,
                         const float *history, ptrdiff_t ry, ptrdiff_t rx, ptrdiff_t rc,
                         const float *previous, ptrdiff_t py, ptrdiff_t px, ptrdiff_t pc,
                         const float *gate, ptrdiff_t gy, ptrdiff_t gx,
                         const float *table, float confidence,
                         const float *mask, ptrdiff_t my, ptrdiff_t mx,
                         size_t height, size_t width, float intensity,
                         float scale, float hold, float slope, float release,
                         const float *grade, float *neural, float *output)
{
    struct temporal_args a = { head, hy, hx, hc, colour, sy, sx, sc, history, ry, rx, rc,
                               previous, py, px, pc, gate, gy, gx, table, confidence,
                               mask, my, mx, width, intensity, scale, hold, slope, release,
                               grade, neural, output };
    nr_parallel_rows(height, temporal_rows, &a);
}

/* -- the fused composition ------------------------------------------------ */

/* A float rounded to half and back, and the half's sixteen bits, with no `_Float16`: a CPU
 * without vector half arithmetic cannot vectorise a loop around the conversion. Normal
 * halves keep ten mantissa bits, ties to even; subnormal ones are multiples of 2^-24, which
 * the float adder's own rounding finds at 0.5; past 65520 is infinity, and a NaN keeps its
 * sign and its top payload bits, quieted — the hardware's conversion, which both match for
 * every one of the 2^32 floats (upstream checked it against `_Float16` on x86-64). */
static inline float half_value(float x)
{
    uint32_t f, sb;
    memcpy(&f, &x, sizeof f);
    uint32_t sign = f & 0x80000000u, a = f & 0x7fffffffu;
    uint32_t normal = (a + 0x0fffu + ((a >> 13) & 1u)) & 0xffffe000u;
    float magnitude, tiny;
    memcpy(&magnitude, &a, sizeof a);
    tiny = (magnitude + 0.5f) - 0.5f;
    memcpy(&sb, &tiny, sizeof sb);
    uint32_t bits = a > 0x7f800000u ? 0x7fc00000u | (a & 0x003fe000u)
                  : a >= 0x477ff000u ? 0x7f800000u
                  : a >= 0x38800000u ? normal : sb;
    bits |= sign;
    float y;
    memcpy(&y, &bits, sizeof y);
    return y;
}

static inline uint32_t half_bits(float x)
{
    uint32_t f;
    memcpy(&f, &x, sizeof f);
    uint32_t sign = (f >> 16) & 0x8000u, a = f & 0x7fffffffu;
    uint32_t normal = (a + 0x0fffu + ((a >> 13) & 1u)) & 0xffffe000u;
    float magnitude;
    memcpy(&magnitude, &a, sizeof a);
    float tiny = (magnitude + 0.5f) - 0.5f;
    uint32_t bits = a > 0x7f800000u ? 0x7e00u | ((a >> 13) & 0x1ffu)
                  : a >= 0x477ff000u ? 0x7c00u
                  : a >= 0x38800000u ? (normal >> 13) - 0x1c000u
                  : (uint32_t)(tiny * 16777216.0f);
    return bits | sign;
}

static inline float clamp01(float value)
{
    value = value < 0.0f ? 0.0f : value;
    return value > 1.0f ? 1.0f : value;
}

/* The encoder's rounding of one composed pixel, all four bytes: the request's alpha written
 * back as it was, so the store is one whole interleaved group rather than three lanes of
 * four. */
NR_ALWAYS_INLINE void encode_pixel(const float o[3], uint8_t *NR_RESTRICT pixel, int x,
                                   const int bgra)
{
    uint8_t bytes[3];
    for (int c = 0; c < 3; ++c) {
        /* NumPy's byte cast maps NaN to zero; do not cast NaN in C. */
        float value = o[c] == o[c] ? clamp01(o[c]) : 0.0f;
        bytes[c] = (uint8_t)(value * 255.0f + 0.5f);
    }
    uint8_t alpha = pixel[4 * x + 3];
    pixel[4 * x] = bytes[bgra ? 2 : 0];
    pixel[4 * x + 1] = bytes[1];
    pixel[4 * x + 2] = bytes[bgra ? 0 : 2];
    pixel[4 * x + 3] = alpha;
}

/* `nr_compose_encode`'s pixel after the head's upscale, in two passes over a row so that
 * each is a loop the compiler can vectorise: first the gate — the table lookup, the one
 * gather, alone in its loop (no vector gather on NEON, and a gather in the loop kept clang
 * from vectorising any of it) — then the release and the floor, then the composition and
 * the encoder, written as selects rather than branches. Each pixel's arithmetic is
 * `temporal_pixel`'s (or `nr_compose`'s without a history), operation for operation and in
 * the same order; only which loop it sits in moved. `confidence` multiplies
 * unconditionally: at 1 that is exact for every value the table holds.
 *
 * `finish_pixel` is split the same way: with `keeping` the prediction goes into `kept`
 * truncated to half, and a `grading` row stops there — `grade_row` and `grade_blend_row`
 * take it on from the kept half, each in a loop of its own. */
NR_ALWAYS_INLINE void
compose_encode_colour(const float *NR_RESTRICT hq, const float *NR_RESTRICT rgb,
                      const float *NR_RESTRICT was, const float *NR_RESTRICT alphas,
                      float intensity, float still, float *NR_RESTRICT out,
                      float *NR_RESTRICT kept, uint8_t *NR_RESTRICT pixel, int x,
                      const int temporal, const int keeping, const int grading, const int bgra)
{
    const float *p = rgb + 3 * x;
    float predicted[3], o[3];
    for (int c = 0; c < 3; ++c) {
        predicted[c] = clamp01(p[c] + half_value(hq[4 * x + c]) * 0.25f);
        if (temporal) predicted[c] += alphas[x] * (was[3 * x + c] - predicted[c]);
    }
    if (keeping)
        for (int c = 0; c < 3; ++c) kept[3 * x + c] = truncate_half(predicted[c]);
    if (grading) return;
    const float blend = temporal ? intensity : still;
    for (int c = 0; c < 3; ++c) o[c] = clamp01(p[c] + blend * (predicted[c] - p[c]));
    for (int c = 0; c < 3; ++c) out[3 * x + c] = o[c];
    encode_pixel(o, pixel, x, bgra);
}

/* The blend back towards the frame from a graded row, and the encoder when `pixel` is
 * given, as `finish_pixel` and `compose_encode_colour` end. */
NR_ALWAYS_INLINE void
grade_blend_pixel(const float *NR_RESTRICT rgb, const float *NR_RESTRICT graded, float blend,
                  float *NR_RESTRICT out, uint8_t *NR_RESTRICT pixel, int x,
                  const int encoding, const int bgra)
{
    float o[3];
    for (int c = 0; c < 3; ++c) {
        float source = rgb[3 * x + c];
        o[c] = clamp01(source + blend * (graded[3 * x + c] - source));
    }
    for (int c = 0; c < 3; ++c) out[3 * x + c] = o[c];
    if (encoding) encode_pixel(o, pixel, x, bgra);
}

static void grade_blend_row(const float *NR_RESTRICT rgb, const float *NR_RESTRICT kept,
                            float *NR_RESTRICT graded, const float *NR_RESTRICT grade,
                            float blend, int width, float *NR_RESTRICT out,
                            uint8_t *NR_RESTRICT pixel, int bgra)
{
    grade_row(kept, graded, width, grade);
    if (!pixel) {
        NR_IVDEP
        for (int x = 0; x < width; ++x) grade_blend_pixel(rgb, graded, blend, out, NULL, x, 0, 0);
    } else if (bgra) {
        NR_IVDEP
        for (int x = 0; x < width; ++x) grade_blend_pixel(rgb, graded, blend, out, pixel, x, 1, 1);
    } else {
        NR_IVDEP
        for (int x = 0; x < width; ++x) grade_blend_pixel(rgb, graded, blend, out, pixel, x, 1, 0);
    }
}

/* The release and the floor on the gates already looked up, where the game's previous
 * frame is given: `temporal_pixel`'s `moved`, then its two steps in its order. */
NR_ALWAYS_INLINE void
compose_encode_moved(const float *NR_RESTRICT rgb, const float *NR_RESTRICT before,
                     float *NR_RESTRICT alphas, float scale, float hold, float slope,
                     float release, int x, const int releasing)
{
    const float *p = rgb + 3 * x;
    float moved = 0.0f;
    for (int c = 0; c < 3; ++c) {
        float step = p[c] - before[3 * x + c];
        step = step < 0.0f ? -step : step;
        moved = step > moved ? step : moved;
    }
    float alpha = alphas[x];
    if (releasing)
        alpha *= clamp01(moved * release + 1.0f);
    float floored = moved * slope + hold;
    floored = floored < 0.0f ? 0.0f : floored;
    floored = floored > hold ? hold : floored;
    floored = floored > 1.0f ? 1.0f : floored;
    floored *= scale;
    alphas[x] = floored > alpha ? floored : alpha;
}

/* One row of `nr_compose_encode` in the layout the daemon hands it: RGB float triples for
 * the colour, the history and the game's previous frame, the head's row already scaled on
 * the first axis with its channels adjacent, the gate from the table, no control mask.
 * The head's second axis goes into a row of its own, a pixel's channels side by side; then
 * each combination of the knobs and the byte order is its own loop, strides constant, so the
 * compiler vectorises across pixels where the general loop took them one at a time. Each
 * pixel's arithmetic is the general loop's, operation for operation — `half_value` and
 * `half_bits` are the conversion itself — so the bytes are the same
 * (`test_native_image.py`). Upstream's general loop spent 22.7 ms of one core on a 1080p
 * frame, all of it arithmetic; with the gather in a loop of its own clang on an M3 goes
 * from 24.9 ms to the figure in notes, and GCC keeps the vector code it had. `rows` holds
 * five floats a pixel: the head's four, then the gate; with a grade, six more after them —
 * the kept half where `kept` is NULL, and the graded row. */
#define COMPOSE_COLOUR(temporal, keeping, grading, bgra)                                     \
    NR_IVDEP                                                                                 \
    for (int x = 0; x < width; ++x)                                                          \
        compose_encode_colour(rows, rgb, was, alphas, intensity, still, out, kept, pixel, x, \
                              temporal, keeping, grading, bgra)
#define COMPOSE_COLOURS(temporal)                                                            \
    if (grade) COMPOSE_COLOUR(temporal, 1, 1, 0);                                            \
    else if (kept && bgra) COMPOSE_COLOUR(temporal, 1, 0, 1);                                \
    else if (kept) COMPOSE_COLOUR(temporal, 1, 0, 0);                                        \
    else if (bgra) COMPOSE_COLOUR(temporal, 0, 0, 1);                                        \
    else COMPOSE_COLOUR(temporal, 0, 0, 0)
#define COMPOSE_MOVED(releasing)                                                             \
    NR_IVDEP                                                                                 \
    for (int x = 0; x < width; ++x)                                                          \
        compose_encode_moved(rgb, before, alphas, scale, hold, slope, release, x, releasing)
static void compose_encode_row(const float *NR_RESTRICT line, int lx,
                               const int32_t *NR_RESTRICT low_x,
                               const int32_t *NR_RESTRICT high_x,
                               const float *NR_RESTRICT weight_x,
                               const float *NR_RESTRICT rgb, const float *NR_RESTRICT was,
                               const float *NR_RESTRICT before,
                               const float *NR_RESTRICT table,
                               float confidence, float intensity, float still, float scale,
                               float hold, float slope, float release,
                               const float *NR_RESTRICT grade, int width,
                               float *NR_RESTRICT rows, float *NR_RESTRICT out,
                               float *NR_RESTRICT kept,
                               uint8_t *NR_RESTRICT pixel, int bgra)
{
    float *NR_RESTRICT alphas = rows + 4 * (size_t)width;
    float *NR_RESTRICT graded = rows + 8 * (size_t)width;
    if (grade && !kept) kept = rows + 5 * (size_t)width;
    /* the head's second axis, its channels side by side — one vector a tap with a history,
     * whose four channels the head row holds; three without, which is all it holds then */
    if (was)
        for (int x = 0; x < width; ++x) {
            float w = weight_x[x], other = 1.0f - w;
            const float *a = line + low_x[x] * lx, *b = line + high_x[x] * lx;
            for (int c = 0; c < 4; ++c) rows[4 * x + c] = a[c] * other + b[c] * w;
        }
    else
        for (int x = 0; x < width; ++x) {
            float w = weight_x[x], other = 1.0f - w;
            const float *a = line + low_x[x] * lx, *b = line + high_x[x] * lx;
            for (int c = 0; c < 3; ++c) rows[4 * x + c] = a[c] * other + b[c] * w;
        }
    if (!was) {
        COMPOSE_COLOURS(0);
    } else {
        for (int x = 0; x < width; ++x)
            alphas[x] = table[half_bits(rows[4 * x + 3])] * confidence;
        if (before) {
            if (release != 0.0f) COMPOSE_MOVED(1);
            else COMPOSE_MOVED(0);
        }
        COMPOSE_COLOURS(1);
    }
    if (grade)
        grade_blend_row(rgb, kept, graded, grade, was ? intensity : still, width, out, pixel,
                        bgra);
}
#undef COMPOSE_COLOURS
#undef COMPOSE_COLOUR
#undef COMPOSE_MOVED

/* nr_compose's and nr_compose_temporal's pixel in the same loops as the fused pass: the
 * composition alone, no encoder. The arithmetic is `compose_rows`' and `temporal_pixel`'s,
 * operation for operation (`half_value` is `half`, `half_bits` the table's index, `clamp01`
 * `unit`); only the loop shape moved, so the compiler can vectorise it. `keeping` and
 * `grading` are `compose_encode_colour`'s. */
NR_ALWAYS_INLINE void
compose_colour(const float *NR_RESTRICT hq, const float *NR_RESTRICT rgb,
               const float *NR_RESTRICT was, const float *NR_RESTRICT alphas,
               float blend, float *NR_RESTRICT out, float *NR_RESTRICT kept, int x,
               const int temporal, const int keeping, const int grading)
{
    float predicted[3];
    for (int c = 0; c < 3; ++c) {
        predicted[c] = clamp01(rgb[3 * x + c] + half_value(hq[4 * x + c]) * 0.25f);
        if (temporal) predicted[c] += alphas[x] * (was[3 * x + c] - predicted[c]);
    }
    if (keeping)
        for (int c = 0; c < 3; ++c) kept[3 * x + c] = truncate_half(predicted[c]);
    if (grading) return;
    for (int c = 0; c < 3; ++c) {
        float source = rgb[3 * x + c];
        out[3 * x + c] = clamp01(source + blend * (predicted[c] - source));
    }
}

#define COLOUR_ROW(temporal, keeping, grading)                                               \
    do {                                                                                     \
        NR_IVDEP                                                                             \
        for (int x = 0; x < w; ++x)                                                          \
            compose_colour(hq, rgb, was, alphas, blend, out, kept, x, temporal, keeping,     \
                           grading);                                                         \
    } while (0)
/* One composed row in the fast layout: the colour, then the grade where there is one. */
NR_ALWAYS_INLINE void
colour_row(const float *NR_RESTRICT hq, const float *NR_RESTRICT rgb,
           const float *NR_RESTRICT was, const float *NR_RESTRICT alphas, float blend,
           float *NR_RESTRICT out, float *NR_RESTRICT kept, float *NR_RESTRICT scratch,
           const float *NR_RESTRICT grade, int w, const int temporal)
{
    if (grade) {
        if (!kept) kept = scratch;
        COLOUR_ROW(temporal, 1, 1);
        grade_blend_row(rgb, kept, scratch + 3 * (size_t)w, grade, blend, w, out, NULL, 0);
    } else if (kept) {
        COLOUR_ROW(temporal, 1, 0);
    } else {
        COLOUR_ROW(temporal, 0, 0);
    }
}
#undef COLOUR_ROW

static int still_rows_fast(const float *head, ptrdiff_t hy, const float *colour, ptrdiff_t sy,
                           size_t y0, size_t y1, size_t width, float still,
                           const float *grade, float *neural, float *output)
{
    int w = (int)width;
    float *scratch = grade ? malloc(6 * width * sizeof *scratch) : NULL;
    if (grade && !scratch) return 0;
    for (size_t y = y0; y < y1; ++y) {
        const float *NR_RESTRICT hq = head + (ptrdiff_t)y * hy;
        const float *NR_RESTRICT rgb = colour + (ptrdiff_t)y * sy;
        float *NR_RESTRICT out = output + y * width * 3;
        colour_row(hq, rgb, NULL, NULL, still, out, neural ? neural + y * width * 3 : NULL,
                   scratch, grade, w, 0);
    }
    free(scratch);
    return 1;
}

static int temporal_rows_fast(const struct temporal_args *a, size_t y0, size_t y1)
{
    if (!(a->hx == 4 && a->hc == 1 && a->sx == 3 && a->sc == 1 && a->rx == 3 && a->rc == 1
          && a->table && !a->mask && (!a->previous || (a->px == 3 && a->pc == 1))
          && a->width < (1u << 24)))
        return 0;
    int width = (int)a->width, releasing = a->previous && a->release != 0.0f;
    float *NR_RESTRICT alphas = malloc((a->grade ? 7 : 1) * a->width * sizeof *alphas);
    if (!alphas) return 0;
    float *NR_RESTRICT scratch = alphas + a->width;
    const float *NR_RESTRICT table = a->table;
    float confidence = a->confidence, scale = a->scale, hold = a->hold, slope = a->slope;
    float release = a->release, intensity = a->intensity;
    for (size_t y = y0; y < y1; ++y) {
        const float *NR_RESTRICT hq = a->head + (ptrdiff_t)y * a->hy;
        const float *NR_RESTRICT rgb = a->colour + (ptrdiff_t)y * a->sy;
        const float *NR_RESTRICT was = a->history + (ptrdiff_t)y * a->ry;
        const float *NR_RESTRICT before = a->previous ? a->previous + (ptrdiff_t)y * a->py : NULL;
        float *NR_RESTRICT out = a->output + y * a->width * 3;
        /* the gate alone: the one gather (`temporal_pixel` multiplies by the confidence
         * only when it is not 1, which is exact for every table value either way) */
        for (int x = 0; x < width; ++x)
            alphas[x] = table[half_bits(hq[4 * x + 3])] * confidence;
        if (before) {
            if (releasing) {
                NR_IVDEP
                for (int x = 0; x < width; ++x)
                    compose_encode_moved(rgb, before, alphas, scale, hold, slope, release, x, 1);
            } else {
                NR_IVDEP
                for (int x = 0; x < width; ++x)
                    compose_encode_moved(rgb, before, alphas, scale, hold, slope, release, x, 0);
            }
        }
        colour_row(hq, rgb, was, alphas, intensity, out,
                   a->neural ? a->neural + y * a->width * 3 : NULL, scratch, a->grade, width, 1);
    }
    free(alphas);
    return 1;
}

struct compose_encode_args {
    const float *head; ptrdiff_t hy, hx, hc; size_t head_width, channels;
    const int32_t *low_y, *high_y; const float *weight_y;
    const int32_t *low_x, *high_x; const float *weight_x;
    const float *colour; ptrdiff_t sy, sx, sc;
    const float *history; ptrdiff_t ry, rx, rc;
    const float *previous; ptrdiff_t py, px, pc;
    const float *table; float confidence;
    const float *mask; ptrdiff_t my, mx;
    size_t width; float intensity, still, scale, hold, slope, release;
    float *output; const float *grade; float *neural; uint8_t *encoded; size_t frame_width, top, left; int bgra;
    float *samples; size_t step, sampled; int fast;
};

static void compose_encode_rows(const void *args, size_t y0, size_t y1)
{
    const struct compose_encode_args *a = args;
    size_t width = a->width, channels = a->channels, step = a->step, sampled = a->sampled;
    const int32_t *low_x = a->low_x, *high_x = a->high_x;
    const float *weight_x = a->weight_x;
    /* each band's own scratch: the head row after the first axis, and the fast path's row
     * after the second */
    float *row = a->low_y ? malloc(a->head_width * channels * sizeof *row) : NULL;
    float *rows = a->fast ? malloc((a->grade ? 11 : 5) * width * sizeof *rows) : NULL;
    if ((a->low_y && !row) || (a->fast && !rows)) { free(row); free(rows); return; }
    for (size_t y = y0; y < y1; ++y) {
        const float *line;
        ptrdiff_t lx, lc;
        if (a->low_y) {
            float w = a->weight_y[y], other = 1.0f - w;
            const float *ra = a->head + (ptrdiff_t)a->low_y[y] * a->hy;
            const float *rb = a->head + (ptrdiff_t)a->high_y[y] * a->hy;
            for (size_t i = 0; i < a->head_width; ++i)
                for (size_t c = 0; c < channels; ++c)
                    row[i * channels + c] =
                        ra[(ptrdiff_t)i * a->hx + (ptrdiff_t)c * a->hc] * other
                        + rb[(ptrdiff_t)i * a->hx + (ptrdiff_t)c * a->hc] * w;
            line = row;
            lx = (ptrdiff_t)channels;
            lc = 1;
        } else {
            line = a->head + (ptrdiff_t)y * a->hy;
            lx = a->hx;
            lc = a->hc;
        }
        if (a->fast && lc == 1) {
            compose_encode_row(line, (int)lx, low_x, high_x, weight_x,
                               a->colour + (ptrdiff_t)y * a->sy,
                               a->history ? a->history + (ptrdiff_t)y * a->ry : NULL,
                               a->previous ? a->previous + (ptrdiff_t)y * a->py : NULL,
                               a->table, a->confidence, a->intensity, a->still, a->scale,
                               a->hold, a->slope, a->release, a->grade, (int)width, rows,
                               a->output + y * width * 3,
                               a->neural ? a->neural + y * width * 3 : NULL,
                               a->encoded + ((a->top + y) * a->frame_width + a->left) * 4,
                               a->bgra);
            if (sampled && y % step == 0) {
                for (size_t x = 0; x < width; x += step) {
                    float w = weight_x[x], other = 1.0f - w;
                    const float *pa = line + (ptrdiff_t)low_x[x] * lx;
                    const float *pb = line + (ptrdiff_t)high_x[x] * lx;
                    float *to = a->samples + ((y / step) * sampled + x / step) * channels;
                    for (size_t c = 0; c < channels; ++c) to[c] = pa[c] * other + pb[c] * w;
                }
            }
            continue;
        }
        for (size_t x = 0; x < width; ++x) {
            float h[4];
            if (low_x) {
                float w = weight_x[x], other = 1.0f - w;
                const float *pa = line + (ptrdiff_t)low_x[x] * lx;
                const float *pb = line + (ptrdiff_t)high_x[x] * lx;
                for (size_t c = 0; c < channels; ++c)
                    h[c] = pa[(ptrdiff_t)c * lc] * other + pb[(ptrdiff_t)c * lc] * w;
            } else {
                const float *pa = line + (ptrdiff_t)x * lx;
                for (size_t c = 0; c < channels; ++c) h[c] = pa[(ptrdiff_t)c * lc];
            }
            if (sampled && y % step == 0 && x % step == 0)
                memcpy(a->samples + ((y / step) * sampled + x / step) * channels, h,
                       channels * sizeof *h);
            const float *rgb = a->colour + (ptrdiff_t)y * a->sy + (ptrdiff_t)x * a->sx;
            float *out = a->output + (y * width + x) * 3;
            if (a->history) {
                temporal_pixel(h, 1, rgb, a->sc,
                               a->history + (ptrdiff_t)y * a->ry + (ptrdiff_t)x * a->rx, a->rc,
                               a->previous ? a->previous + (ptrdiff_t)y * a->py
                                                         + (ptrdiff_t)x * a->px : NULL,
                               a->pc, NULL, a->table, a->confidence,
                               a->mask ? a->mask + (ptrdiff_t)y * a->my
                                                 + (ptrdiff_t)x * a->mx : NULL,
                               a->intensity, a->scale, a->hold, a->slope, a->release,
                               a->grade, a->neural ? a->neural + (y * width + x) * 3 : NULL,
                               out);
            } else {
                float predicted[3];
                for (size_t c = 0; c < 3; ++c)
                    predicted[c] = unit(rgb[(ptrdiff_t)c * a->sc] + half(h[c]) * 0.25f);
                finish_pixel(predicted, rgb, a->sc, a->still, a->grade,
                             a->neural ? a->neural + (y * width + x) * 3 : NULL, out);
            }
            uint8_t *pixel = a->encoded + ((a->top + y) * a->frame_width + a->left + x) * 4;
            for (size_t c = 0; c < 3; ++c) {
                float value = out[c];
                /* NumPy's byte cast maps NaN to zero; do not cast NaN in C. */
                value = value == value ? unit(value) : 0.0f;
                pixel[a->bgra ? 2 - c : c] = (uint8_t)(value * 255.0f + 0.5f);
            }
        }
    }
    free(row);
    free(rows);
}

/* The head's upscale, the composition and the codec in one pass over the output.
 *
 * Separately they are three passes over the full frame — the bilinear resize writes the
 * head at the output's size through an intermediate, the composition reads it back, and
 * the encoder reads the composition again — about 100 MB of memory traffic a 1280x720
 * frame, where this moves about half of it. Each output row takes the resize's first axis
 * into a row of its own (the same float32 intermediate `nr_resize_axis` stores), then per
 * pixel the second axis, the composition — `temporal_pixel` with a history, `nr_compose`'s
 * arithmetic without — and the encoder's rounding into `encoded`, a copy of the request
 * whose alpha and whose rows and columns outside the active region stay as they came.
 *
 * `low_y`/`high_y`/`weight_y` and the `_x` three are `nr_image._axis_plan`'s, NULL for an
 * axis already at the output's extent; `channels` is the head's, 4 with a history and 3
 * without. The composition is still written to `output`, for the history and the log, and
 * the upscaled head itself on every `step`-th row and column into `samples` when given —
 * the values the log's gate figure reads. `grade` and `neural` are `nr_compose`'s.
 */
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
                       size_t top, size_t left, int bgra, float *samples, size_t step)
{
    struct compose_encode_args a = {
        head, hy, hx, hc, head_width, channels, low_y, high_y, weight_y,
        low_x, high_x, weight_x, colour, sy, sx, sc, history, ry, rx, rc,
        previous, py, px, pc, table, confidence, mask, my, mx,
        width, intensity,
        /* nr_compose's blend: below 1 clamped to [0, 1], above it extrapolating */
        intensity > 1.0f ? intensity : unit(intensity),
        scale, hold, slope, release, output, grade, neural, encoded, frame_width, top, left,
        bgra,
        samples, step, samples && step ? (width + step - 1) / step : 0,
        /* the daemon's layout, which `compose_encode_row` takes with its strides fixed */
        low_x && sx == 3 && sc == 1 && !mask && channels == (history ? 4u : 3u)
            && (!history || (rx == 3 && rc == 1 && table))
            && (!previous || (px == 3 && pc == 1))
            && width < (1u << 24) && head_width * channels < (1u << 24),
    };
    nr_parallel_rows(height, compose_encode_rows, &a);
}
