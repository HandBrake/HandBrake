# Windows Port — Complete History, Findings and Verification

*Every discovery, change, test and lesson from porting DLSS-NR on Intel to Windows, in
the order it happened. Written for whoever picks this up next.*

## Since it joined the main line (2026-10-02)

This branch came in squashed, with three of its choices replaced by the main line's, and
what follows is kept as the record of how the port was made. Where it and the code disagree,
the code is the current one:

- `half_round`'s spelling is chosen per driver when libxmx builds each pipeline
  (specialisation constant 1), not by `-DHALF_ROUND_FLOAT16`; `build_win.bat` defines nothing
  per driver and writes no `half_round.txt`, and the start-up probe asks libxmx
  (`xmx_half_by_cast`) which spelling it has to check.
- The libraries load through `xmx.native_library`; there is no `nr_build` hook.
- The features go into the mapped input except on a discrete card under Windows
  (`xmx_discrete`), where the B580 got NaN that way; `NR_INPUT_VIEW` decides either way.
- libxmx declares `DenormPreserve 16`, so Windows and Linux compute the same graph bit for
  bit, and the three tests that differed here by a zero's sign or at mask 7 pass
  (`notes/phase71-intel-windows-driver.md`).
- The daemon keeps NumPy's large blocks from one frame to the next on Windows
  (`src/layer/nr_alloc.py`, `work/libnr_alloc.dll`): Windows' heap gives them back to the
  system, and every frame paid their page faults again. `NR_KEEP_BLOCKS=0` turns it off.
- The pipe reads a frame straight into the buffer the daemon keeps for it, and writes the
  answer from where it lies (`recv_into`, `sendall`). The frame used to be copied three
  times on the way in and twice on the way out: 5.6-6.0 ms of a 1280x720 frame, where
  1.0-1.1 are left.

## What this branch adds to master

1. **The Vulkan layer builds and runs on Windows** (MSVC), with a named-pipe transport,
   the loader exports the loader needs, and per-platform locking.
2. **The layer spawns the daemon itself** (pseudo single-DLL), opt-in and recursion-safe,
   on both platforms.
3. **The daemon listens on a named pipe on Windows** (`nr_pipe.py`).
4. **`libxmx.dll`, `libnr_image.dll` and all shaders build on Windows**
   (`tools/build_win.bat`), with the host passes byte-identical to NumPy.
5. **Deploy and release setup scripts** that carry the runtime, rename the layer, write
   the manifest, and extract weights from the user's own DLL.
6. **A start-up probe** that refuses to run when the driver's float16 conversion is
   broken, with the evidence it was built from.
7. Two MSVC-port bug fixes in master's own files (`nr_image.c`, `nr_frame.py` encoding).

## The transport decision, and why it changed twice

- **TCP loopback (first attempt)**: chosen because CPython has no `AF_UNIX` on Windows
  and a named pipe seemed to need an auth handshake. Wrong on the second point —
  `CreateNamedPipeW` in byte mode with a `NULL` security descriptor is anonymous, and
  `nr_pipe.py` proves it.
- **Named pipe (final)**: `nr_transport.h` keeps the transport in one place — `AF_UNIX`
  on Linux unchanged, a byte-mode pipe on Windows read with `PeekNamedPipe` and a
  deadline because pipes have no `SO_RCVTIMEO`. `nr_pipe.py` is the daemon's half, with
  the same `accept`/`recv`/`sendall` shape a socket has, so `serve()` does not care.

**Lesson**: measure the platform claim before designing around it. The "pipes need an
auth handshake" belief cost a parallel implementation.

## Bugs found by actually running it (each one invisible to compile+link)

