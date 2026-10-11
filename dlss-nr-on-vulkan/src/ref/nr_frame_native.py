"""The C frame library (`work/libnr_frame.so`, `.dylib` on macOS, from `src/ref/nr_frame.c`) for NumPy callers.

`nr_frame.py` is the reference; this is the same frame in C, reached through ctypes so
the two can be run side by side. Images are float32 (height, width, 3) in [0, 1].

    frame = NativeFrame(weights_path)                   # or NativeFrame(): the weights compiled in
    output = frame.update(colour)                       # (h, w, 3)
    output, head = frame.update(colour, want_head=True) # head (h, w, 4)
    features = frame.features(colour)                   # (H, W, 16) at the network extent
    head = frame.run_features(features)                 # (H, W, 4)
    live = NativeLive(frame)                            # the daemon's frame, stateful
    answer, report = live.run(bgra, render_scale=0.5)   # (h, w, 4) uint8 in, the same out
"""
import ctypes as C
import os
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src"))
import nr_build  # noqa: E402
LIBRARY = nr_build.library("nr_frame")


class Params(C.Structure):
    _fields_ = [("intensity", C.c_float), ("detail_strength", C.c_float),
                ("colour_strength", C.c_float), ("detail_radius", C.c_float),
                ("normalized_style", C.c_float), ("local_tone", C.c_float),
                ("local_structure", C.c_float), ("frame_index", C.c_int),
                ("history_confidence", C.c_float), ("blend_scale", C.c_float),
                ("hold", C.c_float), ("slope", C.c_float), ("release", C.c_float),
                ("automatic_mask", C.c_int), ("skin_structure", C.c_float),
                ("automatic_structure", C.c_float), ("min_extent", C.c_int),
                ("grade_off", C.c_int)]


class LiveSettings(C.Structure):
    _fields_ = [("render_scale", C.c_float), ("temporal", C.c_float), ("cut_limit", C.c_float),
                ("hold", C.c_float), ("release", C.c_float), ("letterbox", C.c_int)]


class LiveReport(C.Structure):
    _fields_ = [("top", C.c_int), ("bottom", C.c_int), ("left", C.c_int), ("right", C.c_int),
                ("render_width", C.c_int), ("render_height", C.c_int),
                ("network_width", C.c_int), ("network_height", C.c_int),
                ("with_history", C.c_int), ("cut", C.c_double), ("change", C.c_double)]


_lib = None


