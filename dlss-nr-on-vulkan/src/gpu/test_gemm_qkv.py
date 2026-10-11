#!/usr/bin/env python3
"""The QKV projection's epilogue against the projection plus the three passes it replaces.

Reference: the GEMM into float32, `cosine_publish` for Q (with its head's scale) and K,
`split_heads` with the E4M3 publish for V — the graph's own path. Fused: `gemm_qkv`. The
three targets must match byte for byte, and the guard values past their ends must
survive. Both GEMM paths the epilogue lives in are exercised: the register-tiled one
(C below 128, or rows that are not whole 64-row blocks) and the staged one; and both
normalisations, the window blocks' and the ViT's (`vit`).
"""
import numpy as np
import xmxres as X

GUARD = 64
# not an E4M3 value, so no correct output can ever equal it
FILL = np.float16(-11.1)


def reference(rt, a, weight, scale, proj, q, k, v, windows, tokens, channels, heads, vit):
    rows = windows * tokens
    rt.gemm(a, weight, proj, rows, 3 * channels, channels)
    rt.cosine_publish(proj, q, windows * heads * tokens, tokens=tokens, heads=heads,
                      scale=scale, narrow=True, qkv_part=0, vit=vit)
    rt.cosine_publish(proj, k, windows * heads * tokens, tokens=tokens, heads=heads,
                      narrow=True, qkv_part=1, vit=vit)
    rt.split_heads(proj, v, windows, tokens, channels, heads, 2, epilogue=X.EPI_E4M3,
                   narrow=True)


def main():
    rt = X.Runtime()
    rng = np.random.default_rng(1729)
    cases, paths = 0, set()
    # (windows, tokens, channels): window blocks at every width, then the bottleneck's
    # single window of all tokens, tile-aligned but not a whole 64-row block — now the
    # staged kernel's partial last block, 96 tokens being a 576x352 network's — and a
    # bottleneck of 32 tokens, the 32-row staged build's whole block
    shapes = ((6, 64, 32), (3, 64, 64), (2, 64, 128), (2, 64, 256), (1, 64, 512),
              (1, 240, 1024), (1, 96, 1024), (3, 16, 256), (1, 256, 128), (1, 32, 1024))
    for windows, tokens, channels in shapes:
        heads, rows = channels // 32, windows * tokens
        paths.add("staged" if channels >= 128 and (rows % 64 == 0 or rows == 32) else "tiled")
        buffers = []

        def alloc(count, dtype):
            buf = rt.buffer(count, dtype)
            buffers.append(buf)
            return buf
        try:
            a = alloc(rows * channels, np.float16)
            weight = alloc(channels * 3 * channels, np.float16)
            scale = alloc(heads, np.float32)
            proj = alloc(rows * 3 * channels, np.float32)
            targets = [alloc(rows * channels + GUARD, np.float16) for _ in range(6)]
            X.host_write(weight, rng.normal(0, .2, channels * 3 * channels).astype(np.float16))
            X.host_write(scale, rng.uniform(.5, 12, heads).astype(np.float32))
            for spread in (.01, .3, 30.0):
                values = rng.normal(0, spread, (rows, channels)).astype(np.float16)
                # a zero row takes the norm floor; at the widest spreads the squares
                # overflow half, the reciprocal is zero and so is the row
                values[1] = 0
                values[-1] = values[-1] * np.float16(4)
                X.host_write(a, values.reshape(-1))
                for mask, vit in ((0, False), (7, False), (0, True), (7, True)):
                    rt.specialize(mask)
                    for buf in targets:
                        X.host_write(buf, np.full(rows * channels + GUARD, FILL, np.float16))
                    rt.begin()
                    reference(rt, a, weight, scale, proj, *targets[:3], windows, tokens,
                              channels, heads, vit)
                    rt.gemm_qkv(a, weight, *targets[3:], scale, rows, channels, heads, tokens,
                                vit=vit)
                    rt.submit()
                    for name, want, got in zip("QKV", targets[:3], targets[3:]):
                        want = X.host_view(want, np.float16)
                        got = X.host_view(got, np.float16)
                        if not np.array_equal(got.view(np.uint16), want.view(np.uint16)):
                            bad = np.flatnonzero(got.view(np.uint16) != want.view(np.uint16))
                            raise AssertionError(
                                f"{name} differs at {bad.size} of {got.size} "
                                f"(first {bad[0]}: {got[bad[0]]} against {want[bad[0]]}) "
                                f"windows={windows} tokens={tokens} C={channels} "
                                f"spread={spread} mask={mask} vit={vit}")
                        assert (got[rows * channels:] == FILL).all(), f"{name} guard overwritten"
                        # and every element was written, so matching is not two untouched fills
                        assert not (got[:rows * channels] == FILL).any(), f"{name} left unwritten"
                    cases += 1
            # guards on the host side of the call
            q, k, v = targets[3:]
            for args, why in (((a, weight, q, q, v, scale, rows, channels, heads, tokens), "alias"),
                              ((a, weight, a, k, v, scale, rows, channels, heads, tokens), "input"),
                              ((a, weight, q, k, v, scale, rows, channels, heads + 1, tokens), "heads"),
                              ((a, weight, q, k, v, scale, rows + 8, channels, heads, tokens), "windows")):
                try:
                    rt.gemm_qkv(*args)
                except ValueError:
                    pass
                else:
                    raise AssertionError(f"invalid QKV projection accepted: {why}")
        finally:
            for buf in buffers:
                buf.free()
    for name in ("qkv_epilogue", "fuse_partition"):
        before = rt.graph_key()
        setattr(rt, name, not getattr(rt, name))
        assert rt.graph_key() != before, name
    assert paths == {"tiled", "staged"}, paths
    gathered = window_cases(rt, rng)
    print(f"QKV epilogue: {cases} bit-exact cases on both GEMM paths, {gathered} with the "
          f"window gather, guards and graph key OK; staging={rt.staging}")