| # | Symptom | Root cause | Fix |
|---|---------|-----------|-----|
| 1 | daemon dies before listening | `xmx.py` hardcoded `libxmx.so` | per-platform name, `nr_build.library()` preferred |
| 2 | daemon dies on the 2nd connection | `nr_pipe.accept()` raises `ERROR_PIPE_BUSY` when the backlog is momentarily full | retry, carry empty handle |
| 3 | tools never see a running daemon | `nr_paths.alive()` used `socket.AF_UNIX` | open-the-pipe probe |
| 4 | `frame_profile.py` crashes on Windows | shaders read without encoding | `encoding="utf-8"` |
| 5 | **whole picture NaN through the daemon** | **my own probe**: a second `xmxres.Runtime()` in the daemon's process reset libxmx.c's process-global Vulkan state | probe moved to a child process |
| 6 | batched GEMM path dead: `cannot open spv` | `build_win.bat` compiled "every .comp to a same-named .spv", missing the three aliases (gemm_batched ← gemm_coopmat_batched, gemm_f16acc ← gemm_coopmat_f16acc, gemm_tiled ← gemm_resident `-DRM=2 -DRN=2`) | the Makefile's alias table transplanted |
| 7 | contract test `16x32x16 batch 3 B^T` FAIL | same root as 6 — the staged/batched routing was broken | same fix |
| 8 | layer DLL loaded but silent | first build exported `nr_GetInstanceProcAddr`; the loader asks for **`vkGetInstanceProcAddr`** | `nr_layer.def` with the real names. *Corrected 2026-10-02:* the layer defines no function by those names, so with `vulkan-1.lib` linked they exported the loader's own; the loader had reached the layer through the negotiation all along. They are aliases of the layer's functions now, and the layer links no Vulkan library (`notes/HANDOFF.md`) |

**Lesson**: a green compile + a green `dumpbin /EXPORTS` proves nothing about which
*names* are exported or whether the pipeline computes. Every gate here was passed by a
build that did not work.

## The `packHalf2x16` story — a diagnosis I got wrong, then right

- The B580's Windows driver fails the **round trip** `unpackHalf2x16(packHalf2x16(x))`
  against numpy on 90109 of 90368 values, across two driver versions (101.8993 →
  101.9033), identically.
- I first concluded "the instruction is broken, replace it everywhere" and hand-spelled
  the packing in the four attention shaders. **The picture went black.**
- Why: the softmax's exponential in those shaders is a **bit trick on the packed word**
  (`(packHalf2x16(affine) << 5) + 0x7ff88000`, read back as bits). The trick needs a
  *deterministic* packing whose bits land where the bias expects — and this driver's
  packing does that. It never reads a float back to compare with numpy. My hand-spelled
  pack produced numpy-matching bits that landed somewhere else in the bias's exponent
  space, so my "fix" moved a working instruction.
- **Resolution**: attention shaders keep the hardware `packHalf2x16` exactly as master
  has them; `half_round` (which does read a value back, and which Mesa folds when
  spelled `float(float16_t(x))`) keeps the compile-time switch, Windows build defining
  `HALF_ROUND_FLOAT16`.
- This was caught because the user remembered **two passing runs** from a batch of
  A/B tests I had dismissed. Re-running their exact conditions reproduced the pass and
  falsified my conclusion.

**Lessons**: (a) a probe that reports a defect while running inside a state you do not
fully understand is reporting on that state too; (b) when a batch of A/B results
contradicts your conclusion, the contradiction is the data; (c) a bit trick's contract
is *bits*, not values.

## MSVC porting notes (each cost real time)

- `_Static_assert` needs `/std:c11` — the errors point at the asserts, not the flag.
- MSVC exports nothing from a DLL. Export lists are **generated from the source** at
  build time, so a symbol added later cannot silently go missing (a missing one only
  shows as `function 'xmx_...' not found` from ctypes, far from the cause).
- `nr_image.c` uses `_Float16` (GCC/Clang). On MSVC a half is carried as its sixteen
  bits, and F16C's instructions convert them when the build defines `NR_F16C`, as
  `build_win.bat` does. Without it the file's own `half_bits`/`half_from_bits` arithmetic
  does, verified against numpy over **all 65536 half values** and against F16C over all
  2^32 floats. The first hand-rolled widening had a wrong subnormal exponent bias
  (0x8f-shift instead of 113-shift); caught by the file's own test suite, byte-identical
  in all of its now 353 checks.
- MSVC `/openmp` refuses `size_t` loop variables (C3015), and any `parallel for` in C mode;
  the loops count with `ptrdiff_t` and `build_win.bat` compiles the file as C++ (`/TP`), so
  they run in parallel on Windows too. The GPU does the network; these are the passes around it.
