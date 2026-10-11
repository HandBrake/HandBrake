#!/usr/bin/env python3
"""Group addressing without a GPU; --gpu also checks all three GEMM tile paths."""
import argparse
import contextlib
from types import SimpleNamespace

import numpy as np

import nr_resident as R
import xmxres


def layouts(groups):
    channels = groups * 32
    yield 128, channels, (0, 0, groups * 128), (0, channels * 128, 128), xmxres.EPI_GATE_E4M3
    yield 32, 128, (groups * 128, 0, channels), (128, 128 * 32, 32), xmxres.EPI_E4M3
    channels = groups * 64
    yield 256, 64, (channels, 0, groups * 256), (64, 64 * 256, 256), xmxres.EPI_GATE
    yield 64, 256, (groups * 256, 0, channels), (256, 256 * 64, 64), xmxres.EPI_E4M3


class Recorder:
    def __init__(self, batch):
        self.batch_ffn = batch
        # This counts FFN GEMMs to measure batching alone. The fused residual folds the
        # closing projection's residual into that GEMM and changes nothing it counts,
        # so it is pinned off here and measured by test_gemm_residual.py instead.
        self.fuse_residual = False
        # likewise the fused narrow feed-forward, one pass where this counts two GEMMs
        self.fuse_ffn = False
        self.fuse_branched_ffn = False
        self.fuse_partition = False
        self.fuse_transition = False
        self.fuse_merge_ffn = False
        self.fuse_stem_ffn = False
        self.fuse_pool = False
        self.fuse_window_block = False
        self.fuse_head = False
        self.fuse_global_attention = False
        self.calls = []

    def independent(self):
        return contextlib.nullcontext()

    def gemm(self, a, b, c, m, n, k, **kw):
        self.calls.append((m, n, k, kw))

    def to_half(self, *args, **kwargs):
        pass

    def residual(self, *args, **kwargs):
        pass


def addresses(call):
    m, n, k, kw = call
    lda, ldb, ldc = kw['leading']
    strides = kw.get('strides', (0, 0, 0))
    offsets = kw.get('offsets', (0, 0, 0))
    for group in range(kw.get('batch', 1)):
        yield tuple(offset + group * step + np.arange(rows)[:, None] * ld
                    + np.arange(cols)[None, :]
                    for offset, step, rows, cols, ld in zip(
                        offsets, strides, (m, k, m), (k, n, n),
                        (lda or k, ldb or n, ldc or n)))


