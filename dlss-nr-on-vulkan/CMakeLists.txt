# DLSS 5 Neural Rendering on Vulkan — the C runtime, the frame library, the shaders and the
# tests, for Linux, macOS and Windows.
#
# The Makefile is the build this project grew up with and is still complete on Linux and
# macOS; this is the same build described once for CMake, so that Windows (MSVC or clang-cl,
# with the Vulkan SDK) configures too. Everything lands in the build directory — the
# libraries, the executables, the SPIR-V, the ICD manifest — flat, under the same names the
# Makefile gives them in `work/`. The Python side finds either through `src/nr_build.py`:
# `NR_BUILD_DIR` in the environment first (ctest sets it), else the newest of `work/` and
# `build*/`. Only the inputs stay in `work/`: the weights and the pinned Vulkan-Headers clone.
#
#     cmake -S . -B build && cmake --build build && ctest --test-dir build
#     NR_BUILD_DIR=$PWD/build python3 src/ref/nr_frame.py IN.png OUT.png --resident
#
# What each platform builds:
#   everywhere      libxmx, libnr_image, libnr_frame, nr_frame (needs libpng), gemm_runner,
#                   test_nr_frame, every shader
#   Linux, macOS    the Vulkan layer libnr_layer and its POSIX tests (NR_BUILD_LAYER)
#   macOS           libxmx and gemm_runner link MoltenVK directly (NR_XMX_LINK_MOLTENVK);
#                   the layer and its tests link the loader; work/MoltenVK_icd.json is written;
#                   libmetalmx, the same runtime on Metal directly with its shaders compiled
#                   into it by the Metal compiler (NR_BUILD_METAL; NR_GPU_BACKEND=metal at run time)
#   Windows         libd3dmx, the same runtime on Direct3D 12 with the kernels written in HLSL
#                   and compiled to DXIL by dxc, embedded (NR_BUILD_D3D12, on for every Windows
#                   target but 32-bit x86; NR_GPU_BACKEND=d3d12 at run time); wherever it is
#                   built, libdlssnr is built from it instead of libxmx (NR_DLSSNR_D3D12)
#   Android         the libraries, libdlssnr and the executables against the NDK's libvulkan.so,
#                   with the NDK's own toolchain file; the layer stays off (there is no game to
#                   hook and no daemon), bin2c and slice are built for the build machine
#
# Windows: built and linked as a MinGW-w64 cross-compile from macOS (x86_64-pc-mingw32-gcc,
# -DCMAKE_SYSTEM_NAME=Windows, libpng and the Vulkan import library under the cross prefix);
# not yet run on a Windows machine, and MSVC is configured, not measured (notes/phase69).
#
# Android: standalone,
#     cmake -S . -B build-android -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
#           -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=28 && cmake --build build-android
# or as part of VBA-M's tools/android/build-android-qt.sh, which links libdlssnr into the Qt
# APK. Built and linked here for arm64-v8a; nothing has run on a device yet (notes/phase72).
cmake_minimum_required(VERSION 3.21)
project(dlss_nr_on_vulkan LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)                 # dladdr, clock_gettime, strdup on glibc
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(NR_WORK_DIR "${PROJECT_SOURCE_DIR}/work" CACHE PATH "the inputs the build does not make: the weights, the Vulkan-Headers clone")
set(NR_OUTPUT_DIR "${CMAKE_BINARY_DIR}" CACHE PATH "where libraries, executables and SPIR-V land; -DNR_OUTPUT_DIR=work reproduces the Makefile layout")
# Off everywhere.  VBA-M links libdlssnr as a display filter and never injects
# the pass into another process, so nothing here builds the layer: on Windows it
# is unported (Unix sockets and pthreads), on Android there is no game to hook
# and no daemon, and elsewhere its tests link the Vulkan loader -- which on a
# macOS cross build is the host's, for the wrong architecture.  Build it and its
# tests with -DNR_BUILD_LAYER=ON.
set(NR_LAYER_DEFAULT OFF)
option(NR_BUILD_LAYER "the Vulkan layer that puts the pass inside a game, and its tests (POSIX)" ${NR_LAYER_DEFAULT})
option(NR_XMX_LINK_MOLTENVK "macOS: link the compute runtime to MoltenVK directly rather than the loader" ${APPLE})
# libmetalmx: libxmx's entry points on Metal directly (src/gpu/libmetalmx.m), its shaders
# (src/gpu/metal/*.metal) compiled by the Metal compiler into one metallib that bin2c puts
# inside the library. Apple only: there is no Metal anywhere else, and the option is
# refused rather than ignored off Apple. NR_GPU_BACKEND=metal selects it at run time.
option(NR_BUILD_METAL "macOS: build libmetalmx, the compute runtime on Metal, with the Metal shaders compiled into it" ${APPLE})
if(NR_BUILD_METAL AND NOT APPLE)
  message(FATAL_ERROR "NR_BUILD_METAL is on and this is not an Apple build; libmetalmx exists on macOS only")
endif()
if(NR_BUILD_METAL)
  # since Xcode 15 the Metal compiler is a separate download; without it the rest still builds
  execute_process(COMMAND xcrun -sdk macosx -f metal RESULT_VARIABLE nr_metal_missing OUTPUT_QUIET ERROR_QUIET)
  if(nr_metal_missing)
    message(WARNING "no Metal compiler (xcrun -sdk macosx -f metal failed): libmetalmx is not built. Install it with `xcodebuild -downloadComponent MetalToolchain`, or pass -DNR_BUILD_METAL=OFF to silence this")
    set(NR_BUILD_METAL OFF)
  endif()
