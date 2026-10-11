# Phase 74 — libmetalmx: the runtime on Metal, the shaders through the Metal compiler

**2026-09-21, on the owner's Apple M3.** Until today the only way onto Apple silicon was
MoltenVK: libxmx's Vulkan calls translated to Metal by a third party, SPIRV-Cross
translating the SPIR-V to MSL at pipeline creation, and no matrix path at all because
SPIRV-Cross cannot translate the cooperative-matrix opcodes (`phase67`). Now there is a
second compute runtime, **`libmetalmx`**, that drives Metal directly and carries the
shaders written in the Metal Shading Language, compiled by Apple's compiler into one
`.metallib` that bin2c puts inside the library. It exports every `xmx_*` symbol in
`xmx.h`, so `nr_frame.c`, `xmx.py` and `xmxres.py` bind it exactly as they bind libxmx;
**`NR_GPU_BACKEND=metal`** is the switch, in `src/nr_build.py` for the Python and in
`nr_frame.c` for the C. It is built on Apple alone — the Makefile puts its targets under
`ifeq ($(UNAME_S),Darwin)`, CMake under `NR_BUILD_METAL`, on by default on Apple and refused
anywhere else.

## What it is

| | libxmx | libmetalmx |
| --- | --- | --- |
| source | `src/gpu/libxmx.c` | `src/gpu/libmetalmx.m` (Objective-C, ARC) |
| shaders | `src/gpu/*.comp` → SPIR-V, bin2c | `src/gpu/metal/*.metal` → AIR → `nr_shaders.metallib`, bin2c |
| matrix path | `VK_KHR_cooperative_matrix`, 8x16x16 | `simdgroup_matrix`, 8x8, half in / float accumulate |
| operands | `VkDeviceAddress` in push constants | the buffer's GPU address in the same 64-byte struct, `setBytes` at index 0. Read back through an `MTLArgumentEncoder` over one pointer argument (Metal 2, macOS 10.13), not `MTLBuffer.gpuAddress` (macOS 13): a deployment target of 11.0 warned on it, and the two agree on every buffer of both storage modes (400 checked, 2026-09-21) |
| barrier | `vkCmdPipelineBarrier` | `memoryBarrierWithScope:MTLBarrierScopeBuffers` in a concurrent encoder |
| copy | `vkCmdCopyBuffer` | a blit encoder between compute encoders, fenced |
| graph | a re-submittable command buffer | the recorded op list, re-encoded on every run (~1 ms a frame) |
| profiling | timestamp queries | counter sampling at encoder boundaries, one encoder per pass |
| specialization | constant 0 | function constant 0 |
| adopt | a host's Vulkan device | refused: there is nothing Vulkan to adopt |

The shader names stay the SPIR-V names. `xmx_init("gemm_coopmat.spv")` runs the Metal kernel
`gemm_coopmat`; a path is reduced to its base name (so `XMX_ROW_SPV=work/attention_ab.spv`
still selects `attention_ab`); `XMX_METALLIB=/path/to.metallib` loads a file instead of the
embedded library. `xmx_embedded_shader(name)` answers for the fourteen kernels the metallib
carries, so `nr_build.shader_arg` and `nr_frame.c` hand over bare names as they do for the
CMake-built libxmx.

Fourteen kernels, five sources: `resident`, `history`, `attention` (+`attention_ab` as a
template instance), `gemm_portable` (+`_tiled`, `_desc`, `_batched`), and `gemm_simd` with
`gemm_resident`, `gemm_tiled`, `gemm_staged`, `gemm_coopmat`, `gemm_batched`, `gemm_f16acc`.
The 8x16x16 tile is two `simdgroup_float8x8` accumulators fed by two K slices — four
`simdgroup_multiply_accumulate`s where the GLSL has one `coopMatMulAdd` — and the header
confirms the mixed overload (`simdgroup_multiply_accumulate(float8x8 &, half8x8, half8x8,
float8x8)`), so the half x half product is exact in float32 as it is on XMX. The transposed
key is the `transpose_matrix` argument of `simdgroup_load`, no copy on either side.

## What the port had to get right

Three things, none of them in the arithmetic.

1. **`-fno-fast-math -ffp-contract=off` on the Metal compiler.** Metal's default is fast
   math *with* contraction. `phase67` measured what that does through MoltenVK — the gate's
   multiply and add fused into one FMA, the half rounding between them skipped, 3e-03 on the
   gate epilogue and a quantum on every E4M3 publish after it. Every `precise` in the GLSL
   is therefore the default in these kernels and needs no spelling; `half_round` is
   `float(half(x))`, a real conversion pair.

