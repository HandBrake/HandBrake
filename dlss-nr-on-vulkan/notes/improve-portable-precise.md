# One `precise` cost the portable path a third of its frame

2026-10-03, on the owner's Apple M3 through MoltenVK — the portable Vulkan path, the one a
device without `VK_KHR_cooperative_matrix` takes (MoltenVK, the phones, Xe2 under
`XMX_PORTABLE=1`). Nothing here touches the matrix kernels, and no Xe2 was run.

**The 320x320 graph went from 99.6 to 60.7 ms of device time, 1344x768 (1280x720's field)
from 709 to 473 ms replayed, with both heads unchanged** — `e80b25dbbadfb76d` and
`5eb7742929a13f98`, the same on MoltenVK before and after. The C library's live table on the
same path:

| swapchain, scale | before, ms | after, ms |
| --- | ---: | ---: |
| 512x288 at 0.35 | 123 | 84.5 |
| 640x360 at 0.5 | 124 | 85 |
| 854x480 at 0.5 | 149 | 103 |
| 1024x768 at 0.55 | 223 | 154 |
| 1920x1080 at 0.55 | 507 | 344 |

Metal (`NR_GPU_BACKEND=metal`) is untouched and still 31 ms at the live sizes.

## The profiler did not run on MoltenVK, so nobody had looked

`frame_profile.py` had never printed a table for the portable path: libxmx asks for a
timestamp pool of 8192 queries, MoltenVK backs a pool with one `MTLCounterSampleBuffer`,
Metal caps that at 32 KB — 4096 queries — and MoltenVK logs an error, "reverts to emulated
behavior" and answers every stamp with zero. The profile was a table of noughts and the
script divided by it. libxmx now reads the driver id (`VK_DRIVER_ID_MOLTENVK`) and asks for
4096 there; `XMX_STAMPS=N` sets the pool by hand. A frame is 300-600 passes.

With it working, 320x320 on MoltenVK read 99.6 ms: 64.9 of them GEMMs on the 16x32
register-tiled build (281 passes), 20.1 the one-head window block, 6.0 window attention.

## The 16x32 build ran at a third of its speed, on every shape

`XMX_PORTABLE_TILED=1 XMX_PORTABLE_WIDE=0` against the default routing, the frame's own
small-M shapes (ms, 8 or 16 calls):

| shape | 16x32, as it was | 8x16 | 16x64 wide |
| --- | ---: | ---: | ---: |
| 64x1024x4096 | 12.7 | 4.87 | 6.49 |
| 64x4096x1024 | 7.98 | 4.11 | 3.62 |
| 144x512x512 | 2.88 | 1.44 | 1.58 |
| 256x512x512 | 4.44 | 2.24 | 2.07 |

That is the build the window-gathered QKV projections are forced onto — a 32-column block
is one head, and the epilogue normalises a head — and `gemm+qkv epilogue ... window gather`
was 29 ms of the 99.6. The handoff's two routing findings, "the 16x32 build is slower than
the 8x16 one on every shape" and "the 16x64 build is about twice as fast on the large ones",
were measurements of this.

Bisected with compiled variants under `XMX_TILED_SPV`, each timed on the same shapes:

| variant of the 16x32 build | 64x1024x4096 |
| --- | ---: |
| as it was | 12.7 ms, 337 GFLOP/s |
| the `RN == 2` block removed (no stage, no QKV epilogue, no pool) | 3.34, 1284 |
| the stage declared and touched, the epilogue code removed | 3.33, 1289 |
| the pool block (0x800000) removed, the QKV epilogue kept | 3.37, 1273 |
| the QKV normalisation removed, the pool kept | 12.7 |
| the pool kept, its `precise float total` made `float` | **3.31, 1299** |

So not the 2 KB shared stage, not the cosine tree's `h[32]`, not the ViT tree: one `precise`
on the pooled sum, `precise float total = stage[a] + stage[b]; total += ...; total += ...`,
executed by block 0's residual GEMM alone, cost every GEMM on the build 3.8x.

## Why: glslang spreads it, SPIRV-Cross spells it `optnone`

glslang implements `precise` by propagation, and the propagation is at the *object* level:
a precise read of `stage` makes every store into `stage` precise, and through those stores
the values stored — the accumulators, so the K loop's own `acc += av * bv` — and the index
arithmetic on the way. `spirv-dis | grep -c NoContraction`: **54** decorations on the build
as it was against **8** without the qualifier (publish.glsl's e4m3 polynomial, which is
meant); the 46 were 5 `OpVectorTimesScalar` (the five K-loop forms' multiply), 12 `OpFAdd`,
and 32 integer ops — `OpIAdd`, `OpIMul`, `OpUDiv` — on which NoContraction means nothing.

SPIRV-Cross's MSL backend then spells each NoContraction op as a helper marked
`[[clang::optnone]]` (`spvFMul`, `spvFAdd`; `MVK_CONFIG_SHADER_DUMP_DIR` shows them), so
the K loop's multiply-adds became calls into unoptimised functions. The bits do not change
— every output of six shapes on three builds hashed the same with and without, since
Metal runs these kernels with fast math off and contracts nothing either way — only the
code the compiler could make of the loop.

The same construct was in two more portable kernels, and both had the whole kernel
decorated: `window_block_portable.comp` read `region` precisely in two places (49
decorations, all four multiply-accumulate loops among them) and
`window_attention_portable.comp` read `scores` precisely inside `weights()` and in the
probability pass (23 decorations, the QK^T loop among them).

## What changed

Three kernels, all bit-identical on `test_gemm_qkv`, `test_window_block`, `test_glue`,
`test_window_residual`, `test_window_attention`, `test_gemm_residual`, `test_ffn_fused` and
`test_portable`, and on the two frame heads:

- `gemm_portable.comp`, `window_block_portable.comp`: the pooled sums lose `precise`. A chain
  of adds has no multiply to contract, so the qualifier protected nothing there.
- `window_block_portable.comp`, `window_attention_portable.comp`: the probability's
  `precise float product = weight * reciprocal` moves into a function, `probability()`,
  and `weights()` takes the logits as values rather than reading `scores` itself. A
  function parameter is a fresh object, so the qualifier stays on the one multiply it is
  for — `hmul()` in cosine_tree.glsl has always worked this way, which is why the QKV
  normalisation never contaminated anything.

Decorations after: tiled 8, window block 12, window attention 4. 320x320 on MoltenVK,
per pass: tiled GEMM 64.9 -> 47.8 ms, window block 20.1 -> 11.0, window attention
6.05 -> 3.41; device total 99.6 -> 70.8.

**And the routing, re-measured with the build fixed.** The 16x32 build now wins on every
shape tried, small and large, against both the 8x16 and the 16x64 build:

| shape | 16x32 | 8x16 | 16x64 wide |
| --- | ---: | ---: | ---: |
| 64x1024x4096 | 3.33 | 4.87 | 6.43 |
| 256x512x512 | 1.52 | 2.24 | 2.06 |
| 1600x1024x1024 | 7.39 | 11.0 | 9.11 |
| 6400x2048x512 | 14.3 | 21.7 | 17.4 |
| 102400x128x128 | 3.95 | 5.87 | 4.68 |
| 1600x32x128 (N = 32) | 0.415 | 0.403 | — |

So libxmx's portable defaults are now `XMX_PORTABLE_TILED=1` (the 16x32 build for every
shape it fits) and `XMX_PORTABLE_WIDE=0` (the 16x64 build not built); both remain settable
for a device where the measurement comes out otherwise. Tiled GEMM 47.8 -> 39.6 ms, device
total 70.8 -> **60.7**. 1344x768 after everything: 445.6 ms of device time — GEMM 248,
window block 108, fused feed-forward 57, window attention 25.

## Measured and dropped

- **The window-gathered A fetched four K terms at a time** (one 8-byte load where the
  plain forms take theirs so): the QKV window-gather GEMMs ran **1.4x slower**
  (256x1536x512 0.46 -> 0.64 ms a call, 576x768x256 0.27 -> 0.38), same bytes. The loads
  were not the bound; the selects and the extra vector live across the loop were.
- **The window block's weights fetched as aligned quads** (QKV, projection and head):
  **22 % slower**, 11.1 -> 13.6 ms of the 320x320 frame. The handoff's Metal finding
  ("vector loads in the portable kernels: slower, the compiler already combines them")
  holds through MoltenVK too for these two; it did not for the plain GEMM's own 8-byte
  operand fetches, which stay.
- **A 32x32 register block** (`-DRM=4 -DRN=2`): slower than 16x32 on every shape
  (1600x1024x1024 9.13 against 7.42 ms, 64x1024x4096 6.55 against 3.35) — the 32
  accumulators cost more occupancy than the halved B loads buy. **A 32x16 one**
  (`-DRM=4 -DRN=1`): 5 % faster on the large plain shapes (1600x1024x1024 7.04, 1905
  GFLOP/s) and no faster on the small, and it cannot take the QKV epilogue (a 16-column
  block is half a head), so it would be a third build in the routing for 5 % of a part of
  the frame. Not kept.
- **The QKV epilogue fused at all, on this driver.** With `NR_QKV_EPILOGUE=0` the frame had
  been *faster* on MoltenVK (124.9 -> 115.7 ms) — the unfused passes dodged the crippled
  build. With the build fixed the fused path is the faster one again, as on Xe2.

## What this leaves open

- **The matrix-path shaders carry the same spread.** On Mesa's builds in this tree:
  `gemm_staged.spv` 28 NoContraction decorations, `window_block.spv` 44, `ffn_fused.spv`
  27, `resident.spv` 71, `window_attention.spv` 16 — the integer ops and the pure adds
  among them are the same propagation from pooled sums and probability products. Mesa's
  backend has no `optnone`; what NoContraction costs it on a scalar FP add, if anything,
  has not been measured, and it cannot be from here. A Xe2 session should count them,
  apply the same two moves to `gemm_staged.comp`, `window_block.comp`,
  `window_attention.comp` and `resident.comp`, and compare `frame_replay.py` heads (they
  must not move) and times.
- **The HLSL twins** write `precise` too, with dxc's own propagation (HLSL's is also
  backwards through contributing expressions). Nothing there has run.
- **At 1344x768 the window block is a quarter of the portable frame** (108 ms, 16 393
  windows in 27 ms = 1.7 us a window). Its QKV normalisation runs on 8 of 32 lanes with
  `h[32]` a lane; the matrix-path kernel spread the same tree over four lanes a row with
  shuffles and went 10.9 -> 7.1 ms on Xe2 (`improve-b.md`). The portable twin has not had
  that, and would need `GL_KHR_shader_subgroup_shuffle`, which MoltenVK and the phones have.

## Also from this session

- `src/tools/dump_embedded_weights.c` (CMake target `dump_embedded_weights`, not on
  Windows: the chunk table is data): the safetensors compiled into libnr_frame, back out
  as the file the Python graph reads. This Mac's `work/` had gone again; the dump is
  291 576 650 bytes, sha256 `a55e6f9b2c5cbd6a…`, and every Python test and bench ran on
  it. Clone MLX-DLSS beside it as `README.md` says.
