#!/usr/bin/env python3
"""GEMM + publish pass, separate vs fused into the GEMM epilogue."""
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

print("  %-24s %6s %9s %9s %9s %7s" % ("shape", "calls", "split ms", "fused ms", "GB saved", "gain"))
for m, n, k, calls in [(983040, 128, 32, 2), (245760, 128, 32, 8), (61440, 128, 64, 16),
                       (15360, 128, 128, 63), (3840, 128, 256, 129), (960, 512, 512, 36),
                       (240, 4096, 1024, 8)]:
    a = rt.buffer(m * k, np.float16); w = rt.buffer(k * n, np.float16)
    c = rt.buffer(m * n, np.float32); h = rt.buffer(m * n, np.float16)
    split = time_it(lambda: (rt.gemm(a, w, c, m, n, k), rt.gate_e4m3_half(c, h, m * n)), calls)
    fused = time_it(lambda: rt.gemm(a, w, h, m, n, k, epilogue=3, narrow=True), calls)
    saved = calls * m * n * 8 / 1e9        # 4 B written + 4 B read + 2 B written -> 2 B
    print("  %-24s %6d %9.2f %9.2f %9.2f %6.2fx"
          % (f"{m}x{n}x{k}", calls, split, fused, saved, split / fused))
    for buf in (a, w, c, h): buf.free()
