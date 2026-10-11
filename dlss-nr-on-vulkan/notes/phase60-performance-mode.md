# Phase 60 — the performance mode buys this workload nothing

> **Correction, then re-measured.** The first numbers below were taken with Steam replaying
> Counter-Strike 2's 6.5 GB shader cache — two `fossilize_replay` workers at ~98 % of a core
> each, unnoticed. Flagged the same afternoon, then **re-measured on an idle machine**, and
> the result is in "Idle, three processes" below. In short: the **graph** numbers held — it
> is GPU-bound and two busy cores barely touched it, within 1-5 ms at every extent — while
> one of the two **whole-frame** times, 245 ms, was the background load; idle it is 209-210.
> The conclusion did not change.

2026-09-16. The machine was switched to its performance mode — in Windows, through a
firmware setting Linux cannot reach, and in Linux as well. The question was whether the
graph got faster. **It did not, and the reason is measured, not assumed.**

## The state it was measured in

| | |
| --- | --- |
| mains | online, battery held at 80 % |
| Linux profile / EPP | `performance` / `performance` |
| RAPL package limits | PL1 **35 W**, PL2 **37 W** |
| CPU at idle | 3.86 GHz, against 1.2-1.4 on power-saver |
| GPU power profile | `base` |
| GPU hardware ceiling (`rp0`) | **1950 MHz** |

35 W is the high end for a 17 W Lunar Lake part and is presumably what the Windows-side
setting chose. That is an inference from the value: the limits before the change were never
recorded, so "raised" is not a measured delta.

## Measured

`src/bench/extent_curve.py` — the graph alone, best of five warm frames per extent — with
the GPU's `act_freq` sampled every 250 ms throughout:

| network extent | ms | fps | against 17 + 488 |
| --- | ---: | ---: | ---: |
| 448x256 | 64 | 15.7 | -12.3 % |
| 512x320 | 82 | 12.1 | -15.4 % |
| 640x384 | 126 | 7.9 | -8.0 % |
| 768x448 | 168 | 5.9 | -9.1 % |
| 1024x576 | 280 | 3.6 | -8.1 % |
| 1280x768 | 459 | 2.2 | -7.6 % |

Fitted: **11 ms + 457 ms per megapixel**, residual within 6 ms.

**The GPU spent 89 % of the run at 1950 MHz**, 9 % at 1900 and 2 % at 1450 — pinned at its
hardware ceiling. And the whole-frame stage profile of `phase57` came out at 215 and 245 ms
in two processes, against 214 on power-saver.

## Why that is "nothing" and not "9 %"

The curve is about 9 % under the old formula. It is not attributable to the power mode:

- **The graph runs on the GPU, and the GPU is frequency-capped, not power-capped.** It sat at
  `rp0` — the most the hardware will do — and earlier runs recorded the same 1950 MHz
  ceiling. A package power limit and a CPU energy preference cannot lift a frequency
  ceiling.
- **9 % is inside the band.** The same binary spreads about ten percent either side of its
  median *between processes*, from buffer placement (`HANDOFF`, "quote the band"). The two
  stage-profile processes today differed by 14 % from each other.
- **The system has been updated since `17 + 488` was measured**, and that formula's power
  profile was not recorded.

The host passes did not visibly move either. That fits too: after `phase57` they are small,
and the ones that remain move whole frames through memory, which is bounded by bandwidth —
70-91 GB/s (`phase20`) — not by the CPU's clock.

**A clean attribution would need a paired run in one session**, flipping only the profile.
The Linux half can be flipped; the firmware half cannot be reached from Linux at all, which
is the reason it was set in Windows. Not done, because the practical answer does not depend
on it: nothing about this workload is limited by the thing the mode changes.

## Idle, three processes

Re-run with no shader compilation, load 0.39, Steam's web UI the only thing awake. Three
separate processes of `extent_curve.py`, and the GPU at 1950 MHz for 92-94 % of each:

| network extent | run 1 | run 2 | run 3 | median | spread | with the background load |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 448x256 | 66 | 67 | 65 | 66 | 3.0 % | 64 |
| 640x384 | 129 | 136 | 127 | 129 | 7.0 % | 126 |
| 768x448 | 173 | 176 | 169 | 173 | 4.0 % | 168 |
| 1024x576 | 279 | 283 | 279 | 279 | 1.4 % | 280 |
| 1280x768 | 456 | 454 | 456 | 456 | 0.4 % | 459 |

Median fit **15 ms + 449 ms per megapixel**. The contaminated run sits within 1-5 ms of this
at every extent: **the graph did not notice two cores going missing**, because it runs on the
GPU. The spread between processes is mostly under 5 % here, tighter than the ten percent
recorded for whole frames — best-of-five removes most of what buffer placement adds.

The whole frame, the `phase57` stage profile, twice: **209 and 210 ms**, graph 180-182.
That is the comparison that isolates the mode, because it is the same code as `phase57`
measured on power-saver at **214 ms**: **2 %**, inside the noise. The earlier 245 ms was the
background load; 215 was not.

Against the old formula the slope is 8 % lower, and that is now outside today's band — but
the same-code comparison says the mode is worth 2 %, and the GPU sat at the same ceiling, so
the rest belongs to what changed between `phase45` and now: the Vulkan driver updated, which
is also what set Steam recompiling.

## What that means for anyone tuning this

The lever is the **extent**, as it has been since `phase25`. The formula to plan with is now
`15 ms + 449 ms per megapixel` on this machine, idle — call it the same curve as before within its
band — and the laptop's power mode is not a variable worth recording against it, except to
say the GPU was at its ceiling.
