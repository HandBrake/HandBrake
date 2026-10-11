#!/usr/bin/env python3
"""
Our network against OpenDLSS-NR's, on the same input features.

`maanHimself/OpenDLSS-NR` claims its network bit-exact against captures of the original on an
NVIDIA GPU — every block boundary, byte for byte — and its WebGPU port claims the same bytes with no
FP8 and no tensor cores. That port runs here, on the Arc 140V through headless Chromium
(`opendlss_reference.mjs`), from a model directory `src/tools/opendlss_model.py` writes out of the
DLL. So for the first time this project has something to hold its output against that says it is the
vendor's arithmetic, not a recovery of it.

This feeds both networks the same features — ours, built as the daemon builds them — and compares
the heads and the pictures they compose. The two must agree on the padded field, or the window grids
differ and the comparison measures that instead: the script refuses a size where our field rule
(`nr_frame.network_geometry`) and theirs (`geometryFromValid`) disagree — which, since ours became
theirs on 2026-09-27, is only below 129 pixels a side.

    python3 src/bench/opendlss_reference.py [--size 320x180] [--image IMAGE] [--save DIR]

Needs a clone of the repository (OPENDLSS_PORT, default work/opendlss-nr/ports/browser-webgpu), the
model directory (OPENDLSS_MODEL, default work/opendlss-model), node and Chromium.
"""
import argparse
import json
import os
import pathlib
import subprocess
import sys
import tempfile

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "src" / "ref"), str(ROOT / "src" / "gpu"), str(ROOT / "src" / "layer")]

import image_io        # noqa: E402
import nr_frame        # noqa: E402

PORT = pathlib.Path(os.environ.get("OPENDLSS_PORT", ROOT / "work" / "opendlss-nr" / "ports" / "browser-webgpu"))
MODEL = pathlib.Path(os.environ.get("OPENDLSS_MODEL", ROOT / "work" / "opendlss-model"))
RUNNER = pathlib.Path(__file__).with_name("opendlss_reference.mjs")


