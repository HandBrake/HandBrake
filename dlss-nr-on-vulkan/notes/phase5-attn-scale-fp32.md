# `attn_scale` is FP32, and `unknown_even` is its low half
2026-09-08

## What was assumed

`phase3-first-operator.md` located the per-head attention scale as "the odd slots of
the `2H` run, one per head", values in a tight 1.1-2.2 band, and recorded the even
slots as `unknown_even` — "not identified; kept so nothing is lost".

Both halves are one FP32 number. The run is **H FP32 scalars, not 2H FP16 ones.**

## The evidence, in order of strength

**1. The even slots cannot be trained parameters.** Across the eight ViT-1D blocks
they take exactly **eight distinct values** — 0, ±2, ±512, ±0.0078125 and so on — and
every one is a multiple of `0x2000`, i.e. the low 13 bits are zero. That is precisely
an FP16 widened to FP32: 23 mantissa bits, 10 of them used, 13 zero. Across the
sixteen Swin blocks the same slots hold NaNs and magnitudes above 1e4.

```
                    words that are multiples of 0x2000
real weight tensor (wq)          0.00015      <- i.e. random, 1/8192
even slots, ViT-1D blocks 31-38  1.00
even slots, Swin blocks 23-30,40-47  0.00      <- full FP32 precision instead
odd slots (the "scale")          0.002
```

A pooled figure of exactly 0.50 first looked like an artefact. It is not: 8 ViT blocks
x 32 heads = 256 slots and 16 Swin blocks x 16 heads = 256 slots, so the two families
happen to contribute equally. Splitting by block makes it 1.00 / 0.00 — a clean
family split, not a coincidence.

**2. The kernel reads four bytes and calls them FP32.** In
`cc_split_swin_16h_qkv_512`, tracing the A operand of the `S = Q.K^T` mma backwards:

```
Q from the qkv mma -> mul.f16x2 (square) -> add.f16x2 tree -> max.f16x2 (eps)
                   -> rsqrt -> mul.f16x2                       = q / ||q||
ld.global.b32  %r739, [%rd72+1703936]                          = the scalar
cvt.rn.f16.f32 -> broadcast -> mul.f16x2                       = q_hat * scale
                                                               -> mma operand A
```

`ld.global.b32` then `cvt.rn.f16.f32`: four bytes in memory, read as FP32, narrowed to
FP16 in register. And the address is `param_0+16` plus `4 * (4*ctaid.z + tid.y)` —
**stride four, indexed per head.**

The kernel parameter block also falls out of this:

```
param_0+0    input activations        (v4 loads)
param_0+8    output                   (st.global)
param_0+16   the weight/scalar arena  (v4 weight tiles AND b32 FP32 scalars)
param_0+24   two u32, +32 two u32     dimensions
```

`cc_tinlayout_fused_swin_8h_256_8` reads **65** such FP32 scalars from the arena; the
split-Swin QKV kernel reads 2. This is the "launch parameter block" that
`phase3-block-internals.md` called not recoverable from PTX. It is recoverable, and it
is not a separate host structure — the scalars sit in the same arena as the weights.

**3. Only the FP32 reading makes the hardware clamp mean anything.** The kernels clamp
attention logits at +-6 (`phase5-softmax-found.md`). With Q and K unit-normalised the
logit is `scale * cos`, so `|logit| <= scale`:

```
blk   scale as f16 odd slots   as FP32 pairs    |logit| max   row entropy (max 4.159)
23         1.11 - 2.06           0.015 - 2.45       0.83        4.154 -> 4.158
27         2.53 - 2.94           8.87 - 28.44      10.24        4.148 -> 3.795
30         1.53 - 2.76           0.15 - 16.66       9.00        4.144 -> 3.991
40         2.48 - 2.77           7.60 - 17.56       8.05        4.149 -> 3.954
```

Under the FP16 reading no logit anywhere in the model ever reaches 6, every attention
row is uniform to within 0.015 of the 4.159-nat maximum, and the clamp is dead code.
Under the FP32 reading logits pass through the clamp and the attention becomes
structured. A clamp exists because the thing clamped can exceed it.

Swin-V2 initialises cosine-attention `logit_scale` to 10 and clamps it at 100. The
recovered values, 0.015 to 28, sit inside that.

## The earlier hypothesis this replaces

Two hours before this, `--exp-scale` proposed that `attn_scale` was stored as a *log*,
Swin-V2 style, because raw it left the attention uniform and `exp()` put it in the
clamp's range. That reasoning pointed at the right symptom and the wrong mechanism.
There is no exponential: the number is simply wider than it was being read. Both
readings survive in the code (`--scale-f16`, `--exp-scale`) so the comparison stays
reproducible, and the FP32 pairing is the default.

## What it does not fix

Nothing end to end. `run_frame.py --skip-rms` still scores 1.43x with the network
moving the frame by 0.35 %, identical to four decimals under all three readings,
because the branch is inert for reasons that have nothing to do with attention
sharpness (`phase5-visual-loop.md`). This is a layer-level correction, verified at
layer level, and it should be treated that way until the branch problem is solved.

## Worth carrying forward

The same encoding may explain other leftovers where a "parameter" does not look like
one: ViT-1D `layer3`'s single scalar (`0.109, -0.248, 5.0e-05, ..., -558.5` — the last
wildly out of family), the `+8` in ViT-1D `layer0`, and the 24 trailing values in each
fused-Swin block. **Before calling a small run of values a parameter, check whether it
is half of a wider one.** The test is cheap: real FP16 weights use their low mantissa
bits, and 0.015 % of their words are multiples of 0x2000.
