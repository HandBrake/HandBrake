#define _GNU_SOURCE
/*
 * nr_layer — a Vulkan layer that hands the presented frame to DLSS-NR.
 *
 * Why a Vulkan layer, and not the route everyone else takes.
 *
 * Every published way of getting DLSS-NR into a game — OptiScaler's fork, the
 * ReShade bridges, the dual-GPU MGPU Bridge — loads NVIDIA's own nvngx_dlssnr.dll
 * and therefore needs an NVIDIA GPU (or, for the AMD lab, a PTX translation of it).
 * None of them can run here. What we have instead is a reimplementation that is
 * already Vulkan compute.
 *
 * And under Proton a DX12 game goes DX12 -> VKD3D-Proton -> Vulkan on ANV, which is
 * the same driver our shaders run on. So the frame we want is already a VkImage on
 * the device we already use: no D3D interop, no Windows DLL, no PCIe transfer. A
 * layer at vkQueuePresentKHR sees it. vkBasalt has done exactly this on Linux for
 * years, so the shape is proven.
 *
 * This file is the capture half: chain into the loader, force TRANSFER_SRC onto the
 * swapchain images, and copy the presented frame out on demand.
 *
 * Build: cc -O2 -shared -fPIC -o libnr_layer.so nr_layer.c -lvulkan
 */
/* The layer only needs a platform header so vulkan.h stops complaining; it never calls
 * into the windowing system. Windows has no Xlib, and neither have macOS and Android. */
#if defined(_WIN32)
#  define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__) && !defined(__ANDROID__)
#  define VK_USE_PLATFORM_XLIB_KHR   /* pulls in X11 headers, which macOS and Android do not have */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>
/* the frame transport: a Unix socket on POSIX, a named pipe on Windows */
#include "nr_transport.h"

#ifdef _WIN32
#  include <windows.h>
#  include <process.h>
#  include <io.h>
#  ifndef R_OK
#    define R_OK 4
#  endif
#  ifndef W_OK
#    define W_OK 2
#  endif
#  ifndef F_OK
#    define F_OK 0
#  endif
#  ifndef access
#    define access(p, m) _access(p, m)
#  endif
#else
#  include <pthread.h>
#  include <unistd.h>
#  include <sys/stat.h>
#  include <sys/time.h>
#  include <dlfcn.h>   /* dladdr: the layer's own path */
#  include <spawn.h>
#  include <fcntl.h>
#  ifdef __APPLE__
#    include <crt_externs.h>   /* _NSGetEnviron: a dylib has no `environ` symbol of its own */
#  endif
#endif

/* Locking uses the platform's own primitive: an SRWLOCK needs no destroy step and is
 * what this file used on Windows before, a pthread_mutex is what POSIX uses. */
#ifdef _WIN32
typedef SRWLOCK nr_mutex_t;
#define NR_MUTEX_INIT SRWLOCK_INIT
#define nr_mutex_init(m, ...) InitializeSRWLock(m)
#define nr_mutex_lock(m)      AcquireSRWLockExclusive(m)
#define nr_mutex_unlock(m)    ReleaseSRWLockExclusive(m)
#define nr_mutex_destroy(m)   ((void)(m))
#else
typedef pthread_mutex_t nr_mutex_t;
#define NR_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
#define nr_mutex_init(m, ...) pthread_mutex_init(m, NULL)
#define nr_mutex_lock(m)      pthread_mutex_lock(m)
#define nr_mutex_unlock(m)    pthread_mutex_unlock(m)
#define nr_mutex_destroy(m)   pthread_mutex_destroy(m)
#endif

#define MAX_SWAPCHAINS 8
#define MAX_IMAGES 8

/* Capture and writeback consume the game's waits once, complete on private
 * fences, then present with no remaining waits. Legacy NR_LAYER_SYNC values are
 * accepted for launcher compatibility; both use this single verified path. */

struct device_data {
	VkDevice device;
	VkPhysicalDevice physical;
	VkQueue queue;
	uint32_t queue_family;
	nr_mutex_t present_lock;
	unsigned long frame_counter;
	VkResult transfer_error, device_error;
	VkFence stalled_fence;
	VkSwapchainKHR result_chain;
	int queues_announced;           /* the one-time queue inventory has been logged */
	VkCommandPool pool;
	VkDeviceMemory staging_memory;
	VkBuffer staging;
	VkDeviceSize staging_size;
	void *mapped;
	unsigned char *result;          /* the processed frame, held while the trigger is up */
	VkDeviceSize result_size;
	int holding;
	/* One frame captured before the one being processed, so the daemon can be told
	 * which pixels did not move. Filled on the present after the trigger goes up and
	 * released when it goes down; nothing is copied while the trigger is down, so a
	 * game that never triggers pays nothing for this. */
	unsigned char *earlier;
	VkDeviceSize earlier_size;
	const VkSemaphore *present_wait;
	uint32_t present_wait_count;
	int waits_consumed;
	int have_earlier;
	unsigned char *outgoing;        /* colour followed by the mask, for one send */
	VkDeviceSize outgoing_size;
	/* NR_LAYER_ASYNC: the frame sent on the last processed present, whose answer is read
	 * on the next one. Zero-initialised devices must not read fd 0 as a request. */
	int inflight;
	int inflight_fd;
	VkDeviceSize inflight_size;
	PFN_vkGetDeviceProcAddr get_device_proc;
	PFN_vkQueuePresentKHR present;
	PFN_vkCreateSwapchainKHR create_swapchain;
	PFN_vkDestroySwapchainKHR destroy_swapchain;
	PFN_vkGetDeviceQueue get_device_queue;
	PFN_vkGetDeviceQueue2 get_device_queue2;
	PFN_vkGetSwapchainImagesKHR get_swapchain_images;
	PFN_vkDestroyDevice destroy_device;
};

struct swapchain_data {
	VkSwapchainKHR swapchain;
	VkDevice device;
	VkImage images[MAX_IMAGES];
	uint32_t image_count;
	VkFormat format;
	VkExtent2D extent;
};

static struct device_data devices[8];
static struct swapchain_data swapchains[MAX_SWAPCHAINS];

static nr_mutex_t lock = NR_MUTEX_INIT;
static PFN_vkGetInstanceProcAddr next_instance_proc;
static VkInstance layer_instance;
static const char *capture_path;
static long capture_every;
static long live_every;
static int async_live;
static const char *socket_path;
static const char *trigger_path;
static int ui_mask;

/* The frame goes to a daemon over a Unix socket rather than being processed in
 * process: the implementation is Python and this is a shared object living inside the
 * game. For a photo mode the game is meant to stall anyway, so the round trip is free;
 * a per-frame pass would need the graph ported to C. */
/* `reply_size` is deliberately separate from `payload_size`: the interface mask makes
 * the request larger than the answer, and reusing one size meant asking for bytes the
 * daemon never sends — the read hit EOF and every masked frame came back unchanged. */
/* Sends the request and leaves the connection open: in asynchronous live mode the
 * answer is read on the next present, so the endpoint has to outlive this call.
 * Returns the open link, or -1. */
static int exchange_send(const void *header, size_t header_size, const void *payload,
			 size_t payload_size)
{
	nr_link link;
	if (nr_link_connect(&link, socket_path) != 0) {
		fprintf(stderr, "[nr_layer] no daemon at %s\n", socket_path);
		return -1;
	}
	if (nr_link_write(&link, header, header_size) != 0
	    || nr_link_write(&link, payload, payload_size) != 0) {
		nr_link_close(&link);
		return -1;
	}
	return nr_link_handle(link);
}

/* Reads the whole answer and closes the connection, whichever way it ends. */
static int exchange_receive(int fd, void *reply, size_t reply_size)
{
	nr_link link = nr_link_from_handle(fd);
	if (nr_link_read(&link, reply, reply_size) != 0) {
		nr_link_close(&link);
		return -1;
	}
	nr_link_close(&link);
	return 0;
}

static int exchange(const void *header, size_t header_size, const void *payload,
		    size_t payload_size, void *reply, size_t reply_size)
{
	int fd = exchange_send(header, header_size, payload, payload_size);
	return fd < 0 ? -1 : exchange_receive(fd, reply, reply_size);
}

/* An answer still on its way is read to the end and thrown away rather than cut off: the
 * daemon is single-file and would log a broken pipe for it, and the panel reads that log.
 * It costs at most one frame of the daemon's, and only when the effect is switched off,
 * the swapchain changes or the device goes. */
