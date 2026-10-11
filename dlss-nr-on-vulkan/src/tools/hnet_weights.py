#!/usr/bin/env python3
"""
hnet_weights — reader for the HNet WEIGHTS_HT container in nvngx_dlssnr.dll.

Container format (recovered 2026-09-07, walks the blob to EOF exactly):

    u64                total_size            == len(blob)
    repeat until EOF:
        u64            name_len
        char           name[name_len]        e.g. "block24.layer2.layer"
        u64            rec_len               bytes from here+8 to the record end
        u64            rec_len               (repeated)
        u64            data_len              == 2 * n_elem
        u32            dtype                 always 1
        u8             data[data_len]        little-endian FP16
        u64            0
        u64            1                     ndim
        u32            n_elem                shape[0]

The elements are **FP16**, not FP8 E4M3 — see notes/phase3-weight-format.md.

Usage:
    hnet_weights.py <weights_ht.bin> [--list] [--get <name> <out.f32>]
"""
import struct
import sys


def parse(blob):
    total = struct.unpack_from("<Q", blob, 0)[0]
    if total != len(blob):
        raise ValueError("header size %d != blob size %d" % (total, len(blob)))
    pos = 8
    out = []
    while pos < len(blob):
        name_len = struct.unpack_from("<Q", blob, pos)[0]
        name = blob[pos + 8: pos + 8 + name_len].decode("ascii")
        p = pos + 8 + name_len
        rec_len = struct.unpack_from("<Q", blob, p)[0]
        end = p + 8 + rec_len
        data_len = struct.unpack_from("<Q", blob, p + 16)[0]
        dtype = struct.unpack_from("<I", blob, p + 24)[0]
        data_off = p + 28
        ndim = struct.unpack_from("<Q", blob, data_off + data_len + 8)[0]
        n_elem = struct.unpack_from("<I", blob, data_off + data_len + 16)[0]
        if data_len != 2 * n_elem:
            raise ValueError("%s: data_len %d != 2*n_elem %d" % (name, data_len, n_elem))
        out.append(dict(name=name, offset=data_off, nbytes=data_len,
                        n_elem=n_elem, dtype=dtype, ndim=ndim))
        pos = end
    if pos != len(blob):
        raise ValueError("walk ended at %d, expected %d" % (pos, len(blob)))
    return out


def to_f32(blob, t):
    """Widen one tensor from stored FP16 to a list of Python floats."""
    o = t["offset"]
    return [struct.unpack_from("<e", blob, o + 2 * i)[0] for i in range(t["n_elem"])]


def main():
    blob = open(sys.argv[1], "rb").read()
    tensors = parse(blob)
    if "--get" in sys.argv:
        i = sys.argv.index("--get")
        want, dest = sys.argv[i + 1], sys.argv[i + 2]
        t = next(x for x in tensors if x["name"] == want)
        with open(dest, "wb") as f:
            for v in to_f32(blob, t):
                f.write(struct.pack("<f", v))
        print("%s: %d elements -> %s (float32)" % (want, t["n_elem"], dest))
        return
    print("%d tensors, %s elements, %s bytes of data"
          % (len(tensors), format(sum(t["n_elem"] for t in tensors), ","),
             format(sum(t["nbytes"] for t in tensors), ",")))
    if "--list" in sys.argv:
        for t in tensors:
            print("  %-34s %12s elem  @0x%09x" % (t["name"], format(t["n_elem"], ","), t["offset"]))


if __name__ == "__main__":
    main()
