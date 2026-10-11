# ProjectsCodex's fusions, ported: 18 % at 720p, bit-identical

ProjectsCodex fused three things in its own tree between 2026-09-13 and 09-16 and
measured each. None of it reached this repository: that tree is a separate git history
with no common ancestor, and `improve` had since rewritten the same files — FFN batching,
compact I/O, joint QKV, the unmapped memory path. So this is a port onto `improve`'s code,
not a merge, and it was measured again here rather than trusted.

## What moved

| | Codex's phase | what it removes |
|---|---|---|
| dense residual in the GEMM epilogue | 38 | the float32 branch write, its read, one pass per projection |
| window residual, same, with the window reverse | 39 | the above, plus the residual's own gather back into the image |
| window attention QK^T, softmax and PV in one pass | 42 | the scores and probabilities' two round trips through memory |

These are not dispatch-count reductions for their own sake. `phase45` found every pass that
only moves data already at the memory ceiling; each fusion here deletes a buffer's round
trip, which is why they pay where cutting dispatches alone does not.

## Measured on this tree

Paired, alternating, all three on against all three off, every head bit-identical:

| output | off | on | gain |
|---|---|---|---|
| 384x384 | 82.37 ms | 69.24 ms | **15.9 %** |
| 1280x720 | 470.97 ms | 386.66 ms | **17.9 %** |
| 1920x1080 | 1020.37 ms | 828.90 ms | **18.8 %** |
| 384x384, `XMX_STAGING=1` | | | 14.1 % |

Ranges do not overlap at any size. Dispatches 1128 -> 864, on top of `improve`'s FFN
batching; the two compose. Codex measured the same three at about 18 % in its own tree
(497 -> 407 ms at 720p), and an independent re-run there reproduced it: 496.96 -> 406.77.

## Two things a straight copy would have broken, silently

**The graph-cache key.** Codex keys its fusions at bits 5-7. Here those are `input_fp16`,
`compact_head` and `joint_qkv`. Copied as they were, a graph captured under one setting
would replay for another — a wrong picture and no error anywhere. They take bits 8-11, and
`test_ffn_batch.py` now demands 4096 distinct keys across every combination.

**The publish order.** `gemm_resident.comp` here publishes — rounds to E4M3, applies the
gate — on the accumulator before storing. That is right for everything else and wrong for
a residual, which the two-pass path adds *before* rounding. So the residual has its own
path: raw accumulator through shared memory, then residual, publish, store.
`gemm_staged.comp` already staged the raw value and needed only the addition.

## The scratch arena, checked by hand

The fused kernels write their output while still reading their input, which the two-pass
path never did. The arena aliases roles with disjoint lifetimes, so a fusion could write
into memory its own input occupies. Every site was checked: input and output hold
different roles in all six residuals, and the fused attention writes `context`, not
`merged16`, because `merged16` shares `q16`'s role and another workgroup may still be
reading Q. Codex had made that last choice for the same reason; the role table is
identical in both trees.

## The head merge, added after (2026-09-23)

The merged-output mode of `window_attention.comp` was left behind at first: off in Codex's
tree and unmeasured there. A per-pass profile put it back on the table — `merge heads`
was 16.3 ms of a 382 ms frame at 1280x768, a pass that reads the FP32 context only to
publish it as E4M3 in (window, token, C) order, which the attention's own store can do.
Paired, alternating, `NR_FUSE_ATTENTION_MERGE` off against on, every head bit-identical:

| output | off | on | gain |
|---|---|---|---|
| 384x384 | 69.25 ms | 67.40 ms | 2.7 % |
| 1280x720 | 386.25 ms | 370.08 ms | **4.2 %** |
| 1920x1080 | 838.38 ms | 810.21 ms | 3.4 % |
| 384x384, `XMX_STAGING=1` | 66.90 ms | 65.41 ms | 2.2 % |

A second 720p run gave 384.53 -> 370.87. 62 dispatches fewer, 864 -> 802. The merged store
goes into `context16`, a new name in the scores' arena role, which the fused path leaves
unused — not into `merged16`, for the reason above — so it costs no memory.

