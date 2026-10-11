/*
 * libd3dmx — libxmx on Direct3D 12: the same entry points, the same contract, driving
 * Windows' own graphics API instead of Vulkan.
 *
 * Every `xmx_*` symbol in `xmx.h` is here, so `nr_frame.c`, `xmx.py` and `xmxres.py` bind
 * this library exactly as they bind libxmx (`NR_GPU_BACKEND=d3d12` makes them). Shader
 * names stay the SPIR-V names: `xmx_init("gemm_coopmat.spv")` runs the DXIL kernel this
 * library resolves that name to, from the table compiled into this binary
 * (`nr_dxil_embedded.h`, bin2c over the `.dxil` files dxc writes); a path is reduced to its
 * base name and looked up the same way, `XMX_DXIL_DIR=/dir` loads `<dir>/<kernel>.dxil`
 * files instead of the embedded modules, and a path with a directory in it that exists is
 * loaded as a DXIL file outright (the `XMX_*_SPV` overrides, for experiments).
 *
 * What maps onto what:
 *
 *   VkBuffer + device address     ID3D12Resource, its GPU virtual address bound as a *root
 *                                 UAV* (no descriptor heap: `SetComputeRootUnorderedAccessView`
 *                                 takes the address), and the operand's byte offset inside
 *                                 it in the same 128-byte push block the SPIR-V takes (eight operands: u0..u7), folded
 *                                 element offsets included. HLSL has no pointer to dereference,
 *                                 so the resource and the offset travel apart where Vulkan
 *                                 sends one address; `nr_d3d.hlsli` says how a kernel reads it.
 *   memory types                  heaps. On a cache-coherent UMA device (an integrated GPU)
 *                                 the graph's buffers are a CUSTOM heap in L0 with WRITE_BACK
 *                                 pages: cached, mapped, one pool — what HOST_CACHED was on the
 *                                 Xe2 APU, so `xmx_buf_ptr` is the mapping. On a discrete card
 *                                 (or under XMX_STAGING=1) they are DEFAULT-heap resources the
 *                                 host cannot address and reaches through UPLOAD / READBACK
 *                                 staging copies (`notes/phase65`). The buffers the host reads
 *                                 or writes (kinds 1 and 2) are always the WRITE_BACK kind — on
 *                                 a discrete card that is system memory, the trade `libxmx.c`
 *                                 makes for the head's strided readback.
 *   VK_KHR_cooperative_matrix     nothing. HLSL has no shipped matrix-matrix operation: the
 *                                 SM 6.8 WaveMatrix preview was withdrawn before release and
 *                                 SM 6.9's `linalg` is matrix-vector. Every GEMM here runs on
 *                                 the multiply-add kernels (`gemm_portable*.hlsl`); the
 *                                 cooperative-matrix names resolve to them, `xmx_coopmat()` is
 *                                 0 and `xmx_portable()` is 1 (`kernel_alias` below).
 *   specialization constant 0     nothing either: DXIL is compiled ahead of time and there is
 *                                 no shader compiler at run time, so the flags always come
 *                                 from the push block. `xmx_specialize` accepts the mask and
 *                                 `xmx_specialized_count()` stays 0.
 *   push constants                `SetComputeRoot32BitConstants`, 32 words at b0 (128 bytes).
 *   vkCmdPipelineBarrier          a UAV barrier (`D3D12_RESOURCE_BARRIER_TYPE_UAV`, no
 *                                 resource) between passes — the same places, suppressed by
 *                                 `xmx_sync(0)` the same way — and transition barriers around
 *                                 a copy, from the state this recording last left each buffer
 *                                 in (buffers decay to COMMON when a list completes, so every
 *                                 list starts from COMMON and the first use promotes).
 *   vkCmdCopyBuffer               `CopyBufferRegion`.
 *   a recorded command buffer     a command list: closed at submit, executed as often as
 *                                 wanted, so a captured graph *is* its list (each with its own
 *                                 allocator, which must not be reset under it).
 *   vkCmdWriteTimestamp           `EndQuery(TIMESTAMP)` after every pass, resolved into a
 *                                 READBACK buffer at the end of the list.
 *   vkCmdDispatch                 `Dispatch`, in pieces of at most 65535 groups along one
 *                                 axis (`D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION`;
 *                                 Vulkan on the measured drivers took the whole count). The
 *                                 kernel adds the piece's first group, sent in the push block's
 *                                 `spare` word, to its group id.
 *   xmx_adopt                     a host's ID3D12Device and ID3D12CommandQueue, in the `dev`
 *                                 and `q` arguments; the Vulkan-only arguments must be NULL.
 *
 * Numerics are the shaders' business: `precise` where the GLSL has it, `f32tof16` on every
 * narrow store. Nothing here changes a value.
 *
 * The device must offer Shader Model 6.2 and native 16-bit shader operations
 * (`D3D12_FEATURE_DATA_D3D12_OPTIONS4::Native16BitShaderOpsSupported`) — the half loads and
 * stores need them, as the SPIR-V needs `shaderFloat16` and `storageBuffer16BitAccess` —
 * or `xmx_open` refuses and says so. d3d12.dll and dxgi.dll are loaded at run time, so the
 * library links no import library and the static archive (libdlssnr) brings none.
 *
 * Build (Windows only): cl /O2 /LD libd3dmx.c, or the MinGW equivalent; CMake does it.
 */
#ifndef _WIN32
#error "libd3dmx is the Direct3D 12 runtime and exists on Windows only"
#endif
#ifndef COBJMACROS
#define COBJMACROS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <initguid.h>                 /* the IIDs below are defined here, not in an import library */
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xmx.h"
#include "nr_dxil_embedded.h"
#include "../ref/nr_portable.h"

/* The two DLLs' entry points, resolved by name so nothing is linked. */
typedef HRESULT (WINAPI *d3dmx_create_device_fn)(IUnknown *, D3D_FEATURE_LEVEL, REFIID, void **);
typedef HRESULT (WINAPI *d3dmx_serialize_fn)(const D3D12_ROOT_SIGNATURE_DESC *, D3D_ROOT_SIGNATURE_VERSION,
					       ID3DBlob **, ID3DBlob **);
typedef HRESULT (WINAPI *d3dmx_get_debug_fn)(REFIID, void **);
typedef HRESULT (WINAPI *d3dmx_experimental_fn)(UINT, const IID *, void *, UINT *);
typedef HRESULT (WINAPI *d3dmx_create_factory_fn)(UINT, REFIID, void **);
typedef HRESULT (WINAPI *d3dmx_create_factory1_fn)(REFIID, void **);

/* A lost device is recorded as well as described: the one failure after which nothing on
 * this device can succeed again. It stays set. */
static void note_lost(HRESULT hr);
#define FAIL(msg, r) do { snprintf(g.err, sizeof g.err, "%s (%d)", msg, (int)(r)); return -1; } while (0)
#define FAILH(msg, hr) do { HRESULT fail_hr = (hr); note_lost(fail_hr); \
	snprintf(g.err, sizeof g.err, "%s (0x%08lx)", msg, (unsigned long)fail_hr); return -1; } while (0)
#define RELEASE(p) do { if (p) { IUnknown_Release((IUnknown *)(p)); (p) = NULL; } } while (0)

/* Where a buffer lives. */
enum { MEM_DEVICE, MEM_WRITEBACK, MEM_UPLOAD, MEM_READBACK };

struct buf { ID3D12Resource *r; void *p; UINT64 cap; D3D12_GPU_VIRTUAL_ADDRESS addr; };

/* A command list and the allocator its memory comes from. `open` while recording. */
struct cmd { ID3D12CommandAllocator *alloc; ID3D12GraphicsCommandList *list; int open; };

/* The resident push block: `struct push` in libxmx.c byte for byte, 128 bytes. Here `a`..`d`,
 * `residual_cos` and `qkv_scale` are byte offsets into the root UAVs u0..u5 (and p0, p2 those
 * of u6, u7 where a pass takes eight) rather than
 * addresses, and `spare` is the first group of a piece of a split dispatch. The fifth and
 * sixth operands carry what the fused passes need beyond four (notes/improve-fusions.md):
 * the residual's cosine, a pass's second output, the attention bias, the V target of the
 * QKV epilogue; the query scale, the fused feed-forward's projection. */
struct push {
	uint64_t a, b, c, d;
	uint32_t m, n, k, batch, sa, sb, sc, flags;
	float p0, p1, p2, p3;
	uint32_t lda, ldb, ldc, spare;
	uint64_t residual_cos;
	uint32_t image_h, image_w, window_cols, window_pad;
	uint64_t qkv_scale;
};
#define PUSH_WORDS ((unsigned)(sizeof(struct push) / 4))
#define SPARE_WORD ((unsigned)(offsetof(struct push, spare) / 4))
#define OPERANDS 8                    /* root UAVs u0..u7, root parameters 1..8 */
_Static_assert(sizeof(struct push) == 128, "push block matches the HLSL Push");
_Static_assert(offsetof(struct push, residual_cos) == 96 && offsetof(struct push, qkv_scale) == 120,
	       "fifth and sixth operand offsets match nr_d3d.hlsli");
/* The seventh and eighth operands (u6, u7): where the SPIR-V reads 64-bit addresses at push
 * offsets 64 and 72 (the fused feed-forward's sin-cos table or adapter; the window block's
 * output projection and its pool or head weights), their byte offsets ride in p0 and p2. */
_Static_assert(offsetof(struct push, p0) == 64 && offsetof(struct push, p2) == 72,
	       "seventh and eighth operand offsets match nr_d3d.hlsli");

/* The descriptor path's block: the SPIR-V's seven words and the group base. */
struct desc_push { uint32_t M, N, K, sa, sb, sc, bt, base; };

#define MAX_GROUPS 65535u             /* D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION */

static struct {
	HMODULE d3d12, dxgi;
	d3dmx_create_device_fn create_device; d3dmx_serialize_fn serialize;
	d3dmx_get_debug_fn get_debug; d3dmx_experimental_fn experimental;
	d3dmx_create_factory_fn create_factory; d3dmx_create_factory1_fn create_factory1;
	HRESULT experimental_hr; int experimental_tried;

	IDXGIFactory4 *factory; IDXGIAdapter1 *adapter;
	ID3D12Device *dev; ID3D12CommandQueue *q; D3D12_COMMAND_LIST_TYPE qtype;
	ID3D12RootSignature *rs;
	ID3D12Fence *fence; HANDLE event; UINT64 fence_value;

	ID3D12PipelineState *pipe, *pipeb;             /* the descriptor path */
	ID3D12PipelineState *rgemm, *rtiled, *rstaged, *runary, *rrow, *rhistory;
	ID3D12PipelineState *rwindow[2], *rffn;        /* fused window attention, fused FFN: on first use */
	/* improve-b.md, on first use: the 256-lane whole-row softmax, the window block, the
	 * global attention and the integer GEMM — every one its multiply-add twin */
	ID3D12PipelineState *rrows, *rblock, *rglobal, *rint8;
	char *rpaths[6];
	unsigned specialize;
	unsigned tiling, tilem, tilen;
	/* the 16x64 portable build (gemm_portable_wide), optional: NULL keeps the old routing;
	 * `portable_tiled` (XMX_PORTABLE_TILED=1) gives the 16x32 build every shape it takes */
	ID3D12PipelineState *rwide;
	unsigned wide_calls, portable_tiled;
	int syncing;
	unsigned staging;
	int portable_packed;      /* flag 0x8000000 on every GEMM: A fetched 16 bytes at a time (off) */
	struct cmd cb, rcb, tcb;                       /* plain GEMMs, the recording, transfers */
	int recording, recorded, rready;
	/* XMX_D3D12_STEP=1: every recorded pass runs at once on a list of its own, timed and
	 * named on stderr, and the lists run so far are parked here for the graph to keep. */
	int step; struct cmd *rsteps; int rstep_n, rstep_cap;
	/* the debug layer's stored messages, drained to stderr (XMX_D3D12_DEBUG) */
	ID3D12InfoQueue *iq; int debug_level;

	/* GPU-side profiling: one timestamp after each recorded pass, resolved into `qread`. */
	ID3D12QueryHeap *qheap; struct buf qread; unsigned prof, prof_n; double ns_per_tick;

	char name[256]; char err[512]; char memory[320];
	int ready, lost, discrete, uma, coherent, unmapped, coopmat, portable;
	/* XMX_PIPELINE_STATS=FILE: what the driver made of each pipeline — the size of its cached
	 * blob, the one account Direct3D 12 gives — appended to FILE; XMX_PIPELINE_IR=DIR: the
	 * blobs themselves, a file each */
	FILE *stats;
	const char *ir_dir;
	struct buf A, B, C, up, down, zeros, dummy;

	/* An adopted device belongs to the host: never released here, and every submit on its
	 * queue is bracketed by the host's lock when one was given. */
	int adopted;
	void (*lock)(void *); void (*unlock)(void *); void *lock_ctx;
} g;

/* What `xmx_adopt` was handed, until `xmx_open` takes it. */
static struct {
	int set;
	ID3D12Device *dev; ID3D12CommandQueue *q;
	void (*lock)(void *); void (*unlock)(void *); void *ctx;
} adopt;

static void qlock(void) { if (g.lock) g.lock(g.lock_ctx); }
static void qunlock(void) { if (g.unlock) g.unlock(g.lock_ctx); }

/* A pass's kind is `family * 32 + subkind`, as in libxmx. */
#define MAX_STAMPS 8192
#define PROF_KINDS 256
enum { PK_GEMM = 0, PK_TILED, PK_STAGED, PK_UNARY, PK_ROW, PK_HISTORY, PK_COPY, PK_START = 7 };
static unsigned char stamp_kind[MAX_STAMPS];
static double prof_ms[PROF_KINDS];
static unsigned prof_hits[PROF_KINDS];
/* every pass's own duration, in recording order, since the last reset: what the totals sum away */
static double prof_each[MAX_STAMPS];
static unsigned char prof_each_kind[MAX_STAMPS];   /* the kind each timed pass was stamped with */
static unsigned prof_each_n;

/* Device-resident buffers. `state` is where this recording last left the buffer: every
 * list starts with every buffer in COMMON (they decay there when a list completes), the
 * first use promotes it, and a copy after a dispatch needs the explicit transition. */
