/*
 * nr_portable.h — the handful of things the C here needs that C11 does not give the same
 * way on Linux, macOS and Windows: half-precision conversion, a thread-local, dynamic
 * loading, a clock, and the string and file odds and ends MSVC spells differently.
 *
 * The half conversions are the one place correctness lives. `_Float16` is used where the
 * compiler has it (GCC and Clang, which is every build this project has measured); MSVC,
 * which has no such type, converts through the hardware's own instruction where the build
 * says the CPU has it — F16C on x86-64 (`NR_F16C`, the x86-64-v3 floor), NEON on ARM64 —
 * and the fallback is a software round-to-nearest-even conversion for the rest and for the
 * RISC-V targets that cannot hold a half in a vector (below), checked against the type
 * exhaustively — every one of the 65 536 halves widens to the same float, and every one
 * of the 2^32 floats narrows to the same half, NaN payloads aside (notes/phase69).
 * `NR_NO_FLOAT16` forces the fallback where the type exists, which is how that was run;
 * `NR_NEON_HALF` forces the NEON path the same way.
 */
#ifndef NR_PORTABLE_H
#define NR_PORTABLE_H
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
#define __USE_GNU
#include <dlfcn.h>
#endif

/* -- half precision ------------------------------------------------------ */

/* Clang defines __FLT16_MANT_DIG__ for every riscv64 target, Android's included, and that
 * one has the V extension but neither Zfh nor Zvfh: no half in a scalar FP register and
 * none in a vector. The loop vectorizer still turns the conversions below into
 * <vscale x N x half> fptrunc/fpext, which the backend can only lower by scalarizing --
 * and scalarizing a scalable vector is unimplemented, so the compile dies outright with
 * "Scalarization of scalable vectors is not supported" (`nr_compose` in nr_image.c, and
 * the C frame test). Without Zfh the type is a libcall on that target anyway, so the
 * software path costs it nothing. Vectors that can hold a half keep `_Float16`.
 */
#if defined(__riscv_vector) && !defined(__riscv_zvfh) && !defined(NR_NO_FLOAT16)
#define NR_NO_FLOAT16 1
#endif

#if !defined(NR_NO_FLOAT16) && !defined(NR_NEON_HALF) \
    && (defined(__FLT16_MANT_DIG__) || (defined(__clang__) && defined(__aarch64__)))
#define NR_HAVE_FLOAT16 1
#endif

/* Without `_Float16` — MSVC — the conversion is F16C's `vcvtps2ph` / `vcvtph2ps` where the
 * build says the CPU has it: NR_F16C from CMake's x86-64 floor or build_win.bat, or a build
 * for AVX2, whose level (x86-64-v3) includes F16C. One instruction each way, the hardware's
 * rounding, and the same bits as the software path below on every float but a signalling
 * NaN, which F16C quiets and `nr_float_to_half` never makes (upstream, 2026-10-07). MSVC
 * declares only the vector forms, so the scalar is lane 0 of them. A compiler with
 * `_Float16` emits the same instructions from the cast under -march=x86-64-v3. */
#if !defined(NR_HAVE_FLOAT16) && !defined(NR_NO_FLOAT16) && defined(_MSC_VER) && !defined(__clang__) \
    && (defined(NR_F16C) || defined(__AVX2__)) && (defined(_M_X64) || defined(_M_IX86))
#define NR_HAVE_F16C 1
#include <immintrin.h>
#endif

/* The same on ARM64 without `_Float16` — MSVC for ARM64 (and ARM64EC), or a GCC too old for
 * the type — through NEON's `fcvtl` / `fcvtn`, the F16C block's twin: lane 0 of the vector
 * conversions, rounding to nearest even under the default FPCR, overflow to infinity, a NaN
 * quieted. FCVT between half and single is base ARMv8.0 floating point (only half
 * *arithmetic* needs ARMv8.2's FP16 extension), so no `-march` floor is needed for it, and
 * GCC and Clang emit the very same instruction from the `_Float16` cast above — which is why
 * the floor is empty on arm64 (Makefile, CMakeLists.txt). `NR_NEON_HALF` forces this path on
 * a compiler that has `_Float16`, which is how it was checked against the type exhaustively
 * on an Apple M3 (every float, every half, NaN payloads aside). */
#if !defined(NR_HAVE_FLOAT16) && !defined(NR_NO_FLOAT16) && !defined(NR_HAVE_F16C) \
    && (defined(NR_NEON_HALF) || defined(_M_ARM64) || defined(_M_ARM64EC) \
        || (defined(__aarch64__) && defined(__ARM_NEON)))
