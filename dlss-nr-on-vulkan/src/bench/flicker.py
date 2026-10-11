#!/usr/bin/env python3
"""How much the network moves pixels the game did not move.

Replays a captured sequence of live frames through the daemon and sorts every pixel by
what the *input* did between two presents. A pixel whose bytes did not change at all is
the interesting one: whatever comes back different is invention, and it is what the eye
reads as flicker.

The whole point of a replay rather than a live capture is that both settings see exactly
the same frames, so the numbers can be subtracted.

    python3 src/bench/flicker.py work/good --temporal 0 1
"""
from __future__ import annotations

import argparse, pathlib, socket, struct, subprocess, sys, time
import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))
sys.path.insert(0, str(ROOT / "src" / "layer"))
import image_io  # noqa: E402
import nr_daemon  # noqa: E402

MAGIC = 0x304E524E
FORMAT_B8G8R8A8 = 44


def to_bytes(image):
    frame = np.zeros(image.shape[:2] + (4,), np.uint8)
    frame[..., 2::-1] = np.clip(np.rint(image * 255), 0, 255).astype(np.uint8)
    frame[..., 3] = 255
    return frame


def replay(frames, socket_path, timeout):
    """Each frame through the socket in order, so the daemon sees a sequence."""
    answers = []
    for frame in frames:
        payload = to_bytes(frame)
        height, width = payload.shape[:2]
        header = struct.pack("<4I", MAGIC, width, height, FORMAT_B8G8R8A8)
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
            client.settimeout(timeout)
            client.connect(socket_path)
            client.sendall(header + payload.tobytes())
            want, chunks = width * height * 4, b""
            while len(chunks) < want:
                piece = client.recv(want - len(chunks))
                if not piece:
                    raise RuntimeError("the daemon closed the connection")
                chunks += piece
        answers.append(np.frombuffer(chunks, np.uint8).reshape(height, width, 4)[..., 2::-1])
    return answers