#define MAX_RBUF 8192
struct rbuf { ID3D12Resource *r; void *p; D3D12_GPU_VIRTUAL_ADDRESS addr; UINT64 size;
	      int live, mapped; D3D12_RESOURCE_STATES state; };
static struct rbuf rbufs[MAX_RBUF];

#define MAX_GRAPHS 128
/* A graph is its lists: one, or one per pass when it was recorded under XMX_D3D12_STEP. */
static struct { struct cmd *cmds; int ncmds; int passes, live; unsigned stamps; unsigned char *kinds; } graphs[MAX_GRAPHS];

static void note_lost(HRESULT hr)
{
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_HUNG || hr == DXGI_ERROR_DEVICE_RESET)
		g.lost = 1;
}

static double seconds(void)
{
	static LARGE_INTEGER freq;
	LARGE_INTEGER now;
	if (!freq.QuadPart) QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&now);
	return (double)now.QuadPart / (double)freq.QuadPart;
}

static const char *severity_name(D3D12_MESSAGE_SEVERITY s)
{
	switch (s) {
	case D3D12_MESSAGE_SEVERITY_CORRUPTION: return "CORRUPTION";
	case D3D12_MESSAGE_SEVERITY_ERROR:      return "ERROR";
	case D3D12_MESSAGE_SEVERITY_WARNING:    return "WARNING";
	case D3D12_MESSAGE_SEVERITY_INFO:       return "info";
	default:                                return "message";
	}
}

/* The debug layer reports to a debugger (OutputDebugString), and nothing here runs under
 * one: print what it stored, then clear it. Corruption, errors and warnings always; the
 * informational messages too under XMX_D3D12_DEBUG=2. Nothing without the layer. */
static void drain_messages(const char *when)
{
	if (!g.iq) return;
	UINT64 n = ID3D12InfoQueue_GetNumStoredMessages(g.iq);
	D3D12_MESSAGE_SEVERITY quietest = g.debug_level >= 2 ? D3D12_MESSAGE_SEVERITY_MESSAGE
							    : D3D12_MESSAGE_SEVERITY_WARNING;
	for (UINT64 i = 0; i < n; i++) {
		SIZE_T len = 0;
		if (FAILED(ID3D12InfoQueue_GetMessage(g.iq, i, NULL, &len)) || !len) continue;
		D3D12_MESSAGE *m = malloc(len);
		if (!m) break;
		if (SUCCEEDED(ID3D12InfoQueue_GetMessage(g.iq, i, m, &len)) && m->Severity <= quietest)
			fprintf(stderr, "libd3dmx[%s] D3D12 %s #%d: %.*s\n", when, severity_name(m->Severity), (int)m->ID,
				(int)m->DescriptionByteLength, m->pDescription);
		free(m);
	}
	if (n) { ID3D12InfoQueue_ClearStoredMessages(g.iq); fflush(stderr); }
}

/* -- queries ------------------------------------------------------------ */

const char *xmx_error(void) { return g.err; }
int xmx_device_lost(void) { return g.lost; }
const char *xmx_device(void) { return g.name; }
const char *xmx_memory(void) { return g.dev ? g.memory : "not initialised"; }
int xmx_coopmat(void) { return g.dev ? g.coopmat : -1; }
int xmx_portable(void) { return g.dev ? g.portable : -1; }
/* the tiled portable kernel gathers a window-ordered A, and it is the only GEMM here */
int xmx_window_gather(void) { return g.dev ? 1 : -1; }
const char *xmx_path(void)
{
	if (!g.dev) return "not opened";
	return "portable multiply-add (Direct3D 12: HLSL has no cooperative matrix, so every GEMM is the "
	       "multiply-add kernel; the matrix-path names resolve to it)";
}
int xmx_staging_mode(void) { return g.unmapped; }
int xmx_discrete(void) { return g.discrete; }

/* Half subnormals through the GEMMs (xmx.h). Direct3D 12 has no 16-bit denorm mode to
 * declare: with native 16-bit shader operations (checked at open) a `half` keeps its
 * subnormals by the API's contract — dxc's `-denorm` switch names float32 alone, and the
 * build passes `preserve` there too — so the answer is what the API promises, and
 * `XMX_DENORM16=driver` changes nothing here. `src/gpu/test_denorm.py` is the check on a
 * device; none has run yet. */
int xmx_preserve16(void) { return g.dev ? 1 : 0; }

/* `half_round` in nr_d3d.hlsli is `f16tof32(f32tof16(x))`, the pack-and-unpack round trip
 * (two DXIL conversions, which no driver has been seen to fold); the cast through the
 * native 16-bit type is the probe's third output only. `XMX_HALF_ROUND=cast` is refused
 * with a note rather than honoured. */
int xmx_half_by_cast(void)
{
	const char *half = getenv("XMX_HALF_ROUND");
	if (half && *half && strcmp(half, "pack"))
		fprintf(stderr, "libd3dmx: XMX_HALF_ROUND=%s: the HLSL has one spelling, the round trip; keeping it\n", half);
	return 0;
}

/* There is no staged kernel here, so no packed staged loader: 0, as `xmx_staged_partial`
 * changes nothing. The portable GEMM's packed A fetch is flag 0x8000000 on every GEMM dispatch
 * (xmx_res_init; `XMX_PORTABLE_PACKED=1` turns it on). */
int xmx_staged_packed(void) { return 0; }
int xmx_portable_packed(void) { return g.portable_packed; }

/* -- shaders --------------------------------------------------------------- */

/* Every name a caller may hand over, and the DXIL module it runs. The cooperative-matrix
 * kernels have no HLSL twin, so their names run the multiply-add kernels of the same
 * geometry; `gemm_staged` is never dispatched while `xmx_portable()` is 1 (the runtime
 * skips the staged shape, as libxmx does without matrix units) but has to build. */
static const struct { const char *name, *blob; } kernel_alias[] = {
	{ "gemm_resident",         "gemm_portable" },
	{ "gemm_tiled",            "gemm_portable_tiled" },
	{ "gemm_staged",           "gemm_portable" },
	{ "gemm_coopmat",          "gemm_portable_desc" },
	{ "gemm_batched",          "gemm_portable_batched" },
	{ "gemm_f16acc",           "gemm_f16acc" },
	{ "gemm_portable",         "gemm_portable" },
	{ "gemm_portable_tiled",   "gemm_portable_tiled" },
	{ "gemm_portable_wide",    "gemm_portable_wide" },
	{ "gemm_wide",             "gemm_portable_wide" },
	{ "gemm_portable_desc",    "gemm_portable_desc" },
	{ "gemm_portable_batched", "gemm_portable_batched" },
	{ "resident",              "resident" },
	{ "attention",             "attention" },
	{ "attention_ab",          "attention_ab" },
	{ "history",               "history" },
	/* the fused passes: only their multiply-add twins exist here */
	{ "window_attention",          "window_attention_portable" },
	{ "window_attention_portable", "window_attention_portable" },
	{ "window_attention_portable_merged", "window_attention_portable_merged" },
	{ "ffn_fused",                 "ffn_fused_portable" },
	{ "ffn_fused_portable",        "ffn_fused_portable" },
	/* improve-b.md: the whole-row softmax's 256-lane build, and the fused passes' twins;
	 * the staged kernel's 32-row builds have none, and are accepted without being built */
	{ "attention_rows",            "attention_rows" },
	{ "window_block",              "window_block_portable" },
	{ "window_block_portable",     "window_block_portable" },
	{ "global_attention",          "global_attention_portable" },
	{ "global_attention_portable", "global_attention_portable" },
	{ "gemm_staged_int8",          "gemm_staged_int8_portable" },
	{ "gemm_staged_int8_portable", "gemm_staged_int8_portable" },
	/* the daemon's start-up probe (src/gpu/d3d12/half_probe.hlsl) */
	{ "half_probe",                "half_probe" },
};

/* `gemm_coopmat.spv`, `C:\dir\attention_ab.spv`, `resident` -> the kernel's name. */
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

static const char *blob_for(const char *stem)
{
	for (size_t i = 0; i < sizeof kernel_alias / sizeof *kernel_alias; i++)
		if (!strcmp(kernel_alias[i].name, stem)) return kernel_alias[i].blob;
	return NULL;
}

static const struct nr_embedded_shader *embedded_dxil(const char *blob)
{
#ifdef NR_EMBEDDED_DXIL
	char file[160];
	snprintf(file, sizeof file, "%s.dxil", blob);
	for (size_t i = 0; i < nr_embedded_dxil_count; i++)
		if (!strcmp(nr_embedded_dxil[i].name, file)) return &nr_embedded_dxil[i];
#endif
	(void)blob;
	return NULL;
}

static void *read_file(const char *path, size_t *len)
{
	FILE *f = fopen(path, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
	void *code = malloc(n > 0 ? (size_t)n : 1);
	if (!code || n <= 0 || fread(code, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(code); return NULL; }
	fclose(f);
	*len = (size_t)n;
	return code;
}

size_t xmx_embedded_shader(const char *name)
{
	if (!name) return 0;
	char stem[128];
	kernel_name(name, stem, sizeof stem);
	const char *blob = blob_for(stem);
	if (!blob) return 0;
	const char *dir = getenv("XMX_DXIL_DIR");
	if (dir && *dir) return 1;
	const struct nr_embedded_shader *e = embedded_dxil(blob);
	return e ? e->size : 0;
}

/* The DXIL for `spv_path`: an existing file when the path has a directory in it; else the
 * kernel the name resolves to, from XMX_DXIL_DIR or from the table compiled in. `*owned`
 * is what to free, if anything. */
static const void *shader_code(const char *spv_path, size_t *len, void **owned, char *why, size_t why_cap)
{
	*owned = NULL;
	if (strchr(spv_path, '/') || strchr(spv_path, '\\')) {
		void *code = read_file(spv_path, len);
		if (code) { *owned = code; return code; }
	}
	char stem[128];
	kernel_name(spv_path, stem, sizeof stem);
	const char *blob = blob_for(stem);
	if (!blob) { snprintf(why, why_cap, "no kernel '%s' in libd3dmx (from '%s')", stem, spv_path); return NULL; }
	const char *dir = getenv("XMX_DXIL_DIR");
	if (dir && *dir) {
		char path[1200];
		snprintf(path, sizeof path, "%s/%s.dxil", dir, blob);
		void *code = read_file(path, len);
		if (!code) { snprintf(why, why_cap, "XMX_DXIL_DIR: cannot read %s", path); return NULL; }
		*owned = code;
		return code;
	}
	const struct nr_embedded_shader *e = embedded_dxil(blob);
	if (!e) {
		snprintf(why, why_cap, "no DXIL compiled into libd3dmx for '%s' (kernel %s) and XMX_DXIL_DIR is not set",
			 stem, blob);
		return NULL;
	}
	*len = e->size;
	return e->data;
}

/* A DXIL container carries a 16-byte digest after its 'DXBC' tag; dxc writes it only when
 * dxil.dll sits beside it. Without one the runtime takes the module only with developer
 * mode on and experimental shader models enabled, which `xmx_open` asks for when needed. */
static int dxil_signed(const unsigned char *code, size_t len)
{
	if (len < 20 || memcmp(code, "DXBC", 4)) return 1;
	for (size_t i = 4; i < 20; i++) if (code[i]) return 1;
	return 0;
}

/* Set from another thread to abandon a build in progress; see xmx.h. A plain flag: the
 * build reads it between pipelines and only ever cares whether it is set. */
static volatile int g_cancel;

void xmx_cancel(int on) { g_cancel = on ? 1 : 0; }
int xmx_cancelled(void) { return g_cancel; }

static int build_pipeline(const char *spv_path, ID3D12PipelineState **out)
{
	/* One check for every pipeline this library builds; see xmx.h. */
	if (g_cancel) FAIL("cancelled", 0);
	size_t len = 0;
	void *owned = NULL;
	char why[1400];
	const void *code = shader_code(spv_path, &len, &owned, why, sizeof why);
	if (!code) { snprintf(g.err, sizeof g.err, "%.500s", why); return -1; }
	D3D12_COMPUTE_PIPELINE_STATE_DESC pd;
	memset(&pd, 0, sizeof pd);
	pd.pRootSignature = g.rs;
	pd.CS.pShaderBytecode = code;
	pd.CS.BytecodeLength = len;
	HRESULT hr = ID3D12Device_CreateComputePipelineState(g.dev, &pd, &IID_ID3D12PipelineState, (void **)out);
	int signed_module = dxil_signed(code, len);
	free(owned);
	if (FAILED(hr)) {
		note_lost(hr);
		drain_messages("pipeline");
		if (!signed_module)
			snprintf(g.err, sizeof g.err, "pipeline for '%s' (0x%08lx): the DXIL is unsigned (built by a dxc "
				 "without dxil.dll beside it); Direct3D 12 takes it only with developer mode on and "
				 "experimental shader models enabled, which libd3dmx asked for (result 0x%08lx)",
				 spv_path, (unsigned long)hr, (unsigned long)g.experimental_hr);
		else
			snprintf(g.err, sizeof g.err, "pipeline for '%s' (0x%08lx)", spv_path, (unsigned long)hr);
		return -1;
	}
	if (g.stats) {
		/* The driver's compiled pipeline is the cached blob; its size is the one figure the
		 * API gives of it, on the line libxmx writes for a Vulkan pipeline (flags=-1: there
		 * is no specialisation here, the flags ride in the push block). */
		char stem[128];
		kernel_name(spv_path, stem, sizeof stem);
		ID3DBlob *blob = NULL;
		size_t bytes = 0;
		if (SUCCEEDED(ID3D12PipelineState_GetCachedBlob(*out, &blob)) && blob)
			bytes = ID3D10Blob_GetBufferSize(blob);
		fprintf(g.stats, "%s\tflags=-1\td3d12\tdxil=%zu\tcached_blob=%zu\n", stem, len, bytes);
		if (g.ir_dir && blob) {
			char file[1200];
			snprintf(file, sizeof file, "%s/%s-cached.bin", g.ir_dir, stem);
			FILE *o = fopen(file, "wb");
			if (o) {
				fwrite(ID3D10Blob_GetBufferPointer(blob), 1, bytes, o);
				fclose(o);
				fprintf(g.stats, "\t\trepresentation cached blob (the driver's compiled pipeline): %zu bytes\n", bytes);
			}
		}
		fflush(g.stats);
		RELEASE(blob);
	}
	return 0;
}

static unsigned block_size(const char *name, unsigned fallback)
{
	const char *value = getenv(name);
	if (!value) return fallback;
	int n = atoi(value);
	if (n > 0) return (unsigned)n;
	fprintf(stderr, "libd3dmx: %s=%s is not a positive block size; using %u\n", name, value, fallback);
	return fallback;
}

/* Specialization has no counterpart here: the mask is kept so `xmx_specialization()` answers,
 * and every dispatch takes the generic pipeline with the flags in its push block. */
int xmx_specialize(unsigned mask)
{
	if (g.recording) FAIL("cannot switch specialization during recording", 0);
	if (mask > 7) FAIL("specialization mask must be in 0..7", 0);
	g.specialize = mask;
	return 0;
}

/* There is no staged kernel here to route a partial last 64-row block to; the switch is kept so
 * a caller can set it on any runtime, and changes nothing. */
int xmx_staged_partial(unsigned on)
{
	if (g.recording) FAIL("cannot change the staged routing during recording", 0);
	(void)on;
	return 0;
}

/* The staged kernel's 32-row builds (libxmx.c) have no counterpart either: their names are
 * accepted without being built, the switch is kept, and no GEMM is ever routed to them. */
int xmx_staged32_init(const char *shallow, const char *deep)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	(void)shallow; (void)deep;
	return 0;
}

int xmx_staged32(unsigned on)
{
	if (g.recording) FAIL("cannot change the staged routing during recording", 0);
	(void)on;
	return 0;
}

unsigned xmx_staged32_calls(void) { return 0; }

/* The row passes' 256-lane build (attention.hlsl -DROW_LANES=256), for the whole-row softmax
 * of the rows too wide to stage 32 at a time; without it the 32-lane build runs the same
 * arithmetic. */
int xmx_rows_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rrows) return 0;
	return build_pipeline(path, &g.rrows);
}

