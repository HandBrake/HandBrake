# The attention IS a softmax — `notes/phase5-no-softmax.md` is withdrawn
2026-09-08

## What the previous note claimed, and why it was wrong

`phase5-no-softmax.md` recorded **zero `ex2` in any attention kernel across all 231**
and concluded there is no exponential in the attention path, so the attention must be
some normalise-by-sum form rather than a softmax. The opcode census was correct. The
conclusion was not.

`ex2` is absent because **the exponential is hand-rolled in `f16x2`**, two lanes at a
time, out of `fma` / `max` / `min` and a shift-and-add on the *bit pattern*. PTX's
`ex2.approx` is f32 and scalar; this version is SIMD and needs no special function
unit at all.

The reason it was invisible: **`src/tools/ptx_trace.py` was dropping every statement
inside a `{ ... }` scope.** NVCC wraps hand-written f16x2 arithmetic in braces —

```
{fma.rn.f16x2 %r647,%r644,%r645,%r646;
}
```

— and the parser skipped any line starting with `{`. In `cc_vit_attention` that hid
312 of 2,690 instructions, and they were not a random 312: they were the entire
elementwise path between the two `mma`s. Across all 231 kernels the census had been
missing **26,124 `fma.rn.f16x2`, 48,592 `mul.f16x2`, 24,380 `add.f16x2`, 19,300
`max.f16x2`, 17,100 `min.f16x2`**.

The tell was mechanical, not visual: tracing forward from the first `mma`'s output
reached nothing, and *no* `mma` in the kernel took another `mma`'s result as an A or B
operand. In a fused attention kernel that is impossible. After the fix, **128 of 128**
`mma`s chain.

This is the fourth wrong conclusion in this project from a tool or an eye skipping
part of the input, and the second from this specific file. `ptx_trace.py` now splits
on `;` and descends into scopes, renaming scope-local `.reg` names (`low`, `hl`, `fl`)
so they join the dataflow graph.

## What the attention actually is

```
S    = Q @ K^T                                   mma, f16 accumulate
t    = clamp(S * a + b, lo, hi)                  fma.f16x2, max.f16x2, min.f16x2
P    = bitcast_f16( (bits(t) - 0x3C00) << sh )   shl.b32 + a compensating add.s32
sum  = SUM(P) - n_pad * P_pad                    add.f16x2 tree, padding correction
out  = (P @ V) * 1/max(sum, eps)                 mma, then a hand-rolled rcp.f16x2
```

`t` is clamped inside `[1, 2)`, where an f16's bit pattern is *linear* in its value,
so `(bits - 0x3C00) << sh` is the standard mantissa-linear approximation of `2^x`.
The 32-bit shift crosses the two f16 lanes; the add constant is chosen so the carries
put both lanes back exactly. Verified two ways: a per-element formula is **bit-exact**
against a full 32-bit two-lane emulation, and lane independence holds for every input
pair tested.

Two variants exist across the 231 kernels and no others:

```
family     kernels  a            b           clamp on t          shl  add          S clamped to   exp arg mult
Swin/post    114    0.044921875  1.30078125  [1.03125, 1.56934]   5   0x7FF88000   [-6.00, +5.98]   0.9964
ViT           12    0.089538574  1.70898438  [1.43945, 1.97754]   4   0x3FFC4000   [-3.01, +3.00]   0.9930
```

The multiplier is within 0.7 % of natural `exp`; the residual is a deliberate fit —
lowering the slope slightly re-centres the error of the mantissa linearisation, which
otherwise biases low. Over the clamped range the result stays within
**[0.92, 1.04] of `exp()`**. That ripple does not matter.

## The part that does matter

**There is no max subtraction.** The clamp is what keeps the exponential in range.
`max.f16x2` occurs 66 times in `cc_vit_attention`: 65 are the low half of the clamp
and one is the epsilon floor on the denominator. There is no row-max reduction
anywhere.

So this softmax is **not shift-invariant**, and two things follow that were previously
argued the other way:

1. **The absolute size of the logits decides how sharp the attention is.** A softmax
   with max subtraction only cares about differences; this one does not. The `+-6`
   clamp is a real semantic limit — the sharpest attention the Swin blocks can express
   is a ratio of `e^12` across a window.

2. **`notes/phase3-bias-region.md`'s central argument is void.** It reads: "Softmax is
   shift-invariant, so the -56.23 constant has no effect whatsoever; only the 0.86 of
   variation can matter." Under the real function a constant of -56 drives every logit
   far below the -6 clamp, every entry saturates to the same value, and the attention
   becomes exactly uniform. Measured on real weights: adding the `128C` region to the
   logits *raises* row entropy toward the 4.159-nat uniform maximum rather than
   sharpening anything. It cannot be a pre-softmax additive bias. That region's role
   is still open, but one more reading is now ruled out rather than merely unsupported.

`src/ref/hnet_ops.py` gains `ptx_exp()` and `ptx_softmax()`; the textbook `softmax()`
stays for comparison and is reachable with `run_frame.py --true-softmax`.

## The QK normalisation, re-derived properly

The old attribution ("32 `rsqrt` = 2 per head for 16 heads") was arithmetic
coincidence — `cc_tinlayout_fused_swin_1h_32_1` has 1 head and also 32 `rsqrt`. Traced
mechanically instead, the chain is

```
mma -> mul.f16x2 (square) x4 -> add.f16x2 tree -> shfl.sync.bfly.b32 x2 (cross-lane)
    -> max.f16x2 (epsilon) -> rsqrt.f16x2 (hand-rolled) -> mul.f16x2 -> mma as operand B
```

Four lanes x eight values = 32 = the head dimension, and the normalised result enters
the attention `mma` as **B**, which in `row.col` is K. So it is per-head L2
normalisation of Q and K. The conclusion survives; the reasoning behind it did not.

## `attn_scale` is probably stored as a log — HYPOTHESIS

Not proven, and marked as such in the code. The evidence:

- Used raw, `attn_scale` (1.11-2.77) puts the logits at `+-0.96` to `+-1.55`, and every
  attention row comes out **uniform to within 0.015 nats of the 4.159 maximum**. The
  attention would be doing nothing whatsoever.
- Exponentiated, the logits reach `+-3.2` to `+-8.8` — straddling the `+-6` clamp the
  kernels implement. A clamp exists because the thing clamped can exceed it.
- Entropy then falls to 3.84-4.11: structured, not uniform.
- Swin-V2 stores exactly this parameter as a log (`logit_scale`, initialised to
  `log 10 = 2.303`); our values cluster around it.

Reachable with `run_frame.py --exp-scale`. It cannot yet be confirmed end-to-end,
for the reason in `phase5-visual-loop.md`: nothing in the branch is visible in the
score.
