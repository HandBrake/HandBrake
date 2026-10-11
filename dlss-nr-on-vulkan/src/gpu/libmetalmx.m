/*
 * libmetalmx — libxmx on Metal: the same entry points, the same contract, driving Apple
 * silicon's own API instead of Vulkan through MoltenVK.
 *
 * Every `xmx_*` symbol in `xmx.h` is here, so `nr_frame.c`, `xmx.py` and `xmxres.py` bind
 * this library exactly as they bind libxmx (`NR_GPU_BACKEND=metal` makes them). Shader
 * names stay the SPIR-V names: `xmx_init("gemm_coopmat.spv")` runs the Metal kernel
 * `gemm_coopmat` from the metallib compiled into this binary (`nr_metallib.h`, bin2c over
 * `nr_shaders.metallib`); a path is reduced to its base name and looked up the same way,
 * and `XMX_METALLIB=/path/to.metallib` loads a file instead of the embedded library.
 *
 * What maps onto what:
 *
 *   VkBuffer + device address     MTLBuffer, its GPU address in the push block. The address
 *                                 is not read from `gpuAddress` (macOS 13): an
 *                                 `MTLArgumentEncoder` over one pointer argument (Metal 2,
 *                                 macOS 10.13) writes the same 64-bit value into an argument
 *                                 buffer, and `addr_ptr()` reads it back. The graph's
 *                                 buffers are `MTLStorageModeShared` on unified memory (so
 *                                 `xmx_buf_ptr` is `contents`, as HOST_CACHED was), and
 *                                 `MTLStorageModePrivate` under XMX_STAGING=1 or on a card
 *                                 without unified memory, reached by blit copies.
 *   VK_KHR_cooperative_matrix     `simdgroup_matrix` (`MTLGPUFamilyApple7` and up):
 *                                 `xmx_coopmat()` says so, and `XMX_PORTABLE=1` still picks
 *                                 the multiply-add kernels.
 *   specialization constant 0     function constant 0 (`MTLFunctionConstantValues`).
 *   push constants                `setBytes` of the same 128-byte struct at buffer index 0.
 *   vkCmdPipelineBarrier          `memoryBarrierWithScope:MTLBarrierScopeBuffers` inside a
 *                                 concurrent-dispatch encoder, one per pass unless
 *                                 `xmx_sync(0)` suppresses it — the same places. On a GPU
 *                                 that is not Apple's the encoder dispatches serially too:
 *                                 AMD's driver does not order passes at those barriers.
 *   specialization constant       function constant 2: `half_round` on the bits, where the
 *     (`half_round`'s spelling)   compiler folds the cast away (AMD's; `xmx_half_by_cast`).
 *   vkCmdCopyBuffer               a blit encoder between two compute encoders.
 *   a recorded command buffer     a host-side op list. A MTLCommandBuffer cannot be
 *                                 submitted twice, so a graph is the recorded list and
 *                                 `xmx_graph_run` encodes it again into a fresh command
 *                                 buffer each time (~1 ms for a frame's 1700 passes).
 *   vkCmdWriteTimestamp           counter sampling at encoder boundaries: with profiling
 *                                 on, every pass gets its own encoder with a timestamp pair.
 *   xmx_adopt                     refused: there is no Vulkan device here to adopt.
 *
 * Numerics are the shaders' business and they are compiled with `-fno-fast-math
 * -ffp-contract=off` (see `metal/nr_metal.h`); nothing here changes a value.
 *
 * Build (Apple only): clang -fobjc-arc -O2 -shared -fPIC -o libmetalmx.dylib libmetalmx.m
 *                     -framework Metal -framework Foundation
 */
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "xmx.h"
#ifdef NR_EMBEDDED_METALLIB
#include "nr_metallib.h"                      /* const unsigned char nr_metallib[]; bin2c */
#endif
/* The same kernels compiled as Metal 4 (`-std=metal4.0 -DNR_METAL4`, metal/gemm_simd.metal):
 * the GEMMs' K loop on Metal Performance Primitives' matmul2d. Loaded instead of the Metal
 * 3.1 library on a device and OS in MTLGPUFamilyMetal4 on the simdgroup path, unless
 * XMX_METAL4=0; everything else — an older macOS (the deployment target is 11), another
 * GPU, the portable path, an SDK too old to build it — keeps the Metal 3.1 library. The two
 * give the same bytes: matmul2d sums in the order the hand-written loops do (measured). */
#ifdef NR_EMBEDDED_METALLIB4
#include "nr_metallib4.h"                     /* const unsigned char nr_metallib4[]; bin2c */
#endif

#define FAIL(msg, r) do { snprintf(g.err, sizeof g.err, "%s (%d)", msg, (int)(r)); return -1; } while (0)
#define FAILNS(msg, error) do { snprintf(g.err, sizeof g.err, "%s: %s", msg, \
	(error) ? [[(error) localizedDescription] UTF8String] : "unknown error"); return -1; } while (0)

/* The fixed Metal objects live in globals of their own type: ARC manages those. Objects
 * kept in C tables — buffers, pipelines — are bridged to `void *` with a retain of their
 * own, and released where the table entry dies. */
static id<MTLDevice> dev;
static id<MTLCommandQueue> queue;
/* Orders one encoder after the previous within a command buffer. The graph's buffers are
 * untracked, so Metal would otherwise be free to overlap a blit with the compute encoder
 * that wrote its source; every encoder here updates the fence on its way out and the next
 * waits on it on its way in. */
static id<MTLFence> fence;
static id<MTLLibrary> library;
static id<MTLBinaryArchive> archive;           /* XMX_PIPELINE_IR: the compiled pipelines */
/* The address of a buffer, without `gpuAddress`: encode it as the one pointer argument of
 * this encoder into `addr_scratch` and read the 8 bytes back. Argument buffers hold real
 * device addresses, the same value `gpuAddress` reports, on every macOS with Metal 2. */
static id<MTLArgumentEncoder> addr_encoder;
static id<MTLBuffer> addr_scratch;
/* Timestamp sample buffers, one pair of samples per profiled pass. A sample buffer is
 * capped at 32 KB — 4096 timestamps, 2048 passes — so a frame's 1700-odd passes need a
 * second one and the limit below needs four. */
#define SAMPLE_PAIRS 2048
#define SAMPLE_BUFFERS 4
static id<MTLCounterSampleBuffer> samples[SAMPLE_BUFFERS];
static unsigned sample_buffers;                /* how many are allocated */

struct buf { void *b; void *p; uint64_t cap; };

struct push {
	uint64_t a, b, c, d;
	uint32_t m, n, k, batch, sa, sb, sc, flags;
	float p0, p1, p2, p3;
	uint32_t lda, ldb, ldc, spare;
	/* The fused passes' operands, appended as in libxmx.c: the residual's cosine (and any
	 * pass's fifth operand), the window-layout geometry, the QKV scale (the fused
	 * feed-forward's projection). 128 bytes in all. */
	uint64_t residual_cos;
	uint32_t image_h, image_w, window_cols, window_pad;
	uint64_t qkv_scale;
};
_Static_assert(sizeof(struct push) == 128, "push block matches nr_metal.h");

struct desc_push { uint32_t M, N, K, sa, sb, sc, bt; };

static struct {
	void *pipe, *pipeb;                            /* the descriptor path */
	void *rgemm, *rtiled, *rstaged, *runary, *rrow, *rhistory;
	void *rwindow[2], *rffn;                       /* the fused passes, built on first use */
	void *rstaged32[2];       /* the staged kernel on 32-row blocks, a 32- and a 64-deep K step */
	void *rrows;              /* the whole-row softmax on 256 threads */
	void *rint8;              /* the integer GEMM, built on first use */
	void *rblock;             /* a 32-channel window block's attention half, likewise */
	void *rglobal;            /* a bottleneck block's attention in one pass, likewise */
	void *rwide;              /* the portable 16x64 build (family 12), portable path only */
	unsigned wide_calls;      /* how many GEMMs it has recorded */
	unsigned portable_tiled;  /* XMX_PORTABLE_TILED=1: the 16x32 build takes every shape again */
	char *rpaths[13];
	unsigned specialize;
	unsigned tiling, tilem, tilen;
	int syncing;
	unsigned staging;
	unsigned staged_partial;  /* the staged kernel takes M that is not whole 64-row blocks */
	unsigned staged32;        /* M of 32 or fewer takes the 32-row staged build */
	unsigned staged32_calls;  /* how many GEMMs it has recorded: tests check the routing */
	int recording, recorded, rready;
	unsigned prof, prof_n; double ns_per_tick;
	char name[256]; char err[512]; char memory[256];
	int ready, lost, discrete, unmapped, coopmat, portable;
	int metal4;               /* the Metal 4 library is the one loaded (or to be loaded) */
	int half_bits;            /* half_round on the bits (function constant 2): the cast does not round */
	int staged_packed;        /* the staged kernel's raw 128-bit operand copies (function constant 3) */
	int portable_packed;      /* the portable GEMM's 16-byte A fetch (function constant 4), off */
	int serial;               /* compute encoders dispatch serially: a barrier does not order them */
	int prof_dispatch;        /* timestamps between dispatches: the device has none at encoder boundaries */
	unsigned simd_width;      /* the widest threadExecutionWidth a pipeline has reported */
	/* XMX_PIPELINE_STATS=FILE: what Metal says of each pipeline it compiled, appended to FILE;
	 * XMX_PIPELINE_IR=DIR: the compiled pipelines serialised there as one binary archive */
	FILE *stats;
	const char *ir_dir;
	struct buf A, B, C, stage;
} g;

#define MAX_SPECIALIZED 256
static struct { unsigned family, flags; void *pipeline; } specialized[MAX_SPECIALIZED];
static unsigned specialized_count;

/* A pass's kind is `family * 32 + subkind`, as in libxmx. */
#define MAX_STAMPS 8192
#define PROF_KINDS 256
enum { PK_GEMM = 0, PK_TILED, PK_STAGED, PK_UNARY, PK_ROW, PK_HISTORY, PK_COPY, PK_START = 7 };
static double prof_ms[PROF_KINDS];
static unsigned prof_hits[PROF_KINDS];
/* Every profiled pass's own duration, in recording order, since the last reset. */
static double prof_each[MAX_STAMPS];
static unsigned char prof_each_kind[MAX_STAMPS];   /* the kind each timed pass was stamped with */
static unsigned prof_each_n;

/* Device-resident buffers. */
#define MAX_RBUF 8192
struct rbuf { void *b; void *p; uint64_t addr, size; int live, mapped; };
static struct rbuf rbufs[MAX_RBUF];

/* What a recording is: the passes, kept host-side, encoded at submit. */
enum { OP_DISPATCH, OP_COPY, OP_BARRIER };
struct op {
	unsigned char kind, stamp;
	void *pipeline;                                /* unretained: the tables own it */
	struct push p;
	unsigned gx, gy, gz, tg;
	int src, dst; uint64_t so, to, bytes;
};
struct oplist { struct op *ops; unsigned n, cap; int passes, live; };
static struct oplist rec;
#define MAX_GRAPHS 128
static struct oplist graphs[MAX_GRAPHS];

static int op_push(struct oplist *l, const struct op *op)
{
	if (l->n == l->cap) {
		unsigned cap = l->cap ? l->cap * 2 : 1024;
		struct op *grown = realloc(l->ops, cap * sizeof *grown);
		if (!grown) return -1;
		l->ops = grown; l->cap = cap;
	}
	l->ops[l->n++] = *op;
	return 0;
}

static void op_free(struct oplist *l) { free(l->ops); memset(l, 0, sizeof *l); }

/* -- queries ------------------------------------------------------------ */

const char *xmx_error(void) { return g.err; }
int xmx_device_lost(void) { return g.lost; }
const char *xmx_device(void) { return g.name; }
const char *xmx_memory(void)
{
	if (!dev) return "not initialised";
	if (!g.memory[0])
		snprintf(g.memory, sizeof g.memory, "%s",
			 g.unmapped ? "private storage, the host reaches it by copies (MTLStorageModePrivate"
				      ": staging forced, or no unified memory)"
			 : g.discrete ? "shared storage on a card without unified memory: system memory"
			 : "unified memory (MTLStorageModeShared, one pool)");
	return g.memory;
}
int xmx_coopmat(void) { return dev ? g.coopmat : -1; }
int xmx_portable(void) { return dev ? g.portable : -1; }
/* every Metal path gathers a window-ordered A: the staged or tiled simdgroup kernel, or
 * the tiled portable one */
int xmx_window_gather(void) { return dev ? 1 : -1; }
const char *xmx_path(void)
{
	if (!dev) return "not opened";
	if (!g.portable)
		return g.metal4 ? "simdgroup matrix, Metal 4 library (matmul2d GEMMs, fp16 x fp16 -> fp32)"
				: "simdgroup matrix, Metal 3.1 library (simdgroup_multiply_accumulate, fp16 x fp16 -> fp32)";
	return g.coopmat ? "portable multiply-add (XMX_PORTABLE=1; the device has simdgroup matrices)"
			 : "portable multiply-add (the device has no simdgroup matrix support)";
}
int xmx_staging_mode(void) { return g.unmapped; }
int xmx_discrete(void) { return g.discrete; }

