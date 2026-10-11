#!/usr/bin/env python3
"""
Where along the graph ours and OpenDLSS-NR's part: one block at a time, on their inputs.

`opendlss_reference.py` compares the two heads; this finds which blocks the distance comes from.
The reference port records every block boundary (E4M3 bytes); each of our blocks — the numpy
reference, `nr_model` — is run on the boundary the reference fed its own copy of that block, and its
published output is compared byte for byte with the reference's. Arithmetic that differs only in
rounding leaves most bytes equal and the rest a step apart; a block computed differently shows far
more.

The decoder's first blocks take two inputs, the level below and the encoder skip, and here the two
graphs name different tensors as the skip: ours the output of the last block *before* each encoder
transition block (3, 7, 13, 21), theirs the transition block's own output (4, 8, 14, 22). Both are
tried against the reference's output.

    work/venv/bin/python src/bench/opendlss_blocks.py [--size 320x180] [--image IMAGE]
"""
import argparse
import pathlib
import sys

import numpy as np

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import opendlss_reference as reference   # noqa: E402

import image_io                            # noqa: E402
import nr_frame                            # noqa: E402
import nr_model as M                       # noqa: E402


def their_levels(width, height):
    """The six pooling levels of the reference's geometry, as (height, width), and the field."""
    field = reference.their_field(width, height)
    levels, w, h = [], *field
    for _ in range(6):
        w, h = -(-((w + 1) // 2) // 4) * 4, -(-((h + 1) // 2) // 4) * 4
        levels.append((h, w))
    return field, levels


def compare(ours, theirs):
    """(share of equal values, mean |d| / mean |theirs|) of two published tensors."""
    a, b = np.asarray(ours, np.float32).ravel(), np.asarray(theirs, np.float32).ravel()
    equal = float(np.mean(a == b))
    return equal, float(np.abs(a - b).mean() / max(np.abs(b).mean(), 1e-12))


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--size", default="320x180")
    parser.add_argument("--image", default=str(reference.ROOT / "pngs" / "Cyberpunk-2077_01.jpg"))
    args = parser.parse_args()
    width, height = (int(v) for v in args.size.split("x"))
    field, levels = their_levels(width, height)
    geometry = nr_frame.network_geometry(width, height)
    if (geometry.network_width, geometry.network_height) != field:
        raise SystemExit(f"{args.size}: our field is not the reference's {field}")

    import nr_daemon
    colour = nr_daemon.resample(np.asarray(image_io.load(args.image), np.float32), (height, width))
    features = nr_frame.build_features(colour, geometry=geometry, **nr_frame.PROFILES["standard"])
    head, result, bounds = reference.reference_run(features, width, height, capture=True)
    print(f"{args.size} on a {field[0]}x{field[1]} field; reference on {result['adapter']}, "
          f"{len(bounds)} boundaries")

    def boundary(name, shape):
        values = bounds[name]
        return values.reshape(1, *shape, values.shape[1])

    model = M.NeuralRenderingModel.from_safetensors(nr_frame.WEIGHTS)
    report = []

    def check(name, ours, theirs):
        equal, relative = compare(M.e4m3(ours), theirs)
        report.append((name, equal, relative))
        print(f"  {name:34s} {100 * equal:6.2f} % equal   mean |d| {100 * relative:6.2f} % of the value",
              flush=True)

    full = (field[1], field[0])
    level = [(h, w) for h, w in levels]
    # block 0, from the features
    adapter = model.weight("block0.layer0.input_adapter_weight")
    value = M._per_token(lambda tokens: M.matmul(tokens, adapter), features[None].astype(np.float32))
    block0 = model._window(value, 0, head_count=1)
    check("block 0 (from the features)", block0, boundary("block-0", full))

    # the encoder: 32 channels at level 0, then 64, 128, 256
    stages = [(0, range(1, 5), 1, "transition-0-1"), (1, range(5, 9), 2, "transition-4-5"),
              (2, range(9, 15), 4, "transition-8-9"), (3, range(15, 23), 8, "transition-14-15")]
    for index, blocks, heads, entry in stages:
        shape = level[index]
        previous = entry
        for block in blocks:
            source = boundary(previous, shape)
            check(f"block {block} ({heads} head{'s' if heads > 1 else ''})",
                  model._window(source, block, head_count=heads), boundary(f"block-{block}", shape))
            previous = f"block-{block}"

    # the 512 stage, the ViT and the decoder's input merge
    shape = level[4]
    previous = "transition-22-23"
    for block in range(23, 31):
        check(f"block {block} (512, split)", model._split_window(boundary(previous, shape), block),
              boundary(f"block-{block}", shape))
        previous = f"block-{block}"
    tokens = level[5]
    for block in range(32, 39):
        check(f"block {block} (ViT)", model._global(boundary(f"block-{block - 1}", tokens), block),
              boundary(f"block-{block}", tokens))
    merged = M.decoder_input_merge(
        M.matmul(boundary("block-38", tokens), model.weight("block39.layer0.conv_weight")),
        skip=boundary("block-30", shape), skip_sine=model.weight("block39.layer0.inp_upsample_sin"))
    check("block 39 (decoder input merge)", merged, boundary("block-39", shape))
    previous = "block-39"
    for block in range(40, 48):
        check(f"block {block} (512, split)", model._split_window(boundary(previous, shape), block),
              boundary(f"block-{block}", shape))
        previous = f"block-{block}"

    # the decoder: each stage's first block merges the level below with an encoder skip
    stages = [(3, 48, range(49, 56), 8, (21, 22)), (2, 56, range(57, 62), 4, (13, 14)),
              (1, 62, range(63, 66), 2, (7, 8)), (0, 66, range(67, 70), 1, (3, 4))]
    below = "block-47"
    for index, first, blocks, heads, skips in stages:
        shape = level[index]
        low = boundary(below, level[index + 1] if index < 4 else level[4])
        for skip in skips:
            ours = model._upsample_window(low, boundary(f"block-{skip}", shape), first, head_count=heads)
            check(f"block {first} (skip = block {skip})", ours, boundary(f"block-{first}", shape))
        previous = f"block-{first}"
        for block in blocks:
            check(f"block {block} ({heads} head{'s' if heads > 1 else ''})",
                  model._window(boundary(previous, shape), block, head_count=heads),
                  boundary(f"block-{block}", shape))
            previous = f"block-{block}"
        below = previous


if __name__ == "__main__":
    main()
