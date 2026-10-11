#!/usr/bin/env python3
"""Run a command with every graph shader declaring what it does with float16 subnormals.

Left to itself each driver does its own: Mesa flushes float16 subnormals to zero in the
cooperative-matrix GEMMs and Intel's Windows driver keeps them, and that alone is where the
two first part (notes/phase71). libxmx now declares `DenormPreserve 16` on every module it
loads, where the driver allows it; a module that declares a 16-bit mode of its own keeps it.
This copies the SPIR-V the graph loads with `DenormPreserve` or `DenormFlushToZero` declared,
for 16-bit floats unless `--widths` says otherwise, into a folder of its own, points the
`XMX_*_SPV` variables at the copies, and runs the command under them:

    python3 src/bench/denorm_mode.py preserve -- python3 src/bench/frame_replay.py --size 320 320
    python3 src/bench/denorm_mode.py flush -- python3 src/bench/capture_compare.py --save x.npz

The copies go to `work/denorm-<mode>-<widths>/`; the shaders in `work/` are not touched.
A driver must report `shaderDenormPreserveFloat16` (or `...FlushToZeroFloat16`) for the
mode to be valid; ANV reports preserve only, Intel's Windows driver both.
"""
import argparse
import os
import pathlib
import struct
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]

# What the resident graph loads, by the variable that overrides each path (xmxres.py).
SHADERS = {
    "XMX_GEMM_SPV": "gemm_resident.spv", "XMX_UNARY_SPV": "resident.spv",
    "XMX_ROW_SPV": "attention.spv", "XMX_HISTORY_SPV": "history.spv",
    "XMX_TILED_SPV": "gemm_tiled.spv", "XMX_STAGED_SPV": "gemm_staged.spv",
    "XMX_STAGED32_SPV": "gemm_staged32.spv", "XMX_STAGED32_DEEP_SPV": "gemm_staged32_deep.spv",
    "XMX_ROWS_SPV": "attention_rows.spv", "XMX_WINDOW_BLOCK_SPV": "window_block.spv",
    "XMX_GLOBAL_ATTENTION_SPV": "global_attention.spv", "XMX_INT8_SPV": "gemm_staged_int8.spv",
    "XMX_FFN_SPV": "ffn_fused.spv", "XMX_WINDOW_SPV": "window_attention.spv",
}
# (execution mode, capability): SPIR-V 1.4 made float controls core, so nothing else goes in
MODES = {"preserve": (4459, 4464), "flush": (4460, 4465)}
OP_CAPABILITY, OP_ENTRY_POINT, OP_EXECUTION_MODE, OP_EXECUTION_MODE_ID = 17, 15, 16, 331


def declare(words, mode, capability, widths):
    """The module with `mode` declared for each width on every entry point."""
    header, body = words[:5], words[5:]
    if header[0] != 0x07230203:
        raise ValueError("not little-endian SPIR-V")
    if header[1] < 0x00010400:
        raise ValueError("SPIR-V before 1.4 needs SPV_KHR_float_controls, which this does not add")
    insts, i = [], 0
    while i < len(body):
        count = body[i] >> 16
        insts.append(body[i:i + count])
        i += count
    opcode = lambda inst: inst[0] & 0xFFFF
    for inst in insts:
        if opcode(inst) == OP_EXECUTION_MODE and inst[2] in (4459, 4460) and inst[3] in widths:
            raise ValueError(f"already declares a denorm mode for {inst[3]}-bit floats")
    entries = [inst[2] for inst in insts if opcode(inst) == OP_ENTRY_POINT]
    last_capability = max(n for n, inst in enumerate(insts) if opcode(inst) == OP_CAPABILITY)
    last_mode = max(n for n, inst in enumerate(insts)
                    if opcode(inst) in (OP_ENTRY_POINT, OP_EXECUTION_MODE, OP_EXECUTION_MODE_ID))
    out = []
    for n, inst in enumerate(insts):
        out.append(inst)
        if n == last_capability:
            out.append([(2 << 16) | OP_CAPABILITY, capability])
        if n == last_mode:
            out += [[(4 << 16) | OP_EXECUTION_MODE, entry, mode, width]
                    for entry in entries for width in widths]
    return header + [word for inst in out for word in inst]


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("mode", choices=MODES)
    parser.add_argument("--widths", default="16", help="comma-separated: 16, 32, 64")
    parser.add_argument("command", nargs=argparse.REMAINDER,
                        help="after --: the command to run under the declared mode")
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command:
        parser.error("give the command to run after --")
    widths = tuple(int(width) for width in args.widths.split(","))
    mode, capability = MODES[args.mode]
    folder = ROOT / "work" / f"denorm-{args.mode}-{'-'.join(map(str, widths))}"
    folder.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    for variable, name in SHADERS.items():
        source = pathlib.Path(os.environ.get(variable) or ROOT / "work" / name)
        if not source.exists():
            print(f"  {name}: not built, left alone", file=sys.stderr)
            continue
        data = source.read_bytes()
        words = list(struct.unpack(f"<{len(data) // 4}I", data))
        patched = declare(words, mode, capability, widths)
        (folder / name).write_bytes(struct.pack(f"<{len(patched)}I", *patched))
        env[variable] = str(folder / name)
    print(f"  {args.mode} {'/'.join(map(str, widths))}-bit subnormals: {folder}", file=sys.stderr)
    sys.exit(subprocess.run(command, env=env).returncode)


if __name__ == "__main__":
    main()
