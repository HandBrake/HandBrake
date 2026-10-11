# Phase 30 — what each slider actually does, and one that did nothing

2026-09-10. The owner sent a screenshot of RenoDX's DLSS5 panel running the real
feature — `DLSSNR v310.8.0: ACTIVE — NR INJECTED`, 2142 successful NR frames at
2560x1440 — and asked what responds to what. Two things came out of it: the shipped
control surface confirms our recovered one almost exactly, and comparing them found a
control of ours that was silently inert.

## The shipped panel against ours

| panel | ours | vendor range / default | notes |
|---|---|---|---|
| NR Intensity | `--intensity` | 0..2, default 1.0 | screenshot sits at **1.66** |
| Local Tone Strength | `--local-tone` | 0..2, default 1.0 | |
| Local Structure Strength | `--local-structure` | 0..2, **default 1.5** | ours defaulted to 1.0 |
| Skin Structure Strength | `--skin-structure` | −1..2, **default 2.0** | screenshot at −1.00; only with the mask on |
| Automatic Mask | `--auto-mask` | **on** by default | ours defaults off |
| NR Style | `--style-index` | Default / Natural / Cinematic | our style 0 / 1 / 2 |
| Color Strength | `--colour-strength` | | |
| NR Preset | — | Default / #1 / #2 / #3 | model-level; we have one model |
| Scene Paper-White Scale, HDR Transfer Strength | `nr_display.py` | | our HDR codec covers the same ground |
| Depth Convention, Motion Scale X/Y | — | | guides, for the temporal path |
| NR UI Correction | — | | the panel keeps UI downstream of the pass |

The −1.00 in the screenshot is a **confirmation**, not an oddity: our recovered feature
construction reads a negative skin-structure strength as "follow local structure", and
that is exactly the value a panel would show for a control left alone. Channels 13 and
14 carry those two scalars; the panel's semantics land on them without adjustment.

`vendor` is now a profile — structure 1.5, tone 1.0, style 0 — and pairs with
`AutomaticMask(2.0, -1.0)`. On a 720p face it moves the frame **0.02617** against our
`standard` profile's 0.02540, and the two differ by 0.00378, about 15 % of the effect.
Ours was not wrong, just a milder starting point than the shipped one.

## The bug the comparison found

**`--intensity` above 1 did nothing.** MLX-DLSS's `compose_head` clamps the blend to
[0, 1], so every value over 1 collapsed to 1 — while the vendor's slider goes to 2 and
the screenshot is sitting at 1.66. `nr_frame.compose` now extrapolates past 1 itself and
delegates to the clamped path at or below 1, so the regression is untouched and the
slider behaves like the panel's. Symmetry confirms it: 1.5 now moves the frame as far as
0.5 does (0.01547 against 0.01550), and 2.0 twice that.

## What each control moves, by frequency band

`src/bench/control_atlas.py` sweeps each control on a real frame and splits the
difference into low and high spatial frequency. Every value costs a forward pass —
the conditioning scalars are feature channels — except intensity, which is composition.

| control | change | low / high | concentration |
|---|---|---|---|
| local_tone 0.0 | 0.01360 | 83 / 17 | 4.4x |
| local_tone 2.0 | 0.00876 | 77 / 23 | 4.9x |
| **local_structure 0.0** | **0.02118** | 77 / 23 | 6.2x |
| **local_structure 1.5** | 0.00462 | **66 / 34** | 4.9x |
| skin_structure 0.0 (mask on) | 0.00493 | 67 / 33 | 6.9x |
| style_index 1 | **0.03851** | **84 / 16** | 6.1x |
| intensity 2.0 | 0.03084 | 83 / 17 | 5.9x |

Read from it:

- **Structure is the only control whose high-frequency share rises with its value** —
  23 % at 0 to 34 % at 1.5. The name is earned: it is the one that moves micro-detail.
- **Tone and style stay 77-85 % low frequency at every setting.** Style is also by far
  the largest single lever — style 1 moves the frame three times as much as turning
  structure down to 0.5 — so it is a tonal decision, not a detail one.
- **Turning structure off costs more than turning tone off** (0.0212 against 0.0136).
  Most of what this network does is structure.

## The instrument was wrong before the control was

The first version of this sweep judged skin selectivity with a colour-based skin prior,
and reported that `skin_structure` acted uniformly — 0.00479 on "skin" against 0.00454
elsewhere. That was the prior failing, not the control. Rendering `skin_structure` 0
against 2 and mapping the difference shows a **face**: forehead, cheek, nose and the lit
side of the head, with the background untouched, and the top decile of the change
**6.2x** the rest. The network does the masking itself; the scalar only says how hard.

The atlas now reports that concentration ratio instead, which needs no prior about where
a control ought to act.

## Sources

The panel's ranges and defaults were cross-checked against community reimplementations
of the same feature, which agree with each other and with what our features.py already
did: NGX **feature 18**, `nr_intensity` 0..2 default 1.0, `local_tone_strength` 0..2
default 1.0, `local_structure_strength` 0..2 default 1.5, `skin_structure_strength`
−1..2 default 2.0 and live only with the automatic mask, styles Default / Natural /
Cinematic.

- [Blueforcer/ComfyUI-DLSS5-Enhancer](https://github.com/Blueforcer/ComfyUI-DLSS5-Enhancer) — the same feature headless, with the parameter table
- [jpneagle/dlss5-webcam-demo](https://github.com/jpneagle/dlss5-webcam-demo) — records the NGX key strings read out of `renodx-dlss5.addon64`
- [NIGos/dlss5-bridge](https://github.com/NIGos/dlss5-bridge), [jlrouzies-fr/DLSS5-Feeder](https://github.com/jlrouzies-fr/DLSS5-Feeder) — the injection side, for the guides the panel exposes
