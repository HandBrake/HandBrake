@echo off
setlocal
rem ===========================================================================
rem  deploy.bat  -  run DLSS-NR on Intel against a game (Windows)
rem
rem  One script for the whole setup: weights, build, install.
rem
rem    1. weights  - runs scripts/get_weights.py against a DLL you supply (--dll), which
rem                  clones MLX-DLSS at its pinned commit and writes
rem                  work/mlxw/dlssnr-logical.safetensors. Skipped with --skip-weights,
rem                  or when the file is already there.
rem    2. build    - tools/build_win.bat builds libxmx.dll, nr_layer.dll and the shaders.
rem    3. install  - copies the layer into the game folder under a name you choose, writes
rem                  a manifest whose library_path is that file's absolute path, copies the
rem                  runtime the daemon needs, and writes a launcher.
rem
rem  What it deliberately does not do:
rem
rem    * It does NOT copy the model weights anywhere. The launcher points the daemon at
rem      this checkout with NR_ROOT, so the weights stay in work/mlxw/ and nothing large
rem      can be passed on by accident with the game folder. Extract them wherever this
rem      tree is; the game folder only needs to know where to look.
rem    * It DOES copy the runtime the daemon needs to start: work/mlx-dlss (the MLX
rem      extractor the daemon imports), libxmx.dll, libnr_image.dll and the shaders.
rem      Without them the daemon stops at start.
rem
rem  Why the manifest matters: the Vulkan loader finds a layer through its manifest
rem  plus VK_LAYER_PATH, not through the DLL search order, so the manifest has to name
rem  the file and VK_LAYER_PATH has to point at the folder holding both. That is also
rem  why the default name is NOT version.dll: on Windows a module called version.dll,
rem  once loaded, is what later imports of version.dll resolve to, so it can silently
rem  stand in for the system library. The manifest makes the name irrelevant anyway.
rem
rem  Usage:
rem    deploy.bat --game "C:\Games\MyGame" --dll X:\path\nvngx_dlssnr.dll
rem              [--name nr_layer.dll] [--exe "C:\Games\MyGame\game.exe"]
rem              [--skip-weights] [--skip-build]
rem    deploy.bat --release "dist\dlss-nr-windows" [--skip-weights] [--skip-build]
rem ===========================================================================

set "GAME="
set "DLL="
set "NAME=nr_layer.dll"
set "EXE="
set "RELEASE="
set "SKIP_WEIGHTS=0"
set "SKIP_BUILD=0"
rem Capture the script directory BEFORE any shift: shift moves %0 and %~dp0 with it.
set "TOOLSDIR=%~dp0"

:parse
if "%~1"=="" goto :done_parse
if /i "%~1"=="--game"         ( set "GAME=%~2" & shift & shift & goto :parse )
if /i "%~1"=="--dll"          ( set "DLL=%~2"  & shift & shift & goto :parse )
if /i "%~1"=="--name"         ( set "NAME=%~2" & shift & shift & goto :parse )
if /i "%~1"=="--exe"          ( set "EXE=%~2"  & shift & shift & goto :parse )
if /i "%~1"=="--skip-weights" ( set "SKIP_WEIGHTS=1" & shift & goto :parse )
if /i "%~1"=="--skip-build"   ( set "SKIP_BUILD=1" & shift & goto :parse )
if /i "%~1"=="--release"      ( set "RELEASE=%~2" & shift & shift & goto :parse )
echo Unknown argument: %~1
exit /b 2
:done_parse

set "REPO=%TOOLSDIR%.."
rem Resolve the .. so NR_ROOT and every path below carry no relative segment
pushd "%REPO%" >nul 2>&1
set "REPO=%CD%"
popd
set "WORK=%REPO%\work"

rem --release assembles a folder for other people and installs into nobody's game, so it
rem is the one mode that does not need --game.
if not "%RELEASE%"=="" goto :release_ok
if "%GAME%"=="" (
  echo ERROR: --game is required, the game directory to install into
  echo        ^(or --release ^<folder^> to assemble a distributable instead^)
  exit /b 2
)
if not exist "%GAME%" (
  echo ERROR: game directory not found: "%GAME%"
  exit /b 2
)
:release_ok

