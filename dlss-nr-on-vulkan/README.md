# DLSS 5 Neural Rendering on Vulkan, Metal and Direct3D12

NVIDIA's DLSS 5 Neural Rendering pass — the one-step pixel-space diffusion model that
re-renders a frame's detail — running on **Any Vulkan enabled card**
integrated GPU under macOS, Android, Linux and Windows in a real game, through a Vulkan layer.

No NVIDIA hardware, no NGX, no CUDA. The graph runs on Intel's XMX matrix units through
`VK_KHR_cooperative_matrix`, and the pass is injected at `vkQueuePresentKHR`, so it
attaches to anything that presents with Vulkan: a Windows game under Proton on Linux, and on
Windows a Vulkan game or a DirectX 8–11 one through DXVK.
Where there is no cooperative matrix at all — an Apple M3 through MoltenVK is the case
that was built and tested — the same graph runs on a plain multiply-add GEMM, slower and
with the same numbers.

**It is playable.** Live, every frame goes through the network on the laptop's own iGPU,
beside the game: Tekken 7 runs at **30 fps at 800x450** on an Arc 140V. Block by block, its graph
matches a reference implementation that claims NVIDIA's own arithmetic; what is left between the
two is rounding, not structure (`notes/opendlss-reference.md`).

**This is a research port, not a product.**  
Read "What to expect" before deciding it is broken.

**All breakdowns and analysis are written by AI.**

---

## How to test it