/* Half subnormals through the GEMMs (xmx.h). Metal has no float-controls mode to declare:
 * the compiler runs with fast math off (nr_metal.h), under which the Metal Shading Language
 * keeps half denormals, and `simdgroup_matrix` multiplies them as the ALUs do — measured on
 * an M3 by `src/gpu/test_denorm.py`, which is the check on any other Apple GPU. So the answer
 * is what the device does, and `XMX_DENORM16=driver` changes nothing here. */
int xmx_preserve16(void) { return dev ? 1 : 0; }

/* `half_round` in nr_metal.h is the cast, `float(half(x))`, where the device rounds it, and
 * the bits otherwise (`probe_half_rounding`); -1 before the library is loaded. */
int xmx_half_by_cast(void)
{
	return library ? !g.half_bits : -1;
}

/* Whether the staged kernel copies its operands as raw 128-bit vectors, function constant 3
 * (xmx_init; `XMX_STAGED_PACKED=0` restores the half4 loader), and whether the portable GEMM
 * fetches A that way, function constant 4 (`XMX_PORTABLE_PACKED=1`; off by default). */
int xmx_staged_packed(void) { return g.staged_packed; }
int xmx_portable_packed(void) { return g.portable_packed; }

/* -- shaders --------------------------------------------------------------- */

/* The kernels the metallib carries, under the SPIR-V names every caller uses. */
static const char *const kernel_names[] = {
	"gemm_resident", "gemm_tiled", "gemm_staged", "resident", "attention", "attention_ab",
	"history", "gemm_coopmat", "gemm_batched", "gemm_f16acc", "gemm_portable",
	"gemm_portable_tiled", "gemm_portable_wide", "gemm_portable_desc", "gemm_portable_batched",
	"window_attention", "window_attention_portable", "ffn_fused", "ffn_fused_portable",
	"gemm_staged32", "gemm_staged32_deep", "attention_rows",
	"window_block", "window_block_portable", "global_attention", "global_attention_portable",
	"gemm_staged_int8", "gemm_staged_int8_portable",
	"half_probe",             /* the daemon's start-up probe (src/gpu/metal/half_probe.metal) */
};

/* The fused passes have a matrix kernel and a portable twin; a caller that hands over the
 * matrix name on the portable path (XMX_PORTABLE=1, or no simdgroup matrices) gets the
 * twin, so the name alone never selects a kernel the path cannot run. */
static void path_kernel(char *stem, size_t cap)
{
	/* the integer GEMM is one kernel on Metal, under both names (gemm_int8.metal) */
	if (!strcmp(stem, "gemm_staged_int8_portable")) stem[strlen("gemm_staged_int8")] = 0;
	if (!g.portable) return;
	if (!strcmp(stem, "window_attention") || !strcmp(stem, "ffn_fused")
	    || !strcmp(stem, "window_block") || !strcmp(stem, "global_attention"))
		strncat(stem, "_portable", cap - strlen(stem) - 1);
}

/* `gemm_coopmat.spv`, `/some/dir/attention_ab.spv`, `resident` -> the kernel's name. */
static void kernel_name(const char *spv_path, char *out, size_t cap)
{
	const char *base = spv_path;
	for (const char *q = spv_path; *q; q++)
		if (*q == '/' || *q == '\\') base = q + 1;
	size_t n = strlen(base);
	const char *dot = strrchr(base, '.');
	if (dot && dot > base) n = (size_t)(dot - base);
	if (n >= cap) n = cap - 1;
	memcpy(out, base, n);
	out[n] = 0;
}

size_t xmx_embedded_shader(const char *name)
{
	if (!name) return 0;
	char stem[128];
	kernel_name(name, stem, sizeof stem);
	for (size_t i = 0; i < sizeof kernel_names / sizeof *kernel_names; i++)
		if (!strcmp(kernel_names[i], stem)) {
#ifdef NR_EMBEDDED_METALLIB
			return sizeof nr_metallib;
#else
			return getenv("XMX_METALLIB") ? 1 : 0;
#endif
		}
	return 0;
}

static int probe_half_rounding(void);

static int load_metallib(void)
{
	NSError *error = nil;
	const char *forced = getenv("XMX_METALLIB");
	if (forced && *forced) {
		library = [dev newLibraryWithURL:[NSURL fileURLWithPath:@(forced)] error:&error];
		if (!library) FAILNS("cannot load XMX_METALLIB", error);
		return 0;
	}
#ifdef NR_EMBEDDED_METALLIB4
	if (g.metal4) {
		dispatch_data_t data4 = dispatch_data_create(nr_metallib4, sizeof nr_metallib4, NULL,
							     DISPATCH_DATA_DESTRUCTOR_DEFAULT);
		library = [dev newLibraryWithData:data4 error:&error];
		if (library) return 0;
		fprintf(stderr, "libmetalmx: the Metal 4 library did not load (%s); using Metal 3.1\n",
			error ? [[error localizedDescription] UTF8String] : "unknown error");
		g.metal4 = 0;
		error = nil;
	}
#endif
#ifdef NR_EMBEDDED_METALLIB
	dispatch_data_t data = dispatch_data_create(nr_metallib, sizeof nr_metallib, NULL,
						    DISPATCH_DATA_DESTRUCTOR_DEFAULT);
	library = [dev newLibraryWithData:data error:&error];
	if (!library) FAILNS("cannot load the embedded metallib", error);
	return 0;
#else
	FAIL("no metallib compiled into libmetalmx and XMX_METALLIB is not set", 0);
#endif
}

static int load_library(void)
{
	if (library) return 0;
	if (load_metallib()) return -1;
	if (probe_half_rounding()) { library = nil; return -1; }
	return 0;
}

/* Set from another thread to abandon a build in progress; see xmx.h. A plain flag: the
 * build reads it between pipelines and only ever cares whether it is set. */
static volatile int g_cancel;

void xmx_cancel(int on) { g_cancel = on ? 1 : 0; }
int xmx_cancelled(void) { return g_cancel; }

static int build_pipeline_consts(const char *spv_path, const unsigned *flags, const unsigned *merged, void **out)
{
	/* One check for every pipeline this library builds; see xmx.h. */
	if (g_cancel) FAIL("cancelled", 0);
	if (load_library()) return -1;
	char stem[128];
	kernel_name(spv_path, stem, sizeof stem);
	path_kernel(stem, sizeof stem);
	NSError *error = nil;
	/* A kernel that reads a function constant is created through this call even with no
	 * constant set (Metal refuses the plain lookup): an empty table leaves
	 * `is_function_constant_defined` false and the flags come from the push block. */
	MTLFunctionConstantValues *values = [MTLFunctionConstantValues new];
	unsigned value = flags ? *flags : 0;
	if (flags) [values setConstantValue:&value type:MTLDataTypeUInt atIndex:0];
	bool merged_value = merged && *merged;
	if (merged) [values setConstantValue:&merged_value type:MTLDataTypeBool atIndex:1];
	bool half_bits = g.half_bits;
	[values setConstantValue:&half_bits type:MTLDataTypeBool atIndex:2];
	bool packed = g.staged_packed != 0;
	[values setConstantValue:&packed type:MTLDataTypeBool atIndex:3];
	bool ppacked = g.portable_packed != 0;
	[values setConstantValue:&ppacked type:MTLDataTypeBool atIndex:4];
	id<MTLFunction> fn = [library newFunctionWithName:@(stem) constantValues:values error:&error];
	if (!fn) {
		snprintf(g.err, sizeof g.err, "no kernel '%s' in the metallib (from '%s')%s%s", stem, spv_path,
			 error ? ": " : "", error ? [[error localizedDescription] UTF8String] : "");
		return -1;
	}
	id<MTLComputePipelineState> state = [dev newComputePipelineStateWithFunction:fn error:&error];
	if (!state) {
		snprintf(g.err, sizeof g.err, "pipeline '%s' (flags %ld): %s", stem, flags ? (long)*flags : -1L,
			 error ? [[error localizedDescription] UTF8String] : "unknown error");
		return -1;
	}
	/* The simdgroup kernels are written for a 32-wide simdgroup, as the GLSL is for a
	 * 32-wide subgroup: their simdgroup barriers, shuffles and lane mapping assume it. The
	 * portable kernels order their threadgroup memory with threadgroup barriers only, so
	 * they run at any width -- AMD's 64 (a Vega 64 on an Intel Mac) included. */
	if (state.threadExecutionWidth > g.simd_width) g.simd_width = (unsigned)state.threadExecutionWidth;
	if (!g.portable && state.threadExecutionWidth != 32)
		FAIL("this device's simdgroup is not 32 wide, which the kernels assume", (int)state.threadExecutionWidth);
	if (g.stats) {
		/* What Metal tells of a compiled pipeline (there is no register or spill count
		 * outside Xcode's tools): the occupancy it allows and its static threadgroup
		 * memory, on the line libxmx writes for a Vulkan pipeline. */
		fprintf(g.stats, "%s\tflags=%ld\t%s\tmax_threads=%lu\twidth=%lu\tstatic_shared=%lu\n",
			stem, flags ? (long)*flags : -1L,
			g.metal4 ? "metal4" : (g.portable ? "metal3.1-portable" : "metal3.1"),
			(unsigned long)state.maxTotalThreadsPerThreadgroup,
			(unsigned long)state.threadExecutionWidth,
			(unsigned long)state.staticThreadgroupMemoryLength);
		fflush(g.stats);
		if (archive) {
			/* the compiled code itself, kept in the binary archive xmx_close writes out */
			MTLComputePipelineDescriptor *desc = [MTLComputePipelineDescriptor new];
			desc.computeFunction = fn;
			NSError *add_error = nil;
			if (![archive addComputePipelineFunctionsWithDescriptor:desc error:&add_error])
				fprintf(stderr, "libmetalmx: XMX_PIPELINE_IR: %s not archived: %s\n", stem,
					add_error ? [[add_error localizedDescription] UTF8String] : "unknown error");
		}
	}
	*out = (void *)CFBridgingRetain(state);
	return 0;
}

static int build_pipeline_spec(const char *spv_path, const unsigned *flags, void **out)
{
	return build_pipeline_consts(spv_path, flags, NULL, out);
}

static int build_pipeline(const char *path, void **out) { return build_pipeline_spec(path, NULL, out); }

static void release_pipeline(void **p)
{
	if (*p) { CFBridgingRelease(*p); *p = NULL; }
}

static unsigned block_size(const char *name, unsigned fallback)
{
	const char *value = getenv(name);
	if (!value) return fallback;
	int n = atoi(value);
	if (n > 0) return (unsigned)n;
	fprintf(stderr, "libmetalmx: %s=%s is not a positive block size; using %u\n", name, value, fallback);
	return fallback;
}

static int resident_pipeline(unsigned family, unsigned flags, void *fallback, void **out)
{
	/* families 5-7 and 10 — the fused feed-forward, the 32-row staged builds and the window
	 * block — are GEMMs and specialise with them; 8, the 256-thread row build, with the
	 * row passes (libxmx.c's numbering) */
	unsigned mask = family == 8 ? 4u
		      : (family < 3 || family >= 5) ? 1u : (family == 3 ? 2u : 4u);
	*out = fallback;
	if (!(g.specialize & mask)) return 0;
	for (unsigned i = 0; i < specialized_count; i++)
		if (specialized[i].family == family && specialized[i].flags == flags) {
			*out = specialized[i].pipeline;
			return 0;
		}
	if (specialized_count == MAX_SPECIALIZED) return 0;
	if (build_pipeline_spec(g.rpaths[family], &flags, out)) return -1;
	specialized[specialized_count].family = family;
	specialized[specialized_count].flags = flags;
	specialized[specialized_count++].pipeline = *out;
	return 0;
}

int xmx_staged_partial(unsigned on)
{
	if (g.recording) FAIL("cannot change the staged routing during recording", 0);
	g.staged_partial = on != 0;
	return 0;
}

unsigned xmx_staged32_calls(void) { return g.staged32_calls; }

int xmx_staged32(unsigned on)
{
	if (g.recording) FAIL("cannot change the staged routing during recording", 0);
	g.staged32 = on != 0;
	return 0;
}

/* The 32-row staged builds, `shallow` with the 32-deep K step and `deep` with 64. On the
 * portable path there is no staged kernel to route to: accepted, nothing built. */
int xmx_staged32_init(const char *shallow, const char *deep)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rstaged32[0] || g.portable) return 0;
	const char *paths[2] = { shallow, deep };
	for (unsigned i = 0; i < 2; i++) {
		free(g.rpaths[6 + i]);
		if (!(g.rpaths[6 + i] = strdup(paths[i]))) FAIL("pipeline path allocation", 0);
		@autoreleasepool {
			if (build_pipeline(paths[i], &g.rstaged32[i])) {
				release_pipeline(&g.rstaged32[0]);
				return -1;
			}
		}
	}
	return 0;
}

/* The row passes' 256-thread build, for the whole-row softmax. */
int xmx_rows_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rrows) return 0;
	free(g.rpaths[8]);
	if (!(g.rpaths[8] = strdup(path))) FAIL("pipeline path allocation", 0);
	@autoreleasepool {
		return build_pipeline(path, &g.rrows);
	}
}

int xmx_specialize(unsigned mask)
{
	if (g.recording) FAIL("cannot switch specialization during recording", 0);
	if (mask > 7) FAIL("specialization mask must be in 0..7", 0);
	g.specialize = mask;
	return 0;
}