rem ---- weights: delegated to scripts/get_weights.py ----
rem It clones MLX-DLSS at the pinned commit, runs its extractor against your DLL, and
rem checks that 649 tensors came out. Reusing it rather than repeating the two commands
rem here keeps one definition of "the weights are present and correct".
if "%SKIP_WEIGHTS%"=="1" goto :after_weights
if exist "%WORK%\mlxw\dlssnr-logical.safetensors" (
  echo [1/3] weights already at %WORK%\mlxw\dlssnr-logical.safetensors
  goto :after_weights
)
if "%DLL%"=="" (
  echo ERROR: no weights yet and no --dll given.
  echo        Pass --dll X:\path\to\nvngx_dlssnr.dll, or --skip-weights if you have them.
  exit /b 2
)
if not exist "%DLL%" (
  echo ERROR: DLL not found: "%DLL%"
  exit /b 2
)
echo [1/3] extracting weights with scripts\get_weights.py ...
set "PY="
if defined NR_PYTHON set "PY=%NR_PYTHON%"
if not defined PY ( where /q py && set "PY=py" )
if not defined PY ( where /q python3 && set "PY=python3" )
if not defined PY ( where /q python && set "PY=python" )
if not defined PY (
  echo ERROR: python3 not found on PATH
  exit /b 3
)
"%PY%" "%REPO%\scripts\get_weights.py" "%DLL%"
if errorlevel 1 (
  echo ERROR: weight extraction failed
  exit /b 3
)
:after_weights

rem ---- build: delegated to build_win.bat, which also builds libxmx.dll and the shaders ----
rem Kept in one place on purpose: deploy only has to know that the runtime exists, and
rem a second copy of the MSVC invocation here is a second thing to get wrong.
if "%SKIP_BUILD%"=="1" goto :after_build
echo [2/3] building with build_win.bat ...
if not exist "%TOOLSDIR%build_win.bat" (
  echo ERROR: tools\build_win.bat not found
  exit /b 3
)
call "%TOOLSDIR%build_win.bat"
if errorlevel 1 (
  echo ERROR: the build failed
  exit /b 3
)
:after_build
if not exist "%WORK%\nr_layer.dll" (
  echo ERROR: %WORK%\nr_layer.dll is missing; build it first or drop --skip-build
  exit /b 3
)
if not exist "%WORK%\libxmx.dll" (
  echo WARNING: %WORK%\libxmx.dll is missing - the daemon cannot start without it
)

rem ---- release folder, or the game ----
rem A release is the same assembly as a game install without the game: everything
rem dist-tools/setup.bat needs to run on a machine that has no compiler, no Vulkan SDK and
rem no clone. The one thing that differs is where the layer goes - a release folder carries
rem nr_layer.dll at its top level, because that is what setup.bat looks for, and the game
rem install puts it in dlss-nr\ under the name the user chose.
if not "%RELEASE%"=="" goto :release

rem ---- install into the game folder ----
echo [3/3] installing into "%GAME%" ...
set "DEPLOY=%GAME%\dlss-nr"
set "DEST_NOTE="
if not exist "%DEPLOY%" mkdir "%DEPLOY%"

copy /Y "%WORK%\nr_layer.dll" "%DEPLOY%\%NAME%" >nul || exit /b 3
echo   layer:      "%DEPLOY%\%NAME%"

set "MANIFEST_SRC=%REPO%\src\layer\VkLayer_dlss_nr.json"
set "MANIFEST_DST=%DEPLOY%\VkLayer_dlss_nr.json"
if not exist "%MANIFEST_SRC%" (
  echo ERROR: manifest template not found: %MANIFEST_SRC%
  exit /b 3
)
rem An ABSOLUTE library_path. A bare name makes the loader fail with error 87
rem (ERROR_INVALID_PARAMETER) when the layer is found through VK_LAYER_PATH: measured
rem here, the same manifest loads with the full path and not without it.
powershell -NoProfile -Command "$lib = (Join-Path '%DEPLOY%' '%NAME%'); (Get-Content '%MANIFEST_SRC%') -replace 'LIBRARY_PATH_PLACEHOLDER', $lib.Replace('\','\\') | Set-Content '%MANIFEST_DST%'"
echo   manifest:   "%MANIFEST_DST%"  library_path = "%DEPLOY%\%NAME%"

rem The runtime the daemon needs at start: the MLX extractor it imports, the resident
rem Vulkan runtime, and the shaders. Without these the daemon stops before it listens.
rem The weights are NOT copied - the launcher points NR_ROOT at this checkout instead,
rem so nothing large can travel with the game folder.
if not exist "%DEPLOY%\src" mkdir "%DEPLOY%\src"
if not exist "%DEPLOY%\work" mkdir "%DEPLOY%\work"
xcopy /E /I /Y /Q "%REPO%\src\layer" "%DEPLOY%\src\layer" >nul
xcopy /E /I /Y /Q "%REPO%\src\ref"   "%DEPLOY%\src\ref"   >nul
xcopy /E /I /Y /Q "%REPO%\src\gpu"   "%DEPLOY%\src\gpu"   >nul
xcopy /E /I /Y /Q "%REPO%\src\bench" "%DEPLOY%\src\bench" >nul

