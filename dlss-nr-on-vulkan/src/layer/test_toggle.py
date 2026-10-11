#!/usr/bin/env python3
"""`nr-toggle`: the one switch, and the parts of it that do not need a desktop.

Binding a key is Plasma's business and cannot be checked without a session, so what is
checked here is everything underneath it — the key spelling that gets handed to
kglobalaccel as an integer, the flips themselves, that the two tools agree on which files
they are talking about, and that asking the daemon whether it is alive no longer looks
like a failed frame in its log.
"""
import importlib.machinery, importlib.util, os, pathlib, socket, subprocess, sys, tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
FAILURES = []


def check(name, ok, detail=""):
    print(f"  [{'ok  ' if ok else 'FAIL'}] {name}{'  ' + detail if detail else ''}", flush=True)
    if not ok:
        FAILURES.append(name)


def load(path, name):
    """Import a file whose name is not an identifier — both tools are `nr-something`."""
    spec = importlib.util.spec_from_loader(
        name, importlib.machinery.SourceFileLoader(name, str(path)))
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def launcher_checks(toggle, scratch):
    """What `install` prints and what `status` reads back.

    There is no key parsing to check any more: this tool used to register the shortcut
    itself, over kglobalaccel's D-Bus interface, and that crashed the compositor —
    `notes/phase55`. Binding is System Settings' job now, and the only link back to a
    shortcut it made is the `Exec` line of the launcher it wrote.
    """
    check("the launcher command is the script itself",
          toggle.launcher_command("") == str(toggle.HERE),
          toggle.launcher_command(""))
    check("an argument goes on the end, with no stray space",
          toggle.launcher_command("temporal") == f"{toggle.HERE} temporal",
          toggle.launcher_command("temporal"))

    toggle.DESKTOPS = pathlib.Path(scratch) / "applications"
    toggle.DESKTOPS.mkdir(parents=True, exist_ok=True)
    check("nothing bound reads as nothing bound",
          toggle.bound_key("") is None and toggle.bound_key("temporal") is None,
          "no launcher in the directory")
    # A launcher for the *other* command must not be mistaken for this one: the plain
    # form is a prefix of the `temporal` form, and a prefix match would answer both.
    (toggle.DESKTOPS / "someone-elses.desktop").write_text(
        f"[Desktop Entry]\nType=Application\nExec={toggle.HERE} temporal\n")
    check("a launcher is matched on the whole command, not a prefix",
          toggle.bound_key("") is None,
          "the plain command is a prefix of the temporal one")
def flip_checks(toggle, paths):
    # Neither spawning a model nor firing a desktop notification belongs in a test, so
    # both are stood in for. What is checked is that turning it *on* asks for the daemon
    # and turning it off does not — the trigger is separate from the model so that the
    # picture can come and go without paying the load again.
    started, said = [], []
    # `nr_paths.start_daemon`, not `toggle.start_daemon`: it moved to the shared module
    # when the panel wanted it too, and patching the old name silently started a real
    # daemon in the middle of a test run.
    toggle.nr_paths.start_daemon = lambda: started.append(True)
    toggle.notify = lambda title, body: said.append((title, body))
    paths.TRIGGER.unlink(missing_ok=True)
    paths.SETTINGS.unlink(missing_ok=True)
    toggle.effect(False)
    check("off with nothing there is off", not paths.TRIGGER.exists())
    toggle.effect(True)
    check("on makes the trigger", paths.TRIGGER.exists(), str(paths.TRIGGER))
    toggle.effect()
    check("a flip turns it back off", not paths.TRIGGER.exists())
    toggle.effect()
    check("and a flip turns it on", paths.TRIGGER.exists())

    check("turning it on asks for the shared starter", len(started) == 2,
          f"{len(started)} starts across two turns on and two off")
    check("the notification says which way it went",
          [title for title, _body in said].count("Neural rendering off") == 2
          and [title for title, _body in said].count("Neural rendering ON") >= 2,
          "a fullscreen game covers a terminal; it does not cover a notification")

    toggle.temporal()
    check("flipping an unset temporal knob means off",
          paths.read().get("temporal") == 0.0,
          "absent is the daemon's default, which is on — so the first press must turn it off")
    toggle.temporal()
    check("and back on", paths.read().get("temporal") == 1.0)
    paths.SETTINGS.unlink(missing_ok=True)
    paths.TRIGGER.unlink(missing_ok=True)


def agreement_checks(toggle, paths):
    control = load(ROOT / "src" / "layer" / "nr-ctl", "nr_ctl_under_test")
    check("both tools mean the same trigger", control.TRIGGER == paths.TRIGGER == toggle.nr_paths.TRIGGER)
    check("both tools mean the same settings", control.SETTINGS == paths.SETTINGS)
    check("both tools mean the same socket", control.SOCKET == paths.SOCKET)
    knobs = set(control.KNOBS)
    daemon = load(ROOT / "src" / "layer" / "nr_daemon.py", "nr_daemon_under_test")
    missing = set(daemon.Settings.KNOBS) - knobs
    check("nr-ctl can reach every knob the daemon has", not missing,
          f"missing: {sorted(missing)}" if missing else ", ".join(sorted(knobs)))


