#!/usr/bin/env python3
"""Where a frame's time actually goes, per operation, from GPU timestamps.

`split_cost.py` answers the same question by ablation — run the frame with one half of
the passes removed and difference the wall time — and says itself that the result is
approximate: skipping passes changes the values the rest of the graph works on, and the
buffers stay hot in ways the real frame's do not.

This does not ablate anything. `xmx_profile()` puts one timestamp query after each
recorded pass, so a pass costs `ts[i] - ts[i-1]` on the device's own clock. The barrier
already between passes makes that attribution exact. The frame measured is the frame
that would have run.

    python3 src/bench/frame_profile.py [--size H W] [--runs N] [--warm N] [--calls N]

`--calls` also lists the N most expensive call sites — each pass labelled by the entry
point that recorded it and its shape — because a total per kind cannot say which GEMM of
the hundreds in a frame is the expensive one.

`--warm` runs that many frames before the timed ones. On Linux the GPU's clock is still
climbing through the first frames, and five timed 320x320 frames after one warm one read
31.7 ms of device time where thirty warm frames first read 24.4. Compare across machines
warm.
"""
import argparse
import pathlib
import re
import sys
import time

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src" / "gpu"))
sys.path.insert(0, str(ROOT / "src" / "ref"))
import nr_frame_resident as F
import nr_model
import xmxres

# Both tables are read from the shaders rather than written out here: a hand-kept
# copy silently mislabels every row the moment a kind is added, and this file
# shipped one such mistake before the tables were generated.
def _kinds(path, pattern):
    # encoding is explicit: the shaders are not ASCII, and on Windows the default is
    # the ANSI code page rather than UTF-8.
    text = (ROOT / path).read_text(encoding="utf-8")
    return {int(v): n.lower().replace("_", " ")
            for n, v in re.findall(pattern, text)}


UNARY = _kinds("src/gpu/resident.comp", r"([A-Z][A-Z0-9_]*)\s*=\s*(\d+)u")
ROW = _kinds("src/gpu/attention.comp", r"([A-Z][A-Z0-9_]*)\s*=\s*(\d+)u")
# The fused window attention is a row-family pass too, and names its own kind. Two
# shaders sharing one kind number is how it hid as "qkv prepare" for a while: both said
# 2, and only one of them was read. So both are read, and a shared number is an error.
_WINDOW = _kinds("src/gpu/window_attention.comp", r"(WINDOW_[A-Z0-9_]*)\s*=\s*(\d+)u")
_BLOCK = _kinds("src/gpu/window_block.comp", r"(WINDOW_BLOCK)\s*=\s*(\d+)u")
_GLOBAL = _kinds("src/gpu/global_attention.comp", r"(GLOBAL_ATTENTION)\s*=\s*(\d+)u")
if set(_WINDOW) & set(_BLOCK) or set(_GLOBAL) & (set(_WINDOW) | set(_BLOCK)):
    raise SystemExit("row profile kinds collide between window_attention.comp, "
                     "window_block.comp and global_attention.comp")
_WINDOW.update(_BLOCK)
_WINDOW.update(_GLOBAL)
_CLASH = set(ROW) & set(_WINDOW)
if _CLASH:
    raise SystemExit(f"row profile kinds collide between attention.comp and "
                     f"window_attention.comp: {sorted(_CLASH)}")
ROW.update(_WINDOW)


def label(family, sub):
    if family == "unary":
        return "unary: %s" % UNARY.get(sub, "kind %d" % sub)
    if family == "row":
        return "row: %s" % ROW.get(sub, "kind %d" % sub)
    if family == "gemm" and sub == 31:
        return "ffn fused"          # libxmx stamps the fused feed-forward as kind 31
    if family.startswith("gemm"):
        return "%s (flags %d)" % (family, sub)
    return family


