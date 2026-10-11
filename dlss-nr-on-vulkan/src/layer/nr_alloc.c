/* nr_alloc.c — a NumPy data allocator that keeps the daemon's large blocks from one frame to the
 * next. Windows only: tools/build_win.bat and CMake build it there, as work/libnr_alloc.dll.
 *
 * Windows' heap gives a freed block of about a megabyte or more straight back to the system, so
 * the next array of that size starts on fresh pages and pays a page fault for every 4 KB of it:
 * ~16 000 faults a 1280x720 frame, 11-15 ms of it (HANDOFF, 2026-10-02). The daemon makes the
 * same full-frame arrays every frame, so a block kept from one frame serves the next.
 *
 * It sits in front of the handler NumPy had. Small blocks go straight through to that one, and
 * so does every large block it does not keep, so nothing here allocates or frees on a heap of
 * its own. Kept blocks are matched by exact size. nr_alloc_frame() marks the end of a frame, and
 * a block that a whole frame did not take again goes back then: what stays is what a frame
 * reuses, and a change of extent or scale gives the old sizes back a frame later. A cap bounds
 * it in between. src/layer/nr_alloc.py installs it through NumPy's C API.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#define EXPORT __declspec(dllexport)

/* numpy/ndarraytypes.h, handler version 1 (NumPy 1.22 and later) */
typedef struct {
	void *ctx;
	void *(*malloc)(void *ctx, size_t size);
	void *(*calloc)(void *ctx, size_t nelem, size_t elsize);
	void *(*realloc)(void *ctx, void *ptr, size_t new_size);
	void (*free)(void *ctx, void *ptr, size_t size);
} PyDataMemAllocator;

typedef struct {
	char name[127];
	uint8_t version;
	PyDataMemAllocator allocator;
} PyDataMem_Handler;

#define SLOTS 256

static PyDataMemAllocator next;          /* the handler NumPy had: every block comes from it */
static SRWLOCK lock = SRWLOCK_INIT;
static struct {
	void *block;
	size_t size;
	uint64_t frame;                  /* the frame it was given back in */
} kept[SLOTS];
static size_t threshold = (size_t)1 << 20;
static size_t cap = (size_t)256 << 20;    /* a 2560x1440 frame keeps 172 MB */
static int poison;                       /* tests: fill every large block handed out */
static size_t kept_bytes;
static size_t most, last_most;           /* the most kept at once: this frame, the last one */
static uint64_t frame;
static int busy;                         /* a large block came or went since the last mark */
static long long hits, misses, returned;

static void *take(size_t size)
{
	void *block = NULL;
	AcquireSRWLockExclusive(&lock);
	for (int i = 0; i < SLOTS; i++)
		if (kept[i].block && kept[i].size == size) {
			block = kept[i].block;
			kept[i].block = NULL;
			kept_bytes -= size;
			break;
		}
	if (block)
		hits++;
	else
		misses++;
	busy = 1;
	ReleaseSRWLockExclusive(&lock);
	return block;
}

static int give(void *block, size_t size)
{
	int done = 0;
	AcquireSRWLockExclusive(&lock);
	busy = 1;
	if (kept_bytes + size <= cap)
		for (int i = 0; i < SLOTS; i++)
			if (!kept[i].block) {
				kept[i].block = block;
				kept[i].size = size;
				kept[i].frame = frame;
				kept_bytes += size;
				if (kept_bytes > most)
					most = kept_bytes;
				done = 1;
				break;
			}
	ReleaseSRWLockExclusive(&lock);
	return done;
}

static void *keep_malloc(void *ctx, size_t size)
{
	(void)ctx;
	void *block = size >= threshold ? take(size) : NULL;
	if (!block)
		block = next.malloc(next.ctx, size);
	if (block && poison && size >= threshold)
		memset(block, 0xff, size);
	return block;
}

