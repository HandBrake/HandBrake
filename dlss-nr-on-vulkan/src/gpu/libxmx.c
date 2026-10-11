/*
 * libxmx — a resident Vulkan compute context for the Xe2 XMX GEMM.
 *
 * The subprocess runner built a whole Vulkan instance, device and pipeline for every
 * matmul, which cost ~80 ms of fixed overhead per call and made any timing
 * meaningless. This keeps all of that alive across calls and reuses growable
 * host-visible buffers, so a dispatch costs a memcpy, a submit and a fence wait.
 *
 * Build: cc -O2 -shared -fPIC -I<vulkan headers> -o libxmx.so libxmx.c -lvulkan
 * On macOS the same file links against MoltenVK directly (-lMoltenVK): the Vulkan
 * loader there hides a portability driver until asked, and the compute path needs
 * no layer between it and the driver. `make` chooses.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define VK_NO_PROTOTYPES
#include <stddef.h>
#include <vulkan/vulkan.h>
#include "nr_shaders_embedded.h"
#include "xmx.h"
#include "../ref/nr_portable.h"
#if defined(XMX_NO_VULKAN_LINK) && !defined(_WIN32)
#include <dlfcn.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

/* Every Vulkan entry point this file calls is a pointer resolved through one
 * vkGetInstanceProcAddr: the linked library's (MoltenVK, or the loader) when libxmx makes
 * its own instance, and the adopter's when a host hands over its instance and device
 * (`xmx_adopt`) — so an adopted device is always driven through the very library that
 * created it, even when the host loaded a different copy than the one linked here. */
#define XMX_VK_GLOBAL_FUNCS(F) \
	F(vkCreateInstance) F(vkEnumerateInstanceExtensionProperties)
#define XMX_VK_INSTANCE_FUNCS(F) \
	F(vkAllocateCommandBuffers) F(vkAllocateDescriptorSets) F(vkAllocateMemory) \
	F(vkBeginCommandBuffer) F(vkBindBufferMemory) F(vkCmdBindDescriptorSets) \
	F(vkCmdBindPipeline) F(vkCmdCopyBuffer) F(vkCmdDispatch) F(vkCmdFillBuffer) \
	F(vkCmdPipelineBarrier) F(vkCmdPushConstants) F(vkCmdResetQueryPool) \
	F(vkCmdWriteTimestamp) F(vkCreateBuffer) F(vkCreateCommandPool) \
	F(vkCreateComputePipelines) F(vkCreateDescriptorPool) F(vkCreateDescriptorSetLayout) \
	F(vkCreateDevice) F(vkCreateFence) F(vkCreatePipelineLayout) F(vkCreateQueryPool) \
	F(vkCreateShaderModule) F(vkDestroyBuffer) F(vkDestroyCommandPool) \
	F(vkDestroyDescriptorPool) F(vkDestroyDescriptorSetLayout) F(vkDestroyDevice) \
	F(vkDestroyFence) F(vkDestroyInstance) F(vkDestroyPipeline) F(vkDestroyPipelineLayout) \
	F(vkDestroyQueryPool) F(vkDestroyShaderModule) F(vkDeviceWaitIdle) F(vkEndCommandBuffer) \
	F(vkEnumerateDeviceExtensionProperties) F(vkEnumeratePhysicalDevices) \
	F(vkFreeCommandBuffers) F(vkFreeMemory) F(vkGetBufferDeviceAddress) \
	F(vkGetBufferMemoryRequirements) F(vkGetDeviceProcAddr) F(vkGetDeviceQueue) \
	F(vkGetPhysicalDeviceFeatures2) \
	F(vkGetPhysicalDeviceMemoryProperties) \
	F(vkGetPhysicalDeviceProperties) F(vkGetPhysicalDeviceProperties2) \
	F(vkGetPhysicalDeviceQueueFamilyProperties) \
	F(vkGetQueryPoolResults) F(vkMapMemory) F(vkQueueSubmit) F(vkQueueWaitIdle) \
	F(vkResetCommandBuffer) F(vkResetFences) F(vkUnmapMemory) F(vkUpdateDescriptorSets) \
	F(vkWaitForFences)
#define XMX_VK_DECLARE(name) static PFN_##name name;
XMX_VK_GLOBAL_FUNCS(XMX_VK_DECLARE)
XMX_VK_INSTANCE_FUNCS(XMX_VK_DECLARE)
#undef XMX_VK_DECLARE
static PFN_vkGetInstanceProcAddr xmx_gipa;

#ifdef XMX_NO_VULKAN_LINK
/* The static build (libdlssnr) links no Vulkan library of its own: the host has one —
 * linked, or loaded — and an adopted instance brings its own entry point anyway. For a
 * device libxmx opens itself, look for vkGetInstanceProcAddr in the process first, then
 * load the loader (or MoltenVK) by name. */
static PFN_vkGetInstanceProcAddr linked_gipa(void)
{
#ifdef _WIN32
	HMODULE h = GetModuleHandleA("vulkan-1.dll");
	if (!h) h = LoadLibraryA("vulkan-1.dll");
	return h ? (PFN_vkGetInstanceProcAddr)(void (*)(void))GetProcAddress(h, "vkGetInstanceProcAddr") : NULL;
#else
	void *p = dlsym(RTLD_DEFAULT, "vkGetInstanceProcAddr");
	if (p) return (PFN_vkGetInstanceProcAddr)p;
	static const char *const names[] = { "libvulkan.so.1", "libvulkan.so", "libvulkan.1.dylib",
					      "libvulkan.dylib", "libMoltenVK.dylib" };
	for (size_t i = 0; i < sizeof names / sizeof *names; i++) {
		void *h = dlopen(names[i], RTLD_NOW | RTLD_GLOBAL);
		if (h && (p = dlsym(h, "vkGetInstanceProcAddr"))) return (PFN_vkGetInstanceProcAddr)p;
	}
	return NULL;
#endif
}
#else
/* The linked library's bootstrap symbol; the only one reached by name. */
extern VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *name);
static PFN_vkGetInstanceProcAddr linked_gipa(void) { return vkGetInstanceProcAddr; }
#endif

#ifdef _WIN32
#include <windows.h>
#endif

/* A lost device is recorded as well as described: it is the one failure after which nothing
 * on this device can succeed again, so a caller has to be able to tell it from the rest
 * without reading the message. It stays set. */
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
#define FAIL(msg, r) do { int fail_code = (int)(r); \
    if (fail_code == VK_ERROR_DEVICE_LOST) g.lost = 1; \
    sprintf_s(g.err, sizeof g.err, "%s (%d)", msg, fail_code); return -1; } while (0)
#else
#define FAIL(msg, r) do { int fail_code = (int)(r); \
	if (fail_code == VK_ERROR_DEVICE_LOST) g.lost = 1; \
	snprintf(g.err, sizeof g.err, "%s (%d)", msg, fail_code); return -1; } while (0)
#endif

struct buf { VkBuffer b; VkDeviceMemory m; void *p; VkDeviceSize cap; };

static struct {
	VkInstance inst; VkPhysicalDevice pd; VkDevice dev; VkQueue q; uint32_t qi;
	VkDescriptorSetLayout dsl; VkPipelineLayout pl; VkPipeline pipe;
	VkPipelineLayout plb; VkPipeline pipeb;
	VkDescriptorPool dpool; VkDescriptorSet set;
	VkCommandPool cpool; VkCommandBuffer cb; VkFence fence;
	struct buf A, B, C;
	/* resident path */
	VkPipelineLayout rpl; VkPipeline rgemm, rtiled, rstaged, runary, rrow, rhistory, rwindow[2];
	VkPipeline rffn;          /* the fused feed-forward, built on first use */
	/* the fused passes were built from their `_portable` twins, whose workgroups are
	 * shaped differently from the matrix kernels' (xmx_rec_window_attention, xmx_rec_ffn,
	 * xmx_rec_gemm_int8) */
	int rwindow_twin[2], rffn_twin, rint8_twin;
	/* the device's compute limits the fused passes' workgroups are checked against */
	uint32_t max_shared, max_invocations;
	/* the staged kernel on 32-row blocks, with a 32- and a 64-deep K step */
	VkPipeline rstaged32[2];
	VkPipeline rrows;         /* the whole-row softmax on 256 lanes */
	VkPipeline rint8;         /* the staged GEMM on the integer path, built on first use */
	VkPipeline rblock;        /* a 32-channel window block's attention half, likewise */
	VkPipeline rglobal;       /* a bottleneck block's attention in one pass, likewise */
	/* the portable GEMM's 16x64 build (family 12): on a device without matrix units, the
	 * plain, transposed and residual GEMMs whose N is whole 64-column blocks take it */
	VkPipeline rwide;
	unsigned wide_calls;
	/* portable path: the 16x32 build only where a 32-column block is required */
	unsigned portable_tiled;
	char *rpaths[13];
	unsigned specialize;
	unsigned tiling, tilem, tilen;
	int syncing;
	unsigned staging;
	unsigned staged_partial;  /* the staged kernel takes M that is not whole 64-row blocks */
	unsigned staged32;        /* M of 32 or fewer takes the 32-row staged build */
	unsigned staged32_calls;  /* how many GEMMs it has recorded: tests check the routing */
	VkCommandBuffer rcb; VkFence rfence; int recording, recorded, rready;
	/* transfers have their own command buffer: the weights are created while the
	 * frame's graph is being recorded, so a staged upload cannot borrow `rcb` */
	VkCommandBuffer tcb; VkFence tfence;
	/* GPU-side profiling. One timestamp after each recorded pass, so pass i costs
	 * ts[i+1]-ts[i]; the barrier between passes makes that attribution exact. */
	VkQueryPool qpool; unsigned prof, prof_n, qcount; float ts_period;
	int moltenvk;             /* VK_DRIVER_ID_MOLTENVK: a 4096-query timestamp pool at most (xmx_profile) */
	char name[256]; char err[256]; char memory[256]; char memory_read[256];
	int ready, lost, discrete, unmapped;
	int pin32;                /* pipelines require 32-lane subgroups (build_pipeline_spec) */
	int half_by_cast;         /* half_round's spelling, constant 1 on every pipeline (xmx_init) */
	int staged_packed;        /* raw 128-bit staged operand copies, constant 2 (detect_driver_modes) */
	int portable_packed;      /* the portable GEMM's 16-byte A fetch, constant 3 (detect_driver_modes) */
	int half_bits;            /* ... or the bits by hand, where both spellings are folded */
	int preserve16;           /* float16 subnormals kept: DenormPreserve 16 on every module (xmx_init) */
	int preserve64;           /* DenormPreserve 64 goes with it, which 32_BIT_ONLY requires (xmx_init) */
	int rte16, rte64;         /* RoundingModeRTE likewise: AMD truncates float16 undeclared (xmx_init) */
	/* XMX_PIPELINE_STATS=FILE: what the driver's compiler made of each pipeline, appended to
	 * FILE through VK_KHR_pipeline_executable_properties, where the device has it */
	FILE *stats;
	PFN_vkGetPipelineExecutablePropertiesKHR stats_props;
	PFN_vkGetPipelineExecutableStatisticsKHR stats_values;
	PFN_vkGetPipelineExecutableInternalRepresentationsKHR stats_ir;
	const char *ir_dir;       /* XMX_PIPELINE_IR: a folder for the compiled code, with the stats */
	/* `coopmat`: the device has VK_KHR_cooperative_matrix. `portable`: the GEMMs run on
	 * the plain multiply-add kernels instead — because the device has no matrix path
	 * (MoltenVK on Apple silicon), or because XMX_PORTABLE=1 asked for it here. */
	int coopmat, portable;
	/* `explicit_layout`: VK_KHR_workgroup_memory_explicit_layout is enabled on the device,
	 * which `gemm_staged.comp` needs since its operand tiles and output stage alias one
	 * shared-memory block. `staged_ok`: that kernel is built and may be chosen — matrix
	 * units, the extension, and not forced portable. Without it every shape goes to the
	 * 8x16 and 16x32 kernels, and a window-gathered A (which only it can load) is refused. */
	int explicit_layout, staged_ok;
	/* `int8`: the device has shaderInt8, 8-bit storage and the explicit layout's 8-bit
	 * access, which the integer staged GEMM needs; without them (or without matrix units)
	 * `xmx_int8_init` builds its portable twin, which reads the bytes as words. */
	int int8;
	struct buf stage;
	/* An adopted device belongs to the host: never destroyed here, and every submit on
	 * its queue is bracketed by the host's lock when one was given. */
	int adopted;
	void (*lock)(void *); void (*unlock)(void *); void *lock_ctx;
} g;

/* What `xmx_adopt` was handed, until `xmx_open` takes it. */
static struct {
	int set;
	VkInstance inst; VkPhysicalDevice pd; VkDevice dev; VkQueue q; uint32_t qi;
	int coopmat;
	PFN_vkGetInstanceProcAddr gipa;
	void (*lock)(void *); void (*unlock)(void *); void *ctx;
} adopt;

static void qlock(void) { if (g.lock) g.lock(g.lock_ctx); }
static void qunlock(void) { if (g.unlock) g.unlock(g.lock_ctx); }

#define MAX_SPECIALIZED 256
static struct {
	unsigned family, flags;
	VkPipeline pipeline;
} specialized[MAX_SPECIALIZED];
static unsigned specialized_count;

#define MAX_GRAPHS 128
static struct { VkCommandBuffer commands; int passes; unsigned stamps; unsigned char *kinds; }
	graphs[MAX_GRAPHS];

/* A pass's kind is `family * 32 + subkind`, so a unary or row pass is attributed to the
 * specific operation it runs rather than lumped in with its family. Families below. */
#define MAX_STAMPS 8192
#define PROF_KINDS 256
enum { PK_GEMM = 0, PK_TILED, PK_STAGED, PK_UNARY, PK_ROW, PK_HISTORY, PK_COPY, PK_START = 7 };
static unsigned char stamp_kind[MAX_STAMPS];
static double prof_ms[PROF_KINDS];
static unsigned prof_hits[PROF_KINDS];
/* Every pass's own duration, in recording order, since the last reset: what the totals
 * above sum away. `frame_profile.py --calls` pairs it with the calls that recorded them. */
static double prof_each[MAX_STAMPS];
static unsigned char prof_each_kind[MAX_STAMPS];   /* the kind each timed pass was stamped with */
static unsigned prof_each_n;

/* Device-resident buffers. The graph's activations live here between blocks instead
 * of being read back to the host after every GEMM; on a shared-memory APU the mapping
 * is HOST_CACHED, so the host can still write inputs and read outputs in place. */
#define MAX_RBUF 8192
struct rbuf { VkBuffer b; VkDeviceMemory m; void *p; VkDeviceAddress addr;
		 VkDeviceSize size; int live, mapped; };
static struct rbuf rbufs[MAX_RBUF];

struct push {
	uint64_t a, b, c, d;
	uint32_t m, n, k, batch, sa, sb, sc, flags;
	float p0, p1, p2, p3;
	uint32_t lda, ldb, ldc, spare;
	/* The fused residual (`gemm_resident.comp`, `residual_epilogue.glsl`): the skip
	 * travels in `d`, its per-channel cosine here, and a window-layout projection also
	 * needs the image it writes back into. Appended, so no earlier offset moves. */
	uint64_t residual_cos;
	uint32_t image_h, image_w, window_cols, window_pad;
	/* The QKV projection's epilogue (`qkv_epilogue.glsl`): the query's per-head scale.
	 * 128 bytes in all, the push-constant size every Vulkan device must support. */
	uint64_t qkv_scale;
};
_Static_assert(offsetof(struct push, lda) == 80, "attention QKV scale pointer ABI");
_Static_assert(offsetof(struct push, residual_cos) == 96, "residual epilogue ABI");
_Static_assert(offsetof(struct push, qkv_scale) == 120, "QKV epilogue ABI");
_Static_assert(offsetof(struct push, p0) == 64, "the merged feed-forward reads p0-p1 as an address");
_Static_assert(sizeof(struct push) == 128, "push block matches the GEMM shaders");

const char *xmx_error(void) { return g.err; }
int xmx_device_lost(void) { return g.lost; }
/* Asked before the first buffer exists — which is when the daemon logs it — the answer is
 * still knowable: run the same choice against every type the device has. */
static uint32_t memtype(uint32_t bits, VkMemoryPropertyFlags want, int host_read);
const char *xmx_memory(void)
{
	static char both[600];
	if (!g.ready) return "not initialised";
	if (!g.memory[0])
		memtype(~0u, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0);
	if (!g.memory_read[0])
		memtype(~0u, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1);
	if (!g.memory[0]) return "no host-visible memory type";
    
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    sprintf_s(both, sizeof both, "%s%s%s", g.memory,
         g.memory_read[0] ? "; " : "", g.memory_read);
#else
	snprintf(both, sizeof both, "%s%s%s", g.memory,
		 g.memory_read[0] ? "; " : "", g.memory_read);
#endif

	return both;
}
const char *xmx_device(void) { return g.name; }

/* Where the operands live, which is not the same question on the two kinds of GPU.
 *
 * On this shared-memory APU, HOST_CACHED first: memoryTypes[1] is
 * DEVICE_LOCAL|HOST_VISIBLE|HOST_COHERENT and memoryTypes[2] is the same plus
 * HOST_CACHED, and taking the first match landed on the uncached one, where reading the
 * result back ran at ~80 MB/s and buried a 1.35 TFLOP/s kernel: a 147456x32x128 GEMM
 * spent 1073 ms moving 85 MB.
 *
 * On a discrete GPU that preference is a trap. HOST_CACHED there means system memory, so
 * every operand would be read across PCIe while the card's own memory sits unused —
 * consistent with an Arc B580 measuring 50-141 GFLOP/s on shapes this iGPU runs at
 * 1027-3470. So device-local first there, which resizable BAR makes host-visible as well.
 * With the BAR unresized that window is 256 MB, far under the buffers a frame needs, and
 * preferring it would turn a slow run into a failed allocation — hence the heap-size floor,
 * which sends such a card back to system memory. Untested: there is no discrete GPU on the
 * machine this was written on. */
#define HOST_VISIBLE_VRAM_FLOOR (1024ull * 1024 * 1024)

