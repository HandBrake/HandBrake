# Phase 70 — what the vendor does after the network

2026-09-27. Checking the frame around the network against OpenDLSS-NR turned up a post-network
colour grade for the `natural` and `cinematic` styles that this tree did not have, and a history
that carried something else than ours. Both are now read out of our own DLL rather than taken on
trust: the kernel from its PTX, the preset values and how they are scaled from its x86-64 code.

## The kernel: `cg2r_post_process_kernel`

Module 13 of the fifteen PTX containers (`work/modules/module_13_0112FD90.ptx`, 72 KB, the only
entry in it). **Verified**, instruction by instruction. One pixel of the output, in order:

1. **The neural picture at the output's extent.** Either a texture read (parameter +32), or —
   with a flag at +300 and two textures at +224 and +256 — a *transfer* from a smaller network
   picture: both are sampled at the low grid's texel centres, taken to Oklab, and the change
   stored per low texel as `clamp(ln max(L_b, 0.1) - ln max(L_a, 0.1), -2, 2) * sat(L_a / 0.1)`
   for lightness and plain differences for a and b; the output pixel's own `A` in Oklab then gets
   the bilinear of those, lightness as `sat(max(L, 0.1) * exp(dL))`.
2. **Levels**: `sat((c - black) / (white - black + 1e-10))` (+316, +320).
3. **Temperature and tint** (+344, +348): a lerp towards the pixel's own HSL lightness at full
   saturation and a fixed hue — 22° warm or 202° cool for temperature, 112° green or 292° magenta
   for tint, by sign — skipped below 1e-6.
4. **Exposure**: `c * 2^EV` (+324), saturated.
5. **Contrast**: `c + k * (c²(3 - 2c) - c)`, a smoothstep blend (+332), saturated.
6. **A five-zone tone curve**: `v^(2^-g)` with `g` the sum of five smoothstep bumps centred on 0,
   0.25, 0.5, 0.75, 1 times five parameters (+352 to +368).
7. **A gamma**, `max(0, v)^(2^-p)` (+328).
8. **Saturation** in HSL, `sat(S * (1 + s))` (+336), back to RGB.
9. **A saturation power** in HSL again, `sat(S^(2^-v))` (+340), back to RGB.
10. **The composition**: `sat(base + w * (graded - ref))` into the output surface, where `base` is
    a texture (+64), `ref` another (+128) or `base` when absent, and
    `w = intensity (+288) * mask(+96).r * (1 - sat(ui))`, `ui` the UI texture's alpha (+160) or a
    UI-alpha texture's red (+192). With no `base` the graded picture is written as it is.

## The host side: which values, and when

From the x86-64 code of `ref/nvngx_dlssnr.dll` (310.8.0.0), `objdump -d`. **Verified.**

- **The style table.** Each model descriptor (648 bytes; the one this DLL carries starts at
  `0x1800b0d80` with its name, `CC_Control_History_Blend_Quantize_With_Teacher_honest_tench_
  2026_07_04_22_30_weights`) holds entries of `{valid, style, mask, 14 floats}` from `+0x64` in
  steps of `0x44`, the floats in the kernel's own order — black, white, exposure, gamma,
  contrast, saturation, saturation power, temperature, tint, five zones:

  | style | mask | non-neutral values |
  | --- | --- | --- |
  | 1 (`0x1800b0de4`) | `0x34` | exposure **-0.1**, contrast **-0.25**, saturation **-0.1** |
  | 2 (`0x1800b0e28`) | `0x20` | saturation **-0.15** |

  The mask is which fields apply, bit for field: `0x34` is fields 2, 4 and 5. Style 0 has no
  entry, so no grade.
- **The scaling** (`0x18001d5f0`): for each masked field,
  `neutral + (preset - neutral) * clamp(LocalToneStrength, 0, 1)` in float32 — neutral is 1 for
  white and 0 for everything else. Applied on every evaluate to a fresh neutral state
  (`0x18001d7c0`, called from `0x180018e64`), after the style is clamped to the table's range.
- **When the pass runs** (`0x18001c920`, which fills the 376-byte block): only when the intensity
  is below 1, a control mask is given, the UI correction is on, or any grade value is not neutral
  (`0x1800176e0`). `base` is the snapshot `dlssnr_original_color` (an RGBA16F copy of the colour
  input), `ref` is the colour input only under UI correction, +96 is `DLSSNR.ControlMask`.
