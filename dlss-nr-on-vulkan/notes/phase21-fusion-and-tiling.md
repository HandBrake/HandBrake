# Phase 21 — the frame at 613 ms, and what the microbenchmarks got wrong

2026-09-09. 720p network extent 1280x768, correlation with the host reference
**unchanged at 0.981311 through every change below**. Nothing here trades accuracy
for speed; each step is either bit-exact by construction or was checked to be.

```
1025 ms  the frame as phase 20 left it
 887     hardware half-round, GEMM epilogue
 819     publish where the value is produced, row passes staged through shared memory
 788     the attention bias folded into the softmax
 717     a 16x32 register block in the GEMM
 677     the window reverse folded into the residual, Q/K narrowed at the split
 613     independent dispatches overlapping
```

**1.67x**, and 1920x1080 renders for the first time — it used to die in
`vkAllocateMemory` before the second block. 384x384 goes 180 -> **126 ms**.

**On the numbers.** Every figure here is the best of five or more consecutive frames
inside one process. Within a process the frames are steady to about 2 %; *between*
processes the same binary spreads **586-718 ms**, ten percent either side of a ~625 ms
median, and each process is internally consistent at its own level — so it is buffer
placement, not clocks or heat. The GPU holds 1950 MHz throughout at 43-45 C. The first
frame after an idle costs two to four times the rest while the clock ramps from 550
MHz, which is also why the first version of `src/bench/replay.py` read a third low
until it was made to warm up.

Read the progression as a direction with about 10 % of slack on each step. Where a step
mattered it was A/B'd inside one process against its immediate predecessor — the
register-block comparison below is the clearest example, and the two rejected
optimisations were both measured that way.

## Where the time actually was

Phase 18 recorded the frame as "dispatch overhead and poor occupancy on the many
small shapes", from a traffic estimate of about 10 GB. That estimate was low by six
times. Counting the passes as they are recorded:

| | per 720p frame |
|---|---|
| GEMM arithmetic | 459.6 GFLOP over 1036 dispatches |
| GEMM traffic | 16.9 GB |
| elementwise elements touched | **6.10 G**, over 1594 dispatches |
| elementwise traffic | ~49 GB |

So the frame moved about 65 GB, and at the 90 GB/s the GPU reaches that is a 720 ms
floor against a 1025 ms measurement. It was bandwidth-bound, and the elementwise
half — not the GEMMs — was three quarters of it. `src/bench/census.py` and
`src/bench/census_ew.py` produce those tables; `replay.py` re-runs the GEMM census on the
device, `src/bench/ew_rate.py` measures each elementwise pass at block 0's size.

A second correction to phase 18's arithmetic: the per-dispatch fixed cost is **4.5 us**
(`src/bench/overhead.py`), so 2000 dispatches is 9 ms. Dispatch overhead was never
the problem.

## The coopmat epilogue was not blocked after all

Phase 18 established that **any** operation on a cooperative matrix between
`coopMatMulAdd` and `coopMatStore` scrambles where the elements land, and concluded
that a GEMM epilogue could not be written on this driver. That conclusion was one step
too far.

The bug is in the arithmetic, not in the store. An **untouched** accumulator reaches
*shared memory* intact, and the publish can then be done on ordinary scalars on the way
out to global memory. `src/gpu/test_epilogue.py` checks each epilogue — E4M3, the
quadratic gate, gate+E4M3, half — against the same chain run as a separate pass, on the
GPU's own GEMM output: **0.000e+00 maximum difference** in all six cases.

Two details decide whether it pays:

- **Four 16-bit stores are slower than the one float32 store they replace.** One
  64-bit `f16vec4` store per lane, four consecutive elements, moved the fused shapes
  from 1.15x to 1.4x. A narrow output is only cheaper if it is also wider per lane.
- **The gate's arithmetic stops being hidden.** In a separate pass it rides behind
  92 GB/s of traffic; in the epilogue there is no traffic left to hide it.

## `packHalf2x16` is the hardware half-round

`half_round` was ten instructions and two branches of hand-rolled exponent and mantissa
work, written that way because `float(float16_t(x))` is folded away by the compiler —
the bug that once made every vendor rounding point in this graph silently vanish.

`packHalf2x16` changes the bit representation, so it cannot be elided.
`src/bench/half_probe.py` runs all three side by side over 360 704 values — ordinary
magnitudes, half subnormals, values far below the subnormal range, the overflow range,
and the exact boundaries:

| | mismatches against numpy's float16 |
|---|---|
| bit-twiddled | 0 |
| `packHalf2x16` | **0** |
| `float(float16_t(x))` | 360 428 |

Two instructions, no branches, in all three shaders. Worth 7 % of a frame on its own,
and the folding bug is now documented by a live test rather than by memory.

## Publish where the value is produced

Every shader now reads one flag layout: the low byte is the kind, bits 8-11 an
epilogue, bit 12 a float16 output. A pass that ends in a publish applies it on the way
out instead of leaving a float32 buffer for a second dispatch to read straight back.
`partition`, `split_heads`, `merge_heads`, `pool2`, both row passes and the closing
residual all feed their consumer directly. **1594 -> 910 elementwise dispatches, 6.10 G
-> 3.03 G elements.**

Three of these are exact for a reason worth writing down:

