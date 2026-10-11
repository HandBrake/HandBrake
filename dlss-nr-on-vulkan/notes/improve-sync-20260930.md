# Synchronizing improve with Claude, 2026-09-30

Merged the published master/improve-int8 tip `72a7440` (16 new commits) into
`improve`. Runtime sources and build definitions now match that tip. Kept the
existing OpenMP prerequisite in README and the previous synchronization note.
Resolved the CMake conflict using upstream's newer equivalent implementation,
including its Make/CMake parity check and native math-library linkage.

The unfinished BGRA8 composition prototype is preserved locally on
`experiment-bgra8`, commit `d83377c`. It is not enabled or included in this merge:
the initial row-decoding implementation passed its byte comparisons but measured
slower than float RGB composition. Any continuation needs to rebase it onto the
new color-processing code and repeat correctness and performance measurements.

Validation on the local Intel LNL GPU:

- Clean CMake configure and rebuild passed; `make -n all` has no remaining work.
- All 42 CTest entries passed in 170.48 seconds, including the present negative
  control and build-system parity check.
- All six selected tests passed with `XMX_STAGING=1`: frame execution, global
  attention, window block, scratch arena, staged32 and native image operations.

These are local integration checks, not a new B570/B580 performance measurement.
The media branch already matches origin at `9fb91b1`; no media merge is needed.
Other worktrees and unpublished Windows/subgroup experiments were left intact.
