# DLSS 5 Neural Rendering on Intel Xe2 (Lunar Lake)

## What this project is

Reimplement the inference pass of NVIDIA's DLSS 5 Neural Rendering (DLSS-NR) so it
runs on an Intel Xe2 integrated GPU.

As of 2026-09-07 no public port exists for **any** Intel GPU. The only non-NVIDIA
implementation is `danielblnc/DLSS-NR-on-AMD` for RDNA4, which works because RDNA4
has native FP8 E4M3 matrix hardware. That approach does not transfer here — see
"No FP8" below.

**Success = one frame goes in, a neurally-rendered frame comes out, and it matches
a CPU reference.** Playable framerates are explicitly *not* a goal. Do not propose
optimisations that trade correctness for speed until Phase 3 is done.

> **DONE 2026-09-10, further than the line above set out.** A frame of **Dead or Alive
> 5** goes out of a running game, through the recovered 71-block graph on this iGPU, and
> back into the game's own swapchain — hair separating into strands, eyelashes
> resolving, skin picking up texture that is not in the input. 1280x720 in **~0.49 s**
> of graph time, correlation with the CPU reference **0.981311**, and every optimisation
> since has kept the output bit-identical.
>
> `notes/phase34-doa5.md` for the game, `phase7-first-render.md` for the first frame,
> `phase8-xmx-graph.md` for the XMX path.
>
> **Read `notes/HANDOFF.md` first**; it overrides this file, and large parts of what follows
> are the record of how the answers were reached rather than the answers.
> `notes/INDEX.md` maps every phase note to the question it settles.

---

## Hardware (already probed — do not re-probe, trust these values)

- Acer Aspire A14-52M, Intel Lunar Lake, Arc 140V-class iGPU (Xe2 / Battlemage-family)
- Arch Linux, Mesa ANV Vulkan driver, `intel-compute-runtime` for OpenCL
- **15 GiB total RAM, shared with the iGPU.** 7.6 GiB swap. There is no dedicated VRAM.
- OpenCL stack alive: `Platform #0: Intel(R) OpenCL Graphics / Device #0: Intel(R) Arc(TM) Graphics`

Vulkan capabilities, confirmed via `vulkaninfo`:

```
VK_KHR_cooperative_matrix                : extension revision 2
cooperativeMatrix                        = true
cooperativeMatrixRobustBufferAccess      = false
cooperativeMatrixSupportedStages         = SHADER_STAGE_COMPUTE_BIT   (compute only)
shaderBFloat16CooperativeMatrix          = true                        <-- primary path
VK_NV_cooperative_matrix2                : extension revision 1        (Mesa impl)
```

XMX matrix engines support: TF32, FP16, BF16, INT8, INT4, INT2.
**They do not support FP8.** Xe3 / Panther Lake does not add it either.

**Verified 2026-09-07** (`src/probe/coopmat_probe.c`, full table in
`notes/hw-coopmat.md`): Mesa exposes exactly **6** cooperative matrix configs, all
scope = subgroup, all **M=8 N=16** (K=16 float, K=32 int8) — fp16 and bf16, each
with either same-type or **fp32** accumulate, plus sint8/uint8 → int32.
`subgroupSize` = 32. TF32 and INT4/INT2 are *not* reachable from Vulkan even though
the DPAS hardware has TF32 (OpenCL advertises
`cl_intel_subgroup_matrix_multiply_accumulate_tf32`). `VK_NV_cooperative_matrix2` is
advertised but reports **zero** flexible-dimension configs.
The **primary path is config 1, `fp16 × fp16 → fp32`** — the shipped weights are FP16
(see below), so nothing is converted. The bf16 config is a fallback, not the plan.

---

## The weight format — what is live, and what was wrong twice

This section has been wrong **three** times, so it is worth stating carefully what survived.

