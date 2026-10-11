# Phase 38 — there was no driver bug

2026-09-10. Since phase 18 this project has carried a Mesa/ANV cooperative-matrix bug:
"any operation on the accumulator between `coopMatMulAdd` and `coopMatStore` scrambles
the result". It shaped the epilogue's design, it was the reason the vendor's own fp16
accumulator could not pay for itself (phase 31), it was named in HANDOFF as the one
thing that clearly belonged upstream, and phase 36 had just sharpened it to a precise,
narrow claim ready to file.

It does not exist. The reproducer written to file it is what found that out.

## The last row of the table was our own invalid shader

Phase 36 had already knocked four of the five rows down: whole-matrix arithmetic,
element assignment and copies through a fresh matrix are all correct. One row survived —
storing through a `float16` matrix — and that row was this:

```glsl
layout(...) buffer C { float c[]; };          // a float32 destination
coopmat<float16_t, ..., gl_MatrixUseAccumulator> narrow = coopmat<float16_t, ...>(acc);
coopMatStore(narrow, c, 0, N, gl_CooperativeMatrixLayoutRowMajor);
```

A float16 matrix stored into an array of `float`. The component type does not match the
destination's. glslang accepts it; the result is garbage, and the shape of the garbage
says exactly what happened — 79 of 128 values came back zero and the rest were
denormal-sized, which is what packed float16 pairs look like when read as float32.

The type-correct version — `TOUCH=5` in `src/probe/coopmat_store_bug.comp`, the same
conversion stored into a `float16_t[]` — is **exact**:

```
float16 matrix -> float16_t[] buffer
max |d| 0   mean |d| 0   CORRECT
```

Every one of the five variants works. There is nothing to report to Mesa.

## What it cost, and what it is worth

The cost: phase 21's shared-memory detour was written to dodge a bug that was not there.
It is correct and it is not free — an extra round trip and a subgroup barrier per GEMM —
and phase 36 already removed it for float32 outputs on the strength of a *partial*
correction. It can now go for narrow outputs too.

The worth is larger. Phase 31 measured the vendor's fp16 accumulator at **1.36x to 2.24x
on isolated GEMMs**, halving the register pressure exactly as the arithmetic predicts,
and concluded it bought nothing in a frame — because every GEMM had to detour through
shared memory, and the detour cost what the accumulator saved. That conclusion rested on
the bug. With a type-correct direct store the detour is unnecessary, and the measurement
has to be redone.

## The lesson, which is the point of writing this down

Three phases of design and one nearly-filed upstream report rested on a claim that was
never isolated. Phase 18 measured inside the resident kernel — buffer-reference
addressing, batch strides, slice offsets, an epilogue loop — and read "wrong answer" as
"driver bug" rather than "something in this kernel is wrong".

A minimal reproducer is not politeness for the maintainer. It is the only way to find
out that the bug is yours.
