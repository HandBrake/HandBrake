# Phase 73 — the first device: a Mali-G57 runs the graph

**2026-09-21.** The owner sideloaded the wx APK from `phase72` onto their Samsung SM-A165F
(MediaTek MT6789, Mali-G57 MC2, Android 16, Vulkan 1.3.303, driver r48-era) and selected the
DLSS NR filter. The app showed "DLSS NR: loading model..." and died. This note is what killed
it, how it was found without touching the phone's installed copy, and what the device measures.

## The crash

Every tombstone was the same: `SIGABRT` from inside the vendor driver, on the filter's worker
thread, with libxmx on the stack —

```
#01 libGLES_mali.so
...
#15 libGLES_mali.so (cmpbe_v2_compile_multiple_shaders+8916)
#19 build_pipeline_spec        libxmx.c:416   vkCreateComputePipelines
#20 xmx_res_init               libxmx.c:966   build_pipeline(row_spv, ...)
#21 nr_frame_open              nr_frame.c:1955
#22 dlssnr::Filter::Impl::Run  dlssnr.cpp:215
```

The unstripped `libvisualboyadvance-m_arm64-v8a.so` in the build directory carries the same
build id as the APK's, so `llvm-symbolizer` resolves the frames; line 966 is the third pipeline
of the six `xmx_res_init` builds, the row pass `attention.spv`. The GEMM and unary pipelines
before it compiled. Mali's shader compiler does not return `VK_ERROR_*` for what it cannot
handle: it calls `abort()`, so no error path in libxmx or in the filter sees it.

## Reproduced without the app

The phone's installed APK is signed with a key this machine does not have, so `adb install`
is out (`memory: reference_android_device_testing`). The standalone test binary is not:
`test_dlssnr` (the C test linked against `libdlssnr.a`, weights and shaders embedded) is a
plain PIE that needs only `libvulkan.so`, and `/data/local/tmp` runs it —

```
adb push test_dlssnr *.spv /data/local/tmp/nr/
adb shell "cd /data/local/tmp/nr && ./test_dlssnr"
  embedded weights: 291576650 bytes
  LLVM ERROR: Cannot select: intrinsic %llvm.bifrost.2556
```

The same abort, with the message the app never got to print: the Bifrost/Valhall backend of
the Mali compiler has no lowering for one intrinsic the shader produced. And because
`nr_frame.c` honours `XMX_ROW_SPV` (and the other five `XMX_*_SPV`) as a file-path override
over the embedded module, a candidate shader can be compiled on the Mac, pushed, and tried in
under a minute without relinking anything.

## Which instruction

The opcodes of `attention.spv` that appear in neither `gemm_portable.spv` nor `resident.spv`
(both compiled) are `OpControlBarrier`, `OpMemoryBarrier`, and five arithmetic/compare
opcodes no compiler has trouble with. The barriers were `subgroupBarrier()` and
`subgroupMemoryBarrierShared()` fencing the shared-memory row staging —
`OpControlBarrier %Subgroup %Subgroup ...`. No other shader on the portable path has a
subgroup-scope control barrier (the only other user is `gemm_resident.comp`, the cooperative-
matrix path a phone never loads).

Replacing them with `barrier()` / `memoryBarrierShared()` — workgroup scope — and
pointing `XMX_ROW_SPV` at the result:

```
C library on Mali-G57 MC2: portable multiply-add (the device has no VK_KHR_cooperative_matrix)
  [ok  ] head: finite
  [ok  ] head: the captured graph replays the same bytes
  [ok  ] head: the split accounts for the run   write 1.2 ms, graph 6713.0 ms, read 0.0 ms
  ...
nr_frame in C: 34 checks, the library against its own contract
```

All 34 checks pass. That is the fix now in `attention.comp`.

It was the right barrier all along, not just the compilable one. The workgroup is 32 wide
and the shader was measured where the subgroup is too (Xe2, Apple, MoltenVK), so
"subgroup" and "workgroup" named the same 32 invocations and the cheaper spelling was
chosen. Mali's subgroup is **16** (`subgroupSize 16`, `minSubgroupSize = maxSubgroupSize =
16` from `VK_EXT_subgroup_size_control`): a subgroup barrier there fences half the rows, and
the other half's `stage[]` reads would have raced the gathers even if the compiler had
accepted it. `barrier()` on a workgroup that is one subgroup lowers to the same thing on the
drivers this tree measured, so the desktop numbers do not move — though nobody re-measured
them today, and there is no MoltenVK or Xe2 build on this machine to do it with.

`gemm_resident.comp` keeps its `subgroupBarrier()`: its cooperative-matrix operations are
`gl_ScopeSubgroup` and the shader assumes workgroup == subgroup throughout, so on a
16-wide device with `VK_KHR_cooperative_matrix` (Mali-G715 and later) the barrier would be the
least of its problems. That shader needs a subgroup-size audit, not a one-line change.

## What the device is

From `adb shell cmd gpu vkjson`, the properties libxmx needs:

| | Mali-G57 MC2 |
| --- | --- |
| apiVersion | 1.3.303 |
| subgroupSize | 16, not controllable (min = max = 16) |
| maxComputeSharedMemorySize | 32 KB |
| maxComputeWorkGroupInvocations | 512 |
| shaderFloat16 / shaderInt8 / shaderInt16 / shaderInt64 | yes |
| storageBuffer16BitAccess, bufferDeviceAddress, vulkanMemoryModel (+DeviceScope), scalarBlockLayout | yes |
| VK_KHR_cooperative_matrix | no — portable GEMM |
| shaderFloat64 | no (unused) |

## What it costs

One 320x320 pass: **6.7 s** on the graph (`head: the split accounts for the run`), against
230–320 ms on the M3 through MoltenVK. The filter is asynchronous and latest-frame-wins, so
the emulator keeps its 60 fps and the filtered picture refreshes about every seven seconds.
That is the portable GEMM on a two-core G57 at 16 lanes; the tiled variant and staging knobs
(`XMX_TILE_*`, `XMX_STAGE_K`) have not been tried on it. Loading the model — mapping 292 MB
of weights out of the APK's `.so` and uploading them as FP16 — is not separately timed here.

## Traps

- **A Mali compile failure is an abort, not an error.** Any new SPIR-V construct on the
  portable path has to be tried on a Mali before it ships; the filter's fallback-to-`kNone`
  never runs. `test_dlssnr` under `adb shell` is the way to try it.
- **`subgroupBarrier()` is not a cheaper `barrier()`.** It is a different contract, and the
  32-wide workgroup only happened to satisfy it on 32-lane hardware.
- **`llvm-symbolizer` wants the build-directory `.so`**, not the one under `android-build/`,
  and the tombstone's `pc` is already relative to the `.so` start inside `base.apk`.