The benchmark also showed the head read 1.5-2x slower with the merge on (720p 4.2 -> 6-7 ms,
1080p 8.7 -> 16.7). It did not survive a direct probe: 3.99 against 3.98 ms over twelve
alternating frames each, with and without the benchmark's per-frame comparison. The graph
saving did: 13.6 ms at 720p there too.

## Window attention in 2 KB of shared memory (2026-09-24)

The fused attention declared 3104 bytes of shared memory: 2 KB of logits, 1 KB of
probabilities and 32 bytes of reciprocals. Shared memory is allocated in powers of two on
this hardware (`improve-qkv-epilogue.md` found it the hard way), so every workgroup took
4 KB and only half as many fit on a core as the thread slots allow. Padding the same shader
to 8 KB made it **76 % slower** (56.3 -> 99.2 ms of a 1280x768 frame), which says the pass is
bound by how many workgroups are resident, not by its arithmetic.

It now takes exactly 2 KB: the logits are staged 32 keys at a time, each half becoming its
weights before the next is stored; the weights are kept as halves, which they are exactly;
and the row reciprocals go by `subgroupShuffle` instead of shared memory. Every value and
every order of summation is unchanged — 48 kernel cases and three whole frames
bit-identical, one of them a real game frame. **56.3 -> 46.1 ms**, and the compiler reports
128 registers and no spills, so the pass is now fully resident.

With it, block 70 stores its output as half for the head directly, since nothing reads the
float32: one `to_half` pass and 126 MB of float32 at 720p gone. Graph time at 1280x768,
same script, same clean machine, morning against evening: **274.3 -> 257.7 ms**; the extent
curve is 10 ms + 259 ms per megapixel.

## The full-resolution glue (2026-09-24)

Around blocks 0 and 70 the graph did a stack of full-frame elementwise passes, each writing
float32 for the next to read back. Two of those chains are now one pass each
(`NR_FUSE_GLUE`, graph-key bit 13):

- the stem's GEMM stores its float32 result — block 0's residual — and, in the same
  epilogue, the half copy block 0's first GEMM reads, so its `to_half` pass is gone;
- block 70's input — `upsample2`, `scale_channel`, `residual`, `to_half`, four passes —
  is one `UPSAMPLE_MERGE` pass that stores both widths.

**The residual pass compiles to a fused multiply-add**, and that is measured, not read off
the source: on 64 crafted inputs where one rounding and two differ, the GPU matched `fma`
64 times and multiply-then-add never. So the merged pass writes `fma()` for the residual
and marks the scale `precise`, which keeps it rounded on its own as `scale_channel` did.
Writing the same expression in a new shader would have left the choice to the compiler.

`test_glue.py`: 24 merge and 18 GEMM cases, both widths byte for byte, NaN fills to prove
every element written. Three whole frames bit-identical, in both memory modes. Paired:
1280x720 **268.1 -> 260.2 ms**, 1920x1080 571.2 -> 555.6, 384x384 unchanged (50.7 -> 50.8).

## The narrow feed-forward in one kernel (2026-09-24)

A profile per call site (`frame_profile.py --calls`) put the feed-forward at 96 ms of a
265 ms frame, and the 32-channel blocks' share — block 0, block 70 and the eight at half
resolution — at 33.6 ms in four call sites, every one of them at the memory ceiling. The
reason was the hidden layer: 128 wide, written as half by the expand and read back by the
projection, 504 MB per block at 720p.

`ffn_fused.comp` takes 16 rows through the expand, its gate and publish, the projection
and the residual in one subgroup, 32 hidden columns at a time through 1 KB of shared memory,
so the hidden layer never leaves the chip. Every accumulator takes the same multiply-adds
in the same K order as the two GEMMs did, the publish and the residual are their own
includes, and 144 kernel cases and three whole frames are bit-identical, in both memory
modes. On the GPU clock the four call sites' 33.6 ms became 21.7; paired wall time at
1280x720 fell 9-14 ms across runs, at 1920x1080 27.6 ms (549.9 -> 522.3).

**Spills were not the problem, and removing them made it slower.** The first version
spilled (34 registers out, 36 back in, reported by `INTEL_DEBUG=cs`). Narrowing the chunk
to 16 columns removed every spill and cost 9 % (21.7 -> 23.7 ms): twice the barriers and
shared-memory round trips for the same arithmetic. The compiler's spill count is a symptom
worth reading, not a target in itself; the kept version reads its input again per chunk,
which takes the spills to 31:33 for the same time.

