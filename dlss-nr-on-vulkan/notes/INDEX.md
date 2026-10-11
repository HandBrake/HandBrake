# Index of the notes

Ninety-four files, sixty-five phases. This is what each one settles, so a reader arriving
cold can go straight to the answer instead of the archaeology.

**Read `notes/HANDOFF.md` first** — it carries the current state, the standing conclusions and
the traps, and **`docs/ARCHITECTURE.md`** for the recovered network as a specification
rather than as archaeology. This index is for going deeper on one question.

Notes are the record of what was *measured*, including measurements that turned out
wrong. Where a note is superseded it says so at the top and is kept anyway: several of
this project's worst hours went into re-deriving something a withdrawn note already
disproved.

## Start here

| note | what it settles |
| --- | --- |
| `phase45-frame-profile.md` | where every millisecond of a frame goes, per pass, from GPU timestamps |
| `phase47-live-mode.md` | the live mode, 10.6 fps at 512x288, and the bottleneck leaving the graph |
| `phase38-there-was-no-bug.md` | the "driver bug" that shaped three phases does not exist |
| `phase9-numerics.md` | why bit-identical agreement with a CPU reference is impossible here |
| `opendlss-reference.md` | a second implementation that claims the vendor's own arithmetic, runnable here: where ours differs from it by specification; the decoder skips a block early and six more places MLX-DLSS's graph computed something else — a raw value into four GEMMs, block 30's pool, the wide blocks' and the ViT's publishes, the ViT's own attention — found step by step and fixed; our head against its now RGB corr 0.98-0.997, 0.5-1.9 levels of 255, and what is left is arithmetic; and the padded field — every field with both sides a multiple of 256 draws a 25-30 % weaker pass, 1280x720 ran on one, and the vendor's rule steps around it |
| `reviewing.md` | how to run `/ultrareview` on this repo without wasting a run |
| `reproduce.md` | how to run the resident path from a clean checkout |

## The binary, the weights, the graph

| note | what it settles |
| --- | --- |
| `phase0-acquire.md` | provenance of the DLL, cryptographically |
| `phase1-binary-map.md` | 89 % of the file is weights; no CUDA runtime imports |
| `phase2-graph-rtti.md` | the layer taxonomy, out of intact MSVC RTTI |
| `phase3-weight-format.md` | FP16, not FP8 — the figure that was wrong for days |
| `phase3-ptx-unlock.md` | the `.data` containers are Zstandard and decompress to PTX |
| `phase3-architecture.md` | five encoder/decoder stages, ViT-1D bottleneck, 32 per head |
| `phase3-execution-order.md` | our decode is byte-identical to a capture from real RTX hardware |
| `phase6-mlx-dlss-unpack.md` | **the container is packed backend payloads, not dense FP16** |
| `phase3-block-layout.md`, `-block-internals.md` | the ordered layout inside each block family |
| `phase3-bias-region.md` | the 128C region is the attention bias, in a 12-bit permutation |
| `phase5-softmax-found.md` | the attention is a softmax, hand-rolled in `f16x2` |
| `phase5-attn-scale-fp32.md` | `attn_scale` is FP32 per head |
| `phase5-*` (the rest) | stem, output head, channel order, resampling, I/O structs, narrow blocks |
| `phase5-no-softmax.md` | **withdrawn** — kept so nobody re-derives it |

## Running it

