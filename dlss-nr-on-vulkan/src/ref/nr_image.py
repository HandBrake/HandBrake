"""Optional native CPU image passes; NumPy remains the reference and fallback.

Once the network runs on a small enough frame it stops being the frame, and what is left
is a stack of full-frame passes over the *output* resolution — feature assembly, the two
resizes, the composition, the codec — none of which shrinks with the render scale. This
is those passes in C.

Written and measured in the parallel ProjectsCodex tree (`notes/phase57`); the two
functions this tree needs and that one does not — history in the feature channels and the
temporal composition — are added here. Every function is a transcription of the NumPy
beside it, and the output is required to be byte-identical: `src/ref/test_native_image.py`
runs both and compares.

`make` and CMake build the library for `x86-64-v3` on x86-64 (`NR_IMAGE_ARCH`), whose F16C
turns each half conversion into one instruction rather than a call into libgcc, and for the
plain architecture elsewhere (arm64 converts through `_Float16` in hardware regardless).
Every pass is split by rows across the library's own thread pool, one thread
per core by default (`NR_HOST_THREADS`, else `OMP_NUM_THREADS`, to change it; read once,
when the library first runs a pass); a row's arithmetic does not depend on which thread
does it, so the output is the same bytes at any thread count. The pool waits passively,
as upstream's OpenMP build does with `OMP_WAIT_POLICY=passive`.
`NR_HOST_NATIVE=0` selects the NumPy path for a paired measurement without
changing any model or shader setting. With no library at all everything still runs.
"""
import ctypes as C
from functools import lru_cache
import os
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'src'))
import nr_build  # noqa: E402


@lru_cache(maxsize=1)
def _library():
    try:
        lib = C.CDLL(str(nr_build.library('nr_image')))
    except OSError:
        return None
    ptr, stride, size = C.c_void_p, C.c_ssize_t, C.c_size_t
    lib.nr_features.argtypes = [ptr, stride, stride, stride, ptr, stride, stride, stride,
                                ptr, ptr, size, size, ptr, ptr, ptr]
    lib.nr_features.restype = None
    lib.nr_features_half.argtypes = lib.nr_features.argtypes
    lib.nr_features_half.restype = None
    lib.nr_to_half.argtypes = [ptr, size, ptr]
    lib.nr_to_half.restype = None
    lib.nr_compose_temporal.argtypes = [
        ptr, stride, stride, stride, ptr, stride, stride, stride,
        ptr, stride, stride, stride, ptr, stride, stride, stride,
        ptr, stride, stride, ptr, C.c_float, ptr, stride, stride,
        size, size, C.c_float, C.c_float, C.c_float, C.c_float, C.c_float, ptr, ptr, ptr]
    lib.nr_compose_temporal.restype = None
    lib.nr_compose_encode.argtypes = [
        ptr, stride, stride, stride, size, size, ptr, ptr, ptr, ptr, ptr, ptr,
        ptr, stride, stride, stride, ptr, stride, stride, stride,
        ptr, stride, stride, stride, ptr, C.c_float, ptr, stride, stride,
        size, size, C.c_float, C.c_float, C.c_float, C.c_float, C.c_float,
        ptr, ptr, ptr, ptr, size, size, size, C.c_int, ptr, size]
    lib.nr_compose_encode.restype = None
    lib.nr_resize_axis.argtypes = [ptr, stride, stride, stride, size, size, size,
                                  C.c_int, ptr, ptr, ptr, ptr]
    lib.nr_resize_axis.restype = None
    lib.nr_area_mean.argtypes = [ptr, stride, stride, stride, size, size, size, size, size,
                                 ptr]
    lib.nr_area_mean.restype = None
    lib.nr_compose.argtypes = [ptr, stride, stride, stride, ptr, stride, stride, stride,
                              size, size, C.c_float, ptr, ptr, ptr]
    lib.nr_compose.restype = None
    lib.nr_decode8.argtypes = [ptr, size, C.c_int, ptr]
    lib.nr_decode8.restype = None
    lib.nr_encode8.argtypes = [ptr, stride, stride, stride, ptr, size, size, C.c_int, ptr]
    lib.nr_encode8.restype = None
    return lib


def library():
    return None if os.environ.get('NR_HOST_NATIVE') == '0' else _library()


