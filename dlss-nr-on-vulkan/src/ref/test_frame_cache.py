#!/usr/bin/env python3
"""Check extent eviction without loading weights or allocating GPU memory."""
from types import SimpleNamespace
import numpy as np
import nr_frame


class Weights:
    """Stands in for the shared weight buffers: built once, closed with the backend."""

    def __init__(self, runtime, weights):
        self.closed = False

    def close(self):
        self.closed = True


class Frame:
    def __init__(self, runtime, weights, height, width):
        self.weights = weights
        self.closed = False
        self.height, self.width = height, width

    def close(self):
        self.closed = True


def main():
    backend = object.__new__(nr_frame.ResidentBackend)
    backend._module = SimpleNamespace(ResidentFrame=Frame, DeviceWeights=Weights)
    backend.runtime = backend.weights = None
    backend._frames = {}
    backend.device_weights = None
    backend.max_cached_frames = 2
    first = backend.frame(384, 384)
    second = backend.frame(768, 1280)
    # every extent shares one upload; that is the point of them living above the frames
    assert first.weights is second.weights and not first.weights.closed
    assert backend.frame(384, 384) is first
    third = backend.frame(1088, 1920)
    assert second.closed and not first.closed and not third.closed
    assert len(backend._frames) == 2
    shared = first.weights
    backend.close()
    assert first.closed and third.closed and not backend._frames
    assert shared.closed and backend.device_weights is None, "closing releases the weights"
    backend.max_cached_frames = 2
    backend.max_cached_frames = 1
    first = backend.frame(384, 384)
    assert backend.frame(384, 384) is first
    second = backend.frame(768, 1280)
    assert first.closed and len(backend._frames) == 1
    backend.close()
    assert second.closed
    print('frame cache: reuse, LRU eviction and release OK')
    noise_cache()


def noise_cache():
    """The memoised diffusion noise must be the raw function's, and keyed on all three.

    It is rebuilt from four transcendentals a pixel and was 29-42% of feature assembly
    before caching (notes/phase48). The cached array is handed to every later frame, so
    a wrong key or a writable buffer would corrupt the model's input silently.
    """
    for height, width, index in ((7, 8, 0), (7, 8, 3), (16, 5, 0)):
        want = nr_frame._raw_noise(height, width, index)
        got = nr_frame.deterministic_noise(height, width, index)
        assert np.array_equal(got, want), (height, width, index)
        assert nr_frame.deterministic_noise(height, width, index) is got, 'not cached'
    assert not nr_frame.deterministic_noise(7, 8, 0).flags.writeable, 'must be read-only'
    assert not np.array_equal(nr_frame.deterministic_noise(7, 8, 0),
                              nr_frame.deterministic_noise(7, 8, 1)), 'frame index ignored'
    for index in range(20):                      # eviction must not leak or misfeed
        assert np.array_equal(nr_frame.deterministic_noise(4, 4, index),
                              nr_frame._raw_noise(4, 4, index)), index
    print('noise cache: same values as the raw function, keyed on extent and frame index,\n'
          '  read-only, and correct across eviction')


if __name__ == '__main__':
    main()
