#!/usr/bin/env python3
"""Frame time through the C frame library, the way an embedder pays it.

`live_rates.py` times the daemon over its socket; this times `libnr_frame` directly —
what VBA-M's filter and any other C host calls: `nr_frame_update` on a float32 frame,
with the previous output as history and the previous input behind the hold floor, so
the temporal path is timed too. The render scale is the daemon's: the frame is taken
down to `max(64, round(extent * scale))` first (natively, by the area mean or the
bilinear resize the daemon uses), and that resample is its own column, because the C
library composes at the extent it is given and a host that scales pays it separately.

Each case runs in its own process — the library reads its switches (`NR_INPUT_FP16`,
`NR_COMPACT_HEAD`, the fusions, `NR_GPU_BACKEND`) when it opens — and the first frames
are discarded, while the extent's buffers are built and the clock ramps.

    python3 src/bench/frame_rates.py                          # the live_rates plan
    python3 src/bench/frame_rates.py 1280x720@0.5 --frames 15 --processes 3
    python3 src/bench/frame_rates.py 640x360@0.5 --ab NR_INPUT_FP16 --ab NR_COMPACT_HEAD
    NR_GPU_BACKEND=metal python3 src/bench/frame_rates.py --json rates.json

Columns: `ms` is the median wall time of a whole frame (resample + update), `write`,
`graph` and `read` are the library's own split (`nr_frame_split`), and `host` is the rest
of `nr_frame_update` — feature assembly and the composition. `head` is a hash of the last
frame's head: an A/B whose two sides differ there is not the same computation.
"""
import argparse
import hashlib
import json
import os
import pathlib
import subprocess
import sys
import time

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))

# live_rates.PLAN, so the two tables can be read side by side
PLAN = ((512, 288, 0.35), (512, 288, 0.50), (640, 360, 0.35), (640, 360, 0.50),
        (854, 480, 0.50), (1024, 768, 0.55), (1920, 1080, 0.55))
HOLD = 1.0          # nr_knobs' default hold
HOLD_RAMP = 4.0     # nr_frame.HOLD_RAMP


