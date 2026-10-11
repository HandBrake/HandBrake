# DLSS-NR, as recovered

What the network is, what it expects, and what it returns — written for someone who wants
to run it somewhere it was not meant to run.

The `notes/` directory is the record of how each of these was established, including the
attempts that were wrong; this file is the answer without the archaeology. Where the two
disagree, **this file is current** and the note is dated.

Everything here was recovered from a shipped `nvngx_dlssnr.dll`, build 310.8.0.0, by
reading its container and its embedded code — and cross-checked against
[MLX-DLSS](https://github.com/iamwavecut/MLX-DLSS), an independent extraction of the same
binary from vendor captures. The two agree on all 649 tensors with 0 missing, 0 extra and
0 shape mismatches, which is the strongest evidence available without NVIDIA hardware.

**No weights are here.** This is the shape of the thing, not the thing.

---

## 1. What it is

A **one-step pixel-space diffusion model** that re-renders a frame's detail, conditioned
on the rendered frame, carried temporal state, and three artistic-direction scalars. It is
not a denoiser and not an upscaler: input and output are the same extent, and what it
returns is a *residual* to add to the frame it was given.

Internally: a symmetric U-Net of **71 blocks** — five encoder and five decoder stages of
shifted-window (Swin) attention, an eight-block ViT-1D bottleneck, and an upsample between
them. **145 755 123 parameters**, the large matrices stored as FP8. Vendor codenames: feature `CG2R`,
engine `HNet`, configs `crazy-cuckoo` and `hnet-vigilant-squid`.

## 2. The contract — this is the reusable part

### Extent

The network runs on a padded field, **at least 320 on each axis**, each side aligned to the
graph's own reductions: two to the number of halvings, rounded up to 4, that shrink it, and
one more when level 0 would not be whole 8-pixel windows — 64 for most sizes, 128 for some.
And when both sides come out at four alignments the width takes one more. A frame smaller
than its field is mirrored outward to fit and cropped back afterwards; the mirror is a
reflection of row and column indices, not padding.

```
1024x576 -> 1024x576      already aligned
 563x317 ->  640x320      317 aligns to 64, 563 to 128
1920x1080 -> 1920x1152    level 0 would be 540 rows: aligned to 128
1280x720 -> 1344x768      1280x768 is four alignments each way: one more on the width
   64x48 ->  384x320      the floor, then as for 1280x720: 336, rounded up to 64 here
```

The last step is the one to keep. A field whose sides are both multiples of 256 pools to a
bottleneck with no padding token, and on every such field the pass comes out 25-30 % weaker
than on the fields around it; the vendor's extra column keeps the common sizes off them. This
is the vendor's rule as [OpenDLSS-NR](https://github.com/maanHimself/OpenDLSS-NR) reproduces it
from captures (`notes/opendlss-reference.md`); MLX-DLSS's — a multiple of 64 — put 1280x720 on
1280x768. Below 129 pixels a side the rule need not give a multiple of 64, which this
implementation's exact halvings cannot follow, and it rounds up there.

### Input: 16 channels, float32

| channel | contents |
| --- | --- |
| 0–2 | deterministic noise for this extent and frame index |
| 3 | constant 1 |
| 4–6 | the current frame, scaled |
| 7–9 | the **previous prediction** — the composed head before the grade and the intensity, in half — reprojected along motion, scaled the same way |
| 10 | normalised style index |
| 11 | local tone strength |
| 12 | local structure strength |
| 13 | skin structure strength, or −1 when the automatic mask is off |
| 14 | automatic-mask structure strength, or −1 |
| 15 | unused, zero |

The colour scaling is three FP16 roundings and is **not** a single multiply:

```
scaled(x) = half(half(half(x) - 0.5) * 0.125)
```

On a first frame, channels 7–9 repeat channels 4–6 — the model is told the history is the
current frame. The noise is a function of the extent and the frame index and nothing else,
so it is worth memoising; it is a Gaussian pair built from a hash of the pixel coordinates
and the frame index, rounded to FP16.

With a per-pixel **control mask**, channels 11 and 12 become that mask's green and blue
times the corresponding strength, and 13/14 go to zero.

### Output: a 4-channel head

| channel | contents |
| --- | --- |
| 0–2 | RGB residual |
| 3 | the temporal gate, as a logit |

The composition, with every rounding point that matters:

```
predicted = clamp(colour + half(head.rgb) * 0.25, 0, 1)
alpha     = clamp(sigmoid(half(head.a)) * half(0.73974609375), 0, 1)
output    = predicted + alpha * (history - predicted)
```

`history` here is channels 7–9 recovered as `channels * 8 + 0.5`. `output` is also what
the next frame gets as its history — before anything below — held in RGBA16F, truncated
toward zero. A post-process of its own (`cg2r_post_process_kernel`) then grades that half
for the style and blends it against the untouched frame by `intensity` and the masks:

```
graded = style_grade(half(output))            # natural and cinematic only
result = frame + intensity * mask * (graded - frame)
```

Ours extrapolates above an intensity of 1, which the vendor's pass does only when something
else — a grade, a mask — makes it run. `notes/phase70-post-process.md`.

### The controls

Four profiles: three scalars for the network, and for two of them a colour grade after it,
read out of the DLL's own style table — each value times the tone clamped to [0, 1]:

| profile | style | tone | structure | the grade after the network |
| --- | --- | --- | --- | --- |
| `standard` | 0 | 1 | 1 | none |
| `natural` | 1/128 | 1 | 1 | exposure -0.1 EV, contrast -0.25, saturation -10 % |
| `cinematic` | 2/128 | 1 | 1 | saturation -15 % |
| `neutral` | 0 | 0 | 0 | none |

The contrast is a smoothstep blend, `c + k * (c²(3 - 2c) - c)`, and the saturation is HSL's.

They are a trade, not a quality ladder: everything the pass adds to skin texture it takes
out of speculars and colour. `notes/phase44-profile-tradeoff.md` measures the curve.

## 3. The graph

Five encoder stages at **32 / 64 / 128 / 256 / 512 channels**, head dimension **32**, so
1/2/4/8/16 heads; an eight-block **ViT-1D** bottleneck at C = 1024; `dec_input_upsample
1024 -> 512`; five decoder stages back. Attention is shifted-window over **8x8 windows, 64
tokens**.

One transformer layer at C = 256 (8 heads), exactly as the extraction gives it:

| tensor | shape | |
| --- | --- | --- |
| `qkv_weight` | (C, 3C) | Q, K and V, each C x C — **full multi-head attention** |
| `projection_weight` | (C, C) | |
| `attn_bias` | (heads, 64, 64) | a relative bias over the 8x8 window, `128C` in total |
| `attn_scale` | (heads) | **FP32**, per head |
| `attn_cos_skip`, `ffn_cos_skip` | (C) | the cosine gates on the skip, exactly `C` long |
| `ffn_expand_weight` | (heads, 4, 8, 32, 32) | grouped expansion |
| `ffn_branch_projection_weight` | (heads, 4, 32, 32) | |
| `ffn_output_projection_weight` | (C, C) | |

**There is no grouped-query attention.** An early reading of the container sizes suggested
4:1 and a `1.5C²` QKV; the logical `qkv` is `(C, 3C)`, and the apparent halving was the
storage format, not the attention (section 4). This file said otherwise when it was first
published, repeating a claim the project's own notes had already withdrawn.

The non-linearity in attention is a **softmax**, hand-rolled in `f16x2` with hard logit
clamps and **no max subtraction**. The ViT's is its own: another affine and a four-bit shift for
the exponential, the weights published unnormalised and the value sum normalised instead, over
keys padded to a whole 64 whose weight the denominator gives back.

**Each encoder level's skip is its last block's output** — blocks 4, 8, 14 and 22, the ones
whose output is also pooled into the next level, published E4M3 (and 30 at 512 channels).
MLX-DLSS's model, and this implementation until 2026-09-27, merged the block before it (3, 7,
13, 21). Run on the inputs of [OpenDLSS-NR](https://github.com/maanHimself/OpenDLSS-NR), which
claims its network bit-exact against captures of the original, the first decoder blocks agree
with it 18-25 % that way and 61-69 % this way, like any other block
(`notes/opendlss-reference.md`).

**Every GEMM reads its operand published as E4M3**; half carries only accumulators and the
32-channel blocks' skips. So a 32-channel block's QKV projection reads its feed-forward output
published while the attention's residual takes it raw; a feed-forward whose input arrives raw —
block 0's stem, the merges into blocks 66 and 70 — reads it published and keeps it raw as the
skip; the 512-channel and ViT blocks publish their feed-forward output, as the narrower branched
ones do; and the bottleneck pools block 30's raw output, published before its projection.
MLX-DLSS's graph fed four of those GEMMs a raw value; step by step on the reference's inputs each
of them agrees with it on 0.8-2.3 % of values one way and 48-100 % the other.

`notes/MODEL-SPEC.txt` tabulates the **container** — per-block element counts and layout as
stored. Those counts are storage, not parameters: their total, 73 841 889, is the weight
section's size divided by two, and the model has **145 755 123** (`notes/phase61`). The logical shape list is MLX-DLSS's `weight_spec.json`.

## 4. The weights

**145 755 123 parameters, in three storage formats.** The large matrices — 143.0 M
parameters — are **FP8 E4M3, one byte each**, in NVIDIA's QMMA tile layout. The small
tensors — attention biases, branch projections, cosine gates, 2.7 M — are FP16. The 714
`attn_scale` values are FP32. At those widths the model fits the DLL's 147.7 MB weight
section to within half a percent; stored densely as FP16 it would need 291.5 MB.

So there **is** a decode step: the extraction turns the E4M3 bytes into float and writes FP16
tensors, and those are what this implementation computes with. Read the container bytes as
dense FP16 instead and the values correlate **−0.02** with the truth — and, because its
headers say `data_len == 2 * n_elem`, the parameter count comes out at 73.8 M, half the real
one. This project made both mistakes, and published the second.

### Subnormals: a driver's mode, declared here

Whether FP16 subnormal operands survive the cooperative-matrix GEMM is the driver's
float-controls mode, not the XMX units': Mesa flushes them unless a shader declares a mode,
and Intel's Windows driver keeps them. Left undeclared, the same graph draws a different
picture on each. libxmx declares `DenormPreserve 16` on every module where the device reports
`shaderDenormPreserveFloat16` and lets the 16-bit mode differ from the other widths'
(`denormBehaviorIndependence = ALL`, as both report), so both keep them and compute the graph
bit for bit alike (`notes/phase71`) — as NVIDIA's tensor cores keep them; `XMX_DENORM16=driver`
leaves it to the driver. **The real weights hold 7 subnormal values in 145.8 M**: E4M3's
smallest non-zero magnitude sits far above FP16's normal threshold, so decoded FP8 cannot land
there. An earlier figure of 27 % was measured on the misread
container bytes and says nothing about the model. If your weights arrive in another format,
count before assuming either way.

## 5. Numerics, and what agreement is possible

**NVIDIA accumulates in FP16.** Zero of 218 PTX kernels use an FP32 accumulator. An
FP32-accumulate implementation is therefore 400–800x *more* accurate than the original on
an isolated GEMM — it is not a reproduction of it. Which you want depends on whether you
are matching their output or making a good picture.

**Per-element agreement is not a property a port with other arithmetic can have.** The
graph is chaotic: a relative 1e-06 perturbation of the input moves the head as much as an FP16
GEMM does, because roughly 100 E4M3 publishes, each with a 6.25 % quantum, stand between input
and output. NumPy's own float32 GEMM carries more error than the threshold below which
perturbations vanish. Judge on the composed image and on whether the controls behave;
`notes/phase9-numerics.md` has the measurements. OpenDLSS-NR claims per-element agreement by
doing the vendor's arithmetic itself — FP8 products summed as fixed point onto an f16
accumulator, the vendor's reduction orders — which this implementation does not; with the graph
the same, its head is 0.98-0.997 correlated with theirs on the same input and the pictures 0.5-1.9
levels of 255 apart, about what two arithmetics of one graph make (`notes/opendlss-reference.md`).

## 6. The temporal path

The previous prediction, reprojected along motion vectors into channels 7–9, blended back
through the head's fourth channel. The gate is learned and it discriminates:

| history given | gate |
| --- | --- |
| none | 0.008 |
| correct, reprojected | **0.705** |
| wrong — zero motion on a panning scene | **0.032** |

That last row is ghosting rejection, and it is why the vendor's output is temporally
stable when a single frame through the same network is not. Static-scene flicker falls
3.6x by the fourth frame.

**The gate is not local.** On a frame where most of the picture moves it reads about 0.12
even over pixels that did not move at all — the network is global, so a history that
disagrees over most of the frame is distrusted everywhere. `notes/phase54-flicker-fix.md`
measures this and what to do about it when you have no motion vectors.

## 7. What was not recovered

- **Which named parameter occupies which slice of each packed blob.** The totals are
  pinned and the names are known; the internal assignment is not. It does not matter if
  you use a logical extraction, which is why this stopped being urgent.
- **Which of `crazy-cuckoo` / `hnet-vigilant-squid` this blob is**, and whether both
  configurations ship.
- **The window shift offset.** Shifted windows are confirmed by `_shifted` kernel names
  and the window is 8x8; the shift itself is inferred, not read.

## 8. Where the evidence is

| question | note |
| --- | --- |
| the per-block table | `notes/MODEL-SPEC.txt` |
| widths, stages, kernel inventory | `notes/phase3-architecture.md` |
| kernel → layer class → template config | `notes/ptx-kernel-configs.md` |
| the container format | `notes/phase3-weight-format.md` |
| the subnormal flush | `notes/phase4-subnormal-flush.md` |
| the accumulator choice | `notes/phase4-accumulation-choice.md` |
| softmax and `attn_scale` | `notes/phase5-softmax-found.md`, `phase5-attn-scale-fp32.md` |
| the attention bias region | `notes/phase3-bias-region.md` |
| the 16 input channels | `notes/phase48-feature-inputs.md` |
| the controls | `notes/phase30-control-atlas.md` |
| the temporal path | `notes/phase12-temporal.md`, `phase54-flicker-fix.md` |
| the grade after the network, what the history holds | `notes/phase70-post-process.md` |
| why bit-exactness is impossible | `notes/phase9-numerics.md` |

`notes/INDEX.md` maps all of them.
