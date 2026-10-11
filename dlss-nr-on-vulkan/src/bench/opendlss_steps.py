#!/usr/bin/env python3
"""
Inside a block: each step of ours on the reference's own input to that step.

`opendlss_blocks.py` compares whole blocks, where four or five GEMMs' worth of rounding hides
what is structural. This compares steps — the feed-forward, the QKV projection, the attention,
the pool into the bottleneck and its projection, the feed-forwards of blocks 0, 66 and 70 on
their raw inputs — each run on the tensor the reference fed its own copy of that step, and
each both ways where MLX-DLSS's graph and the reference's differ. A step that differs only in
rounding agrees on most values; one that computes another thing agrees on almost none.
`notes/opendlss-reference.md`.

The reference records inside its blocks only with `opendlss_captures.patch` applied to its
clone, a capture-only change:

    git -C work/opendlss-nr apply src/bench/opendlss_captures.patch
    work/venv/bin/python src/bench/opendlss_steps.py [--size 320x180] [--image IMAGE]
"""
import argparse
import pathlib
import sys

import numpy as np

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import opendlss_reference as reference   # noqa: E402
from opendlss_blocks import their_levels  # noqa: E402

import image_io                            # noqa: E402
import nr_frame                            # noqa: E402
import nr_model as M                       # noqa: E402

H, E = M._half_rounded, M.e4m3


def compare(ours, theirs):
    a, b = np.asarray(ours, np.float32).ravel(), np.asarray(theirs, np.float32).ravel()
    return float(np.mean(a == b)), float(np.abs(a - b).mean() / max(np.abs(b).mean(), 1e-12))


def report(label, ours, theirs):
    equal, relative = compare(ours, theirs)
    print(f"    {label:58s} {100 * equal:6.2f} % equal  mean |d| {100 * relative:6.3f} %",
          flush=True)


# -- the reference's own arithmetic where ours differs, for the attention's step --------------

def their_normalize(value):
    """The window blocks' cosine norm as the reference pairs it: fma(c, c, half(c+16)^2)."""
    h = H(value)

    def fma(a, b, c):
        return H(a * b + c)
    pairs = [H(fma(h[..., c], h[..., c], H(h[..., c + 16] * h[..., c + 16]))
               + fma(h[..., c + 8], h[..., c + 8], H(h[..., c + 24] * h[..., c + 24])))
             for c in range(8)]
    four = [H(pairs[c] + pairs[c + 4]) for c in range(4)]
    two = [H(four[c] + four[c + 2]) for c in range(2)]
    total = np.maximum(H(two[0] + two[1]), np.float32(np.float16(M.COSINE_NORM_FLOOR)))
    return H(h * H(np.float32(1) / np.sqrt(total))[..., None])


def _inverse_tiled(p):
    tile, within = p >> 4, p & 15
    return ((tile >> 1) * 4 + (within >> 2)) * 8 + (tile & 1) * 4 + (within & 3)


PHYSICAL = np.array([_inverse_tiled(p) for p in range(64)])


def their_softmax(value):
    """The window softmax with the reference's denominator: a half tree over the keys in
    the window's 4x4-tiled order, where ours sums in float32."""
    affine = np.clip(H(H(value) * np.float32(0.044921875) + np.float32(1.30078125)),
                     np.float32(1.03125), np.float32(1.5693359375))
    bits = affine.astype(np.float16).view(np.uint16).astype(np.uint32)
    weights = (((bits << np.uint32(5)) + np.uint32(0x8000)) & np.uint32(0xFFFF)).astype(
        np.uint16).view(np.float16).astype(np.float32)
    tiled = weights[..., PHYSICAL]

    def column(g):
        pairs = [H(tiled[..., g + 16 * j] + tiled[..., g + 16 * j + 8]) for j in range(4)]
        return H(H(H(pairs[0] + pairs[1]) + pairs[2]) + pairs[3])
    t = [column(g) for g in range(8)]
    total = H(H(H(H(t[0] + t[2]) + t[4]) + t[6]) + H(H(H(t[1] + t[3]) + t[5]) + t[7]))
    return E(H(weights * H(np.float32(1) / total)[..., None]))


# -- ours -------------------------------------------------------------------------------------

