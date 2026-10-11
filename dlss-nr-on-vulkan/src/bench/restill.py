#!/usr/bin/env python3
"""Replay a `--dump` capture through today's daemon, frame by frame, in the order it was taken.

`nr_daemon.py --dump DIR` keeps what the game handed it (`NNN_in.png`) and what it sent back
(`NNN_out.png`). Sent to a fresh daemon in the same order, the `NNN_in` frames are the same
input — and, since the history is built only from frames the daemon saw, the same temporal
state — through the graph as it is now. So a published still can be re-rendered on the frame
the owner chose, without starting the game again.

    python3 src/bench/restill.py work/tekken work/restill/tekken
    python3 src/bench/restill.py work/mk1cap work/restill/mk1 --set profile=natural

The answers land in OUT through the daemon's own `--dump`, numbered from 001 in the order sent,
and each `NNN_in` it writes is checked against the frame it was sent. `render_scale` is 1.0 unless set:
the published stills are taken with the model at full resolution. Every swapchain behind the
captures so far was B8G8R8A8_UNORM, and the dumps are 8-bit, so the bytes sent are the bytes
the game sent.

`--native` replays through the C frame library instead — `nr_frame --replay`, which runs the
daemon's frame through `nr_frame_live` in one process: the same letterbox, render extent and
history, no socket, and on whichever runtime `NR_GPU_BACKEND` names (Vulkan, Metal, Direct3D
12), so a capture can be re-rendered where the daemon and its Unix socket do not run. The
answers match the daemon's to the rounding of the noise channels, which NumPy and C compute
to a last bit apart (`notes/phase68`); `src/ref/test_nr_frame_live.py` holds the rest of it
byte for byte.

    python3 src/bench/restill.py work/tekken work/restill/tekken --native
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
sys.path.insert(0, str(ROOT / "src" / "ref"))
sys.path.insert(0, str(ROOT / "src"))
import image_io
import nr_build

MAGIC = 0x304E524E
FORMAT_B8G8R8A8 = 44


def frames_of(directory):
    # numbered as the daemon numbers them; anything else in the folder is somebody's own
    frames = sorted((path for path in directory.glob("*_in.png") if path.stem.split("_")[0].isdigit()),
                    key=lambda path: int(path.stem.split("_")[0]))
    numbers = [int(path.stem.split("_")[0]) for path in frames]
    if not frames:
        raise SystemExit(f"no NNN_in.png in {directory}")
    if numbers != list(range(numbers[0], numbers[0] + len(numbers))):
        # a gap means frames the daemon saw and this replay would not: its history differs
        raise SystemExit(f"{directory}: the frames are not consecutive ({numbers[0]}-{numbers[-1]}, "
                         f"{len(numbers)} of them)")
    return frames


def bgra(path):
    rgb = np.rint(image_io.load(path) * 255.0).astype(np.uint8)
    height, width = rgb.shape[:2]
    out = np.empty((height, width, 4), np.uint8)
    out[..., 0], out[..., 1], out[..., 2] = rgb[..., 2], rgb[..., 1], rgb[..., 0]
    out[..., 3] = 255
    return width, height, rgb, out.tobytes()


def exchange(path, width, height, payload):
    want = 4 * width * height
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(600)
        client.connect(path)
        client.sendall(struct.pack("<4I", MAGIC, width, height, FORMAT_B8G8R8A8) + payload)
        got = 0
        while got < want:
            piece = client.recv(want - got)
            if not piece:
                raise RuntimeError(f"the daemon answered {got} of {want} bytes; its log says why")
            got += len(piece)


# the daemon's settings file names, as `nr_frame --replay` spells them
NATIVE_FLAGS = {"render_scale": "--render-scale", "profile": "--profile", "intensity": "--intensity",
                "detail_strength": "--detail-strength", "colour_strength": "--colour-strength",
                "temporal": "--temporal", "hold": "--hold", "release": "--release",
                "cut_limit": "--cut-limit", "min_extent": "--min-extent"}


def native(frames, source, out, settings):
    """The replay through `nr_frame --replay`, then the same check the daemon's gets."""
    command = [str(nr_build.executable("nr_frame")), "--replay", str(source), str(out)]
    for knob, value in settings.items():
        if knob not in NATIVE_FLAGS:
            raise SystemExit(f"--native has no {knob}; it takes " + ", ".join(sorted(NATIVE_FLAGS)))
        command += [NATIVE_FLAGS[knob], str(int(value) if knob == "min_extent" else value)]
    if subprocess.run(command).returncode:
        raise SystemExit("nr_frame --replay failed")
    for index, path in enumerate(frames, 1):
        written = out / f"{index:03d}_in.png"
        if not np.array_equal(np.rint(image_io.load(written) * 255.0), np.rint(image_io.load(path) * 255.0)):
            raise SystemExit(f"{written} is not the frame sent from {path}")
    print(f"  every NNN_in in {out} is the frame it was sent", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", type=pathlib.Path, help="a --dump directory")
    parser.add_argument("out", type=pathlib.Path, help="an empty or new directory")
    parser.add_argument("--set", action="append", default=[], metavar="KNOB=VALUE",
                        help="a daemon setting, e.g. render_scale=0.9 or profile=natural")
    parser.add_argument("--native", action="store_true",
                        help="through the C frame library (nr_frame --replay), not the daemon")
    args = parser.parse_args()
    settings = {"render_scale": 1.0}
    for item in args.set:
        knob, _, value = item.partition("=")
        try:
            settings[knob] = float(value)
        except ValueError:
            settings[knob] = value
    frames = frames_of(args.source)
    if args.out.exists() and any(args.out.iterdir()):
        raise SystemExit(f"{args.out} is not empty: the daemon numbers its dump after what is there")
    args.out.mkdir(parents=True, exist_ok=True)
    if args.native:
        native(frames, args.source, args.out, settings)
        return

    with tempfile.TemporaryDirectory() as directory:
        room = pathlib.Path(directory)
        daemon_socket, settings_file, log = room / "restill.sock", room / "settings.json", room / "log"
        settings_file.write_text(json.dumps(settings))
        with open(log, "w") as sink:
            daemon = subprocess.Popen(
                [sys.executable, str(ROOT / "src" / "layer" / "nr_daemon.py"),
                 "--socket", str(daemon_socket), "--settings", str(settings_file),
                 "--max-pixels", str(1920 * 1200), "--dump", str(args.out)],
                stdout=sink, stderr=subprocess.STDOUT)
        try:
            for _ in range(600):
                if daemon_socket.exists():
                    break
                if daemon.poll() is not None:
                    raise SystemExit("daemon exited:\n" + log.read_text())
                time.sleep(0.5)
            print(f"  {args.source} -> {args.out}, {len(frames)} frames, settings {settings}",
                  flush=True)
            for index, path in enumerate(frames, 1):
                width, height, rgb, payload = bgra(path)
                exchange(str(daemon_socket), width, height, payload)
                written = args.out / f"{index:03d}_in.png"
                # The dump is written after the answer is sent, and the frame's log line after
                # the dump: wait for the line, and both pictures are on disk.
                said = []
                for _ in range(600):
                    said = [line.strip() for line in log.read_text().splitlines() if "B8G8R8A8" in line]
                    if len(said) >= index or daemon.poll() is not None:
                        break
                    time.sleep(0.1)
                if len(said) < index:
                    raise SystemExit("the daemon did not log frame %d:\n%s" % (index, log.read_text()))
                same = np.array_equal(np.rint(image_io.load(written) * 255.0).astype(np.uint8), rgb)
                print(f"  {path.name} -> {written.name}  {'same frame' if same else 'DIFFERENT FRAME'}"
                      f"  {said[index - 1]}", flush=True)
                if not same:
                    raise SystemExit(f"{written} is not the frame sent from {path}")
        finally:
            daemon.terminate()
            daemon.wait(timeout=60)


if __name__ == "__main__":
    main()
