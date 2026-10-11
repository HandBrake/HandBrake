# Where the frame's time goes on Windows, and the one lever that is not closed

Measured on the B580 / Ryzen 5 7500F machine, with the branch at `9e806ea` - which includes
the gcc-parity OpenMP work from the review. Everything below is a number from this machine;
nothing is extrapolated.

## The split, at 1280x720

Driving the daemon's own path with the graph timed once, so the host stages are attributable:

| stage | ms | share |
| --- | ---: | ---: |
| `build_features` (host, native C) | **12-14** | 20 % |
| `run_features` (the graph, on the GPU) | 50-53 | 77 % |
| `compose_encode` (host, native C) | 3.2 | 5 % |

Ranges are across repeated runs on the same machine; the ratio is what matters and it is
stable.

The graph is still the majority, as master's notes say. But the host side is no longer
uniform: one pass is twenty times the other, and it is the one nobody has looked at since it
was ported to C.

## The graph: no easy lever left

Per-pass profile at 1280x720, 318 GEMM dispatches:

```
gemm staged (flags 0)    27.75 ms   68.1 %
row: window block         5.52      13.6 %
ffn fused                 4.06      10.0 %
row: window attention     2.42       5.9 %
everything else           0.98       2.4 %
```

The staged GEMM at the graph's real shapes measures **5.5-15 TFLOP/s** across the four
largest call sites. Master records the same kernel at **3.5-3.8 TFLOP/s** on the Arc 140V and
the machine's FP16 peak at **~32 TFLOP/s**, and explicitly says those older figures "do not
establish the optimized kernel's ceiling". The levers master has already closed and measured:
register tiling, operand staging, integer weights, the accumulator format, OpenCL,
shared-memory bank padding, a 256-register mode (**~6 %** on big shapes, not built), the
small-M kernel (not tried, needs a new kernel), and E4M3 weights (~0.14 ms a frame). Nothing
here is a free win, and the reviewer's own gcc build - which reaches the same head bit for bit
- is the fair reference for the rest.

## The lever that is open: `half()` on MSVC

`build_features` is **13.70 ms** and it is not memory-bound. The native pass writes 66 MB and
reads 11 MB at the network's 1344x768 extent, and achieves:

| | ms | total traffic | achieved | machine ceiling |
| --- | ---: | ---: | ---: | ---: |
| features, no history | 5.4-5.7 | 77 MB | 13.5-14.2 GB/s | 70-91 GB/s |
| features, **with history** | **10.7-10.9** | 77 MB | **7.1-7.2 GB/s** | 70-91 GB/s |

**Seven to fourteen GB/s of a seventy-to-ninety ceiling is roughly 10 % of what the memory
system can do.** The pass scales linearly with OpenMP threads (1→12 threads took the fused
pass from 18.1 to 4.1 ms), so it is not short of cores. It is short of instructions.

The cause is in the file, and it is a consequence of the port rather than of the algorithm:

```c
#if defined(_MSC_VER)
typedef uint16_t nr_half;
static inline nr_half nr_half_of(float x) { return (nr_half)half_bits(x); }
static inline float nr_half_to_float(nr_half h) { return half_from_bits((uint32_t)h); }
#else
typedef _Float16 nr_half;      /* one instruction, GCC and Clang */
```

MSVC has no `_Float16`, so on Windows **every** `half()` is a hand-written conversion: an
if/else chain for NaN, infinity, normal and subnormal in one direction, and a `while` loop
that shifts the mantissa up for subnormals in the other. `feature_pixel` calls it **nine
times per pixel without history and eighteen with** - three roundings per channel, twice over
- which is why the history path doubles the pass.

That the conversion, not the loop, is the cost is shown by the two-spelling microbenchmark on
the same work (4 M values, three roundings each):

```
slow (half_bits/half_from_bits)   32-36 ms
fast (F16C _mm_cvtps_ph/_mm_cvtph_ps)   4-8 ms
speedup 4.5-8x, values differing 0 of 4000000
```

Both spellings were verified to agree on every value before timing, which is the same
requirement `test_native_image.py` enforces on the outputs.

## What this would take, and what it would not

The change is confined to the `_MSC_VER` arm of the half typedef: `_mm_cvtps_ph` and
`_mm_cvtph_ps` from F16C, one instruction each way, behind `/arch:AVX` or an F16C check.
It is not free of care:

- **F16C is not baseline x86-64.** It is Ivy Bridge (2012) and Piledriver onward, so a
  portable build needs a runtime check - `IsProcessorFeaturePresent` or `__cpuid` - and a
  fallback to the current arithmetic, or a build that guarantees the target. Both machines in
  play (this one, and the reviewer's Lunar Lake) have it, but the code should not assume it.
- The conversion is on the **bit-exactness** path this file exists to protect, so the check is
  `test_native_image.py` staying 197/197 byte-identical, not a visual comparison.
- The win is bounded by the pass, and the pass is 20 % of the host side and 13.7 ms of the
  ~69 ms measured here - so the ceiling on the saving is a few milliseconds a frame at 720p,
  not a step change. It is worth having because it is cheap and it is the only measured
  headroom left that is not already closed.

## What I did not do

I did not implement it. It changes the arithmetic path of the one file whose contract is
byte-identity with NumPy, it needs a CPU-feature check to be correct on a machine without
F16C, and this branch is in review with a reviewer who can time it against the gcc build on
the same laptop. The numbers above are what that decision should rest on.

## Done since, on the Arc 140V (2026-10-07)

With `x86-64-v3` the floor of every build, it went in without a runtime check: `build_win.bat`
defines `NR_F16C`, and `nr_image.c`'s MSVC arm converts with `vcvtps2ph` and `vcvtph2ps` on
lane 0, since MSVC declares only their vector forms. For every one of the 2^32 floats the
half's bits and the rounded value are the arithmetic's, and `test_native_image.py` (353
checks now) stays byte-identical.

`daemon_stages.py` at 1280x720 (0.3 and 0.5) and 1920x1080 (0.3), fresh and held frames, in
alternation with the old build on a Lunar Lake laptop: `features` goes from 1.0-2.9 ms to
0.4-0.8, within 0.1-0.3 of gcc's, and the daemon's own time a frame drops by 0.6-2.0 ms, most
on held frames. Not `/arch:AVX2`: MSVC takes the intrinsics without it, and under it the
composition, which MSVC vectorises in neither build, was 0.1-0.4 ms slower. The composition is
where MSVC still trails gcc's vectorised one, by 0.5-1.8 ms.
