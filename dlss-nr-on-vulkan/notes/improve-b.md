# improve-b — the bottleneck's small-M GEMMs

A research branch, opened 2026-09-25 at the owner's request, for the one lever the small
network frames `min_extent` allows left standing: the bottleneck's GEMMs. At 192x128 the
eight bottleneck blocks are about 30 % of the graph; their GEMMs have 16-64 rows and K up
to 4096, and run at 30-45 GB/s of weight streaming against a machine ceiling of 70-91.

## What bounds them (measured on the staged kernel, 64x1024x4096, 200 us)

- Taking the global loads out (constants in their place): A 165 us, B 140, both 119.5. So
  even with no memory traffic a 32-deep K step costs ~0.93 us of shared-memory stores,
  barriers and fragment loads, and with 32 workgroups on 8 cores nothing hides it.
- The three kernels on the bottleneck's shapes, bit-identical to each other: staged is the
  fastest everywhere but 16x3072x1024, where tiled and base tie it (16x1024x4096: staged
  188 us, tiled 399, base 401; 64x1024x4096: 200, 703, 775).

## A deeper K step (tried)

`STAGED_BK` makes the step a build parameter; the loaders cover an A row 32 columns at a
time and the fragment loop runs k in the same order, so every variant gives the same bits
(checked on six shapes). Standalone:

| shape | BK 32 | BK 64 | BK 128 |
| --- | ---: | ---: | ---: |
| 64x1024x4096 | 196-203 | 168-169 | 273 |
| 64x4096x1024 | 93-95 | 118-122 | 272 |
| 64x3072x1024 | 97-98 | 112-124 | 204-214 |
| 64x1024x1024 | 48-49 | 44-47 | 63-64 |
| 144x512x512 | 36-37 | 33-37 | 69-70 |

64 wins only where there are few workgroups (32 of them at N=1024), 15 %; with 128 it
halves occupancy (16 KB of shared memory) and loses. 128 loses everywhere — past 64 the
step's registers spill. The time of a step grows with its work, not with its barriers:
the loop is not waiting on barriers. As a routed variant for K >= 2048 and <= 64
workgroups it would be worth about 0.26 ms a frame (the FFN's down projection, eight
blocks). Kept as a parameter, not routed.

## Not repeated

Register prefetch in the staged kernel was measured on exactly this shape before
(`improve-fusions.md`): twice as slow. Split-K changes the order of summation.

## First, how many tokens the bottleneck has

**64 at 320x320, not 25.** Every level is padded to a multiple of 8 before it is halved
(`nr_frame_resident.py`, the level table), so 320 goes 20 -> 24 -> 12 -> 16 -> 8: an 8x8
bottleneck. At the vendor's minimum extent the pad to 64 rows costs nothing, because
there is none. Fewer than 64 tokens needs `min_extent` below 320: 16 at 256x128 and at
192x128, 32 at 320x192 and 256x192.

## A kernel without the shared-memory round trip (tried, not routed)

`src/gpu/gemm_smallm.comp`: one subgroup per 16x16 block of the output, fragments straight
from memory, two K steps of loads in flight, B packed so a 16x16 tile is 512 contiguous
bytes. Bit-identical to the staged kernel (same fragments, same order). Three things
decided it:

- **It wins only on weights that are still in cache.** Timed the way the other kernels
  were — the same B every call — 16x1024x4096 is 126-146 us against staged's 188 at 16
  rows. Rotating eight copies of B, so it comes from DRAM as in a frame, the four
  bottleneck GEMMs at 16 rows take 400 us a block against staged's 423 at 64 rows. At 32
  rows 451, at 48 rows 586. Nothing to route.
- **Registers cap the loads in flight at two steps.** Four steps spilled (11:26, then
  27:48 with the addresses spelled out) and ran twice as slow; three do not divide K.
  Thirty-two rows a subgroup spilled 49:75. An array of cooperative matrices indexed in a
  loop compiled a third slower than the same tiles written out by macro.
- **SIMD16 does not buy registers.** ANV compiles a cooperative-matrix shader SIMD32 unless
  the pipeline asks for a size (`anv_fixup_subgroup_size`); with
  `requiredSubgroupSize = 16` it is SIMD16 and bit-identical, but a matrix takes the same
  bytes at either width, four stages spill just the same, and two stages take twice the
  sends and run 25 % slower.

Little's law says the rest: at two steps a subgroup holds ~2 KB of loads in flight, 64
subgroups hold 128 KB, and at ~1 us of loaded latency that is the 127 GB/s they reach.

## The weights are not what the bottleneck waits for

