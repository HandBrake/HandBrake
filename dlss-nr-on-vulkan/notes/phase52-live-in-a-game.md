# Phase 52 — live neural rendering, running in a game

2026-09-11, 09:07. Dead or Alive 5 launched from Steam with `NR_LAYER_LIVE=1`, the daemon
holding the model, `nr-ctl` driving it. Not a photo mode and not a held frame: **every
present goes through the network, continuously.**

```
198 frames in 30 s — 6.60 fps
frame time 0.13-0.16 s, tight
change 0.02350 to 0.03144 across those frames — the scene is moving, every frame is its own
```

1280x720 output, render scale 0.35. This is the thing `phase47` built and neither tree had
ever run in a game.

## What it does to a frame, live

Frame 012 of the captured run, the character-select screen:

| region | luma | mean abs diff | relative fine texture |
| --- | --- | --- | --- |
| Kasumi's face | 98.5 -> 93.5 | 8.56 | **+6.3 %** |
| Mai's face | 98.2 -> 93.6 | 6.19 | +5.3 % |
| armour | 134.1 -> 123.4 | 12.74 | **+8.8 %** |
| background, logo | 70.2 -> 72.1 | 3.89 | -2.9 % |

Whole frame: `|d|` 7.23, brightness 93.0 -> 91.3, speckle x0.99. At 4x the skin gains pores
and the hair separates into finer strands; the background, which is flat art, is left
alone. The tone pull-back of `phase42` is there and smaller than at full scale.

## The finding: the render scale barely matters here

Swept live, without restarting the game or reloading the model — which is what `nr-ctl`
was built for:

| render scale | fps | frame |
| --- | --- | --- |
| 0.35 | 6.60 | 130 ms |
| 0.30 | 7.27 | 128 ms |
| 0.25 | 6.20 | 148 ms |
| 0.20 | 7.53 | 124 ms |

**The network shrinks threefold and the frame moves by a tenth.** That is `phase51`
confirmed from the other side: at a 1280x720 output the network is the minority of the
cost and the numpy either side of it — feature assembly, composition, the head upscale,
the codec — is measured at the *output* resolution and does not shrink with the scale.
The non-monotonic row at 0.25 is the network's alignment padding, familiar from the extent
curve.

**Corrected below (2026-09-11, later): that sweep covered 0.20 to 0.35 only, and the
conclusion does not survive outside it.** At scale 1.00 the same frame takes **1.01 s** —
1.10 fps against 6.2. Between 0.35 and 1.0 the network's cost grows sixfold and dominates
again; it is flat only in the range where the numpy at output resolution is the majority.
Generalising from four adjacent points was the error, and it is the same shape of error as
the profile recommendation in `phase44`.

**So the lever is the game's own resolution, not the render scale.** To go faster the game
must present a smaller frame. `phase47` measured 512x288 at 10.6 fps end to end; the route
to keeping that watchable on a 1080p panel is gamescope, which is now installed and which
the layer has been verified to load inside — it forces the swapchain to its nested size and
upscales with FSR 1.0. XeSS does not exist for Linux; checked twice.

## Two operational notes

`nr-photo` picks the last `Proton*` directory alphabetically, so when Steam downloaded
**Proton Hotfix** it silently displaced Experimental as the runtime. Worth pinning.

Launching the game from two places at once — `nr-photo --proton` and Steam — leaves both
fighting over one Wine prefix and neither starts; the second attempt then returns 53
because the first left `wineserver` alive. Check for a running game before launching one.

## Letterbox: a quarter of the frame was black

The owner's smallest window for this game is **1024x768**, and the game renders 16:9
inside it — 576 active rows and 192 black ones. Every stage below the network is measured
at the *output* resolution, so all of them were paying for the bars.

`active_region()` finds them and the frame path works on the interior alone:

| frame | round |
| --- | --- |
| 1024x768, letterboxed | **143 ms** |
| 1024x576, its active content | 130 ms |
| 1280x720 | 169 ms |

So the bars now cost almost nothing, and a 1024x768 window is *cheaper* than 720p while
showing more active pixels than 512x288 would.

**The bars are returned byte-identical**, which is exact rather than approximate:
`encode(decode(v)) == v` holds for all 256 values, so leaving them in the output array is
lossless. The test asserts that too, since the crop depends on it.

The detector refuses more than it accepts, deliberately:

- a bar counts only if **both** sides agree to within a row — a dark sky at the top with
  nothing matching at the bottom is not a letterbox;
- scanning stops at 45 % of the extent;
- if the surviving region is under half the frame the crop is abandoned entirely, so a
  fade to black or a loading screen does not get reduced to a sliver in the middle.

One self-inflicted bug found on the way: the result was being written into the same array
it was then differenced against, so the daemon reported `change 0.00000` for every
letterboxed frame. The measurement now happens before the write-back.


## And the render scale is a quality knob, not a speed knob

Measured on the live fight, same scene, the two settings:

| region | scale 0.35 | scale 1.00 |
| --- | --- | --- |
| whole active frame | +0.3 % | **+13.3 %** |
| kimono, fabric | +7.8 % | **+39.4 %** |
| skin | +5.5 % | +7.6 % |
| **distant crowd** | **-26.7 %** | **+26.9 %** |
| ring floor | -5.8 % | -4.9 % |

(relative fine texture, normalised for level as `phase42` requires)