unsigned xmx_specialized_count(void) { return specialized_count; }
unsigned xmx_specialization(void) { return g.specialize; }

/* -- the device ------------------------------------------------------------ */

static int want_unmapped(void)
{
	const char *forced = getenv("XMX_STAGING");
	if (forced && *forced) return atoi(forced) != 0;
	return g.discrete;
}

int xmx_adopt(void *inst, void *pd, void *device, void *q, unsigned qi, int coopmat, void *gipa,
	      void (*lock)(void *), void (*unlock)(void *), void *ctx)
{
	(void)inst; (void)pd; (void)device; (void)q; (void)qi; (void)coopmat; (void)gipa;
	(void)lock; (void)unlock; (void)ctx;
	FAIL("libmetalmx drives Metal directly: there is no Vulkan device to adopt", 0);
}

int xmx_adopted(void) { return dev ? 0 : -1; }

/* Whether to load the Metal 4 library: built in, on the simdgroup path, not refused with
 * XMX_METAL4=0 or overridden by XMX_METALLIB, and a device and OS in MTLGPUFamilyMetal4.
 * The family is named by its value, 5002, so a macOS 11 SDK still compiles this. */
static int metal4_wanted(void)
{
#ifdef NR_EMBEDDED_METALLIB4
	const char *off = getenv("XMX_METAL4");
	if (off && *off && atoi(off) == 0) return 0;
	const char *file = getenv("XMX_METALLIB");
	if ((file && *file) || g.portable) return 0;
	if (@available(macOS 26.0, iOS 26.0, *))
		return [dev supportsFamily:(MTLGPUFamily)5002] ? 1 : 0;
#endif
	return 0;
}

int xmx_open(void)
{
	if (dev) return 0;
	@autoreleasepool {
		/* XMX_METAL_DEVICE=NAME takes the first device whose name contains NAME, ignoring
		 * case -- the iGPU beside an eGPU, say, to check one against the other. */
		const char *want = getenv("XMX_METAL_DEVICE");
		if (want && *want) {
			NSArray<id<MTLDevice>> *all = MTLCopyAllDevices();
			for (id<MTLDevice> d in all)
				if ([[d name] rangeOfString:@(want) options:NSCaseInsensitiveSearch].location != NSNotFound) {
					dev = d;
					break;
				}
			if (!dev) FAIL("XMX_METAL_DEVICE: no Metal device by that name", 0);
		} else
			dev = MTLCreateSystemDefaultDevice();
		if (!dev) FAIL("no Metal device", 0);
		snprintf(g.name, sizeof g.name, "%s", [[dev name] UTF8String]);
		g.discrete = ![dev hasUnifiedMemory];
		/* simdgroup_matrix is an Apple7 (A14 / M1) feature; a Mac with another vendor's
		 * card takes the multiply-add kernels. */
		g.coopmat = [dev supportsFamily:MTLGPUFamilyApple7] ? 1 : 0;
		const char *forced = getenv("XMX_PORTABLE");
		g.portable = !g.coopmat || (forced && *forced && atoi(forced) != 0);
		g.metal4 = metal4_wanted();
		/* The recording orders its passes with memory barriers inside a concurrent encoder,
		 * as the Vulkan one does with pipeline barriers. AMD's driver (a Vega 64 on macOS 15)
		 * does not honour them for buffers reached through their GPU addresses -- neither the
		 * scoped barrier nor one naming every resource -- so consecutive passes raced there
		 * and a small frame came out different on every run. A serial encoder orders them, and
		 * cost nothing measured on that card (640x360 79.6 ms against 79.7). Apple GPUs keep
		 * the concurrent encoder, whose barriers they honour. XMX_METAL_DISPATCH=serial or
		 * concurrent decides instead. */
		g.serial = 1;
		if (@available(macOS 10.15, *)) g.serial = ![dev supportsFamily:MTLGPUFamilyApple1];
		const char *dispatch = getenv("XMX_METAL_DISPATCH");
		if (dispatch && !strcmp(dispatch, "serial")) g.serial = 1;
		else if (dispatch && !strcmp(dispatch, "concurrent")) g.serial = 0;
		else if (dispatch && *dispatch)
			fprintf(stderr, "libmetalmx: XMX_METAL_DISPATCH=%s is neither serial nor concurrent; %s for this device\n",
				dispatch, g.serial ? "serial" : "concurrent");
		queue = [dev newCommandQueue];
		if (!queue) { dev = nil; FAIL("no command queue", 0); }
		fence = [dev newFence];
		/* XMX_PIPELINE_STATS / XMX_PIPELINE_IR, as libxmx reads them (build_pipeline_consts) */
		const char *stats_path = getenv("XMX_PIPELINE_STATS");
		if (stats_path && *stats_path) {
			g.stats = fopen(stats_path, "a");
			if (!g.stats) fprintf(stderr, "libmetalmx: cannot write pipeline statistics to %s\n", stats_path);
			const char *ir = getenv("XMX_PIPELINE_IR");
			if (g.stats && ir && *ir) {
				NSError *error = nil;
				archive = [dev newBinaryArchiveWithDescriptor:[MTLBinaryArchiveDescriptor new] error:&error];
				if (!archive)
					fprintf(stderr, "libmetalmx: XMX_PIPELINE_IR: no binary archive: %s\n",
						error ? [[error localizedDescription] UTF8String] : "unknown error");
				g.ir_dir = archive ? ir : NULL;
			}
		}
	}
	return 0;
}

static void free_buf(struct buf *b)
{
	if (b->b) CFBridgingRelease(b->b);
	memset(b, 0, sizeof *b);
}

void xmx_close(void)
{
	if (!dev) return;
	for (int i = 0; i < MAX_RBUF; i++) xmx_buf_destroy(i);
	for (int i = 0; i < MAX_GRAPHS; i++) xmx_graph_destroy(i);
	op_free(&rec);
	for (unsigned i = 0; i < specialized_count; i++) release_pipeline(&specialized[i].pipeline);
	specialized_count = 0;
	void **pipes[] = { &g.rgemm, &g.rtiled, &g.rstaged, &g.runary, &g.rrow, &g.rhistory, &g.pipe, &g.pipeb,
			   &g.rwindow[0], &g.rwindow[1], &g.rffn, &g.rstaged32[0], &g.rstaged32[1],
			   &g.rrows, &g.rint8, &g.rblock, &g.rglobal, &g.rwide };
	for (size_t i = 0; i < sizeof pipes / sizeof *pipes; i++) release_pipeline(pipes[i]);
	for (int i = 0; i < 13; i++) free(g.rpaths[i]);
	struct buf *bufs[] = { &g.A, &g.B, &g.C, &g.stage };
	for (size_t i = 0; i < sizeof bufs / sizeof *bufs; i++) free_buf(bufs[i]);
	for (unsigned i = 0; i < SAMPLE_BUFFERS; i++) samples[i] = nil;
	sample_buffers = 0;
	if (archive && g.ir_dir) {
		/* every pipeline this runtime compiled, as Metal's own binary archive */
		char path[1200];
		snprintf(path, sizeof path, "%s/nr_pipelines.metallib", g.ir_dir);
		NSError *error = nil;
		if (![archive serializeToURL:[NSURL fileURLWithPath:@(path)] error:&error])
			fprintf(stderr, "libmetalmx: XMX_PIPELINE_IR: %s not written: %s\n", path,
				error ? [[error localizedDescription] UTF8String] : "unknown error");
		else if (g.stats)
			fprintf(g.stats, "\t\tbinary archive: %s\n", path);
	}
	archive = nil;
	if (g.stats) fclose(g.stats);
	library = nil;
	addr_encoder = nil;
	addr_scratch = nil;
	fence = nil;
	queue = nil;
	dev = nil;
	memset(&g, 0, sizeof g);
	memset(prof_ms, 0, sizeof prof_ms);
	memset(prof_hits, 0, sizeof prof_hits);
	prof_each_n = 0;
}

/* -- one-shot command buffers ------------------------------------------ */

/* How the compute encoders dispatch: concurrently, ordered by the recorded barriers, except
 * where those do not order anything (`g.serial`, xmx_open). */
static MTLDispatchType dispatch_type(void) { return g.serial ? MTLDispatchTypeSerial : MTLDispatchTypeConcurrent; }

/* Commit and wait. A failed command buffer is reported with Metal's own description; a
 * fault the device will not recover from on this queue is recorded as lost. */
static int finish(id<MTLCommandBuffer> cb, const char *what)
{
	[cb commit];
	[cb waitUntilCompleted];
	if (cb.status == MTLCommandBufferStatusError) {
		NSError *error = cb.error;
		if (error && (error.code == MTLCommandBufferErrorInternal || error.code == MTLCommandBufferErrorTimeout
			      || error.code == MTLCommandBufferErrorPageFault))
			g.lost = 1;
		snprintf(g.err, sizeof g.err, "%s: %s", what,
			 error ? [[error localizedDescription] UTF8String] : "command buffer failed");
		return -1;
	}
	return 0;
}

/* Whether float(half(x)) rounds on this device (nr_metal.h, `half_round`): values that must
 * round -- ties both ways, a subnormal, the overflow -- through the cast on the GPU, against
 * the float16 conversion worked out ahead (NumPy's). AMD's Metal compiler folds the round
 * trip away (a Vega 64 on macOS 15 returned 99.8 % of a million values unrounded), and there
 * every pipeline takes the bit-twiddled spelling instead, through function constant 2.
 * XMX_HALF_ROUND=cast or bits decides without asking. */
static int probe_half_rounding(void)
{
	const char *forced = getenv("XMX_HALF_ROUND");
	if (forced && (!strcmp(forced, "cast") || !strcmp(forced, "bits"))) {
		g.half_bits = forced[0] == 'b';
		return 0;
	}
	if (forced && *forced)
		fprintf(stderr, "libmetalmx: XMX_HALF_ROUND=%s is neither cast nor bits; asking the device\n", forced);
	static const float probe[8] = {
		0x1.001p+0f, 0x1.003p+0f, -0x1.002p+1f, 0x1.0c6f7ap-20f,
		0x1.8p-25f, 0x1.ffdep+15f, 0x1.117p+16f, 0x1.99999ap-4f,
	};
	static const float rounded[8] = {
		0x1p+0f, 0x1.004p+0f, -0x1p+1f, 0x1.1p-20f,
		0x1p-24f, 0x1.ffcp+15f, INFINITY, 0x1.998p-4f,
	};
	@autoreleasepool {
		NSError *error = nil;
		id<MTLFunction> fn = [library newFunctionWithName:@"half_cast_probe"];
		if (!fn) FAIL("no kernel 'half_cast_probe' in the metallib", 0);
		id<MTLComputePipelineState> state = [dev newComputePipelineStateWithFunction:fn error:&error];
		if (!state) FAILNS("pipeline 'half_cast_probe'", error);
		id<MTLBuffer> in = [dev newBufferWithBytes:probe length:sizeof probe options:MTLResourceStorageModeShared];
		id<MTLBuffer> out = [dev newBufferWithLength:sizeof probe options:MTLResourceStorageModeShared];
		id<MTLCommandBuffer> cb = [queue commandBuffer];
		if (!in || !out || !cb) FAIL("half rounding probe: no buffer", 0);
		id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
		[enc setComputePipelineState:state];
		[enc setBuffer:in offset:0 atIndex:0];
		[enc setBuffer:out offset:0 atIndex:1];
		[enc dispatchThreadgroups:MTLSizeMake(1, 1, 1) threadsPerThreadgroup:MTLSizeMake(8, 1, 1)];
		[enc endEncoding];
		if (finish(cb, "half rounding probe")) return -1;
		const float *got = [out contents];
		g.half_bits = 0;
		for (int i = 0; i < 8; i++)
			if (got[i] != rounded[i]) g.half_bits = 1;
	}
	if (g.half_bits)
		fprintf(stderr, "libmetalmx: %s does not round float(half(x)); half_round takes the bit-twiddled spelling\n", g.name);
	return 0;
}

/* The descriptor path's growable shared buffers. */
static int ensure(struct buf *b, uint64_t size)
{
	if (b->cap >= size) return 0;
	free_buf(b);
	id<MTLBuffer> buffer = [dev newBufferWithLength:(NSUInteger)size options:MTLResourceStorageModeShared];
	if (!buffer) FAIL("newBufferWithLength", 0);
	b->b = (void *)CFBridgingRetain(buffer);
	b->p = [buffer contents];
	b->cap = size;
	return 0;
}

int xmx_init(const char *spv_path)
{
	if (g.ready) return 0;
	if (xmx_open()) return -1;
	@autoreleasepool {
		if (build_pipeline(spv_path, &g.pipe)) return -1;
	}
	g.ready = 1;
	return 0;
}

int xmx_reserve(unsigned M, unsigned N, unsigned K, void **pa, void **pb, void **pc)
{
	if (!g.ready) FAIL("not initialised", 0);
	if (ensure(&g.A, (uint64_t)M * K * 2) || ensure(&g.B, (uint64_t)K * N * 2) || ensure(&g.C, (uint64_t)M * N * 4))
		return -1;
	if (pa) *pa = g.A.p;
	if (pb) *pb = g.B.p;
	if (pc) *pc = g.C.p;
	return 0;
}

