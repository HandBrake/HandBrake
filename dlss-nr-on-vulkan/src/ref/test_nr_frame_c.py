#!/usr/bin/env python3
"""The C frame library against the Python it transcribes.

The claim under test is that `src/ref/nr_frame.c` records the graph dispatch for dispatch
as `nr_frame_resident.py` does, so the head is **bit-identical** on the same device; and
that the frame around it — features in, composition out — is the same to the last bit
where NumPy and C share the arithmetic, and to a named tolerance where they do not (the
noise's transcendentals, the gate's exponential, the detail blur's kernel).

Runs on the real weights when they exist, and otherwise on **random weights of the real
shapes**, written as a logical safetensors file from MLX-DLSS's `weight_spec.json`. A
graph on random weights is still the graph: every dispatch, every layout, every publish,
and the bit-identity claim is about those. What it cannot check is a picture.
"""
import json
import os
import pathlib
import struct
import sys
import tempfile

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))
sys.path.insert(0, str(ROOT / "src" / "gpu"))

import nr_frame  # noqa: E402
import nr_model  # noqa: E402
import nr_frame_native  # noqa: E402
import nr_image  # noqa: E402

SPEC = ROOT / "work" / "mlx-dlss" / "python" / "mlxdlss" / "weight_spec.json"
failures = []


def check(name, ok, detail=""):
    print(f"  [{'ok  ' if ok else 'FAIL'}] {name}  {detail}")
    if not ok:
        failures.append(name)


def synthetic_weights(path, seed=3):
    """Random weights of the real shapes, in the logical safetensors format."""
    spec = json.loads(SPEC.read_text())["tensors"]
    rng = np.random.default_rng(seed)
    header, chunks, offset = {"__metadata__": {"fully_logical": "true", "format": "test"}}, [], 0
    for name, entry in spec.items():
        shape = tuple(entry["shape"])
        count = int(np.prod(shape)) if shape else 1
        if name.endswith("attn_scale"):
            data = rng.uniform(0.5, 1.5, count).astype(np.float32)
        elif name.endswith(("_sin", "sin", "_cos", "cos_skip")):
            data = rng.uniform(0.3, 1.0, count).astype(np.float32)
        else:
            fan = shape[0] if len(shape) > 1 else 1
            data = (rng.standard_normal(count) / np.sqrt(max(fan, 1))).astype(np.float32)
        dtype = entry["dtype"]
        raw = data.astype(np.float16 if dtype == "F16" else np.float32).tobytes()
        header[name] = {"dtype": dtype, "shape": list(shape), "data_offsets": [offset, offset + len(raw)]}
        chunks.append(raw)
        offset += len(raw)
    encoded = json.dumps(header).encode()
    encoded += b" " * (-len(encoded) % 8)
    with open(path, "wb") as out:
        out.write(struct.pack("<Q", len(encoded)))
        out.write(encoded)
        for chunk in chunks:
            out.write(chunk)


def write_reference(path, features, head):
    """The other side for `work/test_nr_frame`, the C port of this test: the features
    the Python backend saw and the head it produced, so a process with no Python in it can
    still require the same bytes back. Magic, height, width, channels; then the floats."""
    H, W = features.shape[:2]
    with open(path, "wb") as out:
        out.write(struct.pack("<4I", 0x4E524631, H, W, 16))
        out.write(np.ascontiguousarray(features, np.float32).tobytes())
        out.write(np.ascontiguousarray(head, np.float32).tobytes())


