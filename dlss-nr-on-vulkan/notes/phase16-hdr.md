# HDR: the display codec, so the model can be pointed at a game frame
2026-09-09

The network was trained on a display-referred sRGB picture in [0, 1]. A game renders
scene-linear HDR, where highlights run far past 1. That mismatch has to be solved
before any integration, and it is not solved by clamping.

## Why the obvious two answers are wrong

**Clamp to [0, 1].** Everything above 1 becomes 1, and the model is handed a flat white
patch where the frame had structure. Measured on a synthetic HDR frame built from a
Cyberpunk crop — peak 13.0, 5.1 % of pixels with a channel above 1:

| | peak | pixels above 1 | detail sd in the highlights |
|---|---|---|---|
| source | 13.00 | 5.1 % | **4.6495** |
| clamp, then model | 1.00 | 0.0 % | **0.1224** |
| encode / model / resolve | 15.78 | 5.0 % | **4.5468** |

Clamping destroys 97 % of the highlight structure. The keyboard and the screen in the
test frame come back as flat patches.

**Tone-map down, then invert the curve afterwards.** Unstable, and for a reason that is
specific to this model rather than general: the inverse of a compressive curve has a
large derivative exactly where the curve compressed, so whatever the model changed in
the highlights is amplified by that derivative. A 6 % E4M3 quantum in a bright pixel
becomes a much larger error after inversion.

## What the vendor does instead

`src/ref/nr_display.py`, ported from MLX-DLSS's `NeuralRenderingDisplayCodec.swift`
(Apache-2.0; the luminance-ratio composition and OkLab correction follow the
MIT-licensed RenoDX design).

**`encode`** divides by the white point, applies a soft knee to *luminance* above 0.75 —
`0.75 + 0.25 * (1 - exp(-(L - 0.75) / 0.25))` — and converts to sRGB. The knee is on
luminance rather than per channel, so a bright saturated highlight rolls off without
changing hue.

**`resolve`** never inverts anything. It takes the ratio the model produced against the
proxy and applies it to the **untouched original**:

```
ratio    = L_original / L_proxy                       when the original is darker
         = (L_model + max(0, L_original - L_proxy)) / L_model   otherwise
corrected = hue_correct(model * ratio, toward = model)
upgraded  = mix(original, corrected, transfer_strength)
result    = mix(original * clamp(L_upgraded / L_original, 0, max_ratio), upgraded,
                colour_strength)
```

so a highlight the proxy compressed comes back at the original's own level, carrying
the model's *relative* change. `hue_correct` keeps the rescaled pixel's lightness but
takes the model's hue at its own chroma, in OkLab, clamped through the AP1 primaries so
folding negatives out keeps saturated colour instead of clipping a channel to zero in
Rec.709.

Where the model says nothing — `L_model <= 1e-5` — the original passes through
untouched, and `transfer_strength = 0` is an **exact** no-op on the whole frame. That
is the test that says the codec is wired in correctly rather than merely producing
something plausible, and it passes byte for byte.

## Verified

`src/ref/test_nr_model.py`, no network needed: the sRGB transfer round trips, OkLab
round trips, the proxy stays inside [0, 1], the knee leaves ordinary luminance
untouched, zero transfer strength is exact, a display-referred input is not encoded
twice, highlights survive the round trip (peak 12.99 -> 12.82), and the resolve cannot
go negative.

End to end with the resident network in the loop, the model's effect carries: 0.0178 on
the proxy becomes 0.0059 on the non-clipped part of the HDR frame, and the peak is
preserved.

## What it does not settle

The white point. `white_point = 1` treats scene-linear 1.0 as display white, which is
what the test above assumes. A real integration gets that number from the swapchain's
colour space and the game's own exposure, and neither is available here. The codec
takes it as a parameter and the rest follows; nothing about it can be checked without
a game.
