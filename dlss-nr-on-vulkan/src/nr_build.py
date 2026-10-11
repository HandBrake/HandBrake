"""Where the build put the libraries, the executables and the SPIR-V.

Two builds make the same files under the same names: the Makefile into `work/`, CMake into
its own build directory (`cmake -S . -B build`, so `build/` by convention, and the
`.gitignore` already hides every `build*/`). Every Python loader, test and bench script
used to spell `ROOT / "work"` for itself; this is that fact written down once.

Resolution, in order:

1. `NR_BUILD_DIR` in the environment. CMake sets it for every ctest, the Makefile for its
   own `test` target, and the configure step prints the line to export for a shell.
2. The most recently built of `work/` and `build*/` — judged by the compute runtime's
   library, `libxmx`, since nothing runs without it. Newest wins so that switching between
   the two builds does not quietly run last week's binaries from the other one.
3. `work/`, so that a missing-library error at least names the place the Makefile fills.

The weights file is not here: it is the owner's, under `work/mlxw/` or the source root. A CMake
build given that file compiles it into libnr_frame (bin2c, `NR_WEIGHTS_FILE`), and then the C
side needs no file at all; the Python graph still reads the safetensors itself.
Nothing but the standard library is imported, so a single-file tool can share this.
"""
import os
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
# the platform's shared-library suffix, as both builds name them (`.dylib` on macOS, `.dll`
# on Windows, `.so` elsewhere); every library carries the `lib` prefix, Windows included
SUFFIX = {"darwin": ".dylib", "win32": ".dll"}.get(sys.platform, ".so")
EXE = ".exe" if sys.platform == "win32" else ""
MARKER = "libxmx" + SUFFIX


def weights_file(root=ROOT):
    """The weights the Python graph reads: `work/mlxw/dlssnr-prepared.safetensors` when it is
    there (the logical weights with the loaders' derived tensors, F32 first then F16 — what
    `nr_frame --prepare` writes and what the compiled-in weights are), else the logical file.
    NR_WEIGHTS names another. The path is returned whether or not it exists, so a caller can
    say which one is missing."""
    named = os.environ.get("NR_WEIGHTS")
    if named:
        return pathlib.Path(named)
    mlxw = pathlib.Path(root) / "work" / "mlxw"
    prepared, logical = mlxw / "dlssnr-prepared.safetensors", mlxw / "dlssnr-logical.safetensors"
    return prepared if prepared.exists() else logical


def candidates(root=ROOT):
    """The directories a build may have filled, `work/` first."""
    yield root / "work"
    for directory in sorted(root.glob("build*")):
        if directory.is_dir():
            yield directory


def build_dir(root=ROOT):
    forced = os.environ.get("NR_BUILD_DIR")
    if forced:
        return pathlib.Path(forced).expanduser().resolve()
    built = []
    for directory in candidates(root):
        try:
            built.append(((directory / MARKER).stat().st_mtime, directory))
        except OSError:
            continue
    if built:
        return max(built)[1]
    return root / "work"


BUILD_DIR = build_dir()


# the compute runtime each backend is: the same `xmx_*` entry points, three libraries
RUNTIMES = {"vulkan": "xmx", "metal": "metalmx", "d3d12": "d3dmx"}


def backend():
    """Which compute runtime the Python loads as `xmx`: `vulkan` (libxmx, the default
    everywhere), `metal` (libmetalmx, macOS only — the same entry points on Metal directly,
    `NR_GPU_BACKEND=metal`) or `d3d12` (libd3dmx, Windows only — the same entry points on
    Direct3D 12, `NR_GPU_BACKEND=d3d12`). Anything else is refused rather than guessed."""
    chosen = os.environ.get("NR_GPU_BACKEND", "vulkan").strip().lower() or "vulkan"
    if chosen not in RUNTIMES:
        raise ValueError("NR_GPU_BACKEND must be 'vulkan', 'metal' or 'd3d12', not %r" % chosen)
    if chosen == "metal" and sys.platform != "darwin":
        raise ValueError("NR_GPU_BACKEND=metal is macOS only; libmetalmx is built on Apple alone")
    if chosen == "d3d12" and sys.platform != "win32":
        raise ValueError("NR_GPU_BACKEND=d3d12 is Windows only; libd3dmx is built there alone")
    return chosen


_dll_directories = []          # the handles keep the directories on Windows' search path


def dll_directories():
    """Windows: the directories a library's own dependencies are found in. Python since 3.8
    looks only in the system folders and the ones added through `os.add_dll_directory`, so
    the build directory (vulkan-1 is a system DLL, but libnr_frame finds libxmx beside it) and
    `NR_DLL_PATH` — a toolchain's runtime, MinGW's libwinpthread or libgomp, wherever it is
    installed — are added once, the first time a library is asked for. Elsewhere a no-op."""
    if os.name != "nt" or _dll_directories:
        return _dll_directories
    extra = [p for p in os.environ.get("NR_DLL_PATH", "").split(os.pathsep) if p]
    for directory in [str(BUILD_DIR), *extra]:
        if os.path.isdir(directory):
            _dll_directories.append(os.add_dll_directory(directory))
    return _dll_directories


def library(name):
    """`libxmx.so`, `libnr_frame.dylib`, `libnr_layer.dll`: the platform's spelling of a library.
    `xmx` is the compute runtime, which `NR_GPU_BACKEND` redirects to libmetalmx (macOS) or
    libd3dmx (Windows). On Windows the DLL search path is set up on the way (`dll_directories`)."""
    if name == "xmx":
        name = RUNTIMES[backend()]
    dll_directories()
    return BUILD_DIR / ("lib" + name + SUFFIX)


def executable(name):
    return BUILD_DIR / (name + EXE)


def shader(name):
    return BUILD_DIR / (name if name.endswith(".spv") else name + ".spv")


def shader_arg(lib, name):
    """What to hand libxmx for a shader: its bare name when that library carries the module
    compiled in (the CMake build, bin2c), so it loads the embedded bytes; else the file this
    build wrote. `lib` is the ctypes handle of libxmx."""
    import ctypes
    name = name if name.endswith(".spv") else name + ".spv"
    probe = getattr(lib, "xmx_embedded_shader", None)
    if probe is not None:
        probe.argtypes = [ctypes.c_char_p]
        probe.restype = ctypes.c_size_t
        if probe(name.encode()) > 0:
            return name
    return str(shader(name))


if __name__ == "__main__":
    print(BUILD_DIR)
