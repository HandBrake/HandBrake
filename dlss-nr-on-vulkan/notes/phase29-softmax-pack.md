# Phase 29 — native softmax packing and another measured GEMM sweep

2026-09-10. Continued from phase28's captured graph. One small optimization is
retained; broader GEMM changes did not establish a benefit.

## Retained: native FP16 packing

The bit-affine softmax manually reconstructed two half encodings from float32 and
manually decoded the transformed pair. `packHalf2x16` and `unpackHalf2x16` perform
these conversions directly. All preceding half-rounding points, the coupled 32-bit
shift/add (including carry between halves), summation order and E4M3 publication
remain unchanged.

This requires checking the decoder's domain: the old decoder does not implement
IEEE infinity/NaN semantics for exponent 31. Exhausting all **304,704** possible
pairs after affine clamping proves both transformed halves lie in **0x0400..0x48e0**,
so they are finite, normal FP16 values and native decoding is equivalent. This is
a finite-logit contract; no new guarantee about NaN inputs is made.

The old function remains behind `SOFTMAX_AB`, solely for the test shader. Production
`attention.spv` contains only the native implementation. No new runtime pipeline
family, cache dimension or Python work is introduced.

## Measurements and limits

All full-frame comparisons alternate order on the same buffers, keep specialization
and captured replay enabled, exclude initial allocation/compilation/capture, and
require bit-identical heads before and after changing input. Input is the local
Cyberpunk screenshot resized to each output size. Eight paired rounds per size:

| Measurement | Previous softmax | Native packing | Reduction |
|---|---:|---:|---:|
| Isolated softmax, 4 calls, 983040 rows of 64 | 45.54 ms | 39.30 ms | 13.7% |
| Full 384x384 graph | 89.16 ms | 87.43 ms | 1.9% |
| Full 1280x720 graph | 520.97 ms | 515.39 ms | 1.1% |
| Full 1920x1080 graph | 1147.71 ms | 1148.55 ms | no measurable gain |

**The kernel gain is not a 14% frame gain.** The full-frame improvement is small and
1080p is unchanged within noise. These absolute timings should not be subtracted
from another process's phase28 results to infer a regression or improvement.
The 384 head hash still matches the original GPU result recorded in phase27:
`8c4f8a43bf38e358c13e8016b7a6bcae035258dd4c87bc7c8337a1920539ab7f`.

```sh
make work/attention_ab.spv
python3 src/bench/softmax_pack.py --size 720 1280 --input INPUT.png --pairs 8 --json work/softmax.json
```

The reference warmup happens first and includes shared allocation/upload work; it
must not be compared with the native warmup as a cold-start benchmark.

## Investigated, not enabled

Register-tile screening used six complete 720p runs with five warm samples each.
Every head matched the old SHA-256. These are separate processes, so small changes
cannot establish a win; no candidate earned a same-buffer production A/B rollout.

| Tile per subgroup | Median frame, ms |
|---|---:|
| 16x32 (current) | 515.6 |
| 32x32 | 547.4 |
| 32x16 | 543.4 |
| 16x64 | 555.7 |
| 8x32 | 533.6 |
| 16x16 | 516.9 |

Transposed static weights were tested more directly: two captured graphs share
activation/output buffers and hold both weight representations, alternating for
six pairs. All heads match, but transposed reads give **598.8 ms vs 521.8 ms**.
This candidate is not shipped. Neither experiment proves an immutable XMX ceiling.

## Profiling and benchmark repair

The diagnostic stage pass still identifies high-resolution work as substantial:
block70/head 77.8 ms, block0/pool 68.7 ms, and six C=32 encoder/decoder blocks 94.6 ms,
out of a 542 ms diagnostic frame. Stage capture uses the original block submission
mode, so these are a guide to distribution, not timestamps from captured replay.

`split_cost.py` monkeypatches dispatch recording to skip GEMMs or other operations.
After phase28, cached replay could silently ignore those patches. It now explicitly
records each modified graph (`execution="single"`) and restores methods in `finally`.
The repaired approximate ablation gives 514 ms whole, 221 ms GEMMs-only, 300 ms other
operations-only. These runs change intermediate values and include CPU recording;
they are not an exact additive hardware profile. The next useful target is attention
layout/conversion traffic, including opportunities to combine Q/K splitting with
cosine normalization. Actual GPU timestamps would improve attribution.

## Validation and evidence

`make test` passes with local logical weights present. The new softmax test covers
all finite FP16 logits, bias/clamp, staged and wider/padded rows, both output widths,
generic/specialized pipelines, and guard regions (16 GPU cases). The existing CPU
reference and complete resident graph regressions still pass with correlation
0.981311. Optional PyTorch remains unavailable in the system environment.

Local raw evidence lives under `work/perf29/`: `screen.jsonl`, `transposed.log`,
`stages.txt`, `split.txt`, `ew.txt`, `softmax-kernel.json`, `softmax-{384,720,1080}.json`,
`softmax-*.log`, and `tests.log`. Experimental shaders and scripts are also retained
there; only native packing, its reproducible A/B/test support and the benchmark fix
are added to the source tree.