- `restrict` → `__restrict`; `__attribute__((always_inline))` → `__forceinline`.
- `<windows.h>` defines `interface` as `struct` — a Vulkan entry point's parameter named
  `interface` breaks the build 1300 lines later.
- Redirection inside a cmd `if (...)` block is **not performed**, and a variable set
  inside a block is invisible to `if defined` later in the same block. Both fail
  silently. Every batch script here uses top-level `goto` for anything with a
  redirection or a set-then-test pair.
- A `bat` file needs CRLF; a UTF-8 BOM makes cmd try to run `锘緻echo`. `git` treats a
  CRLF rewrite of a LF file as a full-file change — normalize, don't churn.

## cmd batch scripts: the recurring traps

- `%~dp0` changes after `shift` — capture `TOOLSDIR` before any argument loop.
- `for %%I in ("path with spaces\..") do set "X=%%~dpI"` truncates at the space.
  Resolve with `pushd` + `%CD%`, or keep `..` unresolved and let the filesystem do it.
- `xcopy` inside a `for (...)` block does not run. Plain lines.

## Verification performed (Windows, on this machine)

- `build_win.bat` from a plain prompt: `libxmx.dll` (with `/std:c11`, export list
  generated from source), `nr_layer.dll`, `libnr_image.dll`, **20 shaders** — 0 errors.
- Loader: `vulkaninfo --summary` lists `VK_LAYER_dlssnr_intel`;
  `test_layer_loader.c` creates an instance through the layer (exit 0).
- Transport: `test_nr_link_win.c` ↔ `nr_pipe` round trip, bytes matching.
- Unit: `test_native_image.py` **197/197 byte-identical**; `test_gemm_contract.py`
  **all checks pass** (after #6); `test_resident.py` **711 passes / 0 fail** — the whole
  graph tracks the host reference on the B580; `test_input_fp16.py` exact across
  schedules.
- **End to end**: Aperture Desk Job (Steam, Source 2, `-vulkan`) → layer → named pipe →
  B580 inference → 4K frame loop. Best runs: **200 frames, median output ~90 KB, no
  bands**, row-difference std 1.76 (banded runs were 4+).
- Frame times: 4K at scale 0.5 ≈ 2.3–2.9 s to the answer with `gpu 12+300+14 ms`; 640x360
  scale 1.0 ≈ 0.07–0.08 s with `gpu 1+20+1 ms`. With `libnr_image.dll` built, the host
  passes around the network stop falling back to NumPy (they were the dominant cost at
  4K).

## Not fixed, and where the boundary is

- **master's `libxmx.c` has an intermittent `fence wait (-4)` on Windows** (first
  submit after init, B580, current driver). It reproduced during one debugging session
  and not during others on the same day; master's own `test_gemm_contract.py` passes
  19/19 now. Untested on Linux from here — that is where master develops this file, and
  the README calls the Windows runtime "separate changes".
- The reduced `packHalf2x16` reproducer for an Intel bug report is not trimmed yet; the
  probe is the starting point.
- Linux is code-reviewed only from this machine: the socket path is byte-for-byte
  master's, plus the spawn function which is POSIX-native (`posix_spawn`, env copy,
  `dladdr`, `realpath`).

## Compliance

The repository never carries NVIDIA's DLL or any weights derived from one. The extractor
(`scripts/get_weights.py`) drives MLX-DLSS against a DLL the user supplies, writes into
git-ignored `work/`, and verifies 649 tensors. Deploy scripts point the daemon at the
checkout (`NR_ROOT`) instead of copying 278 MB of weights into a game folder. The release
setup (`dist-tools/setup.bat`/`.sh`) finds the user's own DLL two ways — beside the
script, or a path they give — and **downloads nothing**: an earlier version fetched a
third-party pack carrying NVIDIA's binary, which is redistribution and is gone.

## Final sweep (post quality pass)

- Windows knob-file/log defaults were /tmp/... — no such directory there. 
r_paths.py
  now picks 	empfile.gettempdir() on Windows; ensure_daemon()'s settings/log defaults
  match through a 
