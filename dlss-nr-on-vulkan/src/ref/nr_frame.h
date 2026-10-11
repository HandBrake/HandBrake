/*
 * nr_frame — one RGB frame in, one RGB frame out, through the recovered graph, in C.
 *
 * The C library that `src/ref/nr_frame.py` is: the same 16-channel feature assembly,
 * the same 71-block graph recorded against `libxmx` dispatch for dispatch as
 * `nr_frame_resident.py` records it, the same head-to-RGB composition. The head it
 * produces is bit-identical to the Python resident path on the same device — the
 * dispatches are the same, so the bytes are — and `src/ref/test_nr_frame_c.py` checks
 * that. The three places it can differ from NumPy by the last bit are named where they
 * happen: the noise channels, the temporal gate's exponential, and the detail blur.
 *
 * Images are float32 RGB in [0, 1], row-major (height, width, 3); the head is
 * (height, width, 4). One `nr_frame` holds the weights on the device and the graph for
 * the most recent extent; a new extent rebuilds the graph and keeps the weights.
 *
 * The host passes around the graph — feature assembly, the composition and its temporal
 * gate and floor, the detail blur — split their rows across nr_image.c's thread pool, one
 * thread per core (`NR_HOST_THREADS`, else `OMP_NUM_THREADS`, to change it, 1 for none).
 * The bytes do not depend on the count. The temporal gate is a 65536-entry table over the
 * half logit, built once per blend scale, as `nr_frame.gate_table` is in the Python.
 *
 *     nr_frame *f = nr_frame_open("work/mlxw/dlssnr-logical.safetensors");   // or NULL: the weights compiled in
 *     nr_frame_params p; nr_frame_defaults(&p);
 *     nr_frame_update(f, colour, height, width, NULL, NULL, &p, output, NULL);
 *     nr_frame_close(f);
 */
#ifndef NR_FRAME_H
#define NR_FRAME_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nr_frame nr_frame;

typedef struct nr_frame_params {
    /* the composition: post-network, free to sweep */
    float intensity;          /* blend of the model's picture against the source; > 1 extrapolates */
    float detail_strength;    /* high-frequency weight of the change (1 = unchanged) */
    float colour_strength;    /* low-frequency weight of the change (1 = unchanged) */
    float detail_radius;      /* sigma of the split, in pixels */
    /* the conditioning: reaches the network, so a change costs a forward pass */
    float normalized_style;   /* vendor style index / 128 */
    float local_tone;
    float local_structure;
    int   frame_index;        /* seeds the three noise channels */
    /* the temporal path, used when `history` is given */
    float history_confidence; /* scales the model's own gate: 0 the still path, 1 its answer */
    float blend_scale;        /* half(0.73974609375), the recovered package's */
    float hold;               /* floor under the gate where `previous` matches the colour: */
    float slope;              /*   max(alpha, min(clamp(moved * slope + hold, 0, hold), 1) * blend_scale) */
    float release;            /* where `previous` does not match: alpha *= clamp(moved * release + 1, 0, 1),
                               * before the floor; folded like `slope`, -255 / levels (`nr_frame.release_slope`),
                               * 0 off */
    /* the automatic mask: skin structure and automatic-mask structure, each -1 to follow
     * `local_structure`; off unless `automatic_mask` is set */
    int   automatic_mask;
    float skin_structure;
    float automatic_structure;
    /* the network's floor: the frame is padded by mirroring to at least this a side and to a
     * multiple of 64 (`nr_frame.network_geometry`). 320, the default, is the vendor's; the
     * graph runs down to 128, and a small live frame is then mostly picture, not padding.
     * Appended last, so a host built against the older header must be rebuilt. */
    int   min_extent;
    /* The vendor's colour grade after the network (`nr_frame.grade_for`, notes/phase70):
     * style 1 (natural) an exposure of -0.1 EV, a contrast of -0.25 and a saturation of
     * -0.1, style 2 (cinematic) a saturation of -0.15, each times clamp(local_tone, 0, 1);
     * style 0 none. It follows `normalized_style` and `local_tone` unless this is set.
     * Appended last, like `min_extent`. */
    int   grade_off;
} nr_frame_params;

/* The values `nr_frame.py` uses when nothing is asked: the `standard` profile. */
void nr_frame_defaults(nr_frame_params *params);

/* Load the logical weights and open the device. NULL on failure; `nr_frame_error()`.
 * A NULL path means the weights compiled into this library — CMake compiles the bin2c
 * slices in the weights directory (NR_WEIGHTS_DIR, regenerated from the safetensors with
 * NR_BIN2C_WEIGHTS) — and fails with a message in a build without them. */
