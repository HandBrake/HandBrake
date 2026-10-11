@echo off
setlocal
rem ===========================================================================
rem  build_win.bat  -  build the Windows runtime, layer and shaders into work/
rem
rem  All with MSVC:
rem    work\libxmx.dll      the resident Vulkan runtime the daemon drives
rem    work\nr_layer.dll    the Vulkan layer a 64-bit game loads
rem    work\nr_layer32.dll  the same layer for a 32-bit game, if the x86 tools are there
rem    work\libnr_image.dll the host passes around the network
rem    work\libnr_alloc.dll the daemon's NumPy allocator, which keeps a frame's large blocks
rem    work\*.spv           the compute shaders
rem
rem  Two things here are not obvious and both cost an afternoon if missed:
rem
rem    * libxmx.c needs /std:c11. It uses _Static_assert, which MSVC's default C
rem      dialect does not accept - the build fails with a page of syntax errors
rem      pointing at the asserts rather than at the flag.
rem    * MSVC exports nothing from a DLL unless told to. libxmx's export list is
rem      generated from the source (every non-static xmx_* definition) so a symbol
rem      added later cannot silently go missing from the DLL, which is a failure
rem      the Python side only reports as "function 'xmx_...' not found".
rem
rem  Usage:  build_win.bat            build everything
rem          build_win.bat --layer    only the layer (fast; what deploy needs)
rem          build_win.bat --shaders  only the shaders
rem ===========================================================================

set "ONLY="
if /i "%~1"=="--layer"   set "ONLY=layer"
if /i "%~1"=="--shaders" set "ONLY=shaders"

set "REPO=%~dp0.."
pushd "%REPO%" >nul 2>&1
set "REPO=%CD%"
popd
set "WORK=%REPO%\work"
set "GPU=%REPO%\src\gpu"

if not defined VULKAN_SDK (
  echo ERROR: VULKAN_SDK is not set. Install the Vulkan SDK and reopen the prompt.
  exit /b 1
)
if not exist "%WORK%" mkdir "%WORK%"

rem ---- locate MSVC ----
rem Stays OUTSIDE any ( ) block: a variable set inside a block is not visible to a
rem test later in the same block, and redirection inside a block is not performed at
rem all. Both fail silently and leave cl missing.
set "VSROOT="
where cl >nul 2>nul
if not errorlevel 1 goto :have_msvc
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" "%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%TEMP%\nr_vsroot.txt" 2>nul
if exist "%TEMP%\nr_vsroot.txt" set /p VSROOT=< "%TEMP%\nr_vsroot.txt"
del /Q "%TEMP%\nr_vsroot.txt" >nul 2>&1
if not defined VSROOT (
  echo ERROR: no MSVC found. Run from a "x64 Native Tools Command Prompt for VS".
  exit /b 1
)
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
:have_msvc

rem ---- the shader and runtime stages, flat: no ( ) block around a set/if pair ----
if "%ONLY%"=="layer" goto :layer

