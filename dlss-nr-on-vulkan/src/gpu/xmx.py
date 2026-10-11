#!/usr/bin/env python3
"""
xmx — host-side interface to the Xe2 cooperative-matrix GEMM.

Backed by `work/libxmx.so` (`.dylib` on macOS, `.dll` on Windows), a resident Vulkan context: the instance, device,
pipeline and buffers are created once and reused, so a call costs a memcpy, a submit
and a fence wait rather than ~80 ms of setup. `NR_GPU_BACKEND=metal` (macOS) or `=d3d12`
(Windows) binds libmetalmx or libd3dmx instead — the same entry points on Metal or
Direct3D 12 (`nr_build.library`).

Two things this layer must do that the kernel does not:

  1. **Rescale both operands by a power of two.** Mesa flushes subnormal FP16 operands to
     zero in the GEMM unless a float-controls mode is declared (notes/phase71). libxmx
     declares `DenormPreserve 16` where the driver can; where it cannot, an activation can
     land there whatever the weights hold. A power-of-two scale is exact, so this is
     lossless. (notes/phase4-subnormal-flush.md's "27 % of this model" was measured on the
     dense-FP16 misreading of the container and is withdrawn: the real weights hold 7
     subnormals, notes/phase61.)
  2. **Pad to the tile shape.** The only float configuration is M=8 N=16 K=16.
"""
import ctypes
import os
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src"))
import nr_build  # noqa: E402
FP16_MAX = 65504.0
TM, TN, TK = 8, 16, 16

_lib = None


def native_library(name):
    """`lib<name>` from the build this tree runs (`nr_build.library`): `.so`, `.dylib` or
    `.dll`. On Windows, where Python since 3.8 finds a DLL's own dependencies only in the
    system folders and the directories added to its search path, the build directory and
    `NR_DLL_PATH` — a toolchain's runtime, MinGW's libwinpthread for one — are added first
    (`nr_build.dll_directories`). `xmx` is the compute runtime, which `NR_GPU_BACKEND`
    redirects to libmetalmx or libd3dmx."""
    return ctypes.CDLL(str(nr_build.library(name)))


def _load(spv=None):
    """The library, opened and with the plain GEMM pipeline built.

    `spv` names the descriptor-path GEMM shader; left as None it follows the device —
    the cooperative-matrix kernel where `VK_KHR_cooperative_matrix` exists, the portable
    multiply-add kernel where it does not (MoltenVK on Apple silicon) or where
    `XMX_PORTABLE=1` asks for it.
    """
    global _lib
    if _lib is not None:
        return _lib
    if sys.platform == "darwin":
        # MoltenVK reads its configuration from the environment when it is first used, so
        # this has to happen before the library loads. Two settings, and the second is a
        # correctness one: Metal compiles shaders with fast math unless told otherwise,
        # which contracts the gate's multiply and add into one FMA and skips the half
        # rounding between them. Every vendor rounding point in `publish.glsl` then moves —
        # the gate epilogue came out 3e-03 off and every E4M3 publish downstream by a
        # quantum — and the frame stops matching the reference. Off, every epilogue,
        # softmax and cosine check is exact on an M3, same as on Xe2.
        os.environ.setdefault("MVK_CONFIG_LOG_LEVEL", "1")       # errors only
        os.environ.setdefault("MVK_CONFIG_FAST_MATH_ENABLED", "0")
    lib = native_library("xmx")
    lib.xmx_init.argtypes = [ctypes.c_char_p]
    lib.xmx_init.restype = ctypes.c_int
    for name in ("xmx_open", "xmx_coopmat", "xmx_portable"):
        getattr(lib, name).argtypes = []
        getattr(lib, name).restype = ctypes.c_int
    lib.xmx_path.restype = ctypes.c_char_p
    lib.xmx_gemm.argtypes = [ctypes.c_uint] * 3 + [ctypes.c_void_p] * 3 + [ctypes.c_uint]
    lib.xmx_gemm.restype = ctypes.c_int
    lib.xmx_reserve.argtypes = [ctypes.c_uint] * 3 + [ctypes.POINTER(ctypes.c_void_p)] * 3
    lib.xmx_reserve.restype = ctypes.c_int
    lib.xmx_reserve_bytes.argtypes = [ctypes.c_ulonglong] * 3 + [ctypes.POINTER(ctypes.c_void_p)] * 3
    lib.xmx_reserve_bytes.restype = ctypes.c_int
    lib.xmx_init_batched.argtypes = [ctypes.c_char_p]
    lib.xmx_init_batched.restype = ctypes.c_int
    lib.xmx_gemm_batched.argtypes = [ctypes.c_uint] * 8
    lib.xmx_gemm_batched.restype = ctypes.c_int
    lib.xmx_error.restype = ctypes.c_char_p
    lib.xmx_device.restype = ctypes.c_char_p
    lib.xmx_memory.restype = ctypes.c_char_p
    if lib.xmx_open() != 0:
        raise RuntimeError("xmx_open: " + lib.xmx_error().decode())
    if spv is None:
        spv = "gemm_portable_desc.spv" if lib.xmx_portable() else "gemm_coopmat.spv"
    if lib.xmx_init(nr_build.shader_arg(lib, spv).encode()) != 0:
        raise RuntimeError("xmx_init: " + lib.xmx_error().decode())
    _lib = lib
    return lib