- A permutation and an elementwise round commute, so V's E4M3 publish can happen
  inside the head split.
- The cosine publish rounds its input to half as its first act, so a producer that
  already wrote half gives it the value it would have computed from float32.
- An E4M3 value is exactly representable in half, so narrowing a published buffer
  loses nothing at all.

## The row passes were reading against the grain

That halving of the element count bought **almost nothing** on its own, which is what
sent the investigation somewhere better. Measuring each pass at block 0's size:

| pass | before | after |
|---|---|---|
| cosine publish | 23.7 GB/s, 3.9 Melem/ms | 48 GB/s, **8.1** |
| softmax | 56 GB/s, 5.6 Melem/ms | 54 GB/s, 5.4 |
| everything else | 53-136 GB/s | unchanged |

Both row passes read a whole row into one invocation, and a row is 32 or 64 consecutive
floats: neighbouring invocations then land 128 or 256 bytes apart and a single load
instruction spans sixty-four cache lines. Staging the workgroup's rows through shared
memory makes the global traffic contiguous while each invocation still sees its own row
whole — which the vendor's fragment tree and the bit-affine softmax both require, since
neither reduction may be reassociated. One subgroup per workgroup, so the barriers are
subgroup-scoped.

The softmax also stopped parking its float32 weights in the output buffer, which is now
narrowed; they go in shared memory instead, and only the bottleneck's too-wide rows pay
the bit-affine transform twice.

## The attention bias

426 M elements a frame of read-modify-write over the scores, to add a table of a few
tens of kilobytes that never leaves cache. It is now added to the logit inside the
softmax, where the scores are already being read. 5 GB of traffic for nothing.

## Register tiling, and a microbenchmark that lies

The GEMM now keeps a **16x32** block of the output in one subgroup's registers instead
of a single 8x16 tile: each K step loads 2 fragments of A and 2 of B to feed 4
multiply-accumulates, so loads per unit of arithmetic double and the workgroup count
falls fourfold. Shapes whose extents are not a whole block take the 8x16 kernel — the
same source compiled with `RM = RN = 1`.

The finding worth keeping is what happened at **32x32**:

| block | isolated GEMM census | in a frame |
|---|---|---|
| none (8x16) | 256 ms | 785 ms |
| 32x32 | **206 ms** | 815 ms |
| 16x32 | — | **707 ms** |

A 32x32 block is a quarter faster over the whole shape census and *slower in a frame*.
Halving the workgroup count again leaves these shapes without enough workgroups to fill
eight Xe cores, and the latency stops being hidden. 16x32 is where the two agree.
`XMX_TILE_M`, `XMX_TILE_N` and `XMX_TILE_K` exist so the trade can be re-measured.

Two more results from the same corner:

- Putting the transposed-B test **inside** the K loop, which the generalisation did by
  accident, cost 30 % of a frame — 787 -> 1056 ms. The loads stop being hoisted and the
  loop stops pipelining.
- **Software pipelining the K loop made it worse**: 619 -> 760 ms. Issuing the loads
  for step k+1 before the multiply-accumulates for step k doubles the fragment
  registers, and on this machine the pressure costs more than the latency it hides.

## Barriers

Every recorded dispatch was followed by a full pipeline barrier. Right for most of the
graph, wrong for the runs that are independent by construction: a branched
feed-forward's per-head GEMMs write disjoint slices of one buffer, the split family's
per-group GEMMs likewise, the three head splits read one buffer and write three, the two
cosine publishes are separate tensors, and a decoder transition's upsample and skip
scaling touch nothing in common. `runtime.independent()` records those without barriers
and closes the run with one. **677 -> 613 ms.**

## What did not pay

Recorded because the next person will be tempted:

- **Fusing a publish whose input is still in cache.** The `e4m3` that followed each
  block's closing residual: 1946 -> 1890 passes for 677 -> 686 ms, inside the noise.
  Every win came from a pass streaming a buffer larger than the 8 MB L3.
- **Software pipelining**, above.
- **32x32 register blocks**, above.

## Memory

Ten scratch buffers had no reader left once the publishes moved into the producing
pass — `hidden`, `heads_out`, `merged_core`, `merged`, `win`, `q`, `k`, `v` and the
transition's `pooled`. At full resolution they were the largest allocations in the
graph; block 0's alone came to well over a gigabyte. 720p now holds **5.61 GB** of
device buffers, and 1920x1080 renders in **2.9 s** where it used to fail allocation
before the second block.

## Where the 613 ms sits now

`src/bench/split_cost.py` runs the frame with each half of the work skipped in turn:

```
whole frame                      663 ms
GEMMs only                       327 ms
everything but the GEMMs         366 ms
```

An even split. The GEMMs are at 459.6 GFLOP / 327 ms = **1.4 TFLOP/s, 4.4 % of the
~32 TFLOP/s FP16 peak**; the elementwise passes touch 3.03 G elements at 8-13
Melem/ms, most of them at 50-90 GB/s against a ~90 GB/s ceiling. So the second half is
close to its floor and the first is not.

30 fps at 720p is 33 ms. This is 613. The remaining factor of 18 is not in fusion —
that vein is worked out — it is in the GEMM kernel reaching a normal fraction of peak,
which on this driver means shared-memory staging of the operands, and in a smaller
network extent. A **640x384** extent remains the honest route to 30 fps.
