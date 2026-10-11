#!/usr/bin/env python3
"""Peak GFLOP/s of a register-block shape, at a grid that saturates the machine.

`occupancy.py` shows throughput flattening at 256 workgroups — 4 subgroups on each of
64 XMX engines — and peaking at 10 % of the FP16 peak. So the units are all busy and
each is mostly idle. The question this answers is whether a block with more
multiply-accumulates per operand load raises that ceiling.

Every K step loads RM fragments of A and RN of B to feed RM*RN multiply-accumulates,
so the ratio is RM*RN/(RM+RN).
"""
import os, pathlib, sys, time
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src"))
import nr_build  # noqa: E402
RM, RN = int(sys.argv[1]), int(sys.argv[2])
BM, BN = 8 * RM, 16 * RN
os.environ["XMX_TILED_SPV"] = str(nr_build.shader(f"t_{RM}_{RN}.spv"))
os.environ["XMX_TILE_M"], os.environ["XMX_TILE_N"] = str(BM), str(BN)
os.environ["XMX_TILE_K"], os.environ["XMX_STAGE_K"] = "1", "4294967295"
sys.path.insert(0, str(ROOT / "src" / "gpu"))
import xmxres

rt = xmxres.Runtime()
K = 1024
best_rate, best_groups = 0.0, 0
for groups in (64, 128, 256, 512, 1024):
    M, N = groups * BM, BN            # one column block, `groups` rows of blocks
    a = rt.buffer(M * K, np.float16); w = rt.buffer(K * N, np.float16)
    c = rt.buffer(M * N, np.float16)
    calls = max(1, min(400, 2_000_000 // groups))
    best = 1e9
    for repeat in range(3):
        rt.begin()
        for _ in range(calls):
            rt.gemm(a, w, c, M, N, K, epilogue=xmxres.EPI_HALF, narrow=True)
        t = time.perf_counter(); rt.submit()
        if repeat:
            best = min(best, time.perf_counter() - t)
    rate = 2.0 * M * N * K / (best / calls) / 1e9
    if rate > best_rate:
        best_rate, best_groups = rate, groups
    for buf in (a, w, c): buf.free()
print("  RM=%d RN=%d  (%2dx%-3d block, %d MACs per %d loads = %.2f)  peak %7.1f GFLOP/s"
      "  %5.1f%% of peak  at %d workgroups"
      % (RM, RN, BM, BN, RM * RN, RM + RN, RM * RN / (RM + RN), best_rate,
         100 * best_rate / 32000, best_groups))
