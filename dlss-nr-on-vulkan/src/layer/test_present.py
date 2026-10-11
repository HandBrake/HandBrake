#!/usr/bin/env python3
"""The layer inside a real `vkQueuePresentKHR`, on a surface that needs no screen.

Everything else about the layer had a test and this did not: the copy out, the copy back,
and who waits for whom. It took a game, so it was never run — and the first time it was, it
found that `present_now` called itself, which is a stack overflow on the first present of
any application. That is what this file is for.

`work/test_present` (C) presents 2N frames through a headless swapchain with the layer in
the chain; this drives it against a stand-in daemon and checks both directions:

  - the first pass clears each image to a colour carrying the frame number, and the daemon
    must be handed exactly that. Copying before the clear has finished — a missing wait —
    shows up as the previous contents.
  - the second pass records nothing, so each image still holds what the layer wrote into it
    last time round, and the daemon must be handed its own answer back.

The default and legacy `NR_LAYER_SYNC=semaphore` alias now both use present waits
and private copy fences. A separate draw queue and delayed final clear expose a
missing wait. --negative-control builds a temporary faulty layer and requires
incorrect captured pixels in three runs of each alias; the production library is
never changed. A device without separate presentation support cannot run that control.
"""
import os
import json
import pathlib
import re
import shlex
import socket
import struct
import subprocess
import sys
import tempfile
import threading

import nr_paths

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src"))
import nr_build  # noqa: E402
BINARY = nr_build.executable("test_present")
LAYER = nr_build.BUILD_DIR / "layer-check"      # the manifests, beside the library they name
REPLY = bytes((17, 34, 51, 255))          # B G R A, nothing a clear in this test produces


def reply(sequence):
    """The stand-in's answer to its `sequence`th request: green 34 marks an answer, which
    no clear produces, and blue says which request it answered."""
    return bytes((sequence % 256, REPLY[1], REPLY[2], REPLY[3]))
FAILURES = []
# Where a distribution keeps the Khronos validation layer's manifest. The loader takes a
# list, so ours stays first and the system one is only searched after it.
VALIDATION_DIRS = ("/usr/share/vulkan/explicit_layer.d",
                   "/usr/local/share/vulkan/explicit_layer.d")
VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation"


def validation_environment(environment):
    """Turn on the validation layer, with synchronization validation, or say why not.

    Returns (environment, reason-it-is-off). Opt-in through `NR_TEST_VALIDATION=1`,
    because it needs a package this project does not require
    (`vulkan-validation-layers`) and roughly doubles the time each present takes.

    One wrinkle worth knowing: the validation layer sits **above** this one in the
    loader's chain, so it sees the application's `imageUsage` and never the
    TRANSFER_SRC/DST this layer patches into it. Every transfer barrier on a swapchain
    image then reads as invalid usage. `NR_TEST_TRANSFER_USAGE` makes the test ask for
    those bits itself, which is the only way to tell that artefact from a real finding —
    so validation implies it.
    """
    if os.environ.get("NR_TEST_VALIDATION", "") in ("", "0"):
        return environment, "not asked for (NR_TEST_VALIDATION=1)"
    found = [d for d in VALIDATION_DIRS
             if pathlib.Path(d, "VkLayer_khronos_validation.json").exists()]
    if not found:
        return environment, "the Khronos validation layer is not installed"
    environment = dict(environment)
    environment["VK_LAYER_PATH"] = os.pathsep.join([environment["VK_LAYER_PATH"], *found])
    environment["VK_LOADER_LAYERS_ENABLE"] = VALIDATION_LAYER
    environment["VK_LAYER_VALIDATE_SYNC"] = "1"          # hazards, not only VUIDs
    environment["NR_TEST_TRANSFER_USAGE"] = "1"          # see the docstring
    return environment, None


def validation_messages(text):
    """The validation layer's own complaints, one line each, deduplicated."""
    seen = {}
    for line in (text or "").splitlines():
        match = re.search(r"Validation (?:Error|Warning): \[ ([A-Za-z0-9_-]+) \]", line)
        if match:
            seen[match.group(1)] = seen.get(match.group(1), 0) + 1
    return seen


