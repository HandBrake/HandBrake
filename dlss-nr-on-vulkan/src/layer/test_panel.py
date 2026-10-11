#!/usr/bin/env python3
"""`nr-panel` under a pseudo-terminal: it draws, it takes keys, and it writes what it says.

Curses code is easy to break and impossible to eyeball from a script, so this runs the
real program on a real pty, presses real keys, and reads what came out. Everything points
at a scratch directory: the panel writes the live settings file, and a test that used the
real one would be reaching into a running game.
"""
import fcntl, json, os, pathlib, pty, re, select, socket, struct, subprocess, sys
import tempfile, termios, threading, time

ROOT = pathlib.Path(__file__).resolve().parents[2]
PANEL = ROOT / "src" / "layer" / "nr-panel"
FAILURES = []


def check(name, ok, detail=""):
    print(f"  [{'ok  ' if ok else 'FAIL'}] {name}{'  ' + detail if detail else ''}", flush=True)
    if not ok:
        FAILURES.append(name)


class Screen:
    """The panel, running, with a keyboard."""

    def __init__(self, environment, rows=24, columns=100):
        self.output = b""
        self.pid, self.fd = pty.fork()
        if self.pid == 0:                                   # the child is the panel
            os.environ.update(environment)
            os.environ["TERM"] = "xterm-256color"
            os.execv(str(PANEL), [str(PANEL)])
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ, struct.pack("HHHH", rows, columns, 0, 0))

    def pump(self, seconds):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            ready, _, _ = select.select([self.fd], [], [], 0.1)
            if ready:
                try:
                    self.output += os.read(self.fd, 1 << 16)
                except OSError:
                    return

    def press(self, keys, settle=0.5):
        os.write(self.fd, keys)
        self.pump(settle)

    def text(self):
        """What is on the glass, with the escape sequences taken out."""
        return re.sub(rb"\x1b\[[0-9;?]*[A-Za-z]|\x1b[()][AB0]|\x1b[=>]|\r", b"",
                      self.output).decode("utf8", "replace")

    def close(self):
        try:
            os.write(self.fd, b"q")
            self.pump(0.6)
        except OSError:
            pass
        try:
            os.kill(self.pid, 15)
        except ProcessLookupError:
            pass
        try:
            os.waitpid(self.pid, 0)
        except ChildProcessError:
            pass
        os.close(self.fd)



def squashed(text):
    """Without any whitespace at all.

    curses paints by moving the cursor, not by writing newlines, so a wrapped sentence
    comes back with its lines butted together and a phrase that spans a wrap is not
    findable as written. Dropping every space makes the comparison independent of where
    the terminal happened to break the line.
    """
    return re.sub(r"\s+", "", text)

# Application cursor keys, not the normal ones. A curses program that calls `keypad`
# puts the terminal into that mode (it emits `ESC [ ? 1 h`), and there is no terminal
# emulator on the other end of a pty to honour it — this harness *is* the terminal, so it
# has to send what the program asked for. Sending `ESC [ D` instead gets the bytes passed
# through raw, which is not what a key press looks like to the program under test.
DOWN, RIGHT, LEFT = b"\x1bOB", b"\x1bOC", b"\x1bOD"


def stand_in_daemon(path):
    """Something that answers, so the panel never starts a real one.

    `space` turns the effect on, and turning it on starts the model if nothing is
    listening — which is the point of the feature and a disaster in a test: five daemons
    were left behind holding 2.3 GiB apiece before this existed. A socket that accepts and
    closes is all `alive()` looks for.
    """
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(str(path))
    server.listen(8)

    def serve():
        while True:
            try:
                connection, _ = server.accept()
            except OSError:
                return
            connection.close()

    threading.Thread(target=serve, daemon=True).start()
    return server


def daemons_running():
    got = subprocess.run(["ps", "-eo", "pid,args"], capture_output=True, text=True)
    return [line.split(None, 1)[0] for line in got.stdout.splitlines()
            if "nr_daemon.py" in line and "ps -eo" not in line]


