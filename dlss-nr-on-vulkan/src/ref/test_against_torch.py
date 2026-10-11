#!/usr/bin/env python3
"""
test_against_torch — check our numpy port against MLX-DLSS's PyTorch original.

Everything else in this project validates against *properties* (E4M3 fixed points,
unit rows, permutations) or against a *specification* (their weight_spec.json).
This is the one check that compares implementation to implementation: the same
operators, the same real weights, the same inputs, one written in numpy by us and
one in PyTorch by them.

It needs torch, which the system Python does not have:

    python3 -m venv work/venv
    work/venv/bin/pip install --index-url https://download.pytorch.org/whl/cpu torch
    work/venv/bin/pip install numpy safetensors
    NR_CHUNK_TOKENS=0 MLXDLSS_TORCH_CHUNK_TOKENS=0 work/venv/bin/python \\
        src/ref/test_against_torch.py

Chunking is disabled on both sides so any difference is the arithmetic, not the row
count a BLAS was handed.

Our graph is not theirs everywhere: where MLX-DLSS's recovery and the vendor's own differ,
this tree follows the vendor's (`nr_model.MLX_DLSS_GRAPH`, notes/opendlss-reference.md).
The switch restores theirs, and this check runs with it on: it is a check of the port.
"""
from __future__ import annotations

import importlib.util
import pathlib
import sys
import types

import numpy as np

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(HERE))

import nr_model as ours  # noqa: E402

# the port check compares like with like: MLX-DLSS's graph on both sides
ours.MLX_DLSS_GRAPH = True

try:
    import torch
except ImportError:
    raise SystemExit("this check needs torch; see the module docstring")