Reading half of B's bytes (a timing proxy: each load's address halved) on cold weights:
64x4096x1024 105 -> 87 us, 64x3072x1024 84 -> 78, the other two unchanged — 426 -> 395 us a
block, 0.25 ms a frame at best before paying for any decode. Storing the weights as their
E4M3 bytes would buy that; `improve-fusions.md`'s proxy found the same. Neither a 320 MB
footprint of rotating weights (TLB) nor the real weights against random ones moves any
of it. In a frame the four take ~540 us a block against 426 standalone; the epilogues are
not the difference (the gate and E4M3 publish cost nothing measurable) and the rest of it
is not explained.

## Kept: a 32-row block for a bottleneck of 32 tokens or fewer

The staged kernel with `STAGED_BM=32` (and `STAGED_BK=64` where N <= 1024 and K allows it),
routed by libxmx for M <= 32; the bottleneck of 32 tokens or fewer is padded to 32 rows,
not 64. Each row's sums are its own, so the output is the 64-row block's byte for byte
(`src/gpu/test_staged32.py`, 50 cases, and a 32-token case in `test_gemm_qkv.py`).
Standalone, cold weights, the four GEMMs at 32 rows: 32-row block 132 + 109 + 69 + 37 us
against 186 + 107 + 85 + 52 on the 64-row one. Replayed graph, three alternating rounds,
heads unchanged:

| network | before | after |
| --- | ---: | ---: |
| 192x128 | 13.4-14.2 ms | 12.1-13.2 |
| 256x128 | 13.8-14.7 | 13.1-13.5 |
| 256x192 | 16.9-17.9 | 16.6-16.9 |
| 320x192 | 20.1-20.9 | 19.7-19.8 |

Only `min_extent` below 320 reaches it; at the default the bottleneck is 64 tokens and
nothing changes. `XMX_STAGED32=0` is the comparison.

## The staged kernel's K loop, read in its disassembly (tried, not kept)

`INTEL_DEBUG=cs` on the specialised staged GEMM: per 32-deep K step, 8 multiply-adds and
~260 other instructions on the path that runs — and **B's fragments are 16-bit loads**. B
sits in shared memory by row, while a B fragment takes its K pairs a word at a time, so
every pair is two 16-bit loads and a move: 32 loads and 32 moves a step.

- **B by column in shared memory** (`(k, n)` at `n * S + k`) makes the pairs words, and
  spills (7:43). Mesa's lowering reads a lane's words `h + 2i` apart — the two halves of
  the subgroup take alternate K pairs — and in shared memory, where the alignment is known,
  the vectorizer merges them across the gap into `d32x3` loads: six registers for four
  used. Global memory does not merge them (a load may not grow into a new page), which is
  why the same load is clean there. Dense 16x16 tiles, and a word-typed view of the same
  bytes, compile the same.
- **The operands swapped** — C^T = B^T A^T, B by column as the A operand, A's rows as the
  B operand, the output stored by column — is **bit-identical** (the matrix unit sums k in
  the same order whichever operand is which; checked on twelve shapes) and gives a clean
  loop: no spills, 41 moves, 235 lines. And it is **slower**: 256x1536x512 92 -> 108 us,
  576x768x256 59 -> 76, 6400x128x64 37 -> 73 (the transposed store goes element by element).
- Timing proxies, values wrong: B's fragment loads out of the loop entirely, 5-18 % on the
  deep shapes (64x1024x4096 -18 %) and nothing on the shallow ones; A's and B's both,
  10-25 %. The generic loader, never executed on aligned operands, compiled out: 268 lines
  of loop instead of 910, the same time.

So the loop is not bound by its instruction count. By estimate — not measured — a core
moves ~290 KB through shared memory and ~100 KB from L2 per K step, against ~0.5 us of
multiply-adds; traffic and latency set the step. What would cut the traffic per multiply is
a bigger tile per subgroup, and that spills (`phase26`).

## Other block shapes for the staged kernel (tried, not kept)

All bit-identical — a block's shape changes which workgroup computes an element, never the
order of its sums.

- **64 columns to a block**, eight subgroups where four took 32, the same threads a core:
  alone, 3-14 % faster on most shapes (256x1536x512 91 -> 78 us). In a frame nothing:
  1280x768 102 ms of staged GEMM either way, because the published GEMMs gained 5-13 % and
  the residual ones lost 10-23 %. The QKV epilogue would also have needed two heads a block.
