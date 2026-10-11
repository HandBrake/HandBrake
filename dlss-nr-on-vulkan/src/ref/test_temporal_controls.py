#!/usr/bin/env python3
"""The temporal controls the vendor's panel exposes, and the two HANDOFF called missing.

Scene-cut detection and per-pixel history confidence were both listed as still to be
built. They were already in the recovered pipeline; this exercises them, and the motion
scale that the panel calls Motion Scale X/Y Multiplier.
"""
import pathlib, sys
import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))
import image_io, nr_frame, nr_model, nr_temporal  # noqa: E402

FAILURES = []


def check(name, ok, detail=""):
    print(f"  [{'ok  ' if ok else 'FAIL'}] {name}{'  ' + detail if detail else ''}", flush=True)
    if not ok:
        FAILURES.append(name)


def scene(seed, level):
    """A stand-in frame: structure at four scales, so panning it moves detail rather than
    flat colour, and `level` sets how bright the scene is so two of them make a cut.

    The pictures this was written against are game screenshots in `pngs/`, which is not in
    the repository and must not be — they are somebody else's. On the machine that has them
    they are still used; everywhere else the test runs on these.
    """
    height, width = 320, 512
    rng = np.random.default_rng(seed)
    image = np.zeros((height, width, 3), np.float32)
    for cells, weight in ((4, 0.55), (16, 0.25), (64, 0.15), (160, 0.05)):
        coarse = rng.random((cells, cells, 3)).astype(np.float32)
        grown = np.repeat(np.repeat(coarse, -(-height // cells), axis=0),
                          -(-width // cells), axis=1)
        image += weight * grown[:height, :width]
    return np.clip(image * level, 0, 1)


def frame(name, seed, level):
    local = ROOT / "pngs" / name
    return image_io.load(str(local)) if local.exists() else scene(seed, level)


def main():
    if not nr_frame.WEIGHTS.exists():
        print(f"temporal controls: skipped (no logical weights at "
              f"{nr_frame.WEIGHTS.name}) — a skip is not a pass")
        return 0
    source = frame("Cyberpunk-2077_02.jpg", seed=2, level=0.9)
    frames, motions = nr_temporal.pan_sequence(source, shift=(6, 0), size=(192, 192), frames=4)
    model = nr_frame.ResidentBackend()
    nr_temporal.install_gpu_history(model.runtime)

    _, plain = nr_temporal.run_sequence(model, frames, motions, verbose=False)
    settled = plain[-1]["alpha_mean"]
    check("history raises the learned blend", plain[0]["alpha_mean"] < 0.1 < settled,
          f"{plain[0]['alpha_mean']:.4f} with none, {settled:.4f} settled")

    # One transition, not two: the first half pans across one scene and the second
    # half across another, so there is exactly one luma step to find.
    other = frame("Cyberpunk-2077_01.jpg", seed=1, level=0.45)
    second, _ = nr_temporal.pan_sequence(other, shift=(6, 0), size=(192, 192), frames=2)
    cut_frames = frames[:2] + second
    luma_step = float(np.abs(cut_frames[2].mean(axis=2) - cut_frames[1].mean(axis=2)).mean())

    check("the cut frame is a real luma step", luma_step > 0.05, f"mean |dY| {luma_step:.4f}")

    def sequence_with(threshold):
        """Run the cut sequence and return (the session, its per-frame alphas)."""
        live = nr_temporal.session(model, motion="zero", scene_cut_threshold=threshold)
        geometry = nr_frame.NetworkGeometry.vendor_aligned(192, 192)
        alphas = []
        for index, frame in enumerate(cut_frames):
            live.process(frame, motion=None if index == 0 else motions[index])
            alphas.append(float(geometry.crop(
                nr_temporal.blend_alpha(live.pipeline.head)).mean()))
        return live, alphas

    detected, detected_alpha = sequence_with(0.05)
    ignored, ignored_alpha = sequence_with(0.0)
    check("the detector counts exactly the one cut", detected.scene_cuts == 1,
          f"{detected.scene_cuts} cut(s) seen, {ignored.scene_cuts} with it disabled")
    check("a cut restarts the noise index",
          detected.frame_index == len(cut_frames) - 2 and ignored.frame_index == len(cut_frames),
          f"index {detected.frame_index} with the detector, {ignored.frame_index} without")
    # The point of the detector is what it is *not* needed for: the learned gate
    # already refuses history that does not match. That is the phase-12 ghosting
    # rejection, on a real scene change rather than a deliberately wrong motion.
    check("the gate rejects the mismatched history on its own",
          ignored_alpha[2] < 0.05,
          f"alpha {ignored_alpha[2]:.4f} across the cut with detection off")

    # the panel's motion multiplier: wrong motion is worse than none, so a scaled-up
    # motion must move the blend away from the correctly-reprojected value
    _, doubled = nr_temporal.run_sequence(model, frames, [m * np.float32(2) for m in motions],
                                          verbose=False)
    check("scaling the motion breaks the reprojection",
          doubled[-1]["alpha_mean"] < settled,
          f"{doubled[-1]['alpha_mean']:.4f} at 2x against {settled:.4f} at 1x")

    print("\n" + ("temporal controls behave" if not FAILURES
                  else f"{len(FAILURES)} FAILED: {', '.join(FAILURES)}"))
    return 1 if FAILURES else 0


if __name__ == "__main__":
    sys.exit(main())