| Platform | Instructions |
| --- | --- |
| Windows | **[Download the Windows release](https://github.com/Uzbekunknown/dlss-nr-on-intel/releases/latest)**, extract it and run `NR-Setup.exe`: the [release quick start](docs/RELEASE-QUICKSTART.md). It needs Python and your own DLL, no compiler or Vulkan SDK. To build it yourself: the [Windows quick start](docs/WINDOWS-QUICKSTART.md). |
| Linux | [Build](#build), then [Run it in a game](#run-it-in-a-game). Windows games can use the Proton launcher. |

You need a supported **Intel Xe2 GPU** and the logical weights extracted from your
own `nvngx_dlssnr.dll`. The repository supplies code, not the DLL or model weights;
see [You supply the weights](#you-supply-the-weights) and [What you need](#what-you-need).

For the first live test, use a small game window — **640x360 to 800x450** — and
start around **render_scale 0.4**. Choose a Vulkan renderer, or a documented
translation path; loading a Vulkan layer does not enable it in every DirectX game.
Compare the same scene with the effect off and on.

Confirm that the daemon reports processed frames and check for refused frames.
An unchanged picture can be the game's fallback after a failed request. On Linux,
run `src/layer/nr-ctl report`; on Windows, **Save report** in the setup window.
Include the commit, GPU/CPU, OS/driver, game/API, resolution, render scale,
`min_extent`, and FPS off/on in an [Issue](https://github.com/Uzbekunknown/dlss-nr-on-intel/issues).

The effect adds work to each frame. The optimizations reduce its overhead; the
rates below describe particular hardware and settings, not a guaranteed FPS increase.

### Windows and Linux

The pass is the same on both: the same weights, the same graph and the same ten settings,
and since 2026-10-01 Intel's Windows driver and Mesa compute the graph bit for bit alike
(`notes/phase71-intel-windows-driver.md`). What differs is everything around it:

| | Windows | Linux |
| --- | --- | --- |
| Install | the release ZIP and its setup window, in English, Russian or Spanish | built from source with `make` or CMake |
| GPU driver | Intel's own; tested with 32.0.101.9033 | Mesa ANV |
| Games | native Vulkan, and DirectX 8–11 through the DXVK 3.1.1 that setup puts beside the game; 64- and 32-bit; **no DirectX 12 yet** | whatever Proton runs, since DXVK and VKD3D-Proton come with it: DirectX 12 too (Mortal Kombat 1) |
| Starting the game | any way at all — Steam, a shortcut, another launcher: setup puts a `vulkan-1.dll` beside the game that gives it NR | with NR's environment: the Steam launch option `nr-photo --steam` prints, or `nr-photo` itself |
| Controls | the setup window: Enable NR and the ten settings, while the game runs | `nr-panel`, `nr-ctl`, and `nr-toggle` on a key |
| Modes | live | live, and the photo mode |
| Taking it out | **Remove NR** puts the game's folder back as it was | remove the launch option |

The network runs slower on Intel's Windows compiler at large frames — about 170 ms against
140 at 1344x768 — and as fast at the live sizes, about 24 ms at 320x320 on both
(`src/bench/frame_replay.py`). Around the network the two are level.

---

## What it looks like

Stills with the model at full resolution. Left, or on top: the game's own frame. Right, or
below: the same frame through DLSS-NR on this Intel Arc 140V.

<sub>Captured on 2026-09-16 and rendered again on 2026-09-28, from the same captured frames,
through the graph as it is now (`src/bench/restill.py`, or with `--native` through the C frame
library on any runtime); the images and the table below come
out of `src/tools/comparisons.py`.</sub>

**Tekken 7** — Unreal Engine 4, D3D11, 1920x1080, crops enlarged 2x:

![Tekken 7, Sergei Dragunov: face and jacket, game on the left, DLSS-NR on the right](https://raw.githubusercontent.com/Uzbekunknown/dlss-nr-on-intel/media/comparisons/tekken7-dragunov.jpg)

**Dead or Alive 5 Last Round** — D3D9, 1920x1080, a different engine and art style:

![Dead or Alive 5 Last Round: a front-facing close-up, game on the left, DLSS-NR on the right](https://raw.githubusercontent.com/Uzbekunknown/dlss-nr-on-intel/media/comparisons/doa5-closeup.jpg)

![Dead or Alive 5 Last Round: a three-quarter close-up by fire, game on the left, DLSS-NR on the right](https://raw.githubusercontent.com/Uzbekunknown/dlss-nr-on-intel/media/comparisons/doa5-fire.jpg)

**Mortal Kombat 1** — Modified Unreal Engine 4, **D3D12** through VKD3D-Proton, 1600x900:

![Mortal Kombat 1: Omni-Man and Homelander, game on the left, DLSS-NR on the right](https://raw.githubusercontent.com/Uzbekunknown/dlss-nr-on-intel/media/comparisons/mk1-faces.jpg)

![Mortal Kombat 1: a whole fight frame, game on top, DLSS-NR below](https://raw.githubusercontent.com/Uzbekunknown/dlss-nr-on-intel/media/comparisons/mk1-fight.jpg)

Measured rather than eyeballed. Texture is the high-frequency energy normalised for the
change in brightness, because on this model the brightness moves and it fools the eye:

| image | region | brightness | relative texture | colour change |
| --- | --- | --- | ---: | ---: |
| Tekken 7 | face | 78 -> 59 | **+28 %** | 22.0 |
| | jacket weave | 90 -> 70 | **+61 %** | 20.9 |
| | embroidery | 135 -> 115 | +43 % | 21.0 |
| | background | 35 -> 34 | -9 % | **4.5** |
| DoA5, close-up | face | 107 -> 93 | +10 % | 17.9 |
| | background | | -24 % | **4.7** |
| DoA5, by the fire | face | 96 -> 84 | +16 % | 15.3 |
| | background, fire | | -4 % | 8.2 |
| Mortal Kombat 1 | Omni-Man's face | 131 -> 131 | **-10 %** | 14.8 |
| | Homelander's face | 131 -> 130 | **-19 %** | 13.8 |
| | the whole fight frame | 56 -> 57 | -2 % | 6.8 |

**The three games do not get the same treatment, and that is the honest summary.** On
Tekken 7 and Dead or Alive 5 the character comes out darker and gains texture — a great deal
on Tekken's fabric, less on Dead or Alive's already-smooth skin — while the background is
left almost alone. On Mortal Kombat 1, whose faces are already rendered in fine detail, the
brightness does not move and the fine detail on the faces goes *down*: what changes is the
colour, with the warm filmic grade and the glow on the skin taken out. That is not film grain
being removed — a flat defocused patch of the same frame has none to remove — but whether
what goes is skin detail or the game's own sharpening has not been measured.

Across all of them, what the pass adds in one place it takes from another. **Whether any of
it is better is taste, not measurement** — it is photographic where the games are stylised.

These are stills. Live — every frame through the network, beside the game on the same
iGPU — **Tekken 7 runs at 30 fps at 800x450** with the render scale at 0.35, and 27 at 0.6
(the author, 2026-09-27).

<sub>Tekken 7 © Bandai Namco Entertainment. Dead or Alive 5 Last Round © Koei Tecmo Games.
Mortal Kombat 1 © Warner Bros. Entertainment Inc.; its guest characters belong to their
respective owners. Shown for comparison.</sub>

---

## How this was built

**This project was written by AI agents.** The author supplied the machine, the binary and
the direction, and made the decisions; the code, the measurements and the notes were
produced by **Claude Opus 5 and 5.5** and, in a parallel tree, by **Astra** — whose work on the
native host passes was taken into this one (`src/ref/nr_image.c`, `notes/phase57`).

That is stated here rather than left to be noticed, because it changes how you should read
everything else. What it means in practice:

- **Nothing is asserted that was not measured.** Every number in the notes has a program
  behind it in `src/bench/`, and `make test` is about 570 checks, including the native
  passes against the NumPy they replace byte for byte.
- **The wrong turns are in the notes too**, deliberately. A hypothesis about shared-memory
  bank conflicts that measured 1.11x instead of the textbook 32x. A "driver bug" that
  shaped three phases and does not exist. A profile recommended from one frame and
  withdrawn after the next. Several notes say at the top that they are superseded.
- **The author does not claim to be able to defend the code line by line.** What is
  offered instead is the evidence: the tests, the benchmarks, and the record of how each
  conclusion was reached — including the ones that were wrong.

Judge it on that. If something here is wrong, the measurement that would show it is
probably already in `src/bench/`.

---

## You supply the weights

**The repository is code only. It contains no NVIDIA binaries and no weights derived from
them.** `nvngx_dlssnr.dll` is NVIDIA's; every project in this space
requires you to bring your own copy, and so does this one.

Nothing here will run until you have extracted the logical weight file from a DLL you
already have. See [Build](#build).

## What you need

- An Intel **Xe2** GPU: `VK_KHR_cooperative_matrix` with an `fp16 x fp16 -> fp32`
  configuration of **M=8, N=16, K=16**, which every XMX kernel here is written for.
  Developed and measured on **Arc 140V (Lunar Lake), Mesa ANV**; an Arc B580 (discrete
  Battlemage) reports the same six configurations. **Arc A-series (Alchemist, Xe-HPG) does
  not qualify**: its matrix units report 8x8x16, so these kernels do not run there. A Vulkan
  device alone is not enough, and the probe needs neither weights nor the rest of the build:

  ```sh
  gcc -Iwork/vulkan-headers/include src/probe/coopmat_probe.c -o /tmp/probe -lvulkan
  /tmp/probe      # drop the -I if your distribution installs the Vulkan headers
  ```
- A **discrete** Xe2 card (Battlemage, B570/B580) works too, and does not need resizable BAR: where the card's memory
  cannot be mapped, the graph keeps its operands there anyway and the host reaches them by
  copies. Turn resizable BAR on if you can — it is the faster of the two paths and Arc wants
  it for everything else — but it is no longer the difference between working and crawling.
  This is written from one owner's report and tested by forcing the same path on the
  integrated GPU; it has not been measured on a discrete card.
- **Without cooperative matrix** the runtime falls back to a portable multiply-add GEMM
  behind the same dispatches, with every epilogue and store the matrix kernel has. Tested
  on an **Apple M3 under macOS through MoltenVK 1.4.2**: `make test` is green there and the
  GEMM runs at 1300-1900 GFLOP/s against the Xe2 matrix kernel's 1348-3828 (480-570 until a
  `precise` qualifier was found spread over its whole K loop, `notes/improve-portable-precise.md`);
  a 1280x720 frame's graph is 0.47 s there, 320x320 is 84 ms. Any Vulkan 1.3
  device with `shaderFloat16`, `storageBuffer16BitAccess`, `bufferDeviceAddress` and the
  Vulkan memory model should take the same path; `XMX_PORTABLE=1` forces it anywhere.
- On Linux, or macOS with MoltenVK: Python 3 with NumPy, a C compiler, `glslangValidator`
  and the Vulkan loader. On Windows the release needs only a 64-bit Python and your DLL;
  building it needs MSVC and the Vulkan SDK (`tools/build_win.bat`,
  [docs/WINDOWS.md](docs/WINDOWS.md)), or CMake with MSVC, clang-cl or MinGW for the compute
  side. The network's output is the same on both, bit for bit: libxmx declares
  `DenormPreserve 16` on every module and picks `half_round`'s spelling per driver
  (`notes/phase71`). Direct3D 12 is the other runtime there (below).
- **ImageMagick** for the still-frame tools, which read and write pictures through
  `magick`. The game path does not touch it.
- **libpng** for the C command `work/nr_frame`, which reads and writes PNG with it and
  needs nothing else. `make` finds it through pkg-config, Homebrew, vcpkg or `/usr/local`;
  `PNG_CFLAGS` / `PNG_LIBS` override.
- About 0.7 GiB of memory for the device buffers at 720p and 1.3 GiB at 1080p, the weights
  included — it shares system RAM.
- OpenCV is optional and worth having: it is the fast path for the blur that moving
  `detail_strength` or `colour_strength` needs — 32 ms against 110 at 854x480
  (`notes/phase48`). Everything else is the same without it.

## Build

**Windows users** need no build: the release carries everything ([How to test it](#how-to-test-it)).
To build it yourself, the [Windows quick start](docs/WINDOWS-QUICKSTART.md) covers the MSVC
build, the first Vulkan launch, the effect toggle and a diagnostic report. The commands below
are the Linux setup.

```sh
mkdir -p work
git clone --depth 1 --branch v1.4.321 \
          https://github.com/KhronosGroup/Vulkan-Headers.git work/vulkan-headers
git clone https://github.com/iamwavecut/MLX-DLSS.git work/mlx-dlss
git -C work/mlx-dlss checkout 06a3e11a8b68817127406ace5c764463543f699b
make
```

Then extract the weights from your own DLL (needs `safetensors` as well as NumPy):

```sh
mkdir -p work/mlxw
python3 work/mlx-dlss/python/mlxdlss/tools/extract_dlssnr_weights.py \
        /path/to/nvngx_dlssnr.dll work/mlxw/dlssnr-packed.safetensors
python3 work/mlx-dlss/python/mlxdlss/tools/unpack_dlssnr_weights.py \
        work/mlxw/dlssnr-packed.safetensors work/mlxw/dlssnr-logical.safetensors
```

On macOS, skip the Vulkan-Headers clone: `make` finds the headers and libraries under
vcpkg, `/usr/local` or Homebrew (`make VK_PREFIX=/where/they/are` otherwise), links the
compute runtime against MoltenVK directly, and writes `work/MoltenVK_icd.json` so the layer
tests can reach MoltenVK through the loader. Metal's fast math is switched off by the
runtime before the library loads; leave `MVK_CONFIG_FAST_MATH_ENABLED` alone, because with
it on every vendor rounding point in the graph moves (`notes/phase67`).

macOS also gets a **second compute runtime on Metal directly**: `work/libmetalmx.dylib`
(`src/gpu/libmetalmx.m`) implements the same `xmx_*` entry points as libxmx, with the shaders
rewritten in the Metal Shading Language (`src/gpu/metal/`) and compiled by Apple's `metal`
into one `nr_shaders.metallib` that is embedded in the library. Its matrix path is
`simdgroup_matrix` (half operands, float accumulate; exact on the GEMM contract), where
MoltenVK has none. `NR_GPU_BACKEND=metal` switches every Python tool and the C frame library
to it; `make test-metal` runs the GPU tests on it. On an M3 a 1280x720 frame's graph replays in
218 ms through it against 473 ms through MoltenVK (2026-10-03; 735-746 against 938-948 when
it was written, `notes/phase74`). It is built on Apple only —
by `make` under Darwin, by CMake under `NR_BUILD_METAL` — and the Vulkan layer stays Vulkan.
The static archive a host links, `libdlssnr`, is the Metal one on Apple: no Vulkan in it,
`nr_frame_runtime()` says `"metal"`, and `nr_frame_adopt_vulkan` is refused there.

Windows gets a **third runtime, on Direct3D 12**: `build/libd3dmx.dll` (`src/gpu/libd3dmx.c`)
implements the same `xmx_*` entry points, with the kernels rewritten in HLSL (`src/gpu/d3d12/`)
and compiled by `dxc` to nine DXIL modules that are embedded in the library.
`NR_GPU_BACKEND=d3d12` switches every Python tool and the C frame library to it, and
`ctest -R d3d12_` runs the GPU tests on it. It has no matrix path — HLSL ships no matrix-matrix
operation, so every GEMM is the multiply-add kernel — and it takes the operands as root UAVs
with byte offsets, splits dispatches at Direct3D's 65535 groups per axis, and keeps every half
store on `f32tof16` so the half buffers hold what `half_round` gives. CMake builds it under
`NR_BUILD_D3D12` — on by default for every Windows target but 32-bit x86 (x64 and ARM64) when
`dxc` is found: the Windows SDK's, the Vulkan SDK's or vcpkg's `directx-dxc` — and wherever it
is built `libdlssnr` is built from it (`NR_DLSSNR_D3D12`, following `NR_BUILD_D3D12`;
`-DNR_DLSSNR_D3D12=OFF` keeps the Vulkan archive), with `nr_frame_adopt_d3d12` for a host that
shares its `ID3D12Device`. **It is written and
cross-built from macOS, and has not run anywhere yet** — `notes/phase75` says how to make the
first run, and that a `dxc` without `dxil.dll` beside it writes unsigned DXIL the runtime takes
only in developer mode.

The daemon's whole live frame is in the C frame library too: `nr_frame_live_run`
(`nr_frame.h`) takes 8-bit pixels and gives them back through the letterbox, the render
scale, the history and its cut test exactly as `nr_daemon.py` handles them — byte for byte,
`src/ref/test_nr_frame_live.py` — so a host drives the live pass with no daemon and no socket,
on any of the three runtimes. `nr_frame --replay DUMP OUT` replays a `--dump` capture through
it, and `src/bench/restill.py --native` uses that instead of the daemon.

The result is 649 named tensors, **145 755 123 parameters**: the large matrices are
stored in the DLL as FP8 E4M3, one byte each, and decoded to FP16. The reader checks
`fully_logical=true` and refuses anything else — the packed file is **not** a substitute,
and reading it as dense FP16 gives values correlating -0.02 with the truth.

**Or with CMake**, which is the same build for Linux, macOS and Windows. Everything lands
in the build directory, flat, under the names `make` gives them in `work/`: the libraries,
the executables, every `.spv`, the ICD manifest on macOS.

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
export NR_BUILD_DIR=$PWD/build          # for the Python outside ctest; see below
```

It finds the Vulkan SDK (or the headers clone above), MoltenVK on macOS, libpng for the
`nr_frame` command, and registers every test with ctest. It and the Makefile build the same
shaders with the same flags and register the same tests — `src/tools/build_check.py` fails the
suite when they part. Both build the 32-bit layer, `work/libnr_layer32.so`, for 32-bit games,
wherever a 32-bit compiler and a 32-bit Vulkan loader are installed (on Arch, `lib32-glibc`,
`lib32-gcc-libs` and `lib32-vulkan-icd-loader` from multilib); `make LAYER32=0` or
`-DNR_BUILD_LAYER32=OFF` leaves it out, `LAYER32=1` / `ON` demands it. The Python — the daemon, the
tests, the tools — finds either build through `src/nr_build.py`: `NR_BUILD_DIR` in the
environment wins (ctest sets it for every test, `make test` pins its own `work/`), and
without it the most recently built of `work/` and `build*/` is used, so switching between
the two builds does not run last week's binaries from the other one. Only the inputs stay
in `work/`: your weights under `work/mlxw/` and the headers clone. `-DNR_OUTPUT_DIR=work`
reproduces the Makefile layout if you want one directory.

**The weights compiled in.** The CMake build compiles `weights/` — the logical safetensors
as C, one bin2c byte array per 8 MB slice plus a table — into `libnr_frame`, so
`nr_frame_open(NULL)`, the `nr_frame` command without `--weights` and the VBA-M filter open
no file at run time. That directory is used as it is; `-DNR_BIN2C_WEIGHTS=ON` (default OFF)
regenerates it first from `dlssnr-logical.safetensors` in the source root, `work/mlxw/` or
`-DNR_WEIGHTS_FILE=`, through `slice` and `bin2c`, and is skipped with a message when the
file is absent. `-DNR_EMBED_WEIGHTS=OFF` builds a library that needs a path. Compiling a
slice costs the compiler about 0.75 GB; `NR_EMBED_JOBS` (2) is how many run at once.

**The CMake build compiles the weights in.** Given `dlssnr-logical.safetensors` — in the
source root, under `work/mlxw/`, or named with `-DNR_WEIGHTS_FILE=` — it cuts the file into
8 MB slices, turns each into a C byte array with `bin2c` (`src/tools/bin2c.c`, Rafael
Kitover's, the tool the VBA-M GUI embeds its resources with) and links the lot into
`libnr_frame`, so `nr_frame_open(NULL)`, the `nr_frame` command without `--weights`, the C
test and `NativeFrame()` need no file at run time. The slicing is not decoration: a compiler
holds a byte-array initializer one element at a time and clang wants about 90 bytes of memory
per byte, so one array for the whole file would need ~25 GB; 8 MB slices cost ~0.75 GB each,
two at a time (`NR_EMBED_CHUNK_MB`, `NR_EMBED_JOBS`), about two minutes on an M3. The
library grows by the size of the file. `-DNR_EMBED_WEIGHTS=OFF` builds without them, and the
generated headers stay in `build/weights/` — `*.safetensors` is ignored by git and the
generated C never leaves the build directory, so the rule that no weights are committed holds.

**The shaders are compiled in the same way.** Every `.spv` glslangValidator writes also goes
through `bin2c` into `libxmx` and `gemm_runner` (`NR_EMBED_SHADERS`, on by default), and the
runtime loads a shader by *name*: `gemm_coopmat.spv` is the embedded module, a path with a
directory in it is a file — so `XMX_GEMM_SPV=/tmp/variant.spv` still measures a variant — and
a path whose file is missing falls back to the embedded module of that name. The C library
and the Python (`nr_build.shader_arg`) hand over bare names when the runtime has them, so a
build directory with no `.spv` in it runs; the files are still written for the benches. On Windows it builds the compute
runtime, the frame library, the command and the shaders with MSVC or clang-cl and the
Vulkan SDK; the Vulkan layer and its tests are POSIX and stay off there
(`-DNR_BUILD_LAYER`). Windows builds and links as a MinGW-w64 cross-compile from macOS (`libxmx.dll`,
`libnr_frame.dll`, `nr_frame.exe`); nobody has yet *run* it on a Windows machine, and the
MSVC path is configured, not measured (`notes/phase69`).

**Android** builds with the NDK's own toolchain file and nothing else named:

```sh
cmake -S . -B build-android \
      -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=28
cmake --build build-android
```

It links the NDK's `libvulkan.so`, builds `bin2c` and `slice` for the machine doing the build
(a compiler from your `PATH`; `-DNR_HOST_CC=` names one), compiles the weights and shaders in,
and produces the libraries, `libdlssnr.a` and the executables — `test_dlssnr` runs from
`/data/local/tmp` over `adb`. The `nr_frame` command needs a libpng for the target (vcpkg's
`arm64-android` triplet has one); the Vulkan layer stays off, there being no game to hook.
VBA-M's `tools/android/build-android-qt.sh` links `libdlssnr.a` into its Qt APK the same way.
Built for arm64-v8a and armeabi-v7a; **not yet run on a device** — a phone has no cooperative
matrix, so it takes the portable GEMM path, and needs a Vulkan 1.3 driver with `shaderFloat16`,
`storageBuffer16BitAccess` and `bufferDeviceAddress` (`notes/phase72`).

`make` also builds `work/libnr_image.so` (`.dylib` on macOS, as every library here): the full-frame passes around the network —
feature assembly, the resizes, the composition, the 8-bit codecs — in C rather than NumPy,
worth about 2.6x on the host side of a frame. It is built for `x86-64-v3` — AVX2 and F16C,
every CPU since Haswell and Zen 1 — and `NR_IMAGE_ARCH` changes that. Everything still works
without it; `NR_HOST_NATIVE=0` selects the NumPy path for a paired measurement.

```sh
make test                                        # about 570 checks, fewer without weights
python3 src/ref/nr_frame.py IN.png OUT.png --resident   # one still, no game
work/nr_frame IN.png OUT.png                               # the same command, in C, no Python
```

### The frame path as a C library

`work/libnr_frame.so` — `.dylib` on macOS — (`src/ref/nr_frame.c`, API in `nr_frame.h`) is `nr_frame.py` in C:
the weights read from the logical file, the whole graph recorded against `libxmx` exactly as
the Python records it, the feature assembly and the composition around it. One call updates
a frame:

```c
nr_frame *f = nr_frame_open("work/mlxw/dlssnr-logical.safetensors");
nr_frame_params p; nr_frame_defaults(&p);           /* the `standard` profile */
nr_frame_update(f, colour, height, width, NULL, NULL, &p, output, NULL);
```

`colour` and `output` are float32 RGB in [0, 1]; pass the previous output as `history` for
the temporal path and the previous input as the fourth argument for its floor. The head it
produces is **bit-identical** to the Python resident path on the same features
(`src/ref/test_nr_frame_c.py`); the noise channels, the gate's sigmoid and the detail blur's
kernel use the C library's transcendentals and can differ from NumPy's by a last bit, which
the test measures. `work/nr_frame` is `nr_frame.py` itself in C — the same flags (`--profile`, `--style-index`, `--local-tone`, `--local-structure`, `--skin-structure`, `--auto-mask`, `--control-mask`, `--intensity`, `--intensity-ladder`, `--detail-strength`, `--colour-strength`, `--detail-radius`, `--frame-index`, `--size`, `--weights`, `-v`), the same printed lines, PNG in and out through **libpng** rather than ImageMagick (any PNG in, 8-bit RGB out, the same byte codec; `--size` is this project's own bilinear resample rather than ImageMagick's filter, so a resized run differs from the Python's in the resampled pixels and nowhere else). `src/ref/nr_frame_native.py` binds the library for NumPy callers, and `work/test_nr_frame` is the frame test in C (`--reference` takes the head the Python test writes, so the byte-for-byte check runs without Python too). On an Apple M3
through MoltenVK a 1280x720 frame took 0.90 s when this was written (`notes/phase68`); the graph
alone replays in 0.47 s since `notes/improve-portable-precise.md`.

Two I/O switches, **both on by default since 2026-09-25**, in the Python path and in the C
frame library alike: `NR_INPUT_FP16` builds the
network's input as half, straight into the mapped input buffer, so neither a host copy
nor a GPU conversion pass is left; `NR_COMPACT_HEAD` reads back only the four useful
output channels, a quarter of the bytes. Neither can change a value. Set either to `0`
in the daemon's environment, not only the game's, to compare. On a discrete card the
compact head should matter more, since the read crosses PCIe; B570/B580 results are
still needed. For exact checks and paired benchmarks, see
[the I/O experiment notes](notes/improve-compact-io.md).

`NR_JOINT_QKV=1` is another optional experiment: it combines preparation of Q, K
and V into one dispatch, removing 140 passes per frame. It is **off by default**:
on 140V at network extent 448x320 it increased warm frame time from 79.1 to 83.2 ms.
See [the QKV experiment and diagnostic notes](notes/improve-joint-qkv.md) for exact
checks, the paired benchmark and profiling commands for B570/B580.

## Run it in a game

Two processes: a **daemon** that holds the model, and a **Vulkan layer** inside the game
that hands it each frame. They meet over a Unix socket on Linux and a named pipe
on Windows, where the setup window does all of what follows
([release quick start](docs/RELEASE-QUICKSTART.md)).

```sh
python3 src/layer/nr_daemon.py --settings /tmp/nr_settings.json
NR_LIVE=1 src/layer/nr-photo --steam <appid>   # prints the Steam launch option to paste
```

**Generate the launch option on your own machine, in your own clone, and paste what it
prints.** Do not copy one from this page, a forum post or somebody else's screen: it carries
the absolute path to *their* checkout, and on yours that path points at nothing, so the layer
silently never loads. The line looks like this, with your path in place of the placeholder:

```
VK_LAYER_PATH=/path/to/your/clone/work/layer VK_INSTANCE_LAYERS=VK_LAYER_dlssnr_intel ENABLE_NR_LAYER=1 NR_LAYER_SOCKET=/tmp/nr_layer.sock NR_LAYER_TRIGGER=/tmp/nr_trigger NR_LAYER_LIVE=1 %command%
```

`nr-photo` also writes the layer manifests into `work/layer` for you; if you move the clone,
run it again. Leave out `NR_LIVE=1` for photo mode. For a native Vulkan game,
`src/layer/nr-photo <game>` sets the environment itself.

| variable | what it does |
| --- | --- |
| `VK_LAYER_PATH` | where the layer's manifest is — **your** clone's `work/layer` |
| `ENABLE_NR_LAYER=1` | turn the layer on for this process |
| `NR_LAYER_SOCKET` | where the daemon listens. **Required, no default** — without it the layer never contacts the daemon at all. The daemon and the tools use `/tmp/nr_layer.sock` |
| `NR_LAYER_TRIGGER` | the file that means "do it". **Required for the toggle** — without it, live mode captures every frame whether the effect is on or not. The tools use `/tmp/nr_trigger` |
| `NR_LAYER_LIVE=N` | live mode: every Nth present goes through the network |
| `NR_LAYER_ASYNC=1` | with live mode: each processed present shows the answer for the one before it, so the daemon works while the game draws its next frame — more frames a second, one more frame of latency. Off by default; `NR_ASYNC=1 NR_LIVE=1 src/layer/nr-photo --steam <appid>` prints it into the launch option |
| `NR_LAYER_UI_MASK=1` | mark pixels that held still and leave them as the game drew them |
| `NR_LAYER_SYNC` | accepted for old launchers and ignored: there is one synchronisation path, described under **Capture synchronization** below |

**If your frame rate drops as soon as the game starts and the daemon's log shows no frames**,
one of the first four is missing or wrong: the layer is capturing and has nowhere to send it.

### The layer can start the daemon itself

With `NR_LAYER_SPAWN=1` set next to the variables above, and `NR_LAYER_SOCKET` named, the
layer starts the daemon on that socket if nothing is listening there when the game creates
its instance. Without the variable nothing happens and nothing prints, which is the
default: `vulkaninfo` and every other Vulkan process that loads the layer must not each
bring up a model. On Linux a daemon started this way **outlives the game** — it keeps its
buffers and the weights resident, **0.7 GiB at 720p and 1.2 GiB at 1920x1088, weights
included** — so the next launch connects instead of paying the load again. It ends on its
own when the GPU is lost, and otherwise runs until you end the process. **On Windows it
ends with the game that started it**: the layer hands it the game's process id. Steam counts
every process a game starts as part of the game, and a daemon that stayed kept the game
running for Steam after it had closed. Its settings and log follow
`src/layer/nr_paths.py` (`/tmp/nr_settings.json`, `NR_LAYER_LOG`), the same files `nr-ctl`
and `nr-panel` write, so the knobs reach a daemon started this way too.

`NR_PYTHON` names the interpreter to start it with, for an install the layer cannot guess
at. On Windows the default is the `py` launcher, not `python3`: that name belongs there to
the Microsoft Store's zero-byte alias, which `CreateProcessA` starts happily and which
then exits without running anything.

Without `NR_LAYER_LIVE` it is a **photo mode**: the pass fires once and holds its result
on screen while the trigger exists. With `NR_LAYER_LIVE=1` every present goes through the
network — 25-30 fps at 640x360 to 800x450 on the Arc 140V, beside the game; with a larger N
the presents between hold the last result.

## The three tools

All three drive the same two files: a trigger, and a JSON settings file the daemon
re-reads whenever it changes. Nothing reloads the model, so every knob below moves
between frames while the game runs.

### `nr-panel` — everything on one screen

```sh
src/layer/nr-panel
```

Arrow keys pick a knob and change it. The bottom of the screen is what the daemon is
actually doing, read from its log rather than estimated: extent, milliseconds, the
history gate, how much of the frame it is holding still. `space` turns the effect on and
off and starts the daemon if it is not up; `d` gives a knob back to the daemon's own
default; `q` leaves, and the game carries on.

It is curses, not a toolkit — nothing outside the standard library, and it works the same
over ssh.

### `nr-ctl` — the same thing, one shot

```sh
src/layer/nr-ctl                       an interactive session
src/layer/nr-ctl on                    start enhancing
src/layer/nr-ctl off                   hand the game back
src/layer/nr-ctl set scale 0.5
src/layer/nr-ctl help colour_strength  what a knob actually does
src/layer/nr-ctl rates                 measured frame times by extent and scale
src/layer/nr-ctl report                everything a bug report needs, in one paste
```

`report` is the one to run when the picture stops changing and you cannot tell why: it
prints the version, whether the daemon is listening, which GPU it took and where its buffers
went, every knob, the last frames with the time split, and — the part nobody thinks to look
for — the frames the daemon **refused**. A frame that fails is invisible while you play: the
game simply shows its own picture.

### `nr-toggle` — on and off, from a key

```sh
src/layer/nr-toggle            flip it
src/layer/nr-toggle temporal   flip the temporal path, to see the flicker it removes
src/layer/nr-toggle install    print the commands and open the key settings
```

Answers with a desktop notification, because a fullscreen game covers a terminal and does
not cover the compositor. Turning it on starts the daemon, so one key really is one key.

**Bind it in your desktop's own settings.** On Wayland an application cannot grab a key
for itself, and this tool used to register a Plasma global shortcut itself — which
crashed the compositor on the first keypress. `notes/phase55` has the backtrace and the
reason; the short version is that in Plasma 6.7 the shortcut registry lives inside KWin,
so anything that edits its config files behind its back is working on a corpse.

## The knobs

Every one of these is post-network or an extent choice: none invalidates the weights, and
all of them move between frames. Only `profile` costs a forward pass.

<!-- knobs:begin -->

### `render_scale` — the fraction of each side the network runs on

`0.05` to `1`, step `0.05`, default `1`

The only knob that changes the frame rate. The network draws its detail on a frame this much smaller; the detail is then scaled up and laid over the game's full-resolution frame, so the game's own pixels are never resampled. Lower is faster and draws coarser detail, at about the same strength. The network's frame is never smaller than 320 pixels a side (`min_extent`), so on a small window a low scale is raised, at no cost, to fill the cheapest frame it lands on: 640x360 runs as 0.5 for anything up to 0.5, 800x450 as 0.4 for anything up to 0.4, and a lower scale is never the slower one. For play, 0.35-0.6 is the useful range. For screenshots, 1.0 hands the network the game's raw pixels, jagged edges and all, and it turns a little of them into pixel-level grain; 0.9 smooths them first and draws the same strength of effect. Cost on an Arc 140V: about 130 ms per megapixel of network frame, plus 10-20 ms that grows with the game's own resolution.

### `min_extent` — the smallest side the network's frame is padded to

`128` to `320`, step `64`, default `320`

The network's frame is padded, by mirroring the picture, to at least this many pixels on a side. 320 is what NVIDIA's own driver does; the network itself runs down to 128. At small live sizes most of a 320 frame is padding, so a lower floor is much faster — on an Arc 140V, 512x288 at scale 0.35 takes 25 ms a frame at 320 and 14 at 128 — and draws a somewhat different picture, since the network no longer sees a mirrored copy of the scene around it. Neither is wrong; compare them in a game. It changes nothing once the scaled frame is larger than this anyway.

### `profile` — which way to trade skin texture against highlights, and its colour grade

`standard` / `natural` / `cinematic` / `neutral`

The style the network is asked for, and the colour grade that comes with it. The profiles are a trade, not a quality ladder: what one adds to skin and surface texture it takes from highlights. `standard` is the default and adds the most texture; `natural` and `cinematic` keep more of the highlights, and on very bright scenes `cinematic` can smooth fine detail rather than add it. After the network, as NVIDIA grades them, `natural` comes out a little darker, flatter and less saturated and `cinematic` less saturated. `neutral` all but switches the effect off. The profile is an input to the network, so it takes effect on the next frame the network draws; the knobs below act after it.

### `intensity` — how far to go towards the model's picture, or past it

`0` to `2`, step `0.05`, default `1`

Blends the model's picture with the game's own. 1 is the model's picture, 0 is the game's frame untouched, and values between are part of the way. Above 1 the blend goes past the model and exaggerates what it changed, which can look overdone. Free to change: nothing is re-run.

### `detail_strength` — how strongly to apply the fine detail the pass adds

`0` to `2`, step `0.05`, default `1`

The change the pass makes is split into fine detail — pores, hair, grain — and broad tone. This scales the fine part: 0 keeps only the tonal change, above 1 sharpens further, and past about 2 it over-sharpens. Away from 1 it costs one blur over the whole frame.

### `colour_strength` — how strongly to apply the pass's change of tone and colour

`0` to `2`, step `0.05`, default `1`

The broad half of the same split: how much of the pass's change in tone and colour is applied. It does not add colour back. The pass tends to calm bright, saturated areas, and this scales that change — so above 1 colours can look more washed out, not richer, and 0 keeps the game's own tone with the detail on top.

### `temporal` — how much of the previous frame to carry over

`0` to `1`, step `0.05`, default `1`

Each frame, the previous result is fed back to the network, which decides per pixel how much of it to keep; that is what keeps the picture from shimmering. This scales how much it keeps. 0 draws every frame on its own and forgets the stored frame, so turning it back on cannot bring back an old one. Costs a few per cent of the frame time.

### `hold` — how firmly to keep areas the game did not change

`0` to `1`, step `0.05`, default `1`

Where the game hands back exactly the same pixel as last frame, the previous result is still right for it, so it is kept at least this firmly. This is what stops still areas — backgrounds, a waiting character — from shimmering while something else moves. It cannot leave a trail: the first frame in which the game changes a pixel lets go of it.

### `release` — change, in levels of 255, that drops the previous frame where something moved

`0` to `64`, step `1`, default `24`

The other side of `hold`. Where the game's own pixel changed by this many levels of 255 or more, something moved there, and the previous result is dropped for that pixel; smaller changes keep a share that falls with the change. It is what prevents trails behind moving objects, since the layer has no motion vectors to follow them with. 16-24 is the useful range: lower drops more and the picture starts to shimmer over moving things, 0 turns it off and trails come back.

### `cut_limit` — how big a change between frames counts as a new scene

`0` to `1`, step `0.01`, default `0.15`

The average change between two frames above which the scene is taken to have cut — a camera cut, a menu, a replay — and the previous frame is dropped entirely instead of pixel by pixel. Lower cuts more readily; 1 never cuts.

<!-- knobs:end -->

## What to expect

<!-- rates:begin -->

Measured through the socket on 2026-09-27 by `python3 src/bench/live_rates.py` — the whole round trip a game waits for, median of nine frames, not graph time alone:

| swapchain | render scale | ms | fps |
| --- | ---: | ---: | ---: |
| 512x288 | 0.35 | 26 | 38.6 |
| 512x288 | 0.50 | 26 | 38.5 |
| 640x360 | 0.35 | 26 | 38.5 |
| 640x360 | 0.50 | 27 | 37.0 |
| 854x480 | 0.50 | 34 | 29.4 |
| 1024x768 | 0.55 | 56 | 17.7 |
| 1920x1080 | 0.55 | 112 | 8.9 |

Medians of six runs with swap empty; a row's runs spread up to 15 %. On 2026-09-23, with 5.5 GiB in zram and the kernel's memory-pressure figures rising, 1920x1080 ran anywhere from 322 to 463 ms: if that row is much slower for you, look at swap before anything else.

That is the daemon's own cost with nothing else on the GPU. A game adds its own frame to it: **Tekken 7** ran at **30 fps at 800x450** with the render scale at 0.35, and 27 at 0.6, in a live session on 2026-09-27 — against 10.5 fps at 640x360 on 2026-09-16 (`notes/phase59`).

<!-- rates:end -->

**Set the game small.** The cost follows the extent, and a stack of full-frame passes runs
at the *output* resolution regardless of the render scale, so the swapchain size matters
as much as the scale does. A game at 512x288 with the compositor stretching to the panel
is the fastest arrangement there is.

Inside the graph, the passes themselves are done: GEMM is register-bound, and every pass
that only moves data runs at the machine's memory ceiling. Tiling, operand staging, integer
weights, the accumulator format, OpenCL, shared-memory bank padding and handing work to the
E-cores have all been measured and all are closed (`notes/phase45`, `notes/phase46`). What
did move the graph was deleting passes: a pass at the memory ceiling that need not exist is
all waste. Folding the residuals into the projections, attention into one pass with its
head merge, Q/K normalisation and the window partition into the QKV projection's own
epilogue and loads, the narrow blocks' feed-forward into one kernel and the full-resolution
glue into fewer passes took a 1280x720 frame from 445 to 231 ms at the time, 48 %, with every
output bit-identical
(`notes/improve-fusions.md`, `notes/improve-qkv-epilogue.md`). The other thing that moved it
was shared memory, which decides how many workgroups a core holds: 128 KB between them,
each share rounded up to 1, 2, 4 ... KB. Window attention at 3104 bytes took 4 KB and so half
the core's threads, and at exactly 2 KB is 18 % faster; the staged GEMM at 15.5 KB took 16
and half the threads too (`notes/improve-shared-memory.md`, which also has a driver quirk
that makes some *smaller* declarations slower). With the one-pass window blocks and
bottleneck attention since, a full-scale 1280x720 frame is about 150 ms end to end and the
live sizes 25-27 ms. What bounds the GEMMs now is the register file: Xe2's 256-register mode
can be reached with a small Mesa patch, and with these kernels it is slower
(`notes/improve-large-grf.md`).

**On a small window, a low render scale costs no more than the frame it fills.** The network's
frame is never smaller than 320 a side, so a scale whose frame would be smaller is raised,
free, to fill the cheapest frame it lands on — 640x360 runs as 0.5 for anything up to 0.5, and
800x450 as 0.4. The daemon's log line says `runs as` when it happens.

## How it works

```
game ──presents──▶ Vulkan layer ──socket──▶ daemon ──▶ 71-block U-Net on XMX
  ▲                                                          │
  └──────────────── the composed frame ◀─────────────────────┘
```

- **The graph** is a symmetric U-Net: five Swin stages at 32/64/128/256/512 channels down
  to a ViT-1D bottleneck and back, 71 blocks, recovered from the DLL's intact RTTI,
  anchored on MLX-DLSS's independent extraction of the same binary, and corrected where it
  and the vendor's differ against OpenDLSS-NR, which claims the vendor's own arithmetic.
- **Every GEMM runs on XMX** in FP16 with FP32 accumulate, through cooperative matrix.
  The whole graph is resident: operands travel as 64-bit addresses in push constants, and
  activations never come back to the host.
- **The output is a *head***, not a picture — a residual the composition adds to the
  game's own frame. That is why the render scale can be lowered without resampling the
  game's pixels: only the synthesised part is interpolated.
- **A frame of history** is fed back into the network's own history channels, and the
  model's learned gate decides per pixel how much survives. That is what stops the
  picture shimmering.

**[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) is the recovered network**: the extent
rule, all sixteen input channels, the four-channel head and its composition, the block
table, the weight container and the trap in it, and what agreement with the original is
and is not possible. It is written for someone who wants to run this somewhere else, and
it is the file to read before any of the notes.

`notes/INDEX.md` maps every phase note to the question each one settles, and
several of the answers are counter-intuitive. `notes/HANDOFF.md` is the current state and the
traps.

## Troubleshooting

**The picture stops changing, and nothing looks broken.** Run `src/layer/nr-ctl report`.
A failing frame does not announce itself — the daemon writes a line and the game keeps its
own picture — so the two things to look at are the refused count and the last frames. Raising
the render scale is the usual cause: the buffers for a large extent may not fit, and then
every frame is refused with `(-2)`.

**"no daemon" / nothing happens.** `src/layer/nr-ctl status` says whether the daemon is
listening and whether the effect is on. They are independent: the trigger can be up with
no daemon, and a daemon can be idle with the trigger down.

**The game runs but the picture never changes.** The layer only attaches with
`ENABLE_NR_LAYER=1` in the *game's* environment. Steam launches the game as a child of
the client, so exporting it in your shell does not reach it — use the launch option
`src/layer/nr-photo --steam <appid>` prints, or launch Proton directly with `--proton`.

**A 32-bit game (D3D9 through DXVK) does not load the layer.** It needs the 32-bit
library, which `make` builds only where a 32-bit compiler and a 32-bit Vulkan loader are
installed; `make LAYER32=1` says what is missing. `prepare_layer.py` writes both manifests.

**It is unbearably slow.** Look at the swapchain size before the render scale. See the
table above; at 1920x1080 the daemon alone manages about 3 fps at full scale and 9 at 0.55,
and nothing will fix that but a smaller window.

**`GPU lost, stopping` in the daemon's log** — or, from a clone older than 2026-09-17,
`frame rejected/failed ... xmx_graph_run: resident submit (-4)` on every frame. `-4` is
`VK_ERROR_DEVICE_LOST`: the device went away under the daemon, normally the driver resetting
a hung GPU, and nothing on it can run again in that process. So the daemon says so once and
exits; turning the effect on again starts a fresh one. It has not been seen on the machine
this was built on, so the two likely causes are guesses. Either the GPU hung on a long
compute submission — likeliest at a large extent, so try a 640x360 window and render scale
0.35 first — or the driver's cooperative-matrix support on your GPU is not the one this was
built on: it is tested **only on an Intel Arc 140V (Lunar Lake, Xe2) with Mesa ANV**. If you
report it, the useful things are `vulkaninfo --summary`, your Mesa and kernel versions, the
extent and scale, and what `sudo dmesg | grep -iE 'xe|i915|hang|reset|guc'` says right after
it happens.

**It is far slower on a discrete Arc than the table says.** The daemon's log opens with
where its buffers went, which decides everything else:

```
model ready in 12.4s on Intel(R) Arc(tm) B580 Graphics (BMG G21)
buffers in card memory: type 1, heap 11.6 GiB, DEVICE_LOCAL HOST_VISIBLE …; readback cached: …
```

`card memory` is right, mapped or not. **`SYSTEM MEMORY ACROSS PCIE`** means the operands are
being read across the bus while the card's own memory sits unused, which costs about 50x — it
should not happen any more, so report it. Each frame line then ends with
`gpu <in>+<run>+<out>ms`: the host transfer in, the graph, the transfer out. If the outer two
dominate, the traffic is the problem; if the middle one does, the graph is. `XMX_STAGING=1`
and `=0` force the two memory paths for a comparison.

**Capture synchronization.** The layer waits on all semaphores supplied to the current
present, then waits for its own copy fences before the CPU reads or reuses the staging
buffer. It never waits on other application queues or recycles a semaphore still owned by
presentation. `NR_LAYER_SYNC=idle` and `=semaphore` remain accepted as legacy aliases for
this single path. The headless test uses separate queues and delayed writes; its negative
control must detect a layer with the wait deliberately removed. See
[the synchronization and MK1 check](notes/improve-present-fences.md).

**Direct Proton launch exits before rendering.** `nr-photo --proton` now supplies
`SteamAppId`, `SteamGameId` and `STEAM_COMPAT_APP_ID`, as Steam normally does. For a log:

```sh
PROTON_LOG=1 NR_PROTON=/path/to/Proton/proton src/layer/nr-photo --proton <appid> /path/to/game.exe
```

The launcher prints the runtime and log path (`work/proton-logs` by default).
`--check-proton` verifies paths, not a successful game launch. If several Proton installs
are found, select the one wanted with `NR_PROTON`, or launch through Steam.

**The game vanishes when loading characters or a level.** Check the kernel journal for
an OOM kill before diagnosing a GPU error. On a shared-memory iGPU, the game and every
resident daemon compete for the same RAM. During the MK1 check, a second daemon and a
1920x1200 swapchain exhausted memory; stopping the duplicate and using a smaller window
allowed real frames to be processed. A small render scale reduces the network's buffers,
but does not shrink the game's textures or all full-resolution host passes.

**The interface is being re-rendered.** `NR_LAYER_UI_MASK=1` marks pixels that did not
move between two presents and gives them back byte-identical. It drops itself when it
would cover more than 55 % of the frame, because that is the scene holding still rather
than a HUD.

## Layout

```
src/ref/      the CPU reference: the graph, features, composition, the temporal path
src/gpu/      the XMX runtime — compute shaders and the resident Vulkan context
src/layer/    the Vulkan layer, the daemon, and the three control tools
src/bench/    measurement programs; every number in the notes came from one
src/probe/    what your GPU can do, answered before anything else is built
src/tools/    the DLL and weight-container readers, and the checks that keep this
              page honest: what may be published, and what may still be claimed
docs/         the recovered architecture, written as a specification, and the setup guides
dist-tools/   what a release sets up with: the Windows setup window, setup.bat, setup.sh
scripts/      the release builder, DXVK's fetcher and the weight extractor
tools/        the Windows build (build_win.bat) and the release scripts (deploy.bat, deploy.sh)
notes/        what was measured, including the measurements that turned out wrong,
              and the working briefs the agents building this were given
work/         builds, checkouts and your weights. Ignored, and stays that way.
```

## Tests

```sh
make test
```

About 570 checks, including the layer's wire protocol, the native host passes
against the NumPy they replace byte for byte, the interface mask down to the
byte, the temporal path against MLX-DLSS's own composition, that float16 subnormals go
through every GEMM kernel on each runtime (`src/gpu/test_denorm.py`: declared on Vulkan,
measured on Metal), and the panel driven through
a pseudo-terminal. This page is checked too: the knob and frame-time tables are generated
from the same table the tools read, every `src/…` path here has to exist, and
`src/tools/claims_check.py` fails the suite if a withdrawn claim — a number this project
published and then corrected — turns up anywhere but the notes that withdrew it. They skip the weight-dependent parts if you have not supplied weights,
and a skip is not a pass.

## Credit and licences

**Apache License 2.0** — see [LICENSE](LICENSE) and [NOTICE](NOTICE).

The graph was recovered by [MLX-DLSS](https://github.com/iamwavecut/MLX-DLSS) (Apache-2.0)
from vendor captures; this port reads its weight specification and its numpy modules, and
the two independent extractions of the same DLL agree exactly — 0 missing, 0 extra, 0
shape mismatches. Where that graph and the vendor's differ — the decoder's skips, which
values the GEMMs read published, the bottleneck's own attention — it follows
[OpenDLSS-NR](https://github.com/maanHimself/OpenDLSS-NR) (MIT), which claims its network
bit-exact against captures of the original: `src/bench/opendlss_*` run its WebGPU port here
and compare, block by block and step by step (`notes/opendlss-reference.md`).

The Windows release carries [DXVK](https://github.com/doitsujin/dxvk) 3.1.1 under its zlib
licence (`dxvk\LICENSE` in the release), which setup puts beside DirectX 8–11 games.

DLSS, Neural Rendering and `nvngx_dlssnr.dll` are NVIDIA Corporation's. This is an
independent reimplementation of the inference pass, not affiliated with or endorsed by
them, and it contains no NVIDIA code. **No weights and no vendor binary are distributed
here.** `src/tools/publish_check.py` enforces that against the tracked tree on every
`make test`, and `make publish-check` walks the whole history as well — a deleted file
still ships with a repository.