> **Wrong the third time, corrected 2026-09-16** (`notes/phase61-the-parameter-count.md`).
> The model has **145 755 123 parameters**, not 73 841 889. The large matrices — 143.0 M of
> them — are stored as **FP8 E4M3, one byte each**; the small tensors are FP16 and
> `attn_scale` is FP32. That fits the 147.7 MB section to 0.5 %, where dense FP16 would need
> 291.5 MB. **73 841 889 is the section's bytes divided by two** — the dense-FP16 misreading
> this section already warned against, surviving in the count after it had been corrected
> in the encoding. The early "~148 M FP8" figure it dismissed was essentially right.
> Also withdrawn with it: GQA 4:1 (`qkv` is `(C, 3C)`, full MHA) and "27 % subnormals" —
> the real weights hold 7.

**Live, and load-bearing:**

- **There is no FP8 anywhere in our *compute* path.** The XMX units have no FP8, so the
  E4M3 matrices are decoded to FP16 at extraction and computed with from there. That decode
  *is* a dequantisation step — the weights are not "used as they arrive".
- **The primary path is FP16, not BF16.** Config 1 of the cooperative-matrix table is
  `fp16 × fp16 → fp32`, and the logical weights are FP16, so they reach the matrix units
  with **zero conversion**. **Do not convert to BF16** — it would throw away 3 of 10
  mantissa bits to reach a format the hardware likes no better, and phase 22 measured
  exactly that.
- **145 755 123 parameters.** On disk they take 147 MB because most are one byte; decoded to
  FP16 for compute they take about 292 MB.

**Superseded, and the trap to avoid:** the container does *not* hold plain dense FP16.
It holds **packed backend payloads** — permuted into `mma` fragment order, partly E4M3 —
and `data_len == 2 * n_elem` fixes the byte count, not the encoding. Reading those bytes
as dense FP16 gives values correlating **−0.02** with the true tensors, which is how
this project spent its first days measuring scaffolding. Use
`work/mlxw/dlssnr-logical.safetensors`: **649 named, shaped, logical tensors**, matching
MLX-DLSS's `weight_spec.json` exactly — 0 missing, 0 extra, 0 shape mismatches, so two
independent extractions of the same DLL agree. `notes/phase6-mlx-dlss-unpack.md`,
HANDOFF section 0.

**One nuance on the old debugging invariant.** It used to say "the weights are used
exactly as stored, so any divergence from the CPU reference is an implementation bug".
That is no longer safe: the graph is chaotic, ~100 E4M3 publishes with a 6.25 % quantum
stand between input and output, and numpy's own float32 GEMM carries more error than the
threshold below which perturbations vanish. Per-element agreement is not a property a
port can have. Judge on the composed image and the controls.
`notes/phase9-numerics.md`.

---

## The target binary

`nvngx_dlssnr.dll`, ~158 MB.

- ~~Officially shipped since NVIDIA driver 616.56 / 616.64 WHQL. Prefer the official
  driver copy over the leaked NBA 2K27 early-access build 310.8.0.0.~~
  **Both halves falsified 2026-09-07** — see `notes/phase0-acquire.md`. Driver 616.64
  was unpacked in full and does not contain it; the public `NVIDIA/DLSS` SDK is still
  at v310.7.0 (2026-06-23, pre-DLSS-5) and ships only `nvngx_dlss.dll`,
  `nvngx_dlssd.dll`, `nvngx_dlssg.dll`. There is **no official copy to prefer**: the
  NBA 2K27 early-access 310.8.0.0 build is currently the only one in existence.
  Acquisition stays with the owner, per the redistribution rule below.
- **Verified:** 89.1 % of the file is weights (`.rsrc`, 147 697 152 B), holding
  **145 755 123 parameters**, the large matrices as FP8 E4M3 — the "~148 M FP8" report was
  essentially right. (This line said 73 841 889 FP16 until 2026-09-16: that was the byte
  count halved, `notes/phase61`.) Decoded to FP16 they are ~292 MB. Trivial for this machine.
- Architecture, per NVIDIA's own research page (research.nvidia.com/labs/adlr/DLSS5/):
  a **one-step pixel-space diffusion model**, conditioned on the current rendered
  frame, engine motion vectors, carried temporal state, and artistic-direction values.
  Deterministic and temporally stable by design.
