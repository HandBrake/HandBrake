#!/usr/bin/env python3
"""
nr_xmx — run the recovered graph's GEMMs on the Xe2 XMX units.

`nr_model` funnels every matrix multiply through one hook, `nr_model.MATMUL`.
This module points that hook at `xmx.gemm`, which is FP16 x FP16 -> FP32 on the
cooperative-matrix path (config 1 of notes/hw-coopmat.md).

The per-head score `Q @ K^T` and context `P @ V` are genuinely batched — a
different B per head and per window — and go through the batched pipeline
instead, one dispatch for the whole batch. Their shapes are already tile-aligned
(window tokens 64, head_dim 32), and the key is consumed in the layout it already
has: a cooperative-matrix load reads it column-major, so no transpose is copied.

Right-hand operands are the model's weights, so their FP16 conversion, power-of-two
rescale and tile padding are cached per tensor and done once.

    import nr_xmx; nr_xmx.install()
"""
from __future__ import annotations

import os
import pathlib
import sys
import time

import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "ref"))

import xmx  # noqa: E402

# Below this many multiply-accumulates the ~0.4 ms fixed dispatch cost exceeds what
# numpy takes, even at the ~3 GFLOP/s of the reference BLAS this machine ships.
MIN_MACS = 1 << 20

# Arithmetic intensity, K*N/(K+N): multiply-accumulates per element moved across the
# host boundary. The cooperative-matrix kernel itself beats any CPU here — 0.17 to
# 1.4 ms on this graph's shapes — but a dispatch also writes A as float16 and reads C
# back as float32 from the host, which costs 3 to 45 ms. Wide, shallow GEMMs (block 0's
# 147456x32x128 is 1.2 GFLOP against a 75 MB result) are therefore host-bound and
# belong on the CPU; deep, narrow ones (576x512x1536) are 2.8x faster on the GPU.
# Measured break-even against OpenBLAS sits between 192 and 384; against the netlib
# reference build there is none, because that CPU is 20x slower and the GPU wins on
# every shape. So the right threshold depends on the BLAS numpy was linked against and
# `calibrate()` picks it at install time. `NR_MIN_INTENSITY` overrides.
# See notes/phase13-blas-baseline.md.
MIN_INTENSITY = None


def calibrate():
    """Set `MIN_INTENSITY` from how fast this numpy's BLAS actually is.

    One 512x512x512 sgemm, a few milliseconds. A netlib reference build lands around
    3 GFLOP/s and wants everything on the GPU; an OpenBLAS build lands two orders
    higher and should keep the wide, shallow GEMMs, where a dispatch is host-bound.
    """
    global MIN_INTENSITY
    override = os.environ.get("NR_MIN_INTENSITY")
    if override is not None:
        MIN_INTENSITY = int(override)
        return MIN_INTENSITY
    size = 512
    left = np.random.default_rng(0).standard_normal((size, size)).astype(np.float32)
    right = np.ascontiguousarray(left.T)
    left @ right
    started = time.perf_counter()
    left @ right
    rate = 2 * size ** 3 / max(time.perf_counter() - started, 1e-9) / 1e9
    MIN_INTENSITY = 0 if rate < 20.0 else 256
    return MIN_INTENSITY

# When True, activations that float16 cannot hold exactly are carried as a pair of
# halves, two dispatches instead of one. 97.3 % of this graph's GEMM activations are
# already half-valued — they arrive from an E4M3 or a half publish — so the cost lands
# on a small minority of calls and the result tracks the float32 reference.
EXACT = False

_prepared: dict[tuple, tuple] = {}
STATS = {"gpu": 0, "gpu_macs": 0, "gpu_seconds": 0.0,
         "cpu": 0, "cpu_macs": 0, "cpu_seconds": 0.0, "split": 0, "bytes": 0}


def _moved(rows, inner, cols):
    """Bytes crossing the host/device boundary for one dispatch.

    The operands live in shared memory, but the activation is still written as
    float16 on the way in and the result read as float32 on the way out, once per
    GEMM, because the graph between them runs in numpy. This is the number a full
    compute-shader port would delete, not any single kernel's time.
    """
    return rows * inner * 2 + rows * cols * 4


def _operand(weight):
    """Cache the converted, rescaled, padded right-hand operand per tensor.

    Keyed by data pointer plus shape and strides, not by object identity: the
    branched feed-forward indexes `expansion_weight[head, branch, input]`, which
    builds a fresh view object on every access but always over the same bytes.
    The key doubles as the upload key, so a weight reused across chunks of one
    block is written into the device buffer once.
    """
    key = (weight.ctypes.data, weight.shape, weight.strides)
    hit = _prepared.get(key)
    if hit is None:
        hit = (weight, xmx.prepare_b(weight))
        _prepared[key] = hit
    return key, hit[1]


