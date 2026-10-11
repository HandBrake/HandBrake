#!/usr/bin/env python3
"""
The vendor's FP8 GEMM in numpy, as OpenDLSS-NR specifies it (its `numerics.md`, `ada_fp8_fdpa16`):
each k32 step two groups of 16 products; each group aligned with the incoming f16 accumulator to
their shared exponent, every term truncated to 13 fractional bits, the terms summed exactly and the
sum rounded to half; the residual, times its cosine and rounded to half, seeding the accumulator;
split-K partitions each from zero, their sums added in half in order.

Verified: on the reference port's own captured steps (block 1's QKV projection, and its feed-forward
with the SiLU between and the residual seed) this gives the same halves, all of them — once the
port's own roundings are made unfoldable (`opendlss_captures.patch`, `opendlss_rounding.patch`):
Mesa's compiler folds WGSL's `f32(f16(x))` round trip away, and unpatched the port on an Intel GPU
does not round between the groups at all (notes/opendlss-reference.md).

Slow — an (m, 16, n) array a group — and meant for measurements, not frames.
"""
import numpy as np


def _exponent(value, floor):
    """floor(log2 |value|), at least `floor`."""
    _, exponent = np.frexp(np.abs(value).astype(np.float64))
    return np.maximum(exponent - 1, floor)


def half(value):
    return np.asarray(value, np.float64).astype(np.float16).astype(np.float64)


def fp8_gemm(a, b, seed=None, partition=0, rows=4096):
    """a (M, K) and b (K, N) E4M3 values; `seed` (M, N) half values or None -> (M, N) halves."""
    a, b = np.asarray(a, np.float64), np.asarray(b, np.float64)
    m, k = a.shape
    n = b.shape[1]
    ea = np.where(a != 0, _exponent(a, -6), -1000).astype(np.int32)
    eb = np.where(b != 0, _exponent(b, -6), -1000).astype(np.int32)
    out = np.empty((m, n), np.float64)
    width = partition or k
    for r0 in range(0, m, rows):
        ra, rea = a[r0:r0 + rows], ea[r0:r0 + rows]
        total = None
        for p0 in range(0, k, width):
            acc = (half(seed[r0:r0 + rows]) if seed is not None and p0 == 0
                   else np.zeros((ra.shape[0], n)))
            for g in range(p0, min(p0 + width, k), 16):
                pairs = rea[:, g:g + 16, None] + eb[None, g:g + 16, :]
                start = np.where(acc != 0, _exponent(acc, -14), -21)
                shared = np.maximum(start, pairs.max(axis=1))
                scale = np.ldexp(1.0, 13 - shared)
                units = np.trunc(acc * scale)
                units += np.trunc(ra[:, g:g + 16, None] * b[None, g:g + 16, :]
                                  * scale[:, None, :]).sum(axis=1)
                acc = half(units * np.ldexp(1.0, shared - 13))
            total = acc if total is None else half(total + acc)
        out[r0:r0 + rows] = total
    return out
