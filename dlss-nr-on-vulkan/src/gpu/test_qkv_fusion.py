#!/usr/bin/env python3
"""Verify direct QKV gathers, normalization and partial row workgroups."""
import pathlib
import sys
import numpy as np
import xmxres

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'ref'))
import nr_model as M


def main():
    rt = xmxres.Runtime()
    rng = np.random.default_rng(30)
    cases = 0
    for windows, tokens, heads in ((2, 64, 4), (1, 240, 32), (2, 13, 3), (1, 1, 1)):
        shape = (windows, tokens, 3, heads, 32)
        projection = rng.normal(0, 0.3, shape).astype(np.float32)
        projection.reshape(-1)[:32] = np.float32(2**-20)
        source = rt.buffer_from(projection)
        scale_values = np.linspace(0.5, 1.5, heads, dtype=np.float32)
        scale = rt.buffer_from(scale_values)
        count = windows * heads * tokens * 32
        split = rt.buffer(count, np.float16)
        for narrow in (False, True):
            dtype = np.float16 if narrow else np.float32
            out = rt.buffer(count + 32, dtype)
            for mask in (0, 7):
                rt.specialize(mask)
                for part in (0, 1):
                    scaling = scale if part == 0 else None
                    rt.begin()
                    rt.split_heads(source, split, windows, tokens, heads * 32, heads, part,
                                   epilogue=xmxres.EPI_HALF, narrow=True)
                    rt.cosine_publish(split, out, count // 32, tokens=tokens, heads=heads,
                                      scale=scaling, narrow=narrow, from_half=True)
                    rt.submit()
                    separate = np.array(xmxres.host_view(out, dtype, count=count), copy=True)
                    selected = projection[:, :, part].transpose(0, 2, 1, 3).copy()
                    expected = M.vendor_cosine_publish(selected, scale_values if part == 0 else None)
                    np.testing.assert_array_equal(separate, expected.reshape(-1).astype(dtype))
                    xmxres.host_write(out, np.full(out.nbytes // np.dtype(dtype).itemsize, -11, dtype))
                    rt.begin()
                    rt.cosine_publish(source, out, count // 32, tokens=tokens, heads=heads,
                                      scale=scaling, narrow=narrow, qkv_part=part)
                    rt.submit()
                    np.testing.assert_array_equal(xmxres.host_view(out, dtype, count=count), separate)
                    np.testing.assert_array_equal(xmxres.host_view(out, dtype)[count:], -11)
                    cases += 1
    # In a partial workgroup every lane must participate in gather/scatter,
    # even when it does not own a valid row.
    for rows, width in ((1, 32), (7, 64), (33, 32)):
        logits = rng.normal(0, 2, (rows, width)).astype(np.float32)
        source = rt.buffer_from(logits)
        for narrow in (False, True):
            dtype = np.float16 if narrow else np.float32
            out = rt.buffer(rows * width + 32, dtype)
            xmxres.host_write(out, np.full(out.nbytes // np.dtype(dtype).itemsize, -11, dtype))
            rt.begin()
            rt.softmax(source, out, rows, width, narrow=narrow)
            rt.submit()
            np.testing.assert_array_equal(xmxres.host_view(out, dtype, count=rows*width),
                                          M.vendor_approximate_softmax(logits).reshape(-1).astype(dtype))
            np.testing.assert_array_equal(
                xmxres.host_view(out, dtype)[rows*width:], -11)
    # Rows too wide to stage 32 at a time: whole rows in shared memory on the 256-lane build,
    # a partial last workgroup among them, and past 2015 columns one row a lane. A stride
    # under 62 fits more than 32 rows in the stage, but the reciprocals have room for 32:
    # the bottleneck's 16 tokens on 32 rows (a 128x128 network) took 61 a workgroup, and
    # rows 32-60 of each came out zero.
    wide = 0
    for rows, width, stride in ((100, 240, 256), (37, 96, 96), (15, 510, 512), (3, 2048, 2048),
                                (160, 16, 32), (100, 40, 48)):
        logits = rng.normal(0, 2, (rows, stride)).astype(np.float32)
        source = rt.buffer_from(logits)
        want = M.vendor_approximate_softmax(logits[:, :width])
        for narrow in (False, True):
            dtype = np.float16 if narrow else np.float32
            out = rt.buffer(rows * stride + 32, dtype)
            xmxres.host_write(out, np.full(out.nbytes // np.dtype(dtype).itemsize, -11, dtype))
            rt.begin()
            rt.softmax(source, out, rows, width, stride=stride, narrow=narrow)
            rt.submit()
            got = xmxres.host_view(out, dtype, count=rows * stride).reshape(rows, stride)
            np.testing.assert_array_equal(got[:, :width], want.astype(dtype))
            np.testing.assert_array_equal(got[:, width:], 0)
            np.testing.assert_array_equal(xmxres.host_view(out, dtype)[rows * stride:], -11)
            wide += 1
    print(f'QKV fusion: {cases} exact cases; partial cosine/softmax workgroups and {wide} '
          f'wide-row softmax cases match CPU')


if __name__ == '__main__':
    main()
