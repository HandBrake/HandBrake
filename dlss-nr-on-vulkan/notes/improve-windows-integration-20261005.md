# Windows changes integrated into improve

2026-10-05. Source: published `windows` tip `e616afc`, 12 commits newer than the
main-line Windows port at `3d8951c`. The pre-integration improve tip `814e84a` is
retained locally as `backup-improve-before-windows-814e84a`.

## Included behavior

- Windows NumPy large-block reuse, through `libnr_alloc.dll`, with a bounded cache
  and `NR_KEEP_BLOCKS=0` fallback.
- Direct named-pipe reads/writes and a request buffer reused across frames. The
  request-buffer reuse also runs on Linux; shape changes replace the stored buffer.
- Optional x86 Windows layer build for 32-bit games, alongside the x64 layer.
- Raw 128-bit operand loads in the staged GEMM, selected by default only on Intel's
  proprietary Windows driver. Other drivers keep the previous loader;
  `XMX_STAGED_PACKED=0/1` permits a controlled comparison.
- Windows profiling tools, documentation and build-output placement fixes.

Our Windows quick start, DLL manifest preparation and DOOM check remain present.
The quick start now describes the integrated x86 layer and allocator library.
README gains **How to test it**, before **What it looks like**, with OS setup links,
small-window starting conditions and report requirements.

Only HANDOFF conflicted. Both its Windows investigation and our user-guide entry
were retained. BGRA8 experiment branches are not part of this integration.

## Additional correction

`NamedPipeConnection` marked `_open=False` after EOF or a broken write, then
`close()` checked that flag and skipped releasing the handle. A status probe can
reach this path without sending a header. `close()` now releases its owned handle
once regardless of the stream flag and clears the handle. A mocked-kernel lifetime
test checks read EOF and broken writes on Linux too; real transport remains a
separate Windows test.

## Validation

- Clean CMake configure/rebuild passed on Linux.
- Full CTest: all 44 entries passed in 132.94 seconds, including `gpu_staged_packed`.
  The added shader test compares old/new loaders in separate processes with
  changing inputs, epilogues, batches, alignment fallbacks and guards.
- After the pipe-handle correction, daemon regressions passed again with host
  socket access. The handle-lifetime test and Linux/Windows manifest fixtures passed.
- Request-buffer checks passed for same-size overwrite, an independent mask,
  truncated-input recovery and a clean empty status probe.
- Build-system parity, claims and publication checks passed. README section order
  and setup anchors were checked.

The Windows-only allocator/real-pipe tests are registered on Windows, not exercised
by this Linux run. Windows MSVC builds and gameplay were not rerun here. The prior
Windows branch's recorded tests and timings are retained as prior evidence; its
performance improvements are not new measurements on this machine.

This is the reviewable integration candidate for master. At preparation time,
master remains `3d8951c`; promoting it would bring the changes above and the user
documentation into the default branch. The Linux loader default is unchanged, but
this note does not claim a new Linux performance benchmark.
