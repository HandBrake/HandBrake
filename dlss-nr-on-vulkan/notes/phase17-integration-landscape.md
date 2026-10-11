# Getting into a game: what everyone else does, and why we need a different door
2026-09-09

The owner asked for a survey of the ecosystem, including the dual-GPU work, before
attempting an integration. Here is what is out there as of today, what is genuinely
new, and what it means for a machine with no NVIDIA GPU in it.

## What exists

| | what it is | needs |
|---|---|---|
| [Dagherbou/OptiScaler_DLSSNR](https://github.com/Dagherbou/OptiScaler_DLSSNR) | OptiScaler fork; runs the NR pass directly, no ReShade. Has a `setup_linux.sh` for Proton | NVIDIA GPU, DX12 |
| [NIGos/dlss5-bridge](https://github.com/NIGos/dlss5-bridge) | ReShade add-on; mirrors a DX11 or Vulkan game's DLSS onto a **private D3D12 session**. Colour, depth and motion vectors go into shared textures, are evaluated there, and are copied back | NVIDIA GPU |
| **MGPU Bridge** (Guibout, 7 Sept 2026) | offloads NR to a **second GPU**; the game renders on one card, NR runs on the other. 44 -> 67-70 fps in Dawnwalker, up to +127 % elsewhere | two RTX 50 cards, DX12, a monitor on each |
| [jlrouzies-fr/DLSS5-Feeder](https://github.com/jlrouzies-fr/DLSS5-Feeder) | for games with **no DLSS at all**: synthesises the DLAA contract from ReShade's backbuffer, hardware depth and **optical-flow motion vectors from a third-party shader** | NVIDIA GPU |
| packaging and injection variants, several | how the pass is shipped to end users | NVIDIA GPU |
| [danielblnc/DLSS-NR-on-AMD](https://github.com/danielblnc/DLSS-NR-on-AMD) | runs NVIDIA's DLL on RDNA3/4 by translation; ~33 fps at 1080p on an RX 9070 XT | AMD, DX12, an FSR game |

**Every one of them loads `nvngx_dlssnr.dll`.** OptiScaler's Linux script included: it
gets the pass onto Proton, not onto other hardware. There is still no Intel
implementation of any kind, which is what this project is.

## The two findings that matter for us

**1. MGPU Bridge proves the pass can be decoupled from the renderer.** Grab the frame,
hand it to a different device, run NR there, hand it back — and it is *faster*, not
slower, because the copy costs less than the compute it displaces. Our situation is the
same decoupling with the transfer removed: the "other device" is the same iGPU the game
is already rendering on.

**2. DLSS5-Feeder proves motion vectors need not come from the engine.** It reads a
ReShade optical-flow shader and converts its output into the contract — motion in
pixels as RG16F, depth as R32F, and a mask flagging vectors it does not trust. So the
temporal path does not require engine integration; MLX-DLSS's own `FlowMotionEstimator`
(OpenCV DIS, with a forward/backward confidence check) is the same idea on our side.

## Why a Vulkan layer is the right door here

Under Proton a DX12 game runs DX12 -> VKD3D-Proton -> **Vulkan on ANV**, which is the
driver our compute shaders already use. DX11 and DX9 go through DXVK to the same place.
So the frame we want is already a `VkImage` on the device we already run on: no D3D
interop, no Windows DLL, no PCIe transfer, and it works for native Vulkan games too.
[vkBasalt](https://github.com/DadSchoorse/vkBasalt) has done exactly this on Linux for
years, so the shape is not speculative.

Nothing in the NVIDIA-side ecosystem can be reused directly — they are all Windows C++
loading a CUDA DLL — but the *contract* they reverse-engineered is what we already have.

## The capture half works

`src/layer/nr_layer.c`. It chains into the loader, adds `TRANSFER_SRC` to the swapchain
images on the way down (without it the copy is invalid, and `vkCreateSwapchainKHR` is
the only place it can be added), and copies the presented image into a host-visible
buffer.

Tested against `vkcube` on this machine: it reports the swapchain, captures on demand,
and the frame decodes correctly — 500x500, `VK_FORMAT_A2B10G10R10_UNORM_PACK32`, the
LunarG cube. That the compositor hands out a **10-bit** format is itself worth noting:
it is the HDR-capable path, which is what `notes/phase16-hdr.md` exists for.

`vkQueueWaitIdle` before the copy is the blunt way to be sure the frame is finished —
the proper route waits on the present's own semaphores, which means taking them over
from the application. For an on-demand capture the stall costs one frame; it has to
change before the pass runs every frame.

## What this can honestly be

The arithmetic floor from `notes/phase11-what-is-left.md` has not moved: 1.56 s for a
720p frame, so **0.64 fps**. A live in-game pass is not on the table and never was on
this hardware. What is on the table:

- **A photo mode.** Freeze, capture, run the pass, show the result. Two seconds is
  nothing when the game is paused, and this is where the model's effect is worth
  looking at anyway.
- **Offline video**, a frame at a time.
- **A region rather than a frame** — a face-sized 384x384 crop is 0.26 s.

## What is left to build

The write-back — upload the result and blit it into the swapchain image — and the
bridge from the layer to the implementation. The layer is C inside the game's process
and the implementation is Python, so the first version should pass the frame over a
socket to a resident daemon rather than porting the orchestration to C. For a
freeze-frame that latency is free.
