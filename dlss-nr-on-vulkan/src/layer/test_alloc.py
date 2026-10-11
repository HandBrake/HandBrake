#!/usr/bin/env python3
"""The daemon's NumPy allocator on Windows (nr_alloc.c): what it keeps, what it gives back,
and that the daemon answers with the same bytes through it.

The second half runs the daemon's own frame path twice on the same frames. Once on NumPy's
allocator, with a fresh request buffer every frame. Once on this one, with every large block
it hands out filled with 0xff first, and the request buffer kept from frame to frame
(`nr_daemon.receive`). A read of a block before it is written — which fresh pages from the
system, all zero, would have hidden — or of the last frame's request then changes the answer.

    python src/layer/test_alloc.py           # the allocator, then the daemon's frames
    python src/layer/test_alloc.py --quick   # the allocator alone

Exits 77, skipped, off Windows or without work/libnr_alloc.dll.
"""
import argparse
import hashlib
import os
import pathlib
import sys
import threading

import numpy as np

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
for sub in ("src/layer", "src/ref", "src/gpu", "src/bench"):
    sys.path.insert(0, str(ROOT / sub))

import nr_alloc  # noqa: E402

MB = 1 << 20
FAILURES = []


def check(name, ok, detail=""):
    print(f"  [{'ok  ' if ok else 'FAIL'}] {name}{'  ' + detail if detail else ''}", flush=True)
    if not ok:
        FAILURES.append(name)


