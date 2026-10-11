# Phase 31 — the weights are already native; the accumulator is the only format left

2026-09-10. Asked whether the model could be converted to a format native to this
hardware. The answer has two halves and the second one is more interesting than the
first.

## The weights need no converting — they never did

The cooperative-matrix table on this device offers six configurations, and the matrix
formats it accepts are **fp16, bf16 and int8** (`notes/hw-coopmat.md`). The DLL ships
its weights as **FP16**, and config 1 is `fp16 x fp16 -> fp32`, so the stored bytes
reach the XMX units with **zero conversion**. That has been true since phase 3 and it
is why notes/CLAUDE.md forbids a BF16 conversion: it would throw away 3 of 10 mantissa bits
to reach a format the hardware likes no better.

The two obvious alternatives are both closed and both already measured:

- **int8** — the units have it, at twice the K depth. Phase 23: the weights survive it,
  the hardware will not take them alone (config 4 requires *both* operands integer), and
  the activations do not survive — 180 000x of dynamic range, a fifth of the non-zero
  values annihilated at the widest level.
- **bf16** — phase 22: the same two bytes, so no traffic saved, and 7 explicit mantissa
  bits against float16's 10, which cannot hold the half-rounded values the gate and the
  softmax produce by construction.

The E4M3 publishes are not a storage choice either. They are part of the function the
network computes — a hundred quantisation points the training baked in — and an E4M3
value is exactly representable in float16, so we already *store* them natively and only
compute the rounding in software. That software rounding costs about 1.3 ms per 251 M
elements, so roughly 2 % of a frame. There is nothing there.

## The accumulator is the one format choice still open

Config 0 is `fp16 x fp16 -> **fp16**`, and **that is what NVIDIA's own kernels use** —
zero of 218 PTX kernels carry an FP32 accumulator
(`notes/phase4-accumulation-choice.md`). This project chose config 1 and has been
400-800x *more* accurate than the original ever since.

It is also the exact constraint that caps the register block. One 8x16 accumulator is
**4 GRF registers in float32 and 2 in float16**, and phase 26 established that the
register file is the ceiling. Switching halves the pressure, precisely as the arithmetic
predicts:

| register block | spills, fp32 acc | spills, **fp16 acc** |
|---|---|---|
| 16x32 | 15:15 | **0:0** |
| 32x32 | 97:141 | 24:24 |
| 32x64 | 474:435 | 200:207 |
| 32x128 | 1099:1228 | 496:443 |

And the isolated throughput moves with it:

| register block | fp32 acc | **fp16 acc** | |
|---|---|---|---|
| 16x32 | 2345 GFLOP/s | **3178** | 1.36x |
| 32x32 | 1306 | 2717 | 2.08x |
| 32x64 | 744 | 1666 | 2.24x |

16x32 remains the best block either way; a lower register cost does not make a larger
one worthwhile.

## It buys nothing in a frame, for a reason worth knowing

| | 720p frame |
|---|---|
| fp32 accumulate | 488, 499 ms |
| fp16 accumulate | 511, 490 ms |

Indistinguishable, against a 1.36x isolated gain. The mechanism is specific: **a float16
accumulator cannot take the direct store.** The destination buffer is float32, and
converting a cooperative matrix on the way out is one of the five things the ANV bug
scrambles (`notes/phase18-fusion.md`), so every GEMM must detour through the
shared-memory stage. The GEMMs that previously stored straight to global now pay for
staging, and that pays back the accumulator's saving.

It also costs five layer regressions. Whole-graph correlation with the host reference is
**0.984328** against float32's 0.981311 — no worse, arguably closer, which is what one
would expect from moving toward the vendor's own arithmetic — but `softmax(scores)`,
`context`, `merged publish`, `attention output` and the half-input equivalence check all
fail at their tolerances, correctly: fp16 accumulation of a K=4096 dot product carries
real error, and the isolated GEMM shows it growing with K (6.8e-02 at K=32 to 3.2 at
K=4096 on outputs near zero).

**Kept behind `-DACC16`, off by default.** It is the vendor's arithmetic, it is
correctly implemented, and if the ANV store bug is ever fixed the direct store returns
and this becomes worth re-measuring immediately.

## One bug found on the way

The first attempt reported `corr nan` and looked like fp16 overflow. It was not: the
float16 accumulator was being handed to `coopMatStore` against a `float[]` destination,
and the results came back finite but wrong by up to nine orders of magnitude — a type
pun, not a range failure. Worth remembering that "NaN from a narrow accumulator" is not
self-explaining.
