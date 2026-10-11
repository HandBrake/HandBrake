"""Where, in frequency, the pass puts what it adds, at several render scales.

The daemon's own path over a fake socket, temporal off, on one frame given as a path — a
crop of a real capture keeps its native pixels. Prints, per scale, the mean absolute change
to luma and the share of the added energy in each band of spatial frequency, 1.0 being
Nyquist on either axis. Three 1280x720 DoA5 frames, 2026-09-27: the change is the same at
1.0 as at 0.9, and 1.5-3x more of it sits in the upper half of the band at 1.0 (0.9-9.7 %
against 0.3-6.1 %). A weaker change at 1.0, measured on 2026-09-25, was the padded field:
1280x720 then ran on 1280x768, a field with no padding token in its bottleneck
(notes/opendlss-reference.md).

    python3 src/bench/scale_spectrum.py FRAME.png
"""
import argparse, os, pathlib, struct, sys
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "src" / "layer"), str(ROOT / "src" / "ref"), str(ROOT / "src" / "gpu")]
import nr_daemon as d, nr_frame, image_io
frame = np.ascontiguousarray(np.asarray(image_io.load(sys.argv[1]), np.float32)[..., :3])
H, W = frame.shape[:2]
backend = nr_frame.ResidentBackend()
wire = d.encode(frame, np.full((H, W, 4), 255, np.uint8).tobytes(), 44)
class Conn:
    def __init__(self, data): self.data, self.pos, self.out = memoryview(data), 0, bytearray()
    def recv(self, n):
        c = self.data[self.pos:self.pos + n].tobytes(); self.pos += len(c); return c
    def sendall(self, b): self.out += b
def run(scale):
    args = argparse.Namespace(settings=None, profile="standard", intensity=1.0, detail_strength=1.0,
                              colour_strength=1.0, render_scale=scale, temporal=0.0, cut_limit=0.15,
                              hold=1.0, release=24.0, min_extent=320.0, max_pixels=1 << 22, dump=None, meter=None)
    args.live, args.history, args.letterbox = d.Settings(args), d.History(), d.Letterbox()
    for _ in range(2):
        c = Conn(struct.pack("<4I", d.MAGIC, W, H, 44) + wire)
        with open(os.devnull, "w") as sink:
            import contextlib
            with contextlib.redirect_stdout(sink): d.process_connection(c, backend, args)
    px = np.frombuffer(bytes(c.out[-4 * H * W:]), np.uint8).reshape(H, W, 4)[..., [2, 1, 0]]
    return px.astype(np.float32) / 255
source = np.frombuffer(bytes(wire), np.uint8).reshape(H, W, 4)[..., [2, 1, 0]].astype(np.float32) / 255
luma = lambda im: im @ np.float32([0.2126, 0.7152, 0.0722])
fy = np.fft.fftfreq(H)[:, None]; fx = np.fft.rfftfreq(W)[None, :]
radius = np.sqrt((fy / 0.5) ** 2 + (fx / 0.5) ** 2)          # 1.0 = Nyquist on either axis
bands = [(0, 0.25), (0.25, 0.5), (0.5, 0.75), (0.75, 0.9), (0.9, 1.0), (1.0, 1.5)]
print(f"{'scale':>6} {'change':>7} " + " ".join(f"{a:.2f}-{b:.2f}" for a, b in bands) + "   (share of the added energy, radius in Nyquists)")
for scale in (1.0, 0.95, 0.9, 0.85, 0.8, 0.7):
    out = run(scale)
    delta = luma(out) - luma(source)
    power = np.abs(np.fft.rfft2(delta - delta.mean())) ** 2
    total = power.sum()
    shares = [power[(radius >= a) & (radius < b)].sum() / total for a, b in bands]
    print(f"{scale:6.2f} {np.abs(delta).mean():7.4f} " + " ".join(f"{100 * s:9.1f}" for s in shares))