unsigned xmx_specialized_count(void) { return 0; }
unsigned xmx_specialization(void) { return g.specialize; }

/* -- resources ---------------------------------------------------------------- */

static HRESULT make_resource(UINT64 bytes, int where, int uav, ID3D12Resource **out)
{
	D3D12_HEAP_PROPERTIES hp;
	memset(&hp, 0, sizeof hp);
	D3D12_RESOURCE_STATES initial = D3D12_RESOURCE_STATE_COMMON;
	switch (where) {
	case MEM_DEVICE:   hp.Type = D3D12_HEAP_TYPE_DEFAULT; break;
	case MEM_UPLOAD:   hp.Type = D3D12_HEAP_TYPE_UPLOAD; initial = D3D12_RESOURCE_STATE_GENERIC_READ; break;
	case MEM_READBACK: hp.Type = D3D12_HEAP_TYPE_READBACK; initial = D3D12_RESOURCE_STATE_COPY_DEST; break;
	default:
		/* cached pages the GPU can address: on a UMA device the one pool, on a discrete
		 * card system memory across the bus */
		hp.Type = D3D12_HEAP_TYPE_CUSTOM;
		hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		hp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
		break;
	}
	D3D12_RESOURCE_DESC rd;
	memset(&rd, 0, sizeof rd);
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = bytes ? bytes : 4;
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.Format = DXGI_FORMAT_UNKNOWN;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	rd.Flags = uav ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
	return ID3D12Device_CreateCommittedResource(g.dev, &hp, D3D12_HEAP_FLAG_NONE, &rd, initial, NULL,
						    &IID_ID3D12Resource, (void **)out);
}

static void free_buf(struct buf *b)
{
	if (b->r) {
		if (b->p) ID3D12Resource_Unmap(b->r, 0, NULL);
		RELEASE(b->r);
	}
	memset(b, 0, sizeof *b);
}

/* A growable buffer of one kind, mapped when the host can see it. */
static int ensure(struct buf *b, UINT64 size, int where, int uav, const char *what)
{
	if (b->cap >= size) return 0;
	free_buf(b);
	HRESULT hr = make_resource(size, where, uav, &b->r);
	if (FAILED(hr)) FAILH(what, hr);
	if (where != MEM_DEVICE) {
		hr = ID3D12Resource_Map(b->r, 0, NULL, &b->p);
		if (FAILED(hr)) { free_buf(b); FAILH("Map", hr); }
	}
	b->addr = ID3D12Resource_GetGPUVirtualAddress(b->r);
	b->cap = size;
	return 0;
}

/* -- command lists ----------------------------------------------------------- */

static int cmd_create(struct cmd *c)
{
	HRESULT hr = ID3D12Device_CreateCommandAllocator(g.dev, g.qtype, &IID_ID3D12CommandAllocator, (void **)&c->alloc);
	if (FAILED(hr)) FAILH("CreateCommandAllocator", hr);
	hr = ID3D12Device_CreateCommandList(g.dev, 0, g.qtype, c->alloc, NULL, &IID_ID3D12GraphicsCommandList,
					    (void **)&c->list);
	if (FAILED(hr)) FAILH("CreateCommandList", hr);
	/* a list is born open; close it so every user starts with the same reset */
	hr = ID3D12GraphicsCommandList_Close(c->list);
	if (FAILED(hr)) FAILH("Close (new list)", hr);
	c->open = 0;
	return 0;
}

static void cmd_destroy(struct cmd *c)
{
	RELEASE(c->list);
	RELEASE(c->alloc);
	c->open = 0;
}

static int cmd_begin(struct cmd *c)
{
	if (c->open) return 0;
	HRESULT hr = ID3D12CommandAllocator_Reset(c->alloc);
	if (FAILED(hr)) FAILH("CommandAllocator Reset", hr);
	hr = ID3D12GraphicsCommandList_Reset(c->list, c->alloc, NULL);
	if (FAILED(hr)) FAILH("CommandList Reset", hr);
	c->open = 1;
	ID3D12GraphicsCommandList_SetComputeRootSignature(c->list, g.rs);
	return 0;
}

static int cmd_close(struct cmd *c)
{
	if (!c->open) return 0;
	c->open = 0;
	HRESULT hr = ID3D12GraphicsCommandList_Close(c->list);
	if (FAILED(hr)) FAILH("Close", hr);
	return 0;
}

/* How long a wait may stand before it is an error: XMX_D3D12_TIMEOUT seconds, and no limit
 * by default. A hung GPU is Windows' to notice — its watchdog removes the device, which every
 * second of the wait checks for — and a slow one is not an error: the first frame on a card
 * without matrix units, or on a virtual adapter, can take minutes. So the wait reports on
 * stderr how long it has been standing instead of giving up. */
static unsigned wait_limit(void)
{
	static int known; static unsigned limit;
	if (!known) {
		known = 1;
		const char *s = getenv("XMX_D3D12_TIMEOUT");
		if (s && *s) { int n = atoi(s); limit = n > 0 ? (unsigned)n : 0; }
	}
	return limit;
}

static int wait_fence(UINT64 value, const char *what)
{
	if (ID3D12Fence_GetCompletedValue(g.fence) < value) {
		HRESULT hr = ID3D12Fence_SetEventOnCompletion(g.fence, value, g.event);
		if (FAILED(hr)) FAILH(what, hr);
		double started = seconds(), next_note = 10.0;
		unsigned limit = wait_limit();
		for (;;) {
			DWORD w = WaitForSingleObject(g.event, 1000);
			if (w == WAIT_OBJECT_0) break;
			HRESULT removed = ID3D12Device_GetDeviceRemovedReason(g.dev);
			if (FAILED(removed)) { drain_messages(what); FAILH("device removed while waiting", removed); }
			if (w != WAIT_TIMEOUT) FAIL("WaitForSingleObject on the fence event", (int)GetLastError());
			double waited = seconds() - started;
			if (limit && waited >= (double)limit) {
				drain_messages(what);
				snprintf(g.err, sizeof g.err, "%s: fence not signalled after %u s on '%s' and the device is not "
					 "removed, so the GPU is still working (XMX_D3D12_TIMEOUT=0 waits; XMX_D3D12_STEP=1 runs "
					 "the recording pass by pass and times each)", what, limit, g.name);
				return -1;
			}
			if (waited >= next_note) {
				fprintf(stderr, "libd3dmx: %s: %.0f s and still waiting for '%s' (device alive, not removed)%s\n",
					what, waited, g.name,
					next_note < 11.0 ? "; XMX_D3D12_STEP=1 times every pass, XMX_D3D12_TIMEOUT=N gives up after N s"
							 : "");
				fflush(stderr);
				next_note += 30.0;
			}
		}
	}
	HRESULT removed = ID3D12Device_GetDeviceRemovedReason(g.dev);
	if (FAILED(removed)) { drain_messages(what); FAILH("device removed", removed); }
	drain_messages(what);
	return 0;
}

/* Execute `n` lists in order (closing any still open), signal the fence and wait for it.
 * Each list goes in an ExecuteCommandLists call of its own, so the buffers decay to COMMON
 * between them, which is what every list's baked transitions assume. */
static int execute_lists(struct cmd *c, int n, const char *what)
{
	for (int i = 0; i < n; i++) if (cmd_close(&c[i])) return -1;
	qlock();
	for (int i = 0; i < n; i++) {
		ID3D12CommandList *lists[1] = { (ID3D12CommandList *)c[i].list };
		ID3D12CommandQueue_ExecuteCommandLists(g.q, 1, lists);
	}
	HRESULT hr = ID3D12CommandQueue_Signal(g.q, g.fence, ++g.fence_value);
	qunlock();
	if (FAILED(hr)) FAILH(what, hr);
	return wait_fence(g.fence_value, what);
}

static int execute(struct cmd *c, const char *what) { return execute_lists(c, 1, what); }

/* Every list completes with every buffer decayed to COMMON: the recording's view of them
 * starts there. Only the recording tracks states (`transition`), so only its list begins
 * here — a transfer or a plain GEMM list in the middle of a recording must not reset the
 * recording's view, which is what an earlier version did from `cmd_begin`. */
static void states_decayed(void)
{
	for (int i = 0; i < MAX_RBUF; i++) rbufs[i].state = D3D12_RESOURCE_STATE_COMMON;
}

static void uav_barrier(ID3D12GraphicsCommandList *list)
{
	D3D12_RESOURCE_BARRIER b;
	memset(&b, 0, sizeof b);
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	b.UAV.pResource = NULL;
	ID3D12GraphicsCommandList_ResourceBarrier(list, 1, &b);
}

/* Bring a buffer to `want` in this list: nothing when the first use promotes it from
 * COMMON, a transition barrier when the recording already used it another way. */
static void transition(ID3D12GraphicsCommandList *list, struct rbuf *rb, D3D12_RESOURCE_STATES want)
{
	if (rb->state == want) return;
	if (rb->state == D3D12_RESOURCE_STATE_COMMON) { rb->state = want; return; }
	D3D12_RESOURCE_BARRIER b;
	memset(&b, 0, sizeof b);
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource = rb->r;
	b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	b.Transition.StateBefore = rb->state;
	b.Transition.StateAfter = want;
	ID3D12GraphicsCommandList_ResourceBarrier(list, 1, &b);
	rb->state = want;
}

/* -- the device ------------------------------------------------------------ */

static int load_dlls(void)
{
	if (g.d3d12) return 0;
	g.d3d12 = LoadLibraryA("d3d12.dll");
	g.dxgi = LoadLibraryA("dxgi.dll");
	if (!g.d3d12 || !g.dxgi) FAIL("cannot load d3d12.dll / dxgi.dll", (int)GetLastError());
	g.create_device = (d3dmx_create_device_fn)(void (*)(void))GetProcAddress(g.d3d12, "D3D12CreateDevice");
	g.serialize = (d3dmx_serialize_fn)(void (*)(void))GetProcAddress(g.d3d12, "D3D12SerializeRootSignature");
	g.get_debug = (d3dmx_get_debug_fn)(void (*)(void))GetProcAddress(g.d3d12, "D3D12GetDebugInterface");
	g.experimental = (d3dmx_experimental_fn)(void (*)(void))GetProcAddress(g.d3d12, "D3D12EnableExperimentalFeatures");
	g.create_factory = (d3dmx_create_factory_fn)(void (*)(void))GetProcAddress(g.dxgi, "CreateDXGIFactory2");
	g.create_factory1 = (d3dmx_create_factory1_fn)(void (*)(void))GetProcAddress(g.dxgi, "CreateDXGIFactory1");
	if (!g.create_device || !g.serialize || !(g.create_factory || g.create_factory1))
		FAIL("d3d12.dll / dxgi.dll lack D3D12CreateDevice, D3D12SerializeRootSignature or CreateDXGIFactory1/2", 0);
	return 0;
}

/* CreateDXGIFactory2 with the flags when dxgi.dll has it (Windows 8.1 on), CreateDXGIFactory1
 * — what VBA-M's own Direct3D 12 renderer calls — otherwise. */
static HRESULT make_factory(UINT flags, IDXGIFactory4 **out)
{
	if (g.create_factory) {
		HRESULT hr = g.create_factory(flags, &IID_IDXGIFactory4, (void **)out);
		if (SUCCEEDED(hr) || !g.create_factory1) return hr;
	}
	return g.create_factory1(&IID_IDXGIFactory4, (void **)out);
}

/* Unsigned DXIL needs the experimental shader models switched on before the device exists.
 * Asked for when the compiled-in modules are unsigned or XMX_D3D12_UNSIGNED=1; a refusal
 * (no developer mode) is remembered for the pipeline error, not fatal here. */
static void enable_unsigned_if_needed(void)
{
	if (g.experimental_tried) return;
	g.experimental_tried = 1;
	int need = 0;
	const char *forced = getenv("XMX_D3D12_UNSIGNED");
	if (forced && *forced && atoi(forced) != 0) need = 1;
#ifdef NR_EMBEDDED_DXIL
	for (size_t i = 0; i < nr_embedded_dxil_count && !need; i++)
		if (!dxil_signed(nr_embedded_dxil[i].data, nr_embedded_dxil[i].size)) need = 1;
#endif
	if (!need || !g.experimental) return;
	g.experimental_hr = g.experimental(1, &D3D12ExperimentalShaderModels, NULL, NULL);
}

static int check_device(ID3D12Device *dev);

/* Try one adapter: a Direct3D 12 device at feature level 11_0 on it, with Shader Model 6.2
 * and native 16-bit operations. On success the device is handed back; on failure `why`
 * says what was missing and the device, if any, is released. */
