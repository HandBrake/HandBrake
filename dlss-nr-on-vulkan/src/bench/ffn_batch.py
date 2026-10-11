#!/usr/bin/env python3
"""Paired runtime benchmark: replayed full graph, no game or image codecs.

Report warm wall times, not per-dispatch estimates. The input, buffers and shaders
are shared; both graphs are captured before timing. Every result must match exactly.
"""
import argparse
import pathlib
import statistics
import sys
import time

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'src/gpu'), str(ROOT / 'src/ref')]
import nr_frame
import xmx


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--size', nargs=2, type=int, metavar=('HEIGHT', 'WIDTH'),
                        default=(576, 1024), help='input extent, before model padding')
    parser.add_argument('--pairs', type=int, default=8)
    parser.add_argument('--optimization',
                        choices=('ffn', 'input', 'head', 'qkv', 'merge', 'qkv-epilogue',
                                 'glue', 'ffn-fused', 'branched-ffn', 'partition'),
                        default='ffn',
                        help='compare FFN batching, FP16 input, compact head, joint QKV '
                             'preparation, the head merge in the fused attention, the '
                             'QKV projection finished in its own epilogue, the fused '
                             'full-resolution glue or the fused narrow feed-forward')
    args = parser.parse_args()
    if min(*args.size, args.pairs) <= 0:
        parser.error('size and pairs must be positive')
    color = np.random.default_rng(67).random((*args.size, 3), dtype=np.float32)
    geometry = nr_frame.NetworkGeometry.vendor_aligned(args.size[1], args.size[0])
    features = nr_frame.build_features(color, geometry=geometry, **nr_frame.PROFILES['standard'])
    backend = nr_frame.ResidentBackend()
    try:
        rt = backend.runtime
        if args.optimization == 'qkv' and not rt.fuse_qk:
            raise SystemExit('QKV comparison requires NR_FUSE_QK=1; the split reference bypasses this option')
        if args.optimization == 'merge' and not rt.fuse_window_attention:
            raise SystemExit('merge comparison requires NR_FUSE_WINDOW_ATTENTION=1; '
                             'the three-pass attention bypasses this option')
        setting, variable = {'ffn': ('batch_ffn', 'NR_BATCH_FFN'),
                             'input': ('input_fp16', 'NR_INPUT_FP16'),
                             'head': ('compact_head', 'NR_COMPACT_HEAD'),
                             'qkv': ('joint_qkv', 'NR_JOINT_QKV'),
                             'merge': ('fuse_attention_merge', 'NR_FUSE_ATTENTION_MERGE'),
                             'qkv-epilogue': ('qkv_epilogue', 'NR_QKV_EPILOGUE'),
                             'glue': ('fuse_glue', 'NR_FUSE_GLUE'),
                             'ffn-fused': ('fuse_ffn', 'NR_FUSE_FFN'),
                             'branched-ffn': ('fuse_branched_ffn', 'NR_FUSE_BRANCHED_FFN'),
                             'partition': ('fuse_partition', 'NR_FUSE_PARTITION'),
                             }[args.optimization]
        frame = backend.frame(*features.shape[:2])
        samples = {False: [], True: []}
        splits = {False: [], True: []}
        counts = {}
        expected = None
        for mode in (False, True):
            setattr(rt, setting, mode)
            passes = []
            head = frame.run(features, execution='replay', submits=passes)
            counts[mode] = passes[0]
            if expected is None:
                expected = head
            else:
                np.testing.assert_array_equal(head, expected)
            # Warm both captured graphs; never time weight uploads or shader compilation.
            np.testing.assert_array_equal(frame.run(features, execution='replay'), expected)
        print(f'device: {xmx.device_name()}', flush=True)
        print(f'buffers: {xmx.memory_note()}', flush=True)
        print(f'input extent: {args.size[1]}x{args.size[0]}; '
              f'network extent: {features.shape[1]}x{features.shape[0]}; '
              f'{args.pairs} alternating pairs', flush=True)
        fixed = ', '.join(f'{name}={int(getattr(rt, name))}'
                          for name in ('batch_ffn', 'fuse_qk', 'input_fp16', 'compact_head',
                                       'joint_qkv', 'fuse_residual', 'fuse_window_residual',
                                       'fuse_window_attention', 'fuse_attention_merge',
                                       'qkv_epilogue', 'fuse_glue', 'fuse_ffn',
                                       'fuse_branched_ffn', 'fuse_partition')
                          if name != setting)
        print(f'comparing {variable}; fixed {fixed}; staging={rt.staging}', flush=True)
        for pair in range(args.pairs):
            for mode in ((False, True) if pair % 2 == 0 else (True, False)):
                setattr(rt, setting, mode)
                started = time.perf_counter()
                head = frame.run(features, execution='replay')
                samples[mode].append(1000 * (time.perf_counter() - started))
                splits[mode].append(frame.split)
                np.testing.assert_array_equal(head, expected)
        for mode in (False, True):
            values = samples[mode]
            print(f'{variable}={int(mode)}: {counts[mode]} recorded passes, '
                  f'median {statistics.median(values):.3f} ms, '
                  f'range {min(values):.3f}..{max(values):.3f} ms')
            parts = [1000 * statistics.median(x[i] for x in splits[mode]) for i in range(3)]
            print('  host write / graph wait / host read: '
                  + ' / '.join(f'{v:.3f}' for v in parts) + ' ms (wall times, not PCIe counters)')
        before, after = (statistics.median(samples[m]) for m in (False, True))
        print(f'pass reduction: {counts[False] - counts[True]}; '
              f'warm frame speedup: {before / after:.3f}x; '
              f'time saved: {before - after:.3f} ms')
        print('All heads bit-identical. Includes input write and head read; excludes '
              'feature assembly, composition, IPC and game time. This is not game FPS.')
    finally:
        backend.close()


if __name__ == '__main__':
    main()
