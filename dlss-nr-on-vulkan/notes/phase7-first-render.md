# The network renders a frame — first real result
2026-09-09

A Cyberpunk 2077 face crop goes in, a neurally-rendered frame comes out, with the
characteristic DLSS-NR effect: individual eyelashes and eyebrow hairs resolved out of
a smeared input, skin pores synthesised, iris and eyeliner sharpened. Nothing in this
project had produced that before today; every previous score measured the bilinear
scaffolding (`notes/HANDOFF.md` §6).

What changed is not a new insight of ours. It is that the weights are now **decoded
correctly** (`notes/phase6-mlx-dlss-unpack.md`) and the graph is now the one
`iamwavecut/MLX-DLSS` recovered from vendor captures, rather than the one we were
inferring from PTX alone.

## What was built

`src/ref/nr_model.py` — the recovered 71-block graph in numpy. A port of MLX-DLSS's
`python/mlxdlss/model.py` (PyTorch; Apache-2.0), because this machine has no torch and
because the GEMM has to be swappable for the Xe2 XMX path. Every rounding point of the
original is preserved: the E4M3 publishes, the half-precision fragment-tree cosine
normalise, the bit-affine softmax approximation, the FP32-per-head `attn_scale`.
The single GEMM entry point is `nr_model.MATMUL`.

`src/ref/nr_frame.py` — one RGB frame in, one RGB frame out. The 16-channel feature
assembly (with its deterministic PCG noise) and the head-to-RGB composition are
MLX-DLSS's `features.py` / `composition.py`, which are already pure numpy and are
loaded by path from the clone in `work/`.

`src/ref/image_io.py` — fixed: `load()` with no `size` returned a flat byte array
instead of `(H, W, 3)`.

## The weights are confirmed by a second party

Our extracted `work/mlxw/dlssnr-logical.safetensors` against MLX-DLSS's
`weight_spec.json`: **0 missing, 0 extra, 0 shape mismatches** across all 649 tensors.
Two independent extractions of the same DLL agree exactly.

## Primitives, verified before the graph was run

| check | result |
|---|---|
| E4M3 round trip | all **253** finite E4M3 values are fixed points; saturates at 448; ties to even (17.0 -> 16, 17.5 -> 18) |
| bit-affine softmax | rows sum 0.96-1.03; correlation **0.9936** with a true softmax |
| fragment-tree cosine normalise | row norms 0.9993-1.0006 |
| attention-bias fragment order | the 12-bit index map is a **valid permutation** of 4096 |

## The run

384x384 crop of `pngs/Cyberpunk-2077_01.jpg` (Judy's face), network extent 384x384,
**45 s** on CPU.

```
head ch0-2   min -0.39  max +0.18  sd 0.03-0.09     <- the RGB residual, x0.25
head ch3     min -6.32  max -2.39  mean -4.29       <- 4th head channel, unused by RGB
change       mean|d| 0.02610   max|d| 0.20728
```

The residual magnitude is the right order: MLX-DLSS reports 0.0041-0.0048 MAE against
NVIDIA goldens on native game-face crops.

## Control 1 — shuffled weights

Every weight tensor shuffled in place (per-tensor distribution kept, learned structure
destroyed). If the effect survived this, it was never the model.

| | mean dR | mean dG | mean dB | mean\|d\| | HF gain | \|d\| textured/flat |
|---|---|---|---|---|---|---|
| trained  | -0.0040 | -0.0041 | -0.0035 | 0.02608 | 0.747 | **1.18** |
| shuffled | **-0.0356** | +0.0016 | +0.0045 | 0.01566 | 1.041 | 0.98 |

Shuffled produces a flat **red tint**, structure-blind (ratio 0.98) and with no change
in high-frequency energy. Trained is colour-neutral and concentrates its change on
textured regions. Visually the shuffled run has no pores and no lashes.

The trained HF gain of 0.747 is *below* 1: on a JPEG source the model suppresses the
incoherent high frequencies (mosquito noise, aliasing) while synthesising coherent
detail. `corr(d, highpass(input)) = -0.169` says the same thing. This is a re-render,
not a sharpen.

## Control 2 — the conditioning inputs (the decisive one)

Feature channels 10/11/12 carry style, local tone and local structure. Sweeping the
recovered control profiles, same image, same weights:

| profile | head ch0-2 sd | change mean\|d\| | vs standard |
|---|---|---|---|
| `neutral` (tone 0, structure 0) | **0.0036** | **0.00071** | — |
| `standard` | 0.1395 | 0.02610 | — |
| `natural` (style 1) | 0.1570 | 0.02937 | 0.02849 |
| `cinematic` (style 2) | 0.1685 | 0.03196 | 0.02372 |

`neutral` switches the network off by a factor of **37**, and the three styles are
distinct from each other. A wrong graph does not respond coherently to conditioning it
was never told about; this cannot be passed by accident.

## Performance, and why the GPU path now matters much more

numpy here is linked against **netlib reference BLAS**, single-threaded:

```
16384x32x128    2.5 GFLOP/s
4096x512x512    3.2 GFLOP/s
1024x1024x4096  2.9 GFLOP/s
```

`src/gpu/libxmx.so` on the same machine does 2048x512x512 at **26.8 GFLOP/s**
end to end (rel err 3.3e-04), and the kernel alone was measured at 0.7-1.9 TFLOP/s.
So the XMX path is a ~9x win as it stands and much more once the host-side float
conversion and padding come out of the inner loop. Phase 4's remaining work is now the
main lever on frame time.

## What this does not claim

- No NVIDIA parity gate. There is still no way to produce reference activations on this
  machine; the comparison is against MLX-DLSS's recovered semantics, not against the DLL.
- Single frame, no history. The temporal path (`temporal.py`, motion vectors, the
  five-tap history filter) is not ported.
- The 45 s is CPU numpy with reference BLAS. It is not a frame time.
