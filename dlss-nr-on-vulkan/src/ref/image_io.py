#!/usr/bin/env python3
"""
image_io — read and write images without Pillow, via ImageMagick.

The point of this module is the visual feedback loop: a correctly implemented
network fed an image returns a recognisable image, a wrong one returns noise.
That is a test that needs no reference activations, which is the one thing this
project cannot obtain.
"""
import subprocess
import tempfile
from pathlib import Path

import numpy as np


def _magick():
    """ImageMagick, or a sentence saying so.

    Every picture in and out of this project goes through `magick`, and a missing binary
    otherwise surfaces as a CalledProcessError from a subprocess nobody expected.
    """
    import shutil
    if shutil.which("magick") is None:
        raise RuntimeError("ImageMagick is needed to read and write images: install it "
                           "(Arch: `pacman -S imagemagick`). The game path does not need "
                           "it; the still-frame tools do.")
    return "magick"


def load(path, size=None):
    """-> (H, W, 3) float32 in [0,1]."""
    with tempfile.TemporaryDirectory() as temporary:
        d = Path(temporary)
        args = [_magick(), str(path)]
        if size:
            args += ["-resize", "%dx%d!" % (size[1], size[0])]
        args += ["-depth", "8", str(d / "o.rgb")]
        subprocess.run(args, check=True, capture_output=True)
        raw = np.frombuffer((d / "o.rgb").read_bytes(), dtype=np.uint8)
        if size is None:
            probe = subprocess.run([_magick(), "identify", "-format", "%w %h", str(path)],
                                   check=True, capture_output=True, text=True)
            width, height = (int(value) for value in probe.stdout.split()[:2])
            size = (height, width)
        return raw.reshape(size[0], size[1], 3).astype(np.float32) / 255.0


def save(arr, path):
    """arr: (H,W,3) float in [0,1] (clipped) or (H,W) grayscale."""
    a = np.asarray(arr, dtype=np.float32)
    if a.ndim == 2:
        a = np.repeat(a[:, :, None], 3, axis=2)
    a = np.clip(a, 0.0, 1.0)
    H, W, _ = a.shape
    with tempfile.TemporaryDirectory() as temporary:
        d = Path(temporary)
        (d / "i.rgb").write_bytes((a * 255.0 + 0.5).astype(np.uint8).tobytes())
        subprocess.run([_magick(), "-size", "%dx%d" % (W, H), "-depth", "8",
                        "rgb:" + str(d / "i.rgb"), str(path)], check=True, capture_output=True)
    return path


def clean_and_noisy(H, W, seed=0, sigma=0.06):
    """
    A clean frame and the same frame with additive noise.

    This is the objective that attenuation cannot game. Correlation with the input
    rewards doing nothing -- a branch scaled to zero scores perfectly. Denoising does
    not: an identity passes the noise through unchanged, so any improvement in
    noise-to-signal has to come from the network actually computing something useful.
    """
    clean = test_pattern(H, W, seed)
    rng = np.random.default_rng(seed + 1000)
    noisy = np.clip(clean + rng.standard_normal(clean.shape).astype(np.float32) * sigma, 0, 1)
    return clean, noisy


def denoise_score(clean, out):
    """
    Fit out = a*clean + b (the network is free to rescale), then report the residual
    against the clean reference. Lower is better; the identity on a noisy input
    scores exactly the input noise level.

    Returns (score, retained_spread). A configuration that collapses the signal is
    reported as inf rather than as a win -- found the hard way, when a gate form that
    drove the output to sd 0.006 scored better than everything else.
    """
    c = clean.reshape(-1).astype(np.float64)
    o = out.reshape(-1).astype(np.float64)
    a, b = np.polyfit(c, o, 1)
    resid = o - (a * c + b)
    score = float(np.sqrt((resid ** 2).mean()) / max(abs(a), 1e-12))
    # Guard against collapse. Dividing by the fitted gain normalises an honest
    # rescale, but if the output has almost no dynamic range the fit is degenerate
    # and the score becomes meaningless -- 0/0 dressed up as a good result. Require
    # the output to retain a real fraction of the reference's spread.
    keep = float(o.std() / max(c.std(), 1e-12))
    if keep < 0.2:
        return float("inf"), keep
    return score, keep


def test_pattern(H, W, seed=0):
    """
    A frame with the structures a renderer's output actually contains: smooth
    gradients, hard edges, fine high-frequency detail and flat regions. If the
    network is doing anything sensible, edges survive and noise is attenuated.
    """
    rng = np.random.default_rng(seed)
    y, x = np.mgrid[0:H, 0:W].astype(np.float32)
    img = np.zeros((H, W, 3), np.float32)
    img[..., 0] = x / W                                   # horizontal ramp
    img[..., 1] = y / H                                   # vertical ramp
    img[..., 2] = 0.5 + 0.5 * np.sin(x / 16.0) * np.cos(y / 16.0)
    for _ in range(12):                                   # hard-edged blocks
        cy, cx = rng.integers(0, H - 40), rng.integers(0, W - 40)
        h, w = rng.integers(20, 40), rng.integers(20, 40)
        img[cy:cy + h, cx:cx + w] = rng.random(3)
    img[::17, :] = 1.0                                    # thin bright lines
    img[:, ::23] = 0.0
    img += rng.standard_normal(img.shape).astype(np.float32) * 0.03   # sensor noise
    return np.clip(img, 0, 1)

def smooth_pattern(H, W, seed=0):
    """
    A second, structurally unrelated frame: a sum of random low-frequency sinusoids,
    smooth everywhere, with no hard edges and — crucially — **no fixed periods**.

    `test_pattern` has a sin of period 16 and bright/dark lines every 17 and 23 pixels,
    all identical across seeds. The network has 8x8 attention windows and five 2x
    resamplings, so fixed-period content can interact with fixed-period processing and
    produce an effect that is not denoising. Varying only the seed of `test_pattern`
    does not test that; this does.
    """
    rng = np.random.default_rng(seed)
    y, x = np.mgrid[0:H, 0:W].astype(np.float32)
    img = np.zeros((H, W, 3), np.float32)
    for c in range(3):
        acc = np.zeros((H, W), np.float32)
        for _ in range(6):
            fx, fy = rng.uniform(0.5, 6.0), rng.uniform(0.5, 6.0)
            acc += rng.uniform(0.2, 1.0) * np.sin(2 * np.pi * (fx * x / W + fy * y / H)
                                                  + rng.uniform(0, 6.283))
        img[..., c] = acc
    img -= img.min()
    return img / max(float(img.max()), 1e-9)