The branched blocks' feed-forward is the same shape of work per group — all C channels
expanded to 128, gated, projected to the group's 32 — so the kernel takes groups on the
grid's second axis too (`NR_FUSE_BRANCHED_FFN`, 24 more cases bit-identical). **It stays
off.** At 720p, per level: 1.19 -> 0.98 ms a block at C=64, 0.83 -> 0.79 at 128, 0.78 ->
0.71 at 256; the frame moved 2.7 ms paired, 6.8 at 1080p, and at 384x384 it was 2.6 ms
*slower*. The deeper levels are arithmetic rather than memory, and there one subgroup per
16 rows is less efficient than the staged GEMM's 64. Live mode runs at the small extents.

## The bottleneck on whole 64-row blocks (2026-09-24)

The eight bottleneck blocks run GEMMs with K up to 4096 on 240 tokens at 720p, which is
not a whole number of 64-row blocks, so they took the register-tiled kernel. Padded to 256
they take the staged one: 240.6 -> 235.4 ms on the GPU, three alternating runs each, and
bit-identical, since pad rows are zero and the softmax excludes them. It pads only when that
costs at most an eighth more rows; at 384x384 and 1024x576 the counts (64, 192) are whole
blocks already.

## The partition, folded into the QKV projection (2026-09-24)

Every window block partitioned its feed-forward output — read the image, wrote it again in
shifted-window order as half — for the QKV projection to read. `NR_FUSE_PARTITION` has the
projection gather its own window rows instead: the staged GEMM's A loader maps each token
row to its pixel, reads float32 or half from the image, writes zero outside it and rounds to
half on the way into shared memory — the partition's own arithmetic, so 40 kernel cases
(both origins, extents that are not whole windows, float32 and half images) and three whole
frames are bit-identical. It takes the staged path at every depth, since that is the kernel
whose A goes through shared memory.

62 passes fewer. Level 0 at 720p: 4.48 ms for the gathered projection against 3.52 + 2.48 for
the tiled one and its partition. Paired, whole frame: 1280x720 240.1 -> 231.4 ms, 1920x1080
524.7 -> 511.8, 320x320 41.7 -> 41.0.

## The staged GEMM's loads, one at a time (2026-09-24, later)

At the live extent the deepest small-M GEMM, the bottleneck's 64x1024x4096, makes 32 blocks of
64x32 for the machine's 128 places and runs 128 K steps of about 2.1 us each. Two ways to
shorten it changed nothing, which is what located the cause:

- **twice the blocks** — a 64x16 build of the staged kernel, routed to the GEMMs with few
  blocks and bit-identical to the 64x32 one on 44 kernel cases: the call stayed at 0.275 ms;
- **half the steps** — BK = 64: slower, 0.28 -> 0.31 ms, as the entry below already said.

What does move it is the loader. Each of its loads sits in its own `window_a` / `wide_a` branch,
and the compiler waits for one before issuing the next, so a step costs two or three memory
latencies in a row; with the machine full that hides behind other workgroups, with 32 blocks
it is the time. The common case — A read straight, B not transposed, both aligned — now issues
every load of the step before storing any: 64x1024x4096 0.27 -> 0.22 ms, and paired whole
frames at 320x320 39.5 -> 38.5 ms over six rounds (faster in five), 1280x720 199.6 -> 196.3
over four (faster in all). The same values land in the same places: bit-identical.

## A partial last block on the staged kernel (2026-09-24, later)

The staged kernel took only M in whole 64-row blocks, so every GEMM over a level whose pixel
count is not — 144 or 400 rows at the live extent's deeper levels — went to the tiled kernel,
direct loads and no staging. Timed alone, 32 dependent dispatches each, the staged kernel at
the row count rounded up to 64 was twice as fast: 144x512x512 published 137 -> 54 us, with the
residual 99 -> 51, 400x128x256 x8 128 -> 64, and 400x256x256 40.9 -> 39.4.

