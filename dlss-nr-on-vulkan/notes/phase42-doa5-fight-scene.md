# Phase 42 — the pass on a live fight, measured

2026-09-10, 17:16. Dead or Alive 5 Last Round running from Steam with the layer in its
launch options, daemon started separately with `--dump`. Ayane vs Raidou on the rooftop
stage, 1280x720, `VK_FORMAT_B8G8R8A8_UNORM`. One triggered frame, 2.37 s wall in the
daemon — that figure includes the socket round trip and writing two PNGs, not just the
~490 ms of graph.

This is the first in-game frame this project has measured rather than looked at.

## What the model did

| region | mean abs diff /255 | high-frequency energy |
| --- | --- | --- |
| whole frame | 5.37 (max 49) | +1.9 % |
| Ayane, face and hair | 9.37 | +6.4 % |
| Raidou, head and shoulder | 4.35 | **+36.1 %** |

**75.8 % of pixels moved by more than 2/255.** At full-frame scale the picture looks
almost unchanged, which is the honest impression and also why the crops matter: this is
a fighting-game wide shot, the characters occupy a small part of the frame, and the
model works on exactly the parts it was trained for. Zoomed 4x, Ayane's hair separates
into strands, the flower ornament recovers petal edges, and the eye and lips sharpen;
Raidou's mask picks up leather grain and his shoulder plate recovers form.

The +36 % on Raidou against +6.4 % on Ayane is the interesting split. His head is dark,
low-contrast and mostly texture; hers is already high-contrast hair against a bright
ornament. The model has the most to add where the input carries the least.

## The interface, and the case for the mask

`NR_LAYER_UI_MASK` was **not** in the game's environment for this run — the mask was off.
The result is the first direct measurement of what that costs:

```
health bar and name    mean |d| 5.98   max 38
DXVK overlay text      mean |d| 3.77   max 15
```

The health bar loses its dark outline, its edges bleed into the background, and the
"AYANE" lettering softens. The model treated the HUD as scene content, which is exactly
what it should do with no mask and exactly what the mask exists to prevent.

So the mask is still unproven on a live HUD — but the thing it is meant to prevent is now
measured, on a real frame, instead of argued. `notes/phase35-what-the-operator-does.md`
for the detector, `notes/phase39-layer-review.md` for the bug that stopped it working at
all until today.

Frames and crops: `work/doa5live/`.


## A close-up, and a correction to what "more detail" means

17:31, same session, a story-mode still of Hitomi at **1920x1080**, 5.05 s in the daemon.
The largest face this project has put through the pass. First impression from the two
full frames was "pores and freckles appeared". The measurement says that is half right,
and the half it gets wrong matters.

```
face, mean colour   in  R 184.9  G 125.3  B 98.0   luma 136.1   saturation 87.3
                    out R 120.8  G  80.4  B 61.0   luma  87.4   saturation 60.0
```

Mean absolute difference over the face is **48.8/255**, and compensating nothing but the
mean brightness shift drops it to **17.6**. So most of that number is tone, not texture.
Raw fine-texture energy (deviation from a local 3x3 mean) *falls* on the face, by 16 %.

Normalised for level, it rises everywhere:

| region | luma | relative fine texture |
| --- | --- | --- |
| face | 136.1 -> 87.4 | **+30.6 %** |
| hair | 55.0 -> 37.0 | +8.5 % |
| jacket | 60.1 -> 52.8 | +8.6 % |
| background | 41.6 -> 46.4 | +8.9 % |

**The whole frame's brightness is unchanged (-1 %).** The pass is not darkening the
picture; it is specifically pulling back a blown-out skin highlight — the game's shader
puts the face at R=185, near clipping and waxy — and putting structure into the range it
frees. That is a defensible thing for a detail re-render trained on real skin to do, and
whether it is *wanted* is an artistic call rather than a correctness one. The knobs exist
already: `--profile`, `--intensity`, `--detail-strength`, `--colour-strength`, all at
their defaults for this frame.

The lesson for measuring this model: **raw high-frequency energy is the wrong statistic
when the pass also moves the level.** Normalise, or the tone change masquerades as lost
detail. The earlier fight-scene figures in this note are unaffected — the change there
was 5.4/255 with no comparable level shift — but they are the exception, not the rule.

## The mask refused, correctly

Relaunched with `NR_LAYER_UI_MASK=1`, verified present in the game's own `/proc` environ.
The layer said:

```
[nr_layer] ui mask on: the first present after the trigger is kept to find what held still
[nr_layer] 90% of the frame held still; ui mask refused
[nr_layer] processed 1920x1080
```

The frame was a story-mode still. 90 % of it had not moved between the two presents, and
the detector cannot separate an interface over a still scene from a still scene, so it
declined rather than guessed — exactly the designed behaviour
(`notes/phase35-what-the-operator-does.md`). **The mask is still unproven on a live HUD**,
and proving it needs a frame captured mid-round with the characters actually moving.

## The mask, proven on a live HUD

17:37, mid-round: Kasumi throwing Christie in the gym stage, timer at 94, 1920x1080.

```
[nr_layer] 43% of the frame held still; ui mask sent
[nr_layer] processed 1920x1080 with a ui mask
nr_daemon: 1920x1080 B8G8R8A8_UNORM in 3.96s  change 0.01516  interface 44% left alone
```

**44.1 % of the output is bit-identical to the input**, against the daemon's reported
44 % — the two halves of the protocol agree exactly, so the mask plane survives the
socket and is applied per pixel as intended. This is the first time the masked path has
run outside a test.

The acceptance question is not the percentage, it is whether the interface survives:

| | mean abs diff on the health bar | untouched |
| --- | --- | --- |
| frame 001, no mask | 9.69 | 0 % |
| frame 003, mask | **0.97** | **85 %** |

**Ten times less damage to the HUD.** Visually the bar keeps its dark outline and the
"KASUMI" lettering stays crisp; without the mask the outline was gone and the edges bled.

The discrimination is real and points the right way:

| region | protected |
| --- | --- |
| health bar, left | 84.7 % |
| name "KASUMI" | 93.0 % |
| Kasumi herself | 13.9 % |
| Christie | 25.9 % |

The fighters get processed, the interface does not. That is the whole design working.

### Where it is imperfect, and why

Protection is not total. The timer reads 40 % and the right-hand name 37 %, and the
static background comes out speckled at 38-62 % rather than uniformly held. The detector
is `|dR| + |dG| + |dB| > 6` between two consecutive presents, which is a real tolerance
and not exact equality, so the cause is not quantisation:

- the **timer counts**, so its digits genuinely differ between the two presents;
- DoA5's health bars carry an **animated shine**, so part of the bar genuinely moves;
- **bloom from the moving fighters** bleeds onto neighbouring pixels, including HUD edges.

None of these are detector bugs; they are the interface genuinely changing. Raising the
threshold would catch them and would also start protecting slow scene content, which is
the failure the current setting avoids. A better answer, if this is ever worth improving,
is to accumulate over more than two presents rather than to loosen the comparison — but
at a tenfold reduction in HUD damage the feature already does its job, and this is
recorded as a limit rather than a defect.

**All three open items from the earlier HANDOFF list are now closed**: a game frame worth
looking at, the interface mask on a live HUD, and — from `notes/phase41` — the layer under
a second Vulkan client.