def main():
    sys.path.insert(0, str(ROOT / "src" / "layer"))
    import nr_knobs

    with tempfile.TemporaryDirectory() as scratch:
        where = pathlib.Path(scratch)
        settings = where / "settings.json"
        log = where / "daemon.log"
        log.write_text(
            "model ready in 0.3s\nlistening on nowhere\n"
            "1024x768 B8G8R8A8_UNORM in 0.20s  change 0.07385  "
            "gate 0.429, held 65%, cut 0.0204  interface 12% left alone  "
            "letterbox 192px of rows and 0px of columns skipped\n")
        environment = {"NR_SETTINGS": str(settings),
                       "NR_LAYER_TRIGGER": str(where / "trigger"),
                       "NR_LAYER_SOCKET": str(where / "sock"),
                       "NR_LAYER_LOG": str(log)}

        before = set(daemons_running())
        server = stand_in_daemon(where / "sock")
        screen = Screen(environment)
        try:
            screen.pump(2.0)
            drawn = screen.text()
            missing = [knob.label for knob in nr_knobs.KNOBS if knob.label not in drawn]
            check("every knob is on the screen", not missing,
                  f"{len(nr_knobs.KNOBS)} of them" if not missing else f"missing {missing}")
            flat = squashed(drawn)
            opening = nr_knobs.BY_NAME["render_scale"].detail.split(". ")[0]
            check("the selected knob explains itself", squashed(opening) in flat,
                  "the detail text of `render scale`, which starts selected")
            missing = [text for text in ("1024x768", "held 65%", "gate 0.429", "5.0 fps")
                       if squashed(text) not in flat]
            check("the daemon's own last frame is read back, not modelled", not missing,
                  "extent, gate, held and the rate, out of its log"
                  if not missing else f"missing {missing}")

            # render scale is first; one step down is 0.05 off the daemon's default of 1.0
            screen.press(LEFT)
            check("a key writes the settings file",
                  settings.exists() and json.loads(settings.read_text()).get("render_scale") == 0.95,
                  json.dumps(json.loads(settings.read_text())) if settings.exists() else "not written")

            order = [knob.name for knob in nr_knobs.KNOBS]

            def walk(start, to):                   # the table's order, not a count of it
                for _ in range(order.index(to) - order.index(start)):
                    screen.press(DOWN)

            walk("render_scale", "profile")
            screen.press(RIGHT)
            check("a choice knob cycles rather than counts",
                  json.loads(settings.read_text()).get("profile") in nr_knobs.PROFILES,
                  json.loads(settings.read_text()).get("profile"))

            for _ in range(40):                    # run into the end of the range
                screen.press(LEFT, settle=0.05)
            screen.pump(0.5)
            profile = json.loads(settings.read_text()).get("profile")
            check("cycling never leaves the list", profile in nr_knobs.PROFILES, str(profile))

            walk("profile", "intensity")
            for _ in range(60):
                screen.press(RIGHT, settle=0.03)
            screen.pump(0.5)
            intensity = json.loads(settings.read_text()).get("intensity")
            check("a number knob stops at its ceiling",
                  intensity == nr_knobs.BY_NAME["intensity"].high, str(intensity))

            screen.press(b"d")
            check("`d` gives a knob back to the daemon",
                  "intensity" not in json.loads(settings.read_text()),
                  "the key is removed, not set to the default value")

            trigger = where / "trigger"
            screen.press(b" ")
            check("space turns the effect on", trigger.exists(), str(trigger))
            screen.press(b" ")
            check("and space turns it off", not trigger.exists())
        finally:
            screen.close()
            server.close()

        alive = screen.output and b"Traceback" not in screen.output
        check("it drew without raising", bool(alive),
              "no traceback on the terminal" if alive else "it crashed")
        # A short window must lose the explanation, not the knobs, and must never write
        # over its own footer. Laid out from the height available rather than a picture.
        # four rows of frame around the list: header, status and footer
        for rows, want_all in ((len(nr_knobs.KNOBS) + 4, True), (len(nr_knobs.KNOBS) + 1, False)):
            small = Screen(environment, rows=rows, columns=70)
            try:
                small.pump(1.5)
                flat = squashed(small.text())
                labels = [knob.label for knob in nr_knobs.KNOBS
                          if squashed(knob.label) not in flat]
                if want_all:
                    check(f"all the knobs still fit in {rows} rows", not labels,
                          "the explanation goes first, the list stays")
                else:
                    check(f"{rows} rows says so rather than ending early",
                          squashed("window too short") in flat,
                          "and the list is clipped above the footer")
            finally:
                small.close()

        leaked = set(daemons_running()) - before
        check("it left no daemon behind", not leaked,
              "the stand-in answered, so the panel had no reason to start one"
              if not leaked else f"leaked {sorted(leaked)}")

    if FAILURES:
        print(f"\n{len(FAILURES)} FAILED: " + ", ".join(FAILURES), flush=True)
        return 1
    print("\nthe panel draws every knob, takes keys, and writes what it shows", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