- Single-frame capture analysis on an RTX 2070 reported 174 CUDA kernels and 176
  compiled CUBIN modules, containing **Swin and ViT blocks, QKV projections**, and
  FP8 E4M3-specific kernels. Original binaries target `sm_120` (Blackwell) only.

Treat all of the above as *reported*, not verified. Verifying it is Phase 1's job.

### Useful reference implementations (read, do not depend on)

- `Dagherbou/OptiScaler_DLSSNR` — NVIDIA-only, but the cleanest example of how the
  NR pass is driven directly with no vendor integration. v0.2.0 as of 2026-09-06.
- `lisitskyaa/ComfyUI-DLSS5-NR` — runs the pass headless on a single image through a
  small native D3D12 bridge. Closest thing to the I/O contract we need to reproduce.
- `NIGos/dlss5-dx11-bridge` — documents the NGX call contract
  (`NVSDK_NGX_D3D12_CreateFeature` / `EvaluateFeature`).

---

## Hard constraints

1. **Memory.** 15 GiB shared, total, for the whole system. The NVIDIA-side OptiScaler
   mod reserves a fixed 13 GiB. We cannot.
   *Resolved differently than this expected:* **tiling was never needed.** The whole
   frame runs at once, and a shared scratch arena — blocks execute in sequence, so roles
   with disjoint lifetimes alias — brought 720p to **2.3 GiB** of device buffers and
   made 1920x1080 fit without swapping. `notes/phase32-scratch-and-qk.md`. Sizing each
   role by the buffers a recording actually touches then took 720p to **0.7 GiB** and
   1920x1088 from 2.9 to **1.2 GiB**, weights included (2026-09-26, HANDOFF). Do not
   retrofit tiling; there is nothing to retrofit it to.
2. **No NVIDIA GPU exists on this machine or anywhere accessible.** There is no way to
   produce reference activations from the original binary. Ground truth must come from
   our own CPU reference implementation (see Phase 3). *Partially mitigated
   2026-09-07:* our weight decode is byte-identical to a model arena captured from a
   running RTX 50 by `skchen17/dlssnr-amd-lab` (SHA-256 `A5513B18…BD4EE3E5`), so the
   **weights** are anchored to real hardware even though activations are not.
   See `notes/phase3-execution-order.md`.
3. Cooperative matrix is **compute-stage only**. Everything goes through compute
   shaders; no graphics-pipeline path.

---

## Roadmap

- **Phase 0 — Acquire. DONE 2026-09-07.** `ref/nvngx_dlssnr.dll`, 165 840 496 B,
  sha256 `e16bcf15e16e13f527491cdf7845b2fe6521a738d8f7c9c721866a8496e1fc8e`,
  version 310.8.0.0, mode 0444. **Not** from the driver — 616.64 was unpacked in full
  and does not contain it (it ships only the NGX runtime, which knows the feature and
  fetches the DLL out of band). Provenance is cryptographic: full Authenticode
  verification against `CN=NVIDIA Corporation` chaining to DigiCert Trusted Root G4.
  See `notes/phase0-acquire.md`; tooling in `src/tools/`.
- **Phase 1 — Confirm the weight blob. DONE 2026-09-07** — `notes/phase1-binary-map.md`.
  radare2 was never needed; `src/tools/pe_inspect.py` does the section map. Weights are
  in **`.rsrc`, 147 697 152 B = 89.1 % of the file, entropy 5.89**, confirming the
  reported ~89 %. Imports are only ADVAPI32/KERNEL32/USER32/VERSION — no cudart, cudnn,
  cublas, nvinfer or onnxruntime, so the graph is entirely inside this DLL.
  **There is no `.nv_fatbin` section** — that string does not occur in the file. The
  CUDA code is 15 fatbin containers (magic `BA55ED50`, first at 0xdf0e0) and 15 ELF
  cubins (first at 0x1f9220), all embedded in `.data`. `-arch sm_120` confirmed.
