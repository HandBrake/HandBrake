# Phase 37 — Neural Upstream, and why it needs a neural upscaler to work

2026-09-10. Community mods for the real feature — matiasLombo's **Neural Upstream**,
shipped through DLSS5-Autopilot — move the neural pass *before* the game's upscaler, so
it runs at the render resolution rather than the output one. Reported gains are large:
31 to 50 fps on an RTX 4080 in Assassin's Creed Shadows at 4K, against 63 with the
effect off.

The premise applies to us exactly. This network's cost follows the extent it is given —
`17 ms + 488 ms per megapixel` (`notes/phase25`) — and nothing about the content. So
half the extent should be near a quarter of the cost.

Measured on a real captured frame of Dead or Alive 5, 1280x720:

| run at | ms | fps | change from source | detail kept |
|---|---|---|---|---|
| 1280x720 | 876 | 1.14 | 0.02984 | — |
| **640x360, enlarged back** | **293** | **3.42** | 0.04709 | 83 % |

**Three times faster.** That part holds.

## But the 83 % is the wrong number

Splitting what survives into frequency bands says something different:

| | low frequency | **high frequency** |
|---|---|---|
| full resolution (control) | 100 % | 100 % |
| half resolution, enlarged | 84 % | **62 %** |

The high band is the detail synthesis — the entire point of the network. Only 62 % of
it survives, and the half-resolution run puts *more* total energy into that band than
the full run does (0.00690 against 0.00296) while only 62 % of it points the right way.
It is not producing less detail; it is producing detail at the wrong scale, because it
was synthesised for a frame half the size and then enlarged.

A plain resample of the source, with no network at all, changes the frame by 0.00596 —
so the enlargement itself is nearly free. What is lost is not lost to the resampler's
error. It is lost because **Lanczos cannot reconstruct what the network drew.**

Side by side (`work/doa5shots/upstream.png`, source | full | half enlarged | resample
only), the half-resolution result sits much closer to the plain resample than to the
full-resolution enhancement: the hair does not separate into strands and the lashes do
not resolve.

## Which is the whole point of the mod, and the thing we do not have

Neural Upstream works because **DLSS does the upscaling afterwards** — a neural
upscaler with motion vectors and history, which reconstructs high frequency rather than
interpolating it. The enhancement drawn at render resolution survives because something
intelligent enlarges it.

We hand the frame back at the size we got it and have no upscaler at all. Our version of
the same trick keeps the speed and loses most of what the speed was for.

**What it would take to have it properly**: an upscaler downstream of the pass. Either
the game's own — which is the Dead or Alive 6 route, since that title carries FSR2 and
XeSS and both run on this hardware — or our own, which is a second network.

So this is not a dead end, it is a dependency. It also makes the DoA6 interception more
valuable than it looked: that route would give the pre-interface insertion point, the
depth and motion guides, *and* the smaller extent, all from the same hook.