int xmx_reserve_bytes(unsigned long long a, unsigned long long b, unsigned long long c,
		      void **pa, void **pb, void **pc)
{
	if (!g.ready) FAIL("not initialised", 0);
	if (ensure(&g.A, a) || ensure(&g.B, b) || ensure(&g.C, c)) return -1;
	if (pa) *pa = g.A.p;
	if (pb) *pb = g.B.p;
	if (pc) *pc = g.C.p;
	return 0;
}

/* One dispatch of a descriptor-bound kernel, `iters` times with a barrier between. */
static int run_desc(void *pipeline, const struct desc_push *push, unsigned gx, unsigned gy, unsigned gz,
		    unsigned iters)
{
	@autoreleasepool {
		id<MTLCommandBuffer> cb = [queue commandBuffer];
		if (!cb) FAIL("command buffer", 0);
		id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoderWithDispatchType:dispatch_type()];
		[enc setComputePipelineState:(__bridge id<MTLComputePipelineState>)pipeline];
		[enc setBuffer:(__bridge id<MTLBuffer>)g.A.b offset:0 atIndex:0];
		[enc setBuffer:(__bridge id<MTLBuffer>)g.B.b offset:0 atIndex:1];
		[enc setBuffer:(__bridge id<MTLBuffer>)g.C.b offset:0 atIndex:2];
		[enc setBytes:push length:sizeof *push atIndex:3];
		for (unsigned i = 0; i < (iters ? iters : 1); i++) {
			[enc dispatchThreadgroups:MTLSizeMake(gx, gy, gz) threadsPerThreadgroup:MTLSizeMake(32, 1, 1)];
			if (i + 1 < iters) [enc memoryBarrierWithScope:MTLBarrierScopeBuffers];
		}
		[enc endEncoding];
		return finish(cb, "gemm");
	}
}

int xmx_gemm(unsigned M, unsigned N, unsigned K, const void *a, const void *b, void *c, unsigned iters)
{
	if (!g.ready) FAIL("not initialised", 0);
	uint64_t sa = (uint64_t)M * K * 2, sb = (uint64_t)K * N * 2, sc = (uint64_t)M * N * 4;
	if (ensure(&g.A, sa) || ensure(&g.B, sb) || ensure(&g.C, sc)) return -1;
	if (a) memcpy(g.A.p, a, sa);
	if (b) memcpy(g.B.p, b, sb);
	struct desc_push push = { M, N, K, 0, 0, 0, 0 };
	if (run_desc(g.pipe, &push, N / 16, M / 8, 1, iters)) return -1;
	if (c) memcpy(c, g.C.p, sc);
	return 0;
}

int xmx_init_batched(const char *spv_path)
{
	if (!g.ready) FAIL("not initialised", 0);
	if (g.pipeb) return 0;
	@autoreleasepool {
		return build_pipeline(spv_path, &g.pipeb);
	}
}

int xmx_gemm_batched(unsigned M, unsigned N, unsigned K, unsigned batch,
		     unsigned sa, unsigned sb, unsigned sc, unsigned bt)
{
	if (!g.pipeb) FAIL("batched pipeline not built", 0);
	struct desc_push push = { M, N, K, sa, sb, sc, bt };
	return run_desc(g.pipeb, &push, N / 16, M / 8, batch, 1);
}

/* -- the resident runtime ------------------------------------------------- */

int xmx_res_init(const char *gemm_spv, const char *unary_spv, const char *row_spv,
		 const char *history_spv, const char *tiled_spv, const char *staged_spv)
{
	if (!g.ready) FAIL("not initialised", 0);
	if (g.rready) return 0;
	const char *paths[] = { gemm_spv, tiled_spv, staged_spv, unary_spv, row_spv };
	for (unsigned i = 0; i < 5; i++) {
		g.rpaths[i] = strdup(paths[i]);
		if (!g.rpaths[i]) FAIL("pipeline path allocation", 0);
	}
	const char *spec = getenv("XMX_SPECIALIZE");
	g.specialize = spec ? (unsigned)atoi(spec) : 7;
	if (g.specialize > 7) FAIL("XMX_SPECIALIZE must be in 0..7", 0);
	/* libxmx's packed staged loader (gemm_staged.comp, constant 2) on Metal: the staged
	 * kernel copies each operand global-to-threadgroup as one uint4 where the addresses allow
	 * it (gemm_simd.metal, function constant 3). The same halves land in the same places, so
	 * the bytes are the same; on an M3's Metal 3.1 path the 320x320 graph replays 51.4 -> 49.2
	 * ms with it (the Metal 4 library's matmul2d GEMMs stage nothing and are untouched).
	 * `XMX_STAGED_PACKED=0` keeps the half4 loader, to compare. Fixed here, before any
	 * pipeline is built. */
	g.staged_packed = 1;
	const char *packed = getenv("XMX_STAGED_PACKED");
	if (packed && *packed) {
		if (!strcmp(packed, "1")) g.staged_packed = 1;
		else if (!strcmp(packed, "0")) g.staged_packed = 0;
		else fprintf(stderr, "libmetalmx: XMX_STAGED_PACKED=%s is neither 0 nor 1; keeping %d\n",
			     packed, g.staged_packed);
	}
	/* The portable GEMM's A fetched sixteen bytes at a time (gemm_portable.metal, function
	 * constant 4) is off: on the M3 it replayed 320x320 70.5 -> 79.8 ms, the compiler
	 * combining the eight-byte fetches already. `XMX_PORTABLE_PACKED=1` to try it on another
	 * device without simdgroup matrices; the bytes are the same either way. */
	g.portable_packed = 0;
	const char *ppacked = getenv("XMX_PORTABLE_PACKED");
	if (ppacked && *ppacked) {
		if (!strcmp(ppacked, "1")) g.portable_packed = 1;
		else if (!strcmp(ppacked, "0")) g.portable_packed = 0;
		else fprintf(stderr, "libmetalmx: XMX_PORTABLE_PACKED=%s is neither 0 nor 1; keeping %d\n",
			     ppacked, g.portable_packed);
	}
	@autoreleasepool {
		if (build_pipeline(gemm_spv, &g.rgemm) || build_pipeline(unary_spv, &g.runary)
		    || build_pipeline(row_spv, &g.rrow) || build_pipeline(history_spv, &g.rhistory)
		    || build_pipeline(tiled_spv, &g.rtiled) || build_pipeline(staged_spv, &g.rstaged))
			return -1;
	}
	const char *tile = getenv("XMX_TILE_K");
	g.tiling = tile ? atoi(tile) : 1;
	g.tilem = block_size("XMX_TILE_M", 16);
	g.tilen = block_size("XMX_TILE_N", 32);
	const char *sk = getenv("XMX_STAGE_K");
	/* 128, not libxmx's 32: libxmx lowered it once its staged kernel had all its threads
	 * on Xe2, and this kernel is not that one. On the M3, paired at 1280x720 through the C
	 * library, K >= 32 is the same bytes and 1.5 % slower (399-403 against 405-406 ms),
	 * so the stem and the shallow GEMMs stay tiled here. XMX_STAGE_K=32 to compare. */
	g.staging = sk ? (unsigned)atoi(sk) : 128;
	/* As in libxmx: the staged kernel takes a partial last 64-row block itself, so the
	 * deeper levels' 144- or 400-row GEMMs need not drop to the tiled kernel. 0 is the
	 * comparison. */
	const char *sp = getenv("XMX_STAGED_PARTIAL");
	g.staged_partial = sp ? (unsigned)atoi(sp) : 1;
	/* the 32-row staged builds, once `xmx_staged32_init` has them; 0 is the comparison */
	const char *s32 = getenv("XMX_STAGED32");
	g.staged32 = s32 ? (unsigned)atoi(s32) : 1;
	/* libxmx's portable routing on MoltenVK — the 8x16 kernel for every GEMM that does not
	 * need a 32-column block, and a 16x64 build for N a multiple of 64 — is 29 % faster
	 * there, and slower here: on the M3 through native Metal, paired at 1280x768, the 8x16
	 * routing 469 -> 515 ms and the 16x64 build 469 -> 491 (a 32x32 build 477). So both are
	 * off by default and kept to measure on another Apple GPU: XMX_PORTABLE_TILED=0 routes
	 * as libxmx does, XMX_PORTABLE_WIDE=1 builds the 16x64. The sums are the same in every
	 * build. */
	const char *tiled_env = getenv("XMX_PORTABLE_TILED");
	g.portable_tiled = tiled_env ? (unsigned)atoi(tiled_env) : 1;
	const char *wide_env = getenv("XMX_PORTABLE_WIDE");
	if (g.portable && wide_env && atoi(wide_env) != 0) {
		g.rpaths[12] = strdup("gemm_portable_wide.spv");
		@autoreleasepool {
			if (!g.rpaths[12] || build_pipeline(g.rpaths[12], &g.rwide)) g.rwide = NULL;
		}
	}
	g.unmapped = want_unmapped();
	g.rready = 1;
	return 0;
}

/* The GPU address of `buffer`, the way a macOS 11 SDK allows it. `[MTLBuffer gpuAddress]`
 * needs macOS 13; an argument encoder built from one `MTLDataTypePointer` descriptor has
 * existed since Metal 2 and stores exactly that address at offset 0 of the argument buffer
 * it encodes into. The encoder and its 8-byte shared scratch buffer are made once. */
static int buffer_address(id<MTLBuffer> buffer, uint64_t *out)
{
	if (!addr_encoder) {
		MTLArgumentDescriptor *arg = [MTLArgumentDescriptor argumentDescriptor];
		arg.dataType = MTLDataTypePointer;
		arg.index = 0;                         /* access left at its default: only the
							  address is wanted, nothing is dispatched */
		addr_encoder = [dev newArgumentEncoderWithArguments:@[arg]];
		if (!addr_encoder) FAIL("newArgumentEncoderWithArguments", 0);
		addr_scratch = [dev newBufferWithLength:addr_encoder.encodedLength
						options:MTLResourceStorageModeShared];
		if (!addr_scratch) { addr_encoder = nil; FAIL("newBufferWithLength (address scratch)", 0); }
	}
	[addr_encoder setArgumentBuffer:addr_scratch offset:0];
	[addr_encoder setBuffer:buffer offset:0 atIndex:0];
	uint64_t addr;
	memcpy(&addr, [addr_scratch contents], sizeof addr);
	if (!addr) FAIL("argument encoder produced a null address", 0);
	*out = addr;
	return 0;
}

/* `kind`: 0 the graph's own buffers, 1 a buffer the host reads back, 2 one it writes.
 * Kinds 1 and 2 are always shared; kind 0 is private when the device wants it so. Hazard
 * tracking is off for the graph's buffers: the recording carries its own barriers, as the
 * Vulkan one does, and Metal's tracking would only add work. */
int xmx_buf_create_kind(unsigned long long bytes, int kind)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	int slot = -1;
	for (int i = 0; i < MAX_RBUF; i++)
		if (!rbufs[i].live) { slot = i; break; }
	if (slot < 0) FAIL("out of buffer slots", 0);
	int unmapped = kind == 0 && g.unmapped;
	MTLResourceOptions options = (unmapped ? MTLResourceStorageModePrivate : MTLResourceStorageModeShared)
				   | MTLResourceHazardTrackingModeUntracked;
	@autoreleasepool {
		id<MTLBuffer> buffer = [dev newBufferWithLength:(NSUInteger)(bytes ? bytes : 4) options:options];
		if (!buffer) FAIL("newBufferWithLength (resident)", 0);
		struct rbuf *rb = &rbufs[slot];
		rb->b = (void *)CFBridgingRetain(buffer);
		rb->p = unmapped ? NULL : [buffer contents];
		rb->mapped = !unmapped;
		rb->size = bytes ? bytes : 4;
		rb->live = 1;
		if (buffer_address(buffer, &rb->addr)) { xmx_buf_destroy(slot); return -1; }
	}
	return slot;
}

int xmx_buf_create(unsigned long long bytes) { return xmx_buf_create_kind(bytes, 0); }

int xmx_buf_host_visible(int id)
{
	return (id >= 0 && id < MAX_RBUF && rbufs[id].live && rbufs[id].mapped) ? 1 : 0;
}

static int ensure_stage(uint64_t size)
{
	if (g.stage.cap >= size) return 0;
	return ensure(&g.stage, size);
}

/* A blit of its own, now, on the queue: the transfers that happen once rather than every
 * frame — the weights, a test's readback, a clear. */
static int blit_now(void (^body)(id<MTLBlitCommandEncoder> blit), const char *what)
{
	@autoreleasepool {
		id<MTLCommandBuffer> cb = [queue commandBuffer];
		if (!cb) FAIL("command buffer (transfer)", 0);
		id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
		body(blit);
		[blit endEncoding];
		return finish(cb, what);
	}
}

