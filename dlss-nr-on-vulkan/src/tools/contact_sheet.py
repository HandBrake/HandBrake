#!/usr/bin/env python3
"""A numbered contact sheet of a `--dump` capture, for choosing the frames worth a still.

    python3 src/tools/contact_sheet.py work/shots work/shots_sheet.jpg
    python3 src/tools/contact_sheet.py work/shots sheet.jpg --pairs     # the game's | the pass's

Each tile is one dumped frame, labelled with its number, so a choice can be made by number and
the full-resolution frames found again as NNN_in.png / NNN_out.png. The sheet stays local: it
is made of game frames, and which of them are shown anywhere is the owner's to choose.
"""
import argparse
import pathlib
import subprocess
import tempfile

LABEL = ["-font", "Adwaita-Sans", "-pointsize", "28", "-fill", "yellow", "-stroke", "black",
         "-strokewidth", "1", "-gravity", "northwest", "-annotate", "+8+4"]


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("dump", type=pathlib.Path, help="a folder of NNN_in.png / NNN_out.png")
    parser.add_argument("out", type=pathlib.Path)
    parser.add_argument("--width", type=int, default=320, help="a tile's width, per picture")
    parser.add_argument("--columns", type=int, default=5)
    parser.add_argument("--pairs", action="store_true",
                        help="each tile the game's frame beside the pass's, not the game's alone")
    args = parser.parse_args()
    frames = sorted((path for path in args.dump.glob("*_in.png") if path.stem.split("_")[0].isdigit()),
                    key=lambda path: int(path.stem.split("_")[0]))
    if not frames:
        raise SystemExit(f"no NNN_in.png in {args.dump}")
    with tempfile.TemporaryDirectory() as directory:
        room = pathlib.Path(directory)
        tiles = []
        for path in frames:
            number = path.stem.split("_")[0]
            tile = room / f"{number}.png"
            if args.pairs:
                after = path.with_name(f"{number}_out.png")
                subprocess.run(["magick", str(path), str(after), "-resize", f"{args.width}x",
                                "+append", *LABEL, number, str(tile)], check=True)
            else:
                subprocess.run(["magick", str(path), "-resize", f"{args.width}x", *LABEL, number,
                                str(tile)], check=True)
            tiles.append(str(tile))
        subprocess.run(["magick", "montage", *tiles, "-tile", f"{args.columns}x", "-geometry", "+3+3",
                        "-background", "#202020", "-strip", "-quality", "90", str(args.out)], check=True)
    print(f"  {args.out}: {len(frames)} frames, {frames[0].stem.split('_')[0]}-{frames[-1].stem.split('_')[0]}")


if __name__ == "__main__":
    main()