/* Two ways to reach the card's memory, and the second one is not optional.
 *
 * Where the whole of VRAM is host-visible — a shared-memory APU, or a discrete card with
 * resizable BAR — the graph's buffers are mapped and the host writes into them directly.
 * Where it is not, a mapped buffer can only be system memory, which is the 50x trap
 * (`notes/phase63`). Then the graph's buffers are device-local and unmapped, and the host
 * reaches them through copies.
 *
 * `XMX_STAGING=1` forces the second path on hardware that would take the first. That is
 * how it is tested here: correctness does not depend on where the memory is, so the iGPU
 * can run the code a card without resizable BAR would run. */
static int want_unmapped(void)
{
	const char *forced = getenv("XMX_STAGING");
	if (forced && *forced) return atoi(forced) != 0;
	if (!g.discrete) return 0;
	VkPhysicalDeviceMemoryProperties mp;
	vkGetPhysicalDeviceMemoryProperties(g.pd, &mp);
	const VkMemoryPropertyFlags want = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
					 | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
		if ((mp.memoryTypes[i].propertyFlags & want) == want
		    && mp.memoryHeaps[mp.memoryTypes[i].heapIndex].size >= HOST_VISIBLE_VRAM_FLOOR)
			return 0;                      /* resizable BAR: map it and write in place */
	return 1;
}

static void note_memory(const VkPhysicalDeviceMemoryProperties *mp, uint32_t type, int host_read);