def library():
    global _lib
    if _lib is None:
        if sys.platform == "darwin":
            os.environ.setdefault("MVK_CONFIG_LOG_LEVEL", "1")
            os.environ.setdefault("MVK_CONFIG_FAST_MATH_ENABLED", "0")
        lib = C.CDLL(str(LIBRARY))
        lib.nr_frame_open.argtypes = [C.c_char_p]
        lib.nr_frame_open.restype = C.c_void_p
        lib.nr_frame_close.argtypes = [C.c_void_p]
        lib.nr_frame_close.restype = None
        lib.nr_frame_error.restype = C.c_char_p
        lib.nr_frame_embedded_weights_size.argtypes = []
        lib.nr_frame_embedded_weights_size.restype = C.c_size_t
        if hasattr(lib, "nr_frame_prepare"):          # a library older than 2026-10-10 has none
            lib.nr_frame_prepare.argtypes = [C.c_char_p, C.c_char_p]
            lib.nr_frame_prepare.restype = C.c_int
        lib.nr_frame_device.argtypes = [C.c_void_p]
        lib.nr_frame_device.restype = C.c_char_p
        lib.nr_frame_gemm_path.argtypes = [C.c_void_p]
        lib.nr_frame_gemm_path.restype = C.c_char_p
        for name in ("nr_frame_discrete", "nr_frame_preserve16", "nr_frame_half_by_cast",
                     "nr_frame_staged_packed", "nr_frame_portable_packed"):
            getattr(lib, name).argtypes = []
            getattr(lib, name).restype = C.c_int
        lib.nr_frame_input_view.argtypes = [C.c_void_p]
        lib.nr_frame_input_view.restype = C.c_int
        if hasattr(lib, "nr_frame_prepared"):          # 2026-10-10
            lib.nr_frame_prepared.argtypes = [C.c_void_p]
            lib.nr_frame_prepared.restype = C.c_int
            for name in ("nr_frame_weights_aliased", "nr_frame_weights_count"):
                getattr(lib, name).argtypes = [C.c_void_p]
                getattr(lib, name).restype = C.c_size_t
        lib.nr_frame_defaults.argtypes = [C.POINTER(Params)]
        lib.nr_frame_defaults.restype = None
        lib.nr_frame_geometry.argtypes = [C.c_int, C.c_int, C.POINTER(C.c_int), C.POINTER(C.c_int)]
        lib.nr_frame_geometry.restype = None
        lib.nr_frame_geometry_min.argtypes = [C.c_int, C.c_int, C.c_int, C.POINTER(C.c_int),
                                              C.POINTER(C.c_int)]
        lib.nr_frame_geometry_min.restype = None
        lib.nr_frame_compose_encode.argtypes = [
            C.c_void_p, C.c_void_p, C.c_int, C.c_int, C.c_void_p, C.c_int, C.c_int, C.c_void_p,
            C.c_void_p, C.c_void_p, C.POINTER(Params), C.c_void_p, C.c_void_p, C.c_int, C.c_int,
            C.c_int, C.c_int]
        lib.nr_frame_compose_encode.restype = C.c_int
        lib.nr_frame_update.argtypes = [C.c_void_p, C.c_void_p, C.c_int, C.c_int, C.c_void_p,
                                        C.c_void_p, C.POINTER(Params), C.c_void_p, C.c_void_p]
        lib.nr_frame_update.restype = C.c_int
        lib.nr_frame_features.argtypes = [C.c_void_p, C.c_void_p, C.c_int, C.c_int, C.c_void_p,
                                          C.POINTER(Params), C.c_void_p]
        lib.nr_frame_features.restype = C.c_int
        lib.nr_frame_run_features.argtypes = [C.c_void_p, C.c_void_p, C.c_int, C.c_int, C.c_void_p]
        lib.nr_frame_run_features.restype = C.c_int
        lib.nr_frame_update_masked.argtypes = [C.c_void_p, C.c_void_p, C.c_int, C.c_int, C.c_void_p,
                                               C.c_void_p, C.c_void_p, C.POINTER(Params), C.c_void_p,
                                               C.c_void_p]
        lib.nr_frame_update_masked.restype = C.c_int
        lib.nr_frame_features_masked.argtypes = [C.c_void_p, C.c_void_p, C.c_int, C.c_int, C.c_void_p,
                                                 C.c_void_p, C.POINTER(Params), C.c_void_p]
        lib.nr_frame_features_masked.restype = C.c_int
        lib.nr_frame_compose.argtypes = [C.c_void_p, C.c_void_p, C.c_void_p, C.c_int, C.c_int, C.c_void_p,
                                         C.c_void_p, C.c_void_p, C.POINTER(Params), C.c_void_p]
        lib.nr_frame_compose.restype = C.c_int
        lib.nr_frame_update_neural.argtypes = lib.nr_frame_update_masked.argtypes + [C.c_void_p]
        lib.nr_frame_update_neural.restype = C.c_int
        lib.nr_frame_compose_neural.argtypes = lib.nr_frame_compose.argtypes + [C.c_void_p]
        lib.nr_frame_compose_neural.restype = C.c_int
        lib.nr_frame_compose_encode_neural.argtypes = lib.nr_frame_compose_encode.argtypes + [C.c_void_p]
        lib.nr_frame_compose_encode_neural.restype = C.c_int
        lib.nr_frame_render_extent.argtypes = [C.c_int, C.c_int, C.c_float, C.c_int,
                                               C.POINTER(C.c_int), C.POINTER(C.c_int)]
        lib.nr_frame_render_extent.restype = None
        lib.nr_frame_grade.argtypes = [C.POINTER(Params), C.POINTER(C.c_float * 3)]
        lib.nr_frame_grade.restype = C.c_int
        lib.nr_frame_split.argtypes = [C.c_void_p, C.c_int]
        lib.nr_frame_split.restype = C.c_double
        lib.nr_frame_head.argtypes = [C.c_void_p, C.c_void_p, C.c_int, C.c_int, C.c_void_p,
                                      C.c_void_p, C.c_void_p, C.c_void_p]
        lib.nr_frame_head.restype = C.c_int
        lib.nr_frame_live_defaults.argtypes = [C.POINTER(LiveSettings)]
        lib.nr_frame_live_defaults.restype = None
        lib.nr_frame_live_open.argtypes = [C.c_void_p]
        lib.nr_frame_live_open.restype = C.c_void_p
        lib.nr_frame_live_reset.argtypes = [C.c_void_p]
        lib.nr_frame_live_reset.restype = None
        lib.nr_frame_live_close.argtypes = [C.c_void_p]
        lib.nr_frame_live_close.restype = None
        lib.nr_frame_live_run.argtypes = [C.c_void_p, C.POINTER(Params), C.POINTER(LiveSettings),
                                          C.c_void_p, C.c_int, C.c_int, C.c_int, C.c_void_p,
                                          C.POINTER(LiveReport)]
        lib.nr_frame_live_run.restype = C.c_int
        _lib = lib
    return _lib