def device_name():
    return _load().xmx_device().decode()


def portable():
    """True when the GEMMs run on the plain multiply-add kernels rather than the
    cooperative-matrix ones: no `VK_KHR_cooperative_matrix` on the device, or
    `XMX_PORTABLE=1`. The rest of the graph is the same either way."""
    return bool(_load().xmx_portable())


def path_note():
    """Which GEMM kernels this device runs, and why, for a log line."""
    return _load().xmx_path().decode()


def memory_note():
    """Where the buffers landed, which on a discrete card is the whole performance story."""
    return _load().xmx_memory().decode()


def _shift(x):
    """The exact 2^k that lifts |x| just under the FP16 ceiling.

    A driver that cannot be told to keep FP16 subnormals may flush them (Mesa's default
    does, notes/phase71); a power of two is lossless. The "27 %" that
    notes/phase4-subnormal-flush.md reports is withdrawn — it counted the misread decode,
    notes/phase61 — but where a flush happens it is real, and an activation can reach it
    at any time.
    Two reductions rather than `abs(x).max()`, which allocates a whole temporary.
    """
    x = np.asarray(x)
    if x.size == 0:
        return 0
    mx = max(float(x.max()), -float(x.min()))
    return int(np.floor(np.log2(FP16_MAX / mx))) if mx > 0 else 0


def _pad(a, m, n):
    out = np.zeros((m, n), dtype=np.float16)
    out[:a.shape[0], :a.shape[1]] = a
    return out


