#!/usr/bin/env python3
"""Check scratch aliasing against separate storage and release after replay."""
import os
import pathlib
import sys
import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'ref'))
import nr_frame


def main():
    if not nr_frame.WEIGHTS.exists():
        print('scratch arena: skipped (local logical weights missing)')
        return
    backend = nr_frame.ResidentBackend()
    rt = backend.runtime
    initial_bytes = rt.buffer_bytes
    for extent in ((320, 320), (320, 384)):
        color = np.random.default_rng(30).random((*extent, 3), dtype=np.float32)
        features = nr_frame.make_features(color, **nr_frame.PROFILES['standard'])
        changed = features.copy()
        changed[..., 4:7] *= np.float32(0.6)
        expected = {}
        sizes = {}
        for compact in (False, True):
            os.environ['NR_SCRATCH_ARENA'] = str(int(compact))
            frame = backend.frame(*features.shape[:2])
            for fusion in (False, True):
                rt.fuse_qk = fusion
                for index, values in enumerate((features, changed)):
                    head = frame.run(values, execution='replay')
                    if index not in expected:
                        expected[index] = head.copy()
                    np.testing.assert_array_equal(head, expected[index])
                    np.testing.assert_array_equal(frame.run(values, execution='block'), expected[index])
                    np.testing.assert_array_equal(frame.run(values, execution='single'), expected[index])
            sizes[compact] = rt.buffer_bytes - initial_bytes
            if compact:
                try:
                    frame._arena.buffer('unplanned', 16)
                except RuntimeError:
                    pass
                else:
                    raise AssertionError('unplanned storage accepted after capture')
                # Shared intervals have the same address; persistent residuals do not.
                arena = frame._arena
                assert arena.buffer('hidden16', 1, np.float16).id == arena.buffer('proj', 1).id
                assert arena.buffer('proj', 1).id != arena.buffer('ffn', 1).id
            backend.close()
            assert rt.buffer_bytes == initial_bytes, 'resident allocations leaked'
        assert sizes[True] < sizes[False]
        print(f'{extent}: separate {sizes[False]/2**20:.1f}, compact {sizes[True]/2**20:.1f} MiB; exact')
    print('scratch arena: both fusion modes, all executions, changed inputs and release OK')


if __name__ == '__main__':
    main()