| note | what it settles |
| --- | --- |
| `hw-coopmat.md` | six cooperative-matrix configs, all M=8 N=16, subgroup scope |
| `phase71-intel-windows-driver.md` | the first Windows session, on Intel's own driver: the fp16 configuration is there (four configs), the compiler folds `packHalf2x16`'s round trip so `half_round` is chosen per driver, the unmerged window attention hangs the engine, the picture is 47-49 dB from Linux's at 1.2-1.4x the graph time, and the two part at the first GEMM because Mesa flushes float16 subnormals by default where Intel's driver keeps them |
| `phase4-subnormal-flush.md` | subnormal FP16 operands flushed on XMX (premise later corrected; the flush is Mesa's default mode, not the hardware's: `phase71`) |
| `phase4-accumulation-choice.md` | NVIDIA accumulates in FP16; we use FP32 and are more accurate |
| `phase7-first-render.md` | the first real frame, with its two adversarial controls |
| `phase8-xmx-graph.md` | the whole graph on XMX, and the graph is chaotic |
| `phase15-residency.md` | activations never return to the host |
| `phase16-hdr.md` | the display codec, so an HDR frame survives the round trip |
| `phase12-temporal.md` | the temporal gate discriminates: 0.705 right, 0.032 wrong motion |
| `phase70-post-process.md` | what the vendor does after the network, read from the DLL: `cg2r_post_process_kernel` is a whole grading chain, the styles set three of its values (natural exposure -0.1, contrast -0.25, saturation -0.1; cinematic saturation -0.15) scaled by the tone, and the history is the prediction before any of it, in half |
| `phase28-frame-replay.md` | one submission a frame, commands reused |
| `phase27-pipeline-specialization.md` | specialize before believing a register ceiling |
| `phase32-scratch-and-qk.md` | the scratch arena: 720p from 5041 to 2303 MiB |
| `improve-large-grf.md` | Xe2's 256-register mode, reached with a three-place Mesa patch: correct at every size, and 13-26 % slower in a frame with kernels tuned for full occupancy; what a win would take |

## Performance, and the levers that are closed

| note | what it settles |
| --- | --- |
| `phase20-machine-limits.md` | 70-91 GB/s of 136.5, the clock ceiling held, 64 XMX engines |
| `phase26-the-register-ceiling.md` | every engine busy, each ~90 % idle; the register file is the wall |
| `phase45-frame-profile.md` | per-pass timings; everything that moves data is at the memory ceiling — which did not make the graph finished |
| `improve-fusions.md` | residuals, attention and its head merge, the glue, the narrow feed-forward folded into fewer passes; window attention in 2 KB; the staged loader's loads issued together; what was measured and dropped |
| `improve-shared-memory.md` | a Mesa quirk — the core's shared-memory partition sized from the declared bytes, each workgroup's share rounded up — that makes some smaller declarations slower and costs this frame nothing; the 128 KB cap that had the staged GEMM on half its threads, 10 % of a frame; and why window attention and the fused feed-forward, with no L1, fetched the same operands once per subgroup from L2 — and ran at 2x and 1.5x once they shared them |
| `improve-b.md` | the bottleneck's small-M GEMMs: why a direct kernel wins only on cached weights, why the weights' bytes are not the limit, that 320x320 has 64 tokens; the staged loop's 16-bit B loads and the bit-identical operand swap that was slower; other block shapes; the 32-row block that was kept |
| `improve-portable-precise.md` | the portable path (MoltenVK, phones) a third faster for one `precise`: glslang spreads the qualifier through a shared array into the whole K loop, SPIRV-Cross spells it `[[clang::optnone]]`, the 16x32 build ran at a third of its speed on every shape and the routing was measured against it; the profiler's timestamp pool on MoltenVK; what was tried and dropped |
| `improve-qkv-epilogue.md` | Q/K normalised in the QKV projection's own epilogue, 22 %; all fusions together 38 %; why joint QKV was slower; shared memory comes in powers of two; measure with empty swap |
| `phase21`, `phase22`, `phase23`, `phase31`, `phase33` | tiling, staging, integer weights, the accumulator, OpenCL — all measured, all closed |
| `phase25-the-frame-rate-wall.md` | `17 ms + 488 ms per megapixel`, and what that forbids — `9 + 205` since the fusions and the shared-memory fix |
| `phase51-output-resolution-costs.md` | what costs the output extent rather than the network's, and a profile that was measuring swap |
| `phase50-what-the-model-computes-in.md` | 36 % of the shipped model's mma is already FP8; what FP4 would and would not change |
| `phase37-neural-upstream.md` | half the extent is 3x faster and keeps 62 % of the high band |
| `phase46-cpu-share.md` | four busy E-cores cost the GPU 7 % for a theoretical +2 % |
| `phase14-npu-and-rounding.md` | the NPU is not worth using; the rounding point was |
| `phase13-torch-and-blas.md` | the system numpy is netlib; every early CPU baseline was 67x too slow |

