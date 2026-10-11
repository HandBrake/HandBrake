# Phase 40 — what the third review found in `src/gpu`, and what I found first

2026-09-10. Branch `review-gpu` against `pre-gpu`, 29 files, 5423 lines. The last of the
three free reviews.

## The result is itself a finding

Four findings, **all four rated nit**. Not one correctness bug in the runtime that
records and dispatches every GEMM in the graph. Compare `src/layer`, reviewed the same
day: eight findings, one of them a feature that had never worked in a game
(`notes/phase39-layer-review.md`).

That asymmetry is not luck and it is worth naming. `src/gpu` is the part of this project
that has been measured on hardware at every step — every optimisation carried a
before/after and an output comparison, and a wrong answer showed up as a wrong picture.
`src/layer` is the part that runs inside someone else's process, where being wrong looks
like nothing happening. **The code with a tight feedback loop was already right; the code
without one was not.**

## The four

| file | what | real? |
| --- | --- | --- |
| `libxmx.c` | `XMX_TILE_M=0` divides by zero in `xmx_rec_gemm` | **yes, verified** |
| `gemm_runner.c` | `slurp()` calls `fread` on an unchecked `malloc` | yes |
| `nr_xmx.py` | `matmul_nt` skips the lazy `calibrate()` that `matmul` does | **yes, verified** |
| `test_gemm.py`, `test_layer.py` | `mkdtemp` per call, never removed | yes |

Both marked *verified* were reproduced rather than reasoned about:

```
$ XMX_TILE_M=0 python3 src/gpu/test_resident.py
Floating point exception (core dumped)      exit 136
```

```
>>> nr_xmx.matmul_nt(a, b)
TypeError: unsupported operand type(s) for *: 'NoneType' and 'int'
```

Both now pass.

## The tile knob is sharper than the finding said

The review saw a division by zero. The division is the *mild* failure. `g.tilem` and
`g.tilen` are not free parameters: `work/gemm_tiled.spv` is built with `-DRM=2 -DRN=2`,
so its workgroup writes a fixed 16x32 block, while the dispatch computes its grid by
dividing the extent by these two numbers. They have to agree.

- `XMX_TILE_M=0` — SIGFPE. Loud, and the one the review found.
- `XMX_TILE_M=8` — twice the workgroups, each still writing 16 rows. Overlapping writes
  and a run past the bottom edge, which `cooperativeMatrixRobustBufferAccess = false`
  will not catch. **Silent.**
- `XMX_TILE_M=-1` — `unsigned`, so it becomes a number no extent divides. The knob
  quietly does nothing and the tiled path never fires; a benchmark would read as "tiling
  is worth 0 %".

`block_size()` now refuses anything that is not a positive number, says so on stderr, and
falls back. The comment above it states the constraint, because the second and third
cases cannot be fixed by validation — only by knowing the rule.

## Audited before the findings came back

While the review ran I checked the one rule that has already cost this project three
phases: **a cooperative matrix must be stored into an array of its own component type**
(`notes/phase38-there-was-no-bug.md`). All 11 `coopMatStore` calls across the five GEMM
shaders comply. Barriers are right too — `gemm_resident.comp` is one subgroup per
workgroup (`local_size_x = 32`, `subgroupSize = 32`), so its `subgroupBarrier()` covers
the shared stage, and `gemm_staged.comp` uses a full `barrier()` where `WARPS` may exceed
one.

What that audit *did* turn up is in commit `81f7197`, and it is the more interesting
half: five places in the code still asserted the withdrawn phase-18 driver bug as fact,
including a seventeen-line comment in `gemm_resident.comp` that contradicts both the code
directly beneath it and line 152 of the same file. A retired finding lives on wherever it
was written down, and `notes/` was only one of those places.
