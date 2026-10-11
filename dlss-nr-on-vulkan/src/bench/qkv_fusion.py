#!/usr/bin/env python3
"""Paired full-frame measurement of fused Q/K splitting and cosine normalization."""
import argparse
import hashlib
import json
import pathlib
import sys
import time
import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'src' / 'ref'))
import nr_frame


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--size', type=int, nargs=2, default=(720, 1280), metavar=('H', 'W'))
    parser.add_argument('--input', type=pathlib.Path, required=True)
    parser.add_argument('--pairs', type=int, default=6)
    parser.add_argument('--json', type=pathlib.Path)
    args = parser.parse_args()
    if args.pairs < 1 or min(args.size) < 1:
        parser.error('size and pairs must be positive')
    h, w = args.size
    color = nr_frame.image_io.load(args.input, size=(h, w))
    features = nr_frame.make_features(color, geometry=nr_frame.NetworkGeometry.vendor_aligned(w, h),
                                     **nr_frame.PROFILES['standard'])
    backend = nr_frame.ResidentBackend()
    frame = backend.frame(*features.shape[:2])
    modes = ('separate', 'fused')
    times = {mode: [] for mode in modes}
    warmups, passes = {}, {}
    reference = None
    for mode in modes:
        backend.runtime.fuse_qk = mode == 'fused'
        recorded = []
        started = time.perf_counter()
        head = frame.run(features, submits=recorded, execution='replay')
        warmups[mode] = 1000 * (time.perf_counter() - started)
        passes[mode] = recorded[0]
        if reference is None:
            reference = head.copy()
        np.testing.assert_array_equal(head, reference)
        print(f'warmup {mode}: {warmups[mode]:.2f} ms, {recorded[0]} passes, exact', flush=True)
    for pair in range(args.pairs):
        for mode in modes if pair % 2 == 0 else reversed(modes):
            backend.runtime.fuse_qk = mode == 'fused'
            started = time.perf_counter()
            head = frame.run(features, execution='replay')
            times[mode].append(1000 * (time.perf_counter() - started))
            np.testing.assert_array_equal(head, reference)
        print(f'round {pair+1}: '+', '.join(f'{m}={times[m][-1]:.2f}' for m in modes), flush=True)
    changed = features.copy()
    changed[..., 4:7] *= np.float32(0.8)
    backend.runtime.fuse_qk = False
    a = frame.run(changed, execution='replay')
    backend.runtime.fuse_qk = True
    b = frame.run(changed, execution='replay')
    np.testing.assert_array_equal(a, b)
    assert not np.array_equal(a, reference)
    report = {'output_hw': [h, w], 'network_hw': list(features.shape[:2]),
              'bit_identical': True, 'warmup_ms': warmups, 'passes': passes,
              'head_sha256': hashlib.sha256(reference.tobytes()).hexdigest(),
              'results': {mode: {'ms': values, 'median_ms': float(np.median(values))}
                          for mode, values in times.items()}}
    print(json.dumps(report, indent=2), flush=True)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2)+'\n')
    backend.close()


if __name__ == '__main__':
    main()
