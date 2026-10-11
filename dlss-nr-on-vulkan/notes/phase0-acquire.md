# Phase 0 — acquiring `nvngx_dlssnr.dll`

## Status: BLOCKED. The DLL is not in the driver package.

notes/CLAUDE.md Phase 0 says "Extract `nvngx_dlssnr.dll` from the driver package".
That premise is **wrong**, verified 2026-09-07.

## What was done

Source file supplied by the owner:

```
616.64-desktop-win10-win11-64bit-international-dch-whql.exe   984 282 848 bytes
sha256 36584e5df1dc048df5c677e9591295b96b870e7a6a0ad1461457b89de8bc1b37
```

Confirmed genuine 616.64: `setup.cfg` carries `version="616.64"`, payload files are
dated 2026-08-25…27, and NGX binaries carry the build path
`C:/dvs/p4/build/sw/rel/gpu_drv/r615/r616_41/drivers/ngx/...`.

No `7z` on this machine. `bsdtar` (libarchive 3.8.9) **can** read the archive, but
running it against the SFX `.exe` directly lists only the 63 directory entries and
silently drops every file, with exit status 0 — libarchive resolves the 7z internal
offsets against base 0 instead of the payload base, so only streamless entries
survive. Fix: locate the `37 7A BC AF 27 1C` signature (offset `0x10ca24`
= 1 100 324) and `dd` the payload out into `work/driver-payload.7z`. bsdtar then
lists all **1267 entries / 3.61 GiB** cleanly.

## Finding

There is no `nvngx_dlssnr.dll` in the package. The complete set of NGX modules is:

```
Display.Driver/nvngx.dll          488 824
Display.Driver/_nvngx.dll       1 428 200
Display.Driver/nvngx_update.exe 1 045 736
Display.Driver/nvngx_dlssg.dll            (frame generation)
NvDLISR/nvngx_dlisr.dll        58 958 448
```

Note that `nvngx_dlss.dll` (super resolution) and `nvngx_dlssd.dll` (ray
reconstruction) are absent too. Nothing in the package is near the reported
~158 MB; the largest Display.Driver file is `nvdxdlkernels.dll` at 174 MB.

## But the feature does exist in this driver

`_nvngx.dll` — the NGX core — knows about DLSSNR by name:

```
DLSSNR
DLSSNR.Available
DLSSNR.FeatureInitResult
DLSSNR.Width
DLSSNR.Height
DLSSNR.MinDriverVersionMajor
DLSSNR.MinDriverVersionMinor
DLSSNR.NeedsUpdatedDriver
Sending Telemetry: DLSSNR Eval Data
```

plus a UTF-16 `dlssnr` string (the module-name lookup NGX uses to load a feature DLL).

`nvngx_update.exe` lists `dlssnr` alongside `dlssd`, `dlssg` and `dlss_override` as
OTA-updatable module keys, and `_nvngx.dll` contains the units
`nvngx_ota_updates_config` / `nvngx_ota_updates_log`; `nvngx_update.exe` contains
`nvngx_server_config`, `nvngx_mapping`, `nvngx_deny_list`.

**Conclusion (verified):** driver 616.64 ships the NGX *runtime* that can request and
gate a DLSSNR feature, but the DLSSNR feature DLL itself is delivered out of band —
the same way `nvngx_dlss.dll` / `nvngx_dlssd.dll` are. So the DLL comes either
(a) over the air via `nvngx_update.exe`, or (b) bundled with the game that uses it.

No OTA endpoint appears in plaintext in either binary — the only URLs present are
DigiCert/Microsoft certificate CRL and OCSP paths. The endpoint is built at runtime
or stored non-obviously; recovering it would be its own reversing task.

## Where a copy comes from is out of scope, deliberately

This note once listed places to look, named a community mirror and said which of its
tags to take. That is a sourcing guide for a pre-release binary and it is not this
project's business, so it is gone: the repository ships no NVIDIA binary and does not
help anyone find one. Whether you are entitled to the copy you have is between you and
whoever gave it to you.

What is worth keeping is the other half — how to tell whether the copy in your hands is
NVIDIA's original bytes rather than something patched. See "Verifying whatever arrives".

---

## Update — the public NVIDIA SDK does not have it either (verified 2026-09-07)

Queried the GitHub API directly rather than trusting search results:

- `NVIDIA/DLSS` — latest release **v310.7.0, published 2026-06-23**, i.e. *before*
  DLSS 5 shipped on 2026-09-03. Single branch `main`, last commit 2026-06-23.
- `lib/Windows_x86_64/rel/` holds exactly three DLLs:
  `nvngx_dlss.dll` (58 977 904), `nvngx_dlssd.dll` (40 946 800),
  `nvngx_dlssg.dll` (7 519 856). **No `nvngx_dlssnr.dll`.**

So there is no official public copy at all: not in the driver package, not in the SDK.

**This falsifies the second half of the "target binary" note in notes/CLAUDE.md.** That note
said to *prefer the official driver copy over the leaked NBA 2K27 early-access build
310.8.0.0*. There is no official copy to prefer. The 310.8.0.0 build is not merely the
worse option — as of today it is the only one that exists.

*Reported, not verified* (web search, community sources): the DLL identifies as
NVIDIA DLSSNR **310.8.0.0**, ~158 MB, originating from the NBA 2K27 early-access PC
build, and circulates through community channels (RenoDX Discord, assorted GitHub
mirrors). It reportedly refuses to initialise below RTX 50 — irrelevant to us, since
we never execute NVIDIA's kernels, and there is no NVIDIA GPU here regardless.
Community tooling refers to DLSS-NR as **NGX feature 18**; worth checking against
`_nvngx.dll`'s feature-ID table once we have a reason to.

Per the project rule — *every existing project in this space requires the user to
supply the DLL themselves, and this one does the same* — acquisition stays with the
owner. Nothing was downloaded.

---

## Verifying whatever arrives

Anything repackaged to defeat NVIDIA's RTX-50 initialisation gate has had its
Authenticode signature broken, which makes its authenticity unverifiable — and this
project never executes their code, so the gate is irrelevant to it anyway. Run these
before a copy goes anywhere near `ref/`:

1. size ≈ 158 MB, `file` reports PE32+ DLL x86-64;
2. VERSIONINFO says NVIDIA DLSSNR **310.8.0.0**;
3. **Authenticode signature present and signed by NVIDIA Corporation**, with the
   embedded digest matching the recomputed PE hash — this is the check that makes the
   mirror's trustworthiness moot. Needs `osslsigncode`, or a PE cert-table parse plus
   `openssl pkcs7`;
4. section map + entropy: expect one ~140 MB high-but-not-maximal-entropy section
   (the weight blob). That doubles as the start of Phase 1.

### Status

Acquisition is with the owner, and stays there.
