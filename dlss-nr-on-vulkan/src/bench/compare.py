#!/usr/bin/env python3
"""A before/after crop, side by side, to look at what the network actually did."""
import pathlib, sys
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "ref"))
import image_io

before = image_io.load(sys.argv[1])
after = image_io.load(sys.argv[2])
y, x, size = (int(v) for v in sys.argv[4:7]) if len(sys.argv) > 6 else (300, 560, 260)
a = before[y:y + size, x:x + size]
b = after[y:y + size, x:x + size]
gap = np.ones((size, 8, 3), np.float32)
image_io.save(np.concatenate([a, gap, b], axis=1), sys.argv[3])
print("wrote %s  (left: input, right: neurally rendered)" % sys.argv[3])
print("  crop sd  %.4f -> %.4f   mean|difference| %.4f"
      % (a.std(), b.std(), np.abs(a - b).mean()))
