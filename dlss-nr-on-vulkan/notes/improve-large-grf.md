# Xe2's large GRF mode: reachable with a small Mesa patch, and slower with these kernels

**An experiment kept for the future, not part of the pipeline.** Nothing in the tree uses it: the
daemon, the layer and the tests run on the system driver, the patched Mesa is loaded only by hand
(`VK_DRIVER_FILES`), and even then the mode is off without `INTEL_XE2_LARGE_GRF=1`. It is here for
whoever writes a GEMM for 256 registers, or finds the mode worth switching per dispatch.

2026-09-27. `phase26` found the GEMM's ceiling in the register file — 128 registers a thread, the
8x16x16 cooperative-matrix shape and SIMD32 leaving no room for a bigger block — and noted that
Intel hardware has a 256-register mode this Mesa gave no way to ask for. The owner asked to try it,
with the Mesa source already here (`work/mesa-26.2.3`, the build that carries the SLM fix).

## The mode exists, and Mesa leaves it off

- **The hardware.** Mesa's own description of Xe2 (`genxml/xe2.xml`) has `STATE_COMPUTE_MODE`'s
  `Large GRF Mode`, dword 1 bit 15 with its mask bit 31, as Xe-HPC has. Mesa never sets it; its
  instruction validator carries `TODO: Consider Large GRF for certain Xe platforms`.
- **The encoding already reaches it.** Xe2 numbers registers in 64-byte units and takes 0-255 —
  Xe3's variable register allocation uses the same range. What holds Xe2 at 128 is the register
  allocator (`brw_alloc_reg_sets`: `BRW_MAX_GRF` slots below Xe3) and the mode bit.

## The patch

Three places, all behind `INTEL_XE2_LARGE_GRF=1` so one build runs both ways
(`src/probe/mesa-xe2-large-grf.patch`, against Mesa 26.2.3):

- the allocator gets Xe3's 256 slots on Xe2;
- the register-pressure threshold that decides whether SIMD32 is even tried goes 134 -> 268;
- anv sets `LargeGRFMode` in both queue inits' `STATE_COMPUTE_MODE` — the whole process, every
  compute dispatch.

Built with `ninja -C work/mesa-26.2.3/build`, loaded with `VK_DRIVER_FILES=.../intel_devenv_icd.x86_64.json`
and `MESA_SHADER_CACHE_DISABLE=true` (the cache would hand back 128-register builds).

## It works

`INTEL_DEBUG=cs` shows the shaders using r128-r255 (`GRF registers: 256`, against 128 without it —
the allocator is round-robin, so every shader reaches the top half). And everything comes out the
same: the GEMM contract, the staged kernel's partial blocks, and **the daemon's answers, byte for
byte, at 640x360, 1280x720, 1920x1080 at 0.3 and 1920x1080 at full scale**. No hang, nothing in
the kernel log. Lunar Lake honours the mode.

## And it is slower

The mode gives each thread twice the registers and each EU half the threads. Isolated, the kernels
that were short of registers gain a great deal (`src/bench/block_peak.py`, the tiled kernel at a
grid that fills the machine):

| block a subgroup | 128 registers | 256 registers |
| --- | --- | --- |
| 16x32 | 2281 GFLOP/s | 3193 |
| 16x64 | 1289 | **4153** |
| 32x32 | 1338 | 2593 |
| 32x64 | 729 | 2366 |

and the 320x320 frame's GEMMs replayed on their own (`census.py`, `replay.py`) take 27.4 ms on
the 16x32 tiled kernel in the large mode against 34.8 on the staged kernel in the normal one.

But the staged kernel is tuned for sixteen resident workgroups a core, and in the large mode it
loses a sixth: 51 -> 60 us a GEMM, the window blocks 3.2 -> 3.7 ms. On the daemon's path:

| | normal | large, staged | large, tiled where it can |
| --- | --- | --- | --- |
| 640x360 at 0.5 | 25.3 ms | 28.7 | 34.4 |
| 1280x720 at 0.35 | 32.6 | 37.4 | 43.2 |
| 1920x1080 at 0.3 | 48.4 | 55.9 | |
| 1920x1080 at 1.0 | 300 | 378 | 412 |

Routing the GEMMs to the tiled kernel (`XMX_STAGE_K`) is worse still: it has none of the staged
kernel's gather, partial blocks or fused epilogues, and in a frame those were worth more than the
registers. A 64-column block cannot even take the QKV epilogue, which works a 32-channel head at a
time.

## What a win would take

Both of these, neither built:

- **the mode per dispatch**, not per process: anv tracking it in the command buffer and switching
  `STATE_COMPUTE_MODE` only around the dispatches built for it, with whatever stall a switch costs;
- **a staged GEMM built for 256 registers**: a 16x64 block with the staged kernel's loaders and
  fusions. Isolated, the best large-mode block beats the normal-mode staged kernel on big shapes
  by about 6 % (4.15 against ~3.9 TFLOP/s), and at the live sizes the GEMMs wait on latency, where
  half the threads hurts most.

So the ceiling `phase26` described is real, the 256-register side of it is reachable, and on this
graph's shapes it does not pay without a kernel written for it. The patch stays in `src/probe/` as
the way back in.