nr_frame *nr_frame_open(const char *weights_path);

/* The size of the embedded safetensors, or 0 when this build carries none. */
size_t nr_frame_embedded_weights_size(void);

/* Write the weights as a prepared file: the logical safetensors (from `weights_path`, or the
 * compiled-in weights for NULL) with the loaders' layout work already done — the branched
 * blocks' fused expansion, the window biases in logical order, the head matrix, the merge's
 * sine and cosine joined — so an open from it, on any runtime, uploads what it reads. The
 * originals are kept, so `nr_frame_open` takes either file and the device receives the same
 * bytes from both, and the Python graph reads either too (`nr_model.load_logical`). Since v2
 * the tensors are written F32 first, then F16, so every offset is aligned to its element and a
 * reader points into the file (or the compiled-in slices) instead of copying. Preparing a
 * prepared file keeps what is there. Needs no device. 0, or -1 with `nr_frame_error()`.
 * `nr_frame --prepare OUT [--weights IN]` is this. The file derives from NVIDIA's weights and
 * is never committed or redistributed, as the logical one is not; the compiled-in weights
 * (`weights/`, regenerated from `dlssnr-prepared.safetensors`) are this format. */
int nr_frame_prepare(const char *weights_path, const char *out_path);
/* Whether the weights this frame opened were prepared (the metadata says so: the derived
 * tensors are in, and from `v2` the layout lets the reader alias every tensor); -1 for NULL.
 * The compiled-in weights are prepared since 2026-10-10. And how many of the tensors were
 * read in place — out of the compiled-in slices or the file's mapping — rather than copied,
 * of how many there are: with prepared weights and slices cut at tensor boundaries, all. */
int nr_frame_prepared(const nr_frame *frame);
size_t nr_frame_weights_aliased(const nr_frame *frame);
size_t nr_frame_weights_count(const nr_frame *frame);
void nr_frame_close(nr_frame *frame);

/* Share a host's Vulkan instance and device instead of letting libxmx create its own.
 * Call before the first `nr_frame_open` (or after `nr_frame_shutdown`); the next open
 * takes the objects. Handles are `void *` (VkInstance, VkPhysicalDevice, VkDevice,
 * VkQueue) so no Vulkan header is needed here. The device needs Vulkan 1.3 with
 * storageBuffer16BitAccess, vulkanMemoryModel (+DeviceScope), shaderFloat16,
 * bufferDeviceAddress and scalarBlockLayout enabled, plus VK_KHR_portability_subset
 * where offered; `queue` must belong to a compute-capable family `queue_family`.
 * `cooperative_matrix` is a flags word: 1 (XMX_ADOPT_COOPMAT) when VK_KHR_cooperative_matrix
 * is enabled, plus 2 (XMX_ADOPT_EXPLICIT_LAYOUT) when VK_KHR_workgroup_memory_explicit_layout
 * is enabled with its scalar-block-layout and 16-bit-access features — the staged GEMM needs
 * it, and without it those shapes run on the smaller kernels and the window partition stays
 * a pass of its own. If
 * the host submits on the same queue it passes `lock`/`unlock` (bracketing every
 * submit here) and takes the same lock around its own submits, presents and idle
 * waits. `get_instance_proc_addr` is the host's vkGetInstanceProcAddr. 0 on success;
 * `nr_frame_error()` otherwise, and libxmx keeps making its own device.
 *
 * Refused, with a message, when the runtime behind this library is Metal (the Apple
 * libdlssnr, or NR_GPU_BACKEND=metal) or Direct3D 12 (a Windows libdlssnr built with
 * NR_DLSSNR_D3D12, or NR_GPU_BACKEND=d3d12): there is nothing Vulkan to adopt, and the
 * runtime opens its own device — or, on Direct3D 12, takes the host's through
 * `nr_frame_adopt_d3d12`. `nr_frame_runtime()` says which one is behind. */
int nr_frame_adopt_vulkan(void *instance, void *physical_device, void *device, void *queue,
                          unsigned queue_family, int cooperative_matrix,
                          void *get_instance_proc_addr, void (*lock)(void *),
                          void (*unlock)(void *), void *lock_context);

