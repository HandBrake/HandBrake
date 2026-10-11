#!/usr/bin/env python3
"""Does Intel's DPAS through OpenCL beat the cooperative-matrix path through Vulkan?

Phase 26 named this the one lever with a factor left in it: OpenCL exposes
`cl_intel_subgroup_matrix_multiply_accumulate` and a **subgroup size of 16**, where
every cooperative-matrix configuration on this device requires 32. The kernel came from
a parallel run with no harness and no measurement; this is the measurement.

Driven through ctypes because the machine has `libOpenCL.so` and no CL headers. Declare
every entry point: without argtypes, ctypes passes handles as truncated ints and the
first call into the driver takes the process with it.
"""
import argparse, ctypes, pathlib, time
import numpy as np

CL = ctypes.CDLL("libOpenCL.so")
V, U, I, S, L = ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_size_t, ctypes.c_ulonglong
PI, PU = ctypes.POINTER(I), ctypes.POINTER(U)
for name, args, ret in (
        ("clGetPlatformIDs", [U, V, PU], I),
        ("clGetDeviceIDs", [V, L, U, V, PU], I),
        ("clCreateContext", [V, U, V, V, V, PI], V),
        ("clCreateCommandQueue", [V, V, L, PI], V),
        ("clCreateProgramWithSource", [V, U, V, V, PI], V),
        ("clBuildProgram", [V, U, V, ctypes.c_char_p, V, V], I),
        ("clGetProgramBuildInfo", [V, V, U, S, V, V], I),
        ("clCreateKernel", [V, ctypes.c_char_p, PI], V),
        ("clCreateBuffer", [V, L, S, V, PI], V),
        ("clSetKernelArg", [V, U, S, V], I),
        ("clEnqueueNDRangeKernel", [V, V, U, V, V, V, U, V, V], I),
        ("clEnqueueReadBuffer", [V, V, U, S, S, V, U, V, V], I),
        ("clFinish", [V], I)):
    getattr(CL, name).argtypes, getattr(CL, name).restype = args, ret

GPU, READ_ONLY, WRITE_ONLY, COPY_HOST = 1 << 2, 1 << 2, 1 << 1, 1 << 5
BUILD_LOG = 0x1183


def device_and_context():
    platforms, count = (V * 4)(), U()
    CL.clGetPlatformIDs(4, platforms, ctypes.byref(count))
    for index in range(count.value):
        devices, found = (V * 4)(), U()
        if not CL.clGetDeviceIDs(V(platforms[index]), GPU, 4, devices, ctypes.byref(found)) and found.value:
            device, err = V(devices[0]), I()
            context = CL.clCreateContext(None, 1, ctypes.byref(device), None, None, ctypes.byref(err))
            if err.value:
                raise RuntimeError(f"clCreateContext: {err.value}")
            return device, context
    raise RuntimeError("no OpenCL GPU")


def compile_kernel(context, device, source, options):
    err = I()
    text = source.encode()
    array, length = ctypes.c_char_p(text), S(len(text))
    program = CL.clCreateProgramWithSource(context, 1, ctypes.byref(array),
                                           ctypes.byref(length), ctypes.byref(err))
    if err.value:
        raise RuntimeError(f"clCreateProgramWithSource: {err.value}")
    if CL.clBuildProgram(program, 1, ctypes.byref(device), options.encode(), None, None):
        size = S()
        CL.clGetProgramBuildInfo(program, device, BUILD_LOG, 0, None, ctypes.byref(size))
        log = ctypes.create_string_buffer(size.value)
        CL.clGetProgramBuildInfo(program, device, BUILD_LOG, size.value, log, None)
        raise RuntimeError(log.value.decode(errors="replace").strip() or "build failed")
    kernel = CL.clCreateKernel(program, b"gemm", ctypes.byref(err))
    if err.value:
        raise RuntimeError(f"clCreateKernel: {err.value}")
    return kernel


def run(M, N, K, RM, RN, source, repeats=3, calls=40):
    device, context = device_and_context()
    kernel = compile_kernel(context, device, source, f"-DRM={RM} -DRN={RN}")
    err = I()
    queue = CL.clCreateCommandQueue(context, device, 0, ctypes.byref(err))
    rng = np.random.default_rng(0)
    A = (rng.standard_normal((M, K)) * 0.1).astype(np.float16)
    B = (rng.standard_normal((K, N)) * 0.1).astype(np.float16)
    C = np.zeros((M, N), np.float32)

    def buffer(array, flags):
        handle = CL.clCreateBuffer(context, flags | COPY_HOST, array.nbytes,
                                   array.ctypes.data, ctypes.byref(err))
        if err.value:
            raise RuntimeError(f"clCreateBuffer: {err.value}")
        return V(handle)

    da, db, dc = buffer(A, READ_ONLY), buffer(B, READ_ONLY), buffer(C, WRITE_ONLY)
    for index, handle in ((0, da), (1, db), (2, dc)):
        CL.clSetKernelArg(kernel, index, ctypes.sizeof(V), ctypes.byref(handle))
    for index, value in ((3, M), (4, N), (5, K)):
        scalar = U(value)
        CL.clSetKernelArg(kernel, index, ctypes.sizeof(scalar), ctypes.byref(scalar))
    work = (S * 2)(N // (16 * RN) * 16, M // (8 * RM))
    group = (S * 2)(16, 1)

    def once():
        for _ in range(calls):
            status = CL.clEnqueueNDRangeKernel(queue, kernel, 2, None, work, group, 0, None, None)
            if status:
                raise RuntimeError(f"clEnqueueNDRangeKernel: {status}")
        CL.clFinish(queue)

    once()
    best = min((lambda t=time.perf_counter(): (once(), time.perf_counter() - t)[1])()
               for _ in range(repeats))
    CL.clEnqueueReadBuffer(queue, dc, 1, 0, C.nbytes, C.ctypes.data, 0, None, None)
    want = A.astype(np.float32) @ B.astype(np.float32)
    error = float(np.abs(C - want).max() / max(float(np.abs(want).max()), 1e-9))
    return 2.0 * M * N * K * calls / best / 1e9, error


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--blocks", default="1x1,2x1,2x2,4x2,4x4")
    parser.add_argument("--shape", default="8192,512,1024")
    args = parser.parse_args()
    M, N, K = (int(v) for v in args.shape.split(","))
    source = pathlib.Path(__file__).with_suffix(".cl").read_text()
    print(f"  {M}x{N}x{K}, subgroup 16, Intel DPAS through OpenCL")
    print(f"  for comparison, Vulkan cooperative matrix on this machine peaks at 3178 GFLOP/s\n")
    print("  %-12s %12s %14s %s" % ("block", "GFLOP/s", "max rel err", "of peak"))
    for block in args.blocks.split(","):
        rm, rn = (int(v) for v in block.split("x"))
        if M % (8 * rm) or N % (16 * rn):
            continue
        try:
            rate, error = run(M, N, K, rm, rn, source)
            print("  %-12s %12.1f %14.2e %6.1f%%" % (f"{8*rm}x{16*rn}", rate, error, 100 * rate / 32000))
        except RuntimeError as failure:
            print("  %-12s %s" % (f"{8*rm}x{16*rn}", str(failure).splitlines()[0][:90]))


if __name__ == "__main__":
    main()