def check(name, ok, detail=""):
    print(f"  [{'ok  ' if ok else 'FAIL'}] {name}{'  ' + detail if detail else ''}", flush=True)
    if not ok:
        FAILURES.append(name)


def stand_in(path, seen, stop):
    """A daemon that records the frame it was handed and answers with a flat colour."""
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(path)
    server.listen(8)
    server.settimeout(60)
    while not stop.is_set():
        try:
            connection, _ = server.accept()
        except OSError:
            break
        with connection:
            header = connection.recv(16)
            if len(header) < 16:
                continue
            _magic, width, height, _format = struct.unpack("<4I", header)
            want = width * height * 4
            body = b""
            while len(body) < want:
                piece = connection.recv(want - len(body))
                if not piece:
                    break
                body += piece
            # A partly completed clear can have a correct first pixel and stale
            # pixels elsewhere. Validate the whole uniform image, not just its prefix.
            seen.append(body[:4] if len(body) == want and body == body[:4] * (width * height)
                        else b'mixed')
            try:
                connection.sendall(reply(len(seen) - 1) * (width * height))
            except OSError:
                pass          # a layer that stopped listening costs this frame, as the daemon's does
    server.close()


def present(mode, rounds=2, layered=True, pipelined=False):
    """Run the C binary once and return what the daemon saw.

    `layered=False` is the baseline: the same frames with no layer in the chain at all,
    which is how a failure of the layer is told apart from a failure of this harness.
    Nothing reaches the daemon then, by construction.
    """
    with tempfile.TemporaryDirectory() as room:
        path = str(pathlib.Path(room) / "d.sock")
        seen, stop = [], threading.Event()
        thread = threading.Thread(target=stand_in, args=(path, seen, stop), daemon=True)
        thread.start()
        environment = dict(nr_paths.loader_environment(), VK_LAYER_PATH=str(LAYER),
                           ENABLE_NR_LAYER="1", NR_LAYER_SOCKET=path, NR_LAYER_LIVE="1")
        environment.pop("NR_LAYER_TRIGGER", None)
        environment.pop("NR_TEST_NO_LAYER", None)
        environment.pop("NR_LAYER_ASYNC", None)
        if pipelined:
            environment["NR_LAYER_ASYNC"] = "1"
        if not layered:
            environment["NR_TEST_NO_LAYER"] = "1"
            # With no layer nothing patches the usage in, and the test's own transfer
            # barriers would then be invalid against what validation recorded.
            environment["NR_TEST_TRANSFER_USAGE"] = "1"
        if mode:
            environment["NR_LAYER_SYNC"] = mode
        environment, _ = validation_environment(environment)
        got = subprocess.run([str(BINARY), str(rounds)], capture_output=True, text=True,
                             env=environment, timeout=300)
        stop.set()
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as poke:
            try:                                  # unblock the accept so the thread ends
                poke.connect(path)
            except OSError:
                pass
        thread.join(timeout=10)
    return got, seen


