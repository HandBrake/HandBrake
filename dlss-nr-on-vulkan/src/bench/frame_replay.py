#!/usr/bin/env python3
"""Paired full-graph benchmark: block submissions, single submit, cached replay.

Runs on identical buffers, checks exact results with changed input, and reports
allocation/capture separately from warm timing. Requires local logical weights.
"""
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
    parser.add_argument('--size', type=int, nargs=2, default=(384, 384), metavar=('H', 'W'))
    parser.add_argument('--pairs', type=int, default=6)
    parser.add_argument('--input', type=pathlib.Path)
    parser.add_argument('--json', type=pathlib.Path)
    args = parser.parse_args()
    if args.pairs < 1 or min(args.size) < 1:
        parser.error('size and pairs must be positive')
    h, w = args.size
    if args.input:
        color = nr_frame.image_io.load(args.input, size=(h, w))
    else:
        yy, xx = np.mgrid[:h, :w].astype(np.float32)
        color = np.stack((0.5 + 0.3*np.sin(xx/9), 0.5 + 0.3*np.cos(yy/11),
                          0.5 + 0.3*np.sin((xx+yy)/13)), -1)
    geometry = nr_frame.NetworkGeometry.vendor_aligned(w, h)
    features = nr_frame.make_features(color, geometry=geometry, **nr_frame.PROFILES['standard'])
    backend = nr_frame.ResidentBackend()
    frame = backend.frame(*features.shape[:2])
    modes = ['block', 'single', 'replay']
    times = {mode: [] for mode in modes}
    warmups = {}
    reference = None
    for mode in modes:
        started = time.perf_counter()
        head = frame.run(features, execution=mode)
        warmups[mode] = 1000 * (time.perf_counter() - started)
        if reference is None:
            reference = head.copy()
        np.testing.assert_array_equal(head, reference, err_msg=mode)
        print(f'warmup {mode}: {warmups[mode]:.2f} ms, exact', flush=True)
    for pair in range(args.pairs):
        for mode in modes if pair % 2 == 0 else reversed(modes):
            started = time.perf_counter()
            head = frame.run(features, execution=mode)
            times[mode].append(1000 * (time.perf_counter()-started))
            np.testing.assert_array_equal(head, reference, err_msg=mode)
        print(f'round {pair+1}: ' + ', '.join(f'{m}={times[m][-1]:.2f}' for m in modes), flush=True)
    changed = nr_frame.make_features(color, frame_index=1, geometry=geometry,
                                     **nr_frame.PROFILES['standard'])
    changed_reference = frame.run(changed, execution='block')
    assert not np.array_equal(changed_reference, reference), 'input change must affect output'
    for mode in modes[1:]:
        np.testing.assert_array_equal(frame.run(changed, execution=mode), changed_reference)
    # Intervening ordinary command recording must not overwrite the saved graph.
    rt = backend.runtime
    a = rt.buffer_from(np.arange(32, dtype=np.float32))
    b = rt.buffer(32)
    rt.begin()
    rt.scale(a, b, 32, 2)
    rt.submit()
    np.testing.assert_array_equal(frame.run(features, execution='replay'), reference)
    report = {'output_hw': [h, w], 'network_hw': list(features.shape[:2]),
              'warmup_ms': warmups, 'bit_identical': True,
              'head_sha256': hashlib.sha256(reference.tobytes()).hexdigest(),
              'changed_head_sha256': hashlib.sha256(changed_reference.tobytes()).hexdigest(),
              'results': {m: {'ms': t, 'median_ms': float(np.median(t))} for m, t in times.items()}}
    print(json.dumps(report, indent=2), flush=True)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2)+'\n')
    frame.close()


if __name__ == '__main__':
    main()
