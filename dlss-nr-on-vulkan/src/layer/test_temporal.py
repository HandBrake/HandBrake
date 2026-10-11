#!/usr/bin/env python3
"""The temporal path in live mode: the previous output, reprojected by the identity.

A layer at `vkQueuePresentKHR` has no motion vectors, so the history can only be fed
back where it sits. That is right where the scene stood still and wrong where it moved,
and the model's own gate — head channel 4 — is what tells the two apart: 0.705 with
correct history against 0.032 with wrong motion (`notes/phase12-temporal.md`).

What this checks is the contract, not the picture: the reprojection really is the
identity, the composition matches MLX-DLSS's own `compose_temporal`, turning the
confidence to zero is bit-identical to the still path, the history is dropped when the
shot cuts or the geometry moves, and the interface mask still holds once the daemon has
a frame of memory.
"""
import pathlib, socket, struct, subprocess, sys, time
import numpy as np
import nr_daemon

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))
import nr_frame  # noqa: E402
WEIGHTS = nr_frame.WEIGHTS                   # the prepared file when it is there

MAGIC, MAGIC_MASKED = 0x304E524E, 0x314E524E
FORMAT_B8G8R8A8 = 44
SOCKET = "/tmp/nr_temporal_test.sock"
WIDTH, HEIGHT = 256, 192
FAILURES = []


def check(name, ok, detail=""):
    print(f"  [{'ok  ' if ok else 'FAIL'}] {name}{'  ' + detail if detail else ''}", flush=True)
    if not ok:
        FAILURES.append(name)


def scene(seed, *, shift=0):
    """A frame with structure in it. The network on white noise is not a useful subject."""
    rng = np.random.default_rng(seed)
    yy, xx = np.indices((HEIGHT, WIDTH), dtype=np.float32)
    ramp = (xx + shift) / WIDTH
    disc = np.exp(-(((xx - WIDTH * 0.4) ** 2 + (yy - HEIGHT * 0.5) ** 2) / (0.02 * WIDTH * HEIGHT)))
    image = np.stack([ramp, yy / HEIGHT, disc], -1) * 0.8 + 0.1
    image += rng.normal(0, 1.0 / 255, image.shape).astype(np.float32)
    frame = np.zeros((HEIGHT, WIDTH, 4), np.uint8)
    frame[..., 2::-1] = np.clip(image * 255, 0, 255).astype(np.uint8)
    frame[..., 3] = 255
    return frame


# --------------------------------------------------------------------------
# the numerics, with no daemon in the way
# --------------------------------------------------------------------------
def reference_checks():
    features_mod, _composition, _quality, temporal = nr_frame.load_mlx_numpy_modules()
    rng = np.random.default_rng(11)
    height, width = 48, 64
    colour = rng.random((height, width, 3), dtype=np.float32)
    history = rng.random((height, width, 3), dtype=np.float32)
    head = (rng.random((height, width, 4), dtype=np.float32) - 0.5).astype(np.float32)

    yy, xx = np.indices((height, width))
    u = (xx.astype(np.float32) + np.float32(0.5)) / np.float32(width)
    v = (yy.astype(np.float32) + np.float32(0.5)) / np.float32(height)
    check("identity reprojection is bit-exact",
          np.array_equal(temporal.sample_history(history, u, v), history),
          "the five-tap Catmull-Rom collapses to its middle tap at pixel centres")

    vendor = temporal.make_temporal_features(colour, history,
                                             np.zeros((height, width, 2), np.float32),
                                             frame_index=0)
    ours = nr_frame.apply_history(nr_frame.make_features(colour, frame_index=0), history)
    check("the history lands where the vendor puts it",
          np.array_equal(ours, vendor), "all 16 channels, bit-identical")

    reference = temporal.compose_temporal(head, colour, vendor, intensity=1.0)
    composed = nr_frame.compose(head, colour, history=history)
    # The vendor recovers the history from feature channels 7-9, so it has been through
    # `scaled_color` and back; ours never leaves float32. The gap is that round trip.
    check("composition matches `compose_temporal`",
          float(np.abs(composed - reference).max()) < 3e-4,
          f"max |d| {float(np.abs(composed - reference).max()):.2e}, one fp16 round trip")

    check("confidence 0 is the still path exactly",
          np.array_equal(nr_frame.compose(head, colour, history=history,
                                          history_confidence=0.0),
                         nr_frame.compose(head, colour)),
          "bit-identical to `compose_head`")

    ones = np.ones(colour.shape[:2] + (1,), np.float32)
    floored = nr_frame.compose(head, colour, history=history, history_floor=ones)
    predicted = np.clip(colour + features_mod.half(head[..., :3]) * np.float32(0.25), 0, 1)
    at_scale = np.clip(predicted + np.float32(nr_frame.BLEND_SCALE) * (history - predicted), 0, 1)
    check("a floor of 1 holds at exactly the blend scale",
          float(np.abs(floored - at_scale).max()) < 1e-6,
          f"max |d| {float(np.abs(floored - at_scale).max()):.2e} against a hand blend")
    check("a floor of 0 changes nothing",
          np.array_equal(nr_frame.compose(head, colour, history=history,
                                          history_floor=ones * 0),
                         nr_frame.compose(head, colour, history=history)),
          "the floor can only raise the gate, never lower it")

    gate = nr_frame.history_weight(head)
    check("the gate stays inside the blend scale",
          float(gate.min()) >= 0 and float(gate.max()) <= nr_frame.BLEND_SCALE + 1e-6,
          f"{float(gate.min()):.3f}..{float(gate.max()):.3f} of {nr_frame.BLEND_SCALE:.4f}")

    geometry = nr_frame.NetworkGeometry.vendor_aligned(width, height)
    extended = nr_frame.apply_history(
        nr_frame.make_features(colour, geometry=geometry, frame_index=0), history, geometry)
    rows, columns = geometry.source_rows(), geometry.source_columns()
    check("a mirrored extent mirrors the history too",
          np.array_equal(extended[..., 7:10],
                         features_mod.scaled_color(history)[rows[:, None], columns[None, :], :]),
          f"{width}x{height} -> {geometry.network_width}x{geometry.network_height}")


