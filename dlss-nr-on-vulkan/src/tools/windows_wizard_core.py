"""Local Windows release configuration, installation and observations.

Importing this module needs only Python's standard library. Checks never create a
Vulkan instance or run inference. Game/Steam process ownership belongs to the
launcher; this module never starts or kills either one.
"""
from __future__ import annotations

from collections import deque
import base64
from dataclasses import dataclass, field, replace
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import platform
import queue
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import threading
import time


SCHEMA_VERSION = 1
OWNER_NAME = ".windows-wizard-owner.json"
LIBRARIES = ("libxmx.dll", "libnr_image.dll", "libnr_alloc.dll")
SHADERS = frozenset((
    "attention.spv", "attention_rows.spv", "ffn_fused.spv", "gemm_batched.spv",
    "gemm_coopmat.spv", "gemm_resident.spv", "gemm_staged.spv", "gemm_staged32.spv",
    "gemm_staged32_deep.spv", "gemm_staged_int8.spv", "gemm_tiled.spv",
    "global_attention.spv", "half_probe.spv", "history.spv", "resident.spv",
    "window_attention.spv", "window_block.spv",
))
SOURCE_DIRS = ("layer", "ref", "gpu", "bench")
# The layer the game's own architecture loads; the daemon and its libraries are always x64.
LAYERS = {"x64": "nr_layer.dll", "x86": "nr_layer32.dll"}
# DXVK as its release lays it out, x64/ and x32/, and the files it puts beside a game.
DXVK_DIRS = {"x64": "x64", "x86": "x32"}
DXVK_FILES = ("d3d8.dll", "d3d9.dll", "d3d10core.dll", "d3d11.dll", "dxgi.dll")
# Beside every game as vulkan-1.dll: it gives the game NR's environment however the game is
# started (nr_vulkan_proxy.c), from ENV_FILE in the installation.
PROXIES = {"x64": "nr_vulkan_proxy.dll", "x86": "nr_vulkan_proxy32.dll"}
PROXY_NAME = "vulkan-1.dll"
ENV_FILE = "nr-env.txt"
# Where the proxy lists the folders the game was started in, for DXVK's first log there.
START_FOLDERS = "start-folders.txt"
# In the game's dlss-nr folder: what NR put beside the game, and the game's own files it set
# aside to do so, which Remove NR puts back.
GAME_FILES = "game-files.json"
GAME_BACKUP = "game-backup"
RUNTIME_MODULES = (
    "src/layer/nr_daemon.py", "src/layer/nr_alloc.py", "src/layer/nr_pipe.py",
    "src/layer/nr_paths.py", "src/layer/nr_knobs.py", "src/ref/nr_frame.py",
    "src/ref/nr_model.py", "src/ref/nr_image.py", "src/ref/image_io.py",
    "src/gpu/xmx.py", "src/gpu/xmxres.py", "scripts/get_weights.py",
    "work/mlx-dlss/python/mlxdlss/features.py",
    "work/mlx-dlss/python/mlxdlss/tools/extract_dlssnr_weights.py",
    "work/mlx-dlss/python/mlxdlss/tools/unpack_dlssnr_weights.py",
    "LICENSE", "NOTICE", "work/mlx-dlss/LICENSE",
)
MAX_OUTPUT = 256 * 1024
MAX_LOG_READ = 1024 * 1024
MAX_LOG_TAIL = 64 * 1024
_LOG_STATES = {}
_LOG_LOCK = threading.Lock()


@dataclass
class Profile:
    root: Path
    python: Path
    game_exe: Path
    dll: Path | None = None
    api: str = "vulkan"
    game_args: list[str] = field(default_factory=list)
    disable_fossilize: bool = True
    launch_mode: str = "direct"
    steam_app_id: str = ""

    def __post_init__(self):
        self.root = Path(self.root).expanduser().resolve()
        self.python = Path(self.python).expanduser().resolve()
        self.game_exe = Path(self.game_exe).expanduser().resolve()
        self.dll = Path(self.dll).expanduser().resolve() if self.dll else None
        self.game_args = list(self.game_args)

    def to_dict(self):
        return {"root": str(self.root), "python": str(self.python),
                "game_exe": str(self.game_exe), "dll": str(self.dll) if self.dll else None,
                "api": self.api, "game_args": list(self.game_args),
                "disable_fossilize": self.disable_fossilize,
                "launch_mode": self.launch_mode, "steam_app_id": self.steam_app_id}

    @classmethod
    def from_dict(cls, values):
        if not isinstance(values, dict):
            raise ValueError("Profile must be a JSON object")
        arguments = values.get("game_args", [])
        if not isinstance(arguments, list) or any(not isinstance(arg, str) for arg in arguments):
            raise ValueError("game_args must be a list of strings")
        fossilize = values.get("disable_fossilize", True)
        if not isinstance(fossilize, bool):
            raise ValueError("disable_fossilize must be a boolean")
        return cls(root=Path(values.get("root") or Path(__file__).resolve().parents[2]),
                   python=Path(values.get("python") or sys.executable),
                   game_exe=Path(values.get("game_exe") or "."),
                   dll=Path(values["dll"]) if values.get("dll") else None,
                   api=values.get("api", "vulkan"), game_args=arguments,
                   disable_fossilize=fossilize, launch_mode=values.get("launch_mode", "direct"),
                   steam_app_id=str(values.get("steam_app_id") or ""))


def profile_path(root):
    return Path(root) / "work/windows-profile.json"


def _read_json(path, limit=1024 * 1024):
    with Path(path).open("rb") as handle:
        raw = handle.read(limit + 1)
    if len(raw) > limit:
        raise ValueError(f"JSON file is too large: {path}")
    return json.loads(raw.decode("utf-8-sig"))


def _atomic_json(path, values):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(prefix=".nr-json-", dir=path.parent)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as handle:
            json.dump(values, handle, ensure_ascii=False, indent=2)
            handle.write("\n")
        Path(temporary).replace(path)
    finally:
        Path(temporary).unlink(missing_ok=True)


def _runtime_knobs():
    """Read the same standard-library-only catalogue used by Linux nr-panel.

    The backend lives in src/tools in a checkout and in scripts in a release.
    Neither location needs NumPy, a selected game or a running daemon to describe
    the controls. Keep the panel's normal slider ranges distinct from the wider
    ranges accepted by the daemon, so existing advanced values survive a refresh.
    """
    here = Path(__file__).resolve().parent
    candidates = (here.parent / "layer/nr_knobs.py", here.parent / "src/layer/nr_knobs.py")
    source = next((path for path in candidates if path.is_file()), None)
    if source is None:
        raise ValueError("The release is missing its runtime control catalogue")
    spec = importlib.util.spec_from_file_location("_nr_windows_knobs", source)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    knobs = []
    for knob in module.KNOBS:
        high = {"min_extent": 4096.0, "release": 255.0}.get(knob.name, knob.high)
        knobs.append({"name": knob.name, "label": knob.label, "kind": knob.kind,
                      "low": knob.low, "high": knob.high, "step": knob.step,
                      "default": knob.default, "summary": knob.summary, "detail": knob.detail,
                      "runtime_low": knob.low, "runtime_high": high,
                      "choices": list(module.PROFILES) if knob.kind == "choice" else None})
    return knobs


def settings_path(root):
    """Only the settings belonging to this release, never an inherited NR_ROOT."""
    root = Path(root).expanduser().resolve()
    if not root.is_dir():
        raise ValueError("The release root does not exist")
    work = root / "work"
    path = work / "nr_settings.json"
    if (work.is_symlink() or getattr(work, "is_junction", lambda: False)()
            or (work.exists() and not work.is_dir())
            or _canonical(work) != _canonical(root) + "/work"):
        raise ValueError("The release work directory is not an owned regular directory")
    if path.is_symlink() or (path.exists() and not path.is_file()):
        raise ValueError("The release settings path is not an owned regular file")
    return path


def _settings_document(root):
    path = settings_path(root)
    values = _read_json(path) if path.exists() else {}
    if not isinstance(values, dict):
        raise ValueError("Settings must be a JSON object")
    return path, values


def _validated_settings(values, knobs):
    effective = {}
    for knob in knobs:
        name = knob["name"]
        value = values.get(name, knob["default"])
        if knob["kind"] == "choice":
            if value not in knob["choices"]:
                raise ValueError(f"{name} must be one of {', '.join(knob['choices'])}")
        else:
            if isinstance(value, bool):
                raise ValueError(f"{name} must be a number, not a boolean")
            try:
                value = float(value)
            except (TypeError, ValueError, OverflowError):
                raise ValueError(f"{name} must be a finite number") from None
            if not math.isfinite(value) or not knob["runtime_low"] <= value <= knob["runtime_high"]:
                raise ValueError(f"{name} must be between {knob['runtime_low']:g} and {knob['runtime_high']:g}")
        effective[name] = value
    return effective


