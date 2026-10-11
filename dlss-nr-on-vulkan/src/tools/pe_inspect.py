#!/usr/bin/env python3
"""
pe_inspect — section map, entropy, version resource and Authenticode digest for a PE.

Stands in for `rabin2 -S` so the project does not need radare2, and additionally
recomputes the Authenticode PE hash so a downloaded binary can be checked against
the digest the publisher signed.

Usage: pe_inspect.py <file.dll> [--sig-out sig.der]
"""
import hashlib
import math
import struct
import sys

IMAGE_DIRECTORY_ENTRY_SECURITY = 4


def entropy(data):
    if not data:
        return 0.0
    n = len(data)
    h = 0.0
    for b in range(256):
        c = data.count(b)
        if c:
            p = c / n
            h -= p * math.log2(p)
    return h


def sample(f, off, size, budget=8 << 20, chunks=32):
    """Evenly spread sample, so entropy on a 140 MB section stays cheap."""
    if size <= budget:
        f.seek(off)
        return f.read(size)
    per = budget // chunks
    out = bytearray()
    for i in range(chunks):
        f.seek(off + (size - per) * i // (chunks - 1))
        out += f.read(per)
    return bytes(out)


def main():
    path = sys.argv[1]
    sig_out = None
    if "--sig-out" in sys.argv:
        sig_out = sys.argv[sys.argv.index("--sig-out") + 1]

    f = open(path, "rb")
    blob = f.read(4096)
    if blob[:2] != b"MZ":
        sys.exit("not a PE: no MZ")
    e_lfanew = struct.unpack_from("<I", blob, 0x3C)[0]
    f.seek(e_lfanew)
    pe = f.read(24)
    if pe[:4] != b"PE\0\0":
        sys.exit("not a PE: no PE signature")
    nsec, tds, _, _, opt_size, chars = struct.unpack_from("<HIIIHH", pe, 6)
    opt_off = e_lfanew + 24
    f.seek(opt_off)
    opt = f.read(opt_size)
    magic = struct.unpack_from("<H", opt, 0)[0]
    pe32plus = magic == 0x20B

    checksum_off = opt_off + 64
    dd_off = opt_off + (112 if pe32plus else 96)
    nrva = struct.unpack_from("<I", opt, (108 if pe32plus else 92))[0]
    certdir_off = dd_off + IMAGE_DIRECTORY_ENTRY_SECURITY * 8

    f.seek(certdir_off)
    cert_rva, cert_size = struct.unpack("<II", f.read(8))

    import datetime
    print("== header ==")
    print("  magic            : 0x%03x (%s)" % (magic, "PE32+" if pe32plus else "PE32"))
    print("  sections         : %d" % nsec)
    print("  timestamp        : %d (%s UTC)" % (
        tds, datetime.datetime.fromtimestamp(tds, datetime.timezone.utc).strftime("%Y-%m-%d %H:%M:%S")))
    print("  characteristics  : 0x%04x" % chars)
    print("  data directories : %d" % nrva)
    print("  cert table       : offset=0x%x size=%d (%.1f KiB)" % (cert_rva, cert_size, cert_size / 1024))

    # --- sections ---
    sec_off = opt_off + opt_size
    f.seek(sec_off)
    secs = []
    for _ in range(nsec):
        raw = f.read(40)
        name = raw[:8].rstrip(b"\0").decode("latin1")
        vsize, vaddr, rsize, raddr = struct.unpack_from("<IIII", raw, 8)
        flags = struct.unpack_from("<I", raw, 36)[0]
        secs.append((name, vaddr, vsize, raddr, rsize, flags))

    import os
    total = os.path.getsize(path)
    print("\n== sections ==")
    print("  %-10s %10s %12s %12s %12s  %-9s %s" %
          ("name", "vaddr", "vsize", "raw off", "raw size", "flags", "entropy(sampled)"))
    for name, vaddr, vsize, raddr, rsize, flags in secs:
        e = entropy(sample(f, raddr, rsize)) if rsize else 0.0
        print("  %-10s 0x%08x %12d 0x%010x %12d  0x%08x  %.4f  (%5.1f%% of file)" %
              (name, vaddr, vsize, raddr, rsize, flags, e, 100.0 * rsize / total))

    # --- version resource ---
    f.seek(0)
    whole = f.read()
    # Anchor on the UTF-16 "VS_VERSION_INFO" key: the raw FEEF04BD signature
    # also occurs by chance inside large high-entropy resource data.
    anchor = whole.find("VS_VERSION_INFO".encode("utf-16-le"))
    vi = whole.find(b"\xbd\x04\xef\xfe", anchor) if anchor >= 0 else -1
    if vi >= 0:
        (_sig, _sv, fvms, fvls, pvms, pvls) = struct.unpack_from("<IIIIII", whole, vi)
        fv = (fvms >> 16, fvms & 0xFFFF, fvls >> 16, fvls & 0xFFFF)
        pv = (pvms >> 16, pvms & 0xFFFF, pvls >> 16, pvls & 0xFFFF)
        print("\n== version resource ==")
        print("  FileVersion    : %d.%d.%d.%d" % fv)
        print("  ProductVersion : %d.%d.%d.%d" % pv)

    # --- Authenticode PE hash ---
    if cert_size:
        cert_start = cert_rva  # this directory entry stores a file offset, not an RVA
        for algo in ("sha256", "sha1"):
            h = hashlib.new(algo)
            h.update(whole[:checksum_off])
            h.update(whole[checksum_off + 4:certdir_off])
            h.update(whole[certdir_off + 8:cert_start])
            print("  authenticode %-6s: %s" % (algo, h.hexdigest()))
        trailing = total - (cert_start + cert_size)
        print("  bytes after cert table: %d %s" % (trailing, "(clean)" if trailing == 0 else "(!! extra data)"))
        if sig_out:
            # WIN_CERTIFICATE: dwLength, wRevision, wCertificateType, then PKCS#7
            dwlen, rev, ctype = struct.unpack_from("<IHH", whole, cert_start)
            open(sig_out, "wb").write(whole[cert_start + 8: cert_start + dwlen])
            print("  WIN_CERTIFICATE: len=%d revision=0x%04x type=0x%04x -> %s"
                  % (dwlen, rev, ctype, sig_out))
    else:
        print("\n!! NO CERTIFICATE TABLE — binary is unsigned or the signature was stripped")


main()