rem ---- shaders ----
echo [1/6] compiling shaders ...
set "GLSL=%VULKAN_SDK%\Bin\glslangValidator.exe"
if not exist "%GLSL%" (
  echo ERROR: glslangValidator not found in the Vulkan SDK
  exit /b 1
)
rem The Makefile's shader table, transplanted. Compiling "every .comp to its own .spv"
rem missed the three aliases the daemon asks for by a name no source file carries
rem (gemm_batched.spv from gemm_coopmat_batched.comp, gemm_f16acc.spv from
rem gemm_coopmat_f16acc.comp, gemm_tiled.spv from gemm_resident.comp), and a missing one
rem fails at run time with "cannot open spv", which names the file but not the list.
rem Same includes as the Makefile's GEMM_GLSL. Nothing is defined per driver here: libxmx
rem chooses half_round's spelling when it builds each pipeline (publish.glsl's constant 1).
for %%C in ("%GPU%\*.comp") do call :shader %%~nC
call :shader gemm_coopmat_batched
call :shader gemm_coopmat_f16acc
call :shader_def gemm_batched gemm_coopmat_batched ""
call :shader_def gemm_f16acc  gemm_coopmat_f16acc  ""
call :shader_def gemm_tiled   gemm_resident        "-DRM=2 -DRN=2"
call :shader_def gemm_staged32      gemm_staged "-DSTAGED_BM=32"
call :shader_def gemm_staged32_deep gemm_staged "-DSTAGED_BM=32" "-DSTAGED_BK=64"
call :shader_def attention_rows     attention   "-DROW_LANES=256"
call :shader_def attention_ab       attention   "-DSOFTMAX_AB"
rem The daemon's start-up probe, whose source is a bench tool rather than a graph shader.
"%GLSL%" --target-env vulkan1.3 -I"%GPU%" -o "%WORK%\half_probe.spv" "%REPO%\src\bench\half_probe.comp" >nul 2>&1
if exist "%WORK%\half_probe.spv" (echo   half_probe.spv) else (echo   half_probe.spv FAILED)

rem ---- libxmx.dll ----
echo [2/6] building libxmx.dll ...
rem Export list from the source: every non-static xmx_* definition, so a symbol added
rem later cannot silently go missing from the DLL.
powershell -NoProfile -Command "$s = Get-Content '%GPU%\libxmx.c' -Raw; $m = [regex]::Matches($s, '(?m)^(?!static)[^\n]*?\b(xmx_\w+)\s*\([^;]*\)\s*\{') | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique; @('LIBRARY libxmx','EXPORTS') + ($m | ForEach-Object { '    ' + $_ }) | Set-Content '%WORK%\libxmx.def'"
rem /std:c11 is required: libxmx.c uses _Static_assert, and MSVC's default C dialect
rem rejects it with syntax errors pointing at the asserts, not at the missing flag.
cl /nologo /O2 /std:c11 /TC /D_WIN32 /I"%VULKAN_SDK%\Include" /I"%GPU%" /Fo:"%WORK%\libxmx.obj" /Fe:"%WORK%\libxmx.dll" /LD "%GPU%\libxmx.c" /link /DEF:"%WORK%\libxmx.def" "%VULKAN_SDK%\Lib\vulkan-1.lib"
if errorlevel 1 ( echo ERROR: libxmx.dll failed & exit /b 1 )
echo   libxmx.dll

:layer
if "%ONLY%"=="shaders" goto :done
rem The layer calls no Vulkan function by name: the loader hands it every pointer, so it
rem links no vulkan-1.lib, and its exports are its own functions (nr_layer.def).
echo [3/6] building nr_layer.dll ...
cl /nologo /O2 /TC /D_WIN32 /I"%VULKAN_SDK%\Include" /I"%REPO%\src\layer" /Fo:"%WORK%\nr_layer.obj" /Fe:"%WORK%\nr_layer.dll" /LD "%REPO%\src\layer\nr_layer.c" /link /DEF:"%REPO%\src\layer\nr_layer.def"
if errorlevel 1 ( echo ERROR: nr_layer.dll failed & exit /b 1 )
echo   nr_layer.dll
rem What setup puts beside a game as vulkan-1.dll: it sets the layer's environment in the
rem game's own process, so NR is there however the game is started (nr_vulkan_proxy.c).
cl /nologo /O2 /TC /D_WIN32 /I"%VULKAN_SDK%\Include" /I"%REPO%\src\layer" /Fo:"%WORK%\nr_vulkan_proxy.obj" /Fe:"%WORK%\nr_vulkan_proxy.dll" /LD "%REPO%\src\layer\nr_vulkan_proxy.c" /link /DEF:"%REPO%\src\layer\nr_vulkan_proxy.def"
if errorlevel 1 ( echo ERROR: nr_vulkan_proxy.dll failed & exit /b 1 )
echo   nr_vulkan_proxy.dll

