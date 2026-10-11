# A release builder for the integrated Windows runtime

2026-10-05. `improve-release` starts at improve `6d23efb` and replays PR #5
through its original `67b9e03`. The contributor's authorship, author dates and
GitHub noreply author addresses are retained. Four original committer records were
not noreply, so the five commits were cherry-picked with our noreply committer;
the contributor's branch is untouched. The docs conflict keeps both the actual
Windows game results and the shipping section. Master has not changed.

## Result

Both `tools/deploy.sh --release` and `tools/deploy.bat --release` call the same
`scripts/build_release.py`. It requires a complete build before assembling:

- The platform's layer, resident runtime and native image library; Windows also
  requires **libnr_alloc.dll**, previously missing from PR #5's copy list.
- Every shader named by the runtime sources, plus half_probe.spv.
- MLX-DLSS runtime modules and both weight tools, its license, our source modules,
  setup scripts, manifest template, get_weights.py, LICENSE, NOTICE and a user guide.

A release must go into a new or empty directory. It is assembled in a temporary
directory and promoted only after copying succeeds. A missing dependency or an
occupied destination returns an error without replacing the user's files. Copies
omit local weights, vendor binaries, build junk and the upstream Git checkout.

The Windows game-deploy path also copies libnr_alloc.dll. The first Windows release
route is explicitly **x64**; source instructions still cover the optional x86 layer.
Deploy and setup scripts honour NR_PYTHON where Python is selected.

Release setup writes launchers with release-local settings, log and trigger paths.
The initial settings use render scale 0.4 and min_extent 320. Processing starts off
until the trigger is created. Linux launchers quote paths containing spaces,
parentheses, apostrophes and dollar signs. The copied setup.sh is executable.

The ordinary Linux release build recompiles libnr_image without -march=native.
Local make builds retain their existing native default through NR_IMAGE_ARCH;
--skip-build means the maintainer supplies compatible binaries. This addresses CPU
instruction portability, not compatibility with every distribution's shared libraries.

## Evidence

- Twelve release tests pass, including missing allocator/shaders/modules, failed
  copying, an occupied destination left untouched, Windows/Linux contents, first-time
  Linux installation, dry-run, a selected Python interpreter and launcher paths.
  Windows payload tests use fixture DLLs on Linux; they do not execute those DLLs.
- bash syntax and build-system parity pass. The new test is registered in both
  make test and CTest; the focused CTest release/build/claims checks pass.
- An actual Linux release was built: 226 files, 2,800,324 bytes, 17 shaders,
  two work/ runtime libraries and the top-level layer; no weights or NVIDIA DLL.
  This size describes the local test snapshot, not a fixed release-size promise.
- The package was copied to a separate consumer folder and installed into a fresh
  test game directory. The installed 64-bit layer loaded through Vulkan.
- The consumer copy's native image tests are byte-identical to NumPy, and its GPU
  GEMM contract tests pass with the packaged runtime and shaders.
- Its actual daemon processed three synthetic 320x180 BGRA8 frames, using local
  consumer weights, with zero refused frames and preserved alpha. The startup half
  probe ran; the network was 320x320 due to min_extent. No FPS claim is made from
  this small smoke test.

Raw build logs, package contents and consumer-only weight links remain local in
ignored dist/ and work/. The consumer folder is test material, not a distributable.

## Windows check still needed

Use this source branch and MSVC to build a Windows release into a new folder, then
run its setup.bat with a user-supplied DLL in a fresh game folder. Confirm that the
daemon starts with the packaged runtime, reports keeping NumPy blocks, passes the
half-rounding probe and answers real game frames. Check paths with spaces and
parentheses, effect off/on and the local settings/log paths. No Windows batch,
MSVC or gameplay run was performed on this Linux host.