/* Device-local for the graph, host-visibility not required. */
static uint32_t memtype_device(uint32_t bits)
{
	VkPhysicalDeviceMemoryProperties mp;
	vkGetPhysicalDeviceMemoryProperties(g.pd, &mp);
	for (uint32_t pass = 0; pass < 2; pass++)
		for (uint32_t i = 0; i < mp.memoryTypeCount; i++) {
			VkMemoryPropertyFlags f = mp.memoryTypes[i].propertyFlags;
			if (!(bits & (1u << i))) continue;
			if (!(f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) continue;
			/* first pass prefers memory the host cannot see, which on a discrete
			 * card is the card's own and on this APU does not exist */
			if (!pass && (f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) continue;
			note_memory(&mp, i, 0);
			return i;
		}
	return UINT32_MAX;
}

/* Say where the buffers went, in one line, because the answer decides everything about
 * this machine's speed and nobody can see it from outside: on a discrete card, operands in
 * system memory are read across PCIe, and that is what an unresized BAR leaves us with. */
static void note_memory(const VkPhysicalDeviceMemoryProperties *mp, uint32_t type, int host_read)
{
	VkMemoryPropertyFlags f = mp->memoryTypes[type].propertyFlags;
	double heap = (double)mp->memoryHeaps[mp->memoryTypes[type].heapIndex].size / (1 << 30);
	const char *where = (!host_read && g.unmapped)
		? ((f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		   ? "card memory, unmapped by choice (staging forced)"
		   : "card memory, not host-visible: the host reaches it by copies")
		: host_read
		? ((f & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
		   ? "readback cached"
		   : "READBACK UNCACHED - the host reads the head with a stride, and on a discrete "
		     "card that is a strided read across PCIe")
		: !g.discrete ? "shared memory (one pool)"
		: (f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		? "card memory"
		: "SYSTEM MEMORY ACROSS PCIE - resizable BAR is off, or its window is under 1 GiB";
	char *slot = host_read ? g.memory_read : g.memory;
	size_t room = host_read ? sizeof g.memory_read : sizeof g.memory;

#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    sprintf_s(slot, room, "%s: type %u, heap %.1f GiB,%s%s%s%s", where, type, heap,
         (f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ? " DEVICE_LOCAL" : "",
         (f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? " HOST_VISIBLE" : "",
         (f & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ? " HOST_COHERENT" : "",
         (f & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) ? " HOST_CACHED" : "");
#else
	snprintf(slot, room, "%s: type %u, heap %.1f GiB,%s%s%s%s", where, type, heap,
		 (f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ? " DEVICE_LOCAL" : "",
		 (f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? " HOST_VISIBLE" : "",
		 (f & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ? " HOST_COHERENT" : "",
		 (f & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) ? " HOST_CACHED" : "");
#endif
}
static uint32_t memtype(uint32_t bits, VkMemoryPropertyFlags want, int host_read)
{
	VkPhysicalDeviceMemoryProperties mp;
	vkGetPhysicalDeviceMemoryProperties(g.pd, &mp);
	/* A buffer the host reads is the exception to the rule above. The head comes back as
	 * four of every sixteen floats — a strided read — and uncached memory serves that at a
	 * fraction of its streaming rate, which on a discrete card is a fraction of PCIe. The
	 * device writes it once; the host reads it once. Cached wins that trade even when the
	 * cached memory is on the other side of the bus. */
	const VkMemoryPropertyFlags prefer[3] = {
		(host_read || !g.discrete) ? VK_MEMORY_PROPERTY_HOST_CACHED_BIT
					   : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		(host_read || !g.discrete) ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
					   : VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
		0 };
	for (uint32_t pass = 0; pass < 3; pass++) {
		VkMemoryPropertyFlags need = want | prefer[pass];
		for (uint32_t i = 0; i < mp.memoryTypeCount; i++) {
			if (!(bits & (1u << i))) continue;
			if ((mp.memoryTypes[i].propertyFlags & need) != need) continue;
			if (g.discrete && !host_read && (need & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			    && mp.memoryHeaps[mp.memoryTypes[i].heapIndex].size < HOST_VISIBLE_VRAM_FLOOR)
				continue;
			note_memory(&mp, i, host_read);
			return i;
		}
	}
	return UINT32_MAX;
}

/* The compiled-in module of that name, or NULL. `name` is a bare file name. */
static const struct nr_embedded_shader *embedded_shader(const char *name)
{
#ifdef NR_EMBEDDED_SHADERS
	for (size_t i = 0; i < nr_embedded_shader_count; i++)
		if (!strcmp(nr_embedded_shaders[i].name, name)) return &nr_embedded_shaders[i];
#endif
	(void)name;
	return NULL;
}

size_t xmx_embedded_shader(const char *name)
{
	const struct nr_embedded_shader *e = name ? embedded_shader(name) : NULL;
	return e ? e->size : 0;
}

/* The SPIR-V for `spv_path`: a bare name is the embedded module (nr_shaders_embedded.h);
 * a path opens that file; a path whose file is missing falls back to the embedded module
 * of the same base name. `*owned` is what to free, if anything. */
static const void *shader_code(const char *spv_path, size_t *len, void **owned)
{
	*owned = NULL;
	const char *base = spv_path;
	for (const char *q = spv_path; *q; q++)
		if (*q == '/' || *q == '\\') base = q + 1;
	const struct nr_embedded_shader *e = embedded_shader(base);
	if (e && base == spv_path) { *len = e->size; return e->data; }
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
	FILE *f = NULL;
	fopen_s(&f, spv_path, "rb");
#else
	FILE *f = fopen(spv_path, "rb");
#endif
	if (!f) {
		if (e) { *len = e->size; return e->data; }
		return NULL;
	}
	fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
	void *code = malloc(n > 0 ? (size_t)n : 1);
	if (!code || n <= 0 || fread(code, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(code); return NULL; }
	fclose(f);
	*owned = code;
	*len = (size_t)n;
	return code;
}


/* Set from another thread to abandon a build in progress; see xmx.h. A plain flag: the
 * build reads it between pipelines and only ever cares whether it is set. */
static volatile int g_cancel;

void xmx_cancel(int on) { g_cancel = on ? 1 : 0; }
int xmx_cancelled(void) { return g_cancel; }

/* One float-controls execution mode to declare: the mode itself, the capability it needs,
 * and the opposite mode, whose presence in a module means it has already chosen. */
struct float_mode { uint32_t mode, cap, opposite; };

/* `words` with each mode in `modes` declared on every entry point for each width in `widths`,
 * or NULL when there is nothing to add. A mode the module already settles for one of those
 * widths -- either way round -- is left to it (src/bench/denorm_mode.py's copies do that), and
 * SPIR-V before 1.4 would need SPV_KHR_float_controls, which this does not add. The caller
 * frees the result.
 *
 * `widths` is 16 alone where the driver reports that width independent, and 16 and 64 together
 * under VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_32_BIT_ONLY, which requires one mode across
 * every width other than 32 and would make a 16-only module invalid
 * (VUID-RuntimeSpirv-denormBehaviorIndependence-06289, and -06291 for the rounding mode).
 * Declaring 64 needs no Float64 capability, since no 64-bit type appears; it is the execution
 * mode's literal operand. */
static uint32_t *declare_float_controls(const uint32_t *words, size_t n, size_t *out_n,
					const uint32_t *widths, size_t n_widths,
					const struct float_mode *modes, size_t n_modes)
{
	enum { CAPABILITY = 17, ENTRY_POINT = 15, EXECUTION_MODE = 16, EXECUTION_MODE_ID = 331 };
	if (n < 5 || words[0] != 0x07230203u || words[1] < 0x00010400u) return NULL;
	if (n_modes > 4) return NULL;
	int wanted[4], has_cap[4];
	for (size_t k = 0; k < n_modes; k++) { wanted[k] = 1; has_cap[k] = 0; }
	size_t cap_end = 0, mode_end = 0, entries = 0;
	for (size_t i = 5; i < n;) {
		uint32_t count = words[i] >> 16, op = words[i] & 0xffffu;
		if (count == 0 || i + count > n) return NULL;
		if (op == CAPABILITY) {
			cap_end = i + count;
			for (size_t k = 0; k < n_modes; k++)
				if (count > 1 && words[i + 1] == modes[k].cap) has_cap[k] = 1;
		}
		if (op == ENTRY_POINT) entries++;
		if (op == ENTRY_POINT || op == EXECUTION_MODE || op == EXECUTION_MODE_ID) mode_end = i + count;
		if (op == EXECUTION_MODE && count >= 4)
			for (size_t k = 0; k < n_modes; k++)
				if (words[i + 2] == modes[k].mode || words[i + 2] == modes[k].opposite)
					for (size_t w = 0; w < n_widths; w++)
						if (words[i + 3] == widths[w]) wanted[k] = 0;
		i += count;
	}
	if (!cap_end || !mode_end || !entries || cap_end > mode_end) return NULL;
	size_t n_add = 0, n_caps = 0;
	for (size_t k = 0; k < n_modes; k++)
		if (wanted[k]) { n_add++; if (!has_cap[k]) n_caps++; }
	if (!n_add) return NULL;
	size_t m = n + 2 * n_caps + 4 * entries * n_widths * n_add;
	uint32_t *out = malloc(m * sizeof *out);
	if (!out) return NULL;
	size_t o = 0;
	memcpy(out, words, cap_end * sizeof *out); o = cap_end;
	for (size_t k = 0; k < n_modes; k++)
		if (wanted[k] && !has_cap[k]) {
			out[o++] = (2u << 16) | CAPABILITY; out[o++] = modes[k].cap;
		}
	memcpy(out + o, words + cap_end, (mode_end - cap_end) * sizeof *out); o += mode_end - cap_end;
	for (size_t i = 5; i < n;) {
		uint32_t count = words[i] >> 16;
		if ((words[i] & 0xffffu) == ENTRY_POINT)
			for (size_t k = 0; k < n_modes; k++)
				if (wanted[k])
					for (size_t w = 0; w < n_widths; w++) {
						out[o++] = (4u << 16) | EXECUTION_MODE;
						out[o++] = words[i + 2];
						out[o++] = modes[k].mode; out[o++] = widths[w];
					}
		i += count;
	}
	memcpy(out + o, words + mode_end, (n - mode_end) * sizeof *out); o += n - mode_end;
	*out_n = o;
	return out;
}

static int build_pipeline_spec(const char *spv_path, VkPipelineLayout layout, VkPipeline *out,
			      const VkSpecializationInfo *specialization)
{
	/* Every pipeline this library builds comes through here, so one check bounds an
	 * abandoned open to whatever vkCreateComputePipelines is already inside. */
	if (g_cancel) FAIL("cancelled", 0);
	/* Every pipeline also gets constant 1, half_round's spelling (publish.glsl), constant 2,
	 * the staged kernel's packed operand copies (gemm_staged.comp), and constant 3, the
	 * portable GEMM's packed A fetch (gemm_portable.comp). All three are fixed in
	 * detect_driver_modes; a shader that does not declare one ignores its entry. */
	VkSpecializationMapEntry entries[6];
	unsigned char data[48];
	uint32_t count = 0, size = 0;
	if (specialization) {
		if (specialization->mapEntryCount > sizeof entries / sizeof *entries - 3
		    || specialization->dataSize > sizeof data - 3 * sizeof(VkBool32))
			FAIL("specialization too large", 0);
		count = specialization->mapEntryCount;
		size = (uint32_t)specialization->dataSize;
		memcpy(entries, specialization->pMapEntries, count * sizeof *entries);
		memcpy(data, specialization->pData, size);
	}
	uint32_t spelling = g.half_bits ? 2u : g.half_by_cast ? 1u : 0u;   /* publish.glsl's HALF_ROUND */
	entries[count++] = (VkSpecializationMapEntry){ .constantID = 1, .offset = size, .size = sizeof spelling };
	memcpy(data + size, &spelling, sizeof spelling);
	size += sizeof spelling;
	VkBool32 packed = g.staged_packed ? VK_TRUE : VK_FALSE;
	entries[count++] = (VkSpecializationMapEntry){ .constantID = 2, .offset = size, .size = sizeof packed };
	memcpy(data + size, &packed, sizeof packed);
	size += sizeof packed;
	VkBool32 ppacked = g.portable_packed ? VK_TRUE : VK_FALSE;
	entries[count++] = (VkSpecializationMapEntry){ .constantID = 3, .offset = size, .size = sizeof ppacked };
	memcpy(data + size, &ppacked, sizeof ppacked);
	size += sizeof ppacked;
	VkSpecializationInfo constants = { .mapEntryCount = count, .pMapEntries = entries,
					   .dataSize = size, .pData = data };
	size_t len = 0;
	void *owned = NULL;
	const void *code = shader_code(spv_path, &len, &owned);
	if (!code) FAIL("cannot open spv (no such file, and no embedded module of that name)", 0);
	/* Float16 subnormals are kept where the driver can be told to (xmx_init): Mesa flushes them
	 * in the cooperative-matrix GEMMs unless a mode is declared, Intel's Windows driver keeps
	 * them, and with the mode declared the two compute the same graph bit for bit
	 * (notes/phase71). The built shaders are not touched; the copy given to the driver is. */
	size_t patched_n = 0;
	enum { DENORM_PRESERVE = 4459, DENORM_FLUSH = 4460, CAP_DENORM_PRESERVE = 4464,
	       ROUND_RTE = 4462, ROUND_RTZ = 4463, CAP_ROUND_RTE = 4467 };
	const uint32_t widths[2] = { 16, 64 };
	struct float_mode modes[2];
	size_t n_modes = 0, n_widths = 1;
	if (g.preserve16) {
		modes[n_modes++] = (struct float_mode){ DENORM_PRESERVE, CAP_DENORM_PRESERVE,
							DENORM_FLUSH };
		if (g.preserve64) n_widths = 2;
	}
	if (g.rte16) {
		modes[n_modes++] = (struct float_mode){ ROUND_RTE, CAP_ROUND_RTE, ROUND_RTZ };
		if (g.rte64) n_widths = 2;
	}
	uint32_t *patched = n_modes
		? declare_float_controls(code, len / 4, &patched_n, widths, n_widths,
					 modes, n_modes) : NULL;
	VkShaderModuleCreateInfo smi = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
					 .codeSize = patched ? patched_n * 4 : len,
					 .pCode = patched ? patched : code };
	VkShaderModule sm;
	VkResult r = vkCreateShaderModule(g.dev, &smi, NULL, &sm);
	free(patched);
	free(owned);
	if (r) FAIL("shader module", r);
	/* Every kernel here is written for 32-lane subgroups — a row pass's workgroup is one
	 * subgroup, window attention's eight subgroups are a window's eight rows — and SPIR-V
	 * 1.6 lets the driver pick the width: ANV takes SIMD16 where SIMD32 would spill, and
	 * the row passes, which order their shared memory with subgroup barriers, then raced.
	 * Adding the ViT's softmax to attention.comp tipped its unspecialised build over, and
	 * a handful of cosine publishes came out unpublished. So the width is pinned.
	 * A driver that cannot pin it (g.pin32 unset, `xmx_init`) builds as before. */
	VkPipelineShaderStageRequiredSubgroupSizeCreateInfo width = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO,
		.requiredSubgroupSize = 32 };
	VkComputePipelineCreateInfo cpi = { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			   .pNext = (g.pin32 & 1) ? &width : NULL,
			   .flags = (g.pin32 & 2) ? VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT : 0,
			   .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = sm, .pName = "main",
			   .pSpecializationInfo = &constants }, .layout = layout };
	if (g.stats) cpi.flags |= VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR;
	if (g.stats && g.ir_dir) cpi.flags |= VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR;
	r = vkCreateComputePipelines(g.dev, VK_NULL_HANDLE, 1, &cpi, NULL, out);
	vkDestroyShaderModule(g.dev, sm, NULL);
	if (r) FAIL("pipeline", r);
	if (g.stats) {
		/* constant 0, where given, is the specialisation flags (resident_pipeline) */
		long flags = -1;
		for (uint32_t i = 0; specialization && i < specialization->mapEntryCount; i++)
			if (specialization->pMapEntries[i].constantID == 0
			    && specialization->pMapEntries[i].size == sizeof(uint32_t))
				flags = *(const uint32_t *)((const char *)specialization->pData
							    + specialization->pMapEntries[i].offset);
		const char *base = spv_path;
		for (const char *p = spv_path; *p; p++)
			if (*p == '/' || *p == '\\') base = p + 1;
		VkPipelineInfoKHR info = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR, .pipeline = *out };
		VkPipelineExecutablePropertiesKHR props[4];
		uint32_t executables = 4;
		for (uint32_t i = 0; i < executables; i++)
			props[i] = (VkPipelineExecutablePropertiesKHR){
				.sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR };
		if (g.stats_props(g.dev, &info, &executables, props) < 0) executables = 0;
		for (uint32_t e = 0; e < executables; e++) {
			VkPipelineExecutableInfoKHR which = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR,
				.pipeline = *out, .executableIndex = e };
			VkPipelineExecutableStatisticKHR values[64];
			uint32_t n = 64;
			for (uint32_t i = 0; i < n; i++)
				values[i] = (VkPipelineExecutableStatisticKHR){
					.sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR };
			if (g.stats_values(g.dev, &which, &n, values) < 0) n = 0;
			fprintf(g.stats, "%s\tflags=%ld\t%s\tsubgroup=%u", base, flags, props[e].name,
				props[e].subgroupSize);
			for (uint32_t i = 0; i < n; i++) {
				fprintf(g.stats, "\t%s=", values[i].name);
				switch (values[i].format) {
				case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
					fprintf(g.stats, "%u", values[i].value.b32); break;
				case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
					fprintf(g.stats, "%lld", (long long)values[i].value.i64); break;
				case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
					fprintf(g.stats, "%llu", (unsigned long long)values[i].value.u64); break;
				default:
					fprintf(g.stats, "%g", values[i].value.f64); break;
				}
			}
			fputc('\n', g.stats);
			/* XMX_PIPELINE_IR=DIR: whatever the driver shows of the compiled code, a file
			 * per representation, named after the shader and its flags */
			uint32_t reps = 0;
			if (!g.ir_dir || g.stats_ir(g.dev, &which, &reps, NULL) < 0) reps = 0;
			VkPipelineExecutableInternalRepresentationKHR *rep = calloc(reps ? reps : 1, sizeof *rep);
			for (uint32_t i = 0; rep && i < reps; i++)
				rep[i].sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INTERNAL_REPRESENTATION_KHR;
			if (rep && reps && g.stats_ir(g.dev, &which, &reps, rep) >= 0) {
				for (uint32_t i = 0; i < reps; i++)
					rep[i].pData = malloc(rep[i].dataSize ? rep[i].dataSize : 1);
				g.stats_ir(g.dev, &which, &reps, rep);
				for (uint32_t i = 0; i < reps; i++) {
					char file[1024], tag[VK_MAX_DESCRIPTION_SIZE];
					snprintf(tag, sizeof tag, "%s", rep[i].name);
					for (char *c = tag; *c; c++)
						if (!((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z')
						      || (*c >= '0' && *c <= '9'))) *c = '_';
					snprintf(file, sizeof file, "%s/%s-%ld-%u-%s.%s", g.ir_dir, base, flags, e,
						 tag, rep[i].isText ? "txt" : "bin");
					FILE *o = rep[i].pData ? fopen(file, "wb") : NULL;
					if (o) {
						size_t bytes = rep[i].dataSize;
						if (rep[i].isText && bytes && !((char *)rep[i].pData)[bytes - 1]) bytes--;
						fwrite(rep[i].pData, 1, bytes, o);
						fclose(o);
					}
					fprintf(g.stats, "\t\trepresentation %s (%s): %zu bytes\n", rep[i].name,
						rep[i].description, rep[i].dataSize);
					free(rep[i].pData);
				}
			}
			free(rep);
		}
		fflush(g.stats);
	}
	return 0;
}

static int build_pipeline(const char *path, VkPipelineLayout layout, VkPipeline *out)
{
	return build_pipeline_spec(path, layout, out, NULL);
}

/* Freeze operation flags before compilation: dead transpose/publish/width branches
 * otherwise contribute to register pressure even on dispatches that do not use them.
 * Keep the unspecialized path for same-buffer A/B measurements and shader overrides. */
/* The block size of the tiled pipeline, from the environment.
 *
 * These are not free parameters. They have to match the `-DRM`/`-DRN` that
 * `work/gemm_tiled.spv` was built with — 2 and 2, so 16x32 — because the dispatch
 * divides the extent by them while each workgroup writes the block the *shader* has.
 * Too small and the tiles overlap and run off the bottom edge, which
 * `cooperativeMatrixRobustBufferAccess = false` will not catch; zero divides by zero
 * outright, and `unsigned` turns a negative into something no extent is a multiple of,
 * so the knob silently does nothing. Anything but a positive number is refused. */
static unsigned block_size(const char *name, unsigned fallback)
{
	const char *value = getenv(name);
	if (!value) return fallback;
	int n = atoi(value);
	if (n > 0) return (unsigned)n;
	fprintf(stderr, "libxmx: %s=%s is not a positive block size; using %u\n",
		name, value, fallback);
	return fallback;
}

static int resident_pipeline(unsigned family, unsigned flags, VkPipeline fallback,
			     VkPipeline *out)
{
	/* families 5-7 — the fused feed-forward and the 32-row staged builds — are GEMMs and
	 * specialise with them; 8, the 256-lane row build, with the row passes */
	unsigned mask = family == 8 ? 4u
		      : (family < 3 || family >= 5) ? 1u : (family == 3 ? 2u : 4u);
	*out = fallback;
	if (!(g.specialize & mask)) return 0;
	for (unsigned i = 0; i < specialized_count; i++) {
		if (specialized[i].family == family && specialized[i].flags == flags) {
			*out = specialized[i].pipeline;
			return 0;
		}
	}
	if (specialized_count == MAX_SPECIALIZED) return 0;
	VkSpecializationMapEntry entry = { .constantID = 0, .offset = 0, .size = sizeof flags };
	VkSpecializationInfo info = { .mapEntryCount = 1, .pMapEntries = &entry,
				      .dataSize = sizeof flags, .pData = &flags };
	if (build_pipeline_spec(g.rpaths[family], g.rpl, out, &info)) return -1;
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

/* The row passes' 256-lane build, for the whole-row softmax. */
int xmx_rows_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rrows) return 0;
	free(g.rpaths[8]);
	if (!(g.rpaths[8] = nr_strdup(path))) FAIL("pipeline path allocation", 0);
	return build_pipeline(path, g.rpl, &g.rrows);
}

/* The 32-row staged builds, `shallow` with the default 32-deep K step and `deep` with 64.
 * Where the staged kernel is not built (no matrix units, or no explicit layout) nothing is
 * built and the routing never picks them: accepted, so a caller need not ask first. */
int xmx_staged32_init(const char *shallow, const char *deep)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rstaged32[0] || !g.staged_ok) return 0;
	const char *paths[2] = { shallow, deep };
	for (unsigned i = 0; i < 2; i++) {
		free(g.rpaths[6 + i]);
		if (!(g.rpaths[6 + i] = nr_strdup(paths[i]))) FAIL("pipeline path allocation", 0);
		if (build_pipeline(paths[i], g.rpl, &g.rstaged32[i])) {
			g.rstaged32[0] = VK_NULL_HANDLE;
			return -1;
		}
	}
	return 0;
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

/* `host_read` as in `xmx_buf_create_kind`: C is the result the host reads back, A and B are
 * operands it only writes. The plain GEMM path is the benchmark's path, so getting this
 * wrong would mis-measure the very card the rule above exists for. */
static int ensure(struct buf *b, VkDeviceSize size, int host_read)
{
	if (b->cap >= size)
		return 0;
	if (b->b) { vkUnmapMemory(g.dev, b->m); vkDestroyBuffer(g.dev, b->b, NULL); vkFreeMemory(g.dev, b->m, NULL); }
	VkBufferCreateInfo bi = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = size,
				  .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT };
	VkResult r = vkCreateBuffer(g.dev, &bi, NULL, &b->b);
	if (r) FAIL("vkCreateBuffer", r);
	VkMemoryRequirements mr;
	vkGetBufferMemoryRequirements(g.dev, b->b, &mr);
	uint32_t mt = memtype(mr.memoryTypeBits,
			      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, host_read);
	if (mt == UINT32_MAX) FAIL("no host-visible memory type", 0);
	VkMemoryAllocateInfo ai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				    .allocationSize = mr.size, .memoryTypeIndex = mt };
	r = vkAllocateMemory(g.dev, &ai, NULL, &b->m);
	if (r) FAIL("vkAllocateMemory", r);
	vkBindBufferMemory(g.dev, b->b, b->m, 0);
	r = vkMapMemory(g.dev, b->m, 0, size, 0, &b->p);
	if (r) FAIL("vkMapMemory", r);
	b->cap = size;
	return 0;
}

/* Whether an extension is in a list, so the device is asked only for what it has. */
static int has_extension(const VkExtensionProperties *list, uint32_t count, const char *name)
{
	for (uint32_t i = 0; i < count; i++)
		if (!strcmp(list[i].extensionName, name)) return 1;
	return 0;
}

/* The instance, the device and the queue — everything that decides which shaders can
 * run, and nothing that needs a shader. Split from `xmx_init` so a caller can ask
 * `xmx_coopmat()` before it chooses which SPIR-V to hand over. Idempotent. */
/* Resolve the entry points through `xmx_gipa`: the global ones before an instance
 * exists, the rest against it. */
static int resolve_global(void)
{
#define XMX_VK_LOAD(name) name = (PFN_##name)xmx_gipa(NULL, #name); \
	if (!name) FAIL("Vulkan library has no " #name, 0);
	XMX_VK_GLOBAL_FUNCS(XMX_VK_LOAD)
#undef XMX_VK_LOAD
	return 0;
}

static int resolve_instance(VkInstance inst)
{
#define XMX_VK_LOAD(name) name = (PFN_##name)xmx_gipa(inst, #name); \
	if (!name) FAIL("Vulkan instance has no " #name " (Vulkan 1.3 is needed)", 0);
	XMX_VK_INSTANCE_FUNCS(XMX_VK_LOAD)
#undef XMX_VK_LOAD
	return 0;
}

/* Share a host's Vulkan objects instead of creating an instance and a device. Call
 * before `xmx_open` (or after `xmx_close`); the next `xmx_open` takes them. The handles
 * are `void *` so a caller that binds this by name needs no Vulkan header.
 *
 *   inst, pd, dev   the host's VkInstance, VkPhysicalDevice and VkDevice. The device
 *                   must have Vulkan 1.3 and these features enabled: storageBuffer16BitAccess,
 *                   vulkanMemoryModel (+DeviceScope), shaderFloat16, bufferDeviceAddress,
 *                   scalarBlockLayout; VK_KHR_portability_subset where the device offers it.
 *   q, qi           a queue of family `qi`, which must support compute. If the host also
 *                   submits on `q`, it passes `lock`/`unlock` (called around every
 *                   vkQueueSubmit here, with `ctx`) and takes the same lock around its own
 *                   submits, presents and device-idle waits.
 *   coopmat         whether the host enabled VK_KHR_cooperative_matrix on `dev`.
 *   gipa            the host's vkGetInstanceProcAddr, the library that made `inst`.
 *
 * The host owns the objects: `xmx_close` releases everything libxmx made on them and
 * leaves them alone. The host must `xmx_close` before it destroys the device. */
int xmx_adopt(void *inst, void *pd, void *dev, void *q, unsigned qi, int coopmat, void *gipa,
	      void (*lock)(void *), void (*unlock)(void *), void *ctx)
{
	if (g.dev) FAIL("xmx_adopt: a device is open; xmx_close first", 0);
	if (!inst || !pd || !dev || !q || !gipa) FAIL("xmx_adopt: incomplete Vulkan objects", 0);
	adopt.set = 1;
	adopt.inst = (VkInstance)inst; adopt.pd = (VkPhysicalDevice)pd; adopt.dev = (VkDevice)dev;
	adopt.q = (VkQueue)q; adopt.qi = qi; adopt.coopmat = coopmat;
	adopt.gipa = (PFN_vkGetInstanceProcAddr)gipa;
	adopt.lock = lock; adopt.unlock = unlock; adopt.ctx = ctx;
	return 0;
}

/* 1 while an adopted device is open, 0 for libxmx's own, -1 when nothing is open. */
int xmx_adopted(void) { return g.dev ? g.adopted : -1; }

/* Two things about the driver that decide what the pipelines compile, read from the
 * physical device so an adopted one (xmx_adopt) gets the same answer as libxmx's own.
 *
 * half_round's spelling (publish.glsl) is the compiler's to decide, not ours: Mesa folds
 * `float(float16_t(x))` away and keeps packHalf2x16's round trip, and Intel's Windows
 * compiler folds the round trip and keeps the cast. Whichever is folded, every vendor
 * rounding point in the graph silently vanishes; through MoltenVK on an AMD or Intel GPU
 * both are folded, and the bits are rounded by hand. `XMX_HALF_ROUND=pack`, `cast` or `bits`
 * overrides, to measure another one on any driver.
 *
 * And float16 subnormals are kept wherever the driver can declare it. NVIDIA's tensor cores
 * keep them (notes/phase71), Mesa's undeclared default flushes them in the GEMMs, and without
 * one mode on every driver the same graph gives a different picture on each.
 * `XMX_DENORM16=driver` leaves the choice to the driver again, to measure what it does. */
static void detect_driver_modes(VkPhysicalDevice pd, uint32_t api_version)
{
	g.half_by_cast = 0;
	g.half_bits = 0;
	g.moltenvk = 0;
	if (api_version >= VK_API_VERSION_1_2) {
		VkPhysicalDeviceDriverProperties driver = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES };
		VkPhysicalDeviceProperties2 query = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &driver };
		vkGetPhysicalDeviceProperties2(pd, &query);
		/* Which spelling of half_round survives the compiler and rounds like numpy's
		 * float16. Mesa folds the cast away and keeps packHalf2x16's round trip;
		 * Intel's Windows compiler folds the round trip and keeps the cast.
		 *
		 * AMD keeps both, but its packHalf2x16 truncates towards zero and no
		 * declaration reaches it: the float-controls rounding mode governs the
		 * conversion instructions, while packHalf2x16 is a GLSL.std.450 instruction
		 * whose rounding is the implementation's. The cast is a conversion, so
		 * `RoundingModeRTE 16` above does reach it. Measured with
		 * src/bench/half_probe.py on a Radeon iGPU: undeclared, both spellings miss
		 * numpy on 183147 of 360704 values; with the mode declared the cast matches
		 * exactly and the round trip still misses the same 183147. Taking the round
		 * trip there biases every rounding point in the graph low. */
		g.half_by_cast = driver.driverID == VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS
			|| driver.driverID == VK_DRIVER_ID_AMD_PROPRIETARY
			|| driver.driverID == VK_DRIVER_ID_AMD_OPEN_SOURCE;
		g.moltenvk = driver.driverID == VK_DRIVER_ID_MOLTENVK;
		/* Under MoltenVK the Metal compiler of the GPU beneath decides, and on an Intel Mac's
		 * AMD or Intel GPU it folds both spellings to x (a Vega 64 and a UHD 630, macOS 15:
		 * 99.8 % of a million values unrounded either way), so the rounding is done on the
		 * bits there. Apple's keeps the round trip, as measured on an M3. */
		if (g.moltenvk && query.properties.vendorID != 0x106Bu) {
			g.half_by_cast = 0;
			g.half_bits = 1;
		}
	}
	const char *half = getenv("XMX_HALF_ROUND");
	if (half && *half) {
		if (!strcmp(half, "cast")) g.half_by_cast = 1, g.half_bits = 0;
		else if (!strcmp(half, "pack")) g.half_by_cast = 0, g.half_bits = 0;
		else if (!strcmp(half, "bits")) g.half_by_cast = 0, g.half_bits = 1;
		else fprintf(stderr, "libxmx: XMX_HALF_ROUND=%s is neither pack, cast nor bits; keeping %s\n",
			     half, g.half_bits ? "bits" : g.half_by_cast ? "cast" : "pack");
	}
	/* Raw 128-bit global/shared copies in the staged GEMM's loader, on every driver: 6-9 %
	 * of the graph on Intel's Windows driver, about 2 % at 720p and 1080p on Mesa and nothing
	 * at 320x320, the same bits on both (upstream, 2026-10-07). `XMX_STAGED_PACKED=0` keeps
	 * the half-vector loader, to compare them with identical SPIR-V. Fixed before any
	 * pipeline is created, for an adopted device as for libxmx's own. */
	g.staged_packed = 1;
	const char *packed = getenv("XMX_STAGED_PACKED");
	if (packed && *packed) {
		if (!strcmp(packed, "1")) g.staged_packed = 1;
		else if (!strcmp(packed, "0")) g.staged_packed = 0;
		else fprintf(stderr, "libxmx: XMX_STAGED_PACKED=%s is neither 0 nor 1; keeping %d\n",
			     packed, g.staged_packed);
	}
	/* The same idea in the portable GEMM — A fetched sixteen bytes at a time — is off by
	 * default: through MoltenVK on an M3 the compiler already combines the eight-byte fetches
	 * and the sixteen-byte path measured slower (HANDOFF, 2026-10-10). Kept to try on another
	 * device without matrix units; the bytes are the same either way. */
	g.portable_packed = 0;
	const char *ppacked = getenv("XMX_PORTABLE_PACKED");
	if (ppacked && *ppacked) {
		if (!strcmp(ppacked, "1")) g.portable_packed = 1;
		else if (!strcmp(ppacked, "0")) g.portable_packed = 0;
		else fprintf(stderr, "libxmx: XMX_PORTABLE_PACKED=%s is neither 0 nor 1; keeping %d\n",
			     ppacked, g.portable_packed);
	}
	g.preserve16 = 0;
	g.preserve64 = 0;
	g.rte16 = 0;
	g.rte64 = 0;
	if (api_version >= VK_API_VERSION_1_2) {
		VkPhysicalDeviceFloatControlsProperties controls = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES };
		VkPhysicalDeviceProperties2 query = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &controls };
		vkGetPhysicalDeviceProperties2(pd, &query);
		/* What a module may declare follows denormBehaviorIndependence
		 * (VUID-RuntimeSpirv-denormBehaviorIndependence-06289 and -06290):
		 *
		 *   ALL          every width is its own; declare 16 alone. ANV and Intel's
		 *                Windows driver report this.
		 *   32_BIT_ONLY  32 is its own and the rest share one mode, so 16 obliges 64 to
		 *                match. AMD's proprietary driver reports this, with preserve
		 *                available on both widths; declare the pair.
		 *   NONE         one mode for every width, so 16 would drag 32 in with it. That
		 *                changes the float32 arithmetic for the sake of the halves, so
		 *                leave the driver to it. MoltenVK reports this.
		 *
		 * Declaring 64 costs an execution mode per entry point and nothing else: no
		 * 64-bit type appears in these modules, so no Float64 capability comes with it. */
		const int ind = (int)controls.denormBehaviorIndependence;
		if (controls.shaderDenormPreserveFloat16 == VK_TRUE) {
			if (ind == VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL)
				g.preserve16 = 1;
			else if (ind == VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_32_BIT_ONLY
				 && controls.shaderDenormPreserveFloat64 == VK_TRUE)
				g.preserve16 = g.preserve64 = 1;
		}
	}
	if (api_version >= VK_API_VERSION_1_2) {
		VkPhysicalDeviceFloatControlsProperties controls = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES };
		VkPhysicalDeviceProperties2 query = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &controls };
		vkGetPhysicalDeviceProperties2(pd, &query);
		/* The same three cases as the denorm mode above, for the rounding mode. Left
		 * undeclared this is the driver's own: Mesa and Intel's Windows driver round
		 * float16 to nearest even, and AMD's proprietary driver truncates towards zero,
		 * so every rounding point in the graph comes out biased low there
		 * (src/bench/half_probe.py: both hardware spellings of half_round miss numpy's
		 * float16 on half the values, the hand-written one does not). The graph is
		 * written for round-to-nearest-even, which is what numpy and the vendor do. */
		const int ind = (int)controls.roundingModeIndependence;
		if (controls.shaderRoundingModeRTEFloat16 == VK_TRUE) {
			if (ind == VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL)
				g.rte16 = 1;
			else if (ind == VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_32_BIT_ONLY
				 && controls.shaderRoundingModeRTEFloat64 == VK_TRUE)
				g.rte16 = g.rte64 = 1;
		}
	}
	const char *round = getenv("XMX_ROUND16");
	if (round && !strcmp(round, "driver")) g.rte16 = g.rte64 = 0;
	else if (round && *round && strcmp(round, "rte"))
		fprintf(stderr, "libxmx: XMX_ROUND16=%s is neither rte nor driver; keeping %s\n",
			round, g.rte16 ? "rte" : "driver");
	if (!g.rte16 && !(round && !strcmp(round, "driver")))
		fprintf(stderr, "libxmx: %s cannot be told to round float16 to nearest even; its own "
			"default decides, and a driver that truncates biases the graph low\n", g.name);
	const char *denorm = getenv("XMX_DENORM16");
	if (denorm && !strcmp(denorm, "driver")) g.preserve16 = g.preserve64 = 0;
	else if (denorm && *denorm && strcmp(denorm, "preserve"))
		fprintf(stderr, "libxmx: XMX_DENORM16=%s is neither preserve nor driver; keeping %s\n",
			denorm, g.preserve16 ? "preserve" : "driver");
	if (!g.preserve16 && !(denorm && !strcmp(denorm, "driver")))
		fprintf(stderr, "libxmx: %s reports one denorm mode for every float width, so float16 "
			"subnormals cannot be declared kept without also changing float32; "
			"its own default decides, and the picture may differ from other drivers'\n", g.name);
}