# --------------------------------------------------------------------------
# the holder: when memory is trusted and when it is thrown away
# --------------------------------------------------------------------------
def holder_checks():
    rng = np.random.default_rng(5)
    inner = rng.random((32, 48, 3), dtype=np.float32)
    output = rng.random((64, 96, 3), dtype=np.float32)
    pixels = rng.random((64, 96, 3), dtype=np.float32)
    history = nr_daemon.History()

    check("the first frame has no history",
          history.take("a", inner, 0.15) == (None, None, None), "nothing to carry forward")
    history.keep("a", output, inner, pixels)
    got_inner, got_full, got_pixels = history.take("a", inner, 0.15)
    check("the second frame gets it at both extents",
          got_full is output and got_pixels is pixels
          and got_inner.shape[:2] == inner.shape[:2],
          f"{got_full.shape[1]}x{got_full.shape[0]} composed, "
          f"{got_inner.shape[1]}x{got_inner.shape[0]} for the network")

    history.keep("a", output, inner, pixels)
    check("a changed geometry drops it",
          history.take("b", inner, 0.15) == (None, None, None), "different key")
    history.keep("a", output, inner, pixels)
    check("a changed render scale drops it",
          history.take("a", rng.random((16, 24, 3), np.float32), 0.15) == (None, None, None),
          "the network extent moved")
    history.keep("a", output, inner, pixels)
    check("a cut drops it",
          history.take("a", np.clip(inner + 0.5, 0, 1), 0.15) == (None, None, None),
          f"mean change {history.cut:.3f} over a limit of 0.15")
    history.keep("a", output, inner, pixels)
    history.take("a", inner + np.float32(0.01), 0.15)
    check("ordinary motion keeps it",
          history.output is output, f"mean change {history.cut:.3f} under the limit")
    history.keep("a", output, inner, pixels)
    # the floor, which is what makes the held pixels actually hold
    levels = np.zeros((8, 8, 3), np.float32)
    moved = levels.copy()
    for index, step in enumerate((0, 1, 2, 4, 8)):
        moved[index] = step / 255.0
    floor = nr_daemon.hold_floor(moved, levels, 1.0)[..., 0]
    check("an unchanged pixel is held all the way",
          float(floor[0].min()) == 1.0, "the game handed back the same frame there")
    check("the hold lets go as the pixel moves",
          [round(float(floor[i, 0]), 2) for i in range(5)] == [1.0, 0.75, 0.5, 0.0, 0.0],
          f"0, 1, 2, 4, 8 levels -> {[round(float(floor[i, 0]), 2) for i in range(5)]}")
    check("the strength scales the whole floor",
          float(nr_daemon.hold_floor(moved, levels, 0.5)[0].max()) == 0.5, "hold 0.5")

    check("confidence 0 clears the memory",
          history.take("a", inner, -1.0) == (None, None, None) and history.output is None,
          "so switching the path back on does not resurrect a stale frame")


# --------------------------------------------------------------------------
# the daemon, over the real socket
# --------------------------------------------------------------------------
def request(payload, header, socket_path, want=None):
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(180)
        client.connect(socket_path)
        client.sendall(header + payload)
        want, chunks = want or 4 * WIDTH * HEIGHT, b""
        while len(chunks) < want:
            piece = client.recv(want - len(chunks))
            if not piece:
                break
            chunks += piece
        return np.frombuffer(chunks, np.uint8).reshape(HEIGHT, WIDTH, 4)