static void drop_inflight(struct device_data *data)
{
	if (!data->inflight) return;
	data->inflight = 0;
	nr_link link = nr_link_from_handle(data->inflight_fd);
	unsigned char sink[65536];
	for (VkDeviceSize got = 0; got < data->inflight_size; ) {
		size_t want = data->inflight_size - got < sizeof sink
			      ? (size_t)(data->inflight_size - got) : sizeof sink;
		if (nr_link_read(&link, sink, want) != 0) break;
		got += want;
	}
	nr_link_close(&link);
}

static struct device_data *find_device(VkDevice device)
{
	for (int i = 0; i < 8; i++)
		if (devices[i].device == device) return &devices[i];
	return NULL;
}

/* Which device and queue family a VkQueue belongs to.
 *
 * `vkQueuePresentKHR` hands over a queue and nothing else. Taking the first live device
 * and the family of `pQueueCreateInfos[0]` is right for the single-queue case and wrong
 * the moment a game asks for a dedicated present or transfer family, which VKD3D-Proton
 * does: the command buffer would then be allocated from a pool of the wrong family and
 * submitted anyway, which is invalid. So the queues are recorded as they are handed out. */
#define MAX_QUEUES 16
static struct queue_data {
	VkQueue queue;
	VkDevice device;
	uint32_t family;
	int capture_ok;      /* the family can hold a copy, and the queue is not protected */
} queues[MAX_QUEUES];

static struct queue_data *find_queue(VkQueue queue)
{
	for (int i = 0; i < MAX_QUEUES; i++)
		if (queues[i].queue == queue) return &queues[i];
	return NULL;
}

/* Whether a copy can be recorded for this family at all. A present queue is not
 * required to support graphics, compute or transfer — some drivers expose a
 * present-only family — and recording `vkCmdCopyImageToBuffer` on one is invalid.
 * Ported from the parallel ProjectsCodex tree, which had this guard and we did not. */
static int family_can_capture(struct device_data *data, uint32_t family)
{
	if (!data || !next_instance_proc || !layer_instance) return 0;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties properties =
		(PFN_vkGetPhysicalDeviceQueueFamilyProperties)
		next_instance_proc(layer_instance, "vkGetPhysicalDeviceQueueFamilyProperties");
	if (!properties) return 0;
	uint32_t count = 0;
	properties(data->physical, &count, NULL);
	if (!count || family >= count) return 0;
	VkQueueFamilyProperties *families = calloc(count, sizeof *families);
	if (!families) return 0;
	properties(data->physical, &count, families);
	int ok = (families[family].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT
						 | VK_QUEUE_TRANSFER_BIT)) != 0;
	free(families);
	return ok;
}

static void announce_queues(VkDevice device, VkQueue presenting, uint32_t present_family)
{
	uint32_t families[MAX_QUEUES];
	int count = 0, others = 0;
	nr_mutex_lock(&lock);
	for (int i = 0; i < MAX_QUEUES; i++)
		if (queues[i].queue && queues[i].device == device) {
			families[count++] = queues[i].family;
			if (queues[i].queue != presenting && queues[i].family != present_family)
				others++;
		}
	nr_mutex_unlock(&lock);
	char list[128] = { 0 };
	size_t used = 0;
	for (int i = 0; i < count && used + 8 < sizeof list; i++)
		used += (size_t)snprintf(list + used, sizeof list - used, "%s%u",
					 used ? "," : "", families[i]);
	fprintf(stderr, "[nr_layer] queues: %d handed out, families {%s}; presenting on "
		"family %u. %s\n", count, list, present_family,
		others ? "Multiple families: capture waits on the present's semaphores."
		       : "Capture waits on the present's semaphores; copies finish on private fences.");
}

static void remember_queue(VkDevice device, uint32_t family, VkQueue queue, int capture_ok)
{
	if (!queue) return;
	nr_mutex_lock(&lock);
	if (!find_queue(queue))
		for (int i = 0; i < MAX_QUEUES; i++)
			if (!queues[i].queue) {
				queues[i] = (struct queue_data){ queue, device, family, capture_ok };
				break;
			}
	nr_mutex_unlock(&lock);
}

VKAPI_ATTR void VKAPI_CALL nr_GetDeviceQueue(VkDevice device, uint32_t family,
					     uint32_t index, VkQueue *queue)
{
	struct device_data *data = find_device(device);
	if (!data || !data->get_device_queue) return;
	data->get_device_queue(device, family, index, queue);
	remember_queue(device, family, *queue, family_can_capture(data, family));
}

VKAPI_ATTR void VKAPI_CALL nr_GetDeviceQueue2(VkDevice device,
					      const VkDeviceQueueInfo2 *info, VkQueue *queue)
{
	struct device_data *data = find_device(device);
	if (!data || !data->get_device_queue2) return;
	data->get_device_queue2(device, info, queue);
	/* Protected memory cannot be read back into a host-visible buffer, so a protected
	 * queue is never a capture source however capable its family is. */
	remember_queue(device, info->queueFamilyIndex, *queue,
		       (info->flags & VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT)
		       ? 0 : family_can_capture(data, info->queueFamilyIndex));
}

/* Four bytes a pixel is assumed everywhere downstream: the staging buffer is sized
 * `width * height * 4`, `vkCmdCopyImageToBuffer` derives its extent from the image, and
 * the daemon decodes exactly these five formats. An HDR swapchain — R16G16B16A16_SFLOAT
 * is eight — would have the driver copy twice what the buffer holds. So a format that
 * is not on this list is not tracked at all, and the layer stays out of the way. */
static int format_is_four_bytes(VkFormat format)
{
	switch (format) {
	case VK_FORMAT_R8G8B8A8_UNORM:
	case VK_FORMAT_R8G8B8A8_SRGB:
	case VK_FORMAT_B8G8R8A8_UNORM:
	case VK_FORMAT_B8G8R8A8_SRGB:
	case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		return 1;
	default:
		return 0;
	}
}

static struct swapchain_data *find_swapchain(VkSwapchainKHR swapchain)
{
	for (int i = 0; i < MAX_SWAPCHAINS; i++)
		if (swapchains[i].swapchain == swapchain) return &swapchains[i];
	return NULL;
}

/* The loader hands each layer a chain of get-proc-address functions; these walk it. */
static VkLayerInstanceCreateInfo *instance_chain(const VkInstanceCreateInfo *info)
{
	VkLayerInstanceCreateInfo *item = (VkLayerInstanceCreateInfo *)info->pNext;
	while (item && !(item->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO
			 && item->function == VK_LAYER_LINK_INFO))
		item = (VkLayerInstanceCreateInfo *)item->pNext;
	return item;
}

static VkLayerDeviceCreateInfo *device_chain(const VkDeviceCreateInfo *info)
{
	VkLayerDeviceCreateInfo *item = (VkLayerDeviceCreateInfo *)info->pNext;
	while (item && !(item->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO
			 && item->function == VK_LAYER_LINK_INFO))
		item = (VkLayerDeviceCreateInfo *)item->pNext;
	return item;
}

/* The directory the knob files and the daemon's log live in, when the environment does
 * not name one. nr_paths.py picks /tmp on Linux and the user's temp folder on Windows;
 * this matches that, so a default on one side is a default on the other.
 *
 * GetTempPathA returns the length of what it wrote, including the trailing backslash, and
 * 0 on failure. The trailing separator is trimmed, but only after checking that n indexes
 * something we actually wrote: on failure n is 0, and when the buffer is too small n is
 * the length it *would* have needed - either one makes out[n - 1] read and write outside
 * the array. */
static void nr_default_dir(char *out, size_t size)
{
#ifdef _WIN32
	DWORD n = GetTempPathA((DWORD)size, out);
	if (n == 0 || n >= (DWORD)size) {
		snprintf(out, size, ".");
		return;
	}
	if (n >= 1 && out[n - 1] == '\\') out[n - 1] = '\0';
#else
	snprintf(out, size, "/tmp");
#endif
}

/* The interpreter to start the daemon with, when NR_PYTHON does not name one.
 *
 * It cannot be "python3" on both: on Windows that name belongs to the Microsoft Store's
 * App Execution Alias, a zero-byte stub that opens the Store and exits 9009 with no
 * output. `CreateProcessA` starts it happily, so the spawn looks like it worked and the
 * daemon simply never appears - measured here, and the reason the log stayed empty.
 *
 * The launcher Windows installs for python.org is `py`, which is not a Store alias and
 * is on PATH whenever any Python 3 is; `python` is the installer's other name. The
 * launcher is preferred because it cannot be the alias, and a path that exists on disk
 * is preferred over a bare name so a stray `py` earlier on PATH cannot win.
 *
 * On Linux "python3" is right and stays. */
