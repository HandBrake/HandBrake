#!/usr/bin/env python3
"""
nr_temporal — the temporal path: rendered history, motion reprojection, learned blend.

`nr_frame` runs the *first-frame* branch, which is what a single still needs. A
sequence needs the rest of the recovered contract: the previous output reprojected
along motion vectors into feature channels 7-9, and the head's fourth channel used
as a blend logit between the freshly predicted frame and that history.

    alpha  = clamp(sigmoid(half(head[..., 3])) * half(blend_scale), 0, 1)
    output = predicted + alpha * (history - predicted)

with `blend_scale` = 0.73974609375 from the shipped package, so the history can take
at most 74 % of a pixel.

The operators are MLX-DLSS's `temporal.py`, loaded by path like `features.py` and
`composition.py` — at processing scale 1 with engine motion it needs neither OpenCV
nor Pillow. What is here is the adapter onto our model, the diagnostics that say
whether the blend is actually engaging, and a synthetic pan for testing without a
game.

    python3 src/ref/nr_temporal.py --pan 6,0 --frames 4 IN.png OUT --gpu
    python3 src/ref/nr_temporal.py --frames-from a.png,b.png,c.png OUT --gpu
"""
from __future__ import annotations

import argparse
import pathlib
import sys
import time

import numpy as np

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(HERE))

import nr_model  # noqa: E402
import nr_frame  # noqa: E402
import image_io  # noqa: E402

features_mod, composition_mod, _motion_mod, temporal_mod = nr_frame.load_mlx_numpy_modules()

NetworkGeometry = features_mod.NetworkGeometry
BLEND_SCALE = temporal_mod.BLEND_SCALE
TemporalOptions = temporal_mod.TemporalOptions
normalize_pixel_motion = temporal_mod.normalize_pixel_motion
sample_history = temporal_mod.sample_history

WEIGHTS = nr_model.default_weights()        # the prepared file when it is there


class Pipeline:
    """What `TemporalSession` expects of a network: features in, head out.

    Records the last head so the caller can report the blend the model asked for;
    the session itself never exposes it. `model` may be a `NeuralRenderingModel` or
    `nr_frame.ResidentBackend`.
    """

    def __init__(self, model):
        self.model = model
        self.head = None
        self.seconds = 0.0

    def run_features(self, features):
        started = time.perf_counter()
        features = np.asarray(features, dtype=np.float32)
        self.head = (self.model.run_features(features)
                     if hasattr(self.model, "run_features")
                     else self.model.forward(features[None])[0])
        self.seconds = time.perf_counter() - started
        return self.head


class GpuHistory:
    """A device-backed `sample_history`, installed over the reference's.

    At 720p the five-tap reprojection is 874 ms of the 1211 ms the host spends
    assembling a temporal frame — more than the graph itself now takes — and it is a
    per-pixel gather, so it belongs on the GPU. Measured 2.8 ms, and it agrees with
    the reference to float32 rounding (mean 2e-07, correlation 1.00000000); the
    remaining last bits are the sample coordinate, far below the graph's own floor.
    """

    def __init__(self, runtime):
        self.rt = runtime
        self._buffers = {}

    def _buffer(self, name, count):
        key = (name, count)
        if key not in self._buffers:
            self._buffers[key] = self.rt.buffer(count)
        return self._buffers[key]

    def __call__(self, history, u, v):
        history = np.ascontiguousarray(history, dtype=np.float32)
        height, width, channels = history.shape
        pixels = height * width
        source = self._buffer("history", pixels * channels)
        self.rt.write(source, history.reshape(-1))
        coordinates = self._buffer("uv", pixels * 2)
        # built host-side and written whole: the two columns are a strided write, and a
        # buffer in the card's own memory is not addressable to write into column by column
        pairs = np.empty((pixels, 2), np.float32)
        pairs[:, 0] = np.asarray(u, dtype=np.float32).reshape(-1)
        pairs[:, 1] = np.asarray(v, dtype=np.float32).reshape(-1)
        self.rt.write(coordinates, pairs)
        target = self._buffer("out", pixels * channels)
        self.rt.begin()
        self.rt.sample_history(source, coordinates, target, height, width, channels,
                               absolute=True)
        self.rt.submit()
        return np.array(self.rt.read(target, shape=(height, width, channels)), copy=True)


def install_gpu_history(runtime):
    """Replace the reference's five-tap reprojection with the device one."""
    temporal_mod.sample_history = GpuHistory(runtime)
    return temporal_mod.sample_history


def blend_alpha(head, blend_scale=BLEND_SCALE):
    """The history weight the model asked for, per pixel."""
    logit = features_mod.half(np.asarray(head, dtype=np.float32)[..., 3:4])
    return np.clip(1.0 / (1.0 + np.exp(-logit)) * features_mod.half(blend_scale), 0, 1)