static int stage_copy(int to_device, int which, const void *src, void *dst,
		      unsigned long long offset, unsigned long long bytes)
{
	if (which < 0 || which >= MAX_RBUF || !rbufs[which].live) FAIL("buffer is not live", 0);
	if (offset + bytes > rbufs[which].size) FAIL("transfer runs past the buffer", 0);
	if (!bytes) return 0;
	if (rbufs[which].mapped) {
		unsigned char *p = (unsigned char *)rbufs[which].p + offset;
		if (to_device) memcpy(p, src, (size_t)bytes);
		else memcpy(dst, p, (size_t)bytes);
		return 0;
	}
	if (ensure_stage(bytes)) return -1;
	if (to_device) memcpy(g.stage.p, src, (size_t)bytes);
	id<MTLBuffer> stage = (__bridge id<MTLBuffer>)g.stage.b, target = (__bridge id<MTLBuffer>)rbufs[which].b;
	int bad = blit_now(^(id<MTLBlitCommandEncoder> blit) {
		if (to_device) [blit copyFromBuffer:stage sourceOffset:0 toBuffer:target
				   destinationOffset:(NSUInteger)offset size:(NSUInteger)bytes];
		else [blit copyFromBuffer:target sourceOffset:(NSUInteger)offset toBuffer:stage
			     destinationOffset:0 size:(NSUInteger)bytes];
	}, "transfer");
	if (bad) return -1;
	if (!to_device) memcpy(dst, g.stage.p, (size_t)bytes);
	return 0;
}

int xmx_buf_zero(int which)
{
	if (which < 0 || which >= MAX_RBUF || !rbufs[which].live) FAIL("buffer is not live", 0);
	if (rbufs[which].mapped) {
		memset(rbufs[which].p, 0, (size_t)rbufs[which].size);
		return 0;
	}
	/* Now, never into the recording: a recorded fill would run again on every replay and
	 * wipe a weight buffer's padding after the weights had been uploaded (libxmx.c). */
	id<MTLBuffer> target = (__bridge id<MTLBuffer>)rbufs[which].b;
	NSUInteger size = (NSUInteger)rbufs[which].size;
	return blit_now(^(id<MTLBlitCommandEncoder> blit) {
		[blit fillBuffer:target range:NSMakeRange(0, size) value:0];
	}, "fill");
}

int xmx_buf_upload(int id, const void *src, unsigned long long offset, unsigned long long bytes)
{
	if (!src) FAIL("upload source is null", 0);
	return stage_copy(1, id, src, NULL, offset, bytes);
}

int xmx_buf_download(int id, void *dst, unsigned long long offset, unsigned long long bytes)
{
	if (!dst) FAIL("download destination is null", 0);
	return stage_copy(0, id, NULL, dst, offset, bytes);
}

unsigned long long xmx_buf_total_bytes(void)
{
	unsigned long long total = 0;
	for (unsigned i = 0; i < MAX_RBUF; i++) if (rbufs[i].live) total += rbufs[i].size;
	return total;
}

void *xmx_buf_ptr(int id)
{
	return (id >= 0 && id < MAX_RBUF && rbufs[id].live) ? rbufs[id].p : NULL;
}

unsigned long long xmx_buf_bytes(int id)
{
	return (id >= 0 && id < MAX_RBUF && rbufs[id].live) ? rbufs[id].size : 0;
}

int xmx_buf_destroy(int id)
{
	if (id < 0 || id >= MAX_RBUF || !rbufs[id].live) return 0;
	CFBridgingRelease(rbufs[id].b);
	rbufs[id] = (struct rbuf){ 0 };
	return 0;
}

static uint64_t addr_of(int id)
{
	return (id >= 0 && id < MAX_RBUF && rbufs[id].live) ? rbufs[id].addr : 0;
}

/* -- recording -------------------------------------------------------------- */

int xmx_begin(void)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.recording) FAIL("already recording", 0);
	rec.n = 0;
	rec.passes = 0;
	g.recording = 1;
	g.recorded = 0;
	g.syncing = 1;
	return 0;
}

int xmx_abort(void)
{
	if (!g.recording) return 0;
	rec.n = 0;
	g.recording = 0;
	return 0;
}

/* One global barrier between passes, suppressed by `xmx_sync(0)` over a run of dispatches
 * known to be independent. */
static int barrier(void)
{
	if (!g.syncing) return 0;
	struct op op = { .kind = OP_BARRIER };
	if (op_push(&rec, &op)) FAIL("out of memory recording", 0);
	return 0;
}

int xmx_sync(int on)
{
	if (!g.recording) FAIL("not recording", 0);
	int was = g.syncing;
	g.syncing = on;
	if (on && !was) return barrier();
	return 0;
}

int xmx_rec_copy(int source, int target, unsigned long long bytes,
		 unsigned long long source_offset, unsigned long long target_offset)
{
	if (!g.recording) FAIL("not recording", 0);
	if (!addr_of(source) || !addr_of(target)) FAIL("copy buffer is not live", 0);
	struct rbuf *a = &rbufs[source], *b = &rbufs[target];
	if (!bytes || ((bytes | source_offset | target_offset) & 3) ||
	    source_offset > a->size || bytes > a->size - source_offset ||
	    target_offset > b->size || bytes > b->size - target_offset)
		FAIL("copy range must be aligned and within both buffers", 0);
	if (source == target && source_offset < target_offset + bytes &&
	    target_offset < source_offset + bytes) FAIL("overlapping copy", 0);
	struct op op = { .kind = OP_COPY, .stamp = PK_COPY * 32, .src = source, .dst = target,
			 .so = source_offset, .to = target_offset, .bytes = bytes };
	if (op_push(&rec, &op)) FAIL("out of memory recording", 0);
	g.recorded++;
	return 0;
}

/* How many 32-lane blocks a portable GEMM's threadgroup stacks (gemm_portable.metal): enough
 * to fill the device's SIMD group, two at most (measured), XMX_PORTABLE_PACK=N to try others.
 * One on the simdgroup path, whose kernels are another shape. */
static unsigned portable_pack(void)
{
	if (!g.portable) return 1;
	const char *env = getenv("XMX_PORTABLE_PACK");
	if (env && *env) {
		int n = atoi(env);
		return n >= 1 && n <= 32 ? (unsigned)n : 1;
	}
	return g.simd_width >= 64 ? 2 : 1;
}

static int dispatch(void *pipeline, const struct push *p, unsigned gx, unsigned gy, unsigned gz,
		    unsigned tg, unsigned family, unsigned subkind)
{
	struct op op = { .kind = OP_DISPATCH, .stamp = (unsigned char)(family * 32u + (subkind & 31u)),
			 .pipeline = pipeline, .p = *p, .gx = gx, .gy = gy, .gz = gz, .tg = tg };
	if (op_push(&rec, &op)) FAIL("out of memory recording", 0);
	if (barrier()) return -1;
	g.recorded++;
	return 0;
}

/* The QKV epilogue's other operands: K and V targets, the query scale, and the layout the
 * three targets share, (window, head, token, 32). */
struct qkv_targets { int k, v, scale; unsigned tokens, heads; };

/* One GEMM, with every fused epilogue libxmx.c's `record_gemm` knows and the same routing
 * and refusals, so a graph recorded against either runtime is the same graph. */
static int record_gemm(int a, int b, int c, unsigned M, unsigned N, unsigned K,
		       unsigned batch, unsigned sa, unsigned sb, unsigned sc, unsigned bt,
		       unsigned lda, unsigned ldb, unsigned ldc,
		       unsigned oa, unsigned ob, unsigned oc,
		       int skip, int cosine, const uint32_t *window,
		       const struct qkv_targets *qkv, int half_copy, const uint32_t *window_a,
		       int pooled)
{
	if (!g.recording) FAIL("not recording", 0);
	struct push p = { .a = addr_of(a), .b = addr_of(b), .c = addr_of(c),
			  .m = M, .n = N, .k = K, .batch = batch,
			  .sa = sa, .sb = sb, .sc = sc, .flags = bt,
			  .lda = lda, .ldb = ldb, .ldc = ldc };
	if (!p.a || !p.b || !p.c) FAIL("gemm operand is not a live buffer", 0);
	if (skip >= 0) {
		p.d = addr_of(skip); p.residual_cos = addr_of(cosine);
		if (!p.d || !p.residual_cos) FAIL("residual operand is not a live buffer", 0);
	}
	if (window) {
		p.image_h = window[0]; p.image_w = window[1];
		p.window_cols = window[2]; p.window_pad = window[3];
	}
	if (half_copy >= 0 && !(p.d = addr_of(half_copy)))
		FAIL("GEMM half copy is not a live buffer", 0);
	if (window_a) {
		/* A gathered from the image in window order: the batch strides have no use with
		 * one batch, so they carry the geometry */
		p.sa = window_a[0]; p.sb = window_a[1]; p.sc = window_a[2];
		p.window_pad = window_a[3];
	}
	/* Bit 0x800000: block 0's window residual pooled and published in its own epilogue —
	 * `c` takes the published skip as half, the pool rides in the QKV epilogue's slot. */
	if (bt & 0x800000u) {
		if (!window || N != 32u || (bt & ~0x8e0000u) || pooled < 0
		    || window[0] % 2u || window[1] % 2u || (window[3] >> 16) % 2u
		    || (window[3] & 0xffffu) % 2u)
			FAIL("a pooled window residual needs even geometry, 32 channels, no publish", 0);
		if (!(p.qkv_scale = addr_of(pooled)))
			FAIL("pooled output is not a live buffer", 0);
	}
	if (qkv) {
		/* c is Q; K, V and the scale ride in slots only the residual modes use */
		p.d = addr_of(qkv->k); p.residual_cos = addr_of(qkv->v);
		p.qkv_scale = addr_of(qkv->scale);
		p.image_h = qkv->tokens; p.image_w = qkv->heads;
		if (!p.d || !p.residual_cos || !p.qkv_scale)
			FAIL("QKV epilogue operand is not a live buffer", 0);
	}
	p.a += (uint64_t)oa * 2; p.b += (uint64_t)ob * 2;
	p.c += (uint64_t)oc * ((bt & 0x1000u) ? 2 : 4);
	/* The same three-way choice as libxmx: the 64x32 staged kernel for deep, whole-block
	 * shapes; the 16x32 register block where both extents allow it; the 8x16 kernel for
	 * everything else. The staged kernel has no portable twin. */
	/* A last, partial 64-row block is the staged kernel's own business (its rows past M are
	 * neither read out of bounds nor stored), the QKV epilogue's included since its store
	 * skips them too. */
	/* The 32-row builds, routed as libxmx.c routes them: a bottleneck of 32 tokens or
	 * fewer, and the 64-token one's N <= 1024, K >= 1024 GEMMs as two 32-row blocks with
	 * the 64-deep step. Bit-identical — a row's sums are its own. */
	int small = g.staged32 && g.rstaged32[0] && M <= 32 && !(bt & 0x400000u)
		    && (M == 32 || g.staged_partial);
	if (g.staged32 && g.rstaged32[1] && M == 64 && N <= 1024 && K % 64 == 0 && K >= 1024
	    && !(bt & 0x500000u))
		small = 1;
	/* The 32-row builds keep libxmx's K >= 32 rather than this runtime's 128: the
	 * threshold is a measurement of the 64-row kernel on the deeper levels' shapes, and a
	 * GEMM of 32 rows or fewer is the K-loop-bound case the small builds exist for. */
	int staged = (M % 64 == 0 || small || g.staged_partial)
		     && N % 32 == 0 && K % 32 == 0 && (K >= g.staging || (small && K >= 32));
	if (g.portable) staged = 0;
	/* the window gather lives in the staged kernel's A loader on the matrix path, and in
	 * the tiled kernels' (both builds) where there is no staged one */
	int window_tiled = 0;
	if (bt & 0x400000u) {
		if (!window_a || M % 64 || N % 32 || K % 32 || batch > 1 || (bt & 1u))
			FAIL("a window-gathered A needs its geometry, whole blocks and one batch", 0);
		if (!g.portable) staged = 1;
		else window_tiled = 1;
	}
	int tiled = M % g.tilem == 0 && N % g.tilen == 0 && K >= g.tiling;
	if (g.portable && !g.portable_tiled && !(bt & 0x900000u)) tiled = 0;
	if (window_tiled) {
		if (M % g.tilem || N % g.tilen)
			FAIL("a window-gathered A without the staged kernel needs the 16x32 block", 0);
		tiled = 1;
	}
	if ((bt & 0x10000u) && (bt & 0x20000u))
		FAIL("compact head and fused residual are exclusive", 0);
	if (bt & 0x10000u) {
		if (N != 16 || ldc != 4 || (bt & 0x1f00u))
			FAIL("compact head requires N=16, ldc=4 and plain FP32 output", 0);
		staged = tiled = 0;
	}
	/* 0x2000 is the ViT's normalisation, 0x4000 a window-gathered A published on the way in */
	if ((bt & 0x100000u) && !(qkv && (bt & ~0x40e000u) == 0x100000u
				  && (staged || (tiled && g.tilen == 32))))
		FAIL("QKV epilogue needs its targets, no other flag, and a 32-column block", 0);
	if ((bt & 0x4000u) && !(bt & 0x400000u))
		FAIL("an A published on the way in is a window-gathered one", 0);
	if ((bt & 0x200000u) && !(half_copy >= 0 && bt == 0x200000u && !staged))
		FAIL("a GEMM half copy needs its target, no other flag, and the resident kernel", 0);
	/* The pool is the 64-row staged kernel's epilogue on the simdgroup path — at any depth
	 * of K, as the window gather is, since this runtime's staging threshold would keep the
	 * block's K = 32 off it — and the tiled portable kernel's without it. */
	if (bt & 0x800000u) {
		if (!g.portable) {
			if (M % 64 || K % 32) FAIL("a pooled window residual needs whole windows", 0);
			staged = 1;
			small = 0;
		} else if (!tiled || g.tilem != 16u || g.tilen != 32u) {
			FAIL("a pooled window residual needs the staged kernel, or the portable 16x32 "
			     "block (xmx_window_gather() is 0)", 0);
		}
	}
	small = small && staged;
	int deep = small && N <= 1024 && K % 64 == 0;
	/* the portable 16x64 build, for what its generic store takes: not the QKV epilogue or
	 * the pool (both need a 32-column block), the window gather, nor the compact head */
	int wide = g.rwide && !staged && M % 16u == 0 && N % 64u == 0 && K >= g.tiling
		   && !(bt & 0xd10000u);
	void *pipeline;
	if (small) {
		if (resident_pipeline(6 + deep, bt, g.rstaged32[deep], &pipeline)) return -1;
	} else if (wide) {
		if (resident_pipeline(12, bt, g.rwide, &pipeline)) return -1;
	} else if (resident_pipeline(staged ? 2 : (tiled ? 1 : 0), bt,
				     staged ? g.rstaged : (tiled ? g.rtiled : g.rgemm), &pipeline)) {
		return -1;
	}
	unsigned gz = batch ? batch : 1;
	g.staged32_calls += small;
	if (staged) {
		unsigned rows = small ? 32u : 64u;
		return dispatch(pipeline, &p, N / 32, (M + rows - 1) / rows, gz, 128, PK_STAGED, bt);
	}
	g.wide_calls += wide;
	/* The portable kernels are a 32-lane block a threadgroup; where the SIMD group is wider
	 * (AMD's 64), blocks are stacked along M to fill it (gemm_portable.metal). Two a
	 * threadgroup on a Vega 64: 1.3-2.4x on the frame's shapes; four was no faster. The two
	 * epilogues that keep threadgroup memory, the QKV one and the pool, have a stage a block
	 * and barriers both blocks must meet, so they are packed only where every block is real. */
	unsigned pack = portable_pack();
	unsigned gx, gy, kind = PK_TILED;
	if (wide)       gx = N / 64u, gy = M / 16u;
	else if (tiled) gx = N / g.tilen, gy = M / g.tilem;
	else            gx = (N + 15) / 16, gy = (M + 7) / 8, kind = PK_GEMM;
	if ((bt & 0x900000u) && (pack > 2u || gy % pack)) pack = 1;
	return dispatch(pipeline, &p, gx, (gy + pack - 1) / pack, gz, 32 * pack, kind, bt);
}