#define NR_HAVE_NEON_HALF 1
#  if defined(_MSC_VER) && !defined(__clang__)
#    include <arm64_neon.h>
#  else
#    include <arm_neon.h>
#  endif
#endif

static inline float nr_half_to_float(uint16_t h)
{
#ifdef NR_HAVE_FLOAT16
    _Float16 v;
    memcpy(&v, &h, 2);
    return (float)v;
#elif defined(NR_HAVE_F16C)
    return _mm_cvtss_f32(_mm_cvtph_ps(_mm_cvtsi32_si128((int)h)));
#elif defined(NR_HAVE_NEON_HALF)
    return vgetq_lane_f32(vcvt_f32_f16(vreinterpret_f16_u16(vdup_n_u16(h))), 0);
#else
    uint32_t sign = (uint32_t)(h & 0x8000u) << 16;
    uint32_t exponent = (h >> 10) & 0x1Fu, mantissa = h & 0x3FFu, bits;
    if (exponent == 0) {
        if (mantissa == 0) bits = sign;
        else {                                  /* subnormal: normalise */
            int shift = 0;
            while (!(mantissa & 0x400u)) { mantissa <<= 1; shift++; }
            mantissa &= 0x3FFu;
            bits = sign | ((uint32_t)(127 - 15 - shift + 1) << 23) | (mantissa << 13);
        }
    } else if (exponent == 31) {
        bits = sign | 0x7F800000u | (mantissa << 13);
    } else {
        bits = sign | ((exponent + 127 - 15) << 23) | (mantissa << 13);
    }
    float f;
    memcpy(&f, &bits, 4);
    return f;
#endif
}

static inline uint16_t nr_float_to_half(float f)
{
#ifdef NR_HAVE_FLOAT16
    _Float16 v = (_Float16)f;                   /* round to nearest even, overflow to infinity */
    uint16_t h;
    memcpy(&h, &v, 2);
    return h;
#elif defined(NR_HAVE_F16C)
    return (uint16_t)_mm_cvtsi128_si32(_mm_cvtps_ph(_mm_set_ss(f), _MM_FROUND_TO_NEAREST_INT));
#elif defined(NR_HAVE_NEON_HALF)
    return vget_lane_u16(vreinterpret_u16_f16(vcvt_f16_f32(vdupq_n_f32(f))), 0);
#else
    uint32_t bits;
    memcpy(&bits, &f, 4);
    uint16_t sign = (uint16_t)((bits >> 16) & 0x8000u);
    uint32_t abs = bits & 0x7FFFFFFFu;
    if (abs >= 0x7F800000u)                     /* inf or nan */
        return (uint16_t)(sign | 0x7C00u | (abs > 0x7F800000u ? 0x200u : 0u));
    if (abs >= 0x477FF000u)                     /* rounds to or past the largest half */
        return (uint16_t)(sign | 0x7C00u);
    if (abs < 0x33000000u) return sign;         /* below half the smallest subnormal */
    int32_t exponent = (int32_t)(abs >> 23) - 127 + 15;
    uint32_t mantissa = abs & 0x7FFFFFu;
    uint32_t half_bits, shift;
    if (exponent <= 0) {                        /* subnormal in half */
        mantissa |= 0x800000u;
        shift = (uint32_t)(14 - exponent);
        half_bits = mantissa >> shift;
        uint32_t remainder = mantissa & ((1u << shift) - 1u), halfway = 1u << (shift - 1);
        if (remainder > halfway || (remainder == halfway && (half_bits & 1u))) half_bits++;
        return (uint16_t)(sign | half_bits);
    }
    half_bits = ((uint32_t)exponent << 10) | (mantissa >> 13);
    uint32_t remainder = mantissa & 0x1FFFu;
    if (remainder > 0x1000u || (remainder == 0x1000u && (half_bits & 1u))) half_bits++;
    return (uint16_t)(sign | half_bits);
#endif
}

static inline float nr_half_round(float f) { return nr_half_to_float(nr_float_to_half(f)); }

/* -- thread-local, strings, files ---------------------------------------- */

#ifdef _MSC_VER
#define NR_THREAD_LOCAL __declspec(thread)
#define nr_strdup _strdup
#define nr_strtok_r strtok_s
#else
#define NR_THREAD_LOCAL _Thread_local
#define nr_strdup strdup
#define nr_strtok_r strtok_r
#endif