r_default_dir() helper. Linux keeps /tmp.
- 
r_daemon.py normalized to LF (its CRLF copy read as a 2000-line diff).
- master moved twice more during the work: a portable subgroup-width pin (pin only where
  the driver can — a desktop Arc without subgroup size control got no device before) and
  the OpenDLSS-NR reference files. Both rebased in cleanly; all suites re-run green after.
- Git notes from the trench: a detached-HEAD commit after a rebase is easily lost when a
  branch -f reads the wrong HEAD — reflog has it, cherry-pick recovers it; and a CRLF
  copy makes every diff review impossible, so normalize before reviewing anything.


## The review round: what the maintainer found, and what the B580 answered

The maintainer ran the branch on his own Arc 140V and reported back. Three things came out
of it, and one of them corrected me.

- **The branch did not build on Linux at all — two one-line errors**, both invisible to
  MSVC because the Windows arm takes different branches. `nr_transport.h` used `read`,
  `write`, `close`, `errno` and `EINTR` on the POSIX side with only `<sys/socket.h>`,
  `<sys/un.h>` and `<sys/time.h>` included, none of which provides them, and used
  `ssize_t` and `snprintf` without `<sys/types.h>` or `<stdio.h>` either.
  `nr_image.c`'s GNU `NR_ALWAYS_INLINE` was defined as **itself**. That is the concrete
  cost of developing one platform's arm without a compiler for the other: the Windows
  build was green the whole time.
- **`NR_QKV_EPILOGUE` was off for everyone, and that was wrong.** Turning it off after
  bands on the B580 cost Linux **143 -> 257 ms** of graph at 1344x768 for the same head.
  The default follows the platform now — `1` on Linux as upstream has it, `0` on Windows.
  The maintainer's own measurement settled it: 60 frames from three games, byte-identical
  with the switch on and off on his GPU, so the fusion is not what was wrong.
- **The bands were never the `packHalf2x16` story either.** `test_gemm_qkv.py` fails on the
  B580 with K differing at ~2 of 245824 elements — always the first lane of a head, always
  only the sign of a zero, and a different set of heads each run while the count holds. It
  is not driver nondeterminism: three identical float32 GEMMs in one command buffer, and
  the same GEMM across two submits, are bit-identical every time. And it fails
  **identically with the epilogue on and off**, which is why turning that switch off never
  fixed the bands — and why the maintainer's byte-identical result and my "it helped"
  observation were both correct about different things.

**The lesson worth keeping:** I had two separate symptoms — bands and a poisoned probe —
and merged them into one story because the probe's number looked like an explanation. The
probe was running inside the very state it was measuring. Isolate the instrument before
trusting its reading; that is the second time on this port that the fix was a process
boundary.

**Also this round:** `nr_image.c`'s colour grade (master's newest commit) writes
`__attribute__((always_inline))` literally at three call sites, which MSVC rejects; those
go through the file's macro now. `src/tools/build_check.py` reads files with plain
`read_text()`, which resolves to GBK on a Windows locale and dies on the first UTF-8 byte
— three `encoding="utf-8"` arguments. And the release setup scripts **no longer download
anything**: fetching NVIDIA's DLL from a third-party pack is redistribution, so that path
is gone and a missing DLL prints guidance instead.

**Verified after all of it** (on master at `72a7440`): `build_win.bat` builds all four
artefacts and 20 shaders with 0 errors; `test_native_image.py` byte-identical;
`test_gemm_contract.py` every check; `test_resident.py` matches the reference;
`build_check.py` passes on Windows; a real Steam game through the deployed layer —
144 frames, no bands.


## The release scripts, and a class of bug worth naming

Three review rounds went into `dist-tools/setup.bat` / `.sh`, and every finding was the
same shape: **the script reported success without being runnable.** Worth recording as a
category, because each one failed somewhere far from its cause.

- **It downloaded NVIDIA's DLL for you.** A split 7z from a third-party release, fetched
  when the user's copy was not found. That is redistribution whatever the intent, and it
  is the one thing the brief forbids outright. The download path, its `--aio` option and
  its `aio/` folder are gone; a missing DLL prints where to put one and exits 3.
