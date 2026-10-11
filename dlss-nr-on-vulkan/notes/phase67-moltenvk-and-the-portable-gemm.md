# Phase 67 — MoltenVK, an Apple M3, and the GEMM without the matrix (2026-09-19)

## The question

Can this run where there is no `VK_KHR_cooperative_matrix` at all? The owner's other machine
is an Apple M3 under macOS with MoltenVK 1.4.2 (Vulkan 1.4.357 reported, 154 device
extensions), and the extension is not among them. Every GEMM this project has was written
against it — five shaders, three of them selected per dispatch by the runtime — and
SPIRV-Cross, which MoltenVK uses to reach Metal, cannot translate the matrix opcodes:

```
SPIRV-Cross threw an exception: Currently no block to insert opcode.
```

That is the failure, measured on `gemm_resident.spv` with the SPIRV-Cross installed here.
The other shaders — `resident.comp`, `attention.comp`, `history.comp` — translate cleanly.

Implementing the extension *inside* MoltenVK was not attempted. It would be a compiler
project: teach SPIRV-Cross to lower `OpCooperativeMatrix*` onto Metal's `simdgroup_matrix`
(8x8 only, so an 8x16x16 tile is four of them and a K step is two), with `thread_elements()`
for the per-element epilogue this project runs on the accumulator. That is where a native
Apple path would go, and nothing here stands in its way; but it is weeks of C++ against
someone else's repository, and the port needed to *run* first.

## What was done

**A second GEMM kernel with the same contract**, `src/gpu/gemm_portable.comp`: the push
constants, the operation flags (transposed B, epilogue, half output), the row strides and
element offsets, the batch on `gl_WorkGroupID.z`, the 8x16 block per 32-lane workgroup and
the 16x32 block of `-DRM=2 -DRN=2` — all as `gemm_resident.comp` has them, so the runtime
dispatches it with the geometry it already uses. Each lane owns one row and four
consecutive columns of the tile, promotes FP16 operands to FP32 and accumulates with
ordinary multiply-adds; the publish runs on the result and it stores straight to global,
as one 128-bit or 64-bit write where aligned. `gemm_portable_desc.comp` is the same for
the descriptor-bound benchmark path, `-DBATCHED` for the batched one. The staged kernel
(64x32, four subgroups) has no portable twin: its geometry exists to feed matrix units, so
the runtime never selects it without them.

**The runtime asks the device.** `xmx_open()` — split out of `xmx_init` — creates the
instance and device, enables `VK_KHR_cooperative_matrix` only when the device lists it, and
`VK_KHR_portability_subset` when it does (the spec requires that). `xmx_coopmat()` and
`xmx_portable()` report the answer, `xmx_path()` puts it in one line for the daemon's log,
and the Python side picks the SPIR-V from it. `XMX_PORTABLE=1` forces the portable kernels
on a device that has the matrix ones, which is how the Xe2 machine tests this path.

**macOS in the build.** The Makefile detects Darwin and looks for headers and libraries
under vcpkg, `/usr/local` and Homebrew (`VK_PREFIX` overrides). `libxmx.so` and
`gemm_runner` link **MoltenVK directly** — it exports the whole Vulkan API, so the compute
path needs no loader, no ICD manifest and no portability enumeration. The layer and its
tests go through the loader by definition, and there the loader finds no driver on its own:
`make` writes `work/MoltenVK_icd.json`, `nr_paths.loader_environment()` points
`VK_DRIVER_FILES` at it when nothing else does, and the C tests enable
`VK_KHR_portability_enumeration` under `__APPLE__`, because a loader hides a portability
driver until asked. `nr_layer.c`'s `VK_USE_PLATFORM_XLIB_KHR` is now Linux-only; it pulled
in X11 headers macOS does not have and the layer never used a symbol from them.

