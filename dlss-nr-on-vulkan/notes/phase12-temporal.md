# The temporal path works, and the model gates its own history
2026-09-09

`nr_frame` ran the first-frame branch — the right thing for a still, and only the
first frame of a sequence. `src/ref/nr_temporal.py` adds the rest of the recovered
contract: the previous output reprojected along motion vectors into feature channels
7-9, and the head's **fourth channel** used as a blend logit between the freshly
predicted frame and that history.

```
alpha  = clamp(sigmoid(half(head[..., 3])) * half(blend_scale), 0, 1)
output = predicted + alpha * (history - predicted)
```

`blend_scale = 0.73974609375` from the shipped package, so history can take at most
74 % of a pixel. The operators are MLX-DLSS's `temporal.py`, loaded by path the way
`features.py` and `composition.py` already were — at processing scale 1 with engine
motion it needs neither OpenCV nor Pillow. What is ours is the adapter onto the model,
the diagnostics, and a synthetic pan for testing without a game.

This also settles what that fourth head channel is. `notes/phase7-first-render.md`
recorded it sitting at -4.3 mean and noted only that RGB composition ignores it. It is
the history gate, and its resting value is "no history, do not blend".

## The motion convention, checked before anything else

`pan_sequence` walks a window across a larger image, which gives a genuine pan with new
content entering at the edge, and the exact motion for each step. Reprojecting frame
k-1 with frame k's motion must reconstruct frame k:

```
pan  (6, 0)   reprojection error 0.00000        control, zero motion   0.04780
pan  (0, 5)   0.00000
pan (-4, 3)   0.00000
```

Exact, because the shifts are whole pixels and the five-tap Catmull-Rom is then an
identity. A sign error here is silent — the network still runs and still makes a
picture — so this assertion comes first.

## The gate switches on, and refuses bad history

Same 384x384 crop, `alpha` measured over the frame:

| | alpha mean | alpha max |
|---|---|---|
| frame 0, no history | **0.0078** | 0.0659 |
| frames 1-3, history reprojected with **correct** motion | **0.705** | 0.7387 |
| frames 1-3, history reprojected with **zero** motion, scene panning | **0.032** | 0.60 |

Two things at once, and neither is something scaffolding can fake:

- With correct history the model asks for **0.705 against a ceiling of 0.7397** — it
  takes essentially all the history it is allowed, a **90x** switch-on from the
  no-history resting value.
- Give it a moving scene and tell it nothing moved, and alpha **collapses back to
  0.032** — a **22x** rejection. The network compares channels 7-9 against the colour
  in 4-6 and declines to blend history that does not match. That is the ghosting
  rejection, learned, and it works.

## What it buys: the flicker goes away

The three deterministic-noise channels are regenerated from the frame index, so a
*static* scene run as independent single frames still shimmers — the model synthesises
slightly different micro-detail every frame. That is the thing a temporal path exists
to fix.

Static scene, four frames, zero motion:

| step | independent per frame | temporal | |
|---|---|---|---|
| 0 -> 1 | 0.00396 | 0.00479 | history switching on |
| 1 -> 2 | 0.00327 | 0.00146 | **2.2x** |
| 2 -> 3 | 0.00341 | 0.00095 | **3.6x**, still falling |

Peak flicker over the frame drops from 0.0788 to 0.0126, **6.3x**. The decay from
0.00146 to 0.00095 is the exponential moving average a 0.7-weighted history gives; it
has not converged by frame 3.

**And it does not blur.** High-frequency energy against the independent per-frame run,
on the panning sequence: **-1.4 % to +0.2 %** with correct motion. Blending 70 % history
costs no detail, because the history is correctly reprojected first.

## Interface

```
python3 src/ref/nr_temporal.py IN.png OUT --gpu --pan 6,0 --frames 4     # synthetic
python3 src/ref/nr_temporal.py x --frames-from a.png,b.png,c.png OUT --gpu
python3 src/ref/nr_temporal.py IN.png OUT --gpu --pan 6,0 --zero-motion  # the control
```

In code, `session(model, motion="zero", **TemporalOptions fields)` and pass engine
motion per frame to `.process(frame, motion=...)`. Motion is a normalised history-UV
offset; `normalize_pixel_motion()` converts engine pixel motion and applies the
previous-minus-current jitter delta.

## Not done

- **Optical-flow motion** needs OpenCV, which this machine does not have. Engine motion
  and zero motion work; `motion="flow"` will raise.
- **`processing_scale != 1`** needs Pillow for the resample, same story.
- **History confidence** (`history_confidence`) is plumbed through and multiplies alpha,
  but nothing here produces one — it comes from the optical-flow path's
  forward/backward check.
- **Scene-cut detection** falls back to the luma-change threshold without OpenCV.
- **Depth guiding** (`depth_guide="closest"`) is implemented upstream but dormant: this
  DLL binds no depth guide.
