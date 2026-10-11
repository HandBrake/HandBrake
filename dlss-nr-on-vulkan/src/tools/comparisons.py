#!/usr/bin/env python3
"""The README's before/after images and the table under them, rebuilt from `--dump` captures.

Each published image is a frame the owner chose from a capture, cut and laid out as written
below; this is the record of which frame and which pixels, so a new graph can be shown on the
same frames (`src/bench/restill.py` re-renders a capture) and the table measured the same way.

    python3 src/tools/comparisons.py work/restill OUT     # the captures re-rendered, by game
    python3 src/tools/comparisons.py --original OUT       # as captured on 2026-09-16

Writes OUT/<name>.jpg and prints the table's rows. Texture is the luma high-pass RMS (a 5x5
box) divided by the region's own mean, because on this model the level moves and fools the eye;
colour change is the mean absolute difference, in levels of 255. Nothing here is published by
running it: the images go to the orphan `media` branch only when the owner has seen them.
"""
import argparse
import pathlib
import subprocess
import sys
import tempfile

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))
import image_io

# The captures of 2026-09-16, by game: work/<capture>/NNN_{in,out}.png from the daemon's dump.
ORIGINAL = {"tekken": "tekken", "doa5": "doa5cap", "mk1": "mk1cap"}

# (y0, y1, x0, x1) in the captured frame.
IMAGES = {
    # two crops enlarged 2x, a row each, the sheet scaled to 1200 wide
    "tekken7-dragunov": dict(game="tekken", frame=3, crops=[(170, 520, 770, 1090), (590, 880, 700, 1020)],
                             enlarge=2, stack="rows", width=1200, quality=92),
    "doa5-closeup": dict(game="doa5", frame=8, crops=[(60, 640, 640, 1160)], stack="rows", quality=92),
    "doa5-fire": dict(game="doa5", frame=18, crops=[(40, 640, 1080, 1600)], stack="rows", quality=92),
    "mk1-faces": dict(game="mk1", frame=27, crops=[(40, 560, 920, 1520), (60, 580, 40, 640)],
                      stack="rows", quality=92),
    # the whole frame, the game's above the pass's
    "mk1-fight": dict(game="mk1", frame=37, crops=[None], stack="column", quality=90),
}

# The README's table: image, region, where, and whether its brightness is shown.
TABLE = [
    ("Tekken 7", "face", "tekken", 3, (180, 510, 780, 1080), True),
    ("", "jacket weave", "tekken", 3, (600, 900, 1050, 1250), True),
    ("", "embroidery", "tekken", 3, (600, 840, 690, 900), True),
    ("", "background", "tekken", 3, (150, 400, 1400, 1850), True),
    ("DoA5, close-up", "face", "doa5", 8, (60, 640, 640, 1160), True),
    ("", "background", "doa5", 8, (700, 1000, 60, 460), False),
    ("DoA5, by the fire", "face", "doa5", 18, (40, 640, 1080, 1600), True),
    ("", "background, fire", "doa5", 18, (300, 700, 40, 520), False),
    ("Mortal Kombat 1", "Omni-Man's face", "mk1", 27, (40, 560, 920, 1520), True),
    ("", "Homelander's face", "mk1", 27, (60, 580, 40, 640), True),
    ("", "the whole fight frame", "mk1", 37, None, True),
]

LABEL = ["-font", "Adwaita-Sans", "-pointsize", "26", "-fill", "white", "-undercolor", "#000000A0",
         "-gravity", "northwest", "-annotate", "+12+10"]
GAP = "#e8e8e8"


def pair(folders, game, frame):
    folder = folders[game]
    return (image_io.load(folder / f"{frame:03d}_in.png"),
            image_io.load(folder / f"{frame:03d}_out.png"))


def cut(image, crop, enlarge=1):
    if crop is not None:
        y0, y1, x0, x1 = crop
        image = image[y0:y1, x0:x1]
    return image.repeat(enlarge, 0).repeat(enlarge, 1) if enlarge > 1 else image


def build(folders, name, spec, out, room):
    before, after = pair(folders, spec["game"], spec["frame"])
    rows = []
    for index, crop in enumerate(spec["crops"]):
        parts = []
        for tag, image, text in (("b", before, " Game "), ("a", after, " DLSS-NR on Intel Xe2 ")):
            raw = room / f"{index}{tag}.png"
            image_io.save(cut(image, crop, spec.get("enlarge", 1)), raw)
            labelled = room / f"{index}{tag}_l.png"
            subprocess.run(["magick", str(raw), *LABEL, text, str(labelled)], check=True)
            parts.append(str(labelled))
        row = room / f"row{index}.png"
        join = "-append" if spec["stack"] == "column" else "+append"
        spacer = "1x8" if spec["stack"] == "column" else "8x1"
        subprocess.run(["magick", parts[0], "-size", spacer, f"xc:{GAP}", parts[1], join, str(row)],
                       check=True)
        rows.append(str(row))
    command = ["magick", rows[0]]
    for row in rows[1:]:
        command += ["-size", "1x8", f"xc:{GAP}", row]
    if len(rows) > 1:
        command.append("-append")
    if spec.get("width"):
        command += ["-resize", f"{spec['width']}x"]
    target = out / f"{name}.jpg"
    subprocess.run(command + ["-strip", "-quality", str(spec["quality"]), str(target)], check=True)
    return target


def luma(x):
    return x[..., 0] * 0.2126 + x[..., 1] * 0.7152 + x[..., 2] * 0.0722


def box(image, k=5):
    p = k // 2
    c = np.pad(np.pad(image, p, mode="edge").cumsum(0).cumsum(1), ((1, 0), (1, 0)))
    return (c[k:, k:] - c[:-k, k:] - c[k:, :-k] + c[:-k, :-k]) / (k * k)


def stats(a, b):
    la, lb = luma(a), luma(b)
    ta = np.sqrt(((la - box(la)) ** 2).mean()) / la.mean()
    tb = np.sqrt(((lb - box(lb)) ** 2).mean()) / lb.mean()
    return la.mean() * 255, lb.mean() * 255, 100 * (tb / ta - 1), float(np.abs(b - a).mean() * 255)


def table(folders):
    rows = ["| image | region | brightness | relative texture | colour change |",
            "| --- | --- | --- | ---: | ---: |"]
    for image, region, game, frame, where, bright in TABLE:
        before, after = pair(folders, game, frame)
        l0, l1, texture, colour = stats(cut(before, where), cut(after, where))
        rows.append(f"| {image} | {region} | {f'{l0:.0f} -> {l1:.0f}' if bright else ''} "
                    f"| {texture:+.0f} % | {colour:.1f} |")
    return "\n".join(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("captures", nargs="?", type=pathlib.Path,
                        help="a folder with tekken/, doa5/ and mk1/ re-rendered by restill.py")
    parser.add_argument("out", type=pathlib.Path)
    parser.add_argument("--original", action="store_true",
                        help="the captures as taken, from work/tekken, work/doa5cap, work/mk1cap")
    args = parser.parse_args()
    if args.original == (args.captures is not None):
        parser.error("give a captures folder or --original, not both")
    if args.original:
        folders = {game: ROOT / "work" / folder for game, folder in ORIGINAL.items()}
    else:
        folders = {game: args.captures / game for game in ORIGINAL}
    args.out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as directory:
        for name, spec in IMAGES.items():
            target = build(folders, name, spec, args.out, pathlib.Path(directory))
            print(f"  {target}")
    print()
    print(table(folders))


if __name__ == "__main__":
    main()
