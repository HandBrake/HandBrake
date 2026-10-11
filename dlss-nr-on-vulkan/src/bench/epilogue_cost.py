#!/usr/bin/env python3
"""Where the staged epilogue's time goes: staging, the barrier, or the arithmetic."""
import pathlib, sys, time
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu"))
import xmxres
rt = xmxres.Runtime()

def time_it(record, calls):
    best = 1e9
    for repeat in range(3):
        rt.begin()
        for _ in range(calls):
            record()
        started = time.perf_counter(); rt.submit()
        if repeat:
            best = min(best, time.perf_counter() - started)
    return 1000 * best

m, n, k, calls = 245760, 128, 32, 8
a = rt.buffer(m * k, np.float16); w = rt.buffer(k * n, np.float16)
c = rt.buffer(m * n, np.float32); h = rt.buffer(m * n, np.float16)
bytes_of = {"plain f32 store": m * k * 2 + m * n * 4,
            "staged f32 out, no epilogue": m * k * 2 + m * n * 4,
            "staged half out, no epilogue": m * k * 2 + m * n * 2,
            "staged half out, half_round": m * k * 2 + m * n * 2,
            "staged half out, e4m3": m * k * 2 + m * n * 2,
            "staged half out, gate+e4m3": m * k * 2 + m * n * 2,
            "plain + gate_e4m3_half pass": m * k * 2 + m * n * 4 * 2 + m * n * 2}
cases = [("plain f32 store", lambda: rt.gemm(a, w, c, m, n, k)),
         ("staged f32 out, no epilogue", lambda: rt.gemm(a, w, c, m, n, k, epilogue=5)),
         ("staged half out, no epilogue", lambda: rt.gemm(a, w, h, m, n, k, epilogue=5, narrow=True)),
         ("staged half out, half_round", lambda: rt.gemm(a, w, h, m, n, k, epilogue=4, narrow=True)),
         ("staged half out, e4m3", lambda: rt.gemm(a, w, h, m, n, k, epilogue=1, narrow=True)),
         ("staged half out, gate+e4m3", lambda: rt.gemm(a, w, h, m, n, k, epilogue=3, narrow=True)),
         ("plain + gate_e4m3_half pass",
          lambda: (rt.gemm(a, w, c, m, n, k), rt.gate_e4m3_half(c, h, m * n)))]
print("  %dx%dx%d, %d calls\n" % (m, n, k, calls))
print("  %-32s %9s %9s" % ("variant", "ms", "GB/s"))
for name, fn in cases:
    ms = time_it(fn, calls)
    print("  %-32s %9.2f %9.1f" % (name, ms, calls * bytes_of[name] / (ms / 1000) / 1e9))