static int open_adopted(void)
{
	xmx_gipa = adopt.gipa;
	if (resolve_global() || resolve_instance(adopt.inst)) return -1;
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(adopt.pd, &props);
	if (props.apiVersion < VK_API_VERSION_1_3)
		FAIL("adopted device is below Vulkan 1.3", (int)props.apiVersion);
	g.max_shared = props.limits.maxComputeSharedMemorySize;
	g.max_invocations = props.limits.maxComputeWorkGroupInvocations;
	uint32_t nq = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(adopt.pd, &nq, NULL);
	VkQueueFamilyProperties *qf = calloc(nq ? nq : 1, sizeof *qf);
	vkGetPhysicalDeviceQueueFamilyProperties(adopt.pd, &nq, qf);
	int compute = adopt.qi < nq && (qf[adopt.qi].queueFlags & VK_QUEUE_COMPUTE_BIT);
	free(qf);
	if (!compute) FAIL("adopted queue family has no compute", (int)adopt.qi);

	/* deviceName is as wide as g.name, so the suffix has to be reserved:
	   bound the device name to what is left over, or a long one pushes
	   " (shared)" out of the buffer -- losing exactly the part worth
	   keeping, and tripping sprintf_s's truncation handler on Windows. */
	const int name_max = (int)(sizeof g.name - sizeof " (shared)");
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
	sprintf_s(g.name, sizeof g.name, "%.*s (shared)", name_max, props.deviceName);
#else
	snprintf(g.name, sizeof g.name, "%.*s (shared)", name_max, props.deviceName);
#endif
	g.discrete = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	g.coopmat = (adopt.coopmat & XMX_ADOPT_COOPMAT) != 0;
	g.explicit_layout = g.coopmat && (adopt.coopmat & XMX_ADOPT_EXPLICIT_LAYOUT) != 0;
	const char *forced = getenv("XMX_PORTABLE");
	g.portable = !g.coopmat || (forced && *forced && atoi(forced) != 0);
	g.staged_ok = g.coopmat && !g.portable && g.explicit_layout;
	detect_driver_modes(adopt.pd, props.apiVersion);
	g.inst = adopt.inst; g.pd = adopt.pd; g.q = adopt.q; g.qi = adopt.qi;
	g.lock = adopt.lock; g.unlock = adopt.unlock; g.lock_ctx = adopt.ctx;
	g.adopted = 1;
	g.dev = adopt.dev;
	adopt.set = 0;
	return 0;
}

int xmx_open(void)
{
	if (g.dev) return 0;
	if (adopt.set) return open_adopted();
	xmx_gipa = linked_gipa();
	if (!xmx_gipa) FAIL("no Vulkan library in the process (vkGetInstanceProcAddr not found)", 0);
	if (resolve_global()) return -1;
	uint32_t nie = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &nie, NULL);
	VkExtensionProperties *ie = calloc(nie ? nie : 1, sizeof *ie);
	vkEnumerateInstanceExtensionProperties(NULL, &nie, ie);
	/* MoltenVK is a "portability" driver: behind the Vulkan loader on macOS it is
	 * invisible until the instance enables this and sets the flag. Linked directly, or
	 * on Linux, the extension is absent and nothing is asked for. */
	int portability = has_extension(ie, nie, "VK_KHR_portability_enumeration");
	free(ie);
	const char *iext[] = { "VK_KHR_portability_enumeration" };
	VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				  .pApplicationName = "libxmx", .apiVersion = VK_API_VERSION_1_3 };
	VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app,
				     .flags = portability ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0u,
				     .enabledExtensionCount = portability ? 1u : 0u,
				     .ppEnabledExtensionNames = iext };
	VkResult r = vkCreateInstance(&ici, NULL, &g.inst);
	if (r) FAIL("vkCreateInstance", r);
	if (resolve_instance(g.inst)) return -1;

	uint32_t n = 0;
	vkEnumeratePhysicalDevices(g.inst, &n, NULL);
	if (!n) FAIL("no physical device", 0);
	VkPhysicalDevice *pds = calloc(n, sizeof *pds);
	vkEnumeratePhysicalDevices(g.inst, &n, pds);
	g.pd = pds[0];
	free(pds);
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(g.pd, &props);
	g.max_shared = props.limits.maxComputeSharedMemorySize;
	g.max_invocations = props.limits.maxComputeWorkGroupInvocations;
    
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    sprintf_s(g.name, sizeof g.name, "%s", props.deviceName);
#else
	snprintf(g.name, sizeof g.name, "%s", props.deviceName);
#endif

	g.discrete = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

	uint32_t nq = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(g.pd, &nq, NULL);
	VkQueueFamilyProperties *qf = calloc(nq, sizeof *qf);
	vkGetPhysicalDeviceQueueFamilyProperties(g.pd, &nq, qf);
	g.qi = UINT32_MAX;
	for (uint32_t i = 0; i < nq; i++)
		if (qf[i].queueFlags & VK_QUEUE_COMPUTE_BIT) { g.qi = i; break; }
	free(qf);
	if (g.qi == UINT32_MAX) FAIL("no compute queue", 0);

	/* What the device has decides the kernels, not the other way round. Without
	 * VK_KHR_cooperative_matrix every GEMM runs on `gemm_portable*.comp`; the caller
	 * reads `xmx_coopmat()` / `xmx_portable()` and hands over the matching SPIR-V. */
	uint32_t nde = 0;
	vkEnumerateDeviceExtensionProperties(g.pd, NULL, &nde, NULL);
	VkExtensionProperties *de = calloc(nde ? nde : 1, sizeof *de);
	vkEnumerateDeviceExtensionProperties(g.pd, NULL, &nde, de);
	g.coopmat = has_extension(de, nde, VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME);
	int subset = has_extension(de, nde, "VK_KHR_portability_subset");
	int explicit_ext = has_extension(de, nde, VK_KHR_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_EXTENSION_NAME);
	/* XMX_PIPELINE_STATS: the compiler's own account of each pipeline (build_pipeline_spec),
	 * through VK_KHR_pipeline_executable_properties where the device has it */
	const char *stats_path = getenv("XMX_PIPELINE_STATS");
	int stats = 0;
	if (stats_path && *stats_path) {
		stats = has_extension(de, nde, VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME);
		if (!stats)
			fprintf(stderr, "libxmx: XMX_PIPELINE_STATS is set, but %s has no %s\n",
				g.name, VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME);
	}
	free(de);
	const char *forced = getenv("XMX_PORTABLE");
	g.portable = !g.coopmat || (forced && *forced && atoi(forced) != 0);
	/* The staged kernel is only ever dispatched on the matrix path, so the aliasing
	 * extension is asked for only there — and only with all three of its features, which
	 * is what the shader declares. */
	g.explicit_layout = 0;
	if (g.coopmat && explicit_ext) {
		VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR have_wm = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR };
		VkPhysicalDeviceFeatures2 have2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &have_wm };
		vkGetPhysicalDeviceFeatures2(g.pd, &have2);
		g.explicit_layout = have_wm.workgroupMemoryExplicitLayout
				    && have_wm.workgroupMemoryExplicitLayoutScalarBlockLayout
				    && have_wm.workgroupMemoryExplicitLayout16BitAccess;
		g.int8 = g.explicit_layout && have_wm.workgroupMemoryExplicitLayout8BitAccess;
	}
	if (g.int8) {
		VkPhysicalDeviceVulkan12Features have12 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		VkPhysicalDeviceFeatures2 have2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &have12 };
		vkGetPhysicalDeviceFeatures2(g.pd, &have2);
		g.int8 = have12.shaderInt8 && have12.storageBuffer8BitAccess;
	}
	g.staged_ok = g.coopmat && !g.portable && g.explicit_layout;
	const char *ext[4]; uint32_t next = 0;
	if (g.coopmat) ext[next++] = VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME;
	if (g.explicit_layout) ext[next++] = VK_KHR_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_EXTENSION_NAME;
	if (subset) ext[next++] = "VK_KHR_portability_subset";   /* required when offered */
	if (stats) ext[next++] = VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME;

	VkPhysicalDeviceCooperativeMatrixFeaturesKHR cm = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR, .cooperativeMatrix = VK_TRUE };
	/* The subgroup width pinned at 32 on every pipeline (build_pipeline_spec), where the
	 * driver can: Vulkan 1.3's subgroup size control with 32 in range for compute, and full
	 * subgroups apart. Not every driver a desktop Arc runs has both, and asking for a
	 * feature the device lacks fails the device outright. */
	VkPhysicalDeviceVulkan13Features have13 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	VkPhysicalDeviceVulkan13Properties sizes = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES };
	g.pin32 = 0;
	if (props.apiVersion >= VK_API_VERSION_1_3) {
		VkPhysicalDeviceFeatures2 query = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &have13 };
		vkGetPhysicalDeviceFeatures2(g.pd, &query);
		VkPhysicalDeviceProperties2 props2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &sizes };
		vkGetPhysicalDeviceProperties2(g.pd, &props2);
		if (have13.subgroupSizeControl && sizes.minSubgroupSize <= 32 && sizes.maxSubgroupSize >= 32
		    && (sizes.requiredSubgroupSizeStages & VK_SHADER_STAGE_COMPUTE_BIT))
			g.pin32 = 1 | (have13.computeFullSubgroups ? 2 : 0);
	}
	if (!(g.pin32 & 1))
		fprintf(stderr, "libxmx: %s cannot fix the subgroup width at 32; the kernels are written "
			"for 32 lanes, and a driver that picks another width can make them race\n", g.name);
	detect_driver_modes(g.pd, props.apiVersion);
	VkPhysicalDeviceVulkan13Features v13 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = g.coopmat ? (void *)&cm : NULL,
		.subgroupSizeControl = (g.pin32 & 1) ? VK_TRUE : VK_FALSE,
		.computeFullSubgroups = (g.pin32 & 2) ? VK_TRUE : VK_FALSE };
	/* Shared-memory blocks that alias: gemm_staged.comp puts its operand tiles and its
	 * output stage in the same bytes, which is what fits sixteen of its workgroups in a
	 * core's 128 KB (notes/improve-shared-memory.md). */
	VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR wm = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR,
		/* the 1.3 block only where it asks for something: a 1.2 device knows no such struct */
		.pNext = (g.pin32 & 1) ? (void *)&v13 : (g.coopmat ? (void *)&cm : NULL),
		.workgroupMemoryExplicitLayout = VK_TRUE, .workgroupMemoryExplicitLayoutScalarBlockLayout = VK_TRUE,
		.workgroupMemoryExplicitLayout16BitAccess = VK_TRUE,
		/* the integer path's int8 tiles (gemm_staged_int8.comp), where the device has them */
		.workgroupMemoryExplicitLayout8BitAccess = g.int8 ? VK_TRUE : VK_FALSE };
	/* bufferDeviceAddress lets the resident path pass operands as 64-bit pointers in
	 * push constants, so a whole block of dispatches records into one command buffer
	 * without a descriptor pool. scalarBlockLayout matches the shaders' layout. */
	VkPhysicalDeviceVulkan12Features v12 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = g.explicit_layout ? (void *)&wm
		       : (g.pin32 & 1) ? (void *)&v13 : (g.coopmat ? (void *)&cm : NULL),
		.vulkanMemoryModel = VK_TRUE, .vulkanMemoryModelDeviceScope = VK_TRUE, .shaderFloat16 = VK_TRUE,
		.bufferDeviceAddress = VK_TRUE, .scalarBlockLayout = VK_TRUE,
		.storageBuffer8BitAccess = g.int8 ? VK_TRUE : VK_FALSE,
		.shaderInt8 = g.int8 ? VK_TRUE : VK_FALSE };
	VkPhysicalDeviceVulkan11Features v11 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, .pNext = &v12,
		.storageBuffer16BitAccess = VK_TRUE };
	/* The shaders do their address arithmetic in uint64_t, which declares SPIR-V's Int64
	 * capability, and that needs shaderInt64 on: the validation layer says so for nine of
	 * them. Mesa never minded. Asked for only where the device has it. */
	VkPhysicalDeviceFeatures2 base = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	vkGetPhysicalDeviceFeatures2(g.pd, &base);
	VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR executables = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR,
		.pNext = &v11, .pipelineExecutableInfo = VK_TRUE };
	VkPhysicalDeviceFeatures2 f2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
					 .pNext = stats ? (void *)&executables : (void *)&v11,
					 .features.shaderInt64 = base.features.shaderInt64 };
	float prio = 1.0f;
	VkDeviceQueueCreateInfo qci = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueFamilyIndex = g.qi, .queueCount = 1, .pQueuePriorities = &prio };
	VkDeviceCreateInfo dci = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, .pNext = &f2,
				   .queueCreateInfoCount = 1, .pQueueCreateInfos = &qci,
				   .enabledExtensionCount = next, .ppEnabledExtensionNames = ext };
	r = vkCreateDevice(g.pd, &dci, NULL, &g.dev);
	if (r) { g.dev = VK_NULL_HANDLE; FAIL("vkCreateDevice", r); }
	vkGetDeviceQueue(g.dev, g.qi, 0, &g.q);
	if (stats) {
		g.stats_props = (PFN_vkGetPipelineExecutablePropertiesKHR)
			vkGetDeviceProcAddr(g.dev, "vkGetPipelineExecutablePropertiesKHR");
		g.stats_values = (PFN_vkGetPipelineExecutableStatisticsKHR)
			vkGetDeviceProcAddr(g.dev, "vkGetPipelineExecutableStatisticsKHR");
		g.stats_ir = (PFN_vkGetPipelineExecutableInternalRepresentationsKHR)
			vkGetDeviceProcAddr(g.dev, "vkGetPipelineExecutableInternalRepresentationsKHR");
		const char *ir = getenv("XMX_PIPELINE_IR");
		g.ir_dir = ir && *ir && g.stats_ir ? ir : NULL;
		if (g.stats_props && g.stats_values) g.stats = fopen(stats_path, "a");
		if (!g.stats) fprintf(stderr, "libxmx: cannot write pipeline statistics to %s\n", stats_path);
	}
	return 0;
}

/* Release everything libxmx made: buffers, graphs, pipelines, pools, fences — and its
 * own device and instance, unless they were adopted, in which case they go back to the
 * host untouched. `xmx_open` may be called again afterwards (an adoption handed over
 * since then is taken up). Every buffer id and graph id is dead after this. */
int xmx_buf_destroy(int id);
int xmx_graph_destroy(int id);

