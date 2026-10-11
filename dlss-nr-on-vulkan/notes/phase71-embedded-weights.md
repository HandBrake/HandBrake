# Phase 71 — the weights compiled into the library

**2026-09-20.** The owner asked for `dlssnr-logical.safetensors` to be turned into a C header
with `bin2c`, `bin2c` built by CMake, and the C side to load the weights from that header
rather than from the file. `src/tools/bin2c.c` is Rafael Kitover's (BSD-2), the tool the
VBA-M wxWidgets build embeds its resources with, and it is used unchanged.

## The one number that shaped it

`bin2c` writes a file as `const unsigned char NAME[] = { 0x.., ... }` — 5.06 bytes of C per
byte of input, so 1.47 GB of source for the 292 MB file. The source size is not the problem.
**A compiler holds a braced initializer one element at a time**, and measured on Apple
clang 21 on the M3, a 16 MB slice costs:

| | |
| --- | --- |
| bin2c | 1.3 s |
| header | 84.9 MB |
| clang `-O2 -c` | 5.9 s |
| peak memory | **1.44 GB**, ~90 bytes per byte of input |

The whole file in one array would want ~25 GB. This machine has 8 GB. So the file is embedded
as **slices**: `src/tools/slice.c` (25 lines, no `fseek`, so a 32-bit `long` on Windows is
never asked for a file position) cuts it into `NR_EMBED_CHUNK_MB` pieces (8 MB, 35 of them),
`bin2c` writes one header per slice, and each header is compiled as its own translation
unit — `build/weights/dlssnr_logical_safetensors_<k>.c`, generated at configure time, which
includes the header and carries a `typedef char check[sizeof array == N ? 1 : -1]` so the
array is the length the table was written with. A Ninja job pool (`NR_EMBED_JOBS`, 2)
bounds how many compile at once: a `-j8` build of 35 such units would otherwise ask for 6 GB.

## What changed

- **`CMakeLists.txt`**: `NR_WEIGHTS_FILE` (default: the source root, then `work/mlxw/`),
  `NR_EMBED_WEIGHTS` (on when the file exists), `NR_EMBED_CHUNK_MB`, `NR_EMBED_JOBS`.
  `bin2c` and `slice` are host tools built into `build/tools/`; a cross-compile names host
  copies in `NR_BIN2C` / `NR_SLICE` or embeds nothing, with a warning. The slice objects form
  the OBJECT library `nr_weights_blob` and go into `libnr_frame`, which is then compiled with
  `NR_EMBEDDED_WEIGHTS=1`. A generated `nr_weights_embedded.c` lists the slices in order:
  `nr_embedded_weights_chunks[]`, their count and the total, declared by the hand-written
  `src/ref/nr_weights_embedded.h`. A change to the weights file regenerates everything
  after it.
- **`src/ref/nr_frame.c`**: the safetensors reader used to be `fopen`/`fseek`/`fread`; it
  now reads through `struct source`, which is either a `FILE *` or the chunk table, and
  `source_read(offset, n)` walks the slices. The reader was already range-based — it
  converts one tensor at a time to float32 — so nothing else moved. `nr_frame_open(NULL)`
  opens the embedded weights, or fails with a message that names the build when there are
  none; `nr_frame_embedded_weights_size()` says which.
- **`nr_frame` (the command)** without `--weights` uses the embedded weights when the
  library has them, else the file search from `phase70`; it prints which. **`test_nr_frame`**
  runs on the embedded weights first, the real file second, random weights third, with
  `NR_TEST_SYNTHETIC=1` still forcing the last; and it checks the NULL path both ways.
  **`NativeFrame()`** with no argument does the same from Python.
- `*.safetensors` joined `.gitignore`. The generated C stays in `build/`, so the rule that no
  weights are committed holds: the header is a build product of the owner's file, and the
  library it goes into is as private as the file was.
- `NOTICE` names bin2c. The Makefile is untouched and embeds nothing: a Makefile-built
  `libnr_frame` refuses `nr_frame_open(NULL)` with the message above.

## What was checked

On the M3 (8 GB), `cmake -S . -B build && cmake --build build`:

| | |
| --- | --- |
| slices | 35 of 8 MB, the last 6 363 978 bytes; total 291 576 650 = the file |
| whole build, from clean objects | **59.8 s** wall, 81 s user |
| peak memory of the build | **0.89 GB** |
| warnings | none |
| `libnr_frame.dylib` | 103 KB -> **294 MB** |