def _settings_result(path, values, knobs):
    effective = _validated_settings(values, knobs)
    return {"ok": True, "error": None, "path": str(path), "settings": effective,
            "knobs": knobs, "defaults": {knob["name"]: knob["default"] for knob in knobs},
            "explicit_keys": [knob["name"] for knob in knobs if knob["name"] in values],
            "warnings": [], "apply": "next_frame", "game_restart_required": False,
            "processing_confirmed": False}


def get_settings(root):
    """Describe validated saved values; this is not a daemon acknowledgement."""
    path, values = _settings_document(root)
    return _settings_result(path, values, _runtime_knobs())


def save_settings(root, changes):
    """Merge only changed controls with the latest document and replace atomically.

    Unknown existing keys belong to other tools and are retained. A settings save
    does not install a game, save a launch profile, start NR or change its trigger.
    """
    if not isinstance(changes, dict):
        raise ValueError("Settings changes must be a JSON object")
    knobs = _runtime_knobs()
    known = {knob["name"] for knob in knobs}
    unknown = set(changes) - known
    if unknown:
        raise ValueError("Unknown runtime controls: " + ", ".join(sorted(unknown)))
    path, values = _settings_document(root)
    values.update(changes)
    effective = _validated_settings(values, knobs)
    # Store numbers as numbers even when an existing command-line tool supplied
    # a numeric string; never write bool, NaN or Infinity for a runtime control.
    for name in changes:
        values[name] = effective[name]
    if changes:
        _atomic_json(path, values)
    return _settings_result(path, values, knobs)


def reset_settings(root, names=None):
    """Write explicit daemon defaults so a live daemon really changes its values.

    Deleting a key cannot reset a running daemon: Settings.refresh deliberately
    keeps its previous value when that key is absent from the next document.
    """
    knobs = _runtime_knobs()
    defaults = {knob["name"]: knob["default"] for knob in knobs}
    if names is None:
        names = list(defaults)
    if not isinstance(names, list) or any(not isinstance(name, str) for name in names):
        raise ValueError("Reset controls must be a list of names")
    unknown = set(names) - defaults.keys()
    if unknown:
        raise ValueError("Unknown runtime controls: " + ", ".join(sorted(unknown)))
    return save_settings(root, {name: defaults[name] for name in names})


def load_profile(root):
    path = profile_path(root)
    if not path.exists():
        return None
    profile = Profile.from_dict(_read_json(path))
    if _canonical(profile.root) != _canonical(root):
        raise ValueError("Saved profile belongs to a different release root")
    return profile


def save_profile(profile):
    # A Steam configuration an earlier setup left does not hold the profile: its wrapper
    # starts the game from a snapshot of its own (windows_launch._ensure_steam_profile).
    _atomic_json(profile_path(profile.root), profile.to_dict())
    return {"ok": True, "error": None, "path": str(profile_path(profile.root)),
            "profile": profile.to_dict()}


def _canonical(path):
    return os.path.normcase(str(Path(path).resolve())).replace("\\", "/").rstrip("/")


def installed_path(profile):
    return profile.game_exe.parent / "dlss-nr"


def _clean_environment():
    values = {key: value for key, value in os.environ.items()
              if not key.upper().startswith(("NR_", "XMX_", "VK_"))
              and key.upper() not in ("ENABLE_NR_LAYER", "DISABLE_NR_LAYER")}
    # A release's compatibility choice is explicit; other Steam layers retain
    # their normal implicit-loader configuration, including the overlay.
    values.pop("DISABLE_VK_LAYER_VALVE_steam_fossilize_1", None)
    return values


def layer_environment(profile):
    """NR's own variables: what a launch adds to the environment, and what the proxy sets."""
    token = hashlib.sha256(_canonical(profile.root).encode("utf-8")).hexdigest()[:8]
    work = profile.root / "work"
    values = {
        "NR_ROOT": str(profile.root), "NR_PYTHON": str(profile.python),
        "NR_DAEMON": str(profile.root / "src/layer/nr_daemon.py"),
        "NR_SETTINGS": str(work / "nr_settings.json"),
        "NR_LAYER_LOG": str(work / "nr_daemon.log"),
        "NR_LAYER_TRIGGER": str(work / "nr_trigger"),
        "NR_LAYER_SOCKET": rf"\\.\pipe\nr_windows_{token}",
        "NR_LAYER_LIVE": "1", "NR_LAYER_SPAWN": "1", "NR_KEEP_BLOCKS": "1",
        "ENABLE_NR_LAYER": "1", "VK_LAYER_PATH": str(installed_path(profile)),
        "VK_INSTANCE_LAYERS": "VK_LAYER_dlssnr_intel",
        "PYTHONUTF8": "1", "PYTHONDONTWRITEBYTECODE": "1",
        # DXVK logs into the folder the game was started in unless told otherwise. Through
        # the proxy its first log still goes there (_remove_dxvk_logs).
        "DXVK_LOG_PATH": str(work / "logs"),
    }
    if profile.disable_fossilize:
        values["DISABLE_VK_LAYER_VALVE_steam_fossilize_1"] = "1"
    return values


def runtime_env(profile):
    values = _clean_environment()
    values.update(layer_environment(profile))
    return values


def _hidden_options():
    return {"creationflags": subprocess.CREATE_NO_WINDOW} if os.name == "nt" else {}


def _run(command, timeout=10, env=None, emit=None, cwd=None):
    """Capture both bounded streams, emitting progress without shell expansion."""
    command = [str(value) for value in command]
    result = {"command": command, "returncode": None, "stdout": "", "stderr": "",
              "output": "", "timed_out": False}
    try:
        process = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, env=env or _clean_environment(),
                                   cwd=cwd, **_hidden_options())
    except OSError as error:
        result["stderr"] = result["output"] = str(error)
        return result
    events = queue.Queue(maxsize=256)

    def read_stream(name, handle):
        try:
            while True:
                chunk = handle.read1(4096)
                if not chunk:
                    break
                events.put((name, chunk))
        finally:
            handle.close()
            events.put((name, None))

    threads = [threading.Thread(target=read_stream, args=(name, handle), daemon=True)
               for name, handle in (("stdout", process.stdout), ("stderr", process.stderr))]
    for worker in threads:
        worker.start()
    chunks = {"stdout": bytearray(), "stderr": bytearray()}
    completed = set()
    deadline = time.monotonic() + timeout
    while len(completed) < 2:
        if result["timed_out"] and time.monotonic() > deadline + 2:
            break
        remaining = deadline - time.monotonic()
        if remaining <= 0 and not result["timed_out"]:
            result["timed_out"] = True
            if process.poll() is None:
                process.kill()  # only the subprocess this invocation created
        try:
            name, chunk = events.get(timeout=0.1)
        except queue.Empty:
            if result["timed_out"] and time.monotonic() > deadline + 2:
                break
            continue
        if chunk is None:
            completed.add(name)
            continue
        room = MAX_OUTPUT - len(chunks[name])
        if room > 0:
            chunks[name].extend(chunk[:room])
        if emit:
            try:
                emit(chunk.decode("utf-8", errors="replace"))
            except Exception:
                pass  # a UI progress callback must not abandon a running child
    try:
        result["returncode"] = process.wait(timeout=2)
    except subprocess.TimeoutExpired:
        process.kill()
        result["returncode"] = process.wait(timeout=2)
    for name, content in chunks.items():
        result[name] = bytes(content).decode("utf-8", errors="replace")
    result["output"] = result["stdout"] + result["stderr"]
    if result["timed_out"]:
        result["output"] += f"\nCommand timed out after {timeout:g}s"
    return result


_PYTHON_PROBE = """
import ctypes, importlib, json, os, struct, sys, sysconfig
result = {'platform': sys.platform, 'os_name': os.name, 'bits': struct.calcsize('P') * 8,
          'executable': sys.executable, 'version': sys.version.split()[0],
          'abi_platform': sysconfig.get_platform(),
          'native_apis': hasattr(ctypes, 'WinDLL') and hasattr(os, 'add_dll_directory'),
          'base_prefix': sys.base_prefix, 'packages': {}, 'package_errors': {}}
for name in ('numpy', 'safetensors'):
    try:
        module = importlib.import_module(name)
        result['packages'][name] = getattr(module, '__version__', 'present')
    except Exception as error:
        result['package_errors'][name] = str(error)
print(json.dumps(result))
"""