def run(mode, attempt=1, attempts=1, pipelined=False):
    label = ((mode or "queue idle") + (", async" if pipelined else "")
             + (f" #{attempt}" if attempts > 1 else ""))
    got, seen = present(mode, pipelined=pipelined)
    if got.returncode != 0:
        check(f"{label}: the frames present", False,
              (got.stderr.strip().splitlines() or ["no output"])[-1][:120])
        return
    # `state=drawn`, not just "drawn": with validation on, the layer's own messages land
    # on this same stream and a looser match would count them as frames.
    images = sum(1 for line in got.stdout.splitlines() if "state=drawn" in line)
    check(f"{label}: the frames present", images > 0 and len(seen) >= images,
          f"{images} drawn, {len(seen)} reached the daemon")
    drawn = seen[:images]
    # blue carries the frame number: a copy that ran before the clear would hold the frame
    # before it, or nothing at all
    expected = [(frame + 1, 128, 64, 255) for frame in range(len(drawn))]
    seen_tuples = [tuple(pixel) for pixel in drawn]
    check(f"{label}: the daemon is handed the frame the game drew",
          seen_tuples == expected,
          f"{seen_tuples[:3]} against {expected[:3]}")
    # Every present is one request, in order. An untouched image holds what the layer wrote
    # into it at its previous present: the answer to that present's own request, or — async —
    # to the request before it, which is the one-frame delay the mode is for. An image
    # whose previous present had no answer yet still holds the frame the game drew there.
    order = [(int(m.group(1)), int(m.group(2)), m.group(3)) for m in
             (re.search(r"frame (\d+) image (\d+) state=(\w+)", line)
              for line in got.stdout.splitlines()) if m]
    last, wrong, untouched = {}, [], 0
    for present_index, (frame, image, state) in enumerate(order):
        if state == "untouched" and present_index < len(seen):
            untouched += 1
            earlier = last.get(image)
            answered = earlier - 1 if pipelined else earlier
            want = (tuple(reply(answered)) if answered is not None and answered >= 0
                    else (earlier + 1, 128, 64, 255))
            if tuple(seen[present_index]) != want:
                wrong.append((present_index, tuple(seen[present_index]), want))
        last[image] = present_index
    check(f"{label}: what the layer wrote is in the image next time round",
          untouched > 0 and not wrong,
          f"{untouched - len(wrong)} of {untouched} untouched frames hold the answer to request "
          f"{'n-1' if pipelined else 'n'}" + (f"; first wrong {wrong[0]}" if wrong else ""))
    if pipelined:
        check(f"{label}: the layer says it is pipelined", "[nr_layer] async:" in got.stderr,
              "its own announcement, not the environment it was given")
    _, off = validation_environment({"VK_LAYER_PATH": ""})
    if not off:
        # The layer's default output is stdout, not stderr; read both so a change of
        # default cannot quietly turn this into a check that can never fail.
        complaints = validation_messages(got.stdout + "\n" + got.stderr)
        check(f"{label}: the validation layer has nothing to say", not complaints,
              ", ".join(f"{n}x {name}" for name, n in complaints.items()) or
              "no errors or warnings, synchronization validation included")


def baseline():
    """The same frames with no layer: the harness on its own must be valid."""
    got, _ = present(None, layered=False)
    drew = sum(1 for line in got.stdout.splitlines() if "state=drawn" in line)
    check("baseline (no layer): the frames present", got.returncode == 0 and drew > 0,
          f"exit {got.returncode}, {drew} drawn")
    _, off = validation_environment({"VK_LAYER_PATH": ""})
    if not off:
        complaints = validation_messages(got.stdout + "\n" + got.stderr)
        check("baseline (no layer): the validation layer has nothing to say", not complaints,
              ", ".join(f"{n}x {name}" for name, n in complaints.items()) or "clean")


def validation_is_live():
    """Prove the validation layer is in the chain, rather than infer it from silence.

    A run with nothing to say looks exactly like a run with no validation layer loaded.
    So the binary is asked to make one deliberate, harmless mistake and the message for it
    has to come back. Without this the whole validation mode is a check that cannot fail.
    """
    environment, off = validation_environment(dict(os.environ, VK_LAYER_PATH=str(LAYER)))
    if off:
        return
    environment["NR_TEST_VALIDATION_PROBE"] = "1"
    got = subprocess.run([str(BINARY), "1"], capture_output=True, text=True,
                         env=environment, timeout=300)
    complaints = validation_messages(got.stdout + "\n" + got.stderr)
    check("validation is actually in the chain",
          "VUID-VkBufferCreateInfo-size-00912" in complaints,
          ", ".join(complaints) or "the deliberate mistake produced no message")