At 0.35 the whole-frame figure is **+0.3 %** — which is not "a small effect", it is +10 %
on the characters cancelling -27 % on the crowd. The dark, distant crowd also lifts from
luma 30.5 to 42.6, a **40 % brightening**: at an internal 358x202 those spectators are a
few pixels each, the network reads them as noise and smooths them.

At 1.00 the same crowd *gains* 27 % and its level barely moves. At 4x the wing's feather
barbs, the fan's characters, the crowd's faces and the ring ropes all resolve.

**So the live mode's cheap end is not doing what the model is for.** 6.2 fps buys a pass
that helps the subject and damages the background; 1.1 fps buys the effect this project
exists to reproduce. That is the honest trade, and it is not a trade between speed and
*less* quality — below about half scale the sign of the effect flips on anything dark and
distant.

## The compromise, measured on one frame through seven scales

One captured 1024x768 fight frame, run through the whole daemon path at each scale, so
only the scale differs. Relative fine texture, normalised for level:

| scale | ms | fps | whole frame | kimono | skin | crowd | ring floor |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0.35 | 133 | 7.51 | **-1.6 %** | -3.5 % | -3.5 % | +0.7 % | +0.5 % |
| 0.45 | 170 | 5.87 | +0.6 % | +4.6 % | -1.5 % | +5.3 % | -1.4 % |
| **0.55** | **242** | **4.14** | **+2.4 %** | **+6.5 %** | **+5.6 %** | **+9.1 %** | **+6.4 %** |
| 0.65 | 271 | 3.69 | +3.2 % | +14.7 % | +6.5 % | +10.4 % | -2.4 % |
| 0.75 | 341 | 2.93 | +4.4 % | +16.3 % | +5.9 % | +12.3 % | -4.2 % |
| 0.85 | 437 | 2.29 | +6.4 % | +19.5 % | +1.2 % | +15.7 % | -4.3 % |
| 1.00 | 621 | 1.61 | +13.6 % | +41.7 % | +3.8 % | +29.1 % | -10.7 % |

**0.55 is the answer for a moving picture**: the only row where no region is negative.
242 ms, 4.1 fps.

**0.35, which this session had been running all day, is negative on the whole frame.**
Fabric -3.5 %, skin -3.5 %. At that internal size the pass is not adding detail, it is
smoothing. Every live measurement taken at 0.35 was measuring a pass doing the opposite
of its job.

### There is no efficiency optimum in the middle

Return per millisecond, on the most sensitive indicator:

```
scale 0.35   -0.026 % per ms      the money is wasted
scale 0.55   +0.027
scale 0.65   +0.054
scale 1.00   +0.067 % per ms      the best buy
```

**The effect is superlinear in scale.** Detail per millisecond is *highest* at full scale,
so there is no bargain in the middle to find — the trade is only between a moving picture
and a good one. That also explains the shape of the whole table: the network seems to need
a minimum size below which it cannot see structure, and above which it starts
reconstructing it, rather than degrading smoothly.

The floor going negative from 0.65 upward is the one exception and probably the same tone
pull-back `phase42` measured: the ring floor is a bright near-white surface and the pass
pulls its level down.


## The compromise does not exist, and here is the shape of what does

The 0.55 row above had no negative region — on **that** frame. On a second live frame from
the same fight, whose crowd sits at luma 28.7 instead of 62.9, the same setting gives
**-21.4 %** there. Sweeping the scale on that frame:

| scale | ms | fps | whole | **dark crowd** | wing | kimono | floor |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0.45 | 172 | 5.83 | +1.5 % | **-19.1 %** | +5.6 % | +7.4 % | +6.1 % |
| 0.55 | 206 | 4.86 | +5.3 % | **-21.4 %** | +14.4 % | +15.0 % | +9.9 % |
| 0.65 | 288 | 3.47 | +7.6 % | **-13.4 %** | +19.3 % | +16.2 % | +7.0 % |
| 0.80 | 424 | 2.36 | +8.8 % | **-7.7 %** | +17.2 % | +16.2 % | +1.0 % |
| 1.00 | 551 | 1.82 | +17.3 % | **+9.8 %** | +42.5 % | +45.5 % | -4.1 % |

Monotone and it only crosses zero at the top. **The scale that a region needs depends on
how dark and how fine it is**, not on the scale alone:

- bright, large features — fabric, wings, a ring floor — are already positive at 0.55;
- a distant crowd at luma 29 is negative everywhere below 1.00.

So "0.55 harms nothing" was a one-frame conclusion, and this is the third time this
session that a recommendation has been generalised from a single sample — after the
`cinematic` profile (`phase44`) and "the render scale barely matters" above. The pattern
is worth more than the number: **on this model, one frame never establishes a setting,
because the effect's sign depends on local content, not on global configuration.**

### What to actually run

| | rate | what it costs |
| --- | --- | --- |
| **scale 0.55** | **4.9 fps** | +15 % on fabric and characters, -21 % on a dark crowd |
| **scale 1.00** | **1.8 fps** | everything positive but a bright floor, +45 % on fabric |

For a fighting game, where the eye is on the characters, 0.55 is the defensible live
setting and the dark background is the price. For looking at a frame, 1.00.

### An instrumentation note

The live rate at 0.55 is **5.00 fps, 200 ms a frame**. The same run with `--dump` reads
1.28 fps: two 1024x768 PNGs a frame cost five times the network. **Never measure a rate
with the dump on** — a mistake this session made once already at 854x480.
