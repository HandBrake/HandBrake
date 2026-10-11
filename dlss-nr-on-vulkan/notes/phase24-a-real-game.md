# Phase 24 — the pass runs inside a real game, under Proton

2026-09-09. `notes/phase19-photo-mode.md` closed the loop on `vkcube`. The open
question was whether a Vulkan layer written for a native application survives the
Proton stack — DXVK translating D3D11 to Vulkan, inside a Wine prefix, inside Steam's
container. It does.

```
[nr_layer] active; socket=/tmp/nr_layer.sock trigger=/tmp/nr_trigger
[nr_layer] swapchain 1280x720 format 44, 4 images
[nr_layer] swapchain 1920x1080 format 44, 4 images
[nr_layer] processed 1280x720
```

Format 44 is `VK_FORMAT_B8G8R8A8_UNORM` — what DXVK presents, and a different one from
the `A2B10G10R10_UNORM_PACK32` the compositor hands a native application, so both
branches of the daemon's decoder are now exercised by something real. The frame came
back into the game's own swapchain image, `change mean|d| 0.0216`.

The captured frame is a warning screen — 2D interface over pixel art, which is nothing
a model trained on skin and hair has anything to say about. **The pipeline is proven;
the content is not.** Getting to a frame with a face needs someone to drive the game's
menus, which is the one part of this that cannot be done unattended.

## How to run it

Steam launches a game as a child of the client, so the layer's environment does not
reach it unless it is written into the launch options. Running Proton directly sidesteps
that and needs no change to the Steam configuration:

```
python3 src/layer/nr_daemon.py --dump work/gamecap &     # holds the model
src/layer/nr-photo --proton 311730 \
    "/path/to/SteamLibrary/steamapps/common/Dead or Alive 5 Last Round/game.exe"
touch /tmp/nr_trigger     # enhance and hold
rm    /tmp/nr_trigger     # back to the game
```

`--proton` finds the prefix across every library in `libraryfolders.vdf` and the Proton
runtime next to it. The prefix has to exist, which means the game has been started from
Steam at least once. The `--steam` form still prints a launch option to paste, for
anyone who would rather go through the client.

## What this machine has to test with

| game | appid | API | content for this model |
|---|---|---|---|
| Dead or Alive 5 Last Round | 311730 | **32-bit D3D9** | **realistic faces** — the right target |
| Dead or Alive 6 Last Round | 4144680 | D3D11 | realistic faces, 83 GB |
| GUILTY GEAR Xrd -SIGN- | 376300 | D3D9/11 | cel-shaded; good for the D3D9 path, poor for the effect |
| Counter-Strike 2 | 730 | **native Vulkan** | no Proton in the way — but VAC, so not for unattended experiments |
| NEEDY GIRL OVERDOSE | 1451940 | D3D11 (Unity) | 2D; what this phase used, because it reaches a window in seconds |

GG Xrd was tried first and did not reach a swapchain when its executable is launched
directly — it likely wants its own `BootGGXrd.exe` launcher, which needs input.

## Correction 2026-09-10: Dead or Alive 5 is 32-bit D3D9

This note called it D3D11. It is not: `game.exe` is `PE32 ... Intel i386` and imports
`d3d9.dll`. That matters more than the API name — a 32-bit game needs a **32-bit layer
library**, and the one built here was x86_64 only, so it would never have loaded. The
Vulkan loader picks the ABI from `library_arch` in a 1.2.1 manifest;
`src/layer/prepare_layer.py` now writes both, `make test-proton` builds and loads both,
and both report this GPU. `notes/phase32-scratch-and-qk.md`.

## Two things worth knowing before the next attempt

- **The game may ignore a windowed request.** Unity took `-screen-fullscreen 0` and
  went to 1920x1080 anyway, creating a second swapchain. The layer handled both; the
  processed frame was the 1280x720 one.
- **A frame costs what the network costs.** 1280x720 is ~0.6 s now (it was 1.27 s in
  phase 19 — the day's optimisation reaching the integration path), 1920x1080 is 2.9 s.
  The photo mode holds the result on screen while the trigger file exists, so a
  multi-second pass is not a problem; a per-frame pass still would be.
