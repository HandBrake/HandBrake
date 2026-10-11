#!/usr/bin/env python3
"""A daemon the layer started ends with its game, on Windows (`nr_daemon.end_with_game`).

The layer puts the game's process id in NR_LAYER_SPAWNED. Here a sleeping Python stands in
for the game, and the watcher runs in a process of its own, since it ends that process.
Checks:
- while the game runs, the daemon runs;
- when the game exits, the daemon exits within a few seconds, and says why;
- a game that is already gone ends the daemon at once;
- without an id (a daemon started by hand), or with Linux's marker 1, nothing is watched.

    python src/layer/test_spawn_exit.py

Exits 77, skipped, off Windows.
"""
import gc
import os
import pathlib
import subprocess
import sys
import time

HERE = pathlib.Path(__file__).resolve().parent
FAILURES = []
# The daemon's module, imported as the daemon's own main() would have it; prints 'watching'
# once end_with_game has returned, and exits 3 if nothing ended it within a minute.
WATCH = ("import sys, time; sys.path.insert(0, {here!r}); import nr_daemon; "
         "nr_daemon.end_with_game(); print('watching', flush=True); time.sleep(60); sys.exit(3)")


def check(name, ok, detail=""):
    print(f"  [{'ok  ' if ok else 'FAIL'}] {name}{'  ' + detail if detail else ''}", flush=True)
    if not ok:
        FAILURES.append(name)


def game():
    return subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])


def daemon(marker):
    environment = {key: value for key, value in os.environ.items() if key != "NR_LAYER_SPAWNED"}
    if marker is not None:
        environment["NR_LAYER_SPAWNED"] = str(marker)
    return subprocess.Popen([sys.executable, "-c", WATCH.format(here=str(HERE))],
                            env=environment, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)


def first_line(process):
    return (process.stdout.readline() or "").strip()


def ended(process, seconds):
    try:
        return process.wait(timeout=seconds)
    except subprocess.TimeoutExpired:
        return None


def main():
    if os.name != "nt":
        print("skipped: Windows only")
        return 77

    print("the game runs, then exits:")
    stand_in = game()
    watcher = daemon(stand_in.pid)
    lines = [first_line(watcher), first_line(watcher)]
    check("it says what it ends with", lines[0] == f"ends with the game that started it "
          f"(process {stand_in.pid})", lines[0])
    check("and returns to the daemon's start", lines[1] == "watching", lines[1])
    time.sleep(1.5)
    check("while the game runs, the daemon runs", watcher.poll() is None)
    stand_in.kill()
    stand_in.wait()
    gone = time.perf_counter()
    code = ended(watcher, 10)
    check("when the game exits, the daemon exits", code == 0,
          f"exit {code} after {time.perf_counter() - gone:.2f} s")
    rest = watcher.stdout.read().strip() if code is not None else ""
    check("and says why", rest == f"the game that started this daemon (process "
          f"{stand_in.pid}) has exited; stopping", rest)
    if code is None:
        watcher.kill()

    print("the game is already gone:")
    stand_in = game()
    pid = stand_in.pid
    stand_in.kill()
    stand_in.wait()
    # This test's handle is the last one on the process: with it closed, the id is gone too.
    del stand_in
    gc.collect()
    watcher = daemon(pid)
    code = ended(watcher, 30)
    out = watcher.stdout.read().strip() if code is not None else ""
    check("the daemon ends at once", code == 0 and out == f"the game that started this daemon "
          f"(process {pid}) has already exited; stopping", f"exit {code}: {out}")
    if code is None:
        watcher.kill()

    for name, marker in (("no id: a daemon started by hand", None), ("Linux's marker 1", 1),
                         ("not a number", "yes")):
        print(f"{name}:")
        watcher = daemon(marker)
        line = first_line(watcher)
        time.sleep(1.5)
        check("nothing is watched, and the daemon runs", line == "watching" and watcher.poll() is None,
              line)
        watcher.kill()
        watcher.wait()

    if FAILURES:
        print(f"\n{len(FAILURES)} FAILED: " + ", ".join(FAILURES), flush=True)
        return 1
    print("\nall passed", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