WEIGHTS = next((p for p in (ROOT / "work" / "mlxw" / "dlssnr-prepared.safetensors",
                            ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors") if p.exists()),
               ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors")
_MLX_MODEL = ROOT / "work" / "mlx-dlss" / "python" / "mlxdlss" / "model.py"

FAILURES = []
# Two float32 implementations of the same expression differ in the last bits; the
# rounding points in this graph then quantise most of that away. Anything at 1e-05
# relative is the same arithmetic, and anything above it is a porting difference.
TOLERANCE = 1e-5


def load_theirs():
    if not _MLX_MODEL.exists():
        raise SystemExit(f"missing {_MLX_MODEL}; clone MLX-DLSS into work/mlx-dlss")
    package = types.ModuleType("mlxref")
    package.__path__ = [str(_MLX_MODEL.parent)]
    sys.modules["mlxref"] = package
    spec = importlib.util.spec_from_file_location("mlxref.model", _MLX_MODEL)
    module = importlib.util.module_from_spec(spec)
    sys.modules["mlxref.model"] = module
    spec.loader.exec_module(module)
    return module


theirs = load_theirs()


def t(array):
    return torch.from_numpy(np.ascontiguousarray(np.asarray(array, dtype=np.float32)))


def n(tensor):
    return tensor.detach().to(torch.float32).numpy()


def compare(name, mine, reference, tolerance=TOLERANCE):
    mine = np.asarray(mine, dtype=np.float32)
    reference = n(reference) if isinstance(reference, torch.Tensor) else np.asarray(reference)
    if mine.shape != reference.shape:
        print(f"  [FAIL] {name}: shape {mine.shape} vs {reference.shape}")
        FAILURES.append(name)
        return
    identical = np.array_equal(mine, reference)
    scale = max(float(np.abs(reference).max()), 1e-12)
    relative = float(np.abs(mine - reference).max()) / scale
    ok = identical or relative <= tolerance
    mark = "ok  " if ok else "FAIL"
    detail = "bit-identical" if identical else f"max rel {relative:.2e}"
    print(f"  [{mark}] {name:<44s} {detail}")
    if not ok:
        FAILURES.append(name)


def section(title):
    print(f"\n{title}")


# --------------------------------------------------------------------------


def check_primitives():
    section("precision primitives")
    rng = np.random.default_rng(0)
    for label, values in (
        ("normal range", rng.standard_normal(1 << 16).astype(np.float32) * 40),
        ("subnormal range", rng.standard_normal(1 << 16).astype(np.float32) * 1e-3),
        ("saturating", rng.standard_normal(1 << 14).astype(np.float32) * 1e3),
        ("swept", np.linspace(-500, 500, 1 << 16, dtype=np.float32)),
    ):
        # torch has float8_e4m3fn here, so this compares our bit arithmetic against a
        # library float8 cast, not against the same trick reimplemented.
        compare(f"e4m3 ({label})", ours.e4m3(values), theirs.e4m3_round_trip(t(values)))
    # The non-32 cosine path squares, sums and rsqrts in half. It is dead in this graph
    # — head_dim is 32 at every width — and it is where the only real porting error this
    # check found lived: a half sqrt followed by a half divide rounds twice where
    # `rsqrt` rounds once, which drifted by 1e-03.
    for width in (4, 8):
        short = rng.standard_normal((64, width)).astype(np.float32)
        compare(f"vendor_cosine_normalize (non-32 path, {width} wide)",
                ours.vendor_cosine_normalize(short), theirs.vendor_cosine_normalize(t(short)))
    values = rng.standard_normal(1 << 16).astype(np.float32) * 6
    compare("quadratic_gate", ours.quadratic_gate(values), theirs.quadratic_gate(t(values)))
    compare("quadratic_gate_activation", ours.quadratic_gate_activation(values),
            theirs.quadratic_gate_activation(t(values)))
    scores = rng.normal(0, 2, (3, 4, 64, 64)).astype(np.float32)
    compare("vendor_approximate_softmax", ours.vendor_approximate_softmax(scores),
            theirs.vendor_approximate_softmax(t(scores)))
    heads = rng.standard_normal((2, 4, 64, 32)).astype(np.float32)
    compare("vendor_cosine_normalize", ours.vendor_cosine_normalize(heads),
            theirs.vendor_cosine_normalize(t(heads)))
    scale = rng.uniform(0.02, 30, 4).astype(np.float32)
    compare("vendor_cosine_publish", ours.vendor_cosine_publish(heads, scale),
            theirs.vendor_cosine_publish(t(heads), t(scale)))
    wide = rng.standard_normal((2, 4, 64, 64)).astype(np.float32)
    compare("vendor_cosine_normalize (non-32 path, 64 wide)",
            ours.vendor_cosine_normalize(wide), theirs.vendor_cosine_normalize(t(wide)))


def check_layout():
    section("layout and geometry")
    same = all(ours.recovered_window_origin(index)
               == theirs.recovered_window_origin(index) for index in range(72))
    print(f"  [{'ok  ' if same else 'FAIL'}] window origin, all 71 blocks and one past the end")
    if not same:
        FAILURES.append("recovered_window_origin")
    same = ours.FRAGMENT_SWIZZLE_INDICES.tolist() == theirs.FRAGMENT_SWIZZLE_INDICES
    print(f"  [{'ok  ' if same else 'FAIL'}] attention-bias fragment order, all 4096 entries")
    if not same:
        FAILURES.append("FRAGMENT_SWIZZLE_INDICES")
    for heads in (1, 2, 4, 8, 16, 32):
        if ours.uses_fragment_swizzle(0, heads) != theirs.uses_fragment_swizzle(0, heads):
            FAILURES.append(f"uses_fragment_swizzle({heads})")
    print(f"  [{'ok  ' if not any('swizzle(' in f for f in FAILURES) else 'FAIL'}] "
          "which head counts are stored in fragment order")

    rng = np.random.default_rng(1)
    bias = rng.standard_normal((4, 64, 64)).astype(np.float32)
    compare("recover_attention_bias_layout", ours.recover_attention_bias_layout(bias),
            theirs.recover_attention_bias_layout(t(bias)))
    value = rng.standard_normal((1, 24, 32, 16)).astype(np.float32)
    compare("partition_windows", ours.partition_windows(value, 8),
            theirs.partition_windows(t(value), 8))
    windows = ours.partition_windows(value, 8)
    compare("reverse_windows",
            ours.reverse_windows(windows, batch_count=1, height=24, width=32, window_size=8),
            theirs.reverse_windows(t(windows), batch_count=1, height=24, width=32, window_size=8))
    compare("average_pool2", ours.average_pool2(value), theirs.average_pool2(t(value)))
    compare("pad_spatial_end", ours.pad_spatial_end(value, 8), theirs.pad_spatial_end(t(value), 8))
    compare("nearest_upsample2_crop",
            ours.nearest_upsample2_crop(value, height=45, width=61),
            theirs.nearest_upsample2_crop(t(value), height=45, width=61))
    interpolation = rng.uniform(0, 1, 16).astype(np.float32)
    compare("learned_upsample2", ours.learned_upsample2(value, interpolation=interpolation),
            theirs.learned_upsample2(t(value), interpolation=t(interpolation)))
    skip = rng.standard_normal((1, 45, 61, 16)).astype(np.float32)
    sine = rng.standard_normal(16).astype(np.float32)
    compare("decoder_input_merge",
            ours.decoder_input_merge(value, skip=skip, skip_sine=sine),
            theirs.decoder_input_merge(t(value), skip=t(skip), skip_sine=t(sine)))


def _torch_matmul(a, b):
    return (torch.from_numpy(np.ascontiguousarray(np.asarray(a, dtype=np.float32)))
            @ torch.from_numpy(np.ascontiguousarray(np.asarray(b, dtype=np.float32)))).numpy()


def _torch_matmul_nt(a, b):
    left = torch.from_numpy(np.ascontiguousarray(np.asarray(a, dtype=np.float32)))
    right = torch.from_numpy(np.ascontiguousarray(np.asarray(b, dtype=np.float32)))
    return (left @ right.transpose(-1, -2)).numpy()


def _both(function, arrays, scalars):
    """Call the same operator on both sides, arrays converted, scalars passed through."""
    mine = getattr(ours, function)(**{**arrays, **scalars})
    converted = {key: t(value) for key, value in arrays.items()}
    return mine, getattr(theirs, function)(**{**converted, **scalars})


def check_blocks(weights):
    """Block families on real weights, with the GEMM shared so only the port can differ."""
    section("block families, on real weights, identical GEMMs")
    ours.MATMUL, ours.MATMUL_NT = _torch_matmul, _torch_matmul_nt
    try:
        _check_blocks(weights)
    finally:
        ours.MATMUL, ours.MATMUL_NT = None, None


def _check_blocks(weights):
    rng = np.random.default_rng(2)

    def weight(name):
        return weights[name]

    # block 1: plain window block, one head
    value = (rng.standard_normal((1, 16, 24, 32)) * 0.3).astype(np.float32)
    bias = ours.recover_attention_bias_layout(weight("block1.layer0.attn_bias"))
    arrays = dict(value=value, expansion_weight=weight("block1.layer0.weight1"),
                  feed_forward_projection_weight=weight("block1.layer0.weight2"),
                  feed_forward_cosine=weight("block1.layer0.ffn_cos_skip"),
                  qkv_weight=weight("block1.layer0.qkv_weight"),
                  attention_scale=weight("block1.layer0.attn_scale"),
                  attention_bias=bias,
                  attention_projection_weight=weight("block1.layer0.projection_weight"),
                  attention_cosine=weight("block1.layer0.attn_cos_skip"))
    compare("window_block (block 1, 1 head)",
            *_both("window_block", arrays,
                   dict(head_count=1, window_size=8, window_origin=(-4, 0))))

    # block 5: branched feed-forward, two heads
    value = (rng.standard_normal((1, 16, 24, 64)) * 0.3).astype(np.float32)
    arrays = dict(value=value, expansion_weight=weight("block5.layer0.ffn_expand_weight"),
                  branch_projection_weight=weight("block5.layer0.ffn_branch_projection_weight"),
                  output_projection_weight=weight("block5.layer0.ffn_output_projection_weight"),
                  feed_forward_cosine=weight("block5.layer0.ffn_cos_skip"),
                  qkv_weight=weight("block5.layer0.qkv_weight"),
                  attention_scale=weight("block5.layer0.attn_scale"),
                  attention_bias=weight("block5.layer0.attn_bias"),
                  attention_projection_weight=weight("block5.layer0.projection_weight"),
                  attention_cosine=weight("block5.layer0.attn_cos_skip"))
    compare("branched_window_block (block 5, 2 heads)",
            *_both("branched_window_block", arrays,
                   dict(head_count=2, window_size=8, window_origin=(0, 0))))
    compare("branched_feed_forward alone",
            *_both("branched_feed_forward",
                   {k: arrays[k] for k in ("value", "expansion_weight",
                                           "branch_projection_weight",
                                           "output_projection_weight")}, {}))

    # block 23: split family, sixteen heads
    value = (rng.standard_normal((1, 16, 24, 512)) * 0.3).astype(np.float32)
    bias = ours.recover_attention_bias_layout(weight("block23.layer2.attn_bias"))
    arrays = dict(value=value,
                  first_projection_weight=weight("block23.layer0.first_projection_weight"),
                  expand_weight=weight("block23.layer0.group_expand_weight"),
                  project_weight=weight("block23.layer0.group_project_weight"),
                  feed_forward_projection_weight=weight("block23.layer1.weight3"),
                  feed_forward_cosine=weight("block23.layer1.ffn_cos_skip"),
                  qkv_weight=weight("block23.layer2.qkv_weight"),
                  attention_scale=weight("block23.layer2.attn_scale"),
                  attention_bias=bias,
                  attention_projection_weight=weight("block23.layer3.projection_weight"),
                  attention_cosine=weight("block23.layer3.attn_cos_skip"))
    compare("split_window_block (block 23, 16 heads)",
            *_both("split_window_block", arrays,
                   dict(head_count=16, window_size=8, window_origin=(0, 0))))
    compare("split_group_feed_forward alone",
            *_both("split_group_feed_forward",
                   {k: arrays[k] for k in ("value", "first_projection_weight",
                                           "expand_weight", "project_weight")}, {}))

    # block 31: global, thirty-two heads
    value = (rng.standard_normal((1, 8, 8, 1024)) * 0.3).astype(np.float32)
    arrays = dict(value=value, expansion_weight=weight("block31.layer0.weight"),
                  feed_forward_projection_weight=weight("block31.layer1.weight"),
                  feed_forward_cosine=weight("block31.layer1.ffn_cos_skip"),
                  qkv_weight=weight("block31.layer2.qkv_weight"),
                  attention_scale=weight("block31.layer2.attn_scale"),
                  attention_projection_weight=weight("block31.layer4.projection_weight"),
                  attention_cosine=weight("block31.layer4.attn_cos_skip"))
    compare("global_block (block 31, 32 heads)",
            *_both("global_block", arrays, dict(head_count=32)))


def synthetic_features(extent, rng):
    features = np.zeros((1, extent, extent, 16), dtype=np.float32)
    yy, xx = np.mgrid[0:extent, 0:extent].astype(np.float32)
    features[0, ..., 0:3] = rng.normal(0, 1, (extent, extent, 3)).astype(np.float32)
    features[0, ..., 3] = 1
    colour = np.stack([np.sin(xx / 9), np.cos(yy / 11), np.sin((xx + yy) / 13)], -1)
    colour = ((np.clip(0.5 + 0.3 * colour, 0, 1) - 0.5) * 0.125).astype(np.float32)
    features[0, ..., 4:7] = colour
    features[0, ..., 7:10] = colour
    features[0, ..., 11] = 1
    features[0, ..., 12] = 1
    features[0, ..., 13] = -1
    features[0, ..., 14] = -1
    return features


def run_theirs(weights, features):
    model = theirs.NeuralRenderingModel({name: t(value) for name, value in weights.items()}).eval()
    with torch.no_grad():
        return n(model(t(features)))


def check_graph_shared_gemm(weights, extent=320):
    """The port check: give both sides the same GEMM, so only the port can differ.

    numpy's BLAS and torch's ATen reassociate a float32 dot product differently — 2e-07
    to 7e-07 on these shapes — and the E4M3 publishes turn that into whole-quantum flips
    on about 0.0008 % of cells. Routing our GEMM through torch removes that one source,
    and anything left is a real difference between the two implementations.
    """
    section(f"the whole graph with a shared GEMM, {extent}x{extent}")
    features = synthetic_features(extent, np.random.default_rng(3))
    ours.MATMUL, ours.MATMUL_NT = _torch_matmul, _torch_matmul_nt
    try:
        mine = ours.NeuralRenderingModel(weights).forward(features)
    finally:
        ours.MATMUL, ours.MATMUL_NT = None, None
    compare("full 71-block forward, identical GEMMs", mine, run_theirs(weights, features),
            tolerance=0.0)


def check_graph_independent(weights, extent=320):
    """Not a pass/fail: what two independent correct float32 implementations do.

    `notes/phase9-numerics.md` predicted that any of them diverges from this reference
    by the graph's own avalanche floor, 9-12 % of the head's sd. This measures it
    against an implementation we did not write.
    """
    section(f"the whole graph, independent BLAS, {extent}x{extent}")
    features = synthetic_features(extent, np.random.default_rng(3))
    mine = ours.NeuralRenderingModel(weights).forward(features)
    reference = run_theirs(weights, features)
    rgb_mine, rgb_reference = mine[..., :3], reference[..., :3]
    absolute = float(np.abs(rgb_mine - rgb_reference).mean())
    print(f"  head RGB  mean|d| {absolute:.7f}  sd {rgb_reference.std():.4f}  "
          f"-> {100 * absolute / rgb_reference.std():.2f} % of sd "
          f"(phase9 predicts the 9-12 % floor)")
    print(f"  head ch3  mean|d| {float(np.abs(mine[..., 3] - reference[..., 3]).mean()):.7f}")


def main():
    if ours.CHUNK_TOKENS > 0 or theirs.CHUNK_TOKENS > 0:
        print(f"note: chunking is on (ours {ours.CHUNK_TOKENS}, theirs {theirs.CHUNK_TOKENS}); "
              "set NR_CHUNK_TOKENS=0 MLXDLSS_TORCH_CHUNK_TOKENS=0 to rule it out\n")
    print(f"torch {torch.__version__}, native float8: {hasattr(torch, 'float8_e4m3fn')}")
    check_primitives()
    check_layout()
    weights, _ = ours.load_logical(WEIGHTS)
    check_blocks(weights)
    check_graph_shared_gemm(weights)
    check_graph_independent(weights)
    print()
    if FAILURES:
        print(f"{len(FAILURES)} FAILED: {', '.join(FAILURES)}")
        return 1
    print("our numpy port agrees with the PyTorch original everywhere it was checked")
    return 0


if __name__ == "__main__":
    sys.exit(main())