static const char *nr_default_python(void)
{
#ifdef _WIN32
	static char found[1024];
	const char *cands[] = { "py", "python" };
	for (size_t i = 0; i < sizeof cands / sizeof *cands; i++) {
		char buf[1024];
		DWORD n = SearchPathA(NULL, cands[i], ".exe", (DWORD)sizeof buf, buf, NULL);
		if (n == 0 || n >= sizeof buf) continue;
		/* The Store alias is a reparse point with no bytes behind it; its real
		 * file is zero length. Anything with content is an interpreter. */
		WIN32_FILE_ATTRIBUTE_DATA fad;
		if (GetFileAttributesExA(buf, GetFileExInfoStandard, &fad)
		    && fad.nFileSizeHigh == 0 && fad.nFileSizeLow == 0)
			continue;
		snprintf(found, sizeof found, "%s", buf);
		return found;
	}
	return "py";                    /* let CreateProcess say so if there is none */
#else
	return "python3";
#endif
}

static void ensure_daemon(void);

static void ensure_daemon(void)
{
	/* Opt-in, and silent when off: most processes that load the layer (vulkaninfo,
	 * a compositor) must not start a daemon and must not print anything either. */
	if (!getenv("NR_LAYER_SPAWN")) return;
	/* Nothing to talk to without an endpoint: spawning a daemon nobody uses costs
	 * ~3 GiB. */
	if (!socket_path) return;
	/* Do not re-spawn from inside a daemon we started: it inherits VK_INSTANCE_LAYERS
	 * and would load this layer again, recursing until the OOM killer arrives. The
	 * child is marked NR_LAYER_SPAWNED, so its own layer sees this and steps back. */
	if (getenv("NR_LAYER_SPAWNED")) return;

	/* Already up? Then leave it. A one-shot probe, not a blocking connect: waiting
	 * would stall every game load. */
	{
		int alive = 0;
#ifdef _WIN32
		HANDLE h = CreateFileA(socket_path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
				       OPEN_EXISTING, 0, NULL);
		if (h != INVALID_HANDLE_VALUE) { CloseHandle(h); alive = 1; }
		else if (GetLastError() == ERROR_PIPE_BUSY) alive = 1;   /* exists, busy */
#else
		int fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (fd >= 0) {
			struct sockaddr_un a;
			memset(&a, 0, sizeof a);
			a.sun_family = AF_UNIX;
			snprintf(a.sun_path, sizeof a.sun_path, "%s", socket_path);
			if (connect(fd, (struct sockaddr *)&a, sizeof a) == 0) alive = 1;
			close(fd);
		}
#endif
		if (alive) return;
	}

	const char *py = getenv("NR_PYTHON");
	if (!py || !*py) py = nr_default_python();
	/* The daemon script lives next to *this library*. /proc/self/exe is the game
	 * (a Wine binary under Proton), so ask the loader for our own path instead. */
	char self[1024];
	self[0] = '\0';
#ifdef _WIN32
	{
		HMODULE hmod;
		if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
				       (LPCWSTR)(const void *)&ensure_daemon, &hmod)) {
			WCHAR w[1024];
			if (GetModuleFileNameW(hmod, w, 1024))
				WideCharToMultiByte(CP_UTF8, 0, w, -1, self, sizeof self, NULL, NULL);
		}
	}
#else
	{
		Dl_info info;
		if (dladdr((const void *)&ensure_daemon, &info) && info.dli_fname)
			snprintf(self, sizeof self, "%s", info.dli_fname);
	}
#endif
	if (!self[0]) {
		fprintf(stderr, "[nr_layer] could not locate this layer's path; set NR_DAEMON\n");
		return;
	}

	char daemon_buf[1024];
	const char *daemon = getenv("NR_DAEMON");
	if (!daemon || !*daemon) {
		/* Where the daemon sits relative to the library, in the layouts this tree and
		 * its Makefile produce:
		 *   <dir>/nr_daemon.py              flat deployment
		 *   <dir>/src/layer/nr_daemon.py    a checkout, library beside the sources
		 *   ../src/layer/nr_daemon.py       `make` builds the library into work/ and
		 *                                   prepare_layer.py points the manifest there,
		 *                                   so work/ is the directory beside it
		 * The first that exists wins. */
		char *slash = self;
		for (char *p = self; *p; p++) if (*p == '/' || *p == '\\') slash = p;
		*slash = '\0';
		const char *cands[] = { "/nr_daemon.py", "/src/layer/nr_daemon.py",
					"/../src/layer/nr_daemon.py" };
		daemon = NULL;
		for (size_t i = 0; i < sizeof cands / sizeof *cands; i++) {
			snprintf(daemon_buf, sizeof daemon_buf, "%s%s", self, cands[i]);
			if (access(daemon_buf, R_OK) == 0) { daemon = daemon_buf; break; }
		}
		if (daemon) {
			/* The root is derived by stripping two levels off this path, so it must
			 * not still contain a ".." when that happens. */
			char resolved[1024];
#ifdef _WIN32
			void *ok = _fullpath(resolved, daemon, sizeof resolved);
#else
			void *ok = realpath(daemon, resolved);
#endif
			if (ok) {
				snprintf(daemon_buf, sizeof daemon_buf, "%s", resolved);
				daemon = daemon_buf;
			}
		}
		if (!daemon) {
			fprintf(stderr, "[nr_layer] could not find nr_daemon.py next to the "
				"layer; set NR_DAEMON to its path\n");
			return;
		}
	}

	/* The daemon's module root is two levels above src/layer/nr_daemon.py; for a flat
	 * deployment it is the directory holding the script. NR_ROOT overrides both. */
	char root_buf[1024];
	const char *root = getenv("NR_ROOT");
	if (!root || !*root) {
		snprintf(root_buf, sizeof root_buf, "%s", daemon);
		char *slash = root_buf;
		for (char *p = root_buf; *p; p++) if (*p == '/' || *p == '\\') slash = p;
		*slash = '\0';                                  /* .../src/layer */
		if (strstr(root_buf, "/src/layer") || strstr(root_buf, "\\src\\layer")) {
			slash = root_buf;
			for (char *p = root_buf; *p; p++) if (*p == '/' || *p == '\\') slash = p;
			*slash = '\0';                              /* .../src */
			slash = root_buf;
			for (char *p = root_buf; *p; p++) if (*p == '/' || *p == '\\') slash = p;
			*slash = '\0';                              /* ... (the root) */
		}
		root = root_buf;
	}

	/* Same defaults as src/layer/nr_paths.py, or the knobs the tools write never
	 * reach a daemon started from here. */
	const char *settings = getenv("NR_SETTINGS");
	char settings_dir[1024];
	nr_default_dir(settings_dir, sizeof settings_dir);
	/* room for the whole directory and the name: a 1024-byte default cannot be cut */
	char settings_def[1024 + 32];
	snprintf(settings_def, sizeof settings_def, "%s/nr_settings.json", settings_dir);
	if (!settings || !*settings) settings = settings_def;

	char cmd[4096];
	int written = snprintf(cmd, sizeof cmd,
			       "\"%s\" \"%s\" --socket \"%s\" --settings \"%s\" --root \"%s\"",
			       py, daemon, socket_path, settings, root);
	if (written < 0 || (size_t)written >= sizeof cmd) {
		/* a cut command line would start some other program than the daemon */
		fprintf(stderr, "[nr_layer] the daemon's command line is longer than %zu "
			"bytes; start the daemon by hand\n", sizeof cmd);
		return;
	}