- **The transfer path is unused in this build.** Its two textures are passed as null, and
  `DLSSNR.ScalingRatio` is read and then overwritten with 1.0 (`0x18001a96a`, again at
  `0x180018e69`): the network always runs at the output's extent here.

The names are not in the DLL. The hosts that name the styles (ComfyUI-DLSS5-NR, the Zonnery
player) call 1 `natural` and 2 `cinematic`, OptiScaler calls 1 gentler and 2 film-oriented, and
OpenDLSS-NR's native demo agrees; a comment in its WebGPU port has them the other way round.

## The history

Block 70's fused kernel (`cc_tinlayout_fused_post_block_swin_1h_32*`, module 0) composes
`clamp((proxy * 0.125 - 0.0625 + head * 0.03125) * 8 + 0.5)`, blends it with the history through
`sigmoid(logit) * blend_scale`, and stores it with one formatted surface store. The post-process
reads that picture back (+32); it never writes the history. The DLL creates
`dlssnr_network_output_scratch` and `dlssnr_prev_output`, both RGBA16F. So **the history is the
prediction after the history's own blend — before the grade, the intensity, the masks — held in
half** (verified as far as the structure goes). That the store truncates toward zero rather than
rounds is **OpenDLSS-NR's**, from captures; the conversion mode of a formatted store is not in
the PTX.

This tree carried the picture the game received: after the intensity, the detail and colour
strengths and the interface restore. MLX-DLSS's session carried it after the intensity.

## What changed here

- `natural` and `cinematic` get the grade, as `nr_frame.grade_for` / `style_grade` and the same in
  `nr_image.c`, byte-identical to each other (`test_native_image.py`, greys, primaries and hues on
  the segment boundaries included). The kernel's other stages are identities at the values no
  style sets and are left out; the vendor evaluates two of them as `ex2(lg2(x))`, which its
  approximations make a 1e-7 wobble rather than an identity.
- The grade reads the half the history keeps, as the vendor's pass reads its RGBA16F scratch.
- The history is that half, `truncate_half` of the prediction — in the daemon, in the native and
  NumPy compositions (`neural=`), and in `nr_temporal`'s session.

On two DoA5 frames at 1920x1080 the grade moves `natural` by **6.2 levels of 255** — the pass
itself moves the frame 9 — luma -3.3 levels and chroma -5.6, and `cinematic` by 1.65 (chroma
-3.9; HSL lightness unchanged by construction, luma +0.5). The network's `natural` adds chroma (+1.6 against
`standard`'s -4.2) and the grade takes it back out: with it all three styles end within a level of
each other in chroma.

The history change moves every frame after the first, a little: the daemon's answers over 48
frames of a panning Cyberpunk shot are new (standard, three live sizes: `f29ffcfaee77c496`,
`23f59257381d0b3b`, `cba8fc4f60fd907d`), with the same time. The grade costs 0.7 / 2.1 / 3.7 ms at
640x360, 1280x720 and 1920x1080 — once it runs in a loop of its own. Inside the fused pixel it
kept the whole row's composition from vectorising (+19 ms at 1080p); GCC's jump threading turns
its chain of clamps into branches, so that loop is compiled without it.

`test_temporal.py`'s "a held scene settles" compared a fourth frame's step with the first. Over
twelve frames a held scene keeps moving 0.06-0.2 levels a frame whatever the history's precision —
float, rounded half, truncated half — which is the network's own answer to a history one rounding
different; the old history's fifth step was 0.204 against a first of 0.168. It now asks that the
later steps average no more than the first and none reaches half a level.

## Left different, on purpose or for now

- **Intensity above 1.** The vendor skips its pass at an intensity of 1 or more when nothing else
  asks for it, so there it cannot extrapolate; with a grade its blend weight is unclamped. Ours
  extrapolates always (`phase30`). A knob of ours; unchanged.
- **Detail and colour strength** are MLX-DLSS's frequency split, not the vendor's; OpenDLSS-NR's
  `colorStrength` is its demo's own. Unchanged.
- **`base` in half.** The vendor blends against an RGBA16F snapshot of the frame, ours against the
  float decode — a quarter of a level at most, after the 8-bit encode usually nothing.
