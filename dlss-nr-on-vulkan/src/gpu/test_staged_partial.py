#!/usr/bin/env python3
"""The staged GEMM over a partial last 64-row block, against the kernel that took it before.

With `xmx_staged_partial` on, a GEMM whose M is not whole 64-row blocks goes to the staged
kernel: its last block's rows past M read the last real row and are never stored. Against
the tiled or 8x16 kernel on identical inputs the output must match byte for byte — plain and
published, a transposed B, a batch, the fused residual with a float32 and a half skip — and
nothing past M rows may be written. The profiler's per-kernel counts show which kernel ran.
"""
import numpy as np
import xmxres as X

GUARD = 64
FILL16 = np.array([0x7E5A], np.uint16).view(np.float16)[0]
FILL32 = np.array([0x7FC0DEAD], np.uint32).view(np.float32)[0]
PK_STAGED = 2


def fill(dtype):
    return FILL16 if np.dtype(dtype) == np.float16 else FILL32


def filled(values, dtype):
    bits = np.uint16 if np.dtype(dtype) == np.float16 else np.uint32
    return np.asarray(values).view(bits) == np.asarray(fill(dtype)).view(bits)


def staged_count(lib):
    return sum(lib.xmx_profile_count(PK_STAGED * 32 + sub) for sub in range(32))


def main():
    rt = X.Runtime()
    # No staged kernel to route to: without matrix units (MoltenVK, a phone, XMX_PORTABLE=1,
    # Direct3D 12) every runtime skips the staged shape, and a matrix device without
    # VK_KHR_workgroup_memory_explicit_layout does not build it (xmx_window_gather() is 0).
    if rt.lib.xmx_portable() or not rt.lib.xmx_window_gather():
        print("staged GEMM over a partial block: skipped (this device or runtime has no staged "
              "kernel) — a skip is not a pass")
        return
    X.profile(True)
    rng = np.random.default_rng(144400)
    cases = 0
    for rows, cols, inner, batch in ((16, 32, 128, 1), (80, 64, 256, 1), (144, 512, 512, 1),
                                     (400, 128, 256, 8), (400, 256, 256, 1), (8, 32, 128, 1)):
        buffers = []

        def alloc(n, dtype):
            b = rt.buffer(n, dtype)
            buffers.append(b)
            return b
        try:
            count = rows * cols * batch
            a = alloc(rows * inner * batch, np.float16)
            b = alloc(inner * cols * batch, np.float16)
            skip32 = alloc(rows * cols, np.float32)
            skip16 = alloc(rows * cols, np.float16)
            cosine = alloc(cols, np.float32)
            X.host_write(b, rng.normal(0, 0.3, inner * cols * batch).astype(np.float16))
            X.host_write(cosine, rng.uniform(-1.5, 1.5, cols).astype(np.float32))
            X.host_write(a, rng.normal(0, 2.0, rows * inner * batch).astype(np.float16))
            skip = rng.normal(0, 2.0, rows * cols)
            X.host_write(skip32, skip.astype(np.float32))
            X.host_write(skip16, skip.astype(np.float16))
            variants = [("plain", np.float32, {}, None),
                        ("transposed", np.float32, {"transpose_b": True}, None),
                        ("e4m3", np.float16, {"epilogue": X.EPI_E4M3, "narrow": True}, None),
                        ("gate e4m3", np.float16, {"epilogue": X.EPI_GATE_E4M3, "narrow": True}, None)]
            if batch == 1:
                variants += [("residual", np.float32, {}, skip32),
                             ("residual half skip, e4m3", np.float16,
                              {"epilogue": X.EPI_E4M3, "narrow": True}, skip16)]
            for name, dtype, kw, residual in variants:
                total = count if residual is None else rows * cols
                outs = []
                for partial in (0, 1):
                    if rt.lib.xmx_staged_partial(partial) != 0:
                        raise X.failure(rt.lib, "xmx_staged_partial")
                    for mask in (0, 7):
                        rt.specialize(mask)
                        out = alloc(total + GUARD, dtype)
                        X.host_write(out, np.full(total + GUARD, fill(dtype), dtype))
                        X.profile_reset()
                        rt.begin()
                        if residual is not None:
                            rt.gemm_residual(a, b, residual, cosine, out, rows, cols, inner,
                                             skip_half=residual is skip16, **kw)
                        else:
                            rt.gemm(a, b, out, rows, cols, inner, batch=batch, **kw)
                        rt.submit()
                        ran_staged = staged_count(rt.lib) > 0
                        assert ran_staged == bool(partial), \
                            f"{name} {rows}x{cols}x{inner}: partial={partial} but staged ran={ran_staged}"
                        got = X.host_view(out, dtype)
                        assert filled(got[total:], dtype).all(), f"{name}: written past M rows"
                        assert not filled(got[:total], dtype).any(), f"{name}: output left unwritten"
                        outs.append(got[:total].copy())
                reference = outs[0].view(np.uint8)
                for i, values in enumerate(outs[1:], 1):
                    if not np.array_equal(values.view(np.uint8), reference):
                        differ = np.flatnonzero(values.view(np.uint8) != reference).size
                        raise AssertionError(f"{name} {rows}x{cols}x{inner} batch {batch} run {i}: "
                                             f"{differ} bytes differ from the tiled kernel")
                cases += 1
        finally:
            for buf in buffers:
                buf.free()
    rt.lib.xmx_staged_partial(0)
    X.profile(False)
    print(f"staged GEMM over a partial block: {cases} cases bit-identical to the kernel that "
          f"took them before, nothing written past M, the staged kernel seen to run; "
          f"staging={rt.staging}")


if __name__ == "__main__":
    main()
