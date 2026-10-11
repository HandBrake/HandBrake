# Windows — the port, and where it stands

> **In this tree (andyvand/dlss-nr-on-vulkan, 2026-10-02):** this file and
> `docs/WINDOWS-PORT.md` / `docs/PERF-WINDOWS.md` are `uzbekunknown/dlss-nr-on-intel`'s record of
> its Windows port, carried over verbatim with its 34 commits of 2026-09-28 to 10-02. Where it
> says what the build cannot do, read it as that tree's build: here `CMakeLists.txt` builds the
> compute side on Windows with MSVC, clang-cl or MinGW (the native passes carry halves as
> `uint16_t`, not `_Float16`), builds the layer there too through `src/layer/nr_transport.h` and
> `nr_layer.def`, and has a second runtime on Direct3D 12 (`libd3dmx`, `NR_GPU_BACKEND=d3d12`,
> `notes/phase75`) beside libxmx. The compute findings — the fp16 configuration on Intel's
> driver, the folded `packHalf2x16` round trip and `half_round` chosen per driver,
> `DenormPreserve 16` — hold on both; `tools/build_win.bat` and `tools/deploy.*` are upstream's
> MSVC-into-`work/` route and assume that layout.

For installation and a first game test, start with the
[Windows user quick start](WINDOWS-QUICKSTART.md). This file records the port's
status, driver findings and alternative build paths.

**Experimental; first run on Windows on 2026-09-29/30** (Arc 140V, Intel's driver 101.8991,
then 101.9033). The compute side builds and runs there, and since PR #3 joined (2026-10-02) the
layer and the daemon do too, from an MSVC build (`docs/WINDOWS-PORT.md`). What
the machine said is `notes/phase71-intel-windows-driver.md`. The plan below was written on Linux
before that session, and it is corrected where the machine disagreed.

## What is meant to run

| | Linux | Windows, first milestone |
| --- | --- | --- |
| the runtime, `libxmx` | yes | `work/libxmx.dll` |
| the native host passes, `libnr_image` | yes | `work/libnr_image.dll` |
| `coopmat_probe`, the GPU tests | yes | yes |
| photo mode on a still, `src/ref/nr_frame.py` | yes | yes |
| the game layer and live mode | yes | the MSVC build, on a named pipe (`docs/WINDOWS-PORT.md`) |
| `nr-photo`, `nr-toggle` (bash) | yes | no |

The runtime is plain C and Vulkan and the native passes plain C with OpenMP, so the compute side
needs no porting — only a build. The Python side loads `work/lib<name>.dll` there, with its
search path set as Python 3.8+ requires (`src/gpu/xmx.py`, `native_library`).

## The one unknown that decides everything

Every kernel here is written for `VK_KHR_cooperative_matrix` with an `fp16 x fp16 -> fp32`
configuration of **M=8, N=16, K=16** at subgroup scope — what Mesa's ANV offers on Xe2
(`notes/hw-coopmat.md`: six configurations). **Whether Intel's own Windows driver offers the
same is not known here.** `coopmat_probe` answers it in a second, and it is the first thing to
run. If the configuration is missing, nothing else in this file matters until it is solved.

And every speed figure in this repository was measured on Mesa: the shared-memory partitions,
the subgroup width, the specialisation constants and the kernels' shapes were all tuned against
ANV's compiler. Intel's compiler is another one. Expect correct results if the configuration is
there — the tests compare bytes — and expect to measure the speed rather than assume it.

**Answered on 2026-09-29: the configuration is there.** Intel's driver offers four configs, the
fp16 one among them. The compiler turned out to be the difference: it folds
`unpackHalf2x16(packHalf2x16(x))`, which `half_round` relied on, so libxmx now picks the spelling
per driver. The graph runs 1.2-1.4x Mesa's time. The picture was 47-49 dB from Linux's until
libxmx declared `DenormPreserve 16`. Left undeclared, Mesa flushes float16 subnormals in the
GEMMs and Intel's driver keeps them. With the mode declared, the two compute the same network
output bit for bit (phase71).

## Before building

