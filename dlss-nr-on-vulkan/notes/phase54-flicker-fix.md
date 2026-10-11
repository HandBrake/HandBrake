# Phase 54 — the flicker fixed, and why the vendor's own gate was not enough

2026-09-11. Follows `phase53-why-it-flickers.md`, which measured the problem and proposed
the fix. The fix works — **3.7x less invention over pixels the game did not move**, for
**3.7 % of the frame time** — but not for the reason phase53 predicted, and the part that
does the work is not in MLX-DLSS.

Everything below is measured on `work/good`, 22 consecutive presents of Dead or Alive 5 at
1024x768 (1024x576 inside the letterbox), render scale 0.55, profile `standard`, replayed
through the daemon so that every setting sees exactly the same frames
(`src/bench/flicker.py`).

## What was measured

Every pixel of every consecutive pair, sorted by what the *input* did between the two
presents. `x` is output change over input change — how much the network amplifies. `lag`
is how far our answer sits from the game's own frame, which is where a trail would show.

| setting | ms | byte-identical (2.3 %) | \|d\|<2 (7.7 %) | moving (35.1 %) | lag, moving |
|---|---|---|---|---|---|
| effect on, no history | 215 | **3.25** | 6.57x | 1.01x | 9.52 |
| + temporal path | 223 | 2.73 | 4.97x | 0.98x | 9.96 |
| + hold 0.5 | 222 | 1.88 | 3.79x | 0.98x | 9.96 |
| + hold 1.0 | **224** | **0.87** | **2.19x** | **0.98x** | **9.96** |

A pixel whose bytes did not change at all came back **3.25 levels of 255 different**. That
is invention, and it is what the eye reads as shimmer. It is now 0.87 — below where a flat
area reads as moving. Pixels that actually moved are untouched (1.01x -> 0.98x), and the
whole temporal path costs **+9 ms of 215**.

`work/flicker/ab-invented.png` is the same thing to look at: the frame-to-frame change,
x16, masked to the pixels the game left alone. Left is off, right is on. The crowd
speckle, the health bars and the sponsor boards stop shimmering; the fighters are not in
the picture at all, because they moved.

## The part that worked as predicted

Feeding the previous output into feature channels 7-9 with **identity reprojection**. A
layer at `vkQueuePresentKHR` has no motion vectors, and phase53's argument was that zero
motion is the *correct* motion for a static region, which is where the flicker lives.

Identity reprojection is free and exact: `sample_history` at pixel centres is
**bit-identical** to the history — the five-tap Catmull-Rom collapses to its middle tap, so
there is no gather to run and nothing to blur. `nr_frame.apply_history` therefore produces
a feature tensor bit-identical to MLX-DLSS's `make_temporal_features` with zero motion,
checked in `src/layer/test_temporal.py`.

## The part that did not: the gate is not local

Phase12 measured the learned gate at **0.705** with correct history. Here it reads **0.12**
over pixels that did not move, and 0.014 over pixels that did:

| pixels | gate, identity reprojection | gate, with optical flow |
|---|---|---|
| byte-identical | 0.116 - 0.141 | 0.145 - 0.171 |
| \|d\| < 2 | 0.154 - 0.191 | 0.186 - 0.205 |
| moving | 0.013 - 0.017 | 0.056 - 0.073 |
| whole frame | 0.071 - 0.079 | 0.102 - 0.121 |

It *discriminates* — 10x between still and moving, against phase12's 22x — but the whole
scale is down by a factor of five. The reason is the same one that causes the flicker: the
network is global. Phase12's 0.705 came from a pan where the history was correct
**everywhere**; in a fight most of the frame moved, the history disagrees with the current
frame over most of it, and the model distrusts it everywhere — including the 2.3 % where
it happens to be exactly right.

**Optical flow does not fix this, and was measured rather than assumed.** OpenCV DIS at the
network extent costs 7 ms and raises the whole-frame gate 0.076 -> 0.111, but almost all of
that gain lands on the *moving* pixels (0.014 -> 0.066), which is the ghosting case, not
the flicker one. On the still pixels — the ones that shimmer — it is worth 0.12 -> 0.16.
The reprojection it would then need is 61 ms on the CPU at this extent (2.8 ms on the
device via `GpuHistory`, `phase12`). Not worth building. The flow measurement is in the
transcript; `nr_temporal.FlowMotionEstimator` and `install_gpu_history` are still there for
a caller that has a reason.

## The part that is ours: a floor under the gate

The vendor's `history_confidence` can only scale the gate **down**, per pixel. What a
present-time layer needs is the opposite, and it has exactly the right signal to justify
it: **the game's own frame**. Where the game handed back the same pixel, the previous
output is the right answer for that pixel by construction, no motion vector required.

`nr_daemon.hold_floor` turns that into a per-pixel lower bound on the blend:

```
floor = clip(1 - |current - previous| * 255 / 4, 0, 1) * hold
alpha = max(gate * confidence, floor * blend_scale)
```

