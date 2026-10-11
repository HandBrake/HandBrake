#!/usr/bin/env python3
"""
extract_modules — pull the runtime kernel modules out of nvngx_dlssnr.dll.

The 15 containers in `.data` are marked with fatbin magic 0xBA55ED50 but their
payload is a plain **Zstandard** frame, not NVIDIA's proprietary compression.
Decompressed, each is either PTX source or an ELF cubin.

Method credit: skchen17/dlssnr-amd-lab, scripts/extract_runtime_modules.py.
Independently reimplemented here against our own DLL; nothing is redistributed.

Usage: extract_modules.py <dll> <outdir>
"""
import hashlib
import json
import re
import struct
import sys
from compression import zstd
from pathlib import Path

MODULE_MAGIC = 0xBA55ED50
ZSTD_MAGIC = b"\x28\xb5\x2f\xfd"
ELF_MAGIC = b"\x7fELF"


def elf_extent(data, off):
    phoff, shoff = struct.unpack_from("<QQ", data, off + 32)
    phentsize, phnum = struct.unpack_from("<HH", data, off + 54)
    shentsize, shnum = struct.unpack_from("<HH", data, off + 58)
    end = max(phoff + phentsize * phnum, shoff + shentsize * shnum)
    for i in range(shnum):
        b = off + shoff + i * shentsize
        stype, = struct.unpack_from("<I", data, b + 4)
        soff, ssize = struct.unpack_from("<QQ", data, b + 24)
        if stype != 8:
            end = max(end, soff + ssize)
    return end


def main():
    dll = Path(sys.argv[1]).read_bytes()
    out = Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    mag = struct.pack("<I", MODULE_MAGIC)
    recs, cur, idx = [], 0, 0
    while True:
        off = dll.find(mag, cur)
        if off < 0:
            break
        cur = off + 4
        if off + 16 > len(dll):
            continue
        size = struct.unpack_from("<Q", dll, off + 8)[0] + 16
        if not (16 < size <= len(dll) - off):
            continue
        blob = dll[off:off + size]
        z = blob.find(ZSTD_MAGIC)
        if z < 0:
            continue
        # One frame per container, followed by container padding: the one-shot
        # decompress() treats that tail as another frame and fails. Stream instead.
        dec = zstd.ZstdDecompressor()
        raw = dec.decompress(blob[z:])
        head = raw[:4096].decode("ascii", "ignore")
        ver = re.search(r"(?m)^\s*\.version\s+(\S+)", head)
        tgt = re.search(r"(?m)^\s*\.target\s+([^\s,]+)", head)
        r = dict(index=idx, dll_offset="0x%X" % off, container_size=size,
                 decompressed_size=len(raw),
                 sha256=hashlib.sha256(raw).hexdigest())
        if ver and tgt:
            name = "module_%02d_%08X.ptx" % (idx, off)
            (out / name).write_bytes(raw)
            entries = re.findall(r"(?m)^\s*(?:\.visible\s+)?\.entry\s+([^\s(]+)",
                                 raw.decode("utf-8", "replace"))
            r.update(kind="PTX", ptx_version=ver.group(1), ptx_target=tgt.group(1),
                     entry_count=len(entries), file=name)
        else:
            e = raw.find(ELF_MAGIC)
            if e < 0:
                r.update(kind="UNKNOWN")
            else:
                elf = raw[e:e + elf_extent(raw, e)]
                name = "module_%02d_%08X.elf" % (idx, off)
                (out / name).write_bytes(elf)
                r.update(kind="ELF", elf_machine=struct.unpack_from("<H", elf, 18)[0],
                         elf_size=len(elf), file=name)
        recs.append(r)
        idx += 1
    (out / "modules.json").write_text(json.dumps(
        dict(schema=1, source_sha256=hashlib.sha256(dll).hexdigest(),
             module_count=len(recs), modules=recs), indent=2) + "\n")
    for r in recs:
        print("  [%02d] @%-11s %-6s %-12s decompressed=%-12s %s"
              % (r["index"], r["dll_offset"], r["kind"],
                 r.get("ptx_target", "machine=%s" % r.get("elf_machine", "?")),
                 format(r["decompressed_size"], ","),
                 "entries=%d" % r["entry_count"] if r["kind"] == "PTX" else ""))
    print("\n%d modules -> %s" % (len(recs), out))


main()
