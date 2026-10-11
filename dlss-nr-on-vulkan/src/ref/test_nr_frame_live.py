#!/usr/bin/env python3
"""`nr_frame_live` against the daemon's own frame, byte for byte.

The claim is that `nr_frame_live_run` is `nr_daemon.process_connection` on its default
path. So the reference here is the daemon's own pieces — `Letterbox`, `History`, `resample`,
`nr_frame.render_extent` — driving the C library's network half and its fused composition
(`nr_frame_head`, `nr_frame_compose_encode_neural`), and the session has to give the same
answer on every frame: a letterboxed panning shot, a cut, at render scales that take the
area mean, the bilinear and none, with the detail split on and off. What is under test is
the orchestration — the bars, the render extent, when the history is taken and dropped,
which frame it is and at which extent — since both sides run the same kernels.

Runs on the real weights when they exist, else on random weights of the real shapes, on
whichever runtime is behind the library (NR_GPU_BACKEND).
"""
import pathlib
import sys
import tempfile

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))
sys.path.insert(0, str(ROOT / "src" / "layer"))
import nr_frame  # noqa: E402
import nr_frame_native  # noqa: E402
import test_nr_frame_c  # noqa: E402

FAILED = []


def check(name, ok, detail=""):
    print(f"  {'ok  ' if ok else 'FAIL'} {name}" + (f"  ({detail})" if detail else ""))
    if not ok:
        FAILED.append(name)


def shot(rng, height, width, bars, frames, pan, cut_at):
    """A panning letterboxed shot, BGRA, with a cut to another picture at `cut_at`."""
    # live_rates.py's recipe, structure at 16 pixels under fine noise, so a pan of a few
    # pixels stays under the cut limit and a different picture does not
    rows, columns = height - 2 * bars, width + pan * frames
    pictures = []
    for _ in range(2):
        grid = rng.uniform(0.05, 1.0, (rows // 16 + 1, columns // 16 + 1, 3))
        coarse = grid.repeat(16, 0).repeat(16, 1)[:rows, :columns]
        pictures.append(np.clip(coarse * 0.85 + rng.uniform(0, 0.15, coarse.shape), 0, 1))
    out = []
    for index in range(frames):
        picture = pictures[index >= cut_at]
        x = pan * index
        rgb = np.zeros((height, width, 3), np.uint8)
        rgb[bars:height - bars] = np.rint(picture[:, x:x + width] * 255).astype(np.uint8)
        bgra = np.empty((height, width, 4), np.uint8)
        bgra[..., 0], bgra[..., 1], bgra[..., 2], bgra[..., 3] = rgb[..., 2], rgb[..., 1], rgb[..., 0], 255
        out.append(bgra)
    return out


class Mirror:
    """process_connection's default path, from the daemon's own classes."""

    def __init__(self, native, nr_daemon):
        self.native, self.daemon = native, nr_daemon
        self.letterbox, self.history = nr_daemon.Letterbox(), nr_daemon.History()

    def run(self, payload, render_scale, temporal=1.0, hold=1.0, release=24.0, cut_limit=0.15,
            **values):
        d = self.daemon
        height, width = payload.shape[:2]
        whole = d.decode(payload.tobytes(), width, height, 44)
        top, bottom, left, right = self.letterbox.region(whole, (width, height, 44))
        colour = np.ascontiguousarray(whole[top:bottom, left:right])
        active_height, active_width = colour.shape[:2]
        inner = colour
        if render_scale < 1.0:
            extent = nr_frame.render_extent(active_width, active_height, float(render_scale), 320)
            if extent != (active_width, active_height):
                inner = np.ascontiguousarray(d.resample(colour, (extent[1], extent[0])), np.float32)
        key = (width, height, 44, top, bottom, left, right, "standard")
        history_inner, history_full, history_pixels = self.history.take(
            key, inner, cut_limit if temporal > 0 else -1.0)
        head = self.native.head(inner, history=history_inner, **values)
        previous = history_pixels if history_pixels is not None and (hold > 0 or release > 0) else None
        answer = payload.copy()
        neural = np.empty((active_height, active_width, 3), np.float32) if temporal > 0 else None
        self.native.compose_encode(
            head, colour, answer, top=top, left=left, bgra=True, history=history_full,
            previous=previous, neural=neural, history_confidence=temporal,
            hold=hold if previous is not None else 0.0,
            slope=float(np.float32(-255.0 * hold / float(nr_frame.HOLD_RAMP))) if previous is not None else 0.0,
            release=nr_frame.release_slope(release) if previous is not None else 0.0, **values)
        if temporal > 0:
            self.history.keep(key, neural, inner, colour)
        return answer, (top, bottom, left, right), history_full is not None


def main():
    if not nr_frame_native.LIBRARY.exists():
        print(f"nr_frame live: skipped (build {nr_frame_native.LIBRARY.name} first) — a skip is not a pass")
        return 0
    import nr_daemon  # noqa: E402  (after the library check: it opens nothing on import)
    with tempfile.TemporaryDirectory(prefix="nr-frame-live-") as room:
        weights = nr_frame.WEIGHTS
        if not weights.exists():
            weights = pathlib.Path(room) / "random-logical.safetensors"
            test_nr_frame_c.synthetic_weights(weights)
            print(f"no logical weights; random weights of the real shapes at {weights}")
        native = nr_frame_native.NativeFrame(weights)
        print(f"nr_frame_live on {native.device}: {native.gemm_path}")
        rng = np.random.default_rng(11)
        frames = shot(rng, 240, 320, 32, 7, 2, 4)
        cases = [("scale 1", dict(render_scale=1.0)),
                 ("scale 0.5, area mean", dict(render_scale=0.5)),
                 ("scale 0.6, bilinear", dict(render_scale=0.6)),
                 ("detail split, no release", dict(render_scale=1.0, detail_strength=0.5, release=0.0)),
                 ("no history", dict(render_scale=0.6, temporal=0.0))]
        for name, values in cases:
            mirror, live = Mirror(native, nr_daemon), nr_frame_native.NativeLive(native)
            same, history_at, used_count = [], [], 0
            for payload in frames:
                want, bounds, used = mirror.run(payload, **values)
                got, report = live.run(payload, **values)
                same.append(np.array_equal(want, got))
                used_count += report["with_history"]
                history_at.append((report["with_history"] == int(used),
                                   (report["top"], report["bottom"], report["left"], report["right"]) == bounds))
            check(f"{name}: every answer the daemon's", all(same),
                  f"{sum(same)} of {len(same)} frames identical")
            check(f"{name}: the history taken and the bars found where the daemon's are",
                  all(a and b for a, b in history_at), f"history on {used_count} frames")
            live.close()
        # the history is used after the first frame and dropped at the cut
        live = nr_frame_native.NativeLive(native)
        used = [live.run(payload)[1]["with_history"] for payload in frames]
        check("history on a pan, dropped at the cut", used == [0, 1, 1, 1, 0, 1, 1], f"{used}")
        # the answer may be the request itself, and the bars come back as they went
        live.reset()
        answer, report = live.run(frames[0])
        check("the bars untouched", np.array_equal(answer[:32], frames[0][:32])
              and np.array_equal(answer[-32:], frames[0][-32:]),
              f"active {report['top']}-{report['bottom']}")
        live.close()
        native.close()
    print("nr_frame live: " + ("all checks passed" if not FAILED else f"{len(FAILED)} FAILED"))
    return 1 if FAILED else 0


if __name__ == "__main__":
    sys.exit(main())