2. **A fence between encoders.** The graph's buffers are `MTLResourceHazardTrackingModeUntracked`
   (the recording carries its own barriers, as the Vulkan one does). With tracking off, Metal
   is free to overlap a blit encoder with the compute encoder that wrote its source, and it
   does: every block passed bit-exact on its own, and the whole 320x320 graph came out at
   correlation **0.25** with the head's spread **19.7x** too wide, because the skip copies
   ran before the blocks that fed them. One `MTLFence`, updated by every encoder on its way
   out and waited on by the next on its way in, and the same graph reads **0.9786** / 0.985.
   (The block tests could not see it: they have no copies.)

3. **Do not touch `thread_elements()` per element.** The GLSL runs the epilogue on the
   cooperative matrix's own components and stores straight to global. Metal's equivalent,
   `simdgroup_matrix::thread_elements()`, is typed `vec<T, 64>` — the whole 8x8 tile per
   thread, with the hardware mapping hidden by the compiler — and a loop over it with a
   dynamic index compiled to something that cost **19 ms a pass against 0.7 for the plain
   store**, 27x, and 81 % of the frame: every epilogue GEMM at 7.3 ms. The kernel now stores
   the accumulators untouched to threadgroup memory and publishes scalars on the way out,
   which is what `gemm_staged.comp` always did; the same pass is 0.72-0.93 ms, and the
   output is byte-identical to before (it is the same publish of the same values).

Smaller ones: a kernel that reads a function constant must be created through
`newFunctionWithName:constantValues:` even with nothing set (the plain lookup is refused at
pipeline creation, `validateWithDevice:1530`); a counter sample buffer is capped at 32 KB, so
8192 stamps take four of them; `int id` as a parameter name shadows Objective-C's `id` type
inside any function that also declares one; ARC forbids Metal objects in C structs, so the
tables hold `CFBridgingRetain`ed `void *`.

## Numbers, M3

Every test that runs on libxmx runs on libmetalmx with `NR_GPU_BACKEND=metal`, and the GEMM
contract is **exact** on both paths — every shape, epilogue, narrow store and strided slice in
`test_portable.py` at max |d| 0 (5.96e-07 on one 8x16x16 and 5.7e-06 on the batched
descriptor path, the same as MoltenVK); `test_epilogue`, `test_specialization` (51
pipelines), `test_softmax_pack`, `test_qkv_fusion`, `test_graph` (also under `XMX_STAGING=1`,
private buffers reached by blits), `test_scratch_arena`, `test_frame_execution` and
`test_resident` all green, the whole-graph checks included.

**A real 1280x720 frame through the C library** (`build-metal/nr_frame`, embedded weights, warm,
four runs each):

| backend | graph time |
| --- | --- |
| libmetalmx, simdgroup matrix | **735-746 ms** (880 cold) |
| libxmx through MoltenVK, portable | 938-948 ms |

Metal against MoltenVK on the same input: correlation 0.999870, mean 0.28 of 255 levels, max
5, 39 % of pixels identical — the two GEMMs sum in different orders and the graph is chaotic
(`phase9`), so this is the agreement two correct implementations have. Both move the input
by the same amount (mean 3.43 levels, correlation 0.99425 with it).

The frame, per pass (`frame_profile.py`, 1280x768, device totals; profiling puts every pass
in its own encoder, so the wall time under it is 816 ms rather than 740):

| pass | ms | passes | us each |
| --- | --- | --- | --- |
| gemm staged | 210 | 659 | 319 |
| gemm tiled | 169 | 306 | 553 |
| softmax | 91 | 70 | 1295 |
| cosine publish | 64 | 140 | 460 |
| residual | 54 | 141 | 382 |
| gemm tiled, transposed | 38 | 62 | 608 |
| partition / split / merge | 54 | 202 | |
| everything else | 25 | | |
| device total | **705** | 1664 | |

GEMM is 420 ms of 705 (60 %) against 216 of 488 on Xe2. The kernels in isolation on the
frame's own shapes (GPU timestamps, five back to back):

