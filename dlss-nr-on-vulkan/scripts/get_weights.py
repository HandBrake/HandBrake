#!/usr/bin/env python3
"""Get the DLSS-NR weights from *your own* nvngx_dlssnr.dll.

This script does NOT contain, download, or redistribute any NVIDIA binary or any
weights derived from one. It only drives the third-party, Apache-2.0 extractor
(MLX-DLSS) against a DLL you supply, and writes the result into the project's
git-ignored ``work/`` tree (``work/mlxw/dlssnr-logical.safetensors``), which is
exactly where ``nr_frame.WEIGHTS`` and ``nr_daemon.py`` look for it.

Steps it performs:

1. Check the DLL you point at exists and is a file.
2. If ``work/mlx-dlss`` is missing, clone it and pin it to the commit the graph
   recovery was validated against (``06a3e11a8b68817127406ace5c764463543f699b``).
3. Run ``extract_dlssnr_weights.py`` then ``unpack_dlssnr_weights.py``.
4. Verify the logical safetensors landed and has 649 tensors.

Usage:

    python3 scripts/get_weights.py /path/to/nvngx_dlssnr.dll
    python3 scripts/get_weights.py D:\\games\\nvngx_dlssnr.dll --work-dir work

The project root is located by walking up from this file, so the script works no
matter where you invoke it from.
"""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MLX_DLSS_URL = "https://github.com/iamwavecut/MLX-DLSS.git"
MLX_DLSS_PIN = "06a3e11a8b68817127406ace5c764463543f699b"
EXPECTED_TENSORS = 649

# Names of the upstream extractor under work/mlx-dlss/python/mlxdlss/tools/
EXTRACT = "extract_dlssnr_weights.py"
UNPACK = "unpack_dlssnr_weights.py"


def run(cmd, cwd=None):
    print("+ " + " ".join(str(c) for c in cmd), flush=True)
    return subprocess.run(cmd, cwd=cwd)


def ensure_mlx(work: Path) -> Path:
    mlx = work / "mlx-dlss"
    tools = mlx / "python" / "mlxdlss" / "tools"
    if (tools / EXTRACT).exists() and (tools / UNPACK).exists():
        print(f"MLX-DLSS already present at {mlx}")
        return tools
    if mlx.is_dir() and any(mlx.iterdir()):
        # Something is there but it is not a full clone - a deployment that carried only
        # the runtime modules, for instance. Saying so beats a clone failure that reads
        # as a network problem.
        sys.exit(
            f"{mlx} exists but has no {EXTRACT} under {tools}.\n"
            "  It looks like a partial copy: the runtime modules without the tools.\n"
            "  Move it aside, or clone it yourself:\n"
            f"    git clone {MLX_DLSS_URL} {mlx} && git -C {mlx} checkout {MLX_DLSS_PIN}"
        )
    mlx.mkdir(parents=True, exist_ok=True)
    print(f"Cloning MLX-DLSS into {mlx} (pinned at {MLX_DLSS_PIN})…")
    if run(["git", "clone", MLX_DLSS_URL, str(mlx)]).returncode != 0:
        sys.exit("git clone of MLX-DLSS failed; check your network.")
    if run(["git", "-C", str(mlx), "checkout", MLX_DLSS_PIN]).returncode != 0:
        sys.exit(f"could not pin MLX-DLSS at {MLX_DLSS_PIN}.")
    if not (tools / EXTRACT).exists():
        sys.exit(f"{EXTRACT} not found under {tools} after clone.")
    return tools


def count_tensors(path: Path) -> int:
    # Safetensors stores a JSON header of length 8 bytes at the very start.
    import json
    with path.open("rb") as handle:
        length = int.from_bytes(handle.read(8), "little")
        header = json.loads(handle.read(length))
    # The header maps tensor-name -> {dtype, shape, data_offsets}; __metadata__ is not a tensor.
    return sum(1 for key in header if key != "__metadata__")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Extract DLSS-NR logical weights from your own nvngx_dlssnr.dll."
    )
    parser.add_argument("dll", type=Path, help="path to YOUR copy of nvngx_dlssnr.dll")
    parser.add_argument(
        "--work-dir", type=Path, default=ROOT / "work",
        help="project work tree (default: <root>/work, git-ignored)",
    )
    parser.add_argument(
        "--skip-extract", action="store_true",
        help="assume work/mlxw/dlssnr-packed.safetensors already exists; just unpack",
    )
    args = parser.parse_args()

    dll: Path = args.dll
    if not dll.is_file():
        sys.exit(f"DLL not found: {dll}\n"
                 "You must supply your own nvngx_dlssnr.dll (version 310.8.0.0). "
                 "This project does not ship it.")

    work: Path = args.work_dir
    mlxw = work / "mlxw"
    packed = mlxw / "dlssnr-packed.safetensors"
    logical = mlxw / "dlssnr-logical.safetensors"

    if not args.skip_extract:
        tools = ensure_mlx(work)
        mlxw.mkdir(parents=True, exist_ok=True)
        extract = tools / EXTRACT
        unpack = tools / UNPACK
        if run([sys.executable, str(extract), str(dll), str(packed)]).returncode != 0:
            sys.exit("weight extraction failed; see the extractor's output above.")
        if not packed.is_file():
            sys.exit(f"extractor finished but {packed} was not produced.")

    if run([sys.executable, str(unpack if args.skip_extract else (ensure_mlx(work) / UNPACK)),
           str(packed), str(logical)]).returncode != 0:
        sys.exit("weight unpacking failed; see the extractor's output above.")

    if not logical.is_file():
        sys.exit(f"unpack finished but {logical} was not produced.")

    try:
        n = count_tensors(logical)
    except Exception as exc:  # noqa: BLE001 - report rather than crash
        print(f"warning: could not read tensor count ({exc}); continuing.")
        n = -1

    if n != EXPECTED_TENSORS:
        print(f"\nWARNING: expected {EXPECTED_TENSORS} tensors, found {n}. "
              "The model will likely refuse to load. Re-extract from a clean DLL.")
    else:
        print(f"\nOK: {logical} — {n} logical tensors. The daemon can now load it.")

    print("\nNext: start the daemon, it will find the weights automatically,\nor run a still:")
    print(f"    python3 {ROOT / 'src' / 'ref' / 'nr_frame.py'} IN.png OUT.png --resident")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