- **Phase 2 — Recover the graph. DONE 2026-09-07.** The written spec exists:
  `notes/MODEL-SPEC.txt`, regenerated by `src/tools/model_spec.py`, accounts for
  **all 73 841 889 container elements with zero remainder** *(storage, not parameters —
  the model has 145 755 123, and the GQA below is withdrawn: `notes/phase61`)*. Grouped-query attention
  (QKV = 1.5C^2 = Q(C^2) + K(C^2/4) + V(C^2/4), 4:1), attention bias = heads x 64 x 64
  = 128C, transition blocks = standard + one extra C^2. Original notes: `notes/phase2-graph-rtti.md`.
  The CUDA toolkit turned out to be unnecessary: the DLL is MSVC-built with **RTTI
  intact**, and 35 mangled class names in the `HNetCpp` namespace give the layer
  taxonomy outright — Swin blocks at 1/2/4/8/16 heads (`CCSplitSwin16H*`,
  `CCTinlayoutFusedSwin{1,2,4,8}HLayer`), ViT blocks in 2D and 1D flavours
  (`CCVit*Layer`, `CCVit1D*Layer`) with QKV / attention / projection / FFN
  expand+contract, and `CCDecInputUpsampleLayer`. Every one is a GEMM-family op, so all
  of it maps onto our single 8×16×16 bf16→fp32 shape. **Still missing:** layer ordering,
  widths, depths, Swin window size/shift, and where the diffusion conditioning enters.
  Recover from `CCNetwork` construction code in `.text` and from tensor shapes in the
  `.rsrc` blob; carve the 15 fatbins out of `.data` by magic if kernel-level detail is
  needed.
- **Phase 3 — CPU reference. DONE 2026-09-09** — `src/ref/nr_model.py`,
  `src/ref/nr_frame.py`, `notes/phase7-first-render.md`. Not by finishing the recovery
  below, but by porting the graph `iamwavecut/MLX-DLSS` recovered from vendor captures
  (PyTorch -> numpy; Apache-2.0) onto the correctly-decoded logical weights. Our 649
  tensors match their `weight_spec.json` exactly — 0 missing, 0 extra, 0 shape
  mismatches — so the two independent extractions of this DLL agree. Regression:
  `python3 src/ref/test_nr_model.py`. *(Historical, superseded: The `.data` containers are **Zstandard**,
  not a proprietary codec — they decompress to PTX source for all 231 kernels, which
  carries the full layer configuration in mangled template parameters. See
  `notes/phase3-ptx-unlock.md`; this supersedes guesswork about shapes.)* The weight container is fully
  decoded — `notes/phase3-weight-format.md`, `src/tools/hnet_weights.py`: 153 tensors,
  73 841 889 FP16 parameters *(in fact bytes halved — `notes/phase61`)*, walked to EOF exactly.
  **The dequantisation step does
  not exist** — weights are used as stored. Block sizes describe a symmetric U-Net of
  71 blocks with an 8-block bottleneck. Remaining: recover 2-D tensor shapes and the
  block→layer-class mapping from `CCNetwork` construction code in `.text`, then
  rebuild the graph and run one frame. Minutes per frame is acceptable. **This is the
  ground truth for everything after.** First "it's alive" milestone.
  **First numerics are running** — `src/ref/hnet_ref.py`, `notes/phase3-first-numerics.md`:
  41 of 153 tensors bound to kernel shapes, the 512x512 reshape verified against a
  shuffle control (row-norm cv 1.12 vs 0.17), and the accumulation choice measured on
  real weights — fp32 accumulate is **284x** more accurate than fp16 on one layer.
  Architecture is now recovered too — `notes/phase3-architecture.md`: 5 encoder /
  5 decoder Swin stages at **32/64/128/256/512 channels, 32 per head**, a ViT-1D
  bottleneck (blocks 31-38), `dec_input_upsample 1024->512`, shifted-window MSA
  confirmed by `_shifted` kernels, and the full parameter-name inventory. Each
  `block{N}.layer{M}.layer` is a **packed blob of several parameters concatenated**,
  which is why the sizes never factored into `in x out`. The model was exported from
  PyTorch (ATen op names survive in `.rdata`). Codenames: feature CG2R, engine HNet,
  configs `crazy-cuckoo` and `hnet-vigilant-squid`.
