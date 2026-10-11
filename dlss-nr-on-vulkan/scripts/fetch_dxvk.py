#!/usr/bin/env python3
"""Put DXVK where a Windows release takes it from: work/dxvk/{x64,x32}, its licence and version.

The setup window puts DXVK beside a DirectX 8-11 game, so a Windows release carries it. The
version and the archive's SHA-256 are pinned in dist-tools/dxvk.json; the archive comes from
DXVK's GitHub release, or from --archive, and is refused unless it is that file. Nothing in it
is changed: the five DLLs of each architecture are copied out as they are.

    python scripts/fetch_dxvk.py                          # does nothing if work/dxvk is current
    python scripts/fetch_dxvk.py --archive X:\\dxvk-3.1.1.tar.gz
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tarfile
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
PIN = ROOT / "dist-tools/dxvk.json"
LICENSE = ROOT / "dist-tools/DXVK-LICENSE.txt"
DIRECTORIES = ("x64", "x32")
FILES = ("d3d8.dll", "d3d9.dll", "d3d10core.dll", "d3d11.dll", "dxgi.dll")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def current(target: Path, version: str) -> bool:
    marker = target / "VERSION"
    return (marker.is_file() and marker.read_text(encoding="utf-8").strip() == version
            and (target / "LICENSE").is_file()
            and all((target / directory / name).is_file() for directory in DIRECTORIES for name in FILES))


def fetch(target: Path, archive: Path | None = None, pin: Path = PIN) -> Path:
    pinned = json.loads(pin.read_text(encoding="utf-8"))
    version = pinned["version"]
    target = target.resolve()
    if current(target, version):
        return target
    target.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".dxvk-", dir=target.parent) as room:
        room = Path(room)
        if archive is None:
            archive = room / Path(pinned["url"]).name
            with urllib.request.urlopen(pinned["url"], timeout=120) as response, archive.open("wb") as out:
                shutil.copyfileobj(response, out)
        if sha256(archive) != pinned["sha256"]:
            raise ValueError(f"{archive} is not DXVK {version}: its SHA-256 differs from {pin.name}'s")
        stage = room / "dxvk"
        with tarfile.open(archive, "r:gz") as tar:
            for directory in DIRECTORIES:
                for name in FILES:
                    member = tar.extractfile(f"dxvk-{version}/{directory}/{name}")
                    if member is None:
                        raise ValueError(f"{archive} has no regular {directory}/{name}")
                    out = stage / directory / name
                    out.parent.mkdir(parents=True, exist_ok=True)
                    out.write_bytes(member.read())
        shutil.copy2(LICENSE, stage / "LICENSE")
        (stage / "VERSION").write_text(version + "\n", encoding="utf-8")
        if target.exists():
            shutil.rmtree(target)
        stage.rename(target)
    return target


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--archive", type=Path, help="a local copy of the pinned DXVK archive")
    parser.add_argument("--target", type=Path, default=ROOT / "work/dxvk")
    args = parser.parse_args()
    try:
        target = fetch(args.target, args.archive)
    except (OSError, ValueError, KeyError, tarfile.TarError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 3
    print(f"DXVK {(target / 'VERSION').read_text(encoding='utf-8').strip()} in {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
