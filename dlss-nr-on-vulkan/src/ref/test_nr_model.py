#!/usr/bin/env python3
"""
test_nr_model — regression for the numpy port of the recovered 71-block graph.

Checks the precision primitives against properties that pin them exactly, then
runs one whole frame at the smallest legal network extent and asserts the head
is in the range a working network produces.  Exits 0 on success.

    python3 src/ref/test_nr_model.py [--weights PATH] [--skip-forward]
"""
import argparse
import pathlib
import sys
import time

import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import nr_model as M  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parents[2]
WEIGHTS = M.default_weights() if hasattr(M, "default_weights") else ROOT / "work" / "mlxw" / "dlssnr-logical.safetensors"

FAILURES = []


def check(name, condition, detail=""):
    status = "ok  " if condition else "FAIL"
    print(f"  [{status}] {name}{'  ' + detail if detail else ''}")
    if not condition:
        FAILURES.append(name)


def e4m3_values():
    """Every finite E4M3 value, decoded from its bit pattern."""
    values = set()
    for bits in range(256):
        sign = -1 if bits >> 7 else 1
        exponent, mantissa = (bits >> 3) & 0xF, bits & 7
        if exponent == 0xF and mantissa == 7:
            continue                                   # NaN
        values.add(sign * (2.0 ** -6) * (mantissa / 8.0) if exponent == 0
                   else sign * (2.0 ** (exponent - 7)) * (1 + mantissa / 8.0))
    return np.array(sorted(values), dtype=np.float32)


def test_primitives():
    print("primitives")
    values = e4m3_values()
    check("E4M3: every representable value is a fixed point",
          np.array_equal(M.e4m3(values), values), f"n={len(values)}")
    check("E4M3: saturates at 448",
          float(M.e4m3(np.float32([1e4, -1e4])).max()) == 448.0
          and float(M.e4m3(np.float32([1e4, -1e4])).min()) == -448.0)
    check("E4M3: ties to even",
          list(M.e4m3(np.float32([17.0, 17.5]))) == [16.0, 18.0])

    rng = np.random.default_rng(0)
    scores = rng.normal(0, 2, (2, 3, 64, 64)).astype(np.float32)
    probabilities = M.vendor_approximate_softmax(scores)
    totals = probabilities.sum(-1)
    exact = np.exp(scores - scores.max(-1, keepdims=True))
    exact /= exact.sum(-1, keepdims=True)
    check("softmax approximation: rows sum near 1",
          0.9 < totals.min() and totals.max() < 1.1,
          f"{totals.min():.4f}..{totals.max():.4f}")
    correlation = float(np.corrcoef(probabilities.ravel(), exact.ravel())[0, 1])
    check("softmax approximation: tracks a true softmax", correlation > 0.99,
          f"corr {correlation:.4f}")

    normalized = M.vendor_cosine_normalize(
        rng.normal(0, 1, (1, 4, 64, 32)).astype(np.float32))
    norms = np.linalg.norm(normalized, axis=-1)
    check("cosine normalize: unit rows", np.allclose(norms, 1.0, atol=2e-3),
          f"{norms.min():.4f}..{norms.max():.4f}")

    check("attention-bias fragment order is a permutation",
          len(np.unique(M.FRAGMENT_SWIZZLE_INDICES)) == 64 * 64)

    gate = M.quadratic_gate_activation(np.float32([-8, -5, 0, 1, 4, 8]))
    check("quadratic gate: saturates below -4, linear above",
          gate[0] == 0 and gate[1] == 0 and gate[2] == 0 and gate[-1] > gate[-2] > 0,
          str(np.round(gate, 3)))


