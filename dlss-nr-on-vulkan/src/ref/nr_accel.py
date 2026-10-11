#!/usr/bin/env python3
"""
nr_accel — optional torch-backed rounding for the CPU reference.

The graph spends most of its time not in matrix multiplies but in the vendor's
rounding points, and numpy has no SIMD path for either of them: the half round
trip runs at 0.17 Gelem/s where a float32 copy does 2.94, and our E4M3 is eight
integer passes. torch has hardware conversions for both — including a native
`float8_e4m3fn` — and is 13x faster on the first.

Nothing here may change a single bit. `verify()` checks that against every
representable value, and `install()` refuses to proceed unless it passes.

    import nr_accel; nr_accel.install()
"""
from __future__ import annotations

import numpy as np

try:
    import torch
except ImportError:  # pragma: no cover - the reference must not need it
    torch = None

import nr_model


def available():
    return torch is not None


# torch parallelises every elementwise op across its thread pool. The graph issues
# thousands of small rounding calls a frame, so that pool is synchronised thousands of
# times, and it also contends with whatever threads the BLAS is holding. One thread is
# measurably better here; `install(threads=...)` overrides.
THREADS = 1


def half_round(value):
    single = np.ascontiguousarray(np.asarray(value, dtype=np.float32))
    return torch.from_numpy(single).to(torch.float16).to(torch.float32).numpy()


def e4m3_round(value):
    single = np.ascontiguousarray(np.asarray(value, dtype=np.float32))
    tensor = torch.from_numpy(single).clamp(-448, 448)
    return tensor.to(torch.float8_e4m3fn).to(torch.float32).numpy()


def _every_half():
    """All 65536 float16 bit patterns widened to float32, non-finite dropped."""
    values = np.arange(65536, dtype=np.uint16).view(np.float16).astype(np.float32)
    return values[np.isfinite(values)]


def _every_e4m3():
    values = []
    for bits in range(256):
        sign = -1 if bits >> 7 else 1
        exponent, mantissa = (bits >> 3) & 0xF, bits & 7
        if exponent == 0xF and mantissa == 7:
            continue
        values.append(sign * (2.0 ** -6) * (mantissa / 8.0) if exponent == 0
                      else sign * (2.0 ** (exponent - 7)) * (1 + mantissa / 8.0))
    return np.array(sorted(set(values)), dtype=np.float32)


def _midpoints(values):
    order = np.sort(values[values >= 0]).astype(np.float64)
    mid = ((order[:-1] + order[1:]) / 2).astype(np.float32)
    return np.concatenate([mid, -mid])


def verify(verbose=False):
    """-> (ok, report). Every representable value, every tie, and six magnitude regimes."""
    if torch is None:
        return False, ["torch is not installed"]
    rng = np.random.default_rng(0)
    report, ok = [], True
    regimes = {
        "normal": rng.standard_normal(1 << 20).astype(np.float32) * 40,
        "tiny": rng.standard_normal(1 << 20).astype(np.float32) * 1e-6,
        "subnormal": rng.uniform(-6.1e-5, 6.1e-5, 1 << 20).astype(np.float32),
        "huge": rng.standard_normal(1 << 18).astype(np.float32) * 1e5,
        "swept": np.linspace(-70000, 70000, 1 << 20, dtype=np.float32),
        "log-swept": (np.exp(np.linspace(-30, 12, 1 << 20)).astype(np.float32)
                      * rng.choice([-1, 1], 1 << 20)),
    }
    for name, reference, fast, extra in (
            ("half", nr_model._half_rounded, half_round,
             {"every value": _every_half(), "ties": _midpoints(_every_half())}),
            ("e4m3", nr_model.e4m3, e4m3_round,
             {"every value": _every_e4m3(), "ties": _midpoints(_every_e4m3())})):
        for label, values in {**extra, **regimes}.items():
            mine, theirs = reference(values), fast(values)
            same = np.array_equal(mine, theirs, equal_nan=True)
            ok &= same
            report.append(f"  [{'ok  ' if same else 'FAIL'}] {name} {label}"
                          f" ({values.size} values)")
    if verbose:
        print("\n".join(report))
    return ok, report


def install(check=True, threads=None):
    """Point `nr_model` at the torch rounding, after proving it changes nothing."""
    if torch is None:
        return False
    torch.set_num_threads(THREADS if threads is None else threads)
    if check:
        ok, report = verify()
        if not ok:
            raise RuntimeError("torch rounding is not bit-identical:\n"
                               + "\n".join(report))
    nr_model.HALF_ROUND = half_round
    nr_model.E4M3_ROUND = e4m3_round
    return True


def uninstall():
    nr_model.HALF_ROUND = None
    nr_model.E4M3_ROUND = None


if __name__ == "__main__":
    import sys
    ok, report = verify(verbose=True)
    print("\nbit-identical" if ok else "\nDIFFERENT")
    sys.exit(0 if ok else 1)