/* -- a clock ------------------------------------------------------------- */

static inline double nr_now(void)
{
#ifdef _WIN32
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + ts.tv_nsec * 1e-9;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec * 1e-9;
#endif
}

/* -- dynamic loading ------------------------------------------------------- */

#ifdef _WIN32
#define NR_SHARED_SUFFIX ".dll"
#elif defined(__APPLE__)
#define NR_SHARED_SUFFIX ".dylib"
#else
#define NR_SHARED_SUFFIX ".so"
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
typedef HMODULE nr_dl;
static inline nr_dl nr_dl_open(const char *path) { return LoadLibraryA(path); }
static inline void *nr_dl_sym(nr_dl handle, const char *name) { return (void *)GetProcAddress(handle, name); }
static inline const char *nr_dl_error(void)
{
    static NR_THREAD_LOCAL char text[64];

#if __STDC_WANT_SECURE_LIB__
    sprintf_s(text, sizeof text, "error %lu", (unsigned long)GetLastError());
#else
    snprintf(text, sizeof text, "error %lu", (unsigned long)GetLastError());
#endif

    return text;
}
/* The directory holding the module that contains `symbol`. */
static inline int nr_dl_self_dir(const void *symbol, char *out, size_t cap)
{
    HMODULE module = NULL;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCSTR)symbol, &module)) return -1;
    if (!GetModuleFileNameA(module, out, (DWORD)cap)) return -1;
    char *slash = strrchr(out, '\\'), *fwd = strrchr(out, '/');
    if (fwd > slash) slash = fwd;
    if (slash) *slash = 0; else snprintf(out, cap, ".");
    return 0;
}
#else
#include <dlfcn.h>
typedef void *nr_dl;
static inline nr_dl nr_dl_open(const char *path) { return dlopen(path, RTLD_NOW | RTLD_GLOBAL); }
static inline void *nr_dl_sym(nr_dl handle, const char *name) { return dlsym(handle, name); }
static inline const char *nr_dl_error(void) { const char *e = dlerror(); return e ? e : "unknown error"; }
static inline int nr_dl_self_dir(const void *symbol, char *out, size_t cap)
{
    Dl_info info;
    if (!dladdr((void *)symbol, &info) || !info.dli_fname) return -1;
    snprintf(out, cap, "%s", info.dli_fname);
    char *slash = strrchr(out, '/');
    if (slash) *slash = 0; else snprintf(out, cap, ".");
    return 0;
}
#endif

/* -- a read-only file mapping ---------------------------------------------- */

/* The whole of a file mapped read-only, so a reader can point into it instead of copying:
 * the weights (nr_frame.c's safetensors reader aliases every tensor into the mapping, as it
 * aliases the compiled-in slices). `nr_file_map` fills `m` and returns 0, or -1 with errno
 * set; `nr_file_unmap` releases it. An empty file maps to NULL with size 0. */
struct nr_mapping {
    const unsigned char *data;
    size_t size;
    void *file, *mapping;       /* the two handles Win32 keeps open under a view */
};

#ifdef _WIN32
static inline int nr_file_map(const char *path, struct nr_mapping *m)
{
    memset(m, 0, sizeof *m);
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) { errno = ENOENT; return -1; }
    LARGE_INTEGER size;
    if (!GetFileSizeEx(file, &size)) { CloseHandle(file); errno = EIO; return -1; }
    m->file = file;
    m->size = (size_t)size.QuadPart;
    if (!m->size) return 0;
    HANDLE mapping = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!mapping) { CloseHandle(file); m->file = NULL; errno = ENOMEM; return -1; }
    m->mapping = mapping;
    m->data = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (!m->data) { CloseHandle(mapping); CloseHandle(file); memset(m, 0, sizeof *m); errno = ENOMEM; return -1; }
    return 0;
}
static inline void nr_file_unmap(struct nr_mapping *m)
{
    if (m->data) UnmapViewOfFile((LPCVOID)m->data);
    if (m->mapping) CloseHandle((HANDLE)m->mapping);
    if (m->file) CloseHandle((HANDLE)m->file);
    memset(m, 0, sizeof *m);
}
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
static inline int nr_file_map(const char *path, struct nr_mapping *m)
{
    memset(m, 0, sizeof *m);
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    struct stat st;
    if (fstat(fd, &st)) { int e = errno; close(fd); errno = e; return -1; }
    m->size = (size_t)st.st_size;
    if (m->size) {
        void *data = mmap(NULL, m->size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) { int e = errno; close(fd); errno = e; m->size = 0; return -1; }
        m->data = data;
    }
    close(fd);
    return 0;
}
static inline void nr_file_unmap(struct nr_mapping *m)
{
    if (m->data) munmap((void *)m->data, m->size);
    memset(m, 0, sizeof *m);
}
#endif

