# Phase 69 — one build for Linux, macOS and Windows (2026-09-19)

## What was asked

A `CMakeLists.txt` that configures on Windows, macOS and Linux.

## What exists now

`CMakeLists.txt` at the root: the Makefile's targets described once for CMake. Every
output still lands in `work/` — libraries, executables, SPIR-V — because that is where the
Python side looks, so the two builds are interchangeable on a machine that has both.

- **Dependencies found, not assumed.** `find_package(Vulkan)` for the SDK or the
  distribution's headers and loader, with the pinned `work/vulkan-headers` clone as the
  fallback; `libMoltenVK.dylib` on macOS for the compute runtime (`NR_XMX_LINK_MOLTENVK`,
  on by default there), with the ICD manifest written at configure time for the layer
  tests; `glslangValidator` from the SDK or the path; libpng for the `nr_frame` command,
  which is simply not built without it; Python for the ctest registrations.
- **Names the Python expects.** `libxmx`, `libnr_image`, `libnr_frame`, `libnr_layer`
  with the platform's suffix — `.so`, `.dylib`, `.dll` — the `lib` prefix kept on Windows,
  no per-configuration subdirectory under a multi-config generator, and
  `WINDOWS_EXPORT_ALL_SYMBOLS` so a DLL exports what a `.so` exports without decorating
  every function.
- **The same flags as the Makefile**: no fused multiply-add and no fast maths on the host
  code (`-ffp-contract=off -fno-fast-math`; `/fp:precise` on MSVC), `-march=native` on the
  image passes where the compiler has it.
- **ctest runs what `make test` runs**, the Python side of the C frame test before the C
  side that reads its reference, `XMX_PORTABLE=1` as its own test, and `VK_DRIVER_FILES`
  set for the loader-based tests on macOS.
- **The layer is POSIX** — Unix sockets, pthreads — and `NR_BUILD_LAYER` defaults off on
  Windows. That is the one part of the tree with no Windows story yet.

## What the sources needed

`src/ref/nr_portable.h`: the few things C11 spells differently across the three. A
thread-local, `strdup` / `strtok_r`, a clock, a temp directory, dynamic loading (`dlopen`
or `LoadLibrary`, `dladdr` or `GetModuleHandleEx`), `setenv`, and **half-precision
conversion**, which is the one that matters. GCC and Clang have `_Float16`; MSVC does not,
so the header carries a software round-to-nearest-even conversion for it. Checked
exhaustively on this machine against the type, not by tests alone:

| direction | values | differing |
| --- | --- | --- |
| half -> float | all 65 536 | 0 |
| float -> half | all 4 294 967 296 | 0 (16.7 M NaN payloads differ; both NaN) |

`nr_image.c` and the C frame test go through the header. `nr_frame.c` and
`nr_frame_main.c` include it for the half conversion and the clock, and carry their own
`#ifdef _WIN32` blocks for loading and formatting — the owner was porting those two files
by hand while this was written, and that work was left as found rather than folded in.
Two things in it a Windows build would have met: `GetModuleHandleA` on `libxmx.dll` returns a
handle only if the DLL is already loaded, where `LoadLibraryA` (in the header as
`nr_dl_open`) loads it; and `GetCurrentDirectoryA` gives the working directory rather than
the library's own, which the shader and weight paths are relative to. **Folded in the same
evening** (below): those blocks now go through `nr_dl_open`, `nr_dl_sym`, `nr_dl_self_dir`
and `nr_now`, and the `#ifdef _WIN32` copies of them are gone.

## What was measured

On the M3, from a clean cache: configure, build, 25 tests registered; the layer and frame
tests through ctest pass (8 of 8); the whole suite passes but `publish_check`, for the
repository reason `phase68` records. The CMake outputs are the same names in the same
place as the Makefile's, and the Makefile still builds after the header went in.

**Windows: built and linked, not run** (2026-09-19, late evening). A MinGW-w64 cross-build
from the M3 — `x86_64-pc-mingw32-gcc` 16.2 with mingw-w64 13, `-DCMAKE_SYSTEM_NAME=Windows`,
libpng, zlib and the `vulkan-1` import library under `/usr/local/x86_64-pc-mingw32` — now
produces `libxmx.dll`, `libnr_image.dll`, `libnr_frame.dll`, `nr_frame.exe`,
`test_nr_frame.exe`, `gemm_runner.exe` and the sixteen `.spv`, all in `work/`. The frame DLL
exports its fourteen `nr_frame_*` entry points without a `.def` file (GNU ld exports
everything when nothing is decorated, as `WINDOWS_EXPORT_ALL_SYMBOLS` does for MSVC); the
executables import it and the UCRT; the runtime imports `vulkan-1.dll`. There is no Wine on
the Mac, so none of it has executed. The first configure of that tree failed three ways, and
the fixes are the section's real content:

- **`-march=native` is not a flag a cross-compiler accepts** — `cc1: error: bad value
  'native'`, because there is no native to ask about. `CMakeLists.txt` now runs
  `check_c_compiler_flag` and passes it only where it works; the Makefile is unchanged.
  Tests default off under `CMAKE_CROSSCOMPILING`, since the executables cannot run.
- **`nr_frame_main.c` included `dlfcn.h` unconditionally**, and its Windows block defined a
  `clock_gettime(int, struct timeval *)` that MinGW's `time.h` already declares with the
  real signature. Both gone: the command uses `nr_now()` like the library, and finds its
  default weights through `nr_dl_self_dir` on every platform — the old Windows branch
  looked in the working directory for a file that lives in `work/mlxw/`.
- **The library's Windows `BIND` never compiled** (`GetProcAddress(X.handle, symbol}` — a
  brace for a parenthesis) and would have assigned a `FARPROC` without a cast. `struct xmx`
  now holds an `nr_dl`, and the load, the symbol lookup and the own-directory search are
  the header's on every platform. The Unix behaviour is unchanged: `test_nr_frame_c.py`
  passes all 25 checks on the M3 afterwards, head still bit-identical, and the native
  `nr_frame` renders a frame from its default weights path.

Also learned: MinGW defines `__STDC_WANT_SECURE_LIB__` as **0**, so every
`sprintf_s` / `fopen_s` block in the two files takes its `snprintf` branch there. Under MSVC
they would not, and `sprintf_s` does not truncate — it calls the invalid-parameter handler
and aborts — so `FAILF("cannot load %s: %s", path, ...)` with a 1200-byte path into a
512-byte `last_error` is a crash on MSVC where it is a clipped message everywhere else.
Left as it is, flagged here. The MSVC path stays configured and unmeasured; whoever runs it
should read `phase63` first — a discrete card takes a different memory path, and this
project has never had one.

## Traps

- **`option()` takes a value, not a generator expression.** `$<NOT:$<BOOL:${WIN32}>>` as
  a default read as OFF everywhere; the layer silently was not built until the message
  line said so. Compute the default in an `if()`.
- **A cached option survives a reconfigure.** The fix above changed nothing until the
  build directory was deleted.
- **`unistd.h` hides `struct stat`.** Removing the POSIX includes from the C test broke an
  `exists()` helper that had been getting `sys/stat.h` for free; `fopen` is the portable
  spelling.
