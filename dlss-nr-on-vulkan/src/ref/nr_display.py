#!/usr/bin/env python3
"""
nr_display — the display codec that lets the model see a linear-HDR frame.

The network was trained on a display-referred sRGB picture in [0, 1]. A game renders
scene-linear HDR, where highlights run far past 1. Feeding that in directly clips
everything bright; tone-mapping it down and then inverting the curve afterwards is
unstable, because the inverse of a compressive curve amplifies whatever the model
changed in the highlights.

The recovered contract avoids the inverse entirely. `encode` makes an sRGB proxy with
a soft knee above 0.75 luminance; the model runs on that; `resolve` folds the model's
complete picture back onto the **untouched original** by luminance ratio, with an OkLab
hue correction so the rescale does not shift colour.

Ported from MLX-DLSS's `NeuralRenderingDisplayCodec.swift` (Apache-2.0), whose
luminance-ratio composition and OkLab correction follow the MIT-licensed RenoDX design.

    proxy = encode(hdr)
    model = <the network, then compose, on `proxy`>
    output = resolve(proxy, model, hdr)
"""
from __future__ import annotations

from dataclasses import dataclass

import numpy as np

LUMA = np.float32([0.2126, 0.7152, 0.0722])

_OKLAB_M1 = np.float32([[0.41222146, 0.53633255, 0.051445995],
                        [0.2119035, 0.6806995, 0.10739696],
                        [0.08830246, 0.28171885, 0.6299787]])
_OKLAB_M2 = np.float32([[0.21045426, 0.7936178, -0.004072047],
                        [1.9779985, -2.4285922, 0.4505937],
                        [0.025904037, 0.78277177, -0.80867577]])
_OKLAB_M2_INV = np.float32([[1, 0.39633778, 0.21580376],
                            [1, -0.105561346, -0.06385417],
                            [1, -0.08948418, -1.2914855]])
_OKLAB_M1_INV = np.float32([[4.0767417, -3.3077116, 0.23096994],
                            [-1.268438, 2.6097574, -0.3413194],
                            [-0.0041960863, -0.7034186, 1.7076147]])
_TO_AP1 = np.float32([[0.613097, 0.339523, 0.047379],
                      [0.070194, 0.916354, 0.013452],
                      [0.020616, 0.10957, 0.869815]])
_FROM_AP1 = np.float32([[1.705051, -0.621792, -0.083259],
                        [-0.130256, 1.140805, -0.010548],
                        [-0.024003, -0.128969, 1.152972]])


@dataclass(frozen=True)
class DisplayCodec:
    """`transfer_strength = 0` is an exact no-op, which is the test that it is wired
    in correctly rather than merely producing something plausible."""

    white_point: float = 1.0
    transfer_strength: float = 1.0
    colour_strength: float = 1.0
    maximum_luminance_ratio: float = 2.0
    input_is_display_referred: bool = False

    def __post_init__(self):
        if not (np.isfinite(self.white_point) and self.white_point > 0):
            raise ValueError("white_point must be finite and positive")
        for value in (self.transfer_strength, self.colour_strength,
                      self.maximum_luminance_ratio):
            if not np.isfinite(value):
                raise ValueError("codec strengths must be finite")
        if self.maximum_luminance_ratio < 0:
            raise ValueError("maximum_luminance_ratio must not be negative")


def _apply(matrix, value):
    return value @ matrix.T


def luminance(value):
    return value @ LUMA


def linear_to_srgb(value):
    value = np.clip(value, 0.0, 1.0)
    return np.where(value < 0.0031308, value * np.float32(12.92),
                    np.float32(1.055) * np.power(np.maximum(value, 1e-8),
                                                 np.float32(1 / 2.4))
                    - np.float32(0.055)).astype(np.float32)


def srgb_to_linear(value):
    value = np.clip(value, 0.0, 1.0)
    return np.where(value < 0.04045, value / np.float32(12.92),
                    np.power((value + np.float32(0.055)) / np.float32(1.055),
                             np.float32(2.4))).astype(np.float32)


def _signed_cube_root(value):
    return np.where(value < 0, -np.power(np.abs(value), np.float32(1 / 3)),
                    np.power(np.abs(value), np.float32(1 / 3))).astype(np.float32)


def to_oklab(value):
    return _apply(_OKLAB_M2, _signed_cube_root(_apply(_OKLAB_M1, value)))


