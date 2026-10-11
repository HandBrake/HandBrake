#!/usr/bin/env python3
"""Compare pipeline specialization on the same buffers in alternating order.

Masks: 0 = generic, 1 = GEMM, 2 = elementwise, 4 = attention reductions, 7 = all.
Compilation/allocation warmups are reported separately. Every measured output must
be bit-identical to the generic graph, including when the input changes.
"""
import argparse
import hashlib
import json
import os
import pathlib
import sys
import time

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu"))
sys.path.insert(0, str(ROOT / "src" / "ref"))

import nr_frame


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--size", nargs=2, type=int, default=(320, 320), metavar=("H", "W"))
    parser.add_argument("--masks", default="0,1,2,4,7")
    parser.add_argument("--pairs", type=int, default=4)
    parser.add_argument("--input", type=pathlib.Path)
    parser.add_argument("--json", type=pathlib.Path)
    parser.add_argument("--shader-dir", type=pathlib.Path,
                        help="use saved pre-change SPIR-V files for a baseline")
    args = parser.parse_args()
    masks = [int(value) for value in args.masks.split(",")]
    if args.pairs < 1 or any(mask < 0 or mask > 7 for mask in masks):
        parser.error("--pairs must be positive and masks must be in 0..7")
    if args.shader_dir:
        for variable, filename in (("XMX_GEMM_SPV", "gemm_resident.spv"),
                                   ("XMX_TILED_SPV", "gemm_tiled.spv"),
                                   ("XMX_STAGED_SPV", "gemm_staged.spv"),
                                   ("XMX_UNARY_SPV", "resident.spv"),
                                   ("XMX_ROW_SPV", "attention.spv")):
            os.environ[variable] = str((args.shader_dir / filename).resolve())
    height, width = args.size
    if args.input:
        color = nr_frame.image_io.load(args.input, size=(height, width))
    else:
        yy, xx = np.mgrid[:height, :width].astype(np.float32)
        color = np.stack((0.5 + 0.3 * np.sin(xx / 9), 0.5 + 0.3 * np.cos(yy / 11),
                          0.5 + 0.3 * np.sin((xx + yy) / 13)), -1)
    geometry = nr_frame.NetworkGeometry.vendor_aligned(width, height)
    features = nr_frame.make_features(color, geometry=geometry, **nr_frame.PROFILES["standard"])
    backend = nr_frame.ResidentBackend()
    frame = backend.frame(*features.shape[:2])
    rt = backend.runtime
    reference = None
    times = {mask: [] for mask in masks}
    warmups = {}
    print(f"output {width}x{height}, network {features.shape[1]}x{features.shape[0]}", flush=True)
    for mask in dict.fromkeys([0, *masks]):
        rt.specialize(mask)
        started = time.perf_counter()
        head = frame.run(features)
        warmups[mask] = 1000 * (time.perf_counter() - started)
        if reference is None:
            reference = head.copy()
        if not np.array_equal(head, reference):
            raise AssertionError(f"mask {mask}: warmup differs, max|d|={np.abs(head-reference).max()}")
        print(f"warmup mask {mask}: {warmups[mask]:.1f} ms; exact; "
              f"{rt.lib.xmx_specialized_count()} cached pipelines", flush=True)
    for pair in range(args.pairs):
        for mask in masks if pair % 2 == 0 else reversed(masks):
            rt.specialize(mask)
            started = time.perf_counter()
            head = frame.run(features)
            elapsed = 1000 * (time.perf_counter() - started)
            if not np.array_equal(head, reference):
                raise AssertionError(f"mask {mask}: measured output differs")
            times[mask].append(elapsed)
        print(f"round {pair + 1}: " + ", ".join(
            f"{mask}={times[mask][-1]:.1f} ms" for mask in masks), flush=True)
    # A cached pipeline must still consume this frame's inputs, not warmup data.
    changed = nr_frame.make_features(color, frame_index=1, geometry=geometry,
                                     **nr_frame.PROFILES["standard"])
    rt.specialize(0)
    changed_reference = frame.run(changed)
    for mask in masks:
        rt.specialize(mask)
        if not np.array_equal(frame.run(changed), changed_reference):
            raise AssertionError(f"mask {mask}: changed input differs")
    results = {str(mask): {"ms": values, "median_ms": float(np.median(values))}
               for mask, values in times.items()}
    report = {"output_hw": [height, width], "network_hw": list(features.shape[:2]),
              "warmup_ms": warmups, "results": results, "bit_identical": True,
              "head_sha256": hashlib.sha256(reference.tobytes()).hexdigest(),
              "changed_head_sha256": hashlib.sha256(changed_reference.tobytes()).hexdigest()}
    print(json.dumps(report, indent=2), flush=True)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