/* The Direct3D 12 counterpart, for the libd3dmx runtime: share the host's ID3D12Device and
 * ID3D12CommandQueue (as `void *`, so no D3D header is needed here). Lists are recorded
 * for the queue's type, direct or compute. The device must offer Shader Model 6.2 and
 * native 16-bit shader operations. If the host also submits on `queue` it passes
 * `lock`/`unlock` (bracketing every ExecuteCommandLists here) and takes the same lock
 * around its own submits and presents. Refused when the runtime behind this library is
 * not Direct3D 12. 0 on success; `nr_frame_error()` otherwise. */
int nr_frame_adopt_d3d12(void *device, void *queue, void (*lock)(void *), void (*unlock)(void *),
                         void *lock_context);

/* Release the device libxmx holds — its own, or an adopted one back to its host,
 * untouched. Every `nr_frame` must have been closed first. The next `nr_frame_open`
 * opens the device again (adopting whatever `nr_frame_adopt_vulkan` handed over since).
 * A host must call this before destroying a device it shared. */
void nr_frame_shutdown(void);

/* 1 while an adopted (shared) device is open, 0 for libxmx's own, -1 when none is. */
int nr_frame_shared_device(void);

/* The compute runtime behind this library: "vulkan" (libxmx), "metal" (libmetalmx — the
 * Apple libdlssnr, or a shared build under NR_GPU_BACKEND=metal) or "d3d12" (libd3dmx — a
 * Windows libdlssnr built with NR_DLSSNR_D3D12, or NR_GPU_BACKEND=d3d12). Known without
 * opening a device, so a host can decide which device, if any, to share. */
/* Abandon a model open in progress (nr_frame_open), from another thread.
 *
 * The open compiles every pipeline, which a driver can take tens of seconds over. A caller
 * that has decided it no longer wants the model -- a renderer about to destroy the device it
 * lent, say -- sets this, and the open fails after whatever pipeline is already being built
 * rather than after all of them. Clear it before opening again. Nothing is left open by an
 * open that ends this way.
 *
 * Set before the runtime is loaded, it still applies to the open that loads it.
 *
 * Harmless where the runtime does not support it (an older libxmx): the open then runs to
 * completion as it always did. */
void nr_frame_cancel_open(int on);
/* Whether the flag is set, so a caller can tell its own cancellation from a real failure. */
int nr_frame_open_cancelled(void);

const char *nr_frame_runtime(void);

/* The last failure, for the calling thread's most recent call. */
const char *nr_frame_error(void);
const char *nr_frame_device(nr_frame *frame);
const char *nr_frame_gemm_path(nr_frame *frame);

/* Three answers about the compute runtime, once a frame has been opened (-1 before, or on a
 * runtime too old to say): whether the device is a card with memory of its own; whether the
 * GEMMs keep float16 subnormals — libxmx declares `DenormPreserve 16` on every module where
 * the driver reports it can, so Mesa and Intel's Windows driver compute one graph bit for
 * bit (notes/phase71), and Metal and Direct3D 12 keep them without a declaration; and which
 * spelling of `half_round` the pipelines compile, 1 the cast `float(float16_t(x))` and 0 the
 * pack-and-unpack round trip, chosen per driver because each driver's compiler folds one of
 * them away (`xmx_half_by_cast`, `XMX_HALF_ROUND` to override on Vulkan). */
int nr_frame_discrete(void);
int nr_frame_preserve16(void);
int nr_frame_half_by_cast(void);
/* And two more: whether the staged kernel copies its operands global-to-shared as raw 128-bit
 * vectors (`xmx_staged_packed`: 1 by default on Vulkan and Metal, 0 under
 * `XMX_STAGED_PACKED=0`, always 0 on Direct3D 12, which has no staged kernel), and whether
 * the portable GEMM fetches A that way (`xmx_portable_packed`: 0 by default on every runtime,
 * 1 under `XMX_PORTABLE_PACKED=1`). The same bits either way. */
int nr_frame_staged_packed(void);
int nr_frame_portable_packed(void);
/* Whether this frame builds its half features in the graph's mapped input itself (1) or on
 * the host and copies them in (0): NR_INPUT_VIEW, defaulting to 0 only on a discrete card
 * under Windows, where one driver returned NaN for the mapped path (HANDOFF, 2026-10-02). */
int nr_frame_input_view(const nr_frame *frame);

/* The network extent for an output extent, as the vendor pads it: each side aligned to the
 * graph's own reductions, at least 320, one alignment wider when both sides are four
 * alignments (`nr_frame.network_geometry`; 1280x720 -> 1344x768), a multiple of 64. */