- `build/test_nr_frame` with no argument prints `embedded weights: 291576650 bytes` and passes
  its **34 checks**, including the new one that this build carries the weights; the graph
  runs on them (head mean -0.70, sd 1.20, replay byte-identical, two extents).
- **Embedded against the file, same bytes**: `NativeFrame(None)` and `NativeFrame(path)` on
  the same 512x288 random frame give head `bb94971d…` and output `55c3638e…` both ways.
- Under ctest, `test_nr_frame_py` and therefore `nr_frame_c --reference` fail on this
  checkout for a reason that predates the change and is not the reader: there is no
  `work/mlx-dlss` clone here, the Python test stops at its import, and the C test's one
  failing check is the reference file that was never written. Run directly, the C test skips
  that check and says so.
- The Makefile build was not rebuilt; it does not include the new sources and the reader's
  `#ifdef` leaves it as it was.

## The shaders too (later the same day)

The owner then asked for the `.spv` modules embedded and loaded from the embedded data.
Same tool, no slicing — the fifteen modules are 10-40 KB each — and one table:

- `nr_shader()` in `CMakeLists.txt` now also runs bin2c on every module it compiles,
  `build/shaders/spv_<name>.h`, and a generated `nr_shaders_embedded.c` includes them all
  and lists `{ "resident.spv", spv_resident, sizeof spv_resident }` for each. That one object
  is linked into **libxmx and gemm_runner**, both compiled with `NR_EMBEDDED_SHADERS=1`;
  `src/gpu/nr_shaders_embedded.h` declares the table. `NR_EMBED_SHADERS` (on) controls it,
  and the host-tool block moved above the shaders section so both embeddings share it. The
  targets are `bin2c_dlss` and `slice_dlss` because VBA-M adds this tree as a subdirectory
  and has a `bin2c` target of its own.
- **libxmx resolves a shader by name.** `shader_code()` in `libxmx.c`: a bare name is the
  embedded module; a path with a directory opens that file, so the `XMX_*_SPV` overrides
  still measure an experiment's build; a path whose file is missing falls back to the
  embedded module of the same base name. Every `build_pipeline_spec` call, the specialized
  rebuilds included, goes through it. `xmx_embedded_shader(name)` returns the size or 0.
  `gemm_runner` applies the same rule to its first argument.
- **The callers hand over bare names when the runtime carries the module.** `nr_frame.c`
  binds `xmx_embedded_shader` as an optional symbol (a Makefile-built libxmx has none) and
  its `spv()` returns the name in that case; `nr_build.shader_arg(lib, name)` is the same
  decision for the Python, used by `xmx.py` and `xmxres.py`. The `.spv` files are still
  written, because the benches and `test_layer` read some directly.

Checked with **every `.spv` moved out of `build/`**: `test_nr_frame` 34/34, the head on the
embedded weights `bb94971d…` as before, `test_gemm.py` worst relative error 8.303e-07 both
with and without the files (it hands the runner a path, which is the fallback case), and
`gemm_runner gemm_coopmat.spv` reaching the device by bare name. libxmx grew 305 KB.

## What not to do

- **Do not raise `NR_EMBED_CHUNK_MB` to make fewer files.** The per-slice memory is linear in
  the slice; 32 MB slices are 2.9 GB each and two of them do not fit beside a daemon holding
  the model. The 8 MB default is chosen for this 8 GB machine, not for elegance.
- **Do not put the table's sizes in the chunk units as `const size_t` symbols** and read them
  from the table: a static initializer cannot hold another translation unit's value, only
  its address. The sizes are configure-time numbers and the typedef check in each unit is
  what makes that safe.
- **Do not make a bare name look for a file first.** The order — embedded for a name, file
  for a path, embedded again when the path's file is gone — is what lets a build directory
  with no `.spv` in it run, and what keeps an `XMX_ROW_SPV=/tmp/variant.spv` experiment
  honest. A name that tried the working directory first would load whatever an old build
  left there.
- The Python graph (`nr_frame.py --resident`, the daemon) still reads the safetensors file:
  it is NumPy on the logical tensors, and the C library does not export them. Embedding
  covers the C side, which is what was asked.