class Operand:
    """A tile-padded, power-of-two-rescaled FP16 operand, ready to dispatch.

    Preparing a weight once and reusing it removes the float32 -> float16 cast and
    the padding copy from the inner loop; only the memcpy into the mapped buffer
    remains, and on an integrated GPU that is removable too.
    """

    __slots__ = ("data", "rows", "cols", "padded_rows", "padded_cols", "shift")

    def __init__(self, array, *, is_b, rescale=True):
        array = np.asarray(array)
        self.rows, self.cols = array.shape
        self.shift = _shift(array) if rescale else 0
        scaled = (array.astype(np.float32) * np.float32(2.0 ** self.shift)).astype(np.float16)
        self.padded_rows = -(-self.rows // (TK if is_b else TM)) * (TK if is_b else TM)
        self.padded_cols = -(-self.cols // (TN if is_b else TK)) * (TN if is_b else TK)
        if (self.padded_rows, self.padded_cols) == (self.rows, self.cols):
            self.data = np.ascontiguousarray(scaled)
        else:
            self.data = np.ascontiguousarray(_pad(scaled, self.padded_rows, self.padded_cols))


def prepare_b(B, rescale=True):
    """Convert and pad a right-hand operand once, for reuse across calls."""
    return Operand(B, is_b=True, rescale=rescale)


def gemm(A, B, rescale=True, iters=1):
    """C = A @ B, FP16 operands on XMX, FP32 accumulation, returned as float32.

    `B` may be a prepared `Operand`, in which case its conversion is skipped.
    """
    lib = _load()
    left = A if isinstance(A, Operand) else Operand(A, is_b=False, rescale=rescale)
    right = B if isinstance(B, Operand) else Operand(B, is_b=True, rescale=rescale)
    assert left.cols == right.rows, ("inner dimensions disagree: %d vs %d"
                                     % (left.cols, right.rows))
    M, N, K = left.padded_rows, right.padded_cols, left.padded_cols
    assert K == right.padded_rows, "operand padding disagrees on K"
    C = np.empty((M, N), dtype=np.float32)
    if lib.xmx_gemm(M, N, K, left.data.ctypes.data, right.data.ctypes.data,
                    C.ctypes.data, iters) != 0:
        raise RuntimeError("xmx_gemm: " + lib.xmx_error().decode())
    out = C[:left.rows, :right.cols]
    # Undone in two steps: a combined 2^-(ka+kb) can fall out of float32 range when
    # both operands are tiny, and each half on its own cannot.
    return out * np.float32(2.0 ** -left.shift) * np.float32(2.0 ** -right.shift)


# --------------------------------------------------------------------------
# zero-copy path
# --------------------------------------------------------------------------

_last_upload = None     # identifies the B currently sitting in the device buffer


def _mapped(pointer, dtype, shape):
    count = int(np.prod(shape))
    array = np.ctypeslib.as_array(
        ctypes.cast(pointer, ctypes.POINTER(ctypes.c_uint16)), shape=(count * np.dtype(dtype).itemsize // 2,))
    return array.view(dtype).reshape(shape)


def gemm_mapped(A, right, b_key=None):
    """C = A @ right, building A and reading C straight in the mapped buffers.

    `right` must be a prepared `Operand`. `b_key` identifies it so a repeat of the
    same weight skips its upload; pass None to always upload. The memory is
    HOST_CACHED and shared with the GPU, so there is no staging copy on either side.
    """
    global _last_upload
    lib = _load()
    A = np.asarray(A, dtype=np.float32)
    rows, inner = A.shape
    if inner != right.rows:
        raise ValueError("inner dimensions disagree: %d vs %d" % (inner, right.rows))
    M = -(-rows // TM) * TM
    K, N = right.padded_rows, right.padded_cols

    pa, pb, pc = ctypes.c_void_p(), ctypes.c_void_p(), ctypes.c_void_p()
    if lib.xmx_reserve(M, N, K, ctypes.byref(pa), ctypes.byref(pb), ctypes.byref(pc)) != 0:
        raise RuntimeError("xmx_reserve: " + lib.xmx_error().decode())

    left_shift = _shift(A)
    a_mapped = _mapped(pa, np.float16, (M, K))
    if K > inner:
        a_mapped[:rows, inner:] = 0
    if M > rows:
        a_mapped[rows:, :] = 0
    np.multiply(A, np.float32(2.0 ** left_shift), out=a_mapped[:rows, :inner],
                casting="unsafe")

    # The identity of the operand and its padded extent are part of the key: two
    # different weights can otherwise land on the same buffer address without the
    # buffer having been reallocated, and the second would silently reuse the first.
    upload_key = (b_key, id(right), K, N, right.shift, pb.value)
    if b_key is None or _last_upload != upload_key:
        _mapped(pb, np.float16, (K, N))[:] = right.data
        _last_upload = upload_key

    if lib.xmx_gemm(M, N, K, None, None, None, 1) != 0:
        raise RuntimeError("xmx_gemm: " + lib.xmx_error().decode())

    view = _mapped(pc, np.float32, (M, N))[:rows, :right.cols]
    combined = np.float32(2.0 ** -left_shift * 2.0 ** -right.shift)
    if combined == 0 or not np.isfinite(combined):   # only when both operands are tiny
        return view * np.float32(2.0 ** -left_shift) * np.float32(2.0 ** -right.shift)
    return view * combined


_batched_ready = False


def _load_batched(spv=None):
    global _batched_ready
    lib = _load()
    if spv is None:
        spv = "gemm_portable_batched.spv" if lib.xmx_portable() else "gemm_batched.spv"
    if not _batched_ready:
        if lib.xmx_init_batched(nr_build.shader_arg(lib, spv).encode()) != 0:
            raise RuntimeError("xmx_init_batched: " + lib.xmx_error().decode())
        _batched_ready = True
    return lib


def bmm_aligned(A, B, transpose_b=False):
    """C[i] = A[i] @ B[i] (or @ B[i]^T) for a batch of tile-aligned matrices.

    A is (batch, M, K); B is (batch, K, N), or (batch, N, K) with `transpose_b`,
    which is the layout an attention key already has — the cooperative-matrix load
    reads it column-major, so no transpose copy is made.

    Every dimension must already be a multiple of the tile, which for this graph
    they are: window tokens are 64 and head_dim is 32. Raises otherwise, so the
    caller can fall back.
    """
    lib = _load_batched()
    A = np.asarray(A, dtype=np.float32)
    B = np.asarray(B, dtype=np.float32)
    if A.ndim != 3 or B.ndim != 3 or A.shape[0] != B.shape[0]:
        raise ValueError("batched operands must be rank 3 and share a batch")
    batch, rows, inner = A.shape
    cols, inner_b = (B.shape[1], B.shape[2]) if transpose_b else (B.shape[2], B.shape[1])
    if inner_b != inner:
        raise ValueError("inner dimensions disagree: %d vs %d" % (inner, inner_b))
    if rows % TM or cols % TN or inner % TK:
        raise ValueError("batched GEMM needs %dx%dx%d-aligned shapes, got %dx%dx%d"
                         % (TM, TN, TK, rows, cols, inner))

    stride_a, stride_b, stride_c = rows * inner, cols * inner, rows * cols
    pa, pb, pc = ctypes.c_void_p(), ctypes.c_void_p(), ctypes.c_void_p()
    if lib.xmx_reserve_bytes(batch * stride_a * 2, batch * stride_b * 2,
                             batch * stride_c * 4, ctypes.byref(pa), ctypes.byref(pb),
                             ctypes.byref(pc)) != 0:
        raise RuntimeError("xmx_reserve_bytes: " + lib.xmx_error().decode())
    left_shift, right_shift = _shift(A), _shift(B)
    np.multiply(A, np.float32(2.0 ** left_shift),
                out=_mapped(pa, np.float16, A.shape), casting="unsafe")
    np.multiply(B, np.float32(2.0 ** right_shift),
                out=_mapped(pb, np.float16, B.shape), casting="unsafe")
    if lib.xmx_gemm_batched(rows, cols, inner, batch, stride_a, stride_b, stride_c,
                            1 if transpose_b else 0) != 0:
        raise RuntimeError("xmx_gemm_batched: " + lib.xmx_error().decode())
    view = _mapped(pc, np.float32, (batch, rows, cols))
    combined = np.float32(2.0 ** -left_shift * 2.0 ** -right_shift)
    if combined == 0 or not np.isfinite(combined):
        return view * np.float32(2.0 ** -left_shift) * np.float32(2.0 ** -right_shift)
    return view * combined


def is_half_valued(A):
    """True when float16 holds `A` exactly, so an FP16 GEMM loses nothing on it."""
    with np.errstate(over="ignore"):
        return np.array_equal(A.astype(np.float16).astype(np.float32), A)


def gemm_split(A, right, b_key=None, parts=2):
    """C = A @ right with `A` carried as a sum of halves.

    Each step takes the float16 part and leaves an exact float32 remainder — the
    subtraction of two nearby values is exact — so two parts keep ~22 mantissa bits
    and three keep more than float32 has. Measured relative error against a float32
    GEMM: 2e-04 with one part, 6e-07 with two, ~1e-10 with three. The dispatches
    share a `b_key`, so the weight is uploaded once.
    """
    total = None
    residual = A
    for index in range(parts):
        with np.errstate(over="ignore"):
            part = residual.astype(np.float16).astype(np.float32)
        product = gemm_mapped(part, right, b_key=b_key)
        total = product if total is None else total + product
        if index + 1 < parts:
            residual = residual - part
    return total