static int try_adapter(IDXGIAdapter1 *a, ID3D12Device **dev, char *why, size_t cap)
{
	*dev = NULL;
	HRESULT hr = g.create_device((IUnknown *)a, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)dev);
	if (FAILED(hr)) {
		*dev = NULL;
		snprintf(why, cap, "D3D12CreateDevice (0x%08lx)", (unsigned long)hr);
		return -1;
	}
	if (check_device(*dev)) {
		snprintf(why, cap, "%s", g.err);
		RELEASE(*dev);
		return -1;
	}
	return 0;
}

static void adapter_name(IDXGIAdapter1 *a, DXGI_ADAPTER_DESC1 *d, char *out, size_t cap)
{
	memset(d, 0, sizeof *d);
	IDXGIAdapter1_GetDesc1(a, d);
	if (WideCharToMultiByte(CP_UTF8, 0, d->Description, -1, out, (int)cap, NULL, NULL) <= 0)
		snprintf(out, cap, "Direct3D 12 adapter");
}

/* The adapter and the device on it, the way VBA-M's own Direct3D 12 renderer (panel.cpp)
 * finds them: every hardware adapter DXGI lists is *tried* — a device created on it and
 * the features checked — and the first that works is taken; when none does, Microsoft's
 * WARP software adapter, which has Shader Model 6.2 with 16-bit types on a current
 * Windows. The list is in performance order (discrete before integrated) where DXGI 1.6
 * gives one, in enumeration order before Windows 10 1803. `XMX_D3D12_ADAPTER=N` names the
 * N-th hardware adapter of that list and tries only it; `XMX_D3D12_WARP=1` goes straight to
 * the software one. When nothing works the error carries every adapter's reason. */
