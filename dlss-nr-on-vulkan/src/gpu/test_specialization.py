#!/usr/bin/env python3
"""Specialized pipelines preserve GEMM layouts, epilogues and changing inputs.

Exercise all three kernel shapes, transpose, batches, non-dense leading dimensions,
operand offsets and unwritten guard regions. The generic GPU result is the exact
reference; a plain GEMM also checks against numpy to anchor the layout independently.
"""
import pathlib
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu"))
import xmxres


def check_gemm(rt, m, n, k, batch, transposed, rng):
    lda, ldb, ldc = k + 16, (k if transposed else n) + 16, n + 16
    b_rows = n if transposed else k
    strides = (m * lda + 64, b_rows * ldb + 64, m * ldc + 64)
    offsets = (16, 32, 16)
    a = rt.buffer(offsets[0] + batch * strides[0], np.float16)
    b = rt.buffer(offsets[1] + batch * strides[1], np.float16)
    for buf in (a, b):
        values = (rng.standard_normal(buf.nbytes // 2) * 0.3).astype(np.float16)
        # XMX flushes half subnormals; keep this layout test's independent numpy
        # control on the hardware's actual operands (phase4-subnormal-flush).
        values[np.abs(values) < np.finfo(np.float16).tiny] = 0
        xmxres.host_write(buf, values)
    expected = []
    for item in range(batch):
        a_host = xmxres.host_view(a, np.float16)
        b_host = xmxres.host_view(b, np.float16)
        av = a_host[offsets[0] + item * strides[0]:][:m * lda].reshape(m, lda)[:, :k]
        bv = b_host[offsets[1] + item * strides[1]:][:b_rows * ldb].reshape(b_rows, ldb)
        bv = bv[:, :k].T if transposed else bv[:, :n]
        expected.append(av.astype(np.float32) @ bv.astype(np.float32))
    for narrow in (False, True):
        dtype = np.float16 if narrow else np.float32
        c = rt.buffer(offsets[2] + batch * strides[2], dtype)
        written = np.zeros(c.nbytes // np.dtype(dtype).itemsize, dtype=bool)
        for item in range(batch):
            start = offsets[2] + item * strides[2]
            written[start:start + m * ldc].reshape(m, ldc)[:, :n] = True
        for epilogue in range(5):
            baseline = None
            for mask in (0, 7):
                rt.specialize(mask)
                xmxres.host_write(c, np.full(c.nbytes // np.dtype(dtype).itemsize,
                                             -123, dtype))
                rt.begin()
                rt.gemm(a, b, c, m, n, k, batch=batch, strides=strides,
                        leading=(lda, ldb, ldc), offsets=offsets, transpose_b=transposed,
                        epilogue=epilogue, narrow=narrow)
                rt.submit()
                result = np.array(xmxres.host_view(c, dtype), copy=True)
                assert np.all(result[~written] == -123), "GEMM overwrote padding/guards"
                if baseline is None:
                    baseline = result
                    if not narrow and epilogue == 0:
                        for item, want in enumerate(expected):
                            start = offsets[2] + item * strides[2]
                            got = result[start:start + m * ldc].reshape(m, ldc)[:, :n]
                            np.testing.assert_allclose(got, want, atol=3e-5, rtol=1e-5)
                else:
                    np.testing.assert_array_equal(result, baseline,
                        err_msg=f"mask={mask}, epi={epilogue}, narrow={narrow}")
        c.free()
    a.free()
    b.free()
    print(f"  exact: {m}x{n}x{k}, batch={batch}, transpose={transposed}", flush=True)


def check_dynamic_scale(rt):
    source = rt.buffer_from(np.linspace(-2, 2, 1024, dtype=np.float32))
    target = rt.buffer(1024, np.float16)
    for scale in (0.5, 2.0, -1.0):
        for mask in (0, 7):
            rt.specialize(mask)
            rt.begin()
            rt.to_half(source, target, 1024, scale=scale)
            rt.submit()
            np.testing.assert_array_equal(
                xmxres.host_view(target, np.float16, count=1024),
                (xmxres.host_view(source, count=1024) * scale).astype(np.float16))
    source.free()
    target.free()
    print("  exact: cached pipelines consume changing scale values", flush=True)


def main():
    rt = xmxres.Runtime()
    rng = np.random.default_rng(271)
    for shape in ((8, 16, 16, 1, False), (16, 32, 64, 2, False),
                  (64, 32, 128, 2, False), (64, 32, 128, 3, True),
                  (24, 48, 32, 1, True)):
        check_gemm(rt, *shape, rng)
    check_dynamic_scale(rt)
    print(f"all specialization checks passed ({rt.lib.xmx_specialized_count()} pipelines)")


if __name__ == "__main__":
    main()
