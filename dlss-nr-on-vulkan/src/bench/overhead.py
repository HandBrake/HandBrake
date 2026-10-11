#!/usr/bin/env python3
"""Per-dispatch floor: how much of a recorded GEMM is fixed cost, not work."""
import pathlib, sys, time
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu"))
import xmxres

rt = xmxres.Runtime()

def run(m, n, k, calls, batch=1):
    a = rt.buffer(m * k * batch, np.float16); w = rt.buffer(k * n * batch, np.float16)
    c = rt.buffer(m * n * batch, np.float32)
    rt.begin(); rt.gemm(a, w, c, m, n, k, batch=batch); rt.submit()
    best = 1e9
    for _ in range(3):
        rt.begin()
        for _ in range(calls):
            rt.gemm(a, w, c, m, n, k, batch=batch)
        t = time.perf_counter(); rt.submit(); best = min(best, time.perf_counter() - t)
    a.free(); w.free(); c.free()
    return 1000 * best / calls

print("  %-24s %10s %10s" % ("shape", "us/dispatch", "GFLOP/s"))
for m, n, k, calls in [(8, 16, 16, 2000), (64, 128, 128, 2000), (256, 256, 256, 1000),
                       (1024, 128, 256, 500), (3840, 128, 256, 200), (3840, 128, 256, 1),
                       (15360, 128, 128, 200), (4096, 1024, 1024, 40)]:
    ms = run(m, n, k, calls)
    print("  %-24s %10.1f %10.1f" % (f"{m}x{n}x{k} ({calls} calls)", 1000 * ms,
                                     2.0 * m * n * k / (ms / 1000) / 1e9))