void xmx_close(void)
{
	if (!g.dev) { adopt.set = 0; return; }
	if (g.adopted) { qlock(); vkQueueWaitIdle(g.q); qunlock(); }
	else vkDeviceWaitIdle(g.dev);
	for (int i = 0; i < MAX_RBUF; i++) xmx_buf_destroy(i);
	for (int i = 0; i < MAX_GRAPHS; i++) xmx_graph_destroy(i);
	for (unsigned i = 0; i < specialized_count; i++) vkDestroyPipeline(g.dev, specialized[i].pipeline, NULL);
	specialized_count = 0;
	VkPipeline *pipes[] = { &g.rgemm, &g.rtiled, &g.rstaged, &g.runary, &g.rrow, &g.rhistory,
				&g.rwindow[0], &g.rwindow[1], &g.rffn, &g.rstaged32[0], &g.rstaged32[1],
				&g.rrows, &g.rint8, &g.rblock, &g.rglobal, &g.rwide, &g.pipe, &g.pipeb };
	for (size_t i = 0; i < sizeof pipes / sizeof *pipes; i++) if (*pipes[i]) vkDestroyPipeline(g.dev, *pipes[i], NULL);
	if (g.rpl) vkDestroyPipelineLayout(g.dev, g.rpl, NULL);
	if (g.plb) vkDestroyPipelineLayout(g.dev, g.plb, NULL);
	if (g.pl) vkDestroyPipelineLayout(g.dev, g.pl, NULL);
	if (g.dsl) vkDestroyDescriptorSetLayout(g.dev, g.dsl, NULL);
	for (int i = 0; i < 13; i++) free(g.rpaths[i]);
	if (g.qpool) vkDestroyQueryPool(g.dev, g.qpool, NULL);
	if (g.fence) vkDestroyFence(g.dev, g.fence, NULL);
	if (g.rfence) vkDestroyFence(g.dev, g.rfence, NULL);
	if (g.tfence) vkDestroyFence(g.dev, g.tfence, NULL);
	if (g.cpool) vkDestroyCommandPool(g.dev, g.cpool, NULL);   /* frees cb, rcb, tcb */
	if (g.dpool) vkDestroyDescriptorPool(g.dev, g.dpool, NULL); /* frees set */
	struct buf *bufs[] = { &g.A, &g.B, &g.C, &g.stage };
	for (size_t i = 0; i < sizeof bufs / sizeof *bufs; i++)
		if (bufs[i]->b) {
			if (bufs[i]->p) vkUnmapMemory(g.dev, bufs[i]->m);
			vkDestroyBuffer(g.dev, bufs[i]->b, NULL);
			vkFreeMemory(g.dev, bufs[i]->m, NULL);
		}
	if (!g.adopted) {
		vkDestroyDevice(g.dev, NULL);
		vkDestroyInstance(g.inst, NULL);
	}
	if (g.stats) fclose(g.stats);
	memset(&g, 0, sizeof g);
	memset(graphs, 0, sizeof graphs);
	memset(prof_ms, 0, sizeof prof_ms);
	memset(prof_hits, 0, sizeof prof_hits);
	adopt.set = 0;
}

/* Whether the pipelines declare DenormPreserve 16 (detect_driver_modes). */
int xmx_preserve16(void) { return g.preserve16; }

/* Which half_round the pipelines compile, constant 1 (detect_driver_modes): 1 is
 * `float(float16_t(x))`, 0 the packHalf2x16 round trip. The daemon's start-up probe checks
 * the one in use. */
int xmx_half_by_cast(void) { return g.half_by_cast; }

/* Whether the staged GEMM copies its operands as raw 128-bit vectors, constant 2
 * (detect_driver_modes; `XMX_STAGED_PACKED=0` restores the half-vector loader). */
int xmx_staged_packed(void) { return g.staged_packed; }
/* And whether the portable GEMM fetches A as raw 128-bit vectors, constant 3
 * (`XMX_PORTABLE_PACKED=1`; off by default, see detect_driver_modes). */
int xmx_portable_packed(void) { return g.portable_packed; }

/* Whether the device is a card with memory of its own rather than the host's (memtype). */
int xmx_discrete(void) { return g.discrete; }

int xmx_coopmat(void) { return g.dev ? g.coopmat : -1; }
int xmx_portable(void) { return g.dev ? g.portable : -1; }
/* Whether a GEMM may gather its A from the image in window order (0x400000): the staged
 * kernel does it on the matrix path, the tiled portable kernel on the other. A device with
 * matrix units but no VK_KHR_workgroup_memory_explicit_layout — a host that lent its device
 * without enabling it — has neither, and the caller records the partition pass instead. */
int xmx_window_gather(void) { return g.dev ? (g.staged_ok || g.portable) : -1; }
/* One line for a log: which GEMM kernels this device runs, and why. */
const char *xmx_path(void)
{
	if (!g.dev) return "not opened";
	if (!g.portable) return "cooperative matrix (VK_KHR_cooperative_matrix, fp16 x fp16 -> fp32)";
	return g.coopmat ? "portable multiply-add (XMX_PORTABLE=1; the device has cooperative matrix)"
			 : "portable multiply-add (the device has no VK_KHR_cooperative_matrix)";
}

int xmx_init(const char *spv_path)
{
	if (g.ready) return 0;
	if (xmx_open()) return -1;
	VkResult r;

	VkDescriptorSetLayoutBinding bind[3];
	for (int i = 0; i < 3; i++)
		bind[i] = (VkDescriptorSetLayoutBinding){ .binding = i, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
							  .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT };
	VkDescriptorSetLayoutCreateInfo dl = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
					       .bindingCount = 3, .pBindings = bind };
	if ((r = vkCreateDescriptorSetLayout(g.dev, &dl, NULL, &g.dsl))) FAIL("dsl", r);
	VkPushConstantRange pcr = { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .size = 12 };
	VkPipelineLayoutCreateInfo pli = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
					   .setLayoutCount = 1, .pSetLayouts = &g.dsl,
					   .pushConstantRangeCount = 1, .pPushConstantRanges = &pcr };
	if ((r = vkCreatePipelineLayout(g.dev, &pli, NULL, &g.pl))) FAIL("pipeline layout", r);

	if (build_pipeline(spv_path, g.pl, &g.pipe))
		return -1;

	VkDescriptorPoolSize ps = { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 3 };
	VkDescriptorPoolCreateInfo dpi = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
					   .maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &ps };
	if ((r = vkCreateDescriptorPool(g.dev, &dpi, NULL, &g.dpool))) FAIL("descriptor pool", r);
	VkDescriptorSetAllocateInfo dsa = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					    .descriptorPool = g.dpool, .descriptorSetCount = 1, .pSetLayouts = &g.dsl };
	if ((r = vkAllocateDescriptorSets(g.dev, &dsa, &g.set))) FAIL("descriptor set", r);

	VkCommandPoolCreateInfo cpci = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
					 .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
					 .queueFamilyIndex = g.qi };
	if ((r = vkCreateCommandPool(g.dev, &cpci, NULL, &g.cpool))) FAIL("command pool", r);
	VkCommandBufferAllocateInfo cba = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
					    .commandPool = g.cpool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
					    .commandBufferCount = 1 };
	if ((r = vkAllocateCommandBuffers(g.dev, &cba, &g.cb))) FAIL("command buffer", r);
	VkFenceCreateInfo fi = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	if ((r = vkCreateFence(g.dev, &fi, NULL, &g.fence))) FAIL("fence", r);

	g.ready = 1;
	return 0;
}

/* Zero-copy path: hand the caller the mapped operand buffers so it can build A in
 * place and read C in place, instead of memcpying both across. On a shared-memory
 * APU those copies buy nothing. `xmx_reserve` may reallocate, so the pointers it
 * returns are valid only until the next call. */
int xmx_reserve(unsigned M, unsigned N, unsigned K, void **pa, void **pb, void **pc)
{
	if (!g.ready) FAIL("not initialised", 0);
	if (ensure(&g.A, (VkDeviceSize)M * K * 2, 0) || ensure(&g.B, (VkDeviceSize)K * N * 2, 0)
	    || ensure(&g.C, (VkDeviceSize)M * N * 4, 1))
		return -1;
	if (pa) *pa = g.A.p;
	if (pb) *pb = g.B.p;
	if (pc) *pc = g.C.p;
	return 0;
}

/* iters > 1 dispatches the same work repeatedly inside one submit, so the caller can
 * time the GPU without host copies dominating. */
int xmx_gemm(unsigned M, unsigned N, unsigned K, const void *a, const void *b, void *c, unsigned iters)
{
	if (!g.ready) FAIL("not initialised", 0);
	VkDeviceSize sa = (VkDeviceSize)M * K * 2, sb = (VkDeviceSize)K * N * 2, sc = (VkDeviceSize)M * N * 4;
	if (ensure(&g.A, sa, 0) || ensure(&g.B, sb, 0) || ensure(&g.C, sc, 1))
		return -1;
	VkDescriptorBufferInfo dbi[3] = { { g.A.b, 0, sa }, { g.B.b, 0, sb }, { g.C.b, 0, sc } };
	VkWriteDescriptorSet w[3];
	for (int i = 0; i < 3; i++)
		w[i] = (VkWriteDescriptorSet){ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = g.set,
					       .dstBinding = i, .descriptorCount = 1,
					       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &dbi[i] };
	vkUpdateDescriptorSets(g.dev, 3, w, 0, NULL);
	if (a) memcpy(g.A.p, a, sa);
	if (b) memcpy(g.B.p, b, sb);

	vkResetCommandBuffer(g.cb, 0);
	VkCommandBufferBeginInfo bi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
	vkBeginCommandBuffer(g.cb, &bi);
	vkCmdBindPipeline(g.cb, VK_PIPELINE_BIND_POINT_COMPUTE, g.pipe);
	vkCmdBindDescriptorSets(g.cb, VK_PIPELINE_BIND_POINT_COMPUTE, g.pl, 0, 1, &g.set, 0, NULL);
	unsigned pc[3] = { M, N, K };
	vkCmdPushConstants(g.cb, g.pl, VK_SHADER_STAGE_COMPUTE_BIT, 0, 12, pc);
	VkMemoryBarrier mb = { .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			       .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
			       .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT };
	for (unsigned i = 0; i < (iters ? iters : 1); i++) {
		vkCmdDispatch(g.cb, N / 16, M / 8, 1);
		if (i + 1 < iters)
			vkCmdPipelineBarrier(g.cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &mb, 0, NULL, 0, NULL);
	}
	vkEndCommandBuffer(g.cb);
	vkResetFences(g.dev, 1, &g.fence);
	VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &g.cb };
	qlock();
	VkResult r = vkQueueSubmit(g.q, 1, &si, g.fence);
	qunlock();
	if (r) FAIL("submit", r);
	r = vkWaitForFences(g.dev, 1, &g.fence, VK_TRUE, 60ull * 1000000000ull);
	if (r) FAIL("fence wait", r);
	if (c) memcpy(c, g.C.p, sc);
	return 0;
}


/* The batched pipeline is built on first use: the extra shader takes seven push
 * constants instead of three, so it needs its own pipeline layout. */
int xmx_init_batched(const char *spv_path)
{
	if (!g.ready) FAIL("not initialised", 0);
	if (g.pipeb) return 0;
	VkPushConstantRange pcr = { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .size = 28 };
	VkPipelineLayoutCreateInfo pli = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
					   .setLayoutCount = 1, .pSetLayouts = &g.dsl,
					   .pushConstantRangeCount = 1, .pPushConstantRanges = &pcr };
	VkResult r = vkCreatePipelineLayout(g.dev, &pli, NULL, &g.plb);
	if (r) FAIL("batched pipeline layout", r);
	return build_pipeline(spv_path, g.plb, &g.pipeb);
}

/* Byte capacities, so the caller can lay the three tensors out itself. */
int xmx_reserve_bytes(unsigned long long a, unsigned long long b, unsigned long long c,
		      void **pa, void **pb, void **pc)
{
	if (!g.ready) FAIL("not initialised", 0);
	if (ensure(&g.A, a, 0) || ensure(&g.B, b, 0) || ensure(&g.C, c, 1))
		return -1;
	if (pa) *pa = g.A.p;
	if (pb) *pb = g.B.p;
	if (pc) *pc = g.C.p;
	return 0;
}

int xmx_gemm_batched(unsigned M, unsigned N, unsigned K, unsigned batch,
		     unsigned sa, unsigned sb, unsigned sc, unsigned bt)
{
	if (!g.pipeb) FAIL("batched pipeline not built", 0);
	VkDeviceSize sza = (VkDeviceSize)batch * sa * 2, szb = (VkDeviceSize)batch * sb * 2,
		     szc = (VkDeviceSize)batch * sc * 4;
	VkDescriptorBufferInfo dbi[3] = { { g.A.b, 0, sza }, { g.B.b, 0, szb }, { g.C.b, 0, szc } };
	VkWriteDescriptorSet w[3];
	for (int i = 0; i < 3; i++)
		w[i] = (VkWriteDescriptorSet){ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = g.set,
					       .dstBinding = i, .descriptorCount = 1,
					       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &dbi[i] };
	vkUpdateDescriptorSets(g.dev, 3, w, 0, NULL);

	vkResetCommandBuffer(g.cb, 0);
	VkCommandBufferBeginInfo bi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
	vkBeginCommandBuffer(g.cb, &bi);
	vkCmdBindPipeline(g.cb, VK_PIPELINE_BIND_POINT_COMPUTE, g.pipeb);
	vkCmdBindDescriptorSets(g.cb, VK_PIPELINE_BIND_POINT_COMPUTE, g.plb, 0, 1, &g.set, 0, NULL);
	unsigned push[7] = { M, N, K, sa, sb, sc, bt };
	vkCmdPushConstants(g.cb, g.plb, VK_SHADER_STAGE_COMPUTE_BIT, 0, 28, push);
	vkCmdDispatch(g.cb, N / 16, M / 8, batch);
	vkEndCommandBuffer(g.cb);
	vkResetFences(g.dev, 1, &g.fence);
	VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &g.cb };
	qlock();
	VkResult r = vkQueueSubmit(g.q, 1, &si, g.fence);
	qunlock();
	if (r) FAIL("submit", r);
	r = vkWaitForFences(g.dev, 1, &g.fence, VK_TRUE, 60ull * 1000000000ull);
	if (r) FAIL("fence wait", r);
	return 0;
}

/* ------------------------------------------------------------------------- */
/* The resident runtime.                                                      */
/*                                                                            */
/* Operands are 64-bit device addresses in the push constants rather than      */
/* descriptor bindings, so recording is just push-and-dispatch and a whole     */
/* block's dispatches go into one command buffer with one fence at the end,    */
/* instead of one submit per GEMM. Activations stay in device buffers between  */
/* passes; the point is not a faster kernel but the traffic that disappears.   */
/* ------------------------------------------------------------------------- */

int xmx_res_init(const char *gemm_spv, const char *unary_spv, const char *row_spv,
		 const char *history_spv, const char *tiled_spv, const char *staged_spv)
{
	if (!g.ready) FAIL("not initialised", 0);
	if (g.rready) return 0;
	VkPushConstantRange pcr = { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .size = sizeof(struct push) };
	VkPipelineLayoutCreateInfo pli = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
					   .pushConstantRangeCount = 1, .pPushConstantRanges = &pcr };
	VkResult r = vkCreatePipelineLayout(g.dev, &pli, NULL, &g.rpl);
	if (r) FAIL("resident pipeline layout", r);
	const char *paths[] = { gemm_spv, tiled_spv, staged_spv, unary_spv, row_spv };
	for (unsigned i = 0; i < 5; i++) {
		g.rpaths[i] = nr_strdup(paths[i]);
		if (!g.rpaths[i]) FAIL("pipeline path allocation", 0);
	}
	const char *spec = getenv("XMX_SPECIALIZE");
	g.specialize = spec ? (unsigned)atoi(spec) : 7;
	if (g.specialize > 7) FAIL("XMX_SPECIALIZE must be in 0..7", 0);
	if (build_pipeline(gemm_spv, g.rpl, &g.rgemm) || build_pipeline(unary_spv, g.rpl, &g.runary)
	    || build_pipeline(row_spv, g.rpl, &g.rrow)
	    || build_pipeline(history_spv, g.rpl, &g.rhistory)
	    || build_pipeline(tiled_spv, g.rpl, &g.rtiled)
	    || (g.staged_ok && build_pipeline(staged_spv, g.rpl, &g.rstaged)))
		return -1;
	const char *tile = getenv("XMX_TILE_K");
	g.tiling = tile ? atoi(tile) : 1;
	g.tilem = block_size("XMX_TILE_M", 16);
	g.tilen = block_size("XMX_TILE_N", 32);
	const char *sk = getenv("XMX_STAGE_K");
	g.staging = sk ? (unsigned)atoi(sk) : 32;
	/* On by default: the live extent's GEMMs whose M is not whole 64-row blocks — 144 or 400
	 * rows at the deeper levels — ran on the tiled kernel at half the staged one's speed.
	 * Live, 512x288 at scale 0.35: 42.7 -> 36.7 ms, bit-identical. It pays only where some
	 * level's pixel count is not whole 64-row blocks: at 320x320 and 1920x1088, not at
	 * 1280x768. 0 is the comparison. */
	const char *sp = getenv("XMX_STAGED_PARTIAL");
	g.staged_partial = sp ? (unsigned)atoi(sp) : 1;
	/* the 32-row staged builds, once `xmx_staged32_init` has them; 0 is the comparison */
	const char *s32 = getenv("XMX_STAGED32");
	g.staged32 = s32 ? (unsigned)atoi(s32) : 1;
	/* Without matrix units the 16x32 build takes every shape it fits (M % 16, N % 32), and
	 * the 16x64 `_wide` build is off. Both measured through MoltenVK on an M3 once the
	 * 16x32 build's pooled epilogue lost a `precise` that glslang had spread over its whole
	 * K loop (notes/improve-portable-precise.md): 16x32 against 8x16 and 16x64, 64x1024x4096
	 * 3.33 against 4.87 and 6.43 ms, 256x512x512 1.52 / 2.24 / 2.06, 1600x1024x1024
	 * 7.39 / 11.0 / 9.1, 6400x2048x512 14.3 / 21.7 / 17.4 — the same bytes on all three, and
	 * N = 32 and 96 within 3 % of the 8x16 build. The earlier findings this replaces ("16x32
	 * slower than 8x16 on every shape", "16x64 twice as fast on the large ones") were that
	 * one qualifier. XMX_PORTABLE_TILED=0 restores the 8x16 routing, XMX_PORTABLE_WIDE=1
	 * builds the 16x64 kernel (found beside the 16x32 build, `_tiled` made `_wide`) and
	 * gives it the shapes whose N is whole 64-column blocks, for a device where it wins. */
	const char *tiled_env = getenv("XMX_PORTABLE_TILED");
	g.portable_tiled = tiled_env ? (unsigned)atoi(tiled_env) : 1;
	const char *wide_env = getenv("XMX_PORTABLE_WIDE");
	if (g.portable && wide_env && atoi(wide_env) != 0 && strstr(tiled_spv, "_tiled")) {
		size_t n = strlen(tiled_spv), at = (size_t)(strstr(tiled_spv, "_tiled") - tiled_spv);
		char *wide = malloc(n + 1);
		if (wide) {
			memcpy(wide, tiled_spv, at);
			memcpy(wide + at, "_wide", 5);
			memcpy(wide + at + 5, tiled_spv + at + 6, n - at - 6 + 1);
			char saved[sizeof g.err];
			memcpy(saved, g.err, sizeof saved);
			if (build_pipeline(wide, g.rpl, &g.rwide)) {
				g.rwide = VK_NULL_HANDLE;     /* not there: the 16x32 build serves */
				memcpy(g.err, saved, sizeof saved);
				free(wide);
			} else {
				g.rpaths[12] = wide;
			}
		}
	}
	VkCommandBufferAllocateInfo cba = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
					    .commandPool = g.cpool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
					    .commandBufferCount = 1 };
	if ((r = vkAllocateCommandBuffers(g.dev, &cba, &g.rcb))) FAIL("resident command buffer", r);
	if ((r = vkAllocateCommandBuffers(g.dev, &cba, &g.tcb))) FAIL("transfer command buffer", r);
	VkFenceCreateInfo fi = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	if ((r = vkCreateFence(g.dev, &fi, NULL, &g.rfence))) FAIL("resident fence", r);
	if ((r = vkCreateFence(g.dev, &fi, NULL, &g.tfence))) FAIL("transfer fence", r);
	g.unmapped = want_unmapped();
	g.rready = 1;
	return 0;
}

