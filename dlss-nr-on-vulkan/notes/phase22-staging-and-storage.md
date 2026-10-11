# Phase 22 — two suggested optimisations, measured

> **Note 2026-09-10.** The "between processes the same binary spreads 586-718 ms ...
> so it is buffer placement, not clocks or heat" conclusion in phase 21 and repeated
> here was wrong. Most of that spread was **host** variance from 73 command-buffer
> submissions and five CPU-side skip copies per frame. With the frame captured as one
> command buffer and replayed (`notes/phase28-frame-replay.md`) the same measurement
> reads 494, 494, 494 ms across three processes. Measure through replay before
> attributing a spread to placement.

2026-09-09, after phase 21. A second model, consulted separately, proposed three
things: read llama.cpp's Vulkan `mul_mm.comp` for a production coopmat GEMM, watch two
llama.cpp issues about coopmat on Intel, and move activations to **bfloat16** to halve
the traffic on what phase 21 had called a bandwidth-bound frame.

All three were worth following. Two of them turned out differently than proposed, and
the differences are the useful part.

## The storage format: right lever, wrong format

`src/bench/dtype_traffic.py` recovers each buffer's width from its size against the
element count its pass is given, so it needs no bookkeeping in the graph:

```
720p frame, activation traffic by storage width
  float32                21.26 GB  (66 %)
  float16                10.94 GB  (34 %)
```

So the premise held: two thirds of the frame's activation traffic was still float32,
and that is a large lever.

The format was the wrong half of the proposal. `src/bench/bf16_check.py` captures real
activations and asks which are exactly representable:

| tensor | exact in float16 | exact in bfloat16 |
|---|---|---|
| `full_skip`, `l1_in`, `l1`, `l2_in`, `l2`, `l3_in` — **published** | **100.00 %** | **100.00 %** |
| `stem`, `block0`, `head` — **unpublished** | 0.01-0.02 % | **0.00 %** |

What decides whether a buffer can be narrowed is not the format, it is whether the
value has been **published**. A publish is E4M3 — three mantissa bits — and lands
exactly in either format. An unpublished value is a raw float32 GEMM or residual output
and lands exactly in neither.

And bfloat16 is the worse of the two here on every count that matters:

- It is the same two bytes, so it saves no traffic float16 does not.
- It keeps 7 explicit mantissa bits against float16's 10, so it cannot hold the
  half-rounded values the quadratic gate, the softmax's affine map and the cosine tree
  produce **by construction** — the graph's non-published intermediates are float16
  values, not arbitrary float32 ones.
- The weights ship as FP16 and cooperative-matrix config 1 is `fp16 x fp16 -> fp32`, so
  float16 reaches the matrix units with zero conversion. This was already settled in
  notes/CLAUDE.md; the measurement above is the empirical form of it.

**Done, in float16, on the published buffers only.** Every level's value, the skips,
the transition outputs and `full_skip` are now narrow, and the widening pass in front of
each block's first GEMM went with them. The bottleneck keeps float32 — its tensors are
the graph's smallest — and its two edge copies carry the width, exact in both
directions. 1848 -> 1783 passes, correlation unchanged at 0.981311.

**It did not make the frame faster**: ~605 ms against ~625, inside the run-to-run band.

That is the result worth keeping. At 30.9 GB in ~610 ms the frame averages **50 GB/s
against a ~90 GB/s ceiling**, so phase 21's "bandwidth-bound" no longer holds — it was
true at 65 GB and 1025 ms, and the fusion work fixed it. Shaving traffic now returns
less than linearly, and both halves of the frame are **latency-bound**: the GEMMs sit at
4.4 % of arithmetic peak *and* 30 GB/s of traffic, which is neither wall.

## Operand staging in shared memory: a clean negative

