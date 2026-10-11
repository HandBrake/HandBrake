# What the CPU and the GPU can and cannot agree on
2026-09-09

`notes/phase8-xmx-graph.md` claimed the graph is chaotic and left it there. Pushed
further, the picture is sharper, and one of the things said there was wrong.

## The measurements

Everything below is the 384x384 Cyberpunk face crop, head RGB channels, sd **0.1395**.

**1. Where the CPU/XMX gap comes from.** Round every GEMM's *activation* to half in
the float32 reference, keeping float32 arithmetic; then do the same for the weights:

| | head RGB mean\|d\| |
|---|---|
| activations rounded to half | **0.017950** |
| weights rounded to half | **0.000000** |
| the actual XMX path | **0.017441** |

So the gap is one effect, not an accumulation: the FP16 rounding of the activation at
the GEMM inputs. The weights change nothing because they are *already* half — 579 of
the 649 logical tensors are stored F16 and the other 70 are `attn_scale`, which is not
a GEMM operand.

And it touches almost nothing: **6801 of 6987 GEMM activations (97.3 %) are already
exactly half-valued**, because they arrive from an E4M3 or a half publish. Only 186
GEMMs in a whole frame are affected.

**2. The GPU changes are numerically free.** Ablating them against each other:

| | vs CPU | vs each other |
|---|---|---|
| XMX, GEMM hook only | 0.017441 | — |
| \+ batched attention on the GPU | 0.017441 | **0.000000** |
| \+ folded branched feed-forward | 0.017441 | **0.000000** |

Bit-identical. Both take inputs that are already E4M3-published, so FP16 is lossless on
them, and what they do perturb — float32 accumulation order, ~1e-07 — is annihilated by
the half rounding that immediately follows. The two 1280x720 renders, 123.1 s and
94.6 s, are pixel-identical.

**3. Each regime has a threshold, and they are five orders apart.** Perturb the input
colour by a relative amount and measure the head:

| perturbation | CPU reference (float32) | XMX (half activations) |
|---|---|---|
| 1e-09 | 0.0000000 | — |
| 1e-07 | 0.0131013 | — |
| 1e-06 | 0.0117830 | 0.0000000 |
| 1e-05 | — | 0.0000000 |
| 1e-04 | 0.0156280 | — |
| 1e-03 | — | 0.0169696 |

Below its threshold each path annihilates the perturbation **exactly**; above it, the
divergence jumps straight to 9-12 % of the head's sd and stays there. The threshold is
the precision of the activations: ~1e-07 for float32, ~1e-04 for half. There is no
gradual region.

The mechanism is the publishes. E4M3 has a 6.25 % step and about a hundred of them
stand between input and output. A perturbation below the working precision snaps back
onto the same lattice everywhere; one above it flips a few roundings, each flip is a
6 % jump in one element, and those seed further flips. **The vendor's half precision
makes the graph more stable, not less** — the float32 reference is the sensitive one.

**4. The floor cannot be reached from below.** Carrying an activation as a sum of
halves (`xmx.gemm_split`), measured against a float64 GEMM:

| | relative error |
|---|---|
| one half operand | 2.1e-04 |
| **two** | **1.7e-07** |
| three, four | 1.7e-07 (no change) |
| numpy's own float32 GEMM | **5.2e-07** |

Two parts is already *more accurate than the reference's own arithmetic*, and a third
buys nothing because the FP32 accumulator is the limit. But 1.7e-07 and 5.2e-07 differ
by ~5e-07, which is above the float32 threshold in the table above — so the split-half
mode lands at 0.0124 rather than at zero. It costs 16.8 s -> 21.2 s for 150 split
calls out of 2500.

**Any correct float32 implementation would diverge from this reference by the same
floor.** The reference is not stable to its own rounding, so per-element agreement is
not a property a port can have. Judge on the composed image, where the residual enters
at 0.25 and is clipped (CPU vs XMX: **0.0044** against the model's own change of
0.0261, and MLX-DLSS's own gap to NVIDIA is 0.0041-0.0048), and on the behavioural
controls in `notes/phase7-first-render.md`.

## Withdrawn from phase8

- **"Chunk size is part of the numerics."** Wrong. The CPU reference is **bit-identical**
  at `CHUNK_TOKENS` of 4096, 8192 and 131072. This BLAS computes each output element's
  dot product the same way regardless of how many rows it is given.
- **"The graph is chaotic"**, stated unconditionally. True of the float32 reference,
  false of the half-precision path, which annihilates everything below 1e-04. The
  threshold is a property of the working precision, not of the graph alone.

## A bug this found

`xmx.gemm_mapped` skips re-uploading the right-hand operand when the caller passes the
same `b_key`. The key was `(b_key, buffer address)`, and two different weights can land
on the same address without the buffer having been reallocated — the second then
silently reused the first, giving a 1.39 relative error. It surfaced in a throwaway
benchmark that passed `b_key=1` for three different matrices. Now keyed on the operand's
identity and padded extent as well. `nr_xmx` was never exposed: its keys are
`(data pointer, shape, strides)` and it holds the weights alive, so no address is reused.
