#!/usr/bin/env python3
"""What each control actually does, decomposed by spatial frequency.

The vendor's panel calls them tone, structure and skin structure. This sweeps each one
on a real frame and splits the difference it makes into a low-frequency band (broad
lighting and local contrast) and a high-frequency band (micro-detail), so "what responds
to what" is a number rather than an adjective.

Every value in a sweep costs a forward pass, because the conditioning scalars land in
feature channels 10-14 — except `intensity`, which is a composition blend and is free.
"""
import argparse, pathlib, sys
import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))
import image_io, nr_frame  # noqa: E402


def bands(image, sigma=2.0):
    """Split into low and high frequency with a small separable Gaussian."""
    radius = max(1, int(3 * sigma))
    x = np.arange(-radius, radius + 1, dtype=np.float32)
    kernel = np.exp(-0.5 * (x / sigma) ** 2)
    kernel /= kernel.sum()
    low = image
    for axis in (0, 1):
        padded = np.apply_along_axis(
            lambda row: np.convolve(np.pad(row, radius, mode="edge"), kernel, "valid"),
            axis, low)
        low = padded
    return low, image - low


def concentration(delta):
    """How clustered a change is: the top decile's mean over the rest's.

    A colour-based skin prior was the first instrument tried here and it was the wrong
    one — it said `skin_structure` acted uniformly when the difference map is plainly a
    face. Asking whether a change is *concentrated* needs no prior about where.
    """
    flat = np.sort(delta.reshape(-1))
    cut = max(1, flat.size // 10)
    top, rest = flat[-cut:], flat[:-cut]
    return top.mean() / max(rest.mean(), 1e-9)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input")
    parser.add_argument("--size", type=int, default=384)
    parser.add_argument("--sheet", help="write a contact sheet of the sweeps here")
    args = parser.parse_args()

    colour = image_io.load(args.input, size=(args.size, args.size))
    model = nr_frame.ResidentBackend()
    print(f"  {args.size}x{args.size}\n")

    def render(**kw):
        head, _ = nr_frame.run_head(model, colour, **kw)
        return nr_frame.compose(head, colour)

    base = render()
    base_low, base_high = bands(base)

    def report(label, image):
        low, high = bands(image)
        delta = np.abs(image - base)
        dl, dh = np.abs(low - base_low), np.abs(high - base_high)
        total = dl.mean() + dh.mean() + 1e-12
        print("  %-26s change %.5f   low %4.0f%% / high %4.0f%%   concentration %5.1fx"
              % (label, delta.mean(), 100 * dl.mean() / total, 100 * dh.mean() / total,
                 concentration(delta.mean(axis=2))))

    print("  reference: profile=standard, everything at its default\n")
    print("  -- local tone strength (vendor range 0..2, default 1.0) --")
    for value in (0.0, 0.5, 1.5, 2.0):
        report(f"local_tone={value}", render(local_tone=value))
    print("\n  -- local structure strength (vendor range 0..2, default 1.5) --")
    for value in (0.0, 0.5, 1.5, 2.0):
        report(f"local_structure={value}", render(local_structure=value))
    print("\n  -- skin structure, with the automatic mask on (vendor default 2.0) --")
    for value in (-1.0, 0.0, 1.0, 2.0):
        mask = nr_frame.AutomaticMask(skin_structure_strength=value,
                                      automatic_mask_structure_strength=-1.0)
        report(f"skin_structure={value}", render(automatic_mask=mask))
    print("\n  -- style index (0, 1, 2 are the presets; normalised by 1/128) --")
    for value in (1, 2, 4, 8):
        report(f"style_index={value}", render(style_index=value))
    print("\n  -- intensity: a composition blend, free, no forward pass --")
    head, _ = nr_frame.run_head(model, colour)
    for value in (0.5, 1.5, 2.0):
        report(f"intensity={value}", nr_frame.compose(head, colour, intensity=value))

    if args.sheet:
        rows = [
            [colour, base] + [render(local_tone=v) for v in (0.0, 2.0)],
            [colour, base] + [render(local_structure=v) for v in (0.0, 2.0)],
            [colour, base] + [render(style_index=v) for v in (1, 2)],
            [colour, base] + [nr_frame.compose(head, colour, intensity=v) for v in (0.5, 2.0)],
        ]
        sheet(args.sheet, rows,
              ["source | default | local_tone 0 | local_tone 2",
               "source | default | local_structure 0 | local_structure 2",
               "source | default | style 1 | style 2",
               "source | default | intensity 0.5 | intensity 2"], args.size)


def sheet(path, rows, labels, size):
    """A contact sheet: one row per control, one column per value, plus the reference."""
    gap, pad = 4, 1.0
    height = width = size
    columns = max(len(row) for row in rows)
    canvas = np.full(((height + gap) * len(rows) - gap,
                      (width + gap) * columns - gap, 3), pad, np.float32)
    for r, row in enumerate(rows):
        for c, image in enumerate(row):
            y, x = r * (height + gap), c * (width + gap)
            canvas[y:y + height, x:x + width] = image
    image_io.save(canvas, path)
    print("\n  wrote %s" % path)
    for r, label in enumerate(labels):
        print("    row %d: %s" % (r + 1, label))


if __name__ == "__main__":
    main()
