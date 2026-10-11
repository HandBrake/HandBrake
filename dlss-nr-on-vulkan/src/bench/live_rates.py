#!/usr/bin/env python3
"""End-to-end frame time through the socket, the way a game pays it.

Not graph time. This is what the Vulkan layer waits for: the round trip, the 8-bit codecs,
feature assembly, the network at the render scale, and the composition — which runs at the
*swapchain's* resolution and does not shrink with the scale. That is why the table is
indexed by both, and why halving the scale does not halve the frame.

One connection per frame, because that is what `nr_layer.c` does. The first frames are
discarded: the daemon builds its resident buffers for an extent the first time it sees it,
and the temporal path has no history yet.

    python3 src/bench/live_rates.py                       # the published table
    python3 src/bench/live_rates.py 800x600@0.5 --frames 9
    python3 src/bench/live_rates.py 640x360@0.5 --set min_extent=128

Prints the table and the literal for `nr_knobs.RATES`, so the number in the README has a
program behind it rather than a memory of one.
"""
import argparse
import json
import pathlib
import socket
import struct
import subprocess
import sys
import tempfile
import time

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
MAGIC = 0x304E524E
FORMAT_B8G8R8A8 = 44
PLAN = ((512, 288, 0.35), (512, 288, 0.50), (640, 360, 0.35), (640, 360, 0.50),
        (854, 480, 0.50), (1024, 768, 0.55), (1920, 1080, 0.55))


def frame(width, height, seed):
    """A noisy frame with structure at several scales, so the letterbox finder sees no
    bars and the detail passes have something to work on."""
    rng = np.random.default_rng(seed)
    small = rng.random((max(2, -(-height // 16)), max(2, -(-width // 16)), 3))
    coarse = np.repeat(np.repeat(small, 16, axis=0), 16, axis=1)[:height, :width]
    image = np.clip(coarse * 0.7 + rng.random((height, width, 3)) * 0.3, 0, 1)
    out = np.empty((height, width, 4), np.uint8)
    out[..., :3] = (image * 255).astype(np.uint8)
    out[..., 3] = 255
    return out.tobytes()


def round_trip(path, payload, header, want):
    started = time.perf_counter()
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(600)
        client.connect(path)
        client.sendall(header + payload)
        got = 0
        while got < want:
            piece = client.recv(want - got)
            if not piece:
                raise RuntimeError("the daemon answered %d of %d bytes: it rejected the "
                                   "frame, and its log says why" % (got, want))
            got += len(piece)
    return time.perf_counter() - started


def measure(daemon_socket, settings, width, height, scale, frames, warmup, extra=None):
    settings.write_text(json.dumps(dict(extra or {}, render_scale=scale)))
    payload_bytes = 4 * width * height
    header = struct.pack("<4I", MAGIC, width, height, FORMAT_B8G8R8A8)
    times = []
    for i in range(frames + warmup):
        took = round_trip(str(daemon_socket), frame(width, height, i), header, payload_bytes)
        if i >= warmup:
            times.append(took)
    return sorted(times)[len(times) // 2], min(times), max(times)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("cases", nargs="*", help="WxH@scale; the published table if none")
    # Nine and three, not five and two: on 2026-09-23 five frames after two put the 1080p
    # case at 467 ms (451-518) and nine after three at 354 (327-358), run to run.
    parser.add_argument("--frames", type=int, default=9, help="timed frames per case")
    parser.add_argument("--warmup", type=int, default=3,
                        help="frames to discard while the extent's buffers are built")
    parser.add_argument("--set", action="append", default=[], metavar="KNOB=VALUE",
                        help="any other setting for the daemon, e.g. min_extent=128")
    args = parser.parse_args()
    extra = {}
    for item in args.set:
        knob, _, value = item.partition("=")
        extra[knob] = float(value) if value.replace(".", "", 1).isdigit() else value

    plan = []
    for case in args.cases:
        extent, _, scale = case.partition("@")
        width, height = extent.lower().split("x")
        plan.append((int(width), int(height), float(scale or 0.55)))
    plan = plan or list(PLAN)

    with tempfile.TemporaryDirectory() as directory:
        room = pathlib.Path(directory)
        daemon_socket, settings = room / "rates.sock", room / "settings.json"
        settings.write_text(json.dumps({"render_scale": plan[0][2]}))
        daemon = subprocess.Popen(
            [sys.executable, str(ROOT / "src" / "layer" / "nr_daemon.py"),
             "--socket", str(daemon_socket), "--settings", str(settings),
             "--max-pixels", str(1920 * 1200)],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        try:
            for _ in range(600):
                if daemon_socket.exists():
                    break
                if daemon.poll() is not None:
                    raise SystemExit("daemon exited: " + (daemon.stdout.read() or ""))
                time.sleep(0.5)
            print("  %-14s %6s %8s %8s %18s" % ("swapchain", "scale", "ms", "fps", "min-max ms"))
            measured = []
            for width, height, scale in plan:
                middle, low, high = measure(daemon_socket, settings, width, height, scale,
                                            args.frames, args.warmup, extra)
                measured.append((width, height, scale, 1000 * middle))
                print("  %-14s %6.2f %8.0f %8.1f %18s"
                      % ("%dx%d" % (width, height), scale, 1000 * middle, 1 / middle,
                         "%.0f-%.0f" % (1000 * low, 1000 * high)))
            print("\n  nr_knobs.RATES = (")
            for width, height, scale, ms in measured:
                print("      (%d, %d, %.2f, %.1f)," % (width, height, scale, ms))
            print("  )")
        finally:
            daemon.terminate()
            daemon.wait(timeout=60)


if __name__ == "__main__":
    main()