class VendorSession(temporal_mod.TemporalSession):
    """MLX-DLSS's session with the vendor's two steps after the network
    (`notes/phase70-post-process.md`): the history carried to the next frame is the
    prediction after the history's blend, truncated to half — MLX-DLSS carried the
    composition after the intensity — and the style's colour grade reads that half before
    the intensity blends it back towards the frame. With intensity 1, no mask and no grade
    the output is MLX-DLSS's to the bit; only what is carried changes."""

    def _process_prepared(self, prepared, control_mask=None):
        frame, processing, confidence = prepared.source, prepared.color, prepared.confidence
        if prepared.reset:
            self.reset()
            self.scene_cuts += 1
        height, width = processing.shape[:2]
        geometry = NetworkGeometry.vendor_aligned(width, height)
        controls = self._controls()
        if self.history is None:
            network = features_mod.make_features(processing, frame_index=self.frame_index,
                                                 geometry=geometry, control_mask=control_mask,
                                                 **controls)
            head = geometry.crop(self.pipeline.run_features(network))
            temporal = np.clip(processing + features_mod.half(head[..., :3]) * np.float32(0.25),
                               0, 1).astype(np.float32)
        else:
            features = temporal_mod.make_temporal_features(
                processing, self.history, prepared.motion, frame_index=self.frame_index,
                history_confidence=confidence, control_mask=control_mask, **controls)
            network = temporal_mod.extend_features(features, geometry, self.frame_index)
            head = geometry.crop(self.pipeline.run_features(network))
            # at intensity 1 without a mask MLX-DLSS hands back the prediction itself
            temporal = temporal_mod.compose_temporal(
                head, processing, features, blend_scale=self.options.blend_scale,
                history_confidence=confidence)
        grade = nr_frame.grade_for(**controls)
        kept = nr_frame.truncate_half(temporal)
        intensity = self.options.intensity
        if control_mask is None and intensity == 1 and grade is None:
            output = temporal if self.history is not None else composition_mod.compose_head(
                head, processing, intensity=1.0)
        else:
            predicted = temporal if grade is None else nr_frame.style_grade(kept, grade)
            blend = (np.float32(intensity) if control_mask is None
                     else np.asarray(control_mask, np.float32)[..., :1] * np.float32(intensity))
            blend = np.clip(blend, 0, 1)
            output = np.clip(processing + blend * (predicted - processing), 0, 1).astype(np.float32)
        self.history = kept
        self.previous = frame.copy()
        self.frame_index += 1
        return composition_mod.compose_detail(
            frame, composition_mod.resample(output, frame.shape[1], frame.shape[0]),
            detail_strength=self.options.detail_strength,
            colour_strength=self.options.colour_strength, radius=self.options.detail_radius)


def session(model, *, motion="zero", **options):
    """A `VendorSession` over our model.

    `motion` is 'zero', 'flow' (needs OpenCV) or a callable `(current, previous)`.
    Engine motion is better still: pass it per frame to `process`.
    """
    return VendorSession(Pipeline(model), options=TemporalOptions(**options), motion=motion)


# --------------------------------------------------------------------------
# a synthetic pan, so the path can be tested without a game
# --------------------------------------------------------------------------


