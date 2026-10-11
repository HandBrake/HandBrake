#!/usr/bin/env python3
"""
int8_quant — the host side of the integer path: how a float GEMM becomes an integer one.

Cooperative-matrix configuration 4 multiplies `sint8 x sint8` into an exact `sint32`
accumulator. It knows nothing about scale, so the scale is the caller's job, and the
shape of that job is what makes the integer path usable at all:

    C[m][n]  ~=  (Aq @ Bq)[m][n] * a_scale[m] * b_scale[n]

**A scale per row of A and per column of B is free**, because the accumulator is scaled
after the fact and each output element belongs to exactly one row and one column. That is
not a detail. `notes/phase23-integer-weights.md` measured the activations with a single
scale per tensor and found 180 000x of dynamic range crushed onto one grid; per row, each
token gets its own grid and the range that matters is the range *within* a token.

Weights are quantised per output channel for the same reason, once, at load — they do not
change between frames, and storing them as int8 halves 201 MB of the bottleneck's weights
to 101 MB.

What this module does **not** do is decide where to use it. That is
`notes/improve-int8-bottleneck.md`: the eight global blocks, 69.1 % of the model's
parameters, 4.8 % of the effect at 1080p — and nowhere else, where `phase23` measured the
cost as too high.

Padding is here too, because the kernel has no choice about it:
`cooperativeMatrixRobustBufferAccess` is false on this hardware, so an edge tile reads
whatever follows the buffer. Zeros pad exactly in int8 and contribute nothing to the sum.
"""
import numpy as np

TM, TN, TK = 8, 16, 32          # the only integer cooperative-matrix shape on Xe2
LIMIT = 127                     # symmetric: -128 is representable but never produced


def quantise(values, axis):
    """Symmetric int8 along `axis`, returning (int8 array, scales as float32).

    `axis=1` for an (M, K) activation gives one scale per row; `axis=0` for a (K, N)
    weight gives one scale per column. A row or column that is entirely zero keeps a
    scale of 1.0 rather than dividing by zero, and quantises to zeros, which is right.
    """
    wide = np.asarray(values, np.float32)
    if wide.ndim != 2:
        raise ValueError("quantise expects a 2-D matrix")
    if axis not in (0, 1):
        raise ValueError("axis must be 0 (per column) or 1 (per row)")
    peak = np.max(np.abs(wide), axis=axis, keepdims=True)
    scale = np.where(peak == 0, 1.0, peak / LIMIT).astype(np.float32)
    packed = np.round(wide / scale).clip(-LIMIT, LIMIT).astype(np.int8)
    return packed, np.squeeze(scale, axis=axis).astype(np.float32)


def dequantise(packed, scales, axis):
    """The inverse of `quantise`, for checking storage rather than for the GEMM path."""
    scales = np.asarray(scales, np.float32)
    return packed.astype(np.float32) * np.expand_dims(scales, axis)


def pad_for_tiles(a, b):
    """Grow A (M, K) and B (K, N) with zeros so the tile shape divides them.

    Returns (A, B, M, N) where M and N are the *original* sizes: the caller crops the
    result back, and the padded rows and columns are dropped with it.
    """
    a = np.ascontiguousarray(a)
    b = np.ascontiguousarray(b)
    m, k = a.shape
    if b.shape[0] != k:
        raise ValueError(f"inner dimensions disagree: A is {a.shape}, B is {b.shape}")
    n = b.shape[1]
    mp, np_, kp = (-m) % TM, (-n) % TN, (-k) % TK
    if mp or kp:
        a = np.pad(a, ((0, mp), (0, kp)))
    if kp or np_:
        b = np.pad(b, ((0, kp), (0, np_)))
    return a, b, m, n


def rescale(accumulator, a_scale, b_scale, rows, columns):
    """int32 accumulator back to float, and the padding cropped off."""
    out = accumulator[:rows, :columns].astype(np.float32)
    return out * np.asarray(a_scale, np.float32)[:rows, None] \
               * np.asarray(b_scale, np.float32)[None, :columns]


def prepare(a, b):
    """Everything a caller needs to hand one float GEMM to the integer kernel.

    Returns (Aq, Bq, a_scale, b_scale, rows, columns), padded and ready to dispatch.
    """
    a, b, rows, columns = pad_for_tiles(a, b)
    packed_a, a_scale = quantise(a, axis=1)
    packed_b, b_scale = quantise(b, axis=0)
    return packed_a, packed_b, a_scale, b_scale, rows, columns


def storage_bytes(weights_by_name):
    """(float bytes, int8 bytes including scales) for a set of 2-D weights."""
    wide = packed = 0
    for value in weights_by_name.values():
        value = np.asarray(value)
        if value.ndim != 2:
            continue
        wide += value.size * 4
        packed += value.size + value.shape[1] * 4           # one float scale per column
    return wide, packed