def _python_info(python, timeout=30):
    # Long enough for numpy's first import, while Windows' scanner reads its 30 MB OpenBLAS for
    # the first time: at 4 s a fresh environment failed Python, its packages and the DLLs at
    # once (issue #12), and the same Python answered at once a minute later.
    got = _run([python, "-I", "-c", _PYTHON_PROBE], timeout=timeout)
    try:
        value = json.loads(got["stdout"].strip().splitlines()[-1])
    except (ValueError, IndexError):
        value = {}
    value["probe_ok"] = got["returncode"] == 0 and not got["timed_out"]
    value["output"] = got["output"]
    if not value["probe_ok"]:
        value["probe_error"] = (f"no answer in {timeout} s" if got["timed_out"]
                                else got["output"].strip()[-300:] or f"exit code {got['returncode']}")
    return value


def _native_python(info):
    return bool(info.get("probe_ok") and info.get("platform") == "win32"
                and info.get("os_name") == "nt" and info.get("bits") == 64
                and info.get("abi_platform") == "win-amd64" and info.get("native_apis"))


def pe_architecture(path):
    """Read PE headers only; no executable or vendor DLL is loaded here."""
    try:
        with Path(path).open("rb") as handle:
            dos = handle.read(64)
            if len(dos) != 64 or dos[:2] != b"MZ":
                return None
            offset = struct.unpack_from("<I", dos, 60)[0]
            if not 64 <= offset <= 1024 * 1024:
                return None
            handle.seek(offset)
            header = handle.read(26)
        if len(header) != 26 or header[:4] != b"PE\0\0":
            return None
        machine = struct.unpack_from("<H", header, 4)[0]
        magic = struct.unpack_from("<H", header, 24)[0]
        if machine == 0x8664 and magic == 0x20B:
            return "x64"
        if machine == 0x14C and magic == 0x10B:
            return "x86"
        return f"machine-0x{machine:x}"
    except OSError:
        return None


def discover_python():
    """Prefer native CPython over the MinGW toolchain, within a ten-second budget."""
    deadline = time.monotonic() + 9.5
    candidates = [os.environ.get("NR_PYTHON"), sys.executable]
    for name in ("python", "python3"):
        candidates.append(shutil.which(name))
    launcher = shutil.which("py")
    if launcher:
        got = _run([launcher, "-0p"], timeout=2)
        for line in got["stdout"].splitlines():
            match = re.search(r"([A-Za-z]:[\\/].*\.exe)\s*$", line)
            if match:
                candidates.append(match.group(1).strip().strip('"'))
    if os.name == "nt":
        try:
            import winreg
            for hive in (winreg.HKEY_CURRENT_USER, winreg.HKEY_LOCAL_MACHINE):
                try:
                    with winreg.OpenKey(hive, r"Software\Python\PythonCore") as versions:
                        for index in range(winreg.QueryInfoKey(versions)[0]):
                            name = winreg.EnumKey(versions, index)
                            try:
                                with winreg.OpenKey(versions, name + r"\InstallPath") as key:
                                    candidates.append(str(winreg.QueryValue(key, None)) + "\\python.exe")
                            except OSError:
                                continue
                except OSError:
                    continue
        except ImportError:
            pass
    existing = {}
    for value in candidates:
        if value and Path(value).is_file() and pe_architecture(value) == "x64":
            existing.setdefault(_canonical(value), str(Path(value).resolve()))
    ordered = sorted(existing.values(), key=lambda value: (
        any(word in value.lower() for word in ("msys", "mingw", "cygwin")),
        _canonical(value) != _canonical(os.environ.get("NR_PYTHON") or sys.executable),
        value.lower()))
    found = []
    for value in ordered:
        remaining = deadline - time.monotonic()
        if remaining < 0.2:
            break
        if _native_python(_python_info(value, timeout=min(2, remaining))):
            found.append(value)
    return found


def _shader_names(root):
    names = set(SHADERS)
    for name in ("src/gpu/libxmx.c", "src/gpu/xmx.py", "src/gpu/xmxres.py"):
        path = root / name
        if path.is_file():
            names.update(re.findall(r"([A-Za-z0-9_]+\.spv)", path.read_text(encoding="utf-8")))
    return sorted(names)


def _weights_state(profile):
    path = profile.root / "work/mlxw/dlssnr-logical.safetensors"
    try:
        with path.open("rb") as handle:
            size_bytes = handle.read(8)
            if len(size_bytes) != 8:
                raise ValueError("missing safetensors header")
            length = int.from_bytes(size_bytes, "little")
            if not 2 <= length <= 16 * 1024 * 1024:
                raise ValueError("invalid safetensors header length")
            header = json.loads(handle.read(length))
        data_size = path.stat().st_size - 8 - length
        tensors = {key: value for key, value in header.items() if key != "__metadata__"}
        if len(tensors) != 649 or data_size < 0:
            raise ValueError("expected 649 logical tensors")
        for value in tensors.values():
            offsets = value.get("data_offsets", [])
            if (len(offsets) != 2 or not all(isinstance(v, int) for v in offsets)
                    or not 0 <= offsets[0] <= offsets[1] <= data_size):
                raise ValueError("invalid tensor data offsets")
        return {"ready": True, "path": str(path), "detail": "649 local logical tensors"}
    except (OSError, ValueError, TypeError, AttributeError) as error:
        return {"ready": False, "path": str(path), "detail": str(error)}


def _owned_installation(profile):
    path = installed_path(profile)
    if not path.exists():
        return True, "New game-local installation"
    if path.is_symlink() or getattr(path, "is_junction", lambda: False)():
        return False, "Installation directory cannot be a link or junction"
    if not path.is_dir():
        return False, "A file blocks the installation directory"
    try:
        owner = _read_json(path / OWNER_NAME, limit=16 * 1024)
        matches = owner.get("schema_version") == SCHEMA_VERSION and (
            _canonical(owner.get("root", "")) == _canonical(profile.root))
    except (OSError, ValueError, AttributeError):
        matches = False
    return matches, "Owned by this release" if matches else "Existing dlss-nr directory is not owned by this release"


VC_REDIST = "https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist"
# The DLL every test here extracted its weights from (notes/CLAUDE.md, Phase 0).
TESTED_DLL_VERSION = "310.8.0.0"
TESTED_DLL_SHA256 = "e16bcf15e16e13f527491cdf7845b2fe6521a738d8f7c9c721866a8496e1fc8e"
WEIGHTS_LOG = "work/logs/get-weights.log"


def _file_version(path):
    """The file version Windows' Properties shows, from the version resource, or None.

    GetFileVersionInfoW maps the file as data; no code in it runs."""
    if os.name != "nt" or not path or not Path(path).is_file():
        return None
    import ctypes
    from ctypes import wintypes
    version = ctypes.WinDLL("version")
    version.GetFileVersionInfoSizeW.argtypes = (wintypes.LPCWSTR, ctypes.POINTER(wintypes.DWORD))
    version.GetFileVersionInfoW.argtypes = (wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p)
    version.VerQueryValueW.argtypes = (ctypes.c_void_p, wintypes.LPCWSTR, ctypes.POINTER(ctypes.c_void_p),
                                       ctypes.POINTER(wintypes.UINT))
    size = version.GetFileVersionInfoSizeW(str(path), None)
    if not size:
        return None
    data = ctypes.create_string_buffer(size)
    pointer, length = ctypes.c_void_p(), wintypes.UINT()
    if (not version.GetFileVersionInfoW(str(path), 0, size, data)
            or not version.VerQueryValueW(data, "\\", ctypes.byref(pointer), ctypes.byref(length))
            or length.value < 52):
        return None
    # VS_FIXEDFILEINFO: its signature, the structure's version, then the file's as two DWORDs.
    fixed = ctypes.cast(pointer, ctypes.POINTER(wintypes.DWORD * 4)).contents
    if fixed[0] != 0xFEEF04BD:
        return None
    return f"{fixed[2] >> 16}.{fixed[2] & 0xFFFF}.{fixed[3] >> 16}.{fixed[3] & 0xFFFF}"


def _dll_info(path):
    """The user's DLL as a report gives it: its version and whether it is the one tested."""
    if not path or not Path(path).is_file():
        return {"present": False}
    digest = _sha256(path)
    return {"present": True, "size": Path(path).stat().st_size, "version": _file_version(path),
            "sha256": digest, "tested": digest == TESTED_DLL_SHA256}