endif()
# libd3dmx: libxmx's entry points on Direct3D 12 (src/gpu/libd3dmx.c), its kernels written in
# HLSL (src/gpu/d3d12/*.hlsl) and compiled to DXIL by dxc, each module embedded through bin2c.
# Windows only — there is no Direct3D anywhere else, and the option is refused rather than
# ignored off Windows. On by default for every Windows target except 32-bit x86 (x64 and
# ARM64, MSVC or MinGW): the 32-bit build keeps the Vulkan archive, as VBA-M builds no Vulkan
# there either, and -DNR_BUILD_D3D12=ON still asks for it. NR_GPU_BACKEND=d3d12 selects it at
# run time. dxc comes with the Windows SDK, the Vulkan SDK and vcpkg's directx-dxc; without
# one the rest still builds, and the archive stays Vulkan.
#
# 32-bit x86 is read three ways, since the tree is configured both standalone and as VBA-M's
# subdirectory: VBA-M's Architecture.cmake leaves X86 set for it; a vcpkg triplet names it
# x86-…; and standalone the target has a 4-byte pointer under an x86-family processor name
# (an MSVC x86 cross shell reports the host's AMD64 for CMAKE_SYSTEM_PROCESSOR).
set(NR_WIN32_X86 OFF)
if(WIN32)
  if(X86 OR VCPKG_TARGET_TRIPLET MATCHES "^[xX]86-"
     OR (CMAKE_SIZEOF_VOID_P EQUAL 4
         AND CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86|i[3-9]86|[aA][mM][dD]64|[xX]86_64)$"))
    set(NR_WIN32_X86 ON)
  endif()
endif()
option(NR_BUILD_D3D12 "Windows (x64, ARM64): build libd3dmx, the compute runtime on Direct3D 12, with the HLSL compiled to DXIL and embedded" OFF)
if(NR_BUILD_D3D12 AND NOT WIN32)
  message(FATAL_ERROR "NR_BUILD_D3D12 is on and this is not a Windows build; libd3dmx exists on Windows only")
endif()
if(WIN32 AND NR_WIN32_X86 AND NOT NR_BUILD_D3D12)
  message(STATUS "32-bit x86 Windows: libd3dmx is not built and libdlssnr stays the Vulkan archive (-DNR_BUILD_D3D12=ON asks for it anyway)")
endif()
if(NR_BUILD_D3D12)
  # dxc runs on the build machine: the host triplet's copy first in a vcpkg cross build (the
  # same trap as glslangValidator below), then the SDKs' directories.
  set(nr_dxc_host_dirs)
  if(VBAM_VCPKG_HOST_PREFIX)
    list(APPEND nr_dxc_host_dirs "${VBAM_VCPKG_HOST_PREFIX}/tools/directx-dxc")
  endif()
  if(VCPKG_HOST_TRIPLET AND VCPKG_INSTALLED_DIR)
    list(APPEND nr_dxc_host_dirs "${VCPKG_INSTALLED_DIR}/${VCPKG_HOST_TRIPLET}/tools/directx-dxc")
  endif()
  if(nr_dxc_host_dirs)
    find_program(NR_DXC NAMES dxc.exe dxc PATHS ${nr_dxc_host_dirs} NO_DEFAULT_PATH)
  endif()
  set(nr_dxc_sdk_dirs "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin" "$ENV{WindowsSdkVerBinPath}/x64" "$ENV{WindowsSdkVerBinPath}/arm64")
  file(GLOB nr_windows_kits_bins "C:/Program Files (x86)/Windows Kits/10/bin/10.*/x64" "C:/Program Files (x86)/Windows Kits/10/bin/10.*/arm64")
  list(APPEND nr_dxc_sdk_dirs ${nr_windows_kits_bins})
  find_program(NR_DXC NAMES dxc.exe dxc HINTS ${nr_dxc_sdk_dirs} DOC "the DirectX shader compiler, for the HLSL kernels")
  if(NOT NR_DXC)
    message(WARNING "dxc not found (Windows SDK, Vulkan SDK or vcpkg directx-dxc): libd3dmx is not built and libdlssnr stays the Vulkan archive; -DNR_DXC= names one, -DNR_BUILD_D3D12=OFF silences this")
    set(NR_BUILD_D3D12 OFF)
  elseif(CMAKE_CROSSCOMPILING AND VCPKG_TARGET_TRIPLET AND NR_DXC MATCHES "/installed/${VCPKG_TARGET_TRIPLET}/")
    message(FATAL_ERROR "dxc found only as the target triplet's, ${NR_DXC}, which cannot run on this machine. Install it for the host (vcpkg install directx-dxc --triplet <host triplet>) or name one with -DNR_DXC=")
  endif()
endif()
# libdlssnr from libd3dmx rather than libxmx. Follows NR_BUILD_D3D12: wherever the Direct3D 12
# runtime is built, the static archive a host links carries it — libd3dmx's objects and the
# DXIL, no Vulkan anywhere in it; nr_frame_adopt_d3d12 is the share, nr_frame_adopt_vulkan is
# refused and nr_frame_runtime() says "d3d12". -DNR_DLSSNR_D3D12=OFF keeps the Vulkan archive
# beside libd3dmx.dll, for a host whose renderer lends a Vulkan device. The value is fixed on
# the first configure, as a cache variable's is: a dxc installed later, or -DNR_BUILD_D3D12
# changed afterwards, needs -DNR_DLSSNR_D3D12= set to match (or a fresh cache).
option(NR_DLSSNR_D3D12 "Windows: build libdlssnr, the static archive a host links, from libd3dmx (Direct3D 12) instead of libxmx (Vulkan); follows NR_BUILD_D3D12" ${NR_BUILD_D3D12})
if(NR_DLSSNR_D3D12 AND NOT NR_BUILD_D3D12)
  message(FATAL_ERROR "NR_DLSSNR_D3D12 needs NR_BUILD_D3D12 (Windows, with dxc found); pass -DNR_DLSSNR_D3D12=OFF as well to build libdlssnr from libxmx")
endif()
option(NR_NO_FLOAT16 "use the software half conversion even where the compiler has _Float16 (for checking it)" OFF)
if(CMAKE_CROSSCOMPILING)
  set(NR_TESTS_DEFAULT OFF)                        # the executables cannot run here
else()
  set(NR_TESTS_DEFAULT ON)
endif()
option(NR_BUILD_TESTS "register the tests with ctest" ${NR_TESTS_DEFAULT})

# The weights, compiled in. `nr_frame_open(NULL)` — what the command, the C test and
# `NativeFrame()` default to — reads the logical safetensors from byte arrays inside
# libnr_frame; no file is opened at run time. Those arrays are C sources in the weights
# directory (`weights/` in the source tree): one header and one translation unit per
# NR_EMBED_CHUNK_MB slice of the file, and a table listing the slices in order. The build
# compiles what is in that directory. It only *regenerates* it with NR_BIN2C_WEIGHTS: then
# `dlssnr-logical.safetensors` — in the source root, under work/mlxw/, or named in
# NR_WEIGHTS_FILE — is cut into slices and each is run through bin2c into the directory.
# Without the file the regeneration is skipped and the directory is used as it is.
# Since 2026-10-10 the directory holds the *prepared* weights (`nr_frame_prepare`,
# `prepared: v2`): the logical tensors with the loaders' derived tensors beside them, F32
# first then F16 so every offset is aligned, and the slices cut at tensor boundaries so the
# reader points at every tensor inside the library instead of copying it. Regenerating looks
# for `dlssnr-prepared.safetensors` first; `cmake --build . --target prepare_weights` writes it
# from the logical file with the `nr_frame` command this build makes, then reconfigure with
# -DNR_BIN2C_WEIGHTS=ON. A logical file still regenerates a working (unprepared) directory.
option(NR_EMBED_WEIGHTS "compile the weights from the weights directory into libnr_frame" ON)
set(NR_WEIGHTS_DIR "${PROJECT_SOURCE_DIR}/weights" CACHE PATH "the weights as C: the bin2c slices and their table, compiled into libnr_frame")
option(NR_BIN2C_WEIGHTS "regenerate the weights directory from dlssnr-prepared.safetensors (else dlssnr-logical.safetensors) with slice and bin2c (skipped when neither file is there)" OFF)
set(NR_WEIGHTS_FILE "" CACHE FILEPATH "the weights NR_BIN2C_WEIGHTS regenerates from (dlssnr-prepared.safetensors, or the logical file); empty looks in the source root and work/mlxw/")
set(NR_EMBED_CHUNK_MB 8 CACHE STRING "the most a slice holds when the weights are regenerated (the compiler needs ~90 bytes of memory per byte of a slice); slices are cut at tensor boundaries under it")
set(NR_EMBED_JOBS 2 CACHE STRING "how many slices Ninja compiles at once (a job pool; ~0.75 GB each at 8 MB)")
option(NR_EMBED_SHADERS "compile every SPIR-V module into libxmx and gemm_runner with bin2c, so shaders are loaded by name from the binary" ON)
set(NR_EMBED_SYMBOL dlssnr_logical_safetensors)       # the slice arrays are ${NR_EMBED_SYMBOL}_<k>
set(NR_REGENERATE_WEIGHTS OFF)                          # NR_BIN2C_WEIGHTS, and the file exists
if(NR_EMBED_WEIGHTS AND NR_BIN2C_WEIGHTS)
  if(NOT NR_WEIGHTS_FILE)
    foreach(candidate "${PROJECT_SOURCE_DIR}/dlssnr-prepared.safetensors" "${NR_WORK_DIR}/mlxw/dlssnr-prepared.safetensors"
                      "${PROJECT_SOURCE_DIR}/dlssnr-logical.safetensors" "${NR_WORK_DIR}/mlxw/dlssnr-logical.safetensors")
      if(EXISTS "${candidate}")
        set(NR_WEIGHTS_FILE "${candidate}")
        break()
      endif()
    endforeach()
  endif()
  if(EXISTS "${NR_WEIGHTS_FILE}")
    set(NR_REGENERATE_WEIGHTS ON)
  else()
    message(STATUS "NR_BIN2C_WEIGHTS: no dlssnr-prepared.safetensors or dlssnr-logical.safetensors (source root, work/mlxw/, or -DNR_WEIGHTS_FILE=); bin2c skipped, using the weights directory as it is")
  endif()
endif()
if(NR_EMBED_WEIGHTS AND NOT NR_REGENERATE_WEIGHTS)
  # the pre-processed directory: its table and at least the first slice must be there
  if(NOT EXISTS "${NR_WEIGHTS_DIR}/nr_weights_embedded.c" OR NOT EXISTS "${NR_WEIGHTS_DIR}/${NR_EMBED_SYMBOL}_0.h")
    message(STATUS "no pre-processed weights in ${NR_WEIGHTS_DIR} (regenerate with -DNR_BIN2C_WEIGHTS=ON and the safetensors file); nr_frame_open(NULL) will refuse and a path is needed")
    set(NR_EMBED_WEIGHTS OFF)
  endif()
endif()

file(MAKE_DIRECTORY "${NR_OUTPUT_DIR}")

# ---------------------------------------------------------------------------------------
# dependencies
# ---------------------------------------------------------------------------------------

# Vulkan headers: the SDK's, the distribution's, vcpkg's, or the pinned clone in work/.
find_package(Vulkan QUIET)
if(Vulkan_FOUND)
  set(NR_VULKAN_INCLUDE "${Vulkan_INCLUDE_DIRS}")
else()
  find_path(NR_VULKAN_INCLUDE vulkan/vulkan.h
    HINTS "${NR_WORK_DIR}/vulkan-headers/include" "$ENV{VULKAN_SDK}/include"
          "$ENV{HOME}/Downloads/vcpkg/installed/arm64-osx/include" /usr/local/include /opt/homebrew/include)
  if(NOT NR_VULKAN_INCLUDE)
    message(FATAL_ERROR "no Vulkan headers: install the Vulkan SDK, or\n"
      "  git clone --depth 1 --branch v1.4.321 https://github.com/KhronosGroup/Vulkan-Headers.git work/vulkan-headers")
  endif()
endif()
if(NOT Vulkan_INCLUDE_DIRS)
  set(Vulkan_INCLUDE_DIRS "${NR_VULKAN_INCLUDE}")
endif()

# The loader, for everything that is not the compute runtime on macOS.
if(Vulkan_FOUND AND TARGET Vulkan::Vulkan)
  set(NR_LOADER Vulkan::Vulkan)
else()
  find_library(NR_LOADER_LIBRARY NAMES vulkan vulkan-1
    HINTS "$ENV{VULKAN_SDK}/lib" "$ENV{VULKAN_SDK}/Lib" "$ENV{HOME}/Downloads/vcpkg/installed/arm64-osx/lib" /usr/local/lib)
  set(NR_LOADER "${NR_LOADER_LIBRARY}")
endif()

# MoltenVK, for the compute runtime on macOS: it exports the whole Vulkan API, so the
# path needs no loader, no ICD manifest and no portability enumeration.
if(APPLE AND NR_XMX_LINK_MOLTENVK)
  # Frameworks last: MacPorts and Homebrew both ship a MoltenVK.xcframework under
  # Frameworks/ beside lib/libMoltenVK.dylib, and framework-first find_library()
  # takes the directory.  CMake links the static libMoltenVK.a inside it, which
  # the .a check below does not see through, and the ICD manifest would name a
  # libMoltenVK.dylib next to the .xcframework that is not there.
  set(NR_SAVED_FIND_FRAMEWORK "${CMAKE_FIND_FRAMEWORK}")
  set(CMAKE_FIND_FRAMEWORK LAST)
  find_library(NR_MOLTENVK_LIBRARY NAMES MoltenVK
    HINTS /usr/local/lib /opt/homebrew/lib "$ENV{VULKAN_SDK}/lib" "$ENV{VULKAN_SDK}/macOS/lib"
          "$ENV{HOME}/Downloads/vcpkg/installed/arm64-osx/lib")
  if(NR_SAVED_FIND_FRAMEWORK)
    set(CMAKE_FIND_FRAMEWORK "${NR_SAVED_FIND_FRAMEWORK}")
  else()
    unset(CMAKE_FIND_FRAMEWORK)
  endif()
  unset(NR_SAVED_FIND_FRAMEWORK)
  if(NOT NR_MOLTENVK_LIBRARY)
    message(FATAL_ERROR "NR_XMX_LINK_MOLTENVK is on and libMoltenVK.dylib was not found; set NR_MOLTENVK_LIBRARY or turn the option off")
  endif()
  set(NR_XMX_VULKAN "${NR_MOLTENVK_LIBRARY}")

  # A static MoltenVK leaves its own dependencies to whoever links it, where the
  # dylib carries them itself.  Two kinds are missing.  The frameworks: without
  # them xmx and gemm_runner fail on _IOSurface*, _IOService*, _IORegistry*,
  # NSView and the rest -- what MVKPhysicalDevice::initGPUInfoProperties() and
  # MVKImage::useIOSurface() reach for.  And the C++ runtime: MoltenVK is C++
  # and both of those targets are C, so nothing else puts libc++ on the line
  # and 294 std:: and __cxa_* symbols go unresolved.
  # An .xcframework still wins when it is the only MoltenVK there is, and
  # MoltenVK's own is always the static archive.
  if(NR_MOLTENVK_LIBRARY MATCHES "\\.(a|xcframework)$")
    list(APPEND NR_XMX_VULKAN c++)

    foreach(nr_fw Metal Foundation QuartzCore CoreFoundation CoreGraphics
                  IOSurface IOKit AppKit)
      find_library(NR_MOLTENVK_FW_${nr_fw} ${nr_fw})

      if(NOT NR_MOLTENVK_FW_${nr_fw})
        message(FATAL_ERROR "a static MoltenVK needs the ${nr_fw} framework and it was not found")
      endif()

      list(APPEND NR_XMX_VULKAN "${NR_MOLTENVK_FW_${nr_fw}}")
    endforeach()

    unset(nr_fw)
  endif()
  get_filename_component(NR_MOLTENVK_DIR "${NR_MOLTENVK_LIBRARY}" DIRECTORY)
  # the loader finds no driver on its own on macOS; the layer tests point VK_DRIVER_FILES here
  file(WRITE "${NR_OUTPUT_DIR}/MoltenVK_icd.json"
"{
  \"file_format_version\": \"1.0.0\",
  \"ICD\": {
    \"library_path\": \"${NR_MOLTENVK_DIR}/libMoltenVK.dylib\",
    \"api_version\": \"1.4.0\",
    \"is_portability_driver\": true
  }
}
")
else()
  set(NR_XMX_VULKAN ${NR_LOADER})
endif()

# glslangValidator: the SDK's, or the one on the path. It runs on the build machine, and
# that is the trap in a vcpkg cross build: vcpkg.cmake puts the *target* triplet's tools
# directory on CMAKE_PROGRAM_PATH, which find_program() searches before any HINTS, and
# the glslangValidator.exe there is an executable for the target -- Windows refuses the
# arm64 one on an x64 host ("not compatible with the version of Windows you're
# running"). So the host triplet's copy is looked for first, on its own, and a find that
# still lands under the target triplet is refused here rather than at build time.
set(nr_glslang_host_dirs)
if(VBAM_VCPKG_HOST_PREFIX)  # VBA-M's name for vcpkg's host-triplet prefix
  list(APPEND nr_glslang_host_dirs "${VBAM_VCPKG_HOST_PREFIX}/tools/glslang")
endif()
if(VCPKG_HOST_TRIPLET AND VCPKG_INSTALLED_DIR)
  list(APPEND nr_glslang_host_dirs "${VCPKG_INSTALLED_DIR}/${VCPKG_HOST_TRIPLET}/tools/glslang")
endif()
if(nr_glslang_host_dirs)
  find_program(NR_GLSLANG NAMES glslangValidator.exe glslangValidator PATHS ${nr_glslang_host_dirs} NO_DEFAULT_PATH)
endif()
find_program(NR_GLSLANG NAMES glslangValidator.exe glslangValidator HINTS "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/Bin" "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}")
if(NOT NR_GLSLANG)
  message(FATAL_ERROR "glslangValidator not found; it comes with the Vulkan SDK (or the glslang package)")
endif()
if(CMAKE_CROSSCOMPILING AND VCPKG_TARGET_TRIPLET AND NR_GLSLANG MATCHES "/installed/${VCPKG_TARGET_TRIPLET}/")
  message(FATAL_ERROR "glslangValidator found only as the target triplet's, ${NR_GLSLANG}, which cannot run on this machine. Install it for the host (vcpkg install glslang[tools] --triplet <host triplet>) or name one with -DNR_GLSLANG=")
endif()
# It prints each source's name on stdout as it compiles it, one line per shader in the
# build's output. --quiet drops that and keeps errors and warnings; older glslang has no
# such flag and refuses it, so it is passed only when the help lists it.
set(NR_GLSLANG_QUIET)
execute_process(COMMAND "${NR_GLSLANG}" -h OUTPUT_VARIABLE nr_glslang_help ERROR_VARIABLE nr_glslang_help)
if(nr_glslang_help MATCHES "--quiet")
  set(NR_GLSLANG_QUIET --quiet)
endif()

# libpng, for the nr_frame command only; the libraries need nothing.
#
# FindPNG picks the header and the library separately, and on macOS it searches framework
# Headers directories first: on a Mac with a dead /usr/local/include/png.h symlink it paired
# Mono.framework's libpng 1.4 header with /usr/local/lib's 1.6, which compiles, links, and
# refuses every file at run time ("Application built with libpng-1.4.12 but running with
# 1.6.58"). So: frameworks last, the prefixes a Mac keeps libpng under as hints, and a
# run-time check at configure time that the header and the library are the same libpng.
set(NR_SAVED_FIND_FRAMEWORK "${CMAKE_FIND_FRAMEWORK}")
set(CMAKE_FIND_FRAMEWORK LAST)
if(NOT PNG_ROOT AND NOT DEFINED ENV{PNG_ROOT})
  foreach(prefix "$ENV{HOME}/Downloads/vcpkg/installed/arm64-osx" /opt/homebrew /usr/local)
    if(EXISTS "${prefix}/include/png.h")
      set(PNG_ROOT "${prefix}")
      break()
    endif()
  endforeach()
endif()
find_package(PNG QUIET)
# Restore by unsetting when it was unset: writing back an empty string is not
# the same thing, and FindPython3 below rejects an empty value as invalid.
if(NR_SAVED_FIND_FRAMEWORK)
  set(CMAKE_FIND_FRAMEWORK "${NR_SAVED_FIND_FRAMEWORK}")
else()
  unset(CMAKE_FIND_FRAMEWORK)
