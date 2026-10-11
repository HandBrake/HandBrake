# The QKV projection finished in its own epilogue: 22 % of a frame, bit-identical

2026-09-23. Q and K are cosine-normalised and V published inside the QKV projection's GEMM,
so the float32 projection never goes to memory and the three passes that read it back are
gone. With the head merge folded into the fused attention the same day
(`improve-fusions.md`), all five fusions together take a 1280x720 frame from 458 to 284 ms.

## How it was found

The question was a different one: whether fusing the feed-forward of the narrow C=32 blocks
was worth building. Block 0 at 1280x768, pieces recorded separately with GPU timestamps:
feed-forward 12.6 ms (28 %), attention path 34.5 ms (75 %), of 45.7. So not the FFN.

The whole frame at 1280x768 after Codex's fusions was 382.0 ms on the GPU, and the largest
thing in it after GEMM was the preparation of Q, K and V: **cosine publish 69.0 ms** over 140
passes and **split heads 19.1 ms** over 70. Taken apart at block 0 (983 040 rows, C=32):

| pass | ms | MB moved | GB/s |
| --- | ---: | ---: | ---: |
| QKV GEMM, float32 out (as it was) | 6.29 | 440 | 70 |
| the same GEMM, half out | 3.02 | 252 | 83 |
| cosine Q from the float32 projection, through the QKV gather (as it was) | 5.00 | 189 | 38 |
| cosine Q from contiguous float32 | 3.73 | 189 | 51 |
| cosine Q from contiguous half | 3.23 | 126 | 39 |
| split V (as it was) | 2.93 | 189 | 64 |

The GEMM is memory-bound and half its time was writing float32. The cosine pass has a floor
of about 3.2 ms per 31.5 M elements that is arithmetic, not memory. Of the ~19 ms of
preparation, about 10 were the float32 intermediate itself: 377 MB written once and read
three times.

That also explains `improve-joint-qkv.md`, where one dispatch for the three passes came out
**5.3 % slower**: it read the same float32 projection. The dispatches were never the cost.

## What it does

