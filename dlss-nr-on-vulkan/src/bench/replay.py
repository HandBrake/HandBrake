#!/usr/bin/env python3
"""Replay a frame's GEMM census on the device: per-shape rate and the frame total.

Each shape is recorded `calls` times into one submit, exactly as the frame records it
(one global barrier between dispatches), so the sum is the GEMM half of a frame time.
"""
import json, pathlib, sys, time
import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu"))
import xmxres

shapes = json.loads((ROOT / "work" /
                     (sys.argv[1] if len(sys.argv) > 1 else "shapes-768x1280.json")).read_text())
shapes.sort(key=lambda s: -2.0 * s["m"] * s["n"] * s["k"] * s["batch"] * s["calls"])

rt = xmxres.Runtime()
print("  %-30s %6s %9s %9s %9s" % ("M x N x K (batch)", "calls", "ms", "GFLOP/s", "GB/s"))
total_ms = total_flops = total_bytes = 0.0
rows = []
for s in shapes:
    m, n, k, b, t, calls = s["m"], s["n"], s["k"], s["batch"], s["transpose_b"], s["calls"]
    a = rt.buffer(m * k * b, np.float16)
    w = rt.buffer(k * n * b, np.float16)
    c = rt.buffer(m * n * b, np.float32)
    a.view(np.float16)[:] = 0.01
    w.view(np.float16)[:] = 0.01
    # The GPU idles at a low clock between submits, so a single timed pass measures
    # the ramp as much as the kernel; a frame runs continuously. Warm, then best of 3.
    ms = 1e9
    for repeat in range(3):
        rt.begin()
        for _ in range(calls):
            rt.gemm(a, w, c, m, n, k, batch=b, transpose_b=t)
        started = time.perf_counter()
        rt.submit()
        elapsed = 1000 * (time.perf_counter() - started)
        if repeat:
            ms = min(ms, elapsed)
    flops = 2.0 * m * n * k * b * calls
    traffic = float(b) * calls * (m * k * 2 + k * n * 2 + m * n * 4)
    rows.append((m, n, k, b, t, calls, ms, flops, traffic))
    total_ms += ms; total_flops += flops; total_bytes += traffic
    a.free(); w.free(); c.free()

for m, n, k, b, t, calls, ms, flops, traffic in rows[:24]:
    label = f"{m} x {n} x {k}" + (f" (b={b})" if b > 1 else "") + (" T" if t else "")
    print("  %-30s %6d %9.2f %9.1f %9.1f"
          % (label, calls, ms, flops / (ms / 1000) / 1e9, traffic / (ms / 1000) / 1e9))
print("  %-30s %6s %9s %9s %9s" % ("-" * 30, "", "", "", ""))
print("  %-30s %6d %9.1f %9.1f %9.1f"
      % ("TOTAL", sum(r[5] for r in rows), total_ms,
         total_flops / (total_ms / 1000) / 1e9, total_bytes / (total_ms / 1000) / 1e9))
print("\n  %.1f GFLOP and %.2f GB of traffic in %.0f ms of GEMM per frame"
      % (total_flops / 1e9, total_bytes / 1e9, total_ms))
