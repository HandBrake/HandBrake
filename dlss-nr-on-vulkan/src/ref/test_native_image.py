#!/usr/bin/env python3
"""The native CPU passes against the NumPy they transcribe: byte-identical, or nothing.

Every function in `nr_image.c` exists to be faster, not different. The whole discipline
of this port is that an optimisation keeps the output bit-for-bit, so this runs both
implementations over the same awkward inputs — reversed views, padded crops, a mirrored
network extent, values that sit on an FP16 rounding boundary — and requires equality, not
closeness.
"""
import contextlib
import os
import pathlib
import sys

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))
sys.path.insert(0, str(ROOT / "src" / "layer"))
import nr_frame      # noqa: E402
import nr_image      # noqa: E402
import nr_daemon     # noqa: E402

FAILURES = []


@contextlib.contextmanager
def numpy_only():
    """Force the reference side onto NumPy.

    Once the native path is wired into `nr_frame` and `nr_daemon`, calling them for the
    "reference" runs the very code under test and every comparison passes for the wrong
    reason. `NR_HOST_NATIVE` is read per call, not cached, so this is enough.
    """
    keep = os.environ.get("NR_HOST_NATIVE")
    os.environ["NR_HOST_NATIVE"] = "0"
    try:
        yield
    finally:
        if keep is None:
            os.environ.pop("NR_HOST_NATIVE", None)
        else:
            os.environ["NR_HOST_NATIVE"] = keep


def check(name, ok, detail=""):
    print(f"  [{'ok  ' if ok else 'FAIL'}] {name}{'  ' + detail if detail else ''}", flush=True)
    if not ok:
        FAILURES.append(name)


def same(name, native, reference, detail=""):
    if native is None:
        check(name, False, "no native library built")
        return
    equal = np.array_equal(native, reference)
    if not equal:
        gap = np.abs(np.asarray(native, np.float64) - np.asarray(reference, np.float64))
        detail = f"{(gap > 0).sum()} of {gap.size} differ, worst {gap.max():.3e}"
    check(name, equal, detail)


def codec_checks(rng):
    for kind, vk_format, bgra in (("bgra8", 44, 1), ("rgba8", 37, 0)):
        raw = rng.integers(0, 256, (48, 80, 4), dtype=np.uint8).tobytes()
        with numpy_only():
            reference = nr_daemon.decode(raw, 80, 48, vk_format)
        same(f"decode {kind}", nr_image.decode8(raw, 80, 48, bgra),
             reference)
        image = rng.random((48, 80, 3), dtype=np.float32)
        # values that land exactly on a rounding boundary, and outside the range
        image[0, 0], image[0, 1], image[0, 2] = 0.0, 1.0, 0.5
        image[1, 0], image[1, 1] = -0.1, 1.1
        image[1, 2], image[1, 3] = 1.0 / 255, 254.5 / 255
        with numpy_only():
            reference = np.frombuffer(nr_daemon.encode(image, raw, vk_format), np.uint8)
        same(f"encode {kind}", np.frombuffer(nr_image.encode8(image, raw, bgra), np.uint8),
             reference)
    # a NaN must land on zero, as NumPy's byte cast does
    image = np.zeros((4, 4, 3), np.float32)
    image[0, 0, 0] = np.nan
    raw = bytes(4 * 4 * 4)
    with numpy_only():
        reference = np.frombuffer(nr_daemon.encode(image, raw, 44), np.uint8)
    same("encode carries NaN to zero the way NumPy does",
         np.frombuffer(nr_image.encode8(image, raw, 1), np.uint8), reference)


def _numpy_resample(image, size):
    with numpy_only():
        return nr_daemon.resample(image, size)


