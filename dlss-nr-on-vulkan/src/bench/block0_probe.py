#!/usr/bin/env python3
"""Inside block 0: where two machines' first block parts, pass by pass.

`capture_compare.py` shows where two graphs part level by level, and with the float16
denorm mode the same on both sides Linux and Windows part in block 0 (notes/phase71). In
the graph block 0 is two passes — the fused feed-forward and the fused window block — and
nothing in between is stored. This runs block 0 on a capture's input twice:

- as the graph runs it, keeping the stem, its published half copy, the feed-forward's
  output and the block's (`stem`, `stem16`, `ffn`, `block0`);
- with every fusion off and every buffer its own (`NR_SCRATCH_ARENA=0`), keeping what each
  pass leaves: the published stem, the hidden layer, the projection before its residual,
  the feed-forward's output, the window partition, the QKV projection, Q, K and V, the
  scores, the probabilities, the context, the merged heads, the output projection and the
  block's output (`u.*`).

The fused passes were measured exact against the unfused ones, so the second run shows
what the first computes in between — if they agree on this machine, which the probe checks
and says. It saves both as `capture_compare.py` does, whose `--compare` walks them:

    python3 src/bench/block0_probe.py --features windows-capture-320x320.npz --save probe.npz
    python3 src/bench/capture_compare.py --compare linux-probe.npz windows-probe.npz
"""
import argparse
import hashlib
import json
import os
import pathlib
import sys

import numpy as np

# every scratch buffer its own allocation, so no pass's output is another's arena role
os.environ["NR_SCRATCH_ARENA"] = "0"

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))
sys.path.insert(0, str(ROOT / "src" / "gpu"))

UNFUSED = ("fuse_glue", "fuse_ffn", "fuse_residual", "fuse_partition", "qkv_epilogue",
           "fuse_window_attention", "fuse_attention_merge", "fuse_window_residual",
           "fuse_window_block", "fuse_stem_ffn", "fuse_pool")


def run_block0(frame, keep_unfused):
    """Stem and block 0, one submit, and what the run left in each buffer it wrote."""
    import nr_resident as R
    import xmxres
    rt = frame.rt
    height, width = frame.height, frame.width
    pixels = height * width
    block = frame.block(0, 1)
    s = frame.scratch(block, height, width)
    stem = frame.buffer("probe.stem", pixels * 32)
    out = frame.buffer("probe.block0", pixels * 32)
    rt.begin()
    if rt.fuse_glue:
        rt.gemm_dual(frame.input_buffer(), frame.adapter, stem, s.value16, pixels, 32, 16)
    else:
        rt.gemm(frame.input_buffer(), frame.adapter, stem, pixels, 32, 16)
    R.record_block(rt, block, s, source=stem, target=out,
                   source16=s.value16 if rt.fuse_glue else None)
    rt.submit()

    padded_height, padded_width, _ = rt.window_extent(height, width, block.origin)
    windows = (padded_height // 8) * (padded_width // 8)
    tokens, channels, heads = s.tokens, block.channels, block.heads
    windowed = windows * tokens * channels
    batch = windows * heads
    read = lambda buffer, count, dtype=np.float32: np.array(
        xmxres.host_view(buffer, dtype, count=count), np.float32)
    if not keep_unfused:
        return {"stem": read(stem, pixels * 32), "stem16": read(s.value16, pixels * 32, np.float16),
                "ffn": read(s.ffn, pixels * 32), "block0": read(out, pixels * 32)}
    return {"u.stem16": read(s.value16, pixels * 32, np.float16),
            "u.hidden16": read(s.hidden16, pixels * s.hidden_width, np.float16),
            "u.branch": read(s.branch, pixels * 32),
            "u.ffn": read(s.ffn, pixels * 32),
            "u.win16": read(s.win16, windowed, np.float16),
            "u.proj": read(s.proj, windowed * 3),
            "u.q16": read(s.q16, windowed, np.float16),
            "u.k16": read(s.k16, windowed, np.float16),
            "u.v16": read(s.v16, windowed, np.float16),
            "u.scores": read(s.scores, batch * tokens * tokens),
            "u.probs16": read(s.probs16, batch * tokens * tokens, np.float16),
            "u.context": read(s.context, batch * tokens * 32),
            "u.merged16": read(s.merged16, windowed, np.float16),
            "u.attended": read(s.attended, windowed),
            "u.block0": read(out, pixels * 32)}


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--features", type=pathlib.Path, required=True, metavar="NPZ",
                        help="a capture_compare.py capture whose input to run on")
    parser.add_argument("--save", type=pathlib.Path, required=True)
    args = parser.parse_args()
    import nr_frame
    other = np.load(args.features)
    if "features" not in other:
        raise SystemExit(f"{args.features} records no input; capture it again")
    features = np.ascontiguousarray(other["features"])
    size = json.loads(bytes(other["meta"]).decode())["size"]

    backend = nr_frame.ResidentBackend()
    frame = backend.frame(*features.shape[:2])
    if not frame.rt.input_fp16:
        raise SystemExit("the probe reads the half input; unset NR_INPUT_FP16")
    # the whole graph once: the weights go up and the features into the input buffer
    frame.run(features, execution="block")
    kept = run_block0(frame, keep_unfused=False)
    switches = {name: getattr(frame.rt, name) for name in UNFUSED}
    for name in UNFUSED:
        setattr(frame.rt, name, False)
    try:
        kept.update(run_block0(frame, keep_unfused=True))
    finally:
        for name, value in switches.items():
            setattr(frame.rt, name, value)
    frame.close()

    same = np.array_equal(kept["block0"].view(np.uint32), kept["u.block0"].view(np.uint32))
    print(f"  block 0 fused and unfused on this machine: "
          f"{'the same bits' if same else 'DIFFERENT - the unfused points do not show the fused path'}")
    if not same:
        differ = kept["block0"].view(np.uint32) != kept["u.block0"].view(np.uint32)
        print(f"    {int(differ.sum())} of {differ.size} values differ")
    order = list(kept)
    meta = {"size": size, "network": list(features.shape[:2]), "order": order,
            "head_sha256": None, "features_from": str(args.features.name),
            "features_sha256": hashlib.sha256(features.tobytes()).hexdigest(),
            "fused_equals_unfused": bool(same)}
    np.savez(args.save, meta=np.frombuffer(json.dumps(meta).encode(), np.uint8),
             features=features, **kept)
    print(f"  {args.save}: input {meta['features_sha256'][:16]}")
    for name in order:
        print(f"  {name:12} {kept[name].size:10d} "
              f"{hashlib.sha256(kept[name].tobytes()).hexdigest()[:16]}")


if __name__ == "__main__":
    main()