| shape | simd 8x16 | simd 16x32 | simd staged | portable 16x32 |
| --- | --- | --- | --- | --- |
| 98304x64x32 | 908 GFLOP/s | 543 | 592 | 581 |
| 98304x128x64 | 1212 | 914 | 758 | 731 |
| 64x64x32 x1536, Bᵀ | 788 | 784 | 544 | 411 |
| 6144x512x128 | — | — | 1115-1274 | 614 |
| 24576x256x256 | — | — | 1271 | 926 |

Two readings. The matrix path is worth 1.3-2x over the multiply-add kernels on the deep
shapes and not much on the shallow ones, which are bandwidth: 98304x32x32 runs at 90 GB/s
on the 8x16 kernel. And on this GPU the **8x16 kernel beats the 16x32 one** on the wide-M
shapes (908 against 543), the opposite of Xe2, where the register block was the win
(`phase21`); the runtime still picks 16x32 where it fits, and `XMX_TILE_M=1000000` is the
knob to re-measure that with. Not tuned; nothing here is tuned. The point of this phase is
the port, and the port is at parity-plus with MoltenVK from a straight transcription.

## How to run it

```sh
make                                    # Darwin: adds work/libmetalmx.dylib and work/nr_shaders.metallib
make test-metal                         # the GPU tests on libmetalmx; `make test` runs it on Darwin
cmake -S . -B build && cmake --build build && ctest --test-dir build -R metal_
NR_GPU_BACKEND=metal build/nr_frame IN.png OUT.png -v
NR_GPU_BACKEND=metal python3 src/ref/nr_frame.py IN.png OUT.png --resident
NR_GPU_BACKEND=metal python3 src/bench/frame_profile.py
```

`XMX_PORTABLE=1`, `XMX_STAGING=1`, `XMX_SPECIALIZE`, `XMX_TILE_*`, `XMX_STAGE_K` and the
`XMX_*_SPV` overrides all mean what they mean on libxmx. `XMX_METALLIB=` is new.

## libdlssnr on Apple is the Metal one

The static archive a host links (`libdlssnr`, VBA-M's way in) is built from
`libmetalmx.m` on Apple: `NR_STATIC_XMX` as before, `NR_STATIC_METAL` beside it, the
metallib compiled in, `-framework Metal -framework Foundation` as its public link interface
and **no Vulkan symbol anywhere in it** (`nm` finds none). `test_dlssnr` links the archive
alone — no MoltenVK, where the Vulkan archive needed the test to bring one — and its head is
bit-identical to the Python resident path on libmetalmx (36 checks). `-DNR_BUILD_METAL=OFF`
gives the Vulkan archive on Apple as before. New in the frame API: `nr_frame_runtime()`,
`"metal"` or `"vulkan"`, known without opening a device, so a host can decide whether to
share a Vulkan one; `nr_frame_adopt_vulkan` on the Metal archive is refused with a message.

## What is not done

- **The layer is Vulkan.** `nr_layer.c` hooks `vkQueuePresentKHR` and hands frames to the
  daemon; the daemon's Python takes `NR_GPU_BACKEND=metal` like everything else, so a game
  under MoltenVK can already be served by libmetalmx — untested here, no game on this Mac.
  A Metal-side hook (a `MTLCommandQueue` swizzle, or a `CAMetalLayer` present) does not exist.
- **`xmx_adopt` is refused.** On Apple `libdlssnr` — the static archive VBA-M links —
  now carries libmetalmx instead of libxmx (below), so a host there must not call
  `nr_frame_adopt_vulkan`; `nr_frame_runtime()` tells it which runtime it has.
- **Profiling changes the frame.** Apple GPUs sample timestamps at encoder boundaries only,
  so a profiled pass is an encoder of its own and the profiled total (816 ms) is not the
  frame that runs (740). The per-pass numbers are still the pass's own GPU time.
- `gemm_runner` (the one-shot Vulkan runner behind `test_gemm.py`) and the `half_probe`
  bench shader have no Metal twin.
- MoltenVK's `frame_profile.py` run fails here on its own: MoltenVK cannot create an 8192-query
  timestamp pool (the same 32 KB limit) and falls back to "emulated" zeros. Pre-existing,
  not touched.
- Nothing measured on a Mac with a discrete or non-Apple GPU: `hasUnifiedMemory == NO` takes
  the private-storage path, `supportsFamily:MTLGPUFamilyApple7 == NO` the portable kernels,
  both written, neither run.

Also fixed in passing: `src/gpu/test_softmax_pack.py` had no `import sys` and failed on any
backend since the build-directory change (`phase70`).