def _load_dependencies(profile, timeout=30):
    # libnr_image.dll's one dependency outside Windows is VCOMP140.DLL, MSVC's OpenMP, which
    # the Visual C++ Redistributable installs: asked for by name, so its absence says so.
    code = """
import ctypes, json, os, sys
paths = json.loads(sys.argv[1]); result = {'loaded': [], 'failures': {}}
handles = [os.add_dll_directory(os.path.dirname(path)) for path in paths]
for path in ['vulkan-1.dll'] + paths:
    try:
        ctypes.WinDLL(path)
        result['loaded'].append(path)
    except OSError as error:
        result['failures'][path] = str(error)
if result['failures']:
    try:
        ctypes.WinDLL('vcomp140.dll')
    except OSError:
        result['failures']['VCOMP140.DLL'] = 'missing'
print(json.dumps(result))
"""
    got = _run([profile.python, "-I", "-c", code,
                json.dumps([str(profile.root / "work" / name) for name in LIBRARIES])], timeout=timeout)
    try:
        result = json.loads(got["stdout"].strip().splitlines()[-1])
    except (ValueError, IndexError):
        result = {"failures": {"dependency_probe": f"no answer in {timeout} s" if got["timed_out"]
                               else got["output"] or "No probe output"}}
    if "VCOMP140.DLL" in result.get("failures", {}):
        result["failures"]["VCOMP140.DLL"] = "missing: install the Microsoft Visual C++ x64 Redistributable, " + VC_REDIST
    result["ok"] = got["returncode"] == 0 and not got["timed_out"] and not result.get("failures")
    return result


def validate(profile):
    checks = []

    def check(name, ok, detail):
        checks.append({"name": name, "ok": bool(ok), "detail": str(detail)})

    check("release_root", profile.root.is_dir() and str(profile.root).isascii(),
          "The permanent release root must exist and use an ASCII path")
    check("api", profile.api in ("vulkan", "dxvk"), "Native Vulkan or an existing x64 DXVK configuration")
    check("launch_mode", profile.launch_mode in ("direct", "steam") and (
        profile.launch_mode != "steam" or not profile.steam_app_id or profile.steam_app_id.isdecimal()),
        "Direct launch, or Steam with an optional numeric app ID; blank uses autodetection")
    check("python_file", profile.python.is_file(), profile.python)
    info = _python_info(profile.python) if profile.python.is_file() else {}
    check("python_native_x64", _native_python(info),
          "This Python did not answer: " + info["probe_error"] if info.get("probe_error")
          else f"{info.get('platform', 'unknown')} / {info.get('bits', '?')} bit; {info.get('version', '')}")
    check("python_packages", not info.get("package_errors") and all(
        name in info.get("packages", {}) for name in ("numpy", "safetensors")),
        info.get("package_errors") or info.get("packages") or "NumPy and safetensors are required")
    arch = pe_architecture(profile.game_exe) if profile.game_exe.is_file() else None
    check("game_architecture", arch in LAYERS,
          f"{profile.game_exe}: {arch or 'not a PE executable'}; 64-bit and 32-bit games are supported")
    layer = LAYERS.get(arch, LAYERS["x64"])
    proxy = PROXIES.get(arch, PROXIES["x64"])
    shader_names = _shader_names(profile.root)
    required = [layer, proxy, *RUNTIME_MODULES, *["work/" + name for name in (*LIBRARIES, *shader_names)]]
    missing = [name for name in required if not (profile.root / name).is_file()]
    missing.extend("src/" + name for name in SOURCE_DIRS if not (profile.root / "src" / name).is_dir())
    check("runtime_files", not missing, "Missing: " + ", ".join(missing) if missing else "Complete local release runtime and extractors")
    bad_arch = ["work/" + value for value in LIBRARIES if pe_architecture(profile.root / "work" / value) != "x64"]
    for name in (layer, proxy):
        if arch in LAYERS and (profile.root / name).is_file() and pe_architecture(profile.root / name) != arch:
            bad_arch.append(name)
    check("runtime_architecture", not bad_arch, "Wrong architecture: " + ", ".join(bad_arch) if bad_arch
          else f"Native libraries are x64; {layer} and {proxy} for this {arch or 'unidentified'} game")
    dependency = _load_dependencies(profile) if _native_python(info) and not bad_arch else {"ok": False, "failures": {"probe": "Requires native x64 Python and x64 runtime"}}
    failures = dependency.get("failures") or {}
    check("runtime_dependencies", dependency.get("ok"),
          "; ".join(f"{Path(name).name}: {why}" for name, why in failures.items())
          or "Vulkan loader and native DLL dependencies load; no GPU work performed")
    weights = _weights_state(profile)
    dll_ok = bool(profile.dll and profile.dll.is_file() and profile.dll.name.lower() == "nvngx_dlssnr.dll"
                  and pe_architecture(profile.dll) == "x64")
    if weights["ready"]:
        model_detail = weights["detail"]
    elif dll_ok:
        version = _file_version(profile.dll)
        model_detail = (f"Install extracts the weights from {profile.dll}, version {version or 'unknown'}"
                        + ("" if version == TESTED_DLL_VERSION else f"; tested with {TESTED_DLL_VERSION}"))
    else:
        model_detail = "Supply your own x64 nvngx_dlssnr.dll or 649-tensor local logical weights"
    check("local_model_input", weights["ready"] or dll_ok, model_detail)
    owns, detail = _owned_installation(profile)
    check("installation_target", owns, detail)
    for name in ("windows-profile.json", "nr_settings.json", "nr_trigger"):
        path = profile.root / "work" / name
        check("state_path_" + name, not path.exists() or (path.is_file() and not path.is_symlink()), path)
    settings_path = profile.root / "work/nr_settings.json"
    try:
        settings_ok = not settings_path.exists() or isinstance(_read_json(settings_path), dict)
    except (OSError, ValueError):
        settings_ok = False
    check("settings_format", settings_ok, "Existing settings must be a JSON object")
    template = profile.root / "VkLayer_dlss_nr.json"
    if not template.is_file():
        template = profile.root / "src/layer/VkLayer_dlss_nr.json"
    try:
        manifest = _read_json(template)
        manifest_ok = isinstance(manifest.get("layer"), dict) and manifest["layer"].get("name") == "VK_LAYER_dlssnr_intel"
    except (OSError, ValueError, AttributeError):
        manifest_ok = False
    check("manifest", manifest_ok, "A valid layer manifest must be present before installation")
    if profile.api == "dxvk":
        folder = profile.root / "dxvk" / DXVK_DIRS.get(arch, "x64")
        wrong = [name for name in DXVK_FILES if pe_architecture(folder / name) != arch]
        check("dxvk_files", arch in LAYERS and not wrong,
              f"DXVK for this {arch} game, from {folder}" if arch in LAYERS and not wrong
              else "The release's DXVK is missing or of another architecture: " + ", ".join(wrong))
    files_ok, files_detail = _game_files_check(profile, arch)
    check("game_files", files_ok, files_detail)
    ok = all(item["ok"] for item in checks)
    return {"ok": ok, "error": None if ok else "Preflight checks failed", "checks": checks,
            "python_info": info, "weights": weights, "runtime_dependencies": dependency}


def ensure_dependencies(profile, emit=None):
    info = _python_info(profile.python)
    if not _native_python(info):
        return {"ok": False, "error": "Choose native x64 Windows Python before preparing dependencies", "output": info.get("output", "")}
    directory = profile.root / "work/windows-python"
    interpreter = directory / "Scripts/python.exe"
    commands = []
    try:
        if directory.exists() and (directory.is_symlink() or not (directory / "pyvenv.cfg").is_file()):
            raise ValueError("Existing windows-python directory is not a local Python virtual environment")
        if not interpreter.is_file():
            directory.parent.mkdir(parents=True, exist_ok=True)
            if emit:
                emit("Creating release-local Python environment\n")
            got = _run([profile.python, "-I", "-m", "venv", str(directory)], timeout=120, emit=emit)
            commands.append(got)
            if got["returncode"] != 0 or got["timed_out"]:
                raise RuntimeError("Could not create the local virtual environment")
        if emit:
            emit("Installing NumPy and safetensors into the local environment\n")
        got = _run([interpreter, "-I", "-m", "pip", "install", "--disable-pip-version-check",
                    "--only-binary=:all:", "numpy", "safetensors"], timeout=600, emit=emit)
        commands.append(got)
        if got["returncode"] != 0 or got["timed_out"]:
            raise RuntimeError("Dependency installation failed; the global interpreter was not modified")
        chosen = replace(profile, python=interpreter)
        chosen_info = _python_info(chosen.python)
        if (not _native_python(chosen_info) or chosen_info.get("package_errors")
                or not all(name in chosen_info.get("packages", {}) for name in ("numpy", "safetensors"))):
            raise RuntimeError("The local interpreter did not pass its dependency checks")
        return {"ok": True, "error": None, "python": str(chosen.python),
                "profile": chosen.to_dict(), "output": "\n".join(value["output"] for value in commands),
                "commands": commands}
    except (OSError, ValueError, RuntimeError) as error:
        return {"ok": False, "error": str(error), "output": "\n".join(value["output"] for value in commands), "commands": commands}


