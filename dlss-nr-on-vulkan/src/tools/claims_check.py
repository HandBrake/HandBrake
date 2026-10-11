#!/usr/bin/env python3
"""Withdrawn claims must not survive outside the notes that withdrew them.

A withdrawn number is not a typo. It is a claim that was published, checked, found false,
and corrected — and then went on being published somewhere nobody looked. This project has
done it twice: `phase40` found five places in the *code* still asserting a bug that had been
retired, and `phase61`'s corrected parameter count outlived its correction in `bench.py`
for two days, where a stranger on other hardware read it and quoted it back.

`notes/` is exempt: it is the record of how the answer was reached, and a note that says
"this was wrong" has to be able to say what was wrong. Everywhere else — the code, the
README, the architecture — a withdrawn claim may appear only beside its correction, on the
same line or the two around it, so that a reader meeting it also meets the retraction.

    python3 src/tools/claims_check.py          # exit 1 and name every survivor
"""
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
SELF = "src/tools/claims_check.py"
EXEMPT = ("notes/",)
NEAR = 2                                  # lines either side that count as "beside"

# Anything that retracts rather than repeats: cite the note, or say the true thing.
MARKERS = re.compile(r"withdrawn|superseded|was wrong|no longer|not parameters|"
                     r"an earlier|earlier figure|there is no|storage, not|phase61|"
                     r"full MHA|full multi-head|no GQA|\(C, 3C\)", re.I)
# A file that says at the top that it is superseded is a record of how it was thought, kept
# deliberately (`notes/HANDOFF.md` section 3). It may hold the old claim; it may not be
# built on. The marker has to be in the header, not anywhere in the file.
SUPERSEDED = re.compile(r"superseded", re.I)
HEADER = 40


class Claim:
    def __init__(self, name, pattern, truth, note):
        self.name, self.pattern, self.truth, self.note = name, re.compile(pattern), truth, note


WITHDRAWN = (
    Claim("73 841 889 parameters", r"73[\s,]?841[\s,]?889",
          "145 755 123 parameters; 73 841 889 is the weight section's bytes halved",
          "notes/phase61"),
    Claim("grouped-query attention", r"\bGQA\b|grouped-query",
          "qkv_weight is (C, 3C): full multi-head attention", "notes/phase61"),
    Claim("27 % FP16 subnormals", r"27(\.22)?\s*%[^.\n]{0,40}subnormal|subnormal[^.\n]{0,40}27",
          "the real weights hold 7 subnormals; 27 % was measured on the misread decode",
          "notes/phase61"),
    Claim("a weaker pass at render scale 1.0", r"effect comes out weaker|"
          r"(change|effect)[^.\n]{0,40}1\.5(-1\.7)?x\s+smaller",
          "1280x720 at 1.0 ran on a 1280x768 field, which has no padding token in its bottleneck; "
          "on the vendor's 1344x768 the pass is as strong as at 0.9",
          "notes/opendlss-reference.md"),
)


def tracked():
    listing = subprocess.run(["git", "-C", str(ROOT), "ls-files"],
                             capture_output=True, text=True, check=True)
    for name in listing.stdout.split("\n"):
        if not name or name == SELF or name.startswith(EXEMPT):
            continue
        path = ROOT / name
        try:
            yield name, path.read_text().split("\n")
        except (OSError, UnicodeDecodeError):
            continue                       # a binary or unreadable file states no claims


def survivors():
    found = []
    for name, lines in tracked():
        if SUPERSEDED.search("\n".join(lines[:HEADER])):
            continue
        for claim in WITHDRAWN:
            for number, line in enumerate(lines):
                if not claim.pattern.search(line):
                    continue
                window = "\n".join(lines[max(0, number - NEAR):number + NEAR + 1])
                if MARKERS.search(window):
                    continue
                found.append((name, number + 1, claim, line.strip()))
    return found


def main():
    found = survivors()
    for name, number, claim, line in found:
        print(f"{name}:{number}: {claim.name} was withdrawn ({claim.note}).")
        print(f"    says: {line[:96]}")
        print(f"    true: {claim.truth}")
    if found:
        print(f"\n{len(found)} withdrawn claim(s) still published. Correct them, or say "
              f"beside them that they are withdrawn.")
        return 1
    print("no withdrawn claim outside the notes that withdrew it")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