- **Phase 4 — GPU path. DONE 2026-09-09** — `src/gpu/nr_xmx.py`,
  `notes/phase8-xmx-graph.md`. Every GEMM in the recovered graph runs on XMX in FP16
  with FP32 accumulate, the batched per-head attention included (a second pipeline with
  the batch on `gl_WorkGroupID.z`; the key is read column-major so no transpose is
  copied). 384x384: 45.1 s -> 16.8 s. The kernel itself measures **1348 GFLOP/s**.
  Two findings dominate everything else here: `libxmx.c` was choosing the *uncached*
  host-visible memory type, which ran readback at 80 MB/s and hid the kernel entirely
  (one line, 100x on a large GEMM); and **the graph is chaotic** — a relative 1e-06
  perturbation of the input moves the head as much as an FP16 GEMM does, because ~100
  E4M3 publishes with a 6.25 % step stand between input and output. Bitwise agreement,
  with the CPU or with NVIDIA, is unattainable by construction. *(Historical:* Port to XMX via Vulkan
  cooperative matrix in **FP16** (not BF16 — the weights ship as FP16), compute
  shaders, tiled. Validate layer-by-layer against Phase 3.
  `src/gpu/gemm_coopmat.comp` + `src/gpu/gemm_runner.c` run FP16 x FP16 -> FP32 GEMM
  on the XMX units and match numpy to 2.4e-06 on synthetic data.
  **Critical finding — `notes/phase4-subnormal-flush.md`: XMX flushes subnormal FP16
  operands to zero, and 27.22 % of this model's parameters (20.1 M of 73.8 M) are FP16
  subnormals.** Median matrix is 9.64 % subnormal, worst is 82.92 %, and 20 matrices
  are over half. Run naively a quarter of the network evaluates to zero, silently.
  Fixed exactly by a per-tensor `2^k` rescale (`fp16_shift()` in `hnet_model.py`);
  residual error returns to the 5e-06 of ordinary FP32 accumulation.
  *(Both halves corrected since. The 27 % counted the misread decode, and the real
  weights hold 7 subnormals (`phase61`). The flush is Mesa's default float-controls mode,
  not the XMX units': under Intel's Windows driver the same units keep float16
  subnormals (`phase71`, 2026-10-01), and libxmx now declares `DenormPreserve 16`, so
  Mesa keeps them too.)*
  **All three block families now validated on hardware** — 20 layer evaluations,
  worst deviation 2.8e-04 (`src/gpu/test_attention_gpu.py`). The fused Swin layout is
  now anchored on the cosine gate, which is exactly `C` long after 8 zero pad bytes at
  every width; that fixed a real extraction bug where the gate sat inside `wq`.
  Open: the Q/K/V split needs a fractional KV head at C=32 and C=64 under GQA 4:1, so
  those 15 narrow blocks — built from different kernel classes — use some other
  structure, and the loader deliberately does not guess it.
  **NVIDIA accumulates in FP16, not FP32** — zero of 218 PTX kernels use an FP32
  accumulator (`notes/phase4-accumulation-choice.md`). Config 1 is 400–800x *more*
  accurate than the original rather than a reproduction of it; an FP16-accumulate
  shader is built and tested alongside, since it is the only way to compare against
  hardware-captured activations. Which is wanted is the owner's call.
  **End-to-end pass working** — `notes/phase4-end-to-end.md`, `src/ref/forward.py`:
  a tensor goes through all 71 blocks in execution order at 512x256 down to a 16x8
  bottleneck and back, on CPU and on XMX, agreeing to **9.7e-07 relative** over 4.2 M
  output elements. Plumbing only: 27 blocks pass through, the fused blocks' leading
  region is unapplied, and the runtime scale is still missing. **Resident context built**
  (`notes/phase4-resident-context.md`): `src/gpu/libxmx.c` keeps the Vulkan device,
  pipeline and buffers alive across calls, cutting the end-to-end pass from 13.1 s to
  **6.3 s** (CPU numpy: 11.5 s) with correctness unchanged. First honest throughput:
  **0.7–1.9 TFLOP/s** FP16 with FP32 accumulate. That is a floor — the kernel has no
  shared-memory staging, no K-blocking and no register reuse, so it is memory-bound
  rather than matrix-unit-bound. Optimisation stays deferred until Phase 3 correctness
  is settled, per the roadmap. The 6.3 s is not a frame time either: most of it is the
  CPU-side softmax and window shuffling in numpy.
- **Phase 4b — Temporal. DONE 2026-09-09** — `src/ref/nr_temporal.py`,
  `notes/phase12-temporal.md`. A sequence, not a still: the previous output reprojected
  along motion vectors into feature channels 7-9 and blended back through the head's
  fourth channel, `alpha = clamp(sigmoid(half(logit)) * half(0.73974609375), 0, 1)`.
  The gate is learned and it discriminates — 0.008 with no history, **0.705** with
  correct history, **0.032** when the motion is wrong, which is ghosting rejection.
  Static-scene flicker falls 3.6x by the fourth frame (6.3x at the peak) with no
  high-frequency loss.
- **Phase 4c — Residency. DONE 2026-09-09** — `src/gpu/nr_frame_resident.py`,
  `notes/phase15-residency.md`. The whole graph runs on the device: operands travel as
  64-bit addresses in push constants so there are no descriptor sets, a block records
  as one command buffer, and activations never return to the host. 0.26 s at 384x384
  and 1.56 s at 720p, 48x and 41x against the best CPU, head correlation 0.9918 and a
  visually indistinguishable picture. Within 4.6x of the arithmetic floor.
- **Phase 5 — Integration. DONE 2026-09-10** — `notes/phase34-doa5.md`. Not through NGX,
  which needs an NVIDIA GPU, but through a **Vulkan layer** at `vkQueuePresentKHR`: it
  attaches to any Vulkan application including a Windows game under Proton, copies the
  presented frame to a daemon holding the model, and writes the result back into the
  swapchain. A file is the trigger, so it began as a photo mode; the live mode that runs
  every present came after (`phase47`, `phase59`).
  Proven on **Dead or Alive 5 Last Round** — 32-bit D3D9 through DXVK, which needs a
  32-bit layer library; `src/layer/prepare_layer.py` writes both manifests.
  `src/layer/`, `notes/phase17`, `phase19`, `phase24`, `phase34`.

---

## Resolved 2026-09-07 — do not re-litigate

1. **Cooperative matrix table.** Answered by our own probe, not by `vulkaninfo`.
   6 configs, single shape 8×16×16, subgroup scope. See `notes/hw-coopmat.md`.
2. **OpenCL DPAS shapes.** `cl_intel_subgroup_matrix_multiply_accumulate` *and*
   `..._tf32` are both present, confirming DPAS hardware beyond what Vulkan exposes.
   Using it would mean a second, OpenCL backend — not a flag.
3. **FP32 accumulation — yes**, and the path turned out to be **FP16 in**, not BF16:
   the DLL ships FP16 weights, so config 1 (`fp16 × fp16 → fp32`) takes them with zero
   conversion. Config 0 (`fp16 × fp16 → fp16`) is what NVIDIA's own kernels use and is
   implemented behind `-DACC16`; it is exact, halves the register pressure, is worth
   1.36x on an isolated GEMM and **nothing at all in a frame**
   (`notes/phase31`, `phase38`).

## Archive — two superseded sections, removed 2026-09-10

Both were long, both were wrong, and both are preserved in git history rather than here.

- **The denoise-score acceptance test.** `run_frame.py` and `denoise_score` were built
  on the wrong weight decode, and DLSS-NR is a detail re-render rather than a denoiser,
  so the objective was never right either. The live acceptance test is
  `src/ref/nr_frame.py` plus the two controls in `notes/phase7-first-render.md`.
- **"Goal, as redirected by the owner 2026-09-08."** It recorded a project with no
  result yet, a score 1.43x worse than doing nothing, and a network that moved the frame
  by 0.35 %. All of that was the wrong weight decode. `notes/phase6-mlx-dlss-unpack.md`
  is where it turned around.

---

## Open questions — all closed, and where the answers are

Every question this section once listed has been answered. Kept as an index, because
the answers are the useful part and several of them are counter-intuitive.

- **The graph, the operators, the layout.** All recovered. The port is bit-identical to
  MLX-DLSS's PyTorch original given the same GEMM, and per-element agreement with a
  float32 reference is impossible by construction — the graph is chaotic, and numpy's own
  float32 GEMM carries more error than the threshold below which perturbations vanish.
  Judge on the composed image and the controls. `notes/phase9-numerics.md`.
- **The `128C` region is the attention bias**, in a 12-bit fragment permutation.
  `notes/phase3-bias-region.md` and the block-family notes.
- **The attention non-linearity is a softmax**, hand-rolled in `f16x2` with hard logit
  clamps and no max subtraction. `notes/phase5-softmax-found.md`.
- **`attn_scale` is FP32 per head.** `notes/phase5-attn-scale-fp32.md`.
- **`VK_NV_cooperative_matrix2`** advertises zero flexible-dimension configs here.
  **OpenCL** *is* reachable and *is* slower — its DPAS path peaks at 3533 GFLOP/s
  against Vulkan's 3828 on the same shape, and collapses at the same block size.
  `notes/phase33-opencl-measured.md`.
- **Edge padding** for unaligned tiles: the buffers carry the padding, since
  `cooperativeMatrixRobustBufferAccess` is false. Settled in the resident path.

### What is actually left

The three items this section carried on 2026-09-10 morning are all closed. A game frame
worth looking at exists and is measured (`notes/phase42`); the interface mask is proven on
a live HUD, with a failure mode found and fixed the same evening (`phase43`); and the
layer is proven under a second Vulkan client, VKD3D-Proton on a 64-bit D3D12 title
(`phase41`). What replaced them:

1. ~~**The full-frame passes *around* the network.**~~ **Taken, 2026-09-12.** They are in
   C now — `src/ref/nr_image.c`, from the parallel `ProjectsCodex` tree and extended here
   for the history channels and the temporal composition. Host passes **74 -> 28 ms**,
   output byte-identical. `active_region` was the last one left and is now found once and
   checked in eight lines rather than rescanned. `notes/phase57`, `phase47`, `phase48`.
   What remains around the network is small; the graph is 185 ms of a 214 ms frame.
2. **A neural upscaler.** Half the extent is three times faster and keeps only **62 %** of
   the high-frequency band, because the detail is drawn at the wrong scale and no
   interpolator can reconstruct it — that is why the vendor's own arrangement puts DLSS
   after the pass. XeSS is the substitute and is unverified on Linux/Vulkan here.
   `notes/phase37`. *The owner dropped the DLSS-SR research on 2026-09-22 — a reading of
   `nvngx_dlss.dll`, kept only on the local branch `upscaler`; do not restart it unasked.*
3. ~~**A DX12 game that starts.**~~ **Done, 2026-09-16: Mortal Kombat 1**, D3D12 through
   VKD3D-Proton, with a picture — run off the BitLocker Windows partition with the Proton
   prefix kept on Linux. Full render scale does not fit in memory beside it. `notes/phase62`.
   (DOA6LR still dies inside its own build: `phase41`.)

**Performance inside the graph was declared finished here, and that was wrong** — corrected
2026-09-23: each pass is efficient, but over a third of the frame was passes that need not
exist. Eight bit-identical fusions, shared memory kept inside 2 KB and a padded bottleneck
took 1280x720 from 445 to 231 ms and the curve to `10 ms + 230 ms per megapixel`
(`notes/improve-fusions.md`, `notes/improve-qkv-epilogue.md`, HANDOFF). The staged GEMM
had been running on half its threads — 15.5 KB of shared memory a workgroup, where a core
holds 128 KB between them; given all of them, and a partial last block, it took the curve to `9.4 ms + 196`
(`notes/improve-shared-memory.md`, which also records a Mesa quirk that makes some *smaller*
declarations slower and costs this frame nothing). And window attention and the fused
feed-forward, with no L1 at their shared-memory size, fetched the same operands from L2 once
per subgroup; sharing them took those passes to 0.48x and 0.6x and the curve to
`8.9 ms + 162` (2026-09-25, same note). What follows is the
per-pass record, which still stands.
`xmx_profile()` timestamps every pass (`src/bench/frame_profile.py`): GEMM is 216 ms of
488 at 720p and is register-bound; of the other 272 ms, every pass that only moves data
runs at 61-104 GB/s against a machine ceiling of 70-91, and the only two below it are
arithmetic. The extent curve is `17 ms + 488 ms per megapixel`. Levers measured and closed:
register tiling, operand staging, integer weights, storage width, the accumulator format,
OpenCL, and — new — shared-memory bank padding (**1.11x**, not the textbook 32x) and
handing work to the four E-cores (**-7 %** for a theoretical +2 %). `notes/phase45`,
`phase46`.

**Both modes run in a real game.** Photo mode holds a frame while a trigger file exists;
live mode (`NR_LAYER_LIVE=N`) runs continuously — **26-27 ms a frame at 512x288 and at
640x360** for the daemon alone (2026-09-26), and in Tekken 7 beside the game's own rendering
**30 fps at 800x450** with the render scale at 0.35 and 27 at 0.6 (the owner, 2026-09-27;
10.5 at 640x360 before the fusions, `phase59`) — with the game set to that extent and the
compositor doing the stretch. `src/layer/nr-ctl`
changes profile, intensity, both strengths, the render scale and the temporal knobs
between frames without reloading the model, and `src/layer/nr-toggle` is the same three
files on a key, because on Wayland only the compositor sees a key while a fullscreen game
has focus. The binding is made in System Settings and **must not** be made by us: doing it
over kglobalaccel's D-Bus interface crashed KWin on the first keypress, because in Plasma
6.7 that registry lives inside KWin and `plasma-kglobalaccel.service` is not even running.
`notes/phase55`. `src/layer/nr-panel` is all ten knobs on one screen, and
`src/layer/nr_knobs.py` is the one definition that it, `nr-ctl` and `README.md` all
render, checked by `make test`. `notes/phase56`.

**Live mode carries a frame of history, and the daemon is stateful because of it.** The
previous output goes into feature channels 7-9 with identity reprojection — a present-time
layer has no motion vectors, and identity is bit-exact, so it costs nothing — and the
model's learned gate decides per pixel how much survives. The gate turns out **not to be
local**: it reads 0.12 over pixels that did not move on a frame where most things did,
against `phase12`'s 0.705 on a scene where the history was correct everywhere. So there is
a floor under it, driven by the one exact motion signal a layer has — whether the game
handed back the same pixel. Result: **3.7x less flicker for 3.7 % of the frame time**, and
moving pixels untouched. Optical flow was measured and does *not* help here.
`notes/phase53`, `phase54`.

## Repo layout

```
ref/     immutable originals — the DLL, recorded hashes. NEVER modified, NEVER committed.
work/    working copies, carved sections, dumps, scratch
notes/   findings, section maps, kernel name lists, the model graph spec
src/     our code
```

---

## Rules

- **Never commit or redistribute `nvngx_dlssnr.dll` or any weights derived from it.**
  They are NVIDIA's. Every existing project in this space requires the user to supply
  the DLL themselves, and this one does the same. The repo is code only.
- Never modify anything in `ref/`. Copy into `work/` first.
- Distinguish *reported* from *verified* in `notes/`. Most of what is written above came
  from press coverage and community frame captures, not from our own analysis.
- When something contradicts this file, the machine is right and this file is wrong —
  update it.

---

*Last updated 2026-09-27 (the graph aligned with the vendor's in eight more places, the styles' colour grade and the history read out of the DLL, the render scale filling the cheapest network field, CMake brought level with make, and Xe2's 256-register mode tried; before that window attention and the fused feed-forward sharing their operands, the fusions, the staged GEMM's shared memory). **Read `notes/HANDOFF.md` first** — it carries the current state and the traps. Owner runs Arch Linux, is comfortable at kernel/driver level,
prefers C for low-level work, and does not need concepts explained from scratch.*
