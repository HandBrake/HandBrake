# Phase 25 — what frame rate is actually reachable, measured rather than extrapolated

**Historical measurements:** phase27 shader specialization and phase28 command replay
change both kernel costs and fixed overhead. Do not use this fit as the current
performance model; see `notes/phase28-frame-replay.md`.

2026-09-09. Every note in this project that has talked about frame rate has done it by
extrapolation: take the arithmetic, take the bandwidth, assume the kernel improves,
divide. `src/bench/extent_curve.py` measures it instead — the real graph at nine network
extents, best of five consecutive frames each.

| network extent | pixels | ms | fps | ms per megapixel |
|---|---|---|---|---|
| 448x256 | 114 688 | 102 | 9.8 | 890 |
| 512x320 | 163 840 | 132 | 7.6 | 806 |
| 640x384 | 245 760 | **191** | **5.2** | 776 |
| 768x448 | 344 064 | 228 | 4.4 | 662 |
| 1024x576 | 589 824 | 361 | 2.8 | 612 |
| 1280x768 | 983 040 | 649 | 1.5 | 660 |

A straight line fits it to 15 ms:

```
frame = 20 ms  +  632 ms per megapixel
```

## What that means for the target

The owner's target was 30 fps at 720p, later softened to "30 fps at some resolution, at
least in DX9 games". Against the measured line:

| target | budget | extent it would need |
|---|---|---|
| 60 fps | 16.7 ms | **impossible** — the fixed cost alone is 20 ms |
| 30 fps | 33.3 ms | 21 kpixels — about **194x109** |
| 15 fps | 66.7 ms | 74 kpixels — about 363x204 |
| 10 fps | 100 ms | 127 kpixels — about 475x267 |
| 5 fps | 200 ms | 285 kpixels — about 712x400 |

**HANDOFF said 30 fps needed "about a 640x384 extent". That was wrong by a factor of
six** — 640x384 measures 191 ms, which is 5.2 fps. The estimate came from taking a
720p frame's arithmetic and assuming a well-optimised kernel; the kernel is now largely
optimised (1025 -> 649 ms, and both remaining levers measured null in phases 22 and 23)
and the answer did not move nearly far enough.

The slope is the wall, and it is not an implementation problem. 30 fps at 720p means
460 GFLOP in 33 ms — **14 TFLOP/s effective, 44 % of this GPU's FP16 peak, with the
elementwise half of the frame costing nothing.** Half the frame is not GEMM.

## Where the 20 ms of fixed cost is

Worth knowing because it is the part that does not shrink with resolution:

- **73 submits per frame, at 101 us of round trip each = 7.4 ms.** Measured directly:
  73 empty begin/submit/fence cycles cost 7.39 ms on this driver. The submits exist
  because the frame is recorded in block groups with host-side copies between them —
  the encoder skips and the bottleneck's edges. Making those device-side copies would
  let the whole frame record as one command buffer and take most of this back.
- **~8.6 ms of host work outside the submits** — recording 1848 dispatches through
  ctypes, and the skip copies themselves. At 448x256, 8.6 ms of a 147 ms wall.

Both are worth roughly 1 % at 720p and 15 % at 448x256, which is the wrong end of the
curve to be optimising.

## The honest standing

**On this hardware, with this graph, DLSS-NR is a photo mode.** One frame in half a
second at 720p, a fifth of a second at 640x384. That is a genuinely useful thing — the
Vulkan layer makes it work inside a real game (`notes/phase24-a-real-game.md`) — but it
is not a real-time filter and no amount of kernel work will make it one. Reaching
playable rates would need a materially smaller network: fewer than 71 blocks, or
narrower ones, which means a different model rather than a faster port of this one.