- **32-row blocks at the deep levels**, for more workgroups where 64-row ones give 48-64:
  144x512x512 35 -> 30 us and 144x512x2048 124 -> 111, but 256x512x512, 400x256x256,
  400x1024x256 and 576x256x256 slower by 12-50 %. A rule that picks the winners is fitting
  noise for a fraction of a millisecond.
- **The bottleneck's two N = 1024 GEMMs on 32-row blocks with the 64-deep step** (64
  tokens, as at 320x320): 184-186 -> 158-165 us and 50 -> 46 — 0.25 ms a frame. Not routed.
- **More registers for a bigger tile**: none to have. Mesa sizes the register file by
  generation — 128 registers a thread on Xe2; the larger file is Xe3's
  (`brw_alloc_reg_sets`, `ver >= 30`).

What is left inside the graph at the live extent is a long tail of about a percent each.
The daemon at 640x360 and scale 0.5 is 29.7 ms a frame, 26.8 of it the graph at 320x320;
the lever that moves it is the network's extent (`min_extent`), which is the owner's call
because it changes the picture.

## Q, K and V as their E4M3 bytes (tried, not kept)

Every value the QKV epilogue publishes is E4M3, so the window blocks' Q, K and V fit a byte
each: the epilogue packed four to a word, window attention decoded them to halves as it
staged K and V (and Q, into the subgroups' scratch before they need it) — an exact integer
decode, bit-identical frames. The level-1 projection and attention at 1280x768 each move
~190 MB of Q/K/V and ran near 80 GB/s, which read as memory-bound. They are not: with half
the bytes the projection went 3.78 -> 3.93 ms and attention 3.22 -> 3.93, and the graph
155.5 -> 161 ms (1920x1088 328 -> 343.5). Neither pass waits on those bytes; the encode and
decode are what it paid for. A pass running at the machine's bandwidth is not thereby
bandwidth-bound.

## B by pairs of K in shared memory (tried, not kept)

Mesa's 16x16 half B fragment, probed with a known matrix: lane `l` holds column `l % 16`, and
with `h = l / 16` its elements `2i` and `2i + 1` are rows `4i + 2h` and `4i + 2h + 1` — four
pairs of K, a pair a register (the A fragment: rows `l / 8` and `l / 8 + 4`, K pair
`2 (l % 8)`). So B was staged as words holding `B[2p][n]` and `B[2p + 1][n]` and each fragment
filled by element from four word loads, where by row it is eight 16-bit loads: 16 word loads
a step where there were 32 16-bit ones. Bit-identical, and **5-7 % slower in a frame**
(320x320 23.9 -> 25.4 ms, 1280x768 135 -> 144, 1920x1088 281.6 -> 296.4): the compiled kernel
came out with more instructions and more sends, not fewer. The K loop is not bound by its
loads, as the timing proxies above already said.

## A level-1 block whole, its feed-forward inside the window block (tried, not kept)

The one-head blocks' feed-forward writes its float32 output — 67 MB a block at level 1 of a
1920x1088 frame — for the window block to read straight back. Run inside the window block
instead (a flag on `window_block.comp`: each window makes its 64 tokens' feed-forward, the
weights in K's and V's bytes a hidden chunk at a time, the output kept in registers as the
closing residual's skip), it is bit-identical and **no faster**: 1.04x at 160x160, 0.97x at
640x384, 0.95x at 960x544. The traffic it saves is paid back in work: every window loads
all 16 KB of the feed-forward's weights for 64 tokens where `ffn_fused.comp` shares them
among 256, a subgroup takes 8 rows where it took 16 — half the use of each B fragment — and
the chunks need four more barriers a window.

## Two small measurements (2026-09-26, evening)

- **32-row blocks for a 96-token bottleneck** (a 576x352 network): slower, where 64 tokens
  gained. The two N = 1024 GEMMs went 0.246 -> 0.325 and 0.069 -> 0.085 ms a call: a partial
  64-row block wastes a quarter of its rows but reads the weights twice, three 32-row blocks
  read them three times. The rule stays at M = 64.
- **The global blocks' softmax at 640 tokens** (1920x1088): 1.65 ms a call, of which the
  rows' serial sums are 0.43 (taken out, 1.22). Three rows of 641 fit a workgroup's 8 KB, so
  three lanes of 256 sum while the rest wait; the other 1.2 ms is near the memory floor of
  the pass itself — 52 MB of float32 scores in, 26 MB of half out, ~1 ms at 80 GB/s. What
  would pay is not writing the scores at all: QK^T, the weights and PV in one kernel, each
  row's sum still taken in key order — about 7 % of a 1920x1088 frame, growing with the
  square of the tokens, and nothing at the live extents. Not written.
