#!/usr/bin/env python3
"""Window-output GEMM fusion versus the original projection and reverse residual."""
import numpy as np
import xmxres as X


def main():
    rt = X.Runtime()
    rng = np.random.default_rng(39)
    cases = 0
    for height, width, channels, inner in ((1, 1, 16, 16), (13, 19, 32, 32),
                                           (16, 24, 128, 128), (19, 9, 16, 32)):
        for origin in ((0, 0), (-4, -3)):
            reverse = (height, width, 8, origin)
            ph, pw, _ = rt.window_extent(height, width, origin)
            rows, pixels = ph*pw, height*width
            buffers = []
            def upload(values, dtype=np.float32):
                buf = rt.buffer_from(values, dtype)
                buffers.append(buf)
                return buf
            def allocate(count, dtype=np.float32):
                buf = rt.buffer(count, dtype)
                buffers.append(buf)
                return buf
            try:
                a = upload(rng.normal(0, .3, (rows, inner)), np.float16)
                b = upload(rng.normal(0, .3, (inner, channels)), np.float16)
                cosine = upload(rng.uniform(-1.5, 1.5, channels))
                branch = allocate(rows*channels)
                for skip_half in (False, True):
                    skip_dtype = np.float16 if skip_half else np.float32
                    values = rng.normal(0, .5, pixels*channels).astype(skip_dtype)
                    skip = upload(values, skip_dtype)
                    for narrow in (False, True):
                        dtype = np.float16 if narrow else np.float32
                        out = allocate(pixels*channels+16, dtype)
                        expected = allocate(pixels*channels+16, dtype)
                        for mask in (0, 7):
                            rt.specialize(mask)
                            for epilogue in (0, X.EPI_E4M3, X.EPI_HALF):
                                for buf in (out, expected): X.host_write(buf, np.full(pixels*channels+16, -11, dtype))
                                rt.begin()
                                rt.gemm(a, b, branch, rows, channels, inner)
                                rt.residual(branch, skip, cosine, expected, pixels*channels, channels,
                                            reverse=reverse, epilogue=epilogue,
                                            narrow=narrow, b_half=skip_half)
                                rt.gemm_residual(a, b, skip, cosine, out, rows, channels, inner,
                                                 reverse=reverse, epilogue=epilogue,
                                                 narrow=narrow, skip_half=skip_half)
                                rt.submit()
                                np.testing.assert_array_equal(X.host_view(out, dtype).view(np.uint8),
                                                              X.host_view(expected, dtype).view(np.uint8))
                                cases += 1
                            if narrow == skip_half:
                                rt.begin()
                                rt.gemm_residual(a, b, skip, cosine, skip, rows, channels, inner,
                                                 reverse=reverse, epilogue=X.EPI_HALF,
                                                 narrow=narrow, skip_half=skip_half)
                                rt.submit()
                                np.testing.assert_array_equal(
                                    X.host_view(skip, dtype).view(np.uint8),
                                    X.host_view(expected, dtype)[:pixels*channels].view(np.uint8))
                                X.host_write(skip, values)
                for invalid in ((height, width, 4, origin), (0, width, 8, origin),
                                (height, width, 8, (1, 0)), (height+64, width, 8, origin)):
                    try:
                        rt.gemm_residual(a, b, skip, cosine, out, rows, channels, inner, reverse=invalid)
                    except ValueError:
                        pass
                    else:
                        raise AssertionError('invalid window geometry accepted')
            finally:
                for buf in buffers: buf.free()
    before = rt.graph_key()
    rt.fuse_window_residual = not rt.fuse_window_residual
    assert rt.graph_key() != before
    print(f'Window residual: {cases} bit-exact cases, padding, shifts, in-place skip and guards OK')


if __name__ == '__main__':
    main()