int xmx_rec_gemm(int a, int b, int c, unsigned M, unsigned N, unsigned K, unsigned batch,
		 unsigned sa, unsigned sb, unsigned sc, unsigned bt,
		 unsigned lda, unsigned ldb, unsigned ldc,
		 unsigned oa, unsigned ob, unsigned oc)
{
	return record_gemm(a, b, c, M, N, K, batch, sa, sb, sc, bt,
			   lda, ldb, ldc, oa, ob, oc, -1, -1, NULL, NULL, -1, NULL, -1);
}

int xmx_rec_gemm_residual(int a, int b, int c, int skip, int cosine,
			  unsigned M, unsigned N, unsigned K, unsigned flags)
{
	if (skip < 0 || cosine < 0) FAIL("invalid residual buffer", 0);
	return record_gemm(a, b, c, M, N, K, 1, M * K, K * N, M * N,
			   flags | 0x20000u, 0, 0, 0, 0, 0, 0, skip, cosine, NULL, NULL, -1, NULL, -1);
}

int xmx_rec_gemm_window_residual(int a, int b, int c, int skip, int cosine,
				 unsigned M, unsigned N, unsigned K, unsigned flags,
				 unsigned height, unsigned width, unsigned across, unsigned pad)
{
	if (skip < 0 || cosine < 0 || !height || !width || !across)
		FAIL("invalid window residual", 0);
	uint32_t window[] = { height, width, across, pad };
	return record_gemm(a, b, c, M, N, K, 1, M * K, K * N, M * N,
			   flags | 0xa0000u, 0, 0, 0, 0, 0, 0, skip, cosine, window, NULL, -1, NULL, -1);
}

/* Block 0's window residual with both of its readers served from the epilogue
 * (0x800000): `skip_out` takes every value published as half, `pooled` the published 2x2
 * pool, and the float32 output is never stored. */
int xmx_rec_gemm_window_residual_pool(int a, int b, int skip_out, int skip, int cosine,
				      int pooled, unsigned M, unsigned N, unsigned K,
				      unsigned flags, unsigned height, unsigned width,
				      unsigned across, unsigned pad)
{
	if (skip < 0 || cosine < 0 || !height || !width || !across)
		FAIL("invalid window residual", 0);
	uint32_t window[] = { height, width, across, pad };
	return record_gemm(a, b, skip_out, M, N, K, 1, M * K, K * N, M * N,
			   flags | 0x8a0000u, 0, 0, 0, 0, 0, 0, skip, cosine, window, NULL, -1,
			   NULL, pooled);
}

static int record_gemm_qkv(int a, int weight, int q, int k, int v, int scale,
			   unsigned M, unsigned channels, unsigned heads, unsigned tokens,
			   const uint32_t *window_a, unsigned flags)
{
	if (q < 0 || k < 0 || v < 0 || scale < 0 || !heads || !tokens || channels != heads * 32u
	    || M % tokens)
		FAIL("invalid QKV projection", 0);
	struct qkv_targets targets = { k, v, scale, tokens, heads };
	unsigned N = 3u * channels;
	return record_gemm(a, weight, q, M, N, channels, 1, M * channels, channels * N, M * N,
			   0x100000u | flags, 0, 0, 0, 0, 0, 0, -1, -1, NULL, &targets, -1, window_a, -1);
}

/* `vit`: the ViT's normalisation and query scale (nr_epilogue.h, flag 0x2000). */
int xmx_rec_gemm_qkv(int a, int weight, int q, int k, int v, int scale,
		     unsigned M, unsigned channels, unsigned heads, unsigned tokens, unsigned vit)
{
	return record_gemm_qkv(a, weight, q, k, v, scale, M, channels, heads, tokens, NULL,
			       vit ? 0x2000u : 0u);
}

/* `image_mode`: bit 0 a half image (0x8000), bit 1 the image published to E4M3 as it is
 * loaded (0x4000). */
int xmx_rec_gemm_qkv_window(int image, int weight, int q, int k, int v, int scale,
			    unsigned M, unsigned channels, unsigned heads, unsigned tokens,
			    unsigned width, unsigned height, unsigned across, unsigned pad,
			    unsigned image_mode)
{
	if (!width || !height || !across || tokens != 64u || image_mode > 3u)
		FAIL("invalid window-gathered QKV projection", 0);
	uint32_t window_a[] = { width, height, across, pad };
	return record_gemm_qkv(image, weight, q, k, v, scale, M, channels, heads, tokens,
			       window_a, 0x400000u | ((image_mode & 1u) ? 0x8000u : 0u)
			       | ((image_mode & 2u) ? 0x4000u : 0u));
}

int xmx_rec_gemm_dual(int a, int b, int c, int half_copy, unsigned M, unsigned N, unsigned K)
{
	if (half_copy < 0) FAIL("invalid GEMM half copy", 0);
	return record_gemm(a, b, c, M, N, K, 1, M * K, K * N, M * N,
			   0x200000u, 0, 0, 0, 0, 0, 0, -1, -1, NULL, NULL, half_copy, NULL, -1);
}

static int record_unary(unsigned kind, int a, int b, int c, int d, int second,
			unsigned n, unsigned channels, float p0, unsigned batch,
			unsigned sa, unsigned sb, unsigned sc, unsigned k)
{
	if (!g.recording) FAIL("not recording", 0);
	struct push p = { .a = addr_of(a), .b = addr_of(b), .c = addr_of(c), .d = addr_of(d),
			  .m = n, .n = channels, .flags = kind, .p0 = p0,
			  .batch = batch, .sa = sa, .sb = sb, .sc = sc, .k = k };
	if (!p.a || !p.c) FAIL("unary operand is not a live buffer", 0);
	/* a pass that writes two outputs finds the second at offset 96 */
	if (second >= 0 && !(p.residual_cos = addr_of(second)))
		FAIL("second unary output is not a live buffer", 0);
	void *pipeline;
	if (resident_pipeline(3, kind, g.runary, &pipeline)) return -1;
	return dispatch(pipeline, &p, (n + 255) / 256, 1, 1, 256, PK_UNARY, kind);
}

int xmx_rec_unary(unsigned kind, int a, int b, int c, int d, unsigned n, unsigned channels,
		  float p0, unsigned batch, unsigned sa, unsigned sb, unsigned sc, unsigned k)
{
	return record_unary(kind, a, b, c, d, -1, n, channels, p0, batch, sa, sb, sc, k);
}

int xmx_rec_unary2(unsigned kind, int a, int b, int c, int d, int second, unsigned n,
		   unsigned channels, float p0, unsigned batch, unsigned sa, unsigned sb,
		   unsigned sc, unsigned k)
{
	if (second < 0) FAIL("a two-output pass needs its second output", 0);
	return record_unary(kind, a, b, c, d, second, n, channels, p0, batch, sa, sb, sc, k);
}

/* Q, K and V prepared in one row dispatch, three planes on y (attention.metal,
 * QKV_PREPARE); the scale's address rides in lda | ldb << 32. */
int xmx_rec_qkv(int source, int q, int k, int v, int scale,
		unsigned rows, unsigned tokens, unsigned heads, unsigned vit)
{
	if (!g.recording) FAIL("not recording", 0);
	if (!rows || !tokens || !heads) FAIL("invalid QKV extent", 0);
	/* 0x80000: the ViT's normalisation and query scale (attention.metal) */
	const unsigned flags = 2u | 0x1000u | 0x20000u | (vit ? 0x80000u : 0u);
	uint64_t scale_addr = addr_of(scale);
	struct push p = { .a = addr_of(source), .b = addr_of(q), .c = addr_of(k),
		.d = addr_of(v), .m = rows, .n = tokens, .batch = heads, .flags = flags,
		.lda = (uint32_t)scale_addr, .ldb = (uint32_t)(scale_addr >> 32) };
	if (!p.a || !p.b || !p.c || !p.d || !scale_addr) FAIL("QKV operand is not live", 0);
	void *pipeline;
	if (resident_pipeline(4, flags, g.rrow, &pipeline)) return -1;
	return dispatch(pipeline, &p, (rows + 31) / 32, 3, 1, 32, PK_ROW, 2);
}

/* The fused passes run 256 threads a threadgroup; a pipeline whose registers leave it
 * fewer says so here rather than failing at dispatch. */
static int fused_fits(const char *what, void *pipeline)
{
	NSUInteger most = ((__bridge id<MTLComputePipelineState>)pipeline).maxTotalThreadsPerThreadgroup;
	if (most >= 256) return 0;
	snprintf(g.err, sizeof g.err, "%s needs 256 threads a threadgroup; the pipeline allows %lu",
		 what, (unsigned long)most);
	return -1;
}

/* Window attention's QK^T, softmax and PV in one dispatch (window_attention.metal); the
 * portable twin on the portable path. `merged` is function constant 1: the head merge's
 * work as well. Built on first use. */
int xmx_window_init(const char *path, unsigned merged)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (merged > 1) FAIL("invalid window attention output mode", 0);
	if (g.rwindow[merged]) return 0;
	@autoreleasepool {
		return build_pipeline_consts(path, NULL, &merged, &g.rwindow[merged]);
	}
}

int xmx_rec_window_attention(int q, int k, int v, int bias, int out,
			     unsigned batches, unsigned heads, unsigned merged)
{
	if (merged > 1 || !g.recording || !g.rwindow[merged])
		FAIL("window attention not ready for recording", 0);
	if (!batches || !heads || batches % heads) FAIL("invalid attention batch/head count", 0);
	struct push p = { .a = addr_of(q), .b = addr_of(k), .c = addr_of(out), .d = addr_of(v),
			  .n = heads, .batch = batches, .flags = bias >= 0,
			  .residual_cos = bias >= 0 ? addr_of(bias) : 0 };
	if (!p.a || !p.b || !p.c || !p.d || (bias >= 0 && !p.residual_cos))
		FAIL("window attention operand is not a live buffer", 0);
	if (fused_fits("window attention", g.rwindow[merged])) return -1;
	/* one 256-thread threadgroup a window and head: its eight simdgroups share K and V */
	return dispatch(g.rwindow[merged], &p, 1, batches < 65535u ? batches : 65535u,
			1u + (batches - 1u) / 65535u, 256, PK_ROW, 3);
}

/* A feed-forward in one dispatch, the hidden layer on chip (ffn_fused.metal). */
int xmx_ffn_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rffn) return 0;
	free(g.rpaths[5]);
	if (!(g.rpaths[5] = strdup(path))) FAIL("pipeline path allocation", 0);
	@autoreleasepool {
		return build_pipeline(path, &g.rffn);
	}
}