endif()
unset(NR_SAVED_FIND_FRAMEWORK)
if(PNG_FOUND AND NOT CMAKE_CROSSCOMPILING)
  include(CheckCSourceRuns)
  set(CMAKE_REQUIRED_LIBRARIES PNG::PNG)
  set(NR_PNG_CHECK_SOURCE [[
#include <png.h>
#include <stdio.h>
int main(void) {
  unsigned lib = png_access_version_number();
  printf("header %s library %u.%u.%u", PNG_LIBPNG_VER_STRING, lib / 10000, (lib / 100) % 100, lib % 100);
  puts("");   /* no backslash escapes here: the check macro re-reads them */
  return lib / 100 == PNG_LIBPNG_VER / 100 ? 0 : 1;   /* libpng wants major.minor to agree */
}
]])
  check_c_source_runs("${NR_PNG_CHECK_SOURCE}" NR_PNG_HEADER_MATCHES_LIBRARY)
  unset(CMAKE_REQUIRED_LIBRARIES)
  if(NOT NR_PNG_HEADER_MATCHES_LIBRARY)
    message(WARNING "libpng header ${PNG_VERSION_STRING} in ${PNG_PNG_INCLUDE_DIR} does not match the library ${PNG_LIBRARIES}; "
      "the nr_frame command is not built. Set PNG_ROOT to a prefix whose include/ and lib/ hold the same libpng.")
    set(PNG_FOUND FALSE)
  endif()
endif()

find_package(Threads)
find_package(Python3 COMPONENTS Interpreter QUIET)

# ---------------------------------------------------------------------------------------
# compiler flags. The host maths reproduces NumPy's float32 operation order, so no
# fused multiply-add and no fast maths, anywhere.
# ---------------------------------------------------------------------------------------

if(MSVC)
  set(NR_EXACT_FLAGS /fp:precise)
  set(NR_NATIVE_FLAGS "")                   # MSVC has no -march=native; /O2 is what there is
else()
  set(NR_EXACT_FLAGS -ffp-contract=off -fno-fast-math)
  set(NR_WARN_FLAGS -Wall -Wextra -Wno-unused-parameter)
  set(NR_NATIVE_FLAGS "")
  # A host that compiles everything with -ffast-math (VBA-M's Release build) hands it to
  # these targets too; the exact flags come after it on the line and win -- checked on the
  # NDK's clang 21: no fmadd, no reassociation -- but clang says so for every file.
  if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    list(APPEND NR_WARN_FLAGS -Wno-overriding-option)
  endif()
endif()