1. **An Intel Xe2 GPU** — Lunar Lake 130V/140V, Battlemage B570/B580 — with Intel's current
   driver.
2. **The Vulkan SDK** (LunarG): the headers, `vulkan-1.lib`, `glslangValidator`. It sets
   `VULKAN_SDK`, which CMake reads.
3. **MSYS2**, in its UCRT64 shell:
   `pacman -S mingw-w64-ucrt-x86_64-{gcc,cmake,ninja,python,python-numpy}`. GCC 12 or newer —
   the native passes need `_Float16`, which MSVC does not have, and CMake refuses MSVC for that
   reason. OpenMP comes as `libgomp-1.dll`.
   With a python.org Python instead of MSYS2's, set `NR_DLL_PATH` to the UCRT64 `bin` folder,
   so `libnr_image.dll` finds `libgomp-1.dll` and `libwinpthread-1.dll`.
4. **ImageMagick**, with `magick` on the `PATH`, for the still-frame tools.
5. The same checkouts as on Linux — `work/vulkan-headers` is optional with the SDK,
   `work/mlx-dlss` at `06a3e11` is not — and the weights, extracted from your own DLL by the same
   two Python commands as in the README.
6. **A checkout on a path of ASCII characters only**, such as `C:\dlss-nr`. Python hands the
   runtime its shader paths as UTF-8 and the runtime opens them with `fopen`, which reads a path
   in the ANSI code page: a folder named in Cyrillic, as a user profile may be, fails to open.
7. **`PYTHONUTF8=1`** in the environment for anything run by hand. Windows' Python opens text in
   the ANSI code page, and the notes, the README and the knob table are UTF-8; CTest sets it for
   its own tests.

## Git on Windows

The repository's `.gitattributes` keeps every file LF in the checkout and batch files CRLF, so
`core.autocrlf` cannot turn a one-line change into a whole-file rewrite, as it once did. Write
commit messages from a file, or from Git Bash: PowerShell's defaults put a byte-order mark in
front of them.

## Build

```sh
cmake -G Ninja -S . -B work/cmake
cmake --build work/cmake
work/coopmat_probe
ctest --test-dir work/cmake --output-on-failure
```

The layer is off by default in this build on Windows (`NR_BUILD_LAYER`), so CTest registers the
compute tests and the tree's own checks, not the layer's. The layer, the daemon's named pipe and
everything else build with MSVC instead: `tools\build_win.bat`, into the same `work/`, so keep
one build per checkout (`docs/WINDOWS-PORT.md`). Both give the same heads.

Both builds also make `work/libnr_alloc.dll`, the daemon's NumPy allocator on Windows. Windows'
heap gives a freed block of about a megabyte or more straight back to the system, so every
frame's full-frame arrays started on fresh pages and paid a page fault for each 4 KB: 12 700 a
1280x720 frame, 30 000 at 1080p. The daemon now keeps those blocks from one frame to the next,
and gives back whatever a whole frame did not take again. That is 7-8 ms off a 1280x720 frame
and 12-21 ms off a 1080p one. `NR_KEEP_BLOCKS=0` turns it off, and without the library the
daemon runs as before. Linux does not build it.

The staged GEMM uses raw 128-bit operand copies on Intel's Windows driver. On Arc 140V
with driver 101.9033 this reduced warm graph time by about 6% at 320x320 and 9% at 720p,
with the same output bytes (`notes/improve-shared-memory.md`). Set `XMX_STAGED_PACKED=0`
before starting the daemon to compare the old loader; `=1` forces the new one. Since
2026-10-07 Mesa takes it too: about 2 % at 720p and 1080p, nothing at 320x320, the same
output bytes. Rebuild both `libxmx` and the three staged shaders
in the checkout named by the game's `NR_ROOT`; changing another checkout does not update
an already running daemon or a separate MSVC build.

