#!/usr/bin/env python3
"""A bottleneck block's attention in one pass, against the four passes it replaces.

`global_attention` must write what QK^T, the ViT's softmax (`vit_softmax`), PV and the
scaled merge_heads write, byte for byte: whole and partial 64-row blocks, token counts below
the rows — whose keys past the tokens are padding whatever their rows hold — an odd one,
and nothing written past the output. And both must be the ViT's attention as the numpy
reference computes it (`nr_model.vit_attend`), up to the GEMMs' own rounding: the scores
and the value sums there are float32 products summed in another order.
"""
import pathlib
import sys

import numpy as np
import xmxres as X

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "ref"))
import nr_model   # noqa: E402

GUARD = 64
FILL16 = np.array([0x7E5A], np.uint16).view(np.float16)[0]


def main():
    rt = X.Runtime()
    rng = np.random.default_rng(2610)
    cases, worst = 0, 1.0
    # the graph's own shapes (32 heads: 320x320, 1280x768 and a 128x128 network's 16
    # tokens on 32 rows) among partial blocks, odd token counts and a row count under 64
    for rows, tokens, heads in ((64, 64, 32), (256, 240, 32), (32, 16, 32), (96, 90, 8),
                                (112, 105, 4), (256, 256, 8), (640, 640, 4), (32, 17, 2)):
        channels = heads * 32
        buffers = []

        def alloc(n, dtype):
            b = rt.buffer(n, dtype)
            buffers.append(b)
            return b
        try:
            q, k, v = (alloc(heads * rows * 32, np.float16) for _ in range(3))
            scores = alloc(heads * rows * rows, np.float32)
            probs = alloc(heads * rows * rows, np.float16)
            reciprocal = alloc(heads * rows, np.float32)
            context = alloc(heads * rows * 32, np.float32)
            for spread in (0.2, 1.0):
                # Q, K and V as the QKV epilogue leaves them: published, Q scaled. The pad
                # rows hold values too, which the tokens' count must keep out.
                host = {}
                for name, buf, scale in (("q", q, 6.0), ("k", k, 1.0), ("v", v, 1.0)):
                    values = nr_model.e4m3(rng.normal(0, spread, heads * rows * 32)
                                           * scale / 5.0).astype(np.float16)
                    X.host_write(buf, values)
                    host[name] = values.astype(np.float32).reshape(heads, rows, 32)
                for mask in (0, 7):
                    rt.specialize(mask)
                    outs = []
                    for fused in (False, True):
                        merged = alloc(rows * channels + GUARD, np.float16)
                        X.host_write(merged, np.full(rows * channels + GUARD, FILL16, np.float16))
                        rt.begin()
                        if fused:
                            rt.global_attention(q, k, v, merged, rows, tokens, heads)
                        else:
                            rt.gemm(q, k, scores, rows, rows, 32, batch=heads,
                                    strides=(rows * 32, rows * 32, rows * rows), transpose_b=True)
                            rt.vit_softmax(scores, probs, reciprocal, heads * rows, tokens,
                                           stride=rows)
                            rt.gemm(probs, v, context, rows, 32, rows, batch=heads,
                                    strides=(rows * rows, rows * 32, rows * 32))
                            rt.merge_heads(context, merged, 1, rows, channels, heads,
                                           epilogue=X.EPI_E4M3, narrow=True, scale=reciprocal)
                        rt.submit()
                        outs.append(X.host_view(merged, np.float16).copy())
                    n = rows * channels
                    name = f"{rows} rows, {tokens} tokens, {heads} heads, spread {spread}, mask {mask}"
                    want, got = outs[0][:n].view(np.uint16), outs[1][:n].view(np.uint16)
                    if not np.array_equal(got, want):
                        bad = np.flatnonzero(got != want)
                        raise AssertionError(f"{name}: {bad.size} of {n} differ, first at {bad[0]}")
                    assert (outs[1][n:].view(np.uint16) == np.uint16(0x7E5A)).all(), f"{name}: written past"
                    cases += 1
                # the semantics, on the real rows
                reference = nr_model.vit_attend(host["q"][:, :tokens], host["k"][:, :tokens],
                                                host["v"][:, :tokens])
                reference = reference.transpose(1, 0, 2).reshape(tokens, channels)
                got = outs[1][:rows * channels].astype(np.float32).reshape(rows, channels)[:tokens]
                equal = float(np.mean(got == reference))
                worst = min(worst, equal)
                if equal < 0.98:
                    raise AssertionError(f"{rows} rows, {tokens} tokens: only {equal:.4f} of the "
                                         "values are the numpy reference's")
        finally:
            for b in buffers:
                b.free()
    print(f"global attention: {cases} cases bit-identical to the four passes it replaces, and "
          f"at least {100 * worst:.2f} % of values the numpy reference's; staging={rt.staging}")


if __name__ == "__main__":
    main()
