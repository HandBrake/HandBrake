# Phase 45 — where the frame's time actually goes, measured instead of ablated

2026-09-10. The frame has been carried as "221 ms of GEMM against 290 ms of everything
else" since `phase29`, from `split_cost.py`, which gets there by running the frame with
one half of its passes removed and differencing the wall time. That note says itself the
figure is approximate: skipping passes changes the values the rest of the graph works on.

Nobody had ever looked inside the 290 ms.

## The instrument

`libxmx.c` gained GPU timestamping: `xmx_profile(1)` puts one `vkCmdWriteTimestamp` after
each recorded pass, so pass *i* costs `ts[i] - ts[i-1]` on the device's own clock. The
barrier already between passes makes that attribution exact. Nothing is ablated and the
frame measured is the frame that would have run. Off by default and free when off —
`stamp()` returns on its first test. Captured graphs keep their own copy of the pass
labels, since replay rewrites the same query slots.

`src/bench/frame_profile.py` prints the breakdown. 1280x768, three runs:

| pass | ms | share | passes | us each |
| --- | --- | --- | --- | --- |
| gemm tiled | 107.3 | 22.0 % | 306 | 351 |
| gemm staged | 79.9 | 16.4 % | 659 | 121 |
| **row: softmax** | **74.4** | **15.3 %** | 70 | 1063 |
| **row: cosine publish** | **67.9** | **13.9 %** | 140 | 485 |
| **unary: residual** | **51.3** | **10.5 %** | 141 | 364 |
| gemm tiled, transposed | 26.8 | 5.5 % | 62 | 433 |
| unary: split heads | 19.9 | 4.1 % | 70 | 284 |
| unary: partition | 17.7 | 3.6 % | 62 | 286 |
| unary: merge heads | 15.3 | 3.1 % | 70 | 219 |
| unary: to half | 11.3 | 2.3 % | 28 | 405 |
| everything below 4 ms | 15.8 | 3.2 % | | |
| **device total** | **487.6** | | | |
| wall per frame | 516.0 | | | |
| host overhead | 28.4 | | | |

GEMM is **216 ms of 487.6, 44 %** — the old ablation said 221 ms, so both instruments
agree and the earlier figure can be trusted after all.

## Where the headroom is

Three passes hold 194 ms of the 272 ms that is not GEMM. Measuring each one's traffic and
dividing gives the achieved bandwidth, against the 70-91 GB/s this machine reaches
(`phase26`):

| pass | traffic | ms | GB/s | of ~80 |
| --- | --- | --- | --- | --- |
| softmax | 2.65 GB | 74.4 | 36 | **44 %** |
| cosine publish | 2.58 GB | 67.9 | 38 | **48 %** |
| residual | 5.33 GB | 51.3 | 104 | 130 % |
| to half | 0.70 GB | 11.3 | 61 | 77 % |

**`residual` is done.** At 104 GB/s it is above the measured ceiling, which means part of
its reads are being served from cache and the pass is bound by the machine, not by the
kernel. `to half` at 77 % is close enough that it is not worth touching either. Both were
candidates on the way in and both are ruled out by measurement.

**`softmax` and `cosine publish` are the target.** Together 142 ms — 29 % of the frame —
at under half the bandwidth the machine gives. Both are row-wise reductions dispatched as
`(rows + 31) / 32` workgroups of 256 threads, which is 8 lanes per row; whether that
split, the cross-lane reduction, or the access pattern is what costs the other half is
the next thing to find out. The parallelism is not the problem: the frame's softmaxes run
over up to 999,488 rows, which is 31,234 workgroups.

## A mistake worth keeping

The first run of this table labelled every unary pass wrongly, because the kind names in
`frame_profile.py` were written from memory instead of read from the shader. Under those
labels `to half` appeared to cost 49.9 ms and run at 14 GB/s — a fifth of the memory —
and that produced a confident, detailed and entirely false diagnosis about narrow writes,
complete with a fix. The 49.9 ms belongs to `residual`, which is the one pass that turned
out to need nothing.

The tables are now generated from `resident.comp` and `attention.comp` by regex at import
time, so a kind added to a shader cannot silently mislabel a row again. **A profiler that
names things is only as good as the names**, and the numbers looked equally plausible
either way.

## Every pass, against the machine's ceiling — and the answer

Traffic measured per pass and divided by the profiled time, against the 70-91 GB/s this
machine reaches:

| pass | ms | GB/s | of ~80 | verdict |
| --- | --- | --- | --- | --- |
| row: softmax | 74.4 | 36 | 44 % | arithmetic |
| row: cosine publish | 67.9 | 38 | 48 % | arithmetic |
| unary: residual | 51.3 | 104 | **130 %** | at the ceiling |
| unary: split heads | 19.9 | 65 | **81 %** | at the ceiling |
| unary: partition | 17.7 | 69 | **86 %** | at the ceiling |
| unary: merge heads | 15.3 | 84 | **105 %** | at the ceiling |
| unary: to half | 11.3 | 61 | **77 %** | at the ceiling |

**Every pass in the frame that only moves data is already at the memory ceiling.** The
figures above 100 % mean part of the reads are served from cache, which is the same
statement. Nothing in that group is waiting on a better kernel.

The only two below the ceiling are softmax and cosine publish, and both do real
per-element arithmetic — a hand-rolled `f16x2` exponential, an E4M3 quantisation and a
half rounding per element. For a pass like that, 45 % of *bandwidth* is not a deficiency;
it is what an arithmetic pass looks like when measured with a bandwidth ruler. And the
softmax has already had one round of exactly this work: `phase29` replaced the manual
half encode/decode with `packHalf2x16`, won 13.7 % on the isolated kernel, and that came
to **1.1 %** of the frame.

Both scale at 3.85-3.89x for a 4x area, so there is no fixed per-dispatch cost hiding in
them either — it is all per-element work.

### Two hypotheses this note killed

- **`to half` at 14 GB/s.** An artefact of the mislabelled kind table, above. The pass is
  11.3 ms at 61 GB/s.
- **Bank conflicts in the row shaders.** `attention.comp` stages 32 rows at stride 64 and
  gives each of a subgroup's 32 lanes one row, so lane `l` reads `stage[l*64 + i]` — bank
  `i mod 32` for all 32 lanes at once, a textbook 32-way conflict. `src/bench/bank_probe.*`
  measures that exact pattern with the stride padded to 65 and without:

  ```
  64  (conflicting)   25.78 ms
  65  (padded)        23.13 ms      1.11x
  ```

  **1.11x, not 32x.** Xe2's shared memory does not punish this the way the textbook
  describes, and the surgery on `gather`/`scatter` that padding would have required is not
  worth 11 % of two passes. Kept as a probe so the next person does not have to guess.

## What the frame is, now that it is measured

GEMM is register-bound (`phase26`: 128 GRF, an accumulator costs 4, larger blocks spill
and get slower). Everything that moves data is bandwidth-bound. What is left is two
arithmetic passes that have already been through one optimisation round for 1.1 %.

**"Performance is finished" was already the standing conclusion. It is now a measured one,
pass by pass, rather than an inference from two ablations.** The remaining lever is not
inside the frame at all: it is to give the network fewer pixels, which is `phase37` and
needs a neural upscaler downstream.
