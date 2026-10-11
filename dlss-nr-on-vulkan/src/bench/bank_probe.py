#!/usr/bin/env python3
"""What a 32-way shared-memory bank conflict costs on Xe2.

`attention.comp` stages a workgroup's rows in `shared float stage[32 * 64]` and gives
each of the subgroup's 32 lanes one row. Lane `l` then reads `stage[l * 64 + i]`, whose
bank is `(l * 64 + i) mod 32 = i mod 32` — the same bank for all 32 lanes, on every
access. Padding the row stride to 65 makes the bank `(l + i) mod 32`, all distinct.

Softmax and cosine publish together are 142 ms of a 488 ms frame and both run at under
half the machine's bandwidth (`notes/phase45`). This measures whether the conflict is
enough to explain that, before the real shaders are touched.
"""
import os
import pathlib
import sys
import time

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src"))
import nr_build  # noqa: E402
os.environ["XMX_UNARY_SPV"] = str(nr_build.shader("bank_probe.spv"))
sys.path.insert(0, str(ROOT / "src" / "gpu"))
import xmxres

GROUPS, ROUNDS = 4096, 64

runtime = xmxres.Runtime()
source = runtime.buffer(GROUPS)
target = runtime.buffer(GROUPS)
print("  %-24s %10s %10s" % ("row stride", "ms", "relative"))
timings = {}
for stride in (64, 65):
    best = float("inf")
    for _ in range(5):
        runtime.begin()
        # the unary dispatch is (count + 255) / 256 workgroups of this shader's 32 lanes
        # `_pad` is the last push-constant slot; the probe reads its repeat count there
        # rather than adding a parameter to the production call for a benchmark's sake.
        runtime.unary(0, source, target, GROUPS * 256, channels=stride, _pad=ROUNDS)
        started = time.perf_counter()
        runtime.submit()
        best = min(best, time.perf_counter() - started)
    timings[stride] = best * 1e3
for stride in (64, 65):
    print("  %-24s %10.2f %9.2fx"
          % ("%d%s" % (stride, "  (conflicting)" if stride == 64 else "  (padded)"),
             timings[stride], timings[stride] / timings[65]))
print("\n  padding is worth %.2fx here" % (timings[64] / timings[65]))