def geometry(height, width, min_extent=320):
    """The network extent for an output extent, at the floor `min_extent`."""
    h, w = C.c_int(), C.c_int()
    library().nr_frame_geometry_min(height, width, min_extent, C.byref(h), C.byref(w))
    return h.value, w.value


def render_extent(width, height, scale, min_extent=320):
    """`nr_frame.render_extent` in C: the (width, height) the network is handed."""
    w, h = C.c_int(), C.c_int()
    library().nr_frame_render_extent(width, height, scale, min_extent, C.byref(w), C.byref(h))
    return w.value, h.value


def grade(**values):
    """`nr_frame.grade_for` in C, for the parameters `params` would make: three float32
    factors, or None."""
    out = (C.c_float * 3)()
    if not library().nr_frame_grade(C.byref(params(**values)), C.byref(out)):
        return None
    return tuple(np.float32(v) for v in out)


def params(**values):
    p = Params()
    library().nr_frame_defaults(C.byref(p))
    for name, value in values.items():
        if not hasattr(p, name):
            raise TypeError(f"no such parameter: {name}")
        setattr(p, name, value)
    return p


def _image(array, name, channels):
    array = np.ascontiguousarray(array, dtype=np.float32)
    if array.ndim != 3 or array.shape[2] != channels:
        raise ValueError(f"{name} must be (height, width, {channels})")
    return array


def _neural(neural, colour):
    """Where the C keeps the vendor's history: None, or a writable C-contiguous float32
    array of the colour's shape, filled in place."""
    if neural is None:
        return None
    if (not isinstance(neural, np.ndarray) or neural.dtype != np.float32
            or neural.shape != colour.shape or not neural.flags.c_contiguous
            or not neural.flags.writeable):
        raise ValueError("neural must be writable C-contiguous float32 of the colour's shape")
    return neural.ctypes.data


def embedded_weights_size():
    """Bytes of logical safetensors compiled into the library (CMake, bin2c), or 0."""
    return library().nr_frame_embedded_weights_size()


