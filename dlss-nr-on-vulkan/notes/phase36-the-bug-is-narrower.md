# Phase 36 — the driver bug is narrower than we recorded, and four of five rows were wrong

2026-09-10. Phase 18 wrote down that **any** operation on a cooperative matrix between
`coopMatMulAdd` and `coopMatStore` scrambles the result, with a table of five variants
all marked wrong but the control. Everything since has been built around that: the
epilogue's shared-memory detour (phase 21), and the fp16 accumulator being unable to
pay for itself (phase 31).

Writing a **minimal reproducer** for the upstream report — which is the point of writing
one — showed that four of those five rows are wrong.

`src/probe/coopmat_store_bug.comp`, one shader, `TOUCH` selecting what is done, driven
by the standalone `src/gpu/gemm_runner.c`. Mesa 26.2.1, ANV, Arc 140V, 8x16x16:

| what is done to the accumulator | phase 18 | **measured now** |
|---|---|---|
| nothing (control) | correct | correct |
| `acc = acc * 2.0`, whole matrix | wrong | **correct** |
| `acc[0] = acc[0] * 2.0`, one component | wrong | **correct** |
| copied to a fresh matrix and back | wrong | **correct** |
| stored through a `float16` matrix | wrong | **wrong** |

The element-assignment row needed care to read: `acc[0]` is the first component *this
invocation holds*, not element (0,0) of the tile, so a correct result doubles 32 of the
128 elements and leaves 96. That is exactly what comes back — 32 doubled, 96 unchanged,
0 stray, with three near-zero elements matching both tests.

**Only the converting store is broken.** Storing a float16 accumulator into a float16
destination — not a conversion, so in principle the legal case — is broken too, so the
fault is in the narrowing store path rather than in arithmetic on the matrix.

Why phase 18 saw otherwise is worth guessing at but not asserting: its test ran inside
the resident kernel, with `buffer_reference` addressing, batch strides and slice
offsets, none of which the reproducer has. A minimal case is not just better manners for
a bug report; it is how the claim got corrected.

## What that buys

A float32 output no longer needs the detour. The publish runs on the accumulator's own
components and the matrix stores straight to global — no shared-memory round trip, no
subgroup barrier. Every epilogue stays bit-exact (`src/gpu/test_epilogue.py`, max |d| 0
in all seven cases), and **720p goes 490 -> 470 ms**, consistent across processes.

A narrow output still goes through shared memory, because that is the case the driver
actually gets wrong.

## What it means for the report

The bug to file is much sharper than "arithmetic on a cooperative matrix is broken":

> On ANV / Lunar Lake / Mesa 26.2.1, `coopMatStore` of an accumulator whose component
> type differs from the destination's writes the wrong values — not a permutation of the
> right ones. Arithmetic on the accumulator, element assignment and copies are all fine.

That is a claim someone can act on in an afternoon, with a two-file reproducer attached.
It is also still worth filing: it is what stops the vendor's own fp16 accumulator from
paying for itself here (phase 31 measured 1.36x on the GEMMs behind it).