def test_weights(path):
    print("weights")
    weights, metadata = M.load_logical(path)
    check("649 logical tensors", M.logical_count(weights) == 649,
          f"{M.logical_count(weights)} logical, {len(weights) - M.logical_count(weights)} derived")
    check("format is a logical export",
          str(metadata.get("format", "")).startswith("dlssnr-logical"),
          metadata.get("format", "?"))
    check("all finite", all(np.isfinite(value).all() for value in weights.values()))
    check("block0 adapter is (16, 32)",
          weights["block0.layer0.input_adapter_weight"].shape == (16, 32))
    check("qkv is (C, 3C) — full MHA, no GQA",
          weights["block5.layer0.qkv_weight"].shape == (64, 192))
    return weights


def test_forward(weights):
    print("forward (320x320, the smallest legal network extent)")
    rng = np.random.default_rng(1)
    height = width = 320
    features = np.zeros((1, height, width, 16), dtype=np.float32)
    yy, xx = np.mgrid[0:height, 0:width].astype(np.float32)
    features[0, ..., 0:3] = rng.normal(0, 1, (height, width, 3)).astype(np.float32)
    features[0, ..., 3] = 1
    colour = np.stack([np.sin(xx / 9), np.cos(yy / 11), np.sin((xx + yy) / 13)], -1)
    colour = ((np.clip(0.5 + 0.3 * colour, 0, 1) - 0.5) * 0.125).astype(np.float32)
    features[0, ..., 4:7] = colour
    features[0, ..., 7:10] = colour
    features[0, ..., 11] = 1        # local tone
    features[0, ..., 12] = 1        # local structure
    features[0, ..., 13] = -1
    features[0, ..., 14] = -1

    started = time.perf_counter()
    head = M.NeuralRenderingModel(weights).forward(features)
    elapsed = time.perf_counter() - started
    print(f"  {elapsed:.1f}s")

    check("head shape is (1, H, W, 4)", head.shape == (1, height, width, 4),
          str(head.shape))
    check("head is finite", np.isfinite(head).all())
    rgb = head[..., :3]
    check("RGB head channels stay inside the composition's useful range",
          float(np.abs(rgb).max()) < 4.0, f"max|.| {np.abs(rgb).max():.3f}")
    check("RGB head is not collapsed", float(rgb.std()) > 1e-3,
          f"sd {rgb.std():.4f}")

    # The control channels must reach the network: zeroing tone and structure has
    # to shrink the residual by a large factor (measured 37x on a real frame).
    off = features.copy()
    off[0, ..., 11] = 0
    off[0, ..., 12] = 0
    quiet = M.NeuralRenderingModel(weights).forward(off)
    ratio = float(np.abs(head[..., :3]).mean() / max(np.abs(quiet[..., :3]).mean(), 1e-12))
    check("tone/structure controls switch the network down", ratio > 5.0,
          f"{ratio:.1f}x")


