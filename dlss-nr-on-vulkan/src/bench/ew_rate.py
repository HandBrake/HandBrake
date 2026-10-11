#!/usr/bin/env python3
"""Achieved bandwidth of the elementwise passes at block 0's full-resolution size."""
import pathlib, sys, time
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu"))
import xmxres
rt = xmxres.Runtime()

PIX, C, HEADS, TOK = 983040, 32, 1, 64
big32 = [rt.buffer(PIX * 96) for _ in range(3)]
big16 = [rt.buffer(PIX * 96, np.float16) for _ in range(2)]
small = rt.buffer(4096)

def bench(name, record, elements, per_elem, calls=8):
    best = 1e9
    for repeat in range(3):
        rt.begin()
        for _ in range(calls):
            record()
        t = time.perf_counter(); rt.submit()
        if repeat: best = min(best, time.perf_counter() - t)
    gb = calls * elements * per_elem / 1e9
    print("  %-34s %8.2f ms %9.1f GB/s  %6.1f Melem/ms"
          % (name, 1000 * best, gb / best, calls * elements / 1e6 / (1000 * best)))

a, b, c = big32
h0, h1 = big16
bench("to_half            (4+2)", lambda: rt.to_half(a, h0, PIX * C), PIX * C, 6)
bench("e4m3               (4+4)", lambda: rt.e4m3(a, b, PIX * C), PIX * C, 8)
bench("e4m3_half          (4+2)", lambda: rt.e4m3_half(a, h0, PIX * C), PIX * C, 6)
bench("residual          (12)  ", lambda: rt.residual(a, b, small, c, PIX * C, C), PIX * C, 12)
bench("add_bias          (12)  ", lambda: rt.add_bias(a, small, b, PIX * TOK, TOK, HEADS), PIX * TOK, 12)
bench("partition -> half  (4+2)", lambda: rt.partition(a, h0, 768, 1280, C, narrow=True), PIX * C, 6)
bench("reverse            (4+4)", lambda: rt.reverse(a, b, 768, 1280, C), PIX * C, 8)
bench("split_heads        (4+4)", lambda: rt.split_heads(a, b, PIX // 64, 64, C, HEADS, 0), PIX * C, 8)
bench("merge_heads -> half(4+2)", lambda: rt.merge_heads(a, h0, PIX // 64, 64, C, HEADS,
                                                         epilogue=1, narrow=True), PIX * C, 6)
bench("cosine_publish     (4+2)", lambda: rt.cosine_publish(a, h0, PIX * C // 32,
                                                            tokens=64, heads=1, narrow=True), PIX * C, 6)
bench("softmax            (8+2)", lambda: rt.softmax(a, h0, PIX, 64, narrow=True), PIX * 64, 10, calls=2)
