#!/usr/bin/env python3
"""
pe_resources — walk the PE resource tree and report every resource entry.

Phase 3 needs the real layout of the .rsrc weight blob before any E4M3->BF16
dequantiser can be written. `.rsrc` entropy of 5.89 says it is not a flat dense
FP8 array, so start by asking the resource directory what is actually in there.

Usage: pe_resources.py <file.dll> [--dump <type>/<name> <outfile>]
"""
import struct
import sys

RT = {1: "CURSOR", 2: "BITMAP", 3: "ICON", 4: "MENU", 5: "DIALOG", 6: "STRING",
      7: "FONTDIR", 8: "FONT", 9: "ACCELERATOR", 10: "RCDATA", 11: "MESSAGETABLE",
      12: "GROUP_CURSOR", 14: "GROUP_ICON", 16: "VERSION", 17: "DLGINCLUDE",
      19: "PLUGPLAY", 20: "VXD", 21: "ANICURSOR", 22: "ANIICON", 23: "HTML",
      24: "MANIFEST"}


def load(path):
    f = open(path, "rb")
    blob = f.read()
    e_lfanew = struct.unpack_from("<I", blob, 0x3C)[0]
    nsec, = struct.unpack_from("<H", blob, e_lfanew + 6)
    opt_size, = struct.unpack_from("<H", blob, e_lfanew + 20)
    opt_off = e_lfanew + 24
    magic, = struct.unpack_from("<H", blob, opt_off)
    dd_off = opt_off + (112 if magic == 0x20B else 96)
    rsrc_rva, rsrc_size = struct.unpack_from("<II", blob, dd_off + 2 * 8)
    secs = []
    so = opt_off + opt_size
    for i in range(nsec):
        raw = blob[so + i * 40: so + i * 40 + 40]
        name = raw[:8].rstrip(b"\0").decode("latin1")
        vsize, vaddr, rsize, raddr = struct.unpack_from("<IIII", raw, 8)
        secs.append((name, vaddr, vsize, raddr, rsize))
    return blob, secs, rsrc_rva, rsrc_size


def rva2off(secs, rva):
    for name, vaddr, vsize, raddr, rsize in secs:
        if vaddr <= rva < vaddr + max(vsize, rsize):
            return raddr + (rva - vaddr)
    return None


def name_of(blob, base, val):
    if val & 0x80000000:
        off = base + (val & 0x7FFFFFFF)
        ln, = struct.unpack_from("<H", blob, off)
        return blob[off + 2: off + 2 + ln * 2].decode("utf-16-le", "replace")
    return val


def walk(blob, secs, base, off, path, out, depth=0):
    nnamed, nid = struct.unpack_from("<HH", blob, off + 12)
    ent = off + 16
    for i in range(nnamed + nid):
        nm, data = struct.unpack_from("<II", blob, ent + i * 8)
        label = name_of(blob, base, nm)
        if depth == 0 and isinstance(label, int):
            label = RT.get(label, "TYPE_%d" % label)
        if data & 0x80000000:
            walk(blob, secs, base, base + (data & 0x7FFFFFFF), path + [label], out, depth + 1)
        else:
            drva, dsize, cp, _ = struct.unpack_from("<IIII", blob, base + data)
            out.append((path + [label], rva2off(secs, drva), dsize, cp))


def main():
    path = sys.argv[1]
    blob, secs, rsrc_rva, rsrc_size = load(path)
    base = rva2off(secs, rsrc_rva)
    print("resource directory at RVA 0x%x -> file 0x%x, size %d" % (rsrc_rva, base, rsrc_size))
    out = []
    walk(blob, secs, base, base, [], out)
    out.sort(key=lambda e: -e[2])
    print("\n%d resource entries, %s bytes total\n" % (len(out), format(sum(e[2] for e in out), ",")))
    print("  %-40s %14s %14s  %s" % ("path", "file offset", "size", "first 16 bytes"))
    for p, off, size, cp in out[:40]:
        head = blob[off:off + 16].hex() if off else ""
        print("  %-40s 0x%012x %14s  %s" % ("/".join(str(x) for x in p), off or 0, format(size, ","), head))

    if "--dump" in sys.argv:
        want = sys.argv[sys.argv.index("--dump") + 1]
        dest = sys.argv[sys.argv.index("--dump") + 2]
        for p, off, size, cp in out:
            if "/".join(str(x) for x in p).startswith(want):
                open(dest, "wb").write(blob[off:off + size])
                print("\ndumped %s (%d bytes) -> %s" % ("/".join(str(x) for x in p), size, dest))
                break


main()
