#!/usr/bin/env python3
"""The staged GEMM's 32-row builds against the 64-row one.

A GEMM of 32 rows or fewer takes a 32-row block (libxmx.c, `small`) — with a 64-deep K
step where N is 1024 or less and K allows it — where it took a partial 64-row block
before, and so does a 64-row one with N <= 1024 and K >= 1024, two blocks of 32. Each row's sums are its own and the K steps run in the same order, so on identical
inputs the output must match byte for byte: plain and published, a transposed B, a batch,
the fused residual with a float32 and a half skip. Nothing past M rows may be written, and
the routing counter must show which build ran.
"""
import numpy as np
import xmxres as X

GUARD = 64
FILL16 = np.array([0x7E5A], np.uint16).view(np.float16)[0]
FILL32 = np.array([0x7FC0DEAD], np.uint32).view(np.float32)[0]


def fill(dtype):
    return FILL16 if np.dtype(dtype) == np.float16 else FILL32


def filled(values, dtype):
    bits = np.uint16 if np.dtype(dtype) == np.float16 else np.uint32
    return np.asarray(values).view(bits) == np.asarray(fill(dtype)).view(bits)


def main():
    rt = X.Runtime()
    # The 32-row builds are the staged kernel's; without one (portable Vulkan, MoltenVK,
    # Direct3D 12, a matrix device lent without the explicit layout) nothing takes them.
    if rt.lib.xmx_portable() or not rt.lib.xmx_window_gather():
        print("staged GEMM's 32-row builds: skipped (this device or runtime has no staged "
              "kernel) — a skip is not a pass")
        return
    rng = np.random.default_rng(3232)
    cases = 0
    # the bottleneck's four shapes at 32 tokens, its attention (a batch of 32 heads), a K
    # the deep step cannot take, and blocks with fewer than 32 rows
    for rows, cols, inner, batch in ((64, 1024, 4096, 1), (64, 1024, 1024, 1),
                                     (32, 1024, 4096, 1), (32, 4096, 1024, 1),
                                     (32, 3072, 1024, 1), (32, 1024, 1024, 1),
                                     (32, 32, 32, 32), (32, 96, 96, 1),
                                     (16, 1024, 1024, 1), (24, 64, 128, 4), (8, 32, 64, 1)):
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
                for small in (0, 1):
                    if rt.lib.xmx_staged32(small) != 0:
                        raise X.failure(rt.lib, "xmx_staged32")
                    for mask in (0, 7):
                        rt.specialize(mask)
                        out = alloc(total + GUARD, dtype)
                        X.host_write(out, np.full(total + GUARD, fill(dtype), dtype))
                        before = rt.lib.xmx_staged32_calls()
                        rt.begin()
                        if residual is not None:
                            rt.gemm_residual(a, b, residual, cosine, out, rows, cols, inner,
                                             skip_half=residual is skip16, **kw)
                        else:
                            rt.gemm(a, b, out, rows, cols, inner, batch=batch, **kw)
                        rt.submit()
                        ran = rt.lib.xmx_staged32_calls() - before
                        assert ran == small, \
                            f"{name} {rows}x{cols}x{inner}: staged32={small} but it ran {ran} times"
                        got = X.host_view(out, dtype)
                        assert filled(got[total:], dtype).all(), f"{name}: written past M rows"
                        assert not filled(got[:total], dtype).any(), f"{name}: output left unwritten"
                        outs.append(got[:total].copy())
                reference = outs[0].view(np.uint8)
                for i, values in enumerate(outs[1:], 1):
                    if not np.array_equal(values.view(np.uint8), reference):
                        differ = np.flatnonzero(values.view(np.uint8) != reference).size
                        raise AssertionError(f"{name} {rows}x{cols}x{inner} batch {batch} run {i}: "
                                             f"{differ} bytes differ from the 64-row block")
                cases += 1
        finally:
            for buf in buffers:
                buf.free()
    rt.lib.xmx_staged32(1)
    print(f"staged GEMM on 32-row blocks: {cases} cases bit-identical to the 64-row block, "
          f"nothing written past M, the 32-row build seen to run")


if __name__ == "__main__":
    main()