def window_cases(rt, rng):
    """The projection gathering its window rows from the image, against the partition into
    a half buffer followed by the projection — float32 and half images, both origins, and
    extents that are not whole windows, so the zero padding is exercised on every side."""
    cases = 0
    for height, width, channels in ((16, 24, 32), (20, 20, 64), (13, 21, 32), (24, 40, 128),
                                    (8, 16, 256)):
        heads = channels // 32
        for origin in ((0, 0), (-4, -4)):
            ph, pw, _ = rt.window_extent(height, width, origin, 8)
            rows = ph * pw
            buffers = []

            def alloc(n, dtype):
                b = rt.buffer(n, dtype)
                buffers.append(b)
                return b
            try:
                image32 = alloc(height * width * channels, np.float32)
                image16 = alloc(height * width * channels, np.float16)
                weight = alloc(channels * 3 * channels, np.float16)
                scale = alloc(heads, np.float32)
                win16 = alloc(rows * channels, np.float16)
                targets = [alloc(rows * channels + GUARD, np.float16) for _ in range(6)]
                values = rng.normal(0, 1.5, height * width * channels)
                values[:2] = [0.0, 70000.0]                 # a zero, and one past half's range
                X.host_write(image32, values.astype(np.float32))
                X.host_write(image16, values.astype(np.float16))
                X.host_write(weight, rng.normal(0, .2, channels * 3 * channels).astype(np.float16))
                X.host_write(scale, rng.uniform(.5, 12, heads).astype(np.float32))
                for image, half in ((image32, False), (image16, True)):
                    for mask in (0, 7):
                        rt.specialize(mask)
                        for buf in targets:
                            X.host_write(buf, np.full(rows * channels + GUARD, FILL, np.float16))
                        rt.begin()
                        rt.partition(image, win16, height, width, channels, origin=origin,
                                     narrow=True, a_half=half)
                        rt.gemm_qkv(win16, weight, *targets[:3], scale, rows, channels, heads, 64)
                        rt.gemm_qkv(image, weight, *targets[3:], scale, rows, channels, heads, 64,
                                    window=(height, width, origin), image_half=half)
                        rt.submit()
                        for name, want, got in zip("QKV", targets[:3], targets[3:]):
                            w = X.host_view(want, np.float16).view(np.uint16)
                            g = X.host_view(got, np.float16).view(np.uint16)
                            if not np.array_equal(w, g):
                                raise AssertionError(
                                    f"window {name} differs at {int((w != g).sum())} of {w.size}: "
                                    f"{height}x{width} C={channels} origin={origin} half={half} "
                                    f"mask={mask}")
                        cases += 1
            finally:
                for b in buffers:
                    b.free()
    return cases


if __name__ == "__main__":
    main()