def main():
    reference = None
    if "--reference" in sys.argv:
        reference = pathlib.Path(sys.argv[sys.argv.index("--reference") + 1])
    if not nr_frame_native.LIBRARY.exists():
        print(f"nr_frame C: skipped (make {nr_frame_native.LIBRARY.name} first) — a skip is not a pass")
        return 0
    with tempfile.TemporaryDirectory(prefix="nr-frame-c-") as room:
        weights = nr_frame.WEIGHTS
        if weights.exists():
            print(f"real weights: {weights}")
        else:
            weights = pathlib.Path(room) / "random-logical.safetensors"
            synthetic_weights(weights)
            print(f"no logical weights; random weights of the real shapes at {weights}")
        rng = np.random.default_rng(5)
        height, width = 200, 176                     # network extent 320x320, both mirrored
        colour = rng.uniform(0, 1, (height, width, 3)).astype(np.float32)
        history = np.clip(colour + rng.normal(0, 0.05, colour.shape), 0, 1).astype(np.float32)
        previous = colour.copy()
        previous[::3] = np.clip(previous[::3] + 0.1, 0, 1)      # two thirds of the rows unchanged

        native = nr_frame_native.NativeFrame(weights)
        print(f"C library on {native.device}: {native.gemm_path}")
        backend = nr_frame.ResidentBackend(weights)

        # 0. what the library says of its runtime agrees with the runtime's own exports
        # (the same library instance, loaded by both), and is a yes or a no
        lib = backend.runtime.lib
        for name, got in (("discrete", native.discrete), ("preserve16", native.preserve16),
                          ("half_by_cast", native.half_by_cast), ("staged_packed", native.staged_packed),
                          ("portable_packed", native.portable_packed)):
            want = int(getattr(lib, "xmx_" + name)())
            check(f"runtime: nr_frame_{name} is the runtime's {want}", got == want and got in (0, 1),
                  f"got {got}")
        check("runtime: the features go into the mapped input unless NR_INPUT_VIEW says not",
              native.input_view == (0 if os.environ.get("NR_INPUT_VIEW") == "0" else 1)
              or (sys.platform == "win32" and native.discrete == 1),
              f"input_view {native.input_view}")

        # 0b. a prepared file: the layout work done once, and an open from it gives the same
        # head bytes as an open from the logical file
        if hasattr(native.lib, "nr_frame_prepare"):
            prepared = pathlib.Path(room) / "prepared.safetensors"
            nr_frame_native.prepare(prepared, weights)
            with open(prepared, "rb") as fh:
                header = json.loads(fh.read(struct.unpack("<Q", fh.read(8))[0]))
            derived = [k for k in header if k.endswith("_fused") or k.endswith("_unswizzled")
                       or k.endswith("head_matrix") or k.endswith("inp_merge_sincos")]
            check("prepared: the file carries the derived tensors and says so",
                  header.get("__metadata__", {}).get("prepared") == "v2" and len(derived) >= 4
                  and header["block70.layer0.head_matrix"]["dtype"] == "F16", f"{len(derived)} derived")
            # v2: F32 tensors first, then F16, so every offset is aligned to its element
            offsets = [(entry["data_offsets"][0], entry["dtype"]) for key, entry in header.items()
                       if key != "__metadata__"]
            check("prepared: every offset is aligned to its element (F32 first, then F16)",
                  all(off % (4 if dtype == "F32" else 2) == 0 for off, dtype in offsets)
                  and [d for _, d in sorted(offsets)] == sorted((d for _, d in offsets), reverse=True))
            plain_head = native.head(colour)
            from_prepared = nr_frame_native.NativeFrame(str(prepared))
            check("prepared: the head from a prepared open is the logical open's bit for bit",
                  np.array_equal(plain_head, from_prepared.head(colour)))
            aliased, count = from_prepared.weights_aliased
            check("prepared: the reader aliases every tensor of a prepared file, none copied",
                  from_prepared.prepared == 1 and count > 0 and aliased == count, f"{aliased} of {count}")
            from_prepared.close()
            del from_prepared
            # preparing the prepared file keeps what is there: the same tensors, no more
            again = pathlib.Path(room) / "prepared-again.safetensors"
            nr_frame_native.prepare(again, prepared)
            with open(again, "rb") as fh:
                header2 = json.loads(fh.read(struct.unpack("<Q", fh.read(8))[0]))
            check("prepared: preparing a prepared file is idempotent",
                  sorted(k for k in header2 if k != "__metadata__") == sorted(k for k in header if k != "__metadata__"))
            # the compiled-in weights are this format, read in place
            if native.lib.nr_frame_embedded_weights_size() > 8:
                embedded = nr_frame_native.NativeFrame()
                aliased, count = embedded.weights_aliased
                check("prepared: the compiled-in weights are prepared and every tensor is read in place",
                      embedded.prepared == 1 and count > 0 and aliased == count,
                      f"prepared {embedded.prepared}, {aliased} of {count} in place")
                check("prepared: the compiled-in open gives the logical file's head bit for bit",
                      np.array_equal(plain_head, embedded.head(colour)))
                embedded.close()
                del embedded
            # the Python side reads the prepared file too, and derives nothing it carries
            pw, pm = nr_model.load_logical(prepared, keep_half=True)
            check("prepared: the Python loader takes the prepared file, 649 logical tensors among its own",
                  nr_model.logical_count(pw) == 649 and pm.get("prepared") == "v2"
                  and pw["block70.layer0.head_matrix"].dtype == np.float16)
        else:
            print("  [skip] prepared file: this libnr_frame has no nr_frame_prepare")

        # 1. features: the same recipe, NumPy against C
        geometry = nr_frame.NetworkGeometry.vendor_aligned(width, height)
        H, W = geometry.network_height, geometry.network_width
        check("network extent", nr_frame_native.geometry(height, width) == (H, W), f"{H}x{W}")
        values = nr_frame.PROFILES["standard"]
        py_features = nr_frame.build_features(colour, geometry=geometry, **values)
        c_features = native.features(colour)
        exact = np.array_equal(py_features[..., 3:], c_features[..., 3:])
        check("features: colour, constant and control channels bit-identical", exact)
        noise_diff = np.abs(py_features[..., :3] - c_features[..., :3])
        differing = int((noise_diff > 0).sum())
        check("features: noise channels within a half ulp",
              float(noise_diff.max()) <= 2 ** -10 * float(np.abs(py_features[..., :3]).max() + 1),
              f"{differing} of {noise_diff.size} values differ, max {noise_diff.max():.3e}")
        py_hist = nr_frame.build_features(colour, geometry=geometry, history=history, **values)
        c_hist = native.features(colour, history=history)
        check("features with history: channels 3-15 bit-identical",
              np.array_equal(py_hist[..., 3:], c_hist[..., 3:]))

        # 1b. the two other ways of filling channels 10-14: the automatic mask and a control mask
        auto = nr_frame.AutomaticMask(skin_structure_strength=2.0, automatic_mask_structure_strength=-1.0)
        py_auto = nr_frame.make_features(colour, geometry=geometry, automatic_mask=auto, **values)
        c_auto = native.features(colour, automatic_mask=1, skin_structure=2.0, automatic_structure=-1.0)
        check("features: automatic mask channels 3-15 bit-identical",
              np.array_equal(py_auto[..., 3:], c_auto[..., 3:]))
        mask = rng.uniform(0, 1, colour.shape).astype(np.float32)
        py_masked = nr_frame.make_features(colour, geometry=geometry, control_mask=mask,
                                           local_tone_strength=1.3, local_structure_strength=0.7)
        c_masked = native.features(colour, control_mask=mask, local_tone=1.3, local_structure=0.7)
        check("features: control mask channels 3-15 bit-identical",
              np.array_equal(py_masked[..., 3:], c_masked[..., 3:]))

        # 2. the graph: the same features through both, bit for bit
        py_head = backend.run_features(c_features)
        c_head = native.run_features(c_features)
        if reference is not None:
            write_reference(reference, c_features, py_head)
            print(f"  reference for work/test_nr_frame written to {reference}")
        same = np.array_equal(py_head, c_head)
        check("head: C graph bit-identical to the Python resident path", same,
              "" if same else f"max |d| {np.abs(py_head - c_head).max():.3e}, "
                              f"{int((py_head != c_head).sum())} of {c_head.size} differ")
        check("head: finite", bool(np.isfinite(c_head).all()))
        py_head2 = backend.run_features(c_features)
        check("head: the captured graph replays the same bytes", np.array_equal(py_head2, c_head))

        # 3. composition on the same head: the still path is shared C and must be exact
        cropped = geometry.crop(c_head)
        c_out, c_head_out = native.update(colour, want_head=True)
        py_head_c = geometry.crop(backend.run_features(c_features))
        check("update: the head it hands back is the graph's", np.array_equal(c_head_out, py_head_c))
        py_out = nr_frame.compose(py_head_c, colour)
        check("compose: still frame bit-identical", np.array_equal(c_out, py_out),
              f"max |d| {np.abs(c_out - py_out).max():.3e}")
        for intensity in (0.0, 0.5, 1.66):
            c_i = native.update(colour, intensity=intensity)
            py_i = nr_frame.compose(py_head_c, colour, intensity=intensity)
            check(f"compose: intensity {intensity} bit-identical", np.array_equal(c_i, py_i),
                  f"max |d| {np.abs(c_i - py_i).max():.3e}")
        check("compose: intensity 0 returns the source", np.array_equal(
            native.update(colour, intensity=0.0), colour))

        # 3b. the composition on its own, and with a control mask, against the Python's
        for intensity in (0.5, 1.66):
            c_m = native.compose(py_head_c, colour, control_mask=mask, intensity=intensity)
            py_m = nr_frame.compose(py_head_c, colour, control_mask=mask, intensity=intensity)
            check(f"compose: control mask at intensity {intensity} bit-identical",
                  np.array_equal(c_m, py_m), f"max |d| {np.abs(c_m - py_m).max():.3e}")
        # update builds its features straight into the graph's half input under
        # NR_INPUT_FP16, the mask channels included: the head must be the graph's on the
        # float32 features the Python recipe gives
        _, c_m_head = native.update(colour, control_mask=mask, local_tone=1.3,
                                    local_structure=0.7, want_head=True)
        check("update: with a control mask, the head is the graph's on its features",
              np.array_equal(c_m_head, geometry.crop(backend.run_features(c_masked))))
        _, c_a_head = native.update(colour, automatic_mask=1, skin_structure=2.0,
                                    automatic_structure=-1.0, want_head=True)
        check("update: with the automatic mask, the head is the graph's on its features",
              np.array_equal(c_a_head, geometry.crop(backend.run_features(c_auto))))
        check("compose: the standalone composition is update's", np.array_equal(
            native.compose(py_head_c, colour, intensity=0.5), native.update(colour, intensity=0.5)))

        # 4. the detail split: NumPy's blur against C's, exp in the kernel the one difference
        c_d = native.update(colour, detail_strength=1.5, colour_strength=0.0)
        py_d = nr_frame.compose(py_head_c, colour, detail_strength=1.5, colour_strength=0.0)
        d = np.abs(c_d - py_d)
        check("compose: detail/colour split within 1e-6", float(d.max()) <= 1e-6,
              f"max |d| {d.max():.3e}, {int((d > 0).sum())} of {d.size} differ")

        # 5. the temporal path: the same history through both; the gate's exp is the one place
        #    the C may differ from NumPy by a last bit, so a tolerance, and an exact check with
        #    the C's own gate fed to the shared composition
        c_t, c_t_head = native.update(colour, history=history, previous=previous, want_head=True,
                                      hold=0.6, slope=-0.6 * 255 / 4)
        hist_features = native.features(colour, history=history)
        py_t_head = geometry.crop(backend.run_features(hist_features))
        check("temporal: the head is the graph's on the history features",
              np.array_equal(c_t_head, py_t_head))
        moved = np.abs(colour - previous).max(axis=2, keepdims=True)
        floor = np.clip(1.0 - moved * 255 / 4.0, 0, 1) * 0.6
        py_t = nr_frame.compose(py_t_head, colour, history=history, history_floor=floor)
        t = np.abs(c_t - py_t)
        check("temporal: composed frame within 2e-6 of the Python (gate exp, floor fold)",
              float(t.max()) <= 2e-6, f"max |d| {t.max():.3e}, {int((t > 0).sum())} of {t.size} differ")
        # the floor itself, exactly: NumPy's alpha through C's composition
        alpha = nr_frame.history_weight(py_t_head)
        np.maximum(alpha, floor.astype(np.float32) * np.float32(nr_frame.BLEND_SCALE), out=alpha)
        shared = nr_image.compose_temporal(py_t_head, colour, history, None, alpha, None,
                                           intensity=1.0, blend_scale=nr_frame.BLEND_SCALE,
                                           hold=0.0, slope=0.0)
        py_no_floor = nr_frame.compose(py_t_head, colour, history=history)
        check("temporal: the floor holds unchanged rows harder than the gate alone",
              float(np.abs(c_t - history)[1::3].mean()) < float(np.abs(py_no_floor - history)[1::3].mean()))
        check("temporal: shared composition is the NumPy contract",
              shared is not None and np.array_equal(shared, py_t))
        # the release through the C library: the folded slope in the parameters, against
        # NumPy's release on the same head, the floor and the gate as above
        slope_r = nr_frame.release_slope(16.0)
        c_r = native.update(colour, history=history, previous=previous,
                            hold=0.6, slope=-0.6 * 255 / 4, release=slope_r)
        py_r = nr_frame.compose(py_t_head, colour, history=history, history_previous=previous,
                                history_hold=0.6, history_release=16.0)
        r = np.abs(c_r - py_r)
        check("temporal: the release within 2e-6 of the Python (gate exp)",
              float(r.max()) <= 2e-6, f"max |d| {r.max():.3e}, {int((r > 0).sum())} of {r.size} differ")
        gone = nr_frame.release_factor(colour, previous, slope_r)[..., 0] == 0
        c_still = native.update(colour, history=history, history_confidence=0.0)
        check("temporal: released pixels are the confidence-0 frame, bit for bit",
              int(gone.sum()) > gone.size // 10 and np.array_equal(c_r[gone], c_still[gone]),
              f"{int(gone.sum())} of {gone.size} released")

        # 6. a second extent rebuilds the graph and keeps the weights
        small = rng.uniform(0, 1, (96, 128, 3)).astype(np.float32)
        c_small = native.update(small)
        g2 = nr_frame.NetworkGeometry.vendor_aligned(128, 96)
        py_small = nr_frame.compose(g2.crop(backend.run_features(native.features(small))), small)
        check("second extent: bit-identical after the rebuild", np.array_equal(c_small, py_small))
        c_again = native.update(colour)
        check("back to the first extent: bit-identical again", np.array_equal(c_again, py_out))
        # 7. min_extent: the graph's own floor, 128, where the bottleneck is 16 tokens on the
        #    32-row paths (the staged 32-row builds, the 32-row pad, global attention)
        for floor in (128, 256):
            g3 = nr_frame.network_geometry(width, height, minimum=floor)
            H3, W3 = g3.network_height, g3.network_width
            check(f"min_extent {floor}: network extent",
                  nr_frame_native.geometry(height, width, floor) == (H3, W3), f"{H3}x{W3}")
            c3 = native.features(colour, min_extent=floor)
            py3 = nr_frame.build_features(colour, geometry=g3, **values)
            check(f"min_extent {floor}: features channels 3-15 bit-identical",
                  c3.shape == py3.shape and np.array_equal(c3[..., 3:], py3[..., 3:]))
            _, c3_head = native.update(colour, want_head=True, min_extent=floor)
            check(f"min_extent {floor}: head bit-identical to the Python resident path",
                  np.array_equal(c3_head, g3.crop(backend.run_features(c3))))
        check("min_extent 0 is the vendor's 320", nr_frame_native.geometry(height, width, 0) == (H, W))

        # 8. the daemon's end of a frame: the head at a render scale brought up, composed and
        #    encoded in one pass, against nr_frame.compose_encode
        small_head = rng.normal(0, 1, (height * 3 // 5, width * 3 // 5, 4)).astype(np.float32)
        request = rng.integers(0, 256, (height + 6, width + 10, 4), dtype=np.uint8)
        hold, levels = 0.6, 16.0
        cases = [("still", {}, {}),
                 ("still, intensity 1.66", {"intensity": 1.66}, {"intensity": 1.66}),
                 ("history", {"history": history}, {"history": history}),
                 ("history, floor and release",
                  {"history": history, "previous": previous, "hold": hold,
                   "slope": float(np.float32(-255.0 * hold / nr_frame.HOLD_RAMP)),
                   "release": nr_frame.release_slope(levels)},
                  {"history": history, "history_previous": previous, "history_hold": hold,
                   "history_release": levels}),
                 ("history, control mask, confidence 0.5",
                  {"history": history, "control_mask": mask, "history_confidence": 0.5},
                  {"history": history, "control_mask": mask, "history_confidence": 0.5})]
        for name, c_args, py_args in cases:
            c_enc, py_enc = request.copy(), request.copy()
            c_args = dict(c_args)
            c_hist, c_prev, c_mask = (c_args.pop(k, None) for k in ("history", "previous", "control_mask"))
            c_out = native.compose_encode(small_head, colour, c_enc, top=3, left=5, bgra=True,
                                          history=c_hist, previous=c_prev, control_mask=c_mask, **c_args)
            py_out = nr_frame.compose_encode(small_head, colour, py_enc, top=3, left=5, bgra=True,
                                             **py_args)
            check(f"compose_encode, {name}: composition and bytes bit-identical",
                  py_out is not None and np.array_equal(c_out, py_out) and np.array_equal(c_enc, py_enc))
        # where the detail split is not a no-op, the separate passes, the same bytes
        c_enc = request.copy()
        c_out = native.compose_encode(small_head, colour, c_enc, top=3, left=5, bgra=True,
                                      detail_strength=1.5, colour_strength=0.0)
        up = nr_image.bilinear(small_head, (height, width))
        want = native.compose(up, colour, detail_strength=1.5, colour_strength=0.0)
        want_enc = request.copy()
        want_enc[3:3 + height, 5:5 + width] = np.frombuffer(
            nr_image.encode8(want, request[3:3 + height, 5:5 + width].tobytes(), True),
            np.uint8).reshape(height, width, 4)
        check("compose_encode with the detail split: the separate passes, bit for bit",
              np.array_equal(c_out, want) and np.array_equal(c_enc, want_enc))

        # 9. the vendor's post-process (notes/phase70): the style's grade, which the C takes
        #    from the parameters, and the history it keeps, through each composition path
        grades_agree = True
        for style in (0, 1, 2, 3):
            for tone in (1.0, 0.37, 1.8, 0.0, -0.5):
                c_g = nr_frame_native.grade(normalized_style=style / 128, local_tone=tone)
                py_g = nr_frame.grade_for(normalized_style=style / 128, local_tone_strength=tone)
                grades_agree &= (c_g is None) == (py_g is None) and (
                    c_g is None or all(np.float32(a) == np.float32(b) for a, b in zip(c_g, py_g)))
        check("grade: the C's factors are grade_for's, every style and tone", grades_agree)
        check("grade: grade_off turns it off",
              nr_frame_native.grade(normalized_style=1 / 128, grade_off=1) is None)
        for style in (1, 2):
            grade = nr_frame.grade_for(normalized_style=style / 128, local_tone_strength=0.8)
            styled = {"normalized_style": style / 128, "local_tone": 0.8}
            for name, c_args, py_args in (
                    ("still", {}, {}),
                    ("intensity 0.6", {"intensity": 0.6}, {"intensity": 0.6}),
                    ("intensity 1.66", {"intensity": 1.66}, {"intensity": 1.66}),
                    ("control mask", {"control_mask": mask}, {"control_mask": mask}),
                    ("detail split", {"detail_strength": 1.5, "colour_strength": 0.0},
                     {"detail_strength": 1.5, "colour_strength": 0.0})):
                c_n, py_n = np.empty_like(colour), np.empty_like(colour)
                c_o = native.compose(py_head_c, colour, neural=c_n, **styled, **c_args)
                py_o = nr_frame.compose(py_head_c, colour, grade=grade, neural=py_n, **py_args)
                exact = name != "detail split"      # the blur's expf, as in 4.
                d = float(np.abs(c_o - py_o).max())
                check(f"style {style}, {name}: graded composition "
                      + ("bit-identical" if exact else "within 1e-6"),
                      (d == 0.0) if exact else d <= 1e-6, f"max |d| {d:.3e}")
                check(f"style {style}, {name}: the kept history bit-identical",
                      np.array_equal(c_n, py_n))
            # the temporal path: the gate's exp, so a tolerance, as in 5.
            c_n, py_n = np.empty_like(colour), np.empty_like(colour)
            c_o = native.compose(py_t_head, colour, history=history, previous=previous,
                                 neural=c_n, hold=0.6, slope=-0.6 * 255 / 4,
                                 release=slope_r, **styled)
            py_o = nr_frame.compose(py_t_head, colour, history=history, history_previous=previous,
                                    history_hold=0.6, history_release=16.0, grade=grade,
                                    neural=py_n)
            check(f"style {style}, temporal: graded composition and history within 1e-5",
                  float(np.abs(c_o - py_o).max()) <= 1e-5 and float(np.abs(c_n - py_n).max()) <= 1e-5,
                  f"max |d| {np.abs(c_o - py_o).max():.3e} / {np.abs(c_n - py_n).max():.3e}")
            # update, which grades as run_frame does, and hands back the vendor's history
            c_n, py_n = np.empty_like(colour), np.empty_like(colour)
            c_u, c_u_head = native.update(colour, want_head=True, neural=c_n, **styled)
            py_u = nr_frame.compose(c_u_head, colour, grade=grade, neural=py_n)
            check(f"style {style}, update: graded and its history bit-identical",
                  np.array_equal(c_u, py_u) and np.array_equal(c_n, py_n))
            check(f"style {style}, update: grade_off is the ungraded frame", np.array_equal(
                native.update(colour, grade_off=1, **styled),
                nr_frame.compose(c_u_head, colour)))
            # the daemon's end of the frame with a grade: fused and separate, still and temporal
            for name, c_args, py_args in (
                    ("still", {}, {}),
                    ("history, floor and release",
                     {"history": history, "previous": previous, "hold": hold,
                      "slope": float(np.float32(-255.0 * hold / nr_frame.HOLD_RAMP)),
                      "release": nr_frame.release_slope(levels)},
                     {"history": history, "history_previous": previous, "history_hold": hold,
                      "history_release": levels})):
                c_enc, py_enc = request.copy(), request.copy()
                c_n, py_n = np.empty_like(colour), np.empty_like(colour)
                c_args = dict(c_args)
                c_hist, c_prev = c_args.pop("history", None), c_args.pop("previous", None)
                c_o = native.compose_encode(small_head, colour, c_enc, top=3, left=5, bgra=True,
                                            history=c_hist, previous=c_prev, neural=c_n,
                                            **styled, **c_args)
                py_o = nr_frame.compose_encode(small_head, colour, py_enc, top=3, left=5, bgra=True,
                                               grade=grade, neural=py_n, **py_args)
                check(f"style {style}, compose_encode, {name}: bytes and history bit-identical",
                      py_o is not None and np.array_equal(c_o, py_o)
                      and np.array_equal(c_enc, py_enc) and np.array_equal(c_n, py_n))

        # 10. render_extent: the network's frame at a render scale, as the daemon picks it.
        #     The C takes the scale as float32, so the Python is handed the same value.
        disagree = []
        for w, h in ((640, 360), (800, 450), (512, 288), (1280, 720), (1920, 1080), (1024, 768),
                     (854, 480), (320, 180), (1366, 768)):
            for scale in (0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55, 0.6, 0.75, 0.9, 1.0):
                for floor in (320, 192, 128):
                    s32 = float(np.float32(scale))
                    want = nr_frame.render_extent(w, h, s32, floor)
                    got = nr_frame_native.render_extent(w, h, s32, floor)
                    if want != got:
                        disagree.append(f"{w}x{h}@{scale}/{floor}: {got} for {want}")
        check("render_extent: the C picks the Python's frame at 297 sizes, scales and floors",
              not disagree, "; ".join(disagree[:4]))

        split = native.split
        print(f"  last run: write {split[0] * 1e3:.1f} ms, graph {split[1] * 1e3:.1f} ms, "
              f"read {split[2] * 1e3:.1f} ms")
        native.close()
        backend.close()
    if failures:
        print(f"\n{len(failures)} check(s) failed: {', '.join(failures)}")
        return 1
    print("\nnr_frame in C: the graph is bit-identical to the Python, and the frame around it "
          "matches to the last bit where the arithmetic is shared")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