class NativeFrame:
    def __init__(self, weights_path=None):
        """`weights_path` None reads the weights compiled into the library, and fails
        with the library's message in a build that has none."""
        self.lib = library()
        path = None if weights_path is None else str(weights_path).encode()
        self.handle = self.lib.nr_frame_open(path)
        if not self.handle:
            raise RuntimeError("nr_frame_open: " + self.lib.nr_frame_error().decode())

    def close(self):
        if self.handle:
            self.lib.nr_frame_close(self.handle)
            self.handle = None

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass

    @property
    def device(self):
        return self.lib.nr_frame_device(self.handle).decode()

    @property
    def gemm_path(self):
        return self.lib.nr_frame_gemm_path(self.handle).decode()

    @property
    def discrete(self):
        """1 on a card with memory of its own, 0 on a shared-memory device, -1 unknown."""
        return self.lib.nr_frame_discrete()

    @property
    def preserve16(self):
        """Whether the GEMMs keep float16 subnormals (`nr_frame_preserve16`): libxmx's declared
        `DenormPreserve 16` where the driver takes it, Metal's and Direct3D 12's own."""
        return self.lib.nr_frame_preserve16()

    @property
    def half_by_cast(self):
        """Which `half_round` the pipelines compile: 1 the cast, 0 the pack round trip."""
        return self.lib.nr_frame_half_by_cast()

    @property
    def staged_packed(self):
        """Whether the staged kernel copies its operands as raw 128-bit vectors
        (`nr_frame_staged_packed`): 1 by default on Vulkan and Metal, 0 under XMX_STAGED_PACKED=0
        and on Direct3D 12, which has no staged kernel; the same bits either way."""
        return self.lib.nr_frame_staged_packed()

    @property
    def portable_packed(self):
        """Whether the portable GEMM fetches A sixteen bytes at a time (`nr_frame_portable_packed`):
        0 by default, 1 under XMX_PORTABLE_PACKED=1; the same bits either way."""
        return self.lib.nr_frame_portable_packed()

    @property
    def prepared(self):
        """Whether the weights this frame opened were prepared (`nr_frame_prepared`): the
        derived tensors in, and from v2 every tensor read in place; -1 on an older library."""
        return self.lib.nr_frame_prepared(self.handle) if hasattr(self.lib, "nr_frame_prepared") else -1

    @property
    def weights_aliased(self):
        """(tensors read in place out of the slices or the file's mapping, tensors in all)."""
        if not hasattr(self.lib, "nr_frame_weights_aliased"):
            return (0, 0)
        return (self.lib.nr_frame_weights_aliased(self.handle), self.lib.nr_frame_weights_count(self.handle))

    @property
    def input_view(self):
        """Whether the features are built in the graph's mapped input (NR_INPUT_VIEW)."""
        return self.lib.nr_frame_input_view(self.handle)

    @property
    def split(self):
        """Seconds of the last run: input write, graph, head read."""
        return tuple(self.lib.nr_frame_split(self.handle, i) for i in range(3))

    def _fail(self, call):
        raise RuntimeError(f"{call}: {self.lib.nr_frame_error().decode()}")

    @staticmethod
    def _optional(image, colour, name):
        if image is None:
            return None, None
        image = _image(image, name, 3)
        if image.shape != colour.shape:
            raise ValueError(f"{name} must match the colour image shape")
        return image, image.ctypes.data

    def update(self, colour, history=None, previous=None, control_mask=None, want_head=False,
               neural=None, **values):
        """`neural`, an array of the colour's shape, receives the vendor's history
        (`nr_frame_update_neural`)."""
        colour = _image(colour, "colour", 3)
        height, width = colour.shape[:2]
        history, hp = self._optional(history, colour, "history")
        previous, pp = self._optional(previous, colour, "previous")
        control_mask, mp = self._optional(control_mask, colour, "control_mask")
        output = np.empty((height, width, 3), np.float32)
        head = np.empty((height, width, 4), np.float32) if want_head else None
        p = params(**values)
        if self.lib.nr_frame_update_neural(
                self.handle, colour.ctypes.data, height, width, hp, pp, mp,
                C.byref(p), output.ctypes.data,
                head.ctypes.data if head is not None else None, _neural(neural, colour)):
            self._fail("nr_frame_update_neural")
        return (output, head) if want_head else output

    def features(self, colour, history=None, control_mask=None, **values):
        colour = _image(colour, "colour", 3)
        height, width = colour.shape[:2]
        history, hp = self._optional(history, colour, "history")
        control_mask, mp = self._optional(control_mask, colour, "control_mask")
        p = params(**values)
        H, W = geometry(height, width, p.min_extent)
        out = np.empty((H, W, 16), np.float32)
        if self.lib.nr_frame_features_masked(self.handle, colour.ctypes.data, height, width, hp, mp,
                                             C.byref(p), out.ctypes.data):
            self._fail("nr_frame_features_masked")
        return out

    def compose(self, head, colour, history=None, previous=None, control_mask=None, neural=None,
                **values):
        """The composition of a cropped (h, w, 4) head, at any intensity, without the network.
        `neural` is `update`'s."""
        colour = _image(colour, "colour", 3)
        head = _image(head, "head", 4)
        height, width = colour.shape[:2]
        if head.shape[:2] != colour.shape[:2]:
            raise ValueError("head and colour must share height and width")
        history, hp = self._optional(history, colour, "history")
        previous, pp = self._optional(previous, colour, "previous")
        control_mask, mp = self._optional(control_mask, colour, "control_mask")
        output = np.empty((height, width, 3), np.float32)
        p = params(**values)
        if self.lib.nr_frame_compose_neural(self.handle, head.ctypes.data, colour.ctypes.data, height,
                                            width, hp, pp, mp, C.byref(p), output.ctypes.data,
                                            _neural(neural, colour)):
            self._fail("nr_frame_compose_neural")
        return output

    def compose_encode(self, head, colour, encoded, top=0, left=0, bgra=True, history=None,
                       previous=None, control_mask=None, neural=None, **values):
        """`nr_frame.compose_encode` in C: the (hh, hw, 4) head brought up to the colour's
        extent, composed and encoded into `encoded` (a writable (H, W, 4) uint8 copy of the
        request) at (top, left). Returns the composition."""
        colour = _image(colour, "colour", 3)
        head = _image(head, "head", 4)
        height, width = colour.shape[:2]
        if (not isinstance(encoded, np.ndarray) or encoded.dtype != np.uint8 or encoded.ndim != 3
                or encoded.shape[2] != 4 or not encoded.flags.c_contiguous):
            raise ValueError("encoded must be a C-contiguous (H, W, 4) uint8 array")
        history, hp = self._optional(history, colour, "history")
        previous, pp = self._optional(previous, colour, "previous")
        control_mask, mp = self._optional(control_mask, colour, "control_mask")
        output = np.empty((height, width, 3), np.float32)
        p = params(**values)
        if self.lib.nr_frame_compose_encode_neural(
                self.handle, head.ctypes.data, head.shape[0], head.shape[1],
                colour.ctypes.data, height, width, hp, pp, mp, C.byref(p),
                output.ctypes.data, encoded.ctypes.data, encoded.shape[1],
                top, left, int(bool(bgra)), _neural(neural, colour)):
            self._fail("nr_frame_compose_encode_neural")
        return output

    def run_features(self, features):
        features = _image(features, "features", 16)
        H, W = features.shape[:2]
        head = np.empty((H, W, 4), np.float32)
        if self.lib.nr_frame_run_features(self.handle, features.ctypes.data, H, W, head.ctypes.data):
            self._fail("nr_frame_run_features")
        return head

    def head(self, colour, history=None, control_mask=None, **values):
        """`nr_frame_head`: the network half of `update`, the head cropped to (h, w, 4)."""
        colour = _image(colour, "colour", 3)
        height, width = colour.shape[:2]
        history, hp = self._optional(history, colour, "history")
        control_mask, mp = self._optional(control_mask, colour, "control_mask")
        head = np.empty((height, width, 4), np.float32)
        p = params(**values)
        if self.lib.nr_frame_head(self.handle, colour.ctypes.data, height, width, hp, mp,
                                  C.byref(p), head.ctypes.data):
            self._fail("nr_frame_head")
        return head