def test_composition():
    """The post-network controls, which need no forward pass."""
    print("composition controls")
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "nr_frame", pathlib.Path(__file__).resolve().parent / "nr_frame.py")
    frame = importlib.util.module_from_spec(spec)
    sys.modules.setdefault("nr_frame", frame)
    spec.loader.exec_module(frame)

    rng = np.random.default_rng(4)
    colour = rng.uniform(0.2, 0.8, (64, 64, 3)).astype(np.float32)
    head = (rng.standard_normal((64, 64, 4)) * 0.1).astype(np.float32)

    zero = frame.compose(head, colour, intensity=0.0)
    check("intensity 0 is an exact no-op", np.array_equal(zero, colour))

    full = np.abs(frame.compose(head, colour, intensity=1.0) - colour).mean()
    quarter = np.abs(frame.compose(head, colour, intensity=0.25) - colour).mean()
    check("intensity is linear", abs(quarter * 4 - full) < 1e-6 * max(full, 1e-9),
          f"{quarter:.6f} x4 vs {full:.6f}")

    detail = np.abs(frame.compose(head, colour, colour_strength=0.0) - colour).mean()
    check("colour_strength 0 leaves only the detail", detail < full,
          f"{detail:.6f} < {full:.6f}")

    values = frame.controls("standard", style_index=2, local_tone=0.5)
    check("controls() applies overrides onto a profile",
          values["normalized_style"] == 2 / 128.0
          and values["local_tone_strength"] == 0.5
          and values["local_structure_strength"] == 1.0)
    check("the neutral profile zeroes tone and structure",
          frame.PROFILES["neutral"]["local_tone_strength"] == 0
          and frame.PROFILES["neutral"]["local_structure_strength"] == 0)

    # The temporal gate is a table over the half logit's 16 bits; it must be the formula on
    # every one of them, NaN and infinity included, and on logits that are not yet half.
    every = np.arange(1 << 16, dtype=np.uint32).astype(np.uint16).view(np.float16)
    grid = np.zeros((256, 256, 4), np.float32)
    grid[..., 3] = every.astype(np.float32).reshape(256, 256)
    with np.errstate(over="ignore", invalid="ignore"):
        want = frame.gate_formula(frame.half(grid[..., 3:4]))
    check("the gate table is the formula on all 65536 half logits",
          np.array_equal(want.view(np.uint32), frame.history_weight(grid).view(np.uint32)))
    logits = (rng.standard_normal((97, 131, 4)) * 4).astype(np.float32)
    check("... and on logits that are not half yet, through a strided view",
          np.array_equal(frame.gate_formula(frame.half(logits[::2, :, 3:4])).view(np.uint32),
                         frame.history_weight(logits[::2]).view(np.uint32)))


def test_geometry():
    """The network's extent: the vendor's field by default, the graph's floor on request."""
    print("network geometry")
    import nr_frame as frame
    # the vendor's own fields (OpenDLSS-NR's `geometryFromValid`): 1280x720 and 1024x768 take
    # a column of windows more, because their fields would pool to a bottleneck with no
    # padding token; 1920x1080 aligns to 128, level 0 not being whole windows at 1080
    known = {(320, 180): (320, 320), (448, 252): (448, 320), (576, 324): (576, 384),
             (1056, 594): (1088, 640), (1152, 648): (1152, 768), (1280, 720): (1344, 768),
             (1024, 768): (1088, 768), (512, 512): (576, 512), (1920, 1080): (1920, 1152),
             (1153, 642): (1280, 768)}
    got = {size: (g.network_width, g.network_height)
           for size, g in ((size, frame.network_geometry(*size)) for size in known)}
    check("the vendor's field at ten sizes", got == known,
          ", ".join(f"{w}x{h} -> {got[(w, h)]}" for (w, h) in known if got[(w, h)] != known[(w, h)]))
    sizes = [(w, h) for w in (64, 101, 179, 256, 320, 321, 640, 1280, 1920)
             for h in (64, 101, 180, 288, 320, 704, 720, 1080)]
    small = frame.network_geometry(320, 180, minimum=128)
    check("at the graph's floor a 320x180 frame runs at 320x256 — 180 aligns to 128",
          (small.network_width, small.network_height) == (320, 256),
          f"{small.network_width}x{small.network_height}")
    floor = frame.network_geometry(100, 60, minimum=64)
    check("no floor below the graph's 128", (floor.network_width, floor.network_height) == (128, 128),
          f"{floor.network_width}x{floor.network_height}")
    extents = [frame.network_geometry(w, h, minimum=m) for w, h in sizes for m in (128, 192, 256, 320)]
    check("every extent a multiple of 64 that covers the frame",
          all(g.network_width % 64 == 0 and g.network_height % 64 == 0
              and g.network_width >= g.output_width and g.network_height >= g.output_height
              for g in extents))
    check("MLX-DLSS's own pipeline takes the same field",
          all(frame.NetworkGeometry.vendor_aligned(w, h) == frame.network_geometry(w, h)
              for w, h in sizes))

    # the render scale's frame: the cheapest field among frames at least the scale's own,
    # and the largest frame on it
    def area(w, h, m=320):
        g = frame.network_geometry(w, h, minimum=m)
        return g.network_width * g.network_height
    grown = {(640, 360, 0.35): (320, 180), (800, 450, 0.35): (320, 180),
             (800, 450, 0.6): (512, 288), (1280, 720, 0.35): (448, 252),
             (1920, 1080, 1.0): (1920, 1080)}
    got = {key: frame.render_extent(*key) for key in grown}
    check("a small window's low scale fills the cheapest field, a large window's is left alone",
          got == grown, ", ".join(f"{k} -> {got[k]}" for k in grown if got[k] != grown[k]))
    cases = [(w, h, s, m) for w, h in ((640, 360), (800, 450), (854, 480), (1024, 576),
                                       (1280, 720), (1920, 1080), (1920, 1200))
             for s in (0.2, 0.35, 0.5, 0.6, 0.75, 0.9) for m in (128, 320)]
    ok = True
    for w, h, s, m in cases:
        grown_w, grown_h = frame.render_extent(w, h, s, m)
        own = (max(64, round(w * s)), max(64, round(h * s)))
        ok &= grown_w >= own[0] and area(grown_w, grown_h, m) <= area(*own, m)
    check(f"never a smaller frame, never a dearer field ({len(cases)} cases)", ok)
    steps = [area(*frame.render_extent(w, h, s, 320)) for w, h in ((800, 450), (854, 480))
             for s in (0.3, 0.35, 0.4, 0.45, 0.5, 0.6)]
    check("and a lower scale never the slower",
          all(a <= b for a, b in zip(steps[:6], steps[1:6])) and
          all(a <= b for a, b in zip(steps[6:], steps[7:])))