static int pick_adapter(const char *which, int warp, IDXGIAdapter1 **out, ID3D12Device **dev)
{
	*out = NULL; *dev = NULL;
	char why[512], reasons[1500] = "", name[256];
	size_t used = 0;
	DXGI_ADAPTER_DESC1 d;
	if (!warp) {
		IDXGIFactory6 *f6 = NULL;
		IDXGIFactory4_QueryInterface(g.factory, &IID_IDXGIFactory6, (void **)&f6);   /* absent before Windows 10 1803 */
		unsigned wanted = which && *which ? (unsigned)atoi(which) : ~0u, seen = 0;
		for (UINT i = 0;; i++) {
			IDXGIAdapter1 *a = NULL;
			HRESULT hr = f6 ? IDXGIFactory6_EnumAdapterByGpuPreference(f6, i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
										  &IID_IDXGIAdapter1, (void **)&a)
					: IDXGIFactory4_EnumAdapters1(g.factory, i, &a);
			if (FAILED(hr) || !a) break;
			adapter_name(a, &d, name, sizeof name);
			if (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { RELEASE(a); continue; }
			unsigned index = seen++;
			if (wanted != ~0u && index != wanted) { RELEASE(a); continue; }
			if (!try_adapter(a, dev, why, sizeof why)) { *out = a; break; }
			if (used < sizeof reasons)
				used += (size_t)snprintf(reasons + used, sizeof reasons - used, "%s[%u] %s: %s", used ? "; " : "",
							 index, name, why);
			RELEASE(a);
			if (wanted != ~0u) break;
		}
		RELEASE(f6);
		if (*out) return 0;
		if (wanted != ~0u) {
			if (!used) snprintf(reasons, sizeof reasons, "no hardware adapter at index %u", wanted);
			snprintf(g.err, sizeof g.err, "XMX_D3D12_ADAPTER: %.400s", reasons);
			return -1;
		}
		fprintf(stderr, "libd3dmx: no hardware adapter takes the kernels (%s); falling back to WARP\n",
			used ? reasons : "none listed");
	}
	IDXGIAdapter1 *a = NULL;
	HRESULT hr = IDXGIFactory4_EnumWarpAdapter(g.factory, &IID_IDXGIAdapter1, (void **)&a);
	if (FAILED(hr) || !a) {
		snprintf(g.err, sizeof g.err, "no adapter: %.400s%sEnumWarpAdapter 0x%08lx", reasons, used ? "; " : "",
			 (unsigned long)hr);
		return -1;
	}
	adapter_name(a, &d, name, sizeof name);
	if (try_adapter(a, dev, why, sizeof why)) {
		snprintf(g.err, sizeof g.err, "no adapter: %.250s%sWARP (%.60s): %.150s", reasons, used ? "; " : "", name, why);
		RELEASE(a);
		return -1;
	}
	*out = a;
	return 0;
}

/* Say where the buffers go, in one line, because on a discrete card the answer decides
 * everything about speed and nobody can see it from outside. */
static void note_memory(const DXGI_ADAPTER_DESC1 *d)
{
	double dedicated = d ? (double)d->DedicatedVideoMemory / (1 << 30) : 0.0;
	double shared = d ? (double)d->SharedSystemMemory / (1 << 30) : 0.0;
	if (g.unmapped)
		snprintf(g.memory, sizeof g.memory,
			 "%s: graph buffers in DEFAULT heaps, unmapped, the host reaches them by staging copies; "
			 "host-visible buffers WRITE_BACK in L0 (%.1f GiB dedicated, %.1f GiB shared)",
			 g.discrete ? "discrete card" : (!g.coherent ? "UMA without cache coherence" : "staging forced"),
			 dedicated, shared);
	else
		snprintf(g.memory, sizeof g.memory,
			 "cache-coherent UMA: every buffer a CUSTOM heap in L0 with WRITE_BACK pages, mapped, one pool "
			 "(%.1f GiB shared)", shared);
}

static int want_unmapped(void)
{
	const char *forced = getenv("XMX_STAGING");
	if (forced && *forced) return atoi(forced) != 0;
	return !g.coherent;
}

/* What the device must have: Shader Model 6.2 and native 16-bit operations, for the half
 * loads and stores; the architecture query decides where the buffers live. */
static int check_device(ID3D12Device *dev)
{
	D3D12_FEATURE_DATA_SHADER_MODEL sm = { D3D_SHADER_MODEL_6_2 };
	HRESULT hr = ID3D12Device_CheckFeatureSupport(dev, D3D12_FEATURE_SHADER_MODEL, &sm, sizeof sm);
	if (FAILED(hr)) FAILH("this Direct3D 12 runtime predates Shader Model 6.2", hr);
	if (sm.HighestShaderModel < D3D_SHADER_MODEL_6_2)
		FAIL("no Shader Model 6.2 (needed for 16-bit loads and stores); highest", (int)sm.HighestShaderModel);
	D3D12_FEATURE_DATA_D3D12_OPTIONS4 o4;
	memset(&o4, 0, sizeof o4);
	hr = ID3D12Device_CheckFeatureSupport(dev, D3D12_FEATURE_D3D12_OPTIONS4, &o4, sizeof o4);
	if (FAILED(hr) || !o4.Native16BitShaderOpsSupported)
		FAIL("no native 16-bit shader operations (D3D12_OPTIONS4), which the half buffers need", (int)hr);
	D3D12_FEATURE_DATA_ARCHITECTURE1 arch;
	memset(&arch, 0, sizeof arch);
	hr = ID3D12Device_CheckFeatureSupport(dev, D3D12_FEATURE_ARCHITECTURE1, &arch, sizeof arch);
	if (FAILED(hr)) {
		D3D12_FEATURE_DATA_ARCHITECTURE a0;
		memset(&a0, 0, sizeof a0);
		hr = ID3D12Device_CheckFeatureSupport(dev, D3D12_FEATURE_ARCHITECTURE, &a0, sizeof a0);
		if (FAILED(hr)) FAILH("CheckFeatureSupport(ARCHITECTURE)", hr);
		arch.UMA = a0.UMA; arch.CacheCoherentUMA = a0.CacheCoherentUMA;
	}
	g.uma = arch.UMA ? 1 : 0;
	g.coherent = (arch.UMA && arch.CacheCoherentUMA) ? 1 : 0;
	g.discrete = !g.uma;
	g.coopmat = 0;
	g.portable = 1;
	return 0;
}

/* The root signature every kernel here is written against: the push block as 32 root
 * constants at b0 and the eight operands as root UAVs u0..u7 — 32 + 8 * 2 = 48 of the 64
 * DWORDs a root signature may take. */
static int make_root_signature(void)
{
	D3D12_ROOT_PARAMETER params[1 + OPERANDS];
	memset(params, 0, sizeof params);
	params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	params[0].Constants.ShaderRegister = 0;
	params[0].Constants.RegisterSpace = 0;
	params[0].Constants.Num32BitValues = PUSH_WORDS;
	params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	for (unsigned i = 1; i < 1 + OPERANDS; i++) {
		params[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		params[i].Descriptor.ShaderRegister = i - 1;
		params[i].Descriptor.RegisterSpace = 0;
		params[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}
	D3D12_ROOT_SIGNATURE_DESC rsd;
	memset(&rsd, 0, sizeof rsd);
	rsd.NumParameters = 1 + OPERANDS;
	rsd.pParameters = params;
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	ID3DBlob *blob = NULL, *error = NULL;
	HRESULT hr = g.serialize(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
	if (FAILED(hr)) {
		if (error) {
			snprintf(g.err, sizeof g.err, "root signature: %.*s", (int)ID3D10Blob_GetBufferSize(error),
				 (const char *)ID3D10Blob_GetBufferPointer(error));
			RELEASE(error);
			return -1;
		}
		FAILH("D3D12SerializeRootSignature", hr);
	}
	hr = ID3D12Device_CreateRootSignature(g.dev, 0, ID3D10Blob_GetBufferPointer(blob),
					      ID3D10Blob_GetBufferSize(blob), &IID_ID3D12RootSignature, (void **)&g.rs);
	RELEASE(blob);
	RELEASE(error);
	if (FAILED(hr)) FAILH("CreateRootSignature", hr);
	return 0;
}

/* The fence, the root signature, the dummy operand: what both an own and an adopted device
 * need once the device and queue exist. */
static int finish_open(const DXGI_ADAPTER_DESC1 *desc)
{
	if (check_device(g.dev)) return -1;
	HRESULT hr = ID3D12Device_CreateFence(g.dev, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)&g.fence);
	if (FAILED(hr)) FAILH("CreateFence", hr);
	g.event = CreateEventW(NULL, FALSE, FALSE, NULL);
	if (!g.event) FAIL("CreateEvent", (int)GetLastError());
	/* the debug layer's stored messages, when the layer is on (ours or a host's) */
	const char *debug = getenv("XMX_D3D12_DEBUG");
	g.debug_level = debug && *debug ? atoi(debug) : 0;
	if (FAILED(ID3D12Device_QueryInterface(g.dev, &IID_ID3D12InfoQueue, (void **)&g.iq))) g.iq = NULL;
	if (g.iq) fprintf(stderr, "libd3dmx: the Direct3D 12 debug layer is on; its messages follow on stderr\n");
	const char *step = getenv("XMX_D3D12_STEP");
	g.step = step && *step && atoi(step) != 0;
	/* XMX_PIPELINE_STATS / XMX_PIPELINE_IR, as libxmx reads them (build_pipeline) */
	const char *stats_path = getenv("XMX_PIPELINE_STATS");
	if (stats_path && *stats_path) {
		g.stats = fopen(stats_path, "a");
		if (!g.stats) fprintf(stderr, "libd3dmx: cannot write pipeline statistics to %s\n", stats_path);
		const char *ir = getenv("XMX_PIPELINE_IR");
		g.ir_dir = g.stats && ir && *ir ? ir : NULL;
	}
	if (make_root_signature()) return -1;
	/* an operand a pass does not use still needs an address in its root slot */
	if (ensure(&g.dummy, 256, MEM_WRITEBACK, 1, "dummy operand")) return -1;
	memset(g.dummy.p, 0, 256);
	g.unmapped = want_unmapped();
	note_memory(desc);
	return 0;
}

/* Share a host's Direct3D 12 device instead of creating one. Call before `xmx_open` (or
 * after `xmx_close`); the next `xmx_open` takes them. The handles are `void *` so a caller
 * that binds this by name needs no header — and the argument list is libxmx's, so the same
 * host code can hand over either API:
 *
 *   dev, q          the host's ID3D12Device* and ID3D12CommandQueue*. Lists are recorded for
 *                   the queue's own type (a direct or a compute queue). If the host also
 *                   submits on `q`, it passes `lock`/`unlock` (called around every
 *                   ExecuteCommandLists here, with `ctx`) and takes the same lock around its
 *                   own submits and presents.
 *   inst, pd, gipa  Vulkan's; must be NULL, or the call is refused as the wrong API.
 *   qi, coopmat     ignored.
 *
 * The host owns the objects: `xmx_close` releases everything libd3dmx made on them and
 * leaves them alone. The host must `xmx_close` before it destroys the device. */
int xmx_adopt(void *inst, void *pd, void *dev, void *q, unsigned qi, int coopmat, void *gipa,
	      void (*lock)(void *), void (*unlock)(void *), void *ctx)
{
	(void)qi; (void)coopmat;
	if (g.dev) FAIL("xmx_adopt: a device is open; xmx_close first", 0);
	if (inst || pd || gipa)
		FAIL("xmx_adopt: libd3dmx adopts an ID3D12Device and ID3D12CommandQueue; the Vulkan arguments must be NULL", 0);
	if (!dev || !q) FAIL("xmx_adopt: a device and a command queue are needed", 0);
	adopt.set = 1;
	adopt.dev = (ID3D12Device *)dev; adopt.q = (ID3D12CommandQueue *)q;
	adopt.lock = lock; adopt.unlock = unlock; adopt.ctx = ctx;
	return 0;
}

/* 1 while an adopted device is open, 0 for libd3dmx's own, -1 when nothing is open. */
int xmx_adopted(void) { return g.dev ? g.adopted : -1; }

static int open_adopted(void)
{
	if (load_dlls()) return -1;
	g.dev = adopt.dev; g.q = adopt.q;
	D3D12_COMMAND_QUEUE_DESC qd;
	memset(&qd, 0, sizeof qd);
	/* aggregate return: the C binding takes the result through a pointer on both toolchains */
	g.q->lpVtbl->GetDesc(g.q, &qd);
	g.qtype = qd.Type;
	if (g.qtype != D3D12_COMMAND_LIST_TYPE_DIRECT && g.qtype != D3D12_COMMAND_LIST_TYPE_COMPUTE) {
		g.dev = NULL; g.q = NULL;
		FAIL("adopted queue is neither direct nor compute", (int)qd.Type);
	}
	g.lock = adopt.lock; g.unlock = adopt.unlock; g.lock_ctx = adopt.ctx;
	g.adopted = 1;
	adopt.set = 0;
	snprintf(g.name, sizeof g.name, "Direct3D 12 device (shared)");
	IDXGIAdapter1 *a = NULL;
	DXGI_ADAPTER_DESC1 d;
	memset(&d, 0, sizeof d);
	LUID luid;
	memset(&luid, 0, sizeof luid);
	g.dev->lpVtbl->GetAdapterLuid(g.dev, &luid);   /* aggregate return: through a pointer in the C binding */
	if (SUCCEEDED(make_factory(0, &g.factory))
	    && SUCCEEDED(IDXGIFactory4_EnumAdapterByLuid(g.factory, luid, &IID_IDXGIAdapter1, (void **)&a)) && a) {
		IDXGIAdapter1_GetDesc1(a, &d);
		char text[200];
		if (WideCharToMultiByte(CP_UTF8, 0, d.Description, -1, text, sizeof text, NULL, NULL) > 0)
			snprintf(g.name, sizeof g.name, "%s (shared)", text);
		RELEASE(a);
	}
	if (finish_open(&d)) { g.dev = NULL; g.q = NULL; g.adopted = 0; return -1; }
	return 0;
}

int xmx_open(void)
{
	if (g.dev) return 0;
	if (adopt.set) return open_adopted();
	if (load_dlls()) return -1;
	const char *debug = getenv("XMX_D3D12_DEBUG");
	int debugging = debug && *debug && atoi(debug) != 0;
	if (debugging && g.get_debug) {
		ID3D12Debug *dbg = NULL;
		if (SUCCEEDED(g.get_debug(&IID_ID3D12Debug, (void **)&dbg)) && dbg) {
			ID3D12Debug_EnableDebugLayer(dbg);
			RELEASE(dbg);
		}
	}
	enable_unsigned_if_needed();
	HRESULT hr = make_factory(debugging ? DXGI_CREATE_FACTORY_DEBUG : 0u, &g.factory);
	if (FAILED(hr)) FAILH("CreateDXGIFactory", hr);
	const char *warp = getenv("XMX_D3D12_WARP"), *which = getenv("XMX_D3D12_ADAPTER");
	if (pick_adapter(which, warp && *warp && atoi(warp) != 0, &g.adapter, &g.dev)) return -1;
	DXGI_ADAPTER_DESC1 d;
	adapter_name(g.adapter, &d, g.name, sizeof g.name);
	if (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
		size_t n = strlen(g.name);
		snprintf(g.name + n, sizeof g.name - n, " (WARP, software)");
	}
	D3D12_COMMAND_QUEUE_DESC qd;
	memset(&qd, 0, sizeof qd);
	qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	g.qtype = qd.Type;
	hr = ID3D12Device_CreateCommandQueue(g.dev, &qd, &IID_ID3D12CommandQueue, (void **)&g.q);
	if (FAILED(hr)) FAILH("CreateCommandQueue", hr);
	return finish_open(&d);
}

int xmx_buf_destroy(int id);
int xmx_graph_destroy(int id);

/* Release everything libd3dmx made: buffers, graphs, pipelines, lists, the fence — and its
 * own device, queue and adapter, unless they were adopted, in which case they go back to
 * the host untouched. Every buffer id and graph id is dead after this. */
void xmx_close(void)
{
	if (!g.dev) { adopt.set = 0; return; }
	if (g.fence) wait_fence(g.fence_value, "close");
	for (int i = 0; i < MAX_RBUF; i++) xmx_buf_destroy(i);
	for (int i = 0; i < MAX_GRAPHS; i++) xmx_graph_destroy(i);
	for (int i = 0; i < g.rstep_n; i++) cmd_destroy(&g.rsteps[i]);
	free(g.rsteps);
	ID3D12PipelineState **pipes[] = { &g.rgemm, &g.rtiled, &g.rstaged, &g.runary, &g.rrow, &g.rhistory,
					  &g.rwindow[0], &g.rwindow[1], &g.rffn, &g.rrows, &g.rblock, &g.rglobal,
					  &g.rint8, &g.rwide, &g.pipe, &g.pipeb };
	for (size_t i = 0; i < sizeof pipes / sizeof *pipes; i++) RELEASE(*pipes[i]);
	for (int i = 0; i < 6; i++) free(g.rpaths[i]);
	cmd_destroy(&g.cb); cmd_destroy(&g.rcb); cmd_destroy(&g.tcb);
	struct buf *bufs[] = { &g.A, &g.B, &g.C, &g.up, &g.down, &g.zeros, &g.dummy, &g.qread };
	for (size_t i = 0; i < sizeof bufs / sizeof *bufs; i++) free_buf(bufs[i]);
	RELEASE(g.qheap);
	RELEASE(g.rs);
	RELEASE(g.fence);
	drain_messages("close");
	RELEASE(g.iq);
	if (g.event) CloseHandle(g.event);
	if (!g.adopted) {
		RELEASE(g.q);
		RELEASE(g.dev);
	}
	RELEASE(g.adapter);
	RELEASE(g.factory);
	HMODULE d3d12 = g.d3d12, dxgi = g.dxgi;
	d3dmx_create_device_fn cd = g.create_device; d3dmx_serialize_fn se = g.serialize;
	d3dmx_get_debug_fn gd = g.get_debug; d3dmx_experimental_fn ex = g.experimental;
	d3dmx_create_factory_fn cf = g.create_factory; d3dmx_create_factory1_fn cf1 = g.create_factory1;
	int tried = g.experimental_tried; HRESULT ehr = g.experimental_hr;
	if (g.stats) fclose(g.stats);
	memset(&g, 0, sizeof g);
	/* the DLLs stay loaded (a host may be using them too) and the experimental switch is
	 * process-wide, so both survive a close */
	g.d3d12 = d3d12; g.dxgi = dxgi;
	g.create_device = cd; g.serialize = se; g.get_debug = gd; g.experimental = ex; g.create_factory = cf;
	g.create_factory1 = cf1;
	g.experimental_tried = tried; g.experimental_hr = ehr;
	memset(graphs, 0, sizeof graphs);
	memset(rbufs, 0, sizeof rbufs);
	memset(prof_ms, 0, sizeof prof_ms);
	memset(prof_hits, 0, sizeof prof_hits);
	prof_each_n = 0;
	adopt.set = 0;
}

/* -- the plain GEMM path ------------------------------------------------- */

int xmx_init(const char *spv_path)
{
	if (g.ready) return 0;
	if (xmx_open()) return -1;
	if (build_pipeline(spv_path, &g.pipe)) return -1;
	if (cmd_create(&g.cb)) return -1;
	g.ready = 1;
	return 0;
}

/* The descriptor path's operands: mapped, cached, growable. */
static int ensure_operands(UINT64 a, UINT64 b, UINT64 c)
{
	return ensure(&g.A, a, MEM_WRITEBACK, 1, "operand A") || ensure(&g.B, b, MEM_WRITEBACK, 1, "operand B")
	    || ensure(&g.C, c, MEM_WRITEBACK, 1, "operand C");
}

int xmx_reserve(unsigned M, unsigned N, unsigned K, void **pa, void **pb, void **pc)
{
	if (!g.ready) FAIL("not initialised", 0);
	if (ensure_operands((UINT64)M * K * 2, (UINT64)K * N * 2, (UINT64)M * N * 4)) return -1;
	if (pa) *pa = g.A.p;
	if (pb) *pb = g.B.p;
	if (pc) *pc = g.C.p;
	return 0;
}

int xmx_reserve_bytes(unsigned long long a, unsigned long long b, unsigned long long c,
		      void **pa, void **pb, void **pc)
{
	if (!g.ready) FAIL("not initialised", 0);
	if (ensure_operands(a, b, c)) return -1;
	if (pa) *pa = g.A.p;
	if (pb) *pb = g.B.p;
	if (pc) *pc = g.C.p;
	return 0;
}

/* Dispatch `gx` x `gy` x `gz` groups, in pieces of at most MAX_GROUPS along the axis that
 * needs it (`split_y`: the GEMMs' row groups; else the element groups on x). Each piece
 * re-pushes the block with its first group in `base_word`. */
static void dispatch_pieces(ID3D12GraphicsCommandList *list, uint32_t *words, unsigned nwords, unsigned base_word,
			    unsigned gx, unsigned gy, unsigned gz, int split_y)
{
	unsigned total = split_y ? gy : gx;
	if (!total) total = 1;
	for (unsigned base = 0; base < total; base += MAX_GROUPS) {
		unsigned piece = total - base < MAX_GROUPS ? total - base : MAX_GROUPS;
		words[base_word] = base;
		ID3D12GraphicsCommandList_SetComputeRoot32BitConstants(list, 0, nwords, words, 0);
		ID3D12GraphicsCommandList_Dispatch(list, split_y ? gx : piece, split_y ? piece : gy, gz);
	}
}

/* One dispatch of a descriptor-path kernel on A, B, C, `iters` times with a barrier between. */
static int run_desc(ID3D12PipelineState *pso, struct desc_push *push, unsigned gx, unsigned gy, unsigned gz,
		    unsigned iters)
{
	if (cmd_begin(&g.cb)) return -1;
	ID3D12GraphicsCommandList *list = g.cb.list;
	ID3D12GraphicsCommandList_SetPipelineState(list, pso);
	ID3D12GraphicsCommandList_SetComputeRootUnorderedAccessView(list, 1, g.A.addr);
	ID3D12GraphicsCommandList_SetComputeRootUnorderedAccessView(list, 2, g.B.addr);
	ID3D12GraphicsCommandList_SetComputeRootUnorderedAccessView(list, 3, g.C.addr);
	for (unsigned i = 4; i <= OPERANDS; i++)
		ID3D12GraphicsCommandList_SetComputeRootUnorderedAccessView(list, i, g.dummy.addr);
	for (unsigned i = 0; i < (iters ? iters : 1); i++) {
		dispatch_pieces(list, (uint32_t *)push, sizeof *push / 4, 7, gx, gy, gz, 1);
		if (i + 1 < iters) uav_barrier(list);
	}
	return execute(&g.cb, "gemm");
}

int xmx_gemm(unsigned M, unsigned N, unsigned K, const void *a, const void *b, void *c, unsigned iters)
{
	if (!g.ready) FAIL("not initialised", 0);
	UINT64 sa = (UINT64)M * K * 2, sb = (UINT64)K * N * 2, sc = (UINT64)M * N * 4;
	if (ensure_operands(sa, sb, sc)) return -1;
	if (a) memcpy(g.A.p, a, (size_t)sa);
	if (b) memcpy(g.B.p, b, (size_t)sb);
	struct desc_push push = { M, N, K, 0, 0, 0, 0, 0 };
	if (run_desc(g.pipe, &push, N / 16, M / 8, 1, iters)) return -1;
	if (c) memcpy(c, g.C.p, (size_t)sc);
	return 0;
}

int xmx_init_batched(const char *spv_path)
{
	if (!g.ready) FAIL("not initialised", 0);
	if (g.pipeb) return 0;
	return build_pipeline(spv_path, &g.pipeb);
}

int xmx_gemm_batched(unsigned M, unsigned N, unsigned K, unsigned batch,
		     unsigned sa, unsigned sb, unsigned sc, unsigned bt)
{
	if (!g.pipeb) FAIL("batched pipeline not built", 0);
	struct desc_push push = { M, N, K, sa, sb, sc, bt, 0 };
	return run_desc(g.pipeb, &push, N / 16, M / 8, batch ? batch : 1, 1);
}

/* -- the resident runtime ------------------------------------------------- */

int xmx_res_init(const char *gemm_spv, const char *unary_spv, const char *row_spv,
		 const char *history_spv, const char *tiled_spv, const char *staged_spv)
{
	if (!g.ready) FAIL("not initialised", 0);
	if (g.rready) return 0;
	const char *paths[] = { gemm_spv, tiled_spv, staged_spv, unary_spv, row_spv };
	for (unsigned i = 0; i < 5; i++) {
		g.rpaths[i] = nr_strdup(paths[i]);
		if (!g.rpaths[i]) FAIL("pipeline path allocation", 0);
	}
	const char *spec = getenv("XMX_SPECIALIZE");
	g.specialize = spec ? (unsigned)atoi(spec) : 7;
	if (g.specialize > 7) FAIL("XMX_SPECIALIZE must be in 0..7", 0);
	if (build_pipeline(gemm_spv, &g.rgemm) || build_pipeline(unary_spv, &g.runary)
	    || build_pipeline(row_spv, &g.rrow) || build_pipeline(history_spv, &g.rhistory)
	    || build_pipeline(tiled_spv, &g.rtiled) || build_pipeline(staged_spv, &g.rstaged))
		return -1;
	const char *tile = getenv("XMX_TILE_K");
	g.tiling = tile ? atoi(tile) : 1;
	g.tilem = block_size("XMX_TILE_M", 16);
	g.tilen = block_size("XMX_TILE_N", 32);
	const char *sk = getenv("XMX_STAGE_K");
	g.staging = sk ? (unsigned)atoi(sk) : 32;   /* libxmx's default; there is no staged kernel here */
	/* libxmx's packed staged loader (gemm_staged.comp, constant 2) as the one GEMM here can
	 * take it: flag 0x8000000 on every GEMM dispatch, under which gemm_portable.hlsl fetches
	 * A sixteen bytes at a time where the addresses allow. The same terms in the same order,
	 * so the bytes are the same. Off by default, as on the other runtimes' portable paths —
	 * on an M3 both measured slower with it, the compiler combining the eight-byte fetches
	 * already — and not measured on a Direct3D 12 device (none has run a frame yet):
	 * `XMX_PORTABLE_PACKED=1` is how the first one will. */
	g.portable_packed = 0;
	const char *ppacked = getenv("XMX_PORTABLE_PACKED");
	if (ppacked && *ppacked) {
		if (!strcmp(ppacked, "1")) g.portable_packed = 1;
		else if (!strcmp(ppacked, "0")) g.portable_packed = 0;
		else fprintf(stderr, "libd3dmx: XMX_PORTABLE_PACKED=%s is neither 0 nor 1; keeping %d\n",
			     ppacked, g.portable_packed);
	}
	/* As libxmx's portable path: the 16x32 build only where a 32-column block is needed
	 * (the QKV epilogue, the pool, the window gather) — the 8x16 kernel measured 1.3-1.8x
	 * faster on the N = 32 and 96 shapes through MoltenVK — and the 16x64 build, found
	 * beside the tiled one (its name with `_tiled` made `_wide`), for N in whole 64-column
	 * blocks. Every element's sum is the same in every build, so no byte moves. Without the
	 * wide module, or with XMX_PORTABLE_WIDE=0, the 16x32 build serves as before. */
	const char *tiled_env = getenv("XMX_PORTABLE_TILED");
	g.portable_tiled = tiled_env ? (unsigned)atoi(tiled_env) : 0;
	const char *wide_env = getenv("XMX_PORTABLE_WIDE");
	const char *at = strstr(tiled_spv, "_tiled");
	if ((!wide_env || atoi(wide_env) != 0) && at) {
		size_t n = strlen(tiled_spv), off = (size_t)(at - tiled_spv);
		char *wide = malloc(n + 1);
		if (wide) {
			memcpy(wide, tiled_spv, off);
			memcpy(wide + off, "_wide", 5);
			memcpy(wide + off + 5, tiled_spv + off + 6, n - off - 6 + 1);
			if (build_pipeline(wide, &g.rwide)) {
				g.rwide = NULL;              /* not there: the 16x32 build serves */
				g.err[0] = 0;
			}
			free(wide);
		}
	}
	if (cmd_create(&g.rcb) || cmd_create(&g.tcb)) return -1;
	g.rready = 1;
	return 0;
}

/* `kind`: 0 the graph's own buffers, 1 a buffer the host reads back, 2 one it writes.
 * Kinds 1 and 2 are always mapped (WRITE_BACK, cached); kind 0 follows the device. */
int xmx_buf_create_kind(unsigned long long bytes, int kind)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	int id = -1;
	for (int i = 0; i < MAX_RBUF; i++)
		if (!rbufs[i].live) { id = i; break; }
	if (id < 0) FAIL("out of buffer slots", 0);
	struct rbuf *rb = &rbufs[id];
	int unmapped = kind == 0 && g.unmapped;
	HRESULT hr = make_resource(bytes ? bytes : 4, unmapped ? MEM_DEVICE : MEM_WRITEBACK, 1, &rb->r);
	if (FAILED(hr)) FAILH(unmapped ? "CreateCommittedResource (device buffer)" : "CreateCommittedResource (mapped buffer)", hr);
	if (!unmapped) {
		hr = ID3D12Resource_Map(rb->r, 0, NULL, &rb->p);
		if (FAILED(hr)) { RELEASE(rb->r); memset(rb, 0, sizeof *rb); FAILH("Map (resident)", hr); }
	}
	rb->mapped = !unmapped;
	rb->addr = ID3D12Resource_GetGPUVirtualAddress(rb->r);
	rb->size = bytes ? bytes : 4;
	rb->state = D3D12_RESOURCE_STATE_COMMON;
	rb->live = 1;
	return id;
}

int xmx_buf_create(unsigned long long bytes) { return xmx_buf_create_kind(bytes, 0); }

int xmx_buf_host_visible(int id)
{
	return (id >= 0 && id < MAX_RBUF && rbufs[id].live && rbufs[id].mapped) ? 1 : 0;
}

/* A transfer runs on its own list so it can happen while the frame's graph is still being
 * recorded, which is when the weights arrive. */
static int stage_copy(int to_device, int id, const void *src, void *dst,
		      unsigned long long offset, unsigned long long bytes)
{
	if (id < 0 || id >= MAX_RBUF || !rbufs[id].live) FAIL("buffer is not live", 0);
	if (offset + bytes > rbufs[id].size) FAIL("transfer runs past the buffer", 0);
	if (!bytes) return 0;
	struct rbuf *rb = &rbufs[id];
	if (rb->mapped) {
		unsigned char *p = (unsigned char *)rb->p + offset;
		if (to_device) memcpy(p, src, (size_t)bytes);
		else memcpy(dst, p, (size_t)bytes);
		return 0;
	}
	if (to_device) {
		if (ensure(&g.up, bytes, MEM_UPLOAD, 0, "upload staging")) return -1;
		memcpy(g.up.p, src, (size_t)bytes);
	} else if (ensure(&g.down, bytes, MEM_READBACK, 0, "readback staging")) {
		return -1;
	}
	/* the recording's states are the recording's; a transfer list starts from COMMON
	 * and the copy promotes, so no barrier is needed here */
	if (cmd_begin(&g.tcb)) return -1;
	if (to_device) ID3D12GraphicsCommandList_CopyBufferRegion(g.tcb.list, rb->r, offset, g.up.r, 0, bytes);
	else ID3D12GraphicsCommandList_CopyBufferRegion(g.tcb.list, g.down.r, 0, rb->r, offset, bytes);
	if (execute(&g.tcb, "transfer")) return -1;
	if (!to_device) memcpy(dst, g.down.p, (size_t)bytes);
	return 0;
}

#define ZERO_CHUNK (16ull << 20)

/* Clearing a buffer the host cannot address is the device's job: copies from a zeroed
 * upload buffer, now, on the transfer list — never into the recording, where a fill would
 * run again on every replay and wipe a weight buffer's padding after the weights had been
 * uploaded (libxmx.c). */
int xmx_buf_zero(int id)
{
	if (id < 0 || id >= MAX_RBUF || !rbufs[id].live) FAIL("buffer is not live", 0);
	struct rbuf *rb = &rbufs[id];
	if (rb->mapped) {
		memset(rb->p, 0, (size_t)rb->size);
		return 0;
	}
	UINT64 chunk = rb->size < ZERO_CHUNK ? rb->size : ZERO_CHUNK;
	if (g.zeros.cap < chunk) {
		if (ensure(&g.zeros, chunk, MEM_UPLOAD, 0, "zero staging")) return -1;
		memset(g.zeros.p, 0, (size_t)chunk);
	}
	if (cmd_begin(&g.tcb)) return -1;
	for (UINT64 at = 0; at < rb->size; at += chunk) {
		UINT64 n = rb->size - at < chunk ? rb->size - at : chunk;
		ID3D12GraphicsCommandList_CopyBufferRegion(g.tcb.list, rb->r, at, g.zeros.r, 0, n);
	}
	return execute(&g.tcb, "fill");
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
	if (rbufs[id].mapped) ID3D12Resource_Unmap(rbufs[id].r, 0, NULL);
	RELEASE(rbufs[id].r);
	memset(&rbufs[id], 0, sizeof rbufs[id]);
	return 0;
}

static int live(int id) { return id >= 0 && id < MAX_RBUF && rbufs[id].live; }

/* -- recording -------------------------------------------------------------- */

/* Close a pass with a timestamp. Costs one query per pass and nothing when profiling is off. */
static void stamp(unsigned family, unsigned subkind)
{
	if (!g.prof || !g.qheap || g.prof_n >= MAX_STAMPS) return;
	stamp_kind[g.prof_n] = (unsigned char)(family * 32u + (subkind & 31u));
	ID3D12GraphicsCommandList_EndQuery(g.rcb.list, g.qheap, D3D12_QUERY_TYPE_TIMESTAMP, g.prof_n);
	g.prof_n++;
}

int xmx_begin(void)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.recording) FAIL("already recording", 0);
	if (cmd_begin(&g.rcb)) return -1;
	states_decayed();
	g.recording = 1;
	g.recorded = 0;
	g.rstep_n = 0;
	g.syncing = 1;
	g.prof_n = 0;
	if (g.prof && g.qheap) stamp(PK_START, 0);       /* the zero point every later stamp is measured from */
	return 0;
}