def prepare(out_path, weights_path=None):
    """`nr_frame_prepare`: write the weights (a logical safetensors, or None for the compiled-in
    ones) as a prepared file, the loaders' layout work done; `NativeFrame(out_path)` opens it.
    Raises on a library without it or on failure."""
    lib = library()
    if not hasattr(lib, "nr_frame_prepare"):
        raise RuntimeError("this libnr_frame has no nr_frame_prepare; rebuild it")
    weights = None if weights_path is None else str(weights_path).encode()
    if lib.nr_frame_prepare(weights, str(out_path).encode()):
        raise RuntimeError("nr_frame_prepare: " + lib.nr_frame_error().decode())
    return out_path


def live_settings(**values):
    s = LiveSettings()
    library().nr_frame_live_defaults(C.byref(s))
    for name, value in values.items():
        if not hasattr(s, name):
            raise TypeError(f"no such live setting: {name}")
        setattr(s, name, value)
    return s


class NativeLive:
    """`nr_frame_live`: the daemon's frame through C, history and letterbox kept between
    calls. `run` takes an (h, w, 4) uint8 request and returns the answer and the report as a
    dict; keyword arguments are `nr_frame_live_settings` fields or `nr_frame_params` ones."""

    def __init__(self, frame):
        self.frame = frame
        self.lib = frame.lib
        self.handle = self.lib.nr_frame_live_open(frame.handle)
        if not self.handle:
            raise RuntimeError("nr_frame_live_open: " + self.lib.nr_frame_error().decode())

    def close(self):
        if self.handle:
            self.lib.nr_frame_live_close(self.handle)
            self.handle = None

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass

    def reset(self):
        self.lib.nr_frame_live_reset(self.handle)

    def run(self, pixels, bgra=True, **values):
        pixels = np.ascontiguousarray(pixels, dtype=np.uint8)
        if pixels.ndim != 3 or pixels.shape[2] != 4:
            raise ValueError("pixels must be (height, width, 4) uint8")
        live = {k: v for k, v in values.items() if k in dict(LiveSettings._fields_)}
        rest = {k: v for k, v in values.items() if k not in live}
        s, p, report = live_settings(**live), params(**rest), LiveReport()
        answer = np.empty_like(pixels)
        height, width = pixels.shape[:2]
        if self.lib.nr_frame_live_run(self.handle, C.byref(p), C.byref(s), pixels.ctypes.data,
                                      width, height, int(bool(bgra)), answer.ctypes.data,
                                      C.byref(report)):
            raise RuntimeError("nr_frame_live_run: " + self.lib.nr_frame_error().decode())
        return answer, {name: getattr(report, name) for name, _ in LiveReport._fields_}
