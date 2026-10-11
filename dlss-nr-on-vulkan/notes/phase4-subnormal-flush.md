# Phase 4 — the GPU path runs, and it found a real hardware trap
2026-09-08

> **Corrected 2026-10-01** (`phase71`): the flush measured here is real on Mesa, but it is
> Mesa's default float-controls mode, not the XMX units'. Our shaders declare no denorm mode,
> so `cr0`'s float16 denorm bit stays clear and the multiply flushes. The same units keep
> float16 subnormals under Intel's Windows driver, and `DenormFlushToZero 16` declared there
> reproduces Mesa's stem bit for bit. libxmx has declared `DenormPreserve 16` since, so Mesa
> keeps them too. The 27 % below was withdrawn earlier: it counted the misread decode, and the
> real weights hold 7 subnormals (`phase61`).

## The path works

`src/gpu/gemm_coopmat.comp` + `src/gpu/gemm_runner.c` execute an FP16 x FP16 -> FP32
GEMM on the Xe2 XMX units through `VK_KHR_cooperative_matrix`, shape 8x16x16
(configuration 1, the only float config on this hardware that accumulates wider than
it multiplies). Checked against numpy on synthetic data:

```
   8x16   K=16    rel 3.86e-08        64x64   K=256   rel 7.28e-07
   8x16   K=64    rel 5.63e-08       128x256 K=512   rel 1.79e-06
  16x32   K=64    rel 6.53e-08       256x512 K=512   rel 2.36e-06
```

Error grows as sqrt(K), exactly as FP32 accumulation of FP16 products should.

## Then the real weights failed

The same kernel on NVIDIA's actual parameters was off by 5e-04 absolute — three
orders worse — and the error touched 94% of output elements. Not precision, and not
a handful of outliers: something systematic.

**Xe2's XMX units flush subnormal FP16 operands to zero.** Modelling that on the CPU
reproduces the GPU bit-for-bit:

```
tensor                vs exact      vs FTZ model   subnormal
block23.wq            5.523e-04     2.043e-08      12.24 %
block23.proj          5.563e-04     8.966e-08       9.64 %
block23.ffwd          7.283e-04     1.084e-08      18.48 %
block31.wq            1.091e-03     2.296e-08      17.55 %
block39.weight        7.373e-04     5.408e-09      20.46 %
```

The flush-to-zero model is 25,000x closer than the exact model. There is no doubt.

## How much of the model this eats

```
20,102,809 of 73,841,889 parameters are FP16 subnormals   -> 27.22 %
```

Across the 280 weight matrices: median **9.64 %** subnormal, worst **82.92 %**, and
**20 matrices are more than half subnormal**. The worst are the decoder's
`ffwd_proj`:

```
block42.ffwd_proj (512,256)   82.92 %       block46.ffwd_proj (512,256)   77.89 %
block43.ffwd_proj (512,256)   81.81 %       block25.ffwd_proj (512,256)   76.05 %
block44.ffwd_proj (512,256)   78.96 %       block41.ffwd_proj (512,256)   75.31 %
```

Run naively, **more than a quarter of DLSS-NR silently evaluates to zero on this
hardware**, with four fifths of some matrices gone. Nothing would crash; the output
would just be quietly wrong. This is precisely the class of bug a layer-by-layer
reference is meant to catch, and it was invisible until real weights met real silicon.

## The fix, verified

Scale each weight matrix offline by the largest `2^k` that keeps it inside the FP16
normal range, then divide the FP32 accumulator by `2^k` afterwards. A power of two is
exact in binary floating point, so **the rescale contributes no error of its own**.

```
tensor                2^k   subnormal before   after    rel error after
block23.wq            21    12.24 %            0.00 %   5.198e-06
block23.proj          18     9.64 %            0.00 %   3.747e-06
block23.ffwd          21    18.48 %            0.00 %   3.480e-06
block31.wq            20    17.55 %            0.00 %   5.395e-06
block31.ffn_contract  23    66.68 %            0.00 %   6.199e-06
block39.weight        22    20.46 %            0.00 %   9.095e-06
```

Every subnormal is gone and the residual error is back to the ~5e-06 of the synthetic
test — i.e. ordinary FP32 accumulation error, nothing more. Required shifts run from
`2^12` to `2^23`, median `2^20`, so the whole model fits comfortably; FP16's exponent
range has ample headroom above these weights.

`fp16_shift()` and `subnormal_fraction()` are now in `src/ref/hnet_model.py`.

## Why this matters beyond the bug

notes/CLAUDE.md's original premise was that BF16 would be the working type. BF16 has the
same 8-bit exponent as FP32 and would never have hit this. FP16's 5-bit exponent is
what makes a quarter of these weights subnormal — the cost of the format the model
actually ships in. The fix is cheap and exact, but it has to be *there*, and it has to
be applied per tensor.

---

## Layer-level validation on hardware (2026-09-08)

`src/gpu/xmx.py` wraps the kernel with the power-of-two rescale and tile padding;
`src/gpu/test_attention_gpu.py` runs complete layers through it and compares against
the pure-CPU reference. Twenty layer evaluations on real weights, all passing:

```
split-Swin-16H attention   blocks 23,26,29,40,44,47    rel 2.6e-05 .. 1.1e-04
ViT-1D attention           blocks 31,34,38             rel 6.7e-05 .. 2.8e-04
ViT-1D feed-forward        blocks 31,34,38             rel 7.4e-05 .. 1.7e-04
fused Swin attention       blocks 9,12,15,21,49,55,57,61  rel 2.3e-07 .. 7.1e-07
```

Every projection — Q, K, V and the output — executes on XMX; QK-normalisation,
softmax and the bias add stay on the CPU for now. Worst deviation anywhere: 2.8e-04.

The attention layers deviate more than the raw GEMM (5e-06) because the QK
normalisation and softmax amplify small differences in the logits. The fused Swin
path deviates *less* (1e-07) because it has one fewer GEMM in the chain — its output
projection is not in the resolved region.

## A limit found by testing: the qkv split does not hold at C = 32 and C = 64

GQA 4:1 requires K and V to be a whole number of 32-channel heads. `C/4/32` gives
1 head at C=128, 2 at C=256, 4 at C=512 — but **half a head at C=64 and a quarter at
C=32**. The `1.5C^2` reading is arithmetically fine at every width; the *split into
Q, K, V* is not. Those two narrow stages must use a different qkv structure, and it
is not yet determined. This affects 15 of the 45 fused blocks (blocks 0-8, 63-70).

The validation above is restricted to C >= 128 for that reason, which is stated in
the test rather than hidden by it.