/* A whole file read into one malloc'd block (`*size` its length), or NULL with errno. The
 * alternative to a mapping where faulting one in is the slow part (macOS: 0.9-1.3 s of page
 * faults over 291 MB, against a 46 ms read). The block is 16-byte aligned, as malloc gives. */
static inline unsigned char *nr_file_read(const char *path, size_t *size)
{
    *size = 0;
    FILE *f;
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    if (fopen_s(&f, path, "rb")) return NULL;
#else
    f = fopen(path, "rb");
    if (!f) return NULL;
#endif
    struct nr_mapping probe;
    unsigned char *block = NULL;
    size_t n = 0;
    /* the length without seeking past 2 GB on a 32-bit long: a mapping says it */
    if (nr_file_map(path, &probe) == 0) { n = probe.size; nr_file_unmap(&probe); }
    else { fclose(f); return NULL; }
    block = malloc(n ? n : 1);
    if (!block) { fclose(f); errno = ENOMEM; return NULL; }
    size_t got = 0;
    while (got < n) {
        size_t r = fread(block + got, 1, n - got, f);
        if (!r) break;
        got += r;
    }
    fclose(f);
    if (got != n) { free(block); errno = EIO; return NULL; }
    *size = n;
    return block;
}

/* Ask the system to bring a mapped range in before it is touched: a frame's first uploads
 * copy ~300 MB out of the weights in block order, and a page fault per 16 KB page, scattered,
 * costs macOS 0.9-1.3 s where `madvise(MADV_WILLNEED)` costs 190 ms (HANDOFF, 2026-10-10).
 * Windows has PrefetchVirtualMemory from 8 on, resolved at run time; elsewhere nothing. The
 * range is rounded out to page bounds, which madvise requires. */
static inline void nr_prefault(const void *data, size_t size)
{
    if (!data || !size) return;
#ifdef _WIN32
    typedef struct { PVOID VirtualAddress; SIZE_T NumberOfBytes; } nr_win32_range;
    typedef BOOL (WINAPI *nr_prefetch_fn)(HANDLE, ULONG_PTR, nr_win32_range *, ULONG);
    static nr_prefetch_fn prefetch;
    static int looked;
    if (!looked) {
        looked = 1;
        HMODULE k32 = GetModuleHandleA("kernel32.dll");
        if (k32) prefetch = (nr_prefetch_fn)(void *)GetProcAddress(k32, "PrefetchVirtualMemory");
    }
    if (prefetch) {
        nr_win32_range range = { (PVOID)data, (SIZE_T)size };
        prefetch(GetCurrentProcess(), 1, &range, 0);
    }
#else
    long page = sysconf(_SC_PAGESIZE);
    if (page <= 0) page = 4096;
    uintptr_t start = (uintptr_t)data & ~((uintptr_t)page - 1);
    uintptr_t end = ((uintptr_t)data + size + (uintptr_t)page - 1) & ~((uintptr_t)page - 1);
    madvise((void *)start, (size_t)(end - start), MADV_WILLNEED);
#endif
}

/* -- environment and scratch files ---------------------------------------- */

static inline void nr_setenv_default(const char *name, const char *value)
{
    if (getenv(name)) return;
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 0);
#endif
}

/* Somewhere writable for a scratch file: TMPDIR, TEMP or TMP if set, else /tmp or `.` --
 * and on Android, which has no /tmp, the directory adb shell can write to. */
static inline const char *nr_temp_dir(void)
{
    const char *names[] = { "TMPDIR", "TEMP", "TMP" };
    for (size_t i = 0; i < 3; i++) { const char *v = getenv(names[i]); if (v && *v) return v; }
#ifdef _WIN32
    return ".";
#elif defined(__ANDROID__)
    return "/data/local/tmp";
#else
    return "/tmp";
#endif
}

#ifdef _WIN32
#include <process.h>
#define nr_getpid _getpid
#else
#include <unistd.h>
#define nr_getpid getpid
#endif

#endif
