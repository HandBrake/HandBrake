#!/usr/bin/env python3
"""Assemble the built runtime for setup.sh/setup.bat, without model weights.

Both deploy scripts call this so Windows and Linux use the same completeness
checks. This copies an existing build; it does not compile or extract weights.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIRS = ("layer", "ref", "gpu", "bench")
WINDOWS_UI = ("NR-Setup.cmd", "windows-wizard.ps1")
WINDOWS_TOOLS = ("windows_wizard.py", "windows_wizard_core.py", "windows_launch.py")
PROXIES = ("nr_vulkan_proxy.dll", "nr_vulkan_proxy32.dll")
# DXVK for DirectX 8-11 games, from scripts/fetch_dxvk.py: the setup window puts it beside them.
DXVK = tuple(f"work/dxvk/{directory}/{name}" for directory in ("x64", "x32")
             for name in ("d3d8.dll", "d3d9.dll", "d3d10core.dll", "d3d11.dll", "dxgi.dll"))
DXVK_INFO = ("work/dxvk/LICENSE", "work/dxvk/VERSION")
IGNORE = shutil.ignore_patterns(
    ".git", ".venv", "__pycache__", "*.pyc", "*.pyo", "*.o", "*.obj",
    "*.lib", "*.exp", "*.dll", "*.exe", "*.so*", "*.spv", "*.safetensors",
    "*.bin", "*.npy", "*.npz", "*.ptx", "*.cubin", "*.fatbin", "*.png",
    "*.jpg", "*.jpeg", "*.webp", "*.mp4", "*.webm", "*.pt", "*.pth",
    "*.onnx", "*.gguf", "*.dylib", "*.sys", "*.bmp", "*.gif", "*.7z",
    "*.zip", "*.zst", "*.tar", "*.gz",
)


def required_shaders(root: Path) -> set[str]:
    """Use the filenames the runtime loads, plus the daemon's startup probe."""
    names = {"half_probe.spv"}
    for source in ("src/gpu/libxmx.c", "src/gpu/xmx.py", "src/gpu/xmxres.py"):
        names.update(re.findall(r"([A-Za-z0-9_]+\.spv)",
                                (root / source).read_text(encoding="utf-8")))
    return names