def pan_sequence(image, *, shift, size, frames, origin=None):
    """Crop a window that walks across `image`, with the exact motion for each step.

    Returns `(frames, motions)`. Moving the window right by `dx` makes the content
    inside it move left by `dx`, and the history sample for the current pixel sits
    at `+dx/width` — the convention `sample_history` wants. Frame 0's motion is
    unused; the session has no history yet.
    """
    height, width = size
    dx, dy = shift
    if origin is None:
        origin = ((image.shape[0] - height - max(0, dy * (frames - 1))) // 2,
                  (image.shape[1] - width - max(0, dx * (frames - 1))) // 2)
    y0, x0 = origin
    if (y0 < 0 or x0 < 0 or y0 + dy * (frames - 1) + height > image.shape[0]
            or x0 + dx * (frames - 1) + width > image.shape[1]):
        raise ValueError("the pan walks outside the source image")
    sequence = [np.ascontiguousarray(image[y0 + dy * k:y0 + dy * k + height,
                                           x0 + dx * k:x0 + dx * k + width])
                for k in range(frames)]
    motion = np.zeros((height, width, 2), dtype=np.float32)
    motion[..., 0] = np.float32(dx) / np.float32(width)
    motion[..., 1] = np.float32(dy) / np.float32(height)
    return sequence, [motion.copy() for _ in range(frames)]


def check_motion_convention(sequence, motions):
    """Reproject frame k-1 with frame k's motion; it should reconstruct frame k.

    A sign error here is silent — the network still runs and still produces a
    picture — so it is worth one cheap assertion before any of it means anything.
    """
    height, width = sequence[0].shape[:2]
    yy, xx = np.indices((height, width))
    worst = 0.0
    for index in range(1, len(sequence)):
        u = (xx.astype(np.float32) + 0.5) / np.float32(width) + motions[index][..., 0]
        v = (yy.astype(np.float32) + 0.5) / np.float32(height) + motions[index][..., 1]
        reprojected = sample_history(sequence[index - 1], u, v)
        inside = 8            # ignore the border, where content is genuinely new
        error = np.abs(reprojected - sequence[index])[inside:-inside, inside:-inside]
        worst = max(worst, float(error.mean()))
    return worst


# --------------------------------------------------------------------------


def run_sequence(model, sequence, motions, *, verbose=True, **options):
    """-> (outputs, diagnostics). One `TemporalSession` pass over the frames."""
    live = session(model, motion="zero", **options)
    outputs, diagnostics = [], []
    for index, frame in enumerate(sequence):
        started = time.perf_counter()
        supplied = None if index == 0 else motions[index]
        output = live.process(frame, motion=supplied)
        geometry = nr_frame.network_geometry(frame.shape[1], frame.shape[0])
        alpha = geometry.crop(blend_alpha(live.pipeline.head))
        record = {
            "frame": index,
            "seconds": time.perf_counter() - started,
            "history": index > 0,
            "alpha_mean": float(alpha.mean()),
            "alpha_max": float(alpha.max()),
            "change": float(np.abs(output - frame).mean()),
        }
        diagnostics.append(record)
        outputs.append(output)
        if verbose:
            print(f"  frame {index}  {record['seconds']:5.1f}s  "
                  f"history {'yes' if record['history'] else 'no ':>3}  "
                  f"alpha mean {record['alpha_mean']:.4f} max {record['alpha_max']:.4f}  "
                  f"change {record['change']:.5f}", flush=True)
    return outputs, diagnostics


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input", help="one image for --pan, ignored with --frames-from")
    parser.add_argument("output_prefix")
    parser.add_argument("--weights", default=str(WEIGHTS))
    parser.add_argument("--pan", default="6,0", help="window step per frame in pixels, DX,DY")
    parser.add_argument("--frames", type=int, default=4)
    parser.add_argument("--size", default="384x384", help="crop size WxH for --pan")
    parser.add_argument("--frames-from", help="comma-separated image paths; motion is zero")
    parser.add_argument("--zero-motion", action="store_true",
                        help="feed zero motion with a moving image, as a control")
    parser.add_argument("--profile", default="standard")
    parser.add_argument("--intensity", type=float, default=1.0)
    # The vendor's panel exposes these three as guide overrides; the recovered
    # pipeline has always taken them, they were simply never wired to the CLI.
    parser.add_argument("--motion-scale", default="1,1",
                        help="multiplier on the supplied motion, X,Y — the panel's "
                             "Motion Scale X/Y Multiplier")
    parser.add_argument("--scene-cut", type=float, default=0.3,
                        help="mean luma change that clears the history; 0 disables it")
    parser.add_argument("--blend-scale", type=float, default=None,
                        help="ceiling on the learned history blend (default 0.73974609375)")
    parser.add_argument("--gpu", action="store_true")
    parser.add_argument("--resident", action="store_true",
                        help="run the whole graph on the GPU (the fastest path)")
    parser.add_argument("--accel", action="store_true",
                        help="torch's SIMD half and E4M3 conversions (bit-identical)")
    args = parser.parse_args()

    if args.accel:
        import nr_accel
        print(f"accel {'on' if nr_accel.install() else 'requested, but torch is absent'}",
              flush=True)
    if args.gpu:
        sys.path.insert(0, str(ROOT / "src" / "gpu"))
        import nr_xmx
        print(f"backend {nr_xmx.install()}", flush=True)

    if args.frames_from:
        sequence = [image_io.load(path) for path in args.frames_from.split(",")]
        motions = [np.zeros((*sequence[0].shape[:2], 2), np.float32)] * len(sequence)
    else:
        width, height = (int(part) for part in args.size.lower().split("x"))
        shift = tuple(int(part) for part in args.pan.split(","))
        sequence, motions = pan_sequence(image_io.load(args.input), shift=shift,
                                         size=(height, width), frames=args.frames)
        print(f"pan {shift[0]},{shift[1]} px/frame over {args.frames} frames of {width}x{height}")
        print(f"motion convention check: reprojection error {check_motion_convention(sequence, motions):.5f} "
              "(should be small; a sign error shows up here)")
        if args.zero_motion:
            motions = [np.zeros_like(motion) for motion in motions]
            print("CONTROL: motion forced to zero")

    if args.resident:
        model = nr_frame.ResidentBackend(args.weights)
        install_gpu_history(model.runtime)
    else:
        model = nr_model.NeuralRenderingModel.from_safetensors(args.weights)
    scale = tuple(float(part) for part in args.motion_scale.split(","))
    if scale != (1.0, 1.0):
        motions = [motion * np.float32(scale) for motion in motions]
        print(f"motion scaled by {scale[0]},{scale[1]}")
    options = {"profile": args.profile, "intensity": args.intensity,
               "scene_cut_threshold": args.scene_cut}
    if args.blend_scale is not None:
        options["blend_scale"] = args.blend_scale
    outputs, _ = run_sequence(model, sequence, motions, **options)
    for index, output in enumerate(outputs):
        path = f"{args.output_prefix}_{index:02d}.png"
        image_io.save(output, path)
        image_io.save(sequence[index], f"{args.output_prefix}_{index:02d}_in.png")
    print(f"wrote {len(outputs)} frames to {args.output_prefix}_NN.png")


if __name__ == "__main__":
    main()
