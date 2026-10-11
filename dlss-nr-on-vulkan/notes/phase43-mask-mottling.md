# Phase 43 — the mask made a worse artefact than the one it prevents

2026-09-10, straight after `notes/phase42` proved the mask on a live HUD. Three more
frames in the same session found the case where it does harm, and the cause is the
interaction of two things that were each fine alone.

## What it looked like

Frame 004: a round transition, Kasumi on one knee, score card "01-01" on screen. The
output came back with the skin and armour covered in a mottled crust — not detail,
damage. The input is clean.

## What it was

The layer marks **single pixels** that held still between two presents. In a fight that
is the HUD. In a near-static moment it is also most of the subject, because the subject
is not moving either. Composing then interleaves two populations at pixel scale: the
protected pixels as the game drew them, and the re-rendered pixels — which the pass has
moved by 20-30 levels, since it pulls back blown-out skin (`phase42`).

| region | protected | protected pixels | re-rendered pixels | gap |
| --- | --- | --- | --- | --- |
| thigh and leg | 60 % | 130.5 | 102.5 | 28 |
| face | 16 % | 134.8 | 91.1 | 44 |
| armour, chest | 27 % | 213.9 | 114.0 | **99.8** |

Local speckle rose **1.6x to 4.8x**. A hundred levels between neighbouring pixels is the
crust.

Neither half is a bug. A per-pixel mask is right for an interface. A 30-level tone move
is what a detail re-render trained on real skin does to a game's over-bright shader. Put
together at pixel granularity they produce something worse than either problem alone —
worse, certainly, than the softened health bar the mask exists to prevent.

## Two fixes, and only the second one worked

**A majority filter** (`solid_regions`, radius 3, 75 % of a 7x7 window) narrows the mask
to solid held-still blocks. On synthetic masks it separates cleanly — an 85 %-dense block
keeps 91 %, a 60 %-dense speckle keeps 1 %, and the transition is steep. On the live game
it cut the fragmentation of the mask boundary from **16.5 % to 4.8 %** of protected
pixels and the speckle amplification from **1.69x to 1.11x**.

It was not enough. Frame 005 still showed patches — larger and smoother, still wrong.
Filtering the mask treats the symptom: the mask was covering the character, and a cleaner
mask over the character is still a mask over the character.

**A coverage limit** is the actual fix. An interface is a small part of a frame; when the
held region is most of it, what is standing still is the scene. The daemon now drops the
mask above 55 % coverage and says so:

```
interface mask covers 68% of the frame; that is the scene holding still, not a HUD — mask dropped
```

Frame 006, the same scene as 004 and 005, came back clean.

The evidence for the number, three live frames:

| held still | result |
| --- | --- |
| 43 % | clean; HUD protected, ten times less damage (`phase42`) |
| 69 % | mottled |
| 75 % | mottled, less so after the majority filter |

55 % sits between them. The layer's own refusal threshold is 90 %, which is far too
permissive and should follow this down; it is left alone for now because changing it
needs the game restarted, and the daemon-side limit covers the same ground.

## What this says about the feature

The mask is worth having — `phase42` measured a tenfold reduction in HUD damage on a real
fight frame. But it is only safe when the scene is genuinely moving, and "the scene is
moving" is exactly what the detector cannot verify from two presents. Both guards now in
place are heuristics about *shape* and *extent* rather than about interfaces, and they
work because an interface is small and solid while a static scene is large and ragged.

Tests: `src/layer/test_ui_mask.py` now checks the interior of a block is still exact,
that the narrowing only eats the boundary, that an 85 % block survives while 60 % speckle
does not, and that the coverage limit sits between the measured clean and blotched cases.

## Stress case: contre-jour, and a caveat about the metric

Frame 007, 17:57 — Phase 4 vs Christie on the bright cyan stage, dark silhouettes
against a blown-out background. 32 % held still, narrowed to 13 % by the majority
filter, mask applied.

| region | luma | mean abs diff | relative fine texture |
| --- | --- | --- | --- |
| skin, back | 126.5 -> 107.3 | 25.00 | **+12.6 %** |
| dark silhouette, left | 113.2 -> 104.2 | 13.56 | +5.4 % |
| dark silhouette, right | 121.6 -> 115.6 | 9.73 | +8.7 % |
| blown-out background | 139.8 -> 143.9 | 4.49 | +0.1 % |
| HUD, health bar | 77.8 -> 77.3 | 1.19 | -0.9 % |

Clean: speckle amplification **x1.02**, frame brightness unmoved (127.9 -> 127.7), the
interface protected. The model recovered form from a blown-out shoulder — the input back
is a flat pink field, the output has shading, a spine line and a shoulder blade — and
left the blown-out background alone, which is the right call since there is nothing there
to recover.

**The caveat.** Boundary raggedness on this frame is **20.5 %**, *higher* than the 16.5 %
of the mottled frame 004, and yet there is no artefact. So raggedness alone does not
predict the damage: the artefact needs a fragmented boundary **and** a level gap across
it. Here the mask covers the HUD, which sits at luma 78 against surroundings at a similar
level, so the fragmentation costs nothing. On frame 004 the mask covered armour at luma
214 next to re-rendered pixels at 114. Any future guard should look at the product, not
at either factor alone.
