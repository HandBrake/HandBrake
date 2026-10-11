#!/usr/bin/env python3
"""Float16 subnormals go through the GEMMs, wherever the driver can be told to keep them.

Mesa flushes float16 subnormal operands in the cooperative-matrix GEMMs unless a shader
declares a float-controls mode, and Intel's Windows driver keeps them; the same graph then
draws a different picture on each (notes/phase71). libxmx declares `DenormPreserve 16` on
every module where the device reports `shaderDenormPreserveFloat16`. This checks that it does,
on each GEMM kernel the graph uses: an operand of 2^-20, a float16 subnormal, times 1 has to come
out as 2^-20, not 0. With `XMX_DENORM16=driver` the driver's own default is shown, not checked.
"""
import ctypes
import os
import pathlib
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu"))
import xmx      # noqa: E402
import xmxres   # noqa: E402

TINY = np.float16(2.0 ** -20)        # below float16's smallest normal, 2^-14
assert TINY != 0 and abs(float(TINY)) < 2.0 ** -14


def run(rt, rows, cols, inner):
    """C = A @ B with A[0, 0] subnormal, B the identity: C[0, 0] is A[0, 0] if it was kept."""
    A = np.zeros((rows, inner), np.float16)
    A[0, 0] = TINY
    A[1, 1] = np.float16(0.5)        # a normal value beside it, which every mode keeps
    B = np.eye(inner, cols, dtype=np.float16)
    a, b = rt.buffer_from(A, np.float16), rt.buffer_from(B, np.float16)
    c = rt.buffer(rows * cols)
    rt.begin()
    rt.gemm(a, b, c, rows, cols, inner)
    rt.submit()
    got = np.array(xmxres.host_view(c, np.float32, (rows, cols)))
    return float(got[0, 0]), float(got[1, 1])


def main():
    lib = xmx._load()
    lib.xmx_preserve16.restype = ctypes.c_int
    declared = bool(lib.xmx_preserve16())
    forced_driver = os.environ.get("XMX_DENORM16") == "driver"
    rt = xmxres.Runtime()
    print(f"  DenormPreserve 16 declared: {declared}"
          f"{' (XMX_DENORM16=driver)' if forced_driver else ''}")
    failures = []
    # the tiled kernel (the stem's, K = 16), the staged one (K from 32), a partial last block
    for rows, cols, inner in ((16, 32, 16), (64, 64, 64), (72, 32, 32)):
        kept, normal = run(rt, rows, cols, inner)
        status = "kept" if kept == float(TINY) else ("flushed" if kept == 0 else f"{kept!r}")
        print(f"  {rows:3d}x{cols:<3d} K={inner:<3d} 2^-20 -> {status}, 0.5 -> {normal}")
        if normal != 0.5:
            failures.append(f"{rows}x{cols}x{inner}: a normal operand came out {normal}")
        if declared and kept != float(TINY):
            failures.append(f"{rows}x{cols}x{inner}: the subnormal came out {kept!r} under "
                            "DenormPreserve 16")
    if failures:
        raise SystemExit("FAIL: " + "; ".join(failures))
    if declared:
        print("float16 subnormals are kept through every GEMM kernel")
    else:
        print("this driver's own float16 mode decides; nothing to check")


if __name__ == "__main__":
    main()