def frame(width, height, index, base_seed=7):
    """A frame with structure at several scales and a square that moves 8 px a frame, so
    most of the picture is held by the floor and part of it is not — a live frame's mix."""
    rng = np.random.default_rng(base_seed)
    small = rng.random((max(2, -(-height // 16)), max(2, -(-width // 16)), 3), np.float32)
    coarse = np.repeat(np.repeat(small, 16, axis=0), 16, axis=1)[:height, :width]
    image = coarse * np.float32(0.7) + rng.random((height, width, 3), np.float32) * np.float32(0.3)
    side = max(8, min(height, width) // 4)
    x = (index * 8) % max(1, width - side)
    y = (height - side) // 2
    image[y:y + side, x:x + side] = np.float32(1.0) - image[y:y + side, x:x + side]
    # 8-bit values, as a decoded swapchain frame holds
    return np.round(np.clip(image, 0, 1) * 255) / np.float32(255)


def inner_size(width, height, scale):
    if scale >= 1.0:
        return width, height
    return max(64, round(width * scale)), max(64, round(height * scale))


def resample(image, size):
    """The daemon's `resample`, natively: the area mean where the factors are whole, else
    the bilinear resize. Falls back to the daemon's own NumPy when the library is absent."""
    height, width = size
    if image.shape[:2] == (height, width):
        return image
    import nr_image
    if nr_image.library() is not None:
        if image.shape[0] % height == 0 and image.shape[1] % width == 0:
            return nr_image.area_mean(image, (image.shape[0] // height, image.shape[1] // width))
        return nr_image.bilinear(image, (height, width))
    sys.path.insert(0, str(ROOT / "src" / "layer"))
    import nr_daemon
    return nr_daemon.resample(image, (height, width))


def child(width, height, scale, frames, warmup, temporal):
    """One case in this process; prints one JSON line."""
    import nr_frame_native
    nf = nr_frame_native.NativeFrame()
    inner_w, inner_h = inner_size(width, height, scale)
    network_h, network_w = nr_frame_native.geometry(inner_h, inner_w)
    sources = [frame(width, height, i) for i in range(frames + warmup)]
    rows = []
    history = previous = head = None
    for i, colour in enumerate(sources):
        started = time.perf_counter()
        inner = resample(colour, (inner_h, inner_w))
        resampled = time.perf_counter()
        extra = {}
        if temporal and history is not None:
            extra = dict(history=history, previous=previous, hold=HOLD,
                         slope=float(np.float32(-255.0 * HOLD / HOLD_RAMP)))
        output, head = nf.update(inner, want_head=True, **extra)
        done = time.perf_counter()
        if temporal:
            history, previous = output, inner
        if i >= warmup:
            write, graph, read = nf.split
            update = done - resampled
            rows.append(dict(total=done - started, resample=resampled - started,
                             write=write, graph=graph, read=read,
                             host=update - write - graph - read))
    digest = hashlib.sha256(np.ascontiguousarray(head).tobytes()).hexdigest()[:16]
    print(json.dumps(dict(width=width, height=height, scale=scale, inner=[inner_w, inner_h],
                          network=[network_w, network_h], device=nf.device,
                          gemm=nf.gemm_path, head=digest, frames=rows)), flush=True)
    nf.close()


def run_case(width, height, scale, args, env):
    command = [sys.executable, str(pathlib.Path(__file__).resolve()), "--child",
               "%dx%d@%g" % (width, height, scale), "--frames", str(args.frames),
               "--warmup", str(args.warmup)]
    if not args.temporal:
        command.append("--still")
    frames, heads, last = [], set(), None
    for _ in range(args.processes):
        done = subprocess.run(command, env=env, capture_output=True, text=True)
        lines = [line for line in done.stdout.splitlines() if line.startswith("{")]
        if done.returncode or not lines:
            raise SystemExit("case %dx%d@%g failed (%d):\n%s%s" % (
                width, height, scale, done.returncode, done.stdout, done.stderr))
        last = json.loads(lines[-1])
        frames += last["frames"]
        heads.add(last["head"])
    if len(heads) > 1:
        # the same frames in, so a different head between processes is a determinism bug
        last["head"] = "differs:" + ",".join(sorted(heads))
    last["frames"] = frames
    return last


def median(values):
    ordered = sorted(values)
    return ordered[len(ordered) // 2]


def summarise(result):
    rows = result["frames"]
    column = {key: 1000 * median([r[key] for r in rows])
              for key in ("total", "resample", "write", "graph", "read", "host")}
    totals = [1000 * r["total"] for r in rows]
    return column, min(totals), max(totals)


def parse_case(case):
    extent, _, scale = case.partition("@")
    width, height = extent.lower().split("x")
    return int(width), int(height), float(scale or 1.0)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("cases", nargs="*", help="WxH@scale; live_rates' plan if none")
    parser.add_argument("--frames", type=int, default=9, help="timed frames per process")
    parser.add_argument("--warmup", type=int, default=3, help="frames to discard first")
    parser.add_argument("--processes", type=int, default=1,
                        help="processes per case; buffer placement varies between them")
    parser.add_argument("--still", dest="temporal", action="store_false",
                        help="no history: every frame stands alone")
    parser.add_argument("--ab", action="append", default=[], metavar="VAR",
                        help="run every case with VAR=0 and VAR=1 (repeatable)")
    parser.add_argument("--env", action="append", default=[], metavar="VAR=VALUE",
                        help="set for every case (repeatable)")
    parser.add_argument("--json", metavar="FILE", help="write every frame's timings here")
    parser.add_argument("--child", metavar="CASE", help=argparse.SUPPRESS)
    args = parser.parse_args()

    if args.child:
        width, height, scale = parse_case(args.child)
        child(width, height, scale, args.frames, args.warmup, args.temporal)
        return

    base = dict(os.environ)
    for pair in args.env:
        name, _, value = pair.partition("=")
        base[name] = value
    variants = [("", base)]
    for name in args.ab:
        variants = [(("%s %s=%s" % (label, name, value)).strip(), dict(env, **{name: value}))
                    for label, env in variants for value in ("0", "1")]

    plan = [parse_case(case) for case in args.cases] or list(PLAN)
    header = "  %-11s %5s %-9s %-24s %7s %6s %11s %6s %6s %6s %6s %6s  %s" % (
        "extent", "scale", "network", "variant", "ms", "fps", "min-max", "resmp", "write",
        "graph", "read", "host", "head")
    printed = False
    results = []
    for width, height, scale in plan:
        for label, env in variants:
            result = run_case(width, height, scale, args, env)
            result["variant"] = label
            results.append(result)
            if not printed:
                print("  %s, %s, %s" % (result["device"], result["gemm"],
                                        "temporal" if args.temporal else "still"))
                print(header)
                printed = True
            column, low, high = summarise(result)
            print("  %-11s %5.2f %-9s %-24s %7.1f %6.1f %11s %6.1f %6.1f %6.1f %6.1f %6.1f  %s" % (
                "%dx%d" % (width, height), scale, "%dx%d" % tuple(result["network"]),
                label or "-", column["total"], 1000 / column["total"],
                "%.0f-%.0f" % (low, high), column["resample"], column["write"],
                column["graph"], column["read"], column["host"], result["head"]), flush=True)
        if len(variants) > 1 and len({r["head"] for r in results[-len(variants):]}) > 1:
            print("  ^ the variants' heads differ: not the same computation")
    if args.json:
        pathlib.Path(args.json).write_text(json.dumps(results, indent=1))


if __name__ == "__main__":
    main()
