# Synchronizing improve with the published Claude work

`improve` advanced from `92ee241` to `62d7215` by fast-forward: 44 commits, no merge
conflicts and no rewritten history. The earlier fixes, including the private-fence
present path and direct Proton app IDs, remain in its ancestry. The source was the
committed `improve-int8` state, also published as `origin/master` at the time of review.
The local branch named master was stale; it was not the integration target.

The working copy in ProjectsClaude had uncommitted changes in ten GPU/reference
files. Those were deliberately left in that worktree, not copied into this branch.

## Important behavior change

`62d7215` corrects the decoder's skip inputs: outputs of transition blocks 4, 8, 14
and 22 replace the preceding blocks' outputs. The output image changes intentionally;
pre-fix hashes are not parity targets for the corrected model. See
`notes/opendlss-reference.md` for the reference comparison and its limits.

Other included changes reduce scratch storage, fuse attention and composition,
remove bottleneck conversions, and improve small-token handling. INT8 remains a
measured kernel/quantization experiment rather than the default inference graph;
see `notes/improve-int8-bottleneck.md`.

## Integration repair

CMake had not followed the expanding Makefile. It now builds the staged32/deep,
row-softmax, INT8, window-block, global-attention and fused-FFN shaders, and tracks
their included GLSL dependencies. The native image library links OpenMP and uses
the same non-trapping, non-fast-math flags as Make. The added GPU tests are now
registered with CTest. A clean CMake build leaves `make -n all` with nothing to do.

## Validation

- Clean CMake configure/build: passed.
- Full CTest: all 40 entries passed on Intel LNL in 106.37 seconds.
- Forced staging: all six selected CTest entries passed in 6.97 seconds
  (frame execution, scratch arena, global attention, window block, staged32 and
  native image operations).

The performance figures in the inherited notes were not re-benchmarked in this
synchronization task. No claim about B570/B580 performance is added here.