# The host passes' CPU floor (the Makefile's NR_IMAGE_ARCH): x86-64-v3 on x86-64 — every CPU
# since Haswell and Zen 1 — whose F16C makes each half conversion one instruction instead of a
# call into libgcc, and AVX2 is there for the loops; measured upstream at 5-13 % of a frame,
# byte-identical, and as fast as -march=native on Lunar Lake. A release copies the library to
# other machines, so neither build system may build it for this one. On arm64 and the other
# targets the flag means nothing and the hardware conversion comes through `_Float16` anyway
# (FCVT between half and single is base ARMv8.0, so arm64 needs no floor from any -march),
# so the floor is empty there; so it is for a 32-bit x86 build. MSVC gets NR_F16C instead on
# x86-64, under which nr_portable.h converts through the F16C intrinsics (build_win.bat does
# the same); MSVC for ARM64 takes the NEON conversions by itself (nr_portable.h, `_M_ARM64`).
# -DNR_IMAGE_ARCH= overrides it (an empty value builds for the generic CPU).
include(CheckCCompilerFlag)
if(NOT DEFINED NR_IMAGE_ARCH)
  set(nr_image_arch_default "")
  # a universal macOS build compiles every file for each slice: an x86-64 flag on an arm64
  # slice is an error, so such a build takes no floor (its x86-64 slice converts through
  # `_Float16`, one instruction from any -march where the compiler has F16C, a libcall else)
  set(nr_image_arch_one_slice ON)
  if(CMAKE_OSX_ARCHITECTURES AND NOT CMAKE_OSX_ARCHITECTURES MATCHES "^[xX]86_64$")
    set(nr_image_arch_one_slice OFF)
  endif()
  if(nr_image_arch_one_slice AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$"
     AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(MSVC)
      set(nr_image_arch_default "NR_F16C")
    else()
      check_c_compiler_flag(-march=x86-64-v3 NR_HAS_X86_64_V3)
      if(NR_HAS_X86_64_V3)
        set(nr_image_arch_default "-march=x86-64-v3")
      else()
        message(WARNING "the compiler takes no -march=x86-64-v3: the host passes will convert "
                        "every half through libgcc, several times slower")
      endif()
    endif()
  endif()
  set(NR_IMAGE_ARCH "${nr_image_arch_default}" CACHE STRING
      "the CPU the host passes (nr_image.c) are built for: a -march flag, NR_F16C for MSVC, or empty")
endif()
set(NR_IMAGE_ARCH_FLAGS "")
set(NR_IMAGE_ARCH_DEFINES "")
if(NR_IMAGE_ARCH STREQUAL "NR_F16C")
  set(NR_IMAGE_ARCH_DEFINES NR_F16C)
elseif(NR_IMAGE_ARCH)
  separate_arguments(NR_IMAGE_ARCH_FLAGS NATIVE_COMMAND "${NR_IMAGE_ARCH}")
endif()

if(NR_NO_FLOAT16)
  add_compile_definitions(NR_NO_FLOAT16)
endif()

# WINDOWS_EXPORT_ALL_SYMBOLS has CMake read every object on the DLL's link line to write the
# .def file, and that reader does not understand the anonymous objects MSVC's /GL produces
# ("unrecognized file format in 'libxmx.c.obj, 0'"). A host that builds with
# CMAKE_INTERPROCEDURAL_OPTIMIZATION (VBA-M's ENABLE_LTO, on in Release) would hand those
# objects over, so the exported DLLs and the object libraries that feed them opt out of it
# on the MSVC toolchains; the rest of the build keeps whatever the host asked for.
function(nr_no_ipo target)
  if(MSVC OR CMAKE_C_SIMULATE_ID MATCHES "MSVC")
    set_target_properties(${target} PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
  endif()
endfunction()

# Every target lands in the output directory under its plain name: libxmx.so / .dylib / .dll, no
# per-configuration subdirectory, and the Python's `lib` prefix on Windows too.
function(nr_place target)
  set_target_properties(${target} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${NR_OUTPUT_DIR}/$<0:>"
    RUNTIME_OUTPUT_DIRECTORY "${NR_OUTPUT_DIR}/$<0:>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<0:>"
    PREFIX "lib"
    BUILD_RPATH "${NR_OUTPUT_DIR}"
    INSTALL_RPATH "${NR_OUTPUT_DIR}")
  if(WIN32)
    set_target_properties(${target} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    nr_no_ipo(${target})
  endif()
endfunction()

function(nr_executable target)
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${NR_OUTPUT_DIR}/$<0:>"
    BUILD_RPATH "${NR_OUTPUT_DIR}"
    INSTALL_RPATH "${NR_OUTPUT_DIR}")
endfunction()

# ---------------------------------------------------------------------------------------
# host tools: bin2c (a file as a C byte array) and slice (a range of a file), for embedding
# the shaders and the weights. Both run on the build host, so a cross-compile names host
# copies in NR_BIN2C / NR_SLICE or embeds nothing. The targets carry a suffix because this
# tree is also added as a subdirectory of VBA-M, whose build has a bin2c of its own.
# ---------------------------------------------------------------------------------------

if(NR_REGENERATE_WEIGHTS OR NR_EMBED_SHADERS OR NR_BUILD_METAL OR NR_BUILD_D3D12)
  if(CMAKE_CROSSCOMPILING)
    # Under VBA-M, its host_compile() builds a tool for the build machine with a host
    # compiler (the x86 cl of the same MSVC in an arm64 cross shell, `cc` elsewhere) --
    # the same way VBA-M builds its own bin2c. Standalone the module is not there and a
    # host copy has to be named.
    include(HostCompile OPTIONAL RESULT_VARIABLE nr_host_compile)
    if(nr_host_compile AND NOT NR_BIN2C AND NOT NR_SLICE)
      set(nr_host_tools_dir "${CMAKE_CURRENT_BINARY_DIR}/tools")
      file(MAKE_DIRECTORY "${nr_host_tools_dir}")
      set(nr_host_exe_suffix "")
      if(CMAKE_HOST_WIN32)
        set(nr_host_exe_suffix ".exe")
      endif()
      set(NR_BIN2C "${nr_host_tools_dir}/bin2c_dlss${nr_host_exe_suffix}")
      set(NR_SLICE "${nr_host_tools_dir}/slice_dlss${nr_host_exe_suffix}")
      host_compile("${PROJECT_SOURCE_DIR}/src/tools/bin2c.c" "${NR_BIN2C}")
      host_compile("${PROJECT_SOURCE_DIR}/src/tools/slice.c" "${NR_SLICE}")
      message(STATUS "cross-compiling: bin2c and slice built for the host with host_compile()")
    endif()
    if(NOT nr_host_compile AND NOT NR_BIN2C AND NOT NR_SLICE)
      # Standalone -- an NDK toolchain file, say -- the module is not there, so do the
      # same thing here: a C compiler from this machine's PATH, and never the cross
      # toolchain's own, which the toolchain file puts on CMake's program search and
      # which would quietly build the tools for the target.
      if(NOT NR_HOST_CC)
        if(CMAKE_HOST_APPLE AND EXISTS /usr/bin/clang)
          set(NR_HOST_CC /usr/bin/clang CACHE FILEPATH "C compiler that builds executables for the build host")
        else()
          find_program(NR_HOST_CC NAMES cc clang gcc PATHS ENV PATH NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH
            DOC "C compiler that builds executables for the build host")
        endif()
      endif()
      if(NR_HOST_CC)
        get_filename_component(nr_host_cc_dir "${NR_HOST_CC}" DIRECTORY)
        get_filename_component(nr_target_cc_dir "${CMAKE_C_COMPILER}" DIRECTORY)
        if(nr_host_cc_dir STREQUAL nr_target_cc_dir)
          message(WARNING "the only C compiler found for this machine, ${NR_HOST_CC}, is the cross toolchain's own; name one with -DNR_HOST_CC= (or -DNR_BIN2C= and -DNR_SLICE=)")
          unset(NR_HOST_CC CACHE)
        endif()
      endif()
      if(NR_HOST_CC)
        set(nr_host_tools_dir "${CMAKE_CURRENT_BINARY_DIR}/tools")
        file(MAKE_DIRECTORY "${nr_host_tools_dir}")
        set(nr_host_exe_suffix "")
        if(CMAKE_HOST_WIN32)
          set(nr_host_exe_suffix ".exe")
        endif()
        set(NR_BIN2C "${nr_host_tools_dir}/bin2c_dlss${nr_host_exe_suffix}")
        set(NR_SLICE "${nr_host_tools_dir}/slice_dlss${nr_host_exe_suffix}")
        add_custom_command(OUTPUT "${NR_BIN2C}"
          COMMAND "${NR_HOST_CC}" "${PROJECT_SOURCE_DIR}/src/tools/bin2c.c" -o "${NR_BIN2C}"
          DEPENDS "${PROJECT_SOURCE_DIR}/src/tools/bin2c.c" COMMENT "host cc bin2c_dlss" VERBATIM)
        add_custom_command(OUTPUT "${NR_SLICE}"
          COMMAND "${NR_HOST_CC}" "${PROJECT_SOURCE_DIR}/src/tools/slice.c" -o "${NR_SLICE}"
          DEPENDS "${PROJECT_SOURCE_DIR}/src/tools/slice.c" COMMENT "host cc slice_dlss" VERBATIM)
        message(STATUS "cross-compiling: bin2c and slice built for the host with ${NR_HOST_CC}")
      endif()
    endif()
    if(NOT NR_BIN2C)
      find_program(NR_BIN2C NAMES bin2c DOC "a host build of src/tools/bin2c.c")
    endif()
    if(NOT NR_SLICE)
      find_program(NR_SLICE NAMES slice nr_slice DOC "a host build of src/tools/slice.c")
    endif()
    if(NOT NR_BIN2C)
      message(WARNING "cross-compiling and no host bin2c found (NR_BIN2C): the shaders are not embedded and the weights directory is used as it is")
      set(NR_EMBED_SHADERS OFF)
      set(NR_REGENERATE_WEIGHTS OFF)
    elseif(NOT NR_SLICE)
      message(WARNING "cross-compiling and no host slice found (NR_SLICE): the weights directory is used as it is")
      set(NR_REGENERATE_WEIGHTS OFF)
    endif()
  else()
    add_executable(bin2c_dlss src/tools/bin2c.c)
    add_executable(slice_dlss src/tools/slice.c)
    set_target_properties(bin2c_dlss slice_dlss PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tools/$<0:>")
    if(MSVC)
      target_compile_definitions(bin2c_dlss PRIVATE _CRT_SECURE_NO_WARNINGS)
      target_compile_definitions(slice_dlss PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
    set(NR_BIN2C bin2c_dlss)
    set(NR_SLICE slice_dlss)
  endif()
endif()

# ---------------------------------------------------------------------------------------
# shaders: plain GLSL to SPIR-V, into the output directory — and, with NR_EMBED_SHADERS,
# each module through bin2c into a header and all of them into one table object
# (src/gpu/nr_shaders_embedded.h) that libxmx and gemm_runner link, so a shader is
# loaded by name from the binary rather than from the file beside it. The .spv files are
# still written: the Python benches and tests read some of them directly.
# ---------------------------------------------------------------------------------------

set(NR_SHADER_EMBED_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY "${NR_SHADER_EMBED_DIR}")
set(NR_SPV_HEADERS)
set(NR_SPV_INCLUDES "")
set(NR_SPV_ROWS "")

set(NR_SHADER_INCLUDES "${PROJECT_SOURCE_DIR}/src/gpu/publish.glsl" "${PROJECT_SOURCE_DIR}/src/gpu/specialize.glsl" "${PROJECT_SOURCE_DIR}/src/gpu/residual_epilogue.glsl"
    "${PROJECT_SOURCE_DIR}/src/gpu/cosine_tree.glsl" "${PROJECT_SOURCE_DIR}/src/gpu/qkv_epilogue.glsl")
set(NR_SPV)
function(nr_shader output source)          # extra arguments are -D defines
  set(out "${NR_OUTPUT_DIR}/${output}")
  add_custom_command(OUTPUT "${out}"
    COMMAND "${NR_GLSLANG}" ${NR_GLSLANG_QUIET} --target-env vulkan1.3 -I${PROJECT_SOURCE_DIR}/src/gpu ${ARGN} -o "${out}" "${PROJECT_SOURCE_DIR}/${source}"
    DEPENDS "${PROJECT_SOURCE_DIR}/${source}" ${NR_SHADER_INCLUDES}
    COMMENT "glslangValidator ${output}" VERBATIM)
  set(NR_SPV ${NR_SPV} "${out}" PARENT_SCOPE)
  if(NR_EMBED_SHADERS)
    string(REGEX REPLACE "\\.spv$" "" stem "${output}")
    set(symbol "spv_${stem}")
    set(header "${NR_SHADER_EMBED_DIR}/${symbol}.h")
    add_custom_command(OUTPUT "${header}"
      COMMAND ${NR_BIN2C} "${out}" "${header}" ${symbol}
      DEPENDS "${out}" ${NR_BIN2C}
      COMMENT "bin2c ${symbol}.h" VERBATIM)
    set(NR_SPV_HEADERS ${NR_SPV_HEADERS} "${header}" PARENT_SCOPE)
    set(NR_SPV_INCLUDES "${NR_SPV_INCLUDES}#include \"${symbol}.h\"\n" PARENT_SCOPE)
    set(NR_SPV_ROWS "${NR_SPV_ROWS}    { \"${output}\", ${symbol}, sizeof ${symbol} },\n" PARENT_SCOPE)
  endif()
endfunction()

nr_shader(gemm_resident.spv        src/gpu/gemm_resident.comp)
nr_shader(gemm_tiled.spv           src/gpu/gemm_resident.comp -DRM=2 -DRN=2)   # a 16x32 block in one subgroup
nr_shader(gemm_staged.spv          src/gpu/gemm_staged.comp)
# 32-row blocks for a bottleneck of 32 tokens or fewer, with a 32- and a 64-deep K step
nr_shader(gemm_staged32.spv        src/gpu/gemm_staged.comp -DSTAGED_BM=32)
nr_shader(gemm_staged32_deep.spv   src/gpu/gemm_staged.comp -DSTAGED_BM=32 -DSTAGED_BK=64)
nr_shader(resident.spv             src/gpu/resident.comp)
nr_shader(attention.spv            src/gpu/attention.comp)
nr_shader(attention_ab.spv         src/gpu/attention.comp -DSOFTMAX_AB)
nr_shader(attention_rows.spv       src/gpu/attention.comp -DROW_LANES=256)   # the whole-row softmax
nr_shader(history.spv              src/gpu/history.comp)
nr_shader(gemm_coopmat.spv         src/gpu/gemm_coopmat.comp)
nr_shader(gemm_batched.spv         src/gpu/gemm_coopmat_batched.comp)
nr_shader(gemm_f16acc.spv          src/gpu/gemm_coopmat_f16acc.comp)
nr_shader(gemm_coopmat_int8.spv    src/gpu/gemm_coopmat_int8.comp)   # configuration 4, sint8 x sint8 -> sint32
# the same GEMMs without the cooperative matrix, for a device that has none (MoltenVK)
nr_shader(gemm_portable.spv        src/gpu/gemm_portable.comp)
nr_shader(gemm_portable_tiled.spv  src/gpu/gemm_portable.comp -DRM=2 -DRN=2)
nr_shader(gemm_portable_wide.spv   src/gpu/gemm_portable.comp -DRM=2 -DRN=4)   # 16x64, found by libxmx beside the tiled build
nr_shader(gemm_portable_desc.spv   src/gpu/gemm_portable_desc.comp)
nr_shader(gemm_portable_batched.spv src/gpu/gemm_portable_desc.comp -DBATCHED)
# the fused passes (notes/improve-fusions.md), each with a twin for a device without
# matrix units that sums in the portable GEMM's order
nr_shader(window_attention.spv     src/gpu/window_attention.comp)
nr_shader(window_attention_portable.spv src/gpu/window_attention_portable.comp)
nr_shader(ffn_fused.spv            src/gpu/ffn_fused.comp)
nr_shader(ffn_fused_portable.spv   src/gpu/ffn_fused_portable.comp)
# improve-b.md: a 32-channel window block's attention half and a bottleneck block's
# attention in one pass each, and the integer staged GEMM, each with its portable twin
nr_shader(window_block.spv                src/gpu/window_block.comp)
nr_shader(window_block_portable.spv       src/gpu/window_block_portable.comp)
nr_shader(global_attention.spv            src/gpu/global_attention.comp)
nr_shader(global_attention_portable.spv   src/gpu/global_attention_portable.comp)
nr_shader(gemm_staged_int8.spv            src/gpu/gemm_staged_int8.comp)
nr_shader(gemm_staged_int8_portable.spv   src/gpu/gemm_staged_int8_portable.comp)
nr_shader(half_probe.spv           src/bench/half_probe.comp)
add_custom_target(shaders ALL DEPENDS ${NR_SPV})

set(NR_SHADER_OBJECTS)
if(NR_EMBED_SHADERS)
  list(LENGTH NR_SPV_HEADERS NR_SPV_COUNT)
  set(table "${NR_SHADER_EMBED_DIR}/nr_shaders_embedded.c")
  file(WRITE "${table}.in"
"/* generated by CMake: the ${NR_SPV_COUNT} SPIR-V modules, each from bin2c. Do not edit. */
#include \"nr_shaders_embedded.h\"

${NR_SPV_INCLUDES}
const struct nr_embedded_shader nr_embedded_shaders[] = {
${NR_SPV_ROWS}};
const size_t nr_embedded_shader_count = ${NR_SPV_COUNT};
")
  configure_file("${table}.in" "${table}" COPYONLY)
  set_source_files_properties("${table}" PROPERTIES OBJECT_DEPENDS "${NR_SPV_HEADERS}")
  add_library(nr_shaders_blob OBJECT "${table}")
  target_include_directories(nr_shaders_blob PRIVATE src/gpu "${NR_SHADER_EMBED_DIR}")
  target_compile_definitions(nr_shaders_blob PRIVATE NR_EMBEDDED_SHADERS=1)
  set_target_properties(nr_shaders_blob PROPERTIES POSITION_INDEPENDENT_CODE ON)
  nr_no_ipo(nr_shaders_blob)
  set(NR_SHADER_OBJECTS $<TARGET_OBJECTS:nr_shaders_blob>)
  message(STATUS "embedding ${NR_SPV_COUNT} SPIR-V modules into libxmx and gemm_runner")
endif()

# ---------------------------------------------------------------------------------------
# the compute runtime
# ---------------------------------------------------------------------------------------

add_library(xmx SHARED src/gpu/libxmx.c ${NR_SHADER_OBJECTS})
target_include_directories(xmx PRIVATE ${Vulkan_INCLUDE_DIRS} src/gpu)
target_link_libraries(xmx PRIVATE ${NR_XMX_VULKAN})
target_compile_options(xmx PRIVATE ${NR_WARN_FLAGS})
nr_place(xmx)

# What the GPU's cooperative matrix offers — the first thing to run on a new machine or driver.
add_executable(coopmat_probe src/probe/coopmat_probe.c)
target_include_directories(coopmat_probe PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(coopmat_probe PRIVATE ${NR_XMX_VULKAN})
nr_executable(coopmat_probe)

add_executable(gemm_runner src/gpu/gemm_runner.c ${NR_SHADER_OBJECTS})
target_include_directories(gemm_runner PRIVATE ${Vulkan_INCLUDE_DIRS} src/gpu)
target_link_libraries(gemm_runner PRIVATE ${NR_XMX_VULKAN})
target_compile_options(gemm_runner PRIVATE ${NR_WARN_FLAGS})
nr_executable(gemm_runner)
if(NR_EMBED_SHADERS)
  target_compile_definitions(xmx PRIVATE NR_EMBEDDED_SHADERS=1)
  target_compile_definitions(gemm_runner PRIVATE NR_EMBEDDED_SHADERS=1)
endif()

# ---------------------------------------------------------------------------------------
# the compute runtime on Metal (Apple only): libmetalmx
# ---------------------------------------------------------------------------------------
#
# Each .metal compiles to AIR with the Metal compiler from the active Xcode / command line
# tools, the five link into nr_shaders.metallib in the output directory (kept there for
# XMX_METALLIB= experiments), bin2c writes the metallib as a C array, and the array is
# compiled into libmetalmx. The shaders are built with fast math and contraction off: with
# either on, the vendor's half rounding points move (notes/phase67, notes/phase74).

if(NR_BUILD_METAL)
  enable_language(OBJC)
  find_program(NR_XCRUN xcrun DOC "Apple's tool runner, for the Metal compiler")
  if(NOT NR_XCRUN)
    message(FATAL_ERROR "xcrun not found; the Metal compiler needs Xcode or the command line tools (or -DNR_BUILD_METAL=OFF)")
  endif()
  if(NOT NR_BIN2C)
    message(FATAL_ERROR "NR_BUILD_METAL needs bin2c to embed the metallib; name a host build with -DNR_BIN2C=")
  endif()
  set(NR_METAL_DIR "${CMAKE_CURRENT_BINARY_DIR}/metal")
  file(MAKE_DIRECTORY "${NR_METAL_DIR}")
  set(NR_METAL_FLAGS -std=metal3.1 -fno-fast-math -ffp-contract=off -Wall "-I${PROJECT_SOURCE_DIR}/src/gpu/metal")
  set(NR_AIR)
  foreach(stem resident history attention gemm_portable gemm_simd window_attention ffn_fused
               window_block global_attention gemm_int8 half_probe)
    add_custom_command(OUTPUT "${NR_METAL_DIR}/${stem}.air"
      COMMAND "${NR_XCRUN}" -sdk macosx metal ${NR_METAL_FLAGS} -c -o "${NR_METAL_DIR}/${stem}.air"
              "${PROJECT_SOURCE_DIR}/src/gpu/metal/${stem}.metal"
      DEPENDS "${PROJECT_SOURCE_DIR}/src/gpu/metal/${stem}.metal" "${PROJECT_SOURCE_DIR}/src/gpu/metal/nr_metal.h" "${PROJECT_SOURCE_DIR}/src/gpu/metal/nr_epilogue.h"
      COMMENT "metal ${stem}.air" VERBATIM)
    list(APPEND NR_AIR "${NR_METAL_DIR}/${stem}.air")
  endforeach()
  set(NR_METALLIB "${NR_OUTPUT_DIR}/nr_shaders.metallib")
  add_custom_command(OUTPUT "${NR_METALLIB}"
    COMMAND "${NR_XCRUN}" -sdk macosx metallib -o "${NR_METALLIB}" ${NR_AIR}
    DEPENDS ${NR_AIR} COMMENT "metallib nr_shaders.metallib" VERBATIM)
  add_custom_command(OUTPUT "${NR_METAL_DIR}/nr_metallib.h"
    COMMAND ${NR_BIN2C} "${NR_METALLIB}" "${NR_METAL_DIR}/nr_metallib.h" nr_metallib
    DEPENDS "${NR_METALLIB}" ${NR_BIN2C} COMMENT "bin2c nr_metallib.h" VERBATIM)
  add_custom_target(metal_shaders ALL DEPENDS "${NR_METALLIB}")

  # The same sources as Metal 4 (NR_METAL4: the GEMMs' K loop on Metal Performance
  # Primitives' matmul2d), a second library libmetalmx loads on a device and OS in
  # MTLGPUFamilyMetal4 and keeps the Metal 3.1 one for everything else. Built only when this
  # SDK's compiler takes -std=metal4.0 (Xcode 26 and later); an older SDK builds the Metal 3
  # library alone, and the runtime then has nothing else to choose.
  set(NR_METAL4_HEADER)
  set(NR_METAL4_DEFINES)
  option(NR_METAL4 "libmetalmx: also embed a Metal 4 metallib, used where the device and OS have Metal 4" ON)
  if(NR_METAL4)
    set(probe_dir "${CMAKE_CURRENT_BINARY_DIR}/metal4_probe")
    file(MAKE_DIRECTORY "${probe_dir}")
    file(WRITE "${probe_dir}/probe.metal"
      "#include <metal_stdlib>\n#include <metal_tensor>\n#include <MetalPerformancePrimitives/MetalPerformancePrimitives.h>\nkernel void probe(device float *a [[buffer(0)]]) { a[0] = 1.0f; }\n")
    execute_process(COMMAND "${NR_XCRUN}" -sdk macosx metal -std=metal4.0 -c -o "${probe_dir}/probe.air" "${probe_dir}/probe.metal"
                    RESULT_VARIABLE metal4_rc OUTPUT_QUIET ERROR_QUIET)
    if(NOT metal4_rc EQUAL 0)
      message(STATUS "libmetalmx: this SDK's Metal compiler has no -std=metal4.0; the Metal 3.1 library alone")
      set(NR_METAL4 OFF CACHE BOOL "" FORCE)
    endif()
  endif()
  if(NR_METAL4)
    set(NR_METAL4_DIR "${CMAKE_CURRENT_BINARY_DIR}/metal4")
    file(MAKE_DIRECTORY "${NR_METAL4_DIR}")
    set(NR_METAL4_FLAGS -std=metal4.0 -DNR_METAL4 -fno-fast-math -ffp-contract=off -Wall "-I${PROJECT_SOURCE_DIR}/src/gpu/metal")
    set(NR_AIR4)
    foreach(stem resident history attention gemm_portable gemm_simd window_attention ffn_fused
                 window_block global_attention gemm_int8 half_probe)
      add_custom_command(OUTPUT "${NR_METAL4_DIR}/${stem}.air"
        COMMAND "${NR_XCRUN}" -sdk macosx metal ${NR_METAL4_FLAGS} -c -o "${NR_METAL4_DIR}/${stem}.air"
                "${PROJECT_SOURCE_DIR}/src/gpu/metal/${stem}.metal"
        DEPENDS "${PROJECT_SOURCE_DIR}/src/gpu/metal/${stem}.metal" "${PROJECT_SOURCE_DIR}/src/gpu/metal/nr_metal.h" "${PROJECT_SOURCE_DIR}/src/gpu/metal/nr_epilogue.h"
        COMMENT "metal (Metal 4) ${stem}.air" VERBATIM)
      list(APPEND NR_AIR4 "${NR_METAL4_DIR}/${stem}.air")
    endforeach()
    set(NR_METALLIB4 "${NR_OUTPUT_DIR}/nr_shaders4.metallib")
    add_custom_command(OUTPUT "${NR_METALLIB4}"
      COMMAND "${NR_XCRUN}" -sdk macosx metallib -o "${NR_METALLIB4}" ${NR_AIR4}
      DEPENDS ${NR_AIR4} COMMENT "metallib nr_shaders4.metallib" VERBATIM)
    add_custom_command(OUTPUT "${NR_METAL4_DIR}/nr_metallib4.h"
      COMMAND ${NR_BIN2C} "${NR_METALLIB4}" "${NR_METAL4_DIR}/nr_metallib4.h" nr_metallib4
      DEPENDS "${NR_METALLIB4}" ${NR_BIN2C} COMMENT "bin2c nr_metallib4.h" VERBATIM)
    add_custom_target(metal4_shaders ALL DEPENDS "${NR_METALLIB4}")
    set(NR_METAL4_HEADER "${NR_METAL4_DIR}/nr_metallib4.h")
    set(NR_METAL4_DEFINES NR_EMBEDDED_METALLIB4=1)
  endif()

  add_library(metalmx SHARED src/gpu/libmetalmx.m "${NR_METAL_DIR}/nr_metallib.h" ${NR_METAL4_HEADER})
  target_include_directories(metalmx PRIVATE src/gpu "${NR_METAL_DIR}" "${NR_METAL4_DIR}")
  target_compile_definitions(metalmx PRIVATE NR_EMBEDDED_METALLIB=1 ${NR_METAL4_DEFINES})
  target_compile_options(metalmx PRIVATE -fobjc-arc ${NR_WARN_FLAGS})
  target_link_libraries(metalmx PRIVATE "-framework Metal" "-framework Foundation")
  nr_place(metalmx)
  message(STATUS "libmetalmx: the compute runtime on Metal, shaders compiled into it (NR_GPU_BACKEND=metal selects it)")
endif()

# ---------------------------------------------------------------------------------------
# the compute runtime on Direct3D 12 (Windows only): libd3dmx
# ---------------------------------------------------------------------------------------
#
# Each HLSL kernel under src/gpu/d3d12/ compiles to DXIL with dxc — nine modules from five
# sources, the -D variants as the SPIR-V has them — into the output directory (kept there
# for XMX_DXIL_DIR= experiments), bin2c writes each as a C array, and the arrays with their
# table are compiled into libd3dmx. dxc signs a module only when dxil.dll sits beside it
# (the Windows SDK's and vcpkg's do; a cross build's dxc does not): an unsigned module runs
# only with developer mode on, and libd3dmx asks for the experimental shader models itself
# when it finds its modules unsigned (notes/phase75).

set(NR_DXIL_OBJECTS)
if(NR_BUILD_D3D12)
  if(NOT NR_BIN2C)
    message(FATAL_ERROR "NR_BUILD_D3D12 needs bin2c to embed the DXIL; name a host build with -DNR_BIN2C=")
  endif()
  set(NR_D3D12_DIR "${CMAKE_CURRENT_BINARY_DIR}/d3d12")
  file(MAKE_DIRECTORY "${NR_D3D12_DIR}")
  # -denorm preserve: float32 subnormals kept, as Mesa and Intel's Windows driver keep them
  # on the Vulkan path (notes/phase71); 16-bit floats keep theirs on Direct3D 12 regardless
  # (the native 16-bit shader ops' contract), which `xmx_preserve16()` reports there.
  set(NR_DXC_FLAGS -T cs_6_2 -E main -enable-16bit-types -HV 2021 -O3 -Qstrip_reflect -Qstrip_debug
      -denorm preserve "-I${PROJECT_SOURCE_DIR}/src/gpu/d3d12")
  set(NR_DXIL)
  set(NR_DXIL_HEADERS)
  set(NR_DXIL_INCLUDES "")
  set(NR_DXIL_ROWS "")
  function(nr_dxil name source)
    set(out "${NR_OUTPUT_DIR}/${name}.dxil")
    add_custom_command(OUTPUT "${out}"
      COMMAND "${NR_DXC}" ${NR_DXC_FLAGS} ${ARGN} -Fo "${out}" "${PROJECT_SOURCE_DIR}/src/gpu/d3d12/${source}"
      DEPENDS "${PROJECT_SOURCE_DIR}/src/gpu/d3d12/${source}" "${PROJECT_SOURCE_DIR}/src/gpu/d3d12/nr_d3d.hlsli"
              "${PROJECT_SOURCE_DIR}/src/gpu/d3d12/nr_epilogue.hlsli"
      COMMENT "dxc ${name}.dxil" VERBATIM)
    set(NR_DXIL ${NR_DXIL} "${out}" PARENT_SCOPE)
    set(header "${NR_D3D12_DIR}/dxil_${name}.h")
    add_custom_command(OUTPUT "${header}"
      COMMAND ${NR_BIN2C} "${out}" "${header}" dxil_${name}
      DEPENDS "${out}" ${NR_BIN2C}
      COMMENT "bin2c dxil_${name}.h" VERBATIM)
    set(NR_DXIL_HEADERS ${NR_DXIL_HEADERS} "${header}" PARENT_SCOPE)
    set(NR_DXIL_INCLUDES "${NR_DXIL_INCLUDES}#include \"dxil_${name}.h\"\n" PARENT_SCOPE)
    set(NR_DXIL_ROWS "${NR_DXIL_ROWS}    { \"${name}.dxil\", dxil_${name}, sizeof dxil_${name} },\n" PARENT_SCOPE)
  endfunction()

  nr_dxil(gemm_portable         gemm_portable.hlsl)
  nr_dxil(gemm_portable_tiled   gemm_portable.hlsl -DRM=2 -DRN=2)   # the 16x32 block, the gemm_tiled slot
  nr_dxil(gemm_portable_wide    gemm_portable.hlsl -DRM=2 -DRN=4)   # 16x64, found by libd3dmx beside the tiled build
  nr_dxil(gemm_portable_desc    gemm_portable_desc.hlsl)
  nr_dxil(gemm_portable_batched gemm_portable_desc.hlsl -DBATCHED)
  nr_dxil(gemm_f16acc           gemm_portable_desc.hlsl -DACC16)
  nr_dxil(resident              resident.hlsl)
  nr_dxil(attention             attention.hlsl)
  nr_dxil(attention_ab          attention.hlsl -DSOFTMAX_AB)
  nr_dxil(history               history.hlsl)
  # the fused passes (notes/improve-fusions.md): only their multiply-add twins exist here
  nr_dxil(window_attention_portable        window_attention_portable.hlsl)
  nr_dxil(window_attention_portable_merged window_attention_portable.hlsl -DMERGED_OUTPUT=1)
  nr_dxil(ffn_fused_portable               ffn_fused_portable.hlsl)
  nr_dxil(attention_rows                   attention.hlsl -DROW_LANES=256)   # the whole-row softmax
  nr_dxil(window_block_portable            window_block_portable.hlsl)
  nr_dxil(global_attention_portable        global_attention_portable.hlsl)
  nr_dxil(gemm_staged_int8_portable        gemm_staged_int8_portable.hlsl)
  nr_dxil(half_probe                       half_probe.hlsl)   # the daemon's start-up probe
  add_custom_target(dxil_shaders ALL DEPENDS ${NR_DXIL})

  list(LENGTH NR_DXIL_HEADERS NR_DXIL_COUNT)
  set(dxil_table "${NR_D3D12_DIR}/nr_dxil_embedded.c")
  file(WRITE "${dxil_table}.in"
"/* generated by CMake: the ${NR_DXIL_COUNT} DXIL modules, each from bin2c. Do not edit. */
#include \"nr_dxil_embedded.h\"

${NR_DXIL_INCLUDES}
const struct nr_embedded_shader nr_embedded_dxil[] = {
${NR_DXIL_ROWS}};
const size_t nr_embedded_dxil_count = ${NR_DXIL_COUNT};
")
  configure_file("${dxil_table}.in" "${dxil_table}" COPYONLY)
  set_source_files_properties("${dxil_table}" PROPERTIES OBJECT_DEPENDS "${NR_DXIL_HEADERS}")
  add_library(nr_dxil_blob OBJECT "${dxil_table}")
  target_include_directories(nr_dxil_blob PRIVATE src/gpu "${NR_D3D12_DIR}")
  target_compile_definitions(nr_dxil_blob PRIVATE NR_EMBEDDED_DXIL=1)
  set_target_properties(nr_dxil_blob PROPERTIES POSITION_INDEPENDENT_CODE ON)
  nr_no_ipo(nr_dxil_blob)
  set(NR_DXIL_OBJECTS $<TARGET_OBJECTS:nr_dxil_blob>)

  # d3d12.dll and dxgi.dll are loaded at run time: no import library on the line
  add_library(d3dmx SHARED src/gpu/libd3dmx.c ${NR_DXIL_OBJECTS})
  target_include_directories(d3dmx PRIVATE src/gpu)
  target_compile_definitions(d3dmx PRIVATE NR_EMBEDDED_DXIL=1)
  target_compile_options(d3dmx PRIVATE ${NR_WARN_FLAGS})
  nr_place(d3dmx)
  message(STATUS "libd3dmx: the compute runtime on Direct3D 12, ${NR_DXIL_COUNT} DXIL modules compiled into it by ${NR_DXC} (NR_GPU_BACKEND=d3d12 selects it)")
endif()

# ---------------------------------------------------------------------------------------
# the host passes and the frame library
# ---------------------------------------------------------------------------------------

# nr_image.c is one object in two libraries: its own, for the Python, and the frame's.
add_library(nr_image_objects OBJECT src/ref/nr_image.c)
target_include_directories(nr_image_objects PRIVATE src/ref)
target_compile_options(nr_image_objects PRIVATE ${NR_NATIVE_FLAGS} ${NR_EXACT_FLAGS} ${NR_WARN_FLAGS})
# the CPU floor above: `-march=x86-64-v3` on x86-64, NR_F16C under MSVC (the F16C intrinsics)
target_compile_options(nr_image_objects PRIVATE ${NR_IMAGE_ARCH_FLAGS})
target_compile_definitions(nr_image_objects PRIVATE ${NR_IMAGE_ARCH_DEFINES})
# `-fno-trapping-math` changes no value, only whether a comparison may raise an exception
# flag nobody reads; it lets GCC turn the fused composition's clamps into selects and
# vectorise its pixels (`compose_encode_row`).
if(NOT MSVC)
  target_compile_options(nr_image_objects PRIVATE -fno-trapping-math)
endif()
set_target_properties(nr_image_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)
nr_no_ipo(nr_image_objects)

add_library(nr_image SHARED $<TARGET_OBJECTS:nr_image_objects>)
nr_place(nr_image)
# the passes split their rows across nr_image.c's own thread pool (pthreads, or Win32
# threads on Windows, which need nothing linked)
if(TARGET Threads::Threads)
  target_link_libraries(nr_image PRIVATE Threads::Threads)
endif()
# the style grade's truncation to half (floorf, fabsf)
if(UNIX AND NOT APPLE)
  target_link_libraries(nr_image PRIVATE m)
endif()

# The daemon's NumPy allocator on Windows, whose heap returns a frame's large blocks to the
# system for the next frame to fault in again (src/layer/nr_alloc.py): a PyDataMem_Handler
# that keeps them, installed by nr_daemon.py through NumPy's C API. Win32 only, by its code.
if(WIN32)
  add_library(nr_alloc SHARED src/layer/nr_alloc.c)
  target_compile_options(nr_alloc PRIVATE ${NR_WARN_FLAGS})
  nr_place(nr_alloc)
endif()

# ---------------------------------------------------------------------------------------
# the weights, as C: the slices in the weights directory, one object each
# ---------------------------------------------------------------------------------------
#
# bin2c (Rafael Kitover's, BSD-2, the one the VBA-M wx build embeds its resources with)
# writes a file as `const unsigned char NAME[] = { 0x.., ... }`. Compilers hold such an
# initializer one element at a time — clang peaks at 1.4 GB on a 16 MB file, so the whole
# 292 MB file in one array would want ~25 GB — which is why the weights live as a run of
# NR_EMBED_CHUNK_MB slices: each its own header from bin2c and its own translation unit (a
# typedef there checks the array is the size the table says), and a table listing them in
# order for the reader in nr_frame.c. All of that sits in NR_WEIGHTS_DIR and is compiled
# from there; the objects go into libnr_frame; the table's declarations are
# src/ref/nr_weights_embedded.h.
#
# With NR_BIN2C_WEIGHTS and the safetensors file, the directory is regenerated: `slice`
# cuts the file into NR_EMBED_CHUNK_MB pieces in the build tree, bin2c writes each piece's
# header into the directory, and the translation units and the table are rewritten to
# match — so the directory afterwards is the pre-processed form a build without the file
# compiles. Slices past the new count are removed.
set(NR_EMBED_OBJECTS)
if(NR_EMBED_WEIGHTS)
  set(NR_EMBED_SOURCES)
  if(NR_REGENERATE_WEIGHTS)
    file(MAKE_DIRECTORY "${NR_WEIGHTS_DIR}")
    set(NR_EMBED_SCRATCH "${CMAKE_CURRENT_BINARY_DIR}/weights")     # the .bin slices and the .c.in
    file(MAKE_DIRECTORY "${NR_EMBED_SCRATCH}")
    file(SIZE "${NR_WEIGHTS_FILE}" NR_WEIGHTS_SIZE)
    math(EXPR NR_EMBED_CHUNK "${NR_EMBED_CHUNK_MB} * 1024 * 1024")
    # The cut plan: the safetensors header's tensor ends, in order, and a slice closes before
    # the tensor that would take it past NR_EMBED_CHUNK (a tensor larger than that is a slice
    # of its own). No tensor straddles two slices, so the reader in nr_frame.c aliases every
    # one in place (`source_view`). The header is read here, at configure time, with a regex
    # over its JSON — the 8-byte length prefix first, little-endian.
    file(READ "${NR_WEIGHTS_FILE}" nr_hlen_hex OFFSET 0 LIMIT 8 HEX)
    set(nr_hlen 0)
    foreach(i RANGE 7 0 -1)
      math(EXPR pos "${i} * 2")
      string(SUBSTRING "${nr_hlen_hex}" ${pos} 2 byte)
      math(EXPR nr_hlen "${nr_hlen} * 256 + 0x${byte}")
    endforeach()
    file(READ "${NR_WEIGHTS_FILE}" nr_header OFFSET 8 LIMIT ${nr_hlen})
    math(EXPR nr_base "8 + ${nr_hlen}")
    string(REGEX MATCHALL "\"data_offsets\":\\[[0-9]+,[0-9]+\\]" nr_ranges "${nr_header}")
    set(nr_ends)
    foreach(range ${nr_ranges})
      string(REGEX REPLACE ".*,([0-9]+)\\]" "\\1" end "${range}")
      math(EXPR end "${nr_base} + ${end}")
      list(APPEND nr_ends ${end})
    endforeach()
    list(SORT nr_ends COMPARE NATURAL)
    list(REMOVE_DUPLICATES nr_ends)
    string(REGEX MATCH "\"prepared\":\"v[0-9]+\"" nr_prepared_tag "${nr_header}")
    if(NOT nr_prepared_tag)
      message(WARNING "${NR_WEIGHTS_FILE} is not a prepared file: the directory will work, but without the derived tensors nr_frame_prepare adds (cmake --build . --target prepare_weights, then reconfigure)")
    endif()
    # the plan: [start, end) per slice, greedy over whole tensors
    set(nr_cuts)
    set(slice_start 0)
    set(prev_end ${nr_base})
    foreach(end ${nr_ends})
      math(EXPR span "${end} - ${slice_start}")
      math(EXPR have "${prev_end} - ${slice_start}")
      if(span GREATER NR_EMBED_CHUNK AND have GREATER 0)
        list(APPEND nr_cuts ${prev_end})
        set(slice_start ${prev_end})
      endif()
      set(prev_end ${end})
    endforeach()
    if(NOT prev_end EQUAL NR_WEIGHTS_SIZE)
      message(FATAL_ERROR "${NR_WEIGHTS_FILE}: the header's tensors end at byte ${prev_end}, the file at ${NR_WEIGHTS_SIZE}; not a safetensors file this build understands")
    endif()
    list(APPEND nr_cuts ${NR_WEIGHTS_SIZE})
    list(LENGTH nr_cuts NR_EMBED_COUNT)
    set(NR_EMBED_TABLE_EXTERNS "")
    set(NR_EMBED_TABLE_ROWS "")
    set(offset 0)
    set(k 0)
    foreach(cut ${nr_cuts})
      math(EXPR length "${cut} - ${offset}")
      set(symbol "${NR_EMBED_SYMBOL}_${k}")
      set(bin "${NR_EMBED_SCRATCH}/${symbol}.bin")
      set(header "${NR_WEIGHTS_DIR}/${symbol}.h")
      set(source "${NR_WEIGHTS_DIR}/${symbol}.c")
      add_custom_command(OUTPUT "${bin}"
        COMMAND ${NR_SLICE} "${NR_WEIGHTS_FILE}" "${bin}" ${offset} ${length}
        DEPENDS "${NR_WEIGHTS_FILE}" ${NR_SLICE}
        COMMENT "slice ${symbol}: bytes ${offset} + ${length} of the weights" VERBATIM)
      add_custom_command(OUTPUT "${header}"
        COMMAND ${NR_BIN2C} "${bin}" "${header}" ${symbol}
        DEPENDS "${bin}" ${NR_BIN2C}
        COMMENT "bin2c ${symbol}.h -> ${NR_WEIGHTS_DIR}" VERBATIM)
      # the translation unit for this slice: the generated header, and a check that the
      # array is the length the table below was written with. Written through
      # configure_file so an unchanged slice keeps its timestamp and is not recompiled.
      file(WRITE "${NR_EMBED_SCRATCH}/${symbol}.c.in"
"/* generated by CMake: slice ${k} of the embedded weights. Do not edit. */
#include <stddef.h>
#include \"${symbol}.h\"
typedef char ${symbol}_size_check[sizeof ${symbol} == ${length} ? 1 : -1];
")
      configure_file("${NR_EMBED_SCRATCH}/${symbol}.c.in" "${source}" COPYONLY)
      set_source_files_properties("${source}" PROPERTIES OBJECT_DEPENDS "${header}")
      list(APPEND NR_EMBED_SOURCES "${source}")
      string(APPEND NR_EMBED_TABLE_EXTERNS "extern const unsigned char ${symbol}[];\n")
      string(APPEND NR_EMBED_TABLE_ROWS "    { ${symbol}, ${length} },\n")
      math(EXPR offset "${offset} + ${length}")
      math(EXPR k "${k} + 1")
    endforeach()
    # a shorter file leaves slices behind; they would still compile and confuse a reader
    file(GLOB stale "${NR_WEIGHTS_DIR}/${NR_EMBED_SYMBOL}_*.c" "${NR_WEIGHTS_DIR}/${NR_EMBED_SYMBOL}_*.h")
    foreach(old ${stale})
      get_filename_component(name "${old}" NAME_WE)
      string(REGEX REPLACE "^${NR_EMBED_SYMBOL}_" "" n "${name}")
      if(n GREATER_EQUAL NR_EMBED_COUNT)
        file(REMOVE "${old}")
      endif()
    endforeach()
    get_filename_component(NR_WEIGHTS_FILE_NAME "${NR_WEIGHTS_FILE}" NAME)
    file(WRITE "${NR_EMBED_SCRATCH}/nr_weights_embedded.c.in"
"/* generated by CMake: the ${NR_EMBED_COUNT} slices of the embedded weights, in file order,
 * each closed at a tensor boundary (no tensor straddles two: nr_frame.c reads every one in place).
 * Source: ${NR_WEIGHTS_FILE_NAME} (${NR_WEIGHTS_SIZE} bytes, ${nr_prepared_tag}), slices of at most ${NR_EMBED_CHUNK_MB} MB. Do not edit. */
#include \"nr_weights_embedded.h\"

${NR_EMBED_TABLE_EXTERNS}
const struct nr_embedded_chunk nr_embedded_weights_chunks[] = {
${NR_EMBED_TABLE_ROWS}};
const size_t nr_embedded_weights_chunk_count = ${NR_EMBED_COUNT};
const size_t nr_embedded_weights_size = ${NR_WEIGHTS_SIZE};
")
    configure_file("${NR_EMBED_SCRATCH}/nr_weights_embedded.c.in" "${NR_WEIGHTS_DIR}/nr_weights_embedded.c" COPYONLY)
    math(EXPR NR_WEIGHTS_MB "${NR_WEIGHTS_SIZE} / 1048576")
    message(STATUS "regenerating ${NR_WEIGHTS_DIR} from ${NR_WEIGHTS_FILE} (${NR_WEIGHTS_MB} MB, ${nr_prepared_tag}): ${NR_EMBED_COUNT} slices of at most ${NR_EMBED_CHUNK_MB} MB cut at tensor boundaries, ${NR_EMBED_JOBS} compiled at a time")
  else()
    # the pre-processed directory as it is: every slice that has a header, in order
    file(GLOB NR_EMBED_HEADERS CONFIGURE_DEPENDS "${NR_WEIGHTS_DIR}/${NR_EMBED_SYMBOL}_*.h")
    list(LENGTH NR_EMBED_HEADERS NR_EMBED_COUNT)
    math(EXPR last "${NR_EMBED_COUNT} - 1")
    foreach(k RANGE 0 ${last})
      set(symbol "${NR_EMBED_SYMBOL}_${k}")
      if(NOT EXISTS "${NR_WEIGHTS_DIR}/${symbol}.c" OR NOT EXISTS "${NR_WEIGHTS_DIR}/${symbol}.h")
        message(FATAL_ERROR "${NR_WEIGHTS_DIR}: slice ${k} of ${NR_EMBED_COUNT} is incomplete (${symbol}.c and .h); regenerate with -DNR_BIN2C_WEIGHTS=ON and the safetensors file, or -DNR_EMBED_WEIGHTS=OFF")
      endif()
      set_source_files_properties("${NR_WEIGHTS_DIR}/${symbol}.c" PROPERTIES OBJECT_DEPENDS "${NR_WEIGHTS_DIR}/${symbol}.h")
      list(APPEND NR_EMBED_SOURCES "${NR_WEIGHTS_DIR}/${symbol}.c")
    endforeach()
    message(STATUS "embedding the pre-processed weights in ${NR_WEIGHTS_DIR}: ${NR_EMBED_COUNT} slices, ${NR_EMBED_JOBS} compiled at a time")
  endif()
  add_library(nr_weights_blob OBJECT ${NR_EMBED_SOURCES} "${NR_WEIGHTS_DIR}/nr_weights_embedded.c")
  target_include_directories(nr_weights_blob PRIVATE src/ref "${NR_WEIGHTS_DIR}")
  target_compile_definitions(nr_weights_blob PRIVATE NR_EMBEDDED_WEIGHTS=1)
  set_target_properties(nr_weights_blob PROPERTIES POSITION_INDEPENDENT_CODE ON)
  # The slices are nothing but byte arrays, which gain nothing from LTO, and as ~240 MB of
  # bitcode they crash Apple's linker (ld-1230, signal 11) at the host's final link; they are
  # compiled to plain objects on every toolchain, not only on MSVC as nr_no_ipo does.
  set_target_properties(nr_weights_blob PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
  # each slice costs the compiler ~0.75 GB; the pool keeps a parallel build inside memory
  set_property(GLOBAL APPEND PROPERTY JOB_POOLS nr_embed=${NR_EMBED_JOBS})
  set_target_properties(nr_weights_blob PROPERTIES JOB_POOL_COMPILE nr_embed)
  set(NR_EMBED_OBJECTS $<TARGET_OBJECTS:nr_weights_blob>)
endif()

add_library(nr_frame SHARED src/ref/nr_frame.c $<TARGET_OBJECTS:nr_image_objects> ${NR_EMBED_OBJECTS})
target_include_directories(nr_frame PRIVATE src/ref)
target_compile_options(nr_frame PRIVATE ${NR_EXACT_FLAGS} ${NR_WARN_FLAGS})
target_link_libraries(nr_frame PRIVATE ${CMAKE_DL_LIBS})
if(TARGET Threads::Threads)
  target_link_libraries(nr_frame PRIVATE Threads::Threads)
endif()
if(NR_EMBED_WEIGHTS)
  target_compile_definitions(nr_frame PRIVATE NR_EMBEDDED_WEIGHTS=1)
endif()
if(NOT MSVC)
  target_link_libraries(nr_frame PRIVATE m)
endif()
nr_place(nr_frame)

# ---------------------------------------------------------------------------------------
# libdlssnr: the compute runtime and the frame library in one static archive, for a host
# that links the model in (VBA-M). nr_frame calls libxmx directly (NR_STATIC_XMX) instead
# of dlopen, and libxmx links no Vulkan library: it finds vkGetInstanceProcAddr in the
# process at run time (XMX_NO_VULKAN_LINK) — the host's, or the adopted instance's. The
# weights and the shaders are embedded exactly as in the shared libraries.
# ---------------------------------------------------------------------------------------
#
# On Apple the archive carries libmetalmx instead: the same entry points on Metal directly,
# the metallib compiled in, `-framework Metal -framework Foundation` as its public link
# interface, and no Vulkan anywhere in it. A host that shares a Vulkan device
# (`nr_frame_adopt_vulkan`) is refused there, since there is nothing Vulkan to adopt; the
# host lets the runtime open the Metal device itself. NR_BUILD_METAL=OFF gives the Vulkan
# archive on Apple as before.
if(NR_BUILD_METAL)
  add_library(dlssnr STATIC src/gpu/libmetalmx.m "${NR_METAL_DIR}/nr_metallib.h" ${NR_METAL4_HEADER}
    src/ref/nr_frame.c $<TARGET_OBJECTS:nr_image_objects> ${NR_EMBED_OBJECTS})
  target_include_directories(dlssnr
    PRIVATE src/gpu "${NR_METAL_DIR}" "${NR_METAL4_DIR}"
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/ref>)
  target_compile_definitions(dlssnr PRIVATE NR_STATIC_XMX=1 NR_STATIC_METAL=1 NR_EMBEDDED_METALLIB=1
                             ${NR_METAL4_DEFINES})
  set_source_files_properties(src/gpu/libmetalmx.m PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
  target_link_libraries(dlssnr PUBLIC "-framework Metal" "-framework Foundation")
  set(NR_DLSSNR_HOST_LIBS)                        # the test plays a host that brings no Vulkan
  message(STATUS "libdlssnr carries libmetalmx (Metal); -DNR_BUILD_METAL=OFF for the Vulkan archive")
elseif(NR_DLSSNR_D3D12)
  # the Direct3D 12 archive: libd3dmx's objects and the DXIL, no Vulkan and no import
  # library (d3d12.dll and dxgi.dll are loaded at run time); a host shares its device
  # through nr_frame_adopt_d3d12, and nr_frame_adopt_vulkan is refused
  add_library(dlssnr STATIC src/gpu/libd3dmx.c src/ref/nr_frame.c
    $<TARGET_OBJECTS:nr_image_objects> ${NR_EMBED_OBJECTS} ${NR_DXIL_OBJECTS})
  target_include_directories(dlssnr
    PRIVATE src/gpu
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/ref>)
  target_compile_definitions(dlssnr PRIVATE NR_STATIC_XMX=1 NR_STATIC_D3D12=1 NR_EMBEDDED_DXIL=1)
  set(NR_DLSSNR_HOST_LIBS)                        # the test plays a host that brings no Vulkan
  message(STATUS "libdlssnr carries libd3dmx (Direct3D 12); -DNR_DLSSNR_D3D12=OFF for the Vulkan archive")
else()
  add_library(dlssnr STATIC src/gpu/libxmx.c src/ref/nr_frame.c
    $<TARGET_OBJECTS:nr_image_objects> ${NR_EMBED_OBJECTS} ${NR_SHADER_OBJECTS})
  target_include_directories(dlssnr
    PRIVATE ${Vulkan_INCLUDE_DIRS} src/gpu
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/ref>)
  target_compile_definitions(dlssnr PRIVATE NR_STATIC_XMX=1 XMX_NO_VULKAN_LINK=1)
  if(NR_EMBED_SHADERS)
    target_compile_definitions(dlssnr PRIVATE NR_EMBEDDED_SHADERS=1)
  endif()
  set(NR_DLSSNR_HOST_LIBS ${NR_XMX_VULKAN})
  if(NR_BUILD_D3D12)
    message(STATUS "libdlssnr carries libxmx (Vulkan) beside libd3dmx.dll; -DNR_DLSSNR_D3D12=ON for the Direct3D 12 archive")
  else()
    message(STATUS "libdlssnr carries libxmx (Vulkan)")
  endif()
endif()
if(NR_EMBED_WEIGHTS)
  target_compile_definitions(dlssnr PRIVATE NR_EMBEDDED_WEIGHTS=1)
endif()
target_compile_options(dlssnr PRIVATE ${NR_EXACT_FLAGS} ${NR_WARN_FLAGS})
target_link_libraries(dlssnr PUBLIC ${CMAKE_DL_LIBS})
if(TARGET Threads::Threads)
  target_link_libraries(dlssnr PUBLIC Threads::Threads)   # nr_image.c's row pool
endif()
if(NOT MSVC)
  target_link_libraries(dlssnr PUBLIC m)
endif()
set_target_properties(dlssnr PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  PREFIX "lib"
  ARCHIVE_OUTPUT_DIRECTORY "${NR_OUTPUT_DIR}/$<0:>")

# The same C test, against the archive: the static path has to pass what the shared
# one passes. The Vulkan archive links no Vulkan library, so the test plays the host and
# links one (MoltenVK here, the loader elsewhere) for libxmx to find in the process; the
# Metal archive brings its frameworks itself.
add_executable(test_dlssnr src/ref/test_nr_frame.c)
target_include_directories(test_dlssnr PRIVATE src/ref)
target_compile_options(test_dlssnr PRIVATE ${NR_EXACT_FLAGS} ${NR_WARN_FLAGS})
target_link_libraries(test_dlssnr PRIVATE dlssnr ${NR_DLSSNR_HOST_LIBS})
nr_executable(test_dlssnr)

# nr_frame.py, the command, in C: PNG in and out through libpng.
if(PNG_FOUND)
  add_executable(nr_frame_cli src/ref/nr_frame_main.c)
  set_target_properties(nr_frame_cli PROPERTIES OUTPUT_NAME nr_frame)
  target_include_directories(nr_frame_cli PRIVATE src/ref)
  target_compile_options(nr_frame_cli PRIVATE ${NR_EXACT_FLAGS} ${NR_WARN_FLAGS})
  target_link_libraries(nr_frame_cli PRIVATE nr_frame PNG::PNG ${CMAKE_DL_LIBS})
  if(NOT MSVC)
    target_link_libraries(nr_frame_cli PRIVATE m)
  endif()
  nr_executable(nr_frame_cli)
else()
  message(STATUS "libpng not found: the nr_frame command is not built (the libraries and tests are)")
endif()

# live_rates.py for the C library: the daemon's frame, end to end, in one process.
add_executable(nr_frame_rates src/bench/nr_frame_rates.c)
target_include_directories(nr_frame_rates PRIVATE src/ref)
target_compile_options(nr_frame_rates PRIVATE ${NR_WARN_FLAGS})
target_link_libraries(nr_frame_rates PRIVATE nr_frame)
if(NOT MSVC)
  target_link_libraries(nr_frame_rates PRIVATE m)
endif()
nr_executable(nr_frame_rates)

# The weights compiled into libnr_frame, back out as the file the Python graph reads
# (`work/mlxw/dlssnr-prepared.safetensors`, since the compiled-in weights are the prepared
# format): a checkout without `work/` has them nowhere else.
# Not on Windows: the chunk table is data, which WINDOWS_EXPORT_ALL_SYMBOLS does not export.
if(NR_EMBED_WEIGHTS AND NOT WIN32)
  add_executable(dump_embedded_weights src/tools/dump_embedded_weights.c)
  target_include_directories(dump_embedded_weights PRIVATE src/ref)
  target_compile_options(dump_embedded_weights PRIVATE ${NR_WARN_FLAGS})
  target_link_libraries(dump_embedded_weights PRIVATE nr_frame)
  nr_executable(dump_embedded_weights)
endif()

# `prepare_weights`: the prepared file from the logical one (or from the compiled-in weights
# when there is no logical file), where NR_BIN2C_WEIGHTS looks for it first. Not built by
# default; a reconfigure with -DNR_BIN2C_WEIGHTS=ON afterwards regenerates the directory.
set(NR_PREPARED_FILE "${NR_WORK_DIR}/mlxw/dlssnr-prepared.safetensors")
set(nr_prepare_source)
foreach(candidate "${PROJECT_SOURCE_DIR}/dlssnr-logical.safetensors" "${NR_WORK_DIR}/mlxw/dlssnr-logical.safetensors")
  if(EXISTS "${candidate}" AND NOT nr_prepare_source)
    set(nr_prepare_source --weights "${candidate}")
  endif()
endforeach()
add_custom_target(prepare_weights
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${NR_WORK_DIR}/mlxw"
  COMMAND $<TARGET_FILE:nr_frame_cli> --prepare "${NR_PREPARED_FILE}" ${nr_prepare_source}
  DEPENDS nr_frame_cli
  COMMENT "nr_frame --prepare ${NR_PREPARED_FILE} ${nr_prepare_source}" VERBATIM)

add_executable(test_nr_frame src/ref/test_nr_frame.c)
target_include_directories(test_nr_frame PRIVATE src/ref)
target_compile_options(test_nr_frame PRIVATE ${NR_EXACT_FLAGS} ${NR_WARN_FLAGS})
target_link_libraries(test_nr_frame PRIVATE nr_frame)
if(NOT MSVC)
  target_link_libraries(test_nr_frame PRIVATE m)
endif()
nr_executable(test_nr_frame)

# ---------------------------------------------------------------------------------------
# the Vulkan layer and its tests (POSIX)
# ---------------------------------------------------------------------------------------

if(NR_BUILD_LAYER)
  # The layer talks to the daemon over a Unix socket on POSIX and a named pipe on Windows
  # (src/layer/nr_transport.h), and spawns the daemon itself under NR_LAYER_SPAWN=1 (posix_spawn,
  # CreateProcess). On Windows the loader asks a layer DLL for three exports, which MSVC gives
  # only when told: src/layer/nr_layer.def. The tests beside it are POSIX.
  add_library(nr_layer SHARED src/layer/nr_layer.c)
  target_include_directories(nr_layer PRIVATE ${Vulkan_INCLUDE_DIRS})
  target_link_libraries(nr_layer PRIVATE ${NR_LOADER})
  if(NOT WIN32)
    target_link_libraries(nr_layer PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
  elseif(MSVC OR CMAKE_C_SIMULATE_ID MATCHES "MSVC")
    target_sources(nr_layer PRIVATE src/layer/nr_layer.def)
  endif()
  target_compile_options(nr_layer PRIVATE ${NR_WARN_FLAGS})
  nr_place(nr_layer)

  # The 32-bit layer, for 32-bit games under Proton, wherever a 32-bit compiler and a 32-bit
  # Vulkan loader are installed: the Makefile's LAYER32, with NR_BUILD_LAYER32 to override.
  if(NOT WIN32 AND NOT APPLE AND NOT ANDROID)
    include(CheckCSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS -m32)
    set(CMAKE_REQUIRED_LINK_OPTIONS -m32)
    set(CMAKE_REQUIRED_LIBRARIES vulkan)
    check_c_source_compiles("int main(void) { return 0; }" NR_HAVE_VULKAN32)
    unset(CMAKE_REQUIRED_FLAGS)
    unset(CMAKE_REQUIRED_LINK_OPTIONS)
    unset(CMAKE_REQUIRED_LIBRARIES)
    option(NR_BUILD_LAYER32 "Build the 32-bit layer too, for 32-bit games under Proton"
           ${NR_HAVE_VULKAN32})
    if(NR_BUILD_LAYER32)
      add_library(nr_layer32 SHARED src/layer/nr_layer.c)
      target_include_directories(nr_layer32 PRIVATE ${Vulkan_INCLUDE_DIRS})
      target_compile_options(nr_layer32 PRIVATE ${NR_WARN_FLAGS} -m32)
      target_link_options(nr_layer32 PRIVATE -m32)
      target_link_libraries(nr_layer32 PRIVATE vulkan ${CMAKE_DL_LIBS})
      nr_place(nr_layer32)
    endif()
  endif()
endif()
if(NR_BUILD_LAYER AND NOT WIN32)
  foreach(test test_layer_loader test_present)
    add_executable(${test} src/layer/${test}.c)
    target_include_directories(${test} PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(${test} PRIVATE ${NR_LOADER})
    target_compile_options(${test} PRIVATE ${NR_WARN_FLAGS})
    nr_executable(${test})
  endforeach()
  # these two include nr_layer.c directly
  foreach(test test_settled test_exchange)
    add_executable(${test} src/layer/${test}.c)
    target_include_directories(${test} PRIVATE ${Vulkan_INCLUDE_DIRS} src/layer)
    target_link_libraries(${test} PRIVATE ${NR_LOADER} Threads::Threads)
    target_compile_options(${test} PRIVATE ${NR_WARN_FLAGS})
    nr_executable(${test})
  endforeach()
endif()

# ---------------------------------------------------------------------------------------
# tests: what `make test` runs, for ctest
# ---------------------------------------------------------------------------------------

if(NR_BUILD_TESTS)
  enable_testing()
  # every test is told where this build put things, so a `work/` filled by the Makefile
  # cannot be what ctest exercises
  set(NR_TEST_ENV "NR_BUILD_DIR=${NR_OUTPUT_DIR}")
  if(WIN32)
    # Windows' Python opens text in the ANSI code page, and the notes and the README the
    # checks read are UTF-8.
    list(APPEND NR_TEST_ENV "PYTHONUTF8=1")
  endif()
  if(APPLE AND NR_XMX_LINK_MOLTENVK)
    list(APPEND NR_TEST_ENV "VK_DRIVER_FILES=${NR_OUTPUT_DIR}/MoltenVK_icd.json")
  endif()
  function(nr_ctest name)
    add_test(NAME ${name} COMMAND ${ARGN} WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "${NR_TEST_ENV}")
  endfunction()
  if(NR_BUILD_LAYER AND NOT WIN32)
    nr_ctest(settled "${NR_OUTPUT_DIR}/test_settled")
  endif()
  nr_ctest(nr_frame_c "${NR_OUTPUT_DIR}/test_nr_frame" --reference "${NR_OUTPUT_DIR}/nr_frame_reference.bin")
  if(Python3_Interpreter_FOUND)
    set(PY "${Python3_EXECUTABLE}")
    set(NR_PY_TESTS
      src/layer/test_daemon.py src/layer/test_ui_mask.py src/layer/test_present.py
      src/layer/test_temporal.py src/layer/test_toggle.py src/layer/test_panel.py
      src/tools/publish_check.py src/tools/claims_check.py src/tools/build_check.py
      src/gpu/test_portable.py src/gpu/test_gemm_contract.py src/gpu/test_denorm.py
      src/gpu/test_input_fp16.py
      src/gpu/test_compact_head.py src/gpu/test_epilogue.py src/gpu/test_specialization.py
      src/gpu/test_softmax_pack.py src/gpu/test_qkv_fusion.py src/gpu/test_scratch_arena.py
      src/gpu/test_graph.py src/gpu/test_frame_execution.py src/gpu/test_resident.py
      src/gpu/test_ffn_fused.py
      src/gpu/test_staged_partial.py
      src/gpu/test_glue.py
      src/gpu/test_gemm_qkv.py
      src/gpu/test_window_attention.py
      src/gpu/test_gemm_residual.py
      src/gpu/test_window_residual.py
      src/gpu/test_int8_quant.py
      src/gpu/test_gemm_int8.py
      src/gpu/test_gemm_int8_staged.py
      src/gpu/test_window_block.py
      src/gpu/test_global_attention.py
      src/gpu/test_staged32.py
      src/gpu/test_staged_packed.py
      src/gpu/test_joint_qkv.py
      src/tools/test_release.py
      src/ref/test_frame_cache.py src/ref/test_native_image.py src/ref/test_nr_model.py
      src/ref/test_temporal_controls.py)
    foreach(script ${NR_PY_TESTS})
      get_filename_component(name "${script}" NAME_WE)
      if(script MATCHES "src/layer/" AND (NOT NR_BUILD_LAYER OR WIN32))
        continue()      # the layer's tests are POSIX: a stand-in daemon on a Unix socket
      endif()
      nr_ctest(${name} "${PY}" "${PROJECT_SOURCE_DIR}/${script}")
    endforeach()
    if(NR_BUILD_LAYER AND NOT WIN32)
      # the present test's negative controls: each disables one wait and expects the check to fail
      nr_ctest(test_present_negative "${PY}" "${PROJECT_SOURCE_DIR}/src/layer/test_present.py" --negative-control)
      set_tests_properties(test_present_negative PROPERTIES SKIP_RETURN_CODE 77)
    endif()
    if(WIN32)
      # the Windows link between layer and daemon: the allocator, the named pipe, a daemon
      # that ends with its game, and the vulkan-1.dll proxy (each skips where it cannot run)
      foreach(name alloc pipe spawn_exit vulkan_proxy)
        nr_ctest(test_${name} "${PY}" "${PROJECT_SOURCE_DIR}/src/layer/test_${name}.py")
        set_tests_properties(test_${name} PROPERTIES SKIP_RETURN_CODE 77)
      endforeach()
    endif()
    # the host passes at other thread counts: nr_image.c's pool splits rows, so the bytes
    # must not move with the count (the default run above uses one thread per core)
    foreach(threads 1 3)
      add_test(NAME test_native_image_threads${threads} COMMAND "${PY}" "${PROJECT_SOURCE_DIR}/src/ref/test_native_image.py" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
      set_tests_properties(test_native_image_threads${threads} PROPERTIES ENVIRONMENT "NR_HOST_THREADS=${threads};${NR_TEST_ENV}")
    endforeach()
    # the profiler's kind tables, read out of the shaders: two passes sharing a number fail
    nr_ctest(profile_tables "${PY}" "${PROJECT_SOURCE_DIR}/src/bench/frame_profile.py" --tables)
    # the batched feed-forward schedule against the plain one, on the device
    nr_ctest(test_ffn_batch "${PY}" "${PROJECT_SOURCE_DIR}/src/gpu/test_ffn_batch.py" --gpu)
    # the portable GEMM kernels, forced, on a device that has the matrix ones
    add_test(NAME test_portable_forced COMMAND "${PY}" "${PROJECT_SOURCE_DIR}/src/gpu/test_portable.py" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    set_tests_properties(test_portable_forced PROPERTIES ENVIRONMENT "XMX_PORTABLE=1;${NR_TEST_ENV}")
    # the Python side of the C frame test writes the reference the C side reads
    nr_ctest(test_nr_frame_py "${PY}" "${PROJECT_SOURCE_DIR}/src/ref/test_nr_frame_c.py" --reference "${NR_OUTPUT_DIR}/nr_frame_reference.bin")
    set_tests_properties(nr_frame_c PROPERTIES DEPENDS test_nr_frame_py)
    # the daemon's stateful frame through nr_frame_live, against the daemon's own pieces
    nr_ctest(test_nr_frame_live "${PY}" "${PROJECT_SOURCE_DIR}/src/ref/test_nr_frame_live.py")
    # the same GPU tests on libmetalmx, which every one of them reaches through
    # NR_GPU_BACKEND=metal; the C frame test writes and reads a reference of its own
    set(NR_RUNTIME_TESTS
      src/gpu/test_portable.py src/gpu/test_epilogue.py src/gpu/test_specialization.py
      src/gpu/test_softmax_pack.py src/gpu/test_qkv_fusion.py src/gpu/test_scratch_arena.py
      src/gpu/test_graph.py src/gpu/test_frame_execution.py src/gpu/test_resident.py
      src/gpu/test_gemm_contract.py src/gpu/test_denorm.py
      src/gpu/test_gemm_residual.py src/gpu/test_window_residual.py
      src/gpu/test_window_attention.py src/gpu/test_gemm_qkv.py src/gpu/test_ffn_fused.py
      src/gpu/test_glue.py src/gpu/test_joint_qkv.py src/gpu/test_compact_head.py
      src/gpu/test_input_fp16.py src/gpu/test_staged_partial.py
      src/gpu/test_window_block.py src/gpu/test_global_attention.py src/gpu/test_staged32.py
      src/gpu/test_staged_packed.py
      src/gpu/test_gemm_int8_staged.py)
    if(NR_BUILD_METAL)
      set(NR_METAL_TESTS ${NR_RUNTIME_TESTS})
      foreach(script ${NR_METAL_TESTS})
        get_filename_component(name "${script}" NAME_WE)
        add_test(NAME metal_${name} COMMAND "${PY}" "${PROJECT_SOURCE_DIR}/${script}" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
        set_tests_properties(metal_${name} PROPERTIES ENVIRONMENT "NR_GPU_BACKEND=metal;${NR_TEST_ENV}")
      endforeach()
      add_test(NAME metal_test_portable_forced COMMAND "${PY}" "${PROJECT_SOURCE_DIR}/src/gpu/test_portable.py" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
      set_tests_properties(metal_test_portable_forced PROPERTIES ENVIRONMENT "NR_GPU_BACKEND=metal;XMX_PORTABLE=1;${NR_TEST_ENV}")
      # the fused passes on the Metal runtime's portable kernels, which have twins of their own
      foreach(script src/gpu/test_window_block.py src/gpu/test_global_attention.py
                     src/gpu/test_glue.py src/gpu/test_ffn_fused.py src/gpu/test_window_attention.py)
        get_filename_component(name "${script}" NAME_WE)
        add_test(NAME metal_portable_${name} COMMAND "${PY}" "${PROJECT_SOURCE_DIR}/${script}" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
        set_tests_properties(metal_portable_${name} PROPERTIES ENVIRONMENT "NR_GPU_BACKEND=metal;XMX_PORTABLE=1;${NR_TEST_ENV}")
      endforeach()
      add_test(NAME metal_test_graph_staging COMMAND "${PY}" "${PROJECT_SOURCE_DIR}/src/gpu/test_graph.py" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
      set_tests_properties(metal_test_graph_staging PROPERTIES ENVIRONMENT "NR_GPU_BACKEND=metal;XMX_STAGING=1;${NR_TEST_ENV}")
      add_test(NAME metal_test_nr_frame_py COMMAND "${PY}" "${PROJECT_SOURCE_DIR}/src/ref/test_nr_frame_c.py" --reference "${NR_OUTPUT_DIR}/nr_frame_reference_metal.bin" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
      set_tests_properties(metal_test_nr_frame_py PROPERTIES ENVIRONMENT "NR_GPU_BACKEND=metal;${NR_TEST_ENV}")
      add_test(NAME metal_nr_frame_c COMMAND "${NR_OUTPUT_DIR}/test_nr_frame" --reference "${NR_OUTPUT_DIR}/nr_frame_reference_metal.bin" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
      set_tests_properties(metal_nr_frame_c PROPERTIES ENVIRONMENT "NR_GPU_BACKEND=metal;${NR_TEST_ENV}" DEPENDS metal_test_nr_frame_py)
      add_test(NAME metal_test_nr_frame_live COMMAND "${PY}" "${PROJECT_SOURCE_DIR}/src/ref/test_nr_frame_live.py" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
      set_tests_properties(metal_test_nr_frame_live PROPERTIES ENVIRONMENT "NR_GPU_BACKEND=metal;${NR_TEST_ENV}")
    endif()
    # and on libd3dmx through NR_GPU_BACKEND=d3d12 (Windows; every GEMM there is the
    # portable kernel already, so the forced-portable run is the same run and is not repeated)
    if(NR_BUILD_D3D12)
      foreach(script ${NR_RUNTIME_TESTS})
        get_filename_component(name "${script}" NAME_WE)
        add_test(NAME d3d12_${name} COMMAND "${PY}" "${PROJECT_SOURCE_DIR}/${script}" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
        set_tests_properties(d3d12_${name} PROPERTIES ENVIRONMENT "NR_GPU_BACKEND=d3d12;${NR_TEST_ENV}")
      endforeach()
      add_test(NAME d3d12_test_graph_staging COMMAND "${PY}" "${PROJECT_SOURCE_DIR}/src/gpu/test_graph.py" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
      set_tests_properties(d3d12_test_graph_staging PROPERTIES ENVIRONMENT "NR_GPU_BACKEND=d3d12;XMX_STAGING=1;${NR_TEST_ENV}")
      add_test(NAME d3d12_test_nr_frame_py COMMAND "${PY}" "${PROJECT_SOURCE_DIR}/src/ref/test_nr_frame_c.py" --reference "${NR_OUTPUT_DIR}/nr_frame_reference_d3d12.bin" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
      set_tests_properties(d3d12_test_nr_frame_py PROPERTIES ENVIRONMENT "NR_GPU_BACKEND=d3d12;${NR_TEST_ENV}")
      add_test(NAME d3d12_nr_frame_c COMMAND "${NR_OUTPUT_DIR}/test_nr_frame" --reference "${NR_OUTPUT_DIR}/nr_frame_reference_d3d12.bin" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
      set_tests_properties(d3d12_nr_frame_c PROPERTIES ENVIRONMENT "NR_GPU_BACKEND=d3d12;${NR_TEST_ENV}" DEPENDS d3d12_test_nr_frame_py)
      add_test(NAME d3d12_test_nr_frame_live COMMAND "${PY}" "${PROJECT_SOURCE_DIR}/src/ref/test_nr_frame_live.py" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
      set_tests_properties(d3d12_test_nr_frame_live PROPERTIES ENVIRONMENT "NR_GPU_BACKEND=d3d12;${NR_TEST_ENV}")
    endif()
  endif()
endif()

message(STATUS "outputs -> ${NR_OUTPUT_DIR}; compute runtime links ${NR_XMX_VULKAN}; layer: ${NR_BUILD_LAYER}; nr_frame command: ${PNG_FOUND}; embedded weights: ${NR_EMBED_WEIGHTS} (regenerated: ${NR_REGENERATE_WEIGHTS}); embedded shaders: ${NR_EMBED_SHADERS}; Metal runtime: ${NR_BUILD_METAL}; Direct3D 12 runtime: ${NR_BUILD_D3D12} (in libdlssnr: ${NR_DLSSNR_D3D12})")
message(STATUS "the Python finds this build with NR_BUILD_DIR=${NR_OUTPUT_DIR} (ctest sets it; without it the newest of work/ and build*/ wins)")