def _copy_runtime(profile, stage, destination):
    ignore = shutil.ignore_patterns("__pycache__", "*.pyc", "*.safetensors", "nvngx_dlssnr.dll", ".git")
    for name in SOURCE_DIRS:
        shutil.copytree(profile.root / "src" / name, stage / "src" / name,
                        dirs_exist_ok=True, ignore=ignore)
    shutil.copytree(profile.root / "work/mlx-dlss", stage / "work/mlx-dlss",
                    dirs_exist_ok=True, ignore=ignore)
    arch = pe_architecture(profile.game_exe)
    layer = LAYERS[arch]
    shutil.copy2(profile.root / layer, stage / layer)
    for name in (*LIBRARIES, *_shader_names(profile.root)):
        shutil.copy2(profile.root / "work" / name, stage / "work" / name)
    template = profile.root / "VkLayer_dlss_nr.json"
    if not template.is_file():
        template = profile.root / "src/layer/VkLayer_dlss_nr.json"
    manifest = _read_json(template)
    manifest["layer"]["library_path"] = str(destination / layer)
    manifest["layer"]["library_arch"] = "64" if arch == "x64" else "32"
    _atomic_json(stage / "VkLayer_dlss_nr.json", manifest)
    _atomic_json(stage / OWNER_NAME, {"schema_version": SCHEMA_VERSION, "root": str(profile.root),
                                    "game_exe": str(profile.game_exe)})


def _sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def _dxvk_logs(profile):
    """The logs DXVK writes into the folder a game was started in, without DXVK_LOG_PATH."""
    return [f"{profile.game_exe.stem}_{Path(module).stem}.log" for module in DXVK_FILES]


def _game_record(folder):
    """What NR put beside the game, from an installation folder (or its stage)."""
    path = Path(folder) / GAME_FILES
    if not path.exists():
        return {"schema_version": SCHEMA_VERSION, "arch": None, "files": {}}
    record = _read_json(path, limit=64 * 1024)
    if not isinstance(record, dict) or not isinstance(record.get("files"), dict):
        raise ValueError(f"{path} is not a record of placed game files")
    return record


def _game_sources(profile, arch):
    """What goes beside the game, by name: the proxy for every game, and DXVK for DirectX 8-11."""
    sources = {PROXY_NAME: profile.root / PROXIES[arch]}
    if profile.api == "dxvk":
        folder = profile.root / "dxvk" / DXVK_DIRS[arch]
        sources.update((name, folder / name) for name in DXVK_FILES)
    return sources


def _game_files_check(profile, arch):
    """Whether installing may touch the game's folder, and which of its files it would set aside."""
    try:
        record = _game_record(installed_path(profile))
    except (OSError, ValueError) as error:
        return False, str(error)
    if arch not in LAYERS:
        return True, "Nothing is put beside the game"
    sources = _game_sources(profile, arch)
    placed = [name for name, entry in record["files"].items() if entry.get("placed")]
    # An installation from before the proxy has fewer files placed, and gains it; one with
    # DXVK's files placed for DirectX cannot become a Vulkan one without Remove NR first.
    if placed and (not set(placed) <= set(sources) or record.get("arch") != arch):
        return False, "Remove NR from this game before changing its graphics API or executable"
    aside = []
    for name, source in sources.items():
        target = profile.game_exe.parent / name
        entry = record["files"].get(name, {})
        if target.is_symlink() or (target.exists() and not target.is_file()):
            return False, f"{target} is not a regular file"
        if target.is_file() and not entry.get("placed") and source.is_file() \
                and _sha256(target) != _sha256(source):
            aside.append(name)
    return True, ("Set aside, and put back by Remove NR: " + ", ".join(aside) if aside
                  else "No file of the game's is replaced")


def _place_game_files(profile, arch, folder):
    """Put the proxy, and DXVK for DirectX 8-11, beside the game, each step recorded in
    `folder`'s GAME_FILES first.

    A file of the same name that NR did not put there is moved into `folder`'s GAME_BACKUP
    and comes back with Remove NR; an identical copy someone else put there is left alone.
    `folder` is the installation's stage, in the game's own folder, so every move is a
    rename. Returns what was done, for _undo_game_files; undoes its own part if it fails."""
    game = profile.game_exe.parent
    folder = Path(folder)
    record = _game_record(folder)
    record["arch"] = arch
    if "logs_before" not in record:
        # DXVK's logs already beside the game stay when NR is removed; only later ones go.
        record["logs_before"] = [name for name in _dxvk_logs(profile) if (game / name).exists()]
    # And in the folders the game is started in, those written before this.
    record.setdefault("installed_time", time.time())
    files = record["files"]
    done = []
    try:
        for name, source in _game_sources(profile, arch).items():
            target = game / name
            ours = _sha256(source)
            entry = files.get(name, {})
            if target.is_symlink() or (target.exists() and not target.is_file()):
                raise ValueError(f"{target} is not a regular file")
            if target.is_file():
                current = _sha256(target)
                if current == ours:
                    if not entry.get("placed"):
                        files[name] = {"placed": False, "sha256": ours}
                        _atomic_json(folder / GAME_FILES, record)
                    continue
                if entry.get("placed") and current == entry.get("sha256"):
                    # NR's own copy from an earlier release: kept aside until this one commits.
                    undo = folder / ".undo"
                    undo.mkdir(exist_ok=True)
                    os.replace(target, undo / name)
                    done.append((name, "replaced", dict(entry)))
                else:
                    backup = folder / GAME_BACKUP
                    backup.mkdir(exist_ok=True)
                    if (backup / name).exists():
                        raise ValueError(f"{backup / name} already holds a set-aside file")
                    os.replace(target, backup / name)
                    entry = {"backup": True, "previous_sha256": current}
                    done.append((name, "set-aside", None))
            else:
                done.append((name, "added", None))
            temporary = game / (name + ".dlss-nr-new")
            shutil.copy2(source, temporary)
            os.replace(temporary, target)
            files[name] = {"placed": True, "sha256": ours, "backup": bool(entry.get("backup")),
                           "previous_sha256": entry.get("previous_sha256")}
            _atomic_json(folder / GAME_FILES, record)
    except BaseException:
        _undo_game_files(profile, folder, done)
        raise
    return done


def _write_proxy_environment(profile, stage, destination):
    """ENV_FILE, which the proxy reads in the game's process: the layer's environment, the game
    it is for, where to record a launch, and the game's own loader if one was set aside."""
    lines = ["# Read by vulkan-1.dll beside the game (nr_vulkan_proxy.c) in the game's own process.",
             "NR_GAME_EXE=" + str(profile.game_exe),
             "NR_LAUNCH_STATE=" + str(profile.root / "work/windows-wizard/launch-state.json")]
    if _game_record(stage)["files"].get(PROXY_NAME, {}).get("backup"):
        lines.append("NR_REAL_VULKAN=" + str(destination / GAME_BACKUP / PROXY_NAME))
    lines += [f"{name}={value}" for name, value in layer_environment(profile).items()]
    (stage / ENV_FILE).write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def _undo_game_files(profile, folder, done):
    """Reverse _place_game_files' steps, last first, with the record where `folder` now is."""
    game = profile.game_exe.parent
    folder = Path(folder)
    for name, kind, entry in reversed(done):
        target = game / name
        target.unlink(missing_ok=True)
        if kind == "set-aside" and (folder / GAME_BACKUP / name).is_file():
            os.replace(folder / GAME_BACKUP / name, target)
        elif kind == "replaced" and (folder / ".undo" / name).is_file():
            os.replace(folder / ".undo" / name, target)
    try:
        record = _game_record(folder)
        for name, kind, entry in done:
            if kind == "replaced":
                record["files"][name] = entry
            else:
                record["files"].pop(name, None)
        _atomic_json(folder / GAME_FILES, record)
    except (OSError, ValueError):
        pass


