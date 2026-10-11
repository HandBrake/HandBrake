#!/usr/bin/env python3
"""
int8_bottleneck — what it costs to put the eight global blocks through integer weights.

`notes/phase23-integer-weights.md` asked whether the model could use the XMX integer
path and answered no, on two grounds: cooperative-matrix config 4 is
`sint8 x sint8 -> sint32`, so integer weights force integer *activations*, and the
published activations have up to 180 000x of dynamic range, which an int8 grid cannot
hold. Both are true — of the **whole** graph. Neither was measured for a part of it.

Blocks 31-38, the ViT-1D bottleneck, are a different proposition:

  - they hold **100 663 296 of 145 755 123 parameters, 69.1 %** of the model;
  - they run at the deepest level, where `src/bench/int_activations.py` measures int8 at
    1.59 % relative error — four times finer than the 6.25 % E4M3 step the graph is
    already built around, and not the 32 % that level l1 suffers;
  - their GEMMs are the small-M, large-matrix shapes where the integer path's doubled K
    (32 against 16) removes the most instructions.

This measures the damage that trade actually does, on real frames, in the metric this
project judges by: not the head's standard deviation, but the size of the effect the
network is there to produce.

    python3 src/bench/int8_bottleneck.py                       # every frame it can find
    python3 src/bench/int8_bottleneck.py IMAGE [IMAGE ...]
    python3 src/bench/int8_bottleneck.py --write-crops         # also the worst-pixel crops

**Weights only** by default. A real config-4 kernel also quantises the activations, and
`--activations` simulates that half too: before each of the four weight GEMMs of blocks
31-38 the frame stops, and the host rounds the GEMM's A operand onto an int8 grid per row
(per token) and back — what `int8_quant.quantise(a, axis=1)` feeds the integer kernel.
The dequantised values are rounded to half on the way back, 2^-11 against the grid's
2^-8, so this is the integer kernel's answer to within an eighth of its own error, at any
extent. `--size WxH` resamples each frame first, to the network extent a live game sees
(640x360 at render scale 0.5 is a 320x180 frame on a 320x320 network).
"""
import argparse
import os
import pathlib
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "src" / "ref"), str(ROOT / "src" / "gpu")]
import image_io                                                        # noqa: E402
import nr_frame                                                        # noqa: E402
import nr_resident                                                     # noqa: E402
import xmxres                                                          # noqa: E402

GLOBAL_BLOCKS = tuple(f"block{i}." for i in range(31, 39))
LIMIT = 127                          # symmetric int8: -128..127, the negative end unused
MODEL_PARAMETERS = 145_755_123
THRESHOLDS = (1, 2, 4, 8)


def quantise_global_blocks(weights):
    """Round every 2-D weight of blocks 31-38 onto an int8 grid and back.

    Per output channel, which is what a real kernel does: the int32 accumulator is
    scaled by `s_a[row] * s_b[column]` afterwards, so a scale per column is free. The
    1-D tensors — the gates, `attn_scale`, the transition scalars — are not GEMM
    operands and are left exactly alone, as `phase23` left them.
    """
    matrices = elements = 0
    for name in list(weights):
        if not name.startswith(GLOBAL_BLOCKS):
            continue
        value = np.asarray(weights[name])
        if value.ndim != 2:
            continue
        wide = value.astype(np.float32)
        scale = np.max(np.abs(wide), axis=0, keepdims=True) / LIMIT
        scale = np.where(scale == 0, 1.0, scale)
        weights[name] = (np.round(wide / scale).clip(-128, LIMIT) * scale).astype(value.dtype)
        matrices += 1
        elements += wide.size
    return matrices, elements


def quantise_rows(buffer, rows, width):
    """Round a (rows, width) half buffer onto an int8 grid per row and back, in place."""
    view = xmxres.host_view(buffer, np.float16)[:rows * width].reshape(rows, width)
    wide = view.astype(np.float32)
    peak = np.max(np.abs(wide), axis=1, keepdims=True)
    scale = np.where(peak == 0, 1.0, peak / LIMIT)
    view[...] = (np.round(wide / scale).clip(-LIMIT, LIMIT) * scale).astype(np.float16)


OPERANDS = [0]
_RECORD_GLOBAL_BLOCK = nr_resident.record_global_block