#ifdef _WIN32
	{
		STARTUPINFOA si; PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof si); si.cb = sizeof si;
		memset(&pi, 0, sizeof pi);
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdInput = INVALID_HANDLE_VALUE;
		char log[1024 + 32];
		char log_dir[1024];
		nr_default_dir(log_dir, sizeof log_dir);
		const char *log_env = getenv("NR_LAYER_LOG");
		if (log_env && *log_env)
			snprintf(log, sizeof log, "%s", log_env);
		else
			snprintf(log, sizeof log, "%s/nr_daemon.log", log_dir);
		/* Both handles inheritable, or the child's standard streams are invalid: the
		 * spawn passes STARTF_USESTDHANDLES with bInheritHandles=TRUE, and a handle
		 * created without this SECURITY_ATTRIBUTES is not inherited by anyone. Both
		 * failures are silent - the daemon starts and runs, and its log stays empty.
		 *
		 * stdin is NUL rather than INVALID_HANDLE_VALUE. An invalid handle is inherited
		 * as invalid, and anything the daemon starts that asks for its stdin - Python's
		 * own subprocess with capture_output is the one that bit us - fails with
		 * "The handle is invalid" and takes the daemon down with it. NUL is a real,
		 * readable handle that says there is nothing to read. */
		SECURITY_ATTRIBUTES sa = { .nLength = sizeof sa, .lpSecurityDescriptor = NULL,
					   .bInheritHandle = TRUE };
		si.hStdInput = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
					   &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		si.hStdOutput = CreateFileA(log, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
					    &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		si.hStdError = si.hStdOutput;
		/* The marker goes in the child's environment block, not ours: setting it in
		 * the game would stop a second instance in the same process from ever
		 * spawning, and setenv is not safe while other threads read the environment.
		 * VK_INSTANCE_LAYERS goes too: the daemon builds its own Vulkan instance, and
		 * with the layer still named there its status lines land in the daemon's log
		 * where a reader takes them for the layer talking about the game. */
		/* The child's environment, built from ours with two entries dropped and one
		 * added. Sized from what is actually there rather than a fixed 8 KB: an
		 * environment larger than the guess would have been truncated silently, and
		 * the daemon would start with a partial environment and no sign of why.
		 *
		 * The marker carries the game's process id, and the daemon ends when that
		 * process does (nr_daemon.end_with_game). Steam counts every process a game
		 * starts as the game: a daemon that outlived it kept the game running for Steam,
		 * so Restore Steam refused, and kept the model in memory until it was killed. */
		char marker[48];
		snprintf(marker, sizeof marker, "NR_LAYER_SPAWNED=%lu",
			 (unsigned long)GetCurrentProcessId());
		size_t marker_size = strlen(marker) + 1;
		char *env = GetEnvironmentStringsA();
		size_t need = marker_size + 1;
		if (env)
			for (char *e = env; *e; e += strlen(e) + 1)
				need += strlen(e) + 1;
		char *env_block = malloc(need);
		if (!env_block) {
			if (env) FreeEnvironmentStringsA(env);
			fprintf(stderr, "[nr_layer] out of memory building the daemon's "
				"environment; frame stays as the game drew it\n");
			return;
		}
		size_t used = 0;
		if (env) {
			for (char *e = env; *e; e += strlen(e) + 1) {
				if (strncmp(e, "NR_LAYER_SPAWNED=", 17) == 0) continue;
				if (strncmp(e, "VK_INSTANCE_LAYERS=", 19) == 0) continue;
				size_t len = strlen(e) + 1;
				memcpy(env_block + used, e, len);
				used += len;
			}
			FreeEnvironmentStringsA(env);
		}
		memcpy(env_block + used, marker, marker_size);
		used += marker_size;
		env_block[used] = '\0';

		/* A console without a window, not DETACHED_PROCESS: a virtual environment's
		 * python.exe is a launcher that starts the real interpreter as its own child, and a
		 * console child of a process with no console gets a new, visible one - over the game,
		 * which loses the focus and, full screen, stops presenting. Children of a
		 * CREATE_NO_WINDOW process share its hidden console instead. */
		BOOL ok = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW,
					 env_block, NULL, &si, &pi);
		free(env_block);
		if (si.hStdInput != INVALID_HANDLE_VALUE &&
		    si.hStdInput != NULL) CloseHandle(si.hStdInput);
		if (si.hStdOutput != INVALID_HANDLE_VALUE) CloseHandle(si.hStdOutput);
		if (!ok) {
			fprintf(stderr, "[nr_layer] daemon spawn failed (%lu); frame stays "
				"as the game drew it\n", GetLastError());
			return;
		}
		CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
	}
#else
	{
		/* Copy the environment with the marker added, so the game's own environment is
		 * untouched (setenv() in a multithreaded process is not safe).
		 * VK_INSTANCE_LAYERS is dropped for the same reason as on Windows: the daemon
		 * builds its own Vulkan instance and would otherwise load this layer too. */
#ifdef __APPLE__
		char **environ = *_NSGetEnviron();
#else
		extern char **environ;
#endif
		char marker[] = "NR_LAYER_SPAWNED=1";
		size_t count = 0;
		while (environ[count]) count++;
		char **child_env = malloc((count + 2) * sizeof *child_env);
		if (!child_env) return;
		size_t w = 0;
		for (size_t i = 0; i < count; i++)
			if (strncmp(environ[i], "NR_LAYER_SPAWNED=", 17) != 0
			    && strncmp(environ[i], "VK_INSTANCE_LAYERS=", 19) != 0)
				child_env[w++] = environ[i];
		child_env[w++] = marker;
		child_env[w] = NULL;

		posix_spawn_file_actions_t fa;
		posix_spawn_file_actions_init(&fa);
		posix_spawn_file_actions_addopen(&fa, 0, "/dev/null", O_RDONLY, 0);
		const char *log = getenv("NR_LAYER_LOG");
		char log_dir2[1024];
		nr_default_dir(log_dir2, sizeof log_dir2);
		char log_def[1024 + 32];
		snprintf(log_def, sizeof log_def, "%s/nr_daemon.log", log_dir2);
		if (!log || !*log) log = log_def;
		posix_spawn_file_actions_addopen(&fa, 1, log, O_WRONLY | O_CREAT | O_APPEND, 0644);
		posix_spawn_file_actions_adddup2(&fa, 1, 2);

		char *argv[10];
		int i = 0;
		argv[i++] = (char *)py;
		argv[i++] = (char *)daemon;
		argv[i++] = "--socket";   argv[i++] = (char *)socket_path;
		argv[i++] = "--settings"; argv[i++] = (char *)settings;
		argv[i++] = "--root";     argv[i++] = (char *)root;
		argv[i] = NULL;

		pid_t pid;
		if (posix_spawnp(&pid, py, &fa, NULL, argv, child_env) != 0)
			fprintf(stderr, "[nr_layer] daemon spawn failed; frame stays as the "
				"game drew it\n");
		free(child_env);
		posix_spawn_file_actions_destroy(&fa);
	}
#endif
}

VKAPI_ATTR VkResult VKAPI_CALL nr_CreateInstance(const VkInstanceCreateInfo *info,
						 const VkAllocationCallbacks *allocator,
						 VkInstance *instance)
{
	VkLayerInstanceCreateInfo *link = instance_chain(info);
	if (!link) return VK_ERROR_INITIALIZATION_FAILED;
	next_instance_proc = link->u.pLayerInfo->pfnNextGetInstanceProcAddr;
	link->u.pLayerInfo = link->u.pLayerInfo->pNext;
	PFN_vkCreateInstance create =
		(PFN_vkCreateInstance)next_instance_proc(NULL, "vkCreateInstance");
	VkResult r = create(info, allocator, instance);
	if (r == VK_SUCCESS) {
		/* Physical-device entry points cannot be resolved against a NULL
		 * instance — only the handful of global ones can — so the instance has
		 * to be kept. */
		layer_instance = *instance;
		capture_path = getenv("NR_LAYER_CAPTURE");
		socket_path = getenv("NR_LAYER_SOCKET");
		ensure_daemon();
		trigger_path = getenv("NR_LAYER_TRIGGER");
		const char *mask = getenv("NR_LAYER_UI_MASK");
		ui_mask = mask && strcmp(mask, "0") != 0;
		const char *every = getenv("NR_LAYER_EVERY");
		capture_every = every ? strtol(every, NULL, 10) : 0;
		const char *sync = getenv("NR_LAYER_SYNC");
		(void)sync;
		const char *live = getenv("NR_LAYER_LIVE");
		live_every = live ? strtol(live, NULL, 10) : 0;
		if (live_every < 0) live_every = 0;
		const char *pipelined = getenv("NR_LAYER_ASYNC");
		async_live = live_every > 0 && pipelined && strcmp(pipelined, "0") != 0;
		if (async_live)
			fprintf(stderr, "[nr_layer] async: each processed present shows the answer "
				"for the one before it, and the daemon works while the game draws\n");
		if (live_every > 0)
			fprintf(stderr, "[nr_layer] live: every %ld%s present goes through the "
				"network, the frames between hold the last result\n",
				live_every, live_every == 1 ? "st" : "th");
		if (live_every > 0 && getenv("NR_LAYER_TRIGGER"))
			fprintf(stderr, "[nr_layer] live runs while the trigger exists; "
				"remove it to hand the game back\n");
		fprintf(stderr, "[nr_layer] sync: present waits + private fences (readback and writeback)\n");
		fprintf(stderr, "[nr_layer] active; socket=%s trigger=%s capture=%s every=%ld\n",
			socket_path ? socket_path : "(none)",
			trigger_path ? trigger_path : "(none)",
			capture_path ? capture_path : "(off)", capture_every);
		if (ui_mask)
			fprintf(stderr, "[nr_layer] ui mask on: the first present after the "
				"trigger is kept to find what held still\n");
	}
	return r;
}

