#!/usr/bin/env python3
"""Exercise transfer barriers, command lifetime, replay inputs and invalid copies."""
import numpy as np
import xmxres


def main():
    rt = xmxres.Runtime()
    source = rt.buffer_from(np.arange(64, dtype=np.float32))
    temporary = rt.buffer(64)
    copied = rt.buffer(80).zero()
    output = rt.buffer(80)
    rt.begin()
    rt.scale(source, temporary, 64, 2)
    rt.copy(temporary, copied, 64*4, target_offset=8*4)
    rt.scale(copied, output, 80, 3)
    graph = rt.capture()
    for offset in (0, 11, -4):
        xmxres.host_write(source, np.arange(64, dtype=np.float32) + offset)
        assert graph.run() == 3
        expected = np.zeros(80, dtype=np.float32)
        expected[8:72] = (np.arange(64, dtype=np.float32) + offset)*6
        np.testing.assert_array_equal(xmxres.host_view(output, count=80), expected)
        # Ordinary command buffer reuse must leave captured commands intact.
        rt.begin()
        rt.scale(source, temporary, 64, -1)
        rt.submit()
    rt.begin()
    for size, a_offset, b_offset in ((0, 0, 0), (1, 0, 0), (4, 1, 0),
                                    (4, 0, 1), (260, 0, 0), (4, 256, 0)):
        try:
            rt.copy(source, copied, size, a_offset, b_offset)
        except RuntimeError:
            pass
        else:
            raise AssertionError('invalid copy accepted')
    try:
        rt.copy(source, source, 32, 0, 16)
    except RuntimeError:
        pass
    else:
        raise AssertionError('overlapping copy accepted')
    rt.abort()
    graph.run()
    graph.free()
    try:
        graph.run()
    except RuntimeError:
        pass
    else:
        raise AssertionError('freed graph replayed')
    # More captures than the native slot limit prove slots are released.
    for _ in range(140):
        rt.begin()
        rt.copy(source, temporary, source.nbytes)
        graph = rt.capture()
        graph.run()
        graph.free()
    np.testing.assert_array_equal(xmxres.host_view(source, count=64),
                                  xmxres.host_view(temporary, count=64))
    print('graph: transfer/compute barriers, changed input, lifetime, validation OK')


if __name__ == '__main__':
    main()
