# Phase 72 — the tree builds for Android

**2026-09-21.** The owner asked for dlss-nr-on-vulkan to be built on Android too. VBA-M's Qt
port already ships an Android APK (`tools/android/build-android-qt.sh`, Qt for Android from
vcpkg, the NDK's own toolchain file) with `ENABLE_VULKAN=ON`, and its Vulkan panel already
carries the DLSS NR share code (`renderers/vulkan-panel.cpp`) with no Android gate — so the
question was only whether this tree's CMake build survives the NDK, and it very nearly did.

## What happened on the first configure

Nothing failed. `find_package(Vulkan)` finds the NDK sysroot's `libvulkan.so` and headers on
its own; VBA-M's `HostCompile` module builds `bin2c` and `slice` for the Mac; glslangValidator
comes from the host vcpkg triplet (`VBAM_VCPKG_HOST_PREFIX`); libpng is vcpkg's static
`arm64-android` one, so even the `nr_frame` command is built. The pre-processed weights in
`weights/` compile with the NDK's clang 21 under the same two-job pool.

The first *build* had one failure and one worry:

- **`nr_layer.c` defined `VK_USE_PLATFORM_XLIB_KHR` under `__linux__`**, and Android is
  `__linux__` without X11 — `fatal error: 'X11/Xlib.h' file not found` from vcpkg's
  `vulkan.h`. Guard is now `__linux__ && !__ANDROID__`; the layer and its two direct-include
  tests then compile for arm64. The layer's *default* is off on Android anyway
  (`NR_LAYER_DEFAULT`): there is no game to hook and no daemon.
- **VBA-M compiles its whole Release build with `-ffast-math`** (`cmake/Toolchain-gcc-clang.cmake`),
  and that reaches these targets, whose contract is NumPy's float32 operation order. Every
  file printed `clang: warning: overriding '-ffast-math' option with '-ffp-contract=off'`.
  Checked on the NDK's clang, not assumed: `-O2 -ffast-math -ffp-contract=off -fno-fast-math`
  on `a*b+c` and `(a+b)+c-a` gives `fmul; fadd` and three separate adds — no `fmadd`, no
  reassociation — the same code as `-O2` alone with contraction off. So the exact flags,
  which come later on the line, win; the warning was noise and is now silenced on clang
  (`-Wno-overriding-option`). This applies to the desktop VBA-M builds as well, which have
  been compiling `libdlssnr` this way since the archive was added.

Two more small things: `nr_temp_dir()` returned `/tmp`, which Android does not have, and now
returns `/data/local/tmp` there (only `test_nr_frame`'s synthetic weights use it); and a
**standalone** cross build had no way to get `bin2c` and `slice` without naming host copies,
so the CMake now does what VBA-M's module does — a C compiler from the build machine's
`PATH`, refusing the cross toolchain's own directory (`NR_HOST_CC`).

## What was built

| | arm64-v8a, inside VBA-M | armeabi-v7a, standalone |
| --- | --- | --- |
| `libdlssnr.a` | 292 598 964 B | 292 546 480 B |
| `test_dlssnr` (PIE, `linker64`, needs `libvulkan.so libdl.so libm.so libc.so`) | 292 576 112 B | 292 363 352 B |
| `libxmx.so` / `libnr_frame.so` / `libnr_image.so` | 528 KB / 292 MB / 47 KB | built |
| `gemm_runner`, `test_nr_frame`, the 15 `.spv` | built | built |
| `nr_frame` command | built (vcpkg libpng) | not built (no libpng) |
| `libnr_layer.so`, `test_settled`, `test_exchange` | compile when forced on | off |

And the whole thing end to end: `build-android-qt.sh`'s `apk` target links
`vbam-components-filters-dlssnr` and `libdlssnr.a` into the Qt app module, so the APK
carries the model:

| | |
| --- | --- |
| `visualboyadvance-m-qt-ARM64.apk` | 398 252 777 B |
| `lib/arm64-v8a/libvisualboyadvance-m-qt_arm64-v8a.so` inside it | 321 411 488 B, 67 `xmx_*` / `nr_frame_*` dynamic symbols |
| next largest, `libQt6Core` | 42 MB |

The weights are the APK. Before this the module was a few tens of MB.

## What is not claimed

*(Written before any device run. `phase73-mali.md` has the first one: a Mali-G57 needed one
shader fix, the subgroup barrier in `attention.comp`, and then passed all 34 checks.)*

**Nothing has run on a device.** There is no Android device attached to this build, and the
owner's phone holds their signed copy that must not be replaced (`memory: reference_android_device_testing`).
What a device will need, and what is unknown:

- The runtime asks for a **Vulkan 1.3** device with `shaderFloat16`, `storageBuffer16BitAccess`,
  `bufferDeviceAddress`, `vulkanMemoryModel` (+DeviceScope) and `scalarBlockLayout`
  (`libxmx.c`, `xmx_open`); the Qt panel enables the same set when it lends its device. Recent
  Adreno and Mali drivers advertise 1.3; older phones will fail `xmx_open` and the filter
  falls back to `kNone` as it does on any failure.
- Without `VK_KHR_cooperative_matrix` — the normal case on a phone — every GEMM runs on
  `gemm_portable*.comp`, the path MoltenVK takes. The shaders are `local_size_x = 32` and
  the resident kernels use `subgroupBarrier()`; they were measured with a subgroup of 32
  (Xe2, Apple). A 64-wide Adreno wave or a 16-wide Mali subgroup is untested. `phase67`
  says what the portable contract is; whether a mobile driver honours it is a measurement
  nobody has made.
- Memory: at the filter's ~320x320 network extent the device buffers are small; the model
  itself is 292 MB of weights mapped from the APK's `.so`, plus the FP16 upload.
- Speed: the M3 does ~230-320 ms a pass through the portable GEMM at 480-570 GFLOP/s. A phone
  GPU is several times slower; expect one pass a second or worse. The filter is asynchronous
  (latest frame wins), so the emulator's own frame rate is unaffected.
- The APK grows by the embedded weights. `-DNR_EMBED_WEIGHTS=OFF` builds an archive whose
  `nr_frame_open(NULL)` refuses, and the filter has no path-loading route, so on Android the
  embedded form is the only one that works today.

---

## riscv64: `_Float16` is not usable there (2026-09-22)

**The riscv64 APK nightly could not compile the tree.** `nr_image.c` killed the compiler
outright:

```
fatal error: error in backend: Scalarization of scalable vectors is not supported.
3.	Running pass 'Function Pass Manager'
4.	Running pass 'RISC-V DAG->DAG Pattern Instruction Selection' on function '@nr_compose'
```

It surfaced as a **link** failure — VBA-M's Release build is `-flto=thin`, so codegen runs
inside `ld.lld` — which makes it look like an LTO bug. It is not. `clang -O3 -c
nr_image.c` alone reproduces it exactly, and `test_nr_frame.c` dies the same way;
`nr_frame.c` happens to survive, its half calls not being in a loop the vectorizer takes.

**Cause.** Clang defines `__FLT16_MANT_DIG__` for every riscv64 target, so
`nr_portable.h` took the `_Float16` path. Android's riscv64 baseline
(`riscv64-none-linux-android35`) is:

```
__riscv_v 1000000   __riscv_vector 1   __riscv_v_min_vlen 128   __riscv_zba/zbb/zbs
no __riscv_zfh                          no __riscv_zvfh
```

The V extension, and **no half-precision at all** — not in a scalar FP register, not in a
vector. The loop vectorizer still turns the conversions in `nr_compose` into
`<vscale x N x half>` `fptrunc`/`fpext`, the backend can only lower those by scalarizing,
and scalarizing a *scalable* vector is unimplemented. Hence the abort rather than a
diagnostic. (`-fno-vectorize` also silences it, which is what confirmed the mechanism.)

**Fix**, in `nr_portable.h`: take the software conversion whenever the target has vectors
that cannot hold a half.

```c
#if defined(__riscv_vector) && !defined(__riscv_zvfh) && !defined(NR_NO_FLOAT16)
#define NR_NO_FLOAT16 1
#endif
```

Precise on purpose. A riscv64 without V never builds a scalable vector and keeps the type;
one with Zvfh vectorizes it properly and keeps it; only V-without-Zvfh falls back. arm64
and x86_64 are untouched — checked by preprocessing the header for all three Android
triples.

**It costs that target nothing.** Without Zfh the `_Float16` conversions were already
libcalls there; the fallback is integer bit-twiddling the vectorizer can actually take.

**Byte-identity re-verified on this machine**, both directions, both paths compiled from
the same header — the `phase69` table reproduced exactly:

| direction | values | differing |
| --- | --- | --- |
| half -> float | all 65 536 | 0 |
| float -> half | all 4 294 967 296 | 0 (16 744 448 NaN payloads differ; both NaN) |

The NaN payloads cannot reach an output byte: `nr_encode8` maps NaN to zero before the
cast, which is the NumPy behaviour it transcribes.

**Still run on nothing.** The whole tree now builds for `riscv64` (`libnr_image.so`,
`libxmx.so`, `libnr_frame.so`, `libdlssnr.a`), and no riscv64 device has executed a byte
of it. Everything under "What is not claimed" above applies here too, with a device class
even rarer than the phones it was written about.