Both GEMM paths that serve the QKV projections — the register-tiled 16x32 block (C below
128, and the bottleneck's 240 rows at 720p) and the staged 64x32 one — have a column block
32 wide and 32-aligned: exactly one head of one of Q, K or V. `qkv_epilogue.glsl` stages the
raw accumulator block in shared memory, one invocation per row normalises its row in place,
and then every invocation publishes four consecutive elements into the
(window, head, token, 32) target.

**Bit-identical by construction.** The input is the same float32 accumulator the old path
wrote out, and the reciprocal norm is `cosine_tree.glsl` — now the one definition, which
`attention.comp`'s row pass includes too, so the two cannot drift. `test_gemm_qkv.py`: 48
cases on both paths, both specialization masks, zero rows (the norm floor), squares that
overflow half, guard values past each target, and every element checked for having been
written. Two bugs planted on purpose — V without its E4M3 publish, and Q without the half
rounding before the multiply — failed it at 12 213 and at 26 elements of 12 352.

Operands: `c` is Q, `d` is K, `residual_cos` is V, and the query scale is a new field at
offset 120, which makes the push block 128 bytes — the size every Vulkan device must
support. `image_h`/`image_w` carry tokens and heads. Flag `0x100000`, graph-key bit 12,
`NR_QKV_EPILOGUE=0` restores the four passes.

## Two traps, one of them sprung

**Shared memory is allocated in powers of two.** The first version kept the per-row
reciprocals in a 64-byte shared array beside `stage`. The tiled block's `stage` is exactly
2 KB, so every tiled GEMM's workgroup went to 4 KB, and half as many fit on a core — for
*every* tiled GEMM in the graph, with this epilogue or without it, because the array was
declared in the shader they all share. The specialised code was identical to the
instruction (Mesa's `INTEL_DEBUG=cs`, nine variants, the same counts and no spills); the
tiled family went from 85.4 to 108.9 ms of a 720p frame all the same. It was caught by
profiling the frame with the epilogue *off* and comparing against the morning's profile,
then confirmed by swapping the old `gemm_tiled.spv` back in through `XMX_TILED_SPV`. Rows
are now normalised in place, in `stage`, and the epilogue has no shared memory of its own.
The staged shader never showed it: 15 872 bytes plus 256 stayed inside 16 KB. **Before
adding shared memory to a shader, check which power of two its total crosses.**

**A fusion moves a write into an earlier dispatch, so the scratch arena has to be
re-checked.** `k16` shares the `input_key` role with `win16` and `ffn16` — the QKV
projection's own input. The float32 path could write it because by then the projection had
finished; the epilogue writes K while other workgroups are still reading that input. So in
this mode K goes into `key16`, a new name in the projection's role, which nothing uses here:
its owner was the float32 projection. No memory is added.

## Measured

Paired, alternating, `NR_QKV_EPILOGUE` off against on, the head merge on in both, every head
bit-identical:

| output | off | on | gain |
| --- | ---: | ---: | ---: |
| 384x384 | 64.92 ms | 52.96 ms | 18.4 % |
| 1280x720 | 364.89 ms | 286.10 ms | **21.6 %** |
| 1920x1080 | 780.39 ms | 639.11 ms | 18.1 % |
| 1280x720, `XMX_STAGING=1` | 349.32 ms | 279.53 ms | 20.0 % |

210 dispatches fewer, 802 -> 592. A real frame — Cyberpunk 2077 at 1920x1080 through
`nr_frame.py --resident` — is identical in all 2 073 600 pixels either way, and to the
render made before any of the day's changes.

The profile at 1280x768 afterwards: **279.1 ms** on the GPU, from 382.0. Cosine publish and
split heads are gone, merge heads went from 16.3 ms to 0.16 (only the eight bottleneck blocks
keep theirs), and GEMM barely moved: tiled 85.7 -> 80.8 ms and staged 81.1 -> 89.3, so
3.4 ms more in all, against 104 ms of passes removed. In the tiled path the epilogue costs
less than the float32 store it replaces.

All five fusions, off against on in one process, paired, on a freshly booted machine (swap
empty, no memory pressure):

| output | all off | all on | gain | dispatches |
| --- | ---: | ---: | ---: | --- |
| 384x384 | 79.6 ms | 52.2 ms | 34.3 % | 1128 -> 592 |
| 1280x720 | 458.5 ms | 284.3 ms | **38.0 %** | |
| 1920x1080 | 980.2 ms | 599.7 ms | 38.8 % | |

The extent curve is now **10.0 ms + 274 ms per megapixel** of network extent — seven extents
from 320x320 to 1920x1088, worst residual 5.2 ms — against `phase60`'s 15 + 449. The README's
live table was re-measured with it: 512x288 at scale 0.35 went from 72 to 53.9 ms (18.6 fps),
1024x768 at 0.55 from 168 to 121, and 1920x1080 at 0.55 from 412 to 280, three runs within
3 % of each other.

**Measure on a machine with empty swap.** The same code, hours earlier, with 5.5 GiB in zram
and the kernel's memory-pressure figures rising: 1920x1080 live ran 452, 463 and 322 ms in
three runs, the extent curve came out 8.6 + 280 with a worst residual of 8.9 ms, and the
paired frames read 0-4 % slower, the most at 1920x1080. The small extents barely moved; the
large ones are where the pressure shows. `cat /proc/pressure/memory` and `swapon --show` before a run.

The benchmark showed the head read slower in the "on" mode (1080p 8 -> 18 ms). A direct
probe, twelve alternating frames at 1080p: 7.49 against 7.45 ms. It belongs to the
benchmark's own allocations, as it did for the merge; the graph time is what moved.

## The conclusion this retires

`phase45` found every pass that only moves data at the memory ceiling, and the brief and the
README concluded that the graph was finished as an optimisation target. Each pass was
efficient; the graph was not finished. **A pass at the memory ceiling that need not exist is
all waste**, and a per-pass profile answers whether a pass is efficient, never whether it
should be there. Removing passes is where 38 % of the frame was.

## What is left

- Window attention, 58 ms at 1280x768, is now the largest pass that is not a GEMM.
- `partition` (17.5 ms) and `to_half` (10.3 ms) both read what the previous epilogue has
  just written; either could be that epilogue's second output.
- The staged QKV epilogue adds 8 ms. Its reduction runs one invocation per row, 64 of 128.
- Codex's `window_attention_qkv.comp` normalised Q and K inside the attention instead. It
  reads the float32 projection, which no longer exists, so it is superseded here.

## Reproduce

```sh
make
python3 src/gpu/test_gemm_qkv.py                  # 48 cases, both GEMM paths
XMX_STAGING=1 python3 src/gpu/test_gemm_qkv.py
python3 src/gpu/test_joint_qkv.py                 # whole frames, all four preparations
python3 src/bench/ffn_batch.py --optimization qkv-epilogue --size 720 1280 --pairs 8
python3 src/bench/frame_profile.py --size 768 1280
```