rem A 32-bit game - Dead or Alive 5, through DXVK's 32-bit d3d9.dll - loads a 32-bit layer;
rem the daemon it talks to stays 64-bit, on the same pipe. The x86 cross tools come with the
rem C++ workload; they run in a shell of their own so the x86 environment ends with them.
echo [4/6] building nr_layer32.dll ...
set "VCX86=%VCINSTALLDIR%Auxiliary\Build\vcvarsamd64_x86.bat"
if not exist "%VCX86%" (
  echo   skipped: no x86 tools ^(%VCX86%^)
  goto :image
)
cmd /c ""%VCX86%" >nul && cl /nologo /O2 /TC /D_WIN32 /I"%VULKAN_SDK%\Include" /I"%REPO%\src\layer" /Fo:"%WORK%\nr_layer32.obj" /Fe:"%WORK%\nr_layer32.dll" /LD "%REPO%\src\layer\nr_layer.c" /link /DEF:"%REPO%\src\layer\nr_layer.def" /MACHINE:X86"
if errorlevel 1 ( echo ERROR: nr_layer32.dll failed & exit /b 1 )
echo   nr_layer32.dll
cmd /c ""%VCX86%" >nul && cl /nologo /O2 /TC /D_WIN32 /I"%VULKAN_SDK%\Include" /I"%REPO%\src\layer" /Fo:"%WORK%\nr_vulkan_proxy32.obj" /Fe:"%WORK%\nr_vulkan_proxy32.dll" /LD "%REPO%\src\layer\nr_vulkan_proxy.c" /link /DEF:"%REPO%\src\layer\nr_vulkan_proxy.def" /MACHINE:X86"
if errorlevel 1 ( echo ERROR: nr_vulkan_proxy32.dll failed & exit /b 1 )
echo   nr_vulkan_proxy32.dll

:image

rem The host-side passes around the network. Without it those passes fall back to NumPy,
rem which at 4K is where most of a frame goes. /fp:precise matters: the file is required
rem to be byte-identical to the NumPy it transcribes, and fast maths moves the rounding
rem points. The export list is generated for the same reason as libxmx's.
rem
rem /TP and /openmp together, and both are needed: MSVC's /openmp rejects a #pragma omp
rem parallel for in C mode outright (C3015 on the canonical sample, in /TC), and compiles
rem it in C++ mode. So this one file goes through the C++ front end. The only C-isms that
rem costs are explicit casts from malloc, added for it. Without OpenMP every host pass runs
rem on one core: 212-216 ms of replayed graph at 720p against 191-195 with gcc's OpenMP.
rem
rem NR_F16C: MSVC has no _Float16, and with this nr_image.c converts each half with F16C's
rem one instruction instead of the arithmetic, the same bits for every float. F16C is part of
rem the x86-64-v3 floor the Makefile and CMake build for, every CPU since Haswell and Zen 1.
rem On the Arc 140V the features pass goes from 1.0-2.9 ms to 0.4-0.8 at 720p and 1080p.
rem Not /arch:AVX2: MSVC takes the intrinsics without it, and under it the composition,
rem which MSVC does not vectorise either way, was 0.1-0.4 ms slower.
echo [5/6] building libnr_image.dll ...
powershell -NoProfile -Command "$s = Get-Content '%REPO%\src\ref\nr_image.c' -Raw; $m = [regex]::Matches($s, '(?m)^(?!static)[^\n]*?\b(nr_\w+)\s*\([^;]*\)\s*\{') | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique; @('LIBRARY libnr_image','EXPORTS') + ($m | ForEach-Object { '    ' + $_ }) | Set-Content '%WORK%\libnr_image.def'"
cl /nologo /O2 /TP /D_WIN32 /D_CRT_SECURE_NO_WARNINGS /fp:precise /openmp /DNR_F16C /Fo:"%WORK%\nr_image.obj" /Fe:"%WORK%\libnr_image.dll" /LD "%REPO%\src\ref\nr_image.c" /link /DEF:"%WORK%\libnr_image.def"
if errorlevel 1 ( echo ERROR: libnr_image.dll failed & exit /b 1 )
echo   libnr_image.dll

