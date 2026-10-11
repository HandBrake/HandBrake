#!/usr/bin/env python3
"""Host FP16 conversion against the GPU, then exact full-frame A/B replay."""
import pathlib
import sys

import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'ref'))
import nr_frame
import xmxres


def conversion(rt):
    # Every half value, plus both sides of every positive finite rounding boundary
    # and their negatives. This includes normal/subnormal transitions and ties.
    half = np.arange(65536, dtype=np.uint16).view(np.float16).astype(np.float32)
    positive = np.arange(0x7c00, dtype=np.uint16).view(np.float16).astype(np.float32)
    mid = (positive[:-1] + positive[1:]) * np.float32(.5)
    boundary = np.concatenate((mid, np.nextafter(mid, np.float32(-np.inf)),
                               np.nextafter(mid, np.float32(np.inf))))
    values = np.concatenate((half, boundary, -boundary,
                             np.array([65519, 65520, 65521, -65519, -65520, -65521], np.float32)))
    source = rt.buffer_from(values)
    output = rt.buffer(values.size, np.float16)
    try:
        rt.begin()
        rt.to_half(source, output, values.size)
        rt.submit()
        got = xmxres.host_view(output, np.float16).copy()
        want = np.empty(values.size, np.float16)
        with np.errstate(over='ignore', invalid='ignore'):
            np.copyto(want, values, casting='unsafe')
        np.testing.assert_array_equal(np.isnan(got), np.isnan(want))
        non_nan = ~np.isnan(want)
        np.testing.assert_array_equal(got.view(np.uint16)[non_nan], want.view(np.uint16)[non_nan])
        print(f'input FP16: {values.size} GPU/CPU conversions match, including signed zeros and ties')
    finally:
        source.free()
        output.free()


def frames():
    if not nr_frame.WEIGHTS.exists():
        print('input FP16: full frame skipped (no logical weights)')
        return
    backend = nr_frame.ResidentBackend()
    try:
        rt = backend.runtime
        for height, width in ((320, 320), (384, 320)):
            color = np.random.default_rng(68).random((height, width, 3), dtype=np.float32)
            features = nr_frame.make_features(color, **nr_frame.PROFILES['standard'])
            frame = backend.frame(*features.shape[:2])
            counts = {}
            for mask in (7, 0):
                rt.specialize(mask)
                expected = None
                # Start cold with the new mode; restore old recordings after toggling.
                for mode in (True, False, True, False):
                    rt.input_fp16 = mode
                    passes = []
                    head = frame.run(features, execution='replay', submits=passes)
                    counts[mode] = passes[0]
                    if expected is None:
                        expected = head.copy()
                    else:
                        np.testing.assert_array_equal(head, expected)
                    for execution in ('single', 'block'):
                        np.testing.assert_array_equal(frame.run(features, execution=execution), expected)
                assert counts[False] - counts[True] == 1, counts
            pixels = np.prod(features.shape[:2])
            assert frame._buffers['features_host16'].nbytes == pixels * 16 * 2
            assert frame._buffers['features'].nbytes == pixels * 16 * 4
            changed = features.copy()
            changed[..., 4:7] *= np.float32(.75)
            outputs = []
            for rt.input_fp16 in (False, True):
                outputs.append(frame.run(changed, execution='replay'))
            np.testing.assert_array_equal(*outputs)
            assert not np.array_equal(outputs[0], expected)
            print(f'input FP16: {width}x{height} exact across schedules, specialization, '
                  'changed inputs and cached toggles; half-size input, one fewer pass')
    finally:
        backend.close()


if __name__ == '__main__':
    conversion(xmxres.Runtime())
    frames()