def uninstall(profile, emit=None):
    """Take NR out of the game's folder: the files it put there go, the game's own come back."""
    allowed, detail = _install_process_guard(profile)
    if not allowed:
        return {"ok": False, "error": detail}
    destination = installed_path(profile)
    if not destination.exists():
        return {"ok": True, "error": None, "removed": False, "detail": "NR is not installed beside this game"}
    owns, detail = _owned_installation(profile)
    if not owns:
        return {"ok": False, "error": detail}
    # Launch options an earlier setup gave Steam would go on starting the game through a
    # wrapper whose layer is gone, and the game would not start. The bridge returns them first.
    steam = profile.root / "work/windows-wizard/steam-backup.json"
    if steam.exists():
        try:
            pending = not _read_json(steam, limit=64 * 1024).get("restored")
        except (OSError, ValueError, AttributeError):
            pending = True
        if pending:
            return {"ok": False, "error": "Restore Steam's launch options before removing NR"}
    game = profile.game_exe.parent
    kept, lost = [], []
    try:
        record = _game_record(destination)
        placed_any = any(entry.get("placed") for entry in record["files"].values())
        for name in sorted(record["files"]):
            entry = record["files"][name]
            if entry.get("placed"):
                target = game / name
                if target.is_file() and _sha256(target) != entry.get("sha256"):
                    kept.append(name)
                    continue
                target.unlink(missing_ok=True)
                backup = destination / GAME_BACKUP / name
                if entry.get("backup") and backup.is_file():
                    os.replace(backup, target)
                    if emit:
                        emit(f"Put back the game's own {name}\n")
                elif entry.get("backup"):
                    lost.append(name)
            del record["files"][name]
            _atomic_json(destination / GAME_FILES, record)
        if placed_any:
            _remove_dxvk_logs(profile, destination, record)
        if kept:
            return {"ok": False, "error": "Changed since NR put them there, so left as they are: "
                    + ", ".join(kept) + ". The dlss-nr folder keeps the game's originals.", "kept": kept}
        shutil.rmtree(destination)
        (profile.root / "work" / "nr_trigger").unlink(missing_ok=True)
    except (OSError, ValueError) as error:
        return {"ok": False, "error": str(error)}
    if emit:
        emit("NR removed from the game\n")
    result = {"ok": True, "error": None, "removed": True, "trigger_exists": False}
    if lost:
        # The set-aside copies were deleted from dlss-nr by hand; Steam's file check restores them.
        result["missing_originals"] = lost
    return result


def _remove_dxvk_logs(profile, destination, record):
    """The logs NR's DXVK wrote into the game's folders.

    DXVK logs into the folder the game was started in, as <game>_<module>.log, until
    DXVK_LOG_PATH is set. The proxy sets it when DXVK loads vulkan-1.dll, after DXVK's first
    log is open, and lists that folder in START_FOLDERS. Beside the executable, the logs that
    were there before NR stay; in the other folders, those last written before it."""
    game = profile.game_exe.parent
    for name in _dxvk_logs(profile):
        if name not in record.get("logs_before", []):
            (game / name).unlink(missing_ok=True)
    since = record.get("installed_time")
    started = destination / START_FOLDERS
    if not isinstance(since, (int, float)) or not started.is_file():
        return
    for line in started.read_text(encoding="utf-8", errors="replace").splitlines():
        folder = Path(line.strip())
        # A folder that cannot be reached keeps its log, not NR.
        try:
            if not folder.is_absolute() or _canonical(folder) == _canonical(game):
                continue
            for name in _dxvk_logs(profile):
                log = folder / name
                # A file's time can be coarser than this clock: 2 s on FAT.
                if log.is_file() and not log.is_symlink() and log.stat().st_mtime >= since - 2:
                    log.unlink()
        except OSError:
            continue


def _process_snapshot():
    if os.name != "nt":
        raise ValueError("Windows process inspection is required before installation")
    executable = Path(os.environ.get("SystemRoot", r"C:\Windows")) / "System32/WindowsPowerShell/v1.0/powershell.exe"
    script = "[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false); $ErrorActionPreference='Stop'; @(Get-CimInstance Win32_Process | Select-Object ProcessId,Name,ExecutablePath) | ConvertTo-Json -Compress"
    got = _run([executable, "-NoProfile", "-NonInteractive", "-Command", script], timeout=4)
    if got["returncode"] != 0 or got["timed_out"]:
        raise ValueError("Could not safely inspect running games: " + got["output"])
    parsed = json.loads(got["stdout"] or "[]")
    return parsed if isinstance(parsed, list) else [parsed]


def _running_executable(pid):
    """The executable of the process `pid` while it is still running, else None.

    One handle and two queries. The status check runs every 5 s while the window is in
    front, and the whole WMI process list it used to take cost 0.45 s of CPU each time, in
    a PowerShell started for it, while the game played."""
    if os.name != "nt":
        raise ValueError("Windows process inspection is required")
    if not isinstance(pid, int) or isinstance(pid, bool) or pid <= 0:
        return None
    import ctypes
    from ctypes import wintypes
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel32.OpenProcess.restype = wintypes.HANDLE
    kernel32.OpenProcess.argtypes = (wintypes.DWORD, wintypes.BOOL, wintypes.DWORD)
    kernel32.GetExitCodeProcess.argtypes = (wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD))
    kernel32.QueryFullProcessImageNameW.argtypes = (
        wintypes.HANDLE, wintypes.DWORD, wintypes.LPWSTR, ctypes.POINTER(wintypes.DWORD))
    kernel32.CloseHandle.argtypes = (wintypes.HANDLE,)
    handle = kernel32.OpenProcess(0x1000, False, pid)  # PROCESS_QUERY_LIMITED_INFORMATION
    if not handle:
        return None
    try:
        code = wintypes.DWORD()
        if not kernel32.GetExitCodeProcess(handle, ctypes.byref(code)) or code.value != 259:  # STILL_ACTIVE
            return None
        size = wintypes.DWORD(32768)
        name = ctypes.create_unicode_buffer(size.value)
        if not kernel32.QueryFullProcessImageNameW(handle, 0, name, ctypes.byref(size)):
            return None
        return name.value
    finally:
        kernel32.CloseHandle(handle)


def _launch_state(profile):
    path = profile.root / "work/windows-wizard/launch-state.json"
    if not path.exists():
        return {}
    value = _read_json(path, limit=128 * 1024)
    if not isinstance(value, dict):
        raise ValueError("Launch state must be a JSON object")
    return value


def _install_process_guard(profile):
    try:
        processes = _process_snapshot()
        state = _launch_state(profile)
        game_directory = _canonical(profile.game_exe.parent)
        for process in processes:
            executable = process.get("ExecutablePath")
            if executable and _canonical(executable).startswith(game_directory + "/"):
                return False, "Close the selected game before changing its installation"
            if not executable and str(process.get("Name", "")).casefold() == profile.game_exe.name.casefold():
                return False, "A matching game process could not be inspected; close it before installation"
            if state.get("pid") == process.get("ProcessId") and state.get("game_exe") and executable:
                if _canonical(executable) == _canonical(state["game_exe"]):
                    return False, "Another game is still using this release; close it before installation"
        return True, "No selected or release-owned game is running"
    except (OSError, ValueError, TypeError) as error:
        return False, str(error)