Rather than pad the levels' buffers, the kernel now takes the partial block itself: rows past
M read the last real row, so nothing is read out of bounds, and the epilogue never stores
them; the direct store is kept for whole blocks. The QKV epilogue, which finishes a block's
rows together, stays on whole blocks. A real row's arithmetic is unchanged, so 34 kernel cases
match the tiled and 8x16 kernels byte for byte with nothing written past M
(`test_staged_partial.py`, which also reads the profiler to see the staged kernel run, and
fails at once with the row guard taken out), and the three reference frames are unchanged.

Paired whole frames: **320x320 38.0 -> 34.2 ms** over six rounds, and a 1280x720 extent
197.5 -> 189.3 over four, faster in every pair. But which extents gain depends on whether a level's pixel count is whole 64-row blocks. The daemon's extents are multiples of 64, so level 4 is (extent / 16)^2: 400 pixels at 320x320 and 8160 at 1920x1088 are partial, while at 1280x768 every level is whole blocks and nothing changes. The replayed graph
curve: 320x320 36.5 -> 32.7 ms, 1920x1088 444.6 -> 422.6, 1280x768 196.3 -> 196.6; the fit
is now 9.4 ms + 196 ms per megapixel. Live, through the socket: **512x288 at 0.35 42.7 ->
36.7 ms**, 27 fps. On by default; `XMX_STAGED_PARTIAL=0` is the comparison.

## Tried and dropped (2026-09-24)

- **Register-prefetch pipelining in the staged GEMM** — the next K block's global loads
  issued before this block's multiply-adds, the classic way to hide load latency on the
  small-M deep-K GEMMs of the deep levels. 30 % slower: 14.1 -> 20.3 ms of staged GEMM at
  320x320, 83.9 -> 110.5 at 1280x768. Phases 21 and 26 found the same for the K loop.
  Re-measured by mistake on the bottleneck's shapes alone, where it had the best case: twice
  as slow, 64x1024x4096 0.19 -> 0.38 ms. Read this list before trying a loop change.
- **The tiled or base kernel for those GEMMs**, for more workgroups: the frame at 320x320
  went 42.9 -> 53 ms either way. Staged is the best of the three there.
- **A 64-deep K block in the staged GEMM**, for half the trips round the K loop and its
  barriers: slower everywhere, the deepest small-M shapes included (64x4096x1024 0.21 ->
  0.31 ms), and 81 -> 119 ms of staged GEMM at 720p. The barriers are not what those GEMMs
  wait on. Re-measured by mistake later the same day, with one more thing: as a drop-in it
  changes the frame, because the window-gathered projections take the staged kernel at any
  depth and at level 0 their K is 32.
- **Eight subgroups to a block instead of four**, each 8x32, for twice the threads: slower
  everywhere — staged GEMM 14.9 -> 16.7 ms at 320x320 and 71.5 -> 87.8 at 720p. Threads were
  never short; the blocks' serial K loops are what the time is made of.
- **64x16 blocks** for the GEMMs with too few 64x32 ones: bit-identical, and no faster where it
  was aimed (above); used everywhere, 10 ms slower at 720p.
- **The window-gathered A's loads issued together too**, as in the section above:
  bit-identical, and slower — 1.1 ms at 320x320 (all six pairs) and 1.3 ms at 720p. Those
  projections are at the memory ceiling already: at level 0 of a 720p frame one reads a
  120 MB float32 image and writes 180 MB of Q, K and V in 4.2 ms, 71 GB/s. A half copy of the
  image would not help either — the output projection's residual still needs the float32,
  so writing the copy costs what reading it saves.
- **Weights stored in 32-column slabs**, so a workgroup streams its K x 32 block instead of
  64 bytes from every 2 KB row: identical results, 14 % on 64x1024x4096 and 10 % on
  64x4096x1024, nothing on the rest — about 0.6 ms at 320x320 for a layout change at every
  weight's upload and every GEMM path. Not taken. Re-measured 2026-09-25 on the fixed staged
  kernel: 4096x1024 -17 %, 3072x1024 -14 %, 1024x1024 -3 %, 1024x4096 noise either way — and
  the QKV projection, one of the two that gain, cannot always take the staged kernel (its
  epilogue wants whole 64-row blocks; at 384x384 the bottleneck is 144 rows), so its slab
  weights would need a second, row-major copy. Without it, about 0.2 ms. Still not taken.
