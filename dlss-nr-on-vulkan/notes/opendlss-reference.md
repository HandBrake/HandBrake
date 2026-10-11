# A reference that claims the vendor's arithmetic: OpenDLSS-NR (2026-09-27)

`maanHimself/OpenDLSS-NR` (MIT, published 2026-09-21) implements the same 71-block network on
NVIDIA Ada in Vulkan with FP8 on the tensor cores, and claims it **bit-exact against captures of the
original**: every one of 75 block boundaries, byte for byte, at 512x512 and up to 3840x2160, and the
head and composed image at eleven sizes. Its `ports/browser-webgpu/` is a second implementation of
the same bytes in WGSL, with no FP8 and no tensor cores — "the exactness is in the specification, not
in the hardware". Neither the captures nor the weights are published, so the claim is theirs; what
can be checked here is that their scalar arithmetic self-test passes on this GPU, and it does.

Where it came from, as far as the record shows: one `init` commit with the whole project, a second
titled "Fix review findings", four commits in all; the author's other repositories are web 3D
(three.js, webgi), and a fork of the Codex CLI from June. It cites neither MLX-DLSS nor this tree
and shares no code with either. "Publish" for an E4M3 rounding point is common vocabulary, from
MLX-DLSS.

## Their specification, against ours

`docs/numerics.md` and `docs/network.md` there. What this tree does differently, each checked in our
code:

- **the padded field.** Theirs aligns each axis to the number of halvings the graph makes, with an
  extra `+ alignWidth` when both axes are multiples of four alignments (a rule they reproduce without
  a reason). Ours rounds to 64 (`nr_frame.network_geometry`). They agree at the live sizes (320x180 ->
  320x320, 448x252 -> 448x320, 576x324 -> 576x384, 1056x594 -> 1088x640) and not at 1280x720
  (1280x768 here, 1344x768 there), 1920x1080 (1920x1088, 1920x1152), 512x512 or 768x768 — where the
  window grid, and so the picture, differ;
- **the GEMMs**: FP8 products summed in groups of 16 as fixed point with 13 fractional bits,
  truncated, onto an f16 accumulator the residual seeds; the ViT's GEMMs split into partitions summed
  in f16. Ours: fp16 x fp16 -> fp32, the residual added after;
