#!/usr/bin/env python3
"""Fused projection/residual versus two passes on identical GPU inputs."""
import numpy as np
import xmxres as X


def main():
    rt = X.Runtime()
    rng = np.random.default_rng(38)
    cases = 0
    for rows, cols, inner in ((8, 16, 16), (32, 64, 32), (128, 256, 128)):
        for skip_half in (False, True):
            buffers = []
            def upload(value, dtype=np.float32):
                buf = rt.buffer_from(value, dtype)
                buffers.append(buf)
                return buf
            def allocate(count, dtype=np.float32):
                buf = rt.buffer(count, dtype)
                buffers.append(buf)
                return buf
            try:
                a = upload(rng.normal(0, .3, (rows, inner)), np.float16)
                b = upload(rng.normal(0, .3, (inner, cols)), np.float16)
                skip_dtype = np.float16 if skip_half else np.float32
                values = rng.normal(0, .5, rows*cols).astype(skip_dtype)
                skip = upload(values, skip_dtype)
                cosine = upload(rng.uniform(-1.5, 1.5, cols))
                branch = allocate(rows*cols)
                for mask in (0, 7):
                    rt.specialize(mask)
                    for narrow in (False, True):
                        dtype = np.float16 if narrow else np.float32
                        out = allocate(rows*cols+16, dtype)
                        reference = allocate(rows*cols+16, dtype)
                        for epilogue in (0, X.EPI_E4M3, X.EPI_GATE, X.EPI_GATE_E4M3, X.EPI_HALF):
                            for buf in (out, reference): X.host_write(buf, np.full(rows*cols+16, -11, dtype))
                            rt.begin()
                            rt.gemm(a, b, branch, rows, cols, inner)
                            rt.residual(branch, skip, cosine, reference, rows*cols, cols,
                                        epilogue=epilogue, narrow=narrow, b_half=skip_half)
                            rt.gemm_residual(a, b, skip, cosine, out, rows, cols, inner,
                                             epilogue=epilogue, narrow=narrow, skip_half=skip_half)
                            rt.submit()
                            np.testing.assert_array_equal(X.host_view(out, dtype).view(np.uint8),
                                                          X.host_view(reference, dtype).view(np.uint8))
                            cases += 1
                        if narrow == skip_half:
                            rt.begin()
                            rt.gemm_residual(a, b, skip, cosine, skip, rows, cols, inner,
                                             epilogue=X.EPI_HALF, narrow=narrow, skip_half=skip_half)
                            rt.submit()
                            np.testing.assert_array_equal(
                                X.host_view(skip, dtype).view(np.uint8),
                                X.host_view(reference, dtype)[:rows*cols].view(np.uint8))
                            X.host_write(skip, values)
                for target, width, expected in ((a, cols, 'alias'),
                                                (cosine, cols, 'alias'),
                                                (branch, cols+1, 'multiple')):
                    try:
                        rt.gemm_residual(a, b, skip, cosine, target, rows, width, inner)
                    except ValueError as error:
                        assert expected in str(error), error
                    else:
                        raise AssertionError('invalid residual accepted')
                small = allocate(1)
                for kwargs in ({'target': small},
                               {'target': skip, 'narrow': not skip_half}):
                    try:
                        rt.gemm_residual(a, b, skip, cosine, rows=rows, cols=cols,
                                         inner=inner, skip_half=skip_half, **kwargs)
                    except ValueError:
                        pass
                    else:
                        raise AssertionError('invalid residual storage accepted')
            finally:
                for buf in buffers: buf.free()
    before = rt.graph_key()
    rt.fuse_residual = not rt.fuse_residual
    assert rt.graph_key() != before
    print(f'GEMM residual: {cases} bit-exact cases, in-place skip, guards and graph key OK')


if __name__ == '__main__':
    main()