def main():
    if not BINARY.exists():
        print("present: skipped (make work/test_present first) — a skip is not a pass")
        return 0
    if not (LAYER / "VkLayer_dlss_nr.json").exists():
        subprocess.run([sys.executable, str(ROOT / "src" / "layer" / "prepare_layer.py"),
                        str(LAYER)], check=True, capture_output=True)
    probe = subprocess.run([str(BINARY), "1"], capture_output=True, text=True,
                           env=dict(nr_paths.loader_environment(), NR_TEST_NO_LAYER="1"),
                           timeout=300)
    if probe.returncode != 0:
        print("present: skipped (no headless surface here: "
              f"{(probe.stderr.strip().splitlines() or ['?'])[-1][:60]}) — a skip is not a pass")
        return 0
    _, off = validation_environment({"VK_LAYER_PATH": ""})
    print(f"  validation layer: {off if off else 'on, with synchronization validation'}")
    if os.environ.get("NR_TEST_VALIDATION", "") not in ("", "0") and off:
        print("FAILED: validation was requested but is unavailable: " + off)
        return 1
    validation_is_live()
    baseline()
    # One clean run is not evidence: the thing being tested is a race, and a race that
    # does not happen looks like a race that cannot.
    attempts = max(1, int(os.environ.get("NR_TEST_REPEAT", "2")))
    for mode in (None, "semaphore"):
        for attempt in range(1, attempts + 1):
            run(mode, attempt, attempts)
    for attempt in range(1, attempts + 1):
        run(None, attempt, attempts, pipelined=True)
    if FAILURES:
        print("FAILED: " + ", ".join(FAILURES))
        return 1
    print("present: the layer's own copy out and back, both ways of waiting")
    return 0


def negative_control():
    """Prove a missing wait is detected, without changing the working library."""
    global LAYER
    result = main()
    if result:
        return result
    got, _ = present(None)
    if "present queue: separate" not in got.stdout:
        print("negative control: skipped (a separate presentation queue is unavailable)")
        return 77
    original = LAYER
    source = (ROOT / 'src/layer/nr_layer.c').read_text()
    needle = 'uint32_t waits = data->present_wait_count;'
    if source.count(needle) != 1:
        raise RuntimeError('negative control injection point changed; review the test')
    with tempfile.TemporaryDirectory(prefix='nr-missing-wait-') as room:
        room = pathlib.Path(room)
        faulty = room / 'faulty.c'
        faulty.write_text(source.replace(needle, 'uint32_t waits = 0; /* intentional test fault */'))
        library = room / 'libnr_missing_wait.so'
        compiler = shlex.split(os.environ.get('CC', 'cc'))
        # The copy is built away from src/layer, so the headers beside nr_layer.c
        # (nr_transport.h) come in by path.
        command = compiler + ['-O2', '-fPIC', '-shared',
                             '-I' + str(ROOT / 'src/layer'),
                             '-I' + str(ROOT / 'work/vulkan-headers/include')]
        if os.environ.get('VULKAN_SDK'):
            command += ['-I' + str(pathlib.Path(os.environ['VULKAN_SDK']) / 'include')]
        subprocess.run(command + [str(faulty), '-o', str(library), '-lvulkan', '-pthread'], check=True)
        manifest = json.loads((ROOT / 'src/layer/VkLayer_dlss_nr.json').read_text())
        manifest['layer']['library_path'] = str(library)
        manifest['layer']['library_arch'] = str(struct.calcsize('P') * 8)
        (room / 'VkLayer_dlss_nr.json').write_text(json.dumps(manifest))
        try:
            LAYER = room
            for mode in (None, 'semaphore'):
                for attempt in range(3):
                    got, seen = present(mode)
                    drawn = sum('state=drawn' in line for line in got.stdout.splitlines())
                    expected = [(i + 1, 128, 64, 255) for i in range(drawn)]
                    actual = [tuple(pixel) for pixel in seen[:drawn]]
                    if got.returncode != 0 or not drawn or len(seen) < drawn:
                        print('FAILED: faulty layer must reach the data check, not fail for another reason')
                        print(got.stdout + '\n' + got.stderr)
                        return 1
                    if actual == expected:
                        print('FAILED: missing-wait layer escaped the data check')
                        return 1
                    print(f'negative control {mode or "default"} #{attempt + 1}: wrong pixels detected')
        finally:
            LAYER = original
    return 0


if __name__ == "__main__":
    raise SystemExit(negative_control() if '--negative-control' in sys.argv else main())
