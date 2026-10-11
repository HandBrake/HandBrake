#!/usr/bin/env python3
"""8x16 per subgroup against a 32x32 register block, on the frame's real shapes."""
import json, os, pathlib, subprocess, sys, time
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]

sys.path.insert(0, str(ROOT / "src" / "gpu"))
import xmxres
rt = xmxres.Runtime()

SHAPES = [(983040, 128, 32, 2, 3), (983040, 32, 128, 2, 0), (245760, 128, 32, 8, 3),
          (245760, 32, 128, 8, 0), (61440, 128, 64, 16, 3), (15360, 128, 128, 63, 3),
          (3840, 128, 256, 129, 3), (3840, 256, 256, 20, 3), (960, 512, 512, 36, 3),
          (240, 4096, 1024, 8, 3), (240, 1024, 4096, 8, 0), (1536, 1536, 512, 4, 0),
          (4928, 768, 256, 4, 0), (999488, 96, 32, 1, 0)]
total = flops = 0.0
for m, n, k, calls, epi in SHAPES:
    a = rt.buffer(m * k, np.float16); w = rt.buffer(k * n, np.float16)
    c = rt.buffer(m * n, np.float16 if epi else np.float32)
    best = 1e9
    for repeat in range(3):
        rt.begin()
        for _ in range(calls):
            rt.gemm(a, w, c, m, n, k, epilogue=epi, narrow=bool(epi))
        t = time.perf_counter(); rt.submit()
        if repeat: best = min(best, time.perf_counter() - t)
    f = 2.0 * m * n * k * calls
    print("  %-24s %5d %8.2f ms %9.1f GFLOP/s" % (f"{m}x{n}x{k}", calls, 1000 * best, f / best / 1e9))
    total += best; flops += f
    for buf in (a, w, c): buf.free()
print("  %-24s %5s %8.2f ms %9.1f GFLOP/s" % ("TOTAL", "", 1000 * total, flops / total / 1e9))