On Intel's driver, leave `gpu_window_attention` out: `ctest --test-dir work/cmake -E
gpu_window_attention`. Its unmerged variant, which the graph does not use by default, hangs the
engine from 32 windows up (phase71). Everything else passes, now that libxmx declares
`DenormPreserve 16`; before it, a zero's sign in two GEMMs and 378 values of the ViT attention's
unfused reference differed here.

## The first session, in order

1. **`coopmat_probe`**: the configurations, against `notes/hw-coopmat.md`.
2. **CTest**: every GPU test compares against a reference — bit for bit where the path is meant
   to be exact. A failure here is a driver difference to understand before anything is timed.
3. **Photo mode** on a frame both systems have: `python src/ref/nr_frame.py IN.png OUT.png
   --resident`. The picture should be the Linux one; `src/bench/frame_replay.py` prints the
   head's hash and the warm graph time, the same numbers on both.
4. **Timing** with `src/bench/frame_replay.py --size 320 320` and `--size 720 1280`: the live
   extent and a 720p frame (on its 1344x768 field), against the same commands on Linux. `live_rates.py` speaks `AF_UNIX`,
   which Python has not here; the daemon itself listens on a named pipe.

## The second milestone: live mode in a game

- **Games on Windows present through D3D11 and D3D12, not Vulkan**, and the layer sees only
  Vulkan presents. For D3D9-11 titles — Tekken 7, Dead or Alive 5 — DXVK's DLLs in the game's
  folder put the game on Vulkan, as Proton does, and the layer attaches as it does on Linux. For
  D3D12, and for motion vectors and a frame before the interface, the planned OptiScaler route
  (`notes/HANDOFF.md`, "Planned") is the one.
- **The layer's POSIX parts**, all in `src/layer/nr_layer.c`: a pthread mutex (`pthread_mutex_*`,
  twenty-one uses — an `SRWLOCK`), the Unix socket to the daemon (`socket`, `connect`, `send` with
  `MSG_NOSIGNAL`, `recv`, `close` — TCP on `127.0.0.1` through Winsock, or a named pipe),
  `access` on the trigger file (`_access`), and one `open`/`read`/`write` pair. Then a Windows
  layer manifest, found through `VK_ADD_LAYER_PATH` or the registry
  (`HKCU\Software\Khronos\Vulkan\ImplicitLayers`), and `VK_LAYER_EXPORT` on the entry points.
- **The daemon**: Python has no `AF_UNIX` on Windows, so the same TCP or pipe transport on its
  side. `nr-ctl` and `nr-panel` talk through the settings file, the trigger and the daemon's
  log, and are portable once their paths leave `/tmp`.
- **The launchers** — `nr-photo`, `nr-toggle` — are bash; a Python or PowerShell one replaces
  them.

PR #3, an outside contributor's port of exactly this part — the layer's threads and transport, a
named pipe for the daemon, an MSVC build, deploy scripts — run on a B580, joined the main line on
2026-10-02.

**The first game on this machine is Mortal Kombat 11** (2026-10-02). It is 64-bit D3D11 and ran
through DXVK 3.1.1 with the layer from `build_win.bat` in live mode. At 1280x720 the daemon
answered every frame in 60 ms at render scale 0.35 and in 80 ms at 0.5. The game's own counter
showed 10 fps at 0.5. **Dead or Alive 5 Last Round** followed the same evening. It is 32-bit
D3D9, and ran with `work\nr_layer32.dll`, which `build_win.bat` now builds beside the 64-bit
layer. The daemon stays 64-bit, on the same kind of pipe.

To run a game with nothing installed or registered, as those tests did:

- put DXVK's DLLs for the game's API beside its executable: `x64\d3d11.dll` and `x64\dxgi.dll`
  for a 64-bit D3D11 game, `x32\d3d9.dll` for a 32-bit D3D9 one;
- put a copy of `src\layer\VkLayer_dlss_nr.json` in a folder of its own, its `library_path` the
  absolute path of `work\nr_layer.dll`. For a 32-bit game it is `work\nr_layer32.dll`, and
  `library_arch` is `"32"`;
- start the game from a shell with these set:
  - `VK_ADD_IMPLICIT_LAYER_PATH`: that folder;
  - `ENABLE_NR_LAYER=1`;
  - `NR_LAYER_LIVE=1`;
  - `NR_LAYER_SPAWN=1`;
  - `NR_LAYER_SOCKET=\\.\pipe\<name>`;
  - `NR_ROOT`: the checkout;
  - `NR_PYTHON`: the interpreter with NumPy.

**A Steam game that calls `SteamAPI_RestartAppIfNecessary` restarts itself through Steam when it
is started this way, and loses the environment.** The call need not be in the executable:
Dead or Alive 5's imports only `SteamAPI_Init`, and the Steam API library beside it restarted
the game all the same. The sign is DXVK's log, which then lands beside the game rather than in
`DXVK_LOG_PATH`. A `steam_appid.txt` holding the game's app id, beside the executable, keeps it
in place. The daemon the layer starts ends with the game, whose process id the layer hands it.
And a game in a Steam library that Proton also runs gets its folder back as it was: a DLL left
beside the executable is found there before Proton's own.

**A release's setup needs none of that: it puts a `vulkan-1.dll` beside the game**
(`src/layer/nr_vulkan_proxy.c`). Windows looks for `vulkan-1.dll` in the executable's folder
before System32, for a game's own import and for DXVK's `LoadLibrary` alike. The proxy reads
`dlss-nr\nr-env.txt`, and in the process of the executable named there, compared as a file and
not by its spelling, it sets those variables, records the launch for setup's status and lists
the folder the game was started in. All 265 of the loader's exports go on to System32's loader,
or to the game's own copy, set aside. `DISABLE_NR_PROXY=1` turns it off. On 2026-10-09 these
ran with NR, with nothing set in Steam: Mortal Kombat 11 and Dead or Alive 5 from Steam's Play
button, Mortal Kombat Komplete Edition from its own executable, and DOOM through Steam's
`DOOMx64.exe`, which starts `DOOMx64vk.exe`. Three things they showed:

- DXVK opens its first log before it loads `vulkan-1.dll`, so before `DXVK_LOG_PATH` is set,
  and in the folder the game was started in: Steam starts Mortal Kombat 11 in its root, above
  the executable. Remove NR deletes the DXVK logs written there since the installation.
- Once DXVK frees its first instance the proxy can be unloaded, and a later
  `LoadLibrary("vulkan-1.dll")` gets the loader of that name already in the process, System32's.
  So everything the proxy does, it does at its first load.
- A game that loads System32's loader by its path, or keeps its DLL search to System32, never
  loads the proxy. Setup's **Launch game** still starts such a game with the variables.

## Shipping it to someone without a compiler

Two scripts, and the split between them is the point:

- **`tools\deploy.bat --release <folder>`** runs here, on a machine with MSVC and the Vulkan
  SDK. It builds and assembles a distributable in a new or empty folder. The shared
  assembler requires the layer, `libxmx.dll`, `libnr_image.dll`, **`libnr_alloc.dll`**,
  the runtime shaders, startup probe and MLX-DLSS modules/tools before copying.
  The output contains neither the model weights nor NVIDIA's DLL. A build that already
  has the MLX-DLSS modules can use `--skip-weights`; users extract their own weights later.
- **`dist-tools\setup.bat`** runs on the *other* machine, from inside that folder, with
  Python but no compiler. It finds the user's own `nvngx_dlssnr.dll` (beside the script, a
  `--dll` path, or a prompt), extracts the 649 tensors from it, installs the layer into the
  game folder under a name they choose, writes the manifest with an absolute `library_path`,
  and writes a launcher that sets `VK_LAYER_PATH`, `NR_LAYER_SPAWN`, the pipe name and
  `NR_ROOT`.

    tools\deploy.bat --release dist\dlss-nr-windows --dll X:\path\nvngx_dlssnr.dll

  then the user unpacks `dist\dlss-nr-windows` anywhere and runs `setup.bat`.

The weights stay in the release folder on the user's machine; the launcher points `NR_ROOT`
at it rather than copying 278 MB into the game. Nothing NVIDIA's is redistributed by either
script — the DLL is the user's, and the weights are derived from it locally.

The folder includes [a release quick start](RELEASE-QUICKSTART.md), licenses and
initial settings at render scale 0.4. Setup writes a launcher with release-local
settings, log and trigger paths; the effect starts off until the trigger is created.
This first release route is x64. The source setup above remains the route for x86 games.