def global_block_int8(runtime, w, s, source=None, target=None, *, chain=False):
    """`record_global_block` with every weight GEMM's A operand on an int8 grid per row.

    The same passes in the same order; before each of the four weight GEMMs — the
    expand, the feed-forward projection, the QKV projection and the output projection —
    the frame is submitted, the operand rounded on the host, and recording resumed. The
    attention GEMMs multiply activations by activations and stay in half, as a kernel
    that put only the weight GEMMs on config 4 would leave them. The residuals read the
    values the GEMMs' operands were copied from, not the rounded copies.
    """
    R = nr_resident

    def int8_rows(buffer, width):
        # the scratch plan's discovery records the frame once and never runs it
        if getattr(buffer, "_stand_ins", [None])[0] is not None:
            return
        runtime.submit()
        quantise_rows(buffer, s.padded, width)
        OPERANDS[0] += 1
        runtime.begin()

    def half_copy(source16, target16, count):
        runtime.unary(xmxres.E4M3, source16, target16, count, a_half=True, narrow=True)

    channels, heads, padded = w.channels, w.heads, s.padded
    if chain:
        source = s.io16
        half_copy(source, s.value16, padded * channels)
    else:
        source = source or s.value
        runtime.to_half(source, s.value16, padded * channels)
    int8_rows(s.value16, channels)
    runtime.gemm(s.value16, w.expand, s.hidden16, padded, w.hidden_width, channels,
                 epilogue=xmxres.EPI_GATE_E4M3, narrow=True)
    int8_rows(s.hidden16, w.hidden_width)
    R.record_project_residual(runtime, s.hidden16, w.ffn_proj, s.branch, source, w.ffn_cos,
                              s.ffn, padded, channels, w.hidden_width, skip_half=chain,
                              epilogue=xmxres.EPI_E4M3, narrow=True)
    half_copy(s.ffn, s.ffn16, padded * channels)
    int8_rows(s.ffn16, channels)
    key = R.record_qkv_projection(runtime, s.ffn16, w, s, 1, padded, channels, heads, vit=True)
    runtime.gemm(s.q16, key, s.scores, padded, padded, 32, batch=heads,
                 strides=(padded * 32, padded * 32, padded * padded), transpose_b=True)
    runtime.vit_softmax(s.scores, s.probs16, s.reciprocal, heads * padded, s.tokens,
                        stride=padded)
    runtime.gemm(s.probs16, s.v16, s.context, padded, 32, padded, batch=heads,
                 strides=(padded * padded, padded * 32, padded * 32))
    runtime.merge_heads(s.context, s.merged16, 1, padded, channels, heads,
                        epilogue=xmxres.EPI_E4M3, narrow=True, scale=s.reciprocal)
    int8_rows(s.merged16, channels)
    if chain:
        R.record_project_residual(runtime, s.merged16, w.out, s.attention, s.ffn, w.attn_cos,
                                  s.io16, s.tokens, channels, channels,
                                  epilogue=xmxres.EPI_E4M3, narrow=True, skip_half=True)
    else:
        R.record_project_residual(runtime, s.merged16, w.out, s.attention, s.ffn, w.attn_cos,
                                  target or s.out, padded, channels, channels, skip_half=True)


def render(colour, weights, activations):
    # the operands are rounded between submits, which a replayed graph has none of
    os.environ["NR_FRAME_MODE"] = "block" if activations else os.environ.get("NR_FRAME_MODE", "replay")
    backend = nr_frame.ResidentBackend()
    OPERANDS[0] = 0
    nr_resident.record_global_block = (global_block_int8 if activations
                                       else _RECORD_GLOBAL_BLOCK)
    try:
        counts = quantise_global_blocks(backend.weights) if weights else (0, 0)
        head, _ = nr_frame.run_head(backend, colour)
        return np.asarray(nr_frame.compose(head, colour), np.float32), counts
    finally:
        nr_resident.record_global_block = _RECORD_GLOBAL_BLOCK
        backend.close()


def resampled(colour, size):
    """The frame at `size` (width, height), resampled the way the daemon does it."""
    sys.path.insert(0, str(ROOT / "src" / "layer"))
    import nr_daemon
    width, height = size
    return np.ascontiguousarray(nr_daemon.resample(colour, (height, width)), np.float32)


def frames(paths, size=None):
    for path in paths:
        path = pathlib.Path(path)
        if not path.exists():
            print(f"  {path.name}: not here — skipped, and a skip is not a pass")
            continue
        colour = image_io.load(str(path))[:, :, :3].astype(np.float32)
        yield path, (resampled(colour, size) if size else colour)


