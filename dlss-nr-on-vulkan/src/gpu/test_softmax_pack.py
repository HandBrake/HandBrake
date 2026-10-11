#!/usr/bin/env python3
"""Check the FP16 packing replacement, including all finite FP16 logits."""
import os
import pathlib
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src"))
import nr_build  # noqa: E402
os.environ['XMX_ROW_SPV'] = str(nr_build.shader('attention_ab.spv'))
import xmxres


def check_affine_domain():
    # Exhaust the pair domain after clamping/half rounding. The old decoder treats
    # exponent 31 as a finite number; prove this cannot occur before using native
    # unpackHalf2x16, whose exponent-31 semantics would be different.
    low = np.float16(1.03125).view(np.uint16).item()
    high = np.float16(1.5693359375).view(np.uint16).item()
    bits = np.arange(low, high + 1, dtype=np.uint32)
    transformed = ((bits[:, None] | (bits[None, :] << 16)) << 5) + np.uint32(0x7ff88000)
    for half in (transformed & 65535, transformed >> 16):
        exponent, mantissa = (half >> 10) & 31, half & 1023
        assert ((exponent > 0) & (exponent < 31)).all()
        decoded = (((exponent + 112) << 23) | (mantissa << 13)).view(np.float32)
        native = half.astype(np.uint16).view(np.float16).astype(np.float32)
        np.testing.assert_array_equal(native, decoded)
    return transformed.size


def main():
    pairs = check_affine_domain()
    rt = xmxres.Runtime()
    original = rt.lib.xmx_rec_row
    reference = False

    def record_row(kind, *args):
        return original(kind | (0x40000000 if reference else 0), *args)

    rt.lib.xmx_rec_row = record_row
    logits = np.arange(65536, dtype=np.uint16).view(np.float16)
    logits = logits[np.isfinite(logits)].astype(np.float32)
    rng = np.random.default_rng(29)
    cases = 0
    for width, stride, cap, bias_heads in ((32, 32, 0, 0), (64, 64, 6, 2),
                                           (84, 96, 3, 0), (96, 96, 0, 0)):
        rows = xmxres.align(-(-len(logits) // width), 32)
        values = np.full((rows, stride), 12345, dtype=np.float32)
        values[:, :width] = np.resize(logits, rows * width).reshape(rows, width)
        source = rt.buffer_from(values)
        bias = (rt.buffer_from(rng.uniform(-2, 2, (bias_heads, width, width)).astype(np.float32))
                if bias_heads else None)
        for narrow in (False, True):
            dtype = np.float16 if narrow else np.float32
            out = rt.buffer(rows * stride + 32, dtype)
            xmxres.host_write(out, np.full(out.nbytes // np.dtype(dtype).itemsize, -11, dtype))
            for mask in (0, 7):
                rt.specialize(mask)
                expected = None
                for mode in (True, False):
                    reference = mode
                    rt.begin()
                    rt.softmax(source, out, rows, width, stride=stride, cap=cap,
                               bias=bias, heads=bias_heads or 1, narrow=narrow)
                    rt.submit()
                    actual = np.array(xmxres.host_view(out, dtype), copy=True)
                    assert np.isfinite(actual).all()
                    np.testing.assert_array_equal(actual[-32:], np.full(32, -11, dtype=dtype))
                    if expected is None:
                        expected = actual
                    else:
                        np.testing.assert_array_equal(actual, expected)
                    if stride > width:
                        np.testing.assert_array_equal(actual[:rows*stride].reshape(rows,stride)[:,width:], 0)
                cases += 1
    print(f'softmax packing: {pairs} affine pairs; all finite half logits; {cases} GPU cases exact')


if __name__ == '__main__':
    main()
