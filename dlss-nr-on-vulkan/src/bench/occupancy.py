#!/usr/bin/env python3
"""Does a dispatch fill the machine, and at what workgroup count?

Fixed work per workgroup, growing grid. If throughput rises linearly and then flattens,
the flattening point is where the 64 XMX engines are saturated; anything beyond it is
queueing. If it never rises, the kernel is bound by something other than the units.

One workgroup is one subgroup of 32 here, computing a 16x32 block of the output.
"""
import pathlib, sys, time
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu"))
import xmxres

rt = xmxres.Runtime()
N, K = 32, 1024                      # one column block, deep enough to be arithmetic
print("  16x32 blocks of output, K=%d, one subgroup each\n" % K)
print("  %10s %10s %9s %11s %12s %9s" % ("workgroups", "M", "ms", "GFLOP/s", "us/workgroup", "of peak"))
peak = 32000.0
prev = None
for shift in range(0, 14):
    groups = 1 << shift
    M = groups * 16
    if M * K * 2 > 3 << 30:
        break
    a = rt.buffer(M * K, np.float16); w = rt.buffer(K * N, np.float16)
    c = rt.buffer(M * N, np.float16)
    calls = max(1, min(400, 4_000_000 // max(groups, 1)))
    best = 1e9
    for repeat in range(3):
        rt.begin()
        for _ in range(calls):
            rt.gemm(a, w, c, M, N, K, epilogue=xmxres.EPI_HALF, narrow=True)
        t = time.perf_counter(); rt.submit()
        if repeat:
            best = min(best, time.perf_counter() - t)
    per = best / calls
    flops = 2.0 * M * N * K
    print("  %10d %10d %9.4f %11.1f %12.2f %8.1f%%"
          % (groups, M, 1000 * per, flops / per / 1e9, 1e6 * per / groups,
             100 * flops / per / 1e9 / peak))
    for buf in (a, w, c): buf.free()