VKAPI_ATTR VkResult VKAPI_CALL nr_CreateDevice(VkPhysicalDevice physical,
					       const VkDeviceCreateInfo *info,
					       const VkAllocationCallbacks *allocator,
					       VkDevice *device)
{
	VkLayerDeviceCreateInfo *link = device_chain(info);
	if (!link) return VK_ERROR_INITIALIZATION_FAILED;
	PFN_vkGetInstanceProcAddr next_instance = link->u.pLayerInfo->pfnNextGetInstanceProcAddr;
	PFN_vkGetDeviceProcAddr next_device = link->u.pLayerInfo->pfnNextGetDeviceProcAddr;
	link->u.pLayerInfo = link->u.pLayerInfo->pNext;

	PFN_vkCreateDevice create = (PFN_vkCreateDevice)next_instance(NULL, "vkCreateDevice");
	VkResult r = create(physical, info, allocator, device);
	if (r != VK_SUCCESS) return r;

	nr_mutex_lock(&lock);
	struct device_data *data = NULL;
	for (int i = 0; i < 8; i++) if (!devices[i].device) { data = &devices[i]; break; }
	if (data) {
		memset(data, 0, sizeof *data);
		nr_mutex_init(&data->present_lock, NULL);
		data->device = *device;
		data->physical = physical;
		data->get_device_proc = next_device;
		data->present = (PFN_vkQueuePresentKHR)next_device(*device, "vkQueuePresentKHR");
		data->get_device_queue =
			(PFN_vkGetDeviceQueue)next_device(*device, "vkGetDeviceQueue");
		data->get_device_queue2 =
			(PFN_vkGetDeviceQueue2)next_device(*device, "vkGetDeviceQueue2");
		data->destroy_swapchain =
			(PFN_vkDestroySwapchainKHR)next_device(*device, "vkDestroySwapchainKHR");
		data->create_swapchain =
			(PFN_vkCreateSwapchainKHR)next_device(*device, "vkCreateSwapchainKHR");
		data->get_swapchain_images =
			(PFN_vkGetSwapchainImagesKHR)next_device(*device, "vkGetSwapchainImagesKHR");
		data->destroy_device = (PFN_vkDestroyDevice)next_device(*device, "vkDestroyDevice");
		data->queue_family = info->queueCreateInfoCount
			? info->pQueueCreateInfos[0].queueFamilyIndex : 0;
	}
	nr_mutex_unlock(&lock);
	return r;
}

/* The swapchain images are the frame we want, so they have to be copyable. The loader
 * lets a layer edit the create info on the way down; without TRANSFER_SRC the copy
 * below is invalid, and this is the one place it can be added. */
VKAPI_ATTR VkResult VKAPI_CALL nr_CreateSwapchainKHR(VkDevice device,
						     const VkSwapchainCreateInfoKHR *info,
						     const VkAllocationCallbacks *allocator,
						     VkSwapchainKHR *swapchain)
{
	struct device_data *data = find_device(device);
	if (!data) return VK_ERROR_INITIALIZATION_FAILED;
	const VkImageUsageFlags copies = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
				       | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VkSwapchainCreateInfoKHR patched = *info;
	patched.imageUsage |= copies;
	int copyable = 1;
	VkResult r = data->create_swapchain(device, &patched, allocator, swapchain);
	if (r != VK_SUCCESS) {
		/* The surface may refuse transfer usage. Falling back keeps the game alive,
		 * but the images then lack TRANSFER_SRC and copying from them is invalid
		 * usage — so the swapchain is created and deliberately not tracked. Before
		 * this the fallback silently left us issuing an illegal copy every frame;
		 * the parallel ProjectsCodex tree checks the flags and we did not. */
		copyable = (info->imageUsage & copies) == copies;
		r = data->create_swapchain(device, info, allocator, swapchain);
	}
	if (r != VK_SUCCESS) return r;
	if (!copyable) {
		fprintf(stderr, "[nr_layer] swapchain refused transfer usage; capture off\n");
		return r;
	}
	/* One layer, and not protected: the copy reads a single 2-D image, and protected
	 * memory cannot reach a host-visible buffer at all. */
	if (info->imageArrayLayers != 1
	    || (info->flags & VK_SWAPCHAIN_CREATE_PROTECTED_BIT_KHR)) {
		fprintf(stderr, "[nr_layer] swapchain is multi-layer or protected; capture off\n");
		return r;
	}

	if (!format_is_four_bytes(info->imageFormat)) {
		fprintf(stderr, "[nr_layer] swapchain format %d is not four bytes a pixel; "
			"leaving it alone\n", info->imageFormat);
		return r;
	}
	nr_mutex_lock(&lock);
	struct swapchain_data *entry = NULL;
	for (int i = 0; i < MAX_SWAPCHAINS; i++)
		if (!swapchains[i].swapchain) { entry = &swapchains[i]; break; }
	if (!entry)
		fprintf(stderr, "[nr_layer] all %d swapchain slots are in use; this one is "
			"not tracked\n", MAX_SWAPCHAINS);
	if (entry) {
		memset(entry, 0, sizeof *entry);
		entry->swapchain = *swapchain;
		entry->device = device;
		entry->format = info->imageFormat;
		entry->extent = info->imageExtent;
		entry->image_count = MAX_IMAGES;
		VkResult got = data->get_swapchain_images(device, *swapchain,
							  &entry->image_count, entry->images);
		/* VK_INCOMPLETE means there are more images than the array holds, and
		 * `pImageIndices` may then name one past the end. Refusing is the only safe
		 * answer; the alternative is an out-of-bounds read every present. */
		if (got != VK_SUCCESS || !entry->image_count) {
			memset(entry, 0, sizeof *entry);
			nr_mutex_unlock(&lock);
			fprintf(stderr, "[nr_layer] swapchain has more than %d images or none; "
				"capture off\n", MAX_IMAGES);
			return r;
		}
		fprintf(stderr, "[nr_layer] swapchain %ux%u format %d, %u images\n",
			entry->extent.width, entry->extent.height, entry->format,
			entry->image_count);
	}
	nr_mutex_unlock(&lock);
	return r;
}

/* Everything this layer allocated per device, given back. The pointer to
 * `vkDestroyDevice` was being stored and never used: a staging buffer, its device memory
 * and its host mapping leaked on every device teardown, which a game that recreates its
 * device — a resolution change under some wrappers — does more than once. Ported from the
 * parallel ProjectsCodex tree. */
static void release_device(struct device_data *data)
{
	drop_inflight(data);
	if (data->mapped) {
		PFN_vkUnmapMemory unmap =
			(PFN_vkUnmapMemory)data->get_device_proc(data->device, "vkUnmapMemory");
		if (unmap) unmap(data->device, data->staging_memory);
	}
	if (data->staging) {
		PFN_vkDestroyBuffer destroy =
			(PFN_vkDestroyBuffer)data->get_device_proc(data->device, "vkDestroyBuffer");
		if (destroy) destroy(data->device, data->staging, NULL);
	}
	if (data->staging_memory) {
		PFN_vkFreeMemory release =
			(PFN_vkFreeMemory)data->get_device_proc(data->device, "vkFreeMemory");
		if (release) release(data->device, data->staging_memory, NULL);
	}
	if (data->pool) {
		PFN_vkDestroyCommandPool destroy = (PFN_vkDestroyCommandPool)
			data->get_device_proc(data->device, "vkDestroyCommandPool");
		if (destroy) destroy(data->device, data->pool, NULL);
	}
	if (data->stalled_fence) {
        PFN_vkDestroyFence destroy = (PFN_vkDestroyFence)data->get_device_proc(data->device, "vkDestroyFence");
        destroy(data->device, data->stalled_fence, NULL);
    }
	nr_mutex_destroy(&data->present_lock);
	free(data->result);
	free(data->earlier);
	free(data->outgoing);
}

