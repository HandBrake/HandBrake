# NVIDIA accumulates in FP16, not FP32 — and that changes the Phase 4 decision
2026-09-08

## The census

Every `mma` instruction across all 231 extracted PTX kernels:

```
mma.sync.aligned.m16n8k16   f16.f16.f16.f16      x 35,072
mma.sync.aligned.m16n8k32   f16.e4m3.e4m3.f16    x 19,728
```

PTX orders the types `D.A.B.C`, so the accumulator is the last field. **Zero of 218
kernels accumulate in FP32.** The plain path is FP16 throughout; the `_fp8` path takes
E4M3 operands and still accumulates in FP16.

That also pins down the FP8 story precisely: DLSS-NR quantises *operands* to E4M3 at
runtime on Blackwell, keeps an FP16 accumulator, and stores its weights as FP16.
Three different formats, three different roles.

## This corrects an earlier conclusion of mine

`notes/phase4-subnormal-flush.md` says "Phase 4 must use config 1" on the strength of
a measurement: FP32 accumulation is 284x more accurate than FP16 on real weights.
That measurement stands. The *conclusion* was too strong, because it was made without
knowing what the original does — and the original does the less accurate thing.

Both configurations run here. Measured on the same inputs:

```
shape             fp32-acc      fp16-acc      ratio
64x64   K=64      1.513e-06     6.830e-04      451x
64x128  K=256     2.700e-06     1.090e-03      404x
128x256 K=512     4.071e-06     1.787e-03      439x
64x512  K=1024    2.955e-06     2.434e-03      824x
```

## So which one

It depends on what "correct" means here, and the two answers differ:

- **Config 1, `fp16 x fp16 -> fp32`** — 400-800x more accurate than NVIDIA. Our frame
  would be *better* than DLSS-NR's, and would not bit-match it.
- **Config 0, `fp16 x fp16 -> fp16`** — reproduces NVIDIA's numerics. The only option
  if we ever want to check against activations captured from real hardware, which is
  the one external oracle this project can reach (the AMD lab's RTX captures).

notes/CLAUDE.md's success criterion is "matches a CPU reference", and that reference is ours
to define — so config 1 is the sensible default and stays the default. But the FP16
path is now built and tested (`src/gpu/gemm_coopmat_f16acc.comp`,
`work/gemm_f16acc.spv`) and should be kept, because it is the only way to compare
against anything NVIDIA-derived. Which one is *wanted* is the owner's call, not mine.

Worth noting for Phase 3 as well: a CPU reference that accumulates in FP32 while the
original accumulates in FP16 will diverge from NVIDIA by ~1e-3 relative per layer,
compounding over 71 blocks. If matching the original ever becomes the goal, the CPU
reference has to change too, not just the shader.
