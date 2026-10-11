#!/usr/bin/env python3
"""Compare resident allocations and exact outputs with a shared scratch arena.

Layouts run sequentially and are freed between runs; timings are descriptive, not
same-buffer A/B evidence. No simultaneous full-size models are required.
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
sys.path.insert(0, str(ROOT/'src/ref'))
import nr_frame


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--size', type=int, nargs=2, default=(720, 1280), metavar=('H', 'W'))
    parser.add_argument('--input', type=pathlib.Path, required=True)
    parser.add_argument('--repeats', type=int, default=4)
    parser.add_argument('--json', type=pathlib.Path)
    parser.add_argument('--compact-only', action='store_true', help='avoid a large baseline allocation')
    args = parser.parse_args()
    if min(args.size) < 1 or args.repeats < 1:
        parser.error('size and repeats must be positive')
    h, w = args.size
    color = nr_frame.image_io.load(args.input, size=(h, w))
    features = nr_frame.make_features(color, geometry=nr_frame.NetworkGeometry.vendor_aligned(w, h),
                                     **nr_frame.PROFILES['standard'])
    backend = nr_frame.ResidentBackend()
    backend.runtime.fuse_qk = True
    reference = None
    results = {}
    for mode in ('compact',) if args.compact_only else ('separate', 'compact'):
        os.environ['NR_SCRATCH_ARENA'] = '1' if mode == 'compact' else '0'
        before = backend.runtime.buffer_bytes
        frame = backend.frame(*features.shape[:2])
        start = time.perf_counter()
        head = frame.run(features, execution='replay')
        cold = 1000*(time.perf_counter()-start)
        if reference is None:
            reference = head.copy()
        np.testing.assert_array_equal(head, reference)
        times = []
        for _ in range(args.repeats):
            start = time.perf_counter()
            head = frame.run(features, execution='replay')
            times.append(1000*(time.perf_counter()-start))
            np.testing.assert_array_equal(head, reference)
        memory = backend.runtime.buffer_bytes - before
        results[mode] = {'buffer_bytes': memory, 'mib': memory/2**20,
                         'cold_ms': cold, 'ms': times, 'median_ms': float(np.median(times))}
        print(mode, results[mode], flush=True)
        backend.close()
        assert backend.runtime.buffer_bytes == before, 'frame leaked resident buffers'
    report = {'output_hw': [h, w], 'head_sha256': hashlib.sha256(reference.tobytes()).hexdigest(),
              'layouts_bit_identical': not args.compact_only, 'results': results}
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2), flush=True)


if __name__ == '__main__':
    main()
