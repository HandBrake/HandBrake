#!/usr/bin/env python3
"""Joint Q/K/V preparation against CPU math and exact full-frame replay.

Every way the graph has of preparing Q, K and V is compared on whole frames: the QKV
epilogue (the default), the joint pass, the fused Q/K pair and the split-first
reference. The epilogue bypasses the other three, so they are run with it off.
"""
import pathlib
import sys
import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'ref'))
import nr_frame
import nr_model as M
import xmxres


def operators(rt):
    rng = np.random.default_rng(70)
    cases = 0
    for windows, tokens, heads in ((2, 64, 4), (1, 240, 32), (2, 13, 3), (1, 1, 1)):
        projection = rng.normal(0, .3, (windows, tokens, 3, heads, 32)).astype(np.float32)
        projection.reshape(-1)[:32] = np.float32(2**-20)
        scales = np.linspace(.5, 1.5, heads, dtype=np.float32)
        source = rt.buffer_from(projection)
        scale = rt.buffer_from(scales)
        count = windows * tokens * heads * 32
        outputs = [rt.buffer(count + 32, np.float16) for _ in range(3)]
        try:
            for mask in (0, 7):
                rt.specialize(mask)
                for buf in outputs:
                    xmxres.host_write(buf, np.full(count + 32, -11, np.float16))
                rt.begin()
                rt.prepare_qkv(source, *outputs, scale, windows, tokens, heads)
                rt.submit()
                for part, buf in enumerate(outputs):
                    selected = projection[:, :, part].transpose(0, 2, 1, 3).copy()
                    expected = (M.vendor_cosine_publish(selected, scales if part == 0 else None)
                                if part < 2 else M.e4m3(selected))
                    got = xmxres.host_view(buf, np.float16)
                    np.testing.assert_array_equal(got[:count], expected.reshape(-1).astype(np.float16))
                    np.testing.assert_array_equal(got[count:], -11)
                    cases += 1
        finally:
            for buf in (source, scale, *outputs):
                buf.free()
    print(f'joint QKV: {cases} CPU/GPU comparisons exact, partial workgroups and guards OK')


def frames():
    if not nr_frame.WEIGHTS.exists():
        print('joint QKV: frame skipped (no logical weights)')
        return
    backend = nr_frame.ResidentBackend()
    try:
        rt = backend.runtime
        for height, width in ((320, 320), (320, 448)):
            color = np.random.default_rng(70).random((height, width, 3), dtype=np.float32)
            features = nr_frame.make_features(color, **nr_frame.PROFILES['standard'])
            frame = backend.frame(height, width)
            expected = None
            counts = {}
            for mask in (7, 0):
                rt.specialize(mask)
                rt.fuse_qk = True
                rt.qkv_epilogue = False
                for rt.joint_qkv in (True, False, True):
                    passes = []
                    head = frame.run(features, execution='replay', submits=passes)
                    counts[rt.joint_qkv] = passes[0]
                    if expected is None:
                        expected = head.copy()
                    np.testing.assert_array_equal(head, expected)
                    for execution in ('single', 'block', 'replay'):
                        np.testing.assert_array_equal(frame.run(features, execution=execution), expected)
                assert counts[False] - counts[True] == 140, counts
                # The original separate split/normalize path must still be available.
                rt.fuse_qk = False
                np.testing.assert_array_equal(frame.run(features, execution='replay'), expected)
                rt.fuse_qk = True
                # and the epilogue replaces all three of the separate passes at 70 sites —
                # and, gathering its own window rows, the partition at the 62 window blocks
                rt.qkv_epilogue = True
                passes = []
                np.testing.assert_array_equal(
                    frame.run(features, execution='replay', submits=passes), expected)
                folded = 62 if rt.fuse_partition else 0
                # and the ten one-head blocks take all three of their attention passes as
                # one (window_block.comp), block 70's with the head in it
                folded += (20 + (1 if rt.fuse_head else 0)) if rt.fuse_window_block else 0
                assert counts[False] - passes[0] == 210 + folded, (counts, passes)
            changed = features.copy()
            changed[..., 4:7] *= np.float32(.75)
            results = []
            for rt.qkv_epilogue, rt.joint_qkv in ((False, False), (False, True), (True, False)):
                results.append(frame.run(changed, execution='replay'))
            for result in results[1:]:
                np.testing.assert_array_equal(result, results[0])
            assert not np.array_equal(results[0], expected)
            print(f'joint QKV: {width}x{height} exact, 140 fewer passes; QKV epilogue exact, '
                  f'{210 + folded} fewer; cached toggles and changed input OK')
    finally:
        backend.close()


if __name__ == '__main__':
    operators(xmxres.Runtime())
    frames()