def test_temporal():
    """The temporal contract, none of which needs a forward pass."""
    print("temporal path")
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "nr_temporal", pathlib.Path(__file__).resolve().parent / "nr_temporal.py")
    temporal = importlib.util.module_from_spec(spec)
    sys.modules.setdefault("nr_temporal", temporal)
    spec.loader.exec_module(temporal)

    rng = np.random.default_rng(6)
    image = rng.uniform(0, 1, (192, 256, 3)).astype(np.float32)
    worst = 0.0
    for shift in ((6, 0), (0, 5), (-4, 3)):
        sequence, motions = temporal.pan_sequence(image, shift=shift, size=(64, 64),
                                                  frames=3)
        worst = max(worst, temporal.check_motion_convention(sequence, motions))
    check("motion reprojects a whole-pixel pan exactly", worst < 1e-6, f"{worst:.2e}")

    sequence, motions = temporal.pan_sequence(image, shift=(6, 0), size=(64, 64), frames=3)
    zero = [np.zeros_like(motion) for motion in motions]
    check("zero motion on a pan does not reproject",
          temporal.check_motion_convention(sequence, zero) > 0.01)

    alpha = temporal.blend_alpha(np.float32([[[0, 0, 0, -8], [0, 0, 0, 0],
                                              [0, 0, 0, 8]]]))
    check("the blend gate rests closed and opens to blend_scale",
          alpha[0, 0, 0] < 0.001 and abs(float(alpha[0, 2, 0]) - temporal.BLEND_SCALE) < 0.001,
          f"{alpha[0, 0, 0]:.4f} .. {alpha[0, 2, 0]:.4f} (ceiling {temporal.BLEND_SCALE:.4f})")

    # channels 7-9 hold the history under the colour scaling; compose_temporal inverts it
    colour = rng.uniform(0.2, 0.8, (32, 32, 3)).astype(np.float32)
    history = rng.uniform(0.2, 0.8, (32, 32, 3)).astype(np.float32)
    motion = np.zeros((32, 32, 2), dtype=np.float32)
    features = temporal.temporal_mod.make_temporal_features(colour, history, motion,
                                                            frame_index=0)
    recovered = features[..., 7:10] * np.float32(8) + np.float32(0.5)
    check("history survives the channel packing round trip",
          float(np.abs(recovered - history).max()) < 2e-3,
          f"max {np.abs(recovered - history).max():.5f}")

    head = np.zeros((32, 32, 4), dtype=np.float32)
    head[..., 3] = 8.0                      # gate wide open
    opened = temporal.temporal_mod.compose_temporal(head, colour, features)
    head[..., 3] = -8.0                     # gate shut
    shut = temporal.temporal_mod.compose_temporal(head, colour, features)
    check("an open gate moves the frame towards the history",
          np.abs(opened - history).mean() < np.abs(shut - history).mean(),
          f"{np.abs(opened - history).mean():.4f} vs {np.abs(shut - history).mean():.4f}")