if exist "%WORK%\mlx-dlss" (
  xcopy /E /I /Y /Q "%WORK%\mlx-dlss" "%DEPLOY%\work\mlx-dlss" >nul
  echo   runtime:    work\mlx-dlss
) else (
  echo   WARNING: %WORK%\mlx-dlss is missing; the daemon cannot start without it
)
if exist "%WORK%\libxmx.dll"    copy /Y "%WORK%\libxmx.dll"    "%DEPLOY%\work\" >nul
if exist "%WORK%\libnr_image.dll" copy /Y "%WORK%\libnr_image.dll" "%DEPLOY%\work\" >nul
if exist "%WORK%\libnr_alloc.dll" copy /Y "%WORK%\libnr_alloc.dll" "%DEPLOY%\work\" >nul
copy /Y "%WORK%\*.spv" "%DEPLOY%\work\" >nul 2>&1
echo   runtime:    libxmx.dll, libnr_image.dll, libnr_alloc.dll, shaders

rem Keep local junk out of the deployed tree
for /d /r "%DEPLOY%\src" %%J in (__pycache__) do @if exist "%%J" rmdir /S /Q "%%J" 2>nul
del /S /Q "%DEPLOY%\src\*.obj" >nul 2>&1
del /S /Q "%DEPLOY%\src\*.lib" >nul 2>&1
del /S /Q "%DEPLOY%\src\*.exp" >nul 2>&1

rem Drop local junk the copy may have brought along
for /d /r "%DEPLOY%\src" %%J in (__pycache__) do @if exist "%%J" rmdir /S /Q "%%J" 2>nul
del /S /Q "%DEPLOY%\src\*.obj" >nul 2>&1
del /S /Q "%DEPLOY%\src\*.lib" >nul 2>&1
del /S /Q "%DEPLOY%\src\*.exp" >nul 2>&1

rem ---- launcher ----
set "LAUNCH=%DEPLOY%\launch-nr.bat"
(
  echo @echo off
  echo setlocal
  echo.
  echo rem Launcher for DLSS-NR on Intel.
  echo rem The loader finds the layer through VK_LAYER_PATH plus the manifest, so the
  echo rem library name above does not matter; the manifest names it.
  echo set "VK_LAYER_PATH=%DEPLOY:^)=^)%"
  echo set "VK_INSTANCE_LAYERS=VK_LAYER_dlssnr_intel"
  echo set "ENABLE_NR_LAYER=1"
  echo set "NR_LAYER_SPAWN=1"
  echo set "NR_LAYER_LIVE=1"
  echo rem Windows talks to the daemon over a named pipe, not a Unix socket.
  echo set "NR_LAYER_SOCKET=\\.\pipe\nr_dlssnr_intel"
  echo rem The weights stay in the checkout; only this path points at them.
  echo set "NR_ROOT=%REPO%"
  echo.
) > "%LAUNCH%"
if defined EXE (
  echo start "" "%EXE%" %%* >> "%LAUNCH%"
) else (
  echo set "GAME_EXE=GAME_EXE_NOT_SET" >> "%LAUNCH%"
  echo if "%%GAME_EXE%%"=="GAME_EXE_NOT_SET" ^( echo Set GAME_EXE in this file ^& exit /b 1 ^) >> "%LAUNCH%"
  echo start "" "%%GAME_EXE%%" %%* >> "%LAUNCH%"
)
echo   launcher:   %LAUNCH%

echo.
echo Done. Install folder: "%DEPLOY%"
echo   %NAME%, VkLayer_dlss_nr.json, src\, work\ (runtime only - no weights)
echo.
echo Weights stay in %WORK%\mlxw; the launcher points NR_ROOT at the checkout.
echo Run the game through %LAUNCH% so VK_LAYER_PATH is set.
echo.
echo COMPLIANCE: nothing NVIDIA's is shipped. The weights come from a DLL you own and
echo are never copied into the game folder.
endlocal
exit /b 0

rem ==== :release - common checked assembly; no game installation ====
:release
echo [3/3] assembling a release into "%RELEASE%" ...
set "PY="
if defined NR_PYTHON set "PY=%NR_PYTHON%"
if not defined PY ( where /q py && set "PY=py" )
if not defined PY ( where /q python3 && set "PY=python3" )
if not defined PY ( where /q python && set "PY=python" )
if not defined PY (
  echo ERROR: Python 3 is required to assemble the release
  exit /b 3
)
rem DXVK for DirectX 8-11 games: the pinned release, downloaded once into work\dxvk and
rem checked against its SHA-256 (dist-tools\dxvk.json).
"%PY%" "%REPO%\scripts\fetch_dxvk.py"
if errorlevel 1 exit /b 3
"%PY%" "%REPO%\scripts\build_release.py" "%RELEASE%" --platform windows
if errorlevel 1 exit /b 3
endlocal
exit /b 0
