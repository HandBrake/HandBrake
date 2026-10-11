# Phase 35 — what the operator actually does, on three real game frames

2026-09-10. Three frames of Dead or Alive 5, chosen by the owner to span the range: a
post-match close-up, a leaderboard that is pure 2D interface, and a lobby with half the
screen in true black. Same weights, same controls, same 1280x720. The first measured
description of this network's behaviour on content it was not shown by us.

| frame | change |
|---|---|
| 001 post-match close-up | 0.02983 |
| 003 dark lobby | 0.02797 |
| 002 leaderboard, pure 2D | **0.01920** |

## Brightness drives it, not contrast

Correlating the per-tile change against candidate drivers, 16x16 tiles:

| frame | r(brightness) | r(contrast) | r(product) |
|---|---|---|---|
| 001 close-up | 0.149 | 0.141 | 0.159 |
| 002 leaderboard | **0.855** | 0.667 | 0.685 |
| 003 dark lobby | **0.849** | 0.403 | 0.557 |

By region on the dark lobby:

| region | change | contrast | brightness |
|---|---|---|---|
| **black void** | **0.00392** | 0.005 | **0.000** |
| character | 0.03377 | 0.225 | 0.202 |
| **lit floor** | **0.05075** | 0.189 | **0.467** |
| HUD bars | 0.02828 | **0.298** | 0.148 |

The lit floor takes the most while having *less* contrast than the character, and the
HUD bars have the most contrast in the frame and take less than the floor. Black takes
essentially nothing — 0.0039, thirteen times less than the floor.

So the operator is not a sharpener and not a face detector. **It spends its effort where
the detail would be visible**: it works in a display-referred space, where the eye's
just-noticeable difference scales with luminance, so it does nothing in the dark and
most where it is bright.

And where a frame is evenly lit and full of real content — the close-up — luminance
stops predicting anything (r = 0.15) and the effect follows what the surface *is*: skin,
hair, lashes. That is the trained distribution asserting itself once the luminance
gradient stops dominating.

## The first functional gap between our port and the original

On the leaderboard the change is real — 0.0192 — and it is **not an improvement**. The
gradient bars behind the rows flatten and darken; the crop's contrast falls from 0.1751
to 0.1489. The network synthesises plausible micro-structure, and a game's interface is
not where that belongs. The measurements show why it happens: interface elements are
bright, high-contrast marks on dark ground, which is exactly what the operator considers
worth working on.

The shipped feature does not have this problem, and the reason is on the owner's own
screenshot of RenoDX's panel: **"NR UI Correction"**, and beneath it
*"Insertion: immediately after the game's NGX DLSS output; UI remains downstream."*
The vendor inserts the pass **before** the interface is drawn. Our Vulkan layer takes
the frame at `vkQueuePresentKHR`, which is after everything, interface included.

This is the first difference between the port and the original that is about **what it
does** rather than how fast it does it. Everything before this has been performance.

### What could be done about it

Not by hooking earlier — intercepting a game's render passes rather than its swapchain
is a different and much larger project, and engine-specific.

The tractable route uses machinery already recovered: `make_features` takes a
**control mask**, whose red channel scales intensity per pixel
(`notes/phase30-control-atlas.md`). A UI mask fed there would switch the pass off
exactly where the interface is, and nowhere else. Interface pixels are also the easiest
thing in a frame to find — they are the ones that do not change between two consecutive
presents while the scene does. The layer already holds a previous frame for the temporal
path.

Worth doing, and it is a *quality* improvement rather than a speed one, which is the
first of those this project has had available in a while.
