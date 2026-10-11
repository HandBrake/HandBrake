# Phase 75 — libd3dmx: the runtime on Direct3D 12, the kernels in HLSL

**2026-09-22, written on the owner's Apple M3 and not yet run anywhere.** The third compute
runtime. `libxmx` drives Vulkan, `libmetalmx` drives Metal (`phase74`), and now
**`libd3dmx`** drives Direct3D 12 on Windows: every `xmx_*` entry point of `xmx.h` in
`src/gpu/libd3dmx.c`, the fourteen kernel names served by nine DXIL modules compiled by dxc
from five HLSL sources under `src/gpu/d3d12/`, each module embedded through bin2c.
**`NR_GPU_BACKEND=d3d12`** is the switch, in `src/nr_build.py` for the Python and in
`nr_frame.c` for the C; CMake builds it under `NR_BUILD_D3D12`, on by default for every Windows
target but 32-bit x86 when dxc is found and refused anywhere else; `NR_DLSSNR_D3D12` follows it
and builds the static archive a host links from it instead of from libxmx (`-DNR_DLSSNR_D3D12=OFF`
for the Vulkan archive beside `libd3dmx.dll`).

What is verified is what a Mac without Windows can verify, and the last section says what
is not. Nobody has executed a single kernel on a Direct3D 12 device.

## What it is