VKAPI_ATTR void VKAPI_CALL nr_DestroyDevice(VkDevice device,
					    const VkAllocationCallbacks *allocator)
{
	struct device_data *data = find_device(device);
	PFN_vkDestroyDevice next = data ? data->destroy_device : NULL;
	if (data) {
		nr_mutex_lock(&lock);
		for (int i = 0; i < MAX_SWAPCHAINS; i++)
			if (swapchains[i].device == device)
				memset(&swapchains[i], 0, sizeof swapchains[i]);
		for (int i = 0; i < MAX_QUEUES; i++)
			if (queues[i].device == device)
				memset(&queues[i], 0, sizeof queues[i]);
		nr_mutex_unlock(&lock);
		release_device(data);
		memset(data, 0, sizeof *data);
	}
	if (next) next(device, allocator);
}

/* Without this the eight slots are consumed one per window resize, exclusive-fullscreen
 * toggle or HDR renegotiation, after which nothing is tracked and the photo mode goes
 * quiet. Worse, a driver may reuse a handle, and then `find_swapchain` would match a
 * stale entry whose VkImages are long destroyed. */
VKAPI_ATTR void VKAPI_CALL nr_DestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain,
						  const VkAllocationCallbacks *allocator)
{
	struct device_data *data = find_device(device);
    if (data) nr_mutex_lock(&data->present_lock);
    if (data && data->result_chain == swapchain) {
        data->result_chain = VK_NULL_HANDLE;
        data->holding = data->have_earlier = 0;
    }
	nr_mutex_lock(&lock);
	for (int i = 0; i < MAX_SWAPCHAINS; i++)
		if (swapchains[i].swapchain == swapchain)
			memset(&swapchains[i], 0, sizeof swapchains[i]);
	nr_mutex_unlock(&lock);
	if (data && data->destroy_swapchain)
		data->destroy_swapchain(device, swapchain, allocator);
    if (data) nr_mutex_unlock(&data->present_lock);
}

static uint32_t memory_type(struct device_data *data, uint32_t bits,
			    VkMemoryPropertyFlags want)
{
	VkPhysicalDeviceMemoryProperties properties;
	PFN_vkGetPhysicalDeviceMemoryProperties get =
		(PFN_vkGetPhysicalDeviceMemoryProperties)next_instance_proc(
			layer_instance, "vkGetPhysicalDeviceMemoryProperties");
	if (!get) return UINT32_MAX;
	get(data->physical, &properties);
	for (uint32_t i = 0; i < properties.memoryTypeCount; i++)
		if ((bits & (1u << i))
		    && (properties.memoryTypes[i].propertyFlags & want) == want)
			return i;
	return UINT32_MAX;
}

/* Grow host-visible storage only after the previous private copy fence completed. */
static void release_staging(struct device_data *data)
{
    if (data->mapped) ((PFN_vkUnmapMemory)data->get_device_proc(data->device, "vkUnmapMemory"))(data->device, data->staging_memory);
    if (data->staging) ((PFN_vkDestroyBuffer)data->get_device_proc(data->device, "vkDestroyBuffer"))(data->device, data->staging, NULL);
    if (data->staging_memory) ((PFN_vkFreeMemory)data->get_device_proc(data->device, "vkFreeMemory"))(data->device, data->staging_memory, NULL);
    data->mapped = NULL;
    data->staging = VK_NULL_HANDLE;
    data->staging_memory = VK_NULL_HANDLE;
    data->staging_size = 0;
}

static int ensure_resources(struct device_data *data, VkDeviceSize needed)
{
	if (!data->pool) {
		VkCommandPoolCreateInfo info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = data->queue_family };
		PFN_vkCreateCommandPool create = (PFN_vkCreateCommandPool)
			data->get_device_proc(data->device, "vkCreateCommandPool");
		if (create(data->device, &info, NULL, &data->pool) != VK_SUCCESS) return -1;
	}
	if (data->staging_size >= needed && data->result) {
        data->result_size = needed;
        return 0;
    }
    release_staging(data);
    data->holding = data->have_earlier = 0;

	PFN_vkCreateBuffer create_buffer = (PFN_vkCreateBuffer)
		data->get_device_proc(data->device, "vkCreateBuffer");
	PFN_vkGetBufferMemoryRequirements requirements = (PFN_vkGetBufferMemoryRequirements)
		data->get_device_proc(data->device, "vkGetBufferMemoryRequirements");
	PFN_vkAllocateMemory allocate = (PFN_vkAllocateMemory)
		data->get_device_proc(data->device, "vkAllocateMemory");
	PFN_vkBindBufferMemory bind = (PFN_vkBindBufferMemory)
		data->get_device_proc(data->device, "vkBindBufferMemory");
	PFN_vkMapMemory map = (PFN_vkMapMemory)
		data->get_device_proc(data->device, "vkMapMemory");
	VkBufferCreateInfo info = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				    .size = needed,
				    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT
					     | VK_BUFFER_USAGE_TRANSFER_SRC_BIT };
	if (create_buffer(data->device, &info, NULL, &data->staging) != VK_SUCCESS) return -1;
	VkMemoryRequirements mr;
	requirements(data->device, data->staging, &mr);
	uint32_t type = memory_type(data, mr.memoryTypeBits,
				    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
				    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				    | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	if (type == UINT32_MAX)
		type = memory_type(data, mr.memoryTypeBits,
				   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
				   | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (type == UINT32_MAX) { release_staging(data); return -1; }
	VkMemoryAllocateInfo allocation = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					    .allocationSize = mr.size, .memoryTypeIndex = type };
	if (allocate(data->device, &allocation, NULL, &data->staging_memory) != VK_SUCCESS) {
        release_staging(data); return -1;
    }
    if (bind(data->device, data->staging, data->staging_memory, 0) != VK_SUCCESS ||
        map(data->device, data->staging_memory, 0, VK_WHOLE_SIZE, 0, &data->mapped) != VK_SUCCESS) {
        release_staging(data); return -1;
    }
	data->staging_size = needed;
	free(data->result);
	data->result = malloc((size_t)needed);
	data->result_size = needed;
	return data->result ? 0 : -1;
}

/* All copy submissions wait only on the current present's semaphores and on their
 * own fence. No foreign queue is touched and no semaphore is recycled behind WSI.
 * The synchronous daemon already requires host readback; completing writeback too
 * keeps the lifetime contract explicit until an asynchronous path is separately proven. */
static int transfer(struct device_data *data, struct swapchain_data *chain, VkQueue queue,
                    uint32_t index, int to_image)
{
    if (data->device_error || data->transfer_error) return -1;
#define FN(type, name) type name = (type)data->get_device_proc(data->device, "vk" #name)
    FN(PFN_vkAllocateCommandBuffers, AllocateCommandBuffers);
    FN(PFN_vkBeginCommandBuffer, BeginCommandBuffer);
    FN(PFN_vkEndCommandBuffer, EndCommandBuffer);
    FN(PFN_vkCmdPipelineBarrier, CmdPipelineBarrier);
    FN(PFN_vkCmdCopyImageToBuffer, CmdCopyImageToBuffer);
    FN(PFN_vkCmdCopyBufferToImage, CmdCopyBufferToImage);
    FN(PFN_vkQueueSubmit, QueueSubmit);
    FN(PFN_vkWaitForFences, WaitForFences);
    FN(PFN_vkCreateFence, CreateFence);
    FN(PFN_vkDestroyFence, DestroyFence);
    FN(PFN_vkFreeCommandBuffers, FreeCommandBuffers);
#undef FN
    VkCommandBuffer commands = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkPipelineStageFlags *stages = NULL;
    VkResult r;
    VkCommandBufferAllocateInfo alloc = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = data->pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1 };
    r = AllocateCommandBuffers(data->device, &alloc, &commands);
    if (r != VK_SUCCESS) goto failed;
    VkFenceCreateInfo fi = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    r = CreateFence(data->device, &fi, NULL, &fence);
    if (r != VK_SUCCESS) goto failed;
    VkCommandBufferBeginInfo bi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
    r = BeginCommandBuffer(commands, &bi);
    if (r != VK_SUCCESS) goto failed;
    VkImageLayout working = to_image ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                    : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkImageMemoryBarrier into = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = to_image ? VK_ACCESS_TRANSFER_WRITE_BIT : VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, .newLayout = working,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = chain->images[index],
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };
    CmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &into);
    VkBufferMemoryBarrier host = { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT, .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = data->staging, .offset = 0, .size = VK_WHOLE_SIZE };
    if (to_image)
        CmdPipelineBarrier(commands, VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 1, &host, 0, NULL);
    VkBufferImageCopy region = {
        .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .imageExtent = { chain->extent.width, chain->extent.height, 1 } };
    if (to_image) CmdCopyBufferToImage(commands, data->staging, chain->images[index], working, 1, &region);
    else CmdCopyImageToBuffer(commands, chain->images[index], working, data->staging, 1, &region);
    VkImageMemoryBarrier back = into;
    back.srcAccessMask = into.dstAccessMask;
    back.dstAccessMask = 0;
    back.oldLayout = working;
    back.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    CmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &back);
    if (!to_image) {
        host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        CmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT, 0, 0, NULL, 1, &host, 0, NULL);
    }
    r = EndCommandBuffer(commands);
    if (r != VK_SUCCESS) goto failed;
    uint32_t waits = data->present_wait_count;
    if (waits) {
        stages = malloc((size_t)waits * sizeof *stages);
        if (!stages) { r = VK_ERROR_OUT_OF_HOST_MEMORY; goto failed; }
        for (uint32_t i = 0; i < waits; i++) stages[i] = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = waits, .pWaitSemaphores = waits ? data->present_wait : NULL,
        .pWaitDstStageMask = stages, .commandBufferCount = 1, .pCommandBuffers = &commands };
    r = QueueSubmit(queue, 1, &si, fence);
    if (r != VK_SUCCESS) goto failed;
    data->waits_consumed = 1;
    data->present_wait_count = 0;
    r = WaitForFences(data->device, 1, &fence, VK_TRUE, UINT64_MAX);
    if (r != VK_SUCCESS) {
        /* Keep potentially pending commands/fence until device teardown. Never recycle
         * their memory after an unsuccessful wait, or continue presenting this device. */
        data->device_error = r;
        data->stalled_fence = fence;
        free(stages);
        return -1;
    }
    free(stages);
    DestroyFence(data->device, fence, NULL);
    FreeCommandBuffers(data->device, data->pool, 1, &commands);
    return 0;