static void *keep_calloc(void *ctx, size_t count, size_t each)
{
	(void)ctx;
	if (each && count > SIZE_MAX / each)
		return NULL;
	size_t size = count * each;
	void *block = size >= threshold ? take(size) : NULL;
	if (block)
		return memset(block, 0, size);
	return next.calloc(next.ctx, count, each);
}

static void *keep_realloc(void *ctx, void *block, size_t size)
{
	(void)ctx;
	return next.realloc(next.ctx, block, size);
}

static void keep_free(void *ctx, void *block, size_t size)
{
	(void)ctx;
	if (block && size >= threshold && give(block, size))
		return;
	next.free(next.ctx, block, size);
}

static PyDataMem_Handler handler = {
	"nr_keep_blocks", 1, { NULL, keep_malloc, keep_calloc, keep_realloc, keep_free } };

/* This handler, in front of `current`, the one NumPy has in the installing thread's context.
 * The first install decides what it wraps; a later one, in another thread, finds it set. */
EXPORT PyDataMem_Handler *nr_alloc_handler(const PyDataMem_Handler *current)
{
	AcquireSRWLockExclusive(&lock);
	if (!next.malloc && current && current != &handler && current->version == 1)
		next = current->allocator;
	ReleaseSRWLockExclusive(&lock);
	return next.malloc ? &handler : NULL;
}

/* The end of a frame. Gives back every block given back before the last mark and not taken
 * since, and returns how many bytes that was. A mark with no large block in or out since the
 * last one — a status check between frames — changes nothing. Called with the GIL held: the
 * blocks go back through NumPy's own free. */
EXPORT size_t nr_alloc_frame(void)
{
	struct { void *block; size_t size; } out[SLOTS];
	int count = 0;
	size_t bytes = 0;
	AcquireSRWLockExclusive(&lock);
	if (busy) {
		for (int i = 0; i < SLOTS; i++)
			if (kept[i].block && kept[i].frame < frame) {
				out[count].block = kept[i].block;
				out[count++].size = kept[i].size;
				kept_bytes -= kept[i].size;
				kept[i].block = NULL;
			}
		frame++;
		busy = 0;
		returned += count;
		last_most = most;
		most = kept_bytes;
	}
	ReleaseSRWLockExclusive(&lock);
	for (int i = 0; i < count; i++) {
		bytes += out[i].size;
		next.free(next.ctx, out[i].block, out[i].size);
	}
	return bytes;
}

/* Every kept block back, now. */
EXPORT size_t nr_alloc_release(void)
{
	struct { void *block; size_t size; } out[SLOTS];
	int count = 0;
	size_t bytes = 0;
	AcquireSRWLockExclusive(&lock);
	for (int i = 0; i < SLOTS; i++)
		if (kept[i].block) {
			out[count].block = kept[i].block;
			out[count++].size = kept[i].size;
			kept[i].block = NULL;
		}
	kept_bytes = most = 0;
	returned += count;
	ReleaseSRWLockExclusive(&lock);
	for (int i = 0; i < count; i++) {
		bytes += out[i].size;
		next.free(next.ctx, out[i].block, out[i].size);
	}
	return bytes;
}

/* The smallest block kept, the most kept at once, and the tests' poison. */
EXPORT void nr_alloc_configure(size_t new_threshold, size_t new_cap, int new_poison)
{
	AcquireSRWLockExclusive(&lock);
	threshold = new_threshold;
	cap = new_cap;
	poison = new_poison;
	ReleaseSRWLockExclusive(&lock);
}

/* hits, misses, blocks given back, bytes kept, the most bytes kept at once during the last
 * frame, slots in use */
EXPORT void nr_alloc_counts(long long out[6])
{
	int used = 0;
	AcquireSRWLockShared(&lock);
	for (int i = 0; i < SLOTS; i++)
		used += kept[i].block != NULL;
	out[0] = hits;
	out[1] = misses;
	out[2] = returned;
	out[3] = (long long)kept_bytes;
	out[4] = (long long)last_most;
	out[5] = used;
	ReleaseSRWLockShared(&lock);
}