- **the softmax denominator**: a fixed tree of half adds, against our sequential float32 sum (numpy's,
  which MLX-DLSS's reference inherits);
- **the ViT**: its own exponential (`0.0895 s + 1.709`, clamp `[1.4395, 1.9775]`, 4-bit shift), queries
  times `sqrt(32)`, unnormalised weights with the reciprocal on the value accumulator, and a padding
  term taken off the denominator. Ours runs the window exponential with the logits clamped to ±3;
- **E4M3 from half**, never from float32 directly; ours rounds float32 straight to E4M3;
- and, not checked here: the history stored truncated to half, and noise from Box-Muller on a hash of
  the padded coordinate.

## Measured: our head against theirs on the same features

`src/tools/opendlss_model.py` writes their model directory from the carved container (the same 153
records, copied as they are); `src/bench/opendlss_reference.py` feeds both networks the daemon's
features for one frame and compares. Their port runs here in headless Chromium on the Arc 140V — 0.55 s
at 320x320, 3 s at 1088x640, against our 23 ms and ~100 — and SwiftShader instead unless Chromium is
told `--ignore-gpu-blocklist --use-angle=vulkan --use-vulkan=native`.

| frame, field | head RGB corr | gate logit corr | composed apart, mean / 99th pct | the pass itself |
| --- | --- | --- | --- | --- |
| Cyberpunk, 320x180 on 320x320 | 0.971-0.976 | 0.86 | 1.13 / 7.2 levels | 4.4 / 4.2 levels |
| DoA5, 320x180 on 320x320 | 0.971-0.976 | 0.65 | 2.74 / 9.6 | 11.2 / 12.5 |
| DoA5, 1056x594 on 1088x640 | 0.988-0.992 | 0.81 | 1.37 / 7.4 | 11.1 / 10.9 |
| Cyberpunk, 1056x594 on 1088x640 | 0.988-0.990 | 0.81 | 1.45 / 8.7 | 8.5 / 7.9 |

For scale, our own graph against itself — the GPU path against the numpy reference, same features,
Cyberpunk at 320x320: RGB corr 0.985-0.994, gate 0.94. So the distance to theirs is about twice what
our two arithmetics make of the same graph, and the gate moves most: its spread differs (sd 0.85 ours,
1.13 theirs, 0.98 our numpy). The pictures look the same side by side; amplified eight times, the
difference is fine texture and a faint tone on the faces, no window seams, no structure.

## Block by block: the encoder skips are one block early

`src/bench/opendlss_blocks.py` runs each of our blocks — the numpy reference — on the boundary the
reference fed its own copy of that block, and compares the published output byte for byte
(Cyberpunk, 320x180 on 320x320):

| blocks | bytes equal | mean \|d\| of the value |
| --- | --- | --- |
| window blocks, 1-22 and 49-69 | 61-88 % | 0.6-2.3 % |
| the 512 split blocks, 23-30 and 40-47 | 55-81 % | 1.2-3.2 % |
| the ViT, 32-38 | 52-56 % | 3.0-3.4 % |
| block 0, from the features | 57 % | 3.0 % |
| block 39, the decoder's input merge | 99.3 % | 0.04 % |
| **decoder blocks 48, 56, 62, 66 with our skips** | **18-25 %** | **11-16 %** |
| the same with the transition block's output as the skip | 61-69 % | 1.7-2.6 % |

**Our decoder merges the wrong skip.** At every level ours (and MLX-DLSS's `model.py`, which this
tree ports) takes the skip after the last regular block, before the transition block — blocks 3, 7,
13, 21 — and the reference takes the transition block's own published output, 4, 8, 14, 22, as its
graph says and its claimed captures would require. With theirs the first decoder blocks agree like any
other block; with ours they are the four worst in the graph. Fixed in the numpy reference, the head
moves towards theirs — RGB corr 0.958/0.958/0.973 -> 0.962/0.960/0.973, the gate 0.892 -> 0.947 —
less than the per-block distance suggests, because everything else still differs in rounding and the
graph amplifies it: what remains is spread along the whole graph, most in the ViT.

**Fixed on 2026-09-27, in the numpy reference and the GPU path alike** (the transition block's output
published into the level's buffer, one small pass a level; `test_against_torch.py` gives MLX-DLSS's
model the same skips and stays bit-identical everywhere else). The GPU path against the reference
again, same four frames:

| frame, field | head RGB corr | gate logit corr | composed apart |
| --- | --- | --- | --- |
| Cyberpunk, 320x320 | 0.971-0.976 -> 0.974-0.981 | 0.86 -> 0.92 | 1.13 -> 1.04 levels |
| DoA5, 320x320 | 0.971-0.976 -> 0.979-0.980 | 0.65 -> 0.89 | 2.74 -> 2.52 |
| DoA5, 1088x640 | 0.988-0.992 -> 0.990-0.993 | 0.81 -> 0.93 | 1.37 -> 1.33 |
| Cyberpunk, 1088x640 | 0.988-0.990 -> 0.990-0.991 | 0.81 -> 0.92 | 1.45 -> 1.36 |

The temporal gate moved most — the channel live mode blends the history by.

## Step by step: six more places the graph was MLX-DLSS's, not the vendor's (2026-09-27)

A block boundary compares a whole block, where four or five GEMMs' worth of rounding hide what is
structural. So the reference port now records inside blocks too — the feed-forward's output in both
its forms, the QKV projection, the attention's output, block 30's raw output, the pool into the ViT
and its projection, the merges into blocks 66 and 70 (`src/bench/opendlss_captures.patch`, a
capture-only patch to their `graph.js`) — and each of our steps is run on the reference's own input
to that step (`src/bench/opendlss_steps.py`). A step that differs only in rounding agrees on most values; one
that computes a different thing agrees on almost none. Cyberpunk, 320x180 on 320x320:

| step | ours, as MLX-DLSS had it | the reference's structure |
| --- | --- | --- |
| a 32-channel block's QKV projection, from the feed-forward output | **0.8 %** equal (raw input) | **92.8 %** (published input) |
| block 0's feed-forward, from the adapter | **2.2 %** (half input) | **51.1 %** (published input, half skip) |
| block 66's feed-forward, from the merge | **2.3 %** (published skip) | **53.5 %** (raw skip) |
| block 70's feed-forward, from the merge | **1.5 %** (raw input) | **48.0 %** (published input, raw skip) |
| the pool into the ViT, from block 30 | 86.6 % (published output) | **100.00 %** (raw output, half adds) |
| the ViT's input, the pool projected | 76.4 % (no publish before the GEMM) | **99.8 %** (published) |

(The 48-53 % are half values, which a GEMM's rounding moves more often than an E4M3 byte.) Three
more, measured a block at a time: the 512 split blocks and the ViT publish their feed-forward output
before the attention reads it, as the 64-256 channel blocks already did — 55-81 % of bytes equal ->
60-96 % for the split blocks, 52-56 -> 60-65 % for the ViT — and the ViT's attention is its own, not
the window blocks' with the logits capped: 60-65 -> 63-69 % (block 31, from the reference's pool:
45 -> 51 %). The rule under all of it is the reference's `numerics.md`: **every GEMM operand is
E4M3**; half carries only accumulators and the 32-channel blocks' skips. MLX-DLSS's graph fed four
GEMMs a raw value.

All six are in the numpy reference (`nr_model.MLX_DLSS_GRAPH` restores MLX-DLSS's graph, and
`test_against_torch.py` compares that one with its PyTorch original, bit for bit) and on the GPU
path, every fused and unfused route of it — the published input in the staged GEMM's window gather
and in `window_block.comp`, the stem and the merge in `ffn_fused.comp`, a second, published output
on the decoder merge for block 66, block 30 into the bottleneck raw, and the ViT's attention in
`global_attention.comp` and in its four unfused passes, which the fused kernel is bit-identical to.
With the normalisation after the value sum, the ViT's attention is one trip through the keys where
the window-style softmax took two. Every switch still gives the same head as every other, both
memory modes, and the speed is the same (320x320 23.5 ms, 1920x1088 270 ms of graph; the daemon
25 / 32 / 47 ms at the three live sizes).

The numpy reference, block by block on the reference's inputs, now: 75-94 % of bytes equal for the
32-channel blocks, 67-86 % at 64-256 channels, 60-96 % for the split blocks, 63-69 % for the ViT —
what is left is arithmetic. And the GPU path against the reference, same four frames:

| frame, field | head RGB corr | gate logit corr | composed apart | the pass: ours / theirs |
| --- | --- | --- | --- | --- |
| Cyberpunk, 320x320 | 0.974-0.981 -> **0.987-0.990** | 0.92 -> **0.96** | 1.04 -> **0.72** levels | 4.02 / 4.19 |
| DoA5, 320x320 | 0.979-0.980 -> **0.984-0.989** | 0.89 -> 0.89 | 2.52 -> **1.85** | 12.61 / 12.51 |
| DoA5, 1088x640 | 0.990-0.993 -> **0.996-0.997** | 0.93 -> **0.95** | 1.33 -> **0.79** | 10.85 / 10.87 |
| Cyberpunk, 1088x640 | 0.990-0.991 -> **0.995-0.996** | 0.92 -> **0.95** | 1.36 -> **0.53** | 5.80 / 5.80 |

Against where this began, before the skips: 1.13-2.74 levels apart, now 0.53-1.85. That is about
the distance our own two arithmetics make of one graph (the GPU path against the numpy reference,
`test_resident.py`: head corr 0.98 at 320x320), so the structure is as far as a comparison of
heads can see; the rest is arithmetic, and a single frame's head moves by as much between two
float32 GEMM blockings of the same graph (0.04 in the gate's correlation, 0.2 levels composed).

## The padded field: a whole class of frames drew a weaker pass (2026-09-27)

The features were never the question — our noise is the reference's generator (the same hash, the
same Box-Muller, the last bits of the transcendentals apart), and the mirror, the colour's three half
roundings and the control lanes are its too. The field they fill was. MLX-DLSS padded each side to a
multiple of 64 and at least 320; the reference aligns each side to the graph's own reductions and adds
one alignment to the width when both sides are four alignments (`geometryFromValid`, a rule it
reproduces "because it moves the window grid"). At 1280x720 the two give 1280x768 and 1344x768.

**Every field whose sides are both multiples of 256 draws a pass 25-30 % weaker than any field around
it.** One DoA5 frame, the pass's mean change in levels of 255, our GPU path on the frame resampled to the
first size and padded to each field:

| frame | fields with both sides a multiple of 256 | the fields beside them |
| --- | --- | --- |
| 1024x768 | 1024x768: 7.95 | 1088x768 10.74, 1024x832 11.07, 1152x768 10.68 |
| 768x512 | 768x512: 7.75 | 832x512 11.50, 768x576 11.46, 896x512 11.50 |
| 512x512 | 512x512: 8.63 | 576x512 11.55, 512x576 11.13, 640x512 11.64 |
| 1280x1024 | 1280x1024: 7.83 | 1344x1024 10.47, 1280x1088 10.53 |
| 1536x768 | 1536x768: 7.84 | 1600x768 9.79, 1536x832 10.05 |
| 1280x720 | 1280x768: 7.73 | 1344x768 10.53, 1408x768 10.44, 1280x832 10.69 |

Such a field pools down to a bottleneck with no padding token in it — every halving exact, no
row or column of zeros at levels 4 and 5 — and that is the only thing the weak fields share; the
amount of mirrored padding is not it (1024x768 has none, 768x768 for a 700x720 frame has plenty, both
weak). The reference's extra alignment is exactly the step that keeps the common sizes out of it,
and it is the network's, not ours: at 1153x642, where the reference's rule itself lands on 1280x768
(both sides align to 128 there, and its extra step needs four of those), its pass is as weak as ours
— 7.44 against 7.42 levels, 4.53 against 4.51 — and the two agree to 0.54-0.76 levels.

What that did to this tree: 1280x720 at render scale 1.0 ran on 1280x768. Ours on our field against
the reference on its own: **4.28 and 8.41 levels apart, head RGB corr 0.41-0.76** — against 0.48-0.75
levels and 0.996-0.997 for ours on the reference's field. And it is most of why 0.9 looked better than
1.0 at 720p (HANDOFF, 2026-09-25): three DoA5 frames through the daemon now change as much at 1.0 as at
0.9 (0.0396 / 0.0399, 0.0143 / 0.0143, 0.0218 / 0.0209 of luma) where the 1.5-1.7x gap was measured;
what is left is the grain — 1.5-3x more of the added energy above half Nyquist at 1.0.

`nr_frame.network_geometry` is the reference's rule now, `min_extent` its floor. Below 129 pixels a
side the rule need not give a multiple of 64, which this graph's exact halvings cannot follow, and it
is rounded up there. The published live sizes keep their fields except 1024x768 at 0.55 (563x422:
576x448 -> 640x512, 48.5 -> 56.5 ms); at the graph's floor a 320x180 frame runs at 320x256, not 320x192.

## The GEMMs' arithmetic, and a reference that was not rounding here (2026-09-27)

**The reference port did not compute the vendor's arithmetic on this GPU.** Its GEMM kernels round
their f16 accumulator with WGSL's `f32(f16(x))`, and Mesa folds that round trip away: run on the
port's own captured inputs, a numpy transcription of its specification (`src/bench/vendor_fp8.py`:
groups of 16 products, 13-bit truncation, the accumulator rounded to half after each group) matched
its QKV projection on 74.5 % of values, and one that never rounds between the groups matched it on
100 %. With the round trip made unfoldable (`src/bench/opendlss_rounding.patch`, `pack2x16float`, as
this tree's own `half_round` does) the specification matches the port on **100 % of values on block
1's QKV projection and on its whole feed-forward** — the SiLU between and the residual seeding the
accumulator included. So the emulation is exact, and the port is a faithful reference here only
patched. The patched port against our GPU path, same frames: 0.91 / 1.99 / 0.80 / 0.47 levels
apart — about what the unpatched one gave; the structural findings above stand either way, being
tens of times larger than the difference.

**XMX's half accumulator is closer to that arithmetic than our float32, and the picture does not
care.** Real GEMMs of the graph, the reference's inputs, the share of half results equal to the
vendor's (in brackets, of E4M3 publications):

| GEMM | K | float32 accumulator (ours) | half accumulator |
| --- | --- | --- | --- |
| block 1 QKV | 32 | 72.5 % (99.68) | 95.0 % (99.95) |
| block 1 expand | 32 | 71.9 % (99.71) | 97.6 % (99.98) |
| block 5 QKV | 64 | 61.2 % (99.26) | 88.6 % (99.80) |
| block 9 QKV | 128 | 43.2 % (98.66) | 79.2 % (99.52) |
| block 15 QKV | 256 | 36.6 % (98.47) | 68.5 % (99.26) |
| block 23 QKV | 512 | 47.2 % (98.20) | 67.7 % (98.88) |
| block 32 expand | 1024 | 21.5 % (97.92) | 31.1 % (98.24) |

Our float32 accumulator is the exact sum rounded once, to the value. The half one is the hardware's
own 16-deep step rounded to half each time, which is the vendor's grouping less its truncation. But
put through the whole numpy graph — every GEMM accumulated that way — the head moves towards the
reference by a tenth: composed 0.78 -> 0.70 and 2.09 -> 1.91 levels on two frames, within what a
change of rounding anywhere makes of one frame. The graph's other arithmetic differences dominate.
Porting it would touch every GEMM kernel, with the residual moved into the accumulator's seed and the
ViT's split-K partitions added, for no visible change and no speed (a half accumulator measured 1.36x
on an isolated GEMM and nothing in a frame, `phase31`). Not done.

## What still differs

Arithmetic, all of it: the GEMMs (FP8 products as 13-bit fixed point onto an f16 accumulator the
residual seeds, the ViT's split-K partitions — against fp16 x fp16 -> fp32 on XMX); the window
blocks' cosine norm, which pairs channels (c, c+16) where ours pairs (c, c+8), and their softmax
denominator, a fixed half tree over the keys in 4x4-tiled order where ours sums in float32 — the two
together 0.2-1.4 points of the attention's bytes in the step test (97.9 -> 98.1 % at one head, 95.4
-> 96.8 % at eight); the pools' and merges' half roundings; E4M3 always by way of half. Of these only
the norm and the denominator are cheap, and they are worth little. Not arithmetic, and not measured: the history stored truncated to half.
