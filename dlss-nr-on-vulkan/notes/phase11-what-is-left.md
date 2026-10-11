# What actually runs on the GPU, and what is left
2026-09-09

*Corrected 2026-09-09 by `notes/phase13-torch-and-blas.md`: the CPU column below is
the netlib reference BLAS at ~3 GFLOP/s. Under OpenBLAS the CPU alone beats the XMX
path, which makes residency the precondition for the GPU rather than an optimisation.
The arithmetic ceiling is unaffected.*

## No — only the matrix multiplies are on the GPU

Measured, with `nr_xmx` reporting bytes as well as time:

| | total | GPU GEMM | CPU matmul | elementwise numpy | host<->device |
|---|---|---|---|---|---|
| 384x384 | 17.2 s | 4.91 s (29 %) | 0.02 s | **12.3 s (71 %)** | 2.56 GB / 1572 round trips |
| 1280x720 | 96.7 s | 20.95 s (22 %) | 0.15 s | **75.8 s (78 %)** | 16.46 GB / 5078 round trips |

Every GEMM is on XMX, and the CPU column is only the handful of batched attention
calls whose token count is not tile-aligned. Everything *between* the GEMMs is numpy on
the CPU: the E4M3 publishes, the quadratic gate, the bit-affine softmax, the
fragment-tree cosine normalise, the window partition and reverse, the residuals and
gates, the pooling, the nearest and learned upsamples, the decoder merges.

So the activations cross the host/device boundary **twice per GEMM** — written as
float16 on the way in, read as float32 on the way out — 5078 times for a 720p frame.

## The cost of that, in one number

The kernel measures **1348 GFLOP/s** (50 dispatches inside one submit). At that rate a
720p frame's 458.6 GFLOP is **340 ms**. The GPU actually spends 20.95 s, which is
**62x the kernel's own peak**. Almost none of that is the matrix units; it is the
per-call float32<->float16 conversion and the round trip, which exist only because the
graph between the GEMMs lives on the host.

That is what a compute-shader port buys, and it is the reason to do one: not any single
kernel getting faster, but 16.46 GB of traffic per frame going to nearly zero.

## The honest performance ceiling on this hardware

Even with a perfect port — every operator resident, no host traffic — the arithmetic
alone sets the floor:

| | GFLOP | floor at 1348 GFLOP/s | that is |
|---|---|---|---|
| 384x384 | 78.9 | 59 ms | **17 fps** |
| 1280x720 | 458.6 | 340 ms | **2.9 fps** |

60 fps at 720p would need **27.5 TFLOP/s**, about 20x this iGPU's measured peak. So
**full-frame real-time DLSS-NR is not reachable on Lunar Lake**, and no amount of
engineering changes that — it is the model's arithmetic against the hardware.

What *is* reachable, and worth aiming at:

- **Photo mode and stills** — already working today.
- **Offline video** — a full port at ~1-3 s a frame makes a clip practical.
- **A region rather than a frame.** A 384x384 face crop is 17 fps at the floor. The
  model is trained on skin, hair and subsurface scattering, so a face-only pass is
  where the effect is anyway.
- **A lower internal resolution.** The extent floor is 320 and the step is 64; running
  at 640x384 for a 720p output is ~122 GFLOP, 90 ms, 11 fps at the floor.

This does not contradict the brief — notes/CLAUDE.md says playable framerates are explicitly
not a goal — but it does mean "wire into a real game" means photo mode or a cutscene,
not a live pass at 60 fps.

## What is left, in order

**1. GPU residency.** The perf work, and the only thing that moves the 62x. Roughly
eight to ten compute shaders — E4M3 publish, quadratic gate, cosine normalise + publish,
the bit-affine softmax, window partition/reverse, residual and gate, average pool,
nearest and learned upsample, decoder merge — plus, and this is the actual point, a
buffer pool so activations never come back to the host. Expect 720p to land somewhere
around 1-3 s.

**2. The temporal path.** The correctness work, and what "run it in a game" really
means: we currently implement the *first-frame* branch only. A sequence needs motion
vectors reprojecting the previous output into channels 7-9, the sign-encoded validity in
12-14, the recovered five-tap Catmull-Rom history filter, the sigmoid blend on the
head's fourth channel capped by `blend_scale` (0.73974609375), and consecutive-frame
reset semantics. MLX-DLSS has all of it in `python/mlxdlss/temporal.py` — **376 lines,
pure numpy, no torch** — so it ports the same way `model.py` did.

**3. HDR.** Games are linear-HDR internally; we assume sRGB in [0, 1].
`NeuralRenderingDisplayCodec` (375 lines of Swift) is the recovered contract: keep the
untouched source, make the model's sRGB proxy with a highlight soft knee, then fold the
result back through luminance-ratio composition rather than an unstable inverse curve.

**4. The game hook.** Phase 5, untouched. OptiScaler-style interception of
`NVSDK_NGX_D3D12_CreateFeature` / `EvaluateFeature`, and under Proton a DX12 -> VKD3D ->
Vulkan path. Our compute side is already Vulkan, so sharing images with VKD3D-Proton
through external memory is the plausible route, but nothing has been attempted.

**5. Not reproducible at all: Model A / B / C.** The shipped weights prove only slot 0.
See `notes/phase10-controls.md`.