void nr_frame_geometry(int height, int width, int *network_height, int *network_width);
/* The same at the floor `minimum` (`nr_frame_params.min_extent`), never below 128. */
void nr_frame_geometry_min(int height, int width, int minimum, int *network_height, int *network_width);

/* The frame the network is handed for a width x height picture at render `scale`
 * (`nr_frame.render_extent`): of all frames at least the scale's own, aspect kept, the one on
 * the cheapest network field at the floor `minimum`, and of those the largest — so a low
 * scale on a small window fills its field instead of paying for mirror padding (640x360 runs
 * as 0.5 for any scale up to it), and a lower scale is never the slower. From 1.0 up, the
 * picture itself. */
void nr_frame_render_extent(int width, int height, float scale, int minimum,
                            int *render_width, int *render_height);

/* The grade `params` asks for, as the three factors nr_image.c takes (exposure multiplier,
 * contrast, saturation multiplier); 0 when there is none. */
int nr_frame_grade(const nr_frame_params *params, float grade[3]);

/* colour (h, w, 3) -> output (h, w, 3). `history` is the previous output or NULL;
 * `previous` the previous *input* or NULL, for the floor. `head_out` (h, w, 4) optional.
 * 0 on success. */
int nr_frame_update(nr_frame *frame, const float *colour, int height, int width,
                    const float *history, const float *previous,
                    const nr_frame_params *params, float *output, float *head_out);

/* The same with a per-pixel control mask (h, w, 3): red scales the blend in the
 * composition, green the tone and blue the structure that reach the network. */
int nr_frame_update_masked(nr_frame *frame, const float *colour, int height, int width,
                           const float *history, const float *previous, const float *control_mask,
                           const nr_frame_params *params, float *output, float *head_out);

/* The same, keeping the vendor's history: `neural` (h, w, 3), when not NULL, receives the
 * prediction after the history's blend and before the grade, the intensity, the mask and
 * the detail split, truncated to half — what the vendor carries to the next frame in
 * RGBA16F. Hand it back as the next frame's `history` to follow the vendor's temporal
 * path (as nr_daemon.py and nr_temporal.VendorSession do); handing back `output` instead is
 * MLX-DLSS's. Every entry point without `neural` is its `_neural` twin with NULL. */
int nr_frame_update_neural(nr_frame *frame, const float *colour, int height, int width,
                           const float *history, const float *previous, const float *control_mask,
                           const nr_frame_params *params, float *output, float *head_out,
                           float *neural);

/* The halves, for a caller that wants to sit between them: features, the network, and
 * the composition of a cropped (h, w, 4) head — free to repeat at another intensity. */
int nr_frame_features_masked(nr_frame *frame, const float *colour, int height, int width,
                             const float *history, const float *control_mask,
                             const nr_frame_params *params, float *features);
int nr_frame_compose(nr_frame *frame, const float *head, const float *colour, int height, int width,
                     const float *history, const float *previous, const float *control_mask,
                     const nr_frame_params *params, float *output);
int nr_frame_compose_neural(nr_frame *frame, const float *head, const float *colour, int height,
                            int width, const float *history, const float *previous,
                            const float *control_mask, const nr_frame_params *params,
                            float *output, float *neural);

int nr_frame_features(nr_frame *frame, const float *colour, int height, int width,
                      const float *history, const nr_frame_params *params,
                      float *features /* (network_height, network_width, 16) */);
int nr_frame_run_features(nr_frame *frame, const float *features,
                          int network_height, int network_width,
                          float *head /* (network_height, network_width, 4) */);

/* The network half of `nr_frame_update`, as the daemon runs it under a render scale:
 * the features built as the update builds them (as half in the graph's own input under
 * NR_INPUT_FP16), the graph, and the head cropped to (h, w, 4). No float32 feature array
 * and no composition, so the caller can resample the head and compose it at another
 * extent with `nr_frame_compose`. `history` (h, w, 3) or NULL, `control_mask` likewise. */
int nr_frame_head(nr_frame *frame, const float *colour, int height, int width,
                  const float *history, const float *control_mask,
                  const nr_frame_params *params, float *head);

/* The daemon's end of a frame (`nr_frame.compose_encode`): the head (head_height,
 * head_width, 4) — at a render scale, smaller than the colour — brought up bilinearly to
 * (h, w) as `nr_daemon.resample` does, composed as `nr_frame_compose` composes it into
 * `output` (h, w, 3), and encoded to 8 bits into `encoded` at (top, left) of a frame
 * `frame_width` pixels wide, RGBA or with `bgra` BGRA. `encoded` is a copy of the request:
 * its alpha and everything outside the region stay as they came. Where the composition's
 * detail split is a no-op (detail and colour strength 1) all of it is one pass over the
 * output (nr_compose_encode); otherwise the separate passes, the same bytes either way. */