def allocator():
    nr_alloc.configure(threshold=MB, cap=64 * MB, poison=False)
    nr_alloc.uninstall()
    check("installs in this thread's context", nr_alloc.install(),
          np._core.multiarray.get_handler_name())

    first = np.full(3 * MB // 8, 7.0)
    address = first.ctypes.data
    del first
    before = nr_alloc.counts()
    again = np.empty(3 * MB // 8)
    check("a freed large block serves the next array of its size",
          again.ctypes.data == address and nr_alloc.counts()["hits"] == before["hits"] + 1)
    del again
    zeros = np.zeros(3 * MB // 8)
    check("and np.zeros through it is zeros", zeros.ctypes.data == address and not zeros.any())
    other = np.empty(5 * MB // 8)
    check("another size is a block of its own", other.ctypes.data != address)
    del zeros, other

    before = nr_alloc.counts()
    small = [np.arange(n, dtype=np.int64) for n in range(1, 2000, 7)]
    check("small arrays go to NumPy's own allocator and come back right",
          all(int(a.sum()) == len(a) * (len(a) - 1) // 2 for a in small)
          and nr_alloc.counts()["hits"] == before["hits"]
          and nr_alloc.counts()["misses"] == before["misses"])
    del small

    grown = np.arange(2 * MB, dtype=np.uint8)
    grown.resize(4 * MB, refcheck=False)
    check("a resize goes through NumPy's realloc and keeps the contents",
          np.array_equal(grown[:2 * MB], np.arange(2 * MB, dtype=np.uint8)))
    del grown

    # The frame marks. Released first, so the counts below are this check's alone.
    nr_alloc.uninstall()
    nr_alloc.install()
    a = np.empty(2 * MB, np.uint8)
    del a
    returned = nr_alloc.frame_done()
    check("the end of the frame a block was freed in keeps it",
          returned == 0 and nr_alloc.counts()["slots"] == 1)
    returned = nr_alloc.frame_done()
    check("a mark with nothing in or out since — a status check — changes nothing",
          returned == 0 and nr_alloc.counts()["slots"] == 1)
    b = np.empty(3 * MB, np.uint8)
    del b
    returned = nr_alloc.frame_done()
    check("a block a whole frame did not take goes back at its end",
          returned == 2 * MB and nr_alloc.counts()["slots"] == 1, f"{returned} bytes back")
    before = nr_alloc.counts()
    b = np.empty(3 * MB, np.uint8)
    check("and the one it did take is still there", nr_alloc.counts()["hits"] == before["hits"] + 1)
    del b

    nr_alloc.uninstall()
    nr_alloc.configure(threshold=MB, cap=5 * MB, poison=False)
    nr_alloc.install()
    pair = [np.empty(3 * MB, np.uint8), np.empty(3 * MB, np.uint8)]
    del pair
    counts = nr_alloc.counts()
    check("the cap holds", counts["kept_bytes"] == 3 * MB and counts["slots"] == 1,
          f"{counts['kept_bytes']} bytes in {counts['slots']} slots")

    nr_alloc.configure(threshold=MB, cap=64 * MB, poison=True)
    filled = np.empty(3 * MB, np.uint8)
    check("poison fills a block handed out", bool((filled == 0xff).all()))
    del filled
    zeros = np.zeros(3 * MB, np.uint8)
    check("and np.zeros still zeroes it", not zeros.any())
    del zeros
    nr_alloc.configure(threshold=MB, cap=64 * MB, poison=False)

    errors = []

    def churn(seed):
        try:
            nr_alloc.install()                 # NumPy's handler belongs to the thread's context
            rng = np.random.default_rng(seed)
            for _ in range(200):
                size = int(rng.choice([MB, 2 * MB, 3 * MB]))
                block = np.full(size, seed % 256, np.uint8)
                if not (block == seed % 256).all():
                    errors.append(seed)
                del block
        except Exception as error:             # noqa: BLE001 - reported below
            errors.append(repr(error))

    threads = [threading.Thread(target=churn, args=(seed,)) for seed in range(4)]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    check("four threads at once", not errors, ", ".join(map(str, errors[:3])))

    nr_alloc.uninstall()
    check("uninstalls: NumPy's own handler again",
          np._core.multiarray.get_handler_name() != nr_alloc.NAME
          and nr_alloc.counts()["slots"] == 0)


class Exchange:
    """One frame as the layer sends it, and the answer as the daemon writes it."""

    def __init__(self, request):
        self.request, self.at, self.answer = memoryview(request), 0, bytearray()

    def settimeout(self, seconds):
        pass

    def recv_into(self, view, count):
        count = min(count, len(view), len(self.request) - self.at)
        view[:count] = self.request[self.at:self.at + count]
        self.at += count
        return count

    def sendall(self, data):
        self.answer += data


def daemon_frames():
    import live_rates
    import nr_daemon
    import nr_frame

    if not nr_frame.WEIGHTS.exists():
        print(f"  no weights at {nr_frame.WEIGHTS}: the daemon's half is left out", flush=True)
        return
    backend = nr_frame.ResidentBackend()
    # A frame the letterbox finder sees bars in, and one with an interface mask: each takes
    # a path of its own through the frame (the copy into the full frame, the restore).
    boxed = bytearray(live_rates.frame(1280, 720, 99))
    pixels = np.frombuffer(boxed, np.uint8).reshape(720, 1280, 4)
    pixels[:90, :, :3] = 0
    pixels[-90:, :, :3] = 0
    mask = np.zeros((720, 1280), np.uint8)
    mask[20:80, 40:400] = 255
    plan = []
    for width, height, scale in ((1280, 720, 0.3), (1280, 720, 0.5), (640, 360, 0.5)):
        still = live_rates.frame(width, height, 0)
        frames = [live_rates.frame(width, height, i) for i in (1, 2, 3)] + [still] * 3
        plan += [(width, height, scale, nr_daemon.MAGIC, frame) for frame in frames]
    plan += [(1280, 720, 0.3, nr_daemon.MAGIC, bytes(boxed))] * 2
    plan += [(1280, 720, 0.3, nr_daemon.MAGIC_MASKED, live_rates.frame(1280, 720, 7) + mask.tobytes())] * 2

    def answers(keep):
        if keep:
            nr_alloc.configure(threshold=MB, cap=512 * MB, poison=True)
            nr_alloc.install()
        args = argparse.Namespace(
            socket=None, profile="standard", intensity=1.0, detail_strength=1.0,
            colour_strength=1.0, settings=None, render_scale=1.0, temporal=1.0, hold=1.0,
            release=24.0, min_extent=320.0, cut_limit=0.15, max_pixels=1920 * 1200, dump=None,
            meter=None, timeout=600.0)
        args.live = nr_daemon.Settings(args)
        args.history = nr_daemon.History()
        args.letterbox = nr_daemon.Letterbox()
        digests = []
        for width, height, scale, magic, body in plan:
            args.live.render_scale = scale
            if not keep:
                nr_daemon._INBOX.clear()       # a fresh request buffer every frame, as before
            exchange = Exchange(np.array([magic, width, height, 44], "<u4").tobytes() + body)
            nr_daemon.process_connection(exchange, backend, args)
            if keep:
                nr_alloc.frame_done()
            digests.append(hashlib.sha256(exchange.answer).hexdigest()[:16])
        if keep:
            counts = nr_alloc.counts()
            nr_alloc.uninstall()
            return digests, counts
        return digests, None

    plain, _ = answers(False)
    kept, counts = answers(True)
    differ = [i for i, (a, b) in enumerate(zip(plain, kept)) if a != b]
    check(f"the daemon's {len(plain)} answers, the same bytes with every block poisoned "
          f"and the request buffer kept",
          not differ and len(plain) == len(kept), f"frames {differ}" if differ else "")
    check("and the blocks were reused", counts["hits"] > counts["misses"],
          f"hits {counts['hits']}, misses {counts['misses']}, the last frame kept at most "
          f"{counts['frame_most'] / MB:.0f} MB")
    nr_alloc.configure()
    backend.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--quick", action="store_true", help="the allocator alone")
    options = parser.parse_args()
    if os.name != "nt" or not nr_alloc.LIBRARY.exists():
        print(f"skipped: Windows only, and needs {nr_alloc.LIBRARY}", flush=True)
        return 77
    os.environ.pop("NR_KEEP_BLOCKS", None)
    print("the allocator", flush=True)
    allocator()
    if not options.quick:
        print("the daemon's frames", flush=True)
        daemon_frames()
    if FAILURES:
        print(f"\n{len(FAILURES)} FAILED: " + ", ".join(FAILURES), flush=True)
        return 1
    print("\nall passed", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