def install(profile, emit=None):
    allowed, detail = _install_process_guard(profile)
    if not allowed:
        return {"ok": False, "error": detail}
    checked = validate(profile)
    failed = {item["name"] for item in checked["checks"] if not item["ok"]}
    if failed == {"python_packages"}:
        prepared = ensure_dependencies(profile, emit=emit)
        if not prepared["ok"]:
            return prepared
        profile = Profile.from_dict(prepared["profile"])
        checked = validate(profile)
    if not checked["ok"]:
        return checked
    commands = []
    if not checked["weights"]["ready"]:
        if emit:
            emit("Extracting logical weights from your local NVIDIA DLL\n")
        extracted = _run([profile.python, "-I", str(profile.root / "scripts/get_weights.py"),
                          profile.dll, "--work-dir", str(profile.root / "work")],
                         timeout=900, cwd=profile.root, emit=emit)
        commands.append(extracted)
        # Kept for Save report: the window shows it only until the next action (issue #12).
        try:
            (profile.root / "work/logs").mkdir(parents=True, exist_ok=True)
            (profile.root / WEIGHTS_LOG).write_text(extracted["output"], encoding="utf-8")
        except OSError:
            pass
        if extracted["returncode"] != 0 or extracted["timed_out"] or not _weights_state(profile)["ready"]:
            version = _file_version(profile.dll)
            return {"ok": False, "error": "Weight extraction failed or did not produce 649 logical tensors, "
                    f"from nvngx_dlssnr.dll version {version or 'unknown'} (tested: {TESTED_DLL_VERSION}). "
                    "The extractor's output is in Check details and in Save report.",
                    "commands": commands, "output": extracted["output"]}
    # Recheck every input and destination after extraction, before any game write.
    checked = validate(profile)
    if not checked["ok"]:
        return checked
    allowed, detail = _install_process_guard(profile)
    if not allowed:
        return {"ok": False, "error": detail}
    destination = installed_path(profile)
    stage = backup = None
    committed = False
    work = profile.root / "work"
    state_files = [profile_path(profile.root), work / "nr_settings.json", work / "nr_trigger"]
    saved = {}
    placed = []
    try:
        saved = {path: path.read_bytes() if path.exists() else None for path in state_files}
        stage = Path(tempfile.mkdtemp(prefix=".dlss-nr-stage-", dir=destination.parent))
        if destination.exists():
            shutil.copytree(destination, stage, dirs_exist_ok=True)
        _copy_runtime(profile, stage, destination)
        placed = _place_game_files(profile, pe_architecture(profile.game_exe), stage)
        _write_proxy_environment(profile, stage, destination)
        # What the game's process writes into: DXVK's logs, and the proxy's launch record,
        # for which it makes no folder.
        (work / "logs").mkdir(parents=True, exist_ok=True)
        (work / "windows-wizard").mkdir(parents=True, exist_ok=True)
        allowed, detail = _install_process_guard(profile)
        if not allowed:
            raise ValueError(detail)
        owns, detail = _owned_installation(profile)
        if not owns:
            raise ValueError(detail)
        if destination.exists():
            backup = Path(tempfile.mkdtemp(prefix=".dlss-nr-backup-", dir=destination.parent))
            backup.rmdir()
            destination.rename(backup)
        stage.rename(destination)
        stage = None
        committed = True
        settings = {}
        if saved[work / "nr_settings.json"]:
            settings = json.loads(saved[work / "nr_settings.json"].decode("utf-8-sig"))
            if not isinstance(settings, dict):
                raise ValueError("Settings must be a JSON object")
        settings.setdefault("render_scale", 0.4)
        settings.setdefault("min_extent", 320)
        _atomic_json(work / "nr_settings.json", settings)
        (work / "nr_trigger").unlink(missing_ok=True)
        save_profile(profile)
    except (OSError, ValueError, TypeError, KeyError) as error:
        if placed:
            _undo_game_files(profile, destination if committed else stage, placed)
        if committed:
            shutil.rmtree(destination)
        if backup and backup.exists():
            backup.rename(destination)
            backup = None
        if committed:
            for path, content in saved.items():
                if content is None:
                    path.unlink(missing_ok=True)
                else:
                    path.write_bytes(content)
        return {"ok": False, "error": str(error), "commands": commands}
    finally:
        if stage and stage.exists():
            shutil.rmtree(stage)
    if backup and backup.exists():
        shutil.rmtree(backup)
    shutil.rmtree(destination / ".undo", ignore_errors=True)
    if emit:
        emit("Installation complete; the effect is off\n")
    return {"ok": True, "error": None, "installed": str(destination), "root": str(profile.root),
            "profile": profile.to_dict(), "commands": commands, "trigger_exists": False}


def set_effect(profile, enabled):
    owns, detail = _owned_installation(profile)
    if not owns or not (installed_path(profile) / OWNER_NAME).is_file():
        return {"ok": False, "error": detail if not owns else "Install this profile before toggling its effect"}
    trigger = Path(runtime_env(profile)["NR_LAYER_TRIGGER"])
    try:
        launch = _launch_state(profile)
        if launch.get("game_exe") and _canonical(launch["game_exe"]) != _canonical(profile.game_exe):
            if _running_executable(launch.get("pid")):
                raise ValueError("The active game uses another profile; select it before changing its effect")
        if trigger.is_symlink() or (trigger.exists() and not trigger.is_file()):
            raise ValueError("The release trigger path is not an owned regular file")
        if enabled:
            trigger.parent.mkdir(parents=True, exist_ok=True)
            trigger.touch()
        else:
            trigger.unlink(missing_ok=True)
        result = {"ok": True, "error": None, "trigger_exists": bool(enabled),
                  "trigger": str(trigger), "processing_confirmed": False,
                  "detail": "Trigger changed; fresh processed frames must confirm processing"}
        if enabled and _game_running_without_nr(profile, launch):
            result.update(warning="game_without_nr",
                          detail="The game is running without NR's layer: it was started before NR "
                                 "was installed, or it does not load the Vulkan loader from its own "
                                 "folder. Restart it; if that does not help, use Launch game.")
        return result
    except (OSError, ValueError) as error:
        return {"ok": False, "error": str(error)}


def _game_running_without_nr(profile, launch):
    """Whether the selected game runs without NR's layer: a process of it that no launch names.

    One started before NR was installed is such a process, and so is a game that loads Windows'
    own vulkan-1.dll by its path, past the proxy. Switching the effect on changes nothing in
    it. One process list, taken only when the effect is switched on, not by the status check
    that runs every 5 s."""
    target = _canonical(profile.game_exe)
    if launch.get("game_exe") and _canonical(launch["game_exe"]) == target and launch.get("exit_code") is None:
        running = _running_executable(launch.get("pid"))
        if running and _canonical(running) == target:
            return False
    try:
        processes = _process_snapshot()
    except ValueError:
        return False
    return any(item.get("ExecutablePath") and _canonical(item["ExecutablePath"]) == target
               for item in processes)


def _new_log_state(complete=True):
    return {"offset": 0, "pending": b"", "identity": None, "prefix": None,
            "processed": 0, "rejected": 0, "errors": 0, "error_tail": deque(maxlen=30),
            "last_frame": None, "network_shape": None, "half_probe": None, "allocator": False,
            "model_ready": None, "session": 0, "counts_complete": complete}


def _parse_log_line(state, line):
    if line.startswith("half rounding:"):
        previous_session = state["session"]
        for name, value in _new_log_state().items():
            if name not in ("offset", "pending", "identity", "prefix"):
                state[name] = value
        state["session"] = previous_session + 1
        state["half_probe"] = {"checked": "NOT CHECKED" not in line, "ok": None, "detail": line}
    if "half_round uses" in line and state["half_probe"]:
        state["half_probe"]["detail"] += "\n" + line
    if line.startswith("model ready"):
        if state["model_ready"]:
            previous_session = state["session"]
            for name, value in _new_log_state().items():
                if name not in ("offset", "pending", "identity", "prefix"):
                    state[name] = value
            state["session"] = previous_session + 1
        state["model_ready"] = line
        if (state["half_probe"] and state["half_probe"]["checked"]
                and state["half_probe"]["ok"] is None):
            state["half_probe"]["ok"] = True
    if "keeping NumPy's large blocks" in line:
        state["allocator"] = True
    if line.startswith("frame rejected/failed") or "unsupported VkFormat" in line:
        state["rejected"] += 1
    if any(word in line for word in ("frame rejected/failed", "GPU lost", "Traceback", "FAIL:", "ERROR", "unsupported VkFormat")):
        state["errors"] += 1
        state["error_tail"].append(line[:500])
        if line.startswith("FAIL:") and state["half_probe"]:
            state["half_probe"]["ok"] = False
    match = re.match(r"^(\d+)x(\d+)\s+\S+\s+in\s+(\d+(?:\.\d+)?)s\s+change\s+(\S+)", line)
    if match:
        state["processed"] += 1
        network = re.search(r"\bnetwork (\d+)x(\d+)", line)
        state["network_shape"] = [int(value) for value in network.groups()] if network else None
        state["last_frame"] = {"width": int(match[1]), "height": int(match[2]),
                               "seconds": float(match[3]), "change": match[4], "line": line[:1000]}
        if match[4].lower() in ("nan", "inf", "-inf"):
            state["errors"] += 1
            state["error_tail"].append("Non-finite image change: " + line[:400])