**One correctness trap, and it is the finding of the phase.** Metal compiles shaders with
fast math unless told otherwise, and MoltenVK passes that through. With it on, the gate
epilogue came out **3e-03** off and every E4M3 publish downstream by a quantum
(`test_epilogue`: `gate` max|d| 2.96e-03, `gate + e4m3` 0.25), and the QKV fusion and
softmax packing checks failed on the last half bit. The cause is the one `publish.glsl`
warns about: the gate's multiply and add contract into an FMA and the half rounding between
them is skipped. `MVK_CONFIG_FAST_MATH_ENABLED=0`, set by `xmx.py` before the library loads,
and every check is exact — the same zeros the Xe2 machine prints. **This is not optional on
Metal.** A frame rendered with fast math on would look right and match nothing.

**Python 3.9.** macOS ships it, and MLX-DLSS's `motion_quality.py` writes `np.ndarray | None`
in a dataclass body, which 3.9 evaluates and cannot. `nr_frame.load_mlx_numpy_modules` now
compiles upstream's modules under postponed annotations on interpreters older than 3.10 —
annotations become strings and nothing else changes — so the temporal path runs here too.

## What was measured

On the M3, through MoltenVK, `make test` is **green: 117 checks**, the five skips all for
the weights, which are NVIDIA's and are not on this machine. That includes:

- `test_gemm.py` against numpy: worst relative error **8.3e-07** over six shapes, five of
  six bitwise equal to numpy's float32 GEMM;
- `test_portable.py` (new): every epilogue wide and narrow, transposed B, three batch
  sizes, a strided slice with offsets on all three operands, the tiled shape, and both
  descriptor paths — all exact, on the M3, and on the same machine with `XMX_PORTABLE=1`;
- `test_epilogue`, `test_specialization` (41 pipelines), `test_softmax_pack`,
  `test_qkv_fusion`, `test_resident`'s elementwise and permutation halves;
- `test_present.py` — the layer's copy out and back through a headless swapchain, both sync
  modes, through the real loader with MoltenVK behind it.

Throughput of the portable kernel on the M3 (`src/gpu/bench.py`, one dispatch per shape,
no host copies in the timing):

| shape | GFLOP/s |
| --- | --- |
| 512x512 K=512 | 477 |
| 1024x1024 K=1024 | 543 |
| 2048x2048 K=512 | 559 |
| 4096x1024 K=1024 | 569 |
| 8192x512 K=512 | 571 |

Against the matrix kernel's 1348 on Xe2 for the same shapes (`phase8`) and 3828 at its
peak (`phase33`). It is a plain FMA kernel with no operand staging; that is the expected
distance, not a surprise, and it reaches no matrix unit anywhere.

## What was not measured

- **A frame.** No DLL and no weights on this machine, so no forward pass and no picture.
  Everything above is the contract, not the graph; the graph is the same code, but the
  claim that a 720p frame comes out on the M3 has not been made and should not be read here.
- **A frame time.** From the kernel rate, an estimate only: the Xe2 frame spends 216 ms of
  488 in GEMM at 720p (`phase45`), at roughly three times this kernel's rate, so something
  around a second a frame is the order to expect. Measure it before quoting it.
- **The layer in a game on macOS.** The layer attaches through the loader and the test
  passes; no game was run.

## Traps

- **`-lvulkan` on this Mac is two different things.** `/usr/local/lib/libvulkan.dylib` here
  is an x86_64-only 1.4.309 loader the linker skips on arm64; vcpkg's is a real 1.4.357
  loader. The first build of `test_present` linked MoltenVK instead and every layer test
  failed with `VK_ERROR_LAYER_NOT_PRESENT` — MoltenVK knows no layers — while the loader's
  debug output stayed silent because no loader was in the process. `otool -L` first.
- **`-9` from `vkCreateInstance` through the loader is the missing ICD or the missing
  portability flag**, and they look identical. The manifest fixes the first; the
  `__APPLE__` block in each test fixes the second.
- **`timeout` does not exist on macOS**, and this shell's `sed -i ''` is GNU sed reading
  `''` as its script. Neither is a project matter, both cost a run.