def _batched(a, b, transpose_b):
    """Route a rank-4 batched matmul, or return None to leave it on the CPU."""
    if a.ndim != 4 or b.ndim != 4 or a.shape[:2] != b.shape[:2]:
        return None
    if MIN_INTENSITY is None:
        calibrate()          # matmul_nt reaches here without going through matmul()
    batch = a.shape[0] * a.shape[1]
    rows, inner = a.shape[2], a.shape[3]
    cols = b.shape[2] if transpose_b else b.shape[3]
    if (batch * rows * inner * cols < MIN_MACS
            or inner * cols < MIN_INTENSITY * (inner + cols)):
        return None
    started = time.perf_counter()
    try:
        out = xmx.bmm_aligned(np.ascontiguousarray(a).reshape(batch, rows, inner),
                              np.ascontiguousarray(b).reshape(batch, *b.shape[2:]),
                              transpose_b=transpose_b)
    except ValueError:
        return None
    STATS["gpu"] += 1
    STATS["gpu_macs"] += batch * rows * inner * cols
    STATS["bytes"] += batch * (_moved(rows, inner, cols) + cols * inner * 2)
    STATS["gpu_seconds"] += time.perf_counter() - started
    return out.reshape(a.shape[0], a.shape[1], rows, cols)


def matmul_nt(a, b):
    """a @ b^T over the last two axes."""
    a = np.asarray(a, dtype=np.float32)
    b = np.asarray(b, dtype=np.float32)
    out = _batched(a, b, True)
    if out is not None:
        return out
    started = time.perf_counter()
    out = a @ b.swapaxes(-1, -2)
    STATS["cpu"] += 1
    STATS["cpu_seconds"] += time.perf_counter() - started
    return out


def matmul(a, b):
    if MIN_INTENSITY is None:
        calibrate()
    a = np.asarray(a, dtype=np.float32)
    b = np.asarray(b, dtype=np.float32)
    if b.ndim == 2 and a.ndim >= 2 and a.shape[-1] == b.shape[0]:
        rows = int(np.prod(a.shape[:-1], dtype=np.int64))
        inner, cols = b.shape
        if (rows * inner * cols >= MIN_MACS
                and inner * cols >= MIN_INTENSITY * (inner + cols)):
            started = time.perf_counter()
            flat = np.ascontiguousarray(a.reshape(rows, inner))
            key, operand = _operand(b)
            if EXACT and not xmx.is_half_valued(flat):
                STATS["split"] += 1
                out = xmx.gemm_split(flat, operand, b_key=key)
            else:
                out = xmx.gemm_mapped(flat, operand, b_key=key)
            STATS["gpu"] += 1
            STATS["gpu_macs"] += rows * inner * cols
            STATS["bytes"] += _moved(rows, inner, cols)
            STATS["gpu_seconds"] += time.perf_counter() - started
            return out.reshape(*a.shape[:-1], cols)
    routed = _batched(a, b, False)
    if routed is not None:
        return routed
    started = time.perf_counter()
    out = a @ b
    STATS["cpu"] += 1
    STATS["cpu_macs"] += int(np.prod(a.shape[:-1], dtype=np.int64)) * int(
        np.prod(b.shape[-2:], dtype=np.int64))
    STATS["cpu_seconds"] += time.perf_counter() - started
    return out


def install(fuse_branched=True, exact=False):
    """Point the graph's GEMM hooks at the XMX path.

    `exact` carries activations float16 cannot hold as a pair of halves, which tracks
    the float32 CPU reference instead of the vendor's own half precision; it also turns
    the branched fold off, since that reassociates float32 sums.

    `fuse_branched` folds the branched feed-forward's 4*G^2 + 4*G small GEMMs into
    1 + G + 1 larger ones. The fold is exact linear algebra (measured 8e-07, plain
    float32 reassociation), but the E4M3 publish between the two stages amplifies
    that to ~1e-02 on the block output — the same chaos every other perturbation
    hits, see notes/phase8-xmx-graph.md. Off in the reference, on here, because a
    dispatch costs far more than the arithmetic it carries.
    """
    global EXACT
    import nr_model
    EXACT = exact
    if MIN_INTENSITY is None:
        calibrate()
    nr_model.MATMUL = matmul
    nr_model.MATMUL_NT = matmul_nt
    nr_model.FUSE_BRANCHED = fuse_branched and not exact
    return xmx.device_name()


def uninstall():
    import nr_model
    nr_model.MATMUL = None
    nr_model.MATMUL_NT = None


def reset_stats():
    for key in STATS:
        STATS[key] = 0 if isinstance(STATS[key], int) else 0.0


def report():
    lines = []
    for where in ("gpu", "cpu"):
        calls, macs, seconds = (STATS[where], STATS[f"{where}_macs"],
                                STATS[f"{where}_seconds"])
        rate = 2 * macs / seconds / 1e9 if seconds > 0 else 0.0
        lines.append(f"  {where.upper()}  {calls:6d} calls  {2 * macs / 1e9:8.1f} GFLOP  "
                     f"{seconds:7.2f} s  {rate:7.1f} GFLOP/s")
    lines.append(f"  host<->device {STATS['bytes'] / 1e9:.2f} GB in {STATS['gpu']} round trips"
                 f"   (min intensity {MIN_INTENSITY})")
    return "\n".join(lines)