failed:
    free(stages);
    if (fence) DestroyFence(data->device, fence, NULL);
    if (commands) FreeCommandBuffers(data->device, data->pool, 1, &commands);
    data->transfer_error = r;
    fprintf(stderr, "[nr_layer] transfer failed (%d)\n", (int)r);
    return -1;
}

/* Which pixels are the interface.
 *
 * The shipped feature never has to ask: it inserts the pass before the interface is
 * drawn ("UI remains downstream"). A layer at `vkQueuePresentKHR` sees the composed
 * frame and has to work it out, and the one signal available is motion — an interface
 * holds still while the scene under it does not.
 *
 * That signal cannot separate an interface over a still scene from a still scene, so
 * when almost nothing moved the mask is refused rather than guessed: `settled` returns
 * 0 and the frame is sent unmasked, which is what a photo mode of a paused scene wants.
 */
static uint32_t settled(const unsigned char *now, const unsigned char *before,
			uint32_t pixels, unsigned char *mask)
{
	uint32_t held = 0;
	for (uint32_t i = 0; i < pixels; i++) {
		const unsigned char *a = now + 4 * i, *b = before + 4 * i;
		int moved = abs(a[0] - b[0]) + abs(a[1] - b[1]) + abs(a[2] - b[2]) > 6;
		mask[i] = moved ? 0u : 0xFFu;
		held += !moved;
	}
	return held;
}

/* Whether a mask is worth sending at all. Over nine tenths held still means nothing
 * moved, so an interface cannot be told from a still scene and the mask would cover the
 * whole frame. Under a fiftieth means there is no interface worth a second plane on the
 * wire. Between them the signal is real. */
static int mask_worth_sending(uint32_t held, uint32_t pixels)
{
	return held < (uint32_t)((uint64_t)pixels * 9 / 10) && held > pixels / 50;
}

static int process_frame(struct device_data *data, struct swapchain_data *chain,
			 VkQueue queue, uint32_t index)
{
	VkDeviceSize needed = (VkDeviceSize)chain->extent.width * chain->extent.height * 4;
	if (ensure_resources(data, needed)) return -1;
	if (transfer(data, chain, queue, index, 0)) return -1;

	uint32_t pixels = chain->extent.width * chain->extent.height;
	int masked = 0;
	if (ui_mask && data->have_earlier && data->earlier_size >= needed) {
		if (data->outgoing_size < needed + pixels) {
			unsigned char *grown = realloc(data->outgoing, (size_t)needed + pixels);
			if (!grown) return -1;
			data->outgoing = grown;
			data->outgoing_size = needed + pixels;
		}
		memcpy(data->outgoing, data->mapped, (size_t)needed);
		uint32_t held = settled(data->mapped, data->earlier, pixels,
					data->outgoing + needed);
		masked = mask_worth_sending(held, pixels);
		fprintf(stderr, "[nr_layer] %u%% of the frame held still; ui mask %s\n",
			100u * held / pixels, masked ? "sent" : "refused");
	}

	uint32_t header[4] = { masked ? 0x314E524Eu : 0x304E524Eu, chain->extent.width,
			       chain->extent.height, (uint32_t)chain->format };
	if (capture_path) {
		FILE *file = fopen(capture_path, "wb");
		if (file) {
			fwrite(header, sizeof header, 1, file);
			fwrite(data->mapped, 1, (size_t)needed, file);
			fclose(file);
		}
	}
	if (!socket_path) return -1;
	/* The mask travels immediately after the colour, one byte a pixel. */
	if (masked) {
		if (exchange(header, sizeof header, data->outgoing,
			     (size_t)needed + pixels, data->result, (size_t)needed)) {
			fprintf(stderr, "[nr_layer] the daemon did not answer; frame unchanged\n");
			return -1;
		}
		memcpy(data->mapped, data->result, (size_t)needed);
		fprintf(stderr, "[nr_layer] processed %ux%u with a ui mask\n",
			chain->extent.width, chain->extent.height);
		return 0;
	}
	if (exchange(header, sizeof header, data->mapped, (size_t)needed, data->result,
		     (size_t)needed)) {
		fprintf(stderr, "[nr_layer] the daemon did not answer; frame unchanged\n");
		return -1;
	}
	memcpy(data->mapped, data->result, (size_t)needed);
	fprintf(stderr, "[nr_layer] processed %ux%u\n", chain->extent.width,
		chain->extent.height);
	return 0;
}

/* NR_LAYER_ASYNC: the frame goes out and the answer for the one before it comes back, so
 * the daemon works while the game draws its next frame instead of the game waiting for the
 * daemon; what is on screen is one processed present behind. The old answer is read before
 * the new frame is sent. The other order deadlocks: an answer is larger than a socket's
 * buffer, and the daemon, which takes one connection at a time, cannot accept the new frame
 * until the old answer has been read. */
static int process_frame_async(struct device_data *data, struct swapchain_data *chain,
			       VkQueue queue, uint32_t index)
{
	VkDeviceSize needed = (VkDeviceSize)chain->extent.width * chain->extent.height * 4;
	if (ensure_resources(data, needed) || transfer(data, chain, queue, index, 0)) {
		drop_inflight(data);
		return -1;
	}
	int answered = 0;
	if (data->inflight && data->inflight_size == needed) {
		data->inflight = 0;
		answered = exchange_receive(data->inflight_fd, data->result, (size_t)needed) == 0;
		if (!answered)
			fprintf(stderr, "[nr_layer] the daemon did not answer; frame unchanged\n");
	} else {
		drop_inflight(data);          /* the extent changed under it: it fits nothing */
	}
	if (socket_path) {
		uint32_t header[4] = { 0x304E524Eu, chain->extent.width, chain->extent.height,
				       (uint32_t)chain->format };
		int fd = exchange_send(header, sizeof header, data->mapped, (size_t)needed);
		if (fd >= 0) {
			data->inflight = 1;
			data->inflight_fd = fd;
			data->inflight_size = needed;
		}
	}
	if (!answered) return -1;
	memcpy(data->mapped, data->result, (size_t)needed);
	return 0;
}

/* A completed private fence covers the consumed waits and all copies. Forward the
 * original wait list only when no copy submission has taken ownership of it. */
static VkResult present_now(struct device_data *data, VkQueue queue,
                            const VkPresentInfoKHR *info)
{
    if (data->device_error) return data->device_error;
    if (data->transfer_error) return data->transfer_error;
    if (!data->waits_consumed) return data->present(queue, info);
    VkPresentInfoKHR patched = *info;
    patched.waitSemaphoreCount = 0;
    patched.pWaitSemaphores = NULL;
    return data->present(queue, &patched);
}

