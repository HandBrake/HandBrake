# Phase 44 — the profiles are a measured trade-off, and one knob runs backwards

2026-09-10, 18:0x. A cutscene frame from DoA5 — Kasumi masked and in shadow beside a
second character in armour — put through the graph four times **on the same input**, so
this is a controlled comparison rather than four different scenes.

## Why the frame was worth it

The live frame showed the pass behaving in opposite directions in different places:

| region | luma | relative fine texture |
| --- | --- | --- |
| shoulder, blown-out skin | 162.4 -> 123.7 | **+52.0 %** |
| armour | 160.8 -> 151.4 | +20.0 % |
| second face | 119.0 -> 105.3 | +1.0 % |
| **Kasumi's face, in shadow** | 51.8 -> 43.5 | **-13.9 %** |

The loss is real and it is specific: her eyes. Peak in the left eye fell **204 -> 117**,
iris saturation **35.4 -> 16.8**, local contrast **107 -> 59**. Meanwhile the fabric of
her mask *gained* — contrast 22.3 -> 34.3, lifted out of near-black. So the pass lifts
shadow detail and crushes speculars, and on a dark face held together by two bright
irises that is a net loss.

## The four runs

Same 1920x1080 input, `src/ref/nr_frame.py --resident`:

| variant | eye peak | eye saturation | eye contrast | skin luma | skin rel. texture |
| --- | --- | --- | --- | --- | --- |
| input | 204 | 35.4 | 107.3 | 162.4 | — |
| `--profile cinematic` | **169** | **32.7** | **97.0** | 157.0 | +3.7 % |
| `--profile natural` | 144 | 26.6 | 86.6 | 151.7 | +8.9 % |
| `--profile standard` (default) | 117 | 16.8 | 59.0 | 123.7 | **+52.0 %** |
| `--colour-strength 1.5` | 87 | 8.8 | 40.7 | 104.3 | **+80.0 %** |

It is a clean monotone curve. **Everything the pass adds to skin texture it takes out of
speculars and colour**, and the profile chooses where on that curve to sit. There is no
setting that does both, at least not among these.

Visually the ranking is unmistakable at 5x on the eyes: cinematic keeps the amber irises
almost as the game drew them, standard mutes them, and `colour-strength 1.5` leaves them
nearly grey.

## The knob runs backwards from its name

`--colour-strength 1.5` does **not** restore colour. It is the most aggressive setting of
the five — iris saturation 16.8 -> 8.8, half again below the default. It scales the
strength of the pass's colour term, not the colour that survives. Worth stating plainly
because the name invites the opposite reading, and this project spent a measurement
finding out.

## What to use

Nothing here says one profile is correct; it says the choice is a real one with a
measured cost. For a photo mode on faces where the eyes carry the shot, **cinematic** is
the defensible default — it keeps 83 % of the highlight and 92 % of the iris saturation
while still adding texture. For blown-out skin and armour, **standard** is worth four
times the texture. The daemon takes `--profile`, so this is a launch-time choice today
and could be a per-frame one if it ever matters.

Frames: `work/doa5ab/`.

## The same comparison on the best face of the session

Frame 002's input — Hitomi in close-up, the largest face this project has run — through
`standard` and `cinematic`, same input, `--resident`:

| | face luma | face saturation | relative fine texture | iris luma | iris hue |
| --- | --- | --- | --- | --- | --- |
| input | 136.1 | 87.3 | — | 93.3 | 18° |
| `standard` | 87.4 | 60.0 | **+30.6 %** | 57.5 (-38 %) | 21° |
| `cinematic` | 121.3 | 72.2 | **-8.8 %** | 77.4 (-17 %) | 13° |

Note the sign: on *this* face cinematic **removes** relative texture, where on the frame
008 cutscene it added 3.7 %. The trade-off curve is real but its zero point moves with the
scene, so "cinematic adds less texture" is the safe statement and "cinematic adds a
little" is not.

Visually the two are different pictures rather than two strengths of one. Standard turns
the game's waxy, near-clipping skin into something photographic — pores, freckles across
the cheeks and nose, subsurface reddening — at the cost of a much darker face. Cinematic
keeps the game's own look and adds a modest amount.

**A correction.** Looking at the crop I claimed standard had turned the irises from
blue-grey to brown, i.e. changed the character rather than the detail. That is wrong, and
the measurement is what caught it: sampling the actual iris pixels, they are brown in the
input too (R119 G87 B73), and the hue barely moves — 18° to 21° under standard, and
standard slightly *raises* iris saturation, 0.38 to 0.47. What actually happens is that
the eye darkens by 38 %, and in a face that has darkened with it the eye reads as duller.
The conclusion — cinematic is the safer default for faces — survives; the reason given
for it did not.

## Correction: `standard` is the better default for this game, not `cinematic`

Frame 010 — Helena on sand, a photo-mode shot, bright and washed out — put through both
profiles on the same input:

| | sand | face | hair | skin |
| --- | --- | --- | --- | --- |
| `cinematic` | **-17.7 %** | **-5.6 %** | **-11.1 %** | -11.5 % |
| `standard` | +3.8 % | **+34.2 %** | **+27.5 %** | -5.1 % |

Cinematic does not merely add less here — it **removes** detail, smoothing the sand grain
and softening the face. Standard adds a third again to the face and a quarter to the hair.
At 6x the difference is not subtle: standard gives a modelled face with a defined eye and
formed lips, cinematic gives something close to the flat input.

So the recommendation written above — "for a photo mode on faces, cinematic is the
defensible default" — is **wrong**, and it was wrong because it generalised from a single
cutscene frame where cinematic happened to land slightly positive. Three frames now say
otherwise.

The rule that actually holds across all of them: **standard always adds more texture, and
cinematic can go negative.** Standard's cost is a large tone drop. And in *this* game that
cost is mostly a benefit — DoA5 renders skin near clipping (Hitomi's face at luma 136 with
saturation 87, Helena's at 162), so pulling the level back is restoring contrast the
shader threw away, not darkening a correct picture.

**Use `standard` for DoA5.** Reach for `cinematic` when the goal is to keep the game's own
look rather than to get the most out of the frame — and know that on a bright scene it
will smooth rather than sharpen.

The daemon has been running `--profile cinematic` since 18:13; it should go back.
