#!/usr/bin/env python3
"""The named pipe on Windows (nr_pipe.py) and the daemon's kept request buffer.

The daemon reads a frame straight into a buffer it keeps from one frame to the next, and
writes the answer from where it lies (`nr_daemon.receive`, `NamedPipeConnection.recv_into`
and `sendall`). This sends frames of several sizes through a real pipe, one connection a
frame as the layer makes them, has the server write its answer into the request's own bytes
as the daemon does, and checks:
- every byte that comes back;
- that a frame of the size before arrives in the same buffer;
- the send from a bytearray and from `bytes`;
- a connection closed before its first byte, which is a status check.

    python src/layer/test_pipe.py

Exits 77, skipped, off Windows.
"""
import os
import pathlib
import sys
import threading
import time

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
for sub in ("src/layer", "src/ref", "src/gpu"):
    sys.path.insert(0, str(ROOT / sub))

FAILURES = []


def check(name, ok, detail=""):
    print(f"  [{'ok  ' if ok else 'FAIL'}] {name}{'  ' + detail if detail else ''}", flush=True)
    if not ok:
        FAILURES.append(name)


def pattern(size, seed):
    return bytes((i * 7 + seed) & 0xFF for i in range(256)) * (size // 256) + bytes(size % 256)


def main():
    if os.name != "nt":
        print("skipped: the named pipe is Windows'", flush=True)
        return 77
    import numpy as np
    import nr_daemon
    import nr_pipe

    path = rf"\\.\pipe\nr_test_pipe_{os.getpid()}"
    frame = 1280 * 720 * 4
    rounds = [("bytearray", 1), ("bytes", 4096), ("bytearray", (1 << 20) + 3),
              ("bytearray", frame), ("bytes", frame), ("bytearray", frame),
              ("bytearray", 640 * 360 * 4), ("probe", 0), ("bytearray", frame)]
    seen = []
    server = nr_pipe.NamedPipeServer(path)

    def serve():
        for kind, size in rounds:
            connection, _ = server.accept()
            try:
                before = nr_daemon._INBOX.get("frame")
                # a status check is seen where the daemon sees it, on the 16-byte header
                data = (nr_daemon.receive(connection, 16, probe_ok=True) if kind == "probe"
                        else nr_daemon.receive(connection, size, keep="frame"))
                if data is None:
                    seen.append((kind, None, None))
                    continue
                seen.append((kind, data is before, len(data)))
                # the answer goes into the request's own bytes, as the daemon writes it
                np.bitwise_xor(np.frombuffer(data, np.uint8), np.uint8(0x5A),
                               out=np.frombuffer(data, np.uint8))
                connection.sendall(bytes(data) if kind == "bytes" else data)
            except Exception as error:          # noqa: BLE001 - reported by the client side
                seen.append((kind, repr(error), None))
            finally:
                connection.close()

    thread = threading.Thread(target=serve, daemon=True)
    thread.start()
    answers = []
    for index, (kind, size) in enumerate(rounds):
        for _ in range(2000):
            try:
                pipe = open(path, "r+b", buffering=0)
                break
            except OSError:
                time.sleep(0.005)
        else:
            check("the pipe accepts a connection", False, f"round {index}")
            return 1
        with pipe:
            if kind == "probe":
                answers.append(None)
                continue
            payload = pattern(size, index)
            view = memoryview(payload)
            while view:
                view = view[pipe.write(view):]
            got = bytearray()
            while len(got) < size:
                piece = pipe.read(size - len(got))
                if not piece:
                    break
                got += piece
            expected = (np.frombuffer(payload, np.uint8) ^ np.uint8(0x5A)).tobytes()
            answers.append(bytes(got) == expected)
    thread.join(timeout=60)
    server.close()

    sizes = [size for kind, size in rounds if kind != "probe"]
    check(f"{len(sizes)} frames from 1 byte to {max(sizes)}, every byte back",
          all(answer for answer in answers if answer is not None) and len(answers) == len(rounds),
          f"answers {answers}")
    errors = [entry for entry in seen if isinstance(entry[1], str)]
    check("no error on the daemon's side", not errors, str(errors[:2]))
    reused = [entry[1] for entry in seen]
    # rounds 4-6 are three frames of one size in a row; round 7 changes the size; round 9
    # comes after it
    check("a frame of the size before arrives in the same buffer",
          reused[4] is True and reused[5] is True, f"{reused}")
    check("and a new size gets a new one", reused[6] is False and reused[8] is False)
    check("a connection closed before its first byte is a status check",
          seen[7] == ("probe", None, None))
    check("both sends, from a bytearray and from bytes",
          {kind for kind, *_ in seen if kind != "probe"} == {"bytearray", "bytes"})
    if FAILURES:
        print(f"\n{len(FAILURES)} FAILED: " + ", ".join(FAILURES), flush=True)
        return 1
    print("\nall passed", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