## In a game

| note | what it settles |
| --- | --- |
| `phase17-integration-landscape.md` | why a Vulkan layer and not NGX |
| `phase19-photo-mode.md` | the loop closed on `vkcube` |
| `phase24-a-real-game.md` | the layer loads inside a Wine prefix |
| `phase34-doa5.md` | a face, from a real game |
| `phase42-doa5-fight-scene.md` | the pass measured on a live fight; the tone shift that fools the eye |
| `phase35-what-the-operator-does.md` | the interface-mask detector, on three real frames |
| `phase43-mask-mottling.md` | the mask making a worse artefact than it prevents, and both fixes |
| `phase44-profile-tradeoff.md` | profiles trade speculars for skin texture; one knob runs backwards |
| `phase41-doa6-and-vkd3d.md` | the layer under VKD3D-Proton; why DOA6LR will not start |
| `phase47-live-mode.md` | live mode, the control tool, 10.6 fps |
| `phase48-feature-inputs.md` | all sixteen feature channels, and two costs that were pure waste |
| `phase52-live-in-a-game.md` | live neural rendering inside a running game, measured |
| `phase53-why-it-flickers.md` | why a static pixel moves 3.3 levels: the network is global |
| `phase54-flicker-fix.md` | the temporal path in live mode; 3.7x less flicker for 3.7 % of the frame |
| `phase55-a-switch-on-a-key.md` | one key over a fullscreen game, and the compositor crash from binding it the wrong way |
| `phase56-the-panel-and-the-manual.md` | every knob on one screen, one table behind the tools and the README, three bugs in a terminal's input path |
| `phase57-native-host-passes.md` | the passes around the network in C, taken from the parallel tree; host 74 -> 28 ms, byte-identical |
| `phase58-before-publishing.md` | the licence, what a repository carries with it, and the four things the check found |
| `phase59-tekken7.md` | 64-bit D3D11 through DXVK, 10.5 fps at 640x360, and the highest history gate yet |
| `phase60-performance-mode.md` | the laptop's performance mode buys nothing: the GPU is frequency-capped at 1950 MHz, not power-capped |
| `phase61-the-parameter-count.md` | **145 755 123 parameters, not 73 841 889** — the published count was the byte count halved; GQA and 27 % subnormals fall with it |
| `phase62-mortal-kombat-1.md` | the first D3D12 title with a picture; full scale does not fit in memory; on MK1 the pass changes colour and removes fine detail rather than adding it |
| `phase63-a-discrete-gpu.md` | the first report from other hardware: an Arc B580 runs 50x slower than this iGPU because `memtype()` preferred host-cached memory, which on a discrete card is system RAM; and a benchmark that never checked its calls printed 495 TFLOP/s for a dispatch that failed |
| `phase64-auditing-the-readme.md` | seven wrong claims on the published page, including a pinned commit that never existed and a frame-time table a third too slow; the rates table and the file references are now generated and checked |
| `phase65-the-discrete-memory-path.md` | the graph's buffers no longer have to be addressable by the host, so a card without resizable BAR keeps its operands in its own memory; forced on this iGPU by `XMX_STAGING=1`, and the frame is bit-identical |
| `improve-present-fences.md` | **how the layer synchronises now**: one path — the present's own semaphores consumed once, copies finished on private fences, no queue drained — and the real MK1 check |
| `improve-doom-vulkan-20261005.md` | DOOM's own Vulkan renderer through Proton: 390 gameplay frames accepted, 20–23 FPS at 1280x720 with NR versus 59–60 without; process evidence distinguishes this from a native Linux executable |
| `improve-windows-integration-20261005.md` | Windows updates through `e616afc` integrated into improve: allocator and pipe overhead, x86 layer, packed staged loads, first-test guidance and pipe-handle cleanup; 44 Linux tests passed, with a Windows rerun still required |
| `improve-release-20261005.md` | PR #5 integrated with the Windows runtime: one checked release assembler, allocator included, portable Linux host passes, first-test settings and launchers; twelve release tests and an actual Linux package/model smoke pass |
| `phase69-the-stand-that-discriminates.md` | the present test rebuilt so it can fail — frames in flight, a semaphore per image, a real stall, a present queue of another family — and what it found first: the old default waited on the presenting queue, not the drawing one. Its fix never shipped; `improve-present-fences.md` replaced both paths |
| `phase68-what-validation-settles.md` | the present test under the Khronos validation layer: both old sync modes clean, two real bugs in the harness fixed, and a negative control showing the test could not yet prove the wait load-bearing |
| `phase66-the-present-has-a-test.md` | the present's own semaphores instead of a queue idle, behind `NR_LAYER_SYNC`; a headless swapchain makes the layer's copy out and back testable without a game, and the first run found `present_now` calling itself |
| `phase67-moltenvk-and-the-portable-gemm.md` | the build and the GEMM contract on an Apple M3 through MoltenVK, which has no `VK_KHR_cooperative_matrix`: a portable multiply-add kernel behind the same dispatches, the runtime asking the device, `XMX_PORTABLE=1` to test it on Xe2, and Metal's fast math moving every vendor rounding point until `MVK_CONFIG_FAST_MATH_ENABLED=0` |
| `phase68-nr-frame-in-c.md` | `nr_frame.py` as a C library — the weights, the graph recorded call for call against libxmx, the frame around it — with the head bit-identical to the Python path; a first real 720p frame on an Apple M3 at 0.90 s; where C and NumPy differ by a last bit and why; the committed `work/` tree that now fails `publish_check` |
| `phase69-cmake-three-platforms.md` | `CMakeLists.txt` for Linux, macOS and Windows — the Makefile's targets once, outputs still in `work/`, MoltenVK and libpng found, ctest wired — and `nr_portable.h`, whose software half conversion is exhaustively bit-exact against `_Float16`; Windows cross-built and linked from macOS with MinGW-w64 (three fixes: the native flag, a duplicate clock, a `BIND` that never compiled), not yet run; the layer stays POSIX |
| `phase70-cmake-build-directory.md` | the CMake build lands in its own directory rather than `work/`: `src/nr_build.py` as the one place the Python asks where a build put the libraries, executables and SPIR-V (`NR_BUILD_DIR`, else the newest of `work/` and `build*/`), `NR_OUTPUT_DIR` in CMake, ctest told where its build is, the C command finding the weights from `build/`; the two-builds trap |
| `phase71-embedded-weights.md` | the logical weights compiled into `libnr_frame` by the CMake build: `bin2c` (`src/tools/bin2c.c`) over 8 MB slices cut by `src/tools/slice.c`, one translation unit each under a two-job pool because a compiler holds a byte-array initializer at ~90 bytes per byte; the safetensors reader given a source abstraction so `nr_frame_open(NULL)` reads the slices; the command, the C test and `NativeFrame()` default to them; then the fifteen SPIR-V modules compiled into libxmx and gemm_runner the same way, loaded by name with a file path as the override and the embedded module as the fallback |
| `phase72-android.md` | the tree cross-builds for Android with the NDK toolchain, standalone and inside VBA-M's Qt APK: what configured on its own, the one failure (`VK_USE_PLATFORM_XLIB_KHR` under `__linux__`), the `-ffast-math` VBA-M hands every target and why the exact flags still win (verified on the NDK's clang), the host-compiler fallback for `bin2c`/`slice`, `/data/local/tmp`; built for arm64-v8a and armeabi-v7a, run on nothing; and riscv64, where Android's baseline has V but neither Zfh nor Zvfh, so `_Float16` made the vectorizer emit scalable half vectors the backend aborts on — the software half conversion is taken there instead, byte-identity re-verified |
| `phase73-mali.md` | the first device: a Mali-G57 MC2 phone aborted inside `vkCreateComputePipelines` on `attention.spv` (`LLVM ERROR: Cannot select: intrinsic %llvm.bifrost.2556`), because the row pass fenced its shared-memory staging with a *subgroup*-scope barrier on a 32-wide workgroup — and Mali's subgroup is 16, so the barrier was wrong as well as uncompilable; `barrier()` fixes it and all 34 C checks pass on the phone at 6.7 s a pass; how to reproduce with `test_dlssnr` under `adb shell` and `XMX_ROW_SPV`, and the device's Vulkan properties |
| `phase74-metal.md` | `libmetalmx`, the compute runtime on Metal directly (Apple only): every `xmx_*` entry point of libxmx in `src/gpu/libmetalmx.m`, the fourteen kernels rewritten in MSL under `src/gpu/metal/` and compiled by the Metal compiler into one metallib that bin2c puts inside the library, `NR_GPU_BACKEND=metal` to select it; `simdgroup_matrix` as the matrix path (half in, float accumulate, exact on the GEMM contract); the three things the port had to get right — no fast math and no contraction, a fence between encoders on untracked buffers (without it the whole graph read 0.25 while every block passed), and never a per-element loop over `thread_elements()` (27x) — and a 720p frame in 735-746 ms against 938-948 through MoltenVK on the same M3 |
| `phase75-d3d12.md` | `libd3dmx`, the compute runtime on Direct3D 12 (Windows only, cross-built; one run so far, whose graph had not finished after 60 s with the device alive — hence no fence timeout by default, `XMX_D3D12_TIMEOUT`, `XMX_D3D12_STEP=1` per-pass timing, the debug layer drained to stderr, and the per-list state-tracking fix): every `xmx_*` entry point in C over `lpVtbl` with `d3d12.dll` loaded by name, the kernels in HLSL under `src/gpu/d3d12/` as nine DXIL modules from dxc embedded by bin2c, `NR_GPU_BACKEND=d3d12`; what the port had to get right — an operand as a root UAV plus a byte offset because HLSL has no pointers, dispatches split at 65535 groups with the group base in `spare`, `f32tof16` on every narrow store, `precise` and `Round_ne` confirmed in the disassembly, resource states from COMMON decay, unsigned DXIL from a dxc without dxil.dll; no matrix path (HLSL has none shipped) and no specialization; `nr_frame_adopt_d3d12`, `NR_DLSSNR_D3D12` |
| `phase30-control-atlas.md` | what each vendor slider does, and one that does nothing |

## Reviews

| note | what it settles |
| --- | --- |
| `phase39-layer-review.md` | eight findings in `src/layer`; one was a feature that never worked |
| `phase40-gpu-review.md` | four nits in `src/gpu`, and why that area was already right |
| `phase49-from-the-other-tree.md` | seven layer guards and one real bug taken from `ProjectsCodex` |

## Withdrawn or superseded, kept deliberately

`phase5-no-softmax.md`, `phase18-fusion.md` and `phase36-the-bug-is-narrower.md` (both
superseded by `phase38`), the FP8 reading in the early part of `phase3-weight-format.md`,
the dense-FP16 decode behind `phase4-subnormal-flush.md`, and the shortcut-installing
recipe at the top of `phase55-a-switch-on-a-key.md`. Each says so at the top.
**A withdrawn finding lives on wherever it was written down** — `phase40` found five
places in the *code* still asserting the phase-18 bug as fact, months after the note
retired it.