def host_checks():
    cases = 0
    for groups in (1, 2, 4, 8):
        for n, k, leading, strides, epilogue in layouts(groups):
            recordings = []
            for mode in (False, True):
                rt = Recorder(mode)
                R._ffn_groups(rt, None, None, None, 8, n, k, groups,
                              leading=leading, strides=strides, epilogue=epilogue)
                assert len(rt.calls) == (1 if mode else groups)
                assert all(c[3]['epilogue'] == epilogue and c[3]['narrow'] for c in rt.calls)
                recordings.append([a for call in rt.calls for a in addresses(call)])
            written = []
            for old, new in zip(*recordings):
                for before, after in zip(old, new):
                    np.testing.assert_array_equal(before, after)
                written.extend(new[2].reshape(-1).tolist())
            # The groups cover the entire interleaved output exactly once: no races,
            # holes or writes outside the logical output.
            np.testing.assert_array_equal(np.sort(written), np.arange(8 * n * groups))
            cases += 1
    # A toggle must invalidate the captured graph for every existing specialization
    # and Q/K fusion combination, not reuse commands from the other FFN mode. Every
    # combination is walked, one switch a level, and each key marked in a bitmap: a set
    # of 2^24 Python ints was a gigabyte.
    switches = ("fuse_qk", "batch_ffn", "input_fp16", "compact_head", "joint_qkv",
                # the fusions take bits 8 and up. ProjectsCodex's own 5-9 would land on
                # input_fp16, compact_head, joint_qkv and the residuals
                "fuse_residual", "fuse_window_residual", "fuse_window_attention",
                "fuse_attention_merge", "qkv_epilogue", "fuse_glue", "fuse_ffn",
                "fuse_branched_ffn", "fuse_partition", "fuse_transition", "fuse_merge_ffn",
                "fuse_stem_ffn", "fuse_pool", "fuse_window_block", "fuse_head",
                "fuse_global_attention")
    rt = object.__new__(xmxres.Runtime)
    seen = bytearray(1 << 22)
    walked = 0

    def walk(level):
        nonlocal walked
        if level == len(switches):
            key = rt.graph_key()
            assert not seen[key >> 3] & (1 << (key & 7)), f"graph key collides: {key:#x}"
            seen[key >> 3] |= 1 << (key & 7)
            walked += 1
            return
        for value in (False, True):
            setattr(rt, switches[level], value)
            walk(level + 1)

    for mask in range(8):
        rt.lib = SimpleNamespace(xmx_specialization=lambda: mask)
        walk(0)
    assert walked == 8 << len(switches), walked
    # Exercise the production recorders too, not just the batching helper. Multiplicity
    # is the current 71-block model's grouped FFN inventory; no weights are needed.
    savings = 0
    for split, groups, blocks in ((False, 2, 8), (False, 4, 12),
                                  (False, 8, 16), (True, 8, 16)):
        w = SimpleNamespace(groups=groups, channels=groups * (64 if split else 32),
                            branched=True, **{n: n for n in
                            ('expand', 'branch', 'ffn_out', 'ffn_cos', 'first', 'project', 'weight3')})
        s = SimpleNamespace(height=8, width=1, hidden_width=groups * 128,
                            **{n: n for n in
                            ('value16', 'hidden16', 'heads16', 'branch', 'ffn', 'core16')})
        counts = []
        for mode in (False, True):
            rt = Recorder(mode)
            (R.record_split_feed_forward if split else R.record_feed_forward)(
                rt, w, s, 'source', source_half=True)
            counts.append(len(rt.calls))
        assert counts == [2 * groups + (2 if split else 1), 4 if split else 3]
        savings += blocks * (counts[0] - counts[1])
    assert savings == 536
    print(f'FFN batching: {cases} address layouts, disjoint complete outputs, graph keys, '
          f'production FFN reduction {savings} OK')


def gpu_checks():
    rt = xmxres.Runtime()
    rng = np.random.default_rng(67)
    cases = 0
    # Default dispatch selection: 8 rows uses the base kernel, 16 tiled, 64 staged
    # where K >= 128. Run without XMX_TILE/XMX_STAGE overrides to exercise those paths.
    for rows in (8, 16, 64):
        for groups in (1, 2, 4, 8):
            for n, k, leading, strides, epilogue in layouts(groups):
                dummy = (rows, n, k, dict(leading=leading, strides=strides, batch=groups))
                indices = list(addresses(dummy))
                sizes = [max(int(group[j].max()) for group in indices) + 1 for j in range(3)]
                a = rt.buffer_from((rng.standard_normal(sizes[0]) * .2).astype(np.float16), np.float16)
                b = rt.buffer_from((rng.standard_normal(sizes[1]) * .2).astype(np.float16), np.float16)
                out = rt.buffer(sizes[2] + 64, np.float16)
                try:
                    expected = None
                    for rt.batch_ffn in (False, True):
                        xmxres.host_write(out, np.full(sizes[2] + 64, -11, np.float16))
                        rt.begin()
                        R._ffn_groups(rt, a, b, out, rows, n, k, groups,
                                      leading=leading, strides=strides, epilogue=epilogue)
                        rt.submit()
                        got = xmxres.host_view(out, np.float16).copy()
                        np.testing.assert_array_equal(got[sizes[2]:], -11)
                        if expected is None:
                            expected = got
                        else:
                            np.testing.assert_array_equal(got, expected)
                    cases += 1
                finally:
                    for buffer in (a, b, out):
                        buffer.free()
    print(f'FFN batching: {cases} GPU comparisons bit-identical, output guards intact; '
          f'staging={rt.staging}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gpu', action='store_true')
    args = parser.parse_args()
    host_checks()
    if args.gpu:
        gpu_checks()