def _strides(array):
    return tuple(value // array.itemsize for value in array.strides)


def decode8(raw, width, height, bgra):
    lib = library()
    if lib is None:
        return None
    pixels = np.frombuffer(raw, np.uint8).reshape(height, width, 4)
    output = np.empty((height, width, 3), np.float32)
    lib.nr_decode8(pixels.ctypes.data, height * width, bgra, output.ctypes.data)
    return output


def encode8(image, raw, bgra):
    lib = library()
    if lib is None:
        return None
    image = np.require(image, dtype=np.float32, requirements=['A'])
    if image.ndim != 3 or image.shape[2] != 3:
        raise ValueError('encode expects RGB colour')
    height, width = image.shape[:2]
    pixels = np.frombuffer(raw, np.uint8).reshape(height, width, 4)
    output = np.empty((height, width, 4), np.uint8)
    lib.nr_encode8(image.ctypes.data, *_strides(image), pixels.ctypes.data,
                   height, width, bgra, output.ctypes.data)
    return output.tobytes()


def _grade(grade):
    """`nr_frame.grade_for`'s three factors as the C reads them, or None."""
    return None if grade is None else np.array(grade, np.float32)


def _neural(neural, shape):
    """Where the composition keeps the vendor's history: `neural` itself, which must be
    C-contiguous float32 of the colour's shape, or None."""
    if neural is not None and (not isinstance(neural, np.ndarray) or neural.dtype != np.float32
                               or neural.shape != shape or not neural.flags.c_contiguous
                               or not neural.flags.writeable):
        raise ValueError('neural must be writable C-contiguous float32 of the colour\'s shape')
    return neural


def compose(head, colour, intensity, *, grade=None, neural=None):
    """`nr_frame.compose` of a still frame without a mask; `grade` and `neural` are its."""
    lib = library()
    if lib is None:
        return None
    head = np.require(head, dtype=np.float32, requirements=['A'])
    colour = np.require(colour, dtype=np.float32, requirements=['A'])
    if (head.ndim != 3 or colour.ndim != 3 or head.shape[:2] != colour.shape[:2]
            or head.shape[2] < 3 or colour.shape[2] != 3):
        raise ValueError('head and colour must share height and width')
    grade, neural = _grade(grade), _neural(neural, colour.shape)
    output = np.empty(colour.shape, np.float32)
    lib.nr_compose(head.ctypes.data, *_strides(head), colour.ctypes.data, *_strides(colour),
                   *colour.shape[:2], intensity,
                   grade.ctypes.data if grade is not None else None,
                   neural.ctypes.data if neural is not None else None, output.ctypes.data)
    return output


def features(colour, rows, columns, noise, controls, history=None, out=None):
    lib = library()
    if lib is None:
        return None
    colour = np.require(colour, dtype=np.float32, requirements=['A'])
    rows = np.require(rows, dtype=np.int32, requirements=['C', 'A'])
    columns = np.require(columns, dtype=np.int32, requirements=['C', 'A'])
    noise = np.require(noise, dtype=np.float32, requirements=['C', 'A'])
    controls = np.require(controls, dtype=np.float32, requirements=['C', 'A'])
    if rows.ndim != 1 or columns.ndim != 1:
        raise ValueError('native feature coordinates must be one-dimensional')
    height, width = len(rows), len(columns)
    if (colour.ndim != 3 or colour.shape[2] != 3 or controls.shape != (5,)
            or noise.shape != (height, width, 3) or height == 0 or width == 0
            or rows.min() < 0 or rows.max() >= colour.shape[0]
            or columns.min() < 0 or columns.max() >= colour.shape[1]):
        raise ValueError('invalid native feature inputs')
    if history is not None:
        history = np.require(history, dtype=np.float32, requirements=['A'])
        if history.shape != colour.shape:
            raise ValueError('history must match the colour it stands beside')
    if out is None:
        output = np.empty((height, width, 16), np.float32)
    else:
        # `out` is where the graph reads its input from, float32 or half: built in place,
        # there is nothing left to copy (`ResidentFrame.input_view`)
        output = out
        if (output.shape != (height, width, 16) or not output.flags.c_contiguous
                or output.dtype not in (np.float32, np.float16)):
            raise ValueError('out must be a C-contiguous (height, width, 16) float32 or '
                             'float16 array')
    build = lib.nr_features_half if output.dtype == np.float16 else lib.nr_features
    build(colour.ctypes.data, *_strides(colour),
          history.ctypes.data if history is not None else None,
          *(_strides(history) if history is not None else (0, 0, 0)),
          rows.ctypes.data, columns.ctypes.data, height, width,
          noise.ctypes.data, controls.ctypes.data, output.ctypes.data)
    return output


def to_half(source, target):
    """float32 `source` into the float16 `target`, rounding to nearest even; `None`
    without the library."""
    lib = library()
    if lib is None:
        return None
    source = np.require(source, dtype=np.float32, requirements=['C', 'A'])
    if (target.dtype != np.float16 or not target.flags.c_contiguous
            or target.size != source.size):
        raise ValueError('to_half needs a C-contiguous float16 target of the same size')
    lib.nr_to_half(source.ctypes.data, source.size, target.ctypes.data)
    return target


def compose_temporal(head, colour, history, previous, gate, mask, *, intensity,
                     blend_scale, hold, slope, table=None, confidence=1.0, release=0.0,
                     grade=None, neural=None):
    """`nr_frame.compose` with a history, a floor and an optional control mask.

    With `table` — `nr_frame.gate_table`, NumPy's sigmoid on every half value — the gate
    and its `confidence` are computed in the pass, and `gate` is not read. Without it,
    `gate` is the model's own weight already through its sigmoid and its confidence in
    NumPy. Either way the sigmoid is NumPy's: `expf` and NumPy's float32 exponential
    disagree in the last bit, and the contract here is byte-identical output rather than
    nearly. `slope` is the folded constant of the floor, for the same reason — see the C.
    `release`, folded the same way, fades the gate where the game's pixel changed; 0 is off.
    `grade` and `neural` are `nr_frame.compose`'s.
    """
    lib = library()
    if lib is None:
        return None
    head = np.require(head, dtype=np.float32, requirements=['A'])
    colour = np.require(colour, dtype=np.float32, requirements=['A'])
    history = np.require(history, dtype=np.float32, requirements=['A'])
    if table is not None:
        # the gate from its 65536-entry table, in the pass itself (`nr_frame.gate_table`)
        table = np.require(table, dtype=np.float32, requirements=['C', 'A'])
        if table.shape != (1 << 16,):
            raise ValueError('the gate table has one entry for each of the 65536 half values')
        gate = np.zeros((1, 1, 1), np.float32)          # unread
    else:
        gate = np.require(gate, dtype=np.float32, requirements=['A'])
    if (head.ndim != 3 or head.shape[2] < 4 or colour.ndim != 3 or colour.shape[2] != 3
            or history.shape != colour.shape or head.shape[:2] != colour.shape[:2]
            or (table is None and (gate.shape[:2] != colour.shape[:2] or gate.ndim != 3
                                   or gate.shape[2] != 1))):
        raise ValueError('the temporal composition needs a four-channel head, a colour, '
                         'a history of the same shape and a single-channel gate')
    if previous is not None:
        previous = np.require(previous, dtype=np.float32, requirements=['A'])
        if previous.shape != colour.shape:
            raise ValueError('the previous frame must match the colour')
    if mask is not None:
        mask = np.require(mask, dtype=np.float32, requirements=['A'])
        if mask.ndim != 3 or mask.shape[:2] != colour.shape[:2]:
            raise ValueError('the control mask must match the colour')
    grade, neural = _grade(grade), _neural(neural, colour.shape)
    output = np.empty(colour.shape, np.float32)
    lib.nr_compose_temporal(
        head.ctypes.data, *_strides(head),
        colour.ctypes.data, *_strides(colour),
        history.ctypes.data, *_strides(history),
        previous.ctypes.data if previous is not None else None,
        *(_strides(previous) if previous is not None else (0, 0, 0)),
        gate.ctypes.data, *_strides(gate)[:2],
        table.ctypes.data if table is not None else None, confidence,
        mask.ctypes.data if mask is not None else None,
        *(_strides(mask)[:2] if mask is not None else (0, 0)),
        *colour.shape[:2], intensity, blend_scale, hold, slope, release,
        grade.ctypes.data if grade is not None else None,
        neural.ctypes.data if neural is not None else None, output.ctypes.data)
    return output


def compose_encode(head, colour, history, previous, mask, encoded, *, top, left, bgra,
                   intensity, blend_scale=0.0, hold=0.0, slope=0.0, table=None,
                   confidence=1.0, release=0.0, samples=None, grade=None, neural=None):
    """`bilinear` of the head to the colour's extent, then `compose_temporal` — or, with
    no history, `compose` — then `encode8` into `encoded` at (`top`, `left`), in one pass.

    `encoded` is a writable copy of the request, (frame_height, frame_width, 4) bytes: its
    alpha and everything outside the colour's region are left as they are, which is what
    encoding the whole frame would have written there. Returns the composition, which is
    what the separate passes return; the bytes they would have encoded are in `encoded`.
    `samples`, a step: the upscaled head on every step-th row and column comes back too,
    as `(composition, samples)` — what `bilinear(head)[::step, ::step]` would hold.
    `grade` and `neural` are `nr_frame.compose`'s; a grade takes the per-pixel loop.
    """
    lib = library()
    if lib is None:
        return None
    head = np.require(head, dtype=np.float32, requirements=['A'])
    colour = np.require(colour, dtype=np.float32, requirements=['A'])
    channels = 4 if history is not None else 3
    height, width = colour.shape[:2]
    if (head.ndim != 3 or head.shape[2] < channels or colour.ndim != 3 or colour.shape[2] != 3
            or head.shape[0] > height or head.shape[1] > width):
        raise ValueError('the fused composition upscales a head to the colour it composes')
    if history is not None:
        history = np.require(history, dtype=np.float32, requirements=['A'])
        table = np.require(table, dtype=np.float32, requirements=['C', 'A'])
        if history.shape != colour.shape or table.shape != (1 << 16,):
            raise ValueError('the temporal composition needs a history of the colour\'s '
                             'shape and the gate table')
    elif mask is not None or previous is not None:
        raise ValueError('a still frame is composed without a mask or a previous frame')
    if previous is not None:
        previous = np.require(previous, dtype=np.float32, requirements=['A'])
        if previous.shape != colour.shape:
            raise ValueError('the previous frame must match the colour')
    if mask is not None:
        mask = np.require(mask, dtype=np.float32, requirements=['A'])
        if mask.ndim != 3 or mask.shape[:2] != colour.shape[:2]:
            raise ValueError('the control mask must match the colour')
    if (not isinstance(encoded, np.ndarray) or encoded.dtype != np.uint8 or encoded.ndim != 3
            or encoded.shape[2] != 4 or not encoded.flags.c_contiguous
            or not encoded.flags.writeable
            or top + height > encoded.shape[0] or left + width > encoded.shape[1]):
        raise ValueError('the encoded frame must be writable HxWx4 bytes around the colour')
    plans = [(_axis_plan(head.shape[axis], count) if head.shape[axis] != count else (None,) * 3)
             for axis, count in enumerate((height, width))]
    pointer = lambda array: array.ctypes.data if array is not None else None
    grade, neural = _grade(grade), _neural(neural, colour.shape)
    output = np.empty(colour.shape, np.float32)
    step = int(samples or 0)
    sampled = (np.empty((-(-height // step), -(-width // step), channels), np.float32)
               if step else None)
    lib.nr_compose_encode(
        head.ctypes.data, *_strides(head), head.shape[1], channels,
        *(pointer(array) for array in plans[0]), *(pointer(array) for array in plans[1]),
        colour.ctypes.data, *_strides(colour),
        pointer(history), *(_strides(history) if history is not None else (0, 0, 0)),
        pointer(previous), *(_strides(previous) if previous is not None else (0, 0, 0)),
        pointer(table), confidence,
        pointer(mask), *(_strides(mask)[:2] if mask is not None else (0, 0)),
        height, width, intensity, blend_scale, hold, slope, release,
        output.ctypes.data, pointer(grade), pointer(neural),
        encoded.ctypes.data, encoded.shape[1], top, left, int(bool(bgra)),
        pointer(sampled), step)
    return (output, sampled) if step else output


@lru_cache(maxsize=32)
def _axis_plan(extent, count):
    centres = (np.arange(count, dtype=np.float32) + 0.5) * (extent / count) - 0.5
    low = np.clip(np.floor(centres), 0, extent - 1).astype(np.int32)
    high = np.clip(low + 1, 0, extent - 1)
    weight = np.clip(centres - low, 0.0, 1.0).astype(np.float32)
    for array in (low, high, weight):
        array.flags.writeable = False
    return low, high, weight


def area_mean(image, factors):
    """`nr_daemon.resample`'s area mean for a downscale by whole factors, byte-identical
    to its NumPy adds; `None` without the library."""
    lib = library()
    if lib is None:
        return None
    source = np.require(image, dtype=np.float32, requirements=['A'])
    fy, fx = (int(f) for f in factors)
    if (source.ndim != 3 or min(source.shape) < 1 or fy < 1 or fx < 1
            or source.shape[0] % fy or source.shape[1] % fx):
        raise ValueError('area_mean expects nonempty HWC whose extent the factors divide')
    height, width = source.shape[0] // fy, source.shape[1] // fx
    output = np.empty((height, width, source.shape[2]), np.float32)
    lib.nr_area_mean(source.ctypes.data, *_strides(source), height, width, source.shape[2],
                     fy, fx, output.ctypes.data)
    return output


def bilinear(image, size):
    lib = library()
    if lib is None:
        return None
    source = np.require(image, dtype=np.float32, requirements=['A'])
    if (source.ndim != 3 or min(source.shape) < 1 or len(size) != 2
            or any(not isinstance(count, (int, np.integer)) for count in size)
            or min(size) < 1):
        raise ValueError('bilinear expects nonempty HWC and positive target dimensions')
    for axis, count in enumerate(size):
        if source.shape[axis] == count:
            continue
        low, high, weight = _axis_plan(source.shape[axis], count)
        shape = list(source.shape)
        shape[axis] = count
        output = np.empty(shape, np.float32)
        lib.nr_resize_axis(source.ctypes.data, *_strides(source), *shape, axis,
                           low.ctypes.data, high.ctypes.data, weight.ctypes.data,
                           output.ctypes.data)
        source = output
    return source