/* `kind`: 0 the graph's own buffers, 1 a buffer the host reads back, 2 one it writes.
 *
 * Kinds 1 and 2 are always mapped — they exist to be touched by the host, and on an
 * unmapped device they are the staging the graph copies through. Kind 0 follows the
 * device: mapped where the host can see the card's memory, device-local and unmapped
 * where it cannot. */
int xmx_buf_create_kind(unsigned long long bytes, int kind)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	int id = -1;
	for (int i = 0; i < MAX_RBUF; i++)
		if (!rbufs[i].live) { id = i; break; }
	if (id < 0) FAIL("out of buffer slots", 0);
	struct rbuf *rb = &rbufs[id];
	VkBufferCreateInfo bi = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = bytes ? bytes : 4,
				  .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
					   | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
					   | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT };
	VkResult r = vkCreateBuffer(g.dev, &bi, NULL, &rb->b);
	if (r) FAIL("vkCreateBuffer (resident)", r);
	VkMemoryRequirements mr;
	vkGetBufferMemoryRequirements(g.dev, rb->b, &mr);
	int unmapped = kind == 0 && g.unmapped;
	const char *failure = unmapped ? "no device-local memory type"
				       : "no host-visible memory type";
	uint32_t mt = unmapped ? memtype_device(mr.memoryTypeBits)
			       : memtype(mr.memoryTypeBits,
					 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
					 | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, kind == 1);
	if (mt == UINT32_MAX) goto failed_kind;
	VkMemoryAllocateFlagsInfo fl = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
					 .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT };
	VkMemoryAllocateInfo ai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = &fl,
				    .allocationSize = mr.size, .memoryTypeIndex = mt };
	failure = "vkAllocateMemory (resident)";
	if ((r = vkAllocateMemory(g.dev, &ai, NULL, &rb->m))) goto failed_kind;
	failure = "vkBindBufferMemory (resident)";
	if ((r = vkBindBufferMemory(g.dev, rb->b, rb->m, 0))) goto failed_kind;
	if (!unmapped) {
		failure = "vkMapMemory (resident)";
		if ((r = vkMapMemory(g.dev, rb->m, 0, VK_WHOLE_SIZE, 0, &rb->p))) goto failed_kind;
	}
	rb->mapped = !unmapped;
	VkBufferDeviceAddressInfo ai2 = { .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = rb->b };
	rb->addr = vkGetBufferDeviceAddress(g.dev, &ai2);
	rb->size = bi.size;
	rb->live = 1;
	return id;
failed_kind:
	if (rb->p) vkUnmapMemory(g.dev, rb->m);
	if (rb->b) vkDestroyBuffer(g.dev, rb->b, NULL);
	if (rb->m) vkFreeMemory(g.dev, rb->m, NULL);
	*rb = (struct rbuf){ 0 };
	FAIL(failure, r);
}

int xmx_buf_create(unsigned long long bytes) { return xmx_buf_create_kind(bytes, 0); }

int xmx_buf_host_visible(int id)
{
	return (id >= 0 && id < MAX_RBUF && rbufs[id].live && rbufs[id].mapped) ? 1 : 0;
}

int xmx_staging_mode(void) { return g.unmapped; }

/* One growable host-visible buffer, for the transfers that happen once rather than every
 * frame — the weights, and anything a test reads back. Per-frame traffic does not come
 * through here: it is recorded into the graph as a copy, so a frame is still one submit. */
static int submit_commands(VkCommandBuffer commands, int passes);

/* A transfer runs on its own command buffer and fence so it can happen while the frame's
 * graph is still being recorded, which is when the weights arrive. */
static int submit_transfer(void)
{
	VkResult r = vkEndCommandBuffer(g.tcb);
	if (r) FAIL("end transfer", r);
	if ((r = vkResetFences(g.dev, 1, &g.tfence))) FAIL("reset transfer fence", r);
	VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1,
			    .pCommandBuffers = &g.tcb };
	qlock();
	r = vkQueueSubmit(g.q, 1, &si, g.tfence);
	qunlock();
	if (r) FAIL("transfer submit", r);
	if ((r = vkWaitForFences(g.dev, 1, &g.tfence, VK_TRUE, 60ull * 1000000000ull)))
		FAIL("transfer fence", r);
	return 0;
}

static int begin_transfer(void)
{
	VkCommandBufferBeginInfo bi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
	vkResetCommandBuffer(g.tcb, 0);
	VkResult r = vkBeginCommandBuffer(g.tcb, &bi);
	if (r) FAIL("begin transfer", r);
	return 0;
}

static int ensure_stage(VkDeviceSize size)
{
	if (g.stage.cap >= size) return 0;
	if (g.stage.b) {
		vkUnmapMemory(g.dev, g.stage.m);
		vkDestroyBuffer(g.dev, g.stage.b, NULL);
		vkFreeMemory(g.dev, g.stage.m, NULL);
		g.stage = (struct buf){ 0 };
	}
	VkBufferCreateInfo bi = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = size,
				  .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
					   | VK_BUFFER_USAGE_TRANSFER_DST_BIT };
	VkResult r = vkCreateBuffer(g.dev, &bi, NULL, &g.stage.b);
	if (r) FAIL("vkCreateBuffer (stage)", r);
	VkMemoryRequirements mr;
	vkGetBufferMemoryRequirements(g.dev, g.stage.b, &mr);
	uint32_t mt = memtype(mr.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1);
	if (mt == UINT32_MAX) FAIL("no host-visible staging type", 0);
	VkMemoryAllocateInfo ai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				    .allocationSize = mr.size, .memoryTypeIndex = mt };
	if ((r = vkAllocateMemory(g.dev, &ai, NULL, &g.stage.m))) FAIL("vkAllocateMemory (stage)", r);
	if ((r = vkBindBufferMemory(g.dev, g.stage.b, g.stage.m, 0))) FAIL("vkBindBufferMemory (stage)", r);
	if ((r = vkMapMemory(g.dev, g.stage.m, 0, VK_WHOLE_SIZE, 0, &g.stage.p))) FAIL("vkMapMemory (stage)", r);
	g.stage.cap = size;
	return 0;
}

static int stage_copy(int to_device, int id, const void *src, void *dst,
		      unsigned long long offset, unsigned long long bytes)
{
	if (id < 0 || id >= MAX_RBUF || !rbufs[id].live) FAIL("buffer is not live", 0);
	if (offset + bytes > rbufs[id].size) FAIL("transfer runs past the buffer", 0);
	if (!bytes) return 0;
	if (rbufs[id].mapped) {
		unsigned char *p = (unsigned char *)rbufs[id].p + offset;
		if (to_device) memcpy(p, src, (size_t)bytes);
		else memcpy(dst, p, (size_t)bytes);
		return 0;
	}
	if (ensure_stage((VkDeviceSize)bytes)) return -1;
	if (to_device) memcpy(g.stage.p, src, (size_t)bytes);
	if (begin_transfer()) return -1;
	VkBufferCopy region = { .srcOffset = to_device ? 0 : offset,
				.dstOffset = to_device ? offset : 0, .size = bytes };
	if (to_device) vkCmdCopyBuffer(g.tcb, g.stage.b, rbufs[id].b, 1, &region);
	else vkCmdCopyBuffer(g.tcb, rbufs[id].b, g.stage.b, 1, &region);
	if (submit_transfer()) return -1;
	if (!to_device) memcpy(dst, g.stage.p, (size_t)bytes);
	return 0;
}

/* Clearing a buffer the host cannot address is the device's job, and it has to work
 * whether or not a graph is being recorded: the scratch arena allocates and clears its
 * roles lazily, in the middle of the recording that is about to use them. */
static int stage_copy(int to_device, int id, const void *src, void *dst,
		      unsigned long long offset, unsigned long long bytes);

int xmx_buf_zero(int id)
{
	if (id < 0 || id >= MAX_RBUF || !rbufs[id].live) FAIL("buffer is not live", 0);
	if (rbufs[id].mapped) {
		memset(rbufs[id].p, 0, (size_t)rbufs[id].size);
		return 0;
	}
	/* Never into the graph, even while one is being recorded: a recorded fill runs again
	 * on every replay, and `buffer_from` zeroes a weight buffer's padding *before*
	 * uploading the weights — so the graph would wipe them on its way past, every frame.
	 * Clearing means now, on the transfer queue, like the memset it replaces. */
	if (rbufs[id].size & 3) {
		/* vkCmdFillBuffer works in whole words and may not run past the buffer, so a
		 * size that is not a multiple of four would leave a tail dirty. Rare enough to
		 * pay for with a copy rather than a second code path. */
		unsigned char *zeros = calloc(1, (size_t)rbufs[id].size);
		if (!zeros) FAIL("out of memory clearing a buffer", 0);
		int bad = stage_copy(1, id, zeros, NULL, 0, rbufs[id].size);
		free(zeros);
		return bad;
	}
	if (begin_transfer()) return -1;
	vkCmdFillBuffer(g.tcb, rbufs[id].b, 0, rbufs[id].size, 0);
	return submit_transfer();
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
	return (id >= 0 && id < MAX_RBUF && rbufs[id].live) ? (unsigned long long)rbufs[id].size : 0;
}

int xmx_buf_destroy(int id)
{
	if (id < 0 || id >= MAX_RBUF || !rbufs[id].live) return 0;
	if (rbufs[id].mapped) vkUnmapMemory(g.dev, rbufs[id].m);
	vkDestroyBuffer(g.dev, rbufs[id].b, NULL);
	vkFreeMemory(g.dev, rbufs[id].m, NULL);
	rbufs[id] = (struct rbuf){ 0 };
	return 0;
}

static VkDeviceAddress addr_of(int id)
{
	return (id >= 0 && id < MAX_RBUF && rbufs[id].live) ? rbufs[id].addr : 0;
}

/* Close a pass with a timestamp. Bottom-of-pipe, so it lands after the dispatch has
 * finished rather than after it was issued. Costs one query per pass and nothing at all
 * when profiling is off, which is why it can live on the hot path. */
static void stamp(unsigned family, unsigned subkind)
{
	if (!g.prof || !g.qpool || g.prof_n >= g.qcount) return;
	stamp_kind[g.prof_n] = (unsigned char)(family * 32u + (subkind & 31u));
	vkCmdWriteTimestamp(g.rcb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, g.qpool, g.prof_n);
	g.prof_n++;
}

int xmx_begin(void)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.recording) FAIL("already recording", 0);
	vkResetCommandBuffer(g.rcb, 0);
	/* Captured graphs are replayed, so ONE_TIME_SUBMIT is deliberately absent. */
	VkCommandBufferBeginInfo bi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkResult r = vkBeginCommandBuffer(g.rcb, &bi);
	if (r) FAIL("begin resident recording", r);
	g.recording = 1;
	g.recorded = 0;
	g.syncing = 1;
	g.prof_n = 0;
	if (g.prof && g.qpool) {
		vkCmdResetQueryPool(g.rcb, g.qpool, 0, g.qcount);
		stamp(PK_START, 0);       /* the zero point every later stamp is measured from */
	}
	return 0;
}

int xmx_abort(void)
{
	if (!g.recording) return 0;
	VkResult r = vkResetCommandBuffer(g.rcb, 0);
	if (r) FAIL("abort resident recording", r);
	g.recording = 0;
	return 0;
}

/* One global barrier between passes. Most passes consume the previous one's output,
 * so this is right by default; where a run of dispatches is known to be independent —
 * the per-head GEMMs of a branched feed-forward write disjoint slices, the three head
 * splits read one buffer and write three — `xmx_sync(0)` suppresses it and `xmx_sync(1)`
 * closes the run with a single barrier. */
static void barrier(void)
{
	if (!g.syncing) return;
	VkMemoryBarrier mb = { .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			       .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
			       .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT };
	vkCmdPipelineBarrier(g.rcb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &mb, 0, NULL, 0, NULL);
}

/* Copy a skip without a host fence. Both barriers cover execution dependencies
 * (including write-after-read), and make writes visible between compute/transfer. */
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
	VkMemoryBarrier before = { .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT };
	vkCmdPipelineBarrier(g.rcb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &before, 0, NULL, 0, NULL);
	VkBufferCopy region = { .srcOffset = source_offset, .dstOffset = target_offset, .size = bytes };
	vkCmdCopyBuffer(g.rcb, a->b, b->b, 1, &region);
	VkMemoryBarrier after = { .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT |
				 VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT };
	vkCmdPipelineBarrier(g.rcb, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 1, &after, 0, NULL, 0, NULL);
	stamp(PK_COPY, 0);
	g.recorded++;
	return 0;
}

/* Suppress or restore the barrier between dispatches. Restoring emits one, closing the
 * independent run. */
int xmx_sync(int on)
{
	if (!g.recording) FAIL("not recording", 0);
	int was = g.syncing;
	g.syncing = on;
	if (on && !was) barrier();
	return 0;
}

/* The QKV epilogue's other operands: K and V targets, the query scale, and the layout
 * the three targets share, (window, head, token, 32). */
struct qkv_targets { int k, v, scale; unsigned tokens, heads; };

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
		/* A gathered from the image in window order (gemm_staged.comp, 0x400000):
		 * the batch strides have no use with one batch, so they carry the geometry */
		p.sa = window_a[0]; p.sb = window_a[1]; p.sc = window_a[2];
		p.window_pad = window_a[3];
	}
	/* Bit 0x800000 (gemm_staged.comp): block 0's window residual pooled and published in
	 * its own epilogue — `c` takes the published skip as half, and the pool rides in the
	 * slot only the QKV epilogue uses. A block is one window across all its channels. */
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
	/* Element offsets are folded into the addresses, so a sub-matrix needs no shader
	 * support: A and B are half, and C is float unless the epilogue narrows it. */
	p.a += (uint64_t)oa * 2; p.b += (uint64_t)ob * 2;
	p.c += (uint64_t)oc * ((bt & 0x1000u) ? 2 : 4);
	/* The register-tiled kernel keeps a 16x32 block of the output in one subgroup's
	 * registers, which needs both extents to be a whole block; the 8x16 kernel takes
	 * everything else. Slice writes make the N test exact rather than conservative —
	 * a tile past the slice would land in the neighbouring one.
	 *
	 * 16x32 and not larger: a 32x32 block halves the workgroup count again and is
	 * *slower* in a frame, because the shapes here then stop having enough workgroups
	 * to fill the machine. It still wins in isolation, which is why the two disagree.
	 * The environment overrides exist so that trade can be re-measured. */
	/* Operand staging pays only where there is reuse to amortise it. It wins up to
	 * 24 % on a deep-K shape in isolation and loses half as much again on the shallow-K
	 * ones that dominate this graph, where the output write is the cost and there is
	 * nothing to reuse; K >= 128 is where it stops losing. Over a whole frame the two
	 * are indistinguishable — see notes/phase22-staging-and-storage.md.
	 * That was the staged kernel on half its threads, waiting on its loads one at a time.
	 * Given all of them (notes/improve-shared-memory.md, improve-fusions.md) it wins at
	 * every depth it takes: K >= 32 is 1 ms of the 320x320 graph (32.2 -> 31.2) and 4 ms at
	 * 1280x720, the same bytes. What stays tiled is K = 16, the stem. */
	/* A last, partial 64-row block is the staged kernel's own business (its rows past M are
	 * neither read out of bounds nor stored), so with `staged_partial` M need not be whole
	 * blocks. The QKV epilogue was the exception until its store learned to skip them too:
	 * the bottleneck's projection at 96 tokens (a 576x352 network, 1920x1080 at 0.3) ran on
	 * the tiled kernel at 0.255 ms a call. */
	/* A bottleneck of 32 tokens or fewer — 16 at 256x128, 32 at 320x192, network extents
	 * only `min_extent` below 320 reaches — is padded to 32 rows, not 64, and takes a
	 * 32-row build of the same kernel: its GEMMs wait on the K loop, so the pad was half of
	 * every step's work for nothing. Where N is 1024 or less there are 32 blocks or fewer,
	 * and a 64-deep step halves the trips round the loop. Bit-identical — a row's sums are
	 * its own — and 0.7-1.2 ms of a 12-20 ms graph (notes/improve-b.md). */
	int small = g.staged32 && g.rstaged32[0] && M <= 32 && !(bt & 0x400000u)
		    && (M == 32 || g.staged_partial);
	/* A bottleneck of 64 tokens — 320x320, the vendor's minimum — has two GEMMs with
	 * N = 1024 and 32 blocks of 64 rows between eight cores; two 32-row blocks with the
	 * 64-deep step each take them from 184 to 160 us and from 50 to 46 (improve-b.md). */
	if (g.staged32 && g.rstaged32[1] && M == 64 && N <= 1024 && K % 64 == 0 && K >= 1024
	    && !(bt & 0x500000u))
		small = 1;
	int staged = (M % 64 == 0 || small || g.staged_partial)
		     && N % 32 == 0 && K % 32 == 0 && K >= g.staging;
	/* The staged kernel has no portable twin — its 128-lane 64x32 geometry exists to feed
	 * matrix units — and needs VK_KHR_workgroup_memory_explicit_layout; without either,
	 * every shape goes to the 8x16 and 16x32 kernels, whose portable builds take the
	 * same dispatch. */
	if (!g.staged_ok) staged = 0;
	/* The window gather lives in the staged kernel's A loader, at any depth of K. Without
	 * matrix units the tiled portable kernel gathers in its own scalar A loads instead;
	 * the cooperative-matrix tiled kernel cannot, so a matrix device without the staged
	 * kernel refuses it (`xmx_window_gather()` tells the caller beforehand). */
	int window_tiled = 0;
	if (bt & 0x400000u) {
		if (!window_a || M % 64 || N % 32 || K % 32 || batch > 1 || (bt & 1u))
			FAIL("a window-gathered A needs its geometry, whole blocks and one batch", 0);
		if (g.staged_ok) staged = 1;
		else if (g.portable) window_tiled = 1;
		else FAIL("a window-gathered A needs the staged kernel, and this matrix device lacks "
			  "VK_KHR_workgroup_memory_explicit_layout (xmx_window_gather() is 0)", 0);
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
		staged = tiled = 0;  /* its shared-memory scatter is one 8x16 tile */
	}
	/* The QKV epilogue normalises a head inside one workgroup, so the column block must
	 * be one head exactly: 32 wide, which the staged block always is and the tiled one
	 * is unless XMX_TILE_N moved it. The 8x16 kernel never qualifies. */
	if ((bt & 0x100000u) && !(qkv && (bt & ~0x40e000u) == 0x100000u
				  && (staged || (tiled && g.tilen == 32))))
		FAIL("QKV epilogue needs its targets, no other flag, and a 32-column block", 0);
	if ((bt & 0x4000u) && !(bt & 0x400000u))
		FAIL("an A published on the way in is a window-gathered one", 0);
	/* the half copy lives in gemm_resident.comp's plain-store path only */
	if ((bt & 0x200000u) && !(half_copy >= 0 && bt == 0x200000u && !staged))
		FAIL("a GEMM half copy needs its target, no other flag, and the resident kernel", 0);
	/* The pool is the 64-row staged kernel's epilogue on the matrix path; without it, the
	 * tiled portable kernel's, whose 16-row block is two rows of one window (the same
	 * condition `xmx_window_gather()` reports). */
	if ((bt & 0x800000u) && staged && small)
		FAIL("a pooled window residual runs on the 64-row staged kernel", 0);
	if ((bt & 0x800000u) && !staged) {
		if (!g.portable || !tiled || g.tilem != 16u || g.tilen != 32u)
			FAIL("a pooled window residual needs the staged kernel, or the portable 16x32 "
			     "block (xmx_window_gather() is 0)", 0);
	}
	small = small && staged;
	int deep = small && N <= 1024 && K % 64 == 0;
	/* the portable 16x64 build, for what its generic store takes: not the QKV epilogue or
	 * the pool (both need a 32-column block) nor the compact head */
	int wide = g.rwide && !staged && M % 16u == 0 && N % 64u == 0 && K >= g.tiling
		   && !(bt & 0x910000u);
	VkPipeline pipeline;
	if (small) {
		if (resident_pipeline(6 + deep, bt, g.rstaged32[deep], &pipeline)) return -1;
	} else if (wide) {
		if (resident_pipeline(12, bt, g.rwide, &pipeline)) return -1;
	} else if (resident_pipeline(staged ? 2 : (tiled ? 1 : 0), bt,
				     staged ? g.rstaged : (tiled ? g.rtiled : g.rgemm), &pipeline)) {
		return -1;
	}
	vkCmdBindPipeline(g.rcb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdPushConstants(g.rcb, g.rpl, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof p, &p);
	g.staged32_calls += small;
	if (staged) {
		unsigned rows = small ? 32u : 64u;
		vkCmdDispatch(g.rcb, N / 32, (M + rows - 1) / rows, batch ? batch : 1);
	} else if (wide) {
		vkCmdDispatch(g.rcb, N / 64u, M / 16u, batch ? batch : 1);
	} else {
		if (tiled) vkCmdDispatch(g.rcb, N / g.tilen, M / g.tilem, batch ? batch : 1);
		else       vkCmdDispatch(g.rcb, (N + 15) / 16, (M + 7) / 8, batch ? batch : 1);
	}
	g.wide_calls += wide;
	barrier();
	stamp(staged ? PK_STAGED : ((tiled || wide) ? PK_TILED : PK_GEMM), bt);
	g.recorded++;
	return 0;
}