/* Under XMX_D3D12_STEP the parked lists have already run; they are not needed again. */
static void drop_steps(void)
{
	for (int i = 0; i < g.rstep_n; i++) cmd_destroy(&g.rsteps[i]);
	g.rstep_n = 0;
}

int xmx_abort(void)
{
	if (!g.recording) return 0;
	g.recording = 0;
	drop_steps();
	/* close and forget: the next begin resets the list */
	g.rcb.open = 0;
	ID3D12GraphicsCommandList_Close(g.rcb.list);
	return 0;
}

static const char *const family_name[] = { "gemm", "gemm_tiled", "gemm_staged", "unary", "row", "history", "copy", "start" };

/* XMX_D3D12_STEP: run what the recording holds since the last step — one pass — now, on
 * the list it was recorded into, say on stderr what it was and how long it took, park the
 * list for the graph, and go on recording on a fresh one. The buffers hold whatever they
 * hold at recording time, so the values computed are not a frame's, but the control flow
 * is, and a kernel that hangs or crawls is named before the graph ever runs whole. */
static int step_pass(const char *desc)
{
	if (!g.step) return 0;
	fprintf(stderr, "libd3dmx: pass %d %s ...", g.recorded, desc);
	fflush(stderr);
	double t0 = seconds();
	if (execute(&g.rcb, desc)) {
		fprintf(stderr, " FAILED: %s\n", g.err);
		return -1;
	}
	fprintf(stderr, " %.1f ms\n", (seconds() - t0) * 1e3);
	if (g.rstep_n == g.rstep_cap) {
		int cap = g.rstep_cap ? g.rstep_cap * 2 : 64;
		struct cmd *grown = realloc(g.rsteps, (size_t)cap * sizeof *grown);
		if (!grown) FAIL("step list allocation", 0);
		g.rsteps = grown;
		g.rstep_cap = cap;
	}
	g.rsteps[g.rstep_n++] = g.rcb;
	memset(&g.rcb, 0, sizeof g.rcb);
	if (cmd_create(&g.rcb) || cmd_begin(&g.rcb)) return -1;
	states_decayed();                 /* the executed list is complete: every buffer is COMMON again */
	return 0;
}

/* One global barrier between passes, suppressed by `xmx_sync(0)` over a run of dispatches
 * known to be independent. */
static void barrier(void)
{
	if (!g.syncing) return;
	uav_barrier(g.rcb.list);
}

int xmx_sync(int on)
{
	if (!g.recording) FAIL("not recording", 0);
	int was = g.syncing;
	g.syncing = on;
	if (on && !was) barrier();
	return 0;
}

int xmx_rec_copy(int source, int target, unsigned long long bytes,
		 unsigned long long source_offset, unsigned long long target_offset)
{
	if (!g.recording) FAIL("not recording", 0);
	if (!live(source) || !live(target)) FAIL("copy buffer is not live", 0);
	struct rbuf *a = &rbufs[source], *b = &rbufs[target];
	if (!bytes || ((bytes | source_offset | target_offset) & 3) ||
	    source_offset > a->size || bytes > a->size - source_offset ||
	    target_offset > b->size || bytes > b->size - target_offset)
		FAIL("copy range must be aligned and within both buffers", 0);
	if (source == target && source_offset < target_offset + bytes &&
	    target_offset < source_offset + bytes) FAIL("overlapping copy", 0);
	ID3D12GraphicsCommandList *list = g.rcb.list;
	/* the UAV barrier orders the copy after the dispatches that wrote its source; the
	 * transitions put both buffers in the copy states from wherever this recording left them */
	uav_barrier(list);
	transition(list, a, D3D12_RESOURCE_STATE_COPY_SOURCE);
	transition(list, b, D3D12_RESOURCE_STATE_COPY_DEST);
	ID3D12GraphicsCommandList_CopyBufferRegion(list, b->r, target_offset, a->r, source_offset, bytes);
	stamp(PK_COPY, 0);
	g.recorded++;
	if (g.step) {
		char desc[120];
		snprintf(desc, sizeof desc, "copy %llu bytes", bytes);
		return step_pass(desc);
	}
	return 0;
}

/* Record one pass: the operands as root UAVs (brought to the UAV state), the push block,
 * the dispatch in pieces, the barrier and the stamp. */
