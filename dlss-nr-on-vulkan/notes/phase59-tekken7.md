# Phase 59 — a third kind of client, and the best live rate so far

2026-09-16. **Tekken 7**, Steam appid 389730, Unreal Engine 4,
`TekkenGame-Win64-Shipping.exe`. **10.5 fps sustained at 640x360**, measured over 20
seconds of play: 210 frames, 91 ms each.

## What it settles

**The layer attaches to 64-bit D3D11 through DXVK**, with no change to anything. That is
the third kind of Vulkan client it has run under:

| game | API | bits | translation | note |
| --- | --- | --- | --- | --- |
| Dead or Alive 5 | D3D9 | 32 | DXVK | `phase34`, needed the 32-bit layer library |
| a D3D12 title | D3D12 | 64 | VKD3D-Proton | `phase41`, attach proven, no picture |
| **Tekken 7** | **D3D11** | **64** | **DXVK** | here |

The API was not taken on trust. UE4 carries the names of every RHI module — `D3D11RHI`,
`D3D12RHI`, `VulkanRHI`, `OpenGLDrv` — in the string table of any build, so the binary
proves nothing; the game's configuration is inside its pak files. What settles it is
`/proc/<pid>/maps` on the running process: `d3d11.dll` and `dxgi.dll` mapped, and
`libnr_layer` present in five mappings.

## Measured

| | |
| --- | --- |
| swapchain | 640x360, no letterbox |
| render scale | 0.55 |
| frame | **91 ms**, 210 frames in 20 s = **10.5 fps** |
| history gate | **0.573** mean, 0.556-0.661 over the window |
| frame held by the floor | **86 %** |
| frame-to-frame change | 0.006-0.022, far under the 0.15 cut limit |

Power-saver, 1.38 GHz, on battery — as `phase57`, so the two are comparable with each
other and not with anything earlier.

**No letterbox.** UE4 hands over a swapchain the size of the window. Dead or Alive 5
letterboxes 16:9 inside a 4:3 window and a quarter of every frame was bars (`phase51`),
which is why `Letterbox` exists; here it finds nothing and costs its eight lines.

## The gate is the highest this project has seen

0.573 mean, against 0.38-0.54 in a Dead or Alive 5 fight and 0.12 on the slow replay that
`phase54` was measured on. The reason is what that phase predicted: the gate is **global**,
so it reads how much of the *whole frame* agrees with its history. Tekken's camera barely
moves, one fighter occupies the middle, and the background is still — so the history is
correct over most of the frame and the model trusts it. A DoA5 fight moves the camera.

With 86 % of the frame also getting the floor, this is the most stable live picture the
project has produced, and it is stable for a reason that is a property of the game rather
than of anything done here.

## On "the best rate so far"

`phase47` recorded 10.6 fps, at **512x288**. This is 10.5 fps at 640x360 — **1.56x the
pixels for the same rate**. The honest caveat: the power profile of that earlier
measurement is not recorded, and this one is on power-saver, so the comparison is
suggestive rather than clean. What is not in doubt is that the frame at this extent is
91 ms today and the `phase47` table has 640x360 at scale **0.35** costing 116.8 ms — a
lower render scale for a longer frame. The native host passes (`phase57`) are the
difference.

## Before and after, measured

A still of **Sergei Dragunov** on the customisation screen — beret, grey hair, beard, a
jacket with embroidered eagles — at **1920x1080 and render scale 1.0**, so the detail is
drawn at the scale it is shown at (`phase37`: a smaller extent keeps only 62 % of the high
band). Four frames dumped; the third, with history active (gate 0.641, 97 % held).

Relative texture is the luma high-pass RMS divided by the region's own mean, because on
this model the level moves and the level fools the eye (`HANDOFF`, traps):

| region | luma | relative texture | mean colour change |
| --- | --- | --- | ---: |
| face | 78 -> 57 | **+22 %** | 23.5 |
| beard | 103 -> 83 | +16 % | 23.7 |
| eagle embroidery | 135 -> 115 | +36 % | 20.6 |
| jacket weave | 90 -> 71 | **+50 %** | 19.4 |
| background, hangar | 35 -> 34 | -9 % | **4.8** |

Three things, all consistent with Dead or Alive 5:

- **The subject is re-rendered and the background is left almost alone** — a colour change
  of ~20 levels on the character against 4.8 on the hangar behind him.
- **The level comes down 15-27 % on the subject.** The game's face is bright and flushed
  orange-red around the eyes and cheekbones; the pass takes the flush out and the
  brightness down, and puts texture in — pores and fine lines around the eyes, individual
  beard hairs, a sharper weave in the jacket.
- **Speculars go.** The brass buttons lose most of their shine. That is `phase44`'s trade
  seen again on a different game and a different art style: what goes into texture comes
  out of speculars and colour.

Whether the result is *better* is not a measurement. It is photographic where the game is
stylised, and the flush it removes may well have been the artists' choice.

`work/tekken/dragunov_before_after.png`, not committed — game frames are somebody else's.

## A mistake worth writing down

The daemon was started by the toggle, and this session deleted `/tmp/nr_daemon.log` before
starting a second daemon — which exited, correctly, because one was already listening. The
first daemon's output then existed only through `/proc/<pid>/fd/1`, an unlinked inode. It
was readable there, and the measurements above came from it.

`nr-panel` reads the log **by path** and would have shown nothing. Deleting a file another
process is writing to does not give you a fresh one; it gives that process a private one.

## Re-measured 2026-09-24: 25 fps

The same game in the same arrangement — live, every present through the network — after the
fusions, the staged GEMM's shared memory and its partial blocks, with the owner playing and the
daemon's log counting:

| game | network | scale | frames | per frame | graph |
| --- | --- | ---: | ---: | ---: | ---: |
| 640x360 | 320x320 | 0.5 | 745 | 40 ms, **25 fps** | 30 ms |
| 640x360 | 384x320 | 0.6 | 717 | 40 ms, **25 fps** | 33 ms |
| 640x360 | 320x320 | 0.35 | 1142 | 40 ms, **25 fps** | 31 ms |
| 800x450 | 320x320 | 0.35 | 1597 | 40 ms, **25 fps** | 32 ms |
| 640x360 | 512x320 | 0.75-0.8 | 64 | 50 ms, 20 fps | 41 ms |
| 640x360 | 640x384 | 1.0 | 195 | 70 ms, 14 fps | 59 ms |
| 800x450 | 832x512 | 1.0 | 243 | 110 ms, 9 fps | 90 ms |

The log rounds a frame to 10 ms, so read 25 as 22-28; the owner saw 25 on screen. Against the
10.5 fps above: 2.4x. At 640x360 scale 0.6 costs what 0.35 does — the network is the same size
bar one 64-column step, and it holds three times the real pixels.

## A larger window, 2026-09-25

Tekken at **1280x720**, render scale 0.35 (a 448x320 network), practice mode, the owner
playing, counted from the daemon's log in 15-second windows. The daemon was restarted twice
mid-session, to take the host passes' threads away and give them back:

| host passes | fps | daemon | graph |
| --- | ---: | ---: | ---: |
| eight cores (OpenMP) | 17.3-17.4 | 50 ms | 41 ms |
| one core (`OMP_NUM_THREADS=1`) | 14.6-14.7 | 60 ms | 41 ms |
| eight again | 17.2-17.5 | 50 ms | 41 ms |

**+18 % in the game**, all of it in the passes around the network: the graph is the same
41 ms (mean 39.8-40.0) in all three legs. At 640x360 the same change moved nothing, because
there the graph is the frame. The log rounds the daemon's time to 10 ms; the frame counts do
not round.
