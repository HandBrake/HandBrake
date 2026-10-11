# Phase 62 — a DX12 game with a picture: Mortal Kombat 1

2026-09-16. **Mortal Kombat 1**, Steam appid 1971870, D3D12 through VKD3D-Proton. The layer
attached, the daemon rendered, and the owner's verdict on the live picture was that it
looked remarkable. That closes the item the brief carried since `phase41`: under VKD3D the
layer had only ever *attached*; this is the first D3D12 title with a picture.

## Running it off the Windows partition without leaving Proton behind

The game lives in the Windows Steam install on the BitLocker-encrypted system partition,
unlocked and mounted by the desktop at `/run/media/<user>/Acer` with `ntfs3`, read-write.
There was no room to move 152 GB anywhere unencrypted.

- **`steamapps/compatdata` became a symlink to the Linux prefix directory** — the same thing
  the owner had already done for the `Files` library. The Wine prefix, with all its symlinks,
  never touches NTFS, and Windows Steam does not use `compatdata` at all.
- **`steamapps/shadercache` was left alone.** It already held `1971870`, Windows Steam's own
  cache for this game; replacing it with a link would have broken Windows.
- **The translation layers' caches were pointed at Linux explicitly**, with
  `VKD3D_SHADER_CACHE_PATH` and `DXVK_STATE_CACHE_PATH` in the launch options. The `proton`
  script does not set them itself, so where they land by default was not something to trust.
- Returning to Windows: unmount first; coming back, restart rather than shut down, or a
  Fast Startup hibernation leaves the partition unsafe to write.

## Full resolution does not fit

Capturing at render scale 1.0 at 1920x1200 **killed the game repeatedly**: the kernel log
reads `Out of memory: Killed process ... (MK12.exe)`, and Steam's service went down with it.
Mortal Kombat 1's own D3D12 buffers and the model's at full scale do not fit together in 15
GiB shared with the GPU. Live at scale 0.55 they did. The usable frames were captured at
1600x900.

The live run's frame times — 136 frames, 0.4 to 3.8 s — are not recorded as a result: a
first launch, shaders compiling, and a resolution change in the middle.

## What the pass does to it, measured

Two comparisons published, chosen by the owner: Omni-Man and Homelander cropped from frame
27, and the whole of fight frame 37.

| region | brightness | relative texture | colour change |
| --- | --- | ---: | ---: |
| Omni-Man's face | 131 -> 132 | **-14 %** | 15.6 |
| Homelander's face | 131 -> 131 | **-24 %** | 14.6 |
| whole fight frame | 56 -> 57 | -2 % | 6.8 |

**The opposite of Tekken 7 and Dead or Alive 5.** There, the character darkens by 15-27 % and
gains texture. Here the brightness does not move and the fine detail on the faces falls. What
changes is the colour: the warm filmic grade and the glow on the skin come out, and the faces
read as photographed under neutral light — which is presumably what looked remarkable.

**It is not film grain.** That was the first hypothesis, because a high-pass measure cannot
tell grain from skin texture. A flat, defocused patch of sky in the same frame carries 0.36
levels of high-frequency energy before and 0.40 after — there is no grain to remove. On
Omni-Man's face the energy falls in absolute terms, 4.70 to 3.98. A defocused background
region falls too, 2.03 to 1.65, which leaves the game's own TAA sharpening as a candidate for
what goes; that is not measured.

The same trade `phase44` found for the profiles: what the pass adds in one place it takes
from another. On a game whose skin is already detailed and whose grade is strong, the work
goes into tone.
