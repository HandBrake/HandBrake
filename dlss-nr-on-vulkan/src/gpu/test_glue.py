#!/usr/bin/env python3
"""The full-resolution glue in fewer passes, against the passes it replaces.

`upsample_merge` against upsample2, scale_channel, residual and e4m3_half; `upsample_add`
against a decoder transition's upsample2, scale_channel and add; `pool2_skip` against
block 0's e4m3_half and pool2; `gemm_dual` against a GEMM and an e4m3_half of its output; and
`ffn_fused_merge` against `upsample_merge` and the fused feed-forward reading both of its
outputs; `ffn_fused_stem` against `gemm_dual` and the same; and `gemm_residual_pool`
against a window residual into float32 and `pool2_skip` of it. Every output must match
byte for byte, and the guard values past them must survive.
"""
import numpy as np
import xmxres as X

GUARD = 64
# NaNs with their own payloads: no pass fed finite values produces one, so a fill that
# survives in the payload is an element nobody wrote, and one past it a guard intact
FILL16 = np.array([0x7E5A], np.uint16).view(np.float16)[0]
FILL32 = np.array([0x7FC0DEAD], np.uint32).view(np.float32)[0]


def fill(dtype):
    return FILL16 if np.dtype(dtype) == np.float16 else FILL32


def filled(values, dtype):
    """Which elements still hold the fill, compared by bits since the fill is a NaN."""
    bits = np.uint16 if np.dtype(dtype) == np.float16 else np.uint32
    return np.asarray(values).view(bits) == np.asarray(fill(dtype)).view(bits)


def check(name, got, want):
    if not np.array_equal(got.view(np.uint8), want.view(np.uint8)):
        bad = np.flatnonzero(got.reshape(-1) != want.reshape(-1))
        raise AssertionError(f"{name}: {bad.size} of {got.size} differ, first at "
                             f"{bad[0] if bad.size else '?'}")