def daemon(socket_path, *extra):
    pathlib.Path(socket_path).unlink(missing_ok=True)
    process = subprocess.Popen(
        [sys.executable, str(ROOT / "src" / "layer" / "nr_daemon.py"),
         "--socket", socket_path, *extra],
        stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, text=True)
    for _ in range(300):
        if pathlib.Path(socket_path).exists():
            return process
        if process.poll() is not None:
            raise RuntimeError("daemon exited")
        time.sleep(0.5)
    process.terminate()
    raise RuntimeError("daemon never listened")


def daemon_checks():
    header = struct.pack("<4I", MAGIC, WIDTH, HEIGHT, FORMAT_B8G8R8A8)
    still = scene(1).tobytes()

    path = SOCKET + ".off"
    process = daemon(path, "--temporal", "0")
    try:
        first = request(still, header, path)
        check("with the path off the daemon is still stateless",
              np.array_equal(first, request(still, header, path)),
              "the same frame twice, byte-identical")
    finally:
        process.terminate(); process.wait(timeout=30)
        pathlib.Path(path).unlink(missing_ok=True)

    path = SOCKET + ".on"
    process = daemon(path)
    try:
        frames = [request(still, header, path) for _ in range(8)]
        steps = [float(np.abs(b.astype(np.int16) - a).mean()) for a, b in zip(frames, frames[1:])]
        check("the history reaches the picture",
              steps[0] > 0, f"the second frame moves {steps[0]:.3f} levels off the first")
        # After the step the history makes, a held scene keeps moving by the network's own
        # floor — its answer to a history one rounding different, 0.06-0.2 levels a frame
        # whatever the history's precision (float, rounded or truncated half, measured
        # 2026-09-27) — and never by more. The old test compared the fourth step with the
        # first, which that floor decides by chance: the old history's own fifth step was
        # 0.204 against a first of 0.168. Ringing would be steps that grow.
        later = steps[1:]
        check("a held scene settles to the network's floor rather than ringing",
              sum(later) / len(later) <= steps[0] and max(steps) < 0.5,
              "steps " + " -> ".join(f"{s:.3f}" for s in steps))

        # A cut must not smear the old shot over the new one. Cutting away and back leaves
        # the daemon with no memory it is willing to use, so the answer is the one it gave
        # on the very first frame — which we already have.
        elsewhere = scene(2)
        elsewhere[..., :3] = 255 - elsewhere[..., :3]
        request(elsewhere.tobytes(), header, path)
        check("a cut restarts from no history",
              np.array_equal(request(still, header, path), frames[0]),
              "cut away and back, and the answer is a first frame again")
    finally:
        process.terminate(); process.wait(timeout=30)
        pathlib.Path(path).unlink(missing_ok=True)

    # The control mask reaches the composition, and the wire-byte restore after it is exact
    # by construction. The history is the prediction before either, as the vendor keeps it
    # (`notes/phase70-post-process.md`), so under the mask it holds the network's picture —
    # which must still never reach the answer there, on any frame.
    path = SOCKET + ".mask"
    process = daemon(path)
    try:
        masked_header = struct.pack("<4I", MAGIC_MASKED, WIDTH, HEIGHT, FORMAT_B8G8R8A8)
        colour = scene(3)
        mask = np.zeros((HEIGHT, WIDTH), np.uint8)
        mask[:, WIDTH // 3: 2 * WIDTH // 3] = 255
        held = mask > 127
        pad = 2 * nr_daemon.SOLID_RADIUS
        interior = held.copy()
        interior[:pad] = interior[-pad:] = False
        interior &= np.roll(held, pad, 1) & np.roll(held, -pad, 1)
        body = colour.tobytes() + mask.tobytes()
        worst = 0
        for index in range(3):
            answer = request(body, masked_header, path)
            worst = max(worst, int(np.abs(answer[interior].astype(int) - colour[interior]).max()))
        check("the mask holds once the daemon has memory",
              worst == 0, f"three frames, {interior.sum()} interior pixels, max |d| {worst}")
    finally:
        process.terminate(); process.wait(timeout=30)
        pathlib.Path(path).unlink(missing_ok=True)


def main():
    if not WEIGHTS.exists():
        print(f"{pathlib.Path(__file__).stem}: skipped (no logical weights at "
              f"{WEIGHTS.name}) — a skip is not a pass")
        return 0
    reference_checks()
    holder_checks()
    daemon_checks()
    if FAILURES:
        print(f"\n{len(FAILURES)} FAILED: " + ", ".join(FAILURES), flush=True)
        return 1
    print("\nthe temporal path carries the previous frame and knows when not to", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
