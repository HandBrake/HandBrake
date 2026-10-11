#!/usr/bin/env python3
"""
bench — throughput of the Xe2 XMX cooperative-matrix GEMM.

`iters` dispatches the same work repeatedly inside a single submit, with a barrier
between passes, so host copies and submit latency are amortised and what is left is
the GPU. Numbers are honest lower bounds for this kernel: it is a plain one-tile-per-
subgroup GEMM with no shared-memory staging or K-blocking, so it is bound by global
memory traffic rather than by the matrix units.
"""
import ctypes
import sys
import time
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import xmx

xmx._load()
lib = xmx._lib
print("device: %s" % xmx.device_name())


def report_memory():
    """After the first allocation, not before: the choice is made when a buffer is made."""
    print("buffers: %s\n" % xmx.memory_note())


def gemm(M, N, K, A, B, C, iters):
    """A failed call returns in no time at all, and an unchecked one is then reported as
    impossible throughput: a B580 printed 495 TFLOP/s for a dispatch that never ran."""
    if lib.xmx_gemm(M, N, K, A.ctypes.data, B.ctypes.data, C.ctypes.data, iters) != 0:
        raise SystemExit("  %dx%d K=%d failed: xmx_gemm: %s"
                         % (M, N, K, lib.xmx_error().decode()))


def run(M, N, K, iters):
    A = np.ascontiguousarray((np.random.default_rng(0).standard_normal((M, K)) * 0.1).astype(np.float16))
    B = np.ascontiguousarray((np.random.default_rng(1).standard_normal((K, N)) * 0.1).astype(np.float16))
    C = np.empty((M, N), dtype=np.float32)
    gemm(M, N, K, A, B, C, 1)                                               # warm up
    t = time.perf_counter()
    gemm(M, N, K, A, B, C, iters)
    dt = time.perf_counter() - t
    flops = 2.0 * M * N * K * iters
    return dt, flops / dt / 1e9


run(64, 64, 64, 1)                     # one small call, so there is a buffer to report on
report_memory()
print("  %-22s %-8s %-12s %-12s %s" % ("shape", "iters", "time (s)", "GFLOP/s", "per dispatch"))
for M, N, K, it in [(512, 512, 512, 200), (1024, 1024, 1024, 100),
                    (2048, 2048, 512, 50), (4096, 1024, 1024, 40),
                    (8192, 512, 512, 40)]:
    dt, gf = run(M, N, K, it)
    print("  %-22s %-8d %-12.4f %-12.1f %.3f ms"
          % ("%dx%d K=%d" % (M, N, K), it, dt, gf, 1000 * dt / it))

print("\n  For scale: the model is %s parameters. A single 512x512 GEMM is 0.27 GFLOP."
      % format(145755123, ","))