Latency-bound points at memory-level parallelism, and the standard answer is to stage
both operands in shared memory: many wide loads in flight, then `coopMatLoad` from
shared instead of from global. That is what llama.cpp's `mul_mm.comp` does, and reading
it settled a second thing — **it stages the accumulator through shared memory too**
(`coopmat_stage`), for its own layout reasons. The detour phase 21 found around the ANV
store bug is production practice, not a workaround.

`src/gpu/gemm_staged.comp` implements it: 64x32 of A and 32x32 of B filled cooperatively
by 128 threads as two 64-bit loads each, four subgroups stacked along M taking a 16x32
slice apiece. The vectorised fill matters — scalar fills gave 122.6 ms over the shape
census against 109.4 for the same kernel with `f16vec4` loads.

Per shape, against the 16x32 register-block kernel it replaces:

| shape | register block | staged | |
|---|---|---|---|
| 1536x1536x512 | 2.84 ms | **2.29** | staged, −19 % |
| 960x512x512 | 5.59 | **4.89** | staged |
| 3840x128x256 | 12.31 | **10.86** | staged |
| 240x4096x1024 | 6.77 | **6.71** | staged |
| **983040x128x32** | **10.66** | 15.32 | register block, staged +44 % |
| **245760x128x32** | **9.98** | 14.55 | register block |
| **61440x128x64** | **7.26** | 9.13 | register block |

The split is exactly where the theory says it should be. Staging pays for itself out of
**reuse**, and reuse needs depth in K. The shapes that dominate this graph's GEMM time
are shallow — K = 32 or 64 with M near a million — where the cost is the output write,
there is nothing to reuse, and the fill plus two barriers per block are pure overhead.

Gated at K >= 128 it stops losing. Over a whole frame, interleaved so drift cannot
favour either side:

```
off     625  588  581  591      median 591 ms
staged  599  577  601  587      median 587 ms
```

**Indistinguishable.** The kernel is kept, gated, and defaulted on — it is 19-24 % on
the split family's shapes and the balance would shift at a different network extent —
but it is not the lever it was expected to be, and `XMX_STAGE_K` turns it off.

## What llama.cpp's Intel issues actually say

- **#13530** — coopmat is disabled for all Intel GPUs because of Alchemist: the A770
  regressed (#12690). A user has since measured **Arc B580 (Xe2/Battlemage) at +329 %
  on pp512 and +229 % on tg**, and the maintainer wants a way to tell the generations
  apart. So Xe2 is no longer unmeasured — but **B580 is a discrete card with GDDR6**.
  Lunar Lake's Arc 140V is the same architecture on a UMA LPDDR5X pool shared with the
  CPU, which is a different machine where it counts, and that part is still unmeasured
  in public. Our numbers are for exactly that die: config 1 reaching **3.5-3.9 TFLOP/s**
  on well-shaped GEMMs, **1.4** on the shallow-K shapes, and a measured negative result
  for shared-memory staging on both.
- **#18946** — Arc 140V memory-accounting failures under UMA. Closed as *not planned*,
  stale, and on the **Windows** driver. Not our stack; Mesa/ANV on Linux allocated
  everything asked of it. The 1080p `vkAllocateMemory` failure this project hit was its
  own fault — ten dead scratch buffers — and went away when they did.

The one thing here genuinely worth sending upstream is the **Mesa/ANV cooperative-matrix
store bug** from phase 18: any arithmetic on an accumulator between `coopMatMulAdd` and
`coopMatStore` scrambles the result, with a two-line reproducer and a table of five
variants. That is a driver bug, it is still live in 26.2.1, and nobody appears to have
reported it.

## Standing

720p ~600 ms, unchanged by either of this phase's two changes. The frame is
latency-bound in both halves, at 50 GB/s of a ~90 GB/s ceiling and 4.4 % of arithmetic
peak. Neither traffic nor operand staging is the remaining lever; what is left is either
a smaller network extent or a different decomposition of the shallow-K shapes, where the
graph spends its GEMM time and where there is nothing to reuse.
