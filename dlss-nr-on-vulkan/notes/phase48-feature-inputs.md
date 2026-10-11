# Phase 48 — what the feature tensor actually carries, and two costs inside it

2026-09-10. Prompted by a RenoDX contributor's note that the model gets by with the same
inputs already available to ordinary DLSS upscaling, and that anything a feature assembly
does beyond that is a chance to simplify.

## The inputs are fewer than DLSS's, not more

All sixteen channels, from `work/mlx-dlss/python/mlxdlss/features.py`:

| channels | what |
| --- | --- |
| 0:3 | deterministic Gaussian noise — the diffusion sample |
| 3 | the constant 1 |
| 4:7 | `half((half(colour) - 0.5) * 0.125)` |
| 7:10 | **the same scaled colour again** |
| 10 | style index / 128 |
| 11, 12 | local tone, local structure |
| 13, 14 | skin structure, automatic-mask structure |
| 15 | never written; always zero |

**No motion vectors, no depth, no exposure, no jitter.** The observation holds and then
some: this needs strictly less than an upscaler does. There is nothing to strip — every
channel is either the colour, the diffusion noise, or one of five artistic scalars.

Two things worth recording rather than acting on:

- **7:10 is the temporal slot.** `nr_temporal.py` puts the previous output, reprojected
  along motion vectors, there; the still path fills it with the current colour, which is
  what a first frame should carry. In a live mode we do have a previous output but no
  motion vectors, and `phase12` measured the gate: 0.705 with correct history, **0.032
  with wrong motion**. Feeding un-reprojected history would mostly be rejected — harmless
  and useless. It needs real motion vectors, which a layer at `vkQueuePresentKHR` does
  not have.
- **Channel 15 is always zero.** Whether the vendor puts something there is unknown. Ours
  has never carried anything and the model has never been given a reason to expect it.

## The noise was rebuilt every frame

`deterministic_noise` depends on the extent and the frame index and on nothing else, and
both of its callers copy the result into a slice rather than writing through it. The
daemon never sets a frame index, so in a live mode the same array was being rebuilt from
four transcendentals a pixel — `sqrt`, `log`, `cos`, `sin` — on every frame:

| extent | share of feature assembly |
| --- | --- |
| 854x480 | 29 % |
| 512x288 | 31 % |
| 1920x1080 | 42 % |

Memoised in `src/ref/nr_frame.py` rather than in the vendored copy, with the cached array
marked read-only so a future in-place user fails loudly. Feature assembly halves —
**119 -> 56 ms** at 854x480, **465 -> 248 ms** at 1080p — and the noise channels are
bit-identical to the original function.

The end-to-end gain in live mode is smaller than that, because there the features are
built at the *inner* extent: 512x288 at scale 0.35 went **98.6 -> 94.0 ms, 10.15 -> 10.64
fps**. It is the photo mode at full resolution that gains the ~200 ms.

## The two strength knobs cost a full-frame blur

Not from the news item, found while measuring beside it. `compose_detail` returns early
when `detail_strength` and `colour_strength` are both 1, and otherwise splits the change
into frequency bands with a radius-4 Gaussian:

| extent | strengths at 1 | strengths moved |
| --- | --- | --- |
| 854x480 | 15.2 ms | **110.0 ms** |
| 1920x1080 | 86.7 ms | **552.9 ms** |

**Seven times.** The blur is already separable, and the vendored code has a
`cv2.sepFilter2D` path it takes when OpenCV is present; without it the numpy fallback
runs 18 full-frame accumulations. OpenCV is not installed here, so that path never fires.

`nr-ctl` now says so when either strength is set away from 1 — a knob that quietly costs
seven times the composition is exactly what a control tool should surface.

## On the second prediction

The same modder expects upscaling, frame generation and neural rendering to merge into
one pipeline, to cut the number of full-frame passes. Everything measured today points
the same way: once the network is given a small enough extent it stops being the frame,
and what is left is a stack of independent full-frame passes over the *output* — feature
assembly, composition, the head upscale — each of which reads and writes the whole
picture. `phase47` found one of those costing 55 % of a frame. Merging them is the same
observation from the other side.

## OpenCV installed, measured

The owner installed `python-opencv` (cv2 5.0.0) after reading the above, so the vendored
blur now takes its `cv2.sepFilter2D` path:

| extent, strengths moved | numpy fallback | OpenCV |
| --- | --- | --- |
| 854x480 | 110.0 ms | **31.8 ms** |
| 1920x1080 | 552.9 ms | **187.9 ms** |

The penalty for touching either strength drops from about **7x the composition to about
2x**. Agreement with the numpy path is **1.19e-07** maximum over a frame — float32
rounding, not a different filter, which is expected: the numpy fallback is already the
same separable Gaussian, just run as 18 array accumulations.

`nr-ctl` detects OpenCV and reports whichever number is true, and `nr-ctl status` says
which path the blur will take.