class Steps:
    def __init__(self, bounds, model, levels, field):
        self.bounds, self.model = bounds, model
        self.levels, self.field = levels, field

    def get(self, name, shape):
        values = self.bounds[name]
        return values.reshape(1, *shape, values.shape[-1])

    def feed_forward(self, index, value, skip):
        m, prefix = self.model, f"block{index}.layer0"
        if f"{prefix}.ffn_expand_weight" in m.weights:
            branch = M.branched_feed_forward(
                value, expansion_weight=m.weight(f"{prefix}.ffn_expand_weight"),
                branch_projection_weight=m.weight(f"{prefix}.ffn_branch_projection_weight"),
                output_projection_weight=m.weight(f"{prefix}.ffn_output_projection_weight"))
        else:
            branch = M.matmul(E(M.quadratic_gate_activation(
                M.matmul(value, m.weight(f"{prefix}.weight1")))), m.weight(f"{prefix}.weight2"))
        return M.cosine_residual(skip, branch, m.weight(f"{prefix}.ffn_cos_skip"))

    def attend(self, qkv, index, heads, *, normalize=None, softmax=None):
        """Our window attention from a projected (1, H, W, 3C) qkv: the attended values."""
        m, prefix = self.model, f"block{index}.layer0"
        bias = m.weight(f"{prefix}.attn_bias")
        if M.uses_fragment_swizzle(index, heads):
            bias = M.recover_attention_bias_layout(bias)

        def cosine(value, *, qkv_weight, attention_scale, attention_bias, projection_weight,
                   head_count, logit_cap=None, symmetric_logit_cap=False):
            batch, tokens, c3 = value.shape
            shape = (batch, tokens, head_count, c3 // 3 // head_count)
            q, k, v = (np.ascontiguousarray(x.reshape(shape).transpose(0, 2, 1, 3))
                       for x in np.split(value, 3, axis=-1))
            q, k, v = M.vendor_cosine_publish(q, attention_scale), M.vendor_cosine_publish(k), E(v)
            scores = M.matmul_nt(q, k) + attention_bias.reshape(1, head_count, tokens, tokens)
            out = M.matmul(M.vendor_approximate_softmax(scores), v).transpose(0, 2, 1, 3)
            return E(np.ascontiguousarray(out).reshape(batch, tokens, c3 // 3))

        saved = M.cosine_attention, M.vendor_approximate_softmax, M.vendor_cosine_normalize, M.CHUNK_TOKENS
        M.cosine_attention, M.CHUNK_TOKENS = cosine, 0
        if normalize is not None:
            M.vendor_cosine_normalize = normalize
        if softmax is not None:
            M.vendor_approximate_softmax = softmax
        try:
            return M.window_attention(qkv, qkv_weight=None, attention_scale=m.weight(f"{prefix}.attn_scale"),
                                      attention_bias=bias, projection_weight=None, head_count=heads,
                                      window_size=8, window_origin=M.recovered_window_origin(index))
        finally:
            M.cosine_attention, M.vendor_approximate_softmax, M.vendor_cosine_normalize, M.CHUNK_TOKENS = saved

    def window_block(self, index, heads, level, entry):
        h, w = level
        channels = heads * 32
        print(f"  block {index} ({channels} channels, {w}x{h})")
        state = self.get(entry, level)
        ffn = self.feed_forward(index, state, state)
        report("feed-forward output, published", E(ffn), self.get(f"x_{index}_ffn_e4", level))
        qkv_weight = self.model.weight(f"block{index}.layer0.qkv_weight")
        # their qkv is (head, q k v, 32) a token, ours (q k v, head, 32)
        theirs = self.get(f"x_{index}_qkv", level)
        theirs = theirs.reshape(*theirs.shape[:-1], heads, 3, 32).swapaxes(-3, -2).reshape(theirs.shape)
        if channels == 32:
            report("QKV projection from the raw feed-forward output (MLX-DLSS)",
                   H(M.matmul(self.get(f"x_{index}_ffn_f16", level), qkv_weight)), theirs)
        report("QKV projection from the published feed-forward output",
               H(M.matmul(self.get(f"x_{index}_ffn_e4", level), qkv_weight)), theirs)
        attended = self.get(f"x_{index}_attended", level)
        report("attention from their QKV, ours", self.attend(theirs, index, heads), attended)
        report("  with their cosine norm and softmax denominator",
               self.attend(theirs, index, heads, normalize=their_normalize, softmax=their_softmax),
               attended)

    def raw_input_block(self, index, level, published, raw, mlx_input, mlx_skip):
        """A 32-channel block whose input arrives raw: its feed-forward both ways."""
        print(f"  block {index}, its feed-forward on the reference's own input")
        want = self.get(f"x_{index}_ffn_f16", level)
        state, skip = self.get(published, level), self.get(raw, level)
        inputs = {"published": state, "raw": skip}
        report(f"input {mlx_input}, skip {mlx_skip} (MLX-DLSS)",
               H(self.feed_forward(index, inputs[mlx_input], inputs[mlx_skip])), want)
        report("input published, skip raw",
               H(self.feed_forward(index, state, skip)), want)

    def bottleneck(self):
        print("  the ViT's input, from block 30")
        split, tokens = self.levels[4], self.levels[5]
        raw, published = self.get("x_block_30_raw", split), self.get("block_30", split)

        def pool(value):
            v = H(M.pad_spatial_end(value, 8))
            return H(H(H(v[:, 0::2, 0::2] + v[:, 0::2, 1::2]) + H(v[:, 1::2, 0::2] + v[:, 1::2, 1::2]))
                     * np.float32(0.25))
        pooled = self.get("x_vit_pooled", tokens)
        report("pool of the published output (MLX-DLSS)",
               E(M.average_pool2(M.pad_spatial_end(published, 8))), pooled)
        report("pool of the raw output", E(pool(raw)), pooled)
        weight = self.model.weight("block30.layer4.weight")
        entry = self.get("x_vit_entry", tokens)
        report("projection of the published output's pool, unpublished (MLX-DLSS)",
               E(M.downsample(M.pad_spatial_end(published, 8), weight=weight)), entry)
        report("projection of the raw output's pool, published", E(M.matmul(pooled, weight)), entry)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--size", default="320x180")
    parser.add_argument("--image", default=str(reference.ROOT / "pngs" / "Cyberpunk-2077_01.jpg"))
    args = parser.parse_args()
    graph = reference.PORT / "src" / "graph.js"
    if "x-adapter-e4" not in graph.read_text():
        raise SystemExit(f"{graph} records no steps: git -C {reference.PORT.parents[1]} apply "
                         f"{HERE / 'opendlss_captures.patch'}")
    width, height = (int(v) for v in args.size.split("x"))
    field, levels = their_levels(width, height)
    geometry = nr_frame.network_geometry(width, height)
    if (geometry.network_width, geometry.network_height) != field:
        raise SystemExit(f"{args.size}: our field is not the reference's {field}")

    import nr_daemon
    colour = nr_daemon.resample(np.asarray(image_io.load(args.image), np.float32), (height, width))
    features = nr_frame.build_features(colour, geometry=geometry, **nr_frame.PROFILES["standard"])
    _, result, bounds = reference.reference_run(features, width, height, capture=True)
    bounds = {name.replace("-", "_"): value for name, value in bounds.items()}
    print(f"{args.size} on a {field[0]}x{field[1]} field; reference on {result['adapter']}")
    steps = Steps(bounds, M.NeuralRenderingModel.from_safetensors(nr_frame.WEIGHTS), levels, field)
    full = (field[1], field[0])
    for index, heads, level, entry in ((1, 1, levels[0], "transition_0_1"),
                                       (5, 2, levels[1], "transition_4_5"),
                                       (9, 4, levels[2], "transition_8_9"),
                                       (15, 8, levels[3], "transition_14_15")):
        steps.window_block(index, heads, level, entry)
    steps.raw_input_block(0, full, "x_adapter_e4", "x_adapter_f16", "raw", "raw")
    steps.raw_input_block(66, levels[0], "x_merge_66", "x_merge_66_raw", "published", "published")
    steps.raw_input_block(70, full, "x_post_merge", "x_post_merge_raw", "raw", "raw")
    steps.bottleneck()


if __name__ == "__main__":
    main()