int xmx_rec_ffn(int a, int expand, int projection, int out, int skip, int cosine,
		unsigned M, unsigned cin, unsigned hidden, unsigned groups, unsigned flags)
{
	if (!g.recording || !g.rffn) FAIL("fused feed-forward not ready for recording", 0);
	if (!cin || cin % 16u || !hidden || hidden % 32u || !M || M % 16u || !groups)
		FAIL("fused feed-forward needs channels a multiple of 16, hidden of 32, 16-row blocks", 0);
	if (flags & ~0x41f00u)
		FAIL("fused feed-forward takes an epilogue, a half output and a half skip only", 0);
	int residual = skip >= 0 || cosine >= 0;
	if (residual && (groups != 1u || cin != 32u || skip < 0 || cosine < 0))
		FAIL("the fused residual is for one group of 32 channels, with skip and cosine", 0);
	if (!residual && (flags & 0x40000u)) FAIL("a half skip without a residual", 0);
	/* the narrow blocks' shape: both weight matrices fit the threadgroup's memory */
	int staged = cin == 32u && hidden == 128u;
	struct push p = { .a = addr_of(a), .b = addr_of(expand), .c = addr_of(out),
			  .m = M, .n = cin, .k = hidden, .batch = groups,
			  .sa = cin * hidden, .sb = hidden * 32u,
			  .ldc = groups > 1u ? groups * 32u : 0u,
			  .flags = flags | (residual ? 0x20000u : 0u) | (staged ? 0x800000u : 0u),
			  .qkv_scale = addr_of(projection) };
	if (residual) { p.d = addr_of(skip); p.residual_cos = addr_of(cosine); }
	if (!p.a || !p.b || !p.c || !p.qkv_scale || (residual && (!p.d || !p.residual_cos)))
		FAIL("fused feed-forward operand is not a live buffer", 0);
	void *pipeline;
	if (resident_pipeline(5, p.flags, g.rffn, &pipeline)) return -1;
	if (fused_fits("the fused feed-forward", pipeline)) return -1;
	/* eight 16-row blocks a 256-thread threadgroup; the last one's surplus blocks idle */
	return dispatch(pipeline, &p, (M + 127u) / 128u, groups, 1, 256, PK_GEMM, 31);
}

/* Block 70's feed-forward with its input made in the same pass (ffn_fused.metal, flag
 * 0x1000000): `source` the level above at half the extent (half), `skip` the
 * full-resolution skip (half), `sincos` the merge's sin then cos (the 64-bit address in
 * p0-p1), `cosine` the feed-forward residual's — what UPSAMPLE_MERGE followed by
 * xmx_rec_ffn with a float32 skip writes, the merge never stored. */
int xmx_rec_ffn_merge(int source, int skip, int sincos, int expand, int projection, int out,
		      int cosine, unsigned height, unsigned width, unsigned source_width,
		      unsigned flags)
{
	if (!g.recording || !g.rffn) FAIL("fused feed-forward not ready for recording", 0);
	unsigned M = height * width;
	if (!M || M % 16u || height % 2u || width % 2u || source_width < width / 2u)
		FAIL("the merged feed-forward needs an even extent in 16-row blocks", 0);
	if (flags & ~0x1f00u)
		FAIL("the merged feed-forward takes an epilogue and a half output only", 0);
	struct push p = { .a = addr_of(source), .b = addr_of(expand), .c = addr_of(out),
			  .d = addr_of(skip), .m = M, .n = 32u, .k = 128u, .batch = 1u,
			  .sa = 32u * 128u, .sb = 128u * 32u, .lda = width, .ldb = source_width,
			  .flags = flags | 0x20000u | 0x800000u | 0x1000000u,
			  .residual_cos = addr_of(cosine), .qkv_scale = addr_of(projection) };
	uint64_t table = addr_of(sincos);
	memcpy(&p.p0, &table, sizeof table);
	if (!p.a || !p.b || !p.c || !p.d || !table || !p.residual_cos || !p.qkv_scale)
		FAIL("merged feed-forward operand is not a live buffer", 0);
	void *pipeline;
	if (resident_pipeline(5, p.flags, g.rffn, &pipeline)) return -1;
	if (fused_fits("the fused feed-forward", pipeline)) return -1;
	return dispatch(pipeline, &p, (M + 127u) / 128u, 1, 1, 256, PK_GEMM, 31);
}

/* Block 0's feed-forward with its stem made in the same pass (ffn_fused.metal, flag
 * 0x2000000): `features` (half, rows x 16) times `adapter` (16 x 32, the address in
 * p0-p1), which xmx_rec_gemm_dual wrote as float32 and half for this pass to read back. */
int xmx_rec_ffn_stem(int features, int adapter, int expand, int projection, int out,
		     int cosine, unsigned M, unsigned flags)
{
	if (!g.recording || !g.rffn) FAIL("fused feed-forward not ready for recording", 0);
	if (!M || M % 16u) FAIL("the stem feed-forward needs 16-row blocks", 0);
	if (flags & ~0x1f00u)
		FAIL("the stem feed-forward takes an epilogue and a half output only", 0);
	struct push p = { .a = addr_of(features), .b = addr_of(expand), .c = addr_of(out),
			  .m = M, .n = 32u, .k = 128u, .batch = 1u,
			  .sa = 32u * 128u, .sb = 128u * 32u,
			  .flags = flags | 0x20000u | 0x800000u | 0x2000000u,
			  .residual_cos = addr_of(cosine), .qkv_scale = addr_of(projection) };
	uint64_t weights = addr_of(adapter);
	memcpy(&p.p0, &weights, sizeof weights);
	if (!p.a || !p.b || !p.c || !weights || !p.residual_cos || !p.qkv_scale)
		FAIL("stem feed-forward operand is not a live buffer", 0);
	void *pipeline;
	if (resident_pipeline(5, p.flags, g.rffn, &pipeline)) return -1;
	if (fused_fits("the fused feed-forward", pipeline)) return -1;
	return dispatch(pipeline, &p, (M + 127u) / 128u, 1, 1, 256, PK_GEMM, 31);
}

/* A 32-channel window block's attention half in one pass a window (window_block.metal):
 * eight simdgroups (slices), a window row each; the portable twin on the portable path.
 * Built on first use; its whole-block variants are specialisations of one pipeline. */
int xmx_window_block_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rblock) return 0;
	free(g.rpaths[10]);
	if (!(g.rpaths[10] = strdup(path))) FAIL("pipeline path allocation", 0);
	@autoreleasepool {
		if (build_pipeline(path, &g.rblock)) return -1;
	}
	return fused_fits("the window block", g.rblock);
}

int xmx_rec_window_block(int image, int qkv, int projection, int target, int bias,
			 int cosine, int scale, int pooled, unsigned windows, unsigned height,
			 unsigned width, unsigned across, unsigned pad, unsigned flags)
{
	if (!g.recording || !g.rblock) FAIL("window block not ready for recording", 0);
	if (!windows || !height || !width || !across || windows % across)
		FAIL("invalid window block geometry", 0);
	if (flags & ~0x3809f00u)
		FAIL("the window block takes a publish, a half target, a half image, a pool or a head", 0);
	if ((flags & 0x2000000u) && !(flags & 0x1000000u))
		FAIL("all sixteen head columns without the head", 0);
	if ((flags & 0x1000000u) && (pooled < 0 || (flags & 0x801f00u)))
		FAIL("a window block with the head needs its weights, no publish and no pool", 0);
	if ((flags & 0x800000u) && (pooled < 0 || (flags & 0x1f00u) || height % 2u || width % 2u
				    || (pad >> 16) % 2u || (pad & 0xffffu) % 2u))
		FAIL("a pooled window block needs its pool, no publish, even extents and pads", 0);
	struct push p = { .a = addr_of(image), .b = addr_of(qkv), .c = addr_of(target),
			  .d = addr_of(bias), .m = windows, .n = 32u, .k = 32u, .batch = 1u,
			  .flags = flags, .residual_cos = addr_of(cosine), .image_h = height,
			  .image_w = width, .window_cols = across, .window_pad = pad,
			  .qkv_scale = addr_of(scale) };
	/* p0-p1 and p2-p3 carry 64-bit addresses: the output projection and the pool (or the
	 * head's weights) */
	uint64_t weights = addr_of(projection), pool = pooled >= 0 ? addr_of(pooled) : 0;
	memcpy(&p.p0, &weights, sizeof weights);
	memcpy(&p.p2, &pool, sizeof pool);
	if (!p.a || !p.b || !p.c || !p.d || !weights || !p.residual_cos || !p.qkv_scale
	    || ((flags & 0x1800000u) && !pool))
		FAIL("window block operand is not a live buffer", 0);
	void *pipeline;
	if (resident_pipeline(10, flags, g.rblock, &pipeline)) return -1;
	if (fused_fits("the window block", pipeline)) return -1;
	return dispatch(pipeline, &p, windows < 65535u ? windows : 65535u, (windows + 65534u) / 65535u,
			1, 256, PK_ROW, 4);
}

int xmx_global_attention_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rglobal) return 0;
	free(g.rpaths[11]);
	if (!(g.rpaths[11] = strdup(path))) FAIL("pipeline path allocation", 0);
	@autoreleasepool {
		if (build_pipeline(path, &g.rglobal)) return -1;
	}
	return fused_fits("global attention", g.rglobal);
}

/* A bottleneck block's attention in one pass (global_attention.metal): Q, K and V
 * (heads, rows, 32) half into the merged (rows, heads * 32) half — QK^T, the ViT's
 * softmax, PV and the scaled head merge. A 256-thread threadgroup a head and 64 query
 * rows; the rows reach no further than the tokens' last 64-block. */
int xmx_rec_global_attention(int q, int k, int v, int merged, unsigned rows,
			     unsigned tokens, unsigned heads)
{
	if (!g.recording || !g.rglobal) FAIL("global attention not ready for recording", 0);
	if (!rows || rows % 16u || !tokens || tokens > rows || !heads
	    || (rows + 63u) / 64u != (tokens + 63u) / 64u)
		FAIL("global attention needs rows a multiple of 16 within the tokens' 64-block", 0);
	struct push p = { .a = addr_of(q), .b = addr_of(k), .c = addr_of(merged), .d = addr_of(v),
			  .m = rows, .n = tokens, .batch = heads };
	if (!p.a || !p.b || !p.c || !p.d) FAIL("global attention operand is not a live buffer", 0);
	return dispatch(g.rglobal, &p, (rows + 63u) / 64u, heads, 1, 256, PK_ROW, 5);
}

int xmx_int8_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rint8) return 0;
	free(g.rpaths[9]);
	if (!(g.rpaths[9] = strdup(path))) FAIL("pipeline path allocation", 0);
	@autoreleasepool {
		return build_pipeline(path, &g.rint8);
	}
}

/* C = (A @ B^T) * a_scale[row] * b_scale[column]: A int8 M x K, B the weights stored
 * transposed, int8 N x K, the scales float32 (gemm_int8.metal, an 8x16 block a
 * 32-thread threadgroup: Apple's simdgroup matrices have no integer type). */
int xmx_rec_gemm_int8(int a, int b, int c, int a_scale, int b_scale,
		      unsigned M, unsigned N, unsigned K, unsigned flags)
{
	if (!g.recording || !g.rint8) FAIL("integer GEMM not ready for recording", 0);
	if (!M || !N || N % 32u || !K || K % 64u)
		FAIL("the integer GEMM needs N a multiple of 32 and K of 64", 0);
	if (flags) FAIL("the integer GEMM takes no flags yet", 0);
	struct push p = { .a = addr_of(a), .b = addr_of(b), .c = addr_of(c), .d = addr_of(a_scale),
			  .m = M, .n = N, .k = K, .batch = 1u, .flags = flags,
			  .residual_cos = addr_of(b_scale) };
	if (!p.a || !p.b || !p.c || !p.d || !p.residual_cos)
		FAIL("integer GEMM operand is not a live buffer", 0);
	return dispatch(g.rint8, &p, N / 16u, (M + 7u) / 8u, 1, 32, PK_GEMM, 30);
}

int xmx_rec_row(unsigned kind, int a, int b, int c, int d, unsigned rows, unsigned width,
		unsigned heads, unsigned scaled, unsigned stride, float cap)
{
	if (!g.recording) FAIL("not recording", 0);
	struct push p = { .a = addr_of(a), .b = addr_of(b), .c = addr_of(c), .d = addr_of(d),
			  .m = rows, .n = width, .k = scaled, .batch = heads, .flags = kind,
			  .sa = stride, .p0 = cap };
	if (!p.a || !p.c) FAIL("row operand is not a live buffer", 0);
	/* A softmax over rows too wide to stage 32 at a time takes as many whole rows as fit
	 * in the pass's 8 KB, `stride + 1` floats apart below the last 32, on the 256-thread
	 * build (attention.metal, softmax_rows) — at most 32 rows, the reciprocals' room; the
	 * 32-thread build takes the same rows the same way where that build is missing. */
	unsigned row_stride = stride ? stride : width, groups = (rows + 31) / 32;
	int whole = (kind & 0xFFu) == 1u && (row_stride != width || row_stride > 64u)
		    && row_stride + 1u <= 2016u;
	if (whole) {
		unsigned per = 2016u / (row_stride + 1u);
		if (per > 32u) per = 32u;
		groups = (rows + per - 1u) / per;
	}
	void *pipeline;
	if (whole && g.rrows) {
		if (resident_pipeline(8, kind, g.rrows, &pipeline)) return -1;
		return dispatch(pipeline, &p, groups, 1, 1, 256, PK_ROW, kind);
	}
	if (resident_pipeline(4, kind, g.rrow, &pipeline)) return -1;
	return dispatch(pipeline, &p, groups, 1, 1, 32, PK_ROW, kind);
}