def status(profile):
    log = Path(runtime_env(profile)["NR_LAYER_LOG"])
    key = _canonical(log)
    observer = profile.root / "work/windows-wizard/status-observer.json"
    try:
        launch = _launch_state(profile)
    except (OSError, ValueError):
        launch = {}
    matching_launch = bool(launch.get("root") and launch.get("game_exe")
                           and _canonical(launch["root"]) == _canonical(profile.root)
                           and _canonical(launch["game_exe"]) == _canonical(profile.game_exe))
    context = {"root": _canonical(profile.root), "game_exe": _canonical(profile.game_exe),
               "pid": launch.get("pid") if matching_launch else None,
               "started_utc": launch.get("started_utc") if matching_launch else None,
               "start_offset": launch.get("daemon_start_bytes", 0) if matching_launch else 0}
    if not isinstance(context["start_offset"], int) or context["start_offset"] < 0:
        context["start_offset"] = 0
    context_key = hashlib.sha256(json.dumps(context, sort_keys=True).encode("utf-8")).hexdigest()
    with _LOG_LOCK:
        state = _LOG_STATES.get(key)
        if state is None:
            try:
                saved = _read_json(observer, limit=256 * 1024)
                if saved.get("schema_version") == SCHEMA_VERSION and saved.get("context_key") == context_key:
                    state = saved["state"]
                    state["pending"] = base64.b64decode(state["pending"])
                    state["prefix"] = bytes.fromhex(state["prefix"]) if state["prefix"] is not None else None
                    state["identity"] = tuple(state["identity"]) if state["identity"] else None
                    state["error_tail"] = deque(state["error_tail"], maxlen=30)
            except (OSError, ValueError, TypeError, KeyError):
                state = None
        if state is None or state.get("context_key") != context_key:
            state = _new_log_state()
            state["offset"] = context["start_offset"]
            state["context_key"] = context_key
        _LOG_STATES[key] = state
        initial = state["identity"] is None
        fresh_processed = 0
        mtime = None
        try:
            stat = log.stat()
            mtime = stat.st_mtime
            with log.open("rb") as handle:
                prefix = handle.read(64)
                identity = (stat.st_dev, stat.st_ino)
                # A short file's first 64 bytes legitimately grow. Only a changed
                # existing prefix, replaced file or truncation starts a new log.
                prefix_changed = state["prefix"] is not None and not prefix.startswith(state["prefix"])
                if (state["identity"] is not None and identity != state["identity"]) or stat.st_size < state["offset"] or prefix_changed:
                    state = _new_log_state(complete=stat.st_size <= MAX_LOG_READ)
                    state["context_key"] = context_key
                    _LOG_STATES[key] = state
                    initial = True
                offset = state["offset"]
                if stat.st_size - offset > MAX_LOG_READ:
                    offset = stat.st_size - MAX_LOG_READ
                    state["counts_complete"] = False
                    state["pending"] = b""
                handle.seek(offset)
                content = handle.read(min(MAX_LOG_READ, max(0, stat.st_size - offset)))
                consumed = handle.tell()
            if offset and offset != state["offset"]:
                content = content.split(b"\n", 1)[-1]
            before = state["processed"]
            content = state["pending"] + content
            lines = content.split(b"\n")
            state["pending"] = lines.pop()
            for line in lines:
                _parse_log_line(state, line.decode("utf-8", errors="replace").rstrip("\r"))
            # First observation after an authenticated launch may contain its
            # first frames. An unassociated historical log cannot confirm them.
            fresh_processed = max(0, state["processed"] - before) if not initial or matching_launch else 0
            state.update(offset=consumed, identity=identity, prefix=prefix, context_key=context_key)
        except FileNotFoundError:
            state = _new_log_state()
            state["context_key"] = context_key
            _LOG_STATES[key] = state
        except OSError as error:
            return {"ok": False, "error": str(error), "log_path": str(log)}
        result = {name: value for name, value in state.items()
                  if name not in ("offset", "pending", "identity", "prefix", "error_tail", "context_key")}
        result.update(ok=True, error=None, error_tail=list(state["error_tail"]),
                      trigger_exists=Path(runtime_env(profile)["NR_LAYER_TRIGGER"]).is_file(),
                      log_path=str(log), log_mtime=mtime, fresh_processed=fresh_processed,
                      processing_observed=state["processed"] > 0,
                      game_connected=None)
        persisted = dict(state, pending=base64.b64encode(state["pending"]).decode("ascii"),
                         prefix=state["prefix"].hex() if state["prefix"] is not None else None,
                         identity=list(state["identity"]) if state["identity"] else None,
                         error_tail=list(state["error_tail"]))
        try:
            _atomic_json(observer, {"schema_version": SCHEMA_VERSION, "context_key": context_key,
                                    "state": persisted})
        except OSError as error:
            result["observer_error"] = str(error)
    active = False
    process_error = None
    if matching_launch and isinstance(launch.get("pid"), int) and launch.get("exit_code") is None:
        try:
            running = _running_executable(launch["pid"])
            active = bool(running) and _canonical(running) == _canonical(profile.game_exe)
        except (OSError, ValueError):
            process_error = "The launch PID could not be verified"
    result.update(launch=context, active_game=active,
                  fresh_frames=bool(active and fresh_processed > 0 and mtime is not None
                                    and time.time() - mtime < 10))
    if process_error:
        result["process_observation_error"] = process_error
    return result


def _system_metadata():
    values = {"platform": platform.platform(), "system": platform.system(),
              "release": platform.release(), "version": platform.version(), "gpu": []}
    if os.name != "nt":
        return values
    script = """
$null = [Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
$ErrorActionPreference = 'Stop'
$os = Get-CimInstance Win32_OperatingSystem | Select-Object Caption,Version,BuildNumber,OSArchitecture
$gpu = @(Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion,DriverDate,PNPDeviceID)
@{windows=$os;gpu=$gpu} | ConvertTo-Json -Depth 5 -Compress
"""
    command = "powershell.exe"
    system_root = os.environ.get("SystemRoot")
    if system_root:
        command = str(Path(system_root) / "System32/WindowsPowerShell/v1.0/powershell.exe")
    got = _run([command, "-NoProfile", "-NonInteractive", "-Command", script], timeout=8)
    try:
        if got["returncode"] == 0:
            values.update(json.loads(got["stdout"]))
        else:
            values["hardware_query_error"] = got["output"]
    except ValueError:
        values["hardware_query_error"] = got["output"]
    return values


def export_report(profile, destination):
    destination = Path(destination)
    if destination.is_dir():
        destination /= "nr-windows-report-" + datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S") + ".json"
    if destination.suffix.lower() != ".json":
        return {"ok": False, "error": "Report destination must be a JSON file"}
    try:
        observation = status(profile)
        log = Path(runtime_env(profile)["NR_LAYER_LOG"])
        tail = ""
        if log.is_file():
            with log.open("rb") as handle:
                handle.seek(max(0, log.stat().st_size - MAX_LOG_TAIL))
                tail = handle.read(MAX_LOG_TAIL).decode("utf-8", errors="replace")
        metadata = None
        metadata_error = None
        path = profile.root / "release-metadata.json"
        if path.is_file():
            try:
                metadata = _read_json(path, limit=64 * 1024)
            except (OSError, ValueError) as error:
                metadata_error = str(error)
        try:
            settings = _read_json(profile.root / "work/nr_settings.json")
            settings_error = None
        except (OSError, ValueError) as error:
            settings = None
            settings_error = str(error)
        # What Check says, so a report of a failed installation names the item that failed; the
        # DLL's version, and what the weight extractor said last (issue #12).
        try:
            checks = validate(profile)["checks"]
        except (OSError, ValueError) as error:
            checks = [{"name": "validate", "ok": False, "detail": str(error)}]
        weights_log = profile.root / WEIGHTS_LOG
        weights_tail = (weights_log.read_bytes()[-MAX_LOG_TAIL:].decode("utf-8", errors="replace")
                        if weights_log.is_file() else "")
        report = {"schema_version": SCHEMA_VERSION, "created_utc": datetime.now(timezone.utc).isoformat(),
                  "release": metadata, "release_metadata_error": metadata_error,
                  "root": str(profile.root), "profile": profile.to_dict(),
                  "system": _system_metadata(), "python": _python_info(profile.python),
                  "checks": checks, "dll": _dll_info(profile.dll), "weights_log_tail": weights_tail,
                  "settings": settings, "settings_error": settings_error,
                  "status": observation, "daemon_log_tail": tail,
                  "note": "Daemon timings and observed frame counts are not game FPS. No DLL or weights are included."}
        _atomic_json(destination, report)
        return {"ok": True, "error": None, "path": str(destination)}
    except (OSError, ValueError) as error:
        return {"ok": False, "error": str(error)}
