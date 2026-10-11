# Phase 53 — why the live picture flickers, measured

2026-09-11. The owner watched the live mode and said the picture shimmers. It does, and
the cause is not the noise, not the frame rate and not the render scale.

## The measurement

Twenty-two consecutive frames captured from a live fight at 1024x768, scale 0.55. For
each consecutive pair, how much the *input* moved against how much the *output* moved:

| pixels | input Δ | output Δ | amplification |
| --- | --- | --- | --- |
| **byte-identical between frames** (2.3 % of the frame) | 0.000 | **3.33** | out of nothing |
| \|Δ\| < 2 | 0.43 | 2.88 | **6.7x** |
| \|Δ\| < 4 | 1.22 | 3.12 | 2.6x |
| \|Δ\| > 20 — actually moving | 49.4 | 49.8 | **1.01x** |

A pixel that **did not change at all** comes back 3.3 levels different. Where there is
real motion there is no amplification whatsoever. **The instability is exactly where the
scene is still**, which is what the eye reads as shimmer — a moving object is expected to
change, a static crowd is not.

Whole-frame amplification is only 1.06x, which is why this did not show up in any earlier
measurement: averaged over a fight it disappears into the motion.

## Why

The pass is deterministic — identical input gives identical output — so a pixel changing
while its own bytes did not means **its context changed**. The network is global: five
downsampling stages into a ViT-1D bottleneck whose attention sees the whole frame. A
fighter moving in the middle changes the bottleneck's tokens, and the decoder's output
moves *everywhere*, including over a crowd that did not move a pixel.

That is not a bug in the port. It is what a frame-independent pass over a global
architecture does, and it is the reason the vendor's own description of DLSS-NR says
"deterministic and **temporally stable** by design" — the stability is not a property of
the network, it is supplied by the temporal path.

## The fix already exists in this repository and is not wired in

`phase12` built and measured the temporal path: the previous output, reprojected along
motion vectors, into feature channels 7-9, blended back through the head's fourth channel.

| | gate |
| --- | --- |
| history reprojected with correct motion | **0.705** of a 0.7397 ceiling |
| history with zero motion, scene panning | **0.032** — a 22x rejection |

And what it bought: **flicker falls 3.6x by the third frame, 6.3x at the peak.**

Neither game mode supplies it. Channels 7-9 currently carry the current colour, which is
what a first frame should hold, and the daemon keeps no history.

## The proposal this measurement makes obvious

A Vulkan layer at `vkQueuePresentKHR` has no motion vectors and never will. But **zero
motion is the correct motion for a static region**, and static regions are exactly where
the flicker is. Feeding the previous output as history with identity reprojection would
be right where it matters and rejected by the gate where it is not — the 0.032 row above
is that rejection, measured on a panning scene.

Cost: no extra network work, the channels are already there. The daemon has to keep one
previous output and the layer has to not interleave two swapchains. Risk: ghosting if the
gate is less discriminating at this render scale than `phase12` measured at full scale,
which is itself worth measuring.