int xmx_rec_gemm(int a, int b, int c, unsigned M, unsigned N, unsigned K, unsigned batch,
		 unsigned sa, unsigned sb, unsigned sc, unsigned bt,
		 unsigned lda, unsigned ldb, unsigned ldc,
		 unsigned oa, unsigned ob, unsigned oc)
{
	return record_gemm(a, b, c, M, N, K, batch, sa, sb, sc, bt,
			   lda, ldb, ldc, oa, ob, oc, -1, -1, NULL, NULL, -1, NULL, -1);
}

/* A dense projection whose epilogue adds `skip * cosine` before the publish: the
 * `branch + skip * per_channel_cosine` of the residual, without writing the float32
 * branch out and reading it back in a pass of its own. Ported from ProjectsCodex's
 * phase38; bit-identical to the two-pass path (`src/gpu/test_gemm_residual.py`).
 * A half skip is the caller's to flag with 0x40000. */
int xmx_rec_gemm_residual(int a, int b, int c, int skip, int cosine,
			  unsigned M, unsigned N, unsigned K, unsigned flags)
{
	if (skip < 0 || cosine < 0) FAIL("invalid residual buffer", 0);
	return record_gemm(a, b, c, M, N, K, 1, M * K, K * N, M * N,
			   flags | 0x20000u, 0, 0, 0, 0, 0, 0, skip, cosine, NULL, NULL, -1, NULL, -1);
}

/* The same, for a window block's output projection: its rows are in window order,
 * padded, and the result is written straight back into the unpadded image, which is
 * where the residual stream lives. `across` is windows per row, `pad` packs the top
 * and left padding as (top << 16) | left. ProjectsCodex's phase39. */
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

/* The same for block 0, whose output has exactly two readers: the encoder, which pools
 * it 2x2 and publishes the pool, and the last block, which reads it published. Both come
 * out of this epilogue (gemm_staged.comp, 0x800000) — `skip_out` takes every value
 * published, as half, `pooled` the published pool — so the float32 output itself is
 * never stored, nor read back by a pass of its own. */
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

/* The QKV projection with Q and K normalised and V published in its own epilogue
 * (`qkv_epilogue.glsl`), so the float32 projection never goes to memory and the two
 * cosine publishes and the V split that read it back are not recorded at all. `M` rows
 * of `channels` in, (window, head, token, 32) E4M3 halves out, `tokens` per window. */
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

/* `vit`: the ViT's normalisation and query scale (qkv_epilogue.glsl, flag 0x2000). */
int xmx_rec_gemm_qkv(int a, int weight, int q, int k, int v, int scale,
		     unsigned M, unsigned channels, unsigned heads, unsigned tokens, unsigned vit)
{
	return record_gemm_qkv(a, weight, q, k, v, scale, M, channels, heads, tokens, NULL,
			       vit ? 0x2000u : 0u);
}

/* The same, with A gathered from the image — `width` x `height`, `channels` deep, float32
 * or (bit 0 of `image`, 0x8000) half, and with bit 1 (0x4000) published to E4M3 as it
 * is loaded — in the window order of `across` windows a row and `pad` as
 * (top << 16) | left: the partition folded into the projection's own loads. */
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

/* A plain GEMM whose float32 result is also stored as half into `half_copy`: for a
 * value the graph needs both ways, as a residual and as the next GEMM's operand. */
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
	/* a pass that writes two outputs finds the second at offset 96, which only the
	 * GEMMs' residual epilogue uses otherwise */
	if (second >= 0 && !(p.residual_cos = addr_of(second)))
		FAIL("second unary output is not a live buffer", 0);
	VkPipeline pipeline;
	if (resident_pipeline(3, kind, g.runary, &pipeline)) return -1;
	vkCmdBindPipeline(g.rcb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdPushConstants(g.rcb, g.rpl, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof p, &p);
	vkCmdDispatch(g.rcb, (n + 255) / 256, 1, 1);
	barrier();
	stamp(PK_UNARY, kind);
	g.recorded++;
	return 0;
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

/* Row-wise passes: the cosine publish reduces 32 channels through the kernel's own
 * fragment tree, the softmax reduces a window's tokens. One invocation per row. */
/* One dispatch for independent Q, K and V workgroup planes. The fifth pointer
 * occupies byte offset 80, the lda/ldb fields unused by attention.comp. */
int xmx_rec_qkv(int source, int q, int k, int v, int scale,
		unsigned rows, unsigned tokens, unsigned heads, unsigned vit)
{
	if (!g.recording) FAIL("not recording", 0);
	if (!rows || !tokens || !heads) FAIL("invalid QKV extent", 0);
	/* 0x80000: the ViT's normalisation and query scale (attention.comp) */
	const unsigned flags = 2u | 0x1000u | 0x20000u | (vit ? 0x80000u : 0u);
	VkDeviceAddress scale_addr = addr_of(scale);
	struct push p = { .a = addr_of(source), .b = addr_of(q), .c = addr_of(k),
		.d = addr_of(v), .m = rows, .n = tokens, .batch = heads, .flags = flags,
		.lda = (uint32_t)scale_addr, .ldb = (uint32_t)(scale_addr >> 32) };
	if (!p.a || !p.b || !p.c || !p.d || !scale_addr) FAIL("QKV operand is not live", 0);
	VkPipeline pipeline;
	if (resident_pipeline(4, flags, g.rrow, &pipeline)) return -1;
	vkCmdBindPipeline(g.rcb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdPushConstants(g.rcb, g.rpl, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof p, &p);
	vkCmdDispatch(g.rcb, (rows + 31) / 32, 3, 1);
	barrier();
	stamp(PK_ROW, 2);
	g.recorded++;
	return 0;
}

int xmx_rec_row(unsigned kind, int a, int b, int c, int d, unsigned rows, unsigned width,
		unsigned heads, unsigned scaled, unsigned stride, float cap)
{
	if (!g.recording) FAIL("not recording", 0);
	struct push p = { .a = addr_of(a), .b = addr_of(b), .c = addr_of(c), .d = addr_of(d),
			  .m = rows, .n = width, .k = scaled, .batch = heads, .flags = kind,
			  .sa = stride, .p0 = cap };
	if (!p.a || !p.c) FAIL("row operand is not a live buffer", 0);
	/* A softmax over rows too wide to stage 32 at a time takes as many whole rows as fit in
	 * the pass's 8 KB of shared memory, `stride + 1` floats apart below the last 32, on the
	 * 256-lane build (attention.comp, softmax_rows); anything wider keeps one row a lane. */
	unsigned row_stride = stride ? stride : width, groups = (rows + 31) / 32;
	int whole = (kind & 0xFFu) == 1u && (row_stride != width || row_stride > 64u)
		    && row_stride + 1u <= 2016u;
	if (whole) {
		unsigned per = 2016u / (row_stride + 1u);
		if (per > 32u) per = 32u;       /* the last 32 floats hold one reciprocal a row */
		groups = (rows + per - 1u) / per;
	}
	VkPipeline pipeline;
	if (whole && g.rrows) {
		if (resident_pipeline(8, kind, g.rrows, &pipeline)) return -1;
	} else if (resident_pipeline(4, kind, g.rrow, &pipeline)) {
		return -1;
	}
	vkCmdBindPipeline(g.rcb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdPushConstants(g.rcb, g.rpl, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof p, &p);
	vkCmdDispatch(g.rcb, groups, 1, 1);
	barrier();
	stamp(PK_ROW, kind);
	g.recorded++;
	return 0;
}

/* The fused passes' matrix kernels alias their shared memory as several typed blocks
 * (GL_EXT_shared_memory_block), which needs VK_KHR_workgroup_memory_explicit_layout. A
 * matrix device without it — or an adopting host that did not pass
 * XMX_ADOPT_EXPLICIT_LAYOUT — runs the `_portable` twin instead: the same arithmetic in
 * the portable GEMM's summation order, so correct, but not bit-identical to the matrix
 * path's unfused passes. Returns a malloc'd path, and whether it is the twin. */
static char *fused_path(const char *path, int *twin)
{
	if (g.portable || g.explicit_layout || strstr(path, "_portable")) {
		*twin = g.portable || strstr(path, "_portable") != NULL;
		return nr_strdup(path);
	}
	size_t n = strlen(path), stem = n;
	if (n >= 4 && !strcmp(path + n - 4, ".spv")) stem = n - 4;
	char *out = malloc(n + sizeof "_portable");
	if (!out) return NULL;
	memcpy(out, path, stem);
	memcpy(out + stem, "_portable", sizeof "_portable" - 1);
	memcpy(out + stem + sizeof "_portable" - 1, path + stem, n - stem + 1);
	*twin = 1;
	fprintf(stderr, "libxmx: no VK_KHR_workgroup_memory_explicit_layout; %s runs as %s\n",
		path, out);
	return out;
}

/* A fused pass's workgroup against the device's limits, so a device that cannot hold it
 * says so at build time rather than failing pipeline creation with a bare VkResult. */
static int fused_fits(const char *what, uint32_t bytes, uint32_t lanes)
{
	if (g.max_shared && g.max_shared < bytes) {
		snprintf(g.err, sizeof g.err, "%s needs %u bytes of shared memory a workgroup; the "
			 "device has %u", what, bytes, g.max_shared);
		return -1;
	}
	if (g.max_invocations && g.max_invocations < lanes) {
		snprintf(g.err, sizeof g.err, "%s needs %u invocations a workgroup; the device "
			 "allows %u", what, lanes, g.max_invocations);
		return -1;
	}
	return 0;
}

/* Window attention's QK^T, softmax and PV in one dispatch (ProjectsCodex's phase42,
 * `window_attention.comp`): one 256-lane workgroup takes a window and head, its K and V
 * loaded once into 16 KB of shared memory, and each of its eight subgroups eight query
 * rows; the scores and probabilities stay in shared memory instead of making two round
 * trips through device buffers.
 *
 * The portable twin keeps one 32-lane workgroup for each eight rows, K and V read from
 * device memory. The one-workgroup-a-window form was built for it too (K transposed in
 * shared memory, the weights made in registers, bit-exact) and measured on the only
 * portable Vulkan device here, an M3 through MoltenVK: a 1280x768 frame 614 ms against
 * 602 with this kernel — the reverse of Metal, whose own portable kernel does gain from
 * it (window_attention.metal).
 *
 * Built on first use, so a graph that keeps the
 * three-pass path never compiles it. The bias pointer rides in the residual epilogue's
 * slot, `residual_cos` at offset 96, which is why that field was appended rather than
 * inserted.
 *
 * `merged` selects the same shader with `MERGED_OUTPUT` specialised on: it also does the
 * head merge's work, publishing E4M3 in (window, token, C) order, so neither the FP32
 * context nor the merge_heads pass that read it back is needed. */
int xmx_window_init(const char *path, unsigned merged)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (merged > 1) FAIL("invalid window attention output mode", 0);
	if (g.rwindow[merged]) return 0;
	/* the twin is 32 lanes and 2 KB; the matrix kernel's workgroup is the one to check */
	if (!g.portable && fused_fits("window attention", 16384u, 256u)) return -1;
	int twin;
	char *use = fused_path(path, &twin);
	if (!use) FAIL("pipeline path allocation", 0);
	/* a bool specialization constant is a VkBool32, which is what `merged` already is */
	VkSpecializationMapEntry entry = { .constantID = 0, .offset = 0, .size = sizeof merged };
	VkSpecializationInfo info = { .mapEntryCount = 1, .pMapEntries = &entry,
				      .dataSize = sizeof merged, .pData = &merged };
	int r = build_pipeline_spec(use, g.rpl, &g.rwindow[merged], &info);
	free(use);
	g.rwindow_twin[merged] = twin;
	return r;
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
	vkCmdBindPipeline(g.rcb, VK_PIPELINE_BIND_POINT_COMPUTE, g.rwindow[merged]);
	vkCmdPushConstants(g.rcb, g.rpl, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof p, &p);
	/* one 256-lane workgroup a window and head, its eight subgroups sharing K and V; the
	 * portable twin keeps eight 32-lane workgroups of eight rows (see xmx_window_init) */
	vkCmdDispatch(g.rcb, g.rwindow_twin[merged] ? 8u : 1u, batches < 65535u ? batches : 65535u,
		      1u + (batches - 1u) / 65535u);
	barrier();
	stamp(PK_ROW, 3);        /* WINDOW_ATTENTION, as window_attention.comp names it */
	g.recorded++;
	return 0;
}

/* A 32-channel window block's attention half in one pass a window (`window_block.comp`):
 * eight subgroups, a window row each, 16 KB of shared memory; the portable twin eight
 * 32-lane slices and 28.25 KB, the same dispatch. Built on first use, from the twin where
 * `fused_path` says so. The whole-block variants — block 0 with its pool, block 70 with
 * the head — are specialisations of the same pipeline. */
int xmx_window_block_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rblock) return 0;
	int twin;
	char *use = fused_path(path, &twin);
	if (!use) FAIL("pipeline path allocation", 0);
	if (fused_fits("the window block", twin ? 28928u : 16384u, 256u)) {
		free(use);
		return -1;
	}
	free(g.rpaths[10]);
	g.rpaths[10] = use;
	return build_pipeline(use, g.rpl, &g.rblock);
}