def statistics(inputs, outputs, region):
    """Per-pixel: what the input did between two presents, and what came back.

    `lag` is the third number that matters: how far our answer sits from the game's own
    frame. Holding a pixel still is only worth anything if it does not start trailing
    behind the picture, and that is where a trail would show.
    """
    top, bottom, left, right = region
    rows = []
    for previous, current, before, after in zip(inputs, inputs[1:], outputs, outputs[1:]):
        a = previous[top:bottom, left:right].astype(np.int16)
        b = current[top:bottom, left:right].astype(np.int16)
        c = before[top:bottom, left:right].astype(np.int16)
        d = after[top:bottom, left:right].astype(np.int16)
        rows.append((np.abs(b - a), np.abs(d - c), np.abs(d - b)))
    change_in, change_out, lag = (np.concatenate([r[which].reshape(-1, 3) for r in rows])
                                  for which in (0, 1, 2))
    peak = change_in.max(1)
    table = []
    for name, select in (("byte-identical", peak == 0),
                         ("|d| < 2", peak < 2),
                         ("2 <= |d| <= 20", (peak >= 2) & (peak <= 20)),
                         ("|d| > 20 (moving)", peak > 20)):
        if not select.any():
            continue
        got_in = float(change_in[select].mean())
        got_out = float(change_out[select].mean())
        table.append((name, float(select.mean()), got_in, got_out,
                      got_out / got_in if got_in > 1e-9 else float("inf"),
                      float(lag[select].mean())))
    return table


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("capture", help="a directory of NNN_in.png frames, in order")
    parser.add_argument("--temporal", type=float, nargs="+", default=[0.0, 1.0],
                        help="the history confidences to compare")
    parser.add_argument("--hold", type=float, nargs="+", default=[1.0],
                        help="the hold strengths to compare, one run each")
    parser.add_argument("--render-scale", type=float, default=0.55)
    parser.add_argument("--profile", default="standard")
    parser.add_argument("--socket", default="/tmp/nr_flicker.sock")
    parser.add_argument("--timeout", type=float, default=300)
    parser.add_argument("--limit", type=int, default=0, help="use only the first N frames")
    parser.add_argument("--picture", help="also write the frame-to-frame change as PNGs "
                                          "there, amplified, so it can be looked at")
    args = parser.parse_args()

    paths = sorted(pathlib.Path(args.capture).glob("*_in.png"))
    if args.limit:
        paths = paths[:args.limit]
    if len(paths) < 2:
        raise SystemExit(f"need at least two frames in {args.capture}")
    frames = [image_io.load(str(path)) for path in paths]
    whole = to_bytes(frames[0])[..., 2::-1].astype(np.float32) / 255.0
    region = nr_daemon.active_region(whole)
    print(f"{len(frames)} frames of {frames[0].shape[1]}x{frames[0].shape[0]}, "
          f"active region {region[3] - region[2]}x{region[1] - region[0]}, "
          f"scale {args.render_scale}, profile {args.profile}", flush=True)

    for confidence, hold in [(c, h) for c in args.temporal for h in args.hold]:
        pathlib.Path(args.socket).unlink(missing_ok=True)
        daemon = subprocess.Popen(
            [sys.executable, str(ROOT / "src" / "layer" / "nr_daemon.py"),
             "--socket", args.socket, "--render-scale", str(args.render_scale),
             "--profile", args.profile, "--temporal", str(confidence),
             "--hold", str(hold)],
            stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, text=True)
        try:
            for _ in range(600):
                if pathlib.Path(args.socket).exists():
                    break
                if daemon.poll() is not None:
                    raise SystemExit("the daemon exited before it listened")
                time.sleep(0.5)
            started = time.perf_counter()
            answers = replay(frames, args.socket, args.timeout)
            elapsed = (time.perf_counter() - started) / len(frames)
            outputs = [np.rint(a).astype(np.uint8) for a in answers]
            inputs = [to_bytes(f)[..., 2::-1] for f in frames]
            print(f"\ntemporal {confidence:g}, hold {hold:g}   "
                  f"{1000 * elapsed:.0f} ms/frame", flush=True)
            print(f"  {'pixels':<20}{'share':>8}{'in':>9}{'out':>9}{'x':>9}{'lag':>9}")
            for name, share, got_in, got_out, ratio, lag in statistics(inputs, outputs, region):
                print(f"  {name:<20}{100 * share:7.1f}%{got_in:9.2f}{got_out:9.2f}"
                      f"{ratio:9.2f}{lag:9.2f}")
            if args.picture:
                where = pathlib.Path(args.picture)
                where.mkdir(parents=True, exist_ok=True)
                tag = f"t{confidence:g}-h{hold:g}"
                index = len(frames) // 2
                a = outputs[index - 1].astype(np.float32)
                b = outputs[index].astype(np.float32)
                # x16, so a change of 4 levels of 255 fills the range. Flicker is small by
                # definition; shown at its own scale it is an all-black picture. Masked to
                # the pixels the *game* left alone, because change over a pixel that moved
                # is the picture moving, and change over one that did not is invention —
                # and it is the second that the eye reads as shimmer.
                still = (np.abs(inputs[index].astype(np.int16)
                                - inputs[index - 1]).max(2) == 0)[..., None]
                image_io.save(np.clip(np.abs(b - a) * 16 / 255, 0, 1) * still,
                              where / f"invented-{tag}.png")
                image_io.save(np.clip(np.abs(b - a) * 16 / 255, 0, 1),
                              where / f"change-{tag}.png")
                image_io.save(b / 255, where / f"frame-{tag}.png")
                print(f"  -> {where}/{{invented,change,frame}}-{tag}.png", flush=True)
        finally:
            daemon.terminate()
            daemon.wait(timeout=30)
            pathlib.Path(args.socket).unlink(missing_ok=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
