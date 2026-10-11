#!/usr/bin/env python3
"""What must never be committed, checked against what is.

Publishing a repository publishes every byte of it, including the bytes someone meant to
delete later. This project's one hard rule is that NVIDIA's binary and anything derived
from it stay out, and the second rule — added when publication became a real question —
is that nobody's home directory, mail address or game library comes along either.

    python3 src/tools/publish_check.py            the tracked tree
    python3 src/tools/publish_check.py --history  every commit as well, which is slower

The patterns are shapes, not names: this file is published too, and a check that spelled
out the thing it was hiding would be the leak.
"""
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]

# Extensions that are either the vendor's property, derived from it, or somebody else's
# copyrighted frames. None of them has any business in a source tree.
FORBIDDEN_SUFFIXES = {
    ".dll", ".exe", ".sys", ".so", ".dylib", ".spv", ".air", ".metallib", ".dxil", ".cso",
    ".cubin", ".ptx", ".fatbin",
    ".safetensors", ".pt", ".pth", ".onnx", ".gguf", ".bin", ".npy", ".npz",
    ".png", ".jpg", ".jpeg", ".webp", ".bmp", ".gif", ".mp4", ".webm",
    ".7z", ".zip", ".zst", ".tar", ".gz",
}
LARGE = 200 * 1024

# Large on purpose, and reviewed. A file gets in here only with a reason that survives
# being read out loud: the point of the size limit is to catch what nobody meant to
# commit, not to forbid what somebody decided to.
DELIBERATE = {
    # 304 demangled C++ symbol names from the DLL's shared-memory declarations. Names,
    # not code; regenerable by anyone with the same binary; the evidence that
    # notes/ptx-kernel-configs.md and notes/MODEL-SPEC.txt rest on. Owner's call, and
    # the reasoning is in notes/phase58-before-publishing.md.
    "notes/ptx-demangled.txt",
}
# Before/after comparison frames, on the orphan `media` branch so the code history stays
# text only. Each one is chosen by the owner, not by whoever builds the image — they are
# the project's public face, and they are somebody else's game.
DELIBERATE_PREFIXES = ("comparisons/",)


def deliberate(path):
    return path in DELIBERATE or path.startswith(DELIBERATE_PREFIXES)

PATTERNS = (
    ("a home directory", re.compile(rb"/home/[A-Za-z0-9._-]+/")),
    ("a mail address", re.compile(rb"[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}")),
    ("a mounted volume", re.compile(rb"/(?:mnt|media|run/media)/[A-Za-z0-9._-]+/")),
    ("a private key", re.compile(rb"-----BEGIN [A-Z ]*PRIVATE KEY-----")),
    ("an API token", re.compile(rb"\b(?:ghp|gho|github_pat|sk-[A-Za-z0-9]{20}|AKIA)[A-Za-z0-9_]{10,}")),
)
# The one place a mail address is legitimate: git's own trailers, which are part of how
# the commits were made and are already public in every clone.
# Filesystem-path patterns only: a web address has path segments too, and `/media/` is as
# ordinary in a URL as it is telling in a local path. `file://` is deliberately not exempt —
# that *is* a local path.
PATH_PATTERNS = {"a home directory", "a mounted volume"}
WEB = re.compile(rb"https?://[^\s)>\]\"'`]+")


def in_web_address(blob, start):
    line_start = blob.rfind(b"\n", 0, start) + 1
    line_end = blob.find(b"\n", start)
    line = blob[line_start:line_end if line_end >= 0 else len(blob)]
    offset = start - line_start
    return any(m.start() <= offset < m.end() for m in WEB.finditer(line))


# Addresses that are meant to be public: the trailer git itself writes, and the address
# GitHub hands out precisely so a personal one need not be published.
ALLOWED = re.compile(rb"noreply@anthropic\.com|@users\.noreply\.github\.com"
                     rb"|^noreply@github\.com$")


def tracked():
    got = subprocess.run(["git", "-C", str(ROOT), "ls-files", "-z"],
                         capture_output=True, check=True)
    return [name for name in got.stdout.split(b"\0") if name]


def scan(name, blob, findings):
    path = pathlib.PurePosixPath(name.decode("utf8", "replace"))
    if path.suffix.lower() in FORBIDDEN_SUFFIXES:
        findings.append(f"{path}: a {path.suffix} file is tracked")
    if len(blob) > LARGE and str(path) not in DELIBERATE:
        findings.append(f"{path}: {len(blob) // 1024} KB — too big for source, look at it")
    if b"\0" in blob[:8192]:
        findings.append(f"{path}: binary content")
    for what, pattern in PATTERNS:
        for hit in pattern.finditer(blob):
            if ALLOWED.search(hit.group(0)):
                continue
            if what in PATH_PATTERNS and in_web_address(blob, hit.start()):
                continue
            line = blob.count(b"\n", 0, hit.start()) + 1
            findings.append(f"{path}:{line}: {what}")


