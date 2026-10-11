#!/usr/bin/env python3
"""Neural Upstream, measured: run the network small and enlarge, against running it big.

Community mods for the real feature (matiasLombo's Neural Upstream, through
DLSS5-Autopilot) move the neural pass *before* the game's upscaler, so it runs at the
render resolution rather than the output one. The cost of this network follows the
extent it is given — about 15 ms + 450 ms per megapixel (`notes/phase60`) — so a game
rendering at half and upscaling should cost a quarter.

We have no upscaler to hide behind, so this measures the honest local version: shrink,
enhance, enlarge. What it costs in fidelity is the point.
"""
import argparse, pathlib, sys, time
import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))
import image_io, nr_frame  # noqa: E402

resample = nr_frame.composition_mod.resample


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input")
    parser.add_argument("--factors", default="1,2,3,4")
    args = parser.parse_args()

    source = image_io.load(args.input)
    height, width = source.shape[:2]
    model = nr_frame.ResidentBackend()

    def enhance(image):
        started = time.perf_counter()
        head, _ = nr_frame.run_head(model, image)
        out = nr_frame.compose(head, image)
        return out, time.perf_counter() - started

    full, _ = enhance(source)
    _, seconds = enhance(source)
    effect = np.abs(full - source).mean()
    print(f"  {width}x{height}, the effect at full resolution is {effect:.5f}\n")
    print("  %-16s %10s %9s %11s %12s %s"
          % ("run at", "ms", "fps", "effect", "of full", "detail kept"))
    print("  %-16s %10.0f %9.2f %11.5f %11.0f%% %12s"
          % (f"{width}x{height}", 1000 * seconds, 1 / seconds, effect, 100, "-"))

    for factor in (int(v) for v in args.factors.split(",")):
        if factor == 1:
            continue
        w, h = width // factor, height // factor
        if w < 320 or h < 320:
            print("  %-16s below the network's 320 minimum" % f"{w}x{h}")
            continue
        small = resample(source, w, h)
        _, _ = enhance(small)
        out, seconds = enhance(small)
        enlarged = resample(out, width, height)
        change = np.abs(enlarged - source).mean()
        # how much of the full-resolution enhancement survives the round trip
        kept = float(np.sum((enlarged - source) * (full - source))
                     / max(float(np.sum((full - source) ** 2)), 1e-12))
        print("  %-16s %10.0f %9.2f %11.5f %11.0f%% %11.0f%%"
              % (f"{w}x{h}", 1000 * seconds, 1 / seconds, change,
                 100 * change / effect, 100 * kept))
    print("\n  'detail kept' projects the small-run change onto the full-run one:"
          "\n  100% would mean the same enhancement, 0% would mean unrelated.")


if __name__ == "__main__":
    main()
