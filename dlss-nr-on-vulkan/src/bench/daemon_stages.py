#!/usr/bin/env python3
"""Where the daemon's frame goes, stage by stage, on the daemon's own path.

`live_rates.py` times the round trip a game pays, and the daemon's log line says how long the
graph took on the device. What is left between them is everything done at the window's own
resolution, plus the transport. On 2026-10-02 that was ~28 ms of a 1280x720 frame on Windows
and ~7 on Linux, and one number cannot say where the 21 went.

This runs the daemon's `main()` in this process, with every stage `process_connection` calls
wrapped in a timer, and drives it over its own transport, one connection a frame as the layer
does: the Unix socket on Linux, the named pipe on Windows.

Two kinds of frame for each case:
- fresh: `live_rates.py`'s frames, a new pattern every time. Each one is a cut, so there is no
  history. This is what `live_rates.py` measures;
- held: one frame sent again and again, so the history is kept and every pixel is held, as in
  a game's log ("gate ..., held 100%").

    python3 src/bench/daemon_stages.py 1280x720@0.3 1280x720@0.5
    python3 src/bench/daemon_stages.py 1280x720@0.05 --frames 15 --out stages.txt

Each line is a median over the frames, with the least and the most beside it. The derived lines:
- `daemon_rest`: the daemon's own measure from its log, its time less the graph. It runs from
  the decode to the end of the log line, and leaves the receive out;
- `post_send`: the work after the answer has gone (the change figure, the history, the log);
- `rest`: the round trip less the graph.

The next frame is sent only once the daemon has finished the last one, log line included, so no
stage overlaps another frame's.
"""
import argparse
import json
import os
import pathlib
import socket
import statistics
import struct
import sys
import tempfile
import threading
import time

ROOT = pathlib.Path(__file__).resolve().parents[2]
for sub in ("src/bench", "src/layer", "src/ref", "src/gpu"):
    sys.path.insert(0, str(ROOT / sub))

import live_rates  # noqa: E402
import nr_daemon   # noqa: E402
import nr_frame    # noqa: E402

FRAMES = []        # one record of stage times a request, appended when the daemon is done
current = {}       # the request in progress

ORDER = ("receive", "settings", "decode", "letterbox", "resample", "history_take", "features",
         "gpu_in", "gpu_run", "gpu_out", "compose_encode", "compose", "encode", "send",
         "post_send", "to_answer", "total", "round_trip", "daemon_rest", "rest")


def timed(name, function):
    def wrapper(*args, **kwargs):
        started = time.perf_counter()
        try:
            return function(*args, **kwargs)
        finally:
            current[name] = current.get(name, 0.0) + time.perf_counter() - started
    return wrapper


class Connection:
    """The daemon's connection, with the answer's send timed."""

    def __init__(self, inner):
        self._inner = inner

    def __getattr__(self, name):
        return getattr(self._inner, name)

    def sendall(self, data):
        started = time.perf_counter()
        try:
            return self._inner.sendall(data)
        finally:
            now = time.perf_counter()
            current["send"] = current.get("send", 0.0) + now - started
            current["answered"] = now


def instrument():
    """Wrap the stages. The daemon looks each of them up when it calls it, so this holds."""
    for module, name, label in (
            (nr_daemon, "receive", "receive"), (nr_daemon, "decode", "decode"),
            (nr_daemon, "resample", "resample"), (nr_daemon, "encode", "encode"),
            (nr_frame, "build_features", "features"),
            (nr_frame, "compose_encode", "compose_encode"), (nr_frame, "compose", "compose")):
        setattr(module, name, timed(label, getattr(module, name)))
    for cls, name, label in (
            (nr_frame.ResidentBackend, "run_features", "graph_call"),
            (nr_daemon.History, "take", "history_take"),
            (nr_daemon.History, "keep", "history_keep"),
            (nr_daemon.Letterbox, "region", "letterbox"),
            (nr_daemon.Settings, "refresh", "settings")):
        setattr(cls, name, timed(label, getattr(cls, name)))

    geometry = nr_frame.network_geometry

    def network_geometry(*args, **kwargs):
        result = geometry(*args, **kwargs)
        current["field"] = f"{result.network_width}x{result.network_height}"
        return result

    nr_frame.network_geometry = network_geometry
    process = nr_daemon.process_connection

    def process_connection(connection, backend, args):
        current.clear()
        started = time.perf_counter()
        try:
            return process(Connection(connection), backend, args)
        finally:
            ended = time.perf_counter()
            current["to_answer"] = current.get("answered", ended) - started
            current["total"] = ended - started
            current["gpu_in"], current["gpu_run"], current["gpu_out"] = getattr(
                backend, "split", (0.0, 0.0, 0.0))
            FRAMES.append(dict(current))

    nr_daemon.process_connection = process_connection


