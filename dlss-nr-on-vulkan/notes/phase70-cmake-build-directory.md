# Phase 70 — the CMake build lands in its own directory

**2026-09-20.** `phase69` made `CMakeLists.txt` build into `work/` "because the Python looks
there and nowhere else". That was the constraint, not a design: `ROOT / "work"` was spelled
out in sixteen files. The owner asked for the binaries and the SPIR-V in the configured CMake
directory, which is what a CMake build is expected to do, so the constraint went instead.

## What changed

- **`src/nr_build.py`** is the one place that knows where a build put things. Resolution:
  `NR_BUILD_DIR` in the environment; else the most recently built of `work/` and `build*/`,
  judged by `libxmx`'s mtime, because nothing runs without it; else `work/`. It exports
  `BUILD_DIR`, `library(name)`, `executable(name)`, `shader(name)` and the platform `SUFFIX`.
  Standard library only, so the single-file tools can share it.
- Every loader, test and bench script that spelled `ROOT / "work"` for an *output* now asks
  `nr_build`: `xmx.py`, `xmxres.py`, `nr_image.py`, `nr_frame_native.py`, `nr_paths.py` (the
  ICD manifest), `prepare_layer.py`, `test_gemm`, `test_layer`, `test_softmax_pack`,
  `test_daemon`, `test_present` (its `layer-check` manifests go beside the library they name),
  `block_peak`, `bank_probe`, `half_probe`, `softmax_pack`. The weights (`work/mlxw/`), the
  MLX-DLSS clone and the PTX modules stay where they are — they are inputs, not outputs.
- **`CMakeLists.txt`**: `NR_OUTPUT_DIR` (default `${CMAKE_BINARY_DIR}`) is where the
  libraries, executables, `.spv` files, `MoltenVK_icd.json` and `nr_frame_reference.bin` go;
  `NR_WORK_DIR` keeps only its input role (the Vulkan-Headers hint). Every ctest runs with
  `NR_BUILD_DIR=<output dir>` in its environment, so a `work/` filled by the Makefile cannot be
  what ctest exercises. `-DNR_OUTPUT_DIR=work` reproduces the old layout.
- **`Makefile`** exports `NR_BUILD_DIR ?= $(CURDIR)/work`, so `make test` runs what `make`
  built even when a CMake build is more recent.
- **`nr_frame` (the C command)** used to find the weights as `<its own dir>/mlxw/`, which is
  true only when it sits in `work/`. It now tries there, then `<its own dir>/../work/mlxw/`,
  then `work/mlxw/` relative to the caller, checking each is readable.
- A `.gitignore` with `build*/`: the parent repository's `/*build*/*` is anchored at *its* root
  and never covered this tree.

## What was checked

`cmake -S . -B build -G Ninja && cmake --build build` on the M3: 37 steps, every library,
executable and shader in `build/`, `work/` untouched. `python3 src/nr_build.py` answers
`build` afterwards and `work` under `NR_BUILD_DIR=work`. The ctest subset that exercises the
new paths — `settled`, `test_present`, `test_daemon`, `test_portable`, `test_toggle`,
`test_panel`, `claims_check`, `test_nr_frame_py` + `nr_frame_c`, `test_epilogue`,
`test_native_image` — is recorded in the HANDOFF entry with its result.

## The weights directory (later the same day)

The embedding scheme this tree already carried ran `slice` and `bin2c` over
`dlssnr-logical.safetensors` at build time and compiled the result from the build tree. The
owner wants the opposite default: **`weights/` in the source tree is the pre-processed form**
— 35 headers of 8 MB byte arrays, a translation unit each with a `sizeof` check, and the
table `nr_weights_embedded.c` — and the build compiles that directory (`NR_WEIGHTS_DIR`).
`NR_BIN2C_WEIGHTS`, default **OFF**, regenerates the directory from the safetensors — found
in the source root, `work/mlxw/`, or `NR_WEIGHTS_FILE` — with the `.bin` slices and `.c.in`
templates in the build tree and the `.h`, `.c` and table written into the directory, stale
slices past the new count removed. Without the file it prints one line and uses the
directory as it is. The host tools are only built when regenerating or embedding shaders.

Checked: all three configurations (default, regenerate into a scratch directory, regenerate
with the file absent) configure as intended; slices 0 and 34 and the table regenerated
through `slice` + `bin2c` are byte-identical to the checked-in ones below the comment line.

## Trap

**Two builds of the same names in two places.** Without `NR_BUILD_DIR` the newest wins, and
"newest" is `libxmx`'s mtime, not the shader's: rebuild a shader alone in the older tree and
the Python keeps reading the other one. When a result looks stale, `python3 src/nr_build.py`
says which directory is live, and the configure step prints the export line.
