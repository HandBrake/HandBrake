"""Keep the daemon's large NumPy blocks from one frame to the next, on Windows.

Windows' heap gives a freed block of about a megabyte or more back to the system, and the next
array of that size pays a page fault for every 4 KB of its fresh pages: ~16 000 faults and
11-15 ms of a 1280x720 frame (HANDOFF, 2026-10-02). `src/layer/nr_alloc.c`, built as
`work/libnr_alloc.dll`, is a NumPy allocator in front of NumPy's own that keeps those blocks
and gives back whatever a whole frame did not take again.

    nr_alloc.install()      # in the thread that runs the frames: NumPy's handler is per context
    nr_alloc.frame_done()   # after each frame

`NR_KEEP_BLOCKS=0` leaves NumPy's allocator alone. Linux does not build the library: there
every stage of the daemon's frame is under 2.2 ms (HANDOFF, 2026-10-02).
"""
import ctypes
import os
import pathlib

try:
    from numpy._core import _multiarray_umath as _umath, multiarray as _multiarray
except ImportError:                       # NumPy 1.x
    from numpy.core import _multiarray_umath as _umath, multiarray as _multiarray

NAME = "nr_keep_blocks"
LIBRARY = pathlib.Path(__file__).resolve().parents[2] / "work" / "libnr_alloc.dll"
_SET_HANDLER, _GET_HANDLER = 304, 305     # PyDataMem_SetHandler and _GetHandler in NumPy's C API
_CAPSULE_NAME = ctypes.create_string_buffer(b"mem_handler")   # a capsule keeps its name's pointer
_state = {}


def _library():
    if "lib" not in _state:
        lib = None
        if os.name == "nt" and LIBRARY.exists():
            # PyDLL, not CDLL: the GIL stays held through every call, as NumPy's own free,
            # which nr_alloc_frame calls, expects.
            lib = ctypes.PyDLL(str(LIBRARY))
            lib.nr_alloc_handler.restype = ctypes.c_void_p
            lib.nr_alloc_handler.argtypes = (ctypes.c_void_p,)
            lib.nr_alloc_frame.restype = lib.nr_alloc_release.restype = ctypes.c_size_t
            lib.nr_alloc_configure.argtypes = (ctypes.c_size_t, ctypes.c_size_t, ctypes.c_int)
            lib.nr_alloc_counts.argtypes = (ctypes.POINTER(ctypes.c_longlong),)
        _state["lib"] = lib
    return _state["lib"]


def _api(index, restype, *argtypes):
    """A function from NumPy's C API table, which the extension module exports as a capsule."""
    pointer = ctypes.pythonapi.PyCapsule_GetPointer
    pointer.restype, pointer.argtypes = ctypes.c_void_p, (ctypes.py_object, ctypes.c_char_p)
    table = ctypes.cast(pointer(_umath._ARRAY_API, None), ctypes.POINTER(ctypes.c_void_p))
    return ctypes.PYFUNCTYPE(restype, *argtypes)(table[index])


def install():
    """The allocator in front of NumPy's, in the calling thread's context. True when it is in
    place; False off Windows, with NR_KEEP_BLOCKS=0, or when the library is not built."""
    if os.environ.get("NR_KEEP_BLOCKS", "1") == "0":
        return False
    lib = _library()
    if lib is None:
        return False
    if _multiarray.get_handler_name() == NAME:
        return True
    current = _api(_GET_HANDLER, ctypes.py_object)()
    pointer = ctypes.pythonapi.PyCapsule_GetPointer
    pointer.restype, pointer.argtypes = ctypes.c_void_p, (ctypes.py_object, ctypes.c_char_p)
    handler = lib.nr_alloc_handler(pointer(current, _CAPSULE_NAME.value))
    if not handler:
        return False
    if "capsule" not in _state:
        new = ctypes.pythonapi.PyCapsule_New
        new.restype, new.argtypes = ctypes.py_object, (ctypes.c_void_p, ctypes.c_void_p,
                                                       ctypes.c_void_p)
        _state["capsule"] = new(handler, ctypes.addressof(_CAPSULE_NAME), None)
    _state.setdefault("previous", current)
    _api(_SET_HANDLER, ctypes.py_object, ctypes.py_object)(_state["capsule"])
    return _multiarray.get_handler_name() == NAME


def uninstall():
    """NumPy's own handler back in the calling thread's context; the kept blocks go back too.
    Arrays made while it was installed still free through it."""
    if "previous" in _state and _multiarray.get_handler_name() == NAME:
        _api(_SET_HANDLER, ctypes.py_object, ctypes.py_object)(_state["previous"])
    lib = _library()
    if lib is not None and "capsule" in _state:
        lib.nr_alloc_release()


def frame_done():
    """The end of a frame: blocks a whole frame did not take again go back to the system."""
    lib = _library()
    return lib.nr_alloc_frame() if lib is not None and "capsule" in _state else 0


def configure(threshold=1 << 20, cap=256 << 20, poison=False):
    """The smallest block kept, the most bytes kept at once, and the tests' poison: every
    large block handed out filled with 0xff, so a read before a write shows."""
    _library().nr_alloc_configure(threshold, cap, int(poison))


def counts():
    """Hits and misses of large blocks, blocks given back, bytes kept now, the most kept at
    once during the last frame, and slots in use."""
    values = (ctypes.c_longlong * 6)()
    lib = _library()
    if lib is not None:
        lib.nr_alloc_counts(values)
    return dict(zip(("hits", "misses", "returned", "kept_bytes", "frame_most", "slots"), values))