Full hold at zero change, gone by four levels of 255, and never above `blend_scale`
(0.73974609375) — so no pixel is ever held harder than the model itself holds one. It
cannot ghost: the frame the game draws is the thing that releases it, and it releases the
same frame it changes on. `decode` is a bijection, so comparing decoded frames is the same
test as comparing wire bytes, and it stays exact on the packed 10-bit format.

Cost **2.2 ms** at 1024x576, channel by channel and in place. The obvious
`max(abs(a - b), axis=2)` builds two full-frame temporaries and is three times that; this
is the same class of finding as `phase47`.

## Knobs

`temporal` (0-1, default 1) is the vendor's history confidence — 0 turns the path off
entirely and is bit-identical to the still path, including clearing the stored frame so
switching back on cannot resurrect a stale one. `hold` (0-1, default 1) is the floor
strength. `cut_limit` (default 0.15) is the mean frame-to-frame change above which the
shot is taken to have cut and the history is dropped outright. All three move between
frames through `nr-ctl` with no reload.

## In the running game, which is better than the replay says

`--meter` reports the same statistic from inside a live game, because the replay cannot
reproduce the one thing that matters: `--dump` drops the daemon to ~1.28 fps, so
consecutive frames in a capture are four times further apart in game time than in play.
Dead or Alive 5, 1024x768, scale 0.55, ~95 frames each:

| | frame held still | invented there | where it moved | ms |
|---|---|---|---|---|
| temporal 0 | 17 % | **1.58** | 7.35 | 203 |
| temporal 1, hold 1 | 15 % | **0.64** | 6.57 | 227 |

**The gate reads 0.38-0.54 in live play, not the 0.12 of the replay**, and 65-70 % of the
frame gets a floor. Four times less time between presents means four times less of the
frame has moved, so the history is much more nearly correct and the model knows it. The
replay's 0.12 is the pessimistic end of the range, not the typical one.

## What it costs, pass by pass

The controlled number is the replay, because both settings see the same frames: **215 ->
224 ms, +9 ms (4 %)**, repeatable across four runs. Live readings sit between 190 and 230
ms in *both* settings — scene content moves the frame time more than this path does — so
the live pair above is not a timing measurement.

Each added pass at 1024x576 output / 563x317 network, measured on synthetic arrays with
the game running, which makes these an upper bound:

| pass | ms |
|---|---|
| resample the previous output to the network extent | 6.7 |
| `apply_history` including that resample | 14.2 |
| `hold_floor` | 2.3 |
| `History.keep`, three copies | 2.8 |
| `compose` with history and floor, over `compose` without | 7.2 |

Widening the head upscale from three channels to four is **free** — it is the gate channel,
and the separable resample is not what costs. If this path is ever worth trimming, the two
places with something in them are `apply_history`'s gather over the *network* extent and
the sigmoid in `history_weight`, which runs at the output extent over a logit that was
bilinearly upscaled from the network's — computing it before the upscale would be a third
of the work and no less faithful.

## Traps found on the way

- **The daemon is now stateful across frames.** `test_ui_mask.py` sends a masked and an
  unmasked request and requires them to be two views of one frame; that premise is gone
  unless the daemon is started with `--temporal 0`, which it now is. Any future test that
  sends a sequence has to decide which it wants.
- **The interface mask still holds**, and it is not obvious that it should: the history is
  stored *after* the wire-byte restore, so a leak would compound frame over frame instead
  of staying put. Checked over three frames in `test_temporal.py`.
- **The capture this is measured on is slower than live play.** `--dump` drops the daemon
  to about 1.28 fps, so consecutive frames in `work/good` are ~0.8 s apart rather than
  ~0.2 s. That makes this measurement pessimistic: in live play less of the frame has
  moved between presents, so the hold covers more of it.

## And the trail it left, once the frame rate could show one (2026-09-25)

At 25-32 fps in Tekken 7 the owner saw what 10 fps had hidden: **ghosting — a trail behind
everything that moved**. The hold floor was not the cause; it cannot be, since the frame
that changes a pixel releases it. The model's own gate was. Without motion vectors the
history at a pixel something moved across is whatever used to be there, and the gate is
not local enough to refuse it: over the Tekken session it read **0.63** as a whole-frame
median (p10 0.57, p90 0.68) — so a moving pixel kept most of the frame before it, and that
frame most of the one before.

The fix uses the same exact signal as the floor, from the other side. Where the game's own
pixel changed by `release` levels of 255 or more, none of the gate survives; by less, a
share that falls linearly with the change: `alpha = max(gate * confidence * clip(1 - moved
* 255 / release, 0, 1), floor)`. It lowers only the gate, never the floor, which is zero
wherever the game changed a pixel anyway. Native and NumPy byte-identical
(`test_native_image.py`, twelve combinations, and a check that a released pixel is the
still frame exactly).

Tuned by eye in the game, the owner at the controls: at 4 the shimmer over moving things
came back; 8 still showed some trail; **16 and 24 both looked right, 24 preferred** — the
default. `nr-ctl set release 0` is the old behaviour. The daemon's line reports
`released N%`, the share of pixels the release dropped entirely.