rem The daemon's NumPy allocator. Windows' heap returns a frame's large arrays to the system
rem as they are freed, and the next frame pays a page fault for every 4 KB of them again;
rem this keeps them from one frame to the next (src/layer/nr_alloc.py).
echo [6/6] building libnr_alloc.dll ...
cl /nologo /O2 /TC /Fo:"%WORK%\nr_alloc.obj" /Fe:"%WORK%\libnr_alloc.dll" /LD "%REPO%\src\layer\nr_alloc.c"
if errorlevel 1 ( echo ERROR: libnr_alloc.dll failed & exit /b 1 )
echo   libnr_alloc.dll

:done

rem A small native Windows host opens the WPF setup window without a console.
powershell -NoProfile -ExecutionPolicy Bypass -File "%REPO%\tools\build_wizard.ps1" -Root "%REPO%"
if errorlevel 1 ( echo ERROR: NR-Setup.exe failed & exit /b 1 )

echo.
echo Built into %WORK%
echo   libxmx.dll    the runtime the daemon loads
echo   nr_layer.dll  the layer a 64-bit game loads
echo   nr_layer32.dll the layer a 32-bit game loads, with a manifest whose library_arch is 32
echo   libnr_image.dll the host passes around the network
echo   libnr_alloc.dll the daemon's NumPy allocator
echo   *.spv         the compute shaders
echo   NR-Setup.exe  the Windows setup window
echo.
echo Next: tools\deploy.bat --game ^<game dir^> installs the layer and points the
echo daemon at this checkout, so nothing has to be copied into the game folder.
endlocal
exit /b 0

rem ---- :shader <name> - one .comp -> .spv, from src/gpu or src/bench ----
rem
rem No per-driver defines. Intel's Windows compiler folds half_round's packHalf2x16 round
rem trip to x (90109 of 90368 values on the B580's 101.8993, the same on the Arc 140V's
rem 101.9033), so libxmx gives every pipeline the float16_t cast there instead, from the
rem driver it finds, through specialisation constant 1. The attention shaders' softmax
rem uses packHalf2x16 only as a bit trick on the packed word, which the fold does not touch.
rem
rem Two sources, not one: the runtime's shaders live in src/gpu and the probe in src/bench,
rem and the Makefile builds both. A missing source is an error here rather than a quiet
rem skip - looking only in src/gpu once made `call :shader half_probe` fall through in
rem silence, and a daemon spawned by the layer then died on the next thing that assumed it.
:shader
set "SRC=%GPU%\%1.comp"
if not exist "%SRC%" set "SRC=%REPO%\src\bench\%1.comp"
if not exist "%SRC%" (echo ERROR: no source for %1.comp & exit /b 1)
"%GLSL%" --target-env vulkan1.3 -I"%GPU%" -o "%WORK%\%1.spv" "%SRC%" >nul 2>&1
if exist "%WORK%\%1.spv" (echo   %1.spv) else (echo   %1.spv FAILED)
goto :eof

rem ---- :shader_def <out-name> <source-name> <defines...> ----
rem For the aliases and variants the Makefile builds from one source with -D. The
rem defines argument may be empty.
:shader_def
if not exist "%GPU%\%2.comp" goto :eof
set "DEFARGS="
if not "%~3"=="" set "DEFARGS=%~3"
if not "%~4"=="" set "DEFARGS=%DEFARGS% %~4"
"%GLSL%" --target-env vulkan1.3 %DEFARGS% -I"%GPU%" -o "%WORK%\%1.spv" "%GPU%\%2.comp" >nul 2>&1
if exist "%WORK%\%1.spv" (echo   %1.spv) else (echo   %1.spv FAILED)
goto :eof