static int dispatch(ID3D12PipelineState *pso, struct push *p, const int ids[OPERANDS],
		    unsigned gx, unsigned gy, unsigned gz, int split_y, unsigned family, unsigned subkind)
{
	ID3D12GraphicsCommandList *list = g.rcb.list;
	for (unsigned i = 0; i < OPERANDS; i++)
		if (live(ids[i])) transition(list, &rbufs[ids[i]], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	ID3D12GraphicsCommandList_SetPipelineState(list, pso);
	for (unsigned i = 0; i < OPERANDS; i++)
		ID3D12GraphicsCommandList_SetComputeRootUnorderedAccessView(list, 1 + i,
			live(ids[i]) ? rbufs[ids[i]].addr : g.dummy.addr);
	dispatch_pieces(list, (uint32_t *)p, PUSH_WORDS, SPARE_WORD, gx, gy, gz, split_y);
	barrier();
	stamp(family, subkind);
	g.recorded++;
	if (g.step) {
		char desc[200];
		snprintf(desc, sizeof desc, "%s flags 0x%x m=%u n=%u k=%u batch=%u sa=%u sb=%u sc=%u groups %ux%ux%u",
			 family_name[family], p->flags, p->m, p->n, p->k, p->batch, p->sa, p->sb, p->sc, gx, gy, gz);
		return step_pass(desc);
	}
	return 0;
}

/* The QKV epilogue's other operands: K and V targets, the query scale, and the layout the
 * three targets share, (window, head, token, 32). */
struct qkv_targets { int k, v, scale; unsigned tokens, heads; };

/* Every GEMM goes through here, with libxmx's validation and routing (`record_gemm` in
 * libxmx.c) on a runtime with no staged kernel: the 16x32 register block where both extents
 * allow it, the 8x16 kernel for the rest. The fused modes pick their kernel as libxmx does
 * without matrix units: the window-gathered A takes the 16x32 block (gemm_portable.hlsl
 * gathers in its A loads), the QKV epilogue needs that block to be one head wide, the
 * compact head and the half copy take what the flags allow. The skip, the cosine, the half
 * copy and the three QKV targets ride in the fourth to sixth operands. */
static int record_gemm(int a, int b, int c, unsigned M, unsigned N, unsigned K,
		       unsigned batch, unsigned sa, unsigned sb, unsigned sc, unsigned bt,
		       unsigned lda, unsigned ldb, unsigned ldc,
		       unsigned oa, unsigned ob, unsigned oc,
		       int skip, int cosine, const uint32_t *window,
		       const struct qkv_targets *qkv, int half_copy, const uint32_t *window_a,
		       int pooled)
{
	if (!g.recording) FAIL("not recording", 0);
	if (!live(a) || !live(b) || !live(c)) FAIL("gemm operand is not a live buffer", 0);
	/* Element offsets are folded into the byte offsets, as libxmx folds them into the
	 * addresses: A and B are half, and C is float unless the epilogue narrows it. */
	struct push p;
	memset(&p, 0, sizeof p);
	p.a = (uint64_t)oa * 2; p.b = (uint64_t)ob * 2; p.c = (uint64_t)oc * ((bt & 0x1000u) ? 2 : 4);
	p.m = M; p.n = N; p.k = K; p.batch = batch;
	p.sa = sa; p.sb = sb; p.sc = sc; p.flags = bt;
	p.lda = lda; p.ldb = ldb; p.ldc = ldc;
	int ids[OPERANDS] = { a, b, c, -1, -1, -1, -1, -1 };
	if (skip >= 0) {
		if (!live(skip) || !live(cosine)) FAIL("residual operand is not a live buffer", 0);
		ids[3] = skip; ids[4] = cosine;
	}
	if (window) {
		p.image_h = window[0]; p.image_w = window[1];
		p.window_cols = window[2]; p.window_pad = window[3];
	}
	if (half_copy >= 0) {
		if (!live(half_copy)) FAIL("GEMM half copy is not a live buffer", 0);
		ids[3] = half_copy;
	}
	if (window_a) {
		/* A gathered from the image in window order (0x400000): the batch strides have
		 * no use with one batch, so they carry the geometry */
		p.sa = window_a[0]; p.sb = window_a[1]; p.sc = window_a[2];
		p.window_pad = window_a[3];
	}
	/* Bit 0x800000: block 0's window residual pooled and published in its own epilogue — c
	 * takes the published skip as half, the pool rides in the sixth operand, the slot only
	 * the QKV epilogue uses (gemm_portable.hlsl's 16x32 build: a block is two window rows). */
	if (bt & 0x800000u) {
		if (!window || N != 32u || (bt & ~0x8e0000u) || pooled < 0
		    || window[0] % 2u || window[1] % 2u || (window[3] >> 16) % 2u
		    || (window[3] & 0xffffu) % 2u)
			FAIL("a pooled window residual needs even geometry, 32 channels, no publish", 0);
		if (!live(pooled)) FAIL("pooled output is not a live buffer", 0);
		ids[5] = pooled;
	}
	if (qkv) {
		/* c is Q; K, V and the scale ride in the operands only the residual modes use */
		if (!live(qkv->k) || !live(qkv->v) || !live(qkv->scale))
			FAIL("QKV epilogue operand is not a live buffer", 0);
		ids[3] = qkv->k; ids[4] = qkv->v; ids[5] = qkv->scale;
		p.image_h = qkv->tokens; p.image_w = qkv->heads;
	}
	int tiled = M % g.tilem == 0 && N % g.tilen == 0 && K >= g.tiling;
	if (!g.portable_tiled && !(bt & 0x900000u)) tiled = 0;
	if (bt & 0x400000u) {
		if (!window_a || M % 64 || N % 32 || K % 32 || batch > 1 || (bt & 1u))
			FAIL("a window-gathered A needs its geometry, whole blocks and one batch", 0);
		if (M % g.tilem || N % g.tilen)
			FAIL("a window-gathered A without the staged kernel needs the 16x32 block", 0);
		tiled = 1;
	}
	if ((bt & 0x10000u) && (bt & 0x20000u))
		FAIL("compact head and fused residual are exclusive", 0);
	if (bt & 0x10000u) {
		if (N != 16 || ldc != 4 || (bt & 0x1f00u))
			FAIL("compact head requires N=16, ldc=4 and plain FP32 output", 0);
		tiled = 0;
	}
	/* the QKV epilogue normalises a head inside one group: the column block is one head */
	/* 0x2000 is the ViT's normalisation, 0x4000 a window-gathered A published on the way in */
	if ((bt & 0x100000u) && !(qkv && (bt & ~0x40e000u) == 0x100000u && tiled && g.tilen == 32))
		FAIL("QKV epilogue needs its targets, no other flag, and a 32-column block", 0);
	if ((bt & 0x4000u) && !(bt & 0x400000u))
		FAIL("an A published on the way in is a window-gathered one", 0);
	if ((bt & 0x200000u) && !(half_copy >= 0 && bt == 0x200000u))
		FAIL("a GEMM half copy needs its target and no other flag", 0);
	if ((bt & 0x800000u) && (!tiled || g.tilem != 16u || g.tilen != 32u))
		FAIL("a pooled window residual needs the portable 16x32 block (xmx_window_gather() is 0)", 0);
	unsigned gz = batch ? batch : 1;
	/* after the checks on `bt` above, which name every flag a caller may set: the packed A
	 * fetch (gemm_portable.hlsl, 0x8000000) is this runtime's own, not a caller's */
	if (g.portable_packed) p.flags |= 0x8000000u;
	/* the 16x64 build, for what its generic store takes: not the QKV epilogue or the pool
	 * (both need a 32-column block) nor the compact head */
	if (g.rwide && M % 16u == 0 && N % 64u == 0 && K >= g.tiling && !(bt & 0x910000u)) {
		g.wide_calls++;
		return dispatch(g.rwide, &p, ids, N / 64u, M / 16u, gz, 1, PK_TILED, bt);
	}
	if (tiled) return dispatch(g.rtiled, &p, ids, N / g.tilen, M / g.tilem, gz, 1, PK_TILED, bt);
	return dispatch(g.rgemm, &p, ids, (N + 15) / 16, (M + 7) / 8, gz, 1, PK_GEMM, bt);
}

int xmx_rec_gemm(int a, int b, int c, unsigned M, unsigned N, unsigned K, unsigned batch,
		 unsigned sa, unsigned sb, unsigned sc, unsigned bt,
		 unsigned lda, unsigned ldb, unsigned ldc,
		 unsigned oa, unsigned ob, unsigned oc)
{
	return record_gemm(a, b, c, M, N, K, batch, sa, sb, sc, bt,
			   lda, ldb, ldc, oa, ob, oc, -1, -1, NULL, NULL, -1, NULL, -1);
}

/* A projection whose epilogue adds `skip * cosine` before the publish (libxmx.c). A half
 * skip is the caller's to flag with 0x40000. */
int xmx_rec_gemm_residual(int a, int b, int c, int skip, int cosine,
			  unsigned M, unsigned N, unsigned K, unsigned flags)
{
	if (skip < 0 || cosine < 0) FAIL("invalid residual buffer", 0);
	return record_gemm(a, b, c, M, N, K, 1, M * K, K * N, M * N,
			   flags | 0x20000u, 0, 0, 0, 0, 0, 0, skip, cosine, NULL, NULL, -1, NULL, -1);
}

/* The same, for a window block's output projection, written back into the unpadded image:
 * `across` windows a row, `pad` = (top << 16) | left. */
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

/* The same for block 0, whose output has exactly two readers: the encoder, which pools it
 * 2x2 and publishes the pool, and the last block, which reads it published. Both come out of
 * this epilogue (gemm_portable.hlsl, 0x800000) — `skip_out` takes every value published, as
 * half, `pooled` the published pool — so the float32 output is never stored. */
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

/* The QKV projection with Q and K normalised and V published in its own epilogue; `vit`
 * the ViT's normalisation and query scale (nr_epilogue.hlsli, flag 0x2000). */
int xmx_rec_gemm_qkv(int a, int weight, int q, int k, int v, int scale,
		     unsigned M, unsigned channels, unsigned heads, unsigned tokens, unsigned vit)
{
	return record_gemm_qkv(a, weight, q, k, v, scale, M, channels, heads, tokens, NULL,
			       vit ? 0x2000u : 0u);
}

/* The same with A gathered from the image in window order: the partition folded in.
 * `image_mode` bit 0 a half image (0x8000), bit 1 the image published to E4M3 as it is
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

/* A plain GEMM whose float32 result is also stored published, as E4M3 halves, into
 * `half_copy`. */
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
	if (!live(a) || !live(c)) FAIL("unary operand is not a live buffer", 0);
	if (second >= 0 && !live(second)) FAIL("second unary output is not a live buffer", 0);
	struct push p;
	memset(&p, 0, sizeof p);
	p.m = n; p.n = channels; p.flags = kind; p.p0 = p0;
	p.batch = batch; p.sa = sa; p.sb = sb; p.sc = sc; p.k = k;
	/* a pass that writes two outputs finds the second in the fifth operand (u4), and bit
	 * 0x40000000 says it is there — HLSL has no null address to test, where the SPIR-V
	 * tests `pc.second != 0` (resident.hlsl, UPSAMPLE_ADD) */
	if (second >= 0) p.flags |= 0x40000000u;
	int ids[OPERANDS] = { a, b, c, d, second, -1, -1, -1 };
	return dispatch(g.runary, &p, ids, (n + 255) / 256, 1, 1, 0, PK_UNARY, kind);
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

/* Q, K and V prepared from the float32 projection in one row dispatch (attention.hlsl's
 * QKV_PREPARE): three independent group planes on y, Q/K/V out in b/c/d, the query scale in
 * the sixth operand (u5; libxmx passes its address in lda/ldb instead). */
int xmx_rec_qkv(int source, int q, int k, int v, int scale,
		unsigned rows, unsigned tokens, unsigned heads, unsigned vit)
{
	if (!g.recording) FAIL("not recording", 0);
	if (!rows || !tokens || !heads) FAIL("invalid QKV extent", 0);
	if (!live(source) || !live(q) || !live(k) || !live(v) || !live(scale)) FAIL("QKV operand is not live", 0);
	struct push p;
	memset(&p, 0, sizeof p);
	/* 0x80000: the ViT's normalisation and query scale (attention.hlsl) */
	p.m = rows; p.n = tokens; p.batch = heads;
	p.flags = 2u | 0x1000u | 0x20000u | (vit ? 0x80000u : 0u);
	int ids[OPERANDS] = { source, q, k, v, -1, scale, -1, -1 };
	return dispatch(g.rrow, &p, ids, (rows + 31) / 32, 3, 1, 0, PK_ROW, 2);
}

int xmx_rec_row(unsigned kind, int a, int b, int c, int d, unsigned rows, unsigned width,
		unsigned heads, unsigned scaled, unsigned stride, float cap)
{
	if (!g.recording) FAIL("not recording", 0);
	if (!live(a) || !live(c)) FAIL("row operand is not a live buffer", 0);
	struct push p;
	memset(&p, 0, sizeof p);
	p.m = rows; p.n = width; p.k = scaled; p.batch = heads; p.flags = kind;
	p.sa = stride; p.p0 = cap;
	int ids[OPERANDS] = { a, b, c, d, -1, -1, -1, -1 };
	/* A softmax over rows too wide to stage 32 at a time takes as many whole rows as fit in
	 * the pass's 8 KB of groupshared memory, `stride + 1` floats apart below the last 32, on
	 * the 256-lane build (attention.hlsl, softmax_rows) — or the 32-lane one, which has it
	 * too; anything wider keeps one row a lane. libxmx's routing, row for row. */
	unsigned row_stride = stride ? stride : width, groups = (rows + 31) / 32;
	int whole = (kind & 0xFFu) == 1u && (row_stride != width || row_stride > 64u)
		    && row_stride + 1u <= 2016u;
	if (whole) {
		unsigned per = 2016u / (row_stride + 1u);
		if (per > 32u) per = 32u;       /* the last 32 floats hold one reciprocal a row */
		groups = (rows + per - 1u) / per;
	}
	return dispatch(whole && g.rrows ? g.rrows : g.rrow, &p, ids, groups, 1, 1, 0, PK_ROW, kind);
}

int xmx_rec_history(int history, int motion, int out, unsigned pixels, unsigned channels,
		    unsigned height, unsigned width, unsigned absolute)
{
	if (!g.recording) FAIL("not recording", 0);
	if (!live(history) || !live(motion) || !live(out)) FAIL("history operand is not a live buffer", 0);
	struct push p;
	memset(&p, 0, sizeof p);
	p.m = pixels; p.n = channels; p.k = height; p.batch = width; p.flags = absolute;
	int ids[OPERANDS] = { history, motion, out, -1, -1, -1, -1, -1 };
	return dispatch(g.rhistory, &p, ids, (pixels + 63) / 64, 1, 1, 0, PK_HISTORY, absolute & 1u);
}

/* Window attention's QK^T, softmax and PV in one dispatch (window_attention_portable.hlsl):
 * built on first use, the merged-output module (which also does the head merge) its own
 * pipeline since DXIL has no specialisation. Q, K, out and V in u0..u3, the bias in u4. */
int xmx_window_init(const char *path, unsigned merged)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (merged > 1) FAIL("invalid window attention output mode", 0);
	if (g.rwindow[merged]) return 0;
	if (!merged) return build_pipeline(path, &g.rwindow[0]);
	/* The merged module is the same kernel's -DMERGED_OUTPUT=1 build: a name resolves to
	 * it, and an explicit DXIL file (a path with a directory) names its twin beside it,
	 * `<stem>_merged.dxil` — never the unmerged file built as if it were merged. */
	if (strchr(path, '/') || strchr(path, '\\')) {
		char twin[1200];
		const char *dot = strrchr(path, '.');
		int stem_len = dot && dot > strrchr(path, '/') ? (int)(dot - path) : (int)strlen(path);
		snprintf(twin, sizeof twin, "%.*s_merged%s", stem_len, path, dot && stem_len < (int)strlen(path) ? dot : "");
		return build_pipeline(twin, &g.rwindow[1]);
	}
	return build_pipeline("window_attention_portable_merged", &g.rwindow[1]);
}

int xmx_rec_window_attention(int q, int k, int v, int bias, int out,
			     unsigned batches, unsigned heads, unsigned merged)
{
	if (merged > 1 || !g.recording || !g.rwindow[merged])
		FAIL("window attention not ready for recording", 0);
	if (!batches || !heads || batches % heads) FAIL("invalid attention batch/head count", 0);
	if (!live(q) || !live(k) || !live(v) || !live(out) || (bias >= 0 && !live(bias)))
		FAIL("window attention operand is not a live buffer", 0);
	struct push p;
	memset(&p, 0, sizeof p);
	p.n = heads; p.batch = batches; p.flags = bias >= 0;
	int ids[OPERANDS] = { q, k, out, v, bias, -1, -1, -1 };
	/* one 256-lane group a window and head (its slices share K and V), the batch on y and
	 * z as the kernel reads it */
	return dispatch(g.rwindow[merged], &p, ids, 1, batches < 65535u ? batches : 65535u,
			1u + (batches - 1u) / 65535u, 0, PK_ROW, 3);
}