def assemble(root: Path, target: Path, platform: str) -> Path:
    root, target = root.resolve(), target.resolve()
    if platform not in ("linux", "windows"):
        raise ValueError(f"Unsupported platform: {platform}")
    # Never merge a release with stale weights, builds or user files.
    if target.exists() and (not target.is_dir() or any(target.iterdir())):
        raise ValueError(f"Release destination must be new or empty: {target}")
    for source in (root / "src", root / "work", root / "scripts", root / "dist-tools"):
        if target == source or source in target.parents or target in source.parents:
            raise ValueError(f"Release destination overlaps source files: {target}")

    windows = platform == "windows"
    layer = "nr_layer.dll" if windows else "libnr_layer.so"
    libraries = ("libxmx.dll", "libnr_image.dll", "libnr_alloc.dll") if windows else (
        "libxmx.so", "libnr_image.so")
    shaders = required_shaders(root)
    package = root / "work/mlx-dlss/python/mlxdlss"
    files = [root / "work" / name for name in (layer, *libraries, *sorted(shaders))]
    files += [
        root / "src/layer/nr_daemon.py", root / "src/layer/nr_knobs.py", root / "src/ref/nr_frame.py",
        root / "src/layer/VkLayer_dlss_nr.json", root / "scripts/get_weights.py",
        root / "dist-tools/setup.sh", root / "dist-tools/setup.bat",
        root / "docs/RELEASE-QUICKSTART.md", root / "LICENSE", root / "NOTICE",
        root / "work/mlx-dlss/LICENSE", package / "features.py",
        package / "tools/extract_dlssnr_weights.py",
        package / "tools/unpack_dlssnr_weights.py",
    ]
    if windows:
        files.append(root / "work/NR-Setup.exe")
        # The layer a 32-bit game loads; the daemon stays 64-bit.
        files.append(root / "work/nr_layer32.dll")
        # vulkan-1.dll beside each game, which gives it NR's environment however it starts.
        files += [root / "work" / name for name in PROXIES]
        files += [root / name for name in (*DXVK, *DXVK_INFO)]
        files += [root / "dist-tools" / name for name in WINDOWS_UI]
        files += [root / "src/tools" / name for name in WINDOWS_TOOLS]
    missing = [str(path.relative_to(root)) for path in files if not path.is_file()]
    if missing:
        raise ValueError("Incomplete build; missing: " + ", ".join(missing))

    target.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".nr-release-", dir=target.parent) as room:
        stage = Path(room) / "payload"
        (stage / "work").mkdir(parents=True)
        (stage / "scripts").mkdir()
        for name in SOURCE_DIRS:
            shutil.copytree(root / "src" / name, stage / "src" / name, ignore=IGNORE)
        # Runtime + extractor only, without the upstream checkout's .git or assets.
        shutil.copytree(package, stage / "work/mlx-dlss/python/mlxdlss", ignore=IGNORE)
        shutil.copy2(root / "work/mlx-dlss/LICENSE", stage / "work/mlx-dlss/LICENSE")
        shutil.copy2(root / "work" / layer, stage / ("nr_layer.dll" if windows else "nr_layer.so"))
        for name in (*libraries, *sorted(shaders)):
            shutil.copy2(root / "work" / name, stage / "work" / name)
        (stage / "work/nr_settings.json").write_text(
            json.dumps({"render_scale": 0.4, "min_extent": 320}) + "\n", encoding="utf-8")
        shutil.copy2(root / "src/layer/VkLayer_dlss_nr.json", stage / "VkLayer_dlss_nr.json")
        shutil.copy2(root / "scripts/get_weights.py", stage / "scripts/get_weights.py")
        for name in ("setup.sh", "setup.bat"):
            shutil.copy2(root / "dist-tools" / name, stage / name)
        if windows:
            shutil.copy2(root / "work/NR-Setup.exe", stage / "NR-Setup.exe")
            shutil.copy2(root / "work/nr_layer32.dll", stage / "nr_layer32.dll")
            for name in PROXIES:
                shutil.copy2(root / "work" / name, stage / name)
            for name in (*DXVK, *DXVK_INFO):
                destination = stage / "dxvk" / Path(name).relative_to("work/dxvk")
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(root / name, destination)
            for name in WINDOWS_UI:
                shutil.copy2(root / "dist-tools" / name, stage / name)
            for name in WINDOWS_TOOLS:
                shutil.copy2(root / "src/tools" / name, stage / "scripts" / name)
        # Bug reports from a prebuilt package do not require an installed Git.
        commit = None
        try:
            version = subprocess.run(["git", "-C", str(root), "rev-parse", "HEAD"],
                                     capture_output=True, text=True, timeout=3)
            if version.returncode == 0:
                commit = version.stdout.strip()
        except (OSError, subprocess.TimeoutExpired):
            pass
        (stage / "release-metadata.json").write_text(
            json.dumps({"schema_version": 1, "source_commit": commit,
                        "platform": platform, "windows_wizard": windows,
                        "dxvk": (root / "work/dxvk/VERSION").read_text(encoding="utf-8").strip()
                        if windows else None}, indent=2) + "\n",
            encoding="utf-8")
        (stage / "setup.sh").chmod(0o755)
        for name in ("LICENSE", "NOTICE"):
            shutil.copy2(root / name, stage / name)
        shutil.copy2(root / "docs/RELEASE-QUICKSTART.md", stage / "README.md")
        # All copying succeeds before the destination becomes a release. rmdir refuses
        # to remove a destination another process filled while we were assembling.
        if target.exists():
            target.rmdir()
        stage.rename(target)
    return target


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="new or empty release directory")
    parser.add_argument("--platform", choices=("linux", "windows"),
                        default="windows" if os.name == "nt" else "linux")
    args = parser.parse_args()
    try:
        target = assemble(ROOT, args.output, args.platform)
    except (OSError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 3
    print(f"Done. Release folder: {target}")
    print("No model weights or NVIDIA DLL included. Read README.md and run the setup script.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
