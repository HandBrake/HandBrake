# Phase 50 — the shipped model is already a third FP8

2026-09-11. Asked what a reported NVIDIA move to FP4 would mean for this port. The
question is answerable from the binary we already have, so it was, before answering.

## Counted, not assumed

Every `mma` and `cvt` in the 231 carved PTX kernels (`work/modules/*.ptx`):

| instruction | count |
| --- | --- |
| `mma.sync.aligned.MNK.row.col.f16.f16.f16.f16` | 35 072 |
| `mma.sync.aligned.MNK.row.col.f16.e4m3.e4m3.f16` | **19 728** |
| `cvt.rn.satfinite.e4m3x2.f16x2` | 24 736 |
| `cvt.rn.f16x2.e4m3x2` | 4 744 |
| `e5m2`, `e2m1`, `e3m0`, `ue8m0`, `mxf4`, `mxf8f6f4` | **0** |

**36 % of the matrix instructions already take E4M3 operands** — FP8 feeding `mma`, not
FP8 in storage — and every accumulator is FP16, which `phase4-accumulation-choice.md`
established from the other direction. There is no FP4 of any kind in this build.

So a move to FP4 would not be a jump from FP16. It is the next step on a road the model
is already a third of the way down.

## What it would cost us

Nothing in speed, and that is not a good thing. Our own probe found six cooperative-matrix
configs on this hardware, all M=8 N=16, fp16/bf16/int8 (`hw-coopmat.md`): **XMX has no FP8
and no FP4**, and Xe3 does not add them. It has INT4 in silicon, Vulkan does not expose it,
and INT4 is not FP4 anyway. We would decode FP4 to FP16 at load and run exactly as now —
216 ms of register-bound GEMM and 272 ms of memory-bound passes (`phase45`).

Nothing in memory either: 141 MiB of weights against an 11.46 GiB heap was never the
constraint, and after expansion to FP16 the footprint is what it is today.

**The real cost is re-derivation.** FP4 is never used unscaled — E2M1 has eight magnitudes
— so it ships block-scaled, NVFP4 with an E4M3 scale per 16 values or MXFP4 with E8M0 per
32. A new container, new scales, possibly a new graph. This project spent its first days
measuring scaffolding because the container was read as dense FP16 and correlated **-0.02**
with the truth (`phase6`). MLX-DLSS, whose extraction ours stands on, would have to redo
theirs first.

**And the thing actually worth worrying about is not the format.** If the vendor spends the
saved budget on a wider model, our cost follows the arithmetic — `17 ms + 488 ms per
megapixel` — and FP4 gives us nothing back, because we compute in FP16 regardless.

Accuracy is the one place we come out ahead and stay there: zero of 218 vendor kernels use
an FP32 accumulator and ours does, so a model quantised further and trained for it would
still be run more precisely here than on the hardware it was built for.

**Reported, not verified.** The FP4 plan is hearsay. What is verified is the table above.