def their_field(valid_width, valid_height):
    """`geometryFromValid` in the port's geometry.js: the padded field for a valid size."""
    def align_up(value, alignment):
        return -(-value // alignment) * alignment

    def alignment(valid):
        reductions, size = 0, valid
        for level in range(6):
            half = align_up((size + 1) // 2, 4)
            reductions += half < size
            reductions += level == 0 and half % 8 != 0
            size = half
        return 1 << reductions

    align_width, align_height = alignment(valid_width), alignment(valid_height)
    width = max(320, align_up(valid_width, align_width))
    height = max(320, align_up(valid_height, align_height))
    if width % (4 * align_width) == 0 and height % (4 * align_height) == 0:
        width += align_width
    return width, height


def e4m3_table():
    """E4M3 (fn) code -> float32: bias 7, subnormals in 2^-9 steps, 0x7f and 0xff NaN."""
    codes = np.arange(256)
    sign = np.where(codes & 0x80, -1.0, 1.0)
    exponent, mantissa = (codes >> 3) & 15, codes & 7
    value = np.where(exponent == 0, mantissa / 8 * 2.0 ** -6, (1 + mantissa / 8) * 2.0 ** (exponent - 7))
    value = (sign * value).astype(np.float32)
    value[[0x7F, 0xFF]] = np.nan
    return value


E4M3 = e4m3_table()


def reference_run(features, valid_width, valid_height, *, capture=False):
    """Their head for `features` (field rows, 16), f32 (field height, field width, 4), the runner's
    result, and with `capture` every block boundary it records, as float32 (rows, channels)."""
    height, width = features.shape[:2]
    with tempfile.TemporaryDirectory(prefix="opendlss-io-") as io:
        io = pathlib.Path(io)
        np.ascontiguousarray(features, dtype=np.float32).tofile(io / "features.bin")
        done = subprocess.run(["node", str(RUNNER), str(PORT), str(MODEL), str(io),
                               str(valid_width), str(valid_height)] + (["capture"] if capture else []),
                              capture_output=True, text=True)
        result = json.loads((io / "result.json").read_text()) if (io / "result.json").exists() else {}
        if done.returncode != 0 or not result.get("ok"):
            raise RuntimeError(f"the reference failed: {result.get('error') or done.stderr[-2000:]}")
        head = np.fromfile(io / "head.bin", np.float32).reshape(height, width, 4)
        bounds = {}
        if capture:
            for entry in json.loads((io / "boundaries.json").read_text()):
                raw = (io / entry["file"]).read_bytes()
                if entry["format"] == "e4":
                    values = E4M3[np.frombuffer(raw, np.uint8)]
                elif entry["format"] == "f16":
                    values = np.frombuffer(raw, np.float16).astype(np.float32)
                else:
                    raise ValueError(f"boundary {entry['name']} is {entry['format']}")
                bounds[entry["name"]] = values[:entry["rows"] * entry["channels"]].reshape(
                    entry["rows"], entry["channels"])
    return head, result, bounds


def reference_head(features, valid_width, valid_height):
    """Their head for `features` (field rows, 16), f32 (field height, field width, 4)."""
    head, result, _ = reference_run(features, valid_width, valid_height)
    return head, result


def compose(colour, head):
    """The head's composition without history: `clamp(proxy + rgb / 4, 0, 1)`."""
    return np.clip(colour + head[..., :3] / 4, 0.0, 1.0)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--size", default="320x180", help="the valid frame, WxH")
    parser.add_argument("--image", default=str(ROOT / "pngs" / "Cyberpunk-2077_01.jpg"))
    parser.add_argument("--profile", default="standard", choices=sorted(nr_frame.PROFILES))
    parser.add_argument("--save", help="write ours, theirs and the difference as PNGs here")
    args = parser.parse_args()
    valid_width, valid_height = (int(v) for v in args.size.split("x"))

    geometry = nr_frame.network_geometry(valid_width, valid_height)
    ours_field = (geometry.network_width, geometry.network_height)
    if ours_field != their_field(valid_width, valid_height):
        raise SystemExit(f"{args.size}: our field {ours_field} is not theirs "
                         f"{their_field(valid_width, valid_height)}; the window grids would differ")

    import nr_daemon
    colour = nr_daemon.resample(np.asarray(image_io.load(args.image), np.float32),
                                (valid_height, valid_width))
    features = nr_frame.build_features(colour, geometry=geometry, **nr_frame.PROFILES[args.profile])

    backend = nr_frame.ResidentBackend()
    backend.run_features(features)
    ours = np.array(backend.run_features(features), np.float32)
    theirs, result = reference_head(features, valid_width, valid_height)
    if tuple(result["field"]) != ours_field:
        raise SystemExit(f"the reference ran a {result['field']} field, not {ours_field}")

    crop = geometry.crop
    ours_valid, theirs_valid = crop(ours), crop(theirs)
    print(f"{args.size} on a {ours_field[0]}x{ours_field[1]} field, {pathlib.Path(args.image).name}; "
          f"reference on {result['adapter']}, {min(result['times']):.0f} ms a frame")
    for channel, name in enumerate(("red", "green", "blue", "gate logit")):
        a, b = ours_valid[..., channel].ravel(), theirs_valid[..., channel].ravel()
        print(f"  head {name:10s} corr {np.corrcoef(a, b)[0, 1]:.4f}  mean |d| {np.abs(a - b).mean():.4f}  "
              f"sd ours {a.std():.4f} theirs {b.std():.4f}")
    mine, reference = compose(colour, ours_valid), compose(colour, theirs_valid)
    levels = np.abs(mine - reference) * 255
    print(f"  composed: mean {levels.mean():.2f} levels of 255 apart, 99th percentile "
          f"{np.percentile(levels, 99):.1f}, max {levels.max():.1f}; the pass itself moves the frame "
          f"{(np.abs(mine - colour) * 255).mean():.2f} (ours) and {(np.abs(reference - colour) * 255).mean():.2f} "
          f"(theirs) levels")
    if args.save:
        destination = pathlib.Path(args.save)
        destination.mkdir(parents=True, exist_ok=True)
        stem = f"{args.size}-{pathlib.Path(args.image).stem}"
        image_io.save(mine, destination / f"{stem}-ours.png")
        image_io.save(reference, destination / f"{stem}-theirs.png")
        image_io.save(np.clip(np.abs(mine - reference) * 8, 0, 1), destination / f"{stem}-diff8x.png")
        np.save(destination / f"{stem}-heads.npy", np.stack([ours, theirs]))
        print(f"  -> {destination}/{stem}-{{ours,theirs,diff8x}}.png")


if __name__ == "__main__":
    main()