- **The cost of a pass itself** is small: 1.2 us for an empty dependent pass, 4 us for 64k
  elements. The 577 passes of a frame are under a millisecond of it; at 320x320 the time is
  the deep levels' GEMMs, latency-bound on 32-200 workgroups. (Measured again later with a
  unary pass of 64 elements and its barrier, replayed: 4-4.6 us, against 0.5 us recorded as
  independent — about 2 ms over the ~515 passes at 320x320. Small either way.)
- **Weights stored as their E4M3 bytes**, decoded to half in the loader. It would be exact:
  every one of the bottleneck's 100.7 M weights is an E4M3 value — half of them E4M3
  subnormals, which a decoder has to get right — and MLX-DLSS's decode has no scale. A proxy
  that reads half the bytes and decodes nothing moved the bottleneck's four GEMMs 431 -> 414 us
  a block: 0.14 ms a frame at 320x320 before paying for a decode. Only 64x3072x1024 moved
  (96 -> 79 us); the K = 4096 one, the slowest, is not waiting on weight bytes. Not built.
- **Polling the graph's fence instead of sleeping on it** (below, in HANDOFF: a busy core makes
  the graph faster). From the waiting thread it bought 0.5-1 ms of the 4.5 a separate busy
  process buys — whether it paused between polls, did integer work, or polled every 1, 42 or
  680 us. Not kept.

- **The bottleneck blocks publishing straight into `deep`** — the E4M3 pass and the to_half
  after each global block folded into its closing residual, 16 passes fewer. Bit-identical,
  a padded bottleneck included (1920x1088, 2040 tokens on 2048 rows), and no measurable change
  at 320x320 or 1280x720: those passes were 7-10 us each, and a publishing epilogue sends the
  staged kernel through its stage instead of the direct store. Not kept (2026-09-25).

- **The fused FFN's hidden-layer publish from a table.** `e4m3(gate(x))` depends only on
  `half(x)`, so 65536 halves hold it exactly. Without the publish at all the kernel is 21-35 %
  faster (983040 rows 5.8 -> 4.56 ms), which is what made it worth a try; looked up from a
  table in the weights buffer it is 2.7x *slower* (15.9 ms) — sixteen scattered loads a lane
  a chunk cost far more than the thirty instructions they replace. Not kept (2026-09-25).

- **Window attention's bias loaded earlier or in pairs.** All sixteen of a lane's bias values
  loaded before the QK multiply: 5-28 % slower (register pressure). Read as `vec2` pairs,
  eight loads instead of sixteen: level with the scalar loads, 3 % slower at 1600 batches.
  The bias loads are not what this kernel waits on. Not kept (2026-09-25).
- **`encode8` handing back a view instead of `tobytes()`**: a frame's copy fewer, and no
  measurable change at 640x360, 1280x720 or 1920x1080. Not kept.

## Left behind, deliberately

- `window_attention_qkv.comp`: off by default in Codex's tree (`NR_FUSE_QKV_ATTENTION`) and
  not measured there as enabled. It reads the FP32 QKV projection inside the attention;
  normalising in the QKV GEMM's own epilogue instead removed that projection altogether,
  22 % of a frame (`improve-qkv-epilogue.md`), so there is nothing left for it to read.
- `DIRECT_EPILOGUE`: an experiment there, and `improve` already stores float32 epilogues
  straight from the accumulator.
- phase37's paired cosine conversions: measured 0.6 % *slower* by Codex and never enabled.

## Switches

`NR_FUSE_RESIDUAL=0`, `NR_FUSE_WINDOW_RESIDUAL=0`, `NR_FUSE_WINDOW_ATTENTION=0` and
`NR_FUSE_ATTENTION_MERGE=0` each restore their separate passes, which are also the
reference the fused one is bit-identical to. The kernel tests are Codex's: 120 dense
residual cases, 192 window cases with padding and shifted windows, and 48 attention cases,
each in both output layouts, covering zero, negative zero, subnormals, the clamp
boundaries and every finite half as a bias. All of them pass under `XMX_STAGING=1` too —
they did not until 2026-09-23, because they wrote buffers through `.view()`, which an
unmapped buffer refuses.
