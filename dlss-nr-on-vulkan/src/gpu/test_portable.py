#!/usr/bin/env python3
"""The GEMM contract, on whichever kernels the device runs — and on the portable ones by
force.

`gemm_portable.comp` claims to be `gemm_resident.comp` in every respect a caller can see:
transposed B, row strides, operand offsets, the batch, every epilogue, the half output,
and the 16x32 build the runtime picks for whole-block shapes. This checks each of those
against numpy, through the same `Runtime.gemm` the graph uses, plus the two descriptor
paths (`xmx.gemm`, `xmx.bmm_aligned`) behind the benchmarks.

Run once it exercises whatever the device has. `make test` runs it a second time with
`XMX_PORTABLE=1`, which on a cooperative-matrix device forces the portable kernels — so
the machine this project was built on tests the path a Mac runs, without owning one.
"""
import os
import pathlib
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu"))
sys.path.insert(0, str(ROOT / "src" / "ref"))
import xmx      # noqa: E402
import xmxres   # noqa: E402
import nr_model  # noqa: E402

rng = np.random.default_rng(11)
failures = []


def report(name, got, want, tol):
    d = float(np.abs(got.astype(np.float64) - want.astype(np.float64)).max())
    ok = d <= tol
    print("  %-52s max|d| %.3e  %s" % (name, d, "OK" if ok else "FAIL"))
    if not ok:
        failures.append(name)


def reference(A, B, epilogue, narrow):
    out = A.astype(np.float32) @ B.astype(np.float32)
    if epilogue in (xmxres.EPI_GATE, xmxres.EPI_GATE_E4M3):
        out = nr_model.quadratic_gate_activation(out)
    if epilogue in (xmxres.EPI_E4M3, xmxres.EPI_GATE_E4M3):
        out = nr_model.e4m3(out)
    if epilogue == xmxres.EPI_HALF or narrow:
        out = nr_model._half_rounded(out)
    return out