def upsample_cases(rt, rng):
    cases = 0
    channels = 32
    for height, width in ((8, 8), (16, 24), (13, 21), (64, 40)):
        sh, sw = -(-height // 2), -(-width // 2)
        count = height * width * channels
        buffers = []

        def alloc(n, dtype):
            b = rt.buffer(n, dtype)
            buffers.append(b)
            return b
        try:
            source = alloc(sh * sw * channels, np.float16)
            skip = alloc(count, np.float16)
            sin = alloc(channels, np.float32)
            cos = alloc(channels, np.float32)
            sincos = alloc(2 * channels, np.float32)
            upsampled, merged_ref = alloc(count + GUARD, np.float32), alloc(count + GUARD, np.float32)
            half_ref = alloc(count + GUARD, np.float16)
            merged, half = alloc(count + GUARD, np.float32), alloc(count + GUARD, np.float16)
            for spread in (0.01, 1.0, 300.0):
                src = rng.normal(0, spread, sh * sw * channels).astype(np.float16)
                src[:4] = [0, -0.0, 65504, -65504]
                sk = rng.normal(0, spread, count).astype(np.float16)
                sk[:2] = [-0.0, 2 ** -24]
                s = rng.uniform(-2, 2, channels).astype(np.float32)
                c = rng.uniform(-2, 2, channels).astype(np.float32)
                s[0], c[0] = 0.0, -0.0
                X.host_write(source, src)
                X.host_write(skip, sk)
                X.host_write(sin, s)
                X.host_write(cos, c)
                X.host_write(sincos, np.concatenate([s, c]))
                for mask in (0, 7):
                    rt.specialize(mask)
                    for b, dtype in ((merged_ref, np.float32), (merged, np.float32),
                                     (half_ref, np.float16), (half, np.float16)):
                        X.host_write(b, np.full(count + GUARD, fill(dtype), dtype))
                    rt.begin()
                    rt.upsample2(source, upsampled, sw, height, width, channels, a_half=True)
                    rt.scale_channel(upsampled, sin, merged_ref, count, channels)
                    rt.residual(merged_ref, skip, cos, merged_ref, count, channels, b_half=True)
                    rt.e4m3_half(merged_ref, half_ref, count)
                    rt.upsample_merge(source, skip, sincos, merged, half, height, width, sw,
                                      channels)
                    rt.submit()
                    check(f"merged {height}x{width} spread {spread} mask {mask}",
                          X.host_view(merged), X.host_view(merged_ref))
                    check(f"half {height}x{width} spread {spread} mask {mask}",
                          X.host_view(half, np.float16), X.host_view(half_ref, np.float16))
                    assert filled(X.host_view(merged)[count:], np.float32).all()
                    assert filled(X.host_view(half, np.float16)[count:], np.float16).all()
                    assert not filled(X.host_view(merged)[:count], np.float32).any()
                    assert not filled(X.host_view(half, np.float16)[:count], np.float16).any()
                    cases += 1
        finally:
            for b in buffers:
                b.free()
    return cases


def gemm_cases(rt, rng):
    cases = 0
    for rows, cols, inner in ((1024, 32, 16), (40, 32, 16), (64, 48, 32)):
        buffers = []

        def alloc(n, dtype):
            b = rt.buffer(n, dtype)
            buffers.append(b)
            return b
        try:
            a = alloc(rows * inner, np.float16)
            b = alloc(inner * cols, np.float16)
            out_ref, out = alloc(rows * cols + GUARD, np.float32), alloc(rows * cols + GUARD, np.float32)
            half_ref, half = alloc(rows * cols + GUARD, np.float16), alloc(rows * cols + GUARD, np.float16)
            for spread in (0.01, 1.0, 60.0):
                X.host_write(a, rng.normal(0, spread, rows * inner).astype(np.float16))
                X.host_write(b, rng.normal(0, 0.5, inner * cols).astype(np.float16))
                for mask in (0, 7):
                    rt.specialize(mask)
                    for buf, dtype in ((out_ref, np.float32), (out, np.float32),
                                       (half_ref, np.float16), (half, np.float16)):
                        X.host_write(buf, np.full(rows * cols + GUARD, fill(dtype), dtype))
                    rt.begin()
                    rt.gemm(a, b, out_ref, rows, cols, inner)
                    rt.e4m3_half(out_ref, half_ref, rows * cols)
                    rt.gemm_dual(a, b, out, half, rows, cols, inner)
                    rt.submit()
                    check(f"gemm {rows}x{cols}x{inner} float32", X.host_view(out), X.host_view(out_ref))
                    check(f"gemm {rows}x{cols}x{inner} half",
                          X.host_view(half, np.float16), X.host_view(half_ref, np.float16))
                    assert filled(X.host_view(half, np.float16)[rows * cols:], np.float16).all()
                    assert not filled(X.host_view(half, np.float16)[:rows * cols], np.float16).any()
                    assert not filled(X.host_view(out)[:rows * cols], np.float32).any()
                    cases += 1
            for args in ((a, b, out, out, rows, cols, inner), (a, b, out, half, rows + 1, cols, inner)):
                try:
                    rt.gemm_dual(*args)
                except ValueError:
                    pass
                else:
                    raise AssertionError("invalid GEMM half copy accepted")
        finally:
            for buf in buffers:
                buf.free()
    return cases


def transition_cases(rt, rng):
    """`upsample_add` against the decoder transition's own three passes: upsample2 of the
    float32 projection, scale_channel of the skip, and add with the publish on its way
    out — wide and narrow, a half and a float32 skip, odd extents cropped; and block 66's
    form, the merge raw and a published copy beside it, against the add and e4m3_half."""
    cases = 0
    for channels, height, width in ((64, 16, 24), (32, 13, 21), (128, 10, 10), (512, 6, 4),
                                    (256, 9, 7)):
        sh, sw = -(-height // 2), -(-width // 2)
        count = height * width * channels
        buffers = []

        def alloc(n, dtype):
            b = rt.buffer(n, dtype)
            buffers.append(b)
            return b
        try:
            source = alloc(sh * sw * channels, np.float32)
            skip16, skip32 = alloc(count, np.float16), alloc(count, np.float32)
            sine = alloc(channels, np.float32)
            upsampled, scaled = alloc(count, np.float32), alloc(count, np.float32)
            for spread in (0.01, 1.0, 300.0):
                src = rng.normal(0, spread, sh * sw * channels).astype(np.float32)
                src[:4] = [0, -0.0, 448.0, -448.0]
                sk = rng.normal(0, spread, count).astype(np.float16)
                sk[:2] = [-0.0, 2 ** -24]
                s = rng.uniform(-2, 2, channels).astype(np.float32)
                s[0] = -0.0
                X.host_write(source, src)
                X.host_write(skip16, sk)
                X.host_write(skip32, sk.astype(np.float32) * np.float32(1.0009765625))
                X.host_write(sine, s)
                for narrow, skip_half in ((True, True), (False, True), (True, False)):
                    dtype = np.float16 if narrow else np.float32
                    skip = skip16 if skip_half else skip32
                    reference, fused = alloc(count + GUARD, dtype), alloc(count + GUARD, dtype)
                    for mask in (0, 7):
                        rt.specialize(mask)
                        for b in (reference, fused):
                            X.host_write(b, np.full(count + GUARD, fill(dtype), dtype))
                        rt.begin()
                        rt.upsample2(source, upsampled, sw, height, width, channels)
                        rt.scale_channel(skip, sine, scaled, count, channels, a_half=skip_half)
                        rt.add(upsampled, scaled, reference, count, epilogue=X.EPI_E4M3,
                               narrow=narrow)
                        rt.upsample_add(source, skip, sine, fused, height, width, sw, channels,
                                        skip_half=skip_half, epilogue=X.EPI_E4M3,
                                        narrow=narrow)
                        rt.submit()
                        name = (f"transition C={channels} {height}x{width} spread {spread} "
                                f"{'narrow' if narrow else 'wide'} "
                                f"{'half' if skip_half else 'float'} skip mask {mask}")
                        check(name, X.host_view(fused, dtype), X.host_view(reference, dtype))
                        assert filled(X.host_view(fused, dtype)[count:], dtype).all(), name
                        assert not filled(X.host_view(fused, dtype)[:count], dtype).any(), name
                        cases += 1
                raw_ref, raw = alloc(count + GUARD, np.float32), alloc(count + GUARD, np.float32)
                pub_ref, pub = alloc(count + GUARD, np.float16), alloc(count + GUARD, np.float16)
                for mask in (0, 7):
                    rt.specialize(mask)
                    for b, dtype in ((raw_ref, np.float32), (raw, np.float32),
                                     (pub_ref, np.float16), (pub, np.float16)):
                        X.host_write(b, np.full(count + GUARD, fill(dtype), dtype))
                    rt.begin()
                    rt.upsample2(source, upsampled, sw, height, width, channels)
                    rt.scale_channel(skip16, sine, scaled, count, channels, a_half=True)
                    rt.add(upsampled, scaled, raw_ref, count)
                    rt.e4m3_half(raw_ref, pub_ref, count)
                    rt.upsample_add(source, skip16, sine, raw, height, width, sw, channels,
                                    skip_half=True, published=pub)
                    rt.submit()
                    name = f"transition C={channels} {height}x{width} spread {spread} raw and published mask {mask}"
                    check(name + " (raw)", X.host_view(raw), X.host_view(raw_ref))
                    check(name + " (published)", X.host_view(pub, np.float16),
                          X.host_view(pub_ref, np.float16))
                    assert filled(X.host_view(pub, np.float16)[count:], np.float16).all(), name
                    assert not filled(X.host_view(pub, np.float16)[:count], np.float16).any(), name
                    cases += 1
        finally:
            for b in buffers:
                b.free()
    return cases


def pool_cases(rt, rng):
    """`pool2_skip` against block 0's own two passes: e4m3_half of the float32 output for
    the post block's skip, and pool2 of it with the E4M3 publish for the encoder."""
    cases = 0
    for channels, height, width in ((32, 8, 8), (32, 16, 24), (32, 64, 40), (16, 6, 10)):
        count = height * width * channels
        pooled_count = (height // 2) * (width // 2) * channels
        buffers = []

        def alloc(n, dtype):
            b = rt.buffer(n, dtype)
            buffers.append(b)
            return b
        try:
            source = alloc(count, np.float32)
            skip_ref, skip = alloc(count + GUARD, np.float16), alloc(count + GUARD, np.float16)
            pool_ref, pooled = (alloc(pooled_count + GUARD, np.float16),
                                alloc(pooled_count + GUARD, np.float16))
            for spread in (0.01, 1.0, 300.0):
                values = rng.normal(0, spread, count).astype(np.float32)
                values[:4] = [0, -0.0, 448.0, -500.0]
                X.host_write(source, values)
                for mask in (0, 7):
                    rt.specialize(mask)
                    for b, n in ((skip_ref, count), (skip, count), (pool_ref, pooled_count),
                                 (pooled, pooled_count)):
                        X.host_write(b, np.full(n + GUARD, fill(np.float16), np.float16))
                    rt.begin()
                    rt.e4m3_half(source, skip_ref, count)
                    rt.pool2(source, pool_ref, height, width, channels, epilogue=X.EPI_E4M3,
                             narrow=True)
                    rt.pool2_skip(source, pooled, skip, height, width, channels)
                    rt.submit()
                    name = f"pool+skip C={channels} {height}x{width} spread {spread} mask {mask}"
                    check(name + " skip", X.host_view(skip, np.float16),
                          X.host_view(skip_ref, np.float16))
                    check(name + " pooled", X.host_view(pooled, np.float16),
                          X.host_view(pool_ref, np.float16))
                    assert filled(X.host_view(skip, np.float16)[count:], np.float16).all()
                    assert filled(X.host_view(pooled, np.float16)[pooled_count:],
                                  np.float16).all()
                    cases += 1
        finally:
            for b in buffers:
                b.free()
    return cases


def merge_ffn_cases(rt, rng):
    """Block 70's feed-forward making its own input, against the merge stored and read."""
    cases = 0
    for height, width, extra in ((8, 8, 0), (16, 24, 0), (20, 36, 3), (64, 40, 0)):
        sw = width // 2 + extra              # the level above may be wider than half
        rows = height * width
        count = rows * 32
        buffers = []

        def alloc(n, dtype):
            b = rt.buffer(n, dtype)
            buffers.append(b)
            return b
        try:
            source = alloc((height // 2) * sw * 32, np.float16)
            skip = alloc(count, np.float16)
            sincos = alloc(64, np.float32)
            cosine = alloc(32, np.float32)
            expand = alloc(32 * 128, np.float16)
            projection = alloc(128 * 32, np.float16)
            merged, merged16 = alloc(count, np.float32), alloc(count, np.float16)
            want, got = alloc(count + GUARD, np.float32), alloc(count + GUARD, np.float32)
            for spread in (0.05, 1.0, 40.0):
                src = rng.normal(0, spread, (height // 2) * sw * 32).astype(np.float16)
                src[:2] = [0, -0.0]
                sk = rng.normal(0, spread, count).astype(np.float16)
                sk[:2] = [-0.0, 2 ** -24]
                table = rng.uniform(-2, 2, 64).astype(np.float32)
                table[0], table[32] = 0.0, -0.0
                X.host_write(source, src)
                X.host_write(skip, sk)
                X.host_write(sincos, table)
                X.host_write(cosine, rng.uniform(-1.5, 1.5, 32).astype(np.float32))
                X.host_write(expand, rng.normal(0, 0.25, 32 * 128).astype(np.float16))
                X.host_write(projection, rng.normal(0, 0.1, 128 * 32).astype(np.float16))
                for mask in (0, 7):
                    rt.specialize(mask)
                    for b in (want, got):
                        X.host_write(b, np.full(count + GUARD, FILL32, np.float32))
                    rt.begin()
                    rt.upsample_merge(source, skip, sincos, merged, merged16, height, width,
                                      sw, 32)
                    rt.ffn_fused(merged16, expand, projection, want, rows, 32, 128,
                                 skip=merged, cosine=cosine)
                    rt.ffn_fused_merge(source, skip, sincos, expand, projection, got, cosine,
                                       height, width, sw)
                    rt.submit()
                    check(f"merged feed-forward {height}x{width} spread {spread} mask {mask}",
                          X.host_view(got)[:count], X.host_view(want)[:count])
                    assert filled(X.host_view(got)[count:], np.float32).all()
                    assert not filled(X.host_view(got)[:count], np.float32).any()
                    cases += 1
        finally:
            for b in buffers:
                b.free()
    return cases


def stem_ffn_cases(rt, rng):
    """Block 0's feed-forward making its own stem, against the stem stored and read."""
    cases = 0
    for rows in (64, 384, 720, 2560):
        count = rows * 32
        buffers = []

        def alloc(n, dtype):
            b = rt.buffer(n, dtype)
            buffers.append(b)
            return b
        try:
            features, adapter = alloc(rows * 16, np.float16), alloc(16 * 32, np.float16)
            cosine = alloc(32, np.float32)
            expand = alloc(32 * 128, np.float16)
            projection = alloc(128 * 32, np.float16)
            stem, stem16 = alloc(count, np.float32), alloc(count, np.float16)
            want, got = alloc(count + GUARD, np.float32), alloc(count + GUARD, np.float32)
            for spread in (0.05, 1.0, 40.0):
                f = rng.normal(0, spread, rows * 16).astype(np.float16)
                f[:2] = [0, -0.0]
                X.host_write(features, f)
                X.host_write(adapter, rng.normal(0, 0.25, 16 * 32).astype(np.float16))
                X.host_write(cosine, rng.uniform(-1.5, 1.5, 32).astype(np.float32))
                X.host_write(expand, rng.normal(0, 0.25, 32 * 128).astype(np.float16))
                X.host_write(projection, rng.normal(0, 0.1, 128 * 32).astype(np.float16))
                for mask in (0, 7):
                    rt.specialize(mask)
                    for b in (want, got):
                        X.host_write(b, np.full(count + GUARD, FILL32, np.float32))
                    rt.begin()
                    rt.gemm_dual(features, adapter, stem, stem16, rows, 32, 16)
                    rt.ffn_fused(stem16, expand, projection, want, rows, 32, 128,
                                 skip=stem, cosine=cosine)
                    rt.ffn_fused_stem(features, adapter, expand, projection, got, cosine, rows)
                    rt.submit()
                    check(f"stem feed-forward {rows} rows spread {spread} mask {mask}",
                          X.host_view(got)[:count], X.host_view(want)[:count])
                    assert filled(X.host_view(got)[count:], np.float32).all()
                    assert not filled(X.host_view(got)[:count], np.float32).any()
                    cases += 1
        finally:
            for b in buffers:
                b.free()
    return cases


def pool_epilogue_cases(rt, rng):
    """Block 0's window residual pooling and publishing its own output, against the output
    stored as float32 and `pool2_skip` of it — whole and cropped windows, both origins."""
    cases = 0
    for height, width in ((8, 8), (16, 24), (20, 36), (40, 56)):
        for origin in ((0, 0), (-4, -4)):
            ph, pw, _ = rt.window_extent(height, width, origin)
            rows, pixels = ph * pw, height * width
            buffers = []

            def alloc(n, dtype):
                b = rt.buffer(n, dtype)
                buffers.append(b)
                return b
            try:
                attended = alloc(rows * 32, np.float16)
                weight = alloc(32 * 32, np.float16)
                skip, cosine = alloc(pixels * 32, np.float32), alloc(32, np.float32)
                raw = alloc(pixels * 32, np.float32)
                pooled_ref = alloc(pixels // 4 * 32 + GUARD, np.float16)
                pooled = alloc(pixels // 4 * 32 + GUARD, np.float16)
                published_ref = alloc(pixels * 32 + GUARD, np.float16)
                published = alloc(pixels * 32 + GUARD, np.float16)
                for spread in (0.05, 1.0, 30.0):
                    X.host_write(attended, rng.normal(0, spread, rows * 32).astype(np.float16))
                    X.host_write(weight, rng.normal(0, 0.3, 32 * 32).astype(np.float16))
                    X.host_write(skip, rng.normal(0, spread, pixels * 32).astype(np.float32))
                    X.host_write(cosine, rng.uniform(-1.5, 1.5, 32).astype(np.float32))
                    for mask in (0, 7):
                        rt.specialize(mask)
                        for b, n in ((pooled_ref, pixels // 4 * 32), (pooled, pixels // 4 * 32),
                                     (published_ref, pixels * 32), (published, pixels * 32)):
                            X.host_write(b, np.full(n + GUARD, FILL16, np.float16))
                        rt.begin()
                        rt.gemm_residual(attended, weight, skip, cosine, raw, rows, 32, 32,
                                         reverse=(height, width, 8, origin))
                        rt.pool2_skip(raw, pooled_ref, published_ref, height, width, 32)
                        rt.gemm_residual_pool(attended, weight, skip, cosine, published, pooled,
                                              rows, 32, (height, width, 8, origin))
                        rt.submit()
                        name = f"pooled residual {height}x{width} origin {origin} spread {spread} mask {mask}"
                        for got, want, n in ((pooled, pooled_ref, pixels // 4 * 32),
                                             (published, published_ref, pixels * 32)):
                            check(name, X.host_view(got, np.float16)[:n],
                                  X.host_view(want, np.float16)[:n])
                            assert filled(X.host_view(got, np.float16)[n:], np.float16).all(), name
                            assert not filled(X.host_view(got, np.float16)[:n], np.float16).any(), name
                        cases += 1
            finally:
                for b in buffers:
                    b.free()
    return cases


def main():
    rt = X.Runtime()
    rng = np.random.default_rng(424242)
    pooled_residuals = pool_epilogue_cases(rt, rng)
    merges = upsample_cases(rt, rng)
    merge_ffns = merge_ffn_cases(rt, rng)
    stem_ffns = stem_ffn_cases(rt, rng)
    transitions = transition_cases(rt, rng)
    pools = pool_cases(rt, rng)
    gemms = gemm_cases(rt, rng)
    before = rt.graph_key()
    rt.fuse_glue = not rt.fuse_glue
    assert rt.graph_key() != before
    middle = rt.graph_key()
    rt.fuse_transition = not rt.fuse_transition
    assert rt.graph_key() not in (before, middle)
    third = rt.graph_key()
    rt.fuse_merge_ffn = not rt.fuse_merge_ffn
    assert rt.graph_key() not in (before, middle, third)
    fourth = rt.graph_key()
    rt.fuse_stem_ffn = not rt.fuse_stem_ffn
    assert rt.graph_key() not in (before, middle, third, fourth)
    fifth = rt.graph_key()
    rt.fuse_pool = not rt.fuse_pool
    assert rt.graph_key() not in (before, middle, third, fourth, fifth)
    print(f"glue: {merges} upsample-merge, {merge_ffns} merged feed-forward, "
          f"{stem_ffns} stem feed-forward, {pooled_residuals} pooled residual, "
          f"{transitions} transition, {pools} pool-and-skip and {gemms} GEMM half-copy "
          f"cases bit-exact, guards and graph keys OK; staging={rt.staging}")


if __name__ == "__main__":
    main()