def report_checks(control, paths, scratch):
    """`nr-ctl report` is what a stranger is asked to paste when frames stop being
    processed. It has to hold together when there is no daemon, no log and no git."""
    import contextlib, io
    log = pathlib.Path(scratch) / "daemon.log"
    paths.LOG = control.nr_paths.LOG = log
    spoken = io.StringIO()
    with contextlib.redirect_stdout(spoken):
        control.report()
    check("report survives a machine with nothing running", "log" in spoken.getvalue(),
          "no daemon, no log")
    log.write_text(
        "model ready in 1s on OLD_DEVICE\n"
        "100x100 old frame\n"
        "model ready in 0.4s on Intel(R) Graphics (LNL)\n"
        "buffers in shared memory (one pool): type 2, heap 11.5 GiB\n"
        "runtime options NR_JOINT_QKV=1 NR_BATCH_FFN=1\n"
        "1280x720 B8G8R8A8_UNORM in 0.21s  change 0.03  "
        "gate 0.69 held 90% cut 0.001  gpu 1+176+1ms  network 704x448 scale 0.55\n"
        "frame rejected/failed; game keeps original: xmx_buf_create: "
        "vkAllocateMemory (resident) (-2)  — the buffers for this extent do not fit\n")
    spoken = io.StringIO()
    with contextlib.redirect_stdout(spoken):
        control.report()
    said = spoken.getvalue()
    check("report preserves real geometry and runtime options",
          'network 704x448 scale 0.55' in said and 'NR_JOINT_QKV=1' in said,
          'long frame lines are not truncated')
    check("report uses only the latest daemon session", 'OLD_DEVICE' not in said and '100x100' not in said)
    check("status has no hardware-independent FPS guess", 'fps of graph time' not in said
          and '7x' not in said)
    check("report carries the GPU, a frame and the refusal",
          "Intel(R) Graphics" in said and "gpu 1+176+1ms" in said and "(-2)" in said
          and "1 refused" in said,
          "the four things a bug report is asked for")


def manual_checks():
    """The documentation says what the programs do, and points at files that exist."""
    got = subprocess.run([sys.executable, str(ROOT / "src" / "tools" / "knob_doc.py"),
                          "--check"], capture_output=True, text=True, timeout=60)
    check("the README's knob table is current", got.returncode == 0,
          got.stdout.strip() or "generated from nr_knobs, so it cannot drift")

    # A specification whose evidence pointers have rotted is worse than one without any:
    # it looks checkable and is not.
    import re

    def resolves(target):
        """`notes/phase45` is how this project cites a note, not a missing file.

        The shorthand is used everywhere and expanding it would be churn; a wrong number
        still has nothing to match, which is the case worth catching.
        """
        if (ROOT / target).exists():
            return True
        stem = pathlib.PurePosixPath(target)
        if stem.parent.name == "notes" and stem.name.startswith("phase"):
            return any((ROOT / "notes").glob(stem.name + "*"))
        return False

    broken = []
    for page in (ROOT / "README.md", ROOT / "docs" / "ARCHITECTURE.md", ROOT / "NOTICE"):
        if not page.exists():
            broken.append(f"{page.name} is missing")
            continue
        text = page.read_text()
        targets = re.findall(r"\]\((?!https?:|mailto:)([^)#]+)\)", text)
        targets += [note if note.startswith("notes/") else f"notes/{note}"
                    for note in re.findall(
                        r"`(notes/[A-Za-z0-9._-]+|phase[0-9][A-Za-z0-9._-]*\.md)`", text)]
        # a renamed program is the other half of the same rot: `src/layer/nr-panel` in a
        # code span is an instruction, and an instruction that names nothing is worse than
        # none. `work/` is deliberately not checked — it is built, not shipped.
        targets += re.findall(r"`((?:src|docs)/[A-Za-z0-9._/-]+)`", text)
        targets += re.findall(
            r"^\s*(?:[A-Z_][A-Z0-9_]*=\S+ )*(?:python3 |gcc [^`\n]*?)?"
            r"((?:src|docs)/[A-Za-z0-9._/-]+)", text, re.M)
        broken += [f"{page.name} -> {target}" for target in targets if not resolves(target)]
    check("the published pages point at files that exist", not broken,
          "README, the architecture and NOTICE" if not broken else "; ".join(broken[:3]))


def probe_checks(daemon):
    """Connect, close, say nothing: that is a liveness probe, not a truncated frame."""
    left, right = socket.socketpair()
    with left, right:
        left.close()
        check("a probe reads as a probe", daemon.receive(right, 16, probe_ok=True) is None,
              "so a status line does not land in the daemon's log as a failure")
    left, right = socket.socketpair()
    with left, right:
        left.sendall(b"12345678")
        left.close()
        raised = False
        try:
            daemon.receive(right, 16, probe_ok=True)
        except EOFError:
            raised = True
        check("a truncated frame still reports", raised, "8 bytes of a 16-byte header")


def main():
    with tempfile.TemporaryDirectory() as scratch:
        os.environ["NR_LAYER_TRIGGER"] = str(pathlib.Path(scratch) / "trigger")
        os.environ["NR_SETTINGS"] = str(pathlib.Path(scratch) / "settings.json")
        os.environ["NR_LAYER_SOCKET"] = str(pathlib.Path(scratch) / "sock")
        sys.path.insert(0, str(ROOT / "src" / "layer"))
        paths = load(ROOT / "src" / "layer" / "nr_paths.py", "nr_paths")
        toggle = load(ROOT / "src" / "layer" / "nr-toggle", "nr_toggle_under_test")
        daemon = load(ROOT / "src" / "layer" / "nr_daemon.py", "nr_daemon_probe")
        launcher_checks(toggle, scratch)
        flip_checks(toggle, paths)
        agreement_checks(toggle, paths)
        report_checks(load(ROOT / "src" / "layer" / "nr-ctl", "nr_ctl_report"), paths, scratch)
        manual_checks()
        probe_checks(daemon)
    if FAILURES:
        print(f"\n{len(FAILURES)} FAILED: " + ", ".join(FAILURES), flush=True)
        return 1
    print("\nthe switch flips, and both tools mean the same files", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