static VkResult present_locked(VkQueue queue,
						  const VkPresentInfoKHR *info)
{
	struct device_data *data = NULL;
	/* Resolve the queue rather than taking the first live device: with two devices the
	 * first is not necessarily this one, and the family decides which pool is legal. */
	struct queue_data *owner = find_queue(queue);
	if (owner && !owner->capture_ok) {
		/* A present-only or protected queue: nothing can be recorded on it. Find the
		 * device only to reach its `present` pointer. */
		struct device_data *host = find_device(owner->device);
		if (host) return host->present(queue, info);
	}
	if (owner) {
		data = find_device(owner->device);
		if (data && data->queue_family != owner->family) {
			data->queue_family = owner->family;
			/* Destroy rather than drop: command buffers are allocated and freed
			 * per transfer, so nothing outlives the pool, and a dropped handle
			 * would leak once per family change. */
			if (data->pool) {
				PFN_vkDestroyCommandPool destroy = (PFN_vkDestroyCommandPool)
					data->get_device_proc(data->device, "vkDestroyCommandPool");
				if (destroy) destroy(data->device, data->pool, NULL);
				data->pool = VK_NULL_HANDLE;   /* rebuilt for the right family */
			}
		}
	}
	if (!data)
		for (int i = 0; i < 8; i++) if (devices[i].device) { data = &devices[i]; break; }
	if (!data) return VK_ERROR_INITIALIZATION_FAILED;
	data->frame_counter++;
	if (!data->queues_announced) {
		data->queues_announced = 1;
		announce_queues(data->device, queue, owner ? owner->family : data->queue_family);
	}

	/* Offered, not yet taken: a transfer claims them, and if none runs this present goes
	 * through untouched with its own. */
	data->present_wait = info->pWaitSemaphores;
	data->present_wait_count = info->waitSemaphoreCount;
	data->waits_consumed = 0;
	data->transfer_error = VK_SUCCESS;

	/* Live mode is a slideshow rather than a photo: every Nth present goes through the
	 * network and the frames between re-blit the last result, so the picture is steady
	 * instead of alternating with the game's own. The trigger file is not consulted —
	 * an `access()` per present is a syscall the hot path does not need — and neither
	 * is the interface mask, whose detector is built around a frame that was asked for.
	 *
	 * The rate this can hold is set by the daemon's `--render-scale`, not by N: the
	 * network's cost follows the extent it is given (notes/phase37). N only decides how
	 * many game frames each rendered one covers. */
	if (live_every > 0) {
		/* The trigger keeps its meaning — "do the thing" — so the effect can be
		 * turned on and off mid-game without restarting it. With no trigger
		 * configured, live mode simply always runs. */
		int on = !trigger_path || access(trigger_path, F_OK) == 0;
		if (!on) {
			data->holding = 0;
			drop_inflight(data);
		}
		for (uint32_t i = 0; on && i < info->swapchainCount; i++) {
			if (data->device_error || data->transfer_error) return present_now(data, queue, info);
			struct swapchain_data *chain = find_swapchain(info->pSwapchains[i]);
			if (!chain || info->pImageIndices[i] >= chain->image_count) continue;
            if (data->result_chain != chain->swapchain) {
                data->holding = data->have_earlier = 0;
                data->result_chain = chain->swapchain;
                drop_inflight(data);
            }
			uint32_t index = info->pImageIndices[i];
			VkDeviceSize want = (VkDeviceSize)chain->extent.width
					  * chain->extent.height * 4;
			if (data->frame_counter % (unsigned long)live_every == 0) {
				int done = async_live ? process_frame_async(data, chain, queue, index)
						      : process_frame(data, chain, queue, index);
				if (done == 0) {
					data->holding = 1;
					transfer(data, chain, queue, index, 1);
				} else data->holding = 0; /* never replay a partially received reply */
			} else if (data->holding && data->result_size == want) {
				memcpy(data->mapped, data->result, (size_t)data->result_size);
				transfer(data, chain, queue, index, 1);
			}
		}
		return present_now(data, queue, info);
	}

	/* A file is the trigger, not a key: it works the same on X11 and Wayland, needs
	 * no input hooking inside another process's window, and can be set from a script
	 * or a hotkey daemon. While it exists the processed frame is held on screen. */
	int wanted = trigger_path && access(trigger_path, F_OK) == 0;
	if (!wanted && capture_every > 0)
		wanted = data->frame_counter % (unsigned long)capture_every == 0;

	for (uint32_t i = 0; i < info->swapchainCount; i++) {
		if (data->device_error || data->transfer_error) return present_now(data, queue, info);
		struct swapchain_data *chain = find_swapchain(info->pSwapchains[i]);
		if (!chain || info->pImageIndices[i] >= chain->image_count) continue;
            if (data->result_chain != chain->swapchain) {
                data->holding = data->have_earlier = 0;
                data->result_chain = chain->swapchain;
            }
		uint32_t index = info->pImageIndices[i];
		if (wanted && ui_mask && !data->holding && !data->have_earlier) {
			/* Spend the first present after the trigger keeping the frame, and
			 * process the next one against it. One frame of extra latency on a
			 * pass that already takes a second, and nothing at all while the
			 * trigger is down. */
			VkDeviceSize needed = (VkDeviceSize)chain->extent.width
					      * chain->extent.height * 4;
			if (ensure_resources(data, needed) == 0
			    && transfer(data, chain, queue, index, 0) == 0) {
				if (data->earlier_size < needed) {
					unsigned char *grown = realloc(data->earlier, (size_t)needed);
					if (grown) { data->earlier = grown; data->earlier_size = needed; }
				}
				if (data->earlier_size >= needed) {
					memcpy(data->earlier, data->mapped, (size_t)needed);
					data->have_earlier = 1;
				}
			}
		} else if (wanted && !data->holding) {
			if (process_frame(data, chain, queue, index) == 0) {
				data->holding = 1;
				transfer(data, chain, queue, index, 1);
			}
		} else if (wanted && data->holding
			   && data->result_size == (VkDeviceSize)chain->extent.width
						 * chain->extent.height * 4) {
			memcpy(data->mapped, data->result, (size_t)data->result_size);
			transfer(data, chain, queue, index, 1);
		} else if (!wanted) {
			data->holding = 0;
			data->have_earlier = 0;
		}
	}
	return present_now(data, queue, info);
}

VKAPI_ATTR VkResult VKAPI_CALL nr_QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *info)
{
    struct queue_data *owner = find_queue(queue);
    struct device_data *data = owner ? find_device(owner->device) : NULL;
    if (!data) for (int i = 0; i < 8; i++) if (devices[i].device) { data = &devices[i]; break; }
    if (!data) return VK_ERROR_INITIALIZATION_FAILED;
    nr_mutex_lock(&data->present_lock);
    VkResult result = data->device_error ? data->device_error : present_locked(queue, info);
    nr_mutex_unlock(&data->present_lock);
    return result;
}

#define INTERCEPT(name) if (!strcmp(pName, "vk" #name)) return (PFN_vkVoidFunction)nr_##name

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL nr_GetDeviceProcAddr(VkDevice device,
							      const char *pName)
{
	INTERCEPT(QueuePresentKHR);
	INTERCEPT(CreateSwapchainKHR);
	INTERCEPT(DestroyDevice);
	INTERCEPT(DestroySwapchainKHR);
	INTERCEPT(GetDeviceQueue);
	INTERCEPT(GetDeviceQueue2);
	struct device_data *data = find_device(device);
	return data ? data->get_device_proc(device, pName) : NULL;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL nr_GetInstanceProcAddr(VkInstance instance,
								const char *pName)
{
	INTERCEPT(CreateInstance);
	INTERCEPT(CreateDevice);
	INTERCEPT(GetInstanceProcAddr);
	INTERCEPT(GetDeviceProcAddr);
	INTERCEPT(QueuePresentKHR);
	INTERCEPT(CreateSwapchainKHR);
	INTERCEPT(DestroyDevice);
	INTERCEPT(DestroySwapchainKHR);
	INTERCEPT(GetDeviceQueue);
	INTERCEPT(GetDeviceQueue2);
	return next_instance_proc ? next_instance_proc(instance, pName) : NULL;
}

VKAPI_ATTR VkResult VKAPI_CALL vkNegotiateLoaderLayerInterfaceVersion(
	VkNegotiateLayerInterface *pVersion)
{
	if (pVersion->loaderLayerInterfaceVersion < 2)
		return VK_ERROR_INITIALIZATION_FAILED;
	pVersion->loaderLayerInterfaceVersion = 2;
	pVersion->pfnGetInstanceProcAddr = nr_GetInstanceProcAddr;
	pVersion->pfnGetDeviceProcAddr = nr_GetDeviceProcAddr;
	pVersion->pfnGetPhysicalDeviceProcAddr = NULL;
	return VK_SUCCESS;
}