/* A feed-forward in one dispatch (ffn_fused_portable.hlsl): the input, the expand weights,
 * the output and the residual's skip in u0..u3, its cosine in u4, the projection weights
 * in u5. Eight 16-row blocks a 256-lane group on x, split in pieces; the feed-forward
 * group on y. The narrow blocks' shape (32 channels, 128 hidden) sets flag 0x800000, and
 * the group loads both weight matrices into groupshared memory once. */
int xmx_ffn_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rffn) return 0;
	free(g.rpaths[5]);
	if (!(g.rpaths[5] = nr_strdup(path))) FAIL("pipeline path allocation", 0);
	return build_pipeline(path, &g.rffn);
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
	if (!live(a) || !live(expand) || !live(projection) || !live(out)
	    || (residual && (!live(skip) || !live(cosine))))
		FAIL("fused feed-forward operand is not a live buffer", 0);
	struct push p;
	memset(&p, 0, sizeof p);
	p.m = M; p.n = cin; p.k = hidden; p.batch = groups;
	p.sa = cin * hidden; p.sb = hidden * 32u;
	p.ldc = groups > 1u ? groups * 32u : 0u;
	p.flags = flags | (residual ? 0x20000u : 0u) | (cin == 32u && hidden == 128u ? 0x800000u : 0u);
	int ids[OPERANDS] = { a, expand, out, residual ? skip : -1, residual ? cosine : -1, projection, -1, -1 };
	return dispatch(g.rffn, &p, ids, (M + 127u) / 128u, groups, 1, 0, PK_GEMM, 31);
}

/* Block 70's feed-forward with its input made in the same pass (ffn_fused_portable.hlsl,
 * flag 0x1000000): `source` the level above at half the extent (half), `skip` the
 * full-resolution skip (half), `sincos` the merge's sin then cos (u6), `cosine` the feed-
 * forward residual's. What UPSAMPLE_MERGE followed by `xmx_rec_ffn` with a float32 skip
 * writes, without the merge being stored. */
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
	if (!live(source) || !live(skip) || !live(sincos) || !live(expand) || !live(projection)
	    || !live(out) || !live(cosine))
		FAIL("merged feed-forward operand is not a live buffer", 0);
	struct push p;
	memset(&p, 0, sizeof p);
	p.m = M; p.n = 32u; p.k = 128u; p.batch = 1u;
	p.sa = 32u * 128u; p.sb = 128u * 32u; p.lda = width; p.ldb = source_width;
	p.flags = flags | 0x20000u | 0x800000u | 0x1000000u;
	/* p0 carries u6's byte offset: the whole table, 0 */
	int ids[OPERANDS] = { source, expand, out, skip, cosine, projection, sincos, -1 };
	return dispatch(g.rffn, &p, ids, (M + 127u) / 128u, 1, 1, 0, PK_GEMM, 31);
}

/* Block 0's feed-forward with its input made in the same pass (flag 0x2000000): the stem,
 * `features` (half, rows x 16) times `adapter` (16 x 32, u6), which `xmx_rec_gemm_dual`
 * wrote as float32 and half for this pass to read back. */
int xmx_rec_ffn_stem(int features, int adapter, int expand, int projection, int out,
		     int cosine, unsigned M, unsigned flags)
{
	if (!g.recording || !g.rffn) FAIL("fused feed-forward not ready for recording", 0);
	if (!M || M % 16u) FAIL("the stem feed-forward needs 16-row blocks", 0);
	if (flags & ~0x1f00u)
		FAIL("the stem feed-forward takes an epilogue and a half output only", 0);
	if (!live(features) || !live(adapter) || !live(expand) || !live(projection) || !live(out)
	    || !live(cosine))
		FAIL("stem feed-forward operand is not a live buffer", 0);
	struct push p;
	memset(&p, 0, sizeof p);
	p.m = M; p.n = 32u; p.k = 128u; p.batch = 1u;
	p.sa = 32u * 128u; p.sb = 128u * 32u;
	p.flags = flags | 0x20000u | 0x800000u | 0x2000000u;
	int ids[OPERANDS] = { features, expand, out, -1, cosine, projection, adapter, -1 };
	return dispatch(g.rffn, &p, ids, (M + 127u) / 128u, 1, 1, 0, PK_GEMM, 31);
}

/* A 32-channel window block's attention half in one pass a window (window_block_portable
 * .hlsl), built on first use: eight 32-lane slices and 28.25 KB a group. */
int xmx_window_block_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rblock) return 0;
	return build_pipeline(path, &g.rblock);
}

/* The QKV projection gathering its rows from `image`, window attention, and the output
 * projection with the residual into `target`; `image` is also the residual's skip. The
 * operands: image, qkv, target, bias in u0..u3, cosine in u4, the query scale in u5, the
 * output projection in u6 and the pool or the head's weights in u7. */
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
	if (!live(image) || !live(qkv) || !live(target) || !live(bias) || !live(projection)
	    || !live(cosine) || !live(scale) || ((flags & 0x1800000u) && !live(pooled)))
		FAIL("window block operand is not a live buffer", 0);
	struct push p;
	memset(&p, 0, sizeof p);
	p.m = windows; p.n = 32u; p.k = 32u; p.batch = 1u; p.flags = flags;
	p.image_h = height; p.image_w = width; p.window_cols = across; p.window_pad = pad;
	int ids[OPERANDS] = { image, qkv, target, bias, cosine, scale, projection,
			      (flags & 0x1800000u) ? pooled : -1 };
	/* windows on x, and past 65535 on y, as the kernel reads them (not split by pieces) */
	return dispatch(g.rblock, &p, ids, windows < 65535u ? windows : 65535u,
			(windows + 65534u) / 65535u, 1, 1, PK_ROW, 4);
}

int xmx_global_attention_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rglobal) return 0;
	return build_pipeline(path, &g.rglobal);
}

/* A bottleneck block's attention in one pass (global_attention_portable.hlsl): Q, K, V
 * (heads, rows, 32) half in u0, u1, u3, the merged (rows, heads * 32) half in u2 — QK^T,
 * the ViT's softmax, PV and the scaled head merge. A 256-lane group a head (y) and 64 query
 * rows (x, in pieces); the rows reach no further than the tokens' last 64-block. */
int xmx_rec_global_attention(int q, int k, int v, int merged, unsigned rows,
			     unsigned tokens, unsigned heads)
{
	if (!g.recording || !g.rglobal) FAIL("global attention not ready for recording", 0);
	if (!rows || rows % 16u || !tokens || tokens > rows || !heads
	    || (rows + 63u) / 64u != (tokens + 63u) / 64u)
		FAIL("global attention needs rows a multiple of 16 within the tokens' 64-block", 0);
	if (!live(q) || !live(k) || !live(v) || !live(merged))
		FAIL("global attention operand is not a live buffer", 0);
	struct push p;
	memset(&p, 0, sizeof p);
	p.m = rows; p.n = tokens; p.batch = heads;
	int ids[OPERANDS] = { q, k, merged, v, -1, -1, -1, -1 };
	return dispatch(g.rglobal, &p, ids, (rows + 63u) / 64u, heads, 1, 0, PK_ROW, 5);
}

int xmx_int8_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rint8) return 0;
	return build_pipeline(path, &g.rint8);
}

/* C = (A @ B^T) * a_scale[row] * b_scale[column]: A int8 M x K, B the weights transposed,
 * int8 N x K, the scales float32 (gemm_staged_int8_portable.hlsl, an 8x16 block a group, the
 * row groups on y in pieces). */
int xmx_rec_gemm_int8(int a, int b, int c, int a_scale, int b_scale,
		      unsigned M, unsigned N, unsigned K, unsigned flags)
{
	if (!g.recording || !g.rint8) FAIL("integer GEMM not ready for recording", 0);
	if (!M || !N || N % 32u || !K || K % 64u)
		FAIL("the integer GEMM needs N a multiple of 32 and K of 64", 0);
	if (flags) FAIL("the integer GEMM takes no flags yet", 0);
	if (!live(a) || !live(b) || !live(c) || !live(a_scale) || !live(b_scale))
		FAIL("integer GEMM operand is not a live buffer", 0);
	struct push p;
	memset(&p, 0, sizeof p);
	p.m = M; p.n = N; p.k = K; p.batch = 1u;
	int ids[OPERANDS] = { a, b, c, a_scale, b_scale, -1, -1, -1 };
	return dispatch(g.rint8, &p, ids, N / 16u, (M + 7u) / 8u, 1, 1, PK_GEMM, 30);
}

/* -- submission ---------------------------------------------------------------- */

/* Read the timestamps back and add each pass to its kind's running total. Called once the
 * fence has signalled, so the resolve has landed in the readback buffer. */
static void collect(unsigned stamps, const unsigned char *kinds)
{
	if (!g.prof || !g.qheap || !g.qread.p || stamps < 2) return;
	const uint64_t *ticks = (const uint64_t *)g.qread.p;
	for (unsigned i = 1; i < stamps; i++) {
		/* a wrapped counter is not a duration; -1 keeps the order for the caller */
		double ms = ticks[i] < ticks[i - 1] ? -1.0 : (double)(ticks[i] - ticks[i - 1]) * g.ns_per_tick * 1e-6;
		if (prof_each_n < MAX_STAMPS) {
			prof_each_kind[prof_each_n] = kinds[i];
			prof_each[prof_each_n++] = ms;
		}
		if (ms < 0) continue;
		prof_ms[kinds[i]] += ms;
		prof_hits[kinds[i]]++;
	}
}

/* The stamps of this recording, resolved into the readback buffer at the end of the list. */
static void resolve_stamps(void)
{
	if (g.prof && g.qheap && g.prof_n)
		ID3D12GraphicsCommandList_ResolveQueryData(g.rcb.list, g.qheap, D3D12_QUERY_TYPE_TIMESTAMP, 0, g.prof_n,
							   g.qread.r, 0);
}

int xmx_submit(void)
{
	if (!g.recording) FAIL("not recording", 0);
	g.recording = 0;
	resolve_stamps();
	int r = execute(&g.rcb, "resident submit");
	drop_steps();
	if (r) return -1;
	collect(g.prof_n, stamp_kind);
	return g.recorded;
}

/* A graph owns its list and allocator; later recording uses a fresh pair. Callers keep
 * referenced buffers alive until graph_destroy. */
int xmx_graph_capture(void)
{
	if (!g.recording) FAIL("not recording", 0);
	int id;
	for (id = 0; id < MAX_GRAPHS && graphs[id].live; id++);
	if (id == MAX_GRAPHS) FAIL("out of graph slots", 0);
	resolve_stamps();
	if (cmd_close(&g.rcb)) { g.recording = 0; return -1; }
	struct cmd replacement;
	memset(&replacement, 0, sizeof replacement);
	if (cmd_create(&replacement)) { g.recording = 0; return -1; }
	/* the lists in order: the ones stepping parked, then the current one */
	int n = g.rstep_n + 1;
	struct cmd *cmds = malloc((size_t)n * sizeof *cmds);
	if (!cmds) { g.recording = 0; FAIL("graph list allocation", 0); }
	if (g.rstep_n) memcpy(cmds, g.rsteps, (size_t)g.rstep_n * sizeof *cmds);
	cmds[g.rstep_n] = g.rcb;
	g.rstep_n = 0;
	graphs[id].cmds = cmds;
	graphs[id].ncmds = n;
	graphs[id].passes = g.recorded;
	graphs[id].stamps = g.prof_n;
	free(graphs[id].kinds);
	graphs[id].kinds = NULL;
	if (g.prof_n) {
		graphs[id].kinds = malloc(g.prof_n);
		if (graphs[id].kinds) memcpy(graphs[id].kinds, stamp_kind, g.prof_n);
		else graphs[id].stamps = 0;
	}
	graphs[id].live = 1;
	g.rcb = replacement;
	g.recording = 0;
	return id;
}

int xmx_graph_run(int id)
{
	if (g.recording) FAIL("cannot replay during recording", 0);
	if (id < 0 || id >= MAX_GRAPHS || !graphs[id].live) FAIL("graph is not live", 0);
	if (execute_lists(graphs[id].cmds, graphs[id].ncmds, "graph run")) return -1;
	if (graphs[id].kinds) collect(graphs[id].stamps, graphs[id].kinds);
	return graphs[id].passes;
}

int xmx_graph_destroy(int id)
{
	if (id < 0 || id >= MAX_GRAPHS || !graphs[id].live) return 0;
	for (int i = 0; i < graphs[id].ncmds; i++) cmd_destroy(&graphs[id].cmds[i]);
	free(graphs[id].cmds);
	free(graphs[id].kinds);
	memset(&graphs[id], 0, sizeof graphs[id]);
	return 0;
}

/* -- profiling ---------------------------------------------------------------- */

/* Off by default and free when off: `stamp()` returns on the first test. On, every recorded
 * pass gains one timestamp query, a command-list write and not a synchronisation point, so
 * the frame it measures is the frame that would have run. Returns -1 if the queue cannot
 * timestamp, and says so. */
int xmx_profile(int on)
{
	if (!on) { g.prof = 0; return 0; }
	if (!g.dev) FAIL("profiling needs an initialised device", 0);
	if (!g.qheap) {
		UINT64 frequency = 0;
		HRESULT hr = ID3D12CommandQueue_GetTimestampFrequency(g.q, &frequency);
		if (FAILED(hr) || !frequency) FAILH("this queue cannot write timestamps", hr);
		g.ns_per_tick = 1e9 / (double)frequency;
		D3D12_QUERY_HEAP_DESC qd;
		memset(&qd, 0, sizeof qd);
		qd.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		qd.Count = MAX_STAMPS;
		hr = ID3D12Device_CreateQueryHeap(g.dev, &qd, &IID_ID3D12QueryHeap, (void **)&g.qheap);
		if (FAILED(hr)) FAILH("CreateQueryHeap (timestamps)", hr);
		if (ensure(&g.qread, (UINT64)MAX_STAMPS * 8, MEM_READBACK, 0, "timestamp readback")) return -1;
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