def resident(rt):
    print("resident path:")
    # 8x16 blocks and 16x32 blocks (rows % 16 == 0 and cols % 32 == 0 takes the tiled build)
    for rows, cols, inner, batch, transpose in ((8, 16, 16, 1, False), (24, 48, 32, 1, False),
                                                (32, 64, 128, 1, False), (32, 64, 128, 2, True),
                                                (16, 32, 16, 3, True), (40, 80, 64, 2, False)):
        A = (rng.standard_normal((batch, rows, inner)) * 0.5).astype(np.float16)
        Bt = (rng.standard_normal((batch, cols, inner)) * 0.5).astype(np.float16)
        B = np.ascontiguousarray(Bt if transpose else Bt.transpose(0, 2, 1))
        want = np.stack([reference(A[i], Bt[i].T, 0, False) for i in range(batch)])
        a = rt.buffer_from(A, np.float16); b = rt.buffer_from(B, np.float16)
        c = rt.buffer(batch * rows * cols)
        rt.begin()
        rt.gemm(a, b, c, rows, cols, inner, batch=batch, transpose_b=transpose)
        rt.submit()
        got = xmxres.host_view(c, np.float32, (batch, rows, cols))
        report("%dx%dx%d batch %d%s" % (rows, cols, inner, batch, " B^T" if transpose else ""),
               got, want, 2e-6 * inner)
        for buf in (a, b, c):
            buf.free()

    # every epilogue, wide and narrow, on one shape that takes the tiled build
    rows, cols, inner = 64, 96, 64
    A = (rng.standard_normal((rows, inner)) * 0.3).astype(np.float16)
    B = (rng.standard_normal((inner, cols)) * 0.3).astype(np.float16)
    a = rt.buffer_from(A, np.float16); b = rt.buffer_from(B, np.float16)
    for epilogue, label in ((xmxres.EPI_NONE, "none"), (xmxres.EPI_E4M3, "e4m3"),
                            (xmxres.EPI_GATE, "gate"), (xmxres.EPI_GATE_E4M3, "gate+e4m3"),
                            (xmxres.EPI_HALF, "half")):
        for narrow in (False, True):
            dtype = np.float16 if narrow else np.float32
            c = rt.buffer(rows * cols, dtype)
            rt.begin()
            rt.gemm(a, b, c, rows, cols, inner, epilogue=epilogue, narrow=narrow)
            rt.submit()
            got = xmxres.host_view(c, dtype, (rows, cols)).astype(np.float32)
            want = reference(A, B, epilogue, narrow)
            # exact for the publishes: they round to a grid the GEMM's own error cannot
            # straddle, except at a rounding boundary — one quantum is allowed there
            tol = 0.0 if epilogue == xmxres.EPI_NONE and not narrow else float(
                np.abs(np.diff(np.unique(want))).min() if np.unique(want).size > 1 else 0)
            tol = max(tol, 2e-6 * inner)
            report("epilogue %-10s %s" % (label, "half out" if narrow else "float out"),
                   got, want, tol)
            c.free()
    a.free(); b.free()

    # a slice of a wider buffer: row strides and element offsets on all three operands
    rows, cols, inner = 16, 32, 32
    wide_a, wide_b, wide_c = 48, 80, 96
    A = (rng.standard_normal((rows, wide_a)) * 0.5).astype(np.float16)
    B = (rng.standard_normal((inner, wide_b)) * 0.5).astype(np.float16)
    a = rt.buffer_from(A, np.float16); b = rt.buffer_from(B, np.float16)
    c = rt.buffer(rows * wide_c); c.zero()
    oa, ob, oc = 8, 16, 32
    rt.begin()
    rt.gemm(a, b, c, rows, cols, inner, leading=(wide_a, wide_b, wide_c), offsets=(oa, ob, oc))
    rt.submit()
    got = xmxres.host_view(c, np.float32, (rows, wide_c))
    want = np.zeros((rows, wide_c), np.float32)
    want[:, oc:oc + cols] = reference(A[:, oa:oa + inner], B[:, ob:ob + cols], 0, False)
    report("strided slice: lda %d ldb %d ldc %d, offsets %d/%d/%d" % (wide_a, wide_b, wide_c, oa, ob, oc),
           got, want, 2e-6 * inner)
    for buf in (a, b, c):
        buf.free()


def descriptor():
    print("descriptor path:")
    A = rng.standard_normal((100, 70)).astype(np.float32)
    B = rng.standard_normal((70, 45)).astype(np.float32)
    got = xmx.gemm(A, B)
    want = A.astype(np.float16).astype(np.float32) @ B.astype(np.float16).astype(np.float32)
    report("xmx.gemm 100x45x70 (padded, rescaled)", got, want, 2e-6 * 70 * float(np.abs(want).max()))
    A3 = rng.standard_normal((4, 64, 32)).astype(np.float32)
    B3 = rng.standard_normal((4, 64, 32)).astype(np.float32)
    got = xmx.bmm_aligned(A3, B3, transpose_b=True)
    want = np.einsum("bik,bjk->bij", A3.astype(np.float16).astype(np.float32),
                     B3.astype(np.float16).astype(np.float32))
    report("xmx.bmm_aligned 4x64x64x32 B^T", got, want, 2e-6 * 32 * float(np.abs(want).max()))


def main():
    print("=== GEMM contract on: %s ===" % xmx.path_note())
    forced = os.environ.get("XMX_PORTABLE", "")
    if forced and forced != "0" and not xmx.portable():
        raise SystemExit("XMX_PORTABLE=%s was set but the library did not take the portable path" % forced)
    rt = xmxres.Runtime()
    resident(rt)
    descriptor()
    if failures:
        print("\n%d check(s) failed: %s" % (len(failures), ", ".join(failures)))
        return 1
    print("\nall GEMM contract checks passed on this path")
    return 0


raise SystemExit(main())