def measure(path, colour, write_crops, activations):
    reference, _ = render(colour, False, False)
    repeat, _ = render(colour, False, False)
    drift = float(np.abs(repeat - reference).max()) * 255
    quantised, (matrices, elements) = render(colour, True, activations)
    if not matrices:
        print(f"  {path.name}: nothing was quantised — the filter matched no weight")
        return None
    if activations and not OPERANDS[0]:
        print(f"  {path.name}: no activation was quantised — the hook never ran")
        return None

    effect = np.abs(reference - colour).max(axis=2) * 255
    damage = np.abs(quantised - reference).max(axis=2) * 255
    mean_effect = float((np.abs(reference - colour).mean()) * 255)
    mean_damage = float((np.abs(quantised - reference).mean()) * 255)
    row = dict(name=path.name, width=colour.shape[1], height=colour.shape[0],
               matrices=matrices, elements=elements, drift=drift, operands=OPERANDS[0],
               effect=mean_effect, damage=mean_damage,
               share=mean_damage / mean_effect * 100 if mean_effect else float("nan"),
               median=float(np.median(damage)), p99=float(np.percentile(damage, 99)),
               worst=float(damage.max()),
               over={t: (float((damage > t).mean() * 100), float((effect > t).mean() * 100))
                     for t in THRESHOLDS})
    if write_crops:
        y, x = np.unravel_index(int(np.argmax(damage)), damage.shape)
        top, left = max(0, y - 140), max(0, x - 140)
        cut = lambda a: a[top:top + 280, left:left + 280]                      # noqa: E731
        out = ROOT / "work" / f"int8_crop_{path.stem}.png"
        image_io.save(np.clip(np.concatenate([cut(colour), cut(reference), cut(quantised)],
                                             axis=1), 0, 1), str(out))
        row["crop"] = out.relative_to(ROOT)
    return row


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("images", nargs="*", help="frames to measure; default: pngs/ then work/")
    parser.add_argument("--write-crops", action="store_true",
                        help="save input | fp16 | int8 around each frame's worst pixel")
    parser.add_argument("--activations", action="store_true",
                        help="the weight GEMMs' activations on an int8 grid per row too")
    parser.add_argument("--size", metavar="WxH",
                        help="resample each frame to this extent first, as the daemon would")
    args = parser.parse_args()
    size = tuple(int(v) for v in args.size.split("x")) if args.size else None
    if args.activations:
        # the hook submits and resumes inside a block, which a captured graph cannot
        os.environ["NR_FRAME_MODE"] = "block"
    paths = args.images or sorted(
        p for p in list((ROOT / "pngs").glob("*.[jp][pn]g")) + [ROOT / "work" / "frame_in.png"]
        if p.exists())
    if not paths:
        print("no frames to measure — pass one; a skip is not a pass")
        return 0

    rows = [r for r in (measure(p, c, args.write_crops, args.activations)
                        for p, c in frames(paths, size)) if r]
    if not rows:
        return 1
    print(f"\n  {rows[0]['matrices']} matrices, {rows[0]['elements']:,} weights"
          f" ({rows[0]['elements'] / MODEL_PARAMETERS * 100:.1f} % of the model) on the int8 grid")
    if args.activations:
        print(f"  and {rows[0]['operands']} GEMM operands a frame on an int8 grid per row")
    print(f"\n  {'frame':26} {'extent':>11} {'effect':>8} {'damage':>8} {'share':>7}"
          f" {'median':>8} {'p99':>7} {'worst':>7} {'repeat':>8}")
    for r in rows:
        print(f"  {r['name'][:26]:26} {r['width']:5d}x{r['height']:<5d} {r['effect']:7.2f}L"
              f" {r['damage']:7.2f}L {r['share']:6.1f}% {r['median']:7.3f}L {r['p99']:6.2f}L"
              f" {r['worst']:6.1f}L {r['drift']:7.3f}L")
    print("\n  levels of 255. `repeat` is the same configuration rendered twice: it has to be"
          "\n  0.000, or the damage column is measuring the machine and not the quantiser.")
    for r in rows:
        print(f"\n  {r['name']} — where the damage sits")
        for t in THRESHOLDS:
            mine, theirs = r["over"][t]
            print(f"    above {t:2d} levels: int8 moves {mine:6.3f} % of pixels;"
                  f" the network itself moves {theirs:5.1f} %")
        if "crop" in r:
            print(f"    crop: {r['crop']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