def _describe(name, args):
    """One recorded pass, as the entry point and the arguments that shape it."""
    if name == "xmx_rec_gemm":
        m, n, k, batch, bt = args[3], args[4], args[5], args[6], args[10]
        return "gemm %dx%dx%d%s flags %#x" % (m, n, k, " x%d" % batch if batch > 1 else "", bt)
    if name == "xmx_rec_gemm_residual":
        return "gemm+residual %dx%dx%d flags %#x" % args[5:9]
    if name == "xmx_rec_gemm_window_residual_pool":
        return "gemm+window residual %dx%dx%d, pooled and published" % args[6:9]
    if name == "xmx_rec_gemm_window_residual":
        return "gemm+window residual %dx%dx%d flags %#x" % args[5:9]
    if name == "xmx_rec_gemm_qkv":
        m, c = args[6], args[7]
        return "gemm+qkv epilogue %dx%dx%d" % (m, 3 * c, c)
    if name == "xmx_rec_gemm_qkv_window":
        m, c = args[6], args[7]
        return "gemm+qkv epilogue %dx%dx%d, window gather%s%s" % (
            m, 3 * c, c, " (half image)" if args[14] & 1 else "",
            " (published)" if args[14] & 2 else "")
    if name == "xmx_rec_ffn":
        return "ffn fused %dx%dx%d x%d flags %#x" % (args[6], args[7], args[8], args[9], args[10])
    if name == "xmx_rec_ffn_merge":
        return "ffn fused %dx32x128, input merged from %dx%d" % (
            args[7] * args[8], args[7] // 2, args[9])
    if name == "xmx_rec_ffn_stem":
        return "ffn fused %dx32x128, stem made from the features" % args[6]
    if name == "xmx_rec_window_block":
        return "window block %d windows%s%s" % (args[8], ", half image" if args[13] & 0x8000 else "",
                                                ", pooled" if args[13] & 0x800000 else "")
    if name == "xmx_rec_gemm_dual":
        return "gemm+half copy %dx%dx%d" % args[4:7]
    if name == "xmx_rec_unary2":
        return "unary %s n=%d C=%d, two outputs" % (
            UNARY.get(args[0] & 0xFF, "kind %d" % (args[0] & 0xFF)), args[6], args[7])
    if name == "xmx_rec_unary":
        return "unary %s n=%d C=%d" % (UNARY.get(args[0] & 0xFF, "kind %d" % (args[0] & 0xFF)),
                                       args[5], args[6])
    if name == "xmx_rec_row":
        return "row %s rows=%d" % (ROW.get(args[0] & 0xFF, "kind %d" % (args[0] & 0xFF)), args[5])
    if name == "xmx_rec_global_attention":
        return "global attention %d rows, %d tokens, %d heads" % args[4:7]
    if name == "xmx_rec_window_attention":
        return "window attention %d batches, %d heads%s" % (
            args[5], args[6], ", merged" if len(args) > 7 and args[7] else "")
    return name[len("xmx_rec_"):]


def record_calls(lib):
    """Label every pass as it is recorded. Each `xmx_rec_*` entry point stamps exactly one
    pass, so the labels line up one for one with `xmxres.profile_each()`."""
    log = []
    for name in [n for n in dir(lib) if n.startswith("xmx_rec_")] + [
            n for n in ("xmx_rec_gemm", "xmx_rec_gemm_residual", "xmx_rec_gemm_window_residual",
                        "xmx_rec_gemm_qkv", "xmx_rec_unary", "xmx_rec_row", "xmx_rec_qkv",
                        "xmx_rec_window_attention", "xmx_rec_copy", "xmx_rec_history")
            if n not in dir(lib)]:
        real = getattr(lib, name)

        def call(*args, _real=real, _name=name):
            status = _real(*args)
            if status == 0:
                log.append(_describe(_name, args))
            return status
        setattr(lib, name, call)
    return log


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--size", nargs=2, type=int, default=(768, 1280))
    parser.add_argument("--runs", type=int, default=3)
    parser.add_argument("--warm", type=int, default=1, metavar="N",
                        help="frames run before the timed ones, so the GPU's clock has "
                             "settled (the first also records)")
    parser.add_argument("--tables", action="store_true",
                        help="print the kind tables read from the shaders and stop; `make test` "
                             "runs this, so a kind two shaders share fails there, not at the "
                             "next profile")
    parser.add_argument("--calls", type=int, default=0, metavar="N",
                        help="also list the N most expensive call sites")
    args = parser.parse_args()
    if args.tables:
        for family, table in (("unary", UNARY), ("row", ROW)):
            print(family + ": " + ", ".join(f"{k} {v}" for k, v in sorted(table.items())))
        return
    height, width = args.size

    model = nr_model.NeuralRenderingModel.from_safetensors(nr_model.default_weights())
    runtime = xmxres.Runtime()
    xmxres.profile(True)
    frame = F.ResidentFrame(runtime, model.weights, height, width)
    features = (np.random.default_rng(11).standard_normal((height, width, 16))
                * 0.3).astype(np.float32)

    for _ in range(max(1, args.warm)):              # record, and warm
        frame.run(features, execution="single")
    log = record_calls(runtime.lib) if args.calls else None
    xmxres.profile_reset()
    started = time.perf_counter()
    for _ in range(args.runs):
        frame.run(features, execution="single")
    wall = (time.perf_counter() - started) / args.runs

    totals = xmxres.profile_totals()
    rows = sorted(((ms / args.runs, n / args.runs, label(f, s))
                   for (f, s), (ms, n) in totals.items()), reverse=True)
    device = sum(r[0] for r in rows)
    print("%dx%d, %d runs\n" % (width, height, args.runs))
    print("  %-28s %9s %8s %9s %7s" % ("pass", "ms", "share", "passes", "us each"))
    for ms, passes, name in rows:
        print("  %-28s %9.2f %7.1f%% %9.0f %7.1f"
              % (name, ms, 100 * ms / device, passes, 1000 * ms / max(passes, 1)))
    print("  %-28s %9.2f %7.1f%%" % ("— device total", device, 100.0))
    print("  %-28s %9.2f" % ("— wall per frame", wall * 1e3))
    print("  %-28s %9.2f  (host: recording, numpy, the rest)"
          % ("— host overhead", wall * 1e3 - device))
    gemm = sum(ms for ms, _, name in rows if name.startswith("gemm"))
    print("\n  GEMM %.1f ms of %.1f (%.0f%%); everything else %.1f ms (%.0f%%)"
          % (gemm, device, 100 * gemm / device, device - gemm, 100 * (device - gemm) / device))
    if log is not None:
        each = xmxres.profile_each()
        if len(each) != len(log):
            raise SystemExit("%d passes timed against %d recorded: the labels would not line "
                             "up, so none are printed" % (len(each), len(log)))
        # which kernel ran each GEMM: routing is by shape, and a label from the recording
        # side cannot say whether the tiled, the staged or the 8x16 kernel took it
        kinds = xmxres.profile_each_kinds()
        sites = {}
        for name, ms, kind in zip(log, each, kinds):
            family = xmxres.PROFILE_FAMILIES[kind // 32]
            if family.startswith("gemm") and not name.startswith("ffn"):
                name = "%s [%s]" % (name, family.replace("gemm", "").strip() or "8x16")
            total, count = sites.get(name, (0.0, 0))
            sites[name] = (total + max(ms, 0.0), count + 1)
        print("\n  %-58s %8s %6s %8s" % ("call site", "ms", "calls", "ms each"))
        for name, (total, count) in sorted(sites.items(), key=lambda s: -s[1][0])[:args.calls]:
            print("  %-58s %8.2f %6.0f %8.3f"
                  % (name, total / args.runs, count / args.runs, total / count))


if __name__ == "__main__":
    main()