def test_display_codec():
    """The HDR display codec, which needs no network at all."""
    print("display codec")
    import nr_display as display

    rng = np.random.default_rng(7)
    low = rng.random((32, 32, 3)).astype(np.float32)
    check("sRGB transfer round trips",
          float(np.abs(display.srgb_to_linear(display.linear_to_srgb(low)) - low).max()) < 1e-6)
    check("OkLab round trips",
          float(np.abs(display.from_oklab(display.to_oklab(low)) - low).max()) < 1e-5)

    # a scene-linear frame with real highlights
    hdr = (low * (1 + 12 * np.clip((display.luminance(low) - 0.2) / 0.3, 0, 1)[..., None] ** 2)
           ).astype(np.float32)
    proxy = display.encode(hdr)
    check("the proxy is display-referred", proxy.min() >= 0 and proxy.max() <= 1,
          f"{proxy.min():.3f}..{proxy.max():.3f}")
    check("the knee leaves ordinary luminance alone",
          np.allclose(display.encode(low * 0.5),
                      display.linear_to_srgb(low * 0.5), atol=1e-6))

    codec = display.DisplayCodec(transfer_strength=0)
    check("zero transfer strength is an exact no-op",
          np.array_equal(display.resolve(proxy, proxy * 0.5, hdr, codec), hdr))
    check("a display-referred input is not encoded",
          np.array_equal(display.encode(low, display.DisplayCodec(
              input_is_display_referred=True)), low))

    resolved = display.resolve(proxy, proxy, hdr)
    above = hdr.max(-1) > 1
    check("highlights survive the round trip", resolved.max() > 1.0 and above.any(),
          f"peak {hdr.max():.2f} -> {resolved.max():.2f}, "
          f"{100 * above.mean():.1f}% of pixels above 1")
    check("the resolve cannot go negative", resolved.min() >= 0)


def test_accelerator():
    """If torch is here, the optional rounding accelerator must change nothing."""
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "nr_accel", pathlib.Path(__file__).resolve().parent / "nr_accel.py")
    accel = importlib.util.module_from_spec(spec)
    sys.modules.setdefault("nr_accel", accel)
    spec.loader.exec_module(accel)
    if not accel.available():
        print("optional accelerator\n  [skip] torch is not installed")
        return
    print("optional accelerator")
    ok, report = accel.verify()
    for line in report:
        if "FAIL" in line:
            print(line)
    check("torch rounding is bit-identical on every representable value and tie", ok,
          f"{len(report)} checks")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--weights", default=str(WEIGHTS))
    parser.add_argument("--skip-forward", action="store_true")
    args = parser.parse_args()

    test_primitives()
    test_composition()
    test_geometry()
    test_temporal()
    test_display_codec()
    test_accelerator()
    if not pathlib.Path(args.weights).exists():
        print(f"\nweights not found at {args.weights}; skipping the graph checks")
        print("  see notes/HANDOFF.md section 0 for how to produce them")
    else:
        weights = test_weights(args.weights)
        if not args.skip_forward:
            test_forward(weights)

    print()
    if FAILURES:
        print(f"{len(FAILURES)} FAILED: {', '.join(FAILURES)}")
        return 1
    print("all checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
