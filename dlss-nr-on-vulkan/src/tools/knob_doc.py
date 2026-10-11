#!/usr/bin/env python3
"""Render the generated parts of the README, so the manual cannot drift from the program.

Two blocks: the knobs, and the measured frame times. Both come from `nr_knobs`, which the
control tools read as well, so the page and the programs cannot disagree.

    python3 src/tools/knob_doc.py            print them
    python3 src/tools/knob_doc.py --check    exit 1 if README.md is out of date
    python3 src/tools/knob_doc.py --write    put them back into README.md

`test_toggle.py` runs the check, so a knob added to `nr_knobs` and not to the README is a
failing test rather than a surprise for whoever reads the README next. The rates table is
here for a worse reason: the hand-written one survived the host passes moving to C and was
understating this machine by a third, and at 1080p by half, for a week.
"""
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "layer"))
import nr_knobs  # noqa: E402

README = ROOT / "README.md"


def knobs():
    lines = []
    for knob in nr_knobs.KNOBS:
        if knob.kind == "choice":
            span = " / ".join(f"`{name}`" for name in nr_knobs.PROFILES)
        else:
            span = (f"`{knob.low:g}` to `{knob.high:g}`, step `{knob.step:g}`, "
                    f"default `{knob.default:g}`")
        lines += [f"### `{knob.name}` — {knob.summary}", "", span, "",
                  knob.detail.replace(" -> ", " → "), ""]
    return lines


def rates():
    lines = [f"Measured through the socket on {nr_knobs.RATES_MEASURED} by "
             "`python3 src/bench/live_rates.py` — the whole round trip a game waits for, "
             "median of nine frames, not graph time alone:", "",
             "| swapchain | render scale | ms | fps |", "| --- | ---: | ---: | ---: |"]
    for width, height, scale, ms in nr_knobs.RATES:
        lines.append(f"| {width}x{height} | {scale:.2f} | {ms:.0f} | {1000 / ms:.1f} |")
    if nr_knobs.RATES_NOTE:
        lines += ["", nr_knobs.RATES_NOTE]
    lines += ["",
              "That is the daemon's own cost with nothing else on the GPU. A game adds its "
              "own frame to it: **Tekken 7** ran at **30 fps at 800x450** with the render "
              "scale at 0.35, and 27 at 0.6, in a live session on 2026-09-27 — against 10.5 fps "
              "at 640x360 on 2026-09-16 (`notes/phase59`).", ""]
    return lines


BLOCKS = {"knobs": knobs, "rates": rates}


def markdown(name):
    return "\n".join([f"<!-- {name}:begin -->", ""] + BLOCKS[name]() + [f"<!-- {name}:end -->"])


def rendered(text):
    """`text` with every generated block replaced by what the program says now."""
    for name in BLOCKS:
        begin, end = f"<!-- {name}:begin -->", f"<!-- {name}:end -->"
        if begin not in text or end not in text:
            raise LookupError(f"{README.name} has no {begin} ... {end} block")
        head, rest = text.split(begin, 1)
        _stale, tail = rest.split(end, 1)
        text = head + markdown(name) + tail
    return text


def main():
    what = sys.argv[1] if len(sys.argv) > 1 else "--print"
    if what == "--print":
        print("\n\n".join(markdown(name) for name in BLOCKS))
        return 0
    text = README.read_text() if README.exists() else ""
    try:
        fresh = rendered(text)
    except LookupError as missing:
        print(missing)
        return 1
    if what == "--check":
        if fresh != text:
            print("README.md is out of date: python3 src/tools/knob_doc.py --write")
            return 1
        return 0
    if what == "--write":
        README.write_text(fresh)
        print(f"wrote the generated blocks into {README}")
        return 0
    print(__doc__.strip())
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