| | libxmx | libmetalmx | libd3dmx |
| --- | --- | --- | --- |
| source | `libxmx.c` | `libmetalmx.m` | `libd3dmx.c` (C11, COM through `lpVtbl`) |
| shaders | GLSL → SPIR-V, bin2c | MSL → metallib, bin2c | HLSL → DXIL (`dxc -T cs_6_2 -enable-16bit-types`), bin2c |
| matrix path | `VK_KHR_cooperative_matrix` | `simdgroup_matrix` | **none**: HLSL has no shipped matrix-matrix operation (the SM 6.8 WaveMatrix preview was withdrawn, SM 6.9's `linalg` is matrix-vector). Every GEMM is the multiply-add kernel; the matrix-path names resolve to it; `xmx_coopmat()` 0, `xmx_portable()` 1 |
| operands | device address in the push block | GPU address in the push block | the resource as a **root UAV** (`SetComputeRootUnorderedAccessView` takes a GPU virtual address, no descriptor heap) and the operand's **byte offset** in the push block's address slot |
| push block | 96 bytes of push constants | `setBytes` | 24 root constants at b0 — the same struct, byte for byte |
| specialization | constant 0 | function constant 0 | **none**: DXIL is compiled ahead of time and there is no compiler at run time; flags come from the push block, `xmx_specialized_count()` is 0 |
| barrier | `vkCmdPipelineBarrier` | `memoryBarrierWithScope` | a UAV barrier with no resource; transition barriers around a copy |
| copy | `vkCmdCopyBuffer` | blit encoder | `CopyBufferRegion` |
| graph | a re-submittable command buffer | an op list re-encoded per run | a closed command list executed as often as wanted, each graph with its own allocator |
| profiling | timestamp queries | counter sampling per encoder | `EndQuery(TIMESTAMP)` per pass, `ResolveQueryData` into a READBACK buffer at the end of the list |
| memory | `HOST_CACHED` on the APU, `DEVICE_LOCAL` unmapped on a card | shared / private storage | a **CUSTOM heap, L0, WRITE_BACK** (cached, mapped) on a cache-coherent UMA device; **DEFAULT** heaps unmapped with UPLOAD / READBACK staging on a discrete card or under `XMX_STAGING=1`; the host's buffers (kinds 1, 2) WRITE_BACK always |
| adopt | a host's Vulkan device | refused | a host's `ID3D12Device` and `ID3D12CommandQueue`, in libxmx's argument list with the Vulkan-only arguments NULL; `nr_frame_adopt_d3d12` in the frame API |
| loading | links the loader / MoltenVK | frameworks | `d3d12.dll` and `dxgi.dll` by `LoadLibrary`: the DLL imports only kernel32 and the C runtime |

The kernels are the GLSL, pass for pass and flag for flag: `gemm_portable.hlsl`
(`gemm_portable`, and `gemm_portable_tiled` with `-DRM=2 -DRN=2`), `gemm_portable_desc.hlsl`
(`gemm_portable_desc`, `gemm_portable_batched` with `-DBATCHED`, `gemm_f16acc` with `-DACC16`),
`resident.hlsl`, `attention.hlsl` (`attention_ab` with `-DSOFTMAX_AB`), `history.hlsl`, and
`nr_d3d.hlsli` for the push block, the four operands and `publish.glsl` ported. `kernel_alias`
in the runtime maps `gemm_resident`, `gemm_tiled`, `gemm_coopmat` and `gemm_batched` onto the
portable modules of the same geometry, and `gemm_staged` onto `gemm_portable` — it has to
build, and it is never dispatched while `xmx_portable()` is 1, as on Vulkan without matrix
units.

## What the port had to get right

Six things, two of them in the arithmetic and four in the API.

1. **HLSL cannot dereference a pointer.** The SPIR-V takes four 64-bit device addresses in
   the push constants and reads through them. HLSL has `RWByteAddressBuffer` and nothing
   else, so an operand travels as two things: its *resource*, bound as a root UAV (u0..u3),
   and its *byte offset* inside that resource, in the very slot of the push block the
   address used to fill, with the runtime folding the element offsets in as libxmx folds them
   into the address. `pc.oa.x` is that offset for `a`; the high word is zero. The GLSL's
   alignment tests on the address (`uint(uint64_t(pc.c)) & 7u`) become tests on the offset,
   which is the same thing because a resource's address is 64 KB aligned. Root descriptors
   were the way in rather than descriptor tables because they need no heap, so a block of
   dispatches records as push-and-dispatch, as it does in the SPIR-V.

2. **A dispatch is at most 65535 groups per axis** (`D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_
   DIMENSION`), and the graph exceeds it: at 1280x768 the C=32 tensors are 31 M elements,
   122 880 groups of 256 for a unary pass, and a GEMM with the pixels on M has 122 880 groups
   of 8 rows. Vulkan on every driver measured so far took the whole count. So `libd3dmx`
   issues a dispatch in pieces — the row groups (`SV_GroupID.y`) of a GEMM, the element groups
   (`.x`) of everything else — and each piece re-pushes the block with its first group in the
   **`spare`** word, which the SPIR-V never used and which every HLSL kernel adds to its group
   id. The descriptor-path kernels take an eighth word for the same purpose. The C struct is
   unchanged; only the shaders give the word a meaning.

3. **The narrow stores go through `f32tof16`.** `half_round` is `f16tof32(f32tof16(x))` —
   the DXIL `LegacyF32ToF16` / `LegacyF16ToF32` pair, round to nearest even with subnormals
   kept, the `packHalf2x16` of the GLSL. Every store into a float16 buffer uses the same
   `f32tof16` and stores the 16 bits (`Store<uint16_t>`), so what lands in a half buffer is by
   construction the value `half_round` would have given, whatever a driver does with an
   `fptrunc`. Loads are `f16tof32(Load<uint16_t>())`, exact. The 16-bit raw loads and stores
   need `-enable-16bit-types`, Shader Model 6.2 and `Native16BitShaderOpsSupported`, which
   `xmx_open` checks and refuses without, as the Vulkan device is refused without
   `shaderFloat16` and `storageBuffer16BitAccess`.

4. **`precise` where the GLSL has it, and `round()` is round-to-nearest-even.** D3D lets a
   driver contract `a * b + c` into an FMA unless the expression is precise, exactly as
   SPIR-V does without `NoContraction`; the gate rounds to half between its multiply and its
   add, so the contraction would move every E4M3 publish downstream (`phase67`, through
   Metal's fast math). The HLSL spells `precise` in the same places, and the disassembly
   confirms it: `resident.dxil` carries 131 `!dx.precise` marks, `history.dxil` 28. HLSL's
   `round` is DXIL `Round_ne` — the E4M3 quantiser needs ties-to-even and the disassembly
   shows 18 of them in the resident kernel. Two HLSL reserved words caught the transcription:
   `linear` (an interpolation modifier; the gate's variable is `affine` now) and `step` (an
   intrinsic; the quantiser's is `quantum`).

5. **Resource states.** Buffers decay to `COMMON` when a command list completes and are
   promoted to whatever their first use in a list needs, so every list here starts from
   `COMMON` (the runtime resets its per-buffer `state` at every `cmd_begin`) and a dispatch
   promotes its operands to UAV for free. A copy after a dispatch is the one case that needs
   explicit transitions — UAV to `COPY_SOURCE` / `COPY_DEST` and back on the next dispatch —
   and `transition()` emits them from the state the recording last left the buffer in. A
   captured graph's transitions are baked at record time and stay right because every
   replay starts from the same decayed `COMMON`. The transfer list and the plain-GEMM list
   never need a barrier at all: one copy, or dispatches only.

6. **Signing.** A DXIL container carries a 16-byte digest after its `DXBC` tag that dxc
   writes only when `dxil.dll` sits beside it. The Windows SDK's and vcpkg's dxc have it; a
   Vulkan SDK dxc on Linux or macOS does not, so a cross build's modules are **unsigned** and
   the runtime rejects them unless developer mode is on and
   `D3D12EnableExperimentalFeatures(D3D12ExperimentalShaderModels)` was called before the
   device existed. `libd3dmx` looks at its embedded modules at `xmx_open`, asks for the
   experimental models when any is unsigned (or `XMX_D3D12_UNSIGNED=1`), and, if a pipeline
   still fails, says in the error that the module was unsigned and what the request returned.
   Build with a signing dxc for anything that leaves the desk.

## What was verified on the M3

- The nine modules compile with the local `dxc` (libdxcompiler 1.9, the Vulkan SDK's build)
  under `-T cs_6_2 -E main -enable-16bit-types -HV 2021 -O3`, no warnings: `gemm_portable`
  5208 B, `gemm_portable_tiled` 12 264, `gemm_portable_desc` 2508, `gemm_portable_batched` 2908,
  `gemm_f16acc` 2628, `resident` 14 540, `attention` 22 176, `attention_ab` 26 184, `history`
  4260. `dxc -dumpbin` shows the four `RWByteAddressBuffer`s at u0..u3, the 16-bit
  `rawBufferStore.i16` with alignment 2, 107 `LegacyF32ToF16` calls in the resident kernel,
  the `Round_ne` and `!dx.precise` counts above, and the `barrier` in the attention kernel.
- `libd3dmx.c` cross-compiles with `x86_64-pc-mingw32-gcc` 16.2 (mingw-w64's `d3d12.h`,
  `dxgi1_6.h`, `d3d12sdklayers.h`) into a 294 KB DLL that exports **all 49** `xmx_*` entry
  points `xmx.h` declares and imports nothing but kernel32 and the UCRT — no `d3d12.dll`,
  no `dxgi.dll`, no `dxguid`: the two DLLs are loaded by name and the IIDs come from
  `initguid.h`. The one thing the headers taught: an aggregate-returning COM method
  (`GetAdapterLuid`, `GetDesc`) takes its result through a pointer in the C binding, so those
  two go through `lpVtbl` directly, which is also what MSVC's C binding does.
- The CMake path: a MinGW cross configure with `-DNR_BUILD_D3D12=ON -DNR_DLSSNR_D3D12=ON`
  compiles the nine modules, runs bin2c, generates `nr_dxil_embedded.c`, and builds
  `libd3dmx.dll`, the Direct3D `libdlssnr.a` and `test_dlssnr.exe` — the archive with no
  Vulkan symbol in it and the test importing no `vulkan-1.dll`.
- `nr_frame.c` still compiles natively; `nr_build.backend()` refuses `d3d12` off Windows and
  anything unknown with a message.

## What is not done

- **One run, and it did not finish** — see "The first run" below for what it said and what
  changed. Before it, nothing had run: no Windows machine, no Wine on the Mac. The first run should be
  `python src\gpu\test_portable.py` with `NR_GPU_BACKEND=d3d12` and `XMX_D3D12_DEBUG=1` (the
  debug layer) — and, if the machine's GPU is in doubt, `XMX_D3D12_WARP=1` puts the whole
  suite on Microsoft's software rasterizer, which supports SM 6.2 with 16-bit types on a
  current Windows. Then `ctest -R d3d12_`. The frame reference for the C test is a file of
  its own (`nr_frame_reference_d3d12.bin`), as the Metal one is.
- **MSVC is written for, not compiled.** `COBJMACROS`, `initguid.h`, `snprintf`, C11
  anonymous unions through assignment rather than designated initializers, `lpVtbl` for the
  aggregate returns; the same standing `phase69` gave the rest of the tree on that toolchain.
- **No matrix path**, for the reason in the table; on a discrete card the multiply-add
  kernels are the whole GEMM, at whatever rate that card gives them. `notes/phase74` measured
  the portable kernel at 480-570 GFLOP/s on an M3 against 1348-3828 for the matrix path on Xe2.
- **No specialization**, and `test_specialization.py` will print `0 pipelines` there. It
  checks the results, not the count, so it passes.
- **VBA-M's Direct3D 12 renderer does not share its device.** `nr_frame_adopt_d3d12` exists
  for it; nothing calls it yet. The Windows libdlssnr *is* the Direct3D 12 archive by default
  on x64 and ARM64 (`NR_DLSSNR_D3D12` follows `NR_BUILD_D3D12`; 32-bit x86 keeps Vulkan), so
  VBA-M's filter there opens libd3dmx's own device and skips the Vulkan panel's share —
  `nr_frame_runtime()` tells it not to try.
- **Memory on a discrete card** follows the `phase63` / `phase65` design — DEFAULT heaps,
  staging copies, host buffers in system memory — and is as unmeasured here as it was there.
- `gemm_runner` and the `half_probe` bench shader have no Direct3D twin, as they have no
  Metal one.

## How to run it

```sh
cmake -S . -B build && cmake --build build          # Windows x64/ARM64: libd3dmx.dll, the nine .dxil and the Direct3D libdlssnr in build/
ctest --test-dir build -R d3d12_
set NR_GPU_BACKEND=d3d12                             # then any Python tool or the C command
python src\ref\nr_frame.py IN.png OUT.png --resident
build\nr_frame.exe IN.png OUT.png -v
cmake -S . -B build -DNR_DLSSNR_D3D12=OFF            # the Vulkan libdlssnr beside libd3dmx.dll, for a Vulkan host
```

`XMX_PORTABLE`, `XMX_STAGING=1`, `XMX_SPECIALIZE` (accepted, no effect), `XMX_TILE_*`,
`XMX_STAGE_K` and the `XMX_*_SPV` overrides (a path to a `.dxil` file) all mean what they mean
on libxmx. New: `XMX_D3D12_DEBUG=1` (the debug layer, its messages drained to stderr after
every submit and on every failure; `=2` adds the informational ones), `XMX_D3D12_WARP=1` (the
software adapter), `XMX_D3D12_ADAPTER=N` (the N-th hardware adapter in performance order),
`XMX_D3D12_UNSIGNED=1` (ask for experimental shader models even with signed modules),
`XMX_DXIL_DIR=/dir` (load `<dir>/<kernel>.dxil` instead of the embedded modules),
`XMX_D3D12_TIMEOUT=N` (give up a fence wait after N seconds; none by default, see below) and
`XMX_D3D12_STEP=1` (run every recorded pass at once on a list of its own, timed and named on
stderr — the way to find the kernel a first run hangs or crawls in).

## The first run (2026-09-22, evening): the graph never came back

The first Windows run, `nr_frame.exe IN.png TEST.png` on an x64 build cross-compiled here with
the Vulkan SDK's dxc (unsigned modules, so developer mode was on over there):

```
resident backend ready in 16.6s (embedded weights)
input 504x618
xmx_graph_run: fence wait timed out after 60 s (258)
```

Three facts in those lines. **Everything up to the graph works**: the device opened, SM 6.2
and 16-bit ops were there, the nine pipelines built, 141 MiB of weights went up through the
transfer list and its fence, and the graph recorded and captured — some 300 passes of root
UAVs, push blocks and split dispatches all validated by the runtime at record time. **The
graph's list ran for 60 s without signalling its fence and the device was not removed**
(`GetDeviceRemovedReason` was S_OK after the wait, or the error would have said so). Windows'
watchdog removes a device whose work stops making progress, so the GPU was most likely still
*working*: on a device without matrix units the multiply-add GEMMs are the whole frame, and a
slow or virtual adapter (16.6 s to build nine pipelines and upload the weights says slow; the
M3 does it in about a second) may need minutes for the first frame, where libxmx on the M3
needs 320 ms. The other reading — a kernel spinning without the watchdog firing (a preemptible
dispatch can) or a root-UAV write off the end of a buffer — is not excluded, and the 60 s wait
could not tell the two apart. **The timeout was the wrong tool**: it turned "slow" into an
error and said nothing about which pass or which adapter.

What changed for the second run, all in `libd3dmx.c`:

- **The fence wait has no timeout by default.** It waits in one-second slices, asks
  `GetDeviceRemovedReason` after each (a hung GPU is Windows' to notice, and the answer arrives
  here the moment it does), and from 10 s on prints to stderr how long it has been standing and
  on which adapter, then every 30 s. `XMX_D3D12_TIMEOUT=N` restores a limit, and its error names
  the adapter and says the device was alive. libxmx keeps its 60 s `vkWaitForFences`; a Vulkan
  driver has no watchdog contract to lean on the same way.
- **`XMX_D3D12_STEP=1` runs the recording pass by pass.** Each `xmx_rec_*` closes its list,
  executes it, waits, prints `pass N <family> flags m n k batch ... groups: T ms` on stderr, and
  recording continues on a fresh list; the lists are parked and the captured graph is all of
  them, executed in order at replay (one `ExecuteCommandLists` each, so the buffers decay to
  COMMON between them as every list's baked transitions assume). The values a step computes
  are not a frame's — the features buffer is empty at record time — but the control flow is,
  so the pass that hangs is the last line printed and the pass that crawls carries its time.
- **The debug layer's messages reach stderr.** The layer reports through `OutputDebugString`,
  which nobody sees without a debugger, so `finish_open` takes the device's `ID3D12InfoQueue`
  when there is one and `drain_messages` prints and clears it after every fence wait, on
  every failure, and at close. Corruption, errors and warnings; `XMX_D3D12_DEBUG=2` adds the
  rest.
- **A real bug: the recording's resource-state tracking was reset by other lists.**
  `cmd_begin` set every buffer's tracked state to COMMON, and it ran for the transfer list too —
  which `stage_copy` uses *during* recording, when the weights arrive — so a recording that had
  left a buffer in UAV or COPY_SOURCE thought it was COMMON again and skipped the transition
  barrier the next use needed. Buffers on hardware mostly survive that (it is what the debug
  layer would have called a state mismatch), but it was wrong. Only `xmx_begin` and a step's
  fresh list reset the tracking now (`states_decayed`).
- `nr_frame.exe` names the adapter in its `resident backend ready` line.

**The second attempt did not get a device.** `test_dlssnr.exe` on another session stopped at
`xmx_open: D3D12CreateDevice (0x887a0004)` — `DXGI_ERROR_UNSUPPORTED` on the adapter DXGI listed
first in performance order — where VBA-M's own Direct3D 12 renderer (`src/wx/panel.cpp`,
`D3D12DrawingPanel`) opens a device on the same machine. The difference was in the selection:
the panel walks `EnumAdapters1`, skips the software ones, *tries* `D3D12CreateDevice` on each
and takes the first that succeeds, falling back to `EnumWarpAdapter` when none does; libd3dmx
took adapter 0 of `EnumAdapterByGpuPreference` untried and gave up on its failure. `xmx_open`
now selects the way the panel does, with the kernels' own requirements folded into the probe:
`pick_adapter` tries every hardware adapter in the list (performance order under DXGI 1.6,
enumeration order before it) with `try_adapter` — a device at 11_0 *and* `check_device` on it,
SM 6.2 and native 16-bit ops — takes the first that passes, and otherwise says on stderr why
each failed and falls back to WARP, whose name gets " (WARP, software)". `XMX_D3D12_ADAPTER=N`
tries only the N-th; `XMX_D3D12_WARP=1` goes to WARP directly; the error when even WARP fails
lists every adapter's reason. `CreateDXGIFactory1` is accepted when `CreateDXGIFactory2` is
missing, as the panel uses it. Not yet seen to succeed either; the next output tells.

The second run, then, is `nr_frame.exe IN.png TEST.png -v` twice: once plain, to see the
adapter's name and whether the wait notes keep coming (slow) or the device is removed (a hang,
with the HRESULT), and once with `XMX_D3D12_STEP=1 XMX_D3D12_DEBUG=1` to get the pass list with
times and any validation message. If the plain run finishes, `network N s` says what the card
does with the multiply-add kernels, and the DEBUG run says whether the recording is clean.

One trap from the session, for anyone driving dxc from a shell: zsh does not word-split an
unquoted `$FLAGS`, so `dxc $FLAGS` hands the whole flag string as one argument and dxc
answers "unable to parse shader model". An array (`FLAGS=(-T cs_6_2 ...)`) or `${=FLAGS}`.
