#!/usr/bin/env python3
"""Exact full-frame equivalence, cold capture, resolution eviction and recovery."""
import pathlib
import sys
import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'ref'))
import nr_frame


def main():
    if not nr_frame.WEIGHTS.exists():
        print('frame execution: skipped (local logical weights missing)')
        return
    backend = nr_frame.ResidentBackend()
    rt = backend.runtime
    begin = rt.begin
    recordings = []

    def counted_begin():
        recordings.append(1)
        return begin()

    rt.begin = counted_begin
    saved = None
    previous = None
    for height, width in ((320, 320), (384, 320), (320, 320)):
        rng = np.random.default_rng(28)
        color = rng.random((height, width, 3), dtype=np.float32)
        features = nr_frame.make_features(color, **nr_frame.PROFILES['standard'])
        frame = backend.frame(*features.shape[:2])
        if previous is not None:
            assert previous._closed and not previous._graphs and not previous._buffers
        # The first-ever run must work without warming buffers in block mode.
        recordings.clear()
        head = frame.run(features, execution='replay')
        # the scratch plan's discovery, recorded and never run, then the graph itself
        assert len(recordings) == 2, len(recordings)
        recordings.clear()
        np.testing.assert_array_equal(frame.run(features, execution='replay'), head)
        assert not recordings
        recordings.clear()
        np.testing.assert_array_equal(frame.run(features, execution='block'), head)
        # Block mode submits once per block. The five skips are the level buffers
        # themselves, so there is nothing to copy — mapped or not, the same count.
        expected = 73
        assert len(recordings) == expected, (len(recordings), expected)
        recordings.clear()
        np.testing.assert_array_equal(frame.run(features, execution='single'), head)
        assert len(recordings) == 1
        # Both FFN schedules must produce the same complete frame and keep distinct
        # cached recordings. All 52 grouped blocks together remove 536 dispatches.
        counts = {}
        original_batch = rt.batch_ffn
        for mode in (False, True):
            rt.batch_ffn = mode
            passes = []
            np.testing.assert_array_equal(frame.run(features, execution='replay', submits=passes), head)
            counts[mode] = passes[0]
            recordings.clear()
            np.testing.assert_array_equal(frame.run(features, execution='replay'), head)
            assert not recordings
        assert counts[False] - counts[True] == 536, counts
        rt.batch_ffn = original_batch
        changed = features.copy()
        changed[..., 4:7] *= np.float32(0.75)
        changed_head = frame.run(changed, execution='block')
        assert not np.array_equal(head, changed_head)
        np.testing.assert_array_equal(frame.run(changed, execution='replay'), changed_head)
        # A graph captured for generic shaders must coexist with specialized ones.
        rt.specialize(0)
        np.testing.assert_array_equal(frame.run(features, execution='replay'), head)
        rt.specialize(7)
        np.testing.assert_array_equal(frame.run(features, execution='replay'), head)
        if height == width == 320:
            if saved is None:
                saved = head
            else:
                np.testing.assert_array_equal(head, saved)
        previous = frame
    # A recording failure must not leave the singleton runtime stuck recording.
    record_block = backend._module.R.record_block

    def fail(*args, **kwargs):
        raise ValueError('injected recording failure')

    backend._module.R.record_block = fail
    try:
        try:
            frame.run(features, execution='single')
        except ValueError as error:
            assert str(error) == 'injected recording failure'
        else:
            raise AssertionError('expected recording failure')
    finally:
        backend._module.R.record_block = record_block
    np.testing.assert_array_equal(frame.run(features, execution='replay'), saved)
    backend.close()
    try:
        frame.run(features)
    except RuntimeError as error:
        assert 'closed' in str(error)
    else:
        raise AssertionError('closed frame accepted')
    print('frame execution: exact modes, 73 -> 1 submissions, cold capture, eviction, recovery OK')


if __name__ == '__main__':
    main()
