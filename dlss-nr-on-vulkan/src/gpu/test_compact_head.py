#!/usr/bin/env python3
"""Four-column GEMM stores: exact tiles, output guards and full-frame combinations."""
import pathlib
import sys
import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'ref'))
import nr_frame
import xmxres


def operators(rt):
    rng = np.random.default_rng(69)
    cases = 0
    for mask in (0, 7):
        rt.specialize(mask)
        for rows in (8, 16, 64):
            for batch in (1, 3):
                for transpose in (False, True):
                    k = 32
                    a = rt.buffer_from(rng.normal(0, .3, (batch, rows, k)), np.float16)
                    b = rt.buffer_from(rng.normal(0, .3, (batch, 16, k) if transpose
                                                   else (batch, k, 16)), np.float16)
                    wide = rt.buffer(batch * rows * 16)
                    compact = rt.buffer(batch * rows * 4 + 64)
                    try:
                        xmxres.host_write(compact, np.full(batch * rows * 4 + 64, -11, np.float32))
                        rt.begin()
                        rt.gemm(a, b, wide, rows, 16, k, batch=batch, transpose_b=transpose)
                        rt.gemm(a, b, compact, rows, 16, k, batch=batch,
                                transpose_b=transpose, compact_output=True)
                        rt.submit()
                        expected = xmxres.host_view(wide, shape=(batch, rows, 16))[..., :4]
                        got = xmxres.host_view(compact)
                        np.testing.assert_array_equal(got[:batch * rows * 4].reshape(batch, rows, 4), expected)
                        np.testing.assert_array_equal(got[batch * rows * 4:], -11)
                        cases += 1
                    finally:
                        for buf in (a, b, wide, compact):
                            buf.free()
    for kw in ({'cols': 32}, {'narrow': True}, {'epilogue': xmxres.EPI_HALF},
               {'leading': (0, 0, 4)}, {'offsets': (0, 0, 4)}):
        args = dict(rows=8, cols=16, inner=32, compact_output=True) | kw
        try:
            rt.gemm(None, None, None, **args)
        except ValueError:
            pass
        else:
            raise AssertionError(f'invalid compact call accepted: {kw}')
    print(f'compact head: {cases} exact GPU cases; guards and argument checks OK')


def frames():
    if not nr_frame.WEIGHTS.exists():
        print('compact head: full frame skipped (no logical weights)')
        return
    backend = nr_frame.ResidentBackend()
    try:
        rt = backend.runtime
        for height, width in ((320, 320), (384, 320)):
            color = np.random.default_rng(69).random((height, width, 3), dtype=np.float32)
            features = nr_frame.make_features(color, **nr_frame.PROFILES['standard'])
            frame = backend.frame(*features.shape[:2])
            expected = None
            counts = {}
            for mask in (7, 0):
                rt.specialize(mask)
                for rt.input_fp16 in (False, True):
                    for rt.compact_head in (True, False, True):
                        passes = []
                        head = frame.run(features, execution='replay', submits=passes)
                        if expected is None:
                            expected = head.copy()
                        np.testing.assert_array_equal(head, expected)
                        counts[rt.compact_head] = passes[0]
                        for execution in ('single', 'block', 'replay'):
                            np.testing.assert_array_equal(frame.run(features, execution=execution), expected)
                    assert counts[False] == counts[True], counts
            assert frame._buffers['head'].nbytes == frame._buffers['head4'].nbytes * 4
            changed = features.copy()
            changed[..., 4:7] *= np.float32(.75)
            results = []
            for rt.compact_head in (False, True):
                results.append(frame.run(changed, execution='replay'))
            np.testing.assert_array_equal(*results)
            assert not np.array_equal(results[0], expected)
            print(f'compact head: {width}x{height} exact, including FP16 input combinations; '
                  'quarter-size output, no added pass')
    finally:
        backend.close()


if __name__ == '__main__':
    operators(xmxres.Runtime())
    frames()
