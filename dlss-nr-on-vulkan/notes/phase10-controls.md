# The intensity and style controls, measured
2026-09-09

The feature has live controls — OptiScaler exposes some of them as sliders — and all of
them are reachable from our port. They fall into two classes, and the difference matters
for anything interactive.

## Free: post-network, one forward pass covers the whole range

`intensity`, `detail_strength`, `colour_strength` act on the composed image. Render the
head once with `run_head()` and `compose()` it at any setting; `--intensity-ladder`
does exactly that.

**`intensity` is exactly linear and 0 is an exact no-op:**

```
0.00  change mean|d| 0.00000      0.75  0.01960
0.25  0.00653                     1.00  0.02613
0.50  0.01306
```

It is clamped to [0, 1] in `compose_head`, so it cannot over-drive — for "more than
default" use `detail_strength` or the conditioning scalars below.

**`detail_strength` / `colour_strength` split the change by frequency.** The model's
change is mostly *low* frequency — of the 0.02613 default, **0.00490 is high frequency
and 0.02493 is low**. So the visible micro-detail is the small part and most of the
effect is tonal.

| | change | HF part | LF part |
|---|---|---|---|
| default (1, 1) | 0.02613 | 0.00490 | 0.02493 |
| detail 2, colour 1 | 0.02847 | 0.00908 | 0.02543 |
| detail 3, colour 1 | 0.03152 | 0.01333 | 0.02602 |
| detail 1, **colour 0** | 0.00490 | 0.00429 | 0.00134 |
| **detail 0**, colour 1 | 0.02489 | 0.00138 | 0.02446 |

`--colour-strength 0` therefore gives the detail with none of the tonal shift, and
`--detail-strength 0` the reverse. **Beyond `detail_strength` 2 it visibly over-sharpens**
— at 3 the synthesised grain becomes harsh.

## Not free: conditioning, one forward pass per setting

`style_index`, `local_tone`, `local_structure`, `skin_structure`, the automatic mask and
a ControlMask's green/blue channels land in feature channels 10-14, so each value costs
a whole pass. (A ControlMask's *red* channel is per-pixel intensity, and that is free.)

**Tone and structure are smooth, monotone gains, and safe to over-drive.** Measured as
mean |head| against the standard setting, and as correlation with the standard change:

| tone (structure 1) | x standard | corr | | structure (tone 1) | x standard | corr |
|---|---|---|---|---|---|---|
| 0.25 | 0.745 | 0.971 | | 0.25 | 0.582 | 0.939 |
| 0.50 | 0.824 | 0.981 | | 0.50 | 0.734 | 0.964 |
| 0.75 | 0.913 | 0.988 | | 0.75 | 0.835 | 0.989 |
| 1.00 | 1.000 | 1.000 | | 1.00 | 1.000 | 1.000 |
| 1.50 | 1.117 | 0.976 | | 1.50 | 1.137 | 0.978 |
| 2.00 | **1.239** | 0.964 | | 2.00 | 1.108 | 0.968 |

Correlations of 0.94-0.99 say both are largely the *same* operation scaled, structure
slightly less so than tone. Tone keeps growing to 2.0; **structure peaks around 1.5 and
turns back**. Both stay colour-neutral across the whole range (channel imbalance
<= 0.0036), so over-driving them is safe.

Both at 0 is the `neutral` profile and switches the network off by **37x**
(0.00071 against 0.02610) — the control from `notes/phase7-first-render.md`.

**Style is a different character, not a gain.** Correlation with style 0 drops to 0.50
at style 1 and 0.74 at style 2 while the magnitude moves only 1.12-1.25x. These are
genuinely different pictures.

**But the index has a small valid range.** Channel imbalance in the composed change:

| style | 0 | 1 | 2 | 3 | 4 | 8 | **64** |
|---|---|---|---|---|---|---|---|
| imbalance | 0.0006 | 0.0074 | 0.0043 | 0.0039 | 0.0034 | 0.0026 | **0.0286** |

Style 64 produces a magenta cast and a washed-out picture — 0.0286 against the
shuffled-weight control's 0.040, which is what "broken" measures. 0-8 are all sane; the
vendor documents 0, 1 and 2 (`standard`, `natural`, `cinematic`). The index is
normalised by 1/128, so 64 is 0.5 — far outside anything trained.

## What we cannot reproduce

**Model A / B / C.** MLX-DLSS's own notes say the shipped safetensors proves only
slot 0, and that supporting the other slots needs captures of them: they may swap full
weights, small adapters, or create-time graph conditioning. Nothing in our container
distinguishes them, so a "model" selector is not something this port can offer honestly.

## Commands

```
# the live slider: one pass, five images
nr_frame.py IN.png OUT.png --gpu --intensity-ladder 0,0.25,0.5,0.75,1

# detail without the tonal shift / a stronger effect
nr_frame.py IN.png OUT.png --gpu --colour-strength 0
nr_frame.py IN.png OUT.png --gpu --detail-strength 2

# conditioning (a pass each)
nr_frame.py IN.png OUT.png --gpu --local-tone 2 --local-structure 1.5
nr_frame.py IN.png OUT.png --gpu --style-index 2
nr_frame.py IN.png OUT.png --gpu --skin-structure 0.5 --auto-mask -1
nr_frame.py IN.png OUT.png --gpu --control-mask mask.png --intensity 0.8
```