def connect_pipe(path):
    """The named pipe as the layer opens it. The daemon makes the next instance as it accepts
    the last connection, so this should rarely wait."""
    for _ in range(2000):
        try:
            return open(path, "r+b", buffering=0)
        except OSError:                       # not created yet, or every instance busy
            time.sleep(0.0005)
    raise RuntimeError(f"no pipe at {path}")


def round_trip(endpoint, data, want):
    """One frame and its whole answer, one connection, as the layer sends them."""
    started = time.perf_counter()
    got = 0
    if os.name == "nt":
        with connect_pipe(endpoint) as pipe:
            view = memoryview(data)
            while view:
                view = view[pipe.write(view):]
            while got < want:
                piece = pipe.read(want - got)
                if not piece:
                    raise RuntimeError(f"the daemon answered {got} of {want} bytes")
                got += len(piece)
    else:
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
            client.settimeout(600)
            client.connect(endpoint)
            client.sendall(data)
            while got < want:
                piece = client.recv(want - got)
                if not piece:
                    raise RuntimeError(f"the daemon answered {got} of {want} bytes")
                got += len(piece)
    return time.perf_counter() - started


def drive(endpoint, ready, settings, plan, frames, warmup, out):
    while not ready():
        time.sleep(0.1)
    lines = []
    for width, height, scale in plan:
        for kind in ("fresh", "held"):
            settings.write_text(json.dumps({"render_scale": scale}))
            header = struct.pack("<4I", live_rates.MAGIC, width, height,
                                 live_rates.FORMAT_B8G8R8A8)
            still = live_rates.frame(width, height, 0)
            rows = []
            for i in range(warmup + frames):
                payload = live_rates.frame(width, height, i + 1) if kind == "fresh" else still
                seen = len(FRAMES)
                took = round_trip(endpoint, header + payload, 4 * width * height)
                while len(FRAMES) == seen:                  # until the log line is out
                    time.sleep(0.0005)
                record = dict(FRAMES[-1])
                record["round_trip"] = took
                record["post_send"] = record["total"] - record["to_answer"]
                record["daemon_rest"] = (record["total"] - record.get("receive", 0.0)
                                         - record["gpu_run"])
                record["rest"] = took - record["gpu_run"]
                if i >= warmup:
                    rows.append(record)
            lines.append(f"\n{width}x{height} at {scale:g}, {kind}, network "
                         f"{rows[-1].get('field', '?')}: ms, median (least-most) of {len(rows)}")
            for key in ORDER:
                values = [row.get(key, 0.0) for row in rows]
                if max(values) == 0.0:
                    continue
                lines.append(f"  {key:15} {1000 * statistics.median(values):7.2f}"
                             f"   ({1000 * min(values):.2f}-{1000 * max(values):.2f})")
    text = "\n".join(lines) + "\n"
    if out:
        out.write_text(text, encoding="utf-8")
    print(text, flush=True)
    os._exit(0)                      # the daemon's accept loop has no other way out


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("cases", nargs="+", metavar="WxH@scale")
    parser.add_argument("--frames", type=int, default=15, help="timed frames a case and kind")
    parser.add_argument("--warmup", type=int, default=4,
                        help="frames left out first, while the extent's buffers are built")
    parser.add_argument("--out", type=pathlib.Path, help="also write the table here")
    args = parser.parse_args()
    plan = []
    for case in args.cases:
        extent, _, scale = case.partition("@")
        width, height = extent.lower().split("x")
        plan.append((int(width), int(height), float(scale or 1.0)))

    instrument()
    room = pathlib.Path(tempfile.mkdtemp(prefix="daemon-stages-"))
    settings = room / "settings.json"
    settings.write_text(json.dumps({"render_scale": plan[0][2]}))
    if os.name == "nt":
        endpoint = rf"\\.\pipe\nr_daemon_stages_{os.getpid()}"
        ready = lambda: len(FRAMES) > 0 or _pipe_ready(endpoint)  # noqa: E731
    else:
        endpoint = str(room / "stages.sock")
        ready = lambda: os.path.exists(endpoint)  # noqa: E731
    threading.Thread(target=drive, daemon=True,
                     args=(endpoint, ready, settings, plan, args.frames, args.warmup,
                           args.out)).start()
    sys.argv = [str(ROOT / "src" / "layer" / "nr_daemon.py"), "--socket", endpoint,
                "--settings", str(settings), "--max-pixels", str(1920 * 1200)]
    nr_daemon.main()


def _pipe_ready(endpoint):
    """Whether the daemon is listening. The open counts as a request, which the daemon takes
    for a status check and answers by closing."""
    try:
        open(endpoint, "r+b", buffering=0).close()
    except OSError:
        return False
    return True


if __name__ == "__main__":
    main()
