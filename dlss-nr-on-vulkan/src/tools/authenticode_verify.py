#!/usr/bin/env python3
"""
authenticode_verify — verify an Authenticode PKCS#7 blob without osslsigncode.

`openssl cms -verify` cannot parse Authenticode: its eContent is an
SpcIndirectDataContent SEQUENCE, not the OCTET STRING that CMS expects. So walk the
DER by hand and check the two things that actually matter:

  1. the messageDigest signed-attribute equals SHA-256 of the eContent, and
  2. the RSA signature verifies over the DER of the signed attributes.

Together with a PE-hash comparison (see pe_inspect.py) and a chain check
(openssl verify), that is a complete Authenticode verification.

Usage: authenticode_verify.py <sig.der> <signer-cert.pem>
"""
import hashlib
import subprocess
import sys
import tempfile

OID_MESSAGE_DIGEST = bytes.fromhex("2a864886f70d010904")  # 1.2.840.113549.1.9.4


def tlv(buf, off):
    """Return (tag, value_offset, length, next_offset) for one DER element."""
    tag = buf[off]
    i = off + 1
    n = buf[i]
    i += 1
    if n & 0x80:
        k = n & 0x7F
        n = int.from_bytes(buf[i:i + k], "big")
        i += k
    return tag, i, n, i + n


def children(buf, off):
    tag, vo, ln, nxt = tlv(buf, off)
    end = vo + ln
    i = vo
    out = []
    while i < end:
        t, v, l, nx = tlv(buf, i)
        out.append((t, i, v, l))
        i = nx
    return out


def main():
    sig = open(sys.argv[1], "rb").read()
    cert_pem = sys.argv[2]

    # ContentInfo ::= SEQUENCE { contentType OID, content [0] EXPLICIT SignedData }
    ci = children(sig, 0)
    content0 = [c for c in ci if c[0] == 0xA0][0]
    signed_data_off = children(sig, content0[1])[0][1]

    sd = children(sig, signed_data_off)
    # version, digestAlgorithms, encapContentInfo, [certs], [crls], signerInfos
    encap_off = sd[2][1]
    encap = children(sig, encap_off)
    econtent_wrap = [c for c in encap if c[0] == 0xA0][0]
    # inner element of the [0] EXPLICIT wrapper = the SpcIndirectDataContent SEQUENCE
    inner_tag, inner_off, inner_len, inner_end = tlv(sig, econtent_wrap[2])
    econtent_der = sig[econtent_wrap[2]:inner_end]

    signer_infos = [c for c in sd if c[0] == 0x31][-1]
    si_off = children(sig, signer_infos[1])[0][1]
    si = children(sig, si_off)

    signed_attrs = [c for c in si if c[0] == 0xA0][0]
    _t, sa_vo, sa_len, sa_end = tlv(sig, signed_attrs[1])
    # signature is the last OCTET STRING in the SignerInfo
    sig_oct = [c for c in si if c[0] == 0x04][-1]
    signature = sig[sig_oct[2]:sig_oct[2] + sig_oct[3]]

    print("== structure ==")
    print("  eContent (SpcIndirectDataContent) : %d bytes" % len(econtent_der))
    print("  signedAttrs                       : %d bytes" % sa_len)
    print("  RSA signature                     : %d bytes (%d bit)" % (len(signature), len(signature) * 8))

    # --- check 1: messageDigest attribute == SHA-256(eContent) ---
    want = hashlib.sha256(econtent_der).hexdigest()
    # Authenticode hashes the SpcIndirectDataContent *contents*, without the
    # SEQUENCE header; keep both so a mismatch can be told from a parsing slip.
    want_inner = hashlib.sha256(sig[inner_off:inner_end]).hexdigest()
    found = None
    for attr in children(sig, signed_attrs[1]):
        kids = children(sig, attr[1])
        oid_t, oid_off, oid_len = kids[0][0], kids[0][2], kids[0][3]
        if sig[oid_off:oid_off + oid_len] == OID_MESSAGE_DIGEST:
            val = children(sig, kids[1][1])[0]
            found = sig[val[2]:val[2] + val[3]].hex()
    print("\n== check 1: messageDigest attribute vs SHA-256(eContent) ==")
    print("  attribute            : %s" % found)
    print("  sha256(full TLV)     : %s" % want)
    print("  sha256(contents only): %s" % want_inner)
    ok1 = found in (want, want_inner)
    which = "full TLV" if found == want else ("contents only" if found == want_inner else "-")
    print("  -> %s (%s)" % ("MATCH" if ok1 else "MISMATCH", which))

    # --- check 2: RSA signature over DER(signedAttrs), re-tagged [0] IMPLICIT -> SET ---
    attrs_der = bytearray(sig[signed_attrs[1]:sa_end])
    attrs_der[0] = 0x31  # SET OF, as required when signing
    with tempfile.TemporaryDirectory() as d:
        open(d + "/attrs.der", "wb").write(bytes(attrs_der))
        open(d + "/sig.bin", "wb").write(signature)
        subprocess.run(["openssl", "x509", "-in", cert_pem, "-pubkey", "-noout",
                        "-out", d + "/pub.pem"], check=True)
        r = subprocess.run(["openssl", "dgst", "-sha256", "-verify", d + "/pub.pem",
                            "-signature", d + "/sig.bin", d + "/attrs.der"],
                           capture_output=True, text=True)
    print("\n== check 2: RSA signature over signedAttrs ==")
    print("  openssl: %s" % (r.stdout.strip() or r.stderr.strip()))
    ok2 = "Verified OK" in r.stdout
    print("  -> %s" % ("VALID" if ok2 else "INVALID"))

    print("\n== verdict ==")
    print("  %s" % ("BOTH CHECKS PASS" if (ok1 and ok2) else "FAILED"))
    return 0 if (ok1 and ok2) else 1


sys.exit(main())