def history():
    """Every version of every file, because deleting one does not unpublish it.

    A file removed from the tip still ships with the repository, and so does every line
    that was ever edited out of a file that stayed. This walks the objects rather than the
    commits, so each blob is read once however many commits carry it.
    """
    names = {}
    listed = subprocess.run(["git", "-C", str(ROOT), "rev-list", "--objects", "--all"],
                            capture_output=True, text=True, check=True)
    for row in listed.stdout.splitlines():
        parts = row.split(" ", 1)
        if len(parts) == 2:
            names.setdefault(parts[0], parts[1])

    catalogue = subprocess.run(
        ["git", "-C", str(ROOT), "cat-file", "--batch-all-objects",
         "--batch-check=%(objectname) %(objecttype) %(objectsize)"],
        capture_output=True, text=True, check=True)
    wanted = []
    findings = []
    for row in catalogue.stdout.split("\n"):
        parts = row.split()
        if len(parts) != 3 or parts[1] != "blob":
            continue
        name, size = parts[0], int(parts[2])
        where = names.get(name, "(unnamed blob)")
        if deliberate(where):
            continue
        if pathlib.PurePosixPath(where).suffix.lower() in FORBIDDEN_SUFFIXES:
            findings.append(f"history {name[:9]}: {where} was committed once")
        if size > LARGE:
            findings.append(f"history {name[:9]}: {where}, {size // 1024} KB")
        elif size:
            wanted.append((name, where))

    if wanted:
        reader = subprocess.Popen(["git", "-C", str(ROOT), "cat-file", "--batch"],
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        blobs, _ = reader.communicate(b"".join(f"{name}\n".encode() for name, _ in wanted))
        offset = 0
        for name, where in wanted:
            end = blobs.index(b"\n", offset)
            size = int(blobs[offset:end].split()[2])
            blob = blobs[end + 1:end + 1 + size]
            offset = end + 1 + size + 1
            for what, pattern in PATTERNS:
                for hit in pattern.finditer(blob):
                    if ALLOWED.search(hit.group(0)):
                        continue
                    if what in PATH_PATTERNS and in_web_address(blob, hit.start()):
                        continue
                    findings.append(f"history {name[:9]}: {where} holds {what}")
                    break
    return findings


def authorship():
    """Who every commit says it is from.

    Pushing a repository publishes the author and committer of each commit, and git's
    default is whatever address happens to be in the local config. That is the one field
    nobody edits and everybody forgets — this check did not look at it either until a
    repository was about to be published with a personal mail address in all 123 commits.
    GitHub's `<id>+<login>@users.noreply.github.com` is the address to use instead.
    """
    got = subprocess.run(
        ["git", "-C", str(ROOT), "log", "--all", "--format=%H%x00%ae%x00%ce"],
        capture_output=True, text=True, check=True)
    seen = {}
    for row in got.stdout.splitlines():
        parts = row.split("\0")
        if len(parts) != 3:
            continue
        for address in parts[1:]:
            # `noreply@github.com` is the committer GitHub's own web editor writes, not a
            # personal address; the author beside it is the one worth looking at.
            if ALLOWED.search(address.encode()):
                continue
            seen.setdefault(address, parts[0])
    return [f"authorship: {address} — {count_commits(address)} commit(s), first {sha[:9]}"
            for address, sha in sorted(seen.items())]


def count_commits(address):
    got = subprocess.run(
        ["git", "-C", str(ROOT), "log", "--all", "--format=%ae %ce"],
        capture_output=True, text=True, check=True)
    return sum(1 for row in got.stdout.splitlines() if address in row)


def main():
    findings = []
    for name in tracked():
        try:
            blob = (ROOT / name.decode()).read_bytes()
        except OSError as error:
            findings.append(f"{name.decode()}: unreadable ({error})")
            continue
        scan(name, blob, findings)

    if "--history" in sys.argv:
        findings += history()
        findings += authorship()

    if findings:
        print(f"{len(findings)} thing(s) that should not be published:\n")
        for line in sorted(set(findings)):
            print(f"  {line}")
        return 1
    print("nothing tracked that should not be published")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