/* A 32-channel window block's attention half, one 8x8 window a workgroup
 * (window_block.comp): the QKV projection gathering its rows from `image`, window
 * attention, and the output projection with the residual into `target`. `image` is also
 * the residual's skip, as it is in the graph. */
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
	/* the head (0x1000000) is block 70's: `pooled` carries the head's weights, and the
	 * target is the compact head, four float32 columns a pixel */
	if ((flags & 0x1000000u) && (pooled < 0 || (flags & 0x801f00u)))
		FAIL("a window block with the head needs its weights, no publish and no pool", 0);
	/* the pool (0x800000) is block 0's: the target takes the published skip as half */
	if ((flags & 0x800000u) && (pooled < 0 || (flags & 0x1f00u) || height % 2u || width % 2u
				    || (pad >> 16) % 2u || (pad & 0xffffu) % 2u))
		FAIL("a pooled window block needs its pool, no publish, even extents and pads", 0);
	struct push p = { .a = addr_of(image), .b = addr_of(qkv), .c = addr_of(target),
			  .d = addr_of(bias), .m = windows, .n = 32u, .k = 32u, .batch = 1u,
			  .flags = flags, .residual_cos = addr_of(cosine), .image_h = height,
			  .image_w = width, .window_cols = across, .window_pad = pad,
			  .qkv_scale = addr_of(scale) };
	/* the shader reads p0-p1 and p2-p3 as 64-bit addresses: the output projection and
	 * the pool */
	uint64_t weights = addr_of(projection), pool = pooled >= 0 ? addr_of(pooled) : 0;
	memcpy(&p.p0, &weights, sizeof weights);
	memcpy(&p.p2, &pool, sizeof pool);
	if (!p.a || !p.b || !p.c || !p.d || !weights || !p.residual_cos || !p.qkv_scale
	    || ((flags & 0x1800000u) && !pool))
		FAIL("window block operand is not a live buffer", 0);
	VkPipeline pipeline;
	if (resident_pipeline(10, flags, g.rblock, &pipeline)) return -1;
	vkCmdBindPipeline(g.rcb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdPushConstants(g.rcb, g.rpl, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof p, &p);
	vkCmdDispatch(g.rcb, windows < 65535u ? windows : 65535u, (windows + 65534u) / 65535u, 1);
	barrier();
	stamp(PK_ROW, 4);        /* WINDOW_BLOCK, as window_block.comp names it */
	g.recorded++;
	return 0;
}

int xmx_global_attention_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rglobal) return 0;
	int twin;
	char *use = fused_path(path, &twin);
	if (!use) FAIL("pipeline path allocation", 0);
	if (fused_fits("global attention", twin ? 24832u : 16384u, 256u)) {
		free(use);
		return -1;
	}
	free(g.rpaths[11]);
	g.rpaths[11] = use;
	return build_pipeline(use, g.rpl, &g.rglobal);
}

/* A bottleneck block's attention in one pass (global_attention.comp): Q, K and V as the
 * QKV epilogue leaves them, (heads, rows, 32) half, into the merged (rows, heads * 32)
 * half — what QK^T, the ViT's softmax over `tokens` of `rows` columns, PV and the scaled
 * merge_heads write. The rows reach no further than the tokens' last 64-block.
 * A 256-lane workgroup a head and 64 query rows, on the matrix kernel and its twin alike. */
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
	vkCmdBindPipeline(g.rcb, VK_PIPELINE_BIND_POINT_COMPUTE, g.rglobal);
	vkCmdPushConstants(g.rcb, g.rpl, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof p, &p);
	vkCmdDispatch(g.rcb, (rows + 63u) / 64u, heads, 1);
	barrier();
	stamp(PK_ROW, 5);        /* GLOBAL_ATTENTION, as global_attention.comp names it */
	g.recorded++;
	return 0;
}

int xmx_int8_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rint8) return 0;
	/* The integer matrix path needs matrix units, the explicit layout and the 8-bit
	 * features; anything else builds the twin, which reads the bytes as words. */
	int twin = !(g.staged_ok && g.int8) || strstr(path, "_portable") != NULL;
	char *use;
	if (twin && !strstr(path, "_portable")) {
		size_t n = strlen(path), stem = n;
		if (n >= 4 && !strcmp(path + n - 4, ".spv")) stem = n - 4;
		if (!(use = malloc(n + sizeof "_portable"))) FAIL("pipeline path allocation", 0);
		memcpy(use, path, stem);
		memcpy(use + stem, "_portable", sizeof "_portable" - 1);
		memcpy(use + stem + sizeof "_portable" - 1, path + stem, n - stem + 1);
	} else if (!(use = nr_strdup(path))) {
		FAIL("pipeline path allocation", 0);
	}
	free(g.rpaths[9]);
	g.rpaths[9] = use;
	g.rint8_twin = twin;
	return build_pipeline(use, g.rpl, &g.rint8);
}

/* C = (A @ B^T) * a_scale[row] * b_scale[column] on configuration 4: A int8 M x K, B the
 * weights stored transposed, int8 N x K, the scales float32 (gemm_staged_int8.comp). */
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
	vkCmdBindPipeline(g.rcb, VK_PIPELINE_BIND_POINT_COMPUTE, g.rint8);
	vkCmdPushConstants(g.rcb, g.rpl, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof p, &p);
	/* the matrix kernel's 64x32 block, or the twin's 8x16 */
	if (g.rint8_twin) vkCmdDispatch(g.rcb, N / 16u, (M + 7u) / 8u, 1);
	else              vkCmdDispatch(g.rcb, N / 32u, (M + 63u) / 64u, 1);
	barrier();
	stamp(PK_GEMM, 30);
	g.recorded++;
	return 0;
}

/* A 32-channel block's whole feed-forward in one dispatch (`ffn_fused.comp`): the expand,
 * its gate and publish, the projection and the residual, with the hidden layer kept in
 * shared memory instead of written out as half and read back. One subgroup per 16 rows —
 * sixteen of them a 512-lane workgroup on the matrix kernel (32 KB), eight 32-lane slices
 * a 256-lane one on the portable twin (24 KB) — and for the narrow blocks' shape (32
 * channels, 128 hidden; flag 0x800000) both weight matrices loaded once into the
 * workgroup's shared memory. The blocks are on the x axis of the grid, whose limit is
 * 2^31-1 rather than y's 65535. Its profile
 * stamp is the base GEMM family's kind 31, which no plain GEMM's flags can reach. */
int xmx_ffn_init(const char *path)
{
	if (!g.rready) FAIL("resident runtime not initialised", 0);
	if (g.rffn) return 0;
	int twin;
	char *use = fused_path(path, &twin);
	if (!use) FAIL("pipeline path allocation", 0);
	if (fused_fits("the fused feed-forward", twin ? 24576u : 32768u, twin ? 256u : 512u)) {
		free(use);
		return -1;
	}
	free(g.rpaths[5]);
	g.rpaths[5] = use;
	g.rffn_twin = twin;
	return build_pipeline(use, g.rpl, &g.rffn);
}

int xmx_rec_ffn(int a, int expand, int projection, int out, int skip, int cosine,
		unsigned M, unsigned cin, unsigned hidden, unsigned groups, unsigned flags)
{
	if (!g.recording || !g.rffn) FAIL("fused feed-forward not ready for recording", 0);
	if (!cin || cin % 16u || !hidden || hidden % 32u || !M || M % 16u || !groups)
		FAIL("fused feed-forward needs channels a multiple of 16, hidden of 32, 16-row blocks", 0);
	if (flags & ~0x41f00u)
		FAIL("fused feed-forward takes an epilogue, a half output and a half skip only", 0);
	/* The residual reads its skip in the output's own layout, which is only the dense
	 * one when there is a single group of 32 channels. */
	int residual = skip >= 0 || cosine >= 0;
	if (residual && (groups != 1u || cin != 32u || skip < 0 || cosine < 0))
		FAIL("the fused residual is for one group of 32 channels, with skip and cosine", 0);
	if (!residual && (flags & 0x40000u)) FAIL("a half skip without a residual", 0);
	/* the narrow blocks' shape: both weight matrices fit the workgroup's shared memory */
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
	VkPipeline pipeline;
	if (resident_pipeline(5, p.flags, g.rffn, &pipeline)) return -1;
	vkCmdBindPipeline(g.rcb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdPushConstants(g.rcb, g.rpl, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof p, &p);
	/* sixteen subgroups of 16 rows a workgroup (eight on the portable twin); the last
	 * one's surplus row blocks return, or on the twin idle through its barriers */
	unsigned rows = g.rffn_twin ? 128u : 256u;
	vkCmdDispatch(g.rcb, (M + rows - 1u) / rows, groups, 1);
	barrier();
	stamp(PK_GEMM, 31);
	g.recorded++;
	return 0;
}

/* Block 70's feed-forward with its input made in the same pass (ffn_fused.comp, flag
 * 0x1000000): `source` is the level above at half the extent (half), `skip` the
 * full-resolution skip (half), `sincos` the merge's sin then cos, `cosine` the feed-
 * forward residual's. What `UPSAMPLE_MERGE` followed by `xmx_rec_ffn` with a float32
 * skip writes, without the merge ever being stored. */
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
	/* the shader reads p0 and p1 as one 64-bit address, the merge's sin-cos table */
	uint64_t table = addr_of(sincos);
	memcpy(&p.p0, &table, sizeof table);
	if (!p.a || !p.b || !p.c || !p.d || !table || !p.residual_cos || !p.qkv_scale)
		FAIL("merged feed-forward operand is not a live buffer", 0);
	VkPipeline pipeline;
	if (resident_pipeline(5, p.flags, g.rffn, &pipeline)) return -1;
	vkCmdBindPipeline(g.rcb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdPushConstants(g.rcb, g.rpl, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof p, &p);
	vkCmdDispatch(g.rcb, (M + (g.rffn_twin ? 127u : 255u)) / (g.rffn_twin ? 128u : 256u), 1, 1);
	barrier();
	stamp(PK_GEMM, 31);
	g.recorded++;
	return 0;
}

/* Block 0's feed-forward with its input made in the same pass (ffn_fused.comp, flag
 * 0x2000000): the stem, `features` (half, rows x 16) times `adapter` (16 x 32), which
 * `xmx_rec_gemm_dual` wrote as float32 and half for this pass to read back. */
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
	/* the shader reads p0 and p1 as one 64-bit address, the adapter */
	uint64_t weights = addr_of(adapter);
	memcpy(&p.p0, &weights, sizeof weights);
	if (!p.a || !p.b || !p.c || !weights || !p.residual_cos || !p.qkv_scale)
		FAIL("stem feed-forward operand is not a live buffer", 0);
	VkPipeline pipeline;
	if (resident_pipeline(5, p.flags, g.rffn, &pipeline)) return -1;
	vkCmdBindPipeline(g.rcb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdPushConstants(g.rcb, g.rpl, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof p, &p);
	vkCmdDispatch(g.rcb, (M + (g.rffn_twin ? 127u : 255u)) / (g.rffn_twin ? 128u : 256u), 1, 1);
	barrier();
	stamp(PK_GEMM, 31);
	g.recorded++;
	return 0;
}

/* The temporal path's five-tap reprojection: one invocation per pixel. */
int xmx_rec_history(int history, int motion, int out, unsigned pixels, unsigned channels,
		    unsigned height, unsigned width, unsigned absolute)
{
	if (!g.recording) FAIL("not recording", 0);
	struct push p = { .a = addr_of(history), .b = addr_of(motion), .c = addr_of(out),
			  .m = pixels, .n = channels, .k = height, .batch = width,
			  .flags = absolute };
	if (!p.a || !p.b || !p.c) FAIL("history operand is not a live buffer", 0);
	vkCmdBindPipeline(g.rcb, VK_PIPELINE_BIND_POINT_COMPUTE, g.rhistory);
	vkCmdPushConstants(g.rcb, g.rpl, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof p, &p);
	vkCmdDispatch(g.rcb, (pixels + 63) / 64, 1, 1);
	barrier();
	stamp(PK_HISTORY, absolute & 1u);
	g.recorded++;
	return 0;
}

/* Read the timestamps back and add each pass to its kind's running total. Called only
 * once the fence has signalled, so every query is available; WAIT is passed anyway
 * because a driver may still report a query as not-ready immediately after. */
static void collect(unsigned stamps, const unsigned char *kinds)
{
	if (!g.prof || !g.qpool || stamps < 2) return;
	static uint64_t ticks[MAX_STAMPS];
	if (vkGetQueryPoolResults(g.dev, g.qpool, 0, stamps, sizeof ticks[0] * stamps, ticks,
				  sizeof ticks[0],
				  VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT) != VK_SUCCESS)
		return;
	for (unsigned i = 1; i < stamps; i++) {
		/* a wrapped counter is not a duration; -1 keeps the order for the caller */
		double ms = ticks[i] < ticks[i - 1] ? -1.0
			  : (double)(ticks[i] - ticks[i - 1]) * g.ts_period * 1e-6;
		if (prof_each_n < MAX_STAMPS) {
			prof_each_kind[prof_each_n] = kinds[i];
			prof_each[prof_each_n++] = ms;
		}
		if (ms < 0) continue;
		prof_ms[kinds[i]] += ms;
		prof_hits[kinds[i]]++;
	}
}

static int submit_commands(VkCommandBuffer commands, int passes)
{
	VkResult r = vkResetFences(g.dev, 1, &g.rfence);
	if (r) FAIL("reset resident fence", r);
	VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1,
			    .pCommandBuffers = &commands };
	qlock();
	r = vkQueueSubmit(g.q, 1, &si, g.rfence);
	qunlock();
	if (r) FAIL("resident submit", r);
	if ((r = vkWaitForFences(g.dev, 1, &g.rfence, VK_TRUE, 60ull * 1000000000ull)))
		FAIL("resident fence wait", r);
	return passes;
}

int xmx_submit(void)
{
	if (!g.recording) FAIL("not recording", 0);
	g.recording = 0;
	VkResult r = vkEndCommandBuffer(g.rcb);
	if (r) FAIL("end resident recording", r);
	int passes = submit_commands(g.rcb, g.recorded);
	if (passes >= 0) collect(g.prof_n, stamp_kind);
	return passes;
}

/* A graph owns its command buffer; later recording (including temporal history)
 * uses a different one. Callers keep referenced buffers alive until graph_destroy. */
int xmx_graph_capture(void)
{
	if (!g.recording) FAIL("not recording", 0);
	int id;
	for (id = 0; id < MAX_GRAPHS && graphs[id].commands; id++);
	if (id == MAX_GRAPHS) FAIL("out of graph slots", 0);
	VkCommandBuffer replacement;
	VkCommandBufferAllocateInfo ai = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = g.cpool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
	VkResult r = vkAllocateCommandBuffers(g.dev, &ai, &replacement);
	if (r) FAIL("graph command buffer", r);
	r = vkEndCommandBuffer(g.rcb);
	if (r) { vkFreeCommandBuffers(g.dev, g.cpool, 1, &replacement); FAIL("end graph recording", r); }
	graphs[id].commands = g.rcb;
	graphs[id].passes = g.recorded;
	graphs[id].stamps = g.prof_n;
	free(graphs[id].kinds);
	graphs[id].kinds = NULL;
	if (g.prof_n) {
		graphs[id].kinds = malloc(g.prof_n);
		if (graphs[id].kinds) memcpy(graphs[id].kinds, stamp_kind, g.prof_n);
		else graphs[id].stamps = 0;
	}
	g.rcb = replacement;
	g.recording = 0;
	return id;
}

int xmx_graph_run(int id)
{
	if (g.recording) FAIL("cannot replay during recording", 0);
	if (id < 0 || id >= MAX_GRAPHS || !graphs[id].commands) FAIL("graph is not live", 0);
	int passes = submit_commands(graphs[id].commands, graphs[id].passes);
	if (passes >= 0 && graphs[id].kinds) collect(graphs[id].stamps, graphs[id].kinds);
	return passes;
}

/* Turn GPU-side profiling on or off.
 *
 * Off by default and free when off: `stamp()` returns on the first test. On, every
 * recorded pass gains one timestamp query, which is a command-buffer write and not a
 * synchronisation point, so the frame it measures is the frame that would have run.
 *
 * Returns 0, or -1 if the queue family cannot timestamp at all — some do not, and a
 * silent zero would be worse than a refusal.
 */
int xmx_profile(int on)
{
	if (!on) { g.prof = 0; return 0; }
	if (!g.dev) FAIL("profiling needs an initialised device", 0);
	if (!g.qpool) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(g.pd, &props);
		if (props.limits.timestampPeriod == 0.0f)
			FAIL("this device does not support timestamps", 0);
		uint32_t families = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(g.pd, &families, NULL);
		VkQueueFamilyProperties *qf = malloc(families * sizeof *qf);
		if (!qf) FAIL("queue family properties", 0);
		vkGetPhysicalDeviceQueueFamilyProperties(g.pd, &families, qf);
		uint32_t bits = g.qi < families ? qf[g.qi].timestampValidBits : 0;
		free(qf);
		if (!bits) FAIL("this queue family cannot write timestamps", 0);
		g.ts_period = props.limits.timestampPeriod;
		/* MoltenVK backs a timestamp pool with one MTLCounterSampleBuffer, which Metal caps
		 * at 32 KB — 4096 queries. Asked for more it logs an error, "reverts to emulated
		 * behavior" and every stamp reads zero, so the profile is a table of noughts. A
		 * frame is 300-600 passes, so 4096 loses nothing; `XMX_STAMPS` sets it by hand. */
		g.qcount = g.moltenvk ? 4096u : MAX_STAMPS;
		const char *stamps_env = getenv("XMX_STAMPS");
		if (stamps_env && *stamps_env) {
			unsigned want = (unsigned)atoi(stamps_env);
			if (want >= 2u && want <= MAX_STAMPS) g.qcount = want;
			else fprintf(stderr, "libxmx: XMX_STAMPS=%s is not 2-%u; keeping %u\n",
				     stamps_env, (unsigned)MAX_STAMPS, g.qcount);
		}
		VkQueryPoolCreateInfo qi = { .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
					     .queryType = VK_QUERY_TYPE_TIMESTAMP,
					     .queryCount = g.qcount };
		VkResult r = vkCreateQueryPool(g.dev, &qi, NULL, &g.qpool);
		if (r) FAIL("timestamp query pool", r);
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

/* Milliseconds and pass count for one kind, where a kind is `family * 32 + subkind`.
 * Reading a kind that never ran gives 0, which is the honest answer. */
unsigned xmx_profile_each_count(void) { return prof_each_n; }
double xmx_profile_each_ms(unsigned i) { return i < prof_each_n ? prof_each[i] : -1.0; }
unsigned xmx_profile_each_kind(unsigned i) { return i < prof_each_n ? prof_each_kind[i] : 0u; }

double xmx_profile_ms(unsigned kind)
{
	return kind < PROF_KINDS ? prof_ms[kind] : 0.0;
}

unsigned xmx_profile_count(unsigned kind)
{
	return kind < PROF_KINDS ? prof_hits[kind] : 0u;
}

int xmx_graph_destroy(int id)
{
	if (id < 0 || id >= MAX_GRAPHS || !graphs[id].commands) return 0;
	vkFreeCommandBuffers(g.dev, g.cpool, 1, &graphs[id].commands);
	graphs[id].commands = VK_NULL_HANDLE;
	free(graphs[id].kinds);
	graphs[id].kinds = NULL;
	graphs[id].stamps = 0;
	return 0;
}