def resize_checks(rng):
    source = rng.random((57, 91, 3), dtype=np.float32)
    for size in ((31, 50), (120, 200), (57, 33), (57, 91)):
        with numpy_only():
            reference = nr_daemon.resample(source, size)
        same(f"resize to {size[1]}x{size[0]}", nr_image.bilinear(source, size), reference)
    # a reversed view and a padded crop, which the daemon really hands it: the decode is
    # a reversed slice of the wire buffer and the letterbox crop is a padded one
    whole = rng.random((64, 96, 4), dtype=np.float32)
    with numpy_only():
        reference = nr_daemon.resample(whole[..., 2::-1], (20, 30))
    same("resize a reversed view", nr_image.bilinear(whole[..., 2::-1], (20, 30)), reference)
    # Not a whole factor: `resample` takes the area mean when the extent divides evenly,
    # which is a different filter and deliberately so — it is what keeps aliasing out of
    # the network's input. The native path must never stand in for that branch, and the
    # daemon is what keeps them apart.
    crop = whole[8:56, 4:92, :3]
    with numpy_only():
        reference = nr_daemon.resample(crop, (25, 41))
        averaged = nr_daemon.resample(crop, (24, 44))
    same("resize a padded crop", nr_image.bilinear(crop, (25, 41)), reference)
    check("the area mean is not bilinear",
          not np.array_equal(nr_image.bilinear(crop, (24, 44)), averaged),
          "48x88 -> 24x44 divides evenly, so `resample` averages instead of sampling")
    # The area mean itself, natively, against the NumPy adds it transcribes: the live
    # extent's 2x2, uneven factors, four channels, tiny and large values, and the same
    # reversed view and padded crop as above.
    for shape, factors, scale in (((360, 640, 3), (2, 2), 1.0), ((90, 160, 3), (3, 2), 1e-3),
                                  ((96, 128, 4), (4, 4), 255.0), ((64, 96, 3), (1, 2), 1.0)):
        frame = (rng.random(shape, dtype=np.float32) * np.float32(scale))
        size = (shape[0] // factors[0], shape[1] // factors[1])
        with numpy_only():
            reference = nr_daemon.resample(frame, size)
        same(f"area mean {shape[1]}x{shape[0]} by {factors[1]}x{factors[0]}",
             nr_image.area_mean(frame, factors), reference)
    same("area mean of a reversed view", nr_image.area_mean(whole[..., 2::-1], (2, 2)),
         _numpy_resample(whole[..., 2::-1], (32, 48)))
    same("area mean of a padded crop", nr_image.area_mean(crop, (2, 2)), averaged)


def feature_checks(rng):
    for width, height in ((64, 48), (91, 57)):
        colour = rng.random((height, width, 3), dtype=np.float32)
        history = rng.random((height, width, 3), dtype=np.float32)
        geometry = nr_frame.NetworkGeometry.vendor_aligned(width, height)
        rows, columns = geometry.source_rows(), geometry.source_columns()
        noise = nr_frame.deterministic_noise(geometry.network_height,
                                             geometry.network_width, 0)
        controls = np.array([0.0, 1.0, 1.0, -1.0, -1.0], np.float32)
        with numpy_only():
            reference = nr_frame.make_features(colour, geometry=geometry,
                                               **nr_frame.PROFILES["standard"])
        same(f"features {width}x{height} -> {geometry.network_width}x{geometry.network_height}",
             nr_image.features(colour, rows, columns, noise, controls), reference)
        nr_frame.apply_history(reference, history, geometry)     # NumPy either way
        same("features with a history in channels 7-9",
             nr_image.features(colour, rows, columns, noise, controls, history=history),
             reference)
        # Built as half, in place: NumPy's float16 rounding of the same float32 features,
        # which is the GPU's to_half to the bit (test_input_fp16.py). No rounding happens at
        # all: every feature value is a half value already — the colour and the history are
        # rounded to half as they are scaled, and the noise is quantised — so the half input
        # cannot move one. That is checked first, since it is what makes the input exact.
        check("every feature value is a half value",
              np.array_equal(reference, reference.astype(np.float16).astype(np.float32)),
              "the half input would round what is not")
        into = np.full((geometry.network_height, geometry.network_width, 16), np.nan,
                       np.float16)
        built = nr_image.features(colour, rows, columns, noise, controls, history=history,
                                  out=into)
        check("features built into a half array are that array", built is into, "")
        same("features as half, with a history", built, reference.astype(np.float16))
        # and build_features writes the same into a given array without the library
        with numpy_only():
            fallback = nr_frame.build_features(
                colour, geometry=geometry, history=history,
                out=np.empty_like(into), **nr_frame.PROFILES["standard"])
        same("build_features into half, NumPy fallback", fallback, into)


def half_checks():
    # Every half value, and both sides of every positive finite rounding boundary and
    # their negatives — the set test_input_fp16.py checks the GPU's to_half on.
    half = np.arange(65536, dtype=np.uint16).view(np.float16).astype(np.float32)
    positive = np.arange(0x7c00, dtype=np.uint16).view(np.float16).astype(np.float32)
    mid = (positive[:-1] + positive[1:]) * np.float32(.5)
    boundary = np.concatenate((mid, np.nextafter(mid, np.float32(-np.inf)),
                               np.nextafter(mid, np.float32(np.inf))))
    values = np.concatenate((half[~np.isnan(half)], boundary, -boundary,
                             np.array([65519, 65520, 65521, -65519, -65520, -65521],
                                      np.float32)))
    target = np.empty(values.size, np.float16)
    with np.errstate(over="ignore"):
        want = values.astype(np.float16)
    same("to_half at every rounding boundary", nr_image.to_half(values, target), want)


def compose_checks(rng):
    height, width = 48, 80
    colour = rng.random((height, width, 3), dtype=np.float32)
    history = rng.random((height, width, 3), dtype=np.float32)
    previous = colour.copy()
    previous[10:20] = rng.random((10, width, 3), dtype=np.float32)   # a moving band
    previous[20:24] += np.float32(1.0 / 255)                         # one level of drift
    head = (rng.random((height, width, 4), dtype=np.float32) - 0.5).astype(np.float32) * 2
    mask = np.ones((height, width, 3), np.float32)
    mask[30:40, 10:30, 0] = 0.0

    for intensity in (1.0, 0.6, 0.0, 1.66):
        with numpy_only():
            reference = nr_frame.compose(head, colour, intensity=intensity)
        same(f"still composition, intensity {intensity}",
             nr_image.compose(head, colour, intensity), reference)

    scale = np.float32(nr_frame.BLEND_SCALE)
    for hold, with_previous, control in ((1.0, True, None), (0.0, False, None),
                                         (0.5, True, mask), (1.0, True, mask)):
        floor = (nr_daemon.hold_floor(colour, previous, hold)
                 if with_previous and hold > 0 else None)
        with numpy_only():
            reference = nr_frame.compose(head, colour, intensity=1.0, history=history,
                                         history_floor=floor, control_mask=control)
        gate = nr_frame.history_weight(head)
        slope = np.float32(-255.0 * hold / nr_daemon.HOLD_RAMP)
        same(f"temporal composition, hold {hold}"
             f"{', masked' if control is not None else ''}"
             f"{'' if with_previous else ', no previous frame'}",
             nr_image.compose_temporal(head, colour, history,
                                       previous if with_previous and hold > 0 else None,
                                       gate, control, intensity=1.0,
                                       blend_scale=float(scale), hold=hold,
                                       slope=float(slope)),
             reference)

    # The whole temporal path native: the gate from its table, the confidence, the floor and
    # the release from the previous frame inside the pass, against the NumPy that computes
    # each of them.
    for hold, with_previous, control, confidence, release in (
            (1.0, True, None, 1.0, 0.0), (0.5, True, mask, 1.0, 0.0), (1.0, True, None, 0.6, 0.0),
            (0.0, True, None, 1.0, 0.0), (1.0, False, None, 1.0, 0.0), (0.25, True, mask, 0.0, 0.0),
            (1.0, True, None, 1.0, 16.0), (0.0, True, None, 1.0, 16.0), (0.5, True, mask, 0.6, 8.0),
            (1.0, True, None, 1.0, 1.0), (1.0, False, None, 1.0, 16.0), (1.0, True, None, 1.0, 64.0)):
        before = previous if with_previous else None
        with numpy_only():
            reference = nr_frame.compose(head, colour, intensity=1.0, history=history,
                                         history_confidence=confidence,
                                         history_previous=before, history_hold=hold,
                                         history_release=release, control_mask=control)
        same(f"native temporal path, hold {hold}, confidence {confidence}, release {release:g}"
             f"{', masked' if control is not None else ''}"
             f"{'' if with_previous else ', no previous frame'}",
             nr_frame.compose(head, colour, intensity=1.0, history=history,
                              history_confidence=confidence, history_previous=before,
                              history_hold=hold, history_release=release,
                              control_mask=control),
             reference)

    # What the release is for: where the game's pixel moved by the release or more, nothing
    # of the previous output is left — the frame is the still one there, bit for bit.
    released = nr_frame.release_factor(colour, previous, nr_frame.release_slope(16.0))[..., 0] == 0
    temporal = nr_frame.compose(head, colour, intensity=1.0, history=history,
                                history_previous=previous, history_hold=1.0,
                                history_release=16.0)
    still = nr_frame.compose(head, colour, intensity=1.0)
    same(f"released pixels carry no history ({int(released.sum())} of them)",
         temporal[released], still[released])
    if not 100 < int(released.sum()) < released.size // 2:
        FAILURES.append("the release test's moving band does not move")


def fused_checks(rng):
    """The head's upscale, the composition and the codec in one pass (`nr_compose_encode`)
    against the three separate passes the daemon runs otherwise: the same composition, and
    the same bytes in the answer — inside a letterbox too, where everything outside the
    active region must come back as the game sent it."""
    frame_h, frame_w = 40, 56
    cases = 0
    for (top, bottom, left, right), (head_h, head_w), bgra in (
            ((0, 40, 0, 56), (14, 20), True), ((6, 34, 0, 56), (10, 20), True),
            ((0, 40, 4, 52), (40, 16), False), ((3, 37, 2, 54), (34, 52), True),
            ((0, 40, 0, 56), (40, 56), False), ((5, 35, 8, 48), (11, 13), False)):
        raw = rng.integers(0, 256, (frame_h, frame_w, 4), dtype=np.uint8)
        fmt = 44 if bgra else 37
        whole = nr_daemon.decode(raw.tobytes(), frame_w, frame_h, fmt)
        colour = whole[top:bottom, left:right]
        height, width = colour.shape[:2]
        history = rng.random((height, width, 3), dtype=np.float32)
        previous = colour.copy()
        previous[: height // 3] = rng.random((height // 3, width, 3), dtype=np.float32)
        previous[height // 3: height // 3 + 2] += np.float32(1.0 / 255)
        # the head as the daemon holds it: a crop of a wider, deeper network output
        network = ((rng.random((head_h + 6, head_w + 4, 16), dtype=np.float32) - 0.5) * 2)
        head = network[3:3 + head_h, 1:1 + head_w]
        mask = np.ones((height, width, 3), np.float32)
        mask[height // 2:, : width // 3, 0] = 0.0
        for temporal, hold, release, confidence, control, intensity in (
                (True, 1.0, 24.0, 1.0, None, 1.0), (True, 1.0, 0.0, 1.0, None, 1.0),
                (True, 0.0, 0.0, 0.6, None, 1.0), (True, 0.5, 8.0, 1.0, mask, 1.0),
                (True, 1.0, 24.0, 1.0, None, 1.66), (False, 0.0, 0.0, 1.0, None, 1.0),
                (False, 0.0, 0.0, 1.0, None, 0.6), (False, 0.0, 0.0, 1.0, None, 1.66)):
            channels = 4 if temporal else 3
            up = nr_daemon.resample(head[..., :channels], (height, width))
            options = dict(intensity=intensity, control_mask=control)
            if temporal:
                options.update(history=history, history_confidence=confidence,
                               history_previous=previous, history_hold=hold,
                               history_release=release)
            reference = nr_frame.compose(up, colour, **options)
            full = whole.copy()
            full[top:bottom, left:right] = reference
            expected = nr_daemon.encode(full, raw.tobytes(), fmt)
            encoded = raw.copy()
            fused, samples = nr_frame.compose_encode(head[..., :channels], colour, encoded,
                                                     top=top, left=left, bgra=bgra, samples=8,
                                                     **options)
            name = (f"fused pass, region {top}:{bottom}x{left}:{right}, head {head_h}x{head_w}, "
                    + (f"hold {hold:g}, release {release:g}, confidence {confidence:g}"
                       if temporal else "still") + f", intensity {intensity:g}"
                    + (", masked" if control is not None else ""))
            same(name + ", composition", fused, reference)
            same(name + ", bytes", encoded.tobytes(), expected)
            same(name + ", head samples", samples, up[::8, ::8])
            cases += 1
    check(f"fused pass: {cases} cases", cases == 48)


def grade_checks(rng):
    """The vendor's style grade and the history it keeps (`nr_frame.compose`'s `grade` and
    `neural`), native against NumPy: the grade's own edge cases — greys, primaries, hues
    on the boundaries of its six segments, 0 and 1 — through a head of zero, then random
    frames still and temporal, masked, at three intensities and three grades."""
    grades = [nr_frame.grade_for(1 / 128, 1.0), nr_frame.grade_for(2 / 128, 1.0),
              nr_frame.grade_for(1 / 128, 0.37)]
    check("grades: natural and cinematic from the DLL's table, none for style 0 or tone 0",
          grades[0] is not None and grades[1] is not None
          and nr_frame.grade_for(0.0, 1.0) is None and nr_frame.grade_for(1 / 128, 0.0) is None)
    special = [(0, 0, 0), (1, 1, 1), (0.5, 0.5, 0.5), (1, 0, 0), (0, 1, 0), (0, 0, 1),
               (1, 1, 0), (0, 1, 1), (1, 0, 1), (1, 0.5, 0), (0.5, 1, 0), (0, 1, 0.5),
               (0, 0.5, 1), (0.5, 0, 1), (1, 0, 0.5), (0.2, 0.2, 0.2001), (0.9, 0.3, 0.3),
               (0.25, 0.75, 0.5), (1.0, 0.999, 0.998), (0.001, 0.0, 0.0005)]
    edge = np.array(special, np.float32)
    edge = np.concatenate([edge, rng.random((236, 3), dtype=np.float32)]).reshape(16, 16, 3)
    zero = np.zeros((16, 16, 3), np.float32)
    for grade in grades:
        kept = np.empty(edge.shape, np.float32)
        wanted = np.empty(edge.shape, np.float32)
        with numpy_only():
            reference = nr_frame.compose(zero, edge, grade=grade, neural=wanted)
        same(f"grade {tuple(float(v) for v in grade)} on its edge cases",
             nr_image.compose(zero, edge, 1.0, grade=grade, neural=kept), reference)
        same("  and the history it keeps", kept, wanted)
        same("  the grade itself", nr_frame.style_grade(nr_frame.truncate_half(edge), grade),
             nr_frame.style_grade(nr_frame.truncate_half(edge), grade))
    height, width = 40, 64
    colour = rng.random((height, width, 3), dtype=np.float32)
    history = nr_frame.truncate_half(rng.random((height, width, 3), dtype=np.float32))
    previous = colour.copy()
    previous[8:16] = rng.random((8, width, 3), dtype=np.float32)
    head = (rng.random((height, width, 4), dtype=np.float32) - 0.5).astype(np.float32) * 2
    mask = np.ones((height, width, 3), np.float32)
    mask[20:30, 5:25, 0] = 0.0
    cases = 0
    for grade in (None, *grades):
        for temporal in (False, True):
            for intensity, control in ((1.0, None), (0.6, None), (1.66, None), (1.0, mask)):
                options = dict(intensity=intensity, control_mask=control, grade=grade)
                if temporal:
                    options.update(history=history, history_previous=previous,
                                   history_hold=1.0, history_release=24.0)
                kept = np.empty(colour.shape, np.float32)
                wanted = np.empty(colour.shape, np.float32)
                with numpy_only():
                    reference = nr_frame.compose(head, colour, neural=wanted, **options)
                name = (f"{'temporal' if temporal else 'still'} composition, grade "
                        f"{'none' if grade is None else tuple(float(v) for v in grade)}, "
                        f"intensity {intensity:g}{', masked' if control is not None else ''}")
                same(name, nr_frame.compose(head, colour, neural=kept, **options), reference)
                same(name + ", history kept", kept, wanted)
                cases += 1
                if grade is None:
                    # nothing asked of the history, the composition is what it always was
                    with numpy_only():
                        plain = nr_frame.compose(head, colour, **dict(options, grade=None))
                    same(name + ", unchanged by keeping the history", reference, plain)
    check(f"grade and history: {cases} cases", cases == 32)


def fused_grade_checks(rng):
    """The fused pass keeps the history on its fast rows and grades on its general loop;
    both against the separate passes."""
    frame_h, frame_w = 36, 52
    cases = 0
    for (top, bottom, left, right), (head_h, head_w), bgra in (
            ((0, 36, 0, 52), (12, 18), True), ((4, 32, 2, 50), (28, 48), False)):
        raw = rng.integers(0, 256, (frame_h, frame_w, 4), dtype=np.uint8)
        fmt = 44 if bgra else 37
        whole = nr_daemon.decode(raw.tobytes(), frame_w, frame_h, fmt)
        colour = whole[top:bottom, left:right]
        height, width = colour.shape[:2]
        history = nr_frame.truncate_half(rng.random((height, width, 3), dtype=np.float32))
        previous = colour.copy()
        previous[: height // 3] = rng.random((height // 3, width, 3), dtype=np.float32)
        network = ((rng.random((head_h + 4, head_w + 4, 16), dtype=np.float32) - 0.5) * 2)
        head = network[2:2 + head_h, 1:1 + head_w]
        for grade in (None, nr_frame.grade_for(1 / 128, 1.0), nr_frame.grade_for(2 / 128, 1.0)):
            for temporal, intensity in ((True, 1.0), (True, 0.6), (False, 1.0), (False, 1.66)):
                channels = 4 if temporal else 3
                up = nr_daemon.resample(head[..., :channels], (height, width))
                options = dict(intensity=intensity, grade=grade)
                if temporal:
                    options.update(history=history, history_previous=previous,
                                   history_hold=1.0, history_release=24.0)
                wanted = np.empty(colour.shape, np.float32)
                reference = nr_frame.compose(up, colour, neural=wanted, **options)
                full = whole.copy()
                full[top:bottom, left:right] = reference
                expected = nr_daemon.encode(full, raw.tobytes(), fmt)
                encoded = raw.copy()
                kept = np.empty(colour.shape, np.float32)
                fused = nr_frame.compose_encode(head[..., :channels], colour, encoded, top=top,
                                                left=left, bgra=bgra, neural=kept, **options)
                name = (f"fused pass with the history kept, region {top}:{bottom}x{left}:{right},"
                        f" {'temporal' if temporal else 'still'}, intensity {intensity:g}, grade "
                        f"{'none' if grade is None else tuple(float(v) for v in grade)}")
                same(name + ", composition", fused, reference)
                same(name + ", bytes", encoded.tobytes(), expected)
                same(name + ", history", kept, wanted)
                cases += 1
    check(f"fused pass with grade and history: {cases} cases", cases == 24)


def main():
    if nr_image.library() is None:
        print("  no work/libnr_image.so (.dylib on macOS) — `make` builds it; the NumPy path still runs")
        return 0
    rng = np.random.default_rng(17)
    codec_checks(rng)
    resize_checks(rng)
    feature_checks(rng)
    half_checks()
    compose_checks(rng)
    fused_checks(rng)
    grade_checks(rng)
    fused_grade_checks(rng)
    if FAILURES:
        print(f"\n{len(FAILURES)} FAILED: " + ", ".join(FAILURES), flush=True)
        return 1
    print("\nthe native passes are byte-identical to the NumPy they replace", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