int nr_frame_compose_encode(nr_frame *frame, const float *head, int head_height, int head_width,
                            const float *colour, int height, int width, const float *history,
                            const float *previous, const float *control_mask,
                            const nr_frame_params *params, float *output, unsigned char *encoded,
                            int frame_width, int top, int left, int bgra);
int nr_frame_compose_encode_neural(nr_frame *frame, const float *head, int head_height,
                                   int head_width, const float *colour, int height, int width,
                                   const float *history, const float *previous,
                                   const float *control_mask, const nr_frame_params *params,
                                   float *output, unsigned char *encoded, int frame_width,
                                   int top, int left, int bgra, float *neural);

/* Seconds spent, in the last run, writing the input, running the graph, reading the
 * head: 0, 1, 2. On a discrete card the outer two are the bus. */
double nr_frame_split(const nr_frame *frame, int which);

/* The daemon's frame, stateful, in one call: `nr_daemon.process_connection` on its default
 * path — 8-bit pixels in, 8-bit pixels out, the letterbox found once and afterwards checked
 * (`Letterbox`), the render scale's frame (`nr_frame_render_extent`), the history taken and
 * dropped as `History` takes it (on a new geometry or profile, or a cut past `cut_limit`),
 * the network, and the head brought up, composed and encoded in one pass, keeping the
 * vendor's history for the next frame. So a host drives the whole live pass through C with
 * no daemon and no socket, on whichever runtime is behind the library, and a `--dump`
 * capture replayed through it in its order gets the history the daemon gave it
 * (`nr_frame --replay`, `src/bench/restill.py --native`). The interface mask is not taken.
 *
 *     nr_frame_live *live = nr_frame_live_open(frame);
 *     nr_frame_live_settings s; nr_frame_live_defaults(&s);
 *     nr_frame_live_run(live, &params, &s, bgra, width, height, 1, answer, &report);
 *
 * `params` carries the conditioning and the composition (a profile, the intensity, the two
 * strengths, `min_extent`); its temporal fields are overwritten from `settings`, which are
 * the daemon's knobs in its own units. One session holds one shot's history; the frame
 * must outlive it. */
typedef struct nr_frame_live nr_frame_live;

typedef struct nr_frame_live_settings {
    float render_scale;       /* 0.05-1: the network's frame, as a share of the picture's */
    float temporal;           /* the history's confidence, 0-1; 0 turns the history off */
    float cut_limit;          /* mean change of the network's input above which the history goes */
    float hold;               /* the floor where the game handed back the same pixel, 0-1 */
    float release;            /* levels of 255 over which the gate lets go of a changed pixel; 0 off */
    int   letterbox;          /* 1: work on the region inside symmetric black bars */
} nr_frame_live_settings;

typedef struct nr_frame_live_report {
    int    top, bottom, left, right;          /* the active region worked on */
    int    render_width, render_height;       /* the frame the network was handed */
    int    network_width, network_height;     /* and the field it ran on */
    int    with_history;                      /* 1 when the previous frame's history was used */
    double cut;                               /* mean |change| of the network's input, 0 without history */
    double change;                            /* mean |composition - colour| on every fourth row */
} nr_frame_live_report;

/* nr_daemon.py's: scale 1, temporal 1, cut limit 0.15, hold 1, release 24, letterbox on. */
void nr_frame_live_defaults(nr_frame_live_settings *settings);
nr_frame_live *nr_frame_live_open(nr_frame *frame);
/* Forget the history and the letterbox; the next frame starts a shot. */
void nr_frame_live_reset(nr_frame_live *live);
void nr_frame_live_close(nr_frame_live *live);
/* `pixels` and `answer` are (height, width, 4) bytes, BGRA with `bgra` else RGBA; alpha and
 * the bars come back as they went in. `answer` may be `pixels`. `params` NULL for the
 * `standard` profile, `settings` NULL for the defaults, `report` NULL for none. 0 on
 * success; `nr_frame_error()` otherwise, and the history is dropped. */
int nr_frame_live_run(nr_frame_live *live, const nr_frame_params *params,
                      const nr_frame_live_settings *settings, const unsigned char *pixels,
                      int width, int height, int bgra, unsigned char *answer,
                      nr_frame_live_report *report);

#ifdef __cplusplus
}
#endif
#endif