int xmx_rec_history(int history, int motion, int out, unsigned pixels, unsigned channels,
		    unsigned height, unsigned width, unsigned absolute)
{
	if (!g.recording) FAIL("not recording", 0);
	struct push p = { .a = addr_of(history), .b = addr_of(motion), .c = addr_of(out),
			  .m = pixels, .n = channels, .k = height, .batch = width,
			  .flags = absolute };
	if (!p.a || !p.b || !p.c) FAIL("history operand is not a live buffer", 0);
	return dispatch(g.rhistory, &p, (pixels + 63) / 64, 1, 1, 64, PK_HISTORY, absolute & 1u);
}

/* -- encoding and submission ----------------------------------------------- */

/* Buffers reached through raw addresses are not bound, so Metal has to be told they are
 * in use: every live buffer, once per encoder. */
static void use_all(id<MTLComputeCommandEncoder> enc, __unsafe_unretained id<MTLResource> *list, unsigned count)
{
	if (count) [enc useResources:list count:count usage:MTLResourceUsageRead | MTLResourceUsageWrite];
}

static unsigned char stamp_kind[MAX_STAMPS];

/* Where stamp `i` samples: buffer i / SAMPLE_PAIRS, indices 2 * (i % SAMPLE_PAIRS) and + 1. */
static id<MTLCounterSampleBuffer> sample_buffer(unsigned stamp) { return samples[stamp / SAMPLE_PAIRS]; }
static NSUInteger sample_index(unsigned stamp) { return 2 * (stamp % SAMPLE_PAIRS); }

static void collect(unsigned stamps)
{
	if (!g.prof || !sample_buffers || !stamps) return;
	for (unsigned first = 0; first < stamps; first += SAMPLE_PAIRS) {
		unsigned count = stamps - first < SAMPLE_PAIRS ? stamps - first : SAMPLE_PAIRS;
		NSData *resolved = [sample_buffer(first) resolveCounterRange:NSMakeRange(0, 2 * count)];
		if (!resolved || resolved.length < 2 * count * sizeof(MTLCounterResultTimestamp)) return;
		const MTLCounterResultTimestamp *t = resolved.bytes;
		for (unsigned i = 0; i < count; i++) {
			uint64_t start = t[2 * i].timestamp, end = t[2 * i + 1].timestamp;
			if (start == MTLCounterErrorValue || end == MTLCounterErrorValue || end < start) {
				if (prof_each_n < MAX_STAMPS) {
					prof_each_kind[prof_each_n] = stamp_kind[first + i];
					prof_each[prof_each_n++] = -1.0;
				}
				continue;
			}
			double ms = (double)(end - start) * g.ns_per_tick * 1e-6;
			if (prof_each_n < MAX_STAMPS) {
				prof_each_kind[prof_each_n] = stamp_kind[first + i];
				prof_each[prof_each_n++] = ms;
			}
			prof_ms[stamp_kind[first + i]] += ms;
			prof_hits[stamp_kind[first + i]]++;
		}
	}
}

static int run_ops(const struct oplist *l)
{
	@autoreleasepool {
		id<MTLCommandBuffer> cb = [queue commandBuffer];
		if (!cb) FAIL("command buffer", 0);
		unsigned live = 0;
		for (int i = 0; i < MAX_RBUF; i++) live += rbufs[i].live ? 1 : 0;
		__unsafe_unretained id<MTLResource> *list = (__unsafe_unretained id<MTLResource> *)calloc(live ? live : 1, sizeof *list);
		if (!list) FAIL("out of memory listing buffers", 0);
		for (int i = 0, n = 0; i < MAX_RBUF; i++)
			if (rbufs[i].live) list[n++] = (__bridge id<MTLResource>)rbufs[i].b;

		int profiling = g.prof && sample_buffers != 0;
		unsigned stamp_limit = sample_buffers * SAMPLE_PAIRS;
		unsigned stamps = 0;
		id<MTLComputeCommandEncoder> enc = nil;
		for (unsigned i = 0; i < l->n; i++) {
			const struct op *op = &l->ops[i];
			if (op->kind == OP_BARRIER) {
				if (enc) [enc memoryBarrierWithScope:MTLBarrierScopeBuffers];
				continue;
			}
			if (op->kind == OP_COPY) {
				if (enc) { [enc updateFence:fence]; [enc endEncoding]; enc = nil; }
				id<MTLBlitCommandEncoder> blit;
				unsigned blit_stamp = stamps;
				int blit_sampled = profiling && stamps < stamp_limit;
				if (blit_sampled && g.prof_dispatch) {
					stamp_kind[stamps++] = op->stamp;
					blit = [cb blitCommandEncoder];
					[blit waitForFence:fence];
					[blit sampleCountersInBuffer:sample_buffer(blit_stamp) atSampleIndex:sample_index(blit_stamp) withBarrier:YES];
				} else if (blit_sampled) {
					MTLBlitPassDescriptor *d = [MTLBlitPassDescriptor blitPassDescriptor];
					d.sampleBufferAttachments[0].sampleBuffer = sample_buffer(stamps);
					d.sampleBufferAttachments[0].startOfEncoderSampleIndex = sample_index(stamps);
					d.sampleBufferAttachments[0].endOfEncoderSampleIndex = sample_index(stamps) + 1;
					stamp_kind[stamps++] = op->stamp;
					blit = [cb blitCommandEncoderWithDescriptor:d];
					[blit waitForFence:fence];
				} else {
					blit = [cb blitCommandEncoder];
					[blit waitForFence:fence];
				}
				[blit copyFromBuffer:(__bridge id<MTLBuffer>)rbufs[op->src].b sourceOffset:(NSUInteger)op->so
					    toBuffer:(__bridge id<MTLBuffer>)rbufs[op->dst].b destinationOffset:(NSUInteger)op->to
						size:(NSUInteger)op->bytes];
				if (blit_sampled && g.prof_dispatch)
					[blit sampleCountersInBuffer:sample_buffer(blit_stamp) atSampleIndex:sample_index(blit_stamp) + 1 withBarrier:YES];
				[blit updateFence:fence];
				[blit endEncoding];
				continue;
			}
			unsigned pass_stamp = stamps;
			int pass_sampled = profiling && stamps < stamp_limit;
			if (pass_sampled && g.prof_dispatch) {
				/* AMD's and Intel's Macs sample between dispatches, inside the one encoder */
				if (!enc) {
					enc = [cb computeCommandEncoderWithDispatchType:dispatch_type()];
					[enc waitForFence:fence];
					use_all(enc, list, live);
				}
				stamp_kind[stamps++] = op->stamp;
				[enc sampleCountersInBuffer:sample_buffer(pass_stamp) atSampleIndex:sample_index(pass_stamp) withBarrier:YES];
			} else if (pass_sampled) {
				/* Apple GPUs sample timestamps at encoder boundaries, not between dispatches,
				 * so a profiled pass is an encoder of its own. */
				if (enc) { [enc updateFence:fence]; [enc endEncoding]; enc = nil; }
				MTLComputePassDescriptor *d = [MTLComputePassDescriptor computePassDescriptor];
				d.dispatchType = dispatch_type();
				d.sampleBufferAttachments[0].sampleBuffer = sample_buffer(stamps);
				d.sampleBufferAttachments[0].startOfEncoderSampleIndex = sample_index(stamps);
				d.sampleBufferAttachments[0].endOfEncoderSampleIndex = sample_index(stamps) + 1;
				stamp_kind[stamps++] = op->stamp;
				enc = [cb computeCommandEncoderWithDescriptor:d];
				[enc waitForFence:fence];
				use_all(enc, list, live);
			} else if (!enc) {
				enc = [cb computeCommandEncoderWithDispatchType:dispatch_type()];
				[enc waitForFence:fence];
				use_all(enc, list, live);
			}
			[enc setComputePipelineState:(__bridge id<MTLComputePipelineState>)op->pipeline];
			[enc setBytes:&op->p length:sizeof op->p atIndex:0];
			[enc dispatchThreadgroups:MTLSizeMake(op->gx, op->gy, op->gz)
			    threadsPerThreadgroup:MTLSizeMake(op->tg, 1, 1)];
			if (pass_sampled && g.prof_dispatch)
				[enc sampleCountersInBuffer:sample_buffer(pass_stamp) atSampleIndex:sample_index(pass_stamp) + 1 withBarrier:YES];
			else if (profiling) { [enc updateFence:fence]; [enc endEncoding]; enc = nil; }
		}
		if (enc) { [enc updateFence:fence]; [enc endEncoding]; }
		free(list);
		if (finish(cb, "resident submit")) return -1;
		if (profiling) collect(stamps);
		return l->passes;
	}
}

int xmx_submit(void)
{
	if (!g.recording) FAIL("not recording", 0);
	g.recording = 0;
	rec.passes = g.recorded;
	return run_ops(&rec);
}

int xmx_graph_capture(void)
{
	if (!g.recording) FAIL("not recording", 0);
	int id;
	for (id = 0; id < MAX_GRAPHS && graphs[id].live; id++);
	if (id == MAX_GRAPHS) FAIL("out of graph slots", 0);
	/* The recording becomes the graph; a fresh list takes over for the next one. */
	graphs[id] = rec;
	graphs[id].passes = g.recorded;
	graphs[id].live = 1;
	memset(&rec, 0, sizeof rec);
	g.recording = 0;
	return id;
}

int xmx_graph_run(int id)
{
	if (g.recording) FAIL("cannot replay during recording", 0);
	if (id < 0 || id >= MAX_GRAPHS || !graphs[id].live) FAIL("graph is not live", 0);
	return run_ops(&graphs[id]);
}

int xmx_graph_destroy(int id)
{
	if (id < 0 || id >= MAX_GRAPHS || !graphs[id].live) return 0;
	op_free(&graphs[id]);
	return 0;
}

/* -- profiling ---------------------------------------------------------------- */

static double now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

/* Timestamps at encoder boundaries, which is where Apple GPUs can take them; a profiled
 * frame therefore runs one encoder per pass rather than one per block, and its total is
 * not the frame that would have run. A device that samples between dispatches instead (the
 * AMD and Intel GPUs of Intel Macs) keeps its encoders and brackets each pass. Off by default and free when off. Returns -1 when
 * the device cannot sample, and says why. */
int xmx_profile(int on)
{
	if (!on) { g.prof = 0; return 0; }
	if (!dev) FAIL("profiling needs an initialised device", 0);
	if (!sample_buffers) {
		@autoreleasepool {
			/* Apple GPUs sample at encoder boundaries; AMD's and Intel's between dispatches */
			g.prof_dispatch = ![dev supportsCounterSampling:MTLCounterSamplingPointAtStageBoundary];
			if (g.prof_dispatch && (![dev supportsCounterSampling:MTLCounterSamplingPointAtDispatchBoundary]
						|| ![dev supportsCounterSampling:MTLCounterSamplingPointAtBlitBoundary]))
				FAIL("this device cannot sample timestamps at encoder or dispatch boundaries", 0);
			id<MTLCounterSet> timestamps = nil;
			for (id<MTLCounterSet> set in dev.counterSets)
				if ([set.name isEqualToString:MTLCommonCounterSetTimestamp]) timestamps = set;
			if (!timestamps) FAIL("this device has no timestamp counter set", 0);
			MTLCounterSampleBufferDescriptor *d = [MTLCounterSampleBufferDescriptor new];
			d.counterSet = timestamps;
			d.storageMode = MTLStorageModeShared;
			d.sampleCount = 2 * SAMPLE_PAIRS;
			for (unsigned i = 0; i < SAMPLE_BUFFERS; i++) {
				NSError *error = nil;
				samples[i] = [dev newCounterSampleBufferWithDescriptor:d error:&error];
				if (!samples[i]) FAILNS("counter sample buffer", error);
				sample_buffers = i + 1;
			}
			/* GPU ticks to nanoseconds, measured rather than assumed: two paired samples
			 * a known wall-clock interval apart. */
			MTLTimestamp cpu0, gpu0, cpu1, gpu1;
			double wall0 = now_ns();
			[dev sampleTimestamps:&cpu0 gpuTimestamp:&gpu0];
			usleep(50000);
			[dev sampleTimestamps:&cpu1 gpuTimestamp:&gpu1];
			double wall1 = now_ns();
			g.ns_per_tick = gpu1 > gpu0 ? (wall1 - wall0) / (double)(gpu1 - gpu0) : 1.0;
			(void)cpu0; (void)cpu1;
		}
	}
	g.prof = 1;
	return 0;
}

void xmx_profile_reset(void)
{
	memset(prof_ms, 0, sizeof prof_ms);
	memset(prof_hits, 0, sizeof prof_hits);
	prof_each_n = 0;
}

unsigned xmx_profile_each_count(void) { return prof_each_n; }
double xmx_profile_each_ms(unsigned i) { return i < prof_each_n ? prof_each[i] : -1.0; }
unsigned xmx_profile_each_kind(unsigned i) { return i < prof_each_n ? prof_each_kind[i] : 0u; }

double xmx_profile_ms(unsigned kind) { return kind < PROF_KINDS ? prof_ms[kind] : 0.0; }
unsigned xmx_profile_count(unsigned kind) { return kind < PROF_KINDS ? prof_hits[kind] : 0u; }
