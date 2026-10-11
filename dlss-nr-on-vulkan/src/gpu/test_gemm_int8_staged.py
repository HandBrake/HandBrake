#!/usr/bin/env python3
"""The staged GEMM on the integer path against exact integer arithmetic.

`gemm_staged_int8.comp` multiplies int8 activations by int8 weights stored transposed into
an exact int32 accumulator, then scales it: `float(acc) * a_scale[row] * b_scale[column]`.
numpy computes the same accumulator exactly and applies the same two roundings, so the
output must match bit for bit — on the bottleneck's shapes, on a partial last block of
rows, and with nothing written past the output.
"""
import numpy as np
import xmxres as X


def main():
    rt = X.Runtime()
    rng = np.random.default_rng(8)
    cases = 0
    for rows, cols, inner in ((64, 1024, 1024), (64, 32, 64), (100, 96, 128),
                              (256, 3072, 1024), (64, 1024, 4096), (16, 4096, 1024)):
        a = rng.integers(-127, 128, (rows, inner), dtype=np.int8)
        b = rng.integers(-127, 128, (cols, inner), dtype=np.int8)
        a_scale = rng.uniform(0.001, 0.02, rows).astype(np.float32)
        b_scale = rng.uniform(0.001, 0.02, cols).astype(np.float32)
        buffers = [rt.buffer(rows * inner, np.int8), rt.buffer(cols * inner, np.int8),
                   rt.buffer(rows, np.float32), rt.buffer(cols, np.float32),
                   rt.buffer(rows * cols + 64, np.float32)]
        A, B, SA, SB, C = buffers
        try:
            X.host_write(A, a)
            X.host_write(B, b)
            X.host_write(SA, a_scale)
            X.host_write(SB, b_scale)
            X.host_write(C, np.full(rows * cols + 64, np.nan, np.float32))
            rt.begin()
            rt.gemm_int8(A, B, C, SA, SB, rows, cols, inner)
            rt.submit()
            got = X.host_view(C)[:rows * cols].reshape(rows, cols)
            exact = a.astype(np.int64) @ b.astype(np.int64).T
            assert np.abs(exact).max() < 1 << 24, "the accumulator must stay exact in float"
            want = (exact.astype(np.float32) * a_scale[:, None]) * b_scale[None, :]
            assert np.array_equal(got.view(np.uint32), want.astype(np.float32).view(np.uint32)), \
                f"{rows}x{cols}x{inner}: {np.count_nonzero(got != want)} elements differ"
            assert np.isnan(X.host_view(C)[rows * cols:]).all(), "written past the output"
            cases += 1
        finally:
            for buf in buffers:
                buf.free()
    print(f"integer staged GEMM: {cases} shapes bit-exact against exact integer arithmetic")


if __name__ == "__main__":
    main()
