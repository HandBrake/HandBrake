# Phase 1 — the binary, VERIFIED

Target: `ref/nvngx_dlssnr.dll` (read-only, 0444).

```
size    165 840 496 B (158.2 MiB)
sha256  e16bcf15e16e13f527491cdf7845b2fe6521a738d8f7c9c721866a8496e1fc8e
format  PE32+ x86-64 DLL, 7 sections
built   2026-08-11 20:29:31 UTC (COFF timestamp)
version FileVersion = ProductVersion = 310.8.0.0  ("NVIDIA DLSSNR - DVS PRODUCTION")
```

Supplied by the owner
(zip sha256 `388c0a7912e15ec911b9c9e11a692142b11fe387ddf2b637d8c358138fffb3ac`).
Provenance is **cryptographic, not reputational** — see `notes/phase0-acquire.md`.

## Section map

Produced by `src/tools/pe_inspect.py` (our own PE parser; radare2 was never needed).
Entropy is sampled 8 MB per section, evenly spread.

| section | vaddr | raw size | % of file | entropy | what |
|---|---|---|---|---|---|
| `.text` | 0x00001000 | 699 904 | 0.4 % | 6.44 | host code |
| `.rdata` | 0x000ac000 | 210 944 | 0.1 % | 5.74 | |
| `.data` | 0x000e0000 | 17 180 672 | 10.4 % | 6.06 | **CUDA fatbins live here** |
| `.pdata` | 0x0115b000 | 34 816 | 0.0 % | 5.81 | |
| `_RDATA` | 0x01164000 | 512 | 0.0 % | 2.45 | |
| `.rsrc` | 0x01165000 | **147 697 152** | **89.1 %** | **5.89** | **the weight blob** |
| `.reloc` | 0x09e40000 | 5 120 | 0.0 % | 5.28 | |

**Confirms the reported "~89 % of the file is weights"** — and locates them: they are
in `.rsrc`, as a resource, not in a bespoke section.

## Correction to the roadmap: there is no `.nv_fatbin`

notes/CLAUDE.md Phases 1–2 assumed a `.nv_fatbin` section to carve with `dd`. There is
none — the literal string `.nv_fatbin` does not occur anywhere in the file. Instead:

- **15 fatbin containers** (magic `BA55ED50`), first at file offset `0xdf0e0`
- **15 ELF cubins** (`\x7fELF`), first at file offset `0x1f9220`

Both ranges fall inside `.data` (raw 0xdea00 … 0x1141200). So Phase 2 must carve the
15 fatbins out of `.data` by magic, not lift a named section.

`-arch sm_120` appears in embedded compile command lines — **Blackwell-only confirmed**,
previously only reported.

## Imports — nothing external

```
ADVAPI32.dll  KERNEL32.dll  USER32.dll  VERSION.dll
```

No `cudart`, no `cudnn`, no `cublas`, no `nvinfer`, no `onnxruntime`. The DLL is
self-contained: it carries its own kernels and drives them through NGX. Nothing to
reimplement from a third-party inference library — the graph is entirely in here.

## Open question raised by this pass

`.rsrc` entropy is **5.89**, well below the ~7.5–8.0 expected of densely packed FP8
weights. That is a lot of redundancy for 147 MB of parameters. Either the blob has
container structure/padding around the tensors, or the weights are not stored as a
flat dense FP8 array. Resolve before writing the dequantiser in Phase 3 — do not
assume a flat `E4M3[148e6]` layout.
