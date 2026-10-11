#!/usr/bin/env python3
"""Compare native FP16 packing with the previous softmax on identical frame buffers.

Build with `make work/attention_ab.spv`. A test-only specialization bit selects the
old algorithm; the two captured graphs share all allocations and other shaders.
Warm timings exclude compilation, allocation and capture.
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
sys.path.insert(0, str(ROOT / 'src' / 'ref'))
sys.path.insert(0, str(ROOT / 'src'))
import nr_build  # noqa: E402


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--size', type=int, nargs=2, default=(720, 1280), metavar=('H', 'W'))
    parser.add_argument('--input', type=pathlib.Path, required=True)
    parser.add_argument('--pairs', type=int, default=8)
    parser.add_argument('--json', type=pathlib.Path)
    args = parser.parse_args()
    if args.pairs < 1 or min(args.size) < 1:
        parser.error('size and pairs must be positive')
    os.environ['XMX_ROW_SPV'] = str(nr_build.shader('attention_ab.spv'))
    import nr_frame

    h, w = args.size
    color = nr_frame.image_io.load(args.input, size=(h, w))
    features = nr_frame.make_features(color, geometry=nr_frame.NetworkGeometry.vendor_aligned(w, h),
                                     **nr_frame.PROFILES['standard'])
    backend = nr_frame.ResidentBackend()
    rt = backend.runtime
    rt.specialize(7)
    graph_key = rt.graph_key()
    original = rt.lib.xmx_rec_row
    reference_mode = True

    def row(kind, *arguments):
        if reference_mode and kind & 255 == 1:
            kind |= 0x40000000
        return original(kind, *arguments)

    rt.lib.xmx_rec_row = row
    frame = backend.frame(*features.shape[:2])
    graphs, warmups = {}, {}
    modes = ('reference', 'native')
    times = {mode: [] for mode in modes}
    reference = None
    try:
        for mode in modes:
            reference_mode = mode == 'reference'
            started = time.perf_counter()
            head = frame.run(features, execution='replay')
            warmups[mode] = 1000 * (time.perf_counter() - started)
            # Separate captured graphs deliberately share the same frame allocations.
            graphs[mode] = frame._graphs.pop(graph_key)
            if reference is None:
                reference = head.copy()
            np.testing.assert_array_equal(head, reference)
            print(f'warmup {mode}: {warmups[mode]:.2f} ms, exact', flush=True)
        for pair in range(args.pairs):
            for mode in modes if pair % 2 == 0 else reversed(modes):
                frame._graphs[graph_key] = graphs[mode]
                started = time.perf_counter()
                head = frame.run(features, execution='replay')
                times[mode].append(1000 * (time.perf_counter() - started))
                np.testing.assert_array_equal(head, reference)
            print(f'round {pair+1}: ' + ', '.join(f'{m}={times[m][-1]:.2f}' for m in modes), flush=True)
        changed = features.copy()
        changed[..., 4:7] *= np.float32(0.8)
        frame._graphs[graph_key] = graphs['reference']
        a = frame.run(changed, execution='replay')
        frame._graphs[graph_key] = graphs['native']
        b = frame.run(changed, execution='replay')
        np.testing.assert_array_equal(a, b)
        assert not np.array_equal(a, reference)
        report = {'output_hw': [h, w], 'network_hw': list(features.shape[:2]),
                  'bit_identical': True, 'warmup_ms': warmups,
                  'head_sha256': hashlib.sha256(reference.tobytes()).hexdigest(),
                  'results': {mode: {'ms': values, 'median_ms': float(np.median(values))}
                              for mode, values in times.items()}}
        print(json.dumps(report, indent=2), flush=True)
        if args.json:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(json.dumps(report, indent=2) + '\n')
    finally:
        rt.lib.xmx_rec_row = original
        frame._graphs.clear()
        for graph in graphs.values():
            graph.free()
        backend.close()


if __name__ == '__main__':
    main()