- **`--dll` accepted a directory.** The check was `if exist "%DLL%"`, which a folder
  satisfies, and the value went straight to the extractor. The interactive prompt had a
  *different* rule that did look inside a folder — so the two entry points disagreed
  about what was valid, and only the command line was broken. One resolver now serves
  both: a folder expands to the file, a bare file is accepted only if that is its name,
  anything else explains itself and exits 3.
- **`--skip-weights` trusted the user.** It jumped past the existence check *and* the
  extraction, so a folder with no weights installed, wrote a launcher and printed Done —
  and the failure appeared later, in the game, pointing nowhere. It verifies the weights
  file now and refuses to proceed without it.

Two bugs of my own surfaced only by running the reviewer's cases, and both fail
silently, which is why reading the code did not catch them:

- `if "%X:~-1%"=="" ...` puts a backslash against the closing quote; cmd mis-parses it
  and the script dies with "The syntax of the command is incorrect" before printing
  anything.
- The resolver was written **inline between two main-flow statements**, so cmd walked
  into it and its `exit /b 1` ended the run before any argument was read. The logic was
  right and in the wrong place — invisible to review, obvious to a test.

**The lesson:** for a script whose whole job is to decide whether the machine is ready,
every branch needs a test that asserts the *exit code and the absence of side effects*,
not that the happy path printed the right thing. Eight such cases now pass, including
`--dry-run`, which the reviewer suggested and which reports the full plan while writing
nothing — the install being the first write, that flag makes the whole chain checkable on
a machine nobody wants a layer installed on.


## The second review round: OpenMP, stdin, and a papercut that was real

The maintainer ran the branch on his own machine under both Linux and Windows and came back
with five items. Four were as described; chasing them found one defect that no amount of
reading would have caught, and one that could not be fixed the obvious way.

- **MSVC's `/openmp` rejects `#pragma omp parallel for` in C mode.** Not these loops - the
  canonical two-line sample, `int` index, no schedule clause, C3015 "improper form" - while
  the identical code compiles in C++ mode. The suggested fix (signed loop counters) could
  not have worked on its own; the file has to go through the C++ front end, which costs
  four explicit `malloc` casts. Only then do the counters matter: OpenMP 2.0 also wants the
  bound signed, so `ptrdiff_t` against `size_t` fails again.

  Measured on the fused compose-and-encode pass at 1280x720: **18.64 ms → 4.06 ms**, and
  `libnr_image.dll` now depends on `VCOMP140.DLL` where before it ran every pass on one
  core. The whole 10 % the maintainer measured at 720p was this.

- **A spawned daemon died on stdin.** `subprocess.run(capture_output=True)` with no `stdin`
  against the layer's `hStdInput = INVALID_HANDLE_VALUE` → WinError 6. My own spawn test
  passed only because `half_probe.spv` was not being built, so the probe returned before
  reaching that call - the maintainer guessed exactly that, and deleting the stale `.spv`
  confirmed it. Fixed on both sides: `stdin=DEVNULL` in the probe call, and an inheritable
  `NUL` handle for the child in the layer.

- **`build_win.bat` never built `half_probe.spv`.** `:shader` looked only in `src/gpu`, so
  `call :shader half_probe` fell through to `:eof` in silence, and the `.spv` sitting in
  `work/` was from a manual run three days earlier. A missing source is an error now.

- **Em dashes through a GBK round trip.** Master's eighteen `—`, on fifteen lines, were
  `鈥?` here, with the following space eaten. Two sat at end-of-line where the newline went
  too, so a naive repair joined comment lines; done against master byte for byte.

**The lesson, and it is the same one twice over:** both silent failures here - the log that
stayed empty, the `.spv` that was never built - produced a *working* system that was merely
slower or quieter than it should be. Neither had a symptom pointing at its cause. The build
script skipping a missing source without a word is how a three-day-old artefact went on
being used; that one line is now an error.

One item was deliberately **not** acted on: `test_window_attention.py` hangs the GPU on the
maintainer's machine (Windows LiveKernelEvent 141), on the unmerged store that the graph
builds only under `NR_FUSE_ATTENTION_MERGE=0`. This machine already has two bugchecks and
TDR events behind it, so it was left as his finding rather than reproduced blind.
