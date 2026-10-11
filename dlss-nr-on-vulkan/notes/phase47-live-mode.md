# Phase 47 — a live mode, and the bottleneck moving out of the network

2026-09-10. Asked for a slideshow in gameplay at 10 fps or better — render small,
stretch to 1080p — and for a tool to drive the thing without restarting it.

## 10 fps is reached, at 512x288

Measured end to end, the whole round trip through the socket rather than the graph alone:

| swapchain | render scale | ms | fps |
| --- | --- | --- | --- |
| 854x480 | 0.35 | 176.6 | 5.66 |
| 640x360 | 0.35 | 116.8 | 8.56 |
| **512x288** | **0.35** | **98.6** | **10.15** |

Set the *game* to that size and the compositor stretches the result to the panel for
nothing — no resampling of our own, and the game renders fewer pixels too. 512x288 to
1080p is a 3.75x linear stretch and looks it; that was the accepted price.

The non-monotonic rows (720x405 slower at 0.35 than at 0.50) are the network's alignment
padding, which some extents land badly on. The same bump shows in the raw extent curve:
512x288 costs 155 ms of graph where 640x384 costs 146.

## The bottleneck is no longer the network

This is the finding. `phase45` accounted for the frame and concluded nothing inside it
was waiting on a better kernel. Shrink the extent and the network stops being the frame:

| stage | 854x480, scale 0.35 |
| --- | --- |
| **head upscale** | **192.8 ms — 55 %** |
| network | 69.7 ms — 20 % |
| compose | 40.1 ms — 11 % |
| feature assembly | 25.7 ms — 7 % |
| downscale, decode, encode | 24.1 ms — 7 % |

The head upscale was **my own resample**, written two hours earlier in the obvious
two-dimensional form:

```python
top    = image[y0][:, x0] * (1 - wx) + image[y0][:, x1] * wx
bottom = image[y1][:, x0] * (1 - wx) + image[y1][:, x1] * wx
```

Four full-size fancy-index gathers with their temporaries. Separating the axes — one
pass down the rows, then one across the columns, two gathers of the intermediate size —
takes it from **193 ms to 15.5 ms** on an 854x480 head and from 3133 ms to 60 ms at
1080p. Verified equal to the old form to 1e-6.

Round trip at 854x480 went 255 ms -> 174 ms on that one change.

What is left is numpy at the *output* resolution: composition 40 ms and feature assembly
26 ms, neither of which shrinks with the render scale. That is why the swapchain size
matters as much as the scale, and why 512x288 reaches 10 fps while 854x480 at the same
scale does not.

## What was built

**`--render-scale` in the daemon.** The network runs on a fraction of each side; what
comes back up is the *head* — the detail the network drew — which is then composed
against the full-resolution original. The game's own pixels are never resampled and only
the synthesised part is interpolated. That is the best arrangement available without a
neural upscaler (`phase37`).

**Live mode in the layer**, `NR_LAYER_LIVE=N`: every Nth present goes through the network
and the frames between re-blit the last result, so the picture is steady rather than
alternating with the game's own. It respects the trigger file, so the effect can be
switched on and off mid-game. The interface mask is not used — its detector is built
around a frame that was asked for.

**`src/layer/nr-ctl`**, interactive or one-shot. The daemon re-reads a small JSON file
whenever it changes, so profile, intensity, the two strengths and the render scale all
move between frames without restarting the model. `nr-ctl rates` prints the table above.

A size guard went onto the photo-mode hold at the same time: it re-blitted a cached
result without checking it still matched the swapchain extent, which a resize mid-hold
would have broken.