def from_oklab(value):
    lms = _apply(_OKLAB_M2_INV, value)
    return _apply(_OKLAB_M1_INV, lms ** 3)


def clamp_ap1(value):
    """Fold negatives out in the AP1 primaries, which keeps saturated colour rather
    than clipping a channel to zero in Rec.709."""
    return _apply(_FROM_AP1, np.maximum(_apply(_TO_AP1, value), 0.0))


def hue_correct(incorrect, toward):
    """Keep `incorrect`'s lightness, take `toward`'s hue at `incorrect`'s chroma."""
    incorrect_lab = to_oklab(incorrect)
    correct_lab = to_oklab(toward)
    incorrect_chroma = np.hypot(incorrect_lab[..., 1], incorrect_lab[..., 2])
    correct_chroma = np.hypot(correct_lab[..., 1], correct_lab[..., 2])
    scale = np.where(correct_chroma == 0, np.float32(1),
                     incorrect_chroma / np.where(correct_chroma == 0,
                                                 np.float32(1), correct_chroma))
    result = incorrect_lab.copy()
    result[..., 1] = correct_lab[..., 1] * scale
    result[..., 2] = correct_lab[..., 2] * scale
    return clamp_ap1(from_oklab(result))


def encode(original, codec=DisplayCodec()):
    """Scene-linear HDR -> the sRGB proxy the model expects.

    The knee is on *luminance*, not per channel, so a bright saturated highlight rolls
    off without changing hue.
    """
    original = np.asarray(original, dtype=np.float32)
    if codec.input_is_display_referred:
        return original
    display = np.maximum(original, 0.0) / np.float32(codec.white_point)
    lit = luminance(display)
    rolled = np.float32(0.75) + np.float32(0.25) * (
        1 - np.exp(-(lit - np.float32(0.75)) / np.float32(0.25)))
    scale = np.where(lit > 0.75, rolled / np.maximum(lit, 1e-12), np.float32(1))
    return linear_to_srgb(display * scale[..., None])


def resolve(proxy, model, original, codec=DisplayCodec()):
    """Fold the model's picture back onto the untouched original.

    No inverse tone curve: the model's *ratio* to the proxy is what carries over, so a
    highlight the proxy compressed comes back at the original's own level.
    """
    proxy = np.asarray(proxy, dtype=np.float32)
    model = np.asarray(model, dtype=np.float32)
    original = np.asarray(original, dtype=np.float32)
    if proxy.shape != original.shape or model.shape != original.shape:
        raise ValueError("proxy, model and original must share a shape")
    if codec.transfer_strength == 0:
        return original

    decode = (lambda value: value) if codec.input_is_display_referred else srgb_to_linear
    normalization = np.float32(1 if codec.input_is_display_referred else codec.white_point)
    proxy_pixel, model_pixel = decode(proxy), decode(model)
    original_pixel = original / normalization

    model_lit = luminance(model_pixel)
    original_lit = luminance(original_pixel)
    proxy_lit = luminance(proxy_pixel)

    with np.errstate(divide="ignore", invalid="ignore"):
        darker = original_lit / np.maximum(proxy_lit, 1e-6)
        brighter = ((model_lit + np.maximum(0.0, original_lit - proxy_lit))
                    / np.where(model_lit == 0, np.float32(1), model_lit))
    ratio = np.where(original_lit < proxy_lit, darker, brighter)

    corrected = hue_correct(model_pixel * ratio[..., None], model_pixel)
    upgraded = original_pixel + (corrected - original_pixel) * np.float32(
        codec.transfer_strength)
    upgraded_lit = luminance(upgraded)
    with np.errstate(divide="ignore", invalid="ignore"):
        bounded = np.minimum(np.float32(codec.maximum_luminance_ratio),
                             np.maximum(0.0, upgraded_lit / original_lit))
    luminance_ratio = np.where(original_lit > 1e-6, bounded, np.float32(1))
    tinted = original_pixel * luminance_ratio[..., None]
    result = (tinted + (upgraded - tinted) * np.float32(codec.colour_strength)
              ) * normalization
    # where the model says nothing, the original passes through untouched
    return np.where((model_lit <= 1e-5)[..., None], original,
                    np.maximum(result, 0.0)).astype(np.float32)
