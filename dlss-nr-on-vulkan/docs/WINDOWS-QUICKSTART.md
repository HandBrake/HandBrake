# Windows quick start: from source to the first processed frame

This is the user guide for **64-bit Windows and an Intel Xe2 GPU**. Windows support
is experimental. Start with a small window and a game that already has a Vulkan
renderer. The effect adds frame time; its Linux FPS figures are not predictions
for your Windows driver.

These commands use **Command Prompt (`cmd.exe`)**, not PowerShell or Git Bash.
Follow them from one checkout. This guide is published on `improve`, which includes
the Windows port, its updates through `e616afc`, and the Windows manifest helper
used below. Use the checkout command here until this integration joins `master`.

The longer [Windows status page](WINDOWS.md) and [port history](WINDOWS-PORT.md)
are background material. This page gives one MSVC setup path; MSYS2 is not needed.

## 1. Check your hardware and install the tools

- **GPU:** Arc B570/B580 or a supported Lunar Lake Xe2 iGPU such as Arc 130V/140V.
  The kernels require an `fp16 × fp16 → fp32` cooperative-matrix configuration
  of **M=8, N=16, K=16**. Arc A-series is not supported by these kernels.
- Install the current graphics driver for your GPU from
  [Intel](https://www.intel.com/content/www/us/en/download-center/home.html).
- Install [Git for Windows](https://git-scm.com/download/win).
- Install a **64-bit Python 3** from [python.org](https://www.python.org/downloads/windows/).
  The Python install manager or the classic launcher can provide the `py` command.
- Install [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/)
  with **Desktop development with C++**, including the MSVC x64 tools and Windows SDK.
  Visual Studio Code alone does not provide the compiler.
- Install the Windows [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows).
  Reopen your terminal afterwards so `VULKAN_SDK` is available.

Use an ASCII checkout path such as `C:\dlss-nr`. Some shader/runtime paths still
go through Windows' ANSI file APIs, so avoid Cyrillic or other non-ASCII characters
in the checkout path.

## 2. Download the project and create its Python environment

In Command Prompt:

```bat
cd /d C:\
git clone --branch improve https://github.com/Uzbekunknown/dlss-nr-on-intel.git dlss-nr
cd /d C:\dlss-nr
set "PYTHONUTF8=1"
py -3 -m venv .venv
.venv\Scripts\python.exe -m pip install numpy safetensors
.venv\Scripts\python.exe -c "import struct; print('Python bits:', struct.calcsize('P') * 8)"
```

The last command must print **64**. All remaining Python commands use this same
environment, including the daemon started by the layer. You do not need CUDA,
PyTorch or the full MLX Python package for this route.

## 3. Supply the model weights

The repository contains code, not the model or NVIDIA's binaries. You need your
own `nvngx_dlssnr.dll` compatible with the recovered model. `nvngx_dlss.dll`
is a different file and is not a substitute.

Replace the example path with the DLL you have:

```bat
.venv\Scripts\python.exe scripts\get_weights.py "D:\MyFiles\nvngx_dlssnr.dll"
```

This downloads the pinned MLX-DLSS extractor's source and extracts/unpacks your
DLL locally. Success reports **649 logical tensors** and creates:

```text
work\mlxw\dlssnr-logical.safetensors
work\mlx-dlss\python\mlxdlss\features.py
```

Do not continue with an extraction error or a tensor-count warning. If you have
no suitable DLL, you cannot run the model yet. Do not attach the DLL or extracted
weights to bug reports.

## 4. Build the Windows libraries and shaders

Open **x64 Native Tools Command Prompt for Visual Studio** from the Start menu.
Use that prompt for this step so `cl` targets x64:

```bat
cd /d C:\dlss-nr
set "PYTHONUTF8=1"
echo %VULKAN_SDK%
where cl
tools\build_win.bat
```

The build must produce `work\libxmx.dll`, `work\nr_layer.dll`,
`work\libnr_image.dll`, `work\libnr_alloc.dll`, and the `.spv` shaders, including
`work\half_probe.spv`. It also builds `work\nr_layer32.dll` where the x86 MSVC
tools are installed; the first test below uses the x64 layer.
Stop if the output includes **ERROR** or a shader marked **FAILED**, even if the
script prints its final summary. Save the complete build output when reporting it.

Run the small GPU correctness test before trying a game:

```bat
.venv\Scripts\python.exe src\gpu\test_gemm_contract.py
```

If this fails, solve the GPU/driver/build issue first. Do not force an untested
shader variant or skip the half-rounding probe to hide the failure.

## 5. Prepare the layer and a small starting configuration

From `C:\dlss-nr`, in Command Prompt:

```bat
.venv\Scripts\python.exe src\layer\prepare_layer.py work\layer-win --platform windows
.venv\Scripts\python.exe -c "import json,pathlib; pathlib.Path('work/nr_settings.json').write_text(json.dumps(dict(render_scale=0.4,min_extent=320,profile='standard',intensity=1,detail_strength=1,colour_strength=1,temporal=1,hold=1,release=24),indent=2),encoding='utf-8')"
```

The helper creates `work\layer-win\VkLayer_dlss_nr.json` with the absolute path
to your x64 layer. There is no need to copy a DLL into a Windows system directory
or register a global layer. The settings file is read again when it changes.

## 6. Start with the Vulkan SDK cube

In the same Command Prompt, set the session's paths:

```bat
set "NR_ROOT=C:\dlss-nr"
set "NR_PYTHON=%NR_ROOT%\.venv\Scripts\python.exe"
set "PYTHONUTF8=1"
set "VK_LAYER_PATH=%NR_ROOT%\work\layer-win"
set "VK_INSTANCE_LAYERS=VK_LAYER_dlssnr_intel"
set "ENABLE_NR_LAYER=1"
set "NR_LAYER_SPAWN=1"
set "NR_LAYER_LIVE=1"
set "NR_LAYER_SOCKET=\\.\pipe\nr_dlssnr_intel_quickstart"
set "NR_LAYER_TRIGGER=%NR_ROOT%\work\nr_trigger"
set "NR_SETTINGS=%NR_ROOT%\work\nr_settings.json"
set "NR_LAYER_LOG=%NR_ROOT%\work\nr_daemon.log"
start "" "%VULKAN_SDK%\Bin\vkcube.exe"
```

The layer starts the daemon automatically. The effect is **off while the trigger
file is absent**. In the original Command Prompt:

```bat
type nul > "%NR_LAYER_TRIGGER%"
```

That turns live processing on. To turn it off:

```bat
del "%NR_LAYER_TRIGGER%"
```

Look at the log and status:

```bat
type "%NR_LAYER_LOG%"
"%NR_PYTHON%" src\layer\nr-ctl report
```

You should see `model ready`, `listening on` the named pipe, then frame lines
containing an extent, `change`, and `gpu ...ms`. A visible cube with no frame
lines does not prove the effect is working. A refused frame leaves the game's
original picture in place.

Close the cube and remove the trigger before starting a game. The daemon remains
running and can be reused by the game on the same pipe.

## 7. Start a game in its Vulkan mode

For a first game test, choose a **64-bit game with its own Vulkan renderer**.
For example, DOOM (2016) has a Vulkan graphics-API option in Advanced Settings.
Set a small window, such as 800x450 if the game offers it, before enabling NR.

From the same configured Command Prompt, replace both example paths:

```bat
start "" /D "D:\Games\MyGame" "D:\Games\MyGame\MyGame.exe"
```

Starting from Explorer or an ordinary Steam launch does not inherit this prompt's
environment. If a Steam game restarts itself through Steam and the effect never
produces frame lines, completely exit Steam, start it from this configured prompt,
then launch the game from its library:

```bat
start "" "C:\Program Files (x86)\Steam\steam.exe"
```

Use Steam's actual install path if it differs. Keep only the test game running:
these variables also apply to Vulkan applications started by this Steam session.
After testing, remove the trigger and restart Steam normally to clear that session's
layer environment. A release's setup window needs none of this: the `vulkan-1.dll` it
puts beside the game gives the game's own process these variables, however the game
is started ([RELEASE-QUICKSTART.md](RELEASE-QUICKSTART.md)).

Turning NR on/off uses the same trigger commands as the cube. Use the reported
frame timings and refusal messages to confirm it is active, not only a change in FPS.

### What about DirectX games?

The layer intercepts Vulkan presents. A DirectX game does not become compatible
just because the NR DLL exists on disk.

Some D3D9/10/11 games can render through [DXVK](https://github.com/doitsujin/dxvk).
Its DLLs must match the game's API and executable architecture. The **game's**
architecture decides this; a 64-bit Windows installation can run a 32-bit game.
Back up existing game-local DLLs before replacing any, and start with an offline
single-player test. This quick start does not install DXVK into a game for you;
a release's setup window does, for 64-bit and 32-bit games, and takes it out again
with **Remove NR** ([RELEASE-QUICKSTART.md](RELEASE-QUICKSTART.md)).

This integrated build can also prepare an x86 manifest when `nr_layer32.dll` is
available. Games such as Dead or Alive 5 need that x86 layer and matching DXVK
DLLs; follow the [Windows status page](WINDOWS.md) for the 32-bit route. Do not load
an x64 NR layer into a 32-bit game. The Python daemon remains 64-bit.
There is no D3D12 route on Windows yet: VKD3D-Proton, with DXVK's DXGI and D3D11 beside
it, stopped Mortal Kombat 1 at startup on 101.9033. Nor is there an OptiScaler
motion-vector path.

## Common problems

| Symptom | What to check |
| --- | --- |
| `cl` not found | Use the x64 Native Tools prompt and install the C++ workload, not only VS Code. |
| `VULKAN_SDK` is empty | Install the Windows Vulkan SDK and reopen the prompt. |
| Store opens instead of Python | Use the explicit `.venv\Scripts\python.exe` path; verify Python was installed. |
| `No module named numpy` or `safetensors` | Install into the same `.venv` named by `NR_PYTHON`. |
| `WinError 193` | Check x64 Python and x64 DLLs; this commonly indicates an architecture mismatch. |
| `WinError 126`, or a DLL exists but cannot load | Check its dependencies, including the x64 [Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist), `vulkan-1.dll` and the MSVC OpenMP runtime. |
| `cannot open spv` | Rebuild the shaders in the checkout named by `NR_ROOT`; check build failures. |
| `model ready` never appears | Read `work\nr_daemon.log`; check weights, Python, DLL dependencies and the half probe. |
| Layer loaded, but no processed-frame lines | Check the actual game's Vulkan mode, the inherited launch environment, named-pipe name and trigger file. |
| `GPU lost`, submit/fence error `-4` | Stop this daemon and restart after checking the driver/build and lowering the test resolution. An exited/lost device cannot recover inside the same daemon process. |
| Very low FPS | Reduce the game's window resolution first; compare the logged GPU time and whether native image passes loaded. Linux FPS numbers are not a Windows promise. |

For shader/GPU failures, keep the defaults. In particular, do not enable an
unmerged attention kernel as a workaround: the Windows status page documents a
driver hang in a test variant.

## Report a result or update the checkout

From the same configured prompt, collect:

```bat
git rev-parse --short HEAD
"%NR_PYTHON%" src\layer\nr-ctl report
"%VULKAN_SDK%\Bin\vulkaninfo.exe" --summary
```

Include GPU/CPU, Windows and Intel driver versions, game/API, window resolution,
render scale, `min_extent`, FPS with NR off/on, and the failing step. Attach the
relevant log section rather than the DLL or model weights.

Before rebuilding, close the test game and stop the NR daemon shown in your
process list. Do not stop every Python process. Then `git pull`, rebuild with
`tools\build_win.bat`, and regenerate the manifest. A running daemon keeps the
old loaded DLLs and shader pipelines; updating files in another checkout does
not update it.

The layer does not provide a stop button for its spawned daemon yet. To find it
in PowerShell, inspect `Get-CimInstance Win32_Process` for the `nr_daemon.py`
command containing this checkout and pipe, then stop that specific process.
Removing the trigger turns the effect off but leaves the model resident.

`dist-tools\setup.bat` is for a prepared binary distribution, not a substitute
for building a source clone. `tools\deploy.bat` is an alternative deployment
workflow; this guide intentionally keeps the runtime and weights in one checkout.

Validation of this guide: the Python manifest generation and launcher regressions
were checked on Linux with Windows DLL fixtures. The MSVC build and Windows game
steps are based on the current scripts and recorded Windows-port tests; this guide
was not itself run end to end on a Windows machine in this documentation change.
