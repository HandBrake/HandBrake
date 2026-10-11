#!/usr/bin/env python3
"""Packed staged loads must preserve every output bit and alignment fallback.

Use separate processes because the loader specialization is fixed at device
creation. Compare the old and packed loaders with the same installed shaders,
changing inputs, all simple epilogues, batches, partial tiles and output guards.

On every runtime: libxmx's staged kernel (constant 2), libmetalmx's (function constant 3)
and, where there is no staged kernel -- Metal or Vulkan without matrix units, Direct3D 12 --
the portable GEMM's packed A fetch (XMX_PORTABLE_PACKED, off by default and turned on here
beside the staged switch), which is what the shapes below then exercise; there the
staged-kernel checks at the end do not apply.
"""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src"))
import nr_build  # noqa: E402

FILL = -123.125
GUARD = 64


def worker(destination):
    import xmxres as X

    rt = X.Runtime()
    X.profile(True)
    # a staged kernel to route to: not without matrix units, nor on Direct3D 12
    staged = not rt.lib.xmx_portable() and bool(rt.lib.xmx_window_gather())
    # the switch the worker was started under must be the one the runtime took
    want = int(os.environ.get("XMX_STAGED_PACKED", "1"))
    if staged:
        assert rt.lib.xmx_staged_packed() == want, (rt.lib.xmx_staged_packed(), want)
    else:
        want = int(os.environ.get("XMX_PORTABLE_PACKED", "0"))
        assert rt.lib.xmx_portable_packed() == want, (rt.lib.xmx_portable_packed(), want)
    rng = np.random.default_rng(81723)
    results = {}
    families = set()
    # m, n, k, batch, A/B row padding, A/B offsets, A/B batch padding.
    # Four-half offsets and strides are legal for the old 64-bit loader, but
    # must not reach the packed 128-bit loader. Odd values take the scalar path.
    cases = (
        (128, 128, 128, 1, (0, 0), (0, 0), (0, 0)),
        (88, 64, 256, 1, (0, 0), (0, 0), (0, 0)),
        (32, 1056, 128, 1, (0, 0), (0, 0), (0, 0)),
        (24, 128, 96, 1, (0, 0), (0, 0), (0, 0)),
        (32, 128, 1024, 1, (0, 0), (0, 0), (0, 0)),
        (64, 128, 1024, 1, (0, 0), (0, 0), (0, 0)),
        (96, 64, 96, 3, (4, 4), (0, 0), (0, 0)),
        (88, 64, 128, 2, (0, 0), (4, 0), (0, 0)),
        (24, 128, 96, 2, (0, 0), (0, 4), (0, 0)),
        (32, 128, 1024, 3, (0, 0), (0, 0), (4, 4)),
        (88, 64, 128, 3, (0, 4), (0, 0), (4, 0)),
        (96, 64, 96, 2, (7, 5), (1, 3), (16, 16)),
    )
    for specialization in (0, 7):
        rt.specialize(specialization)
        for case, (m, n, k, batch, padding, offsets, gaps) in enumerate(cases):
            for transposed in (False, True):
                lda = k + padding[0]
                ldb = (k if transposed else n) + padding[1]
                ldc = n + 13
                sa = m * lda + gaps[0]
                sb = (n if transposed else k) * ldb + gaps[1]
                sc = m * ldc + 16
                oa, ob = offsets
                oc = 5
                a = rt.buffer(batch * sa + GUARD, np.float16)
                b = rt.buffer(batch * sb + GUARD, np.float16)
                count = batch * sc + GUARD
                outputs = [rt.buffer(count, dtype) for dtype in (np.float32, np.float16)]
                written = np.zeros(count, bool)
                for item in range(batch):
                    for row in range(m):
                        start = oc + item * sc + row * ldc
                        written[start:start + n] = True
                try:
                    for change in range(2):
                        for buf, size in ((a, batch * sa + GUARD), (b, batch * sb + GUARD)):
                            X.host_write(buf, rng.normal(0, .2 + .1 * change, size).astype(np.float16))
                        for epilogue in range(5):
                            for narrow, c in enumerate(outputs):
                                dtype = np.float16 if narrow else np.float32
                                X.host_write(c, np.full(count, FILL, dtype))
                                X.profile_reset()
                                rt.begin()
                                rt.gemm(a, b, c, m, n, k, batch=batch, transpose_b=transposed,
                                        leading=(lda, ldb, ldc), strides=(sa, sb, sc),
                                        offsets=(oa, ob, oc), epilogue=epilogue, narrow=bool(narrow))
                                rt.submit()
                                families.update(int(kind) // 32 for kind in X.profile_each_kinds())
                                values = X.host_view(c, dtype).copy()
                                key = f'{specialization}/{case}/{int(transposed)}/{change}/{epilogue}/{narrow}'
                                assert np.all(values[~written] == FILL), (key, 'guard overwritten')
                                assert np.isfinite(values[written]).all(), (key, 'nonfinite output')
                                assert np.all(values[written] != FILL), (key, 'unwritten output')
                                results[key] = values.view(np.uint8)
                finally:
                    for buf in (a, b, *outputs):
                        buf.free()
        print(f'mask {specialization}: {len(results)} outputs, guards intact', flush=True)
    if staged:
        assert families == {2}, f'calls bypassed the staged kernel: {families}'
        assert rt.lib.xmx_staged32_calls() > 0, '32-row variants were not exercised'
    else:
        print('no staged kernel on this runtime: the portable GEMM\'s packed A fetch was '
              f'under test (pass families {sorted(families)})', flush=True)
    np.savez(destination, **results)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--worker', type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.worker:
        worker(args.worker)
        return
    nr_build.BUILD_DIR.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='staged-packed-', dir=nr_build.BUILD_DIR) as folder:
        archives = [Path(folder) / f'{mode}.npz' for mode in (0, 1)]
        for mode, archive in enumerate(archives):
            env = dict(os.environ)
            for name in ('XMX_STAGED_SPV', 'XMX_STAGED32_SPV', 'XMX_STAGED32_DEEP_SPV'):
                env.pop(name, None)
            env.update(XMX_STAGED_PACKED=str(mode), XMX_PORTABLE_PACKED=str(mode), XMX_STAGE_K='32',
                       XMX_STAGED32='1', XMX_STAGED_PARTIAL='1', PYTHONUTF8='1',
                       NR_BUILD_DIR=str(nr_build.BUILD_DIR))
            subprocess.run([sys.executable, str(Path(__file__).resolve()), '--worker', str(archive)],
                           env=env, cwd=ROOT, check=True, timeout=120)
        with np.load(archives[0]) as old, np.load(archives[1]) as packed:
            assert old.files == packed.files
            for name in old.files:
                np.testing.assert_array_equal(old[name], packed[name], err_msg=name)
            print(f'{len(old.files)} packed-loader outputs byte-identical')


if __name__ == '__main__':
    main()
